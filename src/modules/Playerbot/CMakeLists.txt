# This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

# TrinityCore Playerbot Module - Enterprise Implementation
# High-performance AI-controlled player bots with Intel TBB, parallel hashmap, and boost optimizations
#
# ARCHITECTURE: Multi-Library Build (Fixes LNK1248 4GB COFF limit on Windows)
# The module is split into multiple static libraries to stay under MSVC's 4GB limit.
# Each library is compiled independently and linked together via the 'playerbot' interface target.

if(NOT BUILD_PLAYERBOT)
    message(STATUS "Playerbot module: DISABLED (use -DBUILD_PLAYERBOT=1 to enable)")
    return()
endif()

message(STATUS "=== Building TrinityCore Playerbot Enterprise Module (Multi-Library) ===")

# Include enterprise dependency validation
include(${CMAKE_SOURCE_DIR}/cmake/modules/FindPlayerbotDependencies.cmake)

message(STATUS "Enterprise dependencies validated by FindPlayerbotDependencies.cmake")
message(STATUS "✅ All enterprise dependencies validated")
message(STATUS "Playerbot module: ENABLED with enterprise optimizations")

# ============================================================================
# LIBRARY 1: playerbot-core
# Core systems: Module entry, Config, Session, Chat, Threading, Core hooks
# ============================================================================
set(PLAYERBOT_CORE_SOURCES
    # Module entry point
    ${CMAKE_CURRENT_SOURCE_DIR}/PlayerbotModule.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PlayerbotModule.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PlayerbotModuleAdapter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PlayerbotModuleAdapter.h

    # Configuration system
    ${CMAKE_CURRENT_SOURCE_DIR}/Config/PlayerbotConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Config/PlayerbotConfig.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Config/PlayerbotTradeConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Config/PlayerbotTradeConfig.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Config/PlayerbotLog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Config/PlayerbotLog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Config/GuidedSetupHelper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Config/GuidedSetupHelper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Config/ConfigManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Config/ConfigManager.h

    # Session management
    ${CMAKE_CURRENT_SOURCE_DIR}/Session/BotSession.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Session/BotSession.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Session/BotPacketSimulator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Session/BotPacketSimulator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Session/PacketDeferralClassifier.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Session/PacketDeferralClassifier.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Session/BotSessionManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Session/BotSessionManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Session/BotWorldSessionMgr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Session/BotWorldSessionMgr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Session/BotPacketRelay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Session/BotPacketRelay.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Session/BotPriorityManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Session/BotPriorityManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Session/BotPerformanceMonitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Session/BotPerformanceMonitor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Session/ServerLoadMonitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Session/ServerLoadMonitor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Session/BotHealthCheck.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Session/BotHealthCheck.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Session/BotSessionFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Session/BotSessionFactory.h

    # Chat system
    ${CMAKE_CURRENT_SOURCE_DIR}/Chat/BotChatCommandHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Chat/BotChatCommandHandler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Chat/ChatTemplateManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Chat/ChatTemplateManager.h

    # Core hooks and helpers
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/PlayerBotHooks.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/PlayerBotHooks.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/PlayerBotHelpers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/PlayerBotHelpers.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/References/SafeObjectReference.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/References/SafeObjectReference.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/BotReadinessChecker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/BotReadinessChecker.h

    # Core Managers
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Managers/IManagerBase.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Managers/ManagerRegistry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Managers/ManagerRegistry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Managers/IGameSystemsManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Managers/GameSystemsManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Managers/GameSystemsManager.h

    # Core Utilities
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/DirtyValue.h

    # Core Subsystem Registry
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/PlayerbotSubsystem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/PlayerbotSubsystemRegistry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/PlayerbotSubsystemRegistry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/SubsystemAdapters.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/SubsystemAdapters.h

    # Core Events
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Events/EventDispatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Events/EventDispatcher.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Events/QueueEventData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Events/CombatEventType.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Events/CombatEvent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Events/CombatEvent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Events/ICombatEventSubscriber.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Events/CombatEventRouter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Events/CombatEventRouter.cpp

    # Core Services
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Services/BotNpcLocationService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Services/BotNpcLocationService.h

    # Core Diagnostics
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Diagnostics/BotOperationTracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Diagnostics/BotOperationTracker.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Diagnostics/GroupMemberDiagnostics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Diagnostics/GroupMemberDiagnostics.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Diagnostics/BotCheatMask.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Diagnostics/BotCheatMask.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Diagnostics/BotLogFilter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Diagnostics/BotLogFilter.h

    # Core Combat - Context Detection and Performance
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Combat/CombatContextDetector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Combat/CombatContextDetector.h

    # Core DI Interfaces
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/DI/Interfaces/IObjectiveTracker.h  # TODO: File not created yet
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/DI/Interfaces/IRidingManager.h  # TODO: File not created yet

    # Events
    ${CMAKE_CURRENT_SOURCE_DIR}/Events/BotEventData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/PlayerbotEventScripts.cpp

    # Threading
    ${CMAKE_CURRENT_SOURCE_DIR}/Threading/BotAction.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Threading/BotActionQueue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Threading/BotActionProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Threading/BotActionProcessor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Threading/BotActionManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Threading/LockHierarchy.h

    # Commands
    ${CMAKE_CURRENT_SOURCE_DIR}/Commands/PlayerbotCommands.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Commands/PlayerbotCommands.h

    # Monitoring
    ${CMAKE_CURRENT_SOURCE_DIR}/Monitoring/BotMonitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Monitoring/BotMonitor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Monitoring/PerformanceMetrics.h

    # Adapters
    ${CMAKE_CURRENT_SOURCE_DIR}/Adapters/BotSpawnerAdapter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Adapters/BotSpawnerAdapter.h
)

# ============================================================================
# LIBRARY 2: playerbot-database
# Database systems and persistence
# ============================================================================
set(PLAYERBOT_DATABASE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Database/PlayerbotCharacterDBInterface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Database/PlayerbotCharacterDBInterface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Database/PlayerbotDatabase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Database/PlayerbotDatabase.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Database/PlayerbotDatabaseConnection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Database/PlayerbotDatabaseConnection.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Database/PlayerbotResultSet.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Database/PlayerbotResultSet.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Database/PlayerbotDatabaseStatements.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Database/PlayerbotDatabaseStatements.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Database/PlayerbotMigrationMgr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Database/PlayerbotMigrationMgr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Database/BotStatePersistence.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Database/BotStatePersistence.h
)

