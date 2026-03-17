/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "Player.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Chat.h"
#include <unordered_map>
#include <vector>
#include <queue>
#include <atomic>
#include <mutex>
#include <chrono>
#include "GameTime.h"

namespace Playerbot
{

/**
 * @brief Types of guild activities
 */
enum class GuildActivityType : uint8
{
    CHAT_PARTICIPATION = 0,
    BANK_DEPOSIT = 1,
    BANK_WITHDRAW = 2,
    EVENT_ATTENDANCE = 3,
    RECRUITMENT = 4,
    HELPING_MEMBERS = 5,
    ACHIEVEMENT_CONTRIBUTION = 6,
    RAID_PARTICIPATION = 7,
    PVP_ACTIVITY = 8,
    SOCIAL_GATHERING = 9,
    SOCIAL_INTERACTION = 10,
    GUILD_BANK_INTERACTION = 11,
    GUILD_EVENT_ATTENDANCE = 12,
    OFFICER_DUTIES = 13,
    RECRUITMENT_ASSISTANCE = 14
};

/**
 * @brief Guild member roles
 */
enum class GuildRole : uint8
{
    MEMBER = 0,
    INITIATE = 1,
    VETERAN = 2,
    OFFICER = 3,
    LEADER = 4,
    RECRUITER = 5,
    BANKER = 6,
    EVENT_ORGANIZER = 7
};

/**
 * @brief Guild chat style
 */
enum class GuildChatStyle : uint8
{
    MINIMAL = 0,
    MODERATE = 1,
    ACTIVE = 2,
    SOCIAL = 3,
    HELPFUL = 4,
    PROFESSIONAL = 5
};

/**
 * @brief Guild chat message for processing
 */
struct GuildChatMessage
{
    uint32 senderGuid{0};
    uint32 senderId{0};
    std::string senderName;
    std::string message;
    std::string content;
    uint32 timestamp{0};
    bool isOfficerChat{false};
    bool requiresResponse{false};

    GuildChatMessage() = default;
    GuildChatMessage(uint32 sender, const std::string& name, const std::string& msg)
        : senderGuid(sender), senderId(sender), senderName(name), message(msg), content(msg), timestamp(GameTime::GetGameTimeMS()) {}
};

/**
 * @brief Guild profile configuration
 */
struct GuildProfile
{
    uint32 playerGuid{0};
    float chatParticipationLevel{0.5f};
    float bankContributionLevel{0.5f};
    float eventAttendanceLevel{0.5f};
    float participationLevel{0.5f};
    float helpfulnessLevel{0.5f};
    bool autoRespondToChat{true};
    bool autoBankDeposit{true};
    bool autoAttendEvents{true};
    std::vector<std::string> preferredTopics;
    std::vector<GuildActivityType> activeActivities;
    GuildRole preferredRole{GuildRole::MEMBER};
    uint32 lastProfileUpdate{0};
    GuildChatStyle chatStyle{GuildChatStyle::MODERATE};
    std::vector<std::string> interests;
    std::vector<std::string> expertise;

    GuildProfile() = default;
    GuildProfile(uint32 guid) : playerGuid(guid) {}
};

/**
 * @brief Guild participation tracking
 */
struct GuildParticipation
{
    uint32 playerGuid{0};
    uint32 chatMessagesCount{0};
    uint32 totalChatMessages{0};
    uint32 bankDepositsCount{0};
    uint32 bankWithdrawalsCount{0};
    uint32 eventsAttended{0};
    uint32 membersHelped{0};
    float participationScore{0.0f};
    uint32 lastParticipationTime{0};
    uint32 lastActivity{0};
    std::unordered_map<GuildActivityType, uint32> activityCounts;

    // Additional tracking fields
    float contributionScore{0.0f};
    float socialScore{0.0f};
    float leadershipScore{0.0f};
    uint32 respondedMessages{0};
    float averageResponseTime{0.0f};
    uint32 conversationsInitiated{0};
    std::unordered_map<std::string, uint32> chatTopics;

    GuildParticipation() = default;
    GuildParticipation(uint32 guid) : playerGuid(guid) {}
    GuildParticipation(uint32 guid, uint32 /*guildId*/) : playerGuid(guid) {}

