/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "Core/Events/ICombatEventSubscriber.h"
#include "Core/Events/CombatEventType.h"
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <atomic>

class Player;
class Unit;

namespace Playerbot {

// ============================================================================
// TANK SWAP DEBUFF CATEGORY
// ============================================================================

/**
 * @enum TankSwapDebuffCategory
 * @brief Categories of tank-swap debuffs encountered in WoW 12.0 boss fights
 *
 * Each category represents a common pattern of tank-swap mechanics:
 * - STACKING_DAMAGE_AMP: Debuff that increases damage taken per stack
 * - STACKING_DOT: Debuff that applies increasing DoT damage per stack
 * - STACKING_VULNERABILITY: Debuff that increases vulnerability to a specific school
 * - TIMED_DEBUFF: A debuff that requires a swap after a fixed duration (not stack-based)
 * - FRONTAL_CLEAVE: Boss frontal mechanic that applies a debuff requiring tank rotation
 * - CUSTOM: Encounter-specific mechanic not fitting standard categories
 */
enum class TankSwapDebuffCategory : uint8
{
    STACKING_DAMAGE_AMP     = 0,    // e.g., "Overwhelm" - more damage taken per stack
    STACKING_DOT            = 1,    // e.g., "Searing Blaze" - increasing DoT per stack
    STACKING_VULNERABILITY  = 2,    // e.g., "Shadow Vulnerability" - school-specific
    TIMED_DEBUFF            = 3,    // Duration-based swap (swap when debuff active, not stacks)
    FRONTAL_CLEAVE          = 4,    // Frontal cone applies swap debuff
    CUSTOM                  = 5     // Encounter-specific, user-configured
};

// ============================================================================
// TANK SWAP DEBUFF DEFINITION
// ============================================================================

/**
 * @struct TankSwapDebuffDef
 * @brief Defines a known tank-swap debuff and its threshold
 *
 * Used by TankSwapCoordinator to recognize debuffs that signal a tank swap.
 * Can be pre-loaded with known WoW 12.0 encounter data or configured at runtime
 * via the encounter manager.
 */
struct TankSwapDebuffDef
{
    uint32 spellId = 0;                                     // Debuff spell ID
    uint8  stackThreshold = 3;                              // Swap at this many stacks
    TankSwapDebuffCategory category = TankSwapDebuffCategory::STACKING_DAMAGE_AMP;
    uint32 estimatedDurationMs = 0;                         // Duration of the debuff (ms), 0 = permanent until removed
    bool   swapOnApplication = false;                       // If true, swap on first application (no stacks)
    std::string description;                                // Human-readable description for logging

    TankSwapDebuffDef() = default;
    TankSwapDebuffDef(uint32 spell, uint8 threshold, TankSwapDebuffCategory cat, std::string desc)
        : spellId(spell), stackThreshold(threshold), category(cat), description(std::move(desc)) {}
};

// ============================================================================
// TANK REGISTRATION INFO
// ============================================================================

/**
 * @struct TankSwapTankInfo
 * @brief Tracks a registered tank's state within the swap coordinator
 */
struct TankSwapTankInfo
{
    ObjectGuid guid;
    bool isActiveTank = false;                              // Currently holding boss aggro
    bool isAlive = true;
    uint32 tauntSpellId = 0;                                // The taunt spell this tank uses (class-specific)
    uint32 lastTauntTimeMs = 0;                             // GameTime when last taunt was used
    uint32 tauntCooldownMs = 8000;                          // Taunt CD (default 8s)

    // Per-debuff stack tracking: spellId -> current stacks
    std::unordered_map<uint32, uint8> debuffStacks;