# ============================================================================
# LIBRARY 3: playerbot-ai-base
# AI base systems: BotAI, Strategy, Actions, Triggers, Values, BehaviorTree
# ============================================================================
set(PLAYERBOT_AI_BASE_SOURCES
    # AI Core
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/BotAI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/BotAI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/BotAI_HybridAI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/BotAIFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/BehaviorManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/BehaviorManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ExampleManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ExampleManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/BehaviorPriorityManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/BehaviorPriorityManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ObjectCache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ObjectCache.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/HybridAIController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/HybridAIController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/EnhancedBotAI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/EnhancedBotAI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/BotAI_EventHandlers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/AdaptiveAIUpdateThrottler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/AdaptiveAIUpdateThrottler.h

    # AI Decision Systems
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Decision/DecisionFusionSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Decision/DecisionFusionSystem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Decision/ActionPriorityQueue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Decision/ActionPriorityQueue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Decision/BehaviorTree.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Decision/BehaviorTree.h

    # AI Common
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Common/ActionScoringEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Common/ActionScoringEngine.h

    # AI Services
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Services/ThreatAssistant.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Services/ThreatAssistant.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Services/HealingTargetSelector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Services/HealingTargetSelector.h

    # AI Strategy
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Strategy/Strategy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Strategy/Strategy.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Strategy/GroupCombatStrategy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Strategy/GroupCombatStrategy.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Strategy/SoloStrategy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Strategy/SoloStrategy.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Strategy/QuestStrategy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Strategy/QuestStrategy.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Strategy/LootStrategy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Strategy/LootStrategy.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Strategy/RestStrategy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Strategy/RestStrategy.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Strategy/SoloCombatStrategy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Strategy/SoloCombatStrategy.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Strategy/GrindStrategy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Strategy/GrindStrategy.h

    # AI Actions
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Actions/Action.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Actions/Action.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Actions/CommonActions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Actions/CommonActions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Actions/TargetAssistAction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Actions/TargetAssistAction.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Actions/SpellInterruptAction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Actions/SpellInterruptAction.h

    # AI Triggers
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Triggers/Trigger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Triggers/Trigger.h

    # AI Values
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Values/Value.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Values/Value.h

    # Utility AI
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Utility/UtilitySystem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Utility/UtilityContextBuilder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Utility/Evaluators/CombatEvaluators.h

    # Behavior Tree
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/BehaviorTree/BehaviorTree.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/BehaviorTree/Nodes/CombatNodes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/BehaviorTree/Nodes/HealingNodes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/BehaviorTree/Nodes/MovementNodes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/BehaviorTree/BehaviorTreeFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/BehaviorTree/BehaviorTreeFactory.h

    # AI Cache (Thread-safe state caching for worker threads)
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Cache/AuraStateCache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Cache/AuraStateCache.h

    # AI Coordination
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/RoleCoordinator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/RoleCoordinator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/RaidOrchestrator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/RaidOrchestrator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/ZoneOrchestrator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/ZoneOrchestrator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/ContentContextManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/ContentContextManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/GroupBuffCoordinator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/GroupBuffCoordinator.h

    # AI Coordination - Dungeon
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Dungeon/DungeonState.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Dungeon/DungeonCoordinator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Dungeon/DungeonCoordinator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Dungeon/TrashPullManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Dungeon/TrashPullManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Dungeon/BossEncounterManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Dungeon/BossEncounterManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Dungeon/WipeRecoveryManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Dungeon/WipeRecoveryManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Dungeon/MythicPlusManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Dungeon/MythicPlusManager.cpp

    # AI Coordination - Messaging (Bot-to-Bot Communication)
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Messaging/MessageTypes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Messaging/BotMessage.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Messaging/ClaimResolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Messaging/ClaimResolver.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Messaging/BotMessageBus.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Messaging/BotMessageBus.h

    # ============================================================================
    # PHASE 3 Sprint 5: Raid Coordination Files
    # ============================================================================

    # AI Coordination - Raid Core
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidState.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidCoordinator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidCoordinator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidTankCoordinator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidTankCoordinator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidHealCoordinator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidHealCoordinator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidCooldownRotation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidCooldownRotation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidGroupManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidGroupManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidEncounterManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidEncounterManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidPositioningManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidPositioningManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/KitingManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/KitingManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/AddManagementSystem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/AddManagementSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/TankSwapCoordinator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/TankSwapCoordinator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/CooldownSyncCoordinator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/CooldownSyncCoordinator.cpp

    # ============================================================================
    # PHASE 3 Sprint 6: Arena Coordination Files
    # ============================================================================

    # AI Coordination - Arena Core
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/ArenaState.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/ArenaCoordinator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/ArenaCoordinator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/ArenaPositioning.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/ArenaPositioning.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/BurstCoordinator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/BurstCoordinator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/CCChainManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/CCChainManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/DefensiveCoordinator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/DefensiveCoordinator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/KillTargetManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/KillTargetManager.cpp

    # ============================================================================
    # PHASE 3: Supporting Infrastructure Files
    # ============================================================================

    # AI Blackboard
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Blackboard/SharedBlackboard.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Blackboard/SharedBlackboard.h

    # AI Integration
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Integration/IntegratedAIContext.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Integration/IntegratedAIContext.h

    # Specialized AI Factory
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/SpecializedAIFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/SpecializedAIFactory.h

    # AI Coordination - Battleground Core
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/BGState.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/BGSpatialQueryCache.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/BGSpatialQueryCache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/BGPositionDiscovery.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/BGPositionDiscovery.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/BattlegroundCoordinator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/BattlegroundCoordinator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/BattlegroundCoordinatorManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/BattlegroundCoordinatorManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/ObjectiveManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/ObjectiveManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/BGRoleManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/BGRoleManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/FlagCarrierManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/FlagCarrierManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/NodeController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/NodeController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/BGStrategyEngine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/BGStrategyEngine.cpp

    # AI Coordination - Battleground Scripts
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/IBGScript.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/IBGScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/BGScriptRegistry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/BGScriptRegistry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/BGScriptInit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/BGScriptInit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/BGScriptBase.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/BGScriptBase.cpp

    # CTF Battleground Scripts
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/CTF/CTFScriptBase.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/CTF/CTFScriptBase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/CTF/WarsongGulchData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/CTF/WarsongGulchScript.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/CTF/WarsongGulchScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/CTF/TwinPeaksData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/CTF/TwinPeaksScript.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/CTF/TwinPeaksScript.cpp

    # Domination Battleground Scripts
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Domination/DominationScriptBase.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Domination/DominationScriptBase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Domination/ArathiBasinData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Domination/ArathiBasinScript.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Domination/ArathiBasinScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Domination/EyeOfTheStormData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Domination/EyeOfTheStormScript.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Domination/EyeOfTheStormScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Domination/BattleForGilneasData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Domination/BattleForGilneasScript.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Domination/BattleForGilneasScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Domination/TempleOfKotmoguData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Domination/TempleOfKotmoguScript.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Domination/TempleOfKotmoguScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Domination/DeepwindGorgeData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Domination/DeepwindGorgeScript.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Domination/DeepwindGorgeScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Domination/SeethingShoreData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Domination/SeethingShoreScript.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Domination/SeethingShoreScript.cpp

    # Siege Battleground Scripts
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Siege/SiegeScriptBase.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Siege/SiegeScriptBase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Siege/AlteracValleyData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Siege/AlteracValleyScript.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Siege/AlteracValleyScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Siege/IsleOfConquestData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Siege/IsleOfConquestScript.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Siege/IsleOfConquestScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Siege/StrandOfTheAncientsData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Siege/StrandOfTheAncientsScript.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Siege/StrandOfTheAncientsScript.cpp

    # Resource Race Battleground Scripts
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/ResourceRace/ResourceRaceScriptBase.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/ResourceRace/ResourceRaceScriptBase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/ResourceRace/SilvershardMinesData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/ResourceRace/SilvershardMinesScript.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/ResourceRace/SilvershardMinesScript.cpp

    # Epic Battleground Scripts
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Epic/AshranData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Epic/AshranScript.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Epic/AshranScript.cpp
)

