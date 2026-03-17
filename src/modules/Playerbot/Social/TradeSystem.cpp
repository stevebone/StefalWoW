/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "TradeSystem.h"
#include "Bag.h"
#include "CellImpl.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "DatabaseEnv.h"
#include "GameEventMgr.h"
#include "GameTime.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Log.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "Random.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "TradeData.h"
#include "Unit.h"
#include "UnitDefines.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Cells/Cell.h"
#include "Cells/CellImpl.h"
#include "Grids/Notifiers/GridNotifiers.h"
#include "Grids/Notifiers/GridNotifiersImpl.h"
#include "Config/PlayerbotTradeConfig.h"
#include "Core/PlayerBotHelpers.h"
#include "AI/BotAI.h"
#include <algorithm>
#include <cmath>

namespace Playerbot
{

TradeSystem::TradeSystem(Player* bot) : _bot(bot)
{
    if (!_bot)
    {
        TC_LOG_ERROR("playerbot", "TradeSystem: null bot!");
        return;
    }

    // Initialize global metrics
    _globalMetrics.Reset();

    // Load vendor database on construction
    LoadVendorDatabase();

    // Initialize configuration from PlayerbotTradeConfig for consistency
    TradeConfiguration config;
    config.autoAcceptTrades = PlayerbotTradeConfig::IsTradeAutoAcceptEnabled();
    config.autoDeclineTrades = !PlayerbotTradeConfig::IsTradeEnabled();
    config.acceptTradesFromGuildMembers = PlayerbotTradeConfig::IsTradeAutoAcceptGuildEnabled();
    config.acceptTradesFromFriends = PlayerbotTradeConfig::IsTradeAutoAcceptWhitelistEnabled();
    config.maxTradeValue = static_cast<uint32>(PlayerbotTradeConfig::GetTradeMaxGoldAmount());
    config.acceptanceThreshold = 1.0f - PlayerbotTradeConfig::GetTradeValueTolerance();
    config.requireItemAnalysis = PlayerbotTradeConfig::IsTradeScamProtectionEnabled();
    config.enableTradeHistory = PlayerbotTradeConfig::IsStatisticsTrackingEnabled();
    _playerConfigs[_bot->GetGUID().GetCounter()] = config;

    TC_LOG_DEBUG("bot.trade", "TradeSystem created for bot {} with config from PlayerbotTradeConfig",
        _bot->GetGUID().GetCounter());
}

TradeSystem::~TradeSystem()
{
    // Clean up any active trade sessions
    for (auto& [sessionId, session] : _activeTrades)
    {
        if (session.isActive)
        {
            CancelTradeSession(sessionId);
        }
    }
}

// Core trade functionality
bool TradeSystem::InitiateTrade(Player* initiator, Player* target)
{
    if (!initiator || !target)
        return false;

    // Check if trade can be initiated
    if (!CanInitiateTrade(initiator, target))
        return false;

    // Check if either player is already trading
    if (initiator->GetTrader() || target->GetTrader())
    {
        TC_LOG_DEBUG("playerbot", "TradeSystem: Player already in trade");
        return false;
    }

    // Create new trade session
    uint32 sessionId = _nextSessionId.fetch_add(1);
    TradeSession session(sessionId, initiator->GetGUID().GetCounter(),
                        target->GetGUID().GetCounter(), TradeType::PLAYER_TO_PLAYER);

    // Initialize the session
    InitializeTradeSession(session);

    // Store session
    _activeTrades[sessionId] = session;

    // Update metrics
    _globalMetrics.tradesInitiated.fetch_add(1);
    _playerMetrics[initiator->GetGUID().GetCounter()].tradesInitiated.fetch_add(1);

    // Use TrinityCore's trade initiation
    initiator->InitiateTrade(target);

    TC_LOG_DEBUG("playerbot", "TradeSystem: Initiated trade session {} between {} and {}",
                sessionId, initiator->GetName(), target->GetName());

    return true;
}

void TradeSystem::ProcessTradeRequest(uint32 sessionId, TradeDecision decision)
{
    auto it = _activeTrades.find(sessionId);
    if (it == _activeTrades.end())
    {
        TC_LOG_ERROR("playerbot", "TradeSystem: Session {} not found", sessionId);
        return;
    }

    TradeSession& session = it->second;

    switch (decision)
    {
        case TradeDecision::ACCEPT:
        {
            // Mark target as accepted
            session.targetAccepted = true;

            // If both accepted, complete the trade
            if (session.initiatorAccepted && session.targetAccepted)
            {
                CompleteTradeSession(sessionId);
            }
            break;
        }
        case TradeDecision::DECLINE:
        {
            CancelTradeSession(sessionId);
            break;
        }
        case TradeDecision::NEGOTIATE:
        {
            // Generate counter offer
            GenerateCounterOffer(session);
            break;
        }
        case TradeDecision::COUNTER:
        {
            // Reset acceptance states for new offer
            session.initiatorAccepted = false;
            session.targetAccepted = false;
            break;
        }
        case TradeDecision::PENDING:
        {
            // Keep session active, waiting for decision
            break;
        }
    }
}

void TradeSystem::UpdateTradeSession(uint32 sessionId)
{
    auto it = _activeTrades.find(sessionId);
    if (it == _activeTrades.end())
        return;

    TradeSession& session = it->second;

    // Check timeout
    if (GameTime::GetGameTimeMS() > session.sessionTimeout)
    {
        HandleTradeTimeout(sessionId);
        return;
    }

    // Validate trade items
    ValidateTradeItems(session);

    // Check if trade conditions have changed
    if (!ValidateTradeSession(session))
    {
        CancelTradeSession(sessionId);
        return;
    }

    // Update session timestamp
    session.sessionStartTime = GameTime::GetGameTimeMS();
}

void TradeSystem::CompleteTradeSession(uint32 sessionId)
{
    auto it = _activeTrades.find(sessionId);
    if (it == _activeTrades.end())
        return;

    TradeSession& session = it->second;

    // Final validation
    if (!ValidateTradeSession(session))
    {
        CancelTradeSession(sessionId);
        return;
    }

    // Execute the trade exchange
    ExecuteTradeExchange(session);

    // Log the transaction
    LogTradeTransaction(session);

    // Update metrics
    _globalMetrics.tradesCompleted.fetch_add(1);
    _globalMetrics.totalGoldTraded.fetch_add(session.initiatorGold + session.targetGold);
    _globalMetrics.totalItemsTraded.fetch_add(session.initiatorItems.size() + session.targetItems.size());

    // Update player history
    RecordTradeHistory(session);

    // Mark session as inactive
    session.isActive = false;

    // Remove from active trades
    _activeTrades.erase(sessionId);

    TC_LOG_DEBUG("playerbot", "TradeSystem: Completed trade session {}", sessionId);
}

void TradeSystem::CancelTradeSession(uint32 sessionId)
{
    auto it = _activeTrades.find(sessionId);
    if (it == _activeTrades.end())
        return;

    TradeSession& session = it->second;

    // Get players
    Player* initiator = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(session.initiatorGuid));
    Player* target = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(session.targetGuid));

    // Cancel trade for both players
    if (initiator && initiator->GetTrader())
        initiator->TradeCancel(true);
    if (target && target->GetTrader())
        target->TradeCancel(true);

    // Update metrics
    _globalMetrics.tradesCancelled.fetch_add(1);

    // Mark as inactive
    session.isActive = false;

    // Remove from active trades
    _activeTrades.erase(sessionId);

    TC_LOG_DEBUG("playerbot", "TradeSystem: Cancelled trade session {}", sessionId);
}

