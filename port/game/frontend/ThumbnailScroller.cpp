#include "ThumbnailScroller.h"
#include "bridge/Bridge.h"
#include "FEHash.h"
#include "FEMath.h"
#include "FEObject.h"
#include "FEPackage.h"
#include "FEPackageManager.h"
#include "FEngUtil.h"
#include <cmath>
#include <cstring>

namespace {
using L = long double;

constexpr float kUnselected = 0.8f;        // PC 0x7850EC
constexpr float kHalf = 0.5f;              // PC 0x784260
constexpr float kPulseLimit = 2.0f;        // PC 0x7844C0
constexpr float kPulseCenter = 1.0f;       // PC 0x784250
constexpr float kPulseScale = 1.1f;        // PC 0x787598
constexpr float kPulseBias = 127.5f;       // PC 0x79B954
constexpr float kPulseMax = 255.0f;        // PC 0x784544
constexpr float kPulseMul = 0.0001176470541395247f;   // PC 0x79B950
constexpr float kPulseAdd = 0.9700000286102295f;      // PC 0x79B94C
constexpr float kPulseStep = 0.03f;        // PC 0x788150
constexpr float kDimAlpha = 70.0f;         // PC 0x78A414 (node.mbDimmed)
constexpr float kOffscreen = 6969.0f;      // PC 0x45D9C800

ThumbnailNode *FromNode(WidgetListNode *n)
{
    return n ? reinterpret_cast<ThumbnailNode *>(reinterpret_cast<uint8_t *>(n) - offsetof(ThumbnailNode, node)) : nullptr;
}
ThumbnailNode *End(ThumbnailScroller &s) { return FromNode(&s.mNodes); }
ThumbnailNode *First(ThumbnailScroller &s) { return FromNode(s.mNodes.next); }
ThumbnailNode *Next(ThumbnailNode *n) { return FromNode(n->node.next); }

void Append(ThumbnailScroller &s, ThumbnailNode *n)
{
    WidgetListNode *tail = s.mNodes.prev, *node = &n->node;
    tail->next = node;
    s.mNodes.prev = node;
    node->prev = tail;
    node->next = &s.mNodes;
}

void Unlink(ThumbnailNode *n)
{
    n->node.prev->next = n->node.next;
    n->node.next->prev = n->node.prev;
}

// FEngSetInvisible (PC 50CA00) marks the scalar itself and recurses into groups; the original strips that write
// the flag by hand and then hide the group's children -- same result.
void MarkHidden(FEObject *obj)
{
    FEngSetInvisible(obj);
}
}  // namespace

// ---- lifecycle ----

ThumbnailScroller *ThumbnailScroller::Construct(const char *pkg, const char *master, const char *slot, int32_t alignment,
                                                const char *lockImage, const char *button)
{
    vtable = bridge::ThumbnailScrollerVTable();
    mNodes.next = mNodes.prev = &mNodes;
    mpCurrent = nullptr;
    mpCubic = nullptr;
    mpPackage = pkg;
    mpMasterName = slot;
    mpMaster = nullptr;
    mpLock = mpButton = nullptr;
    mpNext = nullptr;
    mbSmooth = 0;
    mbVertical = 0;
    mbScale = 0;
    mbFadeAlpha = 0;
    mbHorizontal = 1;
    mbVisible = 1;
    mAlignment = alignment;
    mScroll = 0.0f;
    mPulse = 0.0f;
    mOriginX = mOriginY = 0.0f;
    mMasterW = mMasterH = 0.0f;
    mStripW = 480.0f;
    mStripH = 100.0f;
    mCenterX = mCenterY = 0.0f;
    mSpacing = 0.0f;

    auto *cubic = static_cast<IconCubic *>(bridge::Alloc(sizeof(IconCubic)));
    if (cubic) {
        std::memset(cubic, 0, sizeof *cubic);   // PC only initialises the fields below (operator new is not zeroing)
        cubic->mVal = cubic->mdVal = cubic->mValDesired = cubic->mdValDesired = 0.0f;
        cubic->mDuration = 1.0f;
        cubic->_unknown28 = 1.0f;
        cubic->mFlags = 0;
        cubic->mState = 1;
    }
    mpCubic = cubic;
    if (cubic) {
        cubic->mDuration = 0.3f;
        cubic->mState = 0;
    }

    mpMaster = FEngFindObject(mpPackage, FEHashUpper(master));
    FEngGetSize(mpMaster, &mMasterW, &mMasterH);
    SetMasterObject(mpMaster);
    mpNext = GetNextNode(nullptr);
    FEngSetInvisible(mpMaster);
    FEngGetCenter(mpMaster, &mOriginX, &mOriginY);
    FEngSetCurrentButton(mpPackage, mpMaster);

    mpLock = FEngFindImage(pkg, lockImage);
    mpButton = FEngFindObject(pkg, FEHashUpper(button));
    return this;
}

