/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "RogueAI.h"
#include "GameTime.h"
#include "../BaselineRotationManager.h"
#include "AssassinationRogue.h"
#include "OutlawRogue.h"
#include "SubtletyRogue.h"
#include "../../Combat/CombatBehaviorIntegration.h"
#include "Player.h"
#include "Group.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "Item.h"
#include "Creature.h"
#include "ObjectAccessor.h"
#include "Map.h"
#include "WorldSession.h"
#include "../Combat/BotThreatManager.h"
#include "../Combat/TargetSelector.h"
#include "../Combat/PositionManager.h"
#include "../Combat/InterruptManager.h"
#include "../Common/CooldownManager.h"
#include "CellImpl.h"
#include "GridNotifiersImpl.h"
#include "Log.h"
#include <atomic>
#include <chrono>
#include <algorithm>
#include <unordered_map>
#include "../../../Spatial/SpatialGridManager.h"
#include "../../../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5F: Thread-safe queries

namespace Playerbot
{

// Performance metrics structure
struct RogueMetrics
{
    ::std::atomic<uint32> totalEnergySpent{0};
    ::std::atomic<uint32> totalComboPointsGenerated{0};
    ::std::atomic<uint32> totalFinishersExecuted{0};
    ::std::atomic<uint32> stealthOpeners{0};
    ::std::atomic<uint32> poisonApplications{0};
    ::std::atomic<uint32> interruptsExecuted{0};
    ::std::atomic<uint32> backstabsLanded{0};
    ::std::atomic<uint32> cooldownsUsed{0};
    ::std::atomic<float> averageReactionTime{0};
    ::std::atomic<float> energyEfficiency{0};
    ::std::chrono::steady_clock::time_point lastUpdate;

    void Reset()
    {
        totalEnergySpent = 0;
        totalComboPointsGenerated = 0;
        totalFinishersExecuted = 0;
        stealthOpeners = 0;
        poisonApplications = 0;
        interruptsExecuted = 0;
        backstabsLanded = 0;
        cooldownsUsed = 0;
        averageReactionTime = 0;
        energyEfficiency = 0;
        lastUpdate = ::std::chrono::steady_clock::now();
    }

    void UpdateReactionTime(float deltaMs)
    {
        float current = averageReactionTime.load();
        averageReactionTime = (current * 0.9f) + (deltaMs * 0.1f);
    }

    void UpdateEnergyEfficiency(uint32 energyUsed, uint32 comboGenerated)
    {
        if (energyUsed > 0)
        {

            float efficiency = static_cast<float>(comboGenerated) / energyUsed * 100.0f;

            float current = energyEfficiency.load();

            energyEfficiency = (current * 0.8f) + (efficiency * 0.2f);
        }
    }
};

// Combat metrics tracking
class RogueCombatMetrics
{
public:
    void RecordAbilityUsage(uint32 spellId, bool success, uint32 energyCost = 0)
    {
        auto now = ::std::chrono::steady_clock::now();
        _abilityTimings[spellId] = now;

        if (success)
        {

            _successfulCasts[spellId]++;

            _totalEnergyUsed += energyCost;

            // Track finisher usage
    if (IsFinisher(spellId))

                _finisherCount++;
        }
        else

            _failedCasts[spellId]++;

        _lastGCD = now;
    }

    void RecordComboPointGeneration(uint32 points)
    {
        _totalComboPoints += points;
        _comboPointHistory.push_back({::std::chrono::steady_clock::now(), points});
    }

    float GetAbilitySuccessRate(uint32 spellId) const
    {
        auto successful = _successfulCasts.find(spellId);
        auto failed = _failedCasts.find(spellId);

        uint32 total = 0;
        uint32 success = 0;

        if (successful != _successfulCasts.end())
        {

            success = successful->second;

            total += success;
        }

        if (failed != _failedCasts.end())

            total += failed->second;

        return total > 0 ? (float)success / total : 0.0f;
    }

    bool IsOnGlobalCooldown() const
    {
        auto now = ::std::chrono::steady_clock::now();
        auto elapsed = ::std::chrono::duration_cast<::std::chrono::milliseconds>(now - _lastGCD);
        return elapsed.count() < 1000; // 1 second GCD for rogues
    }

private:
    bool IsFinisher(uint32 spellId) const
    {
        // WoW 12.0 finishers - EXPOSE_ARMOR removed from game
        return spellId == RogueAI::SLICE_AND_DICE || spellId == RogueAI::RUPTURE ||
               spellId == RogueAI::EVISCERATE || spellId == RogueAI::KIDNEY_SHOT ||
               spellId == RogueAI::ENVENOM || spellId == RogueAI::CRIMSON_TEMPEST ||
               spellId == RogueAI::BETWEEN_THE_EYES;
    }

    ::std::unordered_map<uint32, ::std::chrono::steady_clock::time_point> _abilityTimings;
    ::std::unordered_map<uint32, uint32> _successfulCasts;
    ::std::unordered_map<uint32, uint32> _failedCasts;
    ::std::chrono::steady_clock::time_point _lastGCD;
    uint32 _totalEnergyUsed = 0;
    uint32 _totalComboPoints = 0;
    uint32 _finisherCount = 0;

