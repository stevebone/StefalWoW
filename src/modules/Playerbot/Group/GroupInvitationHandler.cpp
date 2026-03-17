/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "GroupInvitationHandler.h"
#include "Player.h"
#include "Group.h"
#include "GroupMgr.h"
#include "WorldSession.h"
#include "WorldPacket.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include "World.h"
#include "SocialMgr.h"
#include "PartyPackets.h"
#include "BotSession.h"
#include "BotAI.h"
#include "../Session/BotSession.h"
#include "../AI/BotAI.h"
#include "Opcodes.h"
#include "Config/PlayerbotConfig.h"
#include "Common.h"
#include <algorithm>

namespace Playerbot
{

GroupInvitationHandler::GroupInvitationHandler(Player* bot)
    : _bot(bot)
    , _autoAcceptEnabled(true)
    , _responseDelayMs(500)
    , _maxInvitationsPerMinute(10)
    , _maxAcceptRange(100.0f)
    , _updateTimer(0)
    , _cleanupTimer(0)
{

    TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Initialized for bot {} ({})",
        _bot->GetName(), _bot->GetGUID().ToString());
}

GroupInvitationHandler::~GroupInvitationHandler()
{
    ClearPendingInvitations();
}

bool GroupInvitationHandler::HandleInvitation(WorldPackets::Party::PartyInvite const& packet)
{
    ::std::lock_guard lock(_invitationMutex);

    // Extract invitation details
    ObjectGuid inviterGuid = packet.InviterGUID;
    ::std::string inviterName = packet.InviterName;
    uint32 proposedRoles = packet.ProposedRoles;

    TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Bot {} received invitation from {} ({})",
        _bot->GetName(), inviterName, inviterGuid.ToString());

    // Update statistics
    _stats.totalInvitations++;
    _stats.lastInvitation = ::std::chrono::steady_clock::now();

    // Validate inviter first
    Player* inviter = ObjectAccessor::FindPlayer(inviterGuid);
    if (!inviter)
    {
        TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Inviter {} not found, declining", inviterGuid.ToString());
        _stats.invalidInvitations++;
        return false;
    }

    // Check if invitation is valid
    if (!IsValidInviter(inviter))
    {
        TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Invalid inviter {}, declining", inviterName);
        DeclineInvitation(inviterGuid, "Invalid inviter");
        _stats.invalidInvitations++;
        return false;
    }

    // Check for invitation loops
    if (!ValidateNoInvitationLoop(inviterGuid))
    {
        TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Invitation loop detected from {}, declining", inviterName);
        DeclineInvitation(inviterGuid, "Loop prevention");
        _stats.declinedInvitations++;
        return false;
    }

    // Check if bot already has too many pending invitations (anti-spam)
    if (_pendingInvitations.size() > 5)
    {
        TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Too many pending invitations for bot {}", _bot->GetName());
        _stats.declinedInvitations++;
        return false;
    }

    // Create pending invitation
    PendingInvitation invitation;
    invitation.inviterGuid = inviterGuid;
    invitation.inviterName = inviterName;
    invitation.proposedRoles = proposedRoles;
    invitation.timestamp = ::std::chrono::steady_clock::now();
    invitation.isProcessing = false;

    // Add to queue
    _pendingInvitations.push(invitation);

    TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Queued invitation from {} for bot {}",
        inviterName, _bot->GetName());

    return true;
}

