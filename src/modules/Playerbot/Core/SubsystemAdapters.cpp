/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "SubsystemAdapters.h"
#include "PlayerbotSubsystemRegistry.h"

// Manager includes (same order as PlayerbotModule.cpp)
#include "Config/PlayerbotConfig.h"
#include "Account/BotAccountMgr.h"
#include "Character/BotNameMgr.h"
#include "Character/BotCharacterDistribution.h"
#include "Database/PlayerbotCharacterDBInterface.h"
#include "Session/BotWorldSessionMgr.h"
#include "Session/BotPacketRelay.h"
#include "Chat/BotChatCommandHandler.h"
#include "Professions/ProfessionDatabase.h"
#include "AI/ClassAI/ClassBehaviorTreeRegistry.h"
#include "Quest/QuestHubDatabase.h"
#include "Travel/PortalDatabase.h"
#include "Equipment/BotGearFactory.h"
#include "Equipment/EnchantGemDatabase.h"
#include "Network/PlayerbotPacketSniffer.h"
#include "Cooldown/MajorCooldownTracker.h"
#include "Threading/BotActionManager.h"
#include "Lifecycle/Protection/BotProtectionRegistry.h"
#include "Lifecycle/Retirement/BotRetirementManager.h"
#include "Lifecycle/Prediction/BracketFlowPredictor.h"
#include "Lifecycle/Demand/PlayerActivityTracker.h"
#include "Lifecycle/Demand/DemandCalculator.h"
#include "Lifecycle/PopulationLifecycleController.h"
#include "Lifecycle/Instance/ContentRequirements.h"
#include "Lifecycle/Instance/BotTemplateRepository.h"
#include "Lifecycle/Instance/BotCloneEngine.h"
#include "Lifecycle/Instance/BotPostLoginConfigurator.h"
#include "Lifecycle/Instance/InstanceBotPool.h"
#include "Lifecycle/Instance/JITBotFactory.h"
#include "Lifecycle/Instance/QueueStatePoller.h"
#include "Lifecycle/Instance/QueueShortageSubscriber.h"
#include "Lifecycle/Instance/InstanceBotOrchestrator.h"
#include "Lifecycle/Instance/InstanceBotHooks.h"
#include "Core/Diagnostics/BotOperationTracker.h"
#include "Lifecycle/BotSpawner.h"
#include "Session/ServerLoadMonitor.h"
#include "Social/GuildTaskManager.h"
#include "Account/AccountLinkingManager.h"
#include "Core/Diagnostics/BotCheatMask.h"
#include "Lifecycle/BotSaveController.h"
#include "Spatial/BotClusterDetector.h"
#include "Movement/RoadNetwork/RoadNetworkManager.h"

// EventBus includes
#include "Group/GroupEvents.h"
#include "Core/Events/GenericEventBus.h"
#include "Combat/CombatEvents.h"
#include "Loot/LootEvents.h"
#include "Quest/QuestEvents.h"
#include "Aura/AuraEvents.h"
#include "Cooldown/CooldownEvents.h"
#include "Resource/ResourceEvents.h"
#include "Social/SocialEvents.h"
#include "Auction/AuctionEvents.h"
#include "NPC/NPCEvents.h"
#include "Instance/InstanceEvents.h"
#include "Professions/ProfessionEvents.h"

// BG/LFG packet handlers (forward declarations for registration functions)
#include "Network/PlayerbotPacketSniffer.h"

#include "GameTime.h"
#include "Log.h"
#include <memory>

