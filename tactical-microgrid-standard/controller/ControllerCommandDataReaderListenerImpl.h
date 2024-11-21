#ifndef CONTROLLER_COMMAND_DATA_READER_LISTENER_IMPL_H
#define CONTROLLER_COMMAND_DATA_READER_LISTENER_IMPL_H

#include "common/DataReaderListenerBase.h"
#include "CLIServer.h"

class ControllerCommandDataReaderListenerImpl : public DataReaderListenerBase {
public:
  explicit ControllerCommandDataReaderListenerImpl(CLIServer& cli_server) : cli_server_(cli_server) {}

  virtual ~ControllerCommandDataReaderListenerImpl() = default;

  void on_data_available(DDS::DataReader_ptr reader) final;

private:
  CLIServer& cli_server_;
};

#endif
