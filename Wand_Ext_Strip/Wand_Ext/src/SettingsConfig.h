// SettingsConfig.h
#pragma once
#include "Settings.h"

inline void RegisterAllSettings() {
    auto& s = Settings::Instance();
    
    // General
    s.Register("general.verbose", false);
    s.Register("general.map_travel_delay", 10);  
    s.Register("general.shopping_delay", 600);  

    s.Register("hwspoof.hardware_spoof", false);
    s.Register("hwspoof.mac", std::string("57-41-4E-44-45-58"));
    s.Register("hwspoof.serial", std::string("5350494B"));


    

    s.Register("pathfind.movement_sampling", 20); 
    s.Register("pathfind.velocity_scale", 1.03); 
    s.Register("pathfind.foothold_layer", true); 
    s.Register("pathfind.enable_warm_start", true);  
    s.Register("pathfind.teleport_prob", 1.0);  
    s.Register("pathfind.enable_cross_teleport", true);
    s.Register("pathfind.path_randomness", 0.3);



    s.Register("discord.token",      std::string("token"));
    s.Register("discord.channel_id", std::string("channel_id"));
    s.Register("discord.guild_id", std::string("guild_id"));
    
    // Image matching
    s.Register("image_match.threshold", 5);  // SAD threshold: 0 = exact, 0-25 range

    s.Register("lua.last_script", std::string("F:/Wand_Ext/Wand_Ext/script/test.lua"));

    // Hotkeys (0 = no hotkey assigned)
    s.Register("hotkey.run_script",  0);
    s.Register("hotkey.stop_script", 0);

    // // Hunt
    // s.Register("hunt.attack_delay", 150);
    // s.Register("hunt.loot_delay", 100);
    // s.Register("hunt.hp_threshold", 0.5);
    
    // // Movement
    // s.Register("move.jump_key", 0x20);
    // s.Register("move.use_teleport", false);
    
    // Add your own settings here...
}