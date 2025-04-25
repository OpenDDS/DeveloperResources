#ifndef CONTROLLER_COMMON_H
#define CONTROLLER_COMMON_H

#include <common/mil-std-3071_data_modelTypeSupportImpl.h>
#include <cli_idl/CLICommandsTypeSupportImpl.h>

#include <dds/DCPS/optional.h>

#include <unordered_map>

using OpArgPair = std::pair<std::string, OpenDDS::DCPS::optional<std::string>>;
using PowerDevices = std::unordered_map<tms::Identity, cli::PowerDeviceInfo>;

#endif
