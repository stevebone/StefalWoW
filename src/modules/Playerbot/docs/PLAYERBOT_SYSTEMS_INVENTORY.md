# Playerbot Systems Inventory

**Generated:** 2026-01-06
**Branch:** playerbot-dev
**Purpose:** Complete inventory of all Playerbot systems and their configuration status

---

## Executive Summary

| Metric | Value |
|--------|-------|
| **Total Systems Found** | 46 major classes |
| **Fully Implemented** | 45 systems |
| **Stub/Partial** | 1 system |
| **Config Wired** | 3 systems (6.5%) |
| **Config NOT Wired** | 43 systems (93.5%) |

**Critical Finding:** 93% of fully implemented systems are NOT reading configuration options from `playerbots.conf.dist`.

---

## System Categories

### 1. TALENTS SYSTEM

| File | Size | Status | Config Wired | Config Options Needed |
|------|------|--------|--------------|----------------------|
| `Talents/BotTalentManager.cpp` | 49,732 bytes | FULL | NO | TalentManager.Enable, UseOptimalBuilds, RandomizeMinor, AdaptToContent, RespecFrequency, UseHeroTalents |
| `Talents/BotTalentManager.h` | - | FULL | - | - |

**Features Implemented:**
- TalentLoadout struct with hero talents support
- SpecChoice with role assignment
- Level-based talent configuration
- Multi-spec support

**Config Wiring Required:**
```cpp
// Add to BotTalentManager::Initialize()
_enabled = sPlayerbotConfig->GetBool("Playerbot.TalentManager.Enable", true);
_useOptimalBuilds = sPlayerbotConfig->GetBool("Playerbot.TalentManager.UseOptimalBuilds", true);
_randomizeMinor = sPlayerbotConfig->GetBool("Playerbot.TalentManager.RandomizeMinor", true);
_adaptToContent = sPlayerbotConfig->GetBool("Playerbot.TalentManager.AdaptToContent", true);
_respecFrequency = sPlayerbotConfig->GetUInt("Playerbot.TalentManager.RespecFrequency", 24);
_useHeroTalents = sPlayerbotConfig->GetBool("Playerbot.TalentManager.UseHeroTalents", true);
```

---

### 2. EQUIPMENT SYSTEM

| File | Size | Status | Config Wired | Config Options Needed |
|------|------|--------|--------------|----------------------|
| `Equipment/BotGearFactory.cpp` | 29,629 bytes | FULL | NO | GearFactory.* |
| `Equipment/BotGearFactory.h` | - | FULL | - | - |
| `Equipment/EquipmentManager.cpp` | 61,376 bytes | FULL | NO | Equipment.* |
| `Equipment/EquipmentManager.h` | - | FULL | - | - |

**Features Implemented:**
- Immutable gear cache for lock-free reads
- Class/spec/level-based item selection
- Integration with EquipmentManager for stat weights
- Faction-specific item handling

**Config Wiring Required:**
```cpp
// Add to BotGearFactory::Initialize()
_config.enabled = sPlayerbotConfig->GetBool("Playerbot.GearFactory.Enable", true);
_config.qualityMin = sPlayerbotConfig->GetUInt("Playerbot.GearFactory.QualityMin", 2);
_config.qualityMax = sPlayerbotConfig->GetUInt("Playerbot.GearFactory.QualityMax", 4);
_config.levelRange = sPlayerbotConfig->GetUInt("Playerbot.GearFactory.LevelRange", 5);
_config.useSpecAppropriate = sPlayerbotConfig->GetBool("Playerbot.GearFactory.UseSpecAppropriate", true);
_config.enchantItems = sPlayerbotConfig->GetBool("Playerbot.GearFactory.EnchantItems", true);
_config.gemItems = sPlayerbotConfig->GetBool("Playerbot.GearFactory.GemItems", true);
_config.refreshInterval = sPlayerbotConfig->GetUInt("Playerbot.GearFactory.RefreshInterval", 60);
```

---

### 3. BANKING SYSTEM

| File | Size | Status | Config Wired | Config Options Needed |
|------|------|--------|--------------|----------------------|
| `Banking/BankingManager.cpp` | 45,503 bytes | FULL | NO | Banking.* |
| `Banking/BankingManager.h` | - | FULL | - | - |

