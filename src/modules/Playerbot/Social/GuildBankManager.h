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
#include "Item.h"
#include "GameTime.h"
#include <unordered_map>
#include <vector>
#include <atomic>
#include <mutex>

namespace Playerbot
{

enum class GuildBankItemType : uint8
{
    CONSUMABLES         = 0,
    CRAFTING_MATERIALS  = 1,
    EQUIPMENT           = 2,
    GEMS                = 3,
    ENCHANTING_MATERIALS= 4,
    QUEST_ITEMS         = 5,
    TRADE_GOODS         = 6,
    VALUABLE_ITEMS      = 7
};

enum class BankOperation : uint8
{
    DEPOSIT     = 0,
    WITHDRAW    = 1,
    ORGANIZE    = 2,
    ANALYZE     = 3
};

struct GuildBankItem
{
    uint32 itemId;
    uint32 itemGuid;
    uint32 stackCount;
    uint32 tabId;
    uint32 slotId;
    GuildBankItemType itemType;
    uint32 estimatedValue;
    uint32 lastUpdated;
    std::string depositorName;
    bool isReserved;
    uint32 reservedFor;

    GuildBankItem() : itemId(0), itemGuid(0), stackCount(0), tabId(0), slotId(0)
        , itemType(GuildBankItemType::CONSUMABLES), estimatedValue(0)
        , lastUpdated(GameTime::GetGameTimeMS()), isReserved(false), reservedFor(0) {}
};

/**
 * @brief Advanced guild bank management system for automated organization
 *
 * This system provides intelligent guild bank interactions, item organization,
 * and automated deposit/withdrawal management for playerbots.
 */
class TC_GAME_API GuildBankManager final 
{
public:
    explicit GuildBankManager(Player* bot);
    ~GuildBankManager();
    GuildBankManager(GuildBankManager const&) = delete;
    GuildBankManager& operator=(GuildBankManager const&) = delete;

    // Core guild bank operations using TrinityCore's Guild system
    bool DepositItem(uint32 itemGuid, uint32 tabId, uint32 stackCount);
    bool WithdrawItem(uint32 tabId, uint32 slotId, uint32 stackCount);
    bool MoveItem(uint32 fromTab, uint32 fromSlot, uint32 toTab, uint32 toSlot);
    bool CanAccessGuildBank(uint32 tabId);

    // Intelligent bank management
    void AutoOrganizeGuildBank();
    void OptimizeItemPlacement();
    void AnalyzeGuildBankContents();
    void PlanBankReorganization();

    // Automated deposit strategies
    void AutoDepositItems();
    void DepositExcessConsumables();
    void DepositCraftingMaterials();
    void DepositValuableItems();
    void DepositDuplicateEquipment();

    // Automated withdrawal strategies
    void AutoWithdrawNeededItems();
    void WithdrawConsumables();
    void WithdrawCraftingMaterials();
    void WithdrawRepairItems();
    void WithdrawRequestedItems();

    // Bank organization and optimization
    struct BankOrganizationPlan
    {
        std::unordered_map<uint32, GuildBankItemType> tabAssignments; // tabId -> item type
        std::vector<std::pair<uint32, uint32>> itemMoves; // (fromSlot, toSlot) pairs
        std::vector<uint32> itemsToDeposit;
        std::vector<uint32> itemsToWithdraw;
        uint32 estimatedTime;
        float organizationScore;

        BankOrganizationPlan() : estimatedTime(0), organizationScore(0.0f) {}
    };

    BankOrganizationPlan CreateOrganizationPlan();
    void ExecuteOrganizationPlan(const BankOrganizationPlan& plan);
    float CalculateOrganizationScore(Guild* guild);

    // Bank monitoring and analysis
    struct BankAnalysis
    {
        uint32 guildId;
        std::unordered_map<GuildBankItemType, uint32> itemCounts;
        std::unordered_map<GuildBankItemType, uint32> totalValues;
        std::vector<GuildBankItem> mostValuableItems;
        std::vector<GuildBankItem> duplicateItems;
        std::vector<GuildBankItem> expiredItems;
        float utilizationRate;
        float organizationLevel;
        uint32 lastAnalysisTime;

        BankAnalysis() : guildId(0), utilizationRate(0.0f)
            , organizationLevel(0.5f), lastAnalysisTime(GameTime::GetGameTimeMS()) {}
        BankAnalysis(uint32 gId) : guildId(gId), utilizationRate(0.0f)
            , organizationLevel(0.5f), lastAnalysisTime(GameTime::GetGameTimeMS()) {}
    };

