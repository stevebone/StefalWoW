/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#ifndef BOT_LOGIN_QUERY_HOLDER_H
#define BOT_LOGIN_QUERY_HOLDER_H

#include "DatabaseEnv.h"
#include "Player.h"

namespace Playerbot {

/**
 * BotLoginQueryHolder - Special query holder for bot character loading
 *
 * This class replicates the LoginQueryHolder from CharacterHandler.cpp
 * but adapted for bot usage. It properly handles all the async-prepared
 * statements that are required for character loading.
 *
 * The key difference from regular player login is that bots execute
 * these queries synchronously since they don't have network delays.
 */
class BotLoginQueryHolder : public CharacterDatabaseQueryHolder
{
private:
    uint32 m_accountId;
    ObjectGuid m_guid;

public:
    BotLoginQueryHolder(uint32 accountId, ObjectGuid guid)
        : m_accountId(accountId), m_guid(guid) { }

    ObjectGuid GetGuid() const { return m_guid; }
    uint32 GetAccountId() const { return m_accountId; }

    bool Initialize();
};

} // namespace Playerbot

#endif // BOT_LOGIN_QUERY_HOLDER_H