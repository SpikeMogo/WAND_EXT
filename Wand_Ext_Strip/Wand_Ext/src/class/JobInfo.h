#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

namespace MapleJob
{
    enum Job : uint16_t
    {
        BEGINNER = 0,
        WARRIOR = 100,
        FIGHTER = 110,
        CRUSADER = 111,
        HERO = 112,
        PAGE = 120,
        WHITEKNIGHT = 121,
        PALADIN = 122,
        SPEARMAN = 130,
        DRAGONKNIGHT = 131,
        DARKKNIGHT = 132,

        MAGICIAN = 200,
        FP_WIZARD = 210,
        FP_MAGE = 211,
        FP_ARCHMAGE = 212,
        IL_WIZARD = 220,
        IL_MAGE = 221,
        IL_ARCHMAGE = 222,
        CLERIC = 230,
        PRIEST = 231,
        BISHOP = 232,

        BOWMAN = 300,
        HUNTER = 310,
        RANGER = 311,
        BOWMASTER = 312,
        CROSSBOWMAN = 320,
        SNIPER = 321,
        CROSSBOWMASTER = 322,

        THIEF = 400,
        ASSASSIN = 410,
        HERMIT = 411,
        NIGHTLORD = 412,
        BANDIT = 420,
        CHIEFBANDIT = 421,
        SHADOWER = 422,

        PIRATE = 500,
        BRAWLER = 510,
        GUNSLINGER = 520,
        MARAUDER = 511,
        OUTLAW = 521,
        BUCCANEER = 512,
        CORSAIR = 522,

        MAPLELEAF_BRIGADIER = 800,
        GM = 900,
        SUPERGM = 910,

        NOBLESSE = 1000,
        DAWNWARRIOR1 = 1100,
        DAWNWARRIOR2 = 1110,
        DAWNWARRIOR3 = 1111,
        DAWNWARRIOR4 = 1112,
        BLAZEWIZARD1 = 1200,
        BLAZEWIZARD2 = 1210,
        BLAZEWIZARD3 = 1211,
        BLAZEWIZARD4 = 1212,
        WINDARCHER1 = 1300,
        WINDARCHER2 = 1310,
        WINDARCHER3 = 1311,
        WINDARCHER4 = 1312,
        NIGHTWALKER1 = 1400,
        NIGHTWALKER2 = 1410,
        NIGHTWALKER3 = 1411,
        NIGHTWALKER4 = 1412,
        THUNDERBREAKER1 = 1500,
        THUNDERBREAKER2 = 1510,
        THUNDERBREAKER3 = 1511,
        THUNDERBREAKER4 = 1512,

        LEGEND = 2000,
        ARAN2 = 2100,
        ARAN3 = 2110,
        ARAN4 = 2111,
        ARAN5 = 2112
    };

    inline const std::unordered_map<uint16_t, const char*> jobNames = {
        {BEGINNER, "Beginner"}, {WARRIOR, "Warrior"}, {FIGHTER, "Fighter"},
        {CRUSADER, "Crusader"}, {HERO, "Hero"}, {PAGE, "Page"},
        {WHITEKNIGHT, "White Knight"}, {PALADIN, "Paladin"},
        {SPEARMAN, "Spearman"}, {DRAGONKNIGHT, "Dragon Knight"},
        {DARKKNIGHT, "Dark Knight"},

        {MAGICIAN, "Magician"}, {FP_WIZARD, "F/P Wizard"},
        {FP_MAGE, "F/P Mage"}, {FP_ARCHMAGE, "F/P Arch Mage"},
        {IL_WIZARD, "I/L Wizard"}, {IL_MAGE, "I/L Mage"},
        {IL_ARCHMAGE, "I/L Arch Mage"}, {CLERIC, "Cleric"},
        {PRIEST, "Priest"}, {BISHOP, "Bishop"},

        {BOWMAN, "Bowman"}, {HUNTER, "Hunter"}, {RANGER, "Ranger"},
        {BOWMASTER, "Bowmaster"}, {CROSSBOWMAN, "Crossbowman"},
        {SNIPER, "Sniper"}, {CROSSBOWMASTER, "Crossbow Master"},

        {THIEF, "Thief"}, {ASSASSIN, "Assassin"}, {HERMIT, "Hermit"},
        {NIGHTLORD, "Night Lord"}, {BANDIT, "Bandit"},
        {CHIEFBANDIT, "Chief Bandit"}, {SHADOWER, "Shadower"},

        {PIRATE, "Pirate"}, {BRAWLER, "Brawler"}, {GUNSLINGER, "Gunslinger"},
        {MARAUDER, "Marauder"}, {OUTLAW, "Outlaw"},
        {BUCCANEER, "Buccaneer"}, {CORSAIR, "Corsair"},

        {MAPLELEAF_BRIGADIER, "Maple Brigadier"},
        {GM, "GM"}, {SUPERGM, "Super GM"},

        {NOBLESSE, "Noblesse"},
        {DAWNWARRIOR1, "Dawn Warrior 1"}, {DAWNWARRIOR2, "Dawn Warrior 2"},
        {DAWNWARRIOR3, "Dawn Warrior 3"}, {DAWNWARRIOR4, "Dawn Warrior 4"},
        {BLAZEWIZARD1, "Blaze Wizard 1"}, {BLAZEWIZARD2, "Blaze Wizard 2"},
        {BLAZEWIZARD3, "Blaze Wizard 3"}, {BLAZEWIZARD4, "Blaze Wizard 4"},

        {WINDARCHER1, "Wind Archer 1"}, {WINDARCHER2, "Wind Archer 2"},
        {WINDARCHER3, "Wind Archer 3"}, {WINDARCHER4, "Wind Archer 4"},

        {NIGHTWALKER1, "Night Walker 1"}, {NIGHTWALKER2, "Night Walker 2"},
        {NIGHTWALKER3, "Night Walker 3"}, {NIGHTWALKER4, "Night Walker 4"},

        {THUNDERBREAKER1, "Thunder Breaker 1"},
        {THUNDERBREAKER2, "Thunder Breaker 2"},
        {THUNDERBREAKER3, "Thunder Breaker 3"},
        {THUNDERBREAKER4, "Thunder Breaker 4"},

        {LEGEND, "Legend"}, {ARAN2, "Aran 2"}, {ARAN3, "Aran 3"},
        {ARAN4, "Aran 4"}, {ARAN5, "Aran 5"}
    };

    inline std::string GetJobName(uint16_t jobId)
    {
        auto it = jobNames.find(jobId);
        if (it != jobNames.end())
            return it->second;
        return "Unknown";
    }
}
