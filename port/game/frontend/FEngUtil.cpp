#include "FEngUtil.h"
#include <cstring>
#include "bridge/Bridge.h"
#include "bWare/bString.h"
#include "FEHash.h"
#include "FEObject.h"
#include "FEScript.h"
#include "bWare/bMath.h"
#include "FEMath.h"
#include "FEString.h"
#include "FEPackage.h"
#include "FEPackageManager.h"
#include "FEngine.h"
#include "cFEng.h"

uint32_t FEngGetTextureHash(const FEObject *obj) { return obj ? obj->_unknown24 : 0; }

void FEngSetTextureHash(FEObject *obj, uint32_t hash)
{
    if (obj && obj->_unknown24 != hash) {
        obj->_unknown24 = hash;
        obj->mu32Flags |= kFEObjectDirty;
    }
}

void FEngSetMovieName(FEObject *movie, const char *name)
{
    if (!movie) return;
    bStrNCpy(reinterpret_cast<char *>(static_cast<uintptr_t>(movie->_unknown24)), name, 0x100);   // FEMovie: +0x24 name buffer
    movie->mu32Flags |= kFEObjectDirty;
}

int32_t FEngIsVisible(const FEObject *obj) { return obj && !(obj->mu32Flags & 1) ? 1 : 0; }

using L = long double;

namespace {
template <typename F>
void ForGroupChildren(FEObject *obj, F f)
{
    if (obj->mu32Type != 5) return;
    const FEGroup *g = reinterpret_cast<const FEGroup *>(obj);
    auto *c = reinterpret_cast<FEObject *>(g->mChildren.head);
    for (int32_t n = static_cast<int32_t>(g->mChildren.count); n > 0; --n) {
        f(c);
        c = reinterpret_cast<FEObject *>(c->node.next);
    }
}
}  // namespace

void FEngSetInvisible(FEObject *obj)
{
    if (!obj) return;
    if (bridge::UseOldFEngDirtyFlag() || !(obj->mu32Flags & 1)) obj->mu32Flags |= kFEObjectDirty | 1;
    ForGroupChildren(obj, FEngSetInvisible);
}

void FEngSetVisible(FEObject *obj)
{
    if (!obj) return;
    if (bridge::UseOldFEngDirtyFlag() || (obj->mu32Flags & 1)) obj->mu32Flags = (obj->mu32Flags & ~1u) | kFEObjectDirty;
    ForGroupChildren(obj, FEngSetVisible);
}

bool FEngIsScriptFinished(const FEObject *obj, const char *name)
{
    if (!obj) return false;
    for (auto *s = reinterpret_cast<const FEScript *>(obj->mScripts.head); s; s = reinterpret_cast<const FEScript *>(s->node.next))
        if (s->mu32Id == FEHashUpper(name)) return s->mi32Time >= s->mi32Length;
    return false;
}

void FEngSetSize(FEObject *obj, float w, float h)
{
    if (!obj) return;
    float *d = static_cast<float *>(obj->mpData);
    d[14] = w;   // +0x38 (PC writes it twice for image / movie / CBV types: same result)
    d[15] = h;
    obj->mu32Flags |= kFEObjectDirty;
}

namespace {
void SetScriptImpl(FEObject *obj, FEScript *s, bool restart)
{
    if (s != obj->mpCurrentScript) {
        obj->SetCurrentScript(s);   // PC: +0x54 = s; SetupMoveToTracks (517720)
        obj->mpCurrentScript->mi32Time = 0;
    }
    if (restart) s->mi32Time = 0;
}
}  // namespace

void FEngSetScript(FEObject *obj, const char *name, bool restart)
{
    if (!obj) return;
    for (auto *s = reinterpret_cast<FEScript *>(obj->mScripts.head); s; s = reinterpret_cast<FEScript *>(s->node.next))
        if (s->mu32Id == FEHashUpper(name)) { SetScriptImpl(obj, s, restart); return; }
}

void FEngSetScript(FEObject *obj, uint32_t id, bool restart)
{
    if (!obj) return;
    for (auto *s = reinterpret_cast<FEScript *>(obj->mScripts.head); s; s = reinterpret_cast<FEScript *>(s->node.next))
        if (s->mu32Id == id) { SetScriptImpl(obj, s, restart); return; }
}