# ============================================================================
# LIBRARY 4: playerbot-classai
# ClassAI implementations for all 13 classes
# ============================================================================
set(PLAYERBOT_CLASSAI_SOURCES
    # ClassAI Base
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/ClassAI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/ClassAI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/ActionPriority.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/ActionPriority.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/ResourceManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/ResourceManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/ClassBehaviorTreeRegistry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/ClassBehaviorTreeRegistry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/ClassSpellDatabase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/ClassSpellDatabase.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/SpellIdValidator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/SpellIdValidator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/CombatSpecializationTemplates.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/ResourceTypes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/RoleSpecializations.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/BaselineRotationManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/BaselineRotationManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/SpellFallbackChain.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/SpellFallbackChain.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/HealingEfficiencyManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/HealingEfficiencyManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/HealingSpellTierData.h

    # ClassAI Common Utilities
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Common/StatusEffectTracker.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Common/CooldownManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Common/RotationHelpers.h

    # Death Knight
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/DeathKnights/DeathKnightAI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/DeathKnights/DeathKnightAI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/DeathKnights/DiseaseManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/DeathKnights/DiseaseManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/DeathKnights/RuneManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/DeathKnights/RuneManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/DeathKnights/DeathKnightTypes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/DeathKnights/BloodDeathKnight.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/DeathKnights/FrostDeathKnight.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/DeathKnights/UnholyDeathKnight.h

    # Demon Hunter
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/DemonHunters/DemonHunterAI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/DemonHunters/DemonHunterAI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/DemonHunters/HavocDemonHunter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/DemonHunters/VengeanceDemonHunter.h

    # Druid
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Druids/DruidAI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Druids/DruidAI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Druids/BalanceDruid.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Druids/FeralDruid.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Druids/GuardianDruid.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Druids/RestorationDruid.h

    # Evoker
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Evokers/EvokerAI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Evokers/EvokerAI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Evokers/DevastationEvoker.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Evokers/PreservationEvoker.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Evokers/AugmentationEvoker.h

    # Hunter
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Hunters/HunterAI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Hunters/HunterAI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Hunters/BeastMasteryHunter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Hunters/MarksmanshipHunter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Hunters/SurvivalHunter.h

    # Mage
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Mages/MageAI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Mages/MageAI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Mages/ArcaneMage.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Mages/FireMage.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Mages/FrostMage.h

    # Monk
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Monks/MonkAI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Monks/MonkAI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Monks/BrewmasterMonk.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Monks/MistweaverMonk.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Monks/WindwalkerMonk.h

    # Paladin
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Paladins/PaladinAI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Paladins/PaladinAI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Paladins/HolyPaladin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Paladins/ProtectionPaladin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Paladins/RetributionPaladin.h

    # Priest
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Priests/PriestAI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Priests/PriestAI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Priests/DisciplinePriest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Priests/HolyPriest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Priests/ShadowPriest.h

    # Rogue
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Rogues/RogueAI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Rogues/RogueAI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Rogues/AssassinationRogue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Rogues/OutlawRogue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Rogues/SubtletyRogue.h

    # Shaman
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Shamans/ShamanAI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Shamans/ShamanAI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Shamans/ElementalShaman.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Shamans/EnhancementShaman.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Shamans/RestorationShaman.h

    # Warlock
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Warlocks/WarlockAI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Warlocks/WarlockAI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Warlocks/AfflictionWarlock.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Warlocks/DemonologyWarlock.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Warlocks/DestructionWarlock.h

    # Warrior
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Warriors/WarriorAI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Warriors/WarriorAI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Warriors/ArmsWarrior.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Warriors/FuryWarrior.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI/Warriors/ProtectionWarrior.h
)