    void Reset()
    {
        chatMessagesCount = 0;
        totalChatMessages = 0;
        bankDepositsCount = 0;
        bankWithdrawalsCount = 0;
        eventsAttended = 0;
        membersHelped = 0;
        participationScore = 0.0f;
        lastParticipationTime = 0;
        lastActivity = 0;
        activityCounts.clear();
        contributionScore = 0.0f;
        socialScore = 0.0f;
        leadershipScore = 0.0f;
        respondedMessages = 0;
        averageResponseTime = 0.0f;
        conversationsInitiated = 0;
        chatTopics.clear();
    }
};

/**
 * @brief Guild metrics tracking
 */
struct GuildMetrics
{
    uint32 totalInteractions{0};
    uint32 successfulInteractions{0};
    uint32 chatResponses{0};
    uint32 bankTransactions{0};
    uint32 eventsCoordinated{0};
    float averageParticipation{0.0f};
    uint32 lastMetricsUpdate{0};

    // Extended metrics
    float guildRelationship{0.5f};
    float participationTrend{0.0f};
    float interactionQuality{0.5f};
    float leadershipContribution{0.0f};
    float chatEffectiveness{0.5f};
    float responseRate{0.0f};
    uint32 helpRequestsFulfilled{0};
    uint32 totalHelpRequests{0};
    uint32 guildInteractions{0};
    uint32 chatMessages{0};
    uint32 eventsParticipated{0};
    uint32 helpfulActions{0};
    uint32 lastUpdate{0};

    GuildMetrics() = default;

    void Reset()
    {
        totalInteractions = 0;
        successfulInteractions = 0;
        chatResponses = 0;
        bankTransactions = 0;
        eventsCoordinated = 0;
        averageParticipation = 0.0f;
        lastMetricsUpdate = 0;
        guildRelationship = 0.5f;
        participationTrend = 0.0f;
        interactionQuality = 0.5f;
        leadershipContribution = 0.0f;
        chatEffectiveness = 0.5f;
        responseRate = 0.0f;
        helpRequestsFulfilled = 0;
        totalHelpRequests = 0;
        guildInteractions = 0;
        chatMessages = 0;
        eventsParticipated = 0;
        helpfulActions = 0;
        lastUpdate = 0;
    }
};

/**
 * @brief Comprehensive guild integration system for automated guild participation
 *
 * This system provides intelligent guild interactions including chat participation,
 * guild bank management, event coordination, and social activities for playerbots.
 */
class TC_GAME_API GuildIntegration final 
{
public:
    explicit GuildIntegration(Player* bot);
    ~GuildIntegration();
    GuildIntegration(GuildIntegration const&) = delete;
    GuildIntegration& operator=(GuildIntegration const&) = delete;

    // Core guild functionality using TrinityCore's Guild system
    void ProcessGuildInteraction();
    void HandleGuildChat(const GuildChatMessage& message);
    void ParticipateInGuildActivities();
    void ManageGuildResponsibilities();

    // Guild chat automation
    void AutomateGuildChatParticipation();
    void RespondToGuildChat(const GuildChatMessage& message);
    void InitiateGuildConversation();
    void ShareGuildInformation(const std::string& topic);

    // Guild bank management
    void AutomateGuildBankInteractions();
    void DepositItemsToGuildBank();
    void WithdrawNeededItems();
    void OrganizeGuildBank();
    void ManageGuildBankPermissions();

    // Guild event coordination
    void CoordinateGuildEvents();
    void ScheduleGuildActivities();
    void ManageGuildCalendar();
    void OrganizeGuildRuns();

    // Advanced guild features
    void SetGuildProfile(const GuildProfile& profile);
    GuildProfile GetGuildProfile();

    // Guild participation tracking
    GuildParticipation GetGuildParticipation();
    void UpdateGuildParticipation(GuildActivityType activityType);

    // Guild recruitment assistance
    void AssistWithRecruitment();
    void EvaluateRecruitmentCandidates();
    void WelcomeNewGuildMembers();
    void MentorJuniorMembers();

    // Guild leadership support
    void SupportGuildLeadership();
    void HandleOfficerDuties();
    void AssistWithGuildManagement();
    void ProvideMemberFeedback();

