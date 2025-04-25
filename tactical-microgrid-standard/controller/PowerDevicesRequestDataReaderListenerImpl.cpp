#include "PowerDevicesRequestDataReaderListenerImpl.h"

void PowerDevicesRequestDataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  cli::PowerDevicesRequestSeq data;
  DDS::SampleInfoSeq info_seq;
  cli::PowerDevicesRequestDataReader_var typed_reader = cli::PowerDevicesRequestDataReader::_narrow(reader);
  DDS::ReturnCode_t rc = typed_reader->take(data, info_seq, DDS::LENGTH_UNLIMITED,
                                            DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: PowerDevicesRequestDataReaderListenerImpl::on_data_available: "
               "take data failed: %C\n", OpenDDS::DCPS::retcode_to_string(rc)));
    return;
  }

  const Controller& mc = cli_server_.get_controller();
  const tms::Identity id = mc.id();

  bool my_request = false;
  for (CORBA::ULong i = 0; i < data.length(); ++i) {
    if (data[i].mc_id() != id) {
      ACE_DEBUG((LM_INFO, "(%P|%t) INFO: PowerDevicesRequestDataReaderListenerImpl::on_data_available: "
                 " Received request for different controller with Id: %C\n", data[i].mc_id().c_str()));
      continue;
    }

    if (info_seq[i].valid_data) {
      my_request = true;
      break;
    }
  }

  if (!my_request) {
    // None of these requests are for me.
    return;
  }

  const PowerDevices power_devices = mc.power_devices();
  cli::PowerDevicesReply reply;
  reply.mc_id(id);
  const size_t num_devices = power_devices.size();
  cli::PowerDeviceInfoSeq& pdi_seq = reply.devices();
  pdi_seq.reserve(num_devices);
  for (auto it = power_devices.begin(); it != power_devices.end(); ++it) {
    pdi_seq.push_back(it->second);
  }

  cli::PowerDevicesReplyDataWriter_var pdreply_writer = cli_server_.get_PowerDevicesReply_writer();
  rc = pdreply_writer->write(reply, DDS::HANDLE_NIL);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: PowerDevicesRequestDataReaderListenerImpl::on_data_available: "
               "write PowerDevicesReply failed: %C\n", OpenDDS::DCPS::retcode_to_string(rc)));
  }
}
