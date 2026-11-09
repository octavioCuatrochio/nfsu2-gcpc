// Camera noise (smooth 1D value noise). Original: Camera.cpp (GC zCamera).
#pragma once
#include <cstdint>

// GC 8005FD80 NoiseBase (PC: inlined): integer hash -> 1 - (h & 0x7FFFFFFF) * 2^-30, in [-1, 1].
// GC 8005FDF0 / PC 4411B0: linear interpolation of NoiseBase(floor(x)) and NoiseBase(floor(x) + 1). PC keeps every
// intermediate in x87 (GC rounds to float); floor = ftol(x), minus 1 when that is above x.
long double NoiseInterpolated(float x);
// GC 8005FED0 / PC 441270: 6 octaves, frequency doubling and amplitude halving (each stored as float).
long double Noise(float x);
