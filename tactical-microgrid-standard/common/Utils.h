#ifndef TMS_COMMON_UTILS_H
#define TMS_COMMON_UTILS_H

#include <common/mil-std-3071_data_modelTypeSupportImpl.h>
#include <common/OpenDDS_TMS_export.h>

#include <string>
#include <random>

extern const char* MANUFACTURER_NAME;
extern const char* MODEL_NAME;
extern const char* MODEL_NUMBER;
extern const char* SERIAL_NUMBER;
extern const char* SOFTWARE_VERSION;

namespace Utils {

OpenDDS_TMS_Export DDS::DomainId_t get_sim_domain_id(DDS::DomainId_t tms_domain_id);

void setup_sim_transport(DDS::DomainParticipant_var sim_dp);

OpenDDS_TMS_Export std::string device_role_to_string(tms::DeviceRole role);

OpenDDS_TMS_Export tms::ProductInfo get_ProductInfo();

OpenDDS_TMS_Export tms::TopicInfo get_TopicInfo(const tms::TopicList& published_conditional_topics,
  const tms::TopicList& published_optional_topics, const tms::TopicList& subscribed_topics);

}

#endif
