// UIMain.h
#pragma once

#include "AppState.h"
#include <imgui/imgui.h>

// flags can be by value; we don't modify them
void DrawMainUI(AppState& app, ImGuiWindowFlags flags);
