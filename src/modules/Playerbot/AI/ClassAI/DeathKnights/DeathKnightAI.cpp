/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "RuneManager.h"
#include "GameTime.h"
#include "DeathKnightAI.h"
#include "BloodDeathKnight.h"
#include "FrostDeathKnight.h"
#include "UnholyDeathKnight.h"
#include "../SpellValidation_WoW120.h"
#include "../../Combat/CombatBehaviorIntegration.h"
#include "Player.h"
#include "Group.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "Item.h"
#include "Creature.h"
#include "ObjectAccessor.h"
#include "WorldSession.h"
#include "Map.h"
#include "SpellAuras.h"
#include "CellImpl.h"
#include "GridNotifiersImpl.h"
#include "../Combat/BotThreatManager.h"
#include "../Combat/TargetSelector.h"
#include "../Combat/PositionManager.h"
#include "../Combat/InterruptManager.h"
#include "../BaselineRotationManager.h"
#include "Log.h"
#include <atomic>
#include <chrono>
#include <algorithm>
#include <unordered_map>
#include "../../../Spatial/SpatialGridManager.h"
#include "../../../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5F: Thread-safe queries

namespace Playerbot
{

// Death Knight rune types - using the ones from DeathKnightSpecialization.h
static constexpr uint8 RUNE_BLOOD = 0;
static constexpr uint8 RUNE_FROST = 1;
static constexpr uint8 RUNE_UNHOLY = 2;
static constexpr uint8 RUNE_DEATH = 3;

// Performance metrics structure
struct DeathKnightMetrics
{
    ::std::atomic<uint32> totalRunicPowerGenerated{0};
    ::std::atomic<uint32> totalRunicPowerSpent{0};
    ::std::atomic<uint32> totalRunesUsed{0};
    ::std::atomic<uint32> diseasesApplied{0};
    ::std::atomic<uint32> deathStrikesUsed{0};
    ::std::atomic<uint32> deathGripsUsed{0};
    ::std::atomic<uint32> interruptsExecuted{0};
    ::std::atomic<uint32> cooldownsUsed{0};
    ::std::atomic<float> averageReactionTime{0};
    ::std::atomic<float> runeEfficiency{0};
    ::std::atomic<float> diseaseUptime{0};
    ::std::chrono::steady_clock::time_point lastUpdate;

    void Reset()
    {
        totalRunicPowerGenerated = 0;
        totalRunicPowerSpent = 0;
        totalRunesUsed = 0;
        diseasesApplied = 0;
        deathStrikesUsed = 0;
        deathGripsUsed = 0;
        interruptsExecuted = 0;
        cooldownsUsed = 0;
        averageReactionTime = 0;
        runeEfficiency = 0;
        diseaseUptime = 0;
        lastUpdate = ::std::chrono::steady_clock::now();
    }

    void UpdateReactionTime(float deltaMs)
    {
        float current = averageReactionTime.load();
        averageReactionTime = (current * 0.9f) + (deltaMs * 0.1f);
    }

    void UpdateRuneEfficiency(uint32 runesUsed, uint32 powerGenerated)
    {
        if (runesUsed > 0)
        {

            float efficiency = static_cast<float>(powerGenerated) / runesUsed;

            float current = runeEfficiency.load();

            runeEfficiency = (current * 0.8f) + (efficiency * 0.2f);
        }
    }

    void UpdateDiseaseUptime(float currentUptime)
    {
        float current = diseaseUptime.load();
        diseaseUptime = (current * 0.95f) + (currentUptime * 0.05f);
    }
};

// ============================================================================
// Death Knight Spell IDs - Using Central SpellValidation Registry (WoW 12.0)
// ============================================================================
// All spell IDs sourced from SpellValidation_WoW120.h to maintain single source of truth

// Namespace aliases for cleaner code
namespace DKSpells = WoW120Spells::DeathKnight;
namespace DKBlood = WoW120Spells::DeathKnight::Blood;
namespace DKFrost = WoW120Spells::DeathKnight::Frost;
namespace DKUnholy = WoW120Spells::DeathKnight::Unholy;

// Core Abilities (All Specs) - WoW 12.0
static constexpr uint32 DK_DEATH_STRIKE = DKSpells::DEATH_STRIKE;
static constexpr uint32 DK_DEATH_AND_DECAY = DKSpells::DEATH_AND_DECAY;
static constexpr uint32 DK_DEATH_GRIP = DKSpells::DEATH_GRIP;
static constexpr uint32 DK_ANTI_MAGIC_SHELL = DKSpells::ANTI_MAGIC_SHELL;
static constexpr uint32 DK_ANTI_MAGIC_ZONE = DKSpells::ANTI_MAGIC_ZONE;
static constexpr uint32 DK_ICEBOUND_FORTITUDE = DKSpells::ICEBOUND_FORTITUDE;
static constexpr uint32 DK_CHAINS_OF_ICE = DKSpells::CHAINS_OF_ICE;
static constexpr uint32 DK_MIND_FREEZE = DKSpells::MIND_FREEZE;
static constexpr uint32 DK_PATH_OF_FROST = DKSpells::PATH_OF_FROST;
static constexpr uint32 DK_RAISE_DEAD = DKSpells::RAISE_DEAD;
static constexpr uint32 DK_DEATH_COIL = DKSpells::DEATH_COIL;
static constexpr uint32 DK_DARK_COMMAND = DKSpells::DARK_COMMAND;
static constexpr uint32 DK_ASPHYXIATE = DKSpells::ASPHYXIATE;
static constexpr uint32 DK_DEATHS_ADVANCE = DKSpells::DEATHS_ADVANCE;

// Blood Specialization - WoW 12.0
static constexpr uint32 DK_HEART_STRIKE = DKBlood::HEART_STRIKE;
static constexpr uint32 DK_BLOOD_BOIL = DKBlood::BLOOD_BOIL;
static constexpr uint32 DK_RUNE_TAP = DKBlood::RUNE_TAP;
static constexpr uint32 DK_VAMPIRIC_BLOOD = DKBlood::VAMPIRIC_BLOOD;
static constexpr uint32 DK_DANCING_RUNE_WEAPON = DKBlood::DANCING_RUNE_WEAPON;
static constexpr uint32 DK_BONE_SHIELD = DKBlood::BONE_SHIELD;
static constexpr uint32 DK_BLOOD_PLAGUE = DKBlood::BLOOD_PLAGUE;
static constexpr uint32 DK_MARROWREND = DKBlood::MARROWREND;
static constexpr uint32 DK_DEATHS_CARESS = DKBlood::DEATHS_CARESS;

// Frost Specialization - WoW 12.0
static constexpr uint32 DK_FROST_STRIKE = DKFrost::FROST_STRIKE;
static constexpr uint32 DK_HOWLING_BLAST = DKFrost::HOWLING_BLAST;
static constexpr uint32 DK_OBLITERATE = DKFrost::OBLITERATE;
static constexpr uint32 DK_PILLAR_OF_FROST = DKFrost::PILLAR_OF_FROST;
static constexpr uint32 DK_EMPOWER_RUNE_WEAPON = DKFrost::EMPOWER_RUNE_WEAPON;
static constexpr uint32 DK_FROST_FEVER = DKFrost::FROST_FEVER;
static constexpr uint32 DK_HORN_OF_WINTER = DKFrost::HORN_OF_WINTER;
static constexpr uint32 DK_REMORSELESS_WINTER = DKFrost::REMORSELESS_WINTER;

// Unholy Specialization - WoW 12.0
static constexpr uint32 DK_SCOURGE_STRIKE = DKUnholy::SCOURGE_STRIKE;
static constexpr uint32 DK_EPIDEMIC = DKUnholy::EPIDEMIC;
static constexpr uint32 DK_ARMY_OF_THE_DEAD = DKUnholy::ARMY_OF_THE_DEAD;
static constexpr uint32 DK_SUMMON_GARGOYLE = DKUnholy::SUMMON_GARGOYLE;
static constexpr uint32 DK_UNHOLY_ASSAULT = DKUnholy::UNHOLY_ASSAULT;
static constexpr uint32 DK_OUTBREAK = DKUnholy::OUTBREAK;
static constexpr uint32 DK_FESTERING_STRIKE = DKUnholy::FESTERING_STRIKE;
static constexpr uint32 DK_DARK_TRANSFORMATION = DKUnholy::DARK_TRANSFORMATION;

// Death Knight constants
constexpr float OPTIMAL_MELEE_RANGE = 5.0f;
constexpr float DEATH_GRIP_MIN_RANGE = 10.0f;
constexpr float DEATH_GRIP_MAX_RANGE = 30.0f;
constexpr float DEFENSIVE_COOLDOWN_THRESHOLD = 40.0f;
constexpr float HEALTH_EMERGENCY_THRESHOLD = 20.0f;
constexpr uint32 RUNIC_POWER_DUMP_THRESHOLD = 80;
constexpr uint32 PRESENCE_CHECK_INTERVAL = 5000;
constexpr uint32 HORN_CHECK_INTERVAL = 30000;
constexpr uint32 DEATH_GRIP_COOLDOWN = 25000;
constexpr uint32 DARK_COMMAND_COOLDOWN = 8000;

// Rune types defined above

// Rune management system removed - using external RuneManager class

// Combat metrics tracking
class DeathKnightCombatMetrics
{
public:
    void RecordAbilityUsage(uint32 spellId, bool success, uint32 runesUsed = 0, uint32 powerCost = 0)
    {
        auto now = ::std::chrono::steady_clock::now();
        _abilityTimings[spellId] = now;

        if (success)
        {

            _successfulCasts[spellId]++;

            _totalRunesUsed += runesUsed;

            _totalRunicPowerUsed += powerCost;
        }
        else

            _failedCasts[spellId]++;

        _lastGCD = now;
    }

