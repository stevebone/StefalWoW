/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * REFACTORED BotAI - Clean Update Chain Architecture
 *
 * This refactored version provides:
 * 1. Single, clean update path without DoUpdateAI/UpdateEnhanced confusion
 * 2. Clear separation between base behaviors and combat specialization
 * 3. No throttling that breaks movement/following
 * 4. Proper virtual method hierarchy for class-specific overrides
 */

#pragma once

#include "Define.h"
#include "AIBudgetTier.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "Actions/Action.h"
#include "Triggers/Trigger.h"
#include "Strategy/Strategy.h"
#include "ObjectCache.h"
#include "Blackboard/SharedBlackboard.h"
#include "Core/Events/IEventHandler.h"
#include "Core/Managers/IGameSystemsManager.h"
#include "Advanced/GroupCoordinator.h"
#include "Movement/BotMovement/Core/BotMovementController.h"
#include "Movement/BotMovement/Core/BotMovementManager.h"
#include <memory>
#include <vector>
#include <string>
#include <atomic>
#include <unordered_map>
#include <chrono>
#include <queue>
#include <shared_mutex>

namespace Playerbot
{

// Forward declarations
class BehaviorPriorityManager;
enum class BehaviorPriority : uint8_t;
class Value;
class GroupInvitationHandler;
class TargetScanner;
class QuestManager;
class TradeManager;
class GatheringManager;
class AuctionManager;
class GroupCoordinator; // Advanced/GroupCoordinator
class DeathRecoveryManager;
class UnifiedMovementCoordinator; // Phase 2: Unified Movement System (Week 3 complete)
class QuestCompletion;            // Quest completion system (Phase 0: Hook integration)
class CombatStateManager;
namespace Humanization { class RPGDailyRoutineManager; }
enum class PlayerBotMovementPriority : uint8;

// Two-Phase AddToWorld Lifecycle Management (Initialization Lifecycle)
class BotInitStateManager;
enum class BotInitState : uint8;

// Phase 3: Tactical Coordination forward declarations
namespace Advanced {
    class TacticalCoordinator;
}

// Phase 3: Bot-to-bot messaging forward declaration
struct BotMessage;

// Phase 4: Event structure forward declarations
struct GroupEvent;
struct CombatEvent;
struct CooldownEvent;
struct AuraEvent;
struct LootEvent;
struct QuestEvent;
struct ResourceEvent;
struct SocialEvent;
struct AuctionEvent;
struct NPCEvent;
struct InstanceEvent;
struct ProfessionEvent;

namespace Events
{
    class CombatEventObserver;
    class AuraEventObserver;
    class ResourceEventObserver;
    class QuestEventObserver;
    class MovementEventObserver;
    class TradeEventObserver;
    class SocialEventObserver;
    class InstanceEventObserver;
    class PvPEventObserver;
    class EventDispatcher;
}

class ManagerRegistry;
class HybridAIController;
class AdaptiveAIUpdateThrottler; // ST-1: Adaptive AI Update Throttling

// Phase 5E: Decision Fusion System forward declarations
namespace bot { namespace ai {
    class DecisionFusionSystem;
    class ActionPriorityQueue;
    class BehaviorTree;
}}

// TriggerResult comparator for priority queue
struct TriggerResultComparator
{
    bool operator()(TriggerResult const& a, TriggerResult const& b) const;
};

// Enhanced AI state enum
enum class BotAIState
{
    SOLO,           // Solo play mode (questing, gathering, autonomous combat)
    COMBAT,
    DEAD,
    TRAVELLING,
    QUESTING,
    GATHERING,
    TRADING,
    FOLLOWING,
    FLEEING,
    RESTING
};

// AI update result for performance tracking
struct AIUpdateResult
{
    uint32 actionsExecuted = 0;
    uint32 triggersChecked = 0;
    uint32 strategiesEvaluated = 0;
    std::chrono::microseconds updateTime{0};
};

class TC_GAME_API BotAI : public IEventHandler<LootEvent>,
                           public IEventHandler<QuestEvent>,
                           public IEventHandler<CombatEvent>,
                           public IEventHandler<CooldownEvent>,
                           public IEventHandler<AuraEvent>,
                           public IEventHandler<ResourceEvent>,
                           public IEventHandler<SocialEvent>,
                           public IEventHandler<AuctionEvent>,
                           public IEventHandler<NPCEvent>,
                           public IEventHandler<InstanceEvent>,
                           public IEventHandler<GroupEvent>,
                           public IEventHandler<ProfessionEvent>
{
public:
    /**
     * @brief Construct BotAI for a player bot
     * @param bot The player bot to control
     * @param instanceOnlyMode If true, creates a lightweight bot for instances only
     *        that skips expensive non-essential managers (questing, professions, AH).
     *        Used for JIT bots created to fill BG/LFG queues.
     */
    explicit BotAI(Player* bot, bool instanceOnlyMode = false);
    virtual ~BotAI();

    // ========================================================================
    // CLEAN UPDATE INTERFACE - Single entry point, no confusion
    // ========================================================================

    /**
     * Main update method - SINGLE ENTRY POINT for all AI updates
     * This is the ONLY method called by BotSession::Update()
     *
     * Update flow:
     * 1. Update core behaviors (strategies, movement, idle)
     * 2. Check combat state transitions
     * 3. If in combat AND derived class exists, call OnCombatUpdate()
     *
     * CRITICAL: This method is NOT throttled and runs every frame
     * to ensure smooth movement and responsive behavior
     */
    virtual void UpdateAI(uint32 diff);

    /**
     * Virtual method for class-specific COMBAT ONLY updates
     * Called by UpdateAI() when bot is in combat
     *
     * ClassAI implementations should override this for:
     * - Combat rotations
     * - Target selection
     * - Cooldown management
     * - Resource management
     *
     * MUST NOT:
     * - Control movement (handled by strategies)
     * - Throttle updates (causes following issues)
     * - Call base UpdateAI (would cause recursion)
     */
    virtual void OnCombatUpdate(uint32 /*diff*/) {}

    /**
     * Virtual method for class-specific NON-COMBAT updates
     * Called by UpdateAI() when bot is NOT in combat
     *
     * ClassAI implementations should override this for:
     * - Pet summoning and management
     * - Buff application
     * - Out-of-combat resource regeneration
     * - Preparation for combat
     *
     * MUST NOT:
     * - Control movement (handled by strategies)
     * - Throttle updates (causes following issues)
     * - Call base UpdateAI (would cause recursion)
     */
    virtual void OnNonCombatUpdate(uint32 /*diff*/) {}

    // ========================================================================
    // STATE TRANSITIONS - Clean lifecycle management
    // ========================================================================

    virtual void Reset();
    virtual void OnDeath();
    virtual void OnRespawn();
    virtual void OnCombatStart(::Unit* target);
    virtual void OnCombatEnd();

    // ========================================================================
    // SPELL CASTING - Virtual interface for class-specific implementations
    // ========================================================================

    /**
     * @brief Cast a spell on a target
     *
     * Base implementation returns SPELL_FAILED_NOT_READY.
     * ClassAI overrides this to provide class-specific spell casting logic.
     *
     * @param spellId The spell ID to cast
     * @param target The target unit (nullptr for self-cast)
     * @return SpellCastResult indicating success or failure reason
     */
    virtual ::SpellCastResult CastSpell(uint32 spellId, ::Unit* target = nullptr);

    // ========================================================================
    // STRATEGY MANAGEMENT - Core behavior system
    // ========================================================================

    void AddStrategy(std::unique_ptr<Strategy> strategy);
    void RemoveStrategy(std::string const& name);
    Strategy* GetStrategy(std::string const& name) const;

    /**
     * @brief Get strategy with type-safe casting
     * @tparam T Strategy type to cast to
     * @param name Strategy name
     * @return Typed strategy pointer or nullptr if not found/wrong type
     */
    template<typename T>
    T* GetStrategy(std::string const& name) const
    {
        return dynamic_cast<T*>(GetStrategy(name));
    }

    std::vector<Strategy*> GetActiveStrategies() const;
    void ActivateStrategy(std::string const& name);
    void DeactivateStrategy(std::string const& name);

    // Priority-based strategy selection
    BehaviorPriorityManager* GetPriorityManager() { return _priorityManager.get(); }
    BehaviorPriorityManager const* GetPriorityManager() const { return _priorityManager.get(); }

    // ========================================================================
    // ACTION EXECUTION - Command pattern implementation
    // ========================================================================

    bool ExecuteAction(std::string const& actionName);
    bool ExecuteAction(std::string const& name, ActionContext const& context);
    bool IsActionPossible(std::string const& actionName) const;
    uint32 GetActionPriority(std::string const& actionName) const;

    void QueueAction(std::shared_ptr<Action> action, ActionContext const& context = {});
    void CancelCurrentAction();
    bool IsActionInProgress() const { return _currentAction != nullptr; }

    // ========================================================================
    // STATE MANAGEMENT - AI state tracking
    // ========================================================================

    BotAIState GetAIState() const { return _aiState; }
    void SetAIState(BotAIState state);
    bool IsInCombat() const { return _aiState == BotAIState::COMBAT; }
    bool IsSolo() const { return _aiState == BotAIState::SOLO; }
    bool IsFollowing() const { return _aiState == BotAIState::FOLLOWING; }

    // ========================================================================
    // INSTANCE-ONLY MODE - JIT bot optimization
    // ========================================================================

    /**
     * @brief Check if bot is in instance-only mode
     *
     * Instance-only bots skip expensive non-essential behaviors like:
     * - Questing (DynamicQuestSystem, ObjectiveTracker, QuestPickup, etc.)
     * - Professions (ProfessionManager, GatheringManager, FarmingCoordinator)
     * - Auction House (AuctionManager, AuctionHouse, AuctionMaterialsBridge)
     * - Banking (BankingManager, GuildBankManager)
     *
     * This significantly reduces CPU overhead for JIT bots that only exist
     * to fill BG/LFG queues and will be recycled after the instance ends.
     *
     * @return true if bot is in instance-only mode
     */
    bool IsInstanceOnlyMode() const { return _instanceOnlyMode; }

    /**
     * @brief Set instance-only mode for this bot
     *
     * IMPORTANT: This should be called BEFORE Initialize() is called on
     * the GameSystemsManager, as it affects which managers are created.
     *
     * @param enabled true to enable instance-only mode
     */
    void SetInstanceOnlyMode(bool enabled) { _instanceOnlyMode = enabled; }

    // ========================================================================
    // BOT ACCESS - Core entity access
    // ========================================================================

    Player* GetBot() const { return _bot; }
    ObjectGuid GetBotGuid() const { return _bot ? _bot->GetGUID() : ObjectGuid::Empty; }
    ObjectGuid GetCachedBotGuid() const { return _cachedBotGuid; }  // Safe during destructor

    // ========================================================================
    // LIFECYCLE MANAGEMENT - Two-Phase AddToWorld Pattern
    // ========================================================================

    /**
     * @brief Set the lifecycle manager for this bot
     * Called by BotFactory during staged initialization
     *
     * @param manager Pointer to lifecycle manager (owned by BotFactory)
     */
    void SetLifecycleManager(BotInitStateManager* manager) { _lifecycleManager = manager; }

    /**
     * @brief Get the lifecycle manager for this bot
     * @return Pointer to lifecycle manager, or nullptr if not managed
     */
    BotInitStateManager* GetLifecycleManager() { return _lifecycleManager; }
    BotInitStateManager const* GetLifecycleManager() const { return _lifecycleManager; }

    /**
     * @brief Check if bot lifecycle is in a state safe for player data access
     * @return true if state >= READY and state <= ACTIVE
     */
    bool IsLifecycleSafe() const;

    /**
     * @brief Check if bot is fully operational (ACTIVE state)
     * @return true if bot has completed initialization and is in ACTIVE state
     */
    bool IsLifecycleOperational() const;

    /**
     * @brief Get current lifecycle state
     * @return Current state, or FAILED if no lifecycle manager
     */
    BotInitState GetLifecycleState() const;

    // ========================================================================
    // GROUP MANAGEMENT - Social behavior
    // ========================================================================

    void OnGroupJoined(Group* group);
    void OnGroupLeft();
    void HandleGroupChange();
    GroupInvitationHandler* GetGroupInvitationHandler() { return _gameSystems ? _gameSystems->GetGroupInvitationHandler() : nullptr; }
    GroupInvitationHandler const* GetGroupInvitationHandler() const { return _gameSystems ? _gameSystems->GetGroupInvitationHandler() : nullptr; }

    // ========================================================================
    // TARGET MANAGEMENT - Combat targeting
    // ========================================================================

    void SetTarget(ObjectGuid guid) { _currentTarget = guid; }
    ObjectGuid GetTarget() const { return _currentTarget; }
    ::Unit* GetTargetUnit() const;
    TargetScanner* GetTargetScanner() { return _gameSystems ? _gameSystems->GetTargetScanner() : nullptr; }
    TargetScanner const* GetTargetScanner() const { return _gameSystems ? _gameSystems->GetTargetScanner() : nullptr; }

    // ========================================================================
    // MOVEMENT CONTROL - Strategy-driven movement
    // ========================================================================

    // Legacy movement methods (keep for backward compatibility)
    void MoveTo(float x, float y, float z);
    void Follow(::Unit* target, float distance = 5.0f);
    void StopMovement();
    bool IsMoving() const;

    // NEW: Movement System Integration - Validated pathfinding
    BotMovementController* GetMovementController() { return _movementController.get(); }
    BotMovementController const* GetMovementController() const { return _movementController.get(); }

    // Movement convenience methods with validation
    bool MoveTo(Position const& dest, bool validated = true);
    bool MoveToUnit(::Unit* target, float distance = 0.0f);
    bool IsMovementBlocked() const;
    bool IsStuck() const;

    // ========================================================================
    // GAME SYSTEM MANAGERS - Quest, profession, trade management (Phase 6: Delegation)
    // ========================================================================
    // All managers are owned by _gameSystems facade, not BotAI directly.

    /**
     * @brief Get the GameSystemsManager facade (Phase 6/7)
     * Provides direct access to the facade that owns all 48 per-bot managers.
     * @return Pointer to game systems manager, or nullptr if not initialized
     */
    IGameSystemsManager* GetGameSystems() { return _gameSystems.get(); }
    IGameSystemsManager const* GetGameSystems() const { return _gameSystems.get(); }

    // Quest helper methods - direct player data access (replaces Game/QuestManager)
    // These provide fast, simple quest state queries without the overhead of QuestManager
    uint32 GetActiveQuestCount() const;      // Count of active quests from player slots
    bool IsQuestingActive() const;           // True if bot has any active quests
    bool HasCompletableQuests() const;       // True if any quests are ready for turn-in
    std::vector<uint32> GetCompletableQuestIds() const;  // Quest IDs ready for turn-in

    TradeManager* GetTradeManager() { return _gameSystems ? _gameSystems->GetTradeManager() : nullptr; }
    TradeManager const* GetTradeManager() const { return _gameSystems ? _gameSystems->GetTradeManager() : nullptr; }

    GatheringManager* GetGatheringManager() { return _gameSystems ? _gameSystems->GetGatheringManager() : nullptr; }
    GatheringManager const* GetGatheringManager() const { return _gameSystems ? _gameSystems->GetGatheringManager() : nullptr; }

    AuctionManager* GetAuctionManager() { return _gameSystems ? _gameSystems->GetAuctionManager() : nullptr; }
    AuctionManager const* GetAuctionManager() const { return _gameSystems ? _gameSystems->GetAuctionManager() : nullptr; }

    // Returns GroupCoordinator pointer
    Advanced::GroupCoordinator* GetGroupCoordinator() { return _gameSystems ? _gameSystems->GetGroupCoordinator() : nullptr; }
    Advanced::GroupCoordinator const* GetGroupCoordinator() const { return _gameSystems ? _gameSystems->GetGroupCoordinator() : nullptr; }

    // Alias for backward compatibility
    Advanced::GroupCoordinator* GetGroupCoordinatorAdvanced() { return GetGroupCoordinator(); }
    Advanced::GroupCoordinator const* GetGroupCoordinatorAdvanced() const { return GetGroupCoordinator(); }

    /**
     * @brief Get Tactical Group Coordinator (Phase 3)
     * Combat-focused coordination separate from Advanced/GroupCoordinator
     * @return Pointer to tactical coordinator, or nullptr if not in group
     */
    Advanced::TacticalCoordinator* GetTacticalCoordinator()
    {
        auto gc = GetGroupCoordinatorAdvanced();
        return gc ? gc->GetTacticalCoordinator() : nullptr;
    }
    Advanced::TacticalCoordinator const* GetTacticalCoordinator() const
    {
        auto gc = GetGroupCoordinatorAdvanced();
        return gc ? gc->GetTacticalCoordinator() : nullptr;
    }

    // ========================================================================
    // UTILITY AI DECISION SYSTEM - Hybrid AI Phase 1
    // ========================================================================

    /**
     * @brief Initialize Hybrid AI Decision System (Phase 2 Week 3)
     * Creates HybridAIController with Utility AI + Behavior Trees
     * Called during BotAI constructor
     */
    void InitializeHybridAI();

    /**
     * @brief Update Hybrid AI (Phase 2 Week 3)
     * Selects best behavior via Utility AI, executes via Behavior Tree
     * @param diff Time since last UpdateAI() call
     */
    void UpdateHybridAI(uint32 diff);

    /**
     * @brief Get Hybrid AI Controller (Phase 2 Week 3 / Phase 6: Facade Delegation)
     * @return Pointer to controller, or nullptr if not initialized
     */
    HybridAIController* GetHybridAI() const { return _gameSystems ? _gameSystems->GetHybridAI() : nullptr; }

    /**
     * @brief Get Shared Blackboard (Phase 4)
     * @return Pointer to blackboard, or nullptr if not initialized
     */
    SharedBlackboard* GetSharedBlackboard() const { return _sharedBlackboard; }

    // ========================================================================
    // DEATH RECOVERY - Resurrection management (Phase 6: Facade Delegation)
    // ========================================================================

    DeathRecoveryManager* GetDeathRecoveryManager() { return _gameSystems ? _gameSystems->GetDeathRecoveryManager() : nullptr; }
    DeathRecoveryManager const* GetDeathRecoveryManager() const { return _gameSystems ? _gameSystems->GetDeathRecoveryManager() : nullptr; }

    // ========================================================================
    // QUEST COMPLETION - Phase 0 Hook Integration (Lifecycle management)
    // ========================================================================

    QuestCompletion* GetQuestCompletion() { return _gameSystems ? _gameSystems->GetQuestCompletion() : nullptr; }
    QuestCompletion const* GetQuestCompletion() const { return _gameSystems ? _gameSystems->GetQuestCompletion() : nullptr; }

    // ========================================================================
    // UNIFIED MOVEMENT COORDINATOR - Phase 2 Migration / Phase 6 Facade Delegation
    // ========================================================================
    // Primary movement system - consolidates MovementArbiter, CombatMovementStrategy,
    // GroupFormationManager, and MovementIntegration into unified interface.
    // Migration complete: All user code now uses UnifiedMovementCoordinator.

    UnifiedMovementCoordinator* GetUnifiedMovementCoordinator() { return _gameSystems ? _gameSystems->GetMovementCoordinator() : nullptr; }
    UnifiedMovementCoordinator const* GetUnifiedMovementCoordinator() const { return _gameSystems ? _gameSystems->GetMovementCoordinator() : nullptr; }

    /**
     * Request movement via the Movement Arbiter
     * This is the PREFERRED method for all movement requests
     *
     * @param request MovementRequest to submit for arbitration
     * @return true if request accepted, false if filtered (duplicate/low priority)
     *
     * Example:
     *   auto req = MovementRequest::MakePointMovement(
     *       PlayerBotMovementPriority::COMBAT_AI,
     *       targetPos, true, {}, {}, {},
     *       "Combat positioning", "WarriorAI");
     *   bot->GetBotAI()->RequestMovement(req);
     */
    bool RequestMovement(class MovementRequest const& request);

    /**
     * Convenience method: Request point movement
     *
     * @param priority Movement priority
     * @param position Target position
     * @param reason Debug description
     * @param sourceSystem Source system identifier
     * @return true if accepted
     */
    bool RequestPointMovement(
        PlayerBotMovementPriority priority,
        Position const& position,
        std::string const& reason = "",
        std::string const& sourceSystem = "");

    /**
     * Convenience method: Request chase movement
     *
     * @param priority Movement priority
     * @param target Target to chase
     * @param reason Debug description
     * @param sourceSystem Source system identifier
     * @return true if accepted
     */
    bool RequestChaseMovement(
        PlayerBotMovementPriority priority,
        ObjectGuid targetGuid,
        std::string const& reason = "",
        std::string const& sourceSystem = "");

    /**
     * Convenience method: Request follow movement
     *
     * @param priority Movement priority
     * @param target Target to follow
     * @param distance Follow distance
     * @param reason Debug description
     * @param sourceSystem Source system identifier
     * @return true if accepted
     */
    bool RequestFollowMovement(
        PlayerBotMovementPriority priority,
        ObjectGuid targetGuid,
        float distance = 5.0f,
        std::string const& reason = "",
        std::string const& sourceSystem = "");

    /**
     * Stop all movement immediately
     * Clears pending requests and stops current movement
     */
    void StopAllMovement();

    // ========================================================================
    // PHASE 7.1: EVENT DISPATCHER - Centralized event routing
    // ========================================================================

    Events::EventDispatcher* GetEventDispatcher() { return _gameSystems ? _gameSystems->GetEventDispatcher() : nullptr; }
    Events::EventDispatcher const* GetEventDispatcher() const { return _gameSystems ? _gameSystems->GetEventDispatcher() : nullptr; }

    // ========================================================================
    // SPRINT 3: COMBAT COORDINATION - Cross-bot coordination via claims
    // ========================================================================

    /**
     * @brief Get Combat Coordination Integrator
     * Bridges existing combat managers with BotMessageBus claim system
     * @return Pointer to CombatCoordinationIntegrator, or nullptr if not initialized
     */
    class CombatCoordinationIntegrator* GetCombatCoordinationIntegrator()
    {
        return _gameSystems ? _gameSystems->GetCombatCoordinationIntegrator() : nullptr;
    }
    class CombatCoordinationIntegrator const* GetCombatCoordinationIntegrator() const
    {
        return _gameSystems ? _gameSystems->GetCombatCoordinationIntegrator() : nullptr;
    }

    /**
     * @brief Get Dispel Coordinator for dispel rotation (GAP 2 fix)
     * @return Pointer to DispelCoordinator, or nullptr if not initialized
     */
    class DispelCoordinator* GetDispelCoordinator()
    {
        return _gameSystems ? _gameSystems->GetDispelCoordinator() : nullptr;
    }
    class DispelCoordinator const* GetDispelCoordinator() const
    {
        return _gameSystems ? _gameSystems->GetDispelCoordinator() : nullptr;
    }

    /**
     * @brief Get Interrupt Rotation Manager
     * @return Pointer to InterruptRotationManager, or nullptr if not initialized
     */
    class InterruptRotationManager* GetInterruptRotationManager()
    {
        return _gameSystems ? _gameSystems->GetInterruptRotationManager() : nullptr;
    }
    class InterruptRotationManager const* GetInterruptRotationManager() const
    {
        return _gameSystems ? _gameSystems->GetInterruptRotationManager() : nullptr;
    }

    /**
     * @brief Get Defensive Behavior Manager for external CD coordination (GAP 3 fix)
     * @return Pointer to DefensiveBehaviorManager, or nullptr if not initialized
     */
    class DefensiveBehaviorManager* GetDefensiveBehaviorManager()
    {
        return _gameSystems ? _gameSystems->GetDefensiveBehaviorManager() : nullptr;
    }
    class DefensiveBehaviorManager const* GetDefensiveBehaviorManager() const
    {
        return _gameSystems ? _gameSystems->GetDefensiveBehaviorManager() : nullptr;
    }

    // ========================================================================
    // RPG-STATE AI BUDGET TIER - Scope reduction per update
    // ========================================================================

    /**
     * @brief Get current AI budget tier
     * Controls which AI phases run each update based on RPG state.
     * FULL = all phases, REDUCED = movement+safety, MINIMAL = safety only.
     * Complements ST-1 throttler (frequency) with scope reduction.
     */
    AIBudgetTier GetCurrentBudgetTier() const { return _currentBudgetTier; }

    /**
     * @brief Called when the AI budget tier changes.
     * Handles save frequency adjustment (P6) and non-essential cache pruning (P5).
     */
    void OnBudgetTierTransition(AIBudgetTier oldTier, AIBudgetTier newTier);

    // ========================================================================
    // ST-1: ADAPTIVE AI UPDATE THROTTLING - Performance optimization
    // ========================================================================

    /**
     * @brief Get the adaptive AI update throttler
     * @return Throttler instance or nullptr if not initialized
     */
    AdaptiveAIUpdateThrottler* GetAIUpdateThrottler() { return _aiUpdateThrottler.get(); }
    AdaptiveAIUpdateThrottler const* GetAIUpdateThrottler() const { return _aiUpdateThrottler.get(); }

    /**
     * @brief Check if AI updates are currently throttled (for diagnostics)
     * @return true if in reduced update mode
     */
    bool IsAIUpdateThrottled() const;

    /**
     * @brief Get current throttle tier (0=FULL_RATE to 4=MINIMAL_RATE)
     * @return Throttle tier value for diagnostics
     */
    uint32 GetAIThrottleTier() const;

    // ========================================================================
    // PHASE 7.1: MANAGER REGISTRY - Manager lifecycle management
    // ========================================================================

    ManagerRegistry* GetManagerRegistry() { return _gameSystems ? _gameSystems->GetManagerRegistry() : nullptr; }
    ManagerRegistry const* GetManagerRegistry() const { return _gameSystems ? _gameSystems->GetManagerRegistry() : nullptr; }

    // ========================================================================
    // PHASE 5E: DECISION FUSION SYSTEM - Unified decision arbitration
    // ========================================================================

    /**
     * @brief Get Decision Fusion System for unified action selection
     *
     * The DecisionFusionSystem collects votes from all decision-making systems
     * (BehaviorPriorityManager, ActionScoringEngine, etc.) and fuses them
     * into a single, optimal action selection.
     *
     * @return Pointer to DecisionFusionSystem, or nullptr if not initialized
     *
     * Example usage:
     * @code
     * if (auto fusion = GetDecisionFusion())
     * {
     *     auto votes = fusion->CollectVotes(this, CombatContext::DUNGEON_BOSS);
     *     auto result = fusion->FuseDecisions(votes);
     *     if (result.IsValid())
     *         ExecuteAction(result.actionId, result.target);
     * }
     * @endcode
     */
    bot::ai::DecisionFusionSystem* GetDecisionFusion() { return _gameSystems ? _gameSystems->GetDecisionFusion() : nullptr; }
    bot::ai::DecisionFusionSystem const* GetDecisionFusion() const { return _gameSystems ? _gameSystems->GetDecisionFusion() : nullptr; }

    /**
     * @brief Get Action Priority Queue for spell priority management
     *
     * The ActionPriorityQueue manages spell priorities dynamically based on
     * cooldowns, resources, combat situations, and custom conditions.
     *
     * @return Pointer to ActionPriorityQueue, or nullptr if not initialized
     *
     * Example usage:
     * @code
     * if (auto queue = GetActionPriorityQueue())
     * {
     *     queue->RegisterSpell(FIREBALL, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
     *     queue->AddCondition(PYROBLAST, [](Player* bot, Unit*) {
     *         return bot->HasAura(HOT_STREAK);
     *     }, "Hot Streak proc");
     *
     *     uint32 bestSpell = queue->GetHighestPrioritySpell(GetBot(), target, context);
     *     if (bestSpell)
     *         CastSpell(target, bestSpell);
     * }
     * @endcode
     */
    bot::ai::ActionPriorityQueue* GetActionPriorityQueue() { return _gameSystems ? _gameSystems->GetActionPriorityQueue() : nullptr; }
    bot::ai::ActionPriorityQueue const* GetActionPriorityQueue() const { return _gameSystems ? _gameSystems->GetActionPriorityQueue() : nullptr; }

    /**
     * @brief Get Behavior Tree for hierarchical combat flow
     *
     * The BehaviorTree provides structured decision-making through hierarchical
     * node execution (Sequences, Selectors, Conditions, Actions).
     *
     * @return Pointer to BehaviorTree, or nullptr if not initialized
     *
     * Example usage:
     * @code
     * if (auto tree = GetBehaviorTree())
     * {
     *     // Execute one tick of the tree
     *     NodeStatus status = tree->Tick(GetBot(), target);
     *
     *     // Tree can span multiple ticks if RUNNING
     *     if (status == NodeStatus::RUNNING)
     *         return; // Continue next frame
     * }
     * @endcode
     */
    bot::ai::BehaviorTree* GetBehaviorTree() { return _gameSystems ? _gameSystems->GetBehaviorTree() : nullptr; }
    bot::ai::BehaviorTree const* GetBehaviorTree() const { return _gameSystems ? _gameSystems->GetBehaviorTree() : nullptr; }

    // ========================================================================
    // PHASE 4: EVENT HANDLERS - Event-driven behavior system
    // ========================================================================

    /**
     * Group event handler - Called when group-related events occur
     * Override in ClassAI for class-specific group coordination
     *
     * @param event Group event containing all event data
     *
     * Default implementation delegates to managers and handles common cases:
     * - Ready checks (auto-respond if configured)
     * - Raid target changes (update targeting)
     * - Leader changes (update following)
     */
    virtual void OnGroupEvent(GroupEvent const& event);

    /**
     * Combat event handler - Called when combat-related events occur
     * Override in ClassAI for combat-specific responses (interrupts, defensive CDs)
     *
     * @param event Combat event (spell casts, damage, interrupts)
     *
     * Default implementation:
     * - Detects interruptible casts
     * - Updates combat state
     * - Delegates to combat managers
     */
    virtual void OnCombatEvent(CombatEvent const& event);

    /**
     * Cooldown event handler - Called when spell/item cooldowns change
     * Override in ClassAI for ability rotation tracking
     *
     * @param event Cooldown event (start, clear, modify)
     */
    virtual void OnCooldownEvent(CooldownEvent const& event);

    /**
     * Aura event handler - Called when buffs/debuffs change
     * Override in ClassAI for buff management and dispel priorities
     *
     * @param event Aura event (applied, removed, updated)
     *
     * Default implementation:
     * - Tracks important debuffs for dispelling
     * - Monitors missing buffs for rebuffing
     */
    virtual void OnAuraEvent(AuraEvent const& event);

    /**
     * Loot event handler - Called when loot becomes available
     * Override in ClassAI for smart loot decisions
     *
     * @param event Loot event (window opened, roll started, item received)
     *
     * Default implementation:
     * - Auto-greed on non-upgrades
     * - Auto-need on class-appropriate gear
     * - Auto-pass on unusable items
     */
    virtual void OnLootEvent(LootEvent const& event);

    /**
     * Quest event handler - Called when quest state changes
     * Override in ClassAI for quest-specific behavior
     *
     * @param event Quest event (offered, completed, objective progress)
     *
     * Default implementation:
     * - Delegates to QuestManager
     * - Auto-accepts appropriate quests
     * - Auto-completes when ready
     */
    virtual void OnQuestEvent(QuestEvent const& event);

    /**
     * Resource event handler - Called when health/power changes
     * Override in ClassAI for healing/resource management
     *
     * @param event Resource event (health update, power update, target break)
     *
     * Default implementation:
     * - Detects low health allies for healing
     * - Monitors own resources for regeneration
     * - Updates healing priorities
     */
    virtual void OnResourceEvent(ResourceEvent const& event);

    /**
     * Social event handler - Called when social events occur
     * Override in ClassAI for chat responses and social behavior
     *
     * @param event Social event (chat, emotes, guild, trade)
     *
     * Default implementation:
     * - Processes whispers for commands
     * - Handles guild invites
     * - Responds to trade requests
     */
    virtual void OnSocialEvent(SocialEvent const& event);

    /**
     * Auction event handler - Called when auction house events occur
     * Override in ClassAI for AH participation strategy
     *
     * @param event Auction event (command result, list, bid, won, outbid)
     *
     * Default implementation:
     * - Delegates to AuctionManager
     * - Handles auction notifications
     */
    virtual void OnAuctionEvent(AuctionEvent const& event);

    /**
     * NPC event handler - Called when interacting with NPCs
     * Override in ClassAI for vendor/trainer automation
     *
     * @param event NPC event (gossip, vendor, trainer, bank)
     *
     * Default implementation:
     * - Auto-selects quest gossip options
     * - Handles vendor repairs
     * - Manages trainer interactions
     */
    virtual void OnNPCEvent(NPCEvent const& event);

    /**
     * Instance event handler - Called when instance/raid events occur
     * Override in ClassAI for dungeon/raid coordination
     *
     * @param event Instance event (reset, lockout, encounter, messages)
     *
     * Default implementation:
     * - Tracks instance lockouts
     * - Monitors boss progress
     * - Handles encounter mechanics
     */
    virtual void OnInstanceEvent(InstanceEvent const& event);

    /**
     * Profession event handler - Called when profession-related events occur
     * Override in ClassAI for class-specific profession handling
     *
     * @param event Profession event (crafting, learning, banking, materials)
     *
     * Default implementation:
     * - Handles recipe learning notifications
     * - Tracks crafting progress and completion
     * - Manages material gathering and purchasing
     * - Coordinates banking operations
     */
    virtual void OnProfessionEvent(ProfessionEvent const& event);

    // ========================================================================
    // IEVENTHANDLER INTERFACE IMPLEMENTATIONS (Phase 5)
    // ========================================================================
    // These methods implement the IEventHandler<T> interfaces and delegate
    // to the existing OnXxxEvent() methods for backward compatibility

    void HandleEvent(LootEvent const& event) override { OnLootEvent(event); }
    void HandleEvent(QuestEvent const& event) override { OnQuestEvent(event); }
    void HandleEvent(CombatEvent const& event) override { OnCombatEvent(event); }
    void HandleEvent(CooldownEvent const& event) override { OnCooldownEvent(event); }
    void HandleEvent(AuraEvent const& event) override { OnAuraEvent(event); }
    void HandleEvent(ResourceEvent const& event) override { OnResourceEvent(event); }
    void HandleEvent(SocialEvent const& event) override { OnSocialEvent(event); }
    void HandleEvent(AuctionEvent const& event) override { OnAuctionEvent(event); }
    void HandleEvent(NPCEvent const& event) override { OnNPCEvent(event); }
    void HandleEvent(InstanceEvent const& event) override { OnInstanceEvent(event); }
    void HandleEvent(GroupEvent const& event) override { OnGroupEvent(event); }
    void HandleEvent(ProfessionEvent const& event) override { OnProfessionEvent(event); }

    // ========================================================================
    // BOT-TO-BOT MESSAGING (Phase 3: Sprint 2 - BotMessageBus)
    // ========================================================================

    /**
     * @brief Handle a bot-to-bot message from BotMessageBus
     *
     * Called by BotMessageBus::DeliverMessage() when this bot receives
     * a message from another bot in the same group. Messages include
     * claims (interrupt/dispel/defensive CD), announcements (CD usage,
     * death, position), requests (heal, external CD), and commands
     * (focus target, spread, stack).
     *
     * Default implementation delegates to CombatCoordinationIntegrator
     * if available. ClassAI implementations may override for
     * class-specific message handling.
     *
     * @param message The bot message to handle
     */
    virtual void HandleBotMessage(BotMessage const& message);

    // ========================================================================
    // PERFORMANCE METRICS - Monitoring and optimization
    // ========================================================================

    struct PerformanceMetrics
    {
        std::atomic<uint32> totalUpdates{0};
        std::atomic<uint32> actionsExecuted{0};
        std::atomic<uint32> triggersProcessed{0};
        std::atomic<uint32> strategiesEvaluated{0};
        std::chrono::microseconds averageUpdateTime{0};
        std::chrono::microseconds maxUpdateTime{0};
        std::chrono::steady_clock::time_point lastUpdate;
    };

    PerformanceMetrics const& GetPerformanceMetrics() const { return _performanceMetrics; }

protected:
    // ========================================================================
    // INTERNAL UPDATE METHODS - Called by UpdateAI()
    // ========================================================================

    /**
     * Update all active strategies
     * CRITICAL: Must run every frame for following behavior
     */
    void UpdateStrategies(uint32 diff);

    /**
     * Update movement based on active strategies
     * CRITICAL: Must run every frame for smooth movement
     */
    void UpdateMovement(uint32 diff);

    /**
     * Update solo behaviors (questing, gathering, trading, autonomous combat)
     * Only runs when bot is in solo play mode (not in group or following)
     */
    void UpdateSoloBehaviors(uint32 diff);

    /**
     * Check and handle combat state transitions
     */
    void UpdateCombatState(uint32 diff);

    /**
     * Process all registered triggers
     */
    void ProcessTriggers();

    /**
     * Execute queued and triggered actions
     */
    void UpdateActions(uint32 diff);

    /**
     * Update internal values and caches
     */
    void UpdateValues(uint32 diff);

    // ========================================================================
    // HELPER METHODS - Utilities for derived classes
    // ========================================================================

    Strategy* SelectBestStrategy();
    std::shared_ptr<Action> SelectNextAction();
    bool CanExecuteAction(Action* action) const;
    ActionResult ExecuteActionInternal(Action* action, ActionContext const& context);

    void EvaluateTrigger(Trigger* trigger);
    void HandleTriggeredAction(TriggerResult const& result);

    void InitializeDefaultStrategies();
    void UpdatePerformanceMetrics(uint32 updateTimeMs);
    void LogAIDecision(std::string const& action, float score) const;

    // ========================================================================
    // EVENT HANDLER HELPERS - Common event processing patterns
    // ========================================================================

    void ProcessGroupReadyCheck(GroupEvent const& event);
    void ProcessCombatInterrupt(CombatEvent const& event);
    void ProcessLowHealthAlert(ResourceEvent const& event);
    void ProcessLootRoll(LootEvent const& event);
    void ProcessQuestProgress(QuestEvent const& event);
    void ProcessAuraDispel(AuraEvent const& event);

    // Combat entry helper for neutral mob detection
    void EnterCombatWithTarget(::Unit* target);

    // Event bus subscription management
    void SubscribeToEventBuses();
    void UnsubscribeFromEventBuses();

    // Deferred group validation (called from first UpdateAI, not constructor)
    void ValidateExistingGroupMembership();

protected:
    // Core components
    Player* _bot;
    ObjectGuid _cachedBotGuid;  // Cached at construction for safe destructor cleanup
    BotAIState _aiState = BotAIState::SOLO;
    ObjectGuid _currentTarget;

    // Strategy system
    std::unordered_map<std::string, std::unique_ptr<Strategy>> _strategies;
    std::vector<std::string> _activeStrategies;

    // Shared Blackboard: Thread-safe shared state system (Phase 4)
    SharedBlackboard* _sharedBlackboard = nullptr;

    // Action system
    std::queue<std::pair<std::shared_ptr<Action>, ActionContext>> _actionQueue;
    std::shared_ptr<Action> _currentAction;
    ActionContext _currentContext;

    // Trigger system
    std::vector<std::shared_ptr<Trigger>> _triggers;
    std::priority_queue<TriggerResult, std::vector<TriggerResult>, TriggerResultComparator> _triggeredActions;

    // Value cache
    std::unordered_map<std::string, float> _values;

    // Group management state
    bool _wasInGroup = false;

    // Solo strategy activation tracking
    bool _soloStrategiesActivated = false;

    // Login spell event cleanup tracking (prevents LOGINEFFECT crash)
    bool _firstUpdateComplete = false;

    // Group validation deferred to first UpdateAI (prevents ACCESS_VIOLATION in constructor)
    bool _groupValidationDone = false;

    // Two-Phase AddToWorld lifecycle tracking
    // Set true after first successful UpdateAI when lifecycle transitions to ACTIVE
    bool _lifecycleActivated = false;

    // Lifecycle manager (owned by BotFactory, not by BotAI)
    BotInitStateManager* _lifecycleManager = nullptr;

    // ========================================================================
    // INSTANCE-ONLY MODE - Optimized for JIT/instance bots
    // ========================================================================
    // When true, this bot is a JIT-created bot for instances only and skips
    // expensive non-essential behaviors like questing, professions, and AH.
    // This reduces server overhead significantly for bots that only exist
    // to fill BG/LFG queues and will be recycled after the instance.
    bool _instanceOnlyMode = false;

    // ========================================================================
    // PHASE 6: GAME SYSTEMS FACADE - Consolidates all 17 manager instances
    // ========================================================================
    // Previously scattered across BotAI: 17 manager unique_ptrs + timers
    // Now unified: Single facade owns all managers, provides delegation getters
    // Benefits: Testability, maintainability, reduced coupling (73 → ~10 deps)
    std::unique_ptr<IGameSystemsManager> _gameSystems;

    // Behavior priority management
    std::unique_ptr<BehaviorPriorityManager> _priorityManager;

    // ST-1: Adaptive AI Update Throttling - reduces CPU for bots far from human players
    std::unique_ptr<AdaptiveAIUpdateThrottler> _aiUpdateThrottler;

    // Movement System Integration - Validated pathfinding and state machine
    std::unique_ptr<BotMovementController> _movementController;

    // RPG Daily Routine - Autonomous daily activity simulation for masterless bots
    std::unique_ptr<Humanization::RPGDailyRoutineManager> _rpgRoutineManager;

    // Performance tracking
    mutable PerformanceMetrics _performanceMetrics;

    // CRITICAL FIX #19: ObjectAccessor Deadlock Resolution
    // Cache for all ObjectAccessor results to eliminate recursive TrinityCore
    // std::shared_mutex acquisitions. Refreshed once per UpdateAI() cycle.
    ObjectCache _objectCache;

    // Thread safety
    // DEADLOCK FIX #14: Changed from std::shared_mutex to std::recursive_mutex
    // After 13 failed fixes, the root cause is that std::shared_mutex does NOT
    // support recursive locking (even shared_lock). When the same thread tries
    // to acquire the lock twice (even read-only), it throws "resource deadlock would occur".
    //
    // Despite exhaustive analysis, we could not eliminate ALL possible recursive
    // lock acquisitions across the complex callback chain (strategies -> triggers ->
    // actions -> group handlers -> back to strategies). Some edge case is still
    // triggering recursive acquisition.
    //
    // Solution: Use std::recursive_mutex which ALLOWS the same thread to acquire
    // the lock multiple times. This trades some performance (recursive_mutex is
    // slower than shared_mutex) for correctness and stability.
    //
    // Performance impact: Negligible - lock contention was already minimal, and
    // recursive_mutex overhead is only a few nanoseconds per acquisition.
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _mutex;

    // Phase 7.3: Legacy observers removed (dead code)
    // Events now handled by EventDispatcher → Managers architecture

    // Debug tracking
    uint32 _lastDebugLogTime = 0;

    // Cheat effect throttle timer (ms remaining)
    uint32 _cheatEffectTimer = 0;

    // RPG-State-Based AI Budget System — controls which phases run per update
    AIBudgetTier _currentBudgetTier = AIBudgetTier::FULL;
    uint32 _budgetReassessTimer = 0;
};

// ========================================================================
// DEFAULT IMPLEMENTATION - For bots without specialized AI
// ========================================================================

class TC_GAME_API DefaultBotAI : public BotAI
{
public:
    explicit DefaultBotAI(Player* player) : BotAI(player) {}

