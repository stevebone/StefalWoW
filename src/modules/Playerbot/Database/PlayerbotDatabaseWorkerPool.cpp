/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PlayerbotDatabase.h"
#include "DatabaseWorkerPool.h"
#include "Log.h"
#include "MySQLPreparedStatement.h"
#include "PreparedStatement.h"
#include "QueryResult.h"
#include "Transaction.h"
#include <memory>

// Explicit template instantiation for all DatabaseWorkerPool methods
template class TC_DATABASE_API DatabaseWorkerPool<PlayerbotDatabaseConnection>;

// Explicit template instantiations for prepared statements
template TC_DATABASE_API
void DatabaseWorkerPool<PlayerbotDatabaseConnection>::Execute(PreparedStatement<PlayerbotDatabaseConnection>*);

template TC_DATABASE_API
PreparedQueryResult DatabaseWorkerPool<PlayerbotDatabaseConnection>::Query(PreparedStatement<PlayerbotDatabaseConnection>*);

template TC_DATABASE_API
void DatabaseWorkerPool<PlayerbotDatabaseConnection>::ExecuteOrAppend(SQLTransaction<PlayerbotDatabaseConnection>&, PreparedStatement<PlayerbotDatabaseConnection>*);

// Explicit template instantiations for transaction support
template TC_DATABASE_API
SQLTransaction<PlayerbotDatabaseConnection> DatabaseWorkerPool<PlayerbotDatabaseConnection>::BeginTransaction();

template TC_DATABASE_API
void DatabaseWorkerPool<PlayerbotDatabaseConnection>::CommitTransaction(SQLTransaction<PlayerbotDatabaseConnection>);