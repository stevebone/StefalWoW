/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * ArchaeologyManager - Bot archaeology profession automation
 *
 * Manages the full archaeology profession loop for bots:
 * 1. Travel to dig sites
 * 2. Use Survey to locate artifacts
 * 3. Collect fragments
 * 4. Solve artifacts when enough fragments collected
 */

#ifndef TRINITYCORE_BOT_ARCHAEOLOGY_MANAGER_H
#define TRINITYCORE_BOT_ARCHAEOLOGY_MANAGER_H

#include "Define.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <shared_mutex>

class Player;

namespace Playerbot
{

/**
 * @enum ArchaeologyState
 * @brief Current state in the archaeology activity loop
 */
enum class ArchaeologyState : uint8
{
    IDLE            = 0,    // Not doing archaeology
    TRAVELING       = 1,    // Moving to a dig site
    SURVEYING       = 2,    // Using Survey at a dig site
    MOVING_TO_FIND  = 3,    // Moving toward indicated artifact location
    COLLECTING      = 4,    // Looting artifact fragment
    SOLVING         = 5,    // Solving an artifact at a research bench
    MAX
};

/**
 * @enum SurveyIndicator
 * @brief Survey scope result colors (distance to artifact)
 */
enum class SurveyIndicator : uint8
{
    RED     = 0,    // Far away (>80 yards)
    YELLOW  = 1,    // Medium distance (40-80 yards)
    GREEN   = 2,    // Close (<40 yards)
    FOUND   = 3     // On top of the artifact
};

/**
 * @struct DigSiteInfo
 * @brief Information about a dig site
 */
struct DigSiteInfo
{
    uint32 siteId;
    uint32 mapId;
    Position center;
    float radius;
    uint16 raceId;          // Archaeology race (Dwarf, Night Elf, etc.)
    std::string name;
    uint8 remainingFinds;   // 0-3 finds per site before it moves

    bool IsExhausted() const { return remainingFinds == 0; }
};

/**
 * @struct ArchaeologyRaceProgress
 * @brief Fragment and artifact progress for an archaeology race
 */
struct ArchaeologyRaceProgress
{
    uint16 raceId;
    std::string raceName;
    uint32 fragments;           // Current fragment count
    uint32 keystoneCount;       // Number of keystones
    uint32 artifactsSolved;     // Total solved artifacts
    uint32 currentProjectId;    // Current research project
    uint32 fragmentsNeeded;     // Fragments needed for current project
};

/**
 * @class ArchaeologyManager
 * @brief Manages archaeology profession for a single bot
 *
 * Automates the full archaeology loop:
 * - Detect available dig sites on the world map
 * - Travel to nearest dig site
 * - Use Survey spell to triangulate artifact location
 * - Collect fragments
 * - Solve artifacts when enough fragments collected
 *
 * Key spell IDs:
 * - Survey: 80451
 * - Archaeology passive: 78670
 */
class TC_GAME_API ArchaeologyManager
{
public:
    explicit ArchaeologyManager(Player* bot);
    ~ArchaeologyManager() = default;

    // Spell IDs
    static constexpr uint32 SPELL_SURVEY = 80451;
    static constexpr uint32 SPELL_ARCHAEOLOGY = 78670;

    /**
     * @brief Check if bot has archaeology profession
     */
    bool HasArchaeology() const;

    /**
     * @brief Get current archaeology skill level
     */
    uint32 GetSkillLevel() const;

    /**
     * @brief Get current state in the archaeology loop
     */
    ArchaeologyState GetState() const { return _state; }

    /**
     * @brief Start an archaeology session
     *
     * Begins the archaeology loop: find dig site -> travel -> survey -> collect.
     * @return true if session started (has profession, dig sites available)
     */
    bool StartSession();

    /**
     * @brief Stop the current archaeology session
     */
    void StopSession();

    /**
     * @brief Update archaeology activity
     *
     * Main loop: handles state transitions based on current activity.
     *
     * @param diff Time since last update in milliseconds
     */
    void Update(uint32 diff);

