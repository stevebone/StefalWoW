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

#include "Followship_bots_roles.h"


// Spell Related
static constexpr uint32 BOT_GCD_MS = 1500;

struct DrinkManaPerLevel
{
    uint8 minLevel;
    uint8 maxLevel;
    int32 mana;
};

// min level, max level, value
static constexpr DrinkManaPerLevel DrinkManaRegenTable[] =
{
    { 1, 10,  10 }, // level 1-10
    { 11, 20, 25 }, // level 11-20
    { 21, 30, 45 },
    { 31, 40, 90 },
    { 41, 50, 120 },
    { 51, 60, 220 },
};

// Mana Potion Spells
struct ManaPotionSpell
{
    uint8 minLevel;
    uint8 maxLevel;
    uint32 spellId;
};

static constexpr ManaPotionSpell ManaPotionTable[] =
{
    {  1,  15,  437 },   // Minor Mana Potion 120
    { 16,  29,  438 },   // Lesser Mana Potion 159
    { 30,  40,  17530 }, // Superior Mana Potion 494
    { 41,  50,  17531 }, // Major Mana Potion 512
    { 51,  60,  28499 }, // Super Mana Potion 682
    { 61,  80,  43186 } // Runic Mana Potion 956
};


enum class FSBSpellType
{
    Emergency,
    Heal,
    Buff,
    Damage
};

// Struct for spell info (could mirror your FSBotSpells)
struct FSBSpellCandidate
{
    uint32 spellId = 0;
    FSBSpellType type = FSBSpellType::Damage;
    float priority = 0.0f;      // higher = more important
    float hpThreshold = 100.f;  // optional
    float chance = 100.f;       // 0-100%
    float dist = 0.0f;          // desired distance to target
    bool isSelfCast = false;    // cast on self
    uint32 cooldownMs = 0;      // static cooldown duration
    uint32 allowedRoles = FSB_ROLEMASK_ANY;
    uint32 nextReadyMs = 0;     // dynamic, updated at runtime
};

namespace FSBUtilsSpells
{
    // Gets drink amount of mana regen per level
    int32 GetDrinkManaRegenPerLevel(uint8 level, DrinkManaPerLevel const* table, size_t tableSize);

    // Wrapper for GetDrinkManaRegenPerLevel
    int32 GetDrinkManaRegen(uint16 level);

    // Gets mana potion spell per level
    uint32 GetManaPotionSpellForLevel(uint8 level);

    /// Returns the English name of a spell by its ID, or "unknown spell" if not found.
    std::string GetSpellName(uint32 spellId);

    // Checks global cooldown
    bool CanCastNow(Unit* me, uint32 now, uint32 globalCooldownUntil);

    /// Returns true if the unit has a debuff that is dispellable by the bot (magic, disease, poison)
    bool HasDispellableDebuff(Unit* unit);

    // Returns the best candidate spell for the given unit/target/context
    std::optional<FSBSpellCandidate> SelectSpell(
        Unit* caster,
        Unit* target,
        const std::vector<FSBSpellCandidate>& spellTable,
        FSBSpellType typeFilter,
        bool isSelfCast = false);

    // ------------------------------
    // Select best spell candidate
    // ------------------------------
    /// Selects the best spell for a caster/target based on type, role, self-cast preference, etc.
    FSBSpellCandidate* SelectSpell(Unit* caster, Unit* target, std::vector<FSBSpellCandidate>& spells,
        FSBSpellType type, bool preferSelfCast = false);

    bool TryCast(Unit* caster, Unit* target, FSBSpellCandidate* spell, uint32& globalCooldownUntil);

    /// Random float generator in [min,max]
    float Frand(float min, float max);

    /// Returns true if spell cooldown is ready
    bool IsSpellReady(const FSBSpellCandidate& spell, uint32 now);

    /// Trigger spell cooldown
    void TriggerCooldown(FSBSpellCandidate& spell, uint32 now);

    // Mount Spells
    using MountSpellList = std::vector<uint32>;
    using MountLevelSpellMap = std::map<uint8, MountSpellList>;
    // Data access
    MountSpellList const* GetMountSpellsForLevel(uint8 level);

    uint32 GetRandomMountSpellForBot(Creature* me);
    void CastRandomMountLevelSpell(Creature* me);
}

enum FSB_StandardGroundMounts
{
    SPELL_MOUNT_PINTO = 472,
    SPELL_MOUNT_STALLION = 470,
    SPELL_MOUNT_CHESTNUT = 6648,
    SPELL_MOUNT_BROWN_HORSE = 458,

    SPELL_MOUNT_PALOMINO = 23227,
    SPELL_MOUNT_WHITE_STEED = 23228,
    SPELL_MOUNT_BROWN_STEED = 23229
};

using MountSpellList = std::vector<uint32>;
using MountLevelSpellMap = std::map<uint8, MountSpellList>;

// Example data
static MountLevelSpellMap MountSpellsByLevel =
{
    { 10, { SPELL_MOUNT_STALLION, SPELL_MOUNT_PINTO,SPELL_MOUNT_CHESTNUT,SPELL_MOUNT_BROWN_HORSE } },              // Standard ground mounts
    { 20, { SPELL_MOUNT_PALOMINO, SPELL_MOUNT_WHITE_STEED, SPELL_MOUNT_BROWN_STEED } },            // Epic ground mounts
    //{ 30, { 2120, 120 } },             // 30-39
    //{ 40, { 11426, 10 } },             // 40-49
};