    TankSwapTankInfo() = default;
    explicit TankSwapTankInfo(ObjectGuid g) : guid(g) {}
};

// ============================================================================
// PENDING TAUNT SIGNAL
// ============================================================================

/**
 * @struct PendingTauntSignal
 * @brief Signals an off-tank to taunt a specific target
 *
 * Created when debuff threshold is reached on the active tank.
 * The off-tank polls ShouldTaunt() during its combat update cycle
 * and receives this signal.
 */
struct PendingTauntSignal
{
    ObjectGuid offTankGuid;                                 // Who should taunt
    ObjectGuid tauntTargetGuid;                             // What to taunt (boss)
    uint32 triggerSpellId = 0;                              // The debuff that triggered this swap
    uint8  triggerStacks = 0;                               // Stack count that triggered it
    uint32 createdTimeMs = 0;                               // When this signal was created
    uint32 expirationTimeMs = 0;                            // Signal expires after this time (stale protection)
    bool consumed = false;                                  // Set to true once the off-tank acknowledges
};

// ============================================================================
// TANK SWAP COORDINATOR
// ============================================================================

/**
 * @class TankSwapCoordinator
 * @brief Singleton that monitors boss mechanics for tank swap signals
 *
 * Detects debuff stacks on the active tank via CombatEvent subscription
 * (AURA_APPLIED, AURA_STACK_CHANGED, AURA_REMOVED) and coordinates taunt
 * swaps between registered tank bots.
 *
 * Thread Safety:
 * - Uses std::shared_mutex for read-heavy access patterns
 * - OnCombatEvent() is called from main thread (world update)
 * - ShouldTaunt() is called from bot AI update (may be worker thread)
 * - Registration/Unregistration is infrequent and uses exclusive locks
 *
 * Usage Flow:
 * 1. Register tank bots via RegisterTank() on encounter start
 * 2. Register known swap debuffs via RegisterSwapDebuff()
 * 3. CombatEvent system delivers aura events automatically
 * 4. When threshold reached, a PendingTauntSignal is created
 * 5. Off-tank's combat AI calls ShouldTaunt() and receives the signal
 * 6. Off-tank executes taunt, calls OnTauntExecuted() to confirm
 * 7. On encounter end, call Reset() to clear all state
 *
 * Implements ICombatEventSubscriber for event-driven detection.
 * Priority 150 = higher than normal raid coordination (50) but below
 * emergency systems (200+).
 */
class TankSwapCoordinator : public ICombatEventSubscriber
{
public:
    static TankSwapCoordinator& Instance();

    // Prevent copying
    TankSwapCoordinator(const TankSwapCoordinator&) = delete;
    TankSwapCoordinator& operator=(const TankSwapCoordinator&) = delete;

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /**
     * @brief Initialize the coordinator, subscribe to CombatEventRouter
     * Call once during module initialization.
     */
    void Initialize();

    /**
     * @brief Shutdown the coordinator, unsubscribe from events
     * Call once during module shutdown.
     */
    void Shutdown();

    /**
     * @brief Periodic update for expiring stale signals
     * @param diff Milliseconds since last update
     * Called from the main update loop (e.g., RaidCoordinator::Update or similar).
     */
    void Update(uint32 diff);

    /**
     * @brief Reset all state (tanks, debuffs, signals)
     * Call on encounter end or wipe to clear everything.
     */
    void Reset();

    // ========================================================================
    // ICombatEventSubscriber INTERFACE
    // ========================================================================

    void OnCombatEvent(const CombatEvent& event) override;

    CombatEventType GetSubscribedEventTypes() const override
    {
        return CombatEventType::AURA_APPLIED
             | CombatEventType::AURA_STACK_CHANGED
             | CombatEventType::AURA_REMOVED
             | CombatEventType::UNIT_DIED;
    }

    int32 GetEventPriority() const override { return 150; }

    bool ShouldReceiveEvent(const CombatEvent& event) const override;

    const char* GetSubscriberName() const override { return "TankSwapCoordinator"; }

    // ========================================================================
    // TANK REGISTRATION
    // ========================================================================

    /**
     * @brief Register a tank bot for swap coordination
     * @param tankGuid   The tank player's GUID
     * @param tauntSpell The tank's taunt spell ID (e.g., Taunt for Warriors, Hand of Reckoning for Paladins)
     * @param isActive   true if this tank is currently the active (main) tank
     */
    void RegisterTank(ObjectGuid tankGuid, uint32 tauntSpell, bool isActive);

    /**
     * @brief Unregister a tank (e.g., on death that cannot be recovered, or disconnect)
     */
    void UnregisterTank(ObjectGuid tankGuid);

    /**
     * @brief Check if a player is registered as a tank
     */
    bool IsTankRegistered(ObjectGuid guid) const;

    /**
     * @brief Get the GUID of the currently active tank
     */
    ObjectGuid GetActiveTank() const;

    /**
     * @brief Manually set which tank is active (e.g., after a manual taunt)
     */
    void SetActiveTank(ObjectGuid tankGuid);

    /**
     * @brief Get number of registered tanks
     */
    uint32 GetRegisteredTankCount() const;

    // ========================================================================
    // SWAP DEBUFF REGISTRATION
    // ========================================================================

    /**
     * @brief Register a known tank-swap debuff
     * @param def The debuff definition including spell ID, threshold, category
     *
     * Multiple debuffs can be registered (some encounters have more than one swap trigger).
     */
    void RegisterSwapDebuff(const TankSwapDebuffDef& def);

