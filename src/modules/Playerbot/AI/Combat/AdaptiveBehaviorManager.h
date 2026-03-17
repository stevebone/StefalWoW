/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_ADAPTIVE_BEHAVIOR_MANAGER_H
#define TRINITYCORE_ADAPTIVE_BEHAVIOR_MANAGER_H

#include "Define.h"
#include "CombatStateAnalyzer.h"
#include "SharedDefines.h"
#include "AI/Decision/DecisionFusionSystem.h" // Full definition for DecisionVote and CombatContext
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>

class Player;
class Unit;
class Group;
class SpellInfo;

namespace Playerbot
{
    // Bot roles for behavior adaptation
    enum class BotRole : uint8
    {
        NONE            = 0,
        TANK            = 1,
        HEALER          = 2,
        MELEE_DPS       = 3,
        RANGED_DPS      = 4,
        HYBRID          = 5,
        OFF_TANK        = 6,
        OFF_HEALER      = 7,
        CROWD_CONTROL   = 8,
        SUPPORT         = 9
    };

    // Behavior priority levels
    enum class AdaptiveBehaviorPriority : uint8
    {
        LOW             = 0,
        NORMAL          = 1,
        HIGH            = 2,
        CRITICAL        = 3,
        EMERGENCY       = 4
    };

    // Strategy flags for behavior control
    enum StrategyFlag : uint32
    {
        STRATEGY_NONE               = 0x00000000,
        STRATEGY_DEFENSIVE          = 0x00000001,
        STRATEGY_AGGRESSIVE         = 0x00000002,
        STRATEGY_AOE_FOCUS          = 0x00000004,
        STRATEGY_SINGLE_TARGET      = 0x00000008,
        STRATEGY_CONSERVE_MANA      = 0x00000010,
        STRATEGY_BURST_DAMAGE       = 0x00000020,
        STRATEGY_SURVIVAL           = 0x00000040,
        STRATEGY_CROWD_CONTROL      = 0x00000080,
        STRATEGY_INTERRUPT_FOCUS    = 0x00000100,
        STRATEGY_MOBILITY           = 0x00000200,
        STRATEGY_STAY_RANGED        = 0x00000400,
        STRATEGY_STAY_MELEE         = 0x00000800,
        STRATEGY_EMERGENCY_TANK     = 0x00001000,
        STRATEGY_EMERGENCY_HEAL     = 0x00002000,
        STRATEGY_USE_CONSUMABLES    = 0x00004000,
        STRATEGY_SAVE_COOLDOWNS     = 0x00008000,
        STRATEGY_USE_COOLDOWNS      = 0x00010000,
        STRATEGY_KITE               = 0x00020000,
        STRATEGY_STACK              = 0x00040000,
        STRATEGY_SPREAD             = 0x00080000
    };

    // Behavior profile for specific situations
    struct BehaviorProfile
    {
        ::std::string name;                                                      // Profile name for logging
        AdaptiveBehaviorPriority priority;                                             // Priority level
        ::std::function<bool(const CombatMetrics&, CombatSituation)> condition; // Activation condition
        ::std::function<void(::Player*, uint32)> applyFunction;                 // Apply behavior changes
        uint32 strategyFlags;                                                  // Strategy flags to activate
        uint32 minDuration;                                                    // Minimum time to stay active (ms)
        uint32 maxDuration;                                                    // Maximum time to stay active (ms)
        uint32 cooldown;                                                       // Cooldown before can activate again (ms)
        uint32 lastActivated;                                                  // Last activation time
        uint32 activeTime;                                                     // Time this profile has been active
        bool isActive;                                                         // Currently active

        BehaviorProfile() : priority(AdaptiveBehaviorPriority::NORMAL), strategyFlags(STRATEGY_NONE),
                           minDuration(1000), maxDuration(30000), cooldown(0),
                           lastActivated(0), activeTime(0), isActive(false) {}
    };

    // Group composition data
    struct GroupComposition
    {
        uint32 totalMembers;
        uint32 tanks;
        uint32 healers;
        uint32 meleeDPS;
        uint32 rangedDPS;
        uint32 alive;
        uint32 dead;
        float averageItemLevel;
        bool hasBloodlust;
        bool hasBattleRes;
        bool hasOffHealer;

        GroupComposition() { Reset(); }
        void Reset()
        {
            totalMembers = 0;
            tanks = 0;
            healers = 0;
            meleeDPS = 0;
            rangedDPS = 0;
            alive = 0;
            dead = 0;
            averageItemLevel = 0.0f;
            hasBloodlust = false;
            hasBattleRes = false;
            hasOffHealer = false;
        }
    };

