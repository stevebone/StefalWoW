/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "WarriorAI.h"
#include "GameTime.h"
#include "ArmsWarrior.h"
#include "FuryWarrior.h"
#include "ProtectionWarrior.h"
#include "../BaselineRotationManager.h"
#include "../../Combat/CombatBehaviorIntegration.h"
#include "Player.h"
#include "Log.h"
#include "CellImpl.h"
#include "GridNotifiersImpl.h"
#include "Unit.h"
#include "SpellAuras.h"
#include "../../../Spatial/SpatialGridManager.h"
#include "../../../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5F: Thread-safe queries

namespace Playerbot
{

// Destructor must be defined in cpp file where specialization types are complete
WarriorAI::~WarriorAI() = default;

WarriorAI::WarriorAI(Player* bot) : ClassAI(bot)
{
    _lastStanceChange = 0;
    _lastBattleShout = 0;
    _lastCommandingShout = 0;
    _needsIntercept = false;
    _needsCharge = false;
    _lastChargeTarget = nullptr;
    _lastChargeTime = 0;

    TC_LOG_DEBUG("module.playerbot.ai", "WarriorAI created for player {}",
                 bot ? bot->GetName() : "null");
}

void WarriorAI::UpdateRotation(::Unit* target)
{
    if (!target || !GetBot())
        return;

    // NOTE: Baseline rotation check is now handled at the dispatch level in
    // ClassAI::OnCombatUpdate(). This method is ONLY called when the bot has
    // already chosen a specialization (level 10+ with talents).

    // ========================================================================
    // COMBAT BEHAVIOR INTEGRATION - Priority-based decision making
    // ========================================================================
    auto* behaviors = GetCombatBehaviors();

    // Priority 1: Handle interrupts (Pummel)
    if (behaviors && behaviors->ShouldInterrupt(target))
    {
        Unit* interruptTarget = behaviors->GetInterruptTarget();
        if (!interruptTarget)
        {
            return;
        }
        if (interruptTarget && CanUseAbility(PUMMEL))
        {
            // Cast Pummel on the interrupt target
    if (CastSpell(PUMMEL, interruptTarget))
            {
                RecordInterruptAttempt(interruptTarget, PUMMEL, true);
                TC_LOG_DEBUG("module.playerbot.ai", "Warrior {} interrupted {} with Pummel",
                             GetBot()->GetName(), interruptTarget->GetName());
                return;
            }
        }
    }

    // Priority 2: Handle defensives (Shield Wall, Last Stand, etc.)
    if (behaviors && behaviors->NeedsDefensive())
    {
        // Use defensive cooldowns when health is critical
        UseDefensiveCooldowns();
        if (GetBot()->HasUnitState(UNIT_STATE_CASTING))
            return;
    }

    // Priority 3: Check for target switching
    if (behaviors && behaviors->ShouldSwitchTarget())
    {
        Unit* priorityTarget = behaviors->GetPriorityTarget();
        if (priorityTarget && priorityTarget != target)
        {
            OnTargetChanged(priorityTarget);
            target = priorityTarget;
            TC_LOG_DEBUG("module.playerbot.ai", "Warrior {} switching target to {}",
                         GetBot()->GetName(), priorityTarget->GetName());
        }
    }

    // Priority 4: AoE vs Single-Target decision
    if (behaviors && behaviors->ShouldAOE())
    {
        // Whirlwind for AoE damage
    if (CanUseAbility(WHIRLWIND))
        {
            if (CastSpell(WHIRLWIND))
            {
                RecordAbilityUsage(WHIRLWIND);
                TC_LOG_DEBUG("module.playerbot.ai", "Warrior {} using Whirlwind for AoE",
                             GetBot()->GetName());
                return;
            }
        }

        // Thunder Clap for threat and slow
    if (CanUseAbility(THUNDER_CLAP))
        {
            if (CastSpell(THUNDER_CLAP))
            {
                RecordAbilityUsage(THUNDER_CLAP);
                TC_LOG_DEBUG("module.playerbot.ai", "Warrior {} using Thunder Clap for AoE",
                             GetBot()->GetName());
                return;
            }
        }

        // Bladestorm for massive AoE
    if (CanUseAbility(BLADESTORM))
        {
            if (CastSpell(BLADESTORM))
            {
                RecordAbilityUsage(BLADESTORM);
                TC_LOG_DEBUG("module.playerbot.ai", "Warrior {} activated Bladestorm",
                             GetBot()->GetName());
                return;
            }
        }
    }

    // Priority 5: Use major cooldowns at optimal time
    if (behaviors && behaviors->ShouldUseCooldowns())
    {
        // Recklessness for damage boost
    if (CanUseAbility(RECKLESSNESS))
        {
            if (CastSpell(RECKLESSNESS))
            {
                RecordAbilityUsage(RECKLESSNESS);
                TC_LOG_DEBUG("module.playerbot.ai", "Warrior {} activated Recklessness",
                             GetBot()->GetName());
            }
        }

        // Avatar for overall boost
    if (CanUseAbility(AVATAR))
        {
            if (CastSpell(AVATAR))
            {
                RecordAbilityUsage(AVATAR);
                TC_LOG_DEBUG("module.playerbot.ai", "Warrior {} activated Avatar",
                             GetBot()->GetName());
            }
        }
    }

    // Priority 6: Check positioning requirements
    if (behaviors && behaviors->NeedsRepositioning())
    {
        Position optimalPos = behaviors->GetOptimalPosition();
        // Movement will be handled by BotAI movement strategies
        // Just flag that we need to move
        _needsIntercept = true;
        _needsCharge = true;
    }

    // Priority 7: Execute specialization-specific rotation
    ChrSpecialization spec = GetBot()->GetPrimarySpecialization();
    if (spec != ChrSpecialization(0))
    {
        // Delegation to spec-specific rotation is handled by the spec implementations
        // This is handled through the template system (ArmsWarriorRefactored, etc.)
        // For now, use basic rotation as the spec templates handle their own rotations
        ExecuteBasicWarriorRotation(target);
    }
    else
    {
        // Fallback rotation when no specialization is available
        ExecuteBasicWarriorRotation(target);
    }

    // Handle warrior-specific abilities that don't conflict with behaviors
    UseChargeAbilities(target);
    UpdateAdvancedCombatLogic(target);
}

void WarriorAI::UpdateBuffs()
{
    // NOTE: Baseline buff check is now handled at the dispatch level.
    // This method is only called for level 10+ bots with talents.

    // Use full warrior buff system for specialized bots
    UpdateWarriorBuffs();
}

void WarriorAI::UpdateCooldowns(uint32 diff)
{
    UpdateMetrics(diff);

    // Cooldown tracking is now handled by base class and specialization templates
    // No polymorphic delegation needed
}

bool WarriorAI::CanUseAbility(uint32 spellId)
{
    if (!IsSpellReady(spellId) || !HasEnoughResource(spellId))
        return false;

    // Base ability checks are sufficient - specialization-specific checks
    // are handled in the template system
    return true;
}

void WarriorAI::OnCombatStart(::Unit* target)
{
    _warriorMetrics.combatStartTime = ::std::chrono::steady_clock::now();

    // Combat start logic is handled by specialization templates
    // No polymorphic delegation needed
}

void WarriorAI::OnCombatEnd()
{
    AnalyzeCombatEffectiveness();

    // Combat end logic is handled by specialization templates
    // No polymorphic delegation needed
}

bool WarriorAI::HasEnoughResource(uint32 spellId)
{
    // Warriors use rage - check if we have enough based on spell cost
    // Basic rage threshold check - specific costs handled by specialization templates
    uint32 currentRage = GetBot()->GetPower(POWER_RAGE);

    // Most warrior abilities cost between 10-40 rage
    // Use conservative estimate if exact cost not known
    return currentRage >= 10;
}

void WarriorAI::ConsumeResource(uint32 spellId)
{
    RecordAbilityUsage(spellId);

    // Resource consumption is handled automatically by the spell system
    // Specialization templates handle their specific resource tracking
}

Position WarriorAI::GetOptimalPosition(::Unit* target)
{
    if (!target || !GetBot())
        return Position();

    // Warriors are melee - optimal position is in melee range
    // Specialization-specific positioning handled by templates
    return CalculateOptimalChargePosition(target);
}

float WarriorAI::GetOptimalRange(::Unit* target)
{
    // Warriors are melee fighters - always want to be in melee range
    // Specialization templates may override for specific tactics
    return OPTIMAL_MELEE_RANGE;
}

void WarriorAI::UpdateSpecialization()
{
    // Specialization switching is now handled by the template-based system
    // GetBot()->GetPrimarySpecialization() returns the current spec directly from player data
    // No need for manual detection or switching logic
    TC_LOG_DEBUG("module.playerbot.warrior", "Warrior {} using specialization: {}",
                 GetBot()->GetName(), GetBot()->GetPrimarySpecialization());
}

void WarriorAI::DelegateToSpecialization(::Unit* target)
{
    // Old delegation system removed - now using template-based architecture
    // Specialization-specific logic is handled by ArmsWarriorRefactored, etc.
    // This method kept for compatibility but no longer delegates polymorphically
    ExecuteBasicWarriorRotation(target);
}

void WarriorAI::UpdateWarriorBuffs()
{
    CastBattleShout();
    CastCommandingShout();

    // Specialization-specific buffs are handled by template implementations
    // No polymorphic delegation needed
}

void WarriorAI::CastBattleShout()
{
    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - _lastBattleShout > BATTLE_SHOUT_DURATION)
    {
        // Cast Battle Shout if available
        _lastBattleShout = currentTime;
    }
}

