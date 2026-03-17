/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_BOT_INTERACTION_TYPES_H
#define TRINITYCORE_BOT_INTERACTION_TYPES_H

#include "Define.h"
#include "ObjectGuid.h"
#include <vector>
#include <string>
#include <chrono>
#include <optional>

// Forward declarations
class Player;

namespace Playerbot
{
    // Constants
    constexpr uint32 MIN_INTERACTION_DELAY = 100; // Minimum milliseconds between interactions

    /**
     * @enum InteractionType
     * @brief Types of NPC interactions bots can perform
     */
    enum class InteractionType : uint8
    {

        None
        = 0,
        Vendor          = 1,
        Trainer         = 2,
        QuestGiver      = 3,
        Innkeeper       = 4,
        FlightMaster    = 5,

        Bank
        = 6,
        GuildBank       = 7,
        Mailbox         = 8,
        Auctioneer      = 9,
        Battlemaster    = 10,
        StableMaster    = 11,
        SpiritHealer    = 12,
        Transmogrifier  = 13,
        Reforger        = 14,
        VoidStorage     = 15,
        BarberShop      = 16,
        MAX_INTERACTION
    };

    /**
     * @enum InteractionResult
     * @brief Result codes for NPC interactions
     */
    enum class InteractionResult : uint8
    {

        Success
        = 0,

        Failed
        = 1,
        TooFarAway          = 2,
        InvalidTarget       = 3,
        NotEnoughMoney      = 4,
        InventoryFull       = 5,
        RequirementNotMet   = 6,
        TargetBusy          = 7,

        Cooldown
        = 8,
        NotAvailable        = 9,
        WrongFaction        = 10,

        InCombat
        = 11,
        Interrupted         = 12,

        Pending
        = 13,
        PartialSuccess      = 14
    };

    /**
     * @enum InteractionState
     * @brief States of the interaction state machine
     */
    enum class InteractionState : uint8
    {

        Idle
        = 0,
        Approaching     = 1,
        Initiating      = 2,
        WaitingGossip   = 3,
        ProcessingMenu  = 4,
        ExecutingAction = 5,
        Completing      = 6,
        Failed          = 7
    };

    /**
     * @enum VendorAction
     * @brief Specific vendor interaction actions
     */
    enum class VendorAction : uint8
    {
        None        = 0,
        Buy         = 1,
        Sell        = 2,
        Repair      = 3,
        BuyBack     = 4,
        BuyReagents = 5,
        SellJunk    = 6,
        RepairAll   = 7
    };

    /**
     * @enum TrainerAction
     * @brief Specific trainer interaction actions
     */
    enum class TrainerAction : uint8
    {

        None
        = 0,
        LearnSpell      = 1,
        LearnAllSpells  = 2,
        LearnOptimal    = 3,
        LearnRecipe     = 4,
        LearnRiding     = 5,
        UnlearnTalents  = 6
    };

    /**
     * @enum GossipSelectType
     * @brief Types of gossip menu selections
     */
    enum class GossipSelectType : uint8
    {
        Option      = 0,
        Vendor      = 1,
        Trainer     = 2,
        Taxi        = 3,
        Bank        = 4,
        Inn         = 5,
        GuildBank   = 6,
        Battlemaster = 7,
        Petition    = 8,
        Tabard      = 9,
        Custom      = 10,
        Resurrect   = 11  // Spirit healer resurrection
    };

    /**
     * @struct InteractionContext
     * @brief Context data for an interaction session
     */
    struct InteractionContext
    {
        ObjectGuid targetGuid;
        ObjectGuid botGuid;
        InteractionType type = InteractionType::None;
        InteractionState state = InteractionState::Idle;
        uint32 attemptCount = 0;
        uint32 maxAttempts = 3;
        ::std::chrono::steady_clock::time_point startTime;
        ::std::chrono::milliseconds timeout{10000};
        bool needsGossip = false;
        uint32 gossipMenuId = 0;
        ::std::vector<uint32> gossipPath; // Sequence of menu options to reach service

        void Reset()
        {

            targetGuid.Clear();

            type = InteractionType::None;

            state = InteractionState::Idle;

            attemptCount = 0;

            gossipMenuId = 0;

            gossipPath.clear();
        }

        bool IsExpired() const
        {

            return (::std::chrono::steady_clock::now() - startTime) > timeout;
        }
    };

    /**
     * @struct VendorInteractionData
     * @brief Data specific to vendor interactions
     */
    struct VendorInteractionData
    {
        struct ItemToBuy
        {

            uint32 entry = 0;

            uint32 count = 0;