// Player-to-player trading
bool TradeSystem::CanInitiateTrade(Player* initiator, Player* target)
{
    if (!initiator || !target)
        return false;

    // Check basic conditions
    if (initiator == target)
        return false;

    // Check if players are alive
    if (!initiator->IsAlive() || !target->IsAlive())
        return false;

    // Check distance (must be within 10 yards)
    if (!initiator->IsWithinDistInMap(target, 10.0f))
        return false;

    // Check if in combat
    if (initiator->IsInCombat() || target->IsInCombat())
        return false;

    // Check if players are hostile
    if (initiator->IsHostileTo(target))
        return false;

    // Check if already trading
    if (initiator->GetTrader() || target->GetTrader())
        return false;

    return true;
}

TradeDecision TradeSystem::EvaluateTradeRequest(uint32 sessionId)
{
    auto it = _activeTrades.find(sessionId);
    if (it == _activeTrades.end())
        return TradeDecision::DECLINE;

    TradeSession& session = it->second;

    // Get configuration
    auto configIt = _playerConfigs.find(_bot->GetGUID().GetCounter());
    if (configIt == _playerConfigs.end())
        return TradeDecision::DECLINE;

    const TradeConfiguration& config = configIt->second;

    // Auto-decline if configured
    if (config.autoDeclineTrades)
        return TradeDecision::DECLINE;

    // Auto-accept if configured
    if (config.autoAcceptTrades)
        return TradeDecision::ACCEPT;

    // Check if initiator is trusted
    auto& trustLevels = _playerTradeHistory[_bot->GetGUID().GetCounter()].partnerTrustLevels;
    auto trustIt = trustLevels.find(session.initiatorGuid);
    if (trustIt != trustLevels.end() && trustIt->second >= config.acceptanceThreshold)
        return TradeDecision::ACCEPT;

    // Check if from guild member
    if (config.acceptTradesFromGuildMembers && _bot->GetGuildId())
    {
        Player* initiator = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(session.initiatorGuid));
        if (initiator && initiator->GetGuildId() == _bot->GetGuildId())
            return TradeDecision::ACCEPT;
    }

    // Analyze trade value
    float tradeValue = AnalyzeTradeValue(session);
    if (tradeValue >= config.acceptanceThreshold)
        return TradeDecision::ACCEPT;

    // Check for suspicious activity
    if (DetectSuspiciousTradeActivity(session))
        return TradeDecision::DECLINE;

    // Default to negotiation
    return TradeDecision::NEGOTIATE;
}

// Vendor interactions
void TradeSystem::LoadVendorDatabase()
{
    // Clear existing data
    _vendorDatabase.clear();
    _zoneVendors.clear();
    _vendorsByType.clear();

    // Load vendor data from creatures in world
    if (!_bot || !_bot->GetMap())
        return;

    // This would typically query the database for vendor templates
    // For now, we'll dynamically discover vendors as we encounter them
    LoadVendorsFromCreatureDatabase();
    LoadInnkeepersFromDatabase();
    LoadRepairVendorsFromDatabase();

    TC_LOG_INFO("playerbot", "TradeSystem: Loaded {} vendors", _vendorDatabase.size());
}

std::vector<VendorInfo> TradeSystem::FindNearbyVendors(float radius)
{
    std::vector<VendorInfo> vendors;

    if (!_bot || !_bot->GetMap())
        return vendors;

    // Find all creatures within radius using modern TrinityCore API
    std::list<Creature*> creatures;

    // Lambda checker for any alive creature within range
    auto check = [this, radius](Creature* creature) -> bool
    {
        if (!creature || !creature->IsAlive())
            return false;
        return _bot->IsWithinDist(creature, radius, true);
    };

    Trinity::CreatureListSearcher searcher(_bot, creatures, check);
    Cell::VisitGridObjects(_bot, searcher, radius);

    for (Creature* creature : creatures)
    {
        if (!creature || !creature->IsAlive())
            continue;

        // Check if creature is a vendor
        if (creature->HasNpcFlag(UNIT_NPC_FLAG_VENDOR) ||
            creature->HasNpcFlag(UNIT_NPC_FLAG_VENDOR_AMMO) ||
            creature->HasNpcFlag(UNIT_NPC_FLAG_VENDOR_FOOD) ||
            creature->HasNpcFlag(UNIT_NPC_FLAG_VENDOR_POISON) ||
            creature->HasNpcFlag(UNIT_NPC_FLAG_VENDOR_REAGENT))
        {
            VendorInfo info(creature->GetEntry(), creature->GetSpawnId(),
                          creature->GetName(), DetermineVendorType(creature));
            info.location = creature->GetPosition();
            info.zoneId = creature->GetZoneId();
            info.areaId = creature->GetAreaId();

            // Check for repair capability
            info.isRepairVendor = creature->HasNpcFlag(UNIT_NPC_FLAG_REPAIR);

            // Add to results
            vendors.push_back(info);
        }
    }

    return vendors;
}

bool TradeSystem::InteractWithVendor(uint32 vendorGuid)
{
    if (!_bot)
        return false;

    Creature* vendor = _bot->GetMap()->GetCreatureBySpawnId(vendorGuid);
    if (!vendor)
        return false;

    // Check if vendor is valid
    if (!vendor->IsVendor())
        return false;

    // Check distance (standard interact range is 5 yards)
    if (!_bot->IsWithinDistInMap(vendor, 5.0f))
    {
        // Move closer if needed
        if (!NavigateToVendor(vendorGuid))
            return false;
    }

    // Get vendor info
    VendorInfo info = GetVendorInfo(vendorGuid);

    // Process vendor dialog
    ProcessVendorDialog(info);

    // Update last interaction time
    info.lastInteractionTime = GameTime::GetGameTimeMS();
    _vendorDatabase[vendorGuid] = info;

    // Update metrics
    _globalMetrics.vendorTransactions.fetch_add(1);

    return true;
}

void TradeSystem::ProcessVendorBuy(uint32 vendorGuid, uint32 itemId, uint32 count)
{
    if (!_bot)
        return;

    Creature* vendor = _bot->GetMap()->GetCreatureBySpawnId(vendorGuid);
    if (!vendor || !vendor->IsVendor())
        return;

    // Find the item in vendor's list
    VendorItemData const* vendorItems = vendor->GetVendorItems();
    if (!vendorItems)
        return;

    uint32 slot = 0;
    bool found = false;
    for (VendorItem const& vItem : vendorItems->m_items)
    {
        if (vItem.item == itemId)
        {
            found = true;
            break;
        }
        ++slot;
    }

    if (!found)
    {
        TC_LOG_ERROR("playerbot", "TradeSystem: Item {} not found in vendor {}", itemId, vendorGuid);
        return;
    }

    // Use TrinityCore's buy function
    _bot->BuyItemFromVendorSlot(vendor->GetGUID(), slot, itemId, count, 0, 0);

    TC_LOG_DEBUG("playerbot", "TradeSystem: Bot {} bought {} x {} from vendor {}",
                _bot->GetName(), count, itemId, vendorGuid);
}

