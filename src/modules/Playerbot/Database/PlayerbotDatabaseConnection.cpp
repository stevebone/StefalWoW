/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PlayerbotDatabaseConnection.h"
#include "PlayerbotResultSet.h"
#include "Log.h"
#include "DatabaseEnv.h"
#include "Field.h"
#include "MySQLWorkaround.h"
#include <sstream>

PlayerbotDatabaseConnection::PlayerbotDatabaseConnection()
    : _connected(false), _mysqlHandle(nullptr)
{
}

PlayerbotDatabaseConnection::~PlayerbotDatabaseConnection()
{
    Close();
}

bool PlayerbotDatabaseConnection::Initialize(std::string const& connectionInfo)
{
    if (_connected)
    {
        TC_LOG_WARN("module.playerbot.database", "PlayerbotDatabaseConnection: Already connected");
        return true;
    }

    // Parse connection string format: "hostname;port;username;password;database"
    std::vector<std::string> tokens;
    std::stringstream ss(connectionInfo);
    std::string token;

    while (std::getline(ss, token, ';'))
    {
        tokens.push_back(token);
    }

    if (tokens.size() != 5)
    {
        SetError("Invalid connection string format. Expected: hostname;port;username;password;database");
        return false;
    }

    std::string const& hostname = tokens[0];
    uint32 port = std::stoul(tokens[1]);
    std::string const& username = tokens[2];
    std::string const& password = tokens[3];
    std::string const& database = tokens[4];

    // Initialize MySQL handle
    MYSQL* mysql = mysql_init(nullptr);
    if (!mysql)
    {
        SetError("Failed to initialize MySQL handle");
        return false;
    }

    // Set connection options
    bool reconnect = true;
    mysql_options(mysql, MYSQL_OPT_RECONNECT, &reconnect);

    // Connect to MySQL server
    if (!mysql_real_connect(mysql, hostname.c_str(), username.c_str(), password.c_str(),
                           database.c_str(), port, nullptr, CLIENT_MULTI_STATEMENTS))
    {
        uint32 errorCode = mysql_errno(mysql);

        // Check if database doesn't exist (Error 1049: ER_BAD_DB_ERROR)
        if (errorCode == 1049)
        {
            TC_LOG_INFO("module.playerbot.database",
                "PlayerbotDatabaseConnection: Database '{}' does not exist, attempting auto-create...", database);

            // Try to create the database
            if (TryCreateDatabase(mysql, hostname, port, username, password, database))
            {
                // Retry connection with the newly created database
                if (mysql_real_connect(mysql, hostname.c_str(), username.c_str(), password.c_str(),
                                       database.c_str(), port, nullptr, CLIENT_MULTI_STATEMENTS))
                {
                    _mysqlHandle = mysql;
                    _connected = true;
                    _lastError.clear();

                    TC_LOG_INFO("module.playerbot.database",
                        "PlayerbotDatabaseConnection: Successfully created and connected to {}:{}/{}",
                        hostname, port, database);
                    return true;
                }
            }

            // Auto-create failed - show helpful instructions
            DisplayDatabaseSetupInstructions(database, username);
            mysql_close(mysql);
            return false;
        }

        SetError(Trinity::StringFormat("Failed to connect to MySQL: {}", mysql_error(mysql)));
        mysql_close(mysql);
        return false;
    }

    _mysqlHandle = mysql;
    _connected = true;
    _lastError.clear();

    TC_LOG_INFO("module.playerbot.database", "PlayerbotDatabaseConnection: Connected to {}:{}/{}",
                hostname, port, database);

    return true;
}

