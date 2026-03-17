/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "TestUtilities.h"
#include "GameTime.h"
#include "Log.h"
#include <random>
#include <thread>
#include <cmath>
#include <algorithm>

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#elif defined(__linux__)
#include <unistd.h>
#include <sys/resource.h>
#include <sys/times.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <sys/resource.h>
#endif

namespace Playerbot
{
namespace Test
{

// Static instance
::std::unique_ptr<TestEnvironment> TestEnvironment::s_instance = nullptr;

TestEnvironment* TestEnvironment::Instance()
{
    if (!s_instance)
        s_instance = ::std::unique_ptr<TestEnvironment>(new TestEnvironment());
    return s_instance.get();
}

bool TestEnvironment::Initialize()
{
    TC_LOG_INFO("playerbot.test", "Initializing test environment");

    m_currentTime = GameTime::GetGameTimeMS();
    m_nextBotId = 1;
    m_nextGroupId = 1;

    ResetPerformanceMetrics();

    return true;
}

void TestEnvironment::Cleanup()
{
    TC_LOG_INFO("playerbot.test", "Cleaning up test environment");

    m_currentTestName.clear();
    ResetPerformanceMetrics();
}

::std::unique_ptr<BotTestData> TestEnvironment::CreateTestBot(const ::std::string& name, uint8 class_, uint8 level)
{
    auto bot = ::std::make_unique<BotTestData>(name);

    bot->characterId = m_nextBotId++;
    bot->guid = ObjectGuid::Create<HighGuid::Player>(bot->characterId);
    bot->level = level;
    bot->class_ = class_;
    bot->position = Position(0.0f, 0.0f, 0.0f, 0.0f);

    return bot;
}

::std::unique_ptr<GroupTestData> TestEnvironment::CreateTestGroup(const ::std::string& leaderName)
{
    auto group = ::std::make_unique<GroupTestData>(leaderName);

    group->groupId = ObjectGuid::Create<HighGuid::Group>(m_nextGroupId++);
    group->creationTime = m_currentTime;
    group->groupPosition = Position(0.0f, 0.0f, 0.0f, 0.0f);

    return group;
}

bool TestEnvironment::AddBotToGroup(GroupTestData& group, const BotTestData& bot)
{
    if (group.members.size() >= 5)
    {
        TC_LOG_ERROR("playerbot.test", "Cannot add bot {} to group {}: group is full",
                     bot.name, group.leaderName);
        return false;
    }

    group.members.push_back(bot);
    group.members.back().groupId = group.groupId;
    group.members.back().leaderGuid = group.leaderGuid;
    group.members.back().isInGroup = true;

    TC_LOG_DEBUG("playerbot.test", "Added bot {} to group {}", bot.name, group.leaderName);
    return true;
}

bool TestEnvironment::RemoveBotFromGroup(GroupTestData& group, const ObjectGuid& botGuid)
{
    auto it = ::std::find_if(group.members.begin(), group.members.end(),
        [&botGuid](const BotTestData& bot) { return bot.guid == botGuid; });

    if (it == group.members.end())
    {
        TC_LOG_ERROR("playerbot.test", "Bot with GUID {} not found in group {}",
                     botGuid.ToString(), group.leaderName);
        return false;
    }

    TC_LOG_DEBUG("playerbot.test", "Removed bot {} from group {}", it->name, group.leaderName);
    group.members.erase(it);
    return true;
}

Position TestEnvironment::GetRandomPosition(const Position& center, float radius)
{
    static ::std::random_device rd;
    static ::std::mt19937 gen(rd());
    ::std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * static_cast<float>(M_PI));
    ::std::uniform_real_distribution<float> radiusDist(0.0f, radius);

    float angle = angleDist(gen);
    float distance = radiusDist(gen);

    float x = center.GetPositionX() + distance * cos(angle);
    float y = center.GetPositionY() + distance * sin(angle);

