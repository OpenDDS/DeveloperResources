#include "CLIServer.h"

// Assume the controller is active (i.e. actively sending heartbeats)
CLIServer::CLIServer(Controller& mc)
  : controller_(mc)
  , controller_active_(true)
{
  init();
}

void CLIServer::start_device(const OpArgPair& oparg) const
{
  // TODO:
  if (!oparg.second.has_value()) {
    std::cerr << "Device Id is required to start a device!" << std::endl;
    return;
  }

  const std::string& device_id = oparg.second.value();
  std::cout << "Sending EnergyStartStopRequest to start device Id: " << device_id << std::endl;
}

void CLIServer::stop_device(const OpArgPair& oparg) const
{
  // TODO:
  if (!oparg.second.has_value()) {
    std::cerr << "Device Id is required to stop a device!" << std::endl;
    return;
  }

  const std::string& device_id = oparg.second.value();
  std::cout << "Sending EnergyStartStopRequest to stop device Id: " << device_id << std::endl;
}

void CLIServer::stop_controller() const
{
  if (!controller_active_) {
    std::cout << "Controller is already stopped!" << std::endl;
    return;
  }

  controller_.stop_heartbeats();
  controller_active_ = false;
  std::cout << "Controller stopped!" << std::endl;
}

void CLIServer::resume_controller() const
{
  if (controller_active_) {
    std::cout << "Controller is already running!" << std::endl;
    return;
  }

  const DDS::ReturnCode_t rc = controller_.start_heartbeats();
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: CLI::resume_controller: start heartbeats thread failed\n"));
    return;
  }
  controller_active_ = true;
  std::cout << "Controller resumed!" << std::endl;
}

void CLIServer::terminate() const
{
  // TODO: gracefully terminate the controller
  std::cout << "Terminating the controller..." << std::endl;
}
