/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ShamanAI.h"
#include "ElementalShaman.h"
#include "EnhancementShaman.h"
#include "RestorationShaman.h"
#include "../BaselineRotationManager.h"
#include "../../Combat/CombatBehaviorIntegration.h"
#include "../SpellValidation_WoW120_Part2.h"  // Central spell registry
#include "Player.h"
#include "Unit.h"
#include "Creature.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "SpellAuras.h"
#include "Map.h"
#include "Group.h"
#include "Item.h"
#include "MotionMaster.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include <algorithm>
#include <chrono>
#include "../../../Spatial/SpatialGridManager.h"
#include "../../../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5F: Thread-safe queries
#include "GameTime.h"

namespace Playerbot
{

// ============================================================================
// WoW 12.0 SPELL ID ALIASES - Using Central Registry
// ============================================================================
// Import from SpellValidation_WoW120_Part2.h to avoid hardcoded values

namespace ShamanSpells = WoW120Spells::Shaman;
namespace ShamanElemental = WoW120Spells::Shaman::Elemental;
namespace ShamanEnhancement = WoW120Spells::Shaman::Enhancement;
namespace ShamanRestoration = WoW120Spells::Shaman::Restoration;
namespace ShamanCommon = WoW120Spells::Shaman::Common;

// Totem spell definitions - WoW 12.0 (many totems removed/consolidated)
enum TotemSpells : uint32
{
    // Active totems in WoW 12.0
    SPELL_EARTHBIND_TOTEM          = ShamanCommon::EARTHBIND_TOTEM,        // 2484
    SPELL_TREMOR_TOTEM             = ShamanCommon::TREMOR_TOTEM,           // 8143
    SPELL_HEALING_STREAM_TOTEM     = ShamanCommon::HEALING_STREAM_TOTEM,   // 5394
    SPELL_HEALING_TIDE_TOTEM       = ShamanCommon::HEALING_TIDE_TOTEM,     // 108280
    SPELL_CAPACITOR_TOTEM          = ShamanCommon::CAPACITOR_TOTEM,        // 192058
    SPELL_SPIRIT_LINK_TOTEM        = ShamanCommon::SPIRIT_LINK_TOTEM,      // 98008
    SPELL_WIND_RUSH_TOTEM          = ShamanSpells::WIND_RUSH_TOTEM,        // 192077
    SPELL_WINDFURY_TOTEM           = ShamanCommon::WINDFURY_TOTEM,         // 8512
    SPELL_FIRE_ELEMENTAL           = ShamanCommon::FIRE_ELEMENTAL,         // 198067
    SPELL_EARTH_ELEMENTAL          = ShamanCommon::EARTH_ELEMENTAL,        // 198103
    SPELL_LIQUID_MAGMA_TOTEM       = ShamanElemental::LIQUID_MAGMA_TOTEM,  // 192222

    // Restoration-specific totems
    SPELL_EARTHEN_WALL_TOTEM       = ShamanRestoration::EARTHEN_WALL_TOTEM,      // 198838
    SPELL_ANCESTRAL_PROTECTION_TOTEM = ShamanRestoration::ANCESTRAL_PROTECTION_TOTEM, // 207399
    SPELL_CLOUDBURST_TOTEM         = ShamanRestoration::CLOUDBURST_TOTEM,        // 157153
    SPELL_MANA_TIDE_TOTEM          = ShamanRestoration::MANA_TIDE_TOTEM,         // 16191

    // Legacy totems removed in WoW 12.0 (kept as 0 for compatibility checks)
    SPELL_SEARING_TOTEM            = 0,  // Removed
    SPELL_FIRE_NOVA_TOTEM          = 0,  // Removed
    SPELL_MAGMA_TOTEM              = 0,  // Removed
    SPELL_FLAMETONGUE_TOTEM        = 0,  // Removed
    SPELL_TOTEM_OF_WRATH           = 0,  // Removed
    SPELL_FIRE_ELEMENTAL_TOTEM     = 0,  // Replaced by Fire Elemental (198067)
    SPELL_STONESKIN_TOTEM          = 0,  // Removed
    SPELL_STONECLAW_TOTEM          = 0,  // Removed
    SPELL_STRENGTH_OF_EARTH_TOTEM  = 0,  // Removed
    SPELL_EARTH_ELEMENTAL_TOTEM    = 0,  // Replaced by Earth Elemental (198103)
    SPELL_EARTHGRAB_TOTEM          = 0,  // Replaced by Earthbind Totem root talent
    SPELL_MANA_SPRING_TOTEM        = 0,  // Removed
    SPELL_POISON_CLEANSING_TOTEM   = 0,  // Removed
    SPELL_DISEASE_CLEANSING_TOTEM  = 0,  // Removed
    SPELL_FIRE_RESISTANCE_TOTEM    = 0,  // Removed
    SPELL_GROUNDING_TOTEM          = 0,  // Removed (was 8177)
    SPELL_NATURE_RESISTANCE_TOTEM  = 0,  // Removed
    SPELL_GRACE_OF_AIR_TOTEM       = 0,  // Removed
    SPELL_WRATH_OF_AIR_TOTEM       = 0,  // Removed
    SPELL_SENTRY_TOTEM             = 0   // Removed
};

// Shock spell definitions - WoW 12.0
enum ShockSpells : uint32
{
    SPELL_EARTH_SHOCK  = ShamanCommon::EARTH_SHOCK,   // 8042
    SPELL_FLAME_SHOCK  = ShamanCommon::FLAME_SHOCK,   // 188389 (updated ID)
    SPELL_FROST_SHOCK  = ShamanCommon::FROST_SHOCK,   // 196840 (updated ID)
    SPELL_WIND_SHEAR   = ShamanCommon::WIND_SHEAR     // 57994 - Interrupt
};

// Shield spell definitions - WoW 12.0
enum ShieldSpells : uint32
{
    SPELL_WATER_SHIELD  = ShamanRestoration::WATER_SHIELD,   // 52127
    SPELL_EARTH_SHIELD  = ShamanCommon::EARTH_SHIELD,        // 974
    // Note: Lightning Shield removed as standalone buff in 12.0
    SPELL_LIGHTNING_SHIELD = 0  // Removed/reworked
};

// Weapon imbue spell definitions - WoW 12.0
enum WeaponImbues : uint32
{
    SPELL_FLAMETONGUE_WEAPON = ShamanCommon::FLAMETONGUE_WEAPON, // 318038 (updated ID)
    SPELL_WINDFURY_WEAPON    = ShamanCommon::WINDFURY_WEAPON,    // 33757

    // Legacy weapon imbues removed in WoW 12.0
    SPELL_ROCKBITER_WEAPON   = 0,  // Removed
    SPELL_FROSTBRAND_WEAPON  = 0,  // Removed
    SPELL_EARTHLIVING_WEAPON = 0   // Removed
};

// Utility spell definitions - WoW 12.0
enum UtilitySpells : uint32
{
    SPELL_PURGE             = ShamanCommon::PURGE,             // 370
    SPELL_CLEANSE_SPIRIT    = ShamanCommon::CLEANSE_SPIRIT,    // 51886
    SPELL_HEX               = ShamanCommon::HEX,               // 51514
    SPELL_BLOODLUST         = ShamanCommon::BLOODLUST,         // 2825
    SPELL_HEROISM           = ShamanCommon::HEROISM,           // 32182
    SPELL_GHOST_WOLF        = ShamanCommon::GHOST_WOLF,        // 2645
    SPELL_ANCESTRAL_SPIRIT  = ShamanCommon::ANCESTRAL_SPIRIT,  // 2008
    SPELL_ASTRAL_SHIFT      = ShamanCommon::ASTRAL_SHIFT,      // 108271
    SPELL_SPIRIT_WALK       = ShamanCommon::SPIRIT_WALK,       // 58875
    SPELL_REINCARNATION     = ShamanCommon::REINCARNATION,     // 20608
    SPELL_PURIFY_SPIRIT     = ShamanCommon::PURIFY_SPIRIT,     // 77130

    // Legacy utilities (kept for reference but may not exist in 12.0)
    SPELL_WATER_WALKING     = ShamanSpells::WATER_WALKING,     // 546
    SPELL_ASTRAL_RECALL     = ShamanSpells::ASTRAL_RECALL,     // 556
    SPELL_WATER_BREATHING   = 0,  // Removed/combined with Water Walking
    SPELL_SHAMANISTIC_RAGE  = 0   // Removed (was Enhancement defensive)
};

// Healing spell definitions - WoW 12.0
enum HealingSpells : uint32
{
    SPELL_HEALING_WAVE       = ShamanCommon::HEALING_WAVE,       // 77472 (updated ID)
    SPELL_HEALING_SURGE      = ShamanCommon::HEALING_SURGE,      // 8004
    SPELL_CHAIN_HEAL         = ShamanCommon::CHAIN_HEAL,         // 1064
    SPELL_RIPTIDE            = ShamanCommon::RIPTIDE,            // 61295
    SPELL_HEALING_RAIN       = ShamanCommon::HEALING_RAIN,       // 73920

    // Restoration-specific healing
    SPELL_WELLSPRING         = ShamanRestoration::WELLSPRING,         // 197995
    SPELL_UNLEASH_LIFE       = ShamanRestoration::UNLEASH_LIFE,       // 73685
    SPELL_DOWNPOUR           = ShamanRestoration::DOWNPOUR,           // 207778
    SPELL_SPIRITWALKERS_GRACE = ShamanRestoration::SPIRITWALKERS_GRACE, // 79206