void ThumbnailScroller::Destruct()
{
    vtable = bridge::ThumbnailScrollerVTable();
    FEngSetSize(mpMaster, mMasterW, mMasterH);
    while (mNodes.next != &mNodes) {
        ThumbnailNode *n = FromNode(mNodes.next);
        Unlink(n);
        FEngSetSize(n->mpObject, n->mWidth, n->mHeight);
        bridge::VirtualScalarDelete(n);
    }
    bridge::FreeArray(mpCubic);
    mpCubic = nullptr;
}

ThumbnailScroller *ThumbnailScroller::ScalarDeletingDestructor(uint32_t flags)
{
    Destruct();
    if (flags & 1) bridge::FreeArray(this);
    return this;
}

// ---- list helpers ----

ThumbnailNode *ThumbnailScroller::FindNode(uint32_t id)
{
    for (ThumbnailNode *n = First(*this); n != End(*this); n = Next(n))
        if (n->mu32Id == id) return n;
    return nullptr;
}

int32_t ThumbnailScroller::GetCurrenNodeIndex() const
{
    int32_t i = 1;
    for (ThumbnailNode *n = FromNode(const_cast<WidgetListNode *>(mNodes.next)); n != FromNode(const_cast<WidgetListNode *>(&mNodes));
         n = FromNode(n->node.next)) {
        if (n == mpCurrent) return i;
        ++i;
    }
    return 0;
}

void ThumbnailScroller::MoveMeOffscreen()
{
    for (ThumbnailNode *n = First(*this); n != End(*this); n = Next(n)) FEngSetCenter(n->mpObject, kOffscreen, 0.0f);
}

void ThumbnailScroller::ResetNodes()
{
    for (ThumbnailNode *n = First(*this); n != End(*this); n = Next(n)) {
        FEngSetSize(n->mpObject, n->mWidth, n->mHeight);
        FEngSetColor(n->mpObject, n->mu32Color);
    }
}

void ThumbnailScroller::SetVisibility(bool visible)
{
    mbVisible = visible ? 1 : 0;
    if (!mpPackage) return;
    FEPackage *pkg = FEngFindPackage(mpPackage);
    if (!pkg) return;   // PC dereferences without checking; a package is always loaded when screens use this
    FEObject *obj = reinterpret_cast<FEObject *>(pkg->mObjects.head);
    FEObject *tail = reinterpret_cast<FEObject *>(pkg->mObjects.tail);
    const uint32_t hash = FEHashUpper(mpMasterName);
    while (obj) {
        if (obj->mu32NameHash == hash) {
            if (visible) FEngSetVisible(obj);
            else MarkHidden(obj);
        }
        if (obj == tail) break;
        obj = reinterpret_cast<FEObject *>(obj->node.next);
    }
}

FEObject *ThumbnailScroller::GetNextNode(FEObject *from)
{
    if (!mpPackage) return nullptr;
    FEPackage *pkg = FEngFindPackage(mpPackage);
    if (!pkg) return nullptr;
    FEObject *obj = from ? reinterpret_cast<FEObject *>(from->node.next)
                         : reinterpret_cast<FEObject *>(pkg->mObjects.head);
    FEObject *tail = reinterpret_cast<FEObject *>(pkg->mObjects.tail);
    const uint32_t hash = FEHashUpper(mpMasterName);
    while (obj) {
        if (obj->mu32NameHash == hash) return obj;
        if (obj == tail) return nullptr;
        obj = reinterpret_cast<FEObject *>(obj->node.next);
    }
    return nullptr;
}

