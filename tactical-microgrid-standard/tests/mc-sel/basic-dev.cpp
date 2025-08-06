#include "common.h"

#include <power_devices/PowerDevice.h>

#include <tests/Utils/DistributedConditionSet.h>

int main(int argc, char* argv[])
{
  const std::string name = "dev";
  PowerDevice pd(name);
  if (pd.init(domain, argc, argv) != DDS::RETCODE_OK) {
    return 1;
  }

  DistributedConditionSet_rch dcs =
    OpenDDS::DCPS::make_rch<FileBasedDistributedConditionSet>();

  ControllerCallbacks& cbs = pd.controller_callbacks();
  cbs.set_new_controller_callback([dcs, name](const tms::Identity& id) {
    printf("new_controller\n");
    dcs->post(name, "new controller " + id);
  });
  cbs.set_missed_heartbeat_callback([dcs, name](const tms::Identity& id) {
    dcs->post(name, "missed controller " + id);
  });
  cbs.set_lost_controller_callback([dcs, name](const tms::Identity& id) {
    dcs->post(name, "lost controller " + id);
  });
  cbs.set_no_controllers_callback([dcs, name]() {
    dcs->post(name, "no controllers");
  });

  Exiter exiter(pd);
  return pd.run();
}
