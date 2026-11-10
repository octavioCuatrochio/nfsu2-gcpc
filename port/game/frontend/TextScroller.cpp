#include "TextScroller.h"
#include <cstdio>
#include <cstring>
#include "bridge/Bridge.h"
#include "bWare/bString.h"
#include "FEHash.h"
#include "FEngFont.h"
#include "FEngUtil.h"
#include "FEScrollBar.h"

CTextScroller *CTextScroller::Construct()
{
    mpOwner = nullptr;
    mpFont = nullptr;
    mpScrollBar = nullptr;
    mCount = 0;
    mpLines = nullptr;
    mpBuffer = nullptr;
    mPos = 0;
    mNextMsg = 0x911C0A4Bu;
    mPrevMsg = 0x72619778u;
    return this;
}

void CTextScroller::Destruct()
{
    if (mpBuffer) bridge::FreeArray(mpBuffer);
}

bool CTextScroller::PreAllocate(uint32_t bytes)
{
    if (mpBuffer) return false;
    mBufferSize = bytes;
    mpBuffer = static_cast<uint8_t *>(bridge::Alloc(bytes));
    return mpBuffer != nullptr;
}

void CTextScroller::Initialise(void *owner, int32_t width, int32_t visible, const char *name, FEngFont *font)
{
    mpOwner = owner;
    mVisible = visible;
    bStrNCpy(mName, name, 0x1F);
    mWidth = width;
    mpFont = font;
}

const uint16_t *CTextScroller::FindCR(const uint16_t *s)
{
    const uint16_t *r = nullptr;
    for (; *s && !r; ++s)
        if (*s == '\n' || *s == '^') r = s;
    return r;
}

const uint16_t *CTextScroller::FindEND(const uint16_t *s)
{
    while (*s) ++s;
    return s;
}

void CTextScroller::AddLine(const uint16_t *text, int32_t chars)
{
    const uint32_t len = static_cast<uint32_t>(chars);
    if (mUsed > mBufferSize + (0x7FFFFFFFu - len) * 2u) return;   // PC overflow guard (wraps like the original)
    auto *dst = reinterpret_cast<uint16_t *>(mpBuffer + mUsed);
    mpLines[mCount] = dst;
    if (len) std::memcpy(dst, text, len * 2);
    dst[len] = 0;
    ++mCount;
    mUsed += len * 2 + 2;
}

int32_t CTextScroller::WordWrap(const uint16_t *start, const uint16_t *end, bool countOnly, int32_t *chars)
{
    int32_t lines = 0;
    if (chars) *chars = 0;
    if (end <= start) return 0;
    const uint16_t *p = start;
    for (;;) {
        const int32_t n = static_cast<int32_t>(end - p);
        int32_t count = 0;
        float width = 0.0f;
        uint16_t next = 0;
        const uint16_t *s = p;
        if (static_cast<long double>(mWidth - 0x10) > 0.0L) {
            while (s < end) {
                const uint16_t c = *s;
                if (c == '\n' || c == '^') break;
                bool token = false;
                if (c == '$') {
                    int32_t k = 0;
                    for (const uint16_t *q = s + 1; *q && *q != '$'; ++q) ++k;
                    if (k) {   // "$name$" icon: its texture width
                        token = true;
                        const uint8_t *tex = static_cast<const uint8_t *>(mpFont->GetJoyEventTextureInfo(s));
                        int16_t w = 0;
                        if (tex) std::memcpy(&w, tex + 0x44, 2);
                        width = static_cast<float>(static_cast<long double>(tex ? w : 0) + width);
                        const uint16_t *e = s + 1;
                        if (*e != '$') {
                            while (*e && *e != '$') ++e;
                            ++e;
                        }
                        count += static_cast<int32_t>(e - s);
                        s = e;
                        next = 0;
                    }
                }
                if (!token) {
                    width = static_cast<float>(mpFont->GetCharacterWidth(c, next, 0) + width);
                    ++count;
                    ++s;
                    next = *s;
                }
                if (!(static_cast<long double>(width) < static_cast<long double>(mWidth - 0x10))) break;
            }
        }
        if (count >= n) {
            if (!countOnly) AddLine(p, count);
            if (chars) *chars += count + 1;
            return lines + 1;
        }
        for (int32_t i = count - 1; i > 0; --i)   // break after the last space
            if (p[i] == ' ') {
                count = i + 1;
                break;
            }
        if (!countOnly) AddLine(p, count);
        if (chars) *chars += count + 1;
        ++lines;
        p += count;
        if (end <= p) return lines;
    }
}

