#include <initializer_list>
#include <cstring>
#include "FEPackage.h"
#include "FENode.h"
#include "FEObject.h"
#include "FEScript.h"
#include "FEMessageResponse.h"
#include "bridge/Bridge.h"
#include "FEngine.h"

namespace {
enum : uint32_t {
    kFlagTrack0Active = 0x800000,    // first track still animating
    kFlagTracksRunning = 0x1000000,  // some track with data index < 16 is not done (flag bit 7 clear)
    kFlagAnimating = 0x2000000,
};
}

// PC 541DB0. Tracks are only evaluated while the object's alpha (data dword 3) is non-zero, except track 0 when it
// targets data index 0 (the color itself). As in the original (and GC), the loop starts again at track 0, and
// the clears keep the bits set in uHoldDirtyFlags.
void FEPackage::UpdateObjectTracks(FEObject *obj, FEScript *script) const
{
    auto *data = static_cast<uint8_t *>(obj->mpData);
    int32_t time = static_cast<int32_t>(script->mi32Time);
    FEKeyTrack *tracks = script->mpTracks;
    auto alpha = [&] { return *reinterpret_cast<const uint32_t *>(data + 0xC); };
    uint32_t &flags = obj->mu32Flags;
    if (!mbFastInterp) {
        if (tracks && tracks[0].mi8DataIndex == 0) FEKeyInterp(&tracks[0], time, data);
        uint8_t n = static_cast<uint8_t>(script->mu32NumTracks);
        if (alpha() != 0 && n != 0)
            for (uint8_t i = 0; i < n; ++i) FEKeyInterp(&tracks[i], time, data + tracks[i].mi8DataIndex * 4);
    } else {
        if (!tracks || tracks[0].mi8DataIndex != 0) {
            flags &= bridge::HoldDirtyFlags() | ~kFlagTrack0Active;
        } else if (tracks[0].mu8Flags & 0x80) {
            flags &= bridge::HoldDirtyFlags() | ~kFlagTrack0Active;
            FEKeyInterpFast(&tracks[0], time, data);
        } else {
            flags |= kFlagTrack0Active;
            FEKeyInterpFast(&tracks[0], time, data);
        }
        uint8_t done = 0x80;
        uint8_t n = static_cast<uint8_t>(script->mu32NumTracks);
        bool running = false;
        if (alpha() != 0 && n != 0) {
            for (uint8_t i = 0; i < n; ++i) {
                if (tracks[i].mi8DataIndex < 0x10) done &= tracks[i].mu8Flags;
                FEKeyInterpFast(&tracks[i], time, data + tracks[i].mi8DataIndex * 4);
            }
            running = done == 0;
        }
        if (running) flags |= kFlagTracksRunning;
        else flags &= bridge::HoldDirtyFlags() | ~kFlagTracksRunning;
    }
    if (flags & 0x1C00000) flags |= kFlagAnimating;
}

