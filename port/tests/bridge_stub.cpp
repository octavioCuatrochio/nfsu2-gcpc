#include "frontend/FEPackageManager.h"
#include "frontend/FEPackage.h"
#include "frontend/FEPackageReader.h"
#include "frontend/FEObject.h"
#include "frontend/FEHash.h"
#include "bWare/bSlotPool.h"
#include "frontend/cFEng.h"
#include "frontend/FENode.h"
// Bridge stand-ins for native unit tests: plain heap, no character histogram, settable dirty flag.
#include "bridge/Bridge.h"
#include "bridge_stub.h"
#include "frontend/FEObject.h"
#include "frontend/FEMultiPool.h"
#include "frontend/FEPool.h"
#include "frontend/FEMessageResponse.h"
#include <new>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include "bWare/bMath.h"

namespace teststub {
bool useOldDirtyFlag = false;
int liveAllocations = 0;
int feStringVTable = 0;
void *lastLabelCallback = nullptr;
int feMinNodeVTable = 0, feMinListVTable = 0, virtualDeletes = 0;
int feObjectVTable = 0, feSlotNodeVTable = 0, feSlotPoolVTable = 0;
FEMultiPool objDataPool{{&feMinListVTable, 0, nullptr, nullptr}};
int feMsgRespVTable = 0, feMsgRespBlockVTable = 0, virtualDtorCalls = 0;
int feScriptBlockVTable = 0, feScriptVTable = 0, feRefListVTable = 0, feKeyNodeVTable = 0, keyNodeNews = 0, feKeyNodeBlockVTable = 0, linearCalls = 0;
uint32_t holdDirtyFlags = 0;
std::vector<ScriptMessage> messages;
std::vector<QueuedMessage> queued, forwarded;
int fePackageCommandVTable;
int feMessageNodeVTable = 0;
std::vector<Delivered> delivered;
void *currentButton = nullptr;
void *lastTypeUpdate = nullptr;
FEMinList keyNodePool{&feMinListVTable, 0, nullptr, nullptr};
FEMinList msgRespPool{&feMinListVTable, 0, nullptr, nullptr}, scriptPool{&feMinListVTable, 0, nullptr, nullptr};
uint32_t nextGUID = 100;
void *lastDestructorCallback = nullptr;
std::vector<std::string> levelSortingList;
FEPackage *pushResult = nullptr;
std::string lastPushName;
int32_t fengLoadCounter = 0;
int fePackageDataVTable = 0, fePackageVTable = 0, feNodeVTable = 0, fePackageList74VTable = 0, resourceConnectorVTable = 0;
bool loadResourcesResult = true, readerChunkResult = true;
const void *libraryData = nullptr;
std::vector<void *> readerScripts;
void *screenResult = nullptr;
bool hotChunking = false;
::SlotPool *levelSorterSlots = nullptr;
int renderDisconnectVTable = 0, renderDisconnects = 0, registerJoys = 0;
}  // namespace teststub

