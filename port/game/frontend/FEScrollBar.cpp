#include "bridge/Bridge.h"
#include <cstring>
#include <cmath>
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
    mbMouseDisabled = arg5;
    mDragStartX = mDragStartY = 0;
    mHandleX = mHandleY = 0;
    mGrabX = mGrabY = 0;
    mBackingX = mBackingY = mBackingW = mBackingH = mHandleW = mHandleH = 0;
    mArrowTime = 0;
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
    mArrowTime = 0;
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

namespace {
using L = long double;
void MousePos(float &x, float &y)
{
    const uint8_t *ms = bridge::MouseGlobals();
    int32_t mxi, myi;
    std::memcpy(&mxi, ms, 4);
    std::memcpy(&myi, ms + 4, 4);
    x = static_cast<float>(L(mxi) - 320.0f);
    y = static_cast<float>(L(myi) - 240.0f);
}
}  // namespace

void FEScrollBar::HandleMouse()
{
    mHit = -9;
    if (!mbMouseDisabled) {
        if (FEngIsClickedOn(mpHandle)) {   // grab the handle
            mbNoMove = 1;
            MousePos(mDragStartX, mDragStartY);
            float x, y;
            FEngGetTopLeft(mpHandle, &x, &y);
            mGrabX = static_cast<float>(std::fabs(L(x) - mDragStartX));
            FEngGetTopLeft(mpHandle, &y, &x);   // PC: outputs swapped into the same two slots
            mGrabY = static_cast<float>(std::fabs(L(x) - mDragStartY));
        } else if (FEngIsClickedOn(mpBacking)) {
            PageClick();
        }
        const uint8_t *ms = bridge::MouseGlobals();
        if (ms[0x1C] ? ms[0x22] : ms[0x1F]) mbNoMove = 0;   // released
    }
    if (mbNoMove) {
        Drag();
        return;
    }
    if (L(bridge::RealTimer() - mArrowTime) * 0.00025f > 0.0625f) ArrowClick();   // arrow auto-repeat
}

void FEScrollBar::ArrowClick()
{
    int32_t event;
    if (FEngIsClickedOn(mpArrow1)) {
        mArrowTime = bridge::RealTimer();
        mHit = -1;
        event = 4;
    } else if (FEngIsClickedOn(mpArrow2)) {
        mArrowTime = bridge::RealTimer();
        mHit = 1;
        event = 5;
    } else {
        return;
    }
    bridge::MakeFrontendSound(static_cast<uint32_t>(event), 0);   // table 0x837FE0[event * 0x1B]
}

void FEScrollBar::Drag()
{
    float mx, my;
    MousePos(mx, my);
    float w, h, x, y;
    L d;
    if (mbVertical) {
        const float want = static_cast<float>(L(my) - mGrabY);
        FEngGetSize(mpHandle, &w, &h);
        const float maxY = static_cast<float>((L(mBackingH) + mBackingY) - h);
        const float cand = want > mBackingY ? want : mBackingY;
        const float ny = maxY < cand ? maxY : cand;
        FEngGetTopLeft(mpHandle, &x, &y);
        FEngSetTopLeft(mpHandle, x, ny);
        d = (L(my) - mHandleY) - mGrabY;
    } else {
        const float want = static_cast<float>(L(mx) - mGrabX);
        FEngGetSize(mpHandle, &w, &h);
        const float maxX = static_cast<float>((L(mBackingW) + mBackingX) - w);
        const float cand = want > mBackingX ? want : mBackingX;
        const float nx = maxX < cand ? maxX : cand;
        FEngGetTopLeft(mpHandle, &x, &y);
        FEngSetTopLeft(mpHandle, nx, y);
        d = (L(mx) - mHandleX) - mGrabX;
    }
    if (d > mStep) mHit = 1;
    else if (d < -L(mStep)) mHit = -1;
}

void FEScrollBar::PageClick()
{
    float mx, my;
    MousePos(mx, my);
    float x, y, w, h;
    if (mbVertical) {
        FEngGetTopLeft(mpHandle, &x, &y);
        if (my < y) { mHit = 10; return; }
        FEngGetTopLeft(mpHandle, &x, &y);
        FEngGetSize(mpHandle, &w, &h);
        if (L(h) + y < my) mHit = -10;
    } else {
        FEngGetTopLeft(mpHandle, &x, &y);
        if (mx < x) { mHit = 10; return; }
        FEngGetTopLeft(mpHandle, &x, &y);
        FEngGetSize(mpHandle, &w, &h);
        if (L(w) + x < mx) mHit = -10;
    }
}

void FEScrollBar::BlinkArrow1()
{
    if (mpArrow1 && !FEngIsScriptRunning(mpArrow1, "Blink")) FEngSetScript(mpArrow1, "Blink", true);
}

void FEScrollBar::BlinkArrow2()
{
    if (mpArrow2 && !FEngIsScriptRunning(mpArrow2, "Blink")) FEngSetScript(mpArrow2, "Blink", true);
}
