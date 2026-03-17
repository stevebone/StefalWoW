/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "PlayerbotSubsystem.h"

namespace Playerbot
{

// ============================================================================
// Registration function - call from PlayerbotModule::InitializeManagers()
// ============================================================================

void RegisterAllSubsystems();

// ============================================================================
// Init-order subsystems (initOrder > 0)
// ============================================================================

// 100 - CRITICAL
class BotAccountMgrSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Update(uint32 diff) override;
    void Shutdown() override;
};

// 110 - CRITICAL
class BotNameMgrSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Shutdown() override;
};

// 120 - CRITICAL
class BotCharacterDistributionSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
};

// 130 - CRITICAL
class BotWorldSessionMgrSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Update(uint32 diff) override;
    void Shutdown() override;
};

// 140 - NORMAL
class BotPacketRelaySubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Shutdown() override;
};

// 150 - NORMAL
class BotChatCommandHandlerSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Shutdown() override;
};

// 160 - NORMAL
class ProfessionDatabaseSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
};

// 170 - NORMAL
class ClassBehaviorTreeRegistrySubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
};

// 180 - CRITICAL
class QuestHubDatabaseSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
};

// 190 - HIGH
class PortalDatabaseSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
};

// 200 - NORMAL
// 195 - NORMAL (before BotGearFactory so enchant data is available)
class EnchantGemDatabaseSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
};

class BotGearFactorySubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
};

// 210 - NORMAL
class PlayerbotPacketSnifferSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Shutdown() override;
};

// 220 - NORMAL
class BGLFGPacketHandlersSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
};

// 230 - NORMAL
class MajorCooldownTrackerSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
};

// 240 - NORMAL
class BotActionManagerSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Update(uint32 diff) override;
    void Shutdown() override;
};

// 250 - HIGH
class BotProtectionRegistrySubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Update(uint32 diff) override;
    void Shutdown() override;
};

// 260 - HIGH (dependency: BotProtectionRegistry)
class BotRetirementManagerSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Update(uint32 diff) override;
    void Shutdown() override;
};

// 270 - HIGH
class BracketFlowPredictorSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Update(uint32 diff) override;
    void Shutdown() override;
};

// 280 - HIGH
class PlayerActivityTrackerSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Update(uint32 diff) override;
    void Shutdown() override;
};

// 290 - HIGH (dependencies: ActivityTracker, ProtectionRegistry, FlowPredictor)
class DemandCalculatorSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Update(uint32 diff) override;
    void Shutdown() override;
};

// 300 - HIGH
class PopulationLifecycleCtrlSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Update(uint32 diff) override;
    void Shutdown() override;
};

// 310 - HIGH
class ContentRequirementDbSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
};

// 320 - HIGH
class BotTemplateRepositorySubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Shutdown() override;
};

// 330 - HIGH
class BotCloneEngineSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Shutdown() override;
};

// 340 - HIGH
class BotPostLoginConfiguratorSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Shutdown() override;
};

// 350 - HIGH
class InstanceBotPoolSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Update(uint32 diff) override;
    void Shutdown() override;
};

// 360 - HIGH
class JITBotFactorySubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Update(uint32 diff) override;
    void Shutdown() override;
};

// 370 - HIGH
class QueueStatePollerSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Update(uint32 diff) override;
    void Shutdown() override;
};

// 380 - HIGH
class QueueShortageSubscriberSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Shutdown() override;
};

// 390 - HIGH
class InstanceBotOrchestratorSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Update(uint32 diff) override;
    void Shutdown() override;
};

// 400 - HIGH
class InstanceBotHooksSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Shutdown() override;
};

// 410 - NORMAL (special: PrintStatus before Shutdown)
class BotOperationTrackerSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Shutdown() override;
};

// ============================================================================
// Update-only subsystems (initOrder = 0)
// ============================================================================

// updateOrder=200, init handled by PlayerbotModuleAdapter
class BotSpawnerSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Update(uint32 diff) override;
};

// updateOrder=400, init/shutdown handled by PlayerbotModule directly
class PlayerbotCharDBSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Update(uint32 diff) override;
};

// updateOrder=500
class GroupEventBusSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Update(uint32 diff) override;
};

// updateOrder=600, combines 11 domain EventBuses + 60s health monitor
class DomainEventBusProcessorSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Update(uint32 diff) override;
};

// updateOrder=700 - Server Load Monitor (reaction delay scaling)
class ServerLoadMonitorSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Update(uint32 diff) override;
};

// initOrder=420, updateOrder=800 - Guild Task System
class GuildTaskManagerSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Update(uint32 diff) override;
    void Shutdown() override;
};

// initOrder=430 - Account Linking System
class AccountLinkingManagerSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Shutdown() override;
};

// initOrder=440 - Bot Cheat Mask (debug/testing)
class BotCheatMaskSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
};

// initOrder=450 - Bot Save Controller (P6/P3: save tiering + differential saves)
class BotSaveControllerSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Shutdown() override;
};

// initOrder=155 - Road Network Manager (road-aware bot pathfinding)
class RoadNetworkSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Shutdown() override;
};

// updateOrder=900 - Bot Cluster Detector (R1: anti-cluster dispersal)
class BotClusterDetectorSubsystem final : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override;
    bool Initialize() override;
    void Update(uint32 diff) override;
};

} // namespace Playerbot
