#ifndef QOS_HELPER_H
#define QOS_HELPER_H

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
}

#endif
