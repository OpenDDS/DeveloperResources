#ifndef CLI_ACTIVE_MICROGRID_CONTROLLER_STATE_DATA_READER_LISTENER_IMPL_H
#define CLI_ACTIVE_MICROGRID_CONTROLLER_STATE_DATA_READER_LISTENER_IMPL_H

#include "common/DataReaderListenerBase.h"
#include "CLIClient.h"

class ActiveMicrogridControllerStateDataReaderListenerImpl : public DataReaderListenerBase {
public:
  explicit ActiveMicrogridControllerStateDataReaderListenerImpl(CLIClient& cli_client)
    : DataReaderListenerBase("tms::ActiveMicrogridControllerState - DataReaderListenerImpl")
    , cli_client_(cli_client) {}

  virtual ~ActiveMicrogridControllerStateDataReaderListenerImpl() = default;

  void on_data_available(DDS::DataReader_ptr reader) final;

private:
  CLIClient& cli_client_;
};

#endif
