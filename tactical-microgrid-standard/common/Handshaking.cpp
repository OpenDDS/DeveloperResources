#include "Handshaking.h"
#include "DeviceInfoDataReaderListenerImpl.h"
#include "HeartbeatDataReaderListenerImpl.h"
#include "QosHelper.h"

#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportInst.h>
#include <dds/DCPS/StaticIncludes.h>

Handshaking::~Handshaking()
{
  if (participant_) {
    participant_->delete_contained_entities();
  }
  if (dpf_) {
    dpf_->delete_participant(participant_);
  }
}

DDS::ReturnCode_t Handshaking::join_domain(DDS::DomainId_t domain_id, int argc, char* argv[])
{
  TheServiceParticipant->set_default_discovery(OpenDDS::DCPS::Discovery::DEFAULT_RTPS);
  OpenDDS::DCPS::TransportConfig_rch transport_config =
    TheTransportRegistry->create_config("default_rtps_transport_config");
  OpenDDS::DCPS::TransportInst_rch transport_inst =
    TheTransportRegistry->create_inst("default_rtps_transport", "rtps_udp");
  transport_config->instances_.push_back(transport_inst);
  TheTransportRegistry->global_config(transport_config);

  if (argc != 0) {
    dpf_ = TheParticipantFactoryWithArgs(argc, argv);
  } else {
    dpf_ = TheParticipantFactory;
  }

  participant_ = dpf_->create_participant(domain_id,
                                          PARTICIPANT_QOS_DEFAULT,
                                          nullptr,
                                          OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!participant_) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Handshaking::join_domain: create_participant failed\n"));
    return DDS::RETCODE_ERROR;
  }

  // Create a topic for the DeviceInfo type
  tms::DeviceInfoTypeSupport_var di_ts = new tms::DeviceInfoTypeSupportImpl();
  DDS::ReturnCode_t rc = di_ts->register_type(participant_, "");
  if (DDS::RETCODE_OK != rc) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Handshaking::join_domain: register_type for DeviceInfo failed\n"));
    return rc;
  }

  CORBA::String_var di_type_name = di_ts->get_type_name();
  di_topic_ = participant_->create_topic(tms::topic::TOPIC_DEVICE_INFO.c_str(),
                                         di_type_name,
                                         TOPIC_QOS_DEFAULT,
                                         nullptr,
                                         ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!di_topic_) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Handshaking::join_domain: create topic '%C' failed\n",
               tms::topic::TOPIC_DEVICE_INFO.c_str()));
    return DDS::RETCODE_ERROR;
  }

  // and another topic for the Heartbeat type
  tms::HeartbeatTypeSupport_var hb_ts = new tms::HeartbeatTypeSupportImpl();
  rc = hb_ts->register_type(participant_, "");
  if (DDS::RETCODE_OK != rc) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Handshaking::join_domain: register_type for Heartbeat failed\n"));
    return rc;
  }

  CORBA::String_var hb_type_name = hb_ts->get_type_name();
  hb_topic_ = participant_->create_topic(tms::topic::TOPIC_HEARTBEAT.c_str(),
                                         hb_type_name,
                                         TOPIC_QOS_DEFAULT,
                                         nullptr,
                                         ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!hb_topic_) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Handshaking::join_domain: create topic '%C' failed\n",
               tms::topic::TOPIC_HEARTBEAT.c_str()));
    return DDS::RETCODE_ERROR;
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t Handshaking::create_publishers()
{
  if (!di_topic_ || !hb_topic_) {
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: Handshaking::create_publishers: create topics first with join_domain!\n"));
    return DDS::RETCODE_ERROR;
  }

  const DDS::PublisherQos pub_qos = Qos::Publisher::get_qos();
  DDS::Publisher_var pub = participant_->create_publisher(pub_qos,
                                                          nullptr,
                                                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pub) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Handshaking::create_publishers: create_publisher failed\n"));
    return DDS::RETCODE_ERROR;
  }

  const DDS::DataWriterQos& di_qos = Qos::DataWriter::fn_map.at(tms::topic::TOPIC_DEVICE_INFO)(device_id_);
  DDS::DataWriter_var di_dw_base = pub->create_datawriter(di_topic_,
                                                          di_qos,
                                                          nullptr,
                                                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!di_dw_base) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Handshaking::create_publishers: create_datawriter for topic '%C' failed\n",
               tms::topic::TOPIC_DEVICE_INFO.c_str()));
    return DDS::RETCODE_ERROR;
  }

  di_dw_ = tms::DeviceInfoDataWriter::_narrow(di_dw_base.in());
  if (!di_dw_) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Handshaking::create_publishers: DeviceInfoDataWriter could not be narrowed\n"));
    return DDS::RETCODE_ERROR;
  }

  const DDS::DataWriterQos& hb_qos = Qos::DataWriter::fn_map.at(tms::topic::TOPIC_HEARTBEAT)(device_id_);
  DDS::DataWriter_var hb_dw_base = pub->create_datawriter(hb_topic_,
                                                          hb_qos,
                                                          nullptr,
                                                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!hb_dw_base) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Handshaking::create_publishers: create_datawriter for topic '%C' failed\n",
               tms::topic::TOPIC_HEARTBEAT.c_str()));
    return DDS::RETCODE_ERROR;
  }

  hb_dw_ = tms::HeartbeatDataWriter::_narrow(hb_dw_base.in());
  if (!hb_dw_) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Handshaking::create_publishers: HeartbeatDataWriter could not be narrowed\n"));
    return DDS::RETCODE_ERROR;
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t Handshaking::send_device_info(tms::DeviceInfo device_info)
{
  if (!di_dw_) {
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: Handshaking::send_device_info: create data writers first with create_publishers!\n"));
    return DDS::RETCODE_ERROR;
  }

  const DDS::InstanceHandle_t instance_handle = di_dw_->register_instance(device_info);
  if (instance_handle == DDS::HANDLE_NIL) {
    return DDS::RETCODE_ERROR;
  }

  const DDS::ReturnCode_t rc = di_dw_->write(device_info, instance_handle);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  return start_heartbeats();
}

