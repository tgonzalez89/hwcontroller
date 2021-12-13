#pragma once

#include <stdexcept>
#include <memory>
#include <vector>
#include "event.hpp"
#include "mediator.hpp"
#include "../common/subscriber.hpp"
#include "../hw/hwsim.hpp"
#include "../ctrl/controller.hpp"
#include "../socket/socket_server.hpp"


class HwSim1Adapter : public Component, public Subscriber {
public:
  HwSim1Adapter(Mediator &mediator, HwSim1 &adaptee, int id) :
  Component(mediator), adaptee(adaptee), id(id) {
    adaptee.subscribe(this);
  }

  void update() override {
    std::unique_ptr<Event> e(std::make_unique<EventRegUpdated>(
      RegType::temp, id, adaptee.get_temp()
    ));
    say(std::move(e));
  }

  void notify_impl(Event *event) override {
    switch (event->get_type()) {
      case EventType::WriteReg: {
        auto e = dynamic_cast<EventWriteReg *>(event);
        switch (e->get_reg_type()) {
          case RegType::pwr:
            if (id == e->get_id())
              adaptee.set_pwr(static_cast<uint8_t>(e->get_data() * UINT8_MAX / 100));
            break;
          default:
            break;
        }
        break;
      }
      default:
        break;
    }
  }

private:
  HwSim1 &adaptee;
  int id;
};


class HwSim2Adapter : public Component, public Subscriber {
public:
  HwSim2Adapter(Mediator &mediator, HwSim2 &adaptee, int id) :
  Component(mediator), adaptee(adaptee), id(id) {
    adaptee.subscribe(this);
  }

  void update() override {
    std::unique_ptr<Event> e;

    e = std::make_unique<EventRegUpdated>(RegType::temp, id, adaptee.get_temp());
    say(std::move(e));

    e = std::make_unique<EventRegUpdated>(RegType::lvl, id, adaptee.get_lvl() * 100 / UINT8_MAX);
    say(std::move(e));
  }

  void notify_impl(Event *event) override {
    switch (event->get_type()) {
      case EventType::WriteReg: {
        auto e = dynamic_cast<EventWriteReg *>(event);
        switch (e->get_reg_type()) {
          case RegType::pwr:
            if (id == e->get_id())
              adaptee.set_pwr(static_cast<uint8_t>(e->get_data() * UINT8_MAX / 100));
            break;
          case RegType::valve_open:
            if (id == e->get_id())
              adaptee.set_valve_open(static_cast<bool>(e->get_data()));
            break;
          default:
            break;
        }
        break;
      }
      default:
        break;
    }
  }

private:
  HwSim2 &adaptee;
  int id;
};


class ControllerAdapter : public Component, public Subscriber {
public:
  ControllerAdapter(Mediator &mediator, Controller &adaptee, int id) :
  Component(mediator), adaptee(adaptee), id(id) {
    adaptee.subscribe(this);
  }

  void update() override {
    std::unique_ptr<Event> e;

    e = std::make_unique<EventWriteReg>(RegType::pwr, id, adaptee.get_pwr());
    say(std::move(e));
  }

  void notify_impl(Event *event) override {
    switch (event->get_type()) {
      case EventType::RegUpdated: {
        auto e = dynamic_cast<EventRegUpdated *>(event);
        switch (e->get_reg_type()) {
          case RegType::temp:
            if (id == e->get_id())
              adaptee.set_temp(e->get_data());
            break;
          default:
            break;
        }
        break;
      }
      default:
        break;
    }
  }

private:
  Controller &adaptee;
  int id;
};


class GuiAdapter : public Component {
public:
  GuiAdapter(Mediator &mediator, SocketServer &adaptee) :
  Component(mediator), adaptee(adaptee) {
    start();
  }

  ~GuiAdapter() {
    stop();
  }

  void notify_impl(Event *event) override {
    switch (event->get_type()) {
      case EventType::RegUpdated: {
        auto e = dynamic_cast<EventRegUpdated *>(event);
        std::stringstream ss;
        ss << "RegUpdated " << RegType2Str(e->get_reg_type()) << " " << e->get_id() << " " << e->get_data();
        adaptee.read_queue.put(ss.str());
        break;
      }
      case EventType::WriteReg: {
        auto e = dynamic_cast<EventWriteReg *>(event);
        std::stringstream ss;
        ss << "WriteReg " << RegType2Str(e->get_reg_type()) << " " << e->get_id() << " " << e->get_data();
        adaptee.read_queue.put(ss.str());
        break;
      }
      default:
        break;
    }
  }

private:
  SocketServer &adaptee;
  std::mutex m;
  volatile bool run_state = false;
  std::thread t;

  void run() {
    while (get_run_state()) {
      auto msg = adaptee.write_queue.get();

      std::istringstream iss(msg);
      std::vector<std::string> tokens{
        std::istream_iterator<std::string>{iss},
        std::istream_iterator<std::string>{}
      };

      std::unique_ptr<Event> e;
      int id, val;
      std::istringstream(tokens[2]) >> id;
      std::istringstream(tokens[3]) >> val;
      e = std::make_unique<EventWriteReg>(Str2RegType(tokens[1]), id, val);
      say(std::move(e));
    }
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
    adaptee.write_queue.put("None 0 0 0");
    if (t.joinable()) t.join();
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