namespace Playerbot
{

// ============================================================================
// #1: BotAccountMgr (initOrder=100, updateOrder=100, shutdownOrder=2300) CRITICAL
// ============================================================================

SubsystemInfo BotAccountMgrSubsystem::GetInfo() const
{
    return { "BotAccountMgr", SubsystemPriority::CRITICAL, 100, 100, 2300 };
}

bool BotAccountMgrSubsystem::Initialize()
{
    if (!BotAccountMgr::instance()->Initialize())
    {
        // StrictMode-aware: if strict mode disabled, continue despite failure
        bool strictMode = sPlayerbotConfig->GetBool("Playerbot.StrictCharacterLimit", true);
        if (strictMode)
            return false;

        TC_LOG_WARN("module.playerbot",
            "BotAccountMgr initialization failed - continuing (strict mode disabled)");
    }
    return true;
}

void BotAccountMgrSubsystem::Update(uint32 diff)
{
    BotAccountMgr::instance()->Update(diff);
}

void BotAccountMgrSubsystem::Shutdown()
{
    BotAccountMgr::instance()->Shutdown();
}

// ============================================================================
// #2: BotNameMgr (initOrder=110, shutdownOrder=2200) CRITICAL
// ============================================================================

SubsystemInfo BotNameMgrSubsystem::GetInfo() const
{
    return { "BotNameMgr", SubsystemPriority::CRITICAL, 110, 0, 2200 };
}

bool BotNameMgrSubsystem::Initialize()
{
    return BotNameMgr::instance()->Initialize();
}

void BotNameMgrSubsystem::Shutdown()
{
    BotNameMgr::instance()->Shutdown();
}

// ============================================================================
// #3: BotCharacterDistribution (initOrder=120) CRITICAL
// Special: Uses LoadFromDatabase() not Initialize()
// ============================================================================

SubsystemInfo BotCharacterDistributionSubsystem::GetInfo() const
{
    return { "BotCharacterDistribution", SubsystemPriority::CRITICAL, 120, 0, 0 };
}

bool BotCharacterDistributionSubsystem::Initialize()
{
    return BotCharacterDistribution::instance()->LoadFromDatabase();
}

// ============================================================================
// #4: BotWorldSessionMgr (initOrder=130, updateOrder=300, shutdownOrder=2100) CRITICAL
// Special: Uses UpdateSessions(diff) not Update(diff)
// ============================================================================

SubsystemInfo BotWorldSessionMgrSubsystem::GetInfo() const
{
    return { "BotWorldSessionMgr", SubsystemPriority::CRITICAL, 130, 300, 2100 };
}

bool BotWorldSessionMgrSubsystem::Initialize()
{
    return BotWorldSessionMgr::instance()->Initialize();
}

void BotWorldSessionMgrSubsystem::Update(uint32 diff)
{
    BotWorldSessionMgr::instance()->UpdateSessions(diff);
}

void BotWorldSessionMgrSubsystem::Shutdown()
{
    BotWorldSessionMgr::instance()->Shutdown();
}

// ============================================================================
// #5: BotPacketRelay (initOrder=140, shutdownOrder=1900) NORMAL
// Static class methods
// ============================================================================

SubsystemInfo BotPacketRelaySubsystem::GetInfo() const
{
    return { "BotPacketRelay", SubsystemPriority::NORMAL, 140, 0, 1900 };
}

bool BotPacketRelaySubsystem::Initialize()
{
    BotPacketRelay::Initialize();
    return true;
}

void BotPacketRelaySubsystem::Shutdown()
{
    BotPacketRelay::Shutdown();
}

// ============================================================================
// #6: BotChatCommandHandler (initOrder=150, shutdownOrder=1800) NORMAL
// Static class methods
// ============================================================================

SubsystemInfo BotChatCommandHandlerSubsystem::GetInfo() const
{
    return { "BotChatCommandHandler", SubsystemPriority::NORMAL, 150, 0, 1800 };
}

bool BotChatCommandHandlerSubsystem::Initialize()
{
    BotChatCommandHandler::Initialize();
    return true;
}

void BotChatCommandHandlerSubsystem::Shutdown()
{
    BotChatCommandHandler::Shutdown();
}

// ============================================================================
// #7: ProfessionDatabase (initOrder=160) NORMAL
// ============================================================================

SubsystemInfo ProfessionDatabaseSubsystem::GetInfo() const
{
    return { "ProfessionDatabase", SubsystemPriority::NORMAL, 160, 0, 0 };
}

bool ProfessionDatabaseSubsystem::Initialize()
{
    ProfessionDatabase::instance()->Initialize();
    return true;
}

// ============================================================================
// #8: ClassBehaviorTreeRegistry (initOrder=170) NORMAL
// Static class method
// ============================================================================

SubsystemInfo ClassBehaviorTreeRegistrySubsystem::GetInfo() const
{
    return { "ClassBehaviorTreeRegistry", SubsystemPriority::NORMAL, 170, 0, 0 };
}

bool ClassBehaviorTreeRegistrySubsystem::Initialize()
{
    ClassBehaviorTreeRegistry::Initialize();
    return true;
}

// ============================================================================
// #9: QuestHubDatabase (initOrder=180) CRITICAL
// Uses QuestHubDatabase::Instance() (reference, not pointer)
// ============================================================================

SubsystemInfo QuestHubDatabaseSubsystem::GetInfo() const
{
    return { "QuestHubDatabase", SubsystemPriority::CRITICAL, 180, 0, 0 };
}

bool QuestHubDatabaseSubsystem::Initialize()
{
    return QuestHubDatabase::Instance().Initialize();
}

// ============================================================================
// #10: PortalDatabase (initOrder=190) HIGH
// Non-fatal: uses fallback portal detection on failure
// ============================================================================

SubsystemInfo PortalDatabaseSubsystem::GetInfo() const
{
    return { "PortalDatabase", SubsystemPriority::HIGH, 190, 0, 0 };
}

bool PortalDatabaseSubsystem::Initialize()
{
    return PortalDatabase::Instance().Initialize();
}

// ============================================================================
// #10.5: EnchantGemDatabase (initOrder=195) NORMAL
// ============================================================================

SubsystemInfo EnchantGemDatabaseSubsystem::GetInfo() const
{
    return { "EnchantGemDatabase", SubsystemPriority::NORMAL, 195, 0, 0 };
}

bool EnchantGemDatabaseSubsystem::Initialize()
{
    Playerbot::EnchantGemDatabase::Initialize();
    return true;
}

// ============================================================================
// #11: BotGearFactory (initOrder=200) NORMAL
// ============================================================================

SubsystemInfo BotGearFactorySubsystem::GetInfo() const
{
    return { "BotGearFactory", SubsystemPriority::NORMAL, 200, 0, 0 };
}

bool BotGearFactorySubsystem::Initialize()
{
    BotGearFactory::instance()->Initialize();
    return true;
}

// ============================================================================
// #12: PlayerbotPacketSniffer (initOrder=210, shutdownOrder=2000) NORMAL
// Static class methods
// ============================================================================

SubsystemInfo PlayerbotPacketSnifferSubsystem::GetInfo() const
{
    return { "PlayerbotPacketSniffer", SubsystemPriority::NORMAL, 210, 0, 2000 };
}

bool PlayerbotPacketSnifferSubsystem::Initialize()
{
    PlayerbotPacketSniffer::Initialize();
    return true;
}

void PlayerbotPacketSnifferSubsystem::Shutdown()
{
    PlayerbotPacketSniffer::Shutdown();
}

// ============================================================================
// #13: BGLFGPacketHandlers (initOrder=220) NORMAL
// Two static registration calls combined
// ============================================================================

SubsystemInfo BGLFGPacketHandlersSubsystem::GetInfo() const
{
    return { "BGLFGPacketHandlers", SubsystemPriority::NORMAL, 220, 0, 0 };
}

bool BGLFGPacketHandlersSubsystem::Initialize()
{
    RegisterBattlegroundPacketHandlers();
    RegisterLFGPacketHandlers();
    return true;
}

// ============================================================================
// #14: MajorCooldownTracker (initOrder=230) NORMAL
// ============================================================================

SubsystemInfo MajorCooldownTrackerSubsystem::GetInfo() const
{
    return { "MajorCooldownTracker", SubsystemPriority::NORMAL, 230, 0, 0 };
}

bool MajorCooldownTrackerSubsystem::Initialize()
{
    MajorCooldownTracker::instance()->Initialize();
    return true;
}

// ============================================================================
// #15: BotActionManager (initOrder=240, updateOrder=250, shutdownOrder=100) NORMAL
// Processes deferred actions queued by worker threads (e.g. GO interactions).
// updateOrder=250 runs early in the tick to drain the queue from the previous tick.
// ============================================================================

SubsystemInfo BotActionManagerSubsystem::GetInfo() const
{
    return { "BotActionManager", SubsystemPriority::NORMAL, 240, 250, 100 };
}

bool BotActionManagerSubsystem::Initialize()
{
    BotActionManager::Instance()->Initialize();
    return true;
}

void BotActionManagerSubsystem::Update(uint32 /*diff*/)
{
    // Drain the action queue on the main thread.
    // Worker threads queue actions (INTERACT_OBJECT, ENTER_VEHICLE, etc.)
    // that require main-thread Map/Grid access. Without this call,
    // queued actions accumulate forever and are never executed.
    BotActionManager::Instance()->ProcessActions();
}

void BotActionManagerSubsystem::Shutdown()
{
    BotActionManager::Instance()->Shutdown();
}

// ============================================================================
// #16: BotProtectionRegistry (initOrder=250, updateOrder=700, shutdownOrder=1700) HIGH
// ============================================================================

SubsystemInfo BotProtectionRegistrySubsystem::GetInfo() const
{
    return { "BotProtectionRegistry", SubsystemPriority::HIGH, 250, 700, 1700 };
}

bool BotProtectionRegistrySubsystem::Initialize()
{
    return BotProtectionRegistry::Instance()->Initialize();
}

void BotProtectionRegistrySubsystem::Update(uint32 diff)
{
    BotProtectionRegistry::Instance()->Update(diff);
}

void BotProtectionRegistrySubsystem::Shutdown()
{
    BotProtectionRegistry::Instance()->Shutdown();
}

// ============================================================================
// #17: BotRetirementManager (initOrder=260, updateOrder=800, shutdownOrder=1600) HIGH
// SPECIAL: Wire SetProtectionRegistry BEFORE Initialize
// ============================================================================

SubsystemInfo BotRetirementManagerSubsystem::GetInfo() const
{
    return { "BotRetirementManager", SubsystemPriority::HIGH, 260, 800, 1600 };
}

bool BotRetirementManagerSubsystem::Initialize()
{
    // CRITICAL: Wire dependency before initialization
    BotRetirementManager::Instance()->SetProtectionRegistry(
        BotProtectionRegistry::Instance());
    return BotRetirementManager::Instance()->Initialize();
}

void BotRetirementManagerSubsystem::Update(uint32 diff)
{
    BotRetirementManager::Instance()->Update(diff);
}

void BotRetirementManagerSubsystem::Shutdown()
{
    BotRetirementManager::Instance()->Shutdown();
}

// ============================================================================
// #18: BracketFlowPredictor (initOrder=270, updateOrder=900, shutdownOrder=1500) HIGH
// ============================================================================

SubsystemInfo BracketFlowPredictorSubsystem::GetInfo() const
{
    return { "BracketFlowPredictor", SubsystemPriority::HIGH, 270, 900, 1500 };
}

bool BracketFlowPredictorSubsystem::Initialize()
{
    return BracketFlowPredictor::Instance()->Initialize();
}

void BracketFlowPredictorSubsystem::Update(uint32 diff)
{
    BracketFlowPredictor::Instance()->Update(diff);
}

void BracketFlowPredictorSubsystem::Shutdown()
{
    BracketFlowPredictor::Instance()->Shutdown();
}

// ============================================================================
// #19: PlayerActivityTracker (initOrder=280, updateOrder=1000, shutdownOrder=1400) HIGH
// ============================================================================

SubsystemInfo PlayerActivityTrackerSubsystem::GetInfo() const
{
    return { "PlayerActivityTracker", SubsystemPriority::HIGH, 280, 1000, 1400 };
}

bool PlayerActivityTrackerSubsystem::Initialize()
{
    return PlayerActivityTracker::Instance()->Initialize();
}

void PlayerActivityTrackerSubsystem::Update(uint32 diff)
{
    PlayerActivityTracker::Instance()->Update(diff);
}

void PlayerActivityTrackerSubsystem::Shutdown()
{
    PlayerActivityTracker::Instance()->Shutdown();
}

// ============================================================================
// #20: DemandCalculator (initOrder=290, updateOrder=1100, shutdownOrder=1300) HIGH
// SPECIAL: Initialize first, THEN wire 3 dependencies
// ============================================================================

SubsystemInfo DemandCalculatorSubsystem::GetInfo() const
{
    return { "DemandCalculator", SubsystemPriority::HIGH, 290, 1100, 1300 };
}

bool DemandCalculatorSubsystem::Initialize()
{
    if (!DemandCalculator::Instance()->Initialize())
        return false;

    // Wire dependencies AFTER successful init (matches current PlayerbotModule.cpp behavior)
    DemandCalculator::Instance()->SetActivityTracker(PlayerActivityTracker::Instance());
    DemandCalculator::Instance()->SetProtectionRegistry(BotProtectionRegistry::Instance());
    DemandCalculator::Instance()->SetFlowPredictor(BracketFlowPredictor::Instance());
    return true;
}

void DemandCalculatorSubsystem::Update(uint32 diff)
{
    DemandCalculator::Instance()->Update(diff);
}

void DemandCalculatorSubsystem::Shutdown()
{
    DemandCalculator::Instance()->Shutdown();
}

// ============================================================================
// #21: PopulationLifecycleCtrl (initOrder=300, updateOrder=1200, shutdownOrder=200) HIGH
// ============================================================================

SubsystemInfo PopulationLifecycleCtrlSubsystem::GetInfo() const
{
    return { "PopulationLifecycleCtrl", SubsystemPriority::HIGH, 300, 1200, 200 };
}

bool PopulationLifecycleCtrlSubsystem::Initialize()
{
    return PopulationLifecycleController::Instance()->Initialize();
}

void PopulationLifecycleCtrlSubsystem::Update(uint32 diff)
{
    PopulationLifecycleController::Instance()->Update(diff);
}

void PopulationLifecycleCtrlSubsystem::Shutdown()
{
    PopulationLifecycleController::Instance()->Shutdown();
}

// ============================================================================
// #22: ContentRequirementDb (initOrder=310) HIGH
// ============================================================================

SubsystemInfo ContentRequirementDbSubsystem::GetInfo() const
{
    return { "ContentRequirementDb", SubsystemPriority::HIGH, 310, 0, 0 };
}

bool ContentRequirementDbSubsystem::Initialize()
{
    return ContentRequirementDatabase::Instance()->Initialize();
}

// ============================================================================
// #23: BotTemplateRepository (initOrder=320, shutdownOrder=1100) HIGH
// ============================================================================

SubsystemInfo BotTemplateRepositorySubsystem::GetInfo() const
{
    return { "BotTemplateRepository", SubsystemPriority::HIGH, 320, 0, 1100 };
}

bool BotTemplateRepositorySubsystem::Initialize()
{
    return BotTemplateRepository::Instance()->Initialize();
}

void BotTemplateRepositorySubsystem::Shutdown()
{
    BotTemplateRepository::Instance()->Shutdown();
}

// ============================================================================
// #24: BotCloneEngine (initOrder=330, shutdownOrder=900) HIGH
// ============================================================================

SubsystemInfo BotCloneEngineSubsystem::GetInfo() const
{
    return { "BotCloneEngine", SubsystemPriority::HIGH, 330, 0, 900 };
}

bool BotCloneEngineSubsystem::Initialize()
{
    return BotCloneEngine::Instance()->Initialize();
}

void BotCloneEngineSubsystem::Shutdown()
{
    BotCloneEngine::Instance()->Shutdown();
}

// ============================================================================
// #25: BotPostLoginConfigurator (initOrder=340, shutdownOrder=1000) HIGH
// ============================================================================

SubsystemInfo BotPostLoginConfiguratorSubsystem::GetInfo() const
{
    return { "BotPostLoginConfigurator", SubsystemPriority::HIGH, 340, 0, 1000 };
}

bool BotPostLoginConfiguratorSubsystem::Initialize()
{
    return BotPostLoginConfigurator::Instance()->Initialize();
}

void BotPostLoginConfiguratorSubsystem::Shutdown()
{
    BotPostLoginConfigurator::Instance()->Shutdown();
}

// ============================================================================
// #26: InstanceBotPool (initOrder=350, updateOrder=1300, shutdownOrder=800) HIGH
// ============================================================================

SubsystemInfo InstanceBotPoolSubsystem::GetInfo() const
{
    return { "InstanceBotPool", SubsystemPriority::HIGH, 350, 1300, 800 };
}

bool InstanceBotPoolSubsystem::Initialize()
{
    return InstanceBotPool::Instance()->Initialize();
}

void InstanceBotPoolSubsystem::Update(uint32 diff)
{
    InstanceBotPool::Instance()->Update(diff);
}

void InstanceBotPoolSubsystem::Shutdown()
{
    InstanceBotPool::Instance()->Shutdown();
}

// ============================================================================
// #27: JITBotFactory (initOrder=360, updateOrder=1500, shutdownOrder=700) HIGH
// ============================================================================

SubsystemInfo JITBotFactorySubsystem::GetInfo() const
{
    return { "JITBotFactory", SubsystemPriority::HIGH, 360, 1500, 700 };
}

bool JITBotFactorySubsystem::Initialize()
{
    return JITBotFactory::Instance()->Initialize();
}

void JITBotFactorySubsystem::Update(uint32 diff)
{
    JITBotFactory::Instance()->Update(diff);
}

void JITBotFactorySubsystem::Shutdown()
{
    JITBotFactory::Instance()->Shutdown();
}

// ============================================================================
// #28: QueueStatePoller (initOrder=370, updateOrder=1600, shutdownOrder=600) HIGH
// ============================================================================

SubsystemInfo QueueStatePollerSubsystem::GetInfo() const
{
    return { "QueueStatePoller", SubsystemPriority::HIGH, 370, 1600, 600 };
}

bool QueueStatePollerSubsystem::Initialize()
{
    return QueueStatePoller::Instance()->Initialize();
}

void QueueStatePollerSubsystem::Update(uint32 diff)
{
    QueueStatePoller::Instance()->Update(diff);
}

void QueueStatePollerSubsystem::Shutdown()
{
    QueueStatePoller::Instance()->Shutdown();
}

// ============================================================================
// #29: QueueShortageSubscriber (initOrder=380, shutdownOrder=500) HIGH
// ============================================================================

SubsystemInfo QueueShortageSubscriberSubsystem::GetInfo() const
{
    return { "QueueShortageSubscriber", SubsystemPriority::HIGH, 380, 0, 500 };
}

bool QueueShortageSubscriberSubsystem::Initialize()
{
    return QueueShortageSubscriber::Instance()->Initialize();
}

void QueueShortageSubscriberSubsystem::Shutdown()
{
    QueueShortageSubscriber::Instance()->Shutdown();
}

// ============================================================================
// #30: InstanceBotOrchestrator (initOrder=390, updateOrder=1400, shutdownOrder=400) HIGH
// ============================================================================

SubsystemInfo InstanceBotOrchestratorSubsystem::GetInfo() const
{
    return { "InstanceBotOrchestrator", SubsystemPriority::HIGH, 390, 1400, 400 };
}

bool InstanceBotOrchestratorSubsystem::Initialize()
{
    return InstanceBotOrchestrator::Instance()->Initialize();
}

void InstanceBotOrchestratorSubsystem::Update(uint32 diff)
{
    InstanceBotOrchestrator::Instance()->Update(diff);
}

void InstanceBotOrchestratorSubsystem::Shutdown()
{
    InstanceBotOrchestrator::Instance()->Shutdown();
}

// ============================================================================
// #31: InstanceBotHooks (initOrder=400, shutdownOrder=300) HIGH
// Static class methods
// ============================================================================

SubsystemInfo InstanceBotHooksSubsystem::GetInfo() const
{
    return { "InstanceBotHooks", SubsystemPriority::HIGH, 400, 0, 300 };
}

bool InstanceBotHooksSubsystem::Initialize()
{
    return InstanceBotHooks::Initialize();
}

void InstanceBotHooksSubsystem::Shutdown()
{
    InstanceBotHooks::Shutdown();
}

// ============================================================================
// #32: BotOperationTracker (initOrder=410, shutdownOrder=1200) NORMAL
// SPECIAL: PrintStatus() BEFORE Shutdown()
// ============================================================================

SubsystemInfo BotOperationTrackerSubsystem::GetInfo() const
{
    return { "BotOperationTracker", SubsystemPriority::NORMAL, 410, 0, 1200 };
}

bool BotOperationTrackerSubsystem::Initialize()
{
    BotOperationTracker::instance()->Initialize();
    return true;
}

void BotOperationTrackerSubsystem::Shutdown()
{
    BotOperationTracker::instance()->PrintStatus();  // Print final report FIRST
    BotOperationTracker::instance()->Shutdown();
}

// ============================================================================
// #33: BotSpawner (updateOrder=200) - Update only
// Init handled by PlayerbotModuleAdapter::OnModuleStartup()
// ============================================================================

SubsystemInfo BotSpawnerSubsystem::GetInfo() const
{
    return { "BotSpawner", SubsystemPriority::NORMAL, 0, 200, 0 };
}

bool BotSpawnerSubsystem::Initialize()
{
    return true; // Init handled by PlayerbotModuleAdapter
}

void BotSpawnerSubsystem::Update(uint32 diff)
{
    BotSpawner::instance()->Update(diff);
}

// ============================================================================
// #34: PlayerbotCharDB (updateOrder=400) - Update only
// Init/Shutdown handled by PlayerbotModule directly
// ============================================================================

SubsystemInfo PlayerbotCharDBSubsystem::GetInfo() const
{
    return { "PlayerbotCharDB", SubsystemPriority::NORMAL, 0, 400, 0 };
}

bool PlayerbotCharDBSubsystem::Initialize()
{
    return true; // Init handled by PlayerbotModule::InitializeDatabase()
}

void PlayerbotCharDBSubsystem::Update(uint32 diff)
{
    PlayerbotCharacterDBInterface::instance()->Update(diff);
}

// ============================================================================
// #35: GroupEventBus (updateOrder=500) - Update only
// ============================================================================

SubsystemInfo GroupEventBusSubsystem::GetInfo() const
{
    return { "GroupEventBus", SubsystemPriority::NORMAL, 0, 500, 0 };
}

bool GroupEventBusSubsystem::Initialize()
{
    return true;
}

void GroupEventBusSubsystem::Update(uint32 /*diff*/)
{
    EventBus<GroupEvent>::instance()->ProcessEvents(100);
}

// ============================================================================
// #36: DomainEventBusProcessor (updateOrder=600) - Update only
// Combines 11 domain EventBuses + 60-second queue health monitor
// ============================================================================

SubsystemInfo DomainEventBusProcessorSubsystem::GetInfo() const
{
    return { "DomainEventBusProcessor", SubsystemPriority::NORMAL, 0, 600, 0 };
}

bool DomainEventBusProcessorSubsystem::Initialize()
{
    return true;
}

void DomainEventBusProcessorSubsystem::Update(uint32 /*diff*/)
{
    // Process all 11 domain EventBuses with configured batch limits
    uint32 totalDomainEvents = 0;
    totalDomainEvents += EventBus<CombatEvent>::instance()->ProcessEvents(50);
    totalDomainEvents += EventBus<LootEvent>::instance()->ProcessEvents(50);
    totalDomainEvents += EventBus<QuestEvent>::instance()->ProcessEvents(50);
    totalDomainEvents += EventBus<AuraEvent>::instance()->ProcessEvents(30);
    totalDomainEvents += EventBus<CooldownEvent>::instance()->ProcessEvents(30);
    totalDomainEvents += EventBus<ResourceEvent>::instance()->ProcessEvents(30);
    totalDomainEvents += EventBus<SocialEvent>::instance()->ProcessEvents(30);
    totalDomainEvents += EventBus<AuctionEvent>::instance()->ProcessEvents(20);
    totalDomainEvents += EventBus<NPCEvent>::instance()->ProcessEvents(30);
    totalDomainEvents += EventBus<InstanceEvent>::instance()->ProcessEvents(20);
    totalDomainEvents += EventBus<ProfessionEvent>::instance()->ProcessEvents(20);

    if (totalDomainEvents > 0)
    {
        TC_LOG_DEBUG("module.playerbot.events",
            "PlayerbotModule: Processed {} domain events this cycle", totalDomainEvents);
    }

    // Queue Health Monitoring (every 60 seconds)
    static uint32 s_lastQueueReport = 0;
    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - s_lastQueueReport > 60000)
    {
        s_lastQueueReport = currentTime;

        uint32 totalQueued = 0;
        totalQueued += EventBus<CombatEvent>::instance()->GetQueueSize();
        totalQueued += EventBus<LootEvent>::instance()->GetQueueSize();
        totalQueued += EventBus<QuestEvent>::instance()->GetQueueSize();
        totalQueued += EventBus<AuraEvent>::instance()->GetQueueSize();
        totalQueued += EventBus<CooldownEvent>::instance()->GetQueueSize();
        totalQueued += EventBus<ResourceEvent>::instance()->GetQueueSize();
        totalQueued += EventBus<SocialEvent>::instance()->GetQueueSize();
        totalQueued += EventBus<AuctionEvent>::instance()->GetQueueSize();
        totalQueued += EventBus<NPCEvent>::instance()->GetQueueSize();
        totalQueued += EventBus<InstanceEvent>::instance()->GetQueueSize();
        totalQueued += EventBus<ProfessionEvent>::instance()->GetQueueSize();

        if (totalQueued > 0)
        {
            TC_LOG_INFO("module.playerbot.events",
                "EventBus queue health: {} events pending across 11 domain buses", totalQueued);
        }
        else
        {
            TC_LOG_DEBUG("module.playerbot.events",
                "EventBus queue health: All domain buses clear (0 events pending)");
        }
    }
}

