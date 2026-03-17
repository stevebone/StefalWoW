/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PlayerBotHooks.h"
#include "Player.h"
#include "Group.h"
#include "Core/Events/GenericEventBus.h"
#include "Group/GroupEvents.h"
#include "Log.h"
#include "BotSession.h"
#include "BotAI.h"
#include "ObjectAccessor.h"
#include "Core/Services/BotNpcLocationService.h"
#include "Core/Events/CombatEventRouter.h"
#include "PvP/BGBotManager.h"
#include "SpellInfo.h"
#include "SpellAuras.h"
#include "Unit.h"
#include <sstream>

namespace Playerbot
{

// Meyer's singleton accessors for DLL-safe static data
bool& PlayerBotHooks::GetInitialized()
{
    static bool initialized = false;
    return initialized;
}

PlayerBotHooks::HookStatistics& PlayerBotHooks::GetStats()
{
    static HookStatistics stats;
    return stats;
}

void PlayerBotHooks::Initialize()
{
    if (GetInitialized())
    {
        TC_LOG_WARN("module.playerbot", "PlayerBotHooks::Initialize called multiple times");
        return;
    }

    TC_LOG_INFO("module.playerbot", "Initializing PlayerBot hook system...");

    // Initialize core services (NPC location resolution, etc.)
    TC_LOG_INFO("module.playerbot", "Initializing BotNpcLocationService...");
    if (!sBotNpcLocationService->Initialize())
    {
        TC_LOG_FATAL("module.playerbot", "Failed to initialize BotNpcLocationService! Quest and navigation systems will not function.");
    }
    else
    {
        auto stats = sBotNpcLocationService->GetCacheStats();
        TC_LOG_INFO("module.playerbot", "BotNpcLocationService initialized: {} creature spawns, {} gameobject spawns, {} profession trainers, {} service NPCs",
                    stats.creatureSpawnsCached, stats.gameObjectSpawnsCached, stats.professionTrainersCached, stats.serviceNpcsCached);
    }

    RegisterHooks();

    GetInitialized() = true;
    GetStats().Reset();

    TC_LOG_INFO("module.playerbot", "PlayerBot hook system initialized successfully");
}

void PlayerBotHooks::Shutdown()
{
    if (!GetInitialized())
        return;

    TC_LOG_INFO("module.playerbot", "Shutting down PlayerBot hook system...");

    // Shutdown core services
    TC_LOG_INFO("module.playerbot", "Shutting down BotNpcLocationService...");
    sBotNpcLocationService->Shutdown();

    DumpStatistics();
    UnregisterHooks();

    GetInitialized() = false;

    TC_LOG_INFO("module.playerbot", "PlayerBot hook system shutdown complete");
}

bool PlayerBotHooks::IsActive()
{
    return GetInitialized();
}

void PlayerBotHooks::RegisterHooks()
{
    // Register all hook implementations that publish events to GroupEventBus

    OnGroupMemberAdded = [](Group* group, Player* player)
    {
        if (!group || !player)
            return;

        IncrementHookCall("OnGroupMemberAdded");

        // Publish event to GroupEventBus
        GroupEvent event = GroupEvent::MemberJoined(group->GetGUID(), player->GetGUID());
        EventBus<GroupEvent>::instance()->PublishEvent(event);

        TC_LOG_DEBUG("module.playerbot.hooks", "Hook: Member {} joined group {}",
            player->GetName(), group->GetGUID().ToString());
    };

    OnGroupMemberRemoved = [](Group* group, ObjectGuid guid, RemoveMethod method)
    {
        if (!group || guid.IsEmpty())
            return;

        IncrementHookCall("OnGroupMemberRemoved");

        // Publish event to GroupEventBus
        GroupEvent event = GroupEvent::MemberLeft(group->GetGUID(), guid, static_cast<uint32>(method));
        EventBus<GroupEvent>::instance()->PublishEvent(event);

        TC_LOG_DEBUG("module.playerbot.hooks", "Hook: Member {} left group {} (method: {})",
            guid.ToString(), group->GetGUID().ToString(), static_cast<uint32>(method));
    };

    OnGroupLeaderChanged = [](Group* group, ObjectGuid newLeaderGuid)
    {
        if (!group || newLeaderGuid.IsEmpty())
            return;

        IncrementHookCall("OnGroupLeaderChanged");

        // Publish event to GroupEventBus
        GroupEvent event = GroupEvent::LeaderChanged(group->GetGUID(), newLeaderGuid);
        EventBus<GroupEvent>::instance()->PublishEvent(event);

        TC_LOG_DEBUG("module.playerbot.hooks", "Hook: Group {} leader changed to {}",
            group->GetGUID().ToString(), newLeaderGuid.ToString());
    };

    OnGroupDisbanding = [](Group* group)
    {
        if (!group)
            return;

        IncrementHookCall("OnGroupDisbanding");

        // Publish CRITICAL event to GroupEventBus
        GroupEvent event = GroupEvent::GroupDisbanded(group->GetGUID());
        EventBus<GroupEvent>::instance()->PublishEvent(event);

        // NOTE: Per-group event clearing is unnecessary — GenericEventBus events are
        // transient and consumed each update tick. The GroupDisbanded event above
        // is sufficient for subscribers to react and clean up their own state.

        TC_LOG_DEBUG("module.playerbot.hooks", "Hook: Group {} disbanding",
            group->GetGUID().ToString());
    };

    OnGroupRaidConverted = [](Group* group, bool isRaid)
    {
        if (!group)
            return;

        IncrementHookCall("OnGroupRaidConverted");

        // Create raid conversion event
        GroupEvent event;
        event.type = GroupEventType::RAID_CONVERTED;
        event.priority = EventPriority::HIGH;
        event.groupGuid = group->GetGUID();
        event.data1 = isRaid ? 1 : 0;
        event.timestamp = ::std::chrono::steady_clock::now();
        event.expiryTime = event.timestamp + ::std::chrono::milliseconds(30000);

        EventBus<GroupEvent>::instance()->PublishEvent(event);

        TC_LOG_DEBUG("module.playerbot.hooks", "Hook: Group {} converted to {}",
            group->GetGUID().ToString(), isRaid ? "raid" : "party");
    };

    OnSubgroupChanged = [](Group* group, ObjectGuid playerGuid, uint8 newSubgroup)
    {
        if (!group || playerGuid.IsEmpty())
            return;

        IncrementHookCall("OnSubgroupChanged");

        // Create subgroup change event
        GroupEvent event;
        event.type = GroupEventType::SUBGROUP_CHANGED;
        event.priority = EventPriority::MEDIUM;
        event.groupGuid = group->GetGUID();
        event.targetGuid = playerGuid;
        event.data1 = newSubgroup;
        event.timestamp = ::std::chrono::steady_clock::now();
        event.expiryTime = event.timestamp + ::std::chrono::milliseconds(30000);

        EventBus<GroupEvent>::instance()->PublishEvent(event);

        TC_LOG_DEBUG("module.playerbot.hooks", "Hook: Player {} moved to subgroup {} in group {}",
            playerGuid.ToString(), newSubgroup, group->GetGUID().ToString());
    };

    OnLootMethodChanged = [](Group* group, LootMethod method)
    {
        if (!group)
            return;

        IncrementHookCall("OnLootMethodChanged");

        // Publish loot method change event
        GroupEvent event = GroupEvent::LootMethodChanged(group->GetGUID(), static_cast<uint8>(method));
        EventBus<GroupEvent>::instance()->PublishEvent(event);

        TC_LOG_DEBUG("module.playerbot.hooks", "Hook: Group {} loot method changed to {}",
            group->GetGUID().ToString(), static_cast<uint32>(method));
    };

    OnLootThresholdChanged = [](Group* group, uint8 threshold)
    {
        if (!group)
            return;

        IncrementHookCall("OnLootMethodChanged"); // Share counter with loot method

        // Create loot threshold event
        GroupEvent event;
        event.type = GroupEventType::LOOT_THRESHOLD_CHANGED;
        event.priority = EventPriority::LOW;
        event.groupGuid = group->GetGUID();
        event.data1 = threshold;
        event.timestamp = ::std::chrono::steady_clock::now();
        event.expiryTime = event.timestamp + ::std::chrono::milliseconds(30000);

        EventBus<GroupEvent>::instance()->PublishEvent(event);
    };

    OnMasterLooterChanged = [](Group* group, ObjectGuid masterLooterGuid)
    {
        if (!group)
            return;

        IncrementHookCall("OnLootMethodChanged"); // Share counter

        // Create master looter event
        GroupEvent event;
        event.type = GroupEventType::MASTER_LOOTER_CHANGED;
        event.priority = EventPriority::LOW;
        event.groupGuid = group->GetGUID();
        event.targetGuid = masterLooterGuid;
        event.timestamp = ::std::chrono::steady_clock::now();
        event.expiryTime = event.timestamp + ::std::chrono::milliseconds(30000);

        EventBus<GroupEvent>::instance()->PublishEvent(event);
    };

    OnAssistantChanged = [](Group* group, ObjectGuid memberGuid, bool isAssistant)
    {
        if (!group || memberGuid.IsEmpty())
            return;

        IncrementHookCall("OnGroupMemberAdded"); // Share counter for role changes

        // Create assistant change event
        GroupEvent event;
        event.type = GroupEventType::ASSISTANT_CHANGED;
        event.priority = EventPriority::MEDIUM;
        event.groupGuid = group->GetGUID();
        event.targetGuid = memberGuid;
        event.data1 = isAssistant ? 1 : 0;
        event.timestamp = ::std::chrono::steady_clock::now();
        event.expiryTime = event.timestamp + ::std::chrono::milliseconds(30000);

        EventBus<GroupEvent>::instance()->PublishEvent(event);
    };

    OnMainTankChanged = [](Group* group, ObjectGuid tankGuid)
    {
        if (!group)
            return;

        IncrementHookCall("OnGroupMemberAdded"); // Share counter

        // Create main tank event
        GroupEvent event;
        event.type = GroupEventType::MAIN_TANK_CHANGED;
        event.priority = EventPriority::LOW;
        event.groupGuid = group->GetGUID();
        event.targetGuid = tankGuid;
        event.timestamp = ::std::chrono::steady_clock::now();
        event.expiryTime = event.timestamp + ::std::chrono::milliseconds(30000);

        EventBus<GroupEvent>::instance()->PublishEvent(event);
    };

    OnMainAssistChanged = [](Group* group, ObjectGuid assistGuid)
    {
        if (!group)
            return;

        IncrementHookCall("OnGroupMemberAdded"); // Share counter

        // Create main assist event
        GroupEvent event;
        event.type = GroupEventType::MAIN_ASSIST_CHANGED;
        event.priority = EventPriority::LOW;
        event.groupGuid = group->GetGUID();
        event.targetGuid = assistGuid;
        event.timestamp = ::std::chrono::steady_clock::now();
        event.expiryTime = event.timestamp + ::std::chrono::milliseconds(30000);

        EventBus<GroupEvent>::instance()->PublishEvent(event);
    };

    OnRaidTargetIconChanged = [](Group* group, uint8 icon, ObjectGuid targetGuid)
    {
        if (!group)
            return;

        IncrementHookCall("OnRaidTargetIconChanged");

        // Publish target icon event
        GroupEvent event = GroupEvent::TargetIconChanged(group->GetGUID(), icon, targetGuid);
        EventBus<GroupEvent>::instance()->PublishEvent(event);

        TC_LOG_DEBUG("module.playerbot.hooks", "Hook: Group {} target icon {} set to {}",
            group->GetGUID().ToString(), icon, targetGuid.ToString());
    };

    OnRaidMarkerChanged = [](Group* group, uint32 markerId, uint32 mapId, float x, float y, float z)
    {
        if (!group)
            return;

        IncrementHookCall("OnRaidTargetIconChanged"); // Share counter

        // Create marker event
        GroupEvent event;
        event.type = GroupEventType::RAID_MARKER_CHANGED;
        event.priority = EventPriority::LOW;
        event.groupGuid = group->GetGUID();
        event.data1 = markerId;
        event.data2 = mapId;
        // Pack coordinates into data3 (simplified, not perfect but works)
        event.timestamp = ::std::chrono::steady_clock::now();
        event.expiryTime = event.timestamp + ::std::chrono::milliseconds(60000);

        EventBus<GroupEvent>::instance()->PublishEvent(event);
    };

    OnReadyCheckStarted = [](Group* group, ObjectGuid initiatorGuid, uint32 durationMs)
    {
        if (!group || initiatorGuid.IsEmpty())
            return;

        IncrementHookCall("OnReadyCheckStarted");

        // Publish ready check start event
        GroupEvent event = GroupEvent::ReadyCheckStarted(group->GetGUID(), initiatorGuid, durationMs);
        EventBus<GroupEvent>::instance()->PublishEvent(event);

        TC_LOG_DEBUG("module.playerbot.hooks", "Hook: Ready check started in group {} by {} (duration: {}ms)",
            group->GetGUID().ToString(), initiatorGuid.ToString(), durationMs);
    };

    OnReadyCheckResponse = [](Group* group, ObjectGuid memberGuid, bool ready)
    {
        if (!group || memberGuid.IsEmpty())
            return;

        IncrementHookCall("OnReadyCheckStarted"); // Share counter

        // Create ready check response event
        GroupEvent event;
        event.type = GroupEventType::READY_CHECK_RESPONSE;
        event.priority = EventPriority::LOW;
        event.groupGuid = group->GetGUID();
        event.sourceGuid = memberGuid;
        event.data1 = ready ? 1 : 0;
        event.timestamp = ::std::chrono::steady_clock::now();
        event.expiryTime = event.timestamp + ::std::chrono::milliseconds(5000);

        EventBus<GroupEvent>::instance()->PublishEvent(event);
    };

    OnReadyCheckCompleted = [](Group* group, bool allReady, uint32 respondedCount, uint32 totalMembers)
    {
        if (!group)
            return;

        IncrementHookCall("OnReadyCheckStarted"); // Share counter

        // Create ready check completed event
        GroupEvent event;
        event.type = GroupEventType::READY_CHECK_COMPLETED;
        event.priority = EventPriority::BATCH;
        event.groupGuid = group->GetGUID();
        event.data1 = allReady ? 1 : 0;
        event.data2 = respondedCount;
        event.data3 = totalMembers;
        event.timestamp = ::std::chrono::steady_clock::now();
        event.expiryTime = event.timestamp + ::std::chrono::milliseconds(10000);

        EventBus<GroupEvent>::instance()->PublishEvent(event);
    };

    OnDifficultyChanged = [](Group* group, Difficulty difficulty)
    {
        if (!group)
            return;

        IncrementHookCall("OnDifficultyChanged");

        // Publish difficulty change event (WoW 12.0: Difficulty is int16)
        GroupEvent event = GroupEvent::DifficultyChanged(group->GetGUID(), static_cast<int16>(difficulty));
        EventBus<GroupEvent>::instance()->PublishEvent(event);

        TC_LOG_DEBUG("module.playerbot.hooks", "Hook: Group {} difficulty changed to {}",
            group->GetGUID().ToString(), static_cast<uint32>(difficulty));
    };

    OnInstanceBind = [](Group* group, uint32 instanceId, bool permanent)
    {
        if (!group)
            return;

        IncrementHookCall("OnDifficultyChanged"); // Share counter

        // Create instance bind event
        GroupEvent event;
        event.type = GroupEventType::INSTANCE_LOCK_MESSAGE;
        event.priority = EventPriority::MEDIUM;
        event.groupGuid = group->GetGUID();
        event.data1 = instanceId;
        event.data2 = permanent ? 1 : 0;
        event.timestamp = ::std::chrono::steady_clock::now();
        event.expiryTime = event.timestamp + ::std::chrono::milliseconds(60000);

        EventBus<GroupEvent>::instance()->PublishEvent(event);
    };

    // PLAYER LIFECYCLE HOOKS
    OnPlayerDeath = [](Player* player)
    {
        if (!player)
            return;

        // Only handle bot deaths
    if (!IsPlayerBot(player))
            return;

        IncrementHookCall("OnPlayerDeath");

        // Get bot's AI and call OnDeath
    if (WorldSession* session = player->GetSession())
        {
            if (BotSession* botSession = dynamic_cast<BotSession*>(session))
            {
                if (BotAI* ai = botSession->GetAI())
                {
                    ai->OnDeath();
                    TC_LOG_DEBUG("module.playerbot.hooks", "Hook: Bot {} died, OnDeath() called",
                        player->GetName());
                }
            }
        }
    };

    OnPlayerResurrected = [](Player* player)
    {
        if (!player)
            return;

        // Only handle bot resurrections
    if (!IsPlayerBot(player))
            return;

        IncrementHookCall("OnPlayerResurrected");

        // Get bot's AI and call OnRespawn
    if (WorldSession* session = player->GetSession())
        {
            if (BotSession* botSession = dynamic_cast<BotSession*>(session))
            {
                if (BotAI* ai = botSession->GetAI())
                {
                    ai->OnRespawn();
                    TC_LOG_DEBUG("module.playerbot.hooks", "Hook: Bot {} resurrected, OnRespawn() called",
                        player->GetName());
                }
            }
        }
    };

    // ========================================================================
    // COMBAT EVENT HOOKS - Phase 3 Event-Driven Architecture
    // These hooks dispatch events to CombatEventRouter for the new event system
    // ========================================================================

    // Initialize CombatEventRouter
    CombatEventRouter::Instance().Initialize();

    OnDamageDealt = [](Unit* attacker, Unit* victim, uint32 damage, DamageEffectType /*damagetype*/, SpellInfo const* spellProto)
    {
        if (!victim)
            return;

        IncrementHookCall("OnDamageDealt");

        // Create and dispatch damage events
        ObjectGuid attackerGuid = attacker ? attacker->GetGUID() : ObjectGuid::Empty;
        auto event = CombatEvent::CreateDamageTaken(victim->GetGUID(), attackerGuid, damage, 0, spellProto);
        CombatEventRouter::Instance().QueueEvent(event);

        // Also create DAMAGE_DEALT for the attacker
        if (attacker)
        {
            auto dealerEvent = CombatEvent::CreateDamageDealt(attacker->GetGUID(), victim->GetGUID(), damage, spellProto);
            CombatEventRouter::Instance().QueueEvent(dealerEvent);
        }
    };

    OnHealingDone = [](Unit* healer, Unit* target, uint32 healAmount, uint32 overheal, SpellInfo const* spellProto)
    {
        if (!healer || !target)
            return;

        IncrementHookCall("OnHealingDone");

        auto event = CombatEvent::CreateHealingDone(healer->GetGUID(), target->GetGUID(), healAmount, overheal, spellProto);
        CombatEventRouter::Instance().QueueEvent(event);
    };

    OnSpellCastStart = [](Unit* caster, SpellInfo const* spellInfo, Unit* target)
    {
        if (!caster || !spellInfo)
            return;

        IncrementHookCall("OnSpellCastStart");

        // IMPORTANT: Use Dispatch() for immediate delivery - critical for interrupt coordination
        ObjectGuid targetGuid = target ? target->GetGUID() : ObjectGuid::Empty;
        auto event = CombatEvent::CreateSpellCastStart(caster->GetGUID(), spellInfo, targetGuid);
        CombatEventRouter::Instance().Dispatch(event);  // Immediate dispatch!
    };

    OnSpellCastSuccess = [](Unit* caster, SpellInfo const* spellInfo)
    {
        if (!caster || !spellInfo)
            return;

        IncrementHookCall("OnSpellCastSuccess");

        auto event = CombatEvent::CreateSpellCastSuccess(caster->GetGUID(), spellInfo);
        CombatEventRouter::Instance().QueueEvent(event);
    };

    OnSpellInterrupted = [](Unit* caster, SpellInfo const* spellInfo, Unit* interrupter)
    {
        if (!caster)
            return;

        IncrementHookCall("OnSpellInterrupted");

        ObjectGuid interrupterGuid = interrupter ? interrupter->GetGUID() : ObjectGuid::Empty;
        auto event = CombatEvent::CreateSpellInterrupted(caster->GetGUID(), spellInfo, interrupterGuid);
        CombatEventRouter::Instance().QueueEvent(event);
    };

    OnAuraApplied = [](Unit* target, Aura const* aura, Unit* caster)
    {
        if (!target || !aura)
            return;

        IncrementHookCall("OnAuraApplied");

        ObjectGuid casterGuid = caster ? caster->GetGUID() : ObjectGuid::Empty;
        auto event = CombatEvent::CreateAuraApplied(target->GetGUID(), aura, casterGuid);
        CombatEventRouter::Instance().QueueEvent(event);
    };

    OnAuraRemoved = [](Unit* target, Aura const* aura)
    {
        if (!target || !aura)
            return;

        IncrementHookCall("OnAuraRemoved");

        auto event = CombatEvent::CreateAuraRemoved(target->GetGUID(), aura);
        CombatEventRouter::Instance().QueueEvent(event);
    };

    OnThreatChanged = [](Unit* threatOwner, Unit* victim, float oldThreat, float newThreat)
    {
        if (!threatOwner || !victim)
            return;

        // Only dispatch significant threat changes (>1% change or >100 delta)
        float delta = newThreat - oldThreat;
        if (oldThreat > 0.0f)
        {
            float percentChange = (delta / oldThreat) * 100.0f;
            if (percentChange < 1.0f && ::std::abs(delta) < 100.0f)
                return;  // Skip insignificant changes
        }

        IncrementHookCall("OnThreatChanged");

        auto event = CombatEvent::CreateThreatChanged(threatOwner->GetGUID(), victim->GetGUID(), oldThreat, newThreat);
        CombatEventRouter::Instance().QueueEvent(event);
    };

    OnUnitDied = [](Unit* victim, Unit* killer)
    {
        if (!victim)
            return;

        IncrementHookCall("OnUnitDied");

        ObjectGuid killerGuid = killer ? killer->GetGUID() : ObjectGuid::Empty;
        auto event = CombatEvent::CreateUnitDied(victim->GetGUID(), killerGuid);
        CombatEventRouter::Instance().QueueEvent(event);
    };

    OnCombatStarted = [](Unit* unit)
    {
        if (!unit)
            return;

        IncrementHookCall("OnCombatStarted");

        auto event = CombatEvent::CreateCombatStarted(unit->GetGUID());
        CombatEventRouter::Instance().QueueEvent(event);
    };

    OnCombatEnded = [](Unit* unit)
    {
        if (!unit)
            return;

        IncrementHookCall("OnCombatEnded");

        auto event = CombatEvent::CreateCombatEnded(unit->GetGUID());
        CombatEventRouter::Instance().QueueEvent(event);
    };

    // ========================================================================
    // BATTLEGROUND/ARENA HOOKS
    // ========================================================================

    OnBGInvitationReceived = [](Player* player, uint32 bgInstanceGuid, uint32 bgTypeId)
    {
        if (!player)
            return;

        // Only handle bot invitations
        if (!IsPlayerBot(player))
            return;

        IncrementHookCall("OnBGInvitationReceived");

        TC_LOG_INFO("module.playerbot.hooks", "Hook: Bot {} received BG invitation (BG={}, instance={}), auto-accepting",
            player->GetName(), bgTypeId, bgInstanceGuid);

        // Auto-accept via BGBotManager
        sBGBotManager->OnInvitationReceived(player->GetGUID(), bgInstanceGuid);
    };

    TC_LOG_DEBUG("module.playerbot", "PlayerBotHooks: All {} hook functions registered (including combat events)", 33);
}

void PlayerBotHooks::UnregisterHooks()
{
    // Clear all hook function pointers
    OnGroupMemberAdded = nullptr;
    OnGroupMemberRemoved = nullptr;
    OnGroupLeaderChanged = nullptr;
    OnGroupDisbanding = nullptr;
    OnGroupRaidConverted = nullptr;
    OnSubgroupChanged = nullptr;
    OnLootMethodChanged = nullptr;
    OnLootThresholdChanged = nullptr;
    OnMasterLooterChanged = nullptr;
    OnAssistantChanged = nullptr;
    OnMainTankChanged = nullptr;
    OnMainAssistChanged = nullptr;
    OnRaidTargetIconChanged = nullptr;
    OnRaidMarkerChanged = nullptr;
    OnReadyCheckStarted = nullptr;
    OnReadyCheckResponse = nullptr;
    OnReadyCheckCompleted = nullptr;
    OnDifficultyChanged = nullptr;
    OnInstanceBind = nullptr;
    OnPlayerDeath = nullptr;
    OnPlayerResurrected = nullptr;

    // Clear combat event hooks
    OnDamageDealt = nullptr;
    OnHealingDone = nullptr;
    OnSpellCastStart = nullptr;
    OnSpellCastSuccess = nullptr;
    OnSpellInterrupted = nullptr;
    OnAuraApplied = nullptr;
    OnAuraRemoved = nullptr;
    OnThreatChanged = nullptr;
    OnUnitDied = nullptr;
    OnCombatStarted = nullptr;
    OnCombatEnded = nullptr;

    // Clear BG/Arena hooks
    OnBGInvitationReceived = nullptr;

    // Shutdown CombatEventRouter
    CombatEventRouter::Instance().Shutdown();

    TC_LOG_DEBUG("module.playerbot", "PlayerBotHooks: All hook functions unregistered (including combat events)");
}

bool PlayerBotHooks::IsPlayerBot(Player const* player)
{
    if (!player)
        return false;

    WorldSession* session = player->GetSession();
    if (!session)
        return false;

    // Check if session is a BotSession
    return dynamic_cast<BotSession const*>(session) != nullptr;
}

uint32 PlayerBotHooks::GetBotCountInGroup(Group const* group)
{
    if (!group)
        return 0;

    uint32 botCount = 0;

    for (auto const& slot : group->GetMemberSlots())
    {
        if (Player* member = ObjectAccessor::FindPlayer(slot.guid))
        {
            if (IsPlayerBot(member))
                ++botCount;
        }
    }

    return botCount;
}

bool PlayerBotHooks::GroupHasBots(Group const* group)
{
    return GetBotCountInGroup(group) > 0;
}

void PlayerBotHooks::IncrementHookCall(const char* hookName)
{
    ++GetStats().totalHookCalls;

    // Map specific hook calls to stat counters
    // Using string comparison for simplicity (could optimize with enum)
    ::std::string hook(hookName);

    if (hook == "OnGroupMemberAdded")
        ++GetStats().memberAddedCalls;
    else if (hook == "OnGroupMemberRemoved")
        ++GetStats().memberRemovedCalls;
    else if (hook == "OnGroupLeaderChanged")
        ++GetStats().leaderChangedCalls;
    else if (hook == "OnGroupDisbanding")
        ++GetStats().groupDisbandedCalls;
    else if (hook == "OnGroupRaidConverted")
        ++GetStats().raidConvertedCalls;
    else if (hook == "OnLootMethodChanged")
        ++GetStats().lootMethodChangedCalls;
    else if (hook == "OnReadyCheckStarted")
        ++GetStats().readyCheckCalls;
    else if (hook == "OnRaidTargetIconChanged")
        ++GetStats().targetIconCalls;
    else if (hook == "OnDifficultyChanged")
        ++GetStats().difficultyCalls;
    else if (hook == "OnPlayerDeath")
        ++GetStats().playerDeathCalls;
    else if (hook == "OnPlayerResurrected")
        ++GetStats().playerResurrectedCalls;
    // Combat event hooks
    else if (hook == "OnDamageDealt")
        ++GetStats().damageDealtCalls;
    else if (hook == "OnHealingDone")
        ++GetStats().healingDoneCalls;
    else if (hook == "OnSpellCastStart")
        ++GetStats().spellCastStartCalls;
    else if (hook == "OnSpellCastSuccess")
        ++GetStats().spellCastSuccessCalls;
    else if (hook == "OnSpellInterrupted")
        ++GetStats().spellInterruptedCalls;
    else if (hook == "OnAuraApplied")
        ++GetStats().auraAppliedCalls;
    else if (hook == "OnAuraRemoved")
        ++GetStats().auraRemovedCalls;
    else if (hook == "OnThreatChanged")
        ++GetStats().threatChangedCalls;
    else if (hook == "OnUnitDied")
        ++GetStats().unitDiedCalls;
    else if (hook == "OnCombatStarted")
        ++GetStats().combatStartedCalls;
    else if (hook == "OnCombatEnded")
        ++GetStats().combatEndedCalls;
}

PlayerBotHooks::HookStatistics const& PlayerBotHooks::GetStatistics()
{
    return GetStats();
}

void PlayerBotHooks::ResetStatistics()
{
    GetStats().Reset();
    TC_LOG_DEBUG("module.playerbot", "PlayerBotHooks: Statistics reset");
}

void PlayerBotHooks::DumpStatistics()
{
    TC_LOG_INFO("module.playerbot", "=== PlayerBot Hook Statistics ===");
    TC_LOG_INFO("module.playerbot", "{}", GetStats().ToString());
}

::std::string PlayerBotHooks::HookStatistics::ToString() const
{
    ::std::ostringstream oss;
    oss << "Total Hook Calls: " << totalHookCalls << "\n"
        << "  Group Events: Added=" << memberAddedCalls
        << ", Removed=" << memberRemovedCalls
        << ", Leader=" << leaderChangedCalls
        << ", Disbanded=" << groupDisbandedCalls
        << ", RaidConvert=" << raidConvertedCalls
        << ", Loot=" << lootMethodChangedCalls
        << ", ReadyCheck=" << readyCheckCalls
        << ", TargetIcon=" << targetIconCalls
        << ", Difficulty=" << difficultyCalls << "\n"
        << "  Player Events: Deaths=" << playerDeathCalls
        << ", Resurrected=" << playerResurrectedCalls << "\n"
        << "  Combat Events: Damage=" << damageDealtCalls
        << ", Healing=" << healingDoneCalls
        << ", SpellStart=" << spellCastStartCalls
        << ", SpellSuccess=" << spellCastSuccessCalls
        << ", Interrupted=" << spellInterruptedCalls
        << ", AuraApplied=" << auraAppliedCalls
        << ", AuraRemoved=" << auraRemovedCalls
        << ", Threat=" << threatChangedCalls
        << ", Died=" << unitDiedCalls
        << ", CombatStart=" << combatStartedCalls
        << ", CombatEnd=" << combatEndedCalls;
    return oss.str();
}

void PlayerBotHooks::HookStatistics::Reset()
{
    totalHookCalls = 0;
    memberAddedCalls = 0;
    memberRemovedCalls = 0;
    leaderChangedCalls = 0;
    groupDisbandedCalls = 0;
    raidConvertedCalls = 0;
    lootMethodChangedCalls = 0;
    readyCheckCalls = 0;
    targetIconCalls = 0;
    difficultyCalls = 0;
    playerDeathCalls = 0;
    playerResurrectedCalls = 0;
    // Reset combat event statistics
    damageDealtCalls = 0;
    healingDoneCalls = 0;
    spellCastStartCalls = 0;
    spellCastSuccessCalls = 0;
    spellInterruptedCalls = 0;
    auraAppliedCalls = 0;
    auraRemovedCalls = 0;
    threatChangedCalls = 0;
    unitDiedCalls = 0;
    combatStartedCalls = 0;
    combatEndedCalls = 0;
}

} // namespace Playerbot
