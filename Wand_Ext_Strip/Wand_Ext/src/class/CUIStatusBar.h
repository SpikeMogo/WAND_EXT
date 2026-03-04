#pragma once

#include <Windows.h>
#include <cstdint>
#include <vector>
#include <string>
#include <functional>
#include "MemoryUtil.h"
#include "MapleOffsets.h"
#include "ZRef.h"

/*
 * CUIStatusBar - Chat Log Reader
 * ==============================
 * 
 * Reads chat messages from MapleStory's CUIStatusBar chat log.
 * 
 * Memory Layout:
 * --------------
 * TSingleton<CUIStatusBar>::ms_pInstance at 0x00BEC208
 * 
 * Static member:
 *   CUIStatusBar::m_aChatLog - ZArray<ZRef<CChatLog>> at 0x00BF1100
 *   
 * ZArray layout:
 *   *(m_aChatLog)     = pointer to array data
 *   array[-4]         = count (max 64 entries)
 *   array[i]          = ZRef { int refCount; CChatLog* p; } (8 bytes each)
 * 
 * CChatLog structure (0x34 bytes):
 *   +0x00: vtable
 *   +0x04: _m_nRef (reference count)
 *   +0x08: (unknown)
 *   +0x0C: m_sChat (ZXString<wchar_t> - pointer to wchar_t string)
 *   +0x10: _ZtlSecureTear_m_nType (8 bytes, encrypted type value)
 *   +0x18: _ZtlSecureTear_m_nType_CS (checksum for type)
 *   +0x1C: m_nBack (background color)
 *   +0x20: m_nChannelID (-1 if none)
 *   +0x24: m_bWhisperIcon
 *   +0x28: m_bFirstLine
 *   +0x2C: m_pItem (ZRef<GW_ItemSlotBase>)
 * 
 * Chat Types:
 *   0  = Normal chat
 *   7  = System message
 *   8  = Notice/Event
 *   10 = Item link
 *   11 = Megaphone
 *   14 = Whisper (to)
 *   16 = Whisper (from)
 *   17 = Guild chat
 *   18 = Alliance chat
 *   19 = Party chat
 *   20 = Buddy chat
 *   23 = World megaphone
 *   24 = Super megaphone
 */


//=============================================================================
// DATA STRUCTURES
//=============================================================================

struct ChatLogEntry
{
    int32_t     index   = 0;
    int32_t     type    = 0;
    uint32_t    Back = 0;
    int32_t    ChannelID = 0;
    std::string content;
};

//=============================================================================
// CUIStatusBar CLASS
//=============================================================================

class CUIStatusBar
{
public:
    //-------------------------------------------------------------------------
    // Get singleton instance pointer
    //-------------------------------------------------------------------------
    static uintptr_t GetInstance(HANDLE hProcess)
    {
        if (!hProcess) return 0;
        
        uintptr_t pInstance = 0;
        MemUtil::RPM(hProcess, g_Maple.CUIStatusBar.Instance, pInstance);
        return pInstance;
    }

    //-------------------------------------------------------------------------
    // Get total number of chat log entries
    //-------------------------------------------------------------------------
    static bool GetChatLogCount(HANDLE hProcess, int32_t& outCount)
    {
        outCount = 0;
        if (!hProcess) return false;

        // Read array pointer from ZArray
        uintptr_t pArray = 0;
        if (!MemUtil::RPM(hProcess, g_Maple.CUIStatusBar.m_aChatLog, pArray) || !pArray)
            return false;

        // Count is stored at array[-4] (4 bytes before array start)
        return MemUtil::RPM(hProcess, pArray - sizeof(int32_t), outCount);
    }

    //-------------------------------------------------------------------------
    // Get a single chat log entry by index
    //-------------------------------------------------------------------------
    static bool GetChatLogEntry(HANDLE hProcess, int32_t index, ChatLogEntry& outEntry)
    {
        outEntry = {};
        if (!hProcess) return false;

        // Read array pointer
        uintptr_t pArray = 0;
        if (!MemUtil::RPM(hProcess, g_Maple.CUIStatusBar.m_aChatLog, pArray) || !pArray)
            return false;

        // Validate index against count
        int32_t count = 0;
        if (!MemUtil::RPM(hProcess, pArray - sizeof(int32_t), count))
            return false;

        if (index < 0 || index >= count)
            return false;

        // Read ZRef at index: { int refCount; CChatLog* p; } = 8 bytes
        // Pointer is at offset +4 within ZRef
        uintptr_t pChatLog = 0;
        if (!MemUtil::RPM(hProcess, pArray + index * sizeof(ZRef) + offsetof(ZRef, p), pChatLog) || !pChatLog)
            return false;

        outEntry.index = index;

        // Read type (ZtlSecureTear encrypted)
        int32_t checksum = 0;
        if (MemUtil::RPM(hProcess, pChatLog + g_Maple.CUIStatusBar.CChatLog.m_nType_CS, checksum))
        {
            MemUtil::ZtlSecureFuseI32(hProcess, pChatLog + g_Maple.CUIStatusBar.CChatLog.m_nType, checksum, outEntry.type);
        }

        // Read chat string (ZXString<wchar_t>)
        MemUtil::ReadZXStringWide(hProcess, pChatLog + g_Maple.CUIStatusBar.CChatLog.m_sChat, outEntry.content);

        MemUtil::RPM<uint32_t>(hProcess, pChatLog + g_Maple.CUIStatusBar.CChatLog.m_nBack,       outEntry.Back);
        MemUtil::RPM<int32_t>(hProcess, pChatLog + g_Maple.CUIStatusBar.CChatLog.m_nChannelID , outEntry.ChannelID);
        outEntry.ChannelID+=1;
        // printf("[%X][id %i][%s]\n",outEntry.Back,outEntry.ChannelID,outEntry.content.c_str());

        return true;
    }

