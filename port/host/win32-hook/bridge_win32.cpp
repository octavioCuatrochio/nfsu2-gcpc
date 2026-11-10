#include "frontend/FERender.h"
#include "frontend/FEPackageManager.h"
#include "bWare/bSlotPool.h"
#include "bWare/bMath.h"
// Bridge implementation for the hooked build: calls into the original SPEED2.EXE (PC v1.2).
#include "bridge/Bridge.h"

#define THISCALL __attribute__((thiscall))

namespace bridge {

void *Alloc(uint32_t bytes)
{
    return reinterpret_cast<void *(__cdecl *)(uint32_t)>(0x575620)(bytes);
}

void FreeArray(void *p)
{
    reinterpret_cast<void(__cdecl *)(void *)>(0x575630)(p);
}

void *WideCharHistogram()
{
    return *reinterpret_cast<void **>(0x8383DC);
}

void WideCharHistogram_UnpackString(void *histogram, uint16_t *dst, int dstBytes, const char *packed)
{
    reinterpret_cast<void(THISCALL *)(void *, uint16_t *, int, const char *)>(0x50D3C0)(histogram, dst, dstBytes, packed);
}

bool UseOldFEngDirtyFlag()
{
    return *reinterpret_cast<int *>(0x8384D4) != 0;
}


void *FEStringVTable()
{
    return reinterpret_cast<void *>(0x78F53C);
}

void CallLabelCallback(void *feString)
{
    void *cb = *reinterpret_cast<void **>(0x838478);
    if (!cb) return;
    auto fn = (*reinterpret_cast<void(THISCALL ***)(void *, void *)>(cb))[0];
    fn(cb, feString);
}

void *FEMinNodeVTable() { return reinterpret_cast<void *>(0x79A9E8); }
void *FEFieldNodeVTable() { return reinterpret_cast<void *>(0x79B018); }
void *FETypeNodeVTable() { return reinterpret_cast<void *>(0x79B90C); }
void *FEMinListVTable() { return reinterpret_cast<void *>(0x79A9EC); }

void VirtualScalarDtor(void *object, uint32_t flags)
{
    (*reinterpret_cast<void *(THISCALL ***)(void *, uint32_t)>(object))[0](object, flags);
}

void VirtualScalarDelete(void *object)
{
    auto fn = (*reinterpret_cast<void *(THISCALL ***)(void *, uint32_t)>(object))[0];
    fn(object, 1);
}

void *FEObjectVTable() { return reinterpret_cast<void *>(0x79B8E0); }

uint32_t NextFEObjectGUID()
{
    auto *g = reinterpret_cast<volatile uint32_t *>(0x7F7CF8);
    uint32_t v = *g;
    *g = v + 1;
    return v;
}

FEMultiPool *ObjDataPool() { return reinterpret_cast<FEMultiPool *>(0x863770); }
void *FESlotNodeVTable() { return reinterpret_cast<void *>(0x79B13C); }
void *FESlotPoolVTable() { return reinterpret_cast<void *>(0x79B8DC); }

void CallDestructorCallback(void *object)
{
    void *cb = *reinterpret_cast<void **>(0x838474);
    if (!cb) return;
    (*reinterpret_cast<void(THISCALL ***)(void *, void *)>(cb))[0](cb, object);
}

::FEMinList *FEMessageResponsePool() { return reinterpret_cast<::FEMinList *>(0x8637A0); }
::FEMinList *FEScriptPool() { return reinterpret_cast<::FEMinList *>(0x839C30); }
void *FEMessageResponseBlockVTable() { return reinterpret_cast<void *>(0x79BF50); }
void *FEScriptBlockVTable() { return reinterpret_cast<void *>(0x79BF54); }
void *FEScriptVTable() { return reinterpret_cast<void *>(0x79AD28); }
void *FEKeyNodeVTable() { return reinterpret_cast<void *>(0x79AA34); }
::FEMinList *FEKeyNodePool() { return reinterpret_cast<::FEMinList *>(0x839BD0); }
void *FEKeyNodeBlockVTable() { return reinterpret_cast<void *>(0x79B9EC); }
void *bWareMalloc(uint32_t size, const char *name, int32_t line, uint32_t flags)
{
    return reinterpret_cast<void *(__cdecl *)(uint32_t, const char *, int32_t, uint32_t)>(0x440390)(size, name, line, flags);
}
SlotPoolManager *TheSlotPoolManager() { return reinterpret_cast<SlotPoolManager *>(0x828530); }
SlotPool **BPNodeSlotPool() { return reinterpret_cast<SlotPool **>(0x8284F0); }
int32_t *BPListWantToClose() { return reinterpret_cast<int32_t *>(0x8284F4); }
void *bMalloc(uint32_t size, uint32_t flags) { return reinterpret_cast<void *(__cdecl *)(uint32_t, uint32_t)>(0x440820)(size, flags); }
void LZDecompress(const uint8_t *src, uint8_t *dst) { reinterpret_cast<void(__cdecl *)(const uint8_t *, uint8_t *)>(0x574670)(src, dst); }
FEPackageManager *TheFEPackageManager() { return reinterpret_cast<FEPackageManager *>(0x8637B0); }
void MenuScreen_BaseNotify(void *screen, uint32_t msg, void *obj, uint32_t arg, FEPackage *pkg)
{
    reinterpret_cast<void(THISCALL *)(void *, uint32_t, void *, uint32_t, FEPackage *)>(0x55B5C0)(screen, msg, obj, arg, pkg);
}
int32_t FEngLoadCounter() { return *reinterpret_cast<const int32_t *>(0x8384D0); }
int32_t *FEngLoadCounterPtr() { return reinterpret_cast<int32_t *>(0x8384D0); }
const void *FEGameInterface_GetPackageData(void *gi, const char *n, void **b, uint8_t *f)
{
    return (*reinterpret_cast<const void *(THISCALL ***)(void *, const char *, void **, uint8_t *)>(gi))[0](gi, n, b, f);
}
void FEGameInterface_ReleaseResources(void *gi, FEPackage *p, uint32_t n, void *r)
{
    (*reinterpret_cast<void(THISCALL ***)(void *, FEPackage *, uint32_t, void *)>(gi))[2](gi, p, n, r);
}
bool FEGameInterface_PackageWillUnload(void *gi, FEPackage *p) { return (*reinterpret_cast<bool(THISCALL ***)(void *, FEPackage *)>(gi))[4](gi, p); }
bool FEGameInterface_CanUnloadLibrary(void *gi, FEPackage *p) { return (*reinterpret_cast<bool(THISCALL ***)(void *, FEPackage *)>(gi))[5](gi, p); }
uint32_t FEGameInterface_GetJoyPadBits(void *gi, uint32_t pad) { return (*reinterpret_cast<uint32_t(THISCALL ***)(void *, uint32_t)>(gi))[0x40 / 4](gi, pad); }
void FEGameInterface_GetMouseInfo(void *gi, void *info) { (*reinterpret_cast<void(THISCALL ***)(void *, void *)>(gi))[0x44 / 4](gi, info); }
void FEGameInterface_BeginUpdate(void *gi) { (*reinterpret_cast<void(THISCALL ***)(void *)>(gi))[0x5C / 4](gi); }
void FEGameInterface_EndUpdate(void *gi) { (*reinterpret_cast<void(THISCALL ***)(void *)>(gi))[0x60 / 4](gi); }
uint8_t FEGameInterface_GetContextTransform(void *gi, uint32_t c, float *m) { return (*reinterpret_cast<uint8_t(THISCALL ***)(void *, uint32_t, float *)>(gi))[0x2C / 4](gi, c, m); }
uint8_t FEGameInterface_IsPointInObject(void *gi, float x, float y, FEObject *o) { return (*reinterpret_cast<uint8_t(THISCALL ***)(void *, float, float, FEObject *)>(gi))[0x48 / 4](gi, x, y, o); }
void FEGameInterface_BeginPackageRendering(void *gi, FEPackage *p, uint32_t f) { (*reinterpret_cast<void(THISCALL ***)(void *, FEPackage *, uint32_t)>(gi))[0x20 / 4](gi, p, f); }
void FEGameInterface_EndPackageRendering(void *gi, FEPackage *p) { (*reinterpret_cast<void(THISCALL ***)(void *, FEPackage *)>(gi))[0x24 / 4](gi, p); }
void FEGameInterface_GenerateRenderContext(void *gi, uint32_t c, FEObject *g) { (*reinterpret_cast<void(THISCALL ***)(void *, uint32_t, FEObject *)>(gi))[0x28 / 4](gi, c, g); }
void FEGameInterface_RenderObjectList(void *gi, void *e, uint32_t n) { (*reinterpret_cast<void(THISCALL ***)(void *, void *, uint32_t)>(gi))[0x30 / 4](gi, e, n); }
void FEGameInterface_GetViewTransformation(void *gi, void *m) { (*reinterpret_cast<void(THISCALL ***)(void *, void *)>(gi))[0x3C / 4](gi, m); }
FESortGlobals *FEngSortGlobals() { return reinterpret_cast<FESortGlobals *>(0x838488); }
uint32_t FEngHashStringUpper(const char *f) { return reinterpret_cast<uint32_t(__cdecl *)(const char *, ...)>(0x50B9C0)(f); }
void *FEngFindFont(uint32_t h) { return reinterpret_cast<void *(__cdecl *)(uint32_t)>(0x51BD70)(h); }
long double FEngFont_GetTextWidth(void *f, const uint16_t *t, uint32_t a) { return reinterpret_cast<long double(THISCALL *)(void *, const uint16_t *, uint32_t)>(0x50BEC0)(f, t, a); }
long double FEngFont_GetTextHeight(void *f, const uint16_t *t, uint32_t a) { return reinterpret_cast<long double(THISCALL *)(void *, const uint16_t *, uint32_t)>(0x50BF40)(f, t, a); }
long double FEngFont_CalculateXOffset(void *f, uint32_t j, float w) { return reinterpret_cast<long double(THISCALL *)(void *, uint32_t, float)>(0x50C020)(f, j, w); }
long double FEngFont_CalculateYOffset(void *f, uint32_t j, float h) { return reinterpret_cast<long double(THISCALL *)(void *, uint32_t, float)>(0x50C050)(f, j, h); }
bool FEngGetLocalizedString(uint16_t *b, int32_t n, uint32_t h) { return reinterpret_cast<bool(__cdecl *)(uint16_t *, int32_t, uint32_t)>(0x512570)(b, n, h); }
void *FEWidgetVTable() { return reinterpret_cast<void *>(0x790B98); }
void *FEButtonWidgetVTable() { return reinterpret_cast<void *>(0x79B2F8); }
void *FELabelWidgetVTable() { return reinterpret_cast<void *>(0x79B338); }
void *FEStatWidgetVTable() { return reinterpret_cast<void *>(0x79B378); }
void *FESliderWidgetVTable() { return reinterpret_cast<void *>(0x79B400); }
void *cSliderVTable() { return reinterpret_cast<void *>(0x78F544); }
const char *GameEmptyString() { return reinterpret_cast<const char *>(0x783FE5); }
void cSlider_Toggle(void *s, bool on) { (*reinterpret_cast<void(THISCALL ***)(void *, uint32_t)>(s))[0x0C / 4](s, on); }
void cSlider_Highlight(void *s) { (*reinterpret_cast<void(THISCALL ***)(void *)>(s))[0x10 / 4](s); }
void cSlider_UnHighlight(void *s) { (*reinterpret_cast<void(THISCALL ***)(void *)>(s))[0x14 / 4](s); }
void cSlider_Draw(void *s) { (*reinterpret_cast<void(THISCALL ***)(void *)>(s))[0x18 / 4](s); }
void cSlider_DrawVertical(void *s) { (*reinterpret_cast<void(THISCALL ***)(void *)>(s))[0x1C / 4](s); }
void cSlider_DrawHorizontal(void *s) { (*reinterpret_cast<void(THISCALL ***)(void *)>(s))[0x20 / 4](s); }
void FEPrintfInt(FEObject *o, const char *f, int32_t v) { reinterpret_cast<void(__cdecl *)(FEObject *, const char *, ...)>(0x537BE0)(o, f, v); }
uint32_t FEWidget_DetermineSoundTrigger(void *w) { return (*reinterpret_cast<uint32_t(THISCALL ***)(void *)>(w))[0x3C / 4](w); }
void FEWidget_Act(void *w, const char *p, uint32_t m) { (*reinterpret_cast<void(THISCALL ***)(void *, const char *, uint32_t)>(w))[0x04 / 4](w, p, m); }
void PlayFrontendSound(uint32_t trigger)
{
    const int32_t idx = reinterpret_cast<const int8_t *>(0x8380B8)[trigger];
    if (idx != -1) reinterpret_cast<void(THISCALL *)(void *, int32_t)>(0x458520)(*reinterpret_cast<void **>(0x82B884), idx);
}
int32_t IsJoystickPluggedIn(int32_t p) { return reinterpret_cast<int32_t(__cdecl *)(int32_t)>(0x571A60)(p); }
const uint8_t *JoyPortConfig() { return reinterpret_cast<const uint8_t *>(0x7F9414); }
uint32_t GetJoyEventTexture(const char *n, int32_t p) { return reinterpret_cast<uint32_t(__cdecl *)(const char *, int32_t)>(0x572190)(n, p); }
void *GetTextureInfo(uint32_t h, int32_t a, int32_t b) { return reinterpret_cast<void *(__cdecl *)(uint32_t, int32_t, int32_t)>(0x4901D0)(h, a, b); }
bool TextInputFocused() { return *reinterpret_cast<const uint8_t *>(0x83A9C0) && *reinterpret_cast<const uint32_t *>(0x83A9C4); }
void SetHoldDirtyFlags(uint32_t v) { *reinterpret_cast<uint32_t *>(0x7F7C8C) = v; }
void FEGameInterface_LibraryLoaded(void *gi, FEPackage *p) { (*reinterpret_cast<void(THISCALL ***)(void *, FEPackage *)>(gi))[3](gi, p); }
FEObject *FEObjectTypeConstruct(void *mem, uint32_t type)
{
    uintptr_t ctor = 0;
    switch (type) {
    case 1: ctor = 0x532640; break;
    case 5: ctor = 0x52B080; break;
    case 7: ctor = 0x52B0B0; break;
    case 9: ctor = 0x534B90; break;
    case 10: ctor = 0x534DB0; break;
    case 11: ctor = 0x52B480; break;
    default: return nullptr;
    }
    return reinterpret_cast<FEObject *(THISCALL *)(void *)>(ctor)(mem);
}
FEObject *FEObject_VirtualClone(FEObject *o, bool ref) { return (*reinterpret_cast<FEObject *(THISCALL ***)(FEObject *, uint32_t)>(o))[1](o, ref); }
FEObject *FEGroup_CopyConstruct(void *m, FEObject *s, bool a, bool b) { return reinterpret_cast<FEObject *(THISCALL *)(void *, FEObject *, uint32_t, uint32_t)>(0x5548A0)(m, s, a, b); }
void FEKeyInterpScript(void *s, uint8_t t, uint32_t time, void *o) { reinterpret_cast<void(__cdecl *)(void *, uint32_t, uint32_t, void *)>(0x541F60)(s, t, time, o); }
void FEGameInterface_FillCodeListBoxCell(void *gi, void *lb, int32_t c, int32_t r)
{
    (*reinterpret_cast<void(THISCALL ***)(void *, void *, int32_t, int32_t)>(gi))[0x4C / 4](gi, lb, c, r);
}
void FEGameInterface_Message(void *gi, uint32_t m, void *o, uint32_t a, uint32_t e)
{
    (*reinterpret_cast<void(THISCALL ***)(void *, uint32_t, void *, uint32_t, uint32_t)>(gi))[0x18 / 4](gi, m, o, a, e);
}
void *FECodeListBoxVTable() { return reinterpret_cast<void *>(0x79B904); }
void *FECodeListBoxDefaultCallback() { return *reinterpret_cast<void **>(0x7F7CC4); }
void *FEListBoxVTable() { return reinterpret_cast<void *>(0x79BAE4); }
void *FEPackageVTable() { return reinterpret_cast<void *>(0x79B8F8); }
void *FENodeVTable() { return reinterpret_cast<void *>(0x79AB0C); }
void *FEPackageList74VTable() { return reinterpret_cast<void *>(0x79B878); }
void *ResourceConnectorVTable() { return reinterpret_cast<void *>(0x79AB04); }
bool FEGameInterface_LoadResources(void *gi, FEPackage *p, uint32_t n, void *r)
{
    return (*reinterpret_cast<bool(THISCALL ***)(void *, FEPackage *, uint32_t, void *)>(gi))[1](gi, p, n, r);
}
void *FEPackageDataVTable() { return reinterpret_cast<void *>(0x79B15C); }
void *ScreenFactory(uint32_t h, FEPackage *p, int32_t a) { return reinterpret_cast<void *(__cdecl *)(uint32_t, FEPackage *, int32_t)>(0x50B790)(h, p, a); }
uint32_t FindScreenCreateData(uint32_t h) { return reinterpret_cast<uint32_t(__cdecl *)(uint32_t)>(0x50B840)(h); }
void HackClearCache(FEPackage *p) { reinterpret_cast<void(__cdecl *)(FEPackage *)>(0x50C550)(p); }
void FEngine_Update(void *e, uint32_t a, uint32_t b) { reinterpret_cast<void(THISCALL *)(void *, uint32_t, uint32_t)>(0x558C00)(e, a, b); }
bool IsCurrentlyHotChunking() { return reinterpret_cast<int(__cdecl *)()>(0x571090)() != 0; }
SlotPool *LevelSorterSlots() { return *reinterpret_cast<SlotPool **>(0x8384B0); }
FEngJoyMapEntry *FEngJoyMap() { return reinterpret_cast<FEngJoyMapEntry *>(0x7F91C8); }
uint32_t *FEngJoyRegistered() { return reinterpret_cast<uint32_t *>(0x8384B8); }

void *FEngHandleJoyFunction() { return reinterpret_cast<void *>(0x50C630); }   // the original handler (hooked)
void AddJoyHandlerToAllPorts(uint32_t e, void *h, void *u) { reinterpret_cast<void(__cdecl *)(uint32_t, void *, void *)>(0x5794B0)(e, h, u); }
void RemoveJoyHandlerFromAllPorts(uint32_t e) { reinterpret_cast<void(__cdecl *)(uint32_t)>(0x57D590)(e); }
void InitDebugLanguageChange() { reinterpret_cast<void(__cdecl *)(uint32_t, uint32_t, void *, void *)>(0x579410)(0, 0x34, reinterpret_cast<void *>(0x527520), nullptr); }
void CloseDebugLanguageChange() { reinterpret_cast<void(__cdecl *)(uint32_t, uint32_t)>(0x57C330)(0, 0x34); }
int32_t IsLiveJoystickPluggedIn(int32_t p) { return reinterpret_cast<int32_t(__cdecl *)(int32_t)>(0x571A80)(p); }
void *RenderObjectDisconnectVTable() { return reinterpret_cast<void *>(0x79AB9C); }
void FEGameInterface_RenderObject(void *gi, FEObject *o) { (*reinterpret_cast<void(THISCALL ***)(void *, FEObject *)>(gi))[0x34 / 4](gi, o); }
void cFEngRender_GenerateRenderContext(void *r, uint32_t c, FEObject *g) { reinterpret_cast<void(THISCALL *)(void *, uint32_t, FEObject *)>(0x51B7F0)(r, c, g); }
void cFEngRender_AddToRenderList(void *r, FEObject *o) { reinterpret_cast<void(THISCALL *)(void *, FEObject *)>(0x51B850)(r, o); }
void cFEngRender_PackageFinished(void *r, FEPackage *p) { reinterpret_cast<void(THISCALL *)(void *, FEPackage *)>(0x5371F0)(r, p); }
void FEPackageManager_NotifySoundMessage(FEPackageManager *m, uint32_t a, uint32_t b, uint32_t c, uint32_t d) { reinterpret_cast<void(THISCALL *)(void *, uint32_t, uint32_t, uint32_t, uint32_t)>(0x52CC90)(m, a, b, c, d); }
void bWareFree(void *p) { reinterpret_cast<void(__cdecl *)(void *)>(0x440540)(p); }
uint16_t BaselineFovNoise() { return *reinterpret_cast<const uint16_t *>(0x7EC1F0); }
uint32_t *BRandomSeed() { return reinterpret_cast<uint32_t *>(0x7EA624); }
const uint8_t *BFastATanTable() { return reinterpret_cast<const uint8_t *>(0x7EACB0); }
const uint16_t *BFixATanTable1() { return reinterpret_cast<const uint16_t *>(0x7EB1C0); }
const uint16_t *BFixATanTable2() { return reinterpret_cast<const uint16_t *>(0x7EB2C8); }
const bASinTableEntry *BASinTable() { return reinterpret_cast<const bASinTableEntry *>(0x7EA628); }
void FEUIData_Call4(void *ui, uint32_t slot, uint32_t a, const void *b, uint32_t c, const void *d)
{
    (*reinterpret_cast<void(THISCALL ***)(void *, uint32_t, const void *, uint32_t, const void *)>(ui))[slot / 4](ui, a, b, c, d);
}
void FEUIData_Call5(void *ui, uint32_t slot, uint32_t a, const void *b, const void *c, const void *d, uint32_t e)
{
    (*reinterpret_cast<void(THISCALL ***)(void *, uint32_t, const void *, const void *, const void *, uint32_t)>(ui))[slot / 4](ui, a, b, c, d, e);
}
void FEngine_UnloadPackage(void *e, FEPackage *p) { reinterpret_cast<void(THISCALL *)(void *, FEPackage *)>(0x5362E0)(e, p); }
FEPackage *FEngine_PushPackage(void *e, const char *n, uint8_t pri, uint32_t m)
{
    return reinterpret_cast<FEPackage *(THISCALL *)(void *, const char *, uint32_t, uint32_t)>(0x555860)(e, n, pri, m);
}
cFEng *cFEngInstance() { return *reinterpret_cast<cFEng **>(0x8384C4); }
void *FEPackageCommandVTable() { return reinterpret_cast<void *>(0x79AB64); }
void *FEMessageNodeVTable() { return reinterpret_cast<void *>(0x79AB60); }
void FEUIData_OnQueueMessage(void *ui, uint32_t msg, void *obj, void *pkg, void *target, uint32_t arg)
{
    auto fn = (*reinterpret_cast<void(THISCALL ***)(void *, uint32_t, void *, void *, void *, uint32_t)>(ui))[0x54 / 4];
    fn(ui, msg, obj, pkg, target, arg);
}
void FEngine_SendMessageToGame(void *engine, uint32_t msg, void *obj, void *pkg)
{
    void *ui = *reinterpret_cast<void **>(static_cast<uint8_t *>(engine) + 0x10C);
    auto fn = (*reinterpret_cast<void(THISCALL ***)(void *, uint32_t, void *, uint32_t, void *)>(ui))[6];
    fn(ui, msg, obj, 0, pkg);
}
bool FEObjectCallback_Call(void *cb, void *obj)
{
    return (*reinterpret_cast<bool(THISCALL ***)(void *, void *)>(cb))[1](cb, obj);
}
void FEListBox_Update(void *lb, float d) { reinterpret_cast<void(THISCALL *)(void *, float)>(0x517230)(lb, d); }
void FECodeListBox_Update(void *lb, float d) { reinterpret_cast<void(THISCALL *)(void *, float)>(0x504DC0)(lb, d); }
uint32_t HoldDirtyFlags() { return *reinterpret_cast<const uint32_t *>(0x7F7C8C); }
void *FERefListVTable() { return reinterpret_cast<void *>(0x79AA30); }
void *FEMessageResponseVTable() { return reinterpret_cast<void *>(0x79AA38); }

}  // namespace bridge
