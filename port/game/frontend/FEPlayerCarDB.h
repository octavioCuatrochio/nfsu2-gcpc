// GC FEPlayerCarDB (zFe2.cpp): a profile's car stable. PC 0x9D44 at UserProfile +0x3B0. Records are game classes
// with vtables (FEStockCar 0x1C, FECarConfig 0x434, FECareerCar 0x7F8, online 0x438, SponsorCarInfo 0x1C); here
// they are raw bytes. Every record: +0x00 vtable, +0x08 handle (0 = free slot), +0x10 active, +0x14 filter bits.
#pragma once
#include <cstddef>
#include <cstdint>

struct FEPlayerCarDB {
    uint8_t mStock[48][0x1C];            // +0x0000
    int32_t mNumStock;                   // +0x0540
    uint8_t mTuned[20][0x434];           // +0x0544
    uint8_t mCareer[5][0x7F8];           // +0x5954
    int32_t mNumCareer;                  // +0x812C
    uint8_t mOnline[6][0x438];           // +0x8130
    int32_t mNumOnline;                  // +0x9A80 (DefaultOnlineCareerCars: 6)
    uint8_t mSponsor[12][0x1C];          // +0x9A84
    int32_t mNumSponsor;                 // +0x9BD4
    uint8_t *mRecords[0x5B];             // +0x9BD8 stock, tuned, career, sponsor, online (ResetPointers)

    static uint32_t Handle(const uint8_t *r) { uint32_t h; __builtin_memcpy(&h, r + 8, 4); return h; }

