/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "LeaderFollowBehavior.h"
#include "BotMovementUtil.h"
#include "AI/BotAI.h"
#include "AI/Actions/Action.h"
#include "AI/Actions/CommonActions.h"
#include "AI/Triggers/Trigger.h"
#include "AI/Values/Value.h"
#include "AI/Combat/GroupCombatTrigger.h"
#include "AI/Actions/TargetAssistAction.h"
#include "Player.h"
#include "Group.h"
#include "Unit.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Map.h"
#include "MotionMaster.h"
#include "MovementGenerator.h"
#include "PathGenerator.h"
#include "Log.h"
#include "World.h"
#include "WorldSession.h"
#include "../Session/BotSession.h"  // For thread-safe QueueStopMovement()
#include "UnifiedMovementCoordinator.h"  // Phase 2: Unified movement system
#include "Arbiter/MovementRequest.h"
#include "Arbiter/MovementPriorityMapper.h"
#include "UnitAI.h"
#include "ChaseMovementGenerator.h"
#include <cmath>
#include <set>
#include <unordered_map>
#include "GameTime.h"

namespace Playerbot
{

// FollowAction is now imported from CommonActions.h

// Stop Follow Action
class StopFollowAction : public Action
{
public:
    StopFollowAction(LeaderFollowBehavior* behavior)
        : Action("stop follow"), _behavior(behavior) {}

    virtual bool IsPossible(BotAI* ai) const override
    {
        return ai != nullptr && _behavior != nullptr;
    }

    virtual bool IsUseful(BotAI* ai) const override
    {
        return _behavior != nullptr;
    }

    virtual ActionResult Execute(BotAI* ai, ActionContext const& context) override
    {
        if (!ai || !_behavior)
            return ActionResult::FAILED;

        // DEADLOCK FIX: Use stored pointer instead of calling ai->GetStrategy()
        // which would acquire BotAI::_mutex recursively
        _behavior->ClearFollowTarget();
        return ActionResult::SUCCESS;
    }

private:
    LeaderFollowBehavior* _behavior;
};

// Leader Far Trigger
class LeaderFarTrigger : public Trigger
{
public:
    LeaderFarTrigger(LeaderFollowBehavior* behavior)
        : Trigger("leader far", TriggerType::DISTANCE), _behavior(behavior) {}

    virtual bool Check(BotAI* ai) const override
    {
        if (!ai || !ai->GetBot() || !_behavior)
            return false;

        // DEADLOCK FIX: Use stored pointer instead of calling ai->GetStrategy()
        // which would acquire BotAI::_mutex recursively
    if (!_behavior->HasFollowTarget())
            return false;

        return _behavior->GetDistanceToLeader() > 30.0f;
    }

private:
    LeaderFollowBehavior* _behavior;
};

// Leader Lost Trigger
class LeaderLostTrigger : public Trigger
{
public:
    LeaderLostTrigger(LeaderFollowBehavior* behavior)
        : Trigger("leader lost", TriggerType::WORLD), _behavior(behavior) {}

