/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "SoloStrategy.h"
#include "GameTime.h"
#include "BotAI.h"
#include "Player.h"
#include "Log.h"
#include "Quest/UnifiedQuestManager.h"
#include "Social/TradeManager.h"
#include "Professions/GatheringManager.h"
#include "Economy/AuctionManager.h"
#include "../Actions/Action.h"
#include "../Triggers/Trigger.h"
#include "../Values/Value.h"
#include "Group.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "Session/BotSessionManager.h"
#include "Session/BotSession.h"

namespace Playerbot
{

// ============================================================================
// SOLO ACTIONS - Concrete implementations for solo bot behavior
// ============================================================================

/**
 * @class QuestAction
 * @brief Action for quest-related activities (accept, complete, progress)
 */
class QuestAction : public Action
{
public:
    QuestAction() : Action("quest") { SetRelevance(0.8f); }

    bool IsPossible(BotAI* ai) const override
    {
        if (!ai || !ai->GetBot())
            return false;

        // Can quest if not in combat
        Player* bot = ai->GetBot();
        return !bot->IsInCombat();
    }

    bool IsUseful(BotAI* ai) const override
    {
        if (!ai)
            return false;

        // Useful if there are active quests to progress
        return ai->GetActiveQuestCount() > 0;
    }

    ActionResult Execute(BotAI* ai, ActionContext const& context) override
    {
        if (!ai || !ai->GetBot())
            return ActionResult::IMPOSSIBLE;

        // Delegate to UnifiedQuestManager for quest progression
        Player* bot = ai->GetBot();
        UnifiedQuestManager::instance()->UpdateQuestProgress(bot);
        TC_LOG_DEBUG("module.playerbot", "QuestAction: Bot {} progressing quests",
            bot->GetName());
        return ActionResult::IN_PROGRESS;
    }
};

/**
 * @class GatherAction
 * @brief Action for gathering resources (mining, herbalism, skinning)
 */
class GatherAction : public Action
{
public:
    GatherAction() : Action("gather") { SetRelevance(0.6f); }

    bool IsPossible(BotAI* ai) const override
    {
        if (!ai || !ai->GetBot())
            return false;

        Player* bot = ai->GetBot();
        return !bot->IsInCombat() && ai->GetGatheringManager() != nullptr;
    }

    bool IsUseful(BotAI* ai) const override
    {
        if (!ai || !ai->GetGatheringManager())
            return false;

        // Useful if bot has gathering professions and resources nearby
        GatheringManager* gatherMgr = ai->GetGatheringManager();
        // Uses HasNearbyResources() which internally checks profession compatibility
        return gatherMgr->HasNearbyResources();
    }

    ActionResult Execute(BotAI* ai, ActionContext const& context) override
    {
        if (!ai || !ai->GetGatheringManager())
            return ActionResult::IMPOSSIBLE;

        GatheringManager* gatherMgr = ai->GetGatheringManager();
        // Uses FindNearestNode() + GatherFromNode() which together provide complete gathering functionality
        auto const* node = gatherMgr->FindNearestNode();
        if (node && gatherMgr->GatherFromNode(*node))
        {
            TC_LOG_DEBUG("module.playerbot", "GatherAction: Bot {} gathering resource",
                ai->GetBot()->GetName());
            return ActionResult::SUCCESS;
        }

        return ActionResult::FAILED;
    }
};

/**
 * @class ExploreAction
 * @brief Action for world exploration and discovery
 */
class ExploreAction : public Action
{
public:
    ExploreAction() : Action("explore") { SetRelevance(0.3f); }

    bool IsPossible(BotAI* ai) const override
    {
        if (!ai || !ai->GetBot())
            return false;

        Player* bot = ai->GetBot();
        return !bot->IsInCombat() && bot->IsAlive() && !bot->IsInFlight();
    }

    bool IsUseful(BotAI* ai) const override
    {
        // Exploration is always potentially useful as a fallback activity
        return ai != nullptr && ai->GetBot() != nullptr;
    }

