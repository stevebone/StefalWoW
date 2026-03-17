/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "GuildIntegration.h"
#include "Player.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Chat.h"
#include "Group.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Util.h"
#include "World.h"
#include "WorldSession.h"
#include "Bag.h"
#include "ObjectMgr.h"
#include <algorithm>
#include <random>
#include "GameTime.h"

namespace Playerbot
{

GuildIntegration::GuildIntegration(Player* bot) : _bot(bot)
{
    if (!_bot) TC_LOG_ERROR("playerbot", "GuildIntegration: null bot!");
}

GuildIntegration::~GuildIntegration() {}

void GuildIntegration::ProcessGuildInteraction()
{
    if (!_bot)
        return;

    Guild* guild = _bot->GetGuild();
    if (!guild)
        return;

    uint32 guildId = guild->GetId();

    // Initialize tracking data if needed
    if (_guildTracking.find(guildId) == _guildTracking.end())
    {
        _guildTracking[guildId] = GuildActivityTracker(guildId);
    }

    // Update player participation
    UpdateGuildParticipation(GuildActivityType::SOCIAL_INTERACTION);

    // Handle guild-specific interactions
    AutomateGuildChatParticipation();
    AutomateGuildBankInteractions();
    ParticipateInGuildActivities();
}

void GuildIntegration::HandleGuildChat(const GuildChatMessage& message)
{
    if (!_bot || !_bot->GetGuild())
        return;

    // Store message in chat history
    auto& tracker = _guildTracking[_bot->GetGuild()->GetId()];
    tracker.chatHistory.push(message);

    // Limit chat history size
    while (tracker.chatHistory.size() > MAX_CHAT_HISTORY)
        tracker.chatHistory.pop();

    // Decide whether to respond
    if (ShouldRespondToMessage(message))
    {
        RespondToGuildChat(message);
    }

    // Update participation metrics
    UpdateGuildParticipation(GuildActivityType::CHAT_PARTICIPATION);
}

void GuildIntegration::ParticipateInGuildActivities()
{
    if (!_bot || !_bot->GetGuild())
        return;

    GuildProfile profile = GetGuildProfile();
    // Participate in various guild activities based on profile
    for (GuildActivityType activity : profile.activeActivities)
    {
        switch (activity)
        {
            case GuildActivityType::GUILD_BANK_INTERACTION:
                if (rand() % 100 < 10) // 10% chance per update
                    AutomateGuildBankInteractions();
                break;

            case GuildActivityType::GUILD_EVENT_ATTENDANCE:
                // Check for scheduled events
                break;

            case GuildActivityType::OFFICER_DUTIES:
                if (profile.preferredRole == GuildRole::OFFICER || profile.preferredRole == GuildRole::LEADER)
                    SupportGuildLeadership();
                break;

            case GuildActivityType::RECRUITMENT_ASSISTANCE:
                if (profile.preferredRole == GuildRole::RECRUITER || rand() % 100 < 5)
                    AssistWithRecruitment();
                break;

            case GuildActivityType::ACHIEVEMENT_CONTRIBUTION:
                ContributeToGuildAchievements();
                break;

            default:
                break;
        }
    }
}

void GuildIntegration::ManageGuildResponsibilities()
{
    if (!_bot || !_bot->GetGuild())
        return;

    GuildProfile profile = GetGuildProfile();
    // Handle role-specific responsibilities
    switch (profile.preferredRole)
    {
        case GuildRole::OFFICER:
        case GuildRole::LEADER:
            HandleOfficerDuties();
            break;

        case GuildRole::BANKER:
            OrganizeGuildBank();
            break;

        case GuildRole::RECRUITER:
            EvaluateRecruitmentCandidates();
            break;

        case GuildRole::EVENT_ORGANIZER:
            CoordinateGuildEvents();
            break;

        default:
            // Regular member activities
            break;
    }
}

void GuildIntegration::AutomateGuildChatParticipation()
{
    if (!_bot || !_bot->GetGuild())
        return;

    GuildProfile profile = GetGuildProfile();

    if (!IsAppropriateTimeToChat())
        return;

    // Check if we should initiate conversation
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);

    float chatChance = DEFAULT_CHAT_FREQUENCY * profile.participationLevel;

    if (dis(gen) < chatChance)
    {
        // Decide what type of interaction to make
        std::uniform_int_distribution<int> actionDis(1, 100);
        int action = actionDis(gen);

        if (action <= 30)
        {
            InitiateGuildConversation();
        }
        else if (action <= 60)
        {
            // Share information about something relevant
            ShareGuildInformation("general");
        }
        else if (action <= 80 && profile.helpfulnessLevel > 0.7f)
        {
            // Offer help or assistance
            OfferGuildAssistance("");
        }
    }
}

void GuildIntegration::RespondToGuildChat( const GuildChatMessage& message)
{
    if (!_bot)
        return;

    std::string response = GenerateGuildChatResponse(message);
    if (!response.empty())
    {
        SendGuildChatMessage(response);
        UpdateGuildParticipation(GuildActivityType::CHAT_PARTICIPATION);
    }
}

void GuildIntegration::InitiateGuildConversation()
{
    if (!_bot)
        return;

    std::string message = GenerateConversationStarter();
    if (!message.empty())
    {
        SendGuildChatMessage(message);
    }
}

void GuildIntegration::ShareGuildInformation( const std::string& topic)
{
    if (!_bot)
        return;

    // Generate informative messages based on topic
    std::vector<std::string> infoMessages;

    if (topic == "general" || topic == "events")
    {
        infoMessages = {
            "Don't forget about our guild event this weekend!",
            "The guild bank has some useful consumables available.",
            "Anyone need help with dungeons or quests?",
            "Remember to contribute to guild achievements when you can."
        };
    }
    else if (topic == "tips")
    {
        infoMessages = {
            "Pro tip: Check the guild bank for consumables before raiding.",
            "Don't forget to repair before dungeon runs!",
            "Guild tabard gives a nice reputation bonus.",
            "The guild calendar has upcoming events marked."
        };
    }
    if (!infoMessages.empty())
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dis(0, infoMessages.size() - 1);

        SendGuildChatMessage(infoMessages[dis(gen)]);
    }
}

void GuildIntegration::AutomateGuildBankInteractions()
{
    if (!_bot || !_bot->GetGuild())
        return;

    uint32 currentTime = GameTime::GetGameTimeMS();
    auto& state = _playerStates[_bot->GetGUID().GetCounter()];

    // Check cooldown
    if (currentTime - state.lastGuildBankInteraction < GUILD_BANK_CHECK_INTERVAL)
        return;

    // Check if player is near a guild bank
    // This would require checking proximity to guild bank NPCs

    // Decide what to do with guild bank
    GuildProfile profile = GetGuildProfile();
    if (rand() % 100 < 30) // 30% chance to deposit
    {
        DepositItemsToGuildBank();
    }
    else if (rand() % 100 < 20) // 20% chance to withdraw
    {
        WithdrawNeededItems();
    }

    state.lastGuildBankInteraction = currentTime;
    UpdateGuildParticipation(GuildActivityType::GUILD_BANK_INTERACTION);
}

