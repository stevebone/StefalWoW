/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PlayerbotPacketSniffer.h"
#include "WorldSession.h"
#include "Player.h"
#include "Core/Events/GenericEventBus.h"
#include "Quest/QuestEvents.h"
#include "QuestPackets.h"
#include "QueryPackets.h"  // WoW 12.0: QuestPOIQueryResponse is in Query namespace
#include "Log.h"

namespace Playerbot
{

// ================================================================================================
// TYPED PACKET HANDLERS - QUEST CATEGORY
// Full implementation of all 14 quest packets
// ================================================================================================

void ParseTypedQuestGiverStatus(WorldSession* session, WorldPackets::Quest::QuestGiverStatus const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    QuestEvent event;
    event.type = QuestEventType::QUEST_GIVER_STATUS;
    event.priority = QuestEventPriority::MEDIUM;
    event.playerGuid = bot->GetGUID();
    event.questId = 0;
    event.objectiveId = 0;
    event.objectiveCount = 0;
    event.state = QuestState::NONE;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::seconds(30);

    EventBus<QuestEvent>::instance()->PublishEvent(event);
    TC_LOG_DEBUG("playerbot.packets", "Bot {} received QUEST_GIVER_STATUS (typed): npc={}",
        bot->GetName(), packet.QuestGiver.Guid.ToString());
}

void ParseTypedQuestGiverQuestListMessage(WorldSession* session, WorldPackets::Quest::QuestGiverQuestListMessage const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    QuestEvent event;
    event.type = QuestEventType::QUEST_LIST_RECEIVED;
    event.priority = QuestEventPriority::MEDIUM;
    event.playerGuid = bot->GetGUID();
    event.questId = 0;
    event.objectiveId = 0;
    event.objectiveCount = 0;
    event.state = QuestState::NONE;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::seconds(30);

    EventBus<QuestEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received QUEST_GIVER_QUEST_LIST (typed): npc={}",
        bot->GetName(), packet.QuestGiverGUID.ToString());
}

void ParseTypedQuestGiverQuestDetails(WorldSession* session, WorldPackets::Quest::QuestGiverQuestDetails const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    QuestEvent event;
    event.type = QuestEventType::QUEST_DETAILS_RECEIVED;
    event.priority = QuestEventPriority::MEDIUM;
    event.playerGuid = bot->GetGUID();
    event.questId = packet.QuestID;
    event.objectiveId = 0;
    event.objectiveCount = 0;
    event.state = QuestState::NONE;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::seconds(30);

    EventBus<QuestEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received QUEST_GIVER_QUEST_DETAILS (typed): quest={}",
        bot->GetName(), packet.QuestID);
}

void ParseTypedQuestGiverRequestItems(WorldSession* session, WorldPackets::Quest::QuestGiverRequestItems const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    QuestEvent event;
    event.type = QuestEventType::QUEST_REQUEST_ITEMS;
    event.priority = QuestEventPriority::MEDIUM;
    event.playerGuid = bot->GetGUID();
    event.questId = packet.QuestID;
    event.objectiveId = 0;
    event.objectiveCount = 0;
    event.state = QuestState::NONE;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::seconds(30);

    EventBus<QuestEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received QUEST_GIVER_REQUEST_ITEMS (typed): quest={}",
        bot->GetName(), packet.QuestID);
}

void ParseTypedQuestGiverOfferRewardMessage(WorldSession* session, WorldPackets::Quest::QuestGiverOfferRewardMessage const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    QuestEvent event;
    event.type = QuestEventType::QUEST_OFFER_REWARD;
    event.priority = QuestEventPriority::MEDIUM;
    event.playerGuid = bot->GetGUID();
    event.questId = packet.QuestData.QuestID;  // WoW 12.0: QuestID is in nested QuestData
    event.objectiveId = 0;
    event.objectiveCount = 0;
    event.state = QuestState::NONE;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::seconds(30);

    EventBus<QuestEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received QUEST_GIVER_OFFER_REWARD (typed): quest={}",
        bot->GetName(), packet.QuestData.QuestID);
}

void ParseTypedQuestGiverQuestComplete(WorldSession* session, WorldPackets::Quest::QuestGiverQuestComplete const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    QuestEvent event;
    event.type = QuestEventType::QUEST_COMPLETED;
    event.priority = QuestEventPriority::HIGH;
    event.playerGuid = bot->GetGUID();
    event.questId = packet.QuestID;
    event.objectiveId = 0;
    event.objectiveCount = 0;
    event.state = QuestState::COMPLETE;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::seconds(10);

    EventBus<QuestEvent>::instance()->PublishEvent(event);

    // ORPHANED CODE REMOVED: Duplicate null check fragment
    // if (!bot) { TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: bot in method GetGUID"); return; }

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received QUEST_GIVER_QUEST_COMPLETE (typed): quest={}",
        bot->GetName(), packet.QuestID);
}

void ParseTypedQuestGiverQuestFailed(WorldSession* session, WorldPackets::Quest::QuestGiverQuestFailed const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    QuestEvent event;
    event.type = QuestEventType::QUEST_FAILED;
    event.priority = QuestEventPriority::HIGH;
    event.playerGuid = bot->GetGUID();
    event.questId = packet.QuestID;
    event.objectiveId = 0;
    event.objectiveCount = 0;
    event.state = QuestState::COMPLETE;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::seconds(10);

    EventBus<QuestEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received QUEST_GIVER_QUEST_FAILED (typed): quest={}",
        bot->GetName(), packet.QuestID);
}

