--------------------------------------------------------------------------------
-- 2026 by Spike
-- Main bot framework for MapleStory automation
--------------------------------------------------------------------------------

local hunt = require('hunt')
local travel = require('travel')
local loot = require('loot')
local vk = require('virtualKey')
local global = require('global')
local shop = require('shop')
local leveling = require('level')

local module = {
    ----------------------------------------------------------------------------
    -- User Configuration
    ----------------------------------------------------------------------------
    
    -- Stop at level
    stopAtLevel = 0,                -- Stop script when reaching this level (0 = disabled)

    -- Maximum script runtime in minutes before auto-stop
    maxRunTimeMin = 300,
    
    -- Hunt maps list with safe spots for channel changing
    -- id: Map ID to hunt in
    -- safeSpot: Coordinates to stand before changing channel (avoid dying during CC)
    huntMaps = {
        { id = 107000000, safeSpot = { x = -129, y = -265 } },
        { id = 50000, safeSpot = { x = -61, y = 65 } },
    },
    
    -- Map rotation triggers (whichever comes first)
    switchMapAfterMin = 60,         -- Rotate to next map after X minutes of hunting
    switchMapAfterCC = 3,           -- Rotate to next map after X channel changes due to strangers
    
    -- Map ownership system
    -- Bot will claim a map after hunting alone, and give grace period before leaving if owned
    ownershipTimeMin = 2,           -- Minutes hunting alone to claim ownership
    ownerWaitTimeMin = 1,           -- Minutes to wait before CC if we own the map (grace period)
    strangerAlertIntervalSec = 10,  -- Seconds between alert sounds when strangers detected
    debuffAlertIntervalSec = 10,    -- Seconds between alert sounds when debuffs detected
    whitelist = {},                 -- Player names that don't trigger stranger logic
                                    -- Note: Party members are auto-filtered, no need to add here
    
    -- Buff system
    -- id: Skill ID of the buff
    -- onRope: If true, bot will move to nearest rope before casting (for buffs that need it)
    -- Keys are auto-detected from game keybindings at init
    buffList = {
        -- { id = 1001003, onRope = false },  -- Example: Iron Body
        -- { id = 2001002, onRope = true },   -- Example: Magic Guard (needs rope)
    },
    rebuffSec = 10,                 -- Rebuff when time remaining < X seconds
    buffCooldownSec = 4,            -- Minimum seconds between buff key presses
    


    -- Potion system
    -- id: Item ID of the potion
    -- minNum: Minimum count before warning (play_notify)
    -- threshold: Use potion when HP/MP falls below this raw value
    hpPotion = { id = 2000000, minNum = 50, threshold = 50, buy = 200 },  -- Example: Red Potion
    mpPotion = { id = 2000001, minNum = 50, threshold = 30, buy = 200 },  -- Example: Blue Potion
    petPotion = { id = 2120000, threshold = 30 },                         -- Pet food, feed when fullness < threshold
    potionCooldownMs = 300,         -- Minimum milliseconds between potion uses
    potionWarnIntervalMin = 1,


    -- error management
    alwaysEnsureInput = true,   -- will auto close any UI and dialog to make sure movement is alway enabled.
    removeDialog = true,
    
    ----------------------------------------------------------------------------
    -- Internal State (do not modify)
    ----------------------------------------------------------------------------
    
    
        -- Distance tolerance for position checks (safe spot, rope center)
    distToleranceX = 5,
    distToleranceY = 5,
    
    _mapIdx = 1,
    _ccPending = false,
    _buffKeys = {},
    _buffState = { lastPressTime = {}, pendingBuff = nil },
    _ownership = {
        mapId = nil,
        aloneStartTime = nil,
        owned = false,
        strangerDetectedTime = nil,
        lastAlertTime = nil,
    },
    _rotation = { mapStartTime = nil, ccCount = 0 },

    _potionKeys = { hp = 0, mp = 0, pet = 0 },
    _potionState = { lastHpTime = 0, lastMpTime = 0, lastPetTime = 0, lastHpWarnTime = 0, lastMpWarnTime = 0, lastPetWarnTime = 0 },
    _lastDebuffAlertTime = nil,
    _lastDialogAlertTime = nil,

    _lastLevel = nil,
}

local stuckStartTime = nil
local STUCK_THRESHOLD_MS = 500