    /**
     * @brief Register a swap debuff with minimal parameters (convenience overload)
     * @param spellId        Debuff spell ID
     * @param stackThreshold Number of stacks that triggers a swap (default 3)
     * @param description    Human-readable description
     */
    void RegisterSwapDebuff(uint32 spellId, uint8 stackThreshold = 3,
                            const std::string& description = "");

    /**
     * @brief Remove a registered swap debuff
     */
    void UnregisterSwapDebuff(uint32 spellId);

    /**
     * @brief Clear all registered swap debuffs
     */
    void ClearSwapDebuffs();

    /**
     * @brief Check if a spell ID is a registered swap debuff
     */
    bool IsSwapDebuff(uint32 spellId) const;

    /**
     * @brief Get the number of registered swap debuffs
     */
    uint32 GetSwapDebuffCount() const;

    // ========================================================================
    // SWAP THRESHOLD CONFIGURATION
    // ========================================================================

    /**
     * @brief Set the default stack threshold for debuffs registered without one
     * @param threshold Default number of stacks to trigger swap (default 3)
     */
    void SetDefaultStackThreshold(uint8 threshold);

    /**
     * @brief Get the default stack threshold
     */
    uint8 GetDefaultStackThreshold() const;

    /**
     * @brief Set the minimum time between swaps to prevent rapid ping-pong
     * @param cooldownMs Cooldown in milliseconds (default 5000ms)
     */
    void SetSwapCooldown(uint32 cooldownMs);

    /**
     * @brief Set the expiration time for pending taunt signals
     * @param timeoutMs Time in milliseconds before a signal expires (default 10000ms)
     */
    void SetSignalTimeout(uint32 timeoutMs);

    // ========================================================================
    // SWAP QUERY INTERFACE (Called by bot AI during combat update)
    // ========================================================================

    /**
     * @brief Check if a specific tank should taunt now
     * @param tankGuid The querying tank's GUID
     * @return true if there is a pending taunt signal for this tank
     *
     * Called by the off-tank's combat AI during its update cycle.
     * Thread-safe (uses shared_lock for read access).
     */
    bool ShouldTaunt(ObjectGuid tankGuid) const;

    /**
     * @brief Get the target that the tank should taunt
     * @param tankGuid The querying tank's GUID
     * @return The GUID of the unit to taunt (boss), or empty GUID if no signal
     *
     * Called after ShouldTaunt() returns true to get the actual taunt target.
     * Thread-safe.
     */
    ObjectGuid GetTauntTarget(ObjectGuid tankGuid) const;

    /**
     * @brief Consume the pending taunt signal after taunt execution
     * @param tankGuid The tank that executed the taunt
     *
     * Called by the off-tank after it successfully casts its taunt.
     * This completes the swap: the off-tank becomes the new active tank.
     */
    void OnTauntExecuted(ObjectGuid tankGuid);

    /**
     * @brief Report that a taunt failed (resisted, out of range, etc.)
     * @param tankGuid The tank whose taunt failed
     *
     * The coordinator may attempt to assign a different tank or retry.
     */
    void OnTauntFailed(ObjectGuid tankGuid);

    // ========================================================================
    // DEBUFF STACK QUERIES
    // ========================================================================

    /**
     * @brief Get current debuff stacks on a tank for a specific spell
     * @param tankGuid Tank to query
     * @param spellId  Debuff spell ID
     * @return Current stack count, 0 if not present
     */
    uint8 GetDebuffStacks(ObjectGuid tankGuid, uint32 spellId) const;

    /**
     * @brief Get the highest swap-debuff stack count on a tank across all registered debuffs
     * @param tankGuid Tank to query
     * @return Highest stack count across all registered swap debuffs
     */
    uint8 GetHighestSwapDebuffStacks(ObjectGuid tankGuid) const;

    /**
     * @brief Check if a tank swap is imminent (stacks at threshold - 1)
     * @param tankGuid Tank to check
     * @return true if swap will be needed within 1 more stack application
     */
    bool IsSwapImminent(ObjectGuid tankGuid) const;

    // ========================================================================
    // KNOWN WOW 12.0 DEBUFF PRESETS
    // ========================================================================

    /**
     * @brief Load common tank-swap debuff patterns from WoW 12.0 encounters
     *
     * Registers a curated set of known encounter debuffs. Can be called
     * at initialization or when entering specific raid instances.
     * These serve as fallback recognition -- encounter-specific scripts
     * should register exact debuffs via RegisterSwapDebuff().
     */
    void LoadKnownDebuffPresets();

