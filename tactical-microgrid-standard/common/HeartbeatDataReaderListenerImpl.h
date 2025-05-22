#ifndef TMS_COMMON_HEARTBEAT_DATA_READER_LISTENER_IMPL_H
#define TMS_COMMON_HEARTBEAT_DATA_READER_LISTENER_IMPL_H

#include "DataReaderListenerBase.h"

#include <functional>

class HeartbeatDataReaderListenerImpl : public DataReaderListenerBase {
public:
  explicit HeartbeatDataReaderListenerImpl(std::function<void(const tms::Heartbeat&, const DDS::SampleInfo&)> cb = nullptr)
    : DataReaderListenerBase("tms::Heartbeat - DataReaderListenerImpl")
    , callback_(cb)
  {}

  virtual ~HeartbeatDataReaderListenerImpl() = default;

  void on_data_available(DDS::DataReader_ptr reader) final;

private:
  std::function<void(const tms::Heartbeat&, const DDS::SampleInfo&)> callback_;
};

#endif
