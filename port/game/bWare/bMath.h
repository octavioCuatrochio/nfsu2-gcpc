// bWare trig (fixed-point angles: 0x10000 = 2*pi). Original: zBWare.cpp. PC versions are x87 code; these mirror
// them with long double (same x87 ops on i686) so results are bit-identical.
#pragma once
#include <cstdint>

struct bASinTableEntry {             // PC 0x7EA628 (.data, 12 segments x 16 + 1): hand-rounded slopes, not formula
    uint16_t angle;
    uint16_t _pad;
    float slope;
};

long double bSin(uint16_t angle);    // GC 8005930C (bSin(ushort)) / PC 43C2B0: 9th-order polynomial, returns st0
long double bSinF(float radians);
long double bCos(uint16_t angle);    // PC 43C370: bSin(angle + 0x4000)
// GC 80059370 / PC 43C390: sine and cosine series sharing x^2; even powers x^4, x^6, x^8 are rounded to float
// (stack temporaries), both results multiplied by the quadrant sign and stored as float.
void bSinCos(float *sinOut, float *cosOut, uint16_t angle);    // PC 43C350: bSin(ftol(radians * 65536/2pi)) (low 16 bits)
uint16_t bASin(float x);             // GC 800594C8 / PC 43C490: table + slope; |x| >= 1 -> +-0x4000
