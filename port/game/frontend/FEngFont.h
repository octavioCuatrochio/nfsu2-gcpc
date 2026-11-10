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
    uint32_t mu32KernTableOffset;    // +0x18 kerning table (16-byte glyphs: entries per glyph; else count + list)

    const struct FEngGlyph *FindCharacter(uint16_t c) const;          // PC 4FDA70 (thiscall, ret 4)
    int32_t GetKerning(const struct FEngGlyph *g, uint16_t prev) const;   // PC 6F45F0 (thiscall, ret 8)
};

// Glyph entry (12 or 16 bytes).
struct FEngGlyph {
    uint16_t mu16Code;               // +0x00
    uint8_t _unknown02[9];
    uint8_t mu8NumKerns;             // +0x0B
    uint16_t mu16KernIndex;          // +0x0C (16-byte glyphs)
    int16_t mi16Advance;             // +0x0E
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
    // PC 50BCE0 (ret 0x10; GC 8014CA0C): width of the first line (advances + kerning; '$name$' joy-event tokens
    // count 0). maxWidth != 0 (unsigned): once reached before the end, the width falls back to the last space (flag
    // 0x200: the width before the current character) when > 0, and *truncated = true.
    long double GetLineWidth(const uint16_t *text, uint32_t flags, uint32_t maxWidth, bool *truncated);
    // PC 50BC10 (ret 0xC; GC 8014C878): advance of `c` (+ kerning after `prev` when prev != 0); newline / CR /
    // unknown glyph: 0. Same character remaps as GetLineWidth (0xA0 with flag 0x80: space, no kerning).
    long double GetCharacterWidth(uint16_t c, uint16_t prev, uint32_t flags);
    // PC 50C080 (ret 0xC; GC 8014CF54): lines when wrapping at `maxWidth` (unsigned) on spaces. Flag 0x20: 1. A
    // newline only breaks after an earlier space, and measuring restarts after that space (PC behaviour).
    uint32_t GetNumWordWrappedLines(const uint16_t *text, uint32_t flags, uint32_t maxWidth);
    // PC 50BB10 (ret 4; GC 8014BF7C): texture of a "$name$" token: "ICON_SPACER" -> hash 0xBCA80379, else
    // GetJoyEventTexture(name, port) for the first plugged pad of 0/1 (none: first port whose byte at 0x7F9414 is
    // not 0xFF, else 0); not a token: GetTextureInfo(0). Returns the game's TextureInfo.
    void *GetJoyEventTextureInfo(const uint16_t *text);
    static long double CalculateXOffset(uint32_t justify, float width);    // PC 50C020: flag 1 -> -w/2, 2 -> -w
    static long double CalculateYOffset(uint32_t justify, float height);   // PC 50C050: flag 4 -> -h/2, 8 -> -h
};

// PC 50BA70 / 50BAC0 (__cdecl, GC FEngFont members): "$name$" button-icon tokens (not with flags 0x820).
bool FEngIsJoyEventTexture(const uint16_t *text, uint32_t flags);
const uint16_t *FEngSkipJoyEventTexture(const uint16_t *text, uint32_t flags);
// PC 4FF980 (__cdecl): copies the characters in [-128, 127] (low byte) into `out`, at most size - 1, then a
// terminator; null `out` or `wide`: nothing written.
void FEngWideToCharString(char *out, uint32_t size, const uint16_t *wide);
// PC 6F45B0 (__cdecl): binary search of `count` entries of `size` bytes (16-bit key first) for `key`; null if none.
const void *FEngFindGlyph(uint16_t key, const void *table, uint32_t count, uint32_t size);

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(offsetof(FEngFont, mpData) == 0x0C && offsetof(FEngFont, mfLineHeight) == 0x24);
static_assert(offsetof(FEngFontData, mu32GlyphTableOffset) == 0x14);
#endif