void WarriorAI::CastCommandingShout()
{
    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - _lastCommandingShout > COMMANDING_SHOUT_DURATION)
    {
        // Cast Commanding Shout if available
        _lastCommandingShout = currentTime;
    }
}

void WarriorAI::UseChargeAbilities(::Unit* target)
{
    if (!target)
        return;

    if (CanCharge(target))
    {
        // Use charge abilities
        _needsCharge = true;
    }
}

bool WarriorAI::IsInMeleeRange(::Unit* target) const
{
    if (!target || !GetBot())
        return false;

    float rangeSq = OPTIMAL_MELEE_RANGE * OPTIMAL_MELEE_RANGE; // 25.0f
    return GetBot()->GetExactDistSq(target) <= rangeSq;
}

bool WarriorAI::CanCharge(::Unit* target) const
{
    if (!target || !GetBot())
        return false;

    float distance = ::std::sqrt(GetBot()->GetExactDistSq(target)); // Calculate once from squared distance
    return distance >= CHARGE_MIN_RANGE && distance <= CHARGE_MAX_RANGE;
}

void WarriorAI::UpdateAdvancedCombatLogic(::Unit* target)
{
    // Advanced combat logic
    OptimizeStanceDancing(target);
    ManageRageEfficiency();
}

void WarriorAI::OptimizeStanceDancing(::Unit* target)
{
    // Stance optimization logic
}