            uint32 vendorSlot = 0;

            uint32 extendedCost = 0;
        };

        struct ItemToSell
        {

            ObjectGuid guid;

            uint32 count = 0;
        };

        ::std::vector<ItemToBuy> itemsToBuy;
        ::std::vector<ItemToSell> itemsToSell;
        bool needsRepair = false;
        uint32 repairCost = 0;
        bool sellJunk = true;
        bool buyReagents = true;
        uint32 maxBuyPrice = 10000000; // 1000 gold default max
    };

    /**
     * @struct TrainerInteractionData
     * @brief Data specific to trainer interactions
     */
    struct TrainerInteractionData
    {
        struct SpellToLearn
        {

            uint32 spellId = 0;

            uint32 cost = 0;

            uint32 reqLevel = 0;

            uint32 reqSkillRank = 0;

            bool isEssential = false;

            uint8 priority = 0; // 0 = highest
        };

        ::std::vector<SpellToLearn> availableSpells;
        ::std::vector<uint32> spellsToLearn;
        bool learnAll = false;
        bool optimalOnly = true;
        uint32 maxSpendGold = 1000000; // 100 gold default
        uint32 totalCost = 0;
    };

    /**
     * @struct FlightPathData
     * @brief Data for flight master interactions
     */
    struct FlightPathData
    {
        uint32 nodeId = 0;
        uint32 destinationNode = 0;
        ::std::vector<uint32> discoveredPaths;
        bool discoverNew = true;
        bool useOptimalRoute = true;
        uint32 cost = 0;
    };

    /**
     * @struct BankInteractionData
     * @brief Data for bank interactions
     */
    struct BankInteractionData
    {
        enum BankAction : uint8
        {

            None = 0,

            Deposit = 1,

            Withdraw = 2,

            BuySlot = 3,

            ViewOnly = 4
        };

        BankAction action = None;
        ::std::vector<ObjectGuid> itemsToDeposit;
        ::std::vector<uint32> itemsToWithdraw;
        uint8 slotsNeeded = 0;
        bool autoBuySlots = false;
    };

    /**
     * @struct MailInteractionData
     * @brief Data for mailbox interactions
     */
    struct MailInteractionData
    {
        enum MailAction : uint8
        {

            None = 0,

            CheckMail = 1,

            TakeAll = 2,

            TakeItem = 3,

            TakeMoney = 4,

            SendMail = 5,

            ReturnMail = 6,

            DeleteMail = 7
        };

        struct MailToSend
        {

            ObjectGuid recipient;

            ::std::string subject;

            ::std::string body;

            uint32 money = 0;

            ::std::vector<ObjectGuid> items;

            uint32 cod = 0;
        };

        MailAction action = None;
        ::std::vector<uint32> mailIds;
        ::std::vector<MailToSend> mailsToSend;
        bool takeAllItems = true;
        bool takeAllMoney = true;
        bool deleteEmpty = true;
    };

    /**
     * @struct InteractionMetrics
     * @brief Performance metrics for interactions
     */
    struct InteractionMetrics
    {
        uint32 totalAttempts = 0;
        uint32 successCount = 0;
        uint32 failureCount = 0;
        uint32 timeoutCount = 0;
        ::std::chrono::milliseconds totalDuration{0};
        ::std::chrono::milliseconds avgDuration{0};
        float successRate = 0.0f;

        void RecordAttempt(bool success, ::std::chrono::milliseconds duration)
        {

            ++totalAttempts;

            if (success)

                ++successCount;

            else

                ++failureCount;


            totalDuration += duration;

            if (totalAttempts > 0)

            {

                avgDuration = totalDuration / totalAttempts;

                successRate = static_cast<float>(successCount) / totalAttempts * 100.0f;

            }
        }

        void RecordTimeout()
        {

            ++totalAttempts;

            ++timeoutCount;

            ++failureCount;

            UpdateRates();
        }

    private:
        void UpdateRates()
        {

            if (totalAttempts > 0)

                successRate = static_cast<float>(successCount) / totalAttempts * 100.0f;
        }
    };

    /**
     * @struct GossipMenuOption
     * @brief Represents a single gossip menu option
     */
    struct GossipMenuOption
    {
        uint32 index = 0;
        uint8 icon = 0;
        ::std::string text;
        uint32 sender = 0;
        uint32 action = 0;
        ::std::string boxText;
        uint32 boxMoney = 0;
        bool coded = false;
    };