# ============================================================================
# LIBRARY 5: playerbot-combat
# Combat systems: AI/Combat, CombatBehaviors
# ============================================================================
set(PLAYERBOT_COMBAT_SOURCES
    # AI Combat Core
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/CombatStateAnalyzer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/CombatStateAnalyzer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/AdaptiveBehaviorManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/AdaptiveBehaviorManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/CombatBehaviorIntegration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/CombatBehaviorIntegration.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/TargetManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/TargetManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/CrowdControlManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/CrowdControlManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/DefensiveManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/DefensiveManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/MovementIntegration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/MovementIntegration.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/GroupCombatTrigger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/GroupCombatTrigger.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/RoleBasedCombatPositioning.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/RoleBasedCombatPositioning.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/PositionManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/PositionManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/MechanicAwareness.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/MechanicAwareness.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/InterruptCoordinator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/InterruptCoordinator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/CombatCoordinationIntegrator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/CombatCoordinationIntegrator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/AddPrioritySystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/AddPrioritySystem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/IncomingDamagePredictor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/IncomingDamagePredictor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/PreBurstResourcePooling.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/PreBurstResourcePooling.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/CombatPhaseDetector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/CombatPhaseDetector.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/TrinketUsageManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/TrinketUsageManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/InterruptAwareness.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/InterruptAwareness.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/InterruptManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/InterruptManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/TargetSelector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/TargetSelector.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/TargetScanner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/TargetScanner.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/CombatStateManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/CombatStateManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/BotThreatManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/BotThreatManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/ThreatAbilities.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/ThreatAbilities.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/ThreatCoordinator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/ThreatCoordinator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/FormationManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/FormationManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/LineOfSightManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/LineOfSightManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/ProactiveLoSFixer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/ProactiveLoSFixer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/CombatMetricsTracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/CombatMetricsTracker.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/StructuredCombatLog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/StructuredCombatLog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/RacialAbilityManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/RacialAbilityManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/ProcExpiryMonitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/ProcExpiryMonitor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/InterruptDatabase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/InterruptDatabase.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/CombatAIIntegrator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/CombatAIIntegrator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/TTKEstimator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat/TTKEstimator.h

    # Combat Behaviors
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/CombatBehaviors/DefensiveBehaviorManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/CombatBehaviors/DefensiveBehaviorManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/CombatBehaviors/InterruptRotationManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/CombatBehaviors/InterruptRotationManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/CombatBehaviors/DispelCoordinator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/CombatBehaviors/DispelCoordinator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/CombatBehaviors/AoEDecisionManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/CombatBehaviors/AoEDecisionManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/CombatBehaviors/CooldownStackingOptimizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/CombatBehaviors/CooldownStackingOptimizer.h

    # Combat Event Bus
    ${CMAKE_CURRENT_SOURCE_DIR}/Combat/CombatEvents.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Combat/CombatEvents.h

    # Cooldown Event Bus
    ${CMAKE_CURRENT_SOURCE_DIR}/Cooldown/CooldownEvents.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Cooldown/CooldownEvents.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Cooldown/MajorCooldownDatabase.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Cooldown/MajorCooldownTracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Cooldown/MajorCooldownTracker.h

    # Aura Event Bus
    ${CMAKE_CURRENT_SOURCE_DIR}/Aura/AuraEvents.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Aura/AuraEvents.h

    # Spatial Grid System
    ${CMAKE_CURRENT_SOURCE_DIR}/Spatial/DoubleBufferedSpatialGrid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Spatial/DoubleBufferedSpatialGrid.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Spatial/SpatialGridManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Spatial/SpatialGridManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Spatial/SpatialGridScheduler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Spatial/SpatialGridScheduler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Spatial/SpatialGridQueryHelpers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Spatial/SpatialGridQueryHelpers.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Spatial/BotClusterDetector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Spatial/BotClusterDetector.h
)

# ============================================================================
# LIBRARY 6: playerbot-lifecycle
# Lifecycle management, Instance system, JIT factory
# ============================================================================
set(PLAYERBOT_LIFECYCLE_SOURCES
    # Account Management
    ${CMAKE_CURRENT_SOURCE_DIR}/Account/BotAccountMgr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Account/BotAccountMgr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Account/AccountLinkingManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Account/AccountLinkingManager.h

    # Character Management
    ${CMAKE_CURRENT_SOURCE_DIR}/Character/BotNameMgr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Character/BotNameMgr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Character/BotCharacterDistribution.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Character/BotCharacterDistribution.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Character/BotLevelDistribution.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Character/BotLevelDistribution.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Character/BotCustomizationGenerator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Character/BotCustomizationGenerator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Character/BotLevelManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Character/BotLevelManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Character/ZoneLevelHelper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Character/ZoneLevelHelper.h

    # Companion Systems
    ${CMAKE_CURRENT_SOURCE_DIR}/Companion/MountManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Companion/MountManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Companion/RidingManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Companion/RidingManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Companion/BattlePetManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Companion/BattlePetManager.h

    # Humanization System (Phase 3)
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Core/ActivityType.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Core/PersonalityProfile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Core/PersonalityProfile.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Core/HumanizationConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Core/HumanizationConfig.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Core/HumanizationManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Core/HumanizationManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Core/ActivityExecutor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Core/ActivityExecutor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Sessions/ActivitySession.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Sessions/ActivitySession.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Sessions/ActivitySessionManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Sessions/ActivitySessionManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Sessions/SessionTransitions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Sessions/SessionTransitions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Sessions/FishingSessionManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Sessions/FishingSessionManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Activities/CityLifeBehaviorManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Activities/CityLifeBehaviorManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Activities/AFKSimulator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Activities/AFKSimulator.h

    # RPG Daily Routine System
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Activities/RPGDailyRoutineManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Activities/RPGDailyRoutineManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Activities/RPGActivityScheduler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Activities/RPGActivityScheduler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Activities/ZoneSelector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Activities/ZoneSelector.h

    # Achievement System (Phase 3)
    ${CMAKE_CURRENT_SOURCE_DIR}/Achievements/AchievementManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Achievements/AchievementManager.h

    # GOD_TIER Task 7: Mount Collection System
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Mounts/MountCollectionManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Mounts/MountCollectionManager.h

    # GOD_TIER Task 8: Pet Collection System
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Collections/PetCollectionManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Collections/PetCollectionManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Collections/TransmogManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Collections/TransmogManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Collections/ArchaeologyManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Collections/ArchaeologyManager.h

    # GOD_TIER Task 10: Achievement Grinder
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Achievements/AchievementGrinder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Achievements/AchievementGrinder.h

    # Reputation System (Phase 3)
    ${CMAKE_CURRENT_SOURCE_DIR}/Reputation/ReputationGrindManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Reputation/ReputationGrindManager.h

    # Economy System (Phase 3)
    ${CMAKE_CURRENT_SOURCE_DIR}/Economy/GoldFarmingManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Economy/GoldFarmingManager.h

    # Lifecycle Management
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/BotLifecycleManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/BotLifecycleManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/BotSpawner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/BotSpawner.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/BotCharacterCreator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/BotCharacterCreator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/BotScheduler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/BotScheduler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/BotLifecycleMgr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/BotLifecycleMgr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/DeathRecoveryManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/DeathRecoveryManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/CorpseCrashMitigation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/CorpseCrashMitigation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/ResourceMonitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/ResourceMonitor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/SpawnPriorityQueue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/SpawnPriorityQueue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/SpawnCircuitBreaker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/SpawnCircuitBreaker.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/AdaptiveSpawnThrottler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/AdaptiveSpawnThrottler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/StartupSpawnOrchestrator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/StartupSpawnOrchestrator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/BotSpawnOrchestrator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/BotSpawnOrchestrator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/BotResourcePool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/BotResourcePool.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/BotSpawnEventBus.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/BotSpawnEventBus.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/BotSpawnEvents.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/BotLifecycleState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/BotLifecycleState.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/BotFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/BotFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/BotSaveController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/BotSaveController.h

    # Protection System
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Protection/ProtectionReason.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Protection/ProtectionStatus.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Protection/IProtectionProvider.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Protection/BotProtectionRegistry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Protection/BotProtectionRegistry.h

    # Retirement System
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Retirement/RetirementState.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Retirement/RetirementCandidate.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Retirement/GracefulExitHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Retirement/GracefulExitHandler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Retirement/BotRetirementManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Retirement/BotRetirementManager.h

    # Prediction System
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Prediction/BracketFlowPredictor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Prediction/BracketFlowPredictor.h

    # Demand System
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Demand/PlayerActivityTracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Demand/PlayerActivityTracker.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Demand/DemandCalculator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Demand/DemandCalculator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Demand/PopulationPIDController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Demand/PopulationPIDController.h

    # Population Controller
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/PopulationLifecycleController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/PopulationLifecycleController.h

    # Instance Bot Pool System
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance/PoolSlotState.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance/InstanceBotSlot.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance/PoolConfiguration.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance/PoolStatistics.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance/InstanceBotPool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance/InstanceBotPool.h

    # JIT Bot Factory System
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance/BotTemplateRepository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance/BotTemplateRepository.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance/BotCloneEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance/BotCloneEngine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance/BotPostLoginConfigurator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance/BotPostLoginConfigurator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance/JITBotFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance/JITBotFactory.h

    # Queue Systems
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance/QueueStatePoller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance/QueueStatePoller.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance/QueueShortageSubscriber.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance/QueueShortageSubscriber.h

    # Instance Bot Orchestrator
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance/ContentRequirements.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance/ContentRequirements.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance/InstanceBotOrchestrator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance/InstanceBotOrchestrator.h

    # Instance Bot Hooks and Scripts
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance/InstanceBotHooks.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance/InstanceBotHooks.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance/InstanceBotScripts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance/InstanceBotScripts.h

    # WoW 12.0 Character Creation Data
    ${CMAKE_CURRENT_SOURCE_DIR}/Data/WoW120CharacterCreation.h
)

