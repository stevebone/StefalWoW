/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "GroupEventHandler.h"
#include "Core/Events/GenericEventBus.h"
#include "GroupEvents.h"
#include "Player.h"
#include "Group.h"
#include "Log.h"
// INTEGRATION REQUIRED: BotAI header inclusion blocked
// Implementation depends on BotAI being available to provide:
// - BotAI class definition and interface methods
// - Access to bot-specific managers (FormationMgr, HealingMgr, LootMgr)
// See: BotAI.h for required interface
// #include "BotAI.h"

namespace Playerbot
{

// ============================================================================
// BASE CLASS HELPERS
// ============================================================================

Player* GroupEventHandler::GetBotPlayer() const
{
    if (!_botAI)
        return nullptr;

    // INTEGRATION REQUIRED: BotAI::GetPlayer() access blocked
    // Implementation depends on BotAI providing:
    // - GetPlayer() method returning associated Player* instance
    // See: BotAI.h for required interface
    // return _botAI->GetPlayer();
    return nullptr;
}

Group* GroupEventHandler::GetBotGroup() const
{
    Player* player = GetBotPlayer();
    if (!player)
        return nullptr;

    return player->GetGroup();
}

bool GroupEventHandler::IsBotInEventGroup(GroupEvent const& event) const
{
    Group* botGroup = GetBotGroup();
    if (!botGroup)
        return false;

    return botGroup->GetGUID() == event.groupGuid;
}

void GroupEventHandler::LogEventHandling(GroupEvent const& event, ::std::string const& action) const
{
    TC_LOG_DEBUG("playerbot.group.handler", "{}: {} - Group: {}, Action: {}",
        GetHandlerName(),
        event.ToString(),
        event.groupGuid.ToString(),
        action);
}

// ============================================================================
// MEMBER JOINED HANDLER
// ============================================================================

bool MemberJoinedHandler::HandleEvent(GroupEvent const& event)
{
    if (!IsBotInEventGroup(event))
        return true; // Not our group, ignore

    Player* bot = GetBotPlayer();
    if (!bot)
        return false;

    Group* group = GetBotGroup();
    if (!group)
        return false;

    ObjectGuid newMemberGuid = event.targetGuid;

    LogEventHandling(event, "Member joined group");

    // INTEGRATION REQUIRED: Member join handling logic blocked
    // Implementation depends on BotAI integration providing:
    // - FormationMgr::AddMember() for updating bot formations
    // - Social features configuration and Say() method for greeting
    // - HealingMgr::AddHealTarget() for adding to healing priority list
    // - Role assignment system integration
    // See: BotAI.h, FormationMgr.h, HealingMgr.h for required interfaces
    //
    // Required implementation steps:
    // 1. Update group member cache
    // 2. Greet new member (if social features enabled)
    // 3. Adjust formation to accommodate new member
    // 4. Update role assignments if needed

    /*
    if (Player* newMember = ObjectAccessor::FindConnectedPlayer(newMemberGuid))
    {
        // Update formation
        _botAI->GetFormationMgr()->AddMember(newMember);

        // Greet new member
    if (_botAI->GetConfig()->SocialFeaturesEnabled())
            _botAI->Say(fmt::format("Welcome to the group, {}!", newMember->GetName()));

        // Update healing/buff priorities
        _botAI->GetHealingMgr()->AddHealTarget(newMember);
    }
    */

    TC_LOG_INFO("playerbot.group", "Bot {} handled member joined: {}",
        bot->GetName(), newMemberGuid.ToString());

    return true;
}

// ============================================================================
// MEMBER LEFT HANDLER
// ============================================================================

bool MemberLeftHandler::HandleEvent(GroupEvent const& event)
{
    if (!IsBotInEventGroup(event))
        return true;

    Player* bot = GetBotPlayer();
    if (!bot)
        return false;

    Group* group = GetBotGroup();
    if (!group)
        return false;

    ObjectGuid leftMemberGuid = event.targetGuid;
    uint32 removeMethod = event.data1;

    LogEventHandling(event, fmt::format("Member left (method: {})", removeMethod));

    // INTEGRATION REQUIRED: Member leave handling logic blocked
    // Implementation depends on BotAI integration providing:
    // - FormationMgr::RemoveMember() for updating formations
    // - HealingMgr::RemoveHealTarget() for removing from healing priority
    // - SwitchToSoloStrategy() for small group size adaptation
    // - Role reassignment system for taking over departed roles
    // See: BotAI.h, FormationMgr.h, HealingMgr.h for required interfaces
    //
    // Required implementation steps:
    // 1. Update group member cache
    // 2. Adjust formation after member leaves
    // 3. Take over roles if departed member had important role
    // 4. Check if group should disband (too few members)

    /*
    // Remove from formation
    _botAI->GetFormationMgr()->RemoveMember(leftMemberGuid);

    // Remove from healing priorities
    _botAI->GetHealingMgr()->RemoveHealTarget(leftMemberGuid);

    // Check if departed member was tank/healer - adjust strategy
    if (group->GetMembersCount() < 3)
    {
        _botAI->SwitchToSoloStrategy();
    }
    */

    TC_LOG_INFO("playerbot.group", "Bot {} handled member left: {}",
        bot->GetName(), leftMemberGuid.ToString());

    return true;
}

// ============================================================================
// LEADER CHANGED HANDLER
// ============================================================================

bool LeaderChangedHandler::HandleEvent(GroupEvent const& event)
{
    if (!IsBotInEventGroup(event))
        return true;

    Player* bot = GetBotPlayer();
    if (!bot)
        return false;

    Group* group = GetBotGroup();
    if (!group)
        return false;

    ObjectGuid newLeaderGuid = event.targetGuid;

    LogEventHandling(event, fmt::format("Leader changed to {}", newLeaderGuid.ToString()));

    // INTEGRATION REQUIRED: Leader change handling logic blocked
    // Implementation depends on BotAI integration providing:
    // - SetGroupLeader() for updating leader reference
    // - IsFollowing() and Follow() for leader following logic
    // - SetMainAssist() for updating main assist reference
    // - Group member flag checking for role detection
    // See: BotAI.h for required interface
    //
    // Required implementation steps:
    // 1. Update leader reference
    // 2. Follow new leader if following old leader
    // 3. Update assist target if leader is main assist
    // 4. Adjust behavior based on new leader

    /*
    if (Player* newLeader = ObjectAccessor::FindConnectedPlayer(newLeaderGuid))
    {
        _botAI->SetGroupLeader(newLeader);

        // If bot was following old leader, follow new leader
    if (_botAI->IsFollowing())
            _botAI->Follow(newLeader);

        // Update main assist if leader is MA
    if (group->GetMemberFlags(newLeaderGuid) & MEMBER_FLAG_MAINASSIST)
            _botAI->SetMainAssist(newLeader);
    }
    */

    TC_LOG_INFO("playerbot.group", "Bot {} handled leader changed to: {}",
        bot->GetName(), newLeaderGuid.ToString());

    return true;
}

// ============================================================================
// GROUP DISBANDED HANDLER
// ============================================================================

bool GroupDisbandedHandler::HandleEvent(GroupEvent const& event)
{
    // Don't check IsBotInEventGroup - group is already disbanded
    Player* bot = GetBotPlayer();
    if (!bot)
        return false;

    LogEventHandling(event, "Group disbanded");

    // INTEGRATION REQUIRED: Group disband cleanup logic blocked
    // Implementation depends on BotAI integration providing:
    // - ClearGroupMembers() for clearing member cache
    // - SetGroupLeader/SetMainTank/SetMainAssist() for clearing references
    // - StopFollowing() for stopping movement
    // - FormationMgr::Clear() for clearing formation
    // - SwitchToSoloStrategy() for behavior adaptation
    // See: BotAI.h, FormationMgr.h for required interfaces
    //
    // Required implementation steps:
    // 1. Clean up all group-related state
    // 2. Stop following group members
    // 3. Clear combat coordination state
    // 4. Return to idle behavior

    /*
    // Clear all group references
    _botAI->ClearGroupMembers();
    _botAI->SetGroupLeader(nullptr);
    _botAI->SetMainTank(nullptr);
    _botAI->SetMainAssist(nullptr);

    // Stop following
    _botAI->StopFollowing();

    // Clear formation
    _botAI->GetFormationMgr()->Clear();

    // Return to solo strategy
    _botAI->SwitchToSoloStrategy();
    */

    TC_LOG_INFO("playerbot.group", "Bot {} handled group disbanded: {}",
        bot->GetName(), event.groupGuid.ToString());

    return true;
}

// ============================================================================
// LOOT METHOD CHANGED HANDLER
// ============================================================================

bool LootMethodChangedHandler::HandleEvent(GroupEvent const& event)
{
    if (!IsBotInEventGroup(event))
        return true;

    Player* bot = GetBotPlayer();
    if (!bot)
        return false;

    Group* group = GetBotGroup();
    if (!group)
        return false;

    if (event.type == GroupEventType::LOOT_METHOD_CHANGED)
    {
        uint8 newMethod = event.data1;
        LogEventHandling(event, fmt::format("Loot method changed to {}", newMethod));

        // INTEGRATION REQUIRED: Loot method change handling blocked
        // Implementation depends on BotAI integration providing:
        // - LootMgr::SetLootMethod() for updating loot behavior
        // See: BotAI.h, LootMgr.h for required interface
        // _botAI->GetLootMgr()->SetLootMethod(static_cast<LootMethod>(newMethod));
    }
    else if (event.type == GroupEventType::LOOT_THRESHOLD_CHANGED)
    {
        uint8 newThreshold = event.data1;
        LogEventHandling(event, fmt::format("Loot threshold changed to {}", newThreshold));

        // INTEGRATION REQUIRED: Loot threshold change handling blocked
        // Implementation depends on BotAI integration providing:
        // - LootMgr::SetLootThreshold() for updating roll behavior
        // See: BotAI.h, LootMgr.h for required interface
        // _botAI->GetLootMgr()->SetLootThreshold(static_cast<ItemQualities>(newThreshold));
    }
    else if (event.type == GroupEventType::MASTER_LOOTER_CHANGED)
    {
        ObjectGuid masterLooterGuid = event.targetGuid;
        LogEventHandling(event, fmt::format("Master looter changed to {}", masterLooterGuid.ToString()));

        // INTEGRATION REQUIRED: Master looter change handling blocked
        // Implementation depends on BotAI integration providing:
        // - LootMgr::SetMasterLooter() for tracking master looter
        // See: BotAI.h, LootMgr.h for required interface
        // _botAI->GetLootMgr()->SetMasterLooter(masterLooterGuid);
    }

    return true;
}

// ============================================================================
// TARGET ICON CHANGED HANDLER
// ============================================================================

bool TargetIconChangedHandler::HandleEvent(GroupEvent const& event)
{
    if (!IsBotInEventGroup(event))
        return true;

    Player* bot = GetBotPlayer();
    if (!bot)
        return false;

    uint8 iconIndex = event.data1;
    ObjectGuid targetGuid = event.targetGuid;

    LogEventHandling(event, fmt::format("Target icon {} set to {}", iconIndex, targetGuid.ToString()));

    // INTEGRATION REQUIRED: Target icon handling logic blocked
    // Implementation depends on BotAI integration providing:
    // - SetPrimaryTarget() for kill priority targets
    // - SetCCTarget() for crowd control targets
    // - SetFocusTarget() for focus fire targets
    // - ClearTargetIcon() for icon removal
    // See: BotAI.h for required interface
    //
    // Target icon mappings (WoW standard):
    // Skull (0) = kill first
    // X (1) = sheep/CC
    // Square (2) = secondary target
    // Moon (3) = tertiary target
    // Triangle (4) = trap/freeze
    // Diamond (5) = sap/blind
    // Circle (6) = off-tank
    // Star (7) = focus fire

    /*
    if (Unit* target = ObjectAccessor::GetUnit(*bot, targetGuid))
    {
        // Update target priority based on icon
    switch (iconIndex)
        {
            case 0: // Skull - kill first
                _botAI->SetPrimaryTarget(target);
                break;
            case 1: // X - CC target
                _botAI->SetCCTarget(target);
                break;
            case 7: // Star - focus fire
                _botAI->SetFocusTarget(target);
                break;
            // ... handle other icons
        }
    }
    else if (targetGuid.IsEmpty())
    {
        // Icon cleared
        _botAI->ClearTargetIcon(iconIndex);
    }
    */

    return true;
}

// ============================================================================
// READY CHECK HANDLER
// ============================================================================

bool ReadyCheckHandler::HandleEvent(GroupEvent const& event)
{
    if (!IsBotInEventGroup(event))
        return true;

    Player* bot = GetBotPlayer();
    if (!bot)
        return false;

    Group* group = GetBotGroup();
    if (!group)
        return false;

    if (event.type == GroupEventType::READY_CHECK_STARTED)
    {
        ObjectGuid initiatorGuid = event.sourceGuid;
        uint32 durationMs = event.data1;

        LogEventHandling(event, fmt::format("Ready check started by {} (duration: {}ms)",
            initiatorGuid.ToString(), durationMs));

        // INTEGRATION REQUIRED: Ready check response logic blocked
        // Implementation depends on BotAI integration providing:
        // - IsReadyForCombat() for checking bot readiness state
        // - PrepareForCombat() for preparation actions with callback
        // - Packet sending infrastructure for ReadyCheckResponseClient
        // See: BotAI.h for required interface
        //
        // Required implementation steps:
        // 1. Check if bot is ready (health, mana, cooldowns, position)
        // 2. Send ready/not ready response
        // 3. If not ready, move to safe position and prepare

        /*
        bool isReady = _botAI->IsReadyForCombat();

        if (isReady)
        {
            // Send ready response
            WorldPackets::Party::ReadyCheckResponseClient response;
            response.Player = bot->GetGUID();
            response.IsReady = true;
            bot->SendDirectMessage(response.Write());
        }
        else
        {
            // Prepare and then respond
            _botAI->PrepareForCombat([bot, group]() {
                WorldPackets::Party::ReadyCheckResponseClient response;
                response.Player = bot->GetGUID();
                response.IsReady = true;
                bot->SendDirectMessage(response.Write());
            });
        }
        */
    }
    else if (event.type == GroupEventType::READY_CHECK_COMPLETED)
    {
        bool allReady = event.data1 != 0;
        LogEventHandling(event, fmt::format("Ready check completed (all ready: {})", allReady));

        // INTEGRATION REQUIRED: Ready check completion handling blocked
        // Implementation depends on BotAI integration providing:
        // - PrepareForPull() for pre-encounter preparation
        // See: BotAI.h for required interface
        // if (allReady)
        //     _botAI->PrepareForPull();
    }

    return true;
}

// ============================================================================
// RAID CONVERTED HANDLER
// ============================================================================

bool RaidConvertedHandler::HandleEvent(GroupEvent const& event)
{
    if (!IsBotInEventGroup(event))
        return true;

    Player* bot = GetBotPlayer();
    if (!bot)
        return false;

    bool isRaid = event.data1 != 0;

    LogEventHandling(event, fmt::format("Group converted to {}", isRaid ? "RAID" : "PARTY"));

    // INTEGRATION REQUIRED: Raid conversion logic blocked
    // Implementation depends on BotAI integration providing:
    // - FormationMgr::SwitchToRaidFormation/SwitchToPartyFormation()
    // - EnableRaidAbilities/DisableRaidAbilities() for ability toggling
    // - HealingMgr::SetRaidHealingMode() for healing priority adjustment
    // See: BotAI.h, FormationMgr.h, HealingMgr.h for required interfaces
    //
    // Required implementation steps:
    // 1. Update formation (party formation vs raid subgroups)
    // 2. Enable/disable raid-specific abilities
    // 3. Update healing/buffing priorities
    // 4. Adjust positioning

    /*
    if (isRaid)
    {
        // Switch to raid formation
        _botAI->GetFormationMgr()->SwitchToRaidFormation();

        // Enable raid abilities
        _botAI->EnableRaidAbilities();

        // Update healing to prioritize subgroup
        _botAI->GetHealingMgr()->SetRaidHealingMode(true);
    }
    else
    {
        // Switch to party formation
        _botAI->GetFormationMgr()->SwitchToPartyFormation();

        // Disable raid-only abilities
        _botAI->DisableRaidAbilities();

        // Normal party healing
        _botAI->GetHealingMgr()->SetRaidHealingMode(false);
    }
    */

    return true;
}

// ============================================================================
// SUBGROUP CHANGED HANDLER
// ============================================================================

bool SubgroupChangedHandler::HandleEvent(GroupEvent const& event)
{
    if (!IsBotInEventGroup(event))
        return true;

    Player* bot = GetBotPlayer();
    if (!bot)
        return false;

    ObjectGuid memberGuid = event.targetGuid;
    uint8 newSubgroup = event.data1;

    LogEventHandling(event, fmt::format("Member {} moved to subgroup {}",
        memberGuid.ToString(), newSubgroup));

    // INTEGRATION REQUIRED: Subgroup awareness logic blocked
    // Implementation depends on BotAI integration providing:
    // - OnSubgroupChanged() for bot's own subgroup changes
    // - HealingMgr::AddSubgroupMember/RemoveSubgroupMember() for priority management
    // - Subgroup-based proximity and buff distribution logic
    // See: BotAI.h, HealingMgr.h for required interfaces
    //
    // Required implementation steps:
    // 1. Update bot's subgroup tracking
    // 2. Adjust healing priority (prioritize own subgroup)
    // 3. Update buff distribution (chain heal targets, etc.)
    // 4. Maintain proximity to subgroup members

    /*
    Player* movedMember = ObjectAccessor::FindConnectedPlayer(memberGuid);
    if (!movedMember)
        return true;

    // Check if it's the bot itself that moved
    if (memberGuid == bot->GetGUID())
    {
        _botAI->OnSubgroupChanged(newSubgroup);
        return true;
    }

    // Another member moved
    uint8 botSubgroup = bot->GetSubGroup();
    if (newSubgroup == botSubgroup)
    {
        // Member joined our subgroup - increase healing priority
        _botAI->GetHealingMgr()->AddSubgroupMember(movedMember);
    }
    else
    {
        // Member left our subgroup - decrease healing priority
        _botAI->GetHealingMgr()->RemoveSubgroupMember(movedMember);
    }
    */

    return true;
}

// ============================================================================
// ROLE ASSIGNMENT HANDLER
// ============================================================================

bool RoleAssignmentHandler::HandleEvent(GroupEvent const& event)
{
    if (!IsBotInEventGroup(event))
        return true;

    Player* bot = GetBotPlayer();
    if (!bot)
        return false;

    Group* group = GetBotGroup();
    if (!group)
        return false;

    ObjectGuid memberGuid = event.targetGuid;
    bool isAssigned = event.data1 != 0;

    if (event.type == GroupEventType::MAIN_TANK_CHANGED)
    {
        LogEventHandling(event, fmt::format("Main tank {} to {}",
            isAssigned ? "assigned" : "removed", memberGuid.ToString()));

        // INTEGRATION REQUIRED: Main tank tracking logic blocked
        // Implementation depends on BotAI integration providing:
        // - SetMainTank() for updating main tank reference
        // See: BotAI.h for required interface
        /*
        if (Player* tank = ObjectAccessor::FindConnectedPlayer(memberGuid))
        {
            if (isAssigned)
                _botAI->SetMainTank(tank);
            else
                _botAI->SetMainTank(nullptr);
        }
        */
    }
    else if (event.type == GroupEventType::MAIN_ASSIST_CHANGED)
    {
        LogEventHandling(event, fmt::format("Main assist {} to {}",
            isAssigned ? "assigned" : "removed", memberGuid.ToString()));

        // INTEGRATION REQUIRED: Main assist tracking logic blocked
        // Implementation depends on BotAI integration providing:
        // - SetMainAssist() for updating main assist reference
        // - EnableAssistMode() for toggling assist behavior
        // See: BotAI.h for required interface
        /*
        if (Player* assist = ObjectAccessor::FindConnectedPlayer(memberGuid))
        {
            if (isAssigned)
            {
                _botAI->SetMainAssist(assist);
                // Start assisting main assist's target
                _botAI->EnableAssistMode(true);
            }
            else
            {
                _botAI->SetMainAssist(nullptr);
                _botAI->EnableAssistMode(false);
            }
        }
        */
    }
    else if (event.type == GroupEventType::ASSISTANT_CHANGED)
    {
        LogEventHandling(event, fmt::format("Assistant status {} for {}",
            isAssigned ? "granted" : "revoked", memberGuid.ToString()));

        // INTEGRATION REQUIRED: Assistant powers handling logic blocked
        // Implementation depends on BotAI integration providing:
        // - EnableAssistantPowers() for granting assistant capabilities
        // - DisableAssistantPowers() for revoking assistant capabilities
        // See: BotAI.h for required interface
        /*
        if (memberGuid == bot->GetGUID())
        {
            if (isAssigned)
                _botAI->EnableAssistantPowers();
            else
                _botAI->DisableAssistantPowers();
        }
        */
    }

    return true;
}

// ============================================================================
// DIFFICULTY CHANGED HANDLER
// ============================================================================

bool DifficultyChangedHandler::HandleEvent(GroupEvent const& event)
{
    if (!IsBotInEventGroup(event))
        return true;

    Player* bot = GetBotPlayer();
    if (!bot)
        return false;

    uint8 newDifficulty = event.data1;

    LogEventHandling(event, fmt::format("Difficulty changed to {}", newDifficulty));

    // INTEGRATION REQUIRED: Difficulty adjustment logic blocked
    // Implementation depends on BotAI integration providing:
    // - SetDifficulty() for updating difficulty awareness
    // - SetCooldownUsage*() for adjusting cooldown strategies
    // - EnableConsumableUsage() for toggling consumable usage
    // - IsGearedForDifficulty() for gear checking
    // - Warn() for notifying about undergear status
    // See: BotAI.h for required interface
    //
    // Required implementation steps:
    // 1. Update combat difficulty expectations
    // 2. Adjust cooldown usage for heroic/mythic
    // 3. Update consumable usage thresholds
    // 4. Warn if undergeared

    /*
    _botAI->SetDifficulty(static_cast<Difficulty>(newDifficulty));

    // Adjust strategy based on difficulty
    switch (newDifficulty)
    {
        case DIFFICULTY_NORMAL:
        case DIFFICULTY_NORMAL_RAID:
            _botAI->SetCooldownUsageConservative();
            break;

        case DIFFICULTY_HEROIC:
        case DIFFICULTY_HEROIC_RAID:
            _botAI->SetCooldownUsageAggressive();
            break;

        case DIFFICULTY_MYTHIC:
        case DIFFICULTY_MYTHIC_RAID:
            _botAI->SetCooldownUsageMaximal();
            _botAI->EnableConsumableUsage(true);
            break;
    }

    // Check if bot is appropriately geared
    if (!_botAI->IsGearedForDifficulty(newDifficulty))
    {
        _botAI->Warn("May be undergeared for this difficulty!");
    }
    */

    return true;
}

// ============================================================================
// HANDLER FACTORY
// ============================================================================

::std::vector<::std::unique_ptr<GroupEventHandler>> GroupEventHandlerFactory::CreateAllHandlers(BotAI* botAI)
{
    ::std::vector<::std::unique_ptr<GroupEventHandler>> handlers;

    // Create all handler instances
    handlers.push_back(::std::make_unique<MemberJoinedHandler>(botAI));
    handlers.push_back(::std::make_unique<MemberLeftHandler>(botAI));
    handlers.push_back(::std::make_unique<LeaderChangedHandler>(botAI));
    handlers.push_back(::std::make_unique<GroupDisbandedHandler>(botAI));
    handlers.push_back(::std::make_unique<LootMethodChangedHandler>(botAI));
    handlers.push_back(::std::make_unique<TargetIconChangedHandler>(botAI));
    handlers.push_back(::std::make_unique<ReadyCheckHandler>(botAI));
    handlers.push_back(::std::make_unique<RaidConvertedHandler>(botAI));
    handlers.push_back(::std::make_unique<SubgroupChangedHandler>(botAI));
    handlers.push_back(::std::make_unique<RoleAssignmentHandler>(botAI));
    handlers.push_back(::std::make_unique<DifficultyChangedHandler>(botAI));

    TC_LOG_DEBUG("playerbot.group", "Created {} event handlers for bot", handlers.size());

    return handlers;
}

void GroupEventHandlerFactory::RegisterHandlers(::std::vector<::std::unique_ptr<GroupEventHandler>> const& handlers, BotAI* botAI)
{
    if (!botAI)
        return;

    for (auto const& handler : handlers)
    {
        auto eventTypes = handler->GetSubscribedEvents();
        EventBus<GroupEvent>::instance()->Subscribe(botAI, eventTypes);

        TC_LOG_DEBUG("playerbot.group", "Registered handler '{}' for {} event types",
            handler->GetHandlerName(), eventTypes.size());
    }

    TC_LOG_INFO("playerbot.group", "Registered {} event handlers for bot", handlers.size());
}

void GroupEventHandlerFactory::UnregisterHandlers(BotAI* botAI)
{
    if (!botAI)
        return;

    EventBus<GroupEvent>::instance()->Unsubscribe(botAI);

    TC_LOG_INFO("playerbot.group", "Unregistered all event handlers for bot");
}

} // namespace Playerbot
