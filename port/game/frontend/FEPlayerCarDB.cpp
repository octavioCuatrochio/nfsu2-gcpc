#include "FEPlayerCarDB.h"
#include <cstring>
#include "bridge/Bridge.h"

namespace {
uint32_t U32(const uint8_t *r, size_t off) { uint32_t v; std::memcpy(&v, r + off, 4); return v; }
void W32(uint8_t *r, size_t off, uint32_t v) { std::memcpy(r + off, &v, 4); }
}  // namespace

uint8_t *FEPlayerCarDB::GetCarRecordByHandle(uint32_t h)
{
    for (uint8_t *r : mRecords)
        if (r && Handle(r) == h) return r;
    return nullptr;
}

int32_t FEPlayerCarDB::GetCarRecordIndex(const uint8_t *rec) const
{
    for (int32_t i = 0; i < 0x5B; ++i)
        if (mRecords[i] && mRecords[i] == rec) return i;
    return -1;
}

uint8_t *FEPlayerCarDB::GetCarFiltered(uint32_t filter, const uint8_t *after)
{
    for (int32_t i = GetCarRecordIndex(after) + 1; i < 0x5B; ++i) {
        const uint8_t *r = mRecords[i];
        if (U32(r, 0x10) && (U32(r, 0x14) & filter)) return mRecords[i];
    }
    return nullptr;
}

bool FEPlayerCarDB::IsCarSponsorCar(uint32_t h) const
{
    for (const auto &r : mSponsor)
        if (Handle(r) == h) return true;
    return false;
}

uint8_t *FEPlayerCarDB::GetTunedCarByHandle(uint32_t h)
{
    for (auto &r : mTuned)
        if (Handle(r) && Handle(r) == h) return r;
    for (int32_t i = 0; i < mNumCareer; ++i)
        if (Handle(mCareer[i]) == h) return mCareer[i];
    return nullptr;
}

uint8_t *FEPlayerCarDB::GetOnlineCarByHandle(uint32_t h)
{
    for (auto &r : mOnline)
        if (Handle(r) && Handle(r) == h) return r;
    return nullptr;
}

uint8_t *FEPlayerCarDB::GetCareerCarByHandle(uint32_t h)
{
    for (int32_t i = 0; i < mNumCareer; ++i)
        if (Handle(mCareer[i]) == h) return mCareer[i];
    return nullptr;
}

uint8_t *FEPlayerCarDB::GetStockCarByHandle(uint32_t h)
{
    for (int32_t i = 0; i < mNumStock; ++i)
        if (Handle(mStock[i]) == h) return mStock[i];
    return nullptr;
}

uint8_t *FEPlayerCarDB::FindEmptyTunedCarSlot()
{
    for (auto &r : mTuned)
        if (Handle(r) == 0) return r;
    return nullptr;
}

void FEPlayerCarDB::ResetPointers()
{
    for (auto &p : mRecords) p = nullptr;
    int32_t k = 0;
    for (auto &r : mStock) mRecords[k++] = r;
    for (auto &r : mTuned) mRecords[k++] = r;
    for (auto &r : mCareer) mRecords[k++] = r;
    for (auto &r : mSponsor) mRecords[k++] = r;
    for (auto &r : mOnline) mRecords[k++] = r;
}

void FEPlayerCarDB::DefaultSponsorCars()
{
    mNumSponsor = 0;
    for (auto &r : mSponsor) { W32(r, 4, 0); W32(r, 8, 0); W32(r, 0xC, 0); W32(r, 0x10, 0); W32(r, 0x14, 8); }
    for (int32_t i = 0; i < 8; ++i) AddSponsorCar(bridge::SponsorName(i));
}

void FEPlayerCarDB::DefaultCareerCars()
{
    mNumCareer = 0;
    for (int32_t i = 0; i < 5; ++i) {
        uint8_t *r = mCareer[i];
        bridge::CareerCarData_Clear(r + 0x434);
        W32(r, 8, bridge::FEngHashStringInt("CAREER_SLOT_%d", i));
        W32(r, 4, 0); W32(r, 0xC, 0); W32(r, 0x10, 0); W32(r, 0x14, 4);
        W32(r, 0x10, 1);
        r[0x7F4] = 1;
        r[0x7F5] = 0;
        ++mNumCareer;
    }
}

