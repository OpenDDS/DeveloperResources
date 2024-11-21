#include <controller/Controller.h>

struct Timeout {};
class Test : public TimerHandler<Timeout> {
public:
  Test()
  {
    reactor_->register_handler(SIGINT, this);
    schedule(Timeout{}, Sec(0), Sec(30));
  }

private:
  void timer_fired(Timer<Timeout>& t)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) Timeout\n"));
    t.exit_after = true;
  }

  void any_timer_fired(AnyTimer timer)
  {
    std::visit([&](auto&& value) { this->timer_fired(*value); }, timer);
  }

  int handle_signal(int, siginfo_t*, ucontext_t*)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) SIGINT\n"));
    return end_event_loop();
  }
};

int main(int argc, char* argv[])
{
  const int domain = std::stoi(argv[1]);
  const std::string device_id = argv[2];

  Controller mc(device_id);
  if (mc.init(domain, argc, argv) != DDS::RETCODE_OK) {
    return 1;
  }

  Test test;
  return mc.run();
}