    struct ComboPointEvent
    {
        ::std::chrono::steady_clock::time_point time;
        uint32 points;
    };
    ::std::vector<ComboPointEvent> _comboPointHistory;
};

// ============================================================================
// ROGUE COMBAT POSITIONING IMPLEMENTATION
// ============================================================================

Position RogueCombatPositioning::CalculateOptimalPosition(Unit* target, RogueSpec spec)
        {
    if (!target || !_bot)        return _bot->GetPosition();

    // Calculate position based on specialization requirements
    switch (spec)
    {
        case RogueSpec::ASSASSINATION:
        case RogueSpec::SUBTLETY:
            // Assassination and Subtlety prefer behind target for Backstab/Ambush

            {

                float angle = target->GetOrientation() + static_cast<float>(M_PI); // 180 degrees behind
                float distance = 3.0f; // Close melee range

                float x = target->GetPositionX() + distance * ::std::cos(angle);
                float y = target->GetPositionY() + distance * ::std::sin(angle);
                

                float z = target->GetPositionZ();
                return Position(x, y, z, angle);

            }

        case RogueSpec::COMBAT:
            // Combat can attack from any angle, prefer frontal positioning

            {

                float angle = target->GetOrientation(); // Face to face
                float distance = 4.0f; // Slightly further for Blade Flurry AoE

                float x = target->GetPositionX() + distance * ::std::cos(angle);
                float y = target->GetPositionY() + distance * ::std::sin(angle);
                float z = target->GetPositionZ();

                return Position(x, y, z, target->GetOrientation());

            }

        default:

            return _bot->GetPosition();
    }
}

float RogueCombatPositioning::GetOptimalRange(RogueSpec spec) const
{
    switch (spec)
    {
        case RogueSpec::ASSASSINATION:

            return 3.0f; // Close range for Mutilate/Envenom

        case RogueSpec::COMBAT:

            return 5.0f; // Standard melee range, benefits Blade Flurry

        case RogueSpec::SUBTLETY:

            return 3.5f; // Close range for Backstab/Hemorrhage

        default:

            return 5.0f; // Default melee range
    }
}

// ============================================================================
// ROGUE AI IMPLEMENTATION
// ============================================================================

RogueAI::RogueAI(Player* bot) :
    ClassAI(bot),
    _energySpent(0),
    _comboPointsUsed(0),
    _stealthsUsed(0),
    _lastStealth(0),
    _lastVanish(0)
{
    // Initialize combat systems
    InitializeCombatSystems();

    // Initialize performance tracking
    _metrics = new RogueMetrics();
    _combatMetrics = new RogueCombatMetrics();
    _positioning = new RogueCombatPositioning(bot);
    TC_LOG_DEBUG("playerbot", "RogueAI initialized for bot {}", bot->GetGUID().GetCounter());
}

void RogueAI::InitializeCombatSystems()
{
    // Initialize advanced combat system components
    _threatManager = ::std::make_unique<BotThreatManager>(GetBot());
    _targetSelector = ::std::make_unique<TargetSelector>(GetBot(), _threatManager.get());
    _positionManager = ::std::make_unique<PositionManager>(GetBot(), _threatManager.get());
    _interruptManager = ::std::make_unique<InterruptManager>(GetBot());
    _cooldownManager = ::std::make_unique<CooldownManager>();

    TC_LOG_DEBUG("playerbot", "RogueAI combat systems initialized for bot {}", GetBot()->GetGUID().GetCounter());
}

void RogueAI::UpdateRotation(Unit* target)
{
    if (!target || !GetBot())
        return;

    // NOTE: Baseline rotation check is now handled at the dispatch level in
    // ClassAI::OnCombatUpdate(). This method is ONLY called when the bot has
    // already chosen a specialization (level 10+ with talents).

    auto startTime = ::std::chrono::steady_clock::now();

    // Check if we're on global cooldown
    if (_combatMetrics->IsOnGlobalCooldown())
        return;

    // ========================================================================
    // COMBAT BEHAVIOR INTEGRATION - Priority-based decision making
    // ========================================================================
    auto* behaviors = GetCombatBehaviors();

    // Priority 1: Handle interrupts (Kick)
    if (behaviors && behaviors->ShouldInterrupt(target))
    {
        Unit* interruptTarget = behaviors->GetInterruptTarget();

                             if (!interruptTarget)

                             {

                                 return;

                             }
        if (interruptTarget && CanUseAbility(KICK))
        {
            // Cast Kick on the interrupt target
    if (CastSpell(KICK, interruptTarget))

            {

                RecordInterruptAttempt(interruptTarget, KICK, true);

                _metrics->interruptsExecuted++;

                TC_LOG_DEBUG("module.playerbot.ai", "Rogue {} interrupted {} with Kick",

                             GetBot()->GetName(), interruptTarget->GetName());

                return;

            }
        }
    }

    // Priority 2: Handle defensives (Evasion, Cloak of Shadows, Feint)
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

                         if (!priorityTarget)

                         {

                             return;

                         }

            TC_LOG_DEBUG("module.playerbot.ai", "Rogue {} switching target to {}",

                         GetBot()->GetName(), priorityTarget->GetName());
        }
    }

    // Priority 4: AoE vs Single-Target decision
    if (behaviors && behaviors->ShouldAOE())
    {
        // Blade Flurry for Combat/Outlaw rogues (check via specialization)
        uint32 spec = static_cast<uint32>(GetBot()->GetPrimarySpecialization());
        if (static_cast<uint32>(spec) == 1 && CanUseAbility(BLADE_FLURRY)) // Combat/Outlaw is spec 1
        {

            if (CastSpell(BLADE_FLURRY))

            {

                RecordAbilityUsage(BLADE_FLURRY);

                TC_LOG_DEBUG("module.playerbot.ai", "Rogue {} activated Blade Flurry for AoE",

                             GetBot()->GetName());

                return;

            }
        }

        // Fan of Knives for AoE combo point generation
    if (CanUseAbility(FAN_OF_KNIVES))
        {

            if (CastSpell(FAN_OF_KNIVES))

            {

                RecordAbilityUsage(FAN_OF_KNIVES);

                _combatMetrics->RecordAbilityUsage(FAN_OF_KNIVES, true, 35);

                _combatMetrics->RecordComboPointGeneration(GetNearbyEnemyCount(10.0f));

                TC_LOG_DEBUG("module.playerbot.ai", "Rogue {} using Fan of Knives for AoE",

                             GetBot()->GetName());

                return;

            }
        }
    }

    // Priority 5: Use major cooldowns at optimal time
    if (behaviors && behaviors->ShouldUseCooldowns())
    {
        // Spec-specific offensive cooldowns
        uint32 spec = static_cast<uint32>(GetBot()->GetPrimarySpecialization());
        switch (spec)
        {

            case 0: // Assassination
                // Vendetta for damage amplification
    if (CanUseAbility(VENDETTA))

                {

                    if (CastSpell(VENDETTA, target))

                    {

                        RecordAbilityUsage(VENDETTA);

                        _metrics->cooldownsUsed++;

                        TC_LOG_DEBUG("module.playerbot.ai", "Rogue {} activated Vendetta",

                                     GetBot()->GetName());

                    }

                }
                // Cold Blood for guaranteed crit
    if (CanUseAbility(COLD_BLOOD))

                {

                    if (CastSpell(COLD_BLOOD))

                    {

                        RecordAbilityUsage(COLD_BLOOD);

                        _metrics->cooldownsUsed++;

                        TC_LOG_DEBUG("module.playerbot.ai", "Rogue {} activated Cold Blood",

                                     GetBot()->GetName());

                    }

                }

                break;


            case 1: // Combat/Outlaw
                // Adrenaline Rush for energy regeneration
    if (CanUseAbility(ADRENALINE_RUSH))

                {

                    if (CastSpell(ADRENALINE_RUSH))

                    {

                        RecordAbilityUsage(ADRENALINE_RUSH);

                        _metrics->cooldownsUsed++;

                        TC_LOG_DEBUG("module.playerbot.ai", "Rogue {} activated Adrenaline Rush",

                                     GetBot()->GetName());

                    }

                }
                // Killing Spree for burst damage
    if (CanUseAbility(KILLING_SPREE))

                {

                    if (CastSpell(KILLING_SPREE, target))

                    {

                        RecordAbilityUsage(KILLING_SPREE);

                        _metrics->cooldownsUsed++;

                        TC_LOG_DEBUG("module.playerbot.ai", "Rogue {} activated Killing Spree",

                                     GetBot()->GetName());

                        return;

                    }

                }

                break;


            case 2: // Subtlety
                // Shadow Dance for enhanced abilities
    if (CanUseAbility(SHADOW_DANCE))

                {

                    if (CastSpell(SHADOW_DANCE))

                    {

                        RecordAbilityUsage(SHADOW_DANCE);

                        _metrics->cooldownsUsed++;

                        TC_LOG_DEBUG("module.playerbot.ai", "Rogue {} activated Shadow Dance",

                                     GetBot()->GetName());

                    }

                }
                // Shadowstep for mobility and damage
    if (CanUseAbility(SHADOWSTEP))

                {

                    if (CastSpell(SHADOWSTEP, target))

                    {

                        RecordAbilityUsage(SHADOWSTEP);

                        _metrics->cooldownsUsed++;

                        TC_LOG_DEBUG("module.playerbot.ai", "Rogue {} used Shadowstep",

                                     GetBot()->GetName());

                        return;

                    }

                }

                break;
        }
    }    // Priority 6: Stealth and Openers
    if (!GetBot()->IsInCombat() && !HasAura(STEALTH))
    {
        // Enter stealth for opener opportunity
    if (CanUseAbility(STEALTH))
        {

            float distance = GetBot()->GetDistance(target);
            if (distance > 5.0f && distance < 25.0f)
            {
                if (CastSpell(STEALTH, GetBot()))
                {
                    _metrics->stealthOpeners++;
                    _stealthsUsed++;
                    TC_LOG_DEBUG("module.playerbot.ai", "Rogue {} entering Stealth for opener",

                                 GetBot()->GetName());

                    return;

                }

            }
        }
    }

    // Check for stealth opener opportunities
    if (HasAura(STEALTH) || HasAura(VANISH) || HasAura(SHADOW_DANCE))
    {
        if (ExecuteStealthOpener(target))

            return;
    }    // Priority 7: Execute normal rotation (all specs use baseline logic in RogueAI)
    ExecuteRogueBasicRotation(target);

    // Update performance metrics
    auto endTime = ::std::chrono::steady_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime);
    _metrics->UpdateReactionTime(duration.count() / 1000.0f);
}

