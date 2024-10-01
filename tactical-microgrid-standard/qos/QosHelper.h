#ifndef QOS_HELPER_H
#define QOS_HELPER_H

#include "idl/mil-std-3071_data_modelTypeSupportImpl.h"

#include <dds/DdsDcpsCoreC.h>

#include <unordered_map>
#include <string>
#include <functional>

namespace Qos {
namespace Subscriber {
DDS::SubscriberQos get_qos();
}

namespace DataReader {
DDS::DataReaderQos get_PublishLast(const tms::Identity& device_id);
DDS::DataReaderQos get_Rare(const tms::Identity& device_id);
DDS::DataReaderQos get_Slow(const tms::Identity& device_id);
DDS::DataReaderQos get_Medium(const tms::Identity& device_id);
DDS::DataReaderQos get_Continuous(const tms::Identity& device_id);
DDS::DataReaderQos get_Command(const tms::Identity& device_id);
DDS::DataReaderQos get_Response(const tms::Identity& device_id);
DDS::DataReaderQos get_Reply(const tms::Identity& device_id);

using Fn = std::function<DDS::DataReaderQos(const tms::Identity&)>;
using FnMap = std::unordered_map<std::string, Fn>;
extern const FnMap fn_map;
}

namespace Publisher {
DDS::PublisherQos get_qos();
}

namespace DataWriter {
DDS::DataWriterQos get_PublishLast(const tms::Identity& device_id);
DDS::DataWriterQos get_Rare(const tms::Identity& device_id);
DDS::DataWriterQos get_Slow(const tms::Identity& device_id);
DDS::DataWriterQos get_Medium(const tms::Identity& device_id);
DDS::DataWriterQos get_Continuous(const tms::Identity& device_id);
DDS::DataWriterQos get_Command(const tms::Identity& device_id);
DDS::DataWriterQos get_Response(const tms::Identity& device_id);
DDS::DataWriterQos get_Reply(const tms::Identity& device_id);

using Fn = std::function<DDS::DataWriterQos(const tms::Identity&)>;
using FnMap = std::unordered_map<std::string, Fn>;
extern const FnMap fn_map;
}
}

#endif
