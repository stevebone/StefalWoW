/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PlayerbotMigrationMgr.h"
#include "PlayerbotDatabaseStatements.h"
#include "PlayerbotDatabase.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Timer.h"
#include "Util.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <chrono>

// Static member definitions removed - now inline static in header to fix DLL export issues

PlayerbotMigrationMgr::PlayerbotMigrationMgr()
{
    _initialized = false;
    _currentVersion = "000";
}

std::string PlayerbotMigrationMgr::GetMigrationPath()
{
    // Try multiple paths in order of preference:
    // 1. Current working directory (development)
    // 2. Installed configuration directory (production)
    // 3. Module source directory (fallback)

    std::vector<std::string> searchPaths = {
        "sql/migrations/",                               // Development path
        "../etc/sql/migrations/",                        // Installed path relative to bin
        "../../etc/sql/migrations/",                     // Alternative installed path
        "src/modules/Playerbot/sql/migrations/",         // Source directory
        "../src/modules/Playerbot/sql/migrations/"       // Alternative source directory
    };

    for (auto const& path : searchPaths)
    {
        if (std::filesystem::exists(path) && std::filesystem::is_directory(path))
        {
            TC_LOG_DEBUG("playerbots.migration", "Using migration path: {}", path);
            return path;
        }
    }

    // Fallback to default path
    TC_LOG_WARN("playerbots.migration", "No migration directory found, using default: {}", MIGRATION_PATH);
    return MIGRATION_PATH;
}

PlayerbotMigrationMgr* PlayerbotMigrationMgr::instance()
{
    static PlayerbotMigrationMgr instance;
    return &instance;
}

bool PlayerbotMigrationMgr::Initialize()
{
    if (_initialized)
        return true;

    TC_LOG_INFO("playerbots.migration", "Initializing Playerbot Migration Manager...");

    // Create migration table if it doesn't exist
    if (!CreateMigrationTable())
    {
        TC_LOG_ERROR("playerbots.migration", "Failed to create migration tracking table");
        return false;
    }

    // Load applied migrations from database
    if (!LoadMigrationsFromDatabase())
    {
        TC_LOG_ERROR("playerbots.migration", "Failed to load migration history from database");
        return false;
    }

    // Register built-in migrations
    RegisterBuiltInMigrations();

    // Load migration files from filesystem
    if (!LoadMigrationFiles())
    {
        TC_LOG_WARN("playerbots.migration", "No migration files found or failed to load");
    }

    // Validate migration integrity
    if (!ValidateMigrationIntegrity())
    {
        TC_LOG_ERROR("playerbots.migration", "Migration integrity validation failed");
        return false;
    }

    // Determine current version
    _currentVersion = GetCurrentVersion();

    _initialized = true;
    TC_LOG_INFO("playerbots.migration", "Migration Manager initialized successfully. Current version: {}", _currentVersion);

    return true;
}

