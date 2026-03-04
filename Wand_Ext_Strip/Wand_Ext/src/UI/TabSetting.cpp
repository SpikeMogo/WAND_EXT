// TabSetting.cpp
#include <UI/TabSetting.h>
#include "imgui.h"
#include "Settings.h"



void DrawSettingTab(AppState& app) {
    auto& s = Settings::Instance();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Most changes will be effective after switching map");

    if (ImGui::Button("Open Settings File")) {
        ShellExecuteA(nullptr, "open", s.GetFullPath().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%s", s.GetFullPath().c_str());

    ImGui::PushItemWidth(200.0f); 
    //-----------------------------------------------------

    // ------- //
    ImGui::SeparatorText("System Setting");

    s.Checkbox("Verbose Log", "general.verbose",
    "Display verbose debug information for the pathfinder and other relevant systems.");

    s.SliderInt("Map Travel Delay", "general.map_travel_delay", 5, 80,
    "Unit in ms, this is the delay of each frame for continuously pathfinding/moving when traveling the map list. Short delay is recommended, longer delay may help reduce excessive move adjustment");


    s.SliderInt("Internal Shopping Delay", "general.shopping_delay", 300, 2000,
    "Unit in ms, this is the delay of every internal step of the sell and buy, increase it may help the shopping when game's ping is high and shopping is bugged");


    // ------- //
    ImGui::SeparatorText("Hotkeys");

    s.HotkeyInput("Run Script",  "hotkey.run_script",
    "Global hotkey to start the Lua script (works even when window is not focused)");

    s.HotkeyInput("Stop Script", "hotkey.stop_script",
    "Global hotkey to stop the running Lua script (works even when window is not focused)");


    ImGui::SeparatorText("Spoof Setting");

    s.Checkbox("Hardware Spoof", "hwspoof.hardware_spoof",
    "Spoof Hardware Address and ID (change effective after restart wand and game. May cause some issue)");

    s.InputTextWithRandom("Fake MAC", "hwspoof.mac", []() {
        // Generate random MAC
        char buf[18];
        BYTE mac[6];
        for (int i = 0; i < 6; i++)
            mac[i] = rand() & 0xFF;
        mac[0] &= 0xFE;  // Unicast
        snprintf(buf, sizeof(buf), "%02X-%02X-%02X-%02X-%02X-%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return std::string(buf);
    }, "Spoofed MAC address (change effective after restart and re-attach)");

    s.InputTextWithRandom("Fake Serial", "hwspoof.serial", []() {
        // Generate random serial
        char buf[9];
        DWORD serial = (rand() << 16) | rand();
        snprintf(buf, sizeof(buf), "%08X", serial);
        return std::string(buf);
    }, "Spoofed volume serial (change effective after restart and re-attach)");



    // ------- //
    ImGui::SeparatorText("PathFinder Setting");

    s.SliderInt("Movement Sampling", "pathfind.movement_sampling", 10, 50,
    "Distance of sampling when building possible movements in map, small value may result in very slow computation. 20 ~ 30 is recommended");

    s.SliderDouble("Jump Velocity Scale", "pathfind.velocity_scale", 0.94, 1.10,
    "Slightly rescale the jump velocity, if you find the bot trying to reach platform that cannot reach, dial it down a bit");
    
    s.SliderDouble("Teleport Probability", "pathfind.teleport_prob", 0.00, 1.00,
    "Magician's teleport skill: how frequent to use the skill when move player");
    
    s.SliderDouble("Path Randomness", "pathfind.path_randomness", 0.00, 1.00,
    "Randomness in the connection-build and pathfinder (may cause problem. may not)");

    s.Checkbox("Path Warm Start", "pathfind.enable_warm_start",
    "Warm start with previous path when looking for new path");

    s.Checkbox("Cross Platform Teleport", "pathfind.enable_cross_teleport",
    "Can we teleport to disconnected platforms?");

    s.Checkbox("Enable Foothold Layer", "pathfind.foothold_layer",
    "Take in account the layer difference of footholds in map, i.e., you pass through a wall when its layer is different than yours.");


    // ------- //
    ImGui::SeparatorText("Image Matching");

    s.SliderInt("Match Threshold", "image_match.threshold", 0, 25,
    "SAD threshold for find_image(). 0 = exact pixel match, 5 = default, higher = more lenient. Values above 15 may cause false positives.\n\n"
    "Accepts 24-bit BMP only.\n"
    "To prepare a template, use the game's own screenshot function and crop the section you want to locate.");


    // s.SliderInt("Attack Delay (ms)", "hunt.attack_delay", 50, 500,
    //     "Time between attacks in milliseconds");
    // s.SliderInt("Loot Delay (ms)", "hunt.loot_delay", 50, 300,
    //     "Time to wait before looting after kill");
    // s.SliderDouble("HP Threshold", "hunt.hp_threshold", 0.1, 1.0,
    //     "Use HP potion when health falls below this percentage");
    
    // ImGui::SeparatorText("Movement");
    // s.Checkbox("Use Teleport", "move.use_teleport",
    //     "Use teleport skill for movement if available");
    



    //-----------------------------------------------------
    ImGui::PopItemWidth();

}