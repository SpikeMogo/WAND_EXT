# Troubleshooting

---

## Common Issues

### Application won't start / DLL not found

**Symptom:** Double-clicking `Wand_Ext.exe` shows a "DLL not found" error or nothing happens.

**Cause:** The required DPP library DLLs are missing from the application folder.

**Fix:** Download the DLLs and place them next to `Wand_Ext.exe`. See [Setup](setup.md) for instructions.

---

### Game window not found

**Symptom:** The process list is empty or your game doesn't appear.

**Solutions:**

- Make sure the game window is **visible** (not minimized)
- Wait until you're past the login screen and on a character
- Wand only lists **32-bit** windows — 64-bit processes won't appear

---

### Character doesn't move (Arrow keys not working)

**Symptom:** Script calls `move_to()` but character stands still.

**Solutions:**

- Make sure you selected the correct process (MapleStory v83)
- Try restarting Wand and reattaching
- On **Win11**, keyboard state is handled differently — if movement doesn't work, report the issue with your Windows version and build number

---

### Hardware spoof not fully working

**Symptom:** Not all spoof components are active.

**Solutions:**

- The game may not use standard import paths for all functions on your system
- Volume serial spoofing alone still provides basic identity separation
- Try restarting both Wand and the game

---

### Discord bot won't connect

**Symptom:** Bot stays offline after clicking Connect.

**Checklist:**

1. DPP DLLs are in the same folder as `Wand_Ext.exe` (see [Setup](setup.md))
2. Token is correct (no extra spaces)
3. **Message Content Intent** is enabled in the Developer Portal
4. Bot has been invited to the server with correct permissions
5. Channel ID and Guild ID are correct

---

## Getting Help

If you're stuck, please post in the **#issues** channel on the [Wand Discord server](https://discord.gg/vBbq3bey5D). Include the following:

1. **Windows version** (Win10/Win11 and build number)
2. **What you were doing** when the issue occurred
3. **Lua script** if the issue is script-related
