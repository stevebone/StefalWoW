/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "GuildEventCoordinator.h"
#include "CalendarMgr.h"
#include "GuildMgr.h"
#include "GroupMgr.h"
#include "Chat.h"
#include "World.h"
#include "Log.h"
#include "GameTime.h"
#include "ObjectMgr.h"
#include "DatabaseEnv.h"
#include "ObjectAccessor.h"
#include "Loot.h"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>

namespace Playerbot
{

GuildEventCoordinator::GuildEventCoordinator(Player* bot) : _bot(bot)
{
    if (!_bot)
    {
        TC_LOG_ERROR("playerbot", "GuildEventCoordinator: null bot!");
        return;
    }

    // Initialize event templates
    InitializeEventTemplates();

    // Load existing guild event data if bot is in a guild
    if (Guild* guild = _bot->GetGuild())
    {
        LoadGuildEventData(guild->GetId());
    }

    // Setup player profile
    _playerProfiles[_bot->GetGUID().GetCounter()] = EventCoordinationProfile();
    _playerParticipation[_bot->GetGUID().GetCounter()] = EventParticipation(_bot->GetGUID().GetCounter(),
        _bot->GetGuildId());

    _globalMetrics.Reset();
}

GuildEventCoordinator::~GuildEventCoordinator()
{
    // Clean up any ongoing events the bot was organizing
    for (const auto& [eventId, event] : _guildEvents)
    {
        if (event.organizerGuid == _bot->GetGUID().GetCounter() &&
            event.status == EventStatus::IN_PROGRESS)
        {
            HandleEventCompletion(eventId);
        }
    }
}

uint32 GuildEventCoordinator::CreateGuildEvent(Player* organizer, const GuildEvent& eventData)
{
    if (!organizer || !organizer->GetGuildId())
        return 0;

    // Validate event data
    if (!ValidateEventData(eventData))
    {
        TC_LOG_ERROR("playerbot", "GuildEventCoordinator::CreateGuildEvent - Invalid event data");
        return 0;
    }

    // Create new event with unique ID
    uint32 eventId = _nextEventId.fetch_add(1);
    GuildEvent newEvent = eventData;
    newEvent.eventId = eventId;
    newEvent.guildId = organizer->GetGuildId();
    newEvent.organizerGuid = organizer->GetGUID().GetCounter();
    newEvent.organizerName = organizer->GetName();
    newEvent.status = EventStatus::PLANNING;
    newEvent.creationTime = GameTime::GetGameTimeMS();
    newEvent.lastUpdateTime = newEvent.creationTime;

    // Store the event
    _guildEvents[eventId] = newEvent;
    _guildActiveEvents[newEvent.guildId].push_back(eventId);

    // Create calendar event if scheduled
    if (newEvent.scheduledTime > 0)
    {
        UpdateEventCalendar(eventId);
    }

    // Update metrics
    _globalMetrics.eventsCreated++;
    _guildMetrics[newEvent.guildId].eventsCreated++;
    _playerParticipation[organizer->GetGUID().GetCounter()].totalEventsCreated++;
    _playerParticipation[organizer->GetGUID().GetCounter()].organizedEvents++;

    // Send notifications
    BroadcastEventAnnouncement(eventId);

    TC_LOG_DEBUG("playerbot", "GuildEventCoordinator: Created event %u '%s' for guild %u",
        eventId, newEvent.eventTitle.c_str(), newEvent.guildId);

    return eventId;
}

bool GuildEventCoordinator::UpdateGuildEvent(uint32 eventId, const GuildEvent& updatedData)
{
    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return false;

    GuildEvent& event = it->second;

    // Validate the updated data
    if (!ValidateEventData(updatedData))
        return false;

    // Preserve critical fields
    uint32 originalId = event.eventId;
    uint32 originalGuildId = event.guildId;
    uint32 originalOrganizerGuid = event.organizerGuid;
    uint32 originalCreationTime = event.creationTime;

    // Update event data
    event = updatedData;
    event.eventId = originalId;
    event.guildId = originalGuildId;
    event.organizerGuid = originalOrganizerGuid;
    event.creationTime = originalCreationTime;
    event.lastUpdateTime = GameTime::GetGameTimeMS();

    // Update calendar if scheduled time changed
    if (event.scheduledTime != updatedData.scheduledTime)
    {
        UpdateEventCalendar(eventId);
    }

    // Notify participants of changes
    NotifyEventChanges(eventId);

    TC_LOG_DEBUG("playerbot", "GuildEventCoordinator: Updated event %u", eventId);
    return true;
}

bool GuildEventCoordinator::CancelGuildEvent(Player* organizer, uint32 eventId)
{
    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return false;

    GuildEvent& event = it->second;

    // Verify organizer has permission (simplified - just check if they're in the same guild)
    if (organizer && event.organizerGuid != organizer->GetGUID().GetCounter())
    {
        // Check if player is in the guild
        if (!organizer->GetGuild() || organizer->GetGuildId() != event.guildId)
            return false;
    }

    // Update status
    event.status = EventStatus::CANCELLED;
    event.lastUpdateTime = GameTime::GetGameTimeMS();

    // Remove from active events
    auto& guildEvents = _guildActiveEvents[event.guildId];
    guildEvents.erase(std::remove(guildEvents.begin(), guildEvents.end(), eventId), guildEvents.end());

    // Cancel calendar event
    if (CalendarEvent* calEvent = sCalendarMgr->GetEvent(eventId))
    {
        sCalendarMgr->RemoveEvent(eventId, organizer ? organizer->GetGUID() : ObjectGuid::Empty);
    }

    // Update metrics
    _globalMetrics.eventsCancelled++;
    _guildMetrics[event.guildId].eventsCancelled++;

    // Notify participants
    BroadcastEventUpdates(eventId, "Event has been cancelled");

    TC_LOG_DEBUG("playerbot", "GuildEventCoordinator: Cancelled event %u", eventId);
    return true;
}

void GuildEventCoordinator::ProcessEventInvitations(uint32 eventId)
{
    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    GuildEvent& event = it->second;

    // Get guild members
    Guild* guild = sGuildMgr->GetGuildById(event.guildId);
    if (!guild)
        return;

    // Process each invited member
    for (uint32 memberGuid : event.invitedMembers)
    {
        Player* member = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(memberGuid));
        if (!member)
            continue;

        // Check member's event profile
        auto profileIt = _playerProfiles.find(memberGuid);
        if (profileIt != _playerProfiles.end())
        {
            EventCoordinationProfile& profile = profileIt->second;

            // Auto-accept if configured
            if (profile.autoAcceptInvitations && profile.enableEventParticipation)
            {
                // Check if event type is preferred
                auto typeIt = std::find(profile.preferredEventTypes.begin(),
                    profile.preferredEventTypes.end(), event.eventType);

                if (typeIt != profile.preferredEventTypes.end())
                {
                    // Confirm attendance
                    event.confirmedMembers.push_back(memberGuid);
                    auto inviteIt = std::find(event.invitedMembers.begin(),
                        event.invitedMembers.end(), memberGuid);
                    if (inviteIt != event.invitedMembers.end())
                        event.invitedMembers.erase(inviteIt);

                    // Send confirmation
                    if (member->GetSession())
                        ChatHandler(member->GetSession()).PSendSysMessage("You have confirmed attendance for event: %s",
                            event.eventTitle.c_str());
                }
                else
                {
                    // Decline if not preferred
                    event.declinedMembers.push_back(memberGuid);
                    auto inviteIt = std::find(event.invitedMembers.begin(),
                        event.invitedMembers.end(), memberGuid);
                    if (inviteIt != event.invitedMembers.end())
                        event.invitedMembers.erase(inviteIt);
                }
            }
        }
    }

    event.lastUpdateTime = GameTime::GetGameTimeMS();
}

void GuildEventCoordinator::PlanGuildEvents()
{
    if (!_bot || !_bot->GetGuildId())
        return;

    uint32 guildId = _bot->GetGuildId();
    Guild* guild = sGuildMgr->GetGuildById(guildId);
    if (!guild)
        return;

    // Check if we should plan new events
    auto& activeEvents = _guildActiveEvents[guildId];
    if (activeEvents.size() >= MAX_EVENTS_PER_GUILD)
        return;

    // Analyze guild patterns to determine best event types
    std::vector<GuildEventType> recommendedTypes = RecommendEventTypes();

    // Create events based on recommendations
    for (GuildEventType eventType : recommendedTypes)
    {
        if (activeEvents.size() >= MAX_EVENTS_PER_GUILD)
            break;

        // Find optimal time for this event type
        uint32 optimalTime = FindOptimalEventTime(guildId, eventType,
            eventType == GuildEventType::RAID_DUNGEON ? 10800000 : 7200000); // 3h for raids, 2h otherwise

        if (optimalTime == 0)
            continue;

        // Create event
        GuildEvent newEvent;
        newEvent.guildId = guildId;
        newEvent.eventType = eventType;
        newEvent.scheduledTime = optimalTime;

        // Set event details based on type
        switch (eventType)
        {
            case GuildEventType::RAID_DUNGEON:
                newEvent.eventTitle = "Guild Raid Night";
                newEvent.eventDescription = "Weekly raid progression and gear farming";
                newEvent.maxParticipants = 25;
                newEvent.minParticipants = 10;
                newEvent.duration = 10800000; // 3 hours
                newEvent.priority = EventPriority::HIGH;
                break;
            case GuildEventType::PVP_BATTLEGROUND:
                newEvent.eventTitle = "PvP Battleground Night";
                newEvent.eventDescription = "Organized battleground group for honor and fun";
                newEvent.maxParticipants = 15;
                newEvent.minParticipants = 10;
                newEvent.duration = 7200000; // 2 hours
                newEvent.priority = EventPriority::NORMAL;
                break;
            case GuildEventType::LEVELING_GROUP:
                newEvent.eventTitle = "Alt Leveling Group";
                newEvent.eventDescription = "Help guild members level their alts";
                newEvent.maxParticipants = 5;
                newEvent.minParticipants = 3;
                newEvent.duration = 7200000; // 2 hours
                newEvent.priority = EventPriority::LOW;
                break;
            case GuildEventType::SOCIAL_GATHERING:
                newEvent.eventTitle = "Guild Social Hour";
                newEvent.eventDescription = "Casual hangout and guild bonding";
                newEvent.maxParticipants = 40;
                newEvent.minParticipants = 5;
                newEvent.duration = 3600000; // 1 hour
                newEvent.priority = EventPriority::LOW;
                break;
            default:
                newEvent.eventTitle = "Guild Event";
                newEvent.eventDescription = "Guild organized activity";
                newEvent.maxParticipants = 20;
                newEvent.minParticipants = 5;
                newEvent.duration = 7200000; // 2 hours
                newEvent.priority = EventPriority::NORMAL;
                break;
        }

        // Create the event
        uint32 eventId = CreateGuildEvent(_bot, newEvent);
        if (eventId > 0)
        {
            TC_LOG_DEBUG("playerbot", "GuildEventCoordinator: Planned event %u type %u for guild %u",
                eventId, (uint32)eventType, guildId);
        }
    }
}