void TradeSystem::ProcessVendorSell(uint32 vendorGuid, uint32 itemGuid, uint32 count)
{
    if (!_bot)
        return;

    Creature* vendor = _bot->GetMap()->GetCreatureBySpawnId(vendorGuid);
    if (!vendor || !vendor->IsVendor())
        return;

    Item* item = _bot->GetItemByGuid(ObjectGuid::Create<HighGuid::Item>(itemGuid));
    if (!item)
        return;

    // Check if item can be sold
    ItemTemplate const* proto = item->GetTemplate();
    if (!proto || proto->GetSellPrice() == 0)
        return;

    // Check count validity
    if (count > item->GetCount())
        count = item->GetCount();

    // Calculate sell price
    uint32 price = proto->GetSellPrice() * count;

    // Sell the item - using TrinityCore's standard selling logic
    if (count < item->GetCount())
    {
        // Partial sell - decrease count
        _bot->ItemRemovedQuestCheck(item->GetEntry(), count);
        item->SetCount(item->GetCount() - count);
        item->SetState(ITEM_CHANGED, _bot);
        item->SendUpdateToPlayer(_bot);
    }
    else
    {
        // Full sell - remove item completely
        _bot->MoveItemFromInventory(item->GetBagSlot(), item->GetSlot(), true);
    }

    // Add money to player
    _bot->ModifyMoney(price);

    TC_LOG_DEBUG("playerbot", "TradeSystem: Bot {} sold {} x item to vendor {} for {} copper",
                _bot->GetName(), count, vendorGuid, price);
}

bool TradeSystem::CanBuyFromVendor(uint32 vendorGuid, uint32 itemId)
{
    if (!_bot)
        return false;

    Creature* vendor = _bot->GetMap()->GetCreatureBySpawnId(vendorGuid);
    if (!vendor || !vendor->IsVendor())
        return false;

    // Check vendor items
    VendorItemData const* vendorItems = vendor->GetVendorItems();
    if (!vendorItems)
        return false;

    // Find the item
    for (VendorItem const& vItem : vendorItems->m_items)
    {
        if (vItem.item == itemId)
        {
            // Check if bot has enough money
            ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
            if (proto && _bot->GetMoney() >= proto->GetBuyPrice())
                return true;
        }
    }

    return false;
}

// Equipment repair
void TradeSystem::AutoRepairEquipment()
{
    if (!_bot)
        return;

    // Calculate repair cost
    uint32 repairCost = CalculateRepairCost();
    if (repairCost == 0)
        return; // Nothing to repair

    // Check if bot has enough money
    if (_bot->GetMoney() < repairCost)
    {
        TC_LOG_DEBUG("playerbot", "TradeSystem: Not enough money for repairs (need {}, have {})",
                    repairCost, _bot->GetMoney());
        return;
    }

    // Find nearby repair vendor
    std::vector<uint32> repairVendors = FindRepairVendors(VENDOR_SEARCH_RADIUS);
    if (repairVendors.empty())
    {
        TC_LOG_DEBUG("playerbot", "TradeSystem: No repair vendors nearby");
        return;
    }

    // Use closest repair vendor
    uint32 vendorGuid = repairVendors.front();
    ProcessEquipmentRepair(vendorGuid);
}

std::vector<uint32> TradeSystem::FindRepairVendors(float radius)
{
    std::vector<uint32> repairVendors;

    if (!_bot || !_bot->GetMap())
        return repairVendors;

    // Find all creatures within radius using modern TrinityCore API
    std::list<Creature*> creatures;

    // Lambda checker for any alive creature within range
    auto check = [this, radius](Creature* creature) -> bool
    {
        if (!creature || !creature->IsAlive())
            return false;
        return _bot->IsWithinDist(creature, radius, true);
    };

    Trinity::CreatureListSearcher searcher(_bot, creatures, check);
    Cell::VisitGridObjects(_bot, searcher, radius);

    for (Creature* creature : creatures)
    {
        if (!creature || !creature->IsAlive())
            continue;

        // Check if creature can repair
        if (creature->HasNpcFlag(UNIT_NPC_FLAG_REPAIR))
        {
            repairVendors.push_back(creature->GetSpawnId());
        }
    }

    // Sort by distance
    std::sort(repairVendors.begin(), repairVendors.end(),
        [this](uint32 a, uint32 b) {
            Creature* vendorA = _bot->GetMap()->GetCreatureBySpawnId(a);
            Creature* vendorB = _bot->GetMap()->GetCreatureBySpawnId(b);
            if (!vendorA || !vendorB)
                return false;
            return _bot->GetDistance(vendorA) < _bot->GetDistance(vendorB);
        });

    return repairVendors;
}

void TradeSystem::ProcessEquipmentRepair(uint32 vendorGuid)
{
    if (!_bot)
        return;

    Creature* vendor = _bot->GetMap()->GetCreatureBySpawnId(vendorGuid);
    if (!vendor || !vendor->HasNpcFlag(UNIT_NPC_FLAG_REPAIR))
        return;

    // Check distance (standard interact range is 5 yards)
    if (!_bot->IsWithinDistInMap(vendor, 5.0f))
    {
        NavigateToVendor(vendorGuid);
        return;
    }

    // Repair all items
    _bot->DurabilityRepairAll(true, 0.0f, false);

    // Update metrics
    _globalMetrics.repairTransactions.fetch_add(1);

    TC_LOG_DEBUG("playerbot", "TradeSystem: Bot {} repaired equipment at vendor {}",
                _bot->GetName(), vendorGuid);
}

// Innkeeper services
void TradeSystem::InteractWithInnkeeper(uint32 innkeeperGuid)
{
    if (!_bot)
        return;

    Creature* innkeeper = _bot->GetMap()->GetCreatureBySpawnId(innkeeperGuid);
    if (!innkeeper || !innkeeper->HasNpcFlag(UNIT_NPC_FLAG_INNKEEPER))
        return;

    // Check distance (standard interact range is 5 yards)
    if (!_bot->IsWithinDistInMap(innkeeper, 5.0f))
    {
        NavigateToVendor(innkeeperGuid);
        return;
    }

    // Set hearthstone if needed
    if (_bot->m_homebindAreaId != _bot->GetAreaId())
    {
        SetHearthstone(innkeeperGuid);
    }

    TC_LOG_DEBUG("playerbot", "TradeSystem: Bot {} interacted with innkeeper {}",
                _bot->GetName(), innkeeperGuid);
}

std::vector<uint32> TradeSystem::FindNearbyInnkeepers(float radius)
{
    std::vector<uint32> innkeepers;

    if (!_bot || !_bot->GetMap())
        return innkeepers;

    // Find all creatures within radius using modern TrinityCore API
    std::list<Creature*> creatures;

    // Lambda checker for any alive creature within range
    auto check = [this, radius](Creature* creature) -> bool
    {
        if (!creature || !creature->IsAlive())
            return false;
        return _bot->IsWithinDist(creature, radius, true);
    };

    Trinity::CreatureListSearcher searcher(_bot, creatures, check);
    Cell::VisitGridObjects(_bot, searcher, radius);

    for (Creature* creature : creatures)
    {
        if (!creature || !creature->IsAlive())
            continue;

        // Check if creature is innkeeper
        if (creature->HasNpcFlag(UNIT_NPC_FLAG_INNKEEPER))
        {
            innkeepers.push_back(creature->GetSpawnId());
        }
    }

    return innkeepers;
}

// Intelligent trade decision making
float TradeSystem::AnalyzeTradeValue(const TradeSession& session)
{
    float initiatorValue = 0.0f;
    float targetValue = 0.0f;

    // Calculate initiator's offered value
    for (const auto& [itemGuid, count] : session.initiatorItems)
    {
        initiatorValue += CalculateItemTradeValue(itemGuid) * count;
    }
    initiatorValue += session.initiatorGold;

    // Calculate target's offered value
    for (const auto& [itemGuid, count] : session.targetItems)
    {
        targetValue += CalculateItemTradeValue(itemGuid) * count;
    }
    targetValue += session.targetGold;

    // Return ratio of what we're getting vs giving
    if (initiatorValue > 0)
        return targetValue / initiatorValue;

    return targetValue > 0 ? 10.0f : 1.0f; // If getting something for nothing, high value
}

