#include "CLIClient.h"

#include <ace/Get_Opt.h>

int main(int argc, char* argv[])
{
  const char* id = "CLI Client";
  DDS::DomainId_t tms_domain_id = OpenDDS::DOMAIN_UNKNOWN;

  ACE_Get_Opt get_opt(argc, argv, "i:d:");
  int c;
  while ((c = get_opt()) != -1) {
    switch (c) {
    case 'i':
      id = get_opt.opt_arg();
      break;
    case 'd':
      tms_domain_id = static_cast<DDS::DomainId_t>(ACE_OS::atoi(get_opt.opt_arg()));
      break;
    default:
      break;
    }
  }

  if (tms_domain_id == OpenDDS::DOMAIN_UNKNOWN) {
    ACE_ERROR((LM_ERROR, "Usage: %C -d TMS_Domain_Id [-i CLI_Client_Id]\n", argv[0]));
    return 1;
  }

  CLIClient client(id);
  if (client.init(tms_domain_id, argc, argv) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: CLIMain - Initialize CLI client failed\n"));
    return 1;
  }

  client.run();

  return 0;
}
