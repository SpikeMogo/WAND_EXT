# Setup

**Need help?** Join the [Wand Discord server](https://discord.gg/vBbq3bey5D).

## Download

Download `Wand_Ext.exe` from the [Releases](https://github.com/user/Wand_Ext/releases) page. Place it in any folder.

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

Settings are stored in `bot_settings.ini` (created automatically on first run). You can edit them from the **Settings** tab or manually.

### General

| Setting | Default | Description |
|---|---|---|
| `verbose` | `false` | Enable verbose logging |
| `map_travel_delay` | `10` | Delay between map travel steps (ms) |
| `shopping_delay` | `600` | Delay between shop actions (ms) |

### Hardware Spoof

| Setting | Default | Description |
|---|---|---|
| `hardware_spoof` | `false` | Enable MAC and serial spoofing |
| `mac` | `57-41-4E-44-45-58` | Fake MAC address |
| `serial` | `5350494B` | Fake volume serial (hex) |

!!! note
    Hardware spoof must be enabled **before** attaching to the game process. Changing it while attached has no effect until you reattach.

### Pathfinding

| Setting | Default | Description |
|---|---|---|
| `movement_sampling` | `20` | Jump angle sampling (degrees) |
| `velocity_scale` | `1.03` | Physics velocity multiplier |
| `foothold_layer` | `true` | Use foothold layer detection |
| `enable_warm_start` | `true` | Reuse previous path computations |
| `teleport_prob` | `1.0` | Probability of using teleport (0.0-1.0) |
| `enable_cross_teleport` | `true` | Allow teleport across platforms |
| `path_randomness` | `0.3` | Add randomness to path selection |

### Discord

| Setting | Default | Description |
|---|---|---|
| `token` | *(empty)* | Discord bot token |
| `channel_id` | *(empty)* | Target channel ID |
| `guild_id` | *(empty)* | Discord server (guild) ID |

See the [Discord](discord.md) page for bot setup instructions.

---

## Scripts Folder

Create a `script/` folder next to the exe for your Lua scripts. The settings will remember the last loaded script path.

The bundled `script/` folder contains a full hunting framework with class-specific configs and a shared script library. See the [Scripting](scripting.md) page for details.

```
Wand_Ext/
├── Wand_Ext.exe
├── script/
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
