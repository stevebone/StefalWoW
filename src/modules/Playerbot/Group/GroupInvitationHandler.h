/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_GROUP_INVITATION_HANDLER_H
#define PLAYERBOT_GROUP_INVITATION_HANDLER_H

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include "WorldPacket.h"
#include <memory>
#include <chrono>
#include <mutex>
#include <unordered_set>
#include <queue>

// Forward declarations
class Player;
class Group;
class WorldSession;

namespace WorldPackets
{
    namespace Party
    {
        class PartyInvite;
    }
}

namespace Playerbot
{

/**
 * @class GroupInvitationHandler
 * @brief Critical component for automated group invitation acceptance for bots
 *
 * This handler enables bots to automatically accept group invitations within 2 seconds,
 * with proper validation and thread-safety. It integrates directly with TrinityCore's
 * group system and handles all packet processing for group invitations.
 *
 * Performance Requirements:
 * - Response time: <2 seconds from invitation to acceptance
 * - Memory usage: <100KB additional per bot
 * - Thread-safe operation with mutex protection
 * - CPU usage: <0.01% per bot for invitation processing
 */
class TC_GAME_API GroupInvitationHandler
{
public:
    /**
     * Constructor
     * @param bot The player bot instance this handler manages
     */
    explicit GroupInvitationHandler(Player* bot);

    /**
     * Destructor - ensures clean shutdown
     */
    ~GroupInvitationHandler();

    // Core invitation handling
    /**
     * Handle incoming group invitation packet
     * @param packet The SMSG_PARTY_INVITE packet from TrinityCore
     * @return true if invitation was processed successfully
     */
    bool HandleInvitation(WorldPackets::Party::PartyInvite const& packet);

    /**
     * Process pending invitations with auto-accept logic
     * @param diff Time elapsed since last update in milliseconds
     */
    void Update(uint32 diff);

    // Validation methods
    /**
     * Determine if bot should accept invitation from inviter
     * @param inviterGuid The GUID of the player inviting the bot
     * @return true if invitation should be accepted
     */
    bool ShouldAcceptInvitation(ObjectGuid inviterGuid) const;

    /**
     * Validate if inviter is legitimate and not another bot
     * @param inviter The player attempting to invite
     * @return true if inviter is valid (not a bot, within range, etc.)
     */
    bool IsValidInviter(Player* inviter) const;

    /**
     * Check if bot can join the specified group
     * @param group The group to potentially join
     * @return true if bot meets requirements to join
     */
    bool CanJoinGroup(Group* group) const;

    // Action methods
    /**
     * Accept pending group invitation
     * @param inviterGuid GUID of the inviter
     * @return true if acceptance was successful
     */
    bool AcceptInvitation(ObjectGuid inviterGuid);

    /**
     * Decline group invitation
     * @param inviterGuid GUID of the inviter
     * @param reason Optional reason for declining
     */
    void DeclineInvitation(ObjectGuid inviterGuid, ::std::string const& reason = "");

    // State management
    /**
     * Check if bot has pending invitations
     * @return true if there are unprocessed invitations
     */
    bool HasPendingInvitation() const;

    /**
     * Get the current pending inviter GUID
     * @return ObjectGuid of current inviter, or Empty if none
     */
    ObjectGuid GetPendingInviter() const;

    /**
     * Clear all pending invitations
     */
    void ClearPendingInvitations();

    // Configuration
    /**
     * Set whether to auto-accept all valid invitations
     * @param enable true to enable auto-accept
     */
    void SetAutoAccept(bool enable) { _autoAcceptEnabled = enable; }

    /**
     * Check if auto-accept is enabled
     * @return true if auto-accept is enabled
     */
    bool IsAutoAcceptEnabled() const { return _autoAcceptEnabled; }

    /**
     * Set the response delay for accepting invitations
     * @param delayMs Delay in milliseconds (100-2000ms)
     */
    void SetResponseDelay(uint32 delayMs);

    // Statistics and monitoring
    struct InvitationStats
    {
        uint32 totalInvitations = 0;
        uint32 acceptedInvitations = 0;
        uint32 declinedInvitations = 0;
        uint32 invalidInvitations = 0;
        ::std::chrono::milliseconds averageResponseTime{0};
        ::std::chrono::steady_clock::time_point lastInvitation;
    };

    /**
     * Get current invitation statistics
     * @return Current stats for monitoring
     */
    InvitationStats const& GetStats() const { return _stats; }

