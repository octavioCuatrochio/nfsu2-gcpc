#include "frontend/FERender.h"
#include "frontend/FEPackageManager.h"
#include "bWare/bSlotPool.h"
#include "bWare/bMath.h"
// Bridge implementation for the hooked build: calls into the original SPEED2.EXE (PC v1.2).
#include "bridge/Bridge.h"
#include "frontend/FELocalize.h"

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
void *FEInputWidgetVTable() { return reinterpret_cast<void *>(0x79B450); }
void *FEDateWidgetVTable() { return reinterpret_cast<void *>(0x79B490); }
void *FESliderWidgetVTable() { return reinterpret_cast<void *>(0x79B400); }
void *FEToggleWidgetVTable() { return reinterpret_cast<void *>(0x79B3B8); }
void *MenuScreenVTable() { return reinterpret_cast<void *>(0x79AC00); }
int32_t ProfileState() { return *reinterpret_cast<const int32_t *>(0x8500D0); }
int32_t GameModeFlags() { return *reinterpret_cast<const int32_t *>(0x850078); }
const char *ProfileName(bool alt) { return reinterpret_cast<const char *>(alt ? 0x850128 : 0x8500EC); }
void SetShowEAMIcons(bool on) { *reinterpret_cast<uint8_t *>(0x838528) = on; }
bool CallingScreenFrozen() { return *reinterpret_cast<const uint8_t *>(0x838520) != 0; }
char *CallingScreenPackageName() { return reinterpret_cast<char *>(0x8384E0); }
void FEngPrintfString(const char *p, uint32_t h, const char *f, const char *s) { reinterpret_cast<void(__cdecl *)(const char *, uint32_t, const char *, ...)>(0x537B80)(p, h, f, s); }
void HelpBar_SyncByMask(uint32_t m, const char *p) { reinterpret_cast<void(THISCALL *)(void *, uint32_t, const char *)>(0x54E6E0)(*reinterpret_cast<void **>(0x8384D8), m, p); }
void *TuningSliderVTable() { return reinterpret_cast<void *>(0x79B7B0); }
void *ThreeStageSliderVTable() { return reinterpret_cast<void *>(0x79AE54); }
void *cSliderVTable() { return reinterpret_cast<void *>(0x78F544); }
const char *GameEmptyString() { return reinterpret_cast<const char *>(0x783FE5); }
void cSlider_Toggle(void *s, bool on) { (*reinterpret_cast<void(THISCALL ***)(void *, uint32_t)>(s))[0x0C / 4](s, on); }
void cSlider_Highlight(void *s) { (*reinterpret_cast<void(THISCALL ***)(void *)>(s))[0x10 / 4](s); }
void cSlider_UnHighlight(void *s) { (*reinterpret_cast<void(THISCALL ***)(void *)>(s))[0x14 / 4](s); }
void cSlider_Draw(void *s) { (*reinterpret_cast<void(THISCALL ***)(void *)>(s))[0x18 / 4](s); }
void cSlider_DrawVertical(void *s) { (*reinterpret_cast<void(THISCALL ***)(void *)>(s))[0x1C / 4](s); }
void cSlider_DrawHorizontal(void *s) { (*reinterpret_cast<void(THISCALL ***)(void *)>(s))[0x20 / 4](s); }
void FEPrintfStrings(FEObject *o, const char *f, const char *a, const char *b) { reinterpret_cast<void(__cdecl *)(FEObject *, const char *, ...)>(0x537BE0)(o, f, a, b); }
void FEPrintfDouble(FEObject *o, const char *f, double v) { reinterpret_cast<void(__cdecl *)(FEObject *, const char *, ...)>(0x537BE0)(o, f, v); }
const LanguageStringRecord *LanguageStrings(uint32_t *n)
{
    *n = *reinterpret_cast<const uint32_t *>(0x8383D0);
    return *reinterpret_cast<const LanguageStringRecord *const *>(0x8383D8);
}
const char *TuningMasterScreenName() { return *reinterpret_cast<const char *const *>(0x7FA214); }
static void *TuningObject() { return *reinterpret_cast<void **>(0x83A540); }
int32_t TuningUnitType() { return *reinterpret_cast<const int32_t *>(static_cast<const char *>(TuningObject()) + 4); }
bool Tuning_IsAvailable(int32_t t) { return (reinterpret_cast<uint32_t(THISCALL *)(void *, int32_t)>(0x59B080)(TuningObject(), t) & 0xFF) != 0; }
static long double TuningGet(uintptr_t fn, int32_t t) { return reinterpret_cast<long double(THISCALL *)(void *, int32_t, int32_t, int32_t)>(fn)(TuningObject(), t, 0, 1); }
long double Tuning_GetMin(int32_t t) { return TuningGet(0x5B0BD0, t); }
long double Tuning_GetMax(int32_t t) { return TuningGet(0x5B0C00, t); }
long double Tuning_GetValue(int32_t t) { return TuningGet(0x5B3E40, t); }
long double Tuning_GetUpper(int32_t t) { return TuningGet(0x5B2BD0, t); }
long double Tuning_GetLower(int32_t t) { return TuningGet(0x5B2BA0, t); }
void Tuning_SetValue(float v, int32_t t) { reinterpret_cast<void(THISCALL *)(void *, float, int32_t, int32_t, int32_t)>(0x5B4670)(TuningObject(), v, t, 0, 1); }
long double CarTuneManager_GetStep(int32_t t) { return reinterpret_cast<long double(THISCALL *)(void *, int32_t)>(0x522560)(reinterpret_cast<void *>(0x839C40), t); }
bool cSlider_Update(void *s, uint32_t m) { return ((*reinterpret_cast<uint32_t(THISCALL ***)(void *, uint32_t)>(s))[0x04 / 4](s, m) & 0xFF) != 0; }
void FEWidget_CheckMouse(void *w, const char *p, float x, float y) { (*reinterpret_cast<void(THISCALL ***)(void *, const char *, float, float)>(w))[0x08 / 4](w, p, x, y); }
void MenuScreen_Setup(void *s) { (*reinterpret_cast<void(THISCALL ***)(void *)>(s))[0x10 / 4](s); }
void FEDateWidget_SetDate(void *w) { reinterpret_cast<void(THISCALL *)(void *)>(0x538060)(w); }
void FEInputWidget_SetText(void *w, const char *t) { reinterpret_cast<void(THISCALL *)(void *, const char *)>(0x4957D0)(w, t); }
void FEInputWidget_SetInputFocus(void *w) { reinterpret_cast<void(THISCALL *)(void *)>(0x51EB60)(w); }
void MenuScreen_BeginTextInput(void *s, uint32_t h, uint32_t m, void *b, void *b2, uint32_t mode, uint32_t f)
{
    reinterpret_cast<void(THISCALL *)(void *, uint32_t, uint32_t, void *, void *, uint32_t, uint32_t)>(0x555F80)(s, h, m, b, b2, mode, f);
}
char *EditedStringBuffer() { return reinterpret_cast<char *>(0x83A8B0); }
int32_t RealTimer() { return *reinterpret_cast<const int32_t *>(0x8651AC); }
int32_t KeyboardCreationTimer() { return *reinterpret_cast<const int32_t *>(0x83A9C8); }
float KeyboardDelaySeconds() { return *reinterpret_cast<const float *>(0x7F942C); }
uint8_t *TextInputFlag() { return reinterpret_cast<uint8_t *>(0x83A9C0); }
uint32_t *TextInputHandle() { return reinterpret_cast<uint32_t *>(0x83A9C4); }
char *TextInputBuffer() { return reinterpret_cast<char *>(0x83A7B0); }
void TextInput_Start(void *o, uint32_t m, char *b, uint32_t mode) { reinterpret_cast<void(THISCALL *)(void *, void *, uint32_t, char *, uint32_t)>(0x4FF3B0)(reinterpret_cast<void *>(0x83A7B0), o, m, b, mode); }
void TextInput_Close(void *o, uint32_t f) { reinterpret_cast<void(THISCALL *)(void *, uint32_t)>(0x540AB0)(o, f); }
void TextInput_Accept(void *o) { reinterpret_cast<void(THISCALL *)(void *)>(0x54C8A0)(o); }
void TextInput_Cancel(void *o) { reinterpret_cast<void(THISCALL *)(void *)>(0x54C7F0)(o); }
void TextInput_Notify(void *o, uint32_t m) { reinterpret_cast<void(THISCALL *)(void *, uint32_t)>(0x540BF0)(o, m); }
void MenuScreen_NotificationMessage(void *s, uint32_t m, FEObject *o, uint32_t a, uint32_t p) { (*reinterpret_cast<void(THISCALL ***)(void *, uint32_t, FEObject *, uint32_t, uint32_t)>(s))[0x08 / 4](s, m, o, a, p); }
uint32_t MenuScreen_NotifySoundMessage(void *s, uint32_t m, FEObject *o, uint32_t a, uint32_t p, uint32_t e) { return (*reinterpret_cast<uint32_t(THISCALL ***)(void *, uint32_t, FEObject *, uint32_t, uint32_t, uint32_t)>(s))[0x0C / 4](s, m, o, a, p, e); }
void FEPrintfString(FEObject *o, const char *f, const char *a) { reinterpret_cast<void(__cdecl *)(FEObject *, const char *, ...)>(0x537BE0)(o, f, a); }
const char *ProfileNameBuffer() { return reinterpret_cast<const char *>(0x83A9E0); }
int32_t PlayerCash() { return *reinterpret_cast<const int32_t *>(0x861E74); }
int32_t Timer_CountDown(int32_t *t, float e) { return reinterpret_cast<int32_t(THISCALL *)(int32_t *, float)>(0x577310)(t, e); }
uint8_t *EAMIconState() { return reinterpret_cast<uint8_t *>(0x838524); }
uint8_t *ResetMasksFlag() { return reinterpret_cast<uint8_t *>(0x838521); }
bool ShowEAMIcons() { return *reinterpret_cast<const uint8_t *>(0x838528) != 0; }
void SetCallingScreenFrozen(bool on) { *reinterpret_cast<uint8_t *>(0x838520) = on; }
int32_t MessengerButtonTimer() { return *reinterpret_cast<const int32_t *>(0x8637C8); }
bool OnlineBusy() { return (reinterpret_cast<uint32_t(THISCALL *)(void *)>(0x5F00F0)(reinterpret_cast<void *>(0x89CF48)) & 0xFF) != 0; }
void Call4A0140(int32_t a) { reinterpret_cast<void(__cdecl *)(int32_t)>(0x4A0140)(a); }
void FEngSwitchPackages(const char *a, const char *b, uint32_t c, uint32_t d) { reinterpret_cast<void(__cdecl *)(const char *, const char *, uint32_t, uint32_t)>(0x555F10)(a, b, c, d); }
void FEngPushPackageOver(const char *a, const char *b, uint32_t c, uint32_t d) { reinterpret_cast<void(__cdecl *)(const char *, const char *, uint32_t, uint32_t)>(0x555EB0)(a, b, c, d); }
void PlayUISound(int32_t id) { reinterpret_cast<void(THISCALL *)(void *, int32_t)>(0x458520)(*reinterpret_cast<void **>(0x82B884), id); }
void ShowHelpMessage(const char *p, const char *s, uint32_t h) { reinterpret_cast<void(__cdecl *)(const char *, const char *, uint32_t)>(0x558AC0)(p, s, h); }
bool HelpBar_HandlesSelect() { return (reinterpret_cast<uint32_t(THISCALL *)(void *)>(0x50CF10)(*reinterpret_cast<void **>(0x8384D8)) & 0xFF) != 0; }
void ShowOneButtonDialog(const char *p, const char *s, uint32_t a, int32_t b, int32_t c, const char *m) { reinterpret_cast<void(__cdecl *)(const char *, const char *, uint32_t, int32_t, int32_t, const char *)>(0x558540)(p, s, a, b, c, m); }
const char *LocalizedString(uint32_t h) { return reinterpret_cast<const char *(__cdecl *)(uint32_t)>(0x4FFA80)(h); }
void *Online_Manager() { return reinterpret_cast<void *(__cdecl *)()>(0x58F6D0)(); }
void *Online_GetEntry(void *m, int32_t i) { return reinterpret_cast<void *(THISCALL *)(void *, int32_t)>(0x5847F0)(m, i); }
void *Online_Find(void *m, const char *n, uint32_t id) { return reinterpret_cast<void *(THISCALL *)(void *, const char *, uint32_t)>(0x5847A0)(m, n, id); }
const char *Online_DictGet(void *d, const char *k) { return reinterpret_cast<const char *(__cdecl *)(void *, const char *)>(0x7414A0)(d, k); }
void Online_CopyOrDefault(const char *s, char *d, uint32_t n, const char *def) { reinterpret_cast<void(__cdecl *)(const char *, char *, uint32_t, const char *)>(0x742360)(s, d, n, def); }
void Online_Select(void *m, const char *n, uint32_t id) { reinterpret_cast<void(THISCALL *)(void *, const char *, uint32_t, uint32_t)>(0x58D150)(m, n, id, 0); }
int32_t Online_Login(char *a, char *b, void *s)
{
    void *o = reinterpret_cast<void *(__cdecl *)(int32_t)>(0x58F740)(0);
    return reinterpret_cast<int32_t(THISCALL *)(void *, char *, char *, void *, void *)>(0x58D6B0)(o, a, b, reinterpret_cast<void *>(0x558F50), s);
}
uint32_t Online_558330(const char *p, const char *s, uint32_t h, const char *n) { return reinterpret_cast<uint32_t(__cdecl *)(const char *, const char *, uint32_t, const char *)>(0x558330)(p, s, h, n); }
const char *OnlineUserName() { return reinterpret_cast<const char *>(0x836014); }
char *OnlineField85009C() { return reinterpret_cast<char *>(0x85009C); }
char *OnlineField8500A9() { return reinterpret_cast<char *>(0x8500A9); }
uint8_t *OnlineFlag85009B() { return reinterpret_cast<uint8_t *>(0x85009B); }
uint32_t *OnlineLoginHandle() { return reinterpret_cast<uint32_t *>(0x83852C); }
void *UIWidgetMenuVTable() { return reinterpret_cast<void *>(0x795CDC); }
static void WidgetSlot(void *w, int off) { (*reinterpret_cast<void(THISCALL ***)(void *)>(w))[off / 4](w); }
void FEWidget_Position(void *w) { WidgetSlot(w, 0x10); }
void FEWidget_Show(void *w) { WidgetSlot(w, 0x14); }
void FEWidget_Hide(void *w) { WidgetSlot(w, 0x18); }
void FEWidget_Disable(void *w) { WidgetSlot(w, 0x20); }
void FEWidget_Slot17(void *w) { WidgetSlot(w, 0x44); }
void FEWidget_UnsetFocus(void *w) { WidgetSlot(w, 0x28); }
void FEWidget_SetFocus(void *w, const char *p) { (*reinterpret_cast<void(THISCALL ***)(void *, const char *)>(w))[0x24 / 4](w, p); }
void FEWidget_SetPosX(void *w, float x) { (*reinterpret_cast<void(THISCALL ***)(void *, float)>(w))[0x30 / 4](w, x); }
void FEWidget_SetPosY(void *w, float y) { (*reinterpret_cast<void(THISCALL ***)(void *, float)>(w))[0x34 / 4](w, y); }
void FEWidget_Draw(void *w) { (*reinterpret_cast<void(THISCALL ***)(void *)>(w))[0x0C / 4](w); }
void FEPrintfInts3(FEObject *o, const char *f, int32_t a, int32_t b, int32_t c) { reinterpret_cast<void(__cdecl *)(FEObject *, const char *, ...)>(0x537BE0)(o, f, a, b, c); }
void FEPrintfInt(FEObject *o, const char *f, int32_t v) { reinterpret_cast<void(__cdecl *)(FEObject *, const char *, ...)>(0x537BE0)(o, f, v); }
uint32_t FEWidget_DetermineSoundTrigger(void *w) { return (*reinterpret_cast<uint32_t(THISCALL ***)(void *)>(w))[0x3C / 4](w); }
void FEWidget_Act(void *w, const char *p, uint32_t m) { (*reinterpret_cast<void(THISCALL ***)(void *, const char *, uint32_t)>(w))[0x04 / 4](w, p, m); }
uint32_t FEWidget_DetermineSpecialSound(void *w, uint32_t m, uint32_t e) { return (*reinterpret_cast<uint32_t(THISCALL ***)(void *, uint32_t, uint32_t)>(w))[0x38 / 4](w, m, e); }
void FEWidget_BlinkArrows(void *w, uint32_t m) { (*reinterpret_cast<void(THISCALL ***)(void *, uint32_t)>(w))[0x40 / 4](w, m); }
void MakeFrontendSound(uint32_t event, uint32_t trigger)
{
    const int32_t idx = reinterpret_cast<const int8_t *>(0x837FE0)[event * 0x1B + trigger];
    if (idx != -1) reinterpret_cast<void(THISCALL *)(void *, int32_t)>(0x458520)(*reinterpret_cast<void **>(0x82B884), idx);
}
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
::SlotPool **FERenderObjectPool() { return reinterpret_cast<::SlotPool **>(0x83847C); }
::SlotPool **FERenderEPolyPool() { return reinterpret_cast<::SlotPool **>(0x838480); }
::SlotPool **FERenderEPolyOverflowPool() { return reinterpret_cast<::SlotPool **>(0x838484); }
::FERenderBucket *FERenderBuckets() { return reinterpret_cast<::FERenderBucket *>(0x8639A0); }
void *eGetView(int32_t i) { return reinterpret_cast<void *>(0x832DE0 + 0x70 * i); }
void eView_Render(void *v, ::ePoly *p, void *t, int32_t f) { reinterpret_cast<void(THISCALL *)(void *, ::ePoly *, void *, int32_t)>(0x5C7EA0)(v, p, t, f); }
const int32_t *FERenderStatsMode() { return *reinterpret_cast<const int32_t *const *>(0x8900D8); }
uint32_t *FERenderStats() { return reinterpret_cast<uint32_t *>(0x837D20); }
void *cFEngRenderVTable() { return reinterpret_cast<void *>(0x79BBA0); }
int32_t CurrentLanguage() { return *reinterpret_cast<const int32_t *>(0x7F70D0); }
int32_t MovieActive() { return *reinterpret_cast<const int32_t *>(0x8383AC); }
const uint8_t *MoviePlayer() { return *reinterpret_cast<const uint8_t *const *>(0x8383A4); }
void *MovieTexture() { return reinterpret_cast<void *>(0x8385C0); }
void FEngFont_RenderString(void *f, const int32_t *c, const uint16_t *t, ::FEObject *o, const ::bMatrix4 *m, ::FERenderObject *r, void *i)
{
    reinterpret_cast<void(THISCALL *)(void *, const int32_t *, const uint16_t *, ::FEObject *, const ::bMatrix4 *, ::FERenderObject *, void *)>(0x51C110)(f, c, t, o, m, r, i);
}
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
void *RealFont_Create(const void *d) { return reinterpret_cast<void *(__cdecl *)(const void *)>(0x6F4690)(d); }
void RealFont_Destroy(void *f) { reinterpret_cast<void(__cdecl *)(void *)>(0x6F46C0)(f); }
void *FixupTextureInfoNull(void *t, uint32_t h, uint32_t a, uint32_t b) { return reinterpret_cast<void *(__cdecl *)(void *, uint32_t, uint32_t, uint32_t)>(0x4902D0)(t, h, a, b); }
void *FEGameInterfaceVTable() { return reinterpret_cast<void *>(0x79BCD8); }
void *FEObjectInitializerVTable() { return reinterpret_cast<void *>(0x79AAEC); }
void *FEngMovieStarterVTable() { return reinterpret_cast<void *>(0x79AB84); }
void *FEngMovieStopperVTable() { return reinterpret_cast<void *>(0x79AB94); }
void *FEDirtyMakerVTable() { return reinterpret_cast<void *>(0x79ABF0); }
::SlotPool **LevelSorterSlotsPtr() { return reinterpret_cast<::SlotPool **>(0x8384B0); }
int32_t InsideScreenConstructor() { return *reinterpret_cast<const int32_t *>(0x8384D0); }
int32_t GameFlowState() { return *reinterpret_cast<const int32_t *>(0x8654A4); }
float WorldTimeElapsed() { return *reinterpret_cast<const float *>(0x865154); }
float RealTimeElapsed() { return *reinterpret_cast<const float *>(0x865198); }
uint32_t *LastKnownJoystickTypes() { return reinterpret_cast<uint32_t *>(0x7F9414); }
uint8_t GetJoystickType(int32_t p) { return reinterpret_cast<uint8_t(__cdecl *)(int32_t)>(0x571AC0)(p); }
void *MemoryCardInstance() { return *reinterpret_cast<void **>(0x836498); }
void MemoryCard_Tick(void *c, int32_t ms) { reinterpret_cast<void(THISCALL *)(void *, int32_t)>(0x4F4A20)(c, ms); }
void MovieUpdate() { reinterpret_cast<void(__cdecl *)()>(0x5660F0)(); }
void MovieStop() { reinterpret_cast<void(__cdecl *)()>(0x511BC0)(); }
int32_t *SummonChyron() { return reinterpret_cast<int32_t *>(0x8384BC); }
int32_t *StopChyron() { return reinterpret_cast<int32_t *>(0x8384C0); }
void FakeCallFromSound() { reinterpret_cast<void(__cdecl *)()>(0x4ACA40)(); }
void CurrentSongStopped(void *s) { reinterpret_cast<void(__cdecl *)(void *)>(0x4AC9B0)(s); }
int32_t GetMovieNameEnum(const char *n) { return reinterpret_cast<int32_t(__cdecl *)(const char *)>(0x51C580)(n); }
void CalculateMovieFilename(char *o, int32_t s, const char *n) { reinterpret_cast<void(__cdecl *)(char *, int32_t, const char *)>(0x51C740)(o, s, n); }
bool FILE_exists(const char *p) { return reinterpret_cast<uint8_t(__cdecl *)(const char *)>(0x6F909D)(p) != 0; }
void MovieStart(const char *p, bool l, int32_t w) { reinterpret_cast<void(__cdecl *)(const char *, uint32_t, int32_t)>(0x566020)(p, l, w); }
void FEngine_InitObjects(::FEngine *e) { reinterpret_cast<void(THISCALL *)(void *)>(0x51A7E0)(e); }
int32_t *DiscErrorOccurred() { return reinterpret_cast<int32_t *>(0x864F2C); }
const char **FEngPleaseRenderSinglePackage() { return reinterpret_cast<const char **>(0x8384B4); }
const uint8_t *MouseGlobals() { return reinterpret_cast<const uint8_t *>(0x8763D4); }
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

