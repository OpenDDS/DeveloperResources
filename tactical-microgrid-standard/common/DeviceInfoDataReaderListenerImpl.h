#ifndef TMS_COMMON_DEVICE_INFO_DATA_READER_LISTENER_IMPL_H
#define TMS_COMMON_DEVICE_INFO_DATA_READER_LISTENER_IMPL_H

#include "DataReaderListenerBase.h"

#include <functional>

class DeviceInfoDataReaderListenerImpl : public DataReaderListenerBase {
public:
  explicit DeviceInfoDataReaderListenerImpl(std::function<void(const tms::DeviceInfo&, const DDS::SampleInfo&)> cb = nullptr)
    : DataReaderListenerBase("tms::DeviceInfo - DataReaderListenerImpl")
    , callback_(cb)
  {}

  virtual ~DeviceInfoDataReaderListenerImpl() = default;

  void on_data_available(DDS::DataReader_ptr reader) final;

private:
  std::function<void(const tms::DeviceInfo&, const DDS::SampleInfo&)> callback_;
};

#endif
