/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ArenaBotManager.h"
#include "../Core/PlayerBotHooks.h"
#include "../Core/PlayerBotHelpers.h"
#include "../Session/BotWorldSessionMgr.h"
#include "Player.h"
#include "Group.h"
#include "GroupMgr.h"
#include "BattlegroundMgr.h"
#include "BattlegroundQueue.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "World.h"
#include "WorldSession.h"
#include "GameTime.h"
#include "SpellAuraDefines.h"
#include "DB2Stores.h"

namespace Playerbot
{

// ============================================================================
// SINGLETON
// ============================================================================

ArenaBotManager::ArenaBotManager()
    : _enabled(false)
    , _updateAccumulator(0)
    , _initialized(false)
{
}

ArenaBotManager::~ArenaBotManager()
{
    Shutdown();
}

ArenaBotManager* ArenaBotManager::instance()
{
    static ArenaBotManager instance;
    return &instance;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void ArenaBotManager::Initialize()
{
    std::lock_guard lock(_mutex);

    if (_initialized)
    {
        TC_LOG_WARN("module.playerbot.arena", "ArenaBotManager::Initialize - Already initialized");
        return;
    }

    TC_LOG_INFO("module.playerbot.arena", "Initializing Arena Bot Manager...");

    _queuedBots.clear();
    _humanPlayers.clear();
    _arenaInstanceBots.clear();
    _updateAccumulator = 0;

    _enabled = true;
    _initialized = true;

    TC_LOG_INFO("module.playerbot.arena", "Arena Bot Manager initialized (Enabled: {})", _enabled.load());
}

void ArenaBotManager::Shutdown()
{
    std::lock_guard lock(_mutex);

    if (!_initialized)
        return;

    TC_LOG_INFO("module.playerbot.arena", "Shutting down Arena Bot Manager...");

    for (auto const& [botGuid, queueInfo] : _queuedBots)
    {
        if (Player* bot = ObjectAccessor::FindPlayer(botGuid))
        {
            RemoveBotFromQueue(bot);
        }
    }

    _queuedBots.clear();
    _humanPlayers.clear();
    _arenaInstanceBots.clear();

    _initialized = false;
    _enabled = false;

    TC_LOG_INFO("module.playerbot.arena", "Arena Bot Manager shut down");
}

void ArenaBotManager::Update(uint32 diff)
{
    if (!_enabled || !_initialized)
        return;

    _updateAccumulator += diff;

    if (_updateAccumulator >= CLEANUP_INTERVAL)
    {
        CleanupStaleAssignments();
        _updateAccumulator = 0;
    }
}

// ============================================================================
// QUEUE MANAGEMENT
// ============================================================================

void ArenaBotManager::OnPlayerJoinQueue(Player* player, ArenaBracketType bracketType,
                                         ArenaQueueMode mode, bool asGroup)
{
    if (!_enabled || !_initialized)
        return;

    // Only process human players
    if (PlayerBotHooks::IsPlayerBot(player))
        return;

    ObjectGuid playerGuid = player->GetGUID();
    uint8 teamSize = GetTeamSize(bracketType);

    TC_LOG_INFO("module.playerbot.arena", "ArenaBotManager::OnPlayerJoinQueue - Player {} joined arena queue ({}v{}, Mode: {})",
                player->GetName(), teamSize, teamSize, mode == ArenaQueueMode::RATED ? "Rated" : "Skirmish");

    // Calculate teammates needed (team size - 1 for the human)
    uint32 teammatesNeeded = teamSize - 1;
    uint32 opponentsNeeded = teamSize;

    // Populate teammates
    uint32 teammatesQueued = PopulateTeammates(playerGuid, bracketType, mode, teammatesNeeded);

    // Populate opponents
    uint32 opponentsQueued = PopulateOpponents(bracketType, mode, opponentsNeeded);

    TC_LOG_INFO("module.playerbot.arena", "ArenaBotManager::OnPlayerJoinQueue - Queued {} teammates, {} opponents for player {}",
                teammatesQueued, opponentsQueued, player->GetName());
}

void ArenaBotManager::OnPlayerLeaveQueue(ObjectGuid playerGuid)
{
    if (!_enabled || !_initialized)
        return;

    std::lock_guard lock(_mutex);

    auto humanItr = _humanPlayers.find(playerGuid);
    if (humanItr != _humanPlayers.end())
    {
        TC_LOG_DEBUG("module.playerbot.arena", "ArenaBotManager::OnPlayerLeaveQueue - Human player left, removing bots");

        // Remove teammates
        for (ObjectGuid botGuid : humanItr->second.teammates)
        {
            if (Player* bot = ObjectAccessor::FindPlayer(botGuid))
            {
                RemoveBotFromQueue(bot);
            }
            _queuedBots.erase(botGuid);
        }

        // Remove opponents
        for (ObjectGuid botGuid : humanItr->second.opponents)
        {
            if (Player* bot = ObjectAccessor::FindPlayer(botGuid))
            {
                RemoveBotFromQueue(bot);
            }
            _queuedBots.erase(botGuid);
        }

        _humanPlayers.erase(humanItr);
    }
    else
    {
        auto botItr = _queuedBots.find(playerGuid);
        if (botItr != _queuedBots.end())
        {
            ObjectGuid humanGuid = botItr->second.humanPlayerGuid;
            auto humanPlayerItr = _humanPlayers.find(humanGuid);
            if (humanPlayerItr != _humanPlayers.end())
            {
                auto& teammates = humanPlayerItr->second.teammates;
                auto& opponents = humanPlayerItr->second.opponents;
                teammates.erase(std::remove(teammates.begin(), teammates.end(), playerGuid), teammates.end());
                opponents.erase(std::remove(opponents.begin(), opponents.end(), playerGuid), opponents.end());

                if (teammates.empty() && opponents.empty())
                    _humanPlayers.erase(humanPlayerItr);
            }

            _queuedBots.erase(botItr);
        }
    }
}

void ArenaBotManager::OnInvitationReceived(ObjectGuid playerGuid, uint32 arenaInstanceGuid)
{
    if (!_enabled || !_initialized)
        return;

    std::lock_guard lock(_mutex);

    auto itr = _queuedBots.find(playerGuid);
    if (itr == _queuedBots.end())
        return;

    itr->second.arenaInstanceGuid = arenaInstanceGuid;
    _arenaInstanceBots[arenaInstanceGuid].insert(playerGuid);

    if (Player* bot = ObjectAccessor::FindPlayer(playerGuid))
    {
        TC_LOG_DEBUG("module.playerbot.arena", "ArenaBotManager::OnInvitationReceived - Bot {} accepting arena invitation",
                     bot->GetName());

        BattlegroundTypeId bgTypeId = GetBGTypeForBracket(itr->second.bracketType);
        if (Battleground* bg = sBattlegroundMgr->GetBattleground(arenaInstanceGuid, bgTypeId))
        {
            // Construct the queue type ID for AddPlayer
            uint8 teamSize = GetTeamSize(itr->second.bracketType);
            bool isRated = (itr->second.mode == ArenaQueueMode::RATED);
            BattlegroundQueueIdType queueIdType = isRated ?
                BattlegroundQueueIdType::Arena : BattlegroundQueueIdType::ArenaSkirmish;

            BattlegroundQueueTypeId queueTypeId = BattlegroundMgr::BGQueueTypeId(
                BATTLEGROUND_AA,    // Arena uses BATTLEGROUND_AA
                queueIdType,
                isRated,
                teamSize
            );
            bg->AddPlayer(bot, queueTypeId);
        }
    }
}

void ArenaBotManager::OnArenaStart(Battleground* bg)
{
    if (!_enabled || !_initialized || !bg)
        return;

    std::lock_guard lock(_mutex);

    TC_LOG_DEBUG("module.playerbot.arena", "ArenaBotManager::OnArenaStart - Arena instance {} started",
                 bg->GetInstanceID());
}

void ArenaBotManager::OnArenaEnd(Battleground* bg, Team winnerTeam)
{
    if (!_enabled || !_initialized || !bg)
        return;

    std::lock_guard lock(_mutex);

    uint32 arenaInstanceGuid = bg->GetInstanceID();

    TC_LOG_DEBUG("module.playerbot.arena", "ArenaBotManager::OnArenaEnd - Arena instance {} ended",
                 arenaInstanceGuid);

    auto itr = _arenaInstanceBots.find(arenaInstanceGuid);
    if (itr != _arenaInstanceBots.end())
    {
        for (ObjectGuid botGuid : itr->second)
        {
            UnregisterBotAssignment(botGuid);
        }
        _arenaInstanceBots.erase(itr);
    }
}

// ============================================================================
// QUEUE POPULATION
// ============================================================================

uint32 ArenaBotManager::PopulateTeammates(ObjectGuid playerGuid, ArenaBracketType bracketType,
                                           ArenaQueueMode mode, uint32 teammatesNeeded)
{
    std::lock_guard lock(_mutex);

    Player* humanPlayer = ObjectAccessor::FindPlayer(playerGuid);
    if (!humanPlayer)
        return 0;

    // Arena level range for 12.0 is max level (70-80)
    uint8 humanLevel = humanPlayer->GetLevel();
    uint8 minLevel = std::max(static_cast<uint8>(1), static_cast<uint8>(humanLevel - 5));
    uint8 maxLevel = humanLevel + 5;

    uint8 teamSize = GetTeamSize(bracketType);
    std::vector<Player*> teammates = FindBotsForTeamComposition(teamSize, minLevel, maxLevel);

    uint32 botsQueued = 0;
    for (Player* bot : teammates)
    {
        if (botsQueued >= teammatesNeeded)
            break;

        if (QueueBot(bot, bracketType, mode, true))
        {
            RegisterBotAssignment(playerGuid, bot->GetGUID(), bracketType, mode, true);
            ++botsQueued;
            TC_LOG_DEBUG("module.playerbot.arena", "Queued teammate bot {} for arena", bot->GetName());
        }
    }

    if (botsQueued > 0)
    {
        auto& humanInfo = _humanPlayers[playerGuid];
        humanInfo.bracketType = bracketType;
        humanInfo.mode = mode;
    }

    return botsQueued;
}

uint32 ArenaBotManager::PopulateOpponents(ArenaBracketType bracketType, ArenaQueueMode mode,
                                           uint32 opponentsNeeded)
{
    std::lock_guard lock(_mutex);

    // For opponents, we need to find bots that aren't already assigned
    uint8 teamSize = GetTeamSize(bracketType);

    // Get all human players in this queue and their level range
    uint8 minLevel = 70;
    uint8 maxLevel = 80;

    for (auto const& [humanGuid, info] : _humanPlayers)
    {
        if (info.bracketType == bracketType && info.mode == mode)
        {
            if (Player* human = ObjectAccessor::FindPlayer(humanGuid))
            {
                uint8 level = human->GetLevel();
                minLevel = std::min(minLevel, static_cast<uint8>(std::max(1, static_cast<int>(level) - 5)));
                maxLevel = std::max(maxLevel, static_cast<uint8>(level + 5));
            }
        }
    }

    std::vector<Player*> opponents = FindBotsForTeamComposition(teamSize, minLevel, maxLevel);

    uint32 botsQueued = 0;
    for (Player* bot : opponents)
    {
        if (botsQueued >= opponentsNeeded)
            break;

        if (IsBotQueued(bot->GetGUID()))
            continue;

        if (QueueBot(bot, bracketType, mode, false))
        {
            // For opponents, use empty GUID as human association
            RegisterBotAssignment(ObjectGuid::Empty, bot->GetGUID(), bracketType, mode, false);
            ++botsQueued;
            TC_LOG_DEBUG("module.playerbot.arena", "Queued opponent bot {} for arena", bot->GetName());
        }
    }

    return botsQueued;
}

bool ArenaBotManager::IsBotQueued(ObjectGuid botGuid) const
{
    std::lock_guard lock(_mutex);
    return _queuedBots.find(botGuid) != _queuedBots.end();
}

void ArenaBotManager::GetStatistics(uint32& totalQueued, uint32& totalAssignments) const
{
    std::lock_guard lock(_mutex);
    totalQueued = static_cast<uint32>(_queuedBots.size());
    totalAssignments = static_cast<uint32>(_humanPlayers.size());
}

void ArenaBotManager::SetEnabled(bool enable)
{
    _enabled = enable;
    TC_LOG_INFO("module.playerbot.arena", "Arena Bot Manager {}", enable ? "enabled" : "disabled");

    if (!enable)
    {
        std::lock_guard lock(_mutex);
        for (auto const& [botGuid, queueInfo] : _queuedBots)
        {
            if (Player* bot = ObjectAccessor::FindPlayer(botGuid))
            {
                RemoveBotFromQueue(bot);
            }
        }
        _queuedBots.clear();
        _humanPlayers.clear();
        _arenaInstanceBots.clear();
    }
}

void ArenaBotManager::CleanupStaleAssignments()
{
    std::lock_guard lock(_mutex);

    time_t currentTime = time(nullptr);
    std::vector<ObjectGuid> staleBots;
    std::vector<ObjectGuid> staleHumans;

    for (auto const& [botGuid, queueInfo] : _queuedBots)
    {
        if ((currentTime - queueInfo.queueTime) > MAX_QUEUE_TIME)
        {
            staleBots.push_back(botGuid);
            continue;
        }

        if (!ObjectAccessor::FindPlayer(botGuid))
        {
            staleBots.push_back(botGuid);
        }
    }

    for (auto const& [humanGuid, playerInfo] : _humanPlayers)
    {
        if (!humanGuid.IsEmpty() && !ObjectAccessor::FindPlayer(humanGuid))
        {
            staleHumans.push_back(humanGuid);
        }
    }

    for (ObjectGuid botGuid : staleBots)
    {
        if (Player* bot = ObjectAccessor::FindPlayer(botGuid))
        {
            RemoveBotFromQueue(bot);
        }
        UnregisterBotAssignment(botGuid);
    }

    for (ObjectGuid humanGuid : staleHumans)
    {
        UnregisterAllBotsForPlayer(humanGuid);
    }

    if (!staleBots.empty() || !staleHumans.empty())
    {
        TC_LOG_DEBUG("module.playerbot.arena", "ArenaBotManager::CleanupStaleAssignments - Removed {} bots, {} humans",
                     staleBots.size(), staleHumans.size());
    }
}

// ============================================================================
// HELPER METHODS
// ============================================================================

uint8 ArenaBotManager::GetTeamSize(ArenaBracketType bracketType) const
{
    switch (bracketType)
    {
        case ArenaBracketType::ARENA_2v2:
        case ArenaBracketType::SKIRMISH_2v2:
            return 2;
        case ArenaBracketType::ARENA_3v3:
        case ArenaBracketType::SKIRMISH_3v3:
            return 3;
        case ArenaBracketType::ARENA_5v5:
            return 5;
        default:
            return 2;
    }
}

BattlegroundTypeId ArenaBotManager::GetBGTypeForBracket(ArenaBracketType bracketType) const
{
    switch (bracketType)
    {
        case ArenaBracketType::ARENA_2v2:
        case ArenaBracketType::SKIRMISH_2v2:
            return BATTLEGROUND_AA; // Arena type
        case ArenaBracketType::ARENA_3v3:
        case ArenaBracketType::SKIRMISH_3v3:
            return BATTLEGROUND_AA;
        case ArenaBracketType::ARENA_5v5:
            return BATTLEGROUND_AA;
        default:
            return BATTLEGROUND_AA;
    }
}

bool ArenaBotManager::IsSkirmish(ArenaBracketType bracketType) const
{
    return bracketType == ArenaBracketType::SKIRMISH_2v2 ||
           bracketType == ArenaBracketType::SKIRMISH_3v3;
}

bool ArenaBotManager::QueueBot(Player* bot, ArenaBracketType bracketType, ArenaQueueMode mode, bool asTeammate)
{
    if (!bot || !IsBotAvailable(bot))
        return false;

    uint8 teamSize = GetTeamSize(bracketType);
    bool isRated = (mode == ArenaQueueMode::RATED);
    BattlegroundQueueIdType queueIdType = isRated ?
        BattlegroundQueueIdType::Arena : BattlegroundQueueIdType::ArenaSkirmish;

    // In 12.0, BGQueueTypeId takes 4 params: (battlemasterListId, type, rated, teamSize)
    BattlegroundQueueTypeId bgQueueTypeId = BattlegroundMgr::BGQueueTypeId(
        BATTLEGROUND_AA,    // Arena uses BATTLEGROUND_AA
        queueIdType,
        isRated,
        teamSize
    );

    // Get the bracket entry for this bot's level (arenas use specific maps)
    // Arena maps are typically 559 (Nagrand Arena) or similar
    PVPDifficultyEntry const* bracketEntry = DB2Manager::GetBattlegroundBracketByLevel(
        559,    // Nagrand Arena map ID (common arena map)
        bot->GetLevel()
    );
    if (!bracketEntry)
    {
        TC_LOG_ERROR("module.playerbot.arena", "ArenaBotManager::QueueBot - No bracket entry for bot {} at level {}",
                     bot->GetName(), bot->GetLevel());
        return false;
    }

    // Queue the bot through BattlegroundMgr
    // AddGroup takes 7 params: (leader, group, team, bracketEntry, isPremade, ArenaRating, MatchmakerRating)
    GroupQueueInfo* ginfo = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId).AddGroup(
        bot,
        nullptr,        // No group
        bot->GetTeam(),
        bracketEntry,
        false,          // Not premade
        0,              // Arena rating (MMR system handles)
        0               // Matchmaker rating
    );

    if (ginfo)
    {
        TC_LOG_DEBUG("module.playerbot.arena", "ArenaBotManager::QueueBot - Bot {} queued for {}v{} arena",
                     bot->GetName(), teamSize, teamSize);
        return true;
    }

    return false;
}

void ArenaBotManager::RemoveBotFromQueue(Player* bot)
{
    if (!bot)
        return;

    for (uint8 i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
    {
        BattlegroundQueueTypeId bgQueueTypeId = bot->GetBattlegroundQueueTypeId(i);
        if (bgQueueTypeId != BATTLEGROUND_QUEUE_NONE)
        {
            sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId).RemovePlayer(bot->GetGUID(), false);
        }
    }

    TC_LOG_DEBUG("module.playerbot.arena", "ArenaBotManager::RemoveBotFromQueue - Bot {} removed from queue",
                 bot->GetName());
}

std::vector<Player*> ArenaBotManager::FindAvailableBots(uint8 minLevel, uint8 maxLevel, uint32 count) const
{
    std::vector<Player*> result;
    result.reserve(count);

    // Use sWorld->GetAllSessions() and filter for bots
    for (auto const& [accountId, session] : sWorld->GetAllSessions())
    {
        if (!session)
            continue;

        Player* bot = session->GetPlayer();
        if (!bot || !bot->IsInWorld())
            continue;

        // Only process bots, not human players
        if (!PlayerBotHooks::IsPlayerBot(bot))
            continue;

        uint8 level = bot->GetLevel();
        if (level < minLevel || level > maxLevel)
            continue;

        if (!IsBotAvailable(bot))
            continue;

        if (IsBotQueued(bot->GetGUID()))
            continue;

        result.push_back(bot);

        if (result.size() >= count)
            break;
    }

    return result;
}

std::vector<Player*> ArenaBotManager::FindBotsForTeamComposition(uint32 teamSize, uint8 minLevel, uint8 maxLevel) const
{
    std::vector<Player*> result;
    result.reserve(teamSize);

    // For arena, we want balanced team composition
    // Try to get at least 1 healer for 3v3+
    bool needHealer = (teamSize >= 3);
    bool foundHealer = false;

    // First pass: find a healer if needed
    if (needHealer)
    {
        for (auto const& [accountId, session] : sWorld->GetAllSessions())
        {
            if (!session)
                continue;

            Player* bot = session->GetPlayer();
            if (!bot || !bot->IsInWorld())
                continue;

            // Only process bots, not human players
            if (!PlayerBotHooks::IsPlayerBot(bot))
                continue;

            uint8 level = bot->GetLevel();
            if (level < minLevel || level > maxLevel)
                continue;

            if (!IsBotAvailable(bot))
                continue;

            if (IsBotQueued(bot->GetGUID()))
                continue;

            if (CanBeHealer(bot))
            {
                result.push_back(bot);
                foundHealer = true;
                break;
            }
        }
    }

    // Second pass: fill remaining slots with DPS
    for (auto const& [accountId, session] : sWorld->GetAllSessions())
    {
        if (result.size() >= teamSize)
            break;

        if (!session)
            continue;

        Player* bot = session->GetPlayer();
        if (!bot || !bot->IsInWorld())
            continue;

        // Only process bots, not human players
        if (!PlayerBotHooks::IsPlayerBot(bot))
            continue;

        // Skip if already added (healer)
        bool alreadyAdded = false;
        for (Player* p : result)
        {
            if (p->GetGUID() == bot->GetGUID())
            {
                alreadyAdded = true;
                break;
            }
        }
        if (alreadyAdded)
            continue;

        uint8 level = bot->GetLevel();
        if (level < minLevel || level > maxLevel)
            continue;

        if (!IsBotAvailable(bot))
            continue;

        if (IsBotQueued(bot->GetGUID()))
            continue;

        if (CanBeDPS(bot))
        {
            result.push_back(bot);
        }
    }

    return result;
}

void ArenaBotManager::RegisterBotAssignment(ObjectGuid humanGuid, ObjectGuid botGuid,
                                             ArenaBracketType bracketType, ArenaQueueMode mode, bool isTeammate)
{
    _queuedBots[botGuid] = BotQueueInfo(humanGuid, bracketType, mode, isTeammate);

    if (!humanGuid.IsEmpty())
    {
        auto& humanInfo = _humanPlayers[humanGuid];
        if (isTeammate)
            humanInfo.teammates.push_back(botGuid);
        else
            humanInfo.opponents.push_back(botGuid);
    }

    TC_LOG_DEBUG("module.playerbot.arena", "ArenaBotManager::RegisterBotAssignment - Bot {} assigned as {}",
                 botGuid.ToString(), isTeammate ? "teammate" : "opponent");
}

void ArenaBotManager::UnregisterBotAssignment(ObjectGuid botGuid)
{
    auto itr = _queuedBots.find(botGuid);
    if (itr == _queuedBots.end())
        return;

    ObjectGuid humanGuid = itr->second.humanPlayerGuid;
    bool wasTeammate = itr->second.isTeammate;
    _queuedBots.erase(itr);

    if (!humanGuid.IsEmpty())
    {
        auto humanItr = _humanPlayers.find(humanGuid);
        if (humanItr != _humanPlayers.end())
        {
            auto& list = wasTeammate ? humanItr->second.teammates : humanItr->second.opponents;
            list.erase(std::remove(list.begin(), list.end(), botGuid), list.end());

            if (humanItr->second.teammates.empty() && humanItr->second.opponents.empty())
                _humanPlayers.erase(humanItr);
        }
    }

    TC_LOG_DEBUG("module.playerbot.arena", "ArenaBotManager::UnregisterBotAssignment - Bot {} unregistered", botGuid.ToString());
}

void ArenaBotManager::UnregisterAllBotsForPlayer(ObjectGuid humanGuid)
{
    auto itr = _humanPlayers.find(humanGuid);
    if (itr == _humanPlayers.end())
        return;

    for (ObjectGuid botGuid : itr->second.teammates)
    {
        if (Player* bot = ObjectAccessor::FindPlayer(botGuid))
        {
            RemoveBotFromQueue(bot);
        }
        _queuedBots.erase(botGuid);
    }

    for (ObjectGuid botGuid : itr->second.opponents)
    {
        if (Player* bot = ObjectAccessor::FindPlayer(botGuid))
        {
            RemoveBotFromQueue(bot);
        }
        _queuedBots.erase(botGuid);
    }

    _humanPlayers.erase(itr);

    TC_LOG_DEBUG("module.playerbot.arena", "ArenaBotManager::UnregisterAllBotsForPlayer - All bots unregistered for {}",
                 humanGuid.ToString());
}

bool ArenaBotManager::IsBotAvailable(Player* bot) const
{
    if (!bot || !bot->IsInWorld())
        return false;

    if (bot->GetGroup())
        return false;

    if (bot->InBattleground())
        return false;

    if (bot->InArena())
        return false;

    for (uint8 i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
    {
        if (bot->GetBattlegroundQueueTypeId(i) != BATTLEGROUND_QUEUE_NONE)
            return false;
    }

    if (bot->isDead())
        return false;

    // Check for deserter (26013)
    if (bot->HasAura(26013))
        return false;

    return true;
}

bool ArenaBotManager::CanBeHealer(Player* bot) const
{
    if (!bot)
        return false;

    // Check class - healers are Priest, Paladin, Shaman, Druid, Monk, Evoker
    switch (bot->GetClass())
    {
        case CLASS_PRIEST:
        case CLASS_PALADIN:
        case CLASS_SHAMAN:
        case CLASS_DRUID:
        case CLASS_MONK:
        case CLASS_EVOKER:
            return true;
        default:
            return false;
    }
}

bool ArenaBotManager::CanBeDPS(Player* bot) const
{
    // All classes can DPS
    return bot != nullptr;
}

} // namespace Playerbot
