/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#include "BotSessionManager.h"
#include "BotSession.h"
#include "WorldSession.h"
#include "Player.h"
#include "Log.h"
#include "AI/BotAI.h"
#include <unordered_map>
#include <mutex>

namespace Playerbot {

// Thread-safe registry of bot AI instances
static ::std::unordered_map<WorldSession*, BotAI*> s_botAIRegistry;
static ::std::recursive_mutex s_botAIMutex;

void BotSessionManager::RegisterBotAI(WorldSession* session, BotAI* ai)
{
    if (!session || !ai)
        return;

    ::std::lock_guard lock(s_botAIMutex);
    s_botAIRegistry[session] = ai;
    TC_LOG_DEBUG("module.playerbot.session",
        "Registered BotAI for session {}", session->GetAccountId());
}

void BotSessionManager::UnregisterBotAI(WorldSession* session)
{
    if (!session)
        return;

    ::std::lock_guard lock(s_botAIMutex);
    auto it = s_botAIRegistry.find(session);
    if (it != s_botAIRegistry.end())
    {
        s_botAIRegistry.erase(it);
        TC_LOG_DEBUG("module.playerbot.session",
            "Unregistered BotAI for session {}", session->GetAccountId());
    }
}

BotAI* BotSessionManager::GetBotAI(WorldSession* session)
{
    if (!session)
        return nullptr;

    ::std::lock_guard lock(s_botAIMutex);
    auto it = s_botAIRegistry.find(session);
    if (it != s_botAIRegistry.end())
        return it->second;
    return nullptr;
}

BotSession* BotSessionManager::GetBotSession(WorldSession* session)
{
    if (!session)
        return nullptr;

#ifdef BUILD_PLAYERBOT
    if (!session->IsBot())
        return nullptr;

    // Safe cast - only attempt if we know it's a bot session
    return static_cast<BotSession*>(session);
#else
    return nullptr; // Playerbot not enabled
#endif
}

} // namespace Playerbot