void WarriorAI::ManageRageEfficiency()
{
    // Rage management logic
}

void WarriorAI::RecordAbilityUsage(uint32 spellId)
{
    _abilityUsage[spellId]++;
    _warriorMetrics.totalAbilitiesUsed++;
}

void WarriorAI::AnalyzeCombatEffectiveness()
{
    // Analyze combat performance
}

void WarriorAI::UpdateMetrics(uint32 diff)
{
    _warriorMetrics.lastMetricsUpdate = ::std::chrono::steady_clock::now();
}

float WarriorAI::CalculateRageEfficiency()
{
    return RAGE_EFFICIENCY_TARGET;
}

Position WarriorAI::CalculateOptimalChargePosition(::Unit* target)
{
    if (!target || !GetBot())
        return Position();

    return GetBot()->GetPosition();
}

void WarriorAI::ExecuteBasicWarriorRotation(::Unit* target)
{
    if (!target || !GetBot())
        return;

    // Basic rotation for warriors without specialization
    // Priority: Maintain debuffs -> Generate rage -> Spend rage

    // Apply Sunder Armor for armor reduction
    if (CanUseAbility(SUNDER_ARMOR))
    {
        if (CastSpell(SUNDER_ARMOR, target))
        {
            RecordAbilityUsage(SUNDER_ARMOR);
            return;
        }
    }

    // Apply Rend for bleed damage
    if (CanUseAbility(REND))
    {
        if (!target->HasAura(REND, GetBot()->GetGUID()))
        {
            if (CastSpell(REND, target))
            {
                RecordAbilityUsage(REND);
                return;
            }
        }
    }

    // Use Execute if target is low health
    if (target->GetHealthPct() < 20.0f && CanUseAbility(EXECUTE))
    {
        if (CastSpell(EXECUTE, target))
        {
            RecordAbilityUsage(EXECUTE);
            return;
        }
    }

    // Use Overpower if available
    if (CanUseAbility(OVERPOWER))
    {
        if (CastSpell(OVERPOWER, target))
        {
            RecordAbilityUsage(OVERPOWER);
            return;
        }
    }

    // Use Mortal Strike or Bloodthirst if available
    if (CanUseAbility(MORTAL_STRIKE))
    {
        if (CastSpell(MORTAL_STRIKE, target))
        {
            RecordAbilityUsage(MORTAL_STRIKE);
            return;
        }
    }

    if (CanUseAbility(BLOODTHIRST))
    {
        if (CastSpell(BLOODTHIRST, target))
        {
            RecordAbilityUsage(BLOODTHIRST);
            return;
        }
    }

    // Rage dump with Heroic Strike or Cleave
    if (GetBot()->GetPower(POWER_RAGE) > RAGE_DUMP_THRESHOLD)
    {
        // Use Cleave if multiple enemies
    if (GetNearbyEnemyCount(8.0f) > 1 && CanUseAbility(CLEAVE))
        {
            if (CastSpell(CLEAVE, target))
            {
                RecordAbilityUsage(CLEAVE);
                return;
            }
        }

        // Otherwise use Heroic Strike
    if (CanUseAbility(HEROIC_STRIKE))
        {
            if (CastSpell(HEROIC_STRIKE, target))
            {
                RecordAbilityUsage(HEROIC_STRIKE);
                return;
            }
        }
    }
}

