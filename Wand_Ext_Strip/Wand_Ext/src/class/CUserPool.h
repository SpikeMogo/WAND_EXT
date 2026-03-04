
#pragma once

#include <Windows.h>
#include <cstdint>
#include <functional>
#include "MemoryUtil.h"
#include "MapleOffsets.h"
#include <class/CUserRemote.h>
#include <vector>



class CUserPool
{
public:

    static size_t ForEachUser(HANDLE hProcess, const std::function<bool(const CUserRemote&)>& callback)
    {
        if (!hProcess)
            return 0;

        uintptr_t poolThis = 0;
        if (!MemUtil::ReadPtr32(hProcess, g_Maple.CUser.CUserPool, poolThis) || poolThis == 0)
            return 0;

        uintptr_t pos = 0;
        if (!MemUtil::ReadPtr32(hProcess, poolThis + g_Maple.CUser.CUserPoolList, pos) || pos == 0)
            return 0;

        size_t count = 0;

        while (pos != 0)
        {
            uintptr_t entryAddr = 0;  // this is USERREMOTE_ENTRY*
            if (!MemUtil::ZListGetNext32(hProcess, pos, entryAddr))
                break;

            if (entryAddr == 0)
                continue;

            // USERREMOTE_ENTRY::pUserRemote.p at +0x10
            uintptr_t remoteAddr = 0;
            if (!MemUtil::ReadPtr32(
                    hProcess,
                    entryAddr +  g_Maple.CUser.CUserRemoteEntryOffset,
                    remoteAddr) ||
                remoteAddr == 0)
            {
                continue;
            }

            CUserRemote user(remoteAddr);
            ++count;
            if (!callback(user))
                break;
        }

        return count;
    }

    static bool getPartyIDs(HANDLE hProcess, std::vector<uint32_t>& ids)
    {
        ids.clear(); 

        if (!hProcess)
            return false;


        uintptr_t contextAddr = 0;
        if (!MemUtil::RPM(hProcess, g_Maple.CWvsContext.CWvsContext, contextAddr) || !contextAddr)
            return false;

        // Now read the party base address: contextAddr + offset of m_party
        uintptr_t partyBase = contextAddr + g_Maple.CWvsContext.m_party;

        // Read 6 slots directly
        for (int i = 0; i < 6; i++)
        {
            uint32_t id = 0;
            MemUtil::RPM(hProcess, partyBase + i * 0x4, id);
            ids.push_back(id); 
        }

        return true;
    }



};