#include "TuningUtil.h"

uint32_t GetNameHashForTuningType(int32_t type)
{
    static const uint32_t kNames[25] = {
        0xFE22B6A2u, 0xEFDB2567u, 0xB6D444B4u, 0x0B161EC3u, 0x93B7D1A3u, 0x87C86448u, 0xFF0CBFD5u, 0x534E99E4u,
        0x2430E6BEu, 0x6A71E798u, 0x7D822B64u, 0x7D822B65u, 0x7D822B66u, 0x7D822B67u, 0x7D822B68u, 0x7D822B69u,
        0x55C60293u, 0xFBBDD223u, 0x2308F06Cu, 0x3988FAEFu, 0xE4EEBD70u, 0xD99FC2E4u, 0x21D83E05u, 0xC8B8FBF1u,
        0xFE82FDE0u};
    const uint32_t t = static_cast<uint32_t>(type);   // PC: unsigned compare (negative -> 0)
    return t < 25 ? kNames[t] : 0;
}

uint32_t GetUnitsHashForTuningType(int32_t type, int32_t unitType)
{
    const bool metric = unitType == 1;
    switch (type) {
    case 3: case 7: return metric ? 0x5E1F61EEu : 0x6382ECE0u;
    case 10: return metric ? 0xCBBC8BF8u : 0x8569AB44u;
    case 17: case 23: case 24: return metric ? 0x5B81D613u : 0xACB8DFF3u;
    default: return 0;
    }
}
