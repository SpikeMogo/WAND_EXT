// DiscordCommands.cpp
#include "DiscordCommands.h"
#include "DiscordBot.h"
#include "AppState.h"
// CNetwork.h and CPacket.h removed (stripped)
#include <navigator/Map.h>
#include <class/MapleClass.h>
#include <class/CUIStatusBar.h>
#include "stb_image_write.h"
#include <class/JobInfo.h>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include <chrono>

// Shared state for chat redirect (used by panel buttons, /chat command, and message handler)
static std::mutex chatRedirectMutex;
static std::string chatRedirectType;    // empty = off
static std::string chatRedirectTarget;  // whisper target IGN

// ============================================================================
// Shared Actions - Used by both slash commands and buttons
// ============================================================================

namespace DiscordActions {

std::string RunScript(AppState& app) {
    if (!app.hProcess)
        return "❌ Process not attached.";
    
    std::string scriptPath = app.GetScriptPath();
    if (scriptPath.empty())
        return "❌ No script loaded.";
    
    if (!app.luaRunning.exchange(true)) {
        CWndMan::resetFocus(app.hProcess);
        app.luaCancelRequested = false;
        app.statTracker.Reset();

        if (app.luaThread.joinable())
            app.luaThread.join();
        
        app.luaThread = std::thread([&app, scriptPath]() {
            try {
                app.script.SetCancelFlag(&app.luaCancelRequested);
                bool ok = app.script.RunFile(scriptPath.c_str());
                if (app.luaCancelRequested.load())
                    app.logger.Add("[Lua] Script cancelled: " + scriptPath);
                else if (ok)
                    app.logger.Add("[Lua] Script finished: " + scriptPath);
                else
                    app.logger.AddE("[Lua] Script failed: " + scriptPath);
            }
            catch (const std::exception& e) {
                app.logger.AddE("[Lua] Exception: " + std::string(e.what()));
            }
            catch (...) {
                app.logger.AddE("[Lua] Unknown exception.");
            }
            app.script.ClearCancelHook();
            CInputSystem::StopMove(app);
            app.luaRunning = false;
        });
        
        return "▶️ Started: " + scriptPath;
    }
    return "⚠️ Script is already running.";
}

std::string StopScript(AppState& app) {
    if (!app.luaRunning.load())
        return "⚠️ No script is running.";

    // 1. Signal cancellation
    app.luaCancelRequested = true;

    // 2. Immediately halt movement (breaks out of walk/teleport sleeps faster)
    CInputSystem::StopMove(app);

    // 3. Wait up to 2 seconds for the Lua thread to finish gracefully
    if (app.luaThread.joinable()) {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (app.luaRunning.load() && std::chrono::steady_clock::now() < deadline) {
            Sleep(50);
        }

        if (app.luaRunning.load()) {
            // Thread is stuck in a blocking call — detach and force-reset state
            app.luaThread.detach();
            app.script.ClearCancelHook();
            CInputSystem::StopMove(app);
            app.luaRunning = false;
            app.logger.AddE("[Lua] Force-stopped (thread detached).");
            return "⏹️ Force stopped (thread was stuck).";
        } else {
            app.luaThread.join();
        }
    }

    app.logger.Add("[Lua] Script stopped.");
    return "⏹️ Script stopped.";
}

std::string ToggleChatLog(AppState& app) {
    auto& bot = DiscordBot::Instance();
    
    if (bot.IsChatLogMonitoring()) {
        bot.StopChatLogMonitor();
        return "⏹️ Chat log monitoring stopped.";
    }
    
    bot.StartChatLogMonitor([&app]() -> std::vector<DiscordChatEntry> {
        std::vector<DiscordChatEntry> logs;
        
        if (!app.hProcess)
            return logs;
        
        CUIStatusBar::ForEachChatLogLean(
            app.hProcess,
            [&](int32_t index, const ChatLogEntry& entry) -> bool {
                DiscordChatEntry e;
                e.index = entry.index;
                e.type = entry.type;
                e.channelId = entry.ChannelID;
                e.color = entry.Back;
                e.content = entry.content;
                logs.push_back(e);
                return true;
            }
        );
        
        return logs;
    });
    
    return "✅ Chat log monitoring started.";
}

void BuildStatusEmbed(AppState& app, dpp::embed& embed) {
    embed.set_title("📊 Bot Status");
    
    bool attached = (app.hProcess != nullptr);
    bool luaRunning = app.luaRunning.load();
    
    embed.add_field("Maple Process", attached ? "✅ Attached" : "❌ Not Attached", true);
    embed.add_field("Lua Script", luaRunning ? "✅ Running" : "⏹️ Stopped", true);
    embed.add_field("", "", false);
    
    if (attached && app.selectedPid != 0) {
        long Hp = 0, Mp = 0, MaxHp = 0, MaxMp = 0, Exp = 0;
        uint8_t Lvl = 0;
        uint16_t job = 0;
        double ExpPer = 0;
        
        uint32_t mapID = 0;
        long left = 0, right = 0, top = 0, bottom = 0;
        std::string streetName, mapName;
        
        int32_t Mesos = 0;
        bool haveStats = CUserLocal::GetStats(app.hProcess, Hp, Mp, MaxHp, MaxMp, Exp, ExpPer, Lvl, job, Mesos);
        bool haveMap = CWvsPhysicalSpace2D::GetMap(app.hProcess, left, right, top, bottom, mapID, streetName, mapName);
        
        if (haveStats) {
            embed.set_color(0x00FF00);
            
            embed.add_field("⚔️ Level", std::to_string(Lvl), true);
            embed.add_field("🎭 Job", std::to_string(job), true);
            embed.add_field("", "", false);
            
            float hpPct = (MaxHp > 0) ? (float)Hp / MaxHp * 100.0f : 0.0f;
            float mpPct = (MaxMp > 0) ? (float)Mp / MaxMp * 100.0f : 0.0f;
            
            embed.add_field("❤️ HP", std::to_string(Hp) + "/" + std::to_string(MaxHp) + " (" + std::to_string((int)hpPct) + "%)", true);
            embed.add_field("💙 MP", std::to_string(Mp) + "/" + std::to_string(MaxMp) + " (" + std::to_string((int)mpPct) + "%)", true);
            embed.add_field("", "", false);
            
            std::ostringstream expStr;
            expStr << std::fixed << std::setprecision(2) << ExpPer;
            embed.add_field("✨ EXP", std::to_string(Exp) + " (" + expStr.str() + "%)", true);
        } else {
            embed.set_color(0xFFFF00);
            embed.add_field("⚠️ Player", "Not logged in", false);
        }
        
        if (haveMap) {
            std::string mapInfo = "[" + std::to_string(mapID) + "] " + streetName + " - " + mapName;
            embed.add_field("🗺️ Map", mapInfo, false);
            embed.add_field("📺 Channel", std::to_string(MapleMap::ChannelID + 1), true);
        }
    } else {
        embed.set_color(0xFF0000);
    }

    std::string scriptPath = app.GetScriptPath();
    if (!scriptPath.empty()) {
        embed.add_field("📂 Script", scriptPath, false);
    }
    
    embed.set_timestamp(time(nullptr));
}

bool CaptureScreenshot(AppState& app, std::vector<uint8_t>& jpgData) {
    if (!app.hProcess || !app.hGameWnd)
        return false;
    
    RECT rect;
    if (!GetClientRect(app.hGameWnd, &rect))
        return false;
    
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    
    if (width <= 0 || height <= 0)
        return false;
    
    HDC hdcWindow = GetDC(app.hGameWnd);
    HDC hdcMem = CreateCompatibleDC(hdcWindow);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcWindow, width, height);
    HGDIOBJ hOld = SelectObject(hdcMem, hBitmap);
    
