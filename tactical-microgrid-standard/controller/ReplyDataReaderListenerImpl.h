#ifndef REPLY_DATA_READER_LISTENER_IMPL_H
#define REPLY_DATA_READER_LISTENER_IMPL_H

#include "common/DataReaderListenerBase.h"
#include "CLIServer.h"

class ReplyDataReaderListenerImpl : public DataReaderListenerBase {
public:
  explicit ReplyDataReaderListenerImpl(CLIServer& cli_server) : cli_server_(cli_server) {}

  virtual ~ReplyDataReaderListenerImpl() = default;

  void on_data_available(DDS::DataReader_ptr reader) final;

private:
  CLIServer& cli_server_;
};

#endif
