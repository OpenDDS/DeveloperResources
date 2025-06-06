#ifndef POWER_DEVICES_REQUEST_DATA_READER_LISTENER_IMPL_H
#define POWER_DEVICES_REQUEST_DATA_READER_LISTENER_IMPL_H

#include "CLIServer.h"
#include "common/DataReaderListenerBase.h"

class PowerDevicesRequestDataReaderListenerImpl : public DataReaderListenerBase {
public:
  explicit PowerDevicesRequestDataReaderListenerImpl(CLIServer& cli_server)
    : DataReaderListenerBase("cli::PowerDevicesRequest - DataReaderListenerImpl")
    , cli_server_(cli_server) {}

  virtual ~PowerDevicesRequestDataReaderListenerImpl() = default;

  void on_data_available(DDS::DataReader_ptr reader) final;

private:
  CLIServer& cli_server_;
};

#endif
