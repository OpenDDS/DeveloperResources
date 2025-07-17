#include "CLIClient.h"
#include "common/QosHelper.h"
#include "common/Utils.h"

#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/WaitSet.h>

#include <cctype>
#include <thread>
#include <iomanip>

CLIClient::CLIClient(const tms::Identity& id)
  : handshaking_(id)
  , stop_cli_(false)
{}

DDS::ReturnCode_t CLIClient::init(DDS::DomainId_t tms_domain_id, int argc, char* argv[])
{
  const DDS::ReturnCode_t rc = init_tms(tms_domain_id, argc, argv);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  const DDS::DomainId_t sim_domain_id = Utils::get_sim_domain_id(tms_domain_id);
  return init_sim(sim_domain_id);
}

DDS::ReturnCode_t CLIClient::init_tms(DDS::DomainId_t domain_id, int argc, char* argv[])
{
  DDS::ReturnCode_t rc = handshaking_.join_domain(domain_id, argc, argv);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  // Subscribe to the DeviceInfo and Heartbeat to learn about microgrid controllers
  // But not publish to these topics.
  rc = handshaking_.create_subscribers(
    [&](const tms::DeviceInfo& di, const DDS::SampleInfo& si) { process_device_info(di, si); },
    [&](const tms::Heartbeat& hb, const DDS::SampleInfo& si) { process_heartbeat(hb, si); });
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  DDS::DomainParticipant_var dp = handshaking_.get_domain_participant();

  // Publish to the tms::OperatorIntentRequest topic
  tms::OperatorIntentRequestTypeSupport_var oir_ts = new tms::OperatorIntentRequestTypeSupportImpl;
  if (DDS::RETCODE_OK != oir_ts->register_type(dp, "")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: register_type OperatorIntentRequest failed\n"));
    return DDS::RETCODE_ERROR;
  }

  CORBA::String_var oir_type_name = oir_ts->get_type_name();
  DDS::Topic_var oir_topic = dp->create_topic(tms::topic::TOPIC_OPERATOR_INTENT_REQUEST.c_str(),
                                              oir_type_name,
                                              TOPIC_QOS_DEFAULT,
                                              nullptr,
                                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!oir_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: create_topic \"%C\" failed\n",
               tms::topic::TOPIC_OPERATOR_INTENT_REQUEST.c_str()));
    return DDS::RETCODE_ERROR;
  }

  const DDS::PublisherQos tms_pub_qos = Qos::Publisher::get_qos();
  DDS::Publisher_var tms_pub = dp->create_publisher(tms_pub_qos,
                                                    nullptr,
                                                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!tms_pub) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: create_publisher with TMS QoS failed\n"));
    return DDS::RETCODE_ERROR;
  }

  const tms::Identity device_id = handshaking_.get_device_id();
  const DDS::DataWriterQos& oir_qos = Qos::DataWriter::fn_map.at(tms::topic::TOPIC_OPERATOR_INTENT_REQUEST)(device_id);
  DDS::DataWriter_var oir_dw_base = tms_pub->create_datawriter(oir_topic,
                                                               oir_qos,
                                                               nullptr,
                                                               ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!oir_dw_base) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: create_datawriter for topic \"%C\" failed\n",
               tms::topic::TOPIC_OPERATOR_INTENT_REQUEST.c_str()));
    return DDS::RETCODE_ERROR;
  }

  oir_dw_ = tms::OperatorIntentRequestDataWriter::_narrow(oir_dw_base);
  if (!oir_dw_) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: OperatorIntentRequestDataWriter narrow failed\n"));
    return DDS::RETCODE_ERROR;
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t CLIClient::init_sim(DDS::DomainId_t sim_domain_id)
{
  DDS::DomainParticipantFactory_var dpf = handshaking_.get_participant_factory();
  sim_participant_ = dpf->create_participant(sim_domain_id,
                                             PARTICIPANT_QOS_DEFAULT,
                                             nullptr,
                                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!sim_participant_) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: create simulation participant failed\n"));
    return DDS::RETCODE_ERROR;
  }

  // Separate transport instance for the simulation domain
  Utils::setup_sim_transport(sim_participant_);

  // Publish to the cli::PowerDevicesRequest topic
  cli::PowerDevicesRequestTypeSupport_var pdreq_ts = new cli::PowerDevicesRequestTypeSupportImpl;
  if (DDS::RETCODE_OK != pdreq_ts->register_type(sim_participant_, "")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: register_type PowerDevicesRequest failed\n"));
    return DDS::RETCODE_ERROR;
  }

  CORBA::String_var pdreq_type_name = pdreq_ts->get_type_name();
  DDS::Topic_var pdreq_topic = sim_participant_->create_topic(cli::TOPIC_POWER_DEVICES_REQUEST.c_str(),
                                                              pdreq_type_name,
                                                              TOPIC_QOS_DEFAULT,
                                                              nullptr,
                                                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pdreq_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: create_topic \"%C\" failed\n",
               cli::TOPIC_POWER_DEVICES_REQUEST.c_str()));
    return DDS::RETCODE_ERROR;
  }

  DDS::Publisher_var pub = sim_participant_->create_publisher(PUBLISHER_QOS_DEFAULT,
                                                              nullptr,
                                                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pub) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: create_publisher failed\n"));
    return DDS::RETCODE_ERROR;
  }

  DDS::DataWriterQos dw_qos;
  pub->get_default_datawriter_qos(dw_qos);
  dw_qos.reliability.kind = DDS::ReliabilityQosPolicyKind::RELIABLE_RELIABILITY_QOS;

  DDS::DataWriter_var pdreq_dw_base = pub->create_datawriter(pdreq_topic,
                                                             dw_qos,
                                                             nullptr,
                                                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pdreq_dw_base) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: create_datawriter for topic \"%C\" failed\n",
               cli::TOPIC_POWER_DEVICES_REQUEST.c_str()));
    return DDS::RETCODE_ERROR;
  }

  pdreq_dw_ = cli::PowerDevicesRequestDataWriter::_narrow(pdreq_dw_base);
  if (!pdreq_dw_) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: PowerDevicesRequestDataWriter narrow failed\n"));
    return DDS::RETCODE_ERROR;
  }

  // Publish to the cli::ControllerCommand topic
  cli::ControllerCommandTypeSupport_var cc_ts = new cli::ControllerCommandTypeSupportImpl;
  if (DDS::RETCODE_OK != cc_ts->register_type(sim_participant_, "")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: register_type ControllerCommand failed\n"));
    return DDS::RETCODE_ERROR;
  }

  CORBA::String_var cc_type_name = cc_ts->get_type_name();
  DDS::Topic_var cc_topic = sim_participant_->create_topic(cli::TOPIC_CONTROLLER_COMMAND.c_str(),
                                                           cc_type_name,
                                                           TOPIC_QOS_DEFAULT,
                                                           nullptr,
                                                           ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!cc_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: create_topic \"%C\" failed\n",
               cli::TOPIC_CONTROLLER_COMMAND.c_str()));
    return DDS::RETCODE_ERROR;
  }

  DDS::DataWriter_var cc_dw_base = pub->create_datawriter(cc_topic,
                                                          dw_qos,
                                                          nullptr,
                                                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!cc_dw_base) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: create_datawriter for topic \"%C\" failed\n",
               cli::TOPIC_CONTROLLER_COMMAND.c_str()));
    return DDS::RETCODE_ERROR;
  }

  cc_dw_ = cli::ControllerCommandDataWriter::_narrow(cc_dw_base);
  if (!cc_dw_) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: ControllerCommandDataWriter narrow failed\n"));
    return DDS::RETCODE_ERROR;
  }

  // Subscribe to the cli::PowerDevicesReply topic
  cli::PowerDevicesReplyTypeSupport_var pdrep_ts = new cli::PowerDevicesReplyTypeSupportImpl;
  if (DDS::RETCODE_OK != pdrep_ts->register_type(sim_participant_, "")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: register_type PowerDevicesReply failed\n"));
    return DDS::RETCODE_ERROR;
  }

  CORBA::String_var pdrep_type_name = pdrep_ts->get_type_name();
  DDS::Topic_var pdrep_topic = sim_participant_->create_topic(cli::TOPIC_POWER_DEVICES_REPLY.c_str(),
                                                              pdrep_type_name,
                                                              TOPIC_QOS_DEFAULT,
                                                              nullptr,
                                                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pdrep_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: create_topic \"%C\" failed\n",
               cli::TOPIC_POWER_DEVICES_REPLY.c_str()));
    return DDS::RETCODE_ERROR;
  }

  DDS::Subscriber_var sub = sim_participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                                                nullptr,
                                                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!sub) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: create_subscriber failed\n"));
    return DDS::RETCODE_ERROR;
  }

  DDS::DataReaderQos dr_qos;
  sub->get_default_datareader_qos(dr_qos);
  dr_qos.reliability.kind = DDS::ReliabilityQosPolicyKind::RELIABLE_RELIABILITY_QOS;

  DDS::DataReader_var pdrep_dr_base = sub->create_datareader(pdrep_topic,
                                                             dr_qos,
                                                             nullptr,
                                                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pdrep_dr_base) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: create_datareader for topic \"%C\" failed\n",
               cli::TOPIC_POWER_DEVICES_REPLY.c_str()));
    return DDS::RETCODE_ERROR;
  }

  pdrep_dr_ = cli::PowerDevicesReplyDataReader::_narrow(pdrep_dr_base);
  if (!pdrep_dr_) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: PowerDevicesReplyDataReader narrow failed\n"));
    return DDS::RETCODE_ERROR;
  }

  // Publish to the powersim::PowerTopology topic
  powersim::PowerTopologyTypeSupport_var pt_ts = new powersim::PowerTopologyTypeSupportImpl;
  if (DDS::RETCODE_OK != pt_ts->register_type(sim_participant_, "")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: register_type PowerTopology failed\n"));
    return DDS::RETCODE_ERROR;
  }

  CORBA::String_var pt_type_name = pt_ts->get_type_name();
  DDS::Topic_var pt_topic = sim_participant_->create_topic(powersim::TOPIC_POWER_TOPOLOGY.c_str(),
                                                           pt_type_name,
                                                           TOPIC_QOS_DEFAULT,
                                                           nullptr,
                                                           ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pt_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: create_topic \"%C\" failed\n",
               powersim::TOPIC_POWER_TOPOLOGY.c_str()));
    return DDS::RETCODE_ERROR;
  }

  DDS::DataWriter_var pt_dw_base = pub->create_datawriter(pt_topic,
                                                          dw_qos,
                                                          nullptr,
                                                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pt_dw_base) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: create_datawriter for topic \"%C\" failed\n",
               powersim::TOPIC_POWER_TOPOLOGY.c_str()));
    return DDS::RETCODE_ERROR;
  }

  pt_dw_ = powersim::PowerTopologyDataWriter::_narrow(pt_dw_base);
  if (!pt_dw_) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: PowerTopologyDataWriter narrow failed\n"));
    return DDS::RETCODE_ERROR;
  }

  return DDS::RETCODE_OK;
}

