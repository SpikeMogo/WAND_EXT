#pragma once

#include <Windows.h>
#include <cstdint>
#include <functional>
#include "MemoryUtil.h"
#include "MapleOffsets.h"

#pragma pack(push, 1)
struct FuncKeySlot
{
    uint8_t  type;
    uint32_t data; // little endian
};
struct FuncKeyInfo
{
    int         idx;       // Maple slot index (0–88)
    UINT        vk;        // Win32 VK_ code
    const char* name;      // Display name
};

#pragma pack(pop)


static constexpr int KEY_SLOT_COUNT = 89;
static constexpr int SLOT_SIZE      = 5;


static constexpr FuncKeyInfo kFuncKeyInfoMap[] =
{

    // ─────────────────────────────
    // F1–F12
    // ─────────────────────────────
    { 59, VK_F1,           " F1  "  },
    { 60, VK_F2,           " F2  "  },
    { 61, VK_F3,           " F3  "  },
    { 62, VK_F4,           " F4  "  },
    { 63, VK_F5,           " F5  "  },
    { 64, VK_F6,           " F6  "  },
    { 65, VK_F7,           " F7  "  },
    { 66, VK_F8,           " F8  "  },
    { 67, VK_F9,           " F9  "  },
    { 68, VK_F10,          " F10 " },
    { 87, VK_F11,          " F11 " },
    { 88, VK_F12,          " F12 " },

    // ─────────────────────────────
    // Number row
    // ─────────────────────────────
    { 41, VK_OEM_3,        "  `  "  },
    {  2, '1',             "  1  "  },
    {  3, '2',             "  2  "  },
    {  4, '3',             "  3  "  },
    {  5, '4',             "  4  "  },
    {  6, '5',             "  5  "  },
    {  7, '6',             "  6  "  },
    {  8, '7',             "  7  "  },
    {  9, '8',             "  8  "  },
    { 10, '9',             "  9  "  },
    { 11, '0',             "  0  "  },
    { 12, VK_OEM_MINUS,    "  -  "  },
    { 13, VK_OEM_PLUS,     "  =  "  },

    // ─────────────────────────────
    // QWERTY row
    // ─────────────────────────────
    { 16, 'Q',             "  Q  "  },
    { 17, 'W',             "  W  "  },
    { 18, 'E',             "  E  "  },
    { 19, 'R',             "  R  "  },
    { 20, 'T',             "  T  "  },
    { 21, 'Y',             "  Y  "  },
    { 22, 'U',             "  U  "  },
    { 23, 'I',             "  I  "  },
    { 24, 'O',             "  O  "  },
    { 25, 'P',             "  P  "  },
    { 26, VK_OEM_4,        "  [  "  },
    { 27, VK_OEM_6,        "  ]  "  },
    { 43, VK_OEM_5,        "  \\ "  },

    // ─────────────────────────────
    // ASDF row
    // ─────────────────────────────
    { 30, 'A',             "  A  "   },
    { 31, 'S',             "  S  "   },
    { 32, 'D',             "  D  "   },
    { 33, 'F',             "  F  "   },
    { 34, 'G',             "  G  "   },
    { 35, 'H',             "  H  "   },
    { 36, 'J',             "  J  "   },
    { 37, 'K',             "  K  "   },
    { 38, 'L',             "  L  "   },
    { 39, VK_OEM_1,        "  ;  "   },
    { 40, VK_OEM_7,        "  '  "   },

    // ─────────────────────────────
    // ZXCV row
    // ─────────────────────────────
    { 42, VK_SHIFT,       "Shift"   },
    { 44, 'Z',             "  Z  "   },
    { 45, 'X',             "  X  "   },
    { 46, 'C',             "  C  "   },
    { 47, 'V',             "  V  "   },
    { 48, 'B',             "  B  "   },
    { 49, 'N',             "  N  "   },
    { 50, 'M',             "  M  "   },
    { 51, VK_OEM_COMMA,    "  ,  "   },
    { 52, VK_OEM_PERIOD,   "  .  "   },
    { 54, VK_SHIFT,       "Shift"   },


    // ─────────────────────────────
    // Bottom row
    // ─────────────────────────────
    { 29, VK_CONTROL,     " Ctrl"  },
    { 56, VK_MENU,        " Alt "   },
    { 57, VK_SPACE,        "Space" },



    // ─────────────────────────────
    // Insert/Home/Delete block
    // ─────────────────────────────
    { 82, VK_INSERT,       " Ins "  },
    { 71, VK_HOME,         " Home" },
    { 73, VK_PRIOR,        " PgUp" },
    { 83, VK_DELETE,       " Del "  },
    { 79, VK_END,          " End "  },
    { 81, VK_NEXT,         " PgDn" },
};



class CFuncKeyMapped
{
public:

    static UINT FuncKeyIndexToVK(int idx)
    {
        for (const auto& e : kFuncKeyInfoMap)
            if (e.idx == idx)
                return e.vk;

        return 0;
    }

