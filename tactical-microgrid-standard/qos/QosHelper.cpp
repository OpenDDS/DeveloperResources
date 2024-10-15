#include "QosHelper.h"

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Qos_Helper.h>

// The following QoS is application-specific:
// - RELIABILITY.max_blocking_time
// - ENTITY_FACTORY
// - WRITER_DATA_LIFECYCLE
// - READER_DATA_LIFECYCLE

namespace Qos {

namespace DataReader {
// Add an entry for each supported topic
const FnMap fn_map = {
  {tms::topic::TOPIC_HEARTBEAT, get_Medium},
  {tms::topic::TOPIC_DEVICE_INFO, get_PublishLast}};
}

namespace DataWriter {
// Add an entry for each supported topic
const FnMap fn_map = {
  {tms::topic::TOPIC_HEARTBEAT, get_Medium},
  {tms::topic::TOPIC_DEVICE_INFO, get_PublishLast}};
}

}

namespace {

// Common Qos values for SubscriberQos and PublisherQos
// All TMS Qos profiles have the same values.
template <typename T>
T get_group_qos_profile()
{
  T qos;
  qos.presentation.access_scope = DDS::INSTANCE_PRESENTATION_QOS;
  qos.presentation.coherent_access = false;
  qos.presentation.ordered_access = false;

  // partition & group_data are zero-length sequence

  // entity_factory is application-specific
  qos.entity_factory.autoenable_created_entities = true;
  return qos;
}

void init_UserDataQosPolicy(DDS::UserDataQosPolicy& user_data, const tms::Identity& device_id)
{
  const CORBA::ULong len = device_id.length();
  user_data.value.length(len);
  std::memcpy(user_data.value.get_buffer(), device_id.c_str(), len);
}

// Common Qos values for DataReaderQos and DataWriterQos
template <typename T>
void init_endpoint_PublishLast_profile(T& qos, const tms::Identity& device_id)
{
  init_UserDataQosPolicy(qos.user_data, device_id);

  OpenDDS::DCPS::DurabilityQosPolicyBuilder durability_builder;
  qos.durability = durability_builder.transient_local();

  qos.deadline = TheServiceParticipant->initial_DeadlineQosPolicy();
  qos.latency_budget = TheServiceParticipant->initial_LatencyBudgetQosPolicy();

  OpenDDS::DCPS::OwnershipQosPolicyBuilder ownership_builder;
  qos.ownership = ownership_builder.exclusive();

  qos.liveliness = TheServiceParticipant->initial_LivelinessQosPolicy();

  OpenDDS::DCPS::ReliabilityQosPolicyBuilder reliability_builder;
  qos.reliability = reliability_builder.reliable();

  qos.destination_order = TheServiceParticipant->initial_DestinationOrderQosPolicy();
  qos.history = TheServiceParticipant->initial_HistoryQosPolicy();
  qos.resource_limits = TheServiceParticipant->initial_ResourceLimitsQosPolicy();
}

template <typename T>
void init_endpoint_Rare_profile(T& qos, const tms::Identity& device_id)
{
  init_UserDataQosPolicy(qos.user_data, device_id);
  qos.durability = TheServiceParticipant->initial_DurabilityQosPolicy();

  OpenDDS::DCPS::DeadlineQosPolicyBuilder deadline_builder;
  qos.deadline = deadline_builder.period({ 2000, 0 });

  qos.latency_budget = TheServiceParticipant->initial_LatencyBudgetQosPolicy();

  OpenDDS::DCPS::OwnershipQosPolicyBuilder ownership_builder;
  qos.ownership = ownership_builder.exclusive();

  qos.liveliness = TheServiceParticipant->initial_LivelinessQosPolicy();
  qos.reliability = TheServiceParticipant->initial_ReliabilityQosPolicy();
  qos.destination_order = TheServiceParticipant->initial_DestinationOrderQosPolicy();
  qos.history = TheServiceParticipant->initial_HistoryQosPolicy();
  qos.resource_limits = TheServiceParticipant->initial_ResourceLimitsQosPolicy();
}

template <typename T>
void init_endpoint_Slow_profile(T& qos, const tms::Identity& device_id)
{
  init_UserDataQosPolicy(qos.user_data, device_id);
  qos.durability = TheServiceParticipant->initial_DurabilityQosPolicy();

  OpenDDS::DCPS::DeadlineQosPolicyBuilder deadline_builder;
  qos.deadline = deadline_builder.period({ 20, 0 });

  qos.latency_budget = TheServiceParticipant->initial_LatencyBudgetQosPolicy();

  OpenDDS::DCPS::OwnershipQosPolicyBuilder ownership_builder;
  qos.ownership = ownership_builder.exclusive();

  qos.liveliness = TheServiceParticipant->initial_LivelinessQosPolicy();
  qos.reliability = TheServiceParticipant->initial_ReliabilityQosPolicy();
  qos.destination_order = TheServiceParticipant->initial_DestinationOrderQosPolicy();
  qos.history = TheServiceParticipant->initial_HistoryQosPolicy();
  qos.resource_limits = TheServiceParticipant->initial_ResourceLimitsQosPolicy();
}

template <typename T>
void init_endpoint_Medium_profile(T& qos, const tms::Identity& device_id)
{
  init_UserDataQosPolicy(qos.user_data, device_id);
  qos.durability = TheServiceParticipant->initial_DurabilityQosPolicy();

  OpenDDS::DCPS::DeadlineQosPolicyBuilder deadline_builder;
  qos.deadline = deadline_builder.period({ 3, 0 });

  qos.latency_budget = TheServiceParticipant->initial_LatencyBudgetQosPolicy();

  OpenDDS::DCPS::OwnershipQosPolicyBuilder ownership_builder;
  qos.ownership = ownership_builder.exclusive();

  qos.liveliness = TheServiceParticipant->initial_LivelinessQosPolicy();
  qos.reliability = TheServiceParticipant->initial_ReliabilityQosPolicy();
  qos.destination_order = TheServiceParticipant->initial_DestinationOrderQosPolicy();
  qos.history = TheServiceParticipant->initial_HistoryQosPolicy();
  qos.resource_limits = TheServiceParticipant->initial_ResourceLimitsQosPolicy();
}

template <typename T>
void init_endpoint_Continuous_profile(T& qos, const tms::Identity& device_id)
{
  init_UserDataQosPolicy(qos.user_data, device_id);
  qos.durability = TheServiceParticipant->initial_DurabilityQosPolicy();

  OpenDDS::DCPS::DeadlineQosPolicyBuilder deadline_builder;
  qos.deadline = deadline_builder.period({ 2, 0 });

  qos.latency_budget = TheServiceParticipant->initial_LatencyBudgetQosPolicy();

  OpenDDS::DCPS::OwnershipQosPolicyBuilder ownership_builder;
  qos.ownership = ownership_builder.exclusive();

  qos.liveliness = TheServiceParticipant->initial_LivelinessQosPolicy();
  qos.reliability = TheServiceParticipant->initial_ReliabilityQosPolicy();
  qos.destination_order = TheServiceParticipant->initial_DestinationOrderQosPolicy();
  qos.history = TheServiceParticipant->initial_HistoryQosPolicy();
  qos.resource_limits = TheServiceParticipant->initial_ResourceLimitsQosPolicy();
}

template <typename T>
void init_endpoint_Command_profile(T& qos, const tms::Identity& device_id)
{
  init_UserDataQosPolicy(qos.user_data, device_id);
  qos.durability = TheServiceParticipant->initial_DurabilityQosPolicy();
  qos.deadline = TheServiceParticipant->initial_DeadlineQosPolicy();
  qos.latency_budget = TheServiceParticipant->initial_LatencyBudgetQosPolicy();

  OpenDDS::DCPS::OwnershipQosPolicyBuilder ownership_builder;
  qos.ownership = ownership_builder.exclusive();

  qos.liveliness = TheServiceParticipant->initial_LivelinessQosPolicy();

  OpenDDS::DCPS::ReliabilityQosPolicyBuilder reliability_builder;
  qos.reliability = reliability_builder.reliable();

  qos.destination_order = TheServiceParticipant->initial_DestinationOrderQosPolicy();
  qos.history = TheServiceParticipant->initial_HistoryQosPolicy();
  qos.resource_limits = TheServiceParticipant->initial_ResourceLimitsQosPolicy();
}

template <typename T>
void init_endpoint_Response_profile(T& qos, const tms::Identity& device_id)
{
  init_UserDataQosPolicy(qos.user_data, device_id);
  qos.durability = TheServiceParticipant->initial_DurabilityQosPolicy();
  qos.deadline = TheServiceParticipant->initial_DeadlineQosPolicy();
  qos.latency_budget = TheServiceParticipant->initial_LatencyBudgetQosPolicy();

  OpenDDS::DCPS::OwnershipQosPolicyBuilder ownership_builder;
  qos.ownership = ownership_builder.exclusive();

  qos.liveliness = TheServiceParticipant->initial_LivelinessQosPolicy();

  OpenDDS::DCPS::ReliabilityQosPolicyBuilder reliability_builder;
  qos.reliability = reliability_builder.reliable();

  qos.destination_order = TheServiceParticipant->initial_DestinationOrderQosPolicy();
  qos.history = TheServiceParticipant->initial_HistoryQosPolicy();
  qos.resource_limits = TheServiceParticipant->initial_ResourceLimitsQosPolicy();
}

template <typename T>
void init_endpoint_Reply_profile(T& qos, const tms::Identity& device_id)
{
  init_UserDataQosPolicy(qos.user_data, device_id);
  qos.durability = TheServiceParticipant->initial_DurabilityQosPolicy();
  qos.deadline = TheServiceParticipant->initial_DeadlineQosPolicy();
  qos.latency_budget = TheServiceParticipant->initial_LatencyBudgetQosPolicy();

  OpenDDS::DCPS::OwnershipQosPolicyBuilder ownership_builder;
  qos.ownership = ownership_builder.exclusive();

  qos.liveliness = TheServiceParticipant->initial_LivelinessQosPolicy();

  OpenDDS::DCPS::ReliabilityQosPolicyBuilder reliability_builder;
  qos.reliability = reliability_builder.reliable();

  qos.destination_order = TheServiceParticipant->initial_DestinationOrderQosPolicy();

  OpenDDS::DCPS::HistoryQosPolicyBuilder history_builder;
  qos.history = history_builder.keep_last(128);

  qos.resource_limits = TheServiceParticipant->initial_ResourceLimitsQosPolicy();
}

void init_datareader_common(DDS::DataReaderQos& qos)
{
  qos.time_based_filter = TheServiceParticipant->initial_TimeBasedFilterQosPolicy();
  qos.reader_data_lifecycle = TheServiceParticipant->initial_ReaderDataLifecycleQosPolicy();
}

void init_datawriter_common(DDS::DataWriterQos& qos)
{
  qos.durability_service = TheServiceParticipant->initial_DurabilityServiceQosPolicy();
  qos.transport_priority = TheServiceParticipant->initial_TransportPriorityQosPolicy();
  qos.lifespan = TheServiceParticipant->initial_LifespanQosPolicy();
  OpenDDS::DCPS::OwnershipStrengthQosPolicyBuilder ownership_strength_builder;
  qos.ownership_strength = ownership_strength_builder.value(1);
  qos.writer_data_lifecycle = TheServiceParticipant->initial_WriterDataLifecycleQosPolicy();
}

}