    BankAnalysis AnalyzeGuildBank();
    void UpdateBankAnalysis(uint32 guildId);
    std::vector<uint32> IdentifyDuplicateItems(Guild* guild);
    std::vector<uint32> IdentifyUnusedItems(Guild* guild);

    // Bank permissions and access control
    bool HasDepositRights(uint32 tabId);
    bool HasWithdrawRights(uint32 tabId);
    uint32 GetDailyWithdrawLimit(uint32 tabId);
    uint32 GetRemainingWithdraws(uint32 tabId);
    void TrackBankUsage(BankOperation operation);

    // Item categorization and intelligence
    GuildBankItemType DetermineItemCategory(uint32 itemId);
    uint32 EstimateItemValue(uint32 itemId, uint32 stackCount = 1);
    bool IsItemNeededByGuild(Guild* guild, uint32 itemId);
    std::vector<uint32> GetSimilarItems(uint32 itemId);
    bool ShouldItemBeInGuildBank(uint32 itemId);

    // Bank space optimization
    void OptimizeBankSpace();
    std::vector<uint32> IdentifyLowValueItems(Guild* guild);
    void ConsolidateStacks(uint32 tabId);
    void RemoveExpiredItems();
    uint32 CalculateAvailableSpace(Guild* guild, uint32 tabId);

    // Member interaction with bank
    struct MemberBankProfile
    {
        uint32 playerGuid;
        uint32 guildId;
        std::unordered_map<GuildBankItemType, uint32> depositPreferences;
        std::unordered_map<GuildBankItemType, uint32> withdrawFrequency;
        uint32 totalDeposits;
        uint32 totalWithdrawals;
        uint32 lastBankAccess;
        float contributionScore;
        float trustLevel;

        MemberBankProfile() : playerGuid(0), guildId(0)
            , totalDeposits(0), totalWithdrawals(0), lastBankAccess(GameTime::GetGameTimeMS())
            , contributionScore(0.5f), trustLevel(0.8f) {}
        MemberBankProfile(uint32 pGuid, uint32 gId) : playerGuid(pGuid), guildId(gId)
            , totalDeposits(0), totalWithdrawals(0), lastBankAccess(GameTime::GetGameTimeMS())
            , contributionScore(0.5f), trustLevel(0.8f) {}
    };

    MemberBankProfile GetMemberBankProfile();
    void UpdateMemberBankProfile(BankOperation operation, uint32 itemId);
    std::vector<uint32> GetTopContributors(uint32 guildId);
    std::vector<uint32> GetFrequentUsers(uint32 guildId);

    // Automated bank maintenance
    void PerformBankMaintenance();
    void CleanupBankTabs();
    void UpdateItemPrices(Guild* guild);
    void ArchiveOldItems();
    void GenerateBankReport();

    // Performance monitoring
    struct BankMetrics
    {
        std::atomic<uint32> totalDeposits{0};
        std::atomic<uint32> totalWithdrawals{0};
        std::atomic<uint32> itemsMoved{0};
        std::atomic<uint32> organizationActions{0};
        std::atomic<uint32> totalBankValue{0};
        std::atomic<float> utilizationRate{0.7f};
        std::atomic<float> organizationScore{0.8f};
        std::atomic<float> memberSatisfaction{0.85f};

        void Reset()
        {
            totalDeposits = 0; totalWithdrawals = 0; itemsMoved = 0;
            organizationActions = 0; totalBankValue = 0; utilizationRate = 0.7f;
            organizationScore = 0.8f; memberSatisfaction = 0.85f;
        }
    };

    BankMetrics GetGuildBankMetrics(uint32 guildId);
    BankMetrics GetGlobalBankMetrics();

    // Configuration and policies
    void SetBankPolicy(uint32 guildId, GuildBankItemType itemType, const std::string& policy);
    void SetAutoOrganizationEnabled(uint32 guildId, bool enabled);
    void SetItemCategoryTab(uint32 guildId, GuildBankItemType itemType, uint32 tabId);
    void ConfigureBankAccess(uint32 guildId, uint32 rankId, uint32 tabId, bool canDeposit, bool canWithdraw);

    // Update and maintenance
    void Update(uint32 diff);
    void UpdateBankStates();
    void ProcessBankRequests();
    void CleanupBankData();

private:
    Player* _bot;