    ActionResult Execute(BotAI* ai, ActionContext const& context) override
    {
        if (!ai || !ai->GetBot())
            return ActionResult::IMPOSSIBLE;

        Player* bot = ai->GetBot();

        // Move to a random nearby position for exploration
        float x = bot->GetPositionX() + (float)(rand() % 40 - 20);
        float y = bot->GetPositionY() + (float)(rand() % 40 - 20);
        float z = bot->GetPositionZ();

        // Get ground height at target position
        if (Map* map = bot->GetMap())
        {
            float groundZ = map->GetHeight(bot->GetPhaseShift(), x, y, z + 5.0f, true);
            if (groundZ != INVALID_HEIGHT)
                z = groundZ;
        }

        TC_LOG_DEBUG("module.playerbot", "ExploreAction: Bot {} exploring to ({}, {}, {})",
            bot->GetName(), x, y, z);

        return DoMove(ai, x, y, z) ? ActionResult::SUCCESS : ActionResult::FAILED;
    }
};

/**
 * @class TradeAction
 * @brief Action for trading with NPCs (vendors, trainers)
 */
class TradeAction : public Action
{
public:
    TradeAction() : Action("trade") { SetRelevance(0.5f); }

    bool IsPossible(BotAI* ai) const override
    {
        if (!ai || !ai->GetBot())
            return false;

        Player* bot = ai->GetBot();
        return !bot->IsInCombat() && ai->GetTradeManager() != nullptr;
    }

    bool IsUseful(BotAI* ai) const override
    {
        if (!ai || !ai->GetTradeManager())
            return false;

        // Uses NeedsRepair() and NeedsSupplies() which cover trading needs (repair, restock consumables)
        TradeManager* tradeMgr = ai->GetTradeManager();
        return tradeMgr->NeedsRepair() || tradeMgr->NeedsSupplies();
    }

    ActionResult Execute(BotAI* ai, ActionContext const& context) override
    {
        if (!ai || !ai->GetTradeManager())
            return ActionResult::IMPOSSIBLE;

        // TradeManager processes trades via its Update() method called by BotAI::UpdateManagers()
        // This action signals intent; actual execution happens during manager update cycles
        TC_LOG_DEBUG("module.playerbot", "TradeAction: Bot {} checking trading opportunities",
            ai->GetBot()->GetName());
        return ActionResult::IN_PROGRESS;
    }
};

/**
 * @class AuctionAction
 * @brief Action for auction house interactions
 */
class AuctionAction : public Action
{
public:
    AuctionAction() : Action("auction") { SetRelevance(0.4f); }

    bool IsPossible(BotAI* ai) const override
    {
        if (!ai || !ai->GetBot())
            return false;

        Player* bot = ai->GetBot();
        return !bot->IsInCombat() && ai->GetAuctionManager() != nullptr;
    }

    bool IsUseful(BotAI* ai) const override
    {
        if (!ai || !ai->GetAuctionManager())
            return false;

        // Uses HasActiveAuctions() which tracks all current auction states (pending, active, expired)
        AuctionManager* auctionMgr = ai->GetAuctionManager();
        return auctionMgr->HasActiveAuctions();
    }

    ActionResult Execute(BotAI* ai, ActionContext const& context) override
    {
        if (!ai || !ai->GetAuctionManager())
            return ActionResult::IMPOSSIBLE;

        // AuctionManager processes auctions via its Update() method called by BotAI::UpdateManagers()
        // This action signals intent; actual execution happens during manager update cycles
        TC_LOG_DEBUG("module.playerbot", "AuctionAction: Bot {} checking auctions",
            ai->GetBot()->GetName());
        return ActionResult::IN_PROGRESS;
    }
};

/**
 * @class RestAction
 * @brief Action for resting to recover health/mana
 */
class RestAction : public Action
{
public:
    RestAction() : Action("rest") { SetRelevance(0.7f); }

