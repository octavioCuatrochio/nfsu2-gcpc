#include "FELocalize.h"
#include "bridge/Bridge.h"

const char *SearchForString(uint32_t hash)
{
    uint32_t count = 0;
    const LanguageStringRecord *t = bridge::LanguageStrings(&count);
    uint32_t lo = 0, hi = count - 1;
    uint32_t mid = hi >> 1;   // PC: (lo + hi) >> 1 with lo = 0
    for (;;) {
        const uint32_t h = t[mid].hash;
        if (h == hash) return t[mid].text;
        if (hi - lo <= 2) {
            if (t[lo].hash == hash) return t[lo].text;
            if (t[hi].hash == hash) return t[hi].text;
        }
        if (mid == lo) return nullptr;
        if (h > hash) hi = mid;
        if (h < hash) lo = mid;
        mid = (hi + lo) >> 1;
    }
}

const char *GetLocalizedString(uint32_t hash)
{
    const char *s = SearchForString(hash);
    return s ? s : SearchForString(0xC01A6F63u);
}
