
#pragma once

#include <Windows.h>
#include <cstdint>
#include <functional>
#include "MemoryUtil.h"
#include "MapleOffsets.h"
#include <class/CItemInfo.h>

#pragma pack(push, 1)
struct EquipStats
{
    int16_t STR   = 0;
    int16_t DEX   = 0;
    int16_t INT   = 0;
    int16_t LUK   = 0;
    int16_t MaxHP = 0;
    int16_t MaxMP = 0;

    int16_t WeaponAttack = 0;
    int16_t MagicAttack  = 0;
    int16_t WeaponDef    = 0;
    int16_t MagicDef     = 0;
    int16_t Accuracy     = 0;
    int16_t Avoid        = 0;
    int16_t Hands        = 0;
    int16_t Speed        = 0;
    int16_t Jump         = 0;

    uint8_t UpgradeAvail = 0;

    template <typename Fn>
    void visit(Fn&& fn, bool includeZero = false) const
    {
        auto emit = [&](const char* name, auto value)
        {
            if (!includeZero && value == 0 && std::string(name) != "UpgradeAvail")
                return;
            fn(name, value);
        };

        emit("STR", STR);
        emit("DEX", DEX);
        emit("INT", INT);
        emit("LUK", LUK);
        emit("MaxHP", MaxHP);
        emit("MaxMP", MaxMP);
        emit("Weapon Att", WeaponAttack);
        emit("Magic Att", MagicAttack);
        emit("Weapon Def", WeaponDef);
        emit("Magic Def", MagicDef);
        emit("Accuracy", Accuracy);
        emit("Avoid", Avoid);
        emit("Hands", Hands);
        emit("Speed", Speed);
        emit("Jump", Jump);
        emit("Upgrade Available", UpgradeAvail); // included even if 0 when includeZero=true
    }

};
#pragma pack(pop)



class GW_ItemSlot
{
public:
    uintptr_t remoteAddress;
    InventoryType type;
    size_t slot;
    GW_ItemSlot() : remoteAddress(0) {}
    explicit GW_ItemSlot(uintptr_t addr) : remoteAddress(addr) {}
    explicit GW_ItemSlot(uintptr_t addr, InventoryType t, size_t s) : remoteAddress(addr), type(t), slot(s) {}


    bool Get( HANDLE hProcess, 
    uint32_t& ID,
    uint16_t& Qty,
    std::string& Name,
    std::string& Desc

    ) const
    {
        if (!hProcess || !remoteAddress)
            return false;

            long _ID = 0;
            if (!MemUtil::ReadTSecTypeLong(hProcess, remoteAddress + g_Maple.GW_ItemSlot.GW_ItemSlot_ID, _ID)) return false;

            
            int32_t checksum = 0;
            int16_t _Qty=0;
            if (!MemUtil::RPM(             hProcess, remoteAddress + g_Maple.GW_ItemSlot.GW_ItemSlot_Qty_CS, checksum))    _Qty=1;
            if (!MemUtil::ZtlSecureFuseS16(hProcess, remoteAddress + g_Maple.GW_ItemSlot.GW_ItemSlot_Qty, checksum, _Qty)) _Qty=1;
            
            ID  = static_cast<uint32_t>(_ID);
            Qty = static_cast<uint16_t>(_Qty);

            CItemInfo::GetItemStringByIdAndKey(hProcess,ID,"name",Name);
            CItemInfo::GetItemStringByIdAndKey(hProcess,ID,"desc",Desc);

        return true;
    }


    bool GetEquipStats(
        HANDLE hProcess,
        EquipStats& outStats
    ) const
    {
        if (!hProcess || !remoteAddress)
            return false;

        const auto& off = g_Maple.GW_ItemSlot; // your offsets struct

        // helper to read secure int16 stat:
        auto ReadStatS16 = [&](uintptr_t statOffset, int16_t& out) -> bool
        {
            int32_t checksum = 0;
            int16_t value    = 0;

            // read checksum
            if (!MemUtil::RPM(
                    hProcess,
                    remoteAddress + statOffset + off.Equip_Stat_CS_Offsets,
                    checksum))
            {
                out = 0;
                return false;
            }

            // fuse/decrypt value
            if (!MemUtil::ZtlSecureFuseS16(
                    hProcess,
                    remoteAddress + statOffset,
                    checksum,
                    value))
            {
                out = 0;
                return false;
            }

            out = value;
            return true;
        };

        // Read all 16-bit stats
        ReadStatS16(off.Equip_STR,          outStats.STR);
        ReadStatS16(off.Equip_DEX,          outStats.DEX);
        ReadStatS16(off.Equip_INT,          outStats.INT);
        ReadStatS16(off.Equip_LUK,          outStats.LUK);
        ReadStatS16(off.Equip_MaxHP,        outStats.MaxHP);
        ReadStatS16(off.Equip_MaxMP,        outStats.MaxMP);
        ReadStatS16(off.Equip_WeaponAttack, outStats.WeaponAttack);
        ReadStatS16(off.Equip_MagicAttack,  outStats.MagicAttack);
        ReadStatS16(off.Equip_WeaponDef,    outStats.WeaponDef);
        ReadStatS16(off.Equip_MagicDef,     outStats.MagicDef);
        ReadStatS16(off.Equip_Accuracy,     outStats.Accuracy);
        ReadStatS16(off.Equip_Avoid,        outStats.Avoid);
        ReadStatS16(off.Equip_Hands,        outStats.Hands);
        ReadStatS16(off.Equip_Speed,        outStats.Speed);
        ReadStatS16(off.Equip_Jump,         outStats.Jump);

        // UpgradeAvail: secure U8
        {
            uint32_t cs = 0;   // checksum size for U8, ffset = 0x2
            uint8_t  val = 0;

            MemUtil::RPM(
                hProcess,
                remoteAddress + off.Equip_UpgradeAvail + off.Equip_Upgrade_CS_Offsets,
                cs);

            // Assuming you already have a ZtlSecureFuseU8 helper:
            if (!MemUtil::ZtlSecureFuseU8(
                    hProcess,
                    remoteAddress + off.Equip_UpgradeAvail,
                    cs,
                    val))
            {
                val = 0;
            }

            outStats.UpgradeAvail = val;
        }

        return true;
    }



};
