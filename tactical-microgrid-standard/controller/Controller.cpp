#include "Controller.h"

DDS::ReturnCode_t Controller::run(DDS::DomainId_t domain_id, int argc, char* argv[])
{
  DDS::ReturnCode_t rc = join_domain(domain_id, argc, argv);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  rc = create_subscribers(
    [&](const tms::DeviceInfo& di, const DDS::SampleInfo& si) {
      device_info_cb(di, si);
    },
    [&](const tms::Heartbeat& hb, const DDS::SampleInfo& si) {
      heartbeat_cb(hb, si);
    });
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  rc = create_publishers();
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  tms::DeviceInfo di;
  populate_device_info(di);

  rc = send_device_info(di);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  rc = start_heartbeats();
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  return DDS::RETCODE_OK;
}

tms::Identity Controller::id() const
{
  return device_id_;
}

Controller::PowerDevices Controller::power_devices() const
{
  std::lock_guard<std::mutex> guard(mut_);
  return power_devices_;
}

void Controller::device_info_cb(const tms::DeviceInfo& di, const DDS::SampleInfo& si)
{
  if (!si.valid_data || di.deviceId() == device_id_) {
    return;
  }

  ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: Controller::device_info_cb: device: \"%C\"\n", di.deviceId().c_str()));
  power_devices_.insert(std::make_pair(di.deviceId(), di));
}

void Controller::heartbeat_cb(const tms::Heartbeat& hb, const DDS::SampleInfo& si)
{
  if (!si.valid_data || hb.deviceId() == device_id_) {
    return;
  }

  const tms::Identity& id = hb.deviceId();
  const uint32_t seqnum = hb.sequenceNumber();

  if (power_devices_.count(id) > 0) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: Controller::heartbeat_cb: known device: \"%C\", seqnum: %u\n", id.c_str(), seqnum));
  } else {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: Controller::heartbeat_cb: new device: \"%C\", seqnum: %u\n", id.c_str(), seqnum));
  }
}

void Controller::populate_device_info(tms::DeviceInfo& device_info)
{
  device_info.deviceId(device_id_);
  device_info.role(tms::DeviceRole::ROLE_MICROGRID_CONTROLLER);

  tms::ProductInfo prod_info;
  prod_info.nsn({'6', '2', '4', '0', '0', '0', '0', '2', '7', '2', '0', '5', '9'});
  prod_info.gtin({'0', '0', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '5'});
  prod_info.manufacturerName("Manufacturer A");
  prod_info.modelName("Model A");
  prod_info.modelNumber("n235ka");
  prod_info.serialNumber("C02CG123DC79");
  prod_info.softwareVersion("0.1");
  device_info.product(prod_info);

  tms::TopicInfo topic_info;
  topic_info.dataModelVersion(tms::TMS_VERSION);
  // List of conditional topics that are published by this controller: nothing
  // List of optional topics that are published by this controller
  topic_info.publishedOptionalTopics().push_back(tms::topic::TOPIC_ENERGY_START_STOP_REQUEST);
  topic_info.publishedOptionalTopics().push_back(tms::topic::TOPIC_AC_LOAD_SHARING_REQUEST);
  topic_info.publishedOptionalTopics().push_back(tms::topic::TOPIC_POWER_SWITCH_REQUEST);
  // List of request topics subscribed by this controller: nothing
  device_info.topics(topic_info);

  tms::MicrogridControllerInfo mc_info;
  mc_info.features().push_back(tms::MicrogridControllerFeature::MCF_GENERAL);
  mc_info.priorityRanking(0);
  tms::ControlServiceInfo csi;
  csi.mc() = mc_info;
  device_info.controlService() = csi;

  // The other fields are IDL optional:
  // - controlHardware: probably ignore
  // - powerHardware: not applicable
  // - controlParameters, metricParameters: optional depending whether we want to
  //   include these parameters information
  // - powerDevice: not applicable
}