bool CLIClient::cli_stopped() const
{
  std::lock_guard<std::mutex> guard(cli_m_);
  return stop_cli_;
}

void CLIClient::display_commands() const
{
  const char* msg = R"(=== Command-Line Interface for Microgrid Controller (MC) ===
list-mc          : list the connected MCs.
list-pd          : list the power devices reported by the connected MCs.
connect-pd       : connect power devices to simulate the power topology of a microgrid.
start   <pd_id>  : start a power device with the given Id.
stop    <pd_id>  : stop a power device with the given Id.
suspend <mc_id>  : suspend the heartbeats of the given MC (simulating an MC becomming unavailable).
resume  <mc_id>  : resume the heartbeats of the given MC (simulating an MC becomming available).
term    <mc_id>  : terminate the given MC's process.
show             : display this list of CLI commands.)";
  std::cout << msg << std::endl;
}

void CLIClient::run_cli()
{
  const std::string prompt = "ENTER COMMAND:> ";
  display_commands();
  std::cout << prompt;

  std::string line;
  while (!cli_stopped()) {
    std::getline(std::cin, line);
    auto op_pair = parse(line);
    tolower(op_pair.first);
    const std::string& op = op_pair.first;

    if (op == "list-mc") {
      display_controllers();
    } else if (op == "list-pd") {
      list_power_devices();
    } else if (op == "connect-pd") {
      connect_power_devices();
    } else if (op == "start") {
      send_start_device_cmd(op_pair);
    } else if (op == "stop") {
      send_stop_device_cmd(op_pair);
    } else if (op == "suspend") {
      send_suspend_controller_cmd(op_pair);
    } else if (op == "resume") {
      send_resume_controller_cmd(op_pair);
    } else if (op == "term") {
      send_terminate_controller_cmd(op_pair);
    } else if (op == "show") {
      display_commands();
    } else {
      std::cout << "Unknown operation entered!" << std::endl;
    }
    std::cout << '\n' << prompt;
  }
}

