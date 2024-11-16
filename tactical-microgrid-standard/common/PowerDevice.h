#ifndef TMS_POWER_DEVICE_H
#define TMS_POWER_DEVICE_H

#include "Handshaking.h"

#include <map>

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
class ControllerSelector :
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
  explicit PowerDevice(const tms::Identity& id)
    : Handshaking(id)
    , controller_selector_(*this)
  {
  }

  DDS::ReturnCode_t init(DDS::DomainId_t domain, int argc = 0, char* argv[] = nullptr);

  tms::Identity selected() const
  {
    return controller_selector_.selected();
  }

private:
  void got_heartbeat(const tms::Heartbeat& hb, const DDS::SampleInfo& si);
  void got_device_info(const tms::DeviceInfo& di, const DDS::SampleInfo& si);

  ControllerSelector controller_selector_;
};

#endif
