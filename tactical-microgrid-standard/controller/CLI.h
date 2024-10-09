#ifndef CONTROLLER_CLI_H
#define CONTROLLER_CLI_H

#include "Controller.h"

#include <dds/DCPS/optional.h>

// May open a port for the CLI so that the controller logs
// does not interfere with the CLI output.
class CLI {
public:
  CLI(Controller& mc) : controller_(mc) {}

  void run();

private:
  using OpArgPair = std::pair<std::string, OpenDDS::DCPS::optional<std::string>>;

  void display_commands() const;

  std::string device_role_to_string(tms::DeviceRole role) const;

  void display_power_devices(const Controller::PowerDevices& pd) const;

  void start_device(const OpArgPair& oparg) const;

  void stop_device(const OpArgPair& oparg) const;

  void stop_controller() const;

  void resume_controller() const;

  void terminate() const;

  OpArgPair parse(const std::string& input) const;

private:
  Controller& controller_;
};

#endif CONTROLLER_CLI_H
