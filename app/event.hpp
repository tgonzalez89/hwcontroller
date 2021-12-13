#pragma once

#include <cstdint>


enum class EventType { WriteReg, RegUpdated };


class Event {
public:
  Event(EventType type) : type(type) {}

  virtual ~Event() {};

  auto const &get_type() const {
    return type;
  }

private:
  EventType type;
};


enum RegType { pwr, temp, lvl, valve_open };
static const char * RegTypeStrings[] = {"pwr", "temp", "lvl", "valve_open"};
const char * RegType2Str(int val) {
  return RegTypeStrings[val];
}
RegType Str2RegType(std::string val) {
  int i;
  for (i = 0; i <= 3; ++i) {
    if (std::string(RegTypeStrings[i]) == val) {
      break;
    }
  }
  return (RegType)i;
}


class EventReg : public Event {
public:
  EventReg(EventType type, RegType reg_type, int id, int data = 0) :
  Event(type), reg_type(reg_type), id(id), data(data) {}

  virtual ~EventReg() {};

  auto const &get_reg_type() const {
    return reg_type;
  }

  auto const &get_data() const {
    return data;
  }

  void set_data(int val) {
    data = val;
  }

  auto const &get_id() const {
    return id;
  }

private:
  RegType reg_type;
  int id;
  int data;
};


class EventWriteReg : public EventReg {
public:
  EventWriteReg(RegType reg_type, int id, int data = 0) :
  EventReg(EventType::WriteReg, reg_type, id, data) {}
};

class EventRegUpdated : public EventReg {
public:
  EventRegUpdated(RegType reg_type, int id, int data = 0) :
  EventReg(EventType::RegUpdated, reg_type, id, data) {}
};
