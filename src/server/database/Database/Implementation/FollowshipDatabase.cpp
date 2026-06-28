/*
 * This file is part of the Stefal WoW Project.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 */

#include "FollowshipDatabase.h"
#include "MySQLPreparedStatement.h"

void FollowshipDatabaseConnection::DoPrepareStatements()
{
    if (!m_reconnecting)
        m_stmts.resize(MAX_FOLLOWSHIPDATABASE_STATEMENTS);

    PrepareStatement(FSB_SEL_BOT_OWNERS_ALL,
        "SELECT bot_id, bot_guid, bot_entry, player_guid, hire_expiry_time FROM bot_owners", CONNECTION_SYNCH);

    PrepareStatement(FSB_SEL_BOT_OWNERS_BY_PLAYER,
        "SELECT bot_id, bot_guid, bot_entry, hire_expiry_time FROM bot_owners WHERE player_guid = ?", CONNECTION_SYNCH);

    PrepareStatement(FSB_SEL_BOT_OWNER_ID,
        "SELECT bot_id FROM bot_owners WHERE bot_guid = ? AND player_guid = ?", CONNECTION_SYNCH);

    PrepareStatement(FSB_INS_BOT_OWNER,
        "INSERT INTO bot_owners (bot_guid, bot_entry, player_guid, hire_expiry_time) VALUES (?, ?, ?, ?)", CONNECTION_ASYNC);

    PrepareStatement(FSB_DEL_BOT_OWNER_BY_GUID,
        "DELETE FROM bot_owners WHERE bot_guid = ?", CONNECTION_ASYNC);

    PrepareStatement(FSB_DEL_BOT_OWNER_BY_ENTRY_PLAYER,
        "DELETE FROM bot_owners WHERE bot_entry = ? AND player_guid = ?", CONNECTION_ASYNC);
}

FollowshipDatabaseConnection::FollowshipDatabaseConnection(MySQLConnectionInfo& connInfo, ConnectionFlags connectionFlags)
    : MySQLConnection(connInfo, connectionFlags)
{
}

FollowshipDatabaseConnection::~FollowshipDatabaseConnection()
{
}
