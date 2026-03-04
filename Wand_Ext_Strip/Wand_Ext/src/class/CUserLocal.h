
#pragma once

#include <Windows.h>
#include <cstdint>
#include <functional>
#include "MemoryUtil.h"
#include "MapleOffsets.h"
#include "AppState.h"
#include <class/GW_ItemSlot.h>
#include <class/ZRef.h>
#include <class/CPet.h>


class CUserLocal
{
public:
    // Static: Read position (x, y)
    static bool GetPos(HANDLE hProcess, long& x, long& y)
    {
        x = y = 0;
        if (!hProcess) return false;

        uintptr_t localAddr = 0;
        if (!MemUtil::ReadPtr32(hProcess, g_Maple.CUserLocal.CUserLocal, localAddr) || !localAddr)
            return false;

        int _x = 0, _y = 0;
        if (!MemUtil::RPM<int>(hProcess, localAddr + g_Maple.CUserLocal.x, _x)) return false;
        if (!MemUtil::RPM<int>(hProcess, localAddr + g_Maple.CUserLocal.y, _y)) return false;

        x = _x;
        y = _y;
        return true;
    }

    // Static: Read attack, breath, anim, combo, faceDir
    static bool GetStatus(
        HANDLE hProcess,
        long& attackCount,
        long& breath,
        long& charAnimation,
        long& comboCount,
        long& faceDir,
        long& UID)
    {
        attackCount = breath = charAnimation = comboCount = faceDir = UID =0;
        if (!hProcess) return false;

        uintptr_t localAddr = 0;
        if (!MemUtil::ReadPtr32(hProcess, g_Maple.CUserLocal.CUserLocal, localAddr) || !localAddr)
            return false;

        int _attack = 0, _breath = 0, _anim = 0, _combo = 0, _face = 0, _id = 0;
        if (!MemUtil::RPM<int>(hProcess, localAddr + g_Maple.CUserLocal.attackCount, _attack)) return false;
        if (!MemUtil::RPM<int>(hProcess, localAddr + g_Maple.CUserLocal.breath,      _breath)) return false;
        if (!MemUtil::RPM<int>(hProcess, localAddr + g_Maple.CUserLocal.charAnimation,_anim))  return false;
        if (!MemUtil::RPM<int>(hProcess, localAddr + g_Maple.CUserLocal.comboCount,  _combo))  return false;
        if (!MemUtil::RPM<int>(hProcess, localAddr + g_Maple.CUserLocal.faceDir,     _face))   return false;
        if (!MemUtil::RPM<int>(hProcess, localAddr + g_Maple.CUserLocal.UID,     _id))   return false;


        attackCount  = _attack;
        breath       = _breath;
        charAnimation= _anim;
        comboCount   = _combo;
        faceDir      = _face;
        UID      = _id;

        return true;
    }


    static bool GetFaceDir(
            HANDLE hProcess,
            int& faceDir)
        {

            if (!hProcess) return false;

            uintptr_t localAddr = 0;
            if (!MemUtil::ReadPtr32(hProcess, g_Maple.CUserLocal.CUserLocal, localAddr) || !localAddr)
                return false;

            if (!MemUtil::RPM<int>(hProcess, localAddr + g_Maple.CUserLocal.faceDir,  faceDir))   return false;
            return true;
        }


    static bool GetBreath(
        HANDLE hProcess,
        int& breath)
    {
        breath = 0;
        if (!hProcess) return false;

        uintptr_t localAddr = 0;
        if (!MemUtil::ReadPtr32(hProcess, g_Maple.CUserLocal.CUserLocal, localAddr) || !localAddr)
            return false;

        if (!MemUtil::RPM<int>(hProcess, localAddr + g_Maple.CUserLocal.breath,      breath)) return false;


        return true;
    }