void GroupInvitationHandler::Update(uint32 diff)
{
    // GroupInvitationHandler update processing

    // Update timers
    _updateTimer += diff;
    _cleanupTimer += diff;

    // CRITICAL FIX: Bots don't receive network packets, so HandleInvitation() is never called.
    // We MUST poll TrinityCore's GetGroupInvite() to detect pending invitations.
    // This is safe as long as we don't hold our mutex while calling TrinityCore APIs.
    if (_bot && !_bot->GetGroup())
    {
        Group* pendingInvite = _bot->GetGroupInvite();
        if (pendingInvite)
        {
            ObjectGuid leaderGuid = pendingInvite->GetLeaderGUID();

            // Check if we already have this invite queued
            bool alreadyQueued = false;
            {
                ::std::lock_guard lock(_invitationMutex);
                if (_currentInviter == leaderGuid)
                    alreadyQueued = true;
                // Check if we recently processed this inviter (prevent spam)
                if (_recentInviters.find(leaderGuid) != _recentInviters.end())
                {
                    auto timeSinceLastAccept = ::std::chrono::steady_clock::now() - _lastAcceptTime;
                    if (timeSinceLastAccept < ::std::chrono::milliseconds(5000))
                        alreadyQueued = true;
                }
            }

            if (!alreadyQueued)
            {
                Player* leader = ObjectAccessor::FindPlayer(leaderGuid);
                TC_LOG_INFO("playerbot.group", "GroupInvitationHandler: Bot {} detected pending invite from {} ({}), queueing for processing",
                    _bot->GetName(),
                    leader ? leader->GetName() : "Unknown",
                    leaderGuid.ToString());

                // Create synthetic pending invitation
                ::std::lock_guard lock(_invitationMutex);
                PendingInvitation invitation;
                invitation.inviterGuid = leaderGuid;
                invitation.inviterName = leader ? leader->GetName() : "Unknown";
                invitation.proposedRoles = 0;
                invitation.timestamp = ::std::chrono::steady_clock::now();
                invitation.isProcessing = false;
                _pendingInvitations.push(invitation);

                _stats.totalInvitations++;
                _stats.lastInvitation = ::std::chrono::steady_clock::now();
            }
        }
    }

    // Cleanup expired invitations periodically
    if (_cleanupTimer >= CLEANUP_INTERVAL)
    {
        CleanupExpiredInvitations();
        _cleanupTimer = 0;
    }

    // Process invitations at regular intervals
    if (_updateTimer >= UPDATE_INTERVAL)
    {
        ProcessNextInvitation();
        _updateTimer = 0;
    }
}

bool GroupInvitationHandler::ShouldAcceptInvitation(ObjectGuid inviterGuid) const
{
    // Check if auto-accept is enabled
    if (!_autoAcceptEnabled)
        return false;
    // Get inviter
    Player* inviter = ObjectAccessor::FindPlayer(inviterGuid);
    if (!inviter)
        return false;

    // Validate inviter
    if (!IsValidInviter(inviter))
        return false;

    // Check if bot is already in a group
    if (_bot->GetGroup())
    {
        TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Bot {} already in group, declining", _bot->GetName());
        return false;
    }

    // Check if bot has pending group invite
    if (_bot->GetGroupInvite())
    {
        // If it's from the same inviter, accept it
    if (_bot->GetGroupInvite()->GetLeaderGUID() == inviterGuid)
            return true;

        TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Bot {} has different pending invite", _bot->GetName());
        return false;
    }

    // Check inviter's group if they have one
    Group* inviterGroup = inviter->GetGroup();
    if (inviterGroup)
    {
        if (!CanJoinGroup(inviterGroup))
            return false;
    }

    return true;
}

bool GroupInvitationHandler::IsValidInviter(Player* inviter) const
{
    if (!inviter)
        return false;

    // Don't accept invitations from other bots to prevent bot loops
    if (PlayerbotConfig* config = PlayerbotConfig::instance())
    {
        // Check if inviter is a bot (implementation depends on your bot detection method)
        // For now, we'll check if they have a bot session or are flagged as bots
    if (inviter->GetSession() && inviter->GetSession()->GetPlayer())
        {
            // You may need to add a method to detect if a player is a bot
            // For safety, we'll accept all players for now but log it
            TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Checking inviter {} validity", inviter->GetName());
        }
    }

    // Check if inviter is in range
    if (!IsInviterInRange(inviter))
    {
        TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Inviter {} out of range", inviter->GetName());
        return false;
    }

    // Check if inviter is on same team (unless cross-faction is allowed)
    if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GROUP))
    {
        if (_bot->GetTeam() != inviter->GetTeam())
        {
            TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Cross-faction invitation from {}", inviter->GetName());
            return false;
        }
    }

    // Check if inviter is ignored
    if (_bot->GetSocial()->HasIgnore(inviter->GetGUID(), inviter->GetSession()->GetAccountGUID()))
    {
        TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Inviter {} is ignored", inviter->GetName());
        return false;
    }

    return true;
}