    virtual bool Check(BotAI* ai) const override
    {
        if (!ai || !ai->GetBot() || !_behavior)
            return false;

        // DEADLOCK FIX: Use stored pointer instead of calling ai->GetStrategy()
        // which would acquire BotAI::_mutex recursively
    if (!_behavior->HasFollowTarget())
            return false;

        return !_behavior->IsLeaderInSight() &&
               _behavior->GetTimeSinceLastSeen() > 3000;
    }

private:
    LeaderFollowBehavior* _behavior;
};

// LeaderFollowBehavior implementation
LeaderFollowBehavior::LeaderFollowBehavior()
    : Strategy("follow")
{
    // PHASE 0 - Quick Win #2: Follow has LOW priority (50) so combat (100) always wins
    // Combat must ALWAYS override follow for proper combat behavior
    _priority = 50;
}

void LeaderFollowBehavior::InitializeActions()
{
    AddAction("follow", ::std::make_shared<FollowAction>());

    // DEADLOCK FIX: Pass 'this' pointer to StopFollowAction so it doesn't need to call GetStrategy()
    AddAction("stop follow", ::std::make_shared<StopFollowAction>(this));

    // CRITICAL FIX: Add combat assistance action
    AddAction("assist_group", ::std::make_shared<TargetAssistAction>("assist_group"));
}

void LeaderFollowBehavior::InitializeTriggers()
{
    // DEADLOCK FIX: Pass 'this' pointer to triggers so they don't need to call GetStrategy()
    AddTrigger(::std::make_shared<LeaderFarTrigger>(this));
    AddTrigger(::std::make_shared<LeaderLostTrigger>(this));

    // CRITICAL FIX: Add group combat trigger for combat assistance
    AddTrigger(::std::make_shared<GroupCombatTrigger>("group_combat"));
}

void LeaderFollowBehavior::InitializeValues()
{
    // Values will be implemented when Value system is ready
}

float LeaderFollowBehavior::GetRelevance(BotAI* ai) const
{
    if (!ai || !ai->GetBot())
        return 0.0f;

    Player* bot = ai->GetBot();

    // High relevance if in a group
    if (Group* group = bot->GetGroup())
    {
        // Don't follow if we're the leader
    if (group->GetLeaderGUID() == bot->GetGUID())
            return 0.0f;

        // CRITICAL FIX FOR ISSUES #2 & #3: ZERO relevance during combat
        // This prevents follow from interfering with combat positioning and facing
        // BehaviorPriorityManager will prioritize combat behaviors (priority 100) over follow (priority 0)
    if (bot->IsInCombat())
        {
            TC_LOG_TRACE("module.playerbot.follow",
                "Bot {} in combat - follow behavior disabled (FIX FOR ISSUE #2 & #3)",
                bot->GetName());
            return 0.0f;  // Changed from 10.0f - allows exclusive combat control
        }
        // High relevance for following group leader when not in combat
        return 100.0f;
    }

    return 0.0f;
}

void LeaderFollowBehavior::OnActivate(BotAI* ai)
{
    TC_LOG_ERROR("module.playerbot", " LeaderFollowBehavior::OnActivate() CALLED for bot {}",
                 ai && ai->GetBot() ? ai->GetBot()->GetName() : "NULL");
    TC_LOG_INFO("playerbot.debug", "=== LeaderFollowBehavior::OnActivate START ===");
    if (!ai || !ai->GetBot())
    {
        TC_LOG_ERROR("playerbot.debug", "=== LeaderFollowBehavior::OnActivate FAILED: ai or bot is NULL ===");
        return;
    }

    Player* bot = ai->GetBot();
    TC_LOG_INFO("playerbot.debug", "=== LeaderFollowBehavior::OnActivate: bot={} ===", bot->GetName());

    Group* group = bot->GetGroup();
    if (!group)
    {
        TC_LOG_ERROR("playerbot.debug", "=== LeaderFollowBehavior::OnActivate: Bot {} has NO GROUP ===", bot->GetName());
        SetActive(true);
        return;
    }

    TC_LOG_INFO("playerbot.debug", "=== LeaderFollowBehavior::OnActivate: Bot {} in group, leaderGUID={} ===",
                bot->GetName(), group->GetLeaderGUID().ToString());

    // FIX #21: Search group members directly to avoid ObjectAccessor deadlock
    // OnActivate() is called from BotAI constructor during bot initialization.
    // Calling ObjectAccessor here can cause deadlock if another thread is using it.
    // Solution: Use group->GetMembers() which doesn't require ObjectAccessor.
    Player* leader = nullptr;
    ObjectGuid leaderGuid = group->GetLeaderGUID();

    // Search group members directly (works for both real players and bots)
    for (GroupReference const& itr : group->GetMembers())
    {
        if (Player* member = itr.GetSource())
        {
            // FIX: Only use members that are fully loaded in world
    if (member->IsInWorld() && member->GetGUID() == leaderGuid)
            {
                leader = member;
                TC_LOG_INFO("playerbot.debug", "=== LeaderFollowBehavior::OnActivate: Found leader {} in group members ===",
                            member->GetName());
                break;
            }
        }
    }

    if (!leader)
    {
        TC_LOG_WARN("playerbot.debug", "=== LeaderFollowBehavior::OnActivate: Leader NOT FOUND (may not be loaded yet after server restart) - will retry on next update ===");
        // Don't return here - allow strategy to activate and retry finding leader on next UpdateFollowBehavior
        SetActive(true);
        return;
    }

    if (leader == bot)
    {
        TC_LOG_ERROR("playerbot.debug", "=== LeaderFollowBehavior::OnActivate: Bot IS the leader ===");
        SetActive(true);
        return;
    }

    TC_LOG_ERROR("module.playerbot", " LeaderFollowBehavior::OnActivate: About to call SetFollowTarget({}) for bot {}",
                 leader->GetName(), bot->GetName());
    TC_LOG_INFO("playerbot.debug", "=== LeaderFollowBehavior::OnActivate: Calling SetFollowTarget({}) ===", leader->GetName());
    SetFollowTarget(leader);
    TC_LOG_ERROR("module.playerbot", " SetFollowTarget() completed for bot {}", bot->GetName());

    _currentGroup = group;
    _formationRole = DetermineFormationRole(bot);
    // CRITICAL FIX: Calculate bot's member index in group to prevent position stacking
    uint32 memberIndex = 0;
    uint32 currentIndex = 0;
    for (GroupReference const& itr : group->GetMembers())
    {
        if (Player* member = itr.GetSource())
        {
            if (member == bot)
            {
                memberIndex = currentIndex;
                break;
            }
            currentIndex++;
        }
    }
    _groupPosition = memberIndex;

    TC_LOG_INFO("playerbot.debug", "=== LeaderFollowBehavior::OnActivate: Bot {} will follow leader {}, formationRole={}, groupPosition={} ===",
                bot->GetName(), leader->GetName(), static_cast<int>(_formationRole), _groupPosition);

    TC_LOG_INFO("playerbot.debug", "=== LeaderFollowBehavior::OnActivate: Calling SetActive(true) ===");
    SetActive(true);

    TC_LOG_INFO("playerbot.debug", "=== LeaderFollowBehavior::OnActivate SUCCESS: Activated for bot {} following leader {} ===",
                bot->GetName(), leader->GetName());
}

void LeaderFollowBehavior::OnDeactivate(BotAI* ai)
{
    ClearFollowTarget();
    _currentGroup = nullptr;
    _state = FollowState::IDLE;
    SetActive(false);

    if (ai && ai->GetBot())
    {
        StopMovement(ai->GetBot());
        TC_LOG_DEBUG("module.playerbot", "LeaderFollowBehavior deactivated for bot {}",
            ai->GetBot()->GetName());
    }
}

void LeaderFollowBehavior::UpdateBehavior(BotAI* ai, uint32 diff)
{
    // REFACTORED: This method is now called every frame from BotAI::UpdateStrategies()
    // No throttling - runs at full frame rate for smooth movement
    UpdateFollowBehavior(ai, diff);
}
void LeaderFollowBehavior::UpdateFollowBehavior(BotAI* ai, uint32 diff)
{
    // REFACTORED: Removed throttled logging - now runs every frame
    // Performance tracking will handle monitoring update frequency

    static uint32 updateCounter = 0;
    if (++updateCounter % 100 == 0)
    {
        TC_LOG_ERROR("module.playerbot", " UpdateFollowBehavior CALLED {} times", updateCounter);
    }

    if (!ai || !ai->GetBot())
    {
        TC_LOG_ERROR("module.playerbot", " UpdateFollowBehavior: NULL ai or bot");
        return;
    }

    Player* bot = ai->GetBot();

    // DEBUG LOGGING THROTTLE: Only log for test bots every 50 seconds
    static const ::std::set<::std::string> testBots = {"Anderenz", "Boone", "Nelona", "Sevtap"};
    static ::std::unordered_map<::std::string, uint32> followLogAccumulators;
    bool isTestBot = testBots.find(bot->GetName()) != testBots.end();
    bool shouldLog = false;

    if (isTestBot)
    {
        ::std::string const& botName = bot->GetName();
        // Throttle by call count (every 1000 calls ~= 50s)
        followLogAccumulators[botName]++;
        if (followLogAccumulators[botName] >= 1000)
        {
            shouldLog = true;
            followLogAccumulators[botName] = 0;
        }
    }

    if (shouldLog)
    {
        TC_LOG_ERROR("module.playerbot", " UpdateFollowBehavior: Bot {} state={}, _followTarget.player={}, guid={}",
                    bot->GetName(), static_cast<uint8>(_state),
                    (void*)_followTarget.player, _followTarget.guid.ToString());
    }

    // CRITICAL FIX: Validate leader pointer using ObjectAccessor
    // Player might have logged out, making _followTarget.player a dangling pointer
    Player* leader = nullptr;
    if (!_followTarget.guid.IsEmpty())
    {
        leader = ObjectAccessor::FindPlayer(_followTarget.guid);
    }

    if (!leader)
    {
        // Leader logged out or not found - clear follow target and stop
    if (_followTarget.player != nullptr)
        {
            TC_LOG_INFO("module.playerbot", "LeaderFollowBehavior: Leader {} not found (logged out?), stopping follow for bot {}",
                       _followTarget.guid.ToString(), bot->GetName());
            ClearFollowTarget();
            StopMovement(bot);
            SetFollowState(FollowState::IDLE);
        }
        return;
    }

    // Update cached pointer if it changed
    _followTarget.player = leader;

    auto startTime = ::std::chrono::high_resolution_clock::now();

    // CRITICAL DEBUG: Log state at entry (already throttled above via shouldLog)
    if (shouldLog)
    {
        TC_LOG_ERROR("module.playerbot", " UpdateFollowBehavior: Bot {} state={}",
                    bot->GetName(), static_cast<uint8>(_state));
    }

    // Update follow target information
    UpdateFollowTarget(bot, leader);

    // Check if we need to teleport
    if (_config.autoTeleport && ShouldTeleportToLeader(bot, leader))
    {
        if (GetTimeSince(_lastTeleport) > _config.teleportDelay)
        {
            TeleportToLeader(bot, leader);
            _lastTeleport = GameTime::GetGameTimeMS();
            _metrics.teleportCount++;
        }
    }

    // Update movement based on state
    switch (_state)
    {
        case FollowState::FOLLOWING:
            UpdateMovement(ai);
            break;
        case FollowState::CATCHING_UP:
            // CRITICAL FIX: CATCHING_UP state must continue calling UpdateMovement
            // This was the missing case causing bots to stop after first movement
            UpdateMovement(ai);
            break;
        case FollowState::COMBAT_FOLLOW:
            UpdateCombatFollowing(ai);
            break;
        case FollowState::LOST:
            HandleLostLeader(ai);
            break;
        case FollowState::POSITIONING:
            UpdateFormation(ai);
            break;
        case FollowState::WAITING:
            // Check if leader started moving
    if (_followTarget.isMoving)
                SetFollowState(FollowState::FOLLOWING);
            break;
        case FollowState::TELEPORTING:
            // During teleport, wait for completion
            // Teleport is instant, so transition back to FOLLOWING next frame
            SetFollowState(FollowState::FOLLOWING);
            break;
        case FollowState::PAUSED:
            // Paused state - do nothing until resumed
            break;
        default:
            break;
    }
    // Track performance
    auto endTime = ::std::chrono::high_resolution_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime);
    TrackPerformance(duration, "UpdateFollowBehavior");
}

