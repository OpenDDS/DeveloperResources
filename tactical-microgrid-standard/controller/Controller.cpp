#include "common/Handshaking.h"

#include <unordered_map>

const DDS::DomainId_t domain_id = 11;
const char* MICROGRID_CONTROLLER_IDENTITY = "Microgrid Controller Test";

std::unordered_map<tms::Identity, tms::DeviceInfo> connected_devices;

void device_info_cb(const tms::DeviceInfo& di, const DDS::SampleInfo& si)
{
  if (si.valid_data) {
    connected_devices.insert(std::make_pair(di.deviceId(), di));
  }
}

void heartbeat_cb(const tms::Heartbeat& hb, const DDS::SampleInfo& si)
{
  if (si.valid_data) {
    const tms::Identity& id = hb.deviceId();
    if (connected_devices.count(id) > 0) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: heartbeat_cb: received heartbeat from device: '%C'\n", id.c_str()));
    } else {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: heartbeat_cb: received heartbeat from unknown device: '%C'\n", id.c_str()));
    }
  }
}

int main(int argc, char* argv[])
{
  Handshaking handshake;
  if (handshake.join_domain(domain_id, argc, argv) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: controller: join_domain failed\n"));
    return 1;
  }

  // Announce the controller and publish heartbeats
  if (handshake.create_publishers() != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: controller: create_publishers failed\n"));
    return 1;
  }

  tms::DeviceInfo device_info;
  device_info.deviceId(MICROGRID_CONTROLLER_IDENTITY);
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

  if (handshake.send_device_info(device_info) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: controller: send_device_info failed\n"));
    return 1;
  }

  tms::Identity id(MICROGRID_CONTROLLER_IDENTITY);

  if (handshake.start_heartbeats(id) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: controller: start_heartbeat failed\n"));
    return 1;
  }

  // Subscribe to the other devices' info and heartbeats
  if (handshake.create_subscribers(device_info_cb, heartbeat_cb) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: controller: create_subscribers failed\n"));
    return 1;
  }

  return 0;
}
