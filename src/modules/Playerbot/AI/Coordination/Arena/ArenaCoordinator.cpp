/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ArenaCoordinator.h"
#include "KillTargetManager.h"
#include "BurstCoordinator.h"
#include "CCChainManager.h"
#include "DefensiveCoordinator.h"
#include "ArenaPositioning.h"
#include "Core/Events/CombatEventRouter.h"
#include "AI/Combat/CrowdControlManager.h"
#include "AI/Coordination/Messaging/BotMessageBus.h"
#include "AI/Coordination/Messaging/BotMessage.h"
#include "Player.h"
#include "Battleground.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "GameTime.h"
#include "Log.h"
#include <algorithm>

namespace Playerbot {

// ============================================================================
// COOLDOWN DATABASE
// ============================================================================

// PvP trinket spells
static const ::std::vector<uint32> TRINKET_SPELLS = {
    336126,  // Gladiator's Medallion
    336135,  // Adaptation
    195710,  // Honorable Medallion
    42292,   // PvP Trinket (legacy)
    59752,   // Every Man for Himself
    7744     // Will of the Forsaken
};

// Major defensive cooldowns
static const ::std::vector<uint32> DEFENSIVE_COOLDOWNS = {
    // Death Knight
    48707,   // Anti-Magic Shell
    48792,   // Icebound Fortitude
    49039,   // Lichborne
    // Demon Hunter
    196555,  // Netherwalk
    198589,  // Blur
    // Druid
    22812,   // Barkskin
    61336,   // Survival Instincts
    102342,  // Ironbark
    // Hunter
    186265,  // Aspect of the Turtle
    // Mage
    45438,   // Ice Block
    // Monk
    122278,  // Dampen Harm
    122783,  // Diffuse Magic
    // Paladin
    498,     // Divine Protection
    642,     // Divine Shield
    1022,    // Blessing of Protection
    6940,    // Blessing of Sacrifice
    // Priest
    33206,   // Pain Suppression
    47585,   // Dispersion
    // Rogue
    1966,    // Feint
    31224,   // Cloak of Shadows
    5277,    // Evasion
    // Shaman
    108271,  // Astral Shift
    // Warlock
    104773,  // Unending Resolve
    // Warrior
    118038,  // Die by the Sword
    184364,  // Enraged Regeneration
    12975,   // Last Stand
    871      // Shield Wall
};

// Major offensive cooldowns
static const ::std::vector<uint32> OFFENSIVE_COOLDOWNS = {
    // Death Knight
    47568,   // Empower Rune Weapon
    51271,   // Pillar of Frost
    // Demon Hunter
    191427,  // Metamorphosis
    // Druid
    194223,  // Celestial Alignment
    102560,  // Incarnation: Chosen of Elune
    // Hunter
    193530,  // Aspect of the Wild
    288613,  // Trueshot
    // Mage
    12472,   // Icy Veins
    190319,  // Combustion
    365350,  // Arcane Surge
    // Monk
    137639,  // Storm, Earth, and Fire
    152173,  // Serenity
    // Paladin
    31884,   // Avenging Wrath
    // Priest
    228260,  // Void Eruption
    10060,   // Power Infusion
    // Rogue
    13750,   // Adrenaline Rush
    121471,  // Shadow Blades
    // Shaman
    114051,  // Ascendance
    // Warlock
    113860,  // Dark Soul: Misery
    113858,  // Dark Soul: Instability
    267217,  // Nether Portal
    // Warrior
    1719,    // Recklessness
    107574   // Avatar
};

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

ArenaCoordinator::ArenaCoordinator(Battleground* arena, ::std::vector<Player*> team)
    : _arena(arena)
    , _team(team)
    , _ccManager(nullptr)
{
    DetectArenaType();
}

ArenaCoordinator::~ArenaCoordinator()
{
    Shutdown();
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void ArenaCoordinator::Initialize()
{
    Reset();

    // Create sub-managers
    _killTargetManager = ::std::make_unique<KillTargetManager>(this);
    _burstCoordinator = ::std::make_unique<BurstCoordinator>(this);
    _defensiveCoordinator = ::std::make_unique<DefensiveCoordinator>(this);
    _positioning = ::std::make_unique<ArenaPositioning>(this);

    // CCChainManager needs the DR tracker from Phase 2
    // For now, we'll pass nullptr and handle it gracefully
    _ccChainManager = ::std::make_unique<CCChainManager>(this, _ccManager);

    // Initialize sub-managers
    _killTargetManager->Initialize();
    _burstCoordinator->Initialize();
    _ccChainManager->Initialize();
    _defensiveCoordinator->Initialize();

    if (_arena)
        _positioning->Initialize(_arena->GetMapId());

    // Initialize tracking
    InitializeTeammateTracking();

    // Subscribe to combat events
    CombatEventRouter::Instance().Subscribe(this);

    TC_LOG_DEBUG("playerbot", "ArenaCoordinator::Initialize - Initialized for %uv%u arena",
                 static_cast<uint8>(_type), static_cast<uint8>(_type));
}

void ArenaCoordinator::Shutdown()
{
    // Unsubscribe from events
    CombatEventRouter::Instance().Unsubscribe(this);

    _killTargetManager.reset();
    _burstCoordinator.reset();
    _ccChainManager.reset();
    _defensiveCoordinator.reset();
    _positioning.reset();

    TC_LOG_DEBUG("playerbot", "ArenaCoordinator::Shutdown - Shutdown complete");
}

void ArenaCoordinator::Update(uint32 diff)
{
    if (_state == ArenaState::IDLE || _state == ArenaState::QUEUED)
        return;

    // Update state machine
    UpdateState(diff);

    // Only update sub-managers during active combat
    if (_state == ArenaState::COMBAT)
    {
        // Update tracking
        UpdateEnemyTracking(diff);
        UpdateTeammateTracking(diff);
        UpdateCooldownTimers(diff);

        // Update sub-managers
        if (_killTargetManager)
            _killTargetManager->Update(diff);

        if (_burstCoordinator)
            _burstCoordinator->Update(diff);

        if (_ccChainManager)
            _ccChainManager->Update(diff);

        if (_defensiveCoordinator)
            _defensiveCoordinator->Update(diff);

        if (_positioning)
            _positioning->Update(diff);
    }
}

void ArenaCoordinator::Reset()
{
    _state = ArenaState::IDLE;
    _enemies.clear();
    _teammates.clear();
    _currentBurst.Reset();
    _matchStartTime = 0;
    _gatesOpenTime = 0;
    _matchStats = ArenaMatchStats();

    if (_killTargetManager)
        _killTargetManager->Reset();

    if (_burstCoordinator)
        _burstCoordinator->Reset();

    if (_ccChainManager)
        _ccChainManager->Reset();

    if (_defensiveCoordinator)
        _defensiveCoordinator->Reset();

    if (_positioning)
        _positioning->Reset();
}

// ============================================================================
// STATE
// ============================================================================

uint32 ArenaCoordinator::GetMatchDuration() const
{
    if (_matchStartTime == 0)
        return 0;

    return GameTime::GetGameTimeMS() - _matchStartTime;
}

// ============================================================================
// KILL TARGET
// ============================================================================

ObjectGuid ArenaCoordinator::GetKillTarget() const
{
    return _killTargetManager ? _killTargetManager->GetKillTarget() : ObjectGuid::Empty;
}

void ArenaCoordinator::SetKillTarget(ObjectGuid target)
{
    if (_killTargetManager)
        _killTargetManager->SetKillTarget(target);
}

void ArenaCoordinator::ClearKillTarget()
{
    if (_killTargetManager)
        _killTargetManager->ClearKillTarget();
}

void ArenaCoordinator::CallSwitch(ObjectGuid newTarget)
{
    if (_killTargetManager)
    {
        _killTargetManager->OnSwitchCalled(newTarget);
        TC_LOG_DEBUG("playerbot", "ArenaCoordinator::CallSwitch - Switching to target");
    }
}

TargetPriority ArenaCoordinator::GetTargetPriority(ObjectGuid target) const
{
    const ArenaEnemy* enemy = GetEnemy(target);
    return enemy ? enemy->currentPriority : TargetPriority::NORMAL;
}

bool ArenaCoordinator::ShouldSwitch() const
{
    return _killTargetManager ? _killTargetManager->ShouldSwitch() : false;
}

ObjectGuid ArenaCoordinator::GetRecommendedKillTarget() const
{
    return _killTargetManager ? _killTargetManager->GetRecommendedTarget() : ObjectGuid::Empty;
}

// ============================================================================
// BURST COORDINATION
// ============================================================================

bool ArenaCoordinator::IsBurstWindowActive() const
{
    return _burstCoordinator ? _burstCoordinator->IsBurstActive() : false;
}

void ArenaCoordinator::CallBurst(ObjectGuid target)
{
    if (_burstCoordinator)
    {
        _burstCoordinator->StartBurst(target);
        _matchStats.burstWindowsInitiated++;
    }

    // Broadcast burst window + focus target via BotMessageBus
    if (!_team.empty())
    {
        Player* leader = _team.front();
        if (leader && leader->GetGroup())
        {
            ObjectGuid groupGuid = leader->GetGroup()->GetGUID();
            BotMessage burstMsg = BotMessage::AnnounceBurstWindow(leader->GetGUID(), groupGuid, 6000);
            sBotMessageBus->Publish(burstMsg);
            BotMessage focusMsg = BotMessage::CommandFocusTarget(leader->GetGUID(), groupGuid, target);
            sBotMessageBus->Publish(focusMsg);
        }
    }
}

void ArenaCoordinator::CallOffBurst()
{
    if (_burstCoordinator)
        _burstCoordinator->EndBurst();
}

bool ArenaCoordinator::ShouldUseCooldowns() const
{
    return _burstCoordinator ? _burstCoordinator->ShouldUseCooldowns(ObjectGuid::Empty) : false;
}

const BurstWindow* ArenaCoordinator::GetCurrentBurstWindow() const
{
    return _burstCoordinator ? &_burstCoordinator->GetCurrentBurst() : nullptr;
}

BurstPhase ArenaCoordinator::GetBurstPhase() const
{
    return _burstCoordinator ? _burstCoordinator->GetPhase() : BurstPhase::NONE;
}

bool ArenaCoordinator::CanInitiateBurst() const
{
    return _burstCoordinator ? _burstCoordinator->ShouldInitiateBurst() : false;
}

uint32 ArenaCoordinator::GetBurstReadyCount() const
{
    return _burstCoordinator ? _burstCoordinator->GetReadyBursterCount() : 0;
}

// ============================================================================
// CC MANAGEMENT
// ============================================================================

void ArenaCoordinator::RequestCC(ObjectGuid target, uint32 durationMs, uint8 priority)
{
    if (_ccChainManager)
        _ccChainManager->RequestCC(ObjectGuid::Empty, target, durationMs, priority);
}

void ArenaCoordinator::CallCCChain(ObjectGuid target)
{
    if (_ccChainManager)
        _ccChainManager->StartChain(target, true, false);
}

void ArenaCoordinator::EndCCChain()
{
    if (_ccChainManager)
        _ccChainManager->EndChain();
}

bool ArenaCoordinator::CanCCTarget(ObjectGuid target) const
{
    return _ccChainManager ? _ccChainManager->CanChainTarget(target) : false;
}

float ArenaCoordinator::GetExpectedCCDuration(ObjectGuid target, uint32 spellId) const
{
    return _ccChainManager ? static_cast<float>(_ccChainManager->GetExpectedDuration(target, spellId)) : 0.0f;
}

uint8 ArenaCoordinator::GetDRStacks(ObjectGuid target, uint32 spellId) const
{
    // This would use the CCChainManager's GetDRStacks with category lookup
    return 0;  // Simplified - real implementation would use DR tracker
}

bool ArenaCoordinator::IsTargetCCImmune(ObjectGuid target) const
{
    const ArenaEnemy* enemy = GetEnemy(target);
    if (!enemy)
        return false;

    // Check if any CC category is immune (3 stacks)
    return false;  // Simplified
}

ObjectGuid ArenaCoordinator::GetCCChainTarget() const
{
    return _ccChainManager ? _ccChainManager->GetChainTarget() : ObjectGuid::Empty;
}

bool ArenaCoordinator::IsCCChainActive() const
{
    return _ccChainManager ? _ccChainManager->IsChainActive() : false;
}

// ============================================================================
// DEFENSIVE COORDINATION
// ============================================================================

void ArenaCoordinator::RequestPeel(ObjectGuid teammate, uint8 urgency)
{
    if (_defensiveCoordinator)
        _defensiveCoordinator->RequestPeel(teammate, ObjectGuid::Empty, urgency);
}

void ArenaCoordinator::CallDefensives(ObjectGuid target)
{
    if (_defensiveCoordinator)
    {
        // Request external defensives for target
        _defensiveCoordinator->RequestExternalDefensive(target, 3);
    }
}

ObjectGuid ArenaCoordinator::GetPeelTarget() const
{
    return _defensiveCoordinator ? _defensiveCoordinator->GetPeelTarget() : ObjectGuid::Empty;
}

bool ArenaCoordinator::ShouldUseDefensive(ObjectGuid player) const
{
    return _defensiveCoordinator ? _defensiveCoordinator->ShouldUseDefensive(player) : false;
}

DefensiveState ArenaCoordinator::GetTeammateDefensiveState(ObjectGuid teammate) const
{
    return _defensiveCoordinator ? _defensiveCoordinator->GetTeammateState(teammate) : DefensiveState::HEALTHY;
}

bool ArenaCoordinator::IsTeammateInTrouble(ObjectGuid teammate) const
{
    DefensiveState state = GetTeammateDefensiveState(teammate);
    return state >= DefensiveState::IN_DANGER;
}

ObjectGuid ArenaCoordinator::GetMostEndangeredTeammate() const
{
    return _defensiveCoordinator ? _defensiveCoordinator->GetMostEndangeredTeammate() : ObjectGuid::Empty;
}

// ============================================================================
// POSITIONING
// ============================================================================

void ArenaCoordinator::RequestReposition(ObjectGuid player, float x, float y, float z)
{
    if (_positioning)
        _positioning->RequestReposition(player, x, y, z);
}

bool ArenaCoordinator::ShouldLOS() const
{
    return _positioning ? _positioning->ShouldLOS() : false;
}

float ArenaCoordinator::GetPillarDistance(ObjectGuid player) const
{
    return _positioning ? _positioning->GetPillarDistance(player) : 0.0f;
}

bool ArenaCoordinator::IsInLOSOfHealer(ObjectGuid player) const
{
    return _positioning ? _positioning->IsInLOSOfHealer(player) : true;
}

bool ArenaCoordinator::IsInLOSOfKillTarget(ObjectGuid player) const
{
    return _positioning ? _positioning->IsInLOSOfKillTarget(player) : true;
}

void ArenaCoordinator::GetRecommendedPosition(ObjectGuid player, float& x, float& y, float& z) const
{
    if (_positioning)
    {
        PositionRecommendation rec = _positioning->GetRecommendedPosition(player);
        x = rec.x;
        y = rec.y;
        z = rec.z;
    }
}

// ============================================================================
// ENEMY TRACKING
// ============================================================================

const ArenaEnemy* ArenaCoordinator::GetEnemy(ObjectGuid guid) const
{
    for (const auto& enemy : _enemies)
    {
        if (enemy.guid == guid)
            return &enemy;
    }
    return nullptr;
}

ArenaEnemy* ArenaCoordinator::GetEnemyMutable(ObjectGuid guid)
{
    for (auto& enemy : _enemies)
    {
        if (enemy.guid == guid)
            return &enemy;
    }
    return nullptr;
}

::std::vector<ArenaEnemy> ArenaCoordinator::GetEnemies() const
{
    return _enemies;
}

::std::vector<ArenaEnemy> ArenaCoordinator::GetAliveEnemies() const
{
    ::std::vector<ArenaEnemy> alive;
    for (const auto& enemy : _enemies)
    {
        if (enemy.healthPercent > 0)
            alive.push_back(enemy);
    }
    return alive;
}

bool ArenaCoordinator::IsEnemyTrinketDown(ObjectGuid enemy) const
{
    const ArenaEnemy* e = GetEnemy(enemy);
    return e ? !e->trinketAvailable : false;
}

bool ArenaCoordinator::IsEnemyDefensiveDown(ObjectGuid enemy) const
{
    const ArenaEnemy* e = GetEnemy(enemy);
    return e ? !e->isInDefensiveCooldown : false;
}

bool ArenaCoordinator::IsEnemyInCC(ObjectGuid enemy) const
{
    const ArenaEnemy* e = GetEnemy(enemy);
    return e ? e->isInCC : false;
}

const ArenaEnemy* ArenaCoordinator::GetEnemyHealer() const
{
    for (const auto& enemy : _enemies)
    {
        if (enemy.role == ArenaRole::HEALER && enemy.healthPercent > 0)
            return &enemy;
    }
    return nullptr;
}

uint32 ArenaCoordinator::GetAliveEnemyCount() const
{
    uint32 count = 0;
    for (const auto& enemy : _enemies)
    {
        if (enemy.healthPercent > 0)
            count++;
    }
    return count;
}

// ============================================================================
// TEAMMATE TRACKING
// ============================================================================

const ArenaTeammate* ArenaCoordinator::GetTeammate(ObjectGuid guid) const
{
    for (const auto& teammate : _teammates)
    {
        if (teammate.guid == guid)
            return &teammate;
    }
    return nullptr;
}

ArenaTeammate* ArenaCoordinator::GetTeammateMutable(ObjectGuid guid)
{
    for (auto& teammate : _teammates)
    {
        if (teammate.guid == guid)
            return &teammate;
    }
    return nullptr;
}

::std::vector<ArenaTeammate> ArenaCoordinator::GetTeammates() const
{
    return _teammates;
}

::std::vector<ArenaTeammate> ArenaCoordinator::GetAliveTeammates() const
{
    ::std::vector<ArenaTeammate> alive;
    for (const auto& teammate : _teammates)
    {
        if (teammate.healthPercent > 0)
            alive.push_back(teammate);
    }
    return alive;
}

const ArenaTeammate* ArenaCoordinator::GetTeamHealer() const
{
    for (const auto& teammate : _teammates)
    {
        if (teammate.role == ArenaRole::HEALER && teammate.healthPercent > 0)
            return &teammate;
    }
    return nullptr;
}

uint32 ArenaCoordinator::GetAliveTeammateCount() const
{
    uint32 count = 0;
    for (const auto& teammate : _teammates)
    {
        if (teammate.healthPercent > 0)
            count++;
    }
    return count;
}

float ArenaCoordinator::GetTeamHealthPercent() const
{
    if (_teammates.empty())
        return 100.0f;

    float total = 0.0f;
    for (const auto& teammate : _teammates)
    {
        total += teammate.healthPercent;
    }
    return total / _teammates.size();
}

float ArenaCoordinator::GetTeamManaPercent() const
{
    if (_teammates.empty())
        return 100.0f;

    float total = 0.0f;
    uint32 count = 0;
    for (const auto& teammate : _teammates)
    {
        // Only count mana users
        if (teammate.role == ArenaRole::HEALER ||
            teammate.role == ArenaRole::RANGED_DPS)
        {
            total += teammate.manaPercent;
            count++;
        }
    }
    return count > 0 ? total / count : 100.0f;
}

// ============================================================================
// ICOMBATEVENTSUBSCRIBER
// ============================================================================

void ArenaCoordinator::OnCombatEvent(const CombatEvent& event)
{
    if (_state != ArenaState::COMBAT)
        return;

    switch (event.type)
    {
        case CombatEventType::DAMAGE_TAKEN:
            HandleDamageTaken(event);
            break;
        case CombatEventType::DAMAGE_DEALT:
            HandleDamageDealt(event);
            break;
        case CombatEventType::HEALING_DONE:
            HandleHealingDone(event);
            break;
        case CombatEventType::SPELL_CAST_START:
            HandleSpellCastStart(event);
            break;
        case CombatEventType::SPELL_CAST_SUCCESS:
            HandleSpellCastSuccess(event);
            break;
        case CombatEventType::SPELL_INTERRUPTED:
            HandleSpellInterrupted(event);
            break;
        case CombatEventType::AURA_APPLIED:
            HandleAuraApplied(event);
            break;
        case CombatEventType::AURA_REMOVED:
            HandleAuraRemoved(event);
            break;
        case CombatEventType::UNIT_DIED:
            HandleUnitDied(event);
            break;
        default:
            break;
    }
}

CombatEventType ArenaCoordinator::GetSubscribedEventTypes() const
{
    return CombatEventType::DAMAGE_TAKEN |
           CombatEventType::DAMAGE_DEALT |
           CombatEventType::HEALING_DONE |
           CombatEventType::SPELL_CAST_START |
           CombatEventType::SPELL_CAST_SUCCESS |
           CombatEventType::SPELL_INTERRUPTED |
           CombatEventType::AURA_APPLIED |
           CombatEventType::AURA_REMOVED |
           CombatEventType::UNIT_DIED;
}

// ============================================================================
// STATE MACHINE
// ============================================================================

void ArenaCoordinator::UpdateState(uint32 /*diff*/)
{
    // State transitions based on battleground status
    if (!_arena)
        return;

    // Check for state transitions
    switch (_state)
    {
        case ArenaState::PREPARATION:
            // Waiting for gates to open
            // In real implementation, check arena status
            break;

        case ArenaState::GATES_OPENING:
            // Short countdown before combat starts
            if (_gatesOpenTime > 0 && GameTime::GetGameTimeMS() - _gatesOpenTime >= 5000)
            {
                TransitionTo(ArenaState::COMBAT);
            }
            break;

        case ArenaState::COMBAT:
            // Check for win/loss
            if (GetAliveEnemyCount() == 0)
            {
                TransitionTo(ArenaState::VICTORY);
            }
            else if (GetAliveTeammateCount() == 0)
            {
                TransitionTo(ArenaState::DEFEAT);
            }
            break;

        default:
            break;
    }
}

void ArenaCoordinator::TransitionTo(ArenaState newState)
{
    if (_state == newState)
        return;

    OnStateExit(_state);

    TC_LOG_DEBUG("playerbot", "ArenaCoordinator: State transition %s -> %s",
                 ArenaStateToString(_state), ArenaStateToString(newState));

    _state = newState;
    OnStateEnter(newState);
}

void ArenaCoordinator::OnStateEnter(ArenaState state)
{
    switch (state)
    {
        case ArenaState::PREPARATION:
            _matchStats = ArenaMatchStats();
            InitializeTeammateTracking();
            break;

        case ArenaState::GATES_OPENING:
            _gatesOpenTime = GameTime::GetGameTimeMS();
            InitializeEnemyTracking();
            break;

        case ArenaState::COMBAT:
            _matchStartTime = GameTime::GetGameTimeMS();
            _matchStats.matchStartTime = _matchStartTime;
            _matchStats.teamSize = static_cast<uint8>(_type);
            break;

        case ArenaState::VICTORY:
        case ArenaState::DEFEAT:
            _matchStats.matchDuration = GetMatchDuration();
            if (_burstCoordinator)
            {
                _matchStats.burstWindowsSuccessful = _burstCoordinator->GetSuccessfulBurstCount();
            }
            break;

        default:
            break;
    }
}

void ArenaCoordinator::OnStateExit(ArenaState state)
{
    switch (state)
    {
        case ArenaState::COMBAT:
            // End any active burst/CC chains
            if (_burstCoordinator && _burstCoordinator->IsBurstActive())
                _burstCoordinator->EndBurst();
            if (_ccChainManager && _ccChainManager->IsChainActive())
                _ccChainManager->EndChain();
            break;

        default:
            break;
    }
}

// ============================================================================
// EVENT HANDLERS
// ============================================================================

void ArenaCoordinator::HandleDamageTaken(const CombatEvent& event)
{
    // Track damage for defensive decisions
    if (_defensiveCoordinator)
    {
        _defensiveCoordinator->OnDamageTaken(event.target, event.source, event.amount);
    }

    // Update enemy health tracking
    if (IsEnemy(event.target))
    {
        _matchStats.totalDamageDealt += event.amount;
    }
    else if (IsTeammate(event.target))
    {
        _matchStats.totalDamageTaken += event.amount;
    }
}

void ArenaCoordinator::HandleDamageDealt(const CombatEvent& event)
{
    // Similar to HandleDamageTaken but from our perspective
    if (IsTeammate(event.source) && IsEnemy(event.target))
    {
        _matchStats.totalDamageDealt += event.amount;
    }
}

void ArenaCoordinator::HandleHealingDone(const CombatEvent& event)
{
    if (IsTeammate(event.source))
    {
        _matchStats.totalHealingDone += event.amount;
    }
}

void ArenaCoordinator::HandleSpellCastStart(const CombatEvent& /*event*/)
{
    // Could be used for interrupt coordination
}

void ArenaCoordinator::HandleSpellCastSuccess(const CombatEvent& event)
{
    // Track cooldown usage
    TrackCooldownUsage(event.source, event.spellId);

    // Check for trinket usage
    if (IsTrinketSpell(event.spellId))
    {
        TrackTrinketUsage(event.source);
    }

    // Check for defensive usage
    if (IsDefensiveCooldown(event.spellId))
    {
        TrackDefensiveUsage(event.source, event.spellId);
    }
}

void ArenaCoordinator::HandleSpellInterrupted(const CombatEvent& /*event*/)
{
    // Could track interrupt efficiency
}

void ArenaCoordinator::HandleAuraApplied(const CombatEvent& event)
{
    // Track CC application
    if (IsEnemy(event.target))
    {
        _matchStats.totalCCApplied++;

        ArenaEnemy* enemy = GetEnemyMutable(event.target);
        if (enemy)
        {
            // Check if this is a CC aura
            // Simplified - would need aura type checking
        }
    }
    else if (IsTeammate(event.target))
    {
        _matchStats.totalCCReceived++;
    }

    // Track defensive auras
    if (IsDefensiveCooldown(event.spellId))
    {
        if (IsEnemy(event.target))
        {
            ArenaEnemy* enemy = GetEnemyMutable(event.target);
            if (enemy)
            {
                enemy->isInDefensiveCooldown = true;
                enemy->defensiveEndTime = GameTime::GetGameTimeMS() + GetCooldownDuration(event.spellId);
            }
        }
    }
}

void ArenaCoordinator::HandleAuraRemoved(const CombatEvent& event)
{
    // Track CC expiration
    if (_ccChainManager)
    {
        _ccChainManager->OnCCExpired(event.target);
    }

    // Track defensive expiration
    if (IsDefensiveCooldown(event.spellId))
    {
        if (_defensiveCoordinator)
        {
            _defensiveCoordinator->OnDefensiveExpired(event.target, event.spellId);
        }

        if (IsEnemy(event.target))
        {
            ArenaEnemy* enemy = GetEnemyMutable(event.target);
            if (enemy)
            {
                enemy->isInDefensiveCooldown = false;
            }
        }
    }
}

void ArenaCoordinator::HandleUnitDied(const CombatEvent& event)
{
    if (IsEnemy(event.target))
    {
        _matchStats.killsScored++;

        if (_matchStats.firstBlood.IsEmpty())
        {
            _matchStats.firstBlood = event.target;
        }

        // Mark enemy as dead
        ArenaEnemy* enemy = GetEnemyMutable(event.target);
        if (enemy)
        {
            enemy->healthPercent = 0;
        }

        // Check if burst was successful
        if (_burstCoordinator && _burstCoordinator->IsBurstActive())
        {
            const BurstWindow& burst = _burstCoordinator->GetCurrentBurst();
            if (burst.target == event.target)
            {
                _burstCoordinator->OnTargetDied(event.target);
            }
        }
    }
    else if (IsTeammate(event.target))
    {
        _matchStats.deathsSuffered++;

        ArenaTeammate* teammate = GetTeammateMutable(event.target);
        if (teammate)
        {
            teammate->healthPercent = 0;
        }
    }
}

// ============================================================================
// TRACKING UPDATES
// ============================================================================

void ArenaCoordinator::UpdateEnemyTracking(uint32 /*diff*/)
{
    uint32 now = GameTime::GetGameTimeMS();

    for (auto& enemy : _enemies)
    {
        Player* player = ObjectAccessor::FindPlayer(enemy.guid);
        if (!player)
            continue;

        // Update health/mana
        enemy.healthPercent = player->GetHealthPct();
        enemy.manaPercent = player->GetPowerPct(POWER_MANA);

        // Update position
        enemy.lastKnownX = player->GetPositionX();
        enemy.lastKnownY = player->GetPositionY();
        enemy.lastKnownZ = player->GetPositionZ();
        enemy.lastSeenTime = now;

        // Update defensive status
        if (enemy.isInDefensiveCooldown && now >= enemy.defensiveEndTime)
        {
            enemy.isInDefensiveCooldown = false;
        }

        // Update CC status
        if (enemy.isInCC && now >= enemy.ccEndTime)
        {
            enemy.isInCC = false;
        }

        // Update trinket status
        if (!enemy.trinketAvailable && now >= enemy.trinketCooldown)
        {
            enemy.trinketAvailable = true;
        }

        // Update priority
        enemy.currentPriority = CalculateTargetPriority(enemy);
    }
}

void ArenaCoordinator::UpdateTeammateTracking(uint32 /*diff*/)
{
    for (auto& teammate : _teammates)
    {
        Player* player = ObjectAccessor::FindPlayer(teammate.guid);
        if (!player)
            continue;

        // Update health/mana
        teammate.healthPercent = player->GetHealthPct();
        teammate.manaPercent = player->GetPowerPct(POWER_MANA);

        // Update position
        teammate.x = player->GetPositionX();
        teammate.y = player->GetPositionY();
        teammate.z = player->GetPositionZ();

        // Update defensive state
        if (_defensiveCoordinator)
        {
            teammate.defensiveState = _defensiveCoordinator->GetTeammateState(teammate.guid);
        }

        // Update needs peel status
        teammate.needsPeel = (teammate.defensiveState >= DefensiveState::IN_DANGER);
    }
}

void ArenaCoordinator::TrackCooldownUsage(ObjectGuid caster, uint32 spellId)
{
    // Update enemy cooldown tracking
    ArenaEnemy* enemy = GetEnemyMutable(caster);
    if (enemy)
    {
        uint32 cooldownDuration = GetCooldownDuration(spellId);
        if (cooldownDuration > 0)
        {
            enemy->majorCooldowns[spellId] = GameTime::GetGameTimeMS() + cooldownDuration;
        }
    }

    // Notify burst coordinator
    if (_burstCoordinator)
    {
        _burstCoordinator->OnCooldownUsed(caster, spellId);
    }
}

void ArenaCoordinator::TrackTrinketUsage(ObjectGuid player)
{
    if (IsEnemy(player))
    {
        ArenaEnemy* enemy = GetEnemyMutable(player);
        if (enemy)
        {
            enemy->trinketAvailable = false;
            enemy->trinketCooldown = GameTime::GetGameTimeMS() + 120000;  // 2 min CD
            _matchStats.trinketsForcedOnEnemies++;
        }

        // Notify burst coordinator - great time to burst!
        if (_burstCoordinator && _burstCoordinator->IsBurstActive())
        {
            const BurstWindow& burst = _burstCoordinator->GetCurrentBurst();
            if (burst.target == player)
            {
                _burstCoordinator->OnTargetUsedTrinket(player);
            }
        }
    }
    else if (IsTeammate(player))
    {
        _matchStats.trinketsUsedByTeam++;

        if (_defensiveCoordinator)
        {
            _defensiveCoordinator->OnTrinketUsed(player);
        }
    }
}

void ArenaCoordinator::TrackDefensiveUsage(ObjectGuid player, uint32 spellId)
{
    if (IsEnemy(player))
    {
        ArenaEnemy* enemy = GetEnemyMutable(player);
        if (enemy)
        {
            enemy->isInDefensiveCooldown = true;
            enemy->defensiveEndTime = GameTime::GetGameTimeMS() + GetCooldownDuration(spellId);
        }

        // Notify burst coordinator
        if (_burstCoordinator && _burstCoordinator->IsBurstActive())
        {
            const BurstWindow& burst = _burstCoordinator->GetCurrentBurst();
            if (burst.target == player)
            {
                _burstCoordinator->OnTargetUsedDefensive(player);
            }
        }
    }
    else if (IsTeammate(player))
    {
        if (_defensiveCoordinator)
        {
            _defensiveCoordinator->OnDefensiveUsed(player, spellId);
        }
    }
}

void ArenaCoordinator::UpdateCooldownTimers(uint32 /*diff*/)
{
    uint32 now = GameTime::GetGameTimeMS();

    for (auto& enemy : _enemies)
    {
        // Update major cooldowns
        for (auto it = enemy.majorCooldowns.begin(); it != enemy.majorCooldowns.end(); )
        {
            if (now >= it->second)
            {
                it = enemy.majorCooldowns.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void ArenaCoordinator::InitializeEnemyTracking()
{
    _enemies.clear();

    // In a real implementation, we'd get enemy players from the battleground
    // For now, this is a placeholder
    TC_LOG_DEBUG("playerbot", "ArenaCoordinator::InitializeEnemyTracking - Initialized enemy tracking");
}

void ArenaCoordinator::InitializeTeammateTracking()
{
    _teammates.clear();

    for (Player* player : _team)
    {
        if (!player)
            continue;

        ArenaTeammate teammate;
        teammate.guid = player->GetGUID();
        teammate.classId = player->GetClass();
        teammate.specId = 0;  // Would need spec detection
        teammate.role = DeterminePlayerRole(player);
        teammate.healthPercent = player->GetHealthPct();
        teammate.manaPercent = player->GetPowerPct(POWER_MANA);

        _teammates.push_back(teammate);
    }

    TC_LOG_DEBUG("playerbot", "ArenaCoordinator::InitializeTeammateTracking - Tracking %zu teammates",
                 _teammates.size());
}

void ArenaCoordinator::DetectArenaType()
{
    // Determine arena type based on team size
    uint8 teamSize = static_cast<uint8>(_team.size());

    if (teamSize <= 2)
        _type = ArenaType::ARENA_2V2;
    else if (teamSize <= 3)
        _type = ArenaType::ARENA_3V3;
    else
        _type = ArenaType::ARENA_5V5;

    _bracket = ArenaBracket::RATED;  // Assume rated
}

ArenaRole ArenaCoordinator::DeterminePlayerRole(Player* player) const
{
    if (!player)
        return ArenaRole::UNKNOWN;

    uint8 classId = player->GetClass();

    // Simplified role detection based on class
    // Real implementation would check spec
    switch (classId)
    {
        case CLASS_PRIEST:
        case CLASS_SHAMAN:
        case CLASS_DRUID:
        case CLASS_PALADIN:
        case CLASS_MONK:
        case CLASS_EVOKER:
            // Could be healer - would need spec check
            return ArenaRole::HYBRID;

        case CLASS_WARRIOR:
        case CLASS_DEATH_KNIGHT:
        case CLASS_ROGUE:
        case CLASS_DEMON_HUNTER:
            return ArenaRole::MELEE_DPS;

        case CLASS_MAGE:
        case CLASS_WARLOCK:
        case CLASS_HUNTER:
            return ArenaRole::RANGED_DPS;

        default:
            return ArenaRole::UNKNOWN;
    }
}

// ============================================================================
// UTILITY
// ============================================================================

bool ArenaCoordinator::IsEnemy(ObjectGuid guid) const
{
    for (const auto& enemy : _enemies)
    {
        if (enemy.guid == guid)
            return true;
    }
    return false;
}

bool ArenaCoordinator::IsTeammate(ObjectGuid guid) const
{
    for (const auto& teammate : _teammates)
    {
        if (teammate.guid == guid)
            return true;
    }
    return false;
}

Player* ArenaCoordinator::GetPlayer(ObjectGuid guid) const
{
    return ObjectAccessor::FindPlayer(guid);
}

TargetPriority ArenaCoordinator::CalculateTargetPriority(const ArenaEnemy& enemy) const
{
    // In CC = ignore
    if (enemy.isInCC)
        return TargetPriority::IGNORE;

    // Kill target = highest
    if (_killTargetManager && _killTargetManager->GetKillTarget() == enemy.guid)
        return TargetPriority::KILL_TARGET;

    // Low health = high
    if (enemy.healthPercent < 30.0f)
        return TargetPriority::HIGH;

    // Healer = high
    if (enemy.role == ArenaRole::HEALER)
        return TargetPriority::HIGH;

    // Trinket down = high
    if (!enemy.trinketAvailable)
        return TargetPriority::HIGH;

    return TargetPriority::NORMAL;
}

// ============================================================================
// COOLDOWN DATABASE
// ============================================================================

bool ArenaCoordinator::IsTrinketSpell(uint32 spellId)
{
    return ::std::find(TRINKET_SPELLS.begin(), TRINKET_SPELLS.end(), spellId) != TRINKET_SPELLS.end();
}

bool ArenaCoordinator::IsDefensiveCooldown(uint32 spellId)
{
    return ::std::find(DEFENSIVE_COOLDOWNS.begin(), DEFENSIVE_COOLDOWNS.end(), spellId) != DEFENSIVE_COOLDOWNS.end();
}

bool ArenaCoordinator::IsMajorOffensiveCooldown(uint32 spellId)
{
    return ::std::find(OFFENSIVE_COOLDOWNS.begin(), OFFENSIVE_COOLDOWNS.end(), spellId) != OFFENSIVE_COOLDOWNS.end();
}

uint32 ArenaCoordinator::GetCooldownDuration(uint32 spellId)
{
    // Simplified - return default cooldowns
    // Real implementation would query spell data

    if (IsTrinketSpell(spellId))
        return 120000;  // 2 minutes

    if (IsDefensiveCooldown(spellId))
        return 180000;  // 3 minutes default

    if (IsMajorOffensiveCooldown(spellId))
        return 180000;  // 3 minutes default

    return 0;
}

} // namespace Playerbot
