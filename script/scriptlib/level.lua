--------------------------------------------------------------------------------
-- level Module
-- Automated ap, sp and more
--------------------------------------------------------------------------------

local vk = require('virtualKey')
local travel = require('travel')


local module = {
    ap_per_level = {hp=0,mp=0,str=4,dex=1,int=0,luk=0},
    jobs = {
        {name="swordman", lastJob = 0,   level=10},
        {name="fighter",  lastJob = 100, level=30},
        {name="crusader", lastJob = 110, level=70},
        {name="hero",     lastJob = 111, level=120},
    },
    enable = {ap=true, sp=true, job_adv = true},
    key_assign = {
        {level=10, action = {type = 2, data = 2000000, key = vk.VK_DELETE}},
    },
    _busy = false,
        -- In internal:
    _jobAdvFunc = nil,

}


--------------------------------------------------------------------------------
-- Skill IDs
--------------------------------------------------------------------------------

local Swordman = {
    IMPROVED_HPREC = 1000000,
    IMPROVED_MAXHP = 1000001,
    ENDURE         = 1000002,
    IRON_BODY      = 1001003,
    POWER_STRIKE   = 1001004,
    SLASH_BLAST    = 1001005,
}

local Fighter = {
    SWORD_MASTERY      = 1100000,
    AXE_MASTERY        = 1100001,
    FINAL_ATTACK_SWORD = 1100002,
    FINAL_ATTACK_AXE   = 1100003,
    SWORD_BOOSTER      = 1101004,
    AXE_BOOSTER        = 1101005,
    RAGE               = 1101006,
    POWER_GUARD        = 1101007,
}

local Crusader = {
    IMPROVING_MPREC = 1110000,
    SHIELD_MASTERY  = 1110001,
    COMBO           = 1111002,
    SWORD_PANIC     = 1111003,
    AXE_PANIC       = 1111004,
    SWORD_COMA      = 1111005,
    AXE_COMA        = 1111006,
    ARMOR_CRASH     = 1111007,
    SHOUT           = 1111008,
}

local Hero = {
    MAPLE_Swordman  = 1121000,
    MONSTER_MAGNET = 1121001,
    STANCE         = 1121002,
    ADVANCED_COMBO = 1120003,
    ACHILLES       = 1120004,
    GUARDIAN       = 1120005,
    RUSH           = 1121006,
    BRANDISH       = 1121008,
    ENRAGE         = 1121010,
    HEROS_WILL     = 1121011,
}

--------------------------------------------------------------------------------
-- Helper
--------------------------------------------------------------------------------

local function sp(id, count)
    count = count or 1
    for i = 1, count do
        print("adding sp ",id)
        add_sp(id); sleep(500)
    end
end

local function clear_UI()
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






local function swordman(level)
    -- Level 10:    +1 Improve HP Recovery (1)
    -- Level 11:    +3 Improve HP Recovery (4)
    -- Level 12:    +1 Improve HP Recovery (5), +2 Improving Max HP Increase (2)
    -- Level 13:    +3 Improving Max HP Increase (5)
    -- Level 14:    +3 Improving Max HP Increase (8)
    -- Level 15:    +2 Improving Max HP Increase (10), +1 Power Strike
    -- Level 16:    +3 Slash Blast (3)
    -- Level 17:    +3 Slash Blast (6)
    -- Level 18:    +3 Slash Blast (9)
    -- Level 19:    +3 Slash Blast (12)
    -- Level 20:    +3 Slash Blast (15)
    -- Level 21:    +3 Slash Blast (18)
    -- Level 22:    +2 Slash Blast (20), +1 Power Strike (2)
    -- Level 23:    +3 Power Strike (5)
    -- Level 24:    +3 Power Strike (8)
    -- Level 25:    +3 Power Strike (11)
    -- Level 26:    +3 Power Strike (14)
    -- Level 27:    +3 Power Strike (17)
    -- Level 28:    +3 Power Strike (20)
    -- Level 29-30: +3 Anything (6)

    if level == 10 then
        sp(Swordman.POWER_STRIKE, 1)
    elseif level == 11 then
        sp(Swordman.IMPROVED_HPREC, 3)
    elseif level == 12 then
        sp(Swordman.IMPROVED_HPREC, 2); sp(Swordman.IMPROVED_MAXHP, 1)
    elseif level == 13 then
        sp(Swordman.IMPROVED_MAXHP, 3)
    elseif level == 14 then
        sp(Swordman.IMPROVED_MAXHP, 3)
    elseif level == 15 then
        sp(Swordman.IMPROVED_MAXHP, 3);
    elseif level == 16 then
        sp(Swordman.SLASH_BLAST, 3)
    elseif level == 17 then
        sp(Swordman.SLASH_BLAST, 3)
    elseif level == 18 then
        sp(Swordman.SLASH_BLAST, 3)
    elseif level == 19 then
        sp(Swordman.SLASH_BLAST, 3)
    elseif level == 20 then
        sp(Swordman.SLASH_BLAST, 3)
    elseif level == 21 then
        sp(Swordman.SLASH_BLAST, 3)
    elseif level == 22 then
        sp(Swordman.SLASH_BLAST, 2); sp(Swordman.POWER_STRIKE, 1)
    elseif level == 23 then
        sp(Swordman.POWER_STRIKE, 3)
    elseif level == 24 then
        sp(Swordman.POWER_STRIKE, 3)
    elseif level == 25 then
        sp(Swordman.POWER_STRIKE, 3)
    elseif level == 26 then
        sp(Swordman.POWER_STRIKE, 3)
    elseif level == 27 then
        sp(Swordman.POWER_STRIKE, 3)
    elseif level == 28 then
        sp(Swordman.POWER_STRIKE, 3)
    elseif level >= 29 then
        sp(Swordman.ENDURE, 3)
    end
