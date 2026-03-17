/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BurstCoordinator.h"
#include "ArenaCoordinator.h"
#include "AI/Coordination/Messaging/BotMessageBus.h"
#include "AI/Coordination/Messaging/BotMessage.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "Group.h"
#include "SharedDefines.h"
#include "GameTime.h"
#include "Log.h"
#include <algorithm>

namespace Playerbot {

// ============================================================================
// BURST SPELL DATABASE BY CLASS
// ============================================================================

static const ::std::map<uint32, ::std::vector<uint32>> CLASS_BURST_SPELLS = {
    { CLASS_WARRIOR, { 1719, 107574 } },                    // Recklessness, Avatar
    { CLASS_PALADIN, { 31884, 231895 } },                   // Avenging Wrath, Crusade
    { CLASS_HUNTER, { 193530, 288613, 360952 } },           // Aspect of the Wild, Trueshot, Coordinated Assault
    { CLASS_ROGUE, { 13750, 121471, 385616 } },             // Adrenaline Rush, Shadow Blades, Kingsbane
    { CLASS_PRIEST, { 10060, 228260 } },                    // Power Infusion, Void Eruption
    { CLASS_DEATH_KNIGHT, { 47568, 51271, 49206 } },        // Empower Rune Weapon, Pillar of Frost, Summon Gargoyle
    { CLASS_SHAMAN, { 114051, 191634 } },                   // Ascendance, Stormkeeper
    { CLASS_MAGE, { 12472, 190319, 365350 } },              // Icy Veins, Combustion, Arcane Surge
    { CLASS_WARLOCK, { 113860, 113858, 267217 } },          // Dark Soul: Misery, Dark Soul: Instability, Nether Portal
    { CLASS_MONK, { 137639, 152173, 322507 } },             // Storm, Earth, and Fire; Serenity; Celestial Brew
    { CLASS_DRUID, { 194223, 102560, 106951 } },            // Celestial Alignment, Incarnation, Berserk
    { CLASS_DEMON_HUNTER, { 191427, 258925 } },             // Metamorphosis, Fel Barrage
    { CLASS_EVOKER, { 375087, 370553 } }                    // Dragonrage, Tip the Scales
};

// ============================================================================
// CONSTRUCTOR
// ============================================================================

BurstCoordinator::BurstCoordinator(ArenaCoordinator* coordinator)
    : _coordinator(coordinator)
{
}

void BurstCoordinator::Initialize()
{
    Reset();

    // Initialize cooldown tracking for all teammates
    for (const auto& teammate : _coordinator->GetTeammates())
    {
        InitializePlayerCooldowns(teammate.guid);
    }

    TC_LOG_DEBUG("playerbot", "BurstCoordinator::Initialize - Initialized");
}

void BurstCoordinator::Update(uint32 diff)
{
    // Update cooldown timers
    UpdateCooldownTimers(diff);

    // Update active burst
    if (_currentBurst.isActive)
    {
        UpdatePhase(diff);
    }
}

void BurstCoordinator::Reset()
{
    _currentBurst.Reset();
    _phaseStartTime = 0;
    _playerCooldowns.clear();
    _burstWindowCount = 0;
    _successfulBurstCount = 0;
    _lastBurstEndTime = 0;
}

// ============================================================================
// BURST WINDOW MANAGEMENT
// ============================================================================

bool BurstCoordinator::StartBurst(ObjectGuid target)
{
    if (_currentBurst.isActive)
    {
        TC_LOG_DEBUG("playerbot", "BurstCoordinator::StartBurst - Burst already active");
        return false;
    }

    const ArenaEnemy* enemy = _coordinator->GetEnemy(target);
    if (!enemy)
    {
        TC_LOG_DEBUG("playerbot", "BurstCoordinator::StartBurst - Invalid target");
        return false;
    }

    // Initialize burst window
    _currentBurst.Reset();
    _currentBurst.target = target;
    _currentBurst.startTime = GameTime::GetGameTimeMS();
    _currentBurst.targetHealthAtStart = enemy->healthPercent;
    _currentBurst.lowestHealthReached = enemy->healthPercent;
    _currentBurst.isActive = true;

    // Get ready bursters
    _currentBurst.participants = GetReadyBursters();

    // Start in preparing phase
    TransitionToPhase(BurstPhase::PREPARING);

    _burstWindowCount++;

    TC_LOG_DEBUG("playerbot", "BurstCoordinator::StartBurst - Started burst window #%u with %zu participants",
                 _burstWindowCount, _currentBurst.participants.size());

    // Broadcast burst window and focus target via BotMessageBus
    auto const& teammates = _coordinator->GetTeammates();
    if (!teammates.empty())
    {
        Player* leader = ObjectAccessor::FindPlayer(teammates.front().guid);
        if (leader && leader->GetGroup())
        {
            ObjectGuid groupGuid = leader->GetGroup()->GetGUID();
            ObjectGuid senderGuid = leader->GetGroup()->GetLeaderGUID();

            sBotMessageBus->Publish(BotMessage::AnnounceBurstWindow(senderGuid, groupGuid, _burstDurationThreshold));
            sBotMessageBus->Publish(BotMessage::CommandFocusTarget(senderGuid, groupGuid, target));
        }
    }

    return true;
}

void BurstCoordinator::EndBurst()
{
    if (!_currentBurst.isActive)
        return;

    // Record success/failure
    if (_currentBurst.targetKilled ||
        _currentBurst.lowestHealthReached < _healthTargetThreshold)
    {
        _successfulBurstCount++;
    }

    _lastBurstEndTime = GameTime::GetGameTimeMS();
    _currentBurst.duration = _lastBurstEndTime - _currentBurst.startTime;

    TC_LOG_DEBUG("playerbot", "BurstCoordinator::EndBurst - Burst ended. Target %s, lowest health %.1f%%",
                 _currentBurst.targetKilled ? "killed" : "survived",
                 _currentBurst.lowestHealthReached);

    _currentBurst.isActive = false;
    _currentBurst.phase = BurstPhase::NONE;
}

// ============================================================================
// BURST OPPORTUNITY EVALUATION
// ============================================================================

::std::vector<BurstOpportunity> BurstCoordinator::EvaluateOpportunities() const
{
    ::std::vector<BurstOpportunity> opportunities;

    for (const ArenaEnemy& enemy : _coordinator->GetAliveEnemies())
    {
        BurstOpportunity opp = EvaluateTarget(enemy.guid);
        if (opp.score > 0)
        {
            opportunities.push_back(opp);
        }
    }

    // Sort by score
    ::std::sort(opportunities.begin(), opportunities.end(),
        [](const BurstOpportunity& a, const BurstOpportunity& b)
        {
            return a.score > b.score;
        });

    return opportunities;
}

BurstOpportunity BurstCoordinator::EvaluateTarget(ObjectGuid target) const
{
    BurstOpportunity opp;
    opp.target = target;

    const ArenaEnemy* enemy = _coordinator->GetEnemy(target);
    if (!enemy)
        return opp;

    // Check conditions
    opp.trinketDown = !enemy->trinketAvailable;
    opp.defensivesDown = !enemy->isInDefensiveCooldown;
    opp.inCC = enemy->isInCC;

    if (opp.inCC && enemy->ccEndTime > 0)
    {
        uint32 now = GameTime::GetGameTimeMS();
        opp.ccRemainingMs = enemy->ccEndTime > now ? enemy->ccEndTime - now : 0;
    }

    opp.readyBursters = GetReadyBursters();

    // Calculate score
    opp.score = CalculateBurstScore(*enemy);

    // Build reason string
    if (opp.trinketDown)
        opp.reason += "Trinket down. ";
    if (opp.defensivesDown)
        opp.reason += "No defensives. ";
    if (opp.inCC)
        opp.reason += "In CC. ";
    if (enemy->role == ArenaRole::HEALER)
        opp.reason += "Healer target. ";

    return opp;
}

float BurstCoordinator::CalculateBurstScore(const ArenaEnemy& enemy) const
{
    float score = 0.0f;

    // Component scores
    score += ScoreTrinketStatus(enemy);
    score += ScoreDefensiveStatus(enemy);
    score += ScoreCCStatus(enemy);
    score += ScoreHealthStatus(enemy);
    score += ScoreRole(enemy);
    score += ScoreTeamReadiness();

    return score;
}

bool BurstCoordinator::ShouldInitiateBurst() const
{
    // Don't burst if one is already active
    if (_currentBurst.isActive)
        return false;

    // Need minimum bursters ready
    if (GetReadyBursterCount() < _minBurstersRequired)
        return false;

    // Find best opportunity
    auto opportunities = EvaluateOpportunities();
    if (opportunities.empty())
        return false;

    // High score threshold for auto-burst
    return opportunities[0].score >= 5.0f;
}

ObjectGuid BurstCoordinator::GetBestBurstTarget() const
{
    auto opportunities = EvaluateOpportunities();
    return opportunities.empty() ? ObjectGuid::Empty : opportunities[0].target;
}

// ============================================================================
// COOLDOWN TRACKING
// ============================================================================

bool BurstCoordinator::IsCooldownReady(ObjectGuid player, uint32 spellId) const
{
    auto it = _playerCooldowns.find(player);
    if (it == _playerCooldowns.end())
        return false;

    for (const auto& cd : it->second)
    {
        if (cd.spellId == spellId)
        {
            return cd.readyTime <= GameTime::GetGameTimeMS();
        }
    }

    return false;
}

bool BurstCoordinator::HasBurstReady(ObjectGuid player) const
{
    auto it = _playerCooldowns.find(player);
    if (it == _playerCooldowns.end())
        return false;

    uint32 now = GameTime::GetGameTimeMS();

    for (const auto& cd : it->second)
    {
        if (cd.readyTime <= now)
            return true;
    }

    return false;
}

uint32 BurstCoordinator::GetReadyBursterCount() const
{
    return static_cast<uint32>(GetReadyBursters().size());
}

::std::vector<ObjectGuid> BurstCoordinator::GetReadyBursters() const
{
    ::std::vector<ObjectGuid> ready;

    for (const auto& teammate : _coordinator->GetAliveTeammates())
    {
        if (HasBurstReady(teammate.guid))
        {
            ready.push_back(teammate.guid);
        }
    }

    return ready;
}

void BurstCoordinator::OnCooldownUsed(ObjectGuid player, uint32 spellId)
{
    auto it = _playerCooldowns.find(player);
    if (it == _playerCooldowns.end())
        return;

    uint32 now = GameTime::GetGameTimeMS();

    for (auto& cd : it->second)
    {
        if (cd.spellId == spellId)
        {
            cd.readyTime = now + GetCooldownDuration(spellId);
            cd.isActive = true;
            break;
        }
    }
}

void BurstCoordinator::OnCooldownReady(ObjectGuid player, uint32 spellId)
{
    auto it = _playerCooldowns.find(player);
    if (it == _playerCooldowns.end())
        return;

    for (auto& cd : it->second)
    {
        if (cd.spellId == spellId)
        {
            cd.isActive = false;
            break;
        }
    }
}

// ============================================================================
// BURST PARTICIPATION
// ============================================================================

bool BurstCoordinator::ShouldUseCooldowns(ObjectGuid player) const
{
    if (!_currentBurst.isActive)
        return false;

    // Only use CDs during executing phase
    if (_currentBurst.phase != BurstPhase::EXECUTING)
        return false;

    // Check if this player is a participant
    return IsParticipatingInBurst(player);
}

bool BurstCoordinator::IsParticipatingInBurst(ObjectGuid player) const
{
    for (const ObjectGuid& p : _currentBurst.participants)
    {
        if (p == player)
            return true;
    }
    return false;
}

void BurstCoordinator::OnPlayerJoinedBurst(ObjectGuid player)
{
    if (!IsParticipatingInBurst(player))
    {
        _currentBurst.participants.push_back(player);
    }
}

void BurstCoordinator::OnPlayerLeftBurst(ObjectGuid player)
{
    auto& participants = _currentBurst.participants;
    participants.erase(
        ::std::remove(participants.begin(), participants.end(), player),
        participants.end());
}

uint32 BurstCoordinator::GetBurstDuration() const
{
    if (!_currentBurst.isActive)
        return 0;

    return GameTime::GetGameTimeMS() - _currentBurst.startTime;
}

// ============================================================================
// BURST SUCCESS TRACKING
// ============================================================================

float BurstCoordinator::GetBurstProgress() const
{
    if (!_currentBurst.isActive)
        return 0.0f;

    // Progress = how much health we've taken
    float healthLost = _currentBurst.targetHealthAtStart - _currentBurst.lowestHealthReached;
    float targetHealthLoss = _currentBurst.targetHealthAtStart - _healthTargetThreshold;

    if (targetHealthLoss <= 0)
        return 1.0f;

    return ::std::min(healthLost / targetHealthLoss, 1.0f);
}

bool BurstCoordinator::IsBurstSuccessful() const
{
    if (!_currentBurst.isActive)
        return false;

    return _currentBurst.targetKilled ||
           _currentBurst.lowestHealthReached < _healthTargetThreshold;
}

bool BurstCoordinator::IsBurstFailing() const
{
    if (!_currentBurst.isActive)
        return false;

    // Burst is failing if:
    // 1. Duration exceeds threshold
    // 2. Target used both trinket AND defensive and is recovering

    uint32 duration = GetBurstDuration();
    if (duration > _burstDurationThreshold)
        return true;

    // Target stabilizing with defensives
    if (_currentBurst.targetUsedDefensive)
    {
        const ArenaEnemy* target = _coordinator->GetEnemy(_currentBurst.target);
        if (target && target->healthPercent > _currentBurst.lowestHealthReached + 10.0f)
        {
            return true;  // Target recovering
        }
    }

    return false;
}

void BurstCoordinator::OnTargetUsedDefensive(ObjectGuid target)
{
    if (_currentBurst.isActive && _currentBurst.target == target)
    {
        _currentBurst.targetUsedDefensive = true;

        TC_LOG_DEBUG("playerbot", "BurstCoordinator: Target used defensive");

        // Consider transitioning to sustaining phase
        if (_currentBurst.phase == BurstPhase::EXECUTING)
        {
            TransitionToPhase(BurstPhase::SUSTAINING);
        }
    }
}

void BurstCoordinator::OnTargetUsedTrinket(ObjectGuid target)
{
    if (_currentBurst.isActive && _currentBurst.target == target)
    {
        _currentBurst.targetUsedTrinket = true;

        TC_LOG_DEBUG("playerbot", "BurstCoordinator: Target used trinket - great for next go!");
    }
}

void BurstCoordinator::OnTargetDied(ObjectGuid target)
{
    if (_currentBurst.isActive && _currentBurst.target == target)
    {
        _currentBurst.targetKilled = true;
        _currentBurst.lowestHealthReached = 0.0f;

        TC_LOG_DEBUG("playerbot", "BurstCoordinator: Target killed!");

        EndBurst();
    }
}

// ============================================================================
// BURST HISTORY
// ============================================================================

float BurstCoordinator::GetBurstSuccessRate() const
{
    if (_burstWindowCount == 0)
        return 0.0f;

    return static_cast<float>(_successfulBurstCount) / _burstWindowCount;
}

// ============================================================================
// PHASE MANAGEMENT
// ============================================================================

void BurstCoordinator::UpdatePhase(uint32 diff)
{
    if (!_currentBurst.isActive)
        return;

    // Update target health tracking
    const ArenaEnemy* target = _coordinator->GetEnemy(_currentBurst.target);
    if (target)
    {
        if (target->healthPercent < _currentBurst.lowestHealthReached)
        {
            _currentBurst.lowestHealthReached = target->healthPercent;
        }

        // Check for kill
        if (target->healthPercent <= 0)
        {
            OnTargetDied(_currentBurst.target);
            return;
        }
    }

    // Check for phase advancement or abort
    if (ShouldAbortBurst())
    {
        TC_LOG_DEBUG("playerbot", "BurstCoordinator: Aborting burst");
        TransitionToPhase(BurstPhase::RETREATING);
    }
    else if (ShouldAdvancePhase())
    {
        BurstPhase nextPhase = static_cast<BurstPhase>(static_cast<uint8>(_currentBurst.phase) + 1);
        if (nextPhase <= BurstPhase::RETREATING)
        {
            TransitionToPhase(nextPhase);
        }
    }

    // Check for burst timeout
    if (GetBurstDuration() > _burstDurationThreshold)
    {
        TC_LOG_DEBUG("playerbot", "BurstCoordinator: Burst timeout");
        EndBurst();
    }
}

void BurstCoordinator::TransitionToPhase(BurstPhase newPhase)
{
    TC_LOG_DEBUG("playerbot", "BurstCoordinator: Phase transition %s -> %s",
                 BurstPhaseToString(_currentBurst.phase), BurstPhaseToString(newPhase));

    _currentBurst.phase = newPhase;
    _phaseStartTime = GameTime::GetGameTimeMS();

    if (newPhase == BurstPhase::RETREATING)
    {
        // End burst after retreating phase duration
        EndBurst();
    }
}

bool BurstCoordinator::ShouldAdvancePhase() const
{
    uint32 phaseIndex = static_cast<uint32>(_currentBurst.phase);
    if (phaseIndex >= 5)
        return false;

    uint32 phaseDuration = _phaseDurations[phaseIndex];
    uint32 timeInPhase = GameTime::GetGameTimeMS() - _phaseStartTime;

    return timeInPhase >= phaseDuration;
}

bool BurstCoordinator::ShouldAbortBurst() const
{
    return IsBurstFailing();
}

// ============================================================================
// COOLDOWN MANAGEMENT
// ============================================================================

void BurstCoordinator::InitializePlayerCooldowns(ObjectGuid player)
{
    const ArenaTeammate* teammate = _coordinator->GetTeammate(player);
    if (!teammate)
        return;

    ::std::vector<uint32> burstSpells = GetBurstSpellsForClass(teammate->classId);

    ::std::vector<BurstCooldown> cooldowns;
    for (uint32 spellId : burstSpells)
    {
        BurstCooldown cd;
        cd.player = player;
        cd.spellId = spellId;
        cd.readyTime = 0;  // Assume ready at start
        cd.duration = GetCooldownDuration(spellId);
        cd.damageMultiplier = GetCooldownDamageMultiplier(spellId);
        cd.isActive = false;
        cooldowns.push_back(cd);
    }

    _playerCooldowns[player] = cooldowns;
}

void BurstCoordinator::UpdateCooldownTimers(uint32 /*diff*/)
{
    uint32 now = GameTime::GetGameTimeMS();

    for (auto& [player, cooldowns] : _playerCooldowns)
    {
        for (auto& cd : cooldowns)
        {
            if (cd.isActive && now >= cd.readyTime)
            {
                cd.isActive = false;
            }
        }
    }
}

::std::vector<uint32> BurstCoordinator::GetBurstSpellsForClass(uint32 classId) const
{
    auto it = CLASS_BURST_SPELLS.find(classId);
    return it != CLASS_BURST_SPELLS.end() ? it->second : ::std::vector<uint32>();
}

float BurstCoordinator::GetCooldownDamageMultiplier(uint32 spellId) const
{
    // Simplified - most burst CDs are roughly 20-40% damage increase
    (void)spellId;
    return 1.3f;
}

uint32 BurstCoordinator::GetCooldownDuration(uint32 spellId) const
{
    // Simplified cooldown durations
    (void)spellId;
    return 180000;  // 3 minutes default
}

// ============================================================================
// SCORING
// ============================================================================

float BurstCoordinator::ScoreTrinketStatus(const ArenaEnemy& enemy) const
{
    return enemy.trinketAvailable ? 0.0f : _weightTrinketDown;
}

float BurstCoordinator::ScoreDefensiveStatus(const ArenaEnemy& enemy) const
{
    return enemy.isInDefensiveCooldown ? 0.0f : _weightDefensivesDown;
}

float BurstCoordinator::ScoreCCStatus(const ArenaEnemy& enemy) const
{
    return enemy.isInCC ? _weightInCC : 0.0f;
}

float BurstCoordinator::ScoreHealthStatus(const ArenaEnemy& enemy) const
{
    // Lower health = higher score
    if (enemy.healthPercent < 50.0f)
        return _weightLowHealth;
    return 0.0f;
}

float BurstCoordinator::ScoreRole(const ArenaEnemy& enemy) const
{
    return enemy.role == ArenaRole::HEALER ? _weightHealer : 0.0f;
}

float BurstCoordinator::ScoreTeamReadiness() const
{
    float readyRatio = static_cast<float>(GetReadyBursterCount()) /
                       _coordinator->GetAliveTeammateCount();

    return readyRatio * 2.0f;  // Weight team readiness
}

} // namespace Playerbot
