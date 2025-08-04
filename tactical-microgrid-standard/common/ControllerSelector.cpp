#include "ControllerSelector.h"

ControllerSelector::ControllerSelector(const tms::Identity& device_id, ACE_Reactor* reactor)
  : TimerHandler(reactor)
  , ControllerCallbacks(lock_)
  , device_id_(device_id)
{
}

ControllerSelector::~ControllerSelector()
{
}

void ControllerSelector::got_heartbeat(const tms::Heartbeat& hb)
{
  Guard g(lock_);
  auto it = all_controllers_.find(hb.deviceId());
  if (it != all_controllers_.end()) {
    it->second = Clock::now(); // Update last heartbeat
    cancel<NoControllers>();

    if (selected_.empty()) {
      if (!this->get_timer<NewController>()->active()) {
        schedule_once(NewController{hb.deviceId()}, new_active_controller_delay);
      }
    } else if (is_selected(hb.deviceId())) {
      cancel<LostController>();
      if (this->get_timer<MissedHeartbeat>()->active()) {
        reschedule<MissedHeartbeat>();
      } else {
        // MissedHeartbeat was triggered, so we need to schedule it again.
        schedule_once(MissedHeartbeat{}, heartbeat_deadline);
      }
    }
  }
}

void ControllerSelector::got_device_info(const tms::DeviceInfo& di)
{
  Guard g(lock_);
  // We don't know from the heartbeat what's a controller, so we have to
  // insert entries for all_controllers_ here.
  // TODO: Are these supposed to be removed somehow?
  if (di.role() == tms::DeviceRole::ROLE_MICROGRID_CONTROLLER &&
      !all_controllers_.count(di.deviceId())) {
    all_controllers_[di.deviceId()] = TimePoint::min();
    prioritized_controllers_.insert(PrioritizedController(di));
  }
}

void ControllerSelector::timer_fired(Timer<NewController>& timer)
{
  Guard g(lock_);
  const auto& mc_id = timer.arg.id;
  ACE_DEBUG((LM_INFO, "(%P|%t) INFO: ControllerSelector::timed_event(NewController): "
    "triggered by \"%C\" heartbeat\n", mc_id.c_str()));

  if (all_controllers_.find(mc_id) == all_controllers_.end()) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: ControllerSelector::timed_event(NewController): Controller \"%C\" not found!\n",
               mc_id.c_str()));
    return;
  }

  // Select the highest priority controller from the list of available controllers.
  // The TMS spec doesn't specify this, and an implementation may select the controller
  // associated with this timer. However, that could cause different controllers to be
  // selected by different devices, since each device may receive the first heartbeat from
  // a different controller and thus select a different controller when the timer expires.
  select_controller();
}

void ControllerSelector::timer_fired(Timer<MissedHeartbeat>& timer)
{
  Guard g(lock_);
  const auto& timer_id = timer.id;
  ACE_DEBUG((LM_INFO, "(%P|%t) INFO: ControllerSelector::timed_event(MissedHeartbeat): "
    "\"%C\". Timer id: %d\n", selected_.c_str(), timer_id));
  if (missed_heartbeat_callback_) {
    missed_heartbeat_callback_(selected_);
  }
  schedule_once(LostController{}, lost_active_controller_delay);

  // Start a No MC timer if the device has missed heartbeats from all MCs
  const TimePoint now = Clock::now();
  bool no_avail_mc = true;
  for (const auto& pair : all_controllers_) {
    if (now - pair.second < heartbeat_deadline) {
      no_avail_mc = false;
      break;
    }
  }

  if (no_avail_mc) {
    schedule_once(NoControllers{}, no_controllers_delay);
  }
}

void ControllerSelector::timer_fired(Timer<LostController>&)
{
  Guard g(lock_);
  ACE_DEBUG((LM_INFO, "(%P|%t) INFO: ControllerSelector::timed_event(LostController): "
    "\"%C\"\n", selected_.c_str()));
  if (lost_controller_callback_) {
    lost_controller_callback_(selected_);
  }
  selected_.clear();

  // Select a new controller if possible. If there are no recent controllers
  // and we don't hear from another, the NoControllers timer will fire.
  select_controller();
}

void ControllerSelector::timer_fired(Timer<NoControllers>&)
{
  Guard g(lock_);
  ACE_DEBUG((LM_INFO, "(%P|%t) INFO: ControllerSelector::timed_event(NoControllers)\n"));
  if (no_controllers_callback_) {
    no_controllers_callback_();
  }
  // TODO: CONFIG_ON_COMMS_LOSS
}

// Select a new controller at start up,
// or when a device loses its active controller and has to select a new one.
bool ControllerSelector::select_controller()
{
  const TimePoint now = Clock::now();

  // Select an available controller with smallest identity alphabetically
  for (auto it = prioritized_controllers_.begin(); it != prioritized_controllers_.end(); ++it) {
    auto mc_info = all_controllers_.find(it->id);
    const auto last_hb = now - mc_info->second;
    // TMS spec doesn't specify this. But it should make sure the controller is still available
    // i.e., last heartbeat received within 3 seconds.
    if (last_hb < heartbeat_deadline) {
      select(mc_info->first, std::chrono::duration_cast<Sec>(last_hb));
      break;
    }
  }
  return false;
}

void ControllerSelector::select(const tms::Identity& id, Sec last_hb)
{
  ACE_DEBUG((LM_INFO, "(%P|%t) INFO: ControllerSelector::select: \"%C\"\n", id.c_str()));
  selected_ = id;
  if (new_controller_callback_) {
    new_controller_callback_(selected_);
  }
  send_controller_state();
  schedule_once(MissedHeartbeat{}, heartbeat_deadline - last_hb);
}

void ControllerSelector::send_controller_state()
{
  tms::ActiveMicrogridControllerState amcs;
  amcs.deviceId() = device_id_;
  if (!selected_.empty()) {
    amcs.masterId() = selected_;
  }

  const DDS::ReturnCode_t rc = amcs_dw_->write(amcs, DDS::HANDLE_NIL);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: ControllerSelector::send_controller_state: write ActiveMicrogridControllerState failed\n"));
  }
}
