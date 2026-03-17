/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "TestUtilities.h"
#include "PerformanceValidator.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <unordered_map>

namespace Playerbot
{
namespace Test
{

/**
 * @enum TestSeverity
 * @brief Defines the severity level of test categories
 */
enum class TestSeverity
{
    SMOKE,      // Quick smoke tests (< 1 minute)
    FUNCTIONAL, // Standard functional tests (< 5 minutes)
    STRESS,     // Stress and load tests (< 30 minutes)
    ENDURANCE,  // Long-running endurance tests (> 30 minutes)
    CRITICAL    // Critical path tests that must always pass
};

/**
 * @enum TestCategory
 * @brief Categorizes different types of tests
 */
enum class TestCategory
{
    UNIT,           // Unit tests for individual components
    INTEGRATION,    // Integration tests between systems
    PERFORMANCE,    // Performance and scalability tests
    STRESS,         // Stress and load tests
    EDGE_CASE,      // Edge case and error handling tests
    REGRESSION,     // Regression tests for known issues
    END_TO_END      // Complete end-to-end workflow tests
};

/**
 * @struct TestConfiguration
 * @brief Configuration for test execution
 */
struct TestConfiguration
{
    // Test selection
    ::std::vector<TestCategory> categoriesToRun = {
        TestCategory::UNIT,
        TestCategory::INTEGRATION,
        TestCategory::PERFORMANCE
    };
    ::std::vector<TestSeverity> severityLevels = {
        TestSeverity::SMOKE,
        TestSeverity::FUNCTIONAL
    };

    // Execution parameters
    uint32 maxConcurrentTests = 1;         // Number of tests to run in parallel
    uint32 testTimeoutSeconds = 300;       // 5 minute timeout per test
    uint32 maxRetries = 2;                 // Retry failed tests up to 2 times
    bool stopOnFirstFailure = false;       // Continue running tests after failures
    bool generateDetailedReports = true;   // Generate comprehensive reports

    // Performance thresholds
    PerformanceThresholds performanceThresholds;

    // Test environment
    bool useRealDatabase = false;          // Use real database vs mock
    bool cleanupAfterTests = true;         // Clean up test data
    ::std::string testDataPath = "./test_data/";
    ::std::string reportOutputPath = "./test_reports/";

    // Logging and output
    bool verboseLogging = false;
    bool generateJunitXml = false;
    bool generateCoverageReport = false;
    ::std::string logLevel = "INFO";

    // Load test specific settings
    uint32 maxBotsForLoadTest = 100;
    uint32 maxGroupsForStressTest = 20;
    uint32 stressTestDurationSeconds = 60;
};

/**
 * @struct TestResult
 * @brief Result of a single test execution
 */
struct TestResult
{
    ::std::string testName;
    ::std::string testSuite;
    TestCategory category;
    TestSeverity severity;

    bool passed = false;
    ::std::string failureReason;
    ::std::vector<::std::string> warnings;

    ::std::chrono::milliseconds executionTime{0};
    PerformanceMetrics performanceMetrics;

    // Detailed result data
    ::std::unordered_map<::std::string, ::std::string> additionalData;

    ::std::string GetFormattedResult() const;
    bool IsWithinPerformanceThresholds(const PerformanceThresholds& thresholds) const;
};

/**
 * @struct TestSuiteResult
 * @brief Aggregated results for a test suite
 */
struct TestSuiteResult
{
    ::std::string suiteName;
    TestCategory category;

    uint32 totalTests = 0;
    uint32 passedTests = 0;
    uint32 failedTests = 0;
    uint32 skippedTests = 0;

    ::std::chrono::milliseconds totalExecutionTime{0};
    ::std::vector<TestResult> testResults;
    PerformanceMetrics aggregatedMetrics;

    float GetSuccessRate() const { return totalTests > 0 ? (float)passedTests / totalTests : 0.0f; }
    bool AllTestsPassed() const { return failedTests == 0 && totalTests > 0; }
    ::std::string GenerateSummary() const;
};

/**
 * @struct TestRunResult
 * @brief Complete test run results
 */
struct TestRunResult
{
    ::std::string runId;
    ::std::chrono::system_clock::time_point startTime;
    ::std::chrono::system_clock::time_point endTime;
    TestConfiguration configuration;

    ::std::vector<TestSuiteResult> suiteResults;
    uint32 totalTests = 0;
    uint32 passedTests = 0;
    uint32 failedTests = 0;
    uint32 skippedTests = 0;