bool GroupInvitationHandler::CanJoinGroup(Group* group) const
{
    if (!group)
        return true; // No group yet, can join

    // Check if group is full
    if (group->IsFull())
    {
        TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Group is full");
        return false;
    }

    // Check if bot meets level requirements
    Player* leader = ObjectAccessor::FindPlayer(group->GetLeaderGUID());
    if (leader && !leader->GetSocial()->HasFriend(_bot->GetGUID()) &&
        leader->GetLevel() < sWorld->getIntConfig(CONFIG_PARTY_LEVEL_REQ))
    {
        TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Bot doesn't meet level requirements");
        return false;
    }

    // Check instance compatibility
    if (group->IsCreated())
    {
        // Check if bot can enter the same instances as the group
        uint32 groupInstanceId = 0;
        for (Group::MemberSlot const& member : group->GetMemberSlots())
        {
            if (Player* groupMember = ObjectAccessor::FindPlayer(member.guid))
            {
                if (groupMember->GetInstanceId() != 0)
                {
                    groupInstanceId = groupMember->GetInstanceId();
                    break;
                }
            }
        }

        if (groupInstanceId != 0 && _bot->GetInstanceId() != 0 &&
            _bot->GetInstanceId() != groupInstanceId)
        {
            TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Instance mismatch");
            return false;
        }
    }

    return true;
}

bool GroupInvitationHandler::AcceptInvitation(ObjectGuid inviterGuid)
{
    // NOTE: This method can be called with _invitationMutex already locked by ProcessNextInvitation()
    // We use try_lock to avoid deadlock
    ::std::unique_lock lock(_invitationMutex, ::std::try_to_lock);
    if (!lock.owns_lock())
    {
        // Already locked by caller (ProcessNextInvitation), proceed without locking
        return AcceptInvitationInternal(inviterGuid);
    }
    return AcceptInvitationInternal(inviterGuid);
}

void GroupInvitationHandler::DeclineInvitation(ObjectGuid inviterGuid, ::std::string const& reason)
{
    // NOTE: This method can be called with _invitationMutex already locked by ProcessNextInvitation()
    // We use try_lock to avoid deadlock
    ::std::unique_lock lock(_invitationMutex, ::std::try_to_lock);
    if (!lock.owns_lock())
    {
        // Already locked by caller (ProcessNextInvitation), proceed without locking
        DeclineInvitationInternal(inviterGuid, reason);
        return;
    }

    DeclineInvitationInternal(inviterGuid, reason);
}

bool GroupInvitationHandler::HasPendingInvitation() const
{
    ::std::lock_guard lock(_invitationMutex);
    return !_pendingInvitations.empty() || !_currentInviter.IsEmpty();
}

ObjectGuid GroupInvitationHandler::GetPendingInviter() const
{
    ::std::lock_guard lock(_invitationMutex);

    if (!_currentInviter.IsEmpty())
        return _currentInviter;
    if (!_pendingInvitations.empty())
        return _pendingInvitations.front().inviterGuid;

    return ObjectGuid::Empty;
}

void GroupInvitationHandler::ClearPendingInvitations()
{
    ::std::lock_guard lock(_invitationMutex);
    while (!_pendingInvitations.empty())
        _pendingInvitations.pop();
    _currentInviter = ObjectGuid::Empty;
    _recentInviters.clear();
}

void GroupInvitationHandler::SetResponseDelay(uint32 delayMs)
{
    _responseDelayMs = ::std::min(::std::max(delayMs, MIN_RESPONSE_DELAY), MAX_RESPONSE_DELAY);
    TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Response delay set to {}ms for bot {}",
        _responseDelayMs, _bot->GetName());
}
WorldSession* GroupInvitationHandler::GetSession() const
{
    return _bot ? _bot->GetSession() : nullptr;
}

