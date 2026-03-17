/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef _PLAYERBOT_INTERACTIONMANAGER_H_
#define _PLAYERBOT_INTERACTIONMANAGER_H_

#include "InteractionTypes.h"
#include "Threading/LockHierarchy.h"
#include "Define.h"
#include "ObjectGuid.h"
#include "Position.h"
#include "ItemTemplate.h"
#include "QuestDef.h"
#include "GossipDef.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>
#include <shared_mutex>
#include <functional>
#include <chrono>
#include <atomic>
#include <mutex>

class Player;
class Creature;
class GameObject;
class WorldObject;
class WorldPacket;
class Item;
class Quest;
class SpellInfo;
struct GossipMenuItem;
struct TrainerSpell;

namespace Playerbot
{
    // Forward declarations
    // class VendorDatabase;  // Legacy - not implemented yet
    // class TrainerDatabase;  // Legacy - not implemented yet
    class GossipHandler;
    // class QuestDialogHandler;  // Legacy - not implemented yet
    class InteractionValidator;
    class VendorInteraction;
    // NOTE: The following interaction managers are FULLY IMPLEMENTED as separate files:
    // - TrainerInteractionManager    (Interaction/TrainerInteractionManager.cpp - 735 lines)
    // - InnkeeperInteractionManager  (Interaction/InnkeeperInteractionManager.cpp - 687 lines)
    // - FlightMasterManager          (Interaction/FlightMasterManager.cpp - 674 lines)
    // - BankInteractionManager       (Interaction/BankInteractionManager.cpp - 1004 lines)
    // - MailInteractionManager       (Interaction/MailInteractionManager.cpp - 827 lines)

    /**
     * @enum NPCType
     * @brief Types of NPCs for search and filtering
     */
    enum class NPCType : uint8
    {
        VENDOR              = 0,
        TRAINER             = 1,
        QUEST_GIVER         = 2,
        INNKEEPER           = 3,
        FLIGHT_MASTER       = 4,
        STABLE_MASTER       = 5,
        BANKER              = 6,
        AUCTIONEER          = 7,
        MAILBOX             = 8,
        BATTLEMASTER        = 9,
        REPAIR              = 10,
        TRANSMOGRIFIER      = 11,
        REFORGER            = 12,
        PORTAL              = 13,
        DUNGEON_FINDER      = 14,
        GENERAL             = 15
    };

    /**
     * @struct InteractionRequest
     * @brief Queued interaction request with priority and callback
     */
    struct InteractionRequest
    {
        ObjectGuid botGuid;         // Bot requesting interaction
        ObjectGuid targetGuid;      // Target of interaction
        InteractionType type;       // Type of interaction
        uint32 param1 = 0;          // itemId, spellId, questId, etc.
        uint32 param2 = 0;          // count, choice, etc.
        uint32 param3 = 0;          // additional parameter
        uint32 priority = 0;        // Higher = processed first
        uint32 timeoutMs = 10000;   // Timeout in milliseconds
        ::std::function<void(InteractionResult)> callback;

        // Priority queue comparison (higher priority first)
        bool operator<(const InteractionRequest& other) const
        {
            return priority < other.priority;
        }
    };

    /**
     * @struct VendorItem
     * @brief Information about a vendor's item
     */
    struct VendorItem
    {
        uint32 itemId;
        uint32 maxCount;
        uint32 incrTime;
        uint32 extendedCost;
        int32 price;
        uint8 type;

        bool IsAvailable() const;
        bool CanAfford(Player* player, uint32 count) const;
    };

    /**
     * @struct TrainerSpellInfo
     * @brief Information about a trainer's spell
     */
    struct TrainerSpellInfo
    {
        uint32 spellId;
        uint32 reqSkill;
        uint32 reqSkillValue;
        uint32 reqLevel;
        uint32 cost;
        uint32 reqSpell;

        bool CanLearn(Player* player) const;
        bool IsAlreadyKnown(Player* player) const;
    };