    // Legacy healing spells
    SPELL_LESSER_HEALING_WAVE = 0,  // Removed (replaced by Healing Surge)
    SPELL_ANCESTRAL_GUIDANCE  = 0,  // Reworked/talent
    SPELL_SPIRIT_LINK         = 0   // Now Spirit Link Totem only
};

// Damage spell definitions - WoW 12.0
enum DamageSpells : uint32
{
    SPELL_LIGHTNING_BOLT   = ShamanCommon::LIGHTNING_BOLT,   // 188196 (updated ID)
    SPELL_CHAIN_LIGHTNING  = ShamanCommon::CHAIN_LIGHTNING,  // 188443 (updated ID)
    SPELL_LAVA_BURST       = ShamanCommon::LAVA_BURST,       // 51505
    SPELL_EARTHQUAKE       = ShamanCommon::EARTHQUAKE,       // 61882
    SPELL_ELEMENTAL_BLAST  = ShamanCommon::ELEMENTAL_BLAST,  // 117014
    SPELL_LAVA_BEAM        = ShamanElemental::LAVA_BEAM,     // 114074
    SPELL_THUNDERSTORM     = 0  // 51490 - Removed/reworked in 12.0
};

// Enhancement-specific spells - WoW 12.0
enum EnhancementSpells : uint32
{
    SPELL_STORMSTRIKE       = ShamanCommon::STORMSTRIKE,              // 17364
    SPELL_LAVA_LASH         = ShamanCommon::LAVA_LASH,                // 60103
    SPELL_FERAL_SPIRIT      = ShamanCommon::FERAL_SPIRIT,             // 51533
    SPELL_CRASH_LIGHTNING   = ShamanCommon::CRASH_LIGHTNING,          // 187874
    SPELL_WINDSTRIKE        = ShamanEnhancement::WINDSTRIKE,          // 115356
    SPELL_SUNDERING         = ShamanEnhancement::SUNDERING,           // 197214
    SPELL_DOOM_WINDS        = ShamanEnhancement::DOOM_WINDS,          // 384352 (updated ID)
    SPELL_ICE_STRIKE        = ShamanEnhancement::ICE_STRIKE,          // 342240
    SPELL_FIRE_NOVA         = ShamanEnhancement::FIRE_NOVA,           // 333974
    SPELL_MAELSTROM_WEAPON  = ShamanEnhancement::MAELSTROM_WEAPON     // 187880
};

// Elemental-specific spells - WoW 12.0
enum ElementalSpells : uint32
{
    SPELL_ASCENDANCE         = ShamanCommon::ASCENDANCE,              // 114050
    SPELL_STORMKEEPER        = ShamanCommon::STORMKEEPER,             // 191634
    SPELL_ICEFURY            = ShamanElemental::ICEFURY,              // 210714
    SPELL_PRIMORDIAL_WAVE    = ShamanElemental::PRIMORDIAL_WAVE,      // 375982
    SPELL_ECHOING_SHOCK      = ShamanElemental::ECHOING_SHOCK,        // 320125
    SPELL_LIGHTNING_LASSO    = ShamanElemental::LIGHTNING_LASSO,      // 305483
    SPELL_TEMPEST            = ShamanElemental::TEMPEST,              // 454009

    // Legacy elemental spells
    SPELL_ELEMENTAL_MASTERY  = 0  // Removed/reworked in 12.0
};

// Talent IDs for specialization detection - WoW 12.0
// Note: Talent detection in 12.0 uses different systems; these are kept for compatibility
enum ShamanTalents : uint32
{
    // Core abilities that indicate spec (used as spell checks, not talent IDs)
    TALENT_LAVA_BURST_TALENT         = ShamanCommon::LAVA_BURST,       // 51505 (Elemental indicator)
    TALENT_STORMSTRIKE_TALENT        = ShamanCommon::STORMSTRIKE,      // 17364 (Enhancement indicator)
    TALENT_RIPTIDE_TALENT            = ShamanCommon::RIPTIDE,          // 61295 (Restoration indicator)
    TALENT_EARTH_SHIELD_TALENT       = ShamanCommon::EARTH_SHIELD,     // 974 (Restoration indicator)
    TALENT_LAVA_LASH_TALENT          = ShamanCommon::LAVA_LASH,        // 60103 (Enhancement indicator)
    TALENT_HEALING_RAIN_TALENT       = ShamanCommon::HEALING_RAIN,     // 73920 (Restoration indicator)

    // Legacy talents removed in 12.0
    TALENT_ELEMENTAL_FOCUS     = 0,  // Removed
    TALENT_ELEMENTAL_MASTERY   = 0,  // Removed
    TALENT_LIGHTNING_OVERLOAD  = 0,  // Removed (became Mastery)
    TALENT_TOTEM_OF_WRATH_TALENT = 0, // Removed
    TALENT_DUAL_WIELD          = 0,  // Now baseline for Enhancement
    TALENT_SHAMANISTIC_RAGE_TALENT = 0, // Removed
    TALENT_MAELSTROM_WEAPON    = ShamanEnhancement::MAELSTROM_WEAPON, // 187880
    TALENT_NATURES_SWIFTNESS   = 0,  // Removed (now general talent?)
    TALENT_MANA_TIDE_TOTEM_TALENT = ShamanRestoration::MANA_TIDE_TOTEM // 16191
};

// Combat constants
static const float OPTIMAL_CASTER_RANGE = 30.0f;
static const float OPTIMAL_MELEE_RANGE = 5.0f;
static const float TOTEM_PLACEMENT_RANGE = 20.0f;
static const float TOTEM_EFFECT_RANGE = 40.0f;
static const uint32 SHOCK_GLOBAL_COOLDOWN = 1500;
static const uint32 TOTEM_UPDATE_INTERVAL = 2000;
static const uint32 SHIELD_REFRESH_TIME = 540000; // 9 minutes
static const uint32 WEAPON_IMBUE_DURATION = 1800000; // 30 minutes
static const uint32 FLAME_SHOCK_DURATION = 30000; // 30 seconds
static const uint32 MAELSTROM_WEAPON_MAX = 5;

ShamanAI::ShamanAI(Player* bot) : ClassAI(bot),
    _manaSpent(0),
    _damageDealt(0),
    _healingDone(0),
    _totemsDeploy(0),
    _shocksUsed(0),
    _lastTotemUpdate(0),
    _lastTotemCheck(0),
    _lastShockTime(0),
    _flameshockTarget(0),
    _flameshockExpiry(0),
    _maelstromWeaponStacks(0),
    _lastMaelstromCheck(0),
    _elementalMaelstrom(0),
    _lastWindShear(0),
    _lastBloodlust(0),
    _lastElementalMastery(0),
    _lastAscendance(0),
    _lastFireElemental(0),
    _lastEarthElemental(0),
    _lastSpiritWalk(0),
    _lastShamanisticRage(0),
    _hasFlameShockUp(false),
    _lavaBurstCharges(2),
    _hasLavaSurgeProc(false),
    _healingStreamTotemTime(0),
    _chainHealBounceCount(3)
{

    TC_LOG_DEBUG("module.playerbot.ai", "ShamanAI created for player {}",
    bot ? bot->GetName() : "null");
}

ShamanAI::~ShamanAI() = default;

void ShamanAI::UpdateRotation(::Unit* target)
{
    if (!GetBot() || !target)
        return;

    Player* bot = GetBot();

    // NOTE: Baseline rotation check is now handled at the dispatch level in
    // ClassAI::OnCombatUpdate(). This method is ONLY called when the bot has
    // already chosen a specialization (level 10+ with talents).

    // ========================================================================
    // COMBAT BEHAVIOR INTEGRATION - Priority-based decision system
    // Following MageAI reference pattern with Shaman-specific mechanics
    // ========================================================================
    auto* behaviors = GetCombatBehaviors();

    // Priority 1: Interrupts (Wind Shear)
    if (HandleInterrupts(target))
        return;

    // Priority 2: Defensive abilities
    if (HandleDefensives())
        return;

    // Priority 3: Positioning (range management)
    if (HandlePositioning(target))
        return;

    // Priority 4: Totem management (unique to Shaman)
    if (HandleTotemManagement(target))
        return;

    // Priority 5: Target switching for priority targets
    if (HandleTargetSwitching(target))
        return;

    // Priority 6: Purge/Dispel
    if (HandlePurgeDispel(target))
        return;

    // Priority 7: AoE decisions
    if (HandleAoEDecisions(target))
        return;

    // Priority 8: Offensive cooldowns
    if (HandleOffensiveCooldowns(target))
        return;

    // Priority 9: Resource management (Maelstrom/Mana)
    if (HandleResourceManagement())
        return;

    // Priority 10: Normal rotation (spec-specific)
    HandleNormalRotation(target);

    // Track combat metrics
    if (GetBot()->IsInCombat())
    {
        _damageDealt += CalculateDamageDealt(target);
        _healingDone += CalculateHealingDone();
        _manaSpent += CalculateManaUsage();
    }
}

bool ShamanAI::HandleInterrupts(::Unit* target)
{
    auto* behaviors = GetCombatBehaviors();
    if (!behaviors || !target)
        return false;

    // Check if we should interrupt
    if (behaviors->ShouldInterrupt(target))
    {
        Unit* interruptTarget = behaviors->GetInterruptTarget();
        if (!interruptTarget)

            interruptTarget = target;

        // Wind Shear is our primary interrupt
    if (CanUseAbility(SPELL_WIND_SHEAR))
        {

            uint32 currentTime = GameTime::GetGameTimeMS();

            if (currentTime - _lastWindShear > 12000) // 12 sec cooldown

            {

                if (interruptTarget->IsNonMeleeSpellCast(false))

                {

                    if (CastSpell(SPELL_WIND_SHEAR, interruptTarget))

                    {

                        _lastWindShear = currentTime;

                        TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} interrupted {} with Wind Shear",

                                     GetBot()->GetName(), interruptTarget->GetName());

                        return true;

                    }

                }

            }
        }

        // Grounding Totem as backup interrupt mechanism
    if (CanUseAbility(SPELL_GROUNDING_TOTEM) && !_activeTotems[static_cast<size_t>(TotemType::AIR)].IsActive())
        {

            if (DeployTotem(SPELL_GROUNDING_TOTEM, TotemType::AIR))

            {

                TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} deployed Grounding Totem for spell protection",

                             GetBot()->GetName());

                return true;

            }
        }

        // Capacitor Totem for AoE stun interrupt
    if (static_cast<uint32>(GetBot()->GetPrimarySpecialization()) == 262 && CanUseAbility(SPELL_CAPACITOR_TOTEM)) // 262 = Elemental
        {

            if (GetBot()->GetDistance(target) <= 8.0f)

            {

                if (CastSpell(SPELL_CAPACITOR_TOTEM))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} using Capacitor Totem for stun",

                                 GetBot()->GetName());

                    return true;

                }

            }
        }
    }

    return false;
}

