#include "protocol.hpp"

#include <cmath>

#include <chrono>

#include "utils.hpp"

namespace {

constexpr std::array<csma_cd::Byte, 6> kBroadcastAddress = {0x80, 0xba, 0xba,
                                                            0xff, 0xff, 0xff};
constexpr auto kProcessStart = std::chrono::nanoseconds(0);
constexpr size_t kMaxSleepIncrease = 10;
constexpr size_t kMaxRetries = 16;
constexpr size_t kFrameLengthInTicks = 24;  // 1526 * 8 / 512
constexpr auto kTickDuration = std::chrono::nanoseconds(51200);

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
  utils::InsertAddress(src_id, source_address);
  if (dst_id >= kMaxStationsCount) {
    source_address = kBroadcastAddress;
  } else {
    utils::InsertAddress(dst_id, destination_address);
  }

  std::strcpy(reinterpret_cast<char*>(data.data()), payload_data.data());

  // CRC-32 checksum of whole frame (except for checksum field)
  checksum = utils::CRC32(0, reinterpret_cast<const uint8_t*>(this),
                          sizeof(*this) - 4);
}

Station::Station(size_t id, const Ethernet& ethernet, int rand_seed)
    : id_(id),
      sleep_timer_(0),
      is_sending_frame_(false),
      retry_count_(0),
      rand_gen_(rand_seed),
      ethernet_(ethernet),
      logger_(ethernet.GetLogger()) {}

void Station::AddPayload(Payload&& payload) {
  payload_queue_.push(std::move(payload));
}

bool Station::IsIdle() const {
  return sleep_timer_ == 0 && !is_sending_frame_ && payload_queue_.empty();
}

std::optional<Payload> Station::ProcessTick() {
  // Try receive frame from bus
  std::optional<Frame> bus_frame = ethernet_.GetFrameFromBus();
  if (bus_frame && !ethernet_.IsJammed()) {
    const auto checksum =
        utils::CRC32(0, reinterpret_cast<const uint8_t*>(&*bus_frame),
                     sizeof(*bus_frame) - 4);
    if (bus_frame->start_of_frame_delim == 0xab &&
        checksum == bus_frame->checksum) {
      const auto dst_id = utils::ExctractId(bus_frame->destination_address);
      const auto is_broadcast = dst_id && *dst_id >= kMaxStationsCount &&
                                (bus_frame->destination_address[0] >> 7u);
      if (is_broadcast || dst_id == id_) {
        logger_.LogFrame(*bus_frame, id_, "successfully received frame");
      }
    }
  }

  // Continue sleep if needed
  if (sleep_timer_) {
    --sleep_timer_;
    return std::nullopt;
  }

  // If sending frame, check for collision in bus
  if (is_sending_frame_) {
    if (ethernet_.IsJammed()) {
      is_sending_frame_ = false;

      if (++retry_count_ > kMaxRetries) {
        logger_.LogPayload(payload_queue_.front(), id_,
                           "max retries exceeded while sending frame");
        retry_count_ = 0;
        payload_queue_.pop();
        return std::nullopt;
      }

      logger_.LogMessage(id_, "retry count = " + std::to_string(retry_count_));
      StartSleep();
      return std::nullopt;
    }

    if (ethernet_.IsFree()) {
      logger_.LogPayload(payload_queue_.front(), id_, "finish sending frame");
      is_sending_frame_ = false;
      retry_count_ = 0;
      payload_queue_.pop();
    } else {
      return std::nullopt;
    }
  }

  // Try send payload from queue
  if (!payload_queue_.empty()) {
    if (ethernet_.IsFree()) {
      is_sending_frame_ = true;
      logger_.LogPayload(payload_queue_.front(), id_, "start sending frame");
      return payload_queue_.front();
    }

    StartSleep();
  }

  // Idle tick
  return std::nullopt;
}

void Station::StartSleep() {
  const size_t max_delay =
      std::pow(2, std::min(kMaxSleepIncrease, retry_count_));
  std::uniform_int_distribution<size_t> delay(0, max_delay);
  sleep_timer_ = delay(rand_gen_);
}

Ethernet::Ethernet(size_t stations_count, std::vector<Payload>&& payload,
                   std::ostream& log_stream)
    : clock_(kProcessStart),
      is_bus_jammed_(false),
      send_timer_(0),
      logger_(log_stream, clock_) {
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
      throw std::invalid_argument(
          "Bad payload: source id points on nonexistent station");
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

Logger& Ethernet::GetLogger() const { return logger_; }

void Ethernet::ProcessTick() {
  std::optional<Payload> payload;
  size_t frequency_rate = !IsFree();

  // Process tick in stations
  for (size_t id = 0; id < stations_.size(); ++id) {
    const auto prev_idle_status = stations_[id].IsIdle();
    auto new_payload = stations_[id].ProcessTick();
    if (new_payload) {
      ++frequency_rate;
      if (frequency_rate > 1) {
        logger_.LogBusMessage("!!! collision !!!, rate " +
                              std::to_string(frequency_rate));
      }
    }
    payload = std::move(new_payload);
    // Check if station has payload to send
    if (stations_[id].IsIdle() && stations_[id].IsIdle() != prev_idle_status) {
      logger_.LogBusMessage("station " + std::to_string(id) + " now idle");
    }
  }

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

}  // namespace csma_cd
