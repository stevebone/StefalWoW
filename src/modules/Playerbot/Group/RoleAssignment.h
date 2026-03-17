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
#include "Group.h"
#include "GroupRoleEnums.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <atomic>
#include <mutex>
#include <chrono>
#include "GameTime.h"

class Player;
class Group;

namespace Playerbot
{

// Role performance tracking (previously in IRoleAssignment interface)
struct RolePerformance
{
    uint32 successfulAssignments{0};
    uint32 failedAssignments{0};
    std::atomic<float> averageEffectiveness{0.5f};
    uint32 lastAssignmentTime{0};

    // Additional tracking fields for detailed role performance
    std::atomic<uint32> successfulEncounters{0};
    std::atomic<uint32> failedEncounters{0};
    std::atomic<uint32> assignmentsAccepted{0};
    std::atomic<uint32> assignmentsDeclined{0};
    std::atomic<float> performanceRating{0.5f};
    std::chrono::steady_clock::time_point lastPerformanceUpdate{};

    float GetSuccessRate() const
    {
        uint32 total = successfulAssignments + failedAssignments;
        return total > 0 ? static_cast<float>(successfulAssignments) / total : 0.5f;
    }

    // Copy constructor for atomic members
    RolePerformance() = default;
    RolePerformance(RolePerformance const& other)
        : successfulAssignments(other.successfulAssignments)
        , failedAssignments(other.failedAssignments)
        , averageEffectiveness(other.averageEffectiveness.load())
        , lastAssignmentTime(other.lastAssignmentTime)
        , successfulEncounters(other.successfulEncounters.load())
        , failedEncounters(other.failedEncounters.load())
        , assignmentsAccepted(other.assignmentsAccepted.load())
        , assignmentsDeclined(other.assignmentsDeclined.load())
        , performanceRating(other.performanceRating.load())
        , lastPerformanceUpdate(other.lastPerformanceUpdate)
    {}

    RolePerformance& operator=(RolePerformance const& other)
    {
        if (this != &other)
        {
            successfulAssignments = other.successfulAssignments;
            failedAssignments = other.failedAssignments;
            averageEffectiveness.store(other.averageEffectiveness.load());
            lastAssignmentTime = other.lastAssignmentTime;
            successfulEncounters.store(other.successfulEncounters.load());
            failedEncounters.store(other.failedEncounters.load());
            assignmentsAccepted.store(other.assignmentsAccepted.load());
            assignmentsDeclined.store(other.assignmentsDeclined.load());
            performanceRating.store(other.performanceRating.load());
            lastPerformanceUpdate = other.lastPerformanceUpdate;
        }
        return *this;
    }
};

// Role statistics for monitoring (previously in IRoleAssignment interface)
struct RoleStatistics
{
    std::unordered_map<GroupRole, uint32> roleAssignmentCounts;
    std::unordered_map<GroupRole, float> roleSuccessRates;
    std::atomic<uint32> totalAssignments{0};
    std::atomic<uint32> totalReassignments{0};
    float averageCompositionScore{0.0f};

    // Additional atomic counters for thread-safe updates
    std::atomic<uint32> successfulAssignments{0};
    std::atomic<uint32> roleConflicts{0};
    std::atomic<uint32> emergencyFills{0};
    std::chrono::steady_clock::time_point lastStatsUpdate{};

    void Reset()
    {
        roleAssignmentCounts.clear();
        roleSuccessRates.clear();
        totalAssignments.store(0);
        totalReassignments.store(0);
        averageCompositionScore = 0.0f;
        successfulAssignments.store(0);
        roleConflicts.store(0);
        emergencyFills.store(0);
        lastStatsUpdate = std::chrono::steady_clock::now();
    }

