--------------------------------------------------------------------------------
-- Travel Module
-- Navigate to destination map via portals
--------------------------------------------------------------------------------

local vk = require('virtualKey')

local module = {
    -- Internal
    portalDistX = 4,
    portalDistY = 25,
    portalDelay = 2000,
    walkTimeoutMs = 15000,      -- Max time to walk to a position before giving up
    mapWaitTimeoutMs = 60000,   -- Max time to wait for map transition
}

local function NPC_Talk(mapID, portal)

    local key = get_virtual_key(0x05, 54)

    print(mapID, portal.toMapId)
    


    if mapID == 105040400 and portal.toMapId == 105040401 then
        print("[Travel] NPC: Sleepywood Hotel to Sauna")

        send_key(key) sleep(1000)
        local dialog = get_dialog_text()

        if #dialog ==0 then
            print("[Travel] NPC: Dialog Error")
            return 0
        end

        send_key(key) sleep(1000)
        send_key(key) sleep(1000)
        send_key(vk.VK_RIGHT) sleep(200)  send_key(vk.VK_RETURN) sleep(200) 

        sleep(module.portalDelay)
    end
    return 1

end

function module.run(mapID)

    local space = get_physical_space()
    if(mapID==space.mapId) then
        print("[Travel] Target map arrived.")
        return 2
    end

    local res, portal = get_travel_portal(mapID)

    if res < 0 then
        print(string.format("[Travel] Unable to find portal to map %d (error: %d)", mapID, res))
        return res
    end

    local result = move_to(portal)

    if result == false then
        print(string.format("[Travel] Unable to find path to portal '%s' [%d %d]", portal.name, portal.x, portal.y))
        stop_move()
        return -1
    end

    local player = get_player()
    local dx = math.abs(player.x - portal.x)
    local dy = math.abs(player.y - portal.y)

    if dx < module.portalDistX and dy < module.portalDistY then

        if string.find(portal.name,"npc") then
            return NPC_Talk(space.mapId,portal)
        end

        print(string.format("[Travel] Use Portal '%s' [%d]", portal.name, space.mapId ))
        stop_move()
        send_key(vk.VK_UP)
        sleep(module.portalDelay)
        return 1
    end

    return 0
end

return module