bool PlayerbotDatabaseConnection::TryCreateDatabase(void* handle, std::string const& hostname, uint32 port,
    std::string const& username, std::string const& password, std::string const& database)
{
    MYSQL* mysql = static_cast<MYSQL*>(handle);

    // Reinitialize handle for connection without database
    mysql = mysql_init(nullptr);
    if (!mysql)
    {
        TC_LOG_ERROR("module.playerbot.database",
            "PlayerbotDatabaseConnection: Failed to reinitialize MySQL handle for database creation");
        return false;
    }

    // Connect without specifying a database
    if (!mysql_real_connect(mysql, hostname.c_str(), username.c_str(), password.c_str(),
                           nullptr, port, nullptr, CLIENT_MULTI_STATEMENTS))
    {
        TC_LOG_ERROR("module.playerbot.database",
            "PlayerbotDatabaseConnection: Cannot connect to MySQL server for database creation: {}",
            mysql_error(mysql));
        mysql_close(mysql);
        return false;
    }

    // Create the database with proper character set
    std::string createDbSql = Trinity::StringFormat(
        "CREATE DATABASE IF NOT EXISTS `{}` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci",
        database);

    if (mysql_query(mysql, createDbSql.c_str()))
    {
        uint32 errorCode = mysql_errno(mysql);
        if (errorCode == 1044) // ER_DBACCESS_DENIED_ERROR - Access denied
        {
            TC_LOG_ERROR("module.playerbot.database",
                "PlayerbotDatabaseConnection: Access denied for CREATE DATABASE. "
                "User '{}' needs CREATE privilege.", username);
        }
        else
        {
            TC_LOG_ERROR("module.playerbot.database",
                "PlayerbotDatabaseConnection: Failed to create database '{}': {}",
                database, mysql_error(mysql));
        }
        mysql_close(mysql);
        return false;
    }

    TC_LOG_INFO("module.playerbot.database",
        "PlayerbotDatabaseConnection: Successfully created database '{}'", database);

    mysql_close(mysql);
    return true;
}

void PlayerbotDatabaseConnection::DisplayDatabaseSetupInstructions(std::string const& database, std::string const& username)
{
    TC_LOG_ERROR("module.playerbot.database", "");
    TC_LOG_ERROR("module.playerbot.database", "================================================================================");
    TC_LOG_ERROR("module.playerbot.database", "  PLAYERBOT DATABASE SETUP REQUIRED");
    TC_LOG_ERROR("module.playerbot.database", "================================================================================");
    TC_LOG_ERROR("module.playerbot.database", "");
    TC_LOG_ERROR("module.playerbot.database", "  Database '{}' does not exist and auto-creation failed.", database);
    TC_LOG_ERROR("module.playerbot.database", "");
    TC_LOG_ERROR("module.playerbot.database", "  OPTION 1: Grant CREATE privilege for auto-creation");
    TC_LOG_ERROR("module.playerbot.database", "  ------------------------------------------------");
    TC_LOG_ERROR("module.playerbot.database", "  GRANT CREATE ON *.* TO '{}'@'localhost';", username);
    TC_LOG_ERROR("module.playerbot.database", "  FLUSH PRIVILEGES;");
    TC_LOG_ERROR("module.playerbot.database", "");
    TC_LOG_ERROR("module.playerbot.database", "  OPTION 2: Create the database manually");
    TC_LOG_ERROR("module.playerbot.database", "  --------------------------------------");
    TC_LOG_ERROR("module.playerbot.database", "  CREATE DATABASE {} CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;", database);
    TC_LOG_ERROR("module.playerbot.database", "  GRANT ALL ON {}.* TO '{}'@'localhost';", database, username);
    TC_LOG_ERROR("module.playerbot.database", "  FLUSH PRIVILEGES;");
    TC_LOG_ERROR("module.playerbot.database", "");
    TC_LOG_ERROR("module.playerbot.database", "  After creating the database, restart the server.");
    TC_LOG_ERROR("module.playerbot.database", "  Schema migrations will be applied automatically.");
    TC_LOG_ERROR("module.playerbot.database", "");
    TC_LOG_ERROR("module.playerbot.database", "================================================================================");
    TC_LOG_ERROR("module.playerbot.database", "");

    SetError(Trinity::StringFormat("Database '{}' does not exist", database));
}

void PlayerbotDatabaseConnection::Close()
{
    if (_mysqlHandle)
    {
        mysql_close(static_cast<MYSQL*>(_mysqlHandle));
        _mysqlHandle = nullptr;
    }
    _connected = false;
}