    static bool GetStats(HANDLE hProcess, long& outHp,long& outMp, long& outMaxHp,long& outMaxMp, long& outExp, double& ExpPer, uint8_t& outlvl, uint16_t& outJob, int32_t& outMesos)
    {
        outHp = 0;
        outMp = 0;
        outlvl = 0;
        outExp = 0;
        outJob=0;
        outMesos = 0;

        if (!hProcess)
            return false;

    
        uintptr_t base = 0;
        if (!MemUtil::RPM(hProcess, g_Maple.CharacterStat.CharacterStat, base) || !base)
            return false;

        uintptr_t base2 = 0;
        if (!MemUtil::RPM(hProcess, g_Maple.CWvsContext.CWvsContext, base2) || !base2)
            return false;

        int16_t hp16 = 0;
        int16_t mp16 = 0;
        int32_t Exp32 = 0;

        int32_t max_hp = 0;
        int32_t max_mp = 0;


        uint8_t level = 0;
        int32_t checksum = 0;

        int16_t nJob = 0;

        ExpPer = 0;


        if (!MemUtil::RPM(             hProcess, base + g_Maple.CharacterStat.HP_CS, checksum))       return false;
        if (!MemUtil::ZtlSecureFuseS16(hProcess, base + g_Maple.CharacterStat.HP,    checksum, hp16)) return false;


        if (!MemUtil::RPM(             hProcess, base + g_Maple.CharacterStat.Job_CS, checksum))       return false;
        if (!MemUtil::ZtlSecureFuseS16(hProcess, base + g_Maple.CharacterStat.Job,    checksum, nJob)) return false;

        if (!MemUtil::RPM(             hProcess, base + g_Maple.CharacterStat.MP_CS, checksum))       return false;
        if (!MemUtil::ZtlSecureFuseS16(hProcess, base + g_Maple.CharacterStat.MP,    checksum, mp16)) return false;

        if (!MemUtil::RPM(             hProcess, base + g_Maple.CharacterStat.EXP_CS, checksum))       return false;
        if (!MemUtil::ZtlSecureFuseI32(hProcess, base + g_Maple.CharacterStat.EXP,    checksum, Exp32)) return false;

        if (!MemUtil::RPM(            hProcess,  base + g_Maple.CharacterStat.Level_CS, checksum))     return false;
        if (!MemUtil::ZtlSecureFuseU8(hProcess,  base + g_Maple.CharacterStat.Level, checksum, level))  return false;
        
        if (!MemUtil::RPM(             hProcess, base + g_Maple.CharacterStat.EXP_CS, checksum))       return false;
        if (!MemUtil::ZtlSecureFuseI32(hProcess, base + g_Maple.CharacterStat.EXP,    checksum, Exp32)) return false;

        int32_t mesos32 = 0;
        if (!MemUtil::RPM(             hProcess, base + g_Maple.CharacterStat.Mesos_CS, checksum))         return false;
        if (!MemUtil::ZtlSecureFuseI32(hProcess, base + g_Maple.CharacterStat.Mesos,    checksum, mesos32)) return false;

        if (!MemUtil::RPM(             hProcess, base2 + g_Maple.CWvsContext.MaxHP_CS, checksum))       return false;
        if (!MemUtil::ZtlSecureFuseI32(hProcess, base2 + g_Maple.CWvsContext.MaxHP,    checksum, max_hp)) return false;

        if (!MemUtil::RPM(             hProcess, base2 + g_Maple.CWvsContext.MaxMP_CS, checksum))       return false;
        if (!MemUtil::ZtlSecureFuseI32(hProcess, base2 + g_Maple.CWvsContext.MaxMP,    checksum, max_mp)) return false;


        base = 0;
        if (!MemUtil::RPM(hProcess, g_Maple.CUIStatusBar.Instance, base) || !base) return false;
        if (!MemUtil::RPM<double>(hProcess, base+ g_Maple.CUIStatusBar.m_dEXPNo, ExpPer)) return false;


        outHp = static_cast<long>(hp16);
        outMp = static_cast<long>(mp16);
        outExp = Exp32;
        outlvl = level;
        outMaxHp = max_hp;
        outMaxMp = max_mp;
        outJob = nJob;
        outMesos = mesos32;


        return true;
    }


