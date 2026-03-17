/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_DATABASE_CONNECTION_H
#define PLAYERBOT_DATABASE_CONNECTION_H

#include "DatabaseEnv.h"
#include "QueryResult.h"
#include <memory>
#include <string>

/**
 * @class PlayerbotDatabaseConnection
 * @brief Simple database connection wrapper for playerbot data access
 *
 * This class provides a simplified interface to access the playerbot database
 * without relying on complex template instantiation that causes linking issues.
 * It uses the existing TrinityCore database infrastructure internally.
 */
class TC_DATABASE_API PlayerbotDatabaseConnection
{
public:
    PlayerbotDatabaseConnection();
    ~PlayerbotDatabaseConnection();

    /**
     * @brief Initialize the database connection
     * @param connectionInfo MySQL connection string
     * @return true if connection successful, false otherwise
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
     * @brief Check if connection is active
     * @return true if connected, false otherwise
     */
    bool IsConnected() const { return _connected; }

    /**
     * @brief Get last error message
     * @return Error message string
     */
    std::string const& GetLastError() const { return _lastError; }

private:
    bool _connected;
    std::string _lastError;
    void* _mysqlHandle;  // MYSQL* handle (opaque pointer to avoid MySQL header dependencies)

    /**
     * @brief Internal method to set error message
     * @param error Error message
     */
    void SetError(std::string const& error);

    /**
     * @brief Attempt to create the database if it doesn't exist
     * @param handle MySQL handle
     * @param hostname Server hostname
     * @param port Server port
     * @param username Database username
     * @param password Database password
     * @param database Database name to create
     * @return true if database was created successfully
     */
    bool TryCreateDatabase(void* handle, std::string const& hostname, uint32 port,
        std::string const& username, std::string const& password, std::string const& database);

    /**
     * @brief Display helpful instructions when database setup is required
     * @param database Database name
     * @param username Database username
     */
    void DisplayDatabaseSetupInstructions(std::string const& database, std::string const& username);
};

#endif // PLAYERBOT_DATABASE_CONNECTION_H