    /**
     * @struct NPCInteractionData
     * @brief Cached NPC information for fast lookup
     */
    struct NPCInteractionData
    {
        ObjectGuid guid;
        ::std::string name;
        NPCType type;
        ::Position position;
        float interactionRange;
        ::std::vector<uint32> services;  // Available services/flags
        uint32 lastInteraction = 0;
        bool isAvailable = true;
    };

    /**
     * @class InteractionManager
     * @brief Enterprise-grade asynchronous NPC interaction system with state machine processing
     *
     * Architecture:
     * - Uses InteractionContext for state tracking (defined in InteractionTypes.h)
     * - Implements state machine with InteractionState transitions
     * - Asynchronous queue-based processing for concurrent bot support
     * - Timeout and retry handling with configurable thresholds
     * - Performance metrics per interaction type
     * - Thread-safe for 500+ concurrent bots
     *
     * State Machine Flow:
     *   Idle -> Approaching -> Initiating -> [WaitingGossip -> ProcessingMenu]* -> ExecutingAction -> Completing
     *                                   \
     *                                    -> Failed (with retry logic)
     *
     * Design Decisions:
     * - State machine over simple calls: Supports complex multi-step interactions (gossip navigation, async responses)
     * - Async processing: Allows bots to queue interactions without blocking
     * - Metrics tracking: Performance monitoring for 500-bot scenarios
     * - Handler routing: Delegates specific logic to specialized handlers (VendorInteraction, etc.)
     */
    class InteractionManager final
    {
    public:
        InteractionManager();
        ~InteractionManager();

        static InteractionManager* Instance();

        // =====================================================================
        // LIFECYCLE MANAGEMENT
        // =====================================================================

        /**
         * @brief Initialize the interaction system
         * @return true if initialization succeeded
         */
        bool Initialize();

        /**
         * @brief Shutdown the interaction system and cancel all active interactions
         */
        void Shutdown();

        /**
         * @brief Global update for all active interactions
         * @param diff Time elapsed since last update (milliseconds)
         * @note Called from world update loop, not per-bot
         */
        void Update(uint32 diff);

        // =====================================================================
        // STATE MACHINE CONTROL (Public API for .cpp state machine)
        // =====================================================================

        /**
         * @brief Start an interaction with a world object
         * @param bot Bot initiating the interaction
         * @param target Target world object (Creature or GameObject)
         * @param type Type of interaction (auto-detected if None)
         * @return InteractionResult::Pending if started, error code otherwise
         */
        InteractionResult StartInteraction(Player* bot, WorldObject* target, InteractionType type = InteractionType::None);

        /**
         * @brief Cancel an active interaction
         * @param bot Bot to cancel interaction for
         * @param targetGuid Optional target to verify correct interaction (Empty = cancel any)
         */
        void CancelInteraction(Player* bot, ObjectGuid targetGuid = ObjectGuid::Empty);

        /**
         * @brief Check if bot has an active interaction
         * @param bot Bot to check
         * @return true if bot is currently interacting
         */
        bool HasActiveInteraction(Player* bot) const;

        /**
         * @brief Get current interaction context for a bot
         * @param bot Bot to get context for
         * @return Pointer to InteractionContext or nullptr if none active
         */
        InteractionContext* GetInteractionContext(Player* bot);

        /**
         * @brief Process the interaction state machine for a bot
         * @param bot Bot to process
         * @param diff Time elapsed since last update
         * @return true if interaction is still active, false if completed/failed
         * @note Called by BotAI update loop for active interactions
         */
        bool ProcessInteractionState(Player* bot, uint32 diff);

        /**
         * @brief Manually transition interaction to a new state
         * @param bot Bot to transition
         * @param newState New state to transition to
         * @note Normally handled internally by state machine
         */
        void TransitionState(Player* bot, InteractionState newState);

        // =====================================================================
        // PACKET HANDLERS (Called from worldserver packet handling)
        // =====================================================================

        /**
         * @brief Handle gossip menu packet from server
         * @param bot Bot receiving the gossip
         * @param packet Gossip menu packet
         */
        void HandleGossipMessage(Player* bot, WorldPacket const& packet);

        /**
         * @brief Handle vendor list packet from server
         * @param bot Bot receiving the vendor list
         * @param packet Vendor list packet
         */
        void HandleVendorList(Player* bot, WorldPacket const& packet);