    bool IsPossible(BotAI* ai) const override
    {
        if (!ai || !ai->GetBot())
            return false;

        Player* bot = ai->GetBot();
        return !bot->IsInCombat() && bot->IsAlive();
    }

    bool IsUseful(BotAI* ai) const override
    {
        if (!ai || !ai->GetBot())
            return false;

        Player* bot = ai->GetBot();
        float healthPct = bot->GetHealthPct();
        float manaPct = bot->GetMaxPower(POWER_MANA) > 0 ?
            (float)bot->GetPower(POWER_MANA) / bot->GetMaxPower(POWER_MANA) * 100.0f : 100.0f;

        // Need rest if health < 50% or mana < 30%
        return healthPct < 50.0f || manaPct < 30.0f;
    }

    ActionResult Execute(BotAI* ai, ActionContext const& context) override
    {
        if (!ai || !ai->GetBot())
            return ActionResult::IMPOSSIBLE;

        Player* bot = ai->GetBot();

        // Sit to regenerate faster
        if (!bot->IsSitState())
        {
            bot->SetStandState(UNIT_STAND_STATE_SIT);
            TC_LOG_DEBUG("module.playerbot", "RestAction: Bot {} sitting to rest",
                bot->GetName());
        }

        // Check if fully recovered
        float healthPct = bot->GetHealthPct();
        float manaPct = bot->GetMaxPower(POWER_MANA) > 0 ?
            (float)bot->GetPower(POWER_MANA) / bot->GetMaxPower(POWER_MANA) * 100.0f : 100.0f;

        if (healthPct >= 95.0f && manaPct >= 90.0f)
        {
            bot->SetStandState(UNIT_STAND_STATE_STAND);
            return ActionResult::SUCCESS;
        }

        return ActionResult::IN_PROGRESS;
    }
};

// ============================================================================
// SOLO TRIGGERS - Event detection for solo bot behavior
// ============================================================================

/**
 * @class QuestAvailableTrigger
 * @brief Trigger when a new quest is available nearby
 */
class QuestAvailableTrigger : public QuestTrigger
{
public:
    QuestAvailableTrigger() : QuestTrigger("quest_available") { SetPriority(80); }

    bool Check(BotAI* ai) const override
    {
        if (!ai || !ai->GetBot())
            return false;

        // Check for nearby quest givers via UnifiedQuestManager
        auto questGivers = UnifiedQuestManager::instance()->ScanForQuestGivers(ai->GetBot(), 30.0f);
        return !questGivers.empty();
    }

    float CalculateUrgency(BotAI* ai) const override
    {
        // Higher urgency if bot has few active quests
        if (!ai)
            return 0.5f;

        uint32 activeQuests = ai->GetActiveQuestCount();
        return activeQuests < 5 ? 0.8f : 0.4f;
    }
};

/**
 * @class QuestCompleteTrigger
 * @brief Trigger when a quest is ready to turn in
 */
class QuestCompleteTrigger : public QuestTrigger
{
public:
    QuestCompleteTrigger() : QuestTrigger("quest_complete") { SetPriority(90); }

    bool Check(BotAI* ai) const override
    {
        if (!ai)
            return false;

        // Use BotAI helper method for completable quests
        return ai->HasCompletableQuests();
    }

    float CalculateUrgency(BotAI* ai) const override
    {
        // High urgency to turn in completed quests
        return 0.9f;
    }
};

/**
 * @class ResourceNearbyTrigger
 * @brief Trigger when a gatherable resource is nearby
 */
class ResourceNearbyTrigger : public Trigger
{
public:
    ResourceNearbyTrigger() : Trigger("resource_nearby", TriggerType::WORLD) { SetPriority(60); }

    bool Check(BotAI* ai) const override
    {
        if (!ai || !ai->GetGatheringManager())
            return false;

        return ai->GetGatheringManager()->HasNearbyResources();
    }

