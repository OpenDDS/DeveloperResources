#include "CLIServer.h"
#include "PowerDevicesRequestDataReaderListenerImpl.h"
#include "OperatorIntentRequestDataReaderListenerImpl.h"
#include "ControllerCommandDataReaderListenerImpl.h"
#include "ReplyDataReaderListenerImpl.h"
#include "PowerTopologyDataReaderListenerImpl.h"
#include "common/QosHelper.h"
#include "common/Utils.h"

#include <dds/DCPS/Marked_Default_Qos.h>

CLIServer::CLIServer(Controller& mc)
  : controller_(mc)
{
  init();
}

DDS::ReturnCode_t CLIServer::init()
{
  const DDS::ReturnCode_t rc = init_tms();
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  return init_sim();
}

DDS::ReturnCode_t CLIServer::init_tms()
{
  DDS::DomainParticipant_var dp = controller_.get_domain_participant();

  // Subscribe to the tms::OperatorIntentRequest topic
  tms::OperatorIntentRequestTypeSupport_var oir_ts = new tms::OperatorIntentRequestTypeSupportImpl;
  if (DDS::RETCODE_OK != oir_ts->register_type(dp, "")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: register_type OperatorIntentRequest failed\n"));
    return DDS::RETCODE_ERROR;
  }

  CORBA::String_var oir_type_name = oir_ts->get_type_name();
  DDS::Topic_var oir_topic = dp->create_topic(tms::topic::TOPIC_OPERATOR_INTENT_REQUEST.c_str(),
                                              oir_type_name,
                                              TOPIC_QOS_DEFAULT,
                                              nullptr,
                                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!oir_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: create_topic \"%C\" failed\n",
               tms::topic::TOPIC_OPERATOR_INTENT_REQUEST.c_str()));
    return DDS::RETCODE_ERROR;
  }

  const DDS::SubscriberQos tms_sub_qos = Qos::Subscriber::get_qos();
  DDS::Subscriber_var tms_sub = dp->create_subscriber(tms_sub_qos,
                                                      nullptr,
                                                      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!tms_sub) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: create_subscriber with TMS QoS failed\n"));
    return DDS::RETCODE_ERROR;
  }

  const DDS::DataReaderQos& oir_qos = Qos::DataReader::fn_map.at(tms::topic::TOPIC_OPERATOR_INTENT_REQUEST)(controller_.get_device_id());

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

  // Publish to the tms::EnergyStartStopRequest topic
  tms::EnergyStartStopRequestTypeSupport_var essr_ts = new tms::EnergyStartStopRequestTypeSupportImpl;
  if (DDS::RETCODE_OK != essr_ts->register_type(dp, "")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: register_type tms::EnergyStartStopRequest failed\n"));
    return DDS::RETCODE_ERROR;
  }

  CORBA::String_var essr_type_name = essr_ts->get_type_name();
  DDS::Topic_var essr_topic = dp->create_topic(tms::topic::TOPIC_ENERGY_START_STOP_REQUEST.c_str(),
                                               essr_type_name,
                                               TOPIC_QOS_DEFAULT,
                                               nullptr,
                                               ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!essr_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: create_topic \"%C\" failed\n",
               tms::topic::TOPIC_ENERGY_START_STOP_REQUEST.c_str()));
    return DDS::RETCODE_ERROR;
  }

  const DDS::PublisherQos tms_pub_qos = Qos::Publisher::get_qos();
  DDS::Publisher_var tms_pub = dp->create_publisher(tms_pub_qos,
                                                    nullptr,
                                                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!tms_pub) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: create_publisher with TMS QoS failed\n"));
    return DDS::RETCODE_ERROR;
  }

  const DDS::DataWriterQos& essr_dw_qos = Qos::DataWriter::fn_map.at(tms::topic::TOPIC_ENERGY_START_STOP_REQUEST)(controller_.get_device_id());
  DDS::DataWriter_var essr_dw_base = tms_pub->create_datawriter(essr_topic,
                                                                essr_dw_qos,
                                                                nullptr,
                                                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!essr_dw_base) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: create_datawriter for topic \"%C\" failed\n",
               tms::topic::TOPIC_ENERGY_START_STOP_REQUEST.c_str()));
    return DDS::RETCODE_ERROR;
  }

  essr_dw_ = tms::EnergyStartStopRequestDataWriter::_narrow(essr_dw_base);
  if (!essr_dw_) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: EnergyStartStopRequestDataWriter narrow failed\n"));
    return DDS::RETCODE_ERROR;
  }

  // Subscribe to the tms::Reply topic
  tms::ReplyTypeSupport_var reply_ts = new tms::ReplyTypeSupportImpl;
  if (DDS::RETCODE_OK != reply_ts->register_type(dp, "")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: register_type tms::Reply failed\n"));
    return DDS::RETCODE_ERROR;
  }

  CORBA::String_var reply_type_name = reply_ts->get_type_name();
  DDS::Topic_var reply_topic = dp->create_topic(tms::topic::TOPIC_REPLY.c_str(),
                                                reply_type_name,
                                                TOPIC_QOS_DEFAULT,
                                                nullptr,
                                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!reply_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: create_topic \"%C\" failed\n",
               tms::topic::TOPIC_REPLY.c_str()));
    return DDS::RETCODE_ERROR;
  }

  const DDS::DataReaderQos& reply_qos = Qos::DataReader::fn_map.at(tms::topic::TOPIC_REPLY)(controller_.get_device_id());
  DDS::DataReaderListener_var reply_listener(new ReplyDataReaderListenerImpl(*this));
  DDS::DataReader_var reply_dr_base = tms_sub->create_datareader(reply_topic,
                                                                 reply_qos,
                                                                 reply_listener,
                                                                 ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!reply_dr_base) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: create_datareader for topic \"%C\" failed\n",
               tms::topic::TOPIC_REPLY.c_str()));
    return DDS::RETCODE_ERROR;
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t CLIServer::init_sim()
{
  const DDS::DomainId_t sim_domain_id = Utils::get_sim_domain_id(controller_.tms_domain_id());

  DDS::DomainParticipantFactory_var dpf = controller_.get_participant_factory();
  sim_participant_ = dpf->create_participant(sim_domain_id,
                                             PARTICIPANT_QOS_DEFAULT,
                                             nullptr,
                                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!sim_participant_) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: create simulation participant failed\n"));
    return DDS::RETCODE_ERROR;
  }

  // Subscribe to the cli::PowerDevicesRequest topic
  cli::PowerDevicesRequestTypeSupport_var pdreq_ts = new cli::PowerDevicesRequestTypeSupportImpl;
  if (DDS::RETCODE_OK != pdreq_ts->register_type(sim_participant_, "")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: register_type PowerDevicesRequest failed\n"));
    return DDS::RETCODE_ERROR;
  }

  CORBA::String_var pdreq_type_name = pdreq_ts->get_type_name();
  DDS::Topic_var pdreq_topic = sim_participant_->create_topic(cli::TOPIC_POWER_DEVICES_REQUEST.c_str(),
                                                              pdreq_type_name,
                                                              TOPIC_QOS_DEFAULT,
                                                              nullptr,
                                                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pdreq_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: create_topic \"%C\" failed\n",
               cli::TOPIC_POWER_DEVICES_REQUEST.c_str()));
    return DDS::RETCODE_ERROR;
  }

  DDS::Subscriber_var sub = sim_participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                                                nullptr,
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

  // Subscribe to the cli::ControllerCommand topic
  cli::ControllerCommandTypeSupport_var cc_ts = new cli::ControllerCommandTypeSupportImpl;
  if (DDS::RETCODE_OK != cc_ts->register_type(sim_participant_, "")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: register_type ControllerCommand failed\n"));
    return DDS::RETCODE_ERROR;
  }

  CORBA::String_var cc_type_name = cc_ts->get_type_name();
  DDS::Topic_var cc_topic = sim_participant_->create_topic(cli::TOPIC_CONTROLLER_COMMAND.c_str(),
                                                           cc_type_name,
                                                           TOPIC_QOS_DEFAULT,
                                                           nullptr,
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

  // Publish to cli::PowerDevicesReply topic
  cli::PowerDevicesReplyTypeSupport_var pdrep_ts = new cli::PowerDevicesReplyTypeSupportImpl;
  if (DDS::RETCODE_OK != pdrep_ts->register_type(sim_participant_, "")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: register_type PowerDevicesReply failed\n"));
    return DDS::RETCODE_ERROR;
  }

  CORBA::String_var pdrep_type_name = pdrep_ts->get_type_name();
  DDS::Topic_var pdrep_topic = sim_participant_->create_topic(cli::TOPIC_POWER_DEVICES_REPLY.c_str(),
                                                              pdrep_type_name,
                                                              TOPIC_QOS_DEFAULT,
                                                              nullptr,
                                                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pdrep_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: create_topic \"%C\" failed\n",
               cli::TOPIC_POWER_DEVICES_REPLY.c_str()));
    return DDS::RETCODE_ERROR;
  }

  DDS::Publisher_var pub = sim_participant_->create_publisher(PUBLISHER_QOS_DEFAULT,
                                                              nullptr,
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
                                                             nullptr,
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

  // Subscibe to the powersim::PowerTopology topic
  powersim::PowerTopologyTypeSupport_var pt_ts = new powersim::PowerTopologyTypeSupportImpl;
  if (DDS::RETCODE_OK != pt_ts->register_type(sim_participant_, "")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: register_type PowerTopology failed\n"));
    return DDS::RETCODE_ERROR;
  }

  CORBA::String_var pt_type_name = pt_ts->get_type_name();
  DDS::Topic_var pt_topic = sim_participant_->create_topic(powersim::TOPIC_POWER_TOPOLOGY.c_str(),
                                                           pt_type_name,
                                                           TOPIC_QOS_DEFAULT,
                                                           nullptr,
                                                           ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pt_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: create_topic \"%C\" failed\n",
               powersim::TOPIC_POWER_TOPOLOGY.c_str()));
    return DDS::RETCODE_ERROR;
  }

  DDS::DataReaderListener_var pt_listener(new PowerTopologyDataReaderListenerImpl(*this));
  DDS::DataReader_var pt_dr_base = sub->create_datareader(pt_topic,
                                                          dr_qos,
                                                          pt_listener,
                                                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pt_dr_base) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: create_datareader for topic \"%C\" failed\n",
               powersim::TOPIC_POWER_TOPOLOGY.c_str()));
    return DDS::RETCODE_ERROR;
  }

  // Publish to the powersim::PowerConnection topic
  powersim::PowerConnectionTypeSupport_var pc_ts = new powersim::PowerConnectionTypeSupportImpl;
  if (DDS::RETCODE_OK != pc_ts->register_type(sim_participant_, "")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: register_type PowerConnection failed\n"));
    return DDS::RETCODE_ERROR;
  }

  CORBA::String_var pc_type_name = pc_ts->get_type_name();
  DDS::Topic_var pc_topic = sim_participant_->create_topic(powersim::TOPIC_POWER_CONNECTION.c_str(),
                                                           pc_type_name,
                                                           TOPIC_QOS_DEFAULT,
                                                           nullptr,
                                                           ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pc_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: create_topic \"%C\" failed\n",
               powersim::TOPIC_POWER_CONNECTION.c_str()));
    return DDS::RETCODE_ERROR;
  }

  DDS::DataWriter_var pc_dw_base = pub->create_datawriter(pc_topic,
                                                          dw_qos,
                                                          nullptr,
                                                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pc_dw_base) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: create_datawriter for topic \"%C\" failed\n",
               powersim::TOPIC_POWER_CONNECTION.c_str()));
    return DDS::RETCODE_ERROR;
  }

  pc_dw_ = powersim::PowerConnectionDataWriter::_narrow(pc_dw_base);
  if (!pc_dw_) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIServer::init: PowerConnectionDataWriter narrow failed\n"));
    return DDS::RETCODE_ERROR;
  }

  return DDS::RETCODE_OK;
}

