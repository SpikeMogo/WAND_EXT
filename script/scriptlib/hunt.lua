--------------------------------------------------------------------------------
-- Hunt Module
-- Automated mob hunting with configurable attack ranges and target selection
--------------------------------------------------------------------------------

local vk = require('virtualKey')
local dist = require("distanceFunction")

local _failCount = 0
local _lastNoValidMobAlert = 0

local module = {
    -- KeepAway zone: mobs inside this zone are ignored for attack
    KeepAway = {
        front = 0,
        back = 0,
        top = 0,
        bottom = 0,
    },
    
    -- Attack configuration
    Attack = {
        canTurn = true,
        turnDelay = 300,
        key = vk.VK_CONTROL,
        keyAoe = vk.VK_CONTROL,
        mobsToAttack = 1,
        mobsToSeek = 1,
        stopOnAttack = false,
        count = 0,
        Range = {
            isFan = false,
            front = 80,
            back = 0,
            top = 30,
            bottom = 10,
        },
    },
    
    mobIdFilter = {9999999},               -- e.g. {9999999} ignore attacking these mob IDs
    platformFilter = {},            -- e.g. {0, 1, 2} ignore mobs on these platforms
    distanceFunc = dist.manhattan,
    distanceParams = {},
    dangerZones = {},
    _failCountThreshold = 50,
}

--------------------------------------------------------------------------------
-- Range Check Illustrations (facing right)
--------------------------------------------------------------------------------

--
-- Attack Range (Rectangular)
--
--        -Y 
--    ┌─────┬───────────┐
--    │  top│           │
--    │     │           │
--    │─────*───────front
--    │     │           │
--    │  bot│           │
--    └─────┴───────────┘ 
--        +Y 
--
-- X in [px - back, px + front]
-- Y in [py - top, py + bottom]
--