    // Copy constructor for atomic members
    RoleStatistics() = default;
    RoleStatistics(RoleStatistics const& other)
        : roleAssignmentCounts(other.roleAssignmentCounts)
        , roleSuccessRates(other.roleSuccessRates)
        , totalAssignments(other.totalAssignments.load())
        , totalReassignments(other.totalReassignments.load())
        , averageCompositionScore(other.averageCompositionScore)
        , successfulAssignments(other.successfulAssignments.load())
        , roleConflicts(other.roleConflicts.load())
        , emergencyFills(other.emergencyFills.load())
        , lastStatsUpdate(other.lastStatsUpdate)
    {}

    RoleStatistics& operator=(RoleStatistics const& other)
    {
        if (this != &other)
        {
            roleAssignmentCounts = other.roleAssignmentCounts;
            roleSuccessRates = other.roleSuccessRates;
            totalAssignments.store(other.totalAssignments.load());
            totalReassignments.store(other.totalReassignments.load());
            averageCompositionScore = other.averageCompositionScore;
            successfulAssignments.store(other.successfulAssignments.load());
            roleConflicts.store(other.roleConflicts.load());
            emergencyFills.store(other.emergencyFills.load());
            lastStatsUpdate = other.lastStatsUpdate;
        }
        return *this;
    }
};

struct RoleScore
{
    GroupRole role;
    float effectiveness;        // 0.0 - 1.0 how well they can perform role
    float gearScore;           // 0.0 - 1.0 gear appropriateness for role
    float experienceScore;     // 0.0 - 1.0 bot's experience in role
    float availabilityScore;   // 0.0 - 1.0 willingness to perform role
    float synergy;             // 0.0 - 1.0 synergy with group composition
    float totalScore;          // Combined weighted score

    RoleScore(GroupRole r = GroupRole::NONE) : role(r), effectiveness(0.0f)
        , gearScore(0.0f), experienceScore(0.5f), availabilityScore(1.0f)
        , synergy(0.5f), totalScore(0.0f) {}

    void CalculateTotalScore()
    {
        totalScore = (effectiveness * 0.4f) + (gearScore * 0.25f) +
                    (experienceScore * 0.15f) + (availabilityScore * 0.1f) +
                    (synergy * 0.1f);
    }
};

struct PlayerRoleProfile
{
    uint32 playerGuid;
    uint8 playerClass;
    uint8 playerSpec;
    uint32 playerLevel;
    std::unordered_map<GroupRole, RoleCapability> roleCapabilities;
    std::unordered_map<GroupRole, RoleScore> roleScores;
    GroupRole preferredRole;
    GroupRole assignedRole;
    std::vector<GroupRole> alternativeRoles;
    uint32 lastRoleUpdate;
    bool isFlexible;
    float overallRating;

    // Default constructor required for std::pair/std::unordered_map usage
    PlayerRoleProfile()
        : playerGuid(0), playerClass(0), playerSpec(0), playerLevel(0)
        , preferredRole(GroupRole::NONE), assignedRole(GroupRole::NONE)
        , lastRoleUpdate(0), isFlexible(true), overallRating(5.0f) {}

    PlayerRoleProfile(uint32 guid, uint8 cls, uint8 spec, uint32 level)
        : playerGuid(guid), playerClass(cls), playerSpec(spec), playerLevel(level)
        , preferredRole(GroupRole::NONE), assignedRole(GroupRole::NONE)
        , lastRoleUpdate(GameTime::GetGameTimeMS()), isFlexible(true), overallRating(5.0f) {}
};

struct GroupComposition
{
    std::unordered_map<GroupRole, std::vector<uint32>> roleAssignments; // role -> player guids
    std::unordered_map<GroupRole, uint32> roleRequirements;             // role -> count needed
    std::unordered_map<GroupRole, uint32> roleFulfillment;              // role -> count assigned
    float compositionScore;
    bool isValid;
    bool hasMainTank;
    bool hasMainHealer;
    uint32 dpsCount;
    uint32 totalMembers;

