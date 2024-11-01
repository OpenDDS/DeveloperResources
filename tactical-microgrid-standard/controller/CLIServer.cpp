#include "CLIServer.h"
#include "qos/QosHelper.h"
#include "PowerDevicesRequestDataReaderListenerImpl.h"
#include "OperatorIntentRequestDataReaderListenerImpl.h"

#include <dds/DCPS/Marked_Default_Qos.h>

// Assume the controller is active (i.e. actively sending heartbeats)
CLIServer::CLIServer(Controller& mc)
  : controller_(mc)
  , controller_active_(true)
{
  init();
}

DDS::ReturnCode_t CLIServer::init()
{
  // Optional writer for OperatorIntentState topic
  DDS::DomainParticipant_var dp = controller_.get_domain_participant();

  // Subscribe to the PowerDevicesRequest topic
  cli::PowerDevicesRequestTypeSupport_var pdreq_ts = new cli::PowerDevicesRequestTypeSupportImpl;
  if (DDS::RETCODE_OK != pdreq_ts->register_type(dp, "")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: register_type PowerDevicesRequest failed\n"));
    return DDS::RETCODE_ERROR;
  }

  CORBA::String_var pdreq_type_name = pdreq_ts->get_type_name();
  DDS::Topic_var pdreq_topic = dp->create_topic(cli::TOPIC_POWER_DEVICES_REQUEST.c_str(),
                                                pdreq_type_name.in(),
                                                TOPIC_QOS_DEFAULT,
                                                0,
                                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pdreq_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: create_topic \"%C\" failed\n",
               cli::TOPIC_POWER_DEVICES_REQUEST.c_str()));
    return DDS::RETCODE_ERROR;
  }

  DDS::Subscriber_var sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                                  0,
                                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!sub) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: create_subscriber failed\n"));
    return DDS::RETCODE_ERROR;
  }

  DDS::DataReaderQos dr_qos;
  sub->get_default_datareader_qos(dr_qos);
  dr_qos.reliability.kind = DDS::ReliabilityQosPolicyKind::RELIABLE_RELIABILITY_QOS;

  DDS::DataReaderListener_var pdreq_listener(new PowerDevicesRequestDataReaderListenerImpl(*this));
  DDS::DataReader_var pdreq_dr_base = sub->create_datareader(pdreq_topic,
                                                             dr_qos,
                                                             pdreq_listener,
                                                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pdreq_dr_base) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: create_datareader for topic \"%C\" failed\n",
               cli::TOPIC_POWER_DEVICES_REQUEST.c_str()));
    return DDS::RETCODE_ERROR;
  }

  pdreq_dr_ = cli::PowerDevicesRequestDataReader::_narrow(pdreq_dr_base);
  if (!pdreq_dr_) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: PowerDevicesRequestDataReader narrow failed\n"));
    return DDS::RETCODE_ERROR;
  }

  // Subscribe to the OperatorIntentRequest topic
  tms::OperatorIntentRequestTypeSupport_var oir_ts = new tms::OperatorIntentRequestTypeSupportImpl;
  if (DDS::RETCODE_OK != oir_ts->register_type(dp, "")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: register_type OperatorIntentRequest failed\n"));
    return DDS::RETCODE_ERROR;
  }

  CORBA::String_var oir_type_name = oir_ts->get_type_name();
  DDS::Topic_var oir_topic = dp->create_topic(tms::topic::TOPIC_OPERATOR_INTENT_REQUEST.c_str(),
                                              oir_type_name,
                                              TOPIC_QOS_DEFAULT,
                                              0,
                                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!oir_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: create_topic \"%C\" failed\n",
               tms::topic::TOPIC_OPERATOR_INTENT_REQUEST.c_str()));
    return DDS::RETCODE_ERROR;
  }

  const DDS::SubscriberQos tms_sub_qos = Qos::Subscriber::get_qos();
  DDS::Subscriber_var tms_sub = dp->create_subscriber(tms_sub_qos,
                                                      0,
                                                      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!tms_sub) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: create_subscriber with TMS QoS failed\n"));
    return DDS::RETCODE_ERROR;
  }

  const DDS::DataReaderQos oir_qos = Qos::DataReader::fn_map.at(tms::topic::TOPIC_OPERATOR_INTENT_REQUEST)(controller_.get_device_id());

  DDS::DataReaderListener_var oir_listener(new OperatorIntentRequestDataReaderListenerImpl);
  DDS::DataReader_var oir_dr_base = tms_sub->create_datareader(oir_topic,
                                                               oir_qos,
                                                               oir_listener,
                                                               ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!oir_dr_base) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: create_datareader for topic \"%C\" failed\n",
               tms::topic::TOPIC_OPERATOR_INTENT_REQUEST.c_str()));
    return DDS::RETCODE_ERROR;
  }

  oir_dr_ = tms::OperatorIntentRequestDataReader::_narrow(oir_dr_base);
  if (!oir_dr_) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: OperatorIntentRequestDataReader narrow failed\n"));
    return DDS::RETCODE_ERROR;
  }

  // Publish to PowerDevicesReply topic
  cli::PowerDevicesReplyTypeSupport_var pdrep_ts = new cli::PowerDevicesReplyTypeSupportImpl;
  if (DDS::RETCODE_OK != pdrep_ts->register_type(dp, "")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: register_type PowerDevicesReply failed\n"));
    return DDS::RETCODE_ERROR;
  }

  CORBA::String_var pdrep_type_name = pdrep_ts->get_type_name();
  DDS::Topic_var pdrep_topic = dp->create_topic(cli::TOPIC_POWER_DEVICES_REPLY.c_str(),
                                                pdrep_type_name,
                                                TOPIC_QOS_DEFAULT,
                                                0,
                                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pdrep_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: create_topic \"%C\" failed\n",
               cli::TOPIC_POWER_DEVICES_REPLY.c_str()));
    return DDS::RETCODE_ERROR;
  }

  DDS::Publisher_var pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                                                0,
                                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pub) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: create_publisher failed\n"));
    return DDS::RETCODE_ERROR;
  }

  DDS::DataWriterQos dw_qos;
  pub->get_default_datawriter_qos(dw_qos);
  dw_qos.reliability.kind = DDS::ReliabilityQosPolicyKind::RELIABLE_RELIABILITY_QOS;

  DDS::DataWriter_var pdrep_dw_base = pub->create_datawriter(pdrep_topic,
                                                             dw_qos,
                                                             0,
                                                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pdrep_dw_base) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: create_datawriter for topic \"%C\" failed\n",
               cli::TOPIC_POWER_DEVICES_REPLY.c_str()));
    return DDS::RETCODE_ERROR;
  }

  pdrep_dw_ = cli::PowerDevicesReplyDataWriter::_narrow(pdrep_dw_base);
  if (!pdrep_dw_) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: PowerDevicesReplyDataWriter narrow failed\n"));
    return DDS::RETCODE_ERROR;
  }

  return DDS::RETCODE_OK;
}

