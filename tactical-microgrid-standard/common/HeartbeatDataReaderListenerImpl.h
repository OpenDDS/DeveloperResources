#ifndef HEARTBEAT_DATAREADER_LISTENER_IMPL
#define HEARTBEAT_DATAREADER_LISTENER_IMPL

#include <dds/DCPS/LocalObject.h>
#include <dds/DdsDcpsSubscriptionC.h>

#include <functional>

class HeartbeatDataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
  HeartbeatDataReaderListenerImpl(std::function<void(const tms::Heartbeat&, const DDS::SampleInfo&)> cb = nullptr)
    : callback_(cb)
  {}

  virtual ~HeartbeatDataReaderListenerImpl() {}

  virtual void on_requested_deadline_missed(DDS::DataReader_ptr,
                                            const DDS::RequestedDeadlineMissedStatus&)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) INFO: HeartbeatDataReaderListenerImpl::on_requested_deadline_missed\n"));
  }

  virtual void on_requested_incompatible_qos(DDS::DataReader_ptr,
                                             const DDS::RequestedIncompatibleQosStatus&)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) INFO: HeartbeatDataReaderListenerImpl::on_requested_incompatible_qos\n"));
  }

  virtual void on_liveliness_changed(DDS::DataReader_ptr,
                                     const DDS::LivelinessChangedStatus&)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) INFO: HeartbeatDataReaderListenerImpl::on_liveliness_changed\n"));
  }

  virtual void on_subscription_matched(DDS::DataReader_ptr,
                                       const DDS::SubscriptionMatchedStatus&)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) INFO: HeartbeatDataReaderListenerImpl::on_subscription_matched\n"));
  }

  virtual void on_sample_rejected(DDS::DataReader_ptr,
                                  const DDS::SampleRejectedStatus&)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) INFO: HeartbeatDataReaderListenerImpl::on_sample_rejected\n"));
  }

  virtual void on_data_available(DDS::DataReader_ptr reader);

  virtual void on_sample_lost(DDS::DataReader_ptr,
                              const DDS::SampleLostStatus&)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) INFO: HeartbeatDataReaderListenerImpl::on_sample_lost\n"));
  }

private:
  std::function<void(const tms::Heartbeat&, const DDS::SampleInfo&)> callback_;
};

#endif // HEARTBEAT_DATAREADER_LISTENER_IMPL
