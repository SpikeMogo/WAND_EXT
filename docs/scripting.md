# Scripting Guide

This page covers the example scripts bundled with Wand — a complete, modular hunting framework written in Lua.

---

## Overview

The example scripts are split into two parts:

- **Main scripts** (`Main_*.lua`) — one per class/level range, containing all user configuration (maps, potions, attack ranges, loot rules, etc.)
- **Script library** (`scriptlib/`) — shared modules that implement the actual logic (hunting, looting, shopping, traveling, buffs, alerts, etc.)

You only need to edit a **Main script** to configure the bot for your character. The scriptlib handles everything else.

### Main Scripts

Each `Main_*.lua` is a self-contained config file for a specific class and level range. For example:

| Script | Purpose |
|---|---|
| `Main.lua` | Generic template |
| `Main_Warrior_1_to_10.lua` | Beginner warrior, levels 1-10 |
| `Main_Royal_Warrior_1_to_10.lua` | Royal warrior, levels 1-10 |
| `Main_Royal_Warrior_30_up_Shanghai.lua` | Royal warrior 30+, Shanghai maps |
| `Main_Royal_Warrior_60_up_Sleepywood.lua` | Royal warrior 60+, Sleepywood maps |

All Main scripts follow the same structure: load modules, configure settings, call `maple.run()`. To create your own, copy any existing Main script and adjust the settings.

---

## Getting Started: dump_class.lua

Before writing your own scripts, **run `dump_class.lua` first**. It's a read-only demo that calls most of the Lua API and prints everything to the log — player stats, mobs, drops, buffs, inventory, map geometry, NPCs, other players, pets, chat log, and dialog state.

!!! tip "Start here"
    Load `dump_class.lua` in Wand and hit Run. Read through the output to see what data is available and how each API function returns it. This is the fastest way to understand the API before writing your own scripts.

It covers:

| Section | API calls used |
|---|---|
| Player stats | `get_player()` — position, HP/MP, level, stats, state flags |
| Mobs | `get_mobs()` — ID, name, HP%, position, platform |
| Drops | `get_drops()` — item ID, owner, meso flag, position |
| Buffs | `get_buffs()` — active buff IDs, remaining time, key bindings |
| Inventory | `get_inventory()`, `get_inventory_tab()` — all tabs with item stats |
| Map data | `get_physical_space()` — footholds, ropes, portals, platforms, bounds |
| Other players | `get_other_players()` — name, job, party status, position |
| NPCs | `get_npcs()` — ID, name, position |
| Pets | `get_pets()` — name, fullness |
| Chat log | `get_chatlog()` — recent messages |
| Dialog | `get_dialog_text()` — NPC dialog options |

The script makes no changes to game state — it only reads and prints. Use it as a reference when building your own scripts.

---

## Script Library (scriptlib)

The `scriptlib/` folder contains 10 modules:

| Module | Purpose |
|---|---|
| `maple.lua` | Main orchestrator — runs the grind loop, coordinates all other modules |
| `hunt.lua` | Mob targeting, attack range checks, movement to targets |
| `loot.lua` | Item pickup with priority filtering and anti-stuck |
| `shop.lua` | Auto buy/sell trips with return scroll support |
| `travel.lua` | Inter-map navigation via portals and NPCs |
| `level.lua` | Auto AP/SP distribution and job advancement |
| `distanceFunction.lua` | Pluggable distance functions for target selection |
| `virtualKey.lua` | Virtual key code constants |
| `global.lua` | Shared state flags |
| `store.lua` | Legacy packet-based shop (older alternative to `shop.lua`) |

---

## How It Works

When you call `maple.run()`, the main loop executes every 5ms:

```
maple.run() loop:
  1. checkMapleStates()     -- health checks: dialog removal, death revive, stuck detection
  2. checkLevel()           -- level-up tracking, stop condition
  3. shop.checkInventory()  -- trigger sell trip if equip slots full
  4. leveling.run()         -- job advance if pending (blocks hunting)
  5. shop.run()             -- buy/sell trip if triggered (blocks hunting)
  6. checkRotation()        -- rotate to next map if time/CC limit reached
  7. checkMapOwnership()    -- stranger detection, CC if map contested
  8. handlePendingCC()      -- move to safe spot and change channel
  9. checkStats()           -- potions (always), buffs (at hunt map only)
  10. grind()               -- loot + hunt at hunt map, or travel if not there
```

---

## Configuration Reference

Below is every setting you can configure in a Main script, organized by module.

### maple — General

```lua
maple.maxRunTimeMin = 3000       -- Auto-stop after X minutes
maple.stopAtLevel = 70           -- Stop at this level (0 = disabled)
```

### maple — Hunt Maps & Rotation

```lua
-- List of maps to hunt in (bot rotates through them)
maple.huntMaps = {
    { id = 100020000, safeSpot = { x = 99, y = 5 } },
    { id = 100030000, safeSpot = { x = -4096, y = -63 } },
}

-- Rotation triggers (whichever comes first)
maple.switchMapAfterMin = 10     -- Rotate after X minutes at one map
maple.switchMapAfterCC = 3       -- Rotate after X channel changes
```

