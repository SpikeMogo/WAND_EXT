// Stripped for open-source release — type stubs only
#pragma once

#include <Windows.h>
#include <cstdint>
#include <vector>
#include <functional>
#include <string>
#include "MemoryUtil.h"
#include "MapleOffsets.h"
#include "CInputSystem.h"
#include "CWndMan.h"
#include <class/CItemInfo.h>

//=============================================================================
// DATA STRUCTURES
//=============================================================================

struct ShopItem
{
    int32_t     itemId   = 0;
    int32_t     position = 0;
    int32_t     price    = 0;
    std::string name;
};

struct SellItem
{
    uint32_t    itemID   = 0;
    int32_t     slot     = 0;
    int32_t     price    = 0;
    int32_t     quantity = 0;
    std::string name;
};

struct CtrlRect
{
    int32_t x      = 0;
    int32_t y      = 0;
    int32_t width  = 0;
    int32_t height = 0;
};

//=============================================================================
// CShopDlg CLASS — Stripped for open-source release
// Implement your own shop dialog interaction here.
//=============================================================================

class CShopDlg
{
public:
    static uintptr_t GetInstance(HANDLE hProcess) { return 0; }
    static bool IsShopDialog(HANDLE hProcess) { return false; }

    static bool GetNpcTemplateId(HANDLE hProcess, int32_t& outNpcId) { outNpcId = 0; return false; }

    static bool GetItemCount(HANDLE hProcess, int32_t& outCount) { outCount = 0; return false; }
    static bool GetShopItem(HANDLE hProcess, int32_t index, ShopItem& outItem) { outItem = {}; return false; }
    static size_t ForEachShopItem(HANDLE hProcess, const std::function<bool(int32_t, const ShopItem&)>& callback) { return 0; }
    static int32_t FindItemByItemId(HANDLE hProcess, int32_t targetItemId) { return -1; }

    static bool GetSellItemCount(HANDLE hProcess, int32_t& outCount) { outCount = 0; return false; }
    static bool GetSellItem(HANDLE hProcess, int32_t index, SellItem& outItem) { outItem = {}; return false; }
    static size_t ForEachSellItem(HANDLE hProcess, const std::function<bool(int32_t, const SellItem&)>& callback) { return 0; }
    static int32_t FindSellItemByItemId(HANDLE hProcess, int32_t targetItemId) { return -1; }

    static bool OpenShop(HANDLE hProcess) { return false; }
    static bool PressButton(HANDLE hProcess, uintptr_t pShop, const char* buttonName) { return false; }

    static bool BuySelectedItem(AppState& app, int32_t targetItemId, int32_t quantity = 1) { return false; }
    static bool SellSelectedItem(AppState& app, int32_t tab, int32_t index) { return false; }
    static bool SellAllItem(AppState& app, int32_t tab, std::vector<int32_t> excludeIDs) { return false; }

    static void DebugPrint(HANDLE hProcess) {}
    static void DebugPrintButtons(HANDLE hProcess) {}
    static void DebugPrintShopWindowRect(HANDLE hProcess) {}

private:
    static uintptr_t GetValidShop(HANDLE hProcess) { return 0; }
    static bool GetArrayCount(HANDLE hProcess, uintptr_t pShop, uintptr_t arrayOffset,
                              uintptr_t& outArray, int32_t& outCount) { outArray = 0; outCount = 0; return false; }
    static bool ReadIWzVector2DPos(HANDLE hProcess, uintptr_t pVector2D,
                                   int32_t& outX, int32_t& outY) { outX = outY = 0; return false; }
    static bool GetCtrlRect(HANDLE hProcess, uintptr_t pCtrl, CtrlRect& outRect) { outRect = {}; return false; }
    static bool GetShopWindowRect(HANDLE hProcess, uintptr_t pShop, CtrlRect& outRect) { outRect = {}; return false; }
    static bool GetButtonCenter(HANDLE hProcess, uintptr_t pShop, const char* buttonName,
                                int32_t& outX, int32_t& outY) { outX = outY = 0; return false; }
    static bool GetTabPos(HANDLE hProcess, uintptr_t pShop, int tab, int32_t& outX, int32_t& outY) { outX = outY = 0; return false; }
    static int32_t GetCurTab(HANDLE hProcess, uintptr_t pShop) { return -1; }
    static void ClearDialog(AppState& app) {}
    static bool HandleConfirmDialogSell(AppState& app) { return false; }
    static bool HandleConfirmDialog(AppState& app, int32_t quantity = 1) { return false; }
};
