#include "PowerDevice.h"
#include "PowerSimTypeSupportImpl.h"
#include "common/DataReaderListenerBase.h"
#include "common/QosHelper.h"
#include "common/Utils.h"

#include <dds/DCPS/Marked_Default_Qos.h>

#include <ace/Get_Opt.h>

#include <thread>
#include <chrono>

class SourceDevice;

class EnergyStartStopRequestDataReaderListenerImpl : public DataReaderListenerBase {
public:
  explicit EnergyStartStopRequestDataReaderListenerImpl(SourceDevice& src_dev)
    : DataReaderListenerBase("tms::EnergyStartStopRequest - DataReaderListenerImpl")
    , src_dev_(src_dev) {}

  virtual ~EnergyStartStopRequestDataReaderListenerImpl() = default;

  void on_data_available(DDS::DataReader_ptr reader) final;

private:
  SourceDevice& src_dev_;
};

class SourceDevice : public PowerDevice {
public:
  explicit SourceDevice(const tms::Identity& id, bool verbose = false)
    : PowerDevice(id, tms::DeviceRole::ROLE_SOURCE, verbose)
  {
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

    // Publish to powersim::ElectricCurrent topic
    powersim::ElectricCurrentTypeSupport_var ec_ts = new powersim::ElectricCurrentTypeSupportImpl;
    if (DDS::RETCODE_OK != ec_ts->register_type(sim_participant_, "")) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: SourceDevice::init: register_type ElectricCurrent failed\n"));
      return DDS::RETCODE_ERROR;
    }

    CORBA::String_var ec_type_name = ec_ts->get_type_name();
    DDS::Topic_var ec_topic = sim_participant_->create_topic(powersim::TOPIC_ELECTRIC_CURRENT.c_str(),
                                                             ec_type_name,
                                                             TOPIC_QOS_DEFAULT,
                                                             nullptr,
                                                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!ec_topic) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: SourceDevice::init: create_topic \"%C\" failed\n",
                 powersim::TOPIC_ELECTRIC_CURRENT.c_str()));
      return DDS::RETCODE_ERROR;
    }

    DDS::Publisher_var sim_pub = sim_participant_->create_publisher(PUBLISHER_QOS_DEFAULT,
                                                                    nullptr,
                                                                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!sim_pub) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: SourceDevice::init: create_publisher failed\n"));
      return DDS::RETCODE_ERROR;
    }

    DDS::DataWriter_var ec_dw_base = sim_pub->create_datawriter(ec_topic,
                                                                DATAWRITER_QOS_DEFAULT,
                                                                nullptr,
                                                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!ec_dw_base) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: SourceDevice::init: create_datawriter for topic \"%C\" failed\n",
                 powersim::TOPIC_ELECTRIC_CURRENT.c_str()));
      return DDS::RETCODE_ERROR;
    }

    ec_dw_ = powersim::ElectricCurrentDataWriter::_narrow(ec_dw_base);
    if (!ec_dw_) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: SourceDevice::init: ElectricCurrentDataWriter narrow failed\n"));
      return DDS::RETCODE_ERROR;
    }

    return DDS::RETCODE_OK;
  }

  bool shutdown() const
  {
    std::lock_guard<std::mutex> guard(shutdown_m_);
    return shutdown_;
  }

  int handle_signal(int, siginfo_t*, ucontext_t*) override
  {
    {
      std::lock_guard<std::mutex> guard(shutdown_m_);
      shutdown_ = true;
    }

    reactor_->end_reactor_event_loop();
    return -1;
  }

  tms::EnergyStartStopLevel energy_level() const
  {
    std::lock_guard<std::mutex> guard(essl_m_);
    return essl_;
  }

  void energy_level(tms::EnergyStartStopLevel essl)
  {
    std::lock_guard<std::mutex> guard(essl_m_);
    essl_ = essl;
    essl_cv_.notify_one();
  }

  void wait_for_operational_energy_level()
  {
    std::unique_lock<std::mutex> lock(essl_m_);
    essl_cv_.wait(lock, [this] { return essl_ == tms::EnergyStartStopLevel::ESSL_OPERATIONAL; });
  }

  void simulate_power_flow()
  {
    wait_for_connections();
    const powersim::ConnectedDeviceSeq& connected_devs = connected_devices_out();

    while (!shutdown()) {
      wait_for_operational_energy_level();

      while (!shutdown() && energy_level() == tms::EnergyStartStopLevel::ESSL_OPERATIONAL) {
        powersim::ElectricCurrent ec;
        ec.power_path().push_back(get_device_id());
        ec.power_path().push_back(connected_devs[0].id());
        ec.amperage() = 1.0f;
        const DDS::ReturnCode_t rc = ec_dw_->write(ec, DDS::HANDLE_NIL);
        if (rc != DDS::RETCODE_OK) {
          ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: SourceDevice::simulate_power_flow: "
                    "write ElectricCurrent failed: %C\n", OpenDDS::DCPS::retcode_to_string(rc)));
        }

        if (verbose_) {
          std::cout << "=== Sending power to device \"" << connected_devs[0].id() << "\"..." << std::endl;
        }

        // Frequency of messages can be proportional to the power measure?
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
    }
  }

  int run() override
  {
    std::thread sim_thr(&SourceDevice::simulate_power_flow, this);
    const int ret = run_i();
    sim_thr.join();
    return ret;
  }

  tms::ReplyDataWriter_var reply_dw() const
  {
    return reply_dw_;
  }