void CLIClient::run()
{
  std::thread thr(&CLIClient::run_cli, this);
  reactor_->run_reactor_event_loop();
  thr.join();
}

void CLIClient::tolower(std::string& s) const
{
  for (size_t i = 0; i < s.size(); ++i) {
    s[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(s[i])));
  }
}

OpArgPair CLIClient::parse(const std::string& input) const
{
  const std::string whitespace = " \t";
  const size_t first = input.find_first_not_of(whitespace);
  if (first == std::string::npos) {
    return std::make_pair("", std::optional<std::string>());
  }

  const size_t last = input.find_last_not_of(whitespace);
  const std::string trimed_input = input.substr(first, last - first + 1);
  const size_t delim = trimed_input.find_first_of(whitespace);
  if (delim == std::string::npos) {
    return std::make_pair(trimed_input, std::optional<std::string>());
  }

  const std::string op = trimed_input.substr(0, delim);
  const size_t arg_begin = trimed_input.find_first_not_of(whitespace, delim + 1);
  const std::string arg = trimed_input.substr(arg_begin);
  return std::make_pair(op, std::optional<std::string>(arg));
}

std::string CLIClient::energy_level_to_string(tms::EnergyStartStopLevel essl) const
{
  switch (essl) {
  case tms::EnergyStartStopLevel::ESSL_OPERATIONAL:
    return "Operational";
  case tms::EnergyStartStopLevel::ESSL_READY_SYNCED:
    return "Ready Synced";
  case tms::EnergyStartStopLevel::ESSL_READY:
    return "Ready";
  case tms::EnergyStartStopLevel::ESSL_IDLE:
    return "Idle";
  case tms::EnergyStartStopLevel::ESSL_WARM:
    return "Warm";
  case tms::EnergyStartStopLevel::ESSL_OFF:
    return "Off";
  case tms::EnergyStartStopLevel::ESSL_ANY:
    return "Any";
  default:
    return "Unknown";
  }
}