**Features Implemented:**
- Bank NPC interaction
- Deposit/withdrawal logic
- Gold management
- Transaction history

**Config Wiring Required:**
```cpp
// Add to BankingManager::Initialize()
_config.enabled = sPlayerbotConfig->GetBool("Playerbot.Banking.Enable", true);
_config.checkInterval = sPlayerbotConfig->GetUInt("Playerbot.Banking.CheckInterval", 300000);
_config.autoDeposit = sPlayerbotConfig->GetBool("Playerbot.Banking.AutoDeposit", true);
_config.keepGoldAmount = sPlayerbotConfig->GetUInt("Playerbot.Banking.KeepGoldAmount", 10000);
_config.maxTransactionHistory = sPlayerbotConfig->GetUInt("Playerbot.Banking.MaxTransactionHistory", 100);
```

---

### 4. ECONOMY/AUCTION SYSTEM

| File | Size | Status | Config Wired | Config Options Needed |
|------|------|--------|--------------|----------------------|
| `Economy/AuctionManager.cpp` | 38,523 bytes | FULL | **YES** | AuctionHouse.* |
| `Economy/AuctionManager.h` | - | FULL | - | - |
| `Economy/AuctionManager_Events.cpp` | - | FULL | - | - |
| `Economy/AuctionManager_UnitTest.cpp` | - | FULL | - | - |
| `Economy/PlayerbotAuctionConfig.h` | - | FULL | - | - |
| `Social/AuctionHouse.cpp` | 59,860 bytes | FULL | NO | (Duplicate system) |
| `Social/MarketAnalysis.cpp` | 47,607 bytes | FULL | NO | Market.* |

**Note:** Two competing auction implementations exist. Consolidation recommended.

**Config Status:** AuctionManager is WIRED, AuctionHouse.cpp is NOT.

---

### 5. TRADE SYSTEM

| File | Size | Status | Config Wired | Config Options Needed |
|------|------|--------|--------------|----------------------|
| `Social/TradeSystem.cpp` | 61,798 bytes | FULL | NO | Trade.* |
| `Social/TradeSystem.h` | - | FULL | - | - |
| `Social/TradeManager.cpp` | 47,351 bytes | FULL | NO | Trade.* |
| `Social/TradeManager.h` | - | FULL | - | - |

**Note:** Two competing trade implementations exist. Consolidation recommended.

**Config Wiring Required:**
```cpp
// Add to TradeSystem::Initialize() OR TradeManager::Initialize()
_config.enabled = sPlayerbotConfig->GetBool("Playerbot.Trade.Enable", true);
_config.autoVendor = sPlayerbotConfig->GetBool("Playerbot.Trade.AutoVendor", true);
_config.autoRepair = sPlayerbotConfig->GetBool("Playerbot.Trade.AutoRepair", true);
_config.sellGrayItems = sPlayerbotConfig->GetBool("Playerbot.Trade.SellGrayItems", true);
_config.sellWhiteItems = sPlayerbotConfig->GetBool("Playerbot.Trade.SellWhiteItems", false);
_config.minGoldToTrade = sPlayerbotConfig->GetUInt("Playerbot.Trade.MinGoldToTrade", 100);
_config.maxTradeDistance = sPlayerbotConfig->GetFloat("Playerbot.Trade.MaxTradeDistance", 10.0f);
```

---

### 6. SOCIAL SYSTEM

| File | Size | Status | Config Wired | Config Options Needed |
|------|------|--------|--------------|----------------------|
| `Social/GuildEventCoordinator.cpp` | 92,070 bytes | FULL | NO | Social.* |
| `Social/GuildBankManager.cpp` | 33,831 bytes | FULL | NO | Banking.* |
| `Social/GuildIntegration.cpp` | 60,342 bytes | FULL | NO | Social.* |
| `Social/LootDistribution.cpp` | 109,069 bytes | FULL | NO | Loot.* |
| `Social/UnifiedLootManager.cpp` | 78,406 bytes | FULL | NO | Loot.* |
| `Social/SocialEvents.cpp` | - | FULL | NO | - |
| `Advanced/SocialManager.cpp` | 36,928 bytes | FULL | NO | Social.* |

