#pragma once
#include <windows.h>
#include <cstdint>
#include <vector>
#include <intrin.h> // for _rotl, _rotr on MSVC
#include <string>
#include <class/TSecType.h>
#include <class/ZMap.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")


struct ZXStringDataHeader
{
    int32_t  nRef;
    uint32_t nCap;
    uint32_t nByteLen;
};

namespace MemUtil
{
    template<typename T>
    inline bool RPM(HANDLE hProcess, uintptr_t addr, T& out)
    {
        if (!hProcess || addr == 0)
            return false;

        SIZE_T br = 0;
        if (!ReadProcessMemory(hProcess,
                               reinterpret_cast<LPCVOID>(addr),
                               &out,
                               sizeof(T),
                               &br))
            return false;

        return (br == sizeof(T));
    }

    template <typename T>
    static bool WPM(HANDLE hProcess, uintptr_t addr, const T& value)
    {
        SIZE_T bytesWritten = 0;
        if (!WriteProcessMemory(hProcess, (LPVOID)addr, &value, sizeof(T), &bytesWritten))
            return false;
        return bytesWritten == sizeof(T);
    }

    // Raw byte buffer read helper (no const issues)
    inline bool RPMBytes(HANDLE hProcess, uintptr_t addr, void* buffer, size_t len)
    {
        if (!hProcess || addr == 0 || !buffer || len == 0)
            return false;

        SIZE_T br = 0;
        if (!::ReadProcessMemory(
                hProcess,
                reinterpret_cast<LPCVOID>(addr),
                buffer,
                len,
                &br))
        {
            return false;
        }

        return (br == len);
    }



    template<typename T>
    inline bool ReadPointerChain32(
        HANDLE hProcess,
        uintptr_t baseAddr,              // address holding pointer
        const std::vector<uintptr_t>& offsets,
        T& outValue)
    {
        if (!hProcess || baseAddr == 0)
            return false;

        // First: dereference baseAddr -> initial pointer
        uint32_t ptr32 = 0;
        if (!RPM<uint32_t>(hProcess, baseAddr, ptr32) || ptr32 == 0)
            return false;

        uintptr_t cur = static_cast<uintptr_t>(ptr32);

        // No offsets: read T directly from cur
        if (offsets.empty())
            return RPM<T>(hProcess, cur, outValue);

        for (size_t i = 0; i < offsets.size(); ++i)
        {
            uintptr_t addr = cur + offsets[i];

            // Last offset: read final T value
            if (i + 1 == offsets.size())
            {
                return RPM<T>(hProcess, addr, outValue);
            }

            // Intermediate: read next pointer
            uint32_t nextPtr32 = 0;
            if (!RPM<uint32_t>(hProcess, addr, nextPtr32) || nextPtr32 == 0)
                return false;

            cur = static_cast<uintptr_t>(nextPtr32);
        }

        return false; // should not reach here
    }

    // Read a 32-bit pointer from addr and return it as uintptr_t.
    inline bool ReadPtr32(HANDLE hProcess, uintptr_t addr, uintptr_t& outPtr)
    {
        outPtr = 0;

        uint32_t tmp = 0;
        if (!RPM<uint32_t>(hProcess, addr, tmp))
            return false;              // only fail if RPM fails

        // 0 is a valid value for "no next node"
        outPtr = static_cast<uintptr_t>(tmp);
        return true;
    }

    inline bool ZListGetNext32(HANDLE hProcess, uintptr_t& pos, uintptr_t& outObjectPtr)
    {
        outObjectPtr = 0;

        if (!pos)
            return false;

        // 1) read object pointer: *(pos + 4)
        uint32_t mobRaw = 0;
        if (!RPM<uint32_t>(hProcess, pos + 4, mobRaw))
        {
            pos = 0;
            return false;
        }
        outObjectPtr = static_cast<uintptr_t>(mobRaw);

        // 2) read link pointer: *(pos - 16 + 4)
        uint32_t linkRaw = 0;
        if (!RPM<uint32_t>(hProcess, pos - 16 + 4, linkRaw))
        {
            pos = 0;
            return false;
        }

        if (linkRaw != 0)
            pos = static_cast<uintptr_t>(linkRaw) + 16;  // advance to next node
        else
            pos = 0;                                     // last node; next iter stops

        // IMPORTANT: this still returns true even when linkRaw == 0,
        // so the *current* node's mob is processed by the caller.
        return true;
    }



