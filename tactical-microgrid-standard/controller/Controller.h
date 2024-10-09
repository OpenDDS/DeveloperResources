#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "common/Handshaking.h"

#include <unordered_map>

class Controller : public Handshaking {
public:
  Controller(const tms::Identity& id) : Handshaking(id) {}

  DDS::ReturnCode_t run(DDS::DomainId_t domain_id, int argc = 0, char* argv[] = nullptr);

  tms::Identity id() const;

  using PowerDevices = std::unordered_map<tms::Identity, tms::DeviceInfo>;
  PowerDevices power_devices() const;

private:
  void device_info_cb(const tms::DeviceInfo& di, const DDS::SampleInfo& si);

  void heartbeat_cb(const tms::Heartbeat& hb, const DDS::SampleInfo& si);

  void populate_device_info(tms::DeviceInfo& device_info);

private:
  mutable std::mutex mut_;
  PowerDevices power_devices_;
};

#endif
