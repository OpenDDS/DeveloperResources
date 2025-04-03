#include <common/mil-std-3071_data_modelTypeSupportImpl.h>
#include "DeviceInfoDataReaderListenerImpl.h"

void DeviceInfoDataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  tms::DeviceInfoDataReader_var di_dr = tms::DeviceInfoDataReader::_narrow(reader);
  if (!di_dr) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DeviceInfoDataReaderListenerImpl::on_data_available: _narrow failed\n"));
    return;
  }

  while (true) {
    tms::DeviceInfo device_info;
    DDS::SampleInfo si;
    const DDS::ReturnCode_t rc = di_dr->take_next_sample(device_info, si);

    if (rc == DDS::RETCODE_OK) {
      if (callback_) {
        callback_(device_info, si);
      } else {
        ACE_DEBUG((LM_INFO, "(%P|%t) INFO: DeviceInfoDataReaderListenerImpl::on_data_available: received device info\n"));
      }
    } else if (rc == DDS::RETCODE_NO_DATA) {
      break;
    } else {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DeviceInfoDataReaderListenerImpl::on_data_available: take_next_sample failed (%C)\n",
                 OpenDDS::DCPS::retcode_to_string(rc)));
      break;
    }
  }
}