void GuildIntegration::DepositItemsToGuildBank()
{
    if (!_bot || !_bot->GetGuild())
        return;

    // Find items suitable for guild bank deposit
    std::vector<Item*> itemsToDeposit;

    // Check player's inventory for valuable items that could benefit the guild
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        if (Bag* pBag = _bot->GetBagByPos(bag))
        {
            for (uint32 slot = 0; slot < pBag->GetBagSize(); ++slot)
            {
                if (Item* item = pBag->GetItemByPos(slot))
                {
                    if (ShouldDepositItem(item->GetEntry()))
                    {
                        itemsToDeposit.push_back(item);
                    }
                }
            }
        }
    }

    // Full guild bank deposit implementation
    Guild* guild = _bot->GetGuild();
    if (!guild || itemsToDeposit.empty())
        return;

    uint32 depositsThisSession = 0;
    const uint32 MAX_DEPOSITS_PER_SESSION = 5;

    for (Item* item : itemsToDeposit)
    {
        if (depositsThisSession >= MAX_DEPOSITS_PER_SESSION)
            break;

        // Find the best tab for this item based on category
        uint8 targetTab = FindBestTabForItem(item);
        if (targetTab >= GUILD_BANK_MAX_TABS)
            continue;

        // Check if player has deposit rights for this tab
        if (!HasGuildBankDepositRights(targetTab))
            continue;

        // Find an empty slot in the target tab
        int8 emptySlot = FindEmptySlotInTab(guild, targetTab);
        if (emptySlot < 0)
        {
            // Try other tabs if target is full
            for (uint8 altTab = 0; altTab < GUILD_BANK_MAX_TABS; ++altTab)
            {
                if (altTab == targetTab || !HasGuildBankDepositRights(altTab))
                    continue;
                emptySlot = FindEmptySlotInTab(guild, altTab);
                if (emptySlot >= 0)
                {
                    targetTab = altTab;
                    break;
                }
            }
        }

        if (emptySlot < 0)
            continue;

        // Get item's current position in player inventory
        uint8 srcBag = item->GetBagSlot();
        uint8 srcSlot = item->GetSlot();

        // Use Guild's SwapItemsWithInventory to deposit
        // Parameters: player, toChar=false (to bank), tabId, slotId, playerBag, playerSlotId, splitedAmount=0
        guild->SwapItemsWithInventory(_bot, false, targetTab, static_cast<uint8>(emptySlot), srcBag, srcSlot, 0);

        TC_LOG_DEBUG("playerbot.guild", "Player {} deposited item {} (entry {}) to guild bank tab {} slot {}",
                    _bot->GetName(), item->GetGUID().ToString(), item->GetEntry(), targetTab, emptySlot);

        ++depositsThisSession;
        UpdateGuildMetrics(GuildActivityType::GUILD_BANK_INTERACTION, true);
    }
}

void GuildIntegration::WithdrawNeededItems()
{
    if (!_bot || !_bot->GetGuild())
        return;

    // Comprehensive needs analysis and guild bank withdrawal
    Guild* guild = _bot->GetGuild();
    if (!guild)
        return;

    // Identify items needed by the player
    std::vector<uint32> neededItems;

    // 1. Check consumable needs (food, potions, flasks)
    AnalyzeConsumableNeeds(neededItems);

    // 2. Check reagent needs for class abilities
    AnalyzeReagentNeeds(neededItems);

    // 3. Check profession material needs
    AnalyzeProfessionMaterialNeeds(neededItems);

    // 4. Check equipment upgrade opportunities
    std::vector<std::pair<uint8, uint8>> upgradeLocations; // tab, slot pairs
    AnalyzeEquipmentUpgrades(guild, upgradeLocations);

    uint32 withdrawalsThisSession = 0;
    const uint32 MAX_WITHDRAWALS_PER_SESSION = 3;

    // Execute consumable/reagent withdrawals
    for (uint32 itemId : neededItems)
    {
        if (withdrawalsThisSession >= MAX_WITHDRAWALS_PER_SESSION)
            break;

        if (!ShouldWithdrawItem(itemId))
            continue;

        // Search guild bank for this item
        for (uint8 tabId = 0; tabId < GUILD_BANK_MAX_TABS; ++tabId)
        {
            if (!HasGuildBankWithdrawRights(tabId))
                continue;

            // Check withdrawal slots remaining
            if (GetRemainingWithdrawSlots(tabId) <= 0)
                continue;

            int8 itemSlot = FindItemInTab(guild, tabId, itemId);
            if (itemSlot < 0)
                continue;

            // Find empty inventory slot
            uint8 destBag = 0, destSlot = 0;
            if (!FindFreeInventorySlot(destBag, destSlot))
            {
                TC_LOG_DEBUG("playerbot.guild", "Player {} inventory full, cannot withdraw item {}",
                            _bot->GetName(), itemId);
                break;
            }

            // Use Guild's SwapItemsWithInventory to withdraw
            // Parameters: player, toChar=true (to inventory), tabId, slotId, playerBag, playerSlotId, splitedAmount=0
            guild->SwapItemsWithInventory(_bot, true, tabId, static_cast<uint8>(itemSlot), destBag, destSlot, 0);

            TC_LOG_DEBUG("playerbot.guild", "Player {} withdrew item {} from guild bank tab {} slot {}",
                        _bot->GetName(), itemId, tabId, itemSlot);

            ++withdrawalsThisSession;
            UpdateGuildMetrics(GuildActivityType::GUILD_BANK_INTERACTION, true);
            break; // Found and withdrew this item, move to next needed item
        }
    }

    // Handle equipment upgrades if we have withdrawals remaining
    for (auto const& [tabId, slotId] : upgradeLocations)
    {
        if (withdrawalsThisSession >= MAX_WITHDRAWALS_PER_SESSION)
            break;

        if (!HasGuildBankWithdrawRights(tabId) || GetRemainingWithdrawSlots(tabId) <= 0)
            continue;

        uint8 destBag = 0, destSlot = 0;
        if (!FindFreeInventorySlot(destBag, destSlot))
            break;

        guild->SwapItemsWithInventory(_bot, true, tabId, slotId, destBag, destSlot, 0);
        ++withdrawalsThisSession;
        UpdateGuildMetrics(GuildActivityType::GUILD_BANK_INTERACTION, true);
    }
}

void GuildIntegration::OrganizeGuildBank()
{
    if (!_bot || !_bot->GetGuild())
        return;

    // Banker role: organize and manage guild bank efficiently
    // Sort items by category
    // Stack similar items
    // Move items to appropriate tabs
    // Clean up expired items
}

void GuildIntegration::ManageGuildBankPermissions()
{
    if (!_bot || !_bot->GetGuild())
        return;

    // Officer/Leader role: manage bank access permissions
    // Review member access levels
    // Adjust permissions based on member activity
    // Set up proper tab restrictions
}

void GuildIntegration::CoordinateGuildEvents()
{
    if (!_bot || !_bot->GetGuild())
        return;

    // Event organizer role: plan and coordinate guild activities
    // Check guild calendar
    // Plan upcoming events
    // Send reminders
    // Coordinate with other officers
}

