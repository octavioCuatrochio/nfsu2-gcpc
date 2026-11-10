// GC CTextScroller (zFe2.cpp): word-wrapped wide text shown in N string objects ("<name>1".."<name>N"), scrolled
// line by line (messages 0x911C0A4B / 0x72619778, PC: mouse wheel + scroll bar).
#pragma once
#include <cstddef>
#include <cstdint>

struct FEngFont;
struct FEScrollBar;

struct CTextScroller {               // PC 0x54
    void *mpOwner;                   // +0x00 screen (owner +4 = package name)
    FEngFont *mpFont;                // +0x04
    FEScrollBar *mpScrollBar;        // +0x08
    char mName[0x20];                // +0x0C printf format of the line objects ("%d" appended by the caller)
    int32_t mWidth;                  // +0x2C wrap width + 16
    int32_t mVisible;                // +0x30 lines shown
    int32_t mCount;                  // +0x34 lines
    uint16_t **mpLines;              // +0x38 line pointers (start of the buffer)
    uint8_t *mpBuffer;               // +0x3C
    uint32_t mBufferSize;            // +0x40
    uint32_t mUsed;                  // +0x44
    int32_t mPos;                    // +0x48 first shown line
    uint32_t mNextMsg, mPrevMsg;     // +0x4C 0x911C0A4B / 0x72619778

    CTextScroller *Construct();                  // GC 8015CD04 / PC 50DB40
    void Destruct();                             // GC 8015CD44 / PC 50DB70
    bool PreAllocate(uint32_t bytes);            // GC 8015CD90 / PC 50DB80 (ret 4)
    void Initialise(void *owner, int32_t width, int32_t visible, const char *name, FEngFont *font);   // GC 8015CDF0 / PC 50DBB0 (ret 0x14)
    void SetText(const uint16_t *text);          // GC 8015CE3C / PC 5446F0 (ret 4)
    void Scroll(int32_t delta);                  // GC 8015D064 / PC 544880 (ret 4)
    bool HandleNotificationMessage(uint32_t msg);   // GC 8015D150 / PC 5448F0 (ret 4)
    void Display(int32_t pos);                   // GC 8015D1B4 / PC 5397E0 (ret 4)
    void AddLine(const uint16_t *text, int32_t chars);   // GC 8015D28C / PC 50DBF0 (ret 8)
    // GC 8015D338 + 8015D36C / PC 520070 (ret 0x10): wrap [start, end) at mWidth - 16 ("$token$" = icon width,
    // breaks back to the last space); returns the lines; countOnly: no AddLine; *chars += each line + 1.
    int32_t WordWrap(const uint16_t *start, const uint16_t *end, bool countOnly, int32_t *chars);
    static const uint16_t *FindCR(const uint16_t *s);    // GC 8015D580 / PC 50DC70 (ret 4): '\n' or '^'
    static const uint16_t *FindEND(const uint16_t *s);   // GC 8015D5D0 / PC 50DCB0 (ret 4)
    void UpdateScrollBar();                      // GC 8015D5F0 / PC 5398C0
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(CTextScroller) == 0x54 && offsetof(CTextScroller, mWidth) == 0x2C && offsetof(CTextScroller, mPos) == 0x48);
#endif
