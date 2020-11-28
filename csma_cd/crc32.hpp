#pragma once

#include <cstdlib>

namespace crc32 {

uint32_t crc32(uint32_t crc, const uint8_t* data, size_t size);

}  // namespace crc32
