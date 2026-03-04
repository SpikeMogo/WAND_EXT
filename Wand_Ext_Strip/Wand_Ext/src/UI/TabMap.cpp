// TabMap.cpp
#include <UI/TabMap.h>
#include <imgui/imgui.h>
#include <math.h> // for sqrtf
#include <class/MapleClass.h>
#include <map> 
#include <cmath>
#include <unordered_map>
#include <class/JobInfo.h>
#include <navigator/Map.h>

struct MobCountInfo
{
    int         count = 0;
    std::string name;   // cached template name
};

// Helper: draw a dashed line using multiple short segments
static void AddDashedLine(
    ImDrawList* draw_list,
    ImVec2 a, ImVec2 b,
    ImU32 col,
    float thickness = 1.0f,
    float dash_len = 8.0f,
    float gap_len  = 4.0f)
{
    ImVec2 delta = ImVec2(b.x - a.x, b.y - a.y);
    float dist = sqrtf(delta.x * delta.x + delta.y * delta.y);
    if (dist <= 0.0f)
        return;

    ImVec2 dir = ImVec2(delta.x / dist, delta.y / dist);

    float t = 0.0f;
    while (t < dist)
    {
        float seg_len = dash_len;
        if (t + seg_len > dist)
            seg_len = dist - t;

        ImVec2 p0 = ImVec2(a.x + dir.x * t,             a.y + dir.y * t);
        ImVec2 p1 = ImVec2(a.x + dir.x * (t + seg_len), a.y + dir.y * (t + seg_len));

        draw_list->AddLine(p0, p1, col, thickness);
        t += dash_len + gap_len;
    }
}


// Draw an analytical trajectory (TrajectoryComputer::Trajectory) on the map,
// from t = 0 until Y(t) reaches yCap (world coordinates, Y increasing downward).
template<typename WorldToScreenFn>
static void DrawTrajectoryToYCap(
    ImDrawList* draw_list,
    const TrajectoryComputer::Trajectory& traj,
    double yCap,
    WorldToScreenFn WorldToScreen,
    double fps = 30.0,
    ImU32 color = IM_COL32(0, 255, 0, 255),
    float thickness = 2.0f)
{
    using namespace TrajectoryComputer;


    if (!draw_list || fps <= 0.0)
        return;
    if(!traj.target && traj.ropeReach.size()==0) return;
    // 1) Find time when trajectory hits Y = yCap
    double t_cap = traj.TimeToY(yCap);

    if(traj.target)
        t_cap = traj.t_target;
    else
    {
        t_cap = 0.0;
        for (const auto& [rope, time] : traj.ropeReach)
        {
            if (!rope)  continue; 
            t_cap = (std::max)(t_cap,time);
            
        }
    }

    if (t_cap < 0.0)
        return; // never reaches that Y -> nothing to draw

    const double dt = 1.0 / fps;
    const int sampleCount = (std::max)(2, static_cast<int>(t_cap / dt) + 2);

    std::vector<ImVec2> pts;
    pts.reserve(sampleCount);

    double t = 0.0;
    for (int i = 0; i < sampleCount; ++i)
    {
        if (t > t_cap) t = t_cap;
        FloatState s = traj.StateAt(t);
        pts.push_back(WorldToScreen(static_cast<float>(s.x),
                                    static_cast<float>(s.y)));
        if (t >= t_cap)
            break;
        t += dt;
    }

    if (pts.size() >= 2)
    {
        draw_list->AddPolyline(
            pts.data(),
            static_cast<int>(pts.size()),
            color,
            false,
            thickness
        );
    }
}


// Small helper so we can write: ImVec2 p = a + b;
static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs)
{
    return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
}

static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs)
{
    return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y);
}

// Cyclic color palette for platforms
static ImU32 GetPlatformColor(int index)
{
    static const ImU32 kPlatformColors[] = {
        IM_COL32(231,  76,  60, 255),  // Red
        IM_COL32(243, 156,  18, 255),  // Orange
        IM_COL32(241, 196,  15, 255),  // Yellow
        IM_COL32( 46, 204, 113, 255),  // Green
        IM_COL32( 39, 174,  96, 255),  // Dark Green
        IM_COL32( 26, 188, 156, 255),  // Teal
        IM_COL32( 22, 160, 133, 255),  // Dark Teal
        IM_COL32( 52, 152, 219, 255),  // Blue
        IM_COL32( 41, 128, 185, 255),  // Dark Blue
        IM_COL32(155,  89, 182, 255),  // Purple
        IM_COL32(142,  68, 173, 255),  // Dark Purple
        IM_COL32(236,  72, 153, 255),  // Pink
        IM_COL32(219,  82, 135, 255),  // Dark Pink
        IM_COL32(255, 109,   0, 255),  // Deep Orange
        IM_COL32(160,  82,  45, 255),  // Brown
        IM_COL32(106,  27, 154, 255),  // Violet
    };

    static const int kNumColors = sizeof(kPlatformColors) / sizeof(kPlatformColors[0]);
    
    // Use prime stride to spread adjacent indices apart
    static const int kStride = 7;  // Pick a prime not dividing kNumColors
    return kPlatformColors[(index * kStride) % kNumColors];
}


