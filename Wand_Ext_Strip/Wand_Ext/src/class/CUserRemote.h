#pragma once

#include <Windows.h>
#include <cstdint>
#include <functional>
#include "MemoryUtil.h"
#include "MapleOffsets.h"

class CUserRemote
{
public:
    uintptr_t remoteAddress;
    CUserRemote() : remoteAddress(0) {}
    explicit CUserRemote(uintptr_t addr) : remoteAddress(addr) {}

    bool Get(HANDLE hProcess,  
            uint32_t& ID,
            uint32_t& Job,
            long& x, long& y,
            std::string &Name) const
    {
        ID = Job = 0;
        x = y = 0;
        Name = "Unknown";

        if (!hProcess || !remoteAddress)
            return false;

        uint32_t _ID =0; 
        uint32_t _Job =0; 

        long _x = 0;
        long _y = 0;

        if (!MemUtil::RPM<uint32_t>(hProcess, remoteAddress + g_Maple.CUser.UID, _ID)) return false;
        if (!MemUtil::RPM<uint32_t>(hProcess, remoteAddress + g_Maple.CUser.Job, _Job)) return false;
        if (!MemUtil::RPM<long    >(hProcess, remoteAddress + g_Maple.CUser.x,   _x )) return false;
        if (!MemUtil::RPM<long    >(hProcess, remoteAddress + g_Maple.CUser.y,   _y )) return false;

        MemUtil::ReadZXStringChar(hProcess, remoteAddress + g_Maple.CUser.name, Name);

        ID = _ID; 
        Job = _Job; 
        x  = _x;
        y  = _y;


        return true;
    }



};
