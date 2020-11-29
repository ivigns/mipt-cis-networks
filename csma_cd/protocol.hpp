#pragma once

#include <array>
#include <memory>
#include <optional>
#include <queue>
#include <random>
#include <string>
#include <vector>

#include "logger.hpp"

namespace csma_cd {

using Byte = uint8_t;

static constexpr size_t kMaxStationsCount = 1024;

class Ethernet;

struct Frame {
  Frame(size_t src_id, size_t dst_id, const std::string& task_data);
  [[maybe_unused]] std::array<Byte, 7> preamble;
  Byte start_of_frame_delim;
  std::array<Byte, 6> destination_address;
  std::array<Byte, 6> source_address;
  [[maybe_unused]] uint16_t length;
  std::array<Byte, 1500> data;
  uint32_t checksum;
};

struct Payload {
  size_t src_id;
  size_t dst_id;
  std::string data;
};

class Station {
 public:
  Station(size_t id, const Ethernet& ethernet, int rand_seed);

  void AddPayload(Payload&& payload);

  bool IsIdle() const;

  std::optional<Payload> ProcessTick();

 private:
  void StartSleep();

 private:
  const size_t id_;
  std::queue<Payload> payload_queue_;

  size_t sleep_timer_;
  bool is_sending_frame_;
  size_t retry_count_;

  std::mt19937 rand_gen_;
  const Ethernet& ethernet_;
  mutable Logger logger_;
};

class Ethernet {
 public:
  Ethernet(size_t stations_count, std::vector<Payload>&& payload,
           std::ostream& log_stream);

  std::optional<Frame> GetFrameFromBus() const;

  bool IsJammed() const;

  bool IsFree() const;

  bool IsIdle() const;

  Logger& GetLogger() const;

  void ProcessTick();

 private:
  std::chrono::nanoseconds clock_;

  std::optional<Frame> bus_;
  bool is_bus_jammed_;
  size_t send_timer_;

  std::vector<Station> stations_;
  mutable Logger logger_;
};

}  // namespace csma_cd
