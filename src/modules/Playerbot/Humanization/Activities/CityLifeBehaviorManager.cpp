/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "CityLifeBehaviorManager.h"
#include "AI/BotAI.h"
#include "Player.h"
#include "GameObject.h"
#include "Creature.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Map.h"
#include "Log.h"
#include "GameTime.h"
#include "MotionMaster.h"
#include "../../Spatial/SpatialGridManager.h"
#include <algorithm>
#include <random>

namespace Playerbot
{
namespace Humanization
{

CityLifeBehaviorManager::CityLifeBehaviorManager(Player* bot, BotAI* ai)
    : BehaviorManager(bot, ai, 2000, "CityLifeBehaviorManager")  // 2 second update
{
}

bool CityLifeBehaviorManager::OnInitialize()
{
    if (!GetBot() || !GetBot()->IsInWorld())
        return false;

    _lastCityScan = std::chrono::steady_clock::now();
    UpdateCityDetection();

    return true;
}

void CityLifeBehaviorManager::OnShutdown()
{
    if (_currentSession.state != CityActivityState::INACTIVE)
    {
        StopActivity("Shutdown");
    }

    _nearbyLocations.clear();
}

void CityLifeBehaviorManager::OnUpdate(uint32 elapsed)
{
    if (!GetBot() || !GetBot()->IsInWorld())
        return;

    // Update city detection periodically
    auto now = std::chrono::steady_clock::now();
    auto timeSinceScan = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastCityScan).count();
    if (timeSinceScan >= CITY_SCAN_INTERVAL_MS)
    {
        UpdateCityDetection();
        _lastCityScan = now;
    }