    static int VKToFuncKeyIndex(UINT vk)
    {
        for (const auto& e : kFuncKeyInfoMap)
            if (e.vk == vk)
                return e.idx;

        return -1;
    }


    static const char* FuncKeyIndexToName(int idx)
    {
        for (const auto& e : kFuncKeyInfoMap)
            if (e.idx == idx)
                return e.name;

        return "None";
    }




    static void DumpFuncKeyMapRemote(HANDLE hProcess)
    {
        constexpr SIZE_T TOTAL_BYTES = KEY_SLOT_COUNT * SLOT_SIZE;

        // 1) Read the pointer stored at 0xBED5A0
        uintptr_t manPtr = 0;
        SIZE_T bytesRead = 0;

        if (!ReadProcessMemory(
                hProcess,
                reinterpret_cast<LPCVOID>(g_Maple.CFuncKeyMapped.CFuncKeyMappedMan),
                &manPtr,
                sizeof(manPtr),
                &bytesRead) || bytesRead != sizeof(manPtr) || !manPtr)
        {
            // DEBUG_PRINTF("Failed to RPM CFuncKeyMappedMan pointer\n");
            return;
        }

        // 2) Read the 89 * 5 bytes starting at (manPtr + 4)
        uint8_t buffer[TOTAL_BYTES] = {};
        if (!ReadProcessMemory(
                hProcess,
                reinterpret_cast<LPCVOID>(manPtr + 4),
                buffer,
                TOTAL_BYTES,
                &bytesRead) || bytesRead != TOTAL_BYTES)
        {
            // DEBUG_PRINTF("Failed to RPM func key map\n");
            return;
        }




        // 3) Dump
        for (int i = 0; i < KEY_SLOT_COUNT; ++i)
        {
            const uint8_t* p = buffer + i * SLOT_SIZE;

            uint8_t type = p[0];
            uint32_t val =
                static_cast<uint32_t>(p[1]) |
                (static_cast<uint32_t>(p[2]) << 8) |
                (static_cast<uint32_t>(p[3]) << 16) |
                (static_cast<uint32_t>(p[4]) << 24);

            int x = 0, y = 0;
            ReadProcessMemory(hProcess,
                            (LPCVOID)(g_Maple.CFuncKeyMapped.KeyUIPosX + sizeof(int) * (2 * i)),
                            &x,
                            sizeof(int),
                            nullptr);

            ReadProcessMemory(hProcess,
                            (LPCVOID)(g_Maple.CFuncKeyMapped.KeyUIPosY + sizeof(int) * (2 * i)),
                            &y,
                            sizeof(int),
                            nullptr);
                            

            app.logger.Addf(" [%2d](%6s)  type=%02d  data=%8u pos=(%4i, %4i)",
                        i,FuncKeyIndexToName(i), type, val, x,y);
        }
    }

    static bool GetVirtualKeyMappedRemote(HANDLE hProcess, uint8_t _type, uint32_t _data, UINT& VK)
    {
        constexpr SIZE_T TOTAL_BYTES = KEY_SLOT_COUNT * SLOT_SIZE;

        // 1) Read the pointer stored at 0xBED5A0
        uintptr_t manPtr = 0;
        SIZE_T bytesRead = 0;

        if (!ReadProcessMemory(
                hProcess,
                reinterpret_cast<LPCVOID>(g_Maple.CFuncKeyMapped.CFuncKeyMappedMan),
                &manPtr,
                sizeof(manPtr),
                &bytesRead) || bytesRead != sizeof(manPtr) || !manPtr)
        {
            // DEBUG_PRINTF("Failed to RPM CFuncKeyMappedMan pointer\n");
            return false;
        }

        // 2) Read the 89 * 5 bytes starting at (manPtr + 4)
        uint8_t buffer[TOTAL_BYTES] = {};
        if (!ReadProcessMemory(
                hProcess,
                reinterpret_cast<LPCVOID>(manPtr + 4),
                buffer,
                TOTAL_BYTES,
                &bytesRead) || bytesRead != TOTAL_BYTES)
        {
            // DEBUG_PRINTF("Failed to RPM func key map\n");
            return false;
        }




        // 3) Dump
        for (int i = 0; i < KEY_SLOT_COUNT; ++i)
        {
            const uint8_t* p = buffer + i * SLOT_SIZE;

            uint8_t type = p[0];
            uint32_t val =
                static_cast<uint32_t>(p[1]) |
                (static_cast<uint32_t>(p[2]) << 8) |
                (static_cast<uint32_t>(p[3]) << 16) |
                (static_cast<uint32_t>(p[4]) << 24);

            if(_type ==type && val == _data)
            {
                VK = FuncKeyIndexToVK(i);

                if(VK==0) return false;

                return true;
            }                            


        }

        return false;
    }


    // Stripped — implement your own key mapping write
    static bool SetVirtualKeyMappedRemote(HANDLE hProcess, UINT vk, uint8_t type, uint32_t data)
    {
        return false;
    }


};
