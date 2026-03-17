/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ArenaAI.h"
#include "PvPSpellUtils.h"  // WoW 12.0: PvP multiplier and SpellAttr16 support
#include "GameTime.h"
#include "Player.h"
#include "Unit.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "Group.h"
#include "World.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "DB2Stores.h"
#include "DBCEnums.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace
{
    // ============================================================================
    // ELO RATING SYSTEM CONSTANTS
    // ============================================================================

    // K-factor determines rating volatility
    // Higher K = more volatile ratings, faster adjustment
    constexpr int32 K_FACTOR_2V2 = 32;          // 2v2 has higher K for faster convergence
    constexpr int32 K_FACTOR_3V3 = 24;          // 3v3 is the competitive standard
    constexpr int32 K_FACTOR_5V5 = 20;          // 5v5 has lower K due to team size

    // Rating floors and caps
    constexpr uint32 RATING_FLOOR = 0;          // Minimum possible rating
    constexpr uint32 RATING_CAP = 3000;         // Maximum possible rating
    constexpr uint32 DEFAULT_RATING = 1500;     // Starting rating
    constexpr uint32 SOFT_CAP_RATING = 2400;    // Above this, gains are reduced

    // MMR spread limits
    constexpr int32 MAX_RATING_DIFFERENCE = 400; // Max considered difference for expected score

    // High rating modifier thresholds
    constexpr uint32 HIGH_RATING_THRESHOLD_1 = 2000;
    constexpr uint32 HIGH_RATING_THRESHOLD_2 = 2200;
    constexpr uint32 HIGH_RATING_THRESHOLD_3 = 2400;

    /**
     * @brief Calculate expected win probability using ELO formula
     * @param playerRating The player/team rating
     * @param opponentRating The opponent team rating
     * @return Expected score (probability of winning) between 0.0 and 1.0
     */
    float CalculateExpectedScore(int32 playerRating, int32 opponentRating)
    {
        // Clamp rating difference to prevent extreme expected scores
        int32 diff = std::clamp(opponentRating - playerRating, -MAX_RATING_DIFFERENCE, MAX_RATING_DIFFERENCE);

        // ELO expected score formula: E = 1 / (1 + 10^(D/400))
        // where D is the rating difference
        return 1.0f / (1.0f + std::pow(10.0f, static_cast<float>(diff) / 400.0f));
    }

    /**
     * @brief Get K-factor based on arena bracket
     * @param bracket The arena bracket type
     * @return K-factor for rating calculation
     */
    int32 GetKFactorForBracket(Playerbot::ArenaBracket bracket)
    {
        switch (bracket)
        {
            case Playerbot::ArenaBracket::BRACKET_2v2:
                return K_FACTOR_2V2;
            case Playerbot::ArenaBracket::BRACKET_3v3:
                return K_FACTOR_3V3;
            case Playerbot::ArenaBracket::BRACKET_5v5:
                return K_FACTOR_5V5;
            default:
                return K_FACTOR_3V3;
        }
    }

    /**
     * @brief Apply diminishing returns for high ratings
     * @param baseChange The base rating change before modifiers
     * @param currentRating The player's current rating
     * @param isWin Whether this is a win (true) or loss (false)
     * @return Modified rating change with diminishing returns applied
     */
    int32 ApplyHighRatingModifier(int32 baseChange, uint32 currentRating, bool isWin)
    {
        if (!isWin)
            return baseChange; // No reduction for losses

        // Apply diminishing returns for high-rated players
        float modifier = 1.0f;

        if (currentRating >= HIGH_RATING_THRESHOLD_3)
            modifier = 0.6f;  // 60% gains above 2400
        else if (currentRating >= HIGH_RATING_THRESHOLD_2)
            modifier = 0.75f; // 75% gains above 2200
        else if (currentRating >= HIGH_RATING_THRESHOLD_1)
            modifier = 0.9f;  // 90% gains above 2000

        return static_cast<int32>(baseChange * modifier);
    }

    /**
     * @brief Check if player has healer spec using ChrSpecializationEntry
     * @param player The player to check
     * @return True if player's primary spec is healer role
     */
    bool IsPlayerHealer(Player const* player)
    {
        if (!player)
            return false;

        ChrSpecializationEntry const* spec = player->GetPrimarySpecializationEntry();
        return spec && spec->GetRole() == ChrSpecializationRole::Healer;
    }

    /**
     * @brief Check if player has tank spec using ChrSpecializationEntry
     * @param player The player to check
     * @return True if player's primary spec is tank role
     */
    bool IsPlayerTank(Player const* player)
    {
        if (!player)
            return false;

        ChrSpecializationEntry const* spec = player->GetPrimarySpecializationEntry();
        return spec && spec->GetRole() == ChrSpecializationRole::Tank;
    }

    /**
     * @brief Check if player has DPS spec using ChrSpecializationEntry
     * @param player The player to check
     * @return True if player's primary spec is DPS role
     */
    bool IsPlayerDPS(Player const* player)
    {
        if (!player)
            return false;

        ChrSpecializationEntry const* spec = player->GetPrimarySpecializationEntry();
        return spec && spec->GetRole() == ChrSpecializationRole::Dps;
    }

    /**
     * @brief Get player's role from their specialization
     * @param player The player to check
     * @return The role (Tank=0, Healer=1, DPS=2)
     */
    ChrSpecializationRole GetPlayerRole(Player const* player)
    {
        if (!player)
            return ChrSpecializationRole::Dps;

        ChrSpecializationEntry const* spec = player->GetPrimarySpecializationEntry();
        if (spec)
            return spec->GetRole();

        // Fallback: assume DPS
        return ChrSpecializationRole::Dps;
    }

    /**
     * @brief Get arena map IDs for modern WoW arenas
     * @return Set of arena map IDs
     */
    std::unordered_set<uint32> const& GetArenaMapIds()
    {
        static std::unordered_set<uint32> arenaMapIds = {
            559,    // Nagrand Arena
            562,    // Blade's Edge Arena
            572,    // Ruins of Lordaeron
            617,    // Dalaran Arena
            618,    // Ring of Valor
            980,    // Tol'viron Arena
            1134,   // Tiger's Peak (Pandaria)
            1504,   // Black Rook Hold Arena (Legion)
            1505,   // Ashamane's Fall (Legion)
            1552,   // Robodrome (BFA)
            1672,   // The Robodrome
            1825,   // Hook Point (BFA)
            2167,   // Empyrean Domain (Shadowlands)
            2373,   // Enigma Crucible (Dragonflight)
            2509,   // Nokhudon Proving Grounds (Dragonflight)
            2547,   // Ashamane's Fall (Updated)
            2563,   // Maldraxxus Coliseum (Shadowlands)
        };
        return arenaMapIds;
    }
}

