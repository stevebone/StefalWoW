/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "FarmingCoordinator.h"
#include "GameTime.h"
#include "ProfessionManager.h"
#include "GatheringManager.h"
#include "../Core/Managers/GameSystemsManager.h"
#include "../Session/BotSession.h"
#include "../AI/BotAI.h"
#include "Player.h"
#include "Log.h"
#include "Map.h"
#include <algorithm>

namespace Playerbot
{

// ============================================================================
// STATIC MEMBER INITIALIZATION
// ============================================================================

std::unordered_map<ProfessionType, std::vector<FarmingZoneInfo>> FarmingCoordinator::_farmingZones;
bool FarmingCoordinator::_farmingZonesInitialized = false;
FarmingStatistics FarmingCoordinator::_globalStatistics;
std::atomic<uint32> FarmingCoordinator::_nextSessionId{1};

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

FarmingCoordinator::FarmingCoordinator(Player* bot)
    : _bot(bot)
{
    // Initialize shared zone database once (thread-safe with static flag)
    if (!_farmingZonesInitialized)
    {
        LoadFarmingZones();
        _farmingZonesInitialized = true;
        TC_LOG_INFO("playerbots", "FarmingCoordinator: Loaded farming zones for {} professions",
            _farmingZones.size());
    }
}

FarmingCoordinator::~FarmingCoordinator()
{
    // CRITICAL: Do NOT call _bot->GetName() in destructor!
    // During destruction, _bot may be in invalid state where GetName() returns
    // garbage data, causing ACCESS_VIOLATION or std::bad_alloc.
    // IsInWorld() guard is NOT reliable during destruction sequence.
}

// ============================================================================
// CORE FARMING COORDINATION
// ============================================================================

void FarmingCoordinator::Initialize()
{
    // Per-bot initialization (zones already loaded in constructor)
    // CRITICAL: Do NOT access _bot->GetName() here!
    // Initialize() is called from GameSystemsManager::Initialize() during BotAI
    // constructor, when bot's m_name string is not initialized yet. Accessing it
    // causes ACCESS_VIOLATION in std::string copy constructor.
    // Logging deferred to first Update() call when bot is fully in world.
}

void FarmingCoordinator::Update(::Player* /*player*/, uint32 diff)
{
    if (!_bot || !_enabled)
        return;

    uint32 currentTime = GameTime::GetGameTimeMS();

    // Check if bot has active farming session
    if (_activeSession.isActive)
    {
        UpdateFarmingSession(diff);
        return;
    }

    // Check if enough time passed since last check
    static uint32 lastCheckTime = 0;
    if (currentTime - lastCheckTime < FARMING_CHECK_INTERVAL)
        return;

    lastCheckTime = currentTime;

    // Check if any profession needs farming
    auto professionsNeedingFarm = GetProfessionsNeedingFarm();
    if (!professionsNeedingFarm.empty())
    {
        // Start farming session for highest priority profession
        ProfessionType profession = professionsNeedingFarm.front();
        TC_LOG_INFO("playerbots", "FarmingCoordinator: Bot {} needs farming for profession {}",
            _bot->GetName(), static_cast<uint16>(profession));

        StartFarmingSession(profession, FarmingSessionType::SKILL_CATCHUP);
    }
}

void FarmingCoordinator::SetEnabled(bool enabled)
{
    _enabled = enabled;
}

bool FarmingCoordinator::IsEnabled() const
{
    return _enabled;
}

void FarmingCoordinator::SetCoordinatorProfile(FarmingCoordinatorProfile const& profile)
{
    _profile = profile;
}

FarmingCoordinatorProfile FarmingCoordinator::GetCoordinatorProfile() const
{
    return _profile;
}

// ============================================================================
// SKILL ANALYSIS
// ============================================================================

bool FarmingCoordinator::NeedsFarming(ProfessionType profession) const
{
    if (!_bot)
        return false;

    int32 skillGap = GetSkillGap(profession);
    return skillGap > static_cast<int32>(_profile.skillGapThreshold);
}

int32 FarmingCoordinator::GetSkillGap(ProfessionType profession) const
{
    if (!_bot)
        return 0;

    ProfessionManager* profMgr = const_cast<FarmingCoordinator*>(this)->GetProfessionManager();
    if (!profMgr)
        return 0;

    uint16 currentSkill = profMgr->GetProfessionSkill(profession);
    uint16 targetSkill = GetTargetSkillLevel(profession);

    return static_cast<int32>(targetSkill) - static_cast<int32>(currentSkill);
}

uint16 FarmingCoordinator::GetTargetSkillLevel(ProfessionType profession) const
{
    if (!_bot)
        return 0;

    uint16 charLevel = _bot->GetLevel();
    // Target = Character Level × skillLevelMultiplier (default: 5.0)
    return static_cast<uint16>(charLevel * _profile.skillLevelMultiplier);
}

std::vector<ProfessionType> FarmingCoordinator::GetProfessionsNeedingFarm() const
{
    std::vector<ProfessionType> professions;

    if (!_bot)
        return professions;

    ProfessionManager* profMgr = const_cast<FarmingCoordinator*>(this)->GetProfessionManager();
    if (!profMgr)
        return professions;

    // Get all player professions
    auto playerProfessions = profMgr->GetPlayerProfessions();

    // Check each profession for skill gap
    for (auto const& profInfo : playerProfessions)
    {
        if (NeedsFarming(profInfo.profession))
        {
            professions.push_back(profInfo.profession);
        }
    }

    // Sort by skill gap (largest gap first)
    std::sort(professions.begin(), professions.end(),
        [this](ProfessionType a, ProfessionType b)
        {
            return GetSkillGap(a) > GetSkillGap(b);
        });

    return professions;
}

uint32 FarmingCoordinator::CalculateFarmingDuration(ProfessionType profession) const
{
    if (!_bot)
        return 0;

    int32 skillGap = GetSkillGap(profession);
    if (skillGap <= 0)
        return 0;

    // Estimate: ~10 skill points per 5 minutes of farming
    uint32 estimatedDuration = (skillGap / 10) * 300000; // 5 minutes in ms

    // Clamp to min/max
    estimatedDuration = std::max(estimatedDuration, _profile.minFarmingDuration);
    estimatedDuration = std::min(estimatedDuration, _profile.maxFarmingDuration);

    return estimatedDuration;
}

// ============================================================================
// FARMING SESSION MANAGEMENT
// ============================================================================

bool FarmingCoordinator::StartFarmingSession(ProfessionType profession, FarmingSessionType sessionType)
{
    if (!_bot || !CanStartFarming())
        return false;

    // Check if session already active
    if (_activeSession.isActive)
    {
        TC_LOG_DEBUG("playerbots", "FarmingCoordinator: Bot {} already has active farming session",
            _bot->GetName());
        return false;
    }

    // Get optimal farming zone
    FarmingZoneInfo const* zone = GetOptimalFarmingZone(profession);
    if (!zone)
    {
        TC_LOG_ERROR("playerbots", "FarmingCoordinator: No suitable farming zone found for profession {}",
            static_cast<uint16>(profession));
        return false;
    }

    ProfessionManager* profMgr = GetProfessionManager();
    if (!profMgr)
        return false;

    // Create farming session
    _activeSession.sessionId = GenerateSessionId();
    _activeSession.playerGuid = _bot->GetGUID().GetCounter();
    _activeSession.sessionType = sessionType;
    _activeSession.profession = profession;
    _activeSession.zone = *zone;
    _activeSession.startTime = GameTime::GetGameTimeMS();
    _activeSession.duration = CalculateFarmingDuration(profession);
    _activeSession.startingSkill = profMgr->GetProfessionSkill(profession);
    _activeSession.targetSkill = GetTargetSkillLevel(profession);
    _activeSession.nodesGathered = 0;
    _activeSession.materialsCollected = 0;
    _activeSession.isActive = true;
    _activeSession.originalPosition = _bot->GetPosition();

    // Travel to farming zone
    if (!TravelToFarmingZone(*zone))
    {
        TC_LOG_ERROR("playerbots", "FarmingCoordinator: Failed to travel to farming zone");
        _activeSession = FarmingSession(); // Reset session
        return false;
    }

    TC_LOG_INFO("playerbots", "FarmingCoordinator: Started farming session {} for bot {} (skill {} -> {})",
        _activeSession.sessionId, _bot->GetName(), _activeSession.startingSkill, _activeSession.targetSkill);

    return true;
}

void FarmingCoordinator::StopFarmingSession()
{
    if (!_bot || !_activeSession.isActive)
        return;

    _activeSession.isActive = false;

    // Update statistics
    uint32 sessionTime = GameTime::GetGameTimeMS() - _activeSession.startTime;
    _statistics.sessionsCompleted++;
    _statistics.totalTimeSpent += sessionTime;
    _statistics.totalNodesGathered += _activeSession.nodesGathered;
    _statistics.zonesVisited++;

    _globalStatistics.sessionsCompleted++;
    _globalStatistics.totalTimeSpent += sessionTime;
    _globalStatistics.totalNodesGathered += _activeSession.nodesGathered;

    // Return to original position
    if (_profile.returnToOriginalPosition)
        ReturnToOriginalPosition(_activeSession);

    // Record last farming time for cooldown
    _lastFarmingTime = GameTime::GetGameTimeMS();

    TC_LOG_INFO("playerbots", "FarmingCoordinator: Stopped farming session {} for bot {} (gathered {} nodes)",
        _activeSession.sessionId, _bot->GetName(), _activeSession.nodesGathered);

    // Reset session
    _activeSession = FarmingSession();
}

FarmingSession const* FarmingCoordinator::GetActiveFarmingSession() const
{
    if (_activeSession.isActive)
        return &_activeSession;

    return nullptr;
}

bool FarmingCoordinator::HasActiveFarmingSession() const
{
    return _activeSession.isActive;
}

void FarmingCoordinator::UpdateFarmingSession(uint32 /*diff*/)
{
    if (!_bot || !_activeSession.isActive)
        return;

    // Check if session should end
    if (ShouldEndFarmingSession(_activeSession))
    {
        StopFarmingSession();
        return;
    }

    // Gathering is now handled by GatheringManager via BotAI::UpdateManagers()
    // The manager is updated automatically for all bots through the BehaviorManager system
    // No need to call it here

    // Update session progress (gather stats from GatheringAutomation)
    // In full implementation, track nodes gathered during this session
}

bool FarmingCoordinator::ShouldEndFarmingSession(FarmingSession const& session) const
{
    if (!_bot)
        return true;

    ProfessionManager* profMgr = const_cast<FarmingCoordinator*>(this)->GetProfessionManager();
    if (!profMgr)
        return true;

    // Check if skill target reached
    uint16 currentSkill = profMgr->GetProfessionSkill(session.profession);
    if (currentSkill >= session.targetSkill)
    {
        TC_LOG_INFO("playerbots", "FarmingCoordinator: Skill target reached ({} >= {})",
            currentSkill, session.targetSkill);
        return true;
    }

    // Check if duration exceeded
    uint32 sessionTime = GameTime::GetGameTimeMS() - session.startTime;
    if (sessionTime >= session.duration)
    {
        TC_LOG_INFO("playerbots", "FarmingCoordinator: Session duration exceeded ({} ms)", sessionTime);
        return true;
    }

    // Check if bot is in combat, dead, etc.
    if (_bot->IsInCombat() || !_bot->IsAlive())
        return true;

    return false;
}

// ============================================================================
// ZONE SELECTION
// ============================================================================

FarmingZoneInfo const* FarmingCoordinator::GetOptimalFarmingZone(ProfessionType profession) const
{
    if (!_bot)
        return nullptr;

    auto zones = GetSuitableZones(profession);
    if (zones.empty())
        return nullptr;

    // Find best zone based on score
    FarmingZoneInfo const* bestZone = nullptr;
    float bestScore = 0.0f;

    for (auto const& zone : zones)
    {
        float score = CalculateZoneScore(zone);
        if (score > bestScore)
        {
            bestScore = score;
            bestZone = &zone;
        }
    }

    return bestZone;
}

std::vector<FarmingZoneInfo> FarmingCoordinator::GetSuitableZones(ProfessionType profession) const
{
    std::vector<FarmingZoneInfo> suitable;

    if (!_bot)
        return suitable;

    auto it = _farmingZones.find(profession);
    if (it == _farmingZones.end())
        return suitable;

    ProfessionManager* profMgr = const_cast<FarmingCoordinator*>(this)->GetProfessionManager();
    if (!profMgr)
        return suitable;

    uint16 skillLevel = profMgr->GetProfessionSkill(profession);
    uint8 charLevel = _bot->GetLevel();

    // Filter zones by skill level and character level
    for (auto const& zone : it->second)
    {
        if (skillLevel >= zone.minSkillLevel && skillLevel <= zone.maxSkillLevel)
        {
            // Prefer zones near character level
            if (charLevel >= zone.recommendedCharLevel - 5 && charLevel <= zone.recommendedCharLevel + 10)
                suitable.push_back(zone);
        }
    }

    return suitable;
}

float FarmingCoordinator::CalculateZoneScore(FarmingZoneInfo const& zone) const
{
    if (!_bot)
        return 0.0f;

    float score = 100.0f;

    // Distance penalty (closer is better)
    float distance = _bot->GetDistance(zone.centerPosition);
    float distancePenalty = distance / 1000.0f; // Penalty per 1000 yards
    score -= distancePenalty * 10.0f;

    // Level match bonus
    uint8 charLevel = _bot->GetLevel();
    int8 levelDiff = std::abs(static_cast<int8>(charLevel) - static_cast<int8>(zone.recommendedCharLevel));
    if (levelDiff == 0)
        score += 20.0f;
    else
        score -= levelDiff * 2.0f;

    // PvP zone penalty
    if (zone.isContested)
        score -= 15.0f;

    return std::max(score, 0.0f);
}

// ============================================================================
// MATERIAL MANAGEMENT
// ============================================================================

bool FarmingCoordinator::HasReachedStockpileTarget(uint32 itemId) const
{
    uint32 currentCount = GetMaterialCount(itemId);
    return currentCount >= _profile.materialStockpileTarget;
}

uint32 FarmingCoordinator::GetMaterialCount(uint32 itemId) const
{
    if (!_bot)
        return 0;

    return _bot->GetItemCount(itemId);
}

std::vector<std::pair<uint32, uint32>> FarmingCoordinator::GetNeededMaterials(ProfessionType profession) const
{
    std::vector<std::pair<uint32, uint32>> materials;

    if (!_bot)
        return materials;

    // In full implementation, query auction house targets from ProfessionAuctionBridge
    // For now, return empty list

    return materials;
}

// ============================================================================
// STATISTICS
// ============================================================================

FarmingStatistics const& FarmingCoordinator::GetStatistics() const
{
    return _statistics;
}

FarmingStatistics const& FarmingCoordinator::GetGlobalStatistics()
{
    return _globalStatistics;
}

void FarmingCoordinator::ResetStatistics()
{
    _statistics.Reset();
}

// ============================================================================
// INITIALIZATION HELPERS
// ============================================================================

void FarmingCoordinator::LoadFarmingZones()
{
    InitializeMiningZones();
    InitializeHerbalismZones();
    InitializeSkinningZones();
}

void FarmingCoordinator::InitializeZoneDatabase()
{
    // Zones are initialized in profession-specific methods
    TC_LOG_INFO("playerbots", "FarmingCoordinator: Zone database initialization complete");
}

void FarmingCoordinator::InitializeMiningZones()
{
    std::vector<FarmingZoneInfo> miningZones;
    // Elwynn Forest (Alliance - Copper 1-75)
    FarmingZoneInfo elwynn;
    elwynn.zoneId = 12;
    elwynn.zoneName = "Elwynn Forest";
    elwynn.minSkillLevel = 1;
    elwynn.maxSkillLevel = 75;
    elwynn.profession = ProfessionType::MINING;
    elwynn.centerPosition.Relocate(-9449.0f, -1366.0f, 47.0f, 0.0f);
    elwynn.zoneRadius = 1000.0f;
    elwynn.recommendedCharLevel = 5;
    elwynn.isContested = false;
    miningZones.push_back(elwynn);
    // Westfall (Alliance - Copper/Tin 50-125)
    FarmingZoneInfo westfall;
    westfall.zoneId = 40;
    westfall.zoneName = "Westfall";
    westfall.minSkillLevel = 50;
    westfall.maxSkillLevel = 125;
    westfall.profession = ProfessionType::MINING;
    westfall.centerPosition.Relocate(-10684.0f, 1033.0f, 34.0f, 0.0f);
    westfall.zoneRadius = 1200.0f;
    westfall.recommendedCharLevel = 15;
    westfall.isContested = false;
    miningZones.push_back(westfall);

    _farmingZones[ProfessionType::MINING] = miningZones;
}

void FarmingCoordinator::InitializeHerbalismZones()
{
    std::vector<FarmingZoneInfo> herbalismZones;

    // Elwynn Forest (Alliance - Peacebloom/Silverleaf 1-75)
    FarmingZoneInfo elwynn;
    elwynn.zoneId = 12;
    elwynn.zoneName = "Elwynn Forest";
    elwynn.minSkillLevel = 1;
    elwynn.maxSkillLevel = 75;
    elwynn.profession = ProfessionType::HERBALISM;
    elwynn.centerPosition.Relocate(-9449.0f, -1366.0f, 47.0f, 0.0f);
    elwynn.zoneRadius = 1000.0f;
    elwynn.recommendedCharLevel = 5;
    elwynn.isContested = false;
    herbalismZones.push_back(elwynn);

    _farmingZones[ProfessionType::HERBALISM] = herbalismZones;
}

void FarmingCoordinator::InitializeSkinningZones()
{
    // Skinning works in any zone with beasts - no specific zone database needed
    TC_LOG_DEBUG("playerbots", "FarmingCoordinator: Skinning works in all zones with creatures");
}

// ============================================================================
// FARMING HELPERS
// ============================================================================

uint32 FarmingCoordinator::GenerateSessionId()
{
    return _nextSessionId.fetch_add(1);
}

bool FarmingCoordinator::TravelToFarmingZone(FarmingZoneInfo const& zone)
{
    if (!_bot)
        return false;

    // Simple movement to zone center
    // In full implementation, use proper pathfinding or teleport
    _bot->TeleportTo(_bot->GetMapId(), zone.centerPosition.GetPositionX(),
        zone.centerPosition.GetPositionY(), zone.centerPosition.GetPositionZ(),
        zone.centerPosition.GetOrientation());

    return true;
}

void FarmingCoordinator::ReturnToOriginalPosition(FarmingSession const& session)
{
    if (!_bot)
        return;

    _bot->TeleportTo(_bot->GetMapId(), session.originalPosition.GetPositionX(),
        session.originalPosition.GetPositionY(), session.originalPosition.GetPositionZ(),
        session.originalPosition.GetOrientation());
}

bool FarmingCoordinator::CanStartFarming() const
{
    if (!_bot)
        return false;

    // Check if in combat
    if (_bot->IsInCombat())
        return false;

    // Check if alive
    if (!_bot->IsAlive())
        return false;

    // Check if in group (optional - may want to farm while grouped)
    // if (_bot->GetGroup())
    //     return false;

    // Check farming cooldown
    if (_lastFarmingTime > 0)
    {
        uint32 timeSinceLastFarm = GameTime::GetGameTimeMS() - _lastFarmingTime;
        if (timeSinceLastFarm < _profile.farmingCooldown)
            return false;
    }

    return true;
}

bool FarmingCoordinator::ValidateFarmingSession(FarmingSession const& session) const
{
    if (!_bot || !session.isActive)
        return false;

    ProfessionManager* profMgr = const_cast<FarmingCoordinator*>(this)->GetProfessionManager();
    if (!profMgr)
        return false;

    // Validate bot still has the profession
    if (!profMgr->HasProfession(session.profession))
        return false;

    return true;
}

// ============================================================================
// INTEGRATION HELPERS
// ============================================================================

ProfessionManager* FarmingCoordinator::GetProfessionManager()
{
    if (!_bot)
        return nullptr;

    BotSession* session = static_cast<BotSession*>(_bot->GetSession());
    if (!session || !session->GetAI())
        return nullptr;

    return session->GetAI()->GetGameSystems()->GetProfessionManager();
}

GatheringManager* FarmingCoordinator::GetGatheringManager()
{
    if (!_bot)
        return nullptr;

    BotSession* session = static_cast<BotSession*>(_bot->GetSession());
    if (!session || !session->GetAI())
        return nullptr;

    return session->GetAI()->GetGameSystems()->GetGatheringManager();
}

} // namespace Playerbot