    inline uint32_t rotl32(uint32_t x, int r) {
        return _rotl(x, r);          // or implement manually if not MSVC
    }

    inline uint32_t rotr32(uint32_t x, int r) {
        return _rotr(x, r);
    }

    inline bool ZtlSecureFuseU32(HANDLE hProcess,
                                uintptr_t encSaltAddr,  // corresponds to a1
                                uint32_t checksum,      // corresponds to a2
                                uint32_t& out)
    {
        if (!hProcess || !encSaltAddr)
            return false;

        uint32_t enc  = 0; // v3
        uint32_t salt = 0; // v2

        // a1 points to enc, and a1+4 holds salt
        if (!MemUtil::RPM(hProcess, encSaltAddr,     enc))  return false;
        if (!MemUtil::RPM(hProcess, encSaltAddr + 4, salt)) return false;

        uint32_t decoded = enc ^ rotl32(salt, 5);

        // same integrity check as the game:
        if (salt + rotr32(enc ^ 0xBAADF00D, 5) != checksum)
            return false;

        out = decoded;
        return true;
    }

    // I32 version: same algorithm, signed output
    inline bool ZtlSecureFuseI32(HANDLE hProcess,
                                uintptr_t encSaltAddr,  // a1 in the game
                                int32_t checksum,       // a2 in the game
                                int32_t& out)
    {
        if (!hProcess || !encSaltAddr)
            return false;

        int32_t  enc  = 0;  // v3
        uint32_t salt = 0;  // v2

        // a1 points to enc, and a1+4 holds salt
        if (!MemUtil::RPM(hProcess, encSaltAddr,     enc))  return false;
        if (!MemUtil::RPM(hProcess, encSaltAddr + 4, salt)) return false;

        uint32_t v3 = static_cast<uint32_t>(enc);
        uint32_t v2 = salt;

        uint32_t decoded = v3 ^ rotl32(v2, 5);

        // same integrity check as the game
        if (v2 + rotr32(v3 ^ 0xBAADF00D, 5) != static_cast<uint32_t>(checksum))
            return false;

        out = static_cast<int32_t>(decoded);
        return true;
    }


    // External reimplementation of _ZtlSecureFuse<short>
    inline bool ZtlSecureFuseS16(HANDLE hProcess,
                                uintptr_t encBase,   // a1 in the decompile (v5 + 0x61)
                                int32_t checksum,    // a2 in the decompile (*(v5 + 0x65))
                                int16_t& out)
    {
        out = 0;
        if (!hProcess || !encBase)
            return false;

        // Read 4 bytes starting at a1 (v5+0x61): we need bytes at a1..a1+3
        uint32_t raw = 0;
        if (!MemUtil::RPM(hProcess, encBase, raw))
            return false;

        // Split into bytes like the game would see in memory
        uint8_t buf[4];
        buf[0] = static_cast<uint8_t>( raw        & 0xFF);
        buf[1] = static_cast<uint8_t>((raw >> 8)  & 0xFF);
        buf[2] = static_cast<uint8_t>((raw >> 16) & 0xFF);
        buf[3] = static_cast<uint8_t>((raw >> 24) & 0xFF);

        uint8_t* v2 = buf + 2;          // a1 + 2
        int      v3 = -1163005939;      // constant seed
        int      v6 = 0;

        // v10 = 2; loop twice
        for (int i = 0; i < 2; ++i)
        {
            int v5 = *v2;               // current byte (buf[2], then buf[3])
            int v9 = v3 ^ *(v2 - 2);    // XOR with byte 2 bytes before (buf[0], then buf[1])

            // v2[v4] = v5 ^ *(v2 - 2);
            // Analysis of the pattern shows this effectively writes back into (v2-2),
            // i.e. updates buf[0] then buf[1] in the two iterations:
            *(v2 - 2) = static_cast<uint8_t>(v5 ^ *(v2 - 2));

            v6 = v5 + rotr32(static_cast<uint32_t>(v9), 5);
            ++v2;
            v3 = v6;
        }

        // integrity check
        if (v6 != checksum)
            return false;

        // result is built into the first 2 bytes (buf[0], buf[1])
        int16_t val = 0;
        std::memcpy(&val, buf, sizeof(val));
        out = val;
        return true;
    }

