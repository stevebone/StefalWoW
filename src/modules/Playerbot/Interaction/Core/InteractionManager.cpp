/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "InteractionManager.h"
#include "GossipHandler.h"
#include "InteractionValidator.h"
// ============================================================================
// ARCHITECTURE NOTE: InteractionManager is a FACADE
// ============================================================================
// Individual interaction types are implemented in dedicated Manager classes:
//   - TrainerInteractionManager.cpp   (trainer interactions)
//   - VendorInteractionManager.cpp    (vendor/repair interactions)
//   - InnkeeperInteractionManager.cpp (innkeeper/binding interactions)
//   - FlightMasterManager.cpp         (flight path interactions)
//   - BankInteractionManager.cpp      (bank/guild bank interactions)
//   - MailInteractionManager.cpp      (mailbox interactions)
//
// This facade provides centralized interaction state management,
// NPC type detection and routing, gossip menu navigation, and metrics.
// ============================================================================
#include "../VendorInteractionManager.h"
#include "Player.h"
#include "Creature.h"
#include "GameObject.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "WorldSession.h"
#include "GossipDef.h"
#include "Log.h"
#include "World.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "MotionMaster.h"
#include "MovementGenerator.h"
#include "UnitDefines.h"
#include "../../Spatial/SpatialGridManager.h"  // Lock-free spatial grid for deadlock fix
#include "../../Spatial/SpatialGridQueryHelpers.h"  // Thread-safe spatial queries
#include "../../Core/PlayerBotHelpers.h"
#include "../../AI/BotAI.h"

namespace Playerbot
{
    // Static member initialization - modern singleton pattern
    ::std::unique_ptr<InteractionManager> InteractionManager::s_instance;
    ::std::once_flag InteractionManager::s_initFlag;

    InteractionManager::InteractionManager()
    {
        // Initialize configuration with default values
        m_config.interactionRange = 5.0f;
        m_config.vendorSearchRange = 100.0f;
        m_config.trainerSearchRange = 100.0f;
        m_config.repairSearchRange = 200.0f;
        m_config.interactionDelay = 500;
        m_config.gossipReadDelay = 100;
        m_config.maxInteractionTime = 30000;
        m_config.autoRepair = true;
        m_config.autoSellJunk = true;
        m_config.autoBuyReagents = true;
        m_config.autoLearnSpells = true;
        m_config.autoDiscoverFlightPaths = true;
        m_config.autoEmptyMail = true;
        m_config.repairThreshold = 30.0f;
        m_config.minFreeSlots = 5;
        m_config.reagentStockMultiple = 2.0f;
        m_config.maxConcurrentInteractions = 3;
        m_config.enableMetrics = true;
        m_config.logInteractions = false;

        m_lastCacheClean = ::std::chrono::steady_clock::now();
    }

    InteractionManager::~InteractionManager()
    {
        Shutdown();
    }

    InteractionManager* InteractionManager::Instance()
    {
        ::std::call_once(s_initFlag, []()
        {
            s_instance = ::std::make_unique<InteractionManager>();
        });
        return s_instance.get();
    }

    bool InteractionManager::Initialize()
    {
        // No lock needed - interaction state is per-bot instance data
    if (m_initialized)
            return true;

        TC_LOG_INFO("playerbot", "Initializing InteractionManager");

        InitializeHandlers();

        // Initialize metrics for each interaction type
    for (uint8 i = 0; i < static_cast<uint8>(InteractionType::MAX_INTERACTION); ++i)
        {
            m_metrics[static_cast<InteractionType>(i)] = InteractionMetrics();
        }

        m_initialized = true;
        TC_LOG_INFO("playerbot", "InteractionManager initialized successfully");
        return true;
    }

    void InteractionManager::InitializeHandlers()
    {
        m_gossipHandler = ::std::make_unique<GossipHandler>();
        m_validator = ::std::make_unique<InteractionValidator>();
        // Note: Individual interaction handlers are now implemented as separate Manager classes
        // (TrainerInteractionManager, VendorInteractionManager, etc.) and are accessed through
        // the BotAI's interaction manager lookup system rather than stored as handler members here.
    }

    void InteractionManager::Shutdown()
    {
        // No lock needed - interaction state is per-bot instance data
    if (!m_initialized)
            return;

        TC_LOG_INFO("playerbot", "Shutting down InteractionManager");

        // Cancel all active interactions
    for (auto& [guid, context] : m_activeInteractions)
        {
            context->state = InteractionState::Failed;
        }
        m_activeInteractions.clear();

        // Clear queue
    while (!m_interactionQueue.empty())
            m_interactionQueue.pop();

        CleanupHandlers();

        m_initialized = false;
        TC_LOG_INFO("playerbot", "InteractionManager shut down successfully");
    }