    // Uses base UpdateAI() implementation
    // No combat specialization needed for default bots
};

// ========================================================================
// AI FACTORY - Creates appropriate AI for each class
// ========================================================================

class TC_GAME_API BotAIFactory final
{
    BotAIFactory() = default;
    ~BotAIFactory() = default;
    BotAIFactory(BotAIFactory const&) = delete;
    BotAIFactory& operator=(BotAIFactory const&) = delete;

public:
    static BotAIFactory* instance();

    // AI creation
    std::unique_ptr<BotAI> CreateAI(Player* bot);
    std::unique_ptr<BotAI> CreateClassAI(Player* bot, uint8 classId);
    std::unique_ptr<BotAI> CreateClassAI(Player* bot, uint8 classId, uint8 spec);

    // Specialized AI creation
    std::unique_ptr<BotAI> CreateSpecializedAI(Player* bot, std::string const& type);
    std::unique_ptr<BotAI> CreatePvPAI(Player* bot);
    std::unique_ptr<BotAI> CreatePvEAI(Player* bot);
    std::unique_ptr<BotAI> CreateRaidAI(Player* bot);

    // AI registration
    void RegisterAICreator(std::string const& type,
                          std::function<std::unique_ptr<BotAI>(Player*)> creator);

    // Initialization
    void InitializeDefaultTriggers(BotAI* ai);
    void InitializeDefaultValues(BotAI* ai);

private:
    void InitializeDefaultStrategies(BotAI* ai);
    void InitializeClassStrategies(BotAI* ai, uint8 classId, uint8 spec);

    std::unordered_map<std::string, std::function<std::unique_ptr<BotAI>(Player*)>> _creators;
};

#define sBotAIFactory BotAIFactory::instance()

} // namespace Playerbot