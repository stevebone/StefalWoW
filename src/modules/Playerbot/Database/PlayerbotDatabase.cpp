/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PlayerbotDatabase.h"
#include "Log.h"
#include <set>

PlayerbotDatabaseManager* PlayerbotDatabaseManager::instance()
{
    static PlayerbotDatabaseManager instance;
    return &instance;
}

bool PlayerbotDatabaseManager::Initialize(std::string const& connectionInfo)
{
    TC_LOG_DEBUG("module.playerbot.database", "PlayerbotDatabaseManager: Initializing connection");

    if (_connection && _connection->IsConnected())
    {
        TC_LOG_WARN("module.playerbot.database", "PlayerbotDatabaseManager: Already initialized");
        return true;
    }

    _connection = std::make_unique<PlayerbotDatabaseConnection>();

    if (!_connection->Initialize(connectionInfo))
    {
        TC_LOG_ERROR("module.playerbot.database", "PlayerbotDatabaseManager: Failed to initialize connection: {}",
                    _connection->GetLastError());
        _connection.reset();
        return false;
    }

    TC_LOG_INFO("module.playerbot.database", "PlayerbotDatabaseManager: Successfully initialized");
    return true;
}

void PlayerbotDatabaseManager::Close()
{
    if (_connection)
    {
        TC_LOG_DEBUG("module.playerbot.database", "PlayerbotDatabaseManager: Closing connection");
        _connection->Close();
        _connection.reset();
    }
}

QueryResult PlayerbotDatabaseManager::Query(std::string const& sql)
{
    // Thread safety: Lock mutex to prevent concurrent access from multiple threads
    // This is required because CreateBotAccountsBatch runs in a separate thread
    std::lock_guard<std::mutex> lock(_connectionMutex);

    TC_LOG_DEBUG("server.loading", "PlayerbotDatabaseManager::Query called with SQL: {}", sql);

    // CRITICAL: Check for NULL connection before dereferencing
    // This can happen if database initialization failed (wrong config, server unreachable, etc.)
    if (!_connection)
    {
        TC_LOG_ERROR("server.loading", "PlayerbotDatabaseManager: Database connection is NULL - database was not initialized!");
        TC_LOG_ERROR("server.loading", "PlayerbotDatabaseManager: Check your Playerbot.Database.* configuration settings");
        return nullptr;
    }

    if (!_connection->IsConnected())
    {
        TC_LOG_ERROR("server.loading", "PlayerbotDatabaseManager: Connection is not active");
        return nullptr;
    }

    TC_LOG_DEBUG("server.loading", "PlayerbotDatabaseManager: Forwarding query to connection");
    QueryResult result = _connection->Query(sql);

    if (!result)
    {
        TC_LOG_DEBUG("server.loading", "PlayerbotDatabaseManager: Query returned null result");
    }
    else
    {
        TC_LOG_DEBUG("server.loading", "PlayerbotDatabaseManager: Query returned valid result");
    }

    return result;
}

bool PlayerbotDatabaseManager::Execute(std::string const& sql)
{
    // Thread safety: Lock mutex to prevent concurrent access from multiple threads
    // This is required because CreateBotAccountsBatch runs in a separate thread
    std::lock_guard<std::mutex> lock(_connectionMutex);

    // CRITICAL: Check for NULL connection before dereferencing
    if (!_connection)
    {
        TC_LOG_ERROR("server.loading", "PlayerbotDatabaseManager::Execute: Database connection is NULL - database was not initialized!");
        TC_LOG_ERROR("server.loading", "PlayerbotDatabaseManager::Execute: Check your Playerbot.Database.* configuration settings");
        return false;
    }

    if (!_connection->IsConnected())
    {
        TC_LOG_ERROR("server.loading", "PlayerbotDatabaseManager::Execute: Connection is not active");
        return false;
    }

    return _connection->Execute(sql);
}

bool PlayerbotDatabaseManager::IsConnected() const
{
    std::lock_guard<std::mutex> lock(_connectionMutex);
    return _connection && _connection->IsConnected();
}

bool PlayerbotDatabaseManager::ValidateSchema()
{
    if (!IsConnected())
    {
        TC_LOG_ERROR("server.loading", "PlayerbotDatabaseManager: Cannot validate schema - not connected");
        return false;
    }

    bool schemaValid = true;

    // Validate playerbots_race_class_distribution table
    TC_LOG_INFO("server.loading", "Validating playerbots_race_class_distribution schema...");
    QueryResult result = Query("DESCRIBE playerbots_race_class_distribution");
    if (!result)
    {
        TC_LOG_ERROR("server.loading", "SCHEMA ERROR: Table playerbots_race_class_distribution does not exist");
        schemaValid = false;
    }
    else
    {
        std::set<std::string> requiredColumns = {"race_id", "class_id", "distribution_weight", "enabled"};
        std::set<std::string> foundColumns;

        do
        {
            Field* fields = result->Fetch();
            std::string columnName = fields[0].GetString();
            foundColumns.insert(columnName);
        } while (result->NextRow());

        for (const auto& required : requiredColumns)
        {
            if (foundColumns.find(required) == foundColumns.end())
            {
                TC_LOG_ERROR("server.loading", "SCHEMA ERROR: Missing required column '{}' in playerbots_race_class_distribution", required);
                schemaValid = false;
            }
        }
    }

    // Validate playerbots_gender_distribution table
    TC_LOG_INFO("server.loading", "Validating playerbots_gender_distribution schema...");
    result = Query("DESCRIBE playerbots_gender_distribution");
    if (!result)
    {
        TC_LOG_ERROR("server.loading", "SCHEMA ERROR: Table playerbots_gender_distribution does not exist");
        schemaValid = false;
    }
    else
    {
        std::set<std::string> requiredColumns = {"race_id", "male_percentage", "female_percentage"};
        std::set<std::string> foundColumns;

        do
        {
            Field* fields = result->Fetch();
            std::string columnName = fields[0].GetString();
            foundColumns.insert(columnName);
        } while (result->NextRow());

        for (const auto& required : requiredColumns)
        {
            if (foundColumns.find(required) == foundColumns.end())
            {
                TC_LOG_ERROR("server.loading", "SCHEMA ERROR: Missing required column '{}' in playerbots_gender_distribution", required);
                schemaValid = false;
            }
        }
    }

    // Validate playerbots_class_popularity table
    TC_LOG_INFO("server.loading", "Validating playerbots_class_popularity schema...");
    result = Query("DESCRIBE playerbots_class_popularity");
    if (!result)
    {
        TC_LOG_ERROR("server.loading", "SCHEMA ERROR: Table playerbots_class_popularity does not exist");
        schemaValid = false;
    }
    else
    {
        std::set<std::string> requiredColumns = {"class_id", "class_name", "popularity_weight", "enabled"};
        std::set<std::string> foundColumns;

        do
        {
            Field* fields = result->Fetch();
            std::string columnName = fields[0].GetString();
            foundColumns.insert(columnName);
        } while (result->NextRow());

        for (const auto& required : requiredColumns)
        {
            if (foundColumns.find(required) == foundColumns.end())
            {
                TC_LOG_ERROR("server.loading", "SCHEMA ERROR: Missing required column '{}' in playerbots_class_popularity", required);
                schemaValid = false;
            }
        }
    }

    if (schemaValid)
    {
        TC_LOG_INFO("server.loading", "Database schema validation PASSED");
    }
    else
    {
        TC_LOG_ERROR("server.loading", "Database schema validation FAILED - check migration scripts");
    }

    return schemaValid;
}
