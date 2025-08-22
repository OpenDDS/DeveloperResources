#include <common/TimerHandler.h>
#include <common/Handshaking.h>

const int domain = 1;

struct Timeout {
  bool exit_after = true;
  static const char* name() { return "Timeout"; }
};

class Exiter : public TimerHandler<Timeout> {
public:
  Exiter(Handshaking& handshaking, Sec exit_after = Sec(0))
  : TimerHandler(handshaking.get_reactor())
  , handshaking_(handshaking)
  {
    reactor_->register_handler(SIGINT, this);
    if (exit_after > Sec(0)) {
      schedule_once(Timeout{}, exit_after);
    }
  }

private:
  Handshaking& handshaking_;

  void shutdown()
  {
    handshaking_.delete_all_entities();
    TheServiceParticipant->shutdown();
  }

  void timer_fired(Timer<Timeout>& t)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) Timeout\n"));
    shutdown();
  }

  void any_timer_fired(AnyTimer timer)
  {
    std::visit([&](auto&& value) { this->timer_fired(*value); }, timer);
  }

  int handle_signal(int, siginfo_t*, ucontext_t*)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) SIGINT\n"));
    shutdown();
    reactor_->end_reactor_event_loop();
    return -1;
  }
};
