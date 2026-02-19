log("Hello from Lua 5.3 + sol2!")

log("Maple Class Printer")


-- ---------------------------------------------------

local mobs = get_mobs()

for i, mob in ipairs(mobs) do
    print( string.format("Mob %d (%s) HP[%d%% of %d] at (%d, %d) platform (%d)", mob.id, mob.name, mob.HPP, mob.maxHP, mob.x, mob.y, mob.platform))

end


-- ---------------------------------------------------


local p = get_player()
    
print("=== Player Dump ===")

print(string.format("channel=%d of %d", p.channel, p.total_channel))

print(string.format("x=%d y=%d", p.x, p.y))
print(string.format("vx=%d vy=%d", p.vx, p.vy))


print(string.format("hp=%d mp=%d", p.hp, p.mp))
print(string.format("maxHp=%d maxMp=%d", p.maxHp, p.maxMp))
print(string.format("exp=%d (%.2f) level=%d", p.exp, p.expPer, p.level))

print(string.format("UID=%d faceDir=%d", p.uid, p.faceDir))
print(string.format("animation=%d attackCount=%d", p.animation, p.attackCount))
print(string.format("comboCount=%d breath=%d", p.comboCount, p.breath))

print(string.format("isOnRope=%s  isInAir=%s  isFaceDown=%s ",  tostring(p.isOnRope),  tostring(p.isInAir),  tostring(p.isFaceDown)))

print(string.format("basic.str=%d basic.dex=%d", p.basic.str, p.basic.dex))
print(string.format("basic.int=%d basic.luk=%d", p.basic.int, p.basic.luk))

print(string.format("secondary.attack=%d", p.secondary.attack))
print(string.format("secondary.defense=%d", p.secondary.defense))
print(string.format("secondary.magic=%d", p.secondary.magic))
print(string.format("secondary.magicDef=%d", p.secondary.magicDef))
print(string.format("secondary.accuracy=%d", p.secondary.accuracy))
print(string.format("secondary.avoid=%d", p.secondary.avoid))
print(string.format("secondary.hands=%d", p.secondary.hands))
print(string.format("secondary.speed=%d", p.secondary.speed))
print(string.format("secondary.jump=%d", p.secondary.jump))
print("===================")


-- ---------------------------------------------------

local drops = get_drops()

for i, d in ipairs(drops) do
    print((
        "Drop #%d: %s, type=%d \nUID=%d, ItemID=%d, \nOwner=%d, Source=%d, \nOwnType=%d, isMeso=%s, Pos=(%d,%d)"
    ):format(
        i,
        d.name,
        d.type,
        d.uid, d.id,
        d.ownerId, d.sourceId, d.ownType,
        tostring(d.isMeso),
        d.x, d.y
    ))
end

-- ---------------------------------------------------

local buffs = get_buffs()

print("=== Active Buffs ===")
for i, buff in ipairs(buffs) do
    local seconds = math.floor(buff.tLeft / 1000)
    print(string.format("Buff #%d: id=%d type=%d subId=%d tLeft=%ds vKey = %d",
        i, buff.id, buff.type, buff.subId, seconds, buff.vKey))
end
print("====================")

-- ---------------------------------------------------


local inv = get_inventory()
    
print("=== Inventory Dump ===")

print(string.format("-- Equip (%d slots) --", inv.equip.slotCount))
for _, item in ipairs(inv.equip.items) do
    print(string.format("[%d] %s id=%d qty=%d", item.slot, item.name, item.id, item.quantity))
    local e = item.stats
    local stats = {}
    if e.str > 0 then table.insert(stats, "STR+" .. e.str) end
    if e.dex > 0 then table.insert(stats, "DEX+" .. e.dex) end
    if e.int > 0 then table.insert(stats, "INT+" .. e.int) end
    if e.luk > 0 then table.insert(stats, "LUK+" .. e.luk) end
    if e.attack > 0 then table.insert(stats, "ATK+" .. e.attack) end
    if e.magic > 0 then table.insert(stats, "MAG+" .. e.magic) end
    if e.defense > 0 then table.insert(stats, "DEF+" .. e.defense) end
    if e.magicDef > 0 then table.insert(stats, "MDEF+" .. e.magicDef) end
    if e.accuracy > 0 then table.insert(stats, "ACC+" .. e.accuracy) end
    if e.avoid > 0 then table.insert(stats, "AVO+" .. e.avoid) end
    if e.hands > 0 then table.insert(stats, "HAND+" .. e.hands) end
    if e.speed > 0 then table.insert(stats, "SPD+" .. e.speed) end
    if e.jump > 0 then table.insert(stats, "JMP+" .. e.jump) end
    if e.maxHp > 0 then table.insert(stats, "HP+" .. e.maxHp) end
    if e.maxMp > 0 then table.insert(stats, "MP+" .. e.maxMp) end
    if e.slots > 0 then table.insert(stats, "Slots:" .. e.slots) end
    if #stats > 0 then
        print("    " .. table.concat(stats, " "))
    end
end

print(string.format("-- Use (%d slots) --", inv.use.slotCount))
for _, item in ipairs(inv.use.items) do
    print(string.format("[%d] %s id=%d qty=%d", item.slot, item.name, item.id, item.quantity))
