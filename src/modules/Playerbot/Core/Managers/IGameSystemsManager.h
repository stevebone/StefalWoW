/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * IGAME SYSTEMS MANAGER - Facade Pattern for Bot Manager Instances
 *
 * Phase 6: BotAI Decoupling & Final Cleanup
 *
 * PURPOSE:
 * This facade interface consolidates all 17 manager instances from BotAI,
 * reducing the god class from 73 direct dependencies to ~10, improving
 * testability and maintainability.
 *
 * ARCHITECTURE:
 * - BotAI owns single IGameSystemsManager facade instance
 * - Facade owns all 17 manager unique_ptrs
 * - Facade returns raw pointers (callers don't own)
 * - BotAI provides delegation getters for backward compatibility
 *
 * BENEFITS:
 * - Testability: Facade can be mocked for unit tests
 * - Maintainability: Manager changes isolated to facade
 * - Extensibility: Add managers without touching BotAI
 * - Dependency Injection: Interface enables DI patterns
 * - Single Responsibility: BotAI focuses on AI logic only
 *
 * OWNERSHIP MODEL:
 * GameSystemsManager (facade) OWNS all managers via unique_ptr
 * BotAI OWNS the facade via unique_ptr<IGameSystemsManager>
 * External code receives raw pointers (non-owning references)
 *
 * LIFECYCLE:
 * 1. BotAI constructs facade: _gameSystems = std::make_unique<GameSystemsManager>(_bot)
 * 2. BotAI calls Initialize(): _gameSystems->Initialize(_bot)
 * 3. BotAI calls Update() each frame: _gameSystems->Update(diff)
 * 4. BotAI destruction auto-destroys facade (and all managers)
 */

#ifndef PLAYERBOT_IGAME_SYSTEMS_MANAGER_H
#define PLAYERBOT_IGAME_SYSTEMS_MANAGER_H

#include "Define.h"
#include <cstdint>

// Forward declarations
class Player;

namespace Playerbot
{

// Forward declarations for all manager types
class ObjectiveTracker;
class QuestCompletion;
class QuestPickup;
class QuestTurnIn;
class QuestValidation;
class DynamicQuestSystem;
class TradeManager;
class GatheringManager;
class ProfessionManager;
class AuctionManager;
class BankingManager;
class EquipmentManager;
class LootDistribution;
class DeathRecoveryManager;
class UnifiedMovementCoordinator;
class CombatStateManager;
class TargetScanner;
class GroupInvitationHandler;
class ManagerRegistry;
class HybridAIController;
class BehaviorPriorityManager;

// Combat Coordination (Sprint 3)
class CombatCoordinationIntegrator;

// Combat Behaviors managers
class AoEDecisionManager;
class CooldownStackingOptimizer;
class DefensiveBehaviorManager;
class DispelCoordinator;
class InterruptRotationManager;

// Game Systems
class ConsumableManager;

namespace Advanced
{
    class GroupCoordinator;
}

namespace Events
{
    class EventDispatcher;
}

namespace bot::ai
{
    class DecisionFusionSystem;
    class ActionPriorityQueue;
    class BehaviorTree;
    class HybridAIController;
}

/**
 * @brief Facade interface for all bot game system managers
 *
 * Provides unified access to all 17 manager instances, reducing BotAI's
 * direct dependencies from 73 files to ~10 and improving testability.
 *
 * **Phase 6 Refactoring Goal:**
 * Extract manager instances from BotAI god class into this facade,
 * reducing BotAI.h from 924 lines to ~450 lines (51% reduction).
 *
 * **Implementation Note:**
 * See GameSystemsManager class for concrete implementation.
 */
class TC_GAME_API IGameSystemsManager
{
public:
    virtual ~IGameSystemsManager() = default;

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /**
     * @brief Initialize all game system managers
     * @param bot The bot player this manager collection serves
     *
     * Called once during BotAI construction after facade is created.
     * Initializes all 17 manager instances in correct dependency order.
     */
    virtual void Initialize(Player* bot) = 0;

    /**
     * @brief Shutdown all game system managers
     *
     * Called during BotAI destruction to gracefully cleanup managers.
     * Shuts down in reverse dependency order.
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Update all game system managers
     * @param diff Time elapsed since last update (milliseconds)
     *
     * Called every frame from BotAI::UpdateAI().
     * Updates all managers that require per-frame processing.
     */
    virtual void Update(uint32 diff) = 0;

    // ========================================================================
    // GAME SYSTEM ACCESS (Core Managers)
    // ========================================================================

    /**
     * @brief Get objective tracker for quest progress
     * @return Non-owning pointer to ObjectiveTracker (owned by facade)
     */
    virtual ObjectiveTracker* GetObjectiveTracker() const = 0;

    /**
     * @brief Get quest completion system
     * @return Non-owning pointer to QuestCompletion (owned by facade)
     */
    virtual QuestCompletion* GetQuestCompletion() const = 0;

    /**
     * @brief Get quest pickup system
     * @return Non-owning pointer to QuestPickup (owned by facade)
     */
    virtual QuestPickup* GetQuestPickup() const = 0;

    /**
     * @brief Get quest turn-in system
     * @return Non-owning pointer to QuestTurnIn (owned by facade)
     */
    virtual QuestTurnIn* GetQuestTurnIn() const = 0;

    /**
     * @brief Get dynamic quest system
     * @return Non-owning pointer to DynamicQuestSystem (owned by facade)
     */
    virtual DynamicQuestSystem* GetDynamicQuestSystem() const = 0;

    /**
     * @brief Get quest validation system
     * @return Non-owning pointer to QuestValidation (owned by facade)
     */
    virtual QuestValidation* GetQuestValidation() const = 0;

    /**
     * @brief Get trade management system
     * @return Non-owning pointer to TradeManager (owned by facade)
     */
    virtual TradeManager* GetTradeManager() const = 0;

    /**
     * @brief Get loot distribution system
     * @return Non-owning pointer to LootDistribution (owned by facade)
     */
    virtual LootDistribution* GetLootDistribution() const = 0;

    /**
     * @brief Get gathering management system
     * @return Non-owning pointer to GatheringManager (owned by facade)
     */
    virtual GatheringManager* GetGatheringManager() const = 0;

    /**
     * @brief Get profession management system
     * @return Non-owning pointer to ProfessionManager (owned by facade)
     */
    virtual ProfessionManager* GetProfessionManager() const = 0;

    /**
     * @brief Get auction house management system
     * @return Non-owning pointer to AuctionManager (owned by facade)
     */
    virtual AuctionManager* GetAuctionManager() const = 0;

    /**
     * @brief Get banking management system
     * @return Non-owning pointer to BankingManager (owned by facade)
     */
    virtual BankingManager* GetBankingManager() const = 0;

    /**
     * @brief Get equipment management system
     * @return Non-owning pointer to EquipmentManager (owned by facade)
     */
    virtual EquipmentManager* GetEquipmentManager() const = 0;

    /**
     * @brief Get group coordination system
     * @return Non-owning pointer to GroupCoordinator (owned by facade)
     */
    virtual Advanced::GroupCoordinator* GetGroupCoordinator() const = 0;

    /**
     * @brief Get death recovery management system
     * @return Non-owning pointer to DeathRecoveryManager (owned by facade)
     */
    virtual DeathRecoveryManager* GetDeathRecoveryManager() const = 0;

    /**
     * @brief Get unified movement coordination system
     * @return Non-owning pointer to UnifiedMovementCoordinator (owned by facade)
     */
    virtual UnifiedMovementCoordinator* GetMovementCoordinator() const = 0;

    /**
     * @brief Get combat state management system
     * @return Non-owning pointer to CombatStateManager (owned by facade)
     */
    virtual CombatStateManager* GetCombatStateManager() const = 0;

    // ========================================================================
    // DECISION SYSTEMS
    // ========================================================================

    /**
     * @brief Get decision fusion system
     * @return Non-owning pointer to DecisionFusionSystem (owned by facade)
     */
    virtual bot::ai::DecisionFusionSystem* GetDecisionFusion() const = 0;

    /**
     * @brief Get action priority queue system
     * @return Non-owning pointer to ActionPriorityQueue (owned by facade)
     */
    virtual bot::ai::ActionPriorityQueue* GetActionPriorityQueue() const = 0;

    /**
     * @brief Get behavior tree system
     * @return Non-owning pointer to BehaviorTree (owned by facade)
     */
    virtual bot::ai::BehaviorTree* GetBehaviorTree() const = 0;

    // ========================================================================
    // EVENT SYSTEM
    // ========================================================================

    /**
     * @brief Get event dispatcher
     * @return Non-owning pointer to EventDispatcher (owned by facade)
     */
    virtual Events::EventDispatcher* GetEventDispatcher() const = 0;

    /**
     * @brief Get manager registry
     * @return Non-owning pointer to ManagerRegistry (owned by facade)
     */
    virtual ManagerRegistry* GetManagerRegistry() const = 0;

    // ========================================================================
    // HELPER SYSTEMS
    // ========================================================================

    /**
     * @brief Get target scanning system
     * @return Non-owning pointer to TargetScanner (owned by facade)
     */
    virtual TargetScanner* GetTargetScanner() const = 0;

    /**
     * @brief Get group invitation handler
     * @return Non-owning pointer to GroupInvitationHandler (owned by facade)
     */
    virtual GroupInvitationHandler* GetGroupInvitationHandler() const = 0;

    /**
     * @brief Get hybrid AI controller
     * @return Non-owning pointer to HybridAIController (owned by facade)
     */
    virtual HybridAIController* GetHybridAI() const = 0;

    /**
     * @brief Get behavior priority manager
     * @return Non-owning pointer to BehaviorPriorityManager (owned by facade)
     */
    virtual BehaviorPriorityManager* GetPriorityManager() const = 0;

    // ========================================================================
    // COMBAT COORDINATION (Sprint 3)
    // ========================================================================

    /**
     * @brief Get combat coordination integrator
     * Bridges combat managers with BotMessageBus claim system
     * @return Non-owning pointer to CombatCoordinationIntegrator (owned by facade)
     */
    virtual CombatCoordinationIntegrator* GetCombatCoordinationIntegrator() const = 0;

    // ========================================================================
    // COMBAT BEHAVIORS MANAGERS
    // ========================================================================

    /**
     * @brief Get AoE decision manager
     * Target clustering and DoT spread decisions
     * @return Non-owning pointer to AoEDecisionManager (owned by facade)
     */
    virtual AoEDecisionManager* GetAoEDecisionManager() const = 0;

    /**
     * @brief Get cooldown stacking optimizer
     * 12-class cooldown database for optimal CD stacking
     * @return Non-owning pointer to CooldownStackingOptimizer (owned by facade)
     */
    virtual CooldownStackingOptimizer* GetCooldownStackingOptimizer() const = 0;

    /**
     * @brief Get defensive behavior manager
     * External defensive CD coordination (GAP 3 fix)
     * @return Non-owning pointer to DefensiveBehaviorManager (owned by facade)
     */
    virtual DefensiveBehaviorManager* GetDefensiveBehaviorManager() const = 0;

    /**
     * @brief Get dispel coordinator
     * Dispel rotation and claim system (GAP 2 fix)
     * @return Non-owning pointer to DispelCoordinator (owned by facade)
     */
    virtual DispelCoordinator* GetDispelCoordinator() const = 0;

    /**
     * @brief Get interrupt rotation manager
     * Interrupt assignment and rotation queue
     * @return Non-owning pointer to InterruptRotationManager (owned by facade)
     */
    virtual InterruptRotationManager* GetInterruptRotationManager() const = 0;

    // ========================================================================
    // GAME SYSTEMS
    // ========================================================================

    /**
     * @brief Get consumable manager
     * Pre-combat buffing (flasks, food, runes) and combat potion usage
     * @return Non-owning pointer to ConsumableManager (owned by facade)
     */
    virtual ConsumableManager* GetConsumableManager() const = 0;
};

} // namespace Playerbot

#endif // PLAYERBOT_IGAME_SYSTEMS_MANAGER_H
