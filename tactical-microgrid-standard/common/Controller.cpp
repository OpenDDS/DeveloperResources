#include "Controller.h"

DDS::ReturnCode_t Controller::init(DDS::DomainId_t domain, int argc, char* argv[])
{
  DDS::ReturnCode_t rc = join_domain(domain, argc, argv);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  rc = create_subscribers(
    [&](const tms::DeviceInfo& di, const DDS::SampleInfo& si) {
      got_device_info(di, si);
    },
    [&](const tms::Heartbeat& hb, const DDS::SampleInfo& si) {
      got_heartbeat(hb, si);
    }
  );
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  rc = create_publishers();
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  tms::DeviceInfo device_info;
  device_info.deviceId(id_);
  device_info.role(tms::DeviceRole::ROLE_MICROGRID_CONTROLLER);
  rc = send_device_info(device_info);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  rc = start_heartbeats();
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  return DDS::RETCODE_OK;
}

void Controller::got_heartbeat(const tms::Heartbeat& hb, const DDS::SampleInfo& si)
{
  if (!si.valid_data || hb.deviceId() == id_) {
    return;
  }
  ACE_DEBUG((LM_INFO, "(%P|%t) Handshaking::controller_got_heartbeat: from %C\n",
    hb.deviceId().c_str()));
}

void Controller::got_device_info(const tms::DeviceInfo& di, const DDS::SampleInfo& si)
{
  if (!si.valid_data || di.deviceId() == id_) {
    return;
  }
  ACE_DEBUG((LM_INFO, "(%P|%t) Handshaking::controller_got_device_info: from %C\n",
    di.deviceId().c_str()));
}
