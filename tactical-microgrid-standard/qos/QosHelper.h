#ifndef QOS_HELPER_H
#define QOS_HELPER_H

#include <dds/DCPS/DdsDcpsCoreC.h>

namespace Qos {
namespace Subscriber {
DDS::SubscriberQos get_PublishLast();
DDS::SubscriberQos get_Rare();
DDS::SubscriberQos get_Slow();
DDS::SubscriberQos get_Medium();
DDS::SubscriberQos get_Continuous();
DDS::SubscriberQos get_Command();
DDS::SubscriberQos get_Response();
DDS::SubscriberQos get_Reply();
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
}

namespace Publisher {
DDS::PublisherQos get_PublishLast();
DDS::PublisherQos get_Rare();
DDS::PublisherQos get_Slow();
DDS::PublisherQos get_Medium();
DDS::PublisherQos get_Continuous();
DDS::PublisherQos get_Command();
DDS::PublisherQos get_Response();
DDS::PublisherQos get_Reply();
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
}
}

#endif
