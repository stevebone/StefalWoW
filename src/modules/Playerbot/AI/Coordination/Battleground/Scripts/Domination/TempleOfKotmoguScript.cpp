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

#include "TempleOfKotmoguScript.h"
#include "BGScriptRegistry.h"
#include "BattlegroundCoordinator.h"
#include "Battleground.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "GameObject.h"
#include "GameObjectData.h"
#include "GameTime.h"
#include "SpellAuras.h"
#include "BotMovementUtil.h"
#include "BattlegroundCoordinatorManager.h"
#include "BGSpatialQueryCache.h"
#include "Threading/BotActionManager.h"

namespace Playerbot::Coordination::Battleground
{

// Register the script
REGISTER_BG_SCRIPT(TempleOfKotmoguScript, 998);  // TempleOfKotmogu::MAP_ID

// Force linker inclusion - this function is referenced from BGScriptInit.cpp
// to prevent MSVC from discarding this object file from the static library
namespace BGScriptLinkerForce
{
    void ForceIncludeTempleOfKotmoguScript()
    {
        // This function exists solely to create a symbol that BGScriptInit.cpp
        // can reference, forcing the linker to include this object file
    }
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void TempleOfKotmoguScript::OnLoad(BattlegroundCoordinator* coordinator)
{
    DominationScriptBase::OnLoad(coordinator);
    InitializeNodeTracking();

    // Initialize position discovery (may fail if map not ready yet)
    InitializePositionDiscovery();

    // Cache objective data (uses dynamic positions if available)
    m_cachedObjectives = GetObjectiveData();

    // Register world state mappings for scores
    RegisterScoreWorldState(TempleOfKotmogu::WorldStates::SCORE_ALLY, true);
    RegisterScoreWorldState(TempleOfKotmogu::WorldStates::SCORE_HORDE, false);

    // Clear orb tracking
    m_orbHolders.clear();
    m_playerOrbs.clear();
    m_allianceOrbsHeld = 0;
    m_hordeOrbsHeld = 0;

    TC_LOG_DEBUG("playerbots.bg.script",
        "TempleOfKotmoguScript: Loaded with {} orbs, center bonus active, dynamic positions={}",
        TempleOfKotmogu::ORB_COUNT, m_positionsDiscovered ? "yes" : "no");
}

void TempleOfKotmoguScript::OnMatchStart()
{
    DominationScriptBase::OnMatchStart();

    // Retry position discovery if it failed in OnLoad (map should be ready now)
    if (!m_positionsDiscovered)
    {
        if (InitializePositionDiscovery())
        {
            // Re-cache objective data with new positions
            m_cachedObjectives = GetObjectiveData();
            TC_LOG_INFO("playerbots.bg.script",
                "TOK: Dynamic position discovery succeeded on match start!");
        }
    }

    TC_LOG_INFO("playerbots.bg.script",
        "TOK: Match started! Strategy: Grab orbs then push center with escort (dynamic positions={})",
        m_positionsDiscovered ? "yes" : "no");
}

void TempleOfKotmoguScript::OnMatchEnd(bool victory)
{
    DominationScriptBase::OnMatchEnd(victory);

    TC_LOG_INFO("playerbots.bg.script",
        "TOK: Match ended - {}! Final orb control tracked.",
        victory ? "Victory" : "Defeat");
}

// ============================================================================
// EVENT HANDLING
// ============================================================================

void TempleOfKotmoguScript::OnEvent(const BGScriptEventData& event)
{
    DominationScriptBase::OnEvent(event);

    switch (event.eventType)
    {
        case BGScriptEvent::ORB_PICKED_UP:
            if (event.objectiveId < TempleOfKotmogu::ORB_COUNT)
            {
                m_orbHolders[event.objectiveId] = event.primaryGuid;
                m_playerOrbs[event.primaryGuid] = event.objectiveId;

                // Update faction counts (simplified - would need actual faction lookup)
                if (event.faction == ALLIANCE)
                    m_allianceOrbsHeld++;
                else
                    m_hordeOrbsHeld++;

                TC_LOG_DEBUG("playerbots.bg.script",
                    "TOK: {} picked up by {} ({}). Alliance orbs: {}, Horde orbs: {}",
                    TempleOfKotmogu::GetOrbName(event.objectiveId),
                    event.primaryGuid.GetCounter(),
                    event.faction == ALLIANCE ? "Alliance" : "Horde",
                    m_allianceOrbsHeld, m_hordeOrbsHeld);
            }
            break;

        case BGScriptEvent::ORB_DROPPED:
            if (event.objectiveId < TempleOfKotmogu::ORB_COUNT)
            {
                auto it = m_orbHolders.find(event.objectiveId);
                if (it != m_orbHolders.end())
                {
                    // Update faction counts
                    if (event.faction == ALLIANCE && m_allianceOrbsHeld > 0)
                        m_allianceOrbsHeld--;
                    else if (event.faction == HORDE && m_hordeOrbsHeld > 0)
                        m_hordeOrbsHeld--;

                    m_playerOrbs.erase(it->second);
                    m_orbHolders.erase(it);
                }

                TC_LOG_DEBUG("playerbots.bg.script",
                    "TOK: {} dropped at ({:.1f}, {:.1f}). Alliance orbs: {}, Horde orbs: {}",
                    TempleOfKotmogu::GetOrbName(event.objectiveId),
                    event.x, event.y,
                    m_allianceOrbsHeld, m_hordeOrbsHeld);
            }
            break;

        case BGScriptEvent::PLAYER_KILLED:
            // When player dies, they drop their orb
            {
                auto it = m_playerOrbs.find(event.secondaryGuid);
                if (it != m_playerOrbs.end())
                {
                    uint32 orbId = it->second;

                    // Update faction counts (victim's faction)
                    if (event.faction == ALLIANCE && m_hordeOrbsHeld > 0)  // Victim was Horde
                        m_hordeOrbsHeld--;
                    else if (event.faction == HORDE && m_allianceOrbsHeld > 0)  // Victim was Alliance
                        m_allianceOrbsHeld--;

                    m_orbHolders.erase(orbId);
                    m_playerOrbs.erase(it);

                    TC_LOG_DEBUG("playerbots.bg.script",
                        "TOK: Orb carrier killed! {} dropped.",
                        TempleOfKotmogu::GetOrbName(orbId));
                }
            }
            break;

        default:
            break;
    }
}

// ============================================================================
// DATA PROVIDERS
// ============================================================================

std::vector<BGObjectiveData> TempleOfKotmoguScript::GetObjectiveData() const
{
    std::vector<BGObjectiveData> objectives;

    // Add orbs
    for (uint32 i = 0; i < TempleOfKotmogu::ORB_COUNT; ++i)
        objectives.push_back(GetOrbData(i));

    // Add center as a strategic objective
    BGObjectiveData center;
    center.id = 100;  // Special ID for center
    center.type = ObjectiveType::STRATEGIC;
    center.name = "Center Zone";
    center.x = TempleOfKotmogu::CENTER_X;
    center.y = TempleOfKotmogu::CENTER_Y;
    center.z = TempleOfKotmogu::CENTER_Z;
    center.strategicValue = 10;  // Highest value
    objectives.push_back(center);

    return objectives;
}

BGObjectiveData TempleOfKotmoguScript::GetOrbData(uint32 orbId) const
{
    BGObjectiveData orb;
    // Use dynamic position if available, fall back to hardcoded
    Position pos = GetDynamicOrbPosition(orbId);

    orb.id = orbId;
    orb.type = ObjectiveType::ORB;
    orb.name = TempleOfKotmogu::GetOrbName(orbId);
    orb.x = pos.GetPositionX();
    orb.y = pos.GetPositionY();
    orb.z = pos.GetPositionZ();
    orb.orientation = pos.GetOrientation();
    orb.strategicValue = TempleOfKotmogu::GetOrbStrategicValue(orbId);
    orb.captureTime = 0;  // Instant pickup

    // Set game object entries
    switch (orbId)
    {
        case TempleOfKotmogu::Orbs::ORANGE:
            orb.gameObjectEntry = TempleOfKotmogu::GameObjects::ORANGE_ORB;
            break;
        case TempleOfKotmogu::Orbs::BLUE:
            orb.gameObjectEntry = TempleOfKotmogu::GameObjects::BLUE_ORB;
            break;
        case TempleOfKotmogu::Orbs::GREEN:
            orb.gameObjectEntry = TempleOfKotmogu::GameObjects::GREEN_ORB;
            break;
        case TempleOfKotmogu::Orbs::PURPLE:
            orb.gameObjectEntry = TempleOfKotmogu::GameObjects::PURPLE_ORB;
            break;
    }

    return orb;
}

BGObjectiveData TempleOfKotmoguScript::GetNodeData(uint32 nodeIndex) const
{
    return GetOrbData(nodeIndex);
}

std::vector<BGPositionData> TempleOfKotmoguScript::GetSpawnPositions(uint32 faction) const
{
    std::vector<BGPositionData> spawns;

    if (faction == ALLIANCE)
    {
        for (const auto& pos : TempleOfKotmogu::ALLIANCE_SPAWNS)
        {
            BGPositionData spawn;
            spawn.name = "Alliance Spawn";
            spawn.x = pos.GetPositionX();
            spawn.y = pos.GetPositionY();
            spawn.z = pos.GetPositionZ();
            spawn.orientation = pos.GetOrientation();
            spawn.faction = ALLIANCE;
            spawn.posType = BGPositionData::PositionType::SPAWN_POINT;
            spawn.importance = 5;
            spawns.push_back(spawn);
        }
    }
    else
    {
        for (const auto& pos : TempleOfKotmogu::HORDE_SPAWNS)
        {
            BGPositionData spawn;
            spawn.name = "Horde Spawn";
            spawn.x = pos.GetPositionX();
            spawn.y = pos.GetPositionY();
            spawn.z = pos.GetPositionZ();
            spawn.orientation = pos.GetOrientation();
            spawn.faction = HORDE;
            spawn.posType = BGPositionData::PositionType::SPAWN_POINT;
            spawn.importance = 5;
            spawns.push_back(spawn);
        }
    }

    return spawns;
}

std::vector<BGPositionData> TempleOfKotmoguScript::GetStrategicPositions() const
{
    std::vector<BGPositionData> positions;

    // Add orb defense positions
    for (uint32 i = 0; i < TempleOfKotmogu::ORB_COUNT; ++i)
    {
        auto orbPositions = TempleOfKotmogu::GetOrbDefensePositions(i);
        for (const auto& pos : orbPositions)
        {
            std::string name = std::string(TempleOfKotmogu::GetOrbName(i)) + " Defense";
            positions.push_back(BGPositionData(name,
                pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(),
                pos.GetOrientation(),
                BGPositionData::PositionType::DEFENSIVE_POSITION,
                0, 8));
        }
    }

    // Add center defense positions
    auto centerPositions = TempleOfKotmogu::GetCenterDefensePositions();
    for (const auto& pos : centerPositions)
    {
        positions.push_back(BGPositionData("Center Zone",
            pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(),
            pos.GetOrientation(),
            BGPositionData::PositionType::STRATEGIC_POINT,
            0, 10));
    }

    // Add chokepoints
    auto chokepoints = TempleOfKotmogu::GetChokepoints();
    for (size_t i = 0; i < chokepoints.size(); ++i)
    {
        const auto& pos = chokepoints[i];
        std::string name = "Chokepoint " + std::to_string(i + 1);
        positions.push_back(BGPositionData(name,
            pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(),
            pos.GetOrientation(),
            BGPositionData::PositionType::CHOKEPOINT,
            0, 7));
    }

    // Add sniper positions
    auto sniperPositions = TempleOfKotmogu::GetSniperPositions();
    for (size_t i = 0; i < sniperPositions.size(); ++i)
    {
        const auto& pos = sniperPositions[i];
        std::string name = "Sniper Position " + std::to_string(i + 1);
        positions.push_back(BGPositionData(name,
            pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(),
            pos.GetOrientation(),
            BGPositionData::PositionType::SNIPER_POSITION,
            0, 8));
    }

    // Add buff positions
    auto buffPositions = TempleOfKotmogu::GetBuffPositions();
    for (size_t i = 0; i < buffPositions.size(); ++i)
    {
        const auto& pos = buffPositions[i];
        std::string name = "Power-up " + std::to_string(i + 1);
        positions.push_back(BGPositionData(name,
            pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(),
            pos.GetOrientation(),
            BGPositionData::PositionType::BUFF_LOCATION,
            0, 5));
    }

    return positions;
}

std::vector<BGPositionData> TempleOfKotmoguScript::GetGraveyardPositions(uint32 faction) const
{
    // TOK has faction-specific graveyards (same as spawns)
    return GetSpawnPositions(faction);
}

std::vector<BGWorldState> TempleOfKotmoguScript::GetInitialWorldStates() const
{
    std::vector<BGWorldState> states;

    // Scores
    states.push_back(BGWorldState(TempleOfKotmogu::WorldStates::SCORE_ALLY,
        "Alliance Score", BGWorldState::StateType::SCORE_ALLIANCE, 0));
    states.push_back(BGWorldState(TempleOfKotmogu::WorldStates::SCORE_HORDE,
        "Horde Score", BGWorldState::StateType::SCORE_HORDE, 0));
    states.push_back(BGWorldState(TempleOfKotmogu::WorldStates::MAX_SCORE,
        "Max Score", BGWorldState::StateType::CUSTOM, TempleOfKotmogu::MAX_SCORE));

    // Orb states
    states.push_back(BGWorldState(TempleOfKotmogu::WorldStates::ORANGE_ORB_STATE,
        "Orange Orb", BGWorldState::StateType::OBJECTIVE_STATE, 0));
    states.push_back(BGWorldState(TempleOfKotmogu::WorldStates::BLUE_ORB_STATE,
        "Blue Orb", BGWorldState::StateType::OBJECTIVE_STATE, 0));
    states.push_back(BGWorldState(TempleOfKotmogu::WorldStates::GREEN_ORB_STATE,
        "Green Orb", BGWorldState::StateType::OBJECTIVE_STATE, 0));
    states.push_back(BGWorldState(TempleOfKotmogu::WorldStates::PURPLE_ORB_STATE,
        "Purple Orb", BGWorldState::StateType::OBJECTIVE_STATE, 0));

    return states;
}

std::vector<uint32> TempleOfKotmoguScript::GetTickPointsTable() const
{
    // Points per orb held (modified by center position)
    return std::vector<uint32>(
        TempleOfKotmogu::TICK_POINTS_OUTSIDE.begin(),
        TempleOfKotmogu::TICK_POINTS_OUTSIDE.end()
    );
}

// ============================================================================
// WORLD STATE INTERPRETATION
// ============================================================================

bool TempleOfKotmoguScript::InterpretWorldState(int32 stateId, int32 value,
    uint32& outObjectiveId, BGObjectiveState& outState) const
{
    // Try cached mappings
    if (TryInterpretFromCache(stateId, value, outObjectiveId, outState))
        return true;

    // Orb-specific interpretation would go here
    // (Orbs don't have the same controlled/contested states as nodes)

    return false;
}

void TempleOfKotmoguScript::GetScoreFromWorldStates(const std::map<int32, int32>& states,
    uint32& allianceScore, uint32& hordeScore) const
{
    allianceScore = 0;
    hordeScore = 0;

    auto allyIt = states.find(TempleOfKotmogu::WorldStates::SCORE_ALLY);
    if (allyIt != states.end())
        allianceScore = static_cast<uint32>(std::max(0, allyIt->second));

    auto hordeIt = states.find(TempleOfKotmogu::WorldStates::SCORE_HORDE);
    if (hordeIt != states.end())
        hordeScore = static_cast<uint32>(std::max(0, hordeIt->second));
}

// ============================================================================
// STRATEGY
// ============================================================================

TempleOfKotmoguScript::GamePhase TempleOfKotmoguScript::GetCurrentPhase() const
{
    uint32 elapsed = GetElapsedTime();

    // Check score advantage for desperate phase using member variables
    uint32 faction = m_coordinator ? m_coordinator->GetFaction() : ALLIANCE;
    uint32 ourScore = (faction == ALLIANCE) ? m_allianceScore : m_hordeScore;
    uint32 theirScore = (faction == ALLIANCE) ? m_hordeScore : m_allianceScore;

    // Phase hysteresis: use different thresholds for entering vs exiting DESPERATE
    // Enter DESPERATE at full threshold. Exit DESPERATE only when gap closes to half threshold.
    constexpr uint32 ENTER_THRESHOLD = TempleOfKotmogu::Strategy::DESPERATION_THRESHOLD;
    constexpr uint32 EXIT_THRESHOLD = TempleOfKotmogu::Strategy::DESPERATION_THRESHOLD / 2;

    if (elapsed > TempleOfKotmogu::Strategy::MID_GAME_START)
    {
        if (m_lastPhase == GamePhase::DESPERATE)
        {
            // Already desperate: only exit when gap narrows to half threshold
            if (ourScore + EXIT_THRESHOLD < theirScore)
            {
                m_lastPhase = GamePhase::DESPERATE;
                return GamePhase::DESPERATE;
            }
            // Gap narrowed enough, fall through to normal phase logic
        }
        else
        {
            // Not desperate: enter at full threshold
            if (ourScore + ENTER_THRESHOLD < theirScore)
            {
                m_lastPhase = GamePhase::DESPERATE;
                return GamePhase::DESPERATE;
            }
        }
    }

    GamePhase phase;
    if (elapsed < TempleOfKotmogu::Strategy::OPENING_PHASE_DURATION)
        phase = GamePhase::OPENING;
    else if (elapsed < TempleOfKotmogu::Strategy::LATE_GAME_START)
        phase = GamePhase::MID_GAME;
    else
        phase = GamePhase::LATE_GAME;

    m_lastPhase = phase;
    return phase;
}

void TempleOfKotmoguScript::ApplyPhaseStrategy(StrategicDecision& decision, GamePhase phase, float scoreAdvantage) const
{
    switch (phase)
    {
        case GamePhase::OPENING:
            decision.reasoning = "Opening phase - rush to grab orbs!";
            decision.strategy = BGStrategy::AGGRESSIVE;
            decision.offenseAllocation = 80;
            decision.defenseAllocation = 20;
            break;

        case GamePhase::MID_GAME:
            if (scoreAdvantage > 0.15f)
            {
                decision.reasoning = "Mid-game (leading) - hold orbs and score safely";
                decision.strategy = BGStrategy::DEFENSIVE;
                decision.offenseAllocation = 35;
                decision.defenseAllocation = 65;
            }
            else if (scoreAdvantage < -0.15f)
            {
                decision.reasoning = "Mid-game (behind) - aggressive orb hunting";
                decision.strategy = BGStrategy::AGGRESSIVE;
                decision.offenseAllocation = 70;
                decision.defenseAllocation = 30;
            }
            else
            {
                decision.reasoning = "Mid-game (even) - balanced orb control";
                decision.strategy = BGStrategy::BALANCED;
                decision.offenseAllocation = 50;
                decision.defenseAllocation = 50;
            }
            break;

        case GamePhase::LATE_GAME:
            if (scoreAdvantage > 0.1f)
            {
                decision.reasoning = "Late game (winning) - protect orb carriers";
                decision.strategy = BGStrategy::DEFENSIVE;
                decision.offenseAllocation = 25;
                decision.defenseAllocation = 75;
            }
            else
            {
                decision.reasoning = "Late game (close/behind) - push center with orbs!";
                decision.strategy = BGStrategy::AGGRESSIVE;
                decision.offenseAllocation = 65;
                decision.defenseAllocation = 35;
            }
            break;

        case GamePhase::DESPERATE:
            decision.reasoning = "DESPERATE - All in on center with orbs!";
            decision.strategy = BGStrategy::ALL_IN;
            decision.offenseAllocation = 90;
            decision.defenseAllocation = 10;
            break;
    }
}

void TempleOfKotmoguScript::AdjustStrategy(StrategicDecision& decision,
    float scoreAdvantage, uint32 controlledCount,
    uint32 totalObjectives, uint32 timeRemaining) const
{
    uint32 faction = m_coordinator ? m_coordinator->GetFaction() : ALLIANCE;
    uint32 ourOrbs = GetOrbsHeldByFaction(faction);
    uint32 theirOrbs = GetOrbsHeldByFaction(faction == ALLIANCE ? HORDE : ALLIANCE);

    // Apply phase-specific strategy
    GamePhase phase = GetCurrentPhase();
    ApplyPhaseStrategy(decision, phase, scoreAdvantage);

    // Orb-specific adjustments
    if (ourOrbs >= 2)
    {
        // We have enough orbs - consider center push
        if (ShouldPushToCenter(faction))
        {
            decision.reasoning += " (pushing to center!)";
            decision.offenseAllocation += 10;  // More aggressive
        }
        else
        {
            decision.reasoning += " (holding orbs safely)";
        }
    }
    else if (ourOrbs == 0)
    {
        // No orbs - must be aggressive
        decision.strategy = BGStrategy::AGGRESSIVE;
        decision.reasoning = "No orbs! Must grab orbs immediately!";
        decision.offenseAllocation = 80;
        decision.defenseAllocation = 20;
    }

    // If enemy has many orbs, prioritize killing carriers
    if (theirOrbs >= 3)
    {
        decision.reasoning += " (enemy has too many orbs - HUNT THEM!)";
        decision.offenseAllocation = std::min(static_cast<uint8>(90), static_cast<uint8>(decision.offenseAllocation + 20));
    }

    // Center bonus reminder
    decision.reasoning += " (center = 5x points!)";
}

RoleDistribution TempleOfKotmoguScript::GetRecommendedRoles(const StrategicDecision& decision,
    float scoreAdvantage, uint32 timeRemaining) const
{
    RoleDistribution dist;

    uint32 faction = m_coordinator ? m_coordinator->GetFaction() : ALLIANCE;
    uint32 ourOrbs = GetOrbsHeldByFaction(faction);

    switch (decision.strategy)
    {
        case BGStrategy::AGGRESSIVE:
            dist.roleCounts[BGRole::ORB_CARRIER] = 30;      // Orb grabbers
            dist.roleCounts[BGRole::FLAG_ESCORT] = 20;      // Protect carriers
            dist.roleCounts[BGRole::NODE_ATTACKER] = 40;    // Kill enemy carriers
            dist.roleCounts[BGRole::NODE_DEFENDER] = 10;
            dist.reasoning = "Aggressive orb hunting";
            break;

        case BGStrategy::DEFENSIVE:
            dist.roleCounts[BGRole::ORB_CARRIER] = 25;
            dist.roleCounts[BGRole::FLAG_ESCORT] = 45;      // Heavy protection
            dist.roleCounts[BGRole::NODE_DEFENDER] = 20;
            dist.roleCounts[BGRole::NODE_ATTACKER] = 10;
            dist.reasoning = "Defensive orb hold";
            break;

        case BGStrategy::ALL_IN:
            dist.roleCounts[BGRole::ORB_CARRIER] = 40;
            dist.roleCounts[BGRole::FLAG_ESCORT] = 40;
            dist.roleCounts[BGRole::NODE_ATTACKER] = 20;
            dist.roleCounts[BGRole::NODE_DEFENDER] = 0;
            dist.reasoning = "All-in center push";
            break;

        default:  // BALANCED
            dist.roleCounts[BGRole::ORB_CARRIER] = 30;
            dist.roleCounts[BGRole::FLAG_ESCORT] = 30;
            dist.roleCounts[BGRole::NODE_ATTACKER] = 25;
            dist.roleCounts[BGRole::NODE_DEFENDER] = 15;
            dist.reasoning = "Balanced orb control";
            break;
    }

    // Adjust based on orb count
    if (ourOrbs >= 2)
    {
        // Shift toward escort
        dist.roleCounts[BGRole::FLAG_ESCORT] += 10;
        dist.roleCounts[BGRole::NODE_ATTACKER] -= 10;
        dist.reasoning = "Escorting orb carriers";
    }

    return dist;
}

// ============================================================================
// ORB-SPECIFIC METHODS
// ============================================================================

ObjectGuid TempleOfKotmoguScript::GetOrbHolder(uint32 orbId) const
{
    auto it = m_orbHolders.find(orbId);
    return it != m_orbHolders.end() ? it->second : ObjectGuid::Empty;
}

bool TempleOfKotmoguScript::IsPlayerHoldingOrb(ObjectGuid guid) const
{
    return m_playerOrbs.find(guid) != m_playerOrbs.end();
}

int32 TempleOfKotmoguScript::GetPlayerOrbId(ObjectGuid guid) const
{
    auto it = m_playerOrbs.find(guid);
    return it != m_playerOrbs.end() ? static_cast<int32>(it->second) : -1;
}

uint32 TempleOfKotmoguScript::GetOrbsHeldByFaction(uint32 faction) const
{
    return (faction == ALLIANCE) ? m_allianceOrbsHeld : m_hordeOrbsHeld;
}

bool TempleOfKotmoguScript::IsInCenter(float x, float y) const
{
    return TempleOfKotmogu::IsInCenterZone(x, y);
}

std::vector<Position> TempleOfKotmoguScript::GetOrbCarrierRoute(uint32 orbId) const
{
    return TempleOfKotmogu::GetOrbCarrierRoute(orbId);
}

std::vector<Position> TempleOfKotmoguScript::GetEscortFormation(float carrierX, float carrierY, float carrierZ) const
{
    return TempleOfKotmogu::GetEscortFormation(carrierX, carrierY, carrierZ);
}

bool TempleOfKotmoguScript::ShouldPushToCenter(uint32 faction) const
{
    uint32 ourOrbs = GetOrbsHeldByFaction(faction);

    // Need at least 2 orbs to push center
    if (ourOrbs < TempleOfKotmogu::Strategy::CENTER_PUSH_ORB_COUNT)
        return false;

    // Don't push too early
    if (GetElapsedTime() < TempleOfKotmogu::Strategy::INITIAL_HOLD_TIME)
        return false;

    // Push if we have good orb advantage
    uint32 theirOrbs = GetOrbsHeldByFaction(faction == ALLIANCE ? HORDE : ALLIANCE);
    return ourOrbs >= theirOrbs;
}

std::vector<uint32> TempleOfKotmoguScript::GetOrbPriority(uint32 faction) const
{
    // Alliance closer to Orange/Blue (east side)
    // Horde closer to Green/Purple (west side)
    if (faction == ALLIANCE)
    {
        return {
            TempleOfKotmogu::Orbs::ORANGE,  // Closest
            TempleOfKotmogu::Orbs::BLUE,
            TempleOfKotmogu::Orbs::GREEN,
            TempleOfKotmogu::Orbs::PURPLE   // Furthest
        };
    }
    else
    {
        return {
            TempleOfKotmogu::Orbs::GREEN,   // Closest
            TempleOfKotmogu::Orbs::PURPLE,
            TempleOfKotmogu::Orbs::ORANGE,
            TempleOfKotmogu::Orbs::BLUE     // Furthest
        };
    }
}

// ============================================================================
// ENTERPRISE-GRADE POSITIONING
// ============================================================================

std::vector<Position> TempleOfKotmoguScript::GetChokepoints() const
{
    return TempleOfKotmogu::GetChokepoints();
}

std::vector<Position> TempleOfKotmoguScript::GetSniperPositions() const
{
    return TempleOfKotmogu::GetSniperPositions();
}

std::vector<Position> TempleOfKotmoguScript::GetBuffPositions() const
{
    return TempleOfKotmogu::GetBuffPositions();
}

std::vector<Position> TempleOfKotmoguScript::GetAmbushPositions(uint32 faction) const
{
    return TempleOfKotmogu::GetAmbushPositions(faction);
}

std::vector<Position> TempleOfKotmoguScript::GetCenterDefensePositions() const
{
    return TempleOfKotmogu::GetCenterDefensePositions();
}

std::vector<Position> TempleOfKotmoguScript::GetOrbDefensePositions(uint32 orbId) const
{
    return TempleOfKotmogu::GetOrbDefensePositions(orbId);
}

float TempleOfKotmoguScript::GetOrbToOrbDistance(uint32 fromOrb, uint32 toOrb) const
{
    return TempleOfKotmogu::GetOrbDistance(fromOrb, toOrb);
}

float TempleOfKotmoguScript::GetOrbToCenterDistance(uint32 orbId) const
{
    return TempleOfKotmogu::GetOrbToCenterDistance(orbId);
}

// ============================================================================
// DYNAMIC POSITION DISCOVERY
// ============================================================================

bool TempleOfKotmoguScript::InitializePositionDiscovery()
{
    // Already discovered
    if (m_positionsDiscovered)
        return true;

    // Need coordinator and battleground
    if (!m_coordinator)
    {
        TC_LOG_DEBUG("playerbots.bg.script",
            "TOK: Cannot initialize position discovery - no coordinator");
        return false;
    }

    ::Battleground* bg = m_coordinator->GetBattleground();
    if (!bg)
    {
        TC_LOG_DEBUG("playerbots.bg.script",
            "TOK: Cannot initialize position discovery - no battleground");
        return false;
    }

    // Create discovery system
    m_positionDiscovery = std::make_unique<BGPositionDiscovery>(bg);

    if (!m_positionDiscovery->Initialize())
    {
        TC_LOG_DEBUG("playerbots.bg.script",
            "TOK: Position discovery initialization failed (map not ready?)");
        m_positionDiscovery.reset();
        return false;
    }

    // Discover orb game objects
    std::vector<uint32> orbEntries = {
        TempleOfKotmogu::GameObjects::ORANGE_ORB,
        TempleOfKotmogu::GameObjects::BLUE_ORB,
        TempleOfKotmogu::GameObjects::GREEN_ORB,
        TempleOfKotmogu::GameObjects::PURPLE_ORB
    };

    auto discoveredOrbs = m_positionDiscovery->DiscoverGameObjects(orbEntries, "Orb");

    // Map discovered positions to orb IDs
    bool anyDiscovered = false;
    for (auto const& poi : discoveredOrbs)
    {
        uint32 orbIndex = 0xFFFFFFFF;
        switch (poi.gameObjectEntry)
        {
            case TempleOfKotmogu::GameObjects::ORANGE_ORB:
                orbIndex = TempleOfKotmogu::Orbs::ORANGE;
                break;
            case TempleOfKotmogu::GameObjects::BLUE_ORB:
                orbIndex = TempleOfKotmogu::Orbs::BLUE;
                break;
            case TempleOfKotmogu::GameObjects::GREEN_ORB:
                orbIndex = TempleOfKotmogu::Orbs::GREEN;
                break;
            case TempleOfKotmogu::GameObjects::PURPLE_ORB:
                orbIndex = TempleOfKotmogu::Orbs::PURPLE;
                break;
        }

        if (orbIndex < TempleOfKotmogu::ORB_COUNT)
        {
            m_orbPositions[orbIndex] = poi.position;
            m_positionDiscovery->CachePOI(poi);
            anyDiscovered = true;

            TC_LOG_INFO("playerbots.bg.script",
                "TOK: Dynamically discovered {} at ({:.1f},{:.1f},{:.1f})",
                TempleOfKotmogu::GetOrbName(orbIndex),
                poi.position.GetPositionX(), poi.position.GetPositionY(),
                poi.position.GetPositionZ());
        }
    }

    // If no dynamic discovery, fall back to hardcoded positions
    if (!anyDiscovered)
    {
        TC_LOG_WARN("playerbots.bg.script",
            "TOK: Dynamic orb discovery failed - using hardcoded positions (may cause pathfinding issues!)");

        for (uint32 i = 0; i < TempleOfKotmogu::ORB_COUNT; ++i)
        {
            m_orbPositions[i] = TempleOfKotmogu::GetOrbPosition(i);
        }
    }
    else
    {
        // Fill any missing with hardcoded
        for (uint32 i = 0; i < TempleOfKotmogu::ORB_COUNT; ++i)
        {
            if (m_orbPositions[i].GetPositionX() == 0.0f &&
                m_orbPositions[i].GetPositionY() == 0.0f)
            {
                m_orbPositions[i] = TempleOfKotmogu::GetOrbPosition(i);
                TC_LOG_WARN("playerbots.bg.script",
                    "TOK: {} not discovered - using hardcoded position",
                    TempleOfKotmogu::GetOrbName(i));
            }
        }
    }

    m_positionsDiscovered = true;
    m_positionDiscovery->LogDiscoveryStatus();

    return true;
}

Position TempleOfKotmoguScript::GetDynamicOrbPosition(uint32 orbId) const
{
    if (orbId >= TempleOfKotmogu::ORB_COUNT)
        return Position(0, 0, 0, 0);

    if (m_positionsDiscovered)
    {
        return m_orbPositions[orbId];
    }

    // Fall back to hardcoded
    return TempleOfKotmogu::GetOrbPosition(orbId);
}

// ============================================================================
// RUNTIME BEHAVIOR (lighthouse pattern)
// ============================================================================

// Local constants for TOK behavior
constexpr float TOK_OBJECTIVE_RANGE = 10.0f;
constexpr float TOK_ESCORT_DISTANCE = 8.0f;
constexpr float TOK_DEFENSE_ESCORT_RANGE = 30.0f;
constexpr float TOK_MAX_ESCORT_DISTANCE = 40.0f;
constexpr float TOK_LOW_HEALTH_PCT = 30.0f;

// Orb aura IDs for aura-based checks
constexpr uint32 TOK_ORB_AURAS[] = {
    TempleOfKotmogu::Spells::ORANGE_ORB_AURA,  // 121175
    TempleOfKotmogu::Spells::BLUE_ORB_AURA,    // 121176
    TempleOfKotmogu::Spells::GREEN_ORB_AURA,   // 121177
    TempleOfKotmogu::Spells::PURPLE_ORB_AURA   // 121178
};

// Orb GameObject entries
constexpr uint32 TOK_ORB_ENTRIES[] = {
    TempleOfKotmogu::GameObjects::ORANGE_ORB,   // 212094
    TempleOfKotmogu::GameObjects::BLUE_ORB,     // 212091
    TempleOfKotmogu::GameObjects::GREEN_ORB,    // 212093
    TempleOfKotmogu::GameObjects::PURPLE_ORB    // 212092
};

/// Helper: check if player is carrying any TOK orb via aura check
static bool IsCarryingOrb(::Player* player)
{
    for (uint32 aura : TOK_ORB_AURAS)
    {
        if (player->HasAura(aura))
            return true;
    }
    return false;
}

/// Helper: get orbId (0-3) from orb aura, or -1 if not carrying
static int32 GetCarriedOrbId(::Player* player)
{
    for (uint32 i = 0; i < TempleOfKotmogu::ORB_COUNT; ++i)
    {
        if (player->HasAura(TOK_ORB_AURAS[i]))
            return static_cast<int32>(i);
    }
    return -1;
}

// ============================================================================
// REAL-TIME ORB STATE DETECTION VIA AURA SCANNING
// ============================================================================

void TempleOfKotmoguScript::RefreshOrbState()
{
    // Throttle to once per second
    uint32 now = GameTime::GetGameTimeMS();
    if (now - m_lastOrbRefresh < 1000)
        return;
    m_lastOrbRefresh = now;

    // Need coordinator to access BG
    if (!m_coordinator)
        return;
    ::Battleground* bg = m_coordinator->GetBattleground();
    if (!bg)
        return;

    // CRITICAL: Build new state in LOCAL variables first, then swap.
    // Multiple worker threads call ExecuteStrategy() → RefreshOrbState() concurrently.
    // The old pattern (clear → rebuild) left m_orbHolders EMPTY during rebuild,
    // causing other threads to see ALL orbs as "free" → bots run to orb spawns
    // instead of hunting carriers.
    std::map<uint32, ObjectGuid> newOrbHolders;
    std::map<ObjectGuid, uint32> newPlayerOrbs;
    uint32 newAllianceOrbs = 0;
    uint32 newHordeOrbs = 0;

    for (auto const& [guid, bgPlayer] : bg->GetPlayers())
    {
        ::Player* player = ObjectAccessor::FindPlayer(guid);
        if (!player || !player->IsAlive())
            continue;

        int32 orbId = GetCarriedOrbId(player);
        if (orbId >= 0)
        {
            newOrbHolders[static_cast<uint32>(orbId)] = guid;
            newPlayerOrbs[guid] = static_cast<uint32>(orbId);

            if (player->GetBGTeam() == ALLIANCE)
                newAllianceOrbs++;
            else
                newHordeOrbs++;
        }
    }

    // Atomic-ish swap: state goes from old-complete to new-complete, never empty
    m_orbHolders = std::move(newOrbHolders);
    m_playerOrbs = std::move(newPlayerOrbs);
    m_allianceOrbsHeld = newAllianceOrbs;
    m_hordeOrbsHeld = newHordeOrbs;

    TC_LOG_DEBUG("playerbots.bg", "[TOK] RefreshOrbState: {} orbs held (Alliance={}, Horde={})",
        static_cast<uint32>(m_orbHolders.size()), m_allianceOrbsHeld, m_hordeOrbsHeld);
}

bool TempleOfKotmoguScript::ExecuteStrategy(::Player* player)
{
    if (!player || !player->IsInWorld() || !player->IsAlive())
        return false;

    // Refresh orb state from auras (throttled to once per second)
    RefreshOrbState();

    bool holdingOrb = IsCarryingOrb(player);

    TC_LOG_DEBUG("playerbots.bg", "[TOK] {} holdingOrb={} orbsHeld={} (ally={} horde={})",
        player->GetName(), holdingOrb, static_cast<uint32>(m_orbHolders.size()),
        m_allianceOrbsHeld, m_hordeOrbsHeld);

    // =========================================================================
    // PRIORITY 1: If holding orb, execute carrier movement to center
    // =========================================================================
    if (holdingOrb)
    {
        // Player is now a carrier - remove from orb targeters if present
        m_orbTargeters.erase(
            std::find_if(m_orbTargeters.begin(), m_orbTargeters.end(),
                [&](auto const& kv) { return kv.second == player->GetGUID(); }),
            m_orbTargeters.end());

        // Clear pending pickup — orb was successfully acquired
        m_pendingOrbPickup.erase(player->GetGUID());

        ExecuteOrbCarrierMovement(player);
        return true;
    }

    // =========================================================================
    // PENDING PICKUP HOLD: If this bot queued a Use() via BotActionMgr,
    // hold position at the orb until the main thread processes it.
    // Without this, the bot moves away on the next worker tick and the
    // deferred GO::CastSpell range check fails silently.
    // =========================================================================
    {
        auto pendingIt = m_pendingOrbPickup.find(player->GetGUID());
        if (pendingIt != m_pendingOrbPickup.end())
        {
            uint32 now = GameTime::GetGameTimeMS();
            uint32 elapsed = now - pendingIt->second.queuedTime;

            // Success: bot got the orb aura — clear pending, fall through to Priority 1
            // (already handled above since holdingOrb would be true)

            // Timeout: 2 seconds should be more than enough for main thread to process
            if (elapsed > 2000)
            {
                TC_LOG_DEBUG("playerbots.bg", "[TOK] {} pending pickup timed out after {}ms, clearing",
                    player->GetName(), elapsed);
                m_pendingOrbPickup.erase(pendingIt);
                // Fall through to normal priority evaluation
            }
            else
            {
                // Hold position at the orb — don't move anywhere
                float distToOrb = player->GetExactDist(&pendingIt->second.orbPosition);
                if (distToOrb > 3.0f)
                {
                    // Moved slightly? Move back to orb
                    BotMovementUtil::MoveToPosition(player, pendingIt->second.orbPosition);
                }
                // Otherwise just stand still — the main thread will process Use() soon

                TC_LOG_DEBUG("playerbots.bg", "[TOK] {} holding position at {} for pending pickup ({}ms elapsed, dist={:.1f})",
                    player->GetName(), TempleOfKotmogu::GetOrbName(pendingIt->second.orbId),
                    elapsed, distToOrb);
                return true;
            }
        }
    }

    // =========================================================================
    // PRIORITY 2: Free orb exists → pick it up (with carrier-awareness gating)
    //
    // CRITICAL: When carriers already exist, we must NOT send ALL bots to orb
    // spawns. Only bots whose GUID-based preferred slot maps to a free orb
    // should attempt pickup. The rest continue to escort/hunt in Priority 3.
    // Without this gate, one dropped orb causes all 9 bots to abandon hunting.
    // =========================================================================
    {
        uint32 now = GameTime::GetGameTimeMS();
        uint32 freeCount = 0;
        uint32 heldCount = 0;
        for (uint32 i = 0; i < TempleOfKotmogu::ORB_COUNT; ++i)
        {
            if (IsOrbHeld(i))
            {
                heldCount++;
                continue;
            }
            // Skip orbs already claimed (Use() queued but aura not yet detected)
            auto claimIt = m_orbClaimedUntil.find(i);
            if (claimIt != m_orbClaimedUntil.end() && now < claimIt->second)
                continue;
            // Skip orbs on search-failed cooldown — they appear "free" but have no GO
            auto failIt = m_orbSearchFailed.find(i);
            if (failIt != m_orbSearchFailed.end() && now < failIt->second)
                continue;
            freeCount++;
        }

        if (freeCount > 0)
        {
            bool shouldAttemptPickup = false;

            if (heldCount == 0)
            {
                // No carriers exist at all (BG start or all orbs dropped) — everyone picks up
                shouldAttemptPickup = true;
            }
            else
            {
                // Carriers exist. We need SOME bots to pick up free orbs but NOT all.
                // Use GUID % ORB_COUNT to get a stable slot (0-3). If the slot < freeCount,
                // this bot is assigned to pickup duty. With 9-10 bots and 4 slots, each
                // slot gets ~2-3 bots. So freeCount=1 sends ~2-3 bots, freeCount=2 sends
                // ~5 bots, etc. PickupOrb() internally round-robins to find the best
                // free orb, so no specific orb-to-slot mapping is needed at this gate.
                uint32 botSlot = player->GetGUID().GetCounter() % TempleOfKotmogu::ORB_COUNT;
                shouldAttemptPickup = (botSlot < freeCount);
            }

            if (shouldAttemptPickup)
            {
                if (PickupOrb(player))
                    return true;
                // If PickupOrb fails (all orbs just got taken), fall through to escort/hunt
            }
        }
    }

    // =========================================================================
    // PRIORITY 3: Dynamic behavior based on current game state
    //
    // The coordinator role system (ROAMER/ORB_CARRIER/etc.) does not
    // dynamically reassign roles when game state changes (orbs picked up,
    // carriers killed, etc.). Instead, we evaluate the situation each tick
    // and take the most useful action based on proximity and need.
    //
    // Split: 2/3 of bots escort friendly carriers, 1/3 hunt enemy carriers.
    // This is deterministic per-bot via GUID hash to avoid thrashing.
    // =========================================================================

    // Find nearest friendly and enemy orb carriers
    ::Player* nearestFriendlyCarrier = nullptr;
    float friendlyCarrierDist = std::numeric_limits<float>::max();
    ::Player* nearestEnemyCarrier = nullptr;
    float enemyCarrierDist = std::numeric_limits<float>::max();

    for (uint32 orbId = 0; orbId < TempleOfKotmogu::ORB_COUNT; ++orbId)
    {
        if (!IsOrbHeld(orbId))
            continue;

        ObjectGuid holderGuid = GetOrbHolder(orbId);
        if (holderGuid.IsEmpty())
            continue;

        ::Player* holder = ObjectAccessor::FindPlayer(holderGuid);
        if (!holder || !holder->IsAlive())
            continue;

        float dist = player->GetExactDist(holder);

        if (holder->IsHostileTo(player))
        {
            // Prioritize center carriers (they score more points)
            bool inCenter = IsInCenter(holder->GetPositionX(), holder->GetPositionY());
            float effectiveDist = inCenter ? dist * 0.5f : dist;
            if (effectiveDist < enemyCarrierDist)
            {
                enemyCarrierDist = effectiveDist;
                nearestEnemyCarrier = holder;
            }
        }
        else if (holder->GetGUID() != player->GetGUID())
        {
            if (dist < friendlyCarrierDist)
            {
                friendlyCarrierDist = dist;
                nearestFriendlyCarrier = holder;
            }
        }
    }

    // Use GUID-based hash for deterministic duty split:
    // Slots 0,1 → prefer escort (protect our carriers)
    // Slot 2   → prefer hunt (kill enemy carriers)
    uint32 dutySlot = player->GetGUID().GetCounter() % 3;
    bool preferEscort = (dutySlot < 2);

    if (nearestFriendlyCarrier && nearestEnemyCarrier)
    {
        // Both friendly and enemy carriers exist — split duties
        if (preferEscort && friendlyCarrierDist < 60.0f)
        {
            TC_LOG_DEBUG("playerbots.bg", "[TOK] {} escorting (duty=escort, carrier dist={:.1f})",
                player->GetName(), friendlyCarrierDist);
            EscortOrbCarrier(player);
        }
        else
        {
            TC_LOG_DEBUG("playerbots.bg", "[TOK] {} hunting enemy carrier (duty=hunt, dist={:.1f})",
                player->GetName(), enemyCarrierDist);
            HuntEnemyOrbCarrier(player);
        }
        return true;
    }

    if (nearestFriendlyCarrier)
    {
        // Only friendly carriers — everyone escorts
        TC_LOG_DEBUG("playerbots.bg", "[TOK] {} escorting (no enemy carriers, carrier dist={:.1f})",
            player->GetName(), friendlyCarrierDist);
        EscortOrbCarrier(player);
        return true;
    }

    if (nearestEnemyCarrier)
    {
        // Only enemy carriers — everyone hunts
        TC_LOG_DEBUG("playerbots.bg", "[TOK] {} hunting enemy carrier (no friendly carriers, dist={:.1f})",
            player->GetName(), enemyCarrierDist);
        HuntEnemyOrbCarrier(player);
        return true;
    }

    // =========================================================================
    // PRIORITY 4: No carriers at all — patrol center and fight enemies
    // =========================================================================
    TC_LOG_DEBUG("playerbots.bg", "[TOK] {} no carriers found, patrolling center",
        player->GetName());
    DefendOrbCarrier(player);
    return true;
}

bool TempleOfKotmoguScript::PickupOrb(::Player* player)
{
    if (!player || !player->IsInWorld() || !player->IsAlive())
        return false;

    // Already carrying an orb - nothing to pick up
    if (IsCarryingOrb(player))
    {
        TC_LOG_DEBUG("playerbots.bg", "[TOK] {} already carrying an orb, skipping pickup",
            player->GetName());
        return false;
    }

    // =========================================================================
    // Deterministic orb assignment via GUID-based slot system
    //
    // Old approach: "pick nearest untargeted orb" with m_orbTargeters tracking.
    // Problem: multiple bot worker threads evaluate simultaneously, all see empty
    // m_orbTargeters, all pick the SAME nearest orb → 1 bot on one orb, 8 on another.
    //
    // New approach: each bot has a fixed "preferred orb slot" from GUID % ORB_COUNT.
    // They always try their preferred orb first, then round-robin to the next free
    // orb. This ensures even distribution without shared mutable state or races.
    // =========================================================================
    uint32 now = GameTime::GetGameTimeMS();
    std::vector<uint32> orbPriority = GetOrbPriority(player->GetBGTeam());

    // Build free orb list (not held, not claimed, not search-failed)
    std::vector<uint32> freeOrbs;
    for (uint32 orbId : orbPriority)
    {
        if (orbId >= TempleOfKotmogu::ORB_COUNT)
            continue;
        if (IsOrbHeld(orbId))
            continue;
        auto claimIt = m_orbClaimedUntil.find(orbId);
        if (claimIt != m_orbClaimedUntil.end() && now < claimIt->second)
            continue;
        auto failIt = m_orbSearchFailed.find(orbId);
        if (failIt != m_orbSearchFailed.end() && now < failIt->second)
            continue;
        freeOrbs.push_back(orbId);
    }

    if (freeOrbs.empty())
    {
        TC_LOG_DEBUG("playerbots.bg", "[TOK] {} no available orbs to pick up (all held/claimed)",
            player->GetName());
        return false;
    }

    // Deterministic slot: GUID counter mod ORB_COUNT gives a preferred slot (0-3).
    // Map that to the faction-priority orb list, then round-robin to find the
    // first free orb. This ensures even split: ~2-3 bots per orb for 9 bots.
    // Stable assignment: same bot always prefers the same orb as long as it's free.
    uint32 preferredSlot = player->GetGUID().GetCounter() % TempleOfKotmogu::ORB_COUNT;
    uint32 bestOrbId = TempleOfKotmogu::ORB_COUNT; // sentinel

    for (uint32 attempt = 0; attempt < TempleOfKotmogu::ORB_COUNT; ++attempt)
    {
        uint32 checkSlot = (preferredSlot + attempt) % TempleOfKotmogu::ORB_COUNT;
        uint32 candidateOrb = orbPriority[checkSlot];

        if (std::find(freeOrbs.begin(), freeOrbs.end(), candidateOrb) != freeOrbs.end())
        {
            bestOrbId = candidateOrb;
            break;
        }
    }

    if (bestOrbId >= TempleOfKotmogu::ORB_COUNT)
    {
        TC_LOG_DEBUG("playerbots.bg", "[TOK] {} no available orbs after slot assignment",
            player->GetName());
        return false;
    }

    Position bestOrbPos = GetDynamicOrbPosition(bestOrbId);
    float bestDist = player->GetExactDist(&bestOrbPos);

    TC_LOG_DEBUG("playerbots.bg", "[TOK] {} targeting {} (slot {}, dist: {:.1f})",
        player->GetName(), TempleOfKotmogu::GetOrbName(bestOrbId), preferredSlot, bestDist);

    // Move toward the orb if too far
    if (bestDist > TOK_OBJECTIVE_RANGE)
    {
        // Engage nearby enemies while traveling — initiate combat so ClassAI
        // fires abilities, but always continue movement toward the orb
        ::Playerbot::BattlegroundCoordinator* coord =
            sBGCoordinatorMgr->GetCoordinatorForPlayer(player);
        if (coord)
        {
            float enemyDist = 0.0f;
            auto const* nearestEnemy = coord->GetNearestEnemy(
                player->GetPosition(), 15.0f, player->GetBGTeam(), player->GetGUID(), &enemyDist);

            if (nearestEnemy && nearestEnemy->isAlive)
            {
                ::Player* enemy = ObjectAccessor::FindPlayer(nearestEnemy->guid);
                if (enemy && enemy->IsAlive())
                {
                    player->SetSelection(enemy->GetGUID());
                    if (!player->IsInCombat() || player->GetVictim() != enemy)
                        player->Attack(enemy, true);

                    TC_LOG_DEBUG("playerbots.bg", "[TOK] {} engaging {} en route to {} (dist: {:.1f})",
                        player->GetName(), enemy->GetName(),
                        TempleOfKotmogu::GetOrbName(bestOrbId), enemyDist);
                }
            }
        }

        BotMovementUtil::MoveToPosition(player, bestOrbPos);
        return true;
    }

    // Within range - search for the orb GameObject and use it
    // CRITICAL: Use phase-ignoring search because dynamically spawned BG orbs
    // may not share the bot's PhaseShift. The standard GetGameObjectListWithEntryInGrid
    // filters by phase in its VisitImpl(), which silently drops dynamically created GOs.
    uint32 orbEntry = TOK_ORB_ENTRIES[bestOrbId];
    std::list<GameObject*> goList;

    FindGameObjectOptions options;
    options.GameObjectId = orbEntry;
    options.IgnorePhases = true;
    options.IsSpawned.reset();  // Don't filter by spawn state - dynamic GOs may differ
    options.IgnoreNotOwnedPrivateObjects = false;
    options.IgnorePrivateObjects = false;

    constexpr float ORB_SEARCH_RADIUS = 30.0f;  // Larger radius to account for position variance
    player->GetGameObjectListWithOptionsInGrid(goList, ORB_SEARCH_RADIUS, options);

    TC_LOG_DEBUG("playerbots.bg", "[TOK] {} searching for {} (entry {}) within {:.0f}yd: found {} GOs",
        player->GetName(), TempleOfKotmogu::GetOrbName(bestOrbId), orbEntry,
        ORB_SEARCH_RADIUS, goList.size());

    GameObject* bestGO = nullptr;
    float bestGODist = ORB_SEARCH_RADIUS + 1.0f;

    for (GameObject* go : goList)
    {
        if (!go)
            continue;

        float goDist = player->GetExactDist(go);

        TC_LOG_DEBUG("playerbots.bg", "[TOK]   GO entry={} guid={} type={} dist={:.1f} state={}",
            go->GetEntry(), go->GetGUID().GetCounter(),
            go->GetGOInfo() ? static_cast<uint32>(go->GetGOInfo()->type) : 999u,
            goDist,
            static_cast<uint32>(go->GetGoState()));

        GameObjectTemplate const* goInfo = go->GetGOInfo();
        if (!goInfo)
            continue;

        // Orbs are GAMEOBJECT_TYPE_FLAGSTAND in TOK
        if (goInfo->type != GAMEOBJECT_TYPE_FLAGSTAND && goInfo->type != GAMEOBJECT_TYPE_GOOBER)
            continue;

        if (goDist < bestGODist)
        {
            bestGODist = goDist;
            bestGO = go;
        }
    }

    if (bestGO)
    {
        // Move closer if still too far to interact
        if (bestGODist > TOK_OBJECTIVE_RANGE)
        {
            BotMovementUtil::MoveToPosition(player, bestGO->GetPosition());
            TC_LOG_DEBUG("playerbots.bg", "[TOK] {} found {} GO but too far ({:.1f}yd), moving closer",
                player->GetName(), TempleOfKotmogu::GetOrbName(bestOrbId), bestGODist);
            return true;
        }

        // CRITICAL FIX: Do NOT call bestGO->Use(player) directly!
        // This runs on a bot WORKER THREAD. Use() triggers the core BG script
        // (battleground_temple_of_kotmogu::OnFlagTaken) which calls
        // Map::UpdateSpawnGroupConditions → DespawnAll → Unit::RemoveAllAuras.
        // These Map operations are NOT thread-safe and cause ABORT in _UnapplyAura
        // when multiple bot workers modify the same map concurrently.
        //
        // Solution: Queue the interaction as a deferred action to be executed
        // on the main thread by BotActionMgr::ProcessActions().
        {
            BotAction action;
            action.type = BotActionType::INTERACT_OBJECT;
            action.botGuid = player->GetGUID();
            action.targetGuid = bestGO->GetGUID();
            action.queuedTime = GameTime::GetGameTimeMS();
            action.priority = 10;  // High priority - orb pickup is time-sensitive
            sBotActionMgr->QueueAction(std::move(action));
        }

        // Mark orb as claimed for 3 seconds to prevent race condition where
        // another bot also calls Use() before the aura is detected by RefreshOrbState()
        m_orbClaimedUntil[bestOrbId] = GameTime::GetGameTimeMS() + 3000;
        // Clear any search-failed cooldown since we found and used the GO
        m_orbSearchFailed.erase(bestOrbId);
        // Successfully picked up - remove from targeters (now a carrier)
        m_orbTargeters.erase(bestOrbId);

        // CRITICAL: Record pending pickup so ExecuteStrategy holds this bot at the
        // orb position until the main thread processes Use(). Without this, the bot
        // moves away on the next worker tick and the spell range check fails.
        m_pendingOrbPickup[player->GetGUID()] = PendingPickup{
            bestOrbId,
            bestGO->GetPosition(),
            GameTime::GetGameTimeMS()
        };

        TC_LOG_INFO("playerbots.bg", "[TOK] {} queued pickup of {} (entry {}, dist {:.1f})",
            player->GetName(), TempleOfKotmogu::GetOrbName(bestOrbId), orbEntry, bestGODist);
        return true;
    }

    // At orb location but no GO found - the orb was already picked up by someone
    // or the dynamic GO hasn't respawned/registered in grid yet.
    // Clear our targeting so we don't loop back to the same empty spot
    m_orbTargeters.erase(bestOrbId);

    // Mark this orb as search-failed for 3 seconds. This prevents the infinite loop
    // where bots re-target a "free" orb every tick but find 0 GOs each time.
    // Was 15s but that's too long — at BG start, orb GOs may spawn slightly late
    // and a 15s cooldown means the orb sits unclaimed for the entire opening phase.
    m_orbSearchFailed[bestOrbId] = GameTime::GetGameTimeMS() + 3000;

    TC_LOG_DEBUG("playerbots.bg", "[TOK] {} at orb location but no GO found for {} - orb was taken, cooldown 15s",
        player->GetName(), TempleOfKotmogu::GetOrbName(bestOrbId));

    // Engage any nearby enemies at the contested orb location instead of standing idle
    {
        ::Playerbot::BattlegroundCoordinator* coord =
            sBGCoordinatorMgr->GetCoordinatorForPlayer(player);
        if (coord)
        {
            float enemyDist = 0.0f;
            auto const* nearestEnemy = coord->GetNearestEnemy(
                player->GetPosition(), 20.0f, player->GetBGTeam(), player->GetGUID(), &enemyDist);

            if (nearestEnemy && nearestEnemy->isAlive)
            {
                ::Player* enemy = ObjectAccessor::FindPlayer(nearestEnemy->guid);
                if (enemy && enemy->IsAlive())
                {
                    player->SetSelection(enemy->GetGUID());
                    if (!player->IsInCombat() || player->GetVictim() != enemy)
                        player->Attack(enemy, true);
                    if (enemyDist > 5.0f)
                        BotMovementUtil::ChaseTarget(player, enemy, 5.0f);

                    TC_LOG_DEBUG("playerbots.bg", "[TOK] {} fighting enemy {} at contested orb location (dist: {:.1f})",
                        player->GetName(), enemy->GetName(), enemyDist);
                    return true;
                }
            }
        }
    }

    return false;
}

bool TempleOfKotmoguScript::DefendOrbCarrier(::Player* player)
{
    if (!player || !player->IsInWorld() || !player->IsAlive())
        return false;

    ::Playerbot::BattlegroundCoordinator* coordinator =
        sBGCoordinatorMgr->GetCoordinatorForPlayer(player);

    // =========================================================================
    // PHASE 1: Find nearest friendly orb carrier
    // =========================================================================
    ::Player* friendlyCarrier = nullptr;
    float carrierDist = std::numeric_limits<float>::max();

    for (uint32 orbId = 0; orbId < TempleOfKotmogu::ORB_COUNT; ++orbId)
    {
        if (!IsOrbHeld(orbId))
            continue;

        ObjectGuid holderGuid = GetOrbHolder(orbId);
        if (holderGuid.IsEmpty())
            continue;

        ::Player* holder = ObjectAccessor::FindPlayer(holderGuid);
        if (!holder || !holder->IsAlive() || holder->IsHostileTo(player))
            continue; // skip enemy carriers

        float dist = player->GetExactDist(holder);
        if (dist < carrierDist)
        {
            carrierDist = dist;
            friendlyCarrier = holder;
        }
    }

    // =========================================================================
    // PHASE 2: If friendly carrier found, defend them
    // =========================================================================
    if (friendlyCarrier)
    {
        TC_LOG_DEBUG("playerbots.bg", "[TOK] {} defending carrier {} (dist: {:.1f})",
            player->GetName(), friendlyCarrier->GetName(), carrierDist);

        // If too far from carrier, move closer
        if (carrierDist > TOK_DEFENSE_ESCORT_RANGE)
        {
            BotMovementUtil::MoveToPosition(player, friendlyCarrier->GetPosition());
            return true;
        }

        // Check for enemies near the carrier — engage them
        if (coordinator)
        {
            auto nearbyEnemies = coordinator->QueryNearbyEnemies(
                friendlyCarrier->GetPosition(), TOK_DEFENSE_ESCORT_RANGE, player->GetBGTeam());

            ::Player* closestThreat = nullptr;
            float closestThreatDist = TOK_DEFENSE_ESCORT_RANGE + 1.0f;

            for (auto const* snapshot : nearbyEnemies)
            {
                if (!snapshot || !snapshot->isAlive)
                    continue;

                ::Player* enemy = ObjectAccessor::FindPlayer(snapshot->guid);
                if (!enemy || !enemy->IsAlive())
                    continue;

                float dist = player->GetExactDist(enemy);
                if (dist < closestThreatDist)
                {
                    closestThreatDist = dist;
                    closestThreat = enemy;
                }
            }

            if (closestThreat)
            {
                player->SetSelection(closestThreat->GetGUID());
                if (!player->IsInCombat() || player->GetVictim() != closestThreat)
                    player->Attack(closestThreat, true);
                if (closestThreatDist > 5.0f)
                    BotMovementUtil::ChaseTarget(player, closestThreat, 5.0f);

                TC_LOG_DEBUG("playerbots.bg", "[TOK] {} engaging threat {} near carrier (dist: {:.1f})",
                    player->GetName(), closestThreat->GetName(), closestThreatDist);
                return true;
            }
        }
        else
        {
            // Fallback: legacy O(n) enemy search near carrier
            std::list<Player*> nearbyPlayers;
            friendlyCarrier->GetPlayerListInGrid(nearbyPlayers, TOK_DEFENSE_ESCORT_RANGE);

            ::Player* closestThreat = nullptr;
            float closestThreatDist = TOK_DEFENSE_ESCORT_RANGE + 1.0f;

            for (::Player* nearby : nearbyPlayers)
            {
                if (!nearby || !nearby->IsAlive() || !nearby->IsHostileTo(player))
                    continue;
                float dist = player->GetExactDist(nearby);
                if (dist < closestThreatDist)
                {
                    closestThreatDist = dist;
                    closestThreat = nearby;
                }
            }

            if (closestThreat)
            {
                player->SetSelection(closestThreat->GetGUID());
                if (!player->IsInCombat() || player->GetVictim() != closestThreat)
                    player->Attack(closestThreat, true);
                if (closestThreatDist > 5.0f)
                    BotMovementUtil::ChaseTarget(player, closestThreat, 5.0f);

                TC_LOG_DEBUG("playerbots.bg", "[TOK] {} engaging threat {} near carrier (legacy, dist: {:.1f})",
                    player->GetName(), closestThreat->GetName(), closestThreatDist);
                return true;
            }
        }

        // No threats — maintain escort distance behind carrier
        if (carrierDist > TOK_ESCORT_DISTANCE * 1.5f || !BotMovementUtil::IsMoving(player))
        {
            float angle = friendlyCarrier->GetOrientation() + static_cast<float>(M_PI);
            Position escortPos;
            escortPos.Relocate(
                friendlyCarrier->GetPositionX() + TOK_ESCORT_DISTANCE * 0.7f * std::cos(angle),
                friendlyCarrier->GetPositionY() + TOK_ESCORT_DISTANCE * 0.7f * std::sin(angle),
                friendlyCarrier->GetPositionZ()
            );
            BotMovementUtil::CorrectPositionToGround(player, escortPos);
            BotMovementUtil::MoveToPosition(player, escortPos);
        }
        return true;
    }

    // =========================================================================
    // PHASE 3: No friendly carrier — patrol center and fight enemies
    // =========================================================================
    TC_LOG_DEBUG("playerbots.bg", "[TOK] {} no friendly carrier found, patrolling center",
        player->GetName());

    // Look for enemies to fight while patrolling
    if (coordinator)
    {
        float enemyDist = 0.0f;
        auto const* nearestEnemy = coordinator->GetNearestEnemy(
            player->GetPosition(), 30.0f, player->GetBGTeam(), player->GetGUID(), &enemyDist);

        if (nearestEnemy && nearestEnemy->isAlive)
        {
            ::Player* enemy = ObjectAccessor::FindPlayer(nearestEnemy->guid);
            if (enemy && enemy->IsAlive())
            {
                player->SetSelection(enemy->GetGUID());
                if (!player->IsInCombat() || player->GetVictim() != enemy)
                    player->Attack(enemy, true);
                if (enemyDist > 5.0f)
                    BotMovementUtil::ChaseTarget(player, enemy, 5.0f);

                TC_LOG_DEBUG("playerbots.bg", "[TOK] {} engaging enemy {} while patrolling center (dist: {:.1f})",
                    player->GetName(), enemy->GetName(), enemyDist);
                return true;
            }
        }
    }

    // No enemies - move toward center
    if (!BotMovementUtil::IsMoving(player))
    {
        Position patrolPos;
        float angle = frand(0.0f, 2.0f * static_cast<float>(M_PI));
        float dist = frand(5.0f, 15.0f);
        patrolPos.Relocate(
            TempleOfKotmogu::CENTER_X + dist * std::cos(angle),
            TempleOfKotmogu::CENTER_Y + dist * std::sin(angle),
            TempleOfKotmogu::CENTER_Z
        );
        BotMovementUtil::CorrectPositionToGround(player, patrolPos);
        BotMovementUtil::MoveToPosition(player, patrolPos);
    }
    return true;
}

bool TempleOfKotmoguScript::HuntEnemyOrbCarrier(::Player* player)
{
    if (!player || !player->IsInWorld() || !player->IsAlive())
        return false;

    ::Playerbot::BattlegroundCoordinator* coordinator =
        sBGCoordinatorMgr->GetCoordinatorForPlayer(player);

    // =========================================================================
    // PHASE 1: Find enemy orb carriers via script orb tracking
    // =========================================================================
    ::Player* bestTarget = nullptr;
    float bestDist = std::numeric_limits<float>::max();

    for (uint32 orbId = 0; orbId < TempleOfKotmogu::ORB_COUNT; ++orbId)
    {
        if (!IsOrbHeld(orbId))
            continue;

        ObjectGuid holderGuid = GetOrbHolder(orbId);
        if (holderGuid.IsEmpty())
            continue;

        ::Player* holder = ObjectAccessor::FindPlayer(holderGuid);
        if (!holder || !holder->IsAlive() || !holder->IsHostileTo(player))
            continue;

        float dist = player->GetExactDist(holder);

        // Prefer carriers in center zone (they score more points)
        bool inCenter = IsInCenter(holder->GetPositionX(), holder->GetPositionY());
        if (inCenter)
            dist *= 0.5f; // Effectively double priority for center carriers

        if (dist < bestDist)
        {
            bestDist = dist;
            bestTarget = holder;
        }
    }

    // =========================================================================
    // PHASE 2: If enemy carrier found, chase and engage
    // =========================================================================
    if (bestTarget)
    {
        float actualDist = player->GetExactDist(bestTarget);

        TC_LOG_DEBUG("playerbots.bg", "[TOK] {} hunting enemy orb carrier {} (dist: {:.1f})",
            player->GetName(), bestTarget->GetName(), actualDist);

        player->SetSelection(bestTarget->GetGUID());

        if (actualDist > 30.0f)
        {
            BotMovementUtil::MoveToPosition(player, bestTarget->GetPosition());
        }
        else
        {
            // Initiate auto-attack so ClassAI combat rotation kicks in
            if (!player->IsInCombat() || player->GetVictim() != bestTarget)
                player->Attack(bestTarget, true);

            if (actualDist > 5.0f)
                BotMovementUtil::ChaseTarget(player, bestTarget, 5.0f);
        }
        return true;
    }

    // =========================================================================
    // PHASE 3: No enemy carrier found — attack nearest enemy via spatial cache
    // =========================================================================
    if (coordinator)
    {
        float enemyDist = 0.0f;
        auto const* nearestEnemy = coordinator->GetNearestEnemy(
            player->GetPosition(), 40.0f, player->GetBGTeam(), player->GetGUID(), &enemyDist);

        if (nearestEnemy && nearestEnemy->isAlive)
        {
            ::Player* enemy = ObjectAccessor::FindPlayer(nearestEnemy->guid);
            if (enemy && enemy->IsAlive())
            {
                player->SetSelection(enemy->GetGUID());
                if (!player->IsInCombat() || player->GetVictim() != enemy)
                    player->Attack(enemy, true);
                if (enemyDist > 5.0f)
                    BotMovementUtil::ChaseTarget(player, enemy, 5.0f);

                TC_LOG_DEBUG("playerbots.bg", "[TOK] {} no enemy carrier, engaging nearby enemy {} (dist: {:.1f})",
                    player->GetName(), enemy->GetName(), enemyDist);
                return true;
            }
        }
    }
    else
    {
        // Fallback: legacy O(n) search if no coordinator
        std::list<Player*> nearbyPlayers;
        player->GetPlayerListInGrid(nearbyPlayers, 40.0f);

        ::Player* closestEnemy = nullptr;
        float closestDist = 41.0f;
        for (::Player* nearby : nearbyPlayers)
        {
            if (!nearby || !nearby->IsAlive() || !nearby->IsHostileTo(player))
                continue;
            float dist = player->GetExactDist(nearby);
            if (dist < closestDist)
            {
                closestDist = dist;
                closestEnemy = nearby;
            }
        }

        if (closestEnemy)
        {
            player->SetSelection(closestEnemy->GetGUID());
            if (!player->IsInCombat() || player->GetVictim() != closestEnemy)
                player->Attack(closestEnemy, true);
            if (closestDist > 5.0f)
                BotMovementUtil::ChaseTarget(player, closestEnemy, 5.0f);

            TC_LOG_DEBUG("playerbots.bg", "[TOK] {} engaging nearby enemy {} (legacy, dist: {:.1f})",
                player->GetName(), closestEnemy->GetName(), closestDist);
            return true;
        }
    }

    // =========================================================================
    // PHASE 4: No enemies nearby — move toward center
    // =========================================================================
    TC_LOG_DEBUG("playerbots.bg", "[TOK] {} no enemies found, moving toward center",
        player->GetName());
    Position centerPos(TempleOfKotmogu::CENTER_X, TempleOfKotmogu::CENTER_Y, TempleOfKotmogu::CENTER_Z, 0.0f);
    BotMovementUtil::MoveToPosition(player, centerPos);
    return true;
}

bool TempleOfKotmoguScript::EscortOrbCarrier(::Player* player)
{
    if (!player || !player->IsInWorld() || !player->IsAlive())
        return false;

    ::Playerbot::BattlegroundCoordinator* coordinator =
        sBGCoordinatorMgr->GetCoordinatorForPlayer(player);

    // =========================================================================
    // PHASE 1: Find nearest friendly orb carrier
    // =========================================================================
    ::Player* friendlyCarrier = nullptr;
    float carrierDist = std::numeric_limits<float>::max();

    for (uint32 orbId = 0; orbId < TempleOfKotmogu::ORB_COUNT; ++orbId)
    {
        if (!IsOrbHeld(orbId))
            continue;

        ObjectGuid holderGuid = GetOrbHolder(orbId);
        if (holderGuid.IsEmpty())
            continue;

        ::Player* holder = ObjectAccessor::FindPlayer(holderGuid);
        if (!holder || !holder->IsAlive() || holder->IsHostileTo(player))
            continue; // skip enemy carriers

        float dist = player->GetExactDist(holder);
        if (dist < carrierDist)
        {
            carrierDist = dist;
            friendlyCarrier = holder;
        }
    }

    // =========================================================================
    // PHASE 2: If carrier found, take escort formation
    // =========================================================================
    if (friendlyCarrier)
    {
        TC_LOG_DEBUG("playerbots.bg", "[TOK] {} escorting carrier {} (dist: {:.1f})",
            player->GetName(), friendlyCarrier->GetName(), carrierDist);

        // If too far, just run toward the carrier
        if (carrierDist > TOK_MAX_ESCORT_DISTANCE)
        {
            BotMovementUtil::MoveToPosition(player, friendlyCarrier->GetPosition());
            return true;
        }

        // Try to get formation position from script
        Position escortPos;
        if (carrierDist < TOK_MAX_ESCORT_DISTANCE)
        {
            auto formation = GetEscortFormation(
                friendlyCarrier->GetPositionX(),
                friendlyCarrier->GetPositionY(),
                friendlyCarrier->GetPositionZ());

            if (!formation.empty())
            {
                uint32 idx = player->GetGUID().GetCounter() % formation.size();
                escortPos = formation[idx];
                BotMovementUtil::CorrectPositionToGround(player, escortPos);
            }
        }

        // Fallback: offset behind carrier using angle
        if (escortPos.GetPositionX() == 0.0f)
        {
            float angle = friendlyCarrier->GetOrientation() + static_cast<float>(M_PI);
            escortPos.Relocate(
                friendlyCarrier->GetPositionX() + TOK_ESCORT_DISTANCE * 0.7f * std::cos(angle),
                friendlyCarrier->GetPositionY() + TOK_ESCORT_DISTANCE * 0.7f * std::sin(angle),
                friendlyCarrier->GetPositionZ()
            );
            BotMovementUtil::CorrectPositionToGround(player, escortPos);
        }

        // Proactively engage nearby enemies near the carrier
        // Don't wait until carrier is already being attacked — intercept threats early
        if (coordinator)
        {
            auto nearbyEnemies = coordinator->QueryNearbyEnemies(
                friendlyCarrier->GetPosition(), 20.0f, player->GetBGTeam());

            ::Player* closestEnemy = nullptr;
            float closestEnemyDist = 21.0f;

            for (auto const* snapshot : nearbyEnemies)
            {
                if (!snapshot || !snapshot->isAlive)
                    continue;

                ::Player* enemy = ObjectAccessor::FindPlayer(snapshot->guid);
                if (!enemy || !enemy->IsAlive())
                    continue;

                float dist = player->GetExactDist(enemy);
                if (dist < closestEnemyDist)
                {
                    closestEnemyDist = dist;
                    closestEnemy = enemy;
                }
            }

            if (closestEnemy)
            {
                player->SetSelection(closestEnemy->GetGUID());
                if (!player->IsInCombat() || player->GetVictim() != closestEnemy)
                    player->Attack(closestEnemy, true);
                if (closestEnemyDist > 5.0f)
                    BotMovementUtil::ChaseTarget(player, closestEnemy, 5.0f);

                TC_LOG_DEBUG("playerbots.bg", "[TOK] {} proactively engaging {} near carrier (dist: {:.1f})",
                    player->GetName(), closestEnemy->GetName(), closestEnemyDist);
                return true;
            }
        }
        else
        {
            // Fallback: legacy O(n) search near carrier
            std::list<Player*> nearbyPlayers;
            friendlyCarrier->GetPlayerListInGrid(nearbyPlayers, 20.0f);

            for (::Player* nearby : nearbyPlayers)
            {
                if (nearby && nearby->IsAlive() && nearby->IsHostileTo(player))
                {
                    player->SetSelection(nearby->GetGUID());
                    if (!player->IsInCombat() || player->GetVictim() != nearby)
                        player->Attack(nearby, true);
                    if (player->GetExactDist(nearby) > 5.0f)
                        BotMovementUtil::ChaseTarget(player, nearby, 5.0f);

                    TC_LOG_DEBUG("playerbots.bg", "[TOK] {} proactively engaging {} near carrier (legacy)",
                        player->GetName(), nearby->GetName());
                    return true;
                }
            }
        }

        // No enemies near carrier — maintain escort formation
        if (carrierDist > TOK_ESCORT_DISTANCE * 1.5f || !BotMovementUtil::IsMoving(player))
            BotMovementUtil::MoveToPosition(player, escortPos);

        return true;
    }

    // =========================================================================
    // PHASE 3: No friendly carrier — fall back to defend behavior
    // =========================================================================
    TC_LOG_DEBUG("playerbots.bg", "[TOK] {} no friendly carrier to escort, falling back to defend",
        player->GetName());
    return DefendOrbCarrier(player);
}

bool TempleOfKotmoguScript::ExecuteOrbCarrierMovement(::Player* player)
{
    if (!player || !player->IsInWorld() || !player->IsAlive())
        return false;

    ::Playerbot::BattlegroundCoordinator* coordinator =
        sBGCoordinatorMgr->GetCoordinatorForPlayer(player);

    // Determine which orb this player is carrying
    int32 orbId = GetCarriedOrbId(player);
    if (orbId < 0)
    {
        TC_LOG_DEBUG("playerbots.bg", "[TOK] {} ExecuteOrbCarrierMovement called but not carrying orb",
            player->GetName());
        return false;
    }

    bool inCenter = IsInCenter(player->GetPositionX(), player->GetPositionY());

    // =========================================================================
    // SURVIVAL CHECK: If health low and outnumbered, retreat
    // =========================================================================
    float healthPct = player->GetHealthPct();

    if (healthPct < TOK_LOW_HEALTH_PCT && coordinator)
    {
        Position playerPos = player->GetPosition();
        uint32 nearbyEnemies = coordinator->CountEnemiesInRadius(playerPos, 30.0f, player->GetBGTeam());
        uint32 nearbyAllies = coordinator->CountAlliesInRadius(playerPos, 30.0f, player->GetBGTeam());

        if (nearbyEnemies > nearbyAllies)
        {
            // Retreat toward nearest ally cluster
            auto const* nearestAlly = coordinator->GetNearestAlly(
                playerPos, 60.0f, player->GetBGTeam(), player->GetGUID());

            if (nearestAlly)
            {
                ::Player* ally = ObjectAccessor::FindPlayer(nearestAlly->guid);
                if (ally && ally->IsAlive())
                {
                    TC_LOG_DEBUG("playerbots.bg", "[TOK] {} carrier LOW HP ({:.0f}%), retreating toward {} (enemies={} allies={})",
                        player->GetName(), healthPct, ally->GetName(), nearbyEnemies, nearbyAllies);
                    BotMovementUtil::MoveToPosition(player, ally->GetPosition());
                    return true;
                }
            }

            // No ally found — retreat toward own spawn
            Position spawnPos = (player->GetBGTeam() == ALLIANCE)
                ? Position(TempleOfKotmogu::ALLIANCE_SPAWNS[0])
                : Position(TempleOfKotmogu::HORDE_SPAWNS[0]);

            TC_LOG_DEBUG("playerbots.bg", "[TOK] {} carrier LOW HP ({:.0f}%), retreating to spawn",
                player->GetName(), healthPct);
            BotMovementUtil::MoveToPosition(player, spawnPos);
            return true;
        }
    }

    // =========================================================================
    // ALWAYS PUSH TO CENTER - center gives 3-6x more points per tick
    // This is THE core strategy of Temple of Kotmogu
    // =========================================================================
    if (inCenter)
    {
        // Already in center — hold position and fight nearby enemies
        TC_LOG_DEBUG("playerbots.bg", "[TOK] {} carrier holding center with {}",
            player->GetName(), TempleOfKotmogu::GetOrbName(static_cast<uint32>(orbId)));

        // KITING: When focused by multiple enemies and HP getting low, kite within center
        // This keeps the carrier alive longer while still scoring center-zone points
        if (healthPct < 60.0f && healthPct >= TOK_LOW_HEALTH_PCT && coordinator)
        {
            Position playerPos = player->GetPosition();
            uint32 nearbyEnemyCount = coordinator->CountEnemiesInRadius(playerPos, 10.0f, player->GetBGTeam());

            if (nearbyEnemyCount >= 2)
            {
                // Calculate average enemy position and move opposite direction
                auto enemies = coordinator->QueryNearbyEnemies(playerPos, 10.0f, player->GetBGTeam());
                float avgEX = 0.0f, avgEY = 0.0f;
                uint32 eCount = 0;

                for (auto const* snap : enemies)
                {
                    if (snap && snap->isAlive)
                    {
                        avgEX += snap->position.GetPositionX();
                        avgEY += snap->position.GetPositionY();
                        eCount++;
                    }
                }

                if (eCount > 0)
                {
                    avgEX /= eCount;
                    avgEY /= eCount;

                    float dx = player->GetPositionX() - avgEX;
                    float dy = player->GetPositionY() - avgEY;
                    float len = std::sqrt(dx * dx + dy * dy);

                    if (len > 0.1f)
                    {
                        dx /= len;
                        dy /= len;

                        // Move 12 yards away from enemy cluster, anchored to center
                        Position kitePos;
                        kitePos.Relocate(
                            TempleOfKotmogu::CENTER_X + dx * 12.0f,
                            TempleOfKotmogu::CENTER_Y + dy * 12.0f,
                            TempleOfKotmogu::CENTER_Z
                        );
                        BotMovementUtil::CorrectPositionToGround(player, kitePos);

                        // Only kite if we stay within center zone (don't lose points)
                        if (IsInCenter(kitePos.GetPositionX(), kitePos.GetPositionY()))
                        {
                            // Still fight the closest enemy while kiting
                            float enemyDist = 0.0f;
                            auto const* nearestEnemy = coordinator->GetNearestEnemy(
                                playerPos, 20.0f, player->GetBGTeam(), player->GetGUID(), &enemyDist);

                            if (nearestEnemy && nearestEnemy->isAlive)
                            {
                                ::Player* enemy = ObjectAccessor::FindPlayer(nearestEnemy->guid);
                                if (enemy && enemy->IsAlive())
                                {
                                    player->SetSelection(enemy->GetGUID());
                                    if (!player->IsInCombat() || player->GetVictim() != enemy)
                                        player->Attack(enemy, true);
                                }
                            }

                            BotMovementUtil::MoveToPosition(player, kitePos);
                            TC_LOG_DEBUG("playerbots.bg", "[TOK] {} carrier kiting within center (HP: {:.0f}%, enemies: {})",
                                player->GetName(), healthPct, nearbyEnemyCount);
                            return true;
                        }
                    }
                }
            }
        }

        // Attack nearby enemies while holding center
        if (coordinator)
        {
            float enemyDist = 0.0f;
            auto const* nearestEnemy = coordinator->GetNearestEnemy(
                player->GetPosition(), 20.0f, player->GetBGTeam(), player->GetGUID(), &enemyDist);

            if (nearestEnemy && nearestEnemy->isAlive)
            {
                ::Player* enemy = ObjectAccessor::FindPlayer(nearestEnemy->guid);
                if (enemy && enemy->IsAlive())
                {
                    player->SetSelection(enemy->GetGUID());
                    if (!player->IsInCombat() || player->GetVictim() != enemy)
                        player->Attack(enemy, true);
                    // Don't chase far from center - stay within the zone
                    if (enemyDist > 5.0f && enemyDist < TempleOfKotmogu::CENTER_RADIUS)
                        BotMovementUtil::ChaseTarget(player, enemy, 5.0f);
                    return true;
                }
            }
        }

        // No enemies - small random movement to avoid being static
        if (!BotMovementUtil::IsMoving(player))
        {
            float angle = frand(0.0f, 2.0f * static_cast<float>(M_PI));
            float dist = frand(2.0f, 8.0f);
            Position holdPos;
            holdPos.Relocate(
                TempleOfKotmogu::CENTER_X + dist * std::cos(angle),
                TempleOfKotmogu::CENTER_Y + dist * std::sin(angle),
                TempleOfKotmogu::CENTER_Z
            );
            BotMovementUtil::CorrectPositionToGround(player, holdPos);
            BotMovementUtil::MoveToPosition(player, holdPos);
        }
        return true;
    }

    // Not in center yet — navigate along route to center
    {
        // Initiate combat with nearby enemies while traveling — but NEVER stop moving.
        // The carrier's #1 goal is reaching center for maximum scoring. Attack starts
        // combat so the class AI can cast abilities, but we always continue moving.
        if (coordinator)
        {
            float enemyDist = 0.0f;
            auto const* nearestEnemy = coordinator->GetNearestEnemy(
                player->GetPosition(), 15.0f, player->GetBGTeam(), player->GetGUID(), &enemyDist);

            if (nearestEnemy && nearestEnemy->isAlive)
            {
                ::Player* enemy = ObjectAccessor::FindPlayer(nearestEnemy->guid);
                if (enemy && enemy->IsAlive())
                {
                    player->SetSelection(enemy->GetGUID());
                    if (!player->IsInCombat() || player->GetVictim() != enemy)
                        player->Attack(enemy, true);

                    TC_LOG_DEBUG("playerbots.bg", "[TOK] {} carrier attacking {} en route to center (dist: {:.1f})",
                        player->GetName(), enemy->GetName(), enemyDist);
                    // Fall through to waypoint movement — never stop for enemies
                }
            }
        }

        std::vector<Position> route = GetOrbCarrierRoute(static_cast<uint32>(orbId));

        if (!route.empty())
        {
            // Navigate to center, using intermediate waypoints only when they're
            // AHEAD of us (closer to center than we are). This prevents the oscillation
            // bug where a carrier at center re-targets a midway waypoint because it was
            // the first waypoint with dist > 10yd in a forward-only scan.
            Position targetWaypoint = route.back(); // default: center (last point)
            float playerToCenterDist = player->GetExactDist(&targetWaypoint);
            for (size_t i = 1; i < route.size() - 1; ++i) // intermediate waypoints only
            {
                Position wpPos = route[i];
                float wpToCenterDist = wpPos.GetExactDist(&route.back());
                // Only use this intermediate wp if we're farther from center than it is
                // (i.e., the waypoint is between us and center, not behind us)
                if (playerToCenterDist > wpToCenterDist + 5.0f)
                {
                    targetWaypoint = wpPos;
                    break;
                }
            }

            TC_LOG_DEBUG("playerbots.bg", "[TOK] {} carrier pushing to center with {} (dist: {:.1f})",
                player->GetName(), TempleOfKotmogu::GetOrbName(static_cast<uint32>(orbId)),
                player->GetExactDist(&targetWaypoint));

            BotMovementUtil::MoveToPosition(player, targetWaypoint);
            return true;
        }

        // Fallback: move directly to center
        Position centerPos(TempleOfKotmogu::CENTER_X, TempleOfKotmogu::CENTER_Y, TempleOfKotmogu::CENTER_Z, 0.0f);
        TC_LOG_DEBUG("playerbots.bg", "[TOK] {} carrier moving directly to center with {}",
            player->GetName(), TempleOfKotmogu::GetOrbName(static_cast<uint32>(orbId)));
        BotMovementUtil::MoveToPosition(player, centerPos);
        return true;
    }
}

} // namespace Playerbot::Coordination::Battleground
