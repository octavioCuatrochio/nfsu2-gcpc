#include "IconScroller.h"
#include "bridge/Bridge.h"
#include "bWare/bString.h"
#include "FEHash.h"
#include "FEMath.h"
#include "FEObject.h"
#include "FEPackage.h"
#include "FEPackageManager.h"
#include "FEngUtil.h"
#include "cFEng.h"
#include <cstdio>
#include <cstring>

namespace {
using L = long double;
constexpr float kScaleTimeStep = 0.00025f;   // PC 0x784268: RealTimer ticks -> seconds
constexpr float kUnselected = 0.8f;          // PC 0x7850EC

IconOption *FromNode(WidgetListNode *n)
{
    return n ? reinterpret_cast<IconOption *>(reinterpret_cast<uint8_t *>(n) - offsetof(IconOption, node)) : nullptr;
}
// the list head seen as an option (the loops' end marker; the panel itself on PC)
IconOption *End(IconPanel &p) { return FromNode(&p.mOptions); }
IconOption *First(IconPanel &p) { return FromNode(p.mOptions.next); }
IconOption *Last(IconPanel &p) { return FromNode(p.mOptions.prev); }
IconOption *Next(IconOption *o) { return FromNode(o->node.next); }
IconOption *Prev(IconOption *o) { return FromNode(o->node.prev); }

// bList TraversebList(null) (PC 43BE80): number of nodes
int32_t Count(const WidgetListNode *head)
{
    int32_t n = 0;
    for (const WidgetListNode *e = head->next; e != head; e = e->next) ++n;
    return n;
}

void Append(IconPanel &p, IconOption *opt)
{
    WidgetListNode *tail = p.mOptions.prev, *n = &opt->node;
    tail->next = n;
    p.mOptions.prev = n;
    n->prev = tail;
    n->next = &p.mOptions;
}


// first option becomes current when none is (AddOption)
void SelectFirstIfNone(IconPanel &p)
{
    if (p.mpCurrent) return;
    p.mpCurrent = First(p);
    FEngSetCurrentButton(p.mpPackageName, p.mpCurrent->mpObject);
}
}  // namespace

// ---- IconOption ----

IconOption *IconOption::Construct(uint32_t nameHash, uint32_t header, uint32_t desc)
{
    mHeaderHash = header;
    mNameHash = nameHash;
    mDescHash = desc;
    mpObject = nullptr;
    mOffsetX = 0.0f;
    mOffsetY = 0.0f;
    mbLocked = 0;
    _unknown21 = 1;
    mbShowLock = 0;
    mbFadeInButton = 0;
    vtable = bridge::IconOptionVTable();
    mScaleTarget = mScaleStart = mScaleDuration = mScale = 1.0f;
    mbScaleDone = 1;
    mbActsDirectly = 0;
    return this;
}

CPPaintOption *CPPaintOption::Construct(uint32_t paint, uint32_t nameHash, uint32_t header, uint32_t desc, bool showLock)
{
    IconOption &o = base;
    o.mHeaderHash = header;
    o.mNameHash = nameHash;
    o.mDescHash = desc;
    o.mpObject = nullptr;
    o.mOffsetX = 0.0f;
    o.mOffsetY = 0.0f;
    o.mbLocked = 0;
    o.mScaleTarget = o.mScaleStart = o.mScaleDuration = o.mScale = 1.0f;
    o.mbActsDirectly = 0;
    o._unknown21 = 1;
    o.mbScaleDone = 1;
    o.vtable = bridge::CPPaintOptionVTable();
    mPaint = paint;
    o.mbFadeInButton = 1;
    o.mbShowLock = showLock;
    return this;
}

void IconOption::Destruct() { vtable = bridge::ThumbnailNodeVTable(); }

void IconOption::SetFEngObject(FEObject *img)
{
    if (!img) return;
    mpObject = img;
    FEngGetSize(img, &mWidth, &mHeight);
    mColor = FEngGetObjectColorPacked(img);
}

