#include "common.h"

#include <controller/Controller.h>

int main(int argc, char* argv[])
{
  if (argc < 3) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: requires device id and priority time duration arguments\n"));
    return 1;
  }
  const std::string device_id = argv[1];
  const uint16_t priority = std::stoi(argv[2]);

  Controller mc(device_id, priority);
  if (mc.init(domain, argc, argv) != DDS::RETCODE_OK) {
    return 1;
  }

  Exiter exiter(mc);
  return mc.run();
}
