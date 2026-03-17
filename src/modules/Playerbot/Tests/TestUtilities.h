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
#include "ObjectGuid.h"
#include "Position.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include <memory>
#include <chrono>
#include <functional>
#include <string>

namespace Playerbot
{
namespace Test
{

// Forward declarations
class Player;
class Group;
class Unit;
class WorldSession;
class PlayerbotAI;

/**
 * @struct BotTestData
 * @brief Represents a test bot with all necessary data for testing
 */
struct BotTestData
{
    uint32 characterId = 0;
    ObjectGuid guid;
    ::std::string name;
    uint8 level = 80;
    uint8 race = 1;    // Human
    uint8 class_ = 1;  // Warrior
    Position position;
    bool isInGroup = false;
    ObjectGuid groupId;
    ObjectGuid leaderGuid;

    // Test state flags
    bool hasAcceptedInvitation = false;
    bool isFollowingLeader = false;
    bool isInCombat = false;
    bool isAssistingTarget = false;
    uint32 lastActionTime = 0;

    BotTestData(const ::std::string& botName) : name(botName) {}
};

/**
 * @struct GroupTestData
 * @brief Represents a test group with leader and member bots
 */
struct GroupTestData
{
    ObjectGuid groupId;
    ObjectGuid leaderGuid;
    ::std::string leaderName;
    ::std::vector<BotTestData> members;
    Position groupPosition;
    bool isInCombat = false;
    ObjectGuid currentTarget;
    uint32 creationTime = 0;

    GroupTestData(const ::std::string& leader) : leaderName(leader) {}
};

/**
 * @struct PerformanceMetrics
 * @brief Tracks performance metrics during testing
 */
struct PerformanceMetrics
{
    // Timing metrics (microseconds)
    uint64 invitationAcceptanceTime = 0;
    uint64 followingEngagementTime = 0;
    uint64 combatEngagementTime = 0;
    uint64 targetSwitchTime = 0;
    uint64 teleportTime = 0;

    // Memory metrics (bytes)
    uint64 memoryUsageStart = 0;
    uint64 memoryUsagePeak = 0;
    uint64 memoryUsageEnd = 0;

    // CPU metrics (percentage)
    float cpuUsageStart = 0.0f;
    float cpuUsagePeak = 0.0f;
    float cpuUsageEnd = 0.0f;

    // Success rates
    uint32 totalOperations = 0;
    uint32 successfulOperations = 0;
    uint32 failedOperations = 0;

    float GetSuccessRate() const
    {
        return totalOperations > 0 ? (float)successfulOperations / totalOperations : 1.0f;
    }
};

/**
 * @class MockPlayer
 * @brief Mock implementation of Player for testing
 */
class MockPlayer
{
public:
    MOCK_METHOD(ObjectGuid, GetGUID, (), (const));
    MOCK_METHOD(::std::string, GetName, (), (const));
    MOCK_METHOD(uint8, GetLevel, (), (const));
    MOCK_METHOD(uint8, GetRace, (), (const));
    MOCK_METHOD(uint8, GetClass, (), (const));
    MOCK_METHOD(Position, GetPosition, (), (const));
    MOCK_METHOD(bool, IsInGroup, (), (const));
    MOCK_METHOD(Group*, GetGroup, (), (const));
    MOCK_METHOD(bool, IsInCombat, (), (const));
    MOCK_METHOD(Unit*, GetTarget, (), (const));
    MOCK_METHOD(bool, IsWithinDistInMap, (const Position&, float), (const));
    MOCK_METHOD(void, SetPosition, (const Position&));
    MOCK_METHOD(void, TeleportTo, (uint32, const Position&));
    MOCK_METHOD(PlayerbotAI*, GetPlayerbotAI, (), (const));
    MOCK_METHOD(WorldSession*, GetSession, (), (const));
};

/**
 * @class MockGroup
 * @brief Mock implementation of Group for testing
 */
class MockGroup
{
public:
    MOCK_METHOD(ObjectGuid, GetGUID, (), (const));
    MOCK_METHOD(ObjectGuid, GetLeaderGUID, (), (const));
    MOCK_METHOD(Player*, GetLeader, (), (const));
    MOCK_METHOD(uint32, GetMembersCount, (), (const));
    MOCK_METHOD(bool, IsMember, (ObjectGuid), (const));
    MOCK_METHOD(bool, IsLeader, (ObjectGuid), (const));
    MOCK_METHOD(bool, AddMember, (Player*));
    MOCK_METHOD(bool, RemoveMember, (ObjectGuid));
    MOCK_METHOD(void, BroadcastPacket, (WorldPacket*, bool, int32));
    MOCK_METHOD(void, BroadcastGroupUpdate, ());
};

/**
 * @class MockWorldSession
 * @brief Mock implementation of WorldSession for testing
 */
class MockWorldSession
{
public:
    MOCK_METHOD(Player*, GetPlayer, (), (const));
    MOCK_METHOD(bool, IsBot, (), (const));
    MOCK_METHOD(void, SendPacket, (WorldPacket*));
    MOCK_METHOD(void, HandleGroupInviteOpcode, (WorldPacket&));
    MOCK_METHOD(void, HandleGroupAcceptOpcode, (WorldPacket&));
    MOCK_METHOD(void, HandleGroupDeclineOpcode, (WorldPacket&));
};

/**
 * @class TestEnvironment
 * @brief Provides test environment setup and utilities
 */
class TestEnvironment
{
public:
    static TestEnvironment* Instance();