    GroupComposition() : compositionScore(0.0f), isValid(false)
        , hasMainTank(false), hasMainHealer(false), dpsCount(0), totalMembers(0)
        {
        // Initialize role requirements for standard 5-man group
        roleRequirements[GroupRole::TANK] = 1;
        roleRequirements[GroupRole::HEALER] = 1;
        roleRequirements[GroupRole::MELEE_DPS] = 2;
        roleRequirements[GroupRole::RANGED_DPS] = 1;
        roleRequirements[GroupRole::SUPPORT] = 0;
    }
};

class TC_GAME_API RoleAssignment final
{
public:
    explicit RoleAssignment(Player* bot);
    ~RoleAssignment();
    RoleAssignment(RoleAssignment const&) = delete;
    RoleAssignment& operator=(RoleAssignment const&) = delete;

    // Core role assignment
    bool AssignRoles(Group* group, RoleAssignmentStrategy strategy = RoleAssignmentStrategy::OPTIMAL_ASSIGNMENT);
    bool AssignRole(GroupRole role, Group* group);
    bool SwapRoles(uint32 player1Guid, uint32 player2Guid, Group* group);
    void OptimizeRoleDistribution(Group* group);

    // Role analysis and scoring
    PlayerRoleProfile AnalyzePlayerCapabilities();
    std::vector<RoleScore> CalculateRoleScores(Group* group);
    GroupRole RecommendRole(Group* group);
    float CalculateRoleSynergy(GroupRole role, Group* group);

    // Group composition analysis
    GroupComposition AnalyzeGroupComposition(Group* group);
    bool IsCompositionViable(const GroupComposition& composition);
    std::vector<GroupRole> GetMissingRoles(Group* group);
    std::vector<uint32> FindPlayersForRole(GroupRole role, const std::vector<Player*>& candidates);

    // Dynamic role adjustment
    void HandleRoleConflict(Group* group, GroupRole conflictedRole);
    void RebalanceRoles(Group* group);
    void AdaptToGroupChanges(Group* group, Player* newMember = nullptr, Player* leavingMember = nullptr);
    bool CanPlayerSwitchRole(GroupRole newRole, Group* group);

    // Content-specific role optimization
    void OptimizeForDungeon(Group* group, uint32 dungeonId);
    void OptimizeForRaid(Group* group, uint32 raidId);
    void OptimizeForPvP(Group* group, uint32 battlegroundId);
    void OptimizeForQuesting(Group* group, uint32 questId);

    // Role preferences and constraints
    void SetPlayerRolePreference(GroupRole preferredRole);
    GroupRole GetPlayerRolePreference();
    void SetRoleFlexibility(bool isFlexible);
    void AddRoleConstraint(GroupRole role, RoleCapability capability);

    // Role performance tracking
    RolePerformance GetPlayerRolePerformance(GroupRole role);
    void UpdateRolePerformance(GroupRole role, bool wasSuccessful, float effectiveness);

    // Role assignment validation
    bool ValidateRoleAssignment(Group* group);
    std::vector<std::string> GetRoleAssignmentIssues(Group* group);
    bool CanGroupFunction(Group* group);

    // Emergency role filling
    bool FillEmergencyRole(Group* group, GroupRole urgentRole);
    std::vector<uint32> FindEmergencyReplacements(GroupRole role, uint32 minLevel, uint32 maxLevel);
    void HandleRoleEmergency(Group* group, uint32 disconnectedPlayerGuid);

    // Role statistics and monitoring
    RoleStatistics GetGlobalRoleStatistics();
    void UpdateRoleStatistics();

    // Configuration and settings
    void SetRoleAssignmentStrategy(Group* group, RoleAssignmentStrategy strategy);
    void SetContentTypeRequirements(uint32 contentId, const std::unordered_map<GroupRole, uint32>& requirements);
    void EnableAutoRoleAssignment(bool enable) { _autoAssignmentEnabled = enable; }

