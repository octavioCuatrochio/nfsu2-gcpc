#include "FERender.h"
#include <cstring>
#include "bridge/Bridge.h"
#include "FEngine.h"
#include "FEObject.h"
#include "FEPackage.h"
#include "FEButtonMap.h"

using L = long double;   // x87 evaluation (see FINDINGS: precision follows the FPU control word on i686)

// Generated from the PC asm by tooling/x87sym.py (summation order per element is the original's).
void FEMultMatrix(FEMatrix4 *out, const FEMatrix4 *pa, const FEMatrix4 *pb)
{
    const FEMatrix4 &a = *pa, &b = *pb;
    const float t1 = float(((((L(a.m[0]) * L(b.m[0])) + (L(b.m[3]) * L(a.m[12]))) + (L(b.m[2]) * L(a.m[8]))) + (L(b.m[1]) * L(a.m[4]))));
    const float t2 = float(((((L(a.m[13]) * L(b.m[3])) + (L(a.m[1]) * L(b.m[0]))) + (L(b.m[1]) * L(a.m[5]))) + (L(a.m[9]) * L(b.m[2]))));
    const float t3 = float(((((L(a.m[10]) * L(b.m[2])) + (L(a.m[6]) * L(b.m[1]))) + (L(a.m[2]) * L(b.m[0]))) + (L(b.m[3]) * L(a.m[14]))));
    const float t4 = float(((((L(a.m[7]) * L(b.m[1])) + (L(a.m[15]) * L(b.m[3]))) + (L(b.m[2]) * L(a.m[11]))) + (L(b.m[0]) * L(a.m[3]))));
    const float t5 = float(((((L(b.m[6]) * L(a.m[8])) + (L(a.m[4]) * L(b.m[5]))) + (L(b.m[7]) * L(a.m[12]))) + (L(b.m[4]) * L(a.m[0]))));
    const float t6 = float(((((L(b.m[6]) * L(a.m[9])) + (L(b.m[4]) * L(a.m[1]))) + (L(b.m[5]) * L(a.m[5]))) + (L(b.m[7]) * L(a.m[13]))));
    const float t7 = float(((((L(b.m[7]) * L(a.m[14])) + (L(b.m[6]) * L(a.m[10]))) + (L(a.m[6]) * L(b.m[5]))) + (L(b.m[4]) * L(a.m[2]))));
    const float t8 = float(((((L(a.m[15]) * L(b.m[7])) + (L(b.m[6]) * L(a.m[11]))) + (L(b.m[4]) * L(a.m[3]))) + (L(a.m[7]) * L(b.m[5]))));
    const float t9 = float(((((L(b.m[10]) * L(a.m[8])) + (L(b.m[9]) * L(a.m[4]))) + (L(b.m[11]) * L(a.m[12]))) + (L(b.m[8]) * L(a.m[0]))));
    const float t10 = float(((((L(b.m[10]) * L(a.m[9])) + (L(b.m[8]) * L(a.m[1]))) + (L(b.m[9]) * L(a.m[5]))) + (L(b.m[11]) * L(a.m[13]))));
    const float t11 = float(((((L(b.m[11]) * L(a.m[14])) + (L(b.m[10]) * L(a.m[10]))) + (L(a.m[6]) * L(b.m[9]))) + (L(b.m[8]) * L(a.m[2]))));
    const float t12 = float(((((L(a.m[15]) * L(b.m[11])) + (L(b.m[10]) * L(a.m[11]))) + (L(b.m[8]) * L(a.m[3]))) + (L(a.m[7]) * L(b.m[9]))));
    const float t13 = float(((((L(b.m[14]) * L(a.m[8])) + (L(b.m[13]) * L(a.m[4]))) + (L(b.m[15]) * L(a.m[12]))) + (L(b.m[12]) * L(a.m[0]))));
    const float t14 = float(((((L(b.m[14]) * L(a.m[9])) + (L(b.m[12]) * L(a.m[1]))) + (L(b.m[13]) * L(a.m[5]))) + (L(b.m[15]) * L(a.m[13]))));
    const float t15 = float(((((L(b.m[15]) * L(a.m[14])) + (L(b.m[14]) * L(a.m[10]))) + (L(a.m[6]) * L(b.m[13]))) + (L(b.m[12]) * L(a.m[2]))));
    const float t16 = float(((((L(a.m[15]) * L(b.m[15])) + (L(b.m[14]) * L(a.m[11]))) + (L(b.m[12]) * L(a.m[3]))) + (L(a.m[7]) * L(b.m[13]))));
    const float r[16] = {t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16};
    std::memcpy(out->m, r, sizeof r);
}