// ============================================================================
// #37: GuildTaskManager (initOrder=420, updateOrder=800) NORMAL
// Generates guild tasks and assigns them to bot members for autonomous completion.
// ============================================================================

SubsystemInfo GuildTaskManagerSubsystem::GetInfo() const
{
    return { "GuildTaskManager", SubsystemPriority::NORMAL, 420, 800, 420 };
}

bool GuildTaskManagerSubsystem::Initialize()
{
    return Playerbot::GuildTaskManager::instance()->Initialize();
}

void GuildTaskManagerSubsystem::Update(uint32 diff)
{
    Playerbot::GuildTaskManager::instance()->Update(diff);
}

void GuildTaskManagerSubsystem::Shutdown()
{
    Playerbot::GuildTaskManager::instance()->Shutdown();
}

// ============================================================================
// #38: AccountLinkingManager (initOrder=430) NORMAL
// Links human accounts with bot accounts for permission-based access.
// ============================================================================

SubsystemInfo AccountLinkingManagerSubsystem::GetInfo() const
{
    return { "AccountLinkingManager", SubsystemPriority::NORMAL, 430, 0, 430 };
}

bool AccountLinkingManagerSubsystem::Initialize()
{
    return Playerbot::AccountLinkingManager::instance()->Initialize();
}