    // Environment setup
    bool Initialize();
    void Cleanup();

    // Bot creation and management
    ::std::unique_ptr<BotTestData> CreateTestBot(const ::std::string& name, uint8 class_ = 1, uint8 level = 80);
    ::std::unique_ptr<GroupTestData> CreateTestGroup(const ::std::string& leaderName);
    bool AddBotToGroup(GroupTestData& group, const BotTestData& bot);
    bool RemoveBotFromGroup(GroupTestData& group, const ObjectGuid& botGuid);

    // Position utilities
    Position GetRandomPosition(const Position& center, float radius = 10.0f);
    Position GetFormationPosition(const Position& leaderPos, uint8 memberIndex, float distance = 5.0f);
    bool IsWithinFormationRange(const Position& member, const Position& leader, float maxDistance = 15.0f);

    // Time utilities
    uint32 GetCurrentTime() const;
    void AdvanceTime(uint32 milliseconds);

    // Performance monitoring
    void StartPerformanceMonitoring(const ::std::string& testName);
    void StopPerformanceMonitoring();
    PerformanceMetrics GetPerformanceMetrics() const;
    void ResetPerformanceMetrics();

    // Test data validation
    bool ValidateGroupFormation(const GroupTestData& group, float maxDistance = 15.0f);
    bool ValidateTargetAssistance(const GroupTestData& group, const ObjectGuid& expectedTarget);
    bool ValidateCombatEngagement(const GroupTestData& group);
    bool ValidatePerformanceThresholds(const PerformanceMetrics& metrics);

    // Stress testing utilities
    void SimulateHighLoad(uint32 operationsPerSecond, uint32 duration);
    void SimulateNetworkLatency(uint32 minLatency, uint32 maxLatency);
    void SimulateConcurrentGroups(uint32 groupCount, uint32 botsPerGroup);

    // Mock factory methods
    ::std::shared_ptr<MockPlayer> CreateMockPlayer(const BotTestData& data);
    ::std::shared_ptr<MockGroup> CreateMockGroup(const GroupTestData& data);
    ::std::shared_ptr<MockWorldSession> CreateMockSession(bool isBot = true);

private:
    TestEnvironment() = default;
    ~TestEnvironment() = default;

    static ::std::unique_ptr<TestEnvironment> s_instance;

    // Performance tracking
    ::std::string m_currentTestName;
    ::std::chrono::high_resolution_clock::time_point m_testStartTime;
    PerformanceMetrics m_currentMetrics;

    // Test state
    uint32 m_currentTime = 0;
    uint32 m_nextBotId = 1;
    uint32 m_nextGroupId = 1;

    // Helper methods
    uint64 GetMemoryUsage() const;
    float GetCpuUsage() const;
};

/**
 * @class PerformanceTimer
 * @brief RAII timer for measuring performance
 */
class PerformanceTimer
{
public:
    explicit PerformanceTimer(::std::function<void(uint64)> callback);
    ~PerformanceTimer();

    uint64 GetElapsedMicroseconds() const;
    void Cancel();

private:
    ::std::chrono::high_resolution_clock::time_point m_startTime;
    ::std::function<void(uint64)> m_callback;
    bool m_cancelled = false;
};

/**
 * @class GroupTestHelper
 * @brief Helper class for group functionality testing
 */
class GroupTestHelper
{
public:
    // Group creation scenarios
    static bool TestGroupCreation(const ::std::string& leaderName, const ::std::vector<::std::string>& memberNames);
    static bool TestGroupInvitation(const ::std::string& leaderName, const ::std::string& memberName);
    static bool TestGroupAcceptance(const ::std::string& memberName);

