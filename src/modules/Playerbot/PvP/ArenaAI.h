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
#include "Position.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>

namespace Playerbot
{

/**
 * @brief Arena bracket types
 */
enum class ArenaBracket : uint8
{
    BRACKET_2v2 = 0,
    BRACKET_3v3 = 1,
    BRACKET_5v5 = 2
};

/**
 * @brief Arena strategy types
 */
enum class ArenaStrategy : uint8
{
    KILL_HEALER_FIRST = 0,     // Focus healer
    KILL_DPS_FIRST = 1,        // Focus DPS
    KILL_LOWEST_HEALTH = 2,    // Focus lowest health target
    SPREAD_PRESSURE = 3,       // Spread damage across targets
    TRAIN_ONE_TARGET = 4,      // Focus single target until death
    ADAPTIVE = 5               // Adapt based on situation
};

/**
 * @brief Arena positioning strategy
 */
enum class PositioningStrategy : uint8
{
    AGGRESSIVE = 0,            // Push forward
    DEFENSIVE = 1,             // Stay back, kite
    PILLAR_KITE = 2,           // Use pillars for LoS
    SPREAD_OUT = 3,            // Spread to avoid AoE
    GROUP_UP = 4               // Stay together
};

/**
 * @brief Arena team composition types
 */
enum class TeamComposition : uint8
{
    DOUBLE_DPS = 0,            // 2v2: Two DPS
    DPS_HEALER = 1,            // 2v2: DPS + Healer
    TRIPLE_DPS = 2,            // 3v3: Three DPS (Training comp)
    DOUBLE_DPS_HEALER = 3,     // 3v3: Two DPS + Healer (Standard)
    TANK_DPS_HEALER = 4        // 3v3: Tank + DPS + Healer
};

/**
 * @brief Arena pillar/LoS object
 */
struct ArenaPillar
{
    Position position;
    float radius;
    bool isAvailable;

    ArenaPillar() : radius(5.0f), isAvailable(true) {}
    ArenaPillar(Position pos, float r) : position(pos), radius(r), isAvailable(true) {}
};

/**
 * @brief Arena match state
 */
struct ArenaMatchState
{
    uint32 matchStartTime;
    uint32 damageDealt;
    uint32 damageTaken;
    uint32 healingDone;
    uint32 ccLanded;
    uint32 defensivesUsed;
    uint32 offensivesUsed;
    bool isWinning;
    uint32 teammateAliveCount;
    uint32 enemyAliveCount;

    ArenaMatchState() : matchStartTime(0), damageDealt(0), damageTaken(0),
        healingDone(0), ccLanded(0), defensivesUsed(0), offensivesUsed(0),
        isWinning(false), teammateAliveCount(0), enemyAliveCount(0) {}
};

/**
 * @brief Arena profile configuration
 */
struct ArenaProfile
{
    ArenaStrategy strategy = ArenaStrategy::ADAPTIVE;
    PositioningStrategy positioning = PositioningStrategy::PILLAR_KITE;
    bool usePillars = true;
    bool autoSwitch = true;           // Auto-switch targets
    bool prioritizeHealers = true;
    bool coordCC = true;              // Coordinate CC with teammates
    bool saveDefensivesForBurst = true;
    uint32 pillarKiteHealthThreshold = 40; // Start pillar kiting below %
    float maxDistanceFromTeam = 25.0f;

    ArenaProfile() = default;
};

/**
 * @brief Metrics for arena performance
 */
struct ArenaMetrics
{
    ::std::atomic<uint32> matchesPlayed{0};
    ::std::atomic<uint32> matchesWon{0};
    ::std::atomic<uint32> matchesLost{0};
    ::std::atomic<uint32> killsTotal{0};
    ::std::atomic<uint32> deathsTotal{0};
    ::std::atomic<uint64> damageDealt{0};
    ::std::atomic<uint64> damageTaken{0};
    ::std::atomic<uint64> healingDone{0};
    ::std::atomic<uint32> rating{1500};
    ::std::atomic<uint32> pillarKites{0};
    ::std::atomic<uint32> successfulBursts{0};
    ::std::atomic<uint32> coordCCs{0};

    ArenaMetrics() = default;
};

/**
 * @brief Arena AI - Complete arena automation
 *
 * **Phase 7.1: Per-Bot Instance Pattern (27th Manager)**
 *
 * Features:
 * - 2v2/3v3/5v5 bracket strategies
 * - Team composition analysis
 * - Pillar kiting and LoS mechanics
 * - Focus target coordination
 * - Positioning algorithms
 * - Composition-specific counters
 * - Adaptive strategy based on match state
 * - Performance optimized (per-bot isolation, zero mutex)
 *
 * **Ownership:**
 * - Owned by GameSystemsManager (27th manager)
 * - Each bot has independent arena state and strategy
 * - Shared arena map data across all bots (static)
 */
class TC_GAME_API ArenaAI final
{
public:
    // ============================================================================
    // INITIALIZATION
    // ============================================================================