bool LeaderFollowBehavior::SetFollowTarget(Player* leader)
{
    if (!leader)
        return false;

    _followTarget.guid = leader->GetGUID();
    _followTarget.player = leader;
    _followTarget.lastKnownPosition = leader->GetPosition();
    _followTarget.currentDistance = 0.0f;
    _followTarget.isMoving = leader->isMoving();
    _followTarget.inLineOfSight = true;
    _followTarget.lastSeen = GameTime::GetGameTimeMS();

    SetFollowState(FollowState::FOLLOWING);

    TC_LOG_DEBUG("module.playerbot", "Follow target set to {}", leader->GetName());
    return true;
}

void LeaderFollowBehavior::ClearFollowTarget()
{
    _followTarget = FollowTarget();
    SetFollowState(FollowState::IDLE);
    _currentPath.clear();
    _pathGenerated = false;
}
Player* LeaderFollowBehavior::GetFollowTarget() const
{
    return _followTarget.player;
}

Position LeaderFollowBehavior::CalculateFollowPosition(Player* leader, FormationRole role)
{
    if (!leader)
        return Position();

    // CRITICAL FIX: Use member-index-based positioning to prevent bot stacking
    // Calculate total group members for proper formation spreading
    uint32 totalMembers = 1;
    if (_currentGroup)
    {
        totalMembers = _currentGroup->GetMembersCount();
    }

    // Use formation-based positioning which spreads bots evenly based on their index
    // This prevents all bots of the same role from stacking on the same position
    if (totalMembers > 1)
    {
        return CalculateFormationPosition(leader, _groupPosition, totalMembers);
    }

    // Fallback to role-based positioning for solo bots
    float angle = GetRoleBasedAngle(role);
    float distance = GetRoleBasedDistance(role);

    // Adjust for formation mode
    if (_config.mode == FollowMode::FORMATION)
    {
        FollowFormationPosition formPos = GetFormationPosition(role);
        angle = formPos.angle;
        distance = formPos.distance;
    }
    else if (_config.mode == FollowMode::TIGHT)
    {
        distance = 4.0f;
    }
    else if (_config.mode == FollowMode::LOOSE)
    {
        distance = 18.0f;
    }

    return CalculateBasePosition(leader, angle, distance);
}

Position LeaderFollowBehavior::CalculateFormationPosition(Player* leader, uint32 memberIndex, uint32 totalMembers)
{
    if (!leader)
        return Position();

    float baseAngle = leader->GetOrientation();
    float angleStep = (2.0f * static_cast<float>(M_PI)) / totalMembers;
    float memberAngle = baseAngle + (angleStep * memberIndex);

    // Normalize angle
    memberAngle = NormalizeAngle(memberAngle);

    float distance = _config.mode == FollowMode::TIGHT ? 5.0f : 10.0f;

    Position pos;
    pos.m_positionX = leader->GetPositionX() + cos(memberAngle) * distance;
    pos.m_positionY = leader->GetPositionY() + sin(memberAngle) * distance;
    pos.m_positionZ = leader->GetPositionZ();
    pos.SetOrientation(leader->GetOrientation());
    // Adjust Z coordinate for terrain
    if (Map* map = leader->GetMap())
    {
        float groundZ = map->GetHeight(leader->GetPhaseShift(), pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ());
        if (groundZ > INVALID_HEIGHT)
            pos.m_positionZ = groundZ + 0.5f;
    }

    return pos;
}

bool LeaderFollowBehavior::ShouldTeleportToLeader(Player* bot, Player* leader)
{
    if (!bot || !leader)
        return false;

    // Check distance
    float distance = ::std::sqrt(bot->GetExactDistSq(leader)); // Calculate once from squared distance
    if (distance > _config.teleportDistance)
        return true;

    // Check if on different maps
    if (bot->GetMapId() != leader->GetMapId())
        return true;

    // Check if leader is in an instance we're not in
    if (leader->GetMap()->IsDungeon() && !bot->GetMap()->IsDungeon())
        return true;

    // Check line of sight for extended period
    if (!_followTarget.inLineOfSight && _followTarget.lostDuration > 5000)
        return true;

    return false;
}

bool LeaderFollowBehavior::TeleportToLeader(Player* bot, Player* leader)
{
    if (!bot || !leader)
        return false;

    // Check if we can teleport
    if (bot->IsInCombat())
        return false;

    // Calculate safe teleport position near leader
    Position teleportPos = CalculateFollowPosition(leader, _formationRole);

    // Ensure the position is safe
    if (!IsPositionSafe(teleportPos))
    {
        teleportPos = FindAlternativePosition(bot, teleportPos);
    }

    // Perform teleportation
    bot->NearTeleportTo(teleportPos.GetPositionX(), teleportPos.GetPositionY(),
                        teleportPos.GetPositionZ(), teleportPos.GetOrientation());

    SetFollowState(FollowState::POSITIONING);
    _lastTeleport = GameTime::GetGameTimeMS();

    TC_LOG_DEBUG("module.playerbot", "Bot {} teleported to leader {}",
        bot->GetName(), leader->GetName());

    return true;
}

