/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#ifndef BOT_SESSION_H
#define BOT_SESSION_H

#include "WorldSession.h"
#include "Threading/LockHierarchy.h"
#include "WorldSocket.h"
#include "Define.h"
#include "DatabaseEnv.h"
#include "QueryHolder.h"
#include "Socket.h"
#include <atomic>
#include <memory>
#include <queue>
#include <mutex>
#include <chrono>
#include <unordered_set>

// Forward declaration for bot socket stub
namespace Playerbot {
    class BotSocketStub;
}

// Forward declarations
namespace Playerbot {
class BotAI;
class BotPacketSimulator;
}

namespace Playerbot {

// Use TrinityCore's native LoginQueryHolder (defined in CharacterHandler.cpp)
// No forward declaration needed - we'll include it in the .cpp file

// Forward declaration - implement in BotSession.cpp
class BotSocket;

/**
 * Simplified BotSession to eliminate TBB deadlock issues
 *
 * DEADLOCK FIX: Removed all TBB and Boost dependencies that were causing
 * "resource deadlock would occur" exceptions during BotSession creation.
 *
 * Uses simple std:: containers with mutex protection instead of complex
 * lock-free structures to ensure stability during character creation.
 */
class TC_GAME_API BotSession final : public WorldSession
{
public:
    // Bot-specific login handling (forward declaration moved to public)
    class BotLoginQueryHolder : public CharacterDatabaseQueryHolder
    {
    private:
        uint32 m_accountId;
        ObjectGuid m_guid;
    public:
        BotLoginQueryHolder(uint32 accountId, ObjectGuid guid)
            : m_accountId(accountId), m_guid(guid) { }
        ObjectGuid GetGuid() const { return m_guid; }
        uint32 GetAccountId() const { return m_accountId; }
        bool Initialize();
    };

    explicit BotSession(uint32 bnetAccountId);

    // Factory method to create BotSession with dummy socket
    static ::std::shared_ptr<BotSession> Create(uint32 bnetAccountId);

    virtual ~BotSession();

    // === WorldSession Overrides ===
    void SendPacket(WorldPacket const* packet, bool forced = false);
    // Note: QueuePacket hides WorldSession::QueuePacket (not virtual in base)
    // Bot sessions store packets in their own queue for bot-specific processing
    void QueuePacket(WorldPacket&& packet);  // TrinityCore 12.0 signature
    void QueuePacketLegacy(WorldPacket* packet);      // Legacy compatibility (takes ownership)
    bool Update(uint32 diff, PacketFilter& updater);


    // Override socket-related methods to handle socketless sessions
    bool PlayerDisconnected() const;  // Always return false for bots

    // Query methods for Trinity compatibility
    bool IsConnectionIdle() const { return false; }
    uint32 GetLatency() const { return _simulatedLatency; }

    // Socket safety overrides for bot sessions
    bool HasSocket() const { return false; }
    bool IsSocketOpen() const { return false; }
    void CloseSocket() { /* No socket to close */ }

    // === Bot-Specific Methods ===
    void ProcessBotPackets();

    // Group invitation handling
    void HandleGroupInvitation(WorldPacket const& packet);

    // === Deferred Packet System (Main Thread Processing) ===
    /**
     * @brief Queue packet for main thread execution (thread-safe)
     * @param packet Packet to defer (ownership transferred)
     *
     * Used for packets that modify game state and require serialization
     * with Map::Update() to prevent race conditions.
     *
     * Called from: Bot worker threads (ProcessBotPackets)
     * Executed by: Main world thread (World::UpdateSessions)
     */
    void QueueDeferredPacket(::std::unique_ptr<WorldPacket> packet);

    /**
     * @brief Process all deferred packets (main thread only!)
     * @return Number of packets processed
     *
     * CRITICAL: Must only be called from World::UpdateSessions() on main thread
     * Processes packets that were deferred due to race condition risk.
     */
    uint32 ProcessDeferredPackets();

    /**
     * @brief Check if session has deferred packets pending
     * @return true if deferred packet queue is non-empty
     */
    bool HasDeferredPackets() const;


    // Character Login System (SYNCHRONOUS Pattern)
    bool LoginCharacter(ObjectGuid characterGuid);

