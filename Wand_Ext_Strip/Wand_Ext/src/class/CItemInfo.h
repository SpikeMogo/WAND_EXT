
#pragma once

#include <Windows.h>
#include <cstdint>
#include <functional>
#include "MemoryUtil.h"
#include "MapleOffsets.h"
#include <class/ZMap.h>





class CItemInfo
{
public:



    static bool GetItemStringByIdAndKey(
        HANDLE hProcess,
        int nItemID,
        const char* sKey,
        std::string& out)
    {
        out.clear();

        uintptr_t pCItemInfo = 0;
        if (!MemUtil::ReadPtr32(hProcess, g_Maple.CItemInfo.CItemInfo, pCItemInfo) || !pCItemInfo)
            return false;

        uintptr_t m_mItemString = pCItemInfo+g_Maple.CItemInfo.m_mItemString;
        // if (!MemUtil::ReadPtr32(hProcess, pCItemInfo+g_Maple.CItemInfo.m_mItemString,m_mItemString) || !m_mItemString)
        //     return false;

        uintptr_t innerMapAddr = 0;
        if (!MemUtil::ZMapGetValueAddr_U32Key(hProcess, m_mItemString , nItemID, innerMapAddr))
            return false;

        return MemUtil::GetItemStringFromInnerMap(hProcess, innerMapAddr, sKey, out);
    }


    static bool GetMapStringByIdAndKey(
        HANDLE hProcess,
        int nMapD,
        const char* sKey,
        std::string& out)
    {
        out.clear();

        uintptr_t pCItemInfo = 0;
        if (!MemUtil::ReadPtr32(hProcess, g_Maple.CItemInfo.CItemInfo, pCItemInfo) || !pCItemInfo)
            return false;

        uintptr_t m_mItemString = pCItemInfo+g_Maple.CItemInfo.m_mMapString;
        // if (!MemUtil::ReadPtr32(hProcess, pCItemInfo+g_Maple.CItemInfo.m_mItemString,m_mItemString) || !m_mItemString)
        //     return false;

        uintptr_t innerMapAddr = 0;
        if (!MemUtil::ZMapGetValueAddr_U32Key(hProcess, m_mItemString , nMapD, innerMapAddr))
            return false;

        return MemUtil::GetItemStringFromInnerMap(hProcess, innerMapAddr, sKey, out);
    }


};
