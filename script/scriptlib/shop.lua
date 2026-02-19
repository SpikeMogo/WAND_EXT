--------------------------------------------------------------------------------
-- 2026 by Spike
-- Shop module for MapleStory automation
-- Handles buying potions and selling equipment
--------------------------------------------------------------------------------

local travel = require('travel')
local vk     = require('virtualKey')

local module = {
----------------------------------------------------------------------------
    -- User Configuration
    ----------------------------------------------------------------------------
    
    -- Shop location
    shopMapId = 100000102,              -- Map ID where the shop NPC is located
    shopLocation = { x = 0, y = 0 },    -- Coordinates to stand at to open shop
    enable = {buy = true, sell = true},
    -- Selling configuration
    sellExcludeIds = {},                -- Item IDs to never sell (e.g. {1302000, 1302001})
    equipSlotBuffer = 1,                -- Trigger sell when empty equip slots <= this value
    sellUSE = true,
    sellETC = true,
    sellAfterBuy = true,                -- Also sell inventory after refill potions
    useReturnScrollId = nil,
    additionalBuy = {}, -- item you want to buy except the HP and MP Pots
                                                        --example  {{id = 2010004, targetCount = 1}}
    ShoppingDelay = 600,
    
    ----------------------------------------------------------------------------
    -- Internal State (do not modify)
    ----------------------------------------------------------------------------
    
    _buyAmount = 100,
    _state = "idle",  -- idle, traveling, buying, selling
    _needBuy = false,
    _needSell = false,
    _buyTriggered = false,
    _sellTriggered = false,
    _buyList = {},
}

--------------------------------------------------------------------------------
-- Inventory Checks
--------------------------------------------------------------------------------

local function checkNeedSell()
    local inv = get_inventory()
    local usedSlots = #inv.equip.items
    local emptySlots = inv.equip.slotCount - usedSlots
    return emptySlots <= module.equipSlotBuffer
end

--------------------------------------------------------------------------------
-- Shop Actions
--------------------------------------------------------------------------------

local function doBuy()


    if open_shop()==false then
        
        print("[Shop] Cannot Open Shop, Check MapId or NPC Location")
        return false
    end
    sleep(500)

    for _, item in ipairs(module._buyList) do
        local count = get_item_count(item.id)
        local needed = item.targetCount - count
        if needed > 0 then
            print(("[Shop] Buy %d x%d (have %d)"):format(item.id, needed, count))
            while needed>0 do
                if buy_item(item.id, math.min(module._buyAmount, needed)) then
                    needed = needed - module._buyAmount
                    sleep(module.ShoppingDelay)
                else
                    print(("[Shop] Failed to buy %d"):format(item.id))
                    return false
                end
                
            end

        end
        sleep(module.ShoppingDelay)
    end
    send_key(vk.VK_ESCAPE)
    return true
end

local function doSell()

    print("[Shop] Selling Inventory")

    if open_shop()==false then
        
        print("[Shop] Cannot Open Shop, Check MapId or NPC Location")
        return false
    end
    sleep(module.ShoppingDelay)

    print("[Shop] Try to Sell Equip Tab")
    if sell_all_item(1, module.sellExcludeIds) == false then
        return false
    end

    sleep(module.ShoppingDelay)


    if module.sellUSE then
        print("[Shop] Try to Sell Use Tab")
        if sell_all_item(2, module.sellExcludeIds) == false then
            return false
        end 
        sleep(module.ShoppingDelay)
    end

    if module.sellETC then
        print("[Shop] Try to Sell Etc Tab")
        if sell_all_item(4, module.sellExcludeIds) == false then
            return false
        end 
        sleep(module.ShoppingDelay)
    end

    send_key(vk.VK_ESCAPE)

    return true
end

--------------------------------------------------------------------------------
-- State Machine
--------------------------------------------------------------------------------

local function handleIdle()
    module._needBuy = module._buyTriggered
    module._needSell = module._sellTriggered or checkNeedSell()

    if module._needBuy or module._needSell then
        module._state = "traveling"
        module._scrollUsed = false
        print(("[Shop] Trip started (buy=%s, sell=%s)"):format(
            tostring(module._needBuy), tostring(module._needSell)))
    end
end

local function handleTraveling()
    local space = get_physical_space()

    -- Use return scroll once at the start of travel (not every tick)
    if module.useReturnScrollId and not module._scrollUsed and space.mapId ~= module.shopMapId then
        use_item(module.useReturnScrollId)
        sleep(1000)
        print("[Shop] Try Use Return Scroll")
        module._scrollUsed = true
    end

    if space.mapId ~= module.shopMapId then
        travel.run(module.shopMapId)
        return
    end
    
    local player = get_player()
    local distX = math.abs(player.x - module.shopLocation.x)
    local distY = math.abs(player.y - module.shopLocation.y)
    
    if distX < 10 and distY < 10 then
        stop_move()
        print("[Shop] Arrived at shop")
        module._state = module._needSell and "selling" or 
                        module._needBuy and "buying" or "idle"
    else
        move_to(module.shopLocation.x, module.shopLocation.y)
    end
