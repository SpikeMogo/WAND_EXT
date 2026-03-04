#pragma once

#include <Windows.h>
#include <cstdint>
#include <functional>
#include "MemoryUtil.h"
#include "MapleOffsets.h"
#include <class/CFuncKeyMapped.h>


class CTemporaryStat
{
public:
    uintptr_t remoteAddress;
    CTemporaryStat() : remoteAddress(0) {}
    explicit CTemporaryStat(uintptr_t addr) : remoteAddress(addr) {}

    bool Get(HANDLE hProcess,       uint32_t& Type , 
                                    uint32_t& ID   ,
                                    uint32_t& SubID,
                                    uint32_t& tLeft,
                                    UINT& vKey) const
    {
        Type  = 0;
        ID    = 0;
        SubID = 0;
        tLeft = 0;


        if (!hProcess || !remoteAddress)
            return false;


        if (!MemUtil::RPM< uint32_t>(  hProcess, remoteAddress + g_Maple.CTemporaryStatView.nType   ,    Type)) return false;
        if (!MemUtil::RPM< uint32_t>(  hProcess, remoteAddress + g_Maple.CTemporaryStatView.nID     ,      ID)) return false;
        if (!MemUtil::RPM< uint32_t>(  hProcess, remoteAddress + g_Maple.CTemporaryStatView.nSubID  ,   SubID)) return false;
        if (!MemUtil::RPM< uint32_t>(  hProcess, remoteAddress + g_Maple.CTemporaryStatView.tLeft   ,   tLeft)) return false;

        if (!CFuncKeyMapped::GetVirtualKeyMappedRemote(hProcess, 1, ID, vKey)) vKey=0;

        return true;
    }



};