// Collect the list of power devices from the available MCs.
bool CLIClient::collect_power_devices()
{
  bool ret = true;
  const auto now = Clock::now();
  for (auto it = controllers_.begin(); it != controllers_.end(); ++it) {
    const auto status = controller_status(now, it->second.last_hb);
    if (status == ControllerStatus::AVAILABLE) {
      ret &= send_power_devices_request(it->first);
    } else {
      // There may be stale entries that need to be deleted,
      // so displaying power devices looks clean.
      mc_to_devices_.erase(it->first);
    }
  }
  return ret;
}

void CLIClient::display_power_devices() const
{
  size_t i = 1;
  for (auto it = mc_to_devices_.begin(); it != mc_to_devices_.end(); ++it) {
    const auto mc_id = it->first;
    const auto power_devices = it->second;
    std::cout << "Devices connected to Microgrid Controller \"" << mc_id << "\":" << std::endl;
    for (auto it2 = power_devices.begin(); it2 != power_devices.end(); ++it2) {
      const std::string formated_id = std::string("\"") + it2->first + "\"";
      std::string selected_controller = std::string("\"") + it2->second.master_id().value_or("Undetermined") + "\"";

      std::cout << std::right << std::setfill(' ') << std::setw(3) << i++
        << ". Id: " << std::left << std::setw(15) << formated_id
        << "| Type: " << std::left << std::setw(18) << Utils::device_role_to_string(it2->second.device_info().role())
        << "| Energy Level: " << std::left << std::setw(15) << energy_level_to_string(it2->second.essl())
        << "| Active Controller: " << std::left << selected_controller << std::endl;
    }
    std::cout << std::endl;
  }
}

void CLIClient::list_power_devices()
{
  std::lock_guard<std::mutex> guard(data_m_);
  collect_power_devices();
  display_power_devices();
}

bool CLIClient::is_single_port_device(tms::DeviceRole role) const
{
  switch (role) {
  case tms::DeviceRole::ROLE_SOURCE:
  case tms::DeviceRole::ROLE_LOAD:
  case tms::DeviceRole::ROLE_STORAGE:
    return true;
  default:
    return false;
  }
}

