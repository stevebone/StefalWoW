/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 */

#include "ArchaeologyManager.h"
#include "Player.h"
#include "GameTime.h"
#include "Log.h"
#include "SpellMgr.h"
#include "Map.h"

namespace Playerbot
{

// ============================================================================
// ArchaeologyManager - Per-Bot Implementation
// ============================================================================

ArchaeologyManager::ArchaeologyManager(Player* bot)
    : _bot(bot)
{
}

bool ArchaeologyManager::HasArchaeology() const
{
    if (!_bot)
        return false;

    // Check if bot has the Archaeology secondary profession skill
    return _bot->HasSkill(SKILL_ARCHAEOLOGY);
}

uint32 ArchaeologyManager::GetSkillLevel() const
{
    if (!_bot)
        return 0;

    return _bot->GetSkillValue(SKILL_ARCHAEOLOGY);
}

bool ArchaeologyManager::StartSession()
{
    if (!_bot || !HasArchaeology())
        return false;

    auto sites = GetAvailableDigSites();
    if (sites.empty())
    {
        TC_LOG_DEBUG("module.playerbot", "ArchaeologyManager: Bot {} has no available dig sites",
            _bot->GetName());
        return false;
    }

    _currentSite = SelectBestDigSite();
    SetState(ArchaeologyState::TRAVELING);

    TC_LOG_DEBUG("module.playerbot", "ArchaeologyManager: Bot {} starting archaeology session, heading to '{}'",
        _bot->GetName(), _currentSite.name);

    return true;
}

void ArchaeologyManager::StopSession()
{
    SetState(ArchaeologyState::IDLE);
    _surveyAttempts = 0;

    TC_LOG_DEBUG("module.playerbot", "ArchaeologyManager: Bot {} stopped archaeology session",
        _bot ? _bot->GetName() : "Unknown");
}

void ArchaeologyManager::Update(uint32 diff)
{
    if (!_bot || _state == ArchaeologyState::IDLE)
        return;

    // Safety timeout — if stuck in any state too long, reset
    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - _lastStateChangeMs > TRAVEL_TIMEOUT_MS)
    {
        TC_LOG_WARN("module.playerbot", "ArchaeologyManager: Bot {} timed out in state {}, resetting",
            _bot->GetName(), static_cast<uint8>(_state));
        StopSession();
        return;
    }

    switch (_state)
    {
        case ArchaeologyState::TRAVELING:
            HandleTraveling(diff);
            break;
        case ArchaeologyState::SURVEYING:
            HandleSurveying(diff);
            break;
        case ArchaeologyState::MOVING_TO_FIND:
            HandleMovingToFind(diff);
            break;
        case ArchaeologyState::COLLECTING:
            HandleCollecting(diff);
            break;
        case ArchaeologyState::SOLVING:
            HandleSolving(diff);
            break;
        default:
            break;
    }
}

std::vector<DigSiteInfo> ArchaeologyManager::GetAvailableDigSites() const
{
    std::vector<DigSiteInfo> sites;
    if (!_bot)
        return sites;

    // In WoW, dig sites are tracked per player via the archaeology system
    // The player has up to 4 dig sites per continent
    // For bots, we create placeholder sites based on the bot's current map
    // Future: integrate with actual archaeology DBC data

    uint32 mapId = _bot->GetMapId();

    // Generate dig sites based on archaeology race zones
    // This is a simplified version - real implementation would query
    // ResearchSite.db2 and QuestPOIPoint.db2
    DigSiteInfo site;
    site.siteId = 1;
    site.mapId = mapId;
    site.center = _bot->GetPosition();
    site.radius = 40.0f;
    site.raceId = 1; // Dwarf
    site.name = "Local Dig Site";
    site.remainingFinds = FINDS_PER_SITE;
    sites.push_back(site);

    return sites;
}