    // U8 version: pointer-style, same pattern as ZtlSecureFuseI32
    inline bool ZtlSecureFuseU8(HANDLE hProcess,
                                uintptr_t encAddr,   // a1 in the game
                                int32_t checksum,    // a2 in the game
                                uint8_t& out)
    {
        out = 0;
        if (!hProcess || !encAddr)
            return false;

        uint8_t enc  = 0; // *a1
        uint8_t next = 0; // *(a1 + 1)

        // a1 points to first byte, a1+1 is second byte
        if (!MemUtil::RPM(hProcess, encAddr,     enc))  return false;
        if (!MemUtil::RPM(hProcess, encAddr + 1, next)) return false;

        // v2 = enc
        uint32_t v6 = static_cast<uint32_t>(enc) ^ 0xBAADF00D;  // v6 = *a1 ^ 0xBAADF00D

        // v3 = next ^ enc
        uint8_t decoded = static_cast<uint8_t>(next ^ enc);

        // v4 = ROR(v6, 5)
        uint32_t v4 = rotr32(v6, 5);

        // integrity check: next + v4 == a2
        if (static_cast<uint32_t>(next) + v4 != static_cast<uint32_t>(checksum))
            return false;

        out = decoded;
        return true;
    }


    // Double version of ZtlSecureFuse

    inline bool ZtlSecureFuseDouble(
        HANDLE hProcess,
        uintptr_t encSaltAddr,  // base address (a1 in decompile)
        uint32_t checksum,      // a2 in decompile
        double& out)
    {
        out = 0.0;
        if (!hProcess || !encSaltAddr)
            return false;

        // Read enc[0], enc[1], salt[0], salt[1]
        uint32_t enc[2]  = {0, 0};
        uint32_t salt[2] = {0, 0};

        if (!MemUtil::RPM(hProcess, encSaltAddr + 0x00, enc[0]))  return false;
        if (!MemUtil::RPM(hProcess, encSaltAddr + 0x04, enc[1]))  return false;
        if (!MemUtil::RPM(hProcess, encSaltAddr + 0x08, salt[0])) return false;
        if (!MemUtil::RPM(hProcess, encSaltAddr + 0x0C, salt[1])) return false;

        // Decode loop (2 iterations for double)
        uint32_t decoded[2] = {0, 0};
        uint32_t v14 = 0xBAADF00D;  // -1163005939 as unsigned

        for (int i = 0; i < 2; ++i)
        {
            uint32_t v4 = enc[i];       // *(v2 + a1) - encrypted value
            uint32_t v12 = salt[i];     // *v2 - salt

            uint32_t v5 = rotl32(v12, 5);
            uint32_t v10 = v14 ^ v4;

            decoded[i] = v4 ^ v5;       // *(v2 + v3) = v4 ^ v5

            uint32_t v6 = rotr32(v10, 5);
            uint32_t v7 = v12 + v6;

            v14 = v7;                   // update for next iteration / final checksum
        }

        // Integrity check
        if (v14 != checksum)
            return false;

        // Reconstruct double from two uint32_t
        uint64_t bits = static_cast<uint64_t>(decoded[0])
                      | (static_cast<uint64_t>(decoded[1]) << 32);

        std::memcpy(&out, &bits, sizeof(double));

        return true;
    }


    inline bool ReadZXStringWide(HANDLE hProcess, uintptr_t zxStringAddr, std::string& out)
    {
        out.clear();
        if (!hProcess || !zxStringAddr)
            return false;

        // 1) Read m_pStr
        uintptr_t pStr = 0;
        if (!MemUtil::RPM(hProcess, zxStringAddr, pStr) || !pStr)
            return false;

        // 2) Read header
        ZXStringDataHeader hdr{};
        uintptr_t hdrAddr = pStr - sizeof(ZXStringDataHeader);
        if (!MemUtil::RPM(hProcess, hdrAddr, hdr))
            return false;

        uint32_t byteLen = hdr.nByteLen;
        if (byteLen == 0 || byteLen > 4096)
            return false;

        // 3) Read wchar_t data
        std::vector<wchar_t> buf(byteLen / 2 + 1, 0);
        if (!MemUtil::RPMBytes(hProcess, pStr, buf.data(), byteLen))
            return false;

        buf[byteLen / 2] = 0;

        // 4) Convert to UTF-8
        int len = WideCharToMultiByte(CP_UTF8, 0, buf.data(), -1, nullptr, 0, nullptr, nullptr);
        if (len > 0)
        {
            out.resize(len - 1);
            WideCharToMultiByte(CP_UTF8, 0, buf.data(), -1, &out[0], len, nullptr, nullptr);
        }

        return !out.empty();
    }


