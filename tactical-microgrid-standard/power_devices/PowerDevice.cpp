#include "PowerDevice.h"
#include "PowerConnectionDataReaderListenerImpl.h"
#include "common/Utils.h"

#include <dds/DCPS/Marked_Default_Qos.h>

DDS::ReturnCode_t PowerDevice::init(DDS::DomainId_t domain, int argc, char* argv[])
{
  DDS::ReturnCode_t rc = join_domain(domain, argc, argv);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  rc = create_subscribers(
    [&](const auto& di, const auto& si) { got_device_info(di, si); },
    [&](const auto& hb, const auto& si) { got_heartbeat(hb, si); });
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  rc = create_publishers();
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  // Subscribe to the PowerConnection topic
  const DDS::DomainId_t sim_domain_id = Utils::get_sim_domain_id(domain);
  sim_participant_ = get_participant_factory()->create_participant(sim_domain_id,
                                                                   PARTICIPANT_QOS_DEFAULT,
                                                                   nullptr,
                                                                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!sim_participant_) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: PowerDevice::init: create simulation participant failed\n"));
    return DDS::RETCODE_ERROR;
  }

  powersim::PowerConnectionTypeSupport_var pc_ts = new powersim::PowerConnectionTypeSupportImpl;
  if (DDS::RETCODE_OK != pc_ts->register_type(sim_participant_, "")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: PowerDevice::init: register_type PowerConnection failed\n"));
    return DDS::RETCODE_ERROR;
  }

  CORBA::String_var pc_type_name = pc_ts->get_type_name();
  DDS::Topic_var pc_topic = sim_participant_->create_topic(powersim::TOPIC_POWER_CONNECTION.c_str(),
                                                           pc_type_name,
                                                           TOPIC_QOS_DEFAULT,
                                                           nullptr,
                                                           ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pc_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: PowerDevice::init: create_topic \"%C\" failed\n",
               powersim::TOPIC_POWER_CONNECTION.c_str()));
    return DDS::RETCODE_ERROR;
  }

  DDS::Subscriber_var sim_sub = sim_participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                                                    nullptr,
                                                                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!sim_sub) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: PowerDevice::init: create_subscriber for simulating power connection failed\n"));
    return DDS::RETCODE_ERROR;
  }

  DDS::DataReaderQos dr_qos;
  sim_sub->get_default_datareader_qos(dr_qos);
  dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

  DDS::DataReaderListener_var pc_listener(new PowerConnectionDataReaderListenerImpl(*this));
  DDS::DataReader_var pc_dr_base = sim_sub->create_datareader(pc_topic,
                                                              dr_qos,
                                                              pc_listener,
                                                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pc_dr_base) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: PowerDevice::init: create_datareader for topic \"%C\" failed\n",
               powersim::TOPIC_POWER_CONNECTION.c_str()));
    return DDS::RETCODE_ERROR;
  }

  // Advertise its device information and start heartbeats
  auto di = populate_device_info();
  return send_device_info(di);
}

tms::DeviceInfo PowerDevice::populate_device_info() const
{
  auto di = get_device_info();
  di.role(role_);
  return di;
}

void PowerDevice::got_heartbeat(const tms::Heartbeat& hb, const DDS::SampleInfo& si)
{
  if (!si.valid_data || hb.deviceId() == device_id_) {
    return;
  }

  if (OpenDDS::DCPS::DCPS_debug_level >= 8) {
    ACE_DEBUG((LM_INFO, "(%P|%t) INFO: Handshaking::power_device_got_heartbeat: from %C\n", hb.deviceId().c_str()));
  }
  controller_selector_.got_heartbeat(hb);
}

void PowerDevice::got_device_info(const tms::DeviceInfo& di, const DDS::SampleInfo& si)
{
  if (!si.valid_data || di.deviceId() == device_id_) {
    return;
  }
  ACE_DEBUG((LM_INFO, "(%P|%t) INFO: Handshaking::power_device_got_device_info: from %C\n", di.deviceId().c_str()));
  controller_selector_.got_device_info(di);
}

void PowerDevice::wait_for_connections()
{
  std::unique_lock<std::mutex> lock(connected_devices_m_);
  connected_devices_cv_.wait(lock, [this] { return !connected_devices_in_.empty() || !connected_devices_out_.empty(); });
}

void PowerDevice::connected_devices(const powersim::ConnectedDeviceSeq& devices)
{
  std::lock_guard<std::mutex> guard(connected_devices_m_);

  for (size_t i = 0; i < devices.size(); ++i) {
    switch (role_) {
    case tms::DeviceRole::ROLE_SOURCE:
      // Source device has a single out port
      if (!connected_devices_out_.empty()) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: PowerDevice::connected_devices: Source \"%C\" already connects to \"%C\". Replace with \"%C\"\n",
                   get_device_id().c_str(), connected_devices_out_[0].id().c_str(), devices[i].id().c_str()));
      }
      connected_devices_out_[0] = devices[i];
      break;
    case tms::DeviceRole::ROLE_LOAD:
      // Load device has a single in port.
      if (!connected_devices_in_.empty()) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: PowerDevice::connected_devices: Load \"%C\" already connects to \"%C\". Replace with \"%C\"\n",
                   get_device_id().c_str(), connected_devices_in_[0].id().c_str(), devices[i].id().c_str()));
      }
      connected_devices_in_[0] = devices[i];
      break;
    case tms::DeviceRole::ROLE_DISTRIBUTION:
      {
        const tms::DeviceRole other_role = devices[i].role();
        if (other_role == tms::DeviceRole::ROLE_SOURCE) {
          // Can only receive power from the other device
          connected_devices_in_.push_back(devices[i]);
        } else if (other_role == tms::DeviceRole::ROLE_LOAD) {
          // Can only send power to the other device
          connected_devices_out_.push_back(devices[i]);
        } else if (other_role == tms::DeviceRole::ROLE_DISTRIBUTION) {
          // Can both send to and receive power from the other distribution device
          connected_devices_in_.push_back(devices[i]);
          connected_devices_out_.push_back(devices[i]);
        } else {
          // Should never happen, but just ignore this other device
          ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: PowerDevice::connected_devices: Unsupported device role (\"%C\") of other device!\n",
                     Utils::device_role_to_string(other_role).c_str()));
        }
      }
      break;
    default:
      // Should never happen
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: PowerDevice::connected_devices: Unsupported device role (\"%C\") of mine!\n",
                 Utils::device_role_to_string(role_).c_str()));
      return;
    }
  }
  connected_devices_cv_.notify_one();
}