// PC 54CE60
void FEPackage::UpdateObject(FEObject *obj, int32_t delta)
{
    const uint32_t hold = bridge::HoldDirtyFlags();
    uint32_t &flags = obj->mu32Flags;
    if (flags & 0x1C00000) flags |= kFlagAnimating;
    else flags &= hold | ~kFlagAnimating;

    FEScript *sc = obj->mpCurrentScript;
    const int32_t oldTime = sc->mi32Time;
    const int32_t len = sc->mi32Length;
    sc->mi32Time = static_cast<int32_t>(static_cast<uint32_t>(mi32ScriptDelta) + static_cast<uint32_t>(oldTime));
    if (sc->mi32Time < 0) sc->mi32Time = 0;
    const int32_t now = sc->mi32Time;
    auto issue = [&](FEScript *s, int32_t from, int32_t to) {
        IssueScriptMessages(mpEngine, obj, s, from, to);
    };

    if (now >= len) {                                   // reached the end of the script
        if (mbFastInterp) {
            if (sc->mpChainedScript) {
                UpdateObjectTracks(obj, sc);
                int32_t rest = sc->mi32Time - len;
                sc->mi32Time = 0;
                if (sc->mEvents.mu32Count) issue(sc, oldTime, len);
                sc = sc->mpChainedScript;
                obj->mpCurrentScript = sc;                // FEObject::SetCurrentScript (inlined)
                if (sc) obj->SetupMoveToTracks();
                sc->mi32Time = rest;                      // like the original: crash if the chain ends in null
                if (sc->mEvents.mu32Count) issue(sc, 0, rest);
            } else {
                switch (sc->mu32Flags & 3) {
                case 0:
                    if (sc->mEvents.mu32Count) issue(sc, oldTime, len);
                    sc->mi32Time = len + 1;
                    break;
                case 1:
                    if (len <= 0) { sc->mi32Time = 0; break; }
                    if (sc->mEvents.mu32Count) issue(sc, oldTime, len);
                    sc->mi32Time = sc->mi32Time % sc->mi32Length;   // re-read, like the original
                    if (sc->mEvents.mu32Count) issue(sc, 0, sc->mi32Time);
                    obj->SetupMoveToTracks();
                    break;
                case 2:
                    if (len <= 0) { sc->mi32Time = 0; break; }
                    sc->mi32Time = now % (len * 2);
                    break;
                default: break;
                }
            }
        }
    } else if (mbFastInterp && sc->mEvents.mu32Count) {   // still inside: fire the events passed this frame
        switch (sc->mu32Flags & 3) {
        case 0: issue(sc, oldTime, now); break;
        case 1:
            if (now < oldTime) { issue(sc, oldTime, len); issue(sc, 0, sc->mi32Time); }
            else issue(sc, oldTime, now);
            break;
        case 2:
            if (oldTime < len) {
                if (now < len) issue(sc, oldTime, now);
                else { issue(sc, oldTime, len); issue(sc, len, sc->mi32Time - len); }   // unreachable (now < len)
            } else {
                if (now >= len) issue(sc, oldTime - len, now - len);                  // unreachable
                else { issue(sc, oldTime - len, 0); issue(sc, 0, sc->mi32Time); }
            }
            break;
        default: break;
        }
    }

    // Finished, non-forced scripts are not re-evaluated (fast mode).
    bool finished = mbFastInterp && oldTime == sc->mi32Time && oldTime == sc->mi32Length + 1 &&
                    !(flags & FEOBJ_DIRTY);
    if (!finished) UpdateObjectTracks(obj, sc);

    switch (obj->mu32Type) {
    case 4: bridge::FEListBox_Update(obj, static_cast<float>(delta)); break;
    case 5: UpdateGroup(reinterpret_cast<FEGroup *>(obj), delta); break;
    case 6: bridge::FECodeListBox_Update(obj, static_cast<float>(delta)); break;
    case 7:                                             // FEMovie: playback time right after the FEObject part
        if (mbFastInterp) {
            auto *t = reinterpret_cast<int32_t *>(reinterpret_cast<uint8_t *>(obj) + sizeof(FEObject));
            *t = static_cast<int32_t>(static_cast<uint32_t>(*t) + static_cast<uint32_t>(delta));
        }
        break;
    default: break;
    }
    if (mbFastInterp == 1 && oldTime == sc->mi32Time && oldTime == sc->mi32Length + 1 &&
        !(flags & FEOBJ_DIRTY))
        flags &= hold | ~(kFlagTrack0Active | kFlagTracksRunning);
    flags &= hold | ~FEOBJ_DIRTY;
    if (flags & 0x1C00000) flags |= kFlagAnimating;
}

// PC 504910: children inherit the group's 0x3C00000 flags after their update.
void FEPackage::UpdateGroup(FEGroup *group, int32_t delta)
{
    for (FEMinNode *n = group->mChildren.head; n; n = n->next) {
        auto *child = reinterpret_cast<FEObject *>(n);
        UpdateObject(child, delta);
        child->mu32Flags |= group->obj.mu32Flags & 0x3C00000;
    }
}