    // Process current activity
    if (_currentSession.state != CityActivityState::INACTIVE)
    {
        ProcessActivity(elapsed);
    }
}

void CityLifeBehaviorManager::UpdateCityDetection()
{
    if (!GetBot() || !GetBot()->GetMap())
        return;

    // Check if in a city zone
    uint32 zoneId = GetBot()->GetZoneId();
    uint32 areaId = GetBot()->GetAreaId();

    // Check for common city zones
    bool inCity = false;

    // Check for common city zones
    if (!inCity)
    {
        // Alliance capitals
        static const uint32 cityZones[] = {
            1519,   // Stormwind City
            1537,   // Ironforge
            1657,   // Darnassus
            3557,   // The Exodar
            // Horde capitals
            1637,   // Orgrimmar
            1638,   // Thunder Bluff
            1497,   // Undercity
            3487,   // Silvermoon City
            // Neutral cities
            1,      // Dun Morogh (starting zone with city)
            3703,   // Shattrath City
            4395,   // Dalaran
        };

        for (uint32 cityZone : cityZones)
        {
            if (zoneId == cityZone || areaId == cityZone)
            {
                inCity = true;
                break;
            }
        }
    }

    _isInCity.store(inCity, std::memory_order_release);
    _cityZoneId = zoneId;

    // Scan for nearby services if in city
    if (inCity)
    {
        _nearbyLocations = ScanForCityServices(DEFAULT_DETECTION_RANGE);
        _hasNearbyServices.store(!_nearbyLocations.empty(), std::memory_order_release);
    }
    else
    {
        _nearbyLocations.clear();
        _hasNearbyServices.store(false, std::memory_order_release);
    }
}

std::vector<CityLocation> CityLifeBehaviorManager::ScanForCityServices(float range)
{
    std::vector<CityLocation> locations;

    if (!GetBot() || !GetBot()->IsInWorld())
        return locations;

    // Scan for each type of service
    auto ahLocations = DetectAuctionHouses(range);
    locations.insert(locations.end(), ahLocations.begin(), ahLocations.end());

    auto mailboxLocations = DetectMailboxes(range);
    locations.insert(locations.end(), mailboxLocations.begin(), mailboxLocations.end());

    auto bankLocations = DetectBanks(range);
    locations.insert(locations.end(), bankLocations.begin(), bankLocations.end());

    auto vendorLocations = DetectVendors(range);
    locations.insert(locations.end(), vendorLocations.begin(), vendorLocations.end());

    auto trainerLocations = DetectTrainers(range);
    locations.insert(locations.end(), trainerLocations.begin(), trainerLocations.end());

    auto innkeeperLocations = DetectInnkeepers(range);
    locations.insert(locations.end(), innkeeperLocations.begin(), innkeeperLocations.end());

    auto transmogLocations = DetectTransmogrifiers(range);
    locations.insert(locations.end(), transmogLocations.begin(), transmogLocations.end());

    // Sort by distance
    Position botPos = GetBot()->GetPosition();
    std::sort(locations.begin(), locations.end(),
        [&botPos](CityLocation const& a, CityLocation const& b) {
            float distA = botPos.GetExactDist(a.position);
            float distB = botPos.GetExactDist(b.position);
            return distA < distB;
        });

    return locations;
}

CityLocation const* CityLifeBehaviorManager::FindNearestLocation(ActivityType activityType) const
{
    CityLocation const* nearest = nullptr;
    float minDistance = std::numeric_limits<float>::max();

    if (!GetBot())
        return nullptr;

    for (auto const& location : _nearbyLocations)
    {
        if (location.activityType != activityType)
            continue;

        float distance = GetBot()->GetExactDist(location.position);
        if (distance < minDistance)
        {
            minDistance = distance;
            nearest = &location;
        }
    }

    return nearest;
}

bool CityLifeBehaviorManager::IsServiceNearby(ActivityType activityType, float range) const
{
    if (!GetBot())
        return false;

    for (auto const& location : _nearbyLocations)
    {
        if (location.activityType != activityType)
            continue;

        float distance = GetBot()->GetExactDist(location.position);
        if (distance <= range)
            return true;
    }

    return false;
}

bool CityLifeBehaviorManager::StartActivity(
    ActivityType activityType,
    uint32 durationMs,
    PersonalityProfile const* personality)
{
    if (_currentSession.state != CityActivityState::INACTIVE)
    {
        TC_LOG_DEBUG("module.playerbot.humanization",
            "CityLifeBehaviorManager: Activity already in progress for bot {}",
            GetBot() ? GetBot()->GetName() : "unknown");
        return false;
    }

    if (personality)
        _personality = personality;

    // Find target location
    CityLocation const* target = FindNearestLocation(activityType);
    if (!target)
    {
        TC_LOG_DEBUG("module.playerbot.humanization",
            "CityLifeBehaviorManager: No location found for activity {} for bot {}",
            static_cast<uint32>(activityType),
            GetBot() ? GetBot()->GetName() : "unknown");
        return false;
    }

    // Initialize session
    _currentSession.Reset();
    _currentSession.activityType = activityType;
    _currentSession.targetLocation = *target;
    _currentSession.startTime = std::chrono::steady_clock::now();
    _currentSession.durationMs = durationMs > 0 ? durationMs : CalculateActivityDuration(activityType);

    // Start by traveling to location if not already there
    if (IsAtLocation(*target))
    {
        TransitionState(CityActivityState::INTERACTING);
    }
    else
    {
        TransitionState(CityActivityState::TRAVELING);
        MoveToLocation(*target);
    }

    _statistics.totalActivities++;

    TC_LOG_DEBUG("module.playerbot.humanization",
        "CityLifeBehaviorManager: Started {} activity for bot {}, duration {}ms",
        GetActivityName(activityType),
        GetBot() ? GetBot()->GetName() : "unknown",
        _currentSession.durationMs);

    return true;
}

void CityLifeBehaviorManager::StopActivity(std::string const& reason)
{
    if (_currentSession.state == CityActivityState::INACTIVE)
        return;

    // Stop any movement
    if (GetBot())
        GetBot()->GetMotionMaster()->Clear();

    // Update statistics
    _statistics.totalTimeInCityMs += _currentSession.GetElapsedMs();

    ActivityType activity = _currentSession.activityType;
    _currentSession.Reset();
    TransitionState(CityActivityState::INACTIVE);

    TC_LOG_DEBUG("module.playerbot.humanization",
        "CityLifeBehaviorManager: Stopped {} activity for bot {}, reason: {}",
        GetActivityName(activity),
        GetBot() ? GetBot()->GetName() : "unknown",
        reason.empty() ? "none" : reason.c_str());
}

float CityLifeBehaviorManager::GetActivityProgress() const
{
    if (_currentSession.state == CityActivityState::INACTIVE || _currentSession.durationMs == 0)
        return 0.0f;

    uint32 elapsed = _currentSession.GetElapsedMs();
    return std::min(1.0f, static_cast<float>(elapsed) / static_cast<float>(_currentSession.durationMs));
}

bool CityLifeBehaviorManager::StartAuctionBrowsing(uint32 durationMs)
{
    return StartActivity(ActivityType::AUCTION_BROWSING,
        durationMs > 0 ? durationMs : DEFAULT_AH_BROWSE_MS);
}

bool CityLifeBehaviorManager::StartAuctionPosting()
{
    return StartActivity(ActivityType::AUCTION_POSTING, DEFAULT_AH_BROWSE_MS / 2);
}

bool CityLifeBehaviorManager::StartMailboxCheck()
{
    return StartActivity(ActivityType::MAILBOX_CHECK, DEFAULT_MAILBOX_MS);
}

bool CityLifeBehaviorManager::StartBankVisit()
{
    return StartActivity(ActivityType::BANK_VISIT, DEFAULT_BANK_MS);
}

bool CityLifeBehaviorManager::StartVendorVisit()
{
    return StartActivity(ActivityType::VENDOR_VISIT, DEFAULT_VENDOR_MS);
}

bool CityLifeBehaviorManager::StartTrainerVisit()
{
    return StartActivity(ActivityType::TRAINER_VISIT, DEFAULT_TRAINER_MS);
}

bool CityLifeBehaviorManager::StartInnRest(uint32 durationMs)
{
    return StartActivity(ActivityType::INN_REST,
        durationMs > 0 ? durationMs : DEFAULT_INN_REST_MS);
}

bool CityLifeBehaviorManager::StartCityWandering(uint32 durationMs)
{
    return StartActivity(ActivityType::CITY_WANDERING,
        durationMs > 0 ? durationMs : DEFAULT_WANDERING_MS);
}

bool CityLifeBehaviorManager::StartTransmogBrowsing(uint32 durationMs)
{
    return StartActivity(ActivityType::TRANSMOG_BROWSING,
        durationMs > 0 ? durationMs : DEFAULT_TRANSMOG_MS);
}

void CityLifeBehaviorManager::SetInteractionDelay(uint32 minMs, uint32 maxMs)
{
    _interactionDelayMinMs = minMs;
    _interactionDelayMaxMs = maxMs;
}

void CityLifeBehaviorManager::ProcessActivity(uint32 /*elapsed*/)
{
    // Check if activity duration exceeded
    if (_currentSession.GetElapsedMs() >= _currentSession.durationMs)
    {
        CompleteActivity();
        return;
    }

    // Check for combat interruption
    if (GetBot() && GetBot()->IsInCombat())
    {
        StopActivity("Combat");
        return;
    }

    // Process based on current state
    switch (_currentSession.state)
    {
        case CityActivityState::TRAVELING:
            ProcessTraveling();
            break;
        case CityActivityState::INTERACTING:
            ProcessInteracting();
            break;
        case CityActivityState::BROWSING:
            ProcessBrowsing();
            break;
        case CityActivityState::WAITING:
            ProcessWaiting();
            break;
        case CityActivityState::COMPLETING:
            CompleteActivity();
            break;
        default:
            break;
    }
}

void CityLifeBehaviorManager::ProcessTraveling()
{
    if (!GetBot())
        return;

    // Check if arrived at destination
    if (IsAtLocation(_currentSession.targetLocation))
    {
        TransitionState(CityActivityState::INTERACTING);
        return;
    }

    // Check if movement stopped unexpectedly
    if (!GetBot()->isMoving())
    {
        // Re-attempt movement
        if (!MoveToLocation(_currentSession.targetLocation))
        {
            StopActivity("Movement failed");
        }
    }
}

void CityLifeBehaviorManager::ProcessInteracting()
{
    // Attempt to interact with target
    if (InteractWithTarget())
    {
        _currentSession.interactionCount++;

        // Transition based on activity type
        switch (_currentSession.activityType)
        {
            case ActivityType::AUCTION_BROWSING:
            case ActivityType::TRANSMOG_BROWSING:
                _statistics.auctionSearches++;
                TransitionState(CityActivityState::BROWSING);
                break;

            case ActivityType::AUCTION_POSTING:
                _statistics.auctionPosts++;
                // Add delay then continue browsing
                _waitDurationMs = CalculateInteractionDelay();
                TransitionState(CityActivityState::WAITING);
                break;

            case ActivityType::MAILBOX_CHECK:
                _statistics.mailsChecked++;
                _waitDurationMs = CalculateInteractionDelay();
                TransitionState(CityActivityState::WAITING);
                break;

            case ActivityType::VENDOR_VISIT:
                _statistics.vendorInteractions++;
                _waitDurationMs = CalculateInteractionDelay() * 2;  // Longer for selling
                TransitionState(CityActivityState::WAITING);
                break;

            case ActivityType::TRAINER_VISIT:
                _statistics.trainerInteractions++;
                _waitDurationMs = CalculateInteractionDelay();
                TransitionState(CityActivityState::WAITING);
                break;

            case ActivityType::BANK_VISIT:
                _waitDurationMs = CalculateInteractionDelay() * 3;  // Longer for bank
                TransitionState(CityActivityState::WAITING);
                break;

            case ActivityType::INN_REST:
                // Just wait until duration expires
                TransitionState(CityActivityState::WAITING);
                _waitDurationMs = _currentSession.durationMs;
                break;

            case ActivityType::CITY_WANDERING:
                // Handled separately - pick random destination
                TransitionState(CityActivityState::WAITING);
                _waitDurationMs = 5000 + (rand() % 10000);  // 5-15 seconds between moves
                break;

            default:
                TransitionState(CityActivityState::COMPLETING);
                break;
        }
    }
    else
    {
        // Failed to interact, try again after delay
        _waitDurationMs = 2000;
        TransitionState(CityActivityState::WAITING);
    }
}

void CityLifeBehaviorManager::ProcessBrowsing()
{
    SimulateBrowsing();

    // Add random delay between browse actions
    _waitDurationMs = CalculateInteractionDelay();
    TransitionState(CityActivityState::WAITING);
}

void CityLifeBehaviorManager::ProcessWaiting()
{
    if (_currentSession.GetStateElapsedMs() >= _waitDurationMs)
    {
        // Waiting complete, determine next state
        if (_currentSession.GetElapsedMs() >= _currentSession.durationMs)
        {
            TransitionState(CityActivityState::COMPLETING);
        }
        else if (_currentSession.activityType == ActivityType::CITY_WANDERING)
        {
            // Pick new random destination for wandering
            TransitionState(CityActivityState::TRAVELING);
            // Wandering destination set by TravelingState handler using random city waypoint
        }
        else if (_currentSession.activityType == ActivityType::AUCTION_BROWSING ||
                 _currentSession.activityType == ActivityType::TRANSMOG_BROWSING)
        {
            // Continue browsing
            TransitionState(CityActivityState::BROWSING);
        }
        else
        {
            // Most activities just complete after interaction + wait
            TransitionState(CityActivityState::COMPLETING);
        }
    }
}

void CityLifeBehaviorManager::TransitionState(CityActivityState newState)
{
    if (_currentSession.state == newState)
        return;

    _currentSession.state = newState;
    _currentSession.stateStartTime = std::chrono::steady_clock::now();

    NotifyStateChange();

    TC_LOG_DEBUG("module.playerbot.humanization",
        "CityLifeBehaviorManager: State transition to {} for bot {}",
        static_cast<uint32>(newState),
        GetBot() ? GetBot()->GetName() : "unknown");
}

uint32 CityLifeBehaviorManager::CalculateActivityDuration(ActivityType activityType) const
{
    uint32 baseDuration = 0;

    switch (activityType)
    {
        case ActivityType::AUCTION_BROWSING:
            baseDuration = DEFAULT_AH_BROWSE_MS;
            break;
        case ActivityType::AUCTION_POSTING:
            baseDuration = DEFAULT_AH_BROWSE_MS / 2;
            break;
        case ActivityType::MAILBOX_CHECK:
            baseDuration = DEFAULT_MAILBOX_MS;
            break;
        case ActivityType::BANK_VISIT:
            baseDuration = DEFAULT_BANK_MS;
            break;
        case ActivityType::VENDOR_VISIT:
            baseDuration = DEFAULT_VENDOR_MS;
            break;
        case ActivityType::TRAINER_VISIT:
            baseDuration = DEFAULT_TRAINER_MS;
            break;
        case ActivityType::INN_REST:
            baseDuration = DEFAULT_INN_REST_MS;
            break;
        case ActivityType::CITY_WANDERING:
            baseDuration = DEFAULT_WANDERING_MS;
            break;
        case ActivityType::TRANSMOG_BROWSING:
            baseDuration = DEFAULT_TRANSMOG_MS;
            break;
        default:
            baseDuration = 60000;  // 1 minute default
            break;
    }

    // Apply personality modifier if available
    if (_personality)
    {
        // Use session duration multiplier from personality traits
        float modifier = _personality->GetTraits().sessionDurationMultiplier;
        baseDuration = static_cast<uint32>(baseDuration * modifier);
    }

    // Add some randomness (+/- 20%)
    uint32 variance = baseDuration / 5;
    int32 offset = static_cast<int32>(rand() % (variance * 2)) - static_cast<int32>(variance);
    return static_cast<uint32>(std::max(10000, static_cast<int32>(baseDuration) + offset));
}

uint32 CityLifeBehaviorManager::CalculateInteractionDelay() const
{
    uint32 range = _interactionDelayMaxMs - _interactionDelayMinMs;
    return _interactionDelayMinMs + (rand() % range);
}

std::vector<CityLocation> CityLifeBehaviorManager::DetectAuctionHouses(float range)
{
    std::vector<CityLocation> locations;

    if (!GetBot() || !GetBot()->GetMap())
        return locations;

    // Use spatial grid for lock-free detection
    Map* map = GetBot()->GetMap();
    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
        return locations;

    auto nearbyCreatures = spatialGrid->QueryNearbyCreatures(GetBot()->GetPosition(), range);

    for (auto const& snapshot : nearbyCreatures)
    {
        // Auctioneer NPC flag check would go here
        // For now, check by name pattern or entry (simplified)
        if (snapshot.health > 0 && !snapshot.isHostile)
        {
            // This is simplified - real implementation would check NPC flags
            CityLocation loc;
            loc.guid = snapshot.guid;
            loc.position = snapshot.position;
            loc.activityType = ActivityType::AUCTION_BROWSING;
            loc.name = "Auctioneer";
            // Would check actual creature entry/flags here
        }
    }

    return locations;
}

std::vector<CityLocation> CityLifeBehaviorManager::DetectMailboxes(float range)
{
    std::vector<CityLocation> locations;

    if (!GetBot() || !GetBot()->GetMap())
        return locations;

    Map* map = GetBot()->GetMap();
    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
        return locations;

    auto nearbyObjects = spatialGrid->QueryNearbyGameObjects(GetBot()->GetPosition(), range);

    for (auto const& snapshot : nearbyObjects)
    {
        // GAMEOBJECT_TYPE_MAILBOX = 19
        if (snapshot.goType == 19 && snapshot.isSpawned)
        {
            CityLocation loc;
            loc.guid = snapshot.guid;
            loc.position = snapshot.position;
            loc.activityType = ActivityType::MAILBOX_CHECK;
            loc.name = "Mailbox";
            locations.push_back(loc);
        }
    }

    return locations;
}

std::vector<CityLocation> CityLifeBehaviorManager::DetectBanks(float range)
{
    std::vector<CityLocation> locations;
    // Similar pattern to mailboxes - look for banker NPCs
    return locations;
}

std::vector<CityLocation> CityLifeBehaviorManager::DetectVendors(float range)
{
    std::vector<CityLocation> locations;
    // Look for NPCs with vendor flag
    return locations;
}

std::vector<CityLocation> CityLifeBehaviorManager::DetectTrainers(float range)
{
    std::vector<CityLocation> locations;
    // Look for NPCs with trainer flag
    return locations;
}

std::vector<CityLocation> CityLifeBehaviorManager::DetectInnkeepers(float range)
{
    std::vector<CityLocation> locations;
    // Look for NPCs with innkeeper flag
    return locations;
}

std::vector<CityLocation> CityLifeBehaviorManager::DetectTransmogrifiers(float range)
{
    std::vector<CityLocation> locations;
    // Look for ethereal transmogrifiers
    return locations;
}

bool CityLifeBehaviorManager::MoveToLocation(CityLocation const& location)
{
    if (!GetBot() || !GetAI())
        return false;

    GetAI()->MoveTo(
        location.position.GetPositionX(),
        location.position.GetPositionY(),
        location.position.GetPositionZ());

    return true;
}

bool CityLifeBehaviorManager::IsAtLocation(CityLocation const& location) const
{
    if (!GetBot())
        return false;

    return GetBot()->GetExactDist(location.position) <= INTERACTION_RANGE;
}

bool CityLifeBehaviorManager::InteractWithTarget()
{
    if (!GetBot())
        return false;

    // For now, just simulate interaction
    // Real implementation would open gossip windows, etc.
    return true;
}

void CityLifeBehaviorManager::SimulateBrowsing()
{
    // Simulates browsing action - real implementation would
    // interact with AH API, search for items, etc.
    _currentSession.interactionCount++;
}

void CityLifeBehaviorManager::CompleteActivity()
{
    _currentSession.isComplete = true;
    _statistics.completedActivities++;
    _statistics.totalTimeInCityMs += _currentSession.GetElapsedMs();

    TC_LOG_DEBUG("module.playerbot.humanization",
        "CityLifeBehaviorManager: Completed {} activity for bot {}, interactions: {}",
        GetActivityName(_currentSession.activityType),
        GetBot() ? GetBot()->GetName() : "unknown",
        _currentSession.interactionCount);

    _currentSession.Reset();
    TransitionState(CityActivityState::INACTIVE);
}

void CityLifeBehaviorManager::NotifyStateChange()
{
    if (_activityCallback)
    {
        _activityCallback(_currentSession.activityType, _currentSession.state);
    }
}

} // namespace Humanization
} // namespace Playerbot
