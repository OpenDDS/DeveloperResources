#include "EnergyStartStopStateDataReaderListenerImpl.h"

void EnergyStartStopStateDataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  tms::EnergyStartStopStateSeq data;
  DDS::SampleInfoSeq info_seq;
  tms::EnergyStartStopStateDataReader_var typed_reader =
    tms::EnergyStartStopStateDataReader::_narrow(reader);

  if (!typed_reader) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: EnergyStartStopStateDataReaderListenerImpl::on_data_available: "
               "_narrow failed\n"));
    return;
  }

  DDS::ReturnCode_t rc = typed_reader->take(data, info_seq, DDS::LENGTH_UNLIMITED,
                                            DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: EnergyStartStopStateDataReaderListenerImpl::on_data_available: "
               "take data failed: %C\n", OpenDDS::DCPS::retcode_to_string(rc)));
    return;
  }

  for (CORBA::ULong i = 0; i < data.length(); ++i) {
    if (info_seq[i].valid_data) {
      const tms::Identity& device_id = data[i].deviceId();
      const tms::EnergyStartStopLevel level = data[i].presentLevel();
      controller_.update_essl(device_id, level);
    }
  }
}