    uint8_t *GetCarRecordByHandle(uint32_t h);        // GC 801339A0 / PC 503510 (ret 4)
    int32_t GetCarRecordIndex(const uint8_t *r) const;   // GC 801339D8 (PC: inlined): -1 if absent
    // GC 80133A1C / PC 5162D0 (ret 8): next record after `after` (from the start if absent) that is active and
    // matches `filter` (PC: no null check on later table entries).
    uint8_t *GetCarFiltered(uint32_t filter, const uint8_t *after);
    bool IsCarSponsorCar(uint32_t h) const;           // GC 80133C48 / PC 503590 (ret 4)
    // GC 80133CB4 / PC 5035C0 (ret 4): tuned slots, then the career cars.
    uint8_t *GetTunedCarByHandle(uint32_t h);
    uint8_t *GetOnlineCarByHandle(uint32_t h);        // PC 503640 (ret 4; GC IsCarAnOnlineCar 80133D78 = != null)
    bool IsCarAnOnlineCar(uint32_t h) { return GetOnlineCarByHandle(h) != nullptr; }
    uint8_t *GetCareerCarByHandle(uint32_t h);        // GC 80133F50 / PC 503680 (ret 4)
    uint8_t *GetStockCarByHandle(uint32_t h);         // GC 80133C10 (PC: inlined in 516360)
    bool IsCarCareerCar(uint32_t h) { return GetCareerCarByHandle(h) != nullptr; }   // GC 80133BB0 / PC 516340
    bool IsCarStock(uint32_t h) { return GetStockCarByHandle(h) != nullptr; }         // GC 80133BE0 / PC 516360
    bool IsCarATunedCar(uint32_t h) { return GetTunedCarByHandle(h) != nullptr; }     // GC 80133C84 / PC 5163A0
    uint8_t *FindEmptyTunedCarSlot();                 // GC 8013427C / PC 5036C0
    void ResetPointers();                             // GC 80134488 / PC 5036F0
    bool AbleToAddNewTunedCar() { return FindEmptyTunedCarSlot() != nullptr; }   // GC 8013424C / PC 5165E0
    // GC 80133EA8 / PC 5163C0: sponsor slots cleared (filter 8), then the 8 sponsors (names PC 0x7F7C6C): handle
    // FEngHashString("SPONSOR_%s"), +0x18 = FEngHashString(name), active. Count at +0x9BD4.
    void DefaultSponsorCars();
    // GC 801340A4 / PC 516480: 5 career slots: data at +0x434 cleared (PC 503E70), handle "CAREER_SLOT_%d", filter 4,
    // active, bytes +0x7F4 = 1 / +0x7F5 = 0.
    void DefaultCareerCars();
    // GC 80134144 / PC 516500 (ret 4, arg unused): 48 stock slots cleared (filter 1); then every car type (PC table
    // *0x8A1CCC, 0x890 each, 46) with byte +0x40, bit 1 of +0xDA and +0x844 == 0 (or 2 with byte 0x838470) while
    // room: handle "STOCK_%s", filter 1, active, +0x18 = type index.
    void DefaultStockCars(int32_t unused);
    // GC 80133D20 / PC 52A6C0 (ret 4): a tuned (non-career) car's slot cleared (filter 2, data +0x18..+0x430 zero).
    void DeleteTunedCar(uint32_t h);
    // GC 80134314 / PC 52A710 (ret 4): copy of a tuned car, or a new config from a stock car (record init 516B90),
    // into the first free tuned slot with handle "TUNED_CAR_%d" (slot index); null if none / no room.
    uint8_t *AddNewTunedCar(uint32_t h);
    // GC 80134478 / PC 52A7F0: the 6 online career cars from the table PC 0x8641A8 ({+0x434 value, handle, car
    // type}; record init 516B90 with the stock car of that type), filter 0x10; count 6.
    void DefaultOnlineCareerCars();
    // GC 80134560 / PC 52A8A0 (ret 4): stock (arg), tuned slots cleared, career, sponsor, online, ResetPointers.
    void Default(int32_t defaultCarType);
    // GC 801349C8 / PC 5166B0 (ret 8): records without their vtable (stock 0x18, tuned 0x430, career 0x430 + 2 bytes +
    // career data (PC 503F00), online 0x434) and the counts; returns the end. `size` only feeds the career data's
    // remaining-bytes counter.
    uint8_t *SaveToBuffer(uint8_t *buf, int32_t size);
    // GC 80134A9C / PC 516850 (ret 8): the reverse (career data PC 504040, given a pointer to a slot holding `this`,
    // like the original). `size` unused.
    const uint8_t *LoadFromBuffer(const uint8_t *buf, int32_t size);
    // FECareerCar::ChooseCarForSlot (PC: inlined): record init 516B90 (own handle, stock car), filter 4, +0x7F5 = 1.
    static void ChooseCarForSlot(uint8_t *career, const uint8_t *stock);
    // GC 80133F90 / PC 5348E0 (ret 8): career slot "CAREER_SLOT_%d" gets the stock car (no null checks, like the PC).
    void CareerCarChosen(int32_t slot, uint32_t stockHandle);
    // GC 80133FF4 / PC 534950 (ret 8): the career car's part inventory reset (OwnedPartInventory::Default 503E70),
    // then the stock car chosen for it.
    void CareerCarTrade(uint32_t careerHandle, uint32_t stockHandle);
    // GC 80133828 / PC 52A5D0: element ctors (stock, tuned, career, online, sponsor; game classes, bridge), stock /
    // career / sponsor counts 0 (online count untouched), record table cleared.
    void Construct();
    uint8_t *GetStockCarByCarType(uint32_t type);   // GC 80133920 (PC: inlined in 515800): record vslot 1 == type
    int32_t GetTunedCarSlotIndex(const uint8_t *slot) const { return static_cast<int32_t>((slot - mTuned[0]) / 0x434); }   // GC 801342A4
    // GC 80133E28 (PC: inlined into DefaultSponsorCars): next sponsor slot, handle "SPONSOR_%s", +0x18 name hash.
    void AddSponsorCar(const char *name);
    void DefaultTunerCars();                        // GC 80134430 (PC: inlined into Default): tuned slots cleared
    // GC 80133AA8 / PC 534850 (ret 8): active records with a car type (vslot 1 != -1) matching `filter`; career
    // ones (filter bit 4) count when chosen (+0x7F5), others when UnlockSystem::IsCarUnlocked(arg, handle, player 2's
    // DB). No null check on the table entries (like the PC).
    int32_t GetNumCarsForFilter(uint32_t filter, int32_t unlockArg);
    // GC 80134048 (PC: not located as a function; PlayerCareerState inlines it): first locked career slot (+0x7F4)
    // gets unlocked (FECareerCar::UnlockSlot: +0x7F4 = 0).
    void UnlockACareerCarSlot();
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(offsetof(FEPlayerCarDB, mNumStock) == 0x540 && offsetof(FEPlayerCarDB, mCareer) == 0x5954 &&
              offsetof(FEPlayerCarDB, mNumCareer) == 0x812C && offsetof(FEPlayerCarDB, mOnline) == 0x8130 &&
              offsetof(FEPlayerCarDB, mSponsor) == 0x9A84 && offsetof(FEPlayerCarDB, mRecords) == 0x9BD8 &&
              sizeof(FEPlayerCarDB) == 0x9D44);
#endif