namespace Playerbot
{

// ============================================================================
// SINGLETON
// ============================================================================

ArenaAI::ArenaAI(Player* bot)
    : _bot(bot)
{
    if (!_bot)
    {
        TC_LOG_ERROR("playerbot.arena", "ArenaAI: Attempted to create with null bot!");
        return;
    }

    // Initialize shared arena map data once
    if (!_initialized)
    {
        TC_LOG_INFO("playerbot.arena", "ArenaAI: Loading arena map data...");
        // Load arena pillar positions for each map
        // This would be loaded from database or hardcoded positions
        _initialized = true;
        TC_LOG_INFO("playerbot.arena", "ArenaAI: Arena map data initialized");
    }

    // CRITICAL: Do NOT call _bot->GetName() in constructor!
    // Bot's internal data (m_name) is not initialized during constructor chain.
}

ArenaAI::~ArenaAI()
{
    // CRITICAL: Do NOT call _bot->GetName() or GetGUID() in destructor!
    // During destruction, _bot may be in invalid state where these return
    // garbage data, causing ACCESS_VIOLATION when string tries to copy.
}


// Static member initialization
std::unordered_map<uint32, std::vector<ArenaPillar>> ArenaAI::_arenaMapPillars;
ArenaMetrics ArenaAI::_globalMetrics;
bool ArenaAI::_initialized = false;




// ============================================================================
// INITIALIZATION
// ============================================================================

void ArenaAI::Initialize()
{
    std::lock_guard lock(_mutex);

    TC_LOG_INFO("playerbot", "ArenaAI: Initializing arena systems...");

    InitializePillarDatabase();

    TC_LOG_INFO("playerbot", "ArenaAI: Initialization complete");
}

void ArenaAI::InitializePillarDatabase()
{
    // Load pillar locations for each arena map
    LoadBladesEdgePillars();
    LoadNagrandPillars();
    LoadLordaeronPillars();
    LoadDalaranPillars();
    LoadRingOfValorPillars();

    TC_LOG_INFO("playerbot", "ArenaAI: Loaded {} arena pillar configurations",
        _arenaPillars.size());
}

void ArenaAI::LoadBladesEdgePillars()
{
    uint32 mapId = 562; // Blade's Edge Arena
    std::vector<ArenaPillar> pillars;

    // Bridge pillar (center)
    pillars.push_back(ArenaPillar(Position(6238.0f, 262.0f, 0.8f, 0.0f), 8.0f));

    // Side pillars
    pillars.push_back(ArenaPillar(Position(6229.0f, 272.0f, 0.8f, 0.0f), 5.0f));
    pillars.push_back(ArenaPillar(Position(6247.0f, 252.0f, 0.8f, 0.0f), 5.0f));

    _arenaPillars[mapId] = pillars;
}

void ArenaAI::LoadNagrandPillars()
{
    uint32 mapId = 559; // Nagrand Arena
    std::vector<ArenaPillar> pillars;

    // Center pillars
    pillars.push_back(ArenaPillar(Position(4055.0f, 2919.0f, 13.6f, 0.0f), 6.0f));
    pillars.push_back(ArenaPillar(Position(4037.0f, 2935.0f, 13.6f, 0.0f), 6.0f));

    // Corner pillars
    pillars.push_back(ArenaPillar(Position(4070.0f, 2934.0f, 13.6f, 0.0f), 5.0f));
    pillars.push_back(ArenaPillar(Position(4022.0f, 2920.0f, 13.6f, 0.0f), 5.0f));

    _arenaPillars[mapId] = pillars;
}

void ArenaAI::LoadLordaeronPillars()
{
    uint32 mapId = 572; // Ruins of Lordaeron
    std::vector<ArenaPillar> pillars;

    // Tombstone pillars
    pillars.push_back(ArenaPillar(Position(1285.0f, 1667.0f, 39.6f, 0.0f), 4.0f));
    pillars.push_back(ArenaPillar(Position(1295.0f, 1677.0f, 39.6f, 0.0f), 4.0f));
    pillars.push_back(ArenaPillar(Position(1305.0f, 1667.0f, 39.6f, 0.0f), 4.0f));
    pillars.push_back(ArenaPillar(Position(1295.0f, 1657.0f, 39.6f, 0.0f), 4.0f));

    _arenaPillars[mapId] = pillars;
}

void ArenaAI::LoadDalaranPillars()
{
    uint32 mapId = 617; // Dalaran Arena
    std::vector<ArenaPillar> pillars;

    // Water pipes (center)
    pillars.push_back(ArenaPillar(Position(1299.0f, 784.0f, 9.3f, 0.0f), 6.0f));
    pillars.push_back(ArenaPillar(Position(1214.0f, 765.0f, 9.3f, 0.0f), 6.0f));

    _arenaPillars[mapId] = pillars;
}

void ArenaAI::LoadRingOfValorPillars()
{
    uint32 mapId = 618; // Ring of Valor
    std::vector<ArenaPillar> pillars;

    // Center pillars (when lowered)
    pillars.push_back(ArenaPillar(Position(763.0f, -284.0f, 28.3f, 0.0f), 7.0f));
    pillars.push_back(ArenaPillar(Position(763.0f, -294.0f, 28.3f, 0.0f), 7.0f));

    _arenaPillars[mapId] = pillars;
}

void ArenaAI::Update(uint32 /*diff*/)
{
    if (!_bot || !_bot->IsInWorld())
        return;

    // Check if player is in arena
    if (!_bot->InArena())
        return;
    uint32 currentTime = GameTime::GetGameTimeMS();

    // Throttle updates (100ms for arena responsiveness)
    if (_lastUpdateTimes.count(_bot->GetGUID().GetCounter()))
    {
        uint32 timeSinceLastUpdate = currentTime - _lastUpdateTime;
        if (timeSinceLastUpdate < ARENA_UPDATE_INTERVAL)
            return;
    }

    _lastUpdateTime = currentTime;

    std::lock_guard lock(_mutex);

    // Update match state
    UpdateMatchState();

    // Analyze composition if not done
    if (!_teamCompositions.count(_bot->GetGUID().GetCounter()))
        AnalyzeTeamComposition();

    // Execute positioning
    ExecutePositioning();

    // Execute bracket-specific strategy
    ArenaBracket bracket = GetArenaBracket();
    switch (bracket)
    {
        case ArenaBracket::BRACKET_2v2:
            Execute2v2Strategy();
            break;

        case ArenaBracket::BRACKET_3v3:
            Execute3v3Strategy();
            break;

        case ArenaBracket::BRACKET_5v5:
            Execute5v5Strategy();
            break;

        default:
            break;
    }

    // Adapt strategy based on match state
    AdaptStrategy();
}

void ArenaAI::OnMatchStart()
{
    if (!_bot)
        return;

    std::lock_guard lock(_mutex);
    // Initialize match state
    ArenaMatchState state;
    state.matchStartTime = GameTime::GetGameTimeMS();
    _matchState = state;
    // Analyze compositions
    AnalyzeTeamComposition();

    TC_LOG_INFO("playerbot", "ArenaAI: Match started for player {}", _bot->GetGUID().GetCounter());
}

void ArenaAI::OnMatchEnd(bool won)
{
    if (!_bot)
        return;

    std::lock_guard lock(_mutex);

    // Get current bracket for K-factor calculation
    ArenaBracket bracket = GetArenaBracket();
    int32 kFactor = GetKFactorForBracket(bracket);

    // Get current ratings
    uint32 currentRating = _metrics.rating.load();

    // Calculate opponent team average rating
    // In a real scenario, this would come from the battleground/arena data
    // For bots, we estimate based on match state and enemy composition
    uint32 opponentRating = EstimateOpponentRating();

    // Calculate expected score using ELO formula
    float expectedScore = CalculateExpectedScore(static_cast<int32>(currentRating),
                                                  static_cast<int32>(opponentRating));

    // Actual score: 1.0 for win, 0.0 for loss
    float actualScore = won ? 1.0f : 0.0f;

    // Calculate base rating change: K * (actual - expected)
    int32 baseRatingChange = static_cast<int32>(kFactor * (actualScore - expectedScore));

    // Apply high rating diminishing returns for wins
    int32 finalRatingChange = ApplyHighRatingModifier(baseRatingChange, currentRating, won);

    // Ensure minimum change of 1 for wins against lower-rated opponents
    if (won && finalRatingChange < 1)
        finalRatingChange = 1;

    // Ensure minimum loss of 1 for losses against higher-rated opponents
    if (!won && finalRatingChange > -1)
        finalRatingChange = -1;

    // Apply rating change with floor/cap protection
    int32 newRating = static_cast<int32>(currentRating) + finalRatingChange;
    newRating = std::clamp(newRating, static_cast<int32>(RATING_FLOOR), static_cast<int32>(RATING_CAP));
    _metrics.rating = static_cast<uint32>(newRating);

    // Update win/loss metrics
    if (won)
    {
        _metrics.matchesWon++;
        _globalMetrics.matchesWon++;
    }
    else
    {
        _metrics.matchesLost++;
        _globalMetrics.matchesLost++;
    }

    // Update match state statistics
    ArenaMatchState& state = _matchState;
    uint32 matchDuration = GetMatchDuration();

    TC_LOG_INFO("playerbot.arena", "ArenaAI: Match ended for player {} ({}) - {} | "
        "Rating: {} -> {} ({}{}), Bracket: {}v{}, Duration: {}s, "
        "Expected: {:.2f}, K-factor: {}",
        _bot->GetName(), _bot->GetGUID().GetCounter(),
        won ? "WON" : "LOST",
        currentRating, newRating,
        finalRatingChange >= 0 ? "+" : "", finalRatingChange,
        GetBracketTeamSize(bracket), GetBracketTeamSize(bracket),
        matchDuration,
        expectedScore, kFactor);

    // Record match history for performance analysis
    RecordMatchResult(won, currentRating, newRating, opponentRating, matchDuration);

    // Clear match-specific data
    _matchState = ArenaMatchState();
}

// ============================================================================
// STRATEGY SELECTION
// ============================================================================

void ArenaAI::AnalyzeTeamComposition()
{
    if (!_bot)
        return;

    std::lock_guard lock(_mutex);
    TeamComposition teamComp = GetTeamComposition();
    TeamComposition enemyComp = GetEnemyTeamComposition();

    _teamComposition = teamComp;
    _enemyComposition = enemyComp;

    // Select strategy based on compositions
    ArenaStrategy strategy = GetStrategyForComposition(teamComp, enemyComp);
    ArenaProfile profile = GetArenaProfile();
    profile.strategy = strategy;
    SetArenaProfile(profile);

    TC_LOG_INFO("playerbot", "ArenaAI: Player {} team comp: {}, enemy comp: {}, strategy: {}",
        _bot->GetGUID().GetCounter(), static_cast<uint32>(teamComp), static_cast<uint32>(enemyComp),
        static_cast<uint32>(strategy));
}

ArenaStrategy ArenaAI::GetStrategyForComposition(TeamComposition teamComp,
    TeamComposition enemyComp) const
{
    // If enemy has healer, prioritize killing healer
    if (enemyComp == TeamComposition::DPS_HEALER ||
        enemyComp == TeamComposition::DOUBLE_DPS_HEALER ||
        enemyComp == TeamComposition::TANK_DPS_HEALER)
    {
        return ArenaStrategy::KILL_HEALER_FIRST;
    }

    // If both teams are triple DPS, kill lowest health
    if (teamComp == TeamComposition::TRIPLE_DPS &&
        enemyComp == TeamComposition::TRIPLE_DPS)
    {
        return ArenaStrategy::KILL_LOWEST_HEALTH;
    }

    // Default: adaptive strategy
    return ArenaStrategy::ADAPTIVE;
}

void ArenaAI::AdaptStrategy()
{
    if (!_bot)
        return;
    ArenaProfile profile = GetArenaProfile();

    if (profile.strategy != ArenaStrategy::ADAPTIVE)
        return;

    ArenaMatchState state = GetMatchState();
    // If team is losing, switch to aggressive
    if (!state.isWinning)
    {
        profile.positioning = PositioningStrategy::AGGRESSIVE;
    }
    // If team is winning, play defensively
    else
    {
        profile.positioning = PositioningStrategy::DEFENSIVE;
    }

    SetArenaProfile(profile);
}

// ============================================================================
// TARGET SELECTION
// ============================================================================

::Unit* ArenaAI::SelectFocusTarget() const
{
    if (!_bot)
        return nullptr;

    ArenaProfile profile = GetArenaProfile();
    std::vector<::Unit*> enemies = GetEnemyTeam();

    if (enemies.empty())
        return nullptr;

    switch (profile.strategy)
    {
        case ArenaStrategy::KILL_HEALER_FIRST:
        {
            // Find healer using proper spec-based detection
            ::Unit* healerTarget = nullptr;
            float healerLowestHealth = 100.0f;

            for (::Unit* enemy : enemies)
            {
                if (enemy->IsPlayer())
                {
                    ::Player* enemyPlayer = enemy->ToPlayer();

                    // Use ChrSpecializationEntry to properly detect healer role
                    if (IsPlayerHealer(enemyPlayer))
                    {
                        // If multiple healers, prioritize lowest health
                        float healthPct = enemy->GetHealthPct();
                        if (healthPct < healerLowestHealth)
                        {
                            healerLowestHealth = healthPct;
                            healerTarget = enemy;
                        }
                    }
                }
            }

            if (healerTarget)
            {
                TC_LOG_DEBUG("playerbot.arena", "ArenaAI: Player {} targeting healer {} ({}% health)",
                    _bot->GetGUID().GetCounter(),
                    healerTarget->IsPlayer() ? healerTarget->ToPlayer()->GetName() : "Unknown",
                    static_cast<int>(healerLowestHealth));
                return healerTarget;
            }

            // Fallback: No healer found, use lowest health
            [[fallthrough]];
        }

        case ArenaStrategy::KILL_LOWEST_HEALTH:
        {
            // Find lowest health target with priority for targets under CC/pressure
            ::Unit* lowestHealthTarget = nullptr;
            float lowestHealth = 100.0f;
            float lowestPriorityScore = std::numeric_limits<float>::max();

            for (::Unit* enemy : enemies)
            {
                if (enemy->isDead())
                    continue;

                float healthPct = enemy->GetHealthPct();

                // Calculate priority score (lower = higher priority)
                // Base score is health percentage
                float priorityScore = healthPct;

                // Bonus priority for targets already being attacked
                if (enemy->GetVictim() == _bot || IsTargetUnderTeamPressure(enemy))
                    priorityScore -= 15.0f;

                // Bonus priority for targets in execute range
                if (healthPct < 20.0f)
                    priorityScore -= 20.0f;

                // Penalty for targets with defensive cooldowns active
                if (HasDefensiveCooldownActive(enemy))
                    priorityScore += 25.0f;

                // Bonus for targets in line of sight
                if (IsInLineOfSight(enemy))
                    priorityScore -= 10.0f;
                else
                    priorityScore += 50.0f; // Heavy penalty for out of LoS

                if (priorityScore < lowestPriorityScore)
                {
                    lowestPriorityScore = priorityScore;
                    lowestHealth = healthPct;
                    lowestHealthTarget = enemy;
                }
            }

            if (lowestHealthTarget)
            {
                TC_LOG_DEBUG("playerbot.arena", "ArenaAI: Player {} targeting lowest health {} ({}% health, score: {:.1f})",
                    _bot->GetGUID().GetCounter(),
                    lowestHealthTarget->IsPlayer() ? lowestHealthTarget->ToPlayer()->GetName() : "Unknown",
                    static_cast<int>(lowestHealth), lowestPriorityScore);
            }

            return lowestHealthTarget;
        }

        case ArenaStrategy::TRAIN_ONE_TARGET:
        {
            // Keep attacking same target if valid
            ObjectGuid targetGuid = _focusTarget;
            if (!targetGuid.IsEmpty())
            {
                ::Unit* currentTarget = ObjectAccessor::GetUnit(*_bot, targetGuid);
                if (currentTarget && !currentTarget->isDead() && IsInLineOfSight(currentTarget))
                    return currentTarget;
            }

            // Target died or invalid - select new train target
            // Prefer DPS targets for training strategy
            for (::Unit* enemy : enemies)
            {
                if (enemy->IsPlayer() && !enemy->isDead())
                {
                    ::Player* enemyPlayer = enemy->ToPlayer();
                    if (IsPlayerDPS(enemyPlayer) && IsInLineOfSight(enemy))
                    {
                        // Update focus target
                        const_cast<ArenaAI*>(this)->_focusTarget = enemy->GetGUID();
                        return enemy;
                    }
                }
            }
            break;
        }

        case ArenaStrategy::SPREAD_PRESSURE:
        {
            // Find target that hasn't been damaged recently
            ::Unit* freshTarget = nullptr;

            for (::Unit* enemy : enemies)
            {
                if (enemy->isDead() || !IsInLineOfSight(enemy))
                    continue;

                // Check if this target has been taking damage recently
                // Prefer targets not under pressure
                if (!IsTargetUnderTeamPressure(enemy))
                {
                    freshTarget = enemy;
                    break;
                }
            }

            if (freshTarget)
                return freshTarget;

            // Fallback to lowest health
            break;
        }

        case ArenaStrategy::ADAPTIVE:
        case ArenaStrategy::KILL_DPS_FIRST:
        {
            // Find DPS targets first
            ::Unit* dpsTarget = nullptr;
            float lowestDpsHealth = 100.0f;

            for (::Unit* enemy : enemies)
            {
                if (enemy->IsPlayer() && !enemy->isDead())
                {
                    ::Player* enemyPlayer = enemy->ToPlayer();
                    if (IsPlayerDPS(enemyPlayer))
                    {
                        float healthPct = enemy->GetHealthPct();
                        if (healthPct < lowestDpsHealth && IsInLineOfSight(enemy))
                        {
                            lowestDpsHealth = healthPct;
                            dpsTarget = enemy;
                        }
                    }
                }
            }

            if (dpsTarget)
                return dpsTarget;
            break;
        }

        default:
            break;
    }

    // Default: return first valid enemy in LoS
    for (::Unit* enemy : enemies)
    {
        if (!enemy->isDead() && IsInLineOfSight(enemy))
            return enemy;
    }

    return enemies.empty() ? nullptr : enemies[0];
}

bool ArenaAI::ShouldSwitchTarget(::Unit* currentTarget) const
{
    if (!_bot || !currentTarget)
        return true;

    ArenaProfile profile = GetArenaProfile();
    if (!profile.autoSwitch)
        return false;

    // Switch if current target is dead
    if (currentTarget->isDead())
        return true;

    // Switch if better target is available (e.g., healer in LoS)
    ::Unit* newTarget = SelectFocusTarget();
    if (newTarget && newTarget != currentTarget)
    {
        // Switch if new target is healer and in LoS
        if (newTarget->IsPlayer())
        {
            ::Player* newPlayer = newTarget->ToPlayer();
            uint8 newClass = newPlayer->GetClass();
            if ((newClass == CLASS_PRIEST || newClass == CLASS_PALADIN ||
                 newClass == CLASS_SHAMAN || newClass == CLASS_DRUID ||
                 newClass == CLASS_MONK || newClass == CLASS_EVOKER) &&
                IsInLineOfSight(newTarget))
            {
                return true;
            }
        }
    }

    return false;
}

std::vector<::Unit*> ArenaAI::GetKillTargetPriority() const
{
    std::vector<::Unit*> priorities;

    if (!_bot)
        return priorities;

    std::vector<::Unit*> enemies = GetEnemyTeam();

    // Filter out dead enemies
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
        [](::Unit* u) { return !u || u->isDead(); }), enemies.end());

    // Sort by priority score: Lower score = higher priority
    // Priority order: Healers in execute range > Healers > Low health DPS > Tanks
    std::sort(enemies.begin(), enemies.end(),
        [this](::Unit* a, ::Unit* b) {
            float aScore = CalculateTargetPriorityScore(a);
            float bScore = CalculateTargetPriorityScore(b);
            return aScore < bScore;
        });

    return enemies;
}