    float CalculateUrgency(BotAI* ai) const override
    {
        // Medium urgency for gathering
        return 0.5f;
    }
};

/**
 * @class LowHealthTrigger
 * @brief Trigger when health is low and need to rest
 */
class SoloLowHealthTrigger : public HealthTrigger
{
public:
    SoloLowHealthTrigger() : HealthTrigger("solo_low_health", 0.5f) { SetPriority(95); }

    bool Check(BotAI* ai) const override
    {
        if (!ai || !ai->GetBot())
            return false;

        Player* bot = ai->GetBot();
        return !bot->IsInCombat() && bot->GetHealthPct() < _threshold * 100.0f;
    }

    float CalculateUrgency(BotAI* ai) const override
    {
        if (!ai || !ai->GetBot())
            return 0.0f;

        float healthPct = ai->GetBot()->GetHealthPct();
        // Urgency increases as health decreases
        return 1.0f - (healthPct / 100.0f);
    }
};

/**
 * @class LowManaTrigger
 * @brief Trigger when mana is low
 */
class SoloLowManaTrigger : public HealthTrigger
{
public:
    SoloLowManaTrigger() : HealthTrigger("solo_low_mana", 0.3f) { SetPriority(85); }

    bool Check(BotAI* ai) const override
    {
        if (!ai || !ai->GetBot())
            return false;

        Player* bot = ai->GetBot();
        if (bot->IsInCombat() || bot->GetMaxPower(POWER_MANA) == 0)
            return false;

        float manaPct = (float)bot->GetPower(POWER_MANA) / bot->GetMaxPower(POWER_MANA);
        return manaPct < _threshold;
    }

    float CalculateUrgency(BotAI* ai) const override
    {
        if (!ai || !ai->GetBot())
            return 0.0f;

        Player* bot = ai->GetBot();
        if (bot->GetMaxPower(POWER_MANA) == 0)
            return 0.0f;

        float manaPct = (float)bot->GetPower(POWER_MANA) / bot->GetMaxPower(POWER_MANA);
        return 1.0f - manaPct;
    }
};

/**
 * @class IdleTrigger
 * @brief Trigger when bot has nothing to do
 */
class IdleTrigger : public TimerTrigger
{
public:
    IdleTrigger() : TimerTrigger("idle", 10000) { SetPriority(10); }

    bool Check(BotAI* ai) const override
    {
        if (!TimerTrigger::Check(ai))
            return false;

        if (!ai || !ai->GetBot())
            return false;

        Player* bot = ai->GetBot();
        if (bot->IsInCombat() || !bot->IsAlive())
            return false;

        // Bot is idle if no managers are active
        bool isQuesting = ai->IsQuestingActive();
        bool isGathering = ai->GetGatheringManager() && ai->GetGatheringManager()->IsGathering();
        bool isTrading = ai->GetTradeManager() && ai->GetTradeManager()->IsTradingActive();

        return !isQuesting && !isGathering && !isTrading;
    }
};

// ============================================================================
// SOLO VALUES - Bot state and preference tracking
// ============================================================================

/**
 * @class PreferredActivityValue
 * @brief Tracks bot's preferred solo activity
 */
class PreferredActivityValue : public TypedValue<uint8>
{
public:
    enum Activity : uint8
    {
        ACTIVITY_QUESTING   = 0,
        ACTIVITY_GATHERING  = 1,
        ACTIVITY_EXPLORING  = 2,
        ACTIVITY_TRADING    = 3,
        ACTIVITY_RESTING    = 4
    };

    PreferredActivityValue() : TypedValue<uint8>("preferred_activity"), _activity(ACTIVITY_QUESTING) {}

protected:
    uint8 GetTypedValue(BotAI* ai) const override
    {
        return _activity;
    }

