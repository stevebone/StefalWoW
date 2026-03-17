/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "AutomatedTestRunner.h"
#include "Log.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <regex>
#include <chrono>
#include <filesystem>
#include <thread>

namespace Playerbot
{
namespace Test
{

// ========================
// TestResult Implementation
// ========================

::std::string TestResult::GetFormattedResult() const
{
    ::std::stringstream ss;
    ss << ::std::fixed << ::std::setprecision(2);

    ss << "[" << (passed ? "PASS" : "FAIL") << "] " << testName;
    ss << " (" << executionTime.count() << "ms)";

    if (!passed && !failureReason.empty())
        ss << " - " << failureReason;

    if (!warnings.empty())
        ss << " [" << warnings.size() << " warnings]";

    return ss.str();
}

bool TestResult::IsWithinPerformanceThresholds(const PerformanceThresholds& thresholds) const
{
    PerformanceValidator validator(thresholds);
    return validator.ValidateAllMetrics(performanceMetrics);
}

// ========================
// TestSuiteResult Implementation
// ========================

::std::string TestSuiteResult::GenerateSummary() const
{
    ::std::stringstream ss;
    ss << suiteName << ": " << passedTests << "/" << totalTests << " passed";
    ss << " (" << ::std::fixed << ::std::setprecision(1) << (GetSuccessRate() * 100.0f) << "%)";
    ss << " in " << totalExecutionTime.count() << "ms";

    if (failedTests > 0)
        ss << " [" << failedTests << " failed]";
    if (skippedTests > 0)
        ss << " [" << skippedTests << " skipped]";

    return ss.str();
}

// ========================
// TestRunResult Implementation
// ========================

::std::chrono::milliseconds TestRunResult::GetTotalExecutionTime() const
{
    return ::std::chrono::duration_cast<::std::chrono::milliseconds>(endTime - startTime);
}

::std::string TestRunResult::GenerateFullReport() const
{
    ::std::stringstream report;
    report << ::std::fixed << ::std::setprecision(2);

    // Header
    report << "====================================\n";
    report << "PLAYERBOT GROUP FUNCTIONALITY TEST REPORT\n";
    report << "====================================\n\n";

    // Summary
    report << "Run ID: " << runId << "\n";
    report << "Start Time: " << ::std::put_time(::std::localtime(&::std::chrono::system_clock::to_time_t(startTime)), "%Y-%m-%d %H:%M:%S") << "\n";
    report << "End Time: " << ::std::put_time(::std::localtime(&::std::chrono::system_clock::to_time_t(endTime)), "%Y-%m-%d %H:%M:%S") << "\n";
    report << "Total Duration: " << GetTotalExecutionTime().count() << "ms\n\n";

    // Overall results
    report << "OVERALL RESULTS:\n";
    report << "  Total Tests: " << totalTests << "\n";
    report << "  Passed: " << passedTests << "\n";
    report << "  Failed: " << failedTests << "\n";
    report << "  Skipped: " << skippedTests << "\n";
    report << "  Success Rate: " << (GetOverallSuccessRate() * 100.0f) << "%\n";
    report << "  Status: " << (IsSuccessful() ? "SUCCESS" : "FAILURE") << "\n\n";

    // Suite breakdown
    report << "TEST SUITE BREAKDOWN:\n";
    for (const auto& suite : suiteResults)
    {
        report << "  " << suite.GenerateSummary() << "\n";

        // Show failed tests
    for (const auto& test : suite.testResults)
        {
            if (!test.passed)
            {
                report << "    FAILED: " << test.testName;
                if (!test.failureReason.empty())
                    report << " - " << test.failureReason;
                report << "\n";
            }
        }
    }

    report << "\n";

    // Performance summary
    report << "PERFORMANCE SUMMARY:\n";
    for (const auto& suite : suiteResults)
    {
        if (suite.aggregatedMetrics.totalOperations > 0)
        {
            const auto& metrics = suite.aggregatedMetrics;
            report << "  " << suite.suiteName << ":\n";
            report << "    Total Operations: " << metrics.totalOperations << "\n";
            report << "    Success Rate: " << (metrics.GetSuccessRate() * 100.0f) << "%\n";
            report << "    Peak Memory: " << (metrics.memoryUsagePeak / (1024 * 1024)) << " MB\n";
            report << "    Peak CPU: " << metrics.cpuUsagePeak << "%\n";

            if (metrics.invitationAcceptanceTime > 0)
                report << "    Avg Invitation Time: " << (metrics.invitationAcceptanceTime / 1000.0f) << " ms\n";
            if (metrics.combatEngagementTime > 0)
                report << "    Avg Combat Engagement: " << (metrics.combatEngagementTime / 1000.0f) << " ms\n";
            if (metrics.targetSwitchTime > 0)
                report << "    Avg Target Switch: " << (metrics.targetSwitchTime / 1000.0f) << " ms\n";
        }
    }

    return report.str();
}

// ========================
// TestRegistry Implementation
// ========================

::std::unique_ptr<TestRegistry> TestRegistry::s_instance = nullptr;

TestRegistry* TestRegistry::Instance()
{
    if (!s_instance)
        s_instance = ::std::unique_ptr<TestRegistry>(new TestRegistry());
    return s_instance.get();
}

void TestRegistry::RegisterTest(const TestInfo& testInfo)
{
    m_registeredTests.push_back(testInfo);
    TC_LOG_DEBUG("playerbot.test", "Registered test: {} (Category: {}, Severity: {})",
                testInfo.name, static_cast<int>(testInfo.category), static_cast<int>(testInfo.severity));
}

void TestRegistry::RegisterTestSuite(const ::std::string& suiteName, TestCategory category, const ::std::vector<TestInfo>& tests)
{
    m_testSuites[suiteName] = tests;

    // Also register individual tests
    for (const auto& test : tests)
    {
        RegisterTest(test);
    }

    TC_LOG_INFO("playerbot.test", "Registered test suite: {} with {} tests", suiteName, tests.size());
}

::std::vector<TestRegistry::TestInfo> TestRegistry::GetTestsByCategory(TestCategory category) const
{
    ::std::vector<TestInfo> result;
    ::std::copy_if(m_registeredTests.begin(), m_registeredTests.end(),
                 ::std::back_inserter(result),
                 [category](const TestInfo& test) { return test.category == category; });
    return result;
}

::std::vector<TestRegistry::TestInfo> TestRegistry::GetTestsBySeverity(TestSeverity severity) const
{
    ::std::vector<TestInfo> result;
    ::std::copy_if(m_registeredTests.begin(), m_registeredTests.end(),
                 ::std::back_inserter(result),
                 [severity](const TestInfo& test) { return test.severity == severity; });
    return result;
}

::std::vector<TestRegistry::TestInfo> TestRegistry::GetTestsByPattern(const ::std::string& pattern) const
{
    ::std::vector<TestInfo> result;
    ::std::regex regex(pattern, ::std::regex_constants::icase);

    ::std::copy_if(m_registeredTests.begin(), m_registeredTests.end(),
                 ::std::back_inserter(result),
                 [&regex](const TestInfo& test) { return ::std::regex_search(test.name, regex); });
    return result;
}

::std::vector<TestRegistry::TestInfo> TestRegistry::GetAllTests() const
{
    return m_registeredTests;
}

// ========================
// AutomatedTestRunner Implementation
// ========================

AutomatedTestRunner::AutomatedTestRunner(const TestConfiguration& config)
    : m_config(config)
    , m_performanceValidator(::std::make_unique<PerformanceValidator>(config.performanceThresholds))
    , m_testEnvironment(TestEnvironment::Instance())
{
}

void AutomatedTestRunner::SetConfiguration(const TestConfiguration& config)
{
    m_config = config;
    m_performanceValidator->SetThresholds(config.performanceThresholds);
}

const TestConfiguration& AutomatedTestRunner::GetConfiguration() const
{
    return m_config;
}

TestRunResult AutomatedTestRunner::RunAllTests()
{
    TC_LOG_INFO("playerbot.test", "Starting comprehensive test run");

    TestRunResult result;
    result.runId = GenerateRunId();
    result.startTime = ::std::chrono::system_clock::now();
    result.configuration = m_config;

    SetupTestEnvironment();

    // Get all tests to run
    auto allTests = TestRegistry::Instance()->GetAllTests();
    ::std::vector<TestRegistry::TestInfo> testsToRun;

    // Filter tests based on configuration
    for (const auto& test : allTests)
    {
        if (ShouldRunTest(test))
            testsToRun.push_back(test);
    }

    TC_LOG_INFO("playerbot.test", "Running {} tests across {} categories",
                testsToRun.size(), m_config.categoriesToRun.size());

    // Group tests by suite and execute
    ::std::unordered_map<::std::string, ::std::vector<TestRegistry::TestInfo>> suiteTests;

    for (const auto& test : testsToRun)
    {
        ::std::string suiteName = "Default";
        // Extract suite name from test name (e.g., "GroupFunctionalityTests::TestName" -> "GroupFunctionalityTests")
        size_t pos = test.name.find("::");
        if (pos != ::std::string::npos)
            suiteName = test.name.substr(0, pos);

        suiteTests[suiteName].push_back(test);
    }

    // Execute test suites
    uint32 completedTests = 0;
    for (const auto& [suiteName, tests] : suiteTests)
    {
        TestSuiteResult suiteResult = ExecuteTestSuite(suiteName, tests);
        result.suiteResults.push_back(suiteResult);

        // Update progress
        completedTests += tests.size();
        if (m_progressCallback)
            m_progressCallback(completedTests, testsToRun.size());

        // Check for early termination
    if (m_config.stopOnFirstFailure && !suiteResult.AllTestsPassed())
        {
            TC_LOG_WARN("playerbot.test", "Stopping test execution due to failures in suite: {}", suiteName);
            break;
        }
    }

    // Aggregate results
    for (const auto& suite : result.suiteResults)
    {
        result.totalTests += suite.totalTests;
        result.passedTests += suite.passedTests;
        result.failedTests += suite.failedTests;
        result.skippedTests += suite.skippedTests;
    }

    result.endTime = ::std::chrono::system_clock::now();

    CleanupTestEnvironment();

    TC_LOG_INFO("playerbot.test", "Test run completed: {}/{} tests passed ({}%)",
                result.passedTests, result.totalTests, result.GetOverallSuccessRate() * 100.0f);

    if (m_config.generateDetailedReports)
        GenerateReports(result);

    return result;
}

TestRunResult AutomatedTestRunner::RunTestsByCategory(TestCategory category)
{
    auto tests = TestRegistry::Instance()->GetTestsByCategory(category);
    return RunSpecificTests([&tests]() {
        ::std::vector<::std::string> testNames;
        for (const auto& test : tests)
            testNames.push_back(test.name);
        return testNames;
    }());
}

TestResult AutomatedTestRunner::ExecuteTest(const TestRegistry::TestInfo& testInfo)
{
    TestResult result;
    result.testName = testInfo.name;
    result.category = testInfo.category;
    result.severity = testInfo.severity;

    TC_LOG_DEBUG("playerbot.test", "Executing test: {}", testInfo.name);

    if (m_testStartCallback)
        m_testStartCallback(testInfo.name);

    auto startTime = ::std::chrono::high_resolution_clock::now();

    try
    {
        // Check dependencies first
    if (!CheckDependencies(testInfo))
        {
            result.passed = false;
            result.failureReason = "Dependencies not satisfied";
            TC_LOG_WARN("playerbot.test", "Test {} skipped due to unmet dependencies", testInfo.name);
            return result;
        }

        // Execute the test with timeout
        ::std::atomic<bool> testCompleted{false};
        ::std::atomic<bool> testResult{false};
        ::std::string errorMessage;

        ::std::thread testThread([&]() {
            try
            {
                testResult = testInfo.testFunction();
                testCompleted = true;
            }
            catch (const ::std::exception& e)
            {
                errorMessage = e.what();
                testCompleted = true;
                testResult = false;
            }
        });

        // Wait for completion with timeout
        auto timeout = ::std::chrono::seconds(m_config.testTimeoutSeconds);
        auto timeoutEnd = startTime + timeout;

        while (!testCompleted && ::std::chrono::high_resolution_clock::now() < timeoutEnd)
        {
            ::std::this_thread::sleep_for(::std::chrono::milliseconds(100));
        }

        if (!testCompleted)
        {
            // Test timed out
            result.passed = false;
            result.failureReason = "Test timed out after " + ::std::to_string(m_config.testTimeoutSeconds) + " seconds";
            TC_LOG_ERROR("playerbot.test", "Test {} timed out", testInfo.name);

            // Force terminate the test thread (not recommended but necessary)
            testThread.detach();
        }
        else
        {
            testThread.join();
            result.passed = testResult;
            if (!errorMessage.empty())
                result.failureReason = errorMessage;
        }
    }
    catch (const ::std::exception& e)
    {
        result.passed = false;
        result.failureReason = ::std::string("Exception: ") + e.what();
        TC_LOG_ERROR("playerbot.test", "Test {} threw exception: {}", testInfo.name, e.what());
    }

    auto endTime = ::std::chrono::high_resolution_clock::now();
    result.executionTime = ::std::chrono::duration_cast<::std::chrono::milliseconds>(endTime - startTime);

    ProcessTestResult(result);

    if (m_testCompleteCallback)
        m_testCompleteCallback(result);

    TC_LOG_DEBUG("playerbot.test", "Test {} completed: {} ({}ms)",
                testInfo.name, result.passed ? "PASSED" : "FAILED", result.executionTime.count());

    return result;
}

TestSuiteResult AutomatedTestRunner::ExecuteTestSuite(const ::std::string& suiteName, const ::std::vector<TestRegistry::TestInfo>& tests)
{
    TestSuiteResult suiteResult;
    suiteResult.suiteName = suiteName;
    suiteResult.category = tests.empty() ? TestCategory::UNIT : tests[0].category;

    TC_LOG_INFO("playerbot.test", "Executing test suite: {} ({} tests)", suiteName, tests.size());

    auto suiteStartTime = ::std::chrono::high_resolution_clock::now();

    for (const auto& testInfo : tests)
    {
        TestResult testResult = ExecuteTest(testInfo);
        suiteResult.testResults.push_back(testResult);

        suiteResult.totalTests++;
        if (testResult.passed)
            suiteResult.passedTests++;
        else
            suiteResult.failedTests++;

        suiteResult.totalExecutionTime += testResult.executionTime;

        // Early termination for suite if configured
    if (m_config.stopOnFirstFailure && !testResult.passed)
            break;
    }

    auto suiteEndTime = ::std::chrono::high_resolution_clock::now();
    suiteResult.totalExecutionTime = ::std::chrono::duration_cast<::std::chrono::milliseconds>(suiteEndTime - suiteStartTime);

    AggregateMetrics(suiteResult);

    if (m_suiteCompleteCallback)
        m_suiteCompleteCallback(suiteResult);

    TC_LOG_INFO("playerbot.test", "Test suite {} completed: {}/{} passed",
                suiteName, suiteResult.passedTests, suiteResult.totalTests);

    return suiteResult;
}

bool AutomatedTestRunner::ShouldRunTest(const TestRegistry::TestInfo& testInfo) const
{
    // Check category filter
    bool categoryMatch = ::std::find(m_config.categoriesToRun.begin(), m_config.categoriesToRun.end(),
                                   testInfo.category) != m_config.categoriesToRun.end();
    if (!categoryMatch)
        return false;

    // Check severity filter
    bool severityMatch = ::std::find(m_config.severityLevels.begin(), m_config.severityLevels.end(),
                                   testInfo.severity) != m_config.severityLevels.end();
    if (!severityMatch)
        return false;

    return true;
}

void AutomatedTestRunner::ProcessTestResult(TestResult& result)
{
    // Add performance validation
    if (result.passed)
    {
        bool performanceValid = result.IsWithinPerformanceThresholds(m_config.performanceThresholds);
        if (!performanceValid)
        {
            result.warnings.push_back("Performance thresholds exceeded");
            TC_LOG_WARN("playerbot.test", "Test {} exceeded performance thresholds", result.testName);
        }
    }

    // Add additional metadata
    result.additionalData["execution_timestamp"] = ::std::to_string(::std::chrono::system_clock::now().time_since_epoch().count());
    result.additionalData["test_environment"] = "automated";
}

void AutomatedTestRunner::AggregateMetrics(TestSuiteResult& suiteResult)
{
    // Aggregate performance metrics from all tests in the suite
    for (const auto& testResult : suiteResult.testResults)
    {
        const auto& metrics = testResult.performanceMetrics;

        suiteResult.aggregatedMetrics.totalOperations += metrics.totalOperations;
        suiteResult.aggregatedMetrics.successfulOperations += metrics.successfulOperations;
        suiteResult.aggregatedMetrics.failedOperations += metrics.failedOperations;

        // Take maximum values for timing metrics
        suiteResult.aggregatedMetrics.invitationAcceptanceTime = ::std::max(
            suiteResult.aggregatedMetrics.invitationAcceptanceTime, metrics.invitationAcceptanceTime);
        suiteResult.aggregatedMetrics.combatEngagementTime = ::std::max(
            suiteResult.aggregatedMetrics.combatEngagementTime, metrics.combatEngagementTime);
        suiteResult.aggregatedMetrics.targetSwitchTime = ::std::max(
            suiteResult.aggregatedMetrics.targetSwitchTime, metrics.targetSwitchTime);

        // Take maximum values for resource metrics
        suiteResult.aggregatedMetrics.memoryUsagePeak = ::std::max(
            suiteResult.aggregatedMetrics.memoryUsagePeak, metrics.memoryUsagePeak);
        suiteResult.aggregatedMetrics.cpuUsagePeak = ::std::max(
            suiteResult.aggregatedMetrics.cpuUsagePeak, metrics.cpuUsagePeak);
    }
}

::std::string AutomatedTestRunner::GenerateRunId() const
{
    auto now = ::std::chrono::system_clock::now();
    auto time_t = ::std::chrono::system_clock::to_time_t(now);
    auto ms = ::std::chrono::duration_cast<::std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    ::std::stringstream ss;
    ss << "TestRun_" << ::std::put_time(::std::gmtime(&time_t), "%Y%m%d_%H%M%S");
    ss << "_" << ::std::setfill('0') << ::std::setw(3) << ms.count();

    return ss.str();
}

void AutomatedTestRunner::SetupTestEnvironment()
{
    if (m_testEnvironment)
    {
        m_testEnvironment->Initialize();
    }

    // Create output directories
    ::std::filesystem::create_directories(m_config.reportOutputPath);
    ::std::filesystem::create_directories(m_config.testDataPath);
}

void AutomatedTestRunner::CleanupTestEnvironment()
{
    if (m_testEnvironment && m_config.cleanupAfterTests)
    {
        m_testEnvironment->Cleanup();
    }
}

bool AutomatedTestRunner::CheckDependencies(const TestRegistry::TestInfo& testInfo) const
{
    // Simple dependency check - in a real implementation this would be more sophisticated
    for (const auto& dependency : testInfo.dependencies)
    {
        if (!TestRegistry::Instance()->IsTestRegistered(dependency))
        {
            TC_LOG_WARN("playerbot.test", "Dependency {} not found for test {}", dependency, testInfo.name);
            return false;
        }
    }
    return true;
}

void AutomatedTestRunner::GenerateReports(const TestRunResult& results)
{
    if (m_config.generateDetailedReports)
    {
        GenerateHtmlReport(results);
        GenerateJsonReport(results);
    }

    if (m_config.generateJunitXml)
    {
        GenerateJunitXmlReport(results);
    }

    // Always generate a text summary
    ::std::ofstream summaryFile(m_config.reportOutputPath + "/" + results.runId + "_summary.txt");
    if (summaryFile.is_open())
    {
        summaryFile << results.GenerateFullReport();
        summaryFile.close();
    }
}

void AutomatedTestRunner::GenerateJsonReport(const TestRunResult& results)
{
    ::std::ofstream jsonFile(m_config.reportOutputPath + "/" + results.runId + "_report.json");
    if (!jsonFile.is_open())
        return;

    jsonFile << "{\n";
    jsonFile << "  \"runId\": \"" << results.runId << "\",\n";
    jsonFile << "  \"startTime\": \"" << ::std::chrono::system_clock::to_time_t(results.startTime) << "\",\n";
    jsonFile << "  \"endTime\": \"" << ::std::chrono::system_clock::to_time_t(results.endTime) << "\",\n";
    jsonFile << "  \"totalTests\": " << results.totalTests << ",\n";
    jsonFile << "  \"passedTests\": " << results.passedTests << ",\n";
    jsonFile << "  \"failedTests\": " << results.failedTests << ",\n";
    jsonFile << "  \"successRate\": " << results.GetOverallSuccessRate() << ",\n";
    jsonFile << "  \"testSuites\": [\n";

    bool firstSuite = true;
    for (const auto& suite : results.suiteResults)
    {
        if (!firstSuite) jsonFile << ",\n";
        firstSuite = false;

        jsonFile << "    {\n";
        jsonFile << "      \"name\": \"" << suite.suiteName << "\",\n";
        jsonFile << "      \"totalTests\": " << suite.totalTests << ",\n";
        jsonFile << "      \"passedTests\": " << suite.passedTests << ",\n";
        jsonFile << "      \"failedTests\": " << suite.failedTests << ",\n";
        jsonFile << "      \"executionTime\": " << suite.totalExecutionTime.count() << ",\n";
        jsonFile << "      \"tests\": [\n";

        bool firstTest = true;
        for (const auto& test : suite.testResults)
        {
            if (!firstTest) jsonFile << ",\n";
            firstTest = false;

            jsonFile << "        {\n";
            jsonFile << "          \"name\": \"" << test.testName << "\",\n";
            jsonFile << "          \"passed\": " << (test.passed ? "true" : "false") << ",\n";
            jsonFile << "          \"executionTime\": " << test.executionTime.count() << ",\n";
            jsonFile << "          \"failureReason\": \"" << test.failureReason << "\"\n";
            jsonFile << "        }";
        }

        jsonFile << "\n      ]\n";
        jsonFile << "    }";
    }

    jsonFile << "\n  ]\n";
    jsonFile << "}\n";

    jsonFile.close();
}

// ========================
// ContinuousIntegrationRunner Implementation
// ========================

ContinuousIntegrationRunner::ContinuousIntegrationRunner(const CIConfiguration& config)
    : m_ciConfig(config)
{
    TestConfiguration testConfig = CreateCITestConfiguration();
    m_testRunner = ::std::make_unique<AutomatedTestRunner>(testConfig);
}

int ContinuousIntegrationRunner::RunCIPipeline()
{
    TC_LOG_INFO("playerbot.test", "Starting CI pipeline");

    if (!ValidateCIEnvironment())
    {
        TC_LOG_ERROR("playerbot.test", "CI environment validation failed");
        return 1;
    }

    // Run smoke tests first
    if (!RunSmokeTests())
    {
        TC_LOG_ERROR("playerbot.test", "Smoke tests failed");
        return 1;
    }

    // Run full test suite
    TestRunResult results = m_testRunner->RunAllTests();

    // Generate artifacts
    if (m_ciConfig.generateArtifacts)
        GenerateArtifacts(results);

    // Handle results
    if (!results.IsSuccessful())
    {
        HandleCIFailure(results);
        return 1;
    }

    TC_LOG_INFO("playerbot.test", "CI pipeline completed successfully");
    return 0;
}

bool ContinuousIntegrationRunner::RunSmokeTests()
{
    TestRunResult smokeResults = m_testRunner->RunTestsBySeverity(TestSeverity::SMOKE);
    return smokeResults.IsSuccessful();
}

TestConfiguration ContinuousIntegrationRunner::CreateCITestConfiguration()
{
    TestConfiguration config;

    // CI-optimized settings
    config.categoriesToRun = {
        TestCategory::UNIT,
        TestCategory::INTEGRATION,
        TestCategory::PERFORMANCE
    };

    config.severityLevels = {
        TestSeverity::SMOKE,
        TestSeverity::FUNCTIONAL,
        TestSeverity::CRITICAL
    };

    config.stopOnFirstFailure = m_ciConfig.failFastMode;
    config.testTimeoutSeconds = 60; // Shorter timeout for CI
    config.maxRetries = 1;          // Fewer retries for CI
    config.generateDetailedReports = true;
    config.generateJunitXml = true;

    // Performance thresholds for CI (may be more lenient)
    config.performanceThresholds.maxInvitationAcceptanceTime = 5000000;    // 5s
    config.performanceThresholds.maxCombatEngagementTime = 5000000;        // 5s
    config.performanceThresholds.maxCpuUsage = 95.0f;                      // 95%

    return config;
}

void ContinuousIntegrationRunner::GenerateArtifacts(const TestRunResult& results)
{
    ::std::filesystem::create_directories(m_ciConfig.artifactPath);

    // Copy test reports to artifact directory
    ::std::filesystem::copy(results.configuration.reportOutputPath,
                         m_ciConfig.artifactPath + "/test_reports",
                         ::std::filesystem::copy_options::recursive);

    TC_LOG_INFO("playerbot.test", "Generated CI artifacts in: {}", m_ciConfig.artifactPath);
}

} // namespace Test
} // namespace Playerbot