void GuildEventCoordinator::ScheduleRecurringEvents()
{
    if (!_bot || !_bot->GetGuildId())
        return;

    uint32 guildId = _bot->GetGuildId();

    // Find events marked as recurring
    std::vector<uint32> recurringEvents;
    for (const auto& [eventId, event] : _guildEvents)
    {
        if (event.guildId == guildId && event.isRecurring &&
            event.status == EventStatus::COMPLETED)
        {
            recurringEvents.push_back(eventId);
        }
    }

    // Schedule next occurrence for each recurring event
    for (uint32 eventId : recurringEvents)
    {
        auto it = _guildEvents.find(eventId);
        if (it == _guildEvents.end())
            continue;

        const GuildEvent& originalEvent = it->second;

        // Check if we haven't exceeded max recurring events
        if (_guildActiveEvents[guildId].size() >= MAX_RECURRING_EVENTS)
            break;

        // Create new occurrence
        GuildEvent newEvent = originalEvent;
        newEvent.scheduledTime = originalEvent.scheduledTime + originalEvent.recurringInterval;
        newEvent.status = EventStatus::PLANNING;
        newEvent.confirmedMembers.clear();
        newEvent.declinedMembers.clear();
        newEvent.invitedMembers = originalEvent.confirmedMembers; // Re-invite previous attendees

        uint32 newEventId = CreateGuildEvent(_bot, newEvent);
        if (newEventId > 0)
        {
            TC_LOG_DEBUG("playerbot", "GuildEventCoordinator: Scheduled recurring event %u from %u",
                newEventId, eventId);
        }
    }
}

void GuildEventCoordinator::RecruitEventParticipants(Player* organizer, uint32 eventId)
{
    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    GuildEvent& event = it->second;

    // Update status to recruiting
    if (event.status == EventStatus::PLANNING)
    {
        UpdateEventStatus(eventId, EventStatus::RECRUITING);
    }

    Guild* guild = sGuildMgr->GetGuildById(event.guildId);
    if (!guild)
        return;

    // Get all guild members
    std::unordered_map<ObjectGuid, Guild::Member> const& members = guild->GetMembers();

    // Evaluate each member for invitation
    for (const auto& [guid, member] : members)
    {
        Player* player = ObjectAccessor::FindPlayer(guid);
        if (!player || player->GetGUID().GetCounter() == event.organizerGuid)
            continue;

        // Check if already invited/confirmed/declined
        uint32 memberGuid = player->GetGUID().GetCounter();
        if (std::find(event.invitedMembers.begin(), event.invitedMembers.end(), memberGuid) != event.invitedMembers.end() ||
            std::find(event.confirmedMembers.begin(), event.confirmedMembers.end(), memberGuid) != event.confirmedMembers.end() ||
            std::find(event.declinedMembers.begin(), event.declinedMembers.end(), memberGuid) != event.declinedMembers.end())
            continue;

        // Check member's profile and preferences
        bool shouldInvite = false;
        auto profileIt = _playerProfiles.find(memberGuid);
        if (profileIt != _playerProfiles.end())
        {
            const EventCoordinationProfile& profile = profileIt->second;

            // Check if member wants to participate in events
            if (profile.enableEventParticipation)
            {
                // Check if event type is preferred
                auto typeIt = std::find(profile.preferredEventTypes.begin(),
                    profile.preferredEventTypes.end(), event.eventType);
                if (typeIt != profile.preferredEventTypes.end())
                {
                    shouldInvite = true;
                }
                else
                {
                    // Random chance based on participation rate
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::uniform_real_distribution<> dis(0.0, 1.0);
                    if (dis(gen) < profile.participationRate)
                        shouldInvite = true;
                }
            }
        }
        else
        {
            // No profile, invite based on event type
            shouldInvite = true; // Default to inviting
        }

        if (shouldInvite && event.invitedMembers.size() < event.maxParticipants)
        {
            event.invitedMembers.push_back(memberGuid);

            // Send invitation notification
            if (player->GetSession())
                ChatHandler(player->GetSession()).PSendSysMessage("You have been invited to guild event: %s",
                    event.eventTitle.c_str());
        }
    }

    event.lastUpdateTime = GameTime::GetGameTimeMS();
    ProcessEventInvitations(eventId);
}

void GuildEventCoordinator::ManageEventSignups(uint32 eventId)
{
    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    GuildEvent& event = it->second;

    // Move confirmed members from invited list
    auto inviteIt = event.invitedMembers.begin();
    while (inviteIt != event.invitedMembers.end())
    {
        uint32 memberGuid = *inviteIt;

        // Check if member has responded (this would normally check calendar responses)
        // For now, simulate responses based on profiles
        auto profileIt = _playerProfiles.find(memberGuid);
        if (profileIt != _playerProfiles.end())
        {
            const EventCoordinationProfile& profile = profileIt->second;

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> dis(0.0, 1.0);

            if (dis(gen) < profile.participationRate)
            {
                event.confirmedMembers.push_back(memberGuid);
                inviteIt = event.invitedMembers.erase(inviteIt);
                continue;
            }
        }
        ++inviteIt;
    }

    // Check if we have minimum participants
    if (event.confirmedMembers.size() >= event.minParticipants)
    {
        if (event.status == EventStatus::RECRUITING)
        {
            UpdateEventStatus(eventId, EventStatus::CONFIRMED);
        }
    }
    else if (event.scheduledTime - GameTime::GetGameTimeMS() < 3600000) // Less than 1 hour until event
    {
        // Not enough participants, consider cancelling
        if (event.confirmedMembers.size() < event.minParticipants / 2)
        {
            CancelGuildEvent(nullptr, eventId);
        }
    }

    event.lastUpdateTime = GameTime::GetGameTimeMS();
}

void GuildEventCoordinator::AssignEventRoles(uint32 eventId)
{
    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    GuildEvent& event = it->second;
    event.memberRoles.clear();

    // Assign roles based on event type
    switch (event.eventType)
    {
        case GuildEventType::RAID_DUNGEON:
        {
            // Need tanks, healers, DPS
            uint32 tanksNeeded = 2;
            uint32 healersNeeded = std::min((uint32)event.confirmedMembers.size() / 5, 5u);
            uint32 tanksAssigned = 0;
            uint32 healersAssigned = 0;

            for (uint32 memberGuid : event.confirmedMembers)
            {
                Player* member = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(memberGuid));
                if (!member)
                    continue;

                // Assign based on class (simplified)
                switch (member->GetClass())
                {
                    case CLASS_WARRIOR:
                    case CLASS_PALADIN:
                        if (tanksAssigned < tanksNeeded)
                        {
                            event.memberRoles[memberGuid] = "Tank";
                            tanksAssigned++;
                        }
                        else
                            event.memberRoles[memberGuid] = "DPS";
                        break;
                    case CLASS_PRIEST:
                    case CLASS_DRUID:
                    case CLASS_SHAMAN:
                        if (healersAssigned < healersNeeded)
                        {
                            event.memberRoles[memberGuid] = "Healer";
                            healersAssigned++;
                        }
                        else
                            event.memberRoles[memberGuid] = "DPS";
                        break;
                    default:
                        event.memberRoles[memberGuid] = "DPS";
                        break;
                }
            }
            break;
        }
        case GuildEventType::PVP_BATTLEGROUND:
        case GuildEventType::PVP_ARENA:
        {
            // Assign PvP roles
            for (uint32 memberGuid : event.confirmedMembers)
            {
                Player* member = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(memberGuid));
                if (!member)
                    continue;

                // Simplified PvP role assignment
                switch (member->GetClass())
                {
                    case CLASS_ROGUE:
                    case CLASS_DRUID:
                        event.memberRoles[memberGuid] = "Flag Carrier";
                        break;
                    case CLASS_WARRIOR:
                    case CLASS_PALADIN:
                        event.memberRoles[memberGuid] = "Defender";
                        break;
                    case CLASS_MAGE:
                    case CLASS_WARLOCK:
                    case CLASS_HUNTER:
                        event.memberRoles[memberGuid] = "Damage";
                        break;
                    case CLASS_PRIEST:
                    case CLASS_SHAMAN:
                        event.memberRoles[memberGuid] = "Support";
                        break;
                    default:
                        event.memberRoles[memberGuid] = "Flex";
                        break;
                }
            }
            break;
        }
        default:
            // General roles
            for (uint32 memberGuid : event.confirmedMembers)
            {
                if (memberGuid == event.organizerGuid)
                    event.memberRoles[memberGuid] = "Organizer";
                else
                    event.memberRoles[memberGuid] = "Participant";
            }
            break;
    }

    event.lastUpdateTime = GameTime::GetGameTimeMS();
}