void RogueAI::ExecuteRogueBasicRotation(Unit* target)
{
    if (!target || !GetBot())
        return;

    float distance = GetBot()->GetDistance(target);
    uint32 energy = GetBot()->GetPower(POWER_ENERGY);
    uint8 comboPoints = GetBot()->GetPower(POWER_COMBO_POINTS);

    // Basic rotation for rogues without specialization
    // Priority: Maintain buffs -> Apply debuffs -> Spend combo points -> Build combo points

    // Maintain Slice and Dice buff
    if (comboPoints >= 2 && GetAuraRemainingTime(SLICE_AND_DICE) < 5000)
    {
        if (CanUseAbility(SLICE_AND_DICE))
        {

            if (CastSpell(SLICE_AND_DICE, target))

            {

                RecordAbilityUsage(SLICE_AND_DICE);

                _metrics->totalFinishersExecuted++;

                return;

            }
        }
    }

    // Apply Rupture for bleed damage
    if (comboPoints >= 3 && !target->HasAura(RUPTURE, GetBot()->GetGUID()))    {
        if (CanUseAbility(RUPTURE))
        {

            if (CastSpell(RUPTURE, target))

            {

                RecordAbilityUsage(RUPTURE);

                _metrics->totalFinishersExecuted++;

                return;

            }
        }
    }

    // NOTE: EXPOSE_ARMOR removed from WoW 12.0
    // Armor reduction is now provided by other debuffs (Colossus Smash, etc.)

    // Kidney Shot for control
    if (comboPoints >= 4 && target->GetTypeId() == TYPEID_PLAYER)    {
        if (CanUseAbility(KIDNEY_SHOT))
        {

            if (CastSpell(KIDNEY_SHOT, target))

            {

                RecordAbilityUsage(KIDNEY_SHOT);

                _metrics->totalFinishersExecuted++;

                return;

            }
        }
    }

    // Eviscerate for damage at 5 combo points
    if (comboPoints >= 5)
    {
        if (CanUseAbility(EVISCERATE))
        {

            if (CastSpell(EVISCERATE, target))

            {

                RecordAbilityUsage(EVISCERATE);

                _metrics->totalFinishersExecuted++;

                return;

            }
        }
    }

    // Build combo points
    if (energy >= 40)
    {
        // Try to get behind target for Backstab
    if (_positioning->IsBehindTarget(target) && CanUseAbility(BACKSTAB))
        {

            if (CastSpell(BACKSTAB, target))

            {

                RecordAbilityUsage(BACKSTAB);

                _combatMetrics->RecordAbilityUsage(BACKSTAB, true, 60);

                _combatMetrics->RecordComboPointGeneration(1);

                _metrics->backstabsLanded++;

                return;

            }
        }

        // Use Sinister Strike as default builder
    if (CanUseAbility(SINISTER_STRIKE))
        {

            if (CastSpell(SINISTER_STRIKE, target))

            {

                RecordAbilityUsage(SINISTER_STRIKE);

                _combatMetrics->RecordAbilityUsage(SINISTER_STRIKE, true, 45);

                _combatMetrics->RecordComboPointGeneration(1);

                return;

            }
        }
    }
}

