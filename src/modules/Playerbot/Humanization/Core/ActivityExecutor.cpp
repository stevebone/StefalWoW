/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ActivityExecutor.h"
#include "Player.h"
#include "WorldSession.h"
#include "Log.h"
#include "Map.h"
#include "RestMgr.h"
#include "Creature.h"
#include "GameTime.h"
#include "ObjectAccessor.h"
#include "AI/BotAI.h"
#include "Core/Managers/IGameSystemsManager.h"
#include "Session/BotSessionManager.h"
#include "Professions/GatheringManager.h"
#include "Economy/AuctionManager.h"
#include "Professions/ProfessionManager.h"
#include "Social/TradeManager.h"
#include "Spatial/DoubleBufferedSpatialGrid.h"
#include "Spatial/SpatialGridQueryHelpers.h"
#include "Spatial/SpatialGridManager.h"

namespace Playerbot
{
namespace Humanization
{

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

ActivityExecutor::ActivityExecutor(Player* bot)
    : _bot(bot)
{
}

ActivityExecutor::~ActivityExecutor()
{
    Shutdown();
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void ActivityExecutor::Initialize()
{
    if (_initialized)
        return;

    if (!_bot || !_bot->IsInWorld())
    {
        TC_LOG_WARN("module.playerbot.humanization",
            "ActivityExecutor::Initialize - Bot not ready");
        return;
    }

    _initialized = true;
    _currentActivity = ActivityType::NONE;

    TC_LOG_DEBUG("module.playerbot.humanization",
        "ActivityExecutor::Initialize - Initialized for bot {}",
        _bot->GetName());
}

void ActivityExecutor::Shutdown()
{
    if (!_initialized)
        return;

    StopAllActivities();
    _initialized = false;
}

// ============================================================================
// ACTIVITY EXECUTION
// ============================================================================

ActivityExecutionResult ActivityExecutor::StartActivity(ActivityExecutionContext const& context)
{
    if (!_initialized)
        return ActivityExecutionResult::FAILED_NO_MANAGER;

    if (!_bot || !_bot->IsInWorld())
        return ActivityExecutionResult::FAILED_PRECONDITION;

    // Check if already executing this activity
    if (_currentActivity == context.activity && context.activity != ActivityType::NONE)
        return ActivityExecutionResult::FAILED_ALREADY_ACTIVE;

    // Check cooldown
    if (IsOnCooldown(context.activity))
        return ActivityExecutionResult::FAILED_COOLDOWN;

    // Stop any current activity first
    if (_currentActivity != ActivityType::NONE)
    {
        StopActivity(_currentActivity);
    }

    ActivityExecutionResult result = ActivityExecutionResult::NOT_IMPLEMENTED;

    switch (context.activity)
    {
        // ====================================================================
        // GATHERING ACTIVITIES
        // ====================================================================
        case ActivityType::MINING:
            result = StartMining();
            break;

        case ActivityType::HERBALISM:
            result = StartHerbalism();
            break;

        case ActivityType::SKINNING:
            result = StartSkinning();
            break;

        case ActivityType::FISHING:
            result = StartFishing();
            break;

        // ====================================================================
        // CITY LIFE ACTIVITIES
        // ====================================================================
        case ActivityType::AUCTION_BROWSING:
            result = StartAuctionBrowsing();
            break;

        case ActivityType::AUCTION_POSTING:
            result = StartAuctionPosting();
            break;

        case ActivityType::BANK_VISIT:
            result = StartBankVisit();
            break;

        case ActivityType::VENDOR_VISIT:
            result = StartVendorVisit();
            break;

        case ActivityType::TRAINER_VISIT:
            result = StartTrainerVisit();
            break;

        case ActivityType::INN_REST:
            result = StartInnRest();
            break;

        case ActivityType::MAILBOX_CHECK:
            result = StartMailboxCheck();
            break;

        // ====================================================================
        // CRAFTING ACTIVITIES
        // ====================================================================
        case ActivityType::CRAFTING_SESSION:
            result = StartCrafting();
            break;

        case ActivityType::DISENCHANTING:
            result = StartDisenchanting();
            break;

        // ====================================================================
        // MAINTENANCE ACTIVITIES
        // ====================================================================
        case ActivityType::REPAIRING:
            result = StartRepairing();
            break;

        case ActivityType::VENDORING:
            result = StartVendoring();
            break;

        // ====================================================================
        // IDLE ACTIVITIES
        // ====================================================================
        case ActivityType::STANDING_IDLE:
        case ActivityType::SITTING_IDLE:
        case ActivityType::EMOTING:
        case ActivityType::CITY_WANDERING:
            result = StartIdleBehavior();
            break;

        // ====================================================================
        // AFK ACTIVITIES (just track, no real behavior needed)
        // ====================================================================
        case ActivityType::AFK_SHORT:
        case ActivityType::AFK_MEDIUM:
        case ActivityType::AFK_LONG:
        case ActivityType::AFK_BIO_BREAK:
            _currentActivity = context.activity;
            result = ActivityExecutionResult::SUCCESS;
            break;

        // ====================================================================
        // NOT YET IMPLEMENTED
        // ====================================================================
        default:
            TC_LOG_DEBUG("module.playerbot.humanization",
                "ActivityExecutor::StartActivity - Activity {} not implemented for bot {}",
                GetActivityName(context.activity),
                _bot->GetName());
            result = ActivityExecutionResult::NOT_IMPLEMENTED;
            break;
    }

    if (result == ActivityExecutionResult::SUCCESS)
    {
        _currentActivity = context.activity;
        TC_LOG_DEBUG("module.playerbot.humanization",
            "ActivityExecutor::StartActivity - Bot {} started activity: {}",
            _bot->GetName(),
            GetActivityName(context.activity));
    }

    return result;
}

void ActivityExecutor::StopActivity(ActivityType activity)
{
    if (!_initialized)
        return;

    if (activity == ActivityType::NONE)
    {
        StopAllActivities();
        return;
    }

    ActivityCategory category = GetActivityCategory(activity);

    switch (category)
    {
        case ActivityCategory::GATHERING:
            StopGatheringActivities();
            break;

        case ActivityCategory::CITY_LIFE:
            StopCityLifeActivities();
            break;

        case ActivityCategory::CRAFTING:
            StopCraftingActivities();
            break;

        default:
            // Other categories don't need explicit stop
            break;
    }

    if (_currentActivity == activity)
    {
        _currentActivity = ActivityType::NONE;
    }

    TC_LOG_DEBUG("module.playerbot.humanization",
        "ActivityExecutor::StopActivity - Bot {} stopped activity: {}",
        _bot ? _bot->GetName() : "unknown",
        GetActivityName(activity));
}

bool ActivityExecutor::CanExecuteActivity(ActivityType activity) const
{
    if (!_initialized || !_bot || !_bot->IsInWorld())
        return false;

    // Check cooldown
    if (IsOnCooldown(activity))
        return false;

    // Check location requirements
    if (!IsAtRequiredLocation(activity))
        return false;

    // Check skill requirements
    if (!HasRequiredSkill(activity))
        return false;

    return true;
}

// ============================================================================
// PRECONDITION CHECKS
// ============================================================================

bool ActivityExecutor::IsAtRequiredLocation(ActivityType activity) const
{
    if (!_bot || !_bot->IsInWorld())
        return false;

    ActivityCategory category = GetActivityCategory(activity);

    switch (category)
    {
        case ActivityCategory::CITY_LIFE:
            // Most city activities require being in a city or near specific NPCs
            if (activity == ActivityType::AUCTION_BROWSING || activity == ActivityType::AUCTION_POSTING)
                return IsNearNPCType(UNIT_NPC_FLAG_AUCTIONEER, 50.0f);
            if (activity == ActivityType::BANK_VISIT)
                return IsNearNPCType(UNIT_NPC_FLAG_BANKER, 50.0f);
            if (activity == ActivityType::VENDOR_VISIT)
                return IsNearNPCType(UNIT_NPC_FLAG_VENDOR, 50.0f);
            if (activity == ActivityType::TRAINER_VISIT)
                return IsNearNPCType(UNIT_NPC_FLAG_TRAINER, 50.0f);
            if (activity == ActivityType::INN_REST)
                return _bot->GetRestMgr().HasRestFlag(REST_FLAG_IN_TAVERN);
            if (activity == ActivityType::MAILBOX_CHECK)
                return true; // Mailbox proximity check deferred — cities always have mailboxes
            return IsInCity();

        case ActivityCategory::GATHERING:
            // Gathering can happen anywhere with nodes
            return true;

        case ActivityCategory::CRAFTING:
            // Crafting typically anywhere (some require specific locations)
            return true;

        default:
            return true;
    }
}

bool ActivityExecutor::HasRequiredSkill(ActivityType activity) const
{
    if (!_bot)
        return false;

    switch (activity)
    {
        case ActivityType::MINING:
            return _bot->HasSkill(SKILL_MINING);

        case ActivityType::HERBALISM:
            return _bot->HasSkill(SKILL_HERBALISM);

        case ActivityType::SKINNING:
            return _bot->HasSkill(SKILL_SKINNING);

        case ActivityType::FISHING:
            return _bot->HasSkill(SKILL_FISHING);

        case ActivityType::CRAFTING_SESSION:
            // Any crafting profession works
            return _bot->HasSkill(SKILL_BLACKSMITHING) ||
                   _bot->HasSkill(SKILL_LEATHERWORKING) ||
                   _bot->HasSkill(SKILL_TAILORING) ||
                   _bot->HasSkill(SKILL_ENGINEERING) ||
                   _bot->HasSkill(SKILL_ENCHANTING) ||
                   _bot->HasSkill(SKILL_JEWELCRAFTING) ||
                   _bot->HasSkill(SKILL_INSCRIPTION) ||
                   _bot->HasSkill(SKILL_ALCHEMY) ||
                   _bot->HasSkill(SKILL_COOKING);

        case ActivityType::DISENCHANTING:
            return _bot->HasSkill(SKILL_ENCHANTING);

        default:
            return true;
    }
}

bool ActivityExecutor::IsOnCooldown(ActivityType activity) const
{
    auto it = _cooldowns.find(static_cast<uint8>(activity));
    if (it == _cooldowns.end())
        return false;

    return GameTime::GetGameTimeMS() < it->second;
}

// ============================================================================
// GATHERING ACTIVITY HANDLERS
// ============================================================================

ActivityExecutionResult ActivityExecutor::StartMining()
{
    IGameSystemsManager* gameSystems = GetGameSystems();
    if (!gameSystems)
        return ActivityExecutionResult::FAILED_NO_MANAGER;

    GatheringManager* gatheringMgr = gameSystems->GetGatheringManager();
    if (!gatheringMgr)
        return ActivityExecutionResult::FAILED_NO_MANAGER;

    if (!HasRequiredSkill(ActivityType::MINING))
        return ActivityExecutionResult::FAILED_PRECONDITION;

    // Enable mining in gathering manager
    gatheringMgr->SetGatheringEnabled(true);
    gatheringMgr->SetProfessionEnabled(GatheringNodeType::MINING_VEIN, true);

    TC_LOG_DEBUG("module.playerbot.humanization",
        "ActivityExecutor::StartMining - Bot {} mining enabled",
        _bot->GetName());

    return ActivityExecutionResult::SUCCESS;
}

ActivityExecutionResult ActivityExecutor::StartHerbalism()
{
    IGameSystemsManager* gameSystems = GetGameSystems();
    if (!gameSystems)
        return ActivityExecutionResult::FAILED_NO_MANAGER;

    GatheringManager* gatheringMgr = gameSystems->GetGatheringManager();
    if (!gatheringMgr)
        return ActivityExecutionResult::FAILED_NO_MANAGER;

    if (!HasRequiredSkill(ActivityType::HERBALISM))
        return ActivityExecutionResult::FAILED_PRECONDITION;

    // Enable herbalism in gathering manager
    gatheringMgr->SetGatheringEnabled(true);
    gatheringMgr->SetProfessionEnabled(GatheringNodeType::HERB_NODE, true);

    TC_LOG_DEBUG("module.playerbot.humanization",
        "ActivityExecutor::StartHerbalism - Bot {} herbalism enabled",
        _bot->GetName());

    return ActivityExecutionResult::SUCCESS;
}

ActivityExecutionResult ActivityExecutor::StartSkinning()
{
    IGameSystemsManager* gameSystems = GetGameSystems();
    if (!gameSystems)
        return ActivityExecutionResult::FAILED_NO_MANAGER;

    GatheringManager* gatheringMgr = gameSystems->GetGatheringManager();
    if (!gatheringMgr)
        return ActivityExecutionResult::FAILED_NO_MANAGER;

    if (!HasRequiredSkill(ActivityType::SKINNING))
        return ActivityExecutionResult::FAILED_PRECONDITION;

    // Enable skinning in gathering manager
    gatheringMgr->SetGatheringEnabled(true);
    gatheringMgr->SetProfessionEnabled(GatheringNodeType::CREATURE_CORPSE, true);

    TC_LOG_DEBUG("module.playerbot.humanization",
        "ActivityExecutor::StartSkinning - Bot {} skinning enabled",
        _bot->GetName());

    return ActivityExecutionResult::SUCCESS;
}

ActivityExecutionResult ActivityExecutor::StartFishing()
{
    IGameSystemsManager* gameSystems = GetGameSystems();
    if (!gameSystems)
        return ActivityExecutionResult::FAILED_NO_MANAGER;

    GatheringManager* gatheringMgr = gameSystems->GetGatheringManager();
    if (!gatheringMgr)
        return ActivityExecutionResult::FAILED_NO_MANAGER;

    if (!HasRequiredSkill(ActivityType::FISHING))
        return ActivityExecutionResult::FAILED_PRECONDITION;

    // Enable fishing in gathering manager
    gatheringMgr->SetGatheringEnabled(true);
    gatheringMgr->SetProfessionEnabled(GatheringNodeType::FISHING_POOL, true);

    TC_LOG_DEBUG("module.playerbot.humanization",
        "ActivityExecutor::StartFishing - Bot {} fishing enabled",
        _bot->GetName());

    return ActivityExecutionResult::SUCCESS;
}

// ============================================================================
// CITY LIFE ACTIVITY HANDLERS
// ============================================================================

ActivityExecutionResult ActivityExecutor::StartAuctionBrowsing()
{
    IGameSystemsManager* gameSystems = GetGameSystems();
    if (!gameSystems)
        return ActivityExecutionResult::FAILED_NO_MANAGER;

    AuctionManager* auctionMgr = gameSystems->GetAuctionManager();
    if (!auctionMgr)
        return ActivityExecutionResult::FAILED_NO_MANAGER;

    // Check if near auctioneer
    if (!IsNearNPCType(UNIT_NPC_FLAG_AUCTIONEER, 50.0f))
        return ActivityExecutionResult::FAILED_PRECONDITION;

    // AuctionManager handles browsing automatically when enabled and near auctioneer
    // The manager's Update() will scan for deals

    TC_LOG_DEBUG("module.playerbot.humanization",
        "ActivityExecutor::StartAuctionBrowsing - Bot {} browsing auction house",
        _bot->GetName());

    return ActivityExecutionResult::SUCCESS;
}

ActivityExecutionResult ActivityExecutor::StartAuctionPosting()
{
    IGameSystemsManager* gameSystems = GetGameSystems();
    if (!gameSystems)
        return ActivityExecutionResult::FAILED_NO_MANAGER;

    AuctionManager* auctionMgr = gameSystems->GetAuctionManager();
    if (!auctionMgr)
        return ActivityExecutionResult::FAILED_NO_MANAGER;

    // Check if near auctioneer
    if (!IsNearNPCType(UNIT_NPC_FLAG_AUCTIONEER, 50.0f))
        return ActivityExecutionResult::FAILED_PRECONDITION;

    // AuctionManager handles posting automatically when enabled and near auctioneer

    TC_LOG_DEBUG("module.playerbot.humanization",
        "ActivityExecutor::StartAuctionPosting - Bot {} posting to auction house",
        _bot->GetName());

    return ActivityExecutionResult::SUCCESS;
}

ActivityExecutionResult ActivityExecutor::StartBankVisit()
{
    // Check if near banker
    if (!IsNearNPCType(UNIT_NPC_FLAG_BANKER, 50.0f))
        return ActivityExecutionResult::FAILED_PRECONDITION;

    // Banking operations are handled by BankingManager (standalone)
    // This activity just tracks that the bot is visiting the bank

    TC_LOG_DEBUG("module.playerbot.humanization",
        "ActivityExecutor::StartBankVisit - Bot {} visiting bank",
        _bot->GetName());

    return ActivityExecutionResult::SUCCESS;
}

ActivityExecutionResult ActivityExecutor::StartVendorVisit()
{
    // Check if near vendor
    if (!IsNearNPCType(UNIT_NPC_FLAG_VENDOR, 50.0f))
        return ActivityExecutionResult::FAILED_PRECONDITION;

    // Vendor interactions are handled by other systems

    TC_LOG_DEBUG("module.playerbot.humanization",
        "ActivityExecutor::StartVendorVisit - Bot {} visiting vendor",
        _bot->GetName());

    return ActivityExecutionResult::SUCCESS;
}

ActivityExecutionResult ActivityExecutor::StartTrainerVisit()
{
    // Check if near trainer
    if (!IsNearNPCType(UNIT_NPC_FLAG_TRAINER, 50.0f))
        return ActivityExecutionResult::FAILED_PRECONDITION;

    // Trainer interactions are handled by other systems

    TC_LOG_DEBUG("module.playerbot.humanization",
        "ActivityExecutor::StartTrainerVisit - Bot {} visiting trainer",
        _bot->GetName());

    return ActivityExecutionResult::SUCCESS;
}

ActivityExecutionResult ActivityExecutor::StartInnRest()
{
    // Check if at inn
    if (!_bot->GetRestMgr().HasRestFlag(REST_FLAG_IN_TAVERN))
        return ActivityExecutionResult::FAILED_PRECONDITION;

    // Bot is already resting, just tracking the activity

    TC_LOG_DEBUG("module.playerbot.humanization",
        "ActivityExecutor::StartInnRest - Bot {} resting at inn",
        _bot->GetName());

    return ActivityExecutionResult::SUCCESS;
}

ActivityExecutionResult ActivityExecutor::StartMailboxCheck()
{
    // Mailbox check is currently cosmetic — bot pauses near mailbox for immersion.
    // Full implementation would query Mail table and process mail items.

    TC_LOG_DEBUG("module.playerbot.humanization",
        "ActivityExecutor::StartMailboxCheck - Bot {} checking mailbox",
        _bot->GetName());

    return ActivityExecutionResult::SUCCESS;
}

// ============================================================================
// CRAFTING ACTIVITY HANDLERS
// ============================================================================

ActivityExecutionResult ActivityExecutor::StartCrafting()
{
    IGameSystemsManager* gameSystems = GetGameSystems();
    if (!gameSystems)
        return ActivityExecutionResult::FAILED_NO_MANAGER;

    ProfessionManager* professionMgr = gameSystems->GetProfessionManager();
    if (!professionMgr)
        return ActivityExecutionResult::FAILED_NO_MANAGER;

    if (!HasRequiredSkill(ActivityType::CRAFTING_SESSION))
        return ActivityExecutionResult::FAILED_PRECONDITION;

    // ProfessionManager handles crafting logic

    TC_LOG_DEBUG("module.playerbot.humanization",
        "ActivityExecutor::StartCrafting - Bot {} starting crafting session",
        _bot->GetName());

    return ActivityExecutionResult::SUCCESS;
}

ActivityExecutionResult ActivityExecutor::StartDisenchanting()
{
    if (!HasRequiredSkill(ActivityType::DISENCHANTING))
        return ActivityExecutionResult::FAILED_PRECONDITION;

    // Disenchanting handled by ProfessionManager

    TC_LOG_DEBUG("module.playerbot.humanization",
        "ActivityExecutor::StartDisenchanting - Bot {} disenchanting items",
        _bot->GetName());

    return ActivityExecutionResult::SUCCESS;
}

// ============================================================================
// MAINTENANCE ACTIVITY HANDLERS
// ============================================================================

ActivityExecutionResult ActivityExecutor::StartRepairing()
{
    // Check if near repair vendor
    if (!IsNearNPCType(UNIT_NPC_FLAG_REPAIR, 50.0f))
        return ActivityExecutionResult::FAILED_PRECONDITION;

    // Repair is typically handled automatically by other systems

    TC_LOG_DEBUG("module.playerbot.humanization",
        "ActivityExecutor::StartRepairing - Bot {} repairing gear",
        _bot->GetName());

    return ActivityExecutionResult::SUCCESS;
}

ActivityExecutionResult ActivityExecutor::StartVendoring()
{
    // Check if near vendor
    if (!IsNearNPCType(UNIT_NPC_FLAG_VENDOR, 50.0f))
        return ActivityExecutionResult::FAILED_PRECONDITION;

    // Vendoring is handled by EquipmentManager or similar

    TC_LOG_DEBUG("module.playerbot.humanization",
        "ActivityExecutor::StartVendoring - Bot {} selling items",
        _bot->GetName());

    return ActivityExecutionResult::SUCCESS;
}

// ============================================================================
// IDLE ACTIVITY HANDLERS
// ============================================================================

ActivityExecutionResult ActivityExecutor::StartIdleBehavior()
{
    // Idle behavior is passive - just tracking the state
    // Could add emotes, random movement, etc.

    TC_LOG_DEBUG("module.playerbot.humanization",
        "ActivityExecutor::StartIdleBehavior - Bot {} idling",
        _bot->GetName());

    return ActivityExecutionResult::SUCCESS;
}

// ============================================================================
// STOP HANDLERS
// ============================================================================

void ActivityExecutor::StopGatheringActivities()
{
    IGameSystemsManager* gameSystems = GetGameSystems();
    if (!gameSystems)
        return;

    GatheringManager* gatheringMgr = gameSystems->GetGatheringManager();
    if (gatheringMgr)
    {
        gatheringMgr->StopGathering();
        gatheringMgr->SetGatheringEnabled(false);
    }
}

void ActivityExecutor::StopCityLifeActivities()
{
    // City life activities are mostly passive observations
    // No explicit stop needed for most
}

void ActivityExecutor::StopCraftingActivities()
{
    // Crafting stops naturally when complete
    // Could interrupt ongoing crafts if needed
}

void ActivityExecutor::StopAllActivities()
{
    StopGatheringActivities();
    StopCityLifeActivities();
    StopCraftingActivities();
    _currentActivity = ActivityType::NONE;
}

// ============================================================================
// HELPER METHODS
// ============================================================================

BotAI* ActivityExecutor::GetBotAI() const
{
    if (!_bot || !_bot->GetSession())
        return nullptr;

    return BotSessionManager::GetBotAI(_bot->GetSession());
}

IGameSystemsManager* ActivityExecutor::GetGameSystems() const
{
    BotAI* botAI = GetBotAI();
    return botAI ? botAI->GetGameSystems() : nullptr;
}

bool ActivityExecutor::IsNearNPCType(uint32 npcFlags, float range) const
{
    if (!_bot || !_bot->IsInWorld())
        return false;

    Map* map = _bot->GetMap();
    if (!map)
        return false;

    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return false;
    }

    // Query nearby creature GUIDs (lock-free!)
    std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        _bot->GetPosition(), range);

    // Check each creature for the NPC flags
    for (ObjectGuid guid : nearbyGuids)
    {
        // Thread-safe spatial grid validation
        auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, guid);
        if (!snapshot)
            continue;

        // Get Creature* for NPC type detection
        Creature* creature = ObjectAccessor::GetCreature(*_bot, guid);
        if (!creature || !creature->IsAlive() || creature->IsHostileTo(_bot))
            continue;

        if (creature->HasNpcFlag(NPCFlags(npcFlags)))
            return true;
    }

    return false;
}

bool ActivityExecutor::IsInCity() const
{
    if (!_bot || !_bot->IsInWorld())
        return false;

    return _bot->GetRestMgr().HasRestFlag(REST_FLAG_IN_CITY) ||
           _bot->IsInSanctuary();
}

void ActivityExecutor::SetActivityCooldown(ActivityType activity, uint32 cooldownMs)
{
    _cooldowns[static_cast<uint8>(activity)] = GameTime::GetGameTimeMS() + cooldownMs;
}

} // namespace Humanization
} // namespace Playerbot
