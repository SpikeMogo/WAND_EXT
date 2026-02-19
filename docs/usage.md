# Usage Guide

## UI Tabs

Wand has 6 tabs across the top of the window.

---

### Main Tab

The starting tab. Use it to attach to a game process.

- **Select Maple Window** — opens a dialog listing all visible 32-bit MapleStory windows
- Shows the selected **PID**, **HWND**, and **process name**
- Displays **memory usage** of the Wand process
- Shows **map data loading progress** (WZ parsing happens once on startup)

!!! tip
    If your game window doesn't appear in the list, make sure it's visible (not minimized) and fully loaded past the login screen.

---

### Map Tab

Interactive minimap showing the current game map in real time.

**Entities displayed:**

| Color | Entity |
|---|---|
| Blue lines | Footholds (walkable ground) |
| Brown lines | Ladders and ropes |
| Purple dots | Mobs (with HP %) |
| Green dots | Drops |
| Blue/Orange dots | Portals |
| Green squares | NPCs |
| Yellow crosshair | Your character |
| Yellow dots | Other players |
| Orange dots | Party members |
| Dashed line | Computed path |

**View modes:**

- **Fit** — shows the entire map
- **Zoom** — fit to width or height
- **Center** — follows your character

**Zoom slider:** 1x to 6x magnification

Each entity type can be toggled on/off with checkboxes.

---

### Inventory Tab

Visual 8x12 grid of your inventory.

- **5 sub-tabs:** Equip, Use, Setup, Etc, Cash
- Click any cell to copy its **item ID** to clipboard
- Equipment items show full stats (STR, DEX, INT, LUK, ATT, etc.)
- Item descriptions are displayed with formatting

---

### Settings Tab

Configure all Wand settings. Changes are saved to `bot_settings.ini` automatically.

See the [Setup](setup.md#settings) page for all available settings.

---

### Discord Tab

Discord bot status and controls.

- Connection status indicator
- Toggle **chat log monitoring** (forward in-game chat to Discord)
- **Screenshot** and **map canvas** capture buttons
- See the [Discord](discord.md) page for full setup

---

### WZ Browser Tab

Browse MapleStory's WZ data files. Useful for looking up map IDs, item IDs, mob data, etc.

---

## Running Lua Scripts

1. Go to the **Main** tab
2. Click **Load Script** and select a `.lua` file
3. Click **Run** to start execution
4. Click **Stop** to cancel a running script
5. Script output appears in the log

Scripts run in a separate thread and can be cancelled at any time.

```lua
-- Basic grinding loop
while true do
    local mobs = get_mobs()
    if #mobs > 0 then
        move_to(mobs[1])
        hit_key(0x51)  -- attack key
        sleep(500)
    else
        sleep(100)
    end
end
```

---

## Pathfinding

The pathfinding system computes routes across the map using Dijkstra's algorithm on a platform graph.

### How It Works

1. The map is divided into **platforms** (groups of connected footholds)
2. Platforms are connected by **edges** — walks, jumps, falls, ropes, teleports, and portals
3. `find_path(x, y)` computes the cheapest route from your position to the target
4. `move_to(x, y)` computes and executes the path automatically

### Movement Types

| Type | Description |
|---|---|
| Walk | Horizontal movement along footholds |
| Jump | Physics-based arc between platforms |
| Fall | Gravity drop to lower platform |
| Rope/Ladder | Vertical climb |
| Teleport | Mage teleport skill |
| Portal | Map portal transition |

### Path Return Values

| Value | Meaning |
|---|---|
| `PathReturn.Found` | Path computed successfully |
| `PathReturn.NotFound` | No path exists to target |
| `PathReturn.Float` | Character is in the air |
| `PathReturn.NoTarget` | Target position invalid |
| `PathReturn.Error` | Internal error |

```lua
local result = find_path(100, 200)
if result == PathReturn.Found then
    log_info("Path found!")
    move_to(100, 200)
else
    log_info("No path: " .. tostring(result))
end
```

---

## Background Operation

Wand allows the game to continue running and accepting input while alt-tabbed. This means:

- You can **alt-tab** to other windows while scripts run
- Arrow key input works even when the game is not focused
- Cursor position is managed so click actions work in the background

!!! note
    Some game functions still require the window to be truly focused. If you notice issues, bring the game window to the front briefly.
