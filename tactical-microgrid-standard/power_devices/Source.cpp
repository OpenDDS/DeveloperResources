#include "PowerDevice.h"
#include "PowerSimTypeSupportImpl.h"
#include "common/QosHelper.h"
#include "common/Utils.h"

#include <dds/DCPS/Marked_Default_Qos.h>

#include <ace/Get_Opt.h>

#include <thread>
#include <chrono>

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

  void energy_level(tms::EnergyStartStopLevel essl) override
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

  // For managing changes in energy level while the device is running
  std::condition_variable essl_cv_;

  powersim::ElectricCurrentDataWriter_var ec_dw_;
};


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
