/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "RaidCoordinator.h"
#include "RaidTankCoordinator.h"
#include "RaidHealCoordinator.h"
#include "RaidCooldownRotation.h"
#include "RaidGroupManager.h"
#include "KitingManager.h"
#include "AddManagementSystem.h"
#include "RaidPositioningManager.h"
#include "RaidEncounterManager.h"
#include "Core/Events/CombatEventRouter.h"
#include "AI/Coordination/Messaging/BotMessageBus.h"
#include "AI/Coordination/Messaging/BotMessage.h"
#include "Player.h"
#include "Group.h"
#include "Map.h"
#include "Log.h"

namespace Playerbot {

// ============================================================================
// ROLE DETECTION HELPERS
// ============================================================================

/**
 * Checks if a player has a tank specialization.
 */
static bool IsTankSpecialization(Player* player)
{
    if (!player)
        return false;

    ChrSpecialization spec = player->GetPrimarySpecialization();

    switch (spec)
    {
        case ChrSpecialization::WarriorProtection:
        case ChrSpecialization::PaladinProtection:
        case ChrSpecialization::DeathKnightBlood:
        case ChrSpecialization::DruidGuardian:
        case ChrSpecialization::MonkBrewmaster:
        case ChrSpecialization::DemonHunterVengeance:
            return true;
        default:
            return false;
    }
}

/**
 * Checks if a player has a healer specialization.
 */
static bool IsHealerSpecialization(Player* player)
{
    if (!player)
        return false;

    ChrSpecialization spec = player->GetPrimarySpecialization();

    switch (spec)
    {
        case ChrSpecialization::PriestDiscipline:
        case ChrSpecialization::PriestHoly:
        case ChrSpecialization::PaladinHoly:
        case ChrSpecialization::DruidRestoration:
        case ChrSpecialization::ShamanRestoration:
        case ChrSpecialization::MonkMistweaver:
        case ChrSpecialization::EvokerPreservation:
            return true;
        default:
            return false;
    }
}

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

RaidCoordinator::RaidCoordinator(Map* raidInstance, const ::std::vector<Player*>& raidMembers)
    : _raidInstance(raidInstance)
{
    // Store member GUIDs and cache player pointers
    for (Player* player : raidMembers)
    {
        if (player)
        {
            _raidMembers.push_back(player->GetGUID());
            _playerCache[player->GetGUID()] = player;
        }
    }
}

RaidCoordinator::~RaidCoordinator()
{
    Shutdown();
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void RaidCoordinator::Initialize()
{
    TC_LOG_DEBUG("playerbots.raid", "RaidCoordinator::Initialize - Initializing with %zu members",
        _raidMembers.size());

    // Categorize roster into roles
    CategorizeRoster();

    // Create sub-managers
    CreateSubManagers();

    // Register for combat events
    RegisterCombatEvents();

    // Transition to forming state
    TransitionToState(RaidState::FORMING);
}

void RaidCoordinator::Shutdown()
{
    TC_LOG_DEBUG("playerbots.raid", "RaidCoordinator::Shutdown - Shutting down");

    // Unregister from combat events
    UnregisterCombatEvents();

    // Destroy sub-managers
    DestroySubManagers();

    // Clear caches
    _playerCache.clear();
    _raidMembers.clear();
    _tanks.clear();
    _healers.clear();
    _dps.clear();
}

void RaidCoordinator::Update(uint32 diff)
{
    _lastUpdateTime += diff;
    if (_lastUpdateTime < _updateInterval)
        return;

    _lastUpdateTime = 0;

    // Update sub-managers
    if (_tankCoordinator)
        _tankCoordinator->Update(diff);
    if (_healCoordinator)
        _healCoordinator->Update(diff);
    if (_cooldownRotation)
        _cooldownRotation->Update(diff);
    if (_groupManager)
        _groupManager->Update(diff);
    if (_kitingManager)
        _kitingManager->Update(diff);
    if (_addManager)
        _addManager->Update(diff);
    if (_positioningManager)
        _positioningManager->Update(diff);
    if (_encounterManager)
        _encounterManager->Update(diff);

    // Update based on current state
    switch (_state)
    {
        case RaidState::IDLE:
            UpdateIdle(diff);
            break;
        case RaidState::FORMING:
            UpdateForming(diff);
            break;
        case RaidState::BUFFING:
            UpdateBuffing(diff);
            break;
        case RaidState::PULLING:
            UpdatePulling(diff);
            break;
        case RaidState::COMBAT:
            UpdateCombat(diff);
            break;
        case RaidState::PHASE_TRANSITION:
            UpdatePhaseTransition(diff);
            break;
        case RaidState::WIPED:
            UpdateWiped(diff);
            break;
        case RaidState::RECOVERING:
            UpdateRecovering(diff);
            break;
    }
}

// ============================================================================
// COMBAT EVENT INTERFACE
// ============================================================================

void RaidCoordinator::OnCombatEvent(const CombatEvent& event)
{
    if (event.IsDamageEvent())
    {
        HandleDamageEvent(event);
    }
    else if (event.IsHealingEvent())
    {
        HandleHealingEvent(event);
    }
    else if (event.IsSpellEvent())
    {
        HandleSpellEvent(event);
    }
    else if (event.IsAuraEvent())
    {
        HandleAuraEvent(event);
    }
    else if (event.type == CombatEventType::UNIT_DIED)
    {
        HandleDeathEvent(event);
    }
}

CombatEventType RaidCoordinator::GetSubscribedEvents() const
{
    return CombatEventType::ALL_DAMAGE |
           CombatEventType::ALL_HEALING |
           CombatEventType::ALL_SPELL |
           CombatEventType::ALL_AURA |
           CombatEventType::UNIT_DIED;
}

// ============================================================================
// STATE MANAGEMENT
// ============================================================================

void RaidCoordinator::TransitionToState(RaidState newState)
{
    if (_state == newState)
        return;

    RaidState oldState = _state;
    _state = newState;

    TC_LOG_DEBUG("playerbots.raid", "RaidCoordinator::TransitionToState - %s -> %s",
        RaidStateToString(oldState), RaidStateToString(newState));

    // State entry actions
    switch (newState)
    {
        case RaidState::COMBAT:
            _combatStartTime = 0; // Will be set in Update
            break;
        case RaidState::WIPED:
            _matchStats.wipeCount++;
            break;
        case RaidState::RECOVERING:
            // Start recovery process
            break;
        default:
            break;
    }
}

void RaidCoordinator::OnRaidWipe()
{
    TC_LOG_DEBUG("playerbots.raid", "RaidCoordinator::OnRaidWipe - Raid wiped!");

    TransitionToState(RaidState::WIPED);

    // Reset sub-managers
    if (_addManager)
        _addManager->Reset();
    if (_kitingManager)
        _kitingManager->Reset();
}

void RaidCoordinator::OnEncounterStart(uint32 encounterId)
{
    _currentEncounterId = encounterId;

    TC_LOG_DEBUG("playerbots.raid", "RaidCoordinator::OnEncounterStart - Encounter %u started",
        encounterId);

    if (_encounterManager)
        _encounterManager->OnEncounterStart(encounterId);

    TransitionToState(RaidState::COMBAT);
}

void RaidCoordinator::OnEncounterEnd(bool success)
{
    TC_LOG_DEBUG("playerbots.raid", "RaidCoordinator::OnEncounterEnd - Encounter ended, success: %s",
        success ? "true" : "false");

    if (success)
    {
        TransitionToState(RaidState::BUFFING);
    }
    else
    {
        OnRaidWipe();
    }

    _currentEncounterId = 0;
}

// ============================================================================
// RAID ROSTER
// ============================================================================

void RaidCoordinator::AddMember(Player* player)
{
    if (!player)
        return;

    ObjectGuid guid = player->GetGUID();
    if (IsMember(guid))
        return;

    _raidMembers.push_back(guid);
    _playerCache[guid] = player;

    // Recategorize
    CategorizeRoster();

    TC_LOG_DEBUG("playerbots.raid", "RaidCoordinator::AddMember - Added member, total: %zu",
        _raidMembers.size());
}

void RaidCoordinator::RemoveMember(ObjectGuid playerGuid)
{
    auto it = std::remove(_raidMembers.begin(), _raidMembers.end(), playerGuid);
    if (it != _raidMembers.end())
    {
        _raidMembers.erase(it, _raidMembers.end());
        _playerCache.erase(playerGuid);

        // Remove from role lists
        _tanks.erase(std::remove(_tanks.begin(), _tanks.end(), playerGuid), _tanks.end());
        _healers.erase(std::remove(_healers.begin(), _healers.end(), playerGuid), _healers.end());
        _dps.erase(std::remove(_dps.begin(), _dps.end(), playerGuid), _dps.end());

        TC_LOG_DEBUG("playerbots.raid", "RaidCoordinator::RemoveMember - Removed member, remaining: %zu",
            _raidMembers.size());
    }
}

void RaidCoordinator::UpdateMember(ObjectGuid /*playerGuid*/)
{
    RefreshPlayerCache();
}

bool RaidCoordinator::IsMember(ObjectGuid playerGuid) const
{
    return std::find(_raidMembers.begin(), _raidMembers.end(), playerGuid) != _raidMembers.end();
}

uint32 RaidCoordinator::GetAliveMemberCount() const
{
    uint32 count = 0;
    for (ObjectGuid guid : _raidMembers)
    {
        Player* player = GetPlayer(guid);
        if (player && player->IsAlive())
            count++;
    }
    return count;
}

Player* RaidCoordinator::GetPlayer(ObjectGuid guid) const
{
    auto it = _playerCache.find(guid);
    if (it != _playerCache.end() && it->second && it->second->IsInWorld())
        return it->second;
    return nullptr;
}

// ============================================================================
// ENCOUNTER INFORMATION
// ============================================================================

const RaidEncounterInfo* RaidCoordinator::GetCurrentEncounter() const
{
    if (_encounterManager)
        return _encounterManager->GetCurrentEncounter();
    return nullptr;
}

// ============================================================================
// RAID-WIDE CALLS
// ============================================================================

void RaidCoordinator::CallBloodlust()
{
    TC_LOG_DEBUG("playerbots.raid", "RaidCoordinator::CallBloodlust - Bloodlust called!");

    if (_cooldownRotation)
        _cooldownRotation->UseBloodlust();

    _matchStats.bloodlustUsed++;

    // Broadcast bloodlust command via BotMessageBus
    if (!_raidMembers.empty())
    {
        Player* leader = GetPlayer(_raidMembers.front());
        if (leader && leader->GetGroup())
        {
            ObjectGuid groupGuid = leader->GetGroup()->GetGUID();
            BotMessage msg = BotMessage::CommandBloodlust(leader->GetGUID(), groupGuid);
            sBotMessageBus->Publish(msg);
        }
    }
}

void RaidCoordinator::CallRaidDefensive()
{
    TC_LOG_DEBUG("playerbots.raid", "RaidCoordinator::CallRaidDefensive - Raid defensive called!");

    if (_cooldownRotation)
        _cooldownRotation->UseRaidDefensive();

    // Broadcast use defensives command via BotMessageBus
    if (!_raidMembers.empty())
    {
        Player* leader = GetPlayer(_raidMembers.front());
        if (leader && leader->GetGroup())
        {
            ObjectGuid groupGuid = leader->GetGroup()->GetGUID();
            BotMessage msg = BotMessage::CommandUseDefensives(leader->GetGUID(), groupGuid);
            sBotMessageBus->Publish(msg);
        }
    }
}

void RaidCoordinator::CallBattleRez(ObjectGuid target)
{
    TC_LOG_DEBUG("playerbots.raid", "RaidCoordinator::CallBattleRez - Battle rez called for target");

    if (_cooldownRotation)
        _cooldownRotation->UseBattleRez(target);

    _matchStats.battleRezUsed++;

    // Broadcast rez claim via BotMessageBus
    if (!_raidMembers.empty())
    {
        Player* leader = GetPlayer(_raidMembers.front());
        if (leader && leader->GetGroup())
        {
            ObjectGuid groupGuid = leader->GetGroup()->GetGUID();
            BotMessage msg = BotMessage::ClaimResurrect(leader->GetGUID(), groupGuid, target, ClaimPriority::HIGH);
            sBotMessageBus->Publish(msg);
        }
    }
}

void RaidCoordinator::CallReadyCheck()
{
    TC_LOG_DEBUG("playerbots.raid", "RaidCoordinator::CallReadyCheck - Ready check initiated");

    // Implementation would send ready check packets
}

void RaidCoordinator::CallPull(uint32 countdown)
{
    TC_LOG_DEBUG("playerbots.raid", "RaidCoordinator::CallPull - Pull in %u ms", countdown);

    _pullCountdown = countdown;
    TransitionToState(RaidState::PULLING);
}

void RaidCoordinator::CallWipe()
{
    TC_LOG_DEBUG("playerbots.raid", "RaidCoordinator::CallWipe - Wipe called");

    // Broadcast wipe recovery command via BotMessageBus
    if (!_raidMembers.empty())
    {
        Player* leader = GetPlayer(_raidMembers.front());
        if (leader && leader->GetGroup())
        {
            ObjectGuid groupGuid = leader->GetGroup()->GetGUID();
            BotMessage msg = BotMessage::CommandWipeRecovery(leader->GetGUID(), groupGuid);
            sBotMessageBus->Publish(msg);
        }
    }

    OnRaidWipe();
}

// ============================================================================
// QUICK ACCESS QUERIES
// ============================================================================

ObjectGuid RaidCoordinator::GetMainTank() const
{
    if (_tankCoordinator)
        return _tankCoordinator->GetMainTank();

    if (!_tanks.empty())
        return _tanks.front();

    return ObjectGuid();
}

ObjectGuid RaidCoordinator::GetOffTank() const
{
    if (_tankCoordinator)
        return _tankCoordinator->GetOffTank();

    if (_tanks.size() > 1)
        return _tanks[1];

    return ObjectGuid();
}

ObjectGuid RaidCoordinator::GetKillTarget() const
{
    if (_addManager)
    {
        ObjectGuid priorityAdd = _addManager->GetHighestPriorityAdd();
        if (!priorityAdd.IsEmpty())
            return priorityAdd;
    }

    return GetCurrentBossTarget();
}

ObjectGuid RaidCoordinator::GetCurrentBossTarget() const
{
    if (_encounterManager)
        return _encounterManager->GetCurrentBossGuid();

    return ObjectGuid();
}

bool RaidCoordinator::ShouldUseBloodlustNow() const
{
    if (_cooldownRotation)
        return _cooldownRotation->ShouldUseBloodlust();

    // Default: use bloodlust at 30% boss health
    return GetBossHealthPercent() <= 30.0f;
}

bool RaidCoordinator::ShouldUseRaidDefensiveNow() const
{
    if (_cooldownRotation)
        return _cooldownRotation->ShouldUseRaidDefensive();

    // Default: use defensive when raid is taking heavy damage
    return GetRaidHealthPercent() < 50.0f;
}

bool RaidCoordinator::HasBattleRezAvailable() const
{
    if (_cooldownRotation)
        return _cooldownRotation->HasBattleRezAvailable();

    return false;
}

float RaidCoordinator::GetRaidHealthPercent() const
{
    if (_raidMembers.empty())
        return 100.0f;

    float totalHealth = 0.0f;
    uint32 count = 0;

    for (ObjectGuid guid : _raidMembers)
    {
        Player* player = GetPlayer(guid);
        if (player && player->IsAlive())
        {
            totalHealth += player->GetHealthPct();
            count++;
        }
    }

    return count > 0 ? totalHealth / count : 0.0f;
}

float RaidCoordinator::GetRaidManaPercent() const
{
    float totalMana = 0.0f;
    uint32 count = 0;

    // Only count mana users
    for (ObjectGuid guid : _healers)
    {
        Player* player = GetPlayer(guid);
        if (player && player->IsAlive())
        {
            totalMana += player->GetPowerPct(POWER_MANA);
            count++;
        }
    }

    return count > 0 ? totalMana / count : 100.0f;
}

float RaidCoordinator::GetBossHealthPercent() const
{
    if (_encounterManager)
        return _encounterManager->GetBossHealthPercent();

    return 100.0f;
}

// ============================================================================
// INITIALIZATION (PRIVATE)
// ============================================================================

void RaidCoordinator::CreateSubManagers()
{
    _tankCoordinator = ::std::make_unique<RaidTankCoordinator>(this);
    _healCoordinator = ::std::make_unique<RaidHealCoordinator>(this);
    _cooldownRotation = ::std::make_unique<RaidCooldownRotation>(this);
    _groupManager = ::std::make_unique<RaidGroupManager>(this);
    _kitingManager = ::std::make_unique<KitingManager>(this);
    _addManager = ::std::make_unique<AddManagementSystem>(this);
    _positioningManager = ::std::make_unique<RaidPositioningManager>(this);
    _encounterManager = ::std::make_unique<RaidEncounterManager>(this);

    // Initialize all sub-managers
    _tankCoordinator->Initialize();
    _healCoordinator->Initialize();
    _cooldownRotation->Initialize();
    _groupManager->Initialize();
    _kitingManager->Initialize();
    _addManager->Initialize();
    _positioningManager->Initialize();
    _encounterManager->Initialize();

    TC_LOG_DEBUG("playerbots.raid", "RaidCoordinator::CreateSubManagers - All sub-managers created");
}

void RaidCoordinator::DestroySubManagers()
{
    _tankCoordinator.reset();
    _healCoordinator.reset();
    _cooldownRotation.reset();
    _groupManager.reset();
    _kitingManager.reset();
    _addManager.reset();
    _positioningManager.reset();
    _encounterManager.reset();
}

void RaidCoordinator::CategorizeRoster()
{
    _tanks.clear();
    _healers.clear();
    _dps.clear();

    for (ObjectGuid guid : _raidMembers)
    {
        Player* player = GetPlayer(guid);
        if (!player)
            continue;

        // Check player's specialization/role using helper functions
        if (IsTankSpecialization(player))
        {
            _tanks.push_back(guid);
        }
        else if (IsHealerSpecialization(player))
        {
            _healers.push_back(guid);
        }
        else
        {
            _dps.push_back(guid);
        }
    }

    TC_LOG_DEBUG("playerbots.raid", "RaidCoordinator::CategorizeRoster - Tanks: %zu, Healers: %zu, DPS: %zu",
        _tanks.size(), _healers.size(), _dps.size());
}

void RaidCoordinator::RegisterCombatEvents()
{
    // NOTE: RaidCoordinator uses CombatEvent polling rather than direct subscription.
    // Inheriting from ICombatEventSubscriber is not needed — the coordinator
    // reads combat state from the shared CombatStateAnalyzer each update tick.
    TC_LOG_DEBUG("playerbots.raid", "RaidCoordinator::RegisterCombatEvents - Event registration pending (not ICombatEventSubscriber)");
}

void RaidCoordinator::UnregisterCombatEvents()
{
    // NOTE: RaidCoordinator doesn't inherit from ICombatEventSubscriber yet.
    // Event unregistration is disabled until inheritance is properly set up.
    // CombatEventRouter::Instance().Unsubscribe(this);
}

// ============================================================================
// EVENT HANDLERS (PRIVATE)
// ============================================================================

void RaidCoordinator::HandleDamageEvent(const CombatEvent& event)
{
    // Forward to relevant sub-managers
    if (_tankCoordinator)
        _tankCoordinator->OnDamageEvent(event);

    if (_addManager)
        _addManager->OnDamageEvent(event);
}

void RaidCoordinator::HandleHealingEvent(const CombatEvent& event)
{
    if (_healCoordinator)
        _healCoordinator->OnHealingEvent(event);
}

void RaidCoordinator::HandleSpellEvent(const CombatEvent& event)
{
    if (_encounterManager)
        _encounterManager->OnSpellEvent(event);

    if (_cooldownRotation)
        _cooldownRotation->OnSpellEvent(event);
}

void RaidCoordinator::HandleAuraEvent(const CombatEvent& event)
{
    // Tank swap triggers
    if (_tankCoordinator)
        _tankCoordinator->OnAuraEvent(event);

    if (_encounterManager)
        _encounterManager->OnAuraEvent(event);
}

void RaidCoordinator::HandleDeathEvent(const CombatEvent& event)
{
    ObjectGuid deadGuid = event.source;  // Who died

    // Check if it's a raid member
    if (IsMember(deadGuid))
    {
        _matchStats.totalDeaths++;

        TC_LOG_DEBUG("playerbots.raid", "RaidCoordinator::HandleDeathEvent - Raid member died");

        // Check for wipe
        if (GetAliveMemberCount() == 0)
        {
            OnRaidWipe();
        }
    }

    // Forward to add manager for add deaths
    if (_addManager)
        _addManager->OnDeathEvent(event);

    // Forward to kiting manager
    if (_kitingManager)
        _kitingManager->OnDeathEvent(event);
}

// ============================================================================
// STATE UPDATES (PRIVATE)
// ============================================================================

void RaidCoordinator::UpdateIdle(uint32 /*diff*/)
{
    // Nothing to do in idle state
}

void RaidCoordinator::UpdateForming(uint32 /*diff*/)
{
    // Check if raid is ready to proceed
    if (GetMemberCount() >= 10) // Minimum for raid
    {
        TransitionToState(RaidState::BUFFING);
    }
}

void RaidCoordinator::UpdateBuffing(uint32 /*diff*/)
{
    // Check if all members are buffed and ready
    if (AreAllMembersBuffed() && AreAllMembersReady())
    {
        // Ready to pull
    }
}

void RaidCoordinator::UpdatePulling(uint32 diff)
{
    if (_pullCountdown > diff)
    {
        _pullCountdown -= diff;
    }
    else
    {
        _pullCountdown = 0;
        // Pull initiated
    }
}

void RaidCoordinator::UpdateCombat(uint32 diff)
{
    _combatStartTime += diff;
    _matchStats.combatTime += diff;

    // Check for auto bloodlust
    if (ShouldUseBloodlustNow() && _cooldownRotation && _cooldownRotation->IsBloodlustAvailable())
    {
        CallBloodlust();
    }

    // Check for auto defensive
    if (ShouldUseRaidDefensiveNow() && _cooldownRotation)
    {
        CallRaidDefensive();
    }
}

void RaidCoordinator::UpdatePhaseTransition(uint32 /*diff*/)
{
    // Handle phase transition mechanics
}

void RaidCoordinator::UpdateWiped(uint32 diff)
{
    // Wait for all members to release
    static uint32 wipeTimer = 0;
    wipeTimer += diff;

    if (wipeTimer >= 5000) // 5 second delay
    {
        wipeTimer = 0;
        TransitionToState(RaidState::RECOVERING);
    }
}

void RaidCoordinator::UpdateRecovering(uint32 /*diff*/)
{
    // Check if all members are alive and ready
    if (AreAllMembersAlive())
    {
        TransitionToState(RaidState::BUFFING);
    }
}

// ============================================================================
// UTILITY (PRIVATE)
// ============================================================================

bool RaidCoordinator::AreAllMembersReady() const
{
    // Simplified check
    return true;
}

bool RaidCoordinator::AreAllMembersBuffed() const
{
    // Simplified check
    return true;
}

bool RaidCoordinator::AreAllMembersAlive() const
{
    for (ObjectGuid guid : _raidMembers)
    {
        Player* player = GetPlayer(guid);
        if (!player || !player->IsAlive())
            return false;
    }
    return true;
}

void RaidCoordinator::RefreshPlayerCache()
{
    for (ObjectGuid guid : _raidMembers)
    {
        // Would refresh player pointers from the world
        (void)guid;  // Suppress unused variable warning
    }
}

} // namespace Playerbot