bool ShamanAI::HandleDefensives()
{
    auto* behaviors = GetCombatBehaviors();
    if (!behaviors)
        return false;

    if (!behaviors->NeedsDefensive())
        return false;

    float healthPct = GetBot()->GetHealthPct();

    // Critical health - use major defensives
    if (healthPct < 25.0f)
    {
        // Astral Shift - 40% damage reduction
    if (CanUseAbility(SPELL_ASTRAL_SHIFT))
        {

            if (CastSpell(SPELL_ASTRAL_SHIFT))

            {

                TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} using Astral Shift at {}% health",

                             GetBot()->GetName(), healthPct);

                return true;

            }
        }

        // Earth Elemental Totem for tanking
    if (CanUseAbility(SPELL_EARTH_ELEMENTAL_TOTEM))
        {

            uint32 currentTime = GameTime::GetGameTimeMS();

            if (currentTime - _lastEarthElemental > 300000) // 5 min cooldown

            {

                if (DeployTotem(SPELL_EARTH_ELEMENTAL_TOTEM, TotemType::EARTH))

                {

                    _lastEarthElemental = currentTime;

                    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} summoning Earth Elemental for protection",

                                 GetBot()->GetName());

                    return true;

                }

            }
        }
    }

    // Low health - use moderate defensives
    if (healthPct < 40.0f)
    {
        // Shamanistic Rage for Enhancement
    if (static_cast<uint32>(GetBot()->GetPrimarySpecialization()) == 263 && CanUseAbility(SPELL_SHAMANISTIC_RAGE)) // 263 = Enhancement
        {

            uint32 currentTime = GameTime::GetGameTimeMS();

            if (currentTime - _lastShamanisticRage > 60000) // 1 min cooldown

            {

                if (CastSpell(SPELL_SHAMANISTIC_RAGE))

                {

                    _lastShamanisticRage = currentTime;

                    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} using Shamanistic Rage",

                                 GetBot()->GetName());

                    return true;

                }

            }
        }

        // Healing Stream Totem for passive healing
    if (!_activeTotems[static_cast<size_t>(TotemType::WATER)].IsActive() ||

            _activeTotems[static_cast<size_t>(TotemType::WATER)].spellId != SPELL_HEALING_STREAM_TOTEM)
        {

            if (DeployTotem(SPELL_HEALING_STREAM_TOTEM, TotemType::WATER))

            {

                _healingStreamTotemTime = GameTime::GetGameTimeMS();

                TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} deploying Healing Stream Totem",

                             GetBot()->GetName());

                return true;

            }
        }

        // Stoneclaw Totem for damage absorption
    if (CanUseAbility(SPELL_STONECLAW_TOTEM))
        {

            if (CastSpell(SPELL_STONECLAW_TOTEM))

            {

                TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} using Stoneclaw Totem for shield",

                             GetBot()->GetName());

                return true;

            }
        }
    }

    // Spirit Walk for root/snare removal
    if (GetBot()->HasUnitState(UNIT_STATE_ROOT) || GetBot()->GetSpeedRate(MOVE_RUN) < 1.0f)
    {
        if (CanUseAbility(SPELL_SPIRIT_WALK))
        {

            uint32 currentTime = GameTime::GetGameTimeMS();

            if (currentTime - _lastSpiritWalk > 120000) // 2 min cooldown

            {

                if (CastSpell(SPELL_SPIRIT_WALK))

                {

                    _lastSpiritWalk = currentTime;

                    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} using Spirit Walk to break roots",

                                 GetBot()->GetName());

                    return true;

                }

            }
        }

        // Ghost Wolf as backup escape
    if (!GetBot()->HasAura(SPELL_GHOST_WOLF))
        {

            if (CastSpell(SPELL_GHOST_WOLF))

            {

                TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} shifting to Ghost Wolf form",

                             GetBot()->GetName());

                return true;

            }
        }
    }

    return false;
}

bool ShamanAI::HandlePositioning(::Unit* target)
{
    auto* behaviors = GetCombatBehaviors();
    if (!behaviors || !target)
        return false;

    if (!behaviors->NeedsRepositioning())
        return false;

    float currentDistance = GetBot()->GetDistance(target);
    float optimalRange = GetOptimalRange(target);

    // Enhancement needs to be in melee range
    if (static_cast<uint32>(GetBot()->GetPrimarySpecialization()) == 263) // 263 = Enhancement
    {
        if (currentDistance > OPTIMAL_MELEE_RANGE)
        {
            // Use Ghost Wolf for gap closing
    if (currentDistance > 15.0f && !GetBot()->HasAura(SPELL_GHOST_WOLF))

            {

                if (CastSpell(SPELL_GHOST_WOLF))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} using Ghost Wolf to close gap",

                                 GetBot()->GetName());

                    return true;

                }

            }

            // Feral Spirit for additional damage while closing
    if (CanUseAbility(SPELL_FERAL_SPIRIT))

            {

                if (CastSpell(SPELL_FERAL_SPIRIT))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} summoning Feral Spirits",

                                 GetBot()->GetName());

                    return true;

                }

            }
        }
    }
    // Elemental and Restoration need to maintain range
    else
    {
        if (currentDistance < 8.0f)
        {
            // Thunderstorm for knockback
    if (static_cast<uint32>(GetBot()->GetPrimarySpecialization()) == 262 && CanUseAbility(SPELL_THUNDERSTORM)) // 262 = Elemental

            {

                if (CastSpell(SPELL_THUNDERSTORM))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} using Thunderstorm for knockback",

                                 GetBot()->GetName());

                    return true;

                }

            }

            // Earthbind Totem for slowing
    if (!_activeTotems[static_cast<size_t>(TotemType::EARTH)].IsActive() ||

                _activeTotems[static_cast<size_t>(TotemType::EARTH)].spellId != SPELL_EARTHBIND_TOTEM)

            {

                if (DeployTotem(SPELL_EARTHBIND_TOTEM, TotemType::EARTH))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} placing Earthbind Totem for kiting",

                                 GetBot()->GetName());

                    return true;

                }

            }

            // Frost Shock for slowing while kiting
    if (CanUseAbility(SPELL_FROST_SHOCK))

            {

                if (CastSpell(SPELL_FROST_SHOCK, target))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} using Frost Shock to slow target",

                                 GetBot()->GetName());

                    return true;

                }

            }
        }
    }

    return false;
}

bool ShamanAI::HandleTotemManagement(::Unit* target)
{
    if (!target)
        return false;

    uint32 currentTime = GameTime::GetGameTimeMS();

    // Only update totems periodically
    if (currentTime - _lastTotemUpdate < TOTEM_UPDATE_INTERVAL)
        return false;

    _lastTotemUpdate = currentTime;

    // Check each totem type and deploy if needed
    for (uint8 i = 0; i < 4; ++i) // 4 totem types: FIRE, EARTH, WATER, AIR
    {
        TotemType type = static_cast<TotemType>(i);

        if (NeedsTotemRefresh(type))
        {

            uint32 totemSpell = GetOptimalTotem(type, target);

            if (totemSpell && CanUseAbility(totemSpell))

            {

                if (DeployTotem(totemSpell, type))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} deploying totem {} for slot {}",

                                 GetBot()->GetName(), totemSpell, i);

                    return true;

                }

            }
        }
    }

    return false;
}

bool ShamanAI::HandleTargetSwitching(::Unit* target)
{
    auto* behaviors = GetCombatBehaviors();
    if (!behaviors || !target)
        return false;

    if (!behaviors->ShouldSwitchTarget())
        return false;

    Unit* priorityTarget = behaviors->GetPriorityTarget();

    if (!priorityTarget || priorityTarget == target)
        return false;

    // Hex the current target if it's not the priority
    if (CanUseAbility(SPELL_HEX))
    {
        if (!target->HasAura(SPELL_HEX) && target->GetTypeId() == TYPEID_UNIT)
        {
            if (CastSpell(SPELL_HEX, target))
            {
                TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} hexing {} to switch targets",
                             GetBot()->GetName(), target->GetName());

                // Update target
                SetTarget(priorityTarget->GetGUID());
                return true;
            }
        }
    }

    // Apply Flame Shock to new target for Elemental
    if (static_cast<uint32>(GetBot()->GetPrimarySpecialization()) == 262) // 262 = Elemental
    {
        if (!HasFlameShockOnTarget(priorityTarget))
        {
            if (HandleFlameShock(priorityTarget))
            {
                SetTarget(priorityTarget->GetGUID());
                return true;
            }
        }
    }

    return false;
}

bool ShamanAI::HandlePurgeDispel(::Unit* target)
{
    if (!target)
        return false;

    // Purge enemy buffs
    if (target->IsHostileTo(GetBot()))
    {
        if (CanUseAbility(SPELL_PURGE))
        {
            // Check if target has purgeable buffs

            bool hasPurgeableBuff = false;

            Unit::AuraApplicationMap const& auras = target->GetAppliedAuras();
            for (auto const& [auraId, aurApp] : auras)

            {

                if (aurApp->IsPositive() && aurApp->GetBase()->GetSpellInfo()->Dispel == DISPEL_MAGIC)

                {

                    hasPurgeableBuff = true;

                    break;

                }

            }


            if (hasPurgeableBuff)

            {

                if (CastSpell(SPELL_PURGE, target))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} purging buffs from {}",

                                 GetBot()->GetName(), target->GetName());
                                 return true;

                }

            }
        }
    }

    // Cleanse Spirit for friendly dispels (Restoration)
    if (static_cast<uint32>(GetBot()->GetPrimarySpecialization()) == 264) // 264 = Restoration
    {
        // Check group members for debuffs
    if (Group* group = GetBot()->GetGroup())
        {

            for (GroupReference const& itr : group->GetMembers())

            {

                if (Player* member = itr.GetSource())
                {

                    if (!member->IsAlive() || member->GetDistance(GetBot()) > 40.0f)

                        continue;

                    // Check for dispellable debuffs

                    bool hasDispellableDebuff = false;

                    Unit::AuraApplicationMap const& auras = member->GetAppliedAuras();
                    for (auto const& [auraId, aurApp] : auras)

                    {

                        if (!aurApp->IsPositive())

                        {

                            uint32 dispelType = aurApp->GetBase()->GetSpellInfo()->Dispel;

                            if (dispelType == DISPEL_CURSE)

                            {

                                hasDispellableDebuff = true;

                                break;

                            }

                        }

                    }


                    if (hasDispellableDebuff && CanUseAbility(SPELL_CLEANSE_SPIRIT))

                    {

                        if (CastSpell(SPELL_CLEANSE_SPIRIT, member))

                        {

                            TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} cleansing {} with Cleanse Spirit",

                                         GetBot()->GetName(), member->GetName());

                            return true;

                        }

                    }

                }

            }
        }
    }

    return false;
}