        /**
         * @brief Handle trainer list packet from server
         * @param bot Bot receiving the trainer list
         * @param packet Trainer list packet
         */
        void HandleTrainerList(Player* bot, WorldPacket const& packet);

        /**
         * @brief Handle gossip menu internally (called by state machine)
         * @param bot Bot processing the gossip
         * @param menuId Gossip menu ID
         * @param target Target NPC
         */
        void HandleGossipMenu(Player* bot, uint32 menuId, WorldObject* target);

        /**
         * @brief Select a gossip option (used during gossip navigation)
         * @param bot Bot selecting the option
         * @param optionIndex Option index to select
         * @param target Target NPC
         */
        void SelectGossipOption(Player* bot, uint32 optionIndex, WorldObject* target);

        // =====================================================================
        // VENDOR OPERATIONS
        // =====================================================================

        /**
         * @brief Buy an item from a vendor
         * @param bot Bot buying the item
         * @param vendor Vendor NPC
         * @param itemId Item entry to buy
         * @param count Number to buy
         * @return InteractionResult indicating success or failure reason
         */
        InteractionResult BuyItem(Player* bot, Creature* vendor, uint32 itemId, uint32 count = 1);

        /**
         * @brief Sell junk items to a vendor
         * @param bot Bot selling items
         * @param vendor Vendor NPC
         * @return InteractionResult indicating success or failure reason
         */
        InteractionResult SellJunk(Player* bot, Creature* vendor);

        /**
         * @brief Repair all items at a vendor
         * @param bot Bot repairing items
         * @param vendor Vendor NPC with repair flag
         * @return InteractionResult indicating success or failure reason
         */
        InteractionResult RepairAll(Player* bot, Creature* vendor);

        // Legacy bool-based methods (deprecated but maintained for compatibility)
        bool SellItem(Player* bot, Creature* vendor, Item* item, uint32 count = 1);
        bool SellAllJunk(Player* bot, Creature* vendor);
        bool RepairItem(Player* bot, Creature* vendor, Item* item);
        ::std::vector<VendorItem> GetVendorItems(Creature* vendor) const;
        bool CanAffordItem(Player* bot, uint32 itemId, uint32 count = 1) const;

        // =====================================================================
        // TRAINER OPERATIONS
        // =====================================================================

        /**
         * @brief Learn optimal spells from a trainer
         * @param bot Bot learning spells
         * @param trainer Trainer NPC
         * @return InteractionResult indicating success or failure reason
         */
        InteractionResult LearnOptimalSpells(Player* bot, Creature* trainer);

        // Legacy bool-based methods (deprecated but maintained for compatibility)
        bool LearnSpell(Player* bot, Creature* trainer, uint32 spellId);
        bool LearnAllAvailableSpells(Player* bot, Creature* trainer);
        bool LearnProfession(Player* bot, Creature* trainer, uint32 skillId);
        bool UnlearnProfession(Player* bot, Creature* trainer, uint32 skillId);
        ::std::vector<TrainerSpellInfo> GetAvailableSpells(Player* bot, Creature* trainer) const;
        bool CanLearnSpell(Player* bot, uint32 spellId) const;
        uint32 GetTrainingCost(Player* bot, Creature* trainer) const;

        // =====================================================================
        // SERVICE OPERATIONS
        // =====================================================================

        /**
         * @brief Bind hearthstone at an innkeeper
         * @param bot Bot binding hearthstone
         * @param innkeeper Innkeeper NPC
         * @return InteractionResult indicating success or failure reason
         */
        InteractionResult BindHearthstone(Player* bot, Creature* innkeeper);

        /**
         * @brief Use a flight path
         * @param bot Bot using the flight path
         * @param flightMaster Flight master NPC
         * @param destinationNode Destination node ID (0 = discover only)
         * @return InteractionResult indicating success or failure reason
         */
        InteractionResult UseFlight(Player* bot, Creature* flightMaster, uint32 destinationNode = 0);

        /**
         * @brief Access bank storage
         * @param bot Bot accessing the bank
         * @param banker Banker NPC or bank chest GameObject
         * @return InteractionResult indicating success or failure reason
         */
        InteractionResult AccessBank(Player* bot, WorldObject* banker);

