#ifndef OPERATOR_INTENT_REQUEST_DATA_READER_LISTENER_IMPL_H
#define OPERATOR_INTENT_REQUEST_DATA_READER_LISTENER_IMPL_H

#include "common/DataReaderListenerBase.h"
#include "CLIServer.h"

class OperatorIntentRequestDataReaderListenerImpl : public DataReaderListenerBase {
public:
  explicit OperatorIntentRequestDataReaderListenerImpl(CLIServer& cli_server)
    : DataReaderListenerBase("tms::OperatorIntentRequest - DataReaderListenerImpl")
    , cli_server_(cli_server) {}

  virtual ~OperatorIntentRequestDataReaderListenerImpl() = default;

  void on_data_available(DDS::DataReader_ptr reader) final;

private:
  CLIServer& cli_server_;
};

#endif
