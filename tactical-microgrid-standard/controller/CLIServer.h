#ifndef CONTROLLER_CLI_SERVER_H
#define CONTROLLER_CLI_SERVER_H

#include "Controller.h"
#include "CLICommandsTypeSupportImpl.h"

class CLIServer {
public:
  CLIServer(Controller& mc);
  ~CLIServer() {}

  Controller& get_controller() const
  {
    return controller_;
  }

  cli::PowerDevicesReplyDataWriter_var get_PowerDevicesReply_writer() const
  {
    return pdrep_dw_;
  }

private:
  DDS::ReturnCode_t init();

  void start_device(const OpArgPair& oparg) const;
  void stop_device(const OpArgPair& oparg) const;
  void stop_controller();
  void resume_controller();
  void terminate() const;

  Controller& controller_;
  bool controller_active_;

  cli::PowerDevicesRequestDataReader_var pdreq_dr_;
  tms::OperatorIntentRequestDataReader_var oir_dr_;
  cli::PowerDevicesReplyDataWriter_var pdrep_dw_;
};

#endif