void AccountLinkingManagerSubsystem::Shutdown()
{
    Playerbot::AccountLinkingManager::instance()->Shutdown();
}

// ============================================================================
// #39: BotCheatMask (initOrder=440) LOW
// Per-bot cheat system for testing and debugging.
// ============================================================================

SubsystemInfo BotCheatMaskSubsystem::GetInfo() const
{
    return { "BotCheatMask", SubsystemPriority::LOW, 440, 0, 0 };
}

bool BotCheatMaskSubsystem::Initialize()
{
    Playerbot::BotCheatMask::instance()->Initialize();
    return true;
}

// ============================================================================
// #36: ServerLoadMonitor (updateOrder=700) NORMAL
// Monitors server tick performance and provides dynamic reaction delay scaling.
// ============================================================================

SubsystemInfo ServerLoadMonitorSubsystem::GetInfo() const
{
    return { "ServerLoadMonitor", SubsystemPriority::NORMAL, 0, 700, 0 };
}

bool ServerLoadMonitorSubsystem::Initialize()
{
    Playerbot::ServerLoadMonitor::instance()->Initialize();
    return true;
}

void ServerLoadMonitorSubsystem::Update(uint32 diff)
{
    Playerbot::ServerLoadMonitor::instance()->Update(diff);
}

// ============================================================================
// BotSaveControllerSubsystem - initOrder=450
// ============================================================================

