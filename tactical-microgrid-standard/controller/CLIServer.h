#ifndef CONTROLLER_CLI_SERVER_H
#define CONTROLLER_CLI_SERVER_H

#include "Controller.h"

#include <cli_idl/CLICommandsTypeSupportImpl.h>
#include <power_devices/PowerSimTypeSupportImpl.h>

class CLIServer {
public:
  explicit CLIServer(Controller& mc);
  ~CLIServer() {}

  Controller& get_controller() const
  {
    return controller_;
  }

  cli::PowerDevicesReplyDataWriter_var get_PowerDevicesReply_writer() const
  {
    return pdrep_dw_;
  }

  powersim::PowerConnectionDataWriter_var get_PowerConnection_writer() const
  {
    return pc_dw_;
  }

  void start_stop_device(const tms::Identity& pd_id, tms::OperatorPriorityType opt);
  void receive_reply(const tms::Reply& reply);

private:
  DDS::ReturnCode_t init();
  DDS::ReturnCode_t init_tms();
  DDS::ReturnCode_t init_sim();

  tms::EnergyStartStopLevel ESSL_from_OPT(tms::OperatorPriorityType opt);
  std::string replycode_to_string(tms::ReplyCode code);

  // Provide sequence number for the EnergyStartStopRequest
  tms::RequestSequence essr_seqnum_ = 0;

  // Requests that are waiting for a reply from the target power device
  std::unordered_map<tms::RequestSequence, tms::Identity> pending_essr_;
  std::mutex pending_essr_m_;

  Controller& controller_;
  cli::PowerDevicesReplyDataWriter_var pdrep_dw_;
  tms::EnergyStartStopRequestDataWriter_var essr_dw_;
  powersim::PowerConnectionDataWriter_var pc_dw_;
  DDS::DomainParticipant_var sim_participant_;
};

#endif
