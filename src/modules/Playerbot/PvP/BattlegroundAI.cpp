/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BattlegroundAI.h"
#include "GameTime.h"
#include "Player.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "../AI/Coordination/Battleground/BattlegroundCoordinatorManager.h"
#include "../AI/Coordination/Battleground/BattlegroundCoordinator.h"
#include "../AI/Coordination/Battleground/Scripts/IBGScript.h"

namespace Playerbot
{

// ============================================================================
// SINGLETON
// ============================================================================

BattlegroundAI* BattlegroundAI::instance()
{
    static BattlegroundAI instance;
    return &instance;
}

BattlegroundAI::BattlegroundAI()
{
    TC_LOG_INFO("playerbot", "BattlegroundAI initialized");
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void BattlegroundAI::Initialize()
{
    TC_LOG_INFO("playerbot", "BattlegroundAI: Initialization complete (behavior delegated to BG scripts)");
}

// ============================================================================
// UPDATE - MAIN ENTRY POINT
// ============================================================================
// Called from BotAI::Update() for each bot in a battleground.
// Dispatches to the appropriate BG script's ExecuteStrategy() via
// the BattlegroundCoordinator system.

void BattlegroundAI::Update(::Player* player, uint32 /*diff*/)
{
    if (!player || !player->IsInWorld())
        return;

    // Check if player is in battleground
    ::Battleground* bg = GetPlayerBattleground(player);
    if (!bg)
        return;

    // Handle both prep phase (WAIT_JOIN) and active phase (IN_PROGRESS)
    BattlegroundStatus status = bg->GetStatus();
    if (status != STATUS_IN_PROGRESS && status != STATUS_WAIT_JOIN)
        return;

    // During prep phase, just wait at spawn - don't execute strategy yet
    if (status == STATUS_WAIT_JOIN)
        return;

    uint32 playerGuid = player->GetGUID().GetCounter();
    uint32 currentTime = GameTime::GetGameTimeMS();

    // Throttle updates (500ms for BG responsiveness)
    if (_lastUpdateTimes.count(playerGuid))
    {
        uint32 timeSinceLastUpdate = currentTime - _lastUpdateTimes[playerGuid];
        if (timeSinceLastUpdate < BG_UPDATE_INTERVAL)
            return;
    }

    _lastUpdateTimes[playerGuid] = currentTime;

    // =========================================================================
    // BATTLEGROUND COORDINATOR INTEGRATION
    // =========================================================================
    // Ensure bot is registered with the coordinator for spatial queries
    // and team awareness used by BG scripts.

    BattlegroundCoordinator* coordinator = sBGCoordinatorMgr->GetCoordinatorForPlayer(player);

    if (coordinator)
    {
        // If bot has no role, it's a late-joiner - register it with the coordinator
        BGRole assignedRole = coordinator->GetBotRole(player->GetGUID());
        if (assignedRole == BGRole::UNASSIGNED)
            coordinator->AddBot(player);
    }
    else
    {
        // No coordinator - try to create one through UpdateBot
        sBGCoordinatorMgr->UpdateBot(player, 0);
    }

    // =========================================================================
    // DISPATCH TO BG-SPECIFIC STRATEGY
    // =========================================================================
    // Each Execute*Strategy() delegates to the corresponding BG script's
    // ExecuteStrategy() dynamic behavior tree.

    BGType bgType = GetBattlegroundType(player);
    switch (bgType)
    {
        case BGType::WARSONG_GULCH:
        case BGType::TWIN_PEAKS:
            ExecuteWSGStrategy(player);
            break;

        case BGType::ARATHI_BASIN:
        case BGType::BATTLE_FOR_GILNEAS:
            ExecuteABStrategy(player);
            break;

        case BGType::ALTERAC_VALLEY:
            ExecuteAVStrategy(player);
            break;

        case BGType::EYE_OF_THE_STORM:
            ExecuteEOTSStrategy(player);
            break;

        case BGType::STRAND_OF_THE_ANCIENTS:
        case BGType::ISLE_OF_CONQUEST:
            ExecuteSiegeStrategy(player);
            break;

        case BGType::TEMPLE_OF_KOTMOGU:
            ExecuteKotmoguStrategy(player);
            break;

        case BGType::SILVERSHARD_MINES:
            ExecuteSilvershardStrategy(player);
            break;

        case BGType::DEEPWIND_GORGE:
            ExecuteDeepwindStrategy(player);
            break;

        case BGType::SEETHING_SHORE:
            ExecuteSeethingShoreStrategy(player);
            break;

        case BGType::ASHRAN:
            ExecuteAshranStrategy(player);
            break;

        default:
            break;
    }
}

// ============================================================================
// THIN DELEGATION METHODS
// ============================================================================
// Each method delegates to the BG script's ExecuteStrategy() via the
// coordinator. The scripts contain the full dynamic behavior trees.

void BattlegroundAI::ExecuteWSGStrategy(::Player* player)
{
    if (!player || !player->IsInWorld() || !player->IsAlive())
        return;

    BattlegroundCoordinator* coordinator = sBGCoordinatorMgr->GetCoordinatorForPlayer(player);
    if (coordinator)
    {
        auto* script = coordinator->GetScript();
        if (script && script->ExecuteStrategy(player))
            return;
    }

    TC_LOG_DEBUG("playerbots.bg", "[WSG/TP] {} no coordinator/script available, idle",
        player->GetName());
}

void BattlegroundAI::ExecuteABStrategy(::Player* player)
{
    if (!player || !player->IsInWorld() || !player->IsAlive())
        return;

    BattlegroundCoordinator* coordinator = sBGCoordinatorMgr->GetCoordinatorForPlayer(player);
    if (coordinator)
    {
        auto* script = coordinator->GetScript();
        if (script && script->ExecuteStrategy(player))
            return;
    }

    TC_LOG_DEBUG("playerbots.bg", "[AB/BFG] {} no coordinator/script available, idle",
        player->GetName());
}

void BattlegroundAI::ExecuteAVStrategy(::Player* player)
{
    if (!player || !player->IsInWorld() || !player->IsAlive())
        return;

    BattlegroundCoordinator* coordinator = sBGCoordinatorMgr->GetCoordinatorForPlayer(player);
    if (coordinator)
    {
        auto* script = coordinator->GetScript();
        if (script && script->ExecuteStrategy(player))
            return;
    }

    TC_LOG_DEBUG("playerbots.bg", "[AV] {} no coordinator/script available, idle",
        player->GetName());
}

void BattlegroundAI::ExecuteEOTSStrategy(::Player* player)
{
    if (!player || !player->IsInWorld() || !player->IsAlive())
        return;

    BattlegroundCoordinator* coordinator = sBGCoordinatorMgr->GetCoordinatorForPlayer(player);
    if (coordinator)
    {
        auto* script = coordinator->GetScript();
        if (script && script->ExecuteStrategy(player))
            return;
    }

    TC_LOG_DEBUG("playerbots.bg", "[EOTS] {} no coordinator/script available, idle",
        player->GetName());
}

void BattlegroundAI::ExecuteSiegeStrategy(::Player* player)
{
    if (!player || !player->IsInWorld() || !player->IsAlive())
        return;

    BattlegroundCoordinator* coordinator = sBGCoordinatorMgr->GetCoordinatorForPlayer(player);
    if (coordinator)
    {
        auto* script = coordinator->GetScript();
        if (script && script->ExecuteStrategy(player))
            return;
    }

    TC_LOG_DEBUG("playerbots.bg", "[Siege] {} no coordinator/script available, idle",
        player->GetName());
}

void BattlegroundAI::ExecuteKotmoguStrategy(::Player* player)
{
    if (!player || !player->IsInWorld() || !player->IsAlive())
        return;

    BattlegroundCoordinator* coordinator = sBGCoordinatorMgr->GetCoordinatorForPlayer(player);
    if (coordinator)
    {
        auto* script = coordinator->GetScript();
        if (script && script->ExecuteStrategy(player))
            return;
    }

    TC_LOG_DEBUG("playerbots.bg", "[TOK] {} no coordinator/script available, idle",
        player->GetName());
}

void BattlegroundAI::ExecuteSilvershardStrategy(::Player* player)
{
    if (!player || !player->IsInWorld() || !player->IsAlive())
        return;

    BattlegroundCoordinator* coordinator = sBGCoordinatorMgr->GetCoordinatorForPlayer(player);
    if (coordinator)
    {
        auto* script = coordinator->GetScript();
        if (script && script->ExecuteStrategy(player))
            return;
    }

    TC_LOG_DEBUG("playerbots.bg", "[SSM] {} no coordinator/script available, idle",
        player->GetName());
}

void BattlegroundAI::ExecuteDeepwindStrategy(::Player* player)
{
    if (!player || !player->IsInWorld() || !player->IsAlive())
        return;

    BattlegroundCoordinator* coordinator = sBGCoordinatorMgr->GetCoordinatorForPlayer(player);
    if (coordinator)
    {
        auto* script = coordinator->GetScript();
        if (script && script->ExecuteStrategy(player))
            return;
    }

    TC_LOG_DEBUG("playerbots.bg", "[DWG] {} no coordinator/script available, idle",
        player->GetName());
}

void BattlegroundAI::ExecuteSeethingShoreStrategy(::Player* player)
{
    if (!player || !player->IsInWorld() || !player->IsAlive())
        return;

    BattlegroundCoordinator* coordinator = sBGCoordinatorMgr->GetCoordinatorForPlayer(player);
    if (coordinator)
    {
        auto* script = coordinator->GetScript();
        if (script && script->ExecuteStrategy(player))
            return;
    }

    TC_LOG_DEBUG("playerbots.bg", "[SS] {} no coordinator/script available, idle",
        player->GetName());
}

void BattlegroundAI::ExecuteAshranStrategy(::Player* player)
{
    if (!player || !player->IsInWorld() || !player->IsAlive())
        return;

    BattlegroundCoordinator* coordinator = sBGCoordinatorMgr->GetCoordinatorForPlayer(player);
    if (coordinator)
    {
        auto* script = coordinator->GetScript();
        if (script && script->ExecuteStrategy(player))
            return;
    }

    TC_LOG_DEBUG("playerbots.bg", "[Ashran] {} no coordinator/script available, idle",
        player->GetName());
}

// ============================================================================
// PROFILES
// ============================================================================

void BattlegroundAI::SetStrategyProfile(uint32 playerGuid, BGStrategyProfile const& profile)
{
    ::std::lock_guard lock(_mutex);
    _playerProfiles[playerGuid] = profile;
}

BGStrategyProfile BattlegroundAI::GetStrategyProfile(uint32 playerGuid) const
{
    ::std::lock_guard lock(_mutex);

    if (_playerProfiles.count(playerGuid))
        return _playerProfiles.at(playerGuid);

    return BGStrategyProfile(); // Default
}

// ============================================================================
// METRICS
// ============================================================================

BGMetrics const& BattlegroundAI::GetPlayerMetrics(uint32 playerGuid) const
{
    ::std::lock_guard lock(_mutex);

    if (!_playerMetrics.count(playerGuid))
    {
        static BGMetrics emptyMetrics;
        return emptyMetrics;
    }

    return _playerMetrics.at(playerGuid);
}

BGMetrics const& BattlegroundAI::GetGlobalMetrics() const
{
    return _globalMetrics;
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

BGType BattlegroundAI::GetBattlegroundType(::Player* player) const
{
    if (!player)
        return BGType::WARSONG_GULCH;

    ::Battleground* bg = GetPlayerBattleground(player);
    if (!bg)
        return BGType::WARSONG_GULCH;

    uint32 mapId = bg->GetMapId();

    switch (mapId)
    {
        case 489:   return BGType::WARSONG_GULCH;
        case 529:   return BGType::ARATHI_BASIN;
        case 30:    return BGType::ALTERAC_VALLEY;
        case 566:   return BGType::EYE_OF_THE_STORM;
        case 607:   return BGType::STRAND_OF_THE_ANCIENTS;
        case 628:   return BGType::ISLE_OF_CONQUEST;
        case 726:   return BGType::TWIN_PEAKS;
        case 761:   return BGType::BATTLE_FOR_GILNEAS;
        case 727:   return BGType::SILVERSHARD_MINES;
        case 998:   return BGType::TEMPLE_OF_KOTMOGU;
        case 1105:  return BGType::DEEPWIND_GORGE;
        case 1803:  return BGType::SEETHING_SHORE;
        case 1191:  return BGType::ASHRAN;
        default:
            TC_LOG_WARN("playerbots.bg",
                "BattlegroundAI: Unknown BG map {} for bot {} - defaulting to WSG strategy",
                mapId, player->GetName());
            return BGType::WARSONG_GULCH;
    }
}

::Battleground* BattlegroundAI::GetPlayerBattleground(::Player* player) const
{
    if (!player)
        return nullptr;

    return player->GetBattleground();
}

} // namespace Playerbot