SubsystemInfo BotSaveControllerSubsystem::GetInfo() const
{
    return { "BotSaveController", SubsystemPriority::NORMAL, 450, 0, 450 };
}

bool BotSaveControllerSubsystem::Initialize()
{
    TC_LOG_INFO("module.playerbot", "BotSaveController: Initializing save tiering + differential saves");
    return true;
}

void BotSaveControllerSubsystem::Shutdown()
{
    TC_LOG_INFO("module.playerbot", "BotSaveController: Shutdown (skip rate: {:.1f}%)",
        Playerbot::BotSaveController::instance()->GetStats().GetSkipRate() * 100.0f);
}

// ============================================================================
// BotClusterDetectorSubsystem - updateOrder=900
// ============================================================================

SubsystemInfo BotClusterDetectorSubsystem::GetInfo() const
{
    return { "BotClusterDetector", SubsystemPriority::NORMAL, 0, 900, 0 };
}

bool BotClusterDetectorSubsystem::Initialize()
{
    Playerbot::BotClusterDetector::instance()->Initialize();
    return true;
}

void BotClusterDetectorSubsystem::Update(uint32 diff)
{
    Playerbot::BotClusterDetector::instance()->Update(diff);
}

// ============================================================================
// RoadNetworkSubsystem - initOrder=155
// ============================================================================

