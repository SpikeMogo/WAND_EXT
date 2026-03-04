// LuaBindings.h
#pragma once

#include <cstdint>   // for std::uint32_t
#include <string>    // for std::string
#include <vector>
struct AppState;        // forward declare
class ScriptEngine;     // forward declare



struct MobInfo {
    uint32_t templateId;
    int      x;
    int      y;
    std::string name;
    int HP;
    int maxHP;
    int Platform;
};


struct NpcInfo {
    uint32_t    templateId = 0;
    int         x  = 0;
    int         y  = 0;
    int         xp = 0;
    int         yp = 0;
    std::string name;
};


struct BasicStats {
    uint32_t str = 0;
    uint32_t dex = 0;
    uint32_t int_ = 0;  // 'int' is reserved keyword
    uint32_t luk = 0;
};

struct SecondaryStats {
    uint32_t attack = 0;
    uint32_t defense = 0;
    uint32_t magic = 0;
    uint32_t magicDef = 0;
    uint32_t accuracy = 0;
    uint32_t avoid = 0;
    uint32_t hands = 0;
    uint32_t speed = 0;
    uint32_t jump = 0;
};

struct PlayerInfo {
    // Position
    long x = 0;
    long y = 0;

    double vx=0;
    double vy=0;

    // Core Stats
    long hp = 0;
    long mp = 0;
    long maxHp = 0;
    long maxMp = 0;
    long exp = 0;
    double expPer = 0.0;
    int32_t mesos = 0;
    uint8_t level = 0;
    uint16_t job = 0;

    // Status
    long attackCount = 0;
    long breath = 0;
    long animation = 0;
    long comboCount = 0;
    long faceDir = 0;
    long UID = 0;

    //other 
    uint32_t channel = 0;
    uint32_t tot_channel = 0;

    // Nested stats
    BasicStats basic;
    SecondaryStats secondary;

    //body
    bool inAir = false;
    bool onRope = false;
    bool faceDown = false;
};


struct DropInfo {
    std::uint32_t uid      = 0;  // UID
    std::uint32_t ownerId  = 0;  // OwnerID
    std::uint32_t sourceId = 0;  // SourceID
    std::uint32_t ownType  = 0;  // OwnType
    bool          isMeso   = false;
    std::uint32_t id       = 0;  // item ID (or meso amount / ID)
    long          x        = 0;
    long          y        = 0;
    std::string   name;
    std::uint32_t type     = 0;
};


struct LuaEquipStats {
    int16_t str = 0;
    int16_t dex = 0;
    int16_t int_ = 0;
    int16_t luk = 0;
    int16_t maxHp = 0;
    int16_t maxMp = 0;
    int16_t attack = 0;
    int16_t magic = 0;
    int16_t defense = 0;
    int16_t magicDef = 0;
    int16_t accuracy = 0;
    int16_t avoid = 0;
    int16_t hands= 0;
    int16_t speed = 0;
    int16_t jump = 0;
    uint8_t slots = 0;
};

struct LuaInventoryItem {
    uint32_t id = 0;
    uint16_t quantity = 0;
    size_t slot = 0;
    int type = 0;
    std::string name;
    std::string desc;
    LuaEquipStats stats;
};

struct LuaInventoryTab {
    int type = 0;
    std::string typeName;
    std::vector<LuaInventoryItem> items;
    size_t slotCount = 0;
};

struct LuaInventory {
    LuaInventoryTab equip;
    LuaInventoryTab use;
    LuaInventoryTab setup;
    LuaInventoryTab etc;
    LuaInventoryTab cash;
};



// ---- Physical space Lua structs ----

struct LuaFoothold {
    long x1  = 0;
    long y1  = 0;
    long x2  = 0;
    long y2  = 0;
    long id  = 0;
    long prev = 0;   // previous foothold id
    long next = 0;   // next foothold id
    long layer = 0;
    long platform = 0;

};

struct LuaRope {
    long id        = 0;
    bool isLadder  = false;  // true = ladder, false = rope
    bool fromUpper = false;  // same semantics as CLadderRope::fromUpper (non-zero)
    long x         = 0;
    long y1        = 0;
    long y2        = 0;
    long platform = 0;
};

struct LuaPortal {
    long      x        = 0;
    long      y        = 0;
    long      id       = 0;
    long      type     = 0;
    uint32_t  toMapId  = 0;
    std::string name;
    std::string toName;
};

struct LuaPlatform {
    long id = 0;
    long leftX = 0;
    long rightX = 0;
    long centerX = 0;
    long size = 0;
    bool isRope = false;
};


struct LuaPhysicalSpace {
    // bounds
    long      left   = 0;
    long      right  = 0;
    long      top    = 0;
    long      bottom = 0;

    uint32_t  mapId       = 0;
    std::string streetName;
    std::string mapName;

    std::vector<LuaFoothold> footholds;
    std::vector<LuaRope>     ropes;     // ladders & ropes
    std::vector<LuaPortal>   portals;
    std::vector<LuaPlatform> platforms;
};


struct OtherPlayerInfo {
    uint32_t    id      = 0;
    uint32_t    job     = 0;
    long        x       = 0;
    long        y       = 0;
    std::string name;
    std::string jobName;
    bool        party = false;
};

struct BuffInfo {
    std::uint32_t nType  = 0;  // Type of buff
    std::uint32_t nID    = 0;  // Skill ID
    std::uint32_t nSubID = 0;  // Sub ID
    std::uint32_t  tLeft  = 0;  // Time left (ms)
    std::uint32_t vKey = 0;
};


struct ChatLogInfo
{
    int32_t     index;
    int32_t     type;
    std::string content;
};

struct DialogTextEntry
{
    int type;
    int select;
    std::string text;
};

struct PetInfo
{
    int         id       = -1;   // pet slot index (0/1/2)
    std::string name;
    int         fullness = 0;    // repleteness
};

// Call this once at startup to wire Lua + app together
void SetupLuaEnvironment(AppState& app, ScriptEngine& engine);
void SetupLuaFunctions(AppState& app, ScriptEngine& engine);
void SetupLuaTypes(AppState& app, ScriptEngine& engine);