bool CLIClient::can_connect(const tms::Identity& id1, tms::DeviceRole role1,
                            const tms::Identity& id2, tms::DeviceRole role2) const
{
  if (id1 == id2) {
    return false;
  }

  const bool dev1_is_not_connected = power_connections_.count(id1) == 0 || power_connections_.at(id1).empty();
  const bool dev2_is_not_connected = power_connections_.count(id2) == 0 || power_connections_.at(id2).empty();

  if (dev1_is_not_connected || !is_single_port_device(role1)) {
    if (dev2_is_not_connected) {
      return true;
    }
    return !is_single_port_device(role2);
  }
  return false;
}

void CLIClient::connect(const tms::Identity& id1, tms::DeviceRole role1,
                        const tms::Identity& id2, tms::DeviceRole role2)
{
  power_connections_[id1].insert(powersim::ConnectedDevice{id2, role2});
  power_connections_[id2].insert(powersim::ConnectedDevice{id1, role1});
}

// Gather information for all power devices in the microgrid from all MCs.
// The total information of all devices is then used to construct a topology
// of power devices to simulate a microgrid with devices connected by power cable.
// This is necessary when simulating network partition is supported, e.g., MC 1
// has connections to a subset of devices and MC 2 has connections to the remaining devices.
// When there is no network partition, all MCs should have the same set of devices.
void CLIClient::consolidate_power_devices()
{
  if (mc_to_devices_.empty()) {
    collect_power_devices();
  }

  for (auto it = mc_to_devices_.begin(); it != mc_to_devices_.end(); ++it) {
    const PowerDevices& pds = it->second;
    power_devices_.insert(pds.begin(), pds.end());
  }
}

// Construct connections between power devices to simulate a microgrid.
// Each connection simulates a physical connection using a power cable.
void CLIClient::connect_power_devices()
{
  std::lock_guard<std::mutex> guard(data_m_);
  if (power_devices_.empty()) {
    consolidate_power_devices();
  }

  for (auto it1 = power_devices_.begin(); it1 != power_devices_.end(); ++it1) {
    const tms::Identity& id1 = it1->first;
    const tms::DeviceRole role1 = it1->second.device_info().role();
    auto it2 = it1;
    ++it2;
    std::cout << "Select devices to connect to device Id: " << id1 << std::endl;
    for (; it2 != power_devices_.end(); ++it2) {
      const tms::Identity& id2 = it2->first;
      const tms::DeviceRole role2 = it2->second.device_info().role();
      if (!can_connect(id1, role1, id2, role2)) {
        continue;
      }

      std::cout << "Connect to device Id: " << id2 << " (Y/n)";
      while (true) {
        std::string line;
        std::getline(std::cin, line);
        if (line.empty() || line == "y" || line == "Y") {
          connect(id1, role1, id2, role2);
          break;
        } else if (line == "n" || line == "N") {
          break;
        } else {
          std::cerr << "Invalid input! Please try again." << std::endl;
          std::cout << "Connect to device Id: " << id2 << " (Y/n)";
        }
      }
    }
  }

  // Send the power topology to the current controller which then
  // distributes the power connections to its managed power devices.
  powersim::PowerTopology pt;
  pt.connections().reserve(power_connections_.size());
  CORBA::ULong i = 0;
  for (auto it = power_connections_.begin(); it != power_connections_.end(); ++it, ++i) {
    powersim::PowerConnection pc;
    pc.pd_id() = it->first;
    pc.connected_devices().reserve(it->second.size());
    for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
      pc.connected_devices().push_back(*it2);
    }
    pt.connections().push_back(pc);
  }

  // Forward to the first available controller in the list
  const auto now = Clock::now();
  bool sent = false;
  for (auto it = controllers_.begin(); !sent && it != controllers_.end(); ++it) {
    if (controller_status(now, it->second.last_hb) == ControllerStatus::UNAVAILABLE) {
      continue;
    }

    const auto& mc_id = it->first;
    pt.mc_id() = mc_id;
    DDS::ReturnCode_t rc = pt_dw_->write(pt, DDS::HANDLE_NIL);
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::connect_power_devices: "
                 "write power topology to controller \"%C\" failed\n", mc_id.c_str()));
    } else {
      sent = true;
    }
  }

  if (!sent) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::connect_power_devices: "
               "Failed to setup power topology for the simulation!\n"));
  }
}

CLIClient::ControllerStatus CLIClient::controller_status(TimePoint now, TimePoint last_heartbeat) const
{
  return (now - last_heartbeat < unavail_controller_delay) ? ControllerStatus::AVAILABLE : ControllerStatus::UNAVAILABLE;
}

