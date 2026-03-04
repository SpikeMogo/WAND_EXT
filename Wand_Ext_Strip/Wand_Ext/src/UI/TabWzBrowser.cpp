// TabWzBrowser.cpp
#include "TabWzBrowser.h"
#include <wzparser/MapPathfinder.h>
#include <wzparser/MapData.h>
#include <imgui/imgui.h>
#include <filesystem>
#include "AppState.h"
#include <class/MapleClass.h>
#include <thread>
#include <navigator/Map.h>
#include <atomic>
#include <mutex>
#include <wzparser/MissingPortals.h>


// ============================================================================
// Thread State
// ============================================================================

// Walking thread
static std::thread s_walkThread;
static std::atomic<bool> s_walkingActive{false};
static std::atomic<int> s_currentWalkStep{0};

// Loading thread
static std::thread s_loadThread;
static std::atomic<bool> s_isLoading{false};
static std::atomic<bool> s_shutdownRequested{false};

// Shared data protected by mutex
static std::mutex s_dataMutex;
static std::string s_wzRootPath;
static pathfinder::PathResult s_lastPathResult;
static int s_lastFromMapId = 0;
static int s_lastToMapId = 0;

// ============================================================================
// Thread Management
// ============================================================================

static void JoinWalkThread() {
    s_walkingActive = false;
    if (s_walkThread.joinable()) {
        s_walkThread.join();
    }
}

static void JoinLoadThread() {
    if (s_loadThread.joinable()) {
        s_loadThread.join();
    }
}

void ShutdownWzBrowser() {
    // Signal shutdown
    s_shutdownRequested = true;
    s_walkingActive = false;
    
    // Wait for threads
    JoinWalkThread();
    JoinLoadThread();
    
    // Clear data
    mapdata::GetMapDataCache().Clear();
    
    // Reset state
    s_shutdownRequested = false;
    s_isLoading = false;
}

// ============================================================================
// Process Lifecycle
// ============================================================================

void OnProcessAttachedWZ(const std::string& exeFullPath) {
    // Stop any existing operations
    s_shutdownRequested = true;
    JoinWalkThread();
    JoinLoadThread();
    
    // Clear old data
    mapdata::GetMapDataCache().Clear();
    {
        std::lock_guard<std::mutex> lock(s_dataMutex);
        s_lastPathResult.Clear();
        s_lastFromMapId = 0;
        s_lastToMapId = 0;
    }
    
    // Get WZ path
    std::filesystem::path p(exeFullPath);
    {
        std::lock_guard<std::mutex> lock(s_dataMutex);
        s_wzRootPath = p.parent_path().string();
    }
    
    // Reset flags
    s_shutdownRequested = false;
    s_isLoading = true;
    
    // Set shutdown check callback
    mapdata::GetMapDataCache().SetShutdownCheck([]() {
        return s_shutdownRequested.load();
    });
    
    // Start loading in background
    DWORD pid = app.selectedPid;  // Capture PID before thread launch
    s_loadThread = std::thread([pid]() {
        if (s_shutdownRequested) {
            s_isLoading = false;
            return;
        }

        std::string path;
        {
            std::lock_guard<std::mutex> lock(s_dataMutex);
            path = s_wzRootPath;
        }

        mapdata::GetMapDataCache().LoadFromWz(path, pid);

        auto& cache = mapdata::GetMapDataCache();
        mapdata::LoadMissingPortals(cache);

        s_isLoading = false;
    });
}

void OnProcessDetachedWZ() {
    ShutdownWzBrowser();
}

// ============================================================================
// UI Helpers
// ============================================================================

static void DrawPortalTable(const std::vector<mapdata::PortalData>& portals) {
    if (portals.empty()) {
        ImGui::TextDisabled("No portals");
        return;
    }
    
    ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | 
                            ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY |
                            ImGuiTableFlags_ScrollX;
                            
    if (ImGui::BeginTable("Portals", 6, flags, ImVec2(0, 200))) {
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 30);
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 40);
        ImGui::TableSetupColumn("Position");
        ImGui::TableSetupColumn("Target Map");
        ImGui::TableSetupColumn("Target Name");
        ImGui::TableHeadersRow();
        
        for (const auto& p : portals) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%d", p.id);
            ImGui::TableNextColumn(); ImGui::Text("%s", p.name.c_str());
            ImGui::TableNextColumn(); ImGui::Text("%d", p.type);
            ImGui::TableNextColumn(); ImGui::Text("(%d, %d)", p.x, p.y);
            ImGui::TableNextColumn();
            if (p.targetMapId != 999999999) {
                ImGui::Text("%d", p.targetMapId);
            } else {
                ImGui::TextDisabled("---");
            }
            ImGui::TableNextColumn(); ImGui::Text("%s", p.targetName.c_str());
        }
        ImGui::EndTable();
    }
}

