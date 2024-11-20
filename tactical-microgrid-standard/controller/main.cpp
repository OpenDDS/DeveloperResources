#include "Controller.h"
#include "CLIServer.h"

#include <ace/Get_Opt.h>

int main(int argc, char* argv[])
{
  DDS::DomainId_t domain_id = OpenDDS::DOMAIN_UNKNOWN;
  const char* mc_id = nullptr;

  ACE_Get_Opt get_opt(argc, argv, "i:d:");
  int c;
  while ((c = get_opt()) != -1) {
    switch (c) {
    case 'i':
      mc_id = get_opt.opt_arg();
      break;
    case 'd':
      domain_id = static_cast<DDS::DomainId_t>(ACE_OS::atoi(get_opt.opt_arg()));
      break;
    default:
      break;
    }
  }

  if (domain_id == OpenDDS::DOMAIN_UNKNOWN || mc_id == nullptr) {
    ACE_ERROR((LM_ERROR, "Usage: %C -d DDS_Domain_Id -i Microgrid_Controller_Id\n", argv[0]));
    return 1;
  }

  Controller controller(mc_id);
  controller.init(domain_id, argc, argv);
  CLIServer cli_server(controller);

  return controller.run();
}
