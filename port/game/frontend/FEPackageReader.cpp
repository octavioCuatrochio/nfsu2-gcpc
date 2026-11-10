#include "FEPackageReader.h"
#include <cstring>
#include "FEHash.h"
#include "FEMessageResponse.h"
#include "FENode.h"
#include "FEObject.h"
#include "FEPackage.h"
#include "FEScript.h"
#include "FEString.h"
#include "FEMultiPool.h"
#include "FEngine.h"
#include "bridge/Bridge.h"

namespace {
constexpr uint32_t kTypeSizes = 0x53707954;    // "TypS"
constexpr uint32_t kLibraryRefs = 0x5262694C;  // "LibR"
constexpr uint32_t kResources = 0xCC736552;    // "Res\xCC"
constexpr uint32_t kResNames = 0x6D4E7352;     // "RsNm"
constexpr uint32_t kResRequests = 0x71527352;  // "RsRq"
constexpr uint32_t kPkgResponses = 0x52676B50; // "PkgR"

const uint32_t *NextChunk(const uint32_t *c) { return reinterpret_cast<const uint32_t *>(reinterpret_cast<const uint8_t *>(c) + c[1] + 8); }
}  // namespace

const uint32_t *FEPackageReader::FindChild(const uint32_t *parent, uint32_t id)
{
    const uint32_t *end = NextChunk(parent);
    const uint32_t *c = parent + 2;
    while (c[0] != id && c != end) c = NextChunk(c);
    return c[0] == id ? c : nullptr;
}

FEPackage *FEPackageReader::Load(const void *data, void *gameInterface, FEngine *engine, uint8_t flag09, uint8_t flag0A, uint8_t package16)
{
    mbFlag09 = flag09;
    mbPackage16 = package16;
    mbFlag0A = flag0A;
    mpGameInterface = gameInterface;
    mpPackage = nullptr;
    mpCurrentObject = nullptr;
    mpParentGroup = nullptr;
    mpLastParent = nullptr;
    mu32NumTypes = 0;
    mpTypes = nullptr;
    mu32Header1C = 0;
    mu32Header18 = 0;
    mu32ButtonIndex = 0;
    mu32NumButtons = 0;
    mpData = static_cast<const uint32_t *>(data);
    mpEngine = engine;
    FEPackage *result = nullptr;
    if (ReadHeaderChunk() && ReadTypeSizes() && ReadReferencedPackagesChunk() &&
        ReadLibraryRefsChunk() && ReadResourceChunk() && ReadObjectChunk() &&
        ReadPackageResponseChunk() && ReadMessageTargetListChunk()) {
        mpPackage->_unknown15[1] = mbPackage16;   // FEPackage +0x16
        if (mpPackage->Startup(mpGameInterface)) {
            result = mpPackage;
            mpPackage = nullptr;
        }
    }
    if (mpPackage) bridge::VirtualScalarDelete(mpPackage);
    mpPackage = nullptr;
    return result;
}

bool FEPackageReader::ReadHeaderChunk()
{
    const uint32_t *h = mpData;
    if (h[0] != 0xE76E4546u || h[2] != 0x64486B50u || h[4] < 0x20000u) return false;
    auto *pkg = static_cast<FEPackage *>(bridge::Alloc(sizeof(FEPackage)));
    if (pkg) pkg->Construct();
    mpPackage = pkg;   // PC: a failed allocation crashes on the next store
    pkg->mpCurrentButton = nullptr;
    mu32Header18 = h[6];
    mu32Header1C = h[7];
    const char *name = reinterpret_cast<const char *>(h + 10);
    reinterpret_cast<FENode *>(pkg)->SetName(name);
    pkg->SetFilename(name + h[8]);
    return true;
}

bool FEPackageReader::ReadTypeSizes()
{
    if (const uint32_t *c = FindChild(mpData, kTypeSizes)) {
        mu32NumTypes = c[1] >> 3;
        mpTypes = c + 2;
    }
    return true;
}

uint32_t FEPackageReader::GetTypeSize(uint32_t type) const
{
    for (uint32_t i = 0; i < mu32NumTypes; ++i)
        if (mpTypes[i * 2] == type) return mpTypes[i * 2 + 1];
    return 0;
}

