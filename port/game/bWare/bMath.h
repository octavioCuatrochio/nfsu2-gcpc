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

struct bVector2 { float x, y; };
struct bVector3 { float x, y, z; };          // PC code touches 12 bytes
struct bVector4 { float x, y, z, w; };

// GC ? / PC 43CE10: |a - b| (x87, unrounded; sum order dz^2 + dy^2 + dx^2).
long double bDistBetween(const bVector3 *a, const bVector3 *b);
// bNormalize (PC 43CE50 / 43CEB0 / 43CF10 / 43CF90 / 43D010): out = in / |in| (* scale for the 3-argument forms:
// length divided by scale). Length exactly 0 -> (1,0[,0]) (scaled 3D: (scale,0,0)). PC: the 2D forms round the
// length to float before 1/len; the 3D/4D forms keep it in x87. The bVector4 form writes x, y, z only.
void bNormalize(bVector2 *out, const bVector2 *in);
void bNormalize(bVector2 *out, const bVector2 *in, float scale);
void bNormalize(bVector3 *out, const bVector3 *in);
void bNormalize(bVector3 *out, const bVector3 *in, float scale);
void bNormalize(bVector4 *out, const bVector4 *in);
// GC ? / PC 43D090: out = a + b * s (2D).
void bScaleAdd(bVector2 *out, const bVector2 *a, const bVector2 *b, float s);

// GC 8005D544 / 8005D588, PC 43D150 / 43D190: every |a - b| component is NOT greater than eps (NaN counts as equal).
int32_t bEqual(const bVector2 *a, const bVector2 *b, float eps);
int32_t bEqual(const bVector3 *a, const bVector3 *b, float eps);
// GC 8005D5EC / PC 43D1E0: out = a x b (all reads before the stores).
void bCross(bVector3 *out, const bVector3 *a, const bVector3 *b);
// Bounding boxes (2D). PC 43D220 / 43D250: empty box (+-FLT_MAX) / box of one point.
void bInitializeBoundingBox(bVector2 *min, bVector2 *max);
void bInitializeBoundingBox(bVector2 *min, bVector2 *max, const bVector2 *p);
// PC 43D270 / 43D2F0 / 43D380: min = (min < v) ? min : v, max = (max > v) ? max : v per axis (NaN takes v);
// with radius r: v = p -+ r (unrounded for the comparison); with two points: min against p, max against q.
void bExpandBoundingBox(bVector2 *min, bVector2 *max, const bVector2 *p);
void bExpandBoundingBox(bVector2 *min, bVector2 *max, const bVector2 *p, float r);
void bExpandBoundingBox(bVector2 *min, bVector2 *max, const bVector2 *p, const bVector2 *q);

// 4x4 float matrix, row-major (m[row * 4 + col]); 64 bytes, layout shared with game code.
struct bMatrix4 { float m[16]; };
void bIdentity(bMatrix4 *m);                                   // GC ? / PC 43C780
// PC 4013F0 (GC eMulMatrix, "e" = engine/platform layer): out = a x b, each element summed in the original's
// (unrolled) order in x87 and rounded once; result written through a temporary (out may alias a or b).
void eMulMatrix(bMatrix4 *out, const bMatrix4 *a, const bMatrix4 *b);
void bMulMatrix(bMatrix4 *out, const bMatrix4 *a, const bMatrix4 *b);   // GC 80059A8C / PC 43C990: eMulMatrix(out, b, a)
void bTransposeMatrix(bMatrix4 *out, const bMatrix4 *in);
// GC 80059A6C / PC 43C7C0: inverse of an affine matrix (3x3 cofactors / determinant, translation row = -t * R^-1,
// column 3 = 0,0,0,1). Original's x87 order and float temporaries; reads all of `in` first (in may alias out).
// Known difference: singular input (det 0) gives NaNs whose SIGN may differ (NaN-vs-NaN operand choice).
void bInvertMatrix(bMatrix4 *out, const bMatrix4 *in);      // PC 43C9D0 (via a copy: in may alias out)
// GC 80059210 / PC 43C280: a - (a / |b| * |b| - (a < 0 ? |b| : 0)); note a < 0 and divisible -> |b|, not 0.
int32_t bSignedMod(int32_t a, int32_t b);
// GC 800598A0 / PC 43C650: atan of a 16.16 ratio, piecewise linear over two uint16 tables (PC 0x7EB1C0 below 4.0,
// 0x7EB2C8 below 32.0), a fixed segment below 256.0, 0x3FFF above. Odd: negative input -> negated result.
int32_t bFixATan(int32_t ratio);
// GC 8005996C / PC 43C6F0: integer atan2(y, x) via bFixATan((0x7FFFFFFF / |x| * |y|) >> 16 << 1); |x| <= |y| >> 14 -> 0x4000.
int32_t bFixATan(int32_t x, int32_t y);    // PC 43C350: bSin(ftol(radians * 65536/2pi)) (low 16 bits)
uint16_t bASin(float x);             // GC 800594C8 / PC 43C490: table + slope; |x| >= 1 -> +-0x4000