QueryResult PlayerbotDatabaseConnection::Query(std::string const& sql)
{
    TC_LOG_INFO("server.loading", "PlayerbotDatabaseConnection::Query: Executing SQL: {}", sql);

    if (!_connected || !_mysqlHandle)
    {
        SetError("Not connected to database");
        TC_LOG_ERROR("module.playerbot.database", "PlayerbotDatabaseConnection::Query: Not connected to database");
        return nullptr;
    }

    MYSQL* mysql = static_cast<MYSQL*>(_mysqlHandle);

    if (mysql_query(mysql, sql.c_str()))
    {
        uint32 errorCode = mysql_errno(mysql);
        std::string error = Trinity::StringFormat("Query failed [Error {}]: {}", errorCode, mysql_error(mysql));
        SetError(error);
        TC_LOG_ERROR("server.loading", "PlayerbotDatabaseConnection::Query: {}", error);

        // Log specific schema-related errors
    if (errorCode == 1054) // ER_BAD_FIELD_ERROR - Unknown column
        {
            TC_LOG_ERROR("server.loading", "SCHEMA MISMATCH: Column does not exist in table. SQL: {}", sql);
        }
        else if (errorCode == 1146) // ER_NO_SUCH_TABLE - Table doesn't exist
        {
            TC_LOG_ERROR("server.loading", "SCHEMA MISMATCH: Table does not exist. SQL: {}", sql);
        }
        else if (errorCode == 1064) // ER_PARSE_ERROR - SQL syntax error
        {
            TC_LOG_ERROR("server.loading", "SQL SYNTAX ERROR: Check query syntax. SQL: {}", sql);
        }

        return nullptr;
    }

    MYSQL_RES* result = mysql_store_result(mysql);
    TC_LOG_INFO("module.playerbot.database", "PlayerbotDatabaseConnection::Query: mysql_store_result returned: {}",
                 result ? "valid result" : "nullptr");

    if (!result)
    {
        uint32 fieldCount = mysql_field_count(mysql);
        TC_LOG_INFO("module.playerbot.database", "PlayerbotDatabaseConnection::Query: mysql_field_count: {}", fieldCount);

        // Check if it was a query that should return results
    if (fieldCount > 0)
        {
            std::string error = Trinity::StringFormat("Failed to store result: {}", mysql_error(mysql));
            SetError(error);
            TC_LOG_ERROR("module.playerbot.database", "PlayerbotDatabaseConnection::Query: {}", error);
            return nullptr;
        }
        else
        {
            // Query didn't return results (INSERT, UPDATE, DELETE, etc.)
            TC_LOG_INFO("module.playerbot.database", "PlayerbotDatabaseConnection::Query: Non-SELECT query completed successfully");
            return nullptr;
        }
    }

    // Even if we have a result, it might be empty (0 rows)
    // This is still a valid result set, just with no data
    uint32 rowCount = mysql_num_rows(result);
    TC_LOG_INFO("module.playerbot.database", "PlayerbotDatabaseConnection::Query: Result has {} rows", rowCount);

    // Create a proper ResultSet from our MySQL result
    if (result)
    {
        // Create our custom PlayerbotResultSet that properly wraps the MySQL result
        try
        {
            auto resultSet = std::make_shared<PlayerbotResultSet>(result);
            TC_LOG_INFO("module.playerbot.database",
                "PlayerbotDatabaseConnection: Query returned {} rows", resultSet->GetRowCount());
            return resultSet;
        }
        catch (std::exception const& e)
        {
            TC_LOG_ERROR("module.playerbot.database",
                "PlayerbotDatabaseConnection: Failed to create ResultSet: {}", e.what());
            mysql_free_result(result);
            return nullptr;
        }
    }

    TC_LOG_ERROR("module.playerbot.database", "PlayerbotDatabaseConnection::Query: Unexpected end of function");
    return nullptr;
}

bool PlayerbotDatabaseConnection::Execute(std::string const& sql)
{
    if (!_connected || !_mysqlHandle)
    {
        SetError("Not connected to database");
        return false;
    }

    MYSQL* mysql = static_cast<MYSQL*>(_mysqlHandle);

    if (mysql_query(mysql, sql.c_str()))
    {
        SetError(Trinity::StringFormat("Execute failed: {}", mysql_error(mysql)));
        return false;
    }

    // Clear any result set (for multi-statement support)
    MYSQL_RES* result = mysql_store_result(mysql);
    if (result)
        mysql_free_result(result);

    return true;
}

void PlayerbotDatabaseConnection::SetError(std::string const& error)
{
    _lastError = error;
    TC_LOG_ERROR("module.playerbot.database", "PlayerbotDatabaseConnection: {}", error);
}