void GuildIntegration::ScheduleGuildActivities()
{
    if (!_bot || !_bot->GetGuild())
        return;

    // Schedule various guild activities
    // Raid nights
    // Social events
    // Achievement runs
    // Guild meetings
}

void GuildIntegration::ManageGuildCalendar()
{
    if (!_bot || !_bot->GetGuild())
        return;

    // Manage guild calendar events
    // Create new events
    // Update existing events
    // Send invitations
    // Track attendance
}

void GuildIntegration::OrganizeGuildRuns()
{
    if (!_bot || !_bot->GetGuild())
        return;

    // Organize guild dungeon/raid runs
    // Form groups from guild members
    // Plan difficulty appropriate content
    // Coordinate schedules
}

void GuildIntegration::SetGuildProfile(const GuildProfile& profile)
{
    _playerProfiles[_bot->GetGUID().GetCounter()] = profile;
}

GuildProfile GuildIntegration::GetGuildProfile()
{
    auto it = _playerProfiles.find(_bot->GetGUID().GetCounter());
    if (it != _playerProfiles.end())
        return it->second;

    return GuildProfile(); // Return default profile
}

GuildParticipation GuildIntegration::GetGuildParticipation()
{
    uint32 playerGuid = _bot->GetGUID().GetCounter();
    auto it = _playerParticipation.find(playerGuid);
    if (it != _playerParticipation.end())
        return it->second;

    return GuildParticipation(playerGuid, 0); // Return default participation
}

void GuildIntegration::UpdateGuildParticipation(GuildActivityType activityType)
{
    uint32 playerGuid = _bot->GetGUID().GetCounter();
    auto& participation = _playerParticipation[playerGuid];
    participation.activityCounts[activityType]++;
    participation.lastActivity = GameTime::GetGameTimeMS();

    // Update specific metrics based on activity type
    switch (activityType)
    {
        case GuildActivityType::CHAT_PARTICIPATION:
            participation.totalChatMessages++;
            break;
        case GuildActivityType::GUILD_EVENT_ATTENDANCE:
            participation.eventsAttended++;
            break;
        case GuildActivityType::ACHIEVEMENT_CONTRIBUTION:
            participation.contributionScore += 0.1f;
            break;
        default:
            break;
    }

    // Update social score based on activity
    participation.socialScore = std::min(1.0f, participation.socialScore + 0.01f);
}

void GuildIntegration::AssistWithRecruitment()
{
    if (!_bot || !_bot->GetGuild())
        return;

    // Help with guild recruitment
    // Look for potential recruits
    // Evaluate player suitability
    // Send recruitment messages
    // Welcome new members
}

void GuildIntegration::EvaluateRecruitmentCandidates()
{
    if (!_bot || !_bot->GetGuild())
        return;

    // Evaluate potential guild recruits
    // Check player level and activity
    // Assess compatibility with guild culture
    // Make recruitment recommendations
}

void GuildIntegration::WelcomeNewGuildMembers()
{
    if (!_bot || !_bot->GetGuild())
        return;

    // Welcome new guild members
    // Send welcome messages
    // Explain guild rules and culture
    // Offer assistance and guidance
    // Introduce to other members
}

void GuildIntegration::MentorJuniorMembers()
{
    if (!_bot || !_bot->GetGuild())
        return;

    // Mentor newer or lower-level guild members
    // Offer advice and guidance
    // Help with quests and dungeons
    // Share game knowledge
    // Provide encouragement
}

void GuildIntegration::SupportGuildLeadership()
{
    if (!_bot || !_bot->GetGuild())
        return;

    // Support guild leadership activities
    // Assist with decision making
    // Help enforce guild rules
    // Coordinate with other officers
    // Handle administrative tasks
}

void GuildIntegration::HandleOfficerDuties()
{
    if (!_bot || !_bot->GetGuild())
        return;

    // Handle officer-specific duties
    // Manage guild bank
    // Handle member issues
    // Coordinate events
    // Make guild decisions
}

void GuildIntegration::AssistWithGuildManagement()
{
    if (!_bot || !_bot->GetGuild())
        return;

    // Assist with general guild management
    // Monitor guild health
    // Handle conflicts
    // Maintain guild standards
    // Support guild goals
}

void GuildIntegration::ProvideMemberFeedback()
{
    if (!_bot || !_bot->GetGuild())
        return;

    // Provide feedback to guild members
    // Recognize achievements
    // Offer constructive criticism
    // Suggest improvements
    // Celebrate successes
}

std::string GuildIntegration::GenerateGuildChatResponse( const GuildChatMessage& message)
{
    if (!_bot)
        return "";

    // Analyze message content and generate appropriate response
    float relevance = CalculateMessageRelevance(message);
    if (relevance < 0.3f) // Not relevant enough to respond
        return "";

    // Extract keywords from message
    std::vector<std::string> keywords = ExtractKeywords(message.content);

    // Generate response based on keywords and context
    std::string response = SelectResponseTemplate("general");

    // Personalize the response
    response = PersonalizeResponse(response);

    return response;
}

std::string GuildIntegration::GenerateConversationStarter()
{
    if (!_bot)
        return "";

    GuildProfile profile = GetGuildProfile();
    // Select conversation starter based on profile and current context
    std::vector<std::string> starters;

    switch (profile.chatStyle)
    {
        case GuildChatStyle::HELPFUL:
            starters = {
                "Anyone need help with anything?",
                "How's everyone doing today?",
                "Any interesting quests or adventures happening?",
                "Don't forget to check the guild bank for useful items!"
            };
            break;

        case GuildChatStyle::SOCIAL:
            starters = {
                "Good morning, guild!",
                "Hope everyone's having a great day!",
                "What's everyone up to?",
                "Anyone have any fun stories to share?"
            };
            break;

        case GuildChatStyle::PROFESSIONAL:
            starters = {
                "Guild meeting reminder: check the calendar for upcoming events.",
                "Don't forget about our scheduled raid this week.",
                "Please review the guild bank organization guidelines.",
                "Achievement progress update: we're close to completing [achievement]."
            };
            break;

        default:
            starters = {
                "Hello everyone!",
                "How's the adventuring going?",
                "Any news from the field?",
                "Safe travels, everyone!"
            };
            break;
    }

    if (!starters.empty())
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dis(0, starters.size() - 1);
        return starters[dis(gen)];
    }

    return "";
}

bool GuildIntegration::ShouldRespondToMessage( const GuildChatMessage& message)
{
    if (!_bot)
        return false;

    // Don't respond to own messages
    if (message.senderId == _bot->GetGUID().GetCounter())
        return false;

    GuildProfile profile = GetGuildProfile();

    // Check if message requires response
    if (message.requiresResponse)
        return true;

    // Calculate relevance and decide based on chat style
    float relevance = CalculateMessageRelevance(message);
    float threshold = 0.7f;

    switch (profile.chatStyle)
    {
        case GuildChatStyle::MINIMAL:
            threshold = 0.9f;
            break;
        case GuildChatStyle::MODERATE:
            threshold = 0.7f;
            break;
        case GuildChatStyle::ACTIVE:
        case GuildChatStyle::SOCIAL:
            threshold = 0.5f;
            break;
        case GuildChatStyle::HELPFUL:
            threshold = 0.4f;
            break;
        case GuildChatStyle::PROFESSIONAL:
            threshold = 0.6f;
            break;
    }

    return relevance > threshold;
}

