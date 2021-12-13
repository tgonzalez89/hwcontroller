#pragma once

#include <chrono>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include "../common/subscriber.hpp"


class Controller {
public:
  void subscribe(Subscriber *s) {
    subs.push_back(s);
  }

  void start() {
    std::scoped_lock lock(m);
    if (run_state == false) {
      run_state = true;
      t = std::thread( [this] { run(); } );
    }
  }

  void stop() {
    set_run_state(false);
    if (t.joinable()) t.join();
  }

  uint8_t get_pwr() {
    std::scoped_lock lock(m);
    return pwr;
  }

  void set_temp(int16_t val) {
    std::scoped_lock lock(m);
    temp = val;
  }

protected:
  void run() {
    int target_temp = 25;
    float const Kp = 5.0f;
    float const Ki = 1.0f;
    float const Kd = 1.0f;
    float prev_err = 0.0f;
    float integral = 0.0f;

    while (get_run_state()) {
      auto local_temp = get_temp();

      float error = target_temp - local_temp;
      float proportional = error;
      integral = integral + error * (static_cast<float>(TICK) / 1000);
      float derivative = (error - prev_err) / (static_cast<float>(TICK) / 1000);
      float local_pwr = Kp * proportional + Ki * integral + Kd * derivative;
      prev_err = error;

      if (local_pwr < 0) local_pwr = 0;
      else if (local_pwr > 100) local_pwr = 100;

      set_pwr(local_pwr);
      for (auto sub : subs) sub->update();

      std::this_thread::sleep_for(std::chrono::milliseconds(TICK));
    }
  }

private:
  static constexpr unsigned TICK = 500;
  volatile uint8_t pwr = 0;
  volatile int16_t temp = 25;
  volatile bool run_state = false;
  std::mutex m;
  std::thread t;
  std::vector<Subscriber *> subs;

  void set_pwr(uint8_t val) {
    std::scoped_lock lock(m);
    pwr = val;
  }

  int16_t get_temp() {
    std::scoped_lock lock(m);
    return temp;
  }

  bool get_run_state() {
    std::scoped_lock lock(m);
    return run_state;
  }

  void set_run_state(bool val) {
    std::scoped_lock lock(m);
    run_state = val;
  }
};