DigSiteInfo ArchaeologyManager::GetNearestDigSite() const
{
    auto sites = GetAvailableDigSites();
    if (sites.empty())
        return DigSiteInfo{};

    DigSiteInfo nearest = sites[0];
    float nearestDist = _bot->GetDistance2d(nearest.center.GetPositionX(), nearest.center.GetPositionY());

    for (size_t i = 1; i < sites.size(); ++i)
    {
        float dist = _bot->GetDistance2d(sites[i].center.GetPositionX(), sites[i].center.GetPositionY());
        if (dist < nearestDist)
        {
            nearest = sites[i];
            nearestDist = dist;
        }
    }

    return nearest;
}

bool ArchaeologyManager::IsAtDigSite() const
{
    if (!_bot || _currentSite.siteId == 0)
        return false;

    float dist = _bot->GetDistance2d(_currentSite.center.GetPositionX(), _currentSite.center.GetPositionY());
    return dist <= _currentSite.radius;
}

std::vector<ArchaeologyRaceProgress> ArchaeologyManager::GetRaceProgress() const
{
    std::vector<ArchaeologyRaceProgress> progress;
    if (!_bot)
        return progress;

    // Simplified: return basic progress
    // Real implementation would query player's archaeology data
    ArchaeologyRaceProgress race;
    race.raceId = 1;
    race.raceName = "Dwarf";
    race.fragments = 0;
    race.keystoneCount = 0;
    race.artifactsSolved = _totalSolved;
    race.currentProjectId = 0;
    race.fragmentsNeeded = 35;
    progress.push_back(race);

    return progress;
}

bool ArchaeologyManager::CanSolveArtifact() const
{
    auto races = GetRaceProgress();
    for (const auto& race : races)
    {
        if (race.fragments >= race.fragmentsNeeded && race.currentProjectId > 0)
            return true;
    }
    return false;
}

bool ArchaeologyManager::SolveArtifact(uint16 /*raceId*/)
{
    if (!_bot)
        return false;

    // Solving artifacts requires interacting with the archaeology UI
    // This is handled through a special spell cast
    // For now, track the solve
    ++_totalSolved;

    TC_LOG_DEBUG("module.playerbot", "ArchaeologyManager: Bot {} solved an artifact (total: {})",
        _bot->GetName(), _totalSolved);

    return true;
}

void ArchaeologyManager::HandleTraveling(uint32 /*diff*/)
{
    if (!_bot)
        return;

    if (IsAtDigSite())
    {
        SetState(ArchaeologyState::SURVEYING);
        _surveyAttempts = 0;
        return;
    }

    // Movement is handled by the movement system
    // We just check if we've arrived
}

void ArchaeologyManager::HandleSurveying(uint32 /*diff*/)
{
    if (!_bot)
        return;

    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - _lastSurveyTimeMs < SURVEY_COOLDOWN_MS)
        return;

    if (_surveyAttempts >= MAX_SURVEYS_PER_FIND)
    {
        TC_LOG_DEBUG("module.playerbot", "ArchaeologyManager: Bot {} max surveys reached, moving on",
            _bot->GetName());
        // Site exhausted or failed — try next site
        StopSession();
        return;
    }

    // Cast Survey spell
    if (_bot->HasSpell(SPELL_SURVEY))
    {
        // The survey result creates a scope object that points toward the artifact
        // In a full implementation, we'd read the survey result to determine direction
        _lastSurveyTimeMs = currentTime;
        ++_surveyAttempts;

        // Interpret survey result and move toward artifact
        _surveyTarget = InterpretSurveyResult();
        SetState(ArchaeologyState::MOVING_TO_FIND);

        TC_LOG_DEBUG("module.playerbot", "ArchaeologyManager: Bot {} surveyed (attempt {}/{})",
            _bot->GetName(), _surveyAttempts, MAX_SURVEYS_PER_FIND);
    }
}