    static bool GetBasicStats(HANDLE hProcess, uint32_t& o_str, uint32_t& o_dex, uint32_t& o_int, uint32_t& o_luk)
    {
        o_str = 0; 
        o_dex = 0; 
        o_int = 0; 
        o_luk = 0;

        if (!hProcess)
            return false;

    
        uintptr_t base = 0;
        if (!MemUtil::RPM(hProcess, g_Maple.CWvsContext.CWvsContext, base) || !base)
            return false;

        uintptr_t BasicStat  = base+g_Maple.CWvsContext.BasicStat;

        uint32_t _str, _dex, _int, _luk;
        int32_t checksum = 0;
        

        if (!MemUtil::RPM(             hProcess, BasicStat  + g_Maple.CWvsContext.STR+0x8, checksum))       return false;
        if (!MemUtil::ZtlSecureFuseU32(hProcess, BasicStat  + g_Maple.CWvsContext.STR,     checksum, _str)) return false;


        if (!MemUtil::RPM(             hProcess, BasicStat  + g_Maple.CWvsContext.DEX+0x8, checksum))       return false;
        if (!MemUtil::ZtlSecureFuseU32(hProcess, BasicStat  + g_Maple.CWvsContext.DEX,     checksum, _dex)) return false;

        if (!MemUtil::RPM(             hProcess, BasicStat  + g_Maple.CWvsContext.INT+0x8, checksum))       return false;
        if (!MemUtil::ZtlSecureFuseU32(hProcess, BasicStat  + g_Maple.CWvsContext.INT,     checksum, _int)) return false;

        if (!MemUtil::RPM(             hProcess, BasicStat  + g_Maple.CWvsContext.LUK+0x8, checksum))       return false;
        if (!MemUtil::ZtlSecureFuseU32(hProcess, BasicStat  + g_Maple.CWvsContext.LUK,     checksum, _luk)) return false;

        o_str = _str; 
        o_dex = _dex; 
        o_int = _int;
        o_luk = _luk;

        return true;
    }

