#pragma once

#include <array>

#include "consts.hpp"

namespace csma_cd {

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

}  // namespace csma_cd