    // PrintWindow works even when minimized
    BOOL captured = PrintWindow(app.hGameWnd, hdcMem, PW_CLIENTONLY | PW_RENDERFULLCONTENT);
    if (!captured) {
        BitBlt(hdcMem, 0, 0, width, height, hdcWindow, 0, 0, SRCCOPY);
    }
    
    BITMAPINFOHEADER bi = {};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;
    
    int rowSize = ((width * 3 + 3) & ~3);
    int imageSize = rowSize * height;
    
    std::vector<uint8_t> pixels(imageSize);
    GetDIBits(hdcMem, hBitmap, 0, height, pixels.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);
    
    SelectObject(hdcMem, hOld);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(app.hGameWnd, hdcWindow);
    
    // BGR to RGB
    for (int i = 0; i < imageSize - 2; i += 3) {
        std::swap(pixels[i], pixels[i + 2]);
    }
    
    // Compress to JPG
    auto jpgCallback = [](void* context, void* data, int size) {
        auto* vec = static_cast<std::vector<uint8_t>*>(context);
        auto* bytes = static_cast<uint8_t*>(data);
        vec->insert(vec->end(), bytes, bytes + size);
    };
    
    stbi_write_jpg_to_func(jpgCallback, &jpgData, width, height, 3, pixels.data(), 75);
    return !jpgData.empty();
}

