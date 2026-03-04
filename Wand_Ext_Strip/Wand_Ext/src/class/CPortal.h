
#pragma once

#include <Windows.h>
#include <cstdint>
#include <functional>
#include "MemoryUtil.h"
#include "MapleOffsets.h"

class CPortal
{
public:
    uintptr_t remoteAddress;
    CPortal() : remoteAddress(0) {}
    explicit CPortal(uintptr_t addr) : remoteAddress(addr) {}

    bool Get( HANDLE hProcess, 
    long& x,
    long& y,
    long& ID,
    long& Type,
    uint32_t& toMapID,
    std::string& Name,
    std::string& toName
    ) const
    {
        if (!hProcess || !remoteAddress)
            return false;

        long _x    = 0;         if (!MemUtil::RPM<long>(hProcess, remoteAddress + g_Maple.CPortal.x, _x))               return false;
        long _y    = 0;         if (!MemUtil::RPM<long>(hProcess, remoteAddress + g_Maple.CPortal.y, _y))               return false;
        long _ID   = 0;         if (!MemUtil::RPM<long>(hProcess, remoteAddress + g_Maple.CPortal.ID, _ID))             return false;
        long _Type = 0;         if (!MemUtil::RPM<long>(hProcess, remoteAddress + g_Maple.CPortal.Type, _Type))         return false;
        uint32_t _toMapID = 0;  if (!MemUtil::RPM<uint32_t>(hProcess, remoteAddress + g_Maple.CPortal.toMapID, _toMapID)) return false;
        std::string _Name;      if(!MemUtil::ReadZXStringChar(hProcess, remoteAddress + g_Maple.CPortal.Name, _Name))       return false;
        std::string _toName; if(!MemUtil::ReadZXStringChar(hProcess, remoteAddress + g_Maple.CPortal.toName, _toName)) _toName="Null";

        
        x         = _x;
        y         = _y;
        ID        = _ID;
        Type      = _Type;
        toMapID   = _toMapID;
        Name      = _Name;
        toName = _toName;



        return true;
    }


    bool GetType( HANDLE hProcess, long& type) const
    {
        if (!hProcess || !remoteAddress)
            return false;

        int _type = 0;
        if (!MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.CPortal.Type, _type))  return false;
        
        
        type = _type;

        return true;
    }

    bool GetTopMap(HANDLE hProcess, uint32_t& toMap) const
    {
        if (!hProcess || !remoteAddress)
            return false;

        int _toMap = 0;
        if (!MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.CPortal.toMapID, _toMap))  return false;
        
        
        toMap = _toMap;

        return true;

    }



};
