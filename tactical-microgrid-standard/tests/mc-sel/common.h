#include <common/TimerHandler.h>
#include <common/Handshaking.h>

const int domain = 1;

struct Timeout {
  static const char* name() { return "Timeout"; }
};

class Exiter : public TimerHandler<Timeout> {
public:
  Exiter(Handshaking& handshaking, Sec exit_after = Sec(0))
  : TimerHandler(handshaking.get_reactor())
  {
    reactor_->register_handler(SIGINT, this);
    if (exit_after > Sec(0)) {
      schedule_once(Timeout{}, exit_after);
    }
  }

private:
  void timer_fired(Timer<Timeout>& t)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) Timeout\n"));
    exit(0);
  }

  void any_timer_fired(AnyTimer timer)
  {
    std::visit([&](auto&& value) { this->timer_fired(*value); }, timer);
  }

  int handle_signal(int, siginfo_t*, ucontext_t*)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) SIGINT\n"));
    exit(1);
    return -1;
  }
};