bool GroupInvitationHandler::SendAcceptPacket()
{
    // EXECUTION MARKER: Verify this method is being called
    TC_LOG_INFO("playerbot.debug", "=== EXECUTION MARKER: SendAcceptPacket() called for bot {} ===", _bot ? _bot->GetName() : "NULL");
    WorldSession* session = GetSession();
    if (!session)
    {
        TC_LOG_ERROR("playerbot", "GroupInvitationHandler: No session found for bot {}", _bot->GetName());
        return false;
    }

    // Check if bot has a pending group invite from TrinityCore
    Group* inviteGroup = _bot->GetGroupInvite();
    if (!inviteGroup)
    {
        TC_LOG_ERROR("playerbot", "GroupInvitationHandler: No pending group invite for bot {} - this should not happen", _bot->GetName());
        return false;
    }

    TC_LOG_INFO("playerbot", "GroupInvitationHandler: Bot {} has pending invite from group {} (Leader: {})",
        _bot->GetName(),
        inviteGroup->GetGUID().ToString(),
        ObjectAccessor::FindPlayer(inviteGroup->GetLeaderGUID()) ?
            ObjectAccessor::FindPlayer(inviteGroup->GetLeaderGUID())->GetName() : "Unknown");

    // Verify session state before processing
    TC_LOG_INFO("playerbot", "GroupInvitationHandler: Session state - Player: {}, SessionPlayer: {}, Bot GUID: {}",
        _bot->GetName(),
        session->GetPlayer() ? session->GetPlayer()->GetName() : "NULL",
        _bot->GetGUID().ToString());

    // Create properly formatted packet data
    WorldPacket packet(CMSG_PARTY_INVITE_RESPONSE);
    // Write packet data according to PartyInviteResponse::Read() format:
    // 1. Optional PartyIndex (we don't specify one for normal groups)
    packet.WriteBit(false); // PartyIndex not present
    // 2. Accept flag (1 bit)
    packet.WriteBit(true);  // Accept = true
    // 3. Optional RolesDesired (we don't specify roles)
    packet.WriteBit(false); // RolesDesired not present

    packet.FlushBits(); // Flush any remaining bits

    // No additional data needed since we didn't enable optional fields

    // Create PartyInviteResponse and let it read from our properly formatted packet
    WorldPackets::Party::PartyInviteResponse response(::std::move(packet));
    response.Read(); // This will properly parse our packet data
    TC_LOG_INFO("playerbot", "GroupInvitationHandler: About to call HandlePartyInviteResponseOpcode for bot {}", _bot->GetName());
    // Send the packet through the session handler
    TC_LOG_INFO("playerbot", "GroupInvitationHandler: Before HandlePartyInviteResponseOpcode - Bot group: {}, Bot invite: {}",
        _bot->GetGroup() ? _bot->GetGroup()->GetGUID().ToString() : "None",
        _bot->GetGroupInvite() ? _bot->GetGroupInvite()->GetGUID().ToString() : "None");

    session->HandlePartyInviteResponseOpcode(response);
    TC_LOG_INFO("playerbot", "GroupInvitationHandler: After HandlePartyInviteResponseOpcode - Bot group: {}, Bot invite: {}",
        _bot->GetGroup() ? _bot->GetGroup()->GetGUID().ToString() : "None",
        _bot->GetGroupInvite() ? _bot->GetGroupInvite()->GetGUID().ToString() : "None");

    // Check results immediately after the call
    if (_bot->GetGroup())
    {
        // EXECUTION MARKER: Verify this success path is being reached
        TC_LOG_INFO("playerbot.debug", "=== EXECUTION MARKER: Bot {} successfully joined group - entering follow activation path ===", _bot->GetName());
        Group* botGroup = _bot->GetGroup();
        Player* leader = ObjectAccessor::FindPlayer(botGroup->GetLeaderGUID());
        TC_LOG_INFO("playerbot", "GroupInvitationHandler: SUCCESS! Bot {} successfully joined group (Group ID: {}, Members: {}, Leader: {})",
            _bot->GetName(),
            botGroup->GetGUID().ToString(),
            botGroup->GetMembersCount(),
            leader ? leader->GetName() : "Unknown");
        // EXECUTION MARKER: Check if we reach after success message
        // DEADLOCK FIX #13: The code below was calling OnGroupJoined() THREE TIMES:
        // 1. Line 455: First call (inside "SIMPLE FIX" block)
        // 2. Line 494: Second call (DUPLICATE!)
        // 3. Lines 499-507: Third attempt via GetStrategy/ActivateStrategy
        //
        // This caused "resource deadlock would occur" because OnGroupJoined() acquires
        // unique_lock on BotAI::_mutex, and calling it multiple times in quick succession
        // from the same or different threads caused lock contention.
        //
        // Solution: Call OnGroupJoined() ONCE and ONLY ONCE

        TC_LOG_INFO("module.playerbot.group", "Bot {} accepted group invitation, triggering OnGroupJoined", _bot->GetName());
        // Get the bot's AI
        auto* session = _bot->GetSession();
        if (!session)
        {
            TC_LOG_ERROR("module.playerbot.group", "Bot {} has no session!", _bot->GetName());
            return false;
        }

        auto* botSession = dynamic_cast<BotSession*>(session);
        if (!botSession)
        {
            TC_LOG_ERROR("module.playerbot.group", "Bot {} session is not a BotSession!", _bot->GetName());
            return false;
        }

        auto* botAI = botSession->GetAI();
        if (!botAI)
        {
            TC_LOG_ERROR("module.playerbot.group", "Bot {} BotSession has no AI!", _bot->GetName());
            return false;
        }

        // Call OnGroupJoined ONCE - it handles everything internally:
        // - Creates follow strategy if needed
        // - Creates group_combat strategy if needed
        // - Activates both strategies
        // - Calls OnActivate() callbacks
        // - All under a SINGLE unique_lock (Fix #12)
        botAI->OnGroupJoined(_bot->GetGroup());
        TC_LOG_INFO("module.playerbot.group", "Bot {} OnGroupJoined completed successfully", _bot->GetName());
    }
    else
    {
        TC_LOG_ERROR("playerbot", "GroupInvitationHandler: FAILURE! Bot {} group join failed - no group found after invitation acceptance", _bot->GetName());

        // Check if the bot still has a pending invite (which would indicate a problem)
    if (_bot->GetGroupInvite())
        {
            TC_LOG_ERROR("playerbot", "GroupInvitationHandler: Bot {} still has pending invite - group acceptance may have failed", _bot->GetName());
        }

        // Check if the inviter's group exists and has members
        Group* inviterGroup = inviteGroup;
        if (inviterGroup)
        {
            TC_LOG_ERROR("playerbot", "GroupInvitationHandler: Inviter group {} still exists with {} members",
                inviterGroup->GetGUID().ToString(), inviterGroup->GetMembersCount());
        }
    }

    return true;
}