void CLIServer::start_device(const OpArgPair& oparg) const
{
  // TODO:
  if (!oparg.second.has_value()) {
    std::cerr << "Device Id is required to start a device!" << std::endl;
    return;
  }

  const std::string& device_id = oparg.second.value();
  std::cout << "Sending EnergyStartStopRequest to start device Id: " << device_id << std::endl;
}

void CLIServer::stop_device(const OpArgPair& oparg) const
{
  // TODO:
  if (!oparg.second.has_value()) {
    std::cerr << "Device Id is required to stop a device!" << std::endl;
    return;
  }

  const std::string& device_id = oparg.second.value();
  std::cout << "Sending EnergyStartStopRequest to stop device Id: " << device_id << std::endl;
}

void CLIServer::stop_controller()
{
  if (!controller_active_) {
    std::cout << "Controller is already stopped!" << std::endl;
    return;
  }

  controller_.stop_heartbeats();
  controller_active_ = false;
  std::cout << "Controller stopped!" << std::endl;
}

void CLIServer::resume_controller()
{
  if (controller_active_) {
    std::cout << "Controller is already running!" << std::endl;
    return;
  }

  const DDS::ReturnCode_t rc = controller_.start_heartbeats();
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: CLI::resume_controller: start heartbeats thread failed\n"));
    return;
  }
  controller_active_ = true;
  std::cout << "Controller resumed!" << std::endl;
}

void CLIServer::terminate() const
{
  // TODO: gracefully terminate the controller
  std::cout << "Terminating the controller..." << std::endl;
}
