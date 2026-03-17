/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * CITY LIFE BEHAVIOR MANAGER
 *
 * Phase 3: Humanization Core (Task 5)
 *
 * Manages city-based activities for bots with humanized behavior:
 * - Auction house browsing and posting
 * - Mailbox checking
 * - Bank visits
 * - Vendor interactions
 * - Trainer visits
 * - Inn resting
 * - City wandering
 * - Transmog browsing
 */

#pragma once

#include "Define.h"
#include "AI/BehaviorManager.h"
#include "../Core/ActivityType.h"
#include "../Core/PersonalityProfile.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <atomic>
#include <vector>
#include <chrono>
#include <functional>

class Player;
class GameObject;
class Creature;

namespace Playerbot
{

class BotAI;

namespace Humanization
{

/**
 * @enum CityActivityState
 * @brief Current state of city activity
 */
enum class CityActivityState : uint8
{
    INACTIVE = 0,       // No active city activity
    TRAVELING,          // Moving to destination
    INTERACTING,        // Interacting with NPC/object
    BROWSING,           // Browsing (AH, transmog, etc.)
    WAITING,            // Waiting (animation delay, etc.)
    COMPLETING,         // Wrapping up activity
    COMPLETED           // Activity finished
};

/**
 * @struct CityLocation
 * @brief Information about a city location (vendor, AH, bank, etc.)
 */
struct CityLocation
{
    ObjectGuid guid;            // NPC or GameObject GUID
    uint32 entry;               // Entry ID
    Position position;          // Location
    ActivityType activityType;  // What activity can be done here
    std::string name;           // Friendly name

    CityLocation()
        : entry(0)
        , activityType(ActivityType::NONE)
    {}
};

/**
 * @struct CityActivitySession
 * @brief Tracks a city activity session
 */
struct CityActivitySession
{
    ActivityType activityType{ActivityType::NONE};
    CityActivityState state{CityActivityState::INACTIVE};
    CityLocation targetLocation;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point stateStartTime;
    uint32 durationMs{0};
    uint32 interactionCount{0};     // Number of interactions (AH searches, items sold, etc.)
    bool isComplete{false};

    void Reset()
    {
        activityType = ActivityType::NONE;
        state = CityActivityState::INACTIVE;
        targetLocation = CityLocation();
        durationMs = 0;
        interactionCount = 0;
        isComplete = false;
    }

    uint32 GetElapsedMs() const
    {
        auto now = std::chrono::steady_clock::now();
        return static_cast<uint32>(
            std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count());
    }

    uint32 GetStateElapsedMs() const
    {
        auto now = std::chrono::steady_clock::now();
        return static_cast<uint32>(
            std::chrono::duration_cast<std::chrono::milliseconds>(now - stateStartTime).count());
    }
};

/**
 * @brief Callback for activity state changes
 */
using CityActivityCallback = std::function<void(ActivityType, CityActivityState)>;

/**
 * @class CityLifeBehaviorManager
 * @brief Manages city-based activities with humanized behavior
 *
 * This manager handles all city life activities:
 * - Detects nearby city services (AH, bank, mailbox, vendors, trainers)
 * - Manages activity sessions with natural timing
 * - Simulates browsing and interaction delays
 * - Integrates with humanization system for personality-based behavior
 *
 * Update interval: 2000ms (2 seconds)
 */
class TC_GAME_API CityLifeBehaviorManager : public BehaviorManager
{
public:
    explicit CityLifeBehaviorManager(Player* bot, BotAI* ai);
    ~CityLifeBehaviorManager() override = default;

    // ========================================================================
    // FAST STATE QUERIES (<0.001ms atomic reads)
    // ========================================================================

    /**
     * @brief Check if bot is in a city
     */
    bool IsInCity() const { return _isInCity.load(std::memory_order_acquire); }

    /**
     * @brief Check if bot is currently doing city activity
     */
    bool IsActive() const { return _currentSession.state != CityActivityState::INACTIVE; }

    /**
     * @brief Get current activity type
     */
    ActivityType GetCurrentActivity() const { return _currentSession.activityType; }

    /**
     * @brief Get current activity state
     */
    CityActivityState GetCurrentState() const { return _currentSession.state; }

    // ========================================================================
    // LOCATION DETECTION
    // ========================================================================

    /**
     * @brief Scan for nearby city services
     * @param range Detection range in yards
     * @return Vector of detected city locations
     */
    std::vector<CityLocation> ScanForCityServices(float range = 50.0f);