    // Role assignment data
    struct RoleAssignment
    {
        BotRole primaryRole;
        BotRole secondaryRole;
        uint32 rolePriority;        // Priority for this role (1 = highest)
        float roleEffectiveness;    // How well suited for role (0-100)
        uint32 assignedTime;        // When role was assigned
        bool isTemporary;           // Temporary assignment due to emergency

        RoleAssignment() : primaryRole(BotRole::NONE), secondaryRole(BotRole::NONE),
                          rolePriority(999), roleEffectiveness(0.0f),
                          assignedTime(0), isTemporary(false) {}
    };

    // Adaptive behavior manager for dynamic strategy adjustment
    class AdaptiveBehaviorManager
    {
    public:
        explicit AdaptiveBehaviorManager(::Player* bot);
        ~AdaptiveBehaviorManager();

        // Main update function
        void Update(uint32 diff, const CombatMetrics& metrics, CombatSituation situation);

        // Behavior management
        void UpdateBehavior(const CombatMetrics& metrics, CombatSituation situation);
        void AdaptToComposition();
        void AssignRoles();
        void ActivateStrategy(uint32 flags);
        void DeactivateStrategy(uint32 flags);
        bool IsStrategyActive(uint32 flag) const { return (_activeStrategies & flag) != 0; }
        uint32 GetActiveStrategies() const { return _activeStrategies; }

        // Profile management
        void RegisterProfile(const BehaviorProfile& profile);
        void ActivateProfile(const ::std::string& name);
        void DeactivateProfile(const ::std::string& name);
        bool IsProfileActive(const ::std::string& name) const;
        const BehaviorProfile* GetActiveProfile() const;
        ::std::vector<::std::string> GetActiveProfileNames() const;

        // Role management
        BotRole GetPrimaryRole() const { return _roleAssignment.primaryRole; }
        BotRole GetSecondaryRole() const { return _roleAssignment.secondaryRole; }
        const RoleAssignment& GetRoleAssignment() const { return _roleAssignment; }
        bool CanPerformRole(BotRole role) const;
        float GetRoleEffectiveness(BotRole role) const;
        void ForceRole(BotRole role, bool temporary = false);

        // Group composition
        const GroupComposition& GetGroupComposition() const { return _groupComposition; }
        void UpdateGroupComposition();
        bool IsOptimalComposition() const;
        bool NeedsRoleSwitch() const;

        // Emergency behaviors
        bool ShouldEmergencyTank() const;
        bool ShouldEmergencyHeal() const;
        bool ShouldUseDefensiveCooldowns() const;
        bool ShouldUseOffensiveCooldowns() const;
        bool ShouldSaveResources() const;

        // Tactical decisions
        bool PreferAOE() const { return IsStrategyActive(STRATEGY_AOE_FOCUS); }
        bool PreferSingleTarget() const { return IsStrategyActive(STRATEGY_SINGLE_TARGET); }
        bool ShouldKite() const { return IsStrategyActive(STRATEGY_KITE); }
        bool ShouldStack() const { return IsStrategyActive(STRATEGY_STACK); }
        bool ShouldSpread() const { return IsStrategyActive(STRATEGY_SPREAD); }
        bool ShouldInterruptFocus() const { return IsStrategyActive(STRATEGY_INTERRUPT_FOCUS); }
        bool ShouldUseCrowdControl() const { return IsStrategyActive(STRATEGY_CROWD_CONTROL); }

        /**
         * @brief Get recommended action for DecisionFusion integration
         * @param target Current combat target
         * @param context Current combat context (dungeon, raid, PvP, etc.)
         * @return DecisionVote with role-based action recommendation
         *
         * This method provides intelligent action recommendations based on:
         * - Bot's primary and secondary roles (tank/healer/DPS)
         * - Active behavior strategies (defensive, aggressive, AoE, etc.)
         * - Group composition and role effectiveness
         * - Emergency conditions (low health, emergency tanking/healing)
         * - Combat context (boss fight, trash, arena, etc.)
         *
         * Integrates with Phase 5 DecisionFusion system for unified action arbitration.
         */
        Playerbot::bot::ai::DecisionVote GetRecommendedAction(Unit* target, Playerbot::bot::ai::CombatContext context) const;

        // Resource management
        bool ShouldConserveMana() const { return IsStrategyActive(STRATEGY_CONSERVE_MANA); }
        bool ShouldUseConsumables() const { return IsStrategyActive(STRATEGY_USE_CONSUMABLES); }
        bool ShouldUseCooldowns() const { return IsStrategyActive(STRATEGY_USE_COOLDOWNS); }
        bool ShouldSaveCooldowns() const { return IsStrategyActive(STRATEGY_SAVE_COOLDOWNS); }

