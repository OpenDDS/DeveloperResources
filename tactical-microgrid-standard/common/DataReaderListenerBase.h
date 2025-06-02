#ifndef TMS_COMMON_DATA_READER_LISTENER_BASE_H
#define TMS_COMMON_DATA_READER_LISTENER_BASE_H

#include <dds/DCPS/LocalObject.h>
#include <dds/DCPS/debug.h>
#include <dds/DdsDcpsSubscriptionC.h>

class DataReaderListenerBase : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
  explicit DataReaderListenerBase(const std::string& listener_name) : listener_name_(listener_name) {}

  virtual void on_requested_deadline_missed(DDS::DataReader_ptr,
                                            const DDS::RequestedDeadlineMissedStatus&)
  {
    if (OpenDDS::DCPS::DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_INFO, "(%P|%t) INFO: %C::on_requested_deadline_missed\n", listener_name_.c_str()));
    }
  }

  virtual void on_requested_incompatible_qos(DDS::DataReader_ptr,
                                             const DDS::RequestedIncompatibleQosStatus&)
  {
    if (OpenDDS::DCPS::DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_INFO, "(%P|%t) INFO: %C::on_requested_incompatible_qos\n", listener_name_.c_str()));
    }
  }

  virtual void on_liveliness_changed(DDS::DataReader_ptr,
                                     const DDS::LivelinessChangedStatus&)
  {
    if (OpenDDS::DCPS::DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_INFO, "(%P|%t) INFO: %C::on_liveliness_changed\n", listener_name_.c_str()));
    }
  }

  virtual void on_subscription_matched(DDS::DataReader_ptr,
                                       const DDS::SubscriptionMatchedStatus&)
  {
    if (OpenDDS::DCPS::DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_INFO, "(%P|%t) INFO: %C::on_subscription_matched\n", listener_name_.c_str()));
    }
  }

  virtual void on_sample_rejected(DDS::DataReader_ptr,
                                  const DDS::SampleRejectedStatus&)
  {
    if (OpenDDS::DCPS::DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_INFO, "(%P|%t) INFO: %C::on_sample_rejected\n", listener_name_.c_str()));
    }
  }

  virtual void on_data_available(DDS::DataReader_ptr) = 0;

  virtual void on_sample_lost(DDS::DataReader_ptr,
                              const DDS::SampleLostStatus&)
  {
    if (OpenDDS::DCPS::DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_INFO, "(%P|%t) INFO: %C::on_sample_lost\n", listener_name_.c_str()));
    }
  }

private:
  const std::string listener_name_;
};

#endif
