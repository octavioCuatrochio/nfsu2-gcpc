// FEngFont text measurement (PC; GC zFe2.cpp FEngFont). Only the fields the rewritten members use.
#pragma once
#include <cstddef>
#include <cstdint>

// Font file data (FEngFont +0x0C): glyph table of `mu16NumGlyphs` entries (12 bytes, 16 with flag 0x40000), first
// field the character code; entry for code c usually at index c - 0x20.
struct FEngFontData {
    uint8_t _unknown00[0x0A];
    uint16_t mu16NumGlyphs;          // +0x0A
    uint32_t mu32Flags;              // +0x0C 0x40000: 16-byte glyph entries
    uint8_t _unknown10[4];
    uint32_t mu32GlyphTableOffset;   // +0x14 from the start of the data
};

struct FEngFont {
    uint8_t _unknown00[0x0C];
    FEngFontData *mpData;            // +0x0C
    uint8_t _unknown10[0x14];
    float mfLineHeight;              // +0x24

    // PC 50BEC0 (ret 8; GC 8014CCC8): widest of GetLineWidth(text), then GetLineWidth from after every character
    // (flag 0x20 clear: newlines '\n' / '^' skipped) — the PC measures every suffix, not only line starts.
    long double GetTextWidth(const uint16_t *text, uint32_t flags);
    // PC 50BF40 (ret 8; GC 8014CD94): per newline: spacing + line height; a final line counts when it has a glyph.
    long double GetTextHeight(const uint16_t *text, int32_t lineSpacing);
    static long double CalculateXOffset(uint32_t justify, float width);    // PC 50C020: flag 1 -> -w/2, 2 -> -w
    static long double CalculateYOffset(uint32_t justify, float height);   // PC 50C050: flag 4 -> -h/2, 8 -> -h
};

// PC 6F45B0 (__cdecl): binary search of `count` entries of `size` bytes (16-bit key first) for `key`; null if none.
const void *FEngFindGlyph(uint16_t key, const void *table, uint32_t count, uint32_t size);

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(offsetof(FEngFont, mpData) == 0x0C && offsetof(FEngFont, mfLineHeight) == 0x24);
static_assert(offsetof(FEngFontData, mu32GlyphTableOffset) == 0x14);
#endif