local function checkLevel()
    local player = get_player()
    
    -- Track level up
    module._lastLevel = module._lastLevel or player.level
    if player.level > module._lastLevel then
        print("[Level] Level up! " .. module._lastLevel .. " -> " .. player.level)
        play_notify("Level Up! " .. module._lastLevel .. " -> " .. player.level)
        module._lastLevel = player.level

        leveling.on_level()
    end
    
    -- Check stop condition
    if module.stopAtLevel <= 0 then return false end
    
    if player.level >= module.stopAtLevel then
        play_notify("Target level " .. player.level .. " reached, stopping!")
        print("[Level] Reached level " .. player.level .. ", stopping script!")
        return true
    end
    return false
end


--------------------------------------------------------------------------------
-- Potion System
--------------------------------------------------------------------------------

function module.initPotionKeys()
    module._potionKeys = { hp = 0, mp = 0, pet = 0 }
    module._potionState = { lastHpTime = 0, lastMpTime = 0, lastPetTime = 0, lastHpWarnTime = 0, lastMpWarnTime = 0, lastPetWarnTime = 0 }

    if module.hpPotion and module.hpPotion.id then
        local key = get_virtual_key(0x2, module.hpPotion.id)
        if key == 0 then
            print("[Potion] Warning: HP potion " .. module.hpPotion.id .. " not assigned to key")
            play_alert("HP potion not on key!")
        else
            module._potionKeys.hp = key
            print("[Potion] HP potion " .. module.hpPotion.id .. " bound to key " .. key)
        end
    end

    if module.mpPotion and module.mpPotion.id then
        local key = get_virtual_key(0x2, module.mpPotion.id)
        if key == 0 then
            print("[Potion] Warning: MP potion " .. module.mpPotion.id .. " not assigned to key")
            play_alert("MP potion not on key!")
        else
            module._potionKeys.mp = key
            print("[Potion] MP potion " .. module.mpPotion.id .. " bound to key " .. key)
        end
    end

    if module.petPotion and module.petPotion.id then
        local key = get_virtual_key(0x2, module.petPotion.id)
        if key == 0 then
            print("[Potion] Warning: Pet food " .. module.petPotion.id .. " not assigned to key")
            play_alert("Pet food not on key!")
        else
            module._potionKeys.pet = key
            print("[Potion] Pet food " .. module.petPotion.id .. " bound to key " .. key)
        end
    end
end


local function checkPotions()
    local player = get_player()
    local state = module._potionState
    local now = os.clock() * 1000
    local nowSec = os.clock()
    -- Check HP potion
    if module.hpPotion and module._potionKeys.hp ~= 0 then
        local hpCount = get_item_count(module.hpPotion.id)
        if hpCount < module.hpPotion.minNum then
            if (nowSec - state.lastHpWarnTime) / 60 >= module.potionWarnIntervalMin then
                play_notify("HP potion low: " .. hpCount)
                print("[Potion] Warning: HP potion low (" .. hpCount .. "/" .. module.hpPotion.minNum .. ")")
                state.lastHpWarnTime = nowSec
                shop.triggerBuy()  -- ADD THIS
            end
        end
        
        -- Use HP potion if needed
        if player.hp < module.hpPotion.threshold and (now - state.lastHpTime) >= module.potionCooldownMs then
            send_key(module._potionKeys.hp)
            state.lastHpTime = now
            print("[Potion] Using HP potion (HP: " .. player.hp .. "/" ..player.maxHp .. ")")
        end
    end
    
    -- Check MP potion
    if module.mpPotion and module._potionKeys.mp ~= 0 then
        local mpCount = get_item_count(module.mpPotion.id)
        if mpCount < module.mpPotion.minNum then
            if (nowSec - state.lastMpWarnTime) / 60 >= module.potionWarnIntervalMin then
                play_notify("MP potion low: " .. mpCount)
                print("[Potion] Warning: MP potion low (" .. mpCount .. "/" .. module.mpPotion.minNum .. ")")
                state.lastMpWarnTime = nowSec
                shop.triggerBuy()  -- ADD THIS
            end
        end
        
        -- Use MP potion if needed
        if player.mp < module.mpPotion.threshold and (now - state.lastMpTime) >= module.potionCooldownMs then
            send_key(module._potionKeys.mp)
            state.lastMpTime = now
            print("[Potion] Using MP potion (MP: " .. player.mp .. "/" ..player.maxMp .. ")")
        end
    end

    -- Check pet food
    if module.petPotion and module.petPotion.id and module._potionKeys.pet ~= 0 then
        local petFoodCount = get_item_count(module.petPotion.id)

        -- Low stock alert
        if petFoodCount < 10 then
            if (nowSec - state.lastPetWarnTime) / 60 >= module.potionWarnIntervalMin then
                play_alert("Pet food low: " .. petFoodCount)
                print("[Potion] Warning: Pet food low (" .. petFoodCount .. ")")
                state.lastPetWarnTime = nowSec
            end
        end

        -- Feed the hungriest pet
        if (now - state.lastPetTime) >= module.potionCooldownMs*3 then
            local pets = get_pets()
            if pets and #pets > 0 then
                local hungriest = nil
                for _, pet in ipairs(pets) do
                    if not hungriest or pet.fullness < hungriest.fullness then
                        hungriest = pet
                    end
                end
                if hungriest and hungriest.fullness < module.petPotion.threshold then
                    send_key(module._potionKeys.pet)
                    state.lastPetTime = now
                    print("[Potion] Feeding pet " .. hungriest.name .. " (fullness: " .. hungriest.fullness .. ")")
                end
            end
        end
    end