    static bool GetSecondStats(HANDLE hProcess, 
            uint32_t& attack   ,
            uint32_t& defense  ,
            uint32_t& magic    ,
            uint32_t& magic_def,
            uint32_t& accuracy ,
            uint32_t& avoid    ,
            uint32_t& hands    ,
            uint32_t& speed    ,
            uint32_t& jump      )
    {
        attack    = defense   = magic     = magic_def = accuracy  = avoid     = hands     = speed     = jump      = 0 ;

        if (!hProcess)
            return false;

    
        uintptr_t base = 0;
        if (!MemUtil::RPM(hProcess, g_Maple.CWvsContext.CWvsContext, base) || !base)
            return false;

        uintptr_t SecondaryStat  = base+g_Maple.CWvsContext.SecondaryStat;

        uint32_t _attack, _defense, _magic, _magic_def, _accuracy, _avoid, _hands, _speed, _jump = 0;
        int32_t checksum = 0;
        

        if (!MemUtil::RPM(             hProcess, SecondaryStat  + g_Maple.CWvsContext.attack    +0x8, checksum))            return false;
        if (!MemUtil::ZtlSecureFuseU32(hProcess, SecondaryStat  + g_Maple.CWvsContext.attack    ,     checksum, _attack    )) return false;

        if (!MemUtil::RPM(             hProcess, SecondaryStat  + g_Maple.CWvsContext.defense   +0x8, checksum))            return false;
        if (!MemUtil::ZtlSecureFuseU32(hProcess, SecondaryStat  + g_Maple.CWvsContext.defense   ,     checksum, _defense   )) return false;

        if (!MemUtil::RPM(             hProcess, SecondaryStat  + g_Maple.CWvsContext.magic     +0x8, checksum))            return false;
        if (!MemUtil::ZtlSecureFuseU32(hProcess, SecondaryStat  + g_Maple.CWvsContext.magic     ,     checksum, _magic     )) return false;

        if (!MemUtil::RPM(             hProcess, SecondaryStat  + g_Maple.CWvsContext.magic_def +0x8, checksum))            return false;
        if (!MemUtil::ZtlSecureFuseU32(hProcess, SecondaryStat  + g_Maple.CWvsContext.magic_def ,     checksum, _magic_def )) return false;

        if (!MemUtil::RPM(             hProcess, SecondaryStat  + g_Maple.CWvsContext.accuracy  +0x8, checksum))            return false;
        if (!MemUtil::ZtlSecureFuseU32(hProcess, SecondaryStat  + g_Maple.CWvsContext.accuracy  ,     checksum, _accuracy  )) return false;

        if (!MemUtil::RPM(             hProcess, SecondaryStat  + g_Maple.CWvsContext.avoid     +0x8, checksum))            return false;
        if (!MemUtil::ZtlSecureFuseU32(hProcess, SecondaryStat  + g_Maple.CWvsContext.avoid     ,     checksum, _avoid     )) return false;

        if (!MemUtil::RPM(             hProcess, SecondaryStat  + g_Maple.CWvsContext.hands     +0x8, checksum))            return false;
        if (!MemUtil::ZtlSecureFuseU32(hProcess, SecondaryStat  + g_Maple.CWvsContext.hands     ,     checksum, _hands     )) return false;

        if (!MemUtil::RPM(             hProcess, SecondaryStat  + g_Maple.CWvsContext.speed     +0x8, checksum))            return false;
        if (!MemUtil::ZtlSecureFuseU32(hProcess, SecondaryStat  + g_Maple.CWvsContext.speed     ,     checksum, _speed     )) return false;

        if (!MemUtil::RPM(             hProcess, SecondaryStat  + g_Maple.CWvsContext.jump      +0x8, checksum))            return false;
        if (!MemUtil::ZtlSecureFuseU32(hProcess, SecondaryStat  + g_Maple.CWvsContext.jump      ,     checksum, _jump      )) return false;

        


        attack    = _attack   ; 
        defense   = _defense  ; 
        magic     = _magic    ; 
        magic_def = _magic_def; 
        accuracy  = _accuracy ; 
        avoid     = _avoid    ; 
        hands     = _hands    ; 
        speed     = _speed    ; 
        jump      = _jump     ; 

        return true;
    }

    static size_t GetItemCount(HANDLE hProcess, uint32_t ID)
    {

        int type = ID/1000000;

        if(type<1 && type>5) return 0;

        auto tab = static_cast<InventoryType>(type);
        int count = 0;
        CUserLocal::ForEachInventoryItem(
            hProcess,
            tab,
            [&](const GW_ItemSlot& item) -> bool
            {
                uint32_t item_id;
                uint16_t Qty;
                std::string Name;
                std::string Des;
                item.Get(app.hProcess, item_id, Qty, Name, Des);
                if (item_id == ID)
                {
                    count += Qty;
                }
                
                return true;
            }
        );

        return count;

    }

    // Find the first inventory slot containing item with given ID
    // Returns slot number (1-based), or 0 if not found
    static size_t GetItemSlot(HANDLE hProcess, uint32_t ID)
    {
        int type = ID / 1000000;
        if (type < 1 || type > 5) return 0;

        auto tab = static_cast<InventoryType>(type);
        size_t foundSlot = 0;
        CUserLocal::ForEachInventoryItem(
            hProcess,
            tab,
            [&](const GW_ItemSlot& item) -> bool
            {
                uint32_t item_id = 0;
                uint16_t Qty = 0;
                std::string Name, Des;
                item.Get(hProcess, item_id, Qty, Name, Des);
                if (item_id == ID)
                {
                    foundSlot = item.slot;
                    return false; // stop iteration
                }
                return true;
            }
        );

        return foundSlot;
    }

