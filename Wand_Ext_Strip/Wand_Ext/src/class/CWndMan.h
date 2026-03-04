// Stripped for open-source release — type stubs only
#pragma once

#include <Windows.h>
#include <cstdint>
#include <vector>
#include <string>
#include "MemoryUtil.h"
#include "MapleOffsets.h"
#include "CInputSystem.h"

//=============================================================================
// ENUMS
//=============================================================================

enum class ActiveDialogType
{
    None,
    CUtilDlg,
    CUtilDlgEx,
    Other
};

enum class StatType
{
    HP = 0,
    MP = 1,
    STR = 2,
    DEX = 3,
    INT = 4,
    LUK = 5
};

struct UtilDlgEntry
{
    int nType;
    int nSelect;
    std::string text;
};

//=============================================================================
// CWndMan CLASS — Stripped for open-source release
// Implement your own window manager interaction here.
//=============================================================================

class CWndMan
{
public:
    //=========================================================================
    // INSTANCE ACCESS
    //=========================================================================

    static uintptr_t GetInstance(HANDLE hProcess)
    {
        if (!hProcess) return 0;
        uintptr_t pInstance = 0;
        MemUtil::ReadPtr32(hProcess, g_Maple.CWndMan.Instance, pInstance);
        return pInstance;
    }

    //=========================================================================
    // ACTIVE WINDOW / FOCUS
    //=========================================================================

    static uintptr_t GetActiveWnd(HANDLE hProcess)
    {
        uintptr_t pWndMan = GetInstance(hProcess);
        if (!pWndMan) return 0;
        uintptr_t pActiveWnd = 0;
        MemUtil::RPM(hProcess, pWndMan + g_Maple.CWndMan.m_pActiveWnd, pActiveWnd);
        return pActiveWnd;
    }

    static uintptr_t GetFocus(HANDLE hProcess)
    {
        uintptr_t pWndMan = GetInstance(hProcess);
        if (!pWndMan) return 0;
        uintptr_t pFocus = 0;
        MemUtil::RPM(hProcess, pWndMan + g_Maple.CWndMan.m_pFocus, pFocus);
        return pFocus;
    }

    static uintptr_t GetActiveWndVTable(HANDLE hProcess)
    {
        uintptr_t pActiveWnd = GetActiveWnd(hProcess);
        if (!pActiveWnd) return 0;
        uintptr_t vtable = 0;
        MemUtil::RPM(hProcess, pActiveWnd, vtable);
        return vtable;
    }

    //=========================================================================
    // FOCUS STATE
    //=========================================================================

    static bool IsFocused(HANDLE hProcess)
    {
        uintptr_t pWndMan = GetInstance(hProcess);
        if (!pWndMan) return false;
        uintptr_t pFocus = 0;
        MemUtil::RPM(hProcess, pWndMan + g_Maple.CWndMan.m_pFocus, pFocus);
        return pFocus == (pWndMan + 4);
    }

    static bool IfFieldReady(HANDLE hProcess) { return false; }
    static bool CanWeDoInput(HANDLE hProcess) { return false; }

    static bool IsFocusedOn(HANDLE hProcess, uintptr_t pCWnd) { return false; }

    //=========================================================================
    // DIALOG TYPE DETECTION
    //=========================================================================

    static ActiveDialogType GetActiveDialogType(HANDLE hProcess)
    {
        uintptr_t vtable = GetActiveWndVTable(hProcess);
        if (!vtable) return ActiveDialogType::None;
        if (vtable == g_Maple.CWndMan.CUtilDlg_VTable) return ActiveDialogType::CUtilDlg;
        if (vtable == g_Maple.CWndMan.CUtilDlgEx_VTable) return ActiveDialogType::CUtilDlgEx;
        return ActiveDialogType::Other;
    }

    static bool IsUtilDlgActive(HANDLE hProcess)
    {
        return GetActiveWndVTable(hProcess) == g_Maple.CWndMan.CUtilDlg_VTable;
    }

    static bool IsUtilDlgExActive(HANDLE hProcess)
    {
        return GetActiveWndVTable(hProcess) == g_Maple.CWndMan.CUtilDlgEx_VTable;
    }

    static bool IsAnyUtilDlgActive(HANDLE hProcess)
    {
        uintptr_t vtable = GetActiveWndVTable(hProcess);
        return vtable == g_Maple.CWndMan.CUtilDlg_VTable ||
               vtable == g_Maple.CWndMan.CUtilDlgEx_VTable;
    }

    //=========================================================================
    // UTILITY DIALOG ACCESS
    //=========================================================================

    static uintptr_t GetActiveUtilDlgEx(HANDLE hProcess) { return IsUtilDlgExActive(hProcess) ? GetActiveWnd(hProcess) : 0; }
    static uintptr_t GetActiveUtilDlg(HANDLE hProcess) { return IsUtilDlgActive(hProcess) ? GetActiveWnd(hProcess) : 0; }

    //=========================================================================
    // STRIPPED — Implement your own UI automation here
    //=========================================================================

    static bool ChangeChannel(HANDLE hProcess, uint32_t channel) { return false; }
    static std::vector<UtilDlgEntry> GetUtilDlgText(HANDLE hProcess) { return {}; }
    static bool SetUtilDlgExSelection(HANDLE hProcess, int selection) { return false; }

    //=========================================================================
    // CUIStat — Stripped
    //=========================================================================

    static uintptr_t GetCUIStat(HANDLE hProcess) { return 0; }
    static bool IsCUIStatOpen(HANDLE hProcess) { return false; }
    static bool FocusStatApButton(HANDLE hProcess, StatType stat) { return false; }

    //=========================================================================
    // CUISkill — Stripped
    //=========================================================================

    static uintptr_t GetCUISkill(HANDLE hProcess) { return 0; }
    static bool IsCUISkillOpen(HANDLE hProcess) { return false; }
    static bool SetScrollAndFocusForSkill(HANDLE hProcess, int targetSkillID) { return false; }
    static bool FocusSkillUpButton(HANDLE hProcess, int targetSkillID) { return false; }

    //=========================================================================
    // FOCUS RESET
    //=========================================================================

    // Stripped — implement your own focus reset
    static void resetFocus(HANDLE hProcess) {}

    //=========================================================================
    // MISC
    //=========================================================================

    static bool ReviveDead(HANDLE hProcess) { return false; }
    static bool ClickUtilDlgButton(HANDLE hProcess, bool clickYes) { return false; }

    //=========================================================================
    // DEBUG
    //=========================================================================

    static void DebugPrintUISkillEntries(HANDLE hProcess) {}
    static void DebugPrint(HANDLE hProcess) {}
};