    // Chat intelligence and response generation
    struct ChatIntelligence
    {
        std::unordered_map<std::string, std::vector<std::string>> responseTemplates;
        std::unordered_map<std::string, float> keywordWeights;
        std::vector<std::string> conversationStarters;
        std::vector<std::string> helpfulTips;
        std::unordered_map<std::string, std::string> topicResponses;
        uint32 lastResponseTime;
        float responseFrequency;

        ChatIntelligence() : lastResponseTime(0), responseFrequency(0.3f) {}
    };

    std::string GenerateGuildChatResponse(const GuildChatMessage& message);
    std::string GenerateConversationStarter();
    bool ShouldRespondToMessage(const GuildChatMessage& message);
    void LearnFromGuildConversations();

    // Guild achievement coordination
    void ContributeToGuildAchievements();
    void CoordinateAchievementEfforts(Guild* guild);
    void TrackAchievementProgress();
    void CelebrateGuildAchievements();

    // Guild social features
    void OrganizeSocialEvents();
    void ParticipateInGuildTradition();
    void MaintainGuildFriendships();
    void HandleGuildConflicts();

    // Performance monitoring
    GuildMetrics GetPlayerGuildMetrics();
    GuildMetrics GetGuildBotMetrics(uint32 guildId);

    // Guild bank automation
    void OptimizeGuildBankUsage();
    void AutoDepositValuableItems();
    void AutoWithdrawNeededConsumables();
    void ManageGuildBankTabs();
    void TrackGuildBankActivity();

    // Guild event management
    void CreateGuildEvent(const std::string& eventType);
    void ManageGuildCalendarEvents();
    void CoordinateRaidScheduling();
    void OrganizePvPEvents();

    // Advanced guild AI
    void AnalyzeGuildDynamics(Guild* guild);
    void AdaptToGuildCulture();
    void DetectGuildMoodAndTone(Guild* guild);
    void AdjustBehaviorToGuildNorms();

    // Configuration and customization
    void SetGuildAutomationLevel(float level);
    void EnableGuildActivity(GuildActivityType activity, bool enable);
    void SetGuildChatFrequency(float frequency);
    void ConfigureGuildBankAccess(bool autoDeposit, bool autoWithdraw);

    // Error handling and recovery
    void HandleGuildInteractionError(const std::string& error);
    void RecoverFromGuildFailure();
    void HandleGuildLeaving();
    void HandleGuildInvitations(uint32 guildId);

    // Update and maintenance
    void Update(uint32 diff);
    void UpdateGuildParticipation();
    void ProcessGuildEvents();
    void CleanupGuildData();

private:
    Player* _bot;

    // Core guild data
    struct PlayerState {
        uint32 lastGuildBankInteraction = 0;
        uint32 lastChatTime = 0;
        uint32 lastEventParticipation = 0;
    };

    std::unordered_map<uint32, GuildProfile> _playerProfiles; // playerGuid -> profile
    std::unordered_map<uint32, GuildParticipation> _playerParticipation; // playerGuid -> participation
    std::unordered_map<uint32, GuildMetrics> _playerMetrics; // playerGuid -> metrics
    std::unordered_map<uint32, PlayerState> _playerStates; // playerGuid -> state
    

    // Chat intelligence system
    std::unordered_map<uint32, ChatIntelligence> _chatIntelligence; // playerGuid -> intelligence
    std::unordered_map<std::string, std::vector<std::string>> _globalResponseTemplates;
    std::vector<std::string> _conversationTopics;
    

    // Guild activity tracking
    struct GuildActivityTracker
    {
        uint32 guildId;
        std::unordered_map<uint32, uint32> memberActivity; // playerGuid -> activity count
        std::vector<std::pair<std::string, uint32>> recentEvents; // event, timestamp
        std::queue<GuildChatMessage> chatHistory;
        float overallMorale;
        float activityLevel;
        uint32 lastAnalysisTime;

        // Default constructor for std::unordered_map compatibility
        GuildActivityTracker() : guildId(0), overallMorale(0.8f)
            , activityLevel(0.6f), lastAnalysisTime(GameTime::GetGameTimeMS()) {}

        GuildActivityTracker(uint32 gId) : guildId(gId), overallMorale(0.8f)
            , activityLevel(0.6f), lastAnalysisTime(GameTime::GetGameTimeMS()) {}
    };

    std::unordered_map<uint32, GuildActivityTracker> _guildTracking; // guildId -> tracker