namespace Qos {

namespace Subscriber {
DDS::SubscriberQos get_qos()
{
  return get_group_qos_profile<DDS::SubscriberQos>();
}
} // namespace Subscriber

namespace DataReader {

DDS::DataReaderQos get_PublishLast(const tms::Identity& device_id)
{
  DDS::DataReaderQos qos;
  init_endpoint_PublishLast_profile(qos, device_id);
  init_datareader_common(qos);
  return qos;
}

DDS::DataReaderQos get_Rare(const tms::Identity& device_id)
{
  DDS::DataReaderQos qos;
  init_endpoint_Rare_profile(qos, device_id);
  init_datareader_common(qos);
  return qos;
}

DDS::DataReaderQos get_Slow(const tms::Identity& device_id)
{
  DDS::DataReaderQos qos;
  init_endpoint_Slow_profile(qos, device_id);
  init_datareader_common(qos);
  return qos;
}

DDS::DataReaderQos get_Medium(const tms::Identity& device_id)
{
  DDS::DataReaderQos qos;
  init_endpoint_Medium_profile(qos, device_id);
  init_datareader_common(qos);
  return qos;
}

DDS::DataReaderQos get_Continuous(const tms::Identity& device_id)
{
  DDS::DataReaderQos qos;
  init_endpoint_Continuous_profile(qos, device_id);
  init_datareader_common(qos);
  return qos;
}

DDS::DataReaderQos get_Command(const tms::Identity& device_id)
{
  DDS::DataReaderQos qos;
  init_endpoint_Command_profile(qos, device_id);
  init_datareader_common(qos);
  return qos;
}

DDS::DataReaderQos get_Response(const tms::Identity& device_id)
{
  DDS::DataReaderQos qos;
  init_endpoint_Response_profile(qos, device_id);
  init_datareader_common(qos);
  return qos;
}

DDS::DataReaderQos get_Reply(const tms::Identity& device_id)
{
  DDS::DataReaderQos qos;
  init_endpoint_Reply_profile(qos, device_id);
  init_datareader_common(qos);
  return qos;
}

} // namespace DataReader

namespace Publisher {
DDS::PublisherQos get_qos()
{
  return get_group_qos_profile<DDS::PublisherQos>();
}
} // namespace Publisher

namespace DataWriter {
DDS::DataWriterQos get_PublishLast(const tms::Identity& device_id)
{
  DDS::DataWriterQos qos;
  init_endpoint_PublishLast_profile(qos, device_id);
  init_datawriter_common(qos);
  return qos;
}

DDS::DataWriterQos get_Rare(const tms::Identity& device_id)
{
  DDS::DataWriterQos qos;
  init_endpoint_Rare_profile(qos, device_id);
  init_datawriter_common(qos);
  return qos;
}

DDS::DataWriterQos get_Slow(const tms::Identity& device_id)
{
  DDS::DataWriterQos qos;
  init_endpoint_Slow_profile(qos, device_id);
  init_datawriter_common(qos);
  return qos;
}

DDS::DataWriterQos get_Medium(const tms::Identity& device_id)
{
  DDS::DataWriterQos qos;
  init_endpoint_Medium_profile(qos, device_id);
  init_datawriter_common(qos);
  return qos;
}

DDS::DataWriterQos get_Continuous(const tms::Identity& device_id)
{
  DDS::DataWriterQos qos;
  init_endpoint_Continuous_profile(qos, device_id);
  init_datawriter_common(qos);
  return qos;
}

DDS::DataWriterQos get_Command(const tms::Identity& device_id)
{
  DDS::DataWriterQos qos;
  init_endpoint_Command_profile(qos, device_id);
  init_datawriter_common(qos);
  return qos;
}

DDS::DataWriterQos get_Response(const tms::Identity& device_id)
{
  DDS::DataWriterQos qos;
  init_endpoint_Response_profile(qos, device_id);
  init_datawriter_common(qos);
  return qos;
}

DDS::DataWriterQos get_Reply(const tms::Identity& device_id)
{
  DDS::DataWriterQos qos;
  init_endpoint_Reply_profile(qos, device_id);
  init_datawriter_common(qos);
  return qos;
}

} // namespace DataWriter

} // namespace Qos