FormationRole LeaderFollowBehavior::DetermineFormationRole(Player* bot)
{
    if (!bot)
        return FormationRole::SUPPORT;

    uint8 botClass = bot->GetClass();
    // Determine role based on class
    switch (botClass)
    {
        case CLASS_WARRIOR:
        case CLASS_PALADIN:
            // Check for tank stance
    if (bot->HasAura(71)) // Defensive Stance aura ID
                return FormationRole::TANK;
            return FormationRole::MELEE_DPS;

        case CLASS_DEATH_KNIGHT:
            // Check for tank presence (blood presence)
            return FormationRole::TANK;
        case CLASS_HUNTER:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
            return FormationRole::RANGED_DPS;

        case CLASS_PRIEST:
        case CLASS_SHAMAN:
        case CLASS_DRUID:
            // Check if healer spec
            return FormationRole::HEALER;

        case CLASS_ROGUE:
        case CLASS_MONK:
        case CLASS_DEMON_HUNTER:
            return FormationRole::MELEE_DPS;

        default:
            return FormationRole::SUPPORT;
    }
}

bool LeaderFollowBehavior::MoveToFollowPosition(BotAI* ai, const Position& targetPos)
{
    if (!ai || !ai->GetBot())
    {
        TC_LOG_ERROR("module.playerbot", " MoveToFollowPosition: NULL ai or bot");
        return false;
    }

    Player* bot = ai->GetBot();

    TC_LOG_ERROR("module.playerbot", " MoveToFollowPosition CALLED: Bot {} target=({:.2f},{:.2f},{:.2f}) state={}",
                 bot->GetName(), targetPos.GetPositionX(), targetPos.GetPositionY(), targetPos.GetPositionZ(),
                 static_cast<uint8>(_state));

    // Validate target position
    if (targetPos.GetPositionX() == 0.0f && targetPos.GetPositionY() == 0.0f)
    {
        TC_LOG_ERROR("module.playerbot", " MoveToFollowPosition: Bot {} has invalid target position (0,0,0)", bot->GetName());
        return false;
    }

    // Check if we're already close enough
    float distance = ::std::sqrt(bot->GetExactDistSq(targetPos)); // Calculate once from squared distance
    if (distance <= POSITION_TOLERANCE)
    {
        TC_LOG_ERROR("module.playerbot", " MoveToFollowPosition: Bot {} already at target (dist={:.2f})", bot->GetName(), distance);
        StopMovement(bot);
        SetFollowState(FollowState::WAITING);
        return true;
    }

    TC_LOG_ERROR("module.playerbot", " MoveToFollowPosition: Bot {} initiating movement (dist={:.2f})", bot->GetName(), distance);
    // SIMPLIFIED: Skip complex pathfinding for now, use direct movement
    // This ensures movement works reliably
    // Pathfinding can be re-enabled after basic movement is confirmed working

    TC_LOG_ERROR("module.playerbot", " MoveToFollowPosition: Bot {} using DIRECT movement", bot->GetName());

    // Start movement using StartMovement which has comprehensive error handling
    bool result = StartMovement(bot, targetPos);
    if (result)
    {
        TC_LOG_ERROR("module.playerbot", " MoveToFollowPosition: Bot {} movement initiated successfully", bot->GetName());
    }
    else
    {
        TC_LOG_ERROR("module.playerbot", " MoveToFollowPosition: Bot {} movement FAILED", bot->GetName());
    }

    return result;
}

void LeaderFollowBehavior::UpdateFollowTarget(Player* bot, Player* leader)
{
    // FIX #21: Bot is now passed as parameter instead of looking it up via ObjectAccessor
    // This eliminates another potential deadlock source during follow updates
    if (!bot || !leader)
        return;

    _followTarget.lastKnownPosition = leader->GetPosition();
    _followTarget.currentDistance = ::std::sqrt(bot->GetExactDistSq(leader)); // Calculate once from squared distance
    _followTarget.isMoving = leader->isMoving();
    _followTarget.currentSpeed = leader->GetSpeed(MOVE_RUN);

    // Check line of sight
    bool wasInSight = _followTarget.inLineOfSight;
    _followTarget.inLineOfSight = CheckLineOfSight(bot, leader);

    if (_followTarget.inLineOfSight)
    {
        _followTarget.lastSeen = GameTime::GetGameTimeMS();
        _followTarget.lostDuration = 0;
    }
    else
    {
        _followTarget.lostDuration = GetTimeSince(_followTarget.lastSeen);
        if (!wasInSight && _followTarget.lostDuration > LOST_LEADER_TIMEOUT)
        {
            SetFollowState(FollowState::LOST);
        }
    }

    // Predict future position if leader is moving
    if (_followTarget.isMoving && _usePredictiveFollowing)
    {
        _followTarget.predictedPosition = PredictLeaderPosition(leader, 1.0f);
    }
    else
    {
        _followTarget.predictedPosition = _followTarget.lastKnownPosition;
    }

    _metrics.positionUpdates++;
}

void LeaderFollowBehavior::UpdateMovement(BotAI* ai)
{
    if (!ai || !ai->GetBot() || !_followTarget.player)
        return;

    Player* bot = ai->GetBot();
    Player* leader = _followTarget.player;

    // Calculate target position
    Position targetPos = CalculateFollowPosition(leader, _formationRole);

    // Check current distance
    float currentDistance = ::std::sqrt(bot->GetExactDistSq(targetPos)); // Calculate once from squared distance

    TC_LOG_ERROR("module.playerbot", " UpdateMovement: Bot {} distance={:.2f}, min={:.2f}, max={:.2f}",
                 bot->GetName(), currentDistance, _config.minDistance, _config.maxDistance);

    // Determine if we need to move
    if (currentDistance < _config.minDistance)
    {
        // Too close, back up slightly
        TC_LOG_ERROR("module.playerbot", " UpdateMovement: Bot {} TOO CLOSE, stopping", bot->GetName());
        StopMovement(bot);
        SetFollowState(FollowState::WAITING);
    }
    else if (currentDistance > _config.maxDistance)
    {
        // Too far, need to catch up
        TC_LOG_ERROR("module.playerbot", " UpdateMovement: Bot {} TOO FAR (dist={:.2f}), catching up", bot->GetName(), currentDistance);
        // Only set state to CATCHING_UP if not already in that state
        // This prevents spamming state changes every frame
    if (_state != FollowState::CATCHING_UP)
            SetFollowState(FollowState::CATCHING_UP);

        AdjustMovementSpeed(bot, currentDistance);
        MoveToFollowPosition(ai, targetPos);
    }
    else if (currentDistance > _config.minDistance + POSITION_TOLERANCE)
    {
        // Normal following distance - transition back to FOLLOWING if we were catching up
        TC_LOG_ERROR("module.playerbot", " UpdateMovement: Bot {} NORMAL FOLLOW (dist={:.2f}), moving", bot->GetName(), currentDistance);
        // Transition from CATCHING_UP back to FOLLOWING when we're back in range
    if (_state == FollowState::CATCHING_UP)
        {
            TC_LOG_INFO("module.playerbot", "Bot {} successfully caught up, transitioning to FOLLOWING", bot->GetName());
            SetFollowState(FollowState::FOLLOWING);
        }

        MoveToFollowPosition(ai, targetPos);
    }
    else
    {
        // In position
        TC_LOG_ERROR("module.playerbot", " UpdateMovement: Bot {} IN POSITION (dist={:.2f}), waiting", bot->GetName(), currentDistance);
        StopMovement(bot);
        SetFollowState(FollowState::WAITING);
    }

    _metrics.averageDistance = (_metrics.averageDistance * 0.9f) + (currentDistance * 0.1f);
}