void GroupInvitationHandler::SendDeclinePacket(::std::string const& reason)
{
    WorldSession* session = GetSession();
    if (!session)
        return;

    // Create properly formatted packet data
    WorldPacket packet(CMSG_PARTY_INVITE_RESPONSE);

    // Write packet data according to PartyInviteResponse::Read() format:
    // 1. Optional PartyIndex (we don't specify one for normal groups)
    packet.WriteBit(false); // PartyIndex not present
    // 2. Accept flag (1 bit)
    packet.WriteBit(false); // Accept = false
    // 3. Optional RolesDesired (we don't specify roles)
    packet.WriteBit(false); // RolesDesired not present

    packet.FlushBits(); // Flush any remaining bits

    // No additional data needed since we didn't enable optional fields

    // Create PartyInviteResponse and let it read from our properly formatted packet
    WorldPackets::Party::PartyInviteResponse response(::std::move(packet));
    response.Read(); // This will properly parse our packet data

    // Send the packet through the session handler
    session->HandlePartyInviteResponseOpcode(response);

    TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Sent decline packet for bot {} ({})",
        _bot->GetName(), reason);
}
bool GroupInvitationHandler::ValidateNoInvitationLoop(ObjectGuid inviterGuid) const
{
    // Check if we recently accepted an invitation from this inviter
    if (_recentInviters.find(inviterGuid) != _recentInviters.end())
    {
        // Check if enough time has passed
        auto timeSinceLastAccept = ::std::chrono::steady_clock::now() - _lastAcceptTime;
        if (timeSinceLastAccept < ::std::chrono::milliseconds(RECENT_INVITER_MEMORY))
        {
            TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Recent invitation from same inviter detected");
            return false;
        }
    }

    // Check if inviter is a bot that we invited (would create a loop)
    Player* inviter = ObjectAccessor::FindPlayer(inviterGuid);
    if (inviter && inviter->GetGroup())
    {
        // If we're the leader of a group and the inviter is in our group, this would be a loop
    if (_bot->GetGroup() && _bot->GetGroup()->IsLeader(_bot->GetGUID()))
        {
            if (inviter->GetGroup() == _bot->GetGroup())
            {
                TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Invitation loop detected - inviter in our group");
                return false;
            }
        }
    }

    return true;
}