bool TradeSystem::IsTradeWorthwhile(const TradeSession& session)
{
    // Check if trade is suspicious
    if (DetectSuspiciousTradeActivity(session))
        return false;

    // Check trade value ratio
    float valueRatio = AnalyzeTradeValue(session);
    if (valueRatio < 0.5f) // Getting less than half value
        return false;

    // Check risk assessment
    float risk = AssessTradeRisk(session);
    if (risk > 0.7f) // High risk trade
        return false;

    // Check if meets requirements
    if (!MeetsTradeRequirements(session))
        return false;

    return true;
}

// Trade safety and validation
bool TradeSystem::ValidateTradeSession(const TradeSession& session)
{
    // Check if session is active
    if (!session.isActive)
        return false;

    // Check if session has timed out
    if (GameTime::GetGameTimeMS() > session.sessionTimeout)
        return false;

    // Get players
    Player* initiator = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(session.initiatorGuid));
    Player* target = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(session.targetGuid));

    if (!initiator || !target)
        return false;

    // Check if players are still in range
    if (!initiator->IsWithinDistInMap(target, 10.0f))
        return false;

    // Check if players are still alive
    if (!initiator->IsAlive() || !target->IsAlive())
        return false;

    // Validate items still exist
    for (const auto& [itemGuid, count] : session.initiatorItems)
    {
        Item* item = initiator->GetItemByGuid(ObjectGuid::Create<HighGuid::Item>(itemGuid));
        if (!item || item->GetCount() < count)
            return false;
    }

    for (const auto& [itemGuid, count] : session.targetItems)
    {
        Item* item = target->GetItemByGuid(ObjectGuid::Create<HighGuid::Item>(itemGuid));
        if (!item || item->GetCount() < count)
            return false;
    }

    // Validate gold amounts
    if (initiator->GetMoney() < session.initiatorGold)
        return false;
    if (target->GetMoney() < session.targetGold)
        return false;

    return true;
}

bool TradeSystem::DetectSuspiciousTradeActivity(const TradeSession& session)
{
    // Check for item duplication
    if (DetectItemDuplication(session))
        return true;

    // Check for unfair trade (massive value disparity)
    if (DetectUnfairTrade(session))
        return true;

    // Check player reputations
    if (!ValidatePlayerReputations(session))
        return true;

    // Check for rapid repeated trades (potential exploitation)
    auto& history = _playerTradeHistory[session.targetGuid];
    if (!history.recentTradeOutcomes.empty())
    {
        uint32 lastTradeTime = history.lastTradeTime;
        if (GameTime::GetGameTimeMS() - lastTradeTime < 5000) // Less than 5 seconds
            return true;
    }

    return false;
}

// Performance monitoring
TradeMetrics TradeSystem::GetPlayerTradeMetrics()
{
    if (!_bot)
        return TradeMetrics();

    auto it = _playerMetrics.find(_bot->GetGUID().GetCounter());
    if (it != _playerMetrics.end())
        return it->second;

    return TradeMetrics();
}

TradeMetrics TradeSystem::GetGlobalTradeMetrics()
{
    return _globalMetrics;
}

// Automated vendor management
void TradeSystem::AutoSellJunkItems()
{
    if (!_bot)
        return;

    // Find nearby vendor
    std::vector<VendorInfo> vendors = FindNearbyVendors(VENDOR_SEARCH_RADIUS);
    if (vendors.empty())
        return;

    uint32 vendorGuid = vendors.front().creatureGuid;

    // Iterate through inventory
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        if (Bag* pBag = _bot->GetBagByPos(bag))
        {
            for (uint32 slot = 0; slot < pBag->GetBagSize(); ++slot)
            {
                Item* item = pBag->GetItemByPos(slot);
                if (!item)
                    continue;

                ItemTemplate const* proto = item->GetTemplate();
                if (!proto)
                    continue;

                // Sell gray quality items (junk)
                if (proto->GetQuality() == ITEM_QUALITY_POOR && proto->GetSellPrice() > 0)
                {
                    ProcessVendorSell(vendorGuid, item->GetGUID().GetCounter(), item->GetCount());
                }
            }
        }
    }

    TC_LOG_DEBUG("playerbot", "TradeSystem: Auto-sold junk items");
}

void TradeSystem::AutoBuyConsumables()
{
    if (!_bot)
        return;

    // Define consumables needed based on class
    std::vector<uint32> neededItems;

    switch (_bot->GetClass())
    {
        case CLASS_WARRIOR:
        case CLASS_PALADIN:
            neededItems.push_back(2287); // Haunch of Meat (food)
            neededItems.push_back(159);  // Refreshing Spring Water (drink)
            break;
        case CLASS_HUNTER:
        case CLASS_ROGUE:
            neededItems.push_back(2287); // Haunch of Meat (food)
            neededItems.push_back(2512); // Rough Arrow (ammo for hunter)
            break;
        case CLASS_PRIEST:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
            neededItems.push_back(2287); // Haunch of Meat (food)
            neededItems.push_back(159);  // Refreshing Spring Water (drink)
            break;
        case CLASS_DRUID:
        case CLASS_SHAMAN:
            neededItems.push_back(2287); // Haunch of Meat (food)
            neededItems.push_back(159);  // Refreshing Spring Water (drink)
            break;
    }

    // Find nearby vendor
    std::vector<VendorInfo> vendors = FindNearbyVendors(VENDOR_SEARCH_RADIUS);
    if (vendors.empty())
        return;

    // Try to buy needed items
    for (uint32 itemId : neededItems)
    {
        // Check if we already have enough
        if (_bot->GetItemCount(itemId) >= 20)
            continue;

        // Find vendor that sells this item
        for (const VendorInfo& vendor : vendors)
        {
            if (CanBuyFromVendor(vendor.creatureGuid, itemId))
            {
                uint32 buyCount = 20 - _bot->GetItemCount(itemId);
                ProcessVendorBuy(vendor.creatureGuid, itemId, buyCount);
                break;
            }
        }
    }

    TC_LOG_DEBUG("playerbot", "TradeSystem: Auto-bought consumables");
}

// Configuration
void TradeSystem::SetTradeConfiguration(const TradeConfiguration& config)
{
    if (!_bot)
        return;

    _playerConfigs[_bot->GetGUID().GetCounter()] = config;

    TC_LOG_DEBUG("playerbot", "TradeSystem: Updated trade configuration for bot {}",
                _bot->GetName());
}

TradeConfiguration TradeSystem::GetTradeConfiguration()
{
    if (!_bot)
        return TradeConfiguration();

    auto it = _playerConfigs.find(_bot->GetGUID().GetCounter());
    if (it != _playerConfigs.end())
        return it->second;

    return TradeConfiguration();
}

// Update and maintenance
void TradeSystem::Update(uint32 diff)
{
    static uint32 updateTimer = 0;
    updateTimer += diff;

    // Process active trades at configured interval
    if (updateTimer >= PlayerbotTradeConfig::GetTradeUpdateInterval())
    {
        ProcessActiveTrades();
        CleanupExpiredTradeSessions();
        updateTimer = 0;
    }

    // Refresh vendor data periodically
    static uint32 vendorRefreshTimer = 0;
    vendorRefreshTimer += diff;
    if (vendorRefreshTimer >= VENDOR_DATA_REFRESH_INTERVAL)
    {
        RefreshVendorData();
        vendorRefreshTimer = 0;
    }
}

void TradeSystem::ProcessActiveTrades()
{
    for (auto& [sessionId, session] : _activeTrades)
    {
        if (!session.isActive)
            continue;

        // Update session
        UpdateTradeSession(sessionId);

        // Auto-evaluate if bot is target
        if (session.targetGuid == _bot->GetGUID().GetCounter())
        {
            TradeDecision decision = EvaluateTradeRequest(sessionId);
            ProcessTradeRequest(sessionId, decision);
        }
    }
}