void GuildEventCoordinator::ExecuteGuildEvent(uint32 eventId)
{
    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    GuildEvent& event = it->second;

    // Check if it's time to start
    if (GameTime::GetGameTimeMS() < event.scheduledTime)
        return;

    // Update status
    UpdateEventStatus(eventId, EventStatus::IN_PROGRESS);

    // Form group if needed
    Group* eventGroup = FormEventGroup(eventId);

    // Execute based on event type
    switch (event.eventType)
    {
        case GuildEventType::RAID_DUNGEON:
            if (Player* leader = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(event.organizerGuid)))
                CoordinateRaidEvent(leader, eventId);
            break;
        case GuildEventType::PVP_BATTLEGROUND:
        case GuildEventType::PVP_ARENA:
            if (Player* leader = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(event.organizerGuid)))
                CoordinatePvPEvent(leader, eventId);
            break;
        case GuildEventType::SOCIAL_GATHERING:
            if (Player* leader = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(event.organizerGuid)))
                CoordinateSocialEvent(leader, eventId);
            break;
        case GuildEventType::LEVELING_GROUP:
            if (Player* leader = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(event.organizerGuid)))
                CoordinateLevelingEvent(leader, eventId);
            break;
        default:
            break;
    }

    // Update metrics
    _guildMetrics[event.guildId].totalParticipants += static_cast<uint32>(event.confirmedMembers.size());

    // Update participant tracking
    for (uint32 memberGuid : event.confirmedMembers)
    {
        _playerParticipation[memberGuid].participatedEvents++;
        _playerParticipation[memberGuid].totalEventsAttended++;
        _playerParticipation[memberGuid].lastEventActivity = GameTime::GetGameTimeMS();
    }

    event.lastUpdateTime = GameTime::GetGameTimeMS();
}

void GuildEventCoordinator::CoordinateEventActivities(Player* leader, uint32 eventId)
{
    if (!leader)
        return;

    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    GuildEvent& event = it->second;

    // Coordinate based on event type
    switch (event.eventType)
    {
        case GuildEventType::RAID_DUNGEON:
            CoordinateRaidEvent(leader, eventId);
            break;
        case GuildEventType::PVP_BATTLEGROUND:
        case GuildEventType::PVP_ARENA:
            CoordinatePvPEvent(leader, eventId);
            break;
        case GuildEventType::SOCIAL_GATHERING:
            CoordinateSocialEvent(leader, eventId);
            break;
        case GuildEventType::LEVELING_GROUP:
            CoordinateLevelingEvent(leader, eventId);
            break;
        default:
            // Generic coordination
            HandleEventLogistics(eventId);
            break;
    }

    event.lastUpdateTime = GameTime::GetGameTimeMS();
}

void GuildEventCoordinator::MonitorEventProgress(uint32 eventId)
{
    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    GuildEvent& event = it->second;

    if (event.status != EventStatus::IN_PROGRESS)
        return;

    // Check event health
    MonitorEventHealth(eventId);

    // Check if event duration has passed
    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime >= event.scheduledTime + event.duration)
    {
        HandleEventCompletion(eventId);
        return;
    }

    // Monitor participant status
    uint32 activeParticipants = 0;
    for (uint32 memberGuid : event.confirmedMembers)
    {
        Player* member = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(memberGuid));
        if (member && member->IsInWorld())
            activeParticipants++;
    }

    // Check if too many participants have left
    if (activeParticipants < event.minParticipants / 2)
    {
        // Event is failing, end it
        UpdateEventStatus(eventId, EventStatus::CANCELLED);
        BroadcastEventUpdates(eventId, "Event ended due to insufficient participants");
    }

    event.lastUpdateTime = GameTime::GetGameTimeMS();
}

void GuildEventCoordinator::HandleEventCompletion(uint32 eventId)
{
    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    GuildEvent& event = it->second;

    // Update status
    UpdateEventStatus(eventId, EventStatus::COMPLETED);

    // Calculate success metrics
    float attendanceRate = (float)event.confirmedMembers.size() / (float)event.maxParticipants;
    bool wasSuccessful = attendanceRate >= MIN_ATTENDANCE_RATE;

    // Update metrics
    UpdateEventMetrics(eventId, wasSuccessful);
    _globalMetrics.eventsCompleted++;
    _guildMetrics[event.guildId].eventsCompleted++;

    // Update participant ratings
    for (uint32 memberGuid : event.confirmedMembers)
    {
        auto& participation = _playerParticipation[memberGuid];
        participation.participationRating = std::min(1.0f, participation.participationRating + 0.05f);

        // Update event type preferences - add event type if not already present
        auto& prefs = participation.eventTypePreferences;
        if (std::find(prefs.begin(), prefs.end(), event.eventType) == prefs.end())
            prefs.push_back(event.eventType);
    }

    // Update organizer rating
    auto& organizerParticipation = _playerParticipation[event.organizerGuid];
    if (wasSuccessful)
        organizerParticipation.organizationRating = std::min(1.0f, organizerParticipation.organizationRating + 0.1f);
    else
        organizerParticipation.organizationRating = std::max(0.0f, organizerParticipation.organizationRating - 0.05f);

    // Schedule next occurrence if recurring
    if (event.isRecurring && wasSuccessful)
    {
        GuildEvent recurringEvent = event;
        recurringEvent.scheduledTime += event.recurringInterval;
        recurringEvent.status = EventStatus::PLANNING;
        recurringEvent.confirmedMembers.clear();
        recurringEvent.declinedMembers.clear();
        recurringEvent.invitedMembers = event.confirmedMembers;

        CreateGuildEvent(_bot, recurringEvent);
    }

    // Remove from active events
    auto& guildEvents = _guildActiveEvents[event.guildId];
    guildEvents.erase(std::remove(guildEvents.begin(), guildEvents.end(), eventId), guildEvents.end());

    // Send completion message
    BroadcastEventUpdates(eventId, wasSuccessful ? "Event completed successfully!" : "Event completed.");

    event.lastUpdateTime = GameTime::GetGameTimeMS();
}

void GuildEventCoordinator::SetEventProfile(const EventCoordinationProfile& profile)
{
    if (!_bot)
        return;

    _playerProfiles[_bot->GetGUID().GetCounter()] = profile;

    TC_LOG_DEBUG("playerbot", "GuildEventCoordinator: Updated event profile for player %u",
        _bot->GetGUID().GetCounter());
}

EventCoordinationProfile GuildEventCoordinator::GetEventProfile()
{
    if (!_bot)
        return EventCoordinationProfile();

    auto it = _playerProfiles.find(_bot->GetGUID().GetCounter());
    if (it != _playerProfiles.end())
        return it->second;

    return EventCoordinationProfile();
}

EventParticipation GuildEventCoordinator::GetEventParticipation()
{
    if (!_bot)
        return EventParticipation(0, 0);

    auto it = _playerParticipation.find(_bot->GetGUID().GetCounter());
    if (it != _playerParticipation.end())
        return it->second;

    return EventParticipation(_bot->GetGUID().GetCounter(), _bot->GetGuildId());
}

void GuildEventCoordinator::OptimizeEventScheduling()
{
    if (!_bot || !_bot->GetGuildId())
        return;

    uint32 guildId = _bot->GetGuildId();
    OptimizeEventScheduling(guildId);
}

void GuildEventCoordinator::OptimizeEventScheduling(uint32 guildId)
{
    // Analyze guild member activity patterns
    Guild* guild = sGuildMgr->GetGuildById(guildId);
    if (!guild)
        return;

    auto& scheduler = _guildSchedulers[guildId];
    scheduler.guildId = guildId;

    // Collect member availability data
    std::unordered_map<ObjectGuid, Guild::Member> const& members = guild->GetMembers();
    for (const auto& [guid, member] : members)
    {
        uint32 memberGuid = guid.GetCounter();
        auto profileIt = _playerProfiles.find(memberGuid);
        if (profileIt != _playerProfiles.end())
        {
            const EventCoordinationProfile& profile = profileIt->second;

            // Calculate availability score based on profile
            float availabilityScore = profile.participationRate;
            if (profile.enableEventParticipation)
                availabilityScore *= 1.5f;
            if (profile.enableEventLeadership)
                availabilityScore *= 2.0f;

            scheduler.memberAvailability[memberGuid] = availabilityScore;
        }
        else
        {
            scheduler.memberAvailability[memberGuid] = 0.5f; // Default availability
        }
    }

    // Identify popular times based on past events
    std::map<uint32, uint32> timeSlotPopularity; // hour of week -> count
    for (const auto& [eventId, event] : _guildEvents)
    {
        if (event.guildId == guildId && event.status == EventStatus::COMPLETED)
        {
            // Calculate hour of week (0-167)
            uint32 hourOfWeek = (event.scheduledTime / 3600000) % 168;
            timeSlotPopularity[hourOfWeek]++;
        }
    }

    // Find top popular times
    scheduler.popularTimes.clear();
    for (const auto& [hourOfWeek, count] : timeSlotPopularity)
    {
        if (count >= 2) // At least 2 successful events at this time
        {
            uint32 startTime = hourOfWeek * 3600000;
            uint32 endTime = startTime + 3600000;
            scheduler.popularTimes.push_back({startTime, endTime});
        }
    }

    // Calculate average attendance rate
    uint32 totalAttendance = 0;
    uint32 eventCount = 0;
    for (const auto& [eventId, event] : _guildEvents)
    {
        if (event.guildId == guildId && event.status == EventStatus::COMPLETED)
        {
            totalAttendance += event.confirmedMembers.size();
            eventCount++;
        }
    }

    if (eventCount > 0)
        scheduler.averageAttendanceRate = (float)totalAttendance / (float)(eventCount * 20); // Assume 20 target
    else
        scheduler.averageAttendanceRate = 0.75f;

    // Find conflicting events
    scheduler.conflictingEvents.clear();
    uint32 currentTime = GameTime::GetGameTimeMS();
    for (const auto& [eventId1, event1] : _guildEvents)
    {
        if (event1.guildId != guildId || event1.scheduledTime < currentTime)
            continue;

        for (const auto& [eventId2, event2] : _guildEvents)
        {
            if (eventId1 >= eventId2 || event2.guildId != guildId || event2.scheduledTime < currentTime)
                continue;

            // Check for time overlap
            uint32 start1 = event1.scheduledTime;
            uint32 end1 = start1 + event1.duration;
            uint32 start2 = event2.scheduledTime;
            uint32 end2 = start2 + event2.duration;

            if ((start1 <= start2 && end1 > start2) || (start2 <= start1 && end2 > start1))
            {
                scheduler.conflictingEvents.push_back(eventId1);
                scheduler.conflictingEvents.push_back(eventId2);
            }
        }
    }

    // Resolve conflicts
    if (!scheduler.conflictingEvents.empty())
    {
        RescheduleConflictingEvents();
    }
}