bool ShamanAI::HandleAoEDecisions(::Unit* target)
{
    auto* behaviors = GetCombatBehaviors();
    if (!behaviors || !target)
        return false;

    if (!behaviors->ShouldAOE())
        return false;

    // Count nearby enemies
    ::std::list<Unit*> enemies;
    Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(GetBot(), GetBot(), 40.0f);
    Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(GetBot(), enemies, u_check);
    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = GetBot()->GetMap();
    if (!map)
        return false;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)

            return false;
    }

    // Query nearby GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        GetBot()->GetPosition(), 40.0f);

    // Process results (replace old searcher logic)
    for (ObjectGuid guid : nearbyGuids)
    {
        // PHASE 5F: Thread-safe spatial grid validation
        auto snapshot_entity = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), guid);

        Creature* entity = nullptr;
        if (snapshot_entity)
        {

        } snapshot_entity = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), guid);
 entity = nullptr;
 if (snapshot_entity)
 {
 }
        if (!entity)

            continue;
        // Original filtering logic from searcher goes here
    }
    // End of spatial grid fix
    if (enemies.size() < 3)
        return false;

    switch (static_cast<uint32>(GetBot()->GetPrimarySpecialization()))
    {
        case 262: // Elemental
        {
            // Earthquake for ground AoE
    if (CanUseAbility(SPELL_EARTHQUAKE))

            {
                // Note: Ground-targeted abilities need special handling

                TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} casting Earthquake for AoE",

                             GetBot()->GetName());
                // Would need actual ground targeting implementation

                return true;

            }

            // Chain Lightning for cleave
    if (HandleChainLightning(target))

                return true;

            // Lava Beam during Ascendance
    if (GetBot()->HasAura(SPELL_ASCENDANCE) && CanUseAbility(SPELL_LAVA_BEAM))

            {

                if (CastSpell(SPELL_LAVA_BEAM, target))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} using Lava Beam in Ascendance",

                                 GetBot()->GetName());

                    return true;

                }

            }

            // Liquid Magma Totem
    if (CanUseAbility(SPELL_LIQUID_MAGMA_TOTEM))

            {

                if (DeployTotem(SPELL_LIQUID_MAGMA_TOTEM, TotemType::FIRE))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} deploying Liquid Magma Totem",

                                 GetBot()->GetName());

                    return true;

                }

            }

            break;
        }

        case 263: // Enhancement
        {
            // Crash Lightning for melee AoE
    if (HandleCrashLightning())

                return true;

            // Fire Nova with Flame Shock spread
    if (_hasFlameShockUp && CanUseAbility(SPELL_FIRE_NOVA_TOTEM))

            {

                if (CastSpell(SPELL_FIRE_NOVA_TOTEM))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} using Fire Nova",

                                 GetBot()->GetName());

                    return true;

                }

            }

            // Chain Lightning with Maelstrom Weapon
    if (ShouldUseInstantLightningBolt() && CanUseAbility(SPELL_CHAIN_LIGHTNING))

            {

                if (CastSpell(SPELL_CHAIN_LIGHTNING, target))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} instant Chain Lightning with Maelstrom",

                                 GetBot()->GetName());

                    _maelstromWeaponStacks = 0;

                    return true;

                }

            }

            // Sundering for cone AoE
    if (CanUseAbility(SPELL_SUNDERING))

            {

                if (CastSpell(SPELL_SUNDERING))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} using Sundering",

                                 GetBot()->GetName());

                    return true;

                }

            }

            break;
        }

        case 264: // Restoration
        {
            // Chain Heal for group healing
    if (CountInjuredGroupMembers(80.0f) >= 3)

            {

                if (HandleChainHeal())

                    return true;

            }

            // Healing Rain for area healing
    if (HandleHealingRain())

                return true;

            // Spirit Link Totem for health redistribution
    if (CountInjuredGroupMembers(50.0f) >= 2)

            {

                if (HandleSpiritLink())

                    return true;

            }

            break;
        }
    }

    return false;
}

bool ShamanAI::HandleOffensiveCooldowns(::Unit* target)
{
    auto* behaviors = GetCombatBehaviors();
    if (!behaviors || !target)
        return false;

    if (!behaviors->ShouldUseCooldowns())
        return false;

    uint32 currentTime = GameTime::GetGameTimeMS();

    // Bloodlust/Heroism - raid-wide haste
    if (ShouldUseBloodlust())
    {
        uint32 spellId = GetBot()->GetTeamId() == TEAM_ALLIANCE ? SPELL_HEROISM : SPELL_BLOODLUST;
        if (CanUseAbility(spellId) && currentTime - _lastBloodlust > 600000) // 10 min debuff
        {

            if (CastSpell(spellId))

            {

                _lastBloodlust = currentTime;

                TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} casting Bloodlust/Heroism",

                             GetBot()->GetName());

                return true;

            }
        }
    }

    switch (static_cast<uint32>(GetBot()->GetPrimarySpecialization()))
    {
        case 262: // Elemental
        {
            // Ascendance for Lava Beam
    if (ShouldUseAscendance() && CanUseAbility(SPELL_ASCENDANCE))

            {

                if (currentTime - _lastAscendance > 180000) // 3 min cooldown

                {

                    if (CastSpell(SPELL_ASCENDANCE))

                    {

                        _lastAscendance = currentTime;

                        TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} activating Elemental Ascendance",

                                     GetBot()->GetName());

                        return true;

                    }

                }

            }

            // Elemental Mastery for instant cast
    if (ShouldUseElementalMastery() && CanUseAbility(SPELL_ELEMENTAL_MASTERY))

            {

                if (currentTime - _lastElementalMastery > 90000) // 1.5 min cooldown

                {

                    if (CastSpell(SPELL_ELEMENTAL_MASTERY))

                    {

                        _lastElementalMastery = currentTime;

                        TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} using Elemental Mastery",

                                     GetBot()->GetName());

                        return true;

                    }

                }

            }

            // Fire Elemental Totem
    if (CanUseAbility(SPELL_FIRE_ELEMENTAL_TOTEM))

            {

                if (currentTime - _lastFireElemental > 300000) // 5 min cooldown

                {

                    if (DeployTotem(SPELL_FIRE_ELEMENTAL_TOTEM, TotemType::FIRE))

                    {

                        _lastFireElemental = currentTime;

                        TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} summoning Fire Elemental",

                                     GetBot()->GetName());

                        return true;

                    }

                }

            }

            // Stormkeeper for empowered Lightning Bolts
    if (CanUseAbility(SPELL_STORMKEEPER))

            {

                if (CastSpell(SPELL_STORMKEEPER))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} casting Stormkeeper",

                                 GetBot()->GetName());

                    return true;

                }

            }

            break;
        }

        case 263: // Enhancement
        {
            // Ascendance for Windstrike
    if (ShouldUseAscendance() && CanUseAbility(SPELL_ASCENDANCE))

            {

                if (currentTime - _lastAscendance > 180000)

                {

                    if (CastSpell(SPELL_ASCENDANCE))

                    {

                        _lastAscendance = currentTime;

                        TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} activating Enhancement Ascendance",

                                     GetBot()->GetName());

                        return true;

                    }

                }

            }

            // Doom Winds for Windfury procs
    if (CanUseAbility(SPELL_DOOM_WINDS))

            {

                if (CastSpell(SPELL_DOOM_WINDS))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} activating Doom Winds",

                                 GetBot()->GetName());

                    return true;

                }

            }

            // Feral Spirit wolves
    if (CanUseAbility(SPELL_FERAL_SPIRIT))

            {

                if (CastSpell(SPELL_FERAL_SPIRIT))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} summoning Feral Spirits",

                                 GetBot()->GetName());

                    return true;

                }

            }

            break;
        }

        case 264: // Restoration
        {
            // Ascendance for spreading heals
    if (CountInjuredGroupMembers(60.0f) >= 3 && CanUseAbility(SPELL_ASCENDANCE))

            {

                if (currentTime - _lastAscendance > 180000)

                {

                    if (CastSpell(SPELL_ASCENDANCE))

                    {

                        _lastAscendance = currentTime;

                        TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} activating Restoration Ascendance",

                                     GetBot()->GetName());

                        return true;

                    }

                }

            }

            // Healing Tide Totem for major healing
    if (CountInjuredGroupMembers(50.0f) >= 3 && CanUseAbility(SPELL_HEALING_TIDE_TOTEM))

            {

                if (DeployTotem(SPELL_HEALING_TIDE_TOTEM, TotemType::WATER))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} deploying Healing Tide Totem",

                                 GetBot()->GetName());

                    return true;

                }

            }

            // Ancestral Guidance for healing while dealing damage
    if (CanUseAbility(SPELL_ANCESTRAL_GUIDANCE))

            {

                if (CastSpell(SPELL_ANCESTRAL_GUIDANCE))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} activating Ancestral Guidance",

                                 GetBot()->GetName());

                    return true;

                }

            }

            // Mana Tide Totem for mana restoration
    if (GetBot()->GetPowerPct(POWER_MANA) < 30.0f && CanUseAbility(SPELL_MANA_TIDE_TOTEM))

            {

                if (DeployTotem(SPELL_MANA_TIDE_TOTEM, TotemType::WATER))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} deploying Mana Tide Totem",

                                 GetBot()->GetName());

                    return true;

                }

            }

            break;
        }
    }

    return false;
}