end


--------------------------------------------------------------------------------
-- Buff System
--------------------------------------------------------------------------------

local function initBuffKeys()
    module._buffKeys = {}
    for _, buff in ipairs(module.buffList) do
        local key = get_virtual_key(0x1, buff.id)

        if key ==0 then
            key = get_virtual_key(0x2, buff.id)
        end

        if key == 0 then
            print("[Buff] Warning: " .. buff.id .. " not assigned to key")
        else
            module._buffKeys[buff.id] = key
            print("[Buff] " .. buff.id .. " bound to key " .. key)
        end
    end
end

local function isBuffActive(buffId, minTimeLeftMs)
    for _, buff in ipairs(get_buffs()) do
        if buff.id == buffId then
            return buff.tLeft >= minTimeLeftMs
        end
    end
    return false
end

local function getNearestRope()
    local space = get_physical_space()
    local player = get_player()
    local nearest, nearestDist = nil, math.huge
    
    for _, rope in ipairs(space.ropes) do
        local ropeY = (rope.y1 + rope.y2) / 2
        local dist = math.abs(player.x - rope.x) + math.abs(player.y - ropeY)
        if dist < nearestDist then
            nearestDist = dist
            nearest = rope
        end
    end
    return nearest
end

local function isAtRopeCenter(rope)
    local player = get_player()
    local ropeY = (rope.y1 + rope.y2) / 2
    return math.abs(player.x - rope.x) < module.distToleranceX
       and math.abs(player.y - ropeY) < module.distToleranceY
end

-- Returns: 1 = arrived, 0 = moving, -1 = failed
local function moveToRopeCenter(rope)
    local ropeY = (rope.y1 + rope.y2) / 2
    
    -- print(("[Buff] Move to rope [%d %d]"):format(rope.x, ropeY))
    if move_to(rope.x, ropeY) == false then
        return -1
    end
    
    if isAtRopeCenter(rope) then
        stop_move()
        return 1
    end
    return 0
end

local function pressBuffKey(buffId)
    local key = module._buffKeys[buffId]
    if not key or key == 0 then return false end
    
    local now = os.clock()
    local lastPress = module._buffState.lastPressTime[buffId] or 0
    if (now - lastPress) < module.buffCooldownSec then return false end
    send_key(key,2)
    sleep(200)

    module._buffState.lastPressTime[buffId] = now
    print("[Buff] Pressed key ".. key .." for " .. buffId)
    return true
end

