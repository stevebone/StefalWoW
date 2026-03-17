/*
 * Combat System Enterprise Architecture - Core Interfaces
 * 
 * These interfaces define the contracts for the combat system.
 * Implementation follows these interfaces exactly.
 * 
 * Target: 5,000 concurrent bots
 * Design: Event-driven, hierarchical coordination
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <optional>

// Forward declarations
class Player;
class Unit;
class Group;
class SpellInfo;
class Aura;

namespace Playerbot::Combat
{

// ============================================================================
// ENUMERATIONS
// ============================================================================

/**
 * @brief Combat context types - determines resource allocation and coordination
 */
enum class CombatContextType : uint8
{
    SOLO = 0,               // No group, minimal overhead
    GROUP_OPENWORLD = 1,    // In group, open world
    DUNGEON_TRASH = 2,      // In dungeon, trash mobs
    DUNGEON_BOSS = 3,       // In dungeon, boss fight
    RAID_TRASH = 4,         // In raid, trash
    RAID_BOSS = 5,          // In raid, boss encounter
    PVP_BATTLEGROUND = 6,   // In battleground
    PVP_ARENA = 7,          // In arena
    PVP_WORLD = 8           // World PvP
};

/**
 * @brief Update tier for resource allocation
 */
enum class UpdateTier : uint8
{
    CRITICAL = 0,   // Every tick (50ms)    - Bots in active combat
    HIGH = 1,       // Every 2 ticks (100ms) - Bots near combat
    NORMAL = 2,     // Every 4 ticks (200ms) - Grouped bots
    LOW = 3         // Every 10 ticks (500ms) - Solo/idle bots
};

/**
 * @brief Coordinator types
 */
enum class CoordinatorType : uint8
{
    THREAT = 0,
    INTERRUPT = 1,
    CROWD_CONTROL = 2,
    FORMATION = 3,
    COOLDOWN = 4,
    HEALING = 5
};

/**
 * @brief Combat decision types
 */
enum class DecisionType : uint8
{
    NONE = 0,
    CAST_SPELL = 1,
    USE_ITEM = 2,
    MOVE_TO_POSITION = 3,
    MOVE_TO_TARGET = 4,
    STOP_CASTING = 5,
    WAIT = 6
};

/**
 * @brief Plugin categories
 */
enum class PluginCategory : uint8
{
    SPELL = 0,          // Damage/healing spells
    COOLDOWN = 1,       // Major cooldowns
    DEFENSIVE = 2,      // Defensive abilities
    UTILITY = 3,        // Buffs, movement abilities
    INTERRUPT = 4,      // Interrupt abilities
    CC = 5              // Crowd control
};

/**
 * @brief Combat event types (extends base EventType)
 */
enum class CombatEventType : uint16
{
    // Damage events (500-519)
    DAMAGE_TAKEN = 500,
    DAMAGE_DEALT = 501,
    
    // Healing events (520-539)
    HEALING_RECEIVED = 520,
    HEALING_DONE = 521,
    
    // Spell events (540-559)
    ENEMY_CAST_START = 540,
    ENEMY_CAST_SUCCESS = 541,
    ENEMY_CAST_INTERRUPTED = 542,
    
    // Aura events (560-579)
    HARMFUL_AURA_APPLIED = 560,
    HARMFUL_AURA_REMOVED = 561,
    CC_APPLIED = 564,
    CC_REMOVED = 565,
    
    // Threat events (580-599)
    THREAT_CHANGED = 580,
    AGGRO_GAINED = 583,
    AGGRO_LOST = 584,
    
    // Coordination events (620-639)
    INTERRUPT_ASSIGNED = 620,
    CC_ASSIGNED = 621,
    FOCUS_TARGET_CHANGED = 622,
    
    // Encounter events (640-659)
    BOSS_ENGAGED = 640,
    BOSS_PHASE_CHANGED = 641,
    ENCOUNTER_WIPE = 645,
    ENCOUNTER_VICTORY = 646
};

// ============================================================================
// DATA STRUCTURES
// ============================================================================

/**
 * @brief Combat event structure
 */
struct CombatEvent
{
    CombatEventType type;
    ObjectGuid sourceGuid;
    ObjectGuid targetGuid;
    uint64 timestamp;
    