    void RecordRunicPowerGeneration(uint32 power)
    {
        _totalRunicPowerGenerated += power;
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
        return elapsed.count() < 1500; // 1.5 second GCD
    }

private:
    ::std::unordered_map<uint32, ::std::chrono::steady_clock::time_point> _abilityTimings;
    ::std::unordered_map<uint32, uint32> _successfulCasts;
    ::std::unordered_map<uint32, uint32> _failedCasts;
    ::std::chrono::steady_clock::time_point _lastGCD;
    uint32 _totalRunesUsed = 0;
    uint32 _totalRunicPowerUsed = 0;
    uint32 _totalRunicPowerGenerated = 0;
};

// Combat positioning strategy
class DeathKnightCombatPositioning
{
public:
    explicit DeathKnightCombatPositioning(Player* bot) : _bot(bot) {}    Position CalculateOptimalPosition(Unit* target, DeathKnightSpec detectedSpec)
    {
        if (!target || !_bot)

            return _bot->GetPosition();

        Position optimalPos = _bot->GetPosition();
        float currentDistance = ::std::sqrt(_bot->GetExactDistSq(target)); // Calculate once from squared distance
    switch (detectedSpec)
        {

            case DeathKnightSpec::BLOOD:
                // Tank positioning - in front of target
    if (currentDistance > 5.0f)

                {

                    float angle = target->GetOrientation();
                    optimalPos.m_positionX = target->GetPositionX() + cos(angle) * 3.0f;
                    optimalPos.m_positionY = target->GetPositionY() + sin(angle) * 3.0f;
                    optimalPos.m_positionZ = target->GetPositionZ();
                    }

                break;


            case DeathKnightSpec::FROST:
                // Melee DPS positioning - behind or to the side
    if (currentDistance > 5.0f)

                {

                    float angle = target->GetOrientation() + static_cast<float>(M_PI); // Behind target
                    optimalPos.m_positionX = target->GetPositionX() + cos(angle) * 3.0f;
                    optimalPos.m_positionY = target->GetPositionY() + sin(angle) * 3.0f;
                    optimalPos.m_positionZ = target->GetPositionZ();
                    }

                break;


            case DeathKnightSpec::UNHOLY:
                // Flexible positioning with pet management
    if (currentDistance > 5.0f && currentDistance < 30.0f)

                {
                    // Stay at mid-range for Death Coil

                    float angle = target->GetRelativeAngle(_bot);
                    optimalPos.m_positionX = target->GetPositionX() + cos(angle) * 10.0f;
                    optimalPos.m_positionY = target->GetPositionY() + sin(angle) * 10.0f;
                    optimalPos.m_positionZ = target->GetPositionZ();
                    }

                else if (currentDistance > 30.0f)

                {
                    // Use Death Grip range

                    float angle = target->GetRelativeAngle(_bot);

                    optimalPos.m_positionX = target->GetPositionX() + cos(angle) * 25.0f;
                    optimalPos.m_positionY = target->GetPositionY() + sin(angle) * 25.0f;
                    optimalPos.m_positionZ = target->GetPositionZ();
                    }

                break;
        }

        return optimalPos;
    }