FEObject *ThumbnailScroller::AddNode(uint32_t id, bool dimmed, bool animate, bool useColor, uint32_t color)
{
    if (!mpNext) return nullptr;
    auto *n = static_cast<ThumbnailNode *>(bridge::Alloc(sizeof(ThumbnailNode)));
    if (!n) return nullptr;
    n->vtable = bridge::ThumbnailNodeVTable();
    n->mu32Id = id;
    n->mpObject = mpNext;
    n->mOffsetX = n->mOffsetY = 0.0f;
    n->mbDimmed = dimmed ? 1 : 0;
    n->mbAnimate = animate ? 1 : 0;
    n->mbShowLock = 0;
    n->mbFadeInButton = 0;
    if (mbHorizontal) n->mOffsetX = mScroll;
    else n->mOffsetY = mScroll;
    n->mu32Color = useColor ? color : FEngGetObjectColorPacked(n->mpObject);
    FEngGetSize(n->mpObject, &n->mWidth, &n->mHeight);
    const float step = mbHorizontal ? n->mWidth : n->mHeight;
    mScroll = static_cast<float>(static_cast<L>(step) + static_cast<L>(mScroll));
    mScroll = static_cast<float>(static_cast<L>(mSpacing) + static_cast<L>(mScroll));
    Append(*this, n);
    mpNext = GetNextNode(mpNext);
    return n->mpObject;
}

FEObject *ThumbnailScroller::AddNode(ThumbnailNode *n, bool useColor, uint32_t color)
{
    if (!mpNext) {
        if (n) bridge::VirtualScalarDelete(n);
        return nullptr;
    }
    n->mpObject = mpNext;
    if (mbHorizontal) n->mOffsetX = mScroll;
    else n->mOffsetY = mScroll;
    n->mu32Color = useColor ? color : FEngGetObjectColorPacked(n->mpObject);
    FEngGetSize(n->mpObject, &n->mWidth, &n->mHeight);
    const float step = mbHorizontal ? n->mWidth : n->mHeight;
    mScroll = static_cast<float>(static_cast<L>(step) + static_cast<L>(mScroll));
    mScroll = static_cast<float>(static_cast<L>(mSpacing) + static_cast<L>(mScroll));
    Append(*this, n);
    mpNext = GetNextNode(mpNext);
    return n->mpObject;
}

// ---- scrolling ----

void ThumbnailScroller::SetDesiredItem(uint32_t id)
{
    ThumbnailNode *n = FindNode(id);
    if (!n) return;
    if (n != mpCurrent) {
        mpCurrent = n;
        const float v = -(mbHorizontal ? n->mOffsetX : n->mOffsetY);
        mpCubic->mValDesired = v;
        if (!(v == mpCubic->mVal)) mpCubic->mFlags = 2;
    }
    bridge::SetIconScrollTime(bridge::RealTimer());   // 0x8641A0 = ThumbnailFadeTimer
}

void ThumbnailScroller::SnapToNode(ThumbnailNode *n)
{
    if (n == mpCurrent) return;
    mpCurrent = n;
    const float v = -(mbHorizontal ? n->mOffsetX : n->mOffsetY);
    mpCubic->mValDesired = v;
    if (!(v == mpCubic->mVal)) mpCubic->mFlags = 2;
    mpCubic->mVal = mpCubic->mValDesired;
    mpCubic->mdVal = mpCubic->mdValDesired;
    mpCubic->mFlags = 0;
    MoveObjectsTo(mpCubic->mVal);
}

void ThumbnailScroller::SnapToItem(uint32_t id)
{
    ThumbnailNode *n = FindNode(id);
    if (!n) n = First(*this);
    SnapToNode(n);
}

void ThumbnailScroller::SnapToHead() { SnapToNode(First(*this)); }

void ThumbnailScroller::Scroll(int32_t dir)
{
    if (mNodes.next == &mNodes) return;
    ThumbnailNode *cur = mpCurrent;
    if (!cur) return;
    ThumbnailNode *target = cur;
    if (dir == -1) {
        if (cur == First(*this)) return;
        target = FromNode(cur->node.prev);
    } else if (dir == 1) {
        if (cur == FromNode(mNodes.prev)) return;
        target = FromNode(cur->node.next);
    }
    if (target == cur) return;
    if (mbSmooth) SetDesiredItem(target->mu32Id);
    else SnapToItem(target->mu32Id);
}

void ThumbnailScroller::ClearAll()
{
    FEngSetSize(mpMaster, mMasterW, mMasterH);
    ResetNodes();
    SetVisibility(false);
    MoveMeOffscreen();
    mScroll = 0.0f;
    mpCurrent = nullptr;
    while (mNodes.next != &mNodes) {
        ThumbnailNode *n = FromNode(mNodes.next);
        Unlink(n);
        FEngSetSize(n->mpObject, n->mWidth, n->mHeight);
        bridge::VirtualScalarDelete(n);
    }
    mpNext = GetNextNode(nullptr);
}

