// Checks bStringHash against hashes logged from the running PC game (nfsu2-re docs/dumps/hashes.txt:
// "hash<TAB>string<TAB>result<TAB>function"; function 43DB50 is bStringHash).
#include "bWare/bString.h"
#include "frontend/FEHash.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

// bStr* edge cases (behavior of the PC originals, see bString.h).
static int StrFuncFailures()
{
    int bad = 0;
    auto check = [&](bool ok, const char *what) { if (!ok) { ++bad; std::printf("FAIL %s\n", what); } };
    check(bStrLen(nullptr) == 0 && bStrLen("") == 0 && bStrLen("abc") == 3, "bStrLen");
    char buf[16];
    check(bStrCpy(buf, "hey") == buf && !std::strcmp(buf, "hey"), "bStrCpy");
    std::memset(buf, 'x', sizeof buf);
    check(bStrNCpy(buf, "abcdef", 3) == buf && !std::memcmp(buf, "abcx", 4), "bStrNCpy no terminator");
    std::memset(buf, 'x', sizeof buf);
    bStrNCpy(buf, "ab", 8);
    check(!std::memcmp(buf, "ab\0x", 4), "bStrNCpy stops after terminator");
    check(bStrCmp("abc", "abc") == 0 && bStrCmp("abd", "abc") == 1 && bStrCmp("ab", "abc") == -'c', "bStrCmp");
    check(bStrCmp("\xE9", "a") == static_cast<signed char>(0xE9) - 'a', "bStrCmp signed");
    check(bStrICmp("HeLLo", "hello") == 0 && bStrICmp("a", "B") == 'A' - 'B' && bStrICmp("[", "{") == '[' - '{', "bStrICmp");
    check(bStrICmp("\xE9", "\xC9") == 0x20 && bStrICmp("_", "") == '_', "bStrICmp high bytes / empty");
    const char ab[] = "\0ac\0ad\0abc";   // [0] guard so stopping at the start reads a real byte
    const char *ac = ab + 1, *ad = ab + 4, *abc = ab + 7;
    check(bStrNCmp(ac, ad, 9) == 'c' - 'd', "bStrNCmp mismatch at the end -> previous chars");
    check(bStrNCmp(abc, ad, 9) == 1, "bStrNCmp mismatch with more of a -> 1");
    check(bStrNCmp(ac, ad, 1) == 0 && bStrNCmp(ac, ad, 0) == 0, "bStrNCmp n exhausted");
    check(bStrNCmp(ac, abc, 9) == -1 && bStrNCmp(abc, ac, 9) == 1, "bStrNCmp prefix (mismatch c/b advances)");
    check(bStrNCmp("ab", "ab", 9) == 0, "bStrNCmp equal");
    check(bStrNICmp("ABc", "abC", 9) == 0 && bStrNICmp("AC", "ad", 9) == 'C' - 'D' && bStrNICmp("x", "X", 0) == 0, "bStrNICmp");
    char cat[16];
    check(bStrCat(cat, "ab", "cd") == cat && !std::strcmp(cat, "abcd") && !std::strcmp(bStrCat(cat, "", ""), ""), "bStrCat");
    const char *t = "a\xE9" "b";
    check(bStrChr(t, 'b') == t + 2 && bStrChr(t, 0) == t + 3 && bStrChr(t, 'z') == nullptr && bStrChr(nullptr, 'a') == nullptr, "bStrChr");
    check(bStrChr(t, static_cast<signed char>(0xE9)) == t + 1 && bStrChr(t, 0xE9) == nullptr, "bStrChr signed");
    char up[] = "aZ{\xE9z";
    check(bToUpper(up) == up + 5 && !std::strcmp(up, "AZ{\xE9Z"), "bToUpper");
    check(bStrToLong("0x1aF") == 0x1AF && bStrToLong("0x") == 0 && bStrToLong("0xFFFFFFFF") == -1 && bStrToLong("0x12g4") == 0x12, "bStrToLong hex");
    check(bStrToLong("0X10") == 0 && bStrToLong("-0x10") == 0 && bStrToLong("-42z") == -42 && bStrToLong("+7") == 7 && bStrToLong("") == 0, "bStrToLong dec");
    check(bStrToLong("4294967297") == 1, "bStrToLong wraps");
    check(bStrToFloat("12") == 12 && bStrToFloat("-3") == -3 && bStrToFloat("") == 0 && bStrToFloat("1.0") == 1, "bStrToFloat integers");
    float h = static_cast<float>(bStrToFloat("0.5"));
    uint32_t hb; std::memcpy(&hb, &h, 4);
    check(hb == 0x3F000001u, "bStrToFloat fraction +1 ulp");
    check(bStrToFloat("2e3") == 2000 && bStrToFloat("5E-0") == 5 && bStrToFloat("-1e2") == -100, "bStrToFloat exponent");
    check(std::fabs(static_cast<double>(bStrToFloat("1.25e1")) - 12.5) < 1e-5, "bStrToFloat combined");
    return bad;
}

int main(int argc, char **argv)
{
    int strBad = StrFuncFailures();
    const char *path = argc > 1 ? argv[1] : "tests/data/nfsu2re_hashes.txt";
    FILE *f = std::fopen(path, "r");
    if (!f) { std::perror(path); return 2; }
    char line[4096];
    int checked = 0, failed = 0;
    while (std::fgets(line, sizeof line, f)) {
        line[std::strcspn(line, "\r\n")] = 0;
        char *kind = std::strtok(line, "\t");
        if (!kind || std::strcmp(kind, "hash")) continue;
        // the string may be empty, so split by hand
        char *str = kind + std::strlen(kind) + 1;
        char *tab = std::strchr(str, '\t');
        if (!tab) continue;
        *tab = 0;
        char *hex = tab + 1;
        char *fn = std::strchr(hex, '\t');
        if (!fn) continue;
        *fn++ = 0;
        uint32_t want = std::strtoul(hex, nullptr, 16), got;
        if (!std::strcmp(fn, "43DB50")) got = bStringHash(str);
        else if (!std::strcmp(fn, "505450")) got = FEHashUpper(str);
        else continue;
        ++checked;
        if (got != want) {
            if (++failed <= 10) std::printf("MISMATCH \"%s\": got %08X want %08X\n", str, got, want);
        }
    }
    std::fclose(f);
    std::printf("bStringHash/FEHashUpper: %d checked, %d failed\n", checked, failed);
    return failed || !checked || strBad;
}
