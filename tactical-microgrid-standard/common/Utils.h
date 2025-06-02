#ifndef TMS_COMMON_UTILS_H
#define TMS_COMMON_UTILS_H

#include <common/mil-std-3071_data_modelTypeSupportImpl.h>

#include <string>

namespace Utils {

DDS::DomainId_t get_sim_domain_id(DDS::DomainId_t tms_domain_id)
{
  return (tms_domain_id < ACE_INT32_MAX) ? tms_domain_id + 1 : tms_domain_id - 1;
}

std::string device_role_to_string(tms::DeviceRole role)
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

}

#endif