-- Returns: true = busy (skip grind), false = done
local function checkStats()

    checkPotions()

    -- Only buff while at the hunt map
    local space = get_physical_space()
    local targetMap = module.huntMaps[module._mapIdx]
    if not targetMap or space.mapId ~= targetMap.id then
        return false
    end

    local state = module._buffState
    local rebuffMs = module.rebuffSec * 1000

    -- Handle pending rope buff
    if state.pendingBuff then
        local buff = state.pendingBuff
        local rope = getNearestRope()
        
        if not rope then
            print("[Buff] No rope found, casting anyway")
            pressBuffKey(buff.id)
            state.pendingBuff = nil
            return false
        end
        
        local result = moveToRopeCenter(rope)
        if result == 1 then
            pressBuffKey(buff.id)
            state.pendingBuff = nil
            return false
        elseif result == -1 then
            print("[Buff] Path failed, casting anyway")
            pressBuffKey(buff.id)
            state.pendingBuff = nil
            return false
        end
        return true  -- Still moving
    end
    
    -- Check all buffs
    for _, buff in ipairs(module.buffList) do
        local key = module._buffKeys[buff.id]
        if key and key ~= 0 and not isBuffActive(buff.id, rebuffMs) then
            if buff.onRope then
                state.pendingBuff = buff
                print("[Buff] " .. buff.id .. " needs refresh, moving to rope...")
                return true
            else
                pressBuffKey(buff.id)
            end
        end
    end
    
    -- Check debuffs
    local debuffs = get_debuffs()
    if debuffs ~= "" then
        local now = os.clock() * 1000  -- Convert to ms
        module._lastDebuffAlertTime = module._lastDebuffAlertTime or 0
        local intervalMs = module.debuffAlertIntervalSec * 1000
        
        if now - module._lastDebuffAlertTime >= intervalMs then
            print("[Debuff] " .. debuffs)
            play_alert("Debuff: " .. debuffs)
            module._lastDebuffAlertTime = now
        end
    end
    
    return false
    
end

local function stuckJumpOut()
    local player = get_player()
    local anim = player.animation
    local vx = math.abs(player.vx)
    local vy = math.abs(player.vy)
    
    -- Check if player appears stuck (animation 2 or 3, barely moving)
    local appearsStuck = (anim == 2 or anim == 3) and math.abs(vx) < 1 and math.abs(vy) < 1
    
    if appearsStuck then
        if stuckStartTime == nil then
            -- Start tracking stuck time
            stuckStartTime = os.clock() * 1000  -- Convert to ms
        else
            -- Check if stuck long enough
            local stuckDuration = (os.clock() * 1000) - stuckStartTime
            if stuckDuration >= STUCK_THRESHOLD_MS then
                -- Stuck for 500ms+, press jump
                local key = get_virtual_key(0x05, 53)
                send_key(key,2)
                stuckStartTime = nil  -- Reset timer after jump
            end
        end
    else
        -- Not stuck, reset timer
        stuckStartTime = nil
    end
end


local function checkMapleStates()



    
    -- stuck jump out
    stuckJumpOut()

    -- dialog check and alert

    local dialog = get_dialog_text()

    if #dialog ~=0 then
    

        local now = os.clock() * 1000  -- Convert to ms
        module._lastDialogAlertTime = module._lastDialogAlertTime or 0
        local intervalMs = 5 * 1000
        if now - module._lastDialogAlertTime >= intervalMs then
            print("Dialog popped!")
            play_alert("Unexpected dialog!")
            if module.removeDialog then
                send_key(vk.VK_ESCAPE) sleep(200)
            end
            module._lastDialogAlertTime = now
        end

    else

        -- input check skip the dialog alert

        if module.alwaysEnsureInput then
            local retries = 0
            while is_input_allowed() == false do
                print("Cannot do input, trying to reset focus")
                send_key(vk.VK_ESCAPE)
                sleep(300)
                retries = retries + 1
                if retries > 10 then
                    reset_focus()
                    print("[Error] Cannot regain input after 10 retries, giving up")
                    break
                end
            end
        end

    end

    -- check dead and revive
    if revive_dead() then
        
        print("Revive the dead!")
        play_alert("Died! Reviving...")
    end

end
--------------------------------------------------------------------------------
-- Ownership System
--------------------------------------------------------------------------------

local function isStranger(player)
    if player.party then return false end
    for _, name in ipairs(module.whitelist) do
        if player.name == name then return false end
    end
    return true
end

local function getStrangers()
    local strangers = {}
    for _, player in ipairs(get_other_players()) do
        if isStranger(player) then
            table.insert(strangers, player)
        end
    end
    return strangers
end

