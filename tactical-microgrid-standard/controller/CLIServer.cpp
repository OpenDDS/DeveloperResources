#include "CLIServer.h"
#include "qos/QosHelper.h"
#include "PowerDevicesRequestDataReaderListenerImpl.h"
#include "OperatorIntentRequestDataReaderListenerImpl.h"
#include "ControllerCommandDataReaderListenerImpl.h"

#include <dds/DCPS/Marked_Default_Qos.h>

CLIServer::CLIServer(Controller& mc)
  : controller_(mc)
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

  DDS::DataReaderListener_var oir_listener(new OperatorIntentRequestDataReaderListenerImpl(*this));
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

  // Subscribe to the ControllerCommand topic
  cli::ControllerCommandTypeSupport_var cc_ts = new cli::ControllerCommandTypeSupportImpl;
  if (DDS::RETCODE_OK != cc_ts->register_type(dp, "")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: register_type ControllerCommand failed\n"));
    return DDS::RETCODE_ERROR;
  }

  CORBA::String_var cc_type_name = cc_ts->get_type_name();
  DDS::Topic_var cc_topic = dp->create_topic(cli::TOPIC_CONTROLLER_COMMAND.c_str(),
                                             cc_type_name,
                                             TOPIC_QOS_DEFAULT,
                                             0,
                                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!cc_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: create_topic \"%C\" failed\n",
               cli::TOPIC_CONTROLLER_COMMAND.c_str()));
    return DDS::RETCODE_ERROR;
  }

  DDS::DataReaderListener_var cc_listener(new ControllerCommandDataReaderListenerImpl(*this));
  DDS::DataReader_var cc_dr_base = sub->create_datareader(cc_topic,
                                                          dr_qos,
                                                          cc_listener,
                                                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!cc_dr_base) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: create_datareader for topic \"%C\" failed\n",
               cli::TOPIC_CONTROLLER_COMMAND.c_str()));
    return DDS::RETCODE_ERROR;
  }

  cc_dr_ = cli::ControllerCommandDataReader::_narrow(cc_dr_base);
  if (!cc_dr_) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: ControllerCommandDataReader narrow failed\n"));
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

void CLIServer::start_stop_device(const tms::Identity& /*pd_id*/, tms::OperatorPriorityType /*opt*/)
{
  // TODO: Send EnergyStartStopRequest to the power device.
}
