#include "Utils.h"

const char* MANUFACTURER_NAME = "OCI TMS Demo";
const char* MODEL_NAME = "OCI TMS Model Name";
const char* MODEL_NUMBER = "OCI TMS Model Number";
const char* SERIAL_NUMBER = "OCI TMS Serial Number";
const char* SOFTWARE_VERSION = "0.1";

namespace {
  char gen_numeric_char()
  {
    const unsigned seed = static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count());
    std::mt19937 generator(seed);
    std::uniform_int_distribution<int> distribution(0, 9);
    return '0' + distribution(generator);
  }

  tms::NatoStockNumber get_NatoStockNumber()
  {
    tms::NatoStockNumber nsn{};
    // Supply classification number
    nsn[0] = '6';
    nsn[1] = '1';
    nsn[2] = '1';
    nsn[3] = '0';

    // Item identification number
    nsn[4] = '0';
    nsn[5] = '0';
    for (size_t i = 6; i < 13; ++i) {
      nsn[i] = gen_numeric_char();
    }
    return nsn;
  }

  tms::GlobalTradeItemNumber get_GlobalTradeItemNumber()
  {
    tms::GlobalTradeItemNumber gtin{};
    for (size_t i = 0; i < 14; ++i) {
      gtin[i] = gen_numeric_char();
    }
    return gtin;
  }
}

namespace Utils {

  DDS::DomainId_t get_sim_domain_id(DDS::DomainId_t tms_domain_id)
  {
    return (tms_domain_id < ACE_INT32_MAX) ? tms_domain_id + 1 : tms_domain_id - 1;
  }

  std::string device_role_to_string(tms::DeviceRole role)
  {
    switch (role) {
    case tms::DeviceRole::ROLE_MICROGRID_CONTROLLER:
      return "Microgrid Controller";
    case tms::DeviceRole::ROLE_SOURCE:
      return "Source";
    case tms::DeviceRole::ROLE_LOAD:
      return "Load";
    case tms::DeviceRole::ROLE_STORAGE:
      return "Storage";
    case tms::DeviceRole::ROLE_DISTRIBUTION:
      return "Distribution";
    case tms::DeviceRole::ROLE_MICROGRID_DASHBOARD:
      return "Microgrid Dashboard";
    case tms::DeviceRole::ROLE_CONVERSION:
      return "Conversion";
    case tms::DeviceRole::ROLE_MONITOR:
      return "Monitor";
    default:
      return "Unknown";
    }
  }

  tms::ProductInfo get_ProductInfo()
  {
    tms::ProductInfo prod_info;
    prod_info.nsn() = get_NatoStockNumber();
    prod_info.gtin() = get_GlobalTradeItemNumber();
    prod_info.manufacturerName(MANUFACTURER_NAME);
    prod_info.modelName(MODEL_NAME);
    prod_info.modelNumber(MODEL_NUMBER);
    prod_info.serialNumber(SERIAL_NUMBER);
    prod_info.softwareVersion(SOFTWARE_VERSION);
    return prod_info;
  }

  tms::TopicInfo get_TopicInfo(const tms::TopicList& published_conditional_topics,
    const tms::TopicList& published_optional_topics, const tms::TopicList& subscribed_topics)
  {
    tms::TopicInfo topic_info;
    topic_info.dataModelVersion(tms::TMS_VERSION);
    topic_info.publishedConditionalTopics() = published_conditional_topics;
    topic_info.publishedOptionalTopics() = published_optional_topics;
    topic_info.supportedRequestTopics() = subscribed_topics;
    return topic_info;
  }
}