    void InteractionManager::CleanupHandlers()
    {
        m_gossipHandler.reset();
        m_validator.reset();
        // Individual interaction handlers are separate Manager singletons and manage their own lifecycle
    }

    void InteractionManager::Update(uint32 diff)
    {
        // No lock needed - interaction state is per-bot instance data
    if (!m_initialized)
            return;

        // Clean NPC type cache every 5 minutes
        auto now = ::std::chrono::steady_clock::now();
        if (now - m_lastCacheClean > ::std::chrono::minutes(5))
        {
            m_npcTypeCache.clear();
            m_lastCacheClean = now;
        }

        // Process queued interactions
    while (!m_interactionQueue.empty() &&
               m_activeInteractions.size() < m_config.maxConcurrentInteractions)
        {
            auto& queued = m_interactionQueue.top();

            // Check if enough time has passed since last interaction
            auto lastTime = m_lastInteractionTime.find(queued.botGuid);
            if (lastTime != m_lastInteractionTime.end())
            {
                auto timeSinceLast = now - lastTime->second;
                if (timeSinceLast < ::std::chrono::milliseconds(MIN_INTERACTION_DELAY))
                    break; // Wait more
            }

            // Start the queued interaction
    if (Player* bot = ObjectAccessor::FindPlayer(queued.botGuid))
            {
                // PHASE 5D: Thread-safe spatial grid validation
                // FindPlayer is safe (global storage), but GetWorldObject needs snapshot validation
                auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(bot, queued.targetGuid);
                WorldObject* target = nullptr;

                if (snapshot)
                {
                    // Get WorldObject* for interaction (validated via snapshot first)
                    target = ObjectAccessor::GetWorldObject(*bot, queued.targetGuid);
                }

                if (target)
                {
                    StartInteraction(bot, target, queued.type);
                }
            }

            m_interactionQueue.pop();
        }

        // Update active interactions
        ::std::vector<ObjectGuid> toRemove;
        for (auto& [guid, context] : m_activeInteractions)
        {
            if (Player* bot = ObjectAccessor::FindPlayer(guid))
            {
                if (!UpdateInteraction(bot, *context, diff))
                {
                    toRemove.push_back(guid);
                }
            }
            else
            {
                toRemove.push_back(guid);
            }
        }

        // Remove completed interactions
    for (const auto& guid : toRemove)
        {
            m_activeInteractions.erase(guid);
        }
    }

    InteractionResult InteractionManager::StartInteraction(Player* bot, WorldObject* target,
                                                          InteractionType type)
    {
        if (!bot || !target)
            return InteractionResult::InvalidTarget;

        // No lock needed - interaction state is per-bot instance data

        // Check if bot already has active interaction
    if (HasActiveInteraction(bot))
            return InteractionResult::TargetBusy;

        // Check combat state
    if (bot->IsInCombat() && type != InteractionType::SpiritHealer)
            return InteractionResult::InCombat;

        // Auto-detect type if not specified
    if (type == InteractionType::None)
        {
            if (Creature* creature = target->ToCreature())
                type = DetectNPCType(creature);
            else if (target->GetTypeId() == TYPEID_GAMEOBJECT)
            {
                GameObject* go = target->ToGameObject();
                switch (go->GetGoType())
                {
                    case GAMEOBJECT_TYPE_MAILBOX:
                        type = InteractionType::Mailbox;
                        break;
                    default:
                        type = InteractionType::Bank; // Could be bank chest
                        break;
                }
            }
        }

        if (type == InteractionType::None)
            return InteractionResult::InvalidTarget;

        // Validate interaction requirements
    if (!m_validator->CanInteract(bot, target, type))
            return InteractionResult::RequirementNotMet;
        // Check range
    if (!IsInInteractionRange(bot, target))
        {
            if (!MoveToInteractionRange(bot, target))
                return InteractionResult::TooFarAway;
        }

        // Create interaction context
        auto context = ::std::make_unique<InteractionContext>();
        context->botGuid = bot->GetGUID();
        context->targetGuid = target->GetGUID();
        context->type = type;
        context->state = InteractionState::Approaching;
        context->startTime = ::std::chrono::steady_clock::now();
        context->attemptCount = 0;
        context->maxAttempts = 3;

        // Determine if gossip is needed
    if (Creature* creature = target->ToCreature())
        {
            context->needsGossip = m_gossipHandler->NeedsGossipNavigation(creature, type);
            if (context->needsGossip)
            {
                context->gossipPath = m_gossipHandler->GetGossipPath(creature, type);
            }
        }
        // Store context
        m_activeInteractions[bot->GetGUID()] = ::std::move(context);
        // Record metrics
        ++m_totalInteractionsStarted;
        m_lastInteractionTime[bot->GetGUID()] = ::std::chrono::steady_clock::now();
        if (m_config.logInteractions)
        {
            ::std::string msg = "Starting ";
            msg += InteractionTypeToString(type);
            msg += " interaction";
            LogInteraction(bot, msg);
        }

        return InteractionResult::Pending;
    }

