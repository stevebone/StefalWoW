/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * GAME SYSTEMS MANAGER - Concrete Facade Implementation
 *
 * Phase 6: BotAI Decoupling & Final Cleanup
 *
 * This class consolidates all 17 manager instances from BotAI into a single
 * facade, reducing BotAI's complexity and improving testability.
 */

#ifndef PLAYERBOT_GAME_SYSTEMS_MANAGER_H
#define PLAYERBOT_GAME_SYSTEMS_MANAGER_H

#include "IGameSystemsManager.h"
#include "AI/AIBudgetTier.h"
#include <memory>

// Manager includes
#include "Social/TradeManager.h"
#include "Professions/GatheringManager.h"
#include "Professions/ProfessionManager.h"
#include "Professions/GatheringMaterialsBridge.h"
#include "Professions/AuctionMaterialsBridge.h"
#include "Professions/ProfessionAuctionBridge.h"
#include "Professions/FarmingCoordinator.h"
#include "Economy/AuctionManager.h"
#include "Banking/BankingManager.h"
#include "Equipment/EquipmentManager.h"
#include "Companion/MountManager.h"
#include "Companion/RidingManager.h"
#include "Companion/BattlePetManager.h"
#include "Humanization/Core/HumanizationManager.h"
#include "PvP/ArenaAI.h"
#include "PvP/PvPCombatAI.h"
#include "Social/AuctionHouse.h"
#include "Social/TradeSystem.h"
#include "Quest/QuestValidation.h"
#include "Group/RoleAssignment.h"
#include "LFG/LFGGroupCoordinator.h"
#include "Dungeon/InstanceCoordination.h"
#include "Session/BotPriorityManager.h"
#include "Session/BotWorldSessionMgr.h"
#include "Lifecycle/BotLifecycleManager.h"
#include "LFG/LFGBotSelector.h"
#include "LFG/LFGBotManager.h"
#include "Quest/QuestTurnIn.h"
#include "Quest/QuestPickup.h"
#include "Quest/QuestCompletion.h"
#include "Quest/ObjectiveTracker.h"
#include "Quest/DynamicQuestSystem.h"
// LootDistribution.h NOT included here - forward declaration at line 78 is sufficient
// Full include is in GameSystemsManager.cpp (required for unique_ptr construction)
#include "Social/GuildIntegration.h"
#include "Social/GuildEventCoordinator.h"
#include "Social/GuildBankManager.h"
#include "Advanced/GroupCoordinator.h"
#include "Lifecycle/DeathRecoveryManager.h"
#include "Movement/UnifiedMovementCoordinator.h"
#include "Combat/CombatStateManager.h"
#include "Combat/TargetScanner.h"
#include "Group/GroupInvitationHandler.h"
#include "Core/Events/EventDispatcher.h"
#include "Core/Managers/ManagerRegistry.h"
#include "AI/Decision/DecisionFusionSystem.h"
#include "AI/Decision/ActionPriorityQueue.h"
#include "AI/Decision/BehaviorTree.h"
#include "AI/HybridAIController.h"
#include "BehaviorPriorityManager.h"

// Sprint 3: Combat Coordination
#include "AI/Combat/CombatCoordinationIntegrator.h"

// Combat Behaviors managers
#include "AI/CombatBehaviors/AoEDecisionManager.h"
#include "AI/CombatBehaviors/CooldownStackingOptimizer.h"
#include "AI/CombatBehaviors/DefensiveBehaviorManager.h"
#include "AI/CombatBehaviors/DispelCoordinator.h"
#include "AI/CombatBehaviors/InterruptRotationManager.h"

// Game Systems
#include "Game/ConsumableManager.h"

