/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * CooldownSyncCoordinator - Raid-wide cooldown synchronization
 *
 * Provides a global coordination layer for raid cooldown management across
 * all active encounters. While RaidCooldownRotation (a per-instance sub-manager
 * of RaidCoordinator) handles the mechanical tracking of individual cooldown
 * availability and rotation order, this coordinator addresses the higher-level
 * synchronization problem: deciding WHEN cooldowns should be used based on
 * encounter state, raid health thresholds, and coordinated burst windows.
 *
 * Key responsibilities:
 * - Coordinated burst windows (pull, phase transition, execute phase)
 * - Bloodlust/Heroism/Time Warp timing decisions based on encounter strategy
 * - Healer defensive CD rotation during predictable damage phases
 * - DPS cooldown stacking for maximum burst during optimal windows
 * - Cross-bot cooldown usage tracking to prevent wasteful overlap
 *
 * Thread safety: All public methods are safe to call from any thread.
 * Internal state is protected by a shared_mutex (read-heavy workload).
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <shared_mutex>
#include <atomic>

class Player;

namespace Playerbot
{

// ============================================================================
// ENUMS
// ============================================================================

/**
 * @enum CooldownWindow
 * @brief Defines the current raid-wide cooldown usage window.
 *
 * Each window type drives different cooldown usage policies:
 * - NONE: Normal gameplay, no coordinated CD usage
 * - PULL_BURST: Opening burst, all DPS CDs + possible Bloodlust
 * - PHASE_TRANSITION: Post-phase-change burst, selective CD usage
 * - EXECUTE_PHASE: Boss below execute threshold, burn phase
 * - EMERGENCY: Raid health critical, healer defensives needed
 * - INTERMISSION_END: Coming out of intermission, stacked CDs
 */
enum class CooldownWindow : uint8
{
    NONE              = 0,
    PULL_BURST        = 1,
    PHASE_TRANSITION  = 2,
    EXECUTE_PHASE     = 3,
    EMERGENCY         = 4,
    INTERMISSION_END  = 5
};

/**
 * @enum CooldownCategory
 * @brief Categorizes cooldowns by their strategic role.
 */
enum class CooldownCategory : uint8
{
    BLOODLUST       = 0,    // Bloodlust/Heroism/Time Warp/Primal Rage (once per encounter)
    DPS_MAJOR       = 1,    // Major DPS CDs (2-3 min cooldowns)
    DPS_MINOR       = 2,    // Minor DPS CDs (1 min or shorter)
    HEALER_RAID     = 3,    // Raid-wide healing CDs (Tranquility, Divine Hymn, etc.)
    HEALER_EXTERNAL = 4,    // External defensives (Pain Suppression, Ironbark, etc.)
    RAID_DEFENSIVE  = 5,    // Raid-wide defensives (Spirit Link, Rallying Cry, Aura Mastery)
    PERSONAL        = 6     // Personal defensives (not coordinated, tracked only)
};

// ============================================================================
// STRUCTS
// ============================================================================

/**
 * @struct TrackedCooldown
 * @brief Tracks a single cooldown registered by a bot.
 */
struct TrackedCooldown
{
    ObjectGuid botGuid;
    uint32 spellId           = 0;
    CooldownCategory category = CooldownCategory::PERSONAL;
    uint32 cooldownDurationMs = 0;      // Full cooldown duration
    uint32 remainingMs       = 0;       // Time until ready
    uint32 registeredAt      = 0;       // Server time when registered
    bool isReady             = true;    // Whether the CD is available

    bool IsAvailable() const { return isReady && remainingMs == 0; }
};

/**
 * @struct BurstWindowState
 * @brief Describes the current burst window context.
 */
struct BurstWindowState
{
    CooldownWindow window       = CooldownWindow::NONE;
    uint32 windowStartTime      = 0;    // Server time when window opened
    uint32 windowDurationMs     = 0;    // How long the window lasts (0 = indefinite)
    uint32 instanceId           = 0;    // Raid instance this window belongs to
    uint8 phaseNumber           = 0;    // Current encounter phase
    float bossHealthPct         = 100.0f;
    float raidHealthPct         = 100.0f;
    bool bloodlustUsedThisEncounter = false;
};

/**
 * @struct EncounterCooldownState
 * @brief Per-encounter cooldown coordination state.
 */
struct EncounterCooldownState
{
    uint32 instanceId           = 0;
    uint32 encounterId          = 0;
    bool isActive               = false;