float ArenaAI::CalculateTargetPriorityScore(::Unit* target) const
{
    if (!target || !target->IsPlayer())
        return 1000.0f; // Non-players have lowest priority

    ::Player* player = target->ToPlayer();
    float score = 50.0f; // Base score

    // Role-based priority
    ChrSpecializationRole role = GetPlayerRole(player);
    switch (role)
    {
        case ChrSpecializationRole::Healer:
            score -= 30.0f; // Healers are high priority
            break;
        case ChrSpecializationRole::Tank:
            score += 20.0f; // Tanks are lower priority
            break;
        case ChrSpecializationRole::Dps:
            score += 0.0f; // DPS are neutral
            break;
    }

    // Health-based priority
    float healthPct = target->GetHealthPct();
    score += healthPct * 0.3f; // Lower health = higher priority

    // Execute range bonus (below 20%)
    if (healthPct < 20.0f)
        score -= 25.0f;
    else if (healthPct < 35.0f)
        score -= 10.0f;

    // LoS penalty
    if (!IsInLineOfSight(target))
        score += 100.0f;

    // Defensive cooldown penalty
    if (HasDefensiveCooldownActive(target))
        score += 30.0f;

    // Already being focused bonus
    if (IsTargetUnderTeamPressure(target))
        score -= 15.0f;

    // Distance consideration
    float distanceSq = _bot->GetExactDistSq(target);
    if (distanceSq > 40.0f * 40.0f)
        score += 20.0f; // Penalty for distant targets

    return score;
}