The `safeSpot` is where the bot stands before changing channel to avoid dying during the transition.

### maple — Map Ownership

```lua
maple.ownershipTimeMin = 1          -- Minutes alone to claim the map
maple.ownerWaitTimeMin = 1          -- Grace period before leaving if you own the map
maple.strangerAlertIntervalSec = 6  -- Alert sound interval when strangers present
maple.whitelist = { "FriendName" }  -- Players that don't trigger stranger logic
```

When a stranger appears:

- If you **don't own** the map: immediately trigger channel change
- If you **own** the map: wait for grace period, then CC if they're still there
- Party members are auto-filtered and never trigger stranger logic

### maple — Buffs

```lua
maple.buffList = {
    { id = 1001003, onRope = false },  -- Iron Body
    { id = 1101006, onRope = true },   -- Rage (needs rope)
}
maple.rebuffSec = 10          -- Rebuff when remaining time < X seconds
maple.buffCooldownSec = 2     -- Min seconds between buff key presses
```

Set `onRope = true` for buffs that require standing on a rope/ladder to cast. The bot will find the nearest rope and move to it before casting. Buff keys are auto-detected from the game's key bindings.

### maple — Potions

```lua
-- All three are optional (set to nil to disable)
maple.hpPotion = { id = 2000001, minNum = 50, threshold = 100, buy = 200 }
maple.mpPotion = { id = 2000003, minNum = 50, threshold = 50,  buy = 150 }
maple.petPotion = { id = 2120000, threshold = 15 }

maple.potionCooldownMs = 200       -- Min ms between potion uses
maple.potionWarnIntervalMin = 1    -- Min minutes between low-stock warnings
```

| Field | Description |
|---|---|
| `id` | Item ID of the potion |
| `threshold` | Use potion when HP/MP falls below this raw value |
| `minNum` | Warn and trigger shop trip when count drops below this |
| `buy` | How many to buy per shop trip |

**Pet food** works differently: the bot checks all active pets via `get_pets()`, finds the one with the lowest fullness, and feeds it when fullness drops below `threshold`. An alert fires when pet food stock drops below 10.

### maple — Error Management

```lua
maple.alwaysEnsureInput = true  -- Auto-close dialogs/UI blocking input
maple.removeDialog = true       -- Dismiss unexpected NPC dialogs
```

### hunt — Attack Configuration

```lua
hunt.Attack.canTurn = true          -- Allow turning to face mobs
hunt.Attack.key = vk.VK_SHIFT      -- Single target attack key
hunt.Attack.keyAoe = vk.VK_A       -- AoE attack key
hunt.Attack.mobsToAttack = 1        -- Min mobs in range to attack
hunt.Attack.mobsToSeek = 1          -- Min mobs at destination to move there
hunt.Attack.stopOnAttack = false    -- Stop walking during attack animation

hunt.Attack.Range = {
    isFan = false,   -- false = rectangle, true = cone
    front = 120,     -- pixels in front
    back = 0,        -- pixels behind
    top = 30,        -- pixels above
    bottom = 10,     -- pixels below
}
```

When `mobsToAttack > 1`, the bot switches to `keyAoe` for the attack. The range defines the area where mobs are considered hittable.

**Fan mode** (`isFan = true`): The attack range is a cone that widens with distance. `front` becomes the radius, and `top`/`bottom` define the cone's vertical spread at max range.

### hunt — KeepAway Zone

```lua
hunt.KeepAway = {
    front = 10, back = 10,
    top = 10,   bottom = 10,
}
```

Mobs inside this zone are ignored for attacking (useful for ranged classes that need distance). Set all to 0 to disable.

### hunt — Filtering

```lua
hunt.mobIdFilter = { 100100 }       -- Ignore these mob IDs
hunt.platformFilter = { 0, 1 }     -- Ignore mobs on these platforms
hunt.dangerZones = {                -- Rectangular zones to avoid
    { x1 = 100, y1 = -200, x2 = 300, y2 = 0 },
}
```

### hunt — Distance Function

```lua
hunt.distanceFunc = dist.manhattan     -- Target selection algorithm
hunt.distanceParams = {}               -- Parameters for the function
```

