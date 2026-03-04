#include <UI/TabInventory.h>
#include <imgui/imgui.h>
#include <class/MapleClass.h>
#include <array>
#include <string>

// Names for top-level inventory tabs
static const char* kInvTabNames[5] = { "Equip", "Use", "Setup", "Etc", "Cash" };

// Maple's inventory type values (1..5)
static const InventoryType kInvTypes[5] = {
    static_cast<InventoryType>(1), // Equip
    static_cast<InventoryType>(2), // Use
    static_cast<InventoryType>(3), // Setup
    static_cast<InventoryType>(4), // Etc
    static_cast<InventoryType>(5)  // Cash
};


static void DrawMapleDescFormatted(const std::string& desc)
{
    if (desc.empty())
        return;

    ImGui::PushTextWrapPos(0.0f);

    const char* s = desc.c_str();
    const char* segStart = s;
    bool firstOnLine = true;

    auto emitSegment = [&](const char* start, const char* end, ImU32 colOverride = 0xFFFFFFFF)
    {
        if (end <= start) return;

        if (!firstOnLine)
            ImGui::SameLine(0.0f, 0.0f);    // continue on same line

        if (colOverride != 0xFFFFFFFF)
            ImGui::PushStyleColor(ImGuiCol_Text, colOverride);

        ImGui::TextUnformatted(start, end);

        if (colOverride != 0xFFFFFFFF)
            ImGui::PopStyleColor();

        firstOnLine = false;
    };

    while (*s)
    {
        // Treat literal "\n" as newline
        if (s[0] == '\\' && s[1] == 'n')
        {
            emitSegment(segStart, s);
            // ImGui::NewLine();
            firstOnLine = true;
            s += 2;
            segStart = s;
            continue;
        }

        // Actual newline char
        if (*s == '\n')
        {
            emitSegment(segStart, s);
            // ImGui::NewLine();
            firstOnLine = true;
            ++s;
            segStart = s;
            continue;
        }

        // #c ... #  → orange colored text
        if (s[0] == '#' && s[1] == 'c')
        {
            // flush any plain text before #c
            emitSegment(segStart, s);

            const char* colorStart = s + 2;
            const char* endHash = std::strchr(colorStart, '#');
            if (!endHash)
            {
                // no closing '#', treat the rest as plain text
                segStart = s;
                break;
            }

            // orange text for the enclosed part
            emitSegment(colorStart, endHash,IM_COL32(255, 136, 17, 255));

            s = endHash + 1;
            segStart = s;
            continue;
        }

        ++s;
    }

    // tail
    emitSegment(segStart, s);

    ImGui::PopTextWrapPos();
}