end

print(string.format("-- Setup (%d slots) --", inv.setup.slotCount))
for _, item in ipairs(inv.setup.items) do
    print(string.format("[%d] %s id=%d qty=%d", item.slot, item.name, item.id, item.quantity))
end

print(string.format("-- Etc (%d slots) --", inv.etc.slotCount))
for _, item in ipairs(inv.etc.items) do
    print(string.format("[%d] %s id=%d qty=%d", item.slot, item.name, item.id, item.quantity))
end

print(string.format("-- Cash (%d slots) --", inv.cash.slotCount))
for _, item in ipairs(inv.cash.items) do
    print(string.format("[%d] %s id=%d qty=%d", item.slot, item.name, item.id, item.quantity))
end





local tab = get_inventory_tab(1)
    
print(string.format("=== %s (%d slots) ===", tab.typeName, tab.slotCount))

for _, item in ipairs(tab.items) do
    print(string.format("[%d] %s id=%d qty=%d", item.slot, item.name, item.id, item.quantity))
    if item.type == 1 then
        local e = item.stats
        local stats = {}
        if e.str > 0 then table.insert(stats, "STR+" .. e.str) end
        if e.dex > 0 then table.insert(stats, "DEX+" .. e.dex) end
        if e.int > 0 then table.insert(stats, "INT+" .. e.int) end
        if e.luk > 0 then table.insert(stats, "LUK+" .. e.luk) end
        if e.attack > 0 then table.insert(stats, "ATK+" .. e.attack) end
        if e.magic > 0 then table.insert(stats, "MAG+" .. e.magic) end
        if e.defense > 0 then table.insert(stats, "DEF+" .. e.defense) end
        if e.magicDef > 0 then table.insert(stats, "MDEF+" .. e.magicDef) end
        if e.accuracy > 0 then table.insert(stats, "ACC+" .. e.accuracy) end
        if e.avoid > 0 then table.insert(stats, "AVO+" .. e.avoid) end
        if e.hands > 0 then table.insert(stats, "HAND+" .. e.hands) end
        if e.speed > 0 then table.insert(stats, "SPD+" .. e.speed) end
        if e.jump > 0 then table.insert(stats, "JMP+" .. e.jump) end
        if e.maxHp > 0 then table.insert(stats, "HP+" .. e.maxHp) end
        if e.maxMp > 0 then table.insert(stats, "MP+" .. e.maxMp) end
        if e.slots > 0 then table.insert(stats, "Slots:" .. e.slots) end
        if #stats > 0 then
            print("    " .. table.concat(stats, " "))
        end
    end
end


-- ---------------------------------------------------


local space = get_physical_space()

print(("Map %d: %s - %s"):format(space.mapId, space.streetName, space.mapName))
print(("Bounds: L=%d R=%d T=%d B=%d"):format(space.left, space.right, space.top, space.bottom))

print("Footholds:")
for _, fh in ipairs(space.footholds) do
    print(("  id=%d layer=%d, (%d,%d) -> (%d,%d) prev=%d next=%d platform_id=%d")
        :format(fh.id, fh.layer, fh.x1, fh.y1, fh.x2, fh.y2, fh.prev, fh.next, fh.platform))
end

print("Ropes:")
for _, r in ipairs(space.ropes) do
    print(("  id=%d x=%d y1=%d y2=%d ladder=%s fromUpper=%s platform_id=%d")
        :format(r.id, r.x, r.y1, r.y2, tostring(r.isLadder), tostring(r.fromUpper), r.platform))
end

print("Portals:")
for _, p in ipairs(space.portals) do
    print(("  id=%2d type=%2d (%5d,%5d), [%7s]->[%7s]  ->map %d ")
        :format(p.id, p.type, p.x, p.y, p.name, p.toName, p.toMapId))
end

print("Platforms:")
for _, p in ipairs(space.platforms) do
    print(("  id=%2d left=%5d right=%5d center=%5d size=%5d isRope=%s")
        :format(p.id, p.leftX, p.rightX, p.centerX, p.size, tostring(p.isRope)))
end


-- ---------------------------------------------------


local others = get_other_players()
print("Other Players:")

for _, p in ipairs(others) do
    print(("%s [UID=%d] party=%s job=%d (%s) pos=(%d,%d)")
        :format(p.name, p.id, tostring(p.party), p.job, p.jobName, p.x, p.y))
end

print("All Npcs:")
local npcs = get_npcs()
for i, npc in ipairs(npcs) do
    print(string.format("NPC[%d] id=%d name=%s pos=(%d,%d)", i, npc.id, npc.name, npc.xp, npc.yp))
end


local pets = get_pets()
for _, pet in ipairs(pets) do
    log_info("Pet " .. pet.id .. ": " .. pet.name .. " fullness=" .. pet.fullness)
end



-- ---------------------------------------------------


local logs = get_chatlog()
for _, msg in ipairs(logs) do
    print(string.format("[%d] Type:%d | %s", msg.index, msg.type, msg.content))
end


local dialog = get_dialog_text()
for i, entry in ipairs(dialog) do
    print(string.format("[%d] type=%d sel=%d: %s", i, entry.type, entry.select, entry.text))
end