    // Encounter timing
    uint32 encounterStartTime   = 0;
    uint32 lastPhaseChangeTime  = 0;
    uint8 currentPhase          = 1;

    // Bloodlust tracking
    bool bloodlustUsed          = false;
    ObjectGuid bloodlustCaster;
    uint32 bloodlustTime        = 0;

    // Burst window
    BurstWindowState currentWindow;

    // Healer CD rotation index - tracks which healer CD to use next
    uint32 healerCDRotationIndex = 0;

    // Emergency state
    uint32 lastEmergencyTime    = 0;
    uint32 emergencyCooldownMs  = 10000; // Minimum 10s between emergency triggers

    // Statistics
    uint32 totalBurstWindows    = 0;
    uint32 totalEmergencies     = 0;
};

/**
 * @struct CooldownSyncConfig
 * @brief Configuration parameters for cooldown synchronization.
 */
struct CooldownSyncConfig
{
    // Pull burst window
    uint32 pullBurstDurationMs      = 15000;    // 15 second pull burst window

    // Phase transition burst
    uint32 phaseTransitionBurstMs   = 10000;    // 10 second post-phase burst

    // Execute phase threshold
    float executePhaseThreshold     = 30.0f;    // Boss health % to enter execute

    // Emergency thresholds
    float emergencyRaidHealthPct    = 40.0f;    // Raid health to trigger emergency
    float criticalRaidHealthPct     = 25.0f;    // Critical threshold for stacking CDs

    // Bloodlust timing
    bool bloodlustOnPull            = false;    // Use BL on pull (default: save for execute)
    float bloodlustHealthThreshold  = 30.0f;    // Boss health % to use BL

    // Healer CD rotation
    uint32 healerCDMinIntervalMs    = 15000;    // Minimum 15s between healer CDs
    uint32 healerCDOverlapWindowMs  = 3000;     // Allow 3s overlap between rotating CDs

    // DPS CD coordination
    bool stackDPSCooldowns          = true;     // Stack DPS CDs together for max burst
    uint32 dpsCDStaggerMs           = 500;      // Stagger DPS CD usage by this much

    // Update interval
    uint32 updateIntervalMs         = 250;      // Check every 250ms
};

// ============================================================================
// COOLDOWN SPELL ID CONSTANTS
// ============================================================================

namespace CooldownSpells
{
    // Bloodlust-type effects (all share the Exhaustion/Sated debuff)
    static constexpr uint32 BLOODLUST           = 2825;     // Shaman (Horde)
    static constexpr uint32 HEROISM             = 32182;    // Shaman (Alliance)
    static constexpr uint32 TIME_WARP           = 80353;    // Mage
    static constexpr uint32 PRIMAL_RAGE         = 264667;   // Hunter pet

    // Raid-wide healing CDs
    static constexpr uint32 TRANQUILITY         = 740;      // Druid (Restoration)
    static constexpr uint32 DIVINE_HYMN         = 64843;    // Priest (Holy)
    static constexpr uint32 HEALING_TIDE_TOTEM  = 108280;   // Shaman (Restoration)
    static constexpr uint32 REVIVAL             = 115310;   // Monk (Mistweaver)
    static constexpr uint32 REWIND              = 363534;   // Evoker (Preservation)

    // Raid-wide defensives
    static constexpr uint32 SPIRIT_LINK_TOTEM   = 98008;    // Shaman (Restoration)
    static constexpr uint32 RALLYING_CRY        = 97462;    // Warrior
    static constexpr uint32 AURA_MASTERY        = 31821;    // Paladin (Holy)
    static constexpr uint32 POWER_WORD_BARRIER  = 62618;    // Priest (Discipline)

