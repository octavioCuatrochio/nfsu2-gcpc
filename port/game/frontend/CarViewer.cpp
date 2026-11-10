#include "CarViewer.h"
#include <cstring>
#include <initializer_list>
#include "bridge/Bridge.h"
#include "cFEng.h"
#include "FEPackageManager.h"

namespace {
void *Screen(const char *pkg) { return bridge::TheFEPackageManager()->FindScreen(pkg); }
void W32(uint8_t *p, uint32_t v) { std::memcpy(p, &v, 4); }
}  // namespace

uint8_t *CarViewer::FindWhichScreenToUpdate(int32_t which)
{
    if (FEngIsPackagePushed("GarageMain.fng")) return static_cast<uint8_t *>(Screen("GarageMain.fng"));
    if (!FEngIsPackagePushed("GarageMain_2P.fng")) return nullptr;
    auto *s = static_cast<uint8_t *>(Screen("GarageMain_2P.fng"));
    if (!s) return nullptr;
    uint8_t *child;
    if (which == 0) std::memcpy(&child, s + 0x4C, sizeof child);
    else if (which == 1) std::memcpy(&child, s + 0x50, sizeof child);
    else return nullptr;
    return child;
}

void *CarViewer::FindWhichRideInfoToUpdate(int32_t which) { return which == 1 ? bridge::BottomScreenRide() : bridge::TopOrFullScreenRide(); }
void *CarViewer::GetRideInfo(int32_t which) { return FindWhichRideInfoToUpdate(which); }

void CarViewer::SetCameraParams(int32_t which, float zRot, float tire, const float cam[10])
{
    uint8_t *s = FindWhichScreenToUpdate(which);
    if (!s) return;
    *bridge::GameByte(0x8363FC) = 0;   // AutoRotateCarMode
    bridge::GarageMain_StopLookingAtParts(s, false);
    std::memcpy(s + 0x274, &zRot, 4);
    std::memcpy(bridge::GameInt(0x7F45A4), &tire, 4);   // CarSelectTireSteerAngle
    std::memcpy(s + 0x254, &zRot, 4);                   // CarOrbiter::SetZRotationAndTireAngle (inlined)
    std::memcpy(s + 0x25C, &tire, 4);
    bridge::GarageMain_SetCameraParams(s, cam);
}

void CarViewer::SetLookAtPart(int32_t a, void *part, int32_t which, bool flag)
{
    uint8_t *s = FindWhichScreenToUpdate(which);
    if (!s) return;
    *bridge::GameByte(0x8363FC) = 0;
    bridge::GarageMain_SetLookatPart(s, part, a, flag, true);
}

void CarViewer::StopLookingAtParts(int32_t which, bool flag)
{
    if (uint8_t *s = FindWhichScreenToUpdate(which)) bridge::GarageMain_StopLookingAtParts(s, flag);
}

void CarViewer::SetRideInfo(const void *ride, int32_t reason, int32_t which)
{
    uint8_t *s = FindWhichScreenToUpdate(which);
    void *dst = FindWhichRideInfoToUpdate(which);
    bridge::RideInfo_Assign(dst, ride);
    if (dst == bridge::TopOrFullScreenRide()) *bridge::GameInt(0x83623C) = reason;
    else if (dst == bridge::BottomScreenRide()) *bridge::GameInt(0x836234) = reason;
    if (s) bridge::GarageMain_SetRideInfo(s, dst, reason);
}

void CarViewer::ShowAllCars()
{
    bridge::FEngSendMessageToPackage(0x0C330D44u, "GarageMain.fng");
    bridge::FEngSendMessageToPackage(0x0C330D44u, "GarageMain_2P.fng");
}

void CarViewer::ShowCarScreen()
{
    if (!FEngIsPackagePushed("GarageMain.fng")) FEngPushPackageSpecial(0, "GarageMain.fng", nullptr, 0, 0);
}

void CarViewer::UnshowCarScreen()
{
    if (FEngIsPackagePushed("GarageMain.fng")) FEngPopPackage("GarageMain.fng");
}

void CarViewer::SendScrollDirectionToGarageScreen(int32_t which, uint32_t msg)
{
    W32(FindWhichScreenToUpdate(which) + 0x148, msg != 0xB5971BF1u);
}

void CarViewer::Show3DLock(uint32_t msg)
{
    uint8_t *s = FindWhichScreenToUpdate(0);
    s[0x161] = 1;
    W32(s + 0x148, msg != 0xB5971BF1u);
}

void CarViewer::SpinPlatforms(uint32_t msg)
{
    bridge::FEngSendMessageToPackage(0x0E550F66u, "GarageMain.fng");
    W32(FindWhichScreenToUpdate(0) + 0x148, msg != 0xB5971BF1u);
}

void CarViewer::SetShaperLights(int32_t which, uint32_t hash)
{
    uint8_t *s = FindWhichScreenToUpdate(which);
    if (!s) return;
    const uint32_t v = bridge::ShaperLightsValue(hash);
    for (size_t off : {size_t(0x4C), size_t(0x50)}) {
        uint8_t *c;
        std::memcpy(&c, s + off, sizeof c);
        if (c) W32(c + 0x904, v);
    }
    W32(s + 0x6C, v);
}

void CarViewer::ChangePlatform(int32_t a, int32_t b) { bridge::GarageMain_ChangePlatform(Screen("GarageMain.fng"), a, b); }