bool FEPackageReader::ReadLibraryRefsChunk()
{
    const uint32_t *c = FindChild(mpData, kLibraryRefs);
    if (!c) return mpPackage->mList74.count == 0;
    if (c[1] % 0xC != 0) return false;
    const uint32_t n = c[1] / 0xC;
    mpPackage->SetNumLibraryRefs(n);
    std::memcpy(mpPackage->mpLibraryRefs, c + 2, n * 0xC);
    return true;
}

bool FEPackageReader::ReadResourceChunk()
{
    const uint32_t *c = FindChild(mpData, kResources);
    if (!c || c[2] != kResNames) return false;
    const uint32_t namesSize = c[3];
    const uint8_t *names = reinterpret_cast<const uint8_t *>(c + 4);
    const uint32_t *rq = reinterpret_cast<const uint32_t *>(names + namesSize);
    if (rq[0] != kResRequests) return false;
    const uint32_t n = rq[2];
    const uint32_t *src = rq + 3;
    mpPackage->mu32NumResources = n;
    if (n == 0) return true;
    mpPackage->mpResources = bridge::Alloc(n * 0x18);
    mpPackage->mp98 = bridge::Alloc(namesSize);
    auto *dst = static_cast<uint32_t *>(mpPackage->mpResources);
    std::memcpy(dst, src, n * 0x18);
    std::memcpy(mpPackage->mp98, names, namesSize);
    // name offsets -> pointers into the copied name block (32-bit field, like the original)
    for (uint32_t i = 0; i < n; ++i)
        dst[i * 6 + 1] += static_cast<uint32_t>(reinterpret_cast<uintptr_t>(mpPackage->mp98));
    return true;
}

bool FEPackageReader::ReadPackageResponseChunk()
{
    if (const uint32_t *c = FindChild(mpData, kPkgResponses)) ReadMessageResponseTags(reinterpret_cast<const uint8_t *>(c + 2), c[1], true);
    return true;
}

bool FEPackageReader::ReadMessageResponseTags(const uint8_t *tags, uint32_t size, bool package)
{
    const uint8_t *end = tags + size;
    FEMessageResponse *resp = nullptr;
    FEResponse *cur = nullptr;
    int32_t index = -1;
    for (const uint8_t *t = tags; t < end; t += *reinterpret_cast<const uint16_t *>(t + 2) + 4) {
        const uint32_t value = *reinterpret_cast<const uint32_t *>(t + 4);
        switch (*reinterpret_cast<const uint16_t *>(t)) {
        case 0x694D: {   // "Mi": message id
            resp = (!package && mbMergeResponses) ? mpCurrentObject->FindResponse(value) : nullptr;
            if (resp) {
                resp->PurgeResponses();
            } else {
                resp = FEMessageResponse::Construct(FEMessageResponse::PoolNew());
                resp->mu32MsgId = value;
                if (package) mpPackage->mResponses.AddTail(&resp->node);
                else mpCurrentObject->mResponses.AddTail(&resp->node);
            }
            index = -1;
            break;
        }
        case 0x434D: resp->SetCount(value); break;                         // "MC"
        case 0x6952: cur = &resp->mpResponses[++index]; cur->mu32Type = value; break;   // "Ri"
        case 0x7352: cur->SetParam(reinterpret_cast<const char *>(t + 4)); break;      // "Rs"
        case 0x7552: cur->mParam = value; break;                           // "Ru"
        case 0x7452: cur->mu32Extra = value; break;                        // "Rt"
        default: break;
        }
    }
    return true;
}

