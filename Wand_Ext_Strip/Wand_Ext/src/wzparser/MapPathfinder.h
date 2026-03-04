// MapPathfinder.h
#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include "MapData.h"

namespace pathfinder {

// A single step in the travel path
struct PathStep {
    int32_t mapId = 0;
    int32_t toMapId = 0;
    std::string mapName;
    std::string portalName;
    int32_t portalX = 0;
    int32_t portalY = 0;
};

// Result of pathfinding
struct PathResult {
    bool found = false;
    std::vector<PathStep> steps;
    std::string error;
    
    void Clear() {
        found = false;
        steps.clear();
        error.clear();
    }
};

// Map pathfinder - uses MapDataCache for BFS pathfinding
class MapPathfinder {
public:
    // Find shortest path using BFS
    // Returns immediately with cached data - no disk I/O
    PathResult FindPath(int32_t fromMapId, int32_t toMapId);
    
    // Walk the path - call from a dedicated thread
    // Returns: 1=arrived, 0=in progress, <0=error
    int WalkPath(PathResult& path, int type = 0);

    int GetNextPortal(int32_t toMapId, pathfinder::PathStep& step);
    
    // Check if ready (MapDataCache loaded)
    bool IsReady() const;
};

// Global instance
MapPathfinder& GetMapPathfinder();

} // namespace pathfinder