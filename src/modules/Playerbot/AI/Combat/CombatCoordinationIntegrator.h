/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Sprint 3: Combat Coordination Integration Layer
 * Bridges existing combat managers with BotMessageBus claim system
 */

#ifndef PLAYERBOT_COMBAT_COORDINATION_INTEGRATOR_H
#define PLAYERBOT_COMBAT_COORDINATION_INTEGRATOR_H

#include "Define.h"
#include "ObjectGuid.h"
#include "AI/Coordination/Messaging/BotMessageBus.h"
#include "AI/Coordination/Messaging/ClaimResolver.h"
#include "Cooldown/CooldownEventBus.h"
#include "Cooldown/MajorCooldownTracker.h"
#include <memory>
#include <functional>
#include <unordered_map>
#include <chrono>

class Player;
class Group;
class Unit;

namespace Playerbot
{

class BotAI;
class InterruptCoordinatorFixed;
class DefensiveBehaviorManager;
class CrowdControlManager;
class DispelCoordinator;

/**
 * @brief External defensive cooldown tiers for coordination
 *
 * Per GAP 3: Major CDs should not be wasted on moderate danger
 */
enum class ExternalCDTier : uint8
{
    TIER_MAJOR,     // Guardian Spirit, Pain Suppression, Ironbark, Life Cocoon
    TIER_MODERATE,  // Blessing of Sacrifice, Vigilance
    TIER_MINOR,     // Power Word: Barrier (group), Darkness, AMZ
    TIER_RAID,      // Rallying Cry, Spirit Link, Devotion Aura, Healing Tide
};

/**
 * @brief Danger level for external CD requests
 */
enum class DangerLevel : uint8
{
    NONE = 0,           // No danger
    MODERATE = 1,       // Sustained damage, manageable
    HIGH = 2,           // Spike incoming, need protection
    CRITICAL = 3,       // Death imminent without intervention
    PRE_DANGER = 4,     // Boss ability incoming (predictive)
};

/**
 * @brief Tracked protection window for a target
 *
 * Per GAP 3: 6s window where target is "protected" after external CD
 */
struct ProtectionWindow
{
    ObjectGuid targetGuid;
    ObjectGuid protectorGuid;
    uint32 spellId = 0;
    ExternalCDTier tier = ExternalCDTier::TIER_MINOR;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;

    bool IsActive() const
    {
        return std::chrono::steady_clock::now() < endTime;
    }

    uint32 GetRemainingMs() const
    {
        auto now = std::chrono::steady_clock::now();
        if (now >= endTime) return 0;
        return static_cast<uint32>(
            std::chrono::duration_cast<std::chrono::milliseconds>(endTime - now).count());
    }
};

/**
 * @brief External CD database entry
 */
struct ExternalCDInfo
{
    uint32 spellId;
    ExternalCDTier tier;
    uint32 cooldownMs;
    uint32 durationMs;
    bool isGroupwide;       // AMZ, Barrier, Rallying Cry
    bool requiresTarget;    // Pain Suppression, Guardian Spirit

    static const std::unordered_map<uint32, ExternalCDInfo>& GetDatabase();
};

/**
 * @class CombatCoordinationIntegrator
 * @brief Integrates combat managers with BotMessageBus claim system
 *
 * Sprint 3 Core Component:
 * - Routes interrupt claims through BotMessageBus
 * - Routes dispel claims through BotMessageBus
 * - Routes external defensive CD claims
 * - Manages protection windows (GAP 3 fix)
 * - Coordinates CC claims with DR awareness
 *
 * Performance: <0.05ms per Update() per bot
 */
class TC_GAME_API CombatCoordinationIntegrator
{
public:
    explicit CombatCoordinationIntegrator(BotAI* ai);
    ~CombatCoordinationIntegrator();

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /**
     * Initialize with references to existing managers
     */
    void Initialize(
        InterruptCoordinatorFixed* interruptCoord,
        DefensiveBehaviorManager* defensiveMgr,
        CrowdControlManager* ccMgr,
        DispelCoordinator* dispelCoord
    );

    /**
     * Shutdown and cleanup subscriptions
     */
    void Shutdown();

    /**
     * Main update loop - processes claims and coordinates actions
     * @param diff Milliseconds since last update
     */
    void Update(uint32 diff);

    // ========================================================================
    // INTERRUPT COORDINATION (S3.4)
    // ========================================================================