    // Performance tracking
    GuildMetrics _globalMetrics;

    // Helper functions
    void InitializeChatTemplates();
    void LoadGuildSpecificData(uint32 guildId);
    bool IsAppropriateTimeToChat();
    void UpdateGuildSocialGraph(uint32 guildId);

    // Chat response generation
    void OfferGuildAssistance(const std::string& assistance);
    void SendGuildChatMessage(const std::string& message);
    std::string SelectResponseTemplate(const std::string& category);
    std::string PersonalizeResponse(const std::string& responseTemplate);
    float CalculateMessageRelevance(const GuildChatMessage& message);
    std::vector<std::string> ExtractKeywords(const std::string& message);

    // Guild bank logic
    bool ShouldDepositItem(uint32 itemId);
    bool ShouldWithdrawItem(uint32 itemId);
    void ProcessGuildBankTransaction(uint32 itemId, bool isDeposit);
    void OptimizeGuildBankLayout();

    // Guild bank helper methods
    uint8 FindBestTabForItem(Item* item) const;
    int8 FindEmptySlotInTab(Guild* guild, uint8 tabId) const;
    bool HasGuildBankDepositRights(uint8 tabId) const;
    bool HasGuildBankWithdrawRights(uint8 tabId) const;
    int32 GetRemainingWithdrawSlots(uint8 tabId) const;
    int8 FindItemInTab(Guild* guild, uint8 tabId, uint32 itemId) const;
    bool FindFreeInventorySlot(uint8& outBag, uint8& outSlot) const;

    // Needs analysis methods
    void AnalyzeConsumableNeeds(std::vector<uint32>& neededItems) const;
    void AnalyzeReagentNeeds(std::vector<uint32>& neededItems) const;
    void AnalyzeProfessionMaterialNeeds(std::vector<uint32>& neededItems) const;
    void AnalyzeEquipmentUpgrades(Guild* guild, std::vector<std::pair<uint8, uint8>>& upgradeLocations) const;

    // Item evaluation methods
    bool HasRelevantProfession(uint32 itemSubClass) const;
    bool IsEquipmentUpgrade(ItemTemplate const* itemTemplate) const;
    bool CanLearnRecipe(ItemTemplate const* itemTemplate) const;
    bool HasItemsNeedingSockets() const;

    // Event coordination
    void PlanGuildEvent(const std::string& eventType, uint32 proposedTime);
    void InviteMembersToEvent(const std::string& eventId);
    void ManageEventAttendance(const std::string& eventId);
    bool IsGoodTimeForEvent(uint32 proposedTime, const std::vector<uint32>& memberIds);

    // Social dynamics analysis
    void AnalyzeGuildPersonalities(uint32 guildId);
    void DetectGuildCliques(uint32 guildId);
    void IdentifyInfluentialMembers(uint32 guildId);
    void AssessGuildHealth(uint32 guildId);

    // Performance optimization
    void OptimizeGuildInteractions();
    void CacheGuildData(uint32 guildId);
    void PrioritizeGuildActivities();
    void UpdateGuildMetrics(GuildActivityType activity, bool wasSuccessful);

    // Constants
    static constexpr uint32 GUILD_UPDATE_INTERVAL = 60000; // 1 minute
    static constexpr uint32 CHAT_PROCESSING_INTERVAL = 5000; // 5 seconds
    static constexpr uint32 ACTIVITY_ANALYSIS_INTERVAL = 300000; // 5 minutes
    static constexpr uint32 MAX_CHAT_HISTORY = 100;
    static constexpr float MIN_PARTICIPATION_SCORE = 0.3f;
    static constexpr uint32 GUILD_BANK_CHECK_INTERVAL = 1800000; // 30 minutes
    static constexpr float DEFAULT_CHAT_FREQUENCY = 0.3f; // 30% chance to respond
    static constexpr uint32 EVENT_PLANNING_ADVANCE = 86400000; // 24 hours
    static constexpr uint32 MAX_GUILD_ACTIVITIES_PER_DAY = 15;
    static constexpr float SOCIAL_SCORE_DECAY = 0.01f; // Daily decay without activity
    static constexpr uint32 NEW_MEMBER_WELCOME_DELAY = 300000; // 5 minutes
};

} // namespace Playerbot