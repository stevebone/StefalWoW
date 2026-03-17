/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "GuidedSetupHelper.h"
#include "Log.h"
#include "PlayerbotDatabase.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace Playerbot
{

// Configuration file names
static constexpr char const* CONFIG_FILENAME = "playerbots.conf";
static constexpr char const* CONFIG_DIST_FILENAME = "playerbots.conf.dist";

bool GuidedSetupHelper::CheckAndRunSetup()
{
    TC_LOG_INFO("playerbots", "GuidedSetupHelper: Checking configuration setup...");

    // Check if config file already exists
    if (ConfigFileExists())
    {
        TC_LOG_DEBUG("playerbots", "GuidedSetupHelper: Configuration file found");

        // Validate essential settings
        auto warnings = ValidateEssentialConfig();
        if (!warnings.empty())
        {
            DisplayConfigWarnings(warnings);
        }

        return true;
    }

    // Config doesn't exist - check if we can auto-create from .dist
    if (ConfigDistFileExists())
    {
        TC_LOG_INFO("playerbots", "GuidedSetupHelper: First-time setup detected");
        DisplayFirstTimeSetupMessage();

        // Attempt to create default config
        if (CreateDefaultConfig())
        {
            DisplayConfigCreatedMessage(GetConfigFilePath());
            DisplayQuickStartSummary();
            return true;
        }
        else
        {
            DisplaySetupErrorMessage("Failed to create default configuration file");
            DisplayManualSetupInstructions();
            return false;
        }
    }
    else
    {
        // Neither config nor dist exists - critical error
        DisplaySetupErrorMessage("Configuration template (playerbots.conf.dist) not found");
        DisplayManualSetupInstructions();
        return false;
    }
}

bool GuidedSetupHelper::ConfigFileExists()
{
    std::string configPath = GetConfigFilePath();
    return !configPath.empty() && std::filesystem::exists(configPath);
}

bool GuidedSetupHelper::ConfigDistFileExists()
{
    std::string distPath = GetConfigDistFilePath();
    return !distPath.empty() && std::filesystem::exists(distPath);
}

bool GuidedSetupHelper::CreateDefaultConfig()
{
    std::string distPath = GetConfigDistFilePath();
    if (distPath.empty())
    {
        TC_LOG_ERROR("playerbots", "GuidedSetupHelper: Cannot find playerbots.conf.dist");
        return false;
    }

    // Determine output path - same directory as dist file
    std::filesystem::path distFilePath(distPath);
    std::filesystem::path configPath = distFilePath.parent_path() / CONFIG_FILENAME;

    TC_LOG_INFO("playerbots", "GuidedSetupHelper: Creating default config at: {}", configPath.string());

    return CopyFile(distPath, configPath.string());
}

void GuidedSetupHelper::DisplayFirstTimeSetupMessage()
{
    PrintSeparator();
    PrintBannerLine("PLAYERBOT FIRST-TIME SETUP", true);
    PrintSeparator();
    TC_LOG_INFO("playerbots", "");
    TC_LOG_INFO("playerbots", "  Welcome to the Playerbot Module!");
    TC_LOG_INFO("playerbots", "");
    TC_LOG_INFO("playerbots", "  This appears to be your first time running the module.");
    TC_LOG_INFO("playerbots", "  A default configuration will be created automatically.");
    TC_LOG_INFO("playerbots", "");
    PrintSeparator();
}

void GuidedSetupHelper::DisplayConfigCreatedMessage(std::string const& configPath)
{
    PrintSeparator();
    PrintBannerLine("CONFIGURATION CREATED SUCCESSFULLY", true);
    PrintSeparator();
    TC_LOG_INFO("playerbots", "");
    TC_LOG_INFO("playerbots", "  Config file: {}", configPath);
    TC_LOG_INFO("playerbots", "");
    TC_LOG_INFO("playerbots", "  KEY SETTINGS TO REVIEW:");
    TC_LOG_INFO("playerbots", "  -------------------------");
    TC_LOG_INFO("playerbots", "  Playerbot.Enable = 1              (Bot system enabled)");
    TC_LOG_INFO("playerbots", "  Playerbot.MaxBots = 100           (Maximum concurrent bots)");
    TC_LOG_INFO("playerbots", "  Playerbot.AutoCreateAccounts = 1  (Auto-create bot accounts)");
    TC_LOG_INFO("playerbots", "  Playerbot.Spawn.OnServerStart = 1 (Spawn bots immediately)");
    TC_LOG_INFO("playerbots", "");
    TC_LOG_INFO("playerbots", "  DATABASE SETTINGS:");
    TC_LOG_INFO("playerbots", "  ------------------");
    TC_LOG_INFO("playerbots", "  Playerbot.Database.Host = 127.0.0.1");
    TC_LOG_INFO("playerbots", "  Playerbot.Database.Port = 3306");
    TC_LOG_INFO("playerbots", "  Playerbot.Database.Name = playerbot");
    TC_LOG_INFO("playerbots", "  Playerbot.Database.User = trinity");
    TC_LOG_INFO("playerbots", "  Playerbot.Database.Pass = trinity");
    TC_LOG_INFO("playerbots", "");
    PrintSeparator();
}

void GuidedSetupHelper::DisplaySetupErrorMessage(std::string const& reason)
{
    PrintSeparator();
    PrintBannerLine("PLAYERBOT SETUP ERROR", true);
    PrintSeparator();
    TC_LOG_ERROR("playerbots", "");
    TC_LOG_ERROR("playerbots", "  Reason: {}", reason);
    TC_LOG_ERROR("playerbots", "");
    PrintSeparator();
}

void GuidedSetupHelper::DisplayDatabaseSetupMessage()
{
    PrintSeparator();
    PrintBannerLine("PLAYERBOT DATABASE SETUP REQUIRED", true);
    PrintSeparator();
    TC_LOG_ERROR("playerbots", "");
    TC_LOG_ERROR("playerbots", "  The playerbot database does not exist or is not accessible.");
    TC_LOG_ERROR("playerbots", "");
    TC_LOG_ERROR("playerbots", "  OPTION 1: Auto-create (requires CREATE privilege)");
    TC_LOG_ERROR("playerbots", "  ------------------------------------------------");
    TC_LOG_ERROR("playerbots", "  GRANT CREATE ON *.* TO 'trinity'@'localhost';");
    TC_LOG_ERROR("playerbots", "  FLUSH PRIVILEGES;");
    TC_LOG_ERROR("playerbots", "");
    TC_LOG_ERROR("playerbots", "  OPTION 2: Manual creation");
    TC_LOG_ERROR("playerbots", "  -------------------------");
    TC_LOG_ERROR("playerbots", "  CREATE DATABASE playerbot CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;");
    TC_LOG_ERROR("playerbots", "  GRANT ALL ON playerbot.* TO 'trinity'@'localhost';");
    TC_LOG_ERROR("playerbots", "  FLUSH PRIVILEGES;");
    TC_LOG_ERROR("playerbots", "");
    TC_LOG_ERROR("playerbots", "  After creating the database, restart the server.");
    TC_LOG_ERROR("playerbots", "  Migrations will be applied automatically.");
    TC_LOG_ERROR("playerbots", "");
    PrintSeparator();
}

void GuidedSetupHelper::DisplayManualSetupInstructions()
{
    PrintSeparator();
    PrintBannerLine("MANUAL SETUP INSTRUCTIONS", true);
    PrintSeparator();
    TC_LOG_ERROR("playerbots", "");
    TC_LOG_ERROR("playerbots", "  1. Locate the playerbots.conf.dist file:");
    TC_LOG_ERROR("playerbots", "     - Development: src/modules/Playerbot/conf/playerbots.conf.dist");
    TC_LOG_ERROR("playerbots", "     - Installed:   etc/playerbots.conf.dist");
    TC_LOG_ERROR("playerbots", "");
    TC_LOG_ERROR("playerbots", "  2. Copy to playerbots.conf in the same directory");
    TC_LOG_ERROR("playerbots", "");
    TC_LOG_ERROR("playerbots", "  3. Edit playerbots.conf and configure:");
    TC_LOG_ERROR("playerbots", "     - Playerbot.Enable = 1");
    TC_LOG_ERROR("playerbots", "     - Playerbot.Database.* settings");
    TC_LOG_ERROR("playerbots", "");
    TC_LOG_ERROR("playerbots", "  4. Restart the server");
    TC_LOG_ERROR("playerbots", "");
    TC_LOG_ERROR("playerbots", "  MINIMUM REQUIRED SETTINGS:");
    TC_LOG_ERROR("playerbots", "  --------------------------");
    TC_LOG_ERROR("playerbots", "  Playerbot.Enable = 1");
    TC_LOG_ERROR("playerbots", "  Playerbot.Database.Host = 127.0.0.1");
    TC_LOG_ERROR("playerbots", "  Playerbot.Database.Name = playerbot");
    TC_LOG_ERROR("playerbots", "");
    PrintSeparator();
}

std::string GuidedSetupHelper::GetConfigFilePath()
{
    auto searchPaths = GetConfigSearchPaths();

    for (auto const& basePath : searchPaths)
    {
        std::filesystem::path configPath = std::filesystem::path(basePath) / CONFIG_FILENAME;
        if (std::filesystem::exists(configPath))
        {
            return configPath.string();
        }
    }

    // Return first search path location for creation
    if (!searchPaths.empty())
    {
        // For config file, prefer the etc/ directory if it exists
        for (auto const& basePath : searchPaths)
        {
            if (std::filesystem::exists(basePath))
            {
                return (std::filesystem::path(basePath) / CONFIG_FILENAME).string();
            }
        }
    }

    return "";
}

std::string GuidedSetupHelper::GetConfigDistFilePath()
{
    auto searchPaths = GetConfigSearchPaths();

    // Search in conf subdirectory first (source layout)
    std::vector<std::string> distSearchPaths;
    for (auto const& basePath : searchPaths)
    {
        distSearchPaths.push_back(basePath);
        distSearchPaths.push_back((std::filesystem::path(basePath) / "conf").string());
    }

    // Add source directory paths
    distSearchPaths.push_back("src/modules/Playerbot/conf");
    distSearchPaths.push_back("../src/modules/Playerbot/conf");

    for (auto const& basePath : distSearchPaths)
    {
        std::filesystem::path distPath = std::filesystem::path(basePath) / CONFIG_DIST_FILENAME;
        if (std::filesystem::exists(distPath))
        {
            return distPath.string();
        }
    }

    return "";
}

std::vector<std::string> GuidedSetupHelper::ValidateEssentialConfig()
{
    std::vector<std::string> warnings;

    // This would integrate with PlayerbotConfig to check actual values
    // For now, return empty (no warnings)

    return warnings;
}

void GuidedSetupHelper::DisplayConfigWarnings(std::vector<std::string> const& warnings)
{
    if (warnings.empty())
        return;

    TC_LOG_WARN("playerbots", "================================================================================");
    TC_LOG_WARN("playerbots", "  PLAYERBOT CONFIGURATION WARNINGS");
    TC_LOG_WARN("playerbots", "================================================================================");
    TC_LOG_WARN("playerbots", "");

    for (auto const& warning : warnings)
    {
        TC_LOG_WARN("playerbots", "  - {}", warning);
    }

    TC_LOG_WARN("playerbots", "");
    TC_LOG_WARN("playerbots", "================================================================================");
}

bool GuidedSetupHelper::IsFreshInstallation()
{
    // Check if playerbot_migrations table exists in database
    try
    {
        QueryResult result = sPlayerbotDatabase->Query("SHOW TABLES LIKE 'playerbot_migrations'");
        return !result; // Fresh if table doesn't exist
    }
    catch (...)
    {
        return true; // Assume fresh if database isn't accessible
    }
}

void GuidedSetupHelper::DisplayQuickStartSummary()
{
    PrintSeparator();
    PrintBannerLine("QUICK START - READY TO GO!", true);
    PrintSeparator();
    TC_LOG_INFO("playerbots", "");
    TC_LOG_INFO("playerbots", "  Your Playerbot module is configured with tester-friendly defaults.");
    TC_LOG_INFO("playerbots", "");
    TC_LOG_INFO("playerbots", "  WHAT HAPPENS NEXT:");
    TC_LOG_INFO("playerbots", "  - Database tables will be created automatically");
    TC_LOG_INFO("playerbots", "  - Bot accounts and characters will be generated");
    TC_LOG_INFO("playerbots", "  - Bots will spawn when the server starts");
    TC_LOG_INFO("playerbots", "");
    TC_LOG_INFO("playerbots", "  TO CUSTOMIZE:");
    TC_LOG_INFO("playerbots", "  Edit playerbots.conf to adjust bot count, behavior, and features.");
    TC_LOG_INFO("playerbots", "");
    TC_LOG_INFO("playerbots", "  PROFILES AVAILABLE:");
    TC_LOG_INFO("playerbots", "  - Playerbot.Profile = \"minimal\"      (10 bots, basic features)");
    TC_LOG_INFO("playerbots", "  - Playerbot.Profile = \"standard\"     (100 bots, recommended)");
    TC_LOG_INFO("playerbots", "  - Playerbot.Profile = \"performance\"  (500 bots, optimized)");
    TC_LOG_INFO("playerbots", "  - Playerbot.Profile = \"singleplayer\" (Solo play optimized)");
    TC_LOG_INFO("playerbots", "");
    PrintSeparator();
}

std::vector<std::string> GuidedSetupHelper::GetConfigSearchPaths()
{
    return {
        ".",                                          // Current directory
        "./etc",                                      // Installed path (bin/etc)
        "../etc",                                     // Installed path (relative to bin)
        "./conf",                                     // Source conf directory
        "src/modules/Playerbot/conf",                 // Source directory
        "../src/modules/Playerbot/conf"               // Alternative source directory
    };
}

bool GuidedSetupHelper::CopyFile(std::string const& source, std::string const& dest)
{
    try
    {
        std::error_code ec;
        std::filesystem::copy_file(source, dest, std::filesystem::copy_options::overwrite_existing, ec);

        if (ec)
        {
            TC_LOG_ERROR("playerbots", "GuidedSetupHelper: Failed to copy file: {}", ec.message());
            return false;
        }

        TC_LOG_INFO("playerbots", "GuidedSetupHelper: Successfully created {}", dest);
        return true;
    }
    catch (std::filesystem::filesystem_error const& ex)
    {
        TC_LOG_ERROR("playerbots", "GuidedSetupHelper: Filesystem error: {}", ex.what());
        return false;
    }
}

void GuidedSetupHelper::PrintBannerLine(std::string const& text, bool isHeader)
{
    if (isHeader)
    {
        TC_LOG_INFO("playerbots", "  {}", text);
    }
    else
    {
        TC_LOG_INFO("playerbots", "  {}", text);
    }
}

void GuidedSetupHelper::PrintSeparator()
{
    TC_LOG_INFO("playerbots", "================================================================================");
}

} // namespace Playerbot
