// FEng engine object. Original: zFe2.cpp. Only the fields the rewritten members use (PC offsets).
#pragma once
#include <cstddef>
#include <cstdint>
#include "FEMinList.h"
struct FEObject;
struct FEPackage;

// Queued message (0x20 bytes, game heap). GC FEMessageNode.
struct FEMessageNode {
    FEMinNode node;                  // +0x00 vtable PC 0x79AB60
    FEObject *mpObject;              // +0x0C sender object
    FEObject *mpTarget;              // +0x10 target object or special value (-4/-5/-6)
    FEPackage *mpPackage;            // +0x14
    uint32_t mu32MsgId;              // +0x18
    uint32_t mu32Arg;                // +0x1C
};

struct FEngine {
    uint8_t _unknown000;
    uint8_t mbFlag01;                // +0x01 set by message 0x59BED120, cleared by 0x5D4CE32D (target -6)
    uint8_t _unknown002[0xE2];
    FEPackage *mpPackages;           // +0xE4 first active package (packages are linked through their FEMinNode)
    uint8_t _unknown0E8[0x24];
    void *mpUIData;                  // +0x10C game-side handler (vtable: +0x18 SendMessageToGame, +0x54 message hook)
    uint8_t _unknown110[0x4114 - 0x110];
    FEMinList mMessages;             // +0x4114 queued messages
    uint8_t _unknown4124[0x5251 - 0x4124];
    uint8_t mbForwardQueuedMessages; // +0x5251 also hand every queued message to mpUIData (vtable +0x54)

    // GC 80147A54 / PC 51AA20. Like the original, allocation failure crashes (writes through null).
    void QueueMessage(uint32_t msg, FEObject *obj, FEPackage *pkg, FEObject *target, uint32_t arg);
    // GC 80147F50 / PC 541FC0: deliver and delete every queued message (see the target switch in the .cpp).
    void ProcessMessageQueue();
    void ProcessGlobalMessage(FEPackage *pkg, uint32_t msg, uint32_t arg);   // GC 801484AC (PC: inlined)
    // GC 8014840C / PC 5367E0: list boxes first try the scroll messages, then the object's own response.
    void ProcessObjectMessage(FEObject *obj, FEPackage *pkg, uint32_t msg, uint32_t arg);
    bool ProcessListBoxResponses(FEObject *listBox, FEPackage *pkg, uint32_t msg);       // GC 801482B4 / PC 51B000
    bool ProcessCodeListBoxResponses(FEObject *listBox, FEPackage *pkg, uint32_t msg);   // GC 80148360 / PC 52C750
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(FEMessageNode) == 0x20);
static_assert(offsetof(FEngine, mpPackages) == 0xE4 && offsetof(FEngine, mpUIData) == 0x10C && offsetof(FEngine, mMessages) == 0x4114);
static_assert(offsetof(FEngine, mbForwardQueuedMessages) == 0x5251);
#endif