void LeaderFollowBehavior::UpdateFormation(BotAI* ai)
{
    if (!ai || !ai->GetBot() || !_followTarget.player)
        return;

    Player* bot = ai->GetBot();
    Player* leader = _followTarget.player;

    // Calculate formation position
    Position formationPos = CalculateFollowPosition(leader, _formationRole);

    // Move to formation position
    float distance = ::std::sqrt(bot->GetExactDistSq(formationPos)); // Calculate once from squared distance
    if (distance > POSITION_TOLERANCE * 1.5f)
    {
        MoveToFollowPosition(ai, formationPos);
        _metrics.formationAdjustments++;
    }
    else
    {
        // We're in formation
        SetFollowState(FollowState::WAITING);
    }
}

void LeaderFollowBehavior::UpdateCombatFollowing(BotAI* ai)
{
    if (!ai || !ai->GetBot() || !_followTarget.player)
        return;

    Player* bot = ai->GetBot();
    Player* leader = _followTarget.player;
    // Get current target
    ::Unit* target = bot->GetVictim();
    if (!target)
    {
        // No target, maintain formation
        UpdateMovement(ai);
        return;
    }

    // Calculate combat position
    Position combatPos = CalculateCombatPosition(bot, leader, target);

    // Move to combat position if needed
    float distance = ::std::sqrt(bot->GetExactDistSq(combatPos)); // Calculate once from squared distance
    if (distance > POSITION_TOLERANCE * 2)
    {
        MoveToFollowPosition(ai, combatPos);
    }
}

void LeaderFollowBehavior::SetFollowState(FollowState state)
{
    if (_state == state)
        return;

    FollowState oldState = _state;
    _state = state;
    HandleStateTransition(oldState, state);
}

void LeaderFollowBehavior::SetFollowMode(FollowMode mode)
{
    _config.mode = mode;

    // Adjust distances based on mode
    switch (mode)
    {
        case FollowMode::TIGHT:
            _config.minDistance = 2.0f;  // Very close for melee
            _config.maxDistance = 5.0f;   // Within melee range
            break;
        case FollowMode::NORMAL:
            _config.minDistance = 8.0f;
            _config.maxDistance = 12.0f;
            break;
        case FollowMode::LOOSE:
            _config.minDistance = 15.0f;
            _config.maxDistance = 20.0f;
            break;
        case FollowMode::FORMATION:
            _config.minDistance = 5.0f;
            _config.maxDistance = 15.0f;
            break;
        default:
            break;
    }
}

void LeaderFollowBehavior::SetFollowDistance(float min, float max)
{
    _config.minDistance = ::std::max(MIN_FOLLOW_DISTANCE, min);
    _config.maxDistance = ::std::min(MAX_FOLLOW_DISTANCE, max);
    _config.mode = FollowMode::CUSTOM;
}

bool LeaderFollowBehavior::CheckLineOfSight(Player* bot, Player* leader)
{
    if (!bot || !leader)
        return false;

    // Basic visibility check
    if (!bot->IsWithinLOSInMap(leader))
        return false;

    // Additional checks can be added here
    return true;
}

Position LeaderFollowBehavior::FindAlternativePosition(Player* bot, const Position& targetPos)
{
    if (!bot)
        return targetPos;

    // Try different angles around the target position
    float baseDistance = 5.0f;
    for (int i = 0; i < 8; ++i)
    {
        float angle = (static_cast<float>(M_PI) * 2.0f * i) / 8.0f;
        Position testPos;
        testPos.m_positionX = targetPos.GetPositionX() + cos(angle) * baseDistance;
        testPos.m_positionY = targetPos.GetPositionY() + sin(angle) * baseDistance;
        testPos.m_positionZ = targetPos.GetPositionZ();  // Initial estimate

        // CRITICAL FIX: Correct Z to actual ground level at the test position
        // The test position may be over terrain at a different Z level
        BotMovementUtil::CorrectPositionToGround(bot, testPos);

        if (IsPositionSafe(testPos))
            return testPos;
    }

    // No safe alternative found - return original with Z correction
    Position correctedTarget = targetPos;
    BotMovementUtil::CorrectPositionToGround(bot, correctedTarget);
    return correctedTarget;
}

bool LeaderFollowBehavior::IsPositionSafe(const Position& pos)
{
    // Basic safety check - can be expanded
    return pos.GetPositionX() != 0 && pos.GetPositionY() != 0;
}

bool LeaderFollowBehavior::GenerateFollowPath(Player* bot, const Position& destination)
{
    if (!bot)
        return false;

    _currentPath.clear();

    // Use PathGenerator for complex pathing
    PathGenerator path(bot);
    bool result = path.CalculatePath(destination.GetPositionX(), destination.GetPositionY(),
                                     destination.GetPositionZ());

    // Diagnostic logging for pathfinding issues
    ::PathType pathType = path.GetPathType();
    bool isShortcut = (static_cast<uint32>(pathType) & static_cast<uint32>(::PATHFIND_NOT_USING_PATH)) ||
                      (pathType == ::PATHFIND_SHORTCUT);
    if (isShortcut)
    {
        TC_LOG_WARN("module.playerbot.movement", "⚠️ Bot {} using SHORTCUT path (pathType={}) - may clip through walls! "
            "Check if MMAPs are loaded for this area. Destination: ({:.1f}, {:.1f}, {:.1f})",
            bot->GetName(), static_cast<uint32>(pathType),
            destination.GetPositionX(), destination.GetPositionY(), destination.GetPositionZ());
    }

    if (result)
    {
        _currentPath.clear();
        Movement::PointsArray const& points = path.GetPath();
        for (auto const& point : points)
        {
            Position pos;
            pos.m_positionX = point.x;
            pos.m_positionY = point.y;
            pos.m_positionZ = point.z;
            _currentPath.push_back(pos);
        }

        _pathGenerated = true;
        _needsNewPath = false;
        _lastPathGeneration = GameTime::GetGameTimeMS();
        _metrics.pathRecalculations++;

        // Optimize the path
        OptimizePath(_currentPath);
        return true;
    }

    return false;
}