        /**
         * @brief Check and optionally take mail
         * @param bot Bot checking mail
         * @param mailbox Mailbox GameObject
         * @param takeAll If true, take all mail items/money
         * @return InteractionResult indicating success or failure reason
         */
        InteractionResult CheckMail(Player* bot, GameObject* mailbox, bool takeAll = false);

        // Legacy bool-based methods (deprecated but maintained for compatibility)
        bool SetHearthstone(Player* bot, Creature* innkeeper);
        bool DiscoverFlightPath(Player* bot, Creature* flightMaster);
        bool TakeFlightPath(Player* bot, Creature* flightMaster, uint32 nodeId);
        bool StablePet(Player* bot, Creature* stableMaster, uint32 petSlot);
        bool UnstablePet(Player* bot, Creature* stableMaster, uint32 petSlot);
        bool DepositToBank(Player* bot, Creature* banker, Item* item, uint32 count = 1);
        bool WithdrawFromBank(Player* bot, Creature* banker, uint32 itemId, uint32 count = 1);

        // =====================================================================
        // QUEST OPERATIONS (Legacy API)
        // =====================================================================

        bool AcceptQuest(Player* bot, Object* questGiver, uint32 questId);
        bool CompleteQuest(Player* bot, Object* questGiver, uint32 questId);
        bool TurnInQuest(Player* bot, Object* questGiver, uint32 questId, uint32 rewardChoice = 0);
        bool AbandonQuest(Player* bot, uint32 questId);
        ::std::vector<uint32> GetAvailableQuests(Player* bot, Object* questGiver) const;
        ::std::vector<uint32> GetCompletableQuests(Player* bot, Object* questGiver) const;
        uint32 SelectQuestReward(Player* bot, Quest const* quest) const;

        // =====================================================================
        // GOSSIP HANDLING (Legacy API)
        // =====================================================================

        bool CloseGossip(Player* bot, Creature* npc);
        ::std::vector<GossipMenuItem> GetGossipOptions(Player* bot, Creature* npc) const;
        bool HasGossipOption(Creature* npc, uint32 option) const;
        ::std::string GetGossipText(Player* bot, Creature* npc) const;

        // =====================================================================
        // AUCTION HOUSE (Legacy API)
        // =====================================================================

        bool CreateAuction(Player* bot, Creature* auctioneer, Item* item, uint32 bid, uint32 buyout, uint32 duration);
        bool BidOnAuction(Player* bot, Creature* auctioneer, uint32 auctionId, uint32 bid);
        bool BuyoutAuction(Player* bot, Creature* auctioneer, uint32 auctionId);
        bool CancelAuction(Player* bot, Creature* auctioneer, uint32 auctionId);

        // =====================================================================
        // MAIL OPERATIONS (Legacy API)
        // =====================================================================

        bool SendMail(Player* bot, ::std::string const& recipient, ::std::string const& subject,
                      ::std::string const& body, uint32 money = 0, Item* item = nullptr);
        bool TakeMail(Player* bot, uint32 mailId);
        bool DeleteMail(Player* bot, uint32 mailId);
        bool ReturnMail(Player* bot, uint32 mailId);

        // =====================================================================
        // NPC DETECTION AND SEARCH
        // =====================================================================

        /**
         * @brief Detect the type of an NPC based on flags
         * @param target Creature to detect
         * @return InteractionType detected (cached for performance)
         */
        InteractionType DetectNPCType(Creature* target) const;

        /**
         * @brief Find nearest NPC of a specific type
         * @param bot Bot searching
         * @param type NPCType to search for
         * @param maxRange Maximum search range
         * @return Nearest NPC or nullptr if none found
         */
        Creature* FindNearestNPC(Player* bot, NPCType type, float maxRange = 100.0f) const;