bool FEPackageReader::ReadReferencedPackagesChunk()
{
    const uint32_t *c = FindChild(mpData, 0x4C62694C);   // "LibL"
    if (!c) return true;
    const uint8_t *data = reinterpret_cast<const uint8_t *>(c + 2);
    const uint32_t n = c[2];
    FEMinList &refs = mpPackage->mList74;
    for (uint32_t i = 0; i < n; ++i) {
        auto *node = static_cast<FENode *>(bridge::Alloc(sizeof(FENode)));
        if (node) {
            node->mpcName = nullptr;
            node->mu32NameHash = 0;
            node->node.next = node->node.prev = reinterpret_cast<FEMinNode *>(FEMINNODE_UNLINKED);
            node->node.vtable = bridge::FENodeVTable();
        }
        node->SetName(reinterpret_cast<const char *>(data + c[3 + i]));   // PC: no null check
        refs.AddTail(&node->node);
    }
    for (auto *ref = reinterpret_cast<FENode *>(refs.head); ref; ref = reinterpret_cast<FENode *>(ref->node.next)) {
        if (FEPackage *lib = FEngine_FindLibraryPackage(mpEngine, ref->mu32NameHash)) {
            ++lib->mi32Priority;   // libraries: reference count
            continue;
        }
        void *buffer;
        uint8_t freeBuffer;   // uninitialized in the original unless the game sets it
        const void *pkgData = bridge::FEGameInterface_GetPackageData(mpGameInterface, ref->mpcName, &buffer, &freeBuffer);
        if (!pkgData) return false;
        FEPackage *lib = FEngine_LoadPackage(mpEngine, pkgData, 1);
        if (freeBuffer) bridge::FreeArray(buffer);
        if (!lib) return false;
        bridge::FEGameInterface_LibraryLoaded(mpGameInterface, lib);
        lib->mi32Priority = 1;
        mpEngine->mLibraries.AddTail(&lib->node);   // FEngine::AddToLibraryList (PC: inlined)
    }
    return true;
}

bool FEPackageReader::ReadObjectChunk()
{
    const uint32_t *root = FindChild(mpData, 0xCC6A624F);   // "Obj\xCC"
    if (!root) return false;
    const uint32_t *end = NextChunk(root);
    const uint32_t *c = root + 2;
    if (!c || !end) return false;   // (original checks both pointers)
    for (; c[0] != 0xEA624F46u; c = NextChunk(c)) {   // up to the first "FObj\xEA"
        if (c >= end) return true;
        if (c[0] == 0x6E747542u) {   // "Butn"
            const uint32_t n = c[2];
            mu32NumButtons = n;
            bridge::FreeArray(mpPackage->mppButtons);
            mpPackage->mppButtons = nullptr;
            if (n) mpPackage->mppButtons = static_cast<FEObject **>(bridge::Alloc(n * 4));
            mpPackage->mu32NumButtons = n;
        }
    }
    for (; c < end; c = NextChunk(c)) {
        const uint32_t *objEnd = NextChunk(c);
        mpCurrentObject = nullptr;
        mpParentGroup = nullptr;
        for (const uint32_t *t = c + 2; t < objEnd; t = NextChunk(t)) {
            const uint8_t *data = reinterpret_cast<const uint8_t *>(t + 2);
            if (t[0] == 0x446A624Fu) {   // "ObjD"
                if (!ReadObjectTags(data, t[1])) return false;
            } else if (t[0] == 0x5267734Du) {   // "MsgR"
                ReadMessageResponseTags(data, t[1], false);
            } else if (t[0] == 0x70726353u) {   // "Scrp"
                bridge::FEPackageReader_ReadScriptTags(this, data, t[1]);
            }
        }
        FEObject *obj = mpCurrentObject;
        if (!obj) continue;
        if (obj->mu32Type == 4) bridge::FEListBox_RecalculateCummulative(obj);
        else if (obj->mu32Type == 6) bridge::FECodeListBox_FillAllCells(obj);
        for (auto *s = reinterpret_cast<FEScript *>(obj->mScripts.head); s; s = reinterpret_cast<FEScript *>(s->node.next)) {
            const auto chained = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(s->mpChainedScript));   // id while loading
            if (chained) s->mpChainedScript = obj->FindScript(chained);
        }
        FEScript *cur = obj->FindScript(0x1744B3);
        obj->SetCurrentScript(cur);
        cur->mi32Time = 0;   // PC: no null check
        if (!mbPackage16)
            for (uint8_t i = 0; i < cur->mu32NumTracks; ++i) bridge::FEKeyInterpScript(cur, i, 0, obj);
        if (mpParentGroup) mpParentGroup->mChildren.AddTail(&obj->node);
        else mpPackage->mObjects.AddTail(&obj->node);
    }
    return true;
}

