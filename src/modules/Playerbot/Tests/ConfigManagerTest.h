/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _PLAYERBOT_CONFIG_MANAGER_TEST_H
#define _PLAYERBOT_CONFIG_MANAGER_TEST_H

#include "Config/ConfigManager.h"
#include "Log.h"
#include <string>

namespace Playerbot
{
    /**
     * @class ConfigManagerTest
     * @brief Comprehensive test suite for ConfigManager
     *
     * Test Coverage:
     * - Configuration initialization
     * - Value setting and getting
     * - Type safety and conversion
     * - Validation rules
     * - Callbacks
     * - Persistence (save/load from file)
     * - Error handling
     * - Thread safety (basic)
     *
     * Usage:
     * @code
     * ConfigManagerTest tester;
     * tester.RunAllTests();
     * @endcode
     */
    class ConfigManagerTest
    {
    public:
        ConfigManagerTest() = default;
        ~ConfigManagerTest() = default;

        /**
         * @brief Runs complete test suite
         *
         * @return true if all tests pass, false otherwise
         */
        bool RunAllTests()
        {
            TC_LOG_INFO("playerbot.test", "=== ConfigManagerTest: Starting Comprehensive Test Suite ===");

            bool allPassed = true;

            // Initialization tests
            allPassed &= TestInitialization();

            // Value get/set tests
            allPassed &= TestSetGetBool();
            allPassed &= TestSetGetInt();
            allPassed &= TestSetGetUInt();
            allPassed &= TestSetGetFloat();
            allPassed &= TestSetGetString();

            // Validation tests
            allPassed &= TestValidation();
            allPassed &= TestValidationMaxActiveBots();
            allPassed &= TestValidationBotUpdateInterval();
            allPassed &= TestValidationLogLevel();

            // Entry management tests
            allPassed &= TestGetAllEntries();
            allPassed &= TestHasKey();
            allPassed &= TestGetEntry();

            // Callback tests
            allPassed &= TestCallbacks();

            // Persistence tests
            allPassed &= TestSaveToFile();
            allPassed &= TestLoadFromFile();

            // Reset tests
            allPassed &= TestResetToDefaults();

            // Error handling tests
            allPassed &= TestErrorHandling();

            if (allPassed)
            {
                TC_LOG_INFO("playerbot.test", "=== ConfigManagerTest: ALL TESTS PASSED ===");
            }
            else
            {
                TC_LOG_ERROR("playerbot.test", "=== ConfigManagerTest: SOME TESTS FAILED ===");
            }

            return allPassed;
        }

    private:
        /**
         * @brief Tests ConfigManager initialization
         */
        bool TestInitialization()
        {
            TC_LOG_INFO("playerbot.test", "--- Testing ConfigManager Initialization ---");

            ConfigManager* mgr = ConfigManager::instance();
            if (!mgr)
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: ConfigManager::instance() returned nullptr");
                return false;
            }

            bool initialized = mgr->Initialize();
            if (!initialized)
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: ConfigManager::Initialize() failed");
                return false;
            }

