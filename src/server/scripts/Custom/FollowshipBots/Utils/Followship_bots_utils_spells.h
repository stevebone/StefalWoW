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

}