# ============================================================================
# LIBRARY 7: playerbot-gameplay
# Quest, Movement, Interaction, Social, Economy, Equipment, Talents, Group, Network
# ============================================================================
set(PLAYERBOT_GAMEPLAY_SOURCES
    # Quest System
    ${CMAKE_CURRENT_SOURCE_DIR}/Quest/QuestPickup.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Quest/QuestPickup.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Quest/DynamicQuestSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Quest/DynamicQuestSystem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Quest/DynamicSpawnHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Quest/DynamicSpawnHandler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Quest/ObjectiveTracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Quest/ObjectiveTracker.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Quest/QuestHubDatabase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Quest/QuestHubDatabase.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Quest/QuestCompletion.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Quest/QuestCompletion.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Quest/QuestTurnIn.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Quest/QuestTurnIn.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Quest/QuestValidation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Quest/QuestValidation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Quest/UnifiedQuestManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Quest/UnifiedQuestManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Quest/QuestEvents.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Quest/QuestEvents.h

    # Game Systems
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/QuestAcceptanceManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/QuestAcceptanceManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/InventoryManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/InventoryManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/VendorPurchaseManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/VendorPurchaseManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/ConsumableManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/ConsumableManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/ReagentManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/ReagentManager.h

    # Movement System
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/Core/MovementTypes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/Core/MovementValidator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/Core/MovementValidator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/Pathfinding/PathfindingAdapter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/Pathfinding/PathfindingAdapter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/Pathfinding/PathOptimizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/Pathfinding/PathOptimizer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/Pathfinding/NavMeshInterface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/Pathfinding/NavMeshInterface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/RoadNetwork/RoadNetworkTypes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/RoadNetwork/RoadSpatialIndex.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/RoadNetwork/RoadSpatialIndex.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/RoadNetwork/RoadGraphPathfinder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/RoadNetwork/RoadGraphPathfinder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/RoadNetwork/RoadNetworkManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/RoadNetwork/RoadNetworkManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/LeaderFollowBehavior.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/LeaderFollowBehavior.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/SwimmingBreathManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/SwimmingBreathManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/Arbiter/MovementPriorityMapper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/Arbiter/MovementPriorityMapper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/Arbiter/MovementRequest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/Arbiter/MovementRequest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/Arbiter/MovementArbiter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/Arbiter/MovementArbiter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovementUtil.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovementUtil.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotWorldPositioner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotWorldPositioner.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/QuestPathfinder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/QuestPathfinder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/UnifiedMovementCoordinator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/UnifiedMovementCoordinator.h

    # BotMovement System (Controller, StuckDetector, StateMachine, Pathfinding, Validation)
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Core/BotMovementConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Core/BotMovementConfig.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Core/BotMovementController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Core/BotMovementController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Core/BotMovementDefines.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Core/BotMovementManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Core/BotMovementManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Core/MovementMetrics.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Core/ValidationResult.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Detection/RecoveryStrategies.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Detection/RecoveryStrategies.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Detection/StuckDetector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Detection/StuckDetector.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Generators/BotFollowMovementGenerator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Generators/BotFollowMovementGenerator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Generators/BotMovementGeneratorBase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Generators/BotMovementGeneratorBase.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Generators/BotPointMovementGenerator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Generators/BotPointMovementGenerator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Pathfinding/PathCache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Pathfinding/PathCache.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Pathfinding/ValidatedPathGenerator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Pathfinding/ValidatedPathGenerator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/StateMachine/FallingMovementState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/StateMachine/FallingMovementState.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/StateMachine/GroundMovementState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/StateMachine/GroundMovementState.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/StateMachine/IdleState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/StateMachine/IdleState.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/StateMachine/MovementState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/StateMachine/MovementState.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/StateMachine/MovementStateMachine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/StateMachine/MovementStateMachine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/StateMachine/StuckState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/StateMachine/StuckState.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/StateMachine/SwimmingMovementState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/StateMachine/SwimmingMovementState.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Validation/CollisionValidator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Validation/CollisionValidator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Validation/GroundValidator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Validation/GroundValidator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Validation/LiquidValidator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Validation/LiquidValidator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Validation/PositionValidator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Validation/PositionValidator.h

    # Travel System
    ${CMAKE_CURRENT_SOURCE_DIR}/Travel/TravelRouteManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Travel/TravelRouteManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Travel/PortalDatabase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Travel/PortalDatabase.h

    # Interaction System
    ${CMAKE_CURRENT_SOURCE_DIR}/Interaction/Core/InteractionManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Interaction/Core/InteractionManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Interaction/Core/InteractionTypes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Interaction/Core/GossipHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Interaction/Core/GossipHandler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Interaction/Core/InteractionValidator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Interaction/Core/InteractionValidator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Interaction/VendorInteractionManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Interaction/VendorInteractionManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Interaction/FlightMasterManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Interaction/FlightMasterManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Interaction/InnkeeperInteractionManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Interaction/InnkeeperInteractionManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Interaction/MailInteractionManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Interaction/MailInteractionManager.h

    # Group System
    ${CMAKE_CURRENT_SOURCE_DIR}/Group/GroupInvitationHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Group/GroupInvitationHandler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Group/GroupMemberResolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Group/GroupMemberResolver.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Group/RoleDefinitions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Group/RoleDefinitions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Group/RoleAssignment.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Group/RoleAssignment.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Group/GroupRoleEnums.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Group/GroupRoleUtils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Group/GroupEvents.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Group/GroupEvents.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Group/GroupEventHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Group/GroupEventHandler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Group/PlayerbotGroupScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Group/PlayerbotGroupScript.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Group/GroupStateMachine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Group/GroupStateMachine.h

    # Advanced Group Coordination
    ${CMAKE_CURRENT_SOURCE_DIR}/Advanced/GroupCoordinator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Advanced/GroupCoordinator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Advanced/TacticalCoordinator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Advanced/TacticalCoordinator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Advanced/SocialManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Advanced/SocialManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Advanced/AdvancedBehaviorManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Advanced/AdvancedBehaviorManager.h

    # Network Packet Handling
    ${CMAKE_CURRENT_SOURCE_DIR}/Network/PlayerbotPacketSniffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Network/PlayerbotPacketSniffer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Network/ParseGroupPacket_Typed.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Network/ParseCombatPacket_Typed.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Network/ParseCooldownPacket_Typed.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Network/ParseLootPacket_Typed.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Network/ParseQuestPacket_Typed.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Network/ParseAuraPacket_Typed.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Network/ParseResourcePacket_Typed.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Network/ParseSocialPacket_Typed.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Network/ParseAuctionPacket_Typed.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Network/ParseNPCPacket_Typed.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Network/ParseInstancePacket_Typed.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Network/ParseBattlegroundPacket_Typed.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Network/ParseLFGPacket_Typed.cpp

    # Social Systems
    ${CMAKE_CURRENT_SOURCE_DIR}/Social/SocialEvents.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Social/SocialEvents.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Social/AuctionHouse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Social/AuctionHouse.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Social/MarketAnalysis.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Social/MarketAnalysis.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Social/GuildIntegration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Social/GuildIntegration.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Social/GuildBankManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Social/GuildBankManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Social/GuildEventCoordinator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Social/GuildEventCoordinator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Social/LootDistribution.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Social/LootDistribution.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Social/UnifiedLootManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Social/UnifiedLootManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Social/TradeManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Social/TradeManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Social/TradeManager_Events.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Social/TradeSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Social/TradeSystem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Social/GuildTaskManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Social/GuildTaskManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Social/SummonResponseManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Social/SummonResponseManager.h

    # Economy Systems
    ${CMAKE_CURRENT_SOURCE_DIR}/Economy/AuctionManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Economy/AuctionManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Economy/AuctionManager_Events.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Economy/PlayerbotAuctionConfig.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Banking/BankingManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Banking/BankingManager.h

    # Auction Event Bus
    ${CMAKE_CURRENT_SOURCE_DIR}/Auction/AuctionEvents.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Auction/AuctionEvents.h

    # NPC Event Bus
    ${CMAKE_CURRENT_SOURCE_DIR}/NPC/NPCEvents.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPC/NPCEvents.h

    # Instance Event Bus
    ${CMAKE_CURRENT_SOURCE_DIR}/Instance/InstanceEvents.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Instance/InstanceEvents.h

    # Loot Event Bus
    ${CMAKE_CURRENT_SOURCE_DIR}/Loot/LootEvents.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Loot/LootEvents.h

    # Resource Event Bus
    ${CMAKE_CURRENT_SOURCE_DIR}/Resource/ResourceEvents.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Resource/ResourceEvents.h

    # Equipment System
    ${CMAKE_CURRENT_SOURCE_DIR}/Equipment/EquipmentManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Equipment/EquipmentManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Equipment/BotGearFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Equipment/BotGearFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Equipment/EnchantGemDatabase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Equipment/EnchantGemDatabase.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Equipment/EnchantGemManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Equipment/EnchantGemManager.h

    # Talent System
    ${CMAKE_CURRENT_SOURCE_DIR}/Talents/BotTalentManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Talents/BotTalentManager.h

    # Profession System
    ${CMAKE_CURRENT_SOURCE_DIR}/Professions/ProfessionManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Professions/ProfessionManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Professions/ProfessionDatabase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Professions/ProfessionDatabase.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Professions/GatheringManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Professions/GatheringManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Professions/FarmingCoordinator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Professions/FarmingCoordinator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Professions/ProfessionAuctionBridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Professions/ProfessionAuctionBridge.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Professions/GatheringMaterialsBridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Professions/GatheringMaterialsBridge.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Professions/AuctionMaterialsBridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Professions/AuctionMaterialsBridge.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Professions/ProfessionEvents.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Professions/ProfessionEvents.h

    # Packets
    ${CMAKE_CURRENT_SOURCE_DIR}/Packets/SpellPacketBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Packets/SpellPacketBuilder.h
)

