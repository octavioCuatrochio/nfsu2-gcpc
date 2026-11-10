// Game-side registry of loaded FEng packages (screens). Original: zFe2.cpp. PC layouts (GC differs: see fields).
#pragma once
#include <cstddef>
#include <cstdint>
struct FEPackage;

struct FEPkgNode { FEPkgNode *next, *prev; };   // bTNode (list links point at the node, i.e. object + 4)

struct FEPackageData {
    void *vtable;                    // +0x00
    FEPkgNode node;                  // +0x04 FEPackageManager list
    uint8_t *mpData;                 // +0x0C package data (GC +0x08); decompressed copy for chunk 0x30210
    uint32_t *mpChunk;               // +0x10 bChunk header: [0] id (0x30203 raw / 0x30210 LZ), 0x30210: [2] name hash,
                                     //       [3..] LZ data whose [2] (chunk +0x14) is the unpacked size (GC +0x0C)
    void *mpScreen;                  // +0x14 game-side screen object (GC +0x10)
    FEPackage *mpPackage;            // +0x18 loaded FEng package (GC +0x14)
    int16_t mPermanent;              // +0x1C (GC +0x18) kept by CloseAllPackages(0)
    int16_t mVisible;                // +0x1E (GC +0x1A) starts 1
    uint32_t mScreenCreateData;      // +0x20 (GC +0x1C) FindScreenCreateData(name hash)
    uint32_t mRenderInfo;            // +0x24 (GC +0x20) start of the renderer's per-package info (cFEng uses +0x24)
    uint8_t mRenderInfoFlag;         // +0x28 (GC +0x24 int)
    uint8_t _unknown29[0x4C - 0x29];
    uint32_t mPackageArg;            // +0x4C (GC +0x48) package +0x1C (control mask) at activation
    uint32_t _unknown50;             // +0x50 (GC +0x4C) cleared by UnLoader

    static FEPackageData *FromNode(FEPkgNode *n);   // node -> object (PC: - 4); null stays null
    // GC 8014A4E4 / PC 51B8E0 (ret 4). PC skips FindScreenCreateData (leaves 0) when the name hash can't be read.
    void Construct(uint32_t *chunk);
    // GC 8014A590 / PC 51B990 (non-deleting; LZ chunks free the decompressed copy). Scalar deleting dtor PC 52CC20.
    void Destruct();
    FEPackageData *ScalarDeletingDestructor(uint32_t flags);
    // GC 8014A610 / PC 51B9D0 (ret 8): screen = ScreenFactory(hash, pkg, arg) with the FEng load counter raised.
    void Activate(FEPackage *pkg, int32_t arg);
    // GC 8014A67C / PC 51BA60: deletes the screen, unlinks the package, frees the LZ copy.
    void UnActivate();
    // GC 8014A748 / PC 542340: HackClearCache, cFEng::ForceUnloadPackage (if cFEng exists), UnActivate.
    void Close();
    // GC 8014A82C / PC 50B920: raw chunk: FEHashUpper of the name in the data header (magic 0xE76E4546 at +0,
    // 0x64486B50 at +8, version >= 0x20000 at +0x10, name at +0x28); LZ chunk: stored hash; else 0.
    uint32_t GetNameHash() const;
    // GC 8014A7AC (PC: inlined into FEPackageManager::GetPackageData): raw -> mpData; LZ -> a NEW buffer every call
    // (bMalloc + LZDecompress, previous one not freed, like the original); other ids -> null.
    uint8_t *GetDataChunk();
};

struct FEPackageManager {
    uint32_t _unknown00;
    FEPkgNode mList;                 // +0x04 sentinel {head, tail}

