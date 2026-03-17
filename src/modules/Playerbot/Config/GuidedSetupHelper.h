/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_GUIDED_SETUP_HELPER_H
#define TRINITYCORE_GUIDED_SETUP_HELPER_H

#include "Define.h"
#include <string>
#include <vector>

namespace Playerbot
{

/**
 * @class GuidedSetupHelper
 * @brief Provides first-time setup assistance and configuration guidance
 *
 * This class handles the tester experience when setting up the Playerbot module
 * for the first time. It provides:
 * - Detection of missing configuration files
 * - Auto-creation of default config from .dist template
 * - Clear setup instructions displayed at startup
 * - Configuration validation and suggestions
 *
 * Design Philosophy:
 * - Zero manual steps required for basic setup
 * - Clear, actionable error messages
 * - Automatic fallback to sensible defaults
 * - No modifications to worldserver.conf required
 */
class TC_GAME_API GuidedSetupHelper
{
public:
    /**
     * @brief Check if first-time setup is required and handle it
     * @return true if setup completed successfully or no setup needed, false if critical error
     *
     * This is the main entry point called during PlayerbotModule::Initialize().
     * It checks for configuration files and creates defaults if needed.
     */
    static bool CheckAndRunSetup();

    /**
     * @brief Check if the playerbots.conf file exists
     * @return true if config file exists, false otherwise
     */
    static bool ConfigFileExists();

    /**
     * @brief Check if the playerbots.conf.dist template file exists
     * @return true if dist file exists, false otherwise
     */
    static bool ConfigDistFileExists();

    /**
     * @brief Create default config file from .dist template
     * @return true if config was created successfully, false otherwise
     *
     * Copies playerbots.conf.dist to playerbots.conf with tester-friendly defaults.
     */
    static bool CreateDefaultConfig();

    /**
     * @brief Display the first-time setup welcome message
     *
     * Shows a formatted banner with setup instructions and next steps.
     */
    static void DisplayFirstTimeSetupMessage();

    /**
     * @brief Display configuration created notification
     * @param configPath Path to the created config file
     *
     * Shows a message indicating the default config was created and
     * lists key settings the user might want to review.
     */
    static void DisplayConfigCreatedMessage(std::string const& configPath);

    /**
     * @brief Display error message when setup cannot proceed
     * @param reason The specific reason setup failed
     *
     * Provides clear instructions on how to resolve the issue.
     */
    static void DisplaySetupErrorMessage(std::string const& reason);

    /**
     * @brief Display database setup required message
     *
     * Shows instructions when database connection fails or database doesn't exist.
     */
    static void DisplayDatabaseSetupMessage();

    /**
     * @brief Display configuration missing message with manual setup instructions
     *
     * Shown when .dist file doesn't exist and auto-setup isn't possible.
     */
    static void DisplayManualSetupInstructions();

    /**
     * @brief Get the expected config file path
     * @return Full path to playerbots.conf
     */
    static std::string GetConfigFilePath();

    /**
     * @brief Get the expected .dist template file path
     * @return Full path to playerbots.conf.dist
     */
    static std::string GetConfigDistFilePath();

    /**
     * @brief Validate essential configuration values
     * @return List of validation errors, empty if all valid
     *
     * Checks that critical settings like database connection info are present.
     */
    static std::vector<std::string> ValidateEssentialConfig();

    /**
     * @brief Display validation warnings for non-critical issues
     * @param warnings List of warning messages to display
     */
    static void DisplayConfigWarnings(std::vector<std::string> const& warnings);

    /**
     * @brief Check if this is a fresh installation with no prior data
     * @return true if this appears to be a first-time setup
     *
     * Checks for existence of playerbot database tables to determine
     * if this is a completely new installation.
     */
    static bool IsFreshInstallation();

    /**
     * @brief Display quick start summary after successful setup
     *
     * Shows a brief summary of what was configured and how to proceed.
     */
    static void DisplayQuickStartSummary();

private:
    // Search paths for config files (in order of preference)
    static std::vector<std::string> GetConfigSearchPaths();

    // Copy file with error handling
    static bool CopyFile(std::string const& source, std::string const& dest);

    // Format a banner line for console output
    static void PrintBannerLine(std::string const& text, bool isHeader = false);

    // Print separator line
    static void PrintSeparator();
};

} // namespace Playerbot

#endif // TRINITYCORE_GUIDED_SETUP_HELPER_H