void RogueAI::RecordInterruptAttempt(Unit* target, uint32 spellId, bool success)
{
    if (success)
    {
        TC_LOG_DEBUG("module.playerbot.ai", "Rogue {} successfully interrupted {} with spell {}",

                     GetBot()->GetName(), target->GetName(), spellId);
    }
}

void RogueAI::UseDefensiveCooldowns()
{
    if (!GetBot())
        return;

    float healthPct = GetBot()->GetHealthPct();

    // Evasion for physical damage mitigation
    if (healthPct < 30.0f && CanUseAbility(EVASION))
    {
        if (CastSpell(EVASION, GetBot()))
        {
            RecordAbilityUsage(EVASION);
            TC_LOG_DEBUG("module.playerbot.ai", "Rogue {} activated Evasion (defensive)",
                         GetBot()->GetName());
            return;
        }
    }

    // Cloak of Shadows for magic damage and debuff removal
    Unit* currentTarget = GetBot()->GetSelectedUnit();
    if (currentTarget && currentTarget->HasUnitState(UNIT_STATE_CASTING))
    {
        if (CanUseAbility(CLOAK_OF_SHADOWS))
        {

            if (CastSpell(CLOAK_OF_SHADOWS))

            {

                RecordAbilityUsage(CLOAK_OF_SHADOWS);

                TC_LOG_DEBUG("module.playerbot.ai", "Rogue {} activated Cloak of Shadows",

                             GetBot()->GetName());

                return;

            }
        }
    }

    // Feint for AoE damage reduction
    if (healthPct < 50.0f && CanUseAbility(FEINT))
    {
        if (CastSpell(FEINT))
        {
            RecordAbilityUsage(FEINT);
            TC_LOG_DEBUG("module.playerbot.ai", "Rogue {} used Feint for damage reduction",
                         GetBot()->GetName());
            return;
        }
    }

    // Vanish for emergency escape
    if (healthPct < 20.0f && CanUseAbility(VANISH))
    {
        if (CastSpell(VANISH))
        {

            RecordAbilityUsage(VANISH);

            _lastVanish = GameTime::GetGameTimeMS();

            TC_LOG_DEBUG("module.playerbot.ai", "Rogue {} used Vanish (emergency escape)",

                         GetBot()->GetName());

            return;
        }
    }

    // Combat Readiness for damage reduction (Combat spec)
    uint32 spec = static_cast<uint32>(GetBot()->GetPrimarySpecialization());
    if (static_cast<uint32>(spec) == 1 && healthPct < 40.0f) // Combat/Outlaw is spec 1
    {
        uint32 combatReadiness = 74001; // Combat Readiness spell ID
    if (CanUseAbility(combatReadiness))
        {

            if (CastSpell(combatReadiness))

            {

                RecordAbilityUsage(combatReadiness);

                TC_LOG_DEBUG("module.playerbot.ai", "Rogue {} activated Combat Readiness",

                             GetBot()->GetName());

                return;

            }
        }
    }
}