void CLIClient::display_controllers() const
{
  std::lock_guard<std::mutex> guard(data_m_);
  std::cout << "Number of Connected Microgrid Controllers: " << controllers_.size() << std::endl;
  size_t i = 1;
  const auto now = Clock::now();
  for (auto it = controllers_.begin(); it != controllers_.end(); ++it) {
    std::cout << i << ". Controller Id: " << it->first << " (" <<
      (controller_status(now, it->second.last_hb) == ControllerStatus::AVAILABLE ? "available)" : "unavailable)")
      << std::endl;
  }
}

// Caller must already hold data_m_ lock
bool CLIClient::send_power_devices_request(const tms::Identity& mc_id)
{
  cli::PowerDevicesRequest pd_req;
  pd_req.mc_id(mc_id);

  DDS::ReturnCode_t rc = pdreq_dw_->write(pd_req, DDS::HANDLE_NIL);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: CLIClient::send_power_devices_request: "
               "write to controller \"%C\" failed\n", mc_id.c_str()));
    return false;
  }

  // Wait for the reply
  DDS::StringSeq params;
  params.length(1);
  params[0] = mc_id.c_str();
  DDS::QueryCondition_var qc = pdrep_dr_->create_querycondition(DDS::NOT_READ_SAMPLE_STATE,
                                                                DDS::ANY_VIEW_STATE,
                                                                DDS::ANY_INSTANCE_STATE,
                                                                "mc_id = %0",
                                                                params);
  if (!qc) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: CLIClient::send_power_devices_request: "
               "create_querycondition to receive from controller \"%C\" failed\n",
               mc_id.c_str()));
    return false;
  }

  DDS::WaitSet_var ws = new DDS::WaitSet;
  ws->attach_condition(qc);
  DDS::ConditionSeq cond_seq;
  const DDS::Duration_t forever = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };
  rc = ws->wait(cond_seq, forever);
  ws->detach_condition(qc);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: CLIClient::send_power_devices_request: "
               "WaitSet's wait returned \"%C\"\n", OpenDDS::DCPS::retcode_to_string(rc)));
    return false;
  }

  cli::PowerDevicesReplySeq data;
  DDS::SampleInfoSeq info_seq;
  rc = pdrep_dr_->take(data, info_seq, DDS::LENGTH_UNLIMITED,
                       DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: CLIClient::send_power_devices_request: "
               "take data failed: %C\n", OpenDDS::DCPS::retcode_to_string(rc)));
    return false;
  }

  bool received = false;
  for (CORBA::ULong i = 0; !received && i < data.length(); ++i) {
    if (data[i].mc_id() != mc_id) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: CLIClient::send_power_devices_request: "
                 "reply expected from controller \"%C\". Received from \"%C\"\n",
                 mc_id.c_str(), data[i].mc_id().c_str()));
      continue;
    }

    if (info_seq[i].valid_data) {
      const cli::PowerDeviceInfoSeq& pdi_seq = data[i].devices();
      auto& power_devices = mc_to_devices_[mc_id];
      for (auto it = pdi_seq.begin(); it != pdi_seq.end(); ++it) {
        // Add to the existing list of power devices.
        // This allows power devices to be added gradually in case
        // the "list-pd" command is issued before all devices have joined.
        power_devices.insert_or_assign(it->device_info().deviceId(), *it);
      }
    } else if (info_seq[i].instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
      mc_to_devices_.erase(mc_id);
    }
    received = true;
  }

  if (!received) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: CLIClient::send_power_devices_request: "
               "Failed to receive data from controller (%C)\n", mc_id.c_str()));
  }
  return true;
}