end

local function fighter(level)
    -- Level 30-36: +3 Sword Mastery (19)
    --   Level 30: +3 Sword Mastery (3)
    --   but first point must go to get skill, guide says total 19 by 36
    -- Level 37-43: +3 Sword Booster (20), +1 Power Guard (1)
    -- Level 44-53: +3 Power Guard (30), +1 Sword Mastery (20)
    -- Level 54-70: +52 Anything (53)

    -- Level 30: +3 Sword Mastery
    -- Level 31-36: +3 Sword Mastery each (total 19 by end of 36, so 36-30=6 levels * 3 = 18 + 1 = 19)
    --   Actually: 7 levels * ~3 = ~19. Let's do 6*3=18 + 1 extra = 19
    -- Level 37-43: +3 Sword Booster (20), then +1 Power Guard
    -- Level 44-53: +3 Power Guard (30), +1 Sword Mastery (20)
    -- Level 54-70: anything (Rage, etc)

    if level >= 30 and level <= 36 then
        -- +3 Sword Mastery per level (19 total by 36)
        if level == 36 then
            sp(Fighter.SWORD_MASTERY, 1)  -- reach 19
        else
            sp(Fighter.SWORD_MASTERY, 3)
        end
    elseif level >= 37 and level <= 43 then
        -- +3 Sword Booster, +1 Power Guard (when booster hits 20)
        if level <= 42 then
            sp(Fighter.SWORD_BOOSTER, 3)
        else
            sp(Fighter.SWORD_BOOSTER, 2); sp(Fighter.POWER_GUARD, 1)
        end
    elseif level >= 44 and level <= 53 then
        -- +3 Power Guard, +1 Sword Mastery (when guard hits 30)
        if level == 53 then
            sp(Fighter.POWER_GUARD, 2); sp(Fighter.SWORD_MASTERY, 1)
        else
            sp(Fighter.POWER_GUARD, 3)
        end
    elseif level >= 54 then
        sp(Fighter.RAGE, 3)
    end
end

local function crusader(level)
    -- Level 70-80:  Combo Attack (30), +1 Anything
    -- Level 81-120: +121 Anything (121)

    if level >= 70 and level <= 79 then
        sp(Crusader.COMBO, 3)
    elseif level == 80 then
        sp(Crusader.COMBO, 3)  -- reach 30 + leftover
    elseif level >= 81 then
        sp(Crusader.SHOUT, 3)
    end
end

local function hero(level)
    -- Level 120:     +1 Rush (1), +2 Brandish (2)
    -- Level 121-130: +3 Brandish (30), +2 Advanced Combo Attack (2)
    -- Level 131-140: +3 Advanced Combo Attack (30), +2 Power Stance (2)
    -- Level 141-150: +3 Power Stance (30), +2 Maple Swordman (2)
    -- Level 151-156: +3 Maple Swordman (20)
    -- Level 157-166: +3 Rush (30), +1 Achilles (1)
    -- Level 167-176: +3 Achilles (30), +1 Hero's Will (1)
    -- Level 177-200: +3 Anything (72)

    if level == 120 then
        sp(Hero.RUSH, 1); sp(Hero.BRANDISH, 2)
    elseif level >= 121 and level <= 130 then
        if level <= 129 then
            sp(Hero.BRANDISH, 3)
        else
            sp(Hero.BRANDISH, 1); sp(Hero.ADVANCED_COMBO, 2)  -- Brandish 30
        end
    elseif level >= 131 and level <= 140 then
        if level <= 139 then
            sp(Hero.ADVANCED_COMBO, 3)
        else
            sp(Hero.ADVANCED_COMBO, 2); sp(Hero.STANCE, 1)  -- ACB 30
        end
    elseif level >= 141 and level <= 150 then
        if level <= 149 then
            sp(Hero.STANCE, 3)
        else
            sp(Hero.STANCE, 2); sp(Hero.MAPLE_Swordman, 1)  -- Stance 30
        end
    elseif level >= 151 and level <= 156 then
        sp(Hero.MAPLE_Swordman, 3)
    elseif level >= 157 and level <= 166 then
        if level == 157 then
            sp(Hero.RUSH, 2); sp(Hero.ACHILLES, 1)
        elseif level <= 165 then
            sp(Hero.RUSH, 3)
        else
            sp(Hero.RUSH, 1); sp(Hero.ACHILLES, 2)  -- Rush 30
        end
    elseif level >= 167 and level <= 176 then
        if level <= 175 then
            sp(Hero.ACHILLES, 3)
        else
            sp(Hero.ACHILLES, 2); sp(Hero.HEROS_WILL, 1)  -- Achilles 30
        end
    elseif level >= 177 then
        sp(Hero.ENRAGE, 3)
    end
