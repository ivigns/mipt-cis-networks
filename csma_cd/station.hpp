#pragma once

#include <optional>
#include <queue>
#include <random>

#include "consts.hpp"
#include "logger.hpp"

namespace csma_cd {

class Ethernet;

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
  void ProcessReceive();

  std::optional<Payload> ProcessSend();

  void StartSleep();

  void ForceStopReceive();

  void ForceStopSend();

 private:
  const size_t id_;
  std::queue<Payload> payload_queue_;

  size_t sleep_timer_;
  bool is_receiving_frame_;
  bool is_sending_frame_;
  size_t retry_count_;

  std::mt19937 rand_gen_;
  const Ethernet& ethernet_;
  mutable Logger logger_;
};

}  // namespace csma_cd