--
-- Attack Range (Fan/Cone)
--
--              ____--- top
--          _--─
--   Player *──────────front (radius)
--          `--_
--              ````--- bottom
--
-- Must be within radius AND in front half
-- Angle widens with distance (cone shape)
--

--
-- KeepAway Zone
--
--    ┌───────────┐
--    │ TOO CLOSE │
--    │     *─────front
--    │  Player   │
--    └───────────┘
--
-- Mobs inside this zone are skipped (for ranged attacks)
--

--------------------------------------------------------------------------------
-- Geometry
--------------------------------------------------------------------------------

local function is_in_danger_zone(x, y)
    for _, zone in ipairs(module.dangerZones) do
        if x > zone.x1 and x < zone.x2 and y > zone.y1 and y < zone.y2 then
            return true
        end
    end
    return false
end


--------------------------------------------------------------------------------
-- Range Checking
--------------------------------------------------------------------------------

-- KeepAway: returns true if mob is OUTSIDE the keepaway zone
local function is_outside_KeepAway(px, py, mx, my, facing)
    local ka = module.KeepAway
    
    if ka.front == 0 and ka.back == 0 and ka.top == 0 and ka.bottom == 0 then
        return true
    end
    
    if facing == 0 then
        return (mx <= px - ka.front or mx >= px + ka.back)
            or (my <= py - ka.top   or my >= py + ka.bottom)
    else
        return (mx <= px - ka.back  or mx >= px + ka.front)
            or (my <= py - ka.top   or my >= py + ka.bottom)
    end
end

-- Attack range: returns true if mob is INSIDE attack range
local function is_in_attack_range(px, py, mx, my, facing)
    local r = module.Attack.Range
    
    if r.isFan then
        local dx = mx - px
        local dy = my - py
        local front_dx = facing == 0 and -dx or dx
        
        if front_dx < 0 then
            return false
        end
        
        local dist_sq = dx * dx + dy * dy
        if dist_sq > r.front * r.front then
            return false
        end
        
        if dist_sq < 1 then
            return true
        end
        
        local d = math.sqrt(dist_sq)
        local ratio = d / r.front
        return dy >= -r.top * ratio and dy <= r.bottom * ratio
    else
        if facing == 0 then
            return mx >= px - r.front and mx <= px + r.back
               and my >= py - r.top   and my <= py + r.bottom
        else
            return mx >= px - r.back  and mx <= px + r.front
               and my >= py - r.top   and my <= py + r.bottom
        end
    end
end

--------------------------------------------------------------------------------
-- Filtering
--------------------------------------------------------------------------------

local function passes_filter(value, filter)
    if not filter or #filter == 0 then
        return true
    end
    for _, v in ipairs(filter) do
        if v == value then
            return false
        end
    end
    return true
end

local function is_valid_mob(mob)
    local player = get_player()
    return passes_filter(mob.id, module.mobIdFilter)
       and passes_filter(mob.platform, module.platformFilter)
       and not is_in_danger_zone(mob.x, mob.y)
       and is_outside_KeepAway(player.x, player.y, mob.x, mob.y, player.faceDir)
end

--------------------------------------------------------------------------------
-- Mob Counting
--------------------------------------------------------------------------------

-- Count mobs in attack range (for seeking - no keepaway check)
local function count_in_range(px, py, mobs, facing)
    local n = 0
    for _, mob in ipairs(mobs) do
        if is_in_attack_range(px, py, mob.x, mob.y, facing) then
            n = n + 1
        end
    end
    return n
end

-- Count mobs in range, checking both directions if canTurn
local function count_in_range_best(mx, my, mobs)
    local count = math.max(count_in_range(mx, my, mobs, 0), count_in_range(mx, my, mobs, 1))
    return count
end

-- Count attackable mobs (for attacking - WITH keepaway check)
local function count_attackable(px, py, mobs, facing)
    local n = 0
    for _, mob in ipairs(mobs) do
        if is_in_attack_range(px, py, mob.x, mob.y, facing)
           and is_outside_KeepAway(px, py, mob.x, mob.y, facing) then
            n = n + 1
        end
    end
    return n
end

--------------------------------------------------------------------------------
-- Mob Selection
--------------------------------------------------------------------------------

local function find_best_target(mobs)
    local player = get_player()
    local space = get_physical_space()

    -- Pre-filter to only valid mobs for cluster counting
    local validMobs = {}
    for _, mob in ipairs(mobs) do
        if is_valid_mob(mob) then
            validMobs[#validMobs + 1] = mob
        end
    end

    local best, best_dist = nil, math.huge

    for _, mob in ipairs(validMobs) do
        local in_bounds = mob.x > space.left and mob.x < space.right
                      and mob.y > space.top  and mob.y <= space.bottom

        if in_bounds then
            -- Count from mob's POV using only valid mobs - no keepaway needed
            local in_range = count_in_range_best(mob.x, mob.y, validMobs)

            if in_range >= module.Attack.mobsToSeek then
                local d = module.distanceFunc(player.x, player.y, mob.x, mob.y, module.distanceParams)
                if d < best_dist then
                    best, best_dist = mob, d
                end
            end
        end
    end

    -- Fallback: closest valid mob if none meets mobsToSeek threshold
    if not best then
        for _, mob in ipairs(validMobs) do
            local in_bounds = mob.x > space.left and mob.x < space.right
                          and mob.y > space.top  and mob.y <= space.bottom
            if in_bounds then
                local d = module.distanceFunc(player.x, player.y, mob.x, mob.y, module.distanceParams)
                if d < best_dist then
                    best, best_dist = mob, d
                end
            end
        end
    end

    return best
end

--------------------------------------------------------------------------------
-- Combat
--------------------------------------------------------------------------------

local function try_attack(mobs)
    local player = get_player()
    local atk = module.Attack
    
    if player.isFaceDown then
        stop_move()
        return false
    end
    
    if player.isOnRope or player.isInAir then
        return false
    end
    
    local facing = player.faceDir
    -- Player's POV - WITH keepaway check
    local count = count_attackable(player.x, player.y, mobs, facing)
    local need_turn = false
    
    if count < atk.mobsToAttack and atk.canTurn then
        need_turn = true
        count = count_attackable(player.x, player.y, mobs, 1 - facing)
    end
    
    if count < atk.mobsToAttack then
        return false
    end
    
    if need_turn then
        print("[Hunt] turn around")
        stop_move()
        if atk.turnDelay > 0 then
            sleep(math.random(atk.turnDelay / 1.5, atk.turnDelay))
        end
        hit_key(facing == 1 and vk.VK_LEFT or vk.VK_RIGHT,2)
        sleep(20)
        hit_key(facing == 1 and vk.VK_LEFT or vk.VK_RIGHT,2)
        sleep(20)
        stop_move()
    end
    
    if atk.stopOnAttack then
        stop_move()
    end
    
    -- make sure
    release_key(vk.VK_DOWN)
    
    local key = count > atk.mobsToAttack and atk.keyAoe or atk.key
    send_key(key)
    sleep(50)
    
    atk.count = atk.count + 1
    return true
end

--------------------------------------------------------------------------------
-- Main
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Main
--------------------------------------------------------------------------------

-- Returns: 0 = no mobs, 1 = attacked, 2 = moving to target
function module.run(skipMovement)
    local all_mobs = get_mobs()
    
    if not all_mobs or #all_mobs == 0 then
        return 0
    end
    
    local mobs = {}
    for _, mob in ipairs(all_mobs) do
        if is_valid_mob(mob) then
            mobs[#mobs + 1] = mob
        end
    end
    
    if #mobs == 0 then
        -- Mobs exist but all filtered out — alert once per 30s
        if #all_mobs > 0 then
            local now = os.clock()
            if now - _lastNoValidMobAlert >= 30 then
                print("[Hunt] " .. #all_mobs .. " mobs on map but none huntable (all filtered)")
                play_alert("No huntable mobs! (" .. #all_mobs .. " filtered)")
                _lastNoValidMobAlert = now
            end
        end
        return 0
    end
    
    if try_attack(mobs) then
        return 1
    end
    
    -- Skip movement if requested (e.g., loot is handling movement)
    if skipMovement then
        return 0
    end
    
    local target = find_best_target(mobs)
    
    if not target then
        return 0
    end
    
    local result = move_to(target)
    
    if result == false then
        _failCount = _failCount + 1
        if _failCount > module._failCountThreshold then
            print("[Hunt] Path failed, trying random mob")
            move_to(mobs[math.random(#mobs)])
        end
    else
        _failCount = 0
    end
    
    return 2
end

function module.resetFailCount()
    _failCount = 0
    _lastNoValidMobAlert = 0
end

return module