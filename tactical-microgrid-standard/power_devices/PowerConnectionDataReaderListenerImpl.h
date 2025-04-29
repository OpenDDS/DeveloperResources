#ifndef POWER_CONNECTION_DATA_READER_LISTENER_IMPL_H
#define POWER_CONNECTION_DATA_READER_LISTENER_IMPL_H

#include "common/DataReaderListenerBase.h"
#include "PowerDevice.h"

class PowerConnectionDataReaderListenerImpl : public DataReaderListenerBase {
public:
  explicit PowerConnectionDataReaderListenerImpl(PowerDevice& pd) : pd_(pd) {}

  virtual ~PowerConnectionDataReaderListenerImpl() = default;

  void on_data_available(DDS::DataReader_ptr reader) final;

private:
  PowerDevice& pd_;
};

#endif