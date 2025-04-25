#include "ReplyDataReaderListenerImpl.h"

void ReplyDataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  tms::ReplySeq data;
  DDS::SampleInfoSeq info_seq;
  tms::ReplyDataReader_var typed_reader = tms::ReplyDataReader::_narrow(reader);
  DDS::ReturnCode_t rc = typed_reader->take(data, info_seq, DDS::LENGTH_UNLIMITED,
                                            DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: ReplyDataReaderListenerImpl::on_data_available: "
               "take data failed: %C\n", OpenDDS::DCPS::retcode_to_string(rc)));
    return;
  }

  for (CORBA::ULong i = 0; i < data.length(); ++i) {
    if (info_seq[i].valid_data) {
      const tms::Reply& reply = data[i];
      const tms::Identity& mc_id = reply.requestingDeviceId();
      if (mc_id != cli_server_.get_controller().id()) {
        continue;
      }
      cli_server_.receive_reply(reply);
    }
  }
}