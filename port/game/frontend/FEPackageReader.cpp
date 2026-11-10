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
#include "FEPool.h"
#include "FEListBox.h"
#include "FECodeListBox.h"
#include "FEMath.h"
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
                ReadScriptTags(data, t[1]);
            }
        }
        FEObject *obj = mpCurrentObject;
        if (!obj) continue;
        if (obj->mu32Type == 4) reinterpret_cast<FEListBox *>(obj)->RecalculateCummulative();
        else if (obj->mu32Type == 6) reinterpret_cast<FECodeListBox *>(obj)->FillAllCells();
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
            case 2: ProcessStringTag(t); break;
            case 4: ProcessListBoxTag(t); break;
            case 6: ProcessCodeListBoxTag(t); break;
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
    // sizeof() == the PC size on 32-bit (static_asserts); differs only in native 64-bit tests
    const uint32_t bytes = plain ? sizeof(FEObject) : type == 2 ? sizeof(FEString) : type == 4 ? sizeof(FEListBox) : type == 6 ? sizeof(FECodeListBox) : kSizes[type];
    void *mem = bridge::Alloc(bytes);
    FEObject *obj = nullptr;
    if (plain) obj = mem ? FEObject::Construct(mem) : nullptr;
    else if (type == 2) obj = mem ? reinterpret_cast<FEObject *>(FEString::Construct(mem)) : nullptr;
    else if (type == 4) obj = mem ? &FEListBox::Construct(mem)->obj : nullptr;
    else if (type == 6) obj = mem ? &FECodeListBox::Construct(mem)->obj : nullptr;
    else if (mem) obj = bridge::FEObjectTypeConstruct(mem, type);
    if (type == 6) reinterpret_cast<FECodeListBox *>(obj)->mpGameInterface = mpGameInterface;   // PC: even when null
    obj->mu32Type = type;   // PC: no null check
    const uint32_t size = GetTypeSize(type);   // (inlined)
    bridge::ObjDataPool()->Free(obj->mpData);
    obj->mpData = nullptr;
    void *data = bridge::ObjDataPool()->Alloc(size);
    obj->mu32DataSize = size;
    obj->mpData = data;
    return obj;
}

void FEPackageReader::ProcessListBoxTag(const uint8_t *tag)
{
    auto *lb = reinterpret_cast<FEListBox *>(mpCurrentObject);
    const uint32_t *v = reinterpret_cast<const uint32_t *>(tag + 4);
    switch (*reinterpret_cast<const uint16_t *>(tag)) {
    case 0x644C:   // "Ld"
        lb->SetNumColumns(v[0]);
        lb->SetNumRows(v[1]);
        mi32ListCell = mi32ListRow = mi32ListColumn = -1;
        lb->SetSelColumn(0);
        lb->SetSelRow(0);
        break;
    case 0x634C: {   // "Lc"
        FEListBoxSlot &c = lb->mpColumns[++mi32ListColumn];
        std::memcpy(&c.mfSize, &v[0], 4);
        c._unknown08 = v[1];
        break;
    }
    case 0x724C: {   // "Lr"
        FEListBoxSlot &r = lb->mpRows[++mi32ListRow];
        std::memcpy(&r.mfSize, &v[0], 4);
        r._unknown08 = v[1];
        break;
    }
    case 0x734C: std::memcpy(&lb->mfSpeedX, v, 4); std::memcpy(&lb->mfSpeedY, v + 1, 4); break;        // "Ls"
    case 0x764C: std::memcpy(&lb->mfViewWidth, v, 4); std::memcpy(&lb->mfViewHeight, v + 1, 4); break;   // "Lv"
    case 0x774C: lb->SetAutoWrap(v[0] != 0); break;                                                      // "Lw"
    case 0x5443: lb->SetCellType(v[0]); break;                                                           // "CT"
    case 0x6343: {   // "Cc": next cell, color
        if (++mi32ListCell != 0) lb->IncrementCellByColumn();
        const FEColor c{{static_cast<int32_t>(v[0] & 0xFF), static_cast<int32_t>((v[0] >> 8) & 0xFF),
                         static_cast<int32_t>((v[0] >> 16) & 0xFF), static_cast<int32_t>(v[0] >> 24)}};   // PC 5054A0
        lb->SetCellColor(c);
        break;
    }
    case 0x7343: { float xy[2]; std::memcpy(xy, v, 8); lb->SetCellScale(xy); break; }   // "Cs"
    case 0x7243: lb->SetCellJustification(0, 0, v[0]); break;                          // "Cr"
    case 0x6943: lb->SetCellData(v); break;                                            // "Ci"
    case 0x7443: lb->SetCellString(reinterpret_cast<const uint16_t *>(v)); break;      // "Ct"
    default: break;
    }
}

