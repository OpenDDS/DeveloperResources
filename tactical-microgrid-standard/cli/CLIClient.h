#ifndef CONTROLLER_CLI_CLIENT_H
#define CONTROLLER_CLI_CLIENT_H

#include "common/Handshaking.h"
#include "controller/Common.h"

#include <cli_idl/CLICommandsTypeSupportImpl.h>

#include <string>
#include <utility>
#include <mutex>

struct UnavailableController {
  tms::Identity id;
};

class CLIClient : public TimerHandler<UnavailableController> {
public:
  explicit CLIClient(const tms::Identity& id);
  ~CLIClient() {}

  DDS::ReturnCode_t init(DDS::DomainId_t domain_id, int argc = 0, char* argv[] = nullptr);
  void run();

private:
  void run_cli();
  bool cli_stopped() const;

  void tolower(std::string& s) const;
  OpArgPair parse(const std::string& input) const;

  void display_commands() const;
  std::string device_role_to_string(tms::DeviceRole role) const;
  void display_controllers() const;
  void set_controller(const OpArgPair& op_arg);
  void list_power_devices();
  bool send_power_devices_request();
  void display_power_devices() const;
  void send_start_device_cmd(const OpArgPair& op_arg) const;
  void send_stop_device_cmd(const OpArgPair& op_arg) const;
  void send_start_stop_request(const OpArgPair& op_arg, tms::OperatorPriorityType opt) const;
  void send_stop_controller_cmd() const;
  void send_resume_controller_cmd() const;
  void send_terminate_controller_cmd() const;
  void send_controller_cmd(cli::ControllerCmdType cmd_type) const;

  void process_device_info(const tms::DeviceInfo& di, const DDS::SampleInfo& si);
  void process_heartbeat(const tms::Heartbeat& hb, const DDS::SampleInfo& si);

  void any_timer_fired(TimerHandler<UnavailableController>::AnyTimer any_timer) final;
  int handle_signal(int, siginfo_t*, ucontext_t*);

  Handshaking handshaking_;
  cli::PowerDevicesRequestDataWriter_var pdreq_dw_;
  tms::OperatorIntentRequestDataWriter_var oir_dw_;
  cli::PowerDevicesReplyDataReader_var pdrep_dr_;
  cli::ControllerCommandDataWriter_var cc_dw_;

  // If a heartbeat hasn't been received from a controller in this amount of time
  // since its last heartbeat, the controller is deemed unavailable.
  // The delay is the sum of the missed controller delay (3s) and the lost controller
  // delay (6s) from the TMS spec.
  static constexpr Sec missed_controller_delay{3};
  static constexpr Sec lost_controller_delay{6};
  static constexpr Sec unavail_controller_delay = missed_controller_delay + lost_controller_delay;

  struct ControllerInfo {
    enum class Status {
      AVAILABLE,
      UNAVAILABLE,
    };

    tms::DeviceInfo info;
    Status status;
    TimePoint last_hb;
  };

  mutable std::mutex cli_m_;
  bool stop_cli_;

  mutable std::mutex data_m_;

  // Microgrid controllers to which the CLI client is connected
  std::unordered_map<tms::Identity, ControllerInfo> controllers_;

  // The power devices that are connected to the current controller
  PowerDevices power_devices_;

  // The current microgrid controller with which the CLI client is interacting
  tms::Identity curr_controller_;
};

#endif
