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

#include "AlteracValleyScript.h"
#include "BGScriptRegistry.h"
#include "BattlegroundCoordinator.h"
#include "BGState.h"
#include "BotActionManager.h"
#include "Creature.h"
#include "Player.h"
#include "BotMovementUtil.h"
#include "Log.h"
#include "Timer.h"
#include "ObjectAccessor.h"

namespace Playerbot::Coordination::Battleground
{

REGISTER_BG_SCRIPT(AlteracValleyScript, 30);  // AlteracValley::MAP_ID

// ============================================================================
// LIFECYCLE
// ============================================================================

void AlteracValleyScript::OnLoad(BattlegroundCoordinator* coordinator)
{
    SiegeScriptBase::OnLoad(coordinator);

    m_cachedObjectives = GetObjectiveData();

    // Register reinforcement world states
    RegisterScoreWorldState(AlteracValley::WorldStates::REINF_ALLY, true);
    RegisterScoreWorldState(AlteracValley::WorldStates::REINF_HORDE, false);

    // Initialize all towers as standing
    m_towerStanding.fill(true);

    // Initialize graveyard control
    m_graveyardControl[AlteracValley::Graveyards::STORMPIKE_GY] = ALLIANCE;
    m_graveyardControl[AlteracValley::Graveyards::STORMPIKE_AID_STATION] = ALLIANCE;
    m_graveyardControl[AlteracValley::Graveyards::STONEHEARTH_GY] = ALLIANCE;
    m_graveyardControl[AlteracValley::Graveyards::SNOWFALL_GY] = 0;  // Neutral
    m_graveyardControl[AlteracValley::Graveyards::ICEBLOOD_GY] = HORDE;
    m_graveyardControl[AlteracValley::Graveyards::FROSTWOLF_GY] = HORDE;
    m_graveyardControl[AlteracValley::Graveyards::FROSTWOLF_RELIEF_HUT] = HORDE;

    // Captains and bosses alive
    m_balindaAlive = true;
    m_galvangarAlive = true;
    m_vanndarAlive = true;
    m_drektharAlive = true;

    // Initialize reinforcements
    m_allianceReinforcements = AlteracValley::STARTING_REINFORCEMENTS;
    m_hordeReinforcements = AlteracValley::STARTING_REINFORCEMENTS;

    TC_LOG_DEBUG("playerbots.bg.script",
        "AlteracValleyScript: Loaded (8 towers, 7 graveyards, 2 captains, 2 bosses)");
}

void AlteracValleyScript::OnMatchStart()
{
    SiegeScriptBase::OnMatchStart();

    m_matchStartTime = getMSTime();
    m_lastStrategyUpdate = m_matchStartTime;
    m_lastTowerCheck = m_matchStartTime;

    TC_LOG_INFO("playerbots.bg.script",
        "AlteracValleyScript: Match started - Alliance: {} reinforcements, Horde: {} reinforcements",
        m_allianceReinforcements, m_hordeReinforcements);
}

void AlteracValleyScript::OnMatchEnd(bool victory)
{
    SiegeScriptBase::OnMatchEnd(victory);

    uint32 duration = getMSTime() - m_matchStartTime;
    uint32 allyTowersDestroyed = 4 - GetStandingTowerCount(ALLIANCE);
    uint32 hordeTowersDestroyed = 4 - GetStandingTowerCount(HORDE);

    TC_LOG_INFO("playerbots.bg.script",
        "AlteracValleyScript: Match ended - {} | Duration: {}ms | "
        "Alliance: {} reinforcements, {} towers destroyed | "
        "Horde: {} reinforcements, {} towers destroyed | "
        "Balinda: {} | Galvangar: {} | Vanndar: {} | Drek'Thar: {}",
        victory ? "VICTORY" : "DEFEAT",
        duration,
        m_allianceReinforcements, allyTowersDestroyed,
        m_hordeReinforcements, hordeTowersDestroyed,
        m_balindaAlive ? "Alive" : "Dead",
        m_galvangarAlive ? "Alive" : "Dead",
        m_vanndarAlive ? "Alive" : "Dead",
        m_drektharAlive ? "Alive" : "Dead");
}

void AlteracValleyScript::OnUpdate(uint32 diff)
{
    SiegeScriptBase::OnUpdate(diff);

    uint32 now = getMSTime();

    // Resolve boss GUIDs once on main thread (safe for grid scans)
    if (!m_bossGuidsResolved && m_coordinator)
    {
        // Get any bot player in the BG for grid lookup context
        auto bots = m_coordinator->GetAllBots();
        if (!bots.empty())
        {
            if (::Player* anyBot = ObjectAccessor::FindPlayer(bots.front().guid))
            {
                if (::Creature* vanndar = anyBot->FindNearestCreature(
                    AlteracValley::Bosses::VANNDAR_ENTRY, 5000.0f, true))
                    m_vanndarGuid = vanndar->GetGUID();
                if (::Creature* drekthar = anyBot->FindNearestCreature(
                    AlteracValley::Bosses::DREKTHAR_ENTRY, 5000.0f, true))
                    m_drektharGuid = drekthar->GetGUID();

                m_bossGuidsResolved = true;
                TC_LOG_DEBUG("playerbots.bg.script",
                    "[AV] Boss GUIDs resolved - Vanndar: {}, Drek'Thar: {}",
                    m_vanndarGuid.IsEmpty() ? "not found" : "found",
                    m_drektharGuid.IsEmpty() ? "not found" : "found");
            }
        }
    }

    // Update tower states periodically
    if (now - m_lastTowerCheck >= AlteracValley::Strategy::TOWER_CHECK_INTERVAL)
    {
        m_lastTowerCheck = now;
        UpdateTowerStates();
        UpdateGraveyardStates();
    }

    // Update strategy periodically
    if (now - m_lastStrategyUpdate >= AlteracValley::Strategy::STRATEGY_UPDATE_INTERVAL)
    {
        m_lastStrategyUpdate = now;

        AVPhase phase = GetCurrentPhase();
        TC_LOG_DEBUG("playerbots.bg.script",
            "AlteracValleyScript: Phase update - {} | Alliance: {} | Horde: {}",
            GetPhaseName(phase), m_allianceReinforcements, m_hordeReinforcements);
    }
}

void AlteracValleyScript::OnEvent(const BGScriptEventData& event)
{
    SiegeScriptBase::OnEvent(event);

    switch (event.eventType)
    {
        case BGScriptEvent::OBJECTIVE_CAPTURED:
        {
            // Tower captured (being burned)
            if (event.objectiveId < AlteracValley::Towers::COUNT)
            {
                TC_LOG_INFO("playerbots.bg.script",
                    "AlteracValleyScript: Tower {} captured by {} - burning started",
                    AlteracValley::GetTowerName(event.objectiveId),
                    event.newState == BGObjectiveState::ALLIANCE_CONTROLLED ? "Alliance" : "Horde");
            }
            // Graveyard captured
            else if (event.objectiveId >= AlteracValley::ObjectiveIds::GY_STORMPIKE &&
                     event.objectiveId <= AlteracValley::ObjectiveIds::GY_FROSTWOLF_HUT)
            {
                uint32 gyIndex = event.objectiveId - AlteracValley::ObjectiveIds::GY_STORMPIKE;
                m_graveyardControl[gyIndex] =
                    (event.newState == BGObjectiveState::ALLIANCE_CONTROLLED) ? ALLIANCE : HORDE;

                TC_LOG_INFO("playerbots.bg.script",
                    "AlteracValleyScript: Graveyard {} captured by {}",
                    AlteracValley::GetGraveyardName(gyIndex),
                    event.newState == BGObjectiveState::ALLIANCE_CONTROLLED ? "Alliance" : "Horde");
            }
            break;
        }

        case BGScriptEvent::TOWER_DESTROYED:
        {
            // Tower destroyed (burned down)
            if (event.objectiveId < AlteracValley::Towers::COUNT)
            {
                m_towerStanding[event.objectiveId] = false;

                uint32 faction = AlteracValley::IsAllianceTower(event.objectiveId) ? ALLIANCE : HORDE;
                if (faction == ALLIANCE)
                    m_allianceReinforcements = std::max(0u,
                        m_allianceReinforcements - AlteracValley::REINF_LOSS_PER_TOWER);
                else
                    m_hordeReinforcements = std::max(0u,
                        m_hordeReinforcements - AlteracValley::REINF_LOSS_PER_TOWER);

                TC_LOG_INFO("playerbots.bg.script",
                    "AlteracValleyScript: Tower {} DESTROYED - {} loses {} reinforcements (now: {})",
                    AlteracValley::GetTowerName(event.objectiveId),
                    faction == ALLIANCE ? "Alliance" : "Horde",
                    AlteracValley::REINF_LOSS_PER_TOWER,
                    faction == ALLIANCE ? m_allianceReinforcements : m_hordeReinforcements);
            }
            break;
        }

        case BGScriptEvent::PLAYER_KILLED:
        {
            // Reinforcement loss on player death
            // Note: In actual BG, this is handled by world state updates
            break;
        }

        case BGScriptEvent::BOSS_KILLED:
        {
            if (event.objectiveId == AlteracValley::ObjectiveIds::VANNDAR)
            {
                m_vanndarAlive = false;
                TC_LOG_INFO("playerbots.bg.script",
                    "AlteracValleyScript: VANNDAR KILLED - Horde wins!");
            }
            else if (event.objectiveId == AlteracValley::ObjectiveIds::DREKTHAR)
            {
                m_drektharAlive = false;
                TC_LOG_INFO("playerbots.bg.script",
                    "AlteracValleyScript: DREK'THAR KILLED - Alliance wins!");
            }
            break;
        }

        case BGScriptEvent::CUSTOM_EVENT:
        {
            // Captain kills
            if (event.objectiveId == AlteracValley::ObjectiveIds::BALINDA)
            {
                m_balindaAlive = false;
                TC_LOG_INFO("playerbots.bg.script",
                    "AlteracValleyScript: Balinda killed - Horde bonus!");
            }
            else if (event.objectiveId == AlteracValley::ObjectiveIds::GALVANGAR)
            {
                m_galvangarAlive = false;
                TC_LOG_INFO("playerbots.bg.script",
                    "AlteracValleyScript: Galvangar killed - Alliance bonus!");
            }
            break;
        }

        default:
            break;
    }
}

// ============================================================================
// RUNTIME BEHAVIOR
// ============================================================================

bool AlteracValleyScript::ExecuteStrategy(::Player* player)
{
    if (!player || !player->IsInWorld() || !player->IsAlive())
        return false;

    uint32 faction = player->GetBGTeam();
    uint32 enemyFaction = (faction == ALLIANCE) ? HORDE : ALLIANCE;
    AVPhase phase = GetCurrentPhase();

    // =========================================================================
    // PRIORITY 1: Enemy player within 20yd -> engage immediately
    // =========================================================================
    if (::Player* enemy = FindNearestEnemyPlayer(player, 20.0f))
    {
        TC_LOG_DEBUG("playerbots.bg.script",
            "[AV] {} PRIORITY 1: engaging enemy {} within 20yd",
            player->GetName(), enemy->GetName());
        EngageTarget(player, enemy);
        return true;
    }

    // =========================================================================
    // PRIORITY 2: Capturable objective within 30yd -> interact with it
    // =========================================================================
    // AV objectives are captured via CAPTURE_POINT game objects (towers & GYs)
    if (TryInteractWithGameObject(player, 29 /*GAMEOBJECT_TYPE_CAPTURE_POINT*/, 30.0f))
    {
        TC_LOG_DEBUG("playerbots.bg.script",
            "[AV] {} PRIORITY 2: interacting with nearby capture point",
            player->GetName());
        return true;
    }

    // =========================================================================
    // PRIORITY 3: Phase-based strategic behavior (GUID-hash duty split)
    // =========================================================================
    uint32 dutySlot = player->GetGUID().GetCounter() % 10;

    switch (phase)
    {
        // -----------------------------------------------------------------
        // OPENING: 70% rush forward toward enemy objectives, 30% defend home
        // -----------------------------------------------------------------
        case AVPhase::OPENING:
        {
            if (dutySlot < 7) // 70% rushers
            {
                // Move toward first uncontrolled enemy objective along rush route
                auto rushRoute = GetRushRoute(faction);
                if (!rushRoute.empty())
                {
                    // Pick a waypoint roughly based on progress (don't all stack at start)
                    // Use dutySlot to spread bots across the rush route
                    uint32 waypointIdx = std::min(static_cast<uint32>(dutySlot % rushRoute.size()),
                        static_cast<uint32>(rushRoute.size() - 1));
                    Position target = rushRoute[waypointIdx];

                    TC_LOG_DEBUG("playerbots.bg.script",
                        "[AV] {} OPENING: rushing forward (waypoint {})",
                        player->GetName(), waypointIdx);
                    Playerbot::BotMovementUtil::MoveToPosition(player, target);
                    return true;
                }
            }
            else // 30% defenders
            {
                // Defend home towers
                std::vector<uint32> homeTowers;
                for (uint32 i = 0; i < AlteracValley::Towers::COUNT; ++i)
                {
                    if (!m_towerStanding[i])
                        continue;
                    bool ourTower = (faction == ALLIANCE) ? AlteracValley::IsAllianceTower(i) :
                                                             AlteracValley::IsHordeTower(i);
                    if (ourTower)
                        homeTowers.push_back(i);
                }

                if (!homeTowers.empty())
                {
                    uint32 towerIdx = dutySlot % homeTowers.size();
                    auto defPositions = GetTowerDefensePositions(homeTowers[towerIdx]);
                    if (!defPositions.empty())
                    {
                        uint32 posIdx = player->GetGUID().GetCounter() % defPositions.size();
                        TC_LOG_DEBUG("playerbots.bg.script",
                            "[AV] {} OPENING: defending home tower {}",
                            player->GetName(), AlteracValley::GetTowerName(homeTowers[towerIdx]));
                        PatrolAroundPosition(player, defPositions[posIdx], 3.0f, 10.0f);
                        return true;
                    }
                }
            }
            break;
        }

        // -----------------------------------------------------------------
        // TOWER_BURN: 60% attack enemy towers, 40% defend own towers
        // -----------------------------------------------------------------
        case AVPhase::TOWER_BURN:
        {
            if (dutySlot < 6) // 60% attack enemy towers
            {
                auto burnOrder = GetTowerBurnOrder(faction);
                if (!burnOrder.empty())
                {
                    // Spread attackers across the first 2 burn targets
                    uint32 towerTarget = burnOrder[dutySlot % std::min(size_t(2), burnOrder.size())];
                    Position towerPos = AlteracValley::GetTowerPosition(towerTarget);

                    float dist = player->GetExactDist(&towerPos);
                    if (dist < 15.0f)
                    {
                        // At the tower - try to interact with the capture point
                        if (!TryInteractWithGameObject(player, 29 /*CAPTURE_POINT*/, 15.0f))
                        {
                            // No interactable GO, patrol the tower
                            auto defPos = GetTowerDefensePositions(towerTarget);
                            if (!defPos.empty())
                            {
                                uint32 posIdx = player->GetGUID().GetCounter() % defPos.size();
                                PatrolAroundPosition(player, defPos[posIdx], 3.0f, 8.0f);
                            }
                        }
                        TC_LOG_DEBUG("playerbots.bg.script",
                            "[AV] {} TOWER_BURN: at tower {}, capping/fighting",
                            player->GetName(), AlteracValley::GetTowerName(towerTarget));
                    }
                    else
                    {
                        TC_LOG_DEBUG("playerbots.bg.script",
                            "[AV] {} TOWER_BURN: moving to enemy tower {} (dist={:.0f})",
                            player->GetName(), AlteracValley::GetTowerName(towerTarget), dist);
                        Playerbot::BotMovementUtil::MoveToPosition(player, towerPos);
                    }
                    return true;
                }
            }
            else // 40% defend friendly towers
            {
                std::vector<uint32> friendlyTowers;
                for (uint32 i = 0; i < AlteracValley::Towers::COUNT; ++i)
                {
                    if (!m_towerStanding[i])
                        continue;
                    bool ourTower = (faction == ALLIANCE) ? AlteracValley::IsAllianceTower(i) :
                                                             AlteracValley::IsHordeTower(i);
                    if (ourTower)
                        friendlyTowers.push_back(i);
                }

                if (!friendlyTowers.empty())
                {
                    uint32 towerIdx = dutySlot % friendlyTowers.size();
                    auto defPositions = GetTowerDefensePositions(friendlyTowers[towerIdx]);
                    if (!defPositions.empty())
                    {
                        uint32 posIdx = player->GetGUID().GetCounter() % defPositions.size();
                        TC_LOG_DEBUG("playerbots.bg.script",
                            "[AV] {} TOWER_BURN: defending friendly tower {}",
                            player->GetName(), AlteracValley::GetTowerName(friendlyTowers[towerIdx]));
                        PatrolAroundPosition(player, defPositions[posIdx], 3.0f, 10.0f);
                        return true;
                    }
                }
            }
            break;
        }

        // -----------------------------------------------------------------
        // GRAVEYARD_PUSH: 70% capture enemy GYs, 30% defend captured
        // -----------------------------------------------------------------
        case AVPhase::GRAVEYARD_PUSH:
        {
            if (dutySlot < 7) // 70% capture enemy graveyards
            {
                // Find nearest enemy or neutral graveyard to push
                float bestDist = std::numeric_limits<float>::max();
                uint32 bestGY = UINT32_MAX;

                for (uint32 i = 0; i < AlteracValley::Graveyards::COUNT; ++i)
                {
                    uint32 control = m_graveyardControl[i];
                    if (control == faction)
                        continue; // Already ours

                    Position gyPos = AlteracValley::GetGraveyardPosition(i);
                    float dist = player->GetExactDist(&gyPos);
                    if (dist < bestDist)
                    {
                        bestDist = dist;
                        bestGY = i;
                    }
                }

                if (bestGY != UINT32_MAX)
                {
                    Position gyPos = AlteracValley::GetGraveyardPosition(bestGY);
                    if (bestDist < 15.0f)
                    {
                        // At the GY - try to interact
                        if (!TryInteractWithGameObject(player, 29 /*CAPTURE_POINT*/, 15.0f))
                        {
                            auto defPos = GetGraveyardDefensePositions(bestGY);
                            if (!defPos.empty())
                            {
                                uint32 posIdx = player->GetGUID().GetCounter() % defPos.size();
                                PatrolAroundPosition(player, defPos[posIdx], 3.0f, 8.0f);
                            }
                        }
                        TC_LOG_DEBUG("playerbots.bg.script",
                            "[AV] {} GY_PUSH: capping graveyard {}",
                            player->GetName(), AlteracValley::GetGraveyardName(bestGY));
                    }
                    else
                    {
                        TC_LOG_DEBUG("playerbots.bg.script",
                            "[AV] {} GY_PUSH: moving to enemy GY {} (dist={:.0f})",
                            player->GetName(), AlteracValley::GetGraveyardName(bestGY), bestDist);
                        Playerbot::BotMovementUtil::MoveToPosition(player, gyPos);
                    }
                    return true;
                }
            }
            else // 30% defend captured positions
            {
                // Defend our graveyards
                std::vector<uint32> friendlyGYs;
                for (uint32 i = 0; i < AlteracValley::Graveyards::COUNT; ++i)
                {
                    if (m_graveyardControl[i] == faction)
                        friendlyGYs.push_back(i);
                }

                if (!friendlyGYs.empty())
                {
                    uint32 gyIdx = dutySlot % friendlyGYs.size();
                    auto defPositions = GetGraveyardDefensePositions(friendlyGYs[gyIdx]);
                    if (!defPositions.empty())
                    {
                        uint32 posIdx = player->GetGUID().GetCounter() % defPositions.size();
                        TC_LOG_DEBUG("playerbots.bg.script",
                            "[AV] {} GY_PUSH: defending friendly GY {}",
                            player->GetName(), AlteracValley::GetGraveyardName(friendlyGYs[gyIdx]));
                        PatrolAroundPosition(player, defPositions[posIdx], 3.0f, 10.0f);
                        return true;
                    }
                }
            }
            break;
        }

        // -----------------------------------------------------------------
        // BOSS_ASSAULT: 90% rush enemy boss, 10% defend critical positions
        // -----------------------------------------------------------------
        case AVPhase::BOSS_ASSAULT:
        {
            if (dutySlot < 9) // 90% rush enemy boss
            {
                Position bossPos = GetBossPosition(enemyFaction);
                auto raidPositions = GetBossRaidPositions(enemyFaction);

                float distToBoss = player->GetExactDist(&bossPos);
                if (distToBoss < 40.0f && !raidPositions.empty())
                {
                    // In boss room - engage enemy players first, then boss NPC
                    if (::Player* enemy = FindNearestEnemyPlayer(player, 30.0f))
                    {
                        TC_LOG_DEBUG("playerbots.bg.script",
                            "[AV] {} BOSS_ASSAULT: engaging enemy player {} in boss room",
                            player->GetName(), enemy->GetName());
                        EngageTarget(player, enemy);
                    }
                    else
                    {
                        // No enemy players — target the boss NPC via deferred action
                        QueueBossAttack(player, enemyFaction);
                    }

                    // Take a raid position while fighting
                    uint32 posIdx = player->GetGUID().GetCounter() % raidPositions.size();
                    PatrolAroundPosition(player, raidPositions[posIdx], 1.0f, 5.0f);
                }
                else
                {
                    TC_LOG_DEBUG("playerbots.bg.script",
                        "[AV] {} BOSS_ASSAULT: rushing to enemy boss (dist={:.0f})",
                        player->GetName(), distToBoss);
                    Playerbot::BotMovementUtil::MoveToPosition(player, bossPos);
                }
                return true;
            }
            else // 10% defend critical positions (our boss)
            {
                Position ourBossPos = GetBossPosition(faction);
                auto ourRaidPos = GetBossRaidPositions(faction);
                if (!ourRaidPos.empty())
                {
                    uint32 posIdx = player->GetGUID().GetCounter() % ourRaidPos.size();
                    TC_LOG_DEBUG("playerbots.bg.script",
                        "[AV] {} BOSS_ASSAULT: defending our boss room",
                        player->GetName());
                    PatrolAroundPosition(player, ourRaidPos[posIdx], 3.0f, 10.0f);
                    return true;
                }
            }
            break;
        }

        // -----------------------------------------------------------------
        // DEFENSE: 80% defend home objectives, 20% counter-attack
        // -----------------------------------------------------------------
        case AVPhase::DEFENSE:
        {
            if (dutySlot < 8) // 80% defend home objectives
            {
                // Build list of all home objectives to defend (towers + GYs)
                std::vector<Position> defenseTargets;
                std::vector<std::string> defenseNames;

                // Home towers
                for (uint32 i = 0; i < AlteracValley::Towers::COUNT; ++i)
                {
                    if (!m_towerStanding[i])
                        continue;
                    bool ourTower = (faction == ALLIANCE) ? AlteracValley::IsAllianceTower(i) :
                                                             AlteracValley::IsHordeTower(i);
                    if (ourTower)
                    {
                        auto defPos = GetTowerDefensePositions(i);
                        if (!defPos.empty())
                        {
                            uint32 posIdx = player->GetGUID().GetCounter() % defPos.size();
                            defenseTargets.push_back(defPos[posIdx]);
                            defenseNames.push_back(AlteracValley::GetTowerName(i));
                        }
                    }
                }

                // Our graveyards
                for (uint32 i = 0; i < AlteracValley::Graveyards::COUNT; ++i)
                {
                    if (m_graveyardControl[i] == faction)
                    {
                        auto defPos = GetGraveyardDefensePositions(i);
                        if (!defPos.empty())
                        {
                            uint32 posIdx = player->GetGUID().GetCounter() % defPos.size();
                            defenseTargets.push_back(defPos[posIdx]);
                            defenseNames.push_back(AlteracValley::GetGraveyardName(i));
                        }
                    }
                }

                if (!defenseTargets.empty())
                {
                    uint32 targetIdx = dutySlot % defenseTargets.size();
                    TC_LOG_DEBUG("playerbots.bg.script",
                        "[AV] {} DEFENSE: defending {}",
                        player->GetName(), defenseNames[targetIdx]);
                    PatrolAroundPosition(player, defenseTargets[targetIdx], 3.0f, 12.0f);
                    return true;
                }
            }
            else // 20% counter-attack
            {
                // Find nearest enemy objective to push back
                float bestDist = std::numeric_limits<float>::max();
                Position bestTarget;
                bool found = false;

                for (uint32 i = 0; i < AlteracValley::Towers::COUNT; ++i)
                {
                    if (!m_towerStanding[i])
                        continue;
                    bool enemyTower = (faction == ALLIANCE) ? AlteracValley::IsHordeTower(i) :
                                                               AlteracValley::IsAllianceTower(i);
                    if (enemyTower)
                    {
                        Position towerPos = AlteracValley::GetTowerPosition(i);
                        float dist = player->GetExactDist(&towerPos);
                        if (dist < bestDist)
                        {
                            bestDist = dist;
                            bestTarget = towerPos;
                            found = true;
                        }
                    }
                }

                if (found)
                {
                    TC_LOG_DEBUG("playerbots.bg.script",
                        "[AV] {} DEFENSE: counter-attacking nearest enemy objective (dist={:.0f})",
                        player->GetName(), bestDist);
                    Playerbot::BotMovementUtil::MoveToPosition(player, bestTarget);
                    return true;
                }
            }
            break;
        }

        // -----------------------------------------------------------------
        // DESPERATE: 100% rush nearest enemy objective
        // -----------------------------------------------------------------
        case AVPhase::DESPERATE:
        {
            // Find the nearest enemy objective and rush it
            float bestDist = std::numeric_limits<float>::max();
            Position bestTarget;
            bool found = false;
            std::string targetName;

            // Check enemy towers
            for (uint32 i = 0; i < AlteracValley::Towers::COUNT; ++i)
            {
                if (!m_towerStanding[i])
                    continue;
                bool enemyTower = (faction == ALLIANCE) ? AlteracValley::IsHordeTower(i) :
                                                           AlteracValley::IsAllianceTower(i);
                if (enemyTower)
                {
                    Position towerPos = AlteracValley::GetTowerPosition(i);
                    float dist = player->GetExactDist(&towerPos);
                    if (dist < bestDist)
                    {
                        bestDist = dist;
                        bestTarget = towerPos;
                        found = true;
                        targetName = AlteracValley::GetTowerName(i);
                    }
                }
            }

            // Check enemy graveyards
            for (uint32 i = 0; i < AlteracValley::Graveyards::COUNT; ++i)
            {
                if (m_graveyardControl[i] == faction || m_graveyardControl[i] == 0)
                    continue; // Skip ours and neutral

                Position gyPos = AlteracValley::GetGraveyardPosition(i);
                float dist = player->GetExactDist(&gyPos);
                if (dist < bestDist)
                {
                    bestDist = dist;
                    bestTarget = gyPos;
                    found = true;
                    targetName = AlteracValley::GetGraveyardName(i);
                }
            }

            // Enemy boss as ultimate target
            Position bossPos = GetBossPosition(enemyFaction);
            float bossDist = player->GetExactDist(&bossPos);
            if (bossDist < bestDist)
            {
                bestTarget = bossPos;
                found = true;
                targetName = (enemyFaction == ALLIANCE) ? "Vanndar" : "Drek'Thar";
            }

            if (found)
            {
                TC_LOG_DEBUG("playerbots.bg.script",
                    "[AV] {} DESPERATE: rushing {}!",
                    player->GetName(), targetName);
                Playerbot::BotMovementUtil::MoveToPosition(player, bestTarget);
                return true;
            }
            break;
        }
    }

    // =========================================================================
    // PRIORITY 4: Fallback -> move toward nearest chokepoint
    // =========================================================================
    auto chokepoints = GetChokepoints();
    if (!chokepoints.empty())
    {
        // Pick a chokepoint based on GUID for spread
        uint32 idx = player->GetGUID().GetCounter() % chokepoints.size();
        TC_LOG_DEBUG("playerbots.bg.script",
            "[AV] {} FALLBACK: patrolling chokepoint {}",
            player->GetName(), idx);
        PatrolAroundPosition(player, chokepoints[idx], 5.0f, 15.0f);
        return true;
    }

    return true;
}

// ============================================================================
// DATA PROVIDERS
// ============================================================================

std::vector<BGObjectiveData> AlteracValleyScript::GetObjectiveData() const
{
    std::vector<BGObjectiveData> objectives;

    // Add towers
    auto towers = GetTowerData();
    objectives.insert(objectives.end(), towers.begin(), towers.end());

    // Add graveyards
    auto graveyards = GetGraveyardData();
    objectives.insert(objectives.end(), graveyards.begin(), graveyards.end());

    // Add bosses as objectives
    BGObjectiveData vanndar;
    vanndar.id = AlteracValley::ObjectiveIds::VANNDAR;
    vanndar.type = ObjectiveType::BOSS;
    vanndar.name = "Vanndar Stormpike";
    vanndar.x = AlteracValley::Bosses::VANNDAR_X;
    vanndar.y = AlteracValley::Bosses::VANNDAR_Y;
    vanndar.z = AlteracValley::Bosses::VANNDAR_Z;
    vanndar.strategicValue = 10;
    objectives.push_back(vanndar);

    BGObjectiveData drekthar;
    drekthar.id = AlteracValley::ObjectiveIds::DREKTHAR;
    drekthar.type = ObjectiveType::BOSS;
    drekthar.name = "Drek'Thar";
    drekthar.x = AlteracValley::Bosses::DREKTHAR_X;
    drekthar.y = AlteracValley::Bosses::DREKTHAR_Y;
    drekthar.z = AlteracValley::Bosses::DREKTHAR_Z;
    drekthar.strategicValue = 10;
    objectives.push_back(drekthar);

    // Add captains
    BGObjectiveData balinda;
    balinda.id = AlteracValley::ObjectiveIds::BALINDA;
    balinda.type = ObjectiveType::STRATEGIC;
    balinda.name = "Balinda Stonehearth";
    balinda.x = AlteracValley::Captains::BALINDA_X;
    balinda.y = AlteracValley::Captains::BALINDA_Y;
    balinda.z = AlteracValley::Captains::BALINDA_Z;
    balinda.strategicValue = 6;
    objectives.push_back(balinda);

    BGObjectiveData galvangar;
    galvangar.id = AlteracValley::ObjectiveIds::GALVANGAR;
    galvangar.type = ObjectiveType::STRATEGIC;
    galvangar.name = "Captain Galvangar";
    galvangar.x = AlteracValley::Captains::GALVANGAR_X;
    galvangar.y = AlteracValley::Captains::GALVANGAR_Y;
    galvangar.z = AlteracValley::Captains::GALVANGAR_Z;
    galvangar.strategicValue = 6;
    objectives.push_back(galvangar);

    return objectives;
}

std::vector<BGObjectiveData> AlteracValleyScript::GetTowerData() const
{
    std::vector<BGObjectiveData> towers;

    for (uint32 i = 0; i < AlteracValley::Towers::COUNT; ++i)
    {
        BGObjectiveData tower;
        tower.id = i;
        tower.type = ObjectiveType::TOWER;
        tower.name = AlteracValley::GetTowerName(i);
        tower.x = AlteracValley::TOWER_POSITIONS[i].GetPositionX();
        tower.y = AlteracValley::TOWER_POSITIONS[i].GetPositionY();
        tower.z = AlteracValley::TOWER_POSITIONS[i].GetPositionZ();
        tower.strategicValue = 8;
        tower.captureTime = AlteracValley::Strategy::TOWER_BURN_TIME;

        towers.push_back(tower);
    }

    return towers;
}

std::vector<BGObjectiveData> AlteracValleyScript::GetGraveyardData() const
{
    std::vector<BGObjectiveData> graveyards;

    for (uint32 i = 0; i < AlteracValley::Graveyards::COUNT; ++i)
    {
        BGObjectiveData gy;
        gy.id = AlteracValley::ObjectiveIds::GY_STORMPIKE + i;
        gy.type = ObjectiveType::GRAVEYARD;
        gy.name = AlteracValley::GetGraveyardName(i);
        gy.x = AlteracValley::GRAVEYARD_POSITIONS[i].GetPositionX();
        gy.y = AlteracValley::GRAVEYARD_POSITIONS[i].GetPositionY();
        gy.z = AlteracValley::GRAVEYARD_POSITIONS[i].GetPositionZ();
        gy.strategicValue = (i == AlteracValley::Graveyards::SNOWFALL_GY) ? 7 : 5;
        gy.captureTime = AlteracValley::Strategy::GY_CAPTURE_TIME;

        graveyards.push_back(gy);
    }

    return graveyards;
}

std::vector<BGObjectiveData> AlteracValleyScript::GetGateData() const
{
    // AV doesn't have destructible gates like SOTA/IOC
    return {};
}

std::vector<BGPositionData> AlteracValleyScript::GetSpawnPositions(uint32 faction) const
{
    std::vector<BGPositionData> spawns;

    if (faction == ALLIANCE)
    {
        for (const auto& pos : AlteracValley::ALLIANCE_SPAWNS)
        {
            BGPositionData spawn("Alliance Spawn", pos.GetPositionX(),
                pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(),
                BGPositionData::PositionType::SPAWN_POINT, ALLIANCE, 5);
            spawns.push_back(spawn);
        }
    }
    else
    {
        for (const auto& pos : AlteracValley::HORDE_SPAWNS)
        {
            BGPositionData spawn("Horde Spawn", pos.GetPositionX(),
                pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(),
                BGPositionData::PositionType::SPAWN_POINT, HORDE, 5);
            spawns.push_back(spawn);
        }
    }

    return spawns;
}

std::vector<BGPositionData> AlteracValleyScript::GetStrategicPositions() const
{
    std::vector<BGPositionData> positions;

    // Chokepoints
    auto chokepoints = GetChokepoints();
    for (const auto& pos : chokepoints)
    {
        BGPositionData p("Chokepoint", pos.GetPositionX(), pos.GetPositionY(),
            pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::CHOKEPOINT, 0, 7);
        positions.push_back(p);
    }

    // Sniper positions
    auto snipers = GetSniperPositions();
    for (const auto& pos : snipers)
    {
        BGPositionData p("Sniper Position", pos.GetPositionX(), pos.GetPositionY(),
            pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::SNIPER_POSITION, 0, 6);
        positions.push_back(p);
    }

    // Tower positions
    for (uint32 i = 0; i < AlteracValley::Towers::COUNT; ++i)
    {
        BGPositionData p(AlteracValley::GetTowerName(i),
            AlteracValley::TOWER_POSITIONS[i].GetPositionX(),
            AlteracValley::TOWER_POSITIONS[i].GetPositionY(),
            AlteracValley::TOWER_POSITIONS[i].GetPositionZ(),
            0.0f, BGPositionData::PositionType::DEFENSIVE_POSITION,
            AlteracValley::IsAllianceTower(i) ? ALLIANCE : HORDE, 8);
        positions.push_back(p);
    }

    // Boss positions
    positions.push_back(BGPositionData("Vanndar Stormpike",
        AlteracValley::Bosses::VANNDAR_X, AlteracValley::Bosses::VANNDAR_Y, AlteracValley::Bosses::VANNDAR_Z,
        0.0f, BGPositionData::PositionType::STRATEGIC_POINT, ALLIANCE, 10));

    positions.push_back(BGPositionData("Drek'Thar",
        AlteracValley::Bosses::DREKTHAR_X, AlteracValley::Bosses::DREKTHAR_Y, AlteracValley::Bosses::DREKTHAR_Z,
        0.0f, BGPositionData::PositionType::STRATEGIC_POINT, HORDE, 10));

    return positions;
}

std::vector<BGPositionData> AlteracValleyScript::GetGraveyardPositions(uint32 faction) const
{
    std::vector<BGPositionData> graveyards;

    for (uint32 i = 0; i < AlteracValley::Graveyards::COUNT; ++i)
    {
        uint32 gyFaction = 0;  // Neutral
        if (i == AlteracValley::Graveyards::STORMPIKE_GY ||
            i == AlteracValley::Graveyards::STORMPIKE_AID_STATION ||
            i == AlteracValley::Graveyards::STONEHEARTH_GY)
            gyFaction = ALLIANCE;
        else if (i == AlteracValley::Graveyards::ICEBLOOD_GY ||
                 i == AlteracValley::Graveyards::FROSTWOLF_GY ||
                 i == AlteracValley::Graveyards::FROSTWOLF_RELIEF_HUT)
            gyFaction = HORDE;

        if (faction == 0 || faction == gyFaction || gyFaction == 0)
        {
            BGPositionData p(AlteracValley::GetGraveyardName(i),
                AlteracValley::GRAVEYARD_POSITIONS[i].GetPositionX(),
                AlteracValley::GRAVEYARD_POSITIONS[i].GetPositionY(),
                AlteracValley::GRAVEYARD_POSITIONS[i].GetPositionZ(),
                0.0f, BGPositionData::PositionType::GRAVEYARD, gyFaction, 6);
            graveyards.push_back(p);
        }
    }

    return graveyards;
}

std::vector<BGWorldState> AlteracValleyScript::GetInitialWorldStates() const
{
    std::vector<BGWorldState> states;

    states.push_back(BGWorldState(AlteracValley::WorldStates::REINF_ALLY,
        "Alliance Reinforcements", BGWorldState::StateType::REINFORCEMENTS,
        AlteracValley::STARTING_REINFORCEMENTS));
    states.push_back(BGWorldState(AlteracValley::WorldStates::REINF_HORDE,
        "Horde Reinforcements", BGWorldState::StateType::REINFORCEMENTS,
        AlteracValley::STARTING_REINFORCEMENTS));

    return states;
}

// ============================================================================
// WORLD STATE
// ============================================================================

bool AlteracValleyScript::InterpretWorldState(int32 stateId, int32 value,
    uint32& outObjectiveId, BGObjectiveState& outState) const
{
    // Reinforcement states
    if (stateId == AlteracValley::WorldStates::REINF_ALLY ||
        stateId == AlteracValley::WorldStates::REINF_HORDE)
    {
        return false;  // Handled as score, not objective
    }

    // Tower world states
    for (uint32 i = 0; i < AlteracValley::Towers::COUNT; ++i)
    {
        if (stateId == AlteracValley::TOWER_WORLD_STATES[i].allyControlled && value)
        {
            outObjectiveId = i;
            outState = BGObjectiveState::ALLIANCE_CONTROLLED;
            return true;
        }
        if (stateId == AlteracValley::TOWER_WORLD_STATES[i].hordeControlled && value)
        {
            outObjectiveId = i;
            outState = BGObjectiveState::HORDE_CONTROLLED;
            return true;
        }
        if (stateId == AlteracValley::TOWER_WORLD_STATES[i].destroyed && value)
        {
            outObjectiveId = i;
            outState = BGObjectiveState::DESTROYED;
            return true;
        }
    }

    return TryInterpretFromCache(stateId, value, outObjectiveId, outState);
}

void AlteracValleyScript::GetScoreFromWorldStates(const std::map<int32, int32>& states,
    uint32& allianceScore, uint32& hordeScore) const
{
    // In AV, "score" is reinforcements
    allianceScore = AlteracValley::STARTING_REINFORCEMENTS;
    hordeScore = AlteracValley::STARTING_REINFORCEMENTS;

    auto allyIt = states.find(AlteracValley::WorldStates::REINF_ALLY);
    if (allyIt != states.end())
        allianceScore = static_cast<uint32>(std::max(0, allyIt->second));

    auto hordeIt = states.find(AlteracValley::WorldStates::REINF_HORDE);
    if (hordeIt != states.end())
        hordeScore = static_cast<uint32>(std::max(0, hordeIt->second));
}

// ============================================================================
// STRATEGY & ROLE DISTRIBUTION
// ============================================================================

RoleDistribution AlteracValleyScript::GetRecommendedRoles(const StrategicDecision& decision,
    float scoreAdvantage, uint32 timeRemaining) const
{
    RoleDistribution roles;
    AVPhase phase = GetCurrentPhase();

    switch (phase)
    {
        case AVPhase::OPENING:
            // Initial push - mostly offense
            roles.SetRole(BGRole::ATTACKER, 60, 70);
            roles.SetRole(BGRole::DEFENDER, 15, 25);
            roles.SetRole(BGRole::ROAMER, 10, 20);
            roles.SetRole(BGRole::GRAVEYARD_ASSAULT, 5, 10);
            break;

        case AVPhase::TOWER_BURN:
            // Tower burning phase - dedicated burn teams
            roles.SetRole(BGRole::ATTACKER, 50, 60);
            roles.SetRole(BGRole::DEFENDER, 25, 35);
            roles.SetRole(BGRole::ROAMER, 10, 15);
            roles.SetRole(BGRole::GRAVEYARD_ASSAULT, 5, 10);
            break;

        case AVPhase::GRAVEYARD_PUSH:
            // Pushing forward graveyards
            roles.SetRole(BGRole::ATTACKER, 55, 65);
            roles.SetRole(BGRole::DEFENDER, 20, 30);
            roles.SetRole(BGRole::ROAMER, 10, 15);
            roles.SetRole(BGRole::GRAVEYARD_ASSAULT, 5, 10);
            break;

        case AVPhase::BOSS_ASSAULT:
            // All-in boss kill
            roles.SetRole(BGRole::ATTACKER, 70, 85);
            roles.SetRole(BGRole::DEFENDER, 10, 20);
            roles.SetRole(BGRole::ROAMER, 5, 10);
            roles.SetRole(BGRole::GRAVEYARD_ASSAULT, 0, 5);
            break;

        case AVPhase::DEFENSE:
            // Defensive stance
            roles.SetRole(BGRole::ATTACKER, 25, 35);
            roles.SetRole(BGRole::DEFENDER, 50, 60);
            roles.SetRole(BGRole::ROAMER, 10, 15);
            roles.SetRole(BGRole::GRAVEYARD_ASSAULT, 5, 10);
            break;

        case AVPhase::DESPERATE:
            // Low reinforcements - all-in or defend
            if (scoreAdvantage > 0)
            {
                // Winning - defend and wait
                roles.SetRole(BGRole::ATTACKER, 20, 30);
                roles.SetRole(BGRole::DEFENDER, 55, 65);
                roles.SetRole(BGRole::ROAMER, 10, 15);
                roles.SetRole(BGRole::GRAVEYARD_ASSAULT, 5, 10);
            }
            else
            {
                // Losing - must push boss
                roles.SetRole(BGRole::ATTACKER, 80, 90);
                roles.SetRole(BGRole::DEFENDER, 5, 15);
                roles.SetRole(BGRole::ROAMER, 0, 5);
                roles.SetRole(BGRole::GRAVEYARD_ASSAULT, 0, 5);
            }
            break;
    }

    return roles;
}

void AlteracValleyScript::AdjustStrategy(StrategicDecision& decision,
    float scoreAdvantage, uint32 controlledCount,
    uint32 totalObjectives, uint32 timeRemaining) const
{
    // Get base siege strategy
    SiegeScriptBase::AdjustStrategy(decision, scoreAdvantage,
        controlledCount, totalObjectives, timeRemaining);

    uint32 faction = m_coordinator ? m_coordinator->GetFaction() : ALLIANCE;
    AVPhase phase = GetCurrentPhase();

    // Apply phase-specific strategy
    ApplyPhaseStrategy(decision, phase, faction);

    // Captain status affects strategy
    bool enemyCaptainAlive = (faction == ALLIANCE) ? m_galvangarAlive : m_balindaAlive;
    if (enemyCaptainAlive)
    {
        decision.reasoning += " (captain kill opportunity)";
    }

    TC_LOG_DEBUG("playerbots.bg.script",
        "AlteracValleyScript: Strategy adjusted - Phase: {} | Offense: {}% | Defense: {}% | {}",
        GetPhaseName(phase), decision.offenseAllocation, decision.defenseAllocation,
        decision.reasoning);
}

// ============================================================================
// PHASE MANAGEMENT
// ============================================================================

AlteracValleyScript::AVPhase AlteracValleyScript::GetCurrentPhase() const
{
    uint32 faction = m_coordinator ? m_coordinator->GetFaction() : ALLIANCE;
    uint32 ourReinf = (faction == ALLIANCE) ? m_allianceReinforcements : m_hordeReinforcements;
    uint32 theirReinf = (faction == ALLIANCE) ? m_hordeReinforcements : m_allianceReinforcements;
    uint32 theirTowers = GetStandingTowerCount(faction == ALLIANCE ? HORDE : ALLIANCE);
    uint32 ourTowers = GetStandingTowerCount(faction);

    uint32 elapsed = getMSTime() - m_matchStartTime;

    // Desperate phase - low reinforcements
    if (ourReinf <= AlteracValley::Strategy::REINF_DESPERATE_THRESHOLD ||
        theirReinf <= AlteracValley::Strategy::REINF_DESPERATE_THRESHOLD)
    {
        return AVPhase::DESPERATE;
    }

    // Opening phase - first few minutes
    if (elapsed < AlteracValley::Strategy::OPENING_PHASE_DURATION)
    {
        return AVPhase::OPENING;
    }

    // Defense phase - we're losing towers/graveyards
    if (ourTowers < 3 && theirTowers >= 3)
    {
        return AVPhase::DEFENSE;
    }

    // Boss assault - enemy towers low enough
    if (theirTowers <= 1 && IsBossViable(faction == ALLIANCE ? HORDE : ALLIANCE))
    {
        return AVPhase::BOSS_ASSAULT;
    }

    // Tower burn phase - enemy has towers to burn
    if (theirTowers > 2)
    {
        return AVPhase::TOWER_BURN;
    }

    // Default to graveyard push
    return AVPhase::GRAVEYARD_PUSH;
}

const char* AlteracValleyScript::GetPhaseName(AVPhase phase) const
{
    switch (phase)
    {
        case AVPhase::OPENING:        return "OPENING";
        case AVPhase::TOWER_BURN:     return "TOWER_BURN";
        case AVPhase::GRAVEYARD_PUSH: return "GRAVEYARD_PUSH";
        case AVPhase::BOSS_ASSAULT:   return "BOSS_ASSAULT";
        case AVPhase::DEFENSE:        return "DEFENSE";
        case AVPhase::DESPERATE:      return "DESPERATE";
        default:                      return "UNKNOWN";
    }
}

void AlteracValleyScript::ApplyPhaseStrategy(StrategicDecision& decision,
    AVPhase phase, uint32 faction) const
{
    switch (phase)
    {
        case AVPhase::OPENING:
            ApplyOpeningStrategy(decision, faction);
            break;
        case AVPhase::TOWER_BURN:
            ApplyTowerBurnStrategy(decision, faction);
            break;
        case AVPhase::GRAVEYARD_PUSH:
            ApplyGraveyardPushStrategy(decision, faction);
            break;
        case AVPhase::BOSS_ASSAULT:
            ApplyBossAssaultStrategy(decision, faction);
            break;
        case AVPhase::DEFENSE:
            ApplyDefensiveStrategy(decision, faction);
            break;
        case AVPhase::DESPERATE:
            ApplyDesperateStrategy(decision, faction);
            break;
    }
}

void AlteracValleyScript::ApplyOpeningStrategy(StrategicDecision& decision, uint32 faction) const
{
    decision.strategy = BGStrategy::AGGRESSIVE;
    decision.reasoning = "Opening phase - push forward";
    decision.offenseAllocation = 65;
    decision.defenseAllocation = 25;

    // Attack objectives - forward towers and Snowfall
    decision.attackObjectives.clear();
    if (faction == ALLIANCE)
    {
        decision.attackObjectives.push_back(AlteracValley::Towers::ICEBLOOD_TOWER);
        decision.attackObjectives.push_back(AlteracValley::Towers::TOWER_POINT);
        decision.attackObjectives.push_back(AlteracValley::ObjectiveIds::GY_SNOWFALL);
    }
    else
    {
        decision.attackObjectives.push_back(AlteracValley::Towers::STONEHEARTH_BUNKER);
        decision.attackObjectives.push_back(AlteracValley::Towers::ICEWING_BUNKER);
        decision.attackObjectives.push_back(AlteracValley::ObjectiveIds::GY_SNOWFALL);
    }

    // Defend our forward towers
    decision.defendObjectives.clear();
    if (faction == ALLIANCE)
    {
        decision.defendObjectives.push_back(AlteracValley::Towers::STONEHEARTH_BUNKER);
        decision.defendObjectives.push_back(AlteracValley::Towers::ICEWING_BUNKER);
    }
    else
    {
        decision.defendObjectives.push_back(AlteracValley::Towers::ICEBLOOD_TOWER);
        decision.defendObjectives.push_back(AlteracValley::Towers::TOWER_POINT);
    }
}

void AlteracValleyScript::ApplyTowerBurnStrategy(StrategicDecision& decision, uint32 faction) const
{
    decision.strategy = BGStrategy::BALANCED;
    decision.reasoning = "Tower burn phase - destroy enemy towers";
    decision.offenseAllocation = 55;
    decision.defenseAllocation = 35;

    // Get priority tower burn order
    auto burnOrder = GetTowerBurnOrder(faction);

    decision.attackObjectives.clear();
    for (size_t i = 0; i < std::min(size_t(2), burnOrder.size()); ++i)
    {
        decision.attackObjectives.push_back(burnOrder[i]);
    }

    // Defend our towers that are under attack or burning
    decision.defendObjectives.clear();
    for (uint32 i = 0; i < AlteracValley::Towers::COUNT; ++i)
    {
        bool ourTower = (faction == ALLIANCE) ? AlteracValley::IsAllianceTower(i) :
                                                 !AlteracValley::IsAllianceTower(i);
        if (ourTower && m_towerStanding[i])
        {
            decision.defendObjectives.push_back(i);
        }
    }
}

void AlteracValleyScript::ApplyGraveyardPushStrategy(StrategicDecision& decision, uint32 faction) const
{
    decision.strategy = BGStrategy::BALANCED;
    decision.reasoning = "Graveyard push - secure forward spawns";
    decision.offenseAllocation = 55;
    decision.defenseAllocation = 35;

    decision.attackObjectives.clear();
    decision.defendObjectives.clear();

    // Find graveyards to capture
    for (uint32 i = 0; i < AlteracValley::Graveyards::COUNT; ++i)
    {
        uint32 control = m_graveyardControl[i];
        uint32 objId = AlteracValley::ObjectiveIds::GY_STORMPIKE + i;

        if (control != faction && control != 0)
        {
            // Enemy graveyard - attack
            decision.attackObjectives.push_back(objId);
        }
        else if (control == faction)
        {
            // Our graveyard - defend
            decision.defendObjectives.push_back(objId);
        }
        else if (control == 0)
        {
            // Neutral (Snowfall) - prioritize
            decision.attackObjectives.insert(decision.attackObjectives.begin(), objId);
        }
    }
}

void AlteracValleyScript::ApplyBossAssaultStrategy(StrategicDecision& decision, uint32 faction) const
{
    decision.strategy = BGStrategy::ALL_IN;
    decision.reasoning = "BOSS ASSAULT - all-in on enemy boss!";
    decision.offenseAllocation = 80;
    decision.defenseAllocation = 15;

    decision.attackObjectives.clear();
    decision.defendObjectives.clear();

    // Primary target: enemy boss
    if (faction == ALLIANCE)
    {
        decision.attackObjectives.push_back(AlteracValley::ObjectiveIds::DREKTHAR);
        // Also target any remaining towers near boss
        if (m_towerStanding[AlteracValley::Towers::EAST_FROSTWOLF])
            decision.attackObjectives.push_back(AlteracValley::Towers::EAST_FROSTWOLF);
        if (m_towerStanding[AlteracValley::Towers::WEST_FROSTWOLF])
            decision.attackObjectives.push_back(AlteracValley::Towers::WEST_FROSTWOLF);
    }
    else
    {
        decision.attackObjectives.push_back(AlteracValley::ObjectiveIds::VANNDAR);
        if (m_towerStanding[AlteracValley::Towers::DUN_BALDAR_NORTH])
            decision.attackObjectives.push_back(AlteracValley::Towers::DUN_BALDAR_NORTH);
        if (m_towerStanding[AlteracValley::Towers::DUN_BALDAR_SOUTH])
            decision.attackObjectives.push_back(AlteracValley::Towers::DUN_BALDAR_SOUTH);
    }

    // Minimal defense on base
    decision.defendObjectives.push_back(
        faction == ALLIANCE ? AlteracValley::ObjectiveIds::VANNDAR :
                              AlteracValley::ObjectiveIds::DREKTHAR);
}

void AlteracValleyScript::ApplyDefensiveStrategy(StrategicDecision& decision, uint32 faction) const
{
    decision.strategy = BGStrategy::DEFENSIVE;
    decision.reasoning = "Defense phase - protect our towers and GYs";
    decision.offenseAllocation = 30;
    decision.defenseAllocation = 60;

    decision.attackObjectives.clear();
    decision.defendObjectives.clear();

    // Defend all our standing towers
    for (uint32 i = 0; i < AlteracValley::Towers::COUNT; ++i)
    {
        bool ourTower = (faction == ALLIANCE) ? AlteracValley::IsAllianceTower(i) :
                                                 !AlteracValley::IsAllianceTower(i);
        if (ourTower && m_towerStanding[i])
        {
            decision.defendObjectives.push_back(i);
        }
    }

    // Defend our graveyards
    for (uint32 i = 0; i < AlteracValley::Graveyards::COUNT; ++i)
    {
        if (m_graveyardControl[i] == faction)
        {
            decision.defendObjectives.push_back(AlteracValley::ObjectiveIds::GY_STORMPIKE + i);
        }
    }

    // Still try to cap Snowfall if neutral
    if (m_graveyardControl[AlteracValley::Graveyards::SNOWFALL_GY] == 0)
    {
        decision.attackObjectives.push_back(AlteracValley::ObjectiveIds::GY_SNOWFALL);
    }
}

void AlteracValleyScript::ApplyDesperateStrategy(StrategicDecision& decision, uint32 faction) const
{
    uint32 ourReinf = (faction == ALLIANCE) ? m_allianceReinforcements : m_hordeReinforcements;
    uint32 theirReinf = (faction == ALLIANCE) ? m_hordeReinforcements : m_allianceReinforcements;

    if (ourReinf > theirReinf)
    {
        // We're winning - turtle and wait
        decision.strategy = BGStrategy::TURTLE;
        decision.reasoning = "Desperate - we're ahead, TURTLE!";
        decision.offenseAllocation = 20;
        decision.defenseAllocation = 70;

        decision.attackObjectives.clear();
        decision.defendObjectives.clear();

        // Defend everything
        for (uint32 i = 0; i < AlteracValley::Towers::COUNT; ++i)
        {
            bool ourTower = (faction == ALLIANCE) ? AlteracValley::IsAllianceTower(i) :
                                                     !AlteracValley::IsAllianceTower(i);
            if (ourTower && m_towerStanding[i])
            {
                decision.defendObjectives.push_back(i);
            }
        }

        decision.defendObjectives.push_back(
            faction == ALLIANCE ? AlteracValley::ObjectiveIds::VANNDAR :
                                  AlteracValley::ObjectiveIds::DREKTHAR);
    }
    else
    {
        // We're losing - all-in boss rush
        decision.strategy = BGStrategy::ALL_IN;
        decision.reasoning = "Desperate - behind on reinforcements, BOSS RUSH!";
        decision.offenseAllocation = 90;
        decision.defenseAllocation = 10;

        decision.attackObjectives.clear();
        decision.defendObjectives.clear();

        // Rush enemy boss
        decision.attackObjectives.push_back(
            faction == ALLIANCE ? AlteracValley::ObjectiveIds::DREKTHAR :
                                  AlteracValley::ObjectiveIds::VANNDAR);
    }
}

// ============================================================================
// AV-SPECIFIC METHODS
// ============================================================================

uint32 AlteracValleyScript::GetReinforcements(uint32 faction) const
{
    return (faction == ALLIANCE) ? m_allianceReinforcements : m_hordeReinforcements;
}

bool AlteracValleyScript::IsTowerStanding(uint32 towerId) const
{
    if (towerId >= AlteracValley::Towers::COUNT)
        return false;
    return m_towerStanding[towerId];
}

uint32 AlteracValleyScript::GetStandingTowerCount(uint32 faction) const
{
    uint32 count = 0;
    for (uint32 i = 0; i < AlteracValley::Towers::COUNT; ++i)
    {
        if (!m_towerStanding[i])
            continue;

        bool isAlliance = AlteracValley::IsAllianceTower(i);
        if ((faction == ALLIANCE && isAlliance) || (faction == HORDE && !isAlliance))
            ++count;
    }
    return count;
}

uint32 AlteracValleyScript::GetDestroyedEnemyTowerCount(uint32 faction) const
{
    uint32 count = 0;
    for (uint32 i = 0; i < AlteracValley::Towers::COUNT; ++i)
    {
        if (m_towerStanding[i])
            continue;

        bool isAlliance = AlteracValley::IsAllianceTower(i);
        if ((faction == ALLIANCE && !isAlliance) || (faction == HORDE && isAlliance))
            ++count;
    }
    return count;
}

bool AlteracValleyScript::IsCaptainAlive(uint32 faction) const
{
    return faction == ALLIANCE ? m_balindaAlive : m_galvangarAlive;
}

bool AlteracValleyScript::IsBossViable(uint32 targetFaction) const
{
    // Boss is always "viable" but stronger with more towers
    // Consider viable when enemy has <= 2 towers
    return GetStandingTowerCount(targetFaction) <= 2;
}

std::vector<uint32> AlteracValleyScript::GetTowerBurnOrder(uint32 attackingFaction) const
{
    std::vector<uint32> order;

    if (attackingFaction == ALLIANCE)
    {
        // Alliance burns Horde towers from south to north (nearest to farthest)
        order = {
            AlteracValley::Towers::TOWER_POINT,
            AlteracValley::Towers::ICEBLOOD_TOWER,
            AlteracValley::Towers::EAST_FROSTWOLF,
            AlteracValley::Towers::WEST_FROSTWOLF
        };
    }
    else
    {
        // Horde burns Alliance bunkers from south to north
        order = {
            AlteracValley::Towers::STONEHEARTH_BUNKER,
            AlteracValley::Towers::ICEWING_BUNKER,
            AlteracValley::Towers::DUN_BALDAR_SOUTH,
            AlteracValley::Towers::DUN_BALDAR_NORTH
        };
    }

    // Filter out already destroyed towers
    std::vector<uint32> result;
    for (uint32 towerId : order)
    {
        if (m_towerStanding[towerId])
            result.push_back(towerId);
    }

    return result;
}

std::vector<Position> AlteracValleyScript::GetRushRoute(uint32 faction) const
{
    return AlteracValley::GetRushRoute(faction);
}

std::vector<Position> AlteracValleyScript::GetTowerBurnRoute(uint32 faction) const
{
    return AlteracValley::GetTowerBurnRoute(faction);
}

// ============================================================================
// ENTERPRISE-GRADE POSITIONING
// ============================================================================

std::vector<Position> AlteracValleyScript::GetTowerDefensePositions(uint32 towerId) const
{
    if (towerId >= AlteracValley::Towers::COUNT)
        return {};
    return AlteracValley::GetTowerDefensePositions(towerId);
}

std::vector<Position> AlteracValleyScript::GetGraveyardDefensePositions(uint32 graveyardId) const
{
    if (graveyardId >= AlteracValley::Graveyards::COUNT)
        return {};
    return AlteracValley::GetGraveyardDefensePositions(graveyardId);
}

std::vector<Position> AlteracValleyScript::GetChokepoints() const
{
    return AlteracValley::GetChokepoints();
}

std::vector<Position> AlteracValleyScript::GetSniperPositions() const
{
    return AlteracValley::GetSniperPositions();
}

std::vector<Position> AlteracValleyScript::GetAmbushPositions(uint32 faction) const
{
    return AlteracValley::GetAmbushPositions(faction);
}

std::vector<Position> AlteracValleyScript::GetBossRaidPositions(uint32 targetFaction) const
{
    return AlteracValley::GetBossRaidPositions(targetFaction);
}

Position AlteracValleyScript::GetCaptainPosition(uint32 faction) const
{
    return AlteracValley::GetCaptainPosition(faction);
}

// ============================================================================
// SIEGE IMPLEMENTATIONS
// ============================================================================

uint32 AlteracValleyScript::GetBossEntry(uint32 faction) const
{
    return faction == ALLIANCE ? AlteracValley::VANNDAR_ENTRY : AlteracValley::DREKTHAR_ENTRY;
}

Position AlteracValleyScript::GetBossPosition(uint32 faction) const
{
    return faction == ALLIANCE ?
        AlteracValley::GetVanndarPosition() :
        AlteracValley::GetDrekTharPosition();
}

bool AlteracValleyScript::CanAttackBoss(uint32 faction) const
{
    // In AV, boss can always be attacked, but is much stronger with towers up
    return true;
}

// ============================================================================
// HELPER METHODS
// ============================================================================

bool AlteracValleyScript::ShouldBurnTowers() const
{
    uint32 faction = m_coordinator ? m_coordinator->GetFaction() : ALLIANCE;
    uint32 theirTowers = GetStandingTowerCount(faction == ALLIANCE ? HORDE : ALLIANCE);
    return theirTowers > AlteracValley::Strategy::TOWER_BURN_THRESHOLD;
}

void AlteracValleyScript::UpdateTowerStates()
{
    // Tower states are updated via OnEvent when world states change
    // This method can be used for periodic validation if needed
}

void AlteracValleyScript::UpdateGraveyardStates()
{
    // Graveyard states are updated via OnEvent when world states change
    // This method can be used for periodic validation if needed
}

BGObjectiveData AlteracValleyScript::GetCaptainData(uint32 faction) const
{
    BGObjectiveData captain;

    if (faction == ALLIANCE)
    {
        captain.id = AlteracValley::ObjectiveIds::BALINDA;
        captain.type = ObjectiveType::STRATEGIC;
        captain.name = "Balinda Stonehearth";
        captain.x = AlteracValley::Captains::BALINDA_X;
        captain.y = AlteracValley::Captains::BALINDA_Y;
        captain.z = AlteracValley::Captains::BALINDA_Z;
        captain.strategicValue = 6;
    }
    else
    {
        captain.id = AlteracValley::ObjectiveIds::GALVANGAR;
        captain.type = ObjectiveType::STRATEGIC;
        captain.name = "Captain Galvangar";
        captain.x = AlteracValley::Captains::GALVANGAR_X;
        captain.y = AlteracValley::Captains::GALVANGAR_Y;
        captain.z = AlteracValley::Captains::GALVANGAR_Z;
        captain.strategicValue = 6;
    }

    return captain;
}

// ============================================================================
// BOSS TARGETING (Thread-safe via BotActionMgr)
// ============================================================================

void AlteracValleyScript::QueueBossAttack(::Player* bot, uint32 enemyFaction)
{
    // Get the cached boss GUID (resolved on main thread in OnUpdate)
    ObjectGuid bossGuid = (enemyFaction == ALLIANCE) ? m_vanndarGuid : m_drektharGuid;

    if (bossGuid.IsEmpty())
    {
        TC_LOG_DEBUG("playerbots.bg.script",
            "[AV] {} BOSS_ASSAULT: boss GUID not yet resolved for faction {}",
            bot->GetName(), enemyFaction == ALLIANCE ? "Alliance" : "Horde");
        return;
    }

    // Check if bot is already attacking the boss
    if (bot->GetVictim() && bot->GetVictim()->GetGUID() == bossGuid)
        return;

    // Queue attack via deferred action system (executes on main thread)
    sBotActionMgr->QueueAction(Playerbot::BotAction::AttackTarget(
        bot->GetGUID(), bossGuid, getMSTime()));

    TC_LOG_DEBUG("playerbots.bg.script",
        "[AV] {} BOSS_ASSAULT: queued attack on {} boss NPC",
        bot->GetName(), enemyFaction == ALLIANCE ? "Vanndar" : "Drek'Thar");
}

} // namespace Playerbot::Coordination::Battleground
