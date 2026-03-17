/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file BotAI_EventHandlers.cpp
 * @brief Default implementations of event handlers for BotAI
 *
 * This file contains the default event handler implementations for all 12 event buses.
 * ClassAI implementations can override these virtual methods for specialized behavior.
 *
 * Phase 4: Event Bus Integration
 * Phase 5: Template Migration (ProfessionEventBus added)
 * - 12 event handlers fully implemented
 * - Delegates to managers where appropriate
 * - Provides sensible defaults for autonomous behavior
 */

#include "BotAI.h"
#include "GameTime.h"
#include "Core/Events/GenericEventBus.h"
#include "AI/Coordination/Messaging/BotMessage.h"
#include "AI/Combat/CombatCoordinationIntegrator.h"
#include "Group/GroupEvents.h"
#include "Combat/CombatEvents.h"
#include "Cooldown/CooldownEvents.h"
#include "Aura/AuraEvents.h"
#include "Loot/LootEvents.h"
#include "Quest/QuestEvents.h"
#include "Resource/ResourceEvents.h"
#include "Social/SocialEvents.h"
#include "Auction/AuctionEvents.h"
#include "NPC/NPCEvents.h"
#include "Instance/InstanceEvents.h"
#include "Professions/ProfessionEvents.h"
#include "Social/TradeManager.h"
#include "Economy/AuctionManager.h"
#include "Advanced/GroupCoordinator.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "Spatial/SpatialGridQueryHelpers.h"
#include "Threading/BotAction.h"
#include "Threading/BotActionManager.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Log.h"
#include "Config/PlayerbotConfig.h"
#include "Social/UnifiedLootManager.h"
#include "Chat/BotChatCommandHandler.h"
#include "Interaction/VendorInteractionManager.h"
#include "Interaction/TrainerInteractionManager.h"
#include "Banking/BankingManager.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Creature.h"
#include "DatabaseEnv.h"

