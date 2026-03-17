/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "DungeonCoordinator.h"
#include "TrashPullManager.h"
#include "BossEncounterManager.h"
#include "WipeRecoveryManager.h"
#include "MythicPlusManager.h"
#include "Core/Events/CombatEventRouter.h"
#include "Core/Events/CombatEvent.h"
#include "Group.h"
#include "Player.h"
#include "Map.h"
#include "Creature.h"
#include "ObjectAccessor.h"
#include "GameTime.h"
#include "Log.h"
#include "LFG.h"
#include "AI/Coordination/Messaging/BotMessageBus.h"
#include "AI/Coordination/Messaging/BotMessage.h"
#include "AI/Coordination/ContentContextManager.h"

namespace Playerbot {

DungeonCoordinator::DungeonCoordinator(Group* group)
    : _group(group)
{
    // Create sub-managers
    _trashManager = ::std::make_unique<TrashPullManager>(this);
    _bossManager = ::std::make_unique<BossEncounterManager>(this);
    _wipeManager = ::std::make_unique<WipeRecoveryManager>(this);
    _mythicPlusManager = ::std::make_unique<MythicPlusManager>(this);
}

DungeonCoordinator::~DungeonCoordinator()
{
    Shutdown();
}

void DungeonCoordinator::Initialize()
{
    if (!_group)
    {
        TC_LOG_ERROR("playerbot", "DungeonCoordinator::Initialize - No group!");
        return;
    }

    // Subscribe to combat events
    if (!_subscribed.exchange(true))
    {
        CombatEventRouter::Instance().Subscribe(this);
    }

    // Initialize sub-managers
    _wipeManager->Initialize();

    // Detect roles in the group
    DetectRoles();

    // Try to detect dungeon from map
    Player* leader = GetGroupLeader();
    if (leader && leader->GetMap())
    {
        _dungeonMap = leader->GetMap();
        if (_dungeonMap->IsDungeon())
        {
            LoadDungeonData(_dungeonMap->GetId());
            TransitionTo(DungeonState::ENTERING);
        }
    }

    TC_LOG_DEBUG("playerbot", "DungeonCoordinator::Initialize - Initialized for group");
}

void DungeonCoordinator::Shutdown()
{
    // Unsubscribe from combat events
    if (_subscribed.exchange(false))
    {
        CombatEventRouter::Instance().Unsubscribe(this);
    }

    // Reset state
    _state = DungeonState::IDLE;
    _dungeonMap = nullptr;

    TC_LOG_DEBUG("playerbot", "DungeonCoordinator::Shutdown - Shutdown complete");
}

void DungeonCoordinator::Update(uint32 diff)
{
    // Throttle updates
    _updateTimer += diff;
    if (_updateTimer < UPDATE_INTERVAL_MS)
        return;
    _updateTimer = 0;

    // Skip if not in dungeon
    if (_state == DungeonState::IDLE)
        return;

    // Update state machine
    UpdateState(diff);

    // Update sub-managers based on state
    switch (_state)
    {
        case DungeonState::CLEARING_TRASH:
            _trashManager->Update(diff);
            break;

        case DungeonState::PRE_BOSS:
        case DungeonState::BOSS_COMBAT:
            _bossManager->Update(diff);
            break;

        case DungeonState::WIPED:
            _wipeManager->Update(diff);
            break;

        default:
            break;
    }

    // Update M+ manager if active
    if (_progress.isMythicPlus && _mythicPlusManager)
    {
        _mythicPlusManager->Update(diff);
    }
}

// ============================================================================
// STATE MANAGEMENT
// ============================================================================

void DungeonCoordinator::SetState(DungeonState newState)
{
    if (_state != newState)
    {
        TransitionTo(newState);
    }
}

bool DungeonCoordinator::IsInCombat() const
{
    return _state == DungeonState::CLEARING_TRASH || _state == DungeonState::BOSS_COMBAT;
}

void DungeonCoordinator::UpdateState(uint32 diff)
{
    _stateTimer += diff;

    switch (_state)
    {
        case DungeonState::ENTERING:
            // Wait for all members to zone in
            if (IsGroupReady())
            {
                TransitionTo(DungeonState::READY_CHECK);
            }
            break;

        case DungeonState::READY_CHECK:
            // Timeout or all ready
            if (_stateTimer > READY_CHECK_TIMEOUT_MS || IsGroupReady())
            {
                if (_trashManager->GetRemainingPackCount() > 0)
                    TransitionTo(DungeonState::CLEARING_TRASH);
                else if (_bossManager->GetAllBosses().size() > _progress.bossesKilled)
                    TransitionTo(DungeonState::PRE_BOSS);
            }
            break;

        case DungeonState::CLEARING_TRASH:
            // Check for boss proximity or all trash cleared
            if (!IsGroupInCombat())
            {
                if (_trashManager->GetRemainingPackCount() == 0)
                {
                    // Check if near boss
                    TransitionTo(DungeonState::PRE_BOSS);
                }
            }
            break;

        case DungeonState::PRE_BOSS:
            // Wait for group to be ready (health, mana, buffs)
            if (IsGroupReady())
            {
                // Boss will be engaged externally
            }
            break;

        case DungeonState::BOSS_COMBAT:
            // Combat handled by BossEncounterManager
            break;

        case DungeonState::POST_BOSS:
            // Loot and proceed
            if (_stateTimer > 5000)  // 5 second loot window
            {
                if (_progress.bossesKilled >= _progress.totalBosses)
                {
                    TransitionTo(DungeonState::COMPLETED);
                }
                else if (_trashManager->GetRemainingPackCount() > 0)
                {
                    TransitionTo(DungeonState::CLEARING_TRASH);
                }
                else
                {
                    TransitionTo(DungeonState::PRE_BOSS);
                }
            }
            break;

        case DungeonState::WIPED:
            // Recovery handled by WipeRecoveryManager
            if (_wipeManager->IsGroupReady())
            {
                TransitionTo(DungeonState::READY_CHECK);
            }
            break;

        case DungeonState::COMPLETED:
            // Nothing to do
            break;

        default:
            break;
    }
}

void DungeonCoordinator::TransitionTo(DungeonState newState)
{
    if (_state == newState)
        return;

    TC_LOG_DEBUG("playerbot", "DungeonCoordinator: State transition %s -> %s",
                 DungeonStateToString(_state), DungeonStateToString(newState));

    OnStateExit(_state);
    _state = newState;
    _stateTimer = 0;
    OnStateEnter(newState);
}

void DungeonCoordinator::OnStateEnter(DungeonState state)
{
    switch (state)
    {
        case DungeonState::ENTERING:
            TC_LOG_DEBUG("playerbot", "DungeonCoordinator: Entering dungeon");
            break;

        case DungeonState::READY_CHECK:
            TC_LOG_DEBUG("playerbot", "DungeonCoordinator: Ready check started");
            break;

        case DungeonState::CLEARING_TRASH:
            TC_LOG_DEBUG("playerbot", "DungeonCoordinator: Starting trash clear");
            break;

        case DungeonState::PRE_BOSS:
            TC_LOG_DEBUG("playerbot", "DungeonCoordinator: Preparing for boss");
            break;

        case DungeonState::BOSS_COMBAT:
            TC_LOG_DEBUG("playerbot", "DungeonCoordinator: Boss combat started");
            break;

        case DungeonState::WIPED:
            TC_LOG_DEBUG("playerbot", "DungeonCoordinator: Group wiped, starting recovery");
            _wipeManager->OnGroupWipe();
            break;

        case DungeonState::COMPLETED:
            TC_LOG_DEBUG("playerbot", "DungeonCoordinator: Dungeon completed!");
            break;

        default:
            break;
    }
}

void DungeonCoordinator::OnStateExit(DungeonState state)
{
    switch (state)
    {
        case DungeonState::BOSS_COMBAT:
            // Boss encounter ended (kill or wipe)
            break;

        case DungeonState::WIPED:
            // Recovery complete
            _wipeManager->Reset();
            break;

        default:
            break;
    }
}

// ============================================================================
// PROGRESS TRACKING
// ============================================================================

float DungeonCoordinator::GetCompletionPercent() const
{
    float bossProgress = _progress.totalBosses > 0 ?
        (static_cast<float>(_progress.bossesKilled) / _progress.totalBosses) : 0.0f;

    // For M+, also factor in enemy forces
    if (_progress.isMythicPlus)
    {
        float forcesProgress = _progress.enemyForcesPercent / 100.0f;
        return (bossProgress * 0.7f + forcesProgress * 0.3f) * 100.0f;
    }

    return bossProgress * 100.0f;
}

// ============================================================================
// TRASH MANAGEMENT
// ============================================================================

TrashPack* DungeonCoordinator::GetCurrentPullTarget() const
{
    if (!_trashManager)
        return nullptr;

    uint32 packId = _trashManager->GetNextPackToPull();
    if (packId > 0)
        return _trashManager->GetPack(packId);

    return nullptr;
}

void DungeonCoordinator::MarkPackForPull(uint32 packId)
{
    if (_trashManager)
    {
        _trashManager->QueuePack(packId);
    }
}

void DungeonCoordinator::AssignCCTargets(const TrashPack& pack)
{
    if (_trashManager)
    {
        _trashManager->AssignCC(pack);
    }
}

bool DungeonCoordinator::IsSafeToPull() const
{
    if (!_trashManager)
        return false;

    // Not safe if in combat
    if (IsGroupInCombat())
        return false;

    // Check group readiness
    return _trashManager->IsSafeToPull() && IsGroupReady();
}

void DungeonCoordinator::OnTrashPackCleared(uint32 packId)
{
    if (_trashManager)
    {
        _trashManager->OnPackCleared(packId);
        _progress.trashKilled++;

        // Update M+ forces if applicable
        if (_progress.isMythicPlus && _mythicPlusManager)
        {
            // Forces updated via OnEnemyKilled
        }
    }
}

::std::vector<TrashPack> DungeonCoordinator::GetAllTrashPacks() const
{
    ::std::vector<TrashPack> result;
    if (_trashManager)
    {
        for (const auto& [id, pack] : _trashManager->GetAllPacks())
        {
            result.push_back(pack);
        }
    }
    return result;
}

// ============================================================================
// BOSS MANAGEMENT
// ============================================================================

BossInfo* DungeonCoordinator::GetCurrentBoss() const
{
    return _bossManager ? _bossManager->GetCurrentBoss() : nullptr;
}

void DungeonCoordinator::OnBossEngaged(uint32 bossId)
{
    if (_bossManager)
    {
        _bossManager->OnBossEngaged(bossId);
        TransitionTo(DungeonState::BOSS_COMBAT);

        // Notify group via BotMessageBus that boss combat has started
        // Bots subscribed to the group will receive this and adjust behavior
        if (_group)
        {
            ObjectGuid groupGuid = _group->GetGUID();
            ObjectGuid leaderGuid = _group->GetLeaderGUID();

            // Check if boss strategy calls for bloodlust on pull
            if (_bossManager->ShouldUseBloodlust())
            {
                BotMessage msg = BotMessage::CommandBloodlust(leaderGuid, groupGuid);
                sBotMessageBus->Publish(msg);
            }
        }
    }
}

void DungeonCoordinator::OnBossDefeated(uint32 bossId)
{
    if (_bossManager)
    {
        _bossManager->OnBossDefeated(bossId);
        _progress.bossesKilled++;
        TransitionTo(DungeonState::POST_BOSS);
    }
}

void DungeonCoordinator::OnBossWipe(uint32 bossId)
{
    if (_bossManager)
    {
        _bossManager->OnBossWipe(bossId);
    }
    TransitionTo(DungeonState::WIPED);
}

uint8 DungeonCoordinator::GetBossPhase() const
{
    return _bossManager ? _bossManager->GetCurrentPhase() : 0;
}

::std::vector<BossInfo> DungeonCoordinator::GetAllBosses() const
{
    return _bossManager ? _bossManager->GetAllBosses() : ::std::vector<BossInfo>{};
}

// ============================================================================
// WIPE RECOVERY
// ============================================================================

void DungeonCoordinator::OnGroupWipe()
{
    TransitionTo(DungeonState::WIPED);
}

ObjectGuid DungeonCoordinator::GetNextRezTarget() const
{
    return _wipeManager ? _wipeManager->GetNextRezTarget() : ObjectGuid::Empty;
}

void DungeonCoordinator::OnPlayerRezzed(ObjectGuid playerGuid)
{
    if (_wipeManager)
    {
        _wipeManager->OnPlayerRezzed(playerGuid);
    }
}

bool DungeonCoordinator::IsGroupAlive() const
{
    return _wipeManager ? _wipeManager->AllPlayersAlive() : true;
}

// ============================================================================
// ROLE MANAGEMENT
// ============================================================================

void DungeonCoordinator::DetectRoles()
{
    _healers.clear();
    _dps.clear();
    _mainTank = ObjectGuid::Empty;
    _offTank = ObjectGuid::Empty;

    auto members = GetGroupMembers();
    for (Player* member : members)
    {
        if (!member)
            continue;

        // Check player role using LFG role from group
        uint8 roles = _group ? _group->GetLfgRoles(member->GetGUID()) : 0;

        if (roles & lfg::PLAYER_ROLE_TANK)
        {
            if (_mainTank.IsEmpty())
                _mainTank = member->GetGUID();
            else if (_offTank.IsEmpty())
                _offTank = member->GetGUID();
        }
        else if (roles & lfg::PLAYER_ROLE_HEALER)
        {
            _healers.push_back(member->GetGUID());
        }
        else
        {
            _dps.push_back(member->GetGUID());
        }
    }

    TC_LOG_DEBUG("playerbot", "DungeonCoordinator::DetectRoles - Tank: %s, Healers: %zu, DPS: %zu",
                 _mainTank.IsEmpty() ? "none" : "assigned",
                 _healers.size(), _dps.size());
}

// ============================================================================
// MYTHIC+ SPECIFIC
// ============================================================================

uint32 DungeonCoordinator::GetRemainingTime() const
{
    return _mythicPlusManager ? _mythicPlusManager->GetRemainingTime() : 0;
}

bool DungeonCoordinator::ShouldSkipPack(const TrashPack& pack) const
{
    if (!_progress.isMythicPlus)
        return false;

    if (!_mythicPlusManager)
        return false;

    return _mythicPlusManager->ShouldSkipPack(pack.packId);
}

// ============================================================================
// GROUP STATUS
// ============================================================================

float DungeonCoordinator::CalculateGroupHealth() const
{
    auto members = GetGroupMembers();
    if (members.empty())
        return 100.0f;

    float totalHealth = 0.0f;
    uint32 count = 0;

    for (Player* member : members)
    {
        if (member && member->IsAlive())
        {
            totalHealth += member->GetHealthPct();
            count++;
        }
    }

    return count > 0 ? (totalHealth / count) : 0.0f;
}

float DungeonCoordinator::CalculateGroupMana() const
{
    auto members = GetGroupMembers();
    if (members.empty())
        return 100.0f;

    float totalMana = 0.0f;
    uint32 count = 0;

    for (Player* member : members)
    {
        if (member && member->IsAlive() && member->GetMaxPower(POWER_MANA) > 0)
        {
            totalMana += (static_cast<float>(member->GetPower(POWER_MANA)) /
                         member->GetMaxPower(POWER_MANA)) * 100.0f;
            count++;
        }
    }

    return count > 0 ? (totalMana / count) : 100.0f;
}

bool DungeonCoordinator::IsGroupReady() const
{
    float health = CalculateGroupHealth();
    float mana = CalculateGroupMana();

    return health >= MIN_HEALTH_FOR_PULL && mana >= MIN_MANA_FOR_PULL;
}

// ============================================================================
// ICombatEventSubscriber Implementation
// ============================================================================

void DungeonCoordinator::OnCombatEvent(const CombatEvent& event)
{
    switch (event.type)
    {
        case CombatEventType::UNIT_DIED:
            HandleUnitDied(event);
            break;

        case CombatEventType::COMBAT_STARTED:
            HandleCombatStarted(event);
            break;

        case CombatEventType::COMBAT_ENDED:
            HandleCombatEnded(event);
            break;

        case CombatEventType::ENCOUNTER_START:
            HandleEncounterStart(event);
            break;

        case CombatEventType::ENCOUNTER_END:
            HandleEncounterEnd(event);
            break;

        case CombatEventType::BOSS_PHASE_CHANGED:
            HandleBossPhaseChanged(event);
            break;

        default:
            break;
    }
}

CombatEventType DungeonCoordinator::GetSubscribedEventTypes() const
{
    return CombatEventType::UNIT_DIED |
           CombatEventType::COMBAT_STARTED |
           CombatEventType::COMBAT_ENDED |
           CombatEventType::ENCOUNTER_START |
           CombatEventType::ENCOUNTER_END |
           CombatEventType::BOSS_PHASE_CHANGED;
}

bool DungeonCoordinator::ShouldReceiveEvent(const CombatEvent& event) const
{
    // Only care about events in dungeon context
    if (_state == DungeonState::IDLE)
        return false;

    // For unit events, check if it's a group member or relevant target
    if (!event.source.IsEmpty() && IsPlayerInGroup(event.source))
        return true;
    if (!event.target.IsEmpty() && IsPlayerInGroup(event.target))
        return true;

    // For encounter events, always receive
    if (event.IsEncounterEvent())
        return true;

    return false;
}

// ============================================================================
// EVENT HANDLERS
// ============================================================================

void DungeonCoordinator::HandleUnitDied(const CombatEvent& event)
{
    // Check if it's a group member death
    if (IsPlayerInGroup(event.target))
    {
        // Check if all players are dead (wipe)
        auto members = GetGroupMembers();
        bool allDead = true;
        for (Player* member : members)
        {
            if (member && member->IsAlive())
            {
                allDead = false;
                break;
            }
        }

        if (allDead)
        {
            OnGroupWipe();
        }

        // Update M+ death count
        if (_progress.isMythicPlus && _mythicPlusManager)
        {
            _mythicPlusManager->OnPlayerDied();
        }
    }
    else
    {
        // Enemy died - could be trash or boss
        // Boss deaths are handled via ENCOUNTER_END
    }
}

void DungeonCoordinator::HandleCombatStarted(const CombatEvent& event)
{
    if (!IsPlayerInGroup(event.source))
        return;

    // Started combat - check if trash or boss
    if (_state == DungeonState::CLEARING_TRASH || _state == DungeonState::PRE_BOSS)
    {
        // Combat started during trash phase
        if (_trashManager)
        {
            PullPlan* plan = _trashManager->GetCurrentPullPlan();
            if (plan)
            {
                _trashManager->OnPackPulled(plan->packId);
            }
        }
    }
}

void DungeonCoordinator::HandleCombatEnded(const CombatEvent& event)
{
    if (!IsPlayerInGroup(event.source))
        return;

    // Check if all group members are out of combat
    if (!IsGroupInCombat())
    {
        if (_state == DungeonState::CLEARING_TRASH)
        {
            // Combat ended during trash - pack cleared
            if (_trashManager)
            {
                PullPlan* plan = _trashManager->GetCurrentPullPlan();
                if (plan)
                {
                    OnTrashPackCleared(plan->packId);
                    _trashManager->ClearCurrentPlan();
                }
            }
        }
    }
}

void DungeonCoordinator::HandleEncounterStart(const CombatEvent& event)
{
    OnBossEngaged(event.encounterId);
}

void DungeonCoordinator::HandleEncounterEnd(const CombatEvent& event)
{
    // Determine if kill or wipe based on boss state
    BossInfo* boss = GetCurrentBoss();
    if (boss && boss->healthPercent <= 0)
    {
        OnBossDefeated(event.encounterId);
    }
    else
    {
        OnBossWipe(event.encounterId);
    }
}

void DungeonCoordinator::HandleBossPhaseChanged(const CombatEvent& event)
{
    if (_bossManager)
    {
        _bossManager->OnPhaseChanged(event.encounterPhase);
    }
}

// ============================================================================
// HELPERS
// ============================================================================

void DungeonCoordinator::LoadDungeonData(uint32 dungeonId)
{
    _progress.dungeonId = dungeonId;

    // Load boss data
    if (_bossManager)
    {
        _bossManager->LoadBossStrategies(dungeonId);
        _progress.totalBosses = static_cast<uint8>(_bossManager->GetAllBosses().size());
    }

    // Load trash data
    if (_trashManager)
    {
        _trashManager->Initialize(dungeonId);
        // Total trash count set by TrashPullManager
    }

    TC_LOG_DEBUG("playerbot", "DungeonCoordinator::LoadDungeonData - Loaded dungeon %u, %u bosses",
                 dungeonId, _progress.totalBosses);
}

bool DungeonCoordinator::IsGroupInCombat() const
{
    auto members = GetGroupMembers();
    for (Player* member : members)
    {
        if (member && member->IsInCombat())
            return true;
    }
    return false;
}

Player* DungeonCoordinator::GetGroupLeader() const
{
    if (!_group)
        return nullptr;

    ObjectGuid leaderGuid = _group->GetLeaderGUID();
    return ObjectAccessor::FindPlayer(leaderGuid);
}

::std::vector<Player*> DungeonCoordinator::GetGroupMembers() const
{
    ::std::vector<Player*> result;

    if (!_group)
        return result;

    Group::MemberSlotList const& memberSlots = _group->GetMemberSlots();
    for (auto const& slot : memberSlots)
    {
        Player* member = ObjectAccessor::FindPlayer(slot.guid);
        if (member)
        {
            result.push_back(member);
        }
    }

    return result;
}

bool DungeonCoordinator::IsPlayerInGroup(ObjectGuid guid) const
{
    if (!_group || guid.IsEmpty())
        return false;

    Group::MemberSlotList const& memberSlots = _group->GetMemberSlots();
    for (auto const& slot : memberSlots)
    {
        if (slot.guid == guid)
            return true;
    }
    return false;
}

} // namespace Playerbot
