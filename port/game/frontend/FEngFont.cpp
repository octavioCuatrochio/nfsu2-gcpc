#include "FEngFont.h"
#include <cstddef>
#include "bridge/Bridge.h"
#include "bWare/bString.h"
#include "FEHash.h"
#include "FEMath.h"
#include "FEObject.h"
#include "FEString.h"
#include "cFEngRender.h"
#include <cstring>

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

namespace {
uint16_t RemapCharacter(uint16_t c)
{
    if (c >= 0xFF80) c &= 0xFF;
    if (c == 0x99) return 0x2122;   // (TM)
    if (c == 0x9C) return 0x153;    // oe
    return c;
}
}  // namespace

long double FEngFont::GetCharacterWidth(uint16_t c, uint16_t prev, uint32_t flags)
{
    if ((!(flags & 0x20) && (c == '\n' || c == '^')) || c == '\r') return 0.0L;
    c = RemapCharacter(c);
    if ((flags & 0x80) && c == 0xA0) {
        prev = 0;
        c = 0x20;
    }
    const FEngGlyph *g = mpData->FindCharacter(c);
    if (!g) return 0.0L;
    if (prev) return L(mpData->GetKerning(g, prev)) + g->mi16Advance;
    return L(0.0f) + g->mi16Advance;
}

uint32_t FEngFont::GetNumWordWrappedLines(const uint16_t *text, uint32_t flags, uint32_t maxWidth)
{
    if (!text) return 0;
    if (flags & 0x20) return 1;
    const uint16_t *p = text, *lastSpace = nullptr;
    bool skipped = false;
    float w = 0.0f;
    uint32_t lines = 1, index = 0;
    for (uint16_t c = *p; c; c = *p, ++index) {
        if ((c == '\n' || c == '^') && lastSpace) {
            ++lines;
            p = lastSpace + 1;
            lastSpace = nullptr;
            w = 0.0f;
            skipped = true;
            continue;
        }
        if (c == '\r') {
            ++p;
            continue;
        }
        if (skipped) { index = 0; skipped = false; }
        if (c == ' ') lastSpace = p;
        if (FEngIsJoyEventTexture(p, flags)) {
            p = FEngSkipJoyEventTexture(p, flags);
            skipped = true;
        } else {
            if (const FEngGlyph *g = mpData->FindCharacter(RemapCharacter(c))) {
                const L base = index ? L(mpData->GetKerning(g, p[-1])) + w : L(w);
                w = static_cast<float>(L(g->mi16Advance) + base);
            }
            ++p;
        }
        if (maxWidth && *p && !(L(maxWidth) > w) && !(w != w) && lastSpace) {   // width reached: wrap at the last space
            ++lines;
            w = 0.0f;
            p = lastSpace;
            if (*p == ' ')
                while (*p == ' ') ++p;
            lastSpace = nullptr;
            skipped = true;
        }
    }
    return lines;
}

void FEngWideToCharString(char *out, uint32_t size, const uint16_t *wide)
{
    if (!out || !wide) return;
    uint32_t n = 0;
    for (const uint16_t *p = wide; *p && n < size - 1; ++p) {
        const int16_t c = static_cast<int16_t>(*p);
        if (c >= -128 && c <= 127) {
            *out++ = static_cast<char>(c);
            ++n;
        }
    }
    *out = 0;
}

void *FEngFont::GetJoyEventTextureInfo(const uint16_t *text)
{
    uint32_t hash = 0;
    if (text[0] == '$') {
        uint16_t name[0x41] = {};   // PC: 0x40; a 64-char name runs on into the next stack buffer (+1 terminator here)
        uint32_t bytes = 0;
        for (const uint16_t *p = text + 1; *p != '$' && *p && bytes < 0x80; ++p, bytes += 2) name[bytes / 2] = *p;
        char ascii[0x80];
        FEngWideToCharString(ascii, 0x80, name);
        if (bStrCmp(ascii, "ICON_SPACER") == 0) {
            hash = 0xBCA80379u;
        } else {
            int32_t port = 0;
            bool found = false;
            for (int32_t i = 0; i < 2 && !found; ++i)
                if (bridge::IsJoystickPluggedIn(i)) { port = i; found = true; }
            if (!found)
                for (int32_t i = 0; i < 2; ++i)
                    if (bridge::JoyPortConfig()[i * 4] != 0xFF) { port = i; break; }
            hash = bridge::GetJoyEventTexture(ascii, port);
        }
    }
    return bridge::GetTextureInfo(hash, 1, 0);
}

// ---- font objects ----

