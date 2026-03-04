
#pragma once

#include <Windows.h>
#include <cstdint>
#include <functional>
#include "MemoryUtil.h"
#include "MapleOffsets.h"
#include <class/CMob.h>




class CMobPool
{
public:

    static size_t ForEachMob(HANDLE hProcess, const std::function<bool(const CMob&)>& callback)
    {
        if (!hProcess)
            return 0;

        uintptr_t poolThis = 0;
        if (!MemUtil::ReadPtr32(hProcess, g_Maple.CMob.CMobPool, poolThis) || poolThis == 0)
            return 0;

        uintptr_t pos = 0;
        if (!MemUtil::ReadPtr32(hProcess, poolThis + g_Maple.CMob.CMobPoolList, pos) || pos == 0)
            return 0;

        size_t count = 0;

        while (pos != 0)
        {
            uintptr_t mobAddr = 0;
            if (!MemUtil::ZListGetNext32(hProcess, pos, mobAddr))
                break;

            if (mobAddr != 0)
            {
                CMob mob(mobAddr);
                ++count;
                if (!callback(mob))
                    break;
            }
        }

        return count;
    }
};