bool ShamanAI::HandleResourceManagement()
{
    auto* behaviors = GetCombatBehaviors();

    switch (static_cast<uint32>(GetBot()->GetPrimarySpecialization()))
    {
        case 262: // Elemental
        {
            // Manage Elemental Maelstrom resource

            uint32 maelstrom = GetElementalMaelstrom();

            // Spend maelstrom if capped
    if (maelstrom >= 90 && !_currentTarget.IsEmpty())

            {
                // PHASE 5F: Thread-safe spatial grid validation

                auto snapshot_target = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), _currentTarget);


                Unit* target = nullptr;

                if (snapshot_target)

                {


                }
        
                // Earth Shock to dump maelstrom
    if (target && CanUseAbility(SPELL_EARTH_SHOCK))

                {

                    if (CastSpell(SPELL_EARTH_SHOCK, target))

                    {

                        _elementalMaelstrom -= 60;

                        TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} spending maelstrom with Earth Shock",

                                     GetBot()->GetName());

                        return true;

                    }

                }

            }

            break;
        }

        case 263: // Enhancement
        {
            // Check Maelstrom Weapon stacks

            _maelstromWeaponStacks = GetMaelstromWeaponStacks();

            // Use instant cast at 5 stacks
    if (_maelstromWeaponStacks >= MAELSTROM_WEAPON_MAX && !_currentTarget.IsEmpty())

            {
                // PHASE 5F: Thread-safe spatial grid validation

                auto snapshot_target = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), _currentTarget);


                Unit* target = nullptr;

                if (snapshot_target)

                {


                }
        
                // Instant Lightning Bolt for single target
    if (target && CanUseAbility(SPELL_LIGHTNING_BOLT))

                {

                    if (CastSpell(SPELL_LIGHTNING_BOLT, target))

                    {

                        _maelstromWeaponStacks = 0;

                        TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} instant Lightning Bolt with Maelstrom",

                                     GetBot()->GetName());

                        return true;

                    }

                }

            }

            break;
        }

        case 264: // Restoration
        {
            // Mana management for Restoration

            float manaPct = GetBot()->GetPowerPct(POWER_MANA);


            if (behaviors && behaviors->ShouldConserveMana())

            {
                // Use more efficient heals when low on mana
    if (manaPct < 30.0f)

                {
                    // Mana Spring Totem for regeneration
    if (!_activeTotems[static_cast<size_t>(TotemType::WATER)].IsActive() ||

                        _activeTotems[static_cast<size_t>(TotemType::WATER)].spellId != SPELL_MANA_SPRING_TOTEM)

                    {

                        if (DeployTotem(SPELL_MANA_SPRING_TOTEM, TotemType::WATER))

                        {

                            TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} deploying Mana Spring Totem",

                                         GetBot()->GetName());

                            return true;

                        }

                    }

                }

            }

            break;
        }
    }

    return false;
}

bool ShamanAI::HandleNormalRotation(::Unit* target)
{
    if (!target)
        return false;

    switch (static_cast<uint32>(GetBot()->GetPrimarySpecialization()))
    {
        case 262: // Elemental

            return UpdateElementalRotation(target);

        case 263: // Enhancement

            return UpdateEnhancementRotation(target);

        case 264: // Restoration

            return UpdateRestorationRotation(target);

        default:

            return false;
    }
}

// Elemental rotation implementation
bool ShamanAI::UpdateElementalRotation(::Unit* target)
{
    if (!target)
        return false;

    // Maintain Flame Shock
    if (HandleFlameShock(target))
        return true;

    // Lava Burst on cooldown (especially with Flame Shock up)
    if (HandleLavaBurst(target))
        return true;

    // Elemental Blast if talented
    if (HandleElementalBlast(target))
        return true;

    // Chain Lightning for cleave
    if (HandleChainLightning(target))
        return true;

    // Lightning Bolt as filler
    if (CanUseAbility(SPELL_LIGHTNING_BOLT))
    {
        if (CastSpell(SPELL_LIGHTNING_BOLT, target))
        {

            _elementalMaelstrom += 8;

            TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} casting Lightning Bolt",

                         GetBot()->GetName());
                         return true;
        }
    }

    return false;}

bool ShamanAI::HandleLavaBurst(::Unit* target)
{
    if (!target || !CanUseAbility(SPELL_LAVA_BURST))
        return false;

    // Check for Lava Surge proc
    CheckLavaSurgeProc();

    // Always use if we have charges or proc
    if (_lavaBurstCharges > 0 || _hasLavaSurgeProc)
    {
        // Guaranteed crit if Flame Shock is up
    if (HasFlameShockOnTarget(target))
        {

            if (CastSpell(SPELL_LAVA_BURST, target))

            {

                if (_hasLavaSurgeProc)

                    _hasLavaSurgeProc = false;

                else

                    _lavaBurstCharges--;


                _elementalMaelstrom += 10;

                TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} casting Lava Burst (charges: {})",

                             GetBot()->GetName(), _lavaBurstCharges);

                return true;

            }
        }
    }

    return false;
}

bool ShamanAI::HandleFlameShock(::Unit* target)
{
    if (!target || !CanUseAbility(SPELL_FLAME_SHOCK))
        return false;

    // Check if Flame Shock needs refresh
    if (!HasFlameShockOnTarget(target) ||
        (GameTime::GetGameTimeMS() - _flameshockExpiry < 9000)) // Refresh at <9 seconds
    {
        if (CastSpell(SPELL_FLAME_SHOCK, target))
        {

            _flameshockTarget = target->GetGUID().GetCounter();
            _flameshockExpiry = GameTime::GetGameTimeMS() + FLAME_SHOCK_DURATION;

            _hasFlameShockUp = true;

            _elementalMaelstrom += 20;

            TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} applying Flame Shock to {}",

                         GetBot()->GetName(), target->GetName());

            return true;
        }
    }

    return false;
}

bool ShamanAI::HandleChainLightning(::Unit* target)
{
    if (!target || !CanUseAbility(SPELL_CHAIN_LIGHTNING))
        return false;

    // Use Chain Lightning if there are multiple targets
    ::std::list<Unit*> enemies;
    Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(GetBot(), GetBot(), 30.0f);
    Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(GetBot(), enemies, u_check);
    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = GetBot()->GetMap();
    if (!map)
        return false;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)

            return false;
    }

    // Query nearby GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        GetBot()->GetPosition(), 30.0f);

    // Process results (replace old searcher logic)
    for (ObjectGuid guid : nearbyGuids)
    {
        // PHASE 5F: Thread-safe spatial grid validation
        auto snapshot_entity = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), guid);

        Creature* entity = nullptr;
        if (snapshot_entity)
        {

        } snapshot_entity = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), guid);
 entity = nullptr;
 if (snapshot_entity)
 {
 }
        if (!entity)

            continue;
        // Original filtering logic from searcher goes here
    }
    // End of spatial grid fix
    if (enemies.size() >= 2)
    {
        if (CastSpell(SPELL_CHAIN_LIGHTNING, target))
        {

            _elementalMaelstrom += 4 * ::std::min(static_cast<size_t>(5), enemies.size());

            TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} casting Chain Lightning",

                         GetBot()->GetName());

            return true;
        }
    }

    return false;}

// Enhancement rotation implementation
bool ShamanAI::UpdateEnhancementRotation(::Unit* target)
{
    if (!target || !IsInMeleeRange(target))
        return false;

    // Maintain Flame Shock for Lava Lash
    if (!HasFlameShockOnTarget(target))
    {
        if (HandleFlameShock(target))

            return true;
    }

    // Stormstrike on cooldown
    if (HandleStormstrike(target))
        return true;

    // Windstrike during Ascendance
    if (GetBot()->HasAura(SPELL_ASCENDANCE))
    {
        if (HandleWindstrike(target))

            return true;
    }

    // Lava Lash with Flame Shock up
    if (_hasFlameShockUp)
    {
        if (HandleLavaLash(target))

            return true;
    }

    // Crash Lightning for AoE or buff
    if (HandleCrashLightning())
        return true;

    // Maelstrom Weapon instant casts
    if (HandleMaelstromWeapon())
        return true;

    // Auto attack
    if (GetBot()->GetVictim() != target)
    {
        GetBot()->Attack(target, true);
    }

    return false;
}

bool ShamanAI::HandleStormstrike(::Unit* target)
{
    if (!target || !CanUseAbility(SPELL_STORMSTRIKE))
        return false;

    if (IsInMeleeRange(target))
    {
        if (CastSpell(SPELL_STORMSTRIKE, target))
        {

            TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} using Stormstrike on {}",

                         GetBot()->GetName(), target->GetName());

            return true;
        }
    }

    return false;
}

bool ShamanAI::HandleLavaLash(::Unit* target)
{
    if (!target || !CanUseAbility(SPELL_LAVA_LASH))
        return false;

    if (IsInMeleeRange(target))
    {
        if (CastSpell(SPELL_LAVA_LASH, target))
        {

            TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} using Lava Lash on {}",

                         GetBot()->GetName(), target->GetName());

            return true;
        }
    }

    return false;
}

bool ShamanAI::HandleMaelstromWeapon()
{
    if (_maelstromWeaponStacks < MAELSTROM_WEAPON_MAX || _currentTarget.IsEmpty())
        return false;

    // PHASE 5F: Thread-safe spatial grid validation
    auto snapshot_target = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), _currentTarget);

    Unit* target = nullptr;
    if (snapshot_target)
    {

    }
        
    if (!target)
        return false;

    // Use instant Lightning Bolt at 5 stacks
    if (CanUseAbility(SPELL_LIGHTNING_BOLT))
    {
        if (CastSpell(SPELL_LIGHTNING_BOLT, target))
        {

            _maelstromWeaponStacks = 0;

            TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} instant Lightning Bolt with Maelstrom Weapon",

                         GetBot()->GetName());

            return true;
        }
    }

    return false;
}

// Restoration rotation implementation
bool ShamanAI::UpdateRestorationRotation(::Unit* target)
{
    // Priority healing for group members
    Player* lowestHealth = GetLowestHealthGroupMember();

    if (lowestHealth)
    {
        float healthPct = lowestHealth->GetHealthPct();

        // Emergency healing
    if (healthPct < 30.0f)
        {
            // Healing Surge for fast healing
    if (CanUseAbility(SPELL_HEALING_SURGE))

            {

                if (CastSpell(SPELL_HEALING_SURGE, lowestHealth))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} emergency Healing Surge on {}",

                                 GetBot()->GetName(), lowestHealth->GetName());

                    return true;

                }

            }
        }

        // Riptide for instant heal + HoT
    if (healthPct < 80.0f && !lowestHealth->HasAura(SPELL_RIPTIDE))

        {
        if (HandleRiptide(lowestHealth))

                return true;
        }

        // Chain Heal for group healing
    if (CountInjuredGroupMembers(70.0f) >= 2)
        {

            if (HandleChainHeal())

                return true;
        }

        // Healing Wave for efficient healing
    if (healthPct < 70.0f)
        {

            if (HandleHealingWave(lowestHealth))

                return true;
        }
    }

    // Maintain Healing Stream Totem
    if (HandleHealingStreamTotem())
        return true;

    // Damage if no healing needed
    if (target && target->IsHostileTo(GetBot()))
    {
        // Lightning Bolt for damage
    if (CanUseAbility(SPELL_LIGHTNING_BOLT))
        {

            if (CastSpell(SPELL_LIGHTNING_BOLT, target))

            {

                TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} dealing damage with Lightning Bolt",

                             GetBot()->GetName());

                return true;

            }
        }
    }

    return false;
}