    inline bool ReadZXStringChar(HANDLE hProcess, uintptr_t zxStringAddr, std::string& out)
    {
        out.clear();
        if (!hProcess || !zxStringAddr)
            return false;

        // 1) Read m_pStr
        uintptr_t pStr = 0;
        if (!MemUtil::RPM(hProcess, zxStringAddr, pStr) || !pStr)
            return false;

        // 2) Read header at (pStr - sizeof(ZXStringDataHeader))
        ZXStringDataHeader hdr{};
        uintptr_t hdrAddr = pStr - sizeof(ZXStringDataHeader);
        if (!MemUtil::RPM(hProcess, hdrAddr, hdr))
            return false;

        uint32_t len = hdr.nByteLen;
        if (len == 0 || len > 4096)  // sanity cap
            return false;

        // 3) Bulk-read the whole string
        std::string buf;
        buf.resize(len);  // &buf[0] is non-const char*

        if (!MemUtil::RPMBytes(hProcess, pStr, &buf[0], len))
            return false;

        out = std::move(buf);
        return true;
    }

    // Read a 0-terminated C-string from remote, with a safety maxLen
    inline bool ReadRemoteCString(
        HANDLE   hProcess,
        uintptr_t remotePtr,
        std::string& out,
        size_t   maxLen = 256)
    {
        out.clear();
        if (!hProcess || !remotePtr || maxLen == 0)
            return false;

        // Bulk-read up to maxLen bytes
        std::string buf;
        buf.resize(maxLen);   // &buf[0] is non-const char*

        SIZE_T bytesRead = 0;
        if (!::ReadProcessMemory(
                hProcess,
                reinterpret_cast<LPCVOID>(remotePtr),
                &buf[0],          // LPVOID-compatible pointer
                maxLen,
                &bytesRead) ||
            bytesRead == 0)
        {
            return false;
        }

        size_t validLen = static_cast<size_t>(bytesRead);

        // Find first '\0' within what we actually read
        size_t endPos = 0;
        while (endPos < validLen && buf[endPos] != '\0')
            ++endPos;

        if (endPos == validLen)
            return false;      // no terminator within range

        out.assign(buf.data(), endPos);
        return true;
    }



    inline bool ReadTSecTypeLong(
        HANDLE   hProcess,
        uintptr_t secTypeAddr,    // address of TSecType<long> in game
        long&    outValue)
    {
        // 1) Read TSecType<long> header
        TSecType secType{};
        if (!MemUtil::RPM(hProcess, secTypeAddr, secType))
            return false;

        if (!secType.secDataPtr)
            return false;

        // 2) Read TSecData<long>
        TSecData sd{};
        if (!MemUtil::RPM(hProcess, secType.secDataPtr, sd))
            return false;

        // 3) Decrypt 4 bytes using the decompiled GetData algorithm
        uint8_t key = sd.bKey;
        uint8_t enc[4];
        std::memcpy(enc, &sd.encData, 4);

        uint8_t plain[4];

        for (int i = 0; i < 4; ++i)
        {
            if (key == 0)
                key = 42;

            plain[i] = static_cast<uint8_t>(key ^ enc[i]);
            key = static_cast<uint8_t>(key + enc[i] + 42);
            // The original also updates a checksum here, but we don't
            // need that for just reading the value.
        }

        outValue =
            (long)plain[0]
            | ((long)plain[1] << 8)
            | ((long)plain[2] << 16)
            | ((long)plain[3] << 24);

        return true;
    }


    inline bool ReadTSecTypeDouble(
        HANDLE    hProcess,
        uintptr_t secTypeAddr,    // address of TSecType<double> in game
        double&   outValue)
    {
        // 1) Read TSecType<double> header
        TSecType secType{};
        if (!MemUtil::RPM(hProcess, secTypeAddr, secType))
            return false;

        if (!secType.secDataPtr)
            return false;

        // 2) Read TSecData<double>
        TSecDataDouble sd{};
        if (!MemUtil::RPM(hProcess, secType.secDataPtr, sd))
            return false;

        // 3) Decrypt 8 bytes using the decompiled GetData algorithm
        uint8_t key = sd.bKey;
        uint8_t enc[8];
        std::memcpy(enc, &sd.data, 8);

        uint8_t plain[8];

        for (int i = 0; i < 8; ++i)
        {
            if (key == 0)
                key = 42;

            plain[i] = static_cast<uint8_t>(key ^ enc[i]);
            key = static_cast<uint8_t>(key + enc[i] + 42);
        }

        // Reconstruct the double from decrypted bytes
        uint64_t bits =
            ((uint64_t)plain[0])
            | ((uint64_t)plain[1] << 8)
            | ((uint64_t)plain[2] << 16)
            | ((uint64_t)plain[3] << 24)
            | ((uint64_t)plain[4] << 32)
            | ((uint64_t)plain[5] << 40)
            | ((uint64_t)plain[6] << 48)
            | ((uint64_t)plain[7] << 56);

        std::memcpy(&outValue, &bits, sizeof(double));

        return true;
    }


