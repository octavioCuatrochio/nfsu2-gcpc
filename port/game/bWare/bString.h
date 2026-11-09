// bWare string utilities (EA "bWare" core library).
// Original: zBWare.cpp (GameCube Speed.elf), behavior-equivalent rewrite.
#pragma once
#include <cstdint>

// Case-sensitive string hash used for nearly every asset/name lookup in the game.
// hash = hash * 33 + c over the bytes of str, starting from `seed`.
// GC 8005DC10 / PC 43DB80
uint32_t bStringHash(const char *str, uint32_t seed);

// Same with the default seed 0xFFFFFFFF. GC 8005DBE0 / PC 43DB50
uint32_t bStringHash(const char *str);
