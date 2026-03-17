/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include <string>
#include <string_view>
#include "PlayerbotResultSet.h"
#include "MySQLWorkaround.h"
#include "Log.h"

PlayerbotResultSet::PlayerbotResultSet(MYSQL_RES* result)
    : ResultSet(ConvertMySQLResult(result), ConvertMySQLFields(result),
                GetMySQLRowCount(result), GetMySQLFieldCount(result))
{
}

MySQLResult* PlayerbotResultSet::ConvertMySQLResult(MYSQL_RES* mysqlResult)
{
    // Cast MYSQL_RES* to MySQLResult* - they should be compatible
    return reinterpret_cast<MySQLResult*>(mysqlResult);
}

MySQLField* PlayerbotResultSet::ConvertMySQLFields(MYSQL_RES* mysqlResult)
{
    if (!mysqlResult)
        return nullptr;

    // Get field information from MySQL result
    MYSQL_FIELD* fields = mysql_fetch_fields(mysqlResult);

    // Cast MYSQL_FIELD* to MySQLField* - they should be compatible
    return reinterpret_cast<MySQLField*>(fields);
}

uint64 PlayerbotResultSet::GetMySQLRowCount(MYSQL_RES* mysqlResult)
{
    if (!mysqlResult)
        return 0;

    return mysql_num_rows(mysqlResult);
}

uint32 PlayerbotResultSet::GetMySQLFieldCount(MYSQL_RES* mysqlResult)
{
    if (!mysqlResult)
        return 0;

    return mysql_num_fields(mysqlResult);
}