local function resetOwnership(mapId)
    module._ownership = {
        mapId = mapId,
        aloneStartTime = nil,
        owned = false,
        strangerDetectedTime = nil,
        lastAlertTime = nil,
    }
    hunt.resetFailCount()
end

local function checkMapOwnership()
    local space = get_physical_space()
    
    local targetMap = module.huntMaps[module._mapIdx]
    
    -- Only check ownership at hunt map
    if not targetMap or space.mapId ~= targetMap.id then
        return
    end

    local own = module._ownership
    local now = os.clock()
    
    -- Map changed
    if own.mapId ~= space.mapId then
        resetOwnership(space.mapId)
    end
    
    if module._ccPending then return end
    
    local strangers = getStrangers()
    
    if #strangers > 0 then
        -- Alert + log (throttled together)
        local names = {}
        for _, s in ipairs(strangers) do table.insert(names, s.name) end
        local nameStr = table.concat(names, ", ")

        local shouldLog = not own.lastAlertTime or (now - own.lastAlertTime) >= module.strangerAlertIntervalSec
        if shouldLog then
            play_alert("Stranger: " .. nameStr)
            own.lastAlertTime = now
            print("[Ownership] Strangers: " .. nameStr)
        end

        own.aloneStartTime = nil

        if own.owned then
            -- Grace period
            if not own.strangerDetectedTime then
                own.strangerDetectedTime = now
                print("[Ownership] Stranger entered, waiting " .. module.ownerWaitTimeMin .. " min")
            end
            local waitedMin = (now - own.strangerDetectedTime) / 60
            if waitedMin >= module.ownerWaitTimeMin then
                print("[Ownership] Grace period expired")
                module.triggerCC()
            end
        else
            if shouldLog then
                print("[Ownership] Don't own this map, leaving...")
            end
            module.triggerCC()
        end
    else
        own.strangerDetectedTime = nil
        if not own.aloneStartTime then own.aloneStartTime = now end
        
        if not own.owned then
            local aloneMin = (now - own.aloneStartTime) / 60
            if aloneMin >= module.ownershipTimeMin then
                own.owned = true
                print("[Ownership] Map claimed! (" .. string.format("%.1f", aloneMin) .. " min)")
                play_notify("Map claimed!")
            end
        end
    end
end


--------------------------------------------------------------------------------
-- Rotation System
--------------------------------------------------------------------------------

local function resetRotation()
    module._rotation.mapStartTime = nil
    module._rotation.ccCount = 0
    module._rotation._pausedAt = nil
    module._ccPending = false
end