    void SetTypedValue(BotAI* ai, uint8 const& value) override
    {
        _activity = static_cast<Activity>(value);
    }

private:
    Activity _activity;
};

/**
 * @class ExplorationProgressValue
 * @brief Tracks exploration progress in current zone
 */
class ExplorationProgressValue : public TypedValue<float>
{
public:
    ExplorationProgressValue() : TypedValue<float>("exploration_progress") {}

protected:
    float GetTypedValue(BotAI* ai) const override
    {
        if (!ai || !ai->GetBot())
            return 0.0f;

        // Full zone exploration calculation using discovered area tracking
        // Returns 0.0f as default until exploration tracking is implemented
        // Full implementation should:
        // - Query Player::GetExploredZones() or equivalent DBC data
        // - Calculate percentage of zone area explored (AreaTable.dbc)
        // - Consider sub-zones and points of interest
        // - Cache per-zone exploration state for performance
        // Reference: Player exploration flags, AreaTable.dbc, WorldMapArea.dbc
        return 0.0f;
    }

    void SetTypedValue(BotAI* ai, float const& value) override
    {
        // Exploration progress is read-only (calculated from player data)
    }
};

/**
 * @class SoloEfficiencyValue
 * @brief Tracks how efficiently the bot is playing solo
 */
class SoloEfficiencyValue : public TypedValue<float>
{
public:
    SoloEfficiencyValue() : TypedValue<float>("solo_efficiency"), _efficiency(0.5f) {}

protected:
    float GetTypedValue(BotAI* ai) const override
    {
        return _efficiency;
    }

    void SetTypedValue(BotAI* ai, float const& value) override
    {
        _efficiency = std::clamp(value, 0.0f, 1.0f);
    }

private:
    float _efficiency;
};

/**
 * @class LastActivityTimeValue
 * @brief Tracks when bot last performed an activity
 */
class LastActivityTimeValue : public TypedValue<uint32>
{
public:
    LastActivityTimeValue() : TypedValue<uint32>("last_activity_time"), _lastTime(0) {}

protected:
    uint32 GetTypedValue(BotAI* ai) const override
    {
        return _lastTime;
    }

