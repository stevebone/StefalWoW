/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_BOT_SOCIAL_MANAGER_H
#define TRINITYCORE_BOT_SOCIAL_MANAGER_H

#include "Define.h"
#include "ObjectGuid.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <memory>
#include <chrono>

class Player;
class Guild;
class WorldPacket;

namespace Playerbot
{
    class BotAI;

    /**
     * SocialManager - Social interaction system for PlayerBots
     *
     * Handles all social features including:
     * - Chat message generation and responses
     * - Emote usage and contextual responses
     * - Friend list management
     * - Guild participation and chat
     * - Social reputation tracking
     */
    class SocialManager
    {
    public:
        explicit SocialManager(Player* bot, BotAI* ai);
        ~SocialManager();

        // Core lifecycle
        void Initialize();
        void Update(uint32 diff);
        void Reset();
        void Shutdown();

        // Chat System
        enum class ChatType : uint8
        {
            SAY,            // /say - Local area chat
            YELL,           // /yell - Large area broadcast
            WHISPER,        // /whisper - Private message
            PARTY,          // /party - Group chat
            RAID,           // /raid - Raid chat
            GUILD,          // /guild - Guild chat
            OFFICER,        // /officer - Guild officer chat
            EMOTE,          // /emote - Text emote
            GENERAL         // General channel
        };

        struct ChatMessage
        {
            ChatType type;
            ::std::string content;
            ObjectGuid target;  // For whispers
            uint32 timestamp;
            uint32 channelId;   // For channel chat
        };

        bool SendChatMessage(ChatType type, ::std::string const& message, ObjectGuid target = ObjectGuid::Empty);
        bool RespondToChat(Player* sender, ::std::string const& message, ChatType type);
        void ProcessIncomingChat(Player* sender, ::std::string const& message, ChatType type);
        ::std::string GenerateChatResponse(::std::string const& message, ChatType type);

        // Chat behavior
        void SetChatEnabled(bool enabled) { m_chatEnabled = enabled; }
        bool IsChatEnabled() const { return m_chatEnabled; }
        void SetChatResponseChance(uint32 percent) { m_chatResponseChance = percent; }
        void SetChatDelay(uint32 minDelay, uint32 maxDelay);

        // Emote System
        enum class EmoteType : uint32
        {
            WAVE = 3,
            BOW = 2,
            THANKS = 77,
            CHEER = 71,
            DANCE = 10,
            LAUGH = 11,
            SLEEP = 16,
            SIT = 8,
            STAND = 0,
            KNEEL = 20,
            CRY = 18,
            CHICKEN = 68,
            BEG = 54,
            APPLAUD = 21,
            SHOUT = 22,
            FLEX = 84,
            SHY = 23,
            RUDE = 25,
            KISS = 17,
            ROAR = 15,
            SALUTE = 66,
            SURPRISED = 60,
            VICTORY = 45,
            DRINK = 7,
            EAT = 7,
            POINT = 25,
            NONE = 1
        };

        bool PerformEmote(EmoteType emote);
        bool RespondWithEmote(Player* trigger, EmoteType triggerEmote);
        EmoteType SelectContextualEmote(::std::string const& context);
        void SetEmotesEnabled(bool enabled) { m_emotesEnabled = enabled; }
        bool AreEmotesEnabled() const { return m_emotesEnabled; }

        // Friend List Management
        struct FriendInfo
        {
            ObjectGuid guid;
            ::std::string name;
            uint8 level;
            uint8 playerClass;
            uint32 zoneId;
            bool isOnline;
            uint32 lastSeenTime;
            ::std::string note;
        };

        bool AddFriend(ObjectGuid playerGuid, ::std::string const& note = "");
        bool RemoveFriend(ObjectGuid playerGuid);
        bool IsFriend(ObjectGuid playerGuid) const;
        ::std::vector<FriendInfo> GetFriends() const;
        FriendInfo GetFriendInfo(ObjectGuid playerGuid) const;
        void UpdateFriendNote(ObjectGuid playerGuid, ::std::string const& note);
        void SyncFriendList();

        // Ignore List
        bool IgnorePlayer(ObjectGuid playerGuid);
        bool UnignorePlayer(ObjectGuid playerGuid);
        bool IsIgnored(ObjectGuid playerGuid) const;
        ::std::vector<ObjectGuid> GetIgnoreList() const;

        // Guild System
        bool JoinGuild(Guild* guild);
        bool LeaveGuild();
        bool InviteToGuild(Player* target);
        bool IsInGuild() const;
        Guild* GetGuild() const;

        // Guild chat participation
        bool SendGuildChat(::std::string const& message);
        bool SendOfficerChat(::std::string const& message);
        bool RespondToGuildChat(Player* sender, ::std::string const& message);
        void SetGuildChatEnabled(bool enabled) { m_guildChatEnabled = enabled; }
        bool IsGuildChatEnabled() const { return m_guildChatEnabled; }

        // Guild contribution
        void ContributeToGuildBank();
        void ParticipateInGuildEvents();
        void AcceptGuildInvite(Player* inviter);
        bool ShouldAcceptGuildInvite(Player* inviter) const;

        // Reputation and Social Standing
        struct SocialReputation
        {
            ObjectGuid playerGuid;
            int32 reputation;       // -100 to +100
            uint32 interactions;    // Number of interactions
            uint32 positiveCount;   // Positive interactions
            uint32 negativeCount;   // Negative interactions
            uint32 lastInteraction; // Timestamp
        };

        void UpdateReputation(ObjectGuid playerGuid, int32 change, bool isPositive);
        int32 GetReputation(ObjectGuid playerGuid) const;
        bool HasPositiveReputation(ObjectGuid playerGuid) const;
        ::std::vector<SocialReputation> GetTopFriendlyPlayers(uint32 count = 10) const;

