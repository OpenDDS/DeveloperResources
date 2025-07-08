#include "ActiveMicrogridControllerStateDataReaderListenerImpl.h"

void ActiveMicrogridControllerStateDataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  tms::ActiveMicrogridControllerStateSeq data;
  DDS::SampleInfoSeq info_seq;
  tms::ActiveMicrogridControllerStateDataReader_var typed_reader = tms::ActiveMicrogridControllerStateDataReader::_narrow(reader);
  DDS::ReturnCode_t rc = typed_reader->take(data, info_seq, DDS::LENGTH_UNLIMITED,
                                            DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: ActiveMicrogridControllerStateDataReaderListenerImpl::on_data_available: "
               "take data failed: %C\n", OpenDDS::DCPS::retcode_to_string(rc)));
    return;
  }

  for (CORBA::ULong i = 0; i < data.length(); ++i) {
    if (info_seq[i].valid_data) {
      const tms::Identity& device_id = data[i].deviceId();
      auto master_id = data[i].masterId();
      cli_client_.set_active_controller(device_id, master_id);
    }
  }
}