void FEPackageReader::ProcessCodeListBoxTag(const uint8_t *tag)
{
    auto *lb = reinterpret_cast<FECodeListBox *>(mpCurrentObject);
    const uint32_t *v = reinterpret_cast<const uint32_t *>(tag + 4);
    switch (*reinterpret_cast<const uint16_t *>(tag)) {
    case 0x6343: lb->SetCellColor(0, 0, v[0], lb->mu32NumColumns, lb->mu32NumRows); break;           // "Cc"
    case 0x6A4C: lb->SetCellJustification(0, 0, v[0], lb->mu32NumColumns, lb->mu32NumRows); break;   // "Lj"
    case 0x7343: {   // "Cs"
        float xy[2];
        std::memcpy(xy, v, 8);
        lb->SetCellScale(0, 0, xy, lb->mu32NumColumns, lb->mu32NumRows);
        break;
    }
    case 0x4953: lb->AllocateStrings(v[0], v[1]); break;                          // "SI"
    case 0x444C: lb->Initialize(v[0], v[1]); break;                               // "LD"
    case 0x744C: lb->mu32Flags = (v[0] & ~1u) | (lb->mu32Flags & 1u); break;      // "Lt"
    case 0x764C: std::memcpy(&lb->mfViewWidth, v, 4); std::memcpy(&lb->mfViewHeight, v + 1, 4); break;   // "Lv"
    default: break;
    }
}

void FEPackageReader::ProcessStringTag(const uint8_t *tag)
{
    auto *s = reinterpret_cast<FEString *>(mpCurrentObject);
    const uint32_t v = *reinterpret_cast<const uint32_t *>(tag + 4);
    switch (*reinterpret_cast<const uint16_t *>(tag)) {
    case 0x6253: s->mstrText.SetLength(v); break;                                          // "Sb"
    case 0x4C53: if (mbFlag09) s->SetLabel(reinterpret_cast<const char *>(tag + 4)); break;   // "SL"
    case 0x4853: s->SetLabelHash(v); break;                                                // "SH"
    case 0x6A53: s->_unknown6C = v; break;                                                 // "Sj"
    case 0x6C53: s->_unknown70 = v; break;                                                 // "Sl"
    case 0x7453: s->mstrText = reinterpret_cast<const uint16_t *>(tag + 4); break;         // "St"
    case 0x7753: s->_unknown74 = v; break;                                                 // "Sw"
    default: break;
    }
}

namespace {
void AppendKey(FERefList &l, FEMinNode *n)   // inlined AddTail (no count)
{
    FEMinNode *tail = l.tail;
    if (tail) {
        n->next = tail->next;
        if (tail->next) tail->next->prev = n;
        n->prev = tail;
        tail->next = n;
    } else {
        n->next = l.head;
        if (l.head) l.head->prev = n;
        n->prev = nullptr;
        l.head = n;
    }
    if (l.tail == tail) l.tail = n;
}
void SetLength24(FEKeyTrack &t, uint32_t v) { t._length[0] = uint8_t(v); t._length[1] = uint8_t(v >> 8); t._length[2] = uint8_t(v >> 16); }
FETypeParam *FindTypeParam(FEngine *e, uint32_t type, uint32_t index)
{
    auto *tn = reinterpret_cast<FETypeNode *>(e->mTypes.head);
    while (tn && tn->mu32Type != type) tn = reinterpret_cast<FETypeNode *>(tn->node.next);
    FEMinNode *p = tn->mParams.head;   // PC: no null check on the type
    for (uint32_t i = 0; p && i != index; ++i) p = p->next;
    return reinterpret_cast<FETypeParam *>(p);
}
}  // namespace

