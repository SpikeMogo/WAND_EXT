// UIMain.cpp
// Must include WinSock2 BEFORE windows.h to avoid conflicts
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <WinSock2.h>
#include <WS2tcpip.h>

#include <UI/UIMain.h>
#include <UI/TabInventory.h>
#include <UI/TabMain.h>
#include <UI/TabSetting.h>
#include <UI/TabWzBrowser.h>
#include <UI/TabMap.h>
#include <UI/TabDiscord.h>
#include <class/MapleClass.h>
#include <windows.h>
#include <commdlg.h>
#include "Settings.h"

#pragma comment(lib, "Comdlg32.lib")


static bool OpenLuaScriptDialog(AppState& app)
{
    char buffer[MAX_PATH] = {};

    std::string currentPath = Settings::Instance().GetString("lua.last_script");
    std::replace(currentPath.begin(), currentPath.end(), '/', '\\');

    std::string initialDir;
    size_t lastSlash = currentPath.find_last_of("\\");
    if (lastSlash != std::string::npos)
        initialDir = currentPath.substr(0, lastSlash);

    OPENFILENAMEA ofn{};
    ofn.lStructSize     = sizeof(ofn);
    ofn.hwndOwner       = nullptr;
    ofn.lpstrFilter     = "Lua scripts (*.lua)\0*.lua\0All files (*.*)\0*.*\0\0";
    ofn.lpstrFile       = buffer;
    ofn.nMaxFile        = MAX_PATH;
    ofn.Flags           = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt     = "lua";
    ofn.lpstrInitialDir = initialDir.empty() ? nullptr : initialDir.c_str();

    if (GetOpenFileNameA(&ofn)) {
        Settings::Instance().Set("lua.last_script", std::string(buffer));
        app.logger.Addf("[Lua] File %s selected", buffer);
        return true;
    }

    DWORD err = CommDlgExtendedError();
    if (err != 0)
        app.logger.AddE("[Lua] GetOpenFileName failed, code " + std::to_string(err));

    return false;
}


