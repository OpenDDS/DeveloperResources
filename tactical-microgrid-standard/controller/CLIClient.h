#ifndef CLI_CLIENT_H
#define CLI_CLIENT_H

#include "Controller.h"
#include "CLICommandsTypeSupportImpl.h"

#include <string>
#include <utility>

class CLIClient {
public:
  CLIClient(const tms::Identity& id = "CLI Client") : device_id_(id) {}

  ~CLIClient() {}

  DDS::ReturnCode_t init(DDS::DomainId_t domain_id, int argc = 0, char* argv[] = nullptr);
  void run();

private:
  OpArgPair parse(const std::string& input) const;

  void display_commands() const;
  std::string device_role_to_string(tms::DeviceRole role) const;
  void display_power_devices(const Controller::PowerDevices& pd) const;

  void send_power_devices_request();
  void send_start_device_cmd(const OpArgPair& op_arg);
  void send_stop_device_cmd(const OpArgPair& op_arg);
  void send_stop_controller_cmd();
  void send_resume_controller_cmd();
  void send_terminate_cmd();

  DDS::DomainParticipantFactory_var dpf_;
  DDS::DomainParticipant_var participant_;
  cli::PowerDevicesRequestDataWriter_var pdreq_dw_;
  tms::OperatorIntentRequestDataWriter_var oir_dw_;

  tms::Identity device_id_;
};

#endif