    /**
     * Request to interrupt a spell via claim system
     * @param targetGuid Caster to interrupt
     * @param spellId Spell being cast
     * @param priority Interrupt priority
     * @return true if claim was submitted
     */
    bool RequestInterrupt(ObjectGuid targetGuid, uint32 spellId, ClaimPriority priority);

    /**
     * Check if this bot should interrupt (has active claim)
     * @param targetGuid Out: target to interrupt
     * @param spellId Out: spell to use
     * @return true if this bot should interrupt now
     */
    bool ShouldInterrupt(ObjectGuid& targetGuid, uint32& spellId) const;

    /**
     * Report interrupt result
     */
    void OnInterruptExecuted(ObjectGuid targetGuid, uint32 spellId, bool success);

    // ========================================================================
    // DISPEL COORDINATION (S3.2 - GAP 2 Fix)
    // ========================================================================

    /**
     * Request to dispel a debuff via claim system
     * @param targetGuid Friendly with debuff
     * @param auraId Debuff to dispel
     * @param priority Dispel urgency
     * @return true if claim was submitted
     */
    bool RequestDispel(ObjectGuid targetGuid, uint32 auraId, ClaimPriority priority);

    /**
     * Check if this bot should dispel (has active claim)
     * @param targetGuid Out: target to dispel
     * @param auraId Out: aura to dispel
     * @return true if this bot should dispel now
     */
    bool ShouldDispel(ObjectGuid& targetGuid, uint32& auraId) const;

    /**
     * Report dispel result
     */
    void OnDispelExecuted(ObjectGuid targetGuid, uint32 auraId, bool success);

    /**
     * Calculate dispel priority score
     * Per S3.2: Priority = base + canDispelType + distance + cdStatus + isHealer + gcdFree
     */
    float CalculateDispelPriority(ObjectGuid targetGuid, uint32 auraId) const;

    // ========================================================================
    // EXTERNAL DEFENSIVE CD COORDINATION (S3.3 - GAP 3 Fix)
    // ========================================================================

    /**
     * Request external defensive CD via claim system
     * @param targetGuid Friendly needing protection
     * @param danger Current danger level
     * @return true if claim was submitted
     */
    bool RequestExternalDefensive(ObjectGuid targetGuid, DangerLevel danger);

    /**
     * Check if this bot should provide external CD
     * @param targetGuid Out: who to protect
     * @param spellId Out: which CD to use
     * @return true if this bot should cast external CD
     */
    bool ShouldProvideExternalCD(ObjectGuid& targetGuid, uint32& spellId) const;

    /**
     * Report external CD usage
     */
    void OnExternalCDUsed(ObjectGuid targetGuid, uint32 spellId);

    /**
     * Check if target is currently protected (within danger window)
     * @param targetGuid Target to check
     * @return true if target has active protection
     */
    bool IsTargetProtected(ObjectGuid targetGuid) const;

    /**
     * Get protection window remaining for target
     * @return Milliseconds remaining or 0 if not protected
     */
    uint32 GetProtectionRemaining(ObjectGuid targetGuid) const;

    /**
     * Assess danger level for a target
     * @param targetGuid Target to assess
     * @return Current danger level
     */
    DangerLevel AssessDanger(ObjectGuid targetGuid) const;

    /**
     * Get appropriate CD tier for danger level
     * Per GAP 3: Don't waste major CDs on moderate danger
     */
    ExternalCDTier GetAppropriateCDTier(DangerLevel danger) const;

    // ========================================================================
    // CC COORDINATION (S3.6)
    // ========================================================================

    /**
     * Request to CC a target via claim system
     * @param targetGuid Enemy to CC
     * @param spellId CC spell to use
     * @param priority CC priority
     * @return true if claim was submitted
     */
    bool RequestCC(ObjectGuid targetGuid, uint32 spellId, ClaimPriority priority);

    /**
     * Check if this bot should CC (has active claim)
     * @param targetGuid Out: target to CC
     * @param spellId Out: CC spell to use
     * @return true if this bot should CC now
     */
    bool ShouldCC(ObjectGuid& targetGuid, uint32& spellId) const;

    /**
     * Report CC result
     */
    void OnCCExecuted(ObjectGuid targetGuid, uint32 spellId, bool success);

    // ========================================================================
    // CLAIM CALLBACKS
    // ========================================================================

    /**
     * Called when a claim we submitted is resolved
     */
    void OnClaimResolved(BotMessage const& message, ClaimStatus status);

