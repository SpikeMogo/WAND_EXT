
#pragma once

#include <Windows.h>
#include <cstdint>
#include <functional>
#include "MemoryUtil.h"
#include "MapleOffsets.h"
#include <class/CDrop.h>




class CDropPool
{
public:

    static size_t ForEachDrop(HANDLE hProcess, const std::function<bool(const CDrop&)>& callback)
    {
        if (!hProcess)
            return 0;

        uintptr_t poolThis = 0;
        if (!MemUtil::ReadPtr32(hProcess, g_Maple.CDrop.CDropPool, poolThis) || poolThis == 0)
            return 0;

        uintptr_t pos = 0;
        if (!MemUtil::ReadPtr32(hProcess, poolThis + g_Maple.CDrop.CDropPoolList, pos) || pos == 0)
            return 0;

        size_t count = 0;

        while (pos != 0)
        {
            uintptr_t dropAddr = 0;
            if (!MemUtil::ZListGetNext32(hProcess, pos, dropAddr))
                break;

            if (dropAddr != 0)
            {
                CDrop drop(dropAddr);
                ++count;
                if (!callback(drop))
                    break;
            }
        }

        return count;
    }
};