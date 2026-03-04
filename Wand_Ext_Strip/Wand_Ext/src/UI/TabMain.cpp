// TabMain.cpp
#include <UI/TabMain.h>
#include <imgui/imgui.h>
#include "MemoryUtil.h"
#include <class/MapleClass.h>
#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <string>
#include <navigator/Map.h>
#include <wzparser/WzParser.h>
#include <UI/TabWzBrowser.h>
#include <wzparser/MapData.h>

// Window info for the picker
struct WindowItem
{
    HWND        hwnd;
    DWORD       pid;
    std::string title;
    std::string className;
};

// Helper: check if a PID is 32-bit (Wow64) on a 64-bit OS
static bool IsWow64Process32(DWORD pid, bool& outWow32)
{
    outWow32 = false;
    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!h)
        return false;

    BOOL isWow = FALSE;
    BOOL ok = IsWow64Process(h, &isWow);
    CloseHandle(h);

    if (!ok)
        return false;

    outWow32 = (isWow != FALSE);
    return true;
}

// Helper: enumerate visible windows from 32-bit processes
static void RefreshWindowList(std::vector<WindowItem>& list)
{
    list.clear();

    struct EnumData
    {
        std::vector<WindowItem>* pList;
    } data{ &list };

    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL
    {
        auto* d = reinterpret_cast<EnumData*>(lParam);

        // Must be visible
        if (!IsWindowVisible(hwnd))
            return TRUE;

        // Must be a top-level window (no owner)
        if (GetWindow(hwnd, GW_OWNER) != nullptr)
            return TRUE;

        // Get window title
        char title[256] = {0};
        GetWindowTextA(hwnd, title, sizeof(title));
        
        // Skip windows without titles (usually not game windows)
        if (strlen(title) == 0)
            return TRUE;

        // Skip IME/system windows
        if (strstr(title, "Default IME") || strstr(title, "MSCTFIME"))
            return TRUE;

        // Get class name
        char className[256] = {0};
        GetClassNameA(hwnd, className, sizeof(className));

        // Get PID
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);

        // Check if it's a 32-bit process
        bool isWow32 = false;
        if (!IsWow64Process32(pid, isWow32) || !isWow32)
            return TRUE; // skip non-32-bit

        // Get process name
        char procName[MAX_PATH] = "Unknown";
        HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (hProc)
        {
            DWORD size = MAX_PATH;
            QueryFullProcessImageNameA(hProc, 0, procName, &size);
            CloseHandle(hProc);
            
            // Extract just the filename
            const char* lastSlash = strrchr(procName, '\\');
            if (lastSlash)
                memmove(procName, lastSlash + 1, strlen(lastSlash));
        }

        WindowItem item;
        item.hwnd = hwnd;
        item.pid = pid;
        item.title = title;
        item.className = "[" + std::string(procName) + "]";
        
        d->pList->push_back(item);

        return TRUE; // continue enumeration

    }, reinterpret_cast<LPARAM>(&data));
}



