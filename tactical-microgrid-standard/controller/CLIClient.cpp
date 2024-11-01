#include "CLIClient.h"
#include "qos/QosHelper.h"

#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/Marked_Default_Qos.h>

#include <cctype>

CLIClient::CLIClient(const tms::Identity& id)
  : Handshaking(id)
{}

DDS::ReturnCode_t CLIClient::init(DDS::DomainId_t domain_id, int argc, char* argv[])
{
  DDS::ReturnCode_t rc = join_domain(domain_id, argc, argv);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  // Subscribe to the DeviceInfo and Heartbeat to learn about microgrid controllers
  // But not publish to these topics.
  rc = create_subscribers(
    [&](const tms::DeviceInfo& di, const DDS::SampleInfo& si) { process_device_info(di, si); },
    [&](const tms::Heartbeat& hb, const DDS::SampleInfo& si) { process_heartbeat(hb, si); });
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  // Publish to the PowerDevicesRequest topic
  cli::PowerDevicesRequestTypeSupport_var pdreq_ts = new cli::PowerDevicesRequestTypeSupportImpl;
  if (DDS::RETCODE_OK != pdreq_ts->register_type(participant_.in(), "")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: register_type PowerDevicesRequest failed\n"));
    return DDS::RETCODE_ERROR;
  }

  CORBA::String_var pdreq_type_name = pdreq_ts->get_type_name();
  DDS::Topic_var pdreq_topic = participant_->create_topic(cli::TOPIC_POWER_DEVICES_REQUEST.c_str(),
                                                          pdreq_type_name.in(),
                                                          TOPIC_QOS_DEFAULT,
                                                          0,
                                                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pdreq_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: create_topic \"%C\" failed\n",
               cli::TOPIC_POWER_DEVICES_REQUEST.c_str()));
    return DDS::RETCODE_ERROR;
  }

  DDS::Publisher_var pub = participant_->create_publisher(PUBLISHER_QOS_DEFAULT,
                                                          0,
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
                                                             0,
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

  // Publish to the TMS OperatorIntentRequest topic
  tms::OperatorIntentRequestTypeSupport_var oir_ts = new tms::OperatorIntentRequestTypeSupportImpl;
  if (DDS::RETCODE_OK != oir_ts->register_type(participant_.in(), "")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: register_type OperatorIntentRequest failed\n"));
    return DDS::RETCODE_ERROR;
  }

  CORBA::String_var oir_type_name = oir_ts->get_type_name();
  DDS::Topic_var oir_topic = participant_->create_topic(tms::topic::TOPIC_OPERATOR_INTENT_REQUEST.c_str(),
                                                        oir_type_name,
                                                        TOPIC_QOS_DEFAULT,
                                                        0,
                                                        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!oir_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: create_topic \"%C\" failed\n",
               tms::topic::TOPIC_OPERATOR_INTENT_REQUEST.c_str()));
    return DDS::RETCODE_ERROR;
  }

  const DDS::PublisherQos tms_pub_qos = Qos::Publisher::get_qos();
  DDS::Publisher_var tms_pub = participant_->create_publisher(tms_pub_qos,
                                                              0,
                                                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!tms_pub) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: create_publisher with TMS QoS failed\n"));
    return DDS::RETCODE_ERROR;
  }

  const DDS::DataWriterQos oir_qos = Qos::DataWriter::fn_map.at(tms::topic::TOPIC_OPERATOR_INTENT_REQUEST)(device_id_);
  DDS::DataWriter_var oir_dw_base = tms_pub->create_datawriter(oir_topic,
                                                               oir_qos,
                                                               0,
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

  // Subscribe to the PowerDevicesReply topic
  cli::PowerDevicesReplyTypeSupport_var pdrep_ts = new cli::PowerDevicesReplyTypeSupportImpl;
  if (DDS::RETCODE_OK != pdrep_ts->register_type(participant_.in(), "")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: register_type PowerDevicesReply failed\n"));
    return DDS::RETCODE_ERROR;
  }

  CORBA::String_var pdrep_type_name = pdrep_ts->get_type_name();
  DDS::Topic_var pdrep_topic = participant_->create_topic(cli::TOPIC_POWER_DEVICES_REPLY.c_str(),
                                                          pdrep_type_name.in(),
                                                          TOPIC_QOS_DEFAULT,
                                                          0,
                                                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pdrep_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: create_topic \"%C\" failed\n",
               cli::TOPIC_POWER_DEVICES_REPLY.c_str()));
    return DDS::RETCODE_ERROR;
  }

  DDS::Subscriber_var sub = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                                            0,
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
                                                             0,
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

  return DDS::RETCODE_OK;
}