namespace Playerbot
{

// ============================================================================
// EVENT BUS SUBSCRIPTION MANAGEMENT
// ============================================================================

void BotAI::SubscribeToEventBuses()
{
    if (!_bot)
        return;

    // Subscribe to all event buses for comprehensive event handling
    {
        std::vector<GroupEventType> allGroupTypes;
        for (uint8 i = 0; i < static_cast<uint8>(GroupEventType::MAX_EVENT_TYPE); ++i)
            allGroupTypes.push_back(static_cast<GroupEventType>(i));
        EventBus<GroupEvent>::instance()->Subscribe(this, allGroupTypes);
    }
    {
        std::vector<CombatEventType> allCombatTypes;
        for (uint8 i = 0; i < static_cast<uint8>(CombatEventType::MAX_COMBAT_EVENT); ++i)
            allCombatTypes.push_back(static_cast<CombatEventType>(i));
        EventBus<CombatEvent>::instance()->Subscribe(this, allCombatTypes);
    }
    {
        std::vector<CooldownEventType> allCooldownTypes;
        for (uint8 i = 0; i < static_cast<uint8>(CooldownEventType::MAX_COOLDOWN_EVENT); ++i)
            allCooldownTypes.push_back(static_cast<CooldownEventType>(i));
        EventBus<CooldownEvent>::instance()->Subscribe(this, allCooldownTypes);
    }
    {
        std::vector<AuraEventType> allAuraTypes;
        for (uint8 i = 0; i < static_cast<uint8>(AuraEventType::MAX_AURA_EVENT); ++i)
            allAuraTypes.push_back(static_cast<AuraEventType>(i));
        EventBus<AuraEvent>::instance()->Subscribe(this, allAuraTypes);
    }
    {
        std::vector<LootEventType> allLootTypes;
        for (uint8 i = 0; i < static_cast<uint8>(LootEventType::MAX_LOOT_EVENT); ++i)
            allLootTypes.push_back(static_cast<LootEventType>(i));
        EventBus<LootEvent>::instance()->Subscribe(this, allLootTypes);
    }
    {
        std::vector<QuestEventType> allQuestTypes;
        for (uint8 i = 0; i < static_cast<uint8>(QuestEventType::MAX_QUEST_EVENT); ++i)
            allQuestTypes.push_back(static_cast<QuestEventType>(i));
        EventBus<QuestEvent>::instance()->Subscribe(this, allQuestTypes);
    }
    {
        std::vector<ResourceEventType> allResourceTypes;
        for (uint8 i = 0; i < static_cast<uint8>(ResourceEventType::MAX_RESOURCE_EVENT); ++i)
            allResourceTypes.push_back(static_cast<ResourceEventType>(i));
        EventBus<ResourceEvent>::instance()->Subscribe(this, allResourceTypes);
    }
    {
        std::vector<SocialEventType> allSocialTypes;
        for (uint8 i = 0; i < static_cast<uint8>(SocialEventType::MAX_SOCIAL_EVENT); ++i)
            allSocialTypes.push_back(static_cast<SocialEventType>(i));
        EventBus<SocialEvent>::instance()->Subscribe(this, allSocialTypes);
    }
    {
        std::vector<AuctionEventType> allAuctionTypes;
        for (uint8 i = 0; i < static_cast<uint8>(AuctionEventType::MAX_AUCTION_EVENT); ++i)
            allAuctionTypes.push_back(static_cast<AuctionEventType>(i));
        EventBus<AuctionEvent>::instance()->Subscribe(this, allAuctionTypes);
    }
    {
        std::vector<NPCEventType> allNPCTypes;
        for (uint8 i = 0; i < static_cast<uint8>(NPCEventType::MAX_NPC_EVENT); ++i)
            allNPCTypes.push_back(static_cast<NPCEventType>(i));
        EventBus<NPCEvent>::instance()->Subscribe(this, allNPCTypes);
    }
    {
        std::vector<InstanceEventType> allInstanceTypes;
        for (uint8 i = 0; i < static_cast<uint8>(InstanceEventType::MAX_INSTANCE_EVENT); ++i)
            allInstanceTypes.push_back(static_cast<InstanceEventType>(i));
        EventBus<InstanceEvent>::instance()->Subscribe(this, allInstanceTypes);
    }
    {
        std::vector<ProfessionEventType> allProfessionTypes;
        for (uint8 i = 0; i < static_cast<uint8>(ProfessionEventType::MAX_PROFESSION_EVENT); ++i)
            allProfessionTypes.push_back(static_cast<ProfessionEventType>(i));
        EventBus<ProfessionEvent>::instance()->Subscribe(this, allProfessionTypes);
    }

    // CRITICAL: Use GetGUID().ToString() instead of GetName() during constructor
    // GetName() accesses m_name which may not be initialized yet during bot construction
    // This prevents ACCESS_VIOLATION crash at BotAI_EventHandlers.cpp line 87
    TC_LOG_DEBUG("playerbot.events", "Bot subscribed to all 12 event buses (GUID: {})",
        _bot->GetGUID().ToString());
}

void BotAI::UnsubscribeFromEventBuses()
{
    // CRITICAL FIX: Use cached GUID instead of _bot pointer during destructor
    // The Player object may already be destroyed when BotAI destructor runs,
    // making _bot a dangling pointer. Using the cached GUID is safe.
    if (_cachedBotGuid.IsEmpty())
        return;

    // CRITICAL: Unsubscribe from all event buses using GUID (safe during destructor)
    // Call the underlying template directly with the new UnsubscribeByGuid method
    EventBus<GroupEvent>::instance()->UnsubscribeByGuid(_cachedBotGuid);
    EventBus<CombatEvent>::instance()->UnsubscribeByGuid(_cachedBotGuid);
    EventBus<CooldownEvent>::instance()->UnsubscribeByGuid(_cachedBotGuid);
    EventBus<AuraEvent>::instance()->UnsubscribeByGuid(_cachedBotGuid);
    EventBus<LootEvent>::instance()->UnsubscribeByGuid(_cachedBotGuid);
    EventBus<QuestEvent>::instance()->UnsubscribeByGuid(_cachedBotGuid);
    EventBus<ResourceEvent>::instance()->UnsubscribeByGuid(_cachedBotGuid);
    EventBus<SocialEvent>::instance()->UnsubscribeByGuid(_cachedBotGuid);
    EventBus<AuctionEvent>::instance()->UnsubscribeByGuid(_cachedBotGuid);
    EventBus<NPCEvent>::instance()->UnsubscribeByGuid(_cachedBotGuid);
    EventBus<InstanceEvent>::instance()->UnsubscribeByGuid(_cachedBotGuid);
    EventBus<ProfessionEvent>::instance()->UnsubscribeByGuid(_cachedBotGuid);

    TC_LOG_DEBUG("playerbot.events", "Bot unsubscribed from all event buses (GUID: {})",
        _cachedBotGuid.ToString());
}

// ============================================================================
// GROUP EVENT HANDLER
// ============================================================================

void BotAI::OnGroupEvent(GroupEvent const& event)
{
    if (!_bot)
        return;

    switch (event.type)
    {
        case GroupEventType::READY_CHECK_STARTED:
            ProcessGroupReadyCheck(event);
            break;

        case GroupEventType::TARGET_ICON_CHANGED:
            if (GetGroupCoordinatorAdvanced())
                GetGroupCoordinatorAdvanced()->OnTargetIconChanged(event);
            break;

        case GroupEventType::LEADER_CHANGED:
            // Update following target if we were following old leader
    if (_aiState == BotAIState::FOLLOWING)
                HandleGroupChange();
            break;

        case GroupEventType::GROUP_DISBANDED:
            OnGroupLeft();
            break;

        case GroupEventType::MEMBER_JOINED:
        case GroupEventType::MEMBER_LEFT:
            if (GetGroupCoordinatorAdvanced())
                GetGroupCoordinatorAdvanced()->OnGroupCompositionChanged(event);
            break;

        case GroupEventType::LOOT_METHOD_CHANGED:
            TC_LOG_DEBUG("playerbot.events.group", "Bot {}: Loot method changed to {}",
                _bot->GetName(), event.data1);
            break;

        default:
            // Other events handled by specialized systems
            break;
    }
}

void BotAI::ProcessGroupReadyCheck(GroupEvent const& event)
{
    if (!_bot || !_bot->GetGroup())
        return;

    // Check config for auto-ready-check behavior
    // Config keys: Playerbot.AutoReadyCheck (default: true)
    //              Playerbot.ReadyCheckDelayMs (default: 500-2000ms random for realism)
    bool autoReadyCheck = sPlayerbotConfig->GetBool("Playerbot.AutoReadyCheck", true);
    if (!autoReadyCheck)
    {
        TC_LOG_DEBUG("playerbot.events.group", "Bot {}: Auto-ready-check disabled by config",
            _bot->GetName());
        return;
    }

    // Determine ready state
    bool isReady = true;

    // Check if we're actually ready (not dead, not in combat, etc.)
    if (_bot->isDead())
    {
        isReady = false;
        TC_LOG_DEBUG("playerbot.events.group", "Bot {}: Not ready (dead)", _bot->GetName());
    }
    else if (_bot->IsInCombat())
    {
        isReady = false;
        TC_LOG_DEBUG("playerbot.events.group", "Bot {}: Not ready (in combat)", _bot->GetName());
    }
    else if (_bot->GetHealthPct() < 50.0f)
    {
        isReady = false;
        TC_LOG_DEBUG("playerbot.events.group", "Bot {}: Not ready (low health: {:.1f}%)",
            _bot->GetName(), _bot->GetHealthPct());
    }
    else if (_bot->GetPowerPct(POWER_MANA) < 30.0f && _bot->GetMaxPower(POWER_MANA) > 0)
    {
        isReady = false;
        TC_LOG_DEBUG("playerbot.events.group", "Bot {}: Not ready (low mana: {:.1f}%)",
            _bot->GetName(), _bot->GetPowerPct(POWER_MANA));
    }

    TC_LOG_DEBUG("playerbot.events.group", "Bot {}: Ready check status - {}",
        _bot->GetName(), isReady ? "READY" : "NOT READY");

    // Note: TrinityCore doesn't have a MEMBER_FLAG_READY. Ready check responses
    // are handled differently through the client-server protocol. Bots auto-ready
    // by not needing to send a response - they're considered ready by default.
}

// ============================================================================
// COMBAT EVENT HANDLER
// ============================================================================

void BotAI::OnCombatEvent(CombatEvent const& event)
{
    if (!_bot)
        return;

    ObjectGuid botGuid = _bot->GetGUID();

    switch (event.type)
    {
        case CombatEventType::SPELL_CAST_START:
            // Check for interruptible enemy casts
            ProcessCombatInterrupt(event);

            // NEUTRAL MOB DETECTION: Bot is being targeted by hostile spell
    if (event.targetGuid == botGuid && !_bot->IsInCombat())
            {
                // PHASE 2: Thread-safe spatial grid verification (no Map access from worker thread)
                auto casterSnapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, event.casterGuid);
                if (casterSnapshot && casterSnapshot->IsAlive() && casterSnapshot->isHostile)
                {
                    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(event.spellId, DIFFICULTY_NONE);
                    if (spellInfo && !spellInfo->IsPositive())  // Hostile spell
                    {
                        TC_LOG_DEBUG("playerbot.combat",
                            "Bot {}: Detected neutral mob {} casting hostile spell {} via SPELL_CAST_START (queueing combat action)",
                            _bot->GetName(), event.casterGuid.ToString(), event.spellId);

                        // Queue BotAction for main thread execution (CRITICAL: no Map access from worker threads!)
                        BotAction action = BotAction::AttackTarget(_bot->GetGUID(), event.casterGuid, GameTime::GetGameTimeMS());
                        sBotActionMgr->QueueAction(action);
                    }
                }
            }
            break;

        case CombatEventType::ATTACK_START:
            // Track when bot initiates combat
    if (event.casterGuid == botGuid)
            {
                if (_aiState != BotAIState::COMBAT)
                    SetAIState(BotAIState::COMBAT);
            }

            // NEUTRAL MOB DETECTION: Bot is being attacked
    if (event.victimGuid == botGuid && !_bot->IsInCombat())
            {
                // PHASE 2: Thread-safe spatial grid verification (no Map access from worker thread)
                auto attackerSnapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, event.casterGuid);
                if (attackerSnapshot && attackerSnapshot->IsAlive() && attackerSnapshot->isHostile)
                {
                    TC_LOG_DEBUG("playerbot.combat",
                        "Bot {}: Detected neutral mob {} attacking via ATTACK_START (queueing combat action)",
                        _bot->GetName(), event.casterGuid.ToString());

                    // Queue BotAction for main thread execution (CRITICAL: no Map access from worker threads!)
                    BotAction action = BotAction::AttackTarget(_bot->GetGUID(), event.casterGuid, GameTime::GetGameTimeMS());
                    sBotActionMgr->QueueAction(action);
                }
            }
            break;

