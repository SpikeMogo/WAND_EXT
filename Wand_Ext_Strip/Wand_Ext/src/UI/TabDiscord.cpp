// TabDiscord.cpp
#include <UI/TabDiscord.h>
#include "imgui.h"
#include "Settings.h"
#include "DiscordBot.h"
#include "DiscordCommands.h"


static void DrawStatusIndicator(DiscordStatus status) {
    ImVec4 color;
    const char* tooltip;
    
    switch (status) {
        case DiscordStatus::Connected:
            color = ImVec4(0.2f, 0.9f, 0.2f, 1.0f);
            tooltip = "Connected";
            break;
        case DiscordStatus::Connecting:
            color = ImVec4(0.9f, 0.9f, 0.2f, 1.0f);
            tooltip = "Connecting...";
            break;
        default:
            color = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
            tooltip = "Disconnected";
            break;
    }
    
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::Text("●");
    ImGui::PopStyleColor();
    
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", tooltip);
}

void DrawDiscordTab(AppState& app) {
    auto& s = Settings::Instance();
    auto& bot = DiscordBot::Instance();
    ImGui::SeparatorText("Discord Bot  (hover here to see help)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Discord bot integration.\n\n"
            "Requires dpp.dll in the same folder as this executable.\n"
            "If missing, install it using vcpkg and copy from:\n"
            "  vcpkg\\installed\\x86-windows\\bin\\dpp.dll\n\n"
            "May also need: libssl-*.dll, libcrypto-*.dll, zlib1.dll"
        );
    }

    // Status header
    DrawStatusIndicator(bot.GetStatus());
    ImGui::SameLine();
    ImGui::TextColored(
        bot.GetStatus() == DiscordStatus::Connected 
            ? ImVec4(0.4f, 0.8f, 0.4f, 1.0f) 
            : ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
        "%s", bot.GetStatusText().c_str()
    );
    
    ImGui::Spacing();
    ImGui::PushItemWidth(200.0f);
    
    // Token input
    s.InputText("Bot Token", "discord.token", true,
        "Get your bot token from Discord Developer Portal");
    
    // Channel ID input
    if (s.InputText("Channel ID", "discord.channel_id", false,
        "Right-click channel -> Copy Channel ID\n(Enable Developer Mode in Discord settings)")) {
        bot.SetChannelId(s.GetString("discord.channel_id"));
    }

    // Guild ID input
    if (s.InputText("Guild ID", "discord.guild_id", false,
        "Right-click server name -> Copy Server ID\n(Enable Developer Mode in Discord settings)")) {
        bot.SetGuildId(s.GetString("discord.guild_id"));
    }


    
    
    ImGui::PopItemWidth();
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Control buttons
    bool isRunning = bot.IsRunning();
    bool isConnected = bot.GetStatus() == DiscordStatus::Connected;
    
    if (isRunning)
        ImGui::BeginDisabled();
    
    if (ImGui::Button("Connect", ImVec2(100, 0))) {
        bot.onLog = [&app](const std::string& msg) {
            app.logger.Add("[Discord] " + msg);
        };
        
        // Register commands only once
        static bool commandsRegistered = false;
        if (!commandsRegistered) {
            RegisterDiscordCommands(app);
            commandsRegistered = true;
        }
        
        bot.Start(s.GetString("discord.token"), s.GetString("discord.channel_id"), s.GetString("discord.guild_id"));
    }
    
    if (isRunning)
        ImGui::EndDisabled();
    
    ImGui::SameLine();
    
    if (!isRunning)
        ImGui::BeginDisabled();
    
    if (ImGui::Button("Disconnect", ImVec2(100, 0)))
        bot.Stop();
    
    if (!isRunning)
        ImGui::EndDisabled();
    
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    
    if (!isConnected)
        ImGui::BeginDisabled();
    
    if (ImGui::Button("Send Test", ImVec2(100, 0)))
        bot.SendMessage("🤖 Test message from Wand Bot!");
    
    if (!isConnected)
        ImGui::EndDisabled();
    
    // Error display
    std::string error = bot.GetLastError();
    if (!error.empty()) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Error: %s", error.c_str());
    }
    
    // Commands info
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Available Commands:");
    ImGui::BulletText("/hello  - Bot greeting"); 
    ImGui::BulletText("/ping   - Check latency");
    ImGui::BulletText("/show   - Screenshot");
    ImGui::BulletText("/status - Bot status"); 
    ImGui::BulletText("/runlua - Run script"); 
    ImGui::BulletText("/stoplua - Stop script");
    ImGui::BulletText("/chatlog - Toggle live chat monitoring");
    ImGui::BulletText("/panel - Show control panel");




}