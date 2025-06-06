#ifndef TMS_COMMON_HANDSHAKING_H
#define TMS_COMMON_HANDSHAKING_H

#include "TimerHandler.h"

#include <common/mil-std-3071_data_modelTypeSupportImpl.h>
#include <common/OpenDDS_TMS_export.h>

#include <dds/DCPS/Service_Participant.h>

#include <functional>

class OpenDDS_TMS_Export Handshaking : public TimerHandler<tms::Heartbeat> {
public:
  explicit Handshaking(const tms::Identity& device_id)
    : device_id_(device_id)
    , seq_num_(0)
  {}

  virtual ~Handshaking();

  // Initialize a domain participant for the given domain ID.
  // Create the DeviceInfo and Heartbeat topics.
  DDS::ReturnCode_t join_domain(DDS::DomainId_t domain_id, int argc = 0, char* argv[] = nullptr);

  // Create publishers and data writers for the DeviceInfo and Heartbeat topics.
  DDS::ReturnCode_t create_publishers();

  DDS::ReturnCode_t send_device_info(tms::DeviceInfo device_info);

  // Send heartbeats in a separate thread
  DDS::ReturnCode_t start_heartbeats();

  // Temporarily stop sending heartbeats
  void stop_heartbeats();

  // Create subscribers and data readers for the DeviceInfo and Heartbeat topics.
  // User provides callbacks to process received samples of these 2 topics.
  DDS::ReturnCode_t create_subscribers(
    std::function<void(const tms::DeviceInfo&, const DDS::SampleInfo&)> di_cb = nullptr,
    std::function<void(const tms::Heartbeat&, const DDS::SampleInfo&)> hb_cb = nullptr);

  DDS::DomainParticipantFactory_var get_participant_factory() const
  {
    return dpf_;
  }

  DDS::DomainParticipant_var get_domain_participant() const
  {
    return participant_;
  }

  tms::Identity get_device_id() const
  {
    return device_id_;
  }

protected:
  virtual tms::DeviceInfo get_device_info() const
  {
    tms::DeviceInfo device_info;
    device_info.deviceId(device_id_);
    return device_info;
  }

  const tms::Identity device_id_;
  DDS::DomainParticipant_var participant_;

private:
  static constexpr Sec heartbeat_period = Sec(1);

  void timer_fired(Timer<tms::Heartbeat>& timer);
  void any_timer_fired(AnyTimer timer)
  {
    std::visit([&](auto&& value) { this->timer_fired(*value); }, timer);
  }

  DDS::DomainParticipantFactory_var dpf_;
  DDS::Topic_var di_topic_, hb_topic_;
  tms::DeviceInfoDataWriter_var di_dw_;
  tms::HeartbeatDataWriter_var hb_dw_;

  uint32_t seq_num_;
};

#endif // HANDSHAKING_H