FEngFont *FEngFont::Construct(const uint8_t *chunk)
{
    char *name = reinterpret_cast<char *>(const_cast<uint8_t *>(chunk + 8));
    mpTexture = nullptr;
    mpData = nullptr;
    _unknown10 = 0;
    mu32NameHash = 0;
    mu32TextureHash = 0;
    mfLineHeight = 0;
    mpcName = name;
    mpcTextureName = name + 0x100;
    const uint32_t fullHash = FEHashUpper(name);
    // "_<LANGUAGE>" suffix: for every position, every language id 0..13 (table 0x7F7020 has 11; others "UNKNOWN")
    for (int32_t pos = 0; pos < bStrLen(mpcName); ++pos) {
        for (int32_t lang = 0; lang < 14; ++lang) {
            const char *suffix = bridge::LanguageSuffix(lang);
            if (bStrICmp(mpcName + pos, suffix) == 0 && pos > 0 && mpcName[pos - 1] == '_') {
                mpcName[pos - 1] = 0;
                break;
            }
        }
    }
    mpData = static_cast<FEngFontData *>(bridge::RealFont_Create(mpcTextureName + 0x100));
    mu32NameHash = FEHashUpper(mpcName);
    mu32TextureHash = FEHashUpper(mpcTextureName);
    int32_t lineHeight;
    std::memcpy(&lineHeight, reinterpret_cast<const uint8_t *>(mpData) + 0x24, 4);
    mfLineHeight = static_cast<float>(lineHeight);
    mpTexture = bridge::GetTextureInfo(mu32TextureHash, 0, 0);
    mfBaseline = 0;
    mfLeadingScale = 1.0f;
    const uint8_t *m = bridge::FontMetricsTable();
    for (int i = 0; i < 4; ++i, m += 12) {
        uint32_t h;
        std::memcpy(&h, m, 4);
        if (h == fullHash) {   // PC: the hash before the suffix was cut
            std::memcpy(&mfBaseline, m + 4, 4);
            std::memcpy(&mfLeadingScale, m + 8, 4);
            break;
        }
    }
    return this;
}

void FEngFont::Destroy() { bridge::RealFont_Destroy(mpData); }

void FEngFont::NotifyTextureLoading(uint32_t a, uint32_t b)
{
    bridge::FEngFontList *l = bridge::FontList();
    auto *sentinel = reinterpret_cast<FEngFont *>(l);
    for (FEngFont *f = l->mpHead; f != sentinel; f = f->mpNext) {
        void *t = bridge::FixupTextureInfoNull(f->mpTexture, f->mu32TextureHash, a, b);
        if (t != f->mpTexture) f->mpTexture = t;
    }
}

FEngFont *FEngFindFontNative(uint32_t hash)
{
    bridge::FEngFontList *l = bridge::FontList();
    auto *sentinel = reinterpret_cast<FEngFont *>(l);
    for (;;) {
        for (FEngFont *f = l->mpHead; f != sentinel; f = f->mpNext)
            if (f->mu32NameHash == hash) return f;
        const uint32_t *alias = bridge::FontAlias();
        if (hash != alias[0]) return nullptr;
        hash = alias[1];   // PC: the alias is looked up again (and again, if it aliases itself)
    }
}

int32_t LoaderFEngFont(const uint8_t *chunk)
{
    uint32_t id;
    std::memcpy(&id, chunk, 4);
    if (id != 0x30201u) return 0;
    void *mem = bridge::Alloc(static_cast<uint32_t>(sizeof(FEngFont)));
    FEngFont *f = mem ? static_cast<FEngFont *>(mem)->Construct(chunk) : nullptr;
    bridge::FEngFontList *l = bridge::FontList();
    FEngFont *head = l->mpHead;   // PC: no null check
    l->mpHead = f;
    head->mpPrev = f;
    f->mpPrev = reinterpret_cast<FEngFont *>(l);
    f->mpNext = head;
    return 1;
}

// ---- rendering ----

void FEngFont::RenderCharacter(const FEngGlyph *g, float x, float y, const uint32_t *colours, FERenderObject *cached,
                               void *renderInfo, bool useZ)
{
    const uint8_t *tex = static_cast<const uint8_t *>(mpTexture);
    int16_t tw16, th16;
    std::memcpy(&tw16, tex + 0x44, 2);
    std::memcpy(&th16, tex + 0x46, 2);
    const L tw = tw16, th = th16;
    const float u0 = static_cast<float>(L(int32_t(g->mu16X) - 1) / tw);
    const float u1 = static_cast<float>(L(int32_t(g->mu8Width) + g->mu16X + 1) / tw);
    const float v0 = static_cast<float>(L(int32_t(g->mu16Y) - 1) / th);
    const float v1 = static_cast<float>(L(int32_t(g->mu8Height) + g->mu16Y + 1) / th);
    L w = g->mu8Width;
    if (w < 4.0L) w = 4.0L;   // PC: test 0x05 (narrow glyphs drawn 4 wide)
    const L x0e = L(g->mi8XOffset) + x;
    const float x0 = static_cast<float>(x0e);
    const float x1 = static_cast<float>(x0e + w);
    const float y0 = static_cast<float>((L(g->mi8YOffset) + mfBaseline) + y);
    const float y1 = static_cast<float>(L(g->mu8Height) + y0);
    float z = 0;
    if (useZ) z = static_cast<const float *>(cached->mpObject->mpData)[9];
    cached->AddPoly(x0, y0, x1, y1, z, u0, v0, u1, v1, colours, renderInfo, useZ);
}

