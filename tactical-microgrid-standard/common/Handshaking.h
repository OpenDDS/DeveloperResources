#ifndef HANDSHAKING
#define HANDSHAKING

#include "idl/mil-std-3071_data_modelTypeSupportImpl.h"

#include <dds/DCPS/Service_Participant.h>

#include <atomic>
#include <functional>

class Handshaking {
public:
  Handshaking(const tms::Identity& device_id)
    : seq_num_(0)
    , stop_(false)
    , device_id_(device_id)
  {}

  ~Handshaking();

  // Initialize a domain participant for the given domain ID.
  // Create the DeviceInfo and Heartbeat topics.
  DDS::ReturnCode_t join_domain(DDS::DomainId_t domain_id, int argc = 0, char* argv[] = 0);

  // Create publishers and data writers for the DeviceInfo and Heartbeat topics.
  DDS::ReturnCode_t create_publishers();

  DDS::ReturnCode_t send_device_info(tms::DeviceInfo device_info);

  // Send heartbeats in a separate thread
  DDS::ReturnCode_t start_heartbeats(tms::Identity id);

  void stop_heartbeats()
  {
    stop_ = true;
  }

  // Create subscribers and data readers for the DeviceInfo and Heartbeat topics.
  // User provides callbacks to process received samples of these 2 topics.
  DDS::ReturnCode_t create_subscribers(
    std::function<void(const tms::DeviceInfo&, const DDS::SampleInfo&)> di_cb = nullptr,
    std::function<void(const tms::Heartbeat&, const DDS::SampleInfo&)> hb_cb = nullptr);

  DDS::DomainParticipant_var get_domain_participant()
  {
    return participant_;
  }

private:
  DDS::DomainParticipantFactory_var dpf_;
  DDS::DomainParticipant_var participant_;
  DDS::Topic_var di_topic_, hb_topic_;
  tms::DeviceInfoDataWriter_var di_dw_;
  tms::HeartbeatDataWriter_var hb_dw_;

  unsigned long seq_num_;
  std::atomic<bool> stop_;
  const tms::Identity device_id_;
};

#endif // HANDSHAKING
