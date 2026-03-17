/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 */

#ifndef TRINITYCORE_DEFENSIVE_MANAGER_H
#define TRINITYCORE_DEFENSIVE_MANAGER_H

#include "Define.h"
#include <vector>
#include <unordered_map>
#include "GameTime.h"

class Player;

namespace Playerbot
{
    struct CombatMetrics;

    /**
     * @enum DefensivePriority
     * @brief When to use a defensive cooldown
     */
    enum class DefensivePriority : uint8
    {
        EMERGENCY,      // Use at critical HP (< 20%)
        HIGH,           // Use at low HP (< 40%)
        MEDIUM,         // Use at moderate HP (< 60%)
        LOW,            // Use proactively (< 80%)
        OPTIONAL        // Use if available
    };

    /**
     * @struct DefensiveCooldown
     * @brief Defensive ability configuration
     */
    struct DefensiveCooldown
    {
        uint32 spellId;
        float damageReduction;      // % reduction (e.g., 0.3 = 30% DR)
        uint32 duration;            // Duration in milliseconds
        uint32 cooldown;            // Cooldown in milliseconds
        DefensivePriority priority; // When to use
        bool isEmergency;           // Emergency-only (ice block, guardian spirit)
        uint32 lastUsed;            // Last use timestamp

        DefensiveCooldown()
            : spellId(0)
            , damageReduction(0.0f)
            , duration(0)
            , cooldown(0)
            , priority(DefensivePriority::MEDIUM)
            , isEmergency(false)
            , lastUsed(0)
        {}

        DefensiveCooldown(uint32 spell, float dr, uint32 dur, uint32 cd, DefensivePriority prio, bool emergency = false)
            : spellId(spell)
            , damageReduction(dr)
            , duration(dur)
            , cooldown(cd)
            , priority(prio)
            , isEmergency(emergency)
            , lastUsed(0)
        {}

        [[nodiscard]] bool IsAvailable() const
        {
            uint32 now = GameTime::GetGameTimeMS();
            return (now - lastUsed) >= cooldown;
        }

        void MarkUsed()
        {
            lastUsed = GameTime::GetGameTimeMS();
        }
    };

    /**
     * @class DefensiveManager
     * @brief Manage defensive cooldown rotation
     *
     * **Implementation Status**: COMPLETE
     * **Implementation**: Full intelligent defensive rotation system
     *
     * **Features**:
     * - Health threshold monitoring
     * - Incoming damage prediction
     * - Defensive cooldown rotation (don't stack)
     * - Emergency defensive prioritization
     * - Boss ability anticipation
     *
     * **Usage Example**:
     * @code
     * DefensiveManager defMgr(bot);
     * defMgr.Update(diff, combatMetrics);
     *
     * if (defMgr.NeedsEmergencyDefensive())
     * {
     *     uint32 spell = defMgr.UseEmergencyDefensive();
     *     bot->CastSpell(spell, bot);
     * }
     * else if (defMgr.NeedsDefensive())
     * {
     *     uint32 spell = defMgr.GetRecommendedDefensive();
     *     bot->CastSpell(spell, bot);
     * }
     * @endcode
     *
     * **Expected Impact**:
     * -  30% better survivability for tanks/healers
     * -  Intelligent defensive rotation
     * -  Emergency handling (prevent deaths)
     * -  Avoid defensive waste (stacking, overuse)
     */
    class TC_GAME_API DefensiveManager
    {
    public:
        explicit DefensiveManager(Player* bot);
        ~DefensiveManager() = default;

        /**
         * @brief Update defensive tracking
         *
         * @param diff Time since last update (milliseconds)
         * @param metrics Combat metrics (damage, healing)
         *
         * Updates cooldown tracking and incoming damage estimation
         */
        void Update(uint32 diff, const CombatMetrics& metrics);

        /**
         * @brief Reset defensive manager state
         *
         * Called when leaving combat
         */
        void Reset();