    /**
     * @struct NPCInteractionConfig
     * @brief Configuration for NPC interaction behavior
     */
    struct NPCInteractionConfig
    {
        // Range settings
        float interactionRange = 5.0f;
        float vendorSearchRange = 100.0f;
        float trainerSearchRange = 100.0f;
        float repairSearchRange = 200.0f;

        // Timing settings
        uint32 interactionDelay = 500;      // ms between interaction attempts
        uint32 gossipReadDelay = 100;       // ms to "read" gossip text
        uint32 maxInteractionTime = 30000;  // ms max for single interaction

        // Automation settings
        bool autoRepair = true;
        bool autoSellJunk = true;
        bool autoBuyReagents = true;
        bool autoLearnSpells = true;
        bool autoDiscoverFlightPaths = true;
        bool autoEmptyMail = true;

        // Thresholds
        float repairThreshold = 30.0f;      // Repair when durability < 30%

        uint32 minFreeSlots = 5;
        // Min free bag slots to maintain
        float reagentStockMultiple = 2.0f;  // Stock 2x normal reagent usage

        // Performance settings
        uint32 maxConcurrentInteractions = 3;
        bool enableMetrics = true;
        bool logInteractions = false;
    };

    /**
     * @struct ItemPriority
     * @brief Priority data for item decisions
     */
    struct ItemPriority
    {
        uint32 itemId = 0;
        int8 priority = 0;  // -127 to 127, higher = more important
        uint32 minStock = 0;
        uint32 maxStock = 0;
        bool essential = false;

        bool operator<(const ItemPriority& other) const
        {

            return priority > other.priority; // Higher priority first
        }
    };

    /**
     * @struct InteractionRequirement
     * @brief Requirements for an interaction to succeed
     */
    struct InteractionRequirement
    {
        uint32 minLevel = 0;
        uint32 maxLevel = 0;
        uint32 requiredQuest = 0;
        uint32 requiredItem = 0;
        uint32 requiredSpell = 0;
        uint32 requiredSkill = 0;
        uint32 requiredSkillRank = 0;
        uint32 requiredFaction = 0;
        uint32 requiredFactionRank = 0;
        uint32 requiredMoney = 0;
        bool requiredInCombat = false;
        bool requiredOutOfCombat = true;

        bool CheckRequirements(::Player* bot) const;
    };

    // Utility functions
    inline const char* InteractionTypeToString(InteractionType type)
    {
        switch (type)
        {

            case InteractionType::Vendor:
            return "Vendor";

            case InteractionType::Trainer:
            return "Trainer";

            case InteractionType::QuestGiver:
            return "QuestGiver";

            case InteractionType::Innkeeper:
            return "Innkeeper";

            case InteractionType::FlightMaster:
            return "FlightMaster";

            case InteractionType::Bank:
            return "Bank";

            case InteractionType::GuildBank:
            return "GuildBank";

            case InteractionType::Mailbox:
            return "Mailbox";

            case InteractionType::Auctioneer:
            return "Auctioneer";

            case InteractionType::Battlemaster:
            return "Battlemaster";

            case InteractionType::StableMaster:
            return "StableMaster";

            case InteractionType::SpiritHealer:
            return "SpiritHealer";

            case InteractionType::Transmogrifier:  return "Transmogrifier";

            case InteractionType::Reforger:
            return "Reforger";

            case InteractionType::VoidStorage:
            return "VoidStorage";

            case InteractionType::BarberShop:
            return "BarberShop";

            default:
            return "Unknown";
        }
    }

    inline const char* InteractionResultToString(InteractionResult result)
    {
        switch (result)
        {

            case InteractionResult::Success:
            return "Success";

            case InteractionResult::Failed:
            return "Failed";

            case InteractionResult::TooFarAway:
            return "TooFarAway";

            case InteractionResult::InvalidTarget:
            return "InvalidTarget";

            case InteractionResult::NotEnoughMoney:
            return "NotEnoughMoney";

            case InteractionResult::InventoryFull:
            return "InventoryFull";

            case InteractionResult::RequirementNotMet: return "RequirementNotMet";

            case InteractionResult::TargetBusy:
            return "TargetBusy";

            case InteractionResult::Cooldown:
            return "Cooldown";

            case InteractionResult::NotAvailable:
            return "NotAvailable";

            case InteractionResult::WrongFaction:
            return "WrongFaction";

            case InteractionResult::InCombat:
            return "InCombat";

            case InteractionResult::Interrupted:
            return "Interrupted";

            case InteractionResult::Pending:
            return "Pending";

            case InteractionResult::PartialSuccess:
            return "PartialSuccess";

            default:
            return "Unknown";
        }
    }
}

#endif // TRINITYCORE_BOT_INTERACTION_TYPES_H