uint32 RogueAI::GetNearbyEnemyCount(float range) const
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
    for (auto& target : targets)
    {
        if (GetBot()->IsValidAttackTarget(target))

            count++;
    }

    return count;
}

void RogueAI::RecordAbilityUsage(uint32 spellId)
{
    // Record ability usage for performance tracking
    TC_LOG_DEBUG("module.playerbot.ai", "Rogue {} used ability {}",

                 GetBot()->GetName(), spellId);
}

void RogueAI::OnTargetChanged(Unit* newTarget)
{
    if (!newTarget)
        return;

    // Reset combo points tracking for new target
    _comboPointsUsed = 0;

    TC_LOG_DEBUG("module.playerbot.ai", "Rogue {} changed target to {}",

                 GetBot()->GetName(), newTarget->GetName());
}

void RogueAI::ExecuteFallbackRotation(Unit* target)
{
    if (!target || !GetBot())
        return;

    float distance = GetBot()->GetDistance(target);
    uint32 energy = GetBot()->GetPower(POWER_ENERGY);
    uint8 comboPoints = GetBot()->GetPower(POWER_COMBO_POINTS);

    // Stealth management
    if (!GetBot()->IsInCombat() && !HasAura(STEALTH) && distance > 10.0f)
    {
        if (CanUseAbility(STEALTH))
        {
            CastSpell(STEALTH, GetBot());
            _metrics->stealthOpeners++;
            return;
        }
    }

    // Stealth opener
    if (HasAura(STEALTH) && distance <= 5.0f)
    {
        if (ExecuteStealthOpener(target))

            return;
    }

    // Normal combat rotation
    if (distance <= 5.0f)
    {
        // Maintain Slice and Dice
        if (comboPoints >= 2 && !HasAura(SLICE_AND_DICE))
        {
            if (CanUseAbility(SLICE_AND_DICE))
            {
                CastSpell(SLICE_AND_DICE, target);
                _metrics->totalFinishersExecuted++;
                return;
            }
        }

        // Use finishers at 5 combo points
    if (comboPoints >= 5)
        {

            if (ExecuteFinisher(target))

                return;
        }

        // Build combo points
    if (energy >= 40)        {

            if (BuildComboPoints(target))

                return;
        }
    }

    // Handle interrupts
    if (target->HasUnitState(UNIT_STATE_CASTING))
    {
        if (_interruptManager->IsSpellInterruptWorthy(target->GetCurrentSpell(CURRENT_GENERIC_SPELL) ? target->GetCurrentSpell(CURRENT_GENERIC_SPELL)->GetSpellInfo()->Id : 0, target))
        {

            if (CanUseAbility(KICK))

            {

                CastSpell(KICK, target);

                _metrics->interruptsExecuted++;

                return;

            }
        }
    }
}

bool RogueAI::ExecuteStealthOpener(Unit* target)
{
    if (!target || !HasAura(STEALTH))
        return false;

    // Priority: Cheap Shot > Ambush > Garrote
    if (CanUseAbility(CHEAP_SHOT))
    {
        CastSpell(CHEAP_SHOT, target);
        _combatMetrics->RecordAbilityUsage(CHEAP_SHOT, true, 40);
        return true;
    }

    if (_positioning->IsBehindTarget(target) && CanUseAbility(AMBUSH))
    {
        CastSpell(AMBUSH, target);
        _combatMetrics->RecordAbilityUsage(AMBUSH, true, 60);
        return true;
    }

    if (CanUseAbility(GARROTE))
    {
        CastSpell(GARROTE, target);
        _combatMetrics->RecordAbilityUsage(GARROTE, true, 50);
        return true;
    }

    return false;
}

