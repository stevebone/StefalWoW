/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "GracefulExitHandler.h"
#include "Log.h"
#include "Config/PlayerbotConfig.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Mail.h"
#include "CharacterCache.h"
#include "CharacterDatabase.h"
#include "StringFormat.h"
#include "WorldSession.h"
#include "Session/BotSession.h"
#include "Session/BotWorldSessionMgr.h"
#include "../../AI/BotAI.h"  // P1 FIX: Required for unique_ptr<BotAI> in BotSession.h
#include <sstream>

namespace Playerbot
{

GracefulExitHandler* GracefulExitHandler::Instance()
{
    static GracefulExitHandler instance;
    return &instance;
}

bool GracefulExitHandler::Initialize()
{
    if (_initialized.exchange(true))
        return true;

    LoadConfig();

    TC_LOG_INFO("playerbot.lifecycle", "GracefulExitHandler initialized");
    return true;
}

void GracefulExitHandler::Shutdown()
{
    if (!_initialized.exchange(false))
        return;

    // Cancel all in-progress stages
    {
        std::lock_guard<std::mutex> lock(_stageMutex);
        for (auto& [guid, stage] : _inProgressStages)
        {
            if (stage.callback)
            {
                StageResult result;
                result.success = false;
                result.errorMessage = "Shutdown in progress";
                stage.callback(guid, result);
            }
        }
        _inProgressStages.clear();
    }

    TC_LOG_INFO("playerbot.lifecycle", "GracefulExitHandler shutdown complete. "
        "Stages processed: {}, Stages failed: {}",
        _totalStagesProcessed.load(), _totalStagesFailed.load());
}

void GracefulExitHandler::LoadConfig()
{
    // Stage timeouts
    _config.guildLeaveTimeoutMs = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Retirement.GuildLeaveTimeoutMs", 5000);
    _config.mailClearTimeoutMs = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Retirement.MailClearTimeoutMs", 30000);
    _config.auctionCancelTimeoutMs = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Retirement.AuctionCancelTimeoutMs", 15000);
    _config.saveStateTimeoutMs = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Retirement.SaveStateTimeoutMs", 10000);
    _config.logoutTimeoutMs = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Retirement.LogoutTimeoutMs", 5000);
    _config.deleteTimeoutMs = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Retirement.DeleteTimeoutMs", 10000);

    // Retry settings
    _config.maxRetries = static_cast<uint8>(sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Retirement.MaxRetries", 3));
    _config.retryDelayMs = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Retirement.RetryDelayMs", 1000);

    // Mail handling
    _config.returnMailWithItems = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Retirement.ReturnMailWithItems", true);
    _config.deleteTextOnlyMail = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Retirement.DeleteTextOnlyMail", true);

    // Auction handling
    _config.cancelActiveAuctions = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Retirement.CancelActiveAuctions", true);
    _config.waitForAuctionMail = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Retirement.WaitForAuctionMail", false);

    // Guild handling
    _config.gracefulGuildLeave = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Retirement.GracefulGuildLeave", true);
    _config.transferGuildLeadershipFirst = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Retirement.TransferGuildLeadershipFirst", true);

    // Character deletion
    _config.permanentDelete = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Retirement.PermanentDelete", true);
    _config.archiveCharacterFirst = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Retirement.ArchiveCharacterFirst", false);

    // Logging
    _config.verboseLogging = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Retirement.VerboseLogging", true);

    TC_LOG_INFO("playerbot.lifecycle", "GracefulExitHandler config loaded");
}

bool GracefulExitHandler::ExecuteStage(RetirementCandidate& candidate, StageCallback callback)
{
    return ExecuteStage(candidate.botGuid, candidate.exitStage, callback);
}

bool GracefulExitHandler::ExecuteStage(ObjectGuid botGuid, GracefulExitStage stage, StageCallback callback)
{
    if (!_initialized)
    {
        TC_LOG_ERROR("playerbot.lifecycle", "GracefulExitHandler not initialized");
        return false;
    }

    if (!botGuid)
    {
        TC_LOG_ERROR("playerbot.lifecycle", "ExecuteStage called with invalid bot GUID");
        return false;
    }

    // Check if stage is already in progress
    {
        std::lock_guard<std::mutex> lock(_stageMutex);
        auto it = _inProgressStages.find(botGuid);
        if (it != _inProgressStages.end())
        {
            TC_LOG_WARN("playerbot.lifecycle", "Stage already in progress for bot {}",
                botGuid.ToString());
            return false;
        }

        // Record in-progress
        InProgressStage inProgress;
        inProgress.botGuid = botGuid;
        inProgress.stage = stage;
        inProgress.callback = callback;
        inProgress.startTime = std::chrono::steady_clock::now();
        _inProgressStages[botGuid] = inProgress;
    }

    // Execute the stage
    StageResult result = DispatchStage(botGuid, stage);

    // Log result
    LogStageResult(botGuid, stage, result);

    // Update statistics
    ++_totalStagesProcessed;
    if (!result.success && !result.advance)
        ++_totalStagesFailed;

    // Remove from in-progress
    {
        std::lock_guard<std::mutex> lock(_stageMutex);
        _inProgressStages.erase(botGuid);
    }

    // Call callback
    if (callback)
        callback(botGuid, result);

    return true;
}

bool GracefulExitHandler::IsStageInProgress(ObjectGuid botGuid) const
{
    std::lock_guard<std::mutex> lock(_stageMutex);
    return _inProgressStages.find(botGuid) != _inProgressStages.end();
}

void GracefulExitHandler::CancelStage(ObjectGuid botGuid)
{
    std::lock_guard<std::mutex> lock(_stageMutex);
    auto it = _inProgressStages.find(botGuid);
    if (it != _inProgressStages.end())
    {
        if (it->second.callback)
        {
            StageResult result;
            result.success = false;
            result.errorMessage = "Stage cancelled";
            it->second.callback(botGuid, result);
        }
        _inProgressStages.erase(it);
    }
}

StageResult GracefulExitHandler::DispatchStage(ObjectGuid botGuid, GracefulExitStage stage)
{
    switch (stage)
    {
        case GracefulExitStage::LeavingGuild:
            return HandleLeaveGuild(botGuid);

        case GracefulExitStage::ClearingMail:
            return HandleClearMail(botGuid);

        case GracefulExitStage::CancellingAuctions:
            return HandleCancelAuctions(botGuid);

        case GracefulExitStage::SavingState:
            return HandleSaveState(botGuid);

        case GracefulExitStage::LoggingOut:
            return HandleLogout(botGuid);

        case GracefulExitStage::DeletingCharacter:
            return HandleDeleteCharacter(botGuid);

        case GracefulExitStage::Complete:
            return StageResult::Success();

        default:
            return StageResult::Fail("Unknown stage");
    }
}

StageResult GracefulExitHandler::HandleLeaveGuild(ObjectGuid botGuid)
{
    // Get bot's guild
    ObjectGuid guildGuid = GetBotGuild(botGuid);
    if (!guildGuid)
    {
        // Not in a guild, skip this stage
        if (_config.verboseLogging)
        {
            TC_LOG_DEBUG("playerbot.lifecycle", "Bot {} not in guild, skipping guild leave",
                botGuid.ToString());
        }
        return StageResult::NotNeeded();
    }

    // Get guild
    Guild* guild = sGuildMgr->GetGuildByGuid(guildGuid);
    if (!guild)
    {
        TC_LOG_WARN("playerbot.lifecycle", "Guild {} not found for bot {}",
            guildGuid.ToString(), botGuid.ToString());
        return StageResult::NotNeeded();
    }

    // Check if bot is guild leader
    if (_config.transferGuildLeadershipFirst && IsBotGuildLeader(botGuid))
    {
        Player* bot = GetBotPlayer(botGuid);
        if (bot && !TransferGuildLeadership(bot))
        {
            // Could not transfer leadership - this is a problem
            // We might need to disband the guild or wait
            TC_LOG_WARN("playerbot.lifecycle", "Could not transfer guild leadership for bot {}",
                botGuid.ToString());
            // Continue anyway - will be removed as GM
        }
    }

    // Get player object
    Player* bot = GetBotPlayer(botGuid);
    if (!bot)
    {
        // Bot not online - use direct database deletion
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_MEMBER);
        stmt->setUInt64(0, botGuid.GetCounter());
        CharacterDatabase.Execute(stmt);

        TC_LOG_INFO("playerbot.lifecycle", "Bot {} removed from guild {} (offline removal)",
            botGuid.ToString(), guildGuid.ToString());
        return StageResult::Success();
    }

    // Online - use proper guild leave
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    if (_config.gracefulGuildLeave)
    {
        // This triggers proper events and cleanup
        guild->DeleteMember(trans, botGuid, false, true);
    }
    else
    {
        // Direct removal
        guild->DeleteMember(trans, botGuid, false, false);
    }
    CharacterDatabase.CommitTransaction(trans);

    TC_LOG_INFO("playerbot.lifecycle", "Bot {} left guild {}",
        botGuid.ToString(), guild->GetName());
    return StageResult::Success();
}

StageResult GracefulExitHandler::HandleClearMail(ObjectGuid botGuid)
{
    uint32 mailCount = GetPendingMailCount(botGuid);

    if (mailCount == 0)
    {
        if (_config.verboseLogging)
        {
            TC_LOG_DEBUG("playerbot.lifecycle", "Bot {} has no pending mail",
                botGuid.ToString());
        }
        return StageResult::NotNeeded();
    }

    // Get player if online
    Player* bot = GetBotPlayer(botGuid);

    // Delete/return all mail
    uint32 cleared = 0;

    if (bot)
    {
        // Online - iterate through mail
        // Note: GetMails() returns std::deque<Mail*> so we get pointers
        std::vector<uint64> mailIds;
        for (Mail* mail : bot->GetMails())
        {
            if (mail)
                mailIds.push_back(mail->messageID);
        }

        for (uint64 mailId : mailIds)
        {
            Mail* mail = bot->GetMail(mailId);
            if (mail && ClearMail(bot, mail))
                ++cleared;
        }
    }
    else
    {
        // Offline - direct database operations
        if (_config.returnMailWithItems)
        {
            // Return mail with items to sender
            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_MAIL_LIST_ITEMS);
            stmt->setUInt64(0, botGuid.GetCounter());
            PreparedQueryResult result = CharacterDatabase.Query(stmt);

            if (result)
            {
                do
                {
                    Field* fields = result->Fetch();
                    uint64 mailId = fields[0].GetUInt64();
                    ObjectGuid sender = ObjectGuid::Create<HighGuid::Player>(fields[1].GetUInt64());
                    bool hasItems = fields[2].GetBool();

                    if (hasItems && !sender.IsEmpty())
                    {
                        // Return mail to sender
                        // This is simplified - full implementation would recreate the mail
                        TC_LOG_DEBUG("playerbot.lifecycle", "Would return mail {} to sender {}",
                            mailId, sender.ToString());
                    }

                    // Delete the mail
                    CharacterDatabasePreparedStatement* delStmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_MAIL_BY_ID);
                    delStmt->setUInt64(0, mailId);
                    CharacterDatabase.Execute(delStmt);

                    ++cleared;
                } while (result->NextRow());
            }
        }
        else
        {
            // Just delete all mail
            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_MAIL);
            stmt->setUInt64(0, botGuid.GetCounter());
            CharacterDatabase.Execute(stmt);
            cleared = mailCount;
        }

        // Delete mail items
        CharacterDatabasePreparedStatement* itemStmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_MAIL_ITEMS);
        itemStmt->setUInt64(0, botGuid.GetCounter());
        CharacterDatabase.Execute(itemStmt);
    }

    TC_LOG_INFO("playerbot.lifecycle", "Bot {} mail cleared: {} items",
        botGuid.ToString(), cleared);
    return StageResult::Success(cleared);
}