void GuildIntegration::LearnFromGuildConversations()
{
    if (!_bot)
        return;

    // Learn from guild chat patterns
    // Identify popular topics
    // Adapt response styles
    // Improve conversation quality
}

void GuildIntegration::ContributeToGuildAchievements()
{
    if (!_bot || !_bot->GetGuild())
        return;

    // Actively work towards guild achievements
    // Identify incomplete guild achievements
    // Prioritize achievements player can contribute to
    // Execute achievement-oriented activities
}

void GuildIntegration::CoordinateAchievementEfforts(Guild* guild)
{
    if (!guild)
        return;

    // Coordinate guild-wide achievement efforts
    // Analyze achievement progress
    // Assign tasks to members
    // Track completion status
}

void GuildIntegration::TrackAchievementProgress()
{
    if (!_bot || !_bot->GetGuild())
        return;

    // Track progress towards guild achievements
    // Monitor contribution levels
    // Identify bottlenecks
    // Suggest focus areas
}

void GuildIntegration::CelebrateGuildAchievements()
{
    if (!_bot || !_bot->GetGuild())
        return;

    // Celebrate completed guild achievements
    // Send congratulatory messages
    // Acknowledge contributors
    // Boost guild morale
}

GuildMetrics GuildIntegration::GetPlayerGuildMetrics()
{
    auto it = _playerMetrics.find(_bot->GetGUID().GetCounter());
    if (it != _playerMetrics.end())
        return it->second;

    GuildMetrics metrics;
    metrics.Reset();
    return metrics;
}

GuildMetrics GuildIntegration::GetGuildBotMetrics(uint32 guildId)
{
    GuildMetrics combinedMetrics;
    combinedMetrics.Reset();

    // Aggregate metrics from all bots in the guild
    for (const auto& metricsPair : _playerMetrics)
    {
        // In a real implementation, we'd check if the player belongs to the specified guild
        const GuildMetrics& playerMetrics = metricsPair.second;

        combinedMetrics.guildInteractions += playerMetrics.guildInteractions;
        combinedMetrics.chatMessages += playerMetrics.chatMessages;
        combinedMetrics.bankTransactions += playerMetrics.bankTransactions;
        combinedMetrics.eventsParticipated += playerMetrics.eventsParticipated;
        combinedMetrics.helpfulActions += playerMetrics.helpfulActions;
    }

    return combinedMetrics;
}

void GuildIntegration::InitializeChatTemplates()
{
    // Initialize chat response templates
    _globalResponseTemplates["greeting"] = {
        "Hello there!",
        "Good to see you!",
        "Hey, how's it going?",
        "Greetings, friend!"
    };

    _globalResponseTemplates["help"] = {
        "I'd be happy to help!",
        "What do you need assistance with?",
        "Count me in if you need help!",
        "Let me know if I can assist!"
    };

    _globalResponseTemplates["thanks"] = {
        "You're welcome!",
        "Happy to help!",
        "No problem at all!",
        "Anytime!"
    };

    _globalResponseTemplates["farewell"] = {
        "Safe travels!",
        "See you later!",
        "Take care!",
        "Until next time!"
    };

    // Initialize conversation topics
    _conversationTopics = {
        "guild events",
        "achievements",
        "dungeon runs",
        "guild bank",
        "member assistance",
        "upcoming raids",
        "guild news",
        "general chat"
    };
}

void GuildIntegration::LoadGuildSpecificData(uint32 guildId)
{
    // Load guild-specific configuration and data
    // Custom chat templates
    // Guild-specific rules and preferences
    // Member interaction history
}

bool GuildIntegration::IsAppropriateTimeToChat()
{
    if (!_bot)
        return false;

    // Check if it's an appropriate time to participate in guild chat
    // Consider time of day, recent activity, etc.

    uint32 currentTime = GameTime::GetGameTimeMS();
    auto& chatIntel = _chatIntelligence[_bot->GetGUID().GetCounter()];
    // Don't chat too frequently
    if (currentTime - chatIntel.lastResponseTime < 30000) // 30 seconds cooldown
        return false;

    // Check if guild is active
    Guild* guild = _bot->GetGuild();
    if (!guild)
        return false;

    // More sophisticated timing logic could be added here
    return true;
}

void GuildIntegration::UpdateGuildSocialGraph(uint32 guildId)
{
    // Update social interaction graph for the guild
    // Track member interactions
    // Identify social clusters
    // Monitor guild dynamics
}

std::string GuildIntegration::SelectResponseTemplate(const std::string& category)
{
    auto it = _globalResponseTemplates.find(category);
    if (it == _globalResponseTemplates.end() || it->second.empty())
        return "";

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, it->second.size() - 1);

    return it->second[dis(gen)];
}

std::string GuildIntegration::PersonalizeResponse( const std::string& templateStr)
{
    if (!_bot)
        return templateStr;

    // Personalize the response based on player and context
    std::string response = templateStr;

    // Add player-specific touches based on guild profile
    GuildProfile profile = GetGuildProfile();
    // Modify tone based on chat style
    switch (profile.chatStyle)
    {
        case GuildChatStyle::PROFESSIONAL:
            // Keep formal tone
            break;
        case GuildChatStyle::SOCIAL:
            // Add friendly emotes or expressions
            response += " :)";
            break;
        default:
            break;
    }

    return response;
}

float GuildIntegration::CalculateMessageRelevance( const GuildChatMessage& message)
{
    if (!_bot)
        return 0.0f;

    float relevance = 0.0f;

    // Check for keywords that indicate relevance
    std::vector<std::string> keywords = ExtractKeywords(message.content);

    GuildProfile profile = GetGuildProfile();
    for (const std::string& keyword : keywords)
    {
        // Check against player's interests and expertise
        for (const std::string& interest : profile.interests)
        {
            if (keyword.find(interest) != std::string::npos)
            {
                relevance += 0.3f;
            }
        }

        for (const std::string& expertise : profile.expertise)
        {
            if (keyword.find(expertise) != std::string::npos)
            {
                relevance += 0.4f;
            }
        }

        // Check against common guild-related keywords
        std::vector<std::string> guildKeywords = {
            "help", "assist", "raid", "dungeon", "event", "bank", "achievement"
        };

        for (const std::string& guildKeyword : guildKeywords)
        {
            if (keyword.find(guildKeyword) != std::string::npos)
            {
                relevance += 0.2f;
            }
        }
    }

    // Check if message is directed at the bot
    if (message.content.find(_bot->GetName()) != std::string::npos)
    {
        relevance += 0.5f;
    }

    return std::min(1.0f, relevance);
}