    float GetOptimalRange(DeathKnightSpec detectedSpec) const
    {
        switch (detectedSpec)
        {

            case DeathKnightSpec::BLOOD:

                return 5.0f; // Tank in melee

            case DeathKnightSpec::FROST:

                return 5.0f; // Melee DPS

            case DeathKnightSpec::UNHOLY:

                return 10.0f; // Flexible range

            default:

                return 5.0f;
        }
    }

private:
    Player* _bot;
};

DeathKnightAI::DeathKnightAI(Player* bot) :
    ClassAI(bot),
    _detectedSpec(DeathKnightSpec::BLOOD),
    _runicPowerSpent(0),
    _runesUsed(0),
    _diseasesApplied(0),
    _lastPresence(0),
    _lastHorn(0),
    _lastDeathGrip(0),
    _lastDarkCommand(0),
    _successfulInterrupts(0)
{
    // Initialize combat systems
    InitializeCombatSystems();

    // Detect specialization
    DetectSpecialization();

    // Initialize performance tracking
    _metrics = ::std::make_unique<DeathKnightMetrics>();
    _combatMetrics = ::std::make_unique<DeathKnightCombatMetrics>();
    _runeManager = ::std::make_unique<RuneManager>(bot);
    _diseaseManager = ::std::make_unique<DiseaseManager>(bot);
    _positioning = ::std::make_unique<DeathKnightCombatPositioning>(bot);
    TC_LOG_DEBUG("playerbot", "DeathKnightAI initialized for bot {} with specialization {}",
                 bot->GetGUID().GetCounter(), static_cast<uint32>(_detectedSpec));
}

void DeathKnightAI::InitializeCombatSystems()
{
    // Initialize advanced combat system components
    _threatManager = ::std::make_unique<BotThreatManager>(GetBot());
    _targetSelector = ::std::make_unique<TargetSelector>(GetBot(), _threatManager.get());
    _positionManager = ::std::make_unique<PositionManager>(GetBot(), _threatManager.get());
    _interruptManager = ::std::make_unique<InterruptManager>(GetBot());
    _cooldownManager = ::std::make_unique<CooldownManager>();

    TC_LOG_DEBUG("playerbot", "DeathKnightAI combat systems initialized for bot {}", GetBot()->GetGUID().GetCounter());
}

void DeathKnightAI::DetectSpecialization()
{
    // Advanced specialization detection based on talents and abilities
    uint32 bloodPoints = 0;
    uint32 frostPoints = 0;
    uint32 unholyPoints = 0;

    // Check key Blood abilities/talents
    if (GetBot()->HasSpell(DK_HEART_STRIKE))
        bloodPoints += 10;
    if (GetBot()->HasSpell(DK_VAMPIRIC_BLOOD))
        bloodPoints += 8;
    if (GetBot()->HasSpell(DK_DANCING_RUNE_WEAPON))
        bloodPoints += 10;
    if (GetBot()->HasSpell(DK_RUNE_TAP))
        bloodPoints += 6;

    // Check key Frost abilities/talents
    if (GetBot()->HasSpell(DK_FROST_STRIKE))
        frostPoints += 10;
    if (GetBot()->HasSpell(DK_HOWLING_BLAST))
        frostPoints += 8;
    if (GetBot()->HasSpell(DK_PILLAR_OF_FROST))
        frostPoints += 10;
    if (GetBot()->HasSpell(DK_REMORSELESS_WINTER))
        frostPoints += 6;

    // Check key Unholy abilities/talents
    if (GetBot()->HasSpell(DK_SCOURGE_STRIKE))
        unholyPoints += 10;
    if (GetBot()->HasSpell(DK_SUMMON_GARGOYLE))
        unholyPoints += 10;
    if (GetBot()->HasSpell(DK_BONE_SHIELD))
        unholyPoints += 8;
    if (GetBot()->HasSpell(DK_UNHOLY_ASSAULT))
        unholyPoints += 6;

    // Determine specialization based on points
    if (bloodPoints >= frostPoints && bloodPoints >= unholyPoints)
        _detectedSpec = DeathKnightSpec::BLOOD;
    else if (frostPoints > bloodPoints && frostPoints >= unholyPoints)
        _detectedSpec = DeathKnightSpec::FROST;
    else
        _detectedSpec = DeathKnightSpec::UNHOLY;

    TC_LOG_DEBUG("playerbot", "DeathKnightAI detected specialization: {} (B:{}, F:{}, U:{})",

                 static_cast<uint32>(_detectedSpec), bloodPoints, frostPoints, unholyPoints);
}

void DeathKnightAI::UpdateRotation(Unit* target)
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

    // Priority 1: Handle interrupts (Mind Freeze/Strangulate)
    if (behaviors && behaviors->ShouldInterrupt(target))
    {
        if (HandleInterrupts(target))

            return;
    }

    // Priority 2: Handle defensives (Icebound Fortitude, Anti-Magic Shell, Vampiric Blood)
    if (behaviors && behaviors->NeedsDefensive())
    {
        if (HandleDefensives())

            return;
    }

    // Priority 3: Check for target switching
    if (behaviors && behaviors->ShouldSwitchTarget())
    {
        if (HandleTargetSwitching(target))

            return;
    }

    // Priority 4: AoE vs Single-Target decision
    if (behaviors && behaviors->ShouldAOE())
    {
        if (HandleAoERotation(target))

            return;
    }

    // Priority 5: Use major cooldowns at optimal time
    if (behaviors && behaviors->ShouldUseCooldowns())
    {
        if (HandleOffensiveCooldowns(target))

            return;
    }

    // Priority 6: Rune and Runic Power Management
    if (HandleRuneAndPowerManagement(target))
        return;

    // Priority 7: Execute normal rotation through specialization or fallback
    ExecuteSpecializationRotation(target);

    // Update performance metrics
    auto startTime = ::std::chrono::steady_clock::now();
    _diseaseManager->UpdateDiseases(target);
    auto endTime = ::std::chrono::steady_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime);
    _metrics->UpdateReactionTime(duration.count() / 1000.0f);
    _metrics->UpdateDiseaseUptime(_diseaseManager->GetDiseaseUptime());
}

