#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

// Now the rest of your includes
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <UI/TabWzBrowser.h>

#include "AppState.h"
#include <UI/TabMain.h>
#include <UI/TabSetting.h>
#include <UI/TabMap.h>
#include <UI/UIMain.h>

#include "LuaBindings.h"
#include "DiscordBot.h"      // <-- Add this

#include <d3d11.h>
#include <tchar.h>
#include <future>

#include <navigator/Map.h>
#include <class/MapleClass.h>

#include "Settings.h"
#include "SettingsConfig.h"
#include "PathfinderTunableConfig.h"
#include "Hotkey.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// Globals
static ID3D11Device*           g_pd3dDevice           = nullptr;
static ID3D11DeviceContext*    g_pd3dDeviceContext    = nullptr;
static IDXGISwapChain*         g_pSwapChain           = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Forward declarations
bool     CreateDeviceD3D(HWND hWnd);
void     CleanupDeviceD3D();
void     CreateRenderTarget();
void     CleanupRenderTarget();
LRESULT  WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);



// Make appstate
AppState app;

// Main
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    const wchar_t* className = L"WAND_EXT_CLASS";
    const wchar_t* windowTitle = L"WAND-EXT";

    // Register window class
    WNDCLASSEXW wc = {
        sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L,
        GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
        className, nullptr
    };
    ::RegisterClassExW(&wc);


    #if defined(_DEBUG) || defined(_DPRINT)
        AllocConsole();
        FILE* fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONIN$", "r", stdin);

        // Disable the close button on console
        HWND consoleWnd = GetConsoleWindow();
        HMENU hMenu = GetSystemMenu(consoleWnd, FALSE);
        DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
    #endif


    // Create window

    DWORD style =
    WS_OVERLAPPED |   // basic top-level window
    WS_CAPTION   |    // title bar + border, makes it draggable
    WS_SYSMENU;       // system menu + close button

    DWORD exStyle = WS_EX_APPWINDOW;  // show in taskbar (optional but common)

    HWND hwnd = ::CreateWindowExW(
        exStyle,
        wc.lpszClassName,
        windowTitle,
        style,
        600, 400,   // position
        432, 800,   // size (overall window, not exact client)
        nullptr, nullptr,
        wc.hInstance,
        nullptr
    );
    // Init D3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    app.hAppWnd = hwnd;

    // ImGui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = nullptr; 

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);


    // ---- Load Consolas Font ----
    ImFont* fontConsolas = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 13.0f);
    IM_ASSERT(fontConsolas != nullptr);

    // If you want to use this as default:
    io.FontDefault = fontConsolas;

    // Build font texture
    ImGui_ImplDX11_CreateDeviceObjects();

    // lua states
    app.logger.Add("");
    app.logger.Add("╔══════════════════════════════════════╗");
    app.logger.Add("║         Wand 3.0 - External          ║");
    app.logger.Add("║         Author: Spike (2025)         ║");
    app.logger.Add("╠══════════════════════════════════════╣");
    app.logger.Add("║  Discord:  discord.gg/vBbq3bey5D     ║");
    app.logger.Add("║  Docs:     spikemogo.github.io/WAND  ║");
    app.logger.Add("╚══════════════════════════════════════╝");
    app.logger.Add("");

    SetupLuaEnvironment(app, app.script);
    app.logger.Add("[Lua] environment initialized");


    Settings::Instance().Load("bot_settings.ini");
    RegisterAllSettings();
    PathfinderTunableConfig::Reload(); 
    app.logger.Add("[Lua] setting is loaded");


    // Main loop
    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // === Single full-screen ImGui window ===
        ImGuiIO& io_ref = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(io_ref.DisplaySize);

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize   |
            ImGuiWindowFlags_NoMove     |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings;


        // === Begin UI ===
        DrawMainUI(app,flags);
        // === End UI ===

        // === Global Hotkey Polling ===
        if (!ImGui::GetIO().WantTextInput && !Settings::IsCapturingHotkey()) {
            static bool prevRunDown = false;
            static bool prevStopDown = false;
            auto& settings = Settings::Instance();

            // Run Script hotkey
            if (PollHotkey(settings.GetInt("hotkey.run_script"), prevRunDown)) {
                if (!app.luaRunning.exchange(true)) {
                    CWndMan::resetFocus(app.hProcess);
                    app.luaCancelRequested = false;
                    app.statTracker.Reset();
                    if (app.luaThread.joinable())
                        app.luaThread.join();
                    std::string scriptPath = app.GetScriptPath();
                    app.luaThread = std::thread([scriptPath]() {
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
                        app.script.ClearCancelHook();
                        CInputSystem::StopMove(app);
                        app.luaRunning = false;
                    });
                }
            }

            // Stop Script hotkey
            if (PollHotkey(settings.GetInt("hotkey.stop_script"), prevStopDown)) {
                if (app.luaRunning.load()) {
                    app.luaCancelRequested = true;
                    CInputSystem::StopMove(app);
                    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
                    while (app.luaRunning.load() && std::chrono::steady_clock::now() < deadline)
                        Sleep(50);
                    if (app.luaRunning.load()) {
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
        }
        // === End Hotkey Polling ===

        //  === Update arrow hook block state (stripped) ===
        // === end  ===

        // === LoadMap ===
        uint32_t MapID=0;
        uint32_t channel=0;
        uint32_t totChan=0;

        if(MapleMap::IfFieldReady(app.hProcess) && 
            (   (CWvsPhysicalSpace2D::GetMapID(app.hProcess, MapID)    && MapID   != MapleMap::MapID    ) ||
                (CUserLocal::GetChannel(app.hProcess,channel, totChan) && channel != MapleMap::ChannelID) )
            )
        {

            app.logger.AddV("Loading New Map [%u]",MapID);

            //try 3 times
            int attempts = 0;
            while (attempts < 3 && !(MapleMap::LoadMap(app.hProcess) && MapID == MapleMap::MapID)) {
                attempts++;
                Sleep(200);
            }

            if (MapID == MapleMap::MapID) {
                app.logger.AddV("Successfully Loaded Map [%u]", MapID);
            } else {
                app.logger.AddV("Failed to Load Map [%u] after %d attempts", MapID, attempts);
            }

            PathFinding::g_PathRenderBuffer.Clear();

            CInputSystem::SendKey(app,VK_F13);
        }

        // === End LoadMap ===

        // Render
        ImGui::Render();
        const float clear_color_with_alpha[4] = {
            app.clear_color.x * app.clear_color.w,
            app.clear_color.y * app.clear_color.w,
            app.clear_color.z * app.clear_color.w,
            app.clear_color.w
        };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // vsync

    }


    // Signal cancellation first so threads can start winding down
    if (app.luaRunning.load()) {
        app.luaCancelRequested = true;
    }

    // Shutdown Discord bot (signals stopRequested, joins bot + panel + chatlog threads)
    DiscordBot::Instance().Stop();

    // Wait for Lua thread with timeout
    if (app.luaThread.joinable()) {
        auto luaFuture = std::async(std::launch::async, [&]() { app.luaThread.join(); });
        if (luaFuture.wait_for(std::chrono::seconds(3)) == std::future_status::timeout) {
            app.logger.AddE("[Shutdown] Lua thread did not stop in time, detaching.");
            app.luaThread.detach();
        }
    }

    //Shutdown browser (may take time to cancel loading thread)
    ShutdownWzBrowser();

    if (app.hProcess)
    {
        // Hooks stripped for open-source release
        CloseHandle(app.hProcess);
        app.hProcess = nullptr;
    }


    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// D3D helpers
bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount                        = 2;
    sd.BufferDesc.Width                   = 0;
    sd.BufferDesc.Height                  = 0;
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = hWnd;
    sd.SampleDesc.Count                   = 1;
    sd.SampleDesc.Quality                 = 0;
    sd.Windowed                           = TRUE;
    sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    // createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0
    };

    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        createDeviceFlags, featureLevelArray, 2,
        D3D11_SDK_VERSION, &sd, &g_pSwapChain,
        &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);

    if (res == DXGI_ERROR_UNSUPPORTED)
    {
        res = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
            createDeviceFlags, featureLevelArray, 2,
            D3D11_SDK_VERSION, &sd, &g_pSwapChain,
            &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    }
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain)        { g_pSwapChain->Release();        g_pSwapChain        = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice)        { g_pd3dDevice->Release();        g_pd3dDevice        = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    if (pBackBuffer)
        pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView)
    {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{



    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;



    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(
                0,
                (UINT)LOWORD(lParam),
                (UINT)HIWORD(lParam),
                DXGI_FORMAT_UNKNOWN,
                0);
            CreateRenderTarget();
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT menu
            return 0;
        break;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }

    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