    // Following behavior scenarios
    static bool TestLeaderFollowing(const GroupTestData& group, const Position& destination);
    static bool TestFormationMaintenance(const GroupTestData& group, float maxDeviation = 2.0f);
    static bool TestTeleportBehavior(const GroupTestData& group, float teleportDistance = 100.0f);

    // Combat scenarios
    static bool TestCombatEngagement(const GroupTestData& group, const ObjectGuid& targetGuid);
    static bool TestTargetSwitching(const GroupTestData& group, const ObjectGuid& newTarget);
    static bool TestCombatFormation(const GroupTestData& group);

    // Edge case scenarios
    static bool TestLeaderDisconnect(const GroupTestData& group);
    static bool TestMemberDisconnect(const GroupTestData& group, const ObjectGuid& memberGuid);
    static bool TestMapTransition(const GroupTestData& group, uint32 newMapId);
    static bool TestGroupDisbanding(const GroupTestData& group);

    // Performance validation
    static bool ValidateResponseTimes(const PerformanceMetrics& metrics);
    static bool ValidateMemoryUsage(const PerformanceMetrics& metrics, uint32 botCount);
    static bool ValidateCpuUsage(const PerformanceMetrics& metrics);
    static bool ValidateSuccessRates(const PerformanceMetrics& metrics, float minSuccessRate = 0.95f);
};

/**
 * @class StressTestRunner
 * @brief Runs comprehensive stress tests
 */
class StressTestRunner
{
public:
    // Concurrent group testing
    bool RunConcurrentGroupTest(uint32 groupCount, uint32 botsPerGroup, uint32 durationSeconds);

    // High-frequency operation testing
    bool RunHighFrequencyTest(uint32 operationsPerSecond, uint32 durationSeconds);

    // Memory pressure testing
    bool RunMemoryStressTest(uint32 maxBots, uint32 incrementSize);

    // Network latency simulation
    bool RunLatencyStressTest(uint32 minLatency, uint32 maxLatency, uint32 durationSeconds);

    // Long-running stability test
    bool RunStabilityTest(uint32 groupCount, uint32 durationHours);

private:
    ::std::vector<::std::unique_ptr<GroupTestData>> m_activeGroups;
    PerformanceMetrics m_aggregatedMetrics;