void DeathKnightAI::ExecuteFallbackRotation(Unit* target)
{
    if (!target || !GetBot())
        return;

    float distance = ::std::sqrt(GetBot()->GetExactDistSq(target)); // Calculate once from squared distance
    uint32 runicPower = GetBot()->GetPower(POWER_RUNIC_POWER);

    // Apply diseases first
    if (_diseaseManager->NeedsFrostFever(target))
    {
        if (CanUseAbility(DK_HOWLING_BLAST) && _runeManager->HasRunes(0u, 1u, 0u))
        {

            CastSpell(DK_HOWLING_BLAST, target);

            _runeManager->ConsumeRunes(0, 1, 0);

            _metrics->diseasesApplied++;

            _diseasesApplied++;

            return;
        }
    }

    if (_diseaseManager->NeedsBloodPlague(target))
    {
        if (CanUseAbility(DK_OUTBREAK) && _runeManager->HasRunes(0u, 0u, 1u))
        {

            CastSpell(DK_OUTBREAK, target);

            _runeManager->ConsumeRunes(0, 0, 1);

            _metrics->diseasesApplied++;

            _diseasesApplied++;

            return;
        }
    }

    // Use Death Strike for healing if needed
    if (GetBot()->GetHealthPct() < 70.0f)
    {
        if (CanUseAbility(DK_DEATH_STRIKE) && _runeManager->HasRunes(0u, 1u, 1u))
        {

            CastSpell(DK_DEATH_STRIKE, target);

            _runeManager->ConsumeRunes(0, 1, 1);

            _metrics->deathStrikesUsed++;

            return;
        }
    }

    // Main rotation based on range
    if (distance <= 5.0f)
    {
        // Melee range abilities
    if (_diseaseManager->HasBothDiseases(target))
        {
            // Spread diseases with Blood Boil for AoE
    if (CanUseAbility(DK_BLOOD_BOIL) && _runeManager->HasRunes(1u, 0u, 0u))

            {

                CastSpell(DK_BLOOD_BOIL);

                _runeManager->ConsumeRunes(1, 0, 0);

                return;

            }

            // Use spec-specific strike
    switch (_detectedSpec)

            {

                case DeathKnightSpec::BLOOD:

                    if (CanUseAbility(DK_HEART_STRIKE) && _runeManager->HasRunes(1u, 0u, 0u))

                    {

                        CastSpell(DK_HEART_STRIKE, target);

                        _runeManager->ConsumeRunes(1, 0, 0);

                        _combatMetrics->RecordRunicPowerGeneration(15);

                        return;

                    }

                    break;


                case DeathKnightSpec::FROST:

                    if (CanUseAbility(DK_OBLITERATE) && _runeManager->HasRunes(0u, 1u, 1u))

                    {

                        CastSpell(DK_OBLITERATE, target);

                        _runeManager->ConsumeRunes(0, 1, 1);

                        _combatMetrics->RecordRunicPowerGeneration(25);

                        return;

                    }

                    break;


                case DeathKnightSpec::UNHOLY:

                    if (CanUseAbility(DK_SCOURGE_STRIKE) && _runeManager->HasRunes(0u, 0u, 1u))

                    {

                        CastSpell(DK_SCOURGE_STRIKE, target);

                        _runeManager->ConsumeRunes(0, 0, 1);

                        _combatMetrics->RecordRunicPowerGeneration(15);

                        return;

                    }

                    break;

            }
        }

        // Use Blood Strike as fallback
    if (CanUseAbility(DK_HEART_STRIKE) && _runeManager->HasRunes(1u, 0u, 0u))
        {

            CastSpell(DK_HEART_STRIKE, target);

            _runeManager->ConsumeRunes(1, 0, 0);

            return;
        }
    }
    else if (distance > 5.0f && distance <= 30.0f)
    {
        // Death Grip to pull target
    if (distance > 10.0f && CanUseAbility(DK_DEATH_GRIP))
        {

            CastSpell(DK_DEATH_GRIP, target);

            _metrics->deathGripsUsed++;

            return;
        }

        // Death Coil for ranged damage
    if (runicPower >= 40 && CanUseAbility(DK_DEATH_COIL))
        {

            CastSpell(DK_DEATH_COIL, target);

            _metrics->totalRunicPowerSpent += 40;

            _runicPowerSpent += 40;

            return;
        }
    }

    // Runic Power dump abilities
    if (runicPower >= 60)
    {
        switch (_detectedSpec)
        {

            case DeathKnightSpec::FROST:

                if (CanUseAbility(DK_FROST_STRIKE))

                {

                    CastSpell(DK_FROST_STRIKE, target);

                    _metrics->totalRunicPowerSpent += 40;

                    _runicPowerSpent += 40;

                    return;

                }

                break;


            case DeathKnightSpec::BLOOD:

                if (CanUseAbility(DK_FROST_STRIKE))

                {

                    CastSpell(DK_FROST_STRIKE, target);

                    _metrics->totalRunicPowerSpent += 20;

                    _runicPowerSpent += 20;

                    return;

                }

                break;


            case DeathKnightSpec::UNHOLY:

                if (CanUseAbility(DK_DEATH_COIL))

                {

                    CastSpell(DK_DEATH_COIL, target);

                    _metrics->totalRunicPowerSpent += 40;

                    _runicPowerSpent += 40;

                    return;

                }

                break;
        }
    }

    // Handle interrupts
    if (target->HasUnitState(UNIT_STATE_CASTING))
    {
        if (_interruptManager->IsSpellInterruptWorthy(target->GetCurrentSpell(CURRENT_GENERIC_SPELL) ? target->GetCurrentSpell(CURRENT_GENERIC_SPELL)->GetSpellInfo()->Id : 0, target))
        {

            if (distance <= 5.0f && CanUseAbility(DK_MIND_FREEZE))

            {

                CastSpell(DK_MIND_FREEZE, target);

                _metrics->interruptsExecuted++;

                return;

            }

            else if (distance <= 30.0f && CanUseAbility(DK_ASPHYXIATE))

            {

                CastSpell(DK_ASPHYXIATE, target);

                _metrics->interruptsExecuted++;

                return;

            }
        }
    }
}

void DeathKnightAI::UpdateBuffs()
{
    if (!GetBot())
        return;

    uint32 currentTime = GameTime::GetGameTimeMS();

    // NOTE: Presences removed in WoW 12.0 - specializations are automatic

    // Maintain Horn of Winter
    if (currentTime - _lastHorn > 30000 && !HasAura(DK_HORN_OF_WINTER))
    {
        if (CanUseAbility(DK_HORN_OF_WINTER))
        {

            CastSpell(DK_HORN_OF_WINTER);

            _lastHorn = currentTime;
        }
    }

    // Maintain Bone Shield for Blood/Unholy
    if ((_detectedSpec == DeathKnightSpec::BLOOD || _detectedSpec == DeathKnightSpec::UNHOLY) &&
        !HasAura(DK_BONE_SHIELD) && CanUseAbility(DK_BONE_SHIELD))
    {
        CastSpell(DK_BONE_SHIELD);
    }
}

void DeathKnightAI::UpdateCooldowns(uint32 diff)
{
    if (!GetBot())
        return;

    // Update rune manager
    _runeManager->UpdateRunes(diff);

    // Update cooldown manager
    if (_cooldownManager)
        _cooldownManager->Update(diff);
}

bool DeathKnightAI::CanUseAbility(uint32 spellId)
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

    return true;
}

bool DeathKnightAI::HasEnoughResource(uint32 spellId)
{
    if (!GetBot())
        return false;

    const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(spellId, GetBot()->GetMap()->GetDifficultyID());
    if (!spellInfo)
        return false;

    // Check runic power cost
    auto powerCosts = spellInfo->CalcPowerCost(GetBot(), spellInfo->GetSchoolMask());
    for (auto const& cost : powerCosts)
    {
        if (cost.Power == POWER_RUNIC_POWER)
        {

            uint32 currentPower = GetBot()->GetPower(POWER_RUNIC_POWER);

            if (currentPower < cost.Amount)

                return false;
        }
    }

    // Check rune requirements
    switch (spellId)
    {
        case DK_HOWLING_BLAST:

            return _runeManager->HasRunes(0u, 1u, 0u);
        case DK_OUTBREAK:

            return _runeManager->HasRunes(0u, 0u, 1u);
        case DK_HEART_STRIKE:
        case DK_BLOOD_BOIL:
            return _runeManager->HasRunes(1u, 0u, 0u);
        case DK_DEATH_STRIKE:
        case DK_OBLITERATE:

            return _runeManager->HasRunes(0u, 1u, 1u);
        case DK_SCOURGE_STRIKE:

            return _runeManager->HasRunes(0u, 0u, 1u);
        default:

            break;
    }

    return true;
}