void FEngFont::RenderString(const int32_t *colour, const uint16_t *text, FEObject *obj, const bMatrix4 *matrix,
                            FERenderObject *cached, void *renderInfo)
{
    auto *str = reinterpret_cast<FEString *>(obj);
    const uint32_t justify = str->_unknown6C;
    int32_t leading;
    std::memcpy(&leading, reinterpret_cast<const uint8_t *>(obj) + 0x70, 4);
    const int32_t spacing = FEFtol(L(leading) * mfLeadingScale);
    const uint32_t flags = obj->mu32Flags;
    uint32_t maxWidth;
    std::memcpy(&maxWidth, reinterpret_cast<const uint8_t *>(obj) + 0x74, 4);
    L height;
    if (justify & 5) height = L(static_cast<int32_t>(GetNumWordWrappedLines(text, flags, maxWidth))) * (L(leading) * mfLeadingScale + mfLineHeight);
    else height = GetTextHeight(text, spacing);
    float y = justify & 4 ? static_cast<float>(height * -0.5L) : justify & 8 ? static_cast<float>(-height) : 0.0f;
    auto lineX = [&](L w) -> float { return justify & 1 ? static_cast<float>(w * -0.5L) : justify & 2 ? static_cast<float>(-w) : 0.0f; };
    float x = lineX(GetLineWidth(text, flags, maxWidth, nullptr));
    float lineStart = x;
    const uint32_t p = static_cast<uint32_t>(((((colour[0] / 2) | (static_cast<uint32_t>(colour[3]) << 8)) << 8) | static_cast<uint32_t>(colour[1] / 2)) << 8) | static_cast<uint32_t>(colour[2] / 2);
    const uint32_t colours[4] = {p, p, p, p};
    if (!mpTexture) return;
    cached->mTransform = *matrix;
    bool wrapped = false;
    float lineWidth = static_cast<float>(GetLineWidth(text, flags, maxWidth, &wrapped));
    uint16_t c = *text;
    bool resetKern = false;
    uint32_t count = 0;
    while (c) {
        if (resetKern) { count = 0; resetKern = false; }
        bool newLine = false;
        if (!(flags & 0x20) && (c == '\n' || c == '^')) {
            ++text;
            if (!*text) break;
            lineWidth = static_cast<float>(GetLineWidth(text, flags, maxWidth, &wrapped));
            x = lineStart = lineX(lineWidth);
            newLine = true;
        } else {
            bool token = false;
            if (!(flags & 0x820) && c == '$') {
                const uint16_t *q = text + 1;
                uint32_t len = 0;
                while (*q && *q != '$') { ++q; ++len; }
                if (len) {
                    text = FEngSkipJoyEventTexture(text, flags);
                    resetKern = true;
                    token = true;
                }
            }
            if (!token) {
                bool kern = true;
                if ((flags & 0x80) && c == 0xA0) {
                    kern = false;
                    c = ' ';
                } else {
                    if (c >= 0xFF80) c &= 0xFF;
                    if (c == 0x99) c = 0x2122;
                    else if (c == 0x9C) c = 0x153;
                }
                if (const FEngGlyph *g = mpData->FindCharacter(c)) {
                    if (count && kern) x = static_cast<float>(L(mpData->GetKerning(g, text[-1])) + x);
                    RenderCharacter(g, x, y, colours, cached, renderInfo, (flags >> 8) & 1);
                    x = static_cast<float>(L(g->mi16Advance) + x);
                }
                ++text;
            }
            // word wrap: past the measured line width -> skip spaces, next line
            if (maxWidth && wrapped && L(x) - lineStart >= lineWidth) {   // PC: NaN does not wrap
                while (*text == ' ') ++text;
                lineWidth = static_cast<float>(GetLineWidth(text, flags, maxWidth, &wrapped));
                x = lineStart = lineX(lineWidth);
                newLine = true;
            }
        }
        if (newLine) y = static_cast<float>((L(spacing) + mfLineHeight) + y);
        ++count;
        c = *text;
    }
    cached->Render();
}