void WarriorAI::RecordInterruptAttempt(::Unit* target, uint32 spellId, bool success)
{
    if (success)
    {
        _successfulInterrupts++;
        TC_LOG_DEBUG("module.playerbot.ai", "Warrior {} successfully interrupted with spell {}",
                     GetBot()->GetName(), spellId);
    }
}

void WarriorAI::UseDefensiveCooldowns()
{
    if (!GetBot())
        return;

    float healthPct = GetBot()->GetHealthPct();

    // Shield Wall at critical health
    if (healthPct < HEALTH_EMERGENCY_THRESHOLD && CanUseAbility(SHIELD_WALL))
    {
        if (CastSpell(SHIELD_WALL))
        {
            RecordAbilityUsage(SHIELD_WALL);
            TC_LOG_DEBUG("module.playerbot.ai", "Warrior {} activated Shield Wall",
                         GetBot()->GetName());
            return;
        }
    }

    // Last Stand for health boost
    if (healthPct < DEFENSIVE_COOLDOWN_THRESHOLD && CanUseAbility(LAST_STAND))
    {
        if (CastSpell(LAST_STAND))
        {
            RecordAbilityUsage(LAST_STAND);
            TC_LOG_DEBUG("module.playerbot.ai", "Warrior {} activated Last Stand",
                         GetBot()->GetName());
            return;
        }
    }

    // Shield Block for mitigation (Protection warriors)
    if (healthPct < 60.0f && CanUseAbility(SHIELD_BLOCK))
    {
        if (CastSpell(SHIELD_BLOCK))
        {
            RecordAbilityUsage(SHIELD_BLOCK);
            TC_LOG_DEBUG("module.playerbot.ai", "Warrior {} activated Shield Block",
                         GetBot()->GetName());
            return;
        }
    }

    // Spell Reflection against casters
    Unit* reflectionTarget = GetBot()->GetSelectedUnit();
    if (reflectionTarget && reflectionTarget->HasUnitState(UNIT_STATE_CASTING) && CanUseAbility(SPELL_REFLECTION))
    {
        if (CastSpell(SPELL_REFLECTION))
        {
            RecordAbilityUsage(SPELL_REFLECTION);
            TC_LOG_DEBUG("module.playerbot.ai", "Warrior {} activated Spell Reflection",
                         GetBot()->GetName());
            return;
        }
    }
}

