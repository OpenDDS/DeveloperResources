#include "PowerDevice.h"
#include "PowerSimTypeSupportImpl.h"
#include "common/DataReaderListenerBase.h"

#include <dds/DCPS/Marked_Default_Qos.h>

#include <ace/Get_Opt.h>

class LoadDevice;

class ElectricCurrentDataReaderListenerImpl : public DataReaderListenerBase {
public:
  explicit ElectricCurrentDataReaderListenerImpl(LoadDevice& load_dev)
    : DataReaderListenerBase("powersim::ElectricCurrent - DataReaderListenerImpl")
    , load_dev_(load_dev)
  {
  }

  virtual ~ElectricCurrentDataReaderListenerImpl() = default;

  void on_data_available(DDS::DataReader_ptr reader) final;

private:
  LoadDevice& load_dev_;
};

class LoadDevice : public PowerDevice {
public:
  explicit LoadDevice(const tms::Identity& id)
    : PowerDevice(id, tms::DeviceRole::ROLE_LOAD)
  {
  }

  DDS::ReturnCode_t init(DDS::DomainId_t domain_id, int argc = 0, char* argv[] = nullptr)
  {
    DDS::ReturnCode_t rc = PowerDevice::init(domain_id, argc, argv);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }

    DDS::DomainParticipant_var dp = get_domain_participant();

    // Subscribe to powersim::ElectricCurrent topic
    powersim::ElectricCurrentTypeSupport_var ec_ts = new powersim::ElectricCurrentTypeSupportImpl;
    if (DDS::RETCODE_OK != ec_ts->register_type(dp, "")) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: LoadDevice::init: register_type ElectricCurrent failed\n"));
      return DDS::RETCODE_ERROR;
    }

    CORBA::String_var ec_type_name = ec_ts->get_type_name();
    DDS::Topic_var ec_topic = dp->create_topic(powersim::TOPIC_ELECTRIC_CURRENT.c_str(),
                                               ec_type_name,
                                               TOPIC_QOS_DEFAULT,
                                               nullptr,
                                               ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!ec_topic) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: LoadDevice::init: create_topic \"%C\" failed\n",
                 powersim::TOPIC_ELECTRIC_CURRENT.c_str()));
      return DDS::RETCODE_ERROR;
    }

    DDS::Subscriber_var sim_sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                                        nullptr,
                                                        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!sim_sub) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: LoadDevice::init: create_subscriber failed\n"));
      return DDS::RETCODE_ERROR;
    }

    DDS::DataReaderListener_var ec_listener(new ElectricCurrentDataReaderListenerImpl(*this));
    DDS::DataReader_var ec_dr_base = sim_sub->create_datareader(ec_topic,
                                                                DATAREADER_QOS_DEFAULT,
                                                                ec_listener.in(),
                                                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!ec_dr_base) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: LoadDevice::init: create_datareader for topic \"%C\" failed\n",
                 powersim::TOPIC_ELECTRIC_CURRENT.c_str()));
      return DDS::RETCODE_ERROR;
    }
    return DDS::RETCODE_OK;
  }

  tms::Identity connected_dev_id() const
  {
    const powersim::ConnectedDeviceSeq connected_devs = connected_devices_in();
    if (connected_devs.empty()) {
      return tms::Identity();
    }
    return connected_devs[0].id();
  }
};

void ElectricCurrentDataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  powersim::ElectricCurrentSeq data;
  DDS::SampleInfoSeq info_seq;
  powersim::ElectricCurrentDataReader_var typed_reader = powersim::ElectricCurrentDataReader::_narrow(reader);
  const DDS::ReturnCode_t rc = typed_reader->take(data, info_seq, DDS::LENGTH_UNLIMITED,
                                                  DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: ElectricCurrentDataReaderListenerImpl::on_data_available: "
               "take data failed: %C\n", OpenDDS::DCPS::retcode_to_string(rc)));
    return;
  }

  for (CORBA::ULong i = 0; i < data.length(); ++i) {
    if (info_seq[i].valid_data) {
      const powersim::ElectricCurrent& ec = data[i];
      const auto& power_path = ec.power_path();
      const int path_length = power_path.size();
      if (path_length < 2) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: ElectricCurrentDataReaderListenerImpl::on_data_available: invalid power path\n"));
        continue;
      }

      const tms::Identity& from = power_path[path_length - 2];
      const tms::Identity& to = power_path[path_length - 1];

      if (from == load_dev_.connected_dev_id() && to == load_dev_.get_device_id()) {
        ACE_DEBUG((LM_INFO, "Receiving power from \"%C\" -- %f Amps ...\n", from.c_str(), ec.amperage()));
        break;
      }
    }
  }
}

int main(int argc, char* argv[])
{
  DDS::DomainId_t domain_id = OpenDDS::DOMAIN_UNKNOWN;
  const char* load_id = nullptr;

  ACE_Get_Opt get_opt(argc, argv, "i:d:");
  int c;
  while ((c = get_opt()) != -1) {
    switch (c) {
    case 'i':
      load_id = get_opt.opt_arg();
      break;
    case 'd':
      domain_id = static_cast<DDS::DomainId_t>(ACE_OS::atoi(get_opt.opt_arg()));
      break;
    default:
      break;
    }
  }

  if (domain_id == OpenDDS::DOMAIN_UNKNOWN || load_id == nullptr) {
    ACE_ERROR((LM_ERROR, "Usage: %C -d DDS_Domain_Id -i Load_Device_Id\n", argv[0]));
    return 1;
  }

  LoadDevice load_dev(load_id);
  if (load_dev.init(domain_id, argc, argv) != DDS::RETCODE_OK) {
    return 1;
  }
  return load_dev.run();
}
