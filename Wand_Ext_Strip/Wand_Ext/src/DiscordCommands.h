// DiscordCommands.h
#pragma once

#include <string>
#include <vector>
#include <cstdint>

#pragma warning(push)
#pragma warning(disable: 4251)
#include <dpp/dpp.h>
#pragma warning(pop)

struct AppState;

// Call BEFORE DiscordBot::Start() to register all commands and buttons
void RegisterDiscordCommands(AppState& app);

// ===== Shared Actions =====
// These return response strings or build embeds, usable by both slash commands and buttons

namespace DiscordActions {
    // Simple actions that return a response string
    std::string RunScript(AppState& app);
    std::string StopScript(AppState& app);
    std::string ToggleChatLog(AppState& app);
    
    // Builds a status embed
    void BuildStatusEmbed(AppState& app, dpp::embed& embed);
    
    // Screenshot - returns true if successful, fills jpgData
    bool CaptureScreenshot(AppState& app, std::vector<uint8_t>& jpgData);

    // Map canvas screenshot - switches to Map tab, captures canvas region
    bool CaptureMapCanvas(AppState& app, std::vector<uint8_t>& jpgData);
    
    // Builds panel message with buttons
    void BuildPanelMessage(dpp::message& msg);

    // Kill game process
    std::string KillProcess(AppState& app);

    // Builds life embed (mob list + player list)
    void BuildLifeEmbed(AppState& app, dpp::embed& embed);
}