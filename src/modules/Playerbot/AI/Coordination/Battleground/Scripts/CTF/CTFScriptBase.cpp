/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2021+ WarheadCore <https://github.com/AzerothCore/WarheadCore>
 * Copyright (C) 2025+ TrinityCore Playerbot Integration
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "CTFScriptBase.h"
#include "BattlegroundCoordinator.h"
#include "BattlegroundCoordinatorManager.h"
#include "BGSpatialQueryCache.h"
#include "BotActionManager.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "GameObject.h"
#include "GameObjectData.h"
#include "SharedDefines.h"
#include "SpellAuras.h"
#include "Log.h"
#include "Timer.h"
#include "../../Movement/BotMovementUtil.h"
#include <cmath>

namespace Playerbot::Coordination::Battleground
{

// ============================================================================
// LIFECYCLE
// ============================================================================

void CTFScriptBase::OnLoad(BattlegroundCoordinator* coordinator)
{
    BGScriptBase::OnLoad(coordinator);

    // Reset CTF state
    m_allianceFlagTaken = false;
    m_hordeFlagTaken = false;
    m_allianceFC.Clear();
    m_hordeFC.Clear();
    m_allianceFlagPickupTime = 0;
    m_hordeFlagPickupTime = 0;

    m_allianceCaptures = 0;
    m_hordeCaptures = 0;

    m_isOvertime = false;
    m_overtimeStartTime = 0;

    m_successfulCaptures = 0;
    m_failedCaptures = 0;
    m_flagReturns = 0;

    m_debuffCheckTimer = 0;

    TC_LOG_DEBUG("playerbots.bg.script", "CTFScriptBase: Initialized CTF state for {}",
        GetName());
}

void CTFScriptBase::OnUpdate(uint32 diff)
{
    BGScriptBase::OnUpdate(diff);

    if (!IsMatchActive())
        return;

    // Periodic debuff stack check
    m_debuffCheckTimer += diff;
    if (m_debuffCheckTimer >= DEBUFF_CHECK_INTERVAL)
    {
        m_debuffCheckTimer = 0;

        // Check for overtime condition
        if (!m_isOvertime && GetElapsedTime() >= FOCUSED_ASSAULT_START)
        {
            m_isOvertime = true;
            m_overtimeStartTime = getMSTime();
            TC_LOG_DEBUG("playerbots.bg.script",
                "CTFScriptBase: Overtime started - debuffs active");
        }
    }
}

// ============================================================================
// CTF-SPECIFIC IMPLEMENTATIONS
// ============================================================================

std::vector<Position> CTFScriptBase::GetEscortFormation(
    const Position& fcPos, uint8 escortCount) const
{
    if (escortCount == 0)
        return {};

    // Create a ring formation around the FC
    return CalculateEscortRing(fcPos, fcPos.GetOrientation(),
        escortCount, CTFConstants::ESCORT_RING_RADIUS);
}

std::vector<Position> CTFScriptBase::GetFlagRoomPositions(uint32 faction) const
{
    if (faction == ALLIANCE)
        return GetAllianceFlagRoomDefense();
    else
        return GetHordeFlagRoomDefense();
}

uint32 CTFScriptBase::GetFlagDebuffSpellId(uint8 stackCount) const
{
    // Brutal Assault replaces Focused Assault at higher stacks
    if (stackCount >= 10)
        return CTFSpells::BRUTAL_ASSAULT;
    return CTFSpells::FOCUSED_ASSAULT;
}

// ============================================================================
// STRATEGY - CTF OVERRIDES
// ============================================================================

RoleDistribution CTFScriptBase::GetRecommendedRoles(
    const StrategicDecision& decision,
    float scoreAdvantage,
    uint32 timeRemaining) const
{
    bool weHaveFlag = m_coordinator ?
        m_coordinator->HasFlag(m_coordinator->GetFriendlyFC()) : false;
    bool theyHaveFlag = m_coordinator ?
        !m_coordinator->GetEnemyFC().IsEmpty() : false;

    RoleDistribution dist = CreateCTFRoleDistribution(
        decision, weHaveFlag, theyHaveFlag, GetTeamSize());

    // Adjust based on score
    int32 scoreDiff = GetScoreDifference();

    // Critical: One cap from winning/losing
    if (std::abs(scoreDiff) >= 2)
    {
        if (scoreDiff > 0) // We're winning
        {
            // Heavily defend - just don't let them cap
            dist.SetRole(BGRole::NODE_DEFENDER, 4, 6);
            dist.SetRole(BGRole::FLAG_HUNTER, 4, 5);
            dist.reasoning = "One cap from victory - maximum defense";
        }
        else // We're losing
        {
            // All-in offense to get flag caps
            dist.SetRole(BGRole::FLAG_HUNTER, 5, 7);
            dist.SetRole(BGRole::NODE_DEFENDER, 1, 2);
            dist.reasoning = "Desperate comeback - heavy offense";
        }
    }

    // Time pressure adjustments
    if (timeRemaining < 300000) // Less than 5 minutes
    {
        uint8 escortCount = GetRecommendedEscortCount(weHaveFlag, theyHaveFlag,
            timeRemaining, scoreDiff);
        uint8 hunterCount = GetRecommendedHunterCount(weHaveFlag, theyHaveFlag,
            timeRemaining, scoreDiff);

        if (weHaveFlag)
            dist.SetRole(BGRole::FLAG_ESCORT, escortCount, escortCount + 2);
        if (theyHaveFlag)
            dist.SetRole(BGRole::FLAG_HUNTER, hunterCount, hunterCount + 2);

        dist.reasoning += " (time pressure adjustments)";
    }

    return dist;
}

void CTFScriptBase::AdjustStrategy(StrategicDecision& decision,
    float scoreAdvantage, uint32 /*controlledCount*/,
    uint32 /*totalObjectives*/, uint32 timeRemaining) const
{
    int32 scoreDiff = GetScoreDifference();
    bool isStandoff = IsStandoff();

    // Score-based strategy
    if (scoreDiff >= 2)
    {
        // Up by 2 - turtle and protect
        decision.strategy = BGStrategy::DEFENSIVE;
        decision.reasoning = "Up by 2 caps - protect flag room";
        decision.defenseAllocation = 70;
        decision.offenseAllocation = 30;
    }
    else if (scoreDiff <= -2)
    {
        // Down by 2 - must be aggressive
        decision.strategy = timeRemaining < 300000 ?
            BGStrategy::ALL_IN : BGStrategy::AGGRESSIVE;
        decision.reasoning = "Down by 2 caps - aggressive hunting";
        decision.offenseAllocation = 80;
        decision.defenseAllocation = 20;
    }
    else if (isStandoff)
    {
        // Both flags taken - handle standoff
        if (m_isOvertime)
        {
            // Debuffs active - whoever has more stacks should push
            uint8 ourStacks = CalculateDebuffStacks(GetFlagHoldTime(true));
            uint8 theirStacks = CalculateDebuffStacks(GetFlagHoldTime(false));

            if (ourStacks > theirStacks)
            {
                decision.strategy = BGStrategy::AGGRESSIVE;
                decision.reasoning = "Standoff - we have more debuff stacks, push!";
                decision.offenseAllocation = 60;
            }
            else
            {
                decision.strategy = BGStrategy::DEFENSIVE;
                decision.reasoning = "Standoff - they have more stacks, turtle";
                decision.defenseAllocation = 60;
            }
        }
        else
        {
            decision.strategy = BGStrategy::BALANCED;
            decision.reasoning = "Standoff - balanced approach";
            decision.offenseAllocation = 50;
            decision.defenseAllocation = 50;
        }
    }
    else
    {
        // Close game, no standoff
        if (scoreAdvantage > 0)
        {
            decision.strategy = BGStrategy::DEFENSIVE;
            decision.reasoning = "Leading - controlled defense";
            decision.defenseAllocation = 55;
        }
        else
        {
            decision.strategy = BGStrategy::AGGRESSIVE;
            decision.reasoning = "Behind - need flag captures";
            decision.offenseAllocation = 60;
        }
    }

    // Time pressure override
    if (timeRemaining < 60000)
    {
        if (scoreDiff < 0)
        {
            decision.strategy = BGStrategy::ALL_IN;
            decision.reasoning = "Last minute, behind - all in!";
            decision.offenseAllocation = 90;
            decision.defenseAllocation = 10;
        }
        else if (scoreDiff > 0)
        {
            decision.strategy = BGStrategy::TURTLE;
            decision.reasoning = "Last minute, winning - turtle";
            decision.defenseAllocation = 90;
            decision.offenseAllocation = 10;
        }
    }

    decision.confidence = 0.7f + std::abs(scoreAdvantage) * 0.2f;
}

uint8 CTFScriptBase::GetObjectiveAttackPriority(uint32 objectiveId,
    BGObjectiveState state, uint32 faction) const
{
    // In CTF, "objectives" are the flags
    // High priority on enemy flag when not taken
    if (state == BGObjectiveState::NEUTRAL)
    {
        // Our flag room - priority depends on if we need to return
        return 5;
    }

    if ((faction == ALLIANCE && state == BGObjectiveState::HORDE_CONTROLLED) ||
        (faction == HORDE && state == BGObjectiveState::ALLIANCE_CONTROLLED))
    {
        // Enemy controls their flag = it's at their base = go get it
        return 10;
    }

    return BGScriptBase::GetObjectiveAttackPriority(objectiveId, state, faction);
}

uint8 CTFScriptBase::GetObjectiveDefensePriority(uint32 objectiveId,
    BGObjectiveState state, uint32 faction) const
{
    // Defend our flag room
    if ((faction == ALLIANCE && state == BGObjectiveState::ALLIANCE_CONTROLLED) ||
        (faction == HORDE && state == BGObjectiveState::HORDE_CONTROLLED))
    {
        // Our flag at base - need defense
        bool theyHaveFlag = !m_hordeFC.IsEmpty() && faction == ALLIANCE ||
                           !m_allianceFC.IsEmpty() && faction == HORDE;

        // Higher priority if they have our flag (can't cap without it!)
        return theyHaveFlag ? 8 : 6;
    }

    return BGScriptBase::GetObjectiveDefensePriority(objectiveId, state, faction);
}

float CTFScriptBase::CalculateWinProbability(uint32 allianceScore, uint32 hordeScore,
    uint32 timeRemaining, uint32 /*objectivesControlled*/, uint32 faction) const
{
    uint32 ourScore = (faction == ALLIANCE) ? allianceScore : hordeScore;
    uint32 theirScore = (faction == ALLIANCE) ? hordeScore : allianceScore;

    // Base probability from score
    float scoreProbability = 0.5f;
    if (ourScore > theirScore)
        scoreProbability = 0.5f + (ourScore - theirScore) * 0.15f;
    else if (theirScore > ourScore)
        scoreProbability = 0.5f - (theirScore - ourScore) * 0.15f;

    // Flag possession factor
    bool weHaveFlag = (faction == ALLIANCE && !m_hordeFC.IsEmpty()) ||
                     (faction == HORDE && !m_allianceFC.IsEmpty());
    bool theyHaveFlag = (faction == ALLIANCE && !m_allianceFC.IsEmpty()) ||
                       (faction == HORDE && !m_hordeFC.IsEmpty());

    float flagFactor = 0.0f;
    if (weHaveFlag && !theyHaveFlag)
        flagFactor = 0.15f;  // We can cap, they can't
    else if (!weHaveFlag && theyHaveFlag)
        flagFactor = -0.15f; // They can cap, we can't
    else if (weHaveFlag && theyHaveFlag)
    {
        // Standoff - debuff comparison
        if (m_isOvertime)
        {
            uint8 ourStacks = CalculateDebuffStacks(GetFlagHoldTime(faction == ALLIANCE));
            uint8 theirStacks = CalculateDebuffStacks(GetFlagHoldTime(faction != ALLIANCE));
            flagFactor = (theirStacks - ourStacks) * 0.02f; // More stacks = weaker
        }
    }

    // Time factor - less time = current score matters more
    float timeFactor = 1.0f - (static_cast<float>(timeRemaining) / GetMaxDuration());
    float finalProbability = scoreProbability + flagFactor * (1.0f - timeFactor * 0.3f);

    return std::clamp(finalProbability, 0.05f, 0.95f);
}

// ============================================================================
// EVENT HANDLING
// ============================================================================

void CTFScriptBase::OnEvent(const BGScriptEventData& event)
{
    BGScriptBase::OnEvent(event);

    switch (event.eventType)
    {
        case BGScriptEvent::FLAG_PICKED_UP:
            if (event.faction == ALLIANCE)
            {
                m_allianceFlagTaken = true;
                m_allianceFC = event.primaryGuid;
                m_allianceFlagPickupTime = getMSTime();
            }
            else
            {
                m_hordeFlagTaken = true;
                m_hordeFC = event.primaryGuid;
                m_hordeFlagPickupTime = getMSTime();
            }
            TC_LOG_DEBUG("playerbots.bg.script", "CTF: Flag picked up by {} (faction {})",
                event.primaryGuid.ToString(), event.faction);
            break;

        case BGScriptEvent::FLAG_DROPPED:
            if (event.faction == ALLIANCE)
            {
                m_allianceFC.Clear();
            }
            else
            {
                m_hordeFC.Clear();
            }
            TC_LOG_DEBUG("playerbots.bg.script", "CTF: Flag dropped at ({}, {}, {})",
                event.x, event.y, event.z);
            ++m_failedCaptures;
            break;

        case BGScriptEvent::FLAG_CAPTURED:
            if (event.faction == ALLIANCE)
            {
                m_allianceFlagTaken = false;
                m_allianceFC.Clear();
                ++m_allianceCaptures;
            }
            else
            {
                m_hordeFlagTaken = false;
                m_hordeFC.Clear();
                ++m_hordeCaptures;
            }
            ++m_successfulCaptures;
            TC_LOG_DEBUG("playerbots.bg.script", "CTF: Flag captured! Score: A{} - H{}",
                m_allianceCaptures.load(), m_hordeCaptures.load());
            break;

        case BGScriptEvent::FLAG_RETURNED:
            if (event.faction == ALLIANCE)
            {
                m_hordeFlagTaken = false;  // Alliance returned their flag
                m_hordeFC.Clear();
            }
            else
            {
                m_allianceFlagTaken = false;  // Horde returned their flag
                m_allianceFC.Clear();
            }
            ++m_flagReturns;
            TC_LOG_DEBUG("playerbots.bg.script", "CTF: Flag returned");
            break;

        case BGScriptEvent::FLAG_RESET:
            // Both flags reset to base
            m_allianceFlagTaken = false;
            m_hordeFlagTaken = false;
            m_allianceFC.Clear();
            m_hordeFC.Clear();
            TC_LOG_DEBUG("playerbots.bg.script", "CTF: Flags reset to base");
            break;

        default:
            break;
    }
}

void CTFScriptBase::OnMatchStart()
{
    BGScriptBase::OnMatchStart();

    // Reset CTF-specific state
    m_allianceFlagTaken = false;
    m_hordeFlagTaken = false;
    m_allianceFC.Clear();
    m_hordeFC.Clear();
    m_allianceFlagPickupTime = 0;
    m_hordeFlagPickupTime = 0;
    m_allianceCaptures = 0;
    m_hordeCaptures = 0;
    m_isOvertime = false;
    m_overtimeStartTime = 0;

    TC_LOG_DEBUG("playerbots.bg.script", "CTF: Match started");
}

// ============================================================================
// CTF HELPERS
// ============================================================================

bool CTFScriptBase::IsStandoff() const
{
    return m_allianceFlagTaken && m_hordeFlagTaken;
}

int32 CTFScriptBase::GetScoreDifference() const
{
    if (!m_coordinator)
        return 0;

    const auto& score = m_coordinator->GetScore();
    uint32 ourFaction = m_coordinator->GetFaction();

    if (ourFaction == ALLIANCE)
        return static_cast<int32>(score.allianceFlagCaptures) -
               static_cast<int32>(score.hordeFlagCaptures);
    else
        return static_cast<int32>(score.hordeFlagCaptures) -
               static_cast<int32>(score.allianceFlagCaptures);
}

uint32 CTFScriptBase::GetFlagHoldTime(bool isFriendly) const
{
    if (!m_coordinator)
        return 0;

    uint32 ourFaction = m_coordinator->GetFaction();
    uint32 pickupTime;

    if (isFriendly)
    {
        // Friendly FC = we have THEIR flag
        pickupTime = (ourFaction == ALLIANCE) ? m_allianceFlagPickupTime : m_hordeFlagPickupTime;
    }
    else
    {
        // Enemy FC = they have OUR flag
        pickupTime = (ourFaction == ALLIANCE) ? m_hordeFlagPickupTime : m_allianceFlagPickupTime;
    }

    if (pickupTime == 0)
        return 0;

    return getMSTime() - pickupTime;
}

uint8 CTFScriptBase::CalculateDebuffStacks(uint32 holdTime) const
{
    if (holdTime < FOCUSED_ASSAULT_START)
        return 0;

    // Stacks increase over time
    uint32 overtimeMs = holdTime - FOCUSED_ASSAULT_START;
    uint8 stacks = static_cast<uint8>(overtimeMs / 60000); // 1 stack per minute

    // Cap at reasonable amount
    return std::min(stacks, static_cast<uint8>(15));
}

uint8 CTFScriptBase::GetRecommendedEscortCount(bool weHaveFlag, bool theyHaveFlag,
    uint32 timeRemaining, int32 scoreDiff) const
{
    if (!weHaveFlag)
        return 0;

    uint8 base = 3;

    // More escorts if we're ahead and need to protect the cap
    if (scoreDiff >= 2)
        base = 5;
    else if (scoreDiff <= -2)
        base = 2;  // Sacrifice escorts for hunters

    // Standoff adjustments
    if (theyHaveFlag)
    {
        if (m_isOvertime)
        {
            // Need to protect FC from dying to debuff damage
            base = std::min(static_cast<uint8>(6), static_cast<uint8>(base + 1));
        }
    }

    // Time pressure
    if (timeRemaining < 120000)
    {
        if (scoreDiff < 0)
            base = std::max(static_cast<uint8>(2), static_cast<uint8>(base - 1));
    }

    return std::clamp(base, CTFConstants::MIN_ESCORTS, CTFConstants::MAX_ESCORTS);
}

uint8 CTFScriptBase::GetRecommendedHunterCount(bool weHaveFlag, bool theyHaveFlag,
    uint32 timeRemaining, int32 scoreDiff) const
{
    if (!theyHaveFlag)
        return weHaveFlag ? 1 : 4;  // If no flag taken, go grab one

    uint8 base = 3;

    // More hunters if we're behind
    if (scoreDiff <= -1)
        base = 4;
    if (scoreDiff <= -2)
        base = 5;

    // Less hunters if we're comfortably ahead
    if (scoreDiff >= 2)
        base = 2;

    // Time pressure - need to get flag back
    if (timeRemaining < 120000 && scoreDiff < 0)
        base = 5;

    return std::clamp(base, CTFConstants::MIN_HUNTERS, CTFConstants::MAX_HUNTERS);
}

CTFScriptBase::FCTactic CTFScriptBase::GetFCTactic(bool standoff, uint8 debuffStacks,
    uint8 escortCount, uint32 timeRemaining) const
{
    // High debuff stacks - need to cap quickly or die
    if (debuffStacks >= 10)
        return FCTactic::AGGRESSIVE_PUSH;

    // Standoff situation
    if (standoff)
    {
        // With good escort, kite middle to draw out enemy
        if (escortCount >= 4)
            return FCTactic::KITE_MIDDLE;
        // Low escort - hide in base until support arrives
        return FCTactic::HIDE_BASE;
    }

    // No standoff
    if (escortCount >= 3)
    {
        // Good escort - run home for cap
        return FCTactic::RUN_HOME;
    }

    // Low escort - hide until support
    return FCTactic::HIDE_BASE;
}

std::vector<Position> CTFScriptBase::CalculateEscortRing(
    const Position& center, float heading,
    uint8 count, float radius) const
{
    std::vector<Position> positions;
    positions.reserve(count);

    if (count == 0)
        return positions;

    // Distribute escorts evenly in a ring
    float angleStep = (2.0f * M_PI) / count;

    // First escort directly behind FC
    float startAngle = heading + M_PI;

    for (uint8 i = 0; i < count; ++i)
    {
        float angle = startAngle + (i * angleStep);

        // Adjust radius for healers (further back)
        float adjustedRadius = radius;
        if (i == count - 1 || i == count - 2)  // Last two positions for healers
            adjustedRadius = CTFConstants::ESCORT_HEALER_OFFSET;

        float x = center.GetPositionX() + adjustedRadius * std::cos(angle);
        float y = center.GetPositionY() + adjustedRadius * std::sin(angle);
        float z = center.GetPositionZ();

        // Face toward the FC
        float o = angle + M_PI;

        positions.emplace_back(x, y, z, o);
    }

    return positions;
}

// ============================================================================
// RUNTIME BEHAVIOR METHODS
// ============================================================================

void CTFScriptBase::RefreshFlagState(::Player* bot)
{
    uint32 now = getMSTime();
    if (now - m_lastFlagStateRefresh.load(std::memory_order_relaxed) < FLAG_STATE_REFRESH_INTERVAL)
        return;

    m_lastFlagStateRefresh.store(now, std::memory_order_relaxed);

    ::Player* friendlyFC = nullptr;
    ::Player* enemyFC = nullptr;

    if (!bot || !bot->IsInWorld())
    {
        std::unique_lock lock(m_flagStateMutex);
        m_cachedFriendlyFC = nullptr;
        m_cachedEnemyFC = nullptr;
        return;
    }

    // Try coordinator cache first (O(1))
    ::Playerbot::BattlegroundCoordinator* coordinator =
        sBGCoordinatorMgr->GetCoordinatorForPlayer(bot);

    if (coordinator)
    {
        ObjectGuid friendlyGuid = coordinator->GetCachedFriendlyFC();
        if (!friendlyGuid.IsEmpty())
        {
            ::Player* fc = ObjectAccessor::FindPlayer(friendlyGuid);
            if (fc && fc->IsInWorld() && fc->IsAlive())
                friendlyFC = fc;
        }

        ObjectGuid enemyGuid = coordinator->GetCachedEnemyFC();
        if (!enemyGuid.IsEmpty())
        {
            ::Player* fc = ObjectAccessor::FindPlayer(enemyGuid);
            if (fc && fc->IsInWorld() && fc->IsAlive())
                enemyFC = fc;
        }

        std::unique_lock lock(m_flagStateMutex);
        m_cachedFriendlyFC = friendlyFC;
        m_cachedEnemyFC = enemyFC;
        return;
    }

    // Fallback: O(n) aura scan over all BG players
    ::Battleground* bg = bot->GetBattleground();
    if (!bg)
    {
        std::unique_lock lock(m_flagStateMutex);
        m_cachedFriendlyFC = nullptr;
        m_cachedEnemyFC = nullptr;
        return;
    }

    uint32 teamId = bot->GetBGTeam();
    // Friendly FC carries the enemy's flag aura
    uint32 friendlyFCAura = (teamId == ALLIANCE) ? CTFSpells::ALLIANCE_FLAG_CARRIED : CTFSpells::HORDE_FLAG_CARRIED;
    // Enemy FC carries our flag aura
    uint32 enemyFCAura = (teamId == ALLIANCE) ? CTFSpells::HORDE_FLAG_CARRIED : CTFSpells::ALLIANCE_FLAG_CARRIED;

    for (auto const& itr : bg->GetPlayers())
    {
        ::Player* bgPlayer = ObjectAccessor::FindPlayer(itr.first);
        if (!bgPlayer || !bgPlayer->IsInWorld() || !bgPlayer->IsAlive())
            continue;

        if (bgPlayer->GetBGTeam() == teamId && bgPlayer->HasAura(friendlyFCAura))
            friendlyFC = bgPlayer;
        else if (bgPlayer->GetBGTeam() != teamId && bgPlayer->HasAura(enemyFCAura))
            enemyFC = bgPlayer;

        // Early out if both found
        if (friendlyFC && enemyFC)
            break;
    }

    std::unique_lock lock(m_flagStateMutex);
    m_cachedFriendlyFC = friendlyFC;
    m_cachedEnemyFC = enemyFC;
}

bool CTFScriptBase::IsPlayerCarryingFlag(::Player* player)
{
    if (!player)
        return false;
    return player->HasAura(CTFSpells::ALLIANCE_FLAG_CARRIED) ||
           player->HasAura(CTFSpells::HORDE_FLAG_CARRIED);
}

bool CTFScriptBase::RunFlagHome(::Player* bot)
{
    if (!bot || !bot->IsInWorld())
        return false;

    uint32 teamId = bot->GetBGTeam();

    // Our capture point is our own flag room
    Position capturePoint = (teamId == ALLIANCE)
        ? GetAllianceFlagPosition()
        : GetHordeFlagPosition();

    float distToCapture = bot->GetExactDist(&capturePoint);

    if (distToCapture <= 10.0f)
    {
        // At capture point - interact with flag stand to cap
        TryInteractWithGameObject(bot, GAMEOBJECT_TYPE_FLAGSTAND, 10.0f);
        // Clear route state on arrival
        m_fcRouteStates.erase(bot->GetGUID());
        return true;
    }

    // === Route Evasion System ===
    // Select or continue a route for this FC
    ObjectGuid botGuid = bot->GetGUID();
    auto routeIt = m_fcRouteStates.find(botGuid);

    if (routeIt == m_fcRouteStates.end())
    {
        // First time — select a route based on enemy positions
        std::vector<Position> enemyPositions;

        // Gather enemy positions from coordinator spatial cache (thread-safe)
        if (m_coordinator)
        {
            // Sample enemy positions from mid-map area with large range
            Position midPoint;
            midPoint.Relocate(
                (GetAllianceFlagPosition().GetPositionX() + GetHordeFlagPosition().GetPositionX()) / 2.0f,
                (GetAllianceFlagPosition().GetPositionY() + GetHordeFlagPosition().GetPositionY()) / 2.0f,
                (GetAllianceFlagPosition().GetPositionZ() + GetHordeFlagPosition().GetPositionZ()) / 2.0f);

            auto enemies = m_coordinator->QueryNearbyEnemies(midPoint, 200.0f, teamId);
            for (const auto* snapshot : enemies)
            {
                if (snapshot && snapshot->isAlive)
                    enemyPositions.push_back(snapshot->position);
            }
        }

        // Ask the derived script for route waypoints
        auto waypoints = GetFCRouteWaypoints(teamId, enemyPositions);

        if (!waypoints.empty())
        {
            FCRouteState state;
            state.waypoints = std::move(waypoints);
            state.currentWaypointIndex = 0;
            state.routeSelectedTime = getMSTime();
            m_fcRouteStates[botGuid] = std::move(state);
            routeIt = m_fcRouteStates.find(botGuid);

            TC_LOG_DEBUG("playerbots.bg.script", "CTF FC: {} selected route with {} waypoints",
                bot->GetName(), routeIt->second.waypoints.size());
        }
    }

    // Follow waypoints if we have a route, otherwise straight-line
    if (routeIt != m_fcRouteStates.end() && !routeIt->second.waypoints.empty())
    {
        auto& state = routeIt->second;
        const auto& waypoints = state.waypoints;

        // Advance waypoint if close enough to current one
        if (state.currentWaypointIndex < waypoints.size())
        {
            float distToWaypoint = bot->GetExactDist(&waypoints[state.currentWaypointIndex]);
            if (distToWaypoint < 5.0f)
            {
                state.currentWaypointIndex++;
            }
        }

        // Move to current waypoint, or to capture point if all waypoints passed
        if (state.currentWaypointIndex < waypoints.size())
        {
            BotMovementUtil::MoveToPosition(bot, waypoints[state.currentWaypointIndex]);
            TC_LOG_DEBUG("playerbots.bg.script", "CTF FC: {} following route waypoint {}/{} (dist: {:.1f})",
                bot->GetName(), state.currentWaypointIndex + 1, waypoints.size(),
                bot->GetExactDist(&waypoints[state.currentWaypointIndex]));
        }
        else
        {
            // Past all waypoints, head straight to capture point
            BotMovementUtil::MoveToPosition(bot, capturePoint);
        }
    }
    else
    {
        // No route available — straight-line run
        BotMovementUtil::MoveToPosition(bot, capturePoint);
    }

    TC_LOG_DEBUG("playerbots.bg.script", "CTF FC: {} running flag home (dist: {:.1f})",
        bot->GetName(), distToCapture);

    // Attack enemies en route but never stop
    ::Player* nearEnemy = FindNearestEnemyPlayer(bot, 8.0f);
    if (nearEnemy)
        EngageTarget(bot, nearEnemy);

    return true;
}

bool CTFScriptBase::PickupEnemyFlag(::Player* bot)
{
    if (!bot || !bot->IsInWorld())
        return false;

    uint32 teamId = bot->GetBGTeam();

    // Enemy flag is at THEIR base
    Position enemyFlagPos = (teamId == ALLIANCE)
        ? GetHordeFlagPosition()
        : GetAllianceFlagPosition();

    float distance = bot->GetExactDist(&enemyFlagPos);

    TC_LOG_DEBUG("playerbots.bg.script", "CTF: {} going to pick up enemy flag (dist: {:.1f})",
        bot->GetName(), distance);

    if (distance > 10.0f)
    {
        BotMovementUtil::MoveToPosition(bot, enemyFlagPos);
    }
    else
    {
        // Try flag stand first, then goober (different GO types for different BG versions)
        if (!TryInteractWithGameObject(bot, GAMEOBJECT_TYPE_FLAGSTAND, 10.0f))
            TryInteractWithGameObject(bot, GAMEOBJECT_TYPE_GOOBER, 10.0f);
    }

    return true;
}

bool CTFScriptBase::HuntEnemyFC(::Player* bot, ::Player* enemyFC)
{
    if (!bot || !bot->IsInWorld() || !enemyFC || !enemyFC->IsInWorld() || !enemyFC->IsAlive())
        return false;

    float distance = bot->GetExactDist(enemyFC);

    TC_LOG_DEBUG("playerbots.bg.script", "CTF: {} hunting enemy FC {} (dist: {:.1f})",
        bot->GetName(), enemyFC->GetName(), distance);

    if (distance > 30.0f)
    {
        // Too far - move closer
        BotMovementUtil::MoveToPosition(bot, enemyFC->GetPosition());
    }
    else
    {
        // In range - target and attack
        EngageTarget(bot, enemyFC);

        // Chase to melee range
        if (distance > 5.0f)
            BotMovementUtil::ChaseTarget(bot, enemyFC, 5.0f);
    }

    return true;
}

bool CTFScriptBase::EscortFriendlyFC(::Player* bot, ::Player* friendlyFC)
{
    if (!bot || !bot->IsInWorld() || !friendlyFC || !friendlyFC->IsInWorld())
        return false;

    float distance = bot->GetExactDist(friendlyFC);
    constexpr float ESCORT_DISTANCE = 8.0f;
    constexpr float MAX_ESCORT_DISTANCE = 40.0f;

    TC_LOG_DEBUG("playerbots.bg.script", "CTF: {} escorting FC {} (dist: {:.1f})",
        bot->GetName(), friendlyFC->GetName(), distance);

    // Calculate escort position from formation
    Position escortPos;
    auto formation = GetEscortFormation(friendlyFC->GetPosition(), 4);
    if (!formation.empty() && distance < MAX_ESCORT_DISTANCE)
    {
        uint32 idx = bot->GetGUID().GetCounter() % formation.size();
        escortPos = formation[idx];
        BotMovementUtil::CorrectPositionToGround(bot, escortPos);
    }
    else
    {
        // Fallback: follow behind FC
        float angle = friendlyFC->GetOrientation() + static_cast<float>(M_PI);
        escortPos.Relocate(
            friendlyFC->GetPositionX() + ESCORT_DISTANCE * 0.7f * std::cos(angle),
            friendlyFC->GetPositionY() + ESCORT_DISTANCE * 0.7f * std::sin(angle),
            friendlyFC->GetPositionZ()
        );
        BotMovementUtil::CorrectPositionToGround(bot, escortPos);
    }

    // Move to escort position
    if (distance > ESCORT_DISTANCE * 1.5f || !BotMovementUtil::IsMoving(bot))
        BotMovementUtil::MoveToPosition(bot, escortPos);

    // Protect the FC: attack enemies near the FC
    if (friendlyFC->IsInCombat())
    {
        ::Playerbot::BattlegroundCoordinator* coordinator =
            sBGCoordinatorMgr->GetCoordinatorForPlayer(bot);

        if (coordinator)
        {
            auto nearbyEnemies = coordinator->QueryNearbyEnemies(friendlyFC->GetPosition(), 20.0f, bot->GetBGTeam());
            for (auto const* snapshot : nearbyEnemies)
            {
                if (snapshot && snapshot->isAlive)
                {
                    ::Player* enemy = ObjectAccessor::FindPlayer(snapshot->guid);
                    if (enemy && enemy->IsAlive())
                    {
                        EngageTarget(bot, enemy);
                        break;
                    }
                }
            }
        }
        else
        {
            // Fallback: use base class FindNearestEnemyPlayer which is thread-safe
            // (uses coordinator spatial cache with O(n) legacy fallback)
            ::Player* nearbyEnemy = FindNearestEnemyPlayer(bot, 20.0f);
            if (nearbyEnemy)
                EngageTarget(bot, nearbyEnemy);
        }
    }

    return true;
}

bool CTFScriptBase::DefendOwnFlagRoom(::Player* bot)
{
    if (!bot || !bot->IsInWorld())
        return false;

    uint32 teamId = bot->GetBGTeam();

    // Get our flag room position
    Position flagRoomPos = (teamId == ALLIANCE)
        ? GetAllianceFlagPosition()
        : GetHordeFlagPosition();

    // Also try flag room defense positions for better spread
    auto defensePositions = (teamId == ALLIANCE)
        ? GetAllianceFlagRoomDefense()
        : GetHordeFlagRoomDefense();

    Position targetPos = flagRoomPos;
    if (!defensePositions.empty())
    {
        uint32 idx = bot->GetGUID().GetCounter() % defensePositions.size();
        targetPos = defensePositions[idx];
    }

    float distance = bot->GetExactDist(&targetPos);
    constexpr float DEFENSE_RADIUS = 25.0f;

    TC_LOG_DEBUG("playerbots.bg.script", "CTF: {} defending flag room (dist: {:.1f})",
        bot->GetName(), distance);

    // Move to flag room if too far
    if (distance > DEFENSE_RADIUS)
    {
        BotMovementUtil::MoveToPosition(bot, targetPos);
        return true;
    }

    // Try to return any dropped flags first
    if (ReturnDroppedFlag(bot))
        return true;

    // Look for enemies in the flag room
    ::Player* closestEnemy = FindNearestEnemyPlayer(bot, DEFENSE_RADIUS);
    if (closestEnemy)
    {
        EngageTarget(bot, closestEnemy);

        // Chase if too far
        float enemyDist = bot->GetExactDist(closestEnemy);
        if (enemyDist > 5.0f)
            BotMovementUtil::ChaseTarget(bot, closestEnemy, 5.0f);

        return true;
    }

    // No enemies - patrol around flag room
    PatrolAroundPosition(bot, flagRoomPos, 5.0f, 12.0f);
    return true;
}

bool CTFScriptBase::ReturnDroppedFlag(::Player* bot)
{
    if (!bot || !bot->IsInWorld())
        return false;

    // Use phase-ignoring search for dynamically spawned dropped flags
    FindGameObjectOptions options;
    options.IgnorePhases = true;
    options.IsSpawned.reset();  // Clear default for dynamic GOs
    options.GameObjectType = GAMEOBJECT_TYPE_FLAGDROP;

    // Search 100yd radius to catch mid-field dropped flags
    constexpr float DROPPED_FLAG_SEARCH_RANGE = 100.0f;
    std::list<GameObject*> goList;
    bot->GetGameObjectListWithOptionsInGrid(goList, DROPPED_FLAG_SEARCH_RANGE, options);

    GameObject* droppedFlag = nullptr;
    float closestDist = DROPPED_FLAG_SEARCH_RANGE;

    for (GameObject* go : goList)
    {
        if (!go)
            continue;

        float dist = bot->GetExactDist(go);
        if (dist < closestDist)
        {
            closestDist = dist;
            droppedFlag = go;
        }
    }

    if (!droppedFlag)
        return false;

    if (closestDist > 10.0f)
    {
        // Move to the dropped flag
        Position flagPos;
        flagPos.Relocate(droppedFlag->GetPositionX(), droppedFlag->GetPositionY(), droppedFlag->GetPositionZ());
        BotMovementUtil::MoveToPosition(bot, flagPos);
        TC_LOG_DEBUG("playerbots.bg.script", "CTF: {} moving to return dropped flag (dist: {:.1f})",
            bot->GetName(), closestDist);
    }
    else
    {
        // We're at the flag - defer interaction to main thread for thread safety
        sBotActionMgr->QueueAction(BotAction::InteractObject(
            bot->GetGUID(), droppedFlag->GetGUID(), getMSTime()));
        TC_LOG_INFO("playerbots.bg.script", "CTF: {} queued dropped flag return!",
            bot->GetName());
    }

    return true;
}

} // namespace Playerbot::Coordination::Battleground
