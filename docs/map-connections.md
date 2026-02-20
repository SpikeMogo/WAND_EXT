# Add Map Connections

Wand reads portal and map connection data from the game's WZ files at startup. However, some connections are **server-side or scripted** — they exist in-game but don't appear in the WZ data. Examples include subway rides, elevator transitions, and NPC-gated entrances.

If `get_travel_portal()` cannot find a route to your destination, the connection is likely missing from the WZ data. You can add it manually using `MissingPortal.dat`.

---

## Diagnosing a Missing Connection

1. Open the **WZ Browser** tab in Wand
2. Look up your current map and check its portal list
3. If the portal to your target map is missing, you need to add it to `MissingPortal.dat`

You can also check from a script — if `get_travel_portal(targetMapId)` returns a negative status, the route doesn't exist in the portal graph.

---

## MissingPortal.dat

Place `MissingPortal.dat` in the same folder as `Wand_Ext.exe`. It is read once at startup — restart Wand after editing.

### Format

```
# Lines starting with # are comments
mapID, name, x, y, targetMapId
```

| Field | Description |
|---|---|
| `mapID` | The source map ID |
| `name` | Portal name (you choose this — see naming rules below) |
| `x, y` | Coordinates of the portal on the source map |
| `targetMapId` | The destination map ID |

### Portal Naming

The portal name controls how the bot interacts with it:

| Name contains | Behavior |
|---|---|
| *(anything without "npc")* | Bot walks to the coordinates and presses **Up** to enter, like a normal portal |
| `npc` | Bot walks to the coordinates and triggers `NPC_Talk()` in `travel.lua` for custom dialog handling |
| `dummy` | Used for wait/transition maps (elevators, boats) where the bot just needs to be present |

You can use any name you like — `manual_in`, `my_portal`, `subway_entrance` all work the same. Just include `npc` in the name if the connection requires talking to an NPC.

---

## Examples

### Simple portal (press Up to enter)

The Kerning City subway ticketing booth to the subway platform — this connection is server-side:

```
# Subway Ticketing Booth to Subway
103000100, manual_in, 195, 190, 103000101
```

The bot will walk to (195, 190) on map 103000100 and press Up.

### Elevator chain (dummy waypoints)

The Helios Tower elevator between Ludibrium and Korean Folk Town requires multiple maps. The bot enters the elevator, waits for the transition, then exits:

```
# Ludibrium to Korean Folk Town
# Helios tower 99 to elevator
222020200, manual_in, -133, 1963, 222020210
# wait in elevator
222020210, dummy, 64, 127, 222020211
# wait in elevator to Helios tower 2
222020211, dummy, -72, 127, 222020100

# Korean Folk Town to Ludibrium
# Helios tower 2 to elevator
222020100, manual_in, -135, 290, 222020110
# wait in elevator
222020110, dummy, 59, 127, 222020111
# wait in elevator
222020111, dummy, -72, 127, 222020200
```

### NPC portal (requires dialog)

The Sleepywood hotel entrance to the sauna requires talking to an NPC. Name it with `npc` so `travel.lua` handles it:

```
# Sleepywood hotel to Sauna
105040400, npc_talk, -284, 292, 105040401
```

Then add the dialog sequence in `travel.lua`'s `NPC_Talk()` function:

```lua
local function NPC_Talk(mapID, portal)
    local key = get_virtual_key(0x05, 54)

    if mapID == 105040400 and portal.toMapId == 105040401 then
        print("[Travel] NPC: Sleepywood Hotel to Sauna")
        send_key(key) sleep(1000)
        local dialog = get_dialog_text()
        if #dialog == 0 then
            print("[Travel] NPC: Dialog Error")
            return 0
        end
        send_key(key) sleep(1000)
        send_key(key) sleep(1000)
        send_key(vk.VK_RIGHT) sleep(200)
        send_key(vk.VK_RETURN) sleep(200)
        sleep(module.portalDelay)
    end

    return 1
end
```

The dialog sequence varies per NPC — use `get_dialog_text()` in `dump_class.lua` to figure out what buttons to press for each step.

---

## Adding Your Own

1. Find the **source map ID**, **coordinates**, and **target map ID** using the WZ Browser or by running `dump_class.lua` while standing at the location
2. Add a line to `MissingPortal.dat`
3. If the connection requires NPC dialog, name it with `npc` and add a handler in `travel.lua`'s `NPC_Talk()` function
4. Restart Wand to reload the file

!!! tip
    Use comments liberally in `MissingPortal.dat` to document what each entry does. Future you will thank you.
