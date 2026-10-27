#ifndef ENERGY_START_STOP_STATE_DATA_READER_LISTENER_IMPL_H
#define ENERGY_START_STOP_STATE_DATA_READER_LISTENER_IMPL_H

#include "common/DataReaderListenerBase.h"
#include "Controller.h"

class EnergyStartStopStateDataReaderListenerImpl : public DataReaderListenerBase {
public:
  explicit EnergyStartStopStateDataReaderListenerImpl(Controller& controller)
    : DataReaderListenerBase("tms::EnergyStartStopState - DataReaderListenerImpl")
    , controller_(controller) {}

  virtual ~EnergyStartStopStateDataReaderListenerImpl() = default;

  void on_data_available(DDS::DataReader_ptr reader) final;

private:
  Controller& controller_;
};

#endif // ENERGY_START_STOP_STATE_DATA_READER_LISTENER_IMPL_H