// PC 52EEA0. Every arithmetic step follows the original x87 order; long double keeps the extended intermediates
// and the explicit float casts are the original's `fstp DWORD`.
void ThumbnailScroller::MoveObjectsTo(float scroll)
{
    for (ThumbnailNode *n = First(*this); n != End(*this); n = Next(n)) {
        FEObject *obj = n->mpObject;
        const bool horiz = mbHorizontal != 0;
        float axisPos;
        float axisCenter, axisSize;
        if (!horiz) {
            axisPos = static_cast<float>(static_cast<L>(scroll) + static_cast<L>(n->mOffsetY) + static_cast<L>(mOriginY));
            axisCenter = mCenterY;
            axisSize = mStripH;
            FEngSetCenter(obj, mOriginX, axisPos);
        } else {
            axisPos = static_cast<float>(static_cast<L>(scroll) + static_cast<L>(n->mOffsetX) + static_cast<L>(mOriginX));
            axisCenter = mCenterX;
            axisSize = mStripW;
            FEngSetCenter(obj, axisPos, mOriginY);
        }
        const float axisCoord = axisPos;   // fVar6: the visibility / scale test position

        FEngSetSize(obj, n->mWidth, n->mHeight);

        const L half = static_cast<L>(axisSize) * static_cast<L>(kHalf);
        const L left = static_cast<L>(axisCenter) - half;
        const L right = static_cast<L>(axisCenter) + half;
        if (!mbVisible || static_cast<L>(axisCoord) < left || right < static_cast<L>(axisCoord)) {
            FEngSetColor(obj, 0);
            MarkHidden(obj);
        } else {
            FEngSetVisible(obj);
        }

        float align = 0.0f;
        if (mAlignment == 0) {
            float x, y;
            if (!horiz) {
                FEngGetTopLeft(obj, &x, &axisPos);
                align = x;
            } else {
                FEngGetTopLeft(obj, &axisPos, &y);
                align = y;
            }
        } else if (mAlignment == 1 || mAlignment == 2) {
            if (!horiz) {
                float x;
                FEngGetTopLeft(obj, &x, &axisPos);
                const L shrink = static_cast<L>(n->mWidth) - static_cast<L>(n->mWidth) * static_cast<L>(kUnselected);
                align = static_cast<float>(mAlignment == 2 ? shrink + static_cast<L>(x)
                                                           : shrink * static_cast<L>(kHalf) + static_cast<L>(x));
            } else {
                float y;
                FEngGetTopLeft(obj, &axisPos, &y);
                const L shrink = static_cast<L>(n->mHeight) - static_cast<L>(n->mHeight) * static_cast<L>(kUnselected);
                align = static_cast<float>(mAlignment == 2 ? shrink + static_cast<L>(y)
                                                           : shrink * static_cast<L>(kHalf) + static_cast<L>(y));
            }
        }

        float placeX = align;
        // PC quirk: the scale block writes the rounded scale over the 0.8 constant's stack slot (fst [esp+0x24]
        // is [esp+0x14] while the call arguments are still pushed), so the fade block's 4th argument becomes
        // scale * size instead of 0.8 * size. Without scaling the slot keeps 0.8.
        float fadeFactor = kUnselected;
        if (!mbScale) {
            if (n != mpCurrent) {
                FEngSetSize(obj, static_cast<float>(static_cast<L>(n->mWidth) * static_cast<L>(kUnselected)),
                            static_cast<float>(static_cast<L>(n->mHeight) * static_cast<L>(kUnselected)));
                if (!horiz) {
                    float y;
                    FEngGetTopLeft(obj, &axisPos, &y);
                    align = y;
                } else {
                    float x;
                    FEngGetTopLeft(obj, &x, &axisPos);
                    placeX = x;
                }
                FEngSetTopLeft(obj, placeX, align);
            }
        } else {
            L scale;
            if (!horiz) {
                scale = thumbnail_scale(axisCoord, mCenterY, mStripH,
                                        static_cast<float>(static_cast<L>(n->mHeight) * static_cast<L>(kUnselected)));
                const float rounded = static_cast<float>(scale);
                fadeFactor = rounded;
                const float h = static_cast<float>(scale * static_cast<L>(n->mHeight));
                const float w = static_cast<float>(static_cast<L>(rounded) * static_cast<L>(n->mWidth));
                FEngSetSize(obj, w, h);
                float y;
                FEngGetTopLeft(obj, &axisPos, &y);
                align = y;
            } else {
                scale = thumbnail_scale(axisCoord, mCenterX, mStripW,
                                        static_cast<float>(static_cast<L>(n->mWidth) * static_cast<L>(kUnselected)));
                const float rounded = static_cast<float>(scale);
                fadeFactor = rounded;
                const float h = static_cast<float>(scale * static_cast<L>(n->mHeight));
                const float w = static_cast<float>(static_cast<L>(rounded) * static_cast<L>(n->mWidth));
                FEngSetSize(obj, w, h);
                float x;
                FEngGetTopLeft(obj, &x, &axisPos);
                placeX = x;
            }
            FEngSetTopLeft(obj, placeX, align);
        }

        if (mbFadeAlpha) {
            const float size = horiz ? mStripW : mStripH;
            const float center = horiz ? mCenterX : mCenterY;
            const L fade = thumbnail_scale(axisCoord, center, size,
                                           static_cast<float>(static_cast<L>(fadeFactor) *
                                                              (horiz ? static_cast<L>(n->mWidth) : static_cast<L>(n->mHeight))));
            const int32_t alpha = static_cast<int32_t>(static_cast<uint32_t>(n->mu32Color) >> 24);
            const L a = n->mbDimmed ? static_cast<L>(kDimAlpha) : static_cast<L>(alpha);
            const int32_t outA = FEFtol(a * fade * fade * fade);
            FEngSetColor(obj, (static_cast<uint32_t>(outA) << 24) | (n->mu32Color & 0xFFFFFFu));
        }
    }
}

