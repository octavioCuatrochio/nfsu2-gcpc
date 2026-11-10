// GC CarViewer (zFe2.cpp): static helpers driving the garage 3D view (GarageMainScreen, still the game's).
// which: 0 = top / full screen, 1 = bottom (GarageMain_2P.fng children +0x4C / +0x50 on PC).
#pragma once
#include <cstdint>

namespace CarViewer {
// GC 800FE99C / PC 4A7830: GarageMain.fng's screen if pushed, else GarageMain_2P.fng's child, else null.
uint8_t *FindWhichScreenToUpdate(int32_t which);
// GC 800FEA2C / PC 4A7890: BottomScreenRide (PC 0x836750) for 1, else TopOrFullScreenRide (0x8371B0).
void *FindWhichRideInfoToUpdate(int32_t which);
void *GetRideInfo(int32_t which);                 // GC 800FECA8 (PC: inlined)
// GC 800FEA4C / PC 4C2070: auto-rotate off (byte 0x8363FC), stop looking at parts, z rotation (+0x274, orbiter
// +0x254) and tire angle (0x7F45A4, orbiter +0x25C), then GarageMainScreen::SetCameraParams (10 floats, 4A5B70).
void SetCameraParams(int32_t which, float zRot, float tire, const float cam[10]);
void SetLookAtPart(int32_t a, void *part, int32_t which, bool flag);   // GC 800FEB40 / PC 4C2100
void StopLookingAtParts(int32_t which, bool flag);                    // GC 800FEB9C / PC 4C2140
// GC 800FEC14 / PC 4D8EC0 (__cdecl; also inlined, e.g. 4DF3EA): copy into the view's RideInfo (413440), loading
// reason top PC 0x83623C / bottom 0x836234, then GarageMainScreen::SetRideInfo (4D8C70) if there is a screen.
void SetRideInfo(const void *ride, int32_t reason, int32_t which);
void ShowAllCars();                               // GC 800FECC8 (PC: inlined): 0x0C330D44 to both garage packages
void ShowCarScreen();                             // GC 800FED0C / PC 4A78B0
void UnshowCarScreen();                           // GC 800FED5C / PC 4A78E0
// GC 800FED9C / PC 4A7900: screen +0x148 = msg != 0xB5971BF1 (no null check). Show3DLock (4A7930) also sets byte
// +0x161 (GC +0x168 int); SpinPlatforms (4A7960) first sends 0x0E550F66 to GarageMain.fng.
void SendScrollDirectionToGarageScreen(int32_t which, uint32_t msg);
void Show3DLock(uint32_t msg);
void SpinPlatforms(uint32_t msg);
// GC 800FEE98 / PC 4A7990: light value (48A2D0(hash)) into both 2P children (+0x904) and the screen (+0x6C).
void SetShaperLights(int32_t which, uint32_t hash);
void ChangePlatform(int32_t a, int32_t b);       // GC 800FEED4 (PC: inlined): GarageMain.fng screen, 4A7180
}  // namespace CarViewer
