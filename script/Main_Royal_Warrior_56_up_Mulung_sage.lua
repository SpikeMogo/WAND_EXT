----------------
-- 2026 by Spike

function script_path()
    local str = debug.getinfo(2, "S").source:sub(2)
    return str:match("(.*[/\\])")
end
print("Starting Script")
package.path = script_path().."?.lua;"..script_path().."scriptlib\\".."?.lua"
print("include package path: ", package.path)
math.randomseed(tostring(os.time()):reverse():sub(1, 7))



--------------------------------------------------------------------------------
-- load modules
--------------------------------------------------------------------------------
local vk     = require('virtualKey')
local dist   = require("distanceFunction")
local hunt   = require('hunt')
local loot   = require('loot')
local maple = require('maple')
local shop = require('shop')
local leveling = require('level')


----------------------------------------------------------------------------
-- User Configuration
----------------------------------------------------------------------------
-- Maximum script runtime in minutes before auto-stop

maple.maxRunTimeMin = 3000
maple.stopAtLevel = 65              -- Stop when reaching level 30 (0 = disabled)
-- Hunt maps list with safe spots for channel changing
-- id: Map ID to hunt in
-- safeSpot: Coordinates to stand before changing channel (avoid dying during CC)
maple.huntMaps = {
        { id = 250010502, safeSpot = { x=-879,  y= -177 } },
        -- { id = 220010400, safeSpot = { x=-232,  y= -186 } },

    }

-- Map rotation triggers (whichever comes first)
maple.switchMapAfterMin = 50          -- Rotate to next map after X minutes of hunting
maple.switchMapAfterCC = 3           -- Rotate to next map after X channel changes due to strangers

-- Map ownership system
-- Bot will claim a map after hunting alone, and give grace period before leaving if owned
maple.ownershipTimeMin = 1              -- Minutes hunting alone to claim ownership
maple.ownerWaitTimeMin = 1              -- Minutes to wait before CC if we own the map (grace period)
maple.strangerAlertIntervalSec = 6      -- Seconds between alert sounds when strangers detected
maple.whitelist = {     }               -- Player names that don't trigger stranger logic
                                        -- Note: Party members are auto-filtered, no need to add here

-- Buff system
-- id: Skill ID of the buff
-- onRope: If true, bot will move to nearest rope before casting (for buffs that need it)
-- Keys are auto-detected from game keybindings at init
maple.buffList = {
    { id = 1101004, onRope = false }, 
    { id = 1101006, onRope = false },  
    { id = 1101007, onRope = false },   

    
}
maple.rebuffSec = 10                 -- Rebuff when time remaining < X seconds
maple.buffCooldownSec = 2            -- Minimum seconds between buff key press attempts

-- error management
maple.alwaysEnsureInput = true -- will auto close any UI and dialog to make sure movement is alway enabled.




--------------------------------------------------------------------------------
-- potion settings
--------------------------------------------------------------------------------

maple.hpPotion = { id = 2000002, minNum = 50, threshold = 2000, buy = 1000 }  -- Use when HP < X
maple.mpPotion = { id = 2000003, minNum = 50, threshold = 20 ,  buy = 400 }  -- Use when MP < X
maple.potionCooldownMs = 200


--------------------------------------------------------------------------------
-- shop settings
--------------------------------------------------------------------------------

    -- Shop location
shop.enable = {buy = true, sell = true}
shop.shopMapId = 250000002             -- Map ID where the shop NPC is located
shop.shopLocation = { x = 192, y =  244 }    -- Coordinates to stand at to open shop
-- Selling configuration
shop.sellExcludeIds = {1402039,1432040}                -- Item IDs to never sell (e.g. {1302000, 1302001})
shop.equipSlotBuffer = 2                -- Trigger sell when empty equip slots <= this value

shop.sellUSE = true
shop.sellETC = true
shop.sellAfterBuy = true                -- Also sell inventory after refill potions
--------------------------------------------------------------------------------
-- hunt settings
--------------------------------------------------------------------------------

-- KeepAway zone: minimum safe distance from mobs
hunt.KeepAway = {
        front = 10,
        back = 10,
        top = 10,
        bottom = 10,
    }

-- Attack configuration
hunt.Attack.canTurn = true             -- Allow turning to face mobs
hunt.Attack.key = vk.VK_CONTROL        -- Single target attack
hunt.Attack.keyAoe = vk.VK_SHIFT     -- Multi target attack
hunt.Attack.mobsToAttack = 1           -- Min mobs to trigger attack, also switch for attack or AoeAttack
hunt.Attack.mobsToSeek = 1             -- Min mobs at destination to consider it
hunt.Attack.stopOnAttack = false       -- Stop movement during attack
hunt.Attack.Range = {
        isFan = false, -- Fan-shaped attack pattern
        front = 120,
        back = 0,
        top = 30,
        bottom = 10,
    }


-- Target filtering (empty = no filter, accept all)
hunt.mobIdFilter = {9999999}               -- e.g. {100100, 100101} only attack these mob IDs
hunt.platformFilter = {}            -- e.g. {0, 1, 2} only seek mobs on these platforms

-- Target selection
hunt.distanceFunc = dist.manhattan
hunt.distanceParams = {}

-- Danger zones: {{x1=, y1=, x2=, y2=},{x1=, y1=, x2=, y2=}} rectangles to avoid
hunt.dangerZones = {}


--------------------------------------------------------------------------------
-- loot settings
--------------------------------------------------------------------------------

loot.casualLoot = true          -- Loot nearby items opportunistically
loot.playerDropsOnly = true     -- Only loot items dropped by player (sourceId == player.uid)
loot.lootStyle = 2              -- Loot style for must-pick items: 1 = Move to item, ignore mobs entirely 2 = Move to item, but still attack mobs on the way       
-- Priority items (always loot these)
loot.mustPickTypes = {           -- Item types to always pick up: 0: Mesos; 1: Equip; 2: Use; 3: Setup; 4: Etc; 5: Cash
        1,
    }
loot.mustPickIds = {             -- Specific item IDs
        4001207,
    }

-- Anti-stuck
loot.maxAttempts = 10           -- Give up after this many attempts
loot.maxIgnored = 6             -- Max items in ignore list



--------------------------------------------------------------------------------
-- Level settings
--------------------------------------------------------------------------------

leveling.enable = {ap=true, sp=true, job_adv = false}
leveling.ap_per_level = {hp=0,mp=0,str=5,dex=0,int=0,luk=0}
leveling.jobs = {
        {name="swordman", lastJob = 0,   level=10},
        {name="fighter",  lastJob = 100, level=30},
        {name="crusader", lastJob = 110, level=70},
        {name="hero",     lastJob = 111, level=120},
    }


----------------------------------------------------
--Run the script
maple.run()
---------------------------------------------------