void DrawMainUI(AppState& app, ImGuiWindowFlags flags)
{
    ImGuiIO& io = ImGui::GetIO();

    ImGui::Begin("MainWindow", nullptr, flags);
    {
        ImGui::Separator();

        // Tab bar
        int forceTab = app.forceTab.load();

        if (ImGui::BeginTabBar("MainTabBar")) {
            ImGuiTabItemFlags mapFlags = (forceTab == 2) ? ImGuiTabItemFlags_SetSelected : 0;

            if (ImGui::BeginTabItem("Main")) {
                DrawMainTab(app, io);
                app.currentTab = 0;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Setting")) {
                DrawSettingTab(app);
                app.currentTab = 1;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Map", nullptr, mapFlags)) {
                DrawMapTab(app);
                app.currentTab = 2;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Inventory")) {
                DrawInventoryTab(app);
                app.currentTab = 3;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("WZ Browser")) {
                DrawWzBrowserTab(app, io);
                app.currentTab = 4;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Discord")) {
                DrawDiscordTab(app);
                app.currentTab = 5;
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        // Clear force tab after it's been applied
        if (forceTab >= 0)
            app.forceTab = -1;

        // ---- Lua Script Section ----
        ImGui::SeparatorText("Lua Script");


        if (ImGui::Button("Browse..."))
            OpenLuaScriptDialog(app);

        ImGui::SameLine();

        bool isRunning = app.luaRunning.load(std::memory_order_relaxed);

        if (isRunning)
            ImGui::BeginDisabled();

        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.13f, 0.55f, 0.13f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.18f, 0.70f, 0.18f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.10f, 0.45f, 0.10f, 1.0f));
        if (ImGui::Button("Run")) {
            if (!app.luaRunning.exchange(true)) {
                CWndMan::resetFocus(app.hProcess);
                app.luaCancelRequested = false;
                app.statTracker.Reset();

                if (app.luaThread.joinable())
                    app.luaThread.join();

                std::string scriptPath = app.GetScriptPath();
                app.luaThread = std::thread([&app, scriptPath]() {
                    try {
                        app.script.SetCancelFlag(&app.luaCancelRequested);  // <-- add this
                        bool ok = app.script.RunFile(scriptPath.c_str());

                        if (app.luaCancelRequested.load())
                            app.logger.Add("[Lua] Script cancelled: " + scriptPath);
                        else if (ok)
                            app.logger.Add("[Lua] Script finished: " + scriptPath);
                        else
                            app.logger.AddE("[Lua] Script failed: " + scriptPath);
                    }
                    catch (const std::exception& e) {
                        if (app.luaCancelRequested.load())
                            app.logger.AddE("[Lua] Script cancelled via exception: " + std::string(e.what()));
                        else
                            app.logger.AddE("[Lua] Unhandled C++ exception: " + std::string(e.what()));
                    }
                    catch (...) {
                        if (app.luaCancelRequested.load())
                            app.logger.AddE("[Lua] Script cancelled (unknown exception).");
                        else
                            app.logger.AddE("[Lua] Unhandled non-standard exception.");
                    }
                    app.script.ClearCancelHook();  // <-- safety cleanup
                    CInputSystem::StopMove(app);
                    app.luaRunning = false;
                });
            }
        }

        ImGui::PopStyleColor(3);

        if (isRunning)
            ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.70f, 0.13f, 0.13f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.85f, 0.18f, 0.18f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.55f, 0.10f, 0.10f, 1.0f));
        if (ImGui::Button("Stop")) {
            if (app.luaRunning.load()) {
                app.luaCancelRequested = true;
                CInputSystem::StopMove(app);

                // Wait briefly for graceful stop
                auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
                while (app.luaRunning.load() && std::chrono::steady_clock::now() < deadline) {
                    Sleep(50);
                }

                if (app.luaRunning.load()) {
                    // Thread stuck — detach and force-reset
                    if (app.luaThread.joinable())
                        app.luaThread.detach();
                    app.script.ClearCancelHook();
                    CInputSystem::StopMove(app);
                    app.luaRunning = false;
                    app.logger.AddE("[Lua] Force-stopped (thread detached).");
                } else {
                    if (app.luaThread.joinable())
                        app.luaThread.join();
                    app.logger.Add("[Lua] Script stopped.");
                }
            }
        }
        ImGui::PopStyleColor(3);

        // Script path
        ImGui::SameLine();
        float memKB = app.script.GetMemoryUsage() / 1024.0f;
        float allocKB = app.script.GetAllocatedBytes() / 1024.0f;
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Lua Mem: [%.1f KB/%.1f KB]", memKB, allocKB);

        std::string scriptPath = app.GetScriptPath();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(252/255.0f, 163/255.0f, 17/255.0f, 1.0f)); // RGBA
        ImGui::TextWrapped("%s", scriptPath.c_str());
        ImGui::PopStyleColor();


        // ---- Player Stats ----
        if (app.hProcess && app.selectedPid != 0 && IsProcessRunning(app.selectedPid)) {
            long atk = 0, br = 0, anim = 0, combo = 0, face = 0, id = 0;
            long Hp = 0, Mp = 0, MaxHp = 0, MaxMp = 0, Exp = 0;
            double ExpPer = 0.0;
            int32_t Mesos = 0;
            uint8_t Lvl = 0;
            uint16_t job = 0;
            PhysicsParams::CVecCtrl CVecCtrl;

            bool haveStats    = CUserLocal::GetStats(app.hProcess, Hp, Mp, MaxHp, MaxMp, Exp, ExpPer, Lvl, job, Mesos);

            // Tick stat tracker (internally throttled to 1-second intervals)
            if (haveStats)
                app.statTracker.Tick(ExpPer, Mesos);

            bool haveStatus   = CUserLocal::GetStatus(app.hProcess, atk, br, anim, combo, face, id);
            bool haveCVecCtrl = CUserLocal::GetVecCtrl(app.hProcess, CVecCtrl);

            long left = 0, right = 0, top = 0, bottom = 0;
            uint32_t mapID = 0;
            std::string streetName, mapName;
            bool haveMap = CWvsPhysicalSpace2D::GetMap(app.hProcess, left, right, top, bottom, mapID, streetName, mapName);

            if (haveStats || haveStatus || haveCVecCtrl || haveMap) 
            {
                ImGui::SeparatorText("Player Stats:");

                // Level + HP/MP bars
                if (haveStats) {
                    float hpRatio = (MaxHp > 0) ? (float)Hp / MaxHp : 0.0f;
                    float mpRatio = (MaxMp > 0) ? (float)Mp / MaxMp : 0.0f;
                    ImVec2 barSize(140.0f, 16.0f);

                    ImGui::Bullet();
                    ImGui::SameLine();
                    ImGui::SetWindowFontScale(1.4f);
                    ImGui::Text("LV. %u", (unsigned)Lvl);

                    ImGui::SameLine();
                    ImGui::SetWindowFontScale(1.1f);

                    char hpText[64];
                    snprintf(hpText, sizeof(hpText), "HP[%ld/%ld]", Hp, MaxHp);
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, IM_COL32(193, 18, 31, 255));
                    ImGui::ProgressBar(hpRatio, barSize, hpText);
                    ImGui::PopStyleColor();

                    ImGui::SameLine(0.0f, 10.0f);

                    char mpText[64];
                    snprintf(mpText, sizeof(mpText), "MP[%ld/%ld]", Mp, MaxMp);
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, IM_COL32(0, 180, 216, 255));
                    ImGui::ProgressBar(mpRatio, barSize, mpText);
                    ImGui::PopStyleColor();

                    ImGui::SetWindowFontScale(1.0f);
                }

                // Exp bar
                if (haveStatus) {
                    ImGui::Bullet();
                    ImGui::SameLine();
                    ImGui::SetWindowFontScale(1.4f);
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 0));
                    ImGui::Text("LV. %u", (unsigned)Lvl);
                    ImGui::PopStyleColor();

                    ImGui::SameLine();
                    ImGui::SetWindowFontScale(1.1f);

                    float expRatio = static_cast<float>(ExpPer) / 100.0f;
                    char expText[64];
                    ImVec2 barSize(290.0f, 16.0f);
                    snprintf(expText, sizeof(expText), "Exp. %ld[%.2f%%]", Exp, ExpPer);
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, IM_COL32(14, 173, 105, 255));
                    ImGui::ProgressBar(expRatio, barSize, expText);
                    ImGui::PopStyleColor();
                    ImGui::SetWindowFontScale(1.0f);
                }

                // EXP & Mesos rates
                if (haveStats && app.statTracker.IsActive()) {
                    const auto& r = app.statTracker.GetRates();

                    // Format mesos: <1K raw, <1M .1fK, >=1M .1fM
                    auto fmtMesos = [](int64_t v) -> std::string {
                        char buf[32];
                        int64_t abs = v < 0 ? -v : v;
                        const char* sign = v < 0 ? "-" : "";
                        if (abs >= 1000000)
                            snprintf(buf, sizeof(buf), "%s%.1fM", sign, abs / 1000000.0);
                        else if (abs >= 1000)
                            snprintf(buf, sizeof(buf), "%s%.1fK", sign, abs / 1000.0);
                        else
                            snprintf(buf, sizeof(buf), "%s%lld", sign, (long long)abs);
                        return buf;
                    };

                    char rateBuf[256];
                    snprintf(rateBuf, sizeof(rateBuf),
                        " Exp: %.2f%%/min %.2f%%/hr | Mesos: %s/min %s/hr",
                        r.expPerMin, r.expPerHour,
                        fmtMesos(r.mesosPerMin).c_str(),
                        fmtMesos(r.mesosPerHour).c_str());

                    // Use InvisibleButton for hover detection, then draw text over it
                    ImVec2 textSize = ImGui::CalcTextSize(rateBuf, nullptr, false, ImGui::GetContentRegionAvail().x);
                    ImVec2 cursor = ImGui::GetCursorScreenPos();
                    ImGui::InvisibleButton("stat_rate", textSize);
                    bool hovered = ImGui::IsItemHovered();

                    ImU32 color = hovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(180, 180, 180, 255);
                    
                    ImGui::GetWindowDrawList()->AddText(cursor, color, rateBuf);

                    if (hovered) {
                        ImGui::SetTooltip("Click to reset");
                        if (ImGui::IsItemClicked())
                            app.statTracker.Reset();
                    }
                }

                ImGui::Spacing();

                // Position + Velocity
                if (haveCVecCtrl) {
                    ImGui::BulletText("Pos: ");
                    ImGui::SameLine();

                    char label[64];
                    snprintf(label, sizeof(label), "(%.1f, %.1f)", CVecCtrl.X, CVecCtrl.Y);
                    if (ImGui::SmallButton(label)) {
                        char buf[64];
                        snprintf(buf, sizeof(buf), "%d, %d", (int)CVecCtrl.X, (int)CVecCtrl.Y);
                        ImGui::SetClipboardText(buf);
                    }
                    ImGui::SetItemTooltip("Click to copy");

                    ImGui::SameLine();
                    ImGui::Text(" | V: (%.1f, %.1f)  [%i] [%i]",
                                CVecCtrl.Vx, CVecCtrl.Vy, CVecCtrl.InputX, CVecCtrl.Layer);
                }

                ImGui::Spacing();

                // Breath, Anim, Face, Channel, Job, ID
                if (haveStatus) {
                    ImGui::BulletText("Breath: %ld | Anim: %ld | Face: %ld", br, anim, face);

                    uint32_t channel, tot_channel = 0;
                    CUserLocal::GetChannel(app.hProcess, channel, tot_channel);
                    ImGui::Spacing();
                    ImGui::BulletText("Ch: [%ld of %ld]", channel, tot_channel);
                    if (haveStats) {
                        ImGui::SameLine();
                        ImGui::Text("| Job: %d", job);
                    }
                    ImGui::SameLine();
                    ImGui::Text("| ID: %ld", id);
                }

                ImGui::Spacing();

                // Map info
                if (haveMap) {
                    ImGui::BulletText("Map: ");
                    ImGui::SameLine();

                    char idLabel[32];
                    snprintf(idLabel, sizeof(idLabel), "[%u]", mapID);
                    if (ImGui::SmallButton(idLabel)) {
                        char buf[16];
                        snprintf(buf, sizeof(buf), "%u", mapID);
                        ImGui::SetClipboardText(buf);
                    }
                    ImGui::SetItemTooltip("Click to copy");

                    ImGui::SameLine();
                    ImGui::Text("%s", mapName.c_str());
                }

                // Pet info
                {
                    CPet pets[3];
                    int petCount = CUserLocal::GetActivePets(app.hProcess, pets);
                    if (petCount > 0)
                    {
                        ImGui::Spacing();
                        ImGui::BulletText("Pets:");
                        ImGui::SameLine();
                        float barStartX = ImGui::GetCursorPosX();  // align 3rd bar here

                        int shown = 0;
                        for (int i = 0; i < 3; i++)
                        {
                            if (!pets[i].IsValid()) continue;
                            std::string name;
                            int rep = 0;
                            pets[i].GetName(app.hProcess, name);
                            pets[i].GetRepleteness(app.hProcess, rep);

                            // 3rd pet goes to next line, aligned with first bar
                            if (shown == 2)
                            {
                                ImGui::SetCursorPosX(barStartX);
                            }
                            else if (shown > 0)
                            {
                                ImGui::SameLine();
                            }

                            float fraction = rep / 100.0f;
                            char overlay[64];
                            snprintf(overlay, sizeof(overlay), "%s [%d/100]", name.c_str(), rep);

                            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(251/255.f, 133/255.f, 0/255.f, 1.0f));
                            ImGui::ProgressBar(fraction, ImVec2(150, 14), overlay);
                            ImGui::PopStyleColor();
                            shown++;
                        }
                    }
                }

                // Cursor position
                ImGui::Spacing();
                POINT p;
                if (GetCursorPos(&p)) {
                    ScreenToClient(app.hGameWnd, &p);
                    ImGui::BulletText("Cursor: Wnd (%5ld,%5ld)", p.x, p.y);

                    int X, Y;
                    if (CInputSystem::GetCursorGamePos(app.hProcess, X, Y)) {
                        ImGui::SameLine();
                        ImGui::Text("| Game (%5d,%5d)", X, Y);
                    }
                }
            }

        }
    }
    ImGui::End();
}