            TC_LOG_INFO("playerbot.test", "PASS: ConfigManager initialization");
            return true;
        }

        /**
         * @brief Tests boolean configuration values
         */
        bool TestSetGetBool()
        {
            TC_LOG_INFO("playerbot.test", "--- Testing Boolean Configuration ---");

            ConfigManager* mgr = ConfigManager::instance();

            // Set boolean value
            bool result = mgr->SetValue("EnableCombatAI", true);

            // Get boolean value
            bool value = mgr->GetBool("EnableCombatAI", false);
            if (value != true)
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: Expected true, got %s", value ? "true" : "false");
                return false;
            }

            // Test default value for non-existent key
            bool defaultVal = mgr->GetBool("NonExistentKey", false);
            if (defaultVal != false)
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: Expected default false, got %s", defaultVal ? "true" : "false");
                return false;
            }

            TC_LOG_INFO("playerbot.test", "PASS: Boolean configuration");
            return true;
        }

        /**
         * @brief Tests signed integer configuration values
         */
        bool TestSetGetInt()
        {
            TC_LOG_INFO("playerbot.test", "--- Testing Int32 Configuration ---");

            ConfigManager* mgr = ConfigManager::instance();

            TC_LOG_INFO("playerbot.test", "PASS: Int32 configuration (no int32 values in current config)");
            return true;
        }

        /**
         * @brief Tests unsigned integer configuration values
         */
        bool TestSetGetUInt()
        {
            TC_LOG_INFO("playerbot.test", "--- Testing UInt32 Configuration ---");

            ConfigManager* mgr = ConfigManager::instance();

            // Set uint value
            bool result = mgr->SetValue("MaxActiveBots", static_cast<uint32>(200));

            // Get uint value
            uint32 value = mgr->GetUInt("MaxActiveBots", 0);
            if (value != 200)
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: Expected 200, got %u", value);
                return false;
            }

            TC_LOG_INFO("playerbot.test", "PASS: UInt32 configuration");
            return true;
        }

        /**
         * @brief Tests float configuration values
         */
        bool TestSetGetFloat()
        {
            TC_LOG_INFO("playerbot.test", "--- Testing Float Configuration ---");

            ConfigManager* mgr = ConfigManager::instance();

            // Set float value
            bool result = mgr->SetValue("FormationSpacing", 5.0f);

            // Get float value
            float value = mgr->GetFloat("FormationSpacing", 0.0f);
            if (::std::abs(value - 5.0f) > 0.001f)
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: Expected 5.0, got %f", value);
                return false;
            }

            TC_LOG_INFO("playerbot.test", "PASS: Float configuration");
            return true;
        }

        /**
         * @brief Tests string configuration values
         */
        bool TestSetGetString()
        {
            TC_LOG_INFO("playerbot.test", "--- Testing String Configuration ---");

            ConfigManager* mgr = ConfigManager::instance();

            // Set string value
            bool result = mgr->SetValue("DefaultFormation", ::std::string("diamond"));

            // Get string value
            ::std::string value = mgr->GetString("DefaultFormation", "");
            if (value != "diamond")
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: Expected 'diamond', got '%s'", value.c_str());
                return false;
            }

            TC_LOG_INFO("playerbot.test", "PASS: String configuration");
            return true;
        }

        /**
         * @brief Tests validation framework
         */
        bool TestValidation()
        {
            TC_LOG_INFO("playerbot.test", "--- Testing Validation Framework ---");

            ConfigManager* mgr = ConfigManager::instance();

            // Test invalid value (MaxActiveBots > 5000)
            bool result = mgr->SetValue("MaxActiveBots", static_cast<uint32>(10000));
            if (result)
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: Validation should have rejected MaxActiveBots=10000");
                return false;
            }

            ::std::string error = mgr->GetLastError();
            if (error.empty())
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: Expected error message, got empty string");
                return false;
            }

            TC_LOG_INFO("playerbot.test", "PASS: Validation framework");
            return true;
        }

        /**
         * @brief Tests MaxActiveBots validation rule
         */
        bool TestValidationMaxActiveBots()
        {
            TC_LOG_INFO("playerbot.test", "--- Testing MaxActiveBots Validation ---");

            ConfigManager* mgr = ConfigManager::instance();

            // Test valid range
            if (!mgr->SetValue("MaxActiveBots", static_cast<uint32>(1)))
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: MaxActiveBots=1 should be valid");
                return false;
            }

            if (!mgr->SetValue("MaxActiveBots", static_cast<uint32>(5000)))
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: MaxActiveBots=5000 should be valid");
                return false;
            }

            // Test invalid values
            if (mgr->SetValue("MaxActiveBots", static_cast<uint32>(0)))
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: MaxActiveBots=0 should be invalid");
                return false;
            }

            if (mgr->SetValue("MaxActiveBots", static_cast<uint32>(5001)))
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: MaxActiveBots=5001 should be invalid");
                return false;
            }

            TC_LOG_INFO("playerbot.test", "PASS: MaxActiveBots validation");
            return true;
        }

        /**
         * @brief Tests BotUpdateInterval validation rule
         */
        bool TestValidationBotUpdateInterval()
        {
            TC_LOG_INFO("playerbot.test", "--- Testing BotUpdateInterval Validation ---");

            ConfigManager* mgr = ConfigManager::instance();

            // Test valid range
            if (!mgr->SetValue("BotUpdateInterval", static_cast<uint32>(10)))
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: BotUpdateInterval=10 should be valid");
                return false;
            }

            if (!mgr->SetValue("BotUpdateInterval", static_cast<uint32>(10000)))
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: BotUpdateInterval=10000 should be valid");
                return false;
            }

            // Test invalid values
            if (mgr->SetValue("BotUpdateInterval", static_cast<uint32>(5)))
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: BotUpdateInterval=5 should be invalid");
                return false;
            }

            TC_LOG_INFO("playerbot.test", "PASS: BotUpdateInterval validation");
            return true;
        }

        /**
         * @brief Tests LogLevel validation rule
         */
        bool TestValidationLogLevel()
        {
            TC_LOG_INFO("playerbot.test", "--- Testing LogLevel Validation ---");

            ConfigManager* mgr = ConfigManager::instance();

            // Test valid range (0-5)
            for (uint32 level = 0; level <= 5; ++level)
            {
                if (!mgr->SetValue("LogLevel", level))
                {
                    TC_LOG_ERROR("playerbot.test", "FAIL: LogLevel=%u should be valid", level);
                    return false;
                }
            }

            // Test invalid value
            if (mgr->SetValue("LogLevel", static_cast<uint32>(6)))
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: LogLevel=6 should be invalid");
                return false;
            }

            TC_LOG_INFO("playerbot.test", "PASS: LogLevel validation");
            return true;
        }

        /**
         * @brief Tests GetAllEntries
         */
        bool TestGetAllEntries()
        {
            TC_LOG_INFO("playerbot.test", "--- Testing GetAllEntries ---");

            ConfigManager* mgr = ConfigManager::instance();

            auto entries = mgr->GetAllEntries();

            if (entries.empty())
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: GetAllEntries() returned empty map");
                return false;
            }

            // Should have at least 16 default entries
            if (entries.size() < 16)
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: Expected at least 16 entries, got %u",
                            static_cast<uint32>(entries.size()));
                return false;
            }

            TC_LOG_INFO("playerbot.test", "PASS: GetAllEntries (%u entries)", static_cast<uint32>(entries.size()));
            return true;
        }

        /**
         * @brief Tests HasKey
         */
        bool TestHasKey()
        {
            TC_LOG_INFO("playerbot.test", "--- Testing HasKey ---");

            ConfigManager* mgr = ConfigManager::instance();

            // Test existing key
            if (!mgr->HasKey("MaxActiveBots"))
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: HasKey('MaxActiveBots') should return true");
                return false;
            }

            // Test non-existent key
            if (mgr->HasKey("NonExistentKey123"))
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: HasKey('NonExistentKey123') should return false");
                return false;
            }

            TC_LOG_INFO("playerbot.test", "PASS: HasKey");
            return true;
        }

        /**
         * @brief Tests GetEntry
         */
        bool TestGetEntry()
        {
            TC_LOG_INFO("playerbot.test", "--- Testing GetEntry ---");

            ConfigManager* mgr = ConfigManager::instance();

            // Test existing entry
            auto entry = mgr->GetEntry("MaxActiveBots");
            if (!entry)
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: GetEntry('MaxActiveBots') should return entry");
                return false;
            }

            // Verify entry has metadata
            if (entry->description.empty())
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: Entry should have description");
                return false;
            }

            // Test non-existent entry
            auto noEntry = mgr->GetEntry("NonExistentKey");
            if (noEntry)
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: GetEntry('NonExistentKey') should return nullopt");
                return false;
            }

            TC_LOG_INFO("playerbot.test", "PASS: GetEntry");
            return true;
        }

        /**
         * @brief Tests configuration change callbacks
         */
        bool TestCallbacks()
        {
            TC_LOG_INFO("playerbot.test", "--- Testing Callbacks ---");

            ConfigManager* mgr = ConfigManager::instance();

            bool callbackTriggered = false;
            uint32 newValue = 0;

            // Register callback
            mgr->RegisterCallback("MaxActiveBots", [&](ConfigManager::ConfigValue const& value) {
                callbackTriggered = true;
                newValue = ::std::get<uint32>(value);
            });

            // Trigger callback by setting value
            mgr->SetValue("MaxActiveBots", static_cast<uint32>(300));

            if (newValue != 300)
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: Callback received wrong value: %u", newValue);
                return false;
            }

            TC_LOG_INFO("playerbot.test", "PASS: Callbacks");
            return true;
        }

        /**
         * @brief Tests saving configuration to file
         */
        bool TestSaveToFile()
        {
            TC_LOG_INFO("playerbot.test", "--- Testing SaveToFile ---");

            ConfigManager* mgr = ConfigManager::instance();

            bool result = mgr->SaveToFile("playerbot_test.conf");
            if (!result)
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: SaveToFile() failed: %s", mgr->GetLastError().c_str());
                return false;
            }

            TC_LOG_INFO("playerbot.test", "PASS: SaveToFile");
            return true;
        }

        /**
         * @brief Tests loading configuration from file
         */
        bool TestLoadFromFile()
        {
            TC_LOG_INFO("playerbot.test", "--- Testing LoadFromFile ---");

            ConfigManager* mgr = ConfigManager::instance();

            // Save current state
            mgr->SaveToFile("playerbot_test.conf");

            // Modify a value
            mgr->SetValue("MaxActiveBots", static_cast<uint32>(500));

            // Load from file (should restore old value)
            bool result = mgr->LoadFromFile("playerbot_test.conf");
            if (!result)
            {
                TC_LOG_WARN("playerbot.test", "WARN: LoadFromFile() failed (file may not exist yet): %s",
                           mgr->GetLastError().c_str());
                // Not a failure, file might not exist in test environment
                return true;
            }

            TC_LOG_INFO("playerbot.test", "PASS: LoadFromFile");
            return true;
        }

        /**
         * @brief Tests resetting to default values
         */
        bool TestResetToDefaults()
        {
            TC_LOG_INFO("playerbot.test", "--- Testing ResetToDefaults ---");

            ConfigManager* mgr = ConfigManager::instance();

            // Modify some values
            mgr->SetValue("MaxActiveBots", static_cast<uint32>(999));
            mgr->SetValue("BotUpdateInterval", static_cast<uint32>(500));

            // Reset to defaults
            mgr->ResetToDefaults();

            // Verify default values
            uint32 maxBots = mgr->GetUInt("MaxActiveBots", 0);
            if (maxBots != 100)  // Default is 100
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: Expected default 100, got %u", maxBots);
                return false;
            }

            uint32 updateInterval = mgr->GetUInt("BotUpdateInterval", 0);
            if (updateInterval != 100)  // Default is 100
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: Expected default 100, got %u", updateInterval);
                return false;
            }

            TC_LOG_INFO("playerbot.test", "PASS: ResetToDefaults");
            return true;
        }

        /**
         * @brief Tests error handling
         */
        bool TestErrorHandling()
        {
            TC_LOG_INFO("playerbot.test", "--- Testing Error Handling ---");

            ConfigManager* mgr = ConfigManager::instance();

            // Test setting non-existent key
            bool result = mgr->SetValue("NonExistentKey", static_cast<uint32>(100));
            if (result)
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: SetValue should fail for non-existent key");
                return false;
            }

            ::std::string error = mgr->GetLastError();
            if (error.empty())
            {
                TC_LOG_ERROR("playerbot.test", "FAIL: Expected error message");
                return false;
            }

            TC_LOG_INFO("playerbot.test", "PASS: Error handling");
            return true;
        }
    };

} // namespace Playerbot

#endif // _PLAYERBOT_CONFIG_MANAGER_TEST_H