    /**
     * @brief Find nearest location for activity type
     * @param activityType Activity to find location for
     * @return Pointer to nearest location, nullptr if none found
     */
    CityLocation const* FindNearestLocation(ActivityType activityType) const;

    /**
     * @brief Check if specific service is nearby
     * @param activityType Activity type to check
     * @param range Detection range
     * @return true if service is nearby
     */
    bool IsServiceNearby(ActivityType activityType, float range = 50.0f) const;

    // ========================================================================
    // ACTIVITY CONTROL
    // ========================================================================

    /**
     * @brief Start a city activity
     * @param activityType Activity to start
     * @param durationMs Target duration (0 = use personality default)
     * @param personality Personality for behavior tuning
     * @return true if activity started
     */
    bool StartActivity(
        ActivityType activityType,
        uint32 durationMs = 0,
        PersonalityProfile const* personality = nullptr);

    /**
     * @brief Stop current city activity
     * @param reason Why stopping
     */
    void StopActivity(std::string const& reason = "");

    /**
     * @brief Get current session info
     */
    CityActivitySession const& GetCurrentSession() const { return _currentSession; }

    /**
     * @brief Get activity progress (0.0 to 1.0)
     */
    float GetActivityProgress() const;

    // ========================================================================
    // SPECIFIC ACTIVITIES
    // ========================================================================

    /**
     * @brief Start auction house browsing
     * @param durationMs Browse duration
     * @return true if started
     */
    bool StartAuctionBrowsing(uint32 durationMs = 0);

    /**
     * @brief Start posting items to auction house
     * @return true if started
     */
    bool StartAuctionPosting();

    /**
     * @brief Check mailbox
     * @return true if started
     */
    bool StartMailboxCheck();

    /**
     * @brief Visit bank
     * @return true if started
     */
    bool StartBankVisit();

    /**
     * @brief Visit vendor (sell items, buy supplies)
     * @return true if started
     */
    bool StartVendorVisit();

    /**
     * @brief Visit trainer
     * @return true if started
     */
    bool StartTrainerVisit();

    /**
     * @brief Rest at inn
     * @param durationMs Rest duration
     * @return true if started
     */
    bool StartInnRest(uint32 durationMs = 0);

    /**
     * @brief Wander around city
     * @param durationMs Wander duration
     * @return true if started
     */
    bool StartCityWandering(uint32 durationMs = 0);

    /**
     * @brief Browse transmog at ethereals
     * @param durationMs Browse duration
     * @return true if started
     */
    bool StartTransmogBrowsing(uint32 durationMs = 0);

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Set personality for behavior tuning
     * @param personality Personality profile
     */
    void SetPersonality(PersonalityProfile const* personality) { _personality = personality; }

    /**
     * @brief Set callback for activity state changes
     * @param callback Callback function
     */
    void SetActivityCallback(CityActivityCallback callback) { _activityCallback = std::move(callback); }

    /**
     * @brief Set minimum interaction delay (for humanization)
     * @param minMs Minimum delay in milliseconds
     * @param maxMs Maximum delay in milliseconds
     */
    void SetInteractionDelay(uint32 minMs, uint32 maxMs);

    // ========================================================================
    // STATISTICS
    // ========================================================================

    struct CityLifeStatistics
    {
        std::atomic<uint32> totalActivities{0};
        std::atomic<uint32> completedActivities{0};
        std::atomic<uint32> auctionSearches{0};
        std::atomic<uint32> auctionPosts{0};
        std::atomic<uint32> mailsChecked{0};
        std::atomic<uint32> vendorInteractions{0};
        std::atomic<uint32> trainerInteractions{0};
        std::atomic<uint64> totalTimeInCityMs{0};

        void Reset()
        {
            totalActivities = 0;
            completedActivities = 0;
            auctionSearches = 0;
            auctionPosts = 0;
            mailsChecked = 0;
            vendorInteractions = 0;
            trainerInteractions = 0;
            totalTimeInCityMs = 0;
        }
    };

    CityLifeStatistics const& GetStatistics() const { return _statistics; }

protected:
    // ========================================================================
    // BEHAVIOR MANAGER INTERFACE
    // ========================================================================

    void OnUpdate(uint32 elapsed);
    bool OnInitialize();
    void OnShutdown();

private:
    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /**
     * @brief Update city detection
     */
    void UpdateCityDetection();

    /**
     * @brief Process current activity state
     * @param elapsed Time since last update
     */
    void ProcessActivity(uint32 elapsed);

    /**
     * @brief Process TRAVELING state
     */
    void ProcessTraveling();