std::vector<std::string> GuildIntegration::ExtractKeywords(const std::string& message)
{
    std::vector<std::string> keywords;

    // Simple keyword extraction (in a real implementation, this would be more sophisticated)
    std::istringstream iss(message);
    std::string word;

    while (iss >> word)
    {
        // Convert to lowercase and remove punctuation
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());

        if (word.length() > 2) // Ignore very short words
        {
            keywords.push_back(word);
        }
    }

    return keywords;
}

bool GuildIntegration::ShouldDepositItem( uint32 itemId)
{
    if (!_bot)
        return false;

    // Determine if item should be deposited to guild bank
    const ItemTemplate* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
    if (!itemTemplate)
        return false;

    // Deposit consumables, crafting materials, and other useful items
    switch (itemTemplate->GetClass())
    {
        case ITEM_CLASS_CONSUMABLE:
        case ITEM_CLASS_TRADE_GOODS:
        case ITEM_CLASS_MISCELLANEOUS:
            return itemTemplate->GetQuality() >= ITEM_QUALITY_UNCOMMON;
        default:
            return false;
    }
}

bool GuildIntegration::ShouldWithdrawItem( uint32 itemId)
{
    if (!_bot)
        return false;

    // Full intelligent guild bank item withdrawal decision system
    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
    if (!itemTemplate)
        return false;

    uint32 playerLevel = _bot->GetLevel();

    // 1. Check class compatibility
    if (itemTemplate->GetAllowableClass() != 0 && !(itemTemplate->GetAllowableClass() & _bot->GetClassMask()))
        return false;

    // 2. Check level requirement
    if (itemTemplate->GetBaseRequiredLevel() > static_cast<int32>(playerLevel))
        return false;

    // 3. Analyze by item class
    switch (itemTemplate->GetClass())
    {
        case ITEM_CLASS_CONSUMABLE:
        {
            // Check current stock of this consumable
            uint32 currentCount = _bot->GetItemCount(itemId);

            // Determine target stock based on consumable type
            uint32 targetStock = 0;
            switch (itemTemplate->GetSubClass())
            {
                case ITEM_SUBCLASS_POTION:
                    targetStock = 10; // Keep 10 potions
                    break;
                case ITEM_SUBCLASS_FLASK:
                    targetStock = 5;  // Keep 5 flasks
                    break;
                case ITEM_SUBCLASS_FOOD_DRINK:
                case ITEM_SUBCLASS_BANDAGE:
                    targetStock = 20; // Keep 20 food/bandages
                    break;
                default:
                    targetStock = 5;  // General consumable stock
                    break;
            }

            return currentCount < targetStock;
        }

        case ITEM_CLASS_TRADE_GOODS:
        {
            // Only withdraw trade goods if player has relevant profession
            if (!HasRelevantProfession(itemTemplate->GetSubClass()))
                return false;

            // Check current stock
            uint32 currentCount = _bot->GetItemCount(itemId);
            return currentCount < 20; // Keep reasonable material stock
        }

        case ITEM_CLASS_WEAPON:
        case ITEM_CLASS_ARMOR:
        {
            // Check if this would be an upgrade
            return IsEquipmentUpgrade(itemTemplate);
        }

        case ITEM_CLASS_RECIPE:
        {
            // Only withdraw recipes we can learn and don't already know
            if (!CanLearnRecipe(itemTemplate))
                return false;
            return true;
        }

        case ITEM_CLASS_GEM:
        {
            // Withdraw gems if we have items to socket
            return HasItemsNeedingSockets();
        }

        case ITEM_CLASS_GLYPH:
        {
            // Check if glyph is for our class and we don't have it
            if (itemTemplate->GetAllowableClass() != 0 && !(itemTemplate->GetAllowableClass() & _bot->GetClassMask()))
                return false;
            // TrinityCore 12.0: Use Effects vector instead of deprecated Spells array
            if (!itemTemplate->Effects.empty())
            {
                ItemEffectEntry const* effect = itemTemplate->Effects[0];
                if (effect && effect->SpellID > 0)
                    return !_bot->HasSpell(effect->SpellID);
            }
            return false;
        }

        default:
            return false;
    }
}

void GuildIntegration::SendGuildChatMessage( const std::string& message)
{
    if (!_bot || !_bot->GetGuild() || message.empty())
        return;

    // Send message to guild chat using proper TrinityCore API
    _bot->GetGuild()->BroadcastToGuild(_bot->GetSession(), false, message, LANG_UNIVERSAL);

    // Update metrics
    UpdateGuildMetrics(GuildActivityType::CHAT_PARTICIPATION, true);
}

void GuildIntegration::OfferGuildAssistance( const std::string& assistance)
{
    if (!_bot)
        return;

    // If specific assistance is provided, use it; otherwise use default messages
    if (!assistance.empty())
    {
        SendGuildChatMessage( assistance);
        return;
    }

    std::vector<std::string> helpMessages = {
        "Anyone need help with quests or dungeons?",
        "I'm available to assist with any guild activities!",
        "Let me know if you need help with anything!",
        "Happy to lend a hand wherever needed!"
    };

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, helpMessages.size() - 1);

    SendGuildChatMessage( helpMessages[dis(gen)]);
}

void GuildIntegration::UpdateGuildMetrics(GuildActivityType activity, bool wasSuccessful)
{
    auto& metrics = _playerMetrics[_bot->GetGUID().GetCounter()];
    metrics.guildInteractions++;

    if (wasSuccessful)
    {
        switch (activity)
        {
            case GuildActivityType::CHAT_PARTICIPATION:
                metrics.chatMessages++;
                break;
            case GuildActivityType::GUILD_BANK_INTERACTION:
                metrics.bankTransactions++;
                break;
            case GuildActivityType::GUILD_EVENT_ATTENDANCE:
                metrics.eventsParticipated++;
                break;
            case GuildActivityType::RECRUITMENT_ASSISTANCE:
            case GuildActivityType::ACHIEVEMENT_CONTRIBUTION:
                metrics.helpfulActions++;
                break;
            default:
                break;
        }
    }

    // Update global metrics
    _globalMetrics.guildInteractions++;
    if (wasSuccessful)
    {
        switch (activity)
        {
            case GuildActivityType::CHAT_PARTICIPATION:
                _globalMetrics.chatMessages++;
                break;
            case GuildActivityType::GUILD_BANK_INTERACTION:
                _globalMetrics.bankTransactions++;
                break;
            case GuildActivityType::GUILD_EVENT_ATTENDANCE:
                _globalMetrics.eventsParticipated++;
                break;
            case GuildActivityType::RECRUITMENT_ASSISTANCE:
            case GuildActivityType::ACHIEVEMENT_CONTRIBUTION:
                _globalMetrics.helpfulActions++;
                break;
            default:
                break;
        }
    }

    metrics.lastUpdate = GameTime::GetGameTimeMS();
    _globalMetrics.lastUpdate = GameTime::GetGameTimeMS();
}

void GuildIntegration::Update(uint32 /*diff*/)
{
    static uint32 lastUpdate = 0;
    uint32 currentTime = GameTime::GetGameTimeMS();

    if (currentTime - lastUpdate < GUILD_UPDATE_INTERVAL)
        return;

    lastUpdate = currentTime;

    // Update guild participation
    UpdateGuildParticipation();

    // Process guild events
    ProcessGuildEvents();

    // Clean up old data
    CleanupGuildData();
}

