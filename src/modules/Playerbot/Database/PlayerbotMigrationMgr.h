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
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

struct MigrationInfo
{
    std::string version;
    std::string description;
    std::string filename;
    std::function<bool()> upgradeFunction;
    std::function<bool()> downgradeFunction;
    bool isApplied = false;
    uint32 executionTimeMs = 0;
    std::string checksum;
};

// Migration status (previously in IPlayerbotMigrationMgr)
struct MigrationStatus
{
    std::string currentVersion;
    std::string targetVersion;
    uint32 appliedCount{0};
    uint32 pendingCount{0};
    std::vector<std::string> pendingMigrations;
    bool isUpToDate{false};
    bool isValid{false};
    std::string lastError;
};

class TC_GAME_API PlayerbotMigrationMgr final
{
public:
    static PlayerbotMigrationMgr* instance();

    // Core migration operations
    bool Initialize();
    bool ApplyMigrations();
    bool ApplyMigration(std::string const& version);
    bool RollbackMigration(std::string const& version);

    // Migration information
    std::vector<std::string> GetPendingMigrations();
    std::vector<std::string> GetAppliedMigrations();
    std::string GetCurrentVersion();
    bool IsMigrationApplied(std::string const& version);

    // Database schema validation
    bool ValidateSchema();
    bool ValidateVersion(std::string const& expectedVersion);
    bool CreateMigrationTable();

    // Migration registration
    void RegisterMigration(MigrationInfo const& migration);

    // Utility functions
    std::string CalculateFileChecksum(std::string const& filepath);
    bool ExecuteSQLFile(std::string const& filepath);
    bool ExecuteSQLStatement(std::string const& sql);

    // Safety and rollback
    bool BackupDatabase(std::string const& backupPath = "");
    bool RestoreDatabase(std::string const& backupPath);
    bool CanRollback(std::string const& version);

    // Migration status and reporting
    MigrationStatus GetMigrationStatus();
    void PrintMigrationStatus();

    // Source-Database version synchronization
    /**
     * @brief Get the expected database version from source code
     * @return The PLAYERBOT_DB_VERSION constant
     */
    static uint32 GetExpectedDatabaseVersion() { return PLAYERBOT_DB_VERSION; }

    /**
     * @brief Get the current database version from applied migrations
     * @return Number of applied migrations (version count)
     */
    uint32 GetDatabaseVersion() const;

    /**
     * @brief Check if database version matches source code version
     * @return true if versions match, false if migration needed
     */
    bool ValidateDatabaseVersion() const;

    /**
     * @brief Check for version mismatch and log appropriate warnings
     * @return true if versions match (or mismatch is acceptable), false if critical mismatch
     */
    bool CheckVersionMismatch() const;

private:
    PlayerbotMigrationMgr();
    ~PlayerbotMigrationMgr() = default;

    // Internal migration management
    bool LoadMigrationsFromDatabase();
    bool LoadMigrationFiles();
    bool ValidateMigrationIntegrity();

    // File-based migration discovery (TrinityCore pattern)
    struct MigrationFile
    {
        std::string filename;
        std::string fullPath;
        std::string version;
        std::string description;
        bool isApplied = false;
    };

    std::vector<MigrationFile> DiscoverMigrationFiles();
    std::string ExtractVersionFromFilename(std::string const& filename);
    std::string ExtractDescriptionFromFilename(std::string const& filename);
    bool ApplyMigrationFile(MigrationFile const& migration);

    // Legacy built-in migration support (deprecated)
    void RegisterBuiltInMigrations();
    bool ApplyInitialSchema();
    bool DropAllTables();
    bool ApplyAccountEnhancements();
    bool DropAccountEnhancements();
    bool ApplyLifecycleManagement();
    bool DropLifecycleManagement();
    bool ApplyCharacterDistribution();
    bool DropCharacterDistribution();

    // Database operations
    bool RecordMigration(std::string const& version, std::string const& description, uint32 executionTimeMs = 0, std::string const& checksum = "");
    bool RemoveMigrationRecord(std::string const& version);

    // Version comparison
    int CompareVersions(std::string const& version1, std::string const& version2);
    std::vector<std::string> GetVersionSequence(std::string const& fromVersion, std::string const& toVersion);

    // Error handling
    void LogMigrationError(std::string const& version, std::string const& error);
    void LogMigrationSuccess(std::string const& version, uint32 executionTimeMs);

    // Data members
    std::unordered_map<std::string, MigrationInfo> _migrations;
    std::vector<std::string> _appliedMigrations;
    std::string _currentVersion;
    bool _initialized = false;

    // Migration file paths - will be resolved at runtime
    static constexpr char const* MIGRATION_PATH = "sql/migrations/";
    static constexpr char const* BACKUP_PATH = "sql/backups/";
    static constexpr char const* MIGRATION_TABLE = "playerbot_migrations";

    // Source code database version - increment this when adding new migrations
    // Version 1: Base schema from dump (includes all previous migrations 001-008)
    // Future migrations will increment this number
    static constexpr uint32 PLAYERBOT_DB_VERSION = 1;

    // Helper method to get migration directory path
    static std::string GetMigrationPath();

    // File storage for discovered migrations
    std::vector<MigrationFile> _discoveredMigrations;

    // Legacy: Supported migration versions (deprecated - now auto-discovered)
    inline static std::vector<std::string> const MIGRATION_SEQUENCE = {
        "001",  // Initial schema
        "002",  // Account management
        "003",  // Lifecycle management
        "004"   // Character distribution
    };
};

// Migration helper macros
#define REGISTER_MIGRATION(version, description, upgradeFunc, downgradeFunc) \
    do { \
        MigrationInfo migration; \
        migration.version = version; \
        migration.description = description; \
        migration.upgradeFunction = upgradeFunc; \
        migration.downgradeFunction = downgradeFunc; \
        PlayerbotMigrationMgr::instance()->RegisterMigration(migration); \
    } while(0)

#define MIGRATION_LOG_INFO(version, message, ...) \
    TC_LOG_INFO("playerbots.migration", "[Migration {}] " message, version, ##__VA_ARGS__)

#define MIGRATION_LOG_ERROR(version, message, ...) \
    TC_LOG_ERROR("playerbots.migration", "[Migration {}] ERROR: " message, version, ##__VA_ARGS__)

#define MIGRATION_LOG_WARN(version, message, ...) \
    TC_LOG_WARN("playerbots.migration", "[Migration {}] WARNING: " message, version, ##__VA_ARGS__)