    /**
     * @brief Process INTERACTING state
     */
    void ProcessInteracting();

    /**
     * @brief Process BROWSING state
     */
    void ProcessBrowsing();

    /**
     * @brief Process WAITING state
     */
    void ProcessWaiting();

    /**
     * @brief Transition to new state
     */
    void TransitionState(CityActivityState newState);

    /**
     * @brief Calculate activity duration based on personality
     */
    uint32 CalculateActivityDuration(ActivityType activityType) const;

    /**
     * @brief Calculate random interaction delay
     */
    uint32 CalculateInteractionDelay() const;

    /**
     * @brief Detect auction houses nearby
     */
    std::vector<CityLocation> DetectAuctionHouses(float range);

    /**
     * @brief Detect mailboxes nearby
     */
    std::vector<CityLocation> DetectMailboxes(float range);

    /**
     * @brief Detect banks nearby
     */
    std::vector<CityLocation> DetectBanks(float range);

    /**
     * @brief Detect vendors nearby
     */
    std::vector<CityLocation> DetectVendors(float range);

    /**
     * @brief Detect trainers nearby
     */
    std::vector<CityLocation> DetectTrainers(float range);

    /**
     * @brief Detect innkeepers nearby
     */
    std::vector<CityLocation> DetectInnkeepers(float range);

    /**
     * @brief Detect transmogrifiers nearby
     */
    std::vector<CityLocation> DetectTransmogrifiers(float range);

    /**
     * @brief Move to target location
     * @return true if movement started
     */
    bool MoveToLocation(CityLocation const& location);

    /**
     * @brief Check if at target location
     * @return true if within interaction range
     */
    bool IsAtLocation(CityLocation const& location) const;

    /**
     * @brief Interact with target (NPC or object)
     * @return true if interaction started
     */
    bool InteractWithTarget();

    /**
     * @brief Simulate browsing action (AH, transmog, etc.)
     */
    void SimulateBrowsing();

    /**
     * @brief Complete current activity
     */
    void CompleteActivity();

    /**
     * @brief Notify callback of state change
     */
    void NotifyStateChange();

    // ========================================================================
    // STATE FLAGS
    // ========================================================================

    std::atomic<bool> _isInCity{false};
    std::atomic<bool> _hasNearbyServices{false};

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    PersonalityProfile const* _personality{nullptr};
    uint32 _interactionDelayMinMs{1000};
    uint32 _interactionDelayMaxMs{3000};

    // ========================================================================
    // CITY DETECTION
    // ========================================================================

    std::vector<CityLocation> _nearbyLocations;
    std::chrono::steady_clock::time_point _lastCityScan;
    uint32 _cityZoneId{0};

    // ========================================================================
    // ACTIVITY STATE
    // ========================================================================

    CityActivitySession _currentSession;
    uint32 _waitDurationMs{0};  // Duration for WAITING state

    // ========================================================================
    // CALLBACKS
    // ========================================================================

    CityActivityCallback _activityCallback;

    // ========================================================================
    // STATISTICS
    // ========================================================================

    CityLifeStatistics _statistics;

    // ========================================================================
    // CONSTANTS
    // ========================================================================

    static constexpr uint32 CITY_SCAN_INTERVAL_MS = 10000;      // 10 seconds
    static constexpr float INTERACTION_RANGE = 5.0f;            // Yards
    static constexpr float DEFAULT_DETECTION_RANGE = 50.0f;     // Yards

    // Default activity durations
    static constexpr uint32 DEFAULT_AH_BROWSE_MS = 120000;      // 2 minutes
    static constexpr uint32 DEFAULT_MAILBOX_MS = 30000;         // 30 seconds
    static constexpr uint32 DEFAULT_BANK_MS = 60000;            // 1 minute
    static constexpr uint32 DEFAULT_VENDOR_MS = 45000;          // 45 seconds
    static constexpr uint32 DEFAULT_TRAINER_MS = 30000;         // 30 seconds
    static constexpr uint32 DEFAULT_INN_REST_MS = 300000;       // 5 minutes
    static constexpr uint32 DEFAULT_WANDERING_MS = 180000;      // 3 minutes
    static constexpr uint32 DEFAULT_TRANSMOG_MS = 180000;       // 3 minutes

    // NPC entry IDs (commonly used)
    static constexpr uint32 GOSSIP_OPTION_VENDOR = 1;
    static constexpr uint32 GOSSIP_OPTION_TRAINER = 2;
    static constexpr uint32 GOSSIP_OPTION_INNKEEPER = 3;
};

} // namespace Humanization
} // namespace Playerbot