    FEPackageData *Sentinel() { return FEPackageData::FromNode(&mList); }   // "object" whose node is the sentinel
    // GC 8014AC50 (PC: inlined): the part after the last '\\'.
    static const char *GetBasePkgName(const char *name);
    // GC 8014ACE4 / PC 51BB70 (ret 4): by FEHashUpper(base name); a hit is moved to the front of the list.
    FEPackageData *FindFEPackageData(const char *name);
    // GC 8014ACB4 / PC 51BB30 (ret 4): by chunk pointer.
    FEPackageData *FindFEPackageData(const void *chunk);
    // GC 8014AD7C / 8014AF5C (PC: inlined): the entry's package / screen, null if not registered.
    FEPackage *FindPackage(const char *name) { FEPackageData *d = FindFEPackageData(name); return d ? d->mpPackage : nullptr; }
    void *FindScreen(const char *name) { FEPackageData *d = FindFEPackageData(name); return d ? d->mpScreen : nullptr; }
    // GC 8014ADB0 / PC 51BC40 (ret 4)
    uint8_t *GetPackageData(const char *name);
    // GC 8014AAA0 / PC 51BAD0: sum of bStringHash(package name) over loaded packages.
    uint32_t GetActiveScreensChecksum();
    // Notifications go to the package's game-side screen (MenuScreen::BaseNotify, bridge) and need both a loaded
    // package and a screen (PC adds the screen check to the inlined FEPackageData::NotificationMessage).
    void SendMessageToPackage(const char *name, uint32_t msg);   // GC 8014A978 / PC 55DC20 (ret 8): obj 0, arg 0
    // GC 8014A9D8 / PC 55DC50 (ret 8): snapshot of at most 32 loaded packages, then BaseNotify(msg, 0, arg, pkg).
    void BroadcastMessage(uint32_t msg, uint32_t arg);
    // GC 8014ABC4 / PC 55DCE0 (ret 0x10): every entry whose package is `pkg`.
    void NotificationMessage(uint32_t msg, void *obj, uint32_t arg, FEPackage *pkg);
    // GC 8014ADE4 / PC 542390 (ret 4): Close every entry (all if `all`, else only non-permanent ones; permanent
    // ones get HackClearCache). Snapshot of at most 256 entries (no bounds check in the original).
    void CloseAllPackages(int32_t all);
    // GC 8014B030 / PC 51BCA0 (ret 8; static on GC, PC passes the global manager in ECX): new entry at the tail.
    // GC byte-swaps the LZ header here (GC-only).
    void Loader(uint32_t *chunk, bool hotChunk);
    // GC 8014B140 / PC 542530 (ret 8): JustUpdateFeng, then Close + unlink + delete the chunk's entry.
    void UnLoader(uint32_t *chunk, bool hotChunk);
    // GC 8014AEA8 / 8014AEE0 / 8014AF24 (PC: inlined or unused)
    void SetPermanent(const char *name, int32_t on) { if (FEPackageData *d = FindFEPackageData(name)) d->mPermanent = static_cast<int16_t>(on); }
    bool GetVisibility(const char *name) { FEPackageData *d = FindFEPackageData(name); return d && d->mVisible != 0; }
    void SetVisibility(const char *name, bool on) { if (FEPackageData *d = FindFEPackageData(name)) d->mVisible = on; }
    // GC 8014AF90 / 8014AFD8 (PC: inlined into cFEng::InitPackage 54E4A0 / cFEng::PackageWillUnload 51C8C0)
    void PackageWasLoaded(FEPackage *pkg, int32_t arg);
    void PackageWillBeUnloaded(FEPackage *pkg);
};

inline FEPackageData *FEPackageData::FromNode(FEPkgNode *n)
{
    return n ? reinterpret_cast<FEPackageData *>(reinterpret_cast<uint8_t *>(n) - offsetof(FEPackageData, node)) : nullptr;
}

// GC 8014F950 / PC 52CEF0: while the global FEng load counter (PC 0x8384D0) is 0 and cFEng exists, search the
// engine's active then idle package lists; otherwise ask the package manager.
FEPackage *FEngFindPackage(const char *name);

// GC 8014B560 / PC 51BD30 (cdecl chunk loader for ids 0x30203/0x30210; returns 1 if handled).
int32_t LoaderFEngPackage(uint32_t *chunk);
// GC 8014B5D0 / PC 542590
int32_t UnloaderFEngPackage(uint32_t *chunk);
// GC 8014F1C8 / PC 558EE0: FEngine::Update(0, 0) when cFEng exists.
void JustUpdateFeng();

// PC 51BB20 (port name; no GC symbol): GetActiveScreensChecksum on the global manager.
uint32_t FEPkgMgr_GetActiveScreensChecksum();

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(offsetof(FEPackageData, mpData) == 0xC && offsetof(FEPackageData, mpPackage) == 0x18 && offsetof(FEPackageManager, mList) == 4);
static_assert(offsetof(FEPackageData, mScreenCreateData) == 0x20 && offsetof(FEPackageData, mPackageArg) == 0x4C && sizeof(FEPackageData) == 0x54);
#endif
