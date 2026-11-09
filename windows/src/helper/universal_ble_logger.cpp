#include "universal_ble_logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace universal_ble {

std::atomic<BleLogLevel> UniversalBleLogger::current_level_{BleLogLevel::kNone};
std::mutex UniversalBleLogger::sink_mutex_;
const void *UniversalBleLogger::sink_owner_ = nullptr;
UniversalBleLogger::Sink UniversalBleLogger::sink_;

void UniversalBleLogger::SetSink(const void *owner, Sink sink) {
  std::lock_guard<std::mutex> lock(sink_mutex_);
  sink_owner_ = owner;
  sink_ = std::move(sink);
}

void UniversalBleLogger::ClearSink(const void *owner) {
  std::lock_guard<std::mutex> lock(sink_mutex_);
  if (sink_owner_ != owner)
    return;
  sink_owner_ = nullptr;
  sink_ = nullptr;
}

void UniversalBleLogger::Forward(BleLogLevel level,
                                 const std::string &message) {
  // A sink that logs would re-enter here on the same thread and deadlock on
  // the non-recursive mutex; drop the nested message instead.
  thread_local bool forwarding = false;
  if (forwarding)
    return;
  forwarding = true;
  try {
    // Called under the lock so ClearSink (plugin teardown) cannot destroy
    // the sink while it runs.
    std::lock_guard<std::mutex> lock(sink_mutex_);
    if (sink_)
      sink_(level, message);
  } catch (...) {
  }
  forwarding = false;
}

static std::string GetCurrentTimestampString() {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;

  std::tm timeinfo;
  localtime_s(&timeinfo, &time_t);

  std::ostringstream oss;
  oss << std::put_time(&timeinfo, "%H:%M:%S");
  oss << "." << std::setfill('0') << std::setw(3) << ms.count();
  return "[" + oss.str() + "]";
}

void UniversalBleLogger::SetLogLevel(BleLogLevel level) {
  current_level_.store(level, std::memory_order_relaxed);
}

BleLogLevel UniversalBleLogger::current_log_level() {
  return current_level_.load(std::memory_order_relaxed);
}

void UniversalBleLogger::LogError(const std::string &message) {
  Forward(BleLogLevel::kError, message);
  if (!Allows(BleLogLevel::kError))
    return;
  std::cout << "UniversalBle:ERROR " << message << std::endl;
}

void UniversalBleLogger::LogWarning(const std::string &message) {
  Forward(BleLogLevel::kWarning, message);
  if (!Allows(BleLogLevel::kWarning))
    return;
  std::cout << "UniversalBle:WARN " << message << std::endl;
}

void UniversalBleLogger::LogInfo(const std::string &message) {
  if (!Allows(BleLogLevel::kInfo))
    return;
  std::cout << "UniversalBle:INFO " << message << std::endl;
}

void UniversalBleLogger::LogDebug(const std::string &message) {
  if (!Allows(BleLogLevel::kDebug))
    return;
  std::cout << "UniversalBle:DEBUG " << message << std::endl;
}

void UniversalBleLogger::LogVerbose(const std::string &message) {
  if (!Allows(BleLogLevel::kVerbose))
    return;
  std::cout << "UniversalBle:VERBOSE " << message << std::endl;
}

void UniversalBleLogger::LogDebugWithTimestamp(const std::string &message) {
  if (!Allows(BleLogLevel::kDebug))
    return;
  std::cout << "UniversalBle:DEBUG " << GetCurrentTimestampString() << " "
            << message << std::endl;
}

void UniversalBleLogger::LogVerboseWithTimestamp(const std::string &message) {
  if (!Allows(BleLogLevel::kVerbose))
    return;
  std::cout << "UniversalBle:VERBOSE " << GetCurrentTimestampString() << " "
            << message << std::endl;
}

bool UniversalBleLogger::Allows(BleLogLevel level) {
  const auto current_level = current_level_.load(std::memory_order_relaxed);
  return current_level != BleLogLevel::kNone &&
         static_cast<int>(level) <= static_cast<int>(current_level);
}

}  // namespace universal_ble