    // ========================================================================
    // MESSAGE HANDLING
    // ========================================================================

    /**
     * Handle incoming bot-to-bot message from BotMessageBus
     * Called by BotAI::HandleBotMessage() when message is delivered
     */
    void HandleIncomingMessage(BotMessage const& message) { OnBotMessage(message); }

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    struct Config
    {
        uint32 protectionWindowMs = 6000;       // GAP 3: 6 second danger window
        uint32 claimTimeoutMs = 200;            // First-claim-wins timeout
        float dangerHealthCritical = 0.30f;     // < 30% = CRITICAL
        float dangerHealthHigh = 0.50f;         // < 50% = HIGH
        float dangerHealthModerate = 0.80f;     // < 80% = MODERATE
        uint32 incomingDPSThreshold = 5;        // % max HP/sec for danger
        bool enableInterruptClaims = true;
        bool enableDispelClaims = true;
        bool enableDefensiveClaims = true;
        bool enableCCClaims = true;
    };

    void SetConfig(const Config& config) { _config = config; }
    const Config& GetConfig() const { return _config; }

    // ========================================================================
    // METRICS
    // ========================================================================

    struct Metrics
    {
        uint32 interruptClaimsSubmitted = 0;
        uint32 interruptClaimsWon = 0;
        uint32 interruptClaimsLost = 0;
        uint32 dispelClaimsSubmitted = 0;
        uint32 dispelClaimsWon = 0;
        uint32 dispelClaimsLost = 0;
        uint32 defensiveClaimsSubmitted = 0;
        uint32 defensiveClaimsWon = 0;
        uint32 defensiveClaimsLost = 0;
        uint32 ccClaimsSubmitted = 0;
        uint32 ccClaimsWon = 0;
        uint32 ccClaimsLost = 0;
        uint32 protectionWindowsCreated = 0;
    };

    const Metrics& GetMetrics() const { return _metrics; }
    void ResetMetrics() { _metrics = Metrics(); }

private:
    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /**
     * Subscribe to BotMessageBus for this bot's group
     */
    void SubscribeToMessageBus();

    /**
     * Unsubscribe from BotMessageBus
     */
    void UnsubscribeFromMessageBus();

    /**
     * Handle incoming bot messages
     */
    void OnBotMessage(BotMessage const& message);

    /**
     * Update protection windows (expire old ones)
     */
    void UpdateProtectionWindows();

    /**
     * Get available external CDs this bot can provide
     */
    std::vector<uint32> GetAvailableExternalCDs(ExternalCDTier maxTier) const;

    /**
     * Check if bot can dispel the given type
     */
    bool CanDispelType(uint32 dispelMask) const;

    /**
     * Get bot's interrupt spell
     */
    uint32 GetInterruptSpell() const;

    /**
     * Calculate distance to target
     */
    float GetDistanceToTarget(ObjectGuid targetGuid) const;

    // ========================================================================
    // MEMBER VARIABLES
    // ========================================================================

    BotAI* _ai;
    Player* _bot;
    ObjectGuid _groupGuid;

    // Manager references (owned by BotAI, not us)
    InterruptCoordinatorFixed* _interruptCoord = nullptr;
    DefensiveBehaviorManager* _defensiveMgr = nullptr;
    CrowdControlManager* _ccMgr = nullptr;
    DispelCoordinator* _dispelCoord = nullptr;

    // Active claims for this bot
    struct ActiveClaim
    {
        BotMessageType type;
        ObjectGuid targetGuid;
        uint32 spellOrAuraId = 0;
        ClaimStatus status = ClaimStatus::PENDING;
        std::chrono::steady_clock::time_point submitTime;
        std::chrono::steady_clock::time_point resolveTime;
    };
    std::unordered_map<uint64, ActiveClaim> _activeClaims; // key: hash of target+type

    // Protection windows (GAP 3 fix)
    std::vector<ProtectionWindow> _protectionWindows;

    // Configuration
    Config _config;

    // Metrics
    Metrics _metrics;

    // Subscription state
    bool _subscribed = false;
    uint32 _subscriptionId = 0;

    // Update timing
    uint32 _lastUpdate = 0;
    static constexpr uint32 UPDATE_INTERVAL_MS = 100;
};

} // namespace Playerbot

#endif // PLAYERBOT_COMBAT_COORDINATION_INTEGRATOR_H