bool RogueAI::ExecuteFinisher(Unit* target)
{
    if (!target)
        return false;

    uint8 comboPoints = GetBot()->GetPower(POWER_COMBO_POINTS);
    if (comboPoints < 1)
        return false;

    // Maintain Slice and Dice
    if (GetAuraRemainingTime(SLICE_AND_DICE) < 5000 && comboPoints >= 2)
    {
        if (CanUseAbility(SLICE_AND_DICE))
        {

            CastSpell(SLICE_AND_DICE, target);

            _combatMetrics->RecordAbilityUsage(SLICE_AND_DICE, true, 25);

            _metrics->totalFinishersExecuted++;

            return true;
        }
    }

    // Apply/Refresh Rupture for longer fights
    if (target->GetHealthPct() > 35.0f && !HasAura(RUPTURE, target))
    {
        if (CanUseAbility(RUPTURE))
        {

            CastSpell(RUPTURE, target);

            _combatMetrics->RecordAbilityUsage(RUPTURE, true, 25);

            _metrics->totalFinishersExecuted++;

            return true;
        }
    }

    // Kidney Shot for control
    if (target->GetTypeId() == TYPEID_PLAYER && CanUseAbility(KIDNEY_SHOT))    {
        CastSpell(KIDNEY_SHOT, target);
        _combatMetrics->RecordAbilityUsage(KIDNEY_SHOT, true, 25);
        _metrics->totalFinishersExecuted++;
        return true;
    }

    // Eviscerate for burst damage
    if (CanUseAbility(EVISCERATE))
    {
        CastSpell(EVISCERATE, target);
        _combatMetrics->RecordAbilityUsage(EVISCERATE, true, 35);
        _metrics->totalFinishersExecuted++;
        return true;
    }

    return false;
}

bool RogueAI::BuildComboPoints(Unit* target)
{
    if (!target)
        return false;

    bool behindTarget = _positioning->IsBehindTarget(target);

    // Backstab if behind
    if (behindTarget && CanUseAbility(BACKSTAB))
    {
        CastSpell(BACKSTAB, target);
        _combatMetrics->RecordAbilityUsage(BACKSTAB, true, 60);
        _combatMetrics->RecordComboPointGeneration(1);
        _metrics->backstabsLanded++;
        return true;
    }

    // Spec-specific builders
    uint32 spec = static_cast<uint32>(GetBot()->GetPrimarySpecialization());
    switch (spec)
    {
        case 0: // Assassination
    if (CanUseAbility(MUTILATE))

            {

                CastSpell(MUTILATE, target);

                _combatMetrics->RecordAbilityUsage(MUTILATE, true, 60);

                _combatMetrics->RecordComboPointGeneration(2);

                return true;

            }

            break;

        case 2: // Subtlety
            // NOTE: HEMORRHAGE removed from WoW 12.0
            // Subtlety now uses Shadowstrike (from stealth/Shadow Dance) and Backstab
            if (CanUseAbility(SHADOWSTRIKE))
            {
                CastSpell(SHADOWSTRIKE, target);
                _combatMetrics->RecordAbilityUsage(SHADOWSTRIKE, true, 40);
                _combatMetrics->RecordComboPointGeneration(2);
                return true;
            }
            break;

        case 1: // Combat/Outlaw
            // Outlaw prefers Sinister Strike
            break;
    }

    // Default to Sinister Strike
    if (CanUseAbility(SINISTER_STRIKE))
    {
        CastSpell(SINISTER_STRIKE, target);
        _combatMetrics->RecordAbilityUsage(SINISTER_STRIKE, true, 45);
        _combatMetrics->RecordComboPointGeneration(1);
        return true;
    }

    return false;
}

void RogueAI::UpdateBuffs()
{
    if (!GetBot())
        return;

    // NOTE: Baseline buff check is now handled at the dispatch level.
    // This method is only called for level 10+ bots with talents.

    uint32 currentTime = GameTime::GetGameTimeMS();

    // Apply poisons (check every 30 seconds)
    if (currentTime - _lastPoison > 30000)
    {
        ApplyPoisons();
        _lastPoison = currentTime;
    }

    // Maintain stealth out of combat
    if (!GetBot()->IsInCombat() && currentTime - _lastStealth > 10000)
    {
        if (!HasAura(STEALTH) && CanUseAbility(STEALTH))
        {
            CastSpell(STEALTH, GetBot());
            _lastStealth = currentTime;
        }
    }

    // Spec-specific buffs are handled within individual ability checks
    // No delegation needed - RogueAI handles all buff logic directly
}

