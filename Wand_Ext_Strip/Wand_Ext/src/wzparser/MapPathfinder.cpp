// MapPathfinder.cpp — Stripped for open-source release
#include "MapPathfinder.h"
#include <navigator/Map.h>
#include "AppState.h"
#include <class/MapleClass.h>

namespace pathfinder {

// Global instance
MapPathfinder& GetMapPathfinder() {
    static MapPathfinder instance;
    return instance;
}

bool MapPathfinder::IsReady() const {
    return mapdata::GetMapDataCache().IsLoaded();
}

PathResult MapPathfinder::FindPath(int32_t fromMapId, int32_t toMapId) {
    // Stripped — implement your own map pathfinding here
    PathResult result;
    result.error = "Not implemented (stripped)";
    return result;
}

int MapPathfinder::GetNextPortal(int32_t toMapId, pathfinder::PathStep& step) {
    // Stripped — implement your own portal navigation here
    return -1;
}

int MapPathfinder::WalkPath(PathResult& path, int type) {
    // Stripped — implement your own path walking here
    return -1;
}

} // namespace pathfinder