void LeaderFollowBehavior::OptimizePath(::std::vector<Position>& path)
{
    if (path.size() < 3)
        return;

    // Simple path smoothing - remove unnecessary waypoints
    ::std::vector<Position> optimized;
    optimized.push_back(path[0]);

    for (size_t i = 1; i < path.size() - 1; ++i)
    {
        // Check if this waypoint is necessary
        float angle1 = ::std::atan2(path[i].GetPositionY() - path[i-1].GetPositionY(),
                                 path[i].GetPositionX() - path[i-1].GetPositionX());
        float angle2 = ::std::atan2(path[i+1].GetPositionY() - path[i].GetPositionY(),
                                 path[i+1].GetPositionX() - path[i].GetPositionX());

        float angleDiff = ::std::abs(angle1 - angle2);
        if (angleDiff > static_cast<float>(M_PI) / 6.0f) // 30 degree threshold
        {
            optimized.push_back(path[i]);
        }
    }

    optimized.push_back(path.back());
    path = optimized;
}

void LeaderFollowBehavior::HandleLostLeader(BotAI* ai)
{
    if (!ai || !ai->GetBot())
        return;

    Player* bot = ai->GetBot();

    // Try to reacquire leader
    // FIX #21: Use _followTarget.player instead of ObjectAccessor lookup
    if (_followTarget.player)
    {
        if (CheckLineOfSight(bot, _followTarget.player))
        {
            // Reacquired sight
            SetFollowState(FollowState::FOLLOWING);
            return;
        }
    }

    // Move to last known position
    if (_followTarget.lastKnownPosition.GetPositionX() != 0)
    {
        MoveToFollowPosition(ai, _followTarget.lastKnownPosition);
    }

    // Consider teleporting if lost for too long
    if (_followTarget.lostDuration > 10000 && _config.autoTeleport)
    {
        // FIX #21: Use _followTarget.player instead of ObjectAccessor lookup
    if (_followTarget.player)
        {
            TeleportToLeader(bot, _followTarget.player);
        }
    }
}

Position LeaderFollowBehavior::CalculateCombatPosition(Player* bot, Player* leader, ::Unit* target)
{
    if (!bot || !leader || !target)
        return Position();

    Position combatPos;

    // Calculate position based on role
    switch (_formationRole)
    {
        case FormationRole::TANK:
            // Stay between leader and target
            combatPos = target->GetPosition();
            break;

        case FormationRole::MELEE_DPS:
            // Behind or to the side of target
            {
                float angle = target->GetOrientation() + static_cast<float>(M_PI);
                combatPos.m_positionX = target->GetPositionX() + cos(angle) * 3.0f;
                combatPos.m_positionY = target->GetPositionY() + sin(angle) * 3.0f;
                combatPos.m_positionZ = target->GetPositionZ();  // Will be corrected below
            }
            break;

        case FormationRole::RANGED_DPS:
        case FormationRole::HEALER:
            // Stay at range from target, near leader
            {
                float angle = ::std::atan2(leader->GetPositionY() - target->GetPositionY(),
                                        leader->GetPositionX() - target->GetPositionX());
                float distance = _formationRole == FormationRole::HEALER ? 25.0f : 20.0f;
                combatPos.m_positionX = target->GetPositionX() + cos(angle) * distance;
                combatPos.m_positionY = target->GetPositionY() + sin(angle) * distance;
                combatPos.m_positionZ = target->GetPositionZ();  // Will be corrected below
            }
            break;

        default:
            // Default to following leader closely
            // NOTE: CalculateFollowPosition already handles Z correction
            combatPos = CalculateFollowPosition(leader, _formationRole);
            // Return early since Z is already corrected
            if (!IsPositionSafe(combatPos))
            {
                combatPos = FindAlternativePosition(bot, combatPos);
            }
            return combatPos;
    }

    // CRITICAL FIX: Correct Z to actual ground level at the calculated position
    // Combat positions are calculated relative to target, but the terrain at that
    // X/Y may be at a completely different Z level (cliffs, hills, ramps, etc.)
    // Without this correction, bots can fall through the ground or hover in mid-air
    BotMovementUtil::CorrectPositionToGround(bot, combatPos);

    // Ensure position is safe
    if (!IsPositionSafe(combatPos))
    {
        combatPos = FindAlternativePosition(bot, combatPos);
    }

    return combatPos;
}

Position LeaderFollowBehavior::PredictLeaderPosition(Player* leader, float timeAhead)
{
    if (!leader || !leader->isMoving())
        return leader->GetPosition();

    float speed = leader->GetSpeed(MOVE_RUN);
    float distance = speed * timeAhead;
    float orientation = leader->GetOrientation();
    Position predicted;
    predicted.m_positionX = leader->GetPositionX() + cos(orientation) * distance;
    predicted.m_positionY = leader->GetPositionY() + sin(orientation) * distance;
    predicted.m_positionZ = leader->GetPositionZ();  // Initial estimate
    predicted.SetOrientation(orientation);

    // CRITICAL FIX: Correct Z to actual ground level at the predicted position
    // Predicted positions extrapolate in X/Y but terrain Z may be different
    // This prevents bots from falling through ground when predicting over hills/cliffs
    BotMovementUtil::CorrectPositionToGround(leader, predicted);

    return predicted;
}

void LeaderFollowBehavior::AdjustMovementSpeed(Player* bot, float distanceToTarget)
{
    if (!bot)
        return;

    // Calculate speed modifier based on distance
    float speedMod = 1.0f;

    if (distanceToTarget > _config.maxDistance * 2)
    {
        speedMod = _config.catchUpSpeedBoost;
    }
    else if (distanceToTarget > _config.maxDistance)
    {
        speedMod = 1.2f;
    }

    // Apply speed modifier if changed
    if (::std::abs(_currentSpeedModifier - speedMod) > 0.01f)
    {
        _currentSpeedModifier = speedMod;
        bot->SetSpeed(MOVE_RUN, bot->GetSpeed(MOVE_RUN) * speedMod);
    }
}

FollowFormationPosition LeaderFollowBehavior::GetFormationPosition(FormationRole role)
{
    FollowFormationPosition pos;

    switch (role)
    {
        case FormationRole::TANK:
            pos.angle = 0;           // Front
            pos.distance = 5.0f;
            pos.maintainOrientation = true;
            break;

        case FormationRole::MELEE_DPS:
            pos.angle = static_cast<float>(M_PI) / 4.0f;    // 45 degrees
            pos.distance = 7.0f;
            pos.maintainOrientation = false;
            break;

        case FormationRole::RANGED_DPS:
            pos.angle = static_cast<float>(M_PI) / 2.0f;    // 90 degrees
            pos.distance = 15.0f;
            pos.maintainOrientation = false;
            break;

        case FormationRole::HEALER:
            pos.angle = static_cast<float>(M_PI);        // Behind
            pos.distance = 20.0f;
            pos.maintainOrientation = true;
            break;

        default:
            pos.angle = static_cast<float>(M_PI) * 3.0f / 4.0f; // 135 degrees
            pos.distance = 10.0f;
            pos.maintainOrientation = false;
            break;
    }

    pos.allowVariation = true;
    pos.variationRange = 2.0f;
    pos.height = 0.0f;

    return pos;
}