    void InteractionManager::CancelInteraction(Player* bot, ObjectGuid targetGuid)
    {
        if (!bot)
            return;

        // No lock needed - interaction state is per-bot instance data

        auto it = m_activeInteractions.find(bot->GetGUID());
        if (it != m_activeInteractions.end())
        {
            // Verify this is the correct interaction to cancel
    if (targetGuid.IsEmpty() || it->second->targetGuid == targetGuid)
            {
                CompleteInteraction(bot, InteractionResult::Interrupted);
                m_activeInteractions.erase(it);
            }
        }
    }

    bool InteractionManager::HasActiveInteraction(Player* bot) const
    {
        if (!bot)
            return false;

        // No lock needed - interaction state is per-bot instance data
        return m_activeInteractions.find(bot->GetGUID()) != m_activeInteractions.end();
    }

    InteractionContext* InteractionManager::GetInteractionContext(Player* bot)
    {
        if (!bot)
            return nullptr;

        // No lock needed - interaction state is per-bot instance data

        auto it = m_activeInteractions.find(bot->GetGUID());
        if (it != m_activeInteractions.end())
            return it->second.get();

        return nullptr;
    }

    InteractionType InteractionManager::DetectNPCType(Creature* target) const
    {
        if (!target)
            return InteractionType::None;

        // Check cache first
        auto cached = m_npcTypeCache.find(target->GetGUID());
        if (cached != m_npcTypeCache.end())
            return cached->second;

        InteractionType type = InteractionType::None;

        // Check NPC flags for type detection
        NPCFlags npcFlags = target->GetNpcFlags();

        // Priority order for multi-flag NPCs
    if (npcFlags & UNIT_NPC_FLAG_TRAINER)
            type = InteractionType::Trainer;
        else if (npcFlags & UNIT_NPC_FLAG_VENDOR || npcFlags & UNIT_NPC_FLAG_REPAIR)
            type = InteractionType::Vendor;
        else if (npcFlags & UNIT_NPC_FLAG_FLIGHTMASTER)
            type = InteractionType::FlightMaster;
        else if (npcFlags & UNIT_NPC_FLAG_INNKEEPER)
            type = InteractionType::Innkeeper;
        else if (npcFlags & UNIT_NPC_FLAG_BANKER)
            type = InteractionType::Bank;
        else if (npcFlags & UNIT_NPC_FLAG_AUCTIONEER)
            type = InteractionType::Auctioneer;
        else if (npcFlags & UNIT_NPC_FLAG_STABLEMASTER)
            type = InteractionType::StableMaster;
        else if (npcFlags & UNIT_NPC_FLAG_BATTLEMASTER)
            type = InteractionType::Battlemaster;
        else if (npcFlags & UNIT_NPC_FLAG_SPIRIT_HEALER || npcFlags & UNIT_NPC_FLAG_AREA_SPIRIT_HEALER)
            type = InteractionType::SpiritHealer;
        else if (npcFlags & UNIT_NPC_FLAG_QUESTGIVER)
            type = InteractionType::QuestGiver;
        else if (npcFlags & UNIT_NPC_FLAG_TRANSMOGRIFIER)
            type = InteractionType::Transmogrifier;
        else if (npcFlags & UNIT_NPC_FLAG_VAULTKEEPER)
            type = InteractionType::VoidStorage;

        // Cache the result
        m_npcTypeCache[target->GetGUID()] = type;

        return type;
    }

    Creature* InteractionManager::FindNearestNPC(Player* bot, NPCType type, float maxRange) const
    {
        if (!bot)
            return nullptr;

        Creature* nearest = nullptr;
        float minDist = maxRange;

        ::std::list<Creature*> creatures;
        Trinity::AllCreaturesOfEntryInRange checker(bot, 0, maxRange);
        Trinity::CreatureListSearcher<Trinity::AllCreaturesOfEntryInRange> searcher(bot, creatures, checker);
        // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = bot->GetMap();
    if (!map)
        return nullptr;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return nullptr;
    }

    // Query nearby GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        bot->GetPosition(), maxRange);

