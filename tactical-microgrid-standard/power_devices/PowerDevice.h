#ifndef TMS_POWER_DEVICE_H
#define TMS_POWER_DEVICE_H

#include "common/Handshaking.h"
#include "common/ControllerSelector.h"
#include "PowerSimTypeSupportImpl.h"
#include "PowerSim_Idl_export.h"

#include <condition_variable>

class PowerSim_Idl_Export PowerDevice : public Handshaking {
public:
  explicit PowerDevice(const tms::Identity& id, tms::DeviceRole role = tms::DeviceRole::ROLE_SOURCE, bool verbose = false)
    : Handshaking(id)
    , verbose_(verbose)
    , role_(role)
  {
  }

  DDS::ReturnCode_t init(DDS::DomainId_t domain, int argc = 0, char* argv[] = nullptr);

  tms::Identity selected() const
  {
    return controller_selector_.selected();
  }

  virtual int run()
  {
    return reactor_->run_reactor_event_loop() == 0 ? 0 : 1;
  }

  powersim::ConnectedDeviceSeq connected_devices_in() const
  {
    std::lock_guard<std::mutex> guard(connected_devices_m_);
    return connected_devices_in_;
  }

  powersim::ConnectedDeviceSeq connected_devices_out() const
  {
    std::lock_guard<std::mutex> guard(connected_devices_m_);
    return connected_devices_out_;
  }

  // Wait for power connections to be established
  void wait_for_connections();

  // Set the devices connected to this device
  void connected_devices(const powersim::ConnectedDeviceSeq& devices);

  bool verbose() const
  {
    return verbose_;
  }

protected:
  // Concrete power device should override this function depending on their role.
  virtual tms::DeviceInfo populate_device_info() const;

  std::condition_variable connected_devices_cv_;
  mutable std::mutex connected_devices_m_;

  // List of devices that can send power to this device.
  // Load device has at most one connected device in this list.
  powersim::ConnectedDeviceSeq connected_devices_in_;

  // List of devices that this device can send power to.
  // Source device has at most one connected device in this list.
  powersim::ConnectedDeviceSeq connected_devices_out_;

  // Participant containing entities for simulation topics
  DDS::DomainParticipant_var sim_participant_;

  // Whether to print power simulation messages
  bool verbose_;

private:
  void got_heartbeat(const tms::Heartbeat& hb, const DDS::SampleInfo& si);
  void got_device_info(const tms::DeviceInfo& di, const DDS::SampleInfo& si);

  ControllerSelector controller_selector_;
  tms::DeviceRole role_;
};

#endif