    void CleanupActiveGroups();
    void AggregateMetrics(const PerformanceMetrics& metrics);
};

// Test assertion macros for common group functionality checks
#define EXPECT_GROUP_FORMATION_VALID(group, maxDistance) \
    EXPECT_TRUE(TestEnvironment::Instance()->ValidateGroupFormation(group, maxDistance)) \
    << "Group formation validation failed for group: " << group.leaderName

#define EXPECT_TARGET_ASSISTANCE_VALID(group, target) \
    EXPECT_TRUE(TestEnvironment::Instance()->ValidateTargetAssistance(group, target)) \
    << "Target assistance validation failed for group: " << group.leaderName

#define EXPECT_COMBAT_ENGAGEMENT_VALID(group) \
    EXPECT_TRUE(TestEnvironment::Instance()->ValidateCombatEngagement(group)) \
    << "Combat engagement validation failed for group: " << group.leaderName

#define EXPECT_PERFORMANCE_WITHIN_LIMITS(metrics) \
    EXPECT_TRUE(TestEnvironment::Instance()->ValidatePerformanceThresholds(metrics)) \
    << "Performance metrics exceeded acceptable thresholds"

// Timing assertion macros
#define EXPECT_TIMING_WITHIN_LIMIT(actualMicros, limitMicros, operation) \
    EXPECT_LE(actualMicros, limitMicros) \
    << operation << " took " << (actualMicros / 1000.0f) << "ms, expected <= " << (limitMicros / 1000.0f) << "ms"

#define EXPECT_SUCCESS_RATE_ABOVE(metrics, minRate) \
    EXPECT_GE(metrics.GetSuccessRate(), minRate) \
    << "Success rate " << metrics.GetSuccessRate() << " is below minimum " << minRate

/**
 * @class MockUnit
 * @brief Mock implementation of Unit for testing (enemy casters)
 */
class MockUnit
{
public:
    MOCK_METHOD(ObjectGuid, GetGUID, (), (const));
    MOCK_METHOD(::std::string, GetName, (), (const));
    MOCK_METHOD(Position, GetPosition, (), (const));
    MOCK_METHOD(bool, HasUnitState, (uint32), (const));
    MOCK_METHOD(bool, IsAlive, (), (const));
    MOCK_METHOD(bool, IsCasting, (), (const));
    MOCK_METHOD(uint32, GetCurrentSpellId, (uint8), (const));
    MOCK_METHOD(void*, GetCurrentSpell, (uint8), (const));
    MOCK_METHOD(uint64, GetHealth, (), (const));
    MOCK_METHOD(uint64, GetMaxHealth, (), (const));
    MOCK_METHOD(bool, IsWithinDist, (Position const&, float), (const));
    MOCK_METHOD(bool, IsWithinLOSInMap, (Position const&), (const));
    MOCK_METHOD(void, CastSpell, (Unit*, uint32, bool));
    MOCK_METHOD(void, InterruptSpell, (uint8, bool));
    MOCK_METHOD(bool, IsInCombat, (), (const));
    MOCK_METHOD(uint8, GetLevel, (), (const));
    MOCK_METHOD(Map*, GetMap, (), (const));
};

/**
 * @class MockBotAI
 * @brief Mock implementation of BotAI for testing
 */
class MockBotAI
{
public:
    MOCK_METHOD(Player*, GetBot, (), (const));
    MOCK_METHOD(bool, IsActive, (), (const));
    MOCK_METHOD(void, SetActive, (bool));
    MOCK_METHOD(bool, CanCastSpell, (uint32), (const));
    MOCK_METHOD(bool, CastSpell, (Unit*, uint32));
    MOCK_METHOD(void, OnEnemyCastStart, (Unit*, uint32, uint32));
    MOCK_METHOD(void, OnEnemyCastInterrupted, (ObjectGuid, uint32));
    MOCK_METHOD(bool, HasSpell, (uint32), (const));
    MOCK_METHOD(uint32, GetInterruptSpellId, (), (const));
};

/**
 * @struct CastAttempt
 * @brief Records a spell cast attempt for testing
 */
struct CastAttempt
{
    ObjectGuid casterGuid;
    ObjectGuid targetGuid;
    uint32 spellId;
    ::std::chrono::steady_clock::time_point timestamp;

    CastAttempt(ObjectGuid caster, ObjectGuid target, uint32 spell)
        : casterGuid(caster), targetGuid(target), spellId(spell),
          timestamp(::std::chrono::steady_clock::now())
    {}
};

/**
 * @class SpellPacketBuilderMock
 * @brief Mock implementation of SpellPacketBuilder for testing
 */
class SpellPacketBuilderMock
{
public:
    MOCK_METHOD(bool, CastSpell, (Player*, Unit*, uint32));
    MOCK_METHOD(bool, CastSpellOnTarget, (Player*, ObjectGuid, uint32));
    MOCK_METHOD(void, RecordCastAttempt, (ObjectGuid, ObjectGuid, uint32));
    MOCK_METHOD(::std::vector<CastAttempt>, GetCastHistory, ());
    MOCK_METHOD(void, ClearHistory, ());
    MOCK_METHOD(uint32, GetCastCount, ());
    MOCK_METHOD(bool, WasSpellCast, (uint32));
};

/**
 * @struct MovementRequest
 * @brief Records a movement request for testing
 */
struct MovementRequest
{
    ObjectGuid botGuid;
    Position destination;
    uint32 priority; // PlayerBotMovementPriority enum value
    ::std::chrono::steady_clock::time_point timestamp;

    MovementRequest(ObjectGuid bot, Position const& dest, uint32 prio)
        : botGuid(bot), destination(dest), priority(prio),
          timestamp(::std::chrono::steady_clock::now())
    {}
};

/**
 * @class MovementArbiterMock
 * @brief Mock implementation of MovementArbiter for testing
 */
class MovementArbiterMock
{
public:
    MOCK_METHOD(bool, RequestMovement, (Player*, Position const&, uint32));
    MOCK_METHOD(void, RecordMovementRequest, (ObjectGuid, Position const&, uint32));
    MOCK_METHOD(::std::vector<MovementRequest>, GetMovementHistory, ());
    MOCK_METHOD(void, ClearHistory, ());
    MOCK_METHOD(uint32, GetMovementCount, ());
    MOCK_METHOD(bool, HasMovementRequest, (ObjectGuid));
};

} // namespace Test
} // namespace Playerbot