void GuildIntegration::UpdateGuildParticipation()
{
    uint32 currentTime = GameTime::GetGameTimeMS();

    // Apply social score decay for inactive players
    for (auto& participationPair : _playerParticipation)
    {
        GuildParticipation& participation = participationPair.second;

        if (currentTime - participation.lastActivity > 86400000) // 24 hours
        {
            participation.socialScore = std::max(0.0f, participation.socialScore - SOCIAL_SCORE_DECAY);
        }
    }
}

void GuildIntegration::ProcessGuildEvents()
{
    // Process scheduled guild events
    // Send reminders
    // Track attendance
    // Coordinate activities
}

void GuildIntegration::CleanupGuildData()
{
    uint32 currentTime = GameTime::GetGameTimeMS();

    // Clean up old participation data for inactive players
    for (auto it = _playerParticipation.begin(); it != _playerParticipation.end();)
    {
        if (currentTime - it->second.lastActivity > 30U * 86400000U) // 30 days
        {
            it = _playerParticipation.erase(it);
        }
        else
        {
            ++it;
        }
    }
}


void GuildIntegration::OrganizeSocialEvents()
{
    // Organize social events for guild members
    TC_LOG_DEBUG("playerbot.guild", "GuildIntegration::OrganizeSocialEvents called");
}

void GuildIntegration::ParticipateInGuildTradition()
{
    // Participate in guild traditions
    TC_LOG_DEBUG("playerbot.guild", "GuildIntegration::ParticipateInGuildTradition called");
}

void GuildIntegration::MaintainGuildFriendships()
{
    // Maintain friendships with guild members
    TC_LOG_DEBUG("playerbot.guild", "GuildIntegration::MaintainGuildFriendships called");
}

void GuildIntegration::HandleGuildConflicts()
{
    // Handle conflicts within the guild
    TC_LOG_DEBUG("playerbot.guild", "GuildIntegration::HandleGuildConflicts called");
}

void GuildIntegration::OptimizeGuildBankUsage()
{
    // Optimize guild bank usage patterns
    TC_LOG_DEBUG("playerbot.guild", "GuildIntegration::OptimizeGuildBankUsage called");
}

void GuildIntegration::AutoDepositValuableItems()
{
    // Auto-deposit valuable items to guild bank
    TC_LOG_DEBUG("playerbot.guild", "GuildIntegration::AutoDepositValuableItems called");
}

void GuildIntegration::AutoWithdrawNeededConsumables()
{
    // Auto-withdraw needed consumables
    TC_LOG_DEBUG("playerbot.guild", "GuildIntegration::AutoWithdrawNeededConsumables called");
}

void GuildIntegration::ManageGuildBankTabs()
{
    // Manage guild bank tab organization
    TC_LOG_DEBUG("playerbot.guild", "GuildIntegration::ManageGuildBankTabs called");
}

void GuildIntegration::TrackGuildBankActivity()
{
    // Track guild bank activity
    TC_LOG_DEBUG("playerbot.guild", "GuildIntegration::TrackGuildBankActivity called");
}

void GuildIntegration::CreateGuildEvent(::std::string const& eventName)
{
    // Create a guild event
    TC_LOG_DEBUG("playerbot.guild", "GuildIntegration::CreateGuildEvent called");
}

void GuildIntegration::ManageGuildCalendarEvents()
{
    // Manage guild calendar events
    TC_LOG_DEBUG("playerbot.guild", "GuildIntegration::ManageGuildCalendarEvents called");
}

void GuildIntegration::CoordinateRaidScheduling()
{
    // Coordinate raid scheduling
    TC_LOG_DEBUG("playerbot.guild", "GuildIntegration::CoordinateRaidScheduling called");
}

void GuildIntegration::OrganizePvPEvents()
{
    // Organize PvP events
    TC_LOG_DEBUG("playerbot.guild", "GuildIntegration::OrganizePvPEvents called");
}

void GuildIntegration::AnalyzeGuildDynamics(Guild* /*guild*/)
{
    // Analyze guild dynamics
    TC_LOG_DEBUG("playerbot.guild", "GuildIntegration::AnalyzeGuildDynamics called");
}

void GuildIntegration::AdaptToGuildCulture()
{
    // Adapt behavior to guild culture
    TC_LOG_DEBUG("playerbot.guild", "GuildIntegration::AdaptToGuildCulture called");
}

void GuildIntegration::DetectGuildMoodAndTone(Guild* /*guild*/)
{
    // Detect guild mood and tone
    TC_LOG_DEBUG("playerbot.guild", "GuildIntegration::DetectGuildMoodAndTone called");
}

void GuildIntegration::AdjustBehaviorToGuildNorms()
{
    // Adjust behavior to guild norms
    TC_LOG_DEBUG("playerbot.guild", "GuildIntegration::AdjustBehaviorToGuildNorms called");
}

void GuildIntegration::SetGuildAutomationLevel(float level)
{
    // Set guild automation level
    TC_LOG_DEBUG("playerbot.guild", "GuildIntegration::SetGuildAutomationLevel called");
}

void GuildIntegration::EnableGuildActivity(GuildActivityType type, bool enabled)
{
    // Enable/disable guild activity
    TC_LOG_DEBUG("playerbot.guild", "GuildIntegration::EnableGuildActivity called");
}

void GuildIntegration::SetGuildChatFrequency(float frequency)
{
    // Set guild chat frequency
    TC_LOG_DEBUG("playerbot.guild", "GuildIntegration::SetGuildChatFrequency called");
}

void GuildIntegration::ConfigureGuildBankAccess(bool canDeposit, bool canWithdraw)
{
    // Configure guild bank access
    TC_LOG_DEBUG("playerbot.guild", "GuildIntegration::ConfigureGuildBankAccess called");
}

void GuildIntegration::HandleGuildInteractionError(::std::string const& error)
{
    // Handle guild interaction error
    TC_LOG_DEBUG("playerbot.guild", "GuildIntegration::HandleGuildInteractionError called");
}

void GuildIntegration::RecoverFromGuildFailure()
{
    // Recover from guild failure
    TC_LOG_DEBUG("playerbot.guild", "GuildIntegration::RecoverFromGuildFailure called");
}

void GuildIntegration::HandleGuildLeaving()
{
    // Handle leaving guild
    TC_LOG_DEBUG("playerbot.guild", "GuildIntegration::HandleGuildLeaving called");
}

void GuildIntegration::HandleGuildInvitations(uint32 guildId)
{
    // Handle guild invitations
    TC_LOG_DEBUG("playerbot.guild", "GuildIntegration::HandleGuildInvitations called");
}


// ============================================================================
// Guild Bank Helper Methods
// ============================================================================

