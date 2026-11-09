// FEng (EA frontend engine) growable wide string. Original: zFe2.cpp
// Layout is shared with original game code while running hooked: do not add members or virtuals.
#pragma once
#include <cstddef>
#include <cstdint>

struct FEWideString {
    uint16_t *mpsString;        // +0x0  owned buffer (game heap), may be null
    uint32_t mulBufferLength;   // +0x4  capacity in characters, excluding the terminator

    FEWideString();
    FEWideString(const FEWideString &other);
    ~FEWideString();

    FEWideString &operator=(const FEWideString &other);
    FEWideString &operator=(const uint16_t *str);   // PC: inlined into callers
    FEWideString &operator=(const char *packed);    // PC 532670: unpacks the game's packed text

    uint32_t Length() const;                         // characters before the terminator
    void SetLength(uint32_t length);                 // PC 51DE90: grow to hold `length` chars
    uint16_t *AllocateString(uint32_t length);       // grow if capacity < length, return buffer
    uint32_t GetChecksum() const;                    // PC: inlined into callers
};
static_assert(sizeof(FEWideString) == 8 || sizeof(void *) != 4, "FEWideString must match the game layout");
static_assert(offsetof(FEWideString, mulBufferLength) == sizeof(void *));