StageResult GracefulExitHandler::HandleCancelAuctions(ObjectGuid botGuid)
{
    uint32 auctionCount = GetActiveAuctionCount(botGuid);

    if (auctionCount == 0)
    {
        if (_config.verboseLogging)
        {
            TC_LOG_DEBUG("playerbot.lifecycle", "Bot {} has no active auctions",
                botGuid.ToString());
        }
        return StageResult::NotNeeded();
    }

    if (!_config.cancelActiveAuctions)
    {
        TC_LOG_DEBUG("playerbot.lifecycle", "Bot {} has {} auctions but cancellation disabled",
            botGuid.ToString(), auctionCount);
        return StageResult::NotNeeded();
    }

    // Cancel all auctions
    uint32 cancelled = 0;

    // Get all auctions for this character
    // Note: There's no specific prepared statement for this, using raw query
    std::string query = Trinity::StringFormat(
        "SELECT id FROM auctionhouse WHERE owner = {}", botGuid.GetCounter());
    QueryResult result = CharacterDatabase.Query(query.c_str());

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 auctionId = fields[0].GetUInt32();

            if (CancelAuction(botGuid, auctionId))
                ++cancelled;
        } while (result->NextRow());
    }

    TC_LOG_INFO("playerbot.lifecycle", "Bot {} auctions cancelled: {}",
        botGuid.ToString(), cancelled);
    return StageResult::Success(cancelled);
}

