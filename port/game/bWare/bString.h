// bWare string utilities (EA "bWare" core library).
// Original: zBWare.cpp (GameCube Speed.elf), behavior-equivalent rewrite.
#pragma once
#include <cstdint>

// Case-sensitive string hash used for nearly every asset/name lookup in the game.
// hash = hash * 33 + c over the bytes of str, starting from `seed`.
// GC 8005DC10 / PC 43DB80
uint32_t bStringHash(const char *str, uint32_t seed);

// Same with the default seed 0xFFFFFFFF. GC 8005DBE0 / PC 43DB50
uint32_t bStringHash(const char *str);

// --- C-string helpers (GC zBWare.cpp, PC 43DBB0..43DE49, all __cdecl). "Upper" = 'a'..'z' & 0x5F only (signed
// chars: bytes >= 0x80 unchanged). Results are differences of SIGNED chars, like the originals.

int bStrLen(const char *s);                                   // GC 8005DC40 / PC 43DBB0: null -> 0
char *bStrCpy(char *dst, const char *src);                    // GC 8005DC70 / PC 43DBD0: returns dst
// GC 8005DCA4 / PC 43DC00: copies up to n bytes, stops after the terminator (no padding, may not terminate)
char *bStrNCpy(char *dst, const char *src, int n);
int bStrCmp(const char *a, const char *b);                    // GC 8005DCF0 / PC 43DC30
// GC 8005DD28 / PC 43DC60. Quirks kept: n exhausted -> 0; otherwise, at the stop position, a has more -> 1, b has
// more -> -1, else the difference of the previous characters (a mismatch advances both first, so "ac"/"ad" n=9
// gives 'c'-'d'; stopping at the very start reads a[-1]/b[-1] like the original).
int bStrNCmp(const char *a, const char *b, int n);
int bStrICmp(const char *a, const char *b);                   // GC 8005DDB4 / PC 43DCC0
int bStrNICmp(const char *a, const char *b, int n);           // GC 8005DE18 / PC 43DD10: as bStrNCmp, case-folded
char *bStrCat(char *dst, const char *a, const char *b);       // GC 8005DF00 / PC 43DDA0: dst = a + b; returns dst
// PC 43DDF0 (no GC symbol; our name): first position whose (signed) char equals c, c == 0 finds the terminator;
// null string or not found -> null.
char *bStrChr(const char *s, int c);
// GC 8005DF6C / PC 43DE20: in place. GC returns void; PC leaves EAX at the terminator (nothing uses it).
char *bToUpper(char *s);

// GC 8005DFA0 / PC 43DE50. "0x" prefix (lowercase x only): hex digits (any case) until a non-hex char; otherwise
// optional '-'/'+' then decimal digits until a non-digit. 32-bit wraparound, no overflow checks.
int32_t bStrToLong(const char *s);
// GC 8005E0A0 / PC 43DEF0. x87 result returned unrounded in ST0 (callers store it), so the port returns long double;
// evaluated in the original's order at the current x87 precision. PC quirk: a nonzero fraction is rounded to float
// and bumped by one ulp (float bits + 1) before being added. Exponent 'e'/'E' via bStrToLong, applied as repeated
// *10.0f or *0.1f.
long double bStrToFloat(const char *s);
