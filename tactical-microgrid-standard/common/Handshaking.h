#ifndef HANDSHAKING
#define HANDSHAKING

#include "idl/mil-std-3071_data_modelTypeSupportImpl.h"

#include <dds/DCPS/Service_Participant.h>

#include <atomic>
#include <functional>
#include <condition_variable>
#include <mutex>

class Handshaking {
public:
  Handshaking(const tms::Identity& device_id)
    : device_id_(device_id)
    , seq_num_(0)
    , stop_(false)
    , done_(false)
  {}

  ~Handshaking();

  // Initialize a domain participant for the given domain ID.
  // Create the DeviceInfo and Heartbeat topics.
  DDS::ReturnCode_t join_domain(DDS::DomainId_t domain_id, int argc = 0, char* argv[] = 0);

  // Create publishers and data writers for the DeviceInfo and Heartbeat topics.
  DDS::ReturnCode_t create_publishers();

  DDS::ReturnCode_t send_device_info(tms::DeviceInfo device_info);

  // Send heartbeats in a separate thread
  DDS::ReturnCode_t start_heartbeats();

  void stop_heartbeats();

  // Create subscribers and data readers for the DeviceInfo and Heartbeat topics.
  // User provides callbacks to process received samples of these 2 topics.
  DDS::ReturnCode_t create_subscribers(
    std::function<void(const tms::DeviceInfo&, const DDS::SampleInfo&)> di_cb = nullptr,
    std::function<void(const tms::Heartbeat&, const DDS::SampleInfo&)> hb_cb = nullptr);

  DDS::DomainParticipant_var get_domain_participant()
  {
    return participant_;
  }

protected:
  const tms::Identity device_id_;

private:
  DDS::DomainParticipantFactory_var dpf_;
  DDS::DomainParticipant_var participant_;
  DDS::Topic_var di_topic_, hb_topic_;
  tms::DeviceInfoDataWriter_var di_dw_;
  tms::HeartbeatDataWriter_var hb_dw_;

  unsigned long seq_num_;

  std::atomic<bool> stop_;

  struct NotifyGuard {
    NotifyGuard(Handshaking& handshake) : handshake_(handshake) {}

    ~NotifyGuard()
    {
      std::lock_guard<std::mutex> guard(handshake_.mut_);
      handshake_.done_ = true;
      handshake_.cond_.notify_one();
    }

    Handshaking& handshake_;
  };

  // For the heartbeat thread to announce it has finished and
  // doesn't require this Handshaking object to stay alive.
  std::mutex mut_;
  std::condition_variable cond_;
  bool done_;
};

#endif // HANDSHAKING