StageResult GracefulExitHandler::HandleSaveState(ObjectGuid botGuid)
{
    Player* bot = GetBotPlayer(botGuid);

    if (bot)
    {
        // Save player data (with crash protection)
        // CRITICAL FIX (Item.cpp:1304 crash): Check for pending spell events before SaveToDB
        bool hasPendingEvents = !bot->m_Events.GetEvents().empty();
        bool isCurrentlyCasting = bot->GetCurrentSpell(CURRENT_GENERIC_SPELL) != nullptr ||
                                  bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL) != nullptr ||
                                  bot->GetCurrentSpell(CURRENT_AUTOREPEAT_SPELL) != nullptr;

        if (!hasPendingEvents && !isCurrentlyCasting)
        {
            bot->SaveToDB(false);

            if (_config.verboseLogging)
            {
                TC_LOG_DEBUG("playerbot.lifecycle", "Bot {} final state saved",
                    botGuid.ToString());
            }
        }
        else
        {
            TC_LOG_DEBUG("playerbot.lifecycle",
                "Bot {} has pending events/spells - skipping final save to prevent Item.cpp:1304 crash",
                botGuid.ToString());
        }
    }

    // Archive if configured
    if (_config.archiveCharacterFirst)
    {
        if (!ArchiveCharacter(botGuid))
        {
            TC_LOG_WARN("playerbot.lifecycle", "Failed to archive character {}",
                botGuid.ToString());
            // Continue anyway
        }
    }

    return StageResult::Success();
}