// PC 54D190
void FEPackage::Update(void *engine, int32_t delta)
{
    mpEngine = engine;
    mi32ScriptDelta = delta;
    for (FEMinNode *n = mObjects.head; n; n = n->next) UpdateObject(reinterpret_cast<FEObject *>(n), delta);
}

// PC 534C70. Event times are compared unsigned (like the original). Targets: 0 the object (message 0x1B3909AA:
// make GUID 0's object the current button), -1 the game (UI callback), -4/-5/-6 passed through as the target,
// anything else an object GUID (the queue message is only sent when obj is set).
void FEPackage::IssueScriptMessages(void *engine, FEObject *obj, FEScript *script, int32_t from, int32_t to)
{
    constexpr uint32_t kSetButton = 0x1B3909AA;
    const int32_t count = static_cast<int32_t>(script->mEvents.mu32Count);
    const FEScriptEvent *ev = script->mEvents.mpEvents;
    if (to < from) return;
    if (to == script->mi32Length) ++to;
    if (count <= 0) return;
    int32_t i = 0;
    while (ev[i].mu32Time < static_cast<uint32_t>(from))
        if (++i >= count) return;
    for (; i < count && ev[i].mu32Time < static_cast<uint32_t>(to); ++i) {
        const FEScriptEvent &e = ev[i];
        auto queue = [&](void *target) { static_cast<FEngine *>(engine)->QueueMessage(e.mu32MsgId, obj, this, static_cast<FEObject *>(target), 0); };
        auto setButton = [&] { SetCurrentButton(engine, FindObjectByGUID(e.mu32Target), true); };
        switch (e.mu32Target) {
        case 0xFFFFFFFBu: case 0xFFFFFFFAu: case 0xFFFFFFFCu:
            queue(reinterpret_cast<void *>(static_cast<uintptr_t>(e.mu32Target)));
            break;
        case 0:
            if (e.mu32MsgId == kSetButton) setButton();
            else queue(nullptr);
            break;
        case 0xFFFFFFFFu:
            bridge::FEngine_SendMessageToGame(engine, e.mu32MsgId, obj, this);
            break;
        default: {
            void *found = FindObjectByGUID(e.mu32Target);
            if (e.mu32MsgId == kSetButton) setButton();   // looks the GUID up again, like the original
            else if (obj) queue(found);
            break;
        }
        }
    }
}

// PC 504880
bool FEPackage::ForAllObjects(FEObjectCallback *cb)
{
    for (FEMinNode *n = mObjects.head; n; n = n->next) {
        auto *o = reinterpret_cast<FEObject *>(n);
        if (!bridge::FEObjectCallback_Call(cb, o)) return false;
        if (o->mu32Type == 5 && !ForAllChildren(reinterpret_cast<FEGroup *>(o), cb)) return false;
    }
    return true;
}

// PC 504830
bool FEPackage::ForAllChildren(FEGroup *group, FEObjectCallback *cb)
{
    for (FEMinNode *n = group->mChildren.head; n; n = n->next) {
        auto *o = reinterpret_cast<FEObject *>(n);
        if (!bridge::FEObjectCallback_Call(cb, o)) return false;
        if (o->mu32Type == 5 && !ForAllChildren(reinterpret_cast<FEGroup *>(o), cb)) return false;
    }
    return true;
}

namespace {
// Same order and stop rule as ForAllObjects with the FEFindByGUID / FEFindByHash callbacks (first match wins).
template <typename Pred>
FEObject *FindIn(const FEMinList &list, Pred pred)
{
    for (FEMinNode *n = list.head; n; n = n->next) {
        auto *o = reinterpret_cast<FEObject *>(n);
        if (pred(o)) return o;
        if (o->mu32Type == 5)
            if (FEObject *r = FindIn(reinterpret_cast<FEGroup *>(o)->mChildren, pred)) return r;
    }
    return nullptr;
}
}  // namespace

FEObject *FEPackage::FindObjectByGUID(uint32_t guid)
{
    return FindIn(mObjects, [&](const FEObject *o) { return o->mu32GUID == guid; });
}