        // Positioning preferences
        bool PreferRanged() const { return IsStrategyActive(STRATEGY_STAY_RANGED); }
        bool PreferMelee() const { return IsStrategyActive(STRATEGY_STAY_MELEE); }
        bool NeedsMobility() const { return IsStrategyActive(STRATEGY_MOBILITY); }

        // Performance metrics
        uint32 GetUpdateTime() const { return _lastUpdateTime; }
        uint32 GetAverageUpdateTime() const;
        uint32 GetProfileSwitchCount() const { return _profileSwitchCount; }
        uint32 GetStrategySwitchCount() const { return _strategySwitchCount; }

        // Learning and adaptation
        void RecordDecisionOutcome(const ::std::string& decision, bool success);
        float GetDecisionSuccessRate(const ::std::string& decision) const;
        void AdjustBehaviorWeights();

        // Reset and cleanup
        void Reset();
        void ClearProfiles();
        void ResetStrategies();

    private:
        // Internal initialization
        void InitializeDefaultProfiles();
        void CreateEmergencyTankProfile();
        void CreateAOEProfile();
        void CreateSurvivalProfile();
        void CreateBurstProfile();
        void CreateResourceConservationProfile();

        // Internal update functions
        void UpdateProfiles(uint32 diff, const CombatMetrics& metrics, CombatSituation situation);
        void UpdateRoleAssignment();
        void EvaluateProfileActivation(BehaviorProfile& profile, const CombatMetrics& metrics, CombatSituation situation);
        void ApplyProfile(BehaviorProfile& profile);
        void RemoveProfile(BehaviorProfile& profile);

        // Role calculation
        BotRole DetermineOptimalRole() const;
        BotRole DetermineSecondaryRole() const;
        float CalculateRoleScore(BotRole role) const;
        bool IsRoleNeeded(BotRole role) const;
        uint32 GetRolePriority(BotRole role) const;

        // Strategy helpers
        void ApplyEmergencyStrategies(const CombatMetrics& metrics);
        void ApplyOffensiveStrategies(const CombatMetrics& metrics);
        void ApplyDefensiveStrategies(const CombatMetrics& metrics);
        void ApplyPositioningStrategies(CombatSituation situation);
        void ApplyResourceStrategies(const CombatMetrics& metrics);

        // Utility functions
        Classes GetBotClass() const;
        uint32 GetBotSpec() const;
        bool HasTankSpec() const;
        bool HasHealSpec() const;
        bool HasCrowdControl() const;
        float GetGearScore() const;

        // Member variables
        ::Player* _bot;
        uint32 _activeStrategies;
        RoleAssignment _roleAssignment;
        GroupComposition _groupComposition;

        // Behavior profiles
        ::std::vector<BehaviorProfile> _profiles;
        BehaviorProfile* _activeProfile;
        uint32 _lastProfileSwitch;
        uint32 _profileSwitchCount;

        // Strategy tracking
        uint32 _lastStrategyUpdate;
        uint32 _strategySwitchCount;
        ::std::map<uint32, uint32> _strategyActiveTimes;

        // Decision tracking for learning
        struct DecisionOutcome
        {
            uint32 successCount;
            uint32 failureCount;
            float successRate;
        };
        ::std::map<::std::string, DecisionOutcome> _decisionHistory;

        // Performance tracking
        uint32 _updateTimer;
        uint32 _lastUpdateTime;
        uint32 _totalUpdateTime;
        uint32 _updateCount;

        // Caches
        mutable uint32 _compositionCacheTime;
        mutable uint32 _roleCacheTime;

        // Deferred initialization flag
        bool _rolesInitialized;
    };

    // Helper functions for role determination
    inline const char* GetRoleName(BotRole role)
    {
        switch (role)
        {
            case BotRole::TANK: return "Tank";
            case BotRole::HEALER: return "Healer";
            case BotRole::MELEE_DPS: return "Melee DPS";
            case BotRole::RANGED_DPS: return "Ranged DPS";
            case BotRole::HYBRID: return "Hybrid";
            case BotRole::OFF_TANK: return "Off-Tank";
            case BotRole::OFF_HEALER: return "Off-Healer";
            case BotRole::CROWD_CONTROL: return "Crowd Control";
            case BotRole::SUPPORT: return "Support";
            default: return "None";
        }
    }

    inline bool IsHealingRole(BotRole role)
    {
        return role == BotRole::HEALER || role == BotRole::OFF_HEALER;
    }

    inline bool IsTankingRole(BotRole role)
    {
        return role == BotRole::TANK || role == BotRole::OFF_TANK;
    }

    inline bool IsDPSRole(BotRole role)
    {
        return role == BotRole::MELEE_DPS || role == BotRole::RANGED_DPS;
    }

} // namespace Playerbot

#endif // TRINITYCORE_ADAPTIVE_BEHAVIOR_MANAGER_H