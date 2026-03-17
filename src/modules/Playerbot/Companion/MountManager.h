/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "Player.h"
#include "ObjectGuid.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>

namespace Playerbot
{

/**
 * @brief Mount types based on WoW 12.0 mount system
 */
enum class MountType : uint8
{
    GROUND = 0,
    FLYING = 1,
    AQUATIC = 2,
    DRAGONRIDING = 3,    // WoW 10.0+ Dragonriding mounts
    SPECIAL = 4           // Special mounts (passenger, multi-seat)
};

/**
 * @brief Mount speed categories
 */
enum class MountSpeed : uint16
{
    SLOW = 60,           // 60% ground speed
    NORMAL = 100,        // 100% ground speed
    FAST = 150,          // 150% flying speed (60% in no-fly zones)
    EPIC = 280,          // 280% flying speed (100% in no-fly zones)
    EPIC_PLUS = 310      // 310% flying speed (100% in no-fly zones)
};

/**
 * @brief Mount information
 */
struct MountInfo
{
    uint32 spellId;          // Spell ID to cast for mount
    uint32 displayId;        // Creature display ID
    std::string name;
    MountType type;
    MountSpeed speed;
    uint32 requiredLevel;
    uint32 requiredSkill;    // Riding skill required (75, 150, 225, 300)
    bool isFlyingMount;
    bool isAquaticMount;
    bool isDragonridingMount;
    bool isMultiPassenger;
    uint32 passengerCount;
    std::vector<uint32> zoneRestrictions; // Zones where mount is restricted

    MountInfo() : spellId(0), displayId(0), type(MountType::GROUND),
        speed(MountSpeed::NORMAL), requiredLevel(20), requiredSkill(75),
        isFlyingMount(false), isAquaticMount(false), isDragonridingMount(false),
        isMultiPassenger(false), passengerCount(1) {}
};

/**
 * @brief Mount automation configuration per bot
 */
struct MountAutomationProfile
{
    bool autoMount = true;                   // Auto-mount when traveling
    bool preferFlyingMount = true;           // Prefer flying mount if available
    bool useDragonriding = true;             // Use dragonriding mounts if available
    float minDistanceForMount = 50.0f;       // Min distance to mount (yards)
    bool dismountInCombat = true;            // Auto-dismount when entering combat
    bool remountAfterCombat = true;          // Remount after combat ends
    uint32 mountCastDelay = 1500;            // Delay before mounting (ms)
    bool useGroundMountIndoors = true;       // Use ground mount in no-fly zones
    bool sharePassengerMounts = false;       // Allow group members to ride as passengers
    std::unordered_set<uint32> preferredMounts; // Preferred mount spell IDs

    MountAutomationProfile() = default;
};

/**
 * @brief Mount Manager - Complete mount automation for bots
 *
 * **Phase 6.2: Per-Bot Instance Pattern (25th Manager)**
 *
 * Features:
 * - Auto-mount for long-distance travel
 * - Flying mount support with zone detection
 * - Dragonriding support (WoW 10.0+)
 * - Aquatic mount support
 * - Multi-passenger mounts
 * - Mount collection tracking
 * - Riding skill management
 * - Zone-based mount selection
 * - Performance optimized (per-bot isolation, zero mutex)
 *
 * **Ownership:**
 * - Owned by GameSystemsManager (25th manager)
 * - Each bot has independent mount state
 * - Shared mount database across all bots (static)
 */
class TC_GAME_API MountManager final
{
public:
    /**
     * @brief Construct mount manager for specific bot
     * @param bot The bot player this manager serves
     */
    explicit MountManager(Player* bot);
    ~MountManager();

    // ============================================================================
    // CORE MOUNT MANAGEMENT
    // ============================================================================

    /**
     * Initialize mount system on server startup
     */
    void Initialize();

    /**
     * Update mount automation for player (called periodically)
     */
    void Update(uint32 diff);

    /**
     * Mount player with best available mount
     */
    bool MountPlayer();

    /**
     * Dismount player
     */
    bool DismountPlayer();

    /**
     * Check if player is mounted
     */
    bool IsMounted() const;

    /**
     * Check if player should auto-mount (distance check)
     */
    bool ShouldAutoMount(Position const& destination) const;

    // ============================================================================
    // MOUNT SELECTION
    // ============================================================================

    /**
     * Get best mount for current zone and player state
     */
    MountInfo const* GetBestMount() const;

    /**
     * Get flying mount if zone allows flying
     */
    MountInfo const* GetFlyingMount() const;

    /**
     * Get ground mount
     */
    MountInfo const* GetGroundMount() const;

    /**
     * Get aquatic mount for underwater travel
     */
    MountInfo const* GetAquaticMount() const;

    /**
     * Get dragonriding mount
     */
    MountInfo const* GetDragonridingMount() const;

    /**
     * Check if player can use flying mount in current zone
     */
    bool CanUseFlyingMount() const;

    /**
     * Check if player is underwater
     */
    bool IsPlayerUnderwater() const;

    /**
     * Check if zone allows dragonriding
     */
    bool CanUseDragonriding() const;

    // ============================================================================
    // MOUNT COLLECTION
    // ============================================================================

    /**
     * Get all mounts player knows
     */
    std::vector<MountInfo> GetPlayerMounts() const;

    /**
     * Check if player knows mount
     */
    bool KnowsMount(uint32 spellId) const;

    /**
     * Learn mount spell
     */
    bool LearnMount(uint32 spellId);

    /**
     * Get mount count for player
     */
    uint32 GetMountCount() const;