See [Distance Functions](#distance-functions) below for all options.

### loot — Looting

```lua
loot.casualLoot = true           -- Pick up nearby items while hunting
loot.playerDropsOnly = true      -- Only loot your own drops
loot.lootStyle = 2               -- 1 = stop hunting for must-picks, 2 = attack while moving to loot

loot.mustPickTypes = { 0, 1 }    -- Always pick: 0=Mesos, 1=Equip, 2=Use, 3=Setup, 4=Etc, 5=Cash
loot.mustPickIds = { 4001207 }   -- Always pick these specific item IDs

loot.maxAttempts = 10            -- Give up on unreachable items after X attempts
loot.maxIgnored = 6              -- Max ignored items tracked (oldest removed first)
```

### shop — Shopping

```lua
shop.enable = { buy = true, sell = true }
shop.shopMapId = 100000102                 -- Map ID of the shop
shop.shopLocation = { x = -281, y = 182 } -- Stand here to open shop

shop.sellExcludeIds = { 1462003 }   -- Never sell these item IDs
shop.equipSlotBuffer = 1            -- Trigger sell when empty equip slots <= this
shop.sellUSE = true                 -- Sell Use tab items
shop.sellETC = true                 -- Sell Etc tab items
shop.sellAfterBuy = true            -- Also sell after a buy trip
shop.ShoppingDelay = 600            -- ms between shop actions

shop.useReturnScrollId = nil        -- Item ID of return scroll (nil = walk)
shop.additionalBuy = {              -- Extra items to buy besides potions
    -- { id = 2010004, targetCount = 1 },
}
```

Potion IDs (from `maple.hpPotion`, `maple.mpPotion`, `maple.petPotion`) are automatically added to the sell exclude list.

### leveling — Auto Level

```lua
leveling.enable = { ap = true, sp = true, job_adv = true }

leveling.ap_per_level = {
    hp = 0, mp = 0,
    str = 4, dex = 1, int = 0, luk = 0,
}

leveling.jobs = {
    { name = "swordman", lastJob = 0,   level = 10 },
    { name = "fighter",  lastJob = 100, level = 30 },
    { name = "crusader", lastJob = 111, level = 70 },
}
```

AP is distributed on every level-up. Job advancement triggers automatically when the character reaches the specified level and has the correct `lastJob`.

---

## Distance Functions

The `distanceFunction` module provides pluggable algorithms for target selection. Lower values = higher priority. Set via `hunt.distanceFunc` and `hunt.distanceParams`.

| Function | Params | Description |
|---|---|---|
| `dist.manhattan` | none | Sum of axis distances (`|dx| + |dy|`). Default, good general choice |
| `dist.euclidean` | none | Straight-line distance |
| `dist.chebyshev` | none | Max of axis distances. "King's move" metric |
| `dist.y_scaled` | `{y_scale}` | Euclidean with vertical axis scaled. `y_scale > 1` penalizes vertical distance |
| `dist.xy_scaled` | `{x_scale, y_scale}` | Independent scaling per axis |
| `dist.minkowski` | `{x_power, y_power}` | Generalized distance (p=1 is Manhattan, p=2 is Euclidean) |
| `dist.facing_weighted` | `{weight, y_scale}` | Prefer targets in facing direction. `weight=2` makes front targets appear 2x closer |
| `dist.input_weighted` | `{h_weight, v_weight}` | Prefer targets matching current arrow key input |

**Example:** Prefer mobs in front, penalize vertical movement:

```lua
hunt.distanceFunc = dist.facing_weighted
hunt.distanceParams = { 2.0, 1.5 }
```

---

## Alerts & Notifications

The bot sends messages through `play_alert(msg)` and `play_notify(msg)` at key events. If the Discord bot is connected, these appear in your Discord channel.

**Alerts** (something needs attention):

| Event | Message |
|---|---|
| Stranger on map | `Stranger: PlayerName` |
| Character died | `Died! Reviving...` |
| Unexpected dialog | `Unexpected dialog!` |
| Debuff applied | `Debuff: Seal` |
| Potion key not bound | `HP potion not on key!` |
| Pet food low | `Pet food low: 3` |
| Shop trip failed | `Buy failed! Trip aborted` |
| No huntable mobs | `No huntable mobs! (5 filtered)` |
| Max runtime | `Max runtime reached, stopping!` |
| Only 1 channel | `Only 1 channel, can't CC!` |

**Notifications** (informational):

| Event | Message |
|---|---|
| Level up | `Level Up! 29 -> 30` |
| Target level reached | `Target level 70 reached, stopping!` |
| Potion low | `HP potion low: 12` |
| Map claimed | `Map claimed!` |
| Map rotation | `Rotating to map 2` |
| Channel change | `CC -> ch5` |
| Shop trip started | `Shop: buy trip` |
| Job advance done | `Job advance complete!` |

---

## Creating Your Own Main Script

1. Copy any existing `Main_*.lua` as a template
2. Adjust the settings for your class, level range, and server
3. Key things to configure:
    - `maple.huntMaps` — which maps to hunt and safe spots
    - `maple.hpPotion` / `maple.mpPotion` — potion IDs and thresholds
    - `hunt.Attack.Range` — match your main attack skill's range
    - `hunt.Attack.key` / `keyAoe` — your attack keys
    - `shop.shopMapId` / `shopLocation` — nearest shop NPC
    - `leveling.ap_per_level` — AP distribution for your class
    - `leveling.jobs` — job advancement path
4. Load the script in Wand and click Run

!!! tip
    Use the **Inventory Tab** to look up item IDs and the **WZ Browser** for map IDs and mob IDs.