        // Contextual Responses
        struct ResponseTemplate
        {
            ::std::string trigger;        // Keyword/phrase that triggers response
            ::std::vector<::std::string> responses;  // Possible responses
            ChatType preferredType;     // Preferred chat type
            uint32 cooldown;            // Cooldown in ms
        };

        void LoadResponseTemplates();
        void AddResponseTemplate(ResponseTemplate const& response);
        bool HasResponseTemplate(::std::string const& trigger) const;
        ::std::string GetRandomResponse(::std::string const& trigger);

        // Greetings and Farewells
        void GreetPlayer(Player* player);
        void FarewellPlayer(Player* player);
        void HandlePlayerLogin(Player* player);
        void HandlePlayerLogout(Player* player);

        // Channel Management
        bool JoinChannel(::std::string const& channelName, ::std::string const& password = "");
        bool LeaveChannel(::std::string const& channelName);
        bool IsInChannel(::std::string const& channelName) const;
        ::std::vector<::std::string> GetChannels() const;

        // Configuration
        bool IsEnabled() const { return m_enabled; }
        void SetEnabled(bool enabled) { m_enabled = enabled; }

        void SetAutoGreet(bool enable) { m_autoGreet = enable; }
        void SetAutoRespond(bool enable) { m_autoRespond = enable; }
        void SetRandomEmotes(bool enable) { m_randomEmotes = enable; }
        void SetFriendlyToAll(bool enable) { m_friendlyToAll = enable; }

        // Statistics
        struct Statistics
        {
            uint32 messagesSent = 0;
            uint32 messagesReceived = 0;
            uint32 emotesPerformed = 0;
            uint32 friendsAdded = 0;
            uint32 friendsRemoved = 0;
            uint32 guildChatsSent = 0;
            uint32 whispersSent = 0;
            uint32 whispersReceived = 0;
            uint32 greetingsSent = 0;
        };

        Statistics const& GetStatistics() const { return m_stats; }

        // Performance metrics
        float GetCPUUsage() const { return m_cpuUsage; }
        size_t GetMemoryUsage() const;

    private:
        // Chat processing
        void ProcessChatQueue(uint32 diff);
        bool ShouldRespondToChat(Player* sender, ::std::string const& message, ChatType type);
        ::std::string SanitizeMessage(::std::string const& message) const;
        bool IsSpam(::std::string const& message) const;

        // Chat delay management
        uint32 m_nextChatTime;
        uint32 m_minChatDelay;
        uint32 m_maxChatDelay;
        void ScheduleNextChat();

        // Emote logic
        void ProcessEmoteQueue(uint32 diff);
        uint32 m_nextEmoteTime;
        uint32 m_emoteInterval;
        void ScheduleRandomEmote();
        EmoteType SelectRandomEmote() const;

        // Friend list cache
        ::std::unordered_map<ObjectGuid, FriendInfo> m_friends;
        ::std::unordered_set<ObjectGuid> m_ignoreList;
        void LoadFriendList();
        void SaveFriendList();
        void UpdateFriendStatus();

        // Guild tracking
        Guild* m_guild;
        uint32 m_lastGuildChatTime;
        void UpdateGuildStatus();

        // Reputation tracking
        ::std::unordered_map<ObjectGuid, SocialReputation> m_reputations;
        void LoadReputations();
        void SaveReputations();
        void DecayReputations(uint32 diff);

        // Response templates
        ::std::vector<ResponseTemplate> m_responseTemplates;
        ::std::unordered_map<::std::string, uint32> m_responseCooldowns;
        bool IsOnCooldown(::std::string const& trigger) const;
        void UpdateCooldowns(uint32 diff);

        // Recent chat tracking (for spam prevention)
        struct RecentChat
        {
            ::std::string message;
            uint32 timestamp;
            uint32 count;
        };
        ::std::vector<RecentChat> m_recentChats;
        void TrackChat(::std::string const& message);
        void CleanupOldChats(uint32 diff);

        // Channel membership
        ::std::unordered_set<::std::string> m_channels;

        // Statistics tracking
        void RecordMessageSent(ChatType type);
        void RecordMessageReceived(ChatType type);
        void RecordEmote(EmoteType type);
        void RecordFriendAdded(ObjectGuid playerGuid);
        void RecordGreeting();

        // Performance tracking
        void StartPerformanceTimer();
        void EndPerformanceTimer();
        void UpdatePerformanceMetrics();

    private:
        Player* m_bot;
        BotAI* m_ai;
        bool m_enabled;

        // Configuration
        bool m_chatEnabled;
        bool m_emotesEnabled;
        bool m_guildChatEnabled;
        bool m_autoGreet;
        bool m_autoRespond;
        bool m_randomEmotes;
        bool m_friendlyToAll;
        uint32 m_chatResponseChance;

        // Update intervals
        uint32 m_chatUpdateInterval;
        uint32 m_emoteUpdateInterval;
        uint32 m_reputationUpdateInterval;

        // Last update times
        uint32 m_lastChatUpdate;
        uint32 m_lastEmoteUpdate;
        uint32 m_lastReputationUpdate;

        // Statistics
        Statistics m_stats;

        // Performance metrics
        ::std::chrono::high_resolution_clock::time_point m_performanceStart;
        ::std::chrono::microseconds m_lastUpdateDuration;
        ::std::chrono::microseconds m_totalUpdateTime;
        uint32 m_updateCount;
        float m_cpuUsage;
    };

} // namespace Playerbot

#endif // TRINITYCORE_BOT_SOCIAL_MANAGER_H