void DrawMainTab(AppState& app, ImGuiIO& io)
{
    // Static UI state for the picker
    static bool                   pickerOpen   = false;
    static std::vector<WindowItem> windowList;
    static int                    currentIndex = -1;

    ImGui::Separator();

    // Show currently selected window/process
    if (app.selectedPid != 0 && app.hGameWnd)
    {
        char title[256] = {0};
        GetWindowTextA(app.hGameWnd, title, sizeof(title));
        
        ImGui::Text("Target: [%s] PID:%lu HWND:0x%X",
                    strlen(title) > 0 ? title : app.selectedName,
                    static_cast<unsigned long>(app.selectedPid),
                    (unsigned)(uintptr_t)app.hGameWnd);

        auto [working, priv] = MemUtil::GetMemoryUsageMB();
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Mem Usage: [%.2f MB/%.2f MB]\n", priv,working);

        // Map loading progress
        if (mapdata::GetMapDataCache().GetTotalMaps() > 0 && !mapdata::GetMapDataCache().IsLoaded()) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Loading Maple Worlds Structure...");
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "(WZ reading will use significant memory temporarily)");
            
            float progress = mapdata::GetMapDataCache().GetLoadProgress();
            int loaded = mapdata::GetMapDataCache().GetLoadedMaps();
            int total = mapdata::GetMapDataCache().GetTotalMaps();
            ImGui::ProgressBar(progress, ImVec2(200, 0));
            ImGui::SameLine();
            ImGui::Text("%d/%d", loaded, total);
        }


        if (!IsProcessRunning(app.selectedPid))
        {
            // Process is gone → reset
            app.selectedPid = 0;
            app.selectedName[0] = '\0';
            app.hGameWnd = nullptr;
        }
    }
    else
    {
        ImGui::Text("Target: <none>");
    }

    // Button to open the picker
    if (!pickerOpen)
    {
        if (ImGui::Button("Select Maple Window..."))
        {
            RefreshWindowList(windowList);
            currentIndex = -1;
            pickerOpen   = true;
        }
    }
    else
    {
        ImGui::Separator();
        ImGui::Text("Choose a game window (32-bit processes only):");
        ImGui::Spacing();

        // Window list
        ImGui::BeginChild("window_list", ImVec2(0, 260), true);

        for (int i = 0; i < static_cast<int>(windowList.size()); ++i)
        {
            const WindowItem& w = windowList[i];

            char label[512];
            snprintf(label, sizeof(label), "• %s\n   -%s\n   -(PID %X, HWND 0x%08X)",
                    w.title.c_str(),
                    w.className.c_str(),
                    static_cast<unsigned long>(w.pid),
                    (unsigned)(uintptr_t)w.hwnd);

            bool selected = (i == currentIndex);
            if (ImGui::Selectable(label, selected, 0, ImVec2(0, 36)))
            {
                currentIndex = i;
            }
        }

        ImGui::EndChild();

        ImGui::Spacing();

        // Action buttons
        bool disableSelect = (currentIndex < 0 || currentIndex >= (int)windowList.size());

        if (!disableSelect)
        {
            if (ImGui::Button("Select This Window"))
            {
                const WindowItem& sel = windowList[currentIndex];
                
                // Store the window and PID
                app.hGameWnd = sel.hwnd;
                app.selectedPid = sel.pid;
                
                // Get process name for display
                char procName[MAX_PATH] = {0};
                HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, sel.pid);
                if (hProc)
                {
                    DWORD size = MAX_PATH;
                    QueryFullProcessImageNameA(hProc, 0, procName, &size);
                    CloseHandle(hProc);
                    
                    const char* lastSlash = strrchr(procName, '\\');
                    if (lastSlash)
                        strncpy_s(app.selectedName, lastSlash + 1, _TRUNCATE);
                    else
                        strncpy_s(app.selectedName, procName, _TRUNCATE);
                }

                // Clean up old process handle
                if (app.hProcess)
                {
                    // Hooks stripped for open-source release
                    CloseHandle(app.hProcess);
                    app.hProcess = nullptr;
                    OnProcessDetachedWZ();
                }

                // Open new process handle
                app.hProcess = OpenProcess(
                    PROCESS_ALL_ACCESS,
                    FALSE,
                    app.selectedPid
                );

                if (!app.hProcess)
                {
                    app.logger.AddE("Failed to open process PID %lu",
                                    static_cast<unsigned long>(app.selectedPid));
                    app.selectedPid = 0;
                    app.selectedName[0] = '\0';
                    app.hGameWnd = nullptr;
                }
                else
                {
                    app.logger.Addf("Selected window: '%s' (PID %lu, HWND 0x%08X)",
                                    sel.title.c_str(),
                                    static_cast<unsigned long>(app.selectedPid),
                                    (unsigned)(uintptr_t)app.hGameWnd);


                    char procPath[MAX_PATH] = {0};
                    DWORD size = MAX_PATH;
                    QueryFullProcessImageNameA(app.hProcess, 0, procPath, &size);


                    //clear map;
                    MapleMap::MapID=0;
                    

                    #ifndef _DEBUG
                        OnProcessAttachedWZ(procPath);
                    #endif 

                    // Hooks stripped for open-source release
                    // Implement your own hook initialization here

                }

                pickerOpen = false;
            }
        }
        else
        {
            ImGui::BeginDisabled();
            ImGui::Button("Select This Window");
            ImGui::EndDisabled();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel"))
        {
            pickerOpen = false;
        }

        ImGui::SameLine();
        
        if (ImGui::Button("Refresh List"))
        {
            RefreshWindowList(windowList);
            currentIndex = -1;
        }
    }




        //log
        ImGui::SeparatorText("Log:");

        // --- tighten padding just for the log child ---
        ImGuiStyle& style = ImGui::GetStyle();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, style.WindowPadding.y));


        // Fixed-size log window with scrollbars
        ImGui::BeginChild(
            "log_region",
            ImVec2(0, 300),                     // width = fill, height = 300 px
            true,
            ImGuiWindowFlags_HorizontalScrollbar);

        // Make a copy of lines to render without holding the lock
        std::vector<AppLogger::LogLine> linesCopy;
        int selectedIndexCopy = -1;
        {
            std::lock_guard<std::mutex> lock(app.logger.mutex);
            linesCopy         = app.logger.lines;
            selectedIndexCopy = app.logger.selectedIndex;
        }


        ImGui::SetWindowFontScale(0.94f);

        int idx = 0;
        for (const auto& ll : linesCopy)
        {
            ImGui::PushID(idx);
            bool isSelected = (idx == selectedIndexCopy);

            if (ImGui::Selectable("##row", isSelected,
                                ImGuiSelectableFlags_SpanAllColumns |
                                ImGuiSelectableFlags_AllowOverlap))
            {
                std::lock_guard<std::mutex> lock(app.logger.mutex);
                app.logger.selectedIndex = idx;
            }

            bool rowHovered       = ImGui::IsItemHovered();
            bool rowDoubleClicked = rowHovered && ImGui::IsMouseDoubleClicked(0);

            ImGui::SameLine();

            // Pick message color by type
            ImU32 msgColor = IM_COL32(255, 255, 255, 255);
            switch (ll.color)
            {
                case AppLogger::LogColor::Verbose:
                    msgColor = IM_COL32(255, 190, 11, 255);
                    break;
                case AppLogger::LogColor::Error:
                    msgColor = IM_COL32(255, 120, 120, 255);
                    break;
                default:
                    break;
            }

            // Timestamp in blue
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.85f, 1.0f, 1.0f));
            ImGui::TextUnformatted(ll.timestamp.c_str());
            ImGui::PopStyleColor();

            ImGui::SameLine();

            // Message in appropriate color
            ImGui::PushStyleColor(ImGuiCol_Text, msgColor);
            ImGui::TextUnformatted(ll.text.c_str());
            ImGui::PopStyleColor();

            // Show repeat count if > 1
            if (ll.repeatCount > 1)
            {
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                ImGui::Text("(x%d)", ll.repeatCount);
                ImGui::PopStyleColor();
            }

            if (rowHovered)
                ImGui::SetTooltip("Double-click to copy");

            if (rowDoubleClicked)
            {
                std::string fullLine = ll.timestamp + ll.text;
                ImGui::SetClipboardText(fullLine.c_str());
                std::lock_guard<std::mutex> lock(app.logger.mutex);
                app.logger.selectedIndex = idx;
            }

            ImGui::PopID();
            ++idx;
        }


        ImGui::SetWindowFontScale(1.0f);

        // ---- Auto-scroll ----
        // Only keep at bottom if we were already at bottom.
        if (app.logger.autoScroll)
        {
            float scrollY  = ImGui::GetScrollY();
            float maxY     = ImGui::GetScrollMaxY();
            if (scrollY >= maxY - 1.0f)     // small epsilon
            {
                ImGui::SetScrollHereY(1.0f);
            }
        }

        ImGui::EndChild();
        ImGui::PopStyleVar(1);

        // Controls under the log
        if (ImGui::Button("Clear log"))
        {
            app.logger.Clear();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Copy all"))
        {
            std::string clipboardText;

            {
                std::lock_guard<std::mutex> lock(app.logger.mutex);

                size_t total = 0;
                for (const auto& ll : app.logger.lines)
                    total += ll.timestamp.size() + ll.text.size() + 16;

                clipboardText.reserve(total);

                for (const auto& ll : app.logger.lines)
                {
                    clipboardText.append(ll.timestamp);
                    clipboardText.append(ll.text);
                    if (ll.repeatCount > 1)
                    {
                        clipboardText.append(" (x");
                        clipboardText.append(std::to_string(ll.repeatCount));
                        clipboardText.append(")");
                    }
                    clipboardText.push_back('\n');
                }
            }

            if (!clipboardText.empty())
            {
                ImGui::SetClipboardText(clipboardText.c_str());
            }
        }


        ImGui::SameLine();
        ImGui::Checkbox("Auto scroll", &app.logger.autoScroll);



        // static char offset1Buf[32] = "0xC";
        // static char offset2Buf[32] = "0x14";
        // ImGui::InputText("Offset 1", offset1Buf, IM_ARRAYSIZE(offset1Buf));
        // ImGui::InputText("Offset 2", offset2Buf, IM_ARRAYSIZE(offset2Buf));

        #ifdef _DEBUG
            ImGui::PushItemWidth(100.0f);
            static int s_test_i_1 = 0;
            static int s_test_i_2 = 0;
            // // In your ImGui render loop
            ImGui::InputInt("i1", &s_test_i_1);
            ImGui::SameLine();
            ImGui::InputInt("i2", &s_test_i_2);
            ImGui::SameLine();
            ImGui::PopItemWidth();
        #endif

        ImGui::SameLine();
        if (ImGui::Button("Test"))
        {
            // uint32_t channel=0;
            // CUserLocal::GetChannel(app.hProcess,channel);
            // app.logger.Addf("%i",channel);


            // CWndMan::DebugPrint(app.hProcess);

            // CShopDlg::DebugPrint(app.hProcess);
            // CShopDlg::DebugPrintButtons(app.hProcess);
            // CWndMan::resetFocus(app.hProcess);
            // CUIStatusBar::DebugPrintChatLogs(app.hProcess);



            // CShopDlg::DebugPrintShopWindowRect(app.hProcess);
                // CShopDlg::OpenShop(app.hProcess);
            // CShopDlg::DebugWindowPos(app.hProcess);
// 
            // --- Pet Test ---
            {
                CPet pets[3];
                int petCount = CUserLocal::GetActivePets(app.hProcess, pets);
                app.logger.Addf("[Pet] Active pets: %d", petCount);
                for (int i = 0; i < 3; i++)
                {
                    if (!pets[i].IsValid()) continue;
                    std::string name;
                    int rep = 0;
                    pets[i].GetName(app.hProcess, name);
                    pets[i].GetRepleteness(app.hProcess, rep);
                    app.logger.Addf("[Pet %d] %s  fullness=%d",
                        i, name.c_str(), rep);
                }
            }

            CWndMan::DebugPrint(app.hProcess);
            // CShopDlg::BuySelectedItem(app,  2000001, 100);
            // CShopDlg::SellSelectedItem(app,1,1);
            // CWndMan::DebugPrintUISkillEntries(app.hProcess);
            // CWndMan::FocusSkillUpButton(app.hProcess,1001004);

            // CWndMan::FocusStatApButton(app.hProcess, StatType::STR);
            // CWndMan::FocusStatApButton(app.hProcess, StatType::DEX);
            // CWndMan::FocusStatApButton(app.hProcess, StatType::INT);

            // CShopDlg::DebugPrint(app.hProcess);
            // auto texts = CWndMan::GetUtilDlgText(app.hProcess);
            // for (size_t i = 0; i < texts.size(); i++)
            // {
            //     printf("[UtilDlg] [%zu] type=%d sel=%d: %s\n", 
            //         i, 
            //         texts[i].nType, 
            //         texts[i].nSelect, 
            //         texts[i].text.c_str());
            // }

            // CWndMan::SetUtilDlgExSelection(app.hProcess,4);

            // if (CWndMan::CanWeDoInput(app.hProcess)) {
            //     // Safe to send arrow key input - it will move the player
            //     printf("Can Do Input\n");
            // } 

            // Get hit by Slow (mob skill 126), then call:
            // auto debuff = CUserLocal::GetDebuffs(app.hProcess);
            // app.logger.Addf("%s\n",debuff.c_str());
            // CUserLocal::ScanForDiseaseReasonII(app.hProcess, s_test_i_1);


            // CFuncKeyMapped::DumpFuncKeyMapRemote(app.hProcess);
            
            // After map transition in WalkPath:
            // Sleep(1000);  // Wait for map load

            // wz::WzFile wz;
            //     if (wz.open("F:\\HiddenStreet\\Cosmic-client-main\\MapleStory\\Map.wz")) {
            //         printf("Success! Version: %d\n", wz.version());
            //         for (const auto& [name, child] : *wz.get_root()) {
            //             printf("  %s%s\n", name.c_str(), child->is_directory() ? "/" : "");
            //         }
            //     }

            // CFuncKeyMapped::DumpFuncKeyMapRemote(app.hProcess);

            // MapleMap::Findpath(1448, 466);
            // MapleMap::CurrentPath.DebugPrint();
            // ReachabilityComputer::Reach Reach;

            // if(MapleMap::TestReach(Reach))
            // {
            //     for(int input =0; input<ReachabilityComputer::kNumCases; input++)
            //     {
            //         app.logger.Addf("here %i",input);
            //         auto& middle = Reach.Get(input); // <-- note the &
                    
            //         if(!middle.target) continue;

            //         app.logger.Addf("input %i; t_target %.1f",input, middle.t_target);

            //         auto Ymax = middle.ApexY();
            //         app.logger.Addf("apex Y %.2f tp=%.1f, tt=%.1f",Ymax,middle.t_apex, middle.t_terminal);
            //         for(double y=Ymax; y<=Ymax+200; y=y+3)
            //         {
            //             auto S = middle.StateAtY(y);
            //             app.logger.Addf(
            //                 "targetY = %.2f | reached=%d | t = %.2f | state = (x=%.2f, y=%.2f)",
            //                 y,
            //                 S.reached ? 1 : 0,
            //                 S.t,
            //                 S.state.x,
            //                 S.state.y);
            //         }

            //     }

            // }
            
            // PhysicsParams::CVecCtrl CVecCtrl;
            // if(CUserLocal::GetVecCtrl(app.hProcess, CVecCtrl))
            // {
            //     app.logger.Addf("Layer[%i] X(%.1f,%.1f) V(%.1f,%.1f)\nfh[%X] lr[%X] Fall[%i] Stopped[%i]",
            //             CVecCtrl.Layer,
            //             CVecCtrl.X,
            //             CVecCtrl.Y,
            //             CVecCtrl.Vx,
            //             CVecCtrl.Vy,
            //             CVecCtrl.OnFootholdAddress,
            //             CVecCtrl.OnRopeAddress,
            //             CVecCtrl.IsFalling,
            //             CVecCtrl.IsStopped);
            // }


            // PhysicsParams::CVecCtrl CVecCtrl;
            // CUserLocal::GetVecCtrl(app.hProcess, CVecCtrl);
            // CWvsPhysicalSpace2D::ForEachFoothold(
            //     app.hProcess,
            //     [&](const CStaticFoothold& fh) -> bool
            //     {
            //         long x1 = 0, y1 = 0;
            //         long x2 = 0, y2 = 0;
            //         long id = 0, pr = 0, ne = 0;

            //         if (!fh.GetFoothold(app.hProcess, x1, y1, x2, y2, id, pr, ne))
            //             return true;
                    
            //         PhysicsParams::FootholdParams Params;
            //         if(!fh.GetAttributes(app.hProcess,Params))
            //             return true;
            //             if( y1>y2 && std::abs(x1-CVecCtrl.X)<100)
            //         app.logger.Addf("[FH:%0X][%i] [%i,%i][%i,%i][%.2f][%.2f][%.2f]", fh.remoteAddress,id,x1,y1,x2,y2,Params.footholdWalk,Params.slopeUvX,Params.slopeUvY);
                
            //         return true;
            //     }
            // );
            

            // PhysicsParams::PhysicalSpaceParams ParamsP;
            // PhysicsParams::FieldParams ParamsF;
            
            // if(CWvsPhysicalSpace2D::GetConstants(app.hProcess,ParamsP,ParamsF))
            // {
            //     app.logger.Addf("dWalkForce  =%.2f",ParamsP.dWalkForce );
            //     app.logger.Addf("dWalkSpeed  =%.2f",ParamsP.dWalkSpeed );
            //     app.logger.Addf("dFloatDrag1 =%.2f",ParamsP.dFloatDrag1);
            //     app.logger.Addf("dFloatDrag2 =%.2f",ParamsP.dFloatDrag2);
            //     app.logger.Addf("dFloatCoeff =%.2f",ParamsP.dFloatCoeff);
            //     app.logger.Addf("dGravityAcc =%.2f",ParamsP.dGravityAcc);
            //     app.logger.Addf("dFallSpeed  =%.2f",ParamsP.dFallSpeed );
            //     app.logger.Addf("dJumpSpeed  =%.2f",ParamsP.dJumpSpeed );
            //     app.logger.Addf("fieldDrag   =%.2f",ParamsF.fieldDrag);
            //     app.logger.Addf("gravity     =%.2f",ParamsF.gravity);
            // }

            // PhysicsParams::UserParams ParamsU;
            // if(CUserLocal::GetWalkAttributes(app.hProcess,ParamsU))
            //     app.logger.Addf("Shoe[walkSpeed %.2f] [walkJump %.2f][Mass %0.2f]",ParamsU.shoeWalkSpeed,ParamsU.walkJump,ParamsU.mass);


            // CMobPool::ForEachMob(
            // app.hProcess,
            // [&](const CMob& mob) -> bool
            // {
            //     long HP, maxHP;
            //     mob.GetHP(app.hProcess,  HP, maxHP);
            //     app.logger.Addf("[%X][%i/%i]",mob.remoteAddress,HP,maxHP);
            //     return true;
            // }
            // );
            // uint32_t str, dex, int_, luk;
            // if (CUserLocal::GetBasicStats(app.hProcess, str, dex, int_, luk))
            // {
            //     app.logger.Addf("[BasicStats] STR=%u DEX=%u INT=%u LUK=%u", str, dex, int_, luk);
            // }
            // else
            // {
            //     app.logger.Add("[BasicStats] FAILED to read");
            // }

            // // Secondary Stats (Attack, Defense, etc.)
            // uint32_t attack, defense, magic, magic_def, accuracy, avoid, hands, speed, jump;
            // if (CUserLocal::GetSecondStats(app.hProcess, attack, defense, magic, magic_def, accuracy, avoid, hands, speed, jump))
            // {
            //     app.logger.Addf("[SecondStats] ATK=%u DEF=%u MAG=%u MDEF=%u", attack, defense, magic, magic_def);
            //     app.logger.Addf("[SecondStats] ACC=%u AVO=%u HANDS=%u SPD=%u JMP=%u", accuracy, avoid, hands, speed, jump);
            // }
            // else
            // {
            //     app.logger.Add("[SecondStats] FAILED to read");
            // }
        }








}