void CTextScroller::SetText(const uint16_t *text)
{
    if (mpFont && text) {
        int32_t lines = 0, chars = 0;
        mCount = 0;
        mUsed = 0;
        const uint16_t *p = text;
        const uint16_t *e = FindCR(p);
        if (!e) e = FindEND(p);
        while (*p) {
            if (e == p) {
                ++lines;
                ++chars;
            } else {
                int32_t c = 0;
                lines += WordWrap(p, e, true, &c);
                chars += c;
            }
            if (!*e) break;
            p = e + 1;
            e = FindCR(p);
            if (!e) e = FindEND(p);
        }
        if (lines > 0) {
            if (!mpBuffer) {   // PC: (chars + 2 * lines) * 2 (4-byte pointers)
                mBufferSize = static_cast<uint32_t>(chars * 2 + lines * static_cast<int32_t>(sizeof(uint16_t *)));
                mpBuffer = static_cast<uint8_t *>(bridge::Alloc(mBufferSize));
            }
            mpLines = reinterpret_cast<uint16_t **>(mpBuffer);
            mUsed = static_cast<uint32_t>(lines * static_cast<int32_t>(sizeof(uint16_t *)));
            p = text;
            e = FindCR(p);
            if (!e) e = FindEND(p);
            while (e && *p) {
                if (e == p) AddLine(nullptr, 0);
                else WordWrap(p, e, false, nullptr);
                if (!*e) break;
                p = e + 1;
                e = FindCR(p);
                if (!e) e = FindEND(p);
            }
        }
    } else {
        mCount = 0;
    }
    mPos = 0;
    if (mpScrollBar) mpScrollBar->Update(mVisible, mCount, 1);
    Display(mPos);
}

void CTextScroller::Scroll(int32_t delta)
{
    if (mCount <= mVisible) return;
    int32_t pos = mPos + delta;
    if (pos < 0) pos = 0;
    else if (mVisible + pos >= mCount) pos = mCount - mVisible;
    if (mpScrollBar) {
        if (pos < mPos) mpScrollBar->BlinkArrow1();
        else if (pos > mPos) mpScrollBar->BlinkArrow2();
    }
    mPos = pos;
    Display(pos);
}

void CTextScroller::UpdateScrollBar()
{
    if (mpScrollBar) mpScrollBar->Update(mVisible, mCount, mPos + 1);
}

bool CTextScroller::HandleNotificationMessage(uint32_t msg)
{
    if (msg == mPrevMsg) {
        Scroll(-1);
        UpdateScrollBar();
        return true;
    }
    if (msg == mNextMsg) {
        Scroll(1);
        UpdateScrollBar();
        return true;
    }
    if (msg == 0x9803F6E2u && mpScrollBar) {   // PC mouse: wheel (0x8763EC) or the scroll bar
        mpScrollBar->HandleMouse();
        const int32_t wheel = *bridge::GameInt(0x8763EC);
        int32_t d;
        if (wheel < 0) d = 1;
        else if (wheel > 0) d = -1;
        else {
            d = mpScrollBar->mHit;
            if (d == -9) return false;
        }
        Scroll(d);
        UpdateScrollBar();
        return true;
    }
    return false;
}

void CTextScroller::Display(int32_t pos)
{
    if (!mpOwner || mVisible <= 0) return;
    const char *pkg;
    std::memcpy(&pkg, static_cast<uint8_t *>(mpOwner) + sizeof(void *), sizeof pkg);   // owner +4
    static const uint16_t kEmpty[2] = {0, 0};
    for (int32_t i = 0; i < mVisible; ++i) {
        char name[0x28];
        std::snprintf(name, sizeof name, mName, i + 1);   // PC CRT sprintf 75C784
        FESetString(pkg, FEHashUpper(name), pos + i < mCount ? mpLines[pos + i] : kEmpty);
    }
}
