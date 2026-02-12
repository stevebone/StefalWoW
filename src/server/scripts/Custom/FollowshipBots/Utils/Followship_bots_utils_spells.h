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


// Spell Related
static constexpr uint32 BOT_GCD_MS = 1500;



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


enum class FSBSpellType
{
    Emergency,
    Heal,
    Buff,
    Damage
};


// Struct for spell info (could mirror your FSBotSpells)
struct FSBSpellDefinition
{
    uint32 spellId = 0;
    FSBSpellType type = FSBSpellType::Damage;
    float manaCostOverride = 0.0f;      // for spells that do not have a dbc value
    float hpThreshold = 100.f;  // optional
    float chance = 100.f;       // 0-100%
    float dist = 0.0f;          // desired distance to target
    bool isSelfCast = false;    // cast on self
    uint32 cooldownMs = 0;      // static cooldown duration
    uint32 allowedRoles = FSB_ROLEMASK_ANY;
    //uint32 nextReadyMs = 0;     // dynamic, updated at runtime
};

struct FSBSpellRuntime
{
    FSBSpellDefinition const* def; // pointer to static definition
    uint32 nextReadyMs = 0; // runtime cooldown
};

using FSBSpellTable = std::vector<FSBSpellDefinition>;

namespace FSBUtilsSpells
{
    

    

    // Checks global cooldown
    bool CanCastNow(Unit* me, uint32 now, uint32 globalCooldownUntil);

    bool HasPositiveDebuff(Unit* unit);

    /// Returns true if the unit has a debuff that is dispellable by the bot (magic, disease, poison)
    bool HasDispellableDebuff(Unit* unit);

    bool IsSpellClassValid(Creature* bot, uint32 spellId, Unit* target);

    
    Unit* FindBotDeadResTarget(Creature* bot, std::vector<Unit*> const& botGroup_);

}

namespace FSBUtilsCombatSpells
{
    void InitBotSpellTables();
    void InitSpellRuntime(Creature* bot, std::vector<FSBSpellRuntime>& _runtimeSpells);

    FSBSpellTable const* GetBotSpellTableForClass(FSB_Class botClass);

    std::vector<FSBSpellRuntime*> BotGetAvailableSpells(Creature* bot, std::vector<FSBSpellRuntime>& runtimeSpells, uint32& globalCooldownUntil);
    FSBSpellRuntime* BotSelectSpell(Creature* bot, std::vector<FSBSpellRuntime*>& availableSpells, std::vector<Unit*> botGroup_, Unit*& outTarget);
    void BotCastSpell(Creature* bot, Unit* target, FSBSpellRuntime* runtime, uint32& globalCooldownUntil);
}

enum FSB_StandardGroundMounts
{
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
            { 10, { /* tiger mounts */ } },
            { 20, { /* epic tiger mounts */ } }
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

namespace FSBSpellsUtils
{
    // Gets mana potion spell per level
    uint32 GetManaPotionSpellForLevel(uint8 level);
    // Gets hp potion spell per level
    uint32 GetHealthPotionSpellForLevel(uint8 level);

    /// Returns the English name of a spell by its ID, or "unknown spell" if not found.
    std::string GetSpellName(uint32 spellId);

    const MountSpellList* GetMountSpellsForBot(FSB_Race race, uint8 level);
    uint32 GetRandomMountSpellForBot(Creature* bot);
    bool CastRandomMountLevelSpell(Creature* bot);
    bool BotCastMountSpell(Creature* bot, uint32 spellId);
}
