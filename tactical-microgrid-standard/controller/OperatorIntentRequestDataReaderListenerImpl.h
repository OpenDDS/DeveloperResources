#ifndef OPERATOR_INTENT_REQUEST_DATA_READER_LISTENER_IMPL_H
#define OPERATOR_INTENT_REQUEST_DATA_READER_LISTENER_IMPL_H

#include "common/DataReaderListenerBase.h"

class OperatorIntentRequestDataReaderListenerImpl : public DataReaderListenerBase {
public:
  OperatorIntentRequestDataReaderListenerImpl() {}
  virtual ~OperatorIntentRequestDataReaderListenerImpl() {}

  void on_data_available(DDS::DataReader_ptr reader) final;
};

#endif