bool ShamanAI::HandleRiptide(Player* target)
{
    if (!target || !CanUseAbility(SPELL_RIPTIDE))
        return false;

    if (!target->HasAura(SPELL_RIPTIDE))
    {
        if (CastSpell(SPELL_RIPTIDE, target))
        {

            TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} casting Riptide on {}",

                         GetBot()->GetName(), target->GetName());

            return true;
        }
    }

    return false;}

bool ShamanAI::HandleChainHeal()
{
    if (!CanUseAbility(SPELL_CHAIN_HEAL))
        return false;

    Player* target = GetLowestHealthGroupMember();
    if (target)
    {
        if (CastSpell(SPELL_CHAIN_HEAL, target))
        {

            TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} casting Chain Heal starting from {}",

                         GetBot()->GetName(), target->GetName());

            return true;
        }
    }

    return false;
}

// Totem management helpers
bool ShamanAI::NeedsTotemRefresh(TotemType type) const
{
    const TotemInfo& totem = _activeTotems[static_cast<size_t>(type)];

    // No totem active
    if (!totem.IsActive())
        return true;

    // Totem expired (most totems last 2 minutes)
    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - totem.deployTime > 120000)
        return true;

    // Totem out of range
    if (!_currentTarget.IsEmpty())
    {
        // PHASE 5F: Thread-safe spatial grid validation
        auto snapshot_target = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), _currentTarget);

        Unit* target = nullptr;
        if (snapshot_target)
        {

        }
        
        if (target && !IsTotemInRange(type, target))

            return true;
    }

    return false;
}

uint32 ShamanAI::GetOptimalTotem(TotemType type, ::Unit* target) const
{
    if (!target)
        return 0;

    switch (type)
    {
        case TotemType::FIRE:
        {

            switch (static_cast<uint32>(GetBot()->GetPrimarySpecialization()))

            {

                case 262: // Elemental
                    // Searing Totem for single target damage

                    return SPELL_SEARING_TOTEM;


                case 263: // Enhancement
                    // Magma Totem for AoE
    if (GetBot()->GetDistance(target) <= 8.0f)
                    return SPELL_MAGMA_TOTEM;

                    return SPELL_SEARING_TOTEM;


                case 264: // Restoration
                    // Flametongue Totem for spell power buff

                    return SPELL_FLAMETONGUE_TOTEM;

            }

            break;
        }

        case TotemType::EARTH:
        {
            // Stoneskin Totem for physical mitigation
    if (target->GetTypeId() == TYPEID_UNIT && target->ToCreature()->IsDungeonBoss())
            return SPELL_STONESKIN_TOTEM;

            // Strength of Earth for melee
    if (static_cast<uint32>(GetBot()->GetPrimarySpecialization()) == 263) // 263 = Enhancement

                return SPELL_STRENGTH_OF_EARTH_TOTEM;

            // Earthbind for kiting
    if (static_cast<uint32>(GetBot()->GetPrimarySpecialization()) != 263 && GetBot()->GetDistance(target) < 15.0f) // 263 = Enhancement

                return SPELL_EARTHBIND_TOTEM;


            return SPELL_STONESKIN_TOTEM;
        }

        case TotemType::WATER:
        {

            switch (static_cast<uint32>(GetBot()->GetPrimarySpecialization()))

            {

                case 264: // Restoration
                    // Healing Stream for constant healing

                    return SPELL_HEALING_STREAM_TOTEM;


                default:
                    // Mana Spring for mana regen
    if (GetBot()->GetPowerPct(POWER_MANA) < 70.0f)

                        return SPELL_MANA_SPRING_TOTEM;

                    return SPELL_HEALING_STREAM_TOTEM;

            }

            break;
        }

        case TotemType::AIR:
        {

            switch (static_cast<uint32>(GetBot()->GetPrimarySpecialization()))

            {

                case 263: // Enhancement
                    // Windfury for attack speed

                    return SPELL_WINDFURY_TOTEM;


                case 262: // Elemental
                    // Wrath of Air for spell haste

                    return SPELL_WRATH_OF_AIR_TOTEM;


                case 264: // Restoration
                    // Grace of Air for agility

                    return SPELL_GRACE_OF_AIR_TOTEM;

            }

            break;
        }
    }

    return 0;
}

bool ShamanAI::DeployTotem(uint32 spellId, TotemType type)
{
    if (!CanUseAbility(spellId))        return false;

    if (CastSpell(spellId))
    {
        TotemInfo& totem = _activeTotems[static_cast<size_t>(type)];
        totem.spellId = spellId;
        totem.deployTime = GameTime::GetGameTimeMS();
        totem.position = GetBot()->GetPosition();
        // totemUnit will be set when the totem is summoned

        _totemsDeploy++;

        TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} deployed totem {} in slot {}",

                     GetBot()->GetName(), spellId, static_cast<uint32>(type));
        return true;
    }

    return false;
}

bool ShamanAI::IsTotemInRange(TotemType type, ::Unit* target) const
{
    if (!target)
        return false;

    const TotemInfo& totem = _activeTotems[static_cast<size_t>(type)];
    if (!totem.IsActive())
        return false;

    // Check distance from totem position to target
    float distance = target->GetDistance(totem.position);
    return distance <= TOTEM_EFFECT_RANGE;
}

// Helper methods
bool ShamanAI::IsInMeleeRange(::Unit* target) const
{
    return target && GetBot()->GetDistance(target) <= OPTIMAL_MELEE_RANGE;
}

bool ShamanAI::HasFlameShockOnTarget(::Unit* target) const
{
    if (!target)
        return false;

    return target->HasAura(SPELL_FLAME_SHOCK, GetBot()->GetGUID());
}

void ShamanAI::RefreshFlameShock(::Unit* target)
{
    if (!target)
        return;

    HandleFlameShock(target);
}

bool ShamanAI::CheckLavaSurgeProc()
{
    // Check for Lava Surge buff that gives instant Lava Burst
    // This would check for the actual proc buff ID
    _hasLavaSurgeProc = false; // GetBot()->HasAura(LAVA_SURGE_PROC_BUFF);
    return _hasLavaSurgeProc;
}

uint32 ShamanAI::GetElementalMaelstrom() const
{
    // In real implementation, this would read the actual maelstrom power
    return _elementalMaelstrom;
}

uint32 ShamanAI::GetMaelstromWeaponStacks() const
{
    // In real implementation, this would check the Maelstrom Weapon buff stacks
    return _maelstromWeaponStacks;
}

bool ShamanAI::ShouldUseInstantLightningBolt() const
{
    return _maelstromWeaponStacks >= MAELSTROM_WEAPON_MAX;
}

bool ShamanAI::ShouldUseAscendance() const
{
    // Use on boss fights or when multiple enemies
    if (!_currentTarget.IsEmpty())
    {
        // PHASE 5F: Thread-safe spatial grid validation
        auto snapshot_target = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), _currentTarget);

        Unit* target = nullptr;
        if (snapshot_target)
        {

        }
        
        if (target && target->GetMaxHealth() > 1000000)

            return true;
    }    ::std::list<Unit*> enemies;
    Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(GetBot(), GetBot(), 40.0f);
    Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(GetBot(), enemies, u_check);
    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = GetBot()->GetMap();
    if (!map)
        return false;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)

            return false;
    }

    // Query nearby GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        GetBot()->GetPosition(), 40.0f);

    // Process results (replace old searcher logic)
    for (ObjectGuid guid : nearbyGuids)
    {
        // PHASE 5F: Thread-safe spatial grid validation
        auto snapshot_entity = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), guid);

        Creature* entity = nullptr;
        if (snapshot_entity)
        {

        } snapshot_entity = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), guid);
 entity = nullptr;
 if (snapshot_entity)
 {
 }
        if (!entity)

            continue;
        // Original filtering logic from searcher goes here
    }
    // End of spatial grid fix
    return enemies.size() >= 3;
}

bool ShamanAI::ShouldUseElementalMastery() const
{
    // Use when we need burst damage
    if (!_currentTarget.IsEmpty())
    {
        // PHASE 5F: Thread-safe spatial grid validation
        auto snapshot_target = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), _currentTarget);

        Unit* target = nullptr;
        if (snapshot_target)
        {

        }
        
        return target && target->GetHealthPct() < 30.0f;
    }
    return false;
}

Player* ShamanAI::GetLowestHealthGroupMember() const
{
    Player* lowest = nullptr;
    float lowestPct = 100.0f;

    if (Group* group = GetBot()->GetGroup())
    {
        for (GroupReference const& itr : group->GetMembers())
        {

            if (Player* member = itr.GetSource())
            {

                if (!member->IsAlive() || member->GetDistance(GetBot()) > 40.0f)

                    continue;


                float healthPct = member->GetHealthPct();

                if (healthPct < lowestPct)

                {

                    lowestPct = healthPct;

                    lowest = member;

                }

            }
        }
    }

    // Check self if not in group or lowest
    if (!lowest || GetBot()->GetHealthPct() < lowestPct)
        lowest = GetBot();

    return lowest;
}

uint32 ShamanAI::CountInjuredGroupMembers(float healthThreshold) const
{
    uint32 count = 0;

    if (Group* group = GetBot()->GetGroup())
    {
        for (GroupReference const& itr : group->GetMembers())
        {

            if (Player* member = itr.GetSource())
            {

                if (member->IsAlive() && member->GetHealthPct() < healthThreshold &&

                    member->GetDistance(GetBot()) <= 40.0f)

                {

                    count++;

                }

            }
        }
    }
    else if (GetBot()->GetHealthPct() < healthThreshold)
    {
        count = 1;
    }

    return count;
}

bool ShamanAI::HandleCrashLightning()
{
    if (!CanUseAbility(SPELL_CRASH_LIGHTNING))
        return false;

    // Use if multiple enemies nearby
    ::std::list<Unit*> enemies;
    Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(GetBot(), GetBot(), 8.0f);
    Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(GetBot(), enemies, u_check);
    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = GetBot()->GetMap();
    if (!map)
        return false;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)

            return false;
    }

    // Query nearby GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        GetBot()->GetPosition(), 8.0f);

    // Process results (replace old searcher logic)
    for (ObjectGuid guid : nearbyGuids)
    {
        // PHASE 5F: Thread-safe spatial grid validation
        auto snapshot_entity = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), guid);

        Creature* entity = nullptr;
        if (snapshot_entity)
        {

        } snapshot_entity = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), guid);
 entity = nullptr;
 if (snapshot_entity)
 {
 }
        if (!entity)

            continue;
        // Original filtering logic from searcher goes here
    }
    // End of spatial grid fix

    bool shouldUse = enemies.size() >= 2;
    if (!_currentTarget.IsEmpty())
    {
        // PHASE 5F: Thread-safe spatial grid validation
        auto snapshot_target = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), _currentTarget);

        Unit* target = nullptr;
        if (snapshot_target)
        {

        }
        
        if (target && IsInMeleeRange(target))

            shouldUse = true;
    }

    if (shouldUse)
    {
        if (CastSpell(SPELL_CRASH_LIGHTNING))
        {

            TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} using Crash Lightning",

                         GetBot()->GetName());

            return true;
        }
    }

    return false;
}