FEObject *FEPackage::FindObjectByHash(uint32_t nameHash)
{
    return FindIn(mObjects, [&](const FEObject *o) { return o->mu32NameHash == nameHash; });
}

// PC 504960
FELibraryRef *FEPackage::FindLibraryReference(uint32_t hash) const
{
    for (uint32_t i = 0; i < mu32NumLibraryRefs; ++i)
        if (mpLibraryRefs[i].mu32Hash == hash) return &mpLibraryRefs[i];
    return nullptr;
}

// PC 52B3A0
void FEPackage::SetCurrentButton(void *engine, FEObject *button, bool notify)
{
    constexpr uint32_t kUnhighlight = 0x55D1E635, kHighlight = 0xABC08912;
    auto *special = reinterpret_cast<FEObject *>(static_cast<uintptr_t>(0xFFFFFFFBu));
    if (notify) {
        if (mpCurrentButton) {
            static_cast<FEngine *>(engine)->QueueMessage(kUnhighlight, nullptr, this, mpCurrentButton, 0);
            static_cast<FEngine *>(engine)->QueueMessage(kUnhighlight, mpCurrentButton, this, special, 0);
        }
        if (button) {
            static_cast<FEngine *>(engine)->QueueMessage(kHighlight, nullptr, this, button, 0);
            static_cast<FEngine *>(engine)->QueueMessage(kHighlight, button, this, special, 0);
        }
    }
    mpCurrentButton = button;
}

FEMessageResponse *FEPackage::FindResponse(uint32_t msg) const
{
    for (FEMinNode *n = mResponses.head; n; n = n->next)
        if (reinterpret_cast<FEMessageResponse *>(n)->mu32MsgId == msg) return reinterpret_cast<FEMessageResponse *>(n);
    return nullptr;
}

FEMessageTargets *FEPackage::GetMessageTargets(uint32_t msg) const
{
    for (uint32_t i = 0; i < mu32NumMessageTargets; ++i)
        if (mpMessageTargets[i].mu32MsgId == msg) return &mpMessageTargets[i];
    return nullptr;
}

void FEPackage::Construct()
{
    node.vtable = bridge::FEPackageVTable();
    node.next = node.prev = reinterpret_cast<FEMinNode *>(FEMINNODE_UNLINKED);
    mpcName = nullptr;
    mu32NameHash = 0;
    for (FEMinList *l : {&mObjects, &mResponses}) { l->vtable = bridge::FEMinListVTable(); l->count = 0; l->head = l->tail = nullptr; }
    mList74.vtable = bridge::FEPackageList74VTable();
    mList74.count = 0; mList74.head = mList74.tail = nullptr;
    mppButtons = nullptr;
    mu32NumButtons = 0;
    mList9C.vtable = bridge::FEMinListVTable();
    mList9C.count = 0; mList9C.head = mList9C.tail = nullptr;
    mbFastInterp = 0;
    _unknown15[0] = _unknown15[1] = _unknown15[2] = 0;
    mi32Priority = 0;
    mpcFilename = nullptr;
    mpParent = nullptr;
    mpPackageData = nullptr;
    mu32NumResources = 0;
    mpResources = nullptr;
    mu32NumMessageTargets = 0;
    mpMessageTargets = nullptr;
    mpCurrentButton = nullptr;
    mp98 = nullptr;
    mu32NumLibraryRefs = 0;
    mpLibraryRefs = nullptr;
    mu32ControlMask = mu32SavedControlMask = mu32SavedInputControlMask = mu32InputControlMask = 0xFF;
    mbFlagsAC[0] = mbFlagsAC[1] = mbFlagsAC[2] = mbFlagsAC[3] = 1;
}