# ============================================================================
# LIBRARY 8: playerbot-pvp
# PvP, LFG, Battleground, Arena, Dragonriding, Dungeon, Scripts, Performance
# ============================================================================
set(PLAYERBOT_PVP_SOURCES
    # PvP Systems
    ${CMAKE_CURRENT_SOURCE_DIR}/PvP/ArenaAI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PvP/ArenaAI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PvP/PvPCombatAI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PvP/PvPCombatAI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PvP/BattlegroundAI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PvP/BattlegroundAI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PvP/BGBotManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PvP/BGBotManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PvP/ArenaBotManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PvP/ArenaBotManager.h

    # LFG System
    ${CMAKE_CURRENT_SOURCE_DIR}/LFG/LFGBotManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LFG/LFGBotManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LFG/LFGBotSelector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LFG/LFGBotSelector.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LFG/LFGRoleDetector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LFG/LFGRoleDetector.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LFG/LFGGroupCoordinator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LFG/LFGGroupCoordinator.h

    # Dungeon System
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/DungeonAutonomyManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/DungeonAutonomyManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/DungeonBehavior.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/DungeonBehavior.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/DelveBehaviorManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/DelveBehaviorManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/DungeonScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/DungeonScript.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/DungeonScriptLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/DungeonScriptLoader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/DungeonScriptMgr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/DungeonScriptMgr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/EncounterStrategy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/EncounterStrategy.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/InstanceCoordination.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/InstanceCoordination.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/InstanceFarmingManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/InstanceFarmingManager.h

    # Dungeon Scripts - Vanilla
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/Scripts/Vanilla/BlackfathomDeepsScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/Scripts/Vanilla/DeadminesScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/Scripts/Vanilla/GnomereganScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/Scripts/Vanilla/RagefireChasmScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/Scripts/Vanilla/RazorfenDownsScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/Scripts/Vanilla/RazorfenKraulScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/Scripts/Vanilla/ScarletMonasteryScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/Scripts/Vanilla/ShadowfangKeepScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/Scripts/Vanilla/StockadeScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon/Scripts/Vanilla/WailingCavernsScript.cpp

    # Dragonriding System
    ${CMAKE_CURRENT_SOURCE_DIR}/Dragonriding/DragonridingDefines.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Dragonriding/DragonridingMgr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Dragonriding/DragonridingMgr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Dragonriding/DragonridingAI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Dragonriding/DragonridingAI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Dragonriding/PlayerbotDragonridingScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Dragonriding/PlayerbotDragonridingGlyphScript.cpp

    # Script Integration
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/PlayerbotWorldScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/PlayerbotWorldScript.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/PlayerbotLFGScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/PlayerbotBGScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/PlayerbotArenaScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/PlayerbotSpellFixes.cpp

    # Performance Optimization
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/StringInterningPool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/StringInterningPool.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/BandwidthTelemetry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/BandwidthTelemetry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/ThreadPool/ThreadPool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/ThreadPool/ThreadPool.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/ThreadPool/ThreadPoolDiagnostics.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/ThreadPool/DeadlockDetector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/ThreadPool/DeadlockDetector.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/ThreadPool/ThreadPoolCommands.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/ThreadPool/ContinuousDiagnosticLogger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/ThreadPool/ContinuousDiagnosticLogger.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/ThreadPool/DebuggerIntegration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/ThreadPool/DebuggerIntegration.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/ThreadPool/ETWProvider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/ThreadPool/ETWProvider.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/MemoryPool/MemoryPool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/MemoryPool/MemoryPool.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/QueryOptimizer/QueryOptimizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/QueryOptimizer/QueryOptimizer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/Profiler/Profiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/Profiler/Profiler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/PerformanceManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/PerformanceManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/ObjectPool.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/PacketPoolManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/ShardedMetricsCollector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/ShardedMetricsCollector.h
)