// ============================================================================
// POSITIONING
// ============================================================================

void ArenaAI::ExecutePositioning()
{
    if (!_bot)
        return;

    ArenaProfile profile = GetArenaProfile();
    switch (profile.positioning)
    {
        case PositioningStrategy::AGGRESSIVE:
            // Move forward, pressure enemies
            break;

        case PositioningStrategy::DEFENSIVE:
            // Stay back, maintain distance
            MaintainOptimalDistance();
            break;

        case PositioningStrategy::PILLAR_KITE:
            if (ShouldPillarKite())
                ExecutePillarKite();
            break;

        case PositioningStrategy::SPREAD_OUT:
            // Spread from teammates to avoid AoE
            break;

        case PositioningStrategy::GROUP_UP:
            RegroupWithTeam();
            break;

        default:
            break;
    }
}

ArenaPillar const* ArenaAI::FindBestPillar() const
{
    if (!_bot)
        return nullptr;

    uint32 mapId = _bot->GetMapId();
    if (!_arenaPillars.count(mapId))
        return nullptr;

    std::vector<ArenaPillar> const& pillars = _arenaPillars.at(mapId);
    if (pillars.empty())
        return nullptr;

    // Find closest pillar
    ArenaPillar const* bestPillar = nullptr;
    float closestDistanceSq = 9999.0f * 9999.0f; // Squared distance

    for (ArenaPillar const& pillar : pillars)
    {
        if (!pillar.isAvailable)
            continue;

        float distanceSq = _bot->GetExactDistSq(pillar.position);
        if (distanceSq < closestDistanceSq)
        {
            closestDistanceSq = distanceSq;
            bestPillar = &pillar;
        }
    }

    return bestPillar;
}

