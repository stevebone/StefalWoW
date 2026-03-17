/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * GAME SYSTEMS MANAGER Implementation
 *
 * Phase 6: BotAI Decoupling & Final Cleanup
 *
 * This file consolidates all manager initialization, update, and cleanup
 * logic from BotAI.cpp, reducing god class complexity.
 */

// IMPORTANT: LootDistribution.h MUST be included FIRST to avoid redefinition errors
// This is included before GameSystemsManager.h to ensure it's the first definition seen
#include "Social/LootDistribution.h"

#include "GameSystemsManager.h"
#include "AI/BotAI.h"
#include "Player.h"
#include "Log.h"
#include "Timer.h"

// Singleton manager includes for Update() calls
#include "Equipment/EquipmentManager.h"
#include "Professions/ProfessionManager.h"
#include "Banking/BankingManager.h"
#include "Professions/GatheringMaterialsBridge.h"
#include "Professions/ProfessionAuctionBridge.h"
#include "Professions/AuctionMaterialsBridge.h"
#include "Session/BotSessionManager.h"  // For IsInstanceBot check
#include "Session/BotSession.h"         // For BotSession::IsInstanceBot()

// Manager implementations (for unique_ptr destruction)
#include "AI/BehaviorPriorityManager.h"  // For unique_ptr<BehaviorPriorityManager> destruction

#include <set>

