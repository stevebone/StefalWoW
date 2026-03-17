/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_DATABASE_H
#define PLAYERBOT_DATABASE_H

#include "PlayerbotDatabaseConnection.h"
#include "Define.h"
#include <memory>
#include <mutex>

/**
 * @class PlayerbotDatabaseManager
 * @brief Singleton manager for playerbot database operations
 *
 * This class provides a simplified interface for playerbot database access
 * that doesn't rely on complex template instantiation. It manages the
 * connection lifecycle and provides basic query operations.
 */
class TC_DATABASE_API PlayerbotDatabaseManager final
{
public:
    static PlayerbotDatabaseManager* instance();

    /**
     * @brief Initialize the database connection
     * @param connectionInfo Database connection string
     * @return true if initialization successful, false otherwise
     */
    bool Initialize(std::string const& connectionInfo);

    /**
     * @brief Close the database connection
     */
    void Close();

    /**
     * @brief Execute a query and return results
     * @param sql SQL query string
     * @return QueryResult shared pointer, null if query failed
     */
    QueryResult Query(std::string const& sql);

    /**
     * @brief Execute a statement without returning results
     * @param sql SQL statement string
     * @return true if execution successful, false otherwise
     */
    bool Execute(std::string const& sql);

    /**
     * @brief Check if database is connected and operational
     * @return true if connected, false otherwise
     */
    bool IsConnected() const;

    /**
     * @brief Validate database schema matches expected structure
     * @return true if schema is valid, false otherwise
     */
    bool ValidateSchema();

private:
    PlayerbotDatabaseManager() = default;
    ~PlayerbotDatabaseManager() = default;

    std::unique_ptr<PlayerbotDatabaseConnection> _connection;

    /// Mutex to protect database operations from concurrent access
    /// Required because CreateBotAccountsBatch runs in a separate thread
    mutable std::mutex _connectionMutex;
};

#define sPlayerbotDatabase PlayerbotDatabaseManager::instance()

#endif // PLAYERBOT_DATABASE_H