# ============================================================================
# Common include directories for all libraries
# ============================================================================
set(PLAYERBOT_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Config
    ${CMAKE_CURRENT_SOURCE_DIR}/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/References
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Managers
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Events
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/DI
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/DI/Interfaces
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Combat
    ${CMAKE_CURRENT_SOURCE_DIR}/Events
    ${CMAKE_CURRENT_SOURCE_DIR}/Session
    ${CMAKE_CURRENT_SOURCE_DIR}/Chat
    ${CMAKE_CURRENT_SOURCE_DIR}/Database
    ${CMAKE_CURRENT_SOURCE_DIR}/AI
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Strategy
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Actions
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Triggers
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Values
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/ClassAI
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Combat
    ${CMAKE_CURRENT_SOURCE_DIR}/Group
    ${CMAKE_CURRENT_SOURCE_DIR}/Interfaces
    ${CMAKE_CURRENT_SOURCE_DIR}/Adapters
    ${CMAKE_CURRENT_SOURCE_DIR}/Economy
    ${CMAKE_CURRENT_SOURCE_DIR}/LFG
    ${CMAKE_CURRENT_SOURCE_DIR}/PvP
    ${CMAKE_CURRENT_SOURCE_DIR}/Packets
    ${CMAKE_CURRENT_SOURCE_DIR}/Dragonriding
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle
    ${CMAKE_CURRENT_SOURCE_DIR}/Lifecycle/Instance
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Detection
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Generators
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Pathfinding
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/StateMachine
    ${CMAKE_CURRENT_SOURCE_DIR}/Movement/BotMovement/Validation
    ${CMAKE_CURRENT_SOURCE_DIR}/Threading
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/CTF
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Domination
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Siege
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/ResourceRace
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Battleground/Scripts/Epic
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Dungeon
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Messaging
    ${CMAKE_CURRENT_SOURCE_DIR}/Dungeon
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Activities
    ${CMAKE_CURRENT_SOURCE_DIR}/Humanization/Sessions
    ${CMAKE_SOURCE_DIR}/src/modules/Common
    ${MYSQL_INCLUDE_DIR}
)

# ============================================================================
# Common compile options function
# ============================================================================
function(configure_playerbot_library TARGET_NAME)
    set_target_properties(${TARGET_NAME} PROPERTIES
        FOLDER "modules/playerbot"
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    target_include_directories(${TARGET_NAME}
        PUBLIC ${PLAYERBOT_INCLUDE_DIRS}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Account
            ${CMAKE_CURRENT_SOURCE_DIR}/Character
            ${CMAKE_CURRENT_SOURCE_DIR}/Scripts
            ${CMAKE_CURRENT_BINARY_DIR}
    )

    target_link_libraries(${TARGET_NAME}
        PUBLIC
            game-interface
            shared
            database
            common
            modules-common
            playerbot-dependencies
        PRIVATE
            ${MYSQL_LIBRARY}
    )

    target_compile_definitions(${TARGET_NAME}
        PRIVATE
            BUILD_PLAYERBOT=1
            TRINITY_API_EXPORT_PLAYERBOT
            $<$<CONFIG:Debug>:PLAYERBOT_DEBUG=1>
            $<$<CONFIG:Release>:PLAYERBOT_RELEASE=1>
    )

    if(WIN32)
        target_compile_definitions(${TARGET_NAME} PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            _CRT_SECURE_NO_WARNINGS
        )
        target_link_libraries(${TARGET_NAME} PRIVATE ws2_32 wsock32 mswsock)
    elseif(UNIX)
        target_link_libraries(${TARGET_NAME} PRIVATE pthread dl)
    endif()

    # Enterprise optimization flags
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_CONFIGURATION_TYPES)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${TARGET_NAME} PRIVATE
                $<$<CONFIG:Release>:-O3>
                $<$<CONFIG:Release>:-march=native>
                $<$<CONFIG:Release>:-mtune=native>
                $<$<CONFIG:Release>:-flto>
                $<$<CONFIG:Release>:-ffast-math>
                $<$<CONFIG:Release>:-funroll-loops>
                $<$<CONFIG:Release>:-ftree-vectorize>
            )
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
            target_compile_options(${TARGET_NAME} PRIVATE
                $<$<CONFIG:Release>:/O2>
                $<$<CONFIG:Release>:/GL>
                $<$<CONFIG:Release>:/fp:fast>
                $<$<CONFIG:Release>:/arch:AVX2>
                /Gy
                /Gw
            )
            target_link_options(${TARGET_NAME} PRIVATE $<$<CONFIG:Release>:/LTCG>)
        endif()
    endif()