    // Event-specific data
    uint32 amount = 0;          // Damage/healing amount
    uint32 spellId = 0;         // Spell involved
    float threatAmount = 0.0f;  // Threat delta
    uint32 castTime = 0;        // For cast events
    
    uint8 priority = 100;       // Event priority (higher = more important)
};

/**
 * @brief Coordinator directive to bot
 */
struct CoordinatorDirective
{
    ObjectGuid targetBot;
    CoordinatorType coordinatorType;
    ObjectGuid targetUnit;
    uint32 spellId = 0;
    uint32 priority = 100;
    uint32 expirationTime = 0;
    std::string reason;
    
    bool IsExpired() const;
    bool IsValid() const { return targetBot != ObjectGuid::Empty; }
};

/**
 * @brief Combat decision output
 */
struct CombatDecision
{
    DecisionType type = DecisionType::NONE;
    uint32 spellId = 0;
    uint32 itemId = 0;
    ObjectGuid targetGuid;
    float targetX = 0.0f;
    float targetY = 0.0f;
    float targetZ = 0.0f;
    uint8 priority = 0;
    std::string reason;
    
    bool IsValid() const { return type != DecisionType::NONE; }
};

/**
 * @brief Plugin execution context
 */
struct PluginContext
{
    Player* bot = nullptr;
    Unit* target = nullptr;
    class ICombatContext const* combatContext = nullptr;
    float healthPct = 100.0f;
    float resourcePct = 100.0f;
    uint32 enemyCount = 0;
    bool isMoving = false;
    bool isInCombat = false;
};

/**
 * @brief Plugin execution result
 */
struct ExecutionResult
{
    bool success = false;
    uint32 spellId = 0;
    uint32 globalCooldownMs = 0;
    std::string failureReason;
};

/**
 * @brief Coordinator statistics
 */
struct CoordinatorStats
{
    uint64 directivesIssued = 0;
    uint64 directivesExecuted = 0;
    uint64 directivesFailed = 0;
    uint32 avgResponseTimeMs = 0;
};

// ============================================================================
// INTERFACE: ICombatContext
// ============================================================================

/**
 * @interface ICombatContext
 * @brief Provides context-aware configuration for combat systems
 *
 * Each context type implements this interface to provide appropriate
 * settings for that situation. Solo bots get minimal overhead,
 * raid bots get full coordination.
 */
class ICombatContext
{
public:
    virtual ~ICombatContext() = default;
    
    // Identification
    virtual CombatContextType GetType() const = 0;
    virtual std::string GetName() const = 0;
    virtual uint8 GetPriority() const = 0;
    
    // Update configuration
    virtual UpdateTier GetUpdateTier() const = 0;
    virtual uint32 GetBaseUpdateIntervalMs() const = 0;
    virtual uint32 GetMaxEventsPerUpdate() const = 0;
    
    // Coordination requirements
    virtual bool RequiresThreatCoordination() const = 0;
    virtual bool RequiresInterruptCoordination() const = 0;
    virtual bool RequiresCCCoordination() const = 0;
    virtual bool RequiresFormationManagement() const = 0;
    virtual bool RequiresCooldownCoordination() const = 0;
    virtual bool RequiresHealingCoordination() const = 0;
    
    // Behavior modifiers
    virtual float GetCoordinationIntensity() const = 0;  // 0.0-1.0
    virtual float GetAggressionLevel() const = 0;        // 0.0-1.0
    virtual float GetSurvivalPriority() const = 0;       // 0.0-1.0
    virtual bool ShouldTrackEnemyCooldowns() const = 0;
    virtual bool ShouldUsePredictivePositioning() const = 0;
    
    // Resource limits
    virtual uint32 GetMaxThreatTableSize() const = 0;
    virtual uint32 GetMaxTargetEvaluations() const = 0;
    virtual uint32 GetMaxPathCacheSize() const = 0;
};

// ============================================================================
// INTERFACE: ICombatEventSubscriber
// ============================================================================

/**
 * @interface ICombatEventSubscriber
 * @brief Interface for components that receive combat events
 */