const ImU32 fhColor = IM_COL32(0, 129, 167, 255);//foothold
const ImU32 rpColor = IM_COL32(176, 137, 104, 255);//rope
const ImU32 ldColor = IM_COL32(153, 88, 42, 255);//ladder
const ImU32 mbColor = IM_COL32(131, 56, 236,255);//mob
const ImU32 plColor = IM_COL32(255, 190, 11, 255);//player
const ImU32 dpColor = IM_COL32(0, 128, 0, 255); // drops
const ImU32 ruColor = IM_COL32(193, 18, 31, 255); //remote user
const ImU32 pyColor = IM_COL32(251, 133, 0, 255); //party

const ImU32 dmColor = IM_COL32(255, 0, 110, 255); // dangerous mob
const ImU32 npcColor = IM_COL32(0x20, 0xBF, 0x55, 255); // npc

//portal
const ImU32 ptColor1 = IM_COL32(0, 139, 248, 255); //other
const ImU32 ptColor2 = IM_COL32(241, 113, 5, 255); //hidden
const ImU32 ptColor3 = IM_COL32(241, 113, 5, 255); //ivis




void DrawMapTab(AppState& app)
{
    // 0 = Fit (whole map in view, centered)
    // 1 = Zoom (fit width or height to canvas, touch edges)
    // 2 = Center (zoom + center on player)
    static int   s_viewMode    = 0;
    static float s_zoomFactor  = 1.0f;
    static bool  s_showMobs    = true;   
    static bool  s_showDrops   = true;   
    static bool  s_showPortals = true;   
    static bool  s_showOthers  = true;
    static bool  s_showNpcs    = true;
    static bool  s_showPlatforms = false;
    static bool  s_showTrajectory = false;
    static bool  s_showConnections = false;
    static bool  s_showPath = true;




    const char*  kModeNames[3] = { "Mode: Fit", "Mode: Zoom", "Mode: Center" };
    const ImVec2 canvas_size(400.0f/1.02f, 300.0f/1.02f);

    // --- Read map info first so we can show titles on top ---

    if (!app.hProcess)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "No process attached");
        return;
    }

    long left = 0, right = 0, top = 0, bottom = 0, width = 0, height = 0;
    uint32_t mapID = 0;
    std::string streetName;
    std::string mapName;

    if (!CWvsPhysicalSpace2D::GetMap(app.hProcess, left, right, top, bottom, mapID, streetName, mapName))
    {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Failed to read mini map");
        return;
    }

    const float worldW = float(right - left);
    const float worldH = float(bottom - top);
    if (worldW <= 0.0f || worldH <= 0.0f)
        return;

    // === TOP TITLE ===
    ImGui::SeparatorText(streetName.c_str());
    ImGui::Text("%s [%u]", mapName.c_str(), static_cast<unsigned>(mapID));

    // === CANVAS ===
    ImGui::InvisibleButton("MapCanvas", canvas_size);
    ImVec2 canvas_p0 = ImGui::GetItemRectMin(); // top-left
    ImVec2 canvas_p1 = ImGui::GetItemRectMax(); // bottom-right

    // Store canvas rect for map screenshot capture
    app.mapCanvasRect = { (LONG)canvas_p0.x, (LONG)canvas_p0.y, (LONG)canvas_p1.x, (LONG)canvas_p1.y };
    app.mapCanvasValid = true;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Background + border
    draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(40, 40, 40, 255));
    draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255), 0.0f, 0, 1.0f);

    // ==== UI BELOW CANVAS ====

    // Row 1: toggles for mob / drop / portal (one row)
    ImGui::Spacing();


    ImGui::Checkbox("Mobs", &s_showMobs);
    ImGui::SameLine();
    ImGui::Checkbox("Drops", &s_showDrops);
    ImGui::SameLine();
    ImGui::Checkbox("Portals", &s_showPortals);
    ImGui::SameLine();
    ImGui::Checkbox("Others", &s_showOthers);
    ImGui::SameLine();
    ImGui::Checkbox("NPCs", &s_showNpcs);

    // Platform checkbox - toggles other checkboxes
    if (ImGui::Checkbox("Platforms", &s_showPlatforms))
    {
        if (s_showPlatforms)
        {
            // Uncheck all others when platform is checked
            s_showMobs = false;
            s_showDrops = false;
            s_showPortals = false;
            s_showOthers = false;
            s_showNpcs = false;
        }
        else
        {
            // Re-check all others when platform is unchecked
            s_showMobs = true;
            s_showDrops = true;
            s_showPortals = true;
            s_showOthers = true;
            s_showNpcs = true;
        }
    }
    ImGui::SameLine();
    ImGui::Checkbox("Trajectory", &s_showTrajectory);
    ImGui::SameLine();
    ImGui::Checkbox("Connection", &s_showConnections);
    ImGui::SameLine();
    ImGui::Checkbox("Path", &s_showPath);
    app.bufferPath = s_showPath;



    // Row 2: mode button + zoom slider
    if (ImGui::Button(kModeNames[s_viewMode]))
    {
        s_viewMode = (s_viewMode + 1) % 3; // cycle Fit -> Zoom -> Center -> Fit
    }
    ImGui::SameLine(); ImGui::Dummy(ImVec2(10, 0)); ImGui::SameLine();
    ImGui::SetNextItemWidth(200.0f);
    ImGui::SliderFloat("Zoom-In", &s_zoomFactor, 1.0f, 6.0f, "x %.1f");

    // (Removed: mapID and bounds text)
    // ImGui::Text("MapID:%ld", mapID);
    // ImGui::Text("Bounds: L=%ld R=%ld T=%ld B=%ld  (W=%.0f H=%.0f)", ...);

    // ==== MAP DRAWING ====

    // Inner region with a small margin
    const float margin = 5.0f;
    ImVec2 inner_p0 = ImVec2(canvas_p0.x + margin, canvas_p0.y + margin);
    ImVec2 inner_p1 = ImVec2(canvas_p1.x - margin, canvas_p1.y - margin);

    // enable clipping
    draw_list->PushClipRect(inner_p0, inner_p1, true);

    const float innerW = inner_p1.x - inner_p0.x;
    const float innerH = inner_p1.y - inner_p0.y;

    // Try to get player position early
    PhysicsParams::CVecCtrl CVecCtrl;
    bool havePlayer = CUserLocal::GetVecCtrl(app.hProcess, CVecCtrl);

    float sx = innerW / worldW;
    float sy = innerH / worldH;
    bool mapIsTall = (worldH > worldW);

    float scale = 1.0f;
    ImVec2 mapOrigin(0.0f, 0.0f);

    if (s_viewMode == 0)
    {
        // Fit
        scale = (sx < sy) ? sx : sy;
        float usedW = worldW * scale;
        float usedH = worldH * scale;

        mapOrigin.x = inner_p0.x + (innerW - usedW) * 0.5f;
        mapOrigin.y = inner_p0.y + (innerH - usedH) * 0.5f;
    }
    else if (s_viewMode == 1)
    {
        // Zoom: fit width/height, center player on the other axis
        if (mapIsTall)
        {
            // Fit to width, center Y on player
            scale = sx;
            mapOrigin.x = inner_p0.x;

            if (havePlayer)
            {
                float centerY = 0.5f * (inner_p0.y + inner_p1.y);
                mapOrigin.y = centerY - (float(CVecCtrl.Y) - float(top)) * scale;  // Fixed: CVecCtrl.Y not X
            }
            else
            {
                float usedH = worldH * scale;
                mapOrigin.y = inner_p0.y + (innerH - usedH) * 0.5f;
            }
        }
        else
        {
            // Fit to height, center X on player
            scale = sy;
            mapOrigin.y = inner_p0.y;

            if (havePlayer)
            {
                float centerX = 0.5f * (inner_p0.x + inner_p1.x);
                mapOrigin.x = centerX - (float(CVecCtrl.X) - float(left)) * scale;
            }
            else
            {
                float usedW = worldW * scale;
                mapOrigin.x = inner_p0.x + (innerW - usedW) * 0.5f;
            }
        }
    }
    else
    {
        // Center: like Zoom but center on player in both X/Y
        if (mapIsTall)
            scale = sx;
        else
            scale = sy;

        if (havePlayer)
        {
            ImVec2 center(
                0.5f * (inner_p0.x + inner_p1.x),
                0.5f * (inner_p0.y + inner_p1.y)
            );

            mapOrigin.x = center.x - (float(CVecCtrl.X) - float(left)) * scale;
            mapOrigin.y = center.y - (float(CVecCtrl.Y) - float(top))  * scale;
        }
        else
        {
            if (mapIsTall)
            {
                float usedH = worldH * scale;
                mapOrigin.x = inner_p0.x;
                mapOrigin.y = inner_p0.y + (innerH - usedH) * 0.5f;
            }
            else
            {
                float usedW = worldW * scale;
                mapOrigin.y = inner_p0.y;
                mapOrigin.x = inner_p0.x + (innerW - usedW) * 0.5f;
            }
        }
    }

    // Extra zoom around center of canvas
    if (s_zoomFactor != 1.0f)
    {
        ImVec2 center(
            0.5f * (inner_p0.x + inner_p1.x),
            0.5f * (inner_p0.y + inner_p1.y)
        );

        float worldCx = float(left) + (center.x - mapOrigin.x) / scale;
        float worldCy = float(top)  + (center.y - mapOrigin.y)  / scale;

        scale *= s_zoomFactor;

        mapOrigin.x = center.x - (worldCx - float(left)) * scale;
        mapOrigin.y = center.y - (worldCy - float(top))  * scale;
    }

    auto WorldToScreen = [&](float wx, float wy) -> ImVec2
    {
        float sx_ = mapOrigin.x + (wx - float(left)) * scale;
        float sy_ = mapOrigin.y + (wy - float(top))  * scale;
        return ImVec2(sx_, sy_);
    };

    auto ScreenToWorld = [&](float sx_, float sy_) -> ImVec2
    {
        float wx = float(left) + (sx_ - mapOrigin.x) / scale;
        float wy = float(top)  + (sy_ - mapOrigin.y) / scale;
        return ImVec2(wx, wy);
    };


    // Map bounding box
    ImVec2 bb_min = WorldToScreen(float(left),  float(top));
    ImVec2 bb_max = WorldToScreen(float(right), float(bottom));
    draw_list->AddRect(bb_min, bb_max, IM_COL32(80, 80, 80, 255));

    // --- Platforms (from MapleMap class) ---
    if (s_showPlatforms && MapleMap::NumPlatforms > 0)
    {
        ImFont* font = ImGui::GetFont();
        float zoomForText = s_zoomFactor;
        if (zoomForText > 2.0f)  zoomForText = 2.0f;
        float fontSize = 5.0f+10.0f * zoomForText;

        for (const auto& plat : MapleMap::Platforms)
        {
            ImU32 platColor = GetPlatformColor(plat.id - 1);

            if (plat.IsLadderRope())
            {
                // Draw ladder/rope platform
                const auto* lr = plat.p_LadderRope;
                if (!lr) continue;

                ImVec2 pTop    = WorldToScreen((float)lr->x, (float)lr->y1);
                ImVec2 pBottom = WorldToScreen((float)lr->x, (float)lr->y2);

                AddDashedLine(
                    draw_list,
                    pTop,
                    pBottom,
                    platColor,
                    lr->isLadder ? 3.0f : 2.0f,
                    lr->isLadder ? 6.0f : 4.0f,
                    lr->isLadder ? 3.0f : 2.0f
                );

                // Label at center
                float centerY = (lr->y1 + lr->y2) * 0.5f;
                ImVec2 labelPos = WorldToScreen((float)lr->x, centerY);

                char label[16];
                snprintf(label, sizeof(label), "%d", plat.id);
                ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, label);

                ImVec2 textPos(labelPos.x - textSize.x * 0.5f, labelPos.y - textSize.y * 0.5f);
                draw_list->AddText(font, fontSize, textPos, IM_COL32(255, 255, 255, 255), label);
            }
            else
            {
                // Draw foothold platform
                MapStructures::Foothold* current = plat.p_Begin;
                while (current)
                {
                    ImVec2 p0 = WorldToScreen((float)current->x1, (float)current->y1);
                    ImVec2 p1 = WorldToScreen((float)current->x2, (float)current->y2);
                    draw_list->AddLine(p0, p1, platColor, 2.5f);

                    current = current->p_Next;
                }

                // Label at center of platform
                float centerX = (plat.leftX + plat.rightX) * 0.5f;
                
                // Find Y at center by interpolating on the foothold that contains centerX
                float centerY = 0.0f;
                current = plat.p_Begin;
                while (current)
                {
                    if (centerX >= current->x1 && centerX <= current->x2)
                    {
                        centerY = (float)current->GetYAtX((int)centerX);
                        break;
                    }
                    current = current->p_Next;
                }

                ImVec2 labelPos = WorldToScreen(centerX, centerY);

                char label[16];
                snprintf(label, sizeof(label), "%d", plat.id);
                ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, label);

                // Position label above the platform
                ImVec2 textPos(labelPos.x - textSize.x * 0.5f, labelPos.y - textSize.y/1.5f);
                draw_list->AddText(font, fontSize, textPos, IM_COL32(255, 255, 255, 255), label);
            }
        }
    }
    else
    {
        // --- Footholds (from memory) ---
        CWvsPhysicalSpace2D::ForEachFoothold(
            app.hProcess,
            [&](const CStaticFoothold& fh) -> bool
            {
                long x1 = 0, y1 = 0;
                long x2 = 0, y2 = 0;
                long id = 0, pr = 0, ne = 0, layer=0;

                if (!fh.GetFoothold(app.hProcess, x1, y1, x2, y2, id, pr, ne, layer))
                    return true;

                ImVec2 p0 = WorldToScreen((float)x1, (float)y1);
                ImVec2 p1 = WorldToScreen((float)x2, (float)y2);

                draw_list->AddLine(p0, p1, fhColor, 2.0f);
                
                draw_list->AddCircleFilled(p0, 1.0f+(s_zoomFactor/4.0f*1.5f),   IM_COL32(0, 50, 80, 255), 0);
                draw_list->AddCircleFilled(p1, 1.0f+(s_zoomFactor/4.0f*1.5f),   IM_COL32(0, 50, 80, 255), 0);


                return true;
            }
        );

        // --- Ladders/Ropes (from memory) ---
        CWvsPhysicalSpace2D::ForEachLadderRope(
            app.hProcess,
            [&](const CLadderRope& lr) -> bool
            {
                long id = 0, isLadder = 0, fromUpper = 0;
                long x = 0, y1 = 0, y2 = 0;

                if (!lr.Get(app.hProcess, id, isLadder, fromUpper, x, y1, y2))
                    return true;

                // Skip bogus entry you observed (id == 0)
                if (id <= 0)
                    return true;

                // Ensure y1 is the top and y2 is bottom (top < bottom)
                if (y1 > y2)
                    std::swap(y1, y2);

                if (x < left || x > right)
                    return true;

                // vertical span must intersect [top, bottom]
                if (y2 < top || y1 > bottom)
                    return true;

                ImVec2 pTop    = WorldToScreen((float)x, (float)y1);
                ImVec2 pBottom = WorldToScreen((float)x, (float)y2);

                ImU32 col = isLadder
                    ? ldColor  // ladder = cyan
                    : rpColor; // rope   = yellow

                AddDashedLine(
                    draw_list,
                    pTop,
                    pBottom,
                    col,
                    isLadder ? 3.0f : 2.0f,   // thickness
                    isLadder ? 6.0f : 4.0f,   // dash length
                    isLadder ? 3.0f : 2.0f    // gap length
                );
                draw_list->AddCircleFilled(pTop   , 1.0f+(s_zoomFactor/4.0f*1.5f),  IM_COL32(0, 50, 80, 255), 0);
                draw_list->AddCircleFilled(pBottom, 1.0f+(s_zoomFactor/4.0f*1.5f),  IM_COL32(0, 50, 80, 255), 0);

                return true;
            }
        );
    }

    std::unordered_map<uint32_t, MobCountInfo> mobCounts;

    // --- Mobs ---
    {
        CMobPool::ForEachMob(
            app.hProcess,
            [&](const CMob& mob) -> bool
            {
                long x = 0, y = 0;
                long px = 0, py = 0;

                if (mob.Get(app.hProcess, x, y, px, py) && s_showMobs)
                {
                    ImVec2 p = WorldToScreen((float)x, (float)y);
                    const float sz = 6.0f * s_zoomFactor;
                    ImVec2 a(p.x - sz * 0.5f, p.y - sz);
                    ImVec2 b(p.x + sz * 0.5f, p.y);
                    // 

                    std::string mobName;
                    uint32_t tid = 0;
                    mob.GetTemplateId(app.hProcess, tid);

                    //invisible mob?
                    if(!mob.GetTemplateName(app.hProcess, mobName) || tid==9999999  )
                        draw_list->AddRectFilled(a, b, dmColor);
                    else
                        draw_list->AddRectFilled(a, b, mbColor);
                }

                uint32_t tid = 0;
                if (mob.GetTemplateId(app.hProcess, tid))
                {
                    auto& info = mobCounts[tid];
                    info.count++;

                    // Fill name once per ID
                    if (info.name.empty())
                    {
                        std::string mobName;
                        if (mob.GetTemplateName(app.hProcess, mobName))
                        {
                            info.name = std::move(mobName);
                        }
                        else
                        {
                            info.name = "<unknown>";
                        }
                    }
                }


                return true;
            }
        );
    }

    // --- Drops ---
    if (s_showDrops)
    {
        CDropPool::ForEachDrop(
            app.hProcess,
            [&](const CDrop& drop) -> bool
            {
                uint32_t UID = 0;
                long x = 0, y = 0;
                uint32_t OwnerID = 0, SourceID = 0, OwnType = 0, IsMeso = 0, ID = 0;
                std::string name;

                if (drop.Get(app.hProcess, UID, OwnerID, SourceID, OwnType, IsMeso, ID, x, y, name))
                {
                    const float sz = 2.5f * s_zoomFactor;
                    ImVec2 p = WorldToScreen((float)x, (float)y - 2.5f);
                    draw_list->AddCircleFilled(p, sz, IM_COL32(255, 255, 255, 255), 0);

                    if (!IsMeso)
                        draw_list->AddCircle(p, sz, dpColor, 0, sz / 1.5f);
                    else
                        draw_list->AddCircle(p, sz, IM_COL32(252, 163, 17, 255), 0, sz / 1.5f);
                }
                return true;
            }
        );
    }

    // --- Portals ---
    if (s_showPortals)
    {
        CWvsPhysicalSpace2D::ForEachPortal(
            app.hProcess,
            [&](const CPortal& portal) -> bool
            {
                long x, y, ID, Type;
                uint32_t toMapID;
                std::string Name, toName;

                if (!portal.Get(app.hProcess, x, y, ID, Type, toMapID, Name, toName))
                    return true;

                ImVec2 base = WorldToScreen((float)x, (float)y);

                const double t = ImGui::GetTime();
                const float  pulse = 0.3f + 0.7f * (0.5f * (static_cast<float>(std::sin(t * 2.5)) + 1.0f));
                const int    alpha = (int)(255.0f * pulse);

                if (Type == 1 || Type == 5 || Type == 8) // invisible
                {
                    const float triW = 7.0f * s_zoomFactor;
                    const float triH = triW / 1.7f * 2.0f;

                    ImVec2 t_baseL(base.x - triW * 0.5f, base.y);
                    ImVec2 t_baseR(base.x + triW * 0.5f, base.y);
                    ImVec2 t_tip  (base.x,               base.y - triH);

                    ImU32 col = (ptColor3 & 0x00FFFFFF) | (alpha << 24);
                    draw_list->AddTriangle(t_baseL, t_baseR, t_tip, col, 2.0f);
                }
                else
                {
                    const float r = 5.0f * s_zoomFactor;
                    ImVec2 center(base.x, base.y - r);

                    ImU32 col = (ptColor1 & 0x00FFFFFF) | (alpha << 24);  // others
                    if (Type == 10 || Type == 11) // hidden
                        col = (ptColor2 & 0x00FFFFFF) | (alpha << 24);

                    draw_list->AddCircle(center, r,        col, 0, 2.0f);
                    draw_list->AddCircle(center, r * 0.6f, col, 0, 1.5f);
                }

                return true;
            }
        );
    }

    //--- user Remote  ---
    if(s_showOthers)
    {

        std::vector<uint32_t> party_ids;
        CUserPool::getPartyIDs(app.hProcess, party_ids);

        CUserPool::ForEachUser(
                    app.hProcess,
                    [&](const CUserRemote& user) -> bool
                    {
                        
                        long x,y;
                        std::string Name;
                        uint32_t ID, Job;
                        user.Get(app.hProcess,ID,Job,x,y,Name);


                        ImVec2 p = WorldToScreen((float)x, (float)y);
                        ImVec2 a(p.x - 3 * s_zoomFactor, p.y - 10 * s_zoomFactor);
                        ImVec2 b(p.x + 3 * s_zoomFactor, p.y);

                        if(std::find(party_ids.begin(), party_ids.end(), ID) != party_ids.end())
                            draw_list->AddRectFilled(a, b, pyColor);
                        else
                            draw_list->AddRectFilled(a, b, ruColor);


                            // --- Name label ---
                            if (!Name.empty())
                            {
                                ImFont* font = ImGui::GetFont();

                                // scale font with zoom, but clamp
                                float zoomForText = s_zoomFactor;
                                if (zoomForText < 0.75f) zoomForText = 0.75f;
                                if (zoomForText > 2.0f)  zoomForText = 2.0f;

                                float baseSize = 10.f;
                                float fontSize = baseSize * zoomForText;

                                // measure text at this size
                                ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, Name.c_str());

                                // left-aligned to rect's left, above the rect
                                ImVec2 textPos;
                                textPos.x = a.x;
                                textPos.y = a.y - textSize.y - 2.0f;   // a little padding above

                                draw_list->AddText(
                                    font,
                                    fontSize,
                                    textPos,
                                    IM_COL32(255, 255, 255, 255),
                                    Name.c_str()
                                );
                            }


                        return true;
                    }
                );
    }

    //--- NPCs ---
    if(s_showNpcs)
    {
        CNpcPool::ForEachNpc(
            app.hProcess,
            [&](const CNpc& npc) -> bool
            {
                long x, y, xp, yp;
                if (!npc.Get(app.hProcess, x, y, xp, yp))
                    return true;

                ImVec2 p = WorldToScreen((float)x, (float)y);
                ImVec2 a(p.x - 3 * s_zoomFactor, p.y - 10 * s_zoomFactor);
                ImVec2 b(p.x + 3 * s_zoomFactor, p.y);
                draw_list->AddRectFilled(a, b, npcColor);

                std::string name;
                if (npc.GetTemplateName(app.hProcess, name) && !name.empty())
                {
                    ImFont* font = ImGui::GetFont();
                    float zoomForText = s_zoomFactor;
                    if (zoomForText < 0.75f) zoomForText = 0.75f;
                    if (zoomForText > 2.0f)  zoomForText = 2.0f;

                    float fontSize = 10.f * zoomForText;
                    ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, name.c_str());

                    ImVec2 textPos;
                    textPos.x = a.x;
                    textPos.y = a.y - textSize.y - 2.0f;

                    draw_list->AddText(font, fontSize, textPos,
                        IM_COL32(255, 255, 255, 255), name.c_str());
                }

                return true;
            }
        );
    }

    // --- Player ---
    if (havePlayer)
    {
        ImVec2 p = WorldToScreen((float) CVecCtrl.X, (float)CVecCtrl.Y);
        ImVec2 a(p.x - 3 * s_zoomFactor, p.y - 10 * s_zoomFactor);
        ImVec2 b(p.x + 3 * s_zoomFactor, p.y);
        draw_list->AddRectFilled(a, b, plColor);

        if(CVecCtrl.OnFootholdAddress)
        {
            CStaticFoothold fh(CVecCtrl.OnFootholdAddress);
            long x1 = 0, y1 = 0;
            long x2 = 0, y2 = 0;
            long id, pr, ne, layer;
            if (fh.GetFoothold(app.hProcess, x1, y1, x2, y2, id, pr, ne, layer))
            {
                ImVec2 p0 = WorldToScreen((float)x1, (float)y1);
                ImVec2 p1 = WorldToScreen((float)x2, (float)y2);
                draw_list->AddLine(p0, p1, plColor, 2.0f);
            }
        }


        if(s_showTrajectory)
        {
            ReachabilityComputer::Reach Reach;
            double yCap = static_cast<double>(bottom);
            if(MapleMap::ComputeInstantReach(Reach))
            {

                for(int input =0; input<ReachabilityComputer::kNumCases; input++)
                {
                    auto& Tr = Reach.Get(input);
                    DrawTrajectoryToYCap(draw_list, Tr,  yCap, WorldToScreen, 20.0, IM_COL32(0, 255-20*input, 0, 255), 2.0f); 
                }

            }

            if(MapleMap::ComputeInstantJumpReach(Reach))
            {
                for(int input =0; input<ReachabilityComputer::kNumCases; input++)
                {
                    auto& Tr = Reach.Get(input);
                    DrawTrajectoryToYCap(draw_list, Tr, yCap, WorldToScreen, 20.0, IM_COL32(255-20*input, 0, 0, 255), 2.0f);
                }

            }
        }

        //connections
        if(s_showConnections)
        {
            for (auto&pf : MapleMap::Platforms)
            {
                for(auto&move: pf.Connections)
                {
                    ImVec2 p0 = WorldToScreen((float)move->srcX, (float)move->srcY);
                    ImVec2 p1 = WorldToScreen((float)move->dstX, (float)move->dstY);
                    // if(move->dstY < move->srcY) draw_list->AddLine(p0, p1,  IM_COL32(255, 165, 171, 15), 4.0f);
                    draw_list->AddLine(p0, p1,  move->dstY> move->srcY? IM_COL32(173, 193, 120, 15): IM_COL32(255, 165, 171, 15), 4.0f);

                }

            }

        }

        // --- Current Path ---
        if (s_showPath)
        {
            const auto* snap = PathFinding::g_PathRenderBuffer.GetSnapshot();
            if (snap && snap->valid)
            {
                const ImU32 walkColor = IM_COL32(254, 228, 64, 255);   
                const ImU32 moveColor = IM_COL32(251, 133,  0, 255); 
                
                for (const auto& step : snap->steps)
                {
                    ImVec2 p0 = WorldToScreen((float)step.startX, (float)step.startY);
                    ImVec2 p1 = WorldToScreen((float)step.endX, (float)step.endY);
                    ImVec2 pa = WorldToScreen((float)step.ApeX, (float)step.ApeY);
                    
                    if (step.isWalk)
                    {
                        // Dashed line for walking
                        draw_list->AddLine(p0, p1, walkColor, 3.0f);
                    }
                    else
                    {

                        draw_list->AddLine(p0, p1, moveColor, 3.0f);
                    
                    }
                }

                char buf[128];
                snprintf(buf, sizeof(buf), "Length %.1fs | Cost %.1fms", 
                    MapleMap::CurrentPath.totalCostMs/1000.0,
                    MapleMap::CurrentPath.computeTimeUs/1000.0);
                
                ImVec2 textPos(canvas_p0.x +5, canvas_p0.y + 5);
                draw_list->AddText(textPos, IM_COL32(255, 255, 0, 255), buf);
                auto headInfo = MapleMap::CurrentPath.PathHeadInfo();
                ImVec2 textPos2(canvas_p0.x +5, canvas_p0.y + 19);
                ImGui::SetWindowFontScale(0.9f); 
                draw_list->AddText(textPos2, IM_COL32(255, 255, 0, 255), headInfo.c_str());
                ImGui::SetWindowFontScale(1.0f);

                
            }
        }

        


    }


    // --- Mouse coordinate display ---
    ImVec2 mousePos = ImGui::GetMousePos();
    bool mouseInCanvas = (mousePos.x >= canvas_p0.x && mousePos.x <= canvas_p1.x &&
                        mousePos.y >= canvas_p0.y && mousePos.y <= canvas_p1.y);

    if (mouseInCanvas)
    {
        ImVec2 worldPos = ScreenToWorld(mousePos.x, mousePos.y);
        
        char coordBuf[64];
        snprintf(coordBuf, sizeof(coordBuf), "(%.0f, %.0f)\nclick to copy", worldPos.x, worldPos.y);
        
        ImVec2 textPos(mousePos.x + 15, mousePos.y);  // Offset from cursor
        draw_list->AddText(textPos, IM_COL32(255, 255, 255, 255), coordBuf);

        // Copy to clipboard on click
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            char clipBuf[64];
            snprintf(clipBuf, sizeof(clipBuf), "%i, %i", (int)worldPos.x, (int)worldPos.y);
            ImGui::SetClipboardText(clipBuf);
        }
    }

    // --- Arrow key states display ---
    {
        bool left = false, up = false, right = false, down = false;
        // app.arrowHook.GetArrowStates(left, up, right, down); // Stripped

        const float size = 10.0f;
        const float gap = 2.0f;
        const float margin = 10.0f;

        float cx = canvas_p1.x - margin - size * 2.0f;
        float cy = canvas_p1.y - margin - size * 0.5f;

        ImU32 colOff = IM_COL32(128, 128, 128, 88);
        ImU32 colOn  = IM_COL32(255, 165, 0, 150);

        auto drawArrow = [&](float ox, float oy, int dir, bool on) {
            ImU32 col = on ? colOn : colOff;
            float x = cx + ox;
            float y = cy + oy;
            
            // dir: 0=up, 1=down, 2=left, 3=right
            if (dir == 0) { // Up
                draw_list->AddRect({x, y}, {x + size, y-size}, colOff);
                draw_list->AddTriangleFilled({x+0.5f*size, y - 0.8f*size}, {x + size*0.2f, y - size*0.2f}, {x + size*0.8f, y - size*0.2f}, col);
            } else if (dir == 1) { // Down
                draw_list->AddRect({x, y}, {x + size, y-size}, colOff);
                draw_list->AddTriangleFilled({x + size*0.2f, y - size*0.8f}, {x+0.5f*size, y - 0.2f*size}, {x + size*0.8f, y - size*0.8f}, col);

            } else if (dir == 2) { // Left
                draw_list->AddRect({x, y}, {x + size, y-size}, colOff);
                draw_list->AddTriangleFilled({x + 0.2f*size, y-0.5f*size}, {x+size*0.8f, y - size*0.2f}, {x+size*0.8f, y - size*0.8f}, col);
            } else { // Right
                draw_list->AddRect({x, y}, {x + size, y-size}, colOff);
                draw_list->AddTriangleFilled({x + 0.2f*size, y-0.8f*size}, {x+size*0.2f, y - size*0.2f}, {x+size*0.8f, y - size*0.5f}, col);

            }
        };

        drawArrow(0, -(size + gap), 0, up);      // Up
        drawArrow(0, + gap, 1, down);       // Down
        drawArrow(-(size + gap), gap, 2, left);    // Left
        drawArrow(size + gap, gap, 3, right);      // Right
    }



    // Disable clipping
    draw_list->PopClipRect();

    // --- mob & player summary panel below everything ---
    ImGui::Separator();

    float halfWidth  = ImGui::GetContentRegionAvail().x * 0.5f;
    float panelHeight = 160.0f;  // choose as needed

    ImGui::BeginChild("MobPanel", ImVec2(halfWidth, panelHeight), true,
                    ImGuiWindowFlags_HorizontalScrollbar);
    {
        ImGui::SeparatorText("Mob counts");
        for (const auto& kv : mobCounts)
        {
            uint32_t id = kv.first;
            const MobCountInfo& info = kv.second;
            ImGui::BulletText("%s [%u]: %d", info.name.c_str(), id, info.count);
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("UserPanel", ImVec2(halfWidth, panelHeight), true,
                    ImGuiWindowFlags_HorizontalScrollbar);
    {
        ImGui::SeparatorText("Players");
        CUserPool::ForEachUser(
            app.hProcess,
            [&](const CUserRemote& user) -> bool
            {
                long x, y;
                std::string Name;
                uint32_t ID, Job;
                user.Get(app.hProcess, ID, Job, x, y, Name);

                std::string jobStr = MapleJob::GetJobName(Job);
                ImGui::BulletText("%s [%u][%s]", Name.c_str(), ID, jobStr.c_str());
                ImGui::Text("   (%ld,%ld)", x, y);

                return true;
            }
        );
    }
    ImGui::EndChild();



}