**Config Wiring Required:**
```cpp
// Add to SocialManager::Initialize() or GuildEventCoordinator
_config.enableChat = sPlayerbotConfig->GetBool("Playerbot.Social.EnableChat", false);
_config.chatChance = sPlayerbotConfig->GetUInt("Playerbot.Social.ChatChance", 10);
_config.respondToWhispers = sPlayerbotConfig->GetBool("Playerbot.Social.RespondToWhispers", true);
_config.joinGuilds = sPlayerbotConfig->GetBool("Playerbot.Social.JoinGuilds", false);
_config.formGroups = sPlayerbotConfig->GetBool("Playerbot.Social.FormGroups", true);
_config.acceptInvites = sPlayerbotConfig->GetBool("Playerbot.Social.AcceptInvites", true);
```

---

### 7. GROUP SYSTEM

| File | Size | Status | Config Wired | Config Options Needed |
|------|------|--------|--------------|----------------------|
| `Group/GroupCoordination.cpp` | 41,501 bytes | FULL | NO | Group.* |
| `Group/GroupFormation.cpp` | 34,528 bytes | FULL | NO | Group.* |
| `Group/RoleAssignment.cpp` | 63,804 bytes | FULL | NO | Group.* |
| `Group/GroupInvitationHandler.cpp` | 33,613 bytes | FULL | NO | Group.* |
| `Group/GroupEventHandler.cpp` | 27,704 bytes | FULL | NO | Group.* |
| `Group/GroupStateMachine.cpp` | 17,728 bytes | FULL | NO | Group.* |
| `Group/LFGGroupCoordinator.cpp` | - | FULL | NO | LFG.* |
| `Advanced/GroupCoordinator.cpp` | 43,777 bytes | FULL | NO | Group.* |

**Config Wiring Required:**
```cpp
// Add to GroupCoordination::Initialize()
_config.updateInterval = sPlayerbotConfig->GetUInt("Playerbot.Group.UpdateInterval", 1000);
_config.inviteResponseDelay = sPlayerbotConfig->GetUInt("Playerbot.Group.InviteResponseDelay", 2000);
_config.readyCheckTimeout = sPlayerbotConfig->GetUInt("Playerbot.Group.ReadyCheckTimeout", 30000);
_config.lootRollTimeout = sPlayerbotConfig->GetUInt("Playerbot.Group.LootRollTimeout", 60000);
_config.targetUpdateInterval = sPlayerbotConfig->GetUInt("Playerbot.Group.TargetUpdateInterval", 500);
_config.assistLeader = sPlayerbotConfig->GetBool("Playerbot.Group.AssistLeader", true);
_config.followDistance = sPlayerbotConfig->GetFloat("Playerbot.Group.FollowDistance", 5.0f);
```

---

### 8. QUEST SYSTEM

| File | Size | Status | Config Wired | Config Options Needed |
|------|------|--------|--------------|----------------------|
| `Game/QuestManager.cpp` | 57,478 bytes | FULL | NO | Quest.* |
| `Game/QuestAcceptanceManager.cpp` | 17,587 bytes | FULL | NO | Quest.* |
| `Quest/DynamicQuestSystem.cpp` | - | FULL | NO | Quest.* |
| `Quest/UnifiedQuestManager.cpp` | - | FULL | NO | Quest.* |

**Config Wiring Required:**
```cpp
// Add to QuestManager::Initialize()
_config.enabled = sPlayerbotConfig->GetBool("Playerbot.Quest.Enable", true);
_config.autoAccept = sPlayerbotConfig->GetBool("Playerbot.Quest.AutoAccept", true);
_config.autoComplete = sPlayerbotConfig->GetBool("Playerbot.Quest.AutoComplete", true);
_config.maxActive = sPlayerbotConfig->GetUInt("Playerbot.Quest.MaxActive", 25);
_config.minLevel = sPlayerbotConfig->GetUInt("Playerbot.Quest.MinLevel", 1);
_config.maxLevelDiff = sPlayerbotConfig->GetUInt("Playerbot.Quest.MaxLevelDiff", 5);
_config.preferZoneQuests = sPlayerbotConfig->GetBool("Playerbot.Quest.PreferZoneQuests", true);
_config.groupQuestPriority = sPlayerbotConfig->GetFloat("Playerbot.Quest.GroupQuestPriority", 1.5f);
```