namespace Playerbot
{

// Forward declarations
class BotAI;
class LootDistribution;

/**
 * @brief Concrete implementation of IGameSystemsManager facade
 *
 * **Ownership:**
 * - Owns all 24 manager instances via std::unique_ptr (including bridges)
 * - Owned by BotAI via std::unique_ptr<IGameSystemsManager>
 * - Returns raw pointers (non-owning) to external callers
 *
 * **Lifecycle:**
 * 1. BotAI constructs: _gameSystems = std::make_unique<GameSystemsManager>(_bot, botAI)
 * 2. BotAI initializes: _gameSystems->Initialize(_bot)
 * 3. BotAI updates each frame: _gameSystems->Update(diff)
 * 4. BotAI destructs: _gameSystems.reset() auto-destroys all managers
 *
 * **Phase 6 Migration:**
 * All manager initialization, update, and cleanup code moved from BotAI
 * to this facade, reducing BotAI from 3,013 lines to ~1,800 lines (40% reduction).
 *
 * **Phase 1B Integration (2025-11-18):**
 * ProfessionManager converted from global singleton to per-bot instance (18th manager),
 * eliminating singleton anti-pattern and aligning with Phase 6 per-bot architecture.
 *
 * **Phase 4.1 Integration (2025-11-18):**
 * GatheringMaterialsBridge converted from singleton to per-bot instance (19th manager)
 *
 * **Phase 4.2 Integration (2025-11-18):**
 * AuctionMaterialsBridge converted from singleton to per-bot instance (20th manager)
 *
 * **Phase 4.3 Integration (2025-11-18):**
 * ProfessionAuctionBridge converted from singleton to per-bot instance (21st manager)
 *
 * **Phase 5.1 Integration (2025-11-18):**
 * BankingManager converted from singleton to per-bot instance (22nd manager)
 *
 * **Phase 5.2 Integration (2025-11-18):**
 * FarmingCoordinator converted from singleton to per-bot instance (23rd manager)
 *
 * **Phase 6.1 Integration (2025-11-18):**
 * EquipmentManager converted from singleton to per-bot instance (24th manager)
 *
 * **Phase 6.2 Integration (2025-11-18):**
 * MountManager converted from singleton to per-bot instance (25th manager)
 *
 * **Phase 6.3 Integration (2025-11-18):**
 * BattlePetManager converted from singleton to per-bot instance (26th manager)
 *
 * **Phase 7.1.1 Integration (2025-11-18):**
 * ArenaAI converted from singleton to per-bot instance (27th manager)
 *
 * **Phase 7.1.2 Integration (2025-11-18):**
 * PvPCombatAI converted from singleton to per-bot instance (28th manager)
 */
class TC_GAME_API GameSystemsManager final : public IGameSystemsManager 
{
public:
    /**
     * @brief Construct facade for bot's game systems
     * @param bot The bot player this facade serves
     * @param botAI The BotAI instance (needed for manager constructors)
     */
    explicit GameSystemsManager(Player* bot, BotAI* botAI);

    /**
     * @brief Destructor - handles proper cleanup order
     *
     * CRITICAL: Managers destroyed in correct dependency order to prevent
     * access violations when unsubscribing from EventDispatcher.
     */
    ~GameSystemsManager() override;

    // ========================================================================
    // LIFECYCLE (IGameSystemsManager interface)
    // ========================================================================

    void Initialize(Player* bot) override;
    void Shutdown() override;
    void Update(uint32 diff) override;

    // ========================================================================
    // GAME SYSTEM ACCESS (IGameSystemsManager interface)
    // ========================================================================

