#ifndef TMS_POWER_DEVICE_H
#define TMS_POWER_DEVICE_H

#include "common/Handshaking.h"
#include "PowerSimTypeSupportImpl.h"

#include <dds/DCPS/Marked_Default_Qos.h>

#include <map>
#include <condition_variable>

struct NewController {
  tms::Identity id;
};
struct MissedController {};
struct LostController {};
struct NoControllers {};

class PowerDevice;

/**
 * Logic for determining what controller should be used, based on TMS A.7.5.
 *
 * Pseudo state machine:
 *
 *   got_heartbeat()-+-[E,3s]---------------->NewController
 *                   |                                   |
 *                   |                                   |
 *                   +-[A,C]------------>LostController  |
 *                   |                    ^          |   |
 *                   |                    |         [S]  |
 *                   |                   [6s]        |   |
 *                   |                    |          V   V
 *                   +-[A,R]->MissedController<-[3s]-select()->{ActiveMicrogridControllerState}
 *                   |          |
 *                   |        [10s]
 *                   |          |
 *                   |          V
 *                   +-[C]->NoControllers-->{CONFIG_ON_COMMS_LOSS}
 *
 * <N>s: Schedule a timer to this state in <N> seconds
 * C: Cancel the timer to this state
 * R: Reschedule the timer to this state
 * E: If selected_.empty() and there's not an existing timer to this state
 * A: If heartbeat is from selected
 * S: If there's a selectable controller with a recent heartbeat
 */
class OpenDDS_TMS_Export ControllerSelector :
  public TimerHandler<NewController, MissedController, LostController, NoControllers> {
public:
  explicit ControllerSelector(PowerDevice& pd)
    : pd_(pd)
  {
  }

  void got_heartbeat(const tms::Heartbeat& hb);
  void got_device_info(const tms::DeviceInfo& di);

  tms::Identity selected() const
  {
    Guard g(lock_);
    return selected_;
  }

  bool is_selected(const tms::Identity& id) const
  {
    Guard g(lock_);
    return selected_ == id;
  }

private:
  static constexpr Sec new_controller_delay = Sec(3);
  static constexpr Sec missed_controller_delay = Sec(3);
  static constexpr Sec lost_controller_delay = Sec(6);
  static constexpr Sec no_controllers_delay = Sec(10);

  void timer_fired(Timer<NewController>& timer);
  void timer_fired(Timer<MissedController>&);
  void timer_fired(Timer<LostController>&);
  void timer_fired(Timer<NoControllers>&);
  void any_timer_fired(AnyTimer timer)
  {
    std::visit([&](auto&& value) { this->timer_fired(*value); }, timer);
  }

  void select(const tms::Identity& id, Sec last_hb = Sec(0));

  PowerDevice& pd_;
  tms::Identity selected_;
  std::map<tms::Identity, TimePoint> all_controllers_;
};

class OpenDDS_TMS_Export PowerDevice : public Handshaking {
public:
  explicit PowerDevice(const tms::Identity& id, tms::DeviceRole role = tms::DeviceRole::ROLE_SOURCE)
    : Handshaking(id)
    , controller_selector_(*this)
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

protected:
  std::condition_variable connected_devices_cv_;
  mutable std::mutex connected_devices_m_;

  // List of devices that can send power to this device.
  // Load device has at most one connected device in this list.
  powersim::ConnectedDeviceSeq connected_devices_in_;

  // List of devices that this device can send power to.
  // Source device has at most one connected device in this list.
  powersim::ConnectedDeviceSeq connected_devices_out_;

private:
  void got_heartbeat(const tms::Heartbeat& hb, const DDS::SampleInfo& si);
  void got_device_info(const tms::DeviceInfo& di, const DDS::SampleInfo& si);

  ControllerSelector controller_selector_;
  tms::DeviceRole role_;
};

#endif