    /**
     * Check if mount is usable by player (level, skill, class restrictions)
     */
    bool CanUseMount(MountInfo const& mount) const;

    // ============================================================================
    // RIDING SKILL
    // ============================================================================

    /**
     * Get player riding skill level
     */
    uint32 GetRidingSkill() const;

    /**
     * Check if player has riding skill
     */
    bool HasRidingSkill() const;

    /**
     * Learn riding skill (apprentice, journeyman, expert, artisan, master)
     */
    bool LearnRidingSkill(uint32 skillLevel);

    /**
     * Get max mount speed based on riding skill
     */
    MountSpeed GetMaxMountSpeed() const;

    /**
     * Update riding skill and mounts for bot's current level
     * Called during level-up to automatically teach riding skills and mounts
     * Level thresholds: 10 (Apprentice), 20 (Journeyman), 30 (Expert), 40 (Artisan), 80 (Master)
     */
    bool UpdateRidingForLevel();

    // ============================================================================
    // MULTI-PASSENGER MOUNTS
    // ============================================================================

    /**
     * Check if mount is multi-passenger
     */
    bool IsMultiPassengerMount(MountInfo const& mount) const;

    /**
     * Get available passenger seats
     */
    uint32 GetAvailablePassengerSeats() const;

    /**
     * Add passenger to mount
     */
    bool AddPassenger(::Player* passenger);

    /**
     * Remove passenger from mount
     */
    bool RemovePassenger(::Player* passenger);

    // ============================================================================
    // AUTOMATION PROFILES
    // ============================================================================

    void SetAutomationProfile(MountAutomationProfile const& profile);
    MountAutomationProfile GetAutomationProfile() const;

    // ============================================================================
    // METRICS
    // ============================================================================

    struct MountMetrics
    {
        std::atomic<uint32> mountsLearned{0};
        std::atomic<uint32> timesMounted{0};
        std::atomic<uint32> timesDismounted{0};
        std::atomic<uint32> flyingMountUsage{0};
        std::atomic<uint32> dragonridingUsage{0};
        std::atomic<uint64> totalMountedTime{0};  // Milliseconds

        void Reset()
        {
            mountsLearned = 0;
            timesMounted = 0;
            timesDismounted = 0;
            flyingMountUsage = 0;
            dragonridingUsage = 0;
            totalMountedTime = 0;
        }
    };

    MountMetrics const& GetMetrics() const;
    MountMetrics const& GetGlobalMetrics() const;

private:
    // Non-copyable
    MountManager(MountManager const&) = delete;
    MountManager& operator=(MountManager const&) = delete;

    // ============================================================================
    // INITIALIZATION HELPERS
    // ============================================================================

    void LoadMountDatabase();
    void InitializeVanillaMounts();
    void InitializeTBCMounts();
    void InitializeWrathMounts();
    void InitializeCataclysmMounts();
    void InitializePandariaMounts();
    void InitializeDraenorMounts();
    void InitializeLegionMounts();
    void InitializeBfAMounts();
    void InitializeShadowlandsMounts();
    void InitializeDragonflightMounts();
    void InitializeWarWithinMounts();

    // ============================================================================
    // MOUNT CASTING HELPERS
    // ============================================================================

    bool CastMountSpell(uint32 spellId);
    bool CanCastMountSpell(uint32 spellId) const;
    void HandleMountCastResult(uint32 spellId, bool success);

    // ============================================================================
    // ZONE DETECTION HELPERS
    // ============================================================================

    bool IsInNoFlyZone() const;
    bool IsInDragonridingZone() const;
    bool IsInAquaticZone() const;
    uint32 GetCurrentZoneId() const;

    // ============================================================================
    // VALIDATION HELPERS
    // ============================================================================

    bool ValidateMountUsage() const;
    bool IsInCombat() const;
    bool IsIndoors() const;
    bool IsInInstance() const;

    // ============================================================================
    // MOUNT SELECTION HELPERS (Race/Faction based)
    // ============================================================================

    /**
     * Get an appropriate ground mount spell ID based on bot's race and level
     */
    uint32 GetRaceAppropriateMount(uint32 level) const;

    /**
     * Get an appropriate flying mount spell ID based on bot's faction and level
     */
    uint32 GetRaceAppropriateFlyingMount(uint32 level) const;

    // ============================================================================
    // DATA STRUCTURES
    // ============================================================================

    // Bot reference (non-owning)
    Player* _bot;

    // Per-bot instance data
    MountAutomationProfile _profile;
    std::unordered_set<uint32> _knownMounts;
    uint32 _currentMount{0};
    uint32 _mountTimestamp{0};
    MountMetrics _metrics;
    uint32 _lastUpdateTime{0};

    // Shared static data (all bots)
    static std::unordered_map<uint32, MountInfo> _mountDatabase;
    static bool _mountDatabaseInitialized;
    static MountMetrics _globalMetrics;

    // Update intervals
    static constexpr uint32 MOUNT_UPDATE_INTERVAL = 5000;  // 5 seconds

    // Mount spell IDs (examples - full list loaded from DB/DBC)
    static constexpr uint32 SPELL_MOUNT_RIDING_APPRENTICE = 33388;
    static constexpr uint32 SPELL_MOUNT_RIDING_JOURNEYMAN = 33391;
    static constexpr uint32 SPELL_MOUNT_RIDING_EXPERT = 34090;
    static constexpr uint32 SPELL_MOUNT_RIDING_ARTISAN = 34091;
    static constexpr uint32 SPELL_MOUNT_RIDING_MASTER = 90265;
};

} // namespace Playerbot
