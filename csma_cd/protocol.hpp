#pragma once

#include <array>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace csma_cd {

using Byte = uint8_t;

class Ethernet;

struct Frame {
  Frame(size_t src_id, size_t dst_id, const std::string& task_data);
  std::array<Byte, 7> preamble;
  Byte start_of_frame_delim;
  std::array<Byte, 6> destination_address;
  std::array<Byte, 6> source_address;
  uint16_t length;
  std::array<Byte, 1500> data;
  uint32_t checksum;
};

struct Payload {
  size_t src_id;
  size_t dst_id;
  std::string data;
};

class Logger {
 public:
  Logger(std::ostream& log_stream,
         const std::chrono::time_point<std::chrono::system_clock>& clock);

  void LogNewPayload(const Payload& payload);
  void LogJam();
  void LogFrameSendFinish();
  void LogReceivedFrame(const Frame& frame);

 private:
  void LogClock();

  std::ostream& log_stream_;
  const std::chrono::time_point<std::chrono::system_clock>& clock_;
};

class Station {
 public:
  explicit Station(const Ethernet& ethernet);

  void AddPayload(Payload&& payload);

  std::optional<Payload> ProcessTick();

 private:
  const Ethernet& ethernet_;
  std::vector<Payload> payload_;
  size_t sleep_timer_;

  mutable Logger logger_;
};

class Ethernet {
 public:
  Ethernet(size_t stations_count, std::vector<Payload>&& payload,
           std::ostream& log_stream);

  std::optional<size_t> GetCurrentSender() const;

  bool IsJammed() const;

  bool IsIdle() const;

  void ProcessTick();

  Logger& GetLogger() const;

 private:
  std::chrono::time_point<std::chrono::system_clock> clock_;
  size_t tick_clock_;

  std::optional<Frame> bus_;
  bool bus_jammed_;
  size_t send_timer_;

  std::vector<Station> stations_;

  mutable Logger logger_;
};

}  // namespace csma_cd
