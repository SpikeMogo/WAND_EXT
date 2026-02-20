# Wand 3.0 (Wand_Ext)

**A fully external automation tool for MapleStory v83**

It's been a long time since the release of WAND 2.0, and a long time since I last played GMS private servers. Recently I started working on something new to pass the time, also because I wanted to see the capabilities of vibe coding — how far I can go with AI tools. And with the help of AI, many things have become achievable much faster.

This is **WAND_EXT**, a fully external tool that doesn't rely on injections. It doesn't require any detection bypass and it makes no modifications to the client's memory. All necessary information is read externally from the process memory space. It includes some cool new features:

- More Maple class information, including physical space, mob data, inventory, player stats, other players, and more
- WZ parsers for Maple world structures (for map traveling)
- An accurate replication of Maple's real movement physics based on physical space, gear, and stats — trajectory projection uses velocity, location, environments, key inputs to compute movement from foothold to foothold
- Lua scripting engine with 80+ API functions
- Discord bot for remote control from your phone
- And more

Around 80% of WAND_EXT was written with AI assistance. All the addresses, offsets, and data structures can be easily adapted to versions 62-92.

**Join the Wand Discord server:** [discord.gg/vBbq3bey5D](https://discord.gg/vBbq3bey5D)

### Demo Videos

- [Wand Ext Demo](https://youtube.com/shorts/qdQqcJMQQ2w)
- [Orbis Tower Challenge](https://youtu.be/ugW-z60WiRc)
- [Escape the Ant Tunnel](https://youtu.be/9oIn2OOiD_A)
- [EOS Tower Challenge in 20mins](https://youtu.be/2RAZpyR5nAE)

---

## About Being External

Unlike WAND 2.0, which operated as an injected DLL inside the client, **WAND_EXT is fully external** — it runs as a separate process. This means we cannot sit inside the client's memory space or call game functions directly. Everything has to be done from the outside, which brings a unique set of challenges:

- **No direct function calls** — we can't invoke game functions like an injected bot can. Input, movement, and interactions all need creative external approaches.
- **Read-only by design** — all game state (player position, mobs, inventory, map data) is read externally from process memory.
- **OS compatibility** — external techniques can behave differently across Windows versions. Wand has to handle Win10 and Win11 differences transparently.
- **Movement precision** — without internal access, replicating the game's physics accurately required building a complete physics simulation externally, matching velocity, gravity, jump arcs, and foothold interactions.

Building a full-featured external bot is significantly harder than an injected one. I did my best to deliver the most functionality possible for a good, legit botting experience — and I'm happy with where it landed. Some things that are trivial for an injected bot required real engineering effort to solve externally.

The upside of being fully external is that we don't have to worry about common client-side detections like CRC checks — no bypass is needed. We also have minimal impact on the client itself, which makes attaching and detaching much cleaner and easier.

---

## Disclaimer

!!! warning "Disclaimer"
    **This is a free, open-source project built purely for learning and experimentation.** Just like WAND 1.0 and WAND 2.0, the author has never profited from this project and will never use WAND_EXT as a commercial product. It is not monetized, not sold, and not commercially distributed. If you paid for this, you were scammed.

    This software is provided **as-is, with no warranty of any kind**, express or implied, including but not limited to the warranties of merchantability, fitness for a particular purpose, or non-infringement.

    - **Use at your own risk.** The author assumes **no liability** for any consequences resulting from the use of this software, including but not limited to account bans, data loss, legal action, or any other damages — direct or indirect.
    - This project is intended for **educational and testing purposes only** on private servers. The author does not condone or encourage the use of this tool in violation of any game's Terms of Service or applicable laws.
    - The author is **not responsible** for how others choose to use this software and **disclaims all legal responsibility** arising from its distribution or use.
    - No support or maintenance is guaranteed. The author may update or discontinue this project at any time without notice.
    - By downloading or using this software, you acknowledge that you have read, understood, and agreed to this disclaimer.

---

## Features

| Feature | Description |
|---|---|
| **Lua Scripting** | Full scripting engine with 80+ API functions for player data, movement, combat, inventory, and more |
| **Discord Bot** | Remote control via slash commands — run scripts, check status, capture screenshots from your phone |
| **Pathfinding** | Dijkstra-based navigation across platforms, ropes, ladders, and portals with physics simulation |
| **Interactive Map** | Real-time minimap showing mobs, drops, NPCs, portals, players, and computed paths |
| **Inventory Browser** | Visual grid of all inventory tabs with item names, stats, and descriptions |
| **WZ File Parser** | Reads WZ/IMG files automatically and loads Maple world structures for map traveling and browsing |
| **Hardware Spoof** | MAC address and volume serial spoofing for multi-client setups |
| **Background Operation** | The game continues to run and accept input while alt-tabbed |

---

## Quick Start

1. Download `Wand_Ext.exe` from the [Releases](https://github.com/SpikeMogo/WAND_EXT/releases) page
2. Place it in any folder
3. Run `Wand_Ext.exe`
4. Select your MapleStory window from the dropdown
5. Load a Lua script and hit **Run**

See the [Setup](setup.md) page for detailed instructions.

---

## System Requirements

- Windows 10 or 11 (x86/x64)
- MapleStory v83 client
- Visual C++ Redistributable 2022 (x86)

---

## Project Structure

```
Wand_Ext/
├── Wand_Ext.exe              Main application
├── bot_settings.ini          Settings (auto-created)
├── MissingPortal.dat         Supplemental map connections
├── script/                   Lua scripts
│   ├── dump_class.lua        API demo — prints all game data
│   ├── Main.lua              Config script (one per class/level)
│   ├── Main_*.lua            Class-specific configs
│   └── scriptlib/            Shared bot modules
│       ├── maple.lua             Main orchestrator
│       ├── hunt.lua              Mob targeting & combat
│       ├── loot.lua              Item pickup
│       ├── shop.lua              Buy/sell trips
│       ├── travel.lua            Map navigation
│       ├── level.lua             AP/SP/job advance
│       ├── distanceFunction.lua  Target selection algorithms
│       └── virtualKey.lua        Key code constants
│
├── dpp.dll                   ┐
├── libssl-3.dll              │ DPP library DLLs
├── libcrypto-3.dll           │ (required)
├── opus.dll                  │
└── zlib1.dll                 ┘
```

!!! info "DPP Library DLLs"
    Download **`libdpp-10.1.4-win32-release-vs2022.zip`** from the [DPP releases page](https://github.com/brainboxdotcc/DPP/releases/tag/v10.1.4) and copy all DLLs from `bin/` next to `Wand_Ext.exe`.
    See the [Setup](setup.md) page for details.