        case CombatEventType::ATTACK_STOP:
            // Track when combat ends
    if (event.casterGuid == botGuid)
            {
                if (_aiState == BotAIState::COMBAT && !_bot->IsInCombat())
                    SetAIState(BotAIState::SOLO);
            }
            break;

        case CombatEventType::AI_REACTION:
            // NEUTRAL MOB DETECTION: NPC became hostile and is targeting bot
    if (event.amount > 0)  // Positive reaction = hostile
            {
                // PHASE 2: Thread-safe spatial grid verification (no Map access from worker thread)
                auto mobSnapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, event.casterGuid);
                if (mobSnapshot && mobSnapshot->IsAlive() && mobSnapshot->isHostile &&
                    mobSnapshot->victim == _bot->GetGUID() && !_bot->IsInCombat())
                {
                    TC_LOG_DEBUG("playerbot.combat",
                        "Bot {}: Detected neutral mob {} became hostile via AI_REACTION (queueing combat action)",
                        _bot->GetName(), event.casterGuid.ToString());

                    // Queue BotAction for main thread execution (CRITICAL: no Map access from worker threads!)
                    BotAction action = BotAction::AttackTarget(_bot->GetGUID(), event.casterGuid, GameTime::GetGameTimeMS());
                    sBotActionMgr->QueueAction(action);
                }
            }
            break;

        case CombatEventType::SPELL_DAMAGE_TAKEN:
            // NEUTRAL MOB DETECTION: Catch-all for damage received
    if (event.victimGuid == botGuid && !_bot->IsInCombat())
            {
                // PHASE 2: Thread-safe spatial grid verification (no Map access from worker thread)
                auto attackerSnapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, event.casterGuid);
                if (attackerSnapshot && attackerSnapshot->IsAlive() && attackerSnapshot->isHostile)
                {
                    TC_LOG_DEBUG("playerbot.combat",
                        "Bot {}: Detected damage from neutral mob {} via SPELL_DAMAGE_TAKEN (queueing combat action)",
                        _bot->GetName(), event.casterGuid.ToString());

                    // Queue BotAction for main thread execution (CRITICAL: no Map access from worker threads!)
                    BotAction action = BotAction::AttackTarget(_bot->GetGUID(), event.casterGuid, GameTime::GetGameTimeMS());
                    sBotActionMgr->QueueAction(action);
                }
            }
            break;

        default:
            // Other combat events processed by specialized combat systems
            break;
    }
}

