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

  void start_stop_device(const tms::Identity& pd_id, tms::OperatorPriorityType opt);

private:
  DDS::ReturnCode_t init();

  Controller& controller_;
  cli::PowerDevicesRequestDataReader_var pdreq_dr_;
  tms::OperatorIntentRequestDataReader_var oir_dr_;
  cli::PowerDevicesReplyDataWriter_var pdrep_dw_;
  cli::ControllerCommandDataReader_var cc_dr_;
};

#endif