void DeathKnightAI::ConsumeResource(uint32 spellId)
{
    if (!GetBot())
        return;

    const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(spellId, GetBot()->GetMap()->GetDifficultyID());
    if (!spellInfo)
        return;

    // Track runic power consumption
    auto powerCosts = spellInfo->CalcPowerCost(GetBot(), spellInfo->GetSchoolMask());
    for (auto const& cost : powerCosts)
    {
        if (cost.Power == POWER_RUNIC_POWER)
        {
            _metrics->totalRunicPowerSpent += cost.Amount;

            _runicPowerSpent += cost.Amount;
        }
    }

    // Track rune usage
    uint8 runesUsed = 0;
    switch (spellId)
    {
        case DK_HOWLING_BLAST:
        case DK_OUTBREAK:
        case DK_HEART_STRIKE:
        case DK_BLOOD_BOIL:
        case DK_SCOURGE_STRIKE:
            runesUsed = 1;

            break;
        case DK_DEATH_STRIKE:
        case DK_OBLITERATE:

            runesUsed = 2;

            break;
        default:

            break;
    }

    if (runesUsed > 0)
    {
        _metrics->totalRunesUsed += runesUsed;
        _runesUsed += runesUsed;
    }
}

void DeathKnightAI::OnCombatStart(Unit* target)
{
    if (!target || !GetBot())
        return;

    ClassAI::OnCombatStart(target);

    // Reset combat metrics
    _runicPowerSpent = 0;
    _runesUsed = 0;
    _diseasesApplied = 0;

    // Apply diseases immediately
    if (CanUseAbility(DK_HOWLING_BLAST))
    {
        CastSpell(DK_HOWLING_BLAST, target);
        _diseaseManager->UpdateDiseases(target);
    }

    // Use offensive cooldowns for boss fights
    if (target->GetTypeId() == TYPEID_UNIT && target->ToCreature()->isWorldBoss())
    {
        ActivateBurstCooldowns(target);

        // Army of the Dead for major encounters
        if (CanUseAbility(DK_ARMY_OF_THE_DEAD))
        {
            CastSpell(DK_ARMY_OF_THE_DEAD);

            TC_LOG_DEBUG("playerbot", "DeathKnightAI: Summoned Army of the Dead for boss");
        }
    }

    // Use defensive cooldowns if taking magic damage
    if (target->GetTypeId() == TYPEID_UNIT)
    {
        Creature* creature = target->ToCreature();
        if (!creature)
        {
            return;
        }

        if (creature && creature->GetCreatureTemplate()->unit_class == UNIT_CLASS_MAGE)
        {

            if (CanUseAbility(DK_ANTI_MAGIC_SHELL))

            {

                CastSpell(DK_ANTI_MAGIC_SHELL);

                TC_LOG_DEBUG("playerbot", "DeathKnightAI: Activated Anti-Magic Shell");

            }
        }
    }

    TC_LOG_DEBUG("playerbot", "DeathKnightAI: Combat started against {} with spec {}",

                 target->GetName(), static_cast<uint32>(_detectedSpec));
}

void DeathKnightAI::OnCombatEnd()
{
    ClassAI::OnCombatEnd();

    // Calculate combat metrics
    if (_runesUsed > 0)
    {
        _metrics->UpdateRuneEfficiency(_runesUsed, _metrics->totalRunicPowerGenerated);
    }

    // Reset rune system
    _runeManager->ResetRunes();

    TC_LOG_DEBUG("playerbot", "DeathKnightAI: Combat ended. RP spent: {}, Runes used: {}, Diseases: {}",

                 _runicPowerSpent, _runesUsed, _diseasesApplied);
}

void DeathKnightAI::ActivateBurstCooldowns(Unit* target)
{
    if (!target)
        return;

    switch (_detectedSpec)
    {
        case DeathKnightSpec::BLOOD:

            if (CanUseAbility(DK_VAMPIRIC_BLOOD))

            {

                CastSpell(DK_VAMPIRIC_BLOOD, GetBot());

                _metrics->cooldownsUsed++;

            }

            if (CanUseAbility(DK_DANCING_RUNE_WEAPON))

            {

                CastSpell(DK_DANCING_RUNE_WEAPON);

                _metrics->cooldownsUsed++;

            }

            break;

        case DeathKnightSpec::FROST:

            if (CanUseAbility(DK_PILLAR_OF_FROST))

            {

                CastSpell(DK_PILLAR_OF_FROST);

                _metrics->cooldownsUsed++;

            }

            if (CanUseAbility(DK_REMORSELESS_WINTER))
            {
                CastSpell(DK_REMORSELESS_WINTER);
                _metrics->cooldownsUsed++;
            }

            if (CanUseAbility(DK_EMPOWER_RUNE_WEAPON))

            {

                CastSpell(DK_EMPOWER_RUNE_WEAPON);

                _metrics->cooldownsUsed++;

            }

            break;

        case DeathKnightSpec::UNHOLY:

            if (CanUseAbility(DK_SUMMON_GARGOYLE))

            {

                CastSpell(DK_SUMMON_GARGOYLE);

                _metrics->cooldownsUsed++;

            }

            if (CanUseAbility(DK_UNHOLY_ASSAULT))

            {

                CastSpell(DK_UNHOLY_ASSAULT, target);

                _metrics->cooldownsUsed++;

            }

            break;
    }
}

Position DeathKnightAI::GetOptimalPosition(Unit* target)
{
    if (!target || !GetBot())
        return GetBot()->GetPosition();

    // Use positioning manager for advanced calculations
    if (_positioning)
    {
        return _positioning->CalculateOptimalPosition(target, _detectedSpec);
    }

    return ClassAI::GetOptimalPosition(target);
}

float DeathKnightAI::GetOptimalRange(Unit* target)
{
    if (!target)
        return 5.0f;

    // Use positioning manager
    if (_positioning)
    {
        return _positioning->GetOptimalRange(_detectedSpec);
    }

    return 5.0f; // Default melee range
}

DeathKnightSpec DeathKnightAI::GetCurrentSpecialization() const
{
    return _detectedSpec;
}

// ========================================================================
// CombatBehaviorIntegration Helper Methods Implementation
// ========================================================================