void IconOption::StartScale(float target, float duration)
{
    mScaleTarget = target;
    mScaleDuration = duration;
    mScaleStart = static_cast<float>(static_cast<L>(bridge::RealTimer()) * kScaleTimeStep);
    mbScaleDone = 0;
}

IconOption *IconOption::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

// ---- IconPanel ----

IconPanel *IconPanel::Construct(const char *pkg, const char *master, const char *lock, const char *button, const char *prefix, bool wrap)
{
    vtable = bridge::IconPanelVTable();
    mOptions.next = mOptions.prev = &mOptions;
    mpPrefix = prefix;
    mpCurrent = nullptr;
    mpOptionMaster = mpLockMaster = nullptr;
    mpPackageName = pkg;
    mSpacing = 10.0f;
    mNextNumber = 1;
    mbWrap = wrap;
    mbHorizontal = mbDirty = mbInputEnabled = 1;
    mbAllowLocked = 0;
    mLastDir = -9;
    mpOptionMaster = FEngFindObject(pkg, FEHashUpper(master));
    mpLockMaster = FEngFindObject(mpPackageName, FEHashUpper(lock));
    mpButtonMaster = FEngFindObject(mpPackageName, FEHashUpper(button));
    return this;
}

void IconPanel::Destruct()
{
    vtable = bridge::IconPanelVTable();
    DeleteAllNodeOwners(&mOptions);
}

IconPanel *IconPanel::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

void IconPanel::Update()
{
    float w = 0.0f, h = 0.0f;
    AnimateList(&w, &h);
}

FEObject *IconPanel::AddOption(IconOption *opt)
{
    char name[64];   // PC: bStrCat(prefix, "%d") + FEngSNPrintf in 0x20-byte buffers
    uiwm::OptionName(name, mpPrefix, static_cast<uint32_t>(mNextNumber));
    FEObject *img = FEngFindImage(mpPackageName, name);
    if (!img) return nullptr;
    ++mNextNumber;
    if (!opt) return nullptr;
    opt->SetFEngObject(img);
    Append(*this, opt);
    SelectFirstIfNone(*this);
    return img;
}

void IconPanel::ClearList()
{
    if (mOptions.next == &mOptions) return;
    for (IconOption *o = First(*this); o != End(*this); o = Next(o)) {
        FEngSetSize(o->mpObject, o->mWidth, o->mHeight);
        FEngSetColor(o->mpObject, o->mColor);
        FEngSetTopLeft(o->mpObject, 6969.0f, 6969.0f);   // parked off screen (PC 0x45D9C800)
    }
    DeleteAllNodeOwners(&mOptions);
    mNextNumber = 1;
}

void IconPanel::Act(uint32_t msg, FEObject *obj, uint32_t a3, uint32_t a4)
{
    if (mpCurrent && obj == mpCurrent->mpObject) bridge::IconOption_React(mpCurrent, mpPackageName, msg, obj, a3, a4);
}

int32_t IconPanel::GetOptionIndex(const IconOption *opt) const
{
    if (!opt) return -1;
    auto &self = const_cast<IconPanel &>(*this);
    int32_t i = 1;
    for (IconOption *o = First(self); o != End(self); o = Next(o), ++i)
        if (o == opt) return i;
    return -1;
}

int32_t IconPanel::GetOptionIndex() const { return mpCurrent ? GetOptionIndex(mpCurrent) : 0; }

bool IconPanel::SetSelection(int32_t index)
{
    IconOption *opt = FromNode(ListGetNode(&mOptions, static_cast<uint32_t>(index)));
    if (!opt || opt == mpCurrent) return false;
    if (opt->mbLocked && !mbAllowLocked) return false;
    return SetSelection(opt);   // PC: same body inlined (no null check of the old current option)
}

bool IconPanel::SetSelection(IconOption *opt)
{
    if (opt->mbLocked && !mbAllowLocked) return false;
    mpCurrent->StartScale(1.0f, 0.125f);   // PC 0x799374
    mpCurrent = opt;
    FEngSetCurrentButton(mpPackageName, opt->mpObject);
    mpCurrent->StartScale(1.25f, 0.125f);   // PC 0x799378
    return true;
}