    // Process results (replace old loop)
    for (ObjectGuid guid : nearbyGuids)
    {
        // PHASE 5D: Thread-safe spatial grid validation
        auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(bot, guid);
        Creature* entity = nullptr;

        if (snapshot)
        {
            // Get Creature* for NPC type detection (validated via snapshot first)
            entity = ObjectAccessor::GetCreature(*bot, guid);
        }

        if (!entity)
            continue;
        // Original filtering logic goes here
    }
    // End of spatial grid fix
    for (Creature* creature : creatures)
        {
            // Convert InteractionType to NPCType for comparison
            InteractionType interactionType = DetectNPCType(creature);
            NPCType npcType = NPCType::GENERAL;

            // Map InteractionType to NPCType
    switch (interactionType)
            {
                case InteractionType::Vendor:
                    npcType = NPCType::VENDOR;
                    break;
                case InteractionType::Trainer:
                    npcType = NPCType::TRAINER;
                    break;
                case InteractionType::Innkeeper:
                    npcType = NPCType::INNKEEPER;
                    break;
                case InteractionType::FlightMaster:
                    npcType = NPCType::FLIGHT_MASTER;
                    break;
                case InteractionType::Bank:
                    npcType = NPCType::BANKER;
                    break;
                case InteractionType::Auctioneer:
                    npcType = NPCType::AUCTIONEER;
                    break;
                case InteractionType::Mailbox:
                    npcType = NPCType::MAILBOX;
                    break;
                case InteractionType::StableMaster:
                    npcType = NPCType::STABLE_MASTER;
                    break;
                case InteractionType::Battlemaster:
                    npcType = NPCType::BATTLEMASTER;
                    break;
                case InteractionType::Transmogrifier:
                    npcType = NPCType::TRANSMOGRIFIER;
                    break;
                case InteractionType::QuestGiver:
                    npcType = NPCType::QUEST_GIVER;
                    break;
                default:
                    npcType = NPCType::GENERAL;
                    break;
            }

            if (npcType == type)
            {
                float dist = bot->GetDistance(creature);
                if (dist < minDist)
                {
                    minDist = dist;
                    nearest = creature;
                }
            }
        }

