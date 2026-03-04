#pragma once

#include <Windows.h>
#include <cstdint>
#include <string>
#include "MemoryUtil.h"
#include "MapleOffsets.h"

/*
    CPet - External pet data reader

    CUser holds up to 3 active pets in m_apPet (ZArray<ZRef<CPet>>).
    Each ZRef<CPet> is 8 bytes: { int refCount, uintptr_t p }.

    CPet layout (from IDA):
        +0x94  m_pOwner        (CUser*)  - owner pointer
        +0x9C  m_nPetIndex     (int)     - slot index 0/1/2
        +0xA8  m_nTameness     (int)     - closeness (decoded)
        +0xAC  m_nRepleteness  (int)     - fullness (decoded)
        +0xB0  m_nPetAttribute (int)     - pet attribute flags (decoded)

    These values are plain ints cached by CPet::OnValidateStat(),
    which decodes them from GW_ItemSlotPet's ZtlSecureTear fields.
*/

class CPet
{
public:
    uintptr_t remoteAddress;

    CPet() : remoteAddress(0) {}
    explicit CPet(uintptr_t addr) : remoteAddress(addr) {}

    bool IsValid() const { return remoteAddress != 0; }

    // Read pet name
    bool GetName(HANDLE hProcess, std::string& outName) const
    {
        outName.clear();
        if (!hProcess || !remoteAddress)
            return false;

        return MemUtil::ReadZXStringChar(hProcess, remoteAddress + g_Maple.CPet.m_sName, outName);
    }

    // Read pet slot index (0, 1, or 2)
    bool GetPetIndex(HANDLE hProcess, int& outIndex) const
    {
        outIndex = -1;
        if (!hProcess || !remoteAddress)
            return false;

        return MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.CPet.m_nPetIndex, outIndex);
    }

    // Read tameness (closeness)
    bool GetTameness(HANDLE hProcess, int& outTameness) const
    {
        outTameness = 0;
        if (!hProcess || !remoteAddress)
            return false;

        return MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.CPet.m_nTameness, outTameness);
    }

    // Read repleteness (fullness)
    bool GetRepleteness(HANDLE hProcess, int& outRepleteness) const
    {
        outRepleteness = 0;
        if (!hProcess || !remoteAddress)
            return false;

        return MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.CPet.m_nRepleteness, outRepleteness);
    }

    // Read pet attribute flags
    bool GetPetAttribute(HANDLE hProcess, int& outAttribute) const
    {
        outAttribute = 0;
        if (!hProcess || !remoteAddress)
            return false;

        return MemUtil::RPM<int>(hProcess, remoteAddress + g_Maple.CPet.m_nPetAttribute, outAttribute);
    }
};
