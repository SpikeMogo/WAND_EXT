# Lua API Reference

Complete reference for all functions available in Wand Lua scripts.

---

## Player

### `get_player()`

Returns the local player's current state.

```lua
local p = get_player()
log_info("Level " .. p.level .. " at (" .. p.x .. ", " .. p.y .. ")")
```

**Fields:**

| Field | Type | Description |
|---|---|---|
| `x`, `y` | int | Map position |
| `vx`, `vy` | float | Velocity |
| `hp`, `mp` | int | Current HP/MP |
| `maxHp`, `maxMp` | int | Max HP/MP |
| `exp` | int | Current EXP |
| `expPer` | float | EXP percentage (0-100) |
| `mesos` | int | Mesos |
| `level` | int | Character level |
| `job` | int | Job ID |
| `attackCount` | int | Combo attack count |
| `breath` | int | Swim breath |
| `animation` | int | Current animation state |
| `comboCount` | int | Combo counter |
| `faceDir` | int | Facing direction |
| `uid` | int | Character unique ID |
| `channel` | int | Current channel |
| `total_channel` | int | Total channels |
| `isOnRope` | bool | On a ladder/rope |
| `isInAir` | bool | In the air (jumping/falling) |
| `isFaceDown` | bool | Facing downward |

**Nested — `p.basic`:**

| Field | Type |
|---|---|
| `str`, `dex`, `int`, `luk` | int |

**Nested — `p.secondary`:**

| Field | Type |
|---|---|
| `attack`, `defense` | int |
| `magic`, `magicDef` | int |
| `accuracy`, `avoid` | int |
| `hands`, `speed`, `jump` | int |

---

## Mobs, NPCs & Players

### `get_mobs()`

Returns all mobs on the current map.

```lua
local mobs = get_mobs()
for _, mob in ipairs(mobs) do
    log_info(mob.name .. " HP:" .. mob.HPP .. "% at (" .. mob.x .. "," .. mob.y .. ")")
end
```

| Field | Type | Description |
|---|---|---|
| `id` | int | Mob template ID |
| `x`, `y` | int | Position |
| `name` | string | Mob name |
| `HPP` | int | HP percentage (0-100) |
| `maxHP` | int | Maximum HP |
| `platform` | int | Platform ID the mob is on |

### `get_npcs()`

Returns all NPCs on the current map.

| Field | Type | Description |
|---|---|---|
| `id` | int | NPC ID |
| `x`, `y` | int | Position |
| `xp`, `yp` | int | Direction |
| `name` | string | NPC name |

### `get_other_players()`

Returns other players visible on the map.

| Field | Type | Description |
|---|---|---|
| `id` | int | Character ID |
| `job` | int | Job ID |
| `x`, `y` | int | Position |
| `name` | string | Player name |
| `jobName` | string | Job name string |
| `party` | bool | Is in your party |

### `get_pets()`

Returns your active pets.

| Field | Type | Description |
|---|---|---|
| `id` | int | Pet item ID |
| `name` | string | Pet name |
| `fullness` | int | Fullness (0-100) |

---

## Drops & Items

### `get_drops()`

Returns all items/mesos on the ground.

```lua
local drops = get_drops()
for _, d in ipairs(drops) do
    if d.isMeso then
        log_info("Meso drop at (" .. d.x .. "," .. d.y .. ")")
    else
        log_info(d.name .. " at (" .. d.x .. "," .. d.y .. ")")
    end
end
```

| Field | Type | Description |
|---|---|---|
| `uid` | int | Unique drop ID |
| `ownerId` | int | Owner character ID |
| `sourceId` | int | Source mob/NPC ID |
| `ownType` | int | Ownership type |
| `isMeso` | bool | True if meso drop |
| `id` | int | Item ID (or meso amount) |
| `x`, `y` | int | Position |
| `name` | string | Item name |
| `type` | int | Item type |

### `get_item_count(itemId)`

Returns the total count of an item across all inventory tabs.

```lua
local potions = get_item_count(2000000)  -- white potion
log_info("Potions: " .. potions)
```

