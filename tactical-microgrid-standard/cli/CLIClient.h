#ifndef CLI_CLI_CLIENT_H
#define CLI_CLI_CLIENT_H

#include "common/Handshaking.h"
#include "controller/Common.h"

#include <cli_idl/CLICommandsTypeSupportImpl.h>
#include <power_devices/PowerSimTypeSupportImpl.h>

#include <string>
#include <utility>
#include <mutex>
#include <unordered_set>

struct ConnectedDeviceEqual {
  bool operator()(const powersim::ConnectedDevice& cd1, const powersim::ConnectedDevice& cd2) const
  {
    return cd1.id() == cd2.id() && cd1.role() == cd2.role();
  }
};

struct ConnectedDeviceHash {
  size_t operator()(const powersim::ConnectedDevice& cd) const
  {
    return std::hash<tms::Identity>{}(cd.id());
  }
};

class CLIClient {
public:
  explicit CLIClient(const tms::Identity& id);
  ~CLIClient() {}

  DDS::ReturnCode_t init(DDS::DomainId_t domain_id, int argc = 0, char* argv[] = nullptr);

  void run();

private:
  // Initialize DDS entities in the TMS domain
  DDS::ReturnCode_t init_tms(DDS::DomainId_t tms_domain_id, int argc = 0, char* argv[] = nullptr);

  // Initialize DDS entities used for CLI commands and power simulation
  DDS::ReturnCode_t init_sim(DDS::DomainId_t sim_domain_id);

  void run_cli();
  bool cli_stopped() const;

  void tolower(std::string& s) const;
  OpArgPair parse(const std::string& input) const;

  void display_commands() const;
  std::string energy_level_to_string(tms::EnergyStartStopLevel essl) const;

  enum class ControllerStatus {
    AVAILABLE,
    UNAVAILABLE,
  };

  ControllerStatus controller_status(TimePoint now, TimePoint last_heartbeat) const;
  void display_controllers() const;
  bool collect_power_devices();
  void display_power_devices() const;
  void list_power_devices();
  bool is_single_port_device(tms::DeviceRole role) const;

  // Check that two power devices can have a power connection
  bool can_connect(const tms::Identity& id1, tms::DeviceRole role1,
    const tms::Identity& id2, tms::DeviceRole role2) const;

  // Create a power connection between two devices
  void connect(const tms::Identity& id1, tms::DeviceRole role1,
    const tms::Identity& id2, tms::DeviceRole role2);
  void consolidate_power_devices();
  void connect_power_devices();
  bool send_power_devices_request(const tms::Identity& mc_id);
  void send_start_device_cmd(const OpArgPair& op_arg);
  void send_stop_device_cmd(const OpArgPair& op_arg);
  void send_start_stop_request(const OpArgPair& op_arg, tms::OperatorPriorityType opt);
  void send_suspend_controller_cmd(const OpArgPair& op_pair) const;
  void send_resume_controller_cmd(const OpArgPair& op_pair) const;
  void send_terminate_controller_cmd(const OpArgPair& op_pair) const;
  void send_controller_cmd(const OpArgPair& op_pair, cli::ControllerCmdType cmd_type) const;

  void process_device_info(const tms::DeviceInfo& di, const DDS::SampleInfo& si);
  void process_heartbeat(const tms::Heartbeat& hb, const DDS::SampleInfo& si);

  int handle_signal(int, siginfo_t*, ucontext_t*);

  ACE_Reactor* const reactor_ = ACE_Reactor::instance();
  DDS::DomainParticipant_var sim_participant_;
  Handshaking handshaking_;
  cli::PowerDevicesRequestDataWriter_var pdreq_dw_;
  tms::OperatorIntentRequestDataWriter_var oir_dw_;
  cli::PowerDevicesReplyDataReader_var pdrep_dr_;
  cli::ControllerCommandDataWriter_var cc_dw_;
  powersim::PowerTopologyDataWriter_var pt_dw_;

  // If a heartbeat hasn't been received from a controller in this amount of time
  // since its last heartbeat, the controller is deemed unavailable.
  // The delay is the sum of the missed controller delay (3s) and the lost controller
  // delay (6s) from the TMS spec.
  static constexpr Sec missed_controller_delay{3};
  static constexpr Sec lost_controller_delay{6};
  static constexpr Sec unavail_controller_delay = missed_controller_delay + lost_controller_delay;

  struct ControllerInfo {
    tms::DeviceInfo info;
    TimePoint last_hb;
  };

  mutable std::mutex cli_m_;
  bool stop_cli_;

  // Mutex for the following data
  mutable std::mutex data_m_;

  // Microgrid controllers that are (and were) reachable by the CLI
  std::unordered_map<tms::Identity, ControllerInfo> controllers_;

  // For each controller, cache the power devices that select it as the active controller.
  // This mapping can change during the operation of the microgrid due to the availability of the MCs.
  std::unordered_map<tms::Identity, PowerDevices> mc_to_devices_;

  // All power devices in the microgrid, reported by the MCs.
  PowerDevices power_devices_;

  // Store the simulated power connections between power devices
  using PowerConnection = std::unordered_map<tms::Identity,
    std::unordered_set<powersim::ConnectedDevice, ConnectedDeviceHash, ConnectedDeviceEqual>>;
  PowerConnection power_connections_;
};

#endif
