--------------------------------------------------------------------------------
-- Loot Module
-- Automated item looting with priority and filtering
--------------------------------------------------------------------------------

local vk = require('virtualKey')
local dist = require('distanceFunction')

--[[
Item Types (from item ID / 1000000):
    0 = Mesos (special case, isMeso flag)
    1 = Equip
    2 = Use
    3 = Setup
    4 = Etc
    5 = Cash
--]]

local module = {
    -- Loot behavior
    casualLoot = true,          -- Loot nearby items opportunistically
    playerDropsOnly = true,     -- Only loot items dropped by player (ownerId == player.uid)

    -- Loot style for must-pick items:
    --   1 = Move to item, ignore mobs entirely
    --   2 = Move to item, but still attack mobs on the way
    lootStyle = 1,
    
    -- Priority items (always loot these)
    mustPickTypes = {           -- Item types to always pick up
        0,                      -- Mesos
        1,                      -- Equip
    },
    mustPickIds = {             -- Specific item IDs
        4001207,
    },
    
    -- Anti-stuck
    maxAttempts = 10,           -- Give up after this many attempts
    maxIgnored = 6,             -- Max items in ignore list
    
    -- Distance thresholds
    lootRangeX = 12,            -- X distance to trigger loot
    lootRangeY = 18,            -- Y distance to trigger loot
    
    -- Distance function for finding nearest
    distanceFunc = dist.manhattan,
    distanceParams = {},

    -- Loot key
    key = vk.VK_Z,
}

-- Internal state
local _currentTarget = nil
local _attemptCount = 0
local _ignoredUIDs = {}

--------------------------------------------------------------------------------
-- Filtering
--------------------------------------------------------------------------------

local function is_ignored(uid)
    for _, ignored in ipairs(_ignoredUIDs) do
        if ignored == uid then
            return true
        end
    end
    return false
end

local function add_to_ignored(uid)
    if #_ignoredUIDs >= module.maxIgnored then
        table.remove(_ignoredUIDs, 1)
    end
    table.insert(_ignoredUIDs, uid)
end

local function get_drop_type(drop)
    if drop.isMeso then
        return 0
    end
    return drop.type
end

local function is_must_pick(drop)
    if is_ignored(drop.uid) then
        return false
    end
    
    -- Check item ID
    for _, id in ipairs(module.mustPickIds) do
        if drop.id == id then
            return true
        end
    end
    
    -- Check item type (0 = mesos)
    local dropType = get_drop_type(drop)
    for _, itemType in ipairs(module.mustPickTypes) do
        if dropType == itemType then
            return true
        end
    end
    
    return false
end

local function is_valid_drop(drop, player)
    if is_ignored(drop.uid) then
        return false
    end
    
    -- seems like the party drop ownerId is a big number, not player id
    if module.playerDropsOnly and (drop.ownerId ~= player.uid  and drop.ownerId <1000000000 ) then
        return false
    end
    
    return true
end

--------------------------------------------------------------------------------
-- Target Selection
--------------------------------------------------------------------------------

local function find_best_drop(drops, player)
    local space = get_physical_space()
    
    local nearest = nil
    local nearestDist = math.huge
    
    local mustPick = nil
    local mustPickDist = math.huge
    
    for _, drop in ipairs(drops) do
        if not is_valid_drop(drop, player) then
            goto continue
        end
        
        local in_bounds = drop.x > space.left and drop.x < space.right
                      and drop.y > space.top  and drop.y <= space.bottom
        
        if not in_bounds then
            goto continue
        end
        local d = module.distanceFunc(player.x, player.y, drop.x, drop.y, module.distanceParams)
        
        if d < nearestDist then
            nearest = drop
            nearestDist = d
        end
        
        if is_must_pick(drop) and d < mustPickDist then
            mustPick = drop
            mustPickDist = d
        end
        
        ::continue::
    end
    
    return nearest, mustPick
end

--------------------------------------------------------------------------------
-- Looting
--------------------------------------------------------------------------------

local function is_in_loot_range(player, drop)
    return math.abs(player.x - drop.x) < module.lootRangeX
       and math.abs(player.y - drop.y) < module.lootRangeY
end

local function try_loot()
    send_key(module.key)
end

local function update_attempts(drop)
    if _currentTarget == drop.uid then
        _attemptCount = _attemptCount + 1
    else
        _currentTarget = drop.uid
        _attemptCount = 1
    end
    
    if _attemptCount > module.maxAttempts then
        print(string.format("[Loot] Giving up on drop %d after %d attempts", drop.uid, _attemptCount))
        add_to_ignored(drop.uid)
        _currentTarget = nil
        _attemptCount = 0
        return false
    end
    
    return true
end

--------------------------------------------------------------------------------
-- Initialization
--------------------------------------------------------------------------------

function module.init()
    local key = get_virtual_key(0x05, 50)
    
    if key ~= 0 then
        module.key = key
        print(string.format("[Loot] Loot bound to key: 0x%X", key))
    else
        print("[Loot] Failed to get loot key, using default: Z")
        module.key = vk.VK_Z
    end
end

--------------------------------------------------------------------------------
-- Main
--------------------------------------------------------------------------------

function module.run()
    local player = get_player()
    local drops = get_drops()
    if not drops or #drops == 0 then
        return nil
    end
    
    local nearest, mustPick = find_best_drop(drops, player)
    if mustPick then

        if is_in_loot_range(player, mustPick) then
            if update_attempts(mustPick) then
                try_loot()
                return "looting"
            else
                -- Gave up on this item, skip to casual loot or nil
                print("[Loot] mustPick item ignored, falling through")
            end
        else
            move_to(mustPick)
            return "moving"
        end
    end
    
    if module.casualLoot and nearest then

        if is_in_loot_range(player, nearest) then
            if update_attempts(nearest) then
                try_loot()
                return "looting"
            end
        end
    end
    
    return nil
end

function module.reset()
    _ignoredUIDs = {}
    _currentTarget = nil
    _attemptCount = 0
end

return module