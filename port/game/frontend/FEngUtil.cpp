#include "FEngUtil.h"
#include <cstring>
#include "bridge/Bridge.h"
#include "bWare/bString.h"
#include "FEHash.h"
#include "FEObject.h"
#include "FEScript.h"

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