    // Update and maintenance
    void Update(uint32 diff);
    void RefreshPlayerProfiles();
    void CleanupInactiveProfiles();

private:
    Player* _bot;

    // Core data storage
    std::unordered_map<uint32, PlayerRoleProfile> _playerProfiles; // playerGuid -> profile
    std::unordered_map<uint32, GroupComposition> _groupCompositions; // groupId -> composition
    std::unordered_map<uint32, RoleAssignmentStrategy> _groupStrategies; // groupId -> strategy
    

    // Role performance tracking
    std::unordered_map<uint32, std::unordered_map<GroupRole, RolePerformance>> _rolePerformance; // playerGuid -> role -> performance
    

    // Content-specific requirements
    std::unordered_map<uint32, std::unordered_map<GroupRole, uint32>> _contentRequirements; // contentId -> role requirements
    std::unordered_map<uint32, GroupComposition> _contentOptimalCompositions; // contentId -> optimal composition

    // Global settings
    std::atomic<bool> _autoAssignmentEnabled{true};
    RoleStatistics _globalStatistics;

    // Class and specialization role mappings
    std::unordered_map<uint8, std::unordered_map<uint8, std::vector<std::pair<GroupRole, RoleCapability>>>> _classSpecRoles;

    // Helper functions
    void InitializeClassRoleMappings();
    void BuildPlayerProfile(PlayerRoleProfile& profile);
    void CalculateRoleCapabilities(PlayerRoleProfile& profile);
    void AnalyzePlayerGear(PlayerRoleProfile& profile);
    void UpdateRoleExperience(PlayerRoleProfile& profile);
    GroupRole DetermineOptimalRole(Group* group, RoleAssignmentStrategy strategy);
    float CalculateCompositionScore(const GroupComposition& composition);
    bool HasRoleConflict(Group* group, GroupRole role);
    void ResolveRoleConflict(Group* group, GroupRole role);
    std::vector<uint32> GetAlternativePlayers(GroupRole role, Group* group);
    void NotifyRoleAssignment(GroupRole role, Group* group);
    void HandleRoleAssignmentFailure(Group* group, const std::string& reason);

    // Scoring algorithms
    float CalculateClassRoleEffectiveness(uint8 playerClass, uint8 playerSpec, GroupRole role);
    float CalculateGearScore(GroupRole role);
    float CalculateExperienceScore(GroupRole role);
    float CalculateSynergyScore(GroupRole role, Group* group);
    float CalculateFlexibilityBonus(Group* group);

    // Assignment strategies
    void ExecuteOptimalStrategy(Group* group);
    void ExecuteBalancedStrategy(Group* group);
    void ExecuteFlexibleStrategy(Group* group);
    void ExecuteStrictStrategy(Group* group);
    void ExecuteHybridStrategy(Group* group);
    void ExecuteDungeonStrategy(Group* group);
    void ExecuteRaidStrategy(Group* group);
    void ExecutePvPStrategy(Group* group);

    // Constants
    static constexpr float MIN_ROLE_EFFECTIVENESS = 0.3f;
    static constexpr float ROLE_SWITCH_COOLDOWN = 30000.0f; // 30 seconds
    static constexpr uint32 PROFILE_UPDATE_INTERVAL = 60000; // 1 minute
    static constexpr float COMPOSITION_SCORE_THRESHOLD = 6.0f;
    static constexpr uint32 MAX_ROLE_ASSIGNMENT_ATTEMPTS = 5;
    static constexpr float HYBRID_CLASS_BONUS = 0.1f;
    static constexpr float EXPERIENCE_WEIGHT = 0.15f;
    static constexpr float GEAR_WEIGHT = 0.25f;
    static constexpr float EFFECTIVENESS_WEIGHT = 0.4f;
    static constexpr float SYNERGY_WEIGHT = 0.1f;
    static constexpr float AVAILABILITY_WEIGHT = 0.1f;
};

} // namespace Playerbot