bool CaptureMapCanvas(AppState& app, std::vector<uint8_t>& jpgData) {
    if (!app.hAppWnd)
        return false;

    // Save current tab and force switch to Map
    int prevTab = app.currentTab;
    app.forceTab = 2;

    // Wait for the tab to render (a few frames at ~60fps)
    Sleep(150);

    if (!app.mapCanvasValid)
        return false;

    // Get full client area of app window
    RECT clientRect;
    if (!GetClientRect(app.hAppWnd, &clientRect))
        return false;

    int fullW = clientRect.right - clientRect.left;
    int fullH = clientRect.bottom - clientRect.top;
    if (fullW <= 0 || fullH <= 0)
        return false;

    // PrintWindow the entire app window
    HDC hdcWindow = GetDC(app.hAppWnd);
    HDC hdcMem = CreateCompatibleDC(hdcWindow);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcWindow, fullW, fullH);
    HGDIOBJ hOld = SelectObject(hdcMem, hBitmap);

    BOOL captured = PrintWindow(app.hAppWnd, hdcMem, PW_CLIENTONLY | PW_RENDERFULLCONTENT);
    if (!captured) {
        BitBlt(hdcMem, 0, 0, fullW, fullH, hdcWindow, 0, 0, SRCCOPY);
    }

    // Crop to canvas rect
    RECT cr = app.mapCanvasRect;
    int cropX = cr.left;
    int cropY = cr.top;
    int cropW = cr.right - cr.left;
    int cropH = cr.bottom - cr.top;

    if (cropW <= 0 || cropH <= 0 || cropX < 0 || cropY < 0) {
        SelectObject(hdcMem, hOld);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(app.hAppWnd, hdcWindow);
        return false;
    }

    // Create cropped bitmap
    HDC hdcCrop = CreateCompatibleDC(hdcWindow);
    HBITMAP hCropBitmap = CreateCompatibleBitmap(hdcWindow, cropW, cropH);
    HGDIOBJ hCropOld = SelectObject(hdcCrop, hCropBitmap);

    BitBlt(hdcCrop, 0, 0, cropW, cropH, hdcMem, cropX, cropY, SRCCOPY);

    // Read pixels from cropped bitmap
    BITMAPINFOHEADER bi = {};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = cropW;
    bi.biHeight = -cropH;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;

    int rowSize = ((cropW * 3 + 3) & ~3);
    int imageSize = rowSize * cropH;

    std::vector<uint8_t> pixels(imageSize);
    GetDIBits(hdcCrop, hCropBitmap, 0, cropH, pixels.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    // Cleanup GDI
    SelectObject(hdcCrop, hCropOld);
    DeleteObject(hCropBitmap);
    DeleteDC(hdcCrop);
    SelectObject(hdcMem, hOld);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(app.hAppWnd, hdcWindow);

    // Restore previous tab
    app.forceTab = prevTab;

    // BGR to RGB
    for (int i = 0; i < imageSize - 2; i += 3) {
        std::swap(pixels[i], pixels[i + 2]);
    }

    // Write as PNG (lossless, canvas is small)
    auto pngCallback = [](void* context, void* data, int size) {
        auto* vec = static_cast<std::vector<uint8_t>*>(context);
        auto* bytes = static_cast<uint8_t*>(data);
        vec->insert(vec->end(), bytes, bytes + size);
    };

    stbi_write_png_to_func(pngCallback, &jpgData, cropW, cropH, 3, pixels.data(), cropW * 3);
    return !jpgData.empty();
}

