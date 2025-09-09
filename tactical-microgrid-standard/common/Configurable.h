#ifndef TMS_COMMON_CONFIGURABLE
#define TMS_COMMON_CONFIGURABLE

#include <dds/DCPS/ConfigStoreImpl.h>
#include <dds/DCPS/InternalDataReaderListener.h>
#include <dds/DCPS/Service_Participant.h>

#include <mutex>

class Configurable {
public:
  using Mutex = std::recursive_mutex;
  using Guard = std::lock_guard<Mutex>;

  Configurable(const std::string& prefix)
    : config_prefix_(prefix)
  {
  }

  virtual ~Configurable()
  {
    Guard g(config_lock_);

    if (config_reader_) {
      TheServiceParticipant->config_topic()->disconnect(config_reader_);
    }
  }

  void setup_config()
  {
    Guard g(config_lock_);

    if (!config_reader_) {
      config_listener_ = OpenDDS::DCPS::make_rch<ConfigListener>(this, config_prefix_ + "_");
      config_reader_ = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ConfigReader>(
        TheServiceParticipant->config_store()->datareader_qos(), config_listener_);
      TheServiceParticipant->config_topic()->connect(config_reader_);
    }
  }

  const std::string& config_prefix() const
  {
    return config_prefix_;
  }

  static bool convert_bool(const OpenDDS::DCPS::ConfigPair& pair, bool& value)
  {
    DDS::Boolean x = 0;
    if (pair.value() == "true") {
      value = true;
      return true;
    } else if (pair.value() == "false") {
      value = false;
      return true;
    } else if (OpenDDS::DCPS::convertToInteger(pair.value(), x)) {
      value = x;
      return true;
    } else {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: Configurable::convert_bool: failed to parse boolean for %C=%C\n",
                 pair.key().c_str(), pair.value().c_str()));
      return false;
    }
  }

  virtual bool got_config(const std::string& name, const OpenDDS::DCPS::ConfigPair& pair) = 0;

private:
  class ConfigListener : public OpenDDS::DCPS::ConfigListener {
  public:
    explicit ConfigListener(Configurable* configurable, const std::string& prefix)
      : OpenDDS::DCPS::ConfigListener(TheServiceParticipant->job_queue())
      , configurable_(*configurable)
      , prefix_(prefix)
    {
    }

    void on_data_available(InternalDataReader_rch reader) override
    {
      OpenDDS::DCPS::ConfigReader::SampleSequence samples;
      OpenDDS::DCPS::InternalSampleInfoSequence infos;
      reader->read(samples, infos, DDS::LENGTH_UNLIMITED,
                   DDS::NOT_READ_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
      for (size_t idx = 0; idx != samples.size(); ++idx) {
        const auto& info = infos[idx];
        if (info.valid_data) {
          const auto& pair = samples[idx];
          // Match key to prefix and pass rest of key as a short name
          if (pair.key().substr(0, prefix_.length()) == prefix_) {
            const auto name = pair.key().substr(prefix_.length());
            if (!configurable_.got_config(name, pair)) {
              ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: Configurable::ConfigListener::on_data_available: "
                "%C is not a valid property for %C\n",
                name.c_str(), configurable_.config_prefix().c_str()));
            }
          }
        }
      }
    }

  private:
    Configurable& configurable_;
    const std::string prefix_;
  };

  mutable Mutex config_lock_;
  const std::string config_prefix_;
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::ConfigReader> config_reader_;
  OpenDDS::DCPS::RcHandle<ConfigListener> config_listener_;
};

#endif
