#pragma once

#include <atomic>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>

#include "../generated/universal_ble.g.h"

namespace universal_ble {

class UniversalBleLogger {
public:
  using Sink = std::function<void(BleLogLevel, const std::string &)>;

  // Errors and warnings also go to [sink], whatever the log level: stdout,
  // where the level-gated log goes, does not exist in a release Windows app.
  // The sink must not log itself, and must be cheap (it runs under a lock on
  // the logging thread); the plugin's posts the message to the UI thread.
  // [owner] identifies the plugin instance, so only the instance that
  // installed a sink can clear it.
  static void SetSink(const void *owner, Sink sink);
  static void ClearSink(const void *owner);

  static void SetLogLevel(BleLogLevel level);
  static BleLogLevel current_log_level();

  static void LogError(const std::string &message);
  static void LogWarning(const std::string &message);
  static void LogInfo(const std::string &message);
  static void LogDebug(const std::string &message);
  static void LogVerbose(const std::string &message);
  static void LogDebugWithTimestamp(const std::string &message);
  static void LogVerboseWithTimestamp(const std::string &message);

private:
  static std::atomic<BleLogLevel> current_level_;
  static bool Allows(BleLogLevel level);
  static void Forward(BleLogLevel level, const std::string &message);

  static std::mutex sink_mutex_;
  static const void *sink_owner_;
  static Sink sink_;
};

}  // namespace universal_ble