void BuildLifeEmbed(AppState& app, dpp::embed& embed) {
    embed.set_title("🌍 Life on Map");
    embed.set_timestamp(time(nullptr));

    if (!app.hProcess) {
        embed.set_color(0xFF0000);
        embed.set_description("❌ No process attached.");
        return;
    }

    // --- Map info ---
    long mleft = 0, mright = 0, mtop = 0, mbottom = 0;
    uint32_t mapID = 0;
    std::string streetName, mapName;
    if (CWvsPhysicalSpace2D::GetMap(app.hProcess, mleft, mright, mtop, mbottom, mapID, streetName, mapName)) {
        embed.add_field("🗺️ Map", "[" + std::to_string(mapID) + "] " + streetName + " - " + mapName, false);
    }

    // --- Mob counts ---
    struct MobInfo { int count = 0; std::string name; };
    std::unordered_map<uint32_t, MobInfo> mobCounts;

    CMobPool::ForEachMob(app.hProcess,
        [&](const CMob& mob) -> bool {
            uint32_t tid = 0;
            if (mob.GetTemplateId(app.hProcess, tid)) {
                auto& info = mobCounts[tid];
                info.count++;
                if (info.name.empty()) {
                    std::string n;
                    info.name = mob.GetTemplateName(app.hProcess, n) ? n : "<unknown>";
                }
            }
            return true;
        }
    );

    std::ostringstream mobStr;
    if (mobCounts.empty()) {
        mobStr << "No mobs on map";
    } else {
        int total = 0;
        for (const auto& [tid, info] : mobCounts) {
            mobStr << info.name << " [" << tid << "]: **" << info.count << "**\n";
            total += info.count;
        }
        mobStr << "**Total: " << total << "**";
    }
    embed.add_field("👾 Mobs", mobStr.str(), false);

    // --- Players ---
    std::ostringstream playerStr;
    int playerCount = 0;

    CUserPool::ForEachUser(app.hProcess,
        [&](const CUserRemote& user) -> bool {
            long x, y;
            std::string name;
            uint32_t id, job;
            user.Get(app.hProcess, id, job, x, y, name);

            std::string jobName = MapleJob::GetJobName(job);
            playerStr << std::setw(13) << name << " [" <<std::setw(14) << jobName << "] (" << x << ", " << y << ")\n";
            playerCount++;
            return true;
        }
    );

    if (playerCount == 0) {
        playerStr << "No other players";
    } else {
        playerStr << "**Total: " << playerCount << "**";
    }
    embed.add_field("👥 Players", playerStr.str(), false);

    embed.set_color(0x2ECC71);
}

std::string KillProcess(AppState& app) {
    if (!app.hProcess)
        return "❌ No process attached.";

    // Stop lua script first
    if (app.luaRunning.load()) {
        app.luaCancelRequested = true;
        if (app.luaThread.joinable())
            app.luaThread.join();
    }

    // Hooks stripped for open-source release

    // Terminate the process
    TerminateProcess(app.hProcess, 0);
    CloseHandle(app.hProcess);
    app.hProcess = nullptr;
    app.hGameWnd = nullptr;
    app.selectedPid = 0;

    return "💀 Process killed.";
}

void BuildPanelMessage(dpp::message& msg) {
    auto& bot = DiscordBot::Instance();
    
    // Minimal content (required for buttons without embed)
    msg.set_content("​");  // Zero-width space, appears empty but satisfies Discord
    
    // Row 1: Main controls
    dpp::component row1;
    row1.set_type(dpp::cot_action_row);
    
    row1.add_component(
        dpp::component()
            .set_type(dpp::cot_button)
            .set_label("Run")
            .set_style(dpp::cos_success)
            .set_id("btn_run")
    );
    
    row1.add_component(
        dpp::component()
            .set_type(dpp::cot_button)
            .set_label("Stop")
            .set_style(dpp::cos_danger)
            .set_id("btn_stop")
    );
    
    row1.add_component(
        dpp::component()
            .set_type(dpp::cot_button)
            .set_label("Status")
            .set_style(dpp::cos_primary)
            .set_id("btn_status")
    );
    
    row1.add_component(
        dpp::component()
            .set_type(dpp::cot_button)
            .set_label("Screenshot")
            .set_style(dpp::cos_primary)
            .set_id("btn_show")
    );
    
    msg.add_component(row1);
    
    // Row 2: Extra controls
    dpp::component row2;
    row2.set_type(dpp::cot_action_row);
    
    row2.add_component(
        dpp::component()
            .set_type(dpp::cot_button)
            .set_label("Chat Log")
            .set_style(bot.IsChatLogMonitoring() ? dpp::cos_success : dpp::cos_secondary)
            .set_id("btn_chatlog")
    );

    row2.add_component(
        dpp::component()
            .set_type(dpp::cot_button)
            .set_label("Life")
            .set_style(dpp::cos_success)
            .set_id("btn_life")
    );

    row2.add_component(
        dpp::component()
            .set_type(dpp::cot_button)
            .set_label("Kill")
            .set_style(dpp::cos_danger)
            .set_id("btn_kill")
    );

    msg.add_component(row2);

    // Row 3: Chat forward controls
    // Determine current chat redirect state for button styling
    std::string currentChatType;
    {
        std::lock_guard<std::mutex> lock(chatRedirectMutex);
        currentChatType = chatRedirectType;
    }

    dpp::component row3;
    row3.set_type(dpp::cot_action_row);

    row3.add_component(
        dpp::component()
            .set_type(dpp::cot_button)
            .set_label(currentChatType.empty() ? "Chat: Off" : "Chat: Stop")
            .set_style(currentChatType.empty() ? dpp::cos_secondary : dpp::cos_danger)
            .set_id("btn_chat_stop")
    );
    row3.add_component(
        dpp::component()
            .set_type(dpp::cot_button)
            .set_label("All")
            .set_style(currentChatType == "all" ? dpp::cos_success : dpp::cos_secondary)
            .set_id("btn_chat_all")
    );
    row3.add_component(
        dpp::component()
            .set_type(dpp::cot_button)
            .set_label("Party")
            .set_style(currentChatType == "party" ? dpp::cos_success : dpp::cos_secondary)
            .set_id("btn_chat_party")
    );
    row3.add_component(
        dpp::component()
            .set_type(dpp::cot_button)
            .set_label("Guild")
            .set_style(currentChatType == "guild" ? dpp::cos_success : dpp::cos_secondary)
            .set_id("btn_chat_guild")
    );
    row3.add_component(
        dpp::component()
            .set_type(dpp::cot_button)
            .set_label("Alliance")
            .set_style(currentChatType == "alliance" ? dpp::cos_success : dpp::cos_secondary)
            .set_id("btn_chat_alliance")
    );
    msg.add_component(row3);

    // Row 4: Whisper & Spouse
    dpp::component row4;
    row4.set_type(dpp::cot_action_row);

    row4.add_component(
        dpp::component()
            .set_type(dpp::cot_button)
            .set_label("Whisper")
            .set_style(currentChatType == "whisper" ? dpp::cos_success : dpp::cos_secondary)
            .set_id("btn_chat_whisper")
    );
    row4.add_component(
        dpp::component()
            .set_type(dpp::cot_button)
            .set_label("Spouse")
            .set_style(currentChatType == "spouse" ? dpp::cos_success : dpp::cos_secondary)
            .set_id("btn_chat_spouse")
    );
    msg.add_component(row4);
}

} // namespace DiscordActions