void FEPlayerCarDB::DefaultStockCars(int32_t)
{
    mNumStock = 0;
    for (auto &r : mStock) { W32(r, 4, 0); W32(r, 8, 0); W32(r, 0xC, 0); W32(r, 0x10, 0); W32(r, 0x14, 1); }
    for (int32_t i = 0; i < 46; ++i) {
        const uint8_t *ct = bridge::CarTypeInfo(i);
        if (!ct || !ct[0x40] || !((ct[0xDA] >> 1) & 1) || mNumStock >= 48) continue;
        const uint32_t kind = U32(ct, 0x844);
        if (kind != 0 && (kind != 2 || !*bridge::GameByte(0x838470))) continue;
        uint8_t *r = mStock[mNumStock];
        W32(r, 8, bridge::FEngHashStringArg("STOCK_%s", reinterpret_cast<const char *>(ct)));
        W32(r, 0x14, 1); W32(r, 0x10, 1); W32(r, 4, 0); W32(r, 0xC, 0);
        W32(r, 0x18, static_cast<uint32_t>(i));
        ++mNumStock;
    }
}

void FEPlayerCarDB::DeleteTunedCar(uint32_t h)
{
    uint8_t *r = GetTunedCarByHandle(h);
    if (!r || GetCareerCarByHandle(h)) return;
    W32(r, 4, 0); W32(r, 8, 0); W32(r, 0xC, 0); W32(r, 0x10, 0); W32(r, 0x14, 2);
    std::memset(r + 0x18, 0, 0x418);
}

namespace {
int32_t TunedIndex(const FEPlayerCarDB &d, const uint8_t *slot) { return d.GetTunedCarSlotIndex(slot); }
}  // namespace

uint8_t *FEPlayerCarDB::AddNewTunedCar(uint32_t h)
{
    if (GetTunedCarByHandle(h)) {
        if (!AbleToAddNewTunedCar()) return nullptr;
        const uint8_t *src = GetTunedCarByHandle(h);
        uint8_t *dst = FindEmptyTunedCarSlot();
        if (!dst) return dst;
        const uint32_t nh = bridge::FEngHashStringInt("TUNED_CAR_%d", TunedIndex(*this, dst));
        W32(dst, 4, 0); W32(dst, 0xC, 0); W32(dst, 8, nh); W32(dst, 0x14, 2); W32(dst, 0x10, 1);
        std::memcpy(dst + 0x18, src + 0x18, 0x418);
        return dst;
    }
    if (!IsCarStock(h) || !AbleToAddNewTunedCar()) return nullptr;
    uint8_t *stock = GetStockCarByHandle(h);
    uint8_t *dst = FindEmptyTunedCarSlot();
    if (dst) bridge::CarConfig_Init(dst, bridge::FEngHashStringInt("TUNED_CAR_%d", TunedIndex(*this, dst)), stock);
    return dst;
}

void FEPlayerCarDB::DefaultOnlineCareerCars()
{
    for (int32_t i = 0; i < 6; ++i) {
        const uint32_t *e = bridge::OnlineCareerCarEntry(i);
        uint8_t *stock = nullptr;
        for (int32_t k = 0; k < mNumStock; ++k)
            if (bridge::CarRecord_GetType(mStock[k]) == e[2]) { stock = mStock[k]; break; }
        uint8_t *r = mOnline[i];
        W32(r, 0x434, e[0]);
        bridge::CarConfig_Init(r, e[1], stock);
        W32(r, 0x14, 0x10);
    }
    mNumOnline = 6;
}

void FEPlayerCarDB::Default(int32_t defaultCarType)
{
    DefaultStockCars(defaultCarType);
    DefaultTunerCars();
    DefaultCareerCars();
    DefaultSponsorCars();
    DefaultOnlineCareerCars();   // (PC: empty debug calls 4022C0 around it)
    ResetPointers();
}

uint8_t *FEPlayerCarDB::SaveToBuffer(uint8_t *p, int32_t size)
{
    int32_t rem = size - 0x480;
    for (auto &r : mStock) { std::memcpy(p, r + 4, 0x18); p += 0x18; }
    W32(p, 0, static_cast<uint32_t>(mNumStock)); p += 4;
    rem -= 4 + 0x53C0;
    for (auto &r : mTuned) { std::memcpy(p, r + 4, 0x430); p += 0x430; }
    for (auto &r : mCareer) {   // FECareerCar save (PC 516620)
        std::memcpy(p, r + 4, 0x430); p += 0x430; rem -= 0x430;
        *p++ = r[0x7F4]; --rem;
        *p++ = r[0x7F5]; --rem;
        p = bridge::CareerCarData_Save(r + 0x434, p, &rem);
    }
    W32(p, 0, static_cast<uint32_t>(mNumCareer)); p += 4;
    for (auto &r : mOnline) { std::memcpy(p, r + 4, 0x434); p += 0x434; }
    W32(p, 0, static_cast<uint32_t>(mNumOnline)); p += 4;
    return p;
}