void RogueAI::ApplyPoisons()
{
    Item* mainHand = GetBot()->GetItemByPos(INVENTORY_SLOT_BAG_0);
    Item* offHand = GetBot()->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);

    // Main hand poison based on spec
    if (mainHand && !mainHand->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT))
    {
        uint32 spec = static_cast<uint32>(GetBot()->GetPrimarySpecialization());
        uint32 poisonSpell = 0;
        switch (spec)
        {

            case 0: // Assassination

                poisonSpell = DEADLY_POISON;

                break;

            case 1: // Combat/Outlaw

                poisonSpell = INSTANT_POISON;

                break;

            case 2: // Subtlety

                poisonSpell = WOUND_POISON;

                break;
        }

        if (poisonSpell && CanUseAbility(poisonSpell))
        {

            CastSpell(poisonSpell);

            _metrics->poisonApplications++;

            TC_LOG_DEBUG("playerbot", "RogueAI: Applied main hand poison");
        }
    }

    // Off hand poison
    if (offHand && offHand->GetTemplate()->GetClass() == ITEM_CLASS_WEAPON &&
        !offHand->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT))
    {
        uint32 spec = static_cast<uint32>(GetBot()->GetPrimarySpecialization());
        uint32 poisonSpell = 0;
        switch (spec)
        {

            case 0: // Assassination

                poisonSpell = INSTANT_POISON;

                break;

            case 1: // Combat/Outlaw

                poisonSpell = CRIPPLING_POISON;

                break;

            case 2: // Subtlety
                // NOTE: MIND_NUMBING_POISON renamed to NUMBING_POISON in WoW 12.0
                poisonSpell = NUMBING_POISON;
                break;
        }

        if (poisonSpell && CanUseAbility(poisonSpell))
        {

            CastSpell(poisonSpell);

            _metrics->poisonApplications++;

            TC_LOG_DEBUG("playerbot", "RogueAI: Applied off hand poison");
        }
    }
}

void RogueAI::UpdateCooldowns(uint32 diff)
{
    if (!GetBot())
        return;

    // Update cooldown manager
    if (_cooldownManager)
        _cooldownManager->Update(diff);

    // No specialization delegation needed
    // All cooldowns are tracked via CooldownManager
}

bool RogueAI::CanUseAbility(uint32 spellId)
{
    if (!GetBot())
        return false;

    // Check basic requirements
    if (!GetBot()->HasSpell(spellId))
        return false;

    if (!IsSpellReady(spellId))
        return false;

    if (!HasEnoughResource(spellId))
        return false;

    // All checks passed
    return true;
}

bool RogueAI::HasEnoughResource(uint32 spellId)
{
    if (!GetBot())
        return false;

    const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(spellId, GetBot()->GetMap()->GetDifficultyID());
    if (!spellInfo)
        return false;    // Check energy cost
    auto powerCosts = spellInfo->CalcPowerCost(GetBot(), spellInfo->GetSchoolMask());
    for (auto const& cost : powerCosts)
    {
        if (cost.Power == POWER_ENERGY)
        {

            uint32 currentEnergy = GetBot()->GetPower(POWER_ENERGY);

            if (currentEnergy < cost.Amount)

                return false;
        }
    }

    // Check combo point requirements for finishers
    if (IsFinisher(spellId))
    {        uint8 comboPoints = GetBot()->GetPower(POWER_COMBO_POINTS);
        if (comboPoints < 1)

            return false;
    }

    // Note: Specialization has its own resource tracking internally
    // No need to delegate here as RogueAI handles resource checks above

    return true;
}

bool RogueAI::IsFinisher(uint32 spellId) const
{
    // WoW 12.0 finishers - EXPOSE_ARMOR and DEADLY_THROW removed from game
    return spellId == SLICE_AND_DICE || spellId == RUPTURE ||
           spellId == EVISCERATE || spellId == KIDNEY_SHOT ||
           spellId == ENVENOM || spellId == CRIMSON_TEMPEST ||
           spellId == BETWEEN_THE_EYES;
}

void RogueAI::ConsumeResource(uint32 spellId)
{
    if (!GetBot())
        return;

    const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(spellId, GetBot()->GetMap()->GetDifficultyID());
    if (!spellInfo)
        return;

    // Track energy consumption
    auto powerCosts = spellInfo->CalcPowerCost(GetBot(), spellInfo->GetSchoolMask());
    for (auto const& cost : powerCosts)
    {
        if (cost.Power == POWER_ENERGY)
        {

            _metrics->totalEnergySpent += cost.Amount;

            _energySpent += cost.Amount;
        }
    }

    // Track combo point consumption for finishers
    if (IsFinisher(spellId))
    {
        _metrics->totalFinishersExecuted++;
        _comboPointsUsed++;
    }

    // Track combo point generation
    if (!IsFinisher(spellId))
    {
        _metrics->totalComboPointsGenerated++;
    }

    // Note: Specialization handles its own internal resource consumption
    // No need to delegate here as RogueAI tracks metrics above
}