class ICombatEventSubscriber
{
public:
    virtual ~ICombatEventSubscriber() = default;
    
    /**
     * @brief Handle incoming combat event
     * @return true if event was consumed (stops propagation)
     */
    virtual bool OnCombatEvent(CombatEvent const& event) = 0;
    
    /**
     * @brief Get event types this subscriber wants (bitmask)
     */
    virtual uint64 GetSubscribedEventMask() const = 0;
    
    /**
     * @brief Get subscriber priority (higher = receives first)
     */
    virtual uint8 GetSubscriberPriority() const { return 100; }
};

// ============================================================================
// INTERFACE: ICombatEventRouter
// ============================================================================

/**
 * @interface ICombatEventRouter
 * @brief Routes combat events to appropriate subscribers
 */
class ICombatEventRouter
{
public:
    virtual ~ICombatEventRouter() = default;
    
    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
    
    /**
     * @brief Publish a combat event (lock-free)
     */
    virtual void Publish(CombatEvent&& event) = 0;
    
    /**
     * @brief Subscribe to events for a zone
     */
    virtual void SubscribeZone(uint32 zoneId, ICombatEventSubscriber* subscriber) = 0;
    
    /**
     * @brief Subscribe to events for a group
     */
    virtual void SubscribeGroup(ObjectGuid groupGuid, ICombatEventSubscriber* subscriber) = 0;
    
    /**
     * @brief Subscribe to events for a bot
     */
    virtual void SubscribeBot(ObjectGuid botGuid, ICombatEventSubscriber* subscriber) = 0;
    
    /**
     * @brief Unsubscribe from all events
     */
    virtual void Unsubscribe(ICombatEventSubscriber* subscriber) = 0;
    
    /**
     * @brief Process pending events (called from world update)
     * @return Number of events processed
     */
    virtual uint32 ProcessPendingEvents(uint32 maxEvents = 1000) = 0;
};

// ============================================================================
// INTERFACE: ICombatCoordinator
// ============================================================================

/**
 * @interface ICombatCoordinator
 * @brief Interface for group-level coordination systems
 *
 * Coordinators manage group-wide activities like threat balancing,
 * interrupt rotation, CC chains, etc.
 */
class ICombatCoordinator : public ICombatEventSubscriber
{
public:
    virtual ~ICombatCoordinator() = default;
    
    // Lifecycle
    virtual void Initialize(Group* group) = 0;
    virtual void Shutdown() = 0;
    virtual void Update(uint32 diff) = 0;
    
    // Identification
    virtual CoordinatorType GetType() const = 0;
    virtual std::string GetName() const = 0;
    
    // Context awareness
    virtual bool IsRequiredForContext(ICombatContext const& context) const = 0;
    virtual void OnContextChanged(ICombatContext const& newContext) = 0;
    
    // Directive management
    virtual std::vector<CoordinatorDirective> GetActiveDirectives() const = 0;
    virtual CoordinatorDirective const* GetDirectiveForBot(ObjectGuid botGuid) const = 0;
    
    // Statistics
    virtual CoordinatorStats GetStats() const = 0;
};

// ============================================================================
// INTERFACE: ICombatDecisionEngine
// ============================================================================

/**
 * @interface ICombatDecisionEngine
 * @brief Makes combat decisions for a single bot
 *
 * Uses priority cascade:
 * 1. Survival (health critical)
 * 2. Coordinator directives
 * 3. Role rotation
 * 4. Utility
 */
class ICombatDecisionEngine
{
public:
    virtual ~ICombatDecisionEngine() = default;
    
    /**
     * @brief Make a combat decision
     */
    virtual CombatDecision Decide(ICombatContext const& context, uint32 diff) = 0;
    
    /**
     * @brief Register coordinator for directive reception
     */
    virtual void RegisterCoordinator(ICombatCoordinator* coordinator) = 0;
    
    /**
     * @brief Unregister coordinator
     */
    virtual void UnregisterCoordinator(ICombatCoordinator* coordinator) = 0;
    
    /**
     * @brief Set combat context
     */
    virtual void SetContext(std::shared_ptr<ICombatContext> context) = 0;
    
    /**
     * @brief Get current context
     */
    virtual ICombatContext const* GetContext() const = 0;
};