    void Initialize();
    void Update(uint32 diff);

    /**
     * Called when arena match starts
     */
    void OnMatchStart();

    /**
     * Called when arena match ends
     */
    void OnMatchEnd(bool won);

    // ============================================================================
    // STRATEGY SELECTION
    // ============================================================================

    /**
     * Analyze team composition and select strategy
     */
    void AnalyzeTeamComposition();

    /**
     * Get recommended strategy for composition
     */
    ArenaStrategy GetStrategyForComposition(TeamComposition teamComp,
        TeamComposition enemyComp) const;

    /**
     * Adapt strategy based on match state
     */
    void AdaptStrategy();

    // ============================================================================
    // TARGET SELECTION
    // ============================================================================

    /**
     * Select focus target for arena
     */
    ::Unit* SelectFocusTarget() const;

    /**
     * Check if should switch target
     */
    bool ShouldSwitchTarget(::Unit* currentTarget) const;

    /**
     * Get kill target priority
     */
    std::vector<::Unit*> GetKillTargetPriority() const;

    // ============================================================================
    // POSITIONING
    // ============================================================================

    /**
     * Execute positioning strategy
     */
    void ExecutePositioning();

    /**
     * Find best pillar for kiting
     */
    ArenaPillar const* FindBestPillar() const;

    /**
     * Move to pillar for LoS
     */
    bool MoveToPillar(ArenaPillar const& pillar);

    /**
     * Check if using pillar effectively
     */
    bool IsUsingPillarEffectively() const;

    /**
     * Maintain optimal distance from enemies
     */
    bool MaintainOptimalDistance();

    /**
     * Regroup with teammates
     */
    bool RegroupWithTeam();

    // ============================================================================
    // PILLAR KITING
    // ============================================================================

    /**
     * Check if should pillar kite
     */
    bool ShouldPillarKite() const;

    /**
     * Execute pillar kite
     */
    bool ExecutePillarKite();

    /**
     * Break line of sight with pillar
     */
    bool BreakLoSWithPillar(::Unit* enemy);

    // ============================================================================
    // COOLDOWN COORDINATION
    // ============================================================================

    /**
     * Coordinate offensive burst with team
     */
    bool CoordinateOffensiveBurst();

    /**
     * Check if team is ready for burst
     */
    bool IsTeamReadyForBurst() const;

    /**
     * Signal team for burst
     */
    void SignalBurst();

    // ============================================================================
    // CC COORDINATION
    // ============================================================================

    /**
     * Coordinate CC chain with team
     */
    bool CoordinateCCChain(::Unit* target);

    /**
     * Check if teammate has CC available
     */
    bool TeammateHasCCAvailable() const;

    /**
     * Signal CC target to team
     */
    void SignalCCTarget(::Unit* target);

    // ============================================================================
    // COMP-SPECIFIC STRATEGIES
    // ============================================================================

    // 2v2 Strategies
    void Execute2v2Strategy();
    void Execute2v2DoubleDPS();
    void Execute2v2DPSHealer();

    // 3v3 Strategies
    void Execute3v3Strategy();
    void Execute3v3TripleDPS();
    void Execute3v3DoubleDPSHealer();
    void Execute3v3TankDPSHealer();

    // 5v5 Strategies
    void Execute5v5Strategy();

    // ============================================================================
    // COMPOSITION COUNTERS
    // ============================================================================

    /**
     * Get counter strategy for enemy composition
     */
    ArenaStrategy GetCounterStrategy(TeamComposition enemyComp) const;

    /**
     * Counter RMP (Rogue/Mage/Priest)
     */
    void CounterRMP();

    /**
     * Counter TSG (Warrior/DK/Healer)
     */
    void CounterTSG();

    /**
     * Counter Turbo Cleave (Warrior/Shaman/Healer)
     */
    void CounterTurboCleave();

    // ============================================================================
    // MATCH STATE TRACKING
    // ============================================================================

    ArenaMatchState GetMatchState() const;
    void UpdateMatchState();

    /**
     * Check if team is winning
     */
    bool IsTeamWinning() const;

    /**
     * Get match duration (seconds)
     */
    uint32 GetMatchDuration() const;

    // ============================================================================
    // PROFILES
    // ============================================================================

    void SetArenaProfile(ArenaProfile const& profile);
    ArenaProfile GetArenaProfile() const;

    // ============================================================================
    // METRICS
    // ============================================================================