// TODO: Send request and commands to the CLI server
void CLIClient::run()
{
  display_commands();
  std::string line;
  while (true) {
    std::getline(std::cin, line);
    auto op_pair = parse(line);
    tolower(op_pair.first);
    const std::string& op = op_pair.first;

    if (op == "list") {
      if (curr_controller_.empty()) {
        // No controller specified yet
        display_controllers();
      } else {
        // List the power devices connected to the selected controller
        send_power_devices_request();
        display_power_devices();
      }
    } else if (op == "set") {
      set_controller(op_pair);
    } else if (op == "enable") {
      send_start_device_cmd(op_pair);
    } else if (op == "disable") {
      send_stop_device_cmd(op_pair);
    } else if (op == "stop") {
      send_stop_controller_cmd();
    } else if (op == "resume") {
      send_resume_controller_cmd();
    } else if (op == "term") {
      send_terminate_cmd();
    } else {
      std::cout << "Unknown operation entered!\n" << std::endl;
    }
    display_commands();
  }
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
    return std::make_pair("", OpenDDS::DCPS::optional<std::string>());
  }

  const size_t last = input.find_last_not_of(whitespace);
  const std::string trimed_input = input.substr(first, last - first + 1);
  const size_t delim = trimed_input.find_first_of(whitespace);
  if (delim == std::string::npos) {
    return std::make_pair(trimed_input, OpenDDS::DCPS::optional<std::string>());
  }

  const std::string op = trimed_input.substr(0, delim);
  const size_t arg_begin = trimed_input.find_first_not_of(whitespace, delim + 1);
  const std::string arg = trimed_input.substr(arg_begin);
  return std::make_pair(op, OpenDDS::DCPS::optional<std::string>(arg));
}

void CLIClient::display_commands() const
{
  std::cout << "\n=== Command-Line Interface for Microgrid Controller === " << std::endl;
  std::cout << "\nTop-level commands:" << std::endl;
  std::cout << "[list] the connected microgrid controllers." << std::endl;
  std::cout << "[set] the current microgrid controller. Subsequent commands" << std::endl;
  std::cout << "      affect this controller until another Set command." << std::endl;
  std::cout << "\nController-bounded commands:" << std::endl;
  std::cout << "[list] the connected power devices." << std::endl;
  std::cout << "[enable] (start) a power device with Id." << std::endl;
  std::cout << "[disable] (stop) a power device with Id." << std::endl;
  std::cout << "[stop] the controller's heartbeats." << std::endl;
  std::cout << "[resume] the controller's heartbeats." << std::endl;
  std::cout << "[term](inate) the controller." << std::endl;
  std::cout << "\nEnter next operation: ";
}

std::string CLIClient::device_role_to_string(tms::DeviceRole role) const
{
  switch (role) {
  case tms::DeviceRole::ROLE_MICROGRID_CONTROLLER:
    return "Microgrid Controller";
  case tms::DeviceRole::ROLE_SOURCE:
    return "Source";
  case tms::DeviceRole::ROLE_LOAD:
    return "Load";
  case tms::DeviceRole::ROLE_STORAGE:
    return "Storage";
  case tms::DeviceRole::ROLE_DISTRIBUTION:
    return "Distribution";
  case tms::DeviceRole::ROLE_MICROGRID_DASHBOARD:
    return "Microgrid Dashboard";
  case tms::DeviceRole::ROLE_CONVERSION:
    return "Conversion";
  case tms::DeviceRole::ROLE_MONITOR:
    return "Monitor";
  default:
    return "Unknown";
  }
}

void CLIClient::display_power_devices() const
{
  std::cout << "Current Microgrid Controller: " << curr_controller_ << std::endl;
  std::cout << "Number of Connected Power Devices: " << power_devices_.size() << std::endl;
  for (auto it = power_devices_.begin(); it != power_devices_.end(); ++it) {
    std::cout << "Device Id: " << it->first <<
      ". Type: " << device_role_to_string(it->second.role()) << std::endl;
  }
}