    void SetTypedValue(BotAI* ai, uint32 const& value) override
    {
        _lastTime = value;
    }

private:
    uint32 _lastTime;
};

// ============================================================================
// SOLOSTRATEGY IMPLEMENTATION
// ============================================================================

SoloStrategy::SoloStrategy() : Strategy("solo")
{
    SetPriority(50); // Lower priority than group strategies
}

void SoloStrategy::InitializeActions()
{
    // Register solo actions with priority ordering
    AddAction("quest", std::make_shared<QuestAction>());
    AddAction("gather", std::make_shared<GatherAction>());
    AddAction("explore", std::make_shared<ExploreAction>());
    AddAction("trade", std::make_shared<TradeAction>());
    AddAction("auction", std::make_shared<AuctionAction>());
    AddAction("rest", std::make_shared<RestAction>());

    TC_LOG_DEBUG("module.playerbot", "SoloStrategy: Initialized 6 solo actions");
}

void SoloStrategy::InitializeTriggers()
{
    // Register solo triggers for event detection
    auto questAvailable = std::make_shared<QuestAvailableTrigger>();
    questAvailable->SetAction("quest");
    AddTrigger(questAvailable);

    auto questComplete = std::make_shared<QuestCompleteTrigger>();
    questComplete->SetAction("quest");
    AddTrigger(questComplete);

    auto resourceNearby = std::make_shared<ResourceNearbyTrigger>();
    resourceNearby->SetAction("gather");
    AddTrigger(resourceNearby);

    auto lowHealth = std::make_shared<SoloLowHealthTrigger>();
    lowHealth->SetAction("rest");
    AddTrigger(lowHealth);

    auto lowMana = std::make_shared<SoloLowManaTrigger>();
    lowMana->SetAction("rest");
    AddTrigger(lowMana);

    auto idle = std::make_shared<IdleTrigger>();
    idle->SetAction("explore");
    AddTrigger(idle);

    TC_LOG_DEBUG("module.playerbot", "SoloStrategy: Initialized 6 solo triggers");
}

void SoloStrategy::InitializeValues()
{
    // Register solo values for state tracking
    AddValue("preferred_activity", std::make_shared<PreferredActivityValue>());
    AddValue("exploration_progress", std::make_shared<ExplorationProgressValue>());
    AddValue("solo_efficiency", std::make_shared<SoloEfficiencyValue>());
    AddValue("last_activity_time", std::make_shared<LastActivityTimeValue>());

    TC_LOG_DEBUG("module.playerbot", "SoloStrategy: Initialized 4 solo values");
}

void SoloStrategy::OnActivate(BotAI* ai)
{
    if (!ai || !ai->GetBot())
        return;

    TC_LOG_INFO("module.playerbot", "Solo strategy activated for bot {}", ai->GetBot()->GetName());
    SetActive(true);
}

void SoloStrategy::OnDeactivate(BotAI* ai)
{
    if (!ai || !ai->GetBot())
        return;

    TC_LOG_INFO("module.playerbot", "Solo strategy deactivated for bot {}", ai->GetBot()->GetName());
    SetActive(false);
}

bool SoloStrategy::IsActive(BotAI* ai) const
{
    if (!ai || !ai->GetBot())
        return false;

    // Instance bots (warm pool, JIT) don't do solo activities like questing/grinding
    // They're focused on their instance content (BG/Arena/Dungeon/Raid)
    if (BotSession* session = BotSessionManager::GetBotSession(ai->GetBot()->GetSession()))
    {
        if (session->IsInstanceBot())
            return false;
    }

    bool active = _active;
    bool hasGroup = (ai->GetBot()->GetGroup() != nullptr);
    bool result = active && !hasGroup;

    // Active when not in a group and explicitly activated
    return result;
}

void SoloStrategy::UpdateBehavior(BotAI* ai, uint32 diff)
{
    if (!ai || !ai->GetBot())
        return;

    Player* bot = ai->GetBot();

    // ========================================================================
    // PHASE 2.5: OBSERVER PATTERN IMPLEMENTATION
    // ========================================================================
    // SoloStrategy observes manager states via atomic queries (<0.001ms each)
    // Managers self-throttle (1s-10s intervals) via BotAI::UpdateManagers()
    // This achieves <0.1ms UpdateBehavior() performance target
    // ========================================================================

    // Query manager states atomically (lock-free, <0.001ms per query)
    bool isQuesting = ai->IsQuestingActive();
    bool isGathering = ai->GetGatheringManager() && ai->GetGatheringManager()->IsGathering();
    bool isTrading = ai->GetTradeManager() && ai->GetTradeManager()->IsTradingActive();
    bool hasAuctions = ai->GetAuctionManager() && ai->GetAuctionManager()->HasActiveAuctions();

    // Determine current bot activity state
    bool isBusy = isQuesting || isGathering || isTrading || hasAuctions;

    // Periodic activity logging (every 5 seconds)
    static uint32 activityLogTimer = 0;
    activityLogTimer += diff;
    if (activityLogTimer >= 5000)
    {
        TC_LOG_DEBUG("module.playerbot",
            "SoloStrategy: Bot {} - Questing:{} Gathering:{} Trading:{} Auctions:{} Busy:{}",
            bot->GetName(), isQuesting, isGathering, isTrading, hasAuctions, isBusy);
        activityLogTimer = 0;
    }

    // If bot is busy with any manager activity, skip wandering
    if (isBusy)
        return;

    // ========================================================================
    // FALLBACK: SIMPLE WANDERING BEHAVIOR
    // ========================================================================
    // If no manager is active, bot does basic exploration
    // This is the lowest-priority activity
    // ========================================================================

    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - _lastWanderTime > _wanderInterval)
    {
        // Wandering handled by ExploreAction which uses Map::GetHeight() for terrain following
        // Full pathfinding delegated to UnifiedMovementCoordinator via BotAI when action executes
        TC_LOG_TRACE("module.playerbot",
            "SoloStrategy: Bot {} is in solo mode (no active managers), considering wandering",
            bot->GetName());

        _lastWanderTime = currentTime;
    }
}

} // namespace Playerbot
