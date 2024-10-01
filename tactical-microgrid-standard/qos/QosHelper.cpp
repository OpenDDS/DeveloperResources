#include "mil-std-3071_data_modelTypeSupportImpl.h"

#include <dds/DCPS/Marked_Default_Qos.h>

// TODO: Provide functions to change application-specific QoS
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
}

void init_UserDataQosPolicy(UserDataQosPolicy& user_data, const tms::Identity& device_id)
{
  const CORBA::ULong len = device_id.length();
  user_data.value.length(len);
  std::memcpy(user_data.value.get_buffer(), static_cast<unsigned char*>(device_id.c_str()), len);
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

  OpenDDS::DCPS::OwnershipQosPolicy ownership_builder;
  qos.ownership = ownership_builder.exclusive();

  qos.liveliness = TheServiceParticipant->initial_LivelinessQosPolicy();

  OpenDDS::DCPS::ReliabilityQosPolicy reliability_builder;
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

  OpenDDS::DCPS::OwnershipQosPolicy ownership_builder;
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

  OpenDDS::DCPS::OwnershipQosPolicy ownership_builder;
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

  OpenDDS::DCPS::OwnershipQosPolicy ownership_builder;
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

  OpenDDS::DCPS::OwnershipQosPolicy ownership_builder;
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

  OpenDDS::DCPS::OwnershipQosPolicy ownership_builder;
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

  OpenDDS::DCPS::OwnershipQosPolicy ownership_builder;
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

  OpenDDS::DCPS::OwnershipQosPolicy ownership_builder;
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
  // durability_service is N/A
  qos.transport_priority = TheServiceParticipant->initial_TransportPriorityQosPolicy();
  qos.lifespan = TheServiceParticipant->initial_LifespanQosPolicy();
  OpenDDS::DCPS::OwnershipStrengthQosPolicyBuilder ownership_strength_builder;
  qos.ownership_strength = ownership_strength_builder.value(1);
  qos.writer_data_lifecycle = TheServiceParticipant->initial_WriterDataLifecycleQosPolicy();
}

}

namespace Qos {

namespace Subscriber {
DDS::SubscriberQos get_PublishLast()
{
  return get_group_qos_profile();
}

DDS::SubscriberQos get_Rare()
{
  return get_group_qos_profile();
}

DDS::SubscriberQos get_Slow()
{
  return get_group_qos_profile();
}

DDS::SubscriberQos get_Medium()
{
  return get_group_qos_profile();
}

DDS::SubscriberQos get_Continuous()
{
  return get_group_qos_profile();
}

DDS::SubscriberQos get_Command()
{
  return get_group_qos_profile();
}

DDS::SubscriberQos get_Response()
{
  return get_group_qos_profile();
}

DDS::SubscriberQos get_Reply()
{
  return get_group_qos_profile();
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
DDS::PublisherQos get_PublishLast()
{
  return get_group_qos_profile();
}

DDS::PublisherQos get_Rare()
{
  return get_group_qos_profile();
}

DDS::PublisherQos get_Slow()
{
  return get_group_qos_profile();
}

DDS::PublisherQos get_Medium()
{
  return get_group_qos_profile();
}

DDS::PublisherQos get_Continuous()
{
  return get_group_qos_profile();
}

DDS::PublisherQos get_Command()
{
  return get_group_qos_profile();
}

DDS::PublisherQos get_Response()
{
  return get_group_qos_profile();
}

DDS::PublisherQos get_Reply()
{
  return get_group_qos_profile();
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