bool IconPanel::HandleMouse()
{
    const uint8_t *mouse = bridge::MouseGlobals();
    for (IconOption *o = First(*this); o != End(*this); o = Next(o)) {
        int32_t mx, my;
        std::memcpy(&mx, mouse, 4);
        std::memcpy(&my, mouse + 4, 4);
        const float x = static_cast<float>(static_cast<L>(mx) - 320.0f), y = static_cast<float>(static_cast<L>(my) - 240.0f);
        FERect r;
        if (!o->mpObject || !FEngGet2DExtents(o->mpObject, &r)) continue;
        if (!(x >= r.left) || !(x <= r.right) || !(y >= r.top) || !(y <= r.bottom)) continue;
        if (o == mpCurrent) return false;
        bridge::IconPanel_SetSelectionOption(this, o);
        mbDirty = 1;
        return true;
    }
    return false;
}

void IconPanel::SetInitialPos()
{
    // PC reads the first option's object size without null checks (a list is required)
    const float n = static_cast<float>(Count(&mOptions));
    const IconOption *first = First(*this);
    const auto *size = static_cast<const float *>(first->mpObject->mpData) + 14;   // data +0x38
    const float w = size[0], h = size[1];
    float cx = 0.0f, cy = 0.0f;
    FEngGetCenter(mpOptionMaster, &cx, &cy);
    const L gaps = (static_cast<L>(n) - 1.0f) * mSpacing;
    const float x0 = static_cast<float>(static_cast<L>(cx) - (static_cast<L>(w) * n + gaps) * 0.5f);
    const float y0 = static_cast<float>(static_cast<L>(cy) - (static_cast<L>(h) * n + gaps) * 0.5f);
    float i = 0.0f;
    for (IconOption *o = First(*this); o != End(*this); o = Next(o)) {
        if (mbHorizontal) {
            const float y = static_cast<float>(static_cast<L>(cy) - static_cast<L>(h) * 0.5f);
            const float x = static_cast<float>((static_cast<L>(w) + mSpacing) * i + x0);
            FEngSetTopLeft(o->mpObject, x, y);
        } else {
            const float y = static_cast<float>((static_cast<L>(h) + mSpacing) * i + y0);
            const float x = static_cast<float>(static_cast<L>(cx) - static_cast<L>(h) * 0.5f);   // PC centres x by the height
            FEngSetTopLeft(o->mpObject, x, y);
        }
        i = static_cast<float>(static_cast<L>(i) + 1.0f);
    }
    bridge::IconPanel_SetSelectionOption(this, mpCurrent);
}

void IconPanel::Scroll(int32_t dir)
{
    IconOption *const cur = mpCurrent;
    IconOption *o = cur;
    if (dir == -1) {
        IconOption *const first = First(*this);
        if (o != first)
            for (;;) {
                o = Prev(o);
                if (!o->mbLocked) goto select;
                if (mbAllowLocked || o == first) break;
            }
    } else if (dir == 1) {
        IconOption *const last = Last(*this);
        if (o != last)
            for (;;) {
                o = Next(o);
                if (!o->mbLocked) goto select;
                if (mbAllowLocked || o == last) break;
            }
    }
    if (o->mbLocked && !mbAllowLocked) return;
select:
    if (o == cur) return;
    bridge::IconPanel_SetSelectionOption(this, o);
    mbDirty = 1;
    mLastDir = dir;
}

void IconPanel::ScrollWrapped(int32_t dir)
{
    IconOption *const cur = mpCurrent;
    IconOption *o = cur;
    if (dir == -1) {
        for (;;) {   // PC loops forever when every icon is locked
            o = o == First(*this) ? Last(*this) : Prev(o);
            if (!o->mbLocked) goto select;
            if (mbAllowLocked) break;
        }
    } else if (dir == 1) {
        for (;;) {
            o = o == Last(*this) ? First(*this) : Next(o);
            if (!o->mbLocked) goto select;
            if (mbAllowLocked) break;
        }
    }
    if (o->mbLocked && !mbAllowLocked) return;
select:
    if (o == cur) return;
    bridge::IconPanel_SetSelectionOption(this, o);
    mbDirty = 1;
    mLastDir = dir;
}

