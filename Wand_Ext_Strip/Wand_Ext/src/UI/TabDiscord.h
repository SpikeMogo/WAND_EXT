// TabDiscord.h
#pragma once

// Must include WinSock2 BEFORE windows.h to avoid conflicts
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#include "AppState.h"

void DrawDiscordTab(AppState& app);