void TradeSystem::CleanupExpiredTradeSessions()
{
    std::vector<uint32> toRemove;
    uint32 currentTime = GameTime::GetGameTimeMS();

    for (const auto& [sessionId, session] : _activeTrades)
    {
        if (!session.isActive || currentTime > session.sessionTimeout)
        {
            toRemove.push_back(sessionId);
        }
    }

    for (uint32 sessionId : toRemove)
    {
        CancelTradeSession(sessionId);
    }
}

// Helper implementations
void TradeSystem::LoadVendorsFromCreatureDatabase()
{
    // This would typically query the database
    // For now, we rely on dynamic discovery
}

void TradeSystem::LoadInnkeepersFromDatabase()
{
    // This would typically query the database
    // For now, we rely on dynamic discovery
}

void TradeSystem::LoadRepairVendorsFromDatabase()
{
    // This would typically query the database
    // For now, we rely on dynamic discovery
}

VendorType TradeSystem::DetermineVendorType(const Creature* creature)
{
    if (!creature)
        return VendorType::GENERAL_GOODS;

    uint32 npcFlags = creature->GetNpcFlags();

    if (npcFlags & UNIT_NPC_FLAG_INNKEEPER)
        return VendorType::INNKEEPER;
    if (npcFlags & UNIT_NPC_FLAG_REPAIR)
        return VendorType::REPAIR_VENDOR;
    if (npcFlags & UNIT_NPC_FLAG_VENDOR_REAGENT)
        return VendorType::REAGENT_VENDOR;
    if (npcFlags & UNIT_NPC_FLAG_VENDOR_FOOD)
        return VendorType::FOOD_VENDOR;
    if (npcFlags & UNIT_NPC_FLAG_VENDOR_POISON)
        return VendorType::REAGENT_VENDOR;
    if (npcFlags & UNIT_NPC_FLAG_VENDOR_AMMO)
        return VendorType::WEAPON_VENDOR;
    if (npcFlags & UNIT_NPC_FLAG_FLIGHTMASTER)
        return VendorType::FLIGHT_MASTER;
    if (npcFlags & UNIT_NPC_FLAG_TRAINER)
        return VendorType::TRAINER;

    return VendorType::GENERAL_GOODS;
}

void TradeSystem::InitializeTradeSession(TradeSession& session)
{
    session.sessionStartTime = GameTime::GetGameTimeMS();
    session.sessionTimeout = session.sessionStartTime + PlayerbotTradeConfig::GetTradeTimeout();
    session.isActive = true;
}

void TradeSystem::ValidateTradeItems(TradeSession& session)
{
    // Validate initiator items
    Player* initiator = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(session.initiatorGuid));
    if (initiator)
    {
        auto it = session.initiatorItems.begin();
        while (it != session.initiatorItems.end())
        {
            Item* item = initiator->GetItemByGuid(ObjectGuid::Create<HighGuid::Item>(it->first));
            if (!item || item->GetCount() < it->second)
            {
                it = session.initiatorItems.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    // Validate target items
    Player* target = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(session.targetGuid));
    if (target)
    {
        auto it = session.targetItems.begin();
        while (it != session.targetItems.end())
        {
            Item* item = target->GetItemByGuid(ObjectGuid::Create<HighGuid::Item>(it->first));
            if (!item || item->GetCount() < it->second)
            {
                it = session.targetItems.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
}

void TradeSystem::ExecuteTradeExchange(TradeSession& session)
{
    Player* initiator = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(session.initiatorGuid));
    Player* target = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(session.targetGuid));

    if (!initiator || !target)
        return;

    // Exchange gold
    if (session.initiatorGold > 0)
    {
        initiator->ModifyMoney(-static_cast<int32>(session.initiatorGold));
        target->ModifyMoney(session.initiatorGold);
    }
    if (session.targetGold > 0)
    {
        target->ModifyMoney(-static_cast<int32>(session.targetGold));
        initiator->ModifyMoney(session.targetGold);
    }

    // Exchange items
    for (const auto& [itemGuid, count] : session.initiatorItems)
    {
        Item* item = initiator->GetItemByGuid(ObjectGuid::Create<HighGuid::Item>(itemGuid));
        if (item)
        {
            initiator->MoveItemFromInventory(item->GetBagSlot(), item->GetSlot(), true);
            target->AddItem(item->GetEntry(), count);
        }
    }

    for (const auto& [itemGuid, count] : session.targetItems)
    {
        Item* item = target->GetItemByGuid(ObjectGuid::Create<HighGuid::Item>(itemGuid));
        if (item)
        {
            target->MoveItemFromInventory(item->GetBagSlot(), item->GetSlot(), true);
            initiator->AddItem(item->GetEntry(), count);
        }
    }
}

void TradeSystem::NotifyTradeParticipants(const TradeSession& session, const std::string& message)
{
    Player* initiator = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(session.initiatorGuid));
    Player* target = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(session.targetGuid));

    // Log the notification instead of sending system message
    TC_LOG_DEBUG("playerbot", "TradeSystem: Trade notification - {} (Session {} between {} and {})",
                message, session.sessionId,
                initiator ? initiator->GetName() : "Unknown",
                target ? target->GetName() : "Unknown");
}

bool TradeSystem::NavigateToVendor(uint32 vendorGuid)
{
    if (!_bot)
        return false;

    Creature* vendor = _bot->GetMap()->GetCreatureBySpawnId(vendorGuid);
    if (!vendor)
        return false;

    // Use validated pathfinding for bot movement to vendor
    Position dest = vendor->GetPosition();
    if (BotAI* ai = GetBotAI(_bot))
    {
        if (!ai->MoveTo(dest, true))
        {
            // Fallback to legacy if validation fails
            _bot->GetMotionMaster()->MovePoint(0, vendor->GetPosition());
        }
    }
    else
    {
        // Non-bot player - use standard movement
        _bot->GetMotionMaster()->MovePoint(0, vendor->GetPosition());
    }

    return true;
}

void TradeSystem::ProcessVendorDialog(const VendorInfo& vendor)
{
    // Process vendor gossip dialog
    AnalyzeVendorInventory(vendor);
}

void TradeSystem::AnalyzeVendorInventory(const VendorInfo& vendor)
{
    if (!_bot)
        return;

    Creature* vendorCreature = _bot->GetMap()->GetCreatureBySpawnId(vendor.creatureGuid);
    if (!vendorCreature)
        return;

    VendorItemData const* vendorItems = vendorCreature->GetVendorItems();
    if (!vendorItems)
        return;

    // Analyze available items
    for (VendorItem const& vItem : vendorItems->m_items)
    {
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(vItem.item);
        if (proto)
        {
            // Store useful items for future reference
            TC_LOG_DEBUG("playerbot", "TradeSystem: Vendor {} sells {} (id: {})",
                        vendor.creatureGuid, proto->GetDefaultLocaleName(), vItem.item);
        }
    }
}

float TradeSystem::CalculateItemTradeValue(uint32 itemGuid)
{
    if (!_bot)
        return 0.0f;

    Item* item = _bot->GetItemByGuid(ObjectGuid::Create<HighGuid::Item>(itemGuid));
    if (!item)
        return 0.0f;

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto)
        return 0.0f;

    // Base value on vendor price
    float value = static_cast<float>(proto->GetSellPrice() * item->GetCount());

    // Adjust based on quality
    switch (proto->GetQuality())
    {
        case ITEM_QUALITY_POOR:
            value *= 0.5f;
            break;
        case ITEM_QUALITY_NORMAL:
            value *= 1.0f;
            break;
        case ITEM_QUALITY_UNCOMMON:
            value *= 1.5f;
            break;
        case ITEM_QUALITY_RARE:
            value *= 2.0f;
            break;
        case ITEM_QUALITY_EPIC:
            value *= 3.0f;
            break;
        case ITEM_QUALITY_LEGENDARY:
            value *= 5.0f;
            break;
    }

    return value;
}

