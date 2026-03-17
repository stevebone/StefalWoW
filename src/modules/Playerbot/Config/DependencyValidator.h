#pragma once

#include "Define.h"
#include <string>
#include <vector>

namespace Playerbot {

struct DependencyInfo
{
    ::std::string name;
    ::std::string version;
    ::std::string status;
    bool required;
    ::std::string errorMessage;
};

/**
 * @brief Validates all enterprise-grade dependencies required for Playerbot
 *
 * This class performs runtime validation of:
 * - Intel Threading Building Blocks (TBB) 2021.5+
 * - Parallel Hashmap (phmap) 1.3.8+
 * - Boost Libraries 1.74.0+
 * - MySQL Connector C++ 8.0.33+
 *
 * Must be called during module initialization to ensure all dependencies
 * are available before creating BotSession instances.
 */
class TC_GAME_API DependencyValidator
{
public:
    /**
     * @brief Validates all required dependencies
     * @return true if all dependencies are available and functional
     */
    static bool ValidateAllDependencies();

    /**
     * @brief Gets detailed status of all dependencies
     * @return Vector of dependency information with status details
     */
    static ::std::vector<DependencyInfo> GetDependencyStatus();

    /**
     * @brief Logs detailed dependency report to server log
     */
    static void LogDependencyReport();

    /**
     * @brief Validates if system meets minimum requirements for enterprise features
     * @return true if system can handle high-performance bot operations
     */
    static bool ValidateSystemRequirements();

private:
    // Individual dependency validators
    static bool ValidateTBB();
    static bool ValidatePhmap();
    static bool ValidateBoost();
    static bool ValidateMySQL();

    // Version string getters
    static ::std::string GetTBBVersion();
    static ::std::string GetBoostVersion();
    static ::std::string GetMySQLVersion();
    static ::std::string GetPhmapVersion();

    // System requirement checks
    static bool CheckMemoryRequirements();
    static bool CheckCPURequirements();
    static bool CheckDiskRequirements();

    // Test specific functionality
    static bool TestTBBConcurrency();
    static bool TestPhmapPerformance();
    static bool TestBoostComponents();
    static bool TestMySQLConnectivity();
};

} // namespace Playerbot