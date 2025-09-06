#include "Controller.h"
#include "ActiveMicrogridControllerStateDataReaderListenerImpl.h"

#include <common/Utils.h>
#include <common/QosHelper.h>

#include <dds/DCPS/Marked_Default_Qos.h>

DDS::ReturnCode_t Controller::init(DDS::DomainId_t domain_id, int argc, char* argv[])
{
  DDS::ReturnCode_t rc = join_domain(domain_id, argc, argv);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  setup_config();

  rc = create_subscribers(
    [&](const auto& di, const auto& si) { device_info_cb(di, si); },
    [&](const auto& hb, const auto& si) { heartbeat_cb(hb, si); });
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  rc = create_publishers();
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  DDS::DomainParticipant_var dp = get_domain_participant();

  // Subscribe to the tms::ActiveMicrogridControllerState topic
  tms::ActiveMicrogridControllerStateTypeSupport_var amcs_ts = new tms::ActiveMicrogridControllerStateTypeSupportImpl;
  if (DDS::RETCODE_OK != amcs_ts->register_type(dp, "")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Controller::init: register_type ActiveMicrogridControllerState failed\n"));
    return DDS::RETCODE_ERROR;
  }

  CORBA::String_var amcs_type_name = amcs_ts->get_type_name();
  DDS::Topic_var amcs_topic = dp->create_topic(tms::topic::TOPIC_ACTIVE_MICROGRID_CONTROLLER_STATE.c_str(),
                                               amcs_type_name,
                                               TOPIC_QOS_DEFAULT,
                                               nullptr,
                                               ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!amcs_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Controller::init: create_topic \"%C\" failed\n",
               tms::topic::TOPIC_ACTIVE_MICROGRID_CONTROLLER_STATE.c_str()));
    return DDS::RETCODE_ERROR;
  }

  const DDS::SubscriberQos tms_sub_qos = Qos::Subscriber::get_qos();
  DDS::Subscriber_var tms_sub = dp->create_subscriber(tms_sub_qos,
                                                      nullptr,
                                                      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!tms_sub) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Controller::init: create_subscriber with TMS QoS failed\n"));
    return DDS::RETCODE_ERROR;
  }

  const DDS::DataReaderQos& amcs_dr_qos = Qos::DataReader::fn_map.at(tms::topic::TOPIC_ACTIVE_MICROGRID_CONTROLLER_STATE)(device_id_);
  DDS::DataReaderListener_var amcs_listener(new ActiveMicrogridControllerStateDataReaderListenerImpl(*this));
  DDS::DataReader_var amcs_dr_base = tms_sub->create_datareader(amcs_topic,
                                                                amcs_dr_qos,
                                                                amcs_listener,
                                                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!amcs_dr_base) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Controller::init: create_datareader for topic \"%C\" failed\n",
               tms::topic::TOPIC_ACTIVE_MICROGRID_CONTROLLER_STATE.c_str()));
    return DDS::RETCODE_ERROR;
  }

  auto di = populate_device_info();

  tms_domain_id_ = domain_id;

  return send_device_info(di);
}

int Controller::run()
{
  return reactor_->run_reactor_event_loop() == 0 ? 0 : 1;
}

void Controller::terminate()
{
  stop_heartbeats();
  reactor_->end_reactor_event_loop();
}

bool Controller::got_config(const std::string& name, const OpenDDS::DCPS::ConfigPair& pair)
{
  if (name == "DEBUG") {
    bool tmp;
    if (convert_bool(pair, tmp)) {
      set_debug(tmp);
    }
    return true;
  }
  return false;
}

void Controller::set_debug(bool value)
{
  std::lock_guard<std::mutex> guard(mut_);
  debug_ = value;
}

tms::Identity Controller::id() const
{
  return device_id_;
}

PowerDevices Controller::power_devices() const
{
  std::lock_guard<std::mutex> guard(mut_);
  return power_devices_;
}

void Controller::update_essl(const tms::Identity& pd_id, tms::EnergyStartStopLevel to_level)
{
  std::lock_guard<std::mutex> guard(mut_);
  auto it = power_devices_.find(pd_id);
  if (it != power_devices_.end()) {
    it->second.essl() = to_level;
  }
}

void Controller::device_info_cb(const tms::DeviceInfo& di, const DDS::SampleInfo& si)
{
  if (!si.valid_data || di.deviceId() == device_id_) {
    return;
  }

  std::lock_guard<std::mutex> guard(mut_);
  if (debug_) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Controller::device_info_cb: device: \"%C\"\n", di.deviceId().c_str()));
  }

  // Ignore other control devices, such as microgrid controllers.
  // Store all power devices, including those that select a different MC as its active MC.
  if (di.role() != tms::DeviceRole::ROLE_MICROGRID_CONTROLLER) {
    power_devices_.insert(std::make_pair(di.deviceId(),
                                         cli::PowerDeviceInfo(di, tms::EnergyStartStopLevel::ESSL_OPERATIONAL,
                                                              std::optional<tms::Identity>())));
  }
}

void Controller::heartbeat_cb(const tms::Heartbeat& hb, const DDS::SampleInfo& si)
{
  if (!si.valid_data || hb.deviceId() == device_id_) {
    return;
  }

  std::lock_guard<std::mutex> guard(mut_);
  if (debug_) {
    const tms::Identity& id = hb.deviceId();
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Controller::heartbeat_cb: %C device: \"%C\", seqnum: %u\n",
      power_devices_.count(id) > 0 ? "known" : "other", id.c_str(), hb.sequenceNumber()));
  }
}

void Controller::set_active_controller(const tms::Identity& pd_id, const std::optional<tms::Identity>& master_id)
{
  std::lock_guard<std::mutex> guard(mut_);
  auto it = power_devices_.find(pd_id);
  if (it != power_devices_.end()) {
    it->second.master_id() = master_id;
  }
}

tms::DeviceInfo Controller::populate_device_info() const
{
  auto device_info = get_device_info();
  device_info.role() = tms::DeviceRole::ROLE_MICROGRID_CONTROLLER;
  device_info.product() = Utils::get_ProductInfo();
  device_info.topics() = Utils::get_TopicInfo({}, { tms::topic::TOPIC_ENERGY_START_STOP_REQUEST },
    { tms::topic::TOPIC_OPERATOR_INTENT_REQUEST });

  tms::ControlServiceInfo csi;
  {
    tms::MicrogridControllerInfo mc_info;
    {
      mc_info.features().push_back(tms::MicrogridControllerFeature::MCF_GENERAL);
      mc_info.priorityRanking(priority_);
    }
    csi.mc() = mc_info;
  }
  device_info.controlService() = csi;

  return device_info;
}
