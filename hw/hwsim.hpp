#pragma once

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <mutex>
#include <thread>
#include <typeinfo>
#include <vector>
#include "../common/subscriber.hpp"


class IHwSim {
public:
  virtual void start() = 0;
  virtual void stop() = 0;
  virtual ~IHwSim() {}
};


class HwSim1 : IHwSim {
public:
  HwSim1() {
    srand(time(NULL));
  }

  void subscribe(Subscriber *s) {
    subs.push_back(s);
  }

  void start() override {
    std::scoped_lock lock(m);
    if (run_state == false) {
      run_state = true;
      t = std::thread( [this] { run(); } );
    }
  }

  void stop() override {
    set_run_state(false);
    if (t.joinable()) t.join();
  }

  void set_pwr(uint8_t val) {
    std::scoped_lock lock(m);
    pwr = val;
  }

  int16_t get_temp() {
    std::scoped_lock lock(m);
    return temp;
  }

protected:
  static constexpr unsigned TICK = 500;
  std::mutex m;
  std::vector<Subscriber *> subs;

  virtual void run() {
    while (get_run_state()) {
      run_internal();

      for (auto sub : subs) sub->update();

      std::this_thread::sleep_for(std::chrono::milliseconds(TICK));
    }
  }

  void run_internal() {
    const auto local_pwr = get_pwr();
    auto local_temp = get_temp();

    if (local_pwr < UINT8_MAX / 2) {
      if (local_temp > INT16_MIN + 3)
        local_temp -= rand() % 3 + 1;
    }
    else if (local_pwr > UINT8_MAX / 2) {
      if (local_temp < INT16_MAX - 3)
        local_temp += rand() % 3 + 1;
    }

    set_temp(local_temp);

    //m.lock();
    //std::cout << typeid(*this).name() << " temp=" << local_temp << " pwr=" << static_cast<int>(local_pwr)*100/255 << "%" << std::endl;
    //m.unlock();
  }

  bool get_run_state() {
    std::scoped_lock lock(m);
    return run_state;
  }

  void set_run_state(bool val) {
    std::scoped_lock lock(m);
    run_state = val;
  }

private:
  volatile uint8_t pwr = 0;
  volatile int16_t temp = 25;
  volatile bool run_state = false;
  std::thread t;

  uint8_t get_pwr() {
    std::scoped_lock lock(m);
    return pwr;
  }

  void set_temp(int16_t val) {
    std::scoped_lock lock(m);
    temp = val;
  }
};


class HwSim2 : public HwSim1 {
public:
    uint8_t get_lvl() {
    std::scoped_lock lock(m);
    return lvl;
  }

  void set_valve_open(bool val) {
    std::scoped_lock lock(m);
    valve_open = val;
  }

private:
  volatile uint8_t lvl = 127;
  volatile bool valve_open = false;

  void run() override {
    while (get_run_state()) {
      run_internal();

      const auto local_valve_open = get_valve_open();
      auto local_lvl = get_lvl();

      if (local_valve_open) {
        if (local_lvl > 3) {
          local_lvl -= rand() % 3 + 1;
        }
      }
      else {
        if (local_lvl < UINT8_MAX - 3) {
          local_lvl += rand() % 3 + 1;
        }
      }

      set_lvl(local_lvl);

      //m.lock();
      //std::cout << typeid(*this).name() << " lvl=" << static_cast<int>(local_lvl)*100/255 << "% valve_open=" << local_valve_open << std::endl;
      //m.unlock();

      for (auto sub : subs) sub->update();

      std::this_thread::sleep_for(std::chrono::milliseconds(TICK));
    }
  }

  void set_lvl(uint8_t val) {
    std::scoped_lock lock(m);
    lvl = val;
  }

  bool get_valve_open() {
    std::scoped_lock lock(m);
    return valve_open;
  }
};