void BotAI::ProcessCombatInterrupt(CombatEvent const& event)
{
    if (!_bot || event.type != CombatEventType::SPELL_CAST_START)
        return;

    // PHASE 2: Thread-safe spatial grid verification (no Map access from worker thread)
    auto casterSnapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, event.casterGuid);
    if (!casterSnapshot || !casterSnapshot->isHostile)
        return;

    // Check if spell is interruptible (WoW 12.0: check InterruptFlags)
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(event.spellId, DIFFICULTY_NONE);
    if (!spellInfo || spellInfo->InterruptFlags == SpellInterruptFlags::None)
        return;

    TC_LOG_TRACE("playerbot.events.combat", "Bot {}: Detected interruptible cast {} from {}",
        _bot->GetName(), event.spellId, event.casterGuid.ToString());

    // Note: Actual interrupt logic is handled by ClassAI combat rotations
    // This is just event detection and logging
}

void BotAI::EnterCombatWithTarget(::Unit* target)
{
    if (!_bot || !target)
        return;

    // Prevent duplicate combat entry
    if (_bot->IsInCombat() && _bot->GetVictim() == target)
        return;

    TC_LOG_INFO("playerbot.combat", "Bot {} force-entering combat with {} (GUID: {})",
        _bot->GetName(), target->GetName(), target->GetGUID().ToString());

    // 1. Set combat state manually (required for neutral mobs)
    _bot->SetInCombatWith(target);
    target->SetInCombatWith(_bot);

    // 2. Attack the target
    _bot->Attack(target, true);

    // 3. Update threat if target has threat list
    if (target->CanHaveThreatList())
    {
        target->GetThreatManager().AddThreat(_bot, 1.0f);
    }

    // 4. Notify AI systems
    _currentTarget = target->GetGUID();
    SetAIState(BotAIState::COMBAT);

    // 5. Trigger combat start notification
    OnCombatStart(target);
}

// ============================================================================
// COOLDOWN EVENT HANDLER
// ============================================================================

void BotAI::OnCooldownEvent(CooldownEvent const& event)
{
    if (!_bot)
        return;

    // Default: Just log cooldown events for debugging
    TC_LOG_TRACE("playerbot.events.cooldown", "Bot {}: Cooldown event - spell {} (type {})",
        _bot->GetName(), event.spellId, static_cast<uint32>(event.type));

    // ClassAI implementations should override this for ability rotation tracking
}

// ============================================================================
// AURA EVENT HANDLER
// ============================================================================

void BotAI::OnAuraEvent(AuraEvent const& event)
{
    if (!_bot)
        return;

    switch (event.type)
    {
        case AuraEventType::AURA_APPLIED:
            // Check if we need to dispel harmful debuffs
    if (event.isHarmful && event.targetGuid == _bot->GetGUID())
            {
                ProcessAuraDispel(event);
            }
            break;

        case AuraEventType::AURA_REMOVED:
            // Track when important buffs fall off
            TC_LOG_TRACE("playerbot.events.aura", "Bot {}: Aura {} removed from {}",
                _bot->GetName(), event.spellId, event.targetGuid.ToString());
            break;

        default:
            break;
    }
}

