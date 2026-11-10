// Physics tuning type -> language hashes (GC _global, zFe2.cpp). Tuning types 0..24 (ePhysicsTuningType).
#pragma once
#include <cstdint>

uint32_t GetNameHashForTuningType(int32_t type);                    // GC 8017D6A8 / PC 50FE90 (__cdecl)
// GC 8017D8D0 / PC 50FFB0 (__cdecl): types 3/7, 10, 17/23/24 have units; unit type 1 = metric (GC eUnitType)
uint32_t GetUnitsHashForTuningType(int32_t type, int32_t unitType);
