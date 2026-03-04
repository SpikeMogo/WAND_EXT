
#pragma once

#include <Windows.h>
#include <cstdint>
#include <functional>
#include "MemoryUtil.h"
#include "MapleOffsets.h"
#include <class/CNpc.h>




class CNpcPool
{
public:

    // NPCENTRY node layout (sizeof=0x1C):
    //   +0x00  __vftable
    //   +0x04  next          (ref_or_next)
    //   +0x08  prev
    //   +0x0C  gap           (ZRef padding)
    //   +0x10  pNpc          (ZRef<CNpc>.p)
    //   +0x14  posList
    //   +0x18  bFlag
    // This differs from the standard ZList node used by CMob/CDrop,
    // so we iterate manually instead of using ZListGetNext32.

    static size_t ForEachNpc(HANDLE hProcess, const std::function<bool(const CNpc&)>& callback)
    {
        if (!hProcess)
            return 0;

        uintptr_t poolThis = 0;
        if (!MemUtil::ReadPtr32(hProcess, g_Maple.CNpc.CNpcPool, poolThis) || poolThis == 0)
            return 0;

        // _m_pHead is at CNpcPool + 0x1C (m_lNpc) + 0x0C (_m_pHead) = +0x28
        uintptr_t pos = 0;
        if (!MemUtil::ReadPtr32(hProcess, poolThis + g_Maple.CNpc.CNpcPoolList, pos) || pos == 0)
            return 0;

        size_t count = 0;
        uintptr_t head = pos;

        while (pos != 0)
        {
            // Read CNpc* at NPCENTRY + 0x10
            uint32_t npcRaw = 0;
            if (!MemUtil::RPM<uint32_t>(hProcess, pos + 0x10, npcRaw))
                break;

            if (npcRaw != 0)
            {
                CNpc npc(static_cast<uintptr_t>(npcRaw));
                ++count;
                if (!callback(npc))
                    break;
            }

            // Read next at NPCENTRY + 0x04
            uint32_t nextRaw = 0;
            if (!MemUtil::RPM<uint32_t>(hProcess, pos + 0x04, nextRaw))
                break;

            if (nextRaw == 0 || static_cast<uintptr_t>(nextRaw) == head)
                break;  // end of list or looped back to head

            pos = static_cast<uintptr_t>(nextRaw);

            if (count > 200)  // safety limit
                break;
        }

        return count;
    }
};