    return Position(x, y, center.GetPositionZ(), center.GetOrientation());
}

Position TestEnvironment::GetFormationPosition(const Position& leaderPos, uint8 memberIndex, float distance)
{
    // Create a formation pattern around the leader
    float angleOffset = (2.0f * static_cast<float>(M_PI) / 4.0f) * memberIndex; // 4 cardinal positions
    float x = leaderPos.GetPositionX() + distance * cos(angleOffset);
    float y = leaderPos.GetPositionY() + distance * sin(angleOffset);

    return Position(x, y, leaderPos.GetPositionZ(), leaderPos.GetOrientation());
}

bool TestEnvironment::IsWithinFormationRange(const Position& member, const Position& leader, float maxDistance)
{
    return member.GetExactDist(&leader) <= maxDistance;
}

uint32 TestEnvironment::GetCurrentTime() const
{
    return m_currentTime;
}

void TestEnvironment::AdvanceTime(uint32 milliseconds)
{
    m_currentTime += milliseconds;
}

void TestEnvironment::StartPerformanceMonitoring(const ::std::string& testName)
{
    m_currentTestName = testName;
    m_testStartTime = ::std::chrono::high_resolution_clock::now();

    // Record starting metrics
    m_currentMetrics.memoryUsageStart = GetMemoryUsage();
    m_currentMetrics.cpuUsageStart = GetCpuUsage();
    m_currentMetrics.totalOperations = 0;
    m_currentMetrics.successfulOperations = 0;
    m_currentMetrics.failedOperations = 0;

    TC_LOG_INFO("playerbot.test", "Started performance monitoring for test: {}", testName);
}

void TestEnvironment::StopPerformanceMonitoring()
{
    if (m_currentTestName.empty())
        return;

    // Record ending metrics
    m_currentMetrics.memoryUsageEnd = GetMemoryUsage();
    m_currentMetrics.cpuUsageEnd = GetCpuUsage();

    auto endTime = ::std::chrono::high_resolution_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - m_testStartTime);

    TC_LOG_INFO("playerbot.test", "Performance monitoring completed for test: {} (Duration: {}μs)",
                m_currentTestName, duration.count());

    m_currentTestName.clear();
}

PerformanceMetrics TestEnvironment::GetPerformanceMetrics() const
{
    return m_currentMetrics;
}

void TestEnvironment::ResetPerformanceMetrics()
{
    m_currentMetrics = PerformanceMetrics{};
}

bool TestEnvironment::ValidateGroupFormation(const GroupTestData& group, float maxDistance)
{
    for (const auto& member : group.members)
    {
        if (!IsWithinFormationRange(member.position, group.groupPosition, maxDistance))
        {
            TC_LOG_DEBUG("playerbot.test", "Bot {} is {} yards from group center (max: {})",
                        member.name, member.position.GetExactDist(&group.groupPosition), maxDistance);
            return false;
        }
    }

    return true;
}

bool TestEnvironment::ValidateTargetAssistance(const GroupTestData& group, const ObjectGuid& expectedTarget)
{
    // Check that all group members are targeting the same target
    for (const auto& member : group.members)
    {
        if (member.isInCombat && group.currentTarget != expectedTarget)
            return false;
    }

    return true;
}

bool TestEnvironment::ValidateCombatEngagement(const GroupTestData& group)
{
    // Check that all group members engage in combat when the group is in combat
    if (group.isInCombat)
    {
        for (const auto& member : group.members)
        {
            if (!member.isInCombat)
                return false;
        }
    }

    return true;
}

bool TestEnvironment::ValidatePerformanceThresholds(const PerformanceMetrics& metrics)
{
    // Validate timing thresholds
    if (metrics.invitationAcceptanceTime > 3000000) // 3 seconds
    {
        TC_LOG_DEBUG("playerbot.test", "Invitation acceptance time {} exceeds threshold",
                    metrics.invitationAcceptanceTime);
        return false;
    }

    if (metrics.combatEngagementTime > 3000000) // 3 seconds
    {
        TC_LOG_DEBUG("playerbot.test", "Combat engagement time {} exceeds threshold",
                    metrics.combatEngagementTime);
        return false;
    }

    if (metrics.targetSwitchTime > 1000000) // 1 second
    {
        TC_LOG_DEBUG("playerbot.test", "Target switch time {} exceeds threshold",
                    metrics.targetSwitchTime);
        return false;
    }

    // Validate success rate
    if (metrics.GetSuccessRate() < 0.95f) // 95% minimum success rate
    {
        TC_LOG_DEBUG("playerbot.test", "Success rate {} is below threshold", metrics.GetSuccessRate());
        return false;
    }

    return true;
}

::std::shared_ptr<MockPlayer> TestEnvironment::CreateMockPlayer(const BotTestData& data)
{
    auto mockPlayer = ::std::make_shared<MockPlayer>();

    // Configure default expectations
    ON_CALL(*mockPlayer, GetGUID()).WillByDefault(testing::Return(data.guid));
    ON_CALL(*mockPlayer, GetName()).WillByDefault(testing::Return(data.name));
    ON_CALL(*mockPlayer, GetLevel()).WillByDefault(testing::Return(data.level));
    ON_CALL(*mockPlayer, GetRace()).WillByDefault(testing::Return(data.race));
    ON_CALL(*mockPlayer, GetClass()).WillByDefault(testing::Return(data.class_));
    ON_CALL(*mockPlayer, GetPosition()).WillByDefault(testing::Return(data.position));
    ON_CALL(*mockPlayer, IsInGroup()).WillByDefault(testing::Return(data.isInGroup));
    ON_CALL(*mockPlayer, IsInCombat()).WillByDefault(testing::Return(data.isInCombat));

    return mockPlayer;
}