tms::EnergyStartStopLevel CLIServer::ESSL_from_OPT(tms::OperatorPriorityType opt)
{
  switch (opt) {
  case tms::OperatorPriorityType::OPT_ALWAYS_OPERATE:
    return tms::EnergyStartStopLevel::ESSL_OPERATIONAL;
  case tms::OperatorPriorityType::OPT_NEVER_OPERATE:
    return tms::EnergyStartStopLevel::ESSL_OFF;
  default:
    return tms::EnergyStartStopLevel::ESSL_UNKNOWN;
  }
}

void CLIServer::start_stop_device(const tms::Identity& pd_id, tms::OperatorPriorityType opt)
{
  PowerDevices pdvs = controller_.power_devices();
  auto it = pdvs.find(pd_id);
  if (it == pdvs.end()) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: CLIServer::start_stop_device: power device \"%C\" not found\n", pd_id.c_str()));
    return;
  }

  const tms::EnergyStartStopLevel to_essl = ESSL_from_OPT(opt);
  if (to_essl == tms::EnergyStartStopLevel::ESSL_UNKNOWN) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: CLIServer::start_stop_device: unknown EnergyStartStopLevel\n"));
    return;
  }

  const tms::EnergyStartStopLevel curr_essl = it->second.essl();
  if (curr_essl == to_essl) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) INFO: CLIServer::start_stop_device: device \"%C\" already in requested state\n", pd_id.c_str()));
    return;
  }

  tms::EnergyStartStopRequest essr;
  essr.requestId().requestingDeviceId(controller_.id());
  essr.requestId().targetDeviceId(pd_id);
  essr.requestId().config() = tms::ConfigId::CONFIG_ACTIVE;
  essr.sequenceId() = essr_seqnum_++;
  essr.fromLevel() = tms::EnergyStartStopLevel::ESSL_ANY;
  essr.toLevel() = to_essl;

  const DDS::ReturnCode_t rc = essr_dw_->write(essr, DDS::HANDLE_NIL);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: CLIServer::start_stop_device: write failed: %C\n",
               OpenDDS::DCPS::retcode_to_string(rc)));
    return;
  }

  controller_.update_essl(pd_id, to_essl);
  {
    std::lock_guard<std::mutex> guard(pending_essr_m_);
    pending_essr_.insert(std::make_pair(essr.sequenceId(), pd_id));
  }
}

