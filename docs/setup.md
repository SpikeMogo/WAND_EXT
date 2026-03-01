# Setup

**Need help?** Join the [Wand Discord server](https://discord.gg/vBbq3bey5D).

## Download

Download `Wand_Ext.exe` from the [Releases](https://github.com/SpikeMogo/WAND_EXT/releases) page. Place it in any folder.

The application requires `Wand_Ext.exe` along with the DPP library DLLs. All files must be in the same folder.

### How to get the DLLs

1. Download **`libdpp-10.1.4-win32-release-vs2022.zip`** from the [DPP releases page](https://github.com/brainboxdotcc/DPP/releases/tag/v10.1.4)
2. Extract the zip and find the `bin/` folder
3. Copy all DLLs from `bin/` into the same folder as `Wand_Ext.exe`

The required DLLs are:

| File | Purpose |
|---|---|
| `dpp.dll` | D++ Discord bot library |
| `libssl-3.dll` | OpenSSL |
| `libcrypto-3.dll` | OpenSSL |
| `opus.dll` | Audio codec |
| `zlib1.dll` | Compression |

Your folder should look like:

```
Wand_Ext/
├── Wand_Ext.exe
├── bot_settings.ini      ← auto-created
├── script/
│   └── your_script.lua
│
├── dpp.dll               ┐
├── libssl-3.dll          │ DPP library DLLs
├── libcrypto-3.dll       │ (required)
├── opus.dll              │
└── zlib1.dll             ┘
```

---

## First Run

1. **Start MapleStory** and log into a character
2. **Run `Wand_Ext.exe`** — a window with tabs will appear
3. **Select your game window** from the dropdown on the Main tab
4. All features will initialize automatically

See the [Discord](discord.md) page for bot configuration.

---

## Settings

Settings are stored in `bot_settings.ini` (created automatically on first run). You can edit them from the **Settings** tab or manually. Most changes take effect after switching maps.

### System

| Setting | Default | Description |
|---|---|---|
| `general.verbose` | `false` | Display verbose debug information for the pathfinder and other relevant systems |
| `general.map_travel_delay` | `10` | Delay in ms per frame for continuously pathfinding/moving when traveling the map list. Short delay is recommended; longer delay may help reduce excessive move adjustment |
| `general.shopping_delay` | `600` | Delay in ms for every internal step of sell and buy. Increase if game ping is high and shopping is bugged |

### Spoof

| Setting | Default | Description |
|---|---|---|
| `hwspoof.hardware_spoof` | `false` | Spoof Hardware Address and ID. Change effective after restarting Wand and game |
| `hwspoof.mac` | `57-41-4E-44-45-58` | Spoofed MAC address. Change effective after restart and re-attach |
| `hwspoof.serial` | `5350494B` | Spoofed volume serial (hex). Change effective after restart and re-attach |

### Pathfinder

| Setting | Default | Description |
|---|---|---|
| `pathfind.movement_sampling` | `20` | Distance of sampling when building possible movements in map. Small value may result in very slow computation. 20–30 is recommended |
| `pathfind.velocity_scale` | `1.03` | Slightly rescale the jump velocity. If the bot tries to reach a platform it cannot reach, dial it down a bit |
| `pathfind.teleport_prob` | `1.0` | Magician's teleport skill: how frequently to use the skill when moving the player (0.0–1.0) |
| `pathfind.path_randomness` | `0.3` | Randomness in the connection-build and pathfinder (0.0–1.0) |
| `pathfind.enable_warm_start` | `true` | Warm start with previous path when looking for a new path |
| `pathfind.enable_cross_teleport` | `true` | Allow teleporting to disconnected platforms |
| `pathfind.foothold_layer` | `true` | Take into account the layer difference of footholds in map, i.e., you pass through a wall when its layer is different than yours |

### Image Matching

| Setting | Default | Description |
|---|---|---|
| `image_match.threshold` | `5` | SAD threshold for `find_image()`. 0 = exact pixel match, higher = more lenient. Values above 15 may cause false positives. Accepts 24-bit BMP only |

### Discord

| Setting | Default | Description |
|---|---|---|
| `discord.token` | *(empty)* | Discord bot token |
| `discord.channel_id` | *(empty)* | Target channel ID |
| `discord.guild_id` | *(empty)* | Discord server (guild) ID |

See the [Discord](discord.md) page for bot setup instructions.

---

## Scripts Folder

Create a `script/` folder next to the exe for your Lua scripts. The settings will remember the last loaded script path.

The bundled `script/` folder contains a full hunting framework with class-specific configs and a shared script library. See the [Scripting](scripting.md) page for details.

```
Wand_Ext/
├── Wand_Ext.exe
├── MissingPortal.dat         Supplemental map connections
├── script/
│   ├── dump_class.lua        API demo — prints all game data
│   ├── Main.lua              Generic config template
│   ├── Main_*.lua            Class/level-specific configs
│   └── scriptlib/            Shared bot modules
│       ├── maple.lua             Main orchestrator
│       ├── hunt.lua              Mob targeting & combat
│       ├── loot.lua              Item pickup
│       ├── shop.lua              Buy/sell trips
│       ├── travel.lua            Map navigation
│       ├── level.lua             AP/SP/job advance
│       ├── distanceFunction.lua  Target selection algorithms
│       └── virtualKey.lua        Key code constants
└── ...
```
