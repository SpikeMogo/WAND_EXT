// MapData.h
// Lightweight cached map data - extracted from WZ files then WZ is closed
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <atomic>
#include <functional>
#include <windows.h>

namespace mapdata {

// Portal info (extracted from Map.wz)
struct PortalData {
    int32_t id = 0;
    std::string name;       // pn
    int32_t type = 0;       // pt
    int32_t x = 0;
    int32_t y = 0;
    int32_t targetMapId = 999999999;  // tm
    std::string targetName; // tn
};

// Map info (extracted from Map.wz and String.wz)
struct MapData {
    int32_t mapId = 0;
    std::string mapName;      // from String.wz
    std::string streetName;   // from String.wz
    std::string regionName;   // folder name in String.wz (victoria, etc.)
    
    // From Map.wz info node
    int32_t returnMap = 999999999;
    int32_t forcedReturn = 999999999;
    bool town = false;
    
    // Portals
    std::vector<PortalData> portals;
};

// All cached map data
class MapDataCache {
public:
    // Load all data from WZ files, then close them
    // gamePid: optional game process PID for ReadProcessMemory-based PCOM key extraction
    bool LoadFromWz(const std::string& wzRootPath, DWORD gamePid = 0);
    
    // Check if loaded
    bool IsLoaded() const { return m_loaded.load(); }
    
    // Get map by ID (thread-safe after loading)
    const MapData* GetMap(int32_t mapId) const;
    
    // Get all maps
    const std::unordered_map<int32_t, MapData>& GetAllMaps() const { return m_maps; }
    
    // Get regions (for tree view)
    const std::vector<std::string>& GetRegions() const { return m_regions; }
    
    // Get maps in a region
    std::vector<const MapData*> GetMapsInRegion(const std::string& region) const;
    
    // Clear all data
    void Clear();
    
    // Stats
    size_t MapCount() const { return m_maps.size(); }
    
    // Progress tracking (for UI) - thread-safe
    float GetLoadProgress() const { return m_loadProgress.load(); }
    std::string GetLoadStatus() const;
    int GetTotalMaps() const { return m_totalMaps.load(); }
    int GetLoadedMaps() const { return m_loadedMaps.load(); }
    
    // Shutdown check callback (set by caller to allow early exit)
    void SetShutdownCheck(std::function<bool()> check) { m_shutdownCheck = check; }

    MapData* GetMapMut(int32_t mapId) {
        auto it = m_maps.find(mapId);
        return (it != m_maps.end()) ? &it->second : nullptr;
    }

private:
    std::unordered_map<int32_t, MapData> m_maps;
    std::vector<std::string> m_regions;
    std::atomic<bool> m_loaded{false};
    
    // Progress tracking (atomic for thread safety)
    std::atomic<float> m_loadProgress{0.0f};
    mutable std::string m_loadStatus;  // Protected by atomic flag pattern
    std::atomic<int> m_totalMaps{0};
    std::atomic<int> m_loadedMaps{0};
    
    // Shutdown check
    std::function<bool()> m_shutdownCheck;
};

// Global instance
MapDataCache& GetMapDataCache();

} // namespace mapdata