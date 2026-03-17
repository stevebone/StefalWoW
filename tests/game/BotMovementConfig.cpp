/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
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

#define CATCH_CONFIG_ENABLE_CHRONO_STRINGMAKER
#include "tc_catch2.h"

#include "BotMovementConfig.h"
#include "Config.h"
#include <boost/filesystem.hpp>
#include <fstream>
#include <map>
#include <string>

std::string CreateBotMovementConfig(std::map<std::string, std::string> const& map)
{
    auto tempFileRel = boost::filesystem::unique_path("botmovement_test.ini");
    auto tempFileAbs = boost::filesystem::temp_directory_path() / tempFileRel;
    std::ofstream iniStream;
    iniStream.open(tempFileAbs.c_str());

    iniStream << "[worldserver]\n";
    for (auto const& itr : map)
        iniStream << itr.first << " = " << itr.second << "\n";

    iniStream.close();

    return tempFileAbs.string();
}

TEST_CASE("BotMovementConfig - Default Values", "[BotMovement][Config]")
{
    std::map<std::string, std::string> config;
    auto filePath = CreateBotMovementConfig(config);

    std::string err;
    REQUIRE(sConfigMgr->LoadInitial(filePath, std::vector<std::string>(), err));
    REQUIRE(err.empty());

    BotMovementConfig botConfig;
    botConfig.Load();

    SECTION("Default values are correct")
    {
        REQUIRE(botConfig.IsEnabled() == true);
        REQUIRE(botConfig.GetValidationLevel() == ValidationLevel::Standard);
        REQUIRE(botConfig.GetStuckPositionThreshold() == Milliseconds(3000));
        REQUIRE(botConfig.GetStuckDistanceThreshold() == 2.0f);
        REQUIRE(botConfig.GetMaxRecoveryAttempts() == 5);
        REQUIRE(botConfig.GetPathCacheSize() == 1000);
        REQUIRE(botConfig.GetPathCacheTTL() == Seconds(60));
        REQUIRE(botConfig.GetDebugLogLevel() == 2);
    }

    std::remove(filePath.c_str());
}

TEST_CASE("BotMovementConfig - Custom Values", "[BotMovement][Config]")
{
    std::map<std::string, std::string> config;
    config["BotMovement.Enable"] = "0";
    config["BotMovement.ValidationLevel"] = "3";
    config["BotMovement.StuckDetection.PositionThreshold"] = "5000";
    config["BotMovement.StuckDetection.DistanceThreshold"] = "3.5";
    config["BotMovement.Recovery.MaxAttempts"] = "10";
    config["BotMovement.PathCache.Size"] = "2000";
    config["BotMovement.PathCache.TTL"] = "120";
    config["BotMovement.Debug.LogLevel"] = "4";

    auto filePath = CreateBotMovementConfig(config);

    std::string err;
    REQUIRE(sConfigMgr->LoadInitial(filePath, std::vector<std::string>(), err));
    REQUIRE(err.empty());

    BotMovementConfig botConfig;
    botConfig.Load();

    SECTION("Custom values are loaded correctly")
    {
        REQUIRE(botConfig.IsEnabled() == false);
        REQUIRE(botConfig.GetValidationLevel() == ValidationLevel::Strict);
        REQUIRE(botConfig.GetStuckPositionThreshold() == Milliseconds(5000));
        REQUIRE(botConfig.GetStuckDistanceThreshold() == 3.5f);
        REQUIRE(botConfig.GetMaxRecoveryAttempts() == 10);
        REQUIRE(botConfig.GetPathCacheSize() == 2000);
        REQUIRE(botConfig.GetPathCacheTTL() == Seconds(120));
        REQUIRE(botConfig.GetDebugLogLevel() == 4);
    }

    std::remove(filePath.c_str());
}

TEST_CASE("BotMovementConfig - Validation Level Bounds", "[BotMovement][Config]")
{
    SECTION("Invalid validation level defaults to Standard")
    {
        std::map<std::string, std::string> config;
        config["BotMovement.ValidationLevel"] = "99";

        auto filePath = CreateBotMovementConfig(config);

        std::string err;
        REQUIRE(sConfigMgr->LoadInitial(filePath, std::vector<std::string>(), err));
        REQUIRE(err.empty());

        BotMovementConfig botConfig;
        botConfig.Load();

        REQUIRE(botConfig.GetValidationLevel() == ValidationLevel::Standard);

        std::remove(filePath.c_str());
    }

    SECTION("ValidationLevel::None is valid")
    {
        std::map<std::string, std::string> config;
        config["BotMovement.ValidationLevel"] = "0";

        auto filePath = CreateBotMovementConfig(config);

        std::string err;
        REQUIRE(sConfigMgr->LoadInitial(filePath, std::vector<std::string>(), err));
        REQUIRE(err.empty());

        BotMovementConfig botConfig;
        botConfig.Load();

        REQUIRE(botConfig.GetValidationLevel() == ValidationLevel::None);

        std::remove(filePath.c_str());
    }

    SECTION("ValidationLevel::Strict is valid")
    {
        std::map<std::string, std::string> config;
        config["BotMovement.ValidationLevel"] = "3";

        auto filePath = CreateBotMovementConfig(config);

        std::string err;
        REQUIRE(sConfigMgr->LoadInitial(filePath, std::vector<std::string>(), err));
        REQUIRE(err.empty());

        BotMovementConfig botConfig;
        botConfig.Load();

        REQUIRE(botConfig.GetValidationLevel() == ValidationLevel::Strict);

        std::remove(filePath.c_str());
    }
}

TEST_CASE("BotMovementConfig - Reload", "[BotMovement][Config]")
{
    std::map<std::string, std::string> config;
    config["BotMovement.Enable"] = "1";
    config["BotMovement.ValidationLevel"] = "2";

    auto filePath = CreateBotMovementConfig(config);

    std::string err;
    REQUIRE(sConfigMgr->LoadInitial(filePath, std::vector<std::string>(), err));
    REQUIRE(err.empty());

    BotMovementConfig botConfig;
    botConfig.Load();

    REQUIRE(botConfig.IsEnabled() == true);
    REQUIRE(botConfig.GetValidationLevel() == ValidationLevel::Standard);

    std::ofstream iniStream;
    iniStream.open(filePath.c_str(), std::ios::trunc);
    iniStream << "[worldserver]\n";
    iniStream << "BotMovement.Enable = 0\n";
    iniStream << "BotMovement.ValidationLevel = 1\n";
    iniStream.close();

    std::vector<std::string> errors;
    REQUIRE(sConfigMgr->Reload(errors));
    REQUIRE(errors.empty());

    botConfig.Reload();

    REQUIRE(botConfig.IsEnabled() == false);
    REQUIRE(botConfig.GetValidationLevel() == ValidationLevel::Basic);

    std::remove(filePath.c_str());
}