    static size_t ForEachInventoryItem(
    HANDLE hProcess,
    InventoryType invType,
    const std::function<bool(const GW_ItemSlot&)> &callback)
    {
        if (!hProcess)
            return 0;

        // 1) Get CWvsContext*
        uintptr_t pCWvsContext = 0;
        if (!MemUtil::RPM(hProcess, g_Maple.CWvsContext.CWvsContext, pCWvsContext) || !pCWvsContext)
            return 0;

        // 2) Get CharacterData*
        uintptr_t pCharacterData = 0;
        if (!MemUtil::RPM(hProcess, pCWvsContext + g_Maple.CWvsContext.CharacterData, pCharacterData) || !pCharacterData)
            return 0;

        // 3) Base of aaItemSlot[0] (ZArray<ZRef<GW_ItemSlotBase>> aaItemSlot[5])
        uintptr_t aaItemSlotBase = pCharacterData + g_Maple.CWvsContext.ZArray_ZRef_GW_ItemSlotBase;

        int tIndex = static_cast<int>(invType)-1;

        // 4) Address of ZArrayRemote for this inventory type
        uintptr_t zarrayAddr = aaItemSlotBase + tIndex * sizeof(ZArray);

        ZArray zArr{};
        if (!MemUtil::RPM(hProcess, zarrayAddr, zArr))
            return 0;

        if (!zArr.a)
            return 0;  // no array

        uintptr_t arrayBase = zArr.a;  // remote pointer to first ZRefRemote<GW_ItemSlotBase>

        // 5) Get size using header: inventory[-1].p - 1
        ZRef headerRef{};
        if (!MemUtil::RPM(
                hProcess,
                arrayBase - sizeof(ZRef),
                headerRef))
        {
            return 0;
        }

        size_t size = 0;
        if (headerRef.p > 0)
            size = static_cast<size_t>(headerRef.p) - 1;

        if (size == 0)
            return 0;

        size_t visited = 0;
        
        // 6) Iterate slots 1..size (slot 0 is unused, like original hook)
        for (size_t i = 1; i <= size; ++i)
        {
            ZRef slotRef{};
            uintptr_t slotRefAddr = arrayBase + i * sizeof(ZRef);

            if (!MemUtil::RPM(hProcess, slotRefAddr, slotRef))
                continue;

            if (!slotRef.p)
                continue;  // empty slot

            uintptr_t pItem = slotRef.p;

            GW_ItemSlot item(pItem, invType, i);
            ++visited;
            if (!callback(item))
                break;
        }

        return size;
    }
    

    static bool GetWalkAttributes(HANDLE hProcess, PhysicsParams::UserParams& Params) 
    {

        Params.walkJump = 1.0;
        Params.shoeWalkSpeed = 1.0;
        Params.mass = 100.0;
        Params.hasFlyStuff = false;

        if (!hProcess)
            return false;

        
        uintptr_t localAddr = 0;
        if (!MemUtil::ReadPtr32(hProcess, g_Maple.CUserLocal.CUserLocal, localAddr) || !localAddr) return false;
        uintptr_t pAttrShoe = 0;
        if (!MemUtil::ReadPtr32(hProcess, localAddr+g_Maple.CUserLocal.m_pAttrShoe, pAttrShoe) || !pAttrShoe) return false;


        if (!MemUtil::ReadTSecTypeDouble(hProcess, pAttrShoe + g_Maple.CUserLocal.AttrShoe_WalkSpeed, Params.shoeWalkSpeed)) return false;
        if (!MemUtil::ReadTSecTypeDouble(hProcess, pAttrShoe + g_Maple.CUserLocal.AttrShoe_WalkJump,  Params.walkJump))      return false;
        if (!MemUtil::ReadTSecTypeDouble(hProcess, pAttrShoe + g_Maple.CUserLocal.AttrShoe_Mass,      Params.mass))          return false;


        return true;
    }


