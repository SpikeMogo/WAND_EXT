
#pragma once

#include <Windows.h>
#include <cstdint>
#include <functional>
#include "MemoryUtil.h"
#include "MapleOffsets.h"
#include "CTemporaryStat.h"


class CTemporaryStatView
{
public:


    static size_t ForEachTemporaryStat(
    HANDLE hProcess,
    const std::function<bool(const CTemporaryStat&)>& callback)
    {
        if (!hProcess)
            return 0;

        uintptr_t contextAddr = 0;
        if (!MemUtil::RPM(hProcess, g_Maple.CWvsContext.CWvsContext, contextAddr) || !contextAddr)
            return 0;

        // CTemporaryStatView 
        uintptr_t StatViewThis = contextAddr + g_Maple.CTemporaryStatView.m_tempStatView;

        // Read m_lTemporaryStat._m_pHead at offset 0x10
        uintptr_t pos = 0;
        if (!MemUtil::ReadPtr32(
                hProcess,
                StatViewThis + g_Maple.CTemporaryStatView.m_temporaryStatList,  // 0x10
                pos) || pos == 0)
        {
            return 0;
        }
        
        size_t count = 0;

        while (pos != 0)
        {
            uintptr_t tsAddr = 0;
            if (!MemUtil::ZListGetNext32(hProcess, pos, tsAddr))
                break;

            if (tsAddr != 0)
            {
                CTemporaryStat ts(tsAddr);
                ++count;
                if (!callback(ts))
                    break;
            }
        }

        return count;
    }


};