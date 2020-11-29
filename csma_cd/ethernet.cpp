#include "ethernet.hpp"

namespace csma_cd {

Ethernet::Ethernet(size_t stations_count, std::vector<Payload>&& payload,
                   std::ostream& log_stream)
    : clock_(kProcessStart),
      is_bus_jammed_(false),
      send_timer_(0),
      logger_(log_stream, clock_, stations_count - 1) {
  if (stations_count > kMaxStationsCount) {
    throw std::invalid_argument(
        "Too many stations to create, max count is 1024");
  }

  std::random_device rd;
  for (size_t id = 0; id < stations_count; ++id) {
    stations_.emplace_back(id, *this, rd());
  }

  for (auto&& station_payload : payload) {
    if (station_payload.src_id >= stations_.size()) {
      throw std::invalid_argument("Bad payload: source id " +
                                  std::to_string(station_payload.src_id) +
                                  " points on nonexistent station");
    }
    if (station_payload.dst_id >= stations_.size() &&
        station_payload.dst_id < kMaxStationsCount) {
      throw std::invalid_argument("Bad payload: destination id " +
                                  std::to_string(station_payload.dst_id) +
                                  " points on nonexistent station");
    }
    stations_[station_payload.src_id].AddPayload(std::move(station_payload));
  }
}

std::optional<Frame> Ethernet::GetFrameFromBus() const { return bus_; }

bool Ethernet::IsJammed() const { return is_bus_jammed_; }

bool Ethernet::IsFree() const { return !is_bus_jammed_ && !send_timer_; }

bool Ethernet::IsIdle() const {
  if (is_bus_jammed_ || bus_) {
    return false;
  }
  return !is_bus_jammed_ && !bus_ &&
         std::all_of(stations_.cbegin(), stations_.cend(),
                     [](const Station& station) { return station.IsIdle(); });
}

bool Ethernet::IsNewFrameStart() const {
  return send_timer_ == kFrameLengthInTicks - 1;
}

Logger& Ethernet::GetLogger() const { return logger_; }

void Ethernet::ProcessTick() {
  const auto [payload, frequency_rate] = ProcessStationsTick();

  // Reset bus after jam
  if (is_bus_jammed_) {
    is_bus_jammed_ = false;
    send_timer_ = 0;
  }
  // Reset bus after frame sending
  if (!send_timer_ && bus_) {
    bus_.reset();
  }
  // Tick timer
  if (send_timer_) {
    --send_timer_;
  }

  // Jam bus if there were collisions
  if (frequency_rate > 1) {
    is_bus_jammed_ = true;
  }
  // Load new payload to bus
  if (payload) {
    bus_ = Frame(payload->src_id, payload->dst_id, payload->data);
    send_timer_ = kFrameLengthInTicks - 1;
  }

  // Tick clock
  clock_ += kTickDuration;
}

std::pair<std::optional<Payload>, size_t> Ethernet::ProcessStationsTick() {
  std::optional<Payload> payload;
  size_t frequency_rate = !IsFree();
  for (auto& station : stations_) {
    auto new_payload = station.ProcessTick();
    // After station sends frame carrier frequency increases
    if (new_payload) {
      ++frequency_rate;
      if (frequency_rate > 1) {
        logger_.LogBusMessage("collision,\trate " +
                              std::to_string(frequency_rate));
      }
      payload = std::move(new_payload);
    }
  }
  return {payload, frequency_rate};
}

}  // namespace csma_cd
