#ifndef TMS_COMMON_CONTROLLER_SELECTOR_H
#define TMS_COMMON_CONTROLLER_SELECTOR_H

#include "TimerHandler.h"

#include <common/mil-std-3071_data_modelTypeSupportImpl.h>
#include <common/OpenDDS_TMS_export.h>

#include <map>
#include <tuple>
#include <set>

struct NewController {
  tms::Identity id;
  static const char* name() { return "NewController"; }
};

struct MissedHeartbeat {
  static const char* name() { return "MissedHeartbeat"; }
};

struct LostController {
  static const char* name() { return "LostController"; }
};

struct NoControllers {
  static const char* name() { return "NoControllers"; }
};

class OpenDDS_TMS_Export ControllerCallbacks {
public:
  using IdCallback = std::function<void(const tms::Identity&)>;
  using Callback = std::function<void()>;

  explicit ControllerCallbacks(Mutex& lock) : cb_lock_(lock) {}

  void set_new_controller_callback(IdCallback cb)
  {
    Guard g(cb_lock_);
    new_controller_callback_ = cb;
  }

  void set_missed_heartbeat_callback(IdCallback cb)
  {
    Guard g(cb_lock_);
    missed_heartbeat_callback_ = cb;
  }

  void set_lost_controller_callback(IdCallback cb)
  {
    Guard g(cb_lock_);
    lost_controller_callback_ = cb;
  }

  void set_no_controllers_callback(Callback cb)
  {
    Guard g(cb_lock_);
    no_controllers_callback_ = cb;
  }

protected:
  IdCallback new_controller_callback_;
  IdCallback missed_heartbeat_callback_;
  IdCallback lost_controller_callback_;
  Callback no_controllers_callback_;

private:
  Mutex& cb_lock_;
};

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
 *                   +-[A,R]->MissedHeartbeat<-[3s]-select()->{ActiveMicrogridControllerState}
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
class OpenDDS_TMS_Export ControllerSelector
  : public TimerHandler<NewController, MissedHeartbeat, LostController, NoControllers>
  , public ControllerCallbacks
{
public:
  explicit ControllerSelector(const tms::Identity& device_id, ACE_Reactor* reactor = nullptr);
  ~ControllerSelector();

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

  void set_ActiveMicrogridControllerState_writer(tms::ActiveMicrogridControllerStateDataWriter_var amcs_dw)
  {
    amcs_dw_ = amcs_dw;
  }

private:
  static constexpr Sec heartbeat_deadline = Sec(3);
  static constexpr Sec new_active_controller_delay = Sec(3);
  static constexpr Sec lost_active_controller_delay = Sec(6);
  static constexpr Sec no_controllers_delay = Sec(10);

  void timer_fired(Timer<NewController>& timer);
  void timer_fired(Timer<MissedHeartbeat>&);
  void timer_fired(Timer<LostController>&);
  void timer_fired(Timer<NoControllers>&);
  void any_timer_fired(AnyTimer timer)
  {
    std::visit([&](auto&& value) { this->timer_fired(*value); }, timer);
  }

  void select(const tms::Identity& id, Sec last_hb = Sec(0));

  bool select_controller();
  void send_controller_state();

  tms::Identity selected_;
  std::map<tms::Identity, TimePoint> all_controllers_;

  struct PrioritizedController {
    uint16_t priority = 0;
    tms::Identity id;

    explicit PrioritizedController(const tms::DeviceInfo& di)
    : id(di.deviceId())
    {
      const auto& cs = di.controlService();
      if (cs) {
        const auto& mc = cs->mc();
        if (mc) {
          priority = mc->priorityRanking();
        }
      }
    }

    bool operator<(const PrioritizedController& lhs) const
    {
      return std::tie(priority, id) < std::tie(lhs.priority, lhs.id);
    }
  };
  std::set<PrioritizedController> prioritized_controllers_;

  // Device ID to which this controller selector belong.
  tms::Identity device_id_;

  tms::ActiveMicrogridControllerStateDataWriter_var amcs_dw_;
};

#endif
