// MapData.cpp — Stripped for open-source release
#include "MapData.h"

namespace mapdata {

// Global instance
MapDataCache& GetMapDataCache() {
    static MapDataCache instance;
    return instance;
}

void MapDataCache::Clear() {
    m_maps.clear();
    m_regions.clear();
    m_loaded = false;
    m_loadProgress = 0.0f;
    m_loadStatus = "";
    m_totalMaps = 0;
    m_loadedMaps = 0;
    m_shutdownCheck = nullptr;
}

const MapData* MapDataCache::GetMap(int32_t mapId) const {
    auto it = m_maps.find(mapId);
    return (it != m_maps.end()) ? &it->second : nullptr;
}

std::string MapDataCache::GetLoadStatus() const {
    return m_loadStatus;
}

std::vector<const MapData*> MapDataCache::GetMapsInRegion(const std::string& region) const {
    std::vector<const MapData*> result;
    for (const auto& [id, data] : m_maps) {
        if (data.regionName == region) {
            result.push_back(&data);
        }
    }
    return result;
}

bool MapDataCache::LoadFromWz(const std::string& wzRootPath, DWORD gamePid) {
    // Stripped — implement your own WZ file parsing here
    return false;
}

} // namespace mapdata