bool GroupInvitationHandler::IsInviterInRange(Player* inviter) const
{
    if (!inviter || !_bot)
        return false;
    // Check if on same map
    if (_bot->GetMapId() != inviter->GetMapId())
        return false;

    // Check distance
    float distance = _bot->GetDistance(inviter);
    if (distance > _maxAcceptRange)
    {
        TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Inviter {} is too far ({}y > {}y)",
            inviter->GetName(), distance, _maxAcceptRange);
        return false;
    }

    return true;
}

void GroupInvitationHandler::LogInvitationEvent(::std::string const& action, ObjectGuid inviterGuid, ::std::string const& reason) const
{
    Player* inviter = ObjectAccessor::FindPlayer(inviterGuid);
    ::std::string inviterName = inviter ? inviter->GetName() : inviterGuid.ToString();

    if (reason.empty())
    {
        TC_LOG_INFO("playerbot.group", "Bot {} {} invitation from {}",
            _bot->GetName(), action, inviterName);
    }
    else
    {
        TC_LOG_INFO("playerbot.group", "Bot {} {} invitation from {} ({})",
            _bot->GetName(), action, inviterName, reason);
    }
}

void GroupInvitationHandler::UpdateStatistics(bool accepted, ::std::chrono::milliseconds responseTime)
{
    // Update average response time with moving average
    if (_stats.acceptedInvitations + _stats.declinedInvitations > 0)
    {
        uint32 totalResponses = _stats.acceptedInvitations + _stats.declinedInvitations;
        _stats.averageResponseTime = ::std::chrono::milliseconds(
            (_stats.averageResponseTime.count() * (totalResponses - 1) + responseTime.count()) / totalResponses
        );
    }
    else
    {
        _stats.averageResponseTime = responseTime;
    }
}

