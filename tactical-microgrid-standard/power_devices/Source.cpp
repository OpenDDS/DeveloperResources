#include "common/PowerDevice.h"
#include "common/DataReaderListenerBase.h"
#include "common/QosHelper.h"

#include <dds/DCPS/Marked_Default_Qos.h>

#include <ace/Get_Opt.h>

class SourceDevice;

class EnergyStartStopRequestDataReaderListenerImpl : public DataReaderListenerBase {
public:
  explicit EnergyStartStopRequestDataReaderListenerImpl(SourceDevice& src_dev) : src_dev_(src_dev) {}

  virtual ~EnergyStartStopRequestDataReaderListenerImpl() = default;

  void on_data_available(DDS::DataReader_ptr reader) final;

private:
  SourceDevice& src_dev_;
};

class SourceDevice : public PowerDevice {
public:
  explicit SourceDevice(const tms::Identity& id)
    : PowerDevice(id, tms::DeviceRole::ROLE_SOURCE)
  {
  }

  void energy_level(tms::EnergyStartStopLevel essl)
  {
    essl_ = essl;
  }

  DDS::ReturnCode_t init(DDS::DomainId_t domain_id, int argc = 0, char* argv[] = nullptr)
  {
    DDS::ReturnCode_t rc = PowerDevice::init(domain_id, argc, argv);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }

    DDS::DomainParticipant_var dp = get_domain_participant();

    // Subscribe to tms::EnergyStartStopRequest topic
    tms::EnergyStartStopRequestTypeSupport_var essr_ts = new tms::EnergyStartStopRequestTypeSupportImpl;
    if (DDS::RETCODE_OK != essr_ts->register_type(dp, "")) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: SourceDevice::init: register_type EnergyStartStopRequest failed\n"));
      return DDS::RETCODE_ERROR;
    }

    CORBA::String_var essr_type_name = essr_ts->get_type_name();
    DDS::Topic_var essr_topic = dp->create_topic(tms::topic::TOPIC_ENERGY_START_STOP_REQUEST.c_str(),
                                                 essr_type_name,
                                                 TOPIC_QOS_DEFAULT,
                                                 nullptr,
                                                 ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!essr_topic) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: SourceDevice::init: create_topic \"%C\" failed\n",
                 tms::topic::TOPIC_ENERGY_START_STOP_REQUEST.c_str()));
      return DDS::RETCODE_ERROR;
    }

    const DDS::SubscriberQos tms_sub_qos = Qos::Subscriber::get_qos();
    DDS::Subscriber_var tms_sub = dp->create_subscriber(tms_sub_qos,
                                                        nullptr,
                                                        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!tms_sub) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: SourceDevice::init: create_subscriber with TMS QoS failed\n"));
      return DDS::RETCODE_ERROR;
    }

    const DDS::DataReaderQos& essr_dr_qos = Qos::DataReader::fn_map.at(tms::topic::TOPIC_ENERGY_START_STOP_REQUEST)(device_id_);
    DDS::DataReaderListener_var essr_dr_listener(new EnergyStartStopRequestDataReaderListenerImpl(*this));
    DDS::DataReader_var essr_dr_base = tms_sub->create_datareader(essr_topic,
                                                                  essr_dr_qos,
                                                                  essr_dr_listener,
                                                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!essr_dr_base) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: SourceDevice::init: create_datareader for topic \"%C\" failed\n",
                 tms::topic::TOPIC_ENERGY_START_STOP_REQUEST.c_str()));
      return DDS::RETCODE_ERROR;
    }

    essr_dr_ = tms::EnergyStartStopRequestDataReader::_narrow(essr_dr_base);
    if (!essr_dr_) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: SourceDevice::init: EnergyStartStopRequestDataReader narrow failed\n"));
      return DDS::RETCODE_ERROR;
    }

    // Publish to tms::Reply topic
    tms::ReplyTypeSupport_var reply_ts = new tms::ReplyTypeSupportImpl;
    if (DDS::RETCODE_OK != reply_ts->register_type(dp, "")) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: SourceDevice::init: register_type tms::Reply failed\n"));
      return DDS::RETCODE_ERROR;
    }

    CORBA::String_var reply_type_name = reply_ts->get_type_name();
    DDS::Topic_var reply_topic = dp->create_topic(tms::topic::TOPIC_REPLY.c_str(),
                                                  reply_type_name,
                                                  TOPIC_QOS_DEFAULT,
                                                  nullptr,
                                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!reply_topic) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: SourceDevice::init: create_topic \"%C\" failed\n",
                 tms::topic::TOPIC_REPLY.c_str()));
      return DDS::RETCODE_ERROR;
    }

    const DDS::PublisherQos tms_pub_qos = Qos::Publisher::get_qos();
    DDS::Publisher_var tms_pub = dp->create_publisher(tms_pub_qos,
                                                      nullptr,
                                                      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!tms_pub) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: SourceDevice::init: create_publisher with TMS QoS failed\n"));
      return DDS::RETCODE_ERROR;
    }

    const DDS::DataWriterQos& reply_dw_qos = Qos::DataWriter::fn_map.at(tms::topic::TOPIC_REPLY)(device_id_);
    DDS::DataWriter_var reply_dw_base = tms_pub->create_datawriter(reply_topic,
                                                                   reply_dw_qos,
                                                                   nullptr,
                                                                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!reply_dw_base) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: SourceDevice::init: create_datawriter for topic \"%C\" failed\n",
                 tms::topic::TOPIC_REPLY.c_str()));
      return DDS::RETCODE_ERROR;
    }

    reply_dw_ = tms::ReplyDataWriter::_narrow(reply_dw_base);
    if (!reply_dw_) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: SourceDevice::init: ReplyDataWriter narrow failed\n"));
      return DDS::RETCODE_ERROR;
    }

    return DDS::RETCODE_OK;
  }

  int run()
  {
    // TODO(sonndinh): All devices including the controller may share this function
    return reactor_->run_reactor_event_loop() == 0 ? 0 : 1;
  }

  tms::ReplyDataWriter_var reply_dw() const
  {
    return reply_dw_;
  }

