#include "CLI.h"

void CLI::run()
{
  display_commands();
  std::string line;
  while (true) {
    std::getline(std::cin, line);
    const auto op_pair = parse(line);
    if (op_pair.first == "L") {
      const auto pd = controller_.power_devices();
      display_power_devices(pd);
    } else if (op_pair.first == "E") {
      start_device(op_pair);
    } else if (op_pair.first == "D") {
      stop_device(op_pair);
    } else if (op_pair.first == "S") {
      stop_controller();
    } else if (op_pair.first == "R") {
      resume_controller();
    } else if (op_pair.first == "T") {
      terminate();
    } else {
      std::cout << "Unknown operation entered!\n" << std::endl;
    }
    display_commands();
  }
}

void CLI::display_commands() const
{
  std::cout << "=== CLI for Controller: " << controller_.id() << std::endl;
  std::cout << "[L]ist connected power devices" << std::endl;
  std::cout << "[E]nable (start) a source device with Id" << std::endl;
  std::cout << "[D]isable (stop) a source device with Id" << std::endl;
  std::cout << "[S]top the controller" << std::endl;
  std::cout << "[R]esume the controller" << std::endl;
  std::cout << "[T]erminate the controller" << std::endl;
  std::cout << "Enter an operation: ";
}

std::string CLI::device_role_to_string(tms::DeviceRole role) const
{
  switch (role) {
  case tms::DeviceRole::ROLE_MICROGRID_CONTROLLER:
    return "Microgrid Controller";
  case tms::DeviceRole::ROLE_SOURCE:
    return "Source";
  case tms::DeviceRole::ROLE_LOAD:
    return "Load";
  case tms::DeviceRole::ROLE_STORAGE:
    return "Storage";
  case tms::DeviceRole::ROLE_DISTRIBUTION:
    return "Distribution";
  case tms::DeviceRole::ROLE_MICROGRID_DASHBOARD:
    return "Microgrid Dashboard";
  case tms::DeviceRole::ROLE_CONVERSION:
    return "Conversion";
  case tms::DeviceRole::ROLE_MONITOR:
    return "Monitor";
  default:
    return "Unknown";
  }
}

void CLI::display_power_devices(const Controller::PowerDevices& pd) const
{
  std::cout << "Number of connected power devices: " << pd.size() << std::endl;
  for (auto it = pd.begin(); it != pd.end(); ++it) {
    std::cout << "Device Id: " << it->first <<
      ". Type: " << device_role_to_string(it->second.role()) << std::endl;
  }
}

void CLI::start_device(const OpArgPair& oparg) const
{
  // TODO: EnergyStartStopRequest
}

void CLI::stop_device(const OpArgPair& oparg) const
{
  // TODO: EnergyStartStopRequest
}

void CLI::stop_controller() const
{
  controller_.stop_heartbeats();
}

void CLI::resume_controller() const
{
  const DDS::ReturnCode_t rc = controller_.start_heartbeats();
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: CLI::resume_controller: start heartbeats thread failed\n"));
  }
}

void CLI::terminate() const
{
  // TODO: gracefully terminate the controller
}

OpArgPair CLI::parse(const std::string& input) const
{
  const std::string whitespace = " \t";
  const size_t first = input.find_first_not_of(whitespace);
  if (first == std::string::npos) {
    return std::make_pair("", OpenDDS::DCPS::optional<std::string>());
  }

  const size_t last = input.find_last_not_of(whitespace);
  const std::string trimed_input = input.substr(first, last - first + 1);
  const size_t delim = trimed_input.find_first_of(whitespace);
  if (delim == std::string::npos) {
    return std::make_pair(trimed_input, OpenDDS::DCPS::optional<std::string>());
  }

  const std::string op = trimed_input.substr(0, delim);
  const size_t arg_begin = trimed_input.find_first_not_of(whitespace, delim + 1);
  const std::string arg = trimed_input.substr(arg_begin);
  return std::make_pair(op, OpenDDS::DCPS::optional<std::string>(arg));
}
