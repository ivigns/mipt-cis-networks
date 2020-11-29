#pragma once

#include <array>
#include <optional>

#include "consts.hpp"

namespace csma_cd::utils {

void InsertAddress(size_t id, std::array<csma_cd::Byte, 6>& address);

std::optional<size_t> ExctractId(const std::array<csma_cd::Byte, 6>& address);

uint32_t CRC32(uint32_t crc, const uint8_t* data, size_t size);

}  // namespace csma_cd::utils
