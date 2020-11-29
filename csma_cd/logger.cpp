#include "logger.hpp"

#include <iomanip>

#include "frame.hpp"
#include "station.hpp"
#include "utils.hpp"

namespace csma_cd {

Logger::Logger(std::ostream& log_stream, const std::chrono::nanoseconds& clock,
               size_t max_id)
    : log_stream_(log_stream),
      clock_(clock),
      id_width_(std::to_string(max_id).size()) {}

void Logger::LogPayload(const csma_cd::Payload& payload, size_t station_id,
                        const std::string& message) {
  LogClock();
  LogStation(station_id);
  log_stream_ << ":\t" << message << ",\tsource = ";
  LogStation(payload.src_id);
  log_stream_ << ",\tdestination = ";
  LogStation(payload.dst_id);
  log_stream_ << ",\tdata = \"" << payload.data << "\"" << std::endl;
}

void Logger::LogFrame(const Frame& frame, size_t station_id,
                      const std::string& message) {
  const auto src_id = utils::ExctractId(frame.source_address);
  const auto dst_id = utils::ExctractId(frame.destination_address);
  if (src_id && dst_id) {
    Payload payload{
        *src_id, *dst_id,
        std::string(reinterpret_cast<const char*>(frame.data.data()))};
    LogPayload(payload, station_id, message);
  } else {
    LogClock();
    LogStation(station_id);
    log_stream_ << ":\t!!! corrupted frame,\tmissed message = \"" << message
                << "\"" << std::endl;
  }
}

void Logger::LogMessage(size_t station_id, const std::string& message) {
  LogClock();
  LogStation(station_id);
  log_stream_ << ":\t" << message << std::endl;
}

void Logger::LogBusMessage(const std::string& message) {
  LogClock();
  log_stream_ << "-- bus --:\t" << message << std::endl;
}

void Logger::LogClock() {
  const std::chrono::hours hours =
      std::chrono::floor<std::chrono::hours>(clock_);
  const std::chrono::minutes minutes =
      std::chrono::floor<std::chrono::minutes>(clock_) - hours;
  const std::chrono::seconds seconds =
      std::chrono::floor<std::chrono::seconds>(clock_) - hours - minutes;
  const std::chrono::milliseconds millis =
      std::chrono::floor<std::chrono::milliseconds>(clock_) - hours - minutes -
      seconds;
  const std::chrono::microseconds micros =
      std::chrono::floor<std::chrono::microseconds>(clock_) - hours - minutes -
      seconds - millis;
  log_stream_ << std::setfill('0') << std::setw(2) << hours.count() << ":"
              << std::setw(2) << minutes.count() << ":" << std::setw(2)
              << seconds.count() << "." << std::setw(3) << millis.count()
              << std::setw(3) << micros.count() << ":\t";
}

void Logger::LogStation(size_t id) {
  if (id >= kMaxStationsCount) {
    log_stream_ << "all stations";
  } else {
    log_stream_ << "station " << std::setfill(' ') << std::setw(id_width_)
                << id;
  }
}

}  // namespace csma_cd