    // Core bank data
    std::unordered_map<uint32, BankAnalysis> _guildBankAnalysis; // guildId -> analysis
    std::unordered_map<uint32, MemberBankProfile> _memberProfiles; // playerGuid -> profile
    std::unordered_map<uint32, BankMetrics> _guildMetrics; // guildId -> metrics
    

    // Item categorization system
    std::unordered_map<uint32, GuildBankItemType> _itemCategories; // itemId -> category
    std::unordered_map<GuildBankItemType, std::vector<uint32>> _categoryItems; // category -> itemIds
    std::unordered_map<uint32, uint32> _itemValues; // itemId -> estimated value

    // Bank organization data
    struct BankConfiguration
    {
        uint32 guildId;
        std::unordered_map<uint32, GuildBankItemType> tabPurposes; // tabId -> item type
        std::unordered_map<GuildBankItemType, std::string> categoryPolicies;
        bool autoOrganizationEnabled;
        uint32 organizationFrequency;
        uint32 lastOrganization;

        BankConfiguration() : guildId(0), autoOrganizationEnabled(true)
            , organizationFrequency(86400000), lastOrganization(GameTime::GetGameTimeMS()) {}
        BankConfiguration(uint32 gId) : guildId(gId), autoOrganizationEnabled(true)
            , organizationFrequency(86400000), lastOrganization(GameTime::GetGameTimeMS()) {}
    };

    std::unordered_map<uint32, BankConfiguration> _guildConfigurations; // guildId -> config

    // Performance tracking
    BankMetrics _globalMetrics;

    // Helper functions
    void InitializeItemCategories();
    void LoadGuildBankData(uint32 guildId);
    bool ValidateBankOperation(BankOperation operation, uint32 tabId);
    void LogBankTransaction(BankOperation operation, uint32 itemId, uint32 tabId);

    // Organization algorithms
    void CalculateOptimalTabLayout(Guild* guild, std::unordered_map<uint32, GuildBankItemType>& layout);
    std::vector<std::pair<uint32, uint32>> PlanItemMoves(Guild* guild, const BankOrganizationPlan& plan);
    void ExecuteItemMove(uint32 fromTab, uint32 fromSlot, uint32 toTab, uint32 toSlot);
    float EvaluateOrganizationEfficiency(Guild* guild);
    std::vector<GuildBankItem> GetDuplicatesAnalysis(Guild* guild);
    std::vector<GuildBankItem> GetExpiredItemsAnalysis(Guild* guild);

    // Item analysis
    void AnalyzeItemDistribution(Guild* guild, BankAnalysis& analysis);
    void IdentifyOptimizationOpportunities(Guild* guild, BankAnalysis& analysis);
    void AssessItemUtility(Guild* guild, uint32 itemId, float& utility);
    bool IsItemRedundant(Guild* guild, uint32 itemId);

    // Access control validation
    bool ValidateTabAccess(uint32 tabId, BankOperation operation);
    void UpdateUsageLimits(BankOperation operation, uint32 tabId);
    void EnforceWithdrawLimits(uint32 tabId, uint32 requestedAmount);

    // Performance optimization
    void CacheBankData(uint32 guildId);
    void OptimizeBankQueries(Guild* guild);
    void UpdateBankMetrics(uint32 guildId, BankOperation operation, bool wasSuccessful);
    void PreloadFrequentItems();

    // Constants
    static constexpr uint32 BANK_UPDATE_INTERVAL = 300000; // 5 minutes
    static constexpr uint32 ORGANIZATION_CHECK_INTERVAL = 1800000; // 30 minutes
    static constexpr uint32 ANALYSIS_REFRESH_INTERVAL = 3600000; // 1 hour
    static constexpr uint32 MAX_TABS_PER_GUILD = 8;
    static constexpr uint32 SLOTS_PER_TAB = 98;
    static constexpr float MIN_ORGANIZATION_SCORE = 0.6f;
    static constexpr uint32 ITEM_VALUE_CACHE_DURATION = 86400000; // 24 hours
    static constexpr uint32 MAX_STACK_MOVES_PER_SESSION = 50;
    static constexpr float CONTRIBUTION_SCORE_DECAY = 0.005f; // Daily decay
    static constexpr uint32 BANK_OPERATION_TIMEOUT = 30000; // 30 seconds
};

} // namespace Playerbot