static void DrawPathTable(const pathfinder::PathResult& path) {
    if (!path.found || path.steps.empty()) return;
    
    ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                           ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX;
                           
    if (ImGui::BeginTable("PathTable", 4, flags, ImVec2(0, 150))) {
        ImGui::TableSetupColumn("Step", ImGuiTableColumnFlags_WidthFixed, 40);
        ImGui::TableSetupColumn("Map");
        ImGui::TableSetupColumn("Portal");
        ImGui::TableSetupColumn("Map ID", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableHeadersRow();
        
        for (size_t i = 0; i < path.steps.size(); i++) {
            const auto& step = path.steps[i];
            ImGui::TableNextRow();
            
            ImGui::TableNextColumn();
            ImGui::Text("%zu", i + 1);
            
            ImGui::TableNextColumn();
            ImGui::Text("%s", step.mapName.c_str());
            
            ImGui::TableNextColumn();
            if (!step.portalName.empty()) {
                ImGui::Text("%s (%d, %d)", step.portalName.c_str(), step.portalX, step.portalY);
            } else {
                ImGui::TextDisabled("(destination)");
            }
            
            ImGui::TableNextColumn();
            ImGui::Text("%d", step.mapId);
        }
        ImGui::EndTable();
    }
}

// ============================================================================
// Main Draw Function
// ============================================================================

void DrawWzBrowserTab(AppState& app, ImGuiIO& io) {
    auto& cache = mapdata::GetMapDataCache();
    
    // ========================================================================
    // Loading State
    // ========================================================================
    if (s_isLoading) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Loading Maple Worlds Structure...");
        
        float progress = cache.GetLoadProgress();
        int loaded = cache.GetLoadedMaps();
        int total = cache.GetTotalMaps();
        
        ImGui::ProgressBar(progress, ImVec2(-1, 0));
        
        if (total > 0) {
            ImGui::Text("Processing: %d / %d maps (%.0f%%)", loaded, total, progress * 100.0f);
        } else {
            ImGui::Text("%s", cache.GetLoadStatus().c_str());
        }
        
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "WZ files will be closed after loading.");
        return;
    }
    
    // ========================================================================
    // Not Loaded State
    // ========================================================================
    if (!cache.IsLoaded()) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), 
            "Map data not loaded. Attach to MapleStory first.");
        return;
    }
    
    // ========================================================================
    // Loaded - Show Browser
    // ========================================================================
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), 
        "Loaded %zu maps", cache.MapCount());
    
    ImGui::Separator();
    
    // Search box
    static char searchBuf[64] = "";
    ImGui::SetNextItemWidth(200);
    ImGui::InputTextWithHint("##search", "Search map name or ID...", searchBuf, sizeof(searchBuf));
    
    ImGui::SameLine();
    
    // Current map button
    static bool scrollToSelected = false;
        
    // Selection state
    static int selectedMapId = 0;
    static const mapdata::MapData* selectedMap = nullptr;
    
    if (ImGui::Button("Current"))
    {
        uint32_t currentMapId = 0;
        if (CWvsPhysicalSpace2D::GetMapID(app.hProcess, currentMapId) && currentMapId != 0)
        {
            // Fill search box with map ID
            snprintf(searchBuf, sizeof(searchBuf), "%u", currentMapId);
            
            // Select the map
            const mapdata::MapData* data = cache.GetMap(static_cast<int32_t>(currentMapId));
            if (data)
            {
                selectedMapId = data->mapId;
                selectedMap = data;
                scrollToSelected = true;
            }
        }
    }
    ImGui::SetItemTooltip("Go to current map");
    
    ImGui::Separator();

    
    // Layout
    float panelHeight = ImGui::GetContentRegionAvail().y - 10;
    float treeWidth = ImGui::GetContentRegionAvail().x * 0.38f;
    
    // ========================================================================
    // Left Panel: Map Tree
    // ========================================================================
    ImGui::BeginChild("TreePane", ImVec2(treeWidth, panelHeight), true, 
                      ImGuiWindowFlags_HorizontalScrollbar);
    
    // Search filter
    bool hasFilter = (searchBuf[0] != '\0');
    std::string filterLower;
    if (hasFilter) {
        filterLower = searchBuf;
        for (char& c : filterLower) c = static_cast<char>(tolower(c));
    }
    
    // Draw regions and maps
    for (const auto& regionName : cache.GetRegions()) {
        auto mapsInRegion = cache.GetMapsInRegion(regionName);
        
        // Apply filter
        std::vector<const mapdata::MapData*> filtered;
        filtered.reserve(mapsInRegion.size());
        
        for (const auto* m : mapsInRegion) {
            if (hasFilter) {
                std::string text = m->mapName + " " + m->streetName + " " + std::to_string(m->mapId);
                for (char& c : text) c = static_cast<char>(tolower(c));
                if (text.find(filterLower) == std::string::npos) continue;
            }
            filtered.push_back(m);
        }

        // Sort by map ID when filtering
        if (hasFilter) {
            std::sort(filtered.begin(), filtered.end(), 
                [](const mapdata::MapData* a, const mapdata::MapData* b) {
                    return a->mapId < b->mapId;
                });
        }

        if (filtered.empty()) continue;
        
        // Auto-expand when searching
        if (hasFilter) {
            ImGui::SetNextItemOpen(true);
        }
        
        // Region label
        std::string label = regionName;
        if (hasFilter) {
            label += " (" + std::to_string(filtered.size()) + ")";
        }
        
        if (ImGui::TreeNode(regionName.c_str(), "%s", label.c_str())) {
            for (const auto* m : filtered) {
                // Display label
                std::string mapLabel = m->mapName.empty() 
                    ? std::to_string(m->mapId)
                    : m->mapName + " (" + std::to_string(m->mapId) + ")";
                
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | 
                                           ImGuiTreeNodeFlags_NoTreePushOnOpen;
                if (selectedMapId == m->mapId) {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }
                
                ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<intptr_t>(m->mapId)), 
                                  flags, "%s", mapLabel.c_str());
                

                // Auto-scroll to selected map
                if (scrollToSelected && m->mapId == selectedMapId)
                {
                    ImGui::SetScrollHereY();
                    scrollToSelected = false;
                }

                // Tooltip
                if (ImGui::IsItemHovered() && !m->streetName.empty()) {
                    ImGui::SetTooltip("%s\n%s\nID: %d", m->mapName.c_str(), m->streetName.c_str(), m->mapId);
                }
                
                // Selection
                if (ImGui::IsItemClicked()) {
                    selectedMapId = m->mapId;
                    selectedMap = m;
                }
            }
            ImGui::TreePop();
        }
    }
    
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // ========================================================================
    // Right Panel: Details
    // ========================================================================
    ImGui::BeginChild("DetailsPane", ImVec2(0, panelHeight), true);
    
    if (selectedMap) {
        // Header
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.4f, 1.0f), "%s", selectedMap->mapName.c_str());
        if (!selectedMap->streetName.empty()) {
            ImGui::SameLine();
            ImGui::TextDisabled("- %s", selectedMap->streetName.c_str());
        }
        ImGui::Text("Map ID: ");
        ImGui::SameLine();
        if (ImGui::SmallButton(std::to_string(selectedMapId).c_str()))
        {
            ImGui::SetClipboardText(std::to_string(selectedMapId).c_str());
        }
        ImGui::SetItemTooltip("click to copy");
        ImGui::Separator();
        
        // Info section
        if (ImGui::CollapsingHeader("Info", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Return Map:    %d", selectedMap->returnMap);
            ImGui::Text("Forced Return: %d", selectedMap->forcedReturn);
            ImGui::Text("Town:          %s", selectedMap->town ? "Yes" : "No");
        }
        
        // Portals section
        if (ImGui::CollapsingHeader("Portals", ImGuiTreeNodeFlags_DefaultOpen)) {
            DrawPortalTable(selectedMap->portals);
        }
        
        // Travel section
        if (ImGui::CollapsingHeader("Travel", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Current map
            uint32_t currentMapId = 0;
            CWvsPhysicalSpace2D::GetMapID(app.hProcess, currentMapId);
            
            ImGui::Text("Current:     %u", currentMapId);
            ImGui::Text("Destination: %d (%s)", selectedMap->mapId, selectedMap->mapName.c_str());
            ImGui::Spacing();
            
            // Pathfinder status
            auto& pf = pathfinder::GetMapPathfinder();
            bool ready = pf.IsReady();
            
            if (ready) {
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Pathfinder ready");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Pathfinder not ready");
            }
            
            // Find Path button
            ImGui::SameLine();

            if (!ready) ImGui::BeginDisabled();
            
            if (ImGui::Button("Find Path")) {
                std::lock_guard<std::mutex> lock(s_dataMutex);
                s_lastPathResult = pf.FindPath(currentMapId, selectedMap->mapId);
                s_lastFromMapId = currentMapId;
                s_lastToMapId = selectedMap->mapId;
            }
            
            if (!ready) ImGui::EndDisabled();
            
            ImGui::Separator();
            
            // Path result (thread-safe copy)
            pathfinder::PathResult pathCopy;
            {
                std::lock_guard<std::mutex> lock(s_dataMutex);
                pathCopy = s_lastPathResult;
            }
            
            if (s_lastFromMapId != 0 || s_lastToMapId != 0) {
                if (pathCopy.found) {
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), 
                        "Path found: %zu steps", pathCopy.steps.size());
                    ImGui::Spacing();
                    DrawPathTable(pathCopy);
                } else if (!pathCopy.error.empty()) {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), 
                        "Error: %s", pathCopy.error.c_str());
                }
            }
            
            // Walk controls
            ImGui::Spacing();
            
            if (!s_walkingActive) {
                bool canWalk = ready && pathCopy.found && !pathCopy.steps.empty();
                if (!canWalk) ImGui::BeginDisabled();
                
                if (ImGui::Button("Walk Path")) {
                    // Stop any existing walk
                    JoinWalkThread();
                    
                    s_walkingActive = true;
                    s_currentWalkStep = 0;
                    
                    s_walkThread = std::thread([&app]() {
                        auto& pf = pathfinder::GetMapPathfinder();
                        
                        CWndMan::resetFocus(app.hProcess); 

                        int FrameDelay = Settings::Instance().GetInt("general.map_travel_delay");
                        while (s_walkingActive && !s_shutdownRequested) {
                            // Get current map
                            uint32_t currentMap = 0;
                            CWvsPhysicalSpace2D::GetMapID(app.hProcess, currentMap);
                            
                            // Update step indicator
                            {
                                std::lock_guard<std::mutex> lock(s_dataMutex);
                                for (size_t i = 0; i < s_lastPathResult.steps.size(); i++) {
                                    if (s_lastPathResult.steps[i].mapId == static_cast<int32_t>(currentMap)) {
                                        s_currentWalkStep = static_cast<int>(i) + 1;
                                        break;
                                    }
                                }
                            }
                            
                            // Walk one step
                            int result;
                            {
                                std::lock_guard<std::mutex> lock(s_dataMutex);
                                result = pf.WalkPath(s_lastPathResult, 0);
                            }
                            
                            if (result != 0) {
                                // Arrived (1) or error (<0)
                                break;
                            }
                            
                            Sleep(FrameDelay);
                        }
                        
                        s_walkingActive = false;
                    });
                }
                
                if (!canWalk) ImGui::EndDisabled();
            } else {
                // Walking in progress
                if (ImGui::Button("Stop")) {
                    s_walkingActive = false;
                    CInputSystem::StopMove(app);
                }
                
                ImGui::SameLine();
                
                int step = s_currentWalkStep.load();
                int total;
                {
                    std::lock_guard<std::mutex> lock(s_dataMutex);
                    total = static_cast<int>(s_lastPathResult.steps.size());
                }
                
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), 
                    "Walking... %d / %d", step, total);
            }
        }
    } else {
        ImGui::TextDisabled("Select a map from the list");
    }
    
    ImGui::EndChild();
}