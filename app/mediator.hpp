#pragma once

#include <memory>
#include <vector>
#include "event.hpp"


class IComponent {
public:
  virtual void say(std::unique_ptr<Event> event) = 0;
  virtual void notify(IComponent *sender, Event *event) = 0;
};


class IMediator {
public:
  virtual void broadcast(IComponent *sender, std::unique_ptr<Event> event) = 0;
  virtual void add_component(IComponent *component) = 0;
};


class Mediator : IMediator {
protected:
  std::vector<IComponent *> components;

public:
  virtual void broadcast(IComponent *sender, std::unique_ptr<Event> event) override {
    for (auto c : components) {
      c->notify(sender, event.get());
    }
  }

  virtual void add_component(IComponent *component) override {
    components.push_back(component);
  }
};


class Component : IComponent {
protected:
  Mediator &mediator;
  virtual void notify_impl(Event *event) = 0;

public:
  Component(Mediator &mediator) : mediator(mediator) {
    mediator.add_component(this);
  }

  virtual ~Component() {}

  virtual void say(std::unique_ptr<Event> event) override {
    mediator.broadcast(this, std::move(event));
  }

  virtual void notify(IComponent *sender, Event *event) override {
    if (sender == this) return;
    notify_impl(event);
  }
};