bool LeaderFollowBehavior::IsInPosition(float tolerance) const
{
    if (!_followTarget.player || _state != FollowState::WAITING)
        return false;

    return _followTarget.currentDistance <= tolerance;
}

// Helper methods
Position LeaderFollowBehavior::CalculateBasePosition(Player* leader, float angle, float distance)
{
    if (!leader)
        return Position();

    float leaderOrientation = leader->GetOrientation();
    float finalAngle = NormalizeAngle(leaderOrientation + angle);

    Position pos;
    pos.m_positionX = leader->GetPositionX() + cos(finalAngle) * distance;
    pos.m_positionY = leader->GetPositionY() + sin(finalAngle) * distance;
    pos.m_positionZ = leader->GetPositionZ();
    pos.SetOrientation(leaderOrientation);

    // Adjust for terrain height
    if (Map* map = leader->GetMap())
    {
        float groundZ = map->GetHeight(leader->GetPhaseShift(),
                                       pos.GetPositionX(),
                                       pos.GetPositionY(),
                                       pos.GetPositionZ());
        if (groundZ > INVALID_HEIGHT)
            pos.m_positionZ = groundZ + 0.5f;
    }

    return pos;
}

float LeaderFollowBehavior::GetRoleBasedAngle(FormationRole role)
{
    switch (role)
    {
        case FormationRole::TANK:
            return 0;
        case FormationRole::MELEE_DPS:
            return static_cast<float>(M_PI) / 6.0f;  // 30 degrees
        case FormationRole::RANGED_DPS:
            return static_cast<float>(M_PI) / 3.0f;  // 60 degrees
        case FormationRole::HEALER:
            return static_cast<float>(M_PI);      // 180 degrees
        default:
            return static_cast<float>(M_PI) / 2.0f;  // 90 degrees
    }
}

float LeaderFollowBehavior::GetRoleBasedDistance(FormationRole role) const
{
    switch (role)
    {
        case FormationRole::TANK:
            return 5.0f;
        case FormationRole::MELEE_DPS:
            return 8.0f;
        case FormationRole::RANGED_DPS:
            return 20.0f;
        case FormationRole::HEALER:
            return 25.0f;
        default:
            return 10.0f;
    }
}

float LeaderFollowBehavior::NormalizeAngle(float angle)
{
    while (angle > 2.0f * static_cast<float>(M_PI))
        angle -= 2.0f * static_cast<float>(M_PI);
    while (angle < 0)
        angle += 2.0f * static_cast<float>(M_PI);
    return angle;
}
bool LeaderFollowBehavior::StartMovement(Player* bot, const Position& destination)
{

    if (!bot->IsAlive())
    {
        TC_LOG_ERROR("module.playerbot", " StartMovement: Bot {} is dead, cannot move", bot->GetName());
        return false;
    }

    MotionMaster* motionMaster = bot->GetMotionMaster();
    if (!motionMaster)
    {
        TC_LOG_ERROR("module.playerbot", " StartMovement: Bot {} has NULL MotionMaster", bot->GetName());
        return false;
    }

    // CRITICAL FIX: Use MoveFollow for smooth following, NOT MovePoint which teleports
    // MoveFollow automatically updates position as leader moves
    Player* leader = _followTarget.player;
    if (leader)
    {
        // Follow at formation distance - use minDistance from config
        // Default is 2.0f which is optimal for melee formation
        float followDist = _config.minDistance;

        // Calculate angle based on formation role to spread bots around leader
        float followAngle = (_groupPosition * static_cast<float>(M_PI) / 4.0f); // Spread around leader in formation

        // CRITICAL FIX: Only issue MoveFollow if NOT already following
        // Re-issuing every frame causes speed-up and blinking issues
        auto currentType = motionMaster->GetCurrentMovementGeneratorType(MOTION_SLOT_ACTIVE);

        if (currentType != ::FOLLOW_MOTION_TYPE)
        {
            // If there's leftover combat movement (CHASE/POINT), clear it first
    if (currentType == CHASE_MOTION_TYPE || currentType == POINT_MOTION_TYPE)
            {
                TC_LOG_ERROR("module.playerbot", " StartMovement: Clearing leftover {} motion for bot {}",
                            static_cast<uint32>(currentType), bot->GetName());
                motionMaster->Clear();
            }

            // PHASE 5 MIGRATION: Use Movement Arbiter with FOLLOW priority (70)
            BotAI* botAI = dynamic_cast<BotAI*>(bot->GetAI());
            if (botAI && botAI->GetUnifiedMovementCoordinator())
            {
                // Create follow movement request with angle for formation
                MovementRequest req = MovementRequest::MakeFollowMovement(
                    PlayerBotMovementPriority::FOLLOW,  // Priority 70 - LOW tier
                    leader->GetGUID(),
                    followDist,
                    ChaseAngle(followAngle),  // Formation angle
                    {},  // No duration limit
                    "Following group leader in formation",
                    "LeaderFollowBehavior");

                bool accepted = botAI->GetUnifiedMovementCoordinator()->RequestMovement(req);
                if (accepted)
                {
                    TC_LOG_ERROR("module.playerbot", " StartMovement: Bot {} now following {} at {:.1f}yd, angle {:.1f}rad (was: {})",
                                bot->GetName(), leader->GetName(), followDist, followAngle, static_cast<uint32>(currentType));
                }
                else
                {
                    TC_LOG_TRACE("playerbot.movement.arbiter",
                        "LeaderFollowBehavior: Follow movement rejected for bot {} - higher priority active",
                        bot->GetName());
                }
            }
            else
            {
                // FALLBACK: Direct MotionMaster call if arbiter not available
                motionMaster->MoveFollow(leader, followDist, followAngle);
                TC_LOG_ERROR("module.playerbot", " StartMovement: Bot {} now following {} at {:.1f}yd, angle {:.1f}rad (was: {})",
                            bot->GetName(), leader->GetName(), followDist, followAngle, static_cast<uint32>(currentType));
            }
        }
        else
        {
            TC_LOG_DEBUG("module.playerbot", "⏭ StartMovement: Bot {} already following, skipping", bot->GetName());
        }

        return true;
    }

    return false;
}

void LeaderFollowBehavior::StopMovement(Player* bot)
{
    if (!bot)
        return;

    // THREAD-SAFETY FIX: MotionMaster::Clear() is NOT thread-safe!
    // It modifies unit state and splines which can cause ACCESS_VIOLATION
    // when called from worker threads (e.g., during OnGroupLeft processing).
    // Solution: Queue the stop request for main thread execution.
    WorldSession* session = bot->GetSession();
    if (BotSession* botSession = dynamic_cast<BotSession*>(session))
    {
        botSession->QueueStopMovement();
        TC_LOG_TRACE("module.playerbot.movement",
                     "StopMovement: Queued for main thread (bot {})", bot->GetName());
    }
    else
    {
        // Not a bot session (shouldn't happen) - fall back to direct call
        // This path is only for safety, normal bots always have BotSession
        bot->StopMoving();
        bot->GetMotionMaster()->Clear();
    }
}

