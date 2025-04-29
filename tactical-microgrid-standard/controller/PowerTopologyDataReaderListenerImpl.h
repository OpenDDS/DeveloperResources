#ifndef POWER_TOPOLOGY_DATA_READER_LISTENER_IMPL_H
#define POWER_TOPOLOGY_DATA_READER_LISTENER_IMPL_H

#include "common/DataReaderListenerBase.h"
#include "CLIServer.h"

class PowerTopologyDataReaderListenerImpl : public DataReaderListenerBase {
public:
  explicit PowerTopologyDataReaderListenerImpl(CLIServer& cli_server) : cli_server_(cli_server) {}

  virtual ~PowerTopologyDataReaderListenerImpl() = default;

  void on_data_available(DDS::DataReader_ptr reader) final;

private:
  CLIServer& cli_server_;
};

#endif