namespace Playerbot
{

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

GameSystemsManager::GameSystemsManager(Player* bot, BotAI* botAI)
    : _bot(bot), _botAI(botAI)
{
    // CRITICAL: No logging with _bot->GetName() in constructor/Initialize()
    // Player's m_name can be corrupted during concurrent access, causing ACCESS_VIOLATION
    // Manager instances will be created in Initialize()
}

GameSystemsManager::~GameSystemsManager()
{
    // ========================================================================
    // CRITICAL: Destructor must be COMPLETELY ALLOCATION-FREE!
    // ========================================================================
    // This destructor may be called during stack unwinding from exceptions
    // (e.g., std::bad_alloc). During memory pressure:
    // - TC_LOG_* macros allocate std::string internally
    // - Any allocation can throw std::bad_alloc
    // - Exceptions from destructors during unwinding = std::terminate()
    //
    // Solution: NO LOGGING, NO ALLOCATIONS. Just reset the unique_ptrs.
    // The manager destructors themselves should also be allocation-free.
    // ========================================================================

    // Explicit destruction order to ensure EventDispatcher outlives managers
    // (managers may call UnsubscribeAll() in their destructors)

    // 1. High-level systems first
    _combatCoordinationIntegrator.reset();  // Sprint 3: Must shutdown before managers it references
    _combatStateManager.reset();
    _deathRecoveryManager.reset();
    _unifiedMovementCoordinator.reset();

    // Sprint 3: Combat Behaviors managers
    _aoeDecisionManager.reset();
    _cooldownStackingOptimizer.reset();
    _defensiveBehaviorManager.reset();
    _dispelCoordinator.reset();
    _interruptRotationManager.reset();

    // Game Systems
    _consumableManager.reset();

    // 2. Game system managers
    _tradeManager.reset();
    _gatheringManager.reset();
    _professionManager.reset();
    _gatheringMaterialsBridge.reset();
    _auctionMaterialsBridge.reset();
    _professionAuctionBridge.reset();
    _auctionManager.reset();
    _bankingManager.reset();
    _equipmentManager.reset();
    _mountManager.reset();
    _ridingManager.reset();
    _battlePetManager.reset();
    _humanizationManager.reset();
    _arenaAI.reset();
    _pvpCombatAI.reset();
    _auctionHouse.reset();
    _farmingCoordinator.reset();
    _groupCoordinator.reset();

    // 3. Support systems
    _targetScanner.reset();
    _groupInvitationHandler.reset();
    _priorityManager.reset();

    // 4. Decision systems
    _decisionFusion.reset();
    _actionPriorityQueue.reset();
    _behaviorTree.reset();
    _hybridAI.reset();

    // 5. Finally: Registry and event dispatcher (must be last)
    _managerRegistry.reset();
    _eventDispatcher.reset();
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void GameSystemsManager::Initialize(Player* bot)
{
    if (_initialized)
        return;  // No logging - GetName() unsafe during concurrent access

    _bot = bot;
    // CRITICAL: No logging with _bot->GetName() - concurrent access during initialization

    // Check if bot is in instance-only mode (JIT bots for BG/LFG)
    // Instance-only mode skips expensive non-essential managers to reduce server overhead
    bool instanceOnlyMode = _botAI && _botAI->IsInstanceOnlyMode();

    // ========================================================================
    // PHASE 1: Create Manager Instances (in dependency order)
    // ========================================================================

    // Priority-based behavior manager
    _priorityManager = std::make_unique<BehaviorPriorityManager>(_botAI);

    // Group management - ALWAYS NEEDED for instances
    _groupInvitationHandler = std::make_unique<GroupInvitationHandler>(_bot);

    // Target scanner for autonomous enemy detection - ALWAYS NEEDED for combat
    _targetScanner = std::make_unique<TargetScanner>(_bot);

    // ========================================================================
    // NON-ESSENTIAL MANAGERS - Skipped in instance-only mode to reduce overhead
    // ========================================================================
    if (!instanceOnlyMode)
    {
        // Game system managers - only for full-featured bots
        _tradeManager = std::make_unique<TradeManager>(_bot, _botAI);
        _gatheringManager = std::make_unique<GatheringManager>(_bot, _botAI);
        _professionManager = std::make_unique<ProfessionManager>(_bot);
        _gatheringMaterialsBridge = std::make_unique<GatheringMaterialsBridge>(_bot);
        _auctionMaterialsBridge = std::make_unique<AuctionMaterialsBridge>(_bot);
        _professionAuctionBridge = std::make_unique<ProfessionAuctionBridge>(_bot);
        _farmingCoordinator = std::make_unique<FarmingCoordinator>(_bot);
        _auctionManager = std::make_unique<AuctionManager>(_bot, _botAI);
        _bankingManager = std::make_unique<BankingManager>(_bot);
        _auctionHouse = std::make_unique<AuctionHouse>(_bot);
        _guildBankManager = std::make_unique<GuildBankManager>(_bot);
        _guildEventCoordinator = std::make_unique<GuildEventCoordinator>(_bot);
        _guildIntegration = std::make_unique<GuildIntegration>(_bot);
        _tradeSystem = std::make_unique<TradeSystem>(_bot);

        // Quest system managers - only for questing bots
        _dynamicQuestSystem = std::make_unique<DynamicQuestSystem>(_bot);
        _objectiveTracker = std::make_unique<ObjectiveTracker>(_bot);
        _questCompletion = std::make_unique<QuestCompletion>(_bot);
        _questPickup = std::make_unique<QuestPickup>(_bot);
        _questTurnIn = std::make_unique<QuestTurnIn>(_bot);
        _questValidation = std::make_unique<QuestValidation>(_bot);

        // Companion managers - only for full-featured bots
        _mountManager = std::make_unique<MountManager>(_bot);
        _ridingManager = std::make_unique<RidingManager>(_bot);
        _battlePetManager = std::make_unique<BattlePetManager>(_bot);

        // Humanization system (Phase 3)
        _humanizationManager = std::make_unique<Humanization::HumanizationManager>(_bot);
    }
    // else: instance-only mode - skipping 24 non-essential managers for reduced overhead

    // ========================================================================
    // ESSENTIAL MANAGERS - Always created (needed for BG/LFG/Instance combat)
    // ========================================================================
    _equipmentManager = std::make_unique<EquipmentManager>(_bot);
    _arenaAI = std::make_unique<ArenaAI>(_bot);
    _pvpCombatAI = std::make_unique<PvPCombatAI>(_bot);
    _lootDistribution = std::make_unique<LootDistribution>(_bot);
    _roleAssignment = std::make_unique<RoleAssignment>(_bot);
    // Note: LFGBotManager is a global singleton, accessed via sLFGBotManager macro
    // _lfgBotManager = std::make_unique<LFGBotManager>(_bot);  // ERROR: Cannot instantiate singleton
    // Note: LFGBotSelector is also a global singleton
    // _lfgBotSelector = std::make_unique<LFGBotSelector>(_bot);  // ERROR: Cannot instantiate singleton
    // Note: LFGGroupCoordinator is also a global singleton, accessed via sLFGGroupCoordinator macro
    // _lfgGroupCoordinator = std::make_unique<LFGGroupCoordinator>(_bot);  // ERROR: Cannot instantiate singleton
    // Note: InstanceCoordination is also a global singleton
    // _instanceCoordination = std::make_unique<InstanceCoordination>(_bot);  // ERROR: Cannot instantiate singleton
    // Note: BotPriorityManager is also a global singleton, accessed via sBotPriorityMgr macro
    // _botPriorityManager = std::make_unique<BotPriorityManager>(_bot);  // ERROR: Cannot instantiate singleton
    // Note: BotWorldSessionMgr is also a global singleton, accessed via sBotWorldSessionMgr macro
    // _botWorldSessionMgr = std::make_unique<BotWorldSessionMgr>(_bot);  // ERROR: Cannot instantiate singleton
    _botLifecycleManager = std::make_unique<BotLifecycleManager>(_bot);
    _groupCoordinator = std::make_unique<Advanced::GroupCoordinator>(_bot, _botAI);

    // Death recovery system
    _deathRecoveryManager = std::make_unique<DeathRecoveryManager>(_bot, _botAI);

    // Unified Movement Coordinator (PRIMARY movement system)
    _unifiedMovementCoordinator = std::make_unique<UnifiedMovementCoordinator>(_bot);

    // Combat state manager
    _combatStateManager = std::make_unique<CombatStateManager>(_bot, _botAI);

    // ========================================================================
    // SPRINT 3: COMBAT COORDINATION - Essential for group coordination
    // ========================================================================

    // Combat Behaviors managers - essential for group combat
    _aoeDecisionManager = std::make_unique<AoEDecisionManager>(_botAI);
    _cooldownStackingOptimizer = std::make_unique<CooldownStackingOptimizer>(_botAI);
    _defensiveBehaviorManager = std::make_unique<DefensiveBehaviorManager>(_botAI);
    _dispelCoordinator = std::make_unique<DispelCoordinator>(_botAI);
    _interruptRotationManager = std::make_unique<InterruptRotationManager>(_botAI);

    // Game Systems
    _consumableManager = std::make_unique<ConsumableManager>(_bot, _botAI);
    _consumableManager->Initialize();

    // Combat Coordination Integrator - bridges managers with BotMessageBus claim system
    _combatCoordinationIntegrator = std::make_unique<CombatCoordinationIntegrator>(_botAI);

    // Initialize CombatCoordinationIntegrator with references to combat managers
    _combatCoordinationIntegrator->Initialize(
        nullptr,  // InterruptCoordinatorFixed (not yet migrated)
        _defensiveBehaviorManager.get(),
        nullptr,  // CrowdControlManager (not yet migrated)
        _dispelCoordinator.get()
    );

    // Manager creation complete - no logging to avoid GetName() during init

    // ========================================================================
    // PHASE 2: Event System
    // ========================================================================

    // Event dispatcher and manager registry
    _eventDispatcher = std::make_unique<Events::EventDispatcher>(512);  // Initial queue size: 512 events
    _managerRegistry = std::make_unique<ManagerRegistry>();

    // Event system ready - no logging during init

    // ========================================================================
    // PHASE 3: Decision Systems
    // ========================================================================

    // Decision fusion system
    _decisionFusion = std::make_unique<bot::ai::DecisionFusionSystem>();

    // Decision fusion ready - no logging during init

    // Action priority queue
    _actionPriorityQueue = std::make_unique<bot::ai::ActionPriorityQueue>();

    // Action queue ready - no logging during init

    // Behavior tree
    _behaviorTree = std::make_unique<bot::ai::BehaviorTree>("DefaultTree");

    // Behavior tree ready - no logging during init

    // ========================================================================
    // PHASE 4: Hybrid AI System
    // ========================================================================

    InitializeHybridAI();

    // ========================================================================
    // PHASE 5: Manager Initialization via IManagerBase
    // ========================================================================

    if (_managerRegistry && _eventDispatcher)
    {
        // Initialize NON-ESSENTIAL managers (only if not in instance-only mode)
        if (!instanceOnlyMode)
        {
            if (_tradeManager)
            {
                _tradeManager->Initialize();
                TC_LOG_DEBUG("module.playerbot.managers", "✅ TradeManager initialized via IManagerBase");
            }

            if (_gatheringManager)
            {
                _gatheringManager->Initialize();
                TC_LOG_DEBUG("module.playerbot.managers", "✅ GatheringManager initialized via IManagerBase");
            }

            if (_gatheringMaterialsBridge)
            {
                _gatheringMaterialsBridge->Initialize();
                TC_LOG_DEBUG("module.playerbot.managers", "✅ GatheringMaterialsBridge initialized - gathering-crafting coordination active");
            }

            if (_auctionMaterialsBridge)
            {
                _auctionMaterialsBridge->Initialize();
                TC_LOG_DEBUG("module.playerbot.managers", "✅ AuctionMaterialsBridge initialized - material sourcing optimization active");
            }

            if (_professionAuctionBridge)
            {
                _professionAuctionBridge->Initialize();
                TC_LOG_DEBUG("module.playerbot.managers", "✅ ProfessionAuctionBridge initialized - profession-auction coordination active");
            }

            if (_auctionManager)
            {
                _auctionManager->Initialize();
                TC_LOG_DEBUG("module.playerbot.managers", "✅ AuctionManager initialized via IManagerBase");
            }

            if (_bankingManager)
            {
                _bankingManager->Initialize();
                TC_LOG_DEBUG("module.playerbot.managers", "✅ BankingManager initialized - personal banking automation active");
            }

            if (_farmingCoordinator)
            {
                _farmingCoordinator->Initialize();
                TC_LOG_DEBUG("module.playerbot.managers", "✅ FarmingCoordinator initialized - profession farming automation active");
            }

            if (_mountManager)
            {
                _mountManager->Initialize();
                TC_LOG_DEBUG("module.playerbot.managers", "✅ MountManager initialized - mount automation and collection tracking active");
            }

            if (_ridingManager)
            {
                _ridingManager->Initialize();
                TC_LOG_DEBUG("module.playerbot.managers", "✅ RidingManager initialized - humanized riding skill acquisition active");
            }

            if (_humanizationManager)
            {
                _humanizationManager->Initialize();
                TC_LOG_DEBUG("module.playerbot.managers", "✅ HumanizationManager initialized - human-like behavior active");
            }

            if (_battlePetManager)
            {
                _battlePetManager->Initialize();
                TC_LOG_DEBUG("module.playerbot.managers", "✅ BattlePetManager initialized - battle pet automation and collection active");
            }

            // Subscribe non-essential managers to events
            SubscribeManagersToEvents();
        }
        else
        {
            TC_LOG_DEBUG("module.playerbot.managers", "⚡ Instance-only mode: Skipped 24 non-essential managers for reduced overhead");
        }

        // Initialize ESSENTIAL managers (always needed for combat)
        if (_arenaAI)
        {
            _arenaAI->Initialize();
            TC_LOG_DEBUG("module.playerbot.managers", "✅ ArenaAI initialized - arena PvP automation active");
        }

        if (_pvpCombatAI)
        {
            _pvpCombatAI->Initialize();
            TC_LOG_DEBUG("module.playerbot.managers", "✅ PvPCombatAI initialized - PvP combat automation active");
        }

        if (_groupCoordinator)
        {
            _groupCoordinator->Initialize();
            TC_LOG_DEBUG("module.playerbot.managers", "✅ GroupCoordinator initialized - Dungeon/Raid coordination active");
        }

        if (_combatStateManager)
        {
            _combatStateManager->Initialize();
            TC_LOG_DEBUG("module.playerbot.managers", "✅ CombatStateManager initialized - DAMAGE_TAKEN event subscription active");
        }

        // Phase 7.1 integration complete - no logging during init
    }

    _initialized = true;

    // Initialization complete - no logging to avoid GetName() crash
}

void GameSystemsManager::Shutdown()
{
    // CRITICAL: No logging - GetName() unsafe during shutdown
    // Managers will be destroyed in destructor with proper order
    _initialized = false;
}

// ============================================================================
// HYBRID AI INITIALIZATION
// ============================================================================

void GameSystemsManager::InitializeHybridAI()
{
    // Initialize Hybrid AI Decision System (Utility AI + Behavior Trees)
    // Pass BotAI pointer to HybridAIController
    _hybridAI = std::make_unique<HybridAIController>(_botAI);
    // CRITICAL: No logging with GetName() - causes ACCESS_VIOLATION during concurrent init
}

// ============================================================================
// EVENT SUBSCRIPTION
// ============================================================================

void GameSystemsManager::SubscribeManagersToEvents()
{
    // Subscribe TradeManager to trade events
    if (_tradeManager && _eventDispatcher)
    {
        _eventDispatcher->Subscribe(StateMachine::EventType::TRADE_INITIATED, _tradeManager.get());
        _eventDispatcher->Subscribe(StateMachine::EventType::TRADE_ACCEPTED, _tradeManager.get());
        _eventDispatcher->Subscribe(StateMachine::EventType::TRADE_CANCELLED, _tradeManager.get());
        _eventDispatcher->Subscribe(StateMachine::EventType::TRADE_ITEM_ADDED, _tradeManager.get());
        _eventDispatcher->Subscribe(StateMachine::EventType::TRADE_GOLD_ADDED, _tradeManager.get());
        _eventDispatcher->Subscribe(StateMachine::EventType::GOLD_RECEIVED, _tradeManager.get());
        _eventDispatcher->Subscribe(StateMachine::EventType::GOLD_SPENT, _tradeManager.get());
        _eventDispatcher->Subscribe(StateMachine::EventType::LOW_GOLD_WARNING, _tradeManager.get());
        _eventDispatcher->Subscribe(StateMachine::EventType::VENDOR_PURCHASE, _tradeManager.get());
        _eventDispatcher->Subscribe(StateMachine::EventType::VENDOR_SALE, _tradeManager.get());
        _eventDispatcher->Subscribe(StateMachine::EventType::REPAIR_COST, _tradeManager.get());
        TC_LOG_INFO("module.playerbot.managers", "🔗 TradeManager subscribed to 11 trade/gold events");
    }

    // Subscribe AuctionManager to auction events
    if (_auctionManager && _eventDispatcher)
    {
        _eventDispatcher->Subscribe(StateMachine::EventType::AUCTION_BID_PLACED, _auctionManager.get());
        _eventDispatcher->Subscribe(StateMachine::EventType::AUCTION_WON, _auctionManager.get());
        _eventDispatcher->Subscribe(StateMachine::EventType::AUCTION_OUTBID, _auctionManager.get());
        _eventDispatcher->Subscribe(StateMachine::EventType::AUCTION_EXPIRED, _auctionManager.get());
        _eventDispatcher->Subscribe(StateMachine::EventType::AUCTION_SOLD, _auctionManager.get());
        TC_LOG_INFO("module.playerbot.managers", "🔗 AuctionManager subscribed to 5 auction events");
    }
}

// ============================================================================
// UPDATE
// ============================================================================

void GameSystemsManager::Update(uint32 diff)
{
    if (!_initialized || !_bot || !_bot->IsInWorld())
        return;

    // Update managers
    UpdateManagers(diff);
}

void GameSystemsManager::UpdateManagers(uint32 diff)
{
    // DEBUG LOGGING THROTTLE: Only log every 50 seconds for whitelisted test bots
    _debugLogAccumulator += diff;
    static const std::set<std::string> testBots = {"Anderenz", "Boone", "Nelona", "Sevtap"};
    bool isTestBot = _bot && (testBots.find(_bot->GetName()) != testBots.end());
    bool shouldLog = isTestBot && (_debugLogAccumulator >= 50000);
    if (shouldLog) _debugLogAccumulator = 0;

    if (shouldLog)
    {
        TC_LOG_ERROR("module.playerbot", "🔧 GameSystemsManager::UpdateManagers ENTRY: Bot {}, IsInWorld()={}",
            _bot->GetName(), _bot->IsInWorld());
    }

    if (!_bot || !_bot->IsInWorld())
    {
        if (shouldLog)
            TC_LOG_ERROR("module.playerbot", "❌ GameSystemsManager::UpdateManagers EARLY RETURN: Bot {} not in world",
                _bot->GetName());
        return;
    }

    // ========================================================================
    // INSTANCE BOT CHECK - Skip non-essential managers for pool/JIT bots
    // ========================================================================
    bool isInstanceBot = false;
    if (BotSession* session = BotSessionManager::GetBotSession(_bot->GetSession()))
        isInstanceBot = session->IsInstanceBot();

    // ========================================================================
    // RPG-STATE AI BUDGET TIER - Manager-level scope gating
    // ========================================================================
    // FULL:    all managers run
    // REDUCED: movement + safety + social managers only
    // MINIMAL: only EventDispatcher (safety events) — BotAI skips GameSystems entirely
    //          for MINIMAL via goto, but guard here for safety
    bool isFull    = (_budgetTier == AIBudgetTier::FULL);
    bool isReduced = (_budgetTier <= AIBudgetTier::REDUCED); // FULL or REDUCED

    // ========================================================================
    // PHASE 7.1: EVENT DISPATCHER - Process queued events first
    // ========================================================================
    if (_eventDispatcher)
    {
        // Process up to 100 events per update cycle to maintain performance
        uint32 eventsProcessed = _eventDispatcher->ProcessQueue(100);

        if (eventsProcessed > 0)
        {
            TC_LOG_TRACE("module.playerbot.events",
                "Bot {} processed {} events this cycle", _bot->GetName(), eventsProcessed);
        }

        // Warn if queue is backing up (>500 events indicates processing bottleneck)
        size_t queueSize = _eventDispatcher->GetQueueSize();
        if (queueSize > 500)
        {
            TC_LOG_WARN("module.playerbot.events",
                "Bot {} event queue backlog: {} events pending", _bot->GetName(), queueSize);
        }
    }

    // ========================================================================
    // PHASE 7.1: MANAGER REGISTRY - Update all registered managers (REDUCED+)
    // ========================================================================
    if (isReduced && _managerRegistry)
    {
        uint32 managersUpdated = _managerRegistry->UpdateAll(diff);

        if (managersUpdated > 0)
        {
            TC_LOG_TRACE("module.playerbot.managers",
                "Bot {} updated {} managers this cycle", _bot->GetName(), managersUpdated);
        }
    }

    // ========================================================================
    // MANAGER UPDATES - Legacy direct updates during Phase 7 transition
    // ========================================================================

    // Trade manager handles vendor interactions, repairs, and consumables (REDUCED+)
    if (isReduced && _tradeManager)
        _tradeManager->Update(diff);

    // ========================================================================
    // WORLD BOT ONLY MANAGERS - Skip for instance bots AND REDUCED tier
    // These managers handle open-world activities only needed at FULL tier
    // ========================================================================
    if (isFull && !isInstanceBot)
    {
        // Gathering manager handles mining, herbalism, skinning
        if (_gatheringManager)
            _gatheringManager->Update(diff);

        // ========================================================================
        // THROTTLED BRIDGE UPDATES - Don't need every-frame updates
        // ========================================================================

        // Gathering materials bridge coordinates gathering with crafting needs (2 sec throttle)
        _gatheringBridgeTimer += diff;
        if (_gatheringBridgeTimer >= 2000)
        {
            _gatheringBridgeTimer = 0;
            if (_gatheringMaterialsBridge)
                _gatheringMaterialsBridge->Update(diff);
        }

        // Auction materials bridge optimizes material sourcing (2 sec throttle)
        _auctionBridgeTimer += diff;
        if (_auctionBridgeTimer >= 2000)
        {
            _auctionBridgeTimer = 0;
            if (_auctionMaterialsBridge)
                _auctionMaterialsBridge->Update(diff);
        }

        // Profession auction bridge handles selling materials/crafts (5 sec throttle)
        _professionBridgeTimer += diff;
        if (_professionBridgeTimer >= 5000)
        {
            _professionBridgeTimer = 0;
            if (_professionAuctionBridge)
                _professionAuctionBridge->Update(_bot, diff);
        }

        // Auction manager handles auction house buying, selling, and market scanning (5 sec throttle)
        _auctionUpdateTimer += diff;
        if (_auctionUpdateTimer >= 5000)
        {
            _auctionUpdateTimer = 0;
            if (_auctionManager)
                _auctionManager->Update(diff);
        }

        // Banking manager handles personal banking automation (5 sec throttle - banking is slow)
        _bankingCheckTimer += diff;
        if (_bankingCheckTimer >= 5000)
        {
            _bankingCheckTimer = 0;
            if (_bankingManager)
                _bankingManager->Update(diff);
        }

        // Farming coordinator handles profession skill leveling automation (2 sec throttle)
        _farmingUpdateTimer += diff;
        if (_farmingUpdateTimer >= 2000)
        {
            _farmingUpdateTimer = 0;
            if (_farmingCoordinator)
                _farmingCoordinator->Update(_bot, diff);
        }
    } // end !isInstanceBot

    // Group coordinator handles group/raid mechanics, role assignment, and coordination (REDUCED+)
    if (isReduced && _groupCoordinator)
        _groupCoordinator->Update(diff);

    // ========================================================================
    // EQUIPMENT AUTO-EQUIP - Check every 10 seconds (FULL only)
    // ========================================================================
    if (isFull)
    {
        _equipmentCheckTimer += diff;
        if (_equipmentCheckTimer >= 10000) // 10 seconds
        {
            _equipmentCheckTimer = 0;
            if (_equipmentManager)
                _equipmentManager->AutoEquipBestGear();
        }
    }

    // ========================================================================
    // MORE WORLD BOT ONLY MANAGERS - Budget-tier gated
    // ========================================================================
    if (!isInstanceBot)
    {
        // Mount automation — REDUCED+ (needed for travel states)
        if (isReduced)
        {
            _mountUpdateTimer += diff;
            if (_mountUpdateTimer >= 200)
            {
                _mountUpdateTimer = 0;
                if (_mountManager)
                    _mountManager->Update(diff);
            }
        }

        // Riding acquisition — FULL only (skill learning is rare)
        if (isFull)
        {
            _ridingUpdateTimer += diff;
            if (_ridingUpdateTimer >= 5000)
            {
                _ridingUpdateTimer = 0;
                if (_ridingManager)
                    _ridingManager->Update(diff);
            }
        }

        // Humanization system — REDUCED+ (needed for natural behavior in travel/city)
        if (isReduced && _humanizationManager)
            _humanizationManager->Update(diff);

        // Battle pet automation — FULL only
        if (isFull)
        {
            _battlePetUpdateTimer += diff;
            if (_battlePetUpdateTimer >= 500)
            {
                _battlePetUpdateTimer = 0;
                if (_battlePetManager)
                    _battlePetManager->Update(diff);
            }
        }
    } // end !isInstanceBot (world bot only managers)

    // ========================================================================
    // PVP MANAGERS - FULL only (BG/Dungeon bots always have FULL tier)
    // ========================================================================
    if (isFull)
    {
        _arenaAIUpdateTimer += diff;
        if (_arenaAIUpdateTimer >= 100)
        {
            _arenaAIUpdateTimer = 0;
            if (_arenaAI)
                _arenaAI->Update(diff);
        }

        _pvpCombatUpdateTimer += diff;
        if (_pvpCombatUpdateTimer >= 100)
        {
            _pvpCombatUpdateTimer = 0;
            if (_pvpCombatAI)
                _pvpCombatAI->Update(diff);
        }
    }

    // ========================================================================
    // SPRINT 3: COMBAT COORDINATION - FULL only (combat managers)
    // ========================================================================
    if (isFull)
    {
        _combatCoordTimer += diff;
        if (_combatCoordTimer >= 100)
        {
            _combatCoordTimer = 0;
            if (_combatCoordinationIntegrator)
                _combatCoordinationIntegrator->Update(diff);
        }

        _dispelTimer += diff;
        if (_dispelTimer >= 200)
        {
            _dispelTimer = 0;
            if (_dispelCoordinator)
                _dispelCoordinator->Update(diff);
        }

        _interruptTimer += diff;
        if (_interruptTimer >= 100)
        {
            _interruptTimer = 0;
            if (_interruptRotationManager)
                _interruptRotationManager->Update(diff);
        }

        _aoeTimer += diff;
        if (_aoeTimer >= 500)
        {
            _aoeTimer = 0;
            if (_aoeDecisionManager)
                _aoeDecisionManager->Update(diff);
        }

        _cdStackTimer += diff;
        if (_cdStackTimer >= 500)
        {
            _cdStackTimer = 0;
            if (_cooldownStackingOptimizer)
                _cooldownStackingOptimizer->Update(diff);
        }

        _defenseTimer += diff;
        if (_defenseTimer >= 200)
        {
            _defenseTimer = 0;
            if (_defensiveBehaviorManager)
                _defensiveBehaviorManager->Update(diff);
        }
    }

    // ========================================================================
    // CONSUMABLE MANAGER - REDUCED+ tier (handles pre-combat buffs + emergency potions)
    // Has internal throttling: 5s out of combat, 500ms in combat
    // ========================================================================
    if (isReduced && _consumableManager)
    {
        _consumableManager->Update(diff);
    }

    // ========================================================================
    // MORE WORLD BOT ONLY - PROFESSION AUTOMATION - FULL only
    // ========================================================================
    if (isFull && !isInstanceBot)
    {
        _professionCheckTimer += diff;
        if (_professionCheckTimer >= 15000) // 15 seconds
        {
            _professionCheckTimer = 0;
            if (_professionManager)
                _professionManager->Update(diff);
        }
    }
}

} // namespace Playerbot