    // Returns true if the spell ID is a Bloodlust-type effect
    inline bool IsBloodlustSpell(uint32 spellId)
    {
        return spellId == BLOODLUST || spellId == HEROISM ||
               spellId == TIME_WARP || spellId == PRIMAL_RAGE;
    }

    // Returns true if the spell ID is a raid-wide healing CD
    inline bool IsRaidHealingCD(uint32 spellId)
    {
        return spellId == TRANQUILITY || spellId == DIVINE_HYMN ||
               spellId == HEALING_TIDE_TOTEM || spellId == REVIVAL ||
               spellId == REWIND;
    }

    // Returns true if the spell ID is a raid-wide defensive CD
    inline bool IsRaidDefensiveCD(uint32 spellId)
    {
        return spellId == SPIRIT_LINK_TOTEM || spellId == RALLYING_CRY ||
               spellId == AURA_MASTERY || spellId == POWER_WORD_BARRIER;
    }
}

// ============================================================================
// MAIN CLASS
// ============================================================================

/**
 * @class CooldownSyncCoordinator
 * @brief Global singleton for raid-wide cooldown synchronization.
 *
 * This class provides the decision-making layer for when cooldowns should be
 * used. Individual bots query this coordinator before using major cooldowns.
 *
 * Design:
 * - Thread-safe via std::shared_mutex (read-heavy, write-rare)
 * - Per-encounter state tracking (supports multiple concurrent raids)
 * - Configurable thresholds and timing windows
 * - Integrates with but does not replace RaidCooldownRotation
 *
 * Usage from bot AI:
 * @code
 * if (sCooldownSyncCoordinator->ShouldUseBurstCD(bot))
 *     CastBurstCooldown(bot);
 * if (sCooldownSyncCoordinator->ShouldUseBloodlust(bot))
 *     CastBloodlust(bot);
 * if (sCooldownSyncCoordinator->ShouldUseHealerCD(bot))
 *     CastHealerDefensiveCD(bot);
 * @endcode
 */
class TC_GAME_API CooldownSyncCoordinator
{
public:
    static CooldownSyncCoordinator* instance()
    {
        static CooldownSyncCoordinator inst;
        return &inst;
    }

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /**
     * @brief Initialize the coordinator with default configuration.
     * Called once during module startup.
     */
    void Initialize();

    /**
     * @brief Shut down the coordinator and clear all state.
     * Called during module shutdown.
     */
    void Shutdown();

    /**
     * @brief Periodic update. Ticks cooldown timers and evaluates window transitions.
     * @param diff Time elapsed since last update in milliseconds.
     */
    void Update(uint32 diff);

    // ========================================================================
    // ENCOUNTER LIFECYCLE
    // ========================================================================

    /**
     * @brief Called when a boss encounter begins.
     * Initializes per-encounter tracking state and opens the pull burst window.
     * @param instanceId The raid instance ID.
     * @param encounterId The encounter/boss ID.
     */
    void OnEncounterStart(uint32 instanceId, uint32 encounterId);

    /**
     * @brief Called when a boss encounter ends (kill or wipe).
     * Cleans up per-encounter state.
     * @param instanceId The raid instance ID.
     * @param encounterId The encounter/boss ID.
     * @param success True if the boss was killed, false if wipe.
     */
    void OnEncounterEnd(uint32 instanceId, uint32 encounterId, bool success);

    /**
     * @brief Called when the encounter transitions to a new phase.
     * May open a phase transition burst window.
     * @param instanceId The raid instance ID.
     * @param newPhase The new phase number.
     */
    void OnPhaseChange(uint32 instanceId, uint8 newPhase);

    // ========================================================================
    // COOLDOWN REGISTRATION & TRACKING
    // ========================================================================

    /**
     * @brief Register a cooldown usage by a bot.
     * Called when a bot uses a tracked cooldown spell so the coordinator
     * knows the spell is on cooldown and can factor it into rotation decisions.
     * @param botGuid The bot that used the cooldown.
     * @param spellId The spell that was cast.
     * @param durationMs The cooldown duration in milliseconds.
     */
    void RegisterCooldown(ObjectGuid botGuid, uint32 spellId, uint32 durationMs);