void IconPanel::ScrollNext() { mbWrap ? ScrollWrapped(1) : Scroll(1); }
void IconPanel::ScrollPrev() { mbWrap ? ScrollWrapped(-1) : Scroll(-1); }

void IconPanel::AnimateList(float *w, float *h) { AnimateSelected(w, h); }

void IconPanel::AnimateSelected(float *w, float *h)
{
    mbDirty = 0;
    *w = 0.0f;
    *h = 0.0f;
    for (IconOption *o = First(*this); o != End(*this); o = Next(o)) {
        L s = 1.0f;
        if (!o->mbScaleDone) {
            const float t = static_cast<float>(static_cast<L>(o->mScaleTarget) - o->mScale);
            const L v = (static_cast<L>(bridge::RealTimer()) * kScaleTimeStep - o->mScaleStart) / o->mScaleDuration * t + o->mScale;
            const bool running = (t >= 0.0f && v < o->mScaleTarget) || (t < 0.0f && v > o->mScaleTarget);
            if (running) {
                FEngSetSize(o->mpObject, static_cast<float>(v * o->mWidth), static_cast<float>(v * o->mHeight));
                mbDirty = 1;
            } else {
                // PC: size at the target, but the stored scale is the overshooting value
                FEngSetSize(o->mpObject, static_cast<float>(static_cast<L>(o->mWidth) * o->mScaleTarget),
                            static_cast<float>(static_cast<L>(o->mHeight) * o->mScaleTarget));
                o->mScale = static_cast<float>(v);
                o->mbScaleDone = 1;
            }
            s = v;
        }
        *w = static_cast<float>(s * o->mWidth + *w);
        *h = static_cast<float>(s * o->mHeight + *h);
        if (o != Last(*this)) {
            *w = static_cast<float>(static_cast<L>(*w) + mSpacing);
            *h = static_cast<float>(static_cast<L>(mSpacing) + *h);
        }
    }
}

bool IconPanel::IsHead() const { return mpCurrent == FromNode(mOptions.next); }
bool IconPanel::IsTail() const { return mpCurrent == FromNode(mOptions.prev); }

// ---- IconScroller ----

IconScroller *IconScroller::Construct(const char *pkg, const char *master, const char *lock, const char *button, const char *prefix, float width)
{
    base.Construct(pkg, master, lock, button, prefix, false);
    base.vtable = bridge::IconScrollerVTable();
    mScrollBar.Construct(pkg, "ScrollBar", false, false, false);
    mPos.mVal = mPos.mdVal = mPos.mValDesired = mPos.mdValDesired = 0.0f;
    mPos.mFlags = 0;
    mPos.mDuration = 1.0f;
    mPos._unknown28 = 1.0f;
    mPos.mState = 1;
    mAlign = 1;
    mIndex = 1;
    mStripWidth = width;
    _unknownD0 = 0.0f;
    mCenterX = mCenterY = 0.0f;
    mNextX = 0.0f;
    mbFade = 0;
    base.mSpacing = 40.0f;
    if (FEObject *m = FEngFindObject(pkg, FEHashUpper(master))) {
        FEngGetCenter(m, &mCenterX, &mCenterY);
        FEngSetInvisible(m);
    }
    mPos.mState = 0;
    mPos.mDuration = 0.3f;
    return this;
}

void IconScroller::Update()
{
    IconPanel &p = base;
    if (p.mOptions.next == &p.mOptions || !p.mpCurrent) return;
    if (p.mbDirty) {
        p.mbDirty = 0;
        bridge::SetIconScrollTime(bridge::RealTimer());
        mScrollBar.Update(1, p.mNextNumber - 1, mIndex);
        mPos.mValDesired = -p.mpCurrent->mOffsetX;
        if (!(mPos.mValDesired == mPos.mVal)) mPos.mFlags = 2;
        UpdateArrows(p.mLastDir);
        p.mLastDir = -9;
    }
    UpdateIconOverlay(p.mpCurrent, p.mpLockMaster, p.mpButtonMaster);
    for (IconOption *o = First(p); o != End(p); o = Next(o)) PositionOption(o);
    bridge::SplineSeek(&mPos, bridge::RealTimeElapsed(), 0.0f, 0.0f);
}