// ---- TuningSlidersMenu ----
void *TuningSlidersMenuVTable() { return reinterpret_cast<void *>(0x79D360); }
void HelpBar_SetFlag6C(uint8_t v) { (*reinterpret_cast<uint8_t **>(0x8384D8))[0x6C] = v; }
void HelpBar_542990(bool on) { reinterpret_cast<void(THISCALL *)(void *, uint32_t)>(0x542990)(*reinterpret_cast<void **>(0x8384D8), on); }
int32_t TuningCategory() { return *reinterpret_cast<int32_t *>(0x83A648); }
int32_t FEngAllPortsFlag() { return *reinterpret_cast<int32_t *>(0x83A644); }
int32_t FEngActivePort() { return *reinterpret_cast<int32_t *>(0x83A7A0); }
bool TuningAcceptBlocked() { return *reinterpret_cast<uint8_t *>(0x83A7A4) != 0; }
bool SpeedUnitsMetric() { return *reinterpret_cast<uint8_t *>(0x83AA1A) == 1; }
const char *TuningSlidersScreenName() { return *reinterpret_cast<const char **>(0x7FA220); }
int32_t *ScreenLastButtonSlot(uint32_t h)
{
    auto *d = reinterpret_cast<uint8_t *(__cdecl *)(uint32_t)>(0x50B840)(h);
    return d ? reinterpret_cast<int32_t *>(d + 0x10) : nullptr;
}
void FEngPrintfInt(const char *pkg, uint32_t h, const char *f, int32_t v) { reinterpret_cast<void(__cdecl *)(const char *, uint32_t, const char *, ...)>(0x537B80)(pkg, h, f, v); }
void TuneManager_ResetToDefault(int32_t t) { reinterpret_cast<void(THISCALL *)(void *, int32_t)>(0x50F9B0)(reinterpret_cast<void *>(0x839C40), t); }
void TuneManager_RebuildTheRideInfo() { reinterpret_cast<void(THISCALL *)(void *)>(0x50FDA0)(reinterpret_cast<void *>(0x839C40)); }
void TuneManager_RebuildPlayersRideInfo() { reinterpret_cast<void(THISCALL *)(void *)>(0x50FDD0)(reinterpret_cast<void *>(0x839C40)); }
void BuildPlayerCarPhysicsInfo()
{
    uint8_t *player = reinterpret_cast<uint8_t **>(0x8900AC)[*reinterpret_cast<int32_t *>(0x83A64C)];
    reinterpret_cast<void(THISCALL *)(void *, uint32_t)>(0x5F6C20)(*reinterpret_cast<void **>(player + 4), 1);
}
void RaceCoordinator_RequestUnPause(int32_t v) { reinterpret_cast<void(THISCALL *)(void *, int32_t)>(0x5F4240)(*reinterpret_cast<void **>(0x8900D8), v); }
void DialogInterface_ShowOk(const char *pkg, const char *f, uint32_t h) { reinterpret_cast<void(__cdecl *)(const char *, const char *, ...)>(0x558430)(pkg, f, h); }
// ---- frontend frame entry points ----
bool DrawFEng() { return *reinterpret_cast<const int32_t *>(0x7F91C4) != 0; }
void DrawFEGraphs() { reinterpret_cast<void(__cdecl *)()>(0x510090)(); }
bool MouseCursorEnabled() { return *reinterpret_cast<const uint8_t *>(0x7FA29C) != 0; }
void DrawMouseCursor(int32_t x, int32_t y) { reinterpret_cast<void(__cdecl *)(int32_t, int32_t)>(0x50B460)(x, y); }
// ---- UIWheelOptionsMenu ----
void *UIWheelOptionsMenuVTable() { return reinterpret_cast<void *>(0x795E4C); }
void *WCOWheelRangeVTable() { return reinterpret_cast<void *>(0x791E40); }
void *WCOStrengthOfForceVTable() { return reinterpret_cast<void *>(0x791E90); }
void *WCOOverallForceStrengthVTable() { return reinterpret_cast<void *>(0x791EE0); }
int32_t PlayerToEditForOptions() { return *reinterpret_cast<const int32_t *>(0x836364); }
uint8_t *PlayerControllerOptions(int32_t p) { return reinterpret_cast<uint8_t *>(0x83AA64 + p * 0x28); }
float FEWidget_GetValue(void *w) { return (*reinterpret_cast<float(THISCALL ***)(void *)>(w))[0x48 / 4](w); }
// ---- ChooseBrandsScreen / customization ----
void *BrandScrollerThingVTable() { return reinterpret_cast<void *>(0x79B6A0); }
void *ChooseBrandsScreenVTable() { return reinterpret_cast<void *>(0x79CACC); }
void FEPrintfFormat(FEObject *o, const char *f) { reinterpret_cast<void(__cdecl *)(FEObject *, const char *, ...)>(0x537BE0)(o, f); }
const char *BrandDisplayName(uint32_t b) { return reinterpret_cast<const char *(__cdecl *)(uint32_t)>(0x4FFF00)(b); }
const char *BrandStringFromHash(uint32_t b) { return reinterpret_cast<const char *(__cdecl *)(uint32_t)>(0x4FFF40)(b); }
uint32_t FEngHashStringArg(const char *f, const char *s) { return reinterpret_cast<uint32_t(__cdecl *)(const char *, ...)>(0x50B990)(f, s); }
uint32_t *ChosenBrands() { return reinterpret_cast<uint32_t *>(0x8389B4); }
bool BrowsingIndividualParts() { return *reinterpret_cast<const uint8_t *>(0x83855D) != 0; }
const uint8_t *SelectedPerfPackage() { return *reinterpret_cast<const uint8_t *const *>(0x839BB4); }
const uint8_t *SelectedPerfPart() { return *reinterpret_cast<const uint8_t *const *>(0x839BB0); }
void CCM_BuyPerfPackage(const uint8_t *p) { reinterpret_cast<void(THISCALL *)(void *, const uint8_t *)>(0x54FE50)(reinterpret_cast<void *>(0x838980), p); }
void CCM_InstallPerfPackage(const uint8_t *p) { reinterpret_cast<void(THISCALL *)(void *, const uint8_t *, void *)>(0x521340)(reinterpret_cast<void *>(0x838980), p, nullptr); }
void CCM_BuyPerfPart(const uint8_t *p) { reinterpret_cast<void(THISCALL *)(void *, const uint8_t *)>(0x521B00)(reinterpret_cast<void *>(0x838980), p); }
void CCM_InstallPerfPart(const uint8_t *p) { reinterpret_cast<void(THISCALL *)(void *, const uint8_t *, void *)>(0x5213D0)(reinterpret_cast<void *>(0x838980), p, nullptr); }
void CustomizationScreenManager_ChangeToNextScreen()
{
    void *m = reinterpret_cast<void *(__cdecl *)()>(0x54F980)();
    reinterpret_cast<void(THISCALL *)(void *)>(0x55BD60)(m);
}
// ---- option widgets ----
uint8_t *AudioSettings() { return reinterpret_cast<uint8_t *>(0x83AA28); }
void EAXSound_UpdateAudioSettings() { reinterpret_cast<void(THISCALL *)(void *, void *)>(0x469050)(*reinterpret_cast<void **>(0x82B884), reinterpret_cast<void *>(0x83AA28)); }
uint8_t *GameByte(uint32_t a) { return reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(a)); }
void *JumpCamObject() { return *reinterpret_cast<void **>(0x8650B8); }
void JumpCam_Set(int32_t v) { reinterpret_cast<void(THISCALL *)(void *, int32_t, int32_t, int32_t, int32_t)>(0x578EF0)(JumpCamObject(), 3, 0, v, 1); }
int32_t FtoL(float v) { return static_cast<int32_t>(v); }
bool AutoSaveSecondStateAllowed() { return reinterpret_cast<uint8_t(THISCALL *)(void *)>(0x502FE0)(reinterpret_cast<void *>(0x83A9E0)) != 0; }
void ApplyDetailLevel() { reinterpret_cast<void(__cdecl *)()>(0x5BE9F0)(); }
void DialogInterface_ShowTwoButtons(const char *p, const char *dlg, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f, uint32_t g)
{
    reinterpret_cast<void(__cdecl *)(const char *, const char *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)>(0x558790)(p, dlg, a, b, c, d, e, f, g);
}
const char *TrailerNameSSX() { return reinterpret_cast<const char *>(0x7944C0); }
void SetTrailerName(const char *n) { *reinterpret_cast<const char **>(0x7F4480) = n; }
// ---- UIOptionsScreen ----
void *UIOptionsScreenVTable() { return reinterpret_cast<void *>(0x795E28); }
void *GameVTable(uint32_t a) { return reinterpret_cast<void *>(static_cast<uintptr_t>(a)); }
int32_t OptionsPage() { return *reinterpret_cast<const int32_t *>(0x83AA04); }
void FEngPrintfByName(const char *p, const char *o, const char *f) { reinterpret_cast<void(__cdecl *)(const char *, const char *, const char *, ...)>(0x542600)(p, o, f); }
bool ForceFeedbackAvailable() { return reinterpret_cast<uint8_t(__cdecl *)()>(0x5B8240)() != 0; }
bool WheelPluggedIn(int32_t p) { return reinterpret_cast<uint8_t(__cdecl *)(int32_t)>(0x5C1000)(reinterpret_cast<const int32_t *>(0x83A9D8)[p]) != 0; }
void FEWidget_Enable(void *w) { (*reinterpret_cast<void(THISCALL ***)(void *)>(w))[0x1C / 4](w); }
void ResetVideoDefaults() { reinterpret_cast<void(__cdecl *)()>(0x5CEC90)(); }
void ApplyVideoSettings() { reinterpret_cast<void(__cdecl *)()>(0x5BEEA0)(); }
int32_t RaceCoordinatorPausePlayer() { return *reinterpret_cast<const int32_t *>(*reinterpret_cast<const uint8_t *const *>(0x8900D8) + 0x20); }
int32_t PlayerId(int32_t n) { const uint8_t *p = reinterpret_cast<const uint8_t *const *>(0x8900AC)[n]; return p ? *reinterpret_cast<const int32_t *>(p + 8) : -1; }
void JumpCamObject_Call578EF0(int32_t a, int32_t b, int32_t c, int32_t d) { reinterpret_cast<void(THISCALL *)(void *, int32_t, int32_t, int32_t, int32_t)>(0x578EF0)(JumpCamObject(), a, b, c, d); }
void Player_ChangeCamera(int32_t n, int32_t dir) { reinterpret_cast<void(THISCALL *)(void *, int32_t)>(0x5F2FA0)(reinterpret_cast<void **>(0x8900AC)[n], dir); }
// ---- Icon scrollers ----
void *ThumbnailNodeVTable() { return reinterpret_cast<void *>(0x79AFDC); }
void *IconOptionVTable() { return reinterpret_cast<void *>(0x79B4D8); }
void *CPPaintOptionVTable() { return reinterpret_cast<void *>(0x79B604); }
void *IconPanelVTable() { return reinterpret_cast<void *>(0x795CBC); }
void *IconScrollerVTable() { return reinterpret_cast<void *>(0x79C0F4); }
void *IconScrollerMenuVTable() { return reinterpret_cast<void *>(0x7974D4); }
template <typename F> static F Slot(void *o, int off) { return (*reinterpret_cast<F **>(o))[off / 4]; }
void IconOption_React(void *o, const char *pkg, uint32_t m, void *obj, uint32_t a, uint32_t b) { Slot<void(THISCALL *)(void *, const char *, uint32_t, void *, uint32_t, uint32_t)>(o, 4)(o, pkg, m, obj, a, b); }
void *IconPanel_AddOption(void *p, void *o) { return Slot<void *(THISCALL *)(void *, void *)>(p, 8)(p, o); }
void IconPanel_Update(void *p) { Slot<void(THISCALL *)(void *)>(p, 4)(p); }
void IconPanel_Act(void *p, uint32_t m, void *obj, uint32_t a, uint32_t b) { Slot<void(THISCALL *)(void *, uint32_t, void *, uint32_t, uint32_t)>(p, 0xC)(p, m, obj, a, b); }
bool IconPanel_SetSelectionOption(void *p, void *o) { return (Slot<uint32_t(THISCALL *)(void *, void *)>(p, 0x14)(p, o) & 0xFF) != 0; }
bool IconPanel_HandleMouse(void *p) { return (Slot<uint32_t(THISCALL *)(void *)>(p, 0x18)(p) & 0xFF) != 0; }
void IconScrollerMenu_RefreshHeader(void *m) { Slot<void(THISCALL *)(void *)>(m, 0x10)(m); }
void SplineSeek(void *c, float dt, float a, float b) { reinterpret_cast<void(__cdecl *)(void *, float, float, float)>(0x574E30)(c, dt, a, b); }
void SetIconScrollTime(int32_t t) { *reinterpret_cast<int32_t *>(0x8641A0) = t; }
// ---- UIOptionsMain / UIPauseOptionsMain ----
void *UIOptionsMainVTable() { return reinterpret_cast<void *>(0x797794); }
void *UIPauseOptionsMainVTable() { return reinterpret_cast<void *>(0x797CCC); }
void IconScrollerMenu_AddOption(void *m, void *o) { Slot<void(THISCALL *)(void *, void *)>(m, 0x18)(m, o); }
void IconScroller_SetInitialPos(void *s, int32_t i) { Slot<void(THISCALL *)(void *, int32_t)>(s, 0x20)(s, i); }
void SetOptionsPage(int32_t p) { *reinterpret_cast<int32_t *>(0x83AA04) = p; }
void SetPlayerToEditForOptions(int32_t p) { *reinterpret_cast<int32_t *>(0x836364) = p; }
bool OptionsDB_IsDirty() { return reinterpret_cast<uint8_t(THISCALL *)(void *)>(0x52A4C0)(reinterpret_cast<void *>(0x83A9D0)) != 0; }
void OptionsDB_DeallocBackup() { reinterpret_cast<void(THISCALL *)(void *)>(0x5032C0)(reinterpret_cast<void *>(0x83A9D0)); }
const char *OptionsReturnScreen() { return *reinterpret_cast<const char *const *>(0x83635C); }
void EnterMemcard(const char *p, const char *to, uint32_t op, uint32_t a, uint32_t b, uint32_t c, uint32_t d) { reinterpret_cast<void(__cdecl *)(const char *, const char *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)>(0x4A3100)(p, to, op, a, b, c, d); }
// ---- UIPauseOptions ----
void *UIPauseOptionsVTable() { return reinterpret_cast<void *>(0x796504); }
bool IsCareerFreeRoam() { return reinterpret_cast<uint8_t(THISCALL *)(void *)>(0x500B60)(reinterpret_cast<void *>(0x85AD40)) != 0; }
int32_t QuickRaceNumPlayers() { return reinterpret_cast<int32_t(THISCALL *)(void *)>(0x5026F0)(reinterpret_cast<void *>(0x83AAB4)); }
bool IsDragRace() { return *reinterpret_cast<const uint8_t *>(0x89E7D8) != 0 || *reinterpret_cast<const int32_t *>(0x890108) != 0; }
bool IsInPerformanceTuning() { return *reinterpret_cast<const uint8_t *>(0x89E7E6) != 0; }
}  // namespace bridge
