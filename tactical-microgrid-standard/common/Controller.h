#ifndef TMS_CONTROLLER_H
#define TMS_CONTROLLER_H

#include "Handshaking.h"

class opendds_tms_Export Controller : public Handshaking {
public:
  Controller(const tms::Identity& id)
    : Handshaking(id)
  {
  }

  DDS::ReturnCode_t init(DDS::DomainId_t domain, int argc = 0, char* argv[] = nullptr);

  virtual tms::DeviceInfo get_device_info() const
  {
    auto device_info = Handshaking::get_device_info();
    device_info.role(tms::DeviceRole::ROLE_MICROGRID_CONTROLLER);
    return device_info;
  }

private:
  void got_heartbeat(const tms::Heartbeat& hb, const DDS::SampleInfo& si);
  void got_device_info(const tms::DeviceInfo& di, const DDS::SampleInfo& si);
};

#endif
