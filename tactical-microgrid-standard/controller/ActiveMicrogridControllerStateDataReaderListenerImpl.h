#ifndef ACTIVE_MICROGRID_CONTROLLER_STATE_DATA_READER_LISTENER_IMPL_H
#define ACTIVE_MICROGRID_CONTROLLER_STATE_DATA_READER_LISTENER_IMPL_H

#include "common/DataReaderListenerBase.h"
#include "Controller.h"

class ActiveMicrogridControllerStateDataReaderListenerImpl : public DataReaderListenerBase {
public:
  explicit ActiveMicrogridControllerStateDataReaderListenerImpl(Controller& controller)
    : DataReaderListenerBase("tms::ActiveMicrogridControllerState - DataReaderListenerImpl")
    , controller_(controller) {}

  virtual ~ActiveMicrogridControllerStateDataReaderListenerImpl() = default;

  void on_data_available(DDS::DataReader_ptr reader) final;

private:
  Controller& controller_;
};

#endif
