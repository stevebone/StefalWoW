#pragma once

#include <optional>
#include <string>
#include <vector>
#include <random>

#include "Player.h"
#include "ScriptMgr.h"
#include "SpellMgr.h"
#include "Unit.h"
#include "SpellAuraDefines.h"

#include "Followship_bots_defines.h"
#include "Followship_bots_utils_stats.h"

//#include "Followship_bots_spells_handler.h"

// Mana Potion Spells
struct PotionSpell
{
    uint8 minLevel;
    uint8 maxLevel;
    uint32 spellId;
};

static constexpr PotionSpell ManaPotionTable[] =
{
    {  1,  15,  437 },   // Minor Mana Potion 120
    { 16,  29,  438 },   // Lesser Mana Potion 159
    { 30,  40,  17530 }, // Superior Mana Potion 494
    { 41,  50,  17531 }, // Major Mana Potion 512
    { 51,  60,  28499 }, // Super Mana Potion 682
    { 61,  80,  43186 } // Runic Mana Potion 956
};

static constexpr PotionSpell HealthPotionTable[] =
{
    {  1,  15,  439 },   // Minor HP Potion 120
    { 16,  29,  440 },   // Lesser HP Potion 159
    { 30,  40,  4042 }, // Superior HP Potion 494
    { 41,  50,  17534 }, // Major HP Potion 512
    { 51,  60,  28495 }, // Super HP Potion 682
    { 61,  80,  43185 } // Runic HP Potion 956
};

//extern std::unordered_map<FSB_Class, FSBSpellTable const*> sBotSpellTables;

namespace FSBUtilsSpells
{

    bool IsSpellClassValid(Creature* bot, uint32 spellId, Unit* target);

}

enum FSB_StandardGroundMounts
{
    // N Elf
    SPELL_MOUNT_STRIPED_NIGHTSABER = 10793,
    SPELL_MOUNT_STRIPED_FROSTSABER = 8394,
    SPELL_MOUNT_STRIPED_DAWNSABER = 66847,

    SPELL_MOUNT_SWITFT_STORMSABER = 23338,
    SPELL_MOUNT_SWITFT_FROSTSABER = 18766,
    SPELL_MOUNT_SWITFT_MISTSABER = 18767,

    // Human
    SPELL_MOUNT_PINTO = 472,
    SPELL_MOUNT_STALLION = 470,
    SPELL_MOUNT_CHESTNUT = 6648,
    SPELL_MOUNT_BROWN_HORSE = 458,

    SPELL_MOUNT_PALOMINO = 23227,
    SPELL_MOUNT_WHITE_STEED = 23228,
    SPELL_MOUNT_BROWN_STEED = 23229,

    // Draenei
    SPELL_MOUNT_BROWN_ELEKK = 34406,
    SPELL_MOUNT_GRAY_ELEKK = 35710,
    SPELL_MOUNT_PURPLE_ELEKK = 35711,

    SPELL_MOUNT_GREAT_GREEN_ELEKK = 35712,
    SPELL_MOUNT_GREAT_BLUE_ELEKK = 35713,
    SPELL_MOUNT_GREAT_PURPLE_ELEKK = 35714
};

using MountSpellList = std::vector<uint32>;
using MountRaceLevelMap = std::map<uint8 /*level*/, MountSpellList>;
using MountRaceMap = std::map<FSB_Race /*race*/, MountRaceLevelMap>;

static MountRaceMap MountSpells =
{
    {
        FSB_Race::Human,
        {
            { 10, { SPELL_MOUNT_STALLION, SPELL_MOUNT_PINTO, SPELL_MOUNT_CHESTNUT, SPELL_MOUNT_BROWN_HORSE } },
            { 20, { SPELL_MOUNT_PALOMINO, SPELL_MOUNT_WHITE_STEED, SPELL_MOUNT_BROWN_STEED } }
        }
    },

    {
        FSB_Race::Draenei,
        {
            { 10, { SPELL_MOUNT_BROWN_ELEKK, SPELL_MOUNT_GRAY_ELEKK, SPELL_MOUNT_PURPLE_ELEKK } },
            { 20, { SPELL_MOUNT_GREAT_GREEN_ELEKK, SPELL_MOUNT_GREAT_BLUE_ELEKK, SPELL_MOUNT_GREAT_PURPLE_ELEKK } }
        }
    },
    
    {
        FSB_Race::NightElf,
        {
            { 10, { SPELL_MOUNT_STRIPED_NIGHTSABER, SPELL_MOUNT_STRIPED_FROSTSABER, SPELL_MOUNT_STRIPED_DAWNSABER } },
            { 20, { SPELL_MOUNT_SWITFT_STORMSABER, SPELL_MOUNT_SWITFT_FROSTSABER, SPELL_MOUNT_SWITFT_MISTSABER } }
        }
    },

    {
        FSB_Race::Dwarf,
        {
            { 10, { /* ram mounts */ } },
            { 20, { /* epic ram mounts */ } }
        }
    }
};

struct DispelAbility
{
    uint32 spellId;
    std::vector<DispelType> dispels;
    uint32 cooldown;
};

extern std::unordered_map<FSB_Class, DispelAbility> DispelTable;

enum OffensiveDispelType
{
    OFFDISPEL_MAGIC,
    OFFDISPEL_STEAL
};

struct OffensiveDispelAbility
{
    uint32 spellId;
    OffensiveDispelType type;
};

extern std::unordered_map<FSB_Class, OffensiveDispelAbility> OffensiveDispelTable;

namespace FSBSpellsUtils
{
    bool IsSpellReady(uint32 spellId);
    void PutSpellOnCooldown(uint32 spellId, uint32 cooldownMs);

    bool CanCastNow(Unit* me, uint32 now, uint32 globalCooldownUntil);
    bool BotHasHealSpells(Creature* bot);
    bool BotHasHealSpellsForSelf(Creature* bot);

    DispelType ConvertAuraToDispelType(Aura* aura);

    // Gets mana potion spell per level
    uint32 GetManaPotionSpellForLevel(uint8 level);
    // Gets hp potion spell per level
    uint32 GetHealthPotionSpellForLevel(uint8 level);

    /// Returns the English name of a spell by its ID, or "unknown spell" if not found.
    std::string GetSpellName(uint32 spellId);

    const MountSpellList* GetMountSpellsForBot(FSB_Race race, uint8 level);
    uint32 GetRandomMountSpellForBot(Creature* bot);
    bool CastRandomMountLevelSpell(Creature* bot);

    Aura* FindEnemyBuffToDispel(Unit* enemy);
}

