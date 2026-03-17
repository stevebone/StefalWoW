/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <vector>
#include <unordered_map>

class Player;
class Creature;

namespace Playerbot
{

/**
 * @class InnkeeperInteractionManager
 * @brief Manages all innkeeper interactions for player bots
 *
 * This class provides complete innkeeper functionality using TrinityCore's
 * inn/bind system APIs. It handles:
 * - Hearthstone binding to new inns
 * - Rested state management
 * - Inn proximity detection
 * - Smart innkeeper selection based on questing/leveling zones
 *
 * Performance Target: <0.5ms per interaction
 * Memory Target: <10KB overhead
 *
 * TrinityCore API Integration:
 * - Player::SetHomebind() - Set hearthstone location
 * - Player::GetHomebindX/Y/Z/MapId() - Get current bind location
 * - Player::SetRestState() - Set rested bonus
 * - Creature::IsInnkeeper() - Verify NPC type
 */
class InnkeeperInteractionManager
{
public:
    /**
     * @brief Rest state preference
     */
    enum class RestPreference : uint8
    {
        ALWAYS_REST,      // Always seek rest when possible
        LOW_HEALTH_ONLY,  // Rest only when health/mana is low
        NEVER_REST,       // Never actively seek rest (except for binding)
        AUTO              // Automatic based on situation
    };

    /**
     * @brief Inn binding evaluation
     */
    struct InnEvaluation
    {
        ObjectGuid innkeeperGuid;      // Innkeeper GUID
        ::Position position;             // Inn position
        uint32 mapId;                   // Map ID
        uint32 zoneId;                  // Zone ID
        float distanceToQuestZone;      // Distance to active quest zones
        float distanceFromCurrent;      // Distance from current bind
        bool isCurrentBind;             // Is this the current bind location
        bool isRecommended;             // Is this a recommended bind location
        ::std::string reason;             // Human-readable reason for evaluation

        InnEvaluation()
            : mapId(0), zoneId(0), distanceToQuestZone(0.0f)
            , distanceFromCurrent(0.0f), isCurrentBind(false), isRecommended(false)
        { }
    };

    /**
     * @brief Current homebind info
     */
    struct HomebindInfo
    {
        float x, y, z;
        uint32 mapId;
        uint32 zoneId;
        ::std::string zoneName;
        bool isValid;

        HomebindInfo()
            : x(0.0f), y(0.0f), z(0.0f), mapId(0), zoneId(0), isValid(false)
        { }
    };

    InnkeeperInteractionManager(Player* bot);
    ~InnkeeperInteractionManager() = default;

    // Core Innkeeper Methods

    /**
     * @brief Bind hearthstone at an innkeeper
     * @param innkeeper Innkeeper creature
     * @return True if successfully bound
     *
     * Sets the bot's hearthstone bind location to the innkeeper's location
     * using Player::SetHomebind() API.
     */
    bool BindHearthstone(Creature* innkeeper);

    /**
     * @brief Check if bot should bind at this innkeeper
     * @param innkeeper Innkeeper creature
     * @return True if binding is recommended
     *
     * Evaluates whether the bot should change its bind location based on:
     * - Distance from current bind
     * - Proximity to quest objectives
     * - Level-appropriate zones
     */
    bool ShouldBindHere(Creature* innkeeper) const;

    /**
     * @brief Smart bind - automatically bind if beneficial
     * @param innkeeper Innkeeper creature
     * @return True if bound (or already bound here)
     *
     * Evaluates the innkeeper location and binds if it's more strategic
     * than the current bind location.
     */
    bool SmartBind(Creature* innkeeper);

    /**
     * @brief Use hearthstone to return to bind location
     * @return True if hearthstone was successfully used
     *
     * Uses the hearthstone spell if available and not on cooldown.
     */
    bool UseHearthstone();

    /**
     * @brief Check if hearthstone is available (not on cooldown)
     * @return True if hearthstone can be used
     */
    bool IsHearthstoneReady() const;

    /**
     * @brief Get hearthstone cooldown remaining
     * @return Cooldown in seconds, or 0 if ready
     */
    uint32 GetHearthstoneCooldown() const;

    // Rested State Methods

    /**
     * @brief Start resting at an inn
     * @param innkeeper Innkeeper creature
     * @return True if resting started
     *
     * Sets the bot to rested state if at an inn.
     */
    bool StartResting(Creature* innkeeper);

    /**
     * @brief Check if bot is currently resting
     * @return True if resting
     */
    bool IsResting() const;

    /**
     * @brief Get current rested bonus percentage
     * @return Rested bonus (0-150)
     */
    uint32 GetRestedBonus() const;