### `get_item_slot(itemId)`

Returns the first inventory slot containing the item. Returns `0` if not found.

### `get_item_key(itemId)`

Returns the virtual key code mapped to the item. Returns `0` if no key is mapped.

### `use_item(itemId)`

Uses a consumable item from the Use tab (IDs starting with `2xxxxxx`). Returns `true` on success.

---

## Inventory

### `get_inventory()`

Returns all 5 inventory tabs.

```lua
local inv = get_inventory()
for _, item in ipairs(inv.use.items) do
    log_info("Slot " .. item.slot .. ": " .. item.name .. " x" .. item.quantity)
end
```

**Tab fields:** `equip`, `use`, `setup`, `etc`, `cash`

Each tab has:

| Field | Type | Description |
|---|---|---|
| `type` | int | Tab type (1-5) |
| `typeName` | string | Tab name |
| `items` | array | Items in this tab |
| `slotCount` | int | Total slots |

### `get_inventory_tab(tabType)`

Returns a single tab. `tabType`: 1=Equip, 2=Use, 3=Setup, 4=Etc, 5=Cash.

### Item Fields

| Field | Type | Description |
|---|---|---|
| `id` | int | Item ID |
| `quantity` | int | Stack count |
| `slot` | int | Slot number |
| `type` | int | Item type |
| `name` | string | Item name |
| `desc` | string | Item description |
| `stats` | EquipStats | Equipment stats (equip tab only) |

### EquipStats Fields

| Field | Type |
|---|---|
| `str`, `dex`, `int`, `luk` | int |
| `maxHp`, `maxMp` | int |
| `attack`, `magic` | int |
| `defense`, `magicDef` | int |
| `accuracy`, `avoid` | int |
| `hands`, `speed`, `jump` | int |
| `slots` | int |

---

## Map Data

### `get_physical_space()`

Returns the geometry of the current map.

```lua
local map = get_physical_space()
log_info("Map: " .. map.mapName .. " (" .. map.mapId .. ")")
log_info("Bounds: " .. map.left .. "," .. map.top .. " to " .. map.right .. "," .. map.bottom)
```

**Fields:**

| Field | Type | Description |
|---|---|---|
| `mapId` | int | Map ID |
| `streetName` | string | Area name |
| `mapName` | string | Map name |
| `left`, `right`, `top`, `bottom` | int | Map boundaries |
| `footholds` | array | Walkable segments |
| `ropes` | array | Ladders and ropes |
| `portals` | array | Map portals |
| `platforms` | array | Platform groups |

**Foothold:** `x1, y1, x2, y2, id, prev, next, layer, platform`

**Rope:** `id, isLadder, fromUpper, x, y1, y2, platform`

**Portal:** `x, y, id, type, toMapId, name, toName`

**Platform:** `id, leftX, rightX, centerX, size, isRope`

---

## Buffs & Status

### `get_buffs()`

Returns active buffs.

| Field | Type | Description |
|---|---|---|
| `type` | int | Buff type |
| `id` | int | Buff/skill ID |
| `subId` | int | Sub-identifier |
| `tLeft` | int | Time remaining (ms) |
| `vKey` | int | Virtual key |

### `get_debuffs()`

Returns a string describing current debuffs.

---

## Input & Keyboard

### Key Press Functions

```lua
press_key(0x25)       -- hold left arrow
sleep(500)
release_key(0x25)     -- release left arrow

hit_key(0x51)         -- quick tap Q key (20ms press)

send_key(0x51, 3)     -- send Q key 3 times
```

| Function | Description |
|---|---|
| `press_key(vk)` | Hold a key down |
| `release_key(vk)` | Release a held key |
| `hit_key(vk)` | Simulates a real key press and release (~20ms hold) |
| `send_key(vk, repeat)` | Sends a key message event. `repeat` defaults to 1 |
| `stop_move()` | Release all movement keys |

