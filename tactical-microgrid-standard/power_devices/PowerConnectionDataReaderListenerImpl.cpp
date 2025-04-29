#include "PowerConnectionDataReaderListenerImpl.h"

void PowerConnectionDataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  powersim::PowerConnectionSeq data;
  DDS::SampleInfoSeq info_seq;
  powersim::PowerConnectionDataReader_var typed_reader = powersim::PowerConnectionDataReader::_narrow(reader);
  const DDS::ReturnCode_t rc = typed_reader->take(data, info_seq, DDS::LENGTH_UNLIMITED,
                                                  DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: PowerConnectionDataReaderListenerImpl::on_data_available: "
               "take data failed: %C\n", OpenDDS::DCPS::retcode_to_string(rc)));
    return;
  }

  for (CORBA::ULong i = 0; i < data.length(); ++i) {
    if (info_seq[i].valid_data) {
      const powersim::PowerConnection& pc = data[i];
      if (pc.pd_id() != pd_.get_device_id()) {
        continue;
      }
      pd_.connected_devices(pc.connected_devices());
      break;
    }
  }
}