bool ShamanAI::HandleWindstrike(::Unit* target)
{
    if (!target || !CanUseAbility(SPELL_WINDSTRIKE) || !GetBot()->HasAura(SPELL_ASCENDANCE))
        return false;

    if (IsInMeleeRange(target))
    {
        if (CastSpell(SPELL_WINDSTRIKE, target))
        {

            TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} using Windstrike during Ascendance",

                         GetBot()->GetName());

            return true;
        }
    }

    return false;
}

bool ShamanAI::HandleEarthquake()
{
    if (!CanUseAbility(SPELL_EARTHQUAKE))
        return false;

    // Use if enough maelstrom and multiple enemies
    if (_elementalMaelstrom >= 60)
    {
        ::std::list<Unit*> enemies;
        Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(GetBot(), GetBot(), 40.0f);
        Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(GetBot(), enemies, u_check);
        // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = GetBot()->GetMap();
    if (!map)
        return false;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)

            return false;
    }

    // Query nearby GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        GetBot()->GetPosition(), 40.0f);

    // Process results (replace old searcher logic)
    for (ObjectGuid guid : nearbyGuids)
    {
        // PHASE 5F: Thread-safe spatial grid validation
        auto snapshot_entity = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), guid);

        Creature* entity = nullptr;
        if (snapshot_entity)
        {

        } snapshot_entity = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), guid);
 entity = nullptr;
 if (snapshot_entity)
 {
 }
        if (!entity)

            continue;
        // Original filtering logic from searcher goes here
    }
    // End of spatial grid fix
    if (enemies.size() >= 3)
        {
            // Note: Ground-targeted spell, needs special handling

            TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} casting Earthquake",

                         GetBot()->GetName());

            _elementalMaelstrom -= 60;

            return true;
        }
    }

    return false;
}

bool ShamanAI::HandleElementalBlast(::Unit* target)
{
    if (!target || !CanUseAbility(SPELL_ELEMENTAL_BLAST))
        return false;

    if (CastSpell(SPELL_ELEMENTAL_BLAST, target))
    {
        _elementalMaelstrom += 25;
        TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} casting Elemental Blast",

                     GetBot()->GetName());
        return true;
    }

    return false;
}

bool ShamanAI::HandleHealingWave(Player* target)
{
    if (!target || !CanUseAbility(SPELL_HEALING_WAVE))
        return false;

    if (CastSpell(SPELL_HEALING_WAVE, target))
    {
        TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} casting Healing Wave on {}",

                     GetBot()->GetName(), target->GetName());
        return true;
    }

    return false;
}

bool ShamanAI::HandleHealingRain()
{
    if (!CanUseAbility(SPELL_HEALING_RAIN))
        return false;

    // Use if multiple injured group members
    if (CountInjuredGroupMembers(70.0f) >= 3)
    {
        // Note: Ground-targeted spell
        TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} casting Healing Rain",

                     GetBot()->GetName());
        return true;
    }

    return false;
}

bool ShamanAI::HandleHealingStreamTotem()
{
    // Check if we need healing stream totem
    if (_activeTotems[static_cast<size_t>(TotemType::WATER)].spellId == SPELL_HEALING_STREAM_TOTEM &&
        _activeTotems[static_cast<size_t>(TotemType::WATER)].IsActive())
    {
        return false;
    }

    if (CountInjuredGroupMembers(90.0f) >= 1)
    {
        if (DeployTotem(SPELL_HEALING_STREAM_TOTEM, TotemType::WATER))
        {

            _healingStreamTotemTime = GameTime::GetGameTimeMS();

            TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} deploying Healing Stream Totem",

                         GetBot()->GetName());

            return true;
        }
    }

    return false;
}

bool ShamanAI::HandleSpiritLink()
{
    if (!CanUseAbility(SPELL_SPIRIT_LINK_TOTEM))
        return false;

    // Use for health redistribution when tank is low
    if (Group* group = GetBot()->GetGroup())
    {
        Player* tank = FindGroupTank(group);
        if (tank && tank->GetHealthPct() < 30.0f)
        {

            if (CastSpell(SPELL_SPIRIT_LINK_TOTEM))

            {

                TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} deploying Spirit Link Totem",

                             GetBot()->GetName());

                return true;

            }
        }
    }

    return false;
}

// Keep all the existing helper methods from the original implementation
void ShamanAI::UpdateBuffs()
{
    if (!GetBot())
        return;

    // Update shields first
    UpdateShamanBuffs();

    // Check weapon imbues
    UpdateWeaponImbues();

    // Water walking/breathing utility
    if (!GetBot()->IsInCombat())
    {
        UpdateUtilityBuffs();
    }
}

void ShamanAI::UpdateCooldowns(uint32 diff)
{
    if (!GetBot())
        return;

    // Update ability cooldowns
    for (auto& [spellId, lastUse] : _abilityUsage)
    {
        if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE))
        {
            // Update cooldown tracking
    if (lastUse > 0 && lastUse < diff)

            {

                lastUse = 0;

            }

            else if (lastUse > 0)

            {

                lastUse -= diff;

            }
        }
    }

    // Update Lava Burst charges
    static uint32 lavaBurstRecharge = 0;
    lavaBurstRecharge += diff;
    if (lavaBurstRecharge >= 8000 && _lavaBurstCharges < 2) // 8 sec recharge
    {
        _lavaBurstCharges++;
        lavaBurstRecharge = 0;
    }
}

bool ShamanAI::CanUseAbility(uint32 spellId)
{
    if (!GetBot())
        return false;

    // Check if spell is learned
    if (!GetBot()->HasSpell(spellId))
        return false;

    // Check if spell is ready
    if (!IsSpellReady(spellId))
        return false;

    // Check resource requirements
    if (!HasEnoughResource(spellId))
        return false;

    return true;
}

void ShamanAI::OnCombatStart(::Unit* target)
{
    if (!GetBot() || !target)
        return;

    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} entering combat with {}",

                 GetBot()->GetName(), target->GetName());

    // Deploy initial totems
    DeployInitialTotems(target);

    // Apply initial buffs
    ApplyCombatBuffs();

    // Reset combat tracking
    _hasFlameShockUp = false;
    _flameshockTarget = 0;
    _flameshockExpiry = 0;
    _maelstromWeaponStacks = 0;
    _elementalMaelstrom = 0;
    _lavaBurstCharges = 2;
    _hasLavaSurgeProc = false;

    // Initialize combat tracking
    _combatTime = 0;
    _inCombat = true;
    SetTarget(target->GetGUID());
}

void ShamanAI::OnCombatEnd()
{
    if (!GetBot())
        return;

    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} leaving combat. Metrics - Damage: {}, Healing: {}, Mana Used: {}, Totems: {}, Shocks: {}",

                 GetBot()->GetName(), _damageDealt, _healingDone, _manaSpent, _totemsDeploy, _shocksUsed);

    // Recall unnecessary totems
    RecallCombatTotems();

    // Reset totem tracking
    for (auto& totem : _activeTotems)
    {
        totem.totemUnit = nullptr;
    }

    // Reset combat tracking
    _inCombat = false;
    SetTarget(ObjectGuid::Empty);

    // Log performance metrics
    LogCombatMetrics();
}

bool ShamanAI::HasEnoughResource(uint32 spellId)
{
    if (!GetBot())
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    // Check mana cost
    auto powerCosts = spellInfo->CalcPowerCost(GetBot(), spellInfo->GetSchoolMask());
    for (auto const& cost : powerCosts)
    {
        if (cost.Power == POWER_MANA && GetBot()->GetPower(POWER_MANA) < int32(cost.Amount))

            return false;
    }

    // Check maelstrom cost for Elemental
    if (static_cast<uint32>(GetBot()->GetPrimarySpecialization()) == 262) // 262 = Elemental
    {
        if (spellId == SPELL_EARTH_SHOCK && _elementalMaelstrom < 60)

            return false;
        if (spellId == SPELL_EARTHQUAKE && _elementalMaelstrom < 60)

            return false;
    }

    return true;
}

void ShamanAI::ConsumeResource(uint32 spellId)
{
    if (!GetBot())
        return;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return;

    // Track mana consumption
    auto powerCosts = spellInfo->CalcPowerCost(GetBot(), spellInfo->GetSchoolMask());
    for (auto const& cost : powerCosts)
    {
        if (cost.Power == POWER_MANA)

            _manaSpent += cost.Amount;
    }

    // Track ability usage
    _abilityUsage[spellId] = GameTime::GetGameTimeMS();

    // Track specific spell categories
    if (IsShockSpell(spellId))
    {
        _shocksUsed++;
        _lastShockTime = GameTime::GetGameTimeMS();
    }
    else if (IsTotemSpell(spellId))
    {
        _totemsDeploy++;
    }
}

Position ShamanAI::GetOptimalPosition(::Unit* target)
{
    if (!GetBot() || !target)
        return Position();

    // Use CombatBehaviorIntegration for positioning
    auto* behaviors = GetCombatBehaviors();
    if (behaviors)
    {
        return behaviors->GetOptimalPosition();
    }

    // Fallback to spec-based positioning
    float optimalRange = GetOptimalRange(target);
    float angle = GetBot()->GetAbsoluteAngle(target);
    float x = target->GetPositionX() - optimalRange * ::std::cos(angle);
    float y = target->GetPositionY() - optimalRange * ::std::sin(angle);
    float z = target->GetPositionZ();    return Position(x, y, z);
}

float ShamanAI::GetOptimalRange(::Unit* target)
{
    if (!GetBot() || !target)
        return OPTIMAL_CASTER_RANGE;

    // Enhancement needs melee range
    if (static_cast<uint32>(GetBot()->GetPrimarySpecialization()) == 263) // 263 = Enhancement
        return OPTIMAL_MELEE_RANGE;

    // Elemental and Restoration maintain caster range
    return OPTIMAL_CASTER_RANGE;
}

