#pragma once

#include <Windows.h>
#include <cstdint>
#include <functional>
#include "MemoryUtil.h"
#include "MapleOffsets.h"




class CMob
{
public:
    uintptr_t remoteAddress;
    CMob() : remoteAddress(0) {}
    explicit CMob(uintptr_t addr) : remoteAddress(addr) {}

    bool Get(HANDLE hProcess, long& outX, long& outY,long& outXp, long& outYp) const
    {
        outX = outY = 0;

        if (!hProcess || !remoteAddress)
            return false;

        int x = 0, y = 0;
        if (!MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.CMob.posX_raw, x))
            return false;
        if (!MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.CMob.posY_raw, y))
            return false;

        int xp = 0, yp = 0;
        if (!MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.CMob.prevPosX_raw, xp))
            return false;
        if (!MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.CMob.prevPosY_raw, yp))
            return false;

        outX = x;
        outY = y;
        outXp = xp;
        outYp = yp;
        return true;
    }

    bool GetTemplateId(HANDLE hProcess, uint32_t& outTemplateId) const
    {
        outTemplateId = 0;
        if (!hProcess || !remoteAddress)
            return false;

        // 1) Read pointer to CMobTemplate
        uintptr_t pTemplate = 0;
        if (!MemUtil::RPM(hProcess,
                        remoteAddress + g_Maple.CMob.CMobTemplate,
                        pTemplate) || !pTemplate)
        {
            return false;
        }
        
        uintptr_t encSaltAddr = pTemplate + g_Maple.CMob.TemplateID; // TemplateID offset
        uint32_t checksum     = 0;
        if (!MemUtil::RPM(hProcess,pTemplate + g_Maple.CMob.TemplateID_CS, checksum))
        {
            return false;
        }

        // 3) Decode 
        return MemUtil::ZtlSecureFuseU32(hProcess, encSaltAddr, checksum, outTemplateId);
    }



    bool GetHP(HANDLE hProcess, long& HP, long&maxHP) const
    {

        maxHP = HP = 0;
        if (!hProcess || !remoteAddress)
            return false;


        uintptr_t pTemplate = 0;
        if (!MemUtil::RPM(hProcess,remoteAddress + g_Maple.CMob.CMobTemplate,  pTemplate) || !pTemplate) return false;

        int _maxHP = 0;

        uint32_t checksum     = 0;
        if (!MemUtil::RPM(hProcess,              pTemplate + g_Maple.CMob.TemplateMaxHP_CS, checksum)) return false;
        if (!MemUtil::ZtlSecureFuseI32(hProcess, pTemplate + g_Maple.CMob.TemplateMaxHP,    checksum, _maxHP))  return false;

        maxHP = _maxHP;

        int _HP;
        if (!MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.CMob.HP, _HP)) return false;

        HP = _HP;

        return true;

    }


    bool GetTemplateName(HANDLE hProcess, std::string& outName) const
    {
        outName.clear();
        if (!hProcess || !remoteAddress)
            return false;

        // 1) Read pointer to CMobTemplate
        uintptr_t pTemplate = 0;
        if (!MemUtil::RPM(hProcess,
                        remoteAddress + g_Maple.CMob.CMobTemplate,  // 0x98
                        pTemplate) || !pTemplate)
        {
            return false;
        }

        // 2) Address of ZXString<char> sName in CMobTemplate
        uintptr_t zxStrAddr = pTemplate + g_Maple.CMob.TemplatesName; // 0x30

        // 3) 
        return MemUtil::ReadZXStringChar(hProcess, zxStrAddr, outName);
    }


    bool GetFoothold(HANDLE hProcess, uint32_t& OnFootholdAddress) const
    {
        OnFootholdAddress = 0;
        if (!hProcess || !remoteAddress)
            return false;

        
        uintptr_t pvecCtrl = 0;
        if (!MemUtil::ReadPtr32(hProcess, remoteAddress+g_Maple.CMob.p_CVecCtrlInterface, pvecCtrl) || !pvecCtrl) return false;
        
        pvecCtrl  -= g_Maple.CMob.InterfaceToCVecCtrl;

        if (!MemUtil::RPM(                hProcess, pvecCtrl +g_Maple.CMob.CVecCtrl_pFoothold, OnFootholdAddress))    return false;


    }



};
