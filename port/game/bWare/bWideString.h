// Wide (16-bit) string helpers used by the frontend. Wide characters are game data (UCS-2), so they are
// uint16_t everywhere, not wchar_t (which is 32-bit on Linux).
#pragma once
#include <cstdint>

// Length in characters; null counts as 0.
int bWideStrLen(const uint16_t *s);

// Copies src to dst including the terminator. Null dst: does nothing. Null src: dst becomes "".
// GC CopyString<short> 8030C2A4 / PC 511630 (__cdecl)
void CopyString(uint16_t *dst, const uint16_t *src);

// Widens 8-bit text by SIGN-extending each byte (bytes >= 0x80 become 0xFF80..0xFFFF), as the game does.
// No null checks. Returns dst. GC bStrCpy(short*, char const*) / PC 43E000 (__cdecl)
uint16_t *bStrCpy(uint16_t *dst, const char *src);