    float GetOverallSuccessRate() const { return totalTests > 0 ? (float)passedTests / totalTests : 0.0f; }
    ::std::chrono::milliseconds GetTotalExecutionTime() const;
    bool IsSuccessful() const { return failedTests == 0 && totalTests > 0; }
    ::std::string GenerateFullReport() const;
};

/**
 * @class TestRegistry
 * @brief Registry for test discovery and metadata
 */
class TestRegistry
{
public:
    struct TestInfo
    {
        ::std::string name;
        ::std::string description;
        TestCategory category;
        TestSeverity severity;
        ::std::vector<::std::string> dependencies;
        ::std::function<bool()> testFunction;
        uint32 expectedDurationSeconds;
    };

    static TestRegistry* Instance();

    // Test registration
    void RegisterTest(const TestInfo& testInfo);
    void RegisterTestSuite(const ::std::string& suiteName, TestCategory category, const ::std::vector<TestInfo>& tests);

    // Test discovery
    ::std::vector<TestInfo> GetTestsByCategory(TestCategory category) const;
    ::std::vector<TestInfo> GetTestsBySeverity(TestSeverity severity) const;
    ::std::vector<TestInfo> GetTestsByPattern(const ::std::string& pattern) const;
    ::std::vector<TestInfo> GetAllTests() const;

    // Test metadata
    TestInfo GetTestInfo(const ::std::string& testName) const;
    ::std::vector<::std::string> GetTestDependencies(const ::std::string& testName) const;
    bool IsTestRegistered(const ::std::string& testName) const;

    // Test suite management
    ::std::vector<::std::string> GetTestSuites() const;
    ::std::vector<TestInfo> GetTestsInSuite(const ::std::string& suiteName) const;

private:
    TestRegistry() = default;
    static ::std::unique_ptr<TestRegistry> s_instance;

    ::std::vector<TestInfo> m_registeredTests;
    ::std::unordered_map<::std::string, ::std::vector<TestInfo>> m_testSuites;
};

/**
 * @class AutomatedTestRunner
 * @brief Main test runner for automated execution
 */
class AutomatedTestRunner
{
public:
    explicit AutomatedTestRunner(const TestConfiguration& config = TestConfiguration{});
    ~AutomatedTestRunner() = default;

    // Configuration
    void SetConfiguration(const TestConfiguration& config);
    const TestConfiguration& GetConfiguration() const;

    // Test execution
    TestRunResult RunAllTests();
    TestRunResult RunTestsByCategory(TestCategory category);
    TestRunResult RunTestsBySeverity(TestSeverity severity);
    TestRunResult RunTestsByPattern(const ::std::string& pattern);
    TestRunResult RunSpecificTests(const ::std::vector<::std::string>& testNames);

    // Test suite execution
    TestSuiteResult RunTestSuite(const ::std::string& suiteName);

    // Validation and reporting
    bool ValidateTestEnvironment();
    void GenerateReports(const TestRunResult& results);
    void ExportResults(const TestRunResult& results, const ::std::string& format = "json");

    // Event callbacks
    void SetTestStartCallback(::std::function<void(const ::std::string&)> callback);
    void SetTestCompleteCallback(::std::function<void(const TestResult&)> callback);
    void SetSuiteCompleteCallback(::std::function<void(const TestSuiteResult&)> callback);

    // Progress monitoring
    void SetProgressCallback(::std::function<void(uint32, uint32)> callback); // (completed, total)

private:
    TestConfiguration m_config;
    ::std::unique_ptr<PerformanceValidator> m_performanceValidator;
    ::std::unique_ptr<TestEnvironment> m_testEnvironment;

    // Execution state
    bool m_running = false;
    ::std::string m_currentRunId;

    // Callbacks
    ::std::function<void(const ::std::string&)> m_testStartCallback;
    ::std::function<void(const TestResult&)> m_testCompleteCallback;
    ::std::function<void(const TestSuiteResult&)> m_suiteCompleteCallback;
    ::std::function<void(uint32, uint32)> m_progressCallback;

    // Internal execution methods
    TestResult ExecuteTest(const TestRegistry::TestInfo& testInfo);
    TestSuiteResult ExecuteTestSuite(const ::std::string& suiteName, const ::std::vector<TestRegistry::TestInfo>& tests);
    bool ShouldRunTest(const TestRegistry::TestInfo& testInfo) const;
    ::std::string GenerateRunId() const;

