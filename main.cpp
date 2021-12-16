#include "app/adapter.hpp"
#include "hw/hwsim.hpp"
#include "ctrl/controller.hpp"
#include "socket/socket_server.hpp"


int main() {
  Mediator mediator;

  HwSim1 hw_sim1;
  HwSim1Adapter hw_sim1_adapter(mediator, hw_sim1, 1);

  Controller controller1;
  ControllerAdapter controller1_adapter(mediator, controller1, 1);

  HwSim2 hw_sim2;
  HwSim2Adapter hw_sim2_adapter(mediator, hw_sim2, 2);

  Controller controller2;
  ControllerAdapter controller2_adapter(mediator, controller2, 2);

  SocketServer socket_server;
  GuiAdapter gui_adapter(mediator, socket_server);

  hw_sim1.start();
  controller1.start();
  hw_sim2.start();
  controller2.start();

  socket_server.run();

  hw_sim1.stop();
  controller1.stop();
  hw_sim2.stop();
  controller2.stop();

  return 0;
}