void CLIClient::display_controllers() const
{
  std::cout << "Connected Microgrid Controllers:" << std::endl;
  for (auto it = controllers_.begin(); it != controllers_.end(); ++it) {
    std::cout << "Controller Id: " << *it << std::endl;
  }
}

void CLIClient::set_controller(const OpArgPair& op_arg)
{
  const auto& arg = op_arg.second;
  if (!arg.has_value()) {
    std::cout << "No microgrid controller specified!" << std::endl;
    return;
  }

  curr_controller_ = op_arg.second.value();
  std::cout << "Current microgrid controller set to: " << curr_controller_ << std::endl;
}

void CLIClient::send_power_devices_request()
{
  if (curr_controller_.empty()) {
    std::cout << "Microgrid controller has not been set!" << std::endl;
    return;
  }

  cli::PowerDevicesRequest pd_req;
  pd_req.mc_id(curr_controller_);

  DDS::ReturnCode_t rc = pdreq_dw_->write(pd_req, DDS::HANDLE_NIL);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: CLIClient::send_power_devices_request: "
               "write to controller \"%C\" failed\n", curr_controller_.c_str()));
    return;
  }

  DDS::StringSeq params;
  params.length(1);
  params[0] = curr_controller_.c_str();
  DDS::QueryCondition_var qc = pdrep_dr_->create_querycondition(DDS::NOT_READ_SAMPLE_STATE,
                                                                DDS::ANY_VIEW_STATE,
                                                                DDS::ANY_INSTANCE_STATE,
                                                                "mc_id = %0",
                                                                params);
  if (!qc) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: CLIClient::send_power_devices_request: "
               "create_querycondition to receive from controller \"%C\" failed\n",
               curr_controller_.c_str()));
    return;
  }

  cli::PowerDevicesReplySeq data;
  DDS::SampleInfoSeq info_seq;
  rc = pdrep_dr_->take(data, info_seq, DDS::LENGTH_UNLIMITED,
                       DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: CLIClient::send_power_devices_request: "
               "take data failed: %C\n", OpenDDS::DCPS::retcode_to_string(rc)));
    return;
  }

  bool received = false;
  for (CORBA::ULong i = 0; i < data.length(); ++i) {
    if (data[i].mc_id() != curr_controller_) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: CLIClient::send_power_devices_request: "
                 "reply expected from controller \"%C\". Received from \"%C\"\n",
                 curr_controller_.c_str(), data[i].mc_id().c_str()));
      continue;
    }

    if (info_seq[i].valid_data) {
      const cli::DeviceInfoSeq& power_devices = data[i].devices();
      for (auto it = power_devices.begin(); it != power_devices.end(); ++it) {
        power_devices_.insert(std::make_pair(it->deviceId(), *it));
      }
      received = true;
      break;
    } else if (info_seq[i].instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
      power_devices_.clear();
      received = true;
      break;
    }
  }

  if (!received) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: CLIClient::send_power_devices_request: "
               "Failed to receive data from current controller\n"));
  }
}

void CLIClient::send_start_device_cmd(const OpArgPair& op_arg) const
{
  // TODO:
}

void CLIClient::send_stop_device_cmd(const OpArgPair& op_arg) const
{
  // TODO:
}

void CLIClient::send_stop_controller_cmd() const
{
  // TODO:
}

void CLIClient::send_resume_controller_cmd() const
{
  // TODO:
}

void CLIClient::send_terminate_cmd() const
{
  // TODO:
}

void CLIClient::process_device_info(const tms::DeviceInfo& di, const DDS::SampleInfo& si)
{
  if (si.valid_data) {
    if (di.role() == tms::DeviceRole::ROLE_MICROGRID_CONTROLLER) {
      controllers_.insert(di.deviceId());
    }
  } else if (si.instance_state & DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE){
    controllers_.erase(di.deviceId());
  }
}

void CLIClient::process_heartbeat(const tms::Heartbeat& hb, const DDS::SampleInfo& si)
{
  // TODO: Use heartbeat to detect and delete inactive controller?
}