bool DeathKnightAI::HandleInterrupts(Unit* target)
{
    auto* behaviors = GetCombatBehaviors();
    if (!behaviors)
        return false;

    Unit* interruptTarget = behaviors->GetInterruptTarget();
    if (!interruptTarget)
        interruptTarget = target;

    if (!interruptTarget || !interruptTarget->HasUnitState(UNIT_STATE_CASTING))
        return false;

    float distance = ::std::sqrt(GetBot()->GetExactDistSq(interruptTarget)); // Calculate once from squared distance

    // Priority: Mind Freeze for melee range, Strangulate for ranged
    if (distance <= OPTIMAL_MELEE_RANGE && CanUseAbility(DK_MIND_FREEZE))
    {
        if (CastSpell(DK_MIND_FREEZE, interruptTarget))
        {

            RecordInterruptAttempt(interruptTarget, DK_MIND_FREEZE, true);

            TC_LOG_DEBUG("module.playerbot.ai", "Death Knight {} interrupted {} with Mind Freeze",

                         GetBot()->GetName(), interruptTarget->GetName());

            return true;
        }
    }
    else if (distance <= DEATH_GRIP_MAX_RANGE && CanUseAbility(DK_ASPHYXIATE))
    {
        if (CastSpell(DK_ASPHYXIATE, interruptTarget))
        {

            RecordInterruptAttempt(interruptTarget, DK_ASPHYXIATE, true);

            TC_LOG_DEBUG("module.playerbot.ai", "Death Knight {} interrupted {} with Strangulate",

                         GetBot()->GetName(), interruptTarget->GetName());

            return true;
        }
    }

    return false;
}

bool DeathKnightAI::HandleDefensives()
{
    if (!GetBot())
        return false;

    float healthPct = GetBot()->GetHealthPct();
    bool actionTaken = false;

    // Icebound Fortitude at critical health
    if (healthPct < HEALTH_EMERGENCY_THRESHOLD && CanUseAbility(DK_ICEBOUND_FORTITUDE))
    {
        if (CastSpell(DK_ICEBOUND_FORTITUDE))
        {

            RecordAbilityUsage(DK_ICEBOUND_FORTITUDE);

            TC_LOG_DEBUG("module.playerbot.ai", "Death Knight {} activated Icebound Fortitude",

                         GetBot()->GetName());

            actionTaken = true;
        }
    }

    // Anti-Magic Shell against casters
    Unit* currentTarget = GetBot()->GetSelectedUnit();
    if (currentTarget && currentTarget->HasUnitState(UNIT_STATE_CASTING))
    {
        if (CanUseAbility(DK_ANTI_MAGIC_SHELL))
        {

            if (CastSpell(DK_ANTI_MAGIC_SHELL))

            {

                RecordAbilityUsage(DK_ANTI_MAGIC_SHELL);

                TC_LOG_DEBUG("module.playerbot.ai", "Death Knight {} activated Anti-Magic Shell",

                             GetBot()->GetName());

                actionTaken = true;

            }
        }
    }

    // Spec-specific defensives
    switch (_detectedSpec)
    {
        case DeathKnightSpec::BLOOD:
            // Vampiric Blood for blood DKs
    if (healthPct < DEFENSIVE_COOLDOWN_THRESHOLD && CanUseAbility(DK_VAMPIRIC_BLOOD))

            {

                if (CastSpell(DK_VAMPIRIC_BLOOD))

                {

                    RecordAbilityUsage(DK_VAMPIRIC_BLOOD);

                    TC_LOG_DEBUG("module.playerbot.ai", "Blood Death Knight {} activated Vampiric Blood",

                                 GetBot()->GetName());

                    actionTaken = true;

                }

            }
            // Rune Tap for quick heal
    if (healthPct < 60.0f && CanUseAbility(DK_RUNE_TAP))

            {

                if (CastSpell(DK_RUNE_TAP))

                {

                    RecordAbilityUsage(DK_RUNE_TAP);

                    actionTaken = true;

                }

            }

            break;

        case DeathKnightSpec::FROST:
            // Unbreakable Armor for frost DKs
    if (healthPct < 50.0f && CanUseAbility(DK_PILLAR_OF_FROST))

            {

                if (CastSpell(DK_PILLAR_OF_FROST))

                {

                    RecordAbilityUsage(DK_PILLAR_OF_FROST);

                    TC_LOG_DEBUG("module.playerbot.ai", "Frost Death Knight {} activated Unbreakable Armor",

                                 GetBot()->GetName());

                    actionTaken = true;

                }

            }

            break;

        case DeathKnightSpec::UNHOLY:
            // Bone Shield maintenance
    if (!HasAura(DK_BONE_SHIELD) && CanUseAbility(DK_BONE_SHIELD))

            {

                if (CastSpell(DK_BONE_SHIELD))

                {

                    RecordAbilityUsage(DK_BONE_SHIELD);

                    actionTaken = true;

                }

            }

            break;
    }

    // Death Strike for self-healing when low
    if (healthPct < 70.0f && _runeManager->HasRunes(0u, 1u, 1u) && CanUseAbility(DK_DEATH_STRIKE))
    {
        Unit* target = GetBot()->GetSelectedUnit();
        if (target && IsInMeleeRange(target))
        {

            if (CastSpell(DK_DEATH_STRIKE, target))

            {

                _runeManager->ConsumeRunes(0, 1, 1);

                RecordAbilityUsage(DK_DEATH_STRIKE);

                _metrics->deathStrikesUsed++;

                actionTaken = true;

            }
        }
    }

    return actionTaken;
}

bool DeathKnightAI::HandleTargetSwitching(Unit*& target)
{
    auto* behaviors = GetCombatBehaviors();
    if (!behaviors)
        return false;

    Unit* priorityTarget = behaviors->GetPriorityTarget();
    if (!priorityTarget || priorityTarget == target)
        return false;

    OnTargetChanged(priorityTarget);
    target = priorityTarget;

    if (!priorityTarget)
    {
        return false;
    }

    TC_LOG_DEBUG("module.playerbot.ai", "Death Knight {} switching target to {}",

                 GetBot()->GetName(), priorityTarget->GetName());

    // Use Death Grip on new target if needed
    float distance = ::std::sqrt(GetBot()->GetExactDistSq(priorityTarget)); // Calculate once from squared distance
    if (distance > DEATH_GRIP_MIN_RANGE && distance <= DEATH_GRIP_MAX_RANGE)
    {
        if (ShouldUseDeathGrip(priorityTarget) && CanUseAbility(DK_DEATH_GRIP))
        {

            if (CastSpell(DK_DEATH_GRIP, priorityTarget))

            {

                _lastDeathGrip = GameTime::GetGameTimeMS();

                RecordAbilityUsage(DK_DEATH_GRIP);

                _metrics->deathGripsUsed++;

                return true;

            }
        }
    }

    // Use Dark Command (taunt) if we're a tank
    if (_detectedSpec == DeathKnightSpec::BLOOD && ShouldUseDarkCommand(priorityTarget))
    {
        if (CanUseAbility(DK_DARK_COMMAND))
        {

            if (CastSpell(DK_DARK_COMMAND, priorityTarget))

            {

                _lastDarkCommand = GameTime::GetGameTimeMS();

                RecordAbilityUsage(DK_DARK_COMMAND);

                return true;

            }
        }
    }

    return false;
}

