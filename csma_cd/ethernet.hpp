#pragma once

#include <optional>

#include "frame.hpp"
#include "logger.hpp"
#include "station.hpp"

namespace csma_cd {

class Ethernet {
 public:
  Ethernet(size_t stations_count, std::vector<Payload>&& payload,
           std::ostream& log_stream);

  std::optional<Frame> GetFrameFromBus() const;

  bool IsJammed() const;

  bool IsFree() const;

  bool IsIdle() const;

  bool IsNewFrameStart() const;

  Logger& GetLogger() const;

  void ProcessTick();

 private:
  std::pair<std::optional<Payload>, size_t> ProcessStationsTick();

 private:
  std::chrono::nanoseconds clock_;

  std::optional<Frame> bus_;
  bool is_bus_jammed_;
  size_t send_timer_;

  std::vector<Station> stations_;
  mutable Logger logger_;
};

}  // namespace csma_cd