bool FEngIsScriptRunning(const FEObject *obj, const char *name)
{
    const uint32_t id = bridge::FEngHashStringUpper(name);   // PC 50B9C0: formats `name` (printf) then hashes
    if (!obj) return false;
    const FEScript *s = obj->mpCurrentScript;
    return s && s->mu32Id == id && s->mi32Time <= s->mi32Length;
}

void FEngSetRotationZ(FEObject *obj, float degrees)
{
    if (!obj) return;
    const float rad = static_cast<float>(static_cast<long double>(degrees) * 0.017453292f);   // PC 0x784244
    bMatrix4 m;
    bIdentity(&m);
    const long double c = bSinF(static_cast<float>(static_cast<long double>(rad) + 1.5707964f));   // PC 0x784254
    m.m[5] = static_cast<float>(c);
    m.m[0] = static_cast<float>(c);
    const long double s = bSinF(rad);
    m.m[1] = static_cast<float>(s);
    m.m[4] = static_cast<float>(-s);
    bVector4 q;
    bMatrixToQuaternion(&q, &m);
    const FEQuaternion fq{q.x, q.y, q.z, q.w};
    obj->SetRotation(fq, false);
}

namespace {
const float *ObjData(const FEObject *obj) { return static_cast<const float *>(obj->mpData); }
// size.x/size.y at data +0x38/+0x3C, position at +0x1C/+0x20
bool IsBox(uint32_t type) { return type == 1 || type == 7 || type == 9; }
}  // namespace

void FEngGetTopLeft(const FEObject *obj, float *x, float *y)
{
    if (!obj) return;
    const float *d = ObjData(obj);
    const uint32_t t = obj->mu32Type;
    if (IsBox(t)) {
        *x = static_cast<float>(L(d[7]) - L(d[14]) * 0.5f);
        *y = static_cast<float>(L(d[8]) - L(d[15]) * 0.5f);
    } else if (t == 2) {
        const auto *s = reinterpret_cast<const FEString *>(obj);
        void *font = bridge::FEngFindFont(obj->_unknown24);
        if (!font) return;
        const float w = static_cast<float>(bridge::FEngFont_GetTextWidth(font, s->mstrText.mpsString, 0) * d[14]);
        const float h = static_cast<float>(bridge::FEngFont_GetTextHeight(font, s->mstrText.mpsString, s->_unknown70) * d[15]);
        *x = static_cast<float>(bridge::FEngFont_CalculateXOffset(font, s->_unknown6C, w) + d[7]);
        *y = static_cast<float>(bridge::FEngFont_CalculateYOffset(font, s->_unknown6C, h) + d[8]);
    } else if (t != 5) {
        *x = d[7];
        *y = d[8];
    }
}

void FEngGetBottomRight(const FEObject *obj, float *x, float *y)
{
    if (!obj) return;
    const float *d = ObjData(obj);
    const uint32_t t = obj->mu32Type;
    if (IsBox(t)) {
        *x = static_cast<float>(L(d[14]) * 0.5f + d[7]);
        *y = static_cast<float>(L(d[15]) * 0.5f + d[8]);
    } else if (t == 2) {
        const auto *s = reinterpret_cast<const FEString *>(obj);
        void *font = bridge::FEngFindFont(obj->_unknown24);
        if (!font) return;
        uint16_t buffer[0x400];
        const uint16_t *text = s->mstrText.mpsString;
        if (!(obj->mu32Flags & 2) && bridge::FEngGetLocalizedString(buffer, 0x800, s->mu32LabelHash)) text = buffer;
        const float w = static_cast<float>(bridge::FEngFont_GetTextWidth(font, text, 0) * d[14]);
        const float h = static_cast<float>(bridge::FEngFont_GetTextHeight(font, text, s->_unknown70) * d[15]);
        *x = static_cast<float>((L(d[7]) - bridge::FEngFont_CalculateXOffset(font, s->_unknown6C, w)) + w);
        *y = static_cast<float>((L(d[8]) - bridge::FEngFont_CalculateYOffset(font, s->_unknown6C, h)) + h);
    } else {
        *x = d[7];
        *y = d[8];
    }
}

namespace {
void CommitPosition(FEObject *obj, const FEVector3 &v)
{
    if (static_cast<int32_t>(obj->mu32Type) >= 0x100) return;
    const auto &pos = *reinterpret_cast<const FEVector3 *>(static_cast<const uint8_t *>(obj->mpData) + 0x1C);
    if (!FEVector3Close(v, pos)) obj->mu32Flags |= kFEObjectDirty;
    obj->SetTrackValue(2, v, false);   // PC 5178D0 (track 2: position)
}
}  // namespace

