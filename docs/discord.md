# Discord Bot

Control Wand remotely from your phone or any device with Discord. Join the [Wand Discord server](https://discord.gg/vBbq3bey5D) for support and updates.

!!! note
    The DPP library DLLs are required for Wand to run. Make sure they are in the same folder as `Wand_Ext.exe`. See [Setup](setup.md) for download instructions.

---

## Bot Setup

### 1. Create a Discord Bot

1. Go to the [Discord Developer Portal](https://discord.com/developers/applications)
2. Click **New Application** and give it a name
3. Go to **Bot** in the sidebar
4. Click **Reset Token** and copy the token
5. Enable **Message Content Intent** under Privileged Gateway Intents

### 2. Invite the Bot

1. Go to **OAuth2** > **URL Generator**
2. Select scopes: `bot`, `applications.commands`
3. Select permissions: `Send Messages`, `Read Message History`, `Attach Files`, `Use Slash Commands`
4. Copy the generated URL and open it in your browser
5. Select your server and authorize

### 3. Configure Wand

In the **Settings** tab, fill in:

| Setting | Where to Find It |
|---|---|
| `token` | Bot page in Developer Portal |
| `channel_id` | Right-click the channel in Discord > Copy Channel ID |
| `guild_id` | Right-click the server name > Copy Server ID |

!!! tip
    Enable **Developer Mode** in Discord settings (App Settings > Advanced) to see the "Copy ID" options.

### 4. Connect

Go to the **Discord** tab in Wand and click **Connect**. The bot should come online in your server.

---

## Slash Commands

| Command | Description |
|---|---|
| `/hello` | Bot says hello |
| `/ping` | Check bot latency |
| `/status` | Player stats — level, job, HP/MP, EXP, map, script status |
| `/runlua` | Start the loaded Lua script |
| `/stoplua` | Stop the running script |
| `/show` | Capture and send a game screenshot + map canvas |
| `/chatlog` | Toggle live chat forwarding to Discord |
| `/chat [type] [target]` | Set chat redirect mode |
| `/panel` | Show interactive control panel with buttons |
| `/help` | Show help embed |
| `/life` | List all mobs and players on the map |
| `/kill` | Terminate the game process |

---

## Control Panel

Use `/panel` to get an interactive button panel:

**Row 1 — Main Controls**

| Button | Color | Action |
|---|---|---|
| **Run** | Green | Start Lua script |
| **Stop** | Red | Stop Lua script |
| **Status** | Blue | Show player status |
| **Screenshot** | Blue | Capture game window |

**Row 2 — Extra Controls**

| Button | Color | Action |
|---|---|---|
| **Chat Log** | Gray/Green | Toggle chat monitoring |
| **Life** | Green | Show map entities |
| **Kill** | Red | Terminate game process |

**Row 3 & 4 — Chat Forward**

Quick-toggle chat redirect without using `/chat`. The active type is highlighted green.

| Button | Action |
|---|---|
| **Chat: Off / Chat: Stop** | Turn off chat redirect (red when active) |
| **All** | Redirect to normal chat |
| **Party** | Redirect to party chat |
| **Guild** | Redirect to guild chat |
| **Alliance** | Redirect to alliance chat |
| **Whisper** | Opens a popup to enter the target player name |
| **Spouse** | Redirect to spouse chat |

Once a chat type is active, any message you send in the Discord channel will be forwarded in-game. Press **Chat: Stop** to disable.

---

## Chat Redirect

Forward Discord messages directly into the game as chat.

### Enable

```
/chat all              — send as normal chat
/chat party            — send as party chat
/chat guild            — send as guild chat
/chat alliance         — send as alliance chat
/chat spouse           — send as spouse chat
/chat whisper Player   — send as whisper to Player
```

### Usage

Once enabled, type any message in the Discord channel and it will be sent in-game. A checkmark reaction confirms delivery.

### Disable

```
/chat stop
```

---

## Chat Log Monitoring

Use `/chatlog` to toggle live forwarding of in-game chat to your Discord channel. All chat types are forwarded (normal, party, guild, whisper, system messages).

!!! note
    Chat log monitoring polls periodically. There may be a slight delay between in-game messages and Discord delivery.