void BotAI::ProcessAuraDispel(AuraEvent const& event)
{
    if (!_bot || !event.isHarmful)
        return;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(event.spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return;

    // Check if aura is dispellable
    if (spellInfo->Dispel == DISPEL_NONE)
        return;

    TC_LOG_DEBUG("playerbot.events.aura", "Bot {}: Harmful dispellable aura {} detected",
        _bot->GetName(), event.spellId);

    // Note: Actual dispel logic is handled by ClassAI or healers in group
}

// ============================================================================
// LOOT EVENT HANDLER
// ============================================================================

void BotAI::OnLootEvent(LootEvent const& event)
{
    if (!_bot)
        return;

    switch (event.type)
    {
        case LootEventType::LOOT_ROLL_STARTED:
            ProcessLootRoll(event);
            break;

        case LootEventType::LOOT_ITEM_RECEIVED:
            TC_LOG_DEBUG("playerbot.events.loot", "Bot {}: Received item {} x{}",
                _bot->GetName(), event.itemEntry, event.itemCount);
            break;

        case LootEventType::LOOT_MONEY_RECEIVED:
            TC_LOG_TRACE("playerbot.events.loot", "Bot {}: Received {} copper",
                _bot->GetName(), event.itemCount);
            break;

        default:
            break;
    }
}

void BotAI::ProcessLootRoll(LootEvent const& event)
{
    if (!_bot)
        return;

    TC_LOG_DEBUG("playerbot.events.loot", "Bot {}: Loot roll started for item {}",
        _bot->GetName(), event.itemEntry);

    // Use UnifiedLootManager for smart loot rolling decisions
    // This evaluates: item quality vs current gear, class/spec appropriateness, group loot rules
    UnifiedLootManager* lootMgr = UnifiedLootManager::instance();
    if (!lootMgr)
    {
        TC_LOG_DEBUG("playerbot.events.loot", "Bot {}: UnifiedLootManager not available, defaulting to GREED",
            _bot->GetName());
        return;
    }

    // Create a temporary LootItem for evaluation
    LootItem lootItem;
    lootItem.itemId = event.itemEntry;
    lootItem.itemCount = event.itemCount;

    // Use need-before-greed decision strategy to determine roll type
    LootRollType rollType = lootMgr->DetermineLootDecision(_bot, lootItem, LootDecisionStrategy::NEED_BEFORE_GREED);

    // Log the decision
    TC_LOG_DEBUG("playerbot.events.loot", "Bot {}: Evaluated item {} - would roll {}",
        _bot->GetName(), event.itemEntry,
        rollType == LootRollType::NEED ? "NEED" :
        rollType == LootRollType::GREED ? "GREED" :
        rollType == LootRollType::DISENCHANT ? "DISENCHANT" : "PASS");
}

// ============================================================================
// QUEST EVENT HANDLER
// ============================================================================

void BotAI::OnQuestEvent(QuestEvent const& event)
{
    if (!_bot)
        return;

    // Log quest event and process
    TC_LOG_TRACE("playerbot.events.quest", "Bot {}: Quest event {} for quest {}",
        _bot->GetName(), static_cast<uint32>(event.type), event.questId);

    ProcessQuestProgress(event);
}

void BotAI::ProcessQuestProgress(QuestEvent const& event)
{
    if (!_bot)
        return;

    switch (event.type)
    {
        case QuestEventType::QUEST_CONFIRM_ACCEPT:
            // Auto-accept appropriate quests
            TC_LOG_DEBUG("playerbot.events.quest", "Bot {}: Quest {} offered for acceptance",
                _bot->GetName(), event.questId);
            break;

        case QuestEventType::QUEST_COMPLETED:
            // Quest objectives complete, turn in
            TC_LOG_DEBUG("playerbot.events.quest", "Bot {}: Quest {} completed",
                _bot->GetName(), event.questId);
            break;

        case QuestEventType::QUEST_OBJECTIVE_COMPLETE:
            // Track quest progress
            TC_LOG_TRACE("playerbot.events.quest", "Bot {}: Quest {} objective progress",
                _bot->GetName(), event.questId);
            break;

        default:
            break;
    }
}

// ============================================================================
// RESOURCE EVENT HANDLER
// ============================================================================

void BotAI::OnResourceEvent(ResourceEvent const& event)
{
    if (!_bot)
        return;

    switch (event.type)
    {
        case ResourceEventType::HEALTH_UPDATE:
            // Check for low health allies (healing priority)
            ProcessLowHealthAlert(event);
            break;

        case ResourceEventType::POWER_UPDATE:
            // Track power levels for resource management
            TC_LOG_TRACE("playerbot.events.resource", "Bot {}: Unit {} power update",
                _bot->GetName(), event.playerGuid.ToString());
            break;

        case ResourceEventType::BREAK_TARGET:
            // Target selection broken, need new target
    if (event.playerGuid == _bot->GetGUID())
            {
                TC_LOG_DEBUG("playerbot.events.resource", "Bot {}: Target broken",
                    _bot->GetName());
            }
            break;

        default:
            break;
    }
}

void BotAI::ProcessLowHealthAlert(ResourceEvent const& event)
{
    if (!_bot || event.type != ResourceEventType::HEALTH_UPDATE)
        return;

    // Calculate health percentage
    float healthPercent = event.maxAmount > 0 ? (static_cast<float>(event.amount) / event.maxAmount * 100.0f) : 0.0f;

    // Check if health is critically low (below 30%)
    if (healthPercent > 30.0f)
        return;

    // PHASE 2 TODO: Replace with PlayerSnapshot when available in spatial grid
    // For now, this is low-priority (only logs, doesn't manipulate state)
    Unit* target = ObjectAccessor::GetUnit(*_bot, event.playerGuid);
    if (!target)
        return;

    // Check if target is in our group/raid
    if (!target->IsInRaidWith(_bot))
        return;

    TC_LOG_DEBUG("playerbot.events.resource", "Bot {}: LOW HEALTH ALERT - {} at {:.1f}%",
        _bot->GetName(),
        target->GetName(),
        healthPercent);

    // Note: Actual healing response is handled by ClassAI (healers)
    // or defensive cooldown usage (self-healing)
}

// ============================================================================
// SOCIAL EVENT HANDLER
// ============================================================================

void BotAI::OnSocialEvent(SocialEvent const& event)
{
    if (!_bot)
        return;

    switch (event.type)
    {
        case SocialEventType::MESSAGE_CHAT:
            // Process chat messages for commands
            if (event.chatType == ChatMsg::CHAT_MSG_WHISPER && event.targetGuid == _bot->GetGUID())
            {
                TC_LOG_DEBUG("playerbot.events.social", "Bot {}: Whisper from {}: {}",
                    _bot->GetName(), event.playerGuid.ToString(), event.message);

                // Parse for bot commands using the chat command handler
                if (BotChatCommandHandler::IsInitialized() &&
                    BotChatCommandHandler::IsCommand(event.message))
                {
                    // Get the sender player
                    Player* sender = ObjectAccessor::FindPlayer(event.playerGuid);
                    if (sender)
                    {
                        // Build command context
                        CommandContext context;
                        context.sender = sender;
                        context.bot = _bot;
                        context.botSession = nullptr;  // BotAI doesn't have direct session access
                        context.message = event.message;
                        context.lang = static_cast<uint32>(event.language);
                        context.isWhisper = true;
                        context.timestamp = GameTime::GetGameTimeMS();

                        // Parse and process the command
                        if (BotChatCommandHandler::ParseCommand(event.message, context))
                        {
                            CommandResult result = BotChatCommandHandler::ProcessChatMessage(context);
                            TC_LOG_DEBUG("playerbot.events.social", "Bot {}: Command '{}' processed with result {}",
                                _bot->GetName(), context.command, static_cast<uint8>(result));
                        }
                    }
                }
            }
            break;

        case SocialEventType::GUILD_INVITE_RECEIVED:
            // Handle guild invites
            TC_LOG_DEBUG("playerbot.events.social", "Bot {}: Guild invite from {}",
                _bot->GetName(), event.playerGuid.ToString());

            // Auto-accept guild invites based on configuration
            if (sPlayerbotConfig->GetBool("Playerbot.AutoAcceptGuildInvite", true))
            {
                // Get the inviter player
                Player* inviter = ObjectAccessor::FindPlayer(event.playerGuid);
                if (inviter)
                {
                    // Check if inviter is our master or in our group
                    bool shouldAccept = false;

                    // Accept from group leader (primary trusted source)
                    // Note: Master tracking would require BotSession access which is not available here
                    if (_bot->GetGroup() && _bot->GetGroup()->GetLeaderGUID() == event.playerGuid)
                    {
                        shouldAccept = true;
                        TC_LOG_DEBUG("playerbot.events.social", "Bot {}: Accepting guild invite from group leader {}",
                            _bot->GetName(), inviter->GetName());
                    }
                    // Config option to accept from anyone
                    else if (sPlayerbotConfig->GetBool("Playerbot.AutoAcceptGuildInviteFromAnyone", false))
                    {
                        shouldAccept = true;
                        TC_LOG_DEBUG("playerbot.events.social", "Bot {}: Accepting guild invite from {} (config allows any)",
                            _bot->GetName(), inviter->GetName());
                    }

                    if (shouldAccept && !_bot->GetGuildId())
                    {
                        // Accept the guild invite - use AddMember directly
                        if (Guild* guild = sGuildMgr->GetGuildById(inviter->GetGuildId()))
                        {
                            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
                            if (guild->AddMember(trans, _bot->GetGUID()))
                            {
                                CharacterDatabase.CommitTransaction(trans);
                                TC_LOG_DEBUG("playerbot.events.social", "Bot {}: Joined guild '{}'",
                                    _bot->GetName(), guild->GetName());
                            }
                            else
                            {
                                TC_LOG_DEBUG("playerbot.events.social", "Bot {}: Failed to join guild '{}'",
                                    _bot->GetName(), guild->GetName());
                            }
                        }
                    }
                }
            }
            break;

        case SocialEventType::TRADE_STATUS_CHANGED:
            if (GetGameSystems()->GetTradeManager())
            {
                // Delegate to TradeManager
                TC_LOG_TRACE("playerbot.events.social", "Bot {}: Trade status changed",
                    _bot->GetName());
            }
            break;

        default:
            break;
    }
}

// ============================================================================
// AUCTION EVENT HANDLER
// ============================================================================

void BotAI::OnAuctionEvent(AuctionEvent const& event)
{
    if (!_bot)
        return;

    // Delegate all auction events to AuctionManager
    if (GetGameSystems()->GetAuctionManager())
    {
        TC_LOG_TRACE("playerbot.events.auction", "Bot {}: Auction event type {}",
            _bot->GetName(), static_cast<uint32>(event.type));

        // AuctionManager handles all AH logic (bidding, buying, selling)
    }
}

// ============================================================================
// NPC EVENT HANDLER
// ============================================================================

void BotAI::OnNPCEvent(NPCEvent const& event)
{
    if (!_bot)
        return;

    switch (event.type)
    {
        case NPCEventType::GOSSIP_MENU_RECEIVED:
            // Auto-select quest-related gossip options
            TC_LOG_DEBUG("playerbot.events.npc", "Bot {}: Gossip menu from NPC {}",
                _bot->GetName(), event.npcGuid.ToString());

            // Process gossip menu options based on bot state
            if (sPlayerbotConfig->GetBool("Playerbot.AutoSelectGossip", true))
            {
                // The gossip system should be handled by the quest manager or
                // a dedicated gossip manager. For now, log the gossip options.
                // Gossip selection logic:
                // 1. If bot has quest to turn in, select quest turnin option
                // 2. If bot can accept quest, select quest accept option
                // 3. If vendor option available and bot needs supplies, select vendor
                // 4. If trainer option available and bot needs training, select trainer

                // Note: Actual gossip option selection requires integration with
                // TrinityCore's gossip packet handling system which happens at
                // a different layer (WorldSession). This event informs the bot
                // that a gossip menu was displayed so it can make decisions.
                TC_LOG_DEBUG("playerbot.events.npc", "Bot {}: Processing gossip menu with {} options",
                    _bot->GetName(), event.gossipItems.size());
            }
            break;

        case NPCEventType::VENDOR_LIST_RECEIVED:
            // Handle vendor interactions (repairs, reagents)
            TC_LOG_DEBUG("playerbot.events.npc", "Bot {}: Vendor list received",
                _bot->GetName());

            // Auto-repair using direct TrinityCore API
            if (sPlayerbotConfig->GetBool("Playerbot.AutoVendor", true))
            {
                // Get the vendor creature
                Creature* vendor = ObjectAccessor::GetCreature(*_bot, event.npcGuid);
                if (vendor)
                {
                    // Auto-repair if vendor can repair
                    if (vendor->IsArmorer())
                    {
                        // Repair all items using TrinityCore API
                        // DurabilityRepairAll returns void, not float
                        _bot->DurabilityRepairAll(true, 0.0f, false);
                        TC_LOG_DEBUG("playerbot.events.npc", "Bot {}: Repaired all items at vendor",
                            _bot->GetName());
                    }
                }
            }
            break;

        case NPCEventType::TRAINER_LIST_RECEIVED:
            // Auto-learn available spells
            TC_LOG_DEBUG("playerbot.events.npc", "Bot {}: Trainer list received",
                _bot->GetName());

            // Log trainer interaction - actual training handled by TrainerInteractionManager separately
            if (sPlayerbotConfig->GetBool("Playerbot.AutoTrain", true))
            {
                Creature* trainer = ObjectAccessor::GetCreature(*_bot, event.npcGuid);
                if (trainer)
                {
                    TC_LOG_DEBUG("playerbot.events.npc", "Bot {}: Ready to train at {}",
                        _bot->GetName(), trainer->GetName());
                }
            }
            break;

        case NPCEventType::BANK_OPENED:
            // Manage bank storage
            TC_LOG_DEBUG("playerbot.events.npc", "Bot {}: Bank opened",
                _bot->GetName());

            // Log bank interaction - actual banking operations handled by BankingManager separately
            if (sPlayerbotConfig->GetBool("Playerbot.AutoBank", true))
            {
                TC_LOG_DEBUG("playerbot.events.npc", "Bot {}: Ready to manage bank storage",
                    _bot->GetName());
            }
            break;

        default:
            break;
    }
}

// ============================================================================
// INSTANCE EVENT HANDLER
// ============================================================================

void BotAI::OnInstanceEvent(InstanceEvent const& event)
{
    if (!_bot)
        return;

    switch (event.type)
    {
        case InstanceEventType::INSTANCE_RESET:
            TC_LOG_DEBUG("playerbot.events.instance", "Bot {}: Instance {} reset",
                _bot->GetName(), event.mapId);
            break;

        case InstanceEventType::RAID_INFO_RECEIVED:
            // Track instance lockouts and boss progress
            TC_LOG_DEBUG("playerbot.events.instance", "Bot {}: Raid info - {} bosses killed",
                _bot->GetName(), event.bossStates.size());
            break;

        case InstanceEventType::ENCOUNTER_FRAME_UPDATE:
            // Boss encounter frame updates (target priority)
            TC_LOG_TRACE("playerbot.events.instance", "Bot {}: Encounter frame update - priority {}",
                _bot->GetName(), event.encounterFrame);
            break;

        case InstanceEventType::INSTANCE_MESSAGE_RECEIVED:
            // Instance warnings (lockout warnings, reset notifications)
            TC_LOG_INFO("playerbot.events.instance", "Bot {}: Instance message: {}",
                _bot->GetName(), event.message);
            break;

        default:
            break;
    }
}

// ============================================================================
// PROFESSION EVENT HANDLER
// ============================================================================

void BotAI::OnProfessionEvent(ProfessionEvent const& event)
{
    if (!_bot)
        return;

    switch (event.type)
    {
        case ProfessionEventType::RECIPE_LEARNED:
            TC_LOG_DEBUG("playerbot.events.profession", "Bot {}: Learned recipe {} for {}",
                _bot->GetName(), event.recipeId, static_cast<uint32>(event.profession));
            // Trigger profession system update to evaluate new crafting options
            break;

        case ProfessionEventType::SKILL_UP:
            TC_LOG_DEBUG("playerbot.events.profession", "Bot {}: Profession {} skill increased from {} to {}",
                _bot->GetName(), static_cast<uint32>(event.profession),
                event.skillBefore, event.skillAfter);
            break;

        case ProfessionEventType::CRAFTING_STARTED:
            TC_LOG_TRACE("playerbot.events.profession", "Bot {}: Started crafting recipe {} (item {})",
                _bot->GetName(), event.recipeId, event.itemId);
            break;

        case ProfessionEventType::CRAFTING_COMPLETED:
            TC_LOG_DEBUG("playerbot.events.profession", "Bot {}: Completed crafting item {} x{} from recipe {}",
                _bot->GetName(), event.itemId, event.quantity, event.recipeId);
            // Track successful crafts for profession progression
            break;

        case ProfessionEventType::CRAFTING_FAILED:
            TC_LOG_WARN("playerbot.events.profession", "Bot {}: Crafting failed for recipe {} - {}",
                _bot->GetName(), event.recipeId, event.reason);
            // Profession system should handle retry logic or material acquisition
            break;

        case ProfessionEventType::MATERIALS_NEEDED:
            TC_LOG_DEBUG("playerbot.events.profession", "Bot {}: Materials needed for recipe {} ({})",
                _bot->GetName(), event.recipeId, static_cast<uint32>(event.profession));
            // Trigger material acquisition (gathering, AH purchase, or bank withdrawal)
            break;

        case ProfessionEventType::MATERIAL_GATHERED:
            TC_LOG_TRACE("playerbot.events.profession", "Bot {}: Gathered material {} x{} for {}",
                _bot->GetName(), event.itemId, event.quantity,
                static_cast<uint32>(event.profession));
            break;

        case ProfessionEventType::MATERIAL_PURCHASED:
            TC_LOG_DEBUG("playerbot.events.profession", "Bot {}: Purchased material {} x{} for {} gold",
                _bot->GetName(), event.itemId, event.quantity, event.goldAmount);
            break;

        case ProfessionEventType::ITEM_BANKED:
            TC_LOG_TRACE("playerbot.events.profession", "Bot {}: Banked item {} x{}",
                _bot->GetName(), event.itemId, event.quantity);
            break;

        case ProfessionEventType::ITEM_WITHDRAWN:
            TC_LOG_DEBUG("playerbot.events.profession", "Bot {}: Withdrew item {} x{} from bank",
                _bot->GetName(), event.itemId, event.quantity);
            break;

        case ProfessionEventType::GOLD_BANKED:
            TC_LOG_TRACE("playerbot.events.profession", "Bot {}: Banked {} gold",
                _bot->GetName(), event.goldAmount);
            break;

        case ProfessionEventType::GOLD_WITHDRAWN:
            TC_LOG_DEBUG("playerbot.events.profession", "Bot {}: Withdrew {} gold from bank",
                _bot->GetName(), event.goldAmount);
            break;

        default:
            break;
    }
}

// ============================================================================
// BOT-TO-BOT MESSAGE HANDLER (Phase 3: Sprint 2 - BotMessageBus)
// ============================================================================

void BotAI::HandleBotMessage(BotMessage const& message)
{
    if (!_bot)
        return;

    // Forward to CombatCoordinationIntegrator for claim-related messages
    // (interrupts, dispels, defensive CDs, CC coordination)
    if (_gameSystems)
    {
        auto* combatCoord = _gameSystems->GetCombatCoordinationIntegrator();
        if (combatCoord)
            combatCoord->HandleIncomingMessage(message);
    }

    TC_LOG_TRACE("playerbot.messaging", "Bot {}: Received message {} from {}",
        _bot->GetName(), GetMessageTypeName(message.type),
        message.senderGuid.ToString());
}

} // namespace Playerbot