    TradeManager* GetTradeManager() const override { return _tradeManager.get(); }
    GatheringManager* GetGatheringManager() const override { return _gatheringManager.get(); }
    ProfessionManager* GetProfessionManager() const override { return _professionManager.get(); }
    GatheringMaterialsBridge* GetGatheringMaterialsBridge() const { return _gatheringMaterialsBridge.get(); }
    AuctionMaterialsBridge* GetAuctionMaterialsBridge() const { return _auctionMaterialsBridge.get(); }
    ProfessionAuctionBridge* GetProfessionAuctionBridge() const { return _professionAuctionBridge.get(); }
    FarmingCoordinator* GetFarmingCoordinator() const { return _farmingCoordinator.get(); }
    AuctionManager* GetAuctionManager() const override { return _auctionManager.get(); }
    BankingManager* GetBankingManager() const override { return _bankingManager.get(); }
    EquipmentManager* GetEquipmentManager() const override { return _equipmentManager.get(); }
    MountManager* GetMountManager() const { return _mountManager.get(); }
    RidingManager* GetRidingManager() const { return _ridingManager.get(); }
    BattlePetManager* GetBattlePetManager() const { return _battlePetManager.get(); }
    Humanization::HumanizationManager* GetHumanizationManager() const { return _humanizationManager.get(); }
    ArenaAI* GetArenaAI() const { return _arenaAI.get(); }
    PvPCombatAI* GetPvPCombatAI() const { return _pvpCombatAI.get(); }
    AuctionHouse* GetAuctionHouse() const { return _auctionHouse.get(); }
    GuildBankManager* GetGuildBankManager() const { return _guildBankManager.get(); }
    GuildEventCoordinator* GetGuildEventCoordinator() const { return _guildEventCoordinator.get(); }
    GuildIntegration* GetGuildIntegration() const { return _guildIntegration.get(); }
    LootDistribution* GetLootDistribution() const override { return _lootDistribution.get(); }
    TradeSystem* GetTradeSystem() const { return _tradeSystem.get(); }
    DynamicQuestSystem* GetDynamicQuestSystem() const override { return _dynamicQuestSystem.get(); }
    ObjectiveTracker* GetObjectiveTracker() const override { return _objectiveTracker.get(); }
    QuestCompletion* GetQuestCompletion() const override { return _questCompletion.get(); }
    QuestPickup* GetQuestPickup() const override { return _questPickup.get(); }
    QuestTurnIn* GetQuestTurnIn() const override { return _questTurnIn.get(); }
    QuestValidation* GetQuestValidation() const override { return _questValidation.get(); }
    RoleAssignment* GetRoleAssignment() const { return _roleAssignment.get(); }
    // Note: LFGBotManager is a global singleton, use sLFGBotManager macro instead
    // LFGBotManager* GetLFGBotManager() const { return _lfgBotManager.get(); }
    // Note: LFGBotSelector is a global singleton, use sLFGBotSelector macro instead
    // LFGBotSelector* GetLFGBotSelector() const { return _lfgBotSelector.get(); }
    // Note: LFGGroupCoordinator is a global singleton, use sLFGGroupCoordinator macro instead
    // LFGGroupCoordinator* GetLFGGroupCoordinator() const { return _lfgGroupCoordinator.get(); }
    // Note: InstanceCoordination is a global singleton, use instance() instead
    // InstanceCoordination* GetInstanceCoordination() const { return _instanceCoordination.get(); }
    // Note: BotPriorityManager is a global singleton, use sBotPriorityMgr macro instead
    // BotPriorityManager* GetBotPriorityManager() const { return _botPriorityManager.get(); }
    // Note: BotWorldSessionMgr is a global singleton, use sBotWorldSessionMgr macro instead
    // BotWorldSessionMgr* GetBotWorldSessionMgr() const { return _botWorldSessionMgr.get(); }
    BotLifecycleManager* GetBotLifecycleManager() const { return _botLifecycleManager.get(); }
    Advanced::GroupCoordinator* GetGroupCoordinator() const override { return _groupCoordinator.get(); }
    DeathRecoveryManager* GetDeathRecoveryManager() const override { return _deathRecoveryManager.get(); }
    UnifiedMovementCoordinator* GetMovementCoordinator() const override { return _unifiedMovementCoordinator.get(); }
    CombatStateManager* GetCombatStateManager() const override { return _combatStateManager.get(); }

    // Decision systems
    bot::ai::DecisionFusionSystem* GetDecisionFusion() const override { return _decisionFusion.get(); }
    bot::ai::ActionPriorityQueue* GetActionPriorityQueue() const override { return _actionPriorityQueue.get(); }
    bot::ai::BehaviorTree* GetBehaviorTree() const override { return _behaviorTree.get(); }

    // Event system
    Events::EventDispatcher* GetEventDispatcher() const override { return _eventDispatcher.get(); }
    ManagerRegistry* GetManagerRegistry() const override { return _managerRegistry.get(); }

    // Helper systems
    TargetScanner* GetTargetScanner() const override { return _targetScanner.get(); }
    GroupInvitationHandler* GetGroupInvitationHandler() const override { return _groupInvitationHandler.get(); }
    HybridAIController* GetHybridAI() const override { return _hybridAI.get(); }
    BehaviorPriorityManager* GetPriorityManager() const override { return _priorityManager.get(); }

    // Sprint 3: Combat Coordination
    CombatCoordinationIntegrator* GetCombatCoordinationIntegrator() const override { return _combatCoordinationIntegrator.get(); }

