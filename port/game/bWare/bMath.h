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
void bSinCos(float *sinOut, float *cosOut, uint16_t angle);
// atan2(y, x) in 16-bit angle units (0x10000 = 2pi). Tables are game data (bridge).
// GC 800597AC / PC 43C570: byte table (PC 0x7EACB0) indexed by ftol(ratio * -256) (negative -> read ascending).
// Equal magnitudes: 0x2000, or 0 when both are zero (NaN -> 0x2000). Callers use the low 16 bits.
uint16_t bFastATan(float x, float y);
// Random numbers: result from the OLD seed (seed % range), then seed = s ^ (s >> 5) ^ (s << 27) style step
// (t = s ^ 0x1D872B41; u = t ^ (t >> 5); s' = u ^ (u << 27) ^ t). Global seed: game data (bridge, PC 0x7EA624).
uint32_t bRandom(int32_t range, uint32_t *seed);   // GC 800590C4 / PC 43C140: unsigned seed % range
long double bRandom(float range, uint32_t *seed);  // GC 800590F8 / PC 43C170: (seed % 0x7FFFFFFF) * 2^-31 * range (x87)
uint32_t bRandom(int32_t range);                   // GC 80059160 / PC 43C1C0
long double bRandom(float range);                  // GC 80059184 / PC 43C1F0
// GC 800591A8 / PC 43C240: q = a / |b|; (q - floor(q)) * |b| (floor via ftol, -1 when the truncation is above q).
long double bFMod(float a, float b);
// GC 80059210 / PC 43C280: a - (a / |b| * |b| - (a < 0 ? |b| : 0)); note a < 0 and divisible -> |b|, not 0.
int32_t bSignedMod(int32_t a, int32_t b);
// GC 800598A0 / PC 43C650: atan of a 16.16 ratio, piecewise linear over two uint16 tables (PC 0x7EB1C0 below 4.0,
// 0x7EB2C8 below 32.0), a fixed segment below 256.0, 0x3FFF above. Odd: negative input -> negated result.
int32_t bFixATan(int32_t ratio);
// GC 8005996C / PC 43C6F0: integer atan2(y, x) via bFixATan((0x7FFFFFFF / |x| * |y|) >> 16 << 1); |x| <= |y| >> 14 -> 0x4000.
int32_t bFixATan(int32_t x, int32_t y);    // PC 43C350: bSin(ftol(radians * 65536/2pi)) (low 16 bits)
uint16_t bASin(float x);             // GC 800594C8 / PC 43C490: table + slope; |x| >= 1 -> +-0x4000