void ParseTypedQuestUpdateAddCreditSimple(WorldSession* session, WorldPackets::Quest::QuestUpdateAddCreditSimple const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    QuestEvent event;
    event.type = QuestEventType::QUEST_CREDIT_ADDED;
    event.priority = QuestEventPriority::MEDIUM;
    event.playerGuid = bot->GetGUID();
    event.questId = packet.QuestID;
    event.objectiveId = packet.ObjectID;
    event.objectiveCount = 1;
    event.state = QuestState::INCOMPLETE;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::seconds(30);

    EventBus<QuestEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received QUEST_UPDATE_ADD_CREDIT_SIMPLE (typed): quest={}, credit={}",
        bot->GetName(), packet.QuestID, packet.ObjectID);
}

void ParseTypedQuestUpdateAddCredit(WorldSession* session, WorldPackets::Quest::QuestUpdateAddCredit const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    QuestEvent event;
    event.type = QuestEventType::QUEST_CREDIT_ADDED;
    event.priority = QuestEventPriority::MEDIUM;
    event.playerGuid = bot->GetGUID();
    event.questId = packet.QuestID;
    event.objectiveId = packet.ObjectID;
    event.objectiveCount = packet.Count;
    event.state = QuestState::INCOMPLETE;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::seconds(30);

    EventBus<QuestEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received QUEST_UPDATE_ADD_CREDIT (typed): quest={}, credit={}, count={}",
        bot->GetName(), packet.QuestID, packet.ObjectID, packet.Count);
}

void ParseTypedQuestUpdateComplete(WorldSession* session, WorldPackets::Quest::QuestUpdateComplete const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    QuestEvent event;
    event.type = QuestEventType::QUEST_OBJECTIVE_COMPLETE;
    event.priority = QuestEventPriority::HIGH;
    event.playerGuid = bot->GetGUID();
    event.questId = packet.QuestID;
    event.objectiveId = 0;
    event.objectiveCount = 0;
    event.state = QuestState::COMPLETE;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::seconds(10);

    EventBus<QuestEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received QUEST_UPDATE_COMPLETE (typed): quest={}",
        bot->GetName(), packet.QuestID);
}

// QuestUpdateFailed packet doesn't exist in WoW 12.0 - removed

void ParseTypedQuestUpdateFailedTimer(WorldSession* session, WorldPackets::Quest::QuestUpdateFailedTimer const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    QuestEvent event;
    event.type = QuestEventType::QUEST_UPDATE_FAILED;
    event.priority = QuestEventPriority::HIGH;
    event.playerGuid = bot->GetGUID();
    event.questId = packet.QuestID;
    event.objectiveId = 0;
    event.objectiveCount = 0;
    event.state = QuestState::COMPLETE;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::seconds(10);

    EventBus<QuestEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received QUEST_UPDATE_FAILED_TIMER (typed): quest={}",
        bot->GetName(), packet.QuestID);
}

void ParseTypedQuestConfirmAccept(WorldSession* session, WorldPackets::Quest::QuestConfirmAccept const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    QuestEvent event;
    event.type = QuestEventType::QUEST_CONFIRM_ACCEPT;
    event.priority = QuestEventPriority::MEDIUM;
    event.playerGuid = bot->GetGUID();
    event.questId = packet.QuestID;
    event.objectiveId = 0;
    event.objectiveCount = 0;
    event.state = QuestState::NONE;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::seconds(30);

    EventBus<QuestEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received QUEST_CONFIRM_ACCEPT (typed): quest={}",
        bot->GetName(), packet.QuestID);
}

void ParseTypedQuestPOIQueryResponse(WorldSession* session, WorldPackets::Query::QuestPOIQueryResponse const& packet)  // WoW 12.0: In Query namespace
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    QuestEvent event;
    event.type = QuestEventType::QUEST_POI_RECEIVED;
    event.priority = QuestEventPriority::MEDIUM;
    event.playerGuid = bot->GetGUID();
    event.questId = 0;
    event.objectiveId = 0;
    event.objectiveCount = 0;
    event.state = QuestState::NONE;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::seconds(30);

    EventBus<QuestEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received QUEST_POI_QUERY_RESPONSE (typed): {} POIs",
        bot->GetName(), packet.QuestPOIDataStats.size());  // WoW 12.0: Field is QuestPOIDataStats, not QuestPOIData
}

// ================================================================================================
// HANDLER REGISTRATION
// ================================================================================================

void RegisterQuestPacketHandlers()
{
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Quest::QuestGiverStatus>(&ParseTypedQuestGiverStatus);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Quest::QuestGiverQuestListMessage>(&ParseTypedQuestGiverQuestListMessage);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Quest::QuestGiverQuestDetails>(&ParseTypedQuestGiverQuestDetails);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Quest::QuestGiverRequestItems>(&ParseTypedQuestGiverRequestItems);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Quest::QuestGiverOfferRewardMessage>(&ParseTypedQuestGiverOfferRewardMessage);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Quest::QuestGiverQuestComplete>(&ParseTypedQuestGiverQuestComplete);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Quest::QuestGiverQuestFailed>(&ParseTypedQuestGiverQuestFailed);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Quest::QuestUpdateAddCreditSimple>(&ParseTypedQuestUpdateAddCreditSimple);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Quest::QuestUpdateAddCredit>(&ParseTypedQuestUpdateAddCredit);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Quest::QuestUpdateComplete>(&ParseTypedQuestUpdateComplete);
    // QuestUpdateFailed doesn't exist in WoW 12.0 - removed
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Quest::QuestUpdateFailedTimer>(&ParseTypedQuestUpdateFailedTimer);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Quest::QuestConfirmAccept>(&ParseTypedQuestConfirmAccept);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Query::QuestPOIQueryResponse>(&ParseTypedQuestPOIQueryResponse);  // WoW 12.0: Moved to Query namespace

    TC_LOG_INFO("playerbot", "PlayerbotPacketSniffer: Registered {} Quest packet typed handlers", 13);
}

} // namespace Playerbot