    // Result processing
    void ProcessTestResult(TestResult& result);
    void AggregateMetrics(TestSuiteResult& suiteResult);

    // Report generation
    void GenerateHtmlReport(const TestRunResult& results);
    void GenerateJsonReport(const TestRunResult& results);
    void GenerateJunitXmlReport(const TestRunResult& results);
    void GenerateCsvReport(const TestRunResult& results);

    // Utility methods
    void SetupTestEnvironment();
    void CleanupTestEnvironment();
    bool CheckDependencies(const TestRegistry::TestInfo& testInfo) const;
};

/**
 * @class ContinuousIntegrationRunner
 * @brief Specialized runner for CI/CD environments
 */
class ContinuousIntegrationRunner
{
public:
    struct CIConfiguration
    {
        bool failFastMode = true;               // Stop on first critical failure
        uint32 maxExecutionTimeMinutes = 60;   // Maximum CI run time
        bool requireMinimumCoverage = false;    // Require minimum test coverage
        float minimumCoveragePercent = 80.0f;   // Minimum coverage threshold
        bool generateArtifacts = true;          // Generate CI artifacts
        ::std::string artifactPath = "./ci_artifacts/";
        bool postResultsToWebhook = false;      // Post results to external service
        ::std::string webhookUrl;
    };

    explicit ContinuousIntegrationRunner(const CIConfiguration& config);

    // CI-specific execution
    int RunCIPipeline();  // Returns exit code for CI system
    bool RunSmokeTests(); // Quick validation tests
    bool RunRegressionTests(); // Full regression test suite

    // Artifact generation
    void GenerateArtifacts(const TestRunResult& results);
    void UploadArtifacts();

    // External integrations
    void PostResultsToWebhook(const TestRunResult& results);
    void UpdateTestStatusBadge(bool allPassed);

private:
    CIConfiguration m_ciConfig;
    ::std::unique_ptr<AutomatedTestRunner> m_testRunner;

    bool ValidateCIEnvironment();
    TestConfiguration CreateCITestConfiguration();
    void HandleCIFailure(const TestRunResult& results);
};

/**
 * @class TestScheduler
 * @brief Schedules and manages periodic test execution
 */
class TestScheduler
{
public:
    struct ScheduleConfig
    {
        bool enableNightlyRuns = true;
        ::std::string nightlyTime = "02:00";      // HH:MM format
        bool enableWeeklyStress = true;
        ::std::string weeklyDay = "Sunday";
        ::std::string weeklyTime = "04:00";
        bool enableContinuousSmoke = false;
        uint32 smokeTestIntervalMinutes = 30;
        ::std::string notificationEmail;
        bool sendFailureNotifications = true;
    };

    explicit TestScheduler(const ScheduleConfig& config);

    void Start();
    void Stop();
    bool IsRunning() const;

    // Manual triggers
    void TriggerNightlyRun();
    void TriggerStressTest();
    void TriggerSmokeTest();

private:
    ScheduleConfig m_config;
    bool m_running = false;
    ::std::unique_ptr<::std::thread> m_schedulerThread;

    void SchedulerLoop();
    void ExecuteScheduledTest(const ::std::string& testType);
    void SendNotification(const ::std::string& subject, const ::std::string& body);
};

// Utility macros for test registration
#define REGISTER_PLAYERBOT_TEST(name, category, severity, description) \
    namespace { \
        struct TestRegistrar_##name { \
            TestRegistrar_##name() { \
                TestRegistry::TestInfo info; \
                info.name = #name; \
                info.description = description; \
                info.category = category; \
                info.severity = severity; \
                info.testFunction = []() -> bool { \
                    /* Test implementation would go here */ \
                    return true; \
                }; \
                TestRegistry::Instance()->RegisterTest(info); \
            } \
        }; \
        static TestRegistrar_##name s_testRegistrar_##name; \
    }

#define REGISTER_PERFORMANCE_TEST(name, maxDurationMs, description) \
    REGISTER_PLAYERBOT_TEST(name, TestCategory::PERFORMANCE, TestSeverity::FUNCTIONAL, description)

#define REGISTER_STRESS_TEST(name, description) \
    REGISTER_PLAYERBOT_TEST(name, TestCategory::STRESS, TestSeverity::STRESS, description)

#define REGISTER_SMOKE_TEST(name, description) \
    REGISTER_PLAYERBOT_TEST(name, TestCategory::UNIT, TestSeverity::SMOKE, description)

} // namespace Test
} // namespace Playerbot