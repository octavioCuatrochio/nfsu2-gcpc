#include "bCrc32.h"
#include <array>

namespace {

constexpr std::array<uint32_t, 256> MakeTable()
{
    std::array<uint32_t, 256> t{};
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t c = i << 24;
        for (int k = 0; k < 8; ++k) c = (c & 0x80000000u) ? (c << 1) ^ 0x04C11DB7u : c << 1;
        t[i] = c;
    }
    return t;
}

// identical to the table at PC 0x7868E8 (verified)
constexpr std::array<uint32_t, 256> kTable = MakeTable();

}  // namespace

uint32_t bCalculateCrc32(const void *data, int length, uint32_t seed)
{
    uint32_t crc = seed;
    if (length > 0) {
        const auto *p = static_cast<const unsigned char *>(data);
        for (int i = 0; i < length; ++i) crc = (crc << 8) ^ kTable[(crc >> 24) ^ p[i]];
        for (int n = length; n > 0; n >>= 8) crc = (crc << 8) ^ kTable[((crc >> 24) ^ static_cast<uint32_t>(n)) & 0xFF];
    }
    return ~crc;
}