bool ArenaAI::MoveToPillar(ArenaPillar const& pillar)
{
    if (!_bot)
        return false;

    float distance = std::sqrt(_bot->GetExactDistSq(pillar.position)); // Calculate once from squared distance
    if (distance < 5.0f)
        return true;

    // Full implementation: Use PathGenerator to move to pillar
    TC_LOG_DEBUG("playerbot", "ArenaAI: Player {} moving to pillar",
        _bot->GetGUID().GetCounter());

    return false;
}

bool ArenaAI::IsUsingPillarEffectively() const
{
    if (!_bot)
        return false;

    // Check if player is behind pillar and enemy is out of LoS
    std::vector<::Unit*> enemies = GetEnemyTeam();
    for (::Unit* enemy : enemies)
    {
        if (!IsInLineOfSight(enemy))
            return true; // Successfully broke LoS
    }

    return false;
}

bool ArenaAI::MaintainOptimalDistance()
{
    if (!_bot)
        return false;

    float optimalRange = GetOptimalRangeForClass();
    std::vector<::Unit*> enemies = GetEnemyTeam();

    for (::Unit* enemy : enemies)
    {
        float distance = std::sqrt(_bot->GetExactDistSq(enemy)); // Calculate once from squared distance
        if (distance < optimalRange)
        {
            // Too close - kite away
            TC_LOG_DEBUG("playerbot", "ArenaAI: Player {} kiting away from enemy",
                _bot->GetGUID().GetCounter());
            // Full implementation: Move away from enemy
            return true;
        }
    }

    return false;
}

bool ArenaAI::RegroupWithTeam()
{
    if (!_bot)
        return false;

    std::vector<::Player*> teammates = GetTeammates();
    if (teammates.empty())
        return false;

    // Find teammate position
    Position teammatePos = teammates[0]->GetPosition();
    float distance = std::sqrt(_bot->GetExactDistSq(teammatePos)); // Calculate once from squared distance
    if (distance > REGROUP_RANGE)
    {
        // Move to teammate
        TC_LOG_DEBUG("playerbot", "ArenaAI: Player {} regrouping with team",
            _bot->GetGUID().GetCounter());
        // Full implementation: Move to teammate
        return false;
    }

    return true;
}

// ============================================================================
// PILLAR KITING
// ============================================================================

bool ArenaAI::ShouldPillarKite() const
{
    if (!_bot)
        return false;

    ArenaProfile profile = GetArenaProfile();
    if (!profile.usePillars)
        return false;

    // Pillar kite if health below threshold
    if (_bot->GetHealthPct() < profile.pillarKiteHealthThreshold)
        return true;

    // Pillar kite if under heavy pressure
    std::vector<::Unit*> enemies = GetEnemyTeam();
    uint32 enemiesAttacking = 0;

    for (::Unit* enemy : enemies)
    {
        if (enemy->GetVictim() == _bot)
            enemiesAttacking++;
    }

    return enemiesAttacking >= 2;
}

bool ArenaAI::ExecutePillarKite()
{
    if (!_bot)
        return false;

    ArenaPillar const* pillar = FindBestPillar();
    if (!pillar)
        return false;

    // Move to pillar
    if (!MoveToPillar(*pillar))
        return false;

    // Break LoS with enemies
    std::vector<::Unit*> enemies = GetEnemyTeam();
    for (::Unit* enemy : enemies)
    {
        if (IsInLineOfSight(enemy))
            BreakLoSWithPillar(enemy);
    }

    // Update metrics
    _metrics.pillarKites++;
    _globalMetrics.pillarKites++;

    return true;
}

bool ArenaAI::BreakLoSWithPillar(::Unit* enemy)
{
    if (!_bot || !enemy)
        return false;

    // Full implementation: Position player behind pillar relative to enemy
    TC_LOG_DEBUG("playerbot", "ArenaAI: Player {} breaking LoS with pillar",
        _bot->GetGUID().GetCounter());

    return true;
}

// ============================================================================
// COOLDOWN COORDINATION
// ============================================================================

bool ArenaAI::CoordinateOffensiveBurst()
{
    if (!_bot)
        return false;

    if (!IsTeamReadyForBurst())
        return false;

    // Signal burst
    SignalBurst();

    // Use offensive cooldowns
    TC_LOG_INFO("playerbot", "ArenaAI: Player {} coordinating offensive burst",
        _bot->GetGUID().GetCounter());

    // Full implementation: Cast offensive cooldowns

    // Update metrics
    _metrics.successfulBursts++;
    _globalMetrics.successfulBursts++;

    return true;
}
bool ArenaAI::IsTeamReadyForBurst() const
{
    if (!_bot)
        return false;

    std::vector<::Player*> teammates = GetTeammates();

    // Check if teammates are in range and have cooldowns
    for (::Player* teammate : teammates)
    {
        float distance = std::sqrt(_bot->GetExactDistSq(teammate)); // Calculate once from squared distance
        if (distance > BURST_COORDINATION_RANGE)
            return false;

        // Full implementation: Check if teammate has offensive CDs available
    }

    return true;
}

void ArenaAI::SignalBurst()
{
    if (!_bot)
        return;

    std::lock_guard lock(_mutex);
    _burstReady = true;

    TC_LOG_DEBUG("playerbot", "ArenaAI: Player {} signaling burst", _bot->GetGUID().GetCounter());
}

// ============================================================================
// CC COORDINATION
// ============================================================================

bool ArenaAI::CoordinateCCChain(::Unit* target)
{
    if (!_bot || !target)
        return false;

    // Signal CC target to team
    SignalCCTarget(target);

    // Execute CC
    TC_LOG_INFO("playerbot", "ArenaAI: Player {} coordinating CC chain",
        _bot->GetGUID().GetCounter());

    // Update metrics
    _metrics.coordCCs++;
    _globalMetrics.coordCCs++;

    return true;
}