// PC frees the filename unconditionally, then the arrays; mList9C nodes free their +0x10 buffer and are deleted
// through their vtable; lists are then reset to the plain FEMinList vtable and purged (PC 511670).
void FEPackage::Destruct()
{
    node.vtable = bridge::FEPackageVTable();
    bridge::FreeArray(mpcFilename);
    if (mpResources) bridge::FreeArray(mpResources);
    if (mpMessageTargets) {
        uint32_t *cookie = reinterpret_cast<uint32_t *>(mpMessageTargets) - 1;   // new[] element count
        for (uint32_t i = *cookie; i-- > 0;) bridge::FreeArray(mpMessageTargets[i].mpObjects);   // ~FEMessageTargets (4FF200)
        bridge::FreeArray(cookie);
    }
    if (mp98) bridge::FreeArray(mp98);
    while (FEMinNode *n = mList9C.RemHead()) {
        bridge::FreeArray(reinterpret_cast<FEPackageBufferNode *>(n)->mpBuffer);
        bridge::VirtualScalarDelete(n);
    }
    if (mpLibraryRefs) bridge::FreeArray(mpLibraryRefs);
    mList9C.vtable = bridge::FEMinListVTable();
    mList9C.Purge();
    bridge::FreeArray(mppButtons);
    for (FEMinList *l : {&mList74, &mResponses, &mObjects}) {
        l->vtable = bridge::FEMinListVTable();
        l->Purge();
    }
    reinterpret_cast<FENode *>(this)->Destruct();
}

FEPackage *FEPackage::ScalarDeletingDestructor(uint32_t flags)
{
    Destruct();
    if (flags & 1) bridge::FreeArray(this);
    return this;
}

void FEPackage::SetFilename(const char *name)
{
    bridge::FreeArray(mpcFilename);
    mpcFilename = nullptr;
    if (!name) return;
    const size_t n = std::strlen(name) + 1;
    mpcFilename = static_cast<char *>(bridge::Alloc(static_cast<uint32_t>(n)));
    std::memcpy(mpcFilename, name, n);   // PC: no null check
}

bool FEPackage::Startup(void *gameInterface)
{
    const bool ok = bridge::FEGameInterface_LoadResources(gameInterface, this, mu32NumResources, mpResources);
    ConnectObjectResources();
    return ok;
}

void FEPackage::ConnectObjectResources()
{
    struct { void *vtable; FEPackage *pkg; void *resources; } cb;   // game-side ResourceConnector (PC 0x79AB04)
    cb.vtable = bridge::ResourceConnectorVTable();
    cb.pkg = this;
    cb.resources = &mpResources;
    ForAllObjects(reinterpret_cast<FEObjectCallback *>(&cb));
}

void FEPackage::SetNumLibraryRefs(uint32_t n)
{
    if (n == 0) {
        if (mpLibraryRefs) bridge::FreeArray(mpLibraryRefs);
        mpLibraryRefs = nullptr;
        return;
    }
    auto *refs = static_cast<FELibraryRef *>(bridge::Alloc(n * 0xC));
    if (refs)
        for (uint32_t i = 0; i < n; ++i) { refs[i].mu32Hash = 0; refs[i]._unknown04[0] = 0xFFFFFFFFu; refs[i]._unknown04[1] = 0; }
    const uint32_t keep = n < mu32NumLibraryRefs ? n : mu32NumLibraryRefs;
    if (keep) std::memcpy(refs, mpLibraryRefs, keep * 0xC);
    if (mpLibraryRefs) bridge::FreeArray(mpLibraryRefs);
    mpLibraryRefs = refs;
    mu32NumLibraryRefs = n;
}

void FEMessageTargets::SetCapacity(uint32_t n)
{
    if (n == 0) {
        bridge::FreeArray(mpObjects);
        mu32Capacity = 0;
        mpObjects = nullptr;
        mu32Count = 0;
    } else if (n != mu32Capacity) {
        auto *objs = static_cast<FEObject **>(bridge::Alloc(n * 4));
        const uint32_t keep = n < mu32Capacity ? n : mu32Capacity;
        if (keep) std::memcpy(objs, mpObjects, keep * sizeof(FEObject *));
        bridge::FreeArray(mpObjects);
        mpObjects = objs;
    }
    mu32Capacity = n;
}