    // ========================================================================
    // STATISTICS / DEBUG
    // ========================================================================

    /**
     * @brief Get total number of tank swaps coordinated since last Reset()
     */
    uint32 GetTotalSwapsCoordinated() const;

    /**
     * @brief Get total number of taunt failures since last Reset()
     */
    uint32 GetTotalTauntFailures() const;

    /**
     * @brief Check if the coordinator is active and subscribed to events
     */
    bool IsActive() const;

private:
    TankSwapCoordinator();
    ~TankSwapCoordinator();

    // ========================================================================
    // EVENT HANDLERS
    // ========================================================================

    void HandleAuraApplied(const CombatEvent& event);
    void HandleAuraStackChanged(const CombatEvent& event);
    void HandleAuraRemoved(const CombatEvent& event);
    void HandleUnitDied(const CombatEvent& event);

    // ========================================================================
    // INTERNAL SWAP LOGIC
    // ========================================================================

    /**
     * @brief Check if a swap should be triggered for the given tank and debuff
     * @param tankGuid Tank whose debuff stacks to evaluate
     * @param spellId  The debuff spell that was just applied/stacked
     * @param stacks   Current stack count
     */
    void EvaluateSwapCondition(ObjectGuid tankGuid, uint32 spellId, uint8 stacks);

    /**
     * @brief Create a pending taunt signal for the best available off-tank
     * @param currentActiveTank The tank that needs to be relieved
     * @param bossGuid          The boss/target that needs to be taunted
     * @param triggerSpellId    The debuff that triggered the swap
     * @param triggerStacks     The stack count at trigger time
     * @return true if a signal was successfully created
     */
    bool CreateTauntSignal(ObjectGuid currentActiveTank, ObjectGuid bossGuid,
                           uint32 triggerSpellId, uint8 triggerStacks);

    /**
     * @brief Find the best off-tank to receive a taunt signal
     * Prefers: alive, not on cooldown, lowest debuff stacks, not currently active
     * @param excludeTank Tank to exclude (the one being relieved)
     * @return GUID of the best off-tank, or empty GUID if none available
     */
    ObjectGuid FindBestOffTank(ObjectGuid excludeTank) const;

    /**
     * @brief Clean up expired signals and stale tank data
     */
    void CleanupExpiredSignals(uint32 nowMs);

    /**
     * @brief Attempt to find the boss target from the active tank's current target
     * @param activeTankGuid The active tank
     * @return GUID of the boss the tank is targeting, or empty
     */
    ObjectGuid ResolveBossTarget(ObjectGuid activeTankGuid) const;

    // ========================================================================
    // STATE (Protected by _mutex)
    // ========================================================================

    mutable std::shared_mutex _mutex;

    // Registered tanks: guid -> info
    std::unordered_map<ObjectGuid, TankSwapTankInfo> _tanks;

    // Active tank GUID (the one currently holding boss aggro)
    ObjectGuid _activeTank;

    // Registered swap debuff definitions: spellId -> definition
    std::unordered_map<uint32, TankSwapDebuffDef> _swapDebuffs;

    // Set of registered swap debuff spell IDs for O(1) lookup in ShouldReceiveEvent
    std::unordered_set<uint32> _swapDebuffSpellIds;

    // Pending taunt signals: offTankGuid -> signal
    std::unordered_map<ObjectGuid, PendingTauntSignal> _pendingTaunts;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    uint8 _defaultStackThreshold = 3;
    uint32 _swapCooldownMs = 5000;                          // Min time between swaps
    uint32 _signalTimeoutMs = 10000;                        // Pending signal expiration
    uint32 _lastSwapTimeMs = 0;                             // Timestamp of last completed swap
    uint32 _cleanupTimerMs = 0;                             // Accumulator for periodic cleanup
    static constexpr uint32 CLEANUP_INTERVAL_MS = 2000;     // Run cleanup every 2 seconds

    // ========================================================================
    // STATISTICS
    // ========================================================================

    std::atomic<uint32> _totalSwapsCoordinated{0};
    std::atomic<uint32> _totalTauntFailures{0};

    // ========================================================================
    // SUBSCRIPTION STATE
    // ========================================================================

    std::atomic<bool> _initialized{false};
    std::atomic<bool> _subscribed{false};
};

// Singleton accessor macro
#define sTankSwapCoordinator Playerbot::TankSwapCoordinator::Instance()

} // namespace Playerbot