        return nearest;
    }

    InteractionResult InteractionManager::BuyItem(Player* bot, Creature* vendor, uint32 itemId, uint32 count)
    {
        if (!bot || !vendor || !itemId || !count)
            return InteractionResult::InvalidTarget;

        // Start interaction if not already active
    if (!HasActiveInteraction(bot))
        {
            InteractionResult startResult = StartInteraction(bot, vendor, InteractionType::Vendor);
            if (startResult != InteractionResult::Pending)
                return startResult;
        }

        // Let vendor handler process the purchase
        // Delegate to VendorInteractionManager singleton for actual implementation
        return InteractionResult::NotAvailable;
    }

    InteractionResult InteractionManager::SellJunk(Player* bot, Creature* vendor)
    {
        if (!bot || !vendor)
            return InteractionResult::InvalidTarget;

        if (!HasActiveInteraction(bot))
        {
            InteractionResult startResult = StartInteraction(bot, vendor, InteractionType::Vendor);
            if (startResult != InteractionResult::Pending)
                return startResult;
        }

        // Delegate to VendorInteractionManager singleton for actual implementation
        return InteractionResult::NotAvailable;
    }

    InteractionResult InteractionManager::RepairAll(Player* bot, Creature* vendor)
    {
        if (!bot || !vendor)
            return InteractionResult::InvalidTarget;

        if (!(vendor->GetNpcFlags() & UNIT_NPC_FLAG_REPAIR))
            return InteractionResult::InvalidTarget;

        if (!HasActiveInteraction(bot))
        {
            InteractionResult startResult = StartInteraction(bot, vendor, InteractionType::Vendor);
            if (startResult != InteractionResult::Pending)
                return startResult;
        }

        // Delegate to VendorInteractionManager singleton for actual implementation
        return InteractionResult::NotAvailable;
    }

    InteractionResult InteractionManager::LearnOptimalSpells(Player* bot, Creature* trainer)
    {
        if (!bot || !trainer)
            return InteractionResult::InvalidTarget;

        if (!trainer->IsTrainer())
            return InteractionResult::InvalidTarget;
        if (!HasActiveInteraction(bot))
        {
            InteractionResult startResult = StartInteraction(bot, trainer, InteractionType::Trainer);
            if (startResult != InteractionResult::Pending)
                return startResult;
        }

        // Delegate to TrainerInteractionManager singleton for actual implementation
        return InteractionResult::NotAvailable;
    }

    InteractionResult InteractionManager::UseFlight(Player* bot, Creature* flightMaster, uint32 /*destinationNode*/)
    {
        if (!bot || !flightMaster)
            return InteractionResult::InvalidTarget;

        if (!(flightMaster->GetNpcFlags() & UNIT_NPC_FLAG_FLIGHTMASTER))
            return InteractionResult::InvalidTarget;

        if (!HasActiveInteraction(bot))
        {
            InteractionResult startResult = StartInteraction(bot, flightMaster, InteractionType::FlightMaster);
            if (startResult != InteractionResult::Pending)
                return startResult;
        }

        // Delegate to FlightMasterManager singleton for actual implementation
        return InteractionResult::NotAvailable;
    }

    InteractionResult InteractionManager::BindHearthstone(Player* bot, Creature* innkeeper)
    {
        if (!bot || !innkeeper)
            return InteractionResult::InvalidTarget;

        if (!(innkeeper->GetNpcFlags() & UNIT_NPC_FLAG_INNKEEPER))
            return InteractionResult::InvalidTarget;

        if (!HasActiveInteraction(bot))
        {
            InteractionResult startResult = StartInteraction(bot, innkeeper, InteractionType::Innkeeper);
            if (startResult != InteractionResult::Pending)
                return startResult;
        }

        // Delegate to InnkeeperInteractionManager singleton for actual implementation
        return InteractionResult::NotAvailable;
    }

    InteractionResult InteractionManager::AccessBank(Player* bot, WorldObject* banker)
    {
        if (!bot || !banker)
            return InteractionResult::InvalidTarget;
        bool isValidBanker = false;
        if (Creature* creature = banker->ToCreature())
            isValidBanker = (creature->GetNpcFlags() & UNIT_NPC_FLAG_BANKER) != 0;
        else if (GameObject* go = banker->ToGameObject())
            isValidBanker = go->GetGoType() == GAMEOBJECT_TYPE_CHEST; // Bank chest
    if (!isValidBanker)
            return InteractionResult::InvalidTarget;

        if (!HasActiveInteraction(bot))
        {
            InteractionResult startResult = StartInteraction(bot, banker, InteractionType::Bank);
            if (startResult != InteractionResult::Pending)
                return startResult;
        }

        // Delegate to BankInteractionManager singleton for actual implementation
        return InteractionResult::NotAvailable;
    }

    InteractionResult InteractionManager::CheckMail(Player* bot, GameObject* mailbox, bool /*takeAll*/)
    {
        if (!bot || !mailbox)
            return InteractionResult::InvalidTarget;

        if (mailbox->GetGoType() != GAMEOBJECT_TYPE_MAILBOX)
            return InteractionResult::InvalidTarget;

        if (!HasActiveInteraction(bot))
        {
            InteractionResult startResult = StartInteraction(bot, mailbox, InteractionType::Mailbox);
            if (startResult != InteractionResult::Pending)
                return startResult;
        }

        // Delegate to MailInteractionManager singleton for actual implementation
        return InteractionResult::NotAvailable;
    }

    void InteractionManager::HandleGossipMenu(Player* bot, uint32 menuId, WorldObject* target)
    {
        if (!bot || !target)
            return;

        auto context = GetInteractionContext(bot);
        if (!context)
            return;

        context->gossipMenuId = menuId;
        context->state = InteractionState::ProcessingMenu;

        // Let gossip handler process the menu
        m_gossipHandler->ProcessGossipMenu(bot, menuId, target, context->type);
    }

    void InteractionManager::SelectGossipOption(Player* bot, uint32 /*optionIndex*/, WorldObject* target)
    {
        if (!bot || !target)
            return;

        auto context = GetInteractionContext(bot);
        if (!context)
            return;

        // For bots, select gossip option directly
    if (Creature* creature = target->ToCreature())
        {
            bot->PlayerTalkClass->SendCloseGossip();
            // OnGossipSelect doesn't exist in TrinityCore 12.0 - use gossip handler instead
            // The gossip selection is handled through the gossip handler system
        }
    }

    InteractionMetrics InteractionManager::GetMetrics(InteractionType type) const
    {
        // No lock needed - interaction state is per-bot instance data
    if (type == InteractionType::None)
        {
            // Return combined metrics
            InteractionMetrics combined;
            for (const auto& [t, metrics] : m_metrics)
            {
                combined.totalAttempts += metrics.totalAttempts;
                combined.successCount += metrics.successCount;
                combined.failureCount += metrics.failureCount;
                combined.timeoutCount += metrics.timeoutCount;
                combined.totalDuration += metrics.totalDuration;
            }
            if (combined.totalAttempts > 0)
            {
                combined.avgDuration = combined.totalDuration / combined.totalAttempts;
                combined.successRate = static_cast<float>(combined.successCount) / combined.totalAttempts * 100.0f;
            }
            return combined;
        }

        auto it = m_metrics.find(type);
        if (it != m_metrics.end())
            return it->second;

        return InteractionMetrics();
    }

    void InteractionManager::ResetMetrics()
    {
        // No lock needed - interaction state is per-bot instance data
        m_metrics.clear();
        m_totalInteractionsStarted = 0;
        m_totalInteractionsCompleted = 0;
        m_totalInteractionsFailed = 0;
    }

    bool InteractionManager::ProcessInteractionState(Player* bot, uint32 diff)
    {
        if (!bot)
            return false;

        auto context = GetInteractionContext(bot);
        if (!context)
            return false;

        return UpdateInteraction(bot, *context, diff);
    }

    void InteractionManager::TransitionState(Player* bot, InteractionState newState)
    {
        if (!bot)
            return;

        auto context = GetInteractionContext(bot);
        if (!context)
            return;

        if (m_config.logInteractions)
        {
            ::std::string msg = "State transition: ";
            msg += ::std::to_string(static_cast<int>(context->state));
            msg += " -> ";
            msg += ::std::to_string(static_cast<int>(newState));
            LogInteraction(bot, msg);
        }

        context->state = newState;
    }

    bool InteractionManager::IsInInteractionRange(Player* bot, WorldObject* target) const
    {
        if (!bot || !target)
            return false;

        float distance = bot->GetDistance(target);
        return distance <= m_config.interactionRange;
    }

    bool InteractionManager::MoveToInteractionRange(Player* bot, WorldObject* target)
    {
        if (!bot || !target)
            return false;

        if (IsInInteractionRange(bot, target))
            return true;

        // Stop current movement
        bot->StopMoving();

        // Move to target - use GetAbsoluteAngle instead of GetAngle
        float angle = bot->GetAbsoluteAngle(target);
        float destX = target->GetPositionX() - (m_config.interactionRange - 0.5f) * ::std::cos(angle);
        float destY = target->GetPositionY() - (m_config.interactionRange - 0.5f) * ::std::sin(angle);
        float destZ = target->GetPositionZ();

        // Use validated pathfinding for bot interaction positioning
        Position dest(destX, destY, destZ, 0.0f);
        if (BotAI* ai = GetBotAI(bot))
        {
            if (!ai->MoveTo(dest, true))
            {
                // Fallback to legacy if validation fails
                bot->GetMotionMaster()->MovePoint(0, destX, destY, destZ);
            }
        }
        else
        {
            // Non-bot player - use standard movement
            bot->GetMotionMaster()->MovePoint(0, destX, destY, destZ);
        }

        return true;
    }

    void InteractionManager::HandleInteractionError(Player* bot, InteractionResult error)
    {
        if (!bot)
            return;

        auto context = GetInteractionContext(bot);
        if (!context)
            return;

        if (m_config.logInteractions)
        {
            LogInteraction(bot, "Error: " + ::std::string(InteractionResultToString(error)));
        }

        // Increment attempt count
        ++context->attemptCount;

        // Check if we should retry
    if (context->attemptCount < context->maxAttempts)
        {
            if (AttemptRecovery(bot))
                return;
        }

        // Failed too many times
        CompleteInteraction(bot, error);
    }

    bool InteractionManager::AttemptRecovery(Player* bot)
    {
        if (!bot)
            return false;

        auto context = GetInteractionContext(bot);
        if (!context)
            return false;

        // Reset state machine
        context->state = InteractionState::Approaching;

        // Clear target selection
        bot->SetSelection(ObjectGuid::Empty);

        // Wait a bit before retry
        context->startTime = ::std::chrono::steady_clock::now();
        context->timeout = ::std::chrono::milliseconds(2000);

        if (m_config.logInteractions)
        {
            ::std::string msg = "Attempting recovery, attempt ";
            msg += ::std::to_string(context->attemptCount + 1);
            msg += "/";
            msg += ::std::to_string(context->maxAttempts);
            LogInteraction(bot, msg);
        }

        return true;
    }

    void InteractionManager::HandleVendorList(Player* bot, WorldPacket const& /*packet*/)
    {
        if (!bot)
            return;

        auto context = GetInteractionContext(bot);
        if (!context || context->type != InteractionType::Vendor)
            return;

        // Vendor list handling is delegated to VendorInteractionManager singleton
        // which processes buy/sell operations through the complete implementation
        // Access via: VendorInteractionManager::Instance()->HandleVendorList(bot, packet);
    }

    void InteractionManager::HandleTrainerList(Player* bot, WorldPacket const& /*packet*/)
    {
        if (!bot)
            return;

        auto context = GetInteractionContext(bot);
        if (!context || context->type != InteractionType::Trainer)
            return;

        // Trainer list handling is delegated to TrainerInteractionManager singleton
        // which processes spell learning operations through the complete implementation
        // Access via: TrainerInteractionManager::Instance()->HandleTrainerList(bot, packet);
    }

    void InteractionManager::HandleGossipMessage(Player* bot, WorldPacket const& packet)
    {
        if (!bot)
            return;

        auto context = GetInteractionContext(bot);
        if (!context)
            return;

        // Pass to gossip handler
        m_gossipHandler->HandleGossipPacket(bot, packet, context->type);
    }

    InteractionResult InteractionManager::RouteToHandler(Player* /*bot*/, WorldObject* /*target*/, InteractionType type)
    {
        // RouteToHandler serves as the routing layer for the InteractionManager facade.
        // Each interaction type is implemented in a dedicated Manager singleton class.
        // This method signals that further processing should be handled by the
        // appropriate manager, accessed via their Instance() methods:
        //
        //   - VendorInteractionManager::Instance()      -> Vendor/repair interactions
        //   - TrainerInteractionManager::Instance()     -> Spell learning
        //   - InnkeeperInteractionManager::Instance()   -> Hearthstone binding
        //   - FlightMasterManager::Instance()           -> Flight paths
        //   - BankInteractionManager::Instance()        -> Bank access
        //   - MailInteractionManager::Instance()        -> Mail operations
        //   - AuctioneerInteractionManager::Instance()  -> Auction house
        //   - StableMasterInteractionManager::Instance()-> Pet stabling
        //
        // Return NotAvailable to indicate the caller should invoke the specific manager.

        switch (type)
        {
            case InteractionType::Vendor:
                // Delegate to VendorInteractionManager singleton
                return InteractionResult::NotAvailable;
            case InteractionType::Trainer:
                // Delegate to TrainerInteractionManager singleton
                return InteractionResult::NotAvailable;
            case InteractionType::Innkeeper:
                // Delegate to InnkeeperInteractionManager singleton
                return InteractionResult::NotAvailable;
            case InteractionType::FlightMaster:
                // Delegate to FlightMasterManager singleton
                return InteractionResult::NotAvailable;
            case InteractionType::Bank:
                // Delegate to BankInteractionManager singleton
                return InteractionResult::NotAvailable;
            case InteractionType::Mailbox:
                // Delegate to MailInteractionManager singleton
                return InteractionResult::NotAvailable;
            case InteractionType::Auctioneer:
                // Delegate to AuctioneerInteractionManager singleton
                return InteractionResult::NotAvailable;
            case InteractionType::StableMaster:
                // Delegate to StableMasterInteractionManager singleton
                return InteractionResult::NotAvailable;
            case InteractionType::SpiritHealer:
                // Delegate to SpiritHealerInteractionManager singleton
                return InteractionResult::NotAvailable;
            default:
                return InteractionResult::NotAvailable;
        }
    }

    bool InteractionManager::UpdateInteraction(Player* bot, InteractionContext& context, uint32 /*diff*/)
    {
        // Check timeout
    if (CheckTimeout(context))
        {
            HandleInteractionError(bot, InteractionResult::Cooldown);
            return false;
        }

        // Execute current state
        InteractionResult result = ExecuteState(bot, context);

        if (result == InteractionResult::Success)
        {
            CompleteInteraction(bot, result);
            return false;
        }
        else if (result != InteractionResult::Pending)
        {
            HandleInteractionError(bot, result);
            return false;
        }

        return true;
    }

    InteractionResult InteractionManager::ExecuteState(Player* bot, InteractionContext& context)
    {
        // PHASE 5D: Thread-safe spatial grid validation
        auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(bot, context.targetGuid);
        WorldObject* target = nullptr;

        if (snapshot)
        {
            // Get WorldObject* for state execution (validated via snapshot first)
            target = ObjectAccessor::GetWorldObject(*bot, context.targetGuid);
        }

        if (!target)
            return InteractionResult::InvalidTarget;

        switch (context.state)
        {
            case InteractionState::Approaching:
            {
                if (IsInInteractionRange(bot, target))
                {
                    context.state = InteractionState::Initiating;
                    return InteractionResult::Pending;
                }

                // Still moving - check MotionMaster instead of IsMoving()
    if (bot->GetMotionMaster()->GetCurrentMovementGeneratorType() != IDLE_MOTION_TYPE)
                    return InteractionResult::Pending;

                // Not moving and not in range = problem
                return InteractionResult::TooFarAway;
            }

            case InteractionState::Initiating:
            {
                // Face target
                bot->SetFacingToObject(target);

                // Set selection
                bot->SetSelection(target->GetGUID());
                // Initiate interaction
    if (context.needsGossip)
                {
                    // For bots, we need to interact with gossip NPCs directly
    if (Creature* creature = target->ToCreature())
                    {
                        // SendPrepareGossip was removed - use PlayerTalkClass->SendGossipMenu instead
                        // GossipMenuIds is now a vector in TrinityCore 12.0, use first entry if available
                        ::std::vector<uint32> const& gossipMenuIds = creature->GetCreatureTemplate()->GossipMenuIds;
                        uint32 gossipMenuId = gossipMenuIds.empty() ? 0 : gossipMenuIds[0];
                        bot->PlayerTalkClass->SendGossipMenu(gossipMenuId, creature->GetGUID());
                        context.state = InteractionState::WaitingGossip;
                    }
                    else
                    {
                        context.state = InteractionState::ExecutingAction;
                    }
                }
                else
                {
                    context.state = InteractionState::ExecutingAction;
                }

                return InteractionResult::Pending;
            }

            case InteractionState::WaitingGossip:
            {
                // Waiting for gossip menu from server
                // This is handled by HandleGossipMenu callback
                return InteractionResult::Pending;
            }

            case InteractionState::ProcessingMenu:
            {
                // Process gossip menu options
    if (context.gossipPath.empty())
                {
                    context.state = InteractionState::ExecutingAction;
                }
                else
                {
                    uint32 option = context.gossipPath.front();
                    context.gossipPath.erase(context.gossipPath.begin());
                    SelectGossipOption(bot, option, target);
                    context.state = InteractionState::WaitingGossip;
                }

                return InteractionResult::Pending;
            }

            case InteractionState::ExecutingAction:
            {
                // Route to specific handler
                return RouteToHandler(bot, target, context.type);
            }

            case InteractionState::Completing:
            {
                return InteractionResult::Success;
            }

            case InteractionState::Failed:
            {
                return InteractionResult::Failed;
            }

            default:
                return InteractionResult::Failed;
        }
    }

    void InteractionManager::CompleteInteraction(Player* bot, InteractionResult result)
    {
        if (!bot)
            return;

        auto context = GetInteractionContext(bot);
        if (!context)
            return;

        // Record metrics
        auto duration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
            ::std::chrono::steady_clock::now() - context->startTime);

        m_metrics[context->type].RecordAttempt(
            result == InteractionResult::Success || result == InteractionResult::PartialSuccess,
            duration);

        // Update statistics
        ++m_totalInteractionsCompleted;
        if (result != InteractionResult::Success && result != InteractionResult::PartialSuccess)
            ++m_totalInteractionsFailed;

        // Clear selection
        bot->SetSelection(ObjectGuid::Empty);

        if (m_config.logInteractions)
        {
            LogInteraction(bot, "Completed " + ::std::string(InteractionTypeToString(context->type)) +
                         " interaction with result: " + ::std::string(InteractionResultToString(result)));
        }
    }
    bool InteractionManager::CheckTimeout(const InteractionContext& context) const
    {
        return context.IsExpired();
    }

    void InteractionManager::LogInteraction(Player* bot, const ::std::string& message) const
    {
        if (bot)
        {
            TC_LOG_DEBUG("playerbot", "Bot {} interaction: {}", bot->GetName(), message);
        }
    }
}

