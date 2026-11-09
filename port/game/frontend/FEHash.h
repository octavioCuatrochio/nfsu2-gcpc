// FEng string hash. Original: zFe2.cpp
#pragma once
#include <cstdint>

// Case-insensitive bStringHash: h = h*33 + c, starting at 0xFFFFFFFF, 'a'..'z' uppercased (bytes >= 0x80
// unchanged). Null -> 0xFFFFFFFF. GC 80141AC8 / PC 505450 (__cdecl)
uint32_t FEHashUpper(const char *str);
