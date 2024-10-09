#ifndef TMS_TIMER_HANDLER_H
#define TMS_TIMER_HANDLER_H

#include <ace/Event_Handler.h>
#include <ace/Log_Msg.h>
#include <ace/Reactor.h>

#include <chrono>
#include <map>
#include <variant>
#include <mutex>
#include <type_traits>
#include <memory>
#include <stdexcept>
#include <typeinfo>

using Sec = std::chrono::seconds;
using Clock = std::chrono::system_clock;
using TimePoint = std::chrono::time_point<Clock>;
using TimerId = long;
constexpr TimerId null_timer_id = 0;
using Mutex = std::recursive_mutex;
using Guard = std::lock_guard<Mutex>;

template <typename EventType>
struct Timer {
  using Arg = EventType;
  using Ptr = std::shared_ptr<Timer<EventType>>;
  using Map = std::map<std::string, Ptr>;
  using MapPtr = std::shared_ptr<Map>;

  std::string name;
  TimerId id = null_timer_id;
  EventType arg = {};
  Sec period = Sec(0);
  Sec delay = Sec(0);
  bool exit_after = false;

  bool active() const
  {
    return id != null_timer_id;
  }

  std::string display_name() const
  {
    std::string rv = std::string("Timer<\"") + typeid(EventType).name() + "\">";
    if (name.size()) {
      rv += "\"" + name + "\"";
    }
    return rv;
  }
};

template <typename EventType>
class TimerHolder {
public:
  TimerHolder()
    : timers_(std::make_shared<typename Timer<EventType>::Map>())
  {
  }

  explicit operator typename Timer<EventType>::MapPtr()
  {
    return timers_;
  }

private:
  typename Timer<EventType>::MapPtr timers_;
};

/**
 * Wrapper around ACE_Event_Handler to make using a set of timers easier to
 * use.
 */
template <typename... EventTypes>
class TimerHandler : public ACE_Event_Handler, protected TimerHolder<EventTypes>...
{
public:
  using AnyTimer = std::variant<typename Timer<EventTypes>::Ptr...>;

  TimerHandler(ACE_Reactor* reactor = nullptr)
    : reactor_(reactor ? reactor : ACE_Reactor::instance())
  {
  }

  virtual ~TimerHandler()
  {
    cancel_all();
  }

  template <typename EventType>
  typename Timer<EventType>::Ptr get_timer(const std::string& name = "")
  {
    auto& timers = *get_timers<EventType>();
    const auto it = timers.find(name);
    if (it == timers.end()) {
      auto timer = std::make_shared<Timer<EventType>>();
      timer->name = name;
      timers[name] = timer;
      return timer;
    }
    return it->second;
  }

  template <typename EventType>
  void schedule(typename Timer<EventType>::Ptr timer)
  {
    Guard g(lock_);
    assert_inactive<EventType>(timer);
    const TimerId id = reactor_->schedule_timer(
      this, &timer->id, ACE_Time_Value(timer->delay), ACE_Time_Value(timer->period));
    timer->id = id;
    active_timers_[id] = timer;
  }

  template <typename EventType>
  void schedule(const std::string& name, EventType arg, Sec period, Sec delay = Sec(0))
  {
    Guard g(lock_);
    auto timer = this->get_timer<EventType>(name);
    assert_inactive<EventType>(timer);
    timer->name = name;
    timer->arg = arg;
    timer->period = period;
    timer->delay = delay;
    schedule<EventType>(timer);
  }

  template <typename EventType>
  void schedule_once(const std::string& name, EventType arg, Sec delay)
  {
    schedule(name, arg, Sec(0), delay);
  }

  template <typename EventType>
  void schedule(EventType arg, Sec period, Sec delay = Sec(0))
  {
    schedule("", arg, period, delay);
  }

  template <typename EventType>
  void schedule_once(EventType arg, Sec delay)
  {
    schedule(arg, Sec(0), delay);
  }

  template <typename EventType>
  void cancel(typename Timer<EventType>::Ptr timer)
  {
    if (!timer->active()) {
      return;
    }
    reactor_->cancel_timer(timer->id);
    timer_wont_run<EventType>(timer);
  }

  template <typename EventType>
  void cancel(const std::string& name = "")
  {
    Guard g(lock_);
    cancel<EventType>(this->get_timer<EventType>(name));
  }

  template <typename EventType>
  void reschedule(const std::string& name = "")
  {
    Guard g(lock_);
    auto timer = this->get_timer<EventType>(name);
    cancel<EventType>(timer);
    schedule<EventType>(timer);
  }

  void cancel_all()
  {
    Guard g(lock_);
    for (auto it = active_timers_.begin(); it != active_timers_.end(); ++it) {
      reactor_->cancel_timer(it->first);
      std::visit([&](auto&& timer) {
        timer->id = null_timer_id;
      }, it->second);
    }
    active_timers_.clear();
  }

  int handle_timeout(const ACE_Time_Value&, const void* arg)
  {
    Guard g(lock_);
    auto timer = active_timers_[*reinterpret_cast<const TimerId*>(arg)];
    any_timer_fired(timer);
    bool exit_after = false;
    std::visit([&](auto&& value) {
      if (!value->period.count()) {
        using EventType = typename std::remove_reference_t<decltype(value)>::element_type::Arg;
        timer_wont_run<EventType>(value);
      }
      exit_after = value->exit_after;
    }, timer);
    return end_event_loop(exit_after);
  }

protected:
  mutable Mutex lock_;
  ACE_Reactor* const reactor_;

  int end_event_loop(bool yes = true)
  {
    if (yes) {
      reactor_->end_reactor_event_loop();
      return -1;
    }
    return 0;
  }

  virtual void any_timer_fired(AnyTimer timer) = 0;

private:
  template <typename EventType>
  typename Timer<EventType>::MapPtr get_timers()
  {
    return static_cast<typename Timer<EventType>::MapPtr>(*this);
  }

  template <typename EventType>
  void assert_inactive(typename Timer<EventType>::Ptr timer)
  {
    if (timer->active()) {
      throw std::runtime_error("Can't schedule " + timer->display_name() + ", already active!");
    }
  }

  template <typename EventType>
  void timer_wont_run(typename Timer<EventType>::Ptr timer)
  {
    active_timers_.erase(timer->id);
    timer->id = null_timer_id;
  }

  std::map<TimerId, AnyTimer> active_timers_;
};

#endif
