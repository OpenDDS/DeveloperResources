#ifndef CONTROLLER_CLI_SERVER_H
#define CONTROLLER_CLI_SERVER_H

#include "Controller.h"

#include <dds/DCPS/optional.h>

// TODO: Create DDS readers for:
// - PowerDevicesRequest
// - OperatorIntentRequest
// Create DDS writer for:
// - PowerDevicesReply
// - OperatorIntentState (maybe not)
class CLIServer {
public:
  CLIServer(Controller& mc);
  ~CLIServer() {}

private:
  void init();

  void start_device(const OpArgPair& oparg) const;

  void stop_device(const OpArgPair& oparg) const;

  void stop_controller();

  void resume_controller();

  void terminate() const;

private:
  Controller& controller_;
  bool controller_active_;
};

#endif