bool FEPackageReader::ReadObjectTags(const uint8_t *tags, uint32_t size)
{
    const uint8_t *end = tags + size;
    mbMergeResponses = 0;
    for (const uint8_t *t = tags; t < end; t += *reinterpret_cast<const uint16_t *>(t + 2) + 4) {
        const uint16_t id = *reinterpret_cast<const uint16_t *>(t);
        const uint32_t *v = reinterpret_cast<const uint32_t *>(t + 4);
        FEObject *obj = mpCurrentObject;
        switch (id) {
        case 0x504F: {   // "OP"
            obj->mu32GUID = v[0];
            obj->mu32NameHash = v[1];
            obj->mu32Flags = v[2];
            obj->_unknown22 = static_cast<uint16_t>(v[3]);
            if (obj->mu32Flags & 0x100000) {   // instance of a library object
                if (!FindReferencedObject(obj->mu32GUID, &mpRefObject, &mpRefPackage)) {
                    if (mpCurrentObject) bridge::VirtualScalarDelete(mpCurrentObject);
                    mpCurrentObject = nullptr;
                    return false;
                }
                mbMergeResponses = 1;
                mpCurrentObject->mu32Flags |= mpRefObject->mu32Flags;
                if (mpCurrentObject->_unknown22 == 0xFFFF) {
                    mpCurrentObject->_unknown24 = mpRefObject->_unknown24;
                    mpCurrentObject->_unknown28 = mpRefObject->_unknown28;
                }
                FEObject *copy;
                if (mpRefObject->mu32Type != 5) {
                    copy = bridge::FEObject_VirtualClone(mpRefObject, true);
                } else {
                    void *mem = bridge::Alloc(0x6C);
                    copy = mem ? bridge::FEGroup_CopyConstruct(mem, mpRefObject, false, true) : nullptr;
                }
                FEObject *old = mpCurrentObject;
                copy->mu32GUID = old->mu32GUID;
                copy->mu32NameHash = old->mu32NameHash;
                copy->mu32Flags = old->mu32Flags;
                copy->_unknown22 = old->_unknown22;
                copy->_unknown24 = old->_unknown24;
                copy->_unknown28 = old->_unknown28;
                if (old) bridge::VirtualScalarDelete(old);
                mpCurrentObject = copy;
                for (auto *s = reinterpret_cast<FEScript *>(copy->mScripts.head); s; s = reinterpret_cast<FEScript *>(s->node.next))
                    if (s->mpChainedScript)   // back to ids: ReadObjectChunk resolves them for the new object
                        s->mpChainedScript = reinterpret_cast<FEScript *>(static_cast<uintptr_t>(s->mpChainedScript->mu32Id));
            }
            if (mpCurrentObject->mu32Flags & 0x10000000) mpPackage->mppButtons[mu32ButtonIndex++] = mpCurrentObject;
            break;
        }
        case 0x4153: {   // "SA": raw object data
            const uint32_t n = *reinterpret_cast<const uint16_t *>(t + 2) >> 2;
            for (uint32_t i = 0; i < n; ++i) static_cast<uint32_t *>(obj->mpData)[i] = v[i];
            break;
        }
        case 0x4150:   // "PA": parent group (cached)
            if (!mpLastParent || mpLastParent->obj.mu32GUID != v[0])
                mpLastParent = reinterpret_cast<FEGroup *>(mpPackage->FindObjectByGUID(v[0]));
            mpParentGroup = mpLastParent;
            break;
        case 0x684F: obj->mu32NameHash = v[0]; break;   // "Oh"
        case 0x6E4F: if (mbFlag09) obj->SetName(reinterpret_cast<const char *>(v)); break;   // "On"
        case 0x744F: mpCurrentObject = CreateObject(v[0]); break;   // "Ot"
        default:
            if (!obj) break;
            switch (obj->mu32Type) {   // PC jump table 55541C / 555430
            case 1: case 9: case 10:
                if (id == 0x6649) reinterpret_cast<FEImage *>(obj)->mu32ImageFlags = v[0];   // "If": image flags
                break;
            case 2: bridge::FEPackageReader_ProcessStringTag(this, t); break;
            case 4: bridge::FEPackageReader_ProcessListBoxTag(this, t); break;
            case 6: bridge::FEPackageReader_ProcessCodeListBoxTag(this, t); break;
            default: break;
            }
        }
    }
    return true;
}