    // Simplified login state tracking for synchronous operation
    enum class LoginState : uint8
    {
        NONE,                   // Not logging in
        LOGIN_IN_PROGRESS,      // LoginCharacter() executing synchronously
        LOGIN_COMPLETE,         // Login successful
        LOGIN_FAILED            // Login failed
    };

    LoginState GetLoginState() const { return _loginState.load(); }
    bool IsLoginComplete() const { return _loginState.load() == LoginState::LOGIN_COMPLETE; }
    bool IsLoginFailed() const { return _loginState.load() == LoginState::LOGIN_FAILED; }

    // Bot identification
    bool IsBot() const { return true; }
    bool IsActive() const { return _active.load(); }

    // ========================================================================
    // INSTANCE BOT LIFECYCLE MANAGEMENT
    // ========================================================================
    // Instance bots (JIT/Warm Pool) are special-purpose bots created exclusively
    // for dungeons, raids, battlegrounds, and arenas. They should NOT do quests
    // or other open-world activities. When not in a queue or group, they should
    // automatically log out after a short idle period to reduce server load.
    //
    // IDLE TIMEOUT: 60 seconds (1 minute)
    // ========================================================================

    /**
     * @brief Mark this bot as an instance bot (JIT or warm pool)
     *
     * Called by: JITBotFactory, InstanceBotPool during bot creation
     * Effect: Enables automatic logout when not in queue or group
     */
    void SetInstanceBot(bool isInstanceBot);

    /**
     * @brief Check if this is an instance bot
     * @return true if this bot was created for instanced content only
     */
    bool IsInstanceBot() const { return _isInstanceBot.load(); }

    /**
     * @brief Check if the bot is currently in an "active" instance state
     * @return true if bot is in queue, in group, or in instance
     *
     * Active states that prevent idle logout:
     * - In LFG queue
     * - In BG queue
     * - In Arena queue
     * - In a group (any type)
     * - Inside an instance (dungeon/raid/BG/arena)
     */
    bool IsInActiveInstanceState() const;

    /**
     * @brief Update idle state and check if bot should log out
     * @param diff Time since last update in milliseconds
     * @return true if bot should be logged out due to idle timeout
     *
     * Called from BotWorldSessionMgr::UpdateSessions() every tick.
     * Returns true when instance bot has been idle for 60+ seconds.
     */
    bool UpdateIdleStateAndCheckLogout(uint32 diff);

    /**
     * @brief Get idle duration in milliseconds
     * @return Time the bot has been idle (0 if not idle)
     */
    uint32 GetIdleDurationMs() const;

    /**
     * @brief Get idle timeout for instance bots from configuration
     * @return Timeout in milliseconds (configurable via Playerbot.Session.IdleTimeout)
     *
     * Default: 60000ms (1 minute)
     * Instance bots auto-logout after this duration when not in queue/group
     */
    static uint32 GetInstanceBotIdleTimeout();

    /**
     * @brief Get queue timeout for instance bots from configuration
     * @return Timeout in milliseconds (configurable via Playerbot.Session.QueueTimeout)
     *
     * Default: 300000ms (5 minutes)
     * Instance bots logout after waiting in queue this long without content starting
     */
    static uint32 GetInstanceBotQueueTimeout();

    // Process pending async login operations
    void ProcessPendingLogin();

    // Handle async login query holder callback
    void HandleBotPlayerLogin(BotLoginQueryHolder const& holder);

    // AI Integration (public access for GroupInvitationHandler)
    // P1 FIX: Use unique_ptr for automatic memory management (no manual delete needed)
    void SetAI(::std::unique_ptr<BotAI> ai) { _ai = ::std::move(ai); }
    BotAI* GetAI() const { return _ai.get(); }  // Return raw pointer for compatibility

    // ========================================================================
    // THREAD-SAFE FACING SYSTEM
    // ========================================================================
    // SetFacingToObject() manipulates movement splines which is NOT thread-safe.
    // Bot AI runs on worker threads, but movement APIs require main thread.
    // Solution: Queue the facing request atomically, execute on main thread.
    // ========================================================================