    /**
     * @brief Get available dig sites for the bot's current continent
     */
    std::vector<DigSiteInfo> GetAvailableDigSites() const;

    /**
     * @brief Get nearest dig site
     */
    DigSiteInfo GetNearestDigSite() const;

    /**
     * @brief Check if bot is at a dig site
     */
    bool IsAtDigSite() const;

    /**
     * @brief Get progress for all archaeology races
     */
    std::vector<ArchaeologyRaceProgress> GetRaceProgress() const;

    /**
     * @brief Check if any artifact can be solved
     */
    bool CanSolveArtifact() const;

    /**
     * @brief Attempt to solve the current artifact for a race
     *
     * @param raceId Archaeology race ID
     * @return true if artifact was solved
     */
    bool SolveArtifact(uint16 raceId);

    /**
     * @brief Get total artifacts solved
     */
    uint32 GetTotalArtifactsSolved() const { return _totalSolved; }

    /**
     * @brief Check if archaeology session is active
     */
    bool IsActive() const { return _state != ArchaeologyState::IDLE; }

private:
    Player* _bot;
    ArchaeologyState _state = ArchaeologyState::IDLE;

    // Current activity tracking
    DigSiteInfo _currentSite;
    Position _surveyTarget;             // Estimated artifact position
    uint8 _surveyAttempts = 0;          // Number of surveys at current site
    uint32 _lastSurveyTimeMs = 0;
    uint32 _lastStateChangeMs = 0;
    uint32 _totalSolved = 0;

    // Timing
    static constexpr uint32 SURVEY_COOLDOWN_MS = 3000;      // 3 seconds between surveys
    static constexpr uint32 TRAVEL_TIMEOUT_MS = 120000;      // 2 minutes max travel
    static constexpr uint32 STATE_TIMEOUT_MS = 30000;        // 30 second timeout per state
    static constexpr uint8 MAX_SURVEYS_PER_FIND = 10;        // Safety limit
    static constexpr uint8 FINDS_PER_SITE = 3;              // Standard 3 finds per dig site

    /**
     * @brief Handle TRAVELING state - move to dig site
     */
    void HandleTraveling(uint32 diff);

    /**
     * @brief Handle SURVEYING state - use Survey spell
     */
    void HandleSurveying(uint32 diff);

    /**
     * @brief Handle MOVING_TO_FIND state - move toward artifact
     */
    void HandleMovingToFind(uint32 diff);

    /**
     * @brief Handle COLLECTING state - pick up fragment
     */
    void HandleCollecting(uint32 diff);

    /**
     * @brief Handle SOLVING state - solve artifact
     */
    void HandleSolving(uint32 diff);

    /**
     * @brief Transition to a new state
     */
    void SetState(ArchaeologyState newState);

    /**
     * @brief Interpret Survey result (telescope direction + color)
     *
     * @return Estimated position of the artifact
     */
    Position InterpretSurveyResult() const;

    /**
     * @brief Select the best dig site to visit
     */
    DigSiteInfo SelectBestDigSite() const;
};

/**
 * @class ArchaeologyCoordinator
 * @brief Singleton coordinator for bot archaeology
 */
class TC_GAME_API ArchaeologyCoordinator final
{
public:
    static ArchaeologyCoordinator* instance()
    {
        static ArchaeologyCoordinator inst;
        return &inst;
    }

    ArchaeologyManager* GetManager(Player* bot);
    void RemoveManager(ObjectGuid botGuid);

private:
    ArchaeologyCoordinator() = default;
    ~ArchaeologyCoordinator() = default;
    ArchaeologyCoordinator(const ArchaeologyCoordinator&) = delete;
    ArchaeologyCoordinator& operator=(const ArchaeologyCoordinator&) = delete;

    mutable std::shared_mutex _mutex;
    std::unordered_map<ObjectGuid, std::unique_ptr<ArchaeologyManager>> _managers;
};

#define sArchaeologyCoordinator ArchaeologyCoordinator::instance()

} // namespace Playerbot

#endif // TRINITYCORE_BOT_ARCHAEOLOGY_MANAGER_H