FEObject *IconScroller::AddOption(IconOption *opt)
{
    IconPanel &p = base;
    char name[0x40];   // PC: 0x20 on the stack, FEngSNPrintf(.., 0x20, "%s%d", prefix, n) ignores the size
    std::snprintf(name, sizeof name, "%s%d", p.mpPrefix, p.mNextNumber);
    FEObject *img = FEngFindImage(p.mpPackageName, name);
    if (!img) {
        if (opt) bridge::VirtualScalarDelete(opt);
        return nullptr;
    }
    ++p.mNextNumber;
    if (!opt) return nullptr;
    opt->SetFEngObject(img);
    opt->mOffsetX = mNextX;
    opt->mColor = 0xFFFFFFFFu;
    FEngGetSize(opt->mpObject, &opt->mWidth, &opt->mHeight);
    float w, h;
    FEngGetSize(opt->mpObject, &w, &h);
    mNextX = static_cast<float>(static_cast<L>(p.mSpacing) + mNextX + w);
    Append(p, opt);
    SelectFirstIfNone(p);
    return img;
}

void IconScroller::SetInitialPos(int32_t index)
{
    IconPanel &p = base;
    IconOption *opt = First(p);
    if (index > 0) opt = FromNode(ListGetNode(&p.mOptions, static_cast<uint32_t>(index - 1)));
    mPos.mValDesired = -(p.mbHorizontal ? opt->mOffsetX : opt->mOffsetY);
    if (!(mPos.mValDesired == mPos.mVal)) mPos.mFlags = 2;
    mPos.mVal = mPos.mValDesired;
    mPos.mdVal = mPos.mdValDesired;
    mPos.mFlags = 0;
    for (IconOption *o = First(p); o != End(p); o = Next(o)) PositionOption(o);
    bridge::IconPanel_SetSelectionOption(this, index ? opt : First(p));
    mScrollBar.Update(1, p.mNextNumber - 1, mIndex);
}

bool IconScroller::SetSelection(int32_t index)
{
    IconPanel &p = base;
    for (IconOption *o = First(p); o != End(p); o = Next(o)) {
        const uint32_t h = o->mpObject->mu32NameHash;   // PC: no null check
        FEngSetButtonState(p.mpPackageName, h, false);
    }
    IconOption *opt = FromNode(ListGetNode(&p.mOptions, static_cast<uint32_t>(index)));
    if (!opt || opt == p.mpCurrent) return false;
    if (opt->mbLocked && !p.mbAllowLocked) return false;
    p.mpCurrent = opt;
    if (bridge::cFEngInstance()) cFEng::SetButtonState(p.mpPackageName, opt->mpObject->mu32NameHash, true);
    FEngSetCurrentButton(p.mpPackageName, p.mpCurrent->mpObject);
    mIndex = index;
    return true;
}

bool IconScroller::SetSelection(IconOption *opt)
{
    IconPanel &p = base;
    for (IconOption *o = First(p); o != End(p); o = Next(o)) {
        const uint32_t h = o->mpObject->mu32NameHash;
        FEngSetButtonState(p.mpPackageName, h, false);
    }
    if (opt->mbLocked && !p.mbAllowLocked) return false;
    p.mpCurrent = opt;
    {
        const uint32_t h = opt->mpObject->mu32NameHash;
        FEngSetButtonState(p.mpPackageName, h, true);
    }
    FEngSetCurrentButton(p.mpPackageName, p.mpCurrent->mpObject);
    mIndex = p.GetOptionIndex(opt);
    return true;
}

void IconScroller::StartDim()
{
    mbFade = 0;
    for (IconOption *o = First(base); o != End(base); o = Next(o)) FEngSetColor(o->mpObject, o->mColor);
}

void IconScroller::StopDim()
{
    mbFade = 1;
    for (IconOption *o = First(base); o != End(base); o = Next(o)) PositionOption(o);
}