namespace bridge {
void *Alloc(uint32_t bytes) { ++teststub::liveAllocations; return std::malloc(bytes); }
void FreeArray(void *p) { if (p) --teststub::liveAllocations; std::free(p); }
void *WideCharHistogram() { return nullptr; }
void WideCharHistogram_UnpackString(void *, uint16_t *, int, const char *) { std::abort(); }
bool UseOldFEngDirtyFlag() { return teststub::useOldDirtyFlag; }
void *FEStringVTable() { return &teststub::feStringVTable; }
void CallLabelCallback(void *s) { teststub::lastLabelCallback = s; }
void *FEMinNodeVTable() { return &teststub::feMinNodeVTable; }
void *FEMinListVTable() { return &teststub::feMinListVTable; }
// dispatch on the stand-in vtables, like the game's real vtable slot 0 would
void VirtualScalarDelete(void *o)
{
    ++teststub::virtualDeletes;
    void *vt = *static_cast<void **>(o);
    if (vt == &teststub::feSlotNodeVTable) static_cast<FESlotNode *>(o)->ScalarDeletingDestructor(1);
    else if (vt == &teststub::feSlotPoolVTable) static_cast<FESlotPool *>(o)->ScalarDeletingDestructor(1);
    else if (vt == &teststub::feMsgRespBlockVTable)
        FEMessageResponseBlock_ScalarDtor(static_cast<FEMessageResponseBlock *>(o), 1);
    else if (vt == &teststub::feScriptVTable) static_cast<FEScript *>(o)->ScalarDeletingDestructor(1);
    else if (vt == &teststub::feMsgRespVTable) static_cast<FEMessageResponse *>(o)->ScalarDeletingDestructor(1);
    else if (vt == &teststub::feObjectVTable) static_cast<FEObject *>(o)->ScalarDeletingDestructor(1);
    else if (vt == &teststub::feKeyNodeVTable) static_cast<FEKeyNode *>(o)->ScalarDeletingDestructor(1);
    else if (vt == &teststub::feKeyNodeBlockVTable) FEKeyNodeBlock_ScalarDtor(static_cast<FEKeyNodeBlock *>(o), 1);
    else if (vt == &teststub::feScriptBlockVTable) FEScriptBlock_ScalarDtor(static_cast<FEScriptBlock *>(o), 1);
    else if (vt == &teststub::feNodeVTable) { static_cast<FENode *>(o)->Destruct(); FreeArray(o); }
    else if (vt == &teststub::fePackageVTable) static_cast<FEPackage *>(o)->ScalarDeletingDestructor(1);
    else if (vt == &teststub::fePackageDataVTable) static_cast<FEPackageData *>(o)->ScalarDeletingDestructor(1);
    else if (vt == &teststub::fePackageCommandVTable) {   // ~FENode frees the name
        FreeArray(static_cast<FENode *>(o)->mpcName);
        FreeArray(o);
    }
    else FreeArray(o);
}
void VirtualScalarDtor(void *o, uint32_t flags)
{
    if (*static_cast<void **>(o) == &teststub::feMinNodeVTable && flags == 0) { ++teststub::virtualDtorCalls; return; }
    if (flags & 1) VirtualScalarDelete(o); else ++teststub::virtualDtorCalls;
}
::FEMinList *FEMessageResponsePool() { return &teststub::msgRespPool; }
::FEMinList *FEScriptPool() { return &teststub::scriptPool; }
void *FEMessageResponseBlockVTable() { return &teststub::feMsgRespBlockVTable; }
void *FEScriptBlockVTable() { return &teststub::feScriptBlockVTable; }
void *FEScriptVTable() { return &teststub::feScriptVTable; }
void *FEKeyNodeVTable() { return &teststub::feKeyNodeVTable; }
::FEMinList *FEKeyNodePool() { ++teststub::keyNodeNews; return &teststub::keyNodePool; }
void *FEKeyNodeBlockVTable() { return &teststub::feKeyNodeBlockVTable; }
void *bWareMalloc(uint32_t size, const char *, int32_t, uint32_t)
{
    ++teststub::liveAllocations;
    void *p = std::malloc(size);
    if (p) std::memset(p, 0xCD, size);   // not zeroed
    return p;
}
SlotPoolManager *TheSlotPoolManager()
{
    static SlotPoolManager m = [] { SlotPoolManager x{}; x.mi32Enabled = 1; x.mpHead = x.mpTail = nullptr; return x; }();
    if (!m.mpHead) m.mpHead = m.mpTail = m.Sentinel();
    return &m;
}
SlotPool **BPNodeSlotPool() { static SlotPool *p; return &p; }
int32_t *BPListWantToClose() { static int32_t w; return &w; }
void *bMalloc(uint32_t size, uint32_t) { ++teststub::liveAllocations; return std::malloc(size); }
void LZDecompress(const uint8_t *src, uint8_t *dst) { std::memcpy(dst, src + 0x10, reinterpret_cast<const uint32_t *>(src)[2]); }   // stub: stored
FEPackageManager *TheFEPackageManager() { static FEPackageManager m{0, {&m.mList, &m.mList}}; return &m; }
void MenuScreen_BaseNotify(void *screen, uint32_t msg, void *obj, uint32_t arg, FEPackage *pkg)
{
    teststub::delivered.push_back({"notify", screen, pkg, msg, arg});
    (void)obj;
}
int32_t FEngLoadCounter() { return teststub::fengLoadCounter; }
int32_t *FEngLoadCounterPtr() { return &teststub::fengLoadCounter; }
const void *FEGameInterface_GetPackageData(void *gi, const char *n, void **b, uint8_t *f)
{
    teststub::delivered.push_back({"getpkg", gi, nullptr, FEHashUpper(n), 0});
    *f = teststub::libraryData != nullptr;
    if (*f) *b = bridge::Alloc(4);
    return teststub::libraryData;
}
void FEGameInterface_LibraryLoaded(void *gi, FEPackage *p) { teststub::delivered.push_back({"libloaded", gi, p, 0, 0}); }
FEObject *FEObjectTypeConstruct(void *mem, uint32_t type)
{
    teststub::delivered.push_back({"typector", mem, nullptr, type, 0});
    return FEObject::Construct(mem);   // stand-in: base part only
}
void FEPackageReader_ProcessStringTag(void *r, const uint8_t *t) { teststub::delivered.push_back({"strtag", r, nullptr, *reinterpret_cast<const uint16_t *>(t), 0}); }
void FEPackageReader_ProcessListBoxTag(void *r, const uint8_t *t) { teststub::delivered.push_back({"lbtag", r, nullptr, *reinterpret_cast<const uint16_t *>(t), 0}); }
void FEPackageReader_ProcessCodeListBoxTag(void *r, const uint8_t *t) { teststub::delivered.push_back({"clbtag", r, nullptr, *reinterpret_cast<const uint16_t *>(t), 0}); }
FEObject *FEObject_VirtualClone(FEObject *o, bool ref) { return o->Clone(ref); }
FEObject *FEGroup_CopyConstruct(void *, FEObject *, bool, bool) { std::abort(); }
bool FEPackageReader_ReadScriptTags(void *r, const uint8_t *, uint32_t n)
{
    teststub::delivered.push_back({"scrtags", r, nullptr, n, 0});
    if (FEObject *o = static_cast<FEPackageReader *>(r)->mpCurrentObject)
        for (void *s : teststub::readerScripts) o->mScripts.AddTail(static_cast<FEMinNode *>(s));
    return true;
}
void FEListBox_RecalculateCummulative(void *lb) { teststub::delivered.push_back({"recalc", lb, nullptr, 0, 0}); }
void FECodeListBox_FillAllCells(void *lb) { teststub::delivered.push_back({"fillcells", lb, nullptr, 0, 0}); }
void FEKeyInterpScript(void *s, uint8_t t, uint32_t time, void *o) { teststub::delivered.push_back({"keyinterp", s, static_cast<FEPackage *>(o), t, time}); }
void *FEPackageVTable() { return &teststub::fePackageVTable; }
void *FENodeVTable() { return &teststub::feNodeVTable; }
void *FEPackageList74VTable() { return &teststub::fePackageList74VTable; }
#ifdef NFSU2_REFEREE
void *ResourceConnectorVTable() { return reinterpret_cast<void *>(0x79AB04); }
#else
void *ResourceConnectorVTable() { return &teststub::resourceConnectorVTable; }
#endif
bool FEGameInterface_LoadResources(void *gi, FEPackage *p, uint32_t n, void *)
{
    teststub::delivered.push_back({"loadres", gi, p, n, 0});
    return teststub::loadResourcesResult;
}
void *FEPackageDataVTable() { return &teststub::fePackageDataVTable; }
void *ScreenFactory(uint32_t h, FEPackage *p, int32_t a)
{
    teststub::delivered.push_back({"screen", nullptr, p, h, uint32_t(teststub::fengLoadCounter)});
    (void)a;
    return teststub::screenResult;
}
#ifdef NFSU2_REFEREE
uint32_t FindScreenCreateData(uint32_t h) { return reinterpret_cast<uint32_t(__cdecl *)(uint32_t)>(0x50B840)(h); }   // static table
#else
uint32_t FindScreenCreateData(uint32_t h) { return h ^ 0x5A5A5A5Au; }
#endif
void HackClearCache(FEPackage *p) { teststub::delivered.push_back({"clearcache", nullptr, p, 0, 0}); }
void FEngine_Update(void *e, uint32_t a, uint32_t b) { teststub::delivered.push_back({"update", e, nullptr, a, b}); }
bool IsCurrentlyHotChunking() { return teststub::hotChunking; }
SlotPool *LevelSorterSlots() { return teststub::levelSorterSlots; }
void cFEng_RegisterJoy(cFEng *) { ++teststub::registerJoys; }
void *RenderObjectDisconnectVTable() { return &teststub::renderDisconnectVTable; }
void bWareFree(void *p) { if (p) --teststub::liveAllocations; std::free(p); }
#ifdef NFSU2_REFEREE
uint16_t BaselineFovNoise() { return *reinterpret_cast<const uint16_t *>(0x7EC1F0); }
#else
uint16_t BaselineFovNoise() { return 0x3000; }
#endif
#ifdef NFSU2_REFEREE
uint32_t *BRandomSeed() { return reinterpret_cast<uint32_t *>(0x7EA624); }
#else
uint32_t *BRandomSeed() { static uint32_t seed = 0x12345678; return &seed; }
#endif
#ifdef NFSU2_REFEREE
const uint8_t *BFastATanTable() { return reinterpret_cast<const uint8_t *>(0x7EACB0); }
const uint16_t *BFixATanTable1() { return reinterpret_cast<const uint16_t *>(0x7EB1C0); }
const uint16_t *BFixATanTable2() { return reinterpret_cast<const uint16_t *>(0x7EB2C8); }
#else
// formula approximations (match the game's tables to +-1; [256] of the byte table is 0 like the game's)
static uint16_t AtanUnits(double r) { return static_cast<uint16_t>(std::lround(std::atan(r) * 65536.0 / (2 * 3.14159265358979323846))); }
const uint8_t *BFastATanTable()
{
    static uint8_t t[257];
    for (int i = 0; i < 256; ++i) t[i] = static_cast<uint8_t>(std::lround(std::atan(i / 256.0) * 65536.0 / (2 * 3.14159265358979323846) / 32));
    return t;
}
const uint16_t *BFixATanTable1() { static uint16_t t[129]; for (int i = 0; i < 129; ++i) t[i] = AtanUnits(i / 32.0); return t; }
const uint16_t *BFixATanTable2() { static uint16_t t[129]; for (int i = 0; i < 129; ++i) t[i] = AtanUnits(i / 4.0); return t; }
#endif
const bASinTableEntry *BASinTable()
{
#ifdef NFSU2_REFEREE
    return reinterpret_cast<const bASinTableEntry *>(0x7EA628);   // runs inside SPEED2.EXE
#else
    static bASinTableEntry t[12 * 16 + 1];                          // formula approximation (not the game's data)
    static bool init = false;
    if (!init) {
        for (int seg = 0, start = 0, step = 0x8000; seg < 12; ++seg, start += step, step >>= 1)
            for (int k = 0; k < 16; ++k) {
                double x = (start + (step >> 4) * k) / 65536.0;
                t[seg * 16 + k].angle = static_cast<uint16_t>(std::asin(x) * 65536.0 / 6.283185307179586 + 0.5);
                t[seg * 16 + k].slope = static_cast<float>(1.0 / (6.283185307179586 * std::sqrt(1.0 - x * x)));
            }
        init = true;
    }
    return t;
#endif
}
void FECodeListBox_ScrollSelection(void *lb, int32_t dx, int32_t dy) { teststub::delivered.push_back({"cscroll", lb, nullptr, uint32_t(dx), uint32_t(dy)}); }
void FEUIData_Call4(void *, uint32_t slot, uint32_t a, const void *, uint32_t c, const void *) { teststub::delivered.push_back({"ui", nullptr, nullptr, a + slot * 0x10000, c}); }
void FEUIData_Call5(void *, uint32_t slot, uint32_t a, const void *, const void *, const void *, uint32_t e) { teststub::delivered.push_back({"fwd", nullptr, nullptr, a + slot * 0x10000, e}); }
void FEngine_UnloadPackage(void *, FEPackage *p) { teststub::delivered.push_back({"unload", nullptr, p, 0, 0}); }
FEPackage *FEngine_PushPackage(void *, const char *n, uint8_t pri, uint32_t m)
{
    teststub::lastPushName = n;
    teststub::delivered.push_back({"pushpkg", const_cast<char *>(n), teststub::pushResult, pri, m});
    return teststub::pushResult;
}
// test cFEng whose level sorting list mirrors teststub::levelSortingList (rebuilt on every call)
cFEng *cFEngInstance()
{
    static cFEng fe;
    static std::vector<cFEngLevelSorter> nodes;
    static std::vector<std::string> built;
    static bool first = true;
    if (!first && built == teststub::levelSortingList) return &fe;   // rebuild only when the configured list changes
    first = false;
    built = teststub::levelSortingList;
    nodes.assign(teststub::levelSortingList.size(), cFEngLevelSorter{});
    cFEngLevelSorter *prev = fe.SortSentinel();
    fe.mpSortHead = prev;
    for (size_t i = 0; i < nodes.size(); ++i) {
        nodes[i].mpcPackageName = teststub::levelSortingList[i].c_str();
        nodes[i].prev = prev;
        prev->next = &nodes[i];
        prev = &nodes[i];
    }
    prev->next = fe.SortSentinel();
    fe.mpSortTail = prev;
    return &fe;
}
void *FEPackageCommandVTable() { return &teststub::fePackageCommandVTable; }
void *FEMessageNodeVTable() { return &teststub::feMessageNodeVTable; }
void FEUIData_OnQueueMessage(void *, uint32_t msg, void *obj, void *, void *target, uint32_t) { teststub::forwarded.push_back({msg, obj, target}); }
void FEngine_SendMessageToGame(void *, uint32_t msg, void *obj, void *) { teststub::queued.push_back({msg, obj, reinterpret_cast<void *>(-1)}); }
bool FEObjectCallback_Call(void *cb, void *obj)
{
#ifdef NFSU2_REFEREE
    return (*reinterpret_cast<bool(__attribute__((thiscall)) ***)(void *, void *)>(cb))[1](cb, obj);
#endif
    if (*static_cast<void **>(cb) == &teststub::renderDisconnectVTable) { ++teststub::renderDisconnects; return true; }
    if (*static_cast<void **>(cb) == &teststub::resourceConnectorVTable) { teststub::delivered.push_back({"connect", obj, static_cast<FEPackage **>(cb)[1], 0, 0}); return true; }
    auto *t = static_cast<teststub::TestCallback *>(cb);
    t->visited.push_back(obj);
    return obj != t->stopAt;
}
void FEListBox_Update(void *lb, float) { teststub::lastTypeUpdate = lb; }
void FECodeListBox_Update(void *lb, float) { teststub::lastTypeUpdate = lb; }
uint32_t HoldDirtyFlags()
{
#ifdef NFSU2_REFEREE
    return *reinterpret_cast<const uint32_t *>(0x7F7C8C);
#else
    return teststub::holdDirtyFlags;
#endif
}
void *FERefListVTable() { return &teststub::feRefListVTable; }
void *FEMessageResponseVTable() { return &teststub::feMsgRespVTable; }
void *FEObjectVTable() { return &teststub::feObjectVTable; }
uint32_t NextFEObjectGUID() { return teststub::nextGUID++; }
FEMultiPool *ObjDataPool() { return &teststub::objDataPool; }
void *FESlotNodeVTable() { return &teststub::feSlotNodeVTable; }
void *FESlotPoolVTable() { return &teststub::feSlotPoolVTable; }
void CallDestructorCallback(void *o) { teststub::lastDestructorCallback = o; }
}  // namespace bridge