---

### 9. GAME INTERACTION SYSTEM

| File | Size | Status | Config Wired | Config Options Needed |
|------|------|--------|--------------|----------------------|
| `Game/InventoryManager.cpp` | 44,249 bytes | FULL | NO | Inventory.* |
| `Game/NPCInteractionManager.cpp` | 45,424 bytes | FULL | NO | NPC.* |
| `Game/VendorPurchaseManager.cpp` | 25,263 bytes | FULL | NO | Trade.* |
| `Game/FlightMasterManager.cpp` | 17,970 bytes | FULL | NO | Travel.* |

**Config Wiring Required:**
```cpp
// Add to InventoryManager::Initialize()
_config.autoSort = sPlayerbotConfig->GetBool("Playerbot.Inventory.AutoSort", true);
_config.autoStack = sPlayerbotConfig->GetBool("Playerbot.Inventory.AutoStack", true);
_config.keepFreeSlots = sPlayerbotConfig->GetUInt("Playerbot.Inventory.KeepFreeSlots", 5);
_config.autoDestroyGray = sPlayerbotConfig->GetBool("Playerbot.Inventory.AutoDestroyGray", false);
_config.saveSoulbound = sPlayerbotConfig->GetBool("Playerbot.Inventory.SaveSoulbound", true);
```

---

### 10. CHARACTER SYSTEM

| File | Size | Status | Config Wired | Config Options Needed |
|------|------|--------|--------------|----------------------|
| `Character/BotLevelManager.cpp` | 35,256 bytes | FULL | **PARTIAL** | LevelManager.* |
| `Character/BotLevelDistribution.cpp` | 24,182 bytes | FULL | **PARTIAL** | LevelManager.* |
| `Character/BotCharacterDistribution.cpp` | 12,764 bytes | FULL | NO | Character.* |
| `Character/BotCustomizationGenerator.cpp` | 8,517 bytes | FULL | NO | Character.* |
| `Character/BotNameMgr.cpp` | 9,996 bytes | FULL | NO | Names.* |

**Config Wiring Required for Names:**
```cpp
// Add to BotNameMgr::Initialize()
_config.useRandomNames = sPlayerbotConfig->GetBool("Playerbot.Names.UseRandomNames", true);
_config.allowDuplicates = sPlayerbotConfig->GetBool("Playerbot.Names.AllowDuplicates", false);
_config.minLength = sPlayerbotConfig->GetUInt("Playerbot.Names.MinLength", 4);
_config.maxLength = sPlayerbotConfig->GetUInt("Playerbot.Names.MaxLength", 12);
_config.useRaceTheme = sPlayerbotConfig->GetBool("Playerbot.Names.UseRaceTheme", true);
```

---

### 11. PERFORMANCE SYSTEM

| File | Size | Status | Config Wired | Config Options Needed |
|------|------|--------|--------------|----------------------|
| `Performance/PerformanceManager.cpp` | 168 bytes | **STUB** | NO | Performance.* |
| `Performance/BotLoadTester.cpp` | 38,916 bytes | FULL | NO | Performance.* |
| `Performance/BotProfiler.cpp` | 33,814 bytes | FULL | NO | Performance.* |
| `Performance/BotPerformanceMonitor.cpp` | 21,876 bytes | FULL | NO | Performance.* |
| `Performance/BotPerformanceAnalytics.cpp` | 30,862 bytes | FULL | NO | Performance.* |
| `Performance/BotMemoryManager.cpp` | 29,771 bytes | FULL | NO | Performance.* |
| `Performance/MLPerformanceTracker.cpp` | - | FULL | NO | Performance.* |

**Note:** PerformanceManager.cpp is a stub (168 bytes). Other performance files are full implementations.