void IconScroller::ClipEdges(IconOption *opt, float x)
{
    const L half = static_cast<L>(mStripWidth) * 0.5f;
    const L left = mCenterX - half;
    if (left > x || half + mCenterX < x) FEngSetInvisible(opt->mpObject);
    else FEngSetVisible(opt->mpObject);
}

void IconScroller::PositionOption(IconOption *opt)
{
    if (!opt) return;
    const float x = static_cast<float>(static_cast<L>(mPos.mVal) + opt->mOffsetX + mCenterX);
    ClipEdges(opt, x);
    FEngSetSize(opt->mpObject, opt->mWidth, opt->mHeight);
    FEngSetCenter(opt->mpObject, x, mCenterY);
    float tx, ty = 0.0f;   // ty: edge the shrunk icon keeps (PC: 0 for an unknown alignment)
    const L shrink = static_cast<L>(opt->mHeight) - static_cast<L>(opt->mHeight) * kUnselected;
    switch (mAlign) {
    case 0: FEngGetTopLeft(opt->mpObject, &tx, &ty); break;
    case 1: FEngGetTopLeft(opt->mpObject, &tx, &ty); ty = static_cast<float>(shrink * 0.5f + ty); break;
    case 2: FEngGetTopLeft(opt->mpObject, &tx, &ty); ty = static_cast<float>(shrink + ty); break;
    default: break;
    }
    if (opt != base.mpCurrent) {
        FEngSetSize(opt->mpObject, static_cast<float>(static_cast<L>(opt->mWidth) * kUnselected),
                    static_cast<float>(static_cast<L>(opt->mHeight) * kUnselected));
        float lx, ly;
        FEngGetTopLeft(opt->mpObject, &lx, &ly);
        FEngSetTopLeft(opt->mpObject, lx, ty);
    }
    if (mbFade) {
        const float fade = static_cast<float>(thumbnail_scale(x, mCenterX, mStripWidth, static_cast<float>(static_cast<L>(opt->mWidth) * kUnselected)));
        UpdateFade(opt, fade);
    }
}

void IconScroller::UpdateFade(IconOption *opt, float fade)
{
    if (!opt) return;
    FEObject *obj = opt->mpObject;
    if (!obj || !obj->mpData) return;
    const uint32_t rgb = FEngGetObjectColorPacked(obj) & 0xFFFFFFu;
    L a = static_cast<L>(static_cast<int32_t>(opt->mColor >> 24));
    if (opt->mbLocked && !base.mbAllowLocked) a = 150.0f;   // PC 0x78A584
    const int32_t alpha = FEFtol(a * fade);
    FEngSetColor(obj, static_cast<uint32_t>(alpha) << 24 | rgb);
}

void IconScroller::UpdateArrows(int32_t dir)
{
    FEObject *arrow1 = mScrollBar.mpArrow1;
    if (dir == 1 || dir == -10) {
        if (arrow1 && !FEngIsScriptRunning(arrow1, "Static")) FEngSetScript(arrow1, "Static", true);
        if (mScrollBar.mpArrow2 && !FEngIsScriptRunning(mScrollBar.mpArrow2, "Blink")) FEngSetScript(mScrollBar.mpArrow2, "Blink", true);
    } else if (dir == -1 || dir == 10) {
        FEScrollBar_StaticArrow2(&mScrollBar);
        mScrollBar.BlinkArrow1();
    }
    IconPanel &p = base;
    if (p.mpCurrent == First(p)) FEngSetInvisible(mScrollBar.mpArrow1);
    else if (p.mpCurrent == Last(p)) FEngSetInvisible(mScrollBar.mpArrow2);
    else {
        FEngSetVisible(mScrollBar.mpArrow1);
        FEngSetVisible(mScrollBar.mpArrow2);
    }
}

bool IconScroller::HandleMouse()
{
    mScrollBar.HandleMouse();
    switch (mScrollBar.mHit) {   // PC table: page -10 and arrow 1 -> +1, arrow -1 and page 10 -> -1
    case -10:
    case 1: base.mbWrap ? base.ScrollWrapped(1) : base.Scroll(1); return true;
    case -1:
    case 10: base.mbWrap ? base.ScrollWrapped(-1) : base.Scroll(-1); return true;
    default: return false;
    }
}