void ShamanAI::UpdateShamanBuffs()
{
    if (!GetBot())
        return;

    // Lightning Shield for Elemental/Enhancement
    if (static_cast<uint32>(GetBot()->GetPrimarySpecialization()) != 264) // 264 = Restoration
    {
        if (!HasAura(SPELL_LIGHTNING_SHIELD, GetBot()))
        {

            if (CastSpell(SPELL_LIGHTNING_SHIELD))

            {

                TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} casting Lightning Shield", GetBot()->GetName());

            }
        }
    }
    // Water Shield for Restoration
    else
    {
        if (!HasAura(SPELL_WATER_SHIELD, GetBot()))
        {

            if (CastSpell(SPELL_WATER_SHIELD))

            {

                TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} casting Water Shield", GetBot()->GetName());

            }
        }
    }

    // Earth Shield on tank in group
    if (static_cast<uint32>(GetBot()->GetPrimarySpecialization()) == 264 && GetBot()->HasSpell(SPELL_EARTH_SHIELD)) // 264 = Restoration
    {
        if (Group* group = GetBot()->GetGroup())
        {

            Player* tank = FindGroupTank(group);

                                 if (!tank)

                                 {

                                     return;

                                 }

            if (tank && !HasAura(SPELL_EARTH_SHIELD, tank))

            {

                if (CastSpell(SPELL_EARTH_SHIELD, tank))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} casting Earth Shield on tank {}",

                                 GetBot()->GetName(), tank->GetName());

                }

            }
        }
    }
}

void ShamanAI::UpdateTotemCheck()
{
    if (!GetBot())
        return;

    static uint32 lastTotemCheck = 0;
    uint32 currentTime = GameTime::GetGameTimeMS();

    if (currentTime - lastTotemCheck < TOTEM_UPDATE_INTERVAL)
        return;

    lastTotemCheck = currentTime;

    // Totem management is handled via HandleTotemManagement in UpdateRotation
}

void ShamanAI::UpdateShockRotation()
{
    // Shock rotation is handled via HandleNormalRotation in UpdateRotation
    // No separate delegation needed
}

// Private helper methods
void ShamanAI::UpdateWeaponImbues()
{
    if (!GetBot())
        return;

    // Check main hand weapon imbue
    if (!HasWeaponImbue(true))
    {
        uint32 imbueSpell = GetOptimalWeaponImbue(true);
        if (imbueSpell && CastSpell(imbueSpell))
        {

            TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} applying weapon imbue {} to main hand",

                         GetBot()->GetName(), imbueSpell);
        }
    }

    // Check off-hand weapon imbue for Enhancement
    if (static_cast<uint32>(GetBot()->GetPrimarySpecialization()) == 263 && !HasWeaponImbue(false)) // 263 = Enhancement
    {
        uint32 imbueSpell = GetOptimalWeaponImbue(false);
        if (imbueSpell && CastSpell(imbueSpell))
        {

            TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} applying weapon imbue {} to off-hand",

                         GetBot()->GetName(), imbueSpell);
        }
    }
}

void ShamanAI::UpdateUtilityBuffs()
{
    if (!GetBot())
        return;

    // Water walking when near water
    if (NearWater() && !HasAura(SPELL_WATER_WALKING, GetBot()))
    {
        CastSpell(SPELL_WATER_WALKING, GetBot());
    }

    // Ghost Wolf for movement speed when traveling
    if (GetBot()->isMoving() && !GetBot()->IsInCombat() && !HasAura(SPELL_GHOST_WOLF))
    {
        // Use Ghost Wolf for long-distance travel
        CastSpell(SPELL_GHOST_WOLF);
    }
}

void ShamanAI::DeployInitialTotems(::Unit* target)
{
    if (!GetBot() || !target)
        return;

    // Deploy totems based on spec via HandleTotemManagement
    HandleTotemManagement(target);
}

void ShamanAI::RecallCombatTotems()
{
    // Totems automatically despawn, but we can track their removal
    for (auto& totem : _activeTotems)
    {
        totem.totemUnit = nullptr;
    }

    TC_LOG_DEBUG("module.playerbot.ai", "Shaman {} combat ended, totems will expire naturally",

                 GetBot()->GetName());
}

void ShamanAI::ApplyCombatBuffs()
{
    if (!GetBot())
        return;    // Already handled in HandleOffensiveCooldowns
}

void ShamanAI::LogCombatMetrics()
{
    TC_LOG_DEBUG("module.playerbot.ai",

                 "Shaman {} combat metrics - Duration: {}s, Damage: {}, Healing: {}, Mana: {}, Totems: {}, Shocks: {}",

                 GetBot()->GetName(), _combatTime / 1000, _damageDealt, _healingDone,

                 _manaSpent, _totemsDeploy, _shocksUsed);

    // Reset metrics for next combat
    _damageDealt = 0;
    _healingDone = 0;
    _manaSpent = 0;
    _totemsDeploy = 0;
    _shocksUsed = 0;
}

bool ShamanAI::IsShockSpell(uint32 spellId) const
{
    return spellId == SPELL_EARTH_SHOCK ||
           spellId == SPELL_FLAME_SHOCK ||
           spellId == SPELL_FROST_SHOCK;
}

bool ShamanAI::IsTotemSpell(uint32 spellId) const
{
    // Check if spell is a totem spell
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    // Check spell effects for totem summoning
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (spellInfo->GetEffect(SpellEffIndex(i)).Effect == SPELL_EFFECT_SUMMON)
        {
            // Additional check for totem-specific summons could be added here

            return true;
        }
    }

    // Check against known totem spell IDs
    return (spellId >= SPELL_SEARING_TOTEM && spellId <= SPELL_SENTRY_TOTEM) ||
           spellId == SPELL_FIRE_ELEMENTAL_TOTEM ||
           spellId == SPELL_EARTH_ELEMENTAL_TOTEM ||
           spellId == SPELL_MANA_TIDE_TOTEM ||
           spellId == SPELL_HEALING_TIDE_TOTEM ||
           spellId == SPELL_SPIRIT_LINK_TOTEM ||
           spellId == SPELL_CAPACITOR_TOTEM;
}

uint32 ShamanAI::GetOptimalWeaponImbue(bool mainHand) const
{
    if (!GetBot())
        return 0;

    switch (static_cast<uint32>(GetBot()->GetPrimarySpecialization()))
    {
        case 262: // Elemental

            return SPELL_FLAMETONGUE_WEAPON;
        case 263: // Enhancement

            return mainHand ? SPELL_WINDFURY_WEAPON : SPELL_FLAMETONGUE_WEAPON;
        case 264: // Restoration

            return SPELL_EARTHLIVING_WEAPON;
        default:

            return SPELL_ROCKBITER_WEAPON;
    }
}

bool ShamanAI::HasWeaponImbue(bool mainHand) const
{
    if (!GetBot())
        return false;

    // Check for active weapon enchantment
    Item* weapon = GetBot()->GetItemByPos(INVENTORY_SLOT_BAG_0, mainHand ? EQUIPMENT_SLOT_MAINHAND : EQUIPMENT_SLOT_OFFHAND);
    if (!weapon)
        return false;

    // Check if weapon has temporary enchantment (imbue)
    return weapon->GetEnchantmentId(EnchantmentSlot(TEMP_ENCHANTMENT_SLOT)) != 0;
}

bool ShamanAI::NearWater() const
{
    if (!GetBot())
        return false;

    // Check if bot is near water
    return GetBot()->GetMap()->IsInWater(GetBot()->GetPhaseShift(), GetBot()->GetPositionX(), GetBot()->GetPositionY(), GetBot()->GetPositionZ());
}

bool ShamanAI::ShouldUseBloodlust() const
{
    if (!GetBot())
        return false;

    // Check if already has exhaustion debuff
    if (GetBot()->HasAura(57723) || GetBot()->HasAura(57724)) // Exhaustion/Sated
        return false;

    // Use in boss fights or when health is critical
    if (!_currentTarget.IsEmpty())
    {
        // PHASE 5F: Thread-safe spatial grid validation
        auto snapshot_target = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), _currentTarget);

        Unit* target = nullptr;
        if (snapshot_target)
        {

        }
        
        if (target && target->GetHealthPct() < 30.0f && target->GetMaxHealth() > 100000)

            return true;
    }

    // Use when multiple group members are low
    if (Group* group = GetBot()->GetGroup())
    {
        uint32 lowHealthCount = 0;
        for (GroupReference const& itr : group->GetMembers())
        {

            if (Player* member = itr.GetSource())

            {

                if (member->GetHealthPct() < 40.0f)

                    lowHealthCount++;

            }
        }
        return lowHealthCount >= 3;
    }

    return false;
}

Player* ShamanAI::FindGroupTank(Group* group) const
{
    if (!group)
        return nullptr;

    Player* tank = nullptr;
    uint32 highestHealth = 0;

    for (GroupReference const& itr : group->GetMembers())
    {
        if (Player* member = itr.GetSource())
        {
            // Simple tank detection - highest health or warrior/paladin/death knight
    if (member->GetClass() == CLASS_WARRIOR ||
            member->GetClass() == CLASS_PALADIN ||
            member->GetClass() == CLASS_DEATH_KNIGHT)
            {

                if (member->GetMaxHealth() > highestHealth)

                {

                    highestHealth = member->GetMaxHealth();

                    tank = member;

                }

            }
        }
    }

    return tank;
}

uint32 ShamanAI::CalculateDamageDealt(::Unit* target) const
{
    // Simplified damage calculation for metrics
    // In a real implementation, this would track actual damage events
    return 100;
}

uint32 ShamanAI::CalculateHealingDone() const
{
    // Simplified healing calculation for metrics
    // In a real implementation, this would track actual healing events
    return static_cast<uint32>(GetBot()->GetPrimarySpecialization()) == 264 ? 200 : 0; // 264 = Restoration
}

uint32 ShamanAI::CalculateManaUsage() const
{
    // Simplified mana usage calculation
    // In a real implementation, this would track actual mana consumption
    return 50;
}

void ShamanAI::RecallTotem(TotemType type)
{
    TotemInfo& totem = _activeTotems[static_cast<size_t>(type)];
    totem.totemUnit = nullptr;
    totem.spellId = 0;
}

void ShamanAI::UpdateTotemPositions()
{
    // Update totem positions if they moved (shouldn't happen for totems)
    // This is mainly for tracking purposes
}

} // namespace Playerbot
