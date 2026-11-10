#include "FEngFont.h"
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
    float widest = static_cast<float>(bridge::FEngFont_GetLineWidth(this, text, 0, 0, nullptr));
    const uint16_t *p = text;
    for (uint16_t c = *p++; c; c = *p++) {
        if (!(flags & 0x20) && (c == '\n' || c == '^')) continue;
        const L w = bridge::FEngFont_GetLineWidth(this, p, 0, 0, nullptr);
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
            if (guess < static_cast<int32_t>(d->mu16NumGlyphs) && *reinterpret_cast<const uint16_t *>(table + guess * size) == key)
                g = table + guess * size;
            else
                g = FEngFindGlyph(key, table, d->mu16NumGlyphs, size);
            if (g) pendingLine = true;
        }
    }
    if (pendingLine) h = static_cast<float>(L(h) + mfLineHeight);
    return h;
}
