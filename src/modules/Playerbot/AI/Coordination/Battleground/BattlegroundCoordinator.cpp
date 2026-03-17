/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BattlegroundCoordinator.h"
#include "BGSpatialQueryCache.h"
#include "ObjectiveManager.h"
#include "BGRoleManager.h"
#include "FlagCarrierManager.h"
#include "NodeController.h"
#include "BGStrategyEngine.h"
#include "Scripts/BGScriptRegistry.h"
#include "Scripts/IBGScript.h"
#include "Core/Events/CombatEventRouter.h"
#include "AI/Coordination/Messaging/BotMessageBus.h"
#include "AI/Coordination/Messaging/BotMessage.h"
#include "Player.h"
#include "Battleground.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "GameTime.h"
#include "Log.h"
#include <algorithm>

using namespace Playerbot::Coordination::Battleground;

namespace Playerbot {

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

BattlegroundCoordinator::BattlegroundCoordinator(Battleground* bg, ::std::vector<Player*> bots)
    : _battleground(bg)
    , _managedBots(bots)
    , _faction(ALLIANCE)
{
    DetectBGType();

    // Determine faction from first bot
    if (!bots.empty() && bots[0])
    {
        _faction = bots[0]->GetTeam();
    }
}

BattlegroundCoordinator::~BattlegroundCoordinator()
{
    Shutdown();
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void BattlegroundCoordinator::Initialize()
{
    Reset();

    // Transition to active state - coordinator is created when BG starts
    TransitionTo(BGState::ACTIVE);

    // Create sub-managers
    _objectiveManager = ::std::make_unique<ObjectiveManager>(this);
    _roleManager = ::std::make_unique<BGRoleManager>(this);
    _flagManager = ::std::make_unique<FlagCarrierManager>(this);
    _nodeController = ::std::make_unique<NodeController>(this);
    _strategyEngine = ::std::make_unique<BGStrategyEngine>(this);

    // Create spatial query cache for O(1) player lookups
    _spatialCache = ::std::make_unique<BGSpatialQueryCache>(_battleground, _faction);
    _spatialCache->Initialize();
    TC_LOG_DEBUG("playerbot.bg", "BattlegroundCoordinator: Initialized spatial query cache for O(1) lookups");

    // Initialize sub-managers
    _objectiveManager->Initialize();
    _roleManager->Initialize();
    _flagManager->Initialize();
    _nodeController->Initialize();
    _strategyEngine->Initialize();

    // Load BG-specific script via registry
    uint32 mapId = _battleground ? _battleground->GetMapId() : 0;
    if (BGScriptRegistry::Instance().HasScript(mapId))
    {
        _activeScript = BGScriptRegistry::Instance().CreateScript(mapId);
        if (_activeScript)
        {
            _activeScript->OnLoad(this);

            // Load objectives from script
            auto scriptObjectives = _activeScript->GetObjectiveData();
            for (const auto& objData : scriptObjectives)
            {
                BGObjective obj;
                obj.id = objData.id;
                obj.type = objData.type;
                obj.name = objData.name;
                obj.x = objData.x;
                obj.y = objData.y;
                obj.z = objData.z;
                obj.state = BGObjectiveState::NEUTRAL;
                obj.controllingFaction = 0;
                obj.currentPriority = static_cast<BGPriority>(std::min(objData.strategicValue, static_cast<uint8>(4)));
                _objectives.push_back(obj);
            }

            TC_LOG_DEBUG("playerbot.bg", "BattlegroundCoordinator: Loaded script for map %u (%s) with %zu objectives",
                mapId, _activeScript->GetName().c_str(), _objectives.size());
        }
    }
    else
    {
        // Fallback to legacy initialization
        switch (_bgType)
        {
            case BGType::WARSONG_GULCH:
            case BGType::TWIN_PEAKS:
                InitializeWSG();
                break;
            case BGType::ARATHI_BASIN:
            case BGType::BATTLE_FOR_GILNEAS:
                InitializeAB();
                break;
            case BGType::ALTERAC_VALLEY:
                InitializeAV();
                break;
            case BGType::EYE_OF_THE_STORM:
                InitializeEOTS();
                break;
            case BGType::STRAND_OF_THE_ANCIENTS:
                InitializeSOTA();
                break;
            case BGType::ISLE_OF_CONQUEST:
                InitializeIOC();
                break;
            case BGType::SILVERSHARD_MINES:
                InitializeSilvershardMines();
                break;
            case BGType::TEMPLE_OF_KOTMOGU:
                InitializeTempleOfKotmogu();
                break;
            case BGType::DEEPWIND_GORGE:
                InitializeDeepwindGorge();
                break;
            default:
                break;
        }
    }

    // Initialize bot tracking
    for (Player* bot : _managedBots)
    {
        if (!bot) continue;

        BGPlayer bgPlayer;
        bgPlayer.guid = bot->GetGUID();
        bgPlayer.classId = bot->GetClass();
        bgPlayer.healthPercent = bot->GetHealthPct();
        bgPlayer.manaPercent = bot->GetPowerPct(POWER_MANA);
        bgPlayer.isAlive = bot->IsAlive();
        bgPlayer.x = bot->GetPositionX();
        bgPlayer.y = bot->GetPositionY();
        bgPlayer.z = bot->GetPositionZ();

        _bots.push_back(bgPlayer);
    }

    // Assign initial roles to all bots
    if (_roleManager && !_bots.empty())
    {
        _roleManager->AssignAllRoles();
        TC_LOG_INFO("playerbot.bg", "BattlegroundCoordinator: Assigned initial roles to %zu bots", _bots.size());
    }

    // Subscribe to combat events
    CombatEventRouter::Instance().Subscribe(this);

    TC_LOG_DEBUG("playerbot", "BattlegroundCoordinator::Initialize - Initialized for %s with %zu bots",
                 BGTypeToString(_bgType), _bots.size());
}

void BattlegroundCoordinator::Shutdown()
{
    // Unsubscribe from events
    CombatEventRouter::Instance().Unsubscribe(this);

    // Log spatial cache metrics before shutdown
    if (_spatialCache)
    {
        _spatialCache->LogPerformanceSummary();
        _spatialCache.reset();
    }

    // Unload script
    if (_activeScript)
    {
        _activeScript->OnUnload();
        _activeScript.reset();
    }

    _objectiveManager.reset();
    _roleManager.reset();
    _flagManager.reset();
    _nodeController.reset();
    _strategyEngine.reset();

    TC_LOG_DEBUG("playerbot", "BattlegroundCoordinator::Shutdown - Shutdown complete");
}

void BattlegroundCoordinator::Update(uint32 diff)
{
    if (_state == BGState::IDLE || _state == BGState::QUEUED)
        return;

    // Update state machine
    UpdateState(diff);

    // Only update sub-managers during active play
    if (_state == BGState::ACTIVE || _state == BGState::OVERTIME)
    {
        // Update spatial cache FIRST (other systems depend on it)
        if (_spatialCache)
            _spatialCache->Update(diff);

        // Update tracking
        UpdateBotTracking(diff);
        UpdateScoreTracking();
        UpdateObjectiveTracking(diff);

        // Update sub-managers
        if (_objectiveManager)
            _objectiveManager->Update(diff);

        if (_roleManager)
            _roleManager->Update(diff);

        if (_flagManager && IsCTFMap())
            _flagManager->Update(diff);

        if (_nodeController)
            _nodeController->Update(diff);

        if (_strategyEngine)
            _strategyEngine->Update(diff);

        // Update active script
        if (_activeScript)
            _activeScript->OnUpdate(diff);

        // Evaluate strategy periodically
        EvaluateStrategy(diff);
    }
}

void BattlegroundCoordinator::NotifyScriptEvent(const BGScriptEventData& event)
{
    if (_activeScript)
    {
        _activeScript->OnEvent(event);
    }
}

void BattlegroundCoordinator::Reset()
{
    _state = BGState::IDLE;
    _score = BGScoreInfo();
    _objectives.clear();
    _bots.clear();
    _matchStats = BGMatchStats();
    _matchStartTime = 0;
    _friendlyFlag = FlagInfo();
    _enemyFlag = FlagInfo();

    if (_objectiveManager) _objectiveManager->Reset();
    if (_roleManager) _roleManager->Reset();
    if (_flagManager) _flagManager->Reset();
    if (_nodeController) _nodeController->Reset();
    if (_strategyEngine) _strategyEngine->Reset();
}

// ============================================================================
// SCORE
// ============================================================================

bool BattlegroundCoordinator::IsWinning() const
{
    if (_faction == ALLIANCE)
        return _score.allianceScore > _score.hordeScore;
    return _score.hordeScore > _score.allianceScore;
}

float BattlegroundCoordinator::GetScoreAdvantage() const
{
    float ourScore = (_faction == ALLIANCE) ? _score.allianceScore : _score.hordeScore;
    float theirScore = (_faction == ALLIANCE) ? _score.hordeScore : _score.allianceScore;

    if (_score.maxScore == 0)
        return 0.0f;

    return (ourScore - theirScore) / static_cast<float>(_score.maxScore);
}

uint32 BattlegroundCoordinator::GetTimeRemaining() const
{
    return _score.timeRemaining;
}

float BattlegroundCoordinator::GetWinProbability() const
{
    return _strategyEngine ? _strategyEngine->GetWinProbability() : 0.5f;
}

// ============================================================================
// OBJECTIVES
// ============================================================================

::std::vector<BGObjective> BattlegroundCoordinator::GetObjectives() const
{
    return _objectives;
}

BGObjective* BattlegroundCoordinator::GetObjective(uint32 objectiveId)
{
    for (auto& obj : _objectives)
    {
        if (obj.id == objectiveId)
            return &obj;
    }
    return nullptr;
}

const BGObjective* BattlegroundCoordinator::GetObjective(uint32 objectiveId) const
{
    for (const auto& obj : _objectives)
    {
        if (obj.id == objectiveId)
            return &obj;
    }
    return nullptr;
}

BGObjectiveState BattlegroundCoordinator::GetObjectiveState(uint32 objectiveId) const
{
    const BGObjective* obj = GetObjective(objectiveId);
    return obj ? obj->state : BGObjectiveState::NEUTRAL;
}

BGObjective* BattlegroundCoordinator::GetNearestObjective(ObjectGuid player, ObjectiveType type) const
{
    return _objectiveManager ? _objectiveManager->GetNearestObjectiveOfType(player, type) : nullptr;
}

::std::vector<BGObjective*> BattlegroundCoordinator::GetContestedObjectives() const
{
    return _objectiveManager ? _objectiveManager->GetContestedObjectives() : ::std::vector<BGObjective*>();
}

uint32 BattlegroundCoordinator::GetControlledObjectiveCount() const
{
    return _objectiveManager ? _objectiveManager->GetControlledCount() : 0;
}

uint32 BattlegroundCoordinator::GetEnemyControlledObjectiveCount() const
{
    return _objectiveManager ? _objectiveManager->GetEnemyControlledCount() : 0;
}

// ============================================================================
// ROLE MANAGEMENT
// ============================================================================

BGRole BattlegroundCoordinator::GetBotRole(ObjectGuid bot) const
{
    if (!_roleManager)
    {
        TC_LOG_WARN("playerbot.bg", "BattlegroundCoordinator::GetBotRole - No role manager!");
        return BGRole::UNASSIGNED;
    }

    BGRole role = _roleManager->GetRole(bot);
    if (role == BGRole::UNASSIGNED)
    {
        TC_LOG_DEBUG("playerbot.bg",
            "BattlegroundCoordinator::GetBotRole - Bot {} has UNASSIGNED role (assignments: {})",
            bot.GetCounter(), _roleManager->GetAssignmentCount());
    }
    return role;
}

void BattlegroundCoordinator::AssignRole(ObjectGuid bot, BGRole role)
{
    if (_roleManager)
        _roleManager->AssignRole(bot, role);
}

void BattlegroundCoordinator::AssignToObjective(ObjectGuid bot, uint32 objectiveId)
{
    if (_nodeController)
        _nodeController->AssignDefender(objectiveId, bot);
}

::std::vector<ObjectGuid> BattlegroundCoordinator::GetBotsWithRole(BGRole role) const
{
    return _roleManager ? _roleManager->GetPlayersWithRole(role) : ::std::vector<ObjectGuid>();
}

uint32 BattlegroundCoordinator::GetRoleCount(BGRole role) const
{
    return _roleManager ? _roleManager->GetRoleCount(role) : 0;
}

// ============================================================================
// FLAG MANAGEMENT
// ============================================================================

bool BattlegroundCoordinator::IsCTFMap() const
{
    return _bgType == BGType::WARSONG_GULCH ||
           _bgType == BGType::TWIN_PEAKS ||
           _bgType == BGType::EYE_OF_THE_STORM;
}

bool BattlegroundCoordinator::HasFlag(ObjectGuid player) const
{
    return _flagManager && (_flagManager->GetFriendlyFC() == player || _flagManager->GetEnemyFC() == player);
}

ObjectGuid BattlegroundCoordinator::GetFriendlyFC() const
{
    return _flagManager ? _flagManager->GetFriendlyFC() : ObjectGuid::Empty;
}

ObjectGuid BattlegroundCoordinator::GetEnemyFC() const
{
    return _flagManager ? _flagManager->GetEnemyFC() : ObjectGuid::Empty;
}

const FlagInfo& BattlegroundCoordinator::GetFriendlyFlag() const
{
    return _friendlyFlag;
}

const FlagInfo& BattlegroundCoordinator::GetEnemyFlag() const
{
    return _enemyFlag;
}

bool BattlegroundCoordinator::CanCaptureFlag() const
{
    return _flagManager ? _flagManager->CanCapture() : false;
}

bool BattlegroundCoordinator::ShouldDropFlag() const
{
    return _flagManager ? _flagManager->IsFCDebuffCritical() : false;
}

// ============================================================================
// STRATEGIC COMMANDS
// ============================================================================

void BattlegroundCoordinator::CommandAttack(uint32 objectiveId)
{
    TC_LOG_DEBUG("playerbot", "BattlegroundCoordinator: Attack command for objective %u", objectiveId);

    if (_strategyEngine)
        _strategyEngine->OverrideObjectivePriority(objectiveId, 4);
}

void BattlegroundCoordinator::CommandDefend(uint32 objectiveId)
{
    TC_LOG_DEBUG("playerbot", "BattlegroundCoordinator: Defend command for objective %u", objectiveId);

    if (_nodeController)
        _nodeController->RequestReinforcements(objectiveId, 2);
}

void BattlegroundCoordinator::CommandRecall()
{
    TC_LOG_DEBUG("playerbot", "BattlegroundCoordinator: Recall command");

    if (_strategyEngine)
        _strategyEngine->ForceStrategy(BGStrategy::DEFENSIVE);
}

void BattlegroundCoordinator::CommandPush()
{
    TC_LOG_DEBUG("playerbot", "BattlegroundCoordinator: Push command");

    if (_strategyEngine)
        _strategyEngine->ForceStrategy(BGStrategy::AGGRESSIVE);
}

void BattlegroundCoordinator::CommandRegroup()
{
    TC_LOG_DEBUG("playerbot", "BattlegroundCoordinator: Regroup command");

    if (_roleManager)
        _roleManager->RebalanceRoles();
}

// ============================================================================
// BOT QUERIES
// ============================================================================

BGObjective* BattlegroundCoordinator::GetAssignment(ObjectGuid bot) const
{
    if (!_nodeController)
        return nullptr;

    uint32 nodeId = _nodeController->GetPlayerAssignment(bot);
    if (nodeId == 0)
        return nullptr;

    return const_cast<BattlegroundCoordinator*>(this)->GetObjective(nodeId);
}

float BattlegroundCoordinator::GetDistanceToAssignment(ObjectGuid bot) const
{
    BGObjective* obj = GetAssignment(bot);
    if (!obj)
        return 0.0f;

    const BGPlayer* player = GetBot(bot);
    if (!player)
        return 0.0f;

    float dx = obj->x - player->x;
    float dy = obj->y - player->y;
    float dz = obj->z - player->z;

    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

bool BattlegroundCoordinator::ShouldContestObjective(ObjectGuid bot) const
{
    BGObjective* obj = GetAssignment(bot);
    if (!obj)
        return false;

    return obj->isContested || IsEnemyObjective(*obj);
}

bool BattlegroundCoordinator::ShouldRetreat(ObjectGuid bot) const
{
    const BGPlayer* player = GetBot(bot);
    if (!player)
        return false;

    // Retreat if low health and no healers nearby
    if (player->healthPercent < 30.0f)
        return true;

    // FC should not retreat if close to capture
    if (_flagManager && _flagManager->GetFriendlyFC() == bot)
    {
        if (_flagManager->IsFCNearCapture())
            return false;
    }

    return false;
}

bool BattlegroundCoordinator::ShouldAssist(ObjectGuid bot, ObjectGuid ally) const
{
    const BGPlayer* player = GetBot(bot);
    const BGPlayer* allyPlayer = GetBot(ally);

    if (!player || !allyPlayer)
        return false;

    // Assist FC
    if (_flagManager && _flagManager->GetFriendlyFC() == ally)
        return true;

    // Assist low health ally
    if (allyPlayer->healthPercent < 50.0f)
        return true;

    return false;
}

// ============================================================================
// PLAYER TRACKING
// ============================================================================

void BattlegroundCoordinator::AddBot(Player* bot)
{
    if (!bot)
        return;

    ObjectGuid guid = bot->GetGUID();

    // Check for duplicate
    for (const auto& existing : _bots)
    {
        if (existing.guid == guid)
            return; // Already tracked
    }

    // Add to managed bots list
    _managedBots.push_back(bot);

    // Create tracking entry
    BGPlayer bgPlayer;
    bgPlayer.guid = guid;
    bgPlayer.classId = bot->GetClass();
    bgPlayer.healthPercent = bot->GetHealthPct();
    bgPlayer.manaPercent = bot->GetPowerPct(POWER_MANA);
    bgPlayer.isAlive = bot->IsAlive();
    bgPlayer.x = bot->GetPositionX();
    bgPlayer.y = bot->GetPositionY();
    bgPlayer.z = bot->GetPositionZ();

    _bots.push_back(bgPlayer);

    // Assign best-fit role based on class/spec suitability
    if (_roleManager)
    {
        BGRole bestRole = _roleManager->GetBestRole(guid);
        if (bestRole == BGRole::UNASSIGNED)
            bestRole = BGRole::ROAMER; // Safe fallback
        _roleManager->AssignRole(guid, bestRole);
    }

    TC_LOG_INFO("playerbot.bg",
        "BattlegroundCoordinator::AddBot - Late-joined bot {} added (total: {})",
        bot->GetName(), _bots.size());
}

const BGPlayer* BattlegroundCoordinator::GetBot(ObjectGuid guid) const
{
    for (const auto& bot : _bots)
    {
        if (bot.guid == guid)
            return &bot;
    }
    return nullptr;
}

BGPlayer* BattlegroundCoordinator::GetBotMutable(ObjectGuid guid)
{
    for (auto& bot : _bots)
    {
        if (bot.guid == guid)
            return &bot;
    }
    return nullptr;
}

::std::vector<BGPlayer> BattlegroundCoordinator::GetAllBots() const
{
    return _bots;
}

::std::vector<BGPlayer> BattlegroundCoordinator::GetAliveBots() const
{
    ::std::vector<BGPlayer> alive;
    for (const auto& bot : _bots)
    {
        if (bot.isAlive)
            alive.push_back(bot);
    }
    return alive;
}

// ============================================================================
// PLAYER ACCESS (for sub-managers)
// ============================================================================

Player* BattlegroundCoordinator::GetPlayer(ObjectGuid guid) const
{
    if (guid.IsEmpty())
        return nullptr;

    return ObjectAccessor::FindPlayer(guid);
}

::std::vector<ObjectGuid> BattlegroundCoordinator::GetFriendlyPlayers() const
{
    ::std::vector<ObjectGuid> friendlyPlayers;
    friendlyPlayers.reserve(_bots.size());

    for (const auto& bot : _bots)
    {
        friendlyPlayers.push_back(bot.guid);
    }

    return friendlyPlayers;
}

::std::vector<ObjectGuid> BattlegroundCoordinator::GetEnemyPlayers() const
{
    ::std::vector<ObjectGuid> enemyPlayers;

    if (!_battleground)
        return enemyPlayers;

    // Get enemy team
    Team enemyTeam = (_faction == ALLIANCE) ? HORDE : ALLIANCE;

    // Query battleground for enemy players
    for (auto const& itr : _battleground->GetPlayers())
    {
        if (Player* player = ObjectAccessor::FindPlayer(itr.first))
        {
            if (player->GetTeam() == enemyTeam)
            {
                enemyPlayers.push_back(player->GetGUID());
            }
        }
    }

    return enemyPlayers;
}

// ============================================================================
// ICOMBATEVENTSUBSCRIBER
// ============================================================================

void BattlegroundCoordinator::OnCombatEvent(const CombatEvent& event)
{
    if (_state != BGState::ACTIVE && _state != BGState::OVERTIME)
        return;

    switch (event.type)
    {
        case CombatEventType::UNIT_DIED:
            if (IsAlly(event.target))
            {
                HandlePlayerDied(event.target, event.source);
            }
            else if (IsEnemy(event.target))
            {
                HandlePlayerKill(event.source, event.target);
            }
            break;

        default:
            break;
    }
}

CombatEventType BattlegroundCoordinator::GetSubscribedEventTypes() const
{
    return CombatEventType::UNIT_DIED |
           CombatEventType::DAMAGE_DEALT |
           CombatEventType::HEALING_DONE;
}

// ============================================================================
// STATE MACHINE
// ============================================================================

void BattlegroundCoordinator::UpdateState(uint32 /*diff*/)
{
    // State transitions based on battleground status
    // In real implementation, would check BG status from battleground object
}

void BattlegroundCoordinator::TransitionTo(BGState newState)
{
    if (_state == newState)
        return;

    OnStateExit(_state);

    TC_LOG_DEBUG("playerbot", "BattlegroundCoordinator: State transition %s -> %s",
                 BGStateToString(_state), BGStateToString(newState));

    _state = newState;
    OnStateEnter(newState);
}

void BattlegroundCoordinator::OnStateEnter(BGState state)
{
    switch (state)
    {
        case BGState::PREPARATION:
            _matchStats = BGMatchStats();
            _matchStats.bgType = _bgType;
            break;

        case BGState::ACTIVE:
            _matchStartTime = GameTime::GetGameTimeMS();
            _matchStats.matchStartTime = _matchStartTime;

            // Assign initial roles
            if (_roleManager)
                _roleManager->AssignAllRoles();

            // Broadcast use defensives (opening engagement) via BotMessageBus
            if (!_bots.empty())
            {
                Player* leader = ObjectAccessor::FindPlayer(_bots.front().guid);
                if (leader && leader->GetGroup())
                {
                    ObjectGuid groupGuid = leader->GetGroup()->GetGUID();
                    BotMessage msg = BotMessage::CommandUseDefensives(leader->GetGUID(), groupGuid);
                    sBotMessageBus->Publish(msg);
                }
            }
            break;

        case BGState::VICTORY:
        case BGState::DEFEAT:
            _matchStats.matchDuration = GameTime::GetGameTimeMS() - _matchStartTime;
            break;

        default:
            break;
    }
}

void BattlegroundCoordinator::OnStateExit(BGState /*state*/)
{
    // Cleanup if needed
}

// ============================================================================
// BG-SPECIFIC INITIALIZATION
// ============================================================================

void BattlegroundCoordinator::DetectBGType()
{
    if (!_battleground)
    {
        _bgType = BGType::WARSONG_GULCH;
        return;
    }

    // Would detect from battleground map ID
    _bgType = static_cast<BGType>(_battleground->GetMapId());
}

void BattlegroundCoordinator::InitializeWSG()
{
    // WSG has two flag objectives
    _score.maxScore = 3;

    // Role requirements for CTF
    if (_roleManager)
    {
        _roleManager->SetRoleRequirement(BGRole::FLAG_CARRIER, 0, 1, 1);
        _roleManager->SetRoleRequirement(BGRole::FLAG_ESCORT, 1, 3, 2);
        _roleManager->SetRoleRequirement(BGRole::FLAG_HUNTER, 1, 3, 2);
        _roleManager->SetRoleRequirement(BGRole::NODE_DEFENDER, 1, 2, 1);
        _roleManager->SetRoleRequirement(BGRole::ROAMER, 0, 2, 1);
    }
}

void BattlegroundCoordinator::InitializeAB()
{
    // AB has 5 nodes, 1600 to win
    _score.maxScore = 1600;

    // Register nodes
    BGObjective stables;
    stables.id = 1;
    stables.type = ObjectiveType::NODE;
    stables.name = "Stables";
    stables.strategicValue = 7;
    _objectives.push_back(stables);

    BGObjective blacksmith;
    blacksmith.id = 2;
    blacksmith.type = ObjectiveType::NODE;
    blacksmith.name = "Blacksmith";
    blacksmith.strategicValue = 8;
    _objectives.push_back(blacksmith);

    BGObjective lumberMill;
    lumberMill.id = 3;
    lumberMill.type = ObjectiveType::NODE;
    lumberMill.name = "Lumber Mill";
    lumberMill.strategicValue = 7;
    _objectives.push_back(lumberMill);

    BGObjective goldMine;
    goldMine.id = 4;
    goldMine.type = ObjectiveType::NODE;
    goldMine.name = "Gold Mine";
    goldMine.strategicValue = 6;
    _objectives.push_back(goldMine);

    BGObjective farm;
    farm.id = 5;
    farm.type = ObjectiveType::NODE;
    farm.name = "Farm";
    farm.strategicValue = 7;
    _objectives.push_back(farm);

    // Register objectives with manager
    if (_objectiveManager)
    {
        for (const auto& obj : _objectives)
        {
            _objectiveManager->RegisterObjective(obj);
        }
    }

    // Role requirements for node control
    if (_roleManager)
    {
        _roleManager->SetRoleRequirement(BGRole::NODE_DEFENDER, 3, 5, 4);
        _roleManager->SetRoleRequirement(BGRole::NODE_ATTACKER, 2, 4, 3);
        _roleManager->SetRoleRequirement(BGRole::ROAMER, 1, 3, 2);
    }
}

void BattlegroundCoordinator::InitializeAV()
{
    _score.maxScore = 600;

    // AV has many objectives - simplified
    if (_roleManager)
    {
        _roleManager->SetRoleRequirement(BGRole::NODE_DEFENDER, 5, 10, 7);
        _roleManager->SetRoleRequirement(BGRole::NODE_ATTACKER, 5, 15, 10);
        _roleManager->SetRoleRequirement(BGRole::GRAVEYARD_ASSAULT, 2, 5, 3);
    }
}

void BattlegroundCoordinator::InitializeEOTS()
{
    _score.maxScore = 1600;

    // EOTS has 4 nodes + flag
    if (_roleManager)
    {
        _roleManager->SetRoleRequirement(BGRole::FLAG_CARRIER, 0, 1, 1);
        _roleManager->SetRoleRequirement(BGRole::FLAG_ESCORT, 1, 2, 2);
        _roleManager->SetRoleRequirement(BGRole::NODE_DEFENDER, 2, 4, 3);
        _roleManager->SetRoleRequirement(BGRole::NODE_ATTACKER, 1, 3, 2);
    }
}

void BattlegroundCoordinator::InitializeSOTA()
{
    if (_roleManager)
    {
        _roleManager->SetRoleRequirement(BGRole::NODE_ATTACKER, 5, 10, 7);
        _roleManager->SetRoleRequirement(BGRole::NODE_DEFENDER, 3, 8, 5);
    }
}

void BattlegroundCoordinator::InitializeIOC()
{
    _score.maxScore = 300;

    if (_roleManager)
    {
        _roleManager->SetRoleRequirement(BGRole::NODE_DEFENDER, 4, 8, 6);
        _roleManager->SetRoleRequirement(BGRole::NODE_ATTACKER, 4, 10, 7);
        _roleManager->SetRoleRequirement(BGRole::RESOURCE_GATHERER, 1, 3, 2);
    }
}

void BattlegroundCoordinator::InitializeTwinPeaks()
{
    // Same as WSG
    InitializeWSG();
}

void BattlegroundCoordinator::InitializeBFG()
{
    // Same as AB with 3 nodes
    _score.maxScore = 1600;

    if (_roleManager)
    {
        _roleManager->SetRoleRequirement(BGRole::NODE_DEFENDER, 2, 4, 3);
        _roleManager->SetRoleRequirement(BGRole::NODE_ATTACKER, 2, 4, 3);
        _roleManager->SetRoleRequirement(BGRole::ROAMER, 1, 3, 2);
    }
}

void BattlegroundCoordinator::InitializeSilvershardMines()
{
    _score.maxScore = 1600;

    if (_roleManager)
    {
        _roleManager->SetRoleRequirement(BGRole::CART_PUSHER, 2, 4, 3);
        _roleManager->SetRoleRequirement(BGRole::NODE_DEFENDER, 2, 4, 3);
        _roleManager->SetRoleRequirement(BGRole::ROAMER, 1, 3, 2);
    }
}

void BattlegroundCoordinator::InitializeTempleOfKotmogu()
{
    _score.maxScore = 1600;

    if (_roleManager)
    {
        _roleManager->SetRoleRequirement(BGRole::ORB_CARRIER, 1, 2, 2);
        _roleManager->SetRoleRequirement(BGRole::FLAG_ESCORT, 2, 4, 3);
        _roleManager->SetRoleRequirement(BGRole::FLAG_HUNTER, 1, 3, 2);
    }
}

void BattlegroundCoordinator::InitializeDeepwindGorge()
{
    _score.maxScore = 1600;

    if (_roleManager)
    {
        _roleManager->SetRoleRequirement(BGRole::NODE_DEFENDER, 2, 4, 3);
        _roleManager->SetRoleRequirement(BGRole::RESOURCE_GATHERER, 2, 4, 3);
        _roleManager->SetRoleRequirement(BGRole::ROAMER, 1, 3, 2);
    }
}

// ============================================================================
// EVENT HANDLERS
// ============================================================================

void BattlegroundCoordinator::HandleObjectiveCaptured(uint32 objectiveId, uint32 faction)
{
    if (_objectiveManager)
        _objectiveManager->OnObjectiveCaptured(objectiveId, faction);

    bool isFriendly = (faction == _faction);
    if (isFriendly)
    {
        _matchStats.objectivesCaptured++;
    }
    else
    {
        _matchStats.objectivesLost++;
    }
}

void BattlegroundCoordinator::HandleObjectiveLost(uint32 objectiveId)
{
    if (_objectiveManager)
        _objectiveManager->OnObjectiveLost(objectiveId);

    _matchStats.objectivesLost++;
}

void BattlegroundCoordinator::HandleFlagPickup(ObjectGuid player, bool isEnemyFlag)
{
    if (_flagManager)
        _flagManager->OnFlagPickedUp(player, isEnemyFlag);
}

void BattlegroundCoordinator::HandleFlagDrop(ObjectGuid player)
{
    if (_flagManager)
        _flagManager->OnFlagDropped(player, 0, 0, 0);
}

void BattlegroundCoordinator::HandleFlagCapture(ObjectGuid player)
{
    if (_flagManager)
        _flagManager->OnFlagCaptured(player);

    _matchStats.flagCaptures++;
}

void BattlegroundCoordinator::HandleFlagReturn(ObjectGuid player)
{
    if (_flagManager)
        _flagManager->OnFlagReturned(player);

    _matchStats.flagReturns++;
}

void BattlegroundCoordinator::HandlePlayerDied(ObjectGuid player, ObjectGuid killer)
{
    BGPlayer* bot = GetBotMutable(player);
    if (bot)
    {
        bot->isAlive = false;
        bot->deaths++;
    }

    _matchStats.totalDeaths++;

    (void)killer;
}

void BattlegroundCoordinator::HandlePlayerKill(ObjectGuid killer, ObjectGuid victim)
{
    BGPlayer* bot = GetBotMutable(killer);
    if (bot)
    {
        bot->kills++;
        bot->honorableKills++;
    }

    _matchStats.totalKills++;

    (void)victim;
}

// ============================================================================
// STRATEGIC DECISIONS
// ============================================================================

void BattlegroundCoordinator::EvaluateStrategy(uint32 /*diff*/)
{
    // Strategy is handled by BGStrategyEngine
}

void BattlegroundCoordinator::ReassignRoles()
{
    if (_roleManager)
        _roleManager->RebalanceRoles();
}

void BattlegroundCoordinator::UpdateObjectivePriorities()
{
    // Handled by ObjectiveManager
}

// ============================================================================
// TRACKING UPDATES
// ============================================================================

void BattlegroundCoordinator::UpdateBotTracking(uint32 /*diff*/)
{
    for (auto& bot : _bots)
    {
        Player* player = ObjectAccessor::FindPlayer(bot.guid);
        if (!player)
            continue;

        bot.healthPercent = player->GetHealthPct();
        bot.manaPercent = player->GetPowerPct(POWER_MANA);
        bot.isAlive = player->IsAlive();
        bot.isInCombat = player->IsInCombat();
        bot.x = player->GetPositionX();
        bot.y = player->GetPositionY();
        bot.z = player->GetPositionZ();

        // Update nearest objective
        if (_objectiveManager)
        {
            BGObjective* nearest = _objectiveManager->GetNearestObjective(bot.guid);
            if (nearest)
            {
                bot.nearestObjectiveId = nearest->id;
                float dx = nearest->x - bot.x;
                float dy = nearest->y - bot.y;
                float dz = nearest->z - bot.z;
                bot.distanceToObjective = std::sqrt(dx*dx + dy*dy + dz*dz);
            }
        }
    }
}

void BattlegroundCoordinator::UpdateScoreTracking()
{
    // Would query battleground for current scores
    // Score info would come from the BG object
}

void BattlegroundCoordinator::UpdateObjectiveTracking(uint32 /*diff*/)
{
    // Would update objective states from BG object
}

// ============================================================================
// UTILITY
// ============================================================================

bool BattlegroundCoordinator::IsAlly(ObjectGuid player) const
{
    for (const auto& bot : _bots)
    {
        if (bot.guid == player)
            return true;
    }
    return false;
}

bool BattlegroundCoordinator::IsEnemy(ObjectGuid player) const
{
    return !IsAlly(player);
}

bool BattlegroundCoordinator::IsFriendlyObjective(const BGObjective& objective) const
{
    if (_faction == ALLIANCE)
    {
        return objective.state == BGObjectiveState::ALLIANCE_CONTROLLED ||
               objective.state == BGObjectiveState::ALLIANCE_CONTESTED ||
               objective.state == BGObjectiveState::ALLIANCE_CAPTURING;
    }
    return objective.state == BGObjectiveState::HORDE_CONTROLLED ||
           objective.state == BGObjectiveState::HORDE_CONTESTED ||
           objective.state == BGObjectiveState::HORDE_CAPTURING;
}

bool BattlegroundCoordinator::IsEnemyObjective(const BGObjective& objective) const
{
    return !IsFriendlyObjective(objective) && objective.state != BGObjectiveState::NEUTRAL;
}

// ============================================================================
// SPATIAL QUERY CACHE (O(1) OPTIMIZED LOOKUPS)
// ============================================================================

ObjectGuid BattlegroundCoordinator::GetCachedFriendlyFC() const
{
    if (_spatialCache)
        return _spatialCache->GetFriendlyFlagCarrier();
    return ObjectGuid::Empty;
}

ObjectGuid BattlegroundCoordinator::GetCachedEnemyFC() const
{
    if (_spatialCache)
        return _spatialCache->GetEnemyFlagCarrier();
    return ObjectGuid::Empty;
}

bool BattlegroundCoordinator::GetCachedFriendlyFCPosition(Position& outPosition) const
{
    if (_spatialCache)
        return _spatialCache->GetFriendlyFCPosition(outPosition);
    return false;
}

bool BattlegroundCoordinator::GetCachedEnemyFCPosition(Position& outPosition) const
{
    if (_spatialCache)
        return _spatialCache->GetEnemyFCPosition(outPosition);
    return false;
}

BGPlayerSnapshot const* BattlegroundCoordinator::GetPlayerSnapshot(ObjectGuid guid) const
{
    if (_spatialCache)
        return _spatialCache->GetPlayerSnapshot(guid);
    return nullptr;
}

::std::vector<BGPlayerSnapshot const*> BattlegroundCoordinator::QueryNearbyEnemies(
    Position const& position, float radius, uint32 callerFaction) const
{
    if (_spatialCache)
        return _spatialCache->QueryNearbyEnemies(position, radius, callerFaction);
    return {};
}

::std::vector<BGPlayerSnapshot const*> BattlegroundCoordinator::QueryNearbyAllies(
    Position const& position, float radius, uint32 callerFaction) const
{
    if (_spatialCache)
        return _spatialCache->QueryNearbyAllies(position, radius, callerFaction);
    return {};
}

BGPlayerSnapshot const* BattlegroundCoordinator::GetNearestEnemy(
    Position const& position, float maxRadius, uint32 callerFaction,
    ObjectGuid excludeGuid, float* outDistance) const
{
    if (_spatialCache)
        return _spatialCache->GetNearestEnemy(position, maxRadius, callerFaction, excludeGuid, outDistance);
    return nullptr;
}

BGPlayerSnapshot const* BattlegroundCoordinator::GetNearestAlly(
    Position const& position, float maxRadius, uint32 callerFaction,
    ObjectGuid excludeGuid, float* outDistance) const
{
    if (_spatialCache)
        return _spatialCache->GetNearestAlly(position, maxRadius, callerFaction, excludeGuid, outDistance);
    return nullptr;
}

uint32 BattlegroundCoordinator::CountEnemiesInRadius(Position const& position, float radius, uint32 callerFaction) const
{
    if (_spatialCache)
        return _spatialCache->CountEnemiesInRadius(position, radius, callerFaction);
    return 0;
}

uint32 BattlegroundCoordinator::CountAlliesInRadius(Position const& position, float radius, uint32 callerFaction) const
{
    if (_spatialCache)
        return _spatialCache->CountAlliesInRadius(position, radius, callerFaction);
    return 0;
}

void BattlegroundCoordinator::LogSpatialCacheMetrics() const
{
    if (_spatialCache)
        _spatialCache->LogPerformanceSummary();
}

} // namespace Playerbot
