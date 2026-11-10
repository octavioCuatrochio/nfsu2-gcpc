#include "frontend/FETypeLib.h"
#include "frontend/FERender.h"
#include "frontend/FEPackageManager.h"
#include "frontend/FEPackage.h"
#include "frontend/FEPackageReader.h"
#include "frontend/FEObject.h"
#include "frontend/FEListBox.h"
#include "frontend/FECodeListBox.h"
#include "frontend/FEHash.h"
#include "frontend/FELocalize.h"
#include "bWare/bSlotPool.h"
#include "frontend/cFEng.h"
#include "frontend/FENode.h"
// Bridge stand-ins for native unit tests: plain heap, no character histogram, settable dirty flag.
#include "bridge/Bridge.h"
#include "frontend/cFEngRender.h"
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
int32_t profileState = 0, gameModeFlags = 0, realTimer = 0;
uint8_t textInputFlag = 0, eamIconState = 0, resetMasksFlag = 0;
uint32_t textInputHandle = 0;
bool helpBarHandles = false;
bool showEAMIcons = false, callingScreenFrozen = false;
char callingScreen[64] = {};
const LanguageStringRecord *languageTable = nullptr;
uint32_t languageCount = 0;
int32_t tuningUnitType = 1, tuningUnavailableType = -1;
long double tuningMin = 0, tuningMax = 10, tuningValue = 5, tuningUpper = 8, tuningLower = 2;
bool useOldDirtyFlag = false;
int liveAllocations = 0;
int feStringVTable = 0;
void *lastLabelCallback = nullptr;
int feMinNodeVTable = 0, feMinListVTable = 0, virtualDeletes = 0;
int feFieldNodeVTable = 0, feTypeNodeVTable = 0;
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
int feCodeListBoxVTable = 0, feListBoxVTable = 0, fePackageDataVTable = 0, fePackageVTable = 0, feNodeVTable = 0, fePackageList74VTable = 0, resourceConnectorVTable = 0;
bool loadResourcesResult = true, readerChunkResult = true;
const void *libraryData = nullptr;
bool willUnloadResult = true;
uint32_t padBits = 0;
const float *contextTransform = nullptr;
std::vector<void *> hitObjects;
std::vector<HitTest> hitTests;
bool textInputFocused = false;
int fakeFont = 0;
int32_t joyPluggedPort = -1;
int32_t tuningCategory = 0, allPortsFlag = 0, activePort = 0, lastButton = 0;
bool speedMetric = false, hasLastButton = false, drawFEng = true, cursorEnabled = false;
int32_t discError = 0;
const char *singlePackage = nullptr;
uint8_t mouseGlobals[0x24] = {};
int32_t language = 0;
uint8_t fontMetrics[48] = {};
const void *realFontData = nullptr;
int giVTable, initializerVTable, movieStarterVTable, movieStopperVTable, dirtyMakerVTable;
int32_t insideScreenCtor = 0, gameFlowState = 0;
float worldTime = 0, realTime = 0;
uint32_t lastJoyTypes[2] = {};
uint8_t joyTypes[2] = {};
void *memoryCard = nullptr;
uint8_t moviePlayer[0x10] = {};
uint8_t movieTexture[0x50] = {};
int views[4];
const int32_t *renderStatsMode = nullptr;
uint32_t renderStats[4] = {};
uint8_t joyPortConfig[8] = {0xFF, 0, 0, 0, 0xFF, 0, 0, 0};
std::string lastJoyTextureName;
uint32_t lastTextureHash = 0;
int feWidgetVTable = 0, feButtonWidgetVTable = 0;
bool hitResult = false;
std::vector<void *> renderedObjects;
float viewMatrix[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
FESortGlobals sortGlobals{};
int16_t mouseInfo[4] = {};
void *screenResult = nullptr;
bool hotChunking = false;
::SlotPool *levelSorterSlots = nullptr;
int renderDisconnectVTable = 0, renderDisconnects = 0, registerJoys = 0;
FEngJoyMapEntry joyMap[kFEngJoyMapCount + 1] = {};   // +1: pads >= 4 index past the table like the original
uint32_t joyRegistered = 0;
bool joyPlugged = true;
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
#ifdef NFSU2_REFEREE
void *FEFieldNodeVTable() { return reinterpret_cast<void *>(0x79B018); }
void *FETypeNodeVTable() { return reinterpret_cast<void *>(0x79B90C); }
#else
void *FEFieldNodeVTable() { return &teststub::feFieldNodeVTable; }
void *FETypeNodeVTable() { return &teststub::feTypeNodeVTable; }
#endif
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
    else if (vt == &teststub::feCodeListBoxVTable) static_cast<FECodeListBox *>(o)->ScalarDeletingDestructor(1);
    else if (vt == &teststub::feListBoxVTable) static_cast<FEListBox *>(o)->ScalarDeletingDestructor(1);
    else if (vt == &teststub::feNodeVTable) { static_cast<FENode *>(o)->Destruct(); FreeArray(o); }
    else if (vt == &teststub::fePackageVTable) static_cast<FEPackage *>(o)->ScalarDeletingDestructor(1);
    else if (vt == &teststub::feTypeNodeVTable) static_cast<FETypeNode *>(o)->ScalarDeletingDestructor(1);
    else if (vt == &teststub::feFieldNodeVTable) static_cast<FEFieldNode *>(o)->ScalarDeletingDestructor(1);
    else if (vt == &teststub::fePackageDataVTable) static_cast<FEPackageData *>(o)->ScalarDeletingDestructor(1);
    else if (vt == &teststub::fePackageCommandVTable) {   // ~FENode frees the name
        FreeArray(static_cast<FENode *>(o)->mpcName);
        FreeArray(o);
    }
#ifdef NFSU2_REFEREE
    else if (reinterpret_cast<uintptr_t>(vt) < 0x400000 || reinterpret_cast<uintptr_t>(vt) >= 0x900000)   // check fakes
        (*reinterpret_cast<void *(__thiscall ***)(void *, uint32_t)>(o))[0](o, 1);
#endif
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
void FEGameInterface_ReleaseResources(void *gi, FEPackage *p, uint32_t n, void *) { teststub::delivered.push_back({"releaseres", gi, p, n, 0}); }
bool FEGameInterface_PackageWillUnload(void *gi, FEPackage *p) { teststub::delivered.push_back({"willunload", gi, p, 0, 0}); return teststub::willUnloadResult; }
bool FEGameInterface_CanUnloadLibrary(void *gi, FEPackage *p) { teststub::delivered.push_back({"canunloadlib", gi, p, 0, 0}); return true; }
uint32_t FEGameInterface_GetJoyPadBits(void *gi, uint32_t pad) { teststub::delivered.push_back({"padbits", gi, nullptr, pad, 0}); return teststub::padBits; }
void FEGameInterface_GetMouseInfo(void *gi, void *info) { teststub::delivered.push_back({"mouseinfo", gi, nullptr, 0, 0}); std::memcpy(info, teststub::mouseInfo, 8); }
void FEGameInterface_BeginUpdate(void *gi) { teststub::delivered.push_back({"begin", gi, nullptr, 0, 0}); }
void FEGameInterface_EndUpdate(void *gi) { teststub::delivered.push_back({"end", gi, nullptr, 0, 0}); }
#ifdef NFSU2_REFEREE
uint8_t FEGameInterface_GetContextTransform(void *gi, uint32_t c, float *m)
{
    return (*reinterpret_cast<uint8_t(__attribute__((thiscall)) ***)(void *, uint32_t, float *)>(gi))[0x2C / 4](gi, c, m);
}
#else
uint8_t FEGameInterface_GetContextTransform(void *gi, uint32_t c, float *m)
{
    teststub::delivered.push_back({"ctxxform", gi, nullptr, c, 0});
    if (!teststub::contextTransform) return 0;
    for (int i = 0; i < 16; ++i) m[i] = teststub::contextTransform[i];
    return 1;
}
#endif
uint8_t FEGameInterface_IsPointInObject(void *gi, float x, float y, FEObject *o)
{
    teststub::hitTests.push_back({o, x, y});
    for (void *h : teststub::hitObjects) if (h == o) return 1;
    return 0;
}
#ifdef NFSU2_REFEREE
void FEGameInterface_BeginPackageRendering(void *gi, FEPackage *p, uint32_t f) { (*reinterpret_cast<void(__attribute__((thiscall)) ***)(void *, FEPackage *, uint32_t)>(gi))[0x20 / 4](gi, p, f); }
void FEGameInterface_EndPackageRendering(void *gi, FEPackage *p) { (*reinterpret_cast<void(__attribute__((thiscall)) ***)(void *, FEPackage *)>(gi))[0x24 / 4](gi, p); }
void FEGameInterface_GenerateRenderContext(void *gi, uint32_t c, FEObject *g) { (*reinterpret_cast<void(__attribute__((thiscall)) ***)(void *, uint32_t, FEObject *)>(gi))[0x28 / 4](gi, c, g); }
void FEGameInterface_RenderObjectList(void *gi, void *e, uint32_t n) { (*reinterpret_cast<void(__attribute__((thiscall)) ***)(void *, void *, uint32_t)>(gi))[0x30 / 4](gi, e, n); }
void FEGameInterface_GetViewTransformation(void *gi, void *m) { (*reinterpret_cast<void(__attribute__((thiscall)) ***)(void *, void *)>(gi))[0x3C / 4](gi, m); }
FESortGlobals *FEngSortGlobals() { return reinterpret_cast<FESortGlobals *>(0x838488); }
FEngFontList *FontList() { return reinterpret_cast<FEngFontList *>(0x863768); }
const uint32_t *FontAlias() { return reinterpret_cast<const uint32_t *>(0x7F9184); }
const uint8_t *FontMetricsTable() { return reinterpret_cast<const uint8_t *>(0x7F918C); }
const char *LanguageSuffix(int32_t lang)
{
    const auto *t = reinterpret_cast<const uint8_t *>(0x7F7020);
    for (int i = 0; i < 11; ++i, t += 16)
        if (*reinterpret_cast<const int32_t *>(t) == lang) return *reinterpret_cast<const char *const *>(t + 4);
    return reinterpret_cast<const char *>(0x79AA54);
}
void *RealFont_Create(const void *d) { return const_cast<void *>(d); }
void RealFont_Destroy(void *) {}
void *FixupTextureInfoNull(void *t, uint32_t, uint32_t, uint32_t) { return t; }
void *FEGameInterfaceVTable() { return reinterpret_cast<void *>(0x79BCD8); }
void *FEObjectInitializerVTable() { return reinterpret_cast<void *>(0x79AAEC); }
void *FEngMovieStarterVTable() { return reinterpret_cast<void *>(0x79AB84); }
void *FEngMovieStopperVTable() { return reinterpret_cast<void *>(0x79AB94); }
void *FEDirtyMakerVTable() { return reinterpret_cast<void *>(0x79ABF0); }
::SlotPool **LevelSorterSlotsPtr() { return reinterpret_cast<::SlotPool **>(0x8384B0); }
int32_t InsideScreenConstructor() { return *reinterpret_cast<const int32_t *>(0x8384D0); }
int32_t GameFlowState() { return 0; }
float WorldTimeElapsed() { return 0; }
float RealTimeElapsed() { return 0; }
uint32_t *LastKnownJoystickTypes() { return reinterpret_cast<uint32_t *>(0x7F9414); }
uint8_t GetJoystickType(int32_t) { return 0; }
void *MemoryCardInstance() { return nullptr; }
void MemoryCard_Tick(void *, int32_t) {}
void MovieUpdate() {}
void MovieStop() {}
int32_t *SummonChyron() { static int32_t v; return &v; }
int32_t *StopChyron() { static int32_t v; return &v; }
void FakeCallFromSound() {}
void CurrentSongStopped(void *) {}
int32_t GetMovieNameEnum(const char *) { return 0; }
void CalculateMovieFilename(char *o, int32_t, const char *) { o[0] = 0; }
bool FILE_exists(const char *) { return false; }
void MovieStart(const char *, bool, int32_t) {}
void FEngine_InitObjects(::FEngine *) {}
void *cFEngRenderVTable() { return reinterpret_cast<void *>(0x79BBA0); }
int32_t CurrentLanguage() { return *reinterpret_cast<const int32_t *>(0x7F70D0); }
int32_t MovieActive() { return *reinterpret_cast<const int32_t *>(0x8383AC); }
const uint8_t *MoviePlayer() { return *reinterpret_cast<const uint8_t *const *>(0x8383A4); }
void *MovieTexture() { return reinterpret_cast<void *>(0x8385C0); }
::SlotPool **FERenderObjectPool() { return reinterpret_cast<::SlotPool **>(0x83847C); }
::SlotPool **FERenderEPolyPool() { return reinterpret_cast<::SlotPool **>(0x838480); }
::SlotPool **FERenderEPolyOverflowPool() { return reinterpret_cast<::SlotPool **>(0x838484); }
::FERenderBucket *FERenderBuckets() { return reinterpret_cast<::FERenderBucket *>(0x8639A0); }
void *eGetView(int32_t i) { return reinterpret_cast<void *>(0x832DE0 + 0x70 * i); }
void eView_Render(void *, ::ePoly *, void *, int32_t) {}
const int32_t *FERenderStatsMode() { return *reinterpret_cast<const int32_t *const *>(0x8900D8); }
uint32_t *FERenderStats() { return reinterpret_cast<uint32_t *>(0x837D20); }
void FEngFont_RenderString(void *, const int32_t *, const uint16_t *, FEObject *, const ::bMatrix4 *, ::FERenderObject *, void *) {}
int32_t *DiscErrorOccurred() { return reinterpret_cast<int32_t *>(0x864F2C); }
const char **FEngPleaseRenderSinglePackage() { return reinterpret_cast<const char **>(0x8384B4); }
const uint8_t *MouseGlobals() { return reinterpret_cast<const uint8_t *>(0x8763D4); }
void FEGameInterface_RenderObject(void *gi, FEObject *o) { (*reinterpret_cast<void(__attribute__((thiscall)) ***)(void *, FEObject *)>(gi))[0x34 / 4](gi, o); }
void FEPackageManager_NotifySoundMessage(FEPackageManager *m, uint32_t a, uint32_t b, uint32_t c, uint32_t d) { reinterpret_cast<void(__attribute__((thiscall)) *)(void *, uint32_t, uint32_t, uint32_t, uint32_t)>(0x52CC90)(m, a, b, c, d); }
#else
FEngFontList *FontList() { static FEngFontList l{reinterpret_cast<::FEngFont *>(&l), reinterpret_cast<::FEngFont *>(&l)}; return &l; }
const uint32_t *FontAlias() { static const uint32_t a[2] = {0x9583AA1Au, 0x5B9D88B9u}; return a; }
const uint8_t *FontMetricsTable() { return teststub::fontMetrics; }
const char *LanguageSuffix(int32_t lang)
{
    static const char *const names[11] = {"ENGLISH", "FRENCH", "GERMAN", "ITALIAN", "SPANISH", "DUTCH", "SWEDISH", "DANISH", "KOREAN", "CHINESE", "JAPANESE"};
    return lang >= 0 && lang < 11 ? names[lang] : "UNKNOWN";
}
void *RealFont_Create(const void *d) { teststub::realFontData = d; return const_cast<void *>(d); }
void RealFont_Destroy(void *f) { teststub::delivered.push_back({"fontdestroy", f, nullptr, 0, 0}); }
void *FixupTextureInfoNull(void *t, uint32_t h, uint32_t, uint32_t) { return h == 0x77 ? teststub::movieTexture : t; }
void *FEGameInterfaceVTable() { return &teststub::giVTable; }
void *FEObjectInitializerVTable() { return &teststub::initializerVTable; }
void *FEngMovieStarterVTable() { return &teststub::movieStarterVTable; }
void *FEngMovieStopperVTable() { return &teststub::movieStopperVTable; }
void *FEDirtyMakerVTable() { return &teststub::dirtyMakerVTable; }
::SlotPool **LevelSorterSlotsPtr() { static ::SlotPool *p; return &p; }
int32_t InsideScreenConstructor() { return teststub::insideScreenCtor; }
int32_t GameFlowState() { return teststub::gameFlowState; }
float WorldTimeElapsed() { return teststub::worldTime; }
float RealTimeElapsed() { return teststub::realTime; }
uint32_t *LastKnownJoystickTypes() { return teststub::lastJoyTypes; }
uint8_t GetJoystickType(int32_t p) { return teststub::joyTypes[p]; }
void *MemoryCardInstance() { return teststub::memoryCard; }
void MemoryCard_Tick(void *c, int32_t ms) { teststub::delivered.push_back({"cardtick", c, nullptr, static_cast<uint32_t>(ms), 0}); }
void MovieUpdate() { teststub::delivered.push_back({"movieupdate", nullptr, nullptr, 0, 0}); }
void MovieStop() { teststub::delivered.push_back({"moviestop", nullptr, nullptr, 0, 0}); }
int32_t *SummonChyron() { static int32_t v; return &v; }
int32_t *StopChyron() { static int32_t v; return &v; }
void FakeCallFromSound() { teststub::delivered.push_back({"chyron", nullptr, nullptr, 0, 0}); }
void CurrentSongStopped(void *) { teststub::delivered.push_back({"songstopped", nullptr, nullptr, 0, 0}); }
int32_t GetMovieNameEnum(const char *) { return 7; }
void CalculateMovieFilename(char *o, int32_t, const char *n) { std::strcpy(o, n); }
bool FILE_exists(const char *p) { return p[0] == 'Y'; }
void MovieStart(const char *p, bool l, int32_t w) { teststub::delivered.push_back({"moviestart", const_cast<char *>(p), nullptr, static_cast<uint32_t>(w), l}); }
void FEngine_InitObjects(::FEngine *e) { teststub::delivered.push_back({"initobjects", e, nullptr, 0, 0}); }
void *cFEngRenderVTable() { return &teststub::fakeFont; }
int32_t CurrentLanguage() { return teststub::language; }
int32_t MovieActive() { return 1; }
const uint8_t *MoviePlayer() { return teststub::moviePlayer; }
void *MovieTexture() { return teststub::movieTexture; }
::SlotPool **FERenderObjectPool() { static ::SlotPool *p; return &p; }
::SlotPool **FERenderEPolyPool() { static ::SlotPool *p; return &p; }
::SlotPool **FERenderEPolyOverflowPool() { static ::SlotPool *p; return &p; }
::FERenderBucket *FERenderBuckets()
{
    static ::FERenderBucket b[256];
    static bool init = false;
    if (!init) { for (auto &x : b) x.mpHead = x.mpTail = x.Sentinel(); init = true; }
    return b;
}
void *eGetView(int32_t i) { return &teststub::views[i]; }
void eView_Render(void *v, ::ePoly *p, void *t, int32_t f) { teststub::delivered.push_back({"eview", v, reinterpret_cast<FEPackage *>(p), static_cast<uint32_t>(f), static_cast<uint32_t>(reinterpret_cast<uintptr_t>(t))}); }
const int32_t *FERenderStatsMode() { return teststub::renderStatsMode; }
uint32_t *FERenderStats() { return teststub::renderStats; }
void FEngFont_RenderString(void *f, const int32_t *, const uint16_t *t, FEObject *o, const ::bMatrix4 *, ::FERenderObject *r, void *) { teststub::delivered.push_back({"fontrender", f, reinterpret_cast<FEPackage *>(o), static_cast<uint32_t>(reinterpret_cast<uintptr_t>(t)), static_cast<uint32_t>(reinterpret_cast<uintptr_t>(r))}); }
int32_t *DiscErrorOccurred() { return &teststub::discError; }
const char **FEngPleaseRenderSinglePackage() { return &teststub::singlePackage; }
const uint8_t *MouseGlobals() { return teststub::mouseGlobals; }
void FEGameInterface_RenderObject(void *gi, FEObject *o) { teststub::delivered.push_back({"renderobj", gi, reinterpret_cast<FEPackage *>(o), 0, 0}); }
void FEPackageManager_NotifySoundMessage(FEPackageManager *m, uint32_t a, uint32_t, uint32_t, uint32_t) { teststub::delivered.push_back({"soundmsg", m, nullptr, a, 0}); }
void FEGameInterface_BeginPackageRendering(void *gi, FEPackage *p, uint32_t f) { teststub::delivered.push_back({"beginrender", gi, p, f, 0}); }
void FEGameInterface_EndPackageRendering(void *gi, FEPackage *p) { teststub::delivered.push_back({"endrender", gi, p, 0, 0}); }
void FEGameInterface_GenerateRenderContext(void *gi, uint32_t c, FEObject *g) { teststub::delivered.push_back({"genctx", gi, reinterpret_cast<FEPackage *>(g), c, 0}); }
void FEGameInterface_RenderObjectList(void *gi, void *e, uint32_t n)
{
    teststub::delivered.push_back({"renderlist", gi, nullptr, n, 0});
    teststub::renderedObjects.clear();
    for (uint32_t i = 0; i < n; ++i) teststub::renderedObjects.push_back(static_cast<void **>(e)[i * 2]);
}
void FEGameInterface_GetViewTransformation(void *gi, void *m) { teststub::delivered.push_back({"view", gi, nullptr, 0, 0}); std::memcpy(m, teststub::viewMatrix, 64); }
FESortGlobals *FEngSortGlobals() { return &teststub::sortGlobals; }
#endif
#ifdef NFSU2_REFEREE
uint32_t FEngHashStringUpper(const char *f) { return reinterpret_cast<uint32_t(__cdecl *)(const char *, ...)>(0x50B9C0)(f); }
#else
uint32_t FEngHashStringUpper(const char *f) { return FEHashUpper(f); }   // no % in test names
#endif
#ifdef NFSU2_REFEREE
void *FEngFindFont(uint32_t h) { return reinterpret_cast<void *(__cdecl *)(uint32_t)>(0x51BD70)(h); }
long double FEngFont_GetTextWidth(void *f, const uint16_t *t, uint32_t a) { return reinterpret_cast<long double(__attribute__((thiscall)) *)(void *, const uint16_t *, uint32_t)>(0x50BEC0)(f, t, a); }
long double FEngFont_GetTextHeight(void *f, const uint16_t *t, uint32_t a) { return reinterpret_cast<long double(__attribute__((thiscall)) *)(void *, const uint16_t *, uint32_t)>(0x50BF40)(f, t, a); }
long double FEngFont_CalculateXOffset(void *f, uint32_t j, float w) { return reinterpret_cast<long double(__attribute__((thiscall)) *)(void *, uint32_t, float)>(0x50C020)(f, j, w); }
long double FEngFont_CalculateYOffset(void *f, uint32_t j, float h) { return reinterpret_cast<long double(__attribute__((thiscall)) *)(void *, uint32_t, float)>(0x50C050)(f, j, h); }
bool FEngGetLocalizedString(uint16_t *b, int32_t n, uint32_t h) { return reinterpret_cast<bool(__cdecl *)(uint16_t *, int32_t, uint32_t)>(0x512570)(b, n, h); }
#else
// fake font: width = 10 per character, height 20, offsets: justify 1 -> -width/2, else 0
void *FEngFindFont(uint32_t h) { return h ? &teststub::fakeFont : nullptr; }
long double FEngFont_GetTextWidth(void *, const uint16_t *t, uint32_t) { int n = 0; while (t && t[n]) ++n; return 10.0L * n; }
long double FEngFont_GetTextHeight(void *, const uint16_t *, uint32_t) { return 20.0L; }
long double FEngFont_CalculateXOffset(void *, uint32_t j, float w) { return j & 1 ? -w / 2.0L : 0.0L; }
long double FEngFont_CalculateYOffset(void *, uint32_t j, float h) { return j & 4 ? -h / 2.0L : 0.0L; }
bool FEngGetLocalizedString(uint16_t *b, int32_t, uint32_t h) { if (h != 0x77) return false; b[0] = 'A'; b[1] = 'B'; b[2] = 'C'; b[3] = 0; return true; }
#endif
#ifdef NFSU2_REFEREE
void *FEWidgetVTable() { return reinterpret_cast<void *>(0x790B98); }
void *FEButtonWidgetVTable() { return reinterpret_cast<void *>(0x79B2F8); }
void *FELabelWidgetVTable() { return reinterpret_cast<void *>(0x79B338); }
void *FEStatWidgetVTable() { return reinterpret_cast<void *>(0x79B378); }
void *FEInputWidgetVTable() { return reinterpret_cast<void *>(0x79B450); }
void *FEDateWidgetVTable() { return reinterpret_cast<void *>(0x79B490); }
void *FESliderWidgetVTable() { return reinterpret_cast<void *>(0x79B400); }
void *FEToggleWidgetVTable() { return reinterpret_cast<void *>(0x79B3B8); }
void *TuningSliderVTable() { return reinterpret_cast<void *>(0x79B7B0); }
void *MenuScreenVTable() { return reinterpret_cast<void *>(0x79AC00); }
int32_t ProfileState() { return *reinterpret_cast<const int32_t *>(0x8500D0); }
int32_t GameModeFlags() { return *reinterpret_cast<const int32_t *>(0x850078); }
const char *ProfileName(bool alt) { return reinterpret_cast<const char *>(alt ? 0x850128 : 0x8500EC); }
void SetShowEAMIcons(bool on) { *reinterpret_cast<uint8_t *>(0x838528) = on; }
bool CallingScreenFrozen() { return *reinterpret_cast<const uint8_t *>(0x838520) != 0; }
char *CallingScreenPackageName() { return reinterpret_cast<char *>(0x8384E0); }
void FEngPrintfString(const char *p, uint32_t h, const char *f, const char *s) { reinterpret_cast<void(__cdecl *)(const char *, uint32_t, const char *, ...)>(0x537B80)(p, h, f, s); }
void HelpBar_SyncByMask(uint32_t m, const char *p) { reinterpret_cast<void(__thiscall *)(void *, uint32_t, const char *)>(0x54E6E0)(*reinterpret_cast<void **>(0x8384D8), m, p); }
void *ThreeStageSliderVTable() { return reinterpret_cast<void *>(0x79AE54); }
void *cSliderVTable() { return reinterpret_cast<void *>(0x78F544); }
const char *GameEmptyString() { return reinterpret_cast<const char *>(0x783FE5); }
void cSlider_Toggle(void *s, bool on) { (*reinterpret_cast<void(__thiscall ***)(void *, uint32_t)>(s))[0x0C / 4](s, on); }
void cSlider_Highlight(void *s) { (*reinterpret_cast<void(__thiscall ***)(void *)>(s))[0x10 / 4](s); }
void cSlider_UnHighlight(void *s) { (*reinterpret_cast<void(__thiscall ***)(void *)>(s))[0x14 / 4](s); }
void cSlider_Draw(void *s) { (*reinterpret_cast<void(__thiscall ***)(void *)>(s))[0x18 / 4](s); }
void cSlider_DrawVertical(void *s) { (*reinterpret_cast<void(__thiscall ***)(void *)>(s))[0x1C / 4](s); }
void cSlider_DrawHorizontal(void *s) { (*reinterpret_cast<void(__thiscall ***)(void *)>(s))[0x20 / 4](s); }
void FEPrintfInts3(FEObject *o, const char *f, int32_t a, int32_t b, int32_t c) { reinterpret_cast<void(__cdecl *)(FEObject *, const char *, ...)>(0x537BE0)(o, f, a, b, c); }
void FEPrintfInt(FEObject *o, const char *f, int32_t v) { reinterpret_cast<void(__cdecl *)(FEObject *, const char *, ...)>(0x537BE0)(o, f, v); }
#else
void *FEWidgetVTable() { return &teststub::feWidgetVTable; }
void *FEButtonWidgetVTable() { return &teststub::feButtonWidgetVTable; }
void *FELabelWidgetVTable() { return &teststub::feButtonWidgetVTable; }
void *FEStatWidgetVTable() { return &teststub::feWidgetVTable; }
void *FEInputWidgetVTable() { return &teststub::feButtonWidgetVTable; }
void *FEDateWidgetVTable() { return &teststub::fakeFont; }
void *FESliderWidgetVTable() { return &teststub::feButtonWidgetVTable; }
void *FEToggleWidgetVTable() { return &teststub::feWidgetVTable; }
void *TuningSliderVTable() { return &teststub::feButtonWidgetVTable; }
void *MenuScreenVTable() { return &teststub::feWidgetVTable; }
int32_t ProfileState() { return teststub::profileState; }
int32_t GameModeFlags() { return teststub::gameModeFlags; }
const char *ProfileName(bool alt) { return alt ? "Alt" : "Player"; }
void SetShowEAMIcons(bool on) { teststub::showEAMIcons = on; }
bool CallingScreenFrozen() { return teststub::callingScreenFrozen; }
char *CallingScreenPackageName() { return teststub::callingScreen; }
void FEngPrintfString(const char *p, uint32_t h, const char *, const char *s) { teststub::delivered.push_back({"fengprintf", const_cast<char *>(s), nullptr, h, p ? static_cast<uint32_t>(p[0]) : 0u}); }
void HelpBar_SyncByMask(uint32_t m, const char *) { teststub::delivered.push_back({"helpbar", nullptr, nullptr, m, 0}); }
void *ThreeStageSliderVTable() { return &teststub::fakeFont; }
void *cSliderVTable() { return &teststub::fakeFont; }
const char *GameEmptyString() { return ""; }
void cSlider_Toggle(void *s, bool on) { teststub::delivered.push_back({"slider toggle", s, nullptr, on, 0}); }
void cSlider_Highlight(void *s) { teststub::delivered.push_back({"slider highlight", s, nullptr, 0, 0}); }
void cSlider_UnHighlight(void *s) { teststub::delivered.push_back({"slider unhighlight", s, nullptr, 0, 0}); }
void cSlider_Draw(void *s) { teststub::delivered.push_back({"slider draw", s, nullptr, 0, 0}); }
void cSlider_DrawVertical(void *s) { teststub::delivered.push_back({"slider drawv", s, nullptr, 0, 0}); }
void cSlider_DrawHorizontal(void *s) { teststub::delivered.push_back({"slider drawh", s, nullptr, 0, 0}); }
void FEPrintfInts3(FEObject *o, const char *, int32_t a, int32_t b, int32_t c) { teststub::delivered.push_back({"printf3", o, nullptr, static_cast<uint32_t>(a * 10000 + b * 100), static_cast<uint32_t>(c)}); }
void FEPrintfInt(FEObject *o, const char *, int32_t v) { teststub::delivered.push_back({"printf", o, nullptr, static_cast<uint32_t>(v), 0}); }
#endif
void FEPrintfStrings(FEObject *o, const char *, const char *a, const char *b) { teststub::delivered.push_back({"printf2", o, nullptr, a ? static_cast<uint32_t>(a[0]) : 0u, b ? static_cast<uint32_t>(b[0]) : 0u}); }
void FEPrintfDouble(FEObject *o, const char *f, double v) { teststub::delivered.push_back({"printfd", o, nullptr, static_cast<uint32_t>(f[4]), static_cast<uint32_t>(v * 100.0)}); }
#ifdef NFSU2_REFEREE
const LanguageStringRecord *LanguageStrings(uint32_t *n)
{
    *n = *reinterpret_cast<const uint32_t *>(0x8383D0);
    return *reinterpret_cast<const LanguageStringRecord *const *>(0x8383D8);
}
#else
const LanguageStringRecord *LanguageStrings(uint32_t *n) { *n = teststub::languageCount; return teststub::languageTable; }
#endif
const char *TuningMasterScreenName() { return "TuningMaster"; }
int32_t TuningUnitType() { return teststub::tuningUnitType; }
bool Tuning_IsAvailable(int32_t t) { teststub::delivered.push_back({"tuning avail", nullptr, nullptr, static_cast<uint32_t>(t), 0}); return t != teststub::tuningUnavailableType; }
long double Tuning_GetMin(int32_t) { return teststub::tuningMin; }
long double Tuning_GetMax(int32_t) { return teststub::tuningMax; }
long double Tuning_GetValue(int32_t) { return teststub::tuningValue; }
long double Tuning_GetUpper(int32_t) { return teststub::tuningUpper; }
long double Tuning_GetLower(int32_t) { return teststub::tuningLower; }
void Tuning_SetValue(float v, int32_t t) { teststub::tuningValue = v; teststub::delivered.push_back({"tuning set", nullptr, nullptr, static_cast<uint32_t>(t), 0}); }
long double CarTuneManager_GetStep(int32_t) { return 0.25L; }
bool cSlider_Update(void *s, uint32_t m) { teststub::delivered.push_back({"slider update", s, nullptr, m, 0}); return true; }
#ifdef NFSU2_REFEREE
void FEWidget_CheckMouse(void *w, const char *p, float x, float y) { (*reinterpret_cast<void(__thiscall ***)(void *, const char *, float, float)>(w))[0x08 / 4](w, p, x, y); }
void MenuScreen_Setup(void *s) { (*reinterpret_cast<void(__thiscall ***)(void *)>(s))[0x10 / 4](s); }
int32_t RealTimer() { return *reinterpret_cast<const int32_t *>(0x8651AC); }
int32_t KeyboardCreationTimer() { return *reinterpret_cast<const int32_t *>(0x83A9C8); }
float KeyboardDelaySeconds() { return *reinterpret_cast<const float *>(0x7F942C); }
#else
void FEWidget_CheckMouse(void *w, const char *, float x, float y) { teststub::delivered.push_back({"checkmouse", w, nullptr, static_cast<uint32_t>(x), static_cast<uint32_t>(y)}); }
void MenuScreen_Setup(void *s) { teststub::delivered.push_back({"setup", s, nullptr, 0, 0}); }
int32_t RealTimer() { return teststub::realTimer; }
int32_t KeyboardCreationTimer() { return 0; }
float KeyboardDelaySeconds() { return 0.75f; }
#endif
void FEDateWidget_SetDate(void *w) { teststub::delivered.push_back({"setdate", w, nullptr, 0, 0}); }
void FEInputWidget_SetText(void *w, const char *) { teststub::delivered.push_back({"settext", w, nullptr, 0, 0}); }
void FEInputWidget_SetInputFocus(void *w) { teststub::delivered.push_back({"inputfocus", w, nullptr, 0, 0}); }
void MenuScreen_BeginTextInput(void *s, uint32_t h, uint32_t, void *, void *, uint32_t, uint32_t) { teststub::delivered.push_back({"textinput", s, nullptr, h, 0}); }
char *EditedStringBuffer() { static char buf[256] = "edited"; return buf; }
#ifdef NFSU2_REFEREE
uint8_t *TextInputFlag() { return reinterpret_cast<uint8_t *>(0x83A9C0); }
uint32_t *TextInputHandle() { return reinterpret_cast<uint32_t *>(0x83A9C4); }
char *TextInputBuffer() { return reinterpret_cast<char *>(0x83A7B0); }
void TextInput_Start(void *o, uint32_t m, char *b, uint32_t mode) { reinterpret_cast<void(__thiscall *)(void *, void *, uint32_t, char *, uint32_t)>(0x4FF3B0)(reinterpret_cast<void *>(0x83A7B0), o, m, b, mode); }
void TextInput_Close(void *o, uint32_t f) { reinterpret_cast<void(__thiscall *)(void *, uint32_t)>(0x540AB0)(o, f); }
void TextInput_Accept(void *o) { reinterpret_cast<void(__thiscall *)(void *)>(0x54C8A0)(o); }
void TextInput_Cancel(void *o) { reinterpret_cast<void(__thiscall *)(void *)>(0x54C7F0)(o); }
void TextInput_Notify(void *o, uint32_t m) { reinterpret_cast<void(__thiscall *)(void *, uint32_t)>(0x540BF0)(o, m); }
void MenuScreen_NotificationMessage(void *s, uint32_t m, FEObject *o, uint32_t a, uint32_t p) { (*reinterpret_cast<void(__thiscall ***)(void *, uint32_t, FEObject *, uint32_t, uint32_t)>(s))[0x08 / 4](s, m, o, a, p); }
uint32_t MenuScreen_NotifySoundMessage(void *s, uint32_t m, FEObject *o, uint32_t a, uint32_t p, uint32_t e) { return (*reinterpret_cast<uint32_t(__thiscall ***)(void *, uint32_t, FEObject *, uint32_t, uint32_t, uint32_t)>(s))[0x0C / 4](s, m, o, a, p, e); }
void FEPrintfString(FEObject *o, const char *f, const char *a) { reinterpret_cast<void(__cdecl *)(FEObject *, const char *, ...)>(0x537BE0)(o, f, a); }
const char *ProfileNameBuffer() { return reinterpret_cast<const char *>(0x83A9E0); }
int32_t PlayerCash() { return *reinterpret_cast<const int32_t *>(0x861E74); }
int32_t Timer_CountDown(int32_t *t, float e) { return reinterpret_cast<int32_t(__thiscall *)(int32_t *, float)>(0x577310)(t, e); }
uint8_t *EAMIconState() { return reinterpret_cast<uint8_t *>(0x838524); }
uint8_t *ResetMasksFlag() { return reinterpret_cast<uint8_t *>(0x838521); }
bool ShowEAMIcons() { return *reinterpret_cast<const uint8_t *>(0x838528) != 0; }
void SetCallingScreenFrozen(bool on) { *reinterpret_cast<uint8_t *>(0x838520) = on; }
int32_t MessengerButtonTimer() { return *reinterpret_cast<const int32_t *>(0x8637C8); }
bool OnlineBusy() { return (reinterpret_cast<uint32_t(__thiscall *)(void *)>(0x5F00F0)(reinterpret_cast<void *>(0x89CF48)) & 0xFF) != 0; }
void Call4A0140(int32_t a) { reinterpret_cast<void(__cdecl *)(int32_t)>(0x4A0140)(a); }
void FEngSwitchPackages(const char *a, const char *b, uint32_t c, uint32_t d) { reinterpret_cast<void(__cdecl *)(const char *, const char *, uint32_t, uint32_t)>(0x555F10)(a, b, c, d); }
void FEngPushPackageOver(const char *a, const char *b, uint32_t c, uint32_t d) { reinterpret_cast<void(__cdecl *)(const char *, const char *, uint32_t, uint32_t)>(0x555EB0)(a, b, c, d); }
void PlayUISound(int32_t id) { reinterpret_cast<void(__thiscall *)(void *, int32_t)>(0x458520)(*reinterpret_cast<void **>(0x82B884), id); }
void ShowHelpMessage(const char *p, const char *s, uint32_t h) { reinterpret_cast<void(__cdecl *)(const char *, const char *, uint32_t)>(0x558AC0)(p, s, h); }
bool HelpBar_HandlesSelect() { return (reinterpret_cast<uint32_t(__thiscall *)(void *)>(0x50CF10)(*reinterpret_cast<void **>(0x8384D8)) & 0xFF) != 0; }
void ShowOneButtonDialog(const char *p, const char *s, uint32_t a, int32_t b, int32_t c, const char *m) { reinterpret_cast<void(__cdecl *)(const char *, const char *, uint32_t, int32_t, int32_t, const char *)>(0x558540)(p, s, a, b, c, m); }
const char *LocalizedString(uint32_t h) { return reinterpret_cast<const char *(__cdecl *)(uint32_t)>(0x4FFA80)(h); }
void *Online_Manager() { return reinterpret_cast<void *(__cdecl *)()>(0x58F6D0)(); }
void *Online_GetEntry(void *m, int32_t i) { return reinterpret_cast<void *(__thiscall *)(void *, int32_t)>(0x5847F0)(m, i); }
void *Online_Find(void *m, const char *n, uint32_t id) { return reinterpret_cast<void *(__thiscall *)(void *, const char *, uint32_t)>(0x5847A0)(m, n, id); }
const char *Online_DictGet(void *d, const char *k) { return reinterpret_cast<const char *(__cdecl *)(void *, const char *)>(0x7414A0)(d, k); }
void Online_CopyOrDefault(const char *s, char *d, uint32_t n, const char *def) { reinterpret_cast<void(__cdecl *)(const char *, char *, uint32_t, const char *)>(0x742360)(s, d, n, def); }
void Online_Select(void *m, const char *n, uint32_t id) { reinterpret_cast<void(__thiscall *)(void *, const char *, uint32_t, uint32_t)>(0x58D150)(m, n, id, 0); }
int32_t Online_Login(char *a, char *b, void *s)
{
    void *o = reinterpret_cast<void *(__cdecl *)(int32_t)>(0x58F740)(0);
    return reinterpret_cast<int32_t(__thiscall *)(void *, char *, char *, void *, void *)>(0x58D6B0)(o, a, b, reinterpret_cast<void *>(0x558F50), s);
}
uint32_t Online_558330(const char *p, const char *s, uint32_t h, const char *n) { return reinterpret_cast<uint32_t(__cdecl *)(const char *, const char *, uint32_t, const char *)>(0x558330)(p, s, h, n); }
const char *OnlineUserName() { return reinterpret_cast<const char *>(0x836014); }
char *OnlineField85009C() { return reinterpret_cast<char *>(0x85009C); }
char *OnlineField8500A9() { return reinterpret_cast<char *>(0x8500A9); }
uint8_t *OnlineFlag85009B() { return reinterpret_cast<uint8_t *>(0x85009B); }
uint32_t *OnlineLoginHandle() { return reinterpret_cast<uint32_t *>(0x83852C); }
#else
uint8_t *TextInputFlag() { return &teststub::textInputFlag; }
uint32_t *TextInputHandle() { return &teststub::textInputHandle; }
char *TextInputBuffer() { static char b[0x100]; return b; }
void TextInput_Start(void *o, uint32_t m, char *, uint32_t) { teststub::delivered.push_back({"ti start", o, nullptr, m, 0}); }
void TextInput_Close(void *o, uint32_t f) { teststub::delivered.push_back({"ti close", o, nullptr, f, 0}); }
void TextInput_Accept(void *o) { teststub::delivered.push_back({"ti accept", o, nullptr, 0, 0}); }
void TextInput_Cancel(void *o) { teststub::delivered.push_back({"ti cancel", o, nullptr, 0, 0}); }
void TextInput_Notify(void *o, uint32_t m) { teststub::delivered.push_back({"ti notify", o, nullptr, m, 0}); }
void MenuScreen_NotificationMessage(void *s, uint32_t m, FEObject *, uint32_t a, uint32_t) { teststub::delivered.push_back({"notify", s, nullptr, m, a}); }
uint32_t MenuScreen_NotifySoundMessage(void *s, uint32_t m, FEObject *, uint32_t, uint32_t, uint32_t e) { teststub::delivered.push_back({"notifysound", s, nullptr, m, e}); return e; }
void FEPrintfString(FEObject *o, const char *, const char *a) { teststub::delivered.push_back({"printfs", o, nullptr, a ? static_cast<uint32_t>(a[0]) : 0u, 0}); }
const char *ProfileNameBuffer() { return "Player"; }
int32_t PlayerCash() { return 1234; }
int32_t Timer_CountDown(int32_t *t, float) { *t -= 1000; return *t > 0; }
uint8_t *EAMIconState() { return &teststub::eamIconState; }
uint8_t *ResetMasksFlag() { return &teststub::resetMasksFlag; }
bool ShowEAMIcons() { return teststub::showEAMIcons; }
void SetCallingScreenFrozen(bool on) { teststub::callingScreenFrozen = on; }
int32_t MessengerButtonTimer() { return 0; }
bool OnlineBusy() { return true; }
void Call4A0140(int32_t) {}
void FEngSwitchPackages(const char *, const char *, uint32_t, uint32_t) { teststub::delivered.push_back({"switchpkg", nullptr, nullptr, 0, 0}); }
void FEngPushPackageOver(const char *, const char *, uint32_t, uint32_t) { teststub::delivered.push_back({"pushover", nullptr, nullptr, 0, 0}); }
void PlayUISound(int32_t id) { teststub::delivered.push_back({"uisound", nullptr, nullptr, static_cast<uint32_t>(id), 0}); }
void ShowHelpMessage(const char *, const char *, uint32_t h) { teststub::delivered.push_back({"help", nullptr, nullptr, h, 0}); }
bool HelpBar_HandlesSelect() { return teststub::helpBarHandles; }
void ShowOneButtonDialog(const char *, const char *, uint32_t a, int32_t, int32_t, const char *) { teststub::delivered.push_back({"dialog", nullptr, nullptr, a, 0}); }
const char *LocalizedString(uint32_t) { return "msg"; }
void *Online_Manager() { return nullptr; }
void *Online_GetEntry(void *, int32_t) { return nullptr; }
void *Online_Find(void *, const char *, uint32_t) { return nullptr; }
const char *Online_DictGet(void *, const char *) { return ""; }
void Online_CopyOrDefault(const char *, char *d, uint32_t, const char *def) { std::strcpy(d, def); }
void Online_Select(void *, const char *, uint32_t) {}
int32_t Online_Login(char *, char *, void *) { return -1; }
uint32_t Online_558330(const char *, const char *, uint32_t, const char *) { return 0; }
const char *OnlineUserName() { return "user"; }
char *OnlineField85009C() { static char b[13]; return b; }
char *OnlineField8500A9() { static char b[13]; return b; }
uint8_t *OnlineFlag85009B() { static uint8_t b; return &b; }
uint32_t *OnlineLoginHandle() { static uint32_t h; return &h; }
#endif
#ifdef NFSU2_REFEREE
void *UIWidgetMenuVTable() { return reinterpret_cast<void *>(0x795CDC); }
#else
void *UIWidgetMenuVTable() { return &teststub::feButtonWidgetVTable; }
#endif
#ifdef NFSU2_REFEREE   // widgets in referee checks carry (fake) vtables: call through them like the game
static void WidgetSlot(void *w, int off) { (*reinterpret_cast<void(__thiscall ***)(void *)>(w))[off / 4](w); }
void FEWidget_Position(void *w) { WidgetSlot(w, 0x10); }
void FEWidget_Show(void *w) { WidgetSlot(w, 0x14); }
void FEWidget_Hide(void *w) { WidgetSlot(w, 0x18); }
void FEWidget_Disable(void *w) { WidgetSlot(w, 0x20); }
void FEWidget_Slot17(void *w) { WidgetSlot(w, 0x44); }
void FEWidget_UnsetFocus(void *w) { WidgetSlot(w, 0x28); }
void FEWidget_SetFocus(void *w, const char *p) { (*reinterpret_cast<void(__thiscall ***)(void *, const char *)>(w))[0x24 / 4](w, p); }
void FEWidget_SetPosX(void *w, float x) { (*reinterpret_cast<void(__thiscall ***)(void *, float)>(w))[0x30 / 4](w, x); }
void FEWidget_SetPosY(void *w, float y) { (*reinterpret_cast<void(__thiscall ***)(void *, float)>(w))[0x34 / 4](w, y); }
void FEWidget_Draw(void *w) { WidgetSlot(w, 0x0C); }
#else
void FEWidget_Position(void *w) { teststub::delivered.push_back({"position", w, nullptr, 0, 0}); }
void FEWidget_Show(void *w) { teststub::delivered.push_back({"show", w, nullptr, 0, 0}); }
void FEWidget_Hide(void *w) { teststub::delivered.push_back({"hide", w, nullptr, 0, 0}); }
void FEWidget_Disable(void *w) { teststub::delivered.push_back({"disable", w, nullptr, 0, 0}); }
void FEWidget_Slot17(void *w) { teststub::delivered.push_back({"slot17", w, nullptr, 0, 0}); }
void FEWidget_UnsetFocus(void *w) { teststub::delivered.push_back({"unsetfocus", w, nullptr, 0, 0}); }
void FEWidget_SetFocus(void *w, const char *) { teststub::delivered.push_back({"setfocus", w, nullptr, 0, 0}); }
void FEWidget_SetPosX(void *w, float x) { teststub::delivered.push_back({"setposx", w, nullptr, static_cast<uint32_t>(x), 0}); }
void FEWidget_SetPosY(void *w, float y) { teststub::delivered.push_back({"setposy", w, nullptr, static_cast<uint32_t>(y), 0}); }
void FEWidget_Draw(void *w) { teststub::delivered.push_back({"draw", w, nullptr, 0, 0}); }
#endif
#ifdef NFSU2_REFEREE   // through the (fake) widget vtables, like the game
uint32_t FEWidget_DetermineSpecialSound(void *w, uint32_t m, uint32_t e) { return (*reinterpret_cast<uint32_t(__thiscall ***)(void *, uint32_t, uint32_t)>(w))[0x38 / 4](w, m, e); }
void FEWidget_BlinkArrows(void *w, uint32_t m) { (*reinterpret_cast<void(__thiscall ***)(void *, uint32_t)>(w))[0x40 / 4](w, m); }
uint32_t FEWidget_DetermineSoundTrigger(void *w) { return (*reinterpret_cast<uint32_t(__thiscall ***)(void *)>(w))[0x3C / 4](w); }
void FEWidget_Act(void *w, const char *p, uint32_t m) { (*reinterpret_cast<void(__thiscall ***)(void *, const char *, uint32_t)>(w))[0x04 / 4](w, p, m); }
#else
uint32_t FEWidget_DetermineSpecialSound(void *w, uint32_t m, uint32_t e) { teststub::delivered.push_back({"specialsound", w, nullptr, m, e}); return e + 10; }
void FEWidget_BlinkArrows(void *w, uint32_t m) { teststub::delivered.push_back({"blink", w, nullptr, m, 0}); }
uint32_t FEWidget_DetermineSoundTrigger(void *w) { teststub::delivered.push_back({"soundtrigger", w, nullptr, 0, 0}); return 8; }
void FEWidget_Act(void *w, const char *, uint32_t m) { teststub::delivered.push_back({"act", w, nullptr, m, 0}); }
#endif
void MakeFrontendSound(uint32_t e, uint32_t t) { teststub::delivered.push_back({"makesound", nullptr, nullptr, e, t}); }
void PlayFrontendSound(uint32_t t) { teststub::delivered.push_back({"sound", nullptr, nullptr, t, 0}); }
int32_t IsJoystickPluggedIn(int32_t p) { return p == teststub::joyPluggedPort; }
const uint8_t *JoyPortConfig() { return teststub::joyPortConfig; }
uint32_t GetJoyEventTexture(const char *n, int32_t p) { teststub::lastJoyTextureName = n; return 0x1000u + static_cast<uint32_t>(p); }
void *GetTextureInfo(uint32_t h, int32_t, int32_t) { teststub::lastTextureHash = h; return teststub::movieTexture; }
bool TextInputFocused() { return teststub::textInputFocused; }
#ifdef NFSU2_REFEREE
void SetHoldDirtyFlags(uint32_t v) { *reinterpret_cast<uint32_t *>(0x7F7C8C) = v; }
#else
void SetHoldDirtyFlags(uint32_t v) { teststub::holdDirtyFlags = v; }
#endif
void FEGameInterface_LibraryLoaded(void *gi, FEPackage *p) { teststub::delivered.push_back({"libloaded", gi, p, 0, 0}); }
FEObject *FEObjectTypeConstruct(void *mem, uint32_t type)
{
    teststub::delivered.push_back({"typector", mem, nullptr, type, 0});
    return FEObject::Construct(mem);   // stand-in: base part only
}
FEObject *FEObject_VirtualClone(FEObject *o, bool ref) { return o->Clone(ref); }
FEObject *FEGroup_CopyConstruct(void *, FEObject *, bool, bool) { std::abort(); }
void FEKeyInterpScript(void *s, uint8_t t, uint32_t time, void *o) { teststub::delivered.push_back({"keyinterp", s, static_cast<FEPackage *>(o), t, time}); }
#ifdef NFSU2_REFEREE
void FEGameInterface_Message(void *gi, uint32_t m, void *o, uint32_t a, uint32_t e)
{
    (*reinterpret_cast<void(__attribute__((thiscall)) ***)(void *, uint32_t, void *, uint32_t, uint32_t)>(gi))[0x18 / 4](gi, m, o, a, e);
}
void FEGameInterface_FillCodeListBoxCell(void *gi, void *lb, int32_t c, int32_t r)
{
    (*reinterpret_cast<void(__attribute__((thiscall)) ***)(void *, void *, int32_t, int32_t)>(gi))[0x4C / 4](gi, lb, c, r);
}
#else
void FEGameInterface_Message(void *gi, uint32_t m, void *o, uint32_t a, uint32_t) { teststub::delivered.push_back({"gimsg", gi, static_cast<FEPackage *>(o), m, a}); }
void FEGameInterface_FillCodeListBoxCell(void *gi, void *lb, int32_t c, int32_t r) { teststub::delivered.push_back({"fillcell", gi, static_cast<FEPackage *>(lb), uint32_t(c), uint32_t(r)}); }
#endif
#ifdef NFSU2_REFEREE
void *FECodeListBoxVTable() { return reinterpret_cast<void *>(0x79B904); }
void *FECodeListBoxDefaultCallback() { return *reinterpret_cast<void **>(0x7F7CC4); }
#else
void *FECodeListBoxVTable() { return &teststub::feCodeListBoxVTable; }
void *FECodeListBoxDefaultCallback() { return nullptr; }
#endif
#ifdef NFSU2_REFEREE
void *FEListBoxVTable() { return reinterpret_cast<void *>(0x79BAE4); }
#else
void *FEListBoxVTable() { return &teststub::feListBoxVTable; }
#endif
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
#ifdef NFSU2_REFEREE
FEngJoyMapEntry *FEngJoyMap() { return reinterpret_cast<FEngJoyMapEntry *>(0x7F91C8); }
uint32_t *FEngJoyRegistered() { return reinterpret_cast<uint32_t *>(0x8384B8); }
#else
FEngJoyMapEntry *FEngJoyMap() { return teststub::joyMap; }
uint32_t *FEngJoyRegistered() { return &teststub::joyRegistered; }
#endif
void *FEngHandleJoyFunction() { return reinterpret_cast<void *>(&cFEng::HandleJoy); }
void AddJoyHandlerToAllPorts(uint32_t e, void *h, void *u) { teststub::delivered.push_back({"addjoy", h, static_cast<FEPackage *>(u), e, 0}); }
void RemoveJoyHandlerFromAllPorts(uint32_t e) { teststub::delivered.push_back({"remjoy", nullptr, nullptr, e, 0}); }
void InitDebugLanguageChange() { ++teststub::registerJoys; }
void CloseDebugLanguageChange() { --teststub::registerJoys; }
int32_t IsLiveJoystickPluggedIn(int32_t p) { return teststub::joyPlugged && p >= 0; }
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
    if (*static_cast<void **>(cb) == &teststub::renderDisconnectVTable) { ++teststub::renderDisconnects; return RenderObjectDisconnect_Function(cb, static_cast<FEObject *>(obj)); }
    if (*static_cast<void **>(cb) == &teststub::initializerVTable) return FEObjectInitializer_Function(cb, static_cast<FEObject *>(obj));
    if (*static_cast<void **>(cb) == &teststub::movieStarterVTable) return FEngMovieStarter_Function(cb, static_cast<FEObject *>(obj));
    if (*static_cast<void **>(cb) == &teststub::movieStopperVTable) return FEngMovieStopper_Function(cb, static_cast<FEObject *>(obj));
    if (*static_cast<void **>(cb) == &teststub::dirtyMakerVTable) return FEDirtyMaker_Function(cb, static_cast<FEObject *>(obj));
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
#ifdef NFSU2_REFEREE
void *FERefListVTable() { return reinterpret_cast<void *>(0x79AA30); }
#else
void *FERefListVTable() { return &teststub::feRefListVTable; }
#endif
void *FEMessageResponseVTable() { return &teststub::feMsgRespVTable; }
void *FEObjectVTable() { return &teststub::feObjectVTable; }
uint32_t NextFEObjectGUID() { return teststub::nextGUID++; }
FEMultiPool *ObjDataPool() { return &teststub::objDataPool; }
void *FESlotNodeVTable() { return &teststub::feSlotNodeVTable; }
void *FESlotPoolVTable() { return &teststub::feSlotPoolVTable; }
void CallDestructorCallback(void *o) { teststub::lastDestructorCallback = o; }
// ---- TuningSlidersMenu (native tests: recorded / configurable) ----
void *TuningSlidersMenuVTable() { return &teststub::fakeFont; }
void HelpBar_SetFlag6C(uint8_t v) { teststub::delivered.push_back({"helpbar6c", nullptr, nullptr, v, 0}); }
void HelpBar_542990(bool on) { teststub::delivered.push_back({"helpbar542990", nullptr, nullptr, on, 0}); }
int32_t TuningCategory() { return teststub::tuningCategory; }
int32_t FEngAllPortsFlag() { return teststub::allPortsFlag; }
int32_t FEngActivePort() { return teststub::activePort; }
bool TuningAcceptBlocked() { return false; }
bool SpeedUnitsMetric() { return teststub::speedMetric; }
const char *TuningSlidersScreenName() { return "UI_PerformanceTuning_Sliders.fng"; }
int32_t *ScreenLastButtonSlot(uint32_t) { return teststub::hasLastButton ? &teststub::lastButton : nullptr; }
void FEngPrintfInt(const char *, uint32_t h, const char *, int32_t v) { teststub::delivered.push_back({"fengprintf", nullptr, nullptr, h, static_cast<uint32_t>(v)}); }
void TuneManager_ResetToDefault(int32_t t) { teststub::delivered.push_back({"resetdefault", nullptr, nullptr, static_cast<uint32_t>(t), 0}); }
void TuneManager_RebuildTheRideInfo() { teststub::delivered.push_back({"rebuildride", nullptr, nullptr, 0, 0}); }
void TuneManager_RebuildPlayersRideInfo() { teststub::delivered.push_back({"rebuildplayers", nullptr, nullptr, 0, 0}); }
void BuildPlayerCarPhysicsInfo() { teststub::delivered.push_back({"buildphysics", nullptr, nullptr, 0, 0}); }
void RaceCoordinator_RequestUnPause(int32_t v) { teststub::delivered.push_back({"unpause", nullptr, nullptr, static_cast<uint32_t>(v), 0}); }
void DialogInterface_ShowOk(const char *, const char *, uint32_t h) { teststub::delivered.push_back({"showok", nullptr, nullptr, h, 0}); }
// ---- frontend frame entry points ----
bool DrawFEng() { return teststub::drawFEng; }
void DrawFEGraphs() { teststub::delivered.push_back({"fegraphs", nullptr, nullptr, 0, 0}); }
bool MouseCursorEnabled() { return teststub::cursorEnabled; }
void DrawMouseCursor(int32_t x, int32_t y) { teststub::delivered.push_back({"cursor", nullptr, nullptr, static_cast<uint32_t>(x), static_cast<uint32_t>(y)}); }
}  // namespace bridge