**Config Wiring Required:**
```cpp
// Add to PerformanceManager::Initialize() (or BotPerformanceMonitor)
_config.enableMonitoring = sPlayerbotConfig->GetBool("Playerbot.Performance.EnableMonitoring", false);
_config.cpuWarningThreshold = sPlayerbotConfig->GetUInt("Playerbot.Performance.CpuWarningThreshold", 70);
_config.memoryWarningThreshold = sPlayerbotConfig->GetUInt("Playerbot.Performance.MemoryWarningThreshold", 80);
_config.reportInterval = sPlayerbotConfig->GetUInt("Playerbot.Performance.ReportInterval", 60);
```

---

### 12. THREAD POOL SYSTEM

| File | Size | Status | Config Wired | Config Options Needed |
|------|------|--------|--------------|----------------------|
| `Performance/ThreadPool/ThreadPool.cpp` | - | FULL | NO | ThreadPool.* |
| `Performance/ThreadPool/ThreadPool.h` | - | FULL | NO | ThreadPool.* |
| `Performance/ThreadPool/ThreadPoolCommands.cpp` | - | FULL | NO | - |
| `Performance/ThreadPool/ThreadPoolDeadlockFix.cpp` | - | FULL | NO | - |
| `Performance/ThreadPool/ThreadPoolDiagnostics.h` | - | FULL | NO | - |

**Config Wiring Required:**
```cpp
// Add to ThreadPool::Initialize()
_config.size = sPlayerbotConfig->GetUInt("Playerbot.ThreadPool.Size", 4);
_config.maxQueueSize = sPlayerbotConfig->GetUInt("Playerbot.ThreadPool.MaxQueueSize", 1000);
_config.taskTimeout = sPlayerbotConfig->GetUInt("Playerbot.ThreadPool.TaskTimeout", 5000);
```

---

### 13. AI DECISION SYSTEM

| File | Size | Status | Config Wired | Config Options Needed |
|------|------|--------|--------------|----------------------|
| `AI/Decision/DecisionFusionSystem.cpp` | - | FULL | **YES** | DecisionFusion.* |
| `AI/Common/ActionScoringEngine.cpp` | - | FULL | **PARTIAL** | Weighting.* |
| `Advanced/AdvancedBehaviorManager.cpp` | 52,472 bytes | FULL | NO | AI.* |
| `Advanced/TacticalCoordinator.cpp` | 24,422 bytes | FULL | NO | AI.* |

**DecisionFusionSystem** is already reading 7 config options via `sPlayerbotConfig`.
**ActionScoringEngine** has hardcoded multipliers that should be configurable.

---

### 14. MOVEMENT SYSTEM

| File | Size | Status | Config Wired | Config Options Needed |
|------|------|--------|--------------|----------------------|
| `Movement/BotWorldPositioner.cpp` | - | FULL | **PARTIAL** | WorldPositioner.* |
| `Movement/BotWorldPositioner.h` | - | FULL | - | - |

**Note:** Uses `sConfigMgr` instead of `sPlayerbotConfig` (lines 167, 217). Needs conversion.

---

### 15. DEATH RECOVERY SYSTEM

| File | Size | Status | Config Wired | Config Options Needed |
|------|------|--------|--------------|----------------------|
| `Lifecycle/DeathRecoveryManager.cpp` | - | FULL | **YES** | Death.* |
| `Lifecycle/DeathRecoveryManager.h` | - | FULL | - | - |

**Status:** Already reading 11 config options via `sPlayerbotConfig`. Some option names need alignment with config file.

---

## Duplicate Systems Analysis

### Systems with Multiple Implementations

| Feature | Implementation 1 | Implementation 2 | Recommendation |
|---------|-----------------|------------------|----------------|
| **Auction** | Economy/AuctionManager.cpp (38KB) | Social/AuctionHouse.cpp (59KB) | Consolidate to AuctionManager |
| **Trade** | Social/TradeSystem.cpp (61KB) | Social/TradeManager.cpp (47KB) | Choose one, deprecate other |
| **Loot** | Social/LootDistribution.cpp (109KB) | Social/UnifiedLootManager.cpp (78KB) | Use UnifiedLootManager |
| **Performance** | Performance/BotPerformanceMonitor.cpp | Lifecycle/BotPerformanceMonitor.cpp | Consolidate |
| **Group Coord** | Group/GroupCoordination.cpp | Advanced/GroupCoordinator.cpp | Document distinction |