DDS::ReturnCode_t Handshaking::start_heartbeats()
{
  if (!hb_dw_) {
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: Handshaking::start_heartbeats: create data writers first with create_publishers!\n"));
    return DDS::RETCODE_ERROR;
  }

  if (!get_timer<tms::Heartbeat>()->active()) {
    tms::Heartbeat hb;
    hb.deviceId(device_id_);
    schedule(hb, heartbeat_period);
  }

  return DDS::RETCODE_OK;
}

void Handshaking::stop_heartbeats()
{
  if (get_timer<tms::Heartbeat>()->active()) {
    cancel<tms::Heartbeat>();
  }
}

DDS::ReturnCode_t Handshaking::create_subscribers(
  std::function<void(const tms::DeviceInfo&, const DDS::SampleInfo&)> di_cb,
  std::function<void(const tms::Heartbeat&, const DDS::SampleInfo&)> hb_cb)
{
  if (!di_topic_ || !hb_topic_) {
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: Handshaking::create_subscribers: create topics first with join_domain!\n"));
    return DDS::RETCODE_ERROR;
  }

  const DDS::SubscriberQos sub_qos = Qos::Subscriber::get_qos();
  DDS::Subscriber_var sub = participant_->create_subscriber(sub_qos,
                                                            nullptr,
                                                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!sub) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Handshaking::create_subscribers: create_subscriber failed\n"));
    return DDS::RETCODE_ERROR;
  }

  DDS::DataReaderListener_var di_listener(new DeviceInfoDataReaderListenerImpl(di_cb));
  const DDS::DataReaderQos& di_qos = Qos::DataReader::fn_map.at(tms::topic::TOPIC_DEVICE_INFO)(device_id_);
  DDS::DataReader_var di_dr = sub->create_datareader(di_topic_,
                                                     di_qos,
                                                     di_listener.in(),
                                                     ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!di_dr) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Handshaking::create_subscribers: create_datareader for topic '%C' failed\n",
               tms::topic::TOPIC_DEVICE_INFO.c_str()));
    return DDS::RETCODE_ERROR;
  }

  DDS::DataReaderListener_var hb_listener(new HeartbeatDataReaderListenerImpl(hb_cb));
  const DDS::DataReaderQos& hb_qos = Qos::DataReader::fn_map.at(tms::topic::TOPIC_HEARTBEAT)(device_id_);
  DDS::DataReader_var hb_dr = sub->create_datareader(hb_topic_,
                                                     hb_qos,
                                                     hb_listener.in(),
                                                     ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!hb_dr) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Handshaking::create_subscribers: create_datareader for topic '%C' failed\n",
               tms::topic::TOPIC_HEARTBEAT.c_str()));
    return DDS::RETCODE_ERROR;
  }

  return DDS::RETCODE_OK;
}

void Handshaking::timer_fired(Timer<tms::Heartbeat>& timer)
{
  timer.arg.sequenceNumber(seq_num_++);
  const DDS::ReturnCode_t rc = hb_dw_->write(timer.arg, DDS::HANDLE_NIL);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: Handshaking::send_heartbeats: write Heartbeat failed\n"));
  }
}