private:
  tms::DeviceInfo populate_device_info() const override
  {
    auto device_info = get_device_info();
    device_info.role() = tms::DeviceRole::ROLE_SOURCE;
    device_info.product() = Utils::get_ProductInfo();
    device_info.topics() = Utils::get_TopicInfo({}, {}, { tms::topic::TOPIC_ENERGY_START_STOP_REQUEST });

    tms::PowerDeviceInfo pdi;
    {
      // The spec require 1 power port entry for source device
      pdi.powerPorts() = { tms::PowerPortInfo() };
      tms::SourceInfo source_info;
      {
        source_info.features() = { tms::SourceFeature::SRCF_GENSET, tms::SourceFeature::SRCF_SOLAR };
        source_info.supportedEnergyStartStopLevels() = { tms::EnergyStartStopLevel::ESSL_OFF,
                                                         tms::EnergyStartStopLevel::ESSL_OPERATIONAL };
      }
      pdi.source() = source_info;
    }
    device_info.powerDevice() = pdi;
    return device_info;
  }

  // For graceful shutdown of the device
  mutable std::mutex shutdown_m_;
  bool shutdown_ = false;

  // For changing energy level while the device is running
  std::condition_variable essl_cv_;
  mutable std::mutex essl_m_;
  tms::EnergyStartStopLevel essl_ = tms::EnergyStartStopLevel::ESSL_OPERATIONAL;

  tms::ReplyDataWriter_var reply_dw_;
  powersim::ElectricCurrentDataWriter_var ec_dw_;
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
  bool verbose = false;

  ACE_Get_Opt get_opt(argc, argv, "d:i:v");
  if (get_opt.long_option("domain", 'd', ACE_Get_Opt::ARG_REQUIRED) != 0 ||
      get_opt.long_option("id", 'i', ACE_Get_Opt::ARG_REQUIRED) != 0 ||
      get_opt.long_option("verbose", 'v', ACE_Get_Opt::NO_ARG) != 0) {
    return 1;
  }

  int c;
  while ((c = get_opt()) != -1) {
    switch (c) {
    case 'i':
      src_id = get_opt.opt_arg();
      break;
    case 'd':
      domain_id = static_cast<DDS::DomainId_t>(ACE_OS::atoi(get_opt.opt_arg()));
      break;
    case 'v':
      verbose = true;
      break;
    default:
      break;
    }
  }

  if (domain_id == OpenDDS::DOMAIN_UNKNOWN || src_id == nullptr) {
    ACE_ERROR((LM_ERROR, "Usage: %C -d DDS_Domain_Id -i Source_Device_Id [-v]\n", argv[0]));
    return 1;
  }

  SourceDevice src_dev(src_id, verbose);
  if (src_dev.init(domain_id, argc, argv) != DDS::RETCODE_OK) {
    return 1;
  }
  return src_dev.run();
}
