#ifndef OPERATOR_INTENT_REQUEST_DATAREADER_LISTENER_IMPL_H
#define OPERATOR_INTENT_REQUEST_DATAREADER_LISTENER_IMPL_H

#include <dds/DCPS/LocalObject.h>
#include <dds/DdsDcpsSubscriptionC.h>

class OperatorIntentRequestDataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
  OperatorIntentRequestDataReaderListenerImpl() {}
  virtual ~OperatorIntentRequestDataReaderListenerImpl() {}

  virtual void on_requested_deadline_missed(DDS::DataReader_ptr,
                                            const DDS::RequestedDeadlineMissedStatus&)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) INFO: OperatorIntentRequestDataReaderListenerImpl::on_requested_deadline_missed\n"));
  }

  virtual void on_requested_incompatible_qos(DDS::DataReader_ptr,
                                             const DDS::RequestedIncompatibleQosStatus&)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) INFO: OperatorIntentRequestDataReaderListenerImpl::on_requested_incompatible_qos\n"));
  }

  virtual void on_liveliness_changed(DDS::DataReader_ptr,
                                     const DDS::LivelinessChangedStatus&)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) INFO: OperatorIntentRequestDataReaderListenerImpl::on_liveliness_changed\n"));
  }

  virtual void on_subscription_matched(DDS::DataReader_ptr,
                                       const DDS::SubscriptionMatchedStatus&)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) INFO: OperatorIntentRequestDataReaderListenerImpl::on_subscription_matched\n"));
  }

  virtual void on_sample_rejected(DDS::DataReader_ptr,
                                  const DDS::SampleRejectedStatus&)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) INFO: OperatorIntentRequestDataReaderListenerImpl::on_sample_rejected\n"));
  }

  virtual void on_data_available(DDS::DataReader_ptr reader);

  virtual void on_sample_lost(DDS::DataReader_ptr,
                              const DDS::SampleLostStatus&)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) INFO: OperatorIntentRequestDataReaderListenerImpl::on_sample_lost\n"));
  }
};

#endif