// PC 538760
void ThumbnailScroller::Update()
{
    if (mNodes.next == &mNodes || !mpCurrent) return;
    MoveObjectsTo(mpCubic->mVal);
    bridge::SplineSeek(mpCubic, bridge::RealTimeElapsed(), 0.0f, 0.0f);
    if (mPulse > kPulseLimit) mPulse = 0.0f;

    ThumbnailNode *cur = mpCurrent;
    const float alphaF = static_cast<float>(static_cast<int32_t>(static_cast<uint32_t>(cur->mu32Color) >> 24));
    float cx, cy;
    FEngGetCenter(cur->mpObject, &cx, &cy);
    const float axisCoord = mbHorizontal ? cx : cy;

    if (axisCoord > -3.0f && axisCoord < 3.0f) {
        if (mPulse > kPulseLimit) mPulse = 0.0f;
        const L t = mPulse <= kPulseCenter ? static_cast<L>(mPulse) : static_cast<L>(kPulseLimit) - static_cast<L>(mPulse);
        L f = t * static_cast<L>(alphaF) * static_cast<L>(kPulseScale) + static_cast<L>(kPulseBias);
        if (f > static_cast<L>(kPulseMax)) f = static_cast<L>(kPulseMax);
        f = f * static_cast<L>(kPulseMul) + static_cast<L>(kPulseAdd);
        if (cur->mbAnimate) {
            FEngSetSize(cur->mpObject, static_cast<float>(f * static_cast<L>(cur->mWidth)),
                        static_cast<float>(f * static_cast<L>(cur->mHeight)));
        }
        mPulse = static_cast<float>(static_cast<L>(mPulse) + static_cast<L>(kPulseStep));
    }
    UpdateIconOverlay(reinterpret_cast<const IconOption *>(cur), mpLock, mpButton);
}

void ThumbnailScroller::StartDim()
{
    mbFadeAlpha = 0;
    SetVisibility(false);
}

void ThumbnailScroller::StopDim()
{
    mbFadeAlpha = 1;
    SetVisibility(true);
}

void ThumbnailScroller::SetMasterObject(FEObject *master) { FEngGetCenter(master, &mCenterX, &mCenterY); }

void ThumbnailScroller::UpdateFade(ThumbnailNode *n)
{
    if (!n || !n->mpObject || !n->mpObject->mpData) return;
    const uint32_t c = FEngGetObjectColorPacked(n->mpObject);
    float a = static_cast<float>(c >> 24) - 2.5f;
    if (a < 40.0f) a = 40.0f;
    FEngSetColor(n->mpObject, (c & 0xFFFFFFu) | (static_cast<uint32_t>(static_cast<int32_t>(a)) << 24));
}