    /**
     * @brief Set rest preference
     * @param preference Rest behavior preference
     */
    void SetRestPreference(RestPreference preference) { m_restPreference = preference; }
    RestPreference GetRestPreference() const { return m_restPreference; }

    // Inn Analysis Methods

    /**
     * @brief Get current homebind information
     * @return HomebindInfo structure
     */
    HomebindInfo GetCurrentHomebind() const;

    /**
     * @brief Evaluate an innkeeper for binding
     * @param innkeeper Innkeeper creature
     * @return Evaluation result
     */
    InnEvaluation EvaluateInnkeeper(Creature* innkeeper) const;

    /**
     * @brief Check if bot is at an inn
     * @return True if at an inn
     */
    bool IsAtInn() const;

    /**
     * @brief Check if creature is an innkeeper
     * @param creature Creature to check
     * @return True if innkeeper
     */
    bool IsInnkeeper(Creature* creature) const;

    /**
     * @brief Get distance to current homebind location
     * @return Distance in yards
     */
    float GetDistanceToHomebind() const;

    /**
     * @brief Find nearest innkeeper
     * @param maxRange Maximum search range (default 100 yards)
     * @return Nearest innkeeper creature, or nullptr if none found
     */
    Creature* FindNearestInnkeeper(float maxRange = 100.0f) const;

    // Strategic Methods

    /**
     * @brief Get recommended bind location for current questing
     * @return Zone ID of recommended bind location, or 0 if no preference
     *
     * Analyzes active quests and returns the zone ID where binding
     * would be most strategic.
     */
    uint32 GetRecommendedBindZone() const;

    /**
     * @brief Check if changing bind would be beneficial
     * @param newZoneId Zone ID of potential new bind
     * @return True if changing bind is recommended
     */
    bool ShouldChangeBind(uint32 newZoneId) const;

    // Statistics and Performance

    struct Statistics
    {
        uint32 hearthstoneBounds;     // Times hearthstone was bound
        uint32 hearthstoneUses;       // Times hearthstone was used
        uint32 bindChanges;           // Number of bind location changes
        uint32 restingSessions;       // Number of rest sessions
        uint32 totalRestTime;         // Total rest time in seconds

        Statistics()
            : hearthstoneBounds(0), hearthstoneUses(0), bindChanges(0)
            , restingSessions(0), totalRestTime(0)
        { }
    };

    Statistics const& GetStatistics() const { return m_stats; }
    void ResetStatistics() { m_stats = Statistics(); }

    float GetCPUUsage() const { return m_cpuUsage; }
    size_t GetMemoryUsage() const;

private:
    // Internal Helper Methods

    /**
     * @brief Execute hearthstone bind via TrinityCore API
     * @param innkeeper Innkeeper creature
     * @return True if successfully bound
     */
    bool ExecuteBind(Creature* innkeeper);

    /**
     * @brief Calculate strategic value of binding at a location
     * @param position Position to evaluate
     * @param mapId Map ID
     * @return Strategic value score (higher = better)
     */
    float CalculateBindValue(::Position const& position, uint32 mapId) const;

    /**
     * @brief Check if position is in a strategic leveling zone
     * @param position Position to check
     * @param mapId Map ID
     * @return True if strategic
     */
    bool IsStrategicLocation(::Position const& position, uint32 mapId) const;

    /**
     * @brief Get active quest zones
     * @return Vector of zone IDs with active quests
     */
    ::std::vector<uint32> GetActiveQuestZones() const;

    /**
     * @brief Record binding in statistics
     */
    void RecordBind(bool isNewLocation);

    /**
     * @brief Record hearthstone use in statistics
     */
    void RecordHearthstoneUse(bool success);

    /**
     * @brief Calculate distance to quest zones
     * @param position Position to evaluate
     * @param mapId Map ID
     * @return Distance to nearest quest zone
     */
    float CalculateDistanceToQuestZones(::Position const& position, uint32 mapId) const;

    /**
     * @brief Determine if binding is recommended
     * @param eval Evaluation to update
     * @param currentBind Current homebind info
     */
    void DetermineRecommendation(InnEvaluation& eval, HomebindInfo const& currentBind) const;

    // Member Variables
    Player* m_bot;
    Statistics m_stats;
    RestPreference m_restPreference;

    // Performance Tracking
    float m_cpuUsage;
    uint32 m_totalInteractionTime;   // microseconds
    uint32 m_interactionCount;

    // Cache
    mutable HomebindInfo m_cachedHomebind;
    mutable uint32 m_lastHomebindCheck;
    static constexpr uint32 HOMEBIND_CACHE_DURATION = 60000; // 1 minute

    // Hearthstone spell ID
    static constexpr uint32 HEARTHSTONE_SPELL_ID = 8690;
};

} // namespace Playerbot