void CLIClient::send_start_stop_request(const OpArgPair& op_arg,
                                        tms::OperatorPriorityType opt)
{
  if (!op_arg.second.has_value()) {
    std::cerr << "No power device specified!" << std::endl;
    return;
  }
  auto& pd_id = op_arg.second.value();

  std::lock_guard<std::mutex> guard(data_m_);
  if (mc_to_devices_.empty()) {
    collect_power_devices();
  }

  // Send to start/stop request to the controller that manages this power device
  tms::Identity target_mc;
  for (auto it = mc_to_devices_.begin(); it != mc_to_devices_.end(); ++it) {
    const auto& devices = it->second;
    if (devices.count(pd_id)) {
      target_mc = it->first;
      break;
    }
  }

  if (target_mc.empty()) {
    std::cerr << "There is no controller for the power device \"" << pd_id << "\"!!!" << std::endl;
    return;
  }

  const tms::Identity my_id = handshaking_.get_device_id();

  tms::OperatorIntentRequest oir;
  oir.requestId().requestingDeviceId() = my_id;
  // This doesn't seem to get used since there is no reply for this request.
  oir.sequenceId() = 0;
  tms::OperatorIntent oi;
  oi.requestId().requestingDeviceId() = target_mc;
  oi.intentType() = tms::OperatorIntentType::OIT_OPERATOR_DEFINED;
  tms::DeviceIntent intent;
  intent.deviceId() = pd_id;
  intent.battleShort() = false;
  intent.priority().priorityType() = opt;
  oi.devices().push_back(intent);
  oir.desiredOperatorIntent() = oi;

  DDS::ReturnCode_t rc = oir_dw_->write(oir, DDS::HANDLE_NIL);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: CLIClient::send_start_stop_request: write %C request returned \"%C\"\n",
               opt == tms::OperatorPriorityType::OPT_ALWAYS_OPERATE ? "start" : "stop",
               OpenDDS::DCPS::retcode_to_string(rc)));
    return;
  }
}

void CLIClient::send_start_device_cmd(const OpArgPair& op_arg)
{
  send_start_stop_request(op_arg, tms::OperatorPriorityType::OPT_ALWAYS_OPERATE);
}

void CLIClient::send_stop_device_cmd(const OpArgPair& op_arg)
{
  send_start_stop_request(op_arg, tms::OperatorPriorityType::OPT_NEVER_OPERATE);
}

void CLIClient::send_controller_cmd(const OpArgPair& op_arg, cli::ControllerCmdType cmd_type) const
{
  std::lock_guard<std::mutex> guard(data_m_);
  if (!op_arg.second.has_value()) {
    std::cerr << "No microgrid controller specified!" << std::endl;
    return;
  }

  const auto& mc_id = op_arg.second.value();
  if (!controllers_.count(mc_id)) {
    std::cerr << "Unknown controller \"" << mc_id << "\"!!!" << std::endl;
    return;
  }

  cli::ControllerCommand cmd;
  cmd.mc_id() = mc_id;
  cmd.type() = cmd_type;

  DDS::ReturnCode_t rc = cc_dw_->write(cmd, DDS::HANDLE_NIL);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_WARNING, "CLIClient::send_controller_cmd: write ControllerCommand to MC \"%C\" failed: \"%C\"\n",
               mc_id.c_str(), OpenDDS::DCPS::retcode_to_string(rc)));
  }
}

void CLIClient::send_suspend_controller_cmd(const OpArgPair& op_arg) const
{
  send_controller_cmd(op_arg, cli::ControllerCmdType::CCT_STOP);
}

void CLIClient::send_resume_controller_cmd(const OpArgPair& op_arg) const
{
  send_controller_cmd(op_arg, cli::ControllerCmdType::CCT_RESUME);
}

void CLIClient::send_terminate_controller_cmd(const OpArgPair& op_arg) const
{
  send_controller_cmd(op_arg, cli::ControllerCmdType::CCT_TERMINATE);
}

void CLIClient::process_device_info(const tms::DeviceInfo& di, const DDS::SampleInfo& si)
{
  std::lock_guard<std::mutex> guard(data_m_);
  if (si.valid_data) {
    if (di.role() == tms::DeviceRole::ROLE_MICROGRID_CONTROLLER) {
      controllers_.insert(std::make_pair(di.deviceId(), ControllerInfo{ di, Clock::now() }));
    }
  } else if (si.instance_state & DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE){
    controllers_.erase(di.deviceId());
  }
}

void CLIClient::process_heartbeat(const tms::Heartbeat& hb, const DDS::SampleInfo& si)
{
  std::lock_guard<std::mutex> guard(data_m_);
  if (si.valid_data) {
    auto it = controllers_.find(hb.deviceId());
    if (it != controllers_.end()) {
      it->second.last_hb = Clock::now();
    }
  } else if (si.instance_state & DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
    controllers_.erase(hb.deviceId());
  }
}

int CLIClient::handle_signal(int, siginfo_t*, ucontext_t*)
{
  std::lock_guard<std::mutex> cli_guard(cli_m_);
  stop_cli_ = true;

  reactor_->end_reactor_event_loop();
  return -1;
}
