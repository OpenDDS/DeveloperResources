#ifndef CONTROLLER_COMMON_H
#define CONTROLLER_COMMON_H

#include <common/mil-std-3071_data_modelTypeSupportImpl.h>
#include <cli_idl/CLICommandsTypeSupportImpl.h>

#include <unordered_map>
#include <optional>

using OpArgPair = std::pair<std::string, std::optional<std::string>>;
using PowerDevices = std::unordered_map<tms::Identity, cli::PowerDeviceInfo>;

#endif