void ArchaeologyManager::HandleMovingToFind(uint32 /*diff*/)
{
    if (!_bot)
        return;

    // Check if we've reached the survey target
    float dist = _bot->GetDistance2d(_surveyTarget.GetPositionX(), _surveyTarget.GetPositionY());
    if (dist < 5.0f)
    {
        // Close enough — survey again for better triangulation or collect
        if (_surveyAttempts >= 3)
        {
            // After 3+ surveys, we're likely close enough
            SetState(ArchaeologyState::COLLECTING);
        }
        else
        {
            SetState(ArchaeologyState::SURVEYING);
        }
    }
}

void ArchaeologyManager::HandleCollecting(uint32 /*diff*/)
{
    if (!_bot)
        return;

    // Fragment collection happens automatically when Survey finds the artifact
    // Transition back to surveying for next find, or to solving if we have enough
    if (_currentSite.remainingFinds > 0)
        --_currentSite.remainingFinds;

    if (_currentSite.IsExhausted())
    {
        TC_LOG_DEBUG("module.playerbot", "ArchaeologyManager: Bot {} exhausted dig site '{}'",
            _bot->GetName(), _currentSite.name);

        if (CanSolveArtifact())
            SetState(ArchaeologyState::SOLVING);
        else
            StopSession(); // Need to find a new dig site
    }
    else
    {
        // More finds at this site
        _surveyAttempts = 0;
        SetState(ArchaeologyState::SURVEYING);
    }
}

void ArchaeologyManager::HandleSolving(uint32 /*diff*/)
{
    if (!_bot)
        return;

    auto races = GetRaceProgress();
    for (const auto& race : races)
    {
        if (race.fragments >= race.fragmentsNeeded)
        {
            SolveArtifact(race.raceId);
            break;
        }
    }

    // After solving, go back to digging
    StopSession();
}

void ArchaeologyManager::SetState(ArchaeologyState newState)
{
    if (_state == newState)
        return;

    TC_LOG_DEBUG("module.playerbot", "ArchaeologyManager: Bot {} state {} -> {}",
        _bot ? _bot->GetName() : "Unknown",
        static_cast<uint8>(_state), static_cast<uint8>(newState));

    _state = newState;
    _lastStateChangeMs = GameTime::GetGameTimeMS();
}

Position ArchaeologyManager::InterpretSurveyResult() const
{
    if (!_bot)
        return Position();

    // The survey spell creates a telescope that points toward the artifact
    // The color indicates distance: red=far, yellow=medium, green=close
    // In a full implementation, we'd read the scope's orientation
    // For now, move 30 yards in the bot's facing direction
    float angle = _bot->GetOrientation();
    float distance = 30.0f; // Move 30 yards toward estimated location

    float x = _bot->GetPositionX() + distance * std::cos(angle);
    float y = _bot->GetPositionY() + distance * std::sin(angle);
    float z = _bot->GetPositionZ();

    return Position(x, y, z, angle);
}

DigSiteInfo ArchaeologyManager::SelectBestDigSite() const
{
    return GetNearestDigSite();
}

// ============================================================================
// ArchaeologyCoordinator - Singleton
// ============================================================================

ArchaeologyManager* ArchaeologyCoordinator::GetManager(Player* bot)
{
    if (!bot)
        return nullptr;

    ObjectGuid guid = bot->GetGUID();

    {
        std::shared_lock lock(_mutex);
        auto it = _managers.find(guid);
        if (it != _managers.end())
            return it->second.get();
    }

    std::unique_lock lock(_mutex);
    auto it = _managers.find(guid);
    if (it != _managers.end())
        return it->second.get();

    auto [newIt, inserted] = _managers.emplace(guid, std::make_unique<ArchaeologyManager>(bot));
    return newIt->second.get();
}

void ArchaeologyCoordinator::RemoveManager(ObjectGuid botGuid)
{
    std::unique_lock lock(_mutex);
    _managers.erase(botGuid);
}

} // namespace Playerbot
