#include "CLIClient.h"
#include "PowerDevicesReplyDataReaderListenerImpl.h"
#include "qos/QosHelper.h"

#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/Marked_Default_Qos.h>

DDS::ReturnCode_t CLIClient::init(DDS::DomainId_t domain_id, int argc, char* argv[])
{
  if (argc != 0) {
    dpf_ = TheParticipantFactoryWithArgs(argc, argv);
  } else {
    dpf_ = TheParticipantFactory;
  }

  participant_ = dpf_->create_participant(domain_id,
                                          PARTICIPANT_QOS_DEFAULT,
                                          0,
                                          OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!participant_) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: create_participant failed\n"));
    return DDS::RETCODE_ERROR;
  }

  // Publish to the Power Devices Request topic
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
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: create_topic \"%C\" failed\n", cli::TOPIC_POWER_DEVICES_REQUEST.c_str()));
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
  dw_qos.reliability.kind = DDS::ReliabilityQosPolicyKind::RELIABLE_RELIABILITY_QOS;
  pub->get_default_datawriter_qos(dw_qos);

  DDS::DataWriter_var pdreq_dw_base = pub->create_datawriter(pdreq_topic,
                                                             dw_qos,
                                                             0,
                                                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pdreq_dw_base) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: create_datawriter for topic \"%C\" failed\n",
               cli::TOPIC_POWER_DEVICES_REQUEST.c_str()));
    return DDS::RETCODE_ERROR;
  }

  pdreq_dw_ = cli::PowerDevicesRequestDataWriter::_narrow(pdreq_dw_base.in());
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
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: create_topic \"%C\" failed\n", tms::topic::TOPIC_OPERATOR_INTENT_REQUEST.c_str()));
    return DDS::RETCODE_ERROR;
  }

  const DDS::PublisherQos tms_pub_qos = Qos::Publisher::get_qos();
  DDS::Publisher_var tms_pub = participant_->create_publisher(tms_pub_qos,
                                                              0,
                                                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!tms_pub) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: create publisher with TMS QoS failed\n"));
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

  oir_dw_ = tms::OperatorIntentRequestDataWriter::_narrow(oir_dw_base.in());
  if (!oir_dw_) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: OperatorIntentRequestDataWriter narrow failed\n"));
    return DDS::RETCODE_ERROR;
  }

  // Subscriber to the Power Devices Reply topic
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
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: create_topic \"%C\" failed\n", cli::TOPIC_POWER_DEVICES_REPLY.c_str()));
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

  DDS::DataReaderListener_var pdrep_listener(new PowerDevicesReplyDataReaderListenerImpl);
  DDS::DataReader_var pdrep_dr_base = sub->create_datareader(pdrep_topic,
                                                             dr_qos,
                                                             pdrep_listener,
                                                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pdrep_dr_base) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIClient::init: create_datareader for topic \"%C\" failed\n",
               cli::TOPIC_POWER_DEVICES_REPLY.c_str()));
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
    const auto op_pair = parse(line);
    const std::string& op = op_pair.first;
    if (op == "L" || op == "l") {
      // const auto pd = controller_.power_devices();
      // display_power_devices(pd);
      send_power_devices_request();
    } else if (op == "E" || op == "e") {
      //      start_device(op_pair);
      send_start_device_cmd(op_pair);
    } else if (op == "D" || op == "d") {
      //      stop_device(op_pair);
      send_stop_device_cmd(op_pair);
    } else if (op == "S" || op == "s") {
      //      stop_controller();
      send_stop_controller_cmd();
    } else if (op == "R" || op == "r") {
      //      resume_controller();
      send_resume_controller_cmd();
    } else if (op == "T" || op == "t") {
      //      terminate();
      send_terminate_cmd();
    } else {
      std::cout << "Unknown operation entered!\n" << std::endl;
    }
    display_commands();
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
  // TODO: If we implements a CLI client, the CLI client and server can use the following
  // topics to communicate the operation commands:
  // - Commands E, D can use the tms topic OperatorIntentRequest (B.11). In this case,
  //   the CLI client takes the role of a microgrid dashboard.
  // - Commands L, S, R, T needs a new topic since they are commands for the MC directly
  //   and OperatorIntentRequest topic does not support these commands.
  std::cout << "\n=== CLI for Microgrid Controller (MC): " /*<< controller_.id()*/ << std::endl;
  std::cout << "[L/l]ist connected power devices" << std::endl;
  std::cout << "[E/e]nable (start) a power device with Id" << std::endl;
  std::cout << "[D/d]isable (stop) a power device with Id" << std::endl;
  std::cout << "[S/s]top the MC's heartbeats" << std::endl;
  std::cout << "[R/r]esume the MC's heartbeats" << std::endl;
  std::cout << "[T/t]erminate the MC" << std::endl;
  std::cout << "Enter next operation: ";
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

void CLIClient::display_power_devices(const Controller::PowerDevices& pd) const
{
  std::cout << "Number of connected power devices: " << pd.size() << std::endl;
  for (auto it = pd.begin(); it != pd.end(); ++it) {
    std::cout << "Device Id: " << it->first <<
      ". Type: " << device_role_to_string(it->second.role()) << std::endl;
  }
}

void CLIClient::send_power_devices_request()
{
}

void CLIClient::send_start_device_cmd(const OpArgPair& op_arg)
{
}

void CLIClient::send_stop_device_cmd(const OpArgPair& op_arg)
{
}

void CLIClient::send_stop_controller_cmd()
{
}

void CLIClient::send_resume_controller_cmd()
{
}

void CLIClient::send_terminate_cmd()
{
}