bool ArenaAI::TeammateHasCCAvailable() const
{
    if (!_bot)
        return false;

    std::vector<::Player*> teammates = GetTeammates();
    for (::Player* teammate : teammates)
    {
        // Full implementation: Check if teammate has CC available
        // Query cooldowns and spell availability
    }

    return false;
}

void ArenaAI::SignalCCTarget(::Unit* target)
{
    if (!_bot || !target)
        return;

    TC_LOG_DEBUG("playerbot", "ArenaAI: Player {} signaling CC target {}",
        _bot->GetGUID().GetCounter(), target->GetGUID().GetCounter());

    // Full implementation: Communicate CC target to team
}

// ============================================================================
// 2v2 STRATEGIES
// ============================================================================

void ArenaAI::Execute2v2Strategy()
{
    if (!_bot)
        return;
    TeamComposition comp = _teamComposition;

    switch (comp)
    {
        case TeamComposition::DOUBLE_DPS:
            Execute2v2DoubleDPS();
            break;

        case TeamComposition::DPS_HEALER:
            Execute2v2DPSHealer();
            break;

        default:
            break;
    }
}

void ArenaAI::Execute2v2DoubleDPS()
{
    if (!_bot)
        return;

    // Double DPS: Aggressive strategy, burst same target
    ::Unit* target = SelectFocusTarget();
    if (target)
    {
        _bot->SetSelection(target->GetGUID());
        CoordinateOffensiveBurst();
    }
}

void ArenaAI::Execute2v2DPSHealer()
{
    if (!_bot)
        return;

    // DPS/Healer: Protect healer, pressure enemy healer
    std::vector<::Player*> teammates = GetTeammates();
    for (::Player* teammate : teammates)
    {
        if (IsTeammateInDanger(teammate))
        {
            // Peel for teammate
            TC_LOG_DEBUG("playerbot", "ArenaAI: Player {} peeling for teammate",
                _bot->GetGUID().GetCounter());
            break;
        }
    }

    // Attack enemy healer
    ::Unit* enemyHealer = SelectFocusTarget();
    if (enemyHealer)
        _bot->SetSelection(enemyHealer->GetGUID());
}

// ============================================================================
// 3v3 STRATEGIES
// ============================================================================

void ArenaAI::Execute3v3Strategy()
{
    if (!_bot)
        return;
    TeamComposition comp = _teamComposition;
    switch (comp)
    {
        case TeamComposition::TRIPLE_DPS:
            Execute3v3TripleDPS();
            break;

        case TeamComposition::DOUBLE_DPS_HEALER:
            Execute3v3DoubleDPSHealer();
            break;

        case TeamComposition::TANK_DPS_HEALER:
            Execute3v3TankDPSHealer();
            break;

        default:
            break;
    }
}

void ArenaAI::Execute3v3TripleDPS()
{
    if (!_bot)
        return;

    // Triple DPS: Train one target until death
    ::Unit* target = SelectFocusTarget();
    if (target)
    {
        _bot->SetSelection(target->GetGUID());

        // Save target as focus
        std::lock_guard lock(_mutex);
        _focusTargets[_bot->GetGUID().GetCounter()] = target->GetGUID();
    }
}

void ArenaAI::Execute3v3DoubleDPSHealer()
{
    if (!_bot)
        return;

    // Standard comp: Kill enemy healer, protect friendly healer
    TeamComposition enemyComp = _enemyCompositions[_bot->GetGUID().GetCounter()];
    if (enemyComp == TeamComposition::DOUBLE_DPS_HEALER)
    {
        // Focus enemy healer
        ::Unit* enemyHealer = SelectFocusTarget();
        if (enemyHealer)
            _bot->SetSelection(enemyHealer->GetGUID());
    }
}

void ArenaAI::Execute3v3TankDPSHealer()
{
    if (!_bot)
        return;

    // Tanky comp: Survive and outlast enemy team
    if (_bot->GetHealthPct() < 50)
    {
        // Play defensively
        ExecutePillarKite();
    }
}

// ============================================================================
// 5v5 STRATEGIES
// ============================================================================

void ArenaAI::Execute5v5Strategy()
{
    if (!_bot)
        return;

    // 5v5: Focus fire on priority targets, stay grouped
    ::Unit* target = SelectFocusTarget();
    if (target)
        _bot->SetSelection(target->GetGUID());

    // Stay grouped
    RegroupWithTeam();
}

// ============================================================================
// COMPOSITION COUNTERS
// ============================================================================

ArenaStrategy ArenaAI::GetCounterStrategy(TeamComposition enemyComp) const
{
    switch (enemyComp)
    {
        case TeamComposition::DOUBLE_DPS_HEALER:
            return ArenaStrategy::KILL_HEALER_FIRST;

        case TeamComposition::TRIPLE_DPS:
            return ArenaStrategy::SPREAD_PRESSURE;

        default:
            return ArenaStrategy::ADAPTIVE;
    }
}

void ArenaAI::CounterRMP()
{
    // Counter RMP: Spread out, avoid CC chains, kill healer
    if (!_bot)
        return;

    TC_LOG_DEBUG("playerbot", "ArenaAI: Countering RMP composition");

    // Use defensive positioning
    ArenaProfile profile = GetArenaProfile();
    profile.positioning = PositioningStrategy::SPREAD_OUT;
    SetArenaProfile(profile);
}

void ArenaAI::CounterTSG()
{
    // Counter TSG: Kite melee, pillar, kill healer
    if (!_bot)
        return;

    TC_LOG_DEBUG("playerbot", "ArenaAI: Countering TSG composition");

    ArenaProfile profile = GetArenaProfile();
    profile.positioning = PositioningStrategy::PILLAR_KITE;
    SetArenaProfile(profile);
}

void ArenaAI::CounterTurboCleave()
{
    // Counter Turbo: Interrupt healer, kite melee
    if (!_bot)
        return;

    TC_LOG_DEBUG("playerbot", "ArenaAI: Countering Turbo Cleave composition");
}

// ============================================================================
// MATCH STATE TRACKING
// ============================================================================

ArenaMatchState ArenaAI::GetMatchState() const
{
    if (!_bot)
        return ArenaMatchState();

    std::lock_guard lock(_mutex);
    if (_matchStates.count(_bot->GetGUID().GetCounter()))
        return _matchState;

    return ArenaMatchState();
}

void ArenaAI::UpdateMatchState()
{
    if (!_bot)
        return;

    std::lock_guard lock(_mutex);
    if (!_matchStates.count(_bot->GetGUID().GetCounter()))
        return;

    ArenaMatchState& state = _matchState;

    // Update state
    state.isWinning = IsTeamWinning();

    // Count alive teammates and enemies
    std::vector<::Player*> teammates = GetTeammates();
    state.teammateAliveCount = 0;
    for (::Player* teammate : teammates)
    {
        if (!teammate->isDead())
            state.teammateAliveCount++;
    }

    std::vector<::Unit*> enemies = GetEnemyTeam();
    state.enemyAliveCount = 0;
    for (::Unit* enemy : enemies)
    {
        if (!enemy->isDead())
            state.enemyAliveCount++;
    }
}

bool ArenaAI::IsTeamWinning() const
{
    if (!_bot)
        return false;

    ArenaMatchState state = GetMatchState();
    return state.teammateAliveCount > state.enemyAliveCount;
}