void GuildEventCoordinator::AnalyzeGuildEventPatterns(uint32 guildId)
{
    Guild* guild = sGuildMgr->GetGuildById(guildId);
    if (!guild)
        return;

    // Analyze event type success rates
    std::map<GuildEventType, std::pair<uint32, uint32>> typeSuccess; // type -> (successful, total)

    for (const auto& [eventId, event] : _guildEvents)
    {
        if (event.guildId != guildId)
            continue;

        auto& [successful, total] = typeSuccess[event.eventType];
        total++;

        if (event.status == EventStatus::COMPLETED)
        {
            float attendanceRate = (float)event.confirmedMembers.size() / (float)event.maxParticipants;
            if (attendanceRate >= MIN_ATTENDANCE_RATE)
                successful++;
        }
    }

    // Update guild scheduler with analysis results
    auto& scheduler = _guildSchedulers[guildId];

    // Find most successful event types
    GuildEventType bestType = GuildEventType::SOCIAL_GATHERING;
    float bestSuccessRate = 0.0f;

    for (const auto& [type, stats] : typeSuccess)
    {
        const auto& [successful, total] = stats;
        if (total > 0)
        {
            float successRate = (float)successful / (float)total;
            if (successRate > bestSuccessRate)
            {
                bestSuccessRate = successRate;
                bestType = type;
            }
        }
    }

    // Calculate optimal event duration based on completion rates
    uint32 totalDuration = 0;
    uint32 completedEvents = 0;

    for (const auto& [eventId, event] : _guildEvents)
    {
        if (event.guildId == guildId && event.status == EventStatus::COMPLETED)
        {
            totalDuration += event.duration;
            completedEvents++;
        }
    }

    if (completedEvents > 0)
        scheduler.optimalEventDuration = totalDuration / completedEvents;
    else
        scheduler.optimalEventDuration = 7200000; // Default 2 hours

    TC_LOG_DEBUG("playerbot", "GuildEventCoordinator: Analyzed patterns for guild %u - Best type: %u, Optimal duration: %u",
        guildId, (uint32)bestType, scheduler.optimalEventDuration);
}

void GuildEventCoordinator::BroadcastEventUpdates(uint32 eventId, const std::string& updateMessage)
{
    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    const GuildEvent& event = it->second;
    Guild* guild = sGuildMgr->GetGuildById(event.guildId);
    if (!guild)
        return;

    // Prepare full message
    std::stringstream ss;
    ss << "[Guild Event] " << event.eventTitle << ": " << updateMessage;
    std::string message = ss.str();

    // Send to all event participants
    for (uint32 memberGuid : event.confirmedMembers)
    {
        Player* member = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(memberGuid));
        if (member && member->GetSession())
            ChatHandler(member->GetSession()).PSendSysMessage("%s", message.c_str());
    }

    // Also send to invited members
    for (uint32 memberGuid : event.invitedMembers)
    {
        Player* member = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(memberGuid));
        if (member && member->GetSession())
            ChatHandler(member->GetSession()).PSendSysMessage("%s", message.c_str());
    }

    // Log the update
    LogEventActivity(eventId, updateMessage);
}

void GuildEventCoordinator::SendEventReminders(uint32 eventId)
{
    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    const GuildEvent& event = it->second;

    if (event.status == EventStatus::CANCELLED || event.status == EventStatus::COMPLETED)
        return;

    uint32 currentTime = GameTime::GetGameTimeMS();
    uint32 timeUntilEvent = 0;

    if (event.scheduledTime > currentTime)
        timeUntilEvent = event.scheduledTime - currentTime;
    else
        return; // Event already started or passed

    // Determine reminder message based on time remaining
    std::string reminderMessage;
    bool shouldSend = false;

    for (uint32 reminderTime : EVENT_REMINDER_TIMES)
    {
        if (timeUntilEvent <= reminderTime && timeUntilEvent > reminderTime - 300000) // Within 5 min window
        {
            shouldSend = true;
            if (reminderTime == 86400000)
                reminderMessage = "Event starts in 24 hours!";
            else if (reminderTime == 3600000)
                reminderMessage = "Event starts in 1 hour!";
            else if (reminderTime == 1800000)
                reminderMessage = "Event starts in 30 minutes!";
            break;
        }
    }

    if (!shouldSend)
        return;

    // Send reminders to confirmed participants
    for (uint32 memberGuid : event.confirmedMembers)
    {
        Player* member = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(memberGuid));
        if (member && member->GetSession())
        {
            ChatHandler(member->GetSession()).PSendSysMessage("[Event Reminder] %s - %s",
                event.eventTitle.c_str(), reminderMessage.c_str());
        }
    }

    // Also remind invited members who haven't responded
    for (uint32 memberGuid : event.invitedMembers)
    {
        Player* member = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(memberGuid));
        if (member && member->GetSession())
        {
            ChatHandler(member->GetSession()).PSendSysMessage("[Event Invitation Reminder] %s - %s. Please respond!",
                event.eventTitle.c_str(), reminderMessage.c_str());
        }
    }
}

void GuildEventCoordinator::UpdateEventStatus(uint32 eventId, EventStatus newStatus)
{
    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    GuildEvent& event = it->second;
    EventStatus oldStatus = event.status;
    event.status = newStatus;
    event.lastUpdateTime = GameTime::GetGameTimeMS();

    // Send status update notification
    std::string statusMessage;
    switch (newStatus)
    {
        case EventStatus::PLANNING:
            statusMessage = "Event is being planned";
            break;
        case EventStatus::RECRUITING:
            statusMessage = "Event is now recruiting participants";
            break;
        case EventStatus::CONFIRMED:
            statusMessage = "Event is confirmed and will proceed as scheduled";
            break;
        case EventStatus::IN_PROGRESS:
            statusMessage = "Event has started!";
            break;
        case EventStatus::COMPLETED:
            statusMessage = "Event has completed";
            break;
        case EventStatus::CANCELLED:
            statusMessage = "Event has been cancelled";
            break;
        case EventStatus::POSTPONED:
            statusMessage = "Event has been postponed";
            break;
    }

    BroadcastEventUpdates(eventId, statusMessage);

    TC_LOG_DEBUG("playerbot", "GuildEventCoordinator: Event %u status changed from %u to %u",
        eventId, (uint32)oldStatus, (uint32)newStatus);
}

Group* GuildEventCoordinator::FormEventGroup(uint32 eventId)
{
    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return nullptr;

    const GuildEvent& event = it->second;

    if (event.confirmedMembers.empty())
        return nullptr;

    // Get organizer
    Player* leader = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(event.organizerGuid));
    if (!leader)
    {
        // Try to find another confirmed member to be leader
        for (uint32 memberGuid : event.confirmedMembers)
        {
            leader = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(memberGuid));
            if (leader)
                break;
        }
    }

    if (!leader)
        return nullptr;

    // Create or get existing group
    Group* group = leader->GetGroup();
    if (!group)
    {
        group = new Group();
        if (!group->Create(leader))
        {
            delete group;
            return nullptr;
        }
        sGroupMgr->AddGroup(group);
    }

    // Set group type based on event
    if (event.eventType == GuildEventType::RAID_DUNGEON && event.confirmedMembers.size() > 5)
    {
        group->ConvertToRaid();
    }

    // Add confirmed members to group
    for (uint32 memberGuid : event.confirmedMembers)
    {
        if (memberGuid == leader->GetGUID().GetCounter())
            continue;

        Player* member = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(memberGuid));
        if (member && !member->GetGroup())
        {
            group->AddMember(member);
        }
    }

    // Assign roles within the group
    AssignGroupRoles(group, eventId);

    TC_LOG_DEBUG("playerbot", "GuildEventCoordinator: Formed group for event %u with %u members",
        eventId, group->GetMembersCount());

    return group;
}

void GuildEventCoordinator::AssignGroupRoles(Group* group, uint32 eventId)
{
    if (!group)
        return;

    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    const GuildEvent& event = it->second;

    // Assign group roles based on event member roles
    for (const auto& [memberGuid, role] : event.memberRoles)
    {
        Player* member = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(memberGuid));
        if (!member || member->GetGroup() != group)
            continue;

        // Set role icons in raid
        if (group->isRaidGroup())
        {
            uint8 subgroup = group->GetMemberGroup(member->GetGUID());

            if (role == "Tank")
                group->SetGroupMemberFlag(member->GetGUID(), true, GroupMemberFlags::MEMBER_FLAG_MAINTANK);
            else if (role == "Healer" || role == "Support")
                group->SetGroupMemberFlag(member->GetGUID(), true, GroupMemberFlags::MEMBER_FLAG_MAINASSIST);
        }

        // Set leader/assistant based on roles
        if (role == "Organizer" && member->GetGUID() != group->GetLeaderGUID())
        {
            group->ChangeLeader(member->GetGUID());
        }
        else if (role == "Officer" || role == "Assistant")
        {
            group->SetGroupMemberFlag(member->GetGUID(), true, GroupMemberFlags::MEMBER_FLAG_ASSISTANT);
        }
    }
}

EventMetrics GuildEventCoordinator::GetGuildEventMetrics(uint32 guildId)
{
    auto it = _guildMetrics.find(guildId);
    if (it != _guildMetrics.end())
        return it->second;

    EventMetrics metrics;
    metrics.Reset();
    return metrics;
}

EventMetrics GuildEventCoordinator::GetPlayerEventMetrics()
{
    if (!_bot)
    {
        EventMetrics metrics;
        metrics.Reset();
        return metrics;
    }

    EventMetrics metrics;
    metrics.Reset();

    // Calculate player-specific metrics
    auto it = _playerParticipation.find(_bot->GetGUID().GetCounter());
    if (it != _playerParticipation.end())
    {
        const EventParticipation& participation = it->second;
        metrics.eventsCreated = participation.totalEventsCreated;
        metrics.eventsCompleted = participation.eventsAttended;  // Use attendance count as completed proxy
        metrics.totalParticipants = participation.totalEventsAttended;
        metrics.averageAttendance = participation.participationRating;
        metrics.organizationEfficiency = participation.organizationRating;
    }

    return metrics;
}

void GuildEventCoordinator::SetEventCoordinationEnabled(uint32 guildId, bool enabled)
{
    if (!enabled)
    {
        // Cancel all active events for this guild
        auto it = _guildActiveEvents.find(guildId);
        if (it != _guildActiveEvents.end())
        {
            for (uint32 eventId : it->second)
            {
                CancelGuildEvent(nullptr, eventId);
            }
            it->second.clear();
        }
    }

    TC_LOG_DEBUG("playerbot", "GuildEventCoordinator: Event coordination %s for guild %u",
        enabled ? "enabled" : "disabled", guildId);
}

