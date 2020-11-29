#pragma once

#include <array>
#include <chrono>

namespace csma_cd {

using Byte = uint8_t;

static constexpr std::array<csma_cd::Byte, 6> kBroadcastAddress = {
    0x80, 0xba, 0xba, 0xff, 0xff, 0xff};
static constexpr size_t kMaxStationsCount = 1024;
static constexpr size_t kMaxSleepIncrease = 10;
static constexpr size_t kMaxRetries = 16;
static constexpr auto kProcessStart = std::chrono::nanoseconds(0);
static constexpr size_t kFrameLengthInTicks = 24;  // 1526 * 8 / 512
static constexpr auto kTickDuration = std::chrono::nanoseconds(51200);

}  // namespace csma_cd