void RogueAI::OnCombatStart(Unit* target)
{
    if (!target || !GetBot())
        return;

    ClassAI::OnCombatStart(target);

    // Reset combat metrics
    _energySpent = 0;
    _comboPointsUsed = 0;
    _stealthsUsed = 0;

    // Open from stealth if possible
    if (HasAura(STEALTH))
    {
        ExecuteStealthOpener(target);
    }

    // Use offensive cooldowns for boss fights
    if (target->GetTypeId() == TYPEID_UNIT && target->ToCreature()->isWorldBoss())    {
        ActivateBurstCooldowns(target);
    }

    // Use defensive cooldowns if low health
    if (GetBot()->GetHealthPct() < 50.0f)
    {
        if (CanUseAbility(EVASION))
        {

            CastSpell(EVASION);

            TC_LOG_DEBUG("playerbot", "RogueAI: Activated Evasion (defensive)");
        }
    }

    TC_LOG_DEBUG("playerbot", "RogueAI: Combat started against {}", target->GetName());
}

void RogueAI::OnCombatEnd()
{
    ClassAI::OnCombatEnd();

    // Calculate combat metrics
    if (_energySpent > 0)
    {
        float efficiency = static_cast<float>(_comboPointsUsed) / _energySpent;
        _metrics->UpdateEnergyEfficiency(_energySpent, _comboPointsUsed);
    }

    // Re-stealth after combat
    if (!HasAura(STEALTH) && CanUseAbility(STEALTH))
    {
        CastSpell(STEALTH, GetBot());
    }

    TC_LOG_DEBUG("playerbot", "RogueAI: Combat ended. Energy spent: {}, CP generated: {}, Finishers: {}",

                 _energySpent, _comboPointsUsed, _metrics->totalFinishersExecuted.load());
}

void RogueAI::ActivateBurstCooldowns(Unit* target)
{
    if (!target)
        return;

    uint32 spec = static_cast<uint32>(GetBot()->GetPrimarySpecialization());
    switch (spec)
    {
        case 0: // Assassination
            if (CanUseAbility(COLD_BLOOD))
            {
                CastSpell(COLD_BLOOD, GetBot());
                _metrics->cooldownsUsed++;
            }
            if (CanUseAbility(VENDETTA))
            {
                CastSpell(VENDETTA, target);
                _metrics->cooldownsUsed++;
            }
            break;

        case 1: // Combat/Outlaw
            if (CanUseAbility(BLADE_FLURRY))
            {
                CastSpell(BLADE_FLURRY, GetBot());
                _metrics->cooldownsUsed++;
            }
            if (CanUseAbility(ADRENALINE_RUSH))
            {
                CastSpell(ADRENALINE_RUSH, GetBot());
                _metrics->cooldownsUsed++;
            }
            if (CanUseAbility(KILLING_SPREE))
            {
                CastSpell(KILLING_SPREE, target);
                _metrics->cooldownsUsed++;
            }
            break;

        case 2: // Subtlety
            if (CanUseAbility(SHADOW_DANCE))
            {
                CastSpell(SHADOW_DANCE, GetBot());
                _metrics->cooldownsUsed++;
            }
            if (CanUseAbility(SHADOWSTEP))
            {
                CastSpell(SHADOWSTEP, target);
                _metrics->cooldownsUsed++;
            }
            break;
    }
}

Position RogueAI::GetOptimalPosition(Unit* target)
{
    if (!target || !GetBot())
        return GetBot()->GetPosition();

    // Use positioning manager for advanced calculations
    if (_positioning)
    {
        // Convert spec ID to RogueSpec enum
        uint32 specId = static_cast<uint32>(GetBot()->GetPrimarySpecialization());
        RogueSpec rogueSpec = static_cast<RogueSpec>(specId);
        return _positioning->CalculateOptimalPosition(target, rogueSpec);
    }

    return ClassAI::GetOptimalPosition(target);
}

float RogueAI::GetOptimalRange(Unit* target)
{
    if (!target)
        return 5.0f;

    // Use positioning manager
    if (_positioning)
    {
        // Convert spec ID to RogueSpec enum
        uint32 specId = static_cast<uint32>(GetBot()->GetPrimarySpecialization());
        RogueSpec rogueSpec = static_cast<RogueSpec>(specId);
        return _positioning->GetOptimalRange(rogueSpec);
    }

    return 5.0f; // Default melee range
}

RogueAI::~RogueAI()
{
    // Cleanup is handled by smart pointers for unique_ptr members
    delete _metrics;
    delete _combatMetrics;
    delete _positioning;
}

// Helper methods implementation
void RogueAI::ConsiderStealth()
{
    if (!GetBot() || GetBot()->IsInCombat())
        return;

    if (!HasAura(STEALTH) && CanUseAbility(STEALTH))
    {
        CastSpell(STEALTH, GetBot());
        _stealthsUsed++;
    }
}

bool RogueAI::HasEnoughEnergy(uint32 amount)
{
    return GetBot() && GetBot()->GetPower(POWER_ENERGY) >= amount;
}

uint32 RogueAI::GetEnergy()
{
    return GetBot() ? GetBot()->GetPower(POWER_ENERGY) : 0;
}

uint32 RogueAI::GetComboPoints()
{
    return GetBot() ? GetBot()->GetPower(POWER_COMBO_POINTS) : 0;
}

} // namespace Playerbot