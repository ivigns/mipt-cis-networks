#pragma once

#include <ostream>

namespace csma_cd {

class Payload;
class Frame;

class Logger {
 public:
  Logger(std::ostream& log_stream, const std::chrono::nanoseconds& clock);

  void LogPayload(const csma_cd::Payload& payload, size_t station_id,
                  const std::string& message);
  void LogFrame(const csma_cd::Frame& frame, size_t station_id,
                const std::string& message);
  void LogMessage(size_t station_id, const std::string& message);
  void LogBusMessage(const std::string& message);

 private:
  void LogClock();

 private:
  std::ostream& log_stream_;
  const std::chrono::nanoseconds& clock_;
};

}  // namespace csma_cd
