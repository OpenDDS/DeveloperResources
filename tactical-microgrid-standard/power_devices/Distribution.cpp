#include "PowerDevice.h"
#include "PowerSimTypeSupportImpl.h"
#include "common/DataReaderListenerBase.h"

#include <dds/DCPS/Marked_Default_Qos.h>

#include <ace/Get_Opt.h>


class DistributionDevice;

class ElectricCurrentDataReaderListenerImpl : public DataReaderListenerBase {
public:
  explicit ElectricCurrentDataReaderListenerImpl(DistributionDevice& dist_dev)
    : DataReaderListenerBase("powersim::ElectricCurrent - DataReaderListenerImpl")
    , dist_dev_(dist_dev)
  {
  }

  virtual ~ElectricCurrentDataReaderListenerImpl() = default;

  void on_data_available(DDS::DataReader_ptr reader) final;

private:
  DistributionDevice& dist_dev_;
};

class DistributionDevice : public PowerDevice {
public:
  explicit DistributionDevice(const tms::Identity& id, bool verbose = false)
    : PowerDevice(id, tms::DeviceRole::ROLE_DISTRIBUTION, verbose)
  {
  }

  DDS::ReturnCode_t init(DDS::DomainId_t domain_id, int argc = 0, char* argv[] = nullptr)
  {
    DDS::ReturnCode_t rc = PowerDevice::init(domain_id, argc, argv);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }

    // Publish to powersim::ElectricCurrent topic
    powersim::ElectricCurrentTypeSupport_var ec_ts = new powersim::ElectricCurrentTypeSupportImpl;
    if (DDS::RETCODE_OK != ec_ts->register_type(sim_participant_, "")) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DistributionDevice::init: register_type ElectricCurrent failed\n"));
      return DDS::RETCODE_ERROR;
    }

    CORBA::String_var ec_type_name = ec_ts->get_type_name();
    DDS::Topic_var ec_topic = sim_participant_->create_topic(powersim::TOPIC_ELECTRIC_CURRENT.c_str(),
                                                             ec_type_name,
                                                             TOPIC_QOS_DEFAULT,
                                                             nullptr,
                                                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!ec_topic) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DistributionDevice::init: create_topic \"%C\" failed\n",
                 powersim::TOPIC_ELECTRIC_CURRENT.c_str()));
      return DDS::RETCODE_ERROR;
    }

    DDS::Publisher_var sim_pub = sim_participant_->create_publisher(PUBLISHER_QOS_DEFAULT,
                                                                    nullptr,
                                                                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!sim_pub) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DistributionDevice::init: create_publisher failed\n"));
      return DDS::RETCODE_ERROR;
    }

    DDS::DataWriter_var ec_dw_base = sim_pub->create_datawriter(ec_topic,
                                                                DATAWRITER_QOS_DEFAULT,
                                                                nullptr,
                                                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!ec_dw_base) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DistributionDevice::init: create_datawriter for topic \"%C\" failed\n",
                 powersim::TOPIC_ELECTRIC_CURRENT.c_str()));
      return DDS::RETCODE_ERROR;
    }

    ec_dw_ = powersim::ElectricCurrentDataWriter::_narrow(ec_dw_base);
    if (!ec_dw_) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DistributionDevice::init: ElectricCurrentDataWriter narrow failed\n"));
      return DDS::RETCODE_ERROR;
    }

    // Subscribe to powersim::ElectricCurrent topic
    DDS::Subscriber_var sim_sub = sim_participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                                                      nullptr,
                                                                      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!sim_sub) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DistributionDevice::init: create_subscriber failed\n"));
      return DDS::RETCODE_ERROR;
    }

    DDS::DataReaderListener_var ec_listener(new ElectricCurrentDataReaderListenerImpl(*this));
    DDS::DataReader_var ec_dr_base = sim_sub->create_datareader(ec_topic,
                                                                DATAREADER_QOS_DEFAULT,
                                                                ec_listener,
                                                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!ec_dr_base) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DistributionDevice::init: create_datareader for topic \"%C\" failed\n",
                 powersim::TOPIC_ELECTRIC_CURRENT.c_str()));
      return DDS::RETCODE_ERROR;
    }

    return DDS::RETCODE_OK;;
  }

  powersim::ElectricCurrentDataWriter_var get_electric_current_data_writer() const
  {
    return ec_dw_;
  }

private:
  powersim::ElectricCurrentDataWriter_var ec_dw_;
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

  const powersim::ConnectedDeviceSeq& connected_devices_in = dist_dev_.connected_devices_in();
  const powersim::ConnectedDeviceSeq& connected_devices_out = dist_dev_.connected_devices_out();

  for (CORBA::ULong i = 0; i < data.length(); ++i) {
    if (info_seq[i].valid_data) {
      const powersim::ElectricCurrent& ec = data[i];
      const size_t length = ec.power_path().size();
      if (length < 2) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: ElectricCurrentDataReaderListenerImpl::on_data_available: Invalid power path (length %u)\n", length));
        continue;
      }

      const tms::Identity& from = ec.power_path()[length - 2];
      const tms::Identity& to = ec.power_path()[length - 1];

      // Check that the simulated current is for this device
      if (to != dist_dev_.get_device_id()) {
        continue;
      }

      // Check that it comes from a device connected to an input power port
      bool found_in_dev = false;
      for (const auto& in_dev : connected_devices_in) {
        if (in_dev.id() == from) {
          found_in_dev = true;
          break;
        }
      }
      if (!found_in_dev) {
        continue;
      }

      // Relay to all devices connected to the output power ports
      for (const auto& out_dev : connected_devices_out) {
        powersim::ElectricCurrent relay_ec = ec;
        relay_ec.power_path().push_back(out_dev.id());
        const DDS::ReturnCode_t rc = dist_dev_.get_electric_current_data_writer()->write(relay_ec, DDS::HANDLE_NIL);
        if (rc != DDS::RETCODE_OK) {
          ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: ElectricCurrentDataReaderListenerImpl::on_data_available: "
                     "write ElectricCurrent failed: %C\n", OpenDDS::DCPS::retcode_to_string(rc)));
        }

        if (dist_dev_.verbose()) {
          std::cout << "=== Relaying power from device \"" << from << "\" to device \""
                    << out_dev.id() << "\" -- " << ec.amperage() << "Amps ..." << std::endl;
        }
      }
    }
  }
}

int main(int argc, char* argv[])
{
  DDS::DomainId_t domain_id = OpenDDS::DOMAIN_UNKNOWN;
  const char* dist_id = nullptr;
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
      dist_id = get_opt.opt_arg();
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

  if (domain_id == OpenDDS::DOMAIN_UNKNOWN || dist_id == nullptr) {
    ACE_ERROR((LM_ERROR, "Usage: %C -d DDS_Domain_Id -i Distribution_Device_Id [-v]\n", argv[0]));
    return 1;
  }

  DistributionDevice dist_dev(dist_id, verbose);
  if (dist_dev.init(domain_id, argc, argv) != DDS::RETCODE_OK) {
    return 1;
  }
  return dist_dev.run();
}
