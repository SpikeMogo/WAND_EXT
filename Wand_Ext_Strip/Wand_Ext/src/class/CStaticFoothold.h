
#pragma once

#include <Windows.h>
#include <cstdint>
#include <functional>
#include <cmath>
#include "MemoryUtil.h"
#include "MapleOffsets.h"
#include <navigator/Map.h>




// ==============================
//  Static Foothold & Space2D
// ==============================

class CStaticFoothold
{
public:
    uintptr_t remoteAddress;

    CStaticFoothold() : remoteAddress(0) {}
    explicit CStaticFoothold(uintptr_t addr) : remoteAddress(addr) {}

    // Read foothold endpoints (x1, y1, x2, y2)
    //   g_Maple.Foothold.x1, y1, x2, y2
    bool GetFoothold(HANDLE hProcess,
                 long& x1, long& y1,
                 long& x2, long& y2,
                 long& id, long& pr, long& ne,
                 long& layer) const
    {
        x1 = y1 = x2 = y2 = 0;
        id = pr = ne = 0;

        if (!hProcess || !remoteAddress)
            return false;

        // Temporary storage
        int _x1 = 0, _y1 = 0, _x2 = 0, _y2 = 0;
        uintptr_t _prPtr = 0, _nePtr = 0;
        int _id = 0, _prId = 0, _neId = 0, _layer=0;

        // --- Read position & ID ---
        if (!MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.Foothold.x1, _x1)) return false;
        if (!MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.Foothold.y1, _y1)) return false;
        if (!MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.Foothold.x2, _x2)) return false;
        if (!MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.Foothold.y2, _y2)) return false;
        if (!MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.Foothold.id, _id)) return false;
        if (!MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.Foothold.layer, _layer)) return false;


        // --- Read pointers to previous and next footholds ---
        if (!MemUtil::RPM<uintptr_t>(hProcess, remoteAddress + g_Maple.Foothold.pr, _prPtr)) _prPtr = 0;
        if (!MemUtil::RPM<uintptr_t>(hProcess, remoteAddress + g_Maple.Foothold.ne, _nePtr)) _nePtr = 0;

        // --- Resolve previous foothold pointer → read its ID ---
        if (_prPtr != 0)
        {
            MemUtil::RPM<int>(hProcess, _prPtr + g_Maple.Foothold.id, _prId);
        }

        // --- Resolve next foothold pointer → read its ID ---
        if (_nePtr != 0)
        {
            MemUtil::RPM<int>(hProcess, _nePtr + g_Maple.Foothold.id, _neId);
        }

        // --- Output final values ---
        x1 = _x1; y1 = _y1;
        x2 = _x2; y2 = _y2;
        id = _id;
        pr = _prId;
        ne = _neId;
        layer = _layer;


        return true;
    }


    bool GetAttributes(HANDLE hProcess, PhysicsParams::FootholdParams& Params) const
    {
        Params.footholdWalk = 1.0; //default
        Params.slopeUvX = 1.0; //default
        Params.slopeUvY = 0.0; //default
        Params.force = 0.0; //default


        if (!hProcess || !remoteAddress)
            return false;
        uintptr_t AttrFoothold = 0;
        if (!MemUtil::RPM(hProcess, remoteAddress + g_Maple.Foothold.m_pAttrFoothold, AttrFoothold) || !AttrFoothold) return false;

        if (!MemUtil::ReadTSecTypeDouble(hProcess, AttrFoothold + g_Maple.Foothold.WalkSpeed, Params.footholdWalk)) return false;
        if (!MemUtil::ReadTSecTypeDouble(hProcess, AttrFoothold + g_Maple.Foothold.Force, Params.force)) return false;
        if (!MemUtil::RPM<double>(hProcess, remoteAddress + g_Maple.Foothold.uvx, Params.slopeUvX)) return false;
        if (!MemUtil::RPM<double>(hProcess, remoteAddress + g_Maple.Foothold.uvy, Params.slopeUvY)) return false;


        return true;
    }

};
