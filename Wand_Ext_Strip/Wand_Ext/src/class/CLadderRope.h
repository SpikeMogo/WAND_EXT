
#pragma once

#include <Windows.h>
#include <cstdint>
#include <functional>
#include "MemoryUtil.h"
#include "MapleOffsets.h"




class CLadderRope
{
public:
    uintptr_t remoteAddress;
    CLadderRope() : remoteAddress(0) {}
    explicit CLadderRope(uintptr_t addr) : remoteAddress(addr) {}

    // Optional helper: read all fields, like your CStaticFoothold::GetFoothold
    bool Get(
        HANDLE hProcess,
        long& id,   
        long& isLadder,
        long& fromUpper,
        long& x,
        long& y1,
        long& y2) const
    {
        if (!hProcess || !remoteAddress)
            return false;

        int _id = 0, _ladder = 0, _upper = 0;
        int _x = 0, _y1 = 0, _y2 = 0;

        if (!MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.LadderRope.id, _id))  return false;
        if (!MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.LadderRope.ladder, _ladder))  return false;
        if (!MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.LadderRope.upperFH, _upper))  return false;
        if (!MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.LadderRope.x, _x))        return false;
        if (!MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.LadderRope.y1, _y1))      return false;
        if (!MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.LadderRope.y2, _y2))      return false;

        id = _id;
        isLadder  = _ladder;
        fromUpper = _upper;
        x         = _x;
        y1        = _y1;
        y2        = _y2;
        return true;
    }

    bool GetId(
        HANDLE hProcess,
        long& id) const
    {
        if (!hProcess || !remoteAddress)
            return false;

        int _id = 0;

        if (!MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.LadderRope.id, _id))  return false;
        id = _id;
        return true;
    }
};
