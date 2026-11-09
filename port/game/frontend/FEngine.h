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
    uint8_t _unknown000[0x10C];
    void *mpUIData;                  // +0x10C game-side handler (vtable: +0x18 SendMessageToGame, +0x54 message hook)
    uint8_t _unknown110[0x4114 - 0x110];
    FEMinList mMessages;             // +0x4114 queued messages
    uint8_t _unknown4124[0x5251 - 0x4124];
    uint8_t mbForwardQueuedMessages; // +0x5251 also hand every queued message to mpUIData (vtable +0x54)

    // GC 80147A54 / PC 51AA20. Like the original, allocation failure crashes (writes through null).
    void QueueMessage(uint32_t msg, FEObject *obj, FEPackage *pkg, FEObject *target, uint32_t arg);
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(FEMessageNode) == 0x20);
static_assert(offsetof(FEngine, mpUIData) == 0x10C && offsetof(FEngine, mMessages) == 0x4114);
static_assert(offsetof(FEngine, mbForwardQueuedMessages) == 0x5251);
#endif