FEObject *FEPackageReader::CreateObject(uint32_t type)
{
    // sizes per type: PC jump table 534F84
    static const uint32_t kSizes[12] = {0, 0x60, 0x78, 0x5C, 0xAC, 0x6C, 0xC8, 0x60, 0x5C, 0x60, 0x60, 0x5C};
    if (type == 0) return nullptr;
    const bool plain = type == 3 || type == 8 || type > 11;
    void *mem = bridge::Alloc(plain ? 0x5C : kSizes[type]);
    FEObject *obj = nullptr;
    if (plain) obj = mem ? FEObject::Construct(mem) : nullptr;
    else if (type == 2) obj = mem ? reinterpret_cast<FEObject *>(FEString::Construct(mem)) : nullptr;
    else if (mem) obj = bridge::FEObjectTypeConstruct(mem, type);
    if (type == 6) reinterpret_cast<uint32_t *>(obj)[0x5C / 4] = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(mpGameInterface));   // PC: even when null
    obj->mu32Type = type;   // PC: no null check
    const uint32_t size = GetTypeSize(type);   // (inlined)
    bridge::ObjDataPool()->Free(obj->mpData);
    obj->mpData = nullptr;
    void *data = bridge::ObjDataPool()->Alloc(size);
    obj->mu32DataSize = size;
    obj->mpData = data;
    return obj;
}

bool FEPackageReader::ReadMessageTargetListChunk()
{
    const uint32_t *c = FindChild(mpData, 0x67726154);   // "Targ"
    if (!c) return true;
    const uint8_t *t = reinterpret_cast<const uint8_t *>(c + 2), *end = t + c[1];
    uint32_t entry = 0;
    for (; t < end; t += *reinterpret_cast<const uint16_t *>(t + 2) + 4) {
        const uint16_t id = *reinterpret_cast<const uint16_t *>(t);
        if (id == 0x6354) {   // "Tc"
            const uint32_t n = *reinterpret_cast<const uint32_t *>(t + 4);
            mpPackage->mu32NumMessageTargets = n;
            auto *mem = static_cast<uint32_t *>(bridge::Alloc(static_cast<uint32_t>(n * sizeof(FEMessageTargets) + 4)));   // PC: n * 0x10 + 4
            FEMessageTargets *arr = nullptr;
            if (mem) {
                mem[0] = n;   // new[] cookie
                arr = reinterpret_cast<FEMessageTargets *>(mem + 1);
                for (uint32_t i = 0; i < n; ++i) arr[i] = FEMessageTargets{};
            }
            mpPackage->mpMessageTargets = arr;
        } else if (id == 0x744D) {   // "Mt"
            const uint32_t *v = reinterpret_cast<const uint32_t *>(t + 4);
            const uint32_t n = (*reinterpret_cast<const uint16_t *>(t + 2) >> 2) - 1;
            FEMessageTargets &mt = mpPackage->mpMessageTargets[entry];
            mt.mu32MsgId = v[0];
            mt.SetCapacity(n);
            for (uint32_t i = 0; i < n; ++i) {
                FEObject *o = mpPackage->FindObjectByGUID(v[1 + i]);   // FEFindByGUID (0x79AAFC) walk
                if (mt.mu32Count == mt.mu32Capacity) mt.SetCapacity(mt.mu32Capacity + 1);
                mt.mpObjects[mt.mu32Count++] = o;
            }
            ++entry;
        }
    }
    return true;
}

bool FEPackageReader::FindReferencedObject(uint32_t refHash, FEObject **obj, FEPackage **pkg)
{
    *obj = nullptr;
    *pkg = nullptr;
    FELibraryRef *ref = mpPackage->FindLibraryReference(refHash);
    if (!ref) return false;
    FEPackage *lib = FEngine_FindLibraryPackage(mpEngine, ref->_unknown04[0]);
    *pkg = lib;
    if (!lib) return false;
    *obj = lib->FindObjectByGUID(ref->_unknown04[1]);
    return *obj != nullptr;
}

FEPackage *FEngine_FindLibraryPackage(FEngine *engine, uint32_t hash)
{
    for (FEPackage *p = reinterpret_cast<FEPackage *>(engine->mLibraries.head); p; p = reinterpret_cast<FEPackage *>(p->node.next)) {
        if (FEHashUpper(p->mpcFilename + 2) == hash) return p;   // PC: no null check on the filename
    }
    return nullptr;
}

FEPackage *FEngine_LoadPackage(FEngine *engine, const void *data, uint8_t package16)
{
    FEPackageReader r{};
    const auto *e = reinterpret_cast<const uint8_t *>(engine);
    return r.Load(data, engine->mpUIData, engine, e[3], e[4], package16);
}
