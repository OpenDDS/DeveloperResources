#ifndef CONTROLLER_CONTROLLER_H
#define CONTROLLER_CONTROLLER_H

#include "Common.h"

#include <common/Handshaking.h>
#include <common/Configurable.h>

class Controller : public Handshaking, Configurable {
public:
  explicit Controller(const tms::Identity& id, uint16_t priority = 0)
    : Handshaking(id)
    , Configurable("TMS_CONTROLLER")
    , priority_(priority)
  {
  }

  DDS::ReturnCode_t init(DDS::DomainId_t domain_id, int argc = 0, char* argv[] = nullptr);
  int run();
  tms::Identity id() const;
  PowerDevices power_devices() const;
  void update_essl(const tms::Identity& pd_id, tms::EnergyStartStopLevel to_level);
  void terminate();

  bool got_config(const std::string& name, const OpenDDS::DCPS::ConfigPair& pair);
  void set_debug(bool value);

  DDS::DomainId_t tms_domain_id() const
  {
    return tms_domain_id_;
  }

  void set_active_controller(const tms::Identity& pd_id, const std::optional<tms::Identity>& master_id);

private:
  void device_info_cb(const tms::DeviceInfo& di, const DDS::SampleInfo& si);
  void heartbeat_cb(const tms::Heartbeat& hb, const DDS::SampleInfo& si);
  tms::DeviceInfo populate_device_info() const;

  mutable std::mutex mut_;
  bool debug_ = false;
  PowerDevices power_devices_;
  uint16_t priority_;

  DDS::DomainId_t tms_domain_id_ = OpenDDS::DOMAIN_UNKNOWN;;
};

#endif
