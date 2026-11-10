#include "FEScrollBar.h"
#include "FEHash.h"
#include "FEngUtil.h"
#include "bWare/bString.h"

using L = long double;

FEObject *FEngFindImage(const char *pkg, const char *name) { return FEngFindImage(pkg, FEHashUpper(name)); }

void FEngSetWidth(FEObject *obj, float w)
{
    float cw = 0, ch = 0;
    FEngGetSize(obj, &cw, &ch);
    FEngSetSize(obj, w, ch);
}

void FEngSetHeight(FEObject *obj, float h)
{
    float cw = 0, ch = 0;
    FEngGetSize(obj, &cw, &ch);
    FEngSetSize(obj, cw, h);
}

FEScrollBar *FEScrollBar::Construct(const char *pkg, const char *base, bool vertical, bool resized, bool arg5)
{
    mbVertical = vertical;
    mbResized = resized;
    mbNoMove = 0;
    _unknown03 = arg5;
    _unknown04 = _unknown08 = 0;
    mHandleX = mHandleY = 0;
    _unknown14 = _unknown18 = 0;
    mBackingX = mBackingY = mBackingW = mBackingH = mHandleW = mHandleH = 0;
    _unknown38 = 0;
    mpBacking = mpHandle = mpArrow1 = mpArrow2 = mpBackingEnd1 = mpBackingEnd2 = nullptr;
    if (!base) return this;
    // PC: FEngSNPrintf(buf, 0x40, "%s%s", ...) ignores the size (bVSPrintf); a larger buffer here.
    char name[0x100];
    auto find = [&](const char *suffix) { return FEngFindImage(pkg, bStrCat(name, base, suffix)); };
    mpBacking = find("_Backing");
    mpHandle = find("_Handle");
    mpArrow1 = find("_Arrow_1");
    mpArrow2 = find("_Arrow_2");
    mpBackingEnd1 = find("_Backing_End_1");
    mpBackingEnd2 = find("_Backing_End_2");
    FEngGetTopLeft(mpBacking, &mBackingX, &mBackingY);
    FEngGetSize(mpBacking, &mBackingW, &mBackingH);
    if (mbResized) {
        const float s = mbVertical ? mBackingW : mBackingH;
        mHandleH = s;
        mHandleW = s;
    } else {
        FEngGetSize(mpHandle, &mHandleW, &mHandleH);
    }
    _unknown38 = 0;
    return this;
}

void FEScrollBar::Update(int32_t visible, int32_t total, int32_t index)
{
    if (total <= visible || total == 0) {
        FEngSetInvisible(mpHandle);
        FEngSetInvisible(mpArrow1);
        FEngSetInvisible(mpArrow2);
        return;
    }
    FEngSetVisible(mpBacking);
    if (index) FEngSetVisible(mpHandle);
    else FEngSetInvisible(mpHandle);
    FEngSetVisible(mpArrow1);
    FEngSetVisible(mpArrow2);
    FEngSetVisible(mpBackingEnd2);
    FEngSetVisible(mpBackingEnd1);
    if (mbResized) SetPosResized(visible, total, index);
    else SetPosNonResized(visible, total, index);
}

void FEScrollBar::SetPosResized(int32_t visible, int32_t total, int32_t index)
{
    // x87: a, b rounded to float; everything else extended until stored
    const float a = static_cast<float>(visible), b = static_cast<float>(total);
    const L q = static_cast<L>(a) / static_cast<L>(b);
    const float len = mbVertical ? mBackingH : mBackingW;
    const float size = static_cast<float>(q * len);
    if (mbVertical) FEngSetHeight(mpHandle, size);
    else FEngSetWidth(mpHandle, size);
    const L t = static_cast<L>(index) - 1.0L;
    L d = static_cast<L>(b) - static_cast<L>(a);
    if (1.0L > d) d = 1.0L;   // PC: fcomp + test 0x41 (NaN keeps d)
    const L step = (static_cast<L>(len) - static_cast<L>(size)) / d;
    mStep = static_cast<float>(step);
    const float pos = static_cast<float>(t * step + (mbVertical ? mBackingY : mBackingX));
    if (mbVertical) mHandleY = pos;
    else mHandleX = pos;
    if (mbNoMove) return;
    float x, y;
    FEngGetTopLeft(mpHandle, &x, &y);
    if (mbVertical) FEngSetTopLeft(mpHandle, x, pos);
    else FEngSetTopLeft(mpHandle, pos, y);
}

void FEScrollBar::SetPosNonResized(int32_t visible, int32_t total, int32_t index)
{
    const float start = mbVertical ? mBackingY : mBackingX;
    const float hs = mbVertical ? mHandleH : mHandleW;
    const float len = mbVertical ? mBackingH : mBackingW;
    const float t = static_cast<float>(static_cast<L>(index) - 1.0L);
    const float b = static_cast<float>(total);
    L d = static_cast<L>(b) - static_cast<L>(visible);
    if (1.0L > d) d = 1.0L;
    const float step = static_cast<float>((static_cast<L>(len) - static_cast<L>(hs)) / d);
    mStep = step;
    L pos;
    if (t == 0.0f) pos = static_cast<L>(hs) * 0.5L + start;
    else if (static_cast<L>(b) - 1.0L == static_cast<L>(t)) pos = (static_cast<L>(len) + start) - static_cast<L>(hs) * 0.5L;
    else pos = static_cast<L>(hs) * 0.5L + static_cast<L>(step) * t + start;
    const float p = static_cast<float>(pos);
    float x, y;
    if (mbVertical) {
        if (!mbNoMove) {
            FEngGetCenter(mpHandle, &x, &y);
            FEngSetCenter(mpHandle, x, p);
        }
        mHandleY = p;
    } else {
        if (!mbNoMove) {
            FEngGetCenter(mpHandle, &x, &y);
            FEngSetCenter(mpHandle, p, y);
        }
        mHandleX = p;
    }
}

void FEScrollBar::SetArrowVisibility(int32_t arrow, bool visible)
{
    FEObject *o = arrow == 1 ? mpArrow1 : arrow == 2 ? mpArrow2 : nullptr;
    if (arrow != 1 && arrow != 2) return;
    if (visible) FEngSetVisible(o);
    else FEngSetInvisible(o);
}

void FEScrollBar::SetBackingVisibility(bool visible)
{
    if (visible) {
        FEngSetVisible(mpBacking);
        FEngSetVisible(mpBackingEnd1);
        FEngSetVisible(mpBackingEnd2);
    } else {
        FEngSetInvisible(mpBacking);
        FEngSetInvisible(mpBackingEnd1);
        FEngSetInvisible(mpBackingEnd2);
    }
}