bool DeathKnightAI::HandleAoERotation(Unit* target)
{
    if (!target || !GetBot())
        return false;

    uint32 enemyCount = GetNearbyEnemyCount(10.0f);
    if (enemyCount < 3)
        return false;

    // Death and Decay for AoE damage
    if (CanUseAbility(DK_DEATH_AND_DECAY) && _runeManager->HasRunes(1u, 1u, 1u))
    {
        if (CastSpell(DK_DEATH_AND_DECAY))
        {

            _runeManager->ConsumeRunes(1, 1, 1);

            RecordAbilityUsage(DK_DEATH_AND_DECAY);

            TC_LOG_DEBUG("module.playerbot.ai", "Death Knight {} using Death and Decay for AoE",

                         GetBot()->GetName());

            return true;
        }
    }

    // Blood Boil to spread diseases
    if (_diseaseManager->HasBothDiseases(target) && CanUseAbility(DK_BLOOD_BOIL) && _runeManager->HasRunes(1u, 0u, 0u))
    {
        if (CastSpell(DK_BLOOD_BOIL))
        {

            _runeManager->ConsumeRunes(1, 0, 0);

            RecordAbilityUsage(DK_BLOOD_BOIL);
            TC_LOG_DEBUG("module.playerbot.ai", "Death Knight {} using Blood Boil to spread diseases",

                         GetBot()->GetName());

            return true;
        }
    }

    // Epidemic for Unholy (if available)
    if (_detectedSpec == DeathKnightSpec::UNHOLY && CanUseAbility(DK_EPIDEMIC))
    {
        if (CastSpell(DK_EPIDEMIC))
        {

            RecordAbilityUsage(DK_EPIDEMIC);

            TC_LOG_DEBUG("module.playerbot.ai", "Unholy Death Knight {} using Epidemic for AoE",

                         GetBot()->GetName());

            return true;
        }
    }

    // Howling Blast for Frost
    if (_detectedSpec == DeathKnightSpec::FROST && CanUseAbility(DK_HOWLING_BLAST) && _runeManager->HasRunes(0u, 1u, 0u))
    {
        if (CastSpell(DK_HOWLING_BLAST, target))
        {

            _runeManager->ConsumeRunes(0, 1, 0);

            RecordAbilityUsage(DK_HOWLING_BLAST);

            TC_LOG_DEBUG("module.playerbot.ai", "Frost Death Knight {} using Howling Blast for AoE",

                         GetBot()->GetName());

            return true;
        }
    }

    return false;
}

bool DeathKnightAI::HandleOffensiveCooldowns(Unit* target)
{
    if (!target || !GetBot())
        return false;

    bool actionTaken = false;

    switch (_detectedSpec)
    {
        case DeathKnightSpec::BLOOD:
            // Dancing Rune Weapon for Blood
    if (CanUseAbility(DK_DANCING_RUNE_WEAPON))

            {

                if (CastSpell(DK_DANCING_RUNE_WEAPON))

                {

                    RecordAbilityUsage(DK_DANCING_RUNE_WEAPON);

                    _metrics->cooldownsUsed++;

                    TC_LOG_DEBUG("module.playerbot.ai", "Blood Death Knight {} activated Dancing Rune Weapon",

                                 GetBot()->GetName());

                    actionTaken = true;

                }

            }

            break;

        case DeathKnightSpec::FROST:
            // Pillar of Frost for massive damage boost
    if (CanUseAbility(DK_PILLAR_OF_FROST))

            {

                if (CastSpell(DK_PILLAR_OF_FROST))

                {

                    RecordAbilityUsage(DK_PILLAR_OF_FROST);

                    _metrics->cooldownsUsed++;

                    TC_LOG_DEBUG("module.playerbot.ai", "Frost Death Knight {} activated Pillar of Frost",

                                 GetBot()->GetName());

                    actionTaken = true;

                }

            }
            // Empower Rune Weapon for rune regeneration
    if (CanUseAbility(DK_EMPOWER_RUNE_WEAPON))

            {

                if (CastSpell(DK_EMPOWER_RUNE_WEAPON))

                {

                    RecordAbilityUsage(DK_EMPOWER_RUNE_WEAPON);

                    _metrics->cooldownsUsed++;

                    actionTaken = true;

                }

            }

            break;

        case DeathKnightSpec::UNHOLY:
            // Summon Gargoyle for burst damage
    if (CanUseAbility(DK_SUMMON_GARGOYLE))

            {

                if (CastSpell(DK_SUMMON_GARGOYLE))

                {

                    RecordAbilityUsage(DK_SUMMON_GARGOYLE);

                    _metrics->cooldownsUsed++;

                    TC_LOG_DEBUG("module.playerbot.ai", "Unholy Death Knight {} summoned Gargoyle",

                                 GetBot()->GetName());

                    actionTaken = true;

                }

            }
            // Unholy Frenzy for attack speed
    if (CanUseAbility(DK_UNHOLY_ASSAULT))

            {

                if (CastSpell(DK_UNHOLY_ASSAULT, target))

                {

                    RecordAbilityUsage(DK_UNHOLY_ASSAULT);

                    _metrics->cooldownsUsed++;

                    actionTaken = true;

                }

            }

            break;
    }

    // Army of the Dead for major fights (all specs)
    if (target->GetTypeId() == TYPEID_UNIT && target->ToCreature()->isWorldBoss())
    {
        if (CanUseAbility(DK_ARMY_OF_THE_DEAD))
        {

            if (CastSpell(DK_ARMY_OF_THE_DEAD))

            {

                RecordAbilityUsage(DK_ARMY_OF_THE_DEAD);

                _metrics->cooldownsUsed++;

                TC_LOG_DEBUG("module.playerbot.ai", "Death Knight {} summoned Army of the Dead",

                             GetBot()->GetName());

                actionTaken = true;

            }
        }
    }

    return actionTaken;
}

bool DeathKnightAI::HandleRuneAndPowerManagement(Unit* target)
{
    if (!target || !GetBot())
        return false;

    uint32 runicPower = GetBot()->GetPower(POWER_RUNIC_POWER);

    // Runic Power dump when capped
    if (runicPower >= RUNIC_POWER_DUMP_THRESHOLD)
    {
        switch (_detectedSpec)
        {

            case DeathKnightSpec::FROST:

                if (CanUseAbility(DK_FROST_STRIKE))

                {

                    if (CastSpell(DK_FROST_STRIKE, target))

                    {

                        _metrics->totalRunicPowerSpent += 40;

                        _runicPowerSpent += 40;

                        RecordAbilityUsage(DK_FROST_STRIKE);

                        return true;

                    }

                }

                break;


            case DeathKnightSpec::BLOOD:

                if (CanUseAbility(DK_FROST_STRIKE))

                {

                    if (CastSpell(DK_FROST_STRIKE, target))

                    {

                        _metrics->totalRunicPowerSpent += 20;

                        _runicPowerSpent += 20;

                        RecordAbilityUsage(DK_FROST_STRIKE);

                        return true;

                    }

                }
                break;
                case DeathKnightSpec::UNHOLY:
                if (CanUseAbility(DK_DEATH_COIL))

                {

                    if (CastSpell(DK_DEATH_COIL, target))

                    {

                        _metrics->totalRunicPowerSpent += 40;

                        _runicPowerSpent += 40;

                        RecordAbilityUsage(DK_DEATH_COIL);

                        return true;

                    }

                }

                break;
        }
    }

    // Empower Rune Weapon when out of runes
    if (_runeManager->GetTotalAvailableRunes() == 0 && CanUseAbility(DK_EMPOWER_RUNE_WEAPON))
    {
        if (CastSpell(DK_EMPOWER_RUNE_WEAPON))
        {

            RecordAbilityUsage(DK_EMPOWER_RUNE_WEAPON);

            TC_LOG_DEBUG("module.playerbot.ai", "Death Knight {} used Empower Rune Weapon for rune regeneration",

                         GetBot()->GetName());

            return true;
        }
    }

    return false;
}

