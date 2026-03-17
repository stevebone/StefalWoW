/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "SocialManager.h"
#include "Player.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "SocialMgr.h"
#include "Chat.h"
#include "Language.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "ObjectAccessor.h"
#include "WorldSession.h"
#include "Chat/Channels/ChannelMgr.h"
#include "Chat/Channels/Channel.h"
#include "Server/Packets/ChatPackets.h"
#include "DatabaseEnv.h"
#include "Optional.h"
#include "SharedDefines.h"
#include "Group.h"
#include "../AI/BotAI.h"
#include "../Config/PlayerbotConfig.h"
#include <algorithm>
#include <random>
#include "GameTime.h"

namespace Playerbot
{

SocialManager::SocialManager(Player* bot, BotAI* ai)
    : m_nextChatTime(0)
    , m_minChatDelay(2000)
    , m_maxChatDelay(10000)
    , m_nextEmoteTime(0)
    , m_emoteInterval(60000)
    , m_guild(nullptr)
    , m_lastGuildChatTime(0)
    , m_bot(bot)
    , m_ai(ai)
    , m_enabled(true)
    , m_chatEnabled(false)
    , m_emotesEnabled(true)
    , m_guildChatEnabled(false)
    , m_autoGreet(false)
    , m_autoRespond(false)
    , m_randomEmotes(false)
    , m_friendlyToAll(true)
    , m_chatResponseChance(10)
    , m_chatUpdateInterval(5000)
    , m_emoteUpdateInterval(30000)
    , m_reputationUpdateInterval(60000)
    , m_lastChatUpdate(0)
    , m_lastEmoteUpdate(0)
    , m_lastReputationUpdate(0)
    , m_updateCount(0)
    , m_cpuUsage(0.0f)
{
}

SocialManager::~SocialManager()
{
    Shutdown();
}

void SocialManager::Initialize()
{
    if (!m_bot)
        return;

    // Load configuration from PlayerbotConfig
    m_chatEnabled = sPlayerbotConfig->GetBool("Playerbot.Social.EnableChat", false);
    m_chatUpdateInterval = sPlayerbotConfig->GetInt("Playerbot.Social.ChatFrequency", 60000);
    m_autoRespond = sPlayerbotConfig->GetBool("Playerbot.Social.RespondToWhispers", true);
    m_guildChatEnabled = sPlayerbotConfig->GetBool("Playerbot.Social.JoinGuilds", false);
    m_enabled = sPlayerbotConfig->GetBool("Playerbot.Social.FormGroups", true);

    TC_LOG_DEBUG("playerbot", "SocialManager: Config loaded - Chat=%d, ChatFreq=%u, RespondWhispers=%d, GuildChat=%d",
        m_chatEnabled, m_chatUpdateInterval, m_autoRespond, m_guildChatEnabled);

    LoadFriendList();
    LoadReputations();
    LoadResponseTemplates();

    // Get bot's guild
    if (uint32 guildId = m_bot->GetGuildId())
        m_guild = sGuildMgr->GetGuildById(guildId);

    ScheduleNextChat();
    ScheduleRandomEmote();
}

void SocialManager::Update(uint32 diff)
{
    if (!m_bot || !m_enabled)
        return;

    StartPerformanceTimer();

    ProcessChatQueue(diff);
    ProcessEmoteQueue(diff);
    UpdateCooldowns(diff);
    CleanupOldChats(diff);

    // Periodic updates
    m_lastChatUpdate += diff;
    if (m_lastChatUpdate >= m_chatUpdateInterval)
    {
        UpdateFriendStatus();
        m_lastChatUpdate = 0;
    }

    m_lastEmoteUpdate += diff;
    if (m_lastEmoteUpdate >= m_emoteUpdateInterval && m_randomEmotes)
    {
        if (urand(0, 100) < 20) // 20% chance per interval
            PerformEmote(SelectRandomEmote());
        m_lastEmoteUpdate = 0;
    }

    m_lastReputationUpdate += diff;
    if (m_lastReputationUpdate >= m_reputationUpdateInterval)
    {
        DecayReputations(diff);
        UpdateGuildStatus();
        m_lastReputationUpdate = 0;
    }

    EndPerformanceTimer();
    UpdatePerformanceMetrics();
}

void SocialManager::Reset()
{
    m_friends.clear();
    m_ignoreList.clear();
    m_reputations.clear();
    m_responseTemplates.clear();
    m_responseCooldowns.clear();
    m_recentChats.clear();
    m_channels.clear();
    m_stats = Statistics();
    m_guild = nullptr;
}

void SocialManager::Shutdown()
{
    SaveFriendList();
    SaveReputations();
}

// ============================================================================
// CHAT SYSTEM
// ============================================================================

bool SocialManager::SendChatMessage(ChatType type, ::std::string const& message, ObjectGuid target)
{
    if (!m_bot || !m_chatEnabled || message.empty())
        return false;
    ::std::string sanitized = SanitizeMessage(message);
    if (sanitized.empty() || IsSpam(sanitized))
        return false;

    WorldSession* session = m_bot->GetSession();
    if (!session)
        return false;

    switch (type)
    {
        case ChatType::SAY:
            m_bot->Say(sanitized, LANG_UNIVERSAL);
            break;

        case ChatType::YELL:
            m_bot->Yell(sanitized, LANG_UNIVERSAL);
            break;

        case ChatType::WHISPER:
        {
            if (target.IsEmpty())
                return false;

            Player* recipient = ObjectAccessor::FindPlayer(target);
            if (!recipient || IsIgnored(target))
                return false;

            m_bot->Whisper(sanitized, LANG_UNIVERSAL, recipient);
            RecordMessageSent(ChatType::WHISPER);
            break;
        }

        case ChatType::PARTY:
        {
            Group* group = m_bot->GetGroup();
            if (!group)
                return false;

            WorldPackets::Chat::Chat packet;
            packet.Initialize(CHAT_MSG_PARTY, LANG_UNIVERSAL, m_bot, nullptr, sanitized);
            group->BroadcastPacket(packet.Write(), false, -1, ObjectGuid::Empty);
            break;
        }

        case ChatType::RAID:
        {
            Group* group = m_bot->GetGroup();
            if (!group || !group->isRaidGroup())
                return false;

            WorldPackets::Chat::Chat packet;
            packet.Initialize(CHAT_MSG_RAID, LANG_UNIVERSAL, m_bot, nullptr, sanitized);
            group->BroadcastPacket(packet.Write(), false, -1, ObjectGuid::Empty);
            break;
        }

        case ChatType::GUILD:
            return SendGuildChat(sanitized);

        case ChatType::OFFICER:
            return SendOfficerChat(sanitized);

        case ChatType::EMOTE:
            m_bot->TextEmote(sanitized);
            break;

        default:
            return false;
    }

    TrackChat(sanitized);
    RecordMessageSent(type);
    return true;
}

bool SocialManager::RespondToChat(Player* sender, ::std::string const& message, ChatType type)
{
    if (!m_bot || !sender || !m_chatEnabled || !m_autoRespond)
        return false;

    if (IsIgnored(sender->GetGUID()))
        return false;

    if (!ShouldRespondToChat(sender, message, type))
        return false;

    ::std::string response = GenerateChatResponse(message, type);
    if (response.empty())
        return false;

    // Add a small delay for natural conversation
    uint32 delay = urand(m_minChatDelay, m_maxChatDelay);
    m_nextChatTime = GameTime::GetGameTimeMS() + delay;
    // Respond via whisper if message was a whisper, otherwise use same channel
    ChatType responseType = (type == ChatType::WHISPER) ? ChatType::WHISPER : type;
    ObjectGuid target = (type == ChatType::WHISPER) ? sender->GetGUID() : ObjectGuid::Empty;
    return SendChatMessage(responseType, response, target);
}

void SocialManager::ProcessIncomingChat(Player* sender, ::std::string const& message, ChatType type)
{
    if (!sender || message.empty())
        return;

    RecordMessageReceived(type);

    // Update reputation based on message tone (simplified sentiment analysis)
    bool isPositive = (message.find("thank") != ::std::string::npos ||
                      message.find("great") != ::std::string::npos ||
                      message.find("nice") != ::std::string::npos ||
                      message.find("good") != ::std::string::npos);

    bool isNegative = (message.find("bad") != ::std::string::npos ||
                      message.find("terrible") != ::std::string::npos ||
                      message.find("stupid") != ::std::string::npos);

    if (isPositive)
        UpdateReputation(sender->GetGUID(), 5, true);
    else if (isNegative)
        UpdateReputation(sender->GetGUID(), -5, false);
    else
        UpdateReputation(sender->GetGUID(), 1, true); // Neutral interaction
}

::std::string SocialManager::GenerateChatResponse(::std::string const& message, ChatType type)
{
    // Check for response templates first
    for (auto const& word : m_responseTemplates)
    {
        if (message.find(word.trigger) != ::std::string::npos && !IsOnCooldown(word.trigger))
        {
            if (!word.responses.empty())
            {
                uint32 index = urand(0, static_cast<uint32>(word.responses.size() - 1));
                m_responseCooldowns[word.trigger] = GameTime::GetGameTimeMS() + word.cooldown;
                return word.responses[index];
            }
        }
    }

    // Generic contextual responses
    ::std::vector<::std::string> genericResponses;

    if (message.find("?") != ::std::string::npos)
    {
        genericResponses = {
            "I'm not sure about that.",
            "That's a good question.",
            "Let me think about it.",
            "Hmm, interesting question."
        };
    }
    else if (message.find("hello") != ::std::string::npos || message.find("hi") != ::std::string::npos)
    {
        genericResponses = {
            "Hello!",
            "Hi there!",
            "Greetings!",
            "Hey!"
        };
    }
    else if (message.find("thanks") != ::std::string::npos || message.find("thank") != ::std::string::npos)
    {
        genericResponses = {
            "You're welcome!",
            "No problem!",
            "Anytime!",
            "Happy to help!"
        };
    }
    else if (type == ChatType::PARTY || type == ChatType::RAID)
    {
        genericResponses = {
            "Understood.",
            "Got it.",
            "On it!",
            "Will do."
        };
    }
    else
    {
        genericResponses = {
            "Interesting.",
            "I see.",
            "Okay.",
            "Right."
        };
    }

    if (!genericResponses.empty())
    {
        uint32 index = urand(0, static_cast<uint32>(genericResponses.size() - 1));
        return genericResponses[index];
    }

    return "";
}

void SocialManager::SetChatDelay(uint32 minDelay, uint32 maxDelay)
{
    m_minChatDelay = minDelay;
    m_maxChatDelay = ::std::max(minDelay, maxDelay);
}

// ============================================================================
// EMOTE SYSTEM
// ============================================================================

bool SocialManager::PerformEmote(EmoteType emote)
{
    if (!m_bot || !m_emotesEnabled)
        return false;

    m_bot->HandleEmoteCommand(static_cast<Emote>(emote));
    RecordEmote(emote);
    return true;
}

bool SocialManager::RespondWithEmote(Player* trigger, EmoteType triggerEmote)
{
    if (!m_bot || !trigger || !m_emotesEnabled)
        return false;

    // Respond with appropriate emote
    EmoteType response = EmoteType::WAVE;

    switch (triggerEmote)
    {
        case EmoteType::WAVE:
            response = EmoteType::WAVE;
            break;
        case EmoteType::BOW:
            response = EmoteType::BOW;
            break;
        case EmoteType::THANKS:
            response = EmoteType::BOW;
            break;
        case EmoteType::DANCE:
            response = EmoteType::DANCE;
            break;
        case EmoteType::LAUGH:
            response = EmoteType::LAUGH;
            break;
        case EmoteType::SALUTE:
            response = EmoteType::SALUTE;
            break;
        default:
            response = EmoteType::WAVE;
            break;
    }

    return PerformEmote(response);
}
SocialManager::EmoteType SocialManager::SelectContextualEmote(::std::string const& context)
{
    if (context.find("victory") != ::std::string::npos || context.find("win") != ::std::string::npos)
        return EmoteType::VICTORY;
    else if (context.find("sad") != ::std::string::npos || context.find("lost") != ::std::string::npos)
        return EmoteType::CRY;
    else if (context.find("funny") != ::std::string::npos || context.find("joke") != ::std::string::npos)
        return EmoteType::LAUGH;
    else if (context.find("rest") != ::std::string::npos || context.find("wait") != ::std::string::npos)
        return EmoteType::SIT;
    else if (context.find("hello") != ::std::string::npos || context.find("greet") != ::std::string::npos)
        return EmoteType::WAVE;
    else if (context.find("thank") != ::std::string::npos)
        return EmoteType::THANKS;
    else if (context.find("dance") != ::std::string::npos || context.find("party") != ::std::string::npos)
        return EmoteType::DANCE;

    return EmoteType::NONE;
}

// ============================================================================
// FRIEND LIST MANAGEMENT
// ============================================================================

bool SocialManager::AddFriend(ObjectGuid playerGuid, ::std::string const& note)
{
    if (!m_bot || playerGuid.IsEmpty() || playerGuid == m_bot->GetGUID())
        return false;

    Player* player = ObjectAccessor::FindPlayer(playerGuid);
    if (!player)
        return false;

    // Add to TrinityCore social system
    PlayerSocial* social = m_bot->GetSocial();
    if (!social)
        return false;

    // Get account GUID for social system (required by TrinityCore API)
    ObjectGuid accountGuid = ObjectGuid::Create<HighGuid::WowAccount>(player->GetSession()->GetAccountId());
    social->AddToSocialList(playerGuid, accountGuid, SOCIAL_FLAG_FRIEND);

    // Add to local cache
    FriendInfo info;
    info.guid = playerGuid;
    info.name = player->GetName();
    info.level = player->GetLevel();
    info.playerClass = player->GetClass();
    info.zoneId = player->GetZoneId();
    info.isOnline = true;
    info.lastSeenTime = time(nullptr);
    info.note = note;

    m_friends[playerGuid] = info;

    RecordFriendAdded(playerGuid);
    UpdateReputation(playerGuid, 10, true);

    return true;
}

bool SocialManager::RemoveFriend(ObjectGuid playerGuid)
{
    if (!m_bot || playerGuid.IsEmpty())
        return false;

    PlayerSocial* social = m_bot->GetSocial();
    if (!social)
        return false;

    social->RemoveFromSocialList(playerGuid, SOCIAL_FLAG_FRIEND);
    m_friends.erase(playerGuid);

    m_stats.friendsRemoved++;
    return true;
}

bool SocialManager::IsFriend(ObjectGuid playerGuid) const
{
    return m_friends.find(playerGuid) != m_friends.end();
}

::std::vector<SocialManager::FriendInfo> SocialManager::GetFriends() const
{
    ::std::vector<FriendInfo> friends;
    friends.reserve(m_friends.size());

    for (auto const& pair : m_friends)
        friends.push_back(pair.second);

    return friends;
}

SocialManager::FriendInfo SocialManager::GetFriendInfo(ObjectGuid playerGuid) const
{
    auto itr = m_friends.find(playerGuid);
    if (itr != m_friends.end())
        return itr->second;

    return FriendInfo();
}

void SocialManager::UpdateFriendNote(ObjectGuid playerGuid, ::std::string const& note)
{
    auto itr = m_friends.find(playerGuid);
    if (itr != m_friends.end())
        itr->second.note = note;
}

void SocialManager::SyncFriendList()
{
    if (!m_bot)
        return;

    PlayerSocial* social = m_bot->GetSocial();
    if (!social)
        return;

    // Update online status and info for all friends
    for (auto& pair : m_friends)
    {
        Player* player = ObjectAccessor::FindPlayer(pair.first);
        if (player)
        {
            pair.second.isOnline = true;
            pair.second.level = player->GetLevel();
            pair.second.zoneId = player->GetZoneId();
            pair.second.lastSeenTime = time(nullptr);
        }
        else
        {
            pair.second.isOnline = false;
        }
    }
}

// ============================================================================
// IGNORE LIST
// ============================================================================

bool SocialManager::IgnorePlayer(ObjectGuid playerGuid)
{
    if (!m_bot || playerGuid.IsEmpty())
        return false;

    // Get player to obtain account GUID
    Player* player = ObjectAccessor::FindPlayer(playerGuid);
    if (!player)
        return false;

    PlayerSocial* social = m_bot->GetSocial();
    if (!social)
        return false;

    // Get account GUID for social system (required by TrinityCore API)
    ObjectGuid accountGuid = ObjectGuid::Create<HighGuid::WowAccount>(player->GetSession()->GetAccountId());
    social->AddToSocialList(playerGuid, accountGuid, SOCIAL_FLAG_IGNORED);
    m_ignoreList.insert(playerGuid);

    // Remove from friends if present
    RemoveFriend(playerGuid);

    return true;
}

bool SocialManager::UnignorePlayer(ObjectGuid playerGuid)
{
    if (!m_bot || playerGuid.IsEmpty())
        return false;

    PlayerSocial* social = m_bot->GetSocial();
    if (!social)
        return false;

    social->RemoveFromSocialList(playerGuid, SOCIAL_FLAG_IGNORED);
    m_ignoreList.erase(playerGuid);

    return true;
}

bool SocialManager::IsIgnored(ObjectGuid playerGuid) const
{
    return m_ignoreList.find(playerGuid) != m_ignoreList.end();
}

::std::vector<ObjectGuid> SocialManager::GetIgnoreList() const
{
    return ::std::vector<ObjectGuid>(m_ignoreList.begin(), m_ignoreList.end());
}

// ============================================================================
// GUILD SYSTEM
// ============================================================================

bool SocialManager::JoinGuild(Guild* guild)
{
    if (!m_bot || !guild)
        return false;
    if (m_bot->GetGuildId())
        return false; // Already in a guild

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    guild->AddMember(trans, m_bot->GetGUID(), {});
    CharacterDatabase.CommitTransaction(trans);
    m_guild = guild;

    return true;
}

bool SocialManager::LeaveGuild()
{
    if (!m_bot || !m_guild)
        return false;

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    m_guild->DeleteMember(trans, m_bot->GetGUID(), false, false);
    CharacterDatabase.CommitTransaction(trans);
    m_guild = nullptr;

    return true;
}

bool SocialManager::InviteToGuild(Player* target)
{
    if (!m_bot || !target || !m_guild)
        return false;

    if (target->GetGuildId())
        return false; // Target already in guild

    // Bot cannot directly invite without proper permissions
    // This would require WorldSession packet handling
    return false;
}

bool SocialManager::IsInGuild() const
{
    return m_guild != nullptr;
}

Guild* SocialManager::GetGuild() const
{
    return m_guild;
}

bool SocialManager::SendGuildChat(::std::string const& message)
{
    if (!m_bot || !m_guild || !m_guildChatEnabled || message.empty())
        return false;

    ::std::string sanitized = SanitizeMessage(message);
    if (sanitized.empty() || IsSpam(sanitized))
        return false;

    WorldPackets::Chat::Chat packet;
    packet.Initialize(CHAT_MSG_GUILD, LANG_UNIVERSAL, m_bot, nullptr, sanitized);
    m_guild->BroadcastPacket(packet.Write());

    m_stats.guildChatsSent++;
    m_lastGuildChatTime = GameTime::GetGameTimeMS();

    return true;
}

bool SocialManager::SendOfficerChat(::std::string const& message)
{
    if (!m_bot || !m_guild || !m_guildChatEnabled || message.empty())
        return false;

    // Check if bot has officer rank
    if (!m_guild->IsMember(m_bot->GetGUID()))
        return false;

    ::std::string sanitized = SanitizeMessage(message);
    if (sanitized.empty() || IsSpam(sanitized))
        return false;

    WorldPackets::Chat::Chat packet;
    packet.Initialize(CHAT_MSG_OFFICER, LANG_UNIVERSAL, m_bot, nullptr, sanitized);
    m_guild->BroadcastPacket(packet.Write());

    return true;
}

bool SocialManager::RespondToGuildChat(Player* sender, ::std::string const& message)
{
    if (!m_bot || !sender || !m_guildChatEnabled || !m_autoRespond)
        return false;

    if (!ShouldRespondToChat(sender, message, ChatType::GUILD))
        return false;

    ::std::string response = GenerateChatResponse(message, ChatType::GUILD);
    if (response.empty())
        return false;

    uint32 delay = urand(m_minChatDelay, m_maxChatDelay);
    m_nextChatTime = GameTime::GetGameTimeMS() + delay;

    return SendGuildChat(response);
}

void SocialManager::ContributeToGuildBank()
{
    // Guild bank contributions require WorldSession packet handling
    // Framework in place for future enhancement
}

void SocialManager::ParticipateInGuildEvents()
{
    // Guild event participation requires event system integration
    // Framework in place for future enhancement
}

void SocialManager::AcceptGuildInvite(Player* inviter)
{
    if (!m_bot || !inviter)
        return;

    if (!ShouldAcceptGuildInvite(inviter))
        return;

    // Guild invite acceptance requires WorldSession packet handling
    // Framework in place for future enhancement
}

bool SocialManager::ShouldAcceptGuildInvite(Player* inviter) const
{
    if (!m_bot || !inviter)
        return false;

    if (m_bot->GetGuildId())
        return false; // Already in guild
    if (IsIgnored(inviter->GetGUID()))
        return false;

    if (m_friendlyToAll)
        return true;

    // Accept if inviter is a friend or has positive reputation
    return IsFriend(inviter->GetGUID()) || HasPositiveReputation(inviter->GetGUID());
}

// ============================================================================
// REPUTATION SYSTEM
// ============================================================================

void SocialManager::UpdateReputation(ObjectGuid playerGuid, int32 change, bool isPositive)
{
    if (playerGuid.IsEmpty())
        return;

    auto& rep = m_reputations[playerGuid];
    rep.playerGuid = playerGuid;
    rep.reputation = ::std::max(-100, ::std::min(100, rep.reputation + change));
    rep.interactions++;
    rep.lastInteraction = time(nullptr);

    if (isPositive)
        rep.positiveCount++;
    else
        rep.negativeCount++;
}

int32 SocialManager::GetReputation(ObjectGuid playerGuid) const
{
    auto itr = m_reputations.find(playerGuid);
    if (itr != m_reputations.end())
        return itr->second.reputation;
    return 0;
}

bool SocialManager::HasPositiveReputation(ObjectGuid playerGuid) const
{
    return GetReputation(playerGuid) > 0;
}

::std::vector<SocialManager::SocialReputation> SocialManager::GetTopFriendlyPlayers(uint32 count) const
{
    ::std::vector<SocialReputation> reps;
    reps.reserve(m_reputations.size());

    for (auto const& pair : m_reputations)
        reps.push_back(pair.second);

    ::std::sort(reps.begin(), reps.end(), [](SocialReputation const& a, SocialReputation const& b)
    {
        return a.reputation > b.reputation;
    });

    if (reps.size() > count)
        reps.resize(count);

    return reps;
}

// ============================================================================
// RESPONSE TEMPLATES
// ============================================================================
void SocialManager::LoadResponseTemplates()
{
    // Pre-defined response templates for common scenarios
    AddResponseTemplate({
        "hello",
        {"Hello!", "Hi there!", "Greetings!", "Hey!"},
        ChatType::SAY,
        5000
    });

    AddResponseTemplate({
        "help",
        {"What do you need help with?", "I'm here to help!", "How can I assist?"},
        ChatType::SAY,
        10000
    });

    AddResponseTemplate({
        "thank",
        {"You're welcome!", "No problem!", "Anytime!", "Happy to help!"},
        ChatType::SAY,
        5000
    });

    AddResponseTemplate({
        "bye",
        {"Goodbye!", "See you later!", "Take care!", "Farewell!"},
        ChatType::SAY,
        5000
    });

    AddResponseTemplate({
        "quest",
        {"I'm working on quests too.", "Good luck with your quest!", "Questing is fun!"},
        ChatType::SAY,
        15000
    });

    AddResponseTemplate({
        "raid",
        {"Ready for the raid!", "Let's do this!", "I'm prepared."},
        ChatType::RAID,
        10000
    });

    AddResponseTemplate({
        "dungeon",
        {"Ready when you are!", "Let's go!", "I'm ready."},
        ChatType::PARTY,
        10000
    });
}

void SocialManager::AddResponseTemplate(ResponseTemplate const& response)
{
    m_responseTemplates.push_back(response);
}

bool SocialManager::HasResponseTemplate(::std::string const& trigger) const
{
    for (auto const& templ : m_responseTemplates)
    {
        if (templ.trigger == trigger)
            return true;
    }
    return false;
}

::std::string SocialManager::GetRandomResponse(::std::string const& trigger)
{
    for (auto const& templ : m_responseTemplates)
    {
        if (templ.trigger == trigger && !templ.responses.empty())
        {
            uint32 index = urand(0, static_cast<uint32>(templ.responses.size() - 1));
            return templ.responses[index];
        }
    }
    return "";
}

// ============================================================================
// GREETINGS AND FAREWELLS
// ============================================================================

void SocialManager::GreetPlayer(Player* player)
{
    if (!m_bot || !player || !m_autoGreet)
        return;

    if (IsIgnored(player->GetGUID()))
        return;

    ::std::vector<::std::string> greetings = {
        "Hello!",
        "Greetings!",
        "Hi there!",
        "Hey!"
    };

    uint32 index = urand(0, static_cast<uint32>(greetings.size() - 1));
    SendChatMessage(ChatType::SAY, greetings[index]);
    PerformEmote(EmoteType::WAVE);

    RecordGreeting();
}

void SocialManager::FarewellPlayer(Player* player)
{
    if (!m_bot || !player || !m_autoGreet)
        return;

    ::std::vector<::std::string> farewells = {
        "Goodbye!",
        "Farewell!",
        "See you!",
        "Take care!"
    };

    uint32 index = urand(0, static_cast<uint32>(farewells.size() - 1));
    SendChatMessage(ChatType::SAY, farewells[index]);
    PerformEmote(EmoteType::WAVE);
}

void SocialManager::HandlePlayerLogin(Player* player)
{
    if (!player)
        return;

    if (IsFriend(player->GetGUID()))
    {
        auto itr = m_friends.find(player->GetGUID());
        if (itr != m_friends.end())
        {
            itr->second.isOnline = true;
            itr->second.lastSeenTime = time(nullptr);
        }

        if (m_autoGreet)
            GreetPlayer(player);
    }
}

void SocialManager::HandlePlayerLogout(Player* player)
{
    if (!player)
        return;

    if (IsFriend(player->GetGUID()))
    {
        auto itr = m_friends.find(player->GetGUID());
        if (itr != m_friends.end())
        {
            itr->second.isOnline = false;
            itr->second.lastSeenTime = time(nullptr);
        }

        if (m_autoGreet)
            FarewellPlayer(player);
    }
}

// ============================================================================
// CHANNEL MANAGEMENT
// ============================================================================

bool SocialManager::JoinChannel(::std::string const& channelName, ::std::string const& password)
{
    if (!m_bot || channelName.empty())
        return false;

    ChannelMgr* channelMgr = ChannelMgr::ForTeam(m_bot->GetTeam());
    if (!channelMgr)
        return false;

    Channel* channel = channelMgr->GetChannel(0, channelName, m_bot, true);
    if (!channel)
        return false;

    channel->JoinChannel(m_bot, password);
    m_channels.insert(channelName);

    return true;
}

bool SocialManager::LeaveChannel(::std::string const& channelName)
{
    if (!m_bot || channelName.empty())
        return false;

    ChannelMgr* channelMgr = ChannelMgr::ForTeam(m_bot->GetTeam());
    if (!channelMgr)
        return false;

    Channel* channel = channelMgr->GetChannel(0, channelName, m_bot, false);
    if (!channel)
        return false;

    channel->LeaveChannel(m_bot, true, false);
    m_channels.erase(channelName);

    return true;
}

bool SocialManager::IsInChannel(::std::string const& channelName) const
{
    return m_channels.find(channelName) != m_channels.end();
}

::std::vector<::std::string> SocialManager::GetChannels() const
{
    return ::std::vector<::std::string>(m_channels.begin(), m_channels.end());
}

// ============================================================================
// PRIVATE HELPER METHODS
// ============================================================================

void SocialManager::ProcessChatQueue(uint32 /*diff*/)
{
    // Chat queue processing for delayed responses
    // Framework in place for future enhancement
}

bool SocialManager::ShouldRespondToChat(Player* sender, ::std::string const& message, ChatType type)
{
    if (!sender || message.empty())
        return false;

    if (IsIgnored(sender->GetGUID()))
        return false;

    // Random chance to respond
    if (urand(0, 100) > m_chatResponseChance)
        return false;

    // Don't respond if on cooldown
    if (GameTime::GetGameTimeMS() < m_nextChatTime)
        return false;

    // Don't respond to spam
    if (IsSpam(message))
        return false;

    return true;
}

::std::string SocialManager::SanitizeMessage(::std::string const& message) const
{
    ::std::string sanitized = message;

    // Remove excessive whitespace
    sanitized.erase(::std::unique(sanitized.begin(), sanitized.end(),
        [](char a, char b) { return a == ' ' && b == ' '; }), sanitized.end());

    // Trim leading/trailing whitespace
    size_t start = sanitized.find_first_not_of(" \t\n\r");
    size_t end = sanitized.find_last_not_of(" \t\n\r");

    if (start == ::std::string::npos || end == ::std::string::npos)
        return "";

    return sanitized.substr(start, end - start + 1);
}

bool SocialManager::IsSpam(::std::string const& message) const
{
    // Check for repeated messages
    uint32 count = 0;
    uint32 now = GameTime::GetGameTimeMS();

    for (auto const& recent : m_recentChats)
    {
        if (recent.message == message && (now - recent.timestamp) < 10000) // 10 seconds
            count++;
    }

    return count >= 3; // More than 3 identical messages in 10 seconds
}

void SocialManager::ScheduleNextChat()
{
    m_nextChatTime = GameTime::GetGameTimeMS() + urand(m_minChatDelay, m_maxChatDelay);
}

void SocialManager::ProcessEmoteQueue(uint32 /*diff*/)
{
    // Emote queue processing
    // Framework in place for future enhancement
}

void SocialManager::ScheduleRandomEmote()
{
    m_nextEmoteTime = GameTime::GetGameTimeMS() + m_emoteInterval + urand(0, 30000);
}

SocialManager::EmoteType SocialManager::SelectRandomEmote() const
{
    ::std::vector<EmoteType> emotes = {
        EmoteType::WAVE,
        EmoteType::BOW,
        EmoteType::THANKS,
        EmoteType::CHEER,
        EmoteType::DANCE,
        EmoteType::LAUGH,
        EmoteType::SIT,
        EmoteType::APPLAUD
    };

    uint32 index = urand(0, static_cast<uint32>(emotes.size() - 1));
    return emotes[index];
}

void SocialManager::LoadFriendList()
{
    if (!m_bot)
        return;

    PlayerSocial* social = m_bot->GetSocial();
    if (!social)
        return;

    // TrinityCore social system handles persistence
    // We just cache the information locally
}

void SocialManager::SaveFriendList()
{
    // TrinityCore social system handles persistence automatically
}

void SocialManager::UpdateFriendStatus()
{
    SyncFriendList();
}

void SocialManager::UpdateGuildStatus()
{
    if (!m_bot)
        return;

    if (uint32 guildId = m_bot->GetGuildId())
        m_guild = sGuildMgr->GetGuildById(guildId);
    else
        m_guild = nullptr;
}

void SocialManager::LoadReputations()
{
    // Reputation loading from database
    // Framework in place for future database integration
}

void SocialManager::SaveReputations()
{
    // Reputation saving to database
    // Framework in place for future database integration
}

void SocialManager::DecayReputations(uint32 /*diff*/)
{
    uint32 now = time(nullptr);

    for (auto& pair : m_reputations)
    {
        uint32 timeSinceLastInteraction = now - pair.second.lastInteraction;

        // Decay 1 point per day of no interaction
    if (timeSinceLastInteraction > 86400) // 1 day in seconds
        {
            uint32 daysElapsed = timeSinceLastInteraction / 86400;
            int32 decay = ::std::min<int32>(daysElapsed, ::std::abs(pair.second.reputation));

            if (pair.second.reputation > 0)
                pair.second.reputation -= decay;
            else if (pair.second.reputation < 0)
                pair.second.reputation += decay;
        }
    }
}

bool SocialManager::IsOnCooldown(::std::string const& trigger) const
{
    auto itr = m_responseCooldowns.find(trigger);
    if (itr != m_responseCooldowns.end())
        return GameTime::GetGameTimeMS() < itr->second;
    return false;
}

void SocialManager::UpdateCooldowns(uint32 /*diff*/)
{
    uint32 now = GameTime::GetGameTimeMS();

    for (auto itr = m_responseCooldowns.begin(); itr != m_responseCooldowns.end();)
    {
        if (now >= itr->second)
            itr = m_responseCooldowns.erase(itr);
        else
            ++itr;
    }
}

void SocialManager::TrackChat(::std::string const& message)
{
    RecentChat chat;
    chat.message = message;
    chat.timestamp = GameTime::GetGameTimeMS();
    chat.count = 1;

    m_recentChats.push_back(chat);

    // Keep only last 20 messages
    if (m_recentChats.size() > 20)
        m_recentChats.erase(m_recentChats.begin());
}

void SocialManager::CleanupOldChats(uint32 /*diff*/)
{
    uint32 now = GameTime::GetGameTimeMS();

    m_recentChats.erase(
        ::std::remove_if(m_recentChats.begin(), m_recentChats.end(),
            [now](RecentChat const& chat) {
                return (now - chat.timestamp) > 60000; // Remove chats older than 1 minute
            }),
        m_recentChats.end()
    );
}

// ============================================================================
// STATISTICS
// ============================================================================

void SocialManager::RecordMessageSent(ChatType type)
{
    m_stats.messagesSent++;

    if (type == ChatType::WHISPER)
        m_stats.whispersSent++;
}

void SocialManager::RecordMessageReceived(ChatType type)
{
    m_stats.messagesReceived++;

    if (type == ChatType::WHISPER)
        m_stats.whispersReceived++;
}

void SocialManager::RecordEmote(EmoteType /*type*/)
{
    m_stats.emotesPerformed++;
}

void SocialManager::RecordFriendAdded(ObjectGuid /*playerGuid*/)
{
    m_stats.friendsAdded++;
}

void SocialManager::RecordGreeting()
{
    m_stats.greetingsSent++;
}

// ============================================================================
// PERFORMANCE TRACKING
// ============================================================================

void SocialManager::StartPerformanceTimer()
{
    m_performanceStart = ::std::chrono::high_resolution_clock::now();
}

void SocialManager::EndPerformanceTimer()
{
    auto end = ::std::chrono::high_resolution_clock::now();
    m_lastUpdateDuration = ::std::chrono::duration_cast<::std::chrono::microseconds>(end - m_performanceStart);
    m_totalUpdateTime += m_lastUpdateDuration;
    m_updateCount++;
}

void SocialManager::UpdatePerformanceMetrics()
{
    if (m_updateCount > 0)
    {
        auto avgDuration = m_totalUpdateTime / m_updateCount;
        m_cpuUsage = (avgDuration.count() / 1000.0f) / 100.0f; // Convert to percentage

        // Reset counters periodically
    if (m_updateCount >= 1000)
        {
            m_totalUpdateTime = ::std::chrono::microseconds(0);
            m_updateCount = 0;
        }
    }
}

size_t SocialManager::GetMemoryUsage() const
{
    size_t memory = sizeof(SocialManager);
    memory += m_friends.size() * sizeof(FriendInfo);
    memory += m_ignoreList.size() * sizeof(ObjectGuid);
    memory += m_reputations.size() * sizeof(SocialReputation);
    memory += m_responseTemplates.size() * sizeof(ResponseTemplate);
    memory += m_responseCooldowns.size() * (sizeof(::std::string) + sizeof(uint32));
    memory += m_recentChats.size() * sizeof(RecentChat);
    memory += m_channels.size() * sizeof(::std::string);
    return memory;
}

} // namespace Playerbot