void GuildEventCoordinator::Update(uint32 diff)
{
    static uint32 updateTimer = 0;
    static uint32 reminderTimer = 0;

    updateTimer += diff;
    reminderTimer += diff;

    // Update event states periodically
    if (updateTimer >= EVENT_UPDATE_INTERVAL)
    {
        UpdateEventStates();
        updateTimer = 0;
    }

    // Process reminders periodically
    if (reminderTimer >= REMINDER_CHECK_INTERVAL)
    {
        ProcessEventReminders();
        reminderTimer = 0;
    }

    // Check for events that should start
    uint32 currentTime = GameTime::GetGameTimeMS();
    for (auto& [eventId, event] : _guildEvents)
    {
        if (event.status == EventStatus::CONFIRMED &&
            currentTime >= event.scheduledTime)
        {
            ExecuteGuildEvent(eventId);
        }
        else if (event.status == EventStatus::IN_PROGRESS)
        {
            MonitorEventProgress(eventId);
        }
    }
}

void GuildEventCoordinator::UpdateEventStates()
{
    uint32 currentTime = GameTime::GetGameTimeMS();

    for (auto& [eventId, event] : _guildEvents)
    {
        switch (event.status)
        {
            case EventStatus::PLANNING:
                // Check if we should start recruiting
                if (event.scheduledTime > 0 &&
                    event.scheduledTime - currentTime <= PLANNING_ADVANCE_TIME)
                {
                    RecruitEventParticipants(_bot, eventId);
                }
                break;

            case EventStatus::RECRUITING:
                // Check signups periodically
                ManageEventSignups(eventId);
                break;

            case EventStatus::CONFIRMED:
                // Check if we should assign roles
                if (event.scheduledTime - currentTime <= 3600000 && // 1 hour before
                    event.memberRoles.empty())
                {
                    AssignEventRoles(eventId);
                }
                break;

            case EventStatus::IN_PROGRESS:
                // Monitor ongoing events
                MonitorEventProgress(eventId);
                break;

            default:
                break;
        }
    }

    // Clean up old events
    CleanupExpiredEvents();
}

void GuildEventCoordinator::CleanupExpiredEvents()
{
    uint32 currentTime = GameTime::GetGameTimeMS();
    std::vector<uint32> eventsToRemove;

    for (const auto& [eventId, event] : _guildEvents)
    {
        // Remove completed/cancelled events older than retention period
        if ((event.status == EventStatus::COMPLETED || event.status == EventStatus::CANCELLED) &&
            currentTime - event.lastUpdateTime > EVENT_HISTORY_RETENTION)
        {
            eventsToRemove.push_back(eventId);
        }
    }

    // Remove expired events
    for (uint32 eventId : eventsToRemove)
    {
        auto it = _guildEvents.find(eventId);
        if (it != _guildEvents.end())
        {
            // Remove from guild active events if still there
            auto& guildEvents = _guildActiveEvents[it->second.guildId];
            guildEvents.erase(std::remove(guildEvents.begin(), guildEvents.end(), eventId),
                guildEvents.end());

            _guildEvents.erase(it);
        }
    }

    if (!eventsToRemove.empty())
    {
        TC_LOG_DEBUG("playerbot", "GuildEventCoordinator: Cleaned up %zu expired events",
            eventsToRemove.size());
    }
}

// Helper function implementations

void GuildEventCoordinator::InitializeEventTemplates()
{
    // Raid templates
    GuildEvent raidTemplate;
    raidTemplate.eventType = GuildEventType::RAID_DUNGEON;
    raidTemplate.eventTitle = "Guild Raid Night";
    raidTemplate.eventDescription = "Weekly raid progression";
    raidTemplate.duration = 10800000; // 3 hours
    raidTemplate.maxParticipants = 25;
    raidTemplate.minParticipants = 10;
    raidTemplate.priority = EventPriority::HIGH;
    _eventTemplates["raid_night"] = raidTemplate;
    _typeTemplates[GuildEventType::RAID_DUNGEON].push_back("raid_night");

    // PvP templates
    GuildEvent pvpTemplate;
    pvpTemplate.eventType = GuildEventType::PVP_BATTLEGROUND;
    pvpTemplate.eventTitle = "PvP Battleground Night";
    pvpTemplate.eventDescription = "Organized battleground group";
    pvpTemplate.duration = 7200000; // 2 hours
    pvpTemplate.maxParticipants = 15;
    pvpTemplate.minParticipants = 10;
    pvpTemplate.priority = EventPriority::NORMAL;
    _eventTemplates["bg_night"] = pvpTemplate;
    _typeTemplates[GuildEventType::PVP_BATTLEGROUND].push_back("bg_night");

    // Social templates
    GuildEvent socialTemplate;
    socialTemplate.eventType = GuildEventType::SOCIAL_GATHERING;
    socialTemplate.eventTitle = "Guild Social Hour";
    socialTemplate.eventDescription = "Casual guild hangout";
    socialTemplate.duration = 3600000; // 1 hour
    socialTemplate.maxParticipants = 40;
    socialTemplate.minParticipants = 5;
    socialTemplate.priority = EventPriority::LOW;
    _eventTemplates["social_hour"] = socialTemplate;
    _typeTemplates[GuildEventType::SOCIAL_GATHERING].push_back("social_hour");

    // Leveling templates
    GuildEvent levelingTemplate;
    levelingTemplate.eventType = GuildEventType::LEVELING_GROUP;
    levelingTemplate.eventTitle = "Alt Leveling Group";
    levelingTemplate.eventDescription = "Help guild alts level";
    levelingTemplate.duration = 7200000; // 2 hours
    levelingTemplate.maxParticipants = 5;
    levelingTemplate.minParticipants = 3;
    levelingTemplate.priority = EventPriority::LOW;
    _eventTemplates["leveling_group"] = levelingTemplate;
    _typeTemplates[GuildEventType::LEVELING_GROUP].push_back("leveling_group");
}

void GuildEventCoordinator::LoadGuildEventData(uint32 guildId)
{
    // This would normally load from database
    // For now, just initialize empty structures
    _guildActiveEvents[guildId] = std::vector<uint32>();
    _guildMetrics[guildId].Reset();
    _guildSchedulers[guildId] = GuildEventScheduler(guildId);
}

bool GuildEventCoordinator::ValidateEventData(const GuildEvent& event)
{
    // Validate duration
    if (event.duration < MIN_EVENT_DURATION || event.duration > MAX_EVENT_DURATION)
        return false;

    // Validate participants
    if (event.minParticipants > event.maxParticipants)
        return false;

    if (event.maxParticipants == 0 || event.minParticipants == 0)
        return false;

    // Validate title and description
    if (event.eventTitle.empty())
        return false;

    return true;
}

void GuildEventCoordinator::UpdateEventCalendar(uint32 eventId)
{
    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    const GuildEvent& event = it->second;

    // Set flags based on event type
    uint32 flags = 0;
    if (event.eventType == GuildEventType::RAID_DUNGEON)
        flags |= CALENDAR_FLAG_INVITES_LOCKED;

    // Create calendar event with proper constructor
    CalendarEvent* calEvent = new CalendarEvent(
        eventId,                                            // uint64 eventId
        ObjectGuid::Create<HighGuid::Player>(event.organizerGuid), // ObjectGuid ownerGUID
        event.guildId,                                     // ObjectGuid::LowType guildId
        CalendarEventType::CALENDAR_TYPE_OTHER,           // CalendarEventType type
        -1,                                               // int32 textureId
        event.scheduledTime / 1000,                       // time_t date (convert ms to seconds)
        flags,                                            // uint32 flags
        event.eventTitle,                                 // std::string title
        event.eventDescription,                           // std::string description
        0                                                 // time_t lockDate
    );

    // Add to calendar
    sCalendarMgr->AddEvent(calEvent, CalendarSendEventType::CALENDAR_SENDTYPE_ADD);

    // Send invites to confirmed members
    for (uint32 memberGuid : event.confirmedMembers)
    {
        CalendarInvite* invite = new CalendarInvite(0, eventId,
            ObjectGuid::Create<HighGuid::Player>(memberGuid),
            ObjectGuid::Create<HighGuid::Player>(event.organizerGuid),
            GameTime::GetGameTime(), CalendarInviteStatus::CALENDAR_STATUS_CONFIRMED,
            CalendarModerationRank::CALENDAR_RANK_PLAYER, "");
        sCalendarMgr->AddInvite(calEvent, invite);
    }
}

uint32 GuildEventCoordinator::FindOptimalEventTime(uint32 guildId, GuildEventType eventType, uint32 duration)
{
    auto it = _guildSchedulers.find(guildId);
    if (it == _guildSchedulers.end())
        return GameTime::GetGameTimeMS() + 86400000; // Default to 24 hours from now

    const GuildEventScheduler& scheduler = it->second;

    // If we have popular times, use them
    if (!scheduler.popularTimes.empty())
    {
        // Pick a random popular time slot
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dis(0, scheduler.popularTimes.size() - 1);
        auto& timeSlot = scheduler.popularTimes[dis(gen)];

        // Adjust to next occurrence of this time
        uint32 currentTime = GameTime::GetGameTimeMS();
        uint32 weekInMs = 604800000; // 7 days
        uint32 proposedTime = timeSlot.first;

        while (proposedTime < currentTime)
            proposedTime += weekInMs;

        return proposedTime;
    }

    // Default scheduling based on event type
    uint32 baseTime = GameTime::GetGameTimeMS() + 86400000; // Start 24 hours from now

    switch (eventType)
    {
        case GuildEventType::RAID_DUNGEON:
            // Schedule for evening prime time (7 PM server time)
            baseTime += 68400000; // Add 19 hours to get to 7 PM next day
            break;
        case GuildEventType::PVP_BATTLEGROUND:
        case GuildEventType::PVP_ARENA:
            // Schedule for late evening (9 PM server time)
            baseTime += 75600000; // Add 21 hours to get to 9 PM next day
            break;
        case GuildEventType::SOCIAL_GATHERING:
            // Schedule for afternoon (3 PM server time)
            baseTime += 54000000; // Add 15 hours to get to 3 PM next day
            break;
        default:
            // Use base time
            break;
    }

    return baseTime;
}

