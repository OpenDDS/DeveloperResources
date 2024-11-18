#ifndef CONTROLLER_CONTROLLER_H
#define CONTROLLER_CONTROLLER_H

#include "Common.h"
#include "common/Handshaking.h"

class Controller : public Handshaking {
public:
  explicit Controller(const tms::Identity& id) : Handshaking(id) {}

  DDS::ReturnCode_t init(DDS::DomainId_t domain_id, int argc = 0, char* argv[] = nullptr);
  int run();
  tms::Identity id() const;
  PowerDevices power_devices() const;
  void terminate();

private:
  void device_info_cb(const tms::DeviceInfo& di, const DDS::SampleInfo& si);
  void heartbeat_cb(const tms::Heartbeat& hb, const DDS::SampleInfo& si);
  tms::DeviceInfo populate_device_info() const;

private:
  mutable std::mutex mut_;
  PowerDevices power_devices_;
};

#endif
