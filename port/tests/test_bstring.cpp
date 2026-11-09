// Checks bStringHash against hashes logged from the running PC game (nfsu2-re docs/dumps/hashes.txt:
// "hash<TAB>string<TAB>result<TAB>function"; function 43DB50 is bStringHash).
#include "bWare/bString.h"
#include "frontend/FEHash.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

int main(int argc, char **argv)
{
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
    return failed || !checked;
}
