/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#ifndef BOT_SESSION_MANAGER_H
#define BOT_SESSION_MANAGER_H

#include "Define.h"

class WorldSession;

namespace Playerbot {

class BotAI;
class BotSession;

/**
 * Static manager for bot AI registry
 * Provides thread-safe lookup of BotAI instances by WorldSession
 */
class TC_GAME_API BotSessionManager
{
public:
    // Register/unregister bot AI with sessions
    static void RegisterBotAI(WorldSession* session, BotAI* ai);
    static void UnregisterBotAI(WorldSession* session);

    // Get registered BotAI for a session (thread-safe)
    static BotAI* GetBotAI(WorldSession* session);

    // Safe bot session retrieval
    static BotSession* GetBotSession(WorldSession* session);

private:
    BotSessionManager() = delete;
    BotSessionManager(const BotSessionManager&) = delete;
    BotSessionManager& operator=(const BotSessionManager&) = delete;
};

} // namespace Playerbot

#endif // BOT_SESSION_MANAGER_H