    ArenaMetrics const& GetMetrics() const;
    ArenaMetrics const& GetGlobalMetrics() const;

public:
    /**
     * @brief Construct arena AI for specific bot
     * @param bot The bot player this manager serves
     */
    explicit ArenaAI(Player* bot);
    ~ArenaAI();

    // Non-copyable
    ArenaAI(ArenaAI const&) = delete;
    ArenaAI& operator=(ArenaAI const&) = delete;

private:
    // Bot reference (owned externally)
    Player* _bot;

    // Static shared data across all bot instances
    static std::unordered_map<uint32, std::vector<ArenaPillar>> _arenaMapPillars;
    static ArenaMetrics _globalMetrics;
    static bool _initialized;

    // Per-instance state
    uint32 _lastUpdateTime{0};
    ObjectGuid _focusTarget;
    ArenaMatchState _matchState;
    ArenaProfile _profile;
    ArenaMetrics _metrics;
    TeamComposition _teamComposition{TeamComposition::DOUBLE_DPS_HEALER};
    TeamComposition _enemyComposition{TeamComposition::DOUBLE_DPS_HEALER};
    bool _burstReady{false};


    // ============================================================================
    // HELPER FUNCTIONS
    // ============================================================================

    ArenaBracket GetArenaBracket() const;
    uint8 GetBracketTeamSize(ArenaBracket bracket) const;
    TeamComposition GetTeamComposition() const;
    TeamComposition GetEnemyTeamComposition() const;
    std::vector<::Player*> GetTeammates() const;
    std::vector<::Unit*> GetEnemyTeam() const;
    bool IsInLineOfSight(::Unit* target) const;
    float GetOptimalRangeForClass() const;
    bool IsTeammateInDanger(::Player* teammate) const;

    // ============================================================================
    // RATING SYSTEM HELPERS
    // ============================================================================

    /**
     * @brief Estimate opponent team's rating based on match state
     * @return Estimated opponent rating
     */
    uint32 EstimateOpponentRating() const;

    /**
     * @brief Record match result for performance tracking
     * @param won Whether the match was won
     * @param oldRating Rating before match
     * @param newRating Rating after match
     * @param opponentRating Estimated opponent rating
     * @param duration Match duration in seconds
     */
    void RecordMatchResult(bool won, uint32 oldRating, uint32 newRating,
        uint32 opponentRating, uint32 duration);

    // ============================================================================
    // TARGET ANALYSIS HELPERS
    // ============================================================================

    /**
     * @brief Calculate priority score for target selection
     * @param target The target to evaluate
     * @return Priority score (lower = higher priority)
     */
    float CalculateTargetPriorityScore(::Unit* target) const;

    /**
     * @brief Check if target is under pressure from teammates
     * @param target The target to check
     * @return True if target is being focused by team
     */
    bool IsTargetUnderTeamPressure(::Unit* target) const;

    /**
     * @brief Check if target has defensive cooldown active
     * @param target The target to check
     * @return True if target has major defensive active
     */
    bool HasDefensiveCooldownActive(::Unit* target) const;

    // ============================================================================
    // PILLAR DATABASE
    // ============================================================================

    void InitializePillarDatabase();
    void LoadArenaPillars(uint32 arenaMapId);

    // Blade's Edge Arena
    void LoadBladesEdgePillars();

    // Nagrand Arena
    void LoadNagrandPillars();

    // Ruins of Lordaeron
    void LoadLordaeronPillars();

    // Dalaran Arena
    void LoadDalaranPillars();

    // Ring of Valor
    void LoadRingOfValorPillars();

    // ============================================================================
    // DATA STRUCTURES
    // ============================================================================

    // Arena profiles
    std::unordered_map<uint32, ArenaProfile> _playerProfiles;

    // Match states
    std::unordered_map<uint32, ArenaMatchState> _matchStates;

    // Team compositions (playerGuid -> composition)
    std::unordered_map<uint32, TeamComposition> _teamCompositions;
    std::unordered_map<uint32, TeamComposition> _enemyCompositions;

    // Focus targets (playerGuid -> target GUID)
    std::unordered_map<uint32, ObjectGuid> _focusTargets;

    // Pillar database (mapId -> pillars)
    std::unordered_map<uint32, std::vector<ArenaPillar>> _arenaPillars;

    // Metrics
    std::unordered_map<uint32, ArenaMetrics> _playerMetrics;

    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _mutex;

    // Update intervals
    static constexpr uint32 ARENA_UPDATE_INTERVAL = 100;  // 100ms
    std::unordered_map<uint32, uint32> _lastUpdateTimes;

    // Constants
    static constexpr float PILLAR_RANGE = 30.0f;
    static constexpr float REGROUP_RANGE = 15.0f;
    static constexpr float BURST_COORDINATION_RANGE = 40.0f;
};

} // namespace Playerbot