// Implementation of InteractionRequirement::CheckRequirements
namespace Playerbot
{
    bool InteractionRequirement::CheckRequirements(::Player* bot) const
    {
        if (!bot)
            return false;

        // Level requirements
        uint32 level = bot->GetLevel();
        if (minLevel > 0 && level < minLevel)
            return false;
        if (maxLevel > 0 && level > maxLevel)
            return false;

        // Quest requirements
    if (requiredQuest > 0 && !bot->GetQuestStatus(requiredQuest))
            return false;

        // Item requirements
    if (requiredItem > 0 && !bot->HasItemCount(requiredItem, 1))
            return false;

        // Spell requirements
    if (requiredSpell > 0 && !bot->HasSpell(requiredSpell))
            return false;

        // Skill requirements
    if (requiredSkill > 0)
        {
            uint16 skillValue = bot->GetSkillValue(requiredSkill);
            if (skillValue < requiredSkillRank)
                return false;
        }

        // Faction requirements
    if (requiredFaction > 0)
        {
            ReputationRank rank = bot->GetReputationRank(requiredFaction);
            if (static_cast<uint32>(rank) < requiredFactionRank)
                return false;
        }

        // Money requirements
    if (requiredMoney > 0 && bot->GetMoney() < requiredMoney)
            return false;

        // Combat requirements
        bool inCombat = bot->IsInCombat();
        if (requiredInCombat && !inCombat)
            return false;
        if (requiredOutOfCombat && inCombat)
            return false;

        return true;
    }
}
