// FEng package (a loaded menu: objects, scripts, ...). Original: zFe2.cpp. Only what the rewritten members use.
#pragma once
#include <cstdint>
struct FEObject;
struct FEScript;

struct FEPackage {
    uint8_t _unknown00[0x14];
    uint8_t mbFastInterp;            // +0x14 use FEKeyInterpFast (done-track skipping + dirty-flag tracking)

    // GC 801399F8 / PC 541DB0: evaluate the script's tracks at its current time into the object's data.
    void UpdateObjectTracks(FEObject *obj, FEScript *script) const;
};