local function rotateMap()
    local oldIdx = module._mapIdx
    module._mapIdx = (module._mapIdx % #module.huntMaps) + 1
    
    resetRotation()
    -- Only reset ownership if actually changing to a different map
    if module.huntMaps[oldIdx].id ~= module.huntMaps[module._mapIdx].id then
        resetOwnership(nil)
    end
    
    print("[Rotation] Map " .. oldIdx .. " -> " .. module._mapIdx)
    play_notify("Rotating to map " .. module._mapIdx)
end

local function checkRotation()
    local rot = module._rotation
    local now = os.clock()
    local space = get_physical_space()
    local targetMap = module.huntMaps[module._mapIdx]

    -- Only count hunt time when actually at the hunt map
    if not targetMap or space.mapId ~= targetMap.id then
        -- Pause the timer while not at hunt map (traveling, shopping, etc)
        if rot.mapStartTime then
            rot._pausedAt = rot._pausedAt or now
        end
        return false
    end

    -- Resume timer if we were paused
    if rot._pausedAt then
        local pausedDuration = now - rot._pausedAt
        rot.mapStartTime = rot.mapStartTime + pausedDuration
        rot._pausedAt = nil
    end

    if not rot.mapStartTime then rot.mapStartTime = now end

    local huntedMin = (now - rot.mapStartTime) / 60
    if huntedMin >= module.switchMapAfterMin then
        print("[Rotation] Time limit reached (" .. string.format("%.1f", huntedMin) .. " min)")
        rotateMap()
        return true
    end

    if rot.ccCount >= module.switchMapAfterCC then
        print("[Rotation] CC limit reached (" .. rot.ccCount .. ")")
        rotateMap()
        return true
    end

    return false
end


--------------------------------------------------------------------------------
-- Channel Change System
--------------------------------------------------------------------------------

-- Find safe spot by map ID (not by _mapIdx)
local function getSafeSpotForMap(mapId)
    for _, map in ipairs(module.huntMaps) do
        if map.id == mapId then
            return map.safeSpot
        end
    end
    return nil
end

-- Returns: 1 = arrived, 0 = moving, -1 = failed
local function moveToSafeSpot()
    local space = get_physical_space()
    local safeSpot = getSafeSpotForMap(space.mapId)
    
    if not safeSpot then
        print("[CC] No safe spot defined for map " .. space.mapId)
        return 1
    end
    
    if move_to(safeSpot.x, safeSpot.y) == false then
        print(("[CC] Unable to find path to safe spot %d %d"):format(safeSpot.x, safeSpot.y))
        stop_move()
        return -1
    end
    
    local player = get_player()
    if math.abs(player.x - safeSpot.x) < module.distToleranceX
       and math.abs(player.y - safeSpot.y) < module.distToleranceY then
        stop_move()
        return 1
    end
    return 0
end

local function doChangeChannel()
    module._rotation.ccCount = module._rotation.ccCount + 1
    print("[CC] Changing channel (" .. module._rotation.ccCount .. "/" .. module.switchMapAfterCC .. ")")

    local p = get_player()

    if p.total_channel <= 1 then
        print("[CC] Only 1 channel available, skipping CC")
        play_alert("Only 1 channel, can't CC!")
        module._ccPending = false
        return
    end

    local newChannel
    repeat
        newChannel = math.random(1, p.total_channel)
    until newChannel ~= p.channel

    print("[CC] Changing channel to " .. newChannel)
    play_notify("CC -> ch" .. newChannel)
    change_channel(newChannel)
    sleep(1000)

    module._ccPending = false
    resetOwnership(module._ownership.mapId)
    loot.reset()
end

function module.triggerCC()
    if not module._ccPending then
        module._ccPending = true
        print("[CC] Moving to safe spot...")
    end
end

-- Returns: true = busy (skip grind)
local function handlePendingCC()
    if not module._ccPending then return false end
    
    local result = moveToSafeSpot()
    if result == 1 then
        doChangeChannel()
    elseif result == -1 then
        print("[CC] Path failed, changing anyway...")
        doChangeChannel()
    end
    return true
end


--------------------------------------------------------------------------------
-- Grind Loop
--------------------------------------------------------------------------------

local function grind()
    local space = get_physical_space()
    local targetMap = module.huntMaps[module._mapIdx]
    
    if not targetMap then
        print("Invalid map index")
        return
    end
    
    if space.mapId == targetMap.id then
        if global.enabled.hunt then
            local lootStatus = nil
            if global.enabled.loot then
                lootStatus = loot.run()
            end
            
            if lootStatus == "moving" and loot.lootStyle == 2 then
                hunt.run(true)  -- Attack while moving to loot
            else
                hunt.run()
            end
        end
    else
        travel.run(targetMap.id)
    end
end


--------------------------------------------------------------------------------
-- Public API
--------------------------------------------------------------------------------

function module.init()
    loot.init()
    initBuffKeys()
    module.initPotionKeys()
    shop.init(module.hpPotion, module.mpPotion, module.petPotion)
end

function module.run()
    local startTime = os.clock()
    module.init()
    
    while true do
        -- Check runtime limit
        if (os.clock() - startTime) / 60 >= module.maxRunTimeMin then
            print("Max runtime reached, stopping")
            play_alert("Max runtime reached, stopping!")
            break
        end

        checkMapleStates()

        -- Check level limit
        -- put this before the leveling.run()
        if checkLevel() then
            break
        end
        
        -- Check inventory for selling
        shop.checkInventory()
        
        -- Job advance in progress: only keep potions alive
        if leveling.run() then
            checkPotions()
        -- Handle shop trips (pauses hunting)
        elseif shop.run() then
            checkPotions()
        else
            -- 1. Rotation check
            if not checkRotation() then
                -- 2. Ownership check
                checkMapOwnership()
            end
            
            -- 3. Channel change
            if not handlePendingCC() then
                -- 4. Buff check
                if not checkStats() then
                    -- 5. Grind
                    grind()
                end
            end
        end
        
        sleep(5)
    end
end

return module