    inline bool ZMapGetValueAddr_U32Key(
        HANDLE   hProcess,
        uintptr_t mapAddr,      // address of ZMap< uint32_t, T, ... >
        uint32_t key,
        uintptr_t& outValueAddr) // addr of value area at node + 12
    {
        outValueAddr = 0;

        if (!hProcess || !mapAddr)
            return false;

        ZMapHeader hdr{};
        if (!MemUtil::RPM(hProcess, mapAddr, hdr))
            return false;

        if (!hdr.pBuckets || hdr.bucketCount == 0)
            return false;

        uint32_t bucketIndex = MemUtil::rotr32(key, 5) % hdr.bucketCount;

        uintptr_t bucketHead = 0;
        if (!MemUtil::RPM(
                hProcess,
                hdr.pBuckets + bucketIndex * sizeof(uintptr_t),
                bucketHead))
        {
            return false;
        }

        uintptr_t nodeAddr = bucketHead;
        while (nodeAddr)
        {
            ZMapNode node{};
            if (!MemUtil::RPM(hProcess, nodeAddr, node))
                return false;

            if (static_cast<uint32_t>(node.key) == key)
            {
                // value (ZRef<...> or inner ZMap) lives here:
                outValueAddr = nodeAddr + sizeof(ZMapNode);
                return true;
            }

            nodeAddr = node.pNext;
        }

        return false; // not found
    }



    // 2) From inner string map, find entry by key and read ZXString<char> value
    inline bool GetItemStringFromInnerMap(
        HANDLE hProcess,
        uintptr_t innerMapAddr,
        const char* sKey,
        std::string& out)
    {
        out.clear();
        if (!innerMapAddr || !sKey)
            return false;

        ZMapHeader hdr{};
        if (!MemUtil::RPM(hProcess, innerMapAddr, hdr))
            return false;

        if (!hdr.pBuckets || hdr.bucketCount == 0)
            return false;

        std::string remoteKeyBuf;

        for (uint32_t bi = 0; bi < hdr.bucketCount; ++bi)
        {
            uintptr_t bucketHead = 0;
            if (!MemUtil::RPM(hProcess,
                            hdr.pBuckets + bi * sizeof(uintptr_t),
                            bucketHead))
            {
                return false;
            }

            uintptr_t nodeAddr = bucketHead;
            while (nodeAddr)
            {
                ZMapNode_Str_ToZXString node{};
                if (!MemUtil::RPM(hProcess, nodeAddr, node))
                    return false;

                // compare key strings
                if (node.pKeyCStr)
                {
                    if (!ReadRemoteCString(hProcess, node.pKeyCStr, remoteKeyBuf))
                    {
                        // Failed to read key string, skip this node safely
                        nodeAddr = node.pNext;
                        continue;
                    }

                    if (remoteKeyBuf == sKey)
                    {
                        // Use sizeof(...) instead of hardcoded 12
                        uintptr_t zxStringAddr = nodeAddr + sizeof(ZMapNode_Str_ToZXString);
                        return ReadZXStringChar(hProcess, zxStringAddr, out);
                    }
                }

                nodeAddr = node.pNext;
            }
        }

        return false; // key not found
    }


    inline std::pair<double, double> GetMemoryUsageMB()
    {
        PROCESS_MEMORY_COUNTERS_EX pmc = {};

        if (!GetProcessMemoryInfo(GetCurrentProcess(),
                                  (PROCESS_MEMORY_COUNTERS*)&pmc,
                                  sizeof(pmc)))
        {
            return {0.0, 0.0};
        }

        double workingMB  = pmc.WorkingSetSize / (1024.0 * 1024.0);
        double privateMB  = pmc.PrivateUsage   / (1024.0 * 1024.0);

        return {workingMB, privateMB};
    }



}