endfunction()

# ============================================================================
# Create individual static libraries
# ============================================================================

# Library 1: Core
add_library(playerbot-core STATIC ${PLAYERBOT_CORE_SOURCES})
configure_playerbot_library(playerbot-core)

# Library 2: Database
add_library(playerbot-database STATIC ${PLAYERBOT_DATABASE_SOURCES})
configure_playerbot_library(playerbot-database)
target_link_libraries(playerbot-database PRIVATE playerbot-core)

# Library 3: AI Base
add_library(playerbot-ai-base STATIC ${PLAYERBOT_AI_BASE_SOURCES})
configure_playerbot_library(playerbot-ai-base)
target_link_libraries(playerbot-ai-base PRIVATE playerbot-core)

# Library 4: ClassAI
add_library(playerbot-classai STATIC ${PLAYERBOT_CLASSAI_SOURCES})
configure_playerbot_library(playerbot-classai)
target_link_libraries(playerbot-classai PRIVATE playerbot-ai-base)

# Library 5: Combat
add_library(playerbot-combat STATIC ${PLAYERBOT_COMBAT_SOURCES})
configure_playerbot_library(playerbot-combat)
target_link_libraries(playerbot-combat PRIVATE playerbot-ai-base)

# Library 6: Lifecycle
add_library(playerbot-lifecycle STATIC ${PLAYERBOT_LIFECYCLE_SOURCES})
configure_playerbot_library(playerbot-lifecycle)
target_link_libraries(playerbot-lifecycle PRIVATE playerbot-core)

# Library 7: Gameplay
add_library(playerbot-gameplay STATIC ${PLAYERBOT_GAMEPLAY_SOURCES})
configure_playerbot_library(playerbot-gameplay)
target_link_libraries(playerbot-gameplay PRIVATE playerbot-core playerbot-combat)

# Library 8: PvP
add_library(playerbot-pvp STATIC ${PLAYERBOT_PVP_SOURCES})
configure_playerbot_library(playerbot-pvp)
target_link_libraries(playerbot-pvp PRIVATE playerbot-lifecycle playerbot-gameplay)

# ============================================================================
# Create the main playerbot INTERFACE library that aggregates all sub-libraries
# ============================================================================
# NOTE: Cross-library dependencies require special linking.
# GameSystemsManager (playerbot-core) uses types from playerbot-combat and playerbot-gameplay.
# On Linux/macOS with GCC/Clang, static library link order matters.
#
# Platform-specific solutions:
# - Linux: Use --start-group/--end-group via LINK_GROUP:RESCAN
# - macOS: Repeat libraries that provide cross-dependencies
# - Windows: MSVC handles this automatically
# ============================================================================
add_library(playerbot INTERFACE)

if(UNIX AND NOT APPLE)
    # Linux: Use LINK_GROUP:RESCAN for circular dependency resolution
    # This uses -Wl,--start-group ... -Wl,--end-group
    target_link_libraries(playerbot INTERFACE
        $<LINK_GROUP:RESCAN,
            playerbot-core,
            playerbot-database,
            playerbot-ai-base,
            playerbot-classai,
            playerbot-combat,
            playerbot-lifecycle,
            playerbot-gameplay,
            playerbot-pvp
        >
    )
elseif(APPLE)
    # macOS: ld64 doesn't support --start-group, so we repeat libraries
    # to ensure symbols from combat/gameplay are available to core
    target_link_libraries(playerbot INTERFACE
        playerbot-pvp
        playerbot-gameplay
        playerbot-combat
        playerbot-lifecycle
        playerbot-classai
        playerbot-ai-base
        playerbot-database
        playerbot-core
        # Repeat libraries that provide symbols needed by core
        playerbot-combat
        playerbot-gameplay
    )
else()
    # Windows (MSVC): Standard order works due to different linker behavior
    target_link_libraries(playerbot INTERFACE
        playerbot-core
        playerbot-database
        playerbot-ai-base
        playerbot-classai
        playerbot-combat
        playerbot-lifecycle
        playerbot-gameplay
        playerbot-pvp
    )
endif()

# ============================================================================
# Install configuration
# ============================================================================
if(CONF_DIR)
    install(FILES conf/playerbots.conf.dist DESTINATION ${CONF_DIR}/modules)

    install(DIRECTORY DESTINATION ${CONF_DIR}/sql)
    install(DIRECTORY DESTINATION ${CONF_DIR}/sql/migrations)

    install(FILES
        sql/migrations/001_playerbot_base.sql
        sql/migrations/002_population_lifecycle.sql
        sql/migrations/003_bot_templates.sql
        DESTINATION ${CONF_DIR}/sql/migrations
    )

    message(STATUS "Playerbot migration files will be installed to: ${CONF_DIR}/sql/migrations/")
endif()

if(SQL_DIR)
    install(DIRECTORY sql/ DESTINATION ${SQL_DIR}/playerbot)
endif()

# ============================================================================
# Link to game target
# ============================================================================
if(TARGET game)
    target_link_libraries(game PRIVATE playerbot)
    target_compile_definitions(game PRIVATE BUILD_PLAYERBOT=1)
endif()

message(STATUS "✅ Playerbot module configured with 8 sub-libraries (4GB limit workaround)")
message(STATUS "   - playerbot-core")
message(STATUS "   - playerbot-database")
message(STATUS "   - playerbot-ai-base")
message(STATUS "   - playerbot-classai")
message(STATUS "   - playerbot-combat")
message(STATUS "   - playerbot-lifecycle")
message(STATUS "   - playerbot-gameplay")
message(STATUS "   - playerbot-pvp")