uint32 ArenaAI::GetMatchDuration() const
{
    if (!_bot)
        return 0;

    ArenaMatchState state = GetMatchState();
    uint32 currentTime = GameTime::GetGameTimeMS();

    return (currentTime - state.matchStartTime) / 1000; // Convert to seconds
}

// ============================================================================
// PROFILES
// ============================================================================

void ArenaAI::SetArenaProfile(ArenaProfile const& profile)
{
    std::lock_guard lock(_mutex);
    _profile = profile;
}

ArenaProfile ArenaAI::GetArenaProfile() const
{
    std::lock_guard lock(_mutex);

    if (_playerProfiles.count(_bot->GetGUID().GetCounter()))
        return _profile;

    return ArenaProfile(); // Default
}

// ============================================================================
// METRICS
// ============================================================================

ArenaMetrics const& ArenaAI::GetMetrics() const
{
    std::lock_guard lock(_mutex);

    if (!_playerMetrics.count(_bot->GetGUID().GetCounter()))
    {
        static ArenaMetrics emptyMetrics;
        return emptyMetrics;
    }

    return _metrics;
}

ArenaMetrics const& ArenaAI::GetGlobalMetrics() const
{
    return _globalMetrics;
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

ArenaBracket ArenaAI::GetArenaBracket() const
{
    if (!_bot)
        return ArenaBracket::BRACKET_2v2;

    // Get the battleground the player is in
    Battleground* bg = _bot->GetBattleground();
    if (!bg || !bg->isArena())
        return ArenaBracket::BRACKET_3v3; // Default fallback

    // Get arena type from battleground
    uint8 arenaType = bg->GetArenaType();

    switch (arenaType)
    {
        case ARENA_TYPE_2v2:
            return ArenaBracket::BRACKET_2v2;
        case ARENA_TYPE_3v3:
            return ArenaBracket::BRACKET_3v3;
        case ARENA_TYPE_5v5:
            return ArenaBracket::BRACKET_5v5;
        default:
            break;
    }

    // Fallback: estimate from team size
    Group* group = _bot->GetGroup();
    if (group)
    {
        uint32 memberCount = group->GetMembersCount();
        if (memberCount <= 2)
            return ArenaBracket::BRACKET_2v2;
        else if (memberCount <= 3)
            return ArenaBracket::BRACKET_3v3;
        else
            return ArenaBracket::BRACKET_5v5;
    }

    return ArenaBracket::BRACKET_3v3;
}

uint8 ArenaAI::GetBracketTeamSize(ArenaBracket bracket) const
{
    switch (bracket)
    {
        case ArenaBracket::BRACKET_2v2: return 2;
        case ArenaBracket::BRACKET_3v3: return 3;
        case ArenaBracket::BRACKET_5v5: return 5;
        default: return 3;
    }
}

TeamComposition ArenaAI::GetTeamComposition() const
{
    if (!_bot)
        return TeamComposition::DOUBLE_DPS_HEALER;

    // Analyze team composition based on player specs
    std::vector<::Player*> teammates = GetTeammates();

    // Include self in analysis
    uint32 healerCount = IsPlayerHealer(_bot) ? 1 : 0;
    uint32 tankCount = IsPlayerTank(_bot) ? 1 : 0;
    uint32 dpsCount = IsPlayerDPS(_bot) ? 1 : 0;
    uint32 totalCount = 1;

    for (::Player* teammate : teammates)
    {
        if (!teammate)
            continue;

        totalCount++;

        if (IsPlayerHealer(teammate))
            healerCount++;
        else if (IsPlayerTank(teammate))
            tankCount++;
        else
            dpsCount++;
    }

    // Determine composition based on counts
    ArenaBracket bracket = GetArenaBracket();

    switch (bracket)
    {
        case ArenaBracket::BRACKET_2v2:
        {
            if (healerCount >= 1)
                return TeamComposition::DPS_HEALER;
            return TeamComposition::DOUBLE_DPS;
        }

        case ArenaBracket::BRACKET_3v3:
        {
            if (healerCount >= 1)
            {
                if (tankCount >= 1)
                    return TeamComposition::TANK_DPS_HEALER;
                return TeamComposition::DOUBLE_DPS_HEALER;
            }
            return TeamComposition::TRIPLE_DPS;
        }

        case ArenaBracket::BRACKET_5v5:
        {
            // 5v5 typically runs multiple healers
            if (healerCount >= 2)
                return TeamComposition::DOUBLE_DPS_HEALER; // Close approximation
            else if (healerCount >= 1)
            {
                if (tankCount >= 1)
                    return TeamComposition::TANK_DPS_HEALER;
                return TeamComposition::DOUBLE_DPS_HEALER;
            }
            return TeamComposition::TRIPLE_DPS;
        }

        default:
            break;
    }

    return TeamComposition::DOUBLE_DPS_HEALER;
}

TeamComposition ArenaAI::GetEnemyTeamComposition() const
{
    if (!_bot)
        return TeamComposition::DOUBLE_DPS_HEALER;

    // Analyze enemy team composition
    std::vector<::Unit*> enemies = GetEnemyTeam();

    uint32 healerCount = 0;
    uint32 tankCount = 0;
    uint32 dpsCount = 0;
    uint32 totalCount = 0;

    for (::Unit* enemy : enemies)
    {
        if (!enemy || !enemy->IsPlayer())
            continue;

        ::Player* enemyPlayer = enemy->ToPlayer();
        totalCount++;

        if (IsPlayerHealer(enemyPlayer))
            healerCount++;
        else if (IsPlayerTank(enemyPlayer))
            tankCount++;
        else
            dpsCount++;
    }

    // Determine composition based on counts and bracket
    ArenaBracket bracket = GetArenaBracket();

    if (totalCount == 0)
        return TeamComposition::DOUBLE_DPS_HEALER; // Default assumption

    switch (bracket)
    {
        case ArenaBracket::BRACKET_2v2:
        {
            if (healerCount >= 1)
                return TeamComposition::DPS_HEALER;
            return TeamComposition::DOUBLE_DPS;
        }

        case ArenaBracket::BRACKET_3v3:
        {
            if (healerCount >= 1)
            {
                if (tankCount >= 1)
                    return TeamComposition::TANK_DPS_HEALER;
                return TeamComposition::DOUBLE_DPS_HEALER;
            }
            return TeamComposition::TRIPLE_DPS;
        }

        case ArenaBracket::BRACKET_5v5:
        {
            if (healerCount >= 2)
                return TeamComposition::DOUBLE_DPS_HEALER;
            else if (healerCount >= 1)
            {
                if (tankCount >= 1)
                    return TeamComposition::TANK_DPS_HEALER;
                return TeamComposition::DOUBLE_DPS_HEALER;
            }
            return TeamComposition::TRIPLE_DPS;
        }

        default:
            break;
    }

    return TeamComposition::DOUBLE_DPS_HEALER;
}

std::vector<::Player*> ArenaAI::GetTeammates() const
{
    std::vector<::Player*> teammates;

    if (!_bot)
        return teammates;

    Group* group = _bot->GetGroup();
    if (!group)
        return teammates;

    for (GroupReference const& itr : group->GetMembers())
    {
        ::Player* member = itr.GetSource();
        if (member && member != _bot && member->IsInWorld() && !member->isDead())
            teammates.push_back(member);
    }

    return teammates;
}

std::vector<::Unit*> ArenaAI::GetEnemyTeam() const
{
    std::vector<::Unit*> enemies;

    if (!_bot || !_bot->IsInWorld())
        return enemies;

    // Get battleground for arena-specific enemy detection
    Battleground* bg = _bot->GetBattleground();
    if (bg && bg->isArena())
    {
        // In arena, get all players from opposing team
        ::Team botTeam = _bot->GetBGTeam();
        ::Team enemyTeam = (botTeam == ALLIANCE) ? HORDE : ALLIANCE;

        // Iterate through arena players
        for (auto const& [guid, bgPlayer] : bg->GetPlayers())
        {
            if (bgPlayer.Team == enemyTeam)
            {
                ::Player* player = ObjectAccessor::FindPlayer(guid);
                if (player && player->IsInWorld() && !player->isDead() &&
                    player->IsAlive() && _bot->IsWithinDist(player, 100.0f))
                {
                    enemies.push_back(player);
                }
            }
        }
    }
    else
    {
        // Fallback: Find nearby hostile players
        std::list<::Player*> nearbyPlayers;
        _bot->GetPlayerListInGrid(nearbyPlayers, 100.0f);

        for (::Player* player : nearbyPlayers)
        {
            if (player && player != _bot &&
                player->IsHostileTo(_bot) &&
                !player->isDead() && player->IsAlive())
            {
                enemies.push_back(player);
            }
        }
    }

    // Sort by distance for consistent ordering
    std::sort(enemies.begin(), enemies.end(),
        [this](::Unit* a, ::Unit* b) {
            return _bot->GetExactDistSq(a) < _bot->GetExactDistSq(b);
        });

    return enemies;
}

bool ArenaAI::IsInLineOfSight(::Unit* target) const
{
    if (!_bot || !target)
        return false;

    return _bot->IsWithinLOSInMap(target);
}

float ArenaAI::GetOptimalRangeForClass() const
{
    if (!_bot)
        return 10.0f;

    uint8 playerClass = _bot->GetClass();

    switch (playerClass)
    {
        case CLASS_WARRIOR:
        case CLASS_PALADIN:
        case CLASS_DEATH_KNIGHT:
        case CLASS_ROGUE:
        case CLASS_MONK:
        case CLASS_DEMON_HUNTER:
            return 5.0f; // Melee range

        case CLASS_HUNTER:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
        case CLASS_PRIEST:
        case CLASS_SHAMAN:
        case CLASS_DRUID:
        case CLASS_EVOKER:
            return 30.0f; // Ranged

        default:
            return 10.0f;
    }
}

bool ArenaAI::IsTeammateInDanger(::Player* teammate) const
{
    if (!teammate)
        return false;

    // Check if teammate is low health or under heavy attack
    if (teammate->GetHealthPct() < 30)
        return true;

    return false;
}

// ============================================================================
// RATING SYSTEM HELPERS
// ============================================================================

uint32 ArenaAI::EstimateOpponentRating() const
{
    if (!_bot)
        return DEFAULT_RATING;

    // Get current match state
    ArenaMatchState const& state = _matchState;

    // Base estimate from our own rating
    uint32 baseEstimate = _metrics.rating.load();

    // Analyze opponent team composition for adjustment
    TeamComposition enemyComp = GetEnemyTeamComposition();
    std::vector<::Unit*> enemies = GetEnemyTeam();

    // Estimate based on number of enemies alive
    int32 adjustment = 0;

    // If we're losing, enemies are likely higher rated
    if (!state.isWinning && state.teammateAliveCount < state.enemyAliveCount)
        adjustment += 50;

    // If enemies have strong composition, estimate higher
    if (enemyComp == TeamComposition::DOUBLE_DPS_HEALER)
        adjustment += 25; // Standard comp suggests experience

    // Check enemy health levels - low health enemies might be lower skilled
    float avgEnemyHealth = 0.0f;
    uint32 aliveEnemies = 0;
    for (::Unit* enemy : enemies)
    {
        if (enemy && !enemy->isDead())
        {
            avgEnemyHealth += enemy->GetHealthPct();
            aliveEnemies++;
        }
    }

    if (aliveEnemies > 0)
    {
        avgEnemyHealth /= aliveEnemies;
        // If enemies are taking heavy damage, might be lower rated
        if (avgEnemyHealth < 50.0f)
            adjustment -= 25;
    }

    // Clamp the estimate to reasonable bounds
    int32 estimate = static_cast<int32>(baseEstimate) + adjustment;
    return static_cast<uint32>(std::clamp(estimate,
        static_cast<int32>(RATING_FLOOR),
        static_cast<int32>(RATING_CAP)));
}

void ArenaAI::RecordMatchResult(bool won, uint32 oldRating, uint32 newRating,
    uint32 opponentRating, uint32 duration)
{
    // Store match history for performance analysis
    // This can be expanded to store in database for persistent tracking

    TC_LOG_DEBUG("playerbot.arena", "ArenaAI: Recording match - Won: {}, Rating: {} -> {}, "
        "Opponent: {}, Duration: {}s",
        won ? "Yes" : "No", oldRating, newRating, opponentRating, duration);

    // Update global metrics
    _globalMetrics.matchesWon += won ? 1 : 0;
    _globalMetrics.matchesLost += won ? 0 : 1;
}

bool ArenaAI::IsTargetUnderTeamPressure(::Unit* target) const
{
    if (!target || !_bot)
        return false;

    // Check if target is being attacked by teammates
    std::vector<::Player*> teammates = GetTeammates();
    for (::Player* teammate : teammates)
    {
        if (teammate && teammate->GetTarget() == target->GetGUID())
            return true;
    }

    // Check if target is low health (indicating pressure)
    if (target->GetHealthPct() < 50.0f)
        return true;

    return false;
}

bool ArenaAI::HasDefensiveCooldownActive(::Unit* target) const
{
    if (!target)
        return false;

    // List of major defensive cooldown auras to check
    // These are spell IDs for common defensive abilities
    static const std::vector<uint32> defensiveAuras = {
        // Paladin
        642,    // Divine Shield
        498,    // Divine Protection

        // Mage
        45438,  // Ice Block

        // Rogue
        31224,  // Cloak of Shadows
        5277,   // Evasion

        // Hunter
        186265, // Aspect of the Turtle

        // Warrior
        871,    // Shield Wall
        12975,  // Last Stand
        118038, // Die by the Sword

        // Priest
        47585,  // Dispersion

        // Death Knight
        48792,  // Icebound Fortitude
        48707,  // Anti-Magic Shell

        // Druid
        22812,  // Barkskin
        61336,  // Survival Instincts

        // Monk
        115176, // Zen Meditation
        122278, // Dampen Harm

        // Demon Hunter
        196555, // Netherwalk
        187827, // Metamorphosis (Vengeance)

        // Shaman
        108271, // Astral Shift

        // Warlock
        104773, // Unending Resolve

        // Evoker
        363916, // Obsidian Scales
    };

    for (uint32 spellId : defensiveAuras)
    {
        if (target->HasAura(spellId))
            return true;
    }

    return false;
}

} // namespace Playerbot