std::vector<GuildEventType> GuildEventCoordinator::RecommendEventTypes()
{
    std::vector<GuildEventType> recommendations;

    if (!_bot || !_bot->GetGuildId())
        return recommendations;

    uint32 guildId = _bot->GetGuildId();

    // Analyze past events to determine preferences
    std::map<GuildEventType, float> typeScores;

    for (const auto& [eventId, event] : _guildEvents)
    {
        if (event.guildId != guildId)
            continue;

        float score = 0.0f;

        // Score based on completion
        if (event.status == EventStatus::COMPLETED)
        {
            float attendanceRate = (float)event.confirmedMembers.size() / (float)event.maxParticipants;
            score = attendanceRate;
        }
        else if (event.status == EventStatus::CANCELLED)
        {
            score = -0.5f;
        }

        typeScores[event.eventType] += score;
    }

    // Sort by score
    std::vector<std::pair<GuildEventType, float>> sortedTypes;
    for (const auto& [type, score] : typeScores)
    {
        sortedTypes.push_back({type, score});
    }

    std::sort(sortedTypes.begin(), sortedTypes.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    // Add top types
    for (const auto& [type, score] : sortedTypes)
    {
        if (score > 0)
            recommendations.push_back(type);
        if (recommendations.size() >= 3)
            break;
    }

    // If no history, recommend default types
    if (recommendations.empty())
    {
        recommendations.push_back(GuildEventType::RAID_DUNGEON);
        recommendations.push_back(GuildEventType::PVP_BATTLEGROUND);
        recommendations.push_back(GuildEventType::SOCIAL_GATHERING);
    }

    return recommendations;
}

void GuildEventCoordinator::CoordinateRaidEvent(Player* leader, uint32 eventId)
{
    if (!leader)
        return;

    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    const GuildEvent& event = it->second;

    // Form raid group if not already formed
    Group* group = leader->GetGroup();
    if (!group)
        group = FormEventGroup(eventId);

    if (!group || !group->isRaidGroup())
    {
        BroadcastEventUpdates(eventId, "Failed to form raid group");
        return;
    }

    // Assign raid targets and markers
    uint8 tankCount = 0;
    uint8 healerCount = 0;

    for (const auto& [memberGuid, role] : event.memberRoles)
    {
        if (role == "Tank")
            tankCount++;
        else if (role == "Healer")
            healerCount++;
    }

    // Send raid instructions
    std::stringstream instructions;
    instructions << "Raid composition: " << tankCount << " tanks, "
                 << healerCount << " healers, "
                 << (event.confirmedMembers.size() - tankCount - healerCount) << " DPS";
    BroadcastEventUpdates(eventId, instructions.str());

    // Set raid difficulty if applicable
    if (event.confirmedMembers.size() >= 20)
    {
        BroadcastEventUpdates(eventId, "Setting raid to 25-player mode");
    }
    else
    {
        BroadcastEventUpdates(eventId, "Setting raid to 10-player mode");
    }
}

void GuildEventCoordinator::CoordinatePvPEvent(Player* leader, uint32 eventId)
{
    if (!leader)
        return;

    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    const GuildEvent& event = it->second;

    // Form PvP group
    Group* group = leader->GetGroup();
    if (!group)
        group = FormEventGroup(eventId);

    if (!group)
    {
        BroadcastEventUpdates(eventId, "Failed to form PvP group");
        return;
    }

    // Queue for battleground or arena based on event type
    if (event.eventType == GuildEventType::PVP_BATTLEGROUND)
    {
        BroadcastEventUpdates(eventId, "Queueing for battleground as a group");
        // Note: Actual BG queueing would happen through BattlegroundMgr
    }
    else if (event.eventType == GuildEventType::PVP_ARENA)
    {
        uint32 teamSize = std::min((uint32)event.confirmedMembers.size(), 5u);
        std::stringstream msg;
        msg << "Forming " << teamSize << "v" << teamSize << " arena team";
        BroadcastEventUpdates(eventId, msg.str());
    }

    // Assign PvP strategies
    BroadcastEventUpdates(eventId, "PvP roles assigned - check your assignment!");
}

void GuildEventCoordinator::CoordinateSocialEvent(Player* leader, uint32 eventId)
{
    if (!leader)
        return;

    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    const GuildEvent& event = it->second;

    // Social events don't require formal groups
    BroadcastEventUpdates(eventId, "Social gathering has begun! Join us for fun and conversation!");

    // Suggest social activities
    std::vector<std::string> activities = {
        "Trivia contest in guild chat!",
        "Fashion show - show off your transmog!",
        "Mount parade around the city!",
        "Guild dueling tournament!",
        "Hide and seek in the capital!"
    };

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, activities.size() - 1);

    BroadcastEventUpdates(eventId, "Today's activity: " + activities[dis(gen)]);
}

void GuildEventCoordinator::CoordinateLevelingEvent(Player* leader, uint32 eventId)
{
    if (!leader)
        return;

    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    const GuildEvent& event = it->second;

    // Form leveling group
    Group* group = leader->GetGroup();
    if (!group)
        group = FormEventGroup(eventId);

    if (!group)
    {
        BroadcastEventUpdates(eventId, "Failed to form leveling group");
        return;
    }

    // Determine appropriate leveling content
    uint32 averageLevel = 0;
    uint32 memberCount = 0;

    for (uint32 memberGuid : event.confirmedMembers)
    {
        Player* member = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(memberGuid));
        if (member)
        {
            averageLevel += member->GetLevel();
            memberCount++;
        }
    }

    if (memberCount > 0)
        averageLevel /= memberCount;

    // Suggest appropriate zones/dungeons
    std::stringstream suggestion;
    suggestion << "Recommended content for level " << averageLevel << " group";
    BroadcastEventUpdates(eventId, suggestion.str());

    // Enable experience sharing
    BroadcastEventUpdates(eventId, "Experience sharing enabled for efficient leveling!");
}

void GuildEventCoordinator::ProcessEventReminders()
{
    for (const auto& [eventId, event] : _guildEvents)
    {
        if (event.status == EventStatus::CONFIRMED || event.status == EventStatus::RECRUITING)
        {
            SendEventReminders(eventId);
        }
    }
}

void GuildEventCoordinator::NotifyEventChanges(uint32 eventId)
{
    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    const GuildEvent& event = it->second;

    std::stringstream msg;
    msg << "Event details have been updated. Please check the latest information.";
    BroadcastEventUpdates(eventId, msg.str());
}

void GuildEventCoordinator::BroadcastEventAnnouncement(uint32 eventId)
{
    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    const GuildEvent& event = it->second;
    Guild* guild = sGuildMgr->GetGuildById(event.guildId);
    if (!guild)
        return;

    std::stringstream announcement;
    announcement << "New guild event created: " << event.eventTitle;
    if (event.scheduledTime > 0)
    {
        uint32 hoursUntil = (event.scheduledTime - GameTime::GetGameTimeMS()) / 3600000;
        announcement << " (in " << hoursUntil << " hours)";
    }

    // Broadcast to guild
    std::string announcementMsg = announcement.str();
    guild->BroadcastToGuild(nullptr, false, announcementMsg, LANG_UNIVERSAL);
}

void GuildEventCoordinator::LogEventActivity(uint32 eventId, const std::string& activity)
{
    TC_LOG_DEBUG("playerbot", "Event %u: %s", eventId, activity.c_str());
}

void GuildEventCoordinator::HandleEventLogistics(uint32 eventId)
{
    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    GuildEvent& event = it->second;

    // Handle general logistics
    if (event.status == EventStatus::IN_PROGRESS)
    {
        // Check attendance
        uint32 presentCount = 0;
        for (uint32 memberGuid : event.confirmedMembers)
        {
            Player* member = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(memberGuid));
            if (member && member->IsInWorld())
                presentCount++;
        }

        // Update attendance metrics
        float currentAttendance = (float)presentCount / (float)event.confirmedMembers.size();
        _guildMetrics[event.guildId].averageAttendance =
            (_guildMetrics[event.guildId].averageAttendance + currentAttendance) / 2.0f;
    }
}

void GuildEventCoordinator::MonitorEventHealth(uint32 eventId)
{
    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    const GuildEvent& event = it->second;

    // Check various health metrics
    uint32 activeMembers = 0;
    uint32 deadMembers = 0;

    for (uint32 memberGuid : event.confirmedMembers)
    {
        Player* member = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(memberGuid));
        if (member)
        {
            if (member->IsAlive())
                activeMembers++;
            else
                deadMembers++;
        }
    }

    // Send health update if needed
    if (deadMembers > activeMembers)
    {
        BroadcastEventUpdates(eventId, "Warning: Multiple members need resurrection!");
    }
}

void GuildEventCoordinator::RescheduleConflictingEvents()
{
    // Find all conflicting events and reschedule them
    std::set<uint32> processedEvents;

    for (auto& guildScheduler : _guildSchedulers)
    {
        auto& scheduler = guildScheduler.second;

        for (uint32 eventId : scheduler.conflictingEvents)
        {
            if (processedEvents.find(eventId) != processedEvents.end())
                continue;

            auto it = _guildEvents.find(eventId);
            if (it == _guildEvents.end())
                continue;

            GuildEvent& event = it->second;

            // Only reschedule if event hasn't started
            if (event.status == EventStatus::PLANNING || event.status == EventStatus::RECRUITING)
            {
                // Move event by 2 hours
                event.scheduledTime += 7200000;
                UpdateEventCalendar(eventId);

                std::stringstream msg;
                msg << "Event rescheduled to avoid conflict. New time: ";
                msg << (event.scheduledTime - GameTime::GetGameTimeMS()) / 3600000 << " hours from now";
                BroadcastEventUpdates(eventId, msg.str());

                processedEvents.insert(eventId);
            }
        }

        scheduler.conflictingEvents.clear();
    }
}

void GuildEventCoordinator::UpdateEventMetrics(uint32 eventId, bool wasSuccessful)
{
    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    const GuildEvent& event = it->second;
    auto& metrics = _guildMetrics[event.guildId];

    if (wasSuccessful)
    {
        float efficiency = metrics.organizationEfficiency;
        metrics.organizationEfficiency = std::min(1.0f, efficiency + 0.02f);

        float satisfaction = metrics.memberSatisfaction;
        metrics.memberSatisfaction = std::min(1.0f, satisfaction + 0.03f);
    }
    else
    {
        float efficiency = metrics.organizationEfficiency;
        metrics.organizationEfficiency = std::max(0.0f, efficiency - 0.05f);

        float satisfaction = metrics.memberSatisfaction;
        metrics.memberSatisfaction = std::max(0.0f, satisfaction - 0.05f);
    }

    metrics.lastUpdate = GameTime::GetGameTimeMS();
}