end


--------------------------------------------------------------------------------
-- Job handlers
--------------------------------------------------------------------------------
local function job_adv_Swordman()


    -- setting
    local job_adv_map = 102000003
    local npc_x = -261
    local npc_y = 195
    local key = get_virtual_key(5, 54)




    local player = get_player()
    local level = player.level
    if level ~= 10 then
        return
    end

    if player.job ~=0 then
        
        print("[Job Adv] Wrong job at job_adv_Swordman:  ",player.job)

        return

    end

    module._jobAdvFunc = job_adv_Swordman
    module._busy = true



    local space = get_physical_space()

    if space.mapId == job_adv_map then
        local distX = math.abs(player.x - npc_x)
        local distY = math.abs(player.y - npc_y)
        
        if distX < 10 and distY < 10 then
            stop_move()
            print("[Job Adv] Arrived at NPC")

            send_key(key) sleep(1000) 

            local dialog = get_dialog_text()
            local entry = dialog[1]
            if string.find(entry.text, "Do you want to become") == nil then
                print("[Job Adv] Dialog Error 1")
                play_alert("Job advance dialog error!")
                module._busy = false
                return
            end

            send_key(vk.VK_RETURN) sleep(1000) 
            send_key(vk.VK_RETURN) sleep(1000)


            local dialog = get_dialog_text()
            local entry = dialog[1]
            if string.find(entry.text, "From here on out") == nil then
                print("[Job Adv] Dialog Error 2")
                play_alert("Job advance dialog error!")
                module._busy = false
                return
            end

            print("[Job Adv] Done!")
            play_notify("Job advance complete!")
            sleep(2000)
            clear_UI()
            sleep(500)
            print("[Job Adv] SP!")

            swordman(player.level)
            module._busy = false


        else
            move_to(npc_x, npc_y)
        end


    else
        print("travel to job adv map ",job_adv_map)
        travel.run(job_adv_map)
    end


end

--------------------------------------------------------------------------------
-- Job dispatch
--------------------------------------------------------------------------------

local handle_skill = {
    ["swordman"]  = swordman,
    ["fighter"]  = fighter,
    ["crusader"] = crusader,
    ["hero"]     = hero,
}

local handle_adv = {
    ["swordman"]  = job_adv_Swordman,
    ["fighter"]  = nil,
    ["crusader"] = nil,
    ["hero"]     = nil,
}

local function get_current_job(player)
    local current = nil
    for _, job in ipairs(module.jobs) do
        if player.level >= job.level then
            current = job.name
        else
            break
        end
    end
    return current
end

local function get_current_adv(player)
    local current = nil
    for _, job in ipairs(module.jobs) do
        if job.lastJob == player.job and job.level==player.level then

            current = job.name
            break
        end
    end
    return current
end

--------------------------------------------------------------------------------
-- Main
--------------------------------------------------------------------------------



function module.run()

    local player = get_player()
    if module._busy ==false and module.enable.job_adv == true then
        local job = get_current_adv(player)
        if job then
            local handler = handle_adv[job]
            if handler then
                print("[Job Adv Started]")
                handler()
            end
        end
    end

    if module._busy and module._jobAdvFunc then
        module._jobAdvFunc()
    end
    return module._busy
end


function module.on_level()
    local player = get_player()
    
    -- ap
    if module.enable.ap == true then
        for stat, points in pairs(module.ap_per_level) do
            for i = 1, points do
                add_ap(stat); sleep(100)
            end
        end
    end

    -- sp
    if module.enable.sp == true then
        local job = get_current_job(player)
        if job then
            stop_move()
            local handler = handle_skill[job]
            if handler then
                handler(player.level)
            end
        end
    end

    return
end

return module