private:
  bool running_ = true;
  tms::EnergyStartStopLevel essl_ = tms::EnergyStartStopLevel::ESSL_OPERATIONAL;

  tms::EnergyStartStopRequestDataReader_var essr_dr_;
  tms::ReplyDataWriter_var reply_dw_;
};

void EnergyStartStopRequestDataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  tms::EnergyStartStopRequestSeq data;
  DDS::SampleInfoSeq info_seq;
  tms::EnergyStartStopRequestDataReader_var typed_reader = tms::EnergyStartStopRequestDataReader::_narrow(reader);
  DDS::ReturnCode_t rc = typed_reader->take(data, info_seq, DDS::LENGTH_UNLIMITED,
                                            DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: EnergyStartStopRequestDataReaderListenerImpl::on_data_available: "
               "take data failed: %C\n", OpenDDS::DCPS::retcode_to_string(rc)));
    return;
  }

  for (CORBA::ULong i = 0; i < data.length(); ++i) {
    if (info_seq[i].valid_data) {
      const tms::EnergyStartStopRequest& essr = data[i];
      const tms::Identity& sending_mc_id = essr.requestId().requestingDeviceId();
      if (sending_mc_id != src_dev_.selected()) {
        // Ignore request from non-selected controller
        continue;
      }

      const tms::Identity& target_id = essr.requestId().targetDeviceId();
      if (target_id == src_dev_.get_device_id()) {
        // Always set to the requested level and send an OK reply
        const tms::EnergyStartStopLevel essl = essr.toLevel();
        src_dev_.energy_level(essl);

        tms::Reply reply;
        reply.requestingDeviceId() = sending_mc_id;
        reply.targetDeviceId() = src_dev_.get_device_id();
        reply.config() = essr.requestId().config();
        reply.portNumber() = tms::INVALID_PORT_NUMBER;
        reply.requestSequenceId() = essr.sequenceId();
        reply.status().code() = tms::ReplyCode::REPLY_OK;
        reply.status().reason() = "OK";
        const DDS::ReturnCode_t rc = src_dev_.reply_dw()->write(reply, DDS::HANDLE_NIL);
        if (rc != DDS::RETCODE_OK) {
          ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: EnergyStartStopRequestDataReaderListenerImpl::on_data_available: "
                     "write reply failed: %C\n", OpenDDS::DCPS::retcode_to_string(rc)));
        }
        break;
      }
    }
  }
}

int main(int argc, char *argv[])
{
  DDS::DomainId_t domain_id = OpenDDS::DOMAIN_UNKNOWN;
  const char *src_id = nullptr;

  ACE_Get_Opt get_opt(argc, argv, "i:d:");
  int c;
  while ((c = get_opt()) != -1) {
    switch (c) {
    case 'i':
      src_id = get_opt.opt_arg();
      break;
    case 'd':
      domain_id = static_cast<DDS::DomainId_t>(ACE_OS::atoi(get_opt.opt_arg()));
      break;
    default:
      break;
    }
  }

  if (domain_id == OpenDDS::DOMAIN_UNKNOWN || src_id == nullptr) {
    ACE_ERROR((LM_ERROR, "Usage: %C -d DDS_Domain_Id -i Source_Device_Id\n", argv[0]));
    return 1;
  }

  SourceDevice src_dev(src_id);
  src_dev.init(domain_id, argc, argv);
  return src_dev.run();
}
