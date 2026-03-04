#pragma once
#include "Settings.h"

// PathfinderTunableConfig is a fast cache for settings used in hot loops.
// Problem: Settings::Instance().GetInt() does a hash lookup every call. 
// example In pathfinding code called thousands of times, this slows map building .
// Solution: Load settings once into plain struct members. Access is as fast as the original constexpr constants.

struct PathfinderTunableConfig {
    int kTrajectorySamplingSize = 20;
    int kTrajectorySamplingEdgeExclusion = 10;

    double kVelocityScale = 1.03;
    bool kUseFootholdLayer = true;
    bool kEnableWarmStart = true;
    double kPathRandomness = 0.3;

    // add other pathfind constants here
    
    static PathfinderTunableConfig& Get() {
        static PathfinderTunableConfig instance;
        return instance;
    }
    
    static void Reload() {
        auto& cfg = Get();
        auto& s = Settings::Instance();
        cfg.kTrajectorySamplingSize = s.GetInt("pathfind.movement_sampling");
        cfg.kTrajectorySamplingEdgeExclusion = cfg.kTrajectorySamplingSize/2;
        cfg.kUseFootholdLayer = s.GetBool("pathfind.foothold_layer");
        cfg.kEnableWarmStart = s.GetBool("pathfind.enable_warm_start");
        cfg.kVelocityScale = s.GetDouble("pathfind.velocity_scale");
        cfg.kPathRandomness = s.GetDouble("pathfind.path_randomness");
    }
};