float TradeSystem::AssessTradeRisk(const TradeSession& session)
{
    float risk = 0.0f;

    // Unknown trader increases risk
    auto& history = _playerTradeHistory[_bot->GetGUID().GetCounter()];
    auto trustIt = history.partnerTrustLevels.find(session.initiatorGuid);
    if (trustIt == history.partnerTrustLevels.end())
    {
        risk += 0.3f; // Unknown trader
    }
    else
    {
        risk += (1.0f - trustIt->second) * 0.5f; // Lower trust = higher risk
    }

    // Large value disparity increases risk
    float valueRatio = AnalyzeTradeValue(session);
    if (valueRatio < 0.5f || valueRatio > 2.0f)
    {
        risk += 0.2f;
    }

    // Multiple high value items increase risk
    for (const auto& [itemGuid, count] : session.targetItems)
    {
        float itemValue = CalculateItemTradeValue(itemGuid);
        if (itemValue > 10000) // High value item
        {
            risk += 0.1f;
        }
    }

    return std::min(risk, 1.0f);
}

bool TradeSystem::MeetsTradeRequirements(const TradeSession& session)
{
    auto configIt = _playerConfigs.find(_bot->GetGUID().GetCounter());
    if (configIt == _playerConfigs.end())
        return true; // No config, accept by default

    const TradeConfiguration& config = configIt->second;

    // Check max trade value
    float totalValue = 0.0f;
    for (const auto& [itemGuid, count] : session.initiatorItems)
    {
        totalValue += CalculateItemTradeValue(itemGuid);
    }
    totalValue += session.initiatorGold;

    if (totalValue > config.maxTradeValue)
        return false;

    // Check blacklisted items
    for (const auto& [itemGuid, count] : session.targetItems)
    {
        Item* item = _bot->GetItemByGuid(ObjectGuid::Create<HighGuid::Item>(itemGuid));
        if (item && config.blacklistedItems.find(item->GetEntry()) != config.blacklistedItems.end())
            return false;
    }

    return true;
}

void TradeSystem::GenerateCounterOffer(TradeSession& session)
{
    // Calculate fair trade value
    float targetValue = 0.0f;
    for (const auto& [itemGuid, count] : session.targetItems)
    {
        targetValue += CalculateItemTradeValue(itemGuid);
    }
    targetValue += session.targetGold;

    // Adjust initiator's offer to match
    float initiatorValue = 0.0f;
    for (const auto& [itemGuid, count] : session.initiatorItems)
    {
        initiatorValue += CalculateItemTradeValue(itemGuid);
    }

    // If initiator value is too low, request more gold
    if (initiatorValue < targetValue * 0.8f)
    {
        session.initiatorGold = static_cast<uint32>(targetValue - initiatorValue);
    }

    // Notify participants of counter offer
    NotifyTradeParticipants(session, "Counter offer proposed");
}

bool TradeSystem::DetectItemDuplication(const TradeSession& session)
{
    // Check for duplicate item GUIDs (potential exploit)
    std::unordered_set<uint32> seenItems;

    for (const auto& [itemGuid, count] : session.initiatorItems)
    {
        if (seenItems.find(itemGuid) != seenItems.end())
            return true; // Duplicate item detected
        seenItems.insert(itemGuid);
    }

    for (const auto& [itemGuid, count] : session.targetItems)
    {
        if (seenItems.find(itemGuid) != seenItems.end())
            return true; // Duplicate item detected
        seenItems.insert(itemGuid);
    }

    return false;
}

bool TradeSystem::DetectUnfairTrade(const TradeSession& session)
{
    float valueRatio = AnalyzeTradeValue(session);

    // If getting much more or much less than giving
    if (valueRatio < 0.1f || valueRatio > MAX_TRADE_VALUE_RATIO)
        return true;

    return false;
}

bool TradeSystem::ValidatePlayerReputations(const TradeSession& session)
{
    // Check if player has been marked as scammer
    auto& history = _playerTradeHistory[session.initiatorGuid];

    // Check recent trade outcomes
    int failedTrades = 0;
    for (const auto& [sessionId, success] : history.recentTradeOutcomes)
    {
        if (!success)
            failedTrades++;
    }

    // Too many failed trades indicates potential scammer
    if (failedTrades > 5)
        return false;

    return true;
}

void TradeSystem::UpdateScamDetectionDatabase(uint32 suspectedScammer)
{
    // Update trust level for suspected scammer
    auto& history = _playerTradeHistory[_bot->GetGUID().GetCounter()];
    history.partnerTrustLevels[suspectedScammer] = 0.0f;

    TC_LOG_WARN("playerbot", "TradeSystem: Player {} marked as suspected scammer", suspectedScammer);
}

void TradeSystem::RefreshVendorData()
{
    UpdateVendorAvailability();
}

void TradeSystem::UpdateVendorAvailability()
{
    // Update vendor availability based on time of day, events, etc.
    // This would check game events, vendor spawn times, etc.
}

uint32 TradeSystem::CalculateRepairCost()
{
    if (!_bot)
        return 0;

    uint32 totalCost = 0;

    // Check all equipped items
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        Item* item = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item)
            continue;

        // Get durability values from item data
        uint32 maxDurability = *item->m_itemData->MaxDurability;
        uint32 curDurability = *item->m_itemData->Durability;

        if (maxDurability && curDurability < maxDurability)
        {
            ItemTemplate const* proto = item->GetTemplate();
            if (!proto)
                continue;

            // Calculate repair cost based on durability loss
            float durabilityLoss = (float)(maxDurability - curDurability) / maxDurability;
            totalCost += uint32(proto->GetSellPrice() * durabilityLoss * 0.1f); // 10% of sell price per full repair
        }
    }

    return totalCost;
}

void TradeSystem::RecordTradeHistory(const TradeSession& session)
{
    auto& history = _playerTradeHistory[_bot->GetGUID().GetCounter()];

    // Record completed trade
    history.completedTrades.push_back(session.sessionId);

    // Update partner counts
    uint32 partnerGuid = (session.initiatorGuid == _bot->GetGUID().GetCounter())
                        ? session.targetGuid : session.initiatorGuid;
    history.tradePartnerCounts[partnerGuid]++;

    // Update trust level (successful trade increases trust)
    auto trustIt = history.partnerTrustLevels.find(partnerGuid);
    if (trustIt != history.partnerTrustLevels.end())
    {
        trustIt->second = std::min(1.0f, trustIt->second + 0.1f);
    }
    else
    {
        history.partnerTrustLevels[partnerGuid] = 0.6f; // Start with neutral trust
    }

    // Record outcome
    history.recentTradeOutcomes.push_back({session.sessionId, true});

    // Limit history size
    if (history.recentTradeOutcomes.size() > TRADE_HISTORY_SIZE)
    {
        history.recentTradeOutcomes.erase(history.recentTradeOutcomes.begin());
    }

    // Update total value
    for (const auto& [itemGuid, count] : session.initiatorItems)
    {
        history.totalTradeValue += static_cast<uint32>(CalculateItemTradeValue(itemGuid));
    }
    history.totalTradeValue += session.initiatorGold;

    // Update timestamp
    history.lastTradeTime = GameTime::GetGameTimeMS();
}