bool FEPackageReader::ReadScriptTags(const uint8_t *tags, uint32_t size)
{
    const uint8_t *end = tags + size;
    FEScript *s = nullptr;
    FEKeyTrack *track = nullptr;
    int32_t trackIndex = -1;
    uint32_t dataIndex = 0;
    for (const uint8_t *t = tags; t < end; t += *reinterpret_cast<const uint16_t *>(t + 2) + 4) {
        const uint16_t len = *reinterpret_cast<const uint16_t *>(t + 2);
        const uint32_t *v = reinterpret_cast<const uint32_t *>(t + 4);
        switch (*reinterpret_cast<const uint16_t *>(t)) {
        case 0x6E53: {   // "Sn": new script
            s = static_cast<FEScript *>(FEPoolNew<FEScript, 32>(*bridge::FEScriptPool(), FEScriptBlock_Construct));
            s->node.next = s->node.prev = reinterpret_cast<FEMinNode *>(FEMINNODE_UNLINKED);
            s->node.vtable = bridge::FEScriptVTable();
            s->mEvents.mu32Count = 0; s->mEvents.mpEvents = nullptr; s->mpcName = nullptr; s->mu32Id = 0;
            s->mi32Length = 0; s->mu32Flags = 0; s->mpChainedScript = nullptr; s->mu32NumTracks = 0; s->mpTracks = nullptr;
            s->mi32Time = 0;
            if (mbFlag0A) s->SetName(reinterpret_cast<const char *>(v));
            trackIndex = -1;
            dataIndex = 0;
            break;
        }
        case 0x6853:   // "Sh": header
            if (!s) {
                s = FEScript::Construct(FEPoolNew<FEScript, 32>(*bridge::FEScriptPool(), FEScriptBlock_Construct));
                s->mi32Time = 0;
                trackIndex = -1;
                dataIndex = 0;
            }
            s->mu32Id = v[0];
            s->mi32Length = static_cast<int32_t>(v[1]);
            s->mu32Flags = v[2];
            s->SetTrackCount(v[3]);
            break;
        case 0x4953:   // "SI": an existing script of the object
            s = mpCurrentObject->FindScript(v[0]);
            track = nullptr;
            break;
        case 0x6C53: s->mi32Length = static_cast<int32_t>(v[0]); break;            // "Sl"
        case 0x6653: if (s) s->mu32Flags = v[0]; break;                           // "Sf"
        case 0x6353: s->mpChainedScript = reinterpret_cast<FEScript *>(static_cast<uintptr_t>(v[0])); break;   // "Sc": id until ReadObjectChunk
        case 0x5645: {   // "EV": events
            const uint32_t n = len / 12;
            s->mEvents.SetCount(n);
            std::memcpy(s->mEvents.mpEvents, v, n * 12);   // PC: n == 0 would loop forever (do/while)
            break;
        }
        case 0x4946: {   // "FI": next track's header
            track = &s->mpTracks[++trackIndex];
            const uint8_t *b = reinterpret_cast<const uint8_t *>(v);
            track->mu8ParamType = b[0]; track->mu8ValueSize = b[1]; track->mu8InterpType = b[2]; track->mu8Flags = b[3];
            SetLength24(*track, v[1]);
            track->mi8DataIndex = static_cast<int8_t>(dataIndex);
            dataIndex += track->mu8ValueSize >> 2;
            break;
        }
        case 0x6154: if (s) track->mu8Flags = static_cast<uint8_t>(v[0]); break;        // "Ta"
        case 0x7454: if (s) track->mu8InterpType = static_cast<uint8_t>(v[0]); break;   // "Tt"
        case 0x6F54: track->mi8DataIndex = static_cast<int8_t>(v[0]); break;            // "To"
        case 0x6254: {   // "Tb": base key
            track->mBaseKey._data[0] = v[0];
            for (uint32_t i = 0, n = track->mu8ValueSize >> 2; i < n; ++i) track->mBaseKey._data[1 + i] = v[1 + i];
            break;
        }
        case 0x644B: {   // "Kd": keys {time, value}
            const uint32_t stride = track->mu8ValueSize + 4u;
            const uint32_t n = len / stride, words = (stride >> 2) - 1;
            if (track->mKeys.mbIsReference) {
                track->mKeys.head = track->mKeys.tail = nullptr;
                track->mKeys.mbIsReference = 0;
            }
            const uint32_t *k = v;
            uint32_t i = 0;
            do {   // PC: the first key is written even when n == 0
                FEKeyNode *key;
                if (i == 0) {
                    key = &track->mBaseKey;
                } else {
                    key = static_cast<FEKeyNode *>(FEKeyNode::PoolNew());
                    if (key) {
                        key->node.next = key->node.prev = reinterpret_cast<FEMinNode *>(FEMINNODE_UNLINKED);
                        key->node.vtable = bridge::FEKeyNodeVTable();
                    }
                }
                key->_data[0] = k[0];
                for (uint32_t w = 0; w < words; ++w) key->_data[1 + w] = k[1 + w];
                if (i != 0) AppendKey(track->mKeys, &key->node);
                k += stride / 4;
            } while (++i < n);
            break;
        }
        case 0x6954: {   // "Ti": track by data index; inserted from the type library when missing
            if (!s) break;
            const uint32_t index = *reinterpret_cast<const uint16_t *>(v);
            if ((track = s->FindTrack(static_cast<int>(index))) != nullptr) break;
            const uint32_t n = s->mu32NumTracks + 1;
            auto *raw = static_cast<uint32_t *>(bridge::Alloc(static_cast<uint32_t>(n * sizeof(FEKeyTrack) + 4)));
            FEKeyTrack *tracks = nullptr;
            if (raw) {
                raw[0] = n;
                tracks = reinterpret_cast<FEKeyTrack *>(raw + 1);
                for (uint32_t i = 0; i < n; ++i) FEKeyTrack::Construct(&tracks[i]);
            }
            const FETypeParam *param = FindTypeParam(mpEngine, mpCurrentObject->mu32Type, index);
            FEKeyTrack *old = s->mpTracks;
            uint32_t used = 0;
            for (uint32_t k = 0; k <= s->mu32NumTracks; ++k) {
                FEKeyTrack &dst = tracks[k];
                if (old && used < s->mu32NumTracks && (!param || old[used].mi8DataIndex < static_cast<int32_t>(param->mu32Offset) / 4)) {
                    dst.Assign(old[used]);
                    ++used;
                    continue;
                }
                dst.mu8ParamType = static_cast<uint8_t>(param->mu32ParamType);
                dst.mu8ValueSize = static_cast<uint8_t>(param->mu32ValueSize);
                dst.mu8InterpType = 1;
                dst.mu8Flags = 0;
                SetLength24(dst, static_cast<uint32_t>(s->mi32Length));
                dst.mi8DataIndex = static_cast<int8_t>(param->mu32Offset >> 2);
                track = &dst;
                param = nullptr;
            }
            if (old) DestroyTrackArray(old);
            s->mpTracks = tracks;
            ++s->mu32NumTracks;
            break;
        }
        default: break;
        }
    }
    if (!mbMergeResponses && s) mpCurrentObject->mScripts.AddTail(&s->node);
    if (s->mu32Id == 0x1744B3) mpCurrentObject->mpCurrentScript = s;   // PC: no null check
    return true;
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