SubsystemInfo RoadNetworkSubsystem::GetInfo() const
{
    return { "RoadNetwork", SubsystemPriority::NORMAL, 155, 0, 900 };
}

bool RoadNetworkSubsystem::Initialize()
{
    auto* mgr = Playerbot::RoadNetworkManager::Instance();

    bool enabled = sPlayerbotConfig->GetBool("Playerbot.RoadNetwork.Enable", true);
    mgr->SetEnabled(enabled);

    if (!enabled)
    {
        TC_LOG_INFO("module.playerbot", "RoadNetwork: Disabled by configuration");
        return true;
    }

    std::string path = sPlayerbotConfig->GetString("Playerbot.RoadNetwork.DataPath", "roads");
    mgr->SetMinDistance(sPlayerbotConfig->GetFloat("Playerbot.RoadNetwork.MinDistance", 200.0f));
    mgr->SetMaxDetourRatio(sPlayerbotConfig->GetFloat("Playerbot.RoadNetwork.MaxDetourRatio", 1.5f));
    mgr->SetMaxEntryDistance(sPlayerbotConfig->GetFloat("Playerbot.RoadNetwork.MaxEntryDistance", 200.0f));

    return mgr->Initialize(path);
}

void RoadNetworkSubsystem::Shutdown()
{
    Playerbot::RoadNetworkManager::Instance()->Shutdown();
}

