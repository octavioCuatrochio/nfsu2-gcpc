#include "FEngFont.h"
#include <cstddef>
#include "bridge/Bridge.h"

using L = long double;

const void *FEngFindGlyph(uint16_t key, const void *table, uint32_t count, uint32_t size)
{
    const uint8_t *base = static_cast<const uint8_t *>(table);
    while (count) {
        const uint8_t *mid = base + (count >> 1) * size;
        const int32_t diff = static_cast<int32_t>(key) - *reinterpret_cast<const uint16_t *>(mid);
        if (diff == 0) return mid;
        if (diff > 0) {
            base = mid + size;
            --count;
        }
        count >>= 1;
    }
    return nullptr;
}

long double FEngFont::CalculateXOffset(uint32_t justify, float width)
{
    if (justify & 1) return L(width) * -0.5f;
    if (justify & 2) return -L(width);
    return 0.0L;
}

long double FEngFont::CalculateYOffset(uint32_t justify, float height)
{
    if (justify & 4) return L(height) * -0.5f;
    if (justify & 8) return -L(height);
    return 0.0L;
}

long double FEngFont::GetTextWidth(const uint16_t *text, uint32_t flags)
{
    float widest = static_cast<float>(GetLineWidth(text, 0, 0, nullptr));
    const uint16_t *p = text;
    for (uint16_t c = *p++; c; c = *p++) {
        if (!(flags & 0x20) && (c == '\n' || c == '^')) continue;
        const L w = GetLineWidth(p, 0, 0, nullptr);
        if (w > widest) widest = static_cast<float>(w);
    }
    return widest;
}

long double FEngFont::GetTextHeight(const uint16_t *text, int32_t lineSpacing)
{
    if (!text) return 0.0L;
    float h = 0.0f;
    bool pendingLine = true;
    for (uint16_t c = *text++; c; c = *text++) {
        if (c == '\n' || c == '^') {
            h = static_cast<float>((L(lineSpacing) + h) + mfLineHeight);
            pendingLine = false;
        } else if (c != '\r') {
            const FEngFontData *d = mpData;
            const uint8_t *table = reinterpret_cast<const uint8_t *>(d) + d->mu32GlyphTableOffset;
            const uint32_t size = (d->mu32Flags & 0x40000) ? 16 : 12;
            const uint16_t key = c & 0xFF;   // PC: only the low byte is looked up
            const int32_t guess = static_cast<int32_t>(key) - 0x20;
            const void *g = nullptr;
            if (guess < static_cast<int32_t>(d->mu16NumGlyphs) && *reinterpret_cast<const uint16_t *>(table + static_cast<ptrdiff_t>(guess) * static_cast<ptrdiff_t>(size)) == key)
                g = table + static_cast<ptrdiff_t>(guess) * static_cast<ptrdiff_t>(size);
            else
                g = FEngFindGlyph(key, table, d->mu16NumGlyphs, size);
            if (g) pendingLine = true;
        }
    }
    if (pendingLine) h = static_cast<float>(L(h) + mfLineHeight);
    return h;
}

const FEngGlyph *FEngFontData::FindCharacter(uint16_t c) const
{
    const uint8_t *table = reinterpret_cast<const uint8_t *>(this) + mu32GlyphTableOffset;
    const uint32_t size = (mu32Flags & 0x40000) ? 16 : 12;
    const int32_t guess = static_cast<int32_t>(c) - 0x20;
    if (guess < static_cast<int32_t>(mu16NumGlyphs) && *reinterpret_cast<const uint16_t *>(table + static_cast<ptrdiff_t>(guess) * static_cast<ptrdiff_t>(size)) == c)
        return reinterpret_cast<const FEngGlyph *>(table + static_cast<ptrdiff_t>(guess) * static_cast<ptrdiff_t>(size));
    return static_cast<const FEngGlyph *>(FEngFindGlyph(c, table, mu16NumGlyphs, size));
}

int32_t FEngFontData::GetKerning(const FEngGlyph *g, uint16_t prev) const
{
    if (!g->mu8NumKerns) return 0;
    const uint8_t *kt = reinterpret_cast<const uint8_t *>(this) + mu32KernTableOffset;
    if (mu32Flags & 0x40000) {   // per-glyph list: {u16 previous char, s8 amount, pad}
        const uint8_t *e = kt + 4 + g->mu16KernIndex * 4;
        for (int i = 0; i < g->mu8NumKerns; ++i, e += 4)
            if (*reinterpret_cast<const uint16_t *>(e) == prev) return static_cast<int8_t>(e[2]);
        return 0;
    }
    // global list: count, then {u16 previous char, s8 amount, u8 glyph code}
    const int32_t n = *reinterpret_cast<const int32_t *>(kt);
    for (int32_t i = 0; i < n; ++i) {
        const uint8_t *e = kt + 4 + i * 4;
        if (*reinterpret_cast<const uint16_t *>(e) == prev && e[3] == g->mu16Code) return static_cast<int8_t>(e[2]);
    }
    return 0;
}

bool FEngIsJoyEventTexture(const uint16_t *text, uint32_t flags)
{
    if (!text || (flags & 0x820) || text[0] != '$') return false;
    int n = 0;
    for (const uint16_t *p = text + 1; *p && *p != '$'; ++p) ++n;
    return n != 0;
}

const uint16_t *FEngSkipJoyEventTexture(const uint16_t *text, uint32_t flags)
{
    if (!text || (flags & 0x820) || text[0] != '$') return text;
    const uint16_t *p = text + 1;
    if (*p == '$') return p;
    while (*p && *p != '$') ++p;
    return p + 1;   // past the closing '$' (or past the terminator, like the original)
}

long double FEngFont::GetLineWidth(const uint16_t *text, uint32_t flags, uint32_t maxWidth, bool *truncated)
{
    if (!text) return 0.0L;
    float w = 0.0f, lastSpace = 0.0f;
    bool skipped = false;
    uint32_t index = 0;   // characters since the last reset (kerning needs a previous one)
    const uint16_t *p = text;
    for (uint16_t c = *p; c; c = *p, ++index) {
        if (skipped) { index = 0; skipped = false; }
        const float before = w;
        if (!(flags & 0x20) && (c == '\n' || c == '^')) return w;
        if (c == '\r') { ++p; continue; }
        if (c == ' ') lastSpace = w;
        if ((flags & 0x80) && c == 0xA0) { index = 0; c = 0x20; }
        if (FEngIsJoyEventTexture(p, flags)) {
            p = FEngSkipJoyEventTexture(p, flags);
            skipped = true;
        } else {
            if (c >= 0xFF80) c &= 0xFF;
            if (c == 0x99) c = 0x2122;
            else if (c == 0x9C) c = 0x153;
            if (const FEngGlyph *g = mpData->FindCharacter(c)) {
                const L base = index ? L(mpData->GetKerning(g, p[-1])) + w : L(w);
                w = static_cast<float>(L(g->mi16Advance) + base);
            }
            ++p;
        }
        if (maxWidth && *p) {
            const L limit = L(maxWidth);   // (unsigned, like the original's fild + 2^32)
            if (!(limit > w) && !(w != w)) {   // reached (w >= limit), NaN excluded
                const float v = (flags & 0x200) ? before : lastSpace;
                if (v > 0.0f) w = v;
                if (truncated) *truncated = true;
                return w;
            }
        }
    }
    return w;
}