    // Integration helpers
    /**
     * Get the bot player instance
     * @return Pointer to the bot player
     */
    Player* GetBot() const { return _bot; }

    /**
     * Get the bot's world session
     * @return Pointer to the world session
     */
    WorldSession* GetSession() const;

private:
    // Internal data structures
    struct PendingInvitation
    {
        ObjectGuid inviterGuid;
        ::std::string inviterName;
        uint32 proposedRoles;
        ::std::chrono::steady_clock::time_point timestamp;
        bool isProcessing = false;
    };

    // Internal methods
    /**
     * Send accept packet to server
     * @return true if packet was sent successfully
     */
    bool SendAcceptPacket();

    /**
     * Send decline packet to server
     * @param reason Optional reason for declining
     */
    void SendDeclinePacket(::std::string const& reason);

    /**
     * Validate invitation against anti-loop logic
     * @param inviterGuid The inviter to validate
     * @return true if invitation doesn't create a loop
     */
    bool ValidateNoInvitationLoop(ObjectGuid inviterGuid) const;

    /**
     * Check if inviter is within acceptable range
     * @param inviter The inviter to check
     * @return true if inviter is within range (100 yards default)
     */
    bool IsInviterInRange(Player* inviter) const;

    /**
     * Log invitation event for debugging
     * @param action The action taken (accepted/declined/ignored)
     * @param inviterGuid The inviter involved
     * @param reason Optional reason for the action
     */
    void LogInvitationEvent(::std::string const& action, ObjectGuid inviterGuid, ::std::string const& reason = "") const;

    /**
     * Update statistics after processing invitation
     * @param accepted Whether invitation was accepted
     * @param responseTime Time taken to respond
     */
    void UpdateStatistics(bool accepted, ::std::chrono::milliseconds responseTime);

    /**
     * Process the oldest pending invitation
     * @return true if an invitation was processed
     */
    bool ProcessNextInvitation();

    /**
     * Clean up expired invitations (>30 seconds old)
     */
    void CleanupExpiredInvitations();

    /**
     * Internal accept invitation method (assumes mutex is already locked)
     * @param inviterGuid GUID of the inviter
     * @return true if acceptance was successful
     */
    bool AcceptInvitationInternal(ObjectGuid inviterGuid);

    /**
     * Internal decline invitation method (assumes mutex is already locked)
     * @param inviterGuid GUID of the inviter
     * @param reason Optional reason for declining
     */
    void DeclineInvitationInternal(ObjectGuid inviterGuid, ::std::string const& reason = "");

    // Member variables
    Player* _bot;                                           // The bot player instance
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::GROUP_MANAGER> _invitationMutex;                   // Thread safety mutex
    ::std::queue<PendingInvitation> _pendingInvitations;     // Queue of pending invitations
    ObjectGuid _currentInviter;                            // Currently processing inviter

    // Configuration
    bool _autoAcceptEnabled = true;                        // Auto-accept enabled by default
    uint32 _responseDelayMs = 500;                         // Default 500ms delay for human-like response
    uint32 _maxInvitationsPerMinute = 10;                  // Anti-spam protection
    float _maxAcceptRange = 100.0f;                        // Maximum range to accept invitations

    // Tracking
    ::std::unordered_set<ObjectGuid> _recentInviters;        // Track recent inviters to prevent loops
    ::std::chrono::steady_clock::time_point _lastAcceptTime; // Time of last acceptance
    InvitationStats _stats;                                // Performance statistics

    // Timers
    uint32 _updateTimer = 0;                               // Update timer for processing
    uint32 _cleanupTimer = 0;                              // Cleanup timer for expired invitations

    // Constants
    static constexpr uint32 MIN_RESPONSE_DELAY = 100;      // Minimum 100ms delay
    static constexpr uint32 MAX_RESPONSE_DELAY = 2000;     // Maximum 2000ms delay
    static constexpr uint32 INVITATION_TIMEOUT = 30000;    // 30 second timeout
    static constexpr uint32 UPDATE_INTERVAL = 100;         // Check every 100ms
    static constexpr uint32 CLEANUP_INTERVAL = 5000;       // Cleanup every 5 seconds
    static constexpr uint32 RECENT_INVITER_MEMORY = 60000; // Remember inviters for 60 seconds
};

} // namespace Playerbot

#endif // PLAYERBOT_GROUP_INVITATION_HANDLER_H