---

## Implementation Priority

### Tier 1: Quick Wins (1-2 hours each)
Systems that are fully implemented and just need config wiring:

1. **Death Recovery** - 11 options, already partially wired
2. **Decision Fusion** - 7 options, already wired (verify completeness)
3. **Session Management** - 4 options
4. **BotWorldPositioner** - sConfigMgr → sPlayerbotConfig conversion

### Tier 2: Core Feature Wiring (2-4 hours each)

5. **Quest System** - 8 options
6. **Trade System** - 7 options
7. **Group Coordination** - 7 options
8. **Inventory Management** - 5 options

### Tier 3: Major System Wiring (4-8 hours each)

9. **Talent Manager** - 6 options
10. **Gear Factory** - 8 options
11. **Banking System** - 5 options
12. **Social System** - 6 options
13. **Performance Monitoring** - 4 options

### Tier 4: Consolidation Required

14. **Auction Systems** - Consolidate then wire
15. **Loot Systems** - Consolidate then wire
16. **Trade Systems** - Choose primary then wire

---

## Config File vs Code Gap Analysis

| Category | Config Options Defined | Actually Used | Gap |
|----------|----------------------|---------------|-----|
| Core Settings | 11 | 11 | 0 |
| Database | 5 | 5 | 0 |
| Spawning | 8 | 8 | 0 |
| Throttler | 7 | 7 | 0 |
| Circuit Breaker | 6 | 6 | 0 |
| Resource Monitor | 13 | 13 | 0 |
| Startup Orchestrator | 8 | 8 | 0 |
| Account Management | 4 | 4 | 0 |
| **Quest System** | 8 | 0 | **8** |
| **Trade System** | 7 | 0 | **7** |
| Logging | 5 | 5 | 0 |
| Update Intervals | 2 | 2 | 0 |
| **AI Behavior** | 9 | 0 | **9** |
| **Decision Fusion** | 7 | 7 | 0 |
| **AI Weighting** | 72+ | 0 | **72+** |
| **Performance** | 4 | 0 | **4** |
| **Thread Pool** | 3 | 0 | **3** |
| **Names** | 5 | 0 | **5** |
| **Social** | 6 | 0 | **6** |
| **Security** | 4 | 0 | **4** |
| **Experimental** | 4 | 0 | **4** |
| **Gear Factory** | 8 | 0 | **8** |
| **Talent Manager** | 6 | 0 | **6** |
| **World Positioner** | 6 | 2 | **4** |
| **Level Manager** | 6 | 2 | **4** |
| **Death System** | 8 | 11 | 0 (different keys) |
| **Inventory** | 5 | 0 | **5** |
| **Auction House** | 6 | ~6 | ~0 |
| **Group** | 7 | 0 | **7** |
| **Banking** | 5 | 0 | **5** |
| **Session** | 4 | 0 | **4** |

**Total Gap:** ~163 config options defined but not wired to implementations

---

## Action Items

### Immediate (This Sprint)
- [ ] Wire Quest System config (8 options)
- [ ] Wire Trade System config (7 options)
- [ ] Wire Group System config (7 options)
- [ ] Convert BotWorldPositioner from sConfigMgr to sPlayerbotConfig

### Short Term (Next Sprint)
- [ ] Wire Talent Manager config (6 options)
- [ ] Wire Gear Factory config (8 options)
- [ ] Wire Banking System config (5 options)
- [ ] Wire Social System config (6 options)

### Medium Term
- [ ] Wire AI Weighting config (72+ options)
- [ ] Wire Inventory config (5 options)
- [ ] Wire Names config (5 options)
- [ ] Wire Performance config (4 options)
- [ ] Wire ThreadPool config (3 options)

### Long Term
- [ ] Consolidate duplicate Auction systems
- [ ] Consolidate duplicate Trade systems
- [ ] Consolidate duplicate Loot systems
- [ ] Implement PerformanceManager.cpp (currently stub)

---

**Document Version:** 1.0
**Last Updated:** 2026-01-06
