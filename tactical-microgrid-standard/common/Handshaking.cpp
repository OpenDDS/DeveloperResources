#include "Handshaking.h"
#include "DeviceInfoDataReaderListenerImpl.h"
#include "HeartbeatDataReaderListenerImpl.h"

#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/Marked_Default_Qos.h>

#include <thread>

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
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Handshaking::join_domain: create_participant failed\n"));
    return DDS::RETCODE_ERROR;
  }

  // Create a topic for the DeviceInfo type
  tms::DeviceInfoTypeSupport_var di_ts = new tms::DeviceInfoTypeSupportImpl();
  if (DDS::RETCODE_OK != di_ts->register_type(participant_.in(), "")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Handshaking::join_domain: register_type for DeviceInfo failed\n"));
    return DDS::RETCODE_ERROR;
  }

  CORBA::String_var di_type_name = di_ts->get_type_name();
  di_topic_ = participant_->create_topic(tms::topic::TOPIC_DEVICE_INFO.c_str(),
                                         di_type_name.in(),
                                         TOPIC_QOS_DEFAULT,
                                         DDS::TopicListener::_nil(),
                                         ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!di_topic_) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Handshaking::join_domain: create topic '%C' failed\n",
               tms::topic::TOPIC_DEVICE_INFO.c_str()));
    return DDS::RETCODE_ERROR;
  }

  // and another topic for the Heartbeat type
  tms::HeartbeatTypeSupport_var hb_ts = new tms::HeartbeatTypeSupportImpl();
  if (DDS::RETCODE_OK != hb_ts->register_type(participant_.in(), "")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Handshaking::join_domain: register_type for Heartbeat failed\n"));
    return DDS::RETCODE_ERROR;
  }

  CORBA::String_var hb_type_name = hb_ts->get_type_name();
  hb_topic_ = participant_->create_topic(tms::topic::TOPIC_HEARTBEAT.c_str(),
                                         hb_type_name.in(),
                                         TOPIC_QOS_DEFAULT,
                                         DDS::TopicListener::_nil(),
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

  DDS::Publisher_var pub = participant_->create_publisher(PUBLISHER_QOS_DEFAULT,
                                                          DDS::PublisherListener::_nil(),
                                                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pub) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Handshaking::create_publishers: create_publisher failed\n"));
    return DDS::RETCODE_ERROR;
  }

  DDS::DataWriter_var di_dw_base = pub->create_datawriter(di_topic_.in(),
                                                          DATAWRITER_QOS_DEFAULT,
                                                          DDS::DataWriterListener::_nil(),
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

  DDS::DataWriter_var hb_dw_base = pub->create_datawriter(hb_topic_.in(),
                                                          DATAWRITER_QOS_DEFAULT,
                                                          DDS::DataWriterListener::_nil(),
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

  return di_dw_->write(device_info, instance_handle);
}

DDS::ReturnCode_t Handshaking::start_heartbeats(tms::Identity id)
{
  if (!hb_dw_) {
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: Handshaking::start_heartbeats: create data writers first with create_publishers!\n"));
    return DDS::RETCODE_ERROR;
  }

  const ACE_Time_Value period(1, 0);
  tms::Heartbeat hb;
  hb.deviceId(id);

  const DDS::InstanceHandle_t instance_handle = hb_dw_->register_instance(hb);
  if (instance_handle == DDS::HANDLE_NIL) {
    return DDS::RETCODE_ERROR;
  }

  std::thread thr([&]() {
    while (!stop_) {
      hb.sequenceNumber(seq_num_++);
      const DDS::ReturnCode_t rc = hb_dw_->write(hb, instance_handle);
      if (rc != DDS::RETCODE_OK) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: Handshaking::send_heartbeats: write Heartbeat failed\n"));
      }
      ACE_OS::sleep(period);
    }
  });

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t Handshaking::create_subscribers(
  std::function<void(const tms::DeviceInfo&, const DDS::SampleInfo&)> di_cb,
  std::function<void(const tms::Heartbeat&, const DDS::SampleInfo&)> hb_cb)
{
  if (!di_topic_ || !hb_topic_) {
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: Handshaking::create_subscribers: create topics first with join_domain!\n"));
    return DDS::RETCODE_ERROR;
  }

  DDS::Subscriber_var sub = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                                            DDS::SubscriberListener::_nil(),
                                                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!sub) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Handshaking::create_subscribers: create_subscriber failed\n"));
    return DDS::RETCODE_OK;
  }

  DDS::DataReaderListener_var di_listener(new DeviceInfoDataReaderListenerImpl(di_cb));
  DDS::DataReaderListener_var hb_listener(new HeartbeatDataReaderListenerImpl(hb_cb));

  DDS::DataReader_var di_dr = sub->create_datareader(di_topic_.in(),
                                                     DATAREADER_QOS_DEFAULT,
                                                     di_listener.in(),
                                                     ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!di_dr) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Handshaking::create_subscribers: create_datareader for topic '%C' failed\n",
               tms::topic::TOPIC_DEVICE_INFO.c_str()));
    return DDS::RETCODE_ERROR;
  }

  DDS::DataReader_var hb_dr = sub->create_datareader(hb_topic_.in(),
                                                     DATAREADER_QOS_DEFAULT,
                                                     hb_listener.in(),
                                                     ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!hb_dr) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Handshaking::create_subscribers: create_datareader for topic '%C' failed\n",
               tms::topic::TOPIC_HEARTBEAT.c_str()));
    return DDS::RETCODE_ERROR;
  }

  return DDS::RETCODE_OK;
}