void FEngSetTopLeft(FEObject *obj, float x, float y)
{
    if (!obj) return;
    const float *d = ObjData(obj);
    FEVector3 v{d[7], d[8], d[9]};
    const float sx = d[14], sy = d[15];
    const uint32_t t = obj->mu32Type;
    if (IsBox(t)) {
        v.x = static_cast<float>(L(sx) * 0.5f + x);
        v.y = static_cast<float>(L(sy) * 0.5f + y);
    } else if (t == 2) {
        const auto *s = reinterpret_cast<const FEString *>(obj);
        if (void *font = bridge::FEngFindFont(obj->_unknown24)) {
            const float w = static_cast<float>(bridge::FEngFont_GetTextWidth(font, s->mstrText.mpsString, 0) * sx);
            const float h = static_cast<float>(bridge::FEngFont_GetTextHeight(font, s->mstrText.mpsString, s->_unknown70) * sy);
            v.x = static_cast<float>(L(x) - bridge::FEngFont_CalculateXOffset(font, s->_unknown6C, w));
            v.y = static_cast<float>(L(y) - bridge::FEngFont_CalculateYOffset(font, s->_unknown6C, h));
        }
    } else {
        v.x = x;
        v.y = y;
    }
    CommitPosition(obj, v);
}

void FEngSetBottomRight(FEObject *obj, float x, float y)
{
    if (!obj) return;
    const float *d = ObjData(obj);
    FEVector3 v{x, y, d[9]};
    if (IsBox(obj->mu32Type)) {
        v.x = static_cast<float>(L(x) - L(d[14]) * 0.5f);
        v.y = static_cast<float>(L(y) - L(d[15]) * 0.5f);
    }
    CommitPosition(obj, v);
}

namespace {
// scaled text extent of a string object (PC: width, then height, each rounded to float)
void TextExtent(void *font, const FEObject *obj, float *w, float *h)
{
    const auto *s = reinterpret_cast<const FEString *>(obj);
    const float *d = ObjData(obj);
    *w = static_cast<float>(bridge::FEngFont_GetTextWidth(font, s->mstrText.mpsString, 0) * d[14]);
    *h = static_cast<float>(bridge::FEngFont_GetTextHeight(font, s->mstrText.mpsString, s->_unknown70) * d[15]);
}
uint32_t Justify(const FEObject *obj) { return reinterpret_cast<const FEString *>(obj)->_unknown6C; }
}  // namespace

void FEngSetCenter(FEObject *obj, float x, float y)
{
    if (!obj) return;
    const float *d = ObjData(obj);
    FEVector3 v{d[7], d[8], d[9]};
    if (obj->mu32Type == 2) {
        if (void *font = bridge::FEngFindFont(obj->_unknown24)) {
            float w, h;
            TextExtent(font, obj, &w, &h);
            v.x = static_cast<float>(L(x) - (bridge::FEngFont_CalculateXOffset(font, Justify(obj), w) + L(w) * 0.5f));
            v.y = static_cast<float>(L(y) - (bridge::FEngFont_CalculateYOffset(font, Justify(obj), h) + L(h) * 0.5f));
        }
    } else {
        v.x = x;
        v.y = y;
    }
    CommitPosition(obj, v);
}

void FEngGetCenter(const FEObject *obj, float *x, float *y)
{
    if (!obj) return;
    const float *d = ObjData(obj);
    if (obj->mu32Type != 2) {
        *x = d[7];
        *y = d[8];
        return;
    }
    void *font = bridge::FEngFindFont(obj->_unknown24);
    if (!font) return;
    float w, h;
    TextExtent(font, obj, &w, &h);
    *x = static_cast<float>((bridge::FEngFont_CalculateXOffset(font, Justify(obj), w) + L(w) * 0.5f) + d[7]);
    *y = static_cast<float>((bridge::FEngFont_CalculateYOffset(font, Justify(obj), h) + L(h) * 0.5f) + d[8]);
}