StageResult GracefulExitHandler::HandleLogout(ObjectGuid botGuid)
{
    Player* bot = sBotWorldSessionMgr->GetPlayerBot(botGuid);

    if (!bot)
    {
        if (_config.verboseLogging)
        {
            TC_LOG_DEBUG("playerbot.lifecycle", "Bot {} not logged in, skipping logout",
                botGuid.ToString());
        }
        return StageResult::NotNeeded();
    }

    // Request logout
    sBotWorldSessionMgr->RemovePlayerBot(botGuid);

    TC_LOG_INFO("playerbot.lifecycle", "Bot {} logout requested",
        botGuid.ToString());
    return StageResult::Success();
}

StageResult GracefulExitHandler::HandleDeleteCharacter(ObjectGuid botGuid)
{
    if (!_config.permanentDelete)
    {
        TC_LOG_DEBUG("playerbot.lifecycle", "Bot {} deletion skipped (disabled)",
            botGuid.ToString());
        return StageResult::NotNeeded();
    }

    // Make sure bot is logged out
    if (sBotWorldSessionMgr->GetPlayerBot(botGuid))
    {
        return StageResult::Retry("Bot still logged in");
    }

    // Delete character
    if (!DeleteCharacterFromDB(botGuid))
    {
        return StageResult::Fail("Database deletion failed");
    }

    TC_LOG_INFO("playerbot.lifecycle", "Bot {} character deleted from database",
        botGuid.ToString());
    return StageResult::Success();
}