void TradeSystem::LogTradeTransaction(const TradeSession& session)
{
    Player* initiator = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(session.initiatorGuid));
    Player* target = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(session.targetGuid));

    std::string initiatorName = initiator ? initiator->GetName() : "Unknown";
    std::string targetName = target ? target->GetName() : "Unknown";

    TC_LOG_INFO("playerbot", "TradeSystem: Trade completed - Session {}, {} <-> {}, "
                "Items: {}/{}, Gold: {}/{}",
                session.sessionId, initiatorName, targetName,
                session.initiatorItems.size(), session.targetItems.size(),
                session.initiatorGold, session.targetGold);
}

void TradeSystem::HandleTradeTimeout(uint32 sessionId)
{
    auto it = _activeTrades.find(sessionId);
    if (it == _activeTrades.end())
        return;

    TradeSession& session = it->second;

    TC_LOG_DEBUG("playerbot", "TradeSystem: Trade session {} timed out", sessionId);

    // Update metrics
    auto& history = _playerTradeHistory[_bot->GetGUID().GetCounter()];
    history.recentTradeOutcomes.push_back({sessionId, false});

    // Cancel the session
    CancelTradeSession(sessionId);
}

void TradeSystem::AddItemToTrade(uint32 sessionId, uint32 itemGuid, uint32 count)
{
    auto it = _activeTrades.find(sessionId);
    if (it == _activeTrades.end())
        return;

    TradeSession& session = it->second;

    // Determine which side is adding the item
    if (session.initiatorGuid == _bot->GetGUID().GetCounter())
    {
        session.initiatorItems.push_back({itemGuid, count});
    }
    else if (session.targetGuid == _bot->GetGUID().GetCounter())
    {
        session.targetItems.push_back({itemGuid, count});
    }

    // Reset acceptance states since trade changed
    session.initiatorAccepted = false;
    session.targetAccepted = false;
}

void TradeSystem::SetTradeGold(uint32 sessionId, uint32 goldAmount)
{
    auto it = _activeTrades.find(sessionId);
    if (it == _activeTrades.end())
        return;

    TradeSession& session = it->second;

    // Determine which side is setting gold
    if (session.initiatorGuid == _bot->GetGUID().GetCounter())
    {
        session.initiatorGold = goldAmount;
    }
    else if (session.targetGuid == _bot->GetGUID().GetCounter())
    {
        session.targetGold = goldAmount;
    }

    // Reset acceptance states since trade changed
    session.initiatorAccepted = false;
    session.targetAccepted = false;
}

VendorInfo TradeSystem::GetVendorInfo(uint32 creatureGuid)
{
    auto it = _vendorDatabase.find(creatureGuid);
    if (it != _vendorDatabase.end())
        return it->second;

    // Create new vendor info if not found
    Creature* vendor = _bot->GetMap()->GetCreatureBySpawnId(creatureGuid);
    if (vendor)
    {
        VendorInfo info(vendor->GetEntry(), vendor->GetSpawnId(), vendor->GetName(), DetermineVendorType(vendor));
        info.location = vendor->GetPosition();
        info.zoneId = vendor->GetZoneId();
        info.areaId = vendor->GetAreaId();
        info.isRepairVendor = vendor->HasNpcFlag(UNIT_NPC_FLAG_REPAIR);
        info.isInnkeeper = vendor->HasNpcFlag(UNIT_NPC_FLAG_INNKEEPER);
        info.isFlightMaster = vendor->HasNpcFlag(UNIT_NPC_FLAG_FLIGHTMASTER);

        _vendorDatabase[vendor->GetSpawnId()] = info;
        return info;
    }

    return VendorInfo(0, 0, "", VendorType::GENERAL_GOODS);
}

bool TradeSystem::ShouldSellToVendor(uint32 itemGuid)
{
    if (!_bot)
        return false;

    Item* item = _bot->GetItemByGuid(ObjectGuid::Create<HighGuid::Item>(itemGuid));
    if (!item)
        return false;

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto)
        return false;

    // Always sell gray items
    if (proto->GetQuality() == ITEM_QUALITY_POOR)
        return true;

    // Check if item is useful for bot's class
    // This would require more complex class-specific logic

    return false;
}

void TradeSystem::SetHearthstone(uint32 innkeeperGuid)
{
    if (!_bot)
        return;

    Creature* innkeeper = _bot->GetMap()->GetCreatureBySpawnId(innkeeperGuid);
    if (!innkeeper || !innkeeper->HasNpcFlag(UNIT_NPC_FLAG_INNKEEPER))
        return;

    // Set home bind
    _bot->SetHomebind(_bot->GetWorldLocation(), _bot->GetAreaId());

    TC_LOG_DEBUG("playerbot", "TradeSystem: Bot {} set hearthstone at innkeeper {}",
                _bot->GetName(), innkeeperGuid);
}

bool TradeSystem::CanUseInnkeeperServices(uint32 innkeeperGuid)
{
    if (!_bot)
        return false;

    Creature* innkeeper = _bot->GetMap()->GetCreatureBySpawnId(innkeeperGuid);
    if (!innkeeper || !innkeeper->HasNpcFlag(UNIT_NPC_FLAG_INNKEEPER))
        return false;

    // Check if in range (standard interact range is 5 yards)
    if (!_bot->IsWithinDistInMap(innkeeper, 5.0f))
        return false;

    // Check if innkeeper is friendly
    if (_bot->IsHostileTo(innkeeper))
        return false;

    return true;
}

void TradeSystem::GenerateTradeRecommendation(uint32 sessionId)
{
    auto it = _activeTrades.find(sessionId);
    if (it == _activeTrades.end())
        return;

    const TradeSession& session = it->second;

    float value = AnalyzeTradeValue(session);
    bool worthwhile = IsTradeWorthwhile(session);

    std::string recommendation;
    if (worthwhile)
    {
        recommendation = "Trade recommended - good value ratio";
    }
    else if (value < 0.5f)
    {
        recommendation = "Trade not recommended - poor value";
    }
    else if (DetectSuspiciousTradeActivity(session))
    {
        recommendation = "Trade not recommended - suspicious activity detected";
    }
    else
    {
        recommendation = "Trade neutral - consider negotiating";
    }

    TC_LOG_DEBUG("playerbot", "TradeSystem: Session {} recommendation: {}",
                sessionId, recommendation);
}

TradeDecision TradeSystem::MakeAutomatedTradeDecision(uint32 sessionId)
{
    return EvaluateTradeRequest(sessionId);
}

void TradeSystem::HandleTradeScamAttempt(Player* victim, Player* scammer)
{
    if (!victim || !scammer)
        return;

    // Update scammer in database
    UpdateScamDetectionDatabase(scammer->GetGUID().GetCounter());

    // Log the attempt
    TC_LOG_WARN("playerbot", "TradeSystem: Scam attempt detected - Victim: {}, Scammer: {}",
                victim->GetName(), scammer->GetName());

    // Cancel any active trades with scammer
    for (auto& [sessionId, session] : _activeTrades)
    {
        if (session.initiatorGuid == scammer->GetGUID().GetCounter() ||
            session.targetGuid == scammer->GetGUID().GetCounter())
        {
            CancelTradeSession(sessionId);
        }
    }
}

void TradeSystem::AutoRepairWhenNeeded()
{
    uint32 repairCost = CalculateRepairCost();
    if (repairCost > REPAIR_COST_THRESHOLD)
    {
        AutoRepairEquipment();
    }
}

void TradeSystem::ManageInventorySpace()
{
    if (!_bot)
        return;

    // Count free slots
    uint32 freeSlots = 0;
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        if (Bag* pBag = _bot->GetBagByPos(bag))
        {
            freeSlots += pBag->GetFreeSlots();
        }
    }

    // If low on space, sell junk
    if (freeSlots < 5)
    {
        AutoSellJunkItems();
    }
}

