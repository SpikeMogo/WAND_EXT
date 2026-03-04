#pragma once

#include <Windows.h>
#include <cstdint>
#include <functional>
#include "MemoryUtil.h"
#include "MapleOffsets.h"




class CNpc
{
public:
    uintptr_t remoteAddress;
    CNpc() : remoteAddress(0) {}
    explicit CNpc(uintptr_t addr) : remoteAddress(addr) {}

    bool Get(HANDLE hProcess, long& outX, long& outY, long& outXp, long& outYp) const
    {
        outX = outY = 0;
        outXp = outYp = 0;

        if (!hProcess || !remoteAddress)
            return false;

        int x = 0, y = 0;
        if (!MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.CNpc.posX, x))
            return false;
        if (!MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.CNpc.posY, y))
            return false;

        int xp = 0, yp = 0;
        if (!MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.CNpc.prevPosX, xp))
            return false;
        if (!MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.CNpc.prevPosY, yp))
            return false;

        outX = x;
        outY = y;
        outXp = xp;
        outYp = yp;
        return true;
    }

    bool GetTemplateId(HANDLE hProcess, uint32_t& outTemplateId) const
    {
        outTemplateId = 0;
        if (!hProcess || !remoteAddress)
            return false;

        // 1) Read pointer to CNpcTemplate
        uintptr_t pTemplate = 0;
        if (!MemUtil::RPM(hProcess,
                        remoteAddress + g_Maple.CNpc.CNpcTemplate,
                        pTemplate) || !pTemplate)
        {
            return false;
        }

        // 2) Read template ID (plain DWORD, not encrypted)
        uint32_t tid = 0;
        if (!MemUtil::RPM(hProcess, pTemplate + g_Maple.CNpc.TemplateID, tid))
            return false;

        outTemplateId = tid;
        return true;
    }


    bool GetTemplateName(HANDLE hProcess, std::string& outName) const
    {
        outName.clear();
        if (!hProcess || !remoteAddress)
            return false;

        // 1) Read pointer to CNpcTemplate
        uintptr_t pTemplate = 0;
        if (!MemUtil::RPM(hProcess,
                        remoteAddress + g_Maple.CNpc.CNpcTemplate,
                        pTemplate) || !pTemplate)
        {
            return false;
        }

        // 2) Address of ZXString<char> sName in CNpcTemplate
        uintptr_t zxStrAddr = pTemplate + g_Maple.CNpc.TemplateName;

        // 3) Read the string
        return MemUtil::ReadZXStringChar(hProcess, zxStrAddr, outName);
    }



};