void GuildEventCoordinator::UpdateEventParticipation(uint32 eventId, bool wasOrganizer)
{
    if (!_bot)
        return;

    auto& participation = _playerParticipation[_bot->GetGUID().GetCounter()];

    if (wasOrganizer)
    {
        participation.organizedEvents++;
        participation.totalEventsCreated++;
    }
    else
    {
        participation.participatedEvents++;
        participation.totalEventsAttended++;
    }

    participation.lastEventActivity = GameTime::GetGameTimeMS();
}

void GuildEventCoordinator::ProposeEventIdeas()
{
    if (!_bot || !_bot->GetGuildId())
        return;

    // Generate event proposals based on guild needs
    std::vector<std::string> proposals;

    // Analyze what the guild might need
    uint32 guildId = _bot->GetGuildId();
    Guild* guild = sGuildMgr->GetGuildById(guildId);
    if (!guild)
        return;

    // Check member levels to suggest appropriate events
    uint32 maxLevelCount = 0;
    uint32 levelingCount = 0;

    for (const auto& [guid, member] : guild->GetMembers())
    {
        Player* player = ObjectAccessor::FindPlayer(guid);
        if (player)
        {
            if (player->GetLevel() >= 80) // Assuming WotLK max level
                maxLevelCount++;
            else
                levelingCount++;
        }
    }

    if (maxLevelCount >= 10)
        proposals.push_back("Weekly raid night for progression");
    if (levelingCount >= 5)
        proposals.push_back("Alt leveling group for newer members");
    if (maxLevelCount >= 15)
        proposals.push_back("Rated battleground team");

    proposals.push_back("Guild social hour for team building");
    proposals.push_back("Achievement hunting group");

    // Broadcast proposals
    for (const std::string& proposal : proposals)
    {
        std::string message = "Event idea: " + proposal;
        guild->BroadcastToGuild(nullptr, false, message, LANG_UNIVERSAL);
    }
}

void GuildEventCoordinator::CoordinateEventTiming(uint32 eventId)
{
    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    GuildEvent& event = it->second;

    // Find optimal time based on member availability
    uint32 optimalTime = FindOptimalEventTime(event.guildId, event.eventType, event.duration);

    if (optimalTime != event.scheduledTime)
    {
        event.scheduledTime = optimalTime;
        UpdateEventCalendar(eventId);
        NotifyEventChanges(eventId);
    }
}

void GuildEventCoordinator::HandleEventChanges(uint32 eventId)
{
    NotifyEventChanges(eventId);

    auto it = _guildEvents.find(eventId);
    if (it != _guildEvents.end())
    {
        // Re-process invitations if needed
        if (it->second.status == EventStatus::RECRUITING)
            ProcessEventInvitations(eventId);
    }
}

void GuildEventCoordinator::CoordinateGroupForEvent(Group* group, uint32 eventId)
{
    if (!group)
        return;

    AssignGroupRoles(group, eventId);

    // Set loot rules appropriate for event type
    auto it = _guildEvents.find(eventId);
    if (it != _guildEvents.end())
    {
        const GuildEvent& event = it->second;

        if (event.eventType == GuildEventType::RAID_DUNGEON)
        {
            group->SetLootMethod(LootMethod::MASTER_LOOT);
            group->SetMasterLooterGuid(ObjectGuid::Create<HighGuid::Player>(event.organizerGuid));
        }
        else
        {
            group->SetLootMethod(LootMethod::GROUP_LOOT);
        }

        group->SetLootThreshold(ITEM_QUALITY_UNCOMMON);
    }
}

void GuildEventCoordinator::HandleEventGroupChanges(Group* group, uint32 eventId)
{
    if (!group)
        return;

    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    GuildEvent& event = it->second;

    // Update confirmed members based on current group
    event.confirmedMembers.clear();
    for (GroupReference const& itr : group->GetMembers())
    {
        if (Player* member = itr.GetSource())
        {
            event.confirmedMembers.push_back(member->GetGUID().GetCounter());
        }
    }

    // Re-assign roles if needed
    AssignEventRoles(eventId);
}

void GuildEventCoordinator::ResolveEventConflicts()
{
    RescheduleConflictingEvents();
}

void GuildEventCoordinator::HandleOverlappingEvents(uint32 eventId1, uint32 eventId2)
{
    auto it1 = _guildEvents.find(eventId1);
    auto it2 = _guildEvents.find(eventId2);

    if (it1 == _guildEvents.end() || it2 == _guildEvents.end())
        return;

    GuildEvent& event1 = it1->second;
    GuildEvent& event2 = it2->second;

    // Reschedule lower priority event
    if (event1.priority < event2.priority)
    {
        event1.scheduledTime += 7200000; // Move by 2 hours
        UpdateEventCalendar(eventId1);
        NotifyEventChanges(eventId1);
    }
    else
    {
        event2.scheduledTime += 7200000; // Move by 2 hours
        UpdateEventCalendar(eventId2);
        NotifyEventChanges(eventId2);
    }
}

void GuildEventCoordinator::ManageEventPriorities(uint32 guildId)
{
    auto& events = _guildActiveEvents[guildId];

    // Sort events by priority and time
    std::sort(events.begin(), events.end(), [this](uint32 a, uint32 b)
    {
        auto itA = _guildEvents.find(a);
        auto itB = _guildEvents.find(b);

        if (itA == _guildEvents.end() || itB == _guildEvents.end())
            return false;

        if (itA->second.priority != itB->second.priority)
            return itA->second.priority > itB->second.priority;

        return itA->second.scheduledTime < itB->second.scheduledTime;
    });
}

void GuildEventCoordinator::CreateEventTemplate(const std::string& templateName, const GuildEvent& templateData)
{
    _eventTemplates[templateName] = templateData;
    _typeTemplates[templateData.eventType].push_back(templateName);
}

GuildEvent GuildEventCoordinator::LoadEventTemplate(const std::string& templateName)
{
    auto it = _eventTemplates.find(templateName);
    if (it != _eventTemplates.end())
        return it->second;

    return GuildEvent();
}

std::vector<std::string> GuildEventCoordinator::GetAvailableTemplates()
{
    std::vector<std::string> templates;
    for (const auto& [name, data] : _eventTemplates)
    {
        templates.push_back(name);
    }
    return templates;
}

void GuildEventCoordinator::CustomizeEventFromTemplate(GuildEvent& event, const std::string& templateName)
{
    auto it = _eventTemplates.find(templateName);
    if (it != _eventTemplates.end())
    {
        const GuildEvent& templateData = it->second;

        // Copy template data but preserve unique fields
        uint32 savedId = event.eventId;
        uint32 savedGuildId = event.guildId;
        uint32 savedOrganizerGuid = event.organizerGuid;
        std::string savedOrganizerName = event.organizerName;
        uint32 savedScheduledTime = event.scheduledTime;

        event = templateData;

        event.eventId = savedId;
        event.guildId = savedGuildId;
        event.organizerGuid = savedOrganizerGuid;
        event.organizerName = savedOrganizerName;
        event.scheduledTime = savedScheduledTime;
    }
}

void GuildEventCoordinator::PlanSeasonalEvents(uint32 guildId)
{
    // Check current season/holiday
    time_t rawtime;
    time(&rawtime);
    struct tm* timeinfo = localtime(&rawtime);
    int month = timeinfo->tm_mon + 1;

    GuildEvent seasonalEvent;
    seasonalEvent.guildId = guildId;
    seasonalEvent.priority = EventPriority::HIGH;

    // Plan events based on season
    if (month == 12) // December - Winter Veil
    {
        seasonalEvent.eventTitle = "Winter Veil Guild Celebration";
        seasonalEvent.eventDescription = "Celebrate Winter Veil together!";
        seasonalEvent.eventType = GuildEventType::SOCIAL_GATHERING;
    }
    else if (month == 10) // October - Hallow's End
    {
        seasonalEvent.eventTitle = "Hallow's End Costume Party";
        seasonalEvent.eventDescription = "Spooky fun and costume contest!";
        seasonalEvent.eventType = GuildEventType::SOCIAL_GATHERING;
    }
    else if (month == 2) // February - Love is in the Air
    {
        seasonalEvent.eventTitle = "Love is in the Air Guild Event";
        seasonalEvent.eventDescription = "Spread the love with guildmates!";
        seasonalEvent.eventType = GuildEventType::SOCIAL_GATHERING;
    }

    if (!seasonalEvent.eventTitle.empty())
    {
        seasonalEvent.scheduledTime = FindOptimalEventTime(guildId, seasonalEvent.eventType, 3600000);
        CreateGuildEvent(_bot, seasonalEvent);
    }
}

void GuildEventCoordinator::HandleHolidayEvents(uint32 guildId)
{
    // Similar to PlanSeasonalEvents but for specific holidays
    PlanSeasonalEvents(guildId);
}

void GuildEventCoordinator::OrganizeSpecialCelebrations(uint32 guildId)
{
    Guild* guild = sGuildMgr->GetGuildById(guildId);
    if (!guild)
        return;

    // Check for special milestones
    uint32 memberCount = guild->GetMembersCount();

    if (memberCount == 100 || memberCount == 250 || memberCount == 500)
    {
        GuildEvent celebration;
        celebration.guildId = guildId;
        celebration.eventTitle = "Guild Milestone Celebration!";
        celebration.eventDescription = "Celebrating " + std::to_string(memberCount) + " members!";
        celebration.eventType = GuildEventType::SOCIAL_GATHERING;
        celebration.priority = EventPriority::HIGH;
        celebration.scheduledTime = GameTime::GetGameTimeMS() + 86400000; // Tomorrow
        celebration.maxParticipants = 40;
        celebration.minParticipants = 10;

        CreateGuildEvent(_bot, celebration);
    }
}