void DeathKnightAI::ExecuteSpecializationRotation(Unit* target)
{
    if (!target || !GetBot())
        return;

    // Update presence if needed
    UpdatePresenceIfNeeded();

    // Check if we're on global cooldown
    if (_combatMetrics->IsOnGlobalCooldown())
        return;

    // Execute fallback rotation
    ExecuteFallbackRotation(target);
}

void DeathKnightAI::UpdatePresenceIfNeeded()
{
    // NOTE: Presences removed in WoW 12.0 - specializations are automatic
    // This function is kept for API compatibility but does nothing
}

void DeathKnightAI::UseDefensiveCooldowns()
{
    // Legacy method - now handled by HandleDefensives()
    HandleDefensives();
}

void DeathKnightAI::UseAntiMagicDefenses(Unit* target)
{
    if (!target || !GetBot())
        return;

    // Anti-Magic Shell for personal protection
    if (CanUseAbility(DK_ANTI_MAGIC_SHELL))
    {
        CastSpell(DK_ANTI_MAGIC_SHELL);
        RecordAbilityUsage(DK_ANTI_MAGIC_SHELL);
    }

    // Anti-Magic Zone for group protection
    if (GetBot()->GetGroup() && CanUseAbility(DK_ANTI_MAGIC_ZONE))
    {
        CastSpell(DK_ANTI_MAGIC_ZONE);
        RecordAbilityUsage(DK_ANTI_MAGIC_ZONE);
    }
}

bool DeathKnightAI::ShouldUseDeathGrip(Unit* target) const
{
    if (!target || !GetBot())
        return false;

    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - _lastDeathGrip < DEATH_GRIP_COOLDOWN)
        return false;

    float distance = ::std::sqrt(GetBot()->GetExactDistSq(target)); // Calculate once from squared distance
    if (distance < DEATH_GRIP_MIN_RANGE || distance > DEATH_GRIP_MAX_RANGE)
        return false;

    // Check if target is a caster that should be pulled
    if (target->GetTypeId() == TYPEID_UNIT)
    {
        Creature* creature = target->ToCreature();
        if (!creature)
        {
            return false;
        }

        if (creature && creature->GetCreatureTemplate()->unit_class == UNIT_CLASS_MAGE)
            return true;
    }

    // Pull ranged attackers
    if (distance > 15.0f)
        return true;

    return false;
}

bool DeathKnightAI::ShouldUseDarkCommand(Unit* target) const
{
    if (!target || !GetBot())
        return false;

    // Only for tank spec
    if (_detectedSpec != DeathKnightSpec::BLOOD)
        return false;

    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - _lastDarkCommand < DARK_COMMAND_COOLDOWN)
        return false;

    // Check if we need to taunt
    Unit* currentVictim = target->GetVictim();
    if (!currentVictim || currentVictim == GetBot())
        return false;

    // Taunt if attacking a healer or squishy
    if (currentVictim->GetTypeId() == TYPEID_PLAYER)
    {
        Player* player = currentVictim->ToPlayer();
        if (!player)
        {
            return false;
        }

        if (player->GetClass() == CLASS_PRIEST || player->GetClass() == CLASS_MAGE)
            return true;
    }

    return false;
}

uint32 DeathKnightAI::GetNearbyEnemyCount(float range) const
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
    for (auto& unit : targets)
    {
        if (GetBot()->IsValidAttackTarget(unit))

            count++;
    }

    return count;
}

bool DeathKnightAI::IsInMeleeRange(Unit* target) const
{
    if (!target || !GetBot())
        return false;

    float rangeSq = OPTIMAL_MELEE_RANGE * OPTIMAL_MELEE_RANGE; // 25.0f
    return GetBot()->GetExactDistSq(target) <= rangeSq;
}

bool DeathKnightAI::HasRunesForSpell(uint32 spellId) const
{
    switch (spellId)
    {
        case DK_HOWLING_BLAST:

            return _runeManager->HasRunes(0u, 1u, 0u);
        case DK_OUTBREAK:

            return _runeManager->HasRunes(0u, 0u, 1u);
        case DK_HEART_STRIKE:
        case DK_BLOOD_BOIL:
            return _runeManager->HasRunes(1u, 0u, 0u);
        case DK_DEATH_STRIKE:
        case DK_OBLITERATE:

            return _runeManager->HasRunes(0u, 1u, 1u);
        case DK_SCOURGE_STRIKE:

            return _runeManager->HasRunes(0u, 0u, 1u);
        case DK_DEATH_AND_DECAY:

            return _runeManager->HasRunes(1u, 1u, 1u);
        default:

            return true;
    }
}

uint32 DeathKnightAI::GetRunicPowerCost(uint32 spellId) const
{
    const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(spellId, GetBot()->GetMap()->GetDifficultyID());
    if (!spellInfo)
        return 0;

    auto powerCosts = spellInfo->CalcPowerCost(GetBot(), spellInfo->GetSchoolMask());
    for (auto const& cost : powerCosts)
    {
        if (cost.Power == POWER_RUNIC_POWER)

            return cost.Amount;
    }

    return 0;
}

void DeathKnightAI::RecordInterruptAttempt(Unit* target, uint32 spellId, bool success)
{
    if (success)
    {
        _successfulInterrupts++;
        _metrics->interruptsExecuted++;
        TC_LOG_DEBUG("module.playerbot.ai", "Death Knight {} successfully interrupted with spell {}",

                     GetBot()->GetName(), spellId);
    }
}

void DeathKnightAI::RecordAbilityUsage(uint32 spellId)
{
    _abilityUsage[spellId]++;
    _combatMetrics->RecordAbilityUsage(spellId, true);
}

void DeathKnightAI::OnTargetChanged(Unit* newTarget)
{
    if (!newTarget)
        return;

    TC_LOG_DEBUG("module.playerbot.ai", "Death Knight {} changed target to {}",

                 GetBot()->GetName(), newTarget->GetName());

    // Reset disease tracking for new target
    _diseaseManager->UpdateDiseases(newTarget);
}

DeathKnightAI::~DeathKnightAI()
{
    // Cleanup is now fully handled by smart pointers (std::unique_ptr)
    // No manual delete calls needed - automatic memory management
}

} // namespace Playerbot
