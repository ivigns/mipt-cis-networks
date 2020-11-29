#include "frame.hpp"

#include <string>

#include "utils.hpp"

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
  utils::InsertAddress(dst_id, destination_address);

  std::strcpy(reinterpret_cast<char*>(data.data()), payload_data.data());

  // CRC-32 checksum of whole frame (except for checksum field)
  checksum = utils::CRC32(0, reinterpret_cast<const uint8_t*>(this),
                          sizeof(*this) - 4);
}

}  // namespace csma_cd
