#include "power_devices/PowerDevice.h"

#include <ace/Process_Manager.h>

#include <filesystem>
#include <string>
#include <map>
#include <vector>
#include <optional>

namespace fs = std::filesystem;

const int domain = 1;
const std::string device_id = "dev";

struct McStart {
  std::vector<std::string> ids;
  Sec run_for;
};
struct McSelected {
  std::string id;
  bool exit_after = false;
};
struct McStop {
  std::vector<pid_t> pids;
};
const std::string mc1 = "mc1";
const std::string mc2 = "mc2";
const std::string mc3 = "mc3";
class Test : public TimerHandler<McStart, McSelected, McStop> {
public:
  Test(PowerDevice& pd, const std::string& mc_path)
    : pd_(pd)
    , mc_path_(mc_path)
  {
    proc_man_.open(10, reactor_);
    reactor_->register_handler(SIGINT, this);

    /*
     * Time (s): 01234567890123456789012345678901234567890123456789012345678901234567890
     * Checks:             1    2         3                   4              5
     * selected:     [~~mc1~~~~~~~~~~][~mc2~~~~~~~~~~~~~~]            [-m1--~~~~~~~~~~~]
     * mc1:       <-------->                                       <------------->
     * mc2:            <----------------------->
     * mc3:            <----------------------->
     */
    schedule_once(mc1, McStart{{mc1}, Sec(10)}, Sec(1));
    schedule_once("mc2 & mc3", McStart{{mc2, mc3}, Sec(25)}, Sec(5));
    schedule_once("1st check", McSelected{mc1}, Sec(10));
    schedule_once("2nd check", McSelected{mc1}, Sec(15));
    schedule_once("3rd check", McSelected{mc2}, Sec(25));
    schedule_once("4th check", McSelected{""}, Sec(55));
    schedule_once("mc1 again", McStart{{mc1}, Sec(15)}, Sec(60));
    schedule_once("5th check", McSelected{mc1, /* exit_after = */ true}, Sec(70));
  }

  ~Test()
  {
    stop_all();
  }

  int exit_status()
  {
    return failed_;
  }

private:
  void stop_mc(pid_t pid)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) sigint %d\n", pid));
    if (pid != ACE_INVALID_PID) {
      proc_man_.terminate(pid, SIGINT);
    }
  }

  void stop_all()
  {
    for (auto& pair: procs_) {
      if (!pair.second.status) {
        stop_mc(pair.second.pid);
      }
    }
  }

  void timer_fired(Timer<McStart>& t)
  {
    std::vector<pid_t> pids;
    for (const auto& id : t.arg.ids) {
      ACE_Process_Options proc_opts;
      proc_opts.command_line("%s %d %s", mc_path_.c_str(), domain, id.c_str());
      Proc proc{proc_opts.command_line_buf()};
      proc.pid = proc_man_.spawn(proc_opts, this);
      if (proc.ran()) {
        ACE_ERROR((LM_ERROR, "(%P|%t) McStart: \"%C\" failed: %p\n", proc.cmd.c_str()));
        failed_ = true;
        t.exit_after = true;
        return;
      }
      ACE_DEBUG((LM_INFO, "(%P|%t) McStart: \"%C\" pid: %d\n", proc.cmd.c_str(), proc.pid));
      procs_[proc.pid] = proc;
      pids.push_back(proc.pid);
    }
    schedule_once(t.name, McStop{pids}, t.arg.run_for);
  }

  void timer_fired(Timer<McSelected>& t)
  {
    const auto sel = pd_.selected();
    ACE_DEBUG((LM_INFO, "(%P|%t) McSelected: expect: \"%C\" selected: \"%C\"\n", t.arg.id.c_str(), sel.c_str()));
    if (sel != t.arg.id) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: McSelected: failed\n"));
      failed_ = true;
      t.exit_after = true;
    } else {
      t.exit_after = t.arg.exit_after;
    }
  }

  void timer_fired(Timer<McStop>& t)
  {
    for (const pid_t pid : t.arg.pids) {
      stop_mc(pid);
    }
  }

  void any_timer_fired(AnyTimer timer)
  {
    std::visit([&](auto&& value) { this->timer_fired(*value); }, timer);
  }

  int handle_exit(ACE_Process* p)
  {
    Proc& proc = procs_.find(p->getpid())->second;
    proc.status = p->exit_code();
    if (*proc.status) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: \"%C\" exited with status %d\n",
        proc.cmd.c_str(), *proc.status));
      failed_ = true;
    } else {
      ACE_DEBUG((LM_INFO, "(%P|%t) \"%C\" exited\n", proc.cmd.c_str()));
    }
    return 0;
  }

  int handle_signal(int, siginfo_t*, ucontext_t*)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) SIGINT\n"));
    stop_all();
    return end_event_loop();
  }

  struct Proc {
    std::string cmd;
    pid_t pid = ACE_INVALID_PID;
    std::optional<ACE_exitcode> status;

    bool ran() const
    {
      return pid == ACE_INVALID_PID;
    }

    bool failed() const
    {
      return !ran() || status.value_or(1);
    }
  };

  bool failed_ = false;
  ACE_Process_Manager proc_man_;
  std::map<pid_t, Proc> procs_;
  PowerDevice& pd_;
  const std::string mc_path_;
};

int main(int argc, char* argv[])
{
  const auto our_path = fs::path(argv[0]);
  const auto mc_path = our_path.parent_path() / ("basic-mc" + our_path.extension().string());

  PowerDevice pd(device_id);
  if (pd.init(domain, argc, argv) != DDS::RETCODE_OK) {
    return 1;
  }

  Test test(pd, mc_path.string());
  if (ACE_Reactor::instance()->run_reactor_event_loop() != 0) {
    return 1;
  }

  return test.exit_status();
}