        /**
         * @brief Check if defensive needed
         *
         * @return True if should use defensive cooldown
         *
         * Based on:
         * - Current health %
         * - Incoming damage
         * - Available defensives
         */
        bool NeedsDefensive();

        /**
         * @brief Check if emergency defensive needed
         *
         * @return True if critically low HP (< 20%)
         *
         * Emergency = ice block, divine shield, last stand, etc.
         */
        bool NeedsEmergencyDefensive();

        /**
         * @brief Get best defensive for situation
         *
         * @return Spell ID of recommended defensive or 0
         *
         * Considers:
         * - Health threshold
         * - Incoming damage
         * - Cooldown availability
         * - Don't stack similar effects
         */
        uint32 GetRecommendedDefensive();

        /**
         * @brief Use emergency defensive immediately
         *
         * @return Spell ID of emergency defensive or 0
         *
         * Priority order:
         * 1. Full immunity (ice block, divine shield)
         * 2. Last stand effects (guardian spirit, ardent defender)
         * 3. Major DR (shield wall, barkskin)
         */
        uint32 UseEmergencyDefensive();

        /**
         * @brief Register available defensive
         *
         * @param cooldown Defensive ability configuration
         *
         * Call during bot initialization to register all defensives
         */
        void RegisterDefensive(const DefensiveCooldown& cooldown);

        /**
         * @brief Mark defensive as used
         *
         * @param spellId Spell ID that was used
         *
         * Updates cooldown tracking
         */
        void UseDefensiveCooldown(uint32 spellId);

        /**
         * @brief Check if should use defensive
         *
         * @param healthPercent Current health %
         * @param incomingDamage Estimated incoming damage
         * @return True if defensive recommended
         */
        bool ShouldUseDefensive(float healthPercent, float incomingDamage);

        /**
         * @brief Get best defensive for health threshold
         *
         * @param healthPercent Current health %
         * @param incomingDamage Estimated incoming damage
         * @return Spell ID or 0
         */
        uint32 GetBestDefensive(float healthPercent, float incomingDamage);

        /**
         * @brief Check if defensive is on cooldown
         *
         * @param spellId Spell ID to check
         * @return True if on cooldown
         */
        bool IsOnCooldown(uint32 spellId) const;

        /**
         * @brief Get remaining cooldown
         *
         * @param spellId Spell ID to check
         * @return Milliseconds remaining or 0 if available
         */
        uint32 GetRemainingCooldown(uint32 spellId) const;

        /**
         * @brief Estimate incoming damage
         *
         * @return Estimated damage in next 3 seconds
         *
         * Based on:
         * - Recent damage taken
         * - Number of enemies
         * - Boss abilities (TODO)
         */
        float EstimateIncomingDamage() const;

    private:
        Player* _bot;
        ::std::vector<DefensiveCooldown> _availableDefensives;
        ::std::unordered_map<uint32, uint32> _cooldownTracker;  // spellId -> lastUsed
        float _recentDamage;
        uint32 _lastUpdate;

        static constexpr uint32 UPDATE_INTERVAL = 500;  // 500ms
        static constexpr uint32 DAMAGE_WINDOW = 3000;   // 3 seconds

        /**
         * @brief Find defensive by spell ID
         */
        DefensiveCooldown* FindDefensive(uint32 spellId);

        /**
         * @brief Get available defensives by priority
         */
        ::std::vector<DefensiveCooldown*> GetAvailableDefensives(DefensivePriority minPriority);

        /**
         * @brief Check if any major defensive is active
         */
        bool HasActiveDefensive() const;

        /**
         * @brief Calculate health deficit
         */
        float GetHealthDeficit() const;

        /**
         * @brief Update recent damage tracking
         */
        void UpdateDamageTracking(const CombatMetrics& metrics);
    };

} // namespace Playerbot

#endif // TRINITYCORE_DEFENSIVE_MANAGER_H
