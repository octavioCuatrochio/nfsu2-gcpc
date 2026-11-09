#include "FEWideString.h"
#include "bWare/bCrc32.h"
#include "bWare/bWideString.h"
#include "bridge/Bridge.h"

// GC 80153264. Capacity starts as Length() of the null string, i.e. 0.
FEWideString::FEWideString() : mpsString(nullptr), mulBufferLength(0)
{
    mulBufferLength = Length();
}

// GC 801532A4
FEWideString::FEWideString(const FEWideString &other) : mpsString(nullptr), mulBufferLength(0)
{
    *this = other;
    mulBufferLength = Length();
}

// GC 801532EC
FEWideString::~FEWideString()
{
    if (mpsString) bridge::FreeArray(mpsString);
}

// GC 80153338. A null source leaves this string unchanged.
FEWideString &FEWideString::operator=(const FEWideString &other)
{
    if (other.mpsString) {
        mpsString = AllocateString(bWideStrLen(other.mpsString) + 1);
        CopyString(mpsString, other.mpsString);
    }
    return *this;
}

// GC 80153390; on PC inlined into FEString::SetString(short const *) etc. A null source leaves the
// string unchanged.
FEWideString &FEWideString::operator=(const uint16_t *str)
{
    if (str) {
        mpsString = AllocateString(bWideStrLen(str) + 1);
        CopyString(mpsString, str);
    }
    return *this;
}

// PC 532670 (GC 80121198 differs: it calls PackedStringToWideString). Text is unpacked through the
// language's character histogram when loaded, otherwise widened byte by byte. No null check on `packed`.
FEWideString &FEWideString::operator=(const char *packed)
{
    uint16_t buf[1024];
    if (void *hist = bridge::WideCharHistogram())
        bridge::WideCharHistogram_UnpackString(hist, buf, sizeof buf, packed);
    else
        bStrCpy(buf, packed);
    uint32_t needed = static_cast<uint32_t>(bWideStrLen(buf)) + 1;
    if (mulBufferLength < needed) SetLength(needed);
    CopyString(mpsString, buf);
    return *this;
}

// GC 801533E8
uint32_t FEWideString::Length() const
{
    return static_cast<uint32_t>(bWideStrLen(mpsString));
}

// GC 8015340C / PC 51DE90. Grows only if the CURRENT TEXT (not the capacity) is shorter than `length`;
// the new buffer holds length+1 characters and keeps the old text.
void FEWideString::SetLength(uint32_t length)
{
    if (Length() < length) {
        mulBufferLength = length;
        auto *p = static_cast<uint16_t *>(bridge::Alloc(length * 2 + 2));
        CopyString(p, mpsString);
        bridge::FreeArray(mpsString);
        mpsString = p;
    }
}

// GC 8015347C
uint16_t *FEWideString::AllocateString(uint32_t length)
{
    if (mulBufferLength < length) SetLength(length);
    return mpsString;
}

// GC 801534B8; on PC inlined. CRC over the whole capacity (2 bytes per character), including
// whatever follows the terminator in the buffer.
uint32_t FEWideString::GetChecksum() const
{
    if (!mpsString) return 0;
    return bCalculateCrc32(mpsString, static_cast<int>(mulBufferLength) * 2, 0xFFFFFFFFu);
}
