#include "protocol.hpp"

#include <chrono>

#include "crc32.hpp"

namespace {

constexpr size_t kMaxStationsCount = 1024;
constexpr std::array<csma_cd::Byte, 6> kBroadcastAddress = {0xff, 0xff, 0xff,
                                                            0xff, 0xff, 0xff};
const auto kProcessStart = std::chrono::system_clock::from_time_t(0);

void InsertAddress(size_t id, std::array<csma_cd::Byte, 6>& address) {
  for (size_t i = 5; i >= 3; --i) {
    address[i] = id & 0xffu;
    id >>= 4u;
  }
}

size_t ExctractId(const std::array<csma_cd::Byte, 6>& address) {
  size_t id = 0;
  for (size_t i = 3; i < 6; ++i) {
    id |= address[i];
    id <<= 4u;
  }
  return id;
}

}  // namespace

namespace csma_cd {

Frame::Frame(size_t src_id, size_t dst_id, const std::string& payload_data)
    : preamble({0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa}),
      start_of_frame_delim(0xab),
      destination_address({0x00, 0xba, 0xba, 0x00, 0x00, 0x00}),
      source_address({0x00, 0xba, 0xba, 0x00, 0x00, 0x00}),
      data({0}),
      length(1500) {
  /* address:
   * first bit - 1 if address is broadcast
   * second bit - 1 if local, 0 if centralized
   * first 3 bytes - org unique id
   * last 3 bytes - address */
  if (src_id >= kMaxStationsCount) {
    throw std::invalid_argument(
        "Bad payload: source address cannot be of broadcast type");
  }
  InsertAddress(src_id, source_address);
  if (dst_id >= kMaxStationsCount) {
    source_address = kBroadcastAddress;
  } else {
    InsertAddress(dst_id, destination_address);
  }

  std::strcpy(reinterpret_cast<char*>(data.data()), payload_data.data());

  // checksum is crc32 of the rest of all frame
  checksum = crc32::crc32(0, reinterpret_cast<const uint8_t*>(this),
                          sizeof(*this) - 4);
}

Logger::Logger(std::ostream& log_stream,
               const std::chrono::time_point<std::chrono::system_clock>& clock)
    : log_stream_(log_stream), clock_(clock) {}

void Logger::LogNewPayload(const Payload& payload) {
  LogClock();
  log_stream_ << "station " << payload.src_id << ":\tstart sending data \""
              << payload.data << "\" to station " << payload.dst_id
              << std::endl;
}

void Logger::LogClock() {
  const std::chrono::hours hours =
      std::chrono::duration_cast<std::chrono::hours>(clock_.time_since_epoch());
  const std::chrono::minutes minutes =
      std::chrono::duration_cast<std::chrono::minutes>(
          clock_.time_since_epoch()) -
      hours;
  const std::chrono::seconds seconds =
      std::chrono::duration_cast<std::chrono::seconds>(
          clock_.time_since_epoch()) -
      minutes - hours;
  const std::chrono::milliseconds millis =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          clock_.time_since_epoch()) -
      seconds - minutes - hours;
  const std::chrono::microseconds micros =
      std::chrono::duration_cast<std::chrono::microseconds>(
          clock_.time_since_epoch()) -
      seconds - minutes - hours - millis;
  log_stream_ << hours.count() << ":" << minutes.count() << ":"
              << seconds.count() << "." << millis.count() << micros.count()
              << ":\t";
}

Station::Station(const Ethernet& ethernet)
    : ethernet_(ethernet), sleep_timer_(0), logger_(ethernet.GetLogger()) {}

void Station::AddPayload(Payload&& payload) {
  payload_.push_back(std::move(payload));
}

std::optional<Payload> Station::ProcessTick() { return std::nullopt; }

Ethernet::Ethernet(size_t stations_count, std::vector<Payload>&& payload,
                   std::ostream& log_stream)
    : clock_(kProcessStart),
      tick_clock_(0),
      bus_jammed_(false),
      send_timer_(0),
      logger_(log_stream, clock_) {
  if (stations_count > kMaxStationsCount) {
    throw std::invalid_argument(
        "Too many stations to create, max count is 1024");
  }
  stations_ = std::vector(stations_count, Station(*this));
  for (auto&& station_payload : payload) {
    if (station_payload.src_id >= stations_.size()) {
      throw std::invalid_argument(
          "Bad payload: source id points on nonexistent station");
    }
    stations_[station_payload.src_id].AddPayload(std::move(station_payload));
  }
}

std::optional<size_t> Ethernet::GetCurrentSender() const {
  return bus_ ? std::optional(ExctractId(bus_->source_address)) : std::nullopt;
}

bool Ethernet::IsJammed() const { return bus_jammed_; }

bool Ethernet::IsIdle() const {
  if (bus_jammed_ || bus_) {
    return false;
  }
  for (const auto& station : stations_) {
    if (true) {
    }
  }
}

Logger& Ethernet::GetLogger() const { return logger_; }

}  // namespace csma_cd