// ============================================================================
// REGISTRATION
// ============================================================================

void RegisterAllSubsystems()
{
    auto* registry = PlayerbotSubsystemRegistry::instance();

    // Init-order subsystems (sorted by initOrder)
    registry->RegisterSubsystem(std::make_unique<BotAccountMgrSubsystem>());                // 100
    registry->RegisterSubsystem(std::make_unique<BotNameMgrSubsystem>());                   // 110
    registry->RegisterSubsystem(std::make_unique<BotCharacterDistributionSubsystem>());     // 120
    registry->RegisterSubsystem(std::make_unique<BotWorldSessionMgrSubsystem>());           // 130
    registry->RegisterSubsystem(std::make_unique<BotPacketRelaySubsystem>());               // 140
    registry->RegisterSubsystem(std::make_unique<BotChatCommandHandlerSubsystem>());        // 150
    registry->RegisterSubsystem(std::make_unique<RoadNetworkSubsystem>());                  // 155
    registry->RegisterSubsystem(std::make_unique<ProfessionDatabaseSubsystem>());           // 160
    registry->RegisterSubsystem(std::make_unique<ClassBehaviorTreeRegistrySubsystem>());    // 170
    registry->RegisterSubsystem(std::make_unique<QuestHubDatabaseSubsystem>());             // 180
    registry->RegisterSubsystem(std::make_unique<PortalDatabaseSubsystem>());               // 190
    registry->RegisterSubsystem(std::make_unique<EnchantGemDatabaseSubsystem>());           // 195
    registry->RegisterSubsystem(std::make_unique<BotGearFactorySubsystem>());               // 200
    registry->RegisterSubsystem(std::make_unique<PlayerbotPacketSnifferSubsystem>());       // 210
    registry->RegisterSubsystem(std::make_unique<BGLFGPacketHandlersSubsystem>());          // 220
    registry->RegisterSubsystem(std::make_unique<MajorCooldownTrackerSubsystem>());         // 230
    registry->RegisterSubsystem(std::make_unique<BotActionManagerSubsystem>());             // 240
    registry->RegisterSubsystem(std::make_unique<BotProtectionRegistrySubsystem>());        // 250
    registry->RegisterSubsystem(std::make_unique<BotRetirementManagerSubsystem>());         // 260
    registry->RegisterSubsystem(std::make_unique<BracketFlowPredictorSubsystem>());         // 270
    registry->RegisterSubsystem(std::make_unique<PlayerActivityTrackerSubsystem>());        // 280
    registry->RegisterSubsystem(std::make_unique<DemandCalculatorSubsystem>());             // 290
    registry->RegisterSubsystem(std::make_unique<PopulationLifecycleCtrlSubsystem>());      // 300
    registry->RegisterSubsystem(std::make_unique<ContentRequirementDbSubsystem>());         // 310
    registry->RegisterSubsystem(std::make_unique<BotTemplateRepositorySubsystem>());        // 320
    registry->RegisterSubsystem(std::make_unique<BotCloneEngineSubsystem>());               // 330
    registry->RegisterSubsystem(std::make_unique<BotPostLoginConfiguratorSubsystem>());     // 340
    registry->RegisterSubsystem(std::make_unique<InstanceBotPoolSubsystem>());              // 350
    registry->RegisterSubsystem(std::make_unique<JITBotFactorySubsystem>());                // 360
    registry->RegisterSubsystem(std::make_unique<QueueStatePollerSubsystem>());             // 370
    registry->RegisterSubsystem(std::make_unique<QueueShortageSubscriberSubsystem>());      // 380
    registry->RegisterSubsystem(std::make_unique<InstanceBotOrchestratorSubsystem>());      // 390
    registry->RegisterSubsystem(std::make_unique<InstanceBotHooksSubsystem>());             // 400
    registry->RegisterSubsystem(std::make_unique<BotOperationTrackerSubsystem>());          // 410
    registry->RegisterSubsystem(std::make_unique<GuildTaskManagerSubsystem>());             // 420
    registry->RegisterSubsystem(std::make_unique<AccountLinkingManagerSubsystem>());       // 430
    registry->RegisterSubsystem(std::make_unique<BotCheatMaskSubsystem>());                // 440
    registry->RegisterSubsystem(std::make_unique<BotSaveControllerSubsystem>());            // 450

    // Update-only subsystems (initOrder=0, updateOrder > 0)
    registry->RegisterSubsystem(std::make_unique<BotSpawnerSubsystem>());                   // update=200
    registry->RegisterSubsystem(std::make_unique<PlayerbotCharDBSubsystem>());              // update=400

    // EventBus subsystems (update-only)
    registry->RegisterSubsystem(std::make_unique<GroupEventBusSubsystem>());                // update=500
    registry->RegisterSubsystem(std::make_unique<DomainEventBusProcessorSubsystem>());     // update=600
    registry->RegisterSubsystem(std::make_unique<ServerLoadMonitorSubsystem>());             // update=700

    // Anti-cluster dispersal (update-only)
    registry->RegisterSubsystem(std::make_unique<BotClusterDetectorSubsystem>());           // update=900
}

} // namespace Playerbot