// ---- free helpers ----

long double thumbnail_scale(float x, float center, float width, float w)
{
    const L half = static_cast<L>(width) * 0.5f;
    const float right = static_cast<float>(static_cast<L>(center) + half);
    const float left = static_cast<float>(static_cast<L>(center) - half);
    if (x < left || x > right) return 0.0f;
    if (x >= left && static_cast<L>(left) + w > x) return (static_cast<L>(x) - left) / w;
    if (x <= right && static_cast<L>(right) - w < x) return (static_cast<L>(right) - x) / w;
    return 1.0f;
}

void UpdateIconOverlay(const IconOption *opt, FEObject *lock, FEObject *button)
{
    if (opt->mbShowLock) {
        FEngSetScript(lock, "SHOW", true);
        FEngSetScript(button, "HIDE", true);
    } else if (opt->mbFadeInButton) {
        FEngSetScript(lock, "HIDE", true);
        if (FEngIsScriptRunning(button, "FadeIn") || FEngIsScriptRunning(button, "Animate")) return;
        FEngSetScript(button, "FadeIn", true);
    } else {
        FEngSetScript(lock, "HIDE", true);
        FEngSetScript(button, "HIDE", true);
    }
}

// ---- IconScrollerMenu ----

IconScrollerMenu *IconScrollerMenu::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    base.vtable = bridge::IconScrollerMenuVTable();
    mScroller.Construct(base.mpPackageName, "OPTION_MASTER", "LOCK_MASTER", "BUTTON_MASTER", "option_", 480.0f);
    mpTitle = mpDesc = nullptr;
    mPrevMsg = 0;
    mpPrevObj = nullptr;
    mPrevArg = mPrevPkg = 0;
    mpTitle = FEngFindString(base.mpPackageName, FEHashUpper("ICON_TITLE"));
    mpDesc = FEngFindString(base.mpPackageName, FEHashUpper(""));
    base.mbSounds = 0;
    mbInputStopped = 0;
    base.mSoundTrigger = 9;
    return this;
}

void IconScrollerMenu::Destruct()
{
    base.vtable = bridge::IconScrollerMenuVTable();
    mScroller.base.vtable = bridge::IconPanelVTable();
    DeleteAllNodeOwners(&mScroller.base.mOptions);
    base.vtable = bridge::MenuScreenVTable();   // MenuScreen dtor body (PC 510D40)
}

IconScrollerMenu *IconScrollerMenu::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

void IconScrollerMenu::StorePrevNotification(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg)
{
    mPrevMsg = msg;
    mpPrevObj = obj;
    mPrevArg = arg;
    mPrevPkg = pkg;
}

void IconScrollerMenu::RefreshHeader()
{
    const IconOption *cur = mScroller.base.mpCurrent;
    FEngSetLanguageHash(mpTitle, cur ? cur->mHeaderHash : 0);
    FEngSetLanguageHash(mpDesc, cur ? cur->mDescHash : 0);
}

void IconScrollerMenu::AddOption(IconOption *opt)
{
    auto *img = static_cast<FEObject *>(bridge::IconPanel_AddOption(&mScroller, opt));
    if (!img) return;
    const uint32_t tex = opt->mNameHash;   // the option's texture
    if (img->_unknown24 != tex) {
        img->mu32Flags |= 0x400000u;
        img->_unknown24 = tex;
    }
}

void IconScrollerMenu::StartInput()
{
    if (!mbInputStopped) return;
    mScroller.base.mbInputEnabled = 1;
    FEPackage *pkg = FEngFindPackage(base.mpPackageName);
    pkg->RestoreControlMask();   // PC: no null check
    mbInputStopped = 0;
}

void IconScrollerMenu::StopInput()
{
    mScroller.base.mbInputEnabled = 0;
    FEPackage *pkg = FEngFindPackage(base.mpPackageName);
    pkg->SetControlMaskOnly(0);
    mbInputStopped = 1;
}