// ============================================================================
// INTERFACE: ICombatPlugin
// ============================================================================

/**
 * @interface ICombatPlugin
 * @brief Interface for ability execution plugins
 *
 * Plugins are the execution layer - they know how to cast specific
 * abilities under specific conditions. Configured via YAML.
 */
class ICombatPlugin
{
public:
    virtual ~ICombatPlugin() = default;
    
    // Identification
    virtual std::string GetPluginId() const = 0;
    virtual PluginCategory GetCategory() const = 0;
    virtual int GetPriority() const = 0;
    
    // Execution
    virtual bool CanExecute(PluginContext const& ctx) const = 0;
    virtual ExecutionResult Execute(PluginContext& ctx) = 0;
    
    // Configuration (YAML loading)
    virtual void LoadFromConfig(void const* yamlNode) = 0;
};

// ============================================================================
// INTERFACE: ISpecPluginRegistry
// ============================================================================

/**
 * @interface ISpecPluginRegistry
 * @brief Manages plugins for a specialization
 */
class ISpecPluginRegistry
{
public:
    virtual ~ISpecPluginRegistry() = default;
    
    /**
     * @brief Load plugins from YAML config
     */
    virtual void LoadFromYaml(std::string const& configPath) = 0;
    
    /**
     * @brief Get best executable plugin
     */
    virtual ICombatPlugin* GetBestPlugin(PluginContext const& ctx) const = 0;
    
    /**
     * @brief Get best plugin by category
     */
    virtual ICombatPlugin* GetBestPluginByCategory(
        PluginCategory category, PluginContext const& ctx) const = 0;
    
    /**
     * @brief Register a plugin manually
     */
    virtual void RegisterPlugin(std::unique_ptr<ICombatPlugin> plugin) = 0;
    
    /**
     * @brief Get all registered plugins
     */
    virtual std::vector<ICombatPlugin*> GetAllPlugins() const = 0;
};

// ============================================================================
// INTERFACE: IClassRoleResolver
// ============================================================================

/**
 * @brief Bot roles
 */
enum class BotRole : uint8
{
    UNKNOWN = 0,
    TANK = 1,
    HEALER = 2,
    MELEE_DPS = 3,
    RANGED_DPS = 4
};

/**
 * @interface IClassRoleResolver
 * @brief Central class/role resolution (replaces 28 switch statements)
 */
class IClassRoleResolver
{
public:
    virtual ~IClassRoleResolver() = default;
    
    /**
     * @brief Get primary role for class/spec
     */
    virtual BotRole GetPrimaryRole(uint8 classId, uint8 specId) const = 0;
    
    /**
     * @brief Get all available roles for a class
     */
    virtual std::vector<BotRole> GetAvailableRoles(uint8 classId) const = 0;
    
    /**
     * @brief Check if class/spec can perform a role
     */
    virtual bool CanPerformRole(uint8 classId, uint8 specId, BotRole role) const = 0;
    
    /**
     * @brief Check if class is a tank spec
     */
    virtual bool IsTankSpec(uint8 classId, uint8 specId) const = 0;
    
    /**
     * @brief Check if class is a healer spec
     */
    virtual bool IsHealerSpec(uint8 classId, uint8 specId) const = 0;
    
    /**
     * @brief Check if class is melee
     */
    virtual bool IsMeleeSpec(uint8 classId, uint8 specId) const = 0;
    
    /**
     * @brief Get spec name
     */
    virtual std::string GetSpecName(uint8 classId, uint8 specId) const = 0;
};

// ============================================================================
// FACTORY FUNCTIONS
// ============================================================================

/**
 * @brief Create context for player (uses existing CombatContextDetector)
 */
std::unique_ptr<ICombatContext> CreateContextForPlayer(Player const* player);

/**
 * @brief Create context by type
 */
std::unique_ptr<ICombatContext> CreateContextByType(CombatContextType type);

/**
 * @brief Get global class role resolver
 */
IClassRoleResolver& GetClassRoleResolver();

/**
 * @brief Get global combat event router
 */
ICombatEventRouter& GetCombatEventRouter();

} // namespace Playerbot::Combat
