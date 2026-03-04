// TabWzBrowser.h
#pragma once

#include <string>

struct AppState;
struct ImGuiIO;

// Process lifecycle
void OnProcessAttachedWZ(const std::string& exeFullPath);
void OnProcessDetachedWZ();

// Call this when app is closing (before destroying AppState)
void ShutdownWzBrowser();

// Main UI draw function
void DrawWzBrowserTab(AppState& app, ImGuiIO& io);