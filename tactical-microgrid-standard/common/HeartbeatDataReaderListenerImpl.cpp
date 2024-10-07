#include "idl/mil-std-3071_data_modelTypeSupportImpl.h"
#include "HeartbeatDataReaderListenerImpl.h"

void HeartbeatDataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  tms::HeartbeatDataReader_var hb_dr = tms::HeartbeatDataReader::_narrow(reader);
  if (!hb_dr) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: HeartbeatDataReaderListenerImpl::on_data_available: _narrow failed\n"));
    return;
  }

  while (true) {
    tms::Heartbeat heartbeat;
    DDS::SampleInfo si;
    const DDS::ReturnCode_t rc = hb_dr->take_next_sample(heartbeat, si);

    if (rc == DDS::RETCODE_OK) {
      if (callback_) {
        callback_(heartbeat, si);
      } else {
        ACE_DEBUG((LM_INFO, "(%P|%t) INFO: HeartbeatDataReaderListenerImpl::on_data_available: received heartbeat\n"));
      }
    } else if (rc == DDS::RETCODE_NO_DATA) {
      break;
    } else {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: HeartbeatDataReaderListenerImpl::on_data_available: take_next_sample failed (%C)\n",
                 OpenDDS::DCPS::retcode_to_string(rc)));
      break;
    }
  }
}
