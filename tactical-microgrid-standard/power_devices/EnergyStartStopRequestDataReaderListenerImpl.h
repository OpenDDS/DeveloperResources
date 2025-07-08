#ifndef ENERGY_START_STOP_REQUEST_DATA_READER_LISTENER_IMPL_H
#define ENERGY_START_STOP_REQUEST_DATA_READER_LISTENER_IMPL_H

#include "common/DataReaderListenerBase.h"

class PowerDevice;

class EnergyStartStopRequestDataReaderListenerImpl : public DataReaderListenerBase {
public:
  explicit EnergyStartStopRequestDataReaderListenerImpl(PowerDevice& pwr_dev)
    : DataReaderListenerBase("tms::EnergyStartStopRequest - DataReaderListenerImpl")
    , power_device_(pwr_dev) {}

  virtual ~EnergyStartStopRequestDataReaderListenerImpl() = default;

  void on_data_available(DDS::DataReader_ptr reader) final;

private:
  PowerDevice& power_device_;
};

#endif