    static bool GetVecCtrl(HANDLE hProcess, PhysicsParams::CVecCtrl& CVecCtrl) 
    {

        if (!hProcess)
            return false;

        uintptr_t localAddr = 0;
        if (!MemUtil::ReadPtr32(hProcess, g_Maple.CUserLocal.CUserLocal, localAddr) || !localAddr) return false;
        
        uintptr_t pvecCtrl = 0;
        if (!MemUtil::ReadPtr32(hProcess, localAddr+g_Maple.CUserLocal.p_CVecCtrlInterface, pvecCtrl) || !pvecCtrl) return false;
        
        pvecCtrl  -= g_Maple.CUserLocal.InterfaceToCVecCtrl;
        uint32_t checksum = 0;

        if (!MemUtil::RPM(                hProcess, pvecCtrl +g_Maple.CUserLocal.CVecCtrl_X_CS, checksum))    return false;
        if (!MemUtil::ZtlSecureFuseDouble(hProcess, pvecCtrl +g_Maple.CUserLocal.CVecCtrl_X   , checksum, CVecCtrl.X)) return false;


        if (!MemUtil::RPM(                hProcess, pvecCtrl +g_Maple.CUserLocal.CVecCtrl_Y_CS, checksum))    return false;
        if (!MemUtil::ZtlSecureFuseDouble(hProcess, pvecCtrl +g_Maple.CUserLocal.CVecCtrl_Y   , checksum, CVecCtrl.Y)) return false;


        if (!MemUtil::RPM(                hProcess, pvecCtrl +g_Maple.CUserLocal.CVecCtrl_Vx_CS, checksum))    return false;
        if (!MemUtil::ZtlSecureFuseDouble(hProcess, pvecCtrl +g_Maple.CUserLocal.CVecCtrl_Vx   , checksum, CVecCtrl.Vx)) return false;


        if (!MemUtil::RPM(                hProcess, pvecCtrl +g_Maple.CUserLocal.CVecCtrl_Vy_CS, checksum))    return false;
        if (!MemUtil::ZtlSecureFuseDouble(hProcess, pvecCtrl +g_Maple.CUserLocal.CVecCtrl_Vy   , checksum, CVecCtrl.Vy)) return false;


        CVecCtrl.OnRopeAddress = 0;

        if (!MemUtil::RPM(             hProcess, pvecCtrl +g_Maple.CUserLocal.CVecCtrl_pLadderOrRope_CS, checksum))    return false;
        if (!MemUtil::ZtlSecureFuseU32(hProcess, pvecCtrl +g_Maple.CUserLocal.CVecCtrl_pLadderOrRope   , checksum, CVecCtrl.OnRopeAddress)) return false;

        CVecCtrl.OnFootholdAddress = 0;
        if (!MemUtil::RPM(                hProcess, pvecCtrl +g_Maple.CUserLocal.CVecCtrl_pFoothold, CVecCtrl.OnFootholdAddress))    return false;
        CVecCtrl.Layer = 0;
        if (!MemUtil::RPM(                hProcess, pvecCtrl +g_Maple.CUserLocal.CVecCtrl_layer, CVecCtrl.Layer))    return false;


        CVecCtrl.IsFalling =  (!CVecCtrl.OnFootholdAddress) && (CVecCtrl.Vy>0.0);
        CVecCtrl.IsStopped = (CVecCtrl.Vx==0.0) && (CVecCtrl.Vy==0.0);


        if (!MemUtil::RPM(             hProcess, pvecCtrl +g_Maple.CUserLocal.CVecCtrl_InputX_CS, checksum))    return false;
        if (!MemUtil::ZtlSecureFuseI32(hProcess, pvecCtrl +g_Maple.CUserLocal.CVecCtrl_InputX   , checksum, CVecCtrl.InputX)) return false;


        return true;
    }