void FEngGetSize(const FEObject *obj, float *w, float *h)
{
    if (!obj) return;
    const float *d = ObjData(obj);
    const uint32_t t = obj->mu32Type;
    if (IsBox(t)) {
        *w = d[14];
        *h = d[15];
    } else if (t == 2) {
        void *font = bridge::FEngFindFont(obj->_unknown24);
        if (!font) return;
        const auto *s = reinterpret_cast<const FEString *>(obj);
        *w = static_cast<float>(bridge::FEngFont_GetTextWidth(font, s->mstrText.mpsString, 0) * d[14]);   // stored before measuring height
        *h = static_cast<float>(bridge::FEngFont_GetTextHeight(font, s->mstrText.mpsString, s->_unknown70) * d[15]);
    } else {
        *w = 0.0f;
        *h = 0.0f;
    }
}

void FEngSetUseIdleList(const char *pkg, bool use)
{
    if (!bridge::cFEngInstance()) return;
    FEPackageData *d = bridge::TheFEPackageManager()->FindFEPackageData(pkg);
    if (d && d->mpPackage) d->mpPackage->_unknown15[0] = use;
}

void FEngSetPermanent(const char *pkg, int32_t permanent) { bridge::TheFEPackageManager()->SetPermanent(pkg, permanent); }

FEObject *FEngGetCurrentButton(const char *pkg)
{
    if (!bridge::cFEngInstance()) return nullptr;
    FEPackageData *d = bridge::TheFEPackageManager()->FindFEPackageData(pkg);
    FEPackage *p = d ? d->mpPackage : nullptr;
    return p ? p->mpCurrentButton : nullptr;
}

void FEngSetButtonState(const char *pkg, uint32_t button, bool enable)
{
    if (bridge::cFEngInstance()) cFEng::SetButtonState(pkg, button, enable);
}

void FEngPostMessageToFEng(uint32_t msg, FEObject *obj, FEObject *target, FEPackage *pkg)
{
    cFEng *fe = bridge::cFEngInstance();
    if (fe && fe->mpEngine) fe->mpEngine->QueueMessage(msg, obj, pkg, target, 0);
}

void FEngQueueControllerMessageToPackage(uint32_t msg, const char *pkg, uint32_t arg)
{
    cFEng *fe = bridge::cFEngInstance();   // (debug prints "Send message[%x] to package [%s]" are no-ops)
    if (!fe || !fe->mpEngine) return;
    FEPackageData *d = bridge::TheFEPackageManager()->FindFEPackageData(pkg);
    if (d && d->mpPackage) fe->mpEngine->QueueMessage(msg, nullptr, d->mpPackage, reinterpret_cast<FEObject *>(intptr_t{-1}), arg);
}

void FEngQueueMessageToPackage(uint32_t msg, const char *pkg) { FEngQueueControllerMessageToPackage(msg, pkg, 0); }

void FEngSetPackageVisibility(const char *pkg, uint8_t visible)
{
    if (FEPackageData *d = bridge::TheFEPackageManager()->FindFEPackageData(pkg)) d->mVisible = visible;
}

FEObject *FEngFindObject(const char *pkg, uint32_t objHash)
{
    if (pkg)
        if (FEPackage *p = FEngFindPackage(pkg))
            if (FEObject *o = p->FindObjectByHash(objHash)) return o;
    cFEng *fe = bridge::cFEngInstance();
    return fe ? fe->mpEngine->FindObject(objHash, pkg) : nullptr;
}

FEObject *FEngFindImage(const char *pkg, uint32_t objHash)
{
    FEObject *o = FEngFindObject(pkg, objHash);
    return o && o->mu32Type == 1 ? o : nullptr;
}

void FEngSetPosition(const char *pkg, const char *obj, const FEVector3 *pos)
{
    FEObject *o = FEngFindObject(pkg, FEHashUpper(obj));
    if (!o || static_cast<int32_t>(o->mu32Type) >= 0x100) return;
    const auto &cur = *reinterpret_cast<const FEVector3 *>(static_cast<const uint8_t *>(o->mpData) + 0x1C);
    if (!FEVector3Close(*pos, cur)) o->mu32Flags |= kFEObjectDirty;
    o->SetTrackValue(2, *pos, false);
}

void FEngSetScript(const char *pkg, const char *obj, const char *script, bool restart)
{
    FEngSetScript(FEngFindObject(pkg, FEHashUpper(obj)), script, restart);
}

void FEngSetScript(const char *pkg, uint32_t objHash, const char *script, bool restart)
{
    FEngSetScript(FEngFindObject(pkg, objHash), script, restart);
}

void FEngSetScript(const char *pkg, uint32_t objHash, uint32_t scriptId, bool restart)
{
    FEngSetScript(FEngFindObject(pkg, objHash), scriptId, restart);
}
