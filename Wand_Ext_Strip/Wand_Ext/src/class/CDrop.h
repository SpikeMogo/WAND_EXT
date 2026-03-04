#pragma once

#include <Windows.h>
#include <cstdint>
#include <functional>
#include "MemoryUtil.h"
#include "MapleOffsets.h"

class CDrop
{
public:
    uintptr_t remoteAddress;
    CDrop() : remoteAddress(0) {}
    explicit CDrop(uintptr_t addr) : remoteAddress(addr) {}

    bool Get(HANDLE hProcess,   uint32_t& UID, 
                                    uint32_t& OwnerID ,
                                    uint32_t& SourceID,
                                    uint32_t& OwnType ,
                                    uint32_t& IsMeso  ,
                                    uint32_t& ID      ,
                                    long& x, long& y,
                                    std::string &Name) const
    {
        UID = 0;
        OwnerID  = 0;
        SourceID = 0;
        OwnType  = 0;
        IsMeso   = 0;
        ID       = 0;
        x = y =0;

        if (!hProcess || !remoteAddress)
            return false;

        uint32_t _UID = 0;
        uint32_t _OwnerID  =0; 
        uint32_t _SourceID =0; 
        uint32_t _OwnType  =0; 
        uint32_t _IsMeso   =0; 
        uint32_t _ID       =0; 
        long _x = 0;
        long _y = 0;


        if (!MemUtil::RPM< uint32_t>(hProcess, remoteAddress + g_Maple.CDrop.UID, _UID)) return false;
        if (!MemUtil::RPM< uint32_t>(hProcess, remoteAddress + g_Maple.CDrop.OwnerID  , _OwnerID )) return false;
        if (!MemUtil::RPM< uint32_t>(hProcess, remoteAddress + g_Maple.CDrop.SourceID , _SourceID)) return false;
        if (!MemUtil::RPM< uint32_t>(hProcess, remoteAddress + g_Maple.CDrop.OwnType  , _OwnType )) return false;
        if (!MemUtil::RPM< uint32_t>(hProcess, remoteAddress + g_Maple.CDrop.IsMeso   , _IsMeso  )) return false;
        if (!MemUtil::RPM< uint32_t>(hProcess, remoteAddress + g_Maple.CDrop.ID       , _ID      )) return false;
        
        
        if (!MemUtil::ReadPointerChain32<long>(hProcess, remoteAddress + g_Maple.CDrop.Gr2DLayer,  {g_Maple.CDrop.x}, _x)) return false;
        if (!MemUtil::ReadPointerChain32<long>(hProcess, remoteAddress + g_Maple.CDrop.Gr2DLayer,  {g_Maple.CDrop.y}, _y)) return false;

        if(!_IsMeso)
            CItemInfo::GetItemStringByIdAndKey(hProcess,_ID,"name",Name);
        else
            Name = "Mesos";


        UID = _UID;
        OwnerID  = _OwnerID ; 
        SourceID = _SourceID; 
        OwnType  = _OwnType ; 
        IsMeso   = _IsMeso  ; 
        ID       = _ID      ; 
        x = _x;
        y = _y;


        return true;
    }



};
