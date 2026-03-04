// LuaBindings.cpp - Clean header (replace lines 1-27)
// Remove duplicate includes

#include "DiscordBot.h"    // must come first (WinSock2 before Windows.h)

#include "LuaBindings.h"
#include "AppState.h"
#include "ScriptEngine.h"
#include <class/MapleClass.h>
#include <class/JobInfo.h>
#include <navigator/Map.h>
#include <wzparser/MapPathfinder.h>

#include <thread>
#include <chrono>
#include <string>
#include <unordered_set>

#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

#include <sound_sample/sound_warning.h>
#include <sound_sample/sound_notify.h>
#include "ImageMatch.h"


static void PlayEmbeddedSoundA(int i)
{
    if (i == 1)
        PlaySoundA(reinterpret_cast<LPCSTR>(g_SoundData_a), nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
    else
        PlaySoundA(reinterpret_cast<LPCSTR>(g_SoundData_b), nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
}

// Spam guard: skip duplicate Discord messages within 5 seconds
static bool ShouldSendDiscord(const std::string& message)
{
    static std::string lastMessage;
    static std::chrono::steady_clock::time_point lastTime;

    auto now = std::chrono::steady_clock::now();
    if (message == lastMessage &&
        std::chrono::duration_cast<std::chrono::seconds>(now - lastTime).count() < 5)
        return false;

    lastMessage = message;
    lastTime = now;
    return true;
}



inline void CopyEquipStats(const EquipStats& src, LuaEquipStats& dst)
{
    // Use visit() to copy all stats
    src.visit([&](const char* name, auto value) {
        if (strcmp(name, "STR") == 0) dst.str = value;
        else if (strcmp(name, "DEX") == 0) dst.dex = value;
        else if (strcmp(name, "INT") == 0) dst.int_ = value;
        else if (strcmp(name, "LUK") == 0) dst.luk = value;
        else if (strcmp(name, "MaxHP") == 0) dst.maxHp = value;
        else if (strcmp(name, "MaxMP") == 0) dst.maxMp = value;
        else if (strcmp(name, "Weapon Att") == 0) dst.attack = value;
        else if (strcmp(name, "Magic Att") == 0) dst.magic = value;
        else if (strcmp(name, "Weapon Def") == 0) dst.defense = value;
        else if (strcmp(name, "Magic Def") == 0) dst.magicDef = value;
        else if (strcmp(name, "Accuracy") == 0) dst.accuracy = value;
        else if (strcmp(name, "Avoid") == 0) dst.avoid = value;
        else if (strcmp(name, "Hands") == 0) dst.hands = value;
        else if (strcmp(name, "Speed") == 0) dst.speed = value;
        else if (strcmp(name, "Jump") == 0) dst.jump = value;
        else if (strcmp(name, "Upgrade Available") == 0) dst.slots =  static_cast<uint8_t>(value);
    }, true);
}


void SetupLuaEnvironment(AppState& app, ScriptEngine& engine)
{
    // Log callback
    engine.logCallback = [&app](const std::string& s) {
        app.logger.Add("[Lua] " + s);
    };

    engine.logCallbackE = [&app](const std::string& s) {
        app.logger.AddE("[Lua] " + s);
    };

    // Internal print/log override
    engine.Initialize();

    // Lua VM pointers
    sol::state& lua = engine.GetState();
    lua_State* L = lua.lua_state();

    // Store AppState* in Lua extraspace (available for other uses)
    *reinterpret_cast<AppState**>(lua_getextraspace(L)) = &app;

    // Cancel hook is now managed solely by SetCancelFlag/ClearCancelHook
    // in ScriptEngine — no duplicate hook here.

    app.logger.Add("[Lua] Install Functions");
    SetupLuaTypes(app,engine);
    SetupLuaFunctions(app,engine);

    // Snapshot globals after all bindings are registered
    // so CleanUserGlobals() knows what to preserve between runs
    engine.SnapshotGlobals();
    app.logger.Add("[Lua] Global snapshot taken");


}


void SetupLuaTypes(AppState& app, ScriptEngine& engine)
{
    sol::state& lua = engine.GetState();
    lua_State* L = lua.lua_state();



    lua.new_usertype<MobInfo>("Mob",
        "id",      &MobInfo::templateId,
        "x",       &MobInfo::x,
        "y",       &MobInfo::y,
        "name",    &MobInfo::name,
        "HPP",     &MobInfo::HP,
        "maxHP",   &MobInfo::maxHP,
        "platform",&MobInfo::Platform
    );


    lua.new_usertype<NpcInfo>("Npc",
        "id",   &NpcInfo::templateId,
        "x",    &NpcInfo::x,
        "y",    &NpcInfo::y,
        "xp",   &NpcInfo::xp,
        "yp",   &NpcInfo::yp,
        "name", &NpcInfo::name
    );

    // Register BasicStats
    lua.new_usertype<BasicStats>("BasicStats",
        "str", &BasicStats::str,
        "dex", &BasicStats::dex,
        "int", &BasicStats::int_,   // expose as "int" in Lua
        "luk", &BasicStats::luk
    );

    // Register SecondaryStats
    lua.new_usertype<SecondaryStats>("SecondaryStats",
        "attack",   &SecondaryStats::attack,
        "defense",  &SecondaryStats::defense,
        "magic",    &SecondaryStats::magic,
        "magicDef", &SecondaryStats::magicDef,
        "accuracy", &SecondaryStats::accuracy,
        "avoid",    &SecondaryStats::avoid,
        "hands",    &SecondaryStats::hands,
        "speed",    &SecondaryStats::speed,
        "jump",     &SecondaryStats::jump
    );

    // Register PlayerInfo with nested structs
    lua.new_usertype<PlayerInfo>("Player",
        // Position
        "x", &PlayerInfo::x,
        "y", &PlayerInfo::y,

        "vx", &PlayerInfo::vx,
        "vy", &PlayerInfo::vy,
        
        // Core stats
        "hp", &PlayerInfo::hp,
        "mp", &PlayerInfo::mp,
        "maxHp", &PlayerInfo::maxHp,
        "maxMp", &PlayerInfo::maxMp,
        "exp", &PlayerInfo::exp,
        "expPer", &PlayerInfo::expPer,
        "mesos", &PlayerInfo::mesos,
        "level", &PlayerInfo::level,
        "job", &PlayerInfo::job,
        
        // Status
        "attackCount", &PlayerInfo::attackCount,
        "breath", &PlayerInfo::breath,
        "animation", &PlayerInfo::animation,
        "comboCount", &PlayerInfo::comboCount,
        "faceDir", &PlayerInfo::faceDir,
        "uid", &PlayerInfo::UID,
        
        //other
        "channel", &PlayerInfo::channel, 
        "total_channel", &PlayerInfo::tot_channel, 

        //body
        "isOnRope",  &PlayerInfo::onRope,
        "isInAir",   &PlayerInfo::inAir,
        "isFaceDown",&PlayerInfo::faceDown,


        // Nested stats
        "basic", &PlayerInfo::basic,
        "secondary", &PlayerInfo::secondary
    );


    lua.new_usertype<DropInfo>("Drop",
        "uid",      &DropInfo::uid,
        "ownerId",  &DropInfo::ownerId,
        "sourceId", &DropInfo::sourceId,
        "ownType",  &DropInfo::ownType,
        "isMeso",   &DropInfo::isMeso,
        "id",       &DropInfo::id,
        "x",        &DropInfo::x,
        "y",        &DropInfo::y,
        "name",     &DropInfo::name,
        "type",     &DropInfo::type
    );


    lua.new_usertype<LuaEquipStats>("EquipStats",
        "str",      &LuaEquipStats::str,
        "dex",      &LuaEquipStats::dex,
        "int",      &LuaEquipStats::int_,
        "luk",      &LuaEquipStats::luk,
        "maxHp",    &LuaEquipStats::maxHp,
        "maxMp",    &LuaEquipStats::maxMp,
        "attack",   &LuaEquipStats::attack,
        "magic",    &LuaEquipStats::magic,
        "defense",  &LuaEquipStats::defense,
        "magicDef", &LuaEquipStats::magicDef,
        "accuracy", &LuaEquipStats::accuracy,
        "avoid",    &LuaEquipStats::avoid,
        "hands",    &LuaEquipStats::hands,
        "speed",    &LuaEquipStats::speed,
        "jump",     &LuaEquipStats::jump,
        "slots",    &LuaEquipStats::slots
    );

    lua.new_usertype<LuaInventoryItem>("InventoryItem",
        "id",       &LuaInventoryItem::id,
        "quantity", &LuaInventoryItem::quantity,
        "slot",     &LuaInventoryItem::slot,
        "type",     &LuaInventoryItem::type,
        "name",     &LuaInventoryItem::name,
        "desc",     &LuaInventoryItem::desc,
        "stats",    &LuaInventoryItem::stats
    );

    lua.new_usertype<LuaInventoryTab>("InventoryTab",
        "type",      &LuaInventoryTab::type,
        "typeName",  &LuaInventoryTab::typeName,
        "items",     &LuaInventoryTab::items,
        "slotCount", &LuaInventoryTab::slotCount
    );

    lua.new_usertype<LuaInventory>("Inventory",
        "equip", &LuaInventory::equip,
        "use",   &LuaInventory::use,
        "setup", &LuaInventory::setup,
        "etc",   &LuaInventory::etc,
        "cash",  &LuaInventory::cash
    );


    // ---- Physical space types ----

    lua.new_usertype<LuaFoothold>("Foothold",
        "x1",   &LuaFoothold::x1,
        "y1",   &LuaFoothold::y1,
        "x2",   &LuaFoothold::x2,
        "y2",   &LuaFoothold::y2,
        "id",   &LuaFoothold::id,
        "prev", &LuaFoothold::prev,
        "next", &LuaFoothold::next,
        "layer",&LuaFoothold::layer,
        "platform",&LuaFoothold::platform
    );

    lua.new_usertype<LuaRope>("Rope",
        "id",        &LuaRope::id,
        "isLadder",  &LuaRope::isLadder,
        "fromUpper", &LuaRope::fromUpper,
        "x",         &LuaRope::x,
        "y1",        &LuaRope::y1,
        "y2",        &LuaRope::y2,
        "platform",  &LuaRope::platform

    );

    lua.new_usertype<LuaPortal>("Portal",
        "x",        &LuaPortal::x,
        "y",        &LuaPortal::y,
        "id",       &LuaPortal::id,
        "type",     &LuaPortal::type,
        "toMapId",  &LuaPortal::toMapId,
        "name",     &LuaPortal::name,
        "toName",   &LuaPortal::toName
    );


    lua.new_usertype<LuaPlatform>("Platform",
    "id"     , &LuaPlatform::id     ,
    "leftX"  , &LuaPlatform::leftX  ,
    "rightX" , &LuaPlatform::rightX ,
    "centerX", &LuaPlatform::centerX,
    "size"   , &LuaPlatform::size   ,
    "isRope" , &LuaPlatform::isRope
    );



    lua.new_usertype<LuaPhysicalSpace>("PhysicalSpace",
        "left",      &LuaPhysicalSpace::left,
        "right",     &LuaPhysicalSpace::right,
        "top",       &LuaPhysicalSpace::top,
        "bottom",    &LuaPhysicalSpace::bottom,
        "mapId",     &LuaPhysicalSpace::mapId,
        "streetName",&LuaPhysicalSpace::streetName,
        "mapName",   &LuaPhysicalSpace::mapName,
        "footholds", &LuaPhysicalSpace::footholds,
        "ropes",     &LuaPhysicalSpace::ropes,
        "portals",   &LuaPhysicalSpace::portals,
        "platforms", &LuaPhysicalSpace::platforms
    );



    lua.new_usertype<OtherPlayerInfo>("OtherPlayer",
        "id",      &OtherPlayerInfo::id,
        "job",     &OtherPlayerInfo::job,
        "x",       &OtherPlayerInfo::x,
        "y",       &OtherPlayerInfo::y,
        "name",    &OtherPlayerInfo::name,
        "jobName", &OtherPlayerInfo::jobName,
        "party",   &OtherPlayerInfo::party
    );

        // Expose PathReturn enum to Lua
    lua.new_enum("PathReturn",
        "Error", static_cast<int>(MapStructures::PathReturn::Error),
        "Float", static_cast<int>(MapStructures::PathReturn::Float),
        "NoTarget", static_cast<int>(MapStructures::PathReturn::NoTarget),
        "NotFound", static_cast<int>(MapStructures::PathReturn::NotFound),
        "Found", static_cast<int>(MapStructures::PathReturn::Found)
    );

    lua.new_usertype<BuffInfo>("Buff",
    "type",   &BuffInfo::nType,
    "id",     &BuffInfo::nID,
    "subId",  &BuffInfo::nSubID,
    "tLeft",  &BuffInfo::tLeft,
    "vKey",   &BuffInfo::vKey
    );

    lua.new_usertype<ChatLogInfo>("ChatLogInfo",
    "index",   &ChatLogInfo::index,
    "type",    &ChatLogInfo::type,
    "content", &ChatLogInfo::content
    );

    lua.new_usertype<DialogTextEntry>("DialogTextEntry",
        "type", &DialogTextEntry::type,
        "select", &DialogTextEntry::select,
        "text", &DialogTextEntry::text
    );

    lua.new_usertype<PetInfo>("Pet",
        "id",       &PetInfo::id,
        "name",     &PetInfo::name,
        "fullness", &PetInfo::fullness
    );

}

void SetupLuaFunctions(AppState& app, ScriptEngine& engine)
{
    sol::state& lua = engine.GetState();
    lua_State* L = lua.lua_state();


    // ---- Memory diagnostic functions ----
    
    lua.set_function("mem_usage", [&engine]() -> size_t {
        return engine.GetMemoryUsage();
    });

    lua.set_function("mem_usage_kb", [&engine]() -> double {
        return engine.GetMemoryUsage() / 1024.0;
    });

    lua.set_function("force_gc", [&engine]() {
        engine.ForceGC();
    });

    lua.set_function("mem_report", [&engine, &app]() {
        engine.ForceGC();
        size_t bytes = engine.GetMemoryUsage();
        app.logger.Add("[Mem] After GC: " + std::to_string(bytes / 1024) + " KB");
    });

    // Check for global variable leaks - call at end of script to find leaked globals
    lua.set_function("check_globals", [&lua, &app]() -> std::vector<std::string> {
        // Add your bound function names to this set
        static const std::unordered_set<std::string> known = {
            "_G", "_VERSION", "assert", "collectgarbage",
            "dofile", "error", "getmetatable", "ipairs",
            "load", "loadfile", "next", "pairs",
            "pcall", "print", "rawequal", "rawget",
            "rawlen", "rawset", "require", "select",
            "setmetatable", "tonumber", "tostring", "type",
            "xpcall", "coroutine", "debug", "io",
            "math", "os", "package", "string", "table",
            "utf8", "log",
            // Memory functions
            "mem_usage", "mem_usage_kb", "force_gc", "mem_report", "check_globals",
            // Add all your other bound functions here...
            "sleep", "check_cancel", "log_info",
            "get_player", "get_mobs", "get_npcs", "get_drops", "get_pets", "get_inventory", "get_inventory_tab",
            "get_physical_space", "get_other_players", "get_buffs",
            "key_down", "key_up", "send_key", "press_key",
            "move_to", "stop_move", "jump", "attack",
            "get_skill_key", "get_virtual_key", "set_key_mapping", "dump_key_map",
            "get_arrow_key_states", "get_travel_portal",
            "play_alert", "play_notify", "is_input_allowed",
            "get_debuffs", "change_channel",
            "get_item_slot", "use_item",
            "buy_item", "sell_item", "sell_all_item", "open_shop",
            "get_chatlog", "get_dialog_text", "do_dialog_selection",
            "add_ap", "add_sp",
            "send_packet", "recv_packet", "send_chat",
            "PathReturn"  // enum
        };
        
        std::vector<std::string> leaks;
        sol::table globals = lua.globals();
        
        for (auto& pair : globals)
        {
            if (pair.first.is<std::string>())
            {
                std::string name = pair.first.as<std::string>();
                if (known.find(name) == known.end())
                {
                    std::string typeStr = sol::type_name(lua.lua_state(), pair.second.get_type());
                    leaks.push_back(name + " (" + typeStr + ")");
                }
            }
        }
        
        if (!leaks.empty())
        {
            std::string msg = "[Mem] Potential global leaks: ";
            for (size_t i = 0; i < leaks.size(); ++i)
            {
                if (i > 0) msg += ", ";
                msg += leaks[i];
            }
            app.logger.Add(msg);
        }
        
        return leaks;
    });

    // ---- End memory diagnostic functions ----


    // ====== bind mob ==========

    lua.set_function("get_mobs", [&app]() {
        std::vector<MobInfo> mobs;

        CMobPool::ForEachMob(app.hProcess, [&](const CMob& mob) -> bool
        {
            long x, y, px, py, HP, maxHP;
            uint32_t tid;
            std::string name;

            if (mob.Get(app.hProcess, x, y, px, py) &&
                mob.GetTemplateId(app.hProcess, tid))
            {
                if (!mob.GetTemplateName(app.hProcess, name))
                    name = "<unknown>";
                
                if(!mob.GetHP(app.hProcess, HP, maxHP))
                {
                    HP = maxHP = -1;
                }
                auto pf = MapleMap::GetMobPlatformID(x*1.0, y*1.0);

                mobs.emplace_back(MobInfo{ tid, (int)x, (int)y, name, HP, maxHP, pf});
            }
            return true;
        });

        return mobs; // sol2 converts vector<MobInfo> → Lua array of Mob
    });

    // ====== bind npc ==========

    lua.set_function("get_npcs", [&app]() {
        std::vector<NpcInfo> npcs;

        CNpcPool::ForEachNpc(app.hProcess, [&](const CNpc& npc) -> bool
        {
            long x, y, xp, yp;
            uint32_t tid;
            std::string name;

            if (npc.Get(app.hProcess, x, y, xp, yp) &&
                npc.GetTemplateId(app.hProcess, tid))
            {
                if (!npc.GetTemplateName(app.hProcess, name))
                    name = "<unknown>";

                npcs.emplace_back(NpcInfo{ tid, (int)x, (int)y, (int)xp, (int)yp, name });
            }
            return true;
        });

        return npcs; // sol2 converts vector<NpcInfo> → Lua array of Npc
    });

    // ====== send_packet (stripped) ==========
    lua.set_function("send_packet", [&app](const std::string& hexString) -> bool {
        // Network module stripped for open-source release
        return false;
    });

    // ====== recv_packet (stripped) ==========
    lua.set_function("recv_packet", [&app](const std::string& hexString) -> bool {
        // Network module stripped for open-source release
        return false;
    });

    // ====== send_chat (stripped) ==========
    lua.set_function("send_chat", [&app](const std::string& type, const std::string& message, sol::optional<std::string> target) -> bool {
        // Network module stripped for open-source release
        return false;
    });

    // ====== bind pets ==========

    lua.set_function("get_pets", [&app]() {
        std::vector<PetInfo> pets;

        if (!app.hProcess)
            return pets;

        CPet petArr[3];
        int count = CUserLocal::GetActivePets(app.hProcess, petArr);

        for (int i = 0; i < 3; i++)
        {
            if (!petArr[i].IsValid())
                continue;

            PetInfo info;
            int idx = -1;
            petArr[i].GetPetIndex(app.hProcess, idx);
            info.id = idx;

            std::string name;
            petArr[i].GetName(app.hProcess, name);
            info.name = std::move(name);

            int rep = 0;
            petArr[i].GetRepleteness(app.hProcess, rep);
            info.fullness = rep;

            pets.emplace_back(std::move(info));
        }

        return pets;
    });

    // ====== bind player ==========
    lua.set_function("get_player", [&app]() {
        PlayerInfo p;

        // Position & Status
        CUserLocal::GetPos(app.hProcess, p.x, p.y);
        double expPer = 0;
        CUserLocal::GetStats(app.hProcess, p.hp, p.mp, p.maxHp, p.maxMp, p.exp, p.expPer, p.level, p.job, p.mesos);
        CUserLocal::GetStatus(app.hProcess, p.attackCount, p.breath, p.animation, p.comboCount, p.faceDir, p.UID);

        //others
        CUserLocal::GetChannel(app.hProcess, p.channel, p.tot_channel);

        // Basic stats (STR, DEX, INT, LUK)
        CUserLocal::GetBasicStats(app.hProcess, 
            p.basic.str, 
            p.basic.dex, 
            p.basic.int_, 
            p.basic.luk);

        // Secondary stats
        CUserLocal::GetSecondStats(app.hProcess,
            p.secondary.attack,
            p.secondary.defense,
            p.secondary.magic,
            p.secondary.magicDef,
            p.secondary.accuracy,
            p.secondary.avoid,
            p.secondary.hands,
            p.secondary.speed,
            p.secondary.jump);

        // body states
        
        PhysicsParams::CVecCtrl CVecCtrl;
        if (CUserLocal::GetVecCtrl(app.hProcess, CVecCtrl))
        {
            auto SrcFh = MapleMap::FindFhByAddress(CVecCtrl.OnFootholdAddress);
            auto SrcLr = MapleMap::FindLrByAddress(CVecCtrl.OnRopeAddress);
            if (!SrcFh && !SrcLr) p.inAir = true;

            if(SrcLr) p.onRope = true;

            p.vx = CVecCtrl.Vx;
            p.vy = CVecCtrl.Vy;

        }

        if (p.animation == 10 || p.animation == 11)
            p.faceDown = true;


        return p;
    });

    // ====== inventory ==========

    // Get single tab
    lua.set_function("get_inventory_tab", [&app](int tabType) -> LuaInventoryTab {
        LuaInventoryTab tab;
        tab.type = tabType;
        
        static const char* names[] = { "", "Equip", "Use", "Setup", "Etc", "Cash" };
        if (tabType >= 1 && tabType <= 5)
            tab.typeName = names[tabType];
        
        if (!app.hProcess || tabType < 1 || tabType > 5)
            return tab;
        
        tab.slotCount = CUserLocal::ForEachInventoryItem(
            app.hProcess,
            static_cast<InventoryType>(tabType),
            [&](const GW_ItemSlot& item) -> bool
            {
                LuaInventoryItem luaItem;
                luaItem.slot = item.slot;
                luaItem.type = tabType;
                
                item.Get(app.hProcess, luaItem.id, luaItem.quantity, luaItem.name, luaItem.desc);
                
                if (tabType == 1)
                {
                    EquipStats stats;
                    if (item.GetEquipStats(app.hProcess, stats))
                        CopyEquipStats(stats, luaItem.stats);
                }
                
                tab.items.push_back(luaItem);
                return true;
            }
        );
        
        return tab;
    });

    // Get all tabs
    lua.set_function("get_inventory", [&app]() -> LuaInventory {
        LuaInventory inv;
        
        auto fill = [&app](LuaInventoryTab& tab, int t) {
            tab.type = t;
            static const char* names[] = { "", "Equip", "Use", "Setup", "Etc", "Cash" };
            tab.typeName = names[t];
            
            if (!app.hProcess) return;
            
            tab.slotCount = CUserLocal::ForEachInventoryItem(
                app.hProcess,
                static_cast<InventoryType>(t),
                [&](const GW_ItemSlot& item) -> bool
                {
                    LuaInventoryItem luaItem;
                    luaItem.slot = item.slot;
                    luaItem.type = t;
                    
                    item.Get(app.hProcess, luaItem.id, luaItem.quantity, luaItem.name, luaItem.desc);
                    if (t == 1)
                    {
                        EquipStats stats;
                        if (item.GetEquipStats(app.hProcess, stats))
                            CopyEquipStats(stats, luaItem.stats);
                    }
                    
                    tab.items.push_back(luaItem);
                    return true;
                }
            );
        };
        
        fill(inv.equip, 1);
        fill(inv.use,   2);
        fill(inv.setup, 3);
        fill(inv.etc,   4);
        fill(inv.cash,  5);
        
        return inv;
    });


    // ====== bind physical space ==========

    lua.set_function("get_physical_space", [&app]() -> LuaPhysicalSpace {
        LuaPhysicalSpace ps;

        if (!app.hProcess)
            return ps;

        // Bounds + map info
        CWvsPhysicalSpace2D::GetMap(
            app.hProcess,
            ps.left,
            ps.right,
            ps.top,
            ps.bottom,
            ps.mapId,
            ps.streetName,
            ps.mapName
        );

        // Footholds
        CWvsPhysicalSpace2D::ForEachFoothold(
            app.hProcess,
            [&](const CStaticFoothold& fh) -> bool
            {
                long x1 = 0, y1 = 0, x2 = 0, y2 = 0;
                long id = 0, pr = 0, ne = 0, layer=0;

                if (!fh.GetFoothold(app.hProcess, x1, y1, x2, y2, id, pr, ne, layer))
                    return true;    // skip, keep iterating

                LuaFoothold lfh;
                lfh.x1  = x1;
                lfh.y1  = y1;
                lfh.x2  = x2;
                lfh.y2  = y2;
                lfh.id  = id;
                lfh.prev = pr;
                lfh.next = ne;
                lfh.layer = layer;

                auto pfh = MapleMap::FindFhByAddress(fh.remoteAddress);
                if(pfh && pfh->p_Platform) lfh.platform=pfh->p_Platform->id;

                ps.footholds.push_back(lfh);
                return true;
            }
        );

        // Ropes (ladder + rope)
        CWvsPhysicalSpace2D::ForEachLadderRope(
            app.hProcess,
            [&](const CLadderRope& lr) -> bool
            {
                long id = 0, isLadder = 0, fromUpper = 0;
                long x = 0, y1 = 0, y2 = 0;

                if (!lr.Get(app.hProcess, id, isLadder, fromUpper, x, y1, y2))
                    return true;

                if (id <= 0)
                    return true;

                if (y1 > y2)
                    std::swap(y1, y2);

                LuaRope rope;
                rope.id        = id;
                rope.isLadder  = (isLadder != 0);
                rope.fromUpper = (fromUpper != 0);
                rope.x         = x;
                rope.y1        = y1;
                rope.y2        = y2;

                auto plr = MapleMap::FindLrByAddress(lr.remoteAddress);
                if(plr && plr->p_Platform) rope.platform=plr->p_Platform->id;

                ps.ropes.push_back(rope);
                return true;
            }
        );

        // Portals
        CWvsPhysicalSpace2D::ForEachPortal(
            app.hProcess,
            [&](const CPortal& portal) -> bool
            {
                long x = 0, y = 0, ID = 0, Type = 0;
                uint32_t toMapID = 0;
                std::string Name, toName;

                if (!portal.Get(app.hProcess, x, y, ID, Type, toMapID, Name, toName))
                    return true;

                LuaPortal lp;
                lp.x        = x;
                lp.y        = y;
                lp.id       = ID;
                lp.type     = Type;
                lp.toMapId  = toMapID;
                lp.name     = Name;
                lp.toName= toName;

                ps.portals.push_back(std::move(lp));

                return true;
            }
        );


        //platforms
        for(auto& pf : MapleMap::Platforms)
        {
            LuaPlatform lpf;
            lpf.id       = pf.id     ;
            lpf.leftX    = pf.leftX  ;
            lpf.rightX   = pf.rightX ;
            lpf.centerX  = pf.centerX;
            lpf.size     = pf.size   ;
            lpf.isRope   = pf.p_LadderRope? true : false ;
            //
            ps.platforms.push_back(lpf);
        }

        return ps;
    });



    // ====== bind drop ==========

    lua.set_function("get_drops", [&app]() {
        std::vector<DropInfo> drops;

        // Assuming you have a ForEachDrop similar to ForEachMob:
        // CDropPool::ForEachDrop(app.hProcess, [&](const CDrop& drop) -> bool { ... });

        CDropPool::ForEachDrop(
            app.hProcess,
            [&](const CDrop& drop) -> bool
            {
                std::uint32_t UID      = 0;
                std::uint32_t OwnerID  = 0;
                std::uint32_t SourceID = 0;
                std::uint32_t OwnType  = 0;
                std::uint32_t IsMeso   = 0;
                std::uint32_t ID       = 0;
                std::string name;
                long x = 0, y = 0;

                if (!drop.Get(app.hProcess,
                                UID,
                                OwnerID,
                                SourceID,
                                OwnType,
                                IsMeso,
                                ID,
                                x, y,
                                name))
                {
                    // failed to read this drop; skip but continue
                    return true;
                }

                DropInfo info;
                info.uid      = UID;
                info.ownerId  = OwnerID;
                info.sourceId = SourceID;
                info.ownType  = OwnType;
                info.isMeso   = (IsMeso != 0);
                info.id       = ID;
                info.x        = x;
                info.y        = y;
                info.name     = name;
                if(!IsMeso) info.type= int(ID/1000000);

                drops.emplace_back(std::move(info));
                return true; // keep iterating
            }
        );

        return drops;
    });



    // ====== bind other players (CUserRemote) ==========
    lua.set_function("get_other_players", [&app]() {
        std::vector<OtherPlayerInfo> players;

        if (!app.hProcess)
            return players;
        std::vector<uint32_t> party_ids;
        CUserPool::getPartyIDs(app.hProcess, party_ids);
        CUserPool::ForEachUser(
            app.hProcess,
            [&](const CUserRemote& user) -> bool
            {
                long x = 0, y = 0;
                std::string name;
                uint32_t id = 0, job = 0;

                // Re-use your CUserRemote::Get
                if (!user.Get(app.hProcess, id, job, x, y, name))
                    return true;    // skip this one, keep going

                OtherPlayerInfo p;
                p.id      = id;
                p.job     = job;
                p.x       = x;
                p.y       = y;
                p.name    = name;
                p.jobName = MapleJob::GetJobName(job);  // same as TabMap
                if(std::find(party_ids.begin(), party_ids.end(), id) != party_ids.end())
                    p.party = true;
                players.emplace_back(std::move(p));
                return true;        // continue iteration
            }
        );

        return players; // sol2: vector<OtherPlayerInfo> -> Lua array of OtherPlayer
    });

     // ====== bind Buff ==========
    lua.set_function("get_buffs", [&app]() 
    {
        std::vector<BuffInfo> buffs;

        CTemporaryStatView::ForEachTemporaryStat(
            app.hProcess,
            [&](const CTemporaryStat& stat) -> bool
            {
                std::uint32_t nType  = 0;
                std::uint32_t nID    = 0;
                std::uint32_t nSubID = 0;
                std::uint32_t tLeft  = 0;
                std::uint32_t vKey = 0;


                if (!stat.Get(app.hProcess,
                            nType,
                            nID,
                            nSubID,
                            tLeft,
                            vKey ))
                {
                    return true;  // skip but continue
                }

                BuffInfo info;
                info.nType  = nType;
                info.nID    = nID;
                info.nSubID = nSubID;
                info.tLeft  = tLeft;
                info.vKey   = vKey;


                buffs.emplace_back(std::move(info));
                return true;
            }
        );

        return buffs;
    });


    // ====== input ==========
    lua.set_function("press_key",
        [&](int vk)
        {
            CInputSystem::PressKey(app, static_cast<uint8_t>(vk));
        }
    );

    lua.set_function("release_key",
        [&](int vk)
        {
            CInputSystem::ReleaseKey(app, static_cast<uint8_t>(vk));
        }
    );

    lua.set_function("hit_key",
        [&](int vk)
        {
            CInputSystem::PressKey(app, static_cast<uint8_t>(vk));
            Sleep(20); // if you want a basic tap behavior; remove if not needed
            CInputSystem::ReleaseKey(app, static_cast<uint8_t>(vk));
        }
    );

    lua.set_function("send_key",
    sol::overload(
        [&](int vk)
        {
            CInputSystem::SendKey(app, static_cast<uint8_t>(vk));
        },
        [&](int vk, uint8_t repeat)
        {
            CInputSystem::SendKey(app, static_cast<uint8_t>(vk), repeat);
        }
    ));
    



    lua.set_function("stop_move",
        [&]()
        {
            CInputSystem::StopMove(app);
        }
    );

    lua.set_function("click",
    [&](int x, int y) -> bool
    {
        
        return CInputSystem::Click(app,x,y);

    });

    lua.set_function("double_click",
    [&](int x, int y) -> bool
    {
        
        return CInputSystem::DoubleClick(app,x,y);

    });

    
    lua.set_function("find_path",
    [&](double x, double y) -> int
    {
        auto res = static_cast<int>(MapleMap::Findpath(x, y));
        MapleMap::CurrentPath.DebugPrint();
        return res;

    });

    //move has three overloads
    // move to a location
    // move to a mob
    // move to a drop
    // each of them will different slightly
    lua.set_function("move_to",
    sol::overload(

        // --- (1) Move by x, y only applys to moving to a location ---
        [&](double x, double y)  -> bool
        {
            return MapleMap::MoveTo(nullptr, nullptr, x, y);
        },

        // --- (2) Move to Mob ---
        [&](const MobInfo& mob)  -> bool {
            
            return MapleMap::MoveTo(nullptr, nullptr, mob.x, mob.y);
        },

        // --- (3) Move to drop Item ---
        [&](const DropInfo& drop)  -> bool {
            return MapleMap:: MoveTo(nullptr, nullptr, drop.x, drop.y, PathFinding::Destination::Drop);
        },

        // --- (3) Move to portal ---
        [&](const LuaPortal& port)  -> bool {
            return MapleMap:: MoveTo(nullptr, nullptr, port.x, port.y, PathFinding::Destination::Portal);
        }

        
    ));

    // get item count in the inventory by id
    lua.set_function("get_item_count",
    [&](uint32_t id) -> size_t
    {
        return CUserLocal::GetItemCount(app.hProcess, id);
    });

    // get the first inventory slot containing this item id (0 = not found)
    lua.set_function("get_item_slot",
    [&](uint32_t id) -> size_t
    {
        return CUserLocal::GetItemSlot(app.hProcess, id);
    });

    // use_item (stripped) — network module removed
    lua.set_function("use_item",
    [&](uint32_t id) -> bool
    {
        // Network module stripped for open-source release
        return false;
    });

    // get the key of item on the mapped keyboard by id
    lua.set_function("get_item_key",
    [&](uint32_t id) -> UINT
    {   
        UINT VK=0;
        if(CFuncKeyMapped::GetVirtualKeyMappedRemote(app.hProcess, 2, id, VK))
            return VK;
        return 0;
    });

    // get the key of skill on the mapped keyboard by id
    lua.set_function("get_skill_key",
    [&](uint32_t id) -> UINT
    {   
        UINT VK=0;
        if(CFuncKeyMapped::GetVirtualKeyMappedRemote(app.hProcess, 1, id, VK))
            return VK;
        return 0;
    });

    lua.set_function("get_virtual_key",
    [&](uint8_t type, uint32_t data) -> UINT
    {   
        UINT VK=0;
        if(CFuncKeyMapped::GetVirtualKeyMappedRemote(app.hProcess, type, data, VK))
            return VK;
        return 0;
    });

    lua.set_function("set_key_mapping",
    [&](UINT vk, uint8_t type, uint32_t data) -> bool
    {   

        //clear blocks
        int count = 4;
        while(count>0 && CWndMan::CanWeDoInput(app.hProcess)==false)
        {
            CInputSystem::SendKey(app,VK_ESCAPE);
            count--;
            Sleep(200);
        }
        if(!CWndMan::CanWeDoInput(app.hProcess)) return false;

        //read key UI key
        UINT sKey = 0;
        if(!CFuncKeyMapped::GetVirtualKeyMappedRemote(app.hProcess, 4, 9, sKey)) return false;

        //set key
        if(!CFuncKeyMapped::SetVirtualKeyMappedRemote(app.hProcess, vk, type, data)) return false;
        
        
        //flash UI
        CInputSystem::SendKey(app,sKey);
        Sleep(200);
        CInputSystem::SendKey(app,sKey);


        return true;
    });


    lua.set_function("dump_key_map",
    [&]()
    {   
        CFuncKeyMapped::DumpFuncKeyMapRemote(app.hProcess);
    });


    lua.set_function("get_arrow_key_states",
    [&]() -> std::tuple<bool, bool, bool, bool>
    {
        // Arrow hook stripped for open-source release
        return std::make_tuple(false, false, false, false);
    });

    lua.set_function("get_travel_portal",
    [&](int32_t toMapId) -> std::tuple<int, LuaPortal>
    {
        auto pf = pathfinder::GetMapPathfinder();

        pathfinder::PathStep step;
        auto res = pf.GetNextPortal(toMapId, step);
        
        LuaPortal lp;
        if(res<0) return std::make_tuple(res, lp);

        lp.x = step.portalX;
        lp.y = step.portalY;
        lp.toMapId = step.toMapId;
        lp.name = step.portalName;


        //match portal in memory
        CWvsPhysicalSpace2D::ForEachPortal(
            app.hProcess,
            [&](const CPortal& portal) -> bool
            {
                long x = 0, y = 0, ID = 0, Type = 0;
                uint32_t toMapID = 0;
                std::string Name, toName;

                if (!portal.Get(app.hProcess, x, y, ID, Type, toMapID, Name, toName))
                    return true;

                if( step.portalName==Name)
                {
                    lp.x        = x;
                    lp.y        = y;
                    lp.id       = ID;
                    lp.type     = Type;
                    lp.toMapId  = toMapID;
                    lp.name     = Name;
                    lp.toName= toName;
                }
                
                return true;
            }
        );


        return std::make_tuple(res, lp);

        
    });


    lua.set_function("play_alert",
    sol::overload(
        [&]()
        {
            PlayEmbeddedSoundA(1);
        },
        [&](const std::string& message)
        {
            PlayEmbeddedSoundA(1);
            if (ShouldSendDiscord(message))
            {
                auto& bot = DiscordBot::Instance();
                if (bot.GetStatus() == DiscordStatus::Connected)
                    bot.SendEmbed("⚠️ Alert", message, 0xFF0000);
            }
        }
    ));

    lua.set_function("play_notify",
    sol::overload(
        [&]()
        {
            PlayEmbeddedSoundA(2);
        },
        [&](const std::string& message)
        {
            PlayEmbeddedSoundA(2);
            if (ShouldSendDiscord(message))
            {
                auto& bot = DiscordBot::Instance();
                if (bot.GetStatus() == DiscordStatus::Connected)
                    bot.SendEmbed("🔔 Notify", message, 0x00AAFF);
            }
        }
    ));

    lua.set_function("is_input_allowed",
    [&]() -> bool
    {
        return CWndMan::CanWeDoInput(app.hProcess);
    });

    lua.set_function("reset_focus",
    [&]()
    {
        return CWndMan::resetFocus(app.hProcess);
    });

    CWndMan::resetFocus(app.hProcess);


    lua.set_function("get_debuffs",
    [&]() -> std::string
    {
        return CUserLocal::GetDebuffs(app.hProcess);
    });


    lua.set_function("change_channel",
    [&](uint32_t channel) -> bool
    {
        return CWndMan::ChangeChannel(app.hProcess, channel);
    });

    lua.set_function("buy_item",
    [&](int32_t ID, int32_t Qty) -> bool
    {
        return CShopDlg::BuySelectedItem(app, ID, Qty);
    });

    lua.set_function("sell_item",
    [&](int32_t tab, int32_t index) -> bool
    {
        return CShopDlg::SellSelectedItem(app, tab-1, index);
    });


    lua.set_function("sell_all_item",
    [&](int32_t tab,std::vector<int32_t> excludeIDs ) -> bool
    {
        return CShopDlg::SellAllItem(app, tab-1, excludeIDs);
    });

    lua.set_function("open_shop",
    [&]() -> bool
    {
        return CShopDlg::OpenShop(app.hProcess);
    });

    // get_chatlog() function
    lua.set_function("get_chatlog", [&app]() {
        std::vector<ChatLogInfo> logs;

        CUIStatusBar::ForEachChatLog(
            app.hProcess,
            [&](int32_t index, const ChatLogEntry& entry) -> bool
            {
                ChatLogInfo info;
                info.index   = entry.index;
                info.type    = entry.type;
                info.content = entry.content;

                logs.emplace_back(std::move(info));
                return true; // keep iterating
            }
        );

        return logs;
    });


    lua.set_function("get_dialog_text", [&app]() {
        std::vector<DialogTextEntry> entries;
        
        auto texts = CWndMan::GetUtilDlgText(app.hProcess);
        for (auto& item : texts)
        {
            DialogTextEntry entry;
            entry.type = item.nType;
            entry.select = item.nSelect;
            entry.text = std::move(item.text);
            
            entries.emplace_back(std::move(entry));
        }
        
        return entries;
    });


    lua.set_function("do_dialog_selection", [&app](int selection) {
        return CWndMan::SetUtilDlgExSelection(app.hProcess, selection);
    });

    // HP = 0,MP = 1,STR = 2,DEX = 3,INT = 4,LUK = 5
    lua.set_function("add_ap", [&app](const std::string& stat) {
        static const std::unordered_map<std::string, StatType> statMap = {
            {"hp",  static_cast<StatType>(0)},
            {"mp",  static_cast<StatType>(1)},
            {"str", static_cast<StatType>(2)},
            {"dex", static_cast<StatType>(3)},
            {"int", static_cast<StatType>(4)},
            {"luk", static_cast<StatType>(5)},
        };
        
        auto it = statMap.find(stat);
        if (it == statMap.end())
            return false;
        
        return CWndMan::FocusStatApButton(app.hProcess, it->second);
    });

    lua.set_function("add_sp", [&app](int id) {
        return CWndMan::FocusSkillUpButton(app.hProcess, id);
    });


    lua.set_function("revive_dead", [&app]() -> bool
    {
        return CWndMan::ReviveDead(app.hProcess);
    });





            


    // sleep(ms) with early cancellation
    lua.set_function("sleep", [&app](int ms) {
        if (ms < 0) ms = 0;
        const int chunk = (std::min)(ms,50);
        while (ms > 0) {
            if (app.luaCancelRequested.load(std::memory_order_relaxed))
                throw sol::error("Lua script cancelled during sleep");
            std::this_thread::sleep_for(std::chrono::milliseconds(chunk));
            ms -= chunk;
        }
    });

    // optional: Cooperative cancel optional
    lua.set_function("check_cancel", [&app]() {
        if (app.luaCancelRequested.load(std::memory_order_relaxed)) {
            throw sol::error("Lua script cancelled by user");
        }
    });


    // Additional helper
    lua.set_function("log_info", [&app](const std::string& msg) {
        app.logger.Add("[LuaInfo] " + msg);
    });


    // ---- Image matching ----

    // find_image("path.bmp") -> table {x, y, confidence} or nil
    // x, y = center of matched region in game window client coordinates
    lua.set_function("find_image", [&app, &lua](const std::string& path) -> sol::object {
        if (!app.hGameWnd) {
            app.logger.AddE("[ImageMatch] No game window attached");
            return sol::nil;
        }

        auto tmpl = ImageMatch::LoadBMP(path);
        if (!tmpl.IsValid()) {
            app.logger.AddE("[ImageMatch] Failed to load template: " + path);
            return sol::nil;
        }

        float threshold = static_cast<float>(Settings::Instance().GetInt("image_match.threshold"));
        auto result = ImageMatch::FindInWindow(app.hGameWnd, tmpl, threshold);

        if (result.confidence < 0) {
            return sol::nil;
        }

        sol::table t = lua.create_table();
        t["x"] = result.x + tmpl.width / 2;
        t["y"] = result.y + tmpl.height / 2;
        t["confidence"] = result.confidence;
        return t;
    });


}