    /**
     * @brief Unregister all cooldowns for a bot (e.g., on death or removal from raid).
     * @param botGuid The bot to unregister.
     */
    void UnregisterBot(ObjectGuid botGuid);

    /**
     * @brief Report the current raid health percentage for an instance.
     * Used by the coordinator to evaluate emergency thresholds.
     * @param instanceId The raid instance ID.
     * @param raidHealthPct The current average raid health percentage (0-100).
     */
    void ReportRaidHealth(uint32 instanceId, float raidHealthPct);

    /**
     * @brief Report the current boss health percentage for an instance.
     * Used by the coordinator to evaluate execute phase and Bloodlust timing.
     * @param instanceId The raid instance ID.
     * @param bossHealthPct The current boss health percentage (0-100).
     */
    void ReportBossHealth(uint32 instanceId, float bossHealthPct);

    // ========================================================================
    // DECISION QUERIES - Called by individual bots
    // ========================================================================

    /**
     * @brief Should this bot use its DPS burst cooldowns now?
     *
     * Returns true when a burst window is active (pull, phase transition,
     * execute phase) and the bot has not already been told to burst recently.
     *
     * @param bot The bot player asking.
     * @return True if the bot should activate DPS cooldowns.
     */
    bool ShouldUseBurstCD(Player* bot) const;

    /**
     * @brief Should this bot cast Bloodlust/Heroism/Time Warp?
     *
     * Only returns true for exactly ONE bot per encounter (the designated
     * Bloodlust provider). Considers encounter state, boss health, and
     * whether Bloodlust has already been used this encounter.
     *
     * @param bot The bot player asking.
     * @return True if this specific bot should cast its Bloodlust-type spell.
     */
    bool ShouldUseBloodlust(Player* bot) const;

    /**
     * @brief Should this bot use its healer defensive/healing CD?
     *
     * Implements a rotation: healer CDs are used one at a time during
     * emergency windows or predictable damage phases, not all at once.
     * Returns true for exactly one healer at a time.
     *
     * @param bot The bot player asking.
     * @return True if this specific bot should use its healer CD now.
     */
    bool ShouldUseHealerCD(Player* bot) const;

    /**
     * @brief Should this bot use a raid-wide defensive CD?
     *
     * Similar to healer CD rotation but for raid-wide defensives like
     * Spirit Link Totem, Rallying Cry, Aura Mastery.
     *
     * @param bot The bot player asking.
     * @return True if this specific bot should use its raid defensive now.
     */
    bool ShouldUseRaidDefensive(Player* bot) const;

    // ========================================================================
    // WINDOW MANAGEMENT
    // ========================================================================

    /**
     * @brief Manually open a burst window for an encounter.
     * @param instanceId The raid instance ID.
     * @param window The type of burst window to open.
     * @param durationMs How long the window lasts (0 = use default).
     */
    void OpenBurstWindow(uint32 instanceId, CooldownWindow window, uint32 durationMs = 0);

    /**
     * @brief Close any active burst window for an encounter.
     * @param instanceId The raid instance ID.
     */
    void CloseBurstWindow(uint32 instanceId);

    /**
     * @brief Get the current burst window for an encounter.
     * @param instanceId The raid instance ID.
     * @return The current window state.
     */
    BurstWindowState GetCurrentWindow(uint32 instanceId) const;

    /**
     * @brief Check if any burst window is active for an encounter.
     * @param instanceId The raid instance ID.
     * @return True if a burst window is open.
     */
    bool IsBurstWindowActive(uint32 instanceId) const;

    // ========================================================================
    // QUERY METHODS
    // ========================================================================

    /**
     * @brief Get all tracked cooldowns for a specific bot.
     * @param botGuid The bot to query.
     * @return Vector of tracked cooldowns (may be empty).
     */
    ::std::vector<TrackedCooldown> GetBotCooldowns(ObjectGuid botGuid) const;