std::string CLIServer::replycode_to_string(tms::ReplyCode code)
{
  switch (code) {
  case tms::ReplyCode::REPLY_OK:
    return "OK";
  case tms::ReplyCode::REPLY_BAD_REQUEST:
    return "Bad Request";
  case tms::ReplyCode::REPLY_METHOD_NOT_ALLOWED:
    return "Method Not Allowed";
  case tms::ReplyCode::REPLY_CONFLICT:
    return "Conflict";
  case tms::ReplyCode::REPLY_GONE:
    return "Gone";
  case tms::ReplyCode::REPLY_PRECONDITION_FAILED:
    return "Precondition Failed";
  case tms::ReplyCode::REPLY_INTERNAL_SERVER_ERROR:
    return "Internal Server Error";
  case tms::ReplyCode::REPLY_NOT_IMPLEMENTED:
    return "Not Implemented";
  case tms::ReplyCode::REPLY_SERVICE_UNAVAILABLE:
    return "Service Unavailable";
  case tms::ReplyCode::REPLY_PENDING_AUTHORIZATION:
    return "Pending Authorization";
  case tms::ReplyCode::REPLY_NOT_MASTER:
    return "Not Master";
  case tms::ReplyCode::REPLY_PERMISSION_DENIED:
    return "Permission Denied";
  default:
    return "Unknown";
  }
}

void CLIServer::receive_reply(const tms::Reply& reply)
{
  const tms::RequestSequence seqnum = reply.requestSequenceId();
  const tms::Identity& target_id = reply.targetDeviceId();
  const tms::ReplyStatus status = reply.status();
  if (OpenDDS::DCPS::DCPS_debug_level >= 5) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: ReplyDataReaderListenerImpl::on_data_available: "
               "reply from device id=\"%C\", status=%C\n",
               target_id.c_str(), replycode_to_string(status.code()).c_str()));
  }

  {
    // Regardless of the status, remove the entry for the pending request
    std::lock_guard<std::mutex> guard(pending_essr_m_);
    pending_essr_.erase(seqnum);
  }
}