    // Combat Behaviors managers
    AoEDecisionManager* GetAoEDecisionManager() const override { return _aoeDecisionManager.get(); }
    CooldownStackingOptimizer* GetCooldownStackingOptimizer() const override { return _cooldownStackingOptimizer.get(); }
    DefensiveBehaviorManager* GetDefensiveBehaviorManager() const override { return _defensiveBehaviorManager.get(); }
    DispelCoordinator* GetDispelCoordinator() const override { return _dispelCoordinator.get(); }
    InterruptRotationManager* GetInterruptRotationManager() const override { return _interruptRotationManager.get(); }

    // Game Systems
    ConsumableManager* GetConsumableManager() const override { return _consumableManager.get(); }

    // ========================================================================
    // RPG-STATE AI BUDGET TIER - Manager-level scope gating
    // ========================================================================
    void SetBudgetTier(AIBudgetTier tier) { _budgetTier = tier; }
    AIBudgetTier GetBudgetTier() const { return _budgetTier; }

private:
    // ========================================================================
    // MANAGER INSTANCES - All 26 managers owned by facade
    // ========================================================================

    // Core game systems
    std::unique_ptr<TradeManager> _tradeManager;
    std::unique_ptr<GatheringManager> _gatheringManager;
    std::unique_ptr<ProfessionManager> _professionManager;
    std::unique_ptr<GatheringMaterialsBridge> _gatheringMaterialsBridge;
    std::unique_ptr<AuctionMaterialsBridge> _auctionMaterialsBridge;
    std::unique_ptr<ProfessionAuctionBridge> _professionAuctionBridge;
    std::unique_ptr<FarmingCoordinator> _farmingCoordinator;
    std::unique_ptr<AuctionManager> _auctionManager;
    std::unique_ptr<BankingManager> _bankingManager;
    std::unique_ptr<EquipmentManager> _equipmentManager;
    std::unique_ptr<MountManager> _mountManager;
    std::unique_ptr<RidingManager> _ridingManager;
    std::unique_ptr<BattlePetManager> _battlePetManager;
    std::unique_ptr<Humanization::HumanizationManager> _humanizationManager;
    std::unique_ptr<ArenaAI> _arenaAI;
    std::unique_ptr<PvPCombatAI> _pvpCombatAI;
    std::unique_ptr<AuctionHouse> _auctionHouse;
    std::unique_ptr<GuildBankManager> _guildBankManager;
    std::unique_ptr<GuildEventCoordinator> _guildEventCoordinator;
    std::unique_ptr<GuildIntegration> _guildIntegration;
    std::unique_ptr<LootDistribution> _lootDistribution;
    std::unique_ptr<TradeSystem> _tradeSystem;
    std::unique_ptr<DynamicQuestSystem> _dynamicQuestSystem;
    std::unique_ptr<ObjectiveTracker> _objectiveTracker;
    std::unique_ptr<QuestCompletion> _questCompletion;
    std::unique_ptr<QuestPickup> _questPickup;
    std::unique_ptr<QuestTurnIn> _questTurnIn;
    std::unique_ptr<QuestValidation> _questValidation;
    std::unique_ptr<RoleAssignment> _roleAssignment;
    // Note: These are global singletons - use their respective macros instead
    // std::unique_ptr<LFGBotManager> _lfgBotManager;
    // std::unique_ptr<LFGBotSelector> _lfgBotSelector;
    // std::unique_ptr<LFGGroupCoordinator> _lfgGroupCoordinator;
    // std::unique_ptr<InstanceCoordination> _instanceCoordination;
    // std::unique_ptr<BotPriorityManager> _botPriorityManager;
    // std::unique_ptr<BotWorldSessionMgr> _botWorldSessionMgr;
    std::unique_ptr<BotLifecycleManager> _botLifecycleManager;
    std::unique_ptr<Advanced::GroupCoordinator> _groupCoordinator;

    // Lifecycle systems
    std::unique_ptr<DeathRecoveryManager> _deathRecoveryManager;

    // Movement system
    std::unique_ptr<UnifiedMovementCoordinator> _unifiedMovementCoordinator;

    // Combat systems
    std::unique_ptr<CombatStateManager> _combatStateManager;
    std::unique_ptr<TargetScanner> _targetScanner;

    // Group systems
    std::unique_ptr<GroupInvitationHandler> _groupInvitationHandler;

    // Event and registry systems
    std::unique_ptr<Events::EventDispatcher> _eventDispatcher;
    std::unique_ptr<ManagerRegistry> _managerRegistry;

