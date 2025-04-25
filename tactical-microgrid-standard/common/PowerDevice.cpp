#include "PowerDevice.h"

void ControllerSelector::got_heartbeat(const tms::Heartbeat& hb)
{
  Guard g(lock_);
  auto it = all_controllers_.find(hb.deviceId());
  if (it != all_controllers_.end()) {
    it->second = Clock::now(); // Update last heartbeat
    cancel<NoControllers>();

    if (selected_.empty()) {
      if (!this->get_timer<NewController>()->active()) {
        schedule_once(NewController{hb.deviceId()}, new_controller_delay);
      }
    } else if (is_selected(hb.deviceId())) {
      cancel<LostController>();
      if (this->get_timer<MissedController>()->active()) {
        reschedule<MissedController>();
      } else {
        // MissedController was triggered, so we need to schedule it again.
        schedule_once(MissedController{}, missed_controller_delay);
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
  }
}

void ControllerSelector::timer_fired(Timer<NewController>& timer)
{
  Guard g(lock_);
  const auto& id = timer.arg.id;
  ACE_DEBUG((LM_INFO, "(%P|%t) ControllerSelector::timed_event(NewController): "
    "\"%C\" -> \"%C\"\n", selected_.c_str(), id.c_str()));
  select(id);
}

void ControllerSelector::timer_fired(Timer<MissedController>&)
{
  Guard g(lock_);
  ACE_DEBUG((LM_INFO, "(%P|%t) ControllerSelector::timed_event(MissedController): "
    "\"%C\"\n", selected_.c_str()));
  schedule_once(LostController{}, lost_controller_delay);
  schedule_once(NoControllers{}, no_controllers_delay);
}

void ControllerSelector::timer_fired(Timer<LostController>&)
{
  Guard g(lock_);
  ACE_DEBUG((LM_INFO, "(%P|%t) ControllerSelector::timed_event(LostController): "
    "\"%C\"\n", selected_.c_str()));
  selected_.clear();

  // Select a new controller if possible. If there are no recent controllers
  // and we don't hear from another, the NoControllers timer will fire.
  const TimePoint now = Clock::now();
  for (auto it = all_controllers_.begin(); it != all_controllers_.end(); ++it) {
    const auto last_hb = now - it->second;
    if (last_hb < missed_controller_delay) {
      select(it->first, std::chrono::duration_cast<Sec>(last_hb));
      break;
    }
  }
}

void ControllerSelector::timer_fired(Timer<NoControllers>&)
{
  Guard g(lock_);
  ACE_DEBUG((LM_INFO, "(%P|%t) ControllerSelector::timed_event(NoControllers)\n"));
  // TODO: CONFIG_ON_COMMS_LOSS
}

void ControllerSelector::select(const tms::Identity& id, Sec last_hb)
{
  ACE_DEBUG((LM_INFO, "(%P|%t) ControllerSelector::select: \"%C\"\n", id.c_str()));
  selected_ = id;
  schedule_once(MissedController{}, missed_controller_delay - last_hb);
  // TODO: Send ActiveMicrogridControllerState
}

DDS::ReturnCode_t PowerDevice::init(DDS::DomainId_t domain, int argc, char* argv[])
{
  DDS::ReturnCode_t rc = join_domain(domain, argc, argv);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  rc = create_subscribers(
    [&](const auto& di, const auto& si) { got_device_info(di, si); },
    [&](const auto& hb, const auto& si) { got_heartbeat(hb, si); });
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  rc = create_publishers();
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  auto di = get_device_info();
  di.role(role_);

  return send_device_info(di);
}

void PowerDevice::got_heartbeat(const tms::Heartbeat& hb, const DDS::SampleInfo& si)
{
  if (!si.valid_data || hb.deviceId() == device_id_) {
    return;
  }
  ACE_DEBUG((LM_INFO, "(%P|%t) Handshaking::power_device_got_heartbeat: from %C\n", hb.deviceId().c_str()));
  controller_selector_.got_heartbeat(hb);
}

void PowerDevice::got_device_info(const tms::DeviceInfo& di, const DDS::SampleInfo& si)
{
  if (!si.valid_data || di.deviceId() == device_id_) {
    return;
  }
  ACE_DEBUG((LM_INFO, "(%P|%t) Handshaking::power_device_got_device_info: from %C\n", di.deviceId().c_str()));
  controller_selector_.got_device_info(di);
}