    /**
     * @brief Get all available (ready) cooldowns of a given category across the raid.
     * @param instanceId The raid instance ID.
     * @param category The category to filter by.
     * @return Vector of available cooldowns matching the category.
     */
    ::std::vector<TrackedCooldown> GetAvailableCooldowns(uint32 instanceId, CooldownCategory category) const;

    /**
     * @brief Check if Bloodlust has been used in the current encounter.
     * @param instanceId The raid instance ID.
     * @return True if Bloodlust/Heroism/Time Warp has been used.
     */
    bool IsBloodlustUsed(uint32 instanceId) const;

    /**
     * @brief Get the number of active encounters being tracked.
     * @return Count of active encounters.
     */
    uint32 GetActiveEncounterCount() const;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Get the current configuration (read-only).
     * @return Reference to the current configuration.
     */
    const CooldownSyncConfig& GetConfig() const { return _config; }

    /**
     * @brief Update the configuration. Thread-safe.
     * @param config New configuration to apply.
     */
    void SetConfig(const CooldownSyncConfig& config);

    /**
     * @brief Set whether Bloodlust should be used on pull for a specific encounter.
     * @param instanceId The raid instance ID.
     * @param onPull True to use Bloodlust on pull, false to save for execute.
     */
    void SetBloodlustOnPull(uint32 instanceId, bool onPull);

private:
    CooldownSyncCoordinator();
    ~CooldownSyncCoordinator();

    // Non-copyable, non-movable
    CooldownSyncCoordinator(const CooldownSyncCoordinator&) = delete;
    CooldownSyncCoordinator& operator=(const CooldownSyncCoordinator&) = delete;
    CooldownSyncCoordinator(CooldownSyncCoordinator&&) = delete;
    CooldownSyncCoordinator& operator=(CooldownSyncCoordinator&&) = delete;

    // ========================================================================
    // INTERNAL STATE
    // ========================================================================

    mutable std::shared_mutex _mutex;

    // Per-encounter state, keyed by instanceId
    ::std::unordered_map<uint32, EncounterCooldownState> _encounterStates;

    // All registered cooldowns, keyed by botGuid then spellId
    ::std::unordered_map<ObjectGuid, ::std::unordered_map<uint32, TrackedCooldown>> _cooldowns;

    // Maps botGuid -> instanceId for quick lookup
    ::std::unordered_map<ObjectGuid, uint32> _botToInstance;

    // Bloodlust override flags per instance
    ::std::unordered_map<uint32, bool> _bloodlustOnPullOverride;

    // Configuration
    CooldownSyncConfig _config;

    // Update timer accumulator
    uint32 _updateTimer = 0;

    // Initialization flag
    std::atomic<bool> _initialized{false};

    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /**
     * @brief Update all cooldown timers, decrementing remaining time.
     * @param diff Time elapsed in milliseconds.
     */
    void UpdateCooldownTimers(uint32 diff);

    /**
     * @brief Evaluate and potentially transition burst windows for all encounters.
     * @param diff Time elapsed in milliseconds.
     */
    void UpdateBurstWindows(uint32 diff);

    /**
     * @brief Check if emergency conditions are met for an encounter and open
     *        an emergency window if so.
     * @param state The encounter state to evaluate.
     * @param serverTimeMs Current server time in milliseconds.
     */
    void EvaluateEmergencyConditions(EncounterCooldownState& state, uint32 serverTimeMs);

    /**
     * @brief Check if execute phase should be entered based on boss health.
     * @param state The encounter state to evaluate.
     */
    void EvaluateExecutePhase(EncounterCooldownState& state);

    /**
     * @brief Determine the category for a given spell ID.
     * @param spellId The spell to categorize.
     * @param bot The bot that owns this cooldown (used for class/spec context).
     * @return The cooldown category.
     */
    CooldownCategory CategorizeSpell(uint32 spellId, Player* bot) const;

    /**
     * @brief Find the instance ID for a given bot.
     * @param botGuid The bot's GUID.
     * @return The instance ID, or 0 if not found.
     */
    uint32 GetInstanceForBot(ObjectGuid botGuid) const;