    /**
     * @brief Queue a facing request to be executed on main thread (thread-safe)
     * @param targetGuid The GUID of the unit to face
     *
     * Called from: Worker threads (ClassAI::OnCombatUpdate)
     * Executed by: Main thread (BotSession::Update -> ProcessPendingFacing)
     */
    void QueueFacingTarget(ObjectGuid targetGuid);

    /**
     * @brief Process pending facing request on main thread
     * @return true if facing was executed
     *
     * CRITICAL: Must only be called from main thread (BotSession::Update)
     */
    bool ProcessPendingFacing();

    /**
     * @brief Check if there's a pending facing request
     */
    bool HasPendingFacing() const { return !_pendingFacingTarget.IsEmpty(); }

    // ========================================================================
    // THREAD-SAFE MOVEMENT STOP SYSTEM
    // ========================================================================
    // MotionMaster::Clear() is NOT thread-safe (modifies splines, unit state).
    // Bot AI runs on worker threads, but movement APIs require main thread.
    // Solution: Queue the stop request atomically, execute on main thread.
    // ========================================================================

    /**
     * @brief Queue a stop movement request to be executed on main thread (thread-safe)
     *
     * Called from: Worker threads (LeaderFollowBehavior::OnDeactivate, OnGroupLeft)
     * Executed by: Main thread (BotWorldSessionMgr::ProcessAllDeferredPackets)
     */
    void QueueStopMovement();

    /**
     * @brief Process pending stop movement request on main thread
     * @return true if stop was executed
     *
     * CRITICAL: Must only be called from main thread
     */
    bool ProcessPendingStopMovement();

    /**
     * @brief Check if there's a pending stop movement request
     */
    bool HasPendingStopMovement() const { return _pendingStopMovement.load(); }

    // ========================================================================
    // THREAD-SAFE SAFE RESURRECTION SYSTEM (SpawnCorpseBones Crash Fix)
    // ========================================================================
    // HandleReclaimCorpse → SpawnCorpseBones → Map::RemoveWorldObject crashes
    // due to corrupted i_worldObjects tree structure (infinite loop in _Erase).
    //
    // FIX: Instead of using CMSG_RECLAIM_CORPSE packet (which calls SpawnCorpseBones),
    // we queue a safe resurrection request that calls ResurrectPlayer() directly
    // WITHOUT SpawnCorpseBones. The corpse will decay naturally.
    //
    // Thread Safety:
    // - QueueSafeResurrection() called from worker threads (atomic flag)
    // - ProcessPendingSafeResurrection() called from main thread
    // ========================================================================

    /**
     * @brief Queue a safe resurrection request to be executed on main thread
     *
     * Called from: Worker threads (DeathRecoveryManager::HandleAtCorpse)
     * Executed by: Main thread (BotWorldSessionMgr::ProcessAllDeferredPackets)
     *
     * This bypasses HandleReclaimCorpse/SpawnCorpseBones crash by calling
     * ResurrectPlayer() directly on the main thread.
     */
    void QueueSafeResurrection();

    /**
     * @brief Process pending safe resurrection request on main thread
     * @return true if resurrection was executed
     *
     * CRITICAL: Must only be called from main thread
     */
    bool ProcessPendingSafeResurrection();

    /**
     * @brief Check if there's a pending safe resurrection request
     */
    bool HasPendingSafeResurrection() const { return _pendingSafeResurrection.load(); }

    /**
     * @brief Queue a creature for looting on main thread (thread-safe)
     * @param creatureGuid GUID of the creature corpse to loot
     *
     * Called from worker threads to defer SendLoot to main thread.
     * Prevents ACCESS_VIOLATION crash in Map::SendObjectUpdates.
     */
    void QueueLootTarget(ObjectGuid creatureGuid);

    /**
     * @brief Process pending loot targets on main thread
     * @return true if any loot was processed
     *
     * MUST be called from main thread only.
     */
    bool ProcessPendingLoot();

    /**
     * @brief Check if there are pending loot targets
     */
    bool HasPendingLoot() const;

    // ========================================================================
    // THREAD-SAFE OBJECT USE QUEUE (GameObject::Use Crash Fix)
    // ========================================================================
    // GameObject::Use() is NOT thread-safe - it modifies game object state,
    // loot tables, and triggers Map updates that must only happen on main thread.
    //
    // Crash: Map::SendObjectUpdates ACCESS_VIOLATION when Use() called from worker
    //
    // Solution: Queue object use requests from worker threads, process on main.
    // ========================================================================

