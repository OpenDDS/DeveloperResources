#ifndef TMS_COMMON_TIMER_HANDLER_H
#define TMS_COMMON_TIMER_HANDLER_H

#include <ace/Event_Handler.h>
#include <ace/Log_Msg.h>
#include <ace/Reactor.h>
#include <ace/Recursive_Thread_Mutex.h>
#include <ace/Timer_Hash.h>

#include <chrono>
#include <map>
#include <variant>
#include <mutex>
#include <type_traits>
#include <memory>
#include <stdexcept>
#include <iostream>

using Sec = std::chrono::duration<double>;
using Clock = std::chrono::system_clock;
using TimePoint = std::chrono::time_point<Clock>;
using TimerId = long;
using TimerKey = std::string;
constexpr TimerId null_timer_id = 0;
using Mutex = std::recursive_mutex;
using Guard = std::lock_guard<Mutex>;

// Workaround https://github.com/DOCGroup/ACE_TAO/pull/2462
template<class Rep, class Period>
inline ACE_Time_Value to_time_value(const std::chrono::duration<Rep, Period>& duration)
{
  using namespace std::chrono;

  const seconds s{duration_cast<seconds>(duration)};
  const microseconds usec{duration_cast<microseconds>(duration  - s)};
  return ACE_Time_Value(ACE_Utils::truncate_cast<time_t>(s.count()),
    ACE_Utils::truncate_cast<suseconds_t>(usec.count()));
}

// EventType must implement: static const char* name();
template <typename EventType>
struct Timer {
  using Arg = EventType;
  using Ptr = std::shared_ptr<Timer<EventType>>;
  using Map = std::map<std::string, Ptr>;
  using MapPtr = std::shared_ptr<Map>;

  std::string name;
  TimerId id = null_timer_id;

  // Timer Ids returned by ACE are not unique and can cause issue.
  // E.g., MissedHeartbeat timer is scheduled once. When it expires,
  // LostController is scheduled and the returned id may be the same as the one
  // associated with the MissedHeartbeat timer. handle_timeout() then adds a new
  // entry for LostController keyed with that one timer Id. It then erases the
  // entry for MissedHeartbeat, which used the same exact timer Id as its key.
  // Consequently, the newly created entry for LostController is deleted instead.
  // This fixes that with a string key constructed in activate().
  TimerKey key;

  EventType arg = {};
  Sec period = Sec(0);
  Sec delay = Sec(0);
  bool exit_after = false;

  bool active() const
  {
    return id != null_timer_id;
  }

  void activate(TimerId a_id)
  {
    id = a_id;
    key = std::to_string(id) + "_" + EventType::name();
  }

  void deactivate()
  {
    id = null_timer_id;
    key.clear();
  }

  std::string display_name() const
  {
    std::string rv = std::string("Timer<\"") + EventType::name() + "\">";
    if (name.size()) {
      rv += "\"" + name + "\"";
    }
    return rv;
  }

  void display() const
  {
    std::cout << "{\n"
      << "  name: \"" << name << "\"\n"
      << "  id: " << id << "\n"
      << "  key: \"" << key << "\"\n"
      << "  period: " << period.count() << "\n"
      << "  delay: " << delay.count() << "\n"
      << "  exit_after: " << (exit_after ? "true" : "false") << "\n"
      << "}"
      << std::endl;
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

  // Create a new ACE_Reactor with ACE_Timer_Hash if @a reactor is null.
  // Otherwise, use the provided reactor.
  explicit TimerHandler(ACE_Reactor* reactor = nullptr)
    : reactor_(reactor)
  {
    if (!reactor) {
      reactor_ = new ACE_Reactor;

      // We had an issue with using ACE_Reactor's default timer queue, which is
      // ACE_Timer_Heap, when the rate of timer creation and cancellation is high
      // for detecting missed heartbeat deadline from microgrid controllers.
      // ACE_Timer_Hash seems working okay.
      timer_queue_ = new ACE_Timer_Hash;
      reactor_->timer_queue(timer_queue_);
      own_reactor_ = true;
    }
  }

  virtual ~TimerHandler()
  {
    cancel_all();

    if (own_reactor_) {
      delete timer_queue_;
      delete reactor_;
    }
  }

  ACE_Reactor* get_reactor() const
  {
    Guard g(lock_);
    return reactor_;
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
      this, &timer->key, to_time_value(timer->delay), to_time_value(timer->period));
    timer->activate(id);
    active_timers_[timer->key] = timer;
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
      std::visit([&](auto&& timer) {
        reactor_->cancel_timer(timer->id);
        timer->deactivate();
      }, it->second);
    }
    active_timers_.clear();
  }

  int handle_timeout(const ACE_Time_Value&, const void* arg)
  {
    Guard g(lock_);
    auto key = *reinterpret_cast<const std::string*>(arg);
    ACE_DEBUG((LM_DEBUG, "(%P|%t) TimerHandler::handle_timeout: %C\n", key.c_str()));
    if (active_timers_.count(key) == 0) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: TimerHandler::handle_timeout: timer key %C does NOT exist\n", key.c_str()));
    }

    auto timer = active_timers_[key];
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
  ACE_Reactor* reactor_;

  // Allow using non-default timer queue
  ACE_Timer_Queue* timer_queue_ = nullptr;
  bool own_reactor_ = false;

  int end_event_loop(bool yes = true)
  {
    if (yes) {
      reactor_->end_reactor_event_loop();
      return -1;
    }
    return 0;
  }

  virtual void any_timer_fired(AnyTimer timer) = 0;

  // For debugging purposes. Caller must already hold lock_.
  void display_active_timers(const std::string& preamble) const
  {
    std::cout << preamble;
    for (const auto& pair : active_timers_) {
      const auto timer = pair.second;
      std::visit([&](auto&& value) {
        value->display();
      }, timer);
    }
  }

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
    active_timers_.erase(timer->key);
    timer->deactivate();
  }

  std::map<TimerKey, AnyTimer> active_timers_;
};

#endif
