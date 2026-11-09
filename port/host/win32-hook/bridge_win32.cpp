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
uint32_t *BRandomSeed() { return reinterpret_cast<uint32_t *>(0x7EA624); }
const uint8_t *BFastATanTable() { return reinterpret_cast<const uint8_t *>(0x7EACB0); }
const uint16_t *BFixATanTable1() { return reinterpret_cast<const uint16_t *>(0x7EB1C0); }
const uint16_t *BFixATanTable2() { return reinterpret_cast<const uint16_t *>(0x7EB2C8); }
const bASinTableEntry *BASinTable() { return reinterpret_cast<const bASinTableEntry *>(0x7EA628); }
void FECodeListBox_ScrollSelection(void *lb, int32_t dx, int32_t dy) { reinterpret_cast<void(THISCALL *)(void *, int32_t, int32_t)>(0x52BD70)(lb, dx, dy); }
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
