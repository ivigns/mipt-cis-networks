#include "station.hpp"

#include "ethernet.hpp"
#include "utils.hpp"

namespace csma_cd {

Station::Station(size_t id, const Ethernet& ethernet, int rand_seed)
    : id_(id),
      sleep_timer_(0),
      is_receiving_frame_(false),
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
  ProcessReceive();
  return ProcessSend();
}

void Station::ProcessReceive() {
  // Stop receiving if collision happened
  if (ethernet_.IsJammed()) {
    is_receiving_frame_ = false;
  }
  // Try receive frame from bus
  std::optional<Frame> bus_frame = ethernet_.GetFrameFromBus();
  if (bus_frame && !ethernet_.IsJammed()) {
    const auto checksum =
        utils::CRC32(0, reinterpret_cast<const uint8_t*>(&*bus_frame),
                     sizeof(*bus_frame) - 4);
    if (bus_frame->start_of_frame_delim == 0xab &&
        checksum == bus_frame->checksum) {
      const auto src_id = utils::ExctractId(bus_frame->source_address);
      const auto dst_id = utils::ExctractId(bus_frame->destination_address);
      const auto is_broadcast = dst_id && *dst_id >= kMaxStationsCount &&
                                (bus_frame->destination_address[0] >> 7u);
      if ((is_broadcast || dst_id == id_) && src_id != id_) {
        if (ethernet_.IsNewFrameStart()) {
          ForceStopReceive();
          logger_.LogFrame(*bus_frame, id_, "start receiving frame");
          is_receiving_frame_ = true;
        } else if (ethernet_.IsFree()) {
          if (is_receiving_frame_) {
            logger_.LogFrame(*bus_frame, id_, "successfully received frame");
          } else {
            logger_.LogFrame(*bus_frame, id_, "!!! missed frame");
          }
          is_receiving_frame_ = false;
        }
      } else {
        ForceStopReceive();
      }
    } else {
      logger_.LogMessage(id_, "!!! received corrupted frame");
      ForceStopReceive();
    }
  }
}

std::optional<Payload> Station::ProcessSend() {
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
        ForceStopSend();
        return std::nullopt;
      }

      logger_.LogMessage(id_, "retry count = " + std::to_string(retry_count_));
      StartSleep();
      return std::nullopt;
    }
    if (ethernet_.IsFree()) {
      logger_.LogPayload(payload_queue_.front(), id_, "finish sending frame");
      ForceStopSend();
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

void Station::ForceStopReceive() {
  if (is_receiving_frame_) {
    logger_.LogMessage(id_, "!!! frame receive interrupt");
  }
  is_receiving_frame_ = false;
}

void Station::ForceStopSend() {
  is_sending_frame_ = false;
  retry_count_ = 0;
  payload_queue_.pop();
  if (IsIdle()) {
    logger_.LogMessage(id_, "nothing left to send");
  }
}

}  // namespace csma_cd