end

local function handleBuying()
    if doBuy() == false then
        print("[Shop] Error Happened When Buying, retrying...")
        play_alert("Buy failed, retrying...")
        sleep(1000)
        if doBuy() == false then
            print("[Shop] Buy failed again, aborting trip")
            play_alert("Buy failed! Trip aborted")
            module._needBuy = false
            module._needSell = false
            module._buyTriggered = false
            module._sellTriggered = false
            module._state = "idle"
            return
        end
    end
    module._needBuy = false
    module._buyTriggered = false
    module._state = module._needSell and "selling" or "idle"
end

local function handleSelling()
    if doSell() == false then
        print("[Shop] Error Happened When Selling, retrying...")
        play_alert("Sell failed, retrying...")
        sleep(1000)
        if doSell() == false then
            print("[Shop] Sell failed again, aborting trip")
            play_alert("Sell failed! Trip aborted")
            module._needSell = false
            module._needBuy = false
            module._buyTriggered = false
            module._sellTriggered = false
            module._state = "idle"
            return
        end
    end
    module._needSell = false
    module._sellTriggered = false
    module._state = module._needBuy and "buying" or "idle"
end

--------------------------------------------------------------------------------
-- Public API
--------------------------------------------------------------------------------

function module.init(hpPotion, mpPotion, petPotion)
    module._buyList = {}
    module._state = "idle"
    module._buyTriggered = false
    module._sellTriggered = false

    -- Snapshot user config on first init, rebuild from it on re-init
    if not module._userExcludeIds then
        module._userExcludeIds = {}
        for _, id in ipairs(module.sellExcludeIds) do
            table.insert(module._userExcludeIds, id)
        end
    end
    -- Reset sellExcludeIds to user's original list
    module.sellExcludeIds = {}
    for _, id in ipairs(module._userExcludeIds) do
        table.insert(module.sellExcludeIds, id)
    end

    if hpPotion and hpPotion.id and hpPotion.buy then
        table.insert(module._buyList, { id = hpPotion.id, targetCount = hpPotion.buy })
        table.insert(module.sellExcludeIds, hpPotion.id)
        print("[Shop] HP potion " .. hpPotion.id .. " x" .. hpPotion.buy)
    end

    if mpPotion and mpPotion.id and mpPotion.buy then
        table.insert(module._buyList, { id = mpPotion.id, targetCount = mpPotion.buy })
        table.insert(module.sellExcludeIds, mpPotion.id)
        print("[Shop] MP potion " .. mpPotion.id .. " x" .. mpPotion.buy)
    end

    if petPotion and petPotion.id then
        table.insert(module.sellExcludeIds, petPotion.id)
        print("[Shop] Pet food " .. petPotion.id .. " added to sell exclude")
    end

    if module.useReturnScrollId then
        print("[Shop] Add Return Scroll " .. module.useReturnScrollId .. " to Buy list, minimum keep = 5")
        table.insert(module._buyList, { id = module.useReturnScrollId, targetCount = 5 })
        table.insert(module.sellExcludeIds, module.useReturnScrollId)
    end

    for _, item in ipairs(module.additionalBuy) do
        print("[Shop] Add Additional Buy " .. item.id .. " x"..item.targetCount)
        table.insert(module._buyList, { id = item.id, targetCount = item.targetCount })
    end
end

function module.triggerBuy()
    if not module._buyTriggered and module.enable.buy then
        module._buyTriggered = true
        print("[Shop] Buy triggered")
        play_notify("Shop: buy trip")

        if module.sellAfterBuy then
            module.triggerSell()
        end

    end
end

function module.triggerSell()
    if not module._sellTriggered and module.enable.sell then
        module._sellTriggered = true
        print("[Shop] Sell triggered")
        play_notify("Shop: sell trip")
    end
end

function module.isBusy()
    return module._state ~= "idle"
end

function module.checkInventory()
    if module._state == "idle" and checkNeedSell() then
        module.triggerSell()
    end
end

function module.run()
    if module._state == "idle" then
        handleIdle()
    elseif module._state == "traveling" then
        handleTraveling()
    elseif module._state == "buying" then
        handleBuying()
    elseif module._state == "selling" then
        handleSelling()
    end
    
    return module._state ~= "idle"
end

return module