float LeaderFollowBehavior::GetMovementSpeed(Player* bot)
{
    if (!bot)
        return 0.0f;

    return bot->GetSpeed(bot->IsFlying() ? MOVE_FLIGHT : MOVE_RUN);
}

bool LeaderFollowBehavior::IsMoving(Player* bot)
{
    return bot && bot->isMoving();
}

void LeaderFollowBehavior::HandleStateTransition(FollowState oldState, FollowState newState)
{
    TC_LOG_DEBUG("module.playerbot", "Follow state transition: {} -> {}",
        static_cast<uint32>(oldState), static_cast<uint32>(newState));

    // Handle specific transitions
    if (newState == FollowState::TELEPORTING)
    {
        _metrics.teleportCount++;
    }
    else if (newState == FollowState::LOST)
    {
        _metrics.lostLeaderEvents++;
    }
}

float LeaderFollowBehavior::CalculateDistance2D(const Position& pos1, const Position& pos2)
{
    float dx = pos1.GetPositionX() - pos2.GetPositionX();
    float dy = pos1.GetPositionY() - pos2.GetPositionY();
    return ::std::sqrt(dx * dx + dy * dy);
}

float LeaderFollowBehavior::CalculateDistance3D(const Position& pos1, const Position& pos2)
{
    float dx = pos1.GetPositionX() - pos2.GetPositionX();
    float dy = pos1.GetPositionY() - pos2.GetPositionY();
    float dz = pos1.GetPositionZ() - pos2.GetPositionZ();
    return ::std::sqrt(dx * dx + dy * dy + dz * dz);
}

bool LeaderFollowBehavior::IsWithinRange(float distance, float min, float max)
{
    return distance >= min && distance <= max;
}

uint32 LeaderFollowBehavior::GetTimeSince(uint32 timestamp)
{
    uint32 now = GameTime::GetGameTimeMS();
    return (now >= timestamp) ? (now - timestamp) : 0;
}

void LeaderFollowBehavior::TrackPerformance(::std::chrono::microseconds duration, const ::std::string& operation)
{
    if (duration > _metrics.maxUpdateTime)
        _metrics.maxUpdateTime = duration;

    // Update rolling average
    _metrics.averageUpdateTime = ::std::chrono::microseconds(
        (_metrics.averageUpdateTime.count() * 9 + duration.count()) / 10);
}

// Factory implementations
::std::unique_ptr<LeaderFollowBehavior> FollowBehaviorFactory::CreateFollowBehavior(FollowMode mode)
{
    auto behavior = ::std::make_unique<LeaderFollowBehavior>();
    behavior->SetFollowMode(mode);
    return behavior;
}

::std::unique_ptr<LeaderFollowBehavior> FollowBehaviorFactory::CreateRoleBasedFollowBehavior(FormationRole role)
{
    auto behavior = ::std::make_unique<LeaderFollowBehavior>();
    behavior->SetFollowMode(FollowMode::FORMATION);
    // Role will be used in position calculations
    return behavior;
}

::std::unique_ptr<LeaderFollowBehavior> FollowBehaviorFactory::CreateCombatFollowBehavior()
{
    auto behavior = ::std::make_unique<LeaderFollowBehavior>();
    behavior->GetConfig().followInCombat = true;
    behavior->GetConfig().minDistance = 10.0f;
    behavior->GetConfig().maxDistance = 20.0f;
    return behavior;
}

// Utility function implementations
float FollowBehaviorUtils::CalculateOptimalFollowDistance(Player* bot, Player* leader, FollowMode mode)
{
    if (!bot || !leader)
        return 10.0f;

    float baseDistance = 10.0f;

    switch (mode)
    {
        case FollowMode::TIGHT:
            baseDistance = 4.0f;
            break;
        case FollowMode::NORMAL:
            baseDistance = 10.0f;
            break;
        case FollowMode::LOOSE:
            baseDistance = 18.0f;
            break;
        case FollowMode::FORMATION:
            // Calculate based on bot's class
    switch (bot->GetClass())
            {
                case CLASS_WARRIOR:
                case CLASS_PALADIN:
                case CLASS_DEATH_KNIGHT:
                    baseDistance = 5.0f;
                    break;
                case CLASS_HUNTER:
                case CLASS_MAGE:
                case CLASS_WARLOCK:
                case CLASS_PRIEST:
                    baseDistance = 20.0f;
                    break;
                default:
                    baseDistance = 10.0f;
                    break;
            }
            break;
        default:
            break;
    }

    // Adjust for combat
    if (bot->IsInCombat())
        baseDistance *= 1.5f;

    return baseDistance;
}

bool FollowBehaviorUtils::IsInFollowRange(Player* bot, Player* leader, float minDist, float maxDist)
{
    if (!bot || !leader)
        return false;

    float distance = ::std::sqrt(bot->GetExactDistSq(leader)); // Calculate once from squared distance
    return distance >= minDist && distance <= maxDist;
}

FormationRole FollowBehaviorUtils::GetOptimalFormationRole(Player* bot)
{
    if (!bot)
        return FormationRole::SUPPORT;

    // Simplified role determination based on class
    uint8 botClass = bot->GetClass();

    switch (botClass)
    {
        case CLASS_WARRIOR:
        case CLASS_PALADIN:
        case CLASS_DEATH_KNIGHT:
            return FormationRole::TANK;

        case CLASS_ROGUE:
        case CLASS_MONK:
        case CLASS_DEMON_HUNTER:
            return FormationRole::MELEE_DPS;

        case CLASS_HUNTER:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
            return FormationRole::RANGED_DPS;

        case CLASS_PRIEST:
        case CLASS_DRUID:
        case CLASS_SHAMAN:
            return FormationRole::HEALER;

        default:
            return FormationRole::SUPPORT;
    }
}

Position FollowBehaviorUtils::PredictMovement(::Unit* unit, float timeAhead)
{
    if (!unit || !unit->isMoving())
        return unit->GetPosition();

    float speed = unit->GetSpeed(MOVE_RUN);
    float distance = speed * timeAhead;
    float orientation = unit->GetOrientation();
    Position predicted;
    predicted.m_positionX = unit->GetPositionX() + cos(orientation) * distance;
    predicted.m_positionY = unit->GetPositionY() + sin(orientation) * distance;
    predicted.m_positionZ = unit->GetPositionZ();  // Initial estimate
    predicted.SetOrientation(orientation);

    // CRITICAL FIX: Correct Z to actual ground level at the predicted position
    // Predicted positions extrapolate in X/Y but terrain Z may be different
    // Use FindMap() to avoid ASSERT crash
    if (Map* map = unit->FindMap())
    {
        BotMovementUtil::CorrectPositionToGroundWithMap(map, unit->GetPhaseShift(), predicted);
    }

    return predicted;
}

} // namespace Playerbot