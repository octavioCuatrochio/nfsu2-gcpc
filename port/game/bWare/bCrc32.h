// bWare CRC (EA core library). Original: zBWare.cpp
#pragma once
#include <cstdint>

// POSIX cksum-style CRC-32: MSB-first, polynomial 0x04C11DB7, over `length` bytes of `data`, then over
// the bytes of `length` itself (low byte first, until it is 0), result inverted.
// `length` <= 0 returns ~seed without touching data.
// GC 80058254 / PC 43BCD0 (__cdecl)
uint32_t bCalculateCrc32(const void *data, int length, uint32_t seed);