    static bool GetChannel(HANDLE hProcess, uint32_t& channel, uint32_t& total_channel )
    {
        channel = 0;
        total_channel =1;
        if (!hProcess) return false;

    
        uintptr_t base = 0;
        if (!MemUtil::ReadPtr32(hProcess, g_Maple.CWvsContext.CWvsContext, base) || !base) return false;
        if (!MemUtil::RPM<uint32_t>(hProcess, base + g_Maple.CWvsContext.Channel,   channel)) return false;
        channel += 1;


        uintptr_t arrayPtr = 0;
        if (!MemUtil::ReadPtr32(hProcess, base + g_Maple.CWvsContext.ZArray_Channel, arrayPtr) || !arrayPtr) return false;
        if (!MemUtil::RPM(hProcess, arrayPtr - 4, total_channel)) return false;

        return true;
        
    }

    static std::string GetDebuffs(HANDLE hProcess)
    {
        uintptr_t base = 0;
        if (!MemUtil::RPM(hProcess, g_Maple.CWvsContext.CWvsContext, base) || !base)
            return "";

        uintptr_t SecondaryStat = base + g_Maple.CWvsContext.SecondaryStat;

        const std::pair<uint32_t, const char*> diseases[] = {
            { g_Maple.CWvsContext.SEAL,     "SEAL" },
            { g_Maple.CWvsContext.DARKNESS, "DARKNESS" },
            { g_Maple.CWvsContext.STUN,     "STUN" },
            { g_Maple.CWvsContext.POISON,   "POISON" },
            { g_Maple.CWvsContext.WEAKNESS, "WEAKEN" },
            { g_Maple.CWvsContext.CURSE,    "CURSE" },
            { g_Maple.CWvsContext.SLOW,     "SLOW" },
            { g_Maple.CWvsContext.SEDUCE,   "SEDUCE" },
            { g_Maple.CWvsContext.ZOMBIFY,  "ZOMBIFY" },
            { g_Maple.CWvsContext.CONFUSE,  "CONFUSE" },
        };

        std::string result;

        for (const auto& [rOffset, name] : diseases)
        {
            int32_t nCS = 0, nValue = 0;
            uint32_t nOffset = rOffset - 0x0C;

            if (!MemUtil::RPM(hProcess, SecondaryStat + nOffset + 0x08, nCS))
                continue;

            if (!MemUtil::ZtlSecureFuseI32(hProcess, SecondaryStat + nOffset, nCS, nValue))
                continue;

            if (nValue > 0)
            {
                if (!result.empty())
                    result += " + ";
                result += name;
            }
        }

        return result;
    }

    // Get active pets (up to 3). Returns number of active pets found.
    // m_apPet is a ZArray<ZRef<CPet>> with 3 slots on CUser.
    // ZRef stride = 8 bytes: { int refCount, uintptr_t p }
    // Slot with p == 0 means no pet in that slot.
    static int GetActivePets(HANDLE hProcess, CPet outPets[3])
    {
        outPets[0] = CPet();
        outPets[1] = CPet();
        outPets[2] = CPet();

        if (!hProcess)
            return 0;

        // Get CUserLocal*
        uintptr_t localAddr = 0;
        if (!MemUtil::ReadPtr32(hProcess, g_Maple.CUserLocal.CUserLocal, localAddr) || !localAddr)
            return 0;

        // Read m_apPet.a (pointer to ZRef<CPet>[3] array)
        uintptr_t petArrayPtr = 0;
        if (!MemUtil::RPM(hProcess, localAddr + g_Maple.CPet.m_apPet, petArrayPtr) || !petArrayPtr)
            return 0;

        int count = 0;
        for (int i = 0; i < 3; i++)
        {
            // Each ZRef<CPet> is 8 bytes: { refCount(4), p(4) }
            ZRef ref{};
            if (!MemUtil::RPM(hProcess, petArrayPtr + i * sizeof(ZRef), ref))
                continue;

            if (ref.p)
            {
                outPets[i] = CPet(ref.p);
                count++;
            }
        }

        return count;
    }

};
