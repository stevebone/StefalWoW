/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "DefensiveCoordinator.h"
#include "ArenaCoordinator.h"
#include "AI/Coordination/Messaging/BotMessageBus.h"
#include "AI/Coordination/Messaging/BotMessage.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "Group.h"
#include "GameTime.h"
#include "Log.h"
#include <algorithm>

namespace Playerbot {

// ============================================================================
// DEFENSIVE SPELL DATABASE
// ============================================================================

struct DefensiveSpellInfo
{
    uint32 spellId;
    DefensiveType type;
    uint32 cooldown;
    uint32 effectDuration;
    float damageReduction;
    bool isExternal;
    bool breaksCC;
};

static const ::std::vector<DefensiveSpellInfo> DEFENSIVE_SPELLS = {
    // Personal Immunities
    { 45438, DefensiveType::PERSONAL_IMMUNITY, 240000, 10000, 1.0f, false, true },    // Ice Block
    { 642, DefensiveType::PERSONAL_IMMUNITY, 300000, 8000, 1.0f, false, true },       // Divine Shield
    { 186265, DefensiveType::PERSONAL_IMMUNITY, 180000, 8000, 1.0f, false, false },   // Aspect of the Turtle

    // Personal Walls
    { 48792, DefensiveType::PERSONAL_WALL, 180000, 8000, 0.3f, false, true },         // Icebound Fortitude
    { 22812, DefensiveType::PERSONAL_WALL, 60000, 12000, 0.2f, false, false },        // Barkskin
    { 61336, DefensiveType::PERSONAL_WALL, 180000, 6000, 0.5f, false, false },        // Survival Instincts
    { 118038, DefensiveType::PERSONAL_WALL, 120000, 8000, 0.3f, false, false },       // Die by the Sword
    { 184364, DefensiveType::PERSONAL_WALL, 120000, 8000, 0.2f, false, false },       // Enraged Regeneration
    { 31224, DefensiveType::PERSONAL_WALL, 120000, 5000, 0.0f, false, true },         // Cloak of Shadows (magic immune)
    { 5277, DefensiveType::PERSONAL_WALL, 120000, 10000, 0.0f, false, false },        // Evasion
    { 104773, DefensiveType::PERSONAL_WALL, 180000, 8000, 0.4f, false, false },       // Unending Resolve
    { 108271, DefensiveType::PERSONAL_WALL, 90000, 8000, 0.4f, false, false },        // Astral Shift
    { 198589, DefensiveType::PERSONAL_WALL, 60000, 10000, 0.35f, false, false },      // Blur

    // Personal Absorbs
    { 17, DefensiveType::PERSONAL_ABSORB, 7500, 15000, 0.0f, false, false },          // Power Word: Shield
    { 11426, DefensiveType::PERSONAL_ABSORB, 25000, 60000, 0.0f, false, false },      // Ice Barrier

    // External Shields
    { 33206, DefensiveType::EXTERNAL_SHIELD, 180000, 8000, 0.4f, true, false },       // Pain Suppression
    { 102342, DefensiveType::EXTERNAL_SHIELD, 90000, 12000, 0.2f, true, false },      // Ironbark
    { 6940, DefensiveType::EXTERNAL_SHIELD, 120000, 12000, 0.3f, true, false },       // Blessing of Sacrifice
    { 116849, DefensiveType::EXTERNAL_SHIELD, 120000, 15000, 0.3f, true, false },     // Life Cocoon

    // External Immunity
    { 1022, DefensiveType::EXTERNAL_IMMUNITY, 300000, 10000, 1.0f, true, true },      // Blessing of Protection

    // CC Breaks
    { 336126, DefensiveType::CC_BREAK, 120000, 0, 0.0f, false, true },                // Gladiator's Medallion
    { 59752, DefensiveType::CC_BREAK, 90000, 0, 0.0f, false, true },                  // Every Man for Himself
    { 7744, DefensiveType::CC_BREAK, 90000, 0, 0.0f, false, true }                    // Will of the Forsaken
};

// ============================================================================
// CONSTRUCTOR
// ============================================================================

DefensiveCoordinator::DefensiveCoordinator(ArenaCoordinator* coordinator)
    : _coordinator(coordinator)
{
}

void DefensiveCoordinator::Initialize()
{
    Reset();

    // Initialize defensive cooldowns for all teammates
    for (const auto& teammate : _coordinator->GetTeammates())
    {
        LoadPlayerDefensives(teammate.guid);
    }

    TC_LOG_DEBUG("playerbot", "DefensiveCoordinator::Initialize - Initialized");
}

void DefensiveCoordinator::Update(uint32 diff)
{
    // Update cooldown timers
    UpdateCooldownTimers(diff);

    // Update active defensives
    UpdateActiveDefensives(diff);

    // Update peel assignments
    UpdatePeelAssignments(diff);

    // Process pending peel requests
    ProcessPeelRequests();

    // Clean old damage records
    CleanOldDamageRecords(GameTime::GetGameTimeMS());
}

void DefensiveCoordinator::Reset()
{
    _pendingPeels.clear();
    _activePeels.clear();
    _playerDefensives.clear();
    _recentDamage.clear();
}

// ============================================================================
// THREAT ASSESSMENT
// ============================================================================

ThreatAssessment DefensiveCoordinator::AssessTeammate(ObjectGuid teammate) const
{
    ThreatAssessment assessment;
    assessment.target = teammate;

    const ArenaTeammate* tm = _coordinator->GetTeammate(teammate);
    if (!tm)
        return assessment;

    assessment.healthPercent = tm->healthPercent;
    assessment.healthDeficit = 100.0f - tm->healthPercent;
    assessment.incomingDamageRate = GetDamageRate(teammate);
    assessment.isBeingFocused = IsBeingFocused(teammate);
    assessment.hasActiveCCs = tm->isCC;
    assessment.hasDefensivesUp = HasDefensivesAvailable(teammate);
    assessment.state = CalculateDefensiveState(teammate);
    assessment.urgencyLevel = CalculateUrgency(assessment);

    // Count attackers
    for (const auto& enemy : _coordinator->GetAliveEnemies())
    {
        // Simplified - would need actual targeting check
        if (enemy.currentPriority == TargetPriority::KILL_TARGET)
        {
            assessment.attackerCount++;
        }
    }

    return assessment;
}

DefensiveState DefensiveCoordinator::GetTeammateState(ObjectGuid teammate) const
{
    return CalculateDefensiveState(teammate);
}

ObjectGuid DefensiveCoordinator::GetMostEndangeredTeammate() const
{
    ObjectGuid mostEndangered;
    uint8 highestUrgency = 0;

    for (const auto& teammate : _coordinator->GetAliveTeammates())
    {
        ThreatAssessment assessment = AssessTeammate(teammate.guid);
        if (assessment.urgencyLevel > highestUrgency)
        {
            highestUrgency = assessment.urgencyLevel;
            mostEndangered = teammate.guid;
        }
    }

    return mostEndangered;
}

float DefensiveCoordinator::GetTeamThreatLevel() const
{
    float totalThreat = 0.0f;

    for (const auto& teammate : _coordinator->GetAliveTeammates())
    {
        ThreatAssessment assessment = AssessTeammate(teammate.guid);
        totalThreat += assessment.urgencyLevel;
    }

    return totalThreat;
}

::std::vector<ThreatAssessment> DefensiveCoordinator::GetAllAssessments() const
{
    ::std::vector<ThreatAssessment> assessments;

    for (const auto& teammate : _coordinator->GetAliveTeammates())
    {
        assessments.push_back(AssessTeammate(teammate.guid));
    }

    // Sort by urgency (highest first)
    ::std::sort(assessments.begin(), assessments.end(),
        [](const ThreatAssessment& a, const ThreatAssessment& b)
        {
            return a.urgencyLevel > b.urgencyLevel;
        });

    return assessments;
}

// ============================================================================
// PEEL MANAGEMENT
// ============================================================================

void DefensiveCoordinator::RequestPeel(ObjectGuid teammate, ObjectGuid threat, uint8 urgency)
{
    PeelRequest request;
    request.teammate = teammate;
    request.threat = threat;
    request.requestTime = GameTime::GetGameTimeMS();
    request.urgency = urgency;
    request.isFilled = false;

    _pendingPeels.push_back(request);

    TC_LOG_DEBUG("playerbot", "DefensiveCoordinator::RequestPeel - Peel requested for teammate, urgency %u", urgency);
}

void DefensiveCoordinator::AssignPeel(ObjectGuid peeler, ObjectGuid teammate, ObjectGuid threat)
{
    PeelAssignment assignment;
    assignment.peeler = peeler;
    assignment.target = teammate;
    assignment.threat = threat;
    assignment.assignTime = GameTime::GetGameTimeMS();
    assignment.duration = _peelDuration;
    assignment.isActive = true;

    _activePeels[peeler] = assignment;

    TC_LOG_DEBUG("playerbot", "DefensiveCoordinator::AssignPeel - Peel assigned");
}

void DefensiveCoordinator::CancelPeel(ObjectGuid peeler)
{
    _activePeels.erase(peeler);
}

const PeelAssignment* DefensiveCoordinator::GetPeelAssignment(ObjectGuid peeler) const
{
    auto it = _activePeels.find(peeler);
    return it != _activePeels.end() ? &it->second : nullptr;
}

ObjectGuid DefensiveCoordinator::GetPeelTarget() const
{
    ObjectGuid target = GetMostEndangeredTeammate();

    // Only return target if they need peel
    ThreatAssessment assessment = AssessTeammate(target);
    if (assessment.urgencyLevel >= 2)
        return target;

    return ObjectGuid::Empty;
}

ObjectGuid DefensiveCoordinator::GetBestPeeler(ObjectGuid target, ObjectGuid threat) const
{
    ObjectGuid bestPeeler;
    float bestScore = 0.0f;

    for (const auto& teammate : _coordinator->GetAliveTeammates())
    {
        // Skip the target themselves
        if (teammate.guid == target)
            continue;

        // Skip healers (they should heal, not peel)
        if (teammate.role == ArenaRole::HEALER)
            continue;

        // Check if already peeling
        if (IsPeeling(teammate.guid))
            continue;

        // Score this potential peeler
        float score = ScorePeeler(teammate.guid, target, threat);
        if (score > bestScore)
        {
            bestScore = score;
            bestPeeler = teammate.guid;
        }
    }

    return bestPeeler;
}

bool DefensiveCoordinator::IsPeeling(ObjectGuid player) const
{
    return _activePeels.find(player) != _activePeels.end();
}

// ============================================================================
// DEFENSIVE COOLDOWN MANAGEMENT
// ============================================================================

bool DefensiveCoordinator::ShouldUseDefensive(ObjectGuid player) const
{
    ThreatAssessment assessment = AssessTeammate(player);

    // Use defensive if in danger or critical
    if (assessment.state >= DefensiveState::IN_DANGER)
        return true;

    // Use defensive if about to die
    float dps = assessment.incomingDamageRate;
    float timeToLive = assessment.healthPercent / (dps / 1000.0f);  // seconds

    if (timeToLive < 3.0f && dps > 0)
        return true;

    return false;
}

uint32 DefensiveCoordinator::GetRecommendedDefensive(ObjectGuid player) const
{
    ThreatAssessment assessment = AssessTeammate(player);
    auto defensives = GetAvailableDefensives(player);

    if (defensives.empty())
        return 0;

    // Sort by value for current situation
    ::std::sort(defensives.begin(), defensives.end(),
        [&](const DefensiveCooldown& a, const DefensiveCooldown& b)
        {
            return ScoreDefensiveValue(a, assessment) > ScoreDefensiveValue(b, assessment);
        });

    return defensives[0].spellId;
}

bool DefensiveCoordinator::HasDefensivesAvailable(ObjectGuid player) const
{
    return !GetAvailableDefensives(player).empty();
}

::std::vector<DefensiveCooldown> DefensiveCoordinator::GetAvailableDefensives(ObjectGuid player) const
{
    ::std::vector<DefensiveCooldown> available;

    auto it = _playerDefensives.find(player);
    if (it == _playerDefensives.end())
        return available;

    uint32 now = GameTime::GetGameTimeMS();
    for (const auto& def : it->second)
    {
        if (def.readyTime <= now && !def.isActive)
        {
            available.push_back(def);
        }
    }

    return available;
}

void DefensiveCoordinator::OnDefensiveUsed(ObjectGuid player, uint32 spellId)
{
    auto it = _playerDefensives.find(player);
    if (it == _playerDefensives.end())
        return;

    uint32 now = GameTime::GetGameTimeMS();
    for (auto& def : it->second)
    {
        if (def.spellId == spellId)
        {
            def.readyTime = now + def.cooldownDuration;
            def.isActive = true;
            def.activeEndTime = now + def.effectDuration;
            break;
        }
    }
}

void DefensiveCoordinator::OnDefensiveExpired(ObjectGuid player, uint32 spellId)
{
    auto it = _playerDefensives.find(player);
    if (it == _playerDefensives.end())
        return;

    for (auto& def : it->second)
    {
        if (def.spellId == spellId)
        {
            def.isActive = false;
            break;
        }
    }
}

// ============================================================================
// EXTERNAL DEFENSIVE COORDINATION
// ============================================================================

ObjectGuid DefensiveCoordinator::GetExternalDefensiveTarget() const
{
    return GetMostEndangeredTeammate();
}

bool DefensiveCoordinator::ShouldUseExternalDefensive(ObjectGuid healer, ObjectGuid target) const
{
    (void)healer;  // Healer availability would be checked

    ThreatAssessment assessment = AssessTeammate(target);
    return assessment.state >= DefensiveState::IN_DANGER;
}

uint32 DefensiveCoordinator::GetRecommendedExternalDefensive(ObjectGuid healer, ObjectGuid target) const
{
    auto defensives = GetAvailableDefensives(healer);

    for (const auto& def : defensives)
    {
        if (def.isExternal)
        {
            return def.spellId;
        }
    }

    (void)target;
    return 0;
}

void DefensiveCoordinator::RequestExternalDefensive(ObjectGuid requester, uint8 urgency)
{
    TC_LOG_DEBUG("playerbot", "DefensiveCoordinator: External defensive requested, urgency %u", urgency);

    auto const& teammates = _coordinator->GetTeammates();
    if (!teammates.empty())
    {
        Player* leader = ObjectAccessor::FindPlayer(teammates.front().guid);
        if (leader && leader->GetGroup())
        {
            ObjectGuid groupGuid = leader->GetGroup()->GetGUID();
            BotMessage msg = BotMessage::CommandUseDefensives(requester, groupGuid);
            msg.value = static_cast<float>(urgency);
            sBotMessageBus->Publish(msg);
        }
    }
}

// ============================================================================
// CC BREAK COORDINATION
// ============================================================================

bool DefensiveCoordinator::ShouldTrinket(ObjectGuid player) const
{
    if (!IsCCDangerous(player))
        return false;

    const ArenaTeammate* teammate = _coordinator->GetTeammate(player);
    if (!teammate || !teammate->isCC)
        return false;

    // Check if we'll die in CC
    uint32 ccRemaining = teammate->ccEndTime > GameTime::GetGameTimeMS() ?
        teammate->ccEndTime - GameTime::GetGameTimeMS() : 0;

    return WillDieInCC(player, ccRemaining);
}

bool DefensiveCoordinator::ShouldBreakCC(ObjectGuid player) const
{
    return GetCCBreakPriority(player) >= 2;
}

uint8 DefensiveCoordinator::GetCCBreakPriority(ObjectGuid player) const
{
    const ArenaTeammate* teammate = _coordinator->GetTeammate(player);
    if (!teammate || !teammate->isCC)
        return 0;

    // Priority 3: Will die in CC
    if (WillDieInCC(player, 5000))
        return 3;

    // Priority 2: Healer in CC while team is dying
    if (teammate->role == ArenaRole::HEALER && IsHealerCCed())
    {
        // Check if team is in trouble
        float teamHealth = _coordinator->GetTeamHealthPercent();
        if (teamHealth < 50.0f)
            return 2;
    }

    // Priority 1: In CC during enemy burst
    if (_coordinator->IsBurstWindowActive())
    {
        const BurstWindow* burst = _coordinator->GetCurrentBurstWindow();
        if (burst && burst->target == player)
            return 2;
    }

    return 0;
}

void DefensiveCoordinator::OnTrinketUsed(ObjectGuid player)
{
    TC_LOG_DEBUG("playerbot", "DefensiveCoordinator: Trinket used by player");

    // Update teammate's CC state
    if (ArenaTeammate* teammate = _coordinator->GetTeammateMutable(player))
    {
        teammate->isCC = false;
    }
}

// ============================================================================
// DAMAGE TRACKING
// ============================================================================

void DefensiveCoordinator::OnDamageTaken(ObjectGuid target, ObjectGuid attacker, uint32 damage)
{
    RecordDamage(target, attacker, damage);
}

float DefensiveCoordinator::GetRecentDamageTaken(ObjectGuid player) const
{
    auto it = _recentDamage.find(player);
    if (it == _recentDamage.end())
        return 0.0f;

    float total = 0.0f;
    for (const auto& record : it->second)
    {
        total += record.damage;
    }

    return total;
}

float DefensiveCoordinator::GetDamageRate(ObjectGuid player) const
{
    float recentDamage = GetRecentDamageTaken(player);
    return (recentDamage / _damageTrackingWindow) * 1000.0f;  // DPS
}

ObjectGuid DefensiveCoordinator::GetPrimaryAttacker(ObjectGuid target) const
{
    auto it = _recentDamage.find(target);
    if (it == _recentDamage.end())
        return ObjectGuid::Empty;

    // Count damage by attacker
    ::std::map<ObjectGuid, uint32> damageByAttacker;
    for (const auto& record : it->second)
    {
        damageByAttacker[record.attacker] += record.damage;
    }

    // Find highest
    ObjectGuid primary;
    uint32 highestDamage = 0;
    for (const auto& [attacker, damage] : damageByAttacker)
    {
        if (damage > highestDamage)
        {
            highestDamage = damage;
            primary = attacker;
        }
    }

    return primary;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void DefensiveCoordinator::SetHealthThresholds(float pressured, float danger, float critical)
{
    _healthThresholdPressured = pressured;
    _healthThresholdDanger = danger;
    _healthThresholdCritical = critical;
}

// ============================================================================
// THREAT ASSESSMENT (PRIVATE)
// ============================================================================

DefensiveState DefensiveCoordinator::CalculateDefensiveState(ObjectGuid player) const
{
    const ArenaTeammate* teammate = _coordinator->GetTeammate(player);
    if (!teammate)
        return DefensiveState::HEALTHY;

    // Check for active defensive
    auto it = _playerDefensives.find(player);
    if (it != _playerDefensives.end())
    {
        for (const auto& def : it->second)
        {
            if (def.isActive)
                return DefensiveState::USING_DEFENSIVES;
        }
    }

    // Check health thresholds
    if (teammate->healthPercent < _healthThresholdCritical)
        return DefensiveState::CRITICAL;
    if (teammate->healthPercent < _healthThresholdDanger)
        return DefensiveState::IN_DANGER;
    if (teammate->healthPercent < _healthThresholdPressured)
        return DefensiveState::PRESSURED;

    // Check incoming damage rate
    float dps = GetDamageRate(player);
    if (dps > _damageRateThreshold)
        return DefensiveState::PRESSURED;

    return DefensiveState::HEALTHY;
}

uint8 DefensiveCoordinator::CalculateUrgency(const ThreatAssessment& assessment) const
{
    uint8 urgency = 0;

    // State-based urgency
    switch (assessment.state)
    {
        case DefensiveState::CRITICAL:
            urgency = 3;
            break;
        case DefensiveState::IN_DANGER:
            urgency = 2;
            break;
        case DefensiveState::PRESSURED:
            urgency = 1;
            break;
        default:
            break;
    }

    // Adjust for being focused
    if (assessment.isBeingFocused && urgency < 3)
        urgency++;

    // Adjust for CC
    if (assessment.hasActiveCCs && urgency < 3)
        urgency++;

    return urgency;
}

bool DefensiveCoordinator::IsBeingFocused(ObjectGuid player) const
{
    // Player is being focused if they're receiving high DPS
    float dps = GetDamageRate(player);
    return dps > _damageRateThreshold;
}

// ============================================================================
// PEEL LOGIC (PRIVATE)
// ============================================================================

void DefensiveCoordinator::UpdatePeelAssignments(uint32 /*diff*/)
{
    uint32 now = GameTime::GetGameTimeMS();

    // Remove expired peels
    for (auto it = _activePeels.begin(); it != _activePeels.end(); )
    {
        PeelAssignment& peel = it->second;
        if (now - peel.assignTime >= peel.duration)
        {
            it = _activePeels.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void DefensiveCoordinator::ProcessPeelRequests()
{
    for (auto& request : _pendingPeels)
    {
        if (request.isFilled)
            continue;

        ObjectGuid peeler = GetBestPeeler(request.teammate, request.threat);
        if (!peeler.IsEmpty())
        {
            AssignPeel(peeler, request.teammate, request.threat);
            request.isFilled = true;
            request.assignedPeeler = peeler;
        }
    }

    // Clean up old requests
    uint32 now = GameTime::GetGameTimeMS();
    _pendingPeels.erase(
        ::std::remove_if(_pendingPeels.begin(), _pendingPeels.end(),
            [now](const PeelRequest& r)
            {
                return r.isFilled || (now - r.requestTime > 5000);
            }),
        _pendingPeels.end());
}

bool DefensiveCoordinator::CanPeel(ObjectGuid peeler, ObjectGuid threat) const
{
    // Check if peeler has CC available
    return _coordinator->CanCCTarget(threat);
}

float DefensiveCoordinator::ScorePeeler(ObjectGuid peeler, ObjectGuid target, ObjectGuid threat) const
{
    float score = 1.0f;

    const ArenaTeammate* tm = _coordinator->GetTeammate(peeler);
    if (!tm)
        return 0.0f;

    // Melee DPS are better peelers
    if (tm->role == ArenaRole::MELEE_DPS)
        score += 0.5f;

    // Check CC availability
    if (_coordinator->CanCCTarget(threat))
        score += 1.0f;

    // Consider distance (would need actual calculation)
    (void)target;

    return score;
}

// ============================================================================
// DEFENSIVE COOLDOWN LOGIC (PRIVATE)
// ============================================================================

void DefensiveCoordinator::LoadPlayerDefensives(ObjectGuid player)
{
    Player* p = ObjectAccessor::FindPlayer(player);
    if (!p)
        return;

    ::std::vector<DefensiveCooldown> defensives;

    for (const auto& spell : DEFENSIVE_SPELLS)
    {
        if (p->HasSpell(spell.spellId))
        {
            DefensiveCooldown def;
            def.spellId = spell.spellId;
            def.type = spell.type;
            def.cooldownDuration = spell.cooldown;
            def.effectDuration = spell.effectDuration;
            def.damageReduction = spell.damageReduction;
            def.isExternal = spell.isExternal;
            def.breaksCC = spell.breaksCC;
            def.providesImmunity = (spell.damageReduction >= 1.0f);
            def.readyTime = 0;
            def.isActive = false;
            def.activeEndTime = 0;

            defensives.push_back(def);
        }
    }

    _playerDefensives[player] = defensives;
}

void DefensiveCoordinator::UpdateCooldownTimers(uint32 /*diff*/)
{
    // Cooldowns are tracked by readyTime, nothing to update
}

void DefensiveCoordinator::UpdateActiveDefensives(uint32 /*diff*/)
{
    uint32 now = GameTime::GetGameTimeMS();

    for (auto& [player, defensives] : _playerDefensives)
    {
        for (auto& def : defensives)
        {
            if (def.isActive && now >= def.activeEndTime)
            {
                def.isActive = false;
            }
        }
    }
}

float DefensiveCoordinator::ScoreDefensiveValue(const DefensiveCooldown& defensive,
                                                 const ThreatAssessment& threat) const
{
    float score = 0.0f;

    // Value damage reduction
    score += defensive.damageReduction * 5.0f;

    // Value duration
    score += defensive.effectDuration / 1000.0f;

    // Value immunity highly in critical situations
    if (threat.state == DefensiveState::CRITICAL && defensive.providesImmunity)
    {
        score += 10.0f;
    }

    // Value CC break if in CC
    if (threat.hasActiveCCs && defensive.breaksCC)
    {
        score += 5.0f;
    }

    return score;
}

// ============================================================================
// CC BREAK LOGIC (PRIVATE)
// ============================================================================

bool DefensiveCoordinator::IsCCDangerous(ObjectGuid player) const
{
    const ArenaTeammate* teammate = _coordinator->GetTeammate(player);
    if (!teammate || !teammate->isCC)
        return false;

    // CC is dangerous if we're taking damage
    float dps = GetDamageRate(player);
    return dps > 1000.0f;  // Taking more than 1k DPS
}

bool DefensiveCoordinator::WillDieInCC(ObjectGuid player, uint32 ccDuration) const
{
    const ArenaTeammate* teammate = _coordinator->GetTeammate(player);
    if (!teammate)
        return false;

    float dps = GetDamageRate(player);
    float damageInCC = dps * (ccDuration / 1000.0f);
    float currentHealth = teammate->healthPercent;

    // Would die if damage exceeds remaining health
    return damageInCC > currentHealth;
}

bool DefensiveCoordinator::IsHealerCCed() const
{
    const ArenaTeammate* healer = _coordinator->GetTeamHealer();
    return healer && healer->isCC;
}

// ============================================================================
// DAMAGE TRACKING (PRIVATE)
// ============================================================================

void DefensiveCoordinator::CleanOldDamageRecords(uint32 currentTime)
{
    uint32 cutoff = currentTime - _damageTrackingWindow;

    for (auto& [player, records] : _recentDamage)
    {
        records.erase(
            ::std::remove_if(records.begin(), records.end(),
                [cutoff](const DamageRecord& r) { return r.timestamp < cutoff; }),
            records.end());
    }
}

void DefensiveCoordinator::RecordDamage(ObjectGuid target, ObjectGuid attacker, uint32 damage)
{
    DamageRecord record;
    record.timestamp = GameTime::GetGameTimeMS();
    record.attacker = attacker;
    record.damage = damage;

    _recentDamage[target].push_back(record);
}

} // namespace Playerbot