!!! note "hit_key vs send_key"
    **`hit_key`** simulates a physical key press — it calls `press_key` then `release_key` with a short delay. This is how a real keyboard input works and is required for **arrow keys** and movement. Use `press_key`/`release_key`/`hit_key` for movement controls.

    **`send_key`** sends a window message event directly. It works well for most skill and item keys, but because it doesn't simulate a real press-release cycle, **some skills may behave differently** — for example, a skill that has a charge/hold mechanic may trigger a prolonged hold when using `send_key`.

    If a key isn't responding as expected, try switching between the two.

### Mouse Functions

| Function | Returns | Description |
|---|---|---|
| `click(x, y)` | bool | Click at screen coordinates |
| `double_click(x, y)` | bool | Double-click at screen coordinates |

### Common Virtual Key Codes

| Key | Code | Key | Code |
|---|---|---|---|
| Left Arrow | `0x25` | A | `0x41` |
| Up Arrow | `0x26` | Z | `0x5A` |
| Right Arrow | `0x27` | 0-9 | `0x30`-`0x39` |
| Down Arrow | `0x28` | F1-F12 | `0x70`-`0x7B` |
| Space | `0x20` | Ctrl | `0x11` |
| Enter | `0x0D` | Shift | `0x10` |
| Escape | `0x1B` | Alt | `0x12` |
| Insert | `0x2D` | Delete | `0x2E` |

### Arrow Key States

```lua
local left, up, right, down = get_arrow_key_states()
if left then log_info("Moving left") end
```

---

## Skills & Key Mapping

### `get_skill_key(skillId)`

Returns the virtual key mapped to a skill. Returns `0` if unmapped.

```lua
local key = get_skill_key(2001002)  -- Magic Guard
if key ~= 0 then hit_key(key) end
```

### `get_virtual_key(type, data)`

General key lookup. Types: `1` = skill, `2` = item, `4`, `5`, `6` = UI and other functional actions.

!!! tip
    Run `dump_key_map()` in an empty script to print the full key map to the log. This shows every bound key with its type and data values — useful for finding the right parameters.

### `get_item_key(itemId)`

Shortcut for `get_virtual_key(2, itemId)`.

### `set_key_mapping(vk, type, data)`

Remap a key binding. Automatically closes any open dialogs first.

### `dump_key_map()`

Prints all key mappings to the log, including the type and data for each bound key.

---

## Movement & Pathfinding

### `move_to(x, y)` / `move_to(mob)` / `move_to(drop)` / `move_to(portal)`

Compute a path and move there. When possible, pass the object directly (mob, drop, or portal) rather than raw coordinates — this allows Wand to apply object-specific handling during movement. Use `move_to(x, y)` only when you need to move to a fixed location.

```lua
-- Move to coordinates
move_to(100, -200)

-- Move to nearest mob
local mobs = get_mobs()
if #mobs > 0 then move_to(mobs[1]) end

-- Move to a drop
local drops = get_drops()
if #drops > 0 then move_to(drops[1]) end
```

Returns `true` if movement completed successfully.

### `find_path(x, y)`

Compute a path without moving. Returns a `PathReturn` enum.

```lua
local result = find_path(500, -100)
if result == PathReturn.Found then
    move_to(500, -100)
end
```

| Value | Meaning |
|---|---|
| `PathReturn.Found` | Path exists |
| `PathReturn.NotFound` | No route available |
| `PathReturn.Float` | Character is airborne |
| `PathReturn.NoTarget` | Invalid target position |
| `PathReturn.Error` | Internal error |

### `get_travel_portal(toMapId)`

For inter-map travel. Returns the status and portal to take for the next step toward `toMapId`.

```lua
local status, portal = get_travel_portal(100000000)  -- Henesys
if portal then
    move_to(portal)
    sleep(500)
end
```

---

## Chat & Dialog

### `send_chat(type, message, target)`

Send an in-game chat message.

```lua
send_chat("all", "Hello world!")
send_chat("party", "Need heal!")
send_chat("whisper", "Hey there", "PlayerName")
```

| Type | Description |
|---|---|
| `"all"` | Normal chat |
| `"party"` | Party chat |
| `"guild"` | Guild chat |
| `"alliance"` | Alliance chat |
| `"spouse"` | Spouse chat |
| `"whisper"` | Whisper (requires `target`) |