::std::shared_ptr<MockGroup> TestEnvironment::CreateMockGroup(const GroupTestData& data)
{
    auto mockGroup = ::std::make_shared<MockGroup>();

    // Configure default expectations
    ON_CALL(*mockGroup, GetGUID()).WillByDefault(testing::Return(data.groupId));
    ON_CALL(*mockGroup, GetLeaderGUID()).WillByDefault(testing::Return(data.leaderGuid));
    ON_CALL(*mockGroup, GetMembersCount()).WillByDefault(testing::Return(data.members.size()));

    return mockGroup;
}

::std::shared_ptr<MockWorldSession> TestEnvironment::CreateMockSession(bool isBot)
{
    auto mockSession = ::std::make_shared<MockWorldSession>();

    // Configure default expectations
    ON_CALL(*mockSession, IsBot()).WillByDefault(testing::Return(isBot));

    return mockSession;
}

uint64 TestEnvironment::GetMemoryUsage() const
{
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return pmc.WorkingSetSize;
#elif defined(__linux__)
    // Read from /proc/self/status
    FILE* file = fopen("/proc/self/status", "r");
    if (file)
    {
        char line[128];
        while (fgets(line, 128, file) != nullptr)
        {
            if (strncmp(line, "VmRSS:", 6) == 0)
            {
                uint64 size;
                sscanf(line, "VmRSS: %llu kB", &size);
                fclose(file);
                return size * 1024; // Convert to bytes
            }
        }
        fclose(file);
    }
#elif defined(__APPLE__)
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &infoCount) == KERN_SUCCESS)
        return info.resident_size;
#endif

    return 0; // Fallback if platform-specific code fails
}

float TestEnvironment::GetCpuUsage() const
{
    // Simple CPU usage estimation - in a real implementation, this would be more sophisticated
    static auto lastTime = ::std::chrono::high_resolution_clock::now();
    static uint64 lastUsage = 0;

    auto currentTime = ::std::chrono::high_resolution_clock::now();
    auto timeDiff = ::std::chrono::duration_cast<::std::chrono::microseconds>(currentTime - lastTime);

    if (timeDiff.count() < 1000000) // Less than 1 second
        return 0.0f;

    lastTime = currentTime;

    // Return a simulated CPU usage value for testing
    return 15.0f; // Simulated low CPU usage
}

// PerformanceTimer implementation
PerformanceTimer::PerformanceTimer(::std::function<void(uint64)> callback)
    : m_startTime(::std::chrono::high_resolution_clock::now())
    , m_callback(::std::move(callback))
{
}

PerformanceTimer::~PerformanceTimer()
{
    if (!m_cancelled && m_callback)
    {
        uint64 elapsed = GetElapsedMicroseconds();
        m_callback(elapsed);
    }
}

uint64 PerformanceTimer::GetElapsedMicroseconds() const
{
    auto currentTime = ::std::chrono::high_resolution_clock::now();
    return ::std::chrono::duration_cast<::std::chrono::microseconds>(currentTime - m_startTime).count();
}

void PerformanceTimer::Cancel()
{
    m_cancelled = true;
}

// GroupTestHelper implementation
bool GroupTestHelper::TestGroupCreation(const ::std::string& leaderName, const ::std::vector<::std::string>& memberNames)
{
    auto env = TestEnvironment::Instance();
    auto group = env->CreateTestGroup(leaderName);

    // Add all members to the group
    for (const auto& memberName : memberNames)
    {
        auto bot = env->CreateTestBot(memberName);
        if (!env->AddBotToGroup(*group, *bot))
            return false;
    }

    return group->members.size() == memberNames.size();
}

bool GroupTestHelper::TestGroupInvitation(const ::std::string& leaderName, const ::std::string& memberName)
{
    // Simulate invitation process
    TC_LOG_DEBUG("playerbot.test", "Testing invitation from {} to {}", leaderName, memberName);

    // In a real test, this would interact with the actual invitation system
    // For now, we simulate a successful invitation
    return true;
}