const uint8_t *FEPlayerCarDB::LoadFromBuffer(const uint8_t *p, int32_t)
{
    uint32_t slot = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this));   // PC passes &[esp+0x18] (= this)
    for (auto &r : mStock) { std::memcpy(r + 4, p, 0x18); p += 0x18; }
    mNumStock = static_cast<int32_t>(U32(p, 0)); p += 4;
    for (auto &r : mTuned) { std::memcpy(r + 4, p, 0x430); p += 0x430; }
    for (auto &r : mCareer) {
        std::memcpy(r + 4, p, 0x430); p += 0x430;
        r[0x7F4] = *p++;
        r[0x7F5] = *p++;
        p = bridge::CareerCarData_Load(r + 0x434, p, &slot);
    }
    mNumCareer = static_cast<int32_t>(U32(p, 0)); p += 4;
    for (auto &r : mOnline) { std::memcpy(r + 4, p, 0x434); p += 0x434; }
    mNumOnline = static_cast<int32_t>(U32(p, 0)); p += 4;
    return p;
}

void FEPlayerCarDB::ChooseCarForSlot(uint8_t *career, const uint8_t *stock)
{
    bridge::CarConfig_Init(career, Handle(career), stock);
    W32(career, 0x14, 4);
    career[0x7F5] = 1;
}

void FEPlayerCarDB::CareerCarChosen(int32_t slot, uint32_t stockHandle)
{
    uint8_t *car = GetCareerCarByHandle(bridge::FEngHashStringInt("CAREER_SLOT_%d", slot));
    ChooseCarForSlot(car, GetStockCarByHandle(stockHandle));
}

void FEPlayerCarDB::CareerCarTrade(uint32_t careerHandle, uint32_t stockHandle)
{
    uint8_t *car = GetCareerCarByHandle(careerHandle);
    bridge::CareerCarData_Clear(car + 0x434);
    ChooseCarForSlot(car, GetStockCarByHandle(stockHandle));
}

void FEPlayerCarDB::AddSponsorCar(const char *name)
{
    const uint32_t nameHash = bridge::FEngHashStringArg(name, nullptr);
    const uint32_t h = bridge::FEngHashStringArg("SPONSOR_%s", name);
    uint8_t *r = mSponsor[mNumSponsor];
    W32(r, 8, h); W32(r, 4, 0); W32(r, 0xC, 0); W32(r, 0x10, 0); W32(r, 0x14, 8);
    W32(r, 0x18, nameHash);
    W32(r, 0x10, 1);
    ++mNumSponsor;
}

void FEPlayerCarDB::DefaultTunerCars()
{
    for (auto &r : mTuned) { W32(r, 4, 0); W32(r, 8, 0); W32(r, 0xC, 0); W32(r, 0x10, 0); W32(r, 0x14, 2); }
}

uint8_t *FEPlayerCarDB::GetStockCarByCarType(uint32_t type)
{
    for (int32_t i = 0; i < mNumStock; ++i)
        if (bridge::CarRecord_GetType(mStock[i]) == type) return mStock[i];
    return nullptr;
}

void FEPlayerCarDB::Construct()
{
    for (auto &r : mStock) bridge::CarRecord_Construct(bridge::CarRecordKind::Stock, r);
    for (auto &r : mTuned) bridge::CarRecord_Construct(bridge::CarRecordKind::Tuned, r);
    for (auto &r : mCareer) bridge::CarRecord_Construct(bridge::CarRecordKind::Career, r);
    for (auto &r : mOnline) bridge::CarRecord_Construct(bridge::CarRecordKind::Online, r);
    for (auto &r : mSponsor) bridge::CarRecord_Construct(bridge::CarRecordKind::Sponsor, r);
    mNumStock = 0;
    mNumCareer = 0;
    mNumSponsor = 0;
    for (auto &p : mRecords) p = nullptr;
}

int32_t FEPlayerCarDB::GetNumCarsForFilter(uint32_t filter, int32_t unlockArg)
{
    int32_t n = 0;
    for (uint8_t *r : mRecords) {
        if (!U32(r, 0x10) || bridge::CarRecord_GetType(r) == 0xFFFFFFFFu) continue;
        const uint32_t f = U32(r, 0x14);
        if (!(f & filter)) continue;
        const bool ok = (f & 4) ? r[0x7F5] != 0 : bridge::UnlockSystem_IsCarUnlocked(unlockArg, Handle(r), this == bridge::ProfileCarDB(1));
        if (ok) ++n;
    }
    return n;
}

void FEPlayerCarDB::UnlockACareerCarSlot()
{
    for (int32_t i = 0; i < mNumCareer; ++i)
        if (mCareer[i][0x7F4]) {
            mCareer[i][0x7F4] = 0;
            return;
        }
}