void GuildEventCoordinator::CoordinateGuildAnniversary(uint32 guildId)
{
    Guild* guild = sGuildMgr->GetGuildById(guildId);
    if (!guild)
        return;

    // Calculate guild age (this is simplified)
    GuildEvent anniversary;
    anniversary.guildId = guildId;
    anniversary.eventTitle = "Guild Anniversary Celebration!";
    anniversary.eventDescription = "Celebrating another year together!";
    anniversary.eventType = GuildEventType::SOCIAL_GATHERING;
    anniversary.priority = EventPriority::CRITICAL;
    anniversary.scheduledTime = GameTime::GetGameTimeMS() + 604800000; // Next week
    anniversary.duration = 7200000; // 2 hours
    anniversary.maxParticipants = 40;
    anniversary.minParticipants = 15;
    anniversary.isRecurring = true;
    anniversary.recurringInterval = 2592000000; // 30 days (yearly would overflow uint32)

    CreateGuildEvent(_bot, anniversary);
}

void GuildEventCoordinator::SetMaxConcurrentEvents(uint32 guildId, uint32 maxEvents)
{
    // This would normally be stored in a config map
    // For now, just log the setting
    TC_LOG_DEBUG("playerbot", "GuildEventCoordinator: Max concurrent events for guild %u set to %u",
        guildId, maxEvents);
}

void GuildEventCoordinator::ConfigureEventNotifications(bool enableReminders)
{
    // This would normally be stored in player preferences
    if (_bot)
    {
        auto& profile = _playerProfiles[_bot->GetGUID().GetCounter()];
        // Store reminder preference (would need to add this field to profile)
        TC_LOG_DEBUG("playerbot", "GuildEventCoordinator: Event reminders %s for player %u",
            enableReminders ? "enabled" : "disabled", _bot->GetGUID().GetCounter());
    }
}

void GuildEventCoordinator::SetEventAutoSignup(GuildEventType eventType, bool autoSignup)
{
    if (_bot)
    {
        auto& profile = _playerProfiles[_bot->GetGUID().GetCounter()];
        profile.autoAcceptInvitations = autoSignup;

        if (autoSignup)
        {
            // Add to preferred types if not already there
            auto it = std::find(profile.preferredEventTypes.begin(),
                profile.preferredEventTypes.end(), eventType);
            if (it == profile.preferredEventTypes.end())
                profile.preferredEventTypes.push_back(eventType);
        }
    }
}

void GuildEventCoordinator::HandleEventError(uint32 eventId, const std::string& error)
{
    TC_LOG_ERROR("playerbot", "GuildEventCoordinator: Event %u error: %s", eventId, error.c_str());

    auto it = _guildEvents.find(eventId);
    if (it != _guildEvents.end())
    {
        BroadcastEventUpdates(eventId, "Event encountered an error: " + error);

        // Attempt recovery
        RecoverFromEventFailure(eventId);
    }
}

void GuildEventCoordinator::RecoverFromEventFailure(uint32 eventId)
{
    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    GuildEvent& event = it->second;

    // Try to recover based on event status
    switch (event.status)
    {
        case EventStatus::IN_PROGRESS:
            // Try to continue with reduced participants
            if (event.confirmedMembers.size() >= event.minParticipants / 2)
            {
                BroadcastEventUpdates(eventId, "Continuing event with reduced participants");
            }
            else
            {
                HandleEventCompletion(eventId);
            }
            break;

        case EventStatus::RECRUITING:
            // Extend recruitment time
            event.scheduledTime += 3600000; // Add 1 hour
            UpdateEventCalendar(eventId);
            BroadcastEventUpdates(eventId, "Event delayed by 1 hour for additional recruitment");
            break;

        default:
            // Cancel if can't recover
            CancelGuildEvent(nullptr, eventId);
            break;
    }
}

void GuildEventCoordinator::HandleMissingOrganizer(uint32 eventId)
{
    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    GuildEvent& event = it->second;

    // Find a replacement organizer
    Player* newOrganizer = nullptr;
    for (uint32 memberGuid : event.confirmedMembers)
    {
        Player* member = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(memberGuid));
        if (member)
        {
            // Check if member can lead
            auto profileIt = _playerProfiles.find(memberGuid);
            if (profileIt != _playerProfiles.end() && profileIt->second.enableEventLeadership)
            {
                newOrganizer = member;
                break;
            }
        }
    }

    if (newOrganizer)
    {
        event.organizerGuid = newOrganizer->GetGUID().GetCounter();
        event.organizerName = newOrganizer->GetName();
        BroadcastEventUpdates(eventId, "New event organizer: " + event.organizerName);
    }
    else
    {
        // No replacement found, cancel event
        CancelGuildEvent(nullptr, eventId);
    }
}

void GuildEventCoordinator::EmergencyEventCancellation(uint32 eventId)
{
    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    // Immediate cancellation
    UpdateEventStatus(eventId, EventStatus::CANCELLED);
    BroadcastEventUpdates(eventId, "EMERGENCY: Event has been cancelled immediately!");

    // Disband any groups
    if (Player* organizer = ObjectAccessor::FindPlayer(
        ObjectGuid::Create<HighGuid::Player>(it->second.organizerGuid)))
    {
        if (Group* group = organizer->GetGroup())
        {
            group->Disband();
        }
    }

    // Clean up
    auto& guildEvents = _guildActiveEvents[it->second.guildId];
    guildEvents.erase(std::remove(guildEvents.begin(), guildEvents.end(), eventId),
        guildEvents.end());
}

void GuildEventCoordinator::SuggestOptimalEventTimes(uint32 guildId)
{
    auto it = _guildSchedulers.find(guildId);
    if (it == _guildSchedulers.end())
        return;

    const GuildEventScheduler& scheduler = it->second;

    Guild* guild = sGuildMgr->GetGuildById(guildId);
    if (!guild)
        return;

    std::stringstream suggestions;
    suggestions << "Optimal event times based on guild activity: ";

    if (!scheduler.popularTimes.empty())
    {
        for (const auto& timeSlot : scheduler.popularTimes)
        {
            uint32 hourOfWeek = timeSlot.first / 3600000;
            uint32 dayOfWeek = hourOfWeek / 24;
            uint32 hourOfDay = hourOfWeek % 24;

            std::string dayName[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
            suggestions << dayName[dayOfWeek] << " " << hourOfDay << ":00, ";
        }
    }
    else
    {
        suggestions << "Weekday evenings (7-10 PM), Weekend afternoons (2-6 PM)";
    }

    std::string suggestionsMsg = suggestions.str();
    guild->BroadcastToGuild(nullptr, false, suggestionsMsg, LANG_UNIVERSAL);
}

void GuildEventCoordinator::UpdateEventParticipants(uint32 eventId, const std::string& message)
{
    BroadcastEventUpdates(eventId, message);
}

void GuildEventCoordinator::CoordinateEventPreparation(uint32 eventId)
{
    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    const GuildEvent& event = it->second;

    // Send preparation reminders
    std::stringstream prep;

    switch (event.eventType)
    {
        case GuildEventType::RAID_DUNGEON:
            prep << "Raid preparation: Repair gear, bring consumables, review tactics!";
            break;
        case GuildEventType::PVP_BATTLEGROUND:
        case GuildEventType::PVP_ARENA:
            prep << "PvP preparation: Check PvP gear, stock up on PvP consumables!";
            break;
        default:
            prep << "Event starting soon, please prepare!";
            break;
    }

    BroadcastEventUpdates(eventId, prep.str());
}

void GuildEventCoordinator::ManageEventExecution(uint32 eventId)
{
    ExecuteGuildEvent(eventId);
}

void GuildEventCoordinator::CacheEventData(uint32 guildId)
{
    // Pre-cache guild event data for performance
    LoadGuildEventData(guildId);
}

void GuildEventCoordinator::PreloadEventInformation(uint32 eventId)
{
    // Preload event data for quick access
    auto it = _guildEvents.find(eventId);
    if (it != _guildEvents.end())
    {
        // Cache participant data
        for (uint32 memberGuid : it->second.confirmedMembers)
        {
            ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(memberGuid));
        }
    }
}

void GuildEventCoordinator::OptimizeEventComposition(uint32 eventId)
{
    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return;

    GuildEvent& event = it->second;

    // Optimize group composition based on event type
    if (event.eventType == GuildEventType::RAID_DUNGEON)
    {
        // Ensure proper tank/healer/dps ratio
        AssignEventRoles(eventId);

        // Check if composition is viable
        uint32 tanks = 0, healers = 0, dps = 0;
        for (const auto& [memberGuid, role] : event.memberRoles)
        {
            if (role == "Tank") tanks++;
            else if (role == "Healer") healers++;
            else if (role == "DPS") dps++;
        }

        if (tanks < 1 || healers < 2)
        {
            BroadcastEventUpdates(eventId, "Warning: Need more tanks or healers for optimal composition!");
        }
    }
}

bool GuildEventCoordinator::IsEventViable(const GuildEvent& event)
{
    // Check if event meets minimum requirements
    if (event.confirmedMembers.size() < event.minParticipants)
        return false;

    // Check if we have necessary roles
    if (event.eventType == GuildEventType::RAID_DUNGEON)
    {
        uint32 tanks = 0, healers = 0;
        for (const auto& [memberGuid, role] : event.memberRoles)
        {
            if (role == "Tank") tanks++;
            else if (role == "Healer") healers++;
        }

        if (tanks < 1 || healers < 2)
            return false;
    }

    return true;
}

std::vector<uint32> GuildEventCoordinator::SelectEventParticipants(uint32 eventId)
{
    std::vector<uint32> selectedParticipants;

    auto it = _guildEvents.find(eventId);
    if (it == _guildEvents.end())
        return selectedParticipants;

    const GuildEvent& event = it->second;

    // Prioritize by participation rating and role needs
    std::vector<std::pair<uint32, float>> candidates;

    for (uint32 memberGuid : event.invitedMembers)
    {
        auto partIt = _playerParticipation.find(memberGuid);
        if (partIt != _playerParticipation.end())
        {
            candidates.push_back({memberGuid, partIt->second.participationRating});
        }
        else
        {
            candidates.push_back({memberGuid, 0.5f}); // Default rating
        }
    }

    // Sort by rating
    std::sort(candidates.begin(), candidates.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    // Select top candidates up to max
    for (const auto& [memberGuid, rating] : candidates)
    {
        if (selectedParticipants.size() >= event.maxParticipants)
            break;
        selectedParticipants.push_back(memberGuid);
    }

    return selectedParticipants;
}

} // namespace Playerbot