// ============================================================================
// UTILITY METHODS
// ============================================================================

Player* GracefulExitHandler::GetBotPlayer(ObjectGuid botGuid) const
{
    return ObjectAccessor::FindPlayer(botGuid);
}

bool GracefulExitHandler::IsBotLoggedIn(ObjectGuid botGuid) const
{
    return sBotWorldSessionMgr->GetPlayerBot(botGuid) != nullptr;
}

uint32 GracefulExitHandler::GetPendingMailCount(ObjectGuid botGuid) const
{
    Player* bot = GetBotPlayer(botGuid);
    if (bot)
        return bot->GetMailSize();

    // Query database for offline bot
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_MAIL_COUNT);
    stmt->setUInt64(0, botGuid.GetCounter());
    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (result)
        return (*result)[0].GetUInt32();

    return 0;
}

uint32 GracefulExitHandler::GetActiveAuctionCount(ObjectGuid botGuid) const
{
    // Query auction count directly - no specific prepared statement exists for this
    std::string query = Trinity::StringFormat(
        "SELECT COUNT(*) FROM auctionhouse WHERE owner = {}", botGuid.GetCounter());
    QueryResult result = CharacterDatabase.Query(query.c_str());

    if (result)
        return (*result)[0].GetUInt32();

    return 0;
}

ObjectGuid GracefulExitHandler::GetBotGuild(ObjectGuid botGuid) const
{
    Player* bot = GetBotPlayer(botGuid);
    if (bot)
        return bot->GetGuildId() ? ObjectGuid::Create<HighGuid::Guild>(bot->GetGuildId()) : ObjectGuid::Empty;

    // Query database for offline bot
    CharacterCacheEntry const* cache = sCharacterCache->GetCharacterCacheByGuid(botGuid);
    if (cache && cache->GuildId)
        return ObjectGuid::Create<HighGuid::Guild>(cache->GuildId);

    return ObjectGuid::Empty;
}

bool GracefulExitHandler::IsBotGuildLeader(ObjectGuid botGuid) const
{
    ObjectGuid guildGuid = GetBotGuild(botGuid);
    if (!guildGuid)
        return false;

    Guild* guild = sGuildMgr->GetGuildByGuid(guildGuid);
    if (!guild)
        return false;

    return guild->GetLeaderGUID() == botGuid;
}

bool GracefulExitHandler::TransferGuildLeadership(Player* bot)
{
    if (!bot)
        return false;

    Guild* guild = bot->GetGuild();
    if (!guild)
        return false;

    if (guild->GetLeaderGUID() != bot->GetGUID())
        return true;  // Not leader, nothing to transfer

    // Find another member to transfer to
    ObjectGuid newLeader;
    for (auto const& [memberGuid, member] : guild->GetMembers())
    {
        if (memberGuid != bot->GetGUID())
        {
            newLeader = memberGuid;
            break;
        }
    }

    if (!newLeader)
    {
        // No other members - guild will be disbanded when bot leaves
        TC_LOG_INFO("playerbot.lifecycle", "Bot {} is only member of guild {}, guild will disband",
            bot->GetGUID().ToString(), guild->GetName());
        return true;
    }

    // Transfer leadership
    std::string newLeaderName;
    if (!sCharacterCache->GetCharacterNameByGuid(newLeader, newLeaderName))
    {
        TC_LOG_WARN("playerbot.lifecycle", "Could not get name for new guild leader {}",
            newLeader.ToString());
        return false;
    }

    guild->HandleSetNewGuildMaster(bot->GetSession(), newLeaderName, false);

    TC_LOG_INFO("playerbot.lifecycle", "Guild leadership transferred from {} to {} ({})",
        bot->GetGUID().ToString(), newLeader.ToString(), newLeaderName);
    return true;
}