    /**
     * @brief Queue a game object for Use() on main thread (thread-safe)
     * @param objectGuid GUID of the game object to use
     *
     * Called from worker threads (LootStrategy::LootObject) to defer
     * GameObject::Use() to main thread. Prevents ACCESS_VIOLATION crash.
     */
    void QueueObjectUse(ObjectGuid objectGuid);

    /**
     * @brief Process pending object use requests on main thread
     * @return true if any object was used
     *
     * MUST be called from main thread only.
     */
    bool ProcessPendingObjectUse();

    /**
     * @brief Check if there are pending object use requests
     */
    bool HasPendingObjectUse() const;

    // ========================================================================
    // THREAD-SAFE LFG PROPOSAL AUTO-ACCEPT (Re-entrant Crash Fix)
    // ========================================================================
    // LFGMgr::UpdateProposal() iterates proposal.players map and calls
    // SendLfgUpdateProposal() for each player. For bots, this triggers
    // BotSession::SendPacket() which would call UpdateProposal() again
    // to auto-accept - causing iterator invalidation crash.
    //
    // Solution: Queue accepts during packet interception, process on main thread.
    // ========================================================================

    /**
     * @brief Queue an LFG proposal accept to be executed on main thread (thread-safe)
     * @param proposalId The LFG proposal ID to accept
     *
     * Called from: SendPacket() when intercepting SMSG_LFG_PROPOSAL_UPDATE
     * Executed by: Main thread (BotWorldSessionMgr::ProcessAllDeferredPackets)
     *
     * This defers UpdateProposal() call to prevent re-entrant iterator crash.
     */
    void QueueLfgProposalAccept(uint32 proposalId);

    /**
     * @brief Process pending LFG proposal accepts on main thread
     * @return true if any proposal was accepted
     *
     * CRITICAL: Must only be called from main thread
     */
    bool ProcessPendingLfgProposalAccepts();

    /**
     * @brief Check if there are pending LFG proposal accepts
     */
    bool HasPendingLfgProposalAccepts() const;

private:
    // Helper methods for safe database access
    CharacterDatabasePreparedStatement* GetSafePreparedStatement(CharacterDatabaseStatements statementId, const char* statementName);

    // Removed AsyncLoginState - using async approach now

    // Account accessors
    uint32 GetBnetAccountId() const { return _bnetAccountId; }
    uint32 GetLegacyAccountId() const { return GetAccountId(); }

private:
    // Simple packet queues - NO TBB, NO BOOST
    // ST-2 FIX: Changed from recursive_timed_mutex to simple mutex
    // Analysis shows no recursive locking occurs, and try_lock_for timeouts
    // were causing packet processing deferrals under high load.
    // Simple mutex with blocking wait is more reliable than timeout failures.
    ::std::queue<::std::unique_ptr<WorldPacket>> _incomingPackets;
    ::std::queue<::std::unique_ptr<WorldPacket>> _outgoingPackets;
    mutable ::std::mutex _packetMutex;
    mutable ::std::timed_mutex _updateMutex;  // TIMED MUTEX: Prevents both deadlock AND race conditions

    // Deferred packet queue (main thread processing)
    // Packets that require serialization with Map::Update() to prevent race conditions
    ::std::queue<::std::unique_ptr<WorldPacket>> _deferredPackets;
    mutable OrderedMutex<LockOrder::SESSION_MANAGER> _deferredPacketMutex; // Simple mutex (no recursion needed)

    // Bot state
    ::std::atomic<bool> _active{true};
    ::std::atomic<bool> _destroyed{false};

    // DEADLOCK FIX: Lock-free packet processing flag
    ::std::atomic<bool> _packetProcessing{false};

    // Synchronous login state machine
    ::std::atomic<LoginState> _loginState{LoginState::NONE};

    // Bot AI system
    // P1 FIX: unique_ptr for automatic memory management - no manual delete in destructor
    ::std::unique_ptr<BotAI> _ai;