    /**
     * @brief Find the encounter state for a given instance.
     * Caller must hold at least a shared lock on _mutex.
     * @param instanceId The instance ID.
     * @return Pointer to the encounter state, or nullptr if not found.
     */
    const EncounterCooldownState* GetEncounterState(uint32 instanceId) const;

    /**
     * @brief Mutable version. Caller must hold a unique lock on _mutex.
     */
    EncounterCooldownState* GetEncounterStateMut(uint32 instanceId);

    /**
     * @brief Check if a bot has a Bloodlust-type spell available.
     * @param botGuid The bot to check.
     * @return True if the bot has an available BL spell.
     */
    bool HasBloodlustSpellAvailable(ObjectGuid botGuid) const;

    /**
     * @brief Check if a bot has a raid healing CD available.
     * @param botGuid The bot to check.
     * @return True if the bot has an available healer CD.
     */
    bool HasHealerCDAvailable(ObjectGuid botGuid) const;

    /**
     * @brief Check if a bot has a raid defensive CD available.
     * @param botGuid The bot to check.
     * @return True if the bot has an available raid defensive.
     */
    bool HasRaidDefensiveAvailable(ObjectGuid botGuid) const;

    /**
     * @brief Get the list of bots with available healer CDs, sorted by rotation order.
     * @param instanceId The instance to search.
     * @return Sorted vector of bot GUIDs with available healer CDs.
     */
    ::std::vector<ObjectGuid> GetHealerCDProviders(uint32 instanceId) const;

    /**
     * @brief Get the list of bots with available raid defensives, sorted by rotation order.
     * @param instanceId The instance to search.
     * @return Sorted vector of bot GUIDs with available raid defensives.
     */
    ::std::vector<ObjectGuid> GetRaidDefensiveProviders(uint32 instanceId) const;

    /**
     * @brief Get the list of bots with available Bloodlust-type spells.
     * @param instanceId The instance to search.
     * @return Vector of bot GUIDs that can cast Bloodlust.
     */
    ::std::vector<ObjectGuid> GetBloodlustProviders(uint32 instanceId) const;

    /**
     * @brief Get the current server time in milliseconds.
     * @return Current server time.
     */
    uint32 GetServerTimeMs() const;

    /**
     * @brief Determine default burst window duration for a window type.
     * @param window The window type.
     * @return Duration in milliseconds.
     */
    uint32 GetDefaultWindowDuration(CooldownWindow window) const;

    /**
     * @brief Check if enough time has passed since the last healer CD in this encounter.
     * @param state The encounter state.
     * @return True if the minimum interval has elapsed.
     */
    bool IsHealerCDIntervalElapsed(const EncounterCooldownState& state) const;
};

#define sCooldownSyncCoordinator CooldownSyncCoordinator::instance()

// ============================================================================
// UTILITY
// ============================================================================

inline const char* CooldownWindowToString(CooldownWindow window)
{
    switch (window)
    {
        case CooldownWindow::NONE:              return "NONE";
        case CooldownWindow::PULL_BURST:        return "PULL_BURST";
        case CooldownWindow::PHASE_TRANSITION:  return "PHASE_TRANSITION";
        case CooldownWindow::EXECUTE_PHASE:     return "EXECUTE_PHASE";
        case CooldownWindow::EMERGENCY:         return "EMERGENCY";
        case CooldownWindow::INTERMISSION_END:  return "INTERMISSION_END";
        default:                                return "UNKNOWN";
    }
}

inline const char* CooldownCategoryToString(CooldownCategory category)
{
    switch (category)
    {
        case CooldownCategory::BLOODLUST:       return "BLOODLUST";
        case CooldownCategory::DPS_MAJOR:       return "DPS_MAJOR";
        case CooldownCategory::DPS_MINOR:       return "DPS_MINOR";
        case CooldownCategory::HEALER_RAID:     return "HEALER_RAID";
        case CooldownCategory::HEALER_EXTERNAL: return "HEALER_EXTERNAL";
        case CooldownCategory::RAID_DEFENSIVE:  return "RAID_DEFENSIVE";
        case CooldownCategory::PERSONAL:        return "PERSONAL";
        default:                                return "UNKNOWN";
    }
}

} // namespace Playerbot