bool GroupInvitationHandler::ProcessNextInvitation()
{
    // CRITICAL: Use try_lock to avoid deadlock - if mutex is busy, skip this update and try next time
    ::std::unique_lock lock(_invitationMutex, ::std::try_to_lock);
    if (!lock.owns_lock())
    {
        // Mutex is busy, skip this update cycle
        return false;
    }


    // Check if we're already processing an invitation
    if (!_currentInviter.IsEmpty())
    {
        TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Already processing invitation from {} for bot {}",
            _currentInviter.ToString(), _bot->GetName());

        // Check if enough time has passed for response delay
        auto now = ::std::chrono::steady_clock::now();
        auto timeSinceInvite = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
            now - _stats.lastInvitation);

        TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Time since invite: {}ms, Response delay: {}ms",
            timeSinceInvite.count(), _responseDelayMs);

        if (timeSinceInvite.count() >= _responseDelayMs)
        {
            // Time to accept the invitation - call internal methods to avoid deadlock
            TC_LOG_INFO("playerbot", "GroupInvitationHandler: Processing invitation acceptance for bot {} from {}",
                _bot->GetName(), _currentInviter.ToString());

            if (ShouldAcceptInvitation(_currentInviter))
            {
                TC_LOG_INFO("playerbot", "GroupInvitationHandler: Bot {} accepting invitation from {}",
                    _bot->GetName(), _currentInviter.ToString());
                AcceptInvitationInternal(_currentInviter);
            }
            else
            {
                TC_LOG_INFO("playerbot", "GroupInvitationHandler: Bot {} declining invitation from {}",
                    _bot->GetName(), _currentInviter.ToString());
                DeclineInvitationInternal(_currentInviter, "Validation failed");
            }
            _currentInviter = ObjectGuid::Empty;
        }
        return true;
    }

    // Process next invitation from queue
    if (_pendingInvitations.empty())
    {
        return false;
    }

    PendingInvitation& invitation = _pendingInvitations.front();

    TC_LOG_INFO("playerbot", "GroupInvitationHandler: Processing queued invitation from {} for bot {} (age: {}ms)",
        invitation.inviterName, _bot->GetName(),
        ::std::chrono::duration_cast<::std::chrono::milliseconds>(::std::chrono::steady_clock::now() - invitation.timestamp).count());

    // Check if invitation has expired
    auto now = ::std::chrono::steady_clock::now();
    auto age = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
        now - invitation.timestamp);

    if (age.count() > INVITATION_TIMEOUT)
    {
        TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Invitation from {} expired ({}ms old)",
            invitation.inviterName, age.count());
        _pendingInvitations.pop();
        _stats.invalidInvitations++;
        return ProcessNextInvitation(); // Try next one
    }

    // Start processing this invitation
    _currentInviter = invitation.inviterGuid;
    _pendingInvitations.pop();

    TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Processing invitation from {} for bot {}",
        invitation.inviterName, _bot->GetName());

    return true;
}

void GroupInvitationHandler::CleanupExpiredInvitations()
{
    ::std::lock_guard lock(_invitationMutex);

    // Clean up expired invitations from queue
    ::std::queue<PendingInvitation> validInvitations;
    auto now = ::std::chrono::steady_clock::now();

    while (!_pendingInvitations.empty())
    {
        PendingInvitation& invitation = _pendingInvitations.front();
        auto age = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
            now - invitation.timestamp);

        if (age.count() <= INVITATION_TIMEOUT)
        {
            validInvitations.push(invitation);
        }
        else
        {
            TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Cleaned up expired invitation from {}",
                invitation.inviterName);
            _stats.invalidInvitations++;
        }

        _pendingInvitations.pop();
    }

    _pendingInvitations = validInvitations;

    // Clean up old inviters from recent list
    auto cutoffTime = now - ::std::chrono::milliseconds(RECENT_INVITER_MEMORY);
    if (_lastAcceptTime < cutoffTime)
    {
        _recentInviters.clear();
    }
}

