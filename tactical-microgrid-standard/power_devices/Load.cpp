#include "common/PowerDevice.h"

#include <ace/Get_Opt.h>

class LoadDevice : public PowerDevice {
public:
  explicit LoadDevice(const tms::Identity& id)
    : PowerDevice(id, tms::DeviceRole::ROLE_LOAD)
  {
  }

  int run()
  {
    return reactor_->run_reactor_event_loop() == 0 ? 0 : 1;
  }
};

int main(int argc, char* argv[])
{
  DDS::DomainId_t domain_id = OpenDDS::DOMAIN_UNKNOWN;
  const char* load_id = nullptr;

  ACE_Get_Opt get_opt(argc, argv, "i:d:");
  int c;
  while ((c = get_opt()) != -1) {
    switch (c) {
    case 'i':
      load_id = get_opt.opt_arg();
      break;
    case 'd':
      domain_id = static_cast<DDS::DomainId_t>(ACE_OS::atoi(get_opt.opt_arg()));
      break;
    default:
      break;
    }
  }

  if (domain_id == OpenDDS::DOMAIN_UNKNOWN || load_id == nullptr) {
    ACE_ERROR((LM_ERROR, "Usage: %C -d DDS_Domain_Id -i Load_Device_Id\n", argv[0]));
    return 1;
  }

  LoadDevice load_dev(load_id);
  load_dev.init(domain_id, argc, argv);
  return load_dev.run();
}