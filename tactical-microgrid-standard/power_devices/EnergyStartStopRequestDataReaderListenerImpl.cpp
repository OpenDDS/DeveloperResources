#include "EnergyStartStopRequestDataReaderListenerImpl.h"
#include "PowerDevice.h"

#include <common/mil-std-3071_data_modelTypeSupportImpl.h>


void EnergyStartStopRequestDataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  tms::EnergyStartStopRequestSeq data;
  DDS::SampleInfoSeq info_seq;
  tms::EnergyStartStopRequestDataReader_var typed_reader = tms::EnergyStartStopRequestDataReader::_narrow(reader);
  DDS::ReturnCode_t rc = typed_reader->take(data, info_seq, DDS::LENGTH_UNLIMITED,
                                            DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: EnergyStartStopRequestDataReaderListenerImpl::on_data_available: "
               "take data failed: %C\n", OpenDDS::DCPS::retcode_to_string(rc)));
    return;
  }

  for (CORBA::ULong i = 0; i < data.length(); ++i) {
    if (info_seq[i].valid_data) {
      const tms::EnergyStartStopRequest& essr = data[i];
      const tms::Identity& sending_mc_id = essr.requestId().requestingDeviceId();
      if (sending_mc_id != power_device_.selected()) {
        // Ignore request from non-selected controller
        continue;
      }

      const tms::Identity& target_id = essr.requestId().targetDeviceId();
      if (target_id == power_device_.get_device_id()) {
        // Always set to the requested level and send an OK reply
        const tms::EnergyStartStopLevel essl = essr.toLevel();
        power_device_.energy_level(essl);

        tms::Reply reply;
        reply.requestingDeviceId() = sending_mc_id;
        reply.targetDeviceId() = power_device_.get_device_id();
        reply.config() = essr.requestId().config();
        reply.portNumber() = tms::INVALID_PORT_NUMBER;
        reply.requestSequenceId() = essr.sequenceId();
        reply.status().code() = tms::ReplyCode::REPLY_OK;
        reply.status().reason() = "OK";

        const DDS::ReturnCode_t rc = power_device_.reply_dw()->write(reply, DDS::HANDLE_NIL);
        if (rc != DDS::RETCODE_OK) {
          ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: EnergyStartStopRequestDataReaderListenerImpl::on_data_available: "
                     "write reply failed: %C\n", OpenDDS::DCPS::retcode_to_string(rc)));
        }
        break;
      }
    }
  }
}