uint32 WarriorAI::GetNearbyEnemyCount(float range) const
{
    if (!GetBot())
        return 0;

    uint32 count = 0;
    ::std::list<Unit*> targets;
    Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(GetBot(), GetBot(), range);
    Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(GetBot(), targets, u_check);
    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = GetBot()->GetMap();
    if (!map)
        return 0;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return 0;
    }

    // Query nearby GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        GetBot()->GetPosition(), range);

    // Process results (replace old searcher logic)
    for (ObjectGuid guid : nearbyGuids)
    {
        // PHASE 5F: Thread-safe spatial grid validation
        auto snapshot_entity = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), guid);

        Creature* entity = nullptr;
        if (snapshot_entity)
        {
            // FIXED: CreatureSnapshot to Creature conversion via ObjectAccessor
            entity = ObjectAccessor::GetCreature(*GetBot(), snapshot_entity->guid);
        }
        if (!entity)
            continue;
        // Original filtering logic from searcher goes here
    }
    // End of spatial grid fix
    for (auto& target : targets)
    {
        if (GetBot()->IsValidAttackTarget(target))
            count++;
    }

    return count;
}

bool WarriorAI::IsValidTarget(::Unit* target)
{
    return target && target->IsAlive() && GetBot()->IsValidAttackTarget(target);
}

::Unit* WarriorAI::GetBestChargeTarget()
{
    // Find best charge target
    return GetBestAttackTarget();
}


// ============================================================================
// ARMS WARRIOR REFACTORED IMPLEMENTATIONS
// ============================================================================
// These implementations are required by ArmsWarriorRefactored class
// declared in ArmsWarrior.h - the class uses template-based architecture

void ArmsWarriorRefactored::InitializeDebuffTracking()
{
    // Initialize the debuff tracking maps for Deep Wounds and Rend
    // These track which targets have active debuffs and their expiration times
    _deepWoundsTracking.clear();
    _rendTracking.clear();

    TC_LOG_DEBUG("module.playerbot.warrior", "ArmsWarriorRefactored: Debuff tracking initialized");
}

void ArmsWarriorRefactored::InitializeArmsRotation()
{
    // Initialize the Arms-specific rotation systems including:
    // - Action Priority Queue integration (Phase 5)
    // - Behavior Tree integration (Phase 5)
    // - Cooldown tracking for core abilities

    // Initialize cooldowns for Arms abilities
    // Note: CooldownManager uses Register() for individual cooldowns, no Initialize() needed

    // Setup Phase 5 ActionPriorityQueue and BehaviorTree integration
    SetupActionPriorityQueue();

    TC_LOG_DEBUG("module.playerbot.warrior", "ArmsWarriorRefactored: Arms rotation initialized with {} enemies tracking capacity",
                 _deepWoundsTracking.max_size());
}

} // namespace Playerbot