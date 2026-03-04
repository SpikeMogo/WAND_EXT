// Stripped for open-source release — implement your own input methods
#pragma once

#include <Windows.h>
#include <cstdint>

struct AppState;

class CInputSystem
{
public:
    static bool PressKey(AppState& app, uint8_t vk) { return false; }
    static bool ReleaseKey(AppState& app, uint8_t vk) { return false; }
    static bool SendKey(AppState& app, uint8_t vk) { return false; }
    static bool SendKeyDelay(AppState& app, uint8_t vk, int delay = 50) { return false; }
    static bool SendKey(AppState& app, uint8_t vk, int repeat) { return false; }
    static void StopMove(AppState& app) {}
    static bool GetCursorGamePos(HANDLE hProcess, int& X, int& Y) { return false; }
    static bool Click(AppState& app, int x, int y) { return false; }
    static bool DoubleClick(AppState& app, int x, int y, int delayMs = 50) { return false; }
};