void FEQuatGetMatrix(const FEQuaternion *q, FEMatrix4 *out)
{
    const L x2 = L(q->x) + q->x, y2 = L(q->y) + q->y;
    const float z2 = static_cast<float>(L(q->z) + q->z);
    const float xx = static_cast<float>(x2 * q->x), xy = static_cast<float>(y2 * q->x), xz = static_cast<float>(L(z2) * q->x);
    const float yy = static_cast<float>(y2 * q->y), yz = static_cast<float>(L(z2) * q->y), zz = static_cast<float>(L(z2) * q->z);
    const float wx = static_cast<float>(x2 * q->w);
    const L wy = y2 * q->w, wz = L(z2) * q->w;
    float *m = out->m;
    m[12] = m[13] = m[14] = m[3] = m[7] = m[11] = 0.0f;
    m[0] = static_cast<float>(1.0L - (L(zz) + yy));
    m[15] = 1.0f;
    m[4] = static_cast<float>(L(xy) - wz);
    m[8] = static_cast<float>(wy + xz);
    m[1] = static_cast<float>(wz + xy);
    m[5] = static_cast<float>(1.0L - (L(zz) + xx));
    m[9] = static_cast<float>(L(yz) - wx);
    m[2] = static_cast<float>(L(xz) - wy);
    m[6] = static_cast<float>(L(wx) + yz);
    m[10] = static_cast<float>(1.0L - (L(yy) + xx));
}

void InitFEngObjectSorting(FEPackage *, void *buffer, int32_t size)
{
    FESortGlobals *g = bridge::FEngSortGlobals();
    g->mpBuffer = buffer;
    g->mu32Max = static_cast<uint32_t>(size) >> 3;
}

void CloseFEngObjectSorting(FEPackage *)
{
    FESortGlobals *g = bridge::FEngSortGlobals();
    g->mi32Most = g->mi32Most > g->mi32Num ? g->mi32Most : g->mi32Num;
    g->mpBuffer = nullptr;
    g->mu32Max = 0;
    g->mi32Num = 0;
}

void FEngine::ObjectSorter::SortObjects()
{
    SortEntry *src = mEntries, *dst = mScratch;
    for (int pass = 0; pass < 4; ++pass) {
        uint32_t count[256] = {}, offset[256];
        for (uint32_t i = 0; i < mu32Count; ++i) ++count[(src[i].mu32Key >> (pass * 8)) & 0xFF];
        offset[0] = 0;
        for (int b = 0; b < 255; ++b) offset[b + 1] = offset[b] + count[b];
        for (uint32_t i = 0; i < mu32Count; ++i) dst[offset[(src[i].mu32Key >> (pass * 8)) & 0xFF]++] = src[i];
        SortEntry *t = src;
        src = dst;
        dst = t;
    }
}

void FEngine::RenderObject(FEObject *obj, const FEMatrix4 *m, uint16_t ctx)
{
    const float *d = static_cast<const float *>(obj->mpData);
    if (!reinterpret_cast<const int32_t *>(d)[3]) return;   // color alpha (data +0xC) zero: invisible
    const FEVector3 v{static_cast<float>(L(d[4]) + d[7]), static_cast<float>(L(d[5]) + d[8]), static_cast<float>(L(d[6]) + d[9])};   // pivot + position
    FEVector3 r{0.0f, 0.0f, 0.0f};
    FEMultMatrix(&r, m->m, &v);
    obj->mu16RenderContext = ctx;
    if (!(r.z <= 0.0f)) {   // in front of the camera (NaN counts)
        uint32_t key;
        std::memcpy(&key, &r.z, 4);   // PC: float bits (GC converts to an integer)
        mSorter.mEntries[mSorter.mu32Count] = {obj, key};
        ++mSorter.mu32Count;
    }
}

void FEngine::RenderGroup(FEGroup *group, const FEMatrix4 *parent, const FEMatrix4 *view, uint16_t ctx)
{
    const float *d = static_cast<const float *>(group->obj.mpData);
    if (!reinterpret_cast<const int32_t *>(d)[3]) return;
    if (!_unknown000 && static_cast<int32_t>(group->obj.mu32Flags) < 0) return;   // hidden while FEng is stopped
    FEMatrix4 local, world;
    FEQuatGetMatrix(reinterpret_cast<const FEQuaternion *>(d + 10), &local);   // rotation track (data +0x28)
    // pivot negated with fmul -1 (a NaN pivot keeps its sign; compilers may emit fchs instead)
    const FEVector3 negPivot{static_cast<float>(L(d[4]) * -1.0L), static_cast<float>(L(d[5]) * -1.0L), static_cast<float>(L(d[6]) * -1.0L)};
    FEVector3 rotated{0.0f, 0.0f, 0.0f};
    FEMultMatrix(&rotated, local.m, &negPivot);
    local.m[12] = static_cast<float>((L(rotated.x) + d[7]) + d[4]);
    local.m[13] = static_cast<float>((L(d[8]) + d[5]) + rotated.y);
    local.m[14] = static_cast<float>((L(d[9]) + d[6]) + rotated.z);
    FEMultMatrix(&world, &local, parent);
    FEMultMatrix(&local, &world, view);   // world * view (objects)
    const uint16_t childCtx = ++mu16RenderContextCount;
    group->obj.mu16RenderContext = ctx;
    bridge::FEGameInterface_GenerateRenderContext(mpUIData, childCtx, &group->obj);
    for (auto *o = reinterpret_cast<FEObject *>(group->mChildren.head); o; o = reinterpret_cast<FEObject *>(o->node.next)) {
        if (o->mu32Type == 5)
            RenderGroup(reinterpret_cast<FEGroup *>(o), &world, view, childCtx);
        else
            RenderObject(o, &local, childCtx);
    }
}

