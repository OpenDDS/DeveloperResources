#include "Controller.h"
#include "CLIServer.h"

int main(int argc, char* argv[])
{
  if (argc < 5) {
    ACE_ERROR((LM_ERROR, "main: Must specify Domain Id and Device Id\n"));
    return 1;
  }

  DDS::DomainId_t domain_id = OpenDDS::DOMAIN_UNKNOWN;
  std::string mc_id;

  for (int i = 1; i < argc; ++i) {
    const std::string arg(argv[i++]);
    if (arg == "-DomainId") {
      domain_id = std::stoi(argv[i]);
    } else if (arg == "-DeviceId") {
      mc_id = argv[i];
    }
  }

  Controller controller(mc_id);
  controller.run(domain_id, argc, argv);

  CLIServer cli_server(controller);

  return 0;
}