bool GroupInvitationHandler::AcceptInvitationInternal(ObjectGuid inviterGuid)
{
    // NOTE: This method assumes _invitationMutex is already locked by the caller

    // EXECUTION MARKER: Verify this method is being called
    TC_LOG_INFO("playerbot.debug", "=== EXECUTION MARKER: AcceptInvitationInternal() called for bot {} from inviter {} ===",
        _bot ? _bot->GetName() : "NULL", inviterGuid.ToString());

    TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Bot {} accepting invitation from {}",
        _bot->GetName(), inviterGuid.ToString());

    // Record accept time
    auto acceptTime = ::std::chrono::steady_clock::now();

    // Send accept packet
    if (!SendAcceptPacket())
    {
        TC_LOG_ERROR("playerbot", "GroupInvitationHandler: Failed to send accept packet for bot {}", _bot->GetName());
        return false;
    }

    // Update statistics
    _stats.acceptedInvitations++;
    if (_stats.lastInvitation != ::std::chrono::steady_clock::time_point{})
    {
        auto responseTime = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
            acceptTime - _stats.lastInvitation);
        UpdateStatistics(true, responseTime);
    }

    // Remember this inviter to prevent loops
    _recentInviters.insert(inviterGuid);
    _lastAcceptTime = acceptTime;

    // Clear current inviter
    _currentInviter = ObjectGuid::Empty;

    LogInvitationEvent("ACCEPTED", inviterGuid);

    // CRITICAL: Activate follow behavior after successful group join
    // We need to check if the bot is actually in a group now and activate follow AI
    if (_bot->GetGroup())
    {
        TC_LOG_INFO("module.playerbot.group", "GroupInvitationHandler: Bot {} successfully joined group, activating follow behavior", _bot->GetName());

        // Get the bot's AI and trigger group join handler with detailed debugging
        auto* session = _bot->GetSession();
        if (!session)
        {
            TC_LOG_ERROR("module.playerbot.group", "FOLLOW FIX DEBUG PATH2: Bot {} has no session!", _bot->GetName());
            return true;
        }
        TC_LOG_INFO("module.playerbot.group", "FOLLOW FIX DEBUG PATH2: Bot {} has session, attempting BotSession cast", _bot->GetName());

        auto* botSession = dynamic_cast<BotSession*>(session);
        if (!botSession)
        {
            TC_LOG_ERROR("module.playerbot.group", "FOLLOW FIX DEBUG PATH2: Bot {} session is not a BotSession!", _bot->GetName());
            return true;
        }
        TC_LOG_INFO("module.playerbot.group", "FOLLOW FIX DEBUG PATH2: Bot {} has BotSession, getting AI", _bot->GetName());

        auto* botAI = botSession->GetAI();
        if (!botAI)
        {
            TC_LOG_ERROR("module.playerbot.group", "FOLLOW FIX DEBUG PATH2: Bot {} BotSession has no AI!", _bot->GetName());
            return true;
        }
        TC_LOG_INFO("module.playerbot.group", "FOLLOW FIX DEBUG PATH2: Bot {} has AI, calling OnGroupJoined", _bot->GetName());

        botAI->OnGroupJoined(_bot->GetGroup());
        TC_LOG_INFO("module.playerbot.group", "FOLLOW FIX DEBUG PATH2: Bot {} OnGroupJoined call completed", _bot->GetName());

        // BACKUP FIX: Directly activate follow strategy as fallback
        TC_LOG_INFO("module.playerbot.group", "FOLLOW FIX BACKUP PATH2: Bot {} directly activating follow strategy", _bot->GetName());
        if (botAI->GetStrategy("follow"))
        {
            TC_LOG_INFO("module.playerbot.group", "FOLLOW FIX BACKUP PATH2: Bot {} already has follow strategy", _bot->GetName());
        }
        else
        {
            botAI->ActivateStrategy("follow");
            TC_LOG_INFO("module.playerbot.group", "FOLLOW FIX BACKUP PATH2: Bot {} activated follow strategy directly", _bot->GetName());
        }
    }

    return true;
}

void GroupInvitationHandler::DeclineInvitationInternal(ObjectGuid inviterGuid, ::std::string const& reason)
{
    // NOTE: This method assumes _invitationMutex is already locked by the caller

    TC_LOG_DEBUG("playerbot", "GroupInvitationHandler: Bot {} declining invitation from {} ({})",
        _bot->GetName(), inviterGuid.ToString(), reason);

    // Send decline packet
    SendDeclinePacket(reason);

    // Update statistics
    _stats.declinedInvitations++;

    // Clear current inviter
    if (_currentInviter == inviterGuid)
        _currentInviter = ObjectGuid::Empty;

    LogInvitationEvent("DECLINED", inviterGuid, reason);
}

} // namespace Playerbot