void FEngine::Render()
{
    FEMatrix4 identity{}, view{};
    identity.m[0] = identity.m[5] = identity.m[10] = identity.m[15] = 1.0f;
    bridge::FEGameInterface_GetViewTransformation(mpUIData, &view);
    // re-sort the active packages by priority (stable for equal priorities: inserted after them)
    FEPackage *pkgs[256];   // PC: 32-entry stack array, unchecked
    int32_t n = 0;
    for (auto *p = mpPackages; p; p = reinterpret_cast<FEPackage *>(p->node.next)) pkgs[n++] = p;
    for (int32_t i = 0; i < n; ++i) ActiveList().RemNode(&pkgs[i]->node);
    for (int32_t i = 0; i < n; ++i) AddPackageByPriority(pkgs[i]);
    alignas(8) static uint8_t sortBuffer[0x1000];   // PC: on the stack; only its address is handed out
    for (auto *p = mpPackages; p; p = reinterpret_cast<FEPackage *>(p->node.next)) {
        mu16RenderContextCount = 0;
        InitFEngObjectSorting(p, sortBuffer, 0x1000);
        bridge::FEGameInterface_BeginPackageRendering(mpUIData, p, 1);
        mSorter.mu32Count = 0;
        for (auto *o = reinterpret_cast<FEObject *>(p->mObjects.head); o; o = reinterpret_cast<FEObject *>(o->node.next)) {
            if (o->mu32Type == 5)
                RenderGroup(reinterpret_cast<FEGroup *>(o), &identity, &view, 0);
            else
                RenderObject(o, &view, 0);
        }
        mSorter.SortObjects();
        bridge::FEGameInterface_RenderObjectList(mpUIData, mSorter.mEntries, mSorter.mu32Count);
        bridge::FEGameInterface_EndPackageRendering(mpUIData, p);
        CloseFEngObjectSorting(p);
    }
    _unknown5250 = _unknown000;
}

FEngine *FEngine::Construct(void *mem)
{
    auto *e = static_cast<FEngine *>(mem);
    e->mMouse.mi32Wheel = e->mMouse.mi32Y = e->mMouse.mi32X = 0;
    e->mMouse.mu16Current = e->mMouse.mu16Previous = 0;
    e->mMouse.mHoldTime[2] = e->mMouse.mHoldTime[1] = e->mMouse.mHoldTime[0] = 0;
    e->mMouse._unknown1C = 0;
    e->mMouse.mbMoved = 0;
    void *const listVTable = bridge::FEPackageList74VTable();   // PC 0x79B878 (FEPackageList)
    e->mpPackages = e->mpPackagesTail = nullptr;
    e->mu32NumPackages = 0;
    e->mpActiveListVTable = listVTable;
    e->mIdlePackages = FEMinList{listVTable, 0, nullptr, nullptr};
    e->mLibraries = FEMinList{listVTable, 0, nullptr, nullptr};
    e->mSorter.mu32Count = 0;
    e->mMessages = FEMinList{bridge::FEMinListVTable(), 0, nullptr, nullptr};
    e->mPackageCommands = FEMinList{listVTable, 0, nullptr, nullptr};
    e->mTypeLib.mTypes = FEMinList{listVTable, 0, nullptr, nullptr};
    e->mTypeLib._unknown10 = 0;
    e->_unknown000 = 1;
    e->mbFlag01 = 1;
    e->mbFlag03 = 1;
    e->mbFlag04 = 1;
    e->_unknown5250 = 0;
    e->mu32NumJoyPads = 0;
    e->mpJoyPads = nullptr;
    e->_unknown030 = 0;
    e->_unknown02C = 0;
    e->mu32ButtonMapArg = 0;
    e->mbMouseEnabled = 0;
    e->mbForwardQueuedMessages = 0;
    std::memset(e->mPressedButton, 0, sizeof e->mPressedButton);
    e->mu32NumPackageMarkers = 0;
    std::memset(e->macPackageMarkers, 0, sizeof e->macPackageMarkers);
    e->mi32LastButtonIndex = 0;
    std::memset(e->mLastButtons, 0, sizeof e->mLastButtons);
    e->mTypeLib.Startup();
    return e;
}
