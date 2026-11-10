#include "frontend/FETypeLib.h"
#include "frontend/FERender.h"
#include "frontend/FEPackageManager.h"
#include "frontend/FEPackage.h"
#include "frontend/FEPackageReader.h"
#include "frontend/FEObject.h"
#include "frontend/FEListBox.h"
#include "frontend/FECodeListBox.h"
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
#else
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
void *FESliderWidgetVTable() { return reinterpret_cast<void *>(0x79B400); }
void *cSliderVTable() { return reinterpret_cast<void *>(0x78F544); }
const char *GameEmptyString() { return reinterpret_cast<const char *>(0x783FE5); }
void cSlider_Toggle(void *s, bool on) { (*reinterpret_cast<void(__thiscall ***)(void *, uint32_t)>(s))[0x0C / 4](s, on); }
void cSlider_Highlight(void *s) { (*reinterpret_cast<void(__thiscall ***)(void *)>(s))[0x10 / 4](s); }
void cSlider_UnHighlight(void *s) { (*reinterpret_cast<void(__thiscall ***)(void *)>(s))[0x14 / 4](s); }
void cSlider_Draw(void *s) { (*reinterpret_cast<void(__thiscall ***)(void *)>(s))[0x18 / 4](s); }
#else
void *FEWidgetVTable() { return &teststub::feWidgetVTable; }
void *FEButtonWidgetVTable() { return &teststub::feButtonWidgetVTable; }
void *FELabelWidgetVTable() { return &teststub::feButtonWidgetVTable; }
void *FEStatWidgetVTable() { return &teststub::feWidgetVTable; }
void *FESliderWidgetVTable() { return &teststub::feButtonWidgetVTable; }
void *cSliderVTable() { return &teststub::fakeFont; }
const char *GameEmptyString() { return ""; }
void cSlider_Toggle(void *s, bool on) { teststub::delivered.push_back({"slider toggle", s, nullptr, on, 0}); }
void cSlider_Highlight(void *s) { teststub::delivered.push_back({"slider highlight", s, nullptr, 0, 0}); }
void cSlider_UnHighlight(void *s) { teststub::delivered.push_back({"slider unhighlight", s, nullptr, 0, 0}); }
void cSlider_Draw(void *s) { teststub::delivered.push_back({"slider draw", s, nullptr, 0, 0}); }
#endif
uint32_t FEWidget_DetermineSoundTrigger(void *w) { teststub::delivered.push_back({"soundtrigger", w, nullptr, 0, 0}); return 8; }
void FEWidget_Act(void *w, const char *, uint32_t m) { teststub::delivered.push_back({"act", w, nullptr, m, 0}); }
void PlayFrontendSound(uint32_t t) { teststub::delivered.push_back({"sound", nullptr, nullptr, t, 0}); }
int32_t IsJoystickPluggedIn(int32_t p) { return p == teststub::joyPluggedPort; }
const uint8_t *JoyPortConfig() { return teststub::joyPortConfig; }
uint32_t GetJoyEventTexture(const char *n, int32_t p) { teststub::lastJoyTextureName = n; return 0x1000u + static_cast<uint32_t>(p); }
void *GetTextureInfo(uint32_t h, int32_t, int32_t) { teststub::lastTextureHash = h; return &teststub::fakeFont; }
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