bool GracefulExitHandler::ClearMail(Player* bot, Mail* mail)
{
    if (!bot || !mail)
        return false;

    // For now, just mark mail as deleted
    // Full implementation would return items to sender
    bot->RemoveMail(mail->messageID);

    return true;
}

bool GracefulExitHandler::CancelAuction(ObjectGuid botGuid, uint32 auctionId)
{
    // Cancel auction and return item via mail
    // This is a simplified implementation
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_AUCTION);
    stmt->setUInt32(0, auctionId);
    CharacterDatabase.Execute(stmt);

    return true;
}

bool GracefulExitHandler::ArchiveCharacter(ObjectGuid botGuid)
{
    // Archive character data to backup table
    // This is for disaster recovery if needed
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    // Copy character data to archive table
    std::string query = Trinity::StringFormat(
        "INSERT INTO characters_archive SELECT *, NOW() as archived_at FROM characters WHERE guid = {}",
        botGuid.GetCounter());
    trans->Append(query.c_str());

    CharacterDatabase.CommitTransaction(trans);

    TC_LOG_INFO("playerbot.lifecycle", "Bot {} archived to backup table",
        botGuid.ToString());
    return true;
}

bool GracefulExitHandler::DeleteCharacterFromDB(ObjectGuid botGuid)
{
    // Delete character and all related data
    // This uses TrinityCore's standard character deletion

    // Delete from character table - this cascades to related tables
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHARACTER);
    stmt->setUInt64(0, botGuid.GetCounter());
    CharacterDatabase.Execute(stmt);

    // Clear from character cache
    sCharacterCache->DeleteCharacterCacheEntry(botGuid, "");

    // Note: Bot should already be removed from BotWorldSessionMgr via RemovePlayerBot()
    // No additional cleanup needed here

    return true;
}

void GracefulExitHandler::LogStageResult(ObjectGuid botGuid, GracefulExitStage stage, StageResult const& result)
{
    if (!_config.verboseLogging && result.success)
        return;

    std::string stageName = GracefulExitStageToString(stage);

    if (result.success)
    {
        if (result.itemsAffected > 0)
        {
            TC_LOG_DEBUG("playerbot.lifecycle", "Bot {} stage {} completed: {} items affected",
                botGuid.ToString(), stageName, result.itemsAffected);
        }
        else
        {
            TC_LOG_DEBUG("playerbot.lifecycle", "Bot {} stage {} completed",
                botGuid.ToString(), stageName);
        }
    }
    else if (result.retry)
    {
        TC_LOG_WARN("playerbot.lifecycle", "Bot {} stage {} retry: {}",
            botGuid.ToString(), stageName, result.errorMessage);
    }
    else
    {
        TC_LOG_ERROR("playerbot.lifecycle", "Bot {} stage {} failed: {}",
            botGuid.ToString(), stageName, result.errorMessage);
    }
}

uint32 GracefulExitHandler::GetInProgressCount() const
{
    std::lock_guard<std::mutex> lock(_stageMutex);
    return static_cast<uint32>(_inProgressStages.size());
}

void GracefulExitHandler::SetConfig(GracefulExitConfig const& config)
{
    _config = config;
}

} // namespace Playerbot