void IconScrollerMenu::StartDim()
{
    mScroller.StartDim();
    FEngSetScript(base.mpPackageName, 0x47FF4E7Cu, 0x9E99u, true);
}

void IconScrollerMenu::StopDim()
{
    StartInput();
    mScroller.StopDim();
    FEngSetScript(base.mpPackageName, 0x47FF4E7Cu, 0x3D8EABCu, true);
}

void IconScrollerMenu::NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkgArg)
{
    IconPanel &p = mScroller.base;
    const uint32_t trigger = base.mSoundTrigger;
    uint32_t event = 0;
    const bool empty = p.mOptions.next == &p.mOptions;
    auto afterMove = [&] {   // header, remembered button, sound
        bridge::IconScrollerMenu_RefreshHeader(this);
        FEngSetLastButton(base.mpPackageName, p.GetOptionIndex());
        if (event) bridge::MakeFrontendSound(event, trigger);
    };
    switch (msg) {
    case 0x0C407210u: {   // accept
        IconOption *cur = p.mpCurrent;
        if (cur->mbShowLock) event = 0x13;
        if (empty || !p.mbInputEnabled || cur->mbLocked || cur->mbShowLock) {
            if (event) bridge::MakeFrontendSound(event, trigger);
            return;
        }
        event = 8;
        if (p.mpCurrent && p.mpCurrent->mbActsDirectly) {
            bridge::IconPanel_Act(&mScroller, 0x0C407210u, obj, arg, pkgArg);
        } else {
            mScroller.StartDim();
            StopInput();
        }
        StorePrevNotification(0x0C407210u, obj, arg, pkgArg);
        bridge::MakeFrontendSound(event, trigger);
        return;
    }
    case 0x0153E234u: mScroller.mbFade = 1; return;
    case 0x251BF83Eu: StopDim(); return;
    case 0x35F8620Bu:
        bridge::IconScrollerMenu_RefreshHeader(this);
        mScroller.mbFade = 1;
        return;
    case 0x5ECA113Du:
    case 0x90CA113Du:
    case 0xD9CA113Du: StartInput(); return;
    case 0x72619778u:   // up
        if (empty || p.mbHorizontal || !p.mbInputEnabled) return;
        if (!p.IsHead()) p.ScrollPrev();
        event = 6;
        afterMove();
        return;
    case 0x911C0A4Bu:   // down
        if (empty || p.mbHorizontal || !p.mbInputEnabled) return;
        if (!p.IsTail()) event = 7;
        p.ScrollNext();
        afterMove();
        return;
    case 0x911AB364u:   // back
        event = 0xA;
        mScroller.StartDim();
        StorePrevNotification(0x911AB364u, obj, arg, pkgArg);
        StopInput();
        p.mbInputEnabled = 0;
        bridge::MakeFrontendSound(event, trigger);
        return;
    case 0x9803F6E2u:   // mouse
        if (!bridge::IconPanel_HandleMouse(&mScroller)) return;
        bridge::IconScrollerMenu_RefreshHeader(this);
        FEngSetLastButton(base.mpPackageName, p.GetOptionIndex());
        return;
    case 0x96DDA9E6u: StartDim(); return;
    case 0x9120409Eu:   // left
        if (empty || !p.mbHorizontal || !p.mbInputEnabled) return;
        if (!p.IsHead()) event = 4;
        p.ScrollPrev();
        afterMove();
        return;
    case 0xB5971BF1u:   // right
        if (empty || !p.mbHorizontal || !p.mbInputEnabled) return;
        if (!p.IsTail()) event = 5;
        p.ScrollNext();
        afterMove();
        return;
    case 0xC98356BAu: bridge::IconPanel_Update(&mScroller); return;
    case 0xE1FDE1D1u:   // out-script done: hand the stored accept to the option
        StartInput();
        if (empty || p.mpCurrent->mbActsDirectly) return;
        bridge::IconPanel_Act(&mScroller, mPrevMsg, mpPrevObj, mPrevArg, mPrevPkg);
        return;
    default: return;
    }
}