    // Packet simulation system (Phase 1 refactoring)
    ::std::unique_ptr<BotPacketSimulator> _packetSimulator;

    // Account information
    uint32 _bnetAccountId;
    uint32 _simulatedLatency{50};

    // NOTE: Player loading state (m_playerLoading) is inherited from WorldSession
    // BotSession is a friend class so it can access the base class's private member
    // DO NOT redeclare m_playerLoading here - it shadows the base class and breaks WHO list!

    // Thread-safe pending facing target
    // Worker threads set this, main thread processes it
    // Uses atomic operations for lock-free thread safety
    mutable std::mutex _facingMutex;
    ObjectGuid _pendingFacingTarget;

    // Thread-safe pending stop movement flag
    // Worker threads set this, main thread processes it
    std::atomic<bool> _pendingStopMovement{false};

    // Thread-safe pending safe resurrection flag (SpawnCorpseBones crash fix)
    // Worker threads set this, main thread processes it
    std::atomic<bool> _pendingSafeResurrection{false};

    // Thread-safe pending loot queue (SendLoot crash fix)
    // Worker threads queue targets, main thread processes loot
    mutable std::mutex _pendingLootMutex;
    std::vector<ObjectGuid> _pendingLootTargets;

    // Thread-safe pending object use queue (GameObject::Use crash fix)
    // Worker threads queue objects, main thread calls Use()
    mutable std::mutex _pendingObjectUseMutex;
    std::vector<ObjectGuid> _pendingObjectUseTargets;

    // LFG proposal auto-accept tracking (infinite loop prevention)
    // Tracks which proposal IDs have already been auto-accepted to avoid re-accepting
    // when UpdateProposal() sends SMSG_LFG_PROPOSAL_UPDATE back to all players
    mutable std::mutex _lfgProposalMutex;
    std::unordered_set<uint32> _autoAcceptedProposals;

    // Pending LFG proposal accepts (deferred to main thread to avoid re-entrant crash)
    // RE-ENTRANT CRASH FIX: UpdateProposal() iterates proposal.players map and calls
    // SendLfgUpdateProposal() which triggers BotSession::SendPacket(). If SendPacket()
    // calls UpdateProposal() synchronously, the iterator is invalidated = crash.
    // Solution: Queue accepts during packet handling, process on main thread later.
    mutable std::mutex _pendingLfgAcceptsMutex;
    std::vector<uint32> _pendingLfgProposalAccepts;

    // LFG instance sync timeout tracking
    // Tracks when bot started waiting for group members to enter dungeon first
    // After timeout (30 seconds), bot proceeds anyway to avoid being stuck forever
    std::atomic<uint32> _instanceSyncWaitStartMs{0};

    // ========================================================================
    // INSTANCE BOT LIFECYCLE TRACKING
    // ========================================================================
    // Instance bots (JIT/warm pool) are for instanced content only.
    // They auto-logout after 60 seconds of being idle (not in queue/group).
    // ========================================================================

    /// Whether this bot is an instance bot (JIT or warm pool)
    std::atomic<bool> _isInstanceBot{false};

    /// Time when bot was marked as instance bot (for queue timeout)
    std::atomic<uint32> _instanceBotStartTime{0};

    /// Accumulated idle time in milliseconds
    /// Reset to 0 when bot enters queue or group, incremented when idle
    std::atomic<uint32> _idleAccumulatorMs{0};

    /// Accumulated queue time in milliseconds (time spent waiting in queue without content starting)
    /// This prevents bots from sitting in queue forever when BG/LFG never pops
    std::atomic<uint32> _queueAccumulatorMs{0};

    /// Whether bot was active (in queue/group) last check
    /// Used to detect transition from active to idle
    std::atomic<bool> _wasActiveLastCheck{true};

    /// Whether bot has ever entered actual instanced content (dungeon/BG/arena)
    /// Used to distinguish "waiting in queue" from "actually playing"
    std::atomic<bool> _hasEnteredInstance{false};

    // Deleted copy operations
    BotSession(BotSession const&) = delete;
    BotSession& operator=(BotSession const&) = delete;
    BotSession(BotSession&&) = delete;
    BotSession& operator=(BotSession&&) = delete;
};

} // namespace Playerbot

#endif // BOT_SESSION_H