void DrawInventoryTab(AppState& app)
{
    ImGui::SeparatorText("Inventory");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Click cell to copy item ID");

    if (!app.hProcess)
    {
        ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "No process attached");
        return;
    }

    // ========= Inventory category tabs (Equip / Use / Setup / Etc / Cash) =========
    if (ImGui::BeginTabBar("InvCategories"))
    {
        for (int tabIndex = 0; tabIndex < 5; ++tabIndex)
        {
            if (ImGui::BeginTabItem(kInvTabNames[tabIndex]))
            {
                if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Click cell to copy item ID");
                // ---------- Everything below is "one tab" view ----------

                // ---------- Geometry: 8x12 grid in ~400px width ----------
                constexpr float canvasWidth  = 390.0f;
                constexpr int   COLS         = 8;
                constexpr int   ROWS         = 12;

                // Base small spacing + extra gap between quadrants
                const float baseSpacing   = 2.0f;   // normal gap
                const float quadGapExtra  = 8.0f;   // extra gap between 4x6 blocks
                const float margin        = 5.0f;

                // X: 8 columns -> 7 inner gaps + 2 edge gaps, plus one quadGapExtra
                const float innerW        = canvasWidth - 2.0f * margin;
                const float totalSpacingX = (COLS + 1) * baseSpacing + quadGapExtra;
                const float cellW         = (innerW - totalSpacingX) / COLS;

                // Y: 12 rows -> 11 inner gaps + 2 edge gaps, plus one quadGapExtra
                const float totalSpacingY = (ROWS + 1) * baseSpacing + quadGapExtra;
                const float cellH         = cellW;  // square cells

                const float innerH        = ROWS * cellH + totalSpacingY;
                const float canvasH       = innerH + 2.0f * margin;

                const ImVec2 canvasSize(canvasWidth, canvasH);

                ImGui::InvisibleButton("InvCanvas", canvasSize);
                ImVec2 canvas_p0 = ImGui::GetItemRectMin();
                ImVec2 canvas_p1 = ImGui::GetItemRectMax();

                
                // Capture click state NOW, before anything else consumes it
                bool canvasClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
                ImVec2 clickPos = ImGui::GetIO().MousePos;


                ImDrawList* draw_list = ImGui::GetWindowDrawList();

                // Background + border
                draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(30, 30, 30, 255));
                draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));

                ImVec2 inner_p0(canvas_p0.x + margin, canvas_p0.y + margin);
                ImVec2 inner_p1(canvas_p1.x - margin, canvas_p1.y - margin);

                draw_list->PushClipRect(inner_p0, inner_p1, true);

                // ---------- Cell data ----------
                struct CellInfo
                {
                    bool        used  = false;
                    InventoryType type{};
                    size_t      slot  = 0;

                    uint32_t    id    = 0;
                    uint16_t    qty   = 0;

                    std::string name;
                    std::string desc;

                    EquipStats  stats{};
                    bool        hasEquipStats = false;
                };

                std::array<CellInfo, COLS * ROWS> cells{};

                InventoryType invType = kInvTypes[tabIndex];

                // Fill cells from game inventory
                size_t visitedCount = CUserLocal::ForEachInventoryItem(
                    app.hProcess,
                    invType,
                    [&cells, &app](const GW_ItemSlot& Item) -> bool
                    {
                        uint32_t    ID  = 0;
                        uint16_t    Qty = 0;
                        std::string name;
                        std::string desc;

                        Item.Get(app.hProcess, ID, Qty, name, desc);

                        int idx = static_cast<int>(Item.slot) - 1;  // 1..96 -> 0..95
                        if (idx < 0 || idx >= static_cast<int>(cells.size()))
                            return true;

                        //map to quadrant
                        int qd = idx/24;
                        int qd_row = int(idx%24) /4;
                        int qd_col = int(idx%24) %4;

                        idx = (qd_col + 4*int(qd%2)) + (6*int(qd/2)+qd_row)*8;

                        auto& cell = cells[idx];
                        cell.used = true;
                        cell.type = Item.type;
                        cell.slot = Item.slot;
                        cell.id   = ID;
                        cell.qty  = Qty;
                        cell.name = std::move(name);
                        cell.desc = std::move(desc);

                        EquipStats stats{};
                        if (Item.GetEquipStats(app.hProcess, stats))
                        {
                            cell.stats         = stats;
                            cell.hasEquipStats = true;
                        }
                        else
                        {
                            cell.hasEquipStats = false;
                        }

                        return true; // continue
                    });

                // Helper: brutal fit-to-width for text
                auto FitTextToWidth = [](const std::string& src, float maxWidth) -> std::string
                {
                    if (src.empty())
                        return src;
                    std::string s = src;
                    while (!s.empty())
                    {
                        ImVec2 sz = ImGui::CalcTextSize(s.c_str());
                        if (sz.x <= maxWidth)
                            break;
                        s.pop_back();
                    }
                    return s;
                };

                // ---------- Quadrant separator lines ----------
                // We know quadrants are 4x6 (cols x rows).
                CellInfo* hoveredCell = nullptr;

                // Vertical separator
                {
                    float xCursor  = inner_p0.x + baseSpacing;
                    float xVertLine = 0.0f;

                    for (int col = 0; col < COLS; ++col)
                    {
                        float x0 = xCursor;
                        float x1 = x0 + cellW;

                        if (col == 3)  // between col 3 and 4
                        {
                            float gapStart = x1 + baseSpacing;
                            float gapEnd   = gapStart + quadGapExtra;
                            xVertLine = 0.5f * (gapStart + gapEnd);
                        }

                        xCursor = x1 + baseSpacing;
                        if (col == 3)
                            xCursor += quadGapExtra;
                    }

                    if (xVertLine > 0.0f)
                    {
                        const float thickness = 2.0f;
                        ImU32 colLine = IM_COL32(255, 215, 0, 155);
                        draw_list->AddLine(
                            ImVec2(xVertLine - 1, inner_p0.y),
                            ImVec2(xVertLine - 1, inner_p1.y),
                            colLine,
                            thickness
                        );
                    }
                }

                // Horizontal separator
                {
                    float yCursor   = inner_p0.y + baseSpacing;
                    float yHorzLine = 0.0f;

                    for (int row = 0; row < ROWS; ++row)
                    {
                        float y0 = yCursor;
                        float y1 = y0 + cellH;

                        if (row == 5) // between row 5 and 6
                        {
                            float gapStart = y1 + baseSpacing;
                            float gapEnd   = gapStart + quadGapExtra;
                            yHorzLine = 0.5f * (gapStart + gapEnd);
                        }

                        yCursor = y1 + baseSpacing;
                        if (row == 5)
                            yCursor += quadGapExtra;
                    }

                    if (yHorzLine > 0.0f)
                    {
                        const float thickness = 2.0f;
                        ImU32 colLine = IM_COL32(255, 215, 0, 155);
                        draw_list->AddLine(
                            ImVec2(inner_p0.x, yHorzLine - 1),
                            ImVec2(inner_p1.x, yHorzLine - 1),
                            colLine,
                            thickness
                        );
                    }
                }

                // ---------- Draw cells ----------
                {
                    float yCursor = inner_p0.y + baseSpacing;

                    for (int row = 0; row < ROWS; ++row)
                    {
                        float xCursor = inner_p0.x + baseSpacing;

                        for (int col = 0; col < COLS; ++col)
                        {
                            int idx = row * COLS + col;
                            CellInfo& cell = cells[idx];

                            float x0 = xCursor;
                            float y0 = yCursor;
                            ImVec2 cellMin(x0,         y0);
                            ImVec2 cellMax(x0 + cellW, y0 + cellH);

                            if (canvasClicked && cell.used) {
                                if (clickPos.x >= cellMin.x && clickPos.x <= cellMax.x &&
                                    clickPos.y >= cellMin.y && clickPos.y <= cellMax.y)
                                {
                                        char idBuf[16];
                                        snprintf(idBuf, sizeof(idBuf), "%u", cell.id);
                                        ImGui::SetClipboardText(idBuf);
                                }
                            }

                            ImU32 bgCol = cell.used
                                ? IM_COL32(50, 50, 70, 255)
                                : IM_COL32(20, 20, 30, 255);

                            draw_list->AddRectFilled(cellMin, cellMax, bgCol);
                            draw_list->AddRect(cellMin, cellMax, IM_COL32(160, 160, 200, 255));
                            //locked slot

                            int qd_col = idx % 8;
                            int qd_row = idx / 8;

                            int qd = (qd_row / 6) * 2 + (qd_col / 4);

                            int lrow = qd_row % 6;
                            int lcol = qd_col % 4;

                            int idx2 = lrow * 4 + lcol + qd * 24;


                            if (idx2 >= (int)visitedCount)
                            {
                                // Black cell background
                                draw_list->AddRectFilled(cellMin, cellMax, IM_COL32(0, 0, 0, 200));
                                draw_list->AddRect(cellMin, cellMax, IM_COL32(160, 160, 200, 255));


                                const float w = cellMax.x - cellMin.x;
                                const float h = cellMax.y - cellMin.y;
                                ImVec2 center(cellMin.x + w * 0.5f, cellMin.y + h * 0.5f);

                                // Colors
                                ImU32 lockColor = IM_COL32(220, 220, 220, 155);    // light gray/white
                                ImU32 outlineColor = IM_COL32(180, 180, 180, 155); // slightly darker

                                // --- Lock body ---
                                float bodyW = w * 0.4f;
                                float bodyH = h * 0.3f;

                                ImVec2 bodyMin(center.x - bodyW * 0.5f, center.y);
                                ImVec2 bodyMax(center.x + bodyW * 0.5f, center.y + bodyH);

                                draw_list->AddRectFilled(bodyMin, bodyMax, lockColor, 4.0f);
                                draw_list->AddRect(bodyMin, bodyMax, outlineColor, 1.5f, 0, 1.5f);

                                // --- Shackle (simple arc) ---
                                float radius = bodyW * 0.4f;
                                ImVec2 arcCenter(center.x, bodyMin.y-radius/3.0f);

                                draw_list->PathClear();
                                draw_list->PathArcTo(arcCenter, radius, 3.14159f * 0.85f, 3.14159f * 2.15f, 16);
                                draw_list->PathStroke(lockColor, false, 3.0f);

                                // --- Keyhole ---
                                float keyR = bodyW * 0.16f;
                                ImVec2 keyCenter(center.x, center.y + bodyH * 0.3f);
                                draw_list->AddCircleFilled(keyCenter, keyR, IM_COL32(50, 50, 50, 255)); // black keyhole
                                ImVec2 keyCenter2(center.x-keyR/2, center.y + bodyH * 0.3f);
                                ImVec2 keyCenter3(center.x+keyR/2, center.y + bodyH * 0.8f);
                                draw_list->AddRectFilled(keyCenter2, keyCenter3, IM_COL32(50, 50, 50, 255));

                            }


                            if (cell.used)
                            {
                                const float textMargin = 3.0f;
                                float maxTextWidth = cellW - 2.0f * textMargin;

                                std::string fittedName = FitTextToWidth(cell.name, maxTextWidth);
                                ImVec2 namePos(cellMin.x + textMargin, cellMin.y + textMargin);
                                draw_list->AddText(namePos, IM_COL32(230, 230, 230, 255), fittedName.c_str());

                                char qtyBuf[32];
                                _snprintf_s(qtyBuf, _TRUNCATE, "%u", static_cast<unsigned>(cell.qty));
                                ImVec2 qtySize = ImGui::CalcTextSize(qtyBuf);
                                ImVec2 qtyPos(
                                    cellMax.x - qtySize.x - textMargin,
                                    cellMax.y - qtySize.y - textMargin
                                );
                                draw_list->AddText(qtyPos, IM_COL32(200, 255, 200, 255), qtyBuf);

                                // Hover detection for tooltip
                                if (ImGui::IsMouseHoveringRect(cellMin, cellMax) &&
                                    ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
                                {
                                    hoveredCell = &cell;
                                }
                            }

                            // advance x
                            xCursor += cellW + baseSpacing;
                            if (col == 3) // extra gap after col 3
                                xCursor += quadGapExtra;
                        }

                        // advance y
                        yCursor += cellH + baseSpacing;
                        if (row == 5) // extra gap after row 5
                            yCursor += quadGapExtra;
                    }
                }

                draw_list->PopClipRect();

                // ---------- Maple-style tooltip for hovered cell ----------
                if (hoveredCell)
                {
                    ImGui::SetNextWindowBgAlpha(0.8f);
                    ImGui::SetNextWindowSize(ImVec2(260.0f, 0.0f)); 
                    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(14/255.f, 57/255.f, 90/255.f, 0.8f));
                    ImGui::PushStyleColor(ImGuiCol_Border,  ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 6.0f));
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,   ImVec2(4.0f, 2.0f));

                    if (ImGui::BeginTooltip())
                    {
                        // 1) Name – bullet, white, slightly larger
                        ImGui::SetWindowFontScale(1.1f);
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
                        ImGui::BulletText("%s", hoveredCell->name.c_str());
                        // id 
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 136, 17, 255));
                        ImGui::Text("  [%i]",hoveredCell->id);
                        ImGui::PopStyleColor();

                        ImGui::SetWindowFontScale(1.0f);
                        ImGui::PopStyleColor();
                        ImGui::Separator();
                        // 2) Description (if any)
                        if (!hoveredCell->desc.empty())
                        {
                            ImGui::Spacing();
                            DrawMapleDescFormatted(hoveredCell->desc);
                        }
                        ImGui::Separator();
                        // 3) Equip stats: separator + bullet list of non-zero stats
                        if (hoveredCell->hasEquipStats)
                        {
                            bool first = true;
                            hoveredCell->stats.visit(
                                [&](const char* statName, auto value)
                                {
                                    // small red bullet
                                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 136, 17, 255));
                                    ImGui::TextUnformatted("•");
                                    ImGui::PopStyleColor();

                                    ImGui::SameLine();

                                    ImGui::Text("%s: %d", statName, (int)value);
                                },
                                false
                            );
                        }

                        ImGui::EndTooltip();
                    }

                    ImGui::PopStyleVar(2);
                    ImGui::PopStyleColor(2);
                }

                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }
}
