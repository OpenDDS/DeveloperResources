#ifndef TMS_CONTROLLER_H
#define TMS_CONTROLLER_H

#include "Handshaking.h"

#include <mil-std-3071_data_modelTypeSupportImpl.h>
#include <opendds_tms_export.h>

class opendds_tms_Export Controller : public Handshaking {
public:
  Controller(tms::Identity id)
    : Handshaking(id)
  {
  }

  DDS::ReturnCode_t init(DDS::DomainId_t domain, int argc = 0, char* argv[] = nullptr);

private:
  void got_heartbeat(const tms::Heartbeat& hb, const DDS::SampleInfo& si);
  void got_device_info(const tms::DeviceInfo& di, const DDS::SampleInfo& si);
};

#endif