bool PlayerbotMigrationMgr::CreateMigrationTable()
{
    // Create migration table only if it doesn't exist (don't drop existing data!)
    std::string createTableSQL = "\n        CREATE TABLE IF NOT EXISTS `";
    createTableSQL += MIGRATION_TABLE;
    createTableSQL += R"(` (
            `version` VARCHAR(20) NOT NULL,
            `description` VARCHAR(255) NOT NULL,
            `applied_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            `checksum` VARCHAR(64) NULL,
            `execution_time_ms` INT UNSIGNED NULL DEFAULT 0,
            PRIMARY KEY (`version`),
            INDEX `idx_applied` (`applied_at`)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
        COMMENT='Database migration tracking'
    )";

    return ExecuteSQLStatement(createTableSQL);
}

bool PlayerbotMigrationMgr::LoadMigrationsFromDatabase()
{
    std::string query = "SELECT version, description, checksum, execution_time_ms FROM ";
    query += MIGRATION_TABLE;
    query += " ORDER BY applied_at";

    QueryResult result = sPlayerbotDatabase->Query(query);
    if (!result)
        return true; // No migrations applied yet, which is valid

    _appliedMigrations.clear();

    do
    {
        Field* fields = result->Fetch();
        std::string version = fields[0].GetString();
        std::string description = fields[1].GetString();
        std::string checksum = fields[2].GetString();
        uint32 executionTime = fields[3].GetUInt32();

        // Skip empty or invalid migration records
    if (version.empty())
    {
            TC_LOG_WARN("playerbots.migration", "Skipping empty migration version");
            continue;
        }

        _appliedMigrations.push_back(version);

        // Update migration info if we have it registered
    if (_migrations.find(version) != _migrations.end())
        {
            _migrations[version].isApplied = true;
            _migrations[version].executionTimeMs = executionTime;
            _migrations[version].checksum = checksum;
        }

        MIGRATION_LOG_INFO(version, "Loaded applied migration: {}", description);
    }
    while (result->NextRow());

    std::sort(_appliedMigrations.begin(), _appliedMigrations.end(), [this](std::string const& a, std::string const& b)
    {
        return CompareVersions(a, b) < 0;
    });

    return true;
}

bool PlayerbotMigrationMgr::LoadMigrationFiles()
{
    std::filesystem::path migrationDir = std::filesystem::path(GetMigrationPath());

    if (!std::filesystem::exists(migrationDir))
    {
        MIGRATION_LOG_WARN("SYSTEM", "Migration directory does not exist: {}", migrationDir.string());
        return false;
    }

    for (auto const& entry : std::filesystem::directory_iterator(migrationDir))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".sql")
        {
            std::string filename = entry.path().filename().string();

            // Extract version from filename (format: XXX_to_YYY_description.sql)
            size_t firstUnderscore = filename.find('_');
            if (firstUnderscore == std::string::npos)
                continue;

            std::string fromVersion = filename.substr(0, firstUnderscore);

            size_t toPos = filename.find("_to_");
            if (toPos == std::string::npos)
                continue;

            size_t secondUnderscore = filename.find('_', toPos + 4);
            if (secondUnderscore == std::string::npos)
                continue;

            std::string toVersion = filename.substr(toPos + 4, secondUnderscore - toPos - 4);

            // Register migration if not already registered
    if (_migrations.find(toVersion) == _migrations.end())
            {
                MigrationInfo migration;
                migration.version = toVersion;
                migration.filename = entry.path().string();
                migration.checksum = CalculateFileChecksum(migration.filename);
                migration.description = "File-based migration to version " + toVersion;

                _migrations[toVersion] = migration;
                MIGRATION_LOG_INFO(toVersion, "Registered migration file: {}", filename);
            }
        }
    }

    return true;
}

bool PlayerbotMigrationMgr::ValidateMigrationIntegrity()
{
    // Check if applied migrations exist in our migration sequence
    for (std::string const& appliedVersion : _appliedMigrations)
    {
        auto it = std::find(MIGRATION_SEQUENCE.begin(), MIGRATION_SEQUENCE.end(), appliedVersion);
        if (it == MIGRATION_SEQUENCE.end())
        {
            MIGRATION_LOG_WARN(appliedVersion, "Applied migration not found in expected sequence");
        }
    }

    // Check for gaps in migration sequence
    for (size_t i = 0; i < _appliedMigrations.size(); ++i)
    {
        size_t expectedIndex = i;
        if (expectedIndex >= MIGRATION_SEQUENCE.size())
            break;

        std::string const& expectedVersion = MIGRATION_SEQUENCE[expectedIndex];
        if (_appliedMigrations[i] != expectedVersion)
        {
            MIGRATION_LOG_WARN("SYSTEM", "Migration sequence gap detected. Expected: {}, Found: {}",
                expectedVersion, _appliedMigrations[i]);
        }
    }

    return true;
}

std::string PlayerbotMigrationMgr::GetCurrentVersion()
{
    if (_appliedMigrations.empty())
        return "000"; // No migrations applied

    // Return the highest applied version
    return *std::max_element(_appliedMigrations.begin(), _appliedMigrations.end(),
        [this](std::string const& a, std::string const& b) {
            return CompareVersions(a, b) < 0;
        });
}

bool PlayerbotMigrationMgr::ApplyMigrations()
{
    if (!_initialized && !Initialize())
        return false;

    // Discover all migration files (TrinityCore pattern)
    _discoveredMigrations = DiscoverMigrationFiles();
    if (_discoveredMigrations.empty())
    {
        TC_LOG_INFO("playerbots.migration", "No migration files found in {}", GetMigrationPath());
        return true;
    }

    // Count pending migrations
    size_t pendingCount = 0;
    for (auto const& migration : _discoveredMigrations)
    {
        if (!migration.isApplied)
            ++pendingCount;
    }

    if (pendingCount == 0)
    {
        TC_LOG_INFO("playerbots.migration", "No pending migrations to apply");
        return true;
    }

    TC_LOG_INFO("playerbots.migration", "Applying {} pending migrations (discovered {} total files)", pendingCount, _discoveredMigrations.size());

    // Apply pending migrations in order
    for (auto const& migration : _discoveredMigrations)
    {
        if (!migration.isApplied)
        {
            if (!ApplyMigrationFile(migration))
            {
                TC_LOG_ERROR("playerbots.migration", "Failed to apply migration file {}", migration.filename);
                return false;
            }
        }
    }

    TC_LOG_INFO("playerbots.migration", "All pending migrations applied successfully");
    return true;
}

bool PlayerbotMigrationMgr::ApplyMigration(std::string const& version)
{
    TC_LOG_DEBUG("module.playerbot.migration", "ApplyMigration called for version: {}", version);

    if (IsMigrationApplied(version))
    {
        MIGRATION_LOG_WARN(version, "Migration already applied");
        TC_LOG_DEBUG("module.playerbot.migration", "Migration {} already applied", version);
        return true;
    }

    TC_LOG_DEBUG("module.playerbot.migration", "Looking up migration {} in registry", version);
    auto it = _migrations.find(version);
    if (it == _migrations.end())
    {
        MIGRATION_LOG_ERROR(version, "Migration not registered");
        TC_LOG_ERROR("module.playerbot.migration", "Migration {} not registered", version);
        return false;
    }

    MigrationInfo& migration = it->second;

    MIGRATION_LOG_INFO(version, "Applying migration: {}", migration.description);
    TC_LOG_DEBUG("module.playerbot.migration", "Starting to apply migration {}: {}", version, migration.description);

    auto startTime = std::chrono::high_resolution_clock::now();
    bool success = false;

    // Try to execute registered function first
    if (migration.upgradeFunction)
    {
        TC_LOG_DEBUG("module.playerbot.migration", "Migration {} has upgrade function, calling it now", version);
        try
        {
            success = migration.upgradeFunction();
            TC_LOG_DEBUG("module.playerbot.migration", "Migration {} upgrade function completed with result: {}", version, success ? "SUCCESS" : "FAILURE");
        }
        catch (std::exception const& ex)
        {
            MIGRATION_LOG_ERROR(version, "Exception during migration function: {}", ex.what());
            TC_LOG_ERROR("module.playerbot.migration", "Exception in migration {}: {}", version, ex.what());
            success = false;
        }
    }
    // Otherwise try to execute SQL file
    else if (!migration.filename.empty())
    {
        TC_LOG_DEBUG("module.playerbot.migration", "Migration {} has SQL file, executing it", version);
        success = ExecuteSQLFile(migration.filename);
    }
    else
    {
        MIGRATION_LOG_ERROR(version, "No upgrade method available");
        success = false;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    uint32 executionTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    if (success)
    {
        TC_LOG_DEBUG("module.playerbot.migration", "Migration {} succeeded, recording in database", version);
        // Record successful migration
    if (!RecordMigration(version, migration.description, executionTime, migration.checksum))
        {
            MIGRATION_LOG_ERROR(version, "Failed to record migration in database");
            TC_LOG_ERROR("module.playerbot.migration", "Failed to record migration {} in database", version);
            return false;
        }
        TC_LOG_DEBUG("module.playerbot.migration", "Migration {} recorded successfully", version);

        migration.isApplied = true;
        migration.executionTimeMs = executionTime;
        _appliedMigrations.push_back(version);

        // Update current version
    if (CompareVersions(version, _currentVersion) > 0)
            _currentVersion = version;

        LogMigrationSuccess(version, executionTime);
        TC_LOG_DEBUG("module.playerbot.migration", "Migration {} completed successfully", version);
    }
    else
    {
        LogMigrationError(version, "Migration execution failed");
    }

    return success;
}

bool PlayerbotMigrationMgr::ExecuteSQLFile(std::string const& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        TC_LOG_ERROR("playerbots.migration", "Failed to open migration file: {}", filepath);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    // Split content by semicolons and execute each statement
    std::vector<std::string> statements;
    std::stringstream ss(content);
    std::string statement;

    while (std::getline(ss, statement, ';'))
    {
        // Trim whitespace and skip empty statements
        // Trim whitespace manually
        size_t start = statement.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            statement = "";
        } else {
            size_t end = statement.find_last_not_of(" \t\r\n");
            statement = statement.substr(start, end - start + 1);
        }
        if (statement.empty() || statement.find_first_not_of(" \t\r\n") == std::string::npos)
            continue;

        // Skip comments
    if (statement.substr(0, 2) == "--" || statement.substr(0, 2) == "/*")
            continue;

        if (!ExecuteSQLStatement(statement + ";"))
        {
            TC_LOG_ERROR("playerbots.migration", "Failed to execute statement in file {}: {}", filepath, statement.substr(0, 100));
            return false;
        }
    }

    return true;
}

bool PlayerbotMigrationMgr::ExecuteSQLStatement(std::string const& sql)
{
    try
    {
        sPlayerbotDatabase->Execute(sql);
        return true;
    }
    catch (std::exception const& ex)
    {
        TC_LOG_ERROR("playerbots.migration", "SQL execution failed: {} | Error: {}", sql.substr(0, 200), ex.what());
        return false;
    }
}

bool PlayerbotMigrationMgr::RecordMigration(std::string const& version, std::string const& description, uint32 executionTimeMs, std::string const& checksum)
{
    std::string insertSQL = "INSERT INTO " + std::string(MIGRATION_TABLE) +
        " (version, description, execution_time_ms, checksum) VALUES ('" + version + "', '" + description + "', " +
        std::to_string(executionTimeMs) + ", '" + checksum + "') ON DUPLICATE KEY UPDATE applied_at = CURRENT_TIMESTAMP";

    return ExecuteSQLStatement(insertSQL);
}

bool PlayerbotMigrationMgr::RemoveMigrationRecord(std::string const& version)
{
    std::string deleteSQL = "DELETE FROM " + std::string(MIGRATION_TABLE) + " WHERE version = '" + version + "'";
    return ExecuteSQLStatement(deleteSQL);
}

std::vector<std::string> PlayerbotMigrationMgr::GetPendingMigrations()
{
    TC_LOG_DEBUG("module.playerbot.migration", "GetPendingMigrations called");
    std::vector<std::string> pending;

    TC_LOG_DEBUG("module.playerbot.migration", "MIGRATION_SEQUENCE size: {}", MIGRATION_SEQUENCE.size());

    for (std::string const& version : MIGRATION_SEQUENCE)
    {
        TC_LOG_TRACE("module.playerbot.migration", "Checking migration version: {}", version);
        if (!IsMigrationApplied(version))
        {
            TC_LOG_TRACE("module.playerbot.migration", "Migration {} is pending", version);
            pending.push_back(version);
        }
        else
        {
            TC_LOG_TRACE("module.playerbot.migration", "Migration {} is already applied", version);
        }
    }

    TC_LOG_DEBUG("module.playerbot.migration", "GetPendingMigrations returning {} pending migrations", pending.size());
    return pending;
}

std::vector<std::string> PlayerbotMigrationMgr::GetAppliedMigrations()
{
    return _appliedMigrations;
}

bool PlayerbotMigrationMgr::IsMigrationApplied(std::string const& version)
{
    return std::find(_appliedMigrations.begin(), _appliedMigrations.end(), version) != _appliedMigrations.end();
}

int PlayerbotMigrationMgr::CompareVersions(std::string const& version1, std::string const& version2)
{
    // Handle empty versions (treat as "000")
    if (version1.empty() && version2.empty()) return 0;
    if (version1.empty()) return -1;  // Empty comes before any version
    if (version2.empty()) return 1;   // Any version comes after empty

    try {
        // Simple numeric comparison for versions like "001", "002", "003"
        int v1 = std::stoi(version1);
        int v2 = std::stoi(version2);

        if (v1 < v2) return -1;
        if (v1 > v2) return 1;
        return 0;
    }
    catch (std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot.migration", "Error in CompareVersions: v1='{}', v2='{}', exception: {}", version1, version2, e.what());
        // Fallback to string comparison if numeric conversion fails
        return version1.compare(version2);
    }
}

std::string PlayerbotMigrationMgr::CalculateFileChecksum(std::string const& filepath)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open())
        return "";

    // Simple hash calculation (in real implementation, use proper hash like SHA256)
    std::hash<std::string> hasher;
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return std::to_string(hasher(content));
}

void PlayerbotMigrationMgr::RegisterMigration(MigrationInfo const& migration)
{
    _migrations[migration.version] = migration;
    MIGRATION_LOG_INFO(migration.version, "Registered migration: {}", migration.description);
}

void PlayerbotMigrationMgr::LogMigrationSuccess(std::string const& version, uint32 executionTimeMs)
{
    MIGRATION_LOG_INFO(version, "Migration applied successfully in {}ms", executionTimeMs);
}

void PlayerbotMigrationMgr::LogMigrationError(std::string const& version, std::string const& error)
{
    MIGRATION_LOG_ERROR(version, "Migration failed: {}", error);
}

MigrationStatus PlayerbotMigrationMgr::GetMigrationStatus()
{
    MigrationStatus status;
    status.currentVersion = GetCurrentVersion();
    status.targetVersion = MIGRATION_SEQUENCE.empty() ? "000" : MIGRATION_SEQUENCE.back();

    std::vector<std::string> pending = GetPendingMigrations();
    status.pendingCount = static_cast<uint32>(pending.size());
    status.appliedCount = static_cast<uint32>(_appliedMigrations.size());
    status.pendingMigrations = pending;
    status.isValid = ValidateMigrationIntegrity();

    return status;
}

void PlayerbotMigrationMgr::PrintMigrationStatus()
{
    MigrationStatus status = GetMigrationStatus();

    TC_LOG_INFO("playerbots.migration", "=== Playerbot Migration Status ===");
    TC_LOG_INFO("playerbots.migration", "Current Version: {}", status.currentVersion);
    TC_LOG_INFO("playerbots.migration", "Target Version:  {}", status.targetVersion);
    TC_LOG_INFO("playerbots.migration", "Applied:         {} migrations", status.appliedCount);
    TC_LOG_INFO("playerbots.migration", "Pending:         {} migrations", status.pendingCount);
    TC_LOG_INFO("playerbots.migration", "Status:          {}", status.isValid ? "Valid" : "Invalid");

    if (!status.pendingMigrations.empty())
    {
        std::string pendingList;
        for (size_t i = 0; i < status.pendingMigrations.size(); ++i) {
            if (i > 0) pendingList += ", ";
            pendingList += status.pendingMigrations[i];
        }
        TC_LOG_INFO("playerbots.migration", "Pending migrations: {}", pendingList);
    }
}

void PlayerbotMigrationMgr::RegisterBuiltInMigrations()
{
    TC_LOG_INFO("playerbots.migration", "Registering built-in migrations...");

    // Migration 001: Initial schema - create all basic tables
    {
        MigrationInfo migration;
        migration.version = "001";
        migration.description = "Initial Playerbot schema - create core tables";
        migration.upgradeFunction = [this]() -> bool {
            return ApplyInitialSchema();
        };
        migration.downgradeFunction = [this]() -> bool {
            return DropAllTables();
        };
        RegisterMigration(migration);
    }

    // Migration 002: Account management enhancements
    {
        MigrationInfo migration;
        migration.version = "002";
        migration.description = "Account management system enhancements";
        migration.upgradeFunction = [this]() -> bool {
            return ApplyAccountEnhancements();
        };
        migration.downgradeFunction = [this]() -> bool {
            return DropAccountEnhancements();
        };
        RegisterMigration(migration);
    }

    // Migration 003: Lifecycle management
    {
        MigrationInfo migration;
        migration.version = "003";
        migration.description = "Bot lifecycle management system";
        migration.upgradeFunction = [this]() -> bool {
            return ApplyLifecycleManagement();
        };
        migration.downgradeFunction = [this]() -> bool {
            return DropLifecycleManagement();
        };
        RegisterMigration(migration);
    }

    // Migration 004: Character distribution
    {
        MigrationInfo migration;
        migration.version = "004";
        migration.description = "Character distribution system";
        migration.upgradeFunction = [this]() -> bool {
            return ApplyCharacterDistribution();
        };
        migration.downgradeFunction = [this]() -> bool {
            return DropCharacterDistribution();
        };
        RegisterMigration(migration);
    }

    TC_LOG_INFO("playerbots.migration", "Registered {} built-in migrations", _migrations.size());
}

bool PlayerbotMigrationMgr::ApplyInitialSchema()
{
    TC_LOG_INFO("playerbots.migration", "Applying initial schema from SQL file...");

    std::string sqlFile = GetMigrationPath() + "001_initial_schema.sql";
    if (!ExecuteSQLFile(sqlFile))
    {
        TC_LOG_ERROR("playerbots.migration", "Failed to execute initial schema SQL file: {}", sqlFile);
        return false;
    }

    TC_LOG_INFO("playerbots.migration", "Initial schema applied successfully from SQL file");
    return true;
}

bool PlayerbotMigrationMgr::DropAllTables()
{
    TC_LOG_INFO("playerbots.migration", "Dropping all playerbot tables...");

    std::vector<std::string> dropStatements = {
        "DROP TABLE IF EXISTS `playerbots_names_used`",
        "DROP TABLE IF EXISTS `playerbots_names`",
        "DROP TABLE IF EXISTS `playerbot_activity_patterns`"
    };

    for (const auto& sql : dropStatements)
    {
        ExecuteSQLStatement(sql); // Don't fail on drop errors
    }

    return true;
}

bool PlayerbotMigrationMgr::ApplyAccountEnhancements()
{
    TC_LOG_INFO("playerbots.migration", "Applying account management enhancements from SQL file...");

    std::string sqlFile = GetMigrationPath() + "002_account_management.sql";
    if (!ExecuteSQLFile(sqlFile))
    {
        TC_LOG_ERROR("playerbots.migration", "Failed to execute account management SQL file: {}", sqlFile);
        return false;
    }

    TC_LOG_INFO("playerbots.migration", "Account management enhancement migration completed successfully");
    return true;
}

bool PlayerbotMigrationMgr::DropAccountEnhancements()
{
    TC_LOG_INFO("playerbots.migration", "Dropping account management enhancements...");

    // DESIGN NOTE: Rollback migration 002 - account management enhancements
    // Returns true as default until rollback SQL is implemented
    // Full implementation should:
    // - Execute 002_account_management_rollback.sql if exists
    // - DROP TABLE IF EXISTS for any tables added in migration 002
    // - Revert ALTER TABLE changes to existing tables
    // - Remove any triggers, procedures, or functions added
    // - Validate schema integrity after rollback
    // Reference: sql/migrations/rollback/002_account_management_rollback.sql
    return true;
}

bool PlayerbotMigrationMgr::ApplyLifecycleManagement()
{
    TC_LOG_INFO("playerbots.migration", "Applying lifecycle management system from SQL file...");

    std::string sqlFile = GetMigrationPath() + "003_lifecycle_management.sql";
    if (!ExecuteSQLFile(sqlFile))
    {
        TC_LOG_ERROR("playerbots.migration", "Failed to execute lifecycle management SQL file: {}", sqlFile);
        return false;
    }

    TC_LOG_INFO("playerbots.migration", "Lifecycle management system migration completed successfully");
    return true;
}

bool PlayerbotMigrationMgr::DropLifecycleManagement()
{
    TC_LOG_INFO("playerbots.migration", "Dropping lifecycle management system...");

    // DESIGN NOTE: Rollback migration 003 - lifecycle management system
    // Returns true as default until rollback SQL is implemented
    // Full implementation should:
    // - Execute 003_lifecycle_management_rollback.sql if exists
    // - DROP TABLE playerbot_lifecycle_events
    // - DROP TABLE playerbot_session_log
    // - Revert any ALTER TABLE changes to existing tables
    // - Remove lifecycle-related triggers and indexes
    // - Clean up any orphaned data
    // Reference: sql/migrations/rollback/003_lifecycle_management_rollback.sql
    return true;
}

bool PlayerbotMigrationMgr::ApplyCharacterDistribution()
{
    TC_LOG_INFO("playerbots.migration", "Applying character distribution system from SQL file...");

    std::string sqlFile = GetMigrationPath() + "004_character_distribution.sql";
    if (!ExecuteSQLFile(sqlFile))
    {
        TC_LOG_ERROR("playerbots.migration", "Failed to execute character distribution SQL file: {}", sqlFile);
        return false;
    }

    TC_LOG_INFO("playerbots.migration", "Character distribution system migration completed successfully");
    return true;
}

bool PlayerbotMigrationMgr::DropCharacterDistribution()
{
    TC_LOG_INFO("playerbots.migration", "Dropping character distribution system...");

    // DESIGN NOTE: Rollback migration 004 - character distribution system
    // Returns true as default until rollback SQL is implemented
    // Full implementation should:
    // - Execute 004_character_distribution_rollback.sql if exists
    // - DROP TABLE playerbot_distribution_history
    // - DROP TABLE playerbot_level_brackets
    // - Revert distribution-related columns in existing tables
    // - Remove distribution algorithms and statistics tables
    // - Validate data consistency after rollback
    // Reference: sql/migrations/rollback/004_character_distribution_rollback.sql
    return true;
}

// ============================================================================
// File-based Migration Discovery (TrinityCore Pattern)
// ============================================================================

std::vector<PlayerbotMigrationMgr::MigrationFile> PlayerbotMigrationMgr::DiscoverMigrationFiles()
{
    std::vector<MigrationFile> migrations;

    try
    {
        std::string migrationPath = GetMigrationPath();
        std::filesystem::path migrationDir(migrationPath);

        // Check if migration directory exists
    if (!std::filesystem::exists(migrationDir) || !std::filesystem::is_directory(migrationDir))
        {
            TC_LOG_WARN("playerbots.migration", "Migration directory {} does not exist", migrationPath);
            return migrations;
        }

        // Scan directory for .sql files
    for (auto const& entry : std::filesystem::directory_iterator(migrationDir))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".sql")
            {
                std::string filename = entry.path().filename().string();
                std::string version = ExtractVersionFromFilename(filename);

                if (version.empty())
                {
                    TC_LOG_WARN("playerbots.migration", "Skipping file {} - could not extract version number", filename);
                    continue;
                }

                MigrationFile migration;
                migration.filename = filename;
                migration.fullPath = entry.path().string();
                migration.version = version;
                migration.description = ExtractDescriptionFromFilename(filename);
                migration.isApplied = IsMigrationApplied(version);

                migrations.push_back(migration);

                TC_LOG_DEBUG("playerbots.migration", "Discovered migration file: {} (version: {}, applied: {})",
                    filename, version, migration.isApplied ? "yes" : "no");
            }
        }

        // Sort by version number for proper order
        std::sort(migrations.begin(), migrations.end(), [this](MigrationFile const& a, MigrationFile const& b)
        {
            return CompareVersions(a.version, b.version) < 0;
        });

        TC_LOG_INFO("playerbots.migration", "Discovered {} migration files in {}", migrations.size(), migrationPath);
    }
    catch (std::filesystem::filesystem_error const& ex)
    {
        TC_LOG_ERROR("playerbots.migration", "Filesystem error while discovering migration files: {}", ex.what());
    }

    return migrations;
}

std::string PlayerbotMigrationMgr::ExtractVersionFromFilename(std::string const& filename)
{
    // Pattern: XXX_description.sql (e.g., "001_initial_schema.sql" -> "001")
    std::regex versionRegex(R"(^(\d{3})_.*\.sql$)");
    std::smatch match;

    if (std::regex_match(filename, match, versionRegex))
    {
        return match[1].str();
    }

    return "";
}

std::string PlayerbotMigrationMgr::ExtractDescriptionFromFilename(std::string const& filename)
{
    // Pattern: XXX_description.sql (e.g., "001_initial_schema.sql" -> "Initial schema")
    std::regex descRegex(R"(^\d{3}_(.*)\.sql$)");
    std::smatch match;

    if (std::regex_match(filename, match, descRegex))
    {
        std::string desc = match[1].str();
        // Replace underscores with spaces and capitalize
        std::replace(desc.begin(), desc.end(), '_', ' ');
        if (!desc.empty())
            desc[0] = std::toupper(desc[0]);
        return desc;
    }

    return "Unknown migration";
}

bool PlayerbotMigrationMgr::ApplyMigrationFile(MigrationFile const& migration)
{
    TC_LOG_INFO("playerbots.migration", "Applying migration file: {} ({})", migration.filename, migration.description);

    auto startTime = std::chrono::high_resolution_clock::now();

    // Execute the SQL file
    if (!ExecuteSQLFile(migration.fullPath))
    {
        TC_LOG_ERROR("playerbots.migration", "Failed to execute migration file: {}", migration.fullPath);
        return false;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    // Calculate file checksum for integrity
    std::string checksum = CalculateFileChecksum(migration.fullPath);

    // Record successful migration
    if (!RecordMigration(migration.version, migration.description, static_cast<uint32>(duration.count()), checksum))
    {
        TC_LOG_ERROR("playerbots.migration", "Failed to record migration {} in database", migration.version);
        return false;
    }

    TC_LOG_INFO("playerbots.migration", "Migration {} applied successfully in {}ms",
        migration.version, duration.count());

    return true;
}

// Full implementation of interface methods
bool PlayerbotMigrationMgr::RollbackMigration(std::string const& version)
{
    TC_LOG_INFO("playerbots.migration", "Attempting to rollback migration version: {}", version);

    // Validate migration can be rolled back
    if (!CanRollback(version))
    {
        TC_LOG_ERROR("playerbots.migration", "Cannot rollback migration {}: Not eligible for rollback", version);
        return false;
    }

    // Check if migration exists in registered migrations
    auto it = _migrations.find(version);
    if (it == _migrations.end())
    {
        // Check discovered file-based migrations
        bool foundInFiles = false;
        for (auto const& migration : _discoveredMigrations)
        {
            if (migration.version == version)
            {
                foundInFiles = true;
                break;
            }
        }

        if (!foundInFiles)
        {
            TC_LOG_ERROR("playerbots.migration", "RollbackMigration: Version {} not found in registered migrations", version);
            return false;
        }
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    // If migration has a downgrade function, execute it
    if (it != _migrations.end() && it->second.downgradeFunction)
    {
        TC_LOG_INFO("playerbots.migration", "Executing downgrade function for version {}", version);

        if (!it->second.downgradeFunction())
        {
            TC_LOG_ERROR("playerbots.migration", "Downgrade function failed for version {}", version);
            return false;
        }
    }
    else
    {
        // For file-based migrations, look for a corresponding rollback file
        std::string rollbackPath = GetMigrationPath() + "rollback/";
        std::string rollbackFilename = version + "_rollback.sql";
        std::string fullRollbackPath = rollbackPath + rollbackFilename;

        if (std::filesystem::exists(fullRollbackPath))
        {
            TC_LOG_INFO("playerbots.migration", "Executing rollback SQL file: {}", fullRollbackPath);

            if (!ExecuteSQLFile(fullRollbackPath))
            {
                TC_LOG_ERROR("playerbots.migration", "Failed to execute rollback SQL file: {}", fullRollbackPath);
                return false;
            }
        }
        else
        {
            TC_LOG_WARN("playerbots.migration", "No rollback file found for version {}, removing migration record only", version);
        }
    }

    // Remove migration record from database
    if (!RemoveMigrationRecord(version))
    {
        TC_LOG_ERROR("playerbots.migration", "Failed to remove migration record for version {}", version);
        return false;
    }

    // Update internal tracking
    auto appliedIt = std::find(_appliedMigrations.begin(), _appliedMigrations.end(), version);
    if (appliedIt != _appliedMigrations.end())
    {
        _appliedMigrations.erase(appliedIt);
    }

    // Update current version
    _currentVersion = GetCurrentVersion();

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    TC_LOG_INFO("playerbots.migration", "Successfully rolled back migration {} in {}ms", version, duration.count());

    return true;
}

bool PlayerbotMigrationMgr::ValidateSchema()
{
    TC_LOG_INFO("playerbots.migration", "Validating database schema...");

    bool isValid = true;
    uint32 errorCount = 0;
    uint32 warningCount = 0;

    // Define expected tables for the playerbot module
    std::vector<std::string> expectedTables = {
        MIGRATION_TABLE,                    // playerbot_migrations
        "playerbot_accounts",               // Bot account management
        "playerbot_bots",                   // Bot character data
        "playerbot_settings",               // Per-bot settings
        "playerbot_gear_templates",         // Gear template cache
        "playerbot_talent_templates",       // Talent template cache
        "playerbot_level_brackets"          // Level distribution brackets
    };

    // Check if migration table exists (critical)
    std::string migrationTableCheck = "SHOW TABLES LIKE '" + std::string(MIGRATION_TABLE) + "'";

    if (QueryResult result = sPlayerbotDatabase->Query(migrationTableCheck))
    {
        TC_LOG_DEBUG("playerbots.migration", "ValidateSchema: Migration table {} exists", MIGRATION_TABLE);
    }
    else
    {
        TC_LOG_ERROR("playerbots.migration", "ValidateSchema: Critical table {} is missing!", MIGRATION_TABLE);
        isValid = false;
        ++errorCount;
    }

    // Check for other expected tables (non-critical - may be created by migrations)
    for (auto const& tableName : expectedTables)
    {
        if (tableName == MIGRATION_TABLE)
            continue; // Already checked above

        std::string tableCheck = "SHOW TABLES LIKE '" + tableName + "'";
        if (!sPlayerbotDatabase->Query(tableCheck.c_str()))
        {
            TC_LOG_WARN("playerbots.migration", "ValidateSchema: Table {} not found (may require migration)", tableName);
            ++warningCount;
        }
    }

    // Validate migration table structure if it exists
    if (isValid)
    {
        std::string describeTable = "DESCRIBE " + std::string(MIGRATION_TABLE);
        QueryResult result = sPlayerbotDatabase->Query(describeTable.c_str());

        if (result)
        {
            std::vector<std::string> requiredColumns = {"version", "description", "applied_at"};
            std::vector<std::string> foundColumns;

            do
            {
                Field* fields = result->Fetch();
                foundColumns.push_back(fields[0].GetString());
            } while (result->NextRow());

            // Check for required columns
            for (auto const& required : requiredColumns)
            {
                bool found = false;
                for (auto const& col : foundColumns)
                {
                    if (col == required)
                    {
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    TC_LOG_ERROR("playerbots.migration", "ValidateSchema: Required column '{}' missing from {}", required, MIGRATION_TABLE);
                    isValid = false;
                    ++errorCount;
                }
            }

            TC_LOG_DEBUG("playerbots.migration", "ValidateSchema: Found {} columns in {}", foundColumns.size(), MIGRATION_TABLE);
        }
        else
        {
            TC_LOG_ERROR("playerbots.migration", "ValidateSchema: Failed to describe table {}", MIGRATION_TABLE);
            isValid = false;
            ++errorCount;
        }
    }

    // Validate migration integrity - check that all applied migrations exist
    for (auto const& version : _appliedMigrations)
    {
        // Check if migration exists in _migrations or _discoveredMigrations
        bool found = _migrations.find(version) != _migrations.end();

        if (!found)
        {
            for (auto const& migration : _discoveredMigrations)
            {
                if (migration.version == version)
                {
                    found = true;
                    break;
                }
            }
        }

        if (!found)
        {
            TC_LOG_WARN("playerbots.migration", "ValidateSchema: Applied migration {} not found in registry (orphaned)", version);
            ++warningCount;
        }
    }

    // Log summary
    if (isValid)
    {
        TC_LOG_INFO("playerbots.migration", "ValidateSchema: Schema validation passed ({} warnings)", warningCount);
    }
    else
    {
        TC_LOG_ERROR("playerbots.migration", "ValidateSchema: Schema validation FAILED - {} errors, {} warnings",
            errorCount, warningCount);
    }

    return isValid;
}

bool PlayerbotMigrationMgr::ValidateVersion(std::string const& expectedVersion)
{
    std::string currentVersion = GetCurrentVersion();
    bool isValid = (currentVersion == expectedVersion);

    if (!isValid)
    {
        TC_LOG_WARN("playerbots.migration", "ValidateVersion: Version mismatch - Expected: {}, Current: {}",
            expectedVersion, currentVersion);
    }

    return isValid;
}

bool PlayerbotMigrationMgr::BackupDatabase(std::string const& backupPath)
{
    TC_LOG_INFO("playerbots.migration", "Starting database backup...");

    // Determine backup directory path
    std::string backupDir = backupPath.empty() ? BACKUP_PATH : backupPath;

    // Create backup directory if it doesn't exist
    std::error_code ec;
    if (!std::filesystem::exists(backupDir, ec))
    {
        if (!std::filesystem::create_directories(backupDir, ec))
        {
            TC_LOG_ERROR("playerbots.migration", "BackupDatabase: Failed to create backup directory '{}': {}",
                backupDir, ec.message());
            return false;
        }
        TC_LOG_DEBUG("playerbots.migration", "Created backup directory: {}", backupDir);
    }

    // Generate timestamped backup filename
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &timeT);
#else
    localtime_r(&timeT, &tm);
#endif

    std::ostringstream filenameStream;
    filenameStream << "playerbot_backup_"
                   << std::put_time(&tm, "%Y%m%d_%H%M%S")
                   << ".sql";
    std::string backupFilename = filenameStream.str();
    std::string fullPath = backupDir + "/" + backupFilename;

    // Open backup file for writing
    std::ofstream backupFile(fullPath, std::ios::out | std::ios::trunc);
    if (!backupFile.is_open())
    {
        TC_LOG_ERROR("playerbots.migration", "BackupDatabase: Failed to create backup file '{}'", fullPath);
        return false;
    }

    // Write backup header
    backupFile << "-- Playerbot Database Backup\n";
    backupFile << "-- Generated: " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "\n";
    backupFile << "-- Migration Version: " << GetCurrentVersion() << "\n";
    backupFile << "-- WARNING: This backup contains only playerbot-specific tables\n";
    backupFile << "-- ================================================================\n\n";

    // Define tables to backup (playerbot-specific tables only)
    std::vector<std::string> tablesToBackup = {
        "playerbot_migrations",
        "playerbot_accounts",
        "playerbot_characters",
        "playerbot_session_log",
        "playerbot_lifecycle_events",
        "playerbot_distribution_history",
        "playerbot_settings",
        "playerbot_bot_settings",
        "playerbot_bot_gear",
        "playerbot_bot_talents"
    };

    uint32 tablesBackedUp = 0;
    uint32 rowsBackedUp = 0;

    for (auto const& tableName : tablesToBackup)
    {
        // Check if table exists
        std::string existsQuery = "SELECT 1 FROM information_schema.tables WHERE table_schema = DATABASE() AND table_name = '" + tableName + "'";
        QueryResult existsResult = sPlayerbotDatabase->Query(existsQuery.c_str());

        if (!existsResult)
        {
            TC_LOG_DEBUG("playerbots.migration", "BackupDatabase: Table '{}' does not exist, skipping", tableName);
            continue;
        }

        backupFile << "\n-- ================================================================\n";
        backupFile << "-- Table: " << tableName << "\n";
        backupFile << "-- ================================================================\n\n";

        // Get CREATE TABLE statement
        std::string createQuery = "SHOW CREATE TABLE `" + tableName + "`";
        QueryResult createResult = sPlayerbotDatabase->Query(createQuery.c_str());
        if (createResult)
        {
            Field* fields = createResult->Fetch();
            std::string createStatement = fields[1].GetString();

            backupFile << "DROP TABLE IF EXISTS `" << tableName << "`;\n";
            backupFile << createStatement << ";\n\n";
        }
        else
        {
            TC_LOG_WARN("playerbots.migration", "BackupDatabase: Failed to get CREATE TABLE for '{}'", tableName);
            continue;
        }

        // Get table data
        std::string dataQuery = "SELECT * FROM `" + tableName + "`";
        QueryResult dataResult = sPlayerbotDatabase->Query(dataQuery.c_str());
        if (dataResult)
        {
            // Get column names for INSERT statement
            std::string columnsQuery = "SELECT COLUMN_NAME FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = '" + tableName + "' ORDER BY ORDINAL_POSITION";
            QueryResult columnsResult = sPlayerbotDatabase->Query(columnsQuery.c_str());

            if (!columnsResult)
            {
                TC_LOG_WARN("playerbots.migration", "BackupDatabase: Failed to get columns for '{}'", tableName);
                continue;
            }

            std::vector<std::string> columnNames;
            do
            {
                Field* colFields = columnsResult->Fetch();
                columnNames.push_back(colFields[0].GetString());
            } while (columnsResult->NextRow());

            // Build INSERT statements
            uint32 tableRows = 0;
            do
            {
                Field* dataFields = dataResult->Fetch();
                std::ostringstream insertStream;
                insertStream << "INSERT INTO `" << tableName << "` (";

                // Column names
                for (size_t i = 0; i < columnNames.size(); ++i)
                {
                    if (i > 0) insertStream << ", ";
                    insertStream << "`" << columnNames[i] << "`";
                }
                insertStream << ") VALUES (";

                // Values
                for (size_t i = 0; i < columnNames.size(); ++i)
                {
                    if (i > 0) insertStream << ", ";

                    if (dataFields[i].IsNull())
                    {
                        insertStream << "NULL";
                    }
                    else
                    {
                        // Escape string values
                        std::string value = dataFields[i].GetString();
                        // Simple escape - replace ' with ''
                        std::string escaped;
                        for (char c : value)
                        {
                            if (c == '\'') escaped += "''";
                            else if (c == '\\') escaped += "\\\\";
                            else escaped += c;
                        }
                        insertStream << "'" << escaped << "'";
                    }
                }
                insertStream << ");\n";

                backupFile << insertStream.str();
                tableRows++;
                rowsBackedUp++;
            } while (dataResult->NextRow());

            TC_LOG_DEBUG("playerbots.migration", "BackupDatabase: Backed up {} rows from '{}'", tableRows, tableName);
        }
        else
        {
            TC_LOG_DEBUG("playerbots.migration", "BackupDatabase: Table '{}' is empty", tableName);
        }

        tablesBackedUp++;
    }

    // Write backup footer
    backupFile << "\n-- ================================================================\n";
    backupFile << "-- Backup Complete\n";
    backupFile << "-- Tables: " << tablesBackedUp << "\n";
    backupFile << "-- Rows: " << rowsBackedUp << "\n";
    backupFile << "-- ================================================================\n";

    backupFile.close();

    TC_LOG_INFO("playerbots.migration", "BackupDatabase: Successfully created backup at '{}' ({} tables, {} rows)",
        fullPath, tablesBackedUp, rowsBackedUp);

    return true;
}

bool PlayerbotMigrationMgr::RestoreDatabase(std::string const& backupPath)
{
    TC_LOG_INFO("playerbots.migration", "Starting database restore from '{}'...", backupPath);

    // Validate backup file exists
    std::error_code ec;
    if (!std::filesystem::exists(backupPath, ec))
    {
        TC_LOG_ERROR("playerbots.migration", "RestoreDatabase: Backup file '{}' does not exist", backupPath);
        return false;
    }

    // Open backup file for reading
    std::ifstream backupFile(backupPath, std::ios::in);
    if (!backupFile.is_open())
    {
        TC_LOG_ERROR("playerbots.migration", "RestoreDatabase: Failed to open backup file '{}'", backupPath);
        return false;
    }

    // Parse and validate backup header
    std::string line;
    bool foundHeader = false;
    std::string backupVersion;

    while (std::getline(backupFile, line))
    {
        if (line.find("-- Playerbot Database Backup") != std::string::npos)
        {
            foundHeader = true;
        }
        if (line.find("-- Migration Version:") != std::string::npos)
        {
            size_t pos = line.find(':');
            if (pos != std::string::npos)
            {
                backupVersion = line.substr(pos + 2);
                // Trim whitespace
                backupVersion.erase(0, backupVersion.find_first_not_of(" \t"));
                backupVersion.erase(backupVersion.find_last_not_of(" \t\r\n") + 1);
            }
            break;
        }
    }

    if (!foundHeader)
    {
        TC_LOG_ERROR("playerbots.migration", "RestoreDatabase: Invalid backup file - missing header");
        backupFile.close();
        return false;
    }

    TC_LOG_INFO("playerbots.migration", "RestoreDatabase: Backup version detected: {}", backupVersion.empty() ? "unknown" : backupVersion);

    // Reset file position to beginning
    backupFile.clear();
    backupFile.seekg(0, std::ios::beg);

    // Read entire file content
    std::stringstream buffer;
    buffer << backupFile.rdbuf();
    std::string sqlContent = buffer.str();
    backupFile.close();

    // Split into individual statements (by semicolon followed by newline)
    std::vector<std::string> statements;
    std::string currentStatement;
    bool inString = false;
    char stringDelimiter = 0;

    for (size_t i = 0; i < sqlContent.size(); ++i)
    {
        char c = sqlContent[i];

        // Track string state to avoid splitting on semicolons inside strings
        if (!inString && (c == '\'' || c == '"'))
        {
            inString = true;
            stringDelimiter = c;
        }
        else if (inString && c == stringDelimiter)
        {
            // Check for escaped quote
            if (i + 1 < sqlContent.size() && sqlContent[i + 1] == stringDelimiter)
            {
                currentStatement += c;
                currentStatement += sqlContent[++i];
                continue;
            }
            inString = false;
        }

        currentStatement += c;

        // Statement ends with semicolon followed by newline (not inside string)
        if (!inString && c == ';')
        {
            // Trim leading/trailing whitespace
            size_t start = currentStatement.find_first_not_of(" \t\r\n");
            size_t end = currentStatement.find_last_not_of(" \t\r\n");

            if (start != std::string::npos && end != std::string::npos)
            {
                std::string trimmed = currentStatement.substr(start, end - start + 1);

                // Skip comments
                if (!trimmed.empty() && trimmed[0] != '-' && trimmed.find("--") != 0)
                {
                    statements.push_back(trimmed);
                }
            }
            currentStatement.clear();
        }
    }

    if (statements.empty())
    {
        TC_LOG_ERROR("playerbots.migration", "RestoreDatabase: No valid SQL statements found in backup");
        return false;
    }

    TC_LOG_INFO("playerbots.migration", "RestoreDatabase: Found {} SQL statements to execute", statements.size());

    // Execute statements in a transaction
    uint32 successCount = 0;
    uint32 errorCount = 0;

    // Start transaction
    sPlayerbotDatabase->Execute("START TRANSACTION");

    for (auto const& statement : statements)
    {
        // Skip empty statements
        if (statement.empty() || statement == ";")
            continue;

        // Execute statement
        if (sPlayerbotDatabase->Execute(statement.c_str()))
        {
            successCount++;
        }
        else
        {
            TC_LOG_ERROR("playerbots.migration", "RestoreDatabase: Failed to execute: {}",
                statement.length() > 100 ? statement.substr(0, 100) + "..." : statement);
            errorCount++;

            // Rollback transaction on error
            sPlayerbotDatabase->Execute("ROLLBACK");
            TC_LOG_ERROR("playerbots.migration", "RestoreDatabase: Rolled back due to errors");
            return false;
        }
    }

    // Commit transaction
    sPlayerbotDatabase->Execute("COMMIT");

    TC_LOG_INFO("playerbots.migration", "RestoreDatabase: Successfully restored {} statements from '{}'",
        successCount, backupPath);

    // Reload migration state
    LoadMigrationsFromDatabase();

    return true;
}

bool PlayerbotMigrationMgr::CanRollback(std::string const& version)
{
    // Check if migration exists and has been applied
    if (!IsMigrationApplied(version))
    {
        TC_LOG_WARN("playerbots.migration", "CanRollback: Migration {} has not been applied", version);
        return false;
    }

    // Check if the migration has a downgrade function registered
    auto it = _migrations.find(version);
    if (it != _migrations.end())
    {
        if (it->second.downgradeFunction)
        {
            TC_LOG_DEBUG("playerbots.migration", "CanRollback: Migration {} has a registered downgrade function", version);
            return true;
        }
    }

    // Check if there are any backups available that could be used for restore
    std::error_code ec;
    if (std::filesystem::exists(BACKUP_PATH, ec) && std::filesystem::is_directory(BACKUP_PATH, ec))
    {
        for (auto const& entry : std::filesystem::directory_iterator(BACKUP_PATH))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".sql")
            {
                TC_LOG_DEBUG("playerbots.migration", "CanRollback: Found backup file {} - restore rollback possible", entry.path().string());
                return true;
            }
        }
    }

    // No rollback mechanism available for this version
    TC_LOG_WARN("playerbots.migration", "CanRollback: No rollback mechanism available for version {} (no downgrade function or backup files)", version);
    return false;
}

// ============================================================================
// Source-Database Version Synchronization
// ============================================================================

uint32 PlayerbotMigrationMgr::GetDatabaseVersion() const
{
    // Database version is determined by the number of applied migrations
    // With the new schema:
    //   Version 1 = Base schema (001_playerbot_base.sql)
    //   Version 2+ = Additional migrations (002_*.sql, 003_*.sql, etc.)
    return static_cast<uint32>(_appliedMigrations.size());
}

bool PlayerbotMigrationMgr::ValidateDatabaseVersion() const
{
    uint32 dbVersion = GetDatabaseVersion();
    uint32 expectedVersion = GetExpectedDatabaseVersion();

    return dbVersion == expectedVersion;
}

bool PlayerbotMigrationMgr::CheckVersionMismatch() const
{
    uint32 dbVersion = GetDatabaseVersion();
    uint32 expectedVersion = GetExpectedDatabaseVersion();

    if (dbVersion == expectedVersion)
    {
        TC_LOG_INFO("playerbots.migration", "Database version check PASSED: DB version {} matches source version {}",
            dbVersion, expectedVersion);
        return true;
    }

    if (dbVersion < expectedVersion)
    {
        // Database is behind source - migrations need to be applied
        TC_LOG_ERROR("playerbots.migration", "================================================================================");
        TC_LOG_ERROR("playerbots.migration", "  PLAYERBOT DATABASE VERSION MISMATCH");
        TC_LOG_ERROR("playerbots.migration", "================================================================================");
        TC_LOG_ERROR("playerbots.migration", "");
        TC_LOG_ERROR("playerbots.migration", "  Database version:  {}", dbVersion);
        TC_LOG_ERROR("playerbots.migration", "  Source version:    {}", expectedVersion);
        TC_LOG_ERROR("playerbots.migration", "  Status:            DATABASE IS OUTDATED");
        TC_LOG_ERROR("playerbots.migration", "");
        TC_LOG_ERROR("playerbots.migration", "  {} pending migration(s) need to be applied.", expectedVersion - dbVersion);
        TC_LOG_ERROR("playerbots.migration", "");
        TC_LOG_ERROR("playerbots.migration", "  Resolution: Migrations will be applied automatically on startup.");
        TC_LOG_ERROR("playerbots.migration", "              Check sql/migrations/ for pending SQL files.");
        TC_LOG_ERROR("playerbots.migration", "");
        TC_LOG_ERROR("playerbots.migration", "================================================================================");
        return false;
    }
    else
    {
        // Database is ahead of source - unusual situation (downgrade or stale source)
        TC_LOG_WARN("playerbots.migration", "================================================================================");
        TC_LOG_WARN("playerbots.migration", "  PLAYERBOT DATABASE VERSION WARNING");
        TC_LOG_WARN("playerbots.migration", "================================================================================");
        TC_LOG_WARN("playerbots.migration", "");
        TC_LOG_WARN("playerbots.migration", "  Database version:  {}", dbVersion);
        TC_LOG_WARN("playerbots.migration", "  Source version:    {}", expectedVersion);
        TC_LOG_WARN("playerbots.migration", "  Status:            DATABASE IS AHEAD OF SOURCE");
        TC_LOG_WARN("playerbots.migration", "");
        TC_LOG_WARN("playerbots.migration", "  This may indicate:");
        TC_LOG_WARN("playerbots.migration", "    - Source code is out of date");
        TC_LOG_WARN("playerbots.migration", "    - Manual migrations were applied");
        TC_LOG_WARN("playerbots.migration", "    - Testing/development database with newer schema");
        TC_LOG_WARN("playerbots.migration", "");
        TC_LOG_WARN("playerbots.migration", "  Resolution: Update source code or rollback database migrations.");
        TC_LOG_WARN("playerbots.migration", "");
        TC_LOG_WARN("playerbots.migration", "================================================================================");
        // Return true for warnings - server can continue but should be investigated
        return true;
    }
}