    //-------------------------------------------------------------------------
    // Iterate over all chat log entries with callback
    // Returns number of entries visited
    // Callback returns false to stop iteration
    //-------------------------------------------------------------------------
    static size_t ForEachChatLog(
        HANDLE hProcess,
        const std::function<bool(int32_t index, const ChatLogEntry&)>& callback)
    {
        int32_t count = 0;
        if (!GetChatLogCount(hProcess, count) || count <= 0)
            return 0;

        size_t visited = 0;
        for (int32_t i = 0; i < count; ++i)
        {
            ChatLogEntry entry{};
            if (GetChatLogEntry(hProcess, i, entry))
            {
                ++visited;
                if (!callback(i, entry))
                    break;
            }
        }
        return visited;
    }

    //-------------------------------------------------------------------------
    // Iterate chat log entries, skipping noisy/system types
    // Filtered out: 8 (notice), 12 (?), 14 (whisper-to), 15 (?), 17 (guild), 18 (alliance)
    //-------------------------------------------------------------------------
    static size_t ForEachChatLogLean(
        HANDLE hProcess,
        const std::function<bool(int32_t index, const ChatLogEntry&)>& callback)
    {
        int32_t count = 0;
        if (!GetChatLogCount(hProcess, count) || count <= 0)
            return 0;

        size_t visited = 0;
        for (int32_t i = 0; i < count; ++i)
        {
            ChatLogEntry entry{};
            if (!GetChatLogEntry(hProcess, i, entry))
                continue;

            // Skip filtered types
            //0all
            //
            switch (entry.type)
            {
                case 8:  
                case 12:
                case 13:
                case 14: 
                case 15:
                case 17: 
                case 18: 
                    continue;
            }

            ++visited;
            if (!callback(i, entry))
                break;
        }
        return visited;
    }

    //-------------------------------------------------------------------------
    // Get all chat log entries as vector
    //-------------------------------------------------------------------------
    static std::vector<ChatLogEntry> GetAllChatLogs(HANDLE hProcess)
    {
        std::vector<ChatLogEntry> logs;

        ForEachChatLog(hProcess, [&](int32_t, const ChatLogEntry& entry) {
            logs.push_back(entry);
            return true;
        });

        return logs;
    }

    //-------------------------------------------------------------------------
    // Get latest N chat logs (most recent first)
    //-------------------------------------------------------------------------
    static std::vector<ChatLogEntry> GetRecentChatLogs(HANDLE hProcess, int32_t maxCount = 10)
    {
        std::vector<ChatLogEntry> logs;

        int32_t count = 0;
        if (!GetChatLogCount(hProcess, count) || count <= 0)
            return logs;

        int32_t start = (count > maxCount) ? (count - maxCount) : 0;

        // Iterate from newest to oldest
        for (int32_t i = count - 1; i >= start; --i)
        {
            ChatLogEntry entry{};
            if (GetChatLogEntry(hProcess, i, entry))
                logs.push_back(entry);
        }

        return logs;
    }

    //-------------------------------------------------------------------------
    // Find chat entries by type
    //-------------------------------------------------------------------------
    static std::vector<ChatLogEntry> FindChatLogsByType(HANDLE hProcess, int32_t chatType)
    {
        std::vector<ChatLogEntry> results;

        ForEachChatLog(hProcess, [&](int32_t, const ChatLogEntry& entry) {
            if (entry.type == chatType)
                results.push_back(entry);
            return true;
        });

        return results;
    }

    //-------------------------------------------------------------------------
    // Debug: Print all chat logs to console
    //-------------------------------------------------------------------------
    static void DebugPrintChatLogs(HANDLE hProcess)
    {
        int32_t count = 0;
        GetChatLogCount(hProcess, count);

        printf("\n[CUIStatusBar] === Chat Log (%d entries) ===\n", count);

        ForEachChatLog(hProcess, [](int32_t index, const ChatLogEntry& entry) {
            printf("[%d] Type:%d | %s\n", index, entry.type, entry.content.c_str());
            return true;
        });
    }
};