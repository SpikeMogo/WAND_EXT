#pragma once
#include <imgui/imgui.h>
#include <windows.h>
#include <vector>
#include <string>
#include <cstdarg>
#include <cstdio>
#include "ScriptEngine.h"
#include <mutex>
#include <chrono>
#include <ctime>
#include <thread>
#include <atomic>
// Hooks removed (stripped for open-source release)
#include "StatTracker.h"


#include "Settings.h"


#if defined(_DEBUG) || defined(_DPRINT)
    #define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
    #define DEBUG_PRINTF(...) ((void)0)
#endif


inline std::string GetTimestamp()
{
    using namespace std::chrono;
    auto now   = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);

    std::tm tm_local;
    localtime_s(&tm_local, &t);

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d |",
                  tm_local.tm_hour,
                  tm_local.tm_min,
                  tm_local.tm_sec);
    return buf;
}

struct AppLogger
{
    enum class LogColor : uint8_t
    {
        Normal,
        Verbose,
        Error
    };

    struct LogLine
    {
        std::string text;       // Message without timestamp
        std::string timestamp;  // Timestamp prefix (e.g., "14:05:01 |")
        LogColor    color;
        int         repeatCount = 1;
    };

    std::vector<LogLine> lines;
    bool autoScroll = true;
    int  selectedIndex = -1;
    std::mutex mutex;

    void Clear()
    {
        std::lock_guard<std::mutex> lock(mutex);
        lines.clear();
        selectedIndex = -1;
    }

    void AddColored(const std::string& s, LogColor c)
    {
        std::lock_guard<std::mutex> lock(mutex);
        
        // Check if same as last message - update timestamp instead of adding new line
        if (!lines.empty() && lines.back().text == s && lines.back().color == c)
        {
            lines.back().timestamp = GetTimestamp();
            lines.back().repeatCount++;
            return;
        }
        
        LogLine line;
        line.text = s;
        line.timestamp = GetTimestamp();
        line.color = c;
        line.repeatCount = 1;
        
        lines.push_back(line);
        
        if (lines.size() > 400)
            lines.erase(lines.begin());
    }

    void Add(const std::string& s)
    {
        AddColored(s, LogColor::Normal);
    }

    void Addf(const char* fmt, ...)
    {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        AddColored(buf, LogColor::Normal);
    }

    void AddV(const char* fmt, ...)
    {
        if (!Settings::Instance().GetBool("general.verbose"))
            return;

        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        AddColored(buf, LogColor::Verbose);
    }

    void AddE(const char* fmt, ...)
    {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        AddColored(buf, LogColor::Error);
    }

    void AddE(const std::string& s)
    {
        AddColored(s, LogColor::Error);
    }
};


struct AppState
{
    // Background color
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // ---- Process selection state ----
    DWORD selectedPid   = 0;          // 0 = none
    char  selectedName[260] = "";     // ANSI copy of exe name (for ImGui display)
    HANDLE hProcess          = nullptr;   
    HWND   hGameWnd = nullptr;   


    // Hooks removed (stripped for open-source release)
    // NtAsyncArrowHook arrowHook;
    // GetFocusPtrHook  focusHook;
    // CursorPosHook    cursorHook;
    // GetForegroundPtrHook foregroundHook;
    // HardwareSpoof    hwSpoof;
    // CNetwork         networkHook;
    StatTracker statTracker;

    // Global logger
    AppLogger logger;

    // lua script
    ScriptEngine script;


    // Lua threading
    std::thread luaThread;
    std::atomic<bool> luaRunning{false};
    std::atomic<bool>  luaCancelRequested{false};

    bool bufferPath = false;
    int currentTab = 0;

    // App window handle (for self-screenshot)
    HWND hAppWnd = nullptr;

    // Map canvas screenshot support
    RECT mapCanvasRect = {};          // client-space rect of map canvas, updated each frame
    bool mapCanvasValid = false;      // true when mapCanvasRect is populated
    std::atomic<int> forceTab{-1};    // set to tab index to force-switch next frame, -1 = no request
    int prevTab = -1;                 // tab to restore after screenshot
    AppState()
    {
        
    }

    // --- Lua script UI state ---
    std::string GetScriptPath() {
        return Settings::Instance().GetString("lua.last_script");
    }
    
    void SetScriptPath(const std::string& path) {
        Settings::Instance().Set("lua.last_script", path);
    }


};

extern AppState app;



// Local helper; internal linkage so no clash even if you kept another static
// IsProcessRunning in WinMain.cpp.
static bool IsProcessRunning(DWORD pid)
{
    if (pid == 0) return false;
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (h)
    {
        CloseHandle(h);
        return true;
    }
    return false;
}