// ============================================================================
// Command & Button Registration
// ============================================================================

void RegisterDiscordCommands(AppState& app) {
    auto& bot = DiscordBot::Instance();
    
    // ===== Slash Commands =====
    
    bot.RegisterCommand("hello", "Say hello!", [](const dpp::slashcommand_t& event) {
        std::string user = event.command.get_issuing_user().username;
        event.reply("Hello, " + user + "! Bot is online.");
    });
    
    bot.RegisterCommand("ping", "Check bot latency", [](const dpp::slashcommand_t& event) {
        event.reply("Pong!");
    });
    
    bot.RegisterCommand("status", "Get bot and process status", [&app](const dpp::slashcommand_t& event) {
        dpp::embed embed;
        DiscordActions::BuildStatusEmbed(app, embed);
        event.reply(dpp::message().add_embed(embed));
    });
    
    bot.RegisterCommand("runlua", "Run the loaded Lua script", [&app](const dpp::slashcommand_t& event) {
        event.reply(DiscordActions::RunScript(app));
    });

    bot.RegisterCommand("stoplua", "Stop the running Lua script", [&app](const dpp::slashcommand_t& event) {
        event.reply(DiscordActions::StopScript(app));
    });

    bot.RegisterCommand("show", "Capture and send game screenshot", [&app](const dpp::slashcommand_t& event) {
        std::vector<uint8_t> jpgData;
        if (!DiscordActions::CaptureScreenshot(app, jpgData)) {
            event.reply("❌ Failed to capture screenshot.");
            return;
        }

        std::vector<uint8_t> mapData;
        bool hasMap = DiscordActions::CaptureMapCanvas(app, mapData);

        event.thinking();

        dpp::message msg;
        msg.add_file("screenshot.jpg", std::string(jpgData.begin(), jpgData.end()));

        dpp::embed embed1;
        embed1.set_title("📸 Game Screenshot");
        embed1.set_image("attachment://screenshot.jpg");
        embed1.set_color(0x00FF00);
        embed1.set_timestamp(time(nullptr));
        msg.add_embed(embed1);

        if (hasMap) {
            msg.add_file("map.png", std::string(mapData.begin(), mapData.end()));

            dpp::embed embed2;
            embed2.set_title("🗺️ Map View");
            embed2.set_image("attachment://map.png");
            embed2.set_color(0x2ECC71);
            msg.add_embed(embed2);
        }

        event.edit_response(msg);
    });

    bot.RegisterCommand("chatlog", "Toggle live chat log monitoring", [&app](const dpp::slashcommand_t& event) {
        event.reply(DiscordActions::ToggleChatLog(app));
    });

    // ===== /chat command: redirect Discord messages to game chat =====

    bot.RegisterCommand("chat", "Send Discord messages as in-game chat",
        {
            dpp::command_option(dpp::co_string, "type", "Chat type (all/party/guild/alliance/whisper/spouse) or 'stop'", false),
            dpp::command_option(dpp::co_string, "target", "Target player name (required for whisper)", false)
        },
        [&app](const dpp::slashcommand_t& event) {
            auto param = event.get_parameter("type");
            std::string type;
            if (std::holds_alternative<std::string>(param))
                type = std::get<std::string>(param);

            auto targetParam = event.get_parameter("target");
            std::string target;
            if (std::holds_alternative<std::string>(targetParam))
                target = std::get<std::string>(targetParam);

            // Trim whitespace from type and target
            auto trim = [](std::string& s) {
                while (!s.empty() && s.front() == ' ') s.erase(s.begin());
                while (!s.empty() && s.back() == ' ') s.pop_back();
            };
            trim(type);
            trim(target);

            // Lowercase the type for case-insensitive matching
            for (auto& c : type) c = (char)tolower((unsigned char)c);

            // No argument -> show help + current status
            if (type.empty())
            {
                dpp::embed embed;
                embed.set_title("💬 Chat Redirect");
                embed.set_color(0x3498DB);
                embed.add_field("Usage",
                    "`/chat type` — start redirecting messages\n"
                    "`/chat whisper target:Name` — whisper to a player\n"
                    "`/chat stop` — stop redirecting", false);
                embed.add_field("Types", "`all` `party` `guild` `alliance` `whisper` `spouse`", false);

                std::string current;
                std::string currentTarget;
                {
                    std::lock_guard<std::mutex> lock(chatRedirectMutex);
                    current = chatRedirectType;
                    currentTarget = chatRedirectTarget;
                }
                std::string status;
                if (current.empty()) {
                    status = "🔴 Off";
                    embed.set_color(0x95A5A6);
                } else {
                    status = "🟢 Active → **" + current + "**";
                    if (current == "whisper" && !currentTarget.empty())
                        status += " to **" + currentTarget + "**";
                    embed.set_color(0x2ECC71);
                }
                embed.add_field("Status", status, false);

                event.reply(dpp::message().add_embed(embed));
                return;
            }

            // /chat stop
            if (type == "stop")
            {
                std::lock_guard<std::mutex> lock(chatRedirectMutex);
                if (chatRedirectType.empty()) {
                    event.reply("⚠️ Chat redirect is already off.");
                    return;
                }
                chatRedirectType.clear();
                chatRedirectTarget.clear();
                event.reply("🔴 Chat redirect **stopped**.");
                return;
            }

            // Validate type
            if (type != "all" && type != "party" && type != "guild" &&
                type != "alliance" && type != "whisper" && type != "spouse")
            {
                event.reply("❌ Unknown type `" + type + "`. Valid types: `all`, `party`, `guild`, `alliance`, `whisper`, `spouse`.");
                return;
            }

            // Whisper requires target
            if (type == "whisper")
            {
                if (target.empty()) {
                    event.reply("❌ Whisper requires a target name. Use: `/chat whisper target:PlayerName`");
                    return;
                }
                // Validate target name (no spaces, reasonable length)
                if (target.length() > 13) {
                    event.reply("❌ Target name too long (max 13 characters).");
                    return;
                }
                if (target.find(' ') != std::string::npos) {
                    event.reply("❌ Target name cannot contain spaces.");
                    return;
                }
            }

            // Warn if target given for non-whisper type
            if (type != "whisper" && !target.empty())
            {
                event.reply("⚠️ Target is only used with `whisper` type. Ignoring target for `" + type + "`.");
                target.clear();
            }

            // Check process attached
            if (!app.hProcess)
            {
                event.reply("❌ No process attached.");
                return;
            }

            // Network module stripped for open-source release
            event.reply("❌ Network module not available (stripped).");
            return;

            {
                std::lock_guard<std::mutex> lock(chatRedirectMutex);
                chatRedirectType = type;
                chatRedirectTarget = target;
            }

            // Build a visible embed so everyone in the channel can see
            dpp::embed embed;
            embed.set_title("💬 Chat Redirect Enabled");
            embed.set_color(0x2ECC71);

            std::string desc = "📢 All messages in this channel will be sent as **" + type + "** chat in-game.";
            if (type == "whisper")
                desc = "📢 All messages in this channel will be **whispered** to **" + target + "** in-game.";

            embed.set_description(desc);
            embed.add_field("Type", "`" + type + "`", true);
            if (type == "whisper")
                embed.add_field("Target", "`" + target + "`", true);
            embed.add_field("Stop", "`/chat stop`", true);
            embed.set_timestamp(time(nullptr));

            event.reply(dpp::message().add_embed(embed));
        }
    );

    // Message handler: forward non-command messages to game
    bot.SetMessageHandler([&app](const dpp::message_create_t& event) {
        std::string type;
        std::string target;
        {
            std::lock_guard<std::mutex> lock(chatRedirectMutex);
            type = chatRedirectType;
            target = chatRedirectTarget;
        }
        if (type.empty())
            return;  // redirect not active

        // Network module stripped for open-source release
        return;
    });

    bot.RegisterCommand("panel", "Show control panel", [](const dpp::slashcommand_t& event) {
        auto& bot = DiscordBot::Instance();
        bot.DeletePanel();
        
        dpp::message msg;
        DiscordActions::BuildPanelMessage(msg);
        
        event.reply(msg, [&bot, channelId = event.command.channel_id](const dpp::confirmation_callback_t& callback) {
            if (!callback.is_error()) {
                auto m = callback.get<dpp::message>();
                bot.SetPanelInfo(channelId, m.id);
            }
        });
    });
    
    bot.RegisterCommand("help", "Show available commands", [](const dpp::slashcommand_t& event) {
        dpp::embed embed;
        embed.set_title("📖 Available Commands");
        embed.set_color(0x9B59B6);
        
        embed.add_field("/hello", "Say hello", false);
        embed.add_field("/ping", "Check bot latency", false);
        embed.add_field("/status", "Get bot and player status", false);
        embed.add_field("/runlua", "Run the Lua script", false);
        embed.add_field("/stoplua", "Stop the Lua script", false);
        embed.add_field("/show", "Capture game screenshot", false);
        embed.add_field("/chatlog", "Toggle chat log monitoring", false);
        embed.add_field("/chat [type|stop]", "Redirect Discord messages to game chat", false);
        embed.add_field("/panel", "Show control panel with buttons", false);
        embed.add_field("/help", "Show this message", false);
        
        embed.set_timestamp(time(nullptr));
        event.reply(dpp::message().add_embed(embed));
    });

    // ===== Button Handlers =====
    
    bot.RegisterButton("btn_run", [&app](const dpp::button_click_t& event) {
        event.reply(DiscordActions::RunScript(app), [](const dpp::confirmation_callback_t&) {
            DiscordBot::Instance().RefreshPanel();
        });
    });

    bot.RegisterButton("btn_stop", [&app](const dpp::button_click_t& event) {
        event.reply(DiscordActions::StopScript(app), [](const dpp::confirmation_callback_t&) {
            DiscordBot::Instance().RefreshPanel();
        });
    });

    bot.RegisterButton("btn_status", [&app](const dpp::button_click_t& event) {
        dpp::embed embed;
        DiscordActions::BuildStatusEmbed(app, embed);
        event.reply(dpp::message().add_embed(embed), [](const dpp::confirmation_callback_t&) {
            DiscordBot::Instance().RefreshPanel();
        });
    });

    bot.RegisterButton("btn_chatlog", [&app](const dpp::button_click_t& event) {
        event.reply(DiscordActions::ToggleChatLog(app), [](const dpp::confirmation_callback_t&) {
            DiscordBot::Instance().RefreshPanel();
        });
    });

    bot.RegisterButton("btn_show", [&app](const dpp::button_click_t& event) {
        std::vector<uint8_t> jpgData;
        if (!DiscordActions::CaptureScreenshot(app, jpgData)) {
            event.reply("❌ Failed to capture screenshot.", [](const dpp::confirmation_callback_t&) {
                DiscordBot::Instance().RefreshPanel();
            });
            return;
        }

        // Also capture map canvas
        std::vector<uint8_t> mapData;
        bool hasMap = DiscordActions::CaptureMapCanvas(app, mapData);

        event.thinking();

        dpp::message msg;
        msg.add_file("screenshot.jpg", std::string(jpgData.begin(), jpgData.end()));

        dpp::embed embed1;
        embed1.set_title("📸 Game Screenshot");
        embed1.set_image("attachment://screenshot.jpg");
        embed1.set_color(0x00FF00);
        embed1.set_timestamp(time(nullptr));
        msg.add_embed(embed1);

        if (hasMap) {
            msg.add_file("map.png", std::string(mapData.begin(), mapData.end()));

            dpp::embed embed2;
            embed2.set_title("🗺️ Map View");
            embed2.set_image("attachment://map.png");
            embed2.set_color(0x2ECC71);
            msg.add_embed(embed2);
        }

        event.edit_response(msg, [](const dpp::confirmation_callback_t&) {
            DiscordBot::Instance().RefreshPanel();
        });
    });
    
    bot.RegisterButton("btn_life", [&app](const dpp::button_click_t& event) {
        dpp::embed embed;
        DiscordActions::BuildLifeEmbed(app, embed);
        event.reply(dpp::message().add_embed(embed), [](const dpp::confirmation_callback_t&) {
            DiscordBot::Instance().RefreshPanel();
        });
    });

    bot.RegisterButton("btn_kill", [&app](const dpp::button_click_t& event) {
        event.reply(DiscordActions::KillProcess(app), [](const dpp::confirmation_callback_t&) {
            DiscordBot::Instance().RefreshPanel();
        });
    });

    // ===== Chat Forward Panel Buttons =====

    // Helper lambda to set chat redirect from a button click
    auto setChatMode = [&app](const dpp::button_click_t& event, const std::string& type, const std::string& target = "") {
        if (!app.hProcess) {
            event.reply("❌ No process attached.");
            return;
        }
        // Network module stripped for open-source release
        event.reply("❌ Network module not available (stripped).");
        return;

        {
            std::lock_guard<std::mutex> lock(chatRedirectMutex);
            chatRedirectType = type;
            chatRedirectTarget = target;
        }

        std::string desc = "💬 Chat forward → **" + type + "**";
        if (type == "whisper" && !target.empty())
            desc += " to **" + target + "**";

        event.reply(desc, [](const dpp::confirmation_callback_t&) {
            DiscordBot::Instance().RefreshPanel();
        });
    };

    bot.RegisterButton("btn_chat_stop", [](const dpp::button_click_t& event) {
        {
            std::lock_guard<std::mutex> lock(chatRedirectMutex);
            chatRedirectType.clear();
            chatRedirectTarget.clear();
        }
        event.reply("🔴 Chat forward **stopped**.", [](const dpp::confirmation_callback_t&) {
            DiscordBot::Instance().RefreshPanel();
        });
    });

    bot.RegisterButton("btn_chat_all", [setChatMode](const dpp::button_click_t& event) {
        setChatMode(event, "all");
    });

    bot.RegisterButton("btn_chat_party", [setChatMode](const dpp::button_click_t& event) {
        setChatMode(event, "party");
    });

    bot.RegisterButton("btn_chat_guild", [setChatMode](const dpp::button_click_t& event) {
        setChatMode(event, "guild");
    });

    bot.RegisterButton("btn_chat_alliance", [setChatMode](const dpp::button_click_t& event) {
        setChatMode(event, "alliance");
    });

    bot.RegisterButton("btn_chat_spouse", [setChatMode](const dpp::button_click_t& event) {
        setChatMode(event, "spouse");
    });

    // Whisper button: show a modal to ask for the target player name
    bot.RegisterButton("btn_chat_whisper", [](const dpp::button_click_t& event) {
        dpp::interaction_modal_response modal("modal_whisper_target", "Whisper Target");
        modal.add_component(
            dpp::component()
                .set_type(dpp::cot_text)
                .set_label("Player Name")
                .set_id("whisper_target_name")
                .set_placeholder("Enter in-game name")
                .set_min_length(1)
                .set_max_length(13)
                .set_text_style(dpp::text_short)
        );
        event.dialog(modal);
    });

    // Handle whisper modal submit
    bot.RegisterFormHandler("modal_whisper_target", [&app](const dpp::form_submit_t& event) {
        // Extract target name from modal components
        std::string target;
        auto extractValue = [](const dpp::component& comp) -> std::string {
            if (std::holds_alternative<std::string>(comp.value))
                return std::get<std::string>(comp.value);
            return {};
        };

        for (const auto& row : event.components) {
            // Check top-level (might be the text input directly)
            if (row.custom_id == "whisper_target_name")
                target = extractValue(row);
            // Check nested (action_row > text_input)
            for (const auto& comp : row.components) {
                if (comp.custom_id == "whisper_target_name")
                    target = extractValue(comp);
            }
        }

        if (target.empty()) {
            event.reply("❌ Player name cannot be empty.");
            return;
        }
        if (target.length() > 13 || target.find(' ') != std::string::npos) {
            event.reply("❌ Invalid player name.");
            return;
        }

        if (!app.hProcess) {
            event.reply("❌ No process attached.");
            return;
        }
        // Network module stripped for open-source release
        event.reply("❌ Network module not available (stripped).");
        return;
    });

    // Set panel builder callback
    bot.panelBuilder = [](dpp::message& msg) {
        DiscordActions::BuildPanelMessage(msg);
    };
}