        // Legacy methods
        bool CanInteract(Player* bot, Creature* npc) const;
        bool CanInteract(Player* bot, GameObject* go) const;
        float GetInteractionRange(Creature* npc) const;
        float GetInteractionRange(GameObject* go) const;
        bool IsInInteractionRange(Player* bot, Object* target) const;
        NPCType GetNPCType(Creature* npc) const;
        ::std::vector<Creature*> FindNearbyNPCs(Player* bot, NPCType type, float range = 50.0f) const;
        GameObject* FindNearestGameObject(Player* bot, uint32 entry, float maxRange = 50.0f) const;

        // =====================================================================
        // INTERACTION STATE QUERIES
        // =====================================================================

        bool IsInteracting(Player* bot) const;
        InteractionType GetCurrentInteraction(Player* bot) const;
        Object* GetInteractionTarget(Player* bot) const;
        uint32 GetInteractionTime(Player* bot) const;
        void ResetInteraction(Player* bot);

        // =====================================================================
        // SMART AUTOMATION
        // =====================================================================

        bool SmartSell(Player* bot);          // Find vendor and sell junk
        bool SmartRepair(Player* bot);        // Find repair NPC and repair
        bool SmartTrain(Player* bot);         // Find trainer and learn spells
        bool SmartBank(Player* bot);          // Find bank and manage items
        bool SmartMail(Player* bot);          // Check and collect mail

        // =====================================================================
        // CONFIGURATION
        // =====================================================================

        void SetInteractionDelay(uint32 delayMs) { m_interactionDelay = delayMs; }
        void SetMaxInteractionAttempts(uint8 attempts) { m_maxAttempts = attempts; }
        void EnableAutoSell(Player* bot, bool enable = true);
        void EnableAutoRepair(Player* bot, bool enable = true);
        void EnableAutoTrain(Player* bot, bool enable = true);

        // =====================================================================
        // PERFORMANCE METRICS
        // =====================================================================

        /**
         * @brief Get performance metrics for an interaction type
         * @param type InteractionType to get metrics for (None = combined metrics)
         * @return InteractionMetrics struct with success rates, timings, etc.
         */
        InteractionMetrics GetMetrics(InteractionType type = InteractionType::None) const;

        /**
         * @brief Reset all performance metrics
         */
        void ResetMetrics();

        uint32 GetActiveInteractions() const { return static_cast<uint32>(m_activeInteractions.size()); }
        uint32 GetQueuedInteractions() const { return static_cast<uint32>(m_interactionQueue.size()); }
        void GetPerformanceMetrics(uint32& successCount, uint32& failCount, uint32& avgTime) const;

    private:
        // =====================================================================
        // INTERNAL HELPER METHODS
        // =====================================================================

        void InitializeHandlers();
        void CleanupHandlers();

        /**
         * @brief Route interaction to appropriate handler
         * @param bot Bot performing the interaction
         * @param target Target world object
         * @param type Interaction type
         * @return InteractionResult from the handler
         */
        InteractionResult RouteToHandler(Player* bot, WorldObject* target, InteractionType type);

        /**
         * @brief Update a specific interaction's state machine
         * @param bot Bot being updated
         * @param context Interaction context
         * @param diff Time elapsed
         * @return true if interaction continues, false if completed/failed
         */
        bool UpdateInteraction(Player* bot, InteractionContext& context, uint32 diff);

        /**
         * @brief Execute current state of the state machine
         * @param bot Bot executing state
         * @param context Interaction context
         * @return InteractionResult for the state execution
         */
        InteractionResult ExecuteState(Player* bot, InteractionContext& context);

        /**
         * @brief Complete an interaction and record metrics
         * @param bot Bot completing interaction
         * @param result Final result of the interaction
         */
        void CompleteInteraction(Player* bot, InteractionResult result);

        /**
         * @brief Check if interaction has timed out
         * @param context Interaction context to check
         * @return true if timed out
         */
        bool CheckTimeout(InteractionContext const& context) const;

        /**
         * @brief Check if bot is in interaction range
         * @param bot Bot to check
         * @param target Target to check range to
         * @return true if in range
         */
        bool IsInInteractionRange(Player* bot, WorldObject* target) const;

        /**
         * @brief Move bot to interaction range
         * @param bot Bot to move
         * @param target Target to move to
         * @return true if movement initiated successfully
         */
        bool MoveToInteractionRange(Player* bot, WorldObject* target);