    // Decision systems
    std::unique_ptr<bot::ai::DecisionFusionSystem> _decisionFusion;
    std::unique_ptr<bot::ai::ActionPriorityQueue> _actionPriorityQueue;
    std::unique_ptr<bot::ai::BehaviorTree> _behaviorTree;
    std::unique_ptr<HybridAIController> _hybridAI;  // Note: HybridAIController is in Playerbot::, not Playerbot::bot::ai::

    // Behavior management
    std::unique_ptr<BehaviorPriorityManager> _priorityManager;

    // Sprint 3: Combat Coordination
    std::unique_ptr<CombatCoordinationIntegrator> _combatCoordinationIntegrator;

    // Combat Behaviors managers
    std::unique_ptr<AoEDecisionManager> _aoeDecisionManager;
    std::unique_ptr<CooldownStackingOptimizer> _cooldownStackingOptimizer;
    std::unique_ptr<DefensiveBehaviorManager> _defensiveBehaviorManager;
    std::unique_ptr<DispelCoordinator> _dispelCoordinator;
    std::unique_ptr<InterruptRotationManager> _interruptRotationManager;

    // Game Systems
    std::unique_ptr<ConsumableManager> _consumableManager;

    // ========================================================================
    // INTERNAL STATE
    // ========================================================================

    Player* _bot;                   // Bot player reference (non-owning)
    BotAI* _botAI;                  // BotAI reference (non-owning, needed for initialization)
    bool _initialized{false};       // Initialization state flag
    AIBudgetTier _budgetTier{AIBudgetTier::FULL}; // RPG-state budget tier for manager gating

    // Update throttling timers (moved from BotAI)
    uint32 _equipmentCheckTimer{0};
    uint32 _professionCheckTimer{0};
    uint32 _bankingCheckTimer{0};
    uint32 _debugLogAccumulator{0};

    // PERFORMANCE FIX: Additional throttle timers to reduce per-tick workload
    // Many managers were updating EVERY FRAME causing 1000+ operations/second
    uint32 _mountUpdateTimer{0};           // 200ms - responsive but not every frame
    uint32 _ridingUpdateTimer{0};          // 5000ms - skill learning is rare
    uint32 _battlePetUpdateTimer{0};       // 500ms - pet AI doesn't need 60fps
    uint32 _arenaAIUpdateTimer{0};         // 100ms - fast for PvP responsiveness
    uint32 _pvpCombatUpdateTimer{0};       // 100ms - fast for PvP responsiveness
    uint32 _auctionUpdateTimer{0};         // 5000ms - AH operations are slow anyway
    uint32 _gatheringBridgeTimer{0};       // 2000ms - gathering coordination
    uint32 _auctionBridgeTimer{0};         // 2000ms - material sourcing
    uint32 _professionBridgeTimer{0};      // 5000ms - selling/buying materials
    uint32 _farmingUpdateTimer{0};         // 2000ms - farming coordination

    // Sprint 3: Combat Coordination throttle timers
    uint32 _combatCoordTimer{0};           // 100ms - responsive coordination
    uint32 _dispelTimer{0};                // 200ms - dispel rotation
    uint32 _interruptTimer{0};             // 100ms - fast interrupt response
    uint32 _aoeTimer{0};                   // 500ms - target clustering
    uint32 _cdStackTimer{0};               // 500ms - cooldown optimization
    uint32 _defenseTimer{0};               // 200ms - defensive coordination

    // ========================================================================
    // HELPER METHODS
    // ========================================================================

    /**
     * @brief Initialize Hybrid AI system (Decision Fusion + Behavior Tree)
     * Separated from main initialization for clarity
     */
    void InitializeHybridAI();

    /**
     * @brief Subscribe managers to EventDispatcher
     * Handles all event type registrations
     */
    void SubscribeManagersToEvents();

    /**
     * @brief Update all managers that require per-frame processing
     * @param diff Time delta in milliseconds
     */
    void UpdateManagers(uint32 diff);

    // Non-copyable
    GameSystemsManager(GameSystemsManager const&) = delete;
    GameSystemsManager& operator=(GameSystemsManager const&) = delete;
};

} // namespace Playerbot

#endif // PLAYERBOT_GAME_SYSTEMS_MANAGER_H
