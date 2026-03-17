/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOTRESULTSET_H
#define PLAYERBOTRESULTSET_H

#include "QueryResult.h"
#include "MySQLWorkaround.h"

class TC_DATABASE_API PlayerbotResultSet : public ResultSet
{
public:
    PlayerbotResultSet(MYSQL_RES* result);
    ~PlayerbotResultSet() = default;

private:
    static MySQLResult* ConvertMySQLResult(MYSQL_RES* mysqlResult);
    static MySQLField* ConvertMySQLFields(MYSQL_RES* mysqlResult);
    static uint64 GetMySQLRowCount(MYSQL_RES* mysqlResult);
    static uint32 GetMySQLFieldCount(MYSQL_RES* mysqlResult);
};

#endif // PLAYERBOTRESULTSET_H