        /**
         * @brief Handle interaction error with retry logic
         * @param bot Bot that encountered error
         * @param error Error code
         */
        void HandleInteractionError(Player* bot, InteractionResult error);

        /**
         * @brief Attempt to recover from interaction failure
         * @param bot Bot to recover
         * @return true if recovery initiated
         */
        bool AttemptRecovery(Player* bot);

        /**
         * @brief Log interaction event (if logging enabled)
         * @param bot Bot performing interaction
         * @param message Log message
         */
        void LogInteraction(Player* bot, ::std::string const& message) const;

        // =====================================================================
        // MEMBER VARIABLES
        // =====================================================================

        // Active interactions - maps bot GUID to interaction context
        ::std::unordered_map<ObjectGuid, ::std::unique_ptr<InteractionContext>> m_activeInteractions;

        // Interaction queue for prioritized processing
        ::std::priority_queue<InteractionRequest> m_interactionQueue;

        // Timing trackers
        ::std::unordered_map<ObjectGuid, ::std::chrono::steady_clock::time_point> m_lastInteractionTime;
        ::std::chrono::steady_clock::time_point m_lastCacheClean;

        // NPC type cache for fast detection
        mutable ::std::unordered_map<ObjectGuid, InteractionType> m_npcTypeCache;

        // Subsystems - specialized handlers
        ::std::unique_ptr<GossipHandler> m_gossipHandler;
        ::std::unique_ptr<InteractionValidator> m_validator;
        // NOTE: VendorInteraction is incomplete type - design decision to use separate manager files instead
        // ::std::unique_ptr<VendorInteraction> m_vendorHandler;
        //
        // The following interaction managers are FULLY IMPLEMENTED as separate files with TrinityCore API integration:
        // - TrainerInteractionManager    (Interaction/TrainerInteractionManager.cpp) - Full trainer/profession spell learning
        // - InnkeeperInteractionManager  (Interaction/InnkeeperInteractionManager.cpp) - Hearthstone binding, rested state
        // - FlightMasterManager          (Interaction/FlightMasterManager.cpp) - TaxiPathGraph integration
        // - BankInteractionManager       (Interaction/BankInteractionManager.cpp) - Bank deposit/withdraw operations
        // - MailInteractionManager       (Interaction/MailInteractionManager.cpp) - Mail send/receive/COD handling

        // Legacy subsystems (for backward compatibility) - commented out until implemented
        // std::unique_ptr<VendorDatabase> m_vendorDB;
        // std::unique_ptr<TrainerDatabase> m_trainerDB;
        // std::unique_ptr<QuestDialogHandler> m_questHandler;

        // NPC database cache (legacy)
        ::std::unordered_map<ObjectGuid, NPCInteractionData> m_npcDatabase;

        // Thread safety - shared_mutex allows concurrent reads
        mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::ACTION_PRIORITY> m_mutex;
        mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::ACTION_PRIORITY> m_npcMutex;

        // Performance tracking - atomic for thread safety
        ::std::atomic<uint64> m_totalInteractionsStarted{0};
        ::std::atomic<uint64> m_totalInteractionsCompleted{0};
        ::std::atomic<uint64> m_totalInteractionsFailed{0};
        ::std::atomic<uint32> m_totalSuccess{0};
        ::std::atomic<uint32> m_totalFailed{0};
        ::std::atomic<uint32> m_totalTime{0};

        // Metrics per interaction type
        ::std::unordered_map<InteractionType, InteractionMetrics> m_metrics;

        // Configuration
        NPCInteractionConfig m_config;
        uint32 m_interactionDelay = 1000;      // Default 1 second delay
        uint8 m_maxAttempts = 3;               // Max attempts per interaction
        float m_defaultInteractionRange = 5.0f; // Default interaction range

        // Initialization state
        bool m_initialized = false;

        // Singleton instance (modern C++20 pattern)
        static ::std::unique_ptr<InteractionManager> s_instance;
        static ::std::once_flag s_initFlag;
    };

    // Helper macro for interaction manager access
    #define sInteractionMgr InteractionManager::Instance()
}

#endif // _PLAYERBOT_INTERACTIONMANAGER_H_
