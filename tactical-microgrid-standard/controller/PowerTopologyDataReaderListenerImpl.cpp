#include "PowerTopologyDataReaderListenerImpl.h"

void PowerTopologyDataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  powersim::PowerTopologySeq data;
  DDS::SampleInfoSeq info_seq;
  powersim::PowerTopologyDataReader_var typed_reader = powersim::PowerTopologyDataReader::_narrow(reader);
  DDS::ReturnCode_t rc = typed_reader->take(data, info_seq, DDS::LENGTH_UNLIMITED,
                                            DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: PowerTopologyDataReaderListenerImpl::on_data_available: "
               "take data failed: %C\n", OpenDDS::DCPS::retcode_to_string(rc)));
    return;
  }

  // Propagate the power connections to each power device
  for (CORBA::ULong i = 0; i < data.length(); ++i) {
    if (info_seq[i].valid_data) {
      const powersim::PowerTopology& pt = data[i];
      if (pt.mc_id() != cli_server_.get_controller().id()) {
        continue;
      }

      const auto& connections = pt.connections();
      for (CORBA::ULong j = 0; j < connections.size(); ++j) {
        const powersim::PowerConnection& pc = connections[j];
        const DDS::ReturnCode_t rc = cli_server_.get_PowerConnection_writer()->write(pc, DDS::HANDLE_NIL);
        if (rc != DDS::RETCODE_OK) {
          ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: PowerTopologyDataReaderListenerImpl::on_data_available:"
                     " write PowerConnection to device \"%C\" failed: %C\n", pc.pd_id().c_str(),
                     OpenDDS::DCPS::retcode_to_string(rc)));
          return;
        }
      }
      break;
    }
  }
}
