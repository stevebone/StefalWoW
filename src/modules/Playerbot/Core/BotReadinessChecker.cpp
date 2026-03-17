/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#include "BotReadinessChecker.h"
#include "CharacterCache.h"
#include "LFGMgr.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "WorldSession.h"
#include "../Session/BotSession.h"
#include "../Session/BotWorldSessionMgr.h"
#include "PlayerBotHooks.h"
#include "../AI/BotAI.h"

namespace Playerbot
{

// ============================================================================
// PUBLIC API
// ============================================================================

BotReadinessResult BotReadinessChecker::Check(ObjectGuid botGuid, BotReadinessFlag requiredFlags)
{
    BotReadinessResult result;
    result.botGuid = botGuid;

    if (botGuid.IsEmpty())
    {
        AddFailure(result, BotReadinessFlag::FOUND_CONNECTED, "GUID is empty");
        return result;
    }

    // Step 1: ObjectAccessor checks (finds the player)
    CheckObjectAccessor(botGuid, result);

    // Step 2: CharacterCache check
    CheckCharacterCache(botGuid, result);

    // If we couldn't find the player, we can't do the remaining checks
    if (!result.player)
    {
        return result;
    }

    // Step 3: Player state checks
    CheckPlayerState(result.player, result);

    // Step 4: Bot-specific checks
    CheckBotSpecific(result.player, result);

    // Step 5: Queue/Group state checks
    CheckQueueState(result.player, result);

    return result;
}

BotReadinessResult BotReadinessChecker::Check(Player* player, BotReadinessFlag requiredFlags)
{
    BotReadinessResult result;

    if (!player)
    {
        AddFailure(result, BotReadinessFlag::FOUND_CONNECTED, "Player pointer is null");
        return result;
    }

    result.botGuid = player->GetGUID();
    result.player = player;

    // Mark as found since we have a direct pointer
    AddSuccess(result, BotReadinessFlag::FOUND_CONNECTED);

    // Check if also findable via ObjectAccessor (confirms proper registration)
    if (ObjectAccessor::FindPlayer(player->GetGUID()))
    {
        AddSuccess(result, BotReadinessFlag::FOUND_IN_WORLD);
    }
    else
    {
        AddFailure(result, BotReadinessFlag::FOUND_IN_WORLD,
                   "Player exists but FindPlayer() failed (not in world?)");
    }

    // CharacterCache check
    CheckCharacterCache(result.botGuid, result);

    // Player state checks
    CheckPlayerState(player, result);

    // Bot-specific checks
    CheckBotSpecific(player, result);

    // Queue/Group state checks
    CheckQueueState(player, result);

    return result;
}

bool BotReadinessChecker::IsReady(ObjectGuid botGuid, BotReadinessFlag requiredFlags)
{
    BotReadinessResult result = Check(botGuid, requiredFlags);
    return HasFlag(result.passedChecks, requiredFlags);
}

bool BotReadinessChecker::IsLFGReady(ObjectGuid botGuid)
{
    return IsReady(botGuid, BotReadinessFlag::LFG_READY);
}

bool BotReadinessChecker::IsCombatReady(ObjectGuid botGuid)
{
    return IsReady(botGuid, BotReadinessFlag::COMBAT_READY);
}

// ============================================================================
// INDIVIDUAL CHECK IMPLEMENTATIONS
// ============================================================================

void BotReadinessChecker::CheckObjectAccessor(ObjectGuid botGuid, BotReadinessResult& result)
{
    // Check 1: FindConnectedPlayer (finds any connected player, even if not in world)
    Player* connectedPlayer = ObjectAccessor::FindConnectedPlayer(botGuid);
    if (connectedPlayer)
    {
        AddSuccess(result, BotReadinessFlag::FOUND_CONNECTED);
        result.player = connectedPlayer;
    }
    else
    {
        AddFailure(result, BotReadinessFlag::FOUND_CONNECTED,
                   "ObjectAccessor::FindConnectedPlayer() returned null - bot not in HashMapHolder");
    }

    // Check 2: FindPlayer (requires IsInWorld() to be true)
    Player* inWorldPlayer = ObjectAccessor::FindPlayer(botGuid);
    if (inWorldPlayer)
    {
        AddSuccess(result, BotReadinessFlag::FOUND_IN_WORLD);
        if (!result.player)
            result.player = inWorldPlayer;
    }
    else
    {
        AddFailure(result, BotReadinessFlag::FOUND_IN_WORLD,
                   "ObjectAccessor::FindPlayer() returned null - bot not in world or not registered");
    }
}

void BotReadinessChecker::CheckCharacterCache(ObjectGuid botGuid, BotReadinessResult& result)
{
    CharacterCacheEntry const* cacheEntry = sCharacterCache->GetCharacterCacheByGuid(botGuid);
    if (cacheEntry)
    {
        AddSuccess(result, BotReadinessFlag::IN_CHARACTER_CACHE);
    }
    else
    {
        AddFailure(result, BotReadinessFlag::IN_CHARACTER_CACHE,
                   "Not found in CharacterCache - may cause '??' display in UI");
    }
}

void BotReadinessChecker::CheckPlayerState(Player* player, BotReadinessResult& result)
{
    // IsInWorld check
    if (player->IsInWorld())
    {
        AddSuccess(result, BotReadinessFlag::IS_IN_WORLD);
    }
    else
    {
        AddFailure(result, BotReadinessFlag::IS_IN_WORLD,
                   "player->IsInWorld() is false");
    }

    // Map check
    if (player->GetMap())
    {
        AddSuccess(result, BotReadinessFlag::HAS_MAP);
    }
    else
    {
        AddFailure(result, BotReadinessFlag::HAS_MAP,
                   "player->GetMap() is null");
    }

    // Session check
    if (player->GetSession())
    {
        AddSuccess(result, BotReadinessFlag::HAS_SESSION);
    }
    else
    {
        AddFailure(result, BotReadinessFlag::HAS_SESSION,
                   "player->GetSession() is null");
    }

    // Teleport state checks
    if (!player->IsBeingTeleportedFar())
    {
        AddSuccess(result, BotReadinessFlag::NOT_TELEPORTING_FAR);
    }
    else
    {
        AddFailure(result, BotReadinessFlag::NOT_TELEPORTING_FAR,
                   "Bot is being teleported (far) - wait for teleport completion");
    }

    if (!player->IsBeingTeleportedNear())
    {
        AddSuccess(result, BotReadinessFlag::NOT_TELEPORTING_NEAR);
    }
    else
    {
        AddFailure(result, BotReadinessFlag::NOT_TELEPORTING_NEAR,
                   "Bot is being teleported (near) - wait for teleport completion");
    }

    // Logout check
    WorldSession* session = player->GetSession();
    if (session && !session->isLogingOut())
    {
        AddSuccess(result, BotReadinessFlag::NOT_LOGGING_OUT);
    }
    else
    {
        AddFailure(result, BotReadinessFlag::NOT_LOGGING_OUT,
                   "Bot session is logging out");
    }

    // Alive/Ghost check (can the bot take actions?)
    if (player->IsAlive() || player->HasAura(8326)) // Ghost aura
    {
        AddSuccess(result, BotReadinessFlag::IS_ALIVE_OR_GHOST);
    }
    else
    {
        AddFailure(result, BotReadinessFlag::IS_ALIVE_OR_GHOST,
                   "Bot is dead (not ghost form) - cannot take actions");
    }
}

void BotReadinessChecker::CheckBotSpecific(Player* player, BotReadinessResult& result)
{
    // Verify this is actually a bot
    if (PlayerBotHooks::IsPlayerBot(player))
    {
        AddSuccess(result, BotReadinessFlag::IS_BOT);
    }
    else
    {
        AddFailure(result, BotReadinessFlag::IS_BOT,
                   "Player is not identified as a bot by PlayerBotHooks");
    }

    // Check BotSession state
    WorldSession* session = player->GetSession();
    if (session)
    {
        // Try to cast to BotSession to verify it's a bot session
        BotSession* botSession = dynamic_cast<BotSession*>(session);
        if (botSession)
        {
            if (botSession->IsActive() && !botSession->PlayerDisconnected())
            {
                AddSuccess(result, BotReadinessFlag::BOT_SESSION_ACTIVE);
            }
            else
            {
                AddFailure(result, BotReadinessFlag::BOT_SESSION_ACTIVE,
                           "BotSession exists but is not active or is disconnected");
            }

            // Check if BotAI is initialized
            if (botSession->GetAI())
            {
                AddSuccess(result, BotReadinessFlag::BOT_AI_INITIALIZED);
            }
            else
            {
                AddFailure(result, BotReadinessFlag::BOT_AI_INITIALIZED,
                           "BotAI is not attached to session");
            }
        }
        else
        {
            // Not a BotSession - might be a regular session with bot flag?
            // This is unusual but check if bot is managed by BotWorldSessionMgr
            if (sBotWorldSessionMgr->GetPlayerBot(player->GetGUID()) != nullptr)
            {
                AddSuccess(result, BotReadinessFlag::BOT_SESSION_ACTIVE);
                // Can't check AI without BotSession
                AddFailure(result, BotReadinessFlag::BOT_AI_INITIALIZED,
                           "Session is not BotSession - cannot verify AI");
            }
            else
            {
                AddFailure(result, BotReadinessFlag::BOT_SESSION_ACTIVE,
                           "Session is not a BotSession and not managed by BotWorldSessionMgr");
            }
        }
    }
    else
    {
        AddFailure(result, BotReadinessFlag::BOT_SESSION_ACTIVE,
                   "No session attached to player");
        AddFailure(result, BotReadinessFlag::BOT_AI_INITIALIZED,
                   "No session - cannot check AI");
    }
}

void BotReadinessChecker::CheckQueueState(Player* player, BotReadinessResult& result)
{
    // Check LFG queue state
    lfg::LfgState lfgState = sLFGMgr->GetState(player->GetGUID());
    if (lfgState == lfg::LFG_STATE_NONE)
    {
        AddSuccess(result, BotReadinessFlag::NOT_IN_QUEUE);
    }
    else
    {
        AddFailure(result, BotReadinessFlag::NOT_IN_QUEUE,
                   "Bot is already in LFG queue (state: " + std::to_string(static_cast<int>(lfgState)) + ")");
    }

    // Check group state
    if (!player->GetGroup())
    {
        AddSuccess(result, BotReadinessFlag::NOT_IN_GROUP);
    }
    else
    {
        AddFailure(result, BotReadinessFlag::NOT_IN_GROUP,
                   "Bot is already in a group");
    }

    // Check combat state
    if (!player->IsInCombat())
    {
        AddSuccess(result, BotReadinessFlag::NOT_IN_COMBAT);
    }
    else
    {
        AddFailure(result, BotReadinessFlag::NOT_IN_COMBAT,
                   "Bot is currently in combat");
    }
}

// ============================================================================
// HELPERS
// ============================================================================

void BotReadinessChecker::AddFailure(BotReadinessResult& result, BotReadinessFlag flag, std::string reason)
{
    result.failedChecks |= flag;
    result.failureReasons.push_back(std::string(GetFlagName(flag)) + ": " + reason);
}

void BotReadinessChecker::AddSuccess(BotReadinessResult& result, BotReadinessFlag flag)
{
    result.passedChecks |= flag;
}

char const* BotReadinessChecker::GetFlagName(BotReadinessFlag flag)
{
    switch (flag)
    {
        case BotReadinessFlag::FOUND_CONNECTED:      return "FOUND_CONNECTED";
        case BotReadinessFlag::FOUND_IN_WORLD:       return "FOUND_IN_WORLD";
        case BotReadinessFlag::IN_CHARACTER_CACHE:   return "IN_CHARACTER_CACHE";
        case BotReadinessFlag::IS_IN_WORLD:          return "IS_IN_WORLD";
        case BotReadinessFlag::HAS_MAP:              return "HAS_MAP";
        case BotReadinessFlag::HAS_SESSION:          return "HAS_SESSION";
        case BotReadinessFlag::NOT_TELEPORTING_FAR:  return "NOT_TELEPORTING_FAR";
        case BotReadinessFlag::NOT_TELEPORTING_NEAR: return "NOT_TELEPORTING_NEAR";
        case BotReadinessFlag::NOT_LOGGING_OUT:      return "NOT_LOGGING_OUT";
        case BotReadinessFlag::IS_ALIVE_OR_GHOST:    return "IS_ALIVE_OR_GHOST";
        case BotReadinessFlag::IS_BOT:               return "IS_BOT";
        case BotReadinessFlag::BOT_SESSION_ACTIVE:   return "BOT_SESSION_ACTIVE";
        case BotReadinessFlag::BOT_AI_INITIALIZED:   return "BOT_AI_INITIALIZED";
        case BotReadinessFlag::NOT_IN_QUEUE:         return "NOT_IN_QUEUE";
        case BotReadinessFlag::NOT_IN_GROUP:         return "NOT_IN_GROUP";
        case BotReadinessFlag::NOT_IN_COMBAT:        return "NOT_IN_COMBAT";
        default:                                      return "UNKNOWN";
    }
}

std::string BotReadinessResult::GetSummary() const
{
    uint32 passed = 0;
    uint32 failed = 0;

    // Count bits
    uint32 passedBits = static_cast<uint32>(passedChecks);
    uint32 failedBits = static_cast<uint32>(failedChecks);

    while (passedBits) { passed += passedBits & 1; passedBits >>= 1; }
    while (failedBits) { failed += failedBits & 1; failedBits >>= 1; }

    std::string status;
    if (IsFullyReady())
        status = "FULLY_READY";
    else if (IsLFGReady())
        status = "LFG_READY";
    else if (IsBasicReady())
        status = "BASIC_READY";
    else
        status = "NOT_READY";

    return "Bot " + botGuid.ToString() + ": " + status +
           " (passed: " + std::to_string(passed) + ", failed: " + std::to_string(failed) + ")";
}

std::string BotReadinessResult::GetFailureReport() const
{
    if (failureReasons.empty())
        return "No failures";

    std::string report = "Failed checks for bot " + botGuid.ToString() + ":\n";
    for (auto const& reason : failureReasons)
    {
        report += "  - " + reason + "\n";
    }
    return report;
}

} // namespace Playerbot