void TradeSystem::AnalyzeTradePatterns()
{
    // Analyze recent trades to identify patterns
    auto& history = _playerTradeHistory[_bot->GetGUID().GetCounter()];

    // Calculate success rate
    int successful = 0;
    for (const auto& [sessionId, success] : history.recentTradeOutcomes)
    {
        if (success)
            successful++;
    }

    float successRate = history.recentTradeOutcomes.empty() ? 0.0f :
                       static_cast<float>(successful) / history.recentTradeOutcomes.size();

    _globalMetrics.tradeSuccessRate = successRate;
}

void TradeSystem::LearnFromTradeOutcomes(uint32 sessionId, bool wasSuccessful)
{
    auto& history = _playerTradeHistory[_bot->GetGUID().GetCounter()];
    history.recentTradeOutcomes.push_back({sessionId, wasSuccessful});

    // Limit history size
    if (history.recentTradeOutcomes.size() > TRADE_HISTORY_SIZE)
    {
        history.recentTradeOutcomes.erase(history.recentTradeOutcomes.begin());
    }

    // Adjust trading behavior based on outcomes
    AdaptTradingBehavior();
}

void TradeSystem::AdaptTradingBehavior()
{
    auto& history = _playerTradeHistory[_bot->GetGUID().GetCounter()];
    auto& config = _playerConfigs[_bot->GetGUID().GetCounter()];

    // Count recent failures
    int recentFailures = 0;
    for (auto it = history.recentTradeOutcomes.rbegin();
         it != history.recentTradeOutcomes.rend() && std::distance(history.recentTradeOutcomes.rbegin(), it) < 10;
         ++it)
    {
        if (!it->second)
            recentFailures++;
    }

    // If too many failures, become more cautious
    if (recentFailures > 5)
    {
        config.acceptanceThreshold = std::min(0.9f, config.acceptanceThreshold + 0.1f);
        config.requireItemAnalysis = true;
    }
    else if (recentFailures == 0)
    {
        // If all successful, can be less cautious
        config.acceptanceThreshold = std::max(0.3f, config.acceptanceThreshold - 0.05f);
    }
}

void TradeSystem::HandleGuildBankInteraction(uint32 guildBankGuid)
{
    if (!_bot || !_bot->GetGuildId())
        return;

    Guild* guild = sGuildMgr->GetGuildById(_bot->GetGuildId());
    if (!guild)
        return;

    // This would interact with guild bank
    TC_LOG_DEBUG("playerbot", "TradeSystem: Bot {} interacting with guild bank",
                _bot->GetName());
}

void TradeSystem::ProcessGuildBankDeposit(uint32 itemGuid, uint32 count)
{
    if (!_bot || !_bot->GetGuildId())
        return;

    Guild* guild = sGuildMgr->GetGuildById(_bot->GetGuildId());
    if (!guild)
        return;

    Item* item = _bot->GetItemByGuid(ObjectGuid::Create<HighGuid::Item>(itemGuid));
    if (!item || item->GetCount() < count)
        return;

    // This would deposit to guild bank
    TC_LOG_DEBUG("playerbot", "TradeSystem: Bot {} depositing item to guild bank",
                _bot->GetName());
}

void TradeSystem::ProcessGuildBankWithdrawal(uint32 itemId, uint32 count)
{
    if (!_bot || !_bot->GetGuildId())
        return;

    Guild* guild = sGuildMgr->GetGuildById(_bot->GetGuildId());
    if (!guild)
        return;

    // This would withdraw from guild bank
    TC_LOG_DEBUG("playerbot", "TradeSystem: Bot {} withdrawing from guild bank",
                _bot->GetName());
}

bool TradeSystem::CanAccessGuildBank()
{
    if (!_bot || !_bot->GetGuildId())
        return false;

    Guild* guild = sGuildMgr->GetGuildById(_bot->GetGuildId());
    if (!guild)
        return false;

    // Check if bot has guild bank permissions
    // Use const access to GetMember
    Guild::Member const* member = const_cast<Guild const*>(guild)->GetMember(_bot->GetGUID());
    if (!member)
        return false;

    // Lower rank IDs have more permissions (0 = guild master)
    // Check if member has sufficient rank to access guild bank (typically ranks 0-2)
    return static_cast<uint8>(member->GetRankId()) <= 2;
}

void TradeSystem::UpdatePlayerTrustLevel(uint32 targetGuid, float trustDelta)
{
    auto& history = _playerTradeHistory[_bot->GetGUID().GetCounter()];
    auto& trustLevel = history.partnerTrustLevels[targetGuid];

    trustLevel = std::max(0.0f, std::min(1.0f, trustLevel + trustDelta));
}

float TradeSystem::GetPlayerTrustLevel(uint32 targetGuid)
{
    auto& history = _playerTradeHistory[_bot->GetGUID().GetCounter()];
    auto it = history.partnerTrustLevels.find(targetGuid);

    if (it != history.partnerTrustLevels.end())
        return it->second;

    return 0.5f; // Default neutral trust
}

void TradeSystem::HandleTradeError(uint32 sessionId, const std::string& error)
{
    TC_LOG_ERROR("playerbot", "TradeSystem: Session {} error: {}", sessionId, error);

    auto it = _activeTrades.find(sessionId);
    if (it != _activeTrades.end())
    {
        NotifyTradeParticipants(it->second, "Trade error: " + error);
        RecoverFromTradeFailure(sessionId);
    }
}

void TradeSystem::RecoverFromTradeFailure(uint32 sessionId)
{
    // Attempt to recover from trade failure
    auto it = _activeTrades.find(sessionId);
    if (it == _activeTrades.end())
        return;

    TradeSession& session = it->second;

    // Reset acceptance states
    session.initiatorAccepted = false;
    session.targetAccepted = false;

    // Validate session again
    if (!ValidateTradeSession(session))
    {
        CancelTradeSession(sessionId);
    }
}

void TradeSystem::ValidateTradeStates()
{
    // Validate all active trade sessions
    std::vector<uint32> invalidSessions;

    for (const auto& [sessionId, session] : _activeTrades)
    {
        if (!ValidateTradeSession(session))
        {
            invalidSessions.push_back(sessionId);
        }
    }

    // Cancel invalid sessions
    for (uint32 sessionId : invalidSessions)
    {
        CancelTradeSession(sessionId);
    }
}

void TradeSystem::OptimizeVendorTransactions()
{
    // Optimize vendor interactions by batching operations
    // This would batch multiple buy/sell operations together
}

void TradeSystem::OptimizeVendorQueries()
{
    // Optimize database queries for vendor data
    // This would cache frequently accessed vendor data
}

void TradeSystem::CacheFrequentVendorData()
{
    // Cache data for frequently visited vendors
    // This reduces database lookups
}

void TradeSystem::PreloadNearbyVendors()
{
    // Preload vendor data for nearby zones
    // This improves response time when finding vendors
}

void TradeSystem::UpdateTradeMetrics(const TradeSession& session, bool wasSuccessful)
{
    if (wasSuccessful)
    {
        _globalMetrics.tradesCompleted.fetch_add(1);
    }
    else
    {
        _globalMetrics.tradesCancelled.fetch_add(1);
    }

    // Update average trade value
    float totalValue = 0.0f;
    for (const auto& [itemGuid, count] : session.initiatorItems)
    {
        totalValue += CalculateItemTradeValue(itemGuid);
    }
    totalValue += session.initiatorGold;

    // Simple moving average
    float currentAvg = _globalMetrics.averageTradeValue;
    float newAvg = (currentAvg * 0.9f) + (totalValue * 0.1f);
    _globalMetrics.averageTradeValue = newAvg;

    _globalMetrics.lastUpdate = GameTime::GetGameTimeMS();
}

} // namespace Playerbot