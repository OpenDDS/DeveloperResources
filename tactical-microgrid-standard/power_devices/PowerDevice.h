#ifndef TMS_POWER_DEVICE_H
#define TMS_POWER_DEVICE_H

#include "common/Handshaking.h"
#include "common/ControllerSelector.h"
#include "PowerSimTypeSupportImpl.h"
#include "PowerSim_Idl_export.h"

#include <condition_variable>
#include <thread>

class PowerSim_Idl_Export PowerDevice : public Handshaking {
public:
  explicit PowerDevice(const tms::Identity& id, tms::DeviceRole role = tms::DeviceRole::ROLE_SOURCE, bool verbose = false)
    : Handshaking(id)
    , verbose_(verbose)
    , controller_selector_(id)
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
    return run_i();
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

  tms::ReplyDataWriter_var reply_dw() const
  {
    return reply_dw_;
  }

  // Each concrete power device overrides this method, if necessary, to handle
  // the change in energy level for that particular power device.
  virtual void energy_level(tms::EnergyStartStopLevel essl)
  {
    std::lock_guard<std::mutex> guard(essl_m_);
    essl_ = essl;
  }

  tms::EnergyStartStopLevel energy_level() const
  {
    std::lock_guard<std::mutex> guard(essl_m_);
    return essl_;
  }

  ControllerCallbacks& controller_callbacks()
  {
    return controller_selector_;
  }

protected:
  virtual int run_i()
  {
    if (controller_selector_.reactor() == reactor_) {
      // Same reactor instance for both handshaking and controller selection
      return reactor_->run_reactor_event_loop() == 0 ? 0 : 1;
    }

    std::thread handshaking_thr([&] { reactor_->run_reactor_event_loop(); });
    const int ret = controller_selector_.get_reactor()->run_reactor_event_loop() == 0 ? 0 : 1;
    handshaking_thr.join();
    return ret;
  }

  void delete_extra_entities()
  {
    delete_entities(sim_participant_);
  }

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

  // Data writer for sending tms::Reply. Used for tms::EnergyStartStopRequest, for example.
  tms::ReplyDataWriter_var reply_dw_;

  mutable std::mutex essl_m_;
  tms::EnergyStartStopLevel essl_ = tms::EnergyStartStopLevel::ESSL_OPERATIONAL;

  // Whether to print power simulation messages
  bool verbose_;

private:
  void got_heartbeat(const tms::Heartbeat& hb, const DDS::SampleInfo& si);
  void got_device_info(const tms::DeviceInfo& di, const DDS::SampleInfo& si);

  ControllerSelector controller_selector_;
  tms::DeviceRole role_;
};

#endif