### `get_chatlog()`

Returns recent chat messages.

| Field | Type | Description |
|---|---|---|
| `index` | int | Message index |
| `type` | int | Message type |
| `content` | string | Message text |

### `get_dialog_text()`

Returns NPC dialog options when a dialog is open.

| Field | Type | Description |
|---|---|---|
| `type` | int | Entry type |
| `select` | int | Selection index |
| `text` | string | Option text |

### `do_dialog_selection(selection)`

Click a dialog option by index. Returns `true` on success.

---

## Shopping

### `open_shop()`

Open the NPC shop dialog (must be near a shop NPC). Returns `true` on success.

### `buy_item(itemId, quantity)`

Buy an item from the currently open shop.

```lua
open_shop()
sleep(500)
buy_item(2000000, 100)  -- buy 100 white potions
```

### `sell_item(tab, index)`

Sell a single item. `tab`: 1-5, `index`: slot number.

### `sell_all_item(tab, excludeIds)`

Sell all items in a tab except those in the exclude list.

```lua
sell_all_item(4, {4000000, 4000001})  -- sell all Etc except these two
```

---

## Networking

### `send_packet(hexString)`

Send a raw packet to the server. The hex string represents the packet bytes.

```lua
send_packet("1A 00 FF 00")
```

!!! warning
    Sending invalid packets can disconnect you or trigger server-side detection. Use with caution.

### `change_channel(channel)`

Change to a specific channel number.

```lua
change_channel(3)  -- switch to channel 3
sleep(5000)        -- wait for channel change
```

---

## Stats & Skills

### `add_ap(stat)`

Click the AP (Ability Point) button for a stat.

```lua
add_ap("str")   -- add 1 AP to STR
add_ap("luk")   -- add 1 AP to LUK
```

Valid stats: `"hp"`, `"mp"`, `"str"`, `"dex"`, `"int"`, `"luk"`

### `add_sp(skillId)`

Click the SP (Skill Point) button for a skill.

```lua
add_sp(2001002)  -- add 1 SP to Magic Guard
```

---

## Alerts & Audio

### `play_alert()` / `play_alert(message)`

Play an alert sound. If a message is provided and the Discord bot is connected, also sends a red alert embed to Discord.

```lua
play_alert()                -- sound only
play_alert("Low HP!")       -- sound + Discord alert
```

### `play_notify()` / `play_notify(message)`

Play a notification sound. If a message is provided and the Discord bot is connected, also sends a blue notification embed to Discord.

```lua
play_notify()                    -- sound only
play_notify("Script finished")   -- sound + Discord notification
```

---

## Image Matching

### `find_image(path)`

Search for a template image inside the game window. Returns the **center coordinates** of the best match, or `nil` if no match is found within the configured threshold.

```lua
local result = find_image("potion.bmp")
if result then
    log_info("Found at (" .. result.x .. ", " .. result.y .. ") confidence: " .. result.confidence)
    click(result.x, result.y)
end
```

**Returns:** `table` or `nil`

| Field | Type | Description |
|---|---|---|
| `x` | int | Center X of the matched region (game window client coordinates) |
| `y` | int | Center Y of the matched region (game window client coordinates) |
| `confidence` | float | Match quality — 0.0 = perfect match, higher = worse |

**Preparing a template:**

1. Take a screenshot using the game's built-in screenshot function
2. Open the screenshot and crop the section you want to locate (e.g., an item icon, a UI element)
3. Save as **24-bit BMP** — this is the only supported format

The path can be absolute or relative to the exe folder.

!!! tip
    The match threshold is configurable in the **Settings** tab under **Image Matching**. Default is 5. Lower values require a closer pixel match; higher values are more lenient but may cause false positives.

---

## Utility

### `is_input_allowed()`

Returns `false` if player controls are blocked by other UIs (e.g. input box active, dialog UIs).

### `reset_focus()`

Force the game focus on the main window so player controls are not blocked.

### `revive_dead()`

Interact with the death/revive NPC. Returns `true` on success.
