#include "ControllerCommandDataReaderListenerImpl.h"

void ControllerCommandDataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  cli::ControllerCommandSeq data;
  DDS::SampleInfoSeq info_seq;
  cli::ControllerCommandDataReader_var typed_reader = cli::ControllerCommandDataReader::_narrow(reader);
  DDS::ReturnCode_t rc = typed_reader->take(data, info_seq, DDS::LENGTH_UNLIMITED,
                                            DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: ControllerCommandDataReaderListenerImpl::on_data_available: "
               "take data failed: %C\n", OpenDDS::DCPS::retcode_to_string(rc)));
    return;
  }

  Controller& mc = cli_server_.get_controller();
  bool found_valid_cmd = false;
  cli::ControllerCmdType cct;
  for (CORBA::ULong i = 0; i < data.length(); ++i) {
    if (info_seq[i].valid_data && data[i].mc_id() == mc.id()) {
      cct = data[i].type();
      found_valid_cmd = true;
      break;
    }
  }

  if (found_valid_cmd) {
    switch (cct) {
    case cli::ControllerCmdType::CCT_STOP:
      mc.stop_heartbeats();
      break;
    case cli::ControllerCmdType::CCT_RESUME:
      mc.start_heartbeats();
      break;
    default:
      mc.terminate();
      break;
    }
  }
}