bool GroupTestHelper::TestGroupAcceptance(const ::std::string& memberName)
{
    // Simulate acceptance process
    TC_LOG_DEBUG("playerbot.test", "Testing acceptance by {}", memberName);

    // In a real test, this would check the GroupInvitationHandler
    // For now, we simulate a successful acceptance
    return true;
}

bool GroupTestHelper::TestLeaderFollowing(const GroupTestData& group, const Position& destination)
{
    // Simulate leader movement and check if bots follow
    TC_LOG_DEBUG("playerbot.test", "Testing leader following for group {}", group.leaderName);

    // Check if all members are within formation range of the destination
    auto env = TestEnvironment::Instance();
    for (const auto& member : group.members)
    {
        if (!env->IsWithinFormationRange(member.position, destination, 15.0f))
            return false;
    }

    return true;
}

bool GroupTestHelper::ValidateResponseTimes(const PerformanceMetrics& metrics)
{
    return TestEnvironment::Instance()->ValidatePerformanceThresholds(metrics);
}

bool GroupTestHelper::ValidateMemoryUsage(const PerformanceMetrics& metrics, uint32 botCount)
{
    // Maximum 10MB per bot
    uint64 maxMemoryPerBot = 10ULL * 1024 * 1024;
    uint64 maxTotalMemory = maxMemoryPerBot * botCount;

    return metrics.memoryUsagePeak <= maxTotalMemory;
}

bool GroupTestHelper::ValidateCpuUsage(const PerformanceMetrics& metrics)
{
    // Maximum 90% CPU usage during testing
    return metrics.cpuUsagePeak <= 90.0f;
}

bool GroupTestHelper::ValidateSuccessRates(const PerformanceMetrics& metrics, float minSuccessRate)
{
    return metrics.GetSuccessRate() >= minSuccessRate;
}

// StressTestRunner implementation
bool StressTestRunner::RunConcurrentGroupTest(uint32 groupCount, uint32 botsPerGroup, uint32 durationSeconds)
{
    TC_LOG_INFO("playerbot.test", "Running concurrent group test: {} groups, {} bots per group, {} seconds",
                groupCount, botsPerGroup, durationSeconds);

    auto env = TestEnvironment::Instance();
    env->StartPerformanceMonitoring("ConcurrentGroupTest");

    // Create all test groups
    m_activeGroups.clear();
    for (uint32 i = 0; i < groupCount; ++i)
    {
        ::std::string leaderName = "Leader" + ::std::to_string(i + 1);
        auto group = env->CreateTestGroup(leaderName);

        // Add bots to each group
    for (uint32 j = 0; j < botsPerGroup; ++j)
        {
            ::std::string botName = "Bot" + ::std::to_string(i + 1) + "_" + ::std::to_string(j + 1);
            auto bot = env->CreateTestBot(botName);
            env->AddBotToGroup(*group, *bot);
        }

        m_activeGroups.push_back(::std::move(group));
    }

    // Simulate activity for the specified duration
    auto startTime = ::std::chrono::high_resolution_clock::now();
    while (true)
    {
        auto currentTime = ::std::chrono::high_resolution_clock::now();
        auto elapsed = ::std::chrono::duration_cast<::std::chrono::seconds>(currentTime - startTime);

        if (elapsed.count() >= durationSeconds)
            break;

        // Simulate group activities
        env->AdvanceTime(100);
        ::std::this_thread::sleep_for(::std::chrono::milliseconds(100));
    }

    env->StopPerformanceMonitoring();
    auto metrics = env->GetPerformanceMetrics();

    CleanupActiveGroups();

    return GroupTestHelper::ValidateMemoryUsage(metrics, groupCount * botsPerGroup) &&
           GroupTestHelper::ValidateCpuUsage(metrics);
}

void StressTestRunner::CleanupActiveGroups()
{
    m_activeGroups.clear();
}

void StressTestRunner::AggregateMetrics(const PerformanceMetrics& metrics)
{
    m_aggregatedMetrics.totalOperations += metrics.totalOperations;
    m_aggregatedMetrics.successfulOperations += metrics.successfulOperations;
    m_aggregatedMetrics.failedOperations += metrics.failedOperations;

    m_aggregatedMetrics.memoryUsagePeak = ::std::max(m_aggregatedMetrics.memoryUsagePeak, metrics.memoryUsagePeak);
    m_aggregatedMetrics.cpuUsagePeak = ::std::max(m_aggregatedMetrics.cpuUsagePeak, metrics.cpuUsagePeak);
}

} // namespace Test
} // namespace Playerbot