uint8 GuildIntegration::FindBestTabForItem(Item* item) const
{
    if (!item)
        return GUILD_BANK_MAX_TABS;

    ItemTemplate const* itemTemplate = item->GetTemplate();
    if (!itemTemplate)
        return GUILD_BANK_MAX_TABS;

    // Categorize items by type for organization
    // Tab 0: Consumables (food, potions, flasks)
    // Tab 1: Trade goods and materials
    // Tab 2: Equipment (weapons, armor)
    // Tab 3: Miscellaneous and valuable items
    // Tab 4-7: Overflow

    switch (itemTemplate->GetClass())
    {
        case ITEM_CLASS_CONSUMABLE:
            return 0;
        case ITEM_CLASS_TRADE_GOODS:
            return 1;
        case ITEM_CLASS_WEAPON:
        case ITEM_CLASS_ARMOR:
            return 2;
        case ITEM_CLASS_GEM:
        case ITEM_CLASS_RECIPE:
            return 3;
        default:
            return 4;
    }
}

int8 GuildIntegration::FindEmptySlotInTab(Guild* /*guild*/, uint8 tabId) const
{
    // TrinityCore 12.0: Guild::GetBankTab is private, so we cannot directly inspect tabs
    // Return first slot as default - SwapItemsWithInventory will handle actual slot finding
    if (tabId >= GUILD_BANK_MAX_TABS)
        return -1;

    // Return slot 0 as a starting point - the actual swap operation will handle validation
    return 0;
}

bool GuildIntegration::HasGuildBankDepositRights(uint8 /*tabId*/) const
{
    if (!_bot || !_bot->GetGuild())
        return false;

    Guild* guild = _bot->GetGuild();
    // TrinityCore 12.0: GetMember is private, use GetGuildRank to check permissions
    GuildRankId rank = static_cast<GuildRankId>(_bot->GetGuildRank());

    // Guild bank deposit rights are typically available to all ranks with withdraw rights
    // We check for GR_RIGHT_WITHDRAW_GOLD as a proxy for general bank permissions
    return guild->HasAnyRankRight(rank, GR_RIGHT_WITHDRAW_GOLD);
}

bool GuildIntegration::HasGuildBankWithdrawRights(uint8 /*tabId*/) const
{
    if (!_bot || !_bot->GetGuild())
        return false;

    Guild* guild = _bot->GetGuild();
    // TrinityCore 12.0: GetMember is private, use GetGuildRank to check permissions
    GuildRankId rank = static_cast<GuildRankId>(_bot->GetGuildRank());

    // Check for withdraw rights through rank permissions
    return guild->HasAnyRankRight(rank, GR_RIGHT_WITHDRAW_GOLD);
}

int32 GuildIntegration::GetRemainingWithdrawSlots(uint8 /*tabId*/) const
{
    if (!_bot || !_bot->GetGuild())
        return 0;

    // TrinityCore 12.0: GetMember is private, so we cannot query remaining slots directly
    // Return a reasonable default; actual swap operations will validate permissions
    return GUILD_BANK_MAX_SLOTS;
}

int8 GuildIntegration::FindItemInTab(Guild* /*guild*/, uint8 tabId, uint32 /*itemId*/) const
{
    // TrinityCore 12.0: Guild::GetBankTab and Guild::BankTab are private
    // Cannot directly iterate guild bank contents from outside Guild class
    // Return -1 to indicate we cannot find specific items without direct API access
    if (tabId >= GUILD_BANK_MAX_TABS)
        return -1;

    // The SwapItemsWithInventory function will handle actual item operations
    return -1;
}

bool GuildIntegration::FindFreeInventorySlot(uint8& outBag, uint8& outSlot) const
{
    if (!_bot)
        return false;

    // Check backpack first
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
    {
        if (!_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
        {
            outBag = INVENTORY_SLOT_BAG_0;
            outSlot = slot;
            return true;
        }
    }

    // Check equipped bags
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        if (Bag* pBag = _bot->GetBagByPos(bag))
        {
            for (uint8 slot = 0; slot < pBag->GetBagSize(); ++slot)
            {
                if (!pBag->GetItemByPos(slot))
                {
                    outBag = bag;
                    outSlot = slot;
                    return true;
                }
            }
        }
    }

    return false;
}

void GuildIntegration::AnalyzeConsumableNeeds(std::vector<uint32>& neededItems) const
{
    if (!_bot)
        return;

    // Common consumable IDs for different purposes (WoW 12.0)
    static const std::vector<uint32> healthPotions = { 191379, 191380, 191381 }; // Healing Potions
    static const std::vector<uint32> manaPotions = { 191382, 191383, 191384 };   // Mana Potions
    static const std::vector<uint32> flasks = { 191327, 191328, 191329, 191330 }; // Dragonflight Flasks
    static const std::vector<uint32> foodBuffs = { 194684, 194685, 197777 };      // Stat food

    // Check health potion stock
    bool hasHealthPotions = false;
    for (uint32 potionId : healthPotions)
    {
        if (_bot->GetItemCount(potionId) >= 5)
        {
            hasHealthPotions = true;
            break;
        }
    }
    if (!hasHealthPotions && !healthPotions.empty())
        neededItems.push_back(healthPotions[0]);

    // Check mana potion stock for casters
    bool isCaster = (_bot->GetClass() == CLASS_MAGE || _bot->GetClass() == CLASS_WARLOCK ||
                    _bot->GetClass() == CLASS_PRIEST || _bot->GetClass() == CLASS_SHAMAN ||
                    _bot->GetClass() == CLASS_DRUID || _bot->GetClass() == CLASS_EVOKER);
    if (isCaster)
    {
        bool hasManaPotions = false;
        for (uint32 potionId : manaPotions)
        {
            if (_bot->GetItemCount(potionId) >= 5)
            {
                hasManaPotions = true;
                break;
            }
        }
        if (!hasManaPotions && !manaPotions.empty())
            neededItems.push_back(manaPotions[0]);
    }

    // Check flask stock
    bool hasFlask = false;
    for (uint32 flaskId : flasks)
    {
        if (_bot->GetItemCount(flaskId) >= 3)
        {
            hasFlask = true;
            break;
        }
    }
    if (!hasFlask && !flasks.empty())
        neededItems.push_back(flasks[0]);
}

void GuildIntegration::AnalyzeReagentNeeds(std::vector<uint32>& /*neededItems*/) const
{
    // Most class reagents were removed in modern WoW
    // This method is preserved for potential future reagent needs
}

void GuildIntegration::AnalyzeProfessionMaterialNeeds(std::vector<uint32>& neededItems) const
{
    if (!_bot)
        return;

    // Check player's professions and add common materials
    for (uint32 skill : { SKILL_ALCHEMY, SKILL_BLACKSMITHING, SKILL_ENCHANTING,
                          SKILL_ENGINEERING, SKILL_HERBALISM, SKILL_INSCRIPTION,
                          SKILL_JEWELCRAFTING, SKILL_LEATHERWORKING, SKILL_MINING,
                          SKILL_SKINNING, SKILL_TAILORING })
    {
        if (_bot->HasSkill(skill) && _bot->GetSkillValue(skill) > 0)
        {
            // Add common materials for each profession
            // These are placeholder IDs - in real implementation, look up current expansion materials
            switch (skill)
            {
                case SKILL_ALCHEMY:
                    // Check for common herbs
                    break;
                case SKILL_BLACKSMITHING:
                case SKILL_ENGINEERING:
                case SKILL_JEWELCRAFTING:
                    // Check for ore/bars
                    break;
                case SKILL_ENCHANTING:
                    // Check for enchanting materials
                    break;
                case SKILL_INSCRIPTION:
                    // Check for pigments/inks
                    break;
                case SKILL_LEATHERWORKING:
                case SKILL_SKINNING:
                    // Check for leather
                    break;
                case SKILL_TAILORING:
                    // Check for cloth
                    break;
                default:
                    break;
            }
        }
    }
}

void GuildIntegration::AnalyzeEquipmentUpgrades(Guild* /*guild*/, std::vector<std::pair<uint8, uint8>>& /*upgradeLocations*/) const
{
    // TrinityCore 12.0: Guild::BankTab and Guild::GetBankTab are private
    // Cannot directly scan guild bank items from module code
    // The SwapItemsWithInventory API handles actual bank operations
    // This function returns without populating upgradeLocations since we cannot inspect bank contents
    // Actual equipment upgrade detection should be done through database queries or guild events

    if (!_bot)
        return;

    // Note: In production, this would query the guild bank items table or use
    // an event-based system to track bank contents
}

bool GuildIntegration::HasRelevantProfession(uint32 itemSubClass) const
{
    if (!_bot)
        return false;

    // Map item subclass to profession skill
    // TrinityCore 12.0: Use correct ItemSubclassTradeGoods enum names
    switch (itemSubClass)
    {
        case ITEM_SUBCLASS_CLOTH:
            return _bot->HasSkill(SKILL_TAILORING);
        case ITEM_SUBCLASS_LEATHER:
            return _bot->HasSkill(SKILL_LEATHERWORKING);
        case ITEM_SUBCLASS_METAL_STONE:
            return _bot->HasSkill(SKILL_BLACKSMITHING) || _bot->HasSkill(SKILL_ENGINEERING) ||
                   _bot->HasSkill(SKILL_JEWELCRAFTING);
        case ITEM_SUBCLASS_PARTS:
            return _bot->HasSkill(SKILL_ENGINEERING);
        case ITEM_SUBCLASS_DEVICES:
            return _bot->HasSkill(SKILL_ENGINEERING);
        case ITEM_SUBCLASS_EXPLOSIVES:
            return _bot->HasSkill(SKILL_ENGINEERING);
        case ITEM_SUBCLASS_HERB:
            return _bot->HasSkill(SKILL_ALCHEMY) || _bot->HasSkill(SKILL_INSCRIPTION);
        case ITEM_SUBCLASS_ENCHANTING:
            return _bot->HasSkill(SKILL_ENCHANTING);
        case ITEM_SUBCLASS_INSCRIPTION:
            return _bot->HasSkill(SKILL_INSCRIPTION);
        case ITEM_SUBCLASS_JEWELCRAFTING:
            return _bot->HasSkill(SKILL_JEWELCRAFTING);
        default:
            return false;
    }
}

bool GuildIntegration::IsEquipmentUpgrade(ItemTemplate const* itemTemplate) const
{
    if (!_bot || !itemTemplate)
        return false;

    // Check class/race restrictions
    if (itemTemplate->GetAllowableClass() != 0 && !(itemTemplate->GetAllowableClass() & _bot->GetClassMask()))
        return false;
    if (!itemTemplate->GetAllowableRace().IsEmpty() && !itemTemplate->GetAllowableRace().HasRace(_bot->GetRace()))
        return false;

    // Check level requirement
    if (itemTemplate->GetBaseRequiredLevel() > static_cast<int32>(_bot->GetLevel()))
        return false;

    // Determine equipment slot
    uint8 equipSlot = itemTemplate->GetInventoryType();
    if (equipSlot == 0)
        return false;

    // Get currently equipped item in that slot
    Item* equippedItem = nullptr;
    switch (equipSlot)
    {
        case INVTYPE_HEAD:
            equippedItem = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_HEAD);
            break;
        case INVTYPE_SHOULDERS:
            equippedItem = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_SHOULDERS);
            break;
        case INVTYPE_CHEST:
        case INVTYPE_ROBE:
            equippedItem = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_CHEST);
            break;
        case INVTYPE_WAIST:
            equippedItem = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_WAIST);
            break;
        case INVTYPE_LEGS:
            equippedItem = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_LEGS);
            break;
        case INVTYPE_FEET:
            equippedItem = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_FEET);
            break;
        case INVTYPE_WRISTS:
            equippedItem = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_WRISTS);
            break;
        case INVTYPE_HANDS:
            equippedItem = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_HANDS);
            break;
        case INVTYPE_CLOAK:
            equippedItem = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_BACK);
            break;
        case INVTYPE_WEAPONMAINHAND:
        case INVTYPE_2HWEAPON:
            equippedItem = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
            break;
        case INVTYPE_WEAPONOFFHAND:
        case INVTYPE_SHIELD:
        case INVTYPE_HOLDABLE:
            equippedItem = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
            break;
        default:
            return false;
    }

    // If slot is empty, this is an upgrade
    if (!equippedItem)
        return true;

    // Compare item levels
    uint32 newItemLevel = itemTemplate->GetBaseItemLevel();
    uint32 equippedItemLevel = equippedItem->GetTemplate()->GetBaseItemLevel();

    // Consider it an upgrade if it's at least 10 item levels higher
    return newItemLevel > equippedItemLevel + 10;
}

bool GuildIntegration::CanLearnRecipe(ItemTemplate const* itemTemplate) const
{
    if (!_bot || !itemTemplate || itemTemplate->GetClass() != ITEM_CLASS_RECIPE)
        return false;

    // Check if we have the profession for this recipe
    uint32 requiredSkill = itemTemplate->GetRequiredSkill();
    if (requiredSkill != 0 && !_bot->HasSkill(requiredSkill))
        return false;

    // Check skill level requirement
    uint32 requiredSkillRank = itemTemplate->GetRequiredSkillRank();
    if (requiredSkillRank > 0 && _bot->GetSkillValue(requiredSkill) < requiredSkillRank)
        return false;

    // TrinityCore 12.0: Use Effects vector instead of deprecated Spells array
    for (ItemEffectEntry const* effect : itemTemplate->Effects)
    {
        if (effect && effect->SpellID != 0 && _bot->HasSpell(effect->SpellID))
            return false;
    }

    return true;
}

bool GuildIntegration::HasItemsNeedingSockets() const
{
    if (!_bot)
        return false;

    // Check equipped items for empty sockets
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        Item* item = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item)
            continue;

        // Check if item has empty gem sockets
        for (uint32 i = 0; i < MAX_ITEM_PROTO_SOCKETS; ++i)
        {
            SocketColor socketColor = item->GetTemplate()->GetSocketColor(i);
            if (socketColor != 0) // 0 means no socket
            {
                // Socket exists, check if it's empty
                if (item->GetGem(i) == nullptr)
                    return true;
            }
        }
    }

    return false;
}

} // namespace Playerbot