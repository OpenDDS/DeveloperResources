#include "OperatorIntentRequestDataReaderListenerImpl.h"

void OperatorIntentRequestDataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  tms::OperatorIntentRequestSeq data;
  DDS::SampleInfoSeq info_seq;
  tms::OperatorIntentRequestDataReader_var typed_reader = tms::OperatorIntentRequestDataReader::_narrow(reader);
  DDS::ReturnCode_t rc = typed_reader->take(data, info_seq, DDS::LENGTH_UNLIMITED,
                                            DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: OperatorIntentRequestDataReaderListenerImpl::on_data_available: "
               "take data failed: %C\n", OpenDDS::DCPS::retcode_to_string(rc)));
    return;
  }

  const tms::Identity& mc_id = cli_server_.get_controller().id();

  tms::Identity target_device;
  tms::OperatorPriorityType opt;
  for (CORBA::ULong i = 0; i < data.length(); ++i) {
    if (info_seq[i].valid_data) {
      const tms::OperatorIntent& oi = data[i].desiredOperatorIntent();
      if (oi.requestId().requestingDeviceId() == mc_id) {
        for (size_t i = 0; i < oi.devices().size(); ++i) {
          const tms::Identity& device_id = oi.devices()[i].deviceId();
          if (!device_id.empty()) {
            target_device = device_id;
            opt = oi.devices()[i].priority().priorityType();
            break;
          }
        }
      }
    }
  }

  if (!target_device.empty()) {
    cli_server_.start_stop_device(target_device, opt);
  }
}
