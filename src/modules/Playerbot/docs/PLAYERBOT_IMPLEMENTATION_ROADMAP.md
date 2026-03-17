# Playerbot Implementation Roadmap

**Version:** 2.0.0
**Date:** 2026-01-06
**Branch:** playerbot-dev

This document provides the implementation roadmap for wiring fully implemented systems to their configuration options.

---

## Executive Summary

### CRITICAL AUDIT CORRECTION (2026-01-06)

Previous documentation incorrectly classified many systems as "stubs" requiring full implementation. A comprehensive codebase audit revealed:

| Metric | Value | Notes |
|--------|-------|-------|
| **Total Major Systems** | 46 | Complete Playerbot module |
| **Fully Implemented** | 45 (98%) | Working code exists |
| **True Stubs** | 1 (2%) | Only PerformanceManager.cpp |
| **Properly Wired** | 20 (43%) | Config connected to code |
| **Remaining** | 25 (54%) | Still need config wiring |

### Config Wiring Progress (2026-01-06 - UPDATED)

| Status | Count | Systems |
|--------|-------|---------|
| ✅ Wired | 24 | DecisionFusion, ActionScoring (incl LogScoring/LogTopActions), Auction, Death, Throttler, Spawner, Quest, QuestManager, Trade(2), Talent, Gear, Banking, Inventory, WorldPositioner, Names, LevelManager, LevelDistribution (tier-based), ZoneLevelHelper, SocialManager, PerformanceMonitor, ThreadPool, BotWorldSessionMgr, Dragonriding |
| ⚠️ Blocked | 1 | GroupCoordination (constexpr refactoring needed) |
| ⏳ Stub/Placeholder | 2 | BotSession (needs code implementation), AI Behavior Delays (no code) |
| ✅ Per-Bot (N/A) | 5 | GuildEventCoordinator, EquipmentManager, DynamicQuestSystem, UnifiedQuestManager, AuctionHouse |
| ⏳ No Config Defined | 4 | GuildIntegration, RoleAssignment, GroupFormation, GuildBankManager |

### Duplicate System Audit (2026-01-06 - COMPLETE)

| System Pair | Finding |
|-------------|---------|
| TradeSystem vs TradeManager | Complementary (vendor focus vs player focus) - Keep both |
| AuctionManager vs AuctionHouse | Singleton + per-bot pattern - Keep both |
| LootDistribution vs UnifiedLootManager | UnifiedLootManager wraps LootDistribution - By design |

### sConfigMgr Elimination (COMPLETE)

All `sConfigMgr` usage has been converted to `sPlayerbotConfig`. Zero remaining instances in source files.

### What This Means

**Before Audit:** "Need 50-70 days to implement stub features"
**After Audit:** "Need 20-30 hours to wire existing code to config"

The Playerbot module is **98% implemented**. The remaining work is connecting configuration options to already-working code.

---

## System Inventory Overview

### Systems with FULL Implementation (45 systems)

| Category | System | File Size | Config Status |
|----------|--------|-----------|---------------|
| **AI** | DecisionFusionSystem | 42KB | ✅ WIRED |
| **AI** | ActionScoringEngine | 38KB | ✅ WIRED (6 weight options at line 237-242) |
| **Talents** | BotTalentManager | 49KB | ✅ WIRED (2026-01-06) |
| **Equipment** | BotGearFactory | 29KB | ✅ WIRED (2026-01-06) |
| **Equipment** | EquipmentManager | 61KB | ⚠️ Per-bot, no config defined |
| **Economy** | AuctionManager | 38KB | ✅ WIRED (10 calls) |
| **Economy** | GoldManager | N/A | ⚠️ FILE DOES NOT EXIST |
| **Social** | TradeSystem | 61KB | ✅ WIRED via PlayerbotTradeConfig |
| **Social** | TradeManager | 47KB | ✅ WIRED via PlayerbotTradeConfig |
| **Social** | SocialManager | 36KB | ✅ WIRED (2026-01-06) |
| **Social** | GuildEventCoordinator | 92KB | ⚠️ No config defined |
| **Social** | GuildIntegration | 60KB | ⚠️ No config defined |
| **Banking** | BankingManager | 45KB | ✅ WIRED (2026-01-06) |
| **Banking** | GuildBankManager | 33KB | ⚠️ No config defined |
| **Quest** | QuestManager | 57KB | ✅ WIRED (6 options at lines 240-255) |
| **Quest** | DynamicQuestSystem | 44KB | ⚠️ Per-bot, no config defined |
| **Quest** | UnifiedQuestManager | 52KB | ⚠️ Per-bot, no config defined |
| **Group** | GroupCoordination | 41KB | ⚠️ BLOCKED (constexpr) |
| **Group** | RoleAssignment | 63KB | ⚠️ No config defined |
| **Group** | GroupFormation | 34KB | ⚠️ No config defined |
| **Inventory** | InventoryManager | 44KB | ✅ WIRED (in constructor) |
| **Lifecycle** | DeathRecoveryManager | 52KB | ✅ WIRED (11 calls) |
| **Lifecycle** | BotSpawner | 78KB | ✅ PARTIAL |
| **Movement** | BotWorldPositioner | 38KB | ✅ WIRED (2026-01-06) |
| **Character** | BotNameMgr | 9KB | ✅ WIRED (2026-01-06) |
| **Character** | BotLevelManager | 35KB | ✅ WIRED (2026-01-06) |
| **Character** | BotLevelDistribution | 18KB | ✅ WIRED (2026-01-06) - Tier-based via ContentTuning DB2 |
| **Character** | ZoneLevelHelper | 12KB | ✅ NEW (2026-01-06) - ContentTuning DB2 integration |
| **Session** | BotSession | 28KB | ❌ NOT WIRED |
| **Performance** | AdaptiveSpawnThrottler | 18KB | ✅ WIRED |
| **Performance** | BotPerformanceMonitor | 21KB | ✅ WIRED (2026-01-06) |

### True Stub (1 system)

| System | File | Size | Status |
|--------|------|------|--------|
| PerformanceManager | Performance/PerformanceManager.cpp | 168 bytes | STUB - Needs implementation |

---

## Phase 1: Config Wiring (Priority Order)

### Estimated Total: 20-30 hours

All systems below have **working code**. The only change needed is adding config reading.

### Tier 1: Quick Wins (1-2 hours each)

| System | Config Options | Files | Effort |
|--------|---------------|-------|--------|
| BotNameMgr | 5 | Character/BotNameMgr.cpp | 1 hour |
| BotSession | 4 | Session/BotSession.cpp | 1 hour |
| BotWorldPositioner | 6 | Movement/BotWorldPositioner.cpp | 1 hour |
| BotLevelManager | 6 | Character/BotLevelManager.cpp | 1 hour |

**Pattern for Quick Wins:**
```cpp
// Add to Initialize() method:
bool BotNameMgr::Initialize()
{
    _config.useRandomNames = sPlayerbotConfig->GetBool("Playerbot.Names.UseRandomNames", true);
    _config.allowDuplicates = sPlayerbotConfig->GetBool("Playerbot.Names.AllowDuplicates", false);
    _config.minLength = sPlayerbotConfig->GetInt("Playerbot.Names.MinLength", 4);
    _config.maxLength = sPlayerbotConfig->GetInt("Playerbot.Names.MaxLength", 12);
    _config.useRaceTheme = sPlayerbotConfig->GetBool("Playerbot.Names.UseRaceTheme", true);
    return true;
}
```

### Tier 2: Core Systems (2-3 hours each)

| System | Config Options | Files | Effort |
|--------|---------------|-------|--------|
| QuestManager | 8 | Game/QuestManager.cpp | 2-3 hours |
| TradeSystem | 7 | Social/TradeSystem.cpp | 2-3 hours |
| GroupCoordination | 7 | Group/GroupCoordination.cpp | 2-3 hours |
| InventoryManager | 5 | Game/InventoryManager.cpp | 2 hours |
| BankingManager | 5 | Banking/BankingManager.cpp | 2 hours |

**Pattern for Core Systems:**
```cpp
// QuestManager::Initialize()
bool QuestManager::Initialize()
{
    // Load config
    _config.enabled = sPlayerbotConfig->GetBool("Playerbot.Quest.Enable", true);
    _config.autoAccept = sPlayerbotConfig->GetBool("Playerbot.Quest.AutoAccept", true);
    _config.autoComplete = sPlayerbotConfig->GetBool("Playerbot.Quest.AutoComplete", true);
    _config.maxActive = sPlayerbotConfig->GetInt("Playerbot.Quest.MaxActive", 25);
    _config.minLevel = sPlayerbotConfig->GetInt("Playerbot.Quest.MinLevel", 1);
    _config.maxLevelDiff = sPlayerbotConfig->GetInt("Playerbot.Quest.MaxLevelDiff", 5);
    _config.preferZoneQuests = sPlayerbotConfig->GetBool("Playerbot.Quest.PreferZoneQuests", true);
    _config.groupQuestPriority = sPlayerbotConfig->GetFloat("Playerbot.Quest.GroupQuestPriority", 1.5f);

    // Existing initialization continues...
    return InitializeInternal();
}
```

### Tier 3: Complex Systems (3-4 hours each)

| System | Config Options | Files | Effort |
|--------|---------------|-------|--------|
| BotTalentManager | 6 | Talents/BotTalentManager.cpp | 3 hours |
| BotGearFactory | 8 | Equipment/BotGearFactory.cpp | 3 hours |
| GuildEventCoordinator | 6 | Social/GuildEventCoordinator.cpp | 3 hours |
| SocialManager | 6 | Advanced/SocialManager.cpp | 3 hours |

---

## Phase 2: sConfigMgr → sPlayerbotConfig Conversion

### Files Using Wrong Config Manager

| File | Lines | Issue |
|------|-------|-------|
| Movement/BotWorldPositioner.cpp | 167, 217 | Uses sConfigMgr->GetStringDefault() |
| AI/Common/ActionScoringEngine.cpp | Multiple | Uses sConfigMgr (6 calls) |
| AI/ClassAI/Mages/ArcaneMageWeighted.h | 2 locations | Uses sConfigMgr |

**Required Change:**
```cpp
// FROM:
std::string overrideStr = sConfigMgr->GetStringDefault(configKey.c_str(), "");

// TO:
std::string overrideStr = sPlayerbotConfig->GetString(configKey, "");
```

**Estimated Effort:** 2-3 hours total

---

## Phase 3: Duplicate System Resolution

### Identified Duplicates

| Area | System A | System B | Recommendation |
|------|----------|----------|----------------|
| Trade | TradeSystem.cpp (61KB) | TradeManager.cpp (47KB) | Audit usage, consolidate |
| Auction | Economy/AuctionManager.cpp | Social/AuctionHouse.cpp | Keep Economy version |
| Loot | LootDistribution.cpp | UnifiedLootManager.cpp | Keep Unified version |
| Quest | DynamicQuestSystem.cpp | UnifiedQuestManager.cpp | UnifiedQuestManager wraps Dynamic |

### Resolution Strategy

1. **Audit all call sites** for each duplicate pair
2. **Identify primary** system (most complete, most used)
3. **Deprecate secondary** with redirects
4. **Eventually remove** secondary system

**Estimated Effort:** 8-16 hours (one-time cleanup)

---

## Implementation Progress Tracker

### Tier 1: Quick Wins

| Task | Status | Date | Notes |
|------|--------|------|-------|
| Wire BotNameMgr | ✅ DONE | 2026-01-06 | 5 config options wired |
| Wire BotSession | ⚠️ PLACEHOLDER | 2026-01-06 | Config exists but code needs implementation (timeout/cleanup features) |
| Convert BotWorldPositioner | ✅ DONE | 2026-01-06 | sConfigMgr → sPlayerbotConfig |
| Complete BotLevelManager | ✅ DONE | 2026-01-06 | Already wired: MaxBotsPerUpdate, VerboseLogging, Population.* |
| BotLevelDistribution → ContentTuning | ✅ DONE | 2026-01-06 | Replaced 102 config ranges with 4 expansion tiers via ContentTuning DB2 |
| New: ZoneLevelHelper | ✅ DONE | 2026-01-06 | Zone level requirements from ContentTuning DB2 (WoW 11.x dynamic scaling) |

### Tier 2: Core Systems

| Task | Status | Date | Notes |
|------|--------|------|-------|
| Wire QuestManager | ✅ DONE | 2026-01-06 | 8 config options wired |
| Wire TradeSystem | ✅ DONE | 2026-01-06 | Already wired via PlayerbotTradeConfig |
| Wire GroupCoordination | ⚠️ BLOCKED | - | Needs constexpr → runtime refactoring |
| Wire InventoryManager | ✅ DONE | 2026-01-06 | Already wired, fixed config keys |
| Wire BankingManager | ✅ DONE | 2026-01-06 | 5 config options wired |

### Tier 3: Complex Systems

| Task | Status | Date | Notes |
|------|--------|------|-------|
| Wire BotTalentManager | ✅ DONE | 2026-01-06 | 6 config options wired |
| Wire BotGearFactory | ✅ DONE | 2026-01-06 | 8 config options wired |
| Wire GuildEventCoordinator | ✅ N/A | 2026-01-06 | Per-bot instance, no global config needed |
| Wire SocialManager | ✅ DONE | 2026-01-06 | 5 config options wired (EnableChat, ChatFrequency, etc.) |

### Phase 2: Conversions

| Task | Status | Date | Notes |
|------|--------|------|-------|
| Convert ActionScoringEngine | ✅ DONE | 2026-01-06 | Already uses sPlayerbotConfig (6 weight options) |
| Convert ArcaneMageWeighted | ✅ DONE | 2026-01-06 | Already uses sPlayerbotConfig (LogScoring, LogTopActions) |
| Add missing config options | ✅ DONE | 2026-01-06 | Added LogScoring, LogTopActions to config file |

### Phase 3: Duplicate Resolution

| Task | Status | Date | Notes |
|------|--------|------|-------|
| Audit Trade systems | ✅ DONE | 2026-01-06 | Complementary: TradeSystem (vendor) + TradeManager (player) |
| Audit Auction systems | ✅ DONE | 2026-01-06 | Complementary: AuctionManager (singleton) + AuctionHouse (per-bot) |
| Audit Loot systems | ✅ DONE | 2026-01-06 | UnifiedLootManager wraps LootDistribution by design |

---

## Config Wiring Code Snippets

### QuestManager

**File:** `src/modules/Playerbot/Game/QuestManager.cpp`

```cpp
#include "Config/PlayerbotConfig.h"

struct QuestConfig
{
    bool enabled = true;
    bool autoAccept = true;
    bool autoComplete = true;
    uint32 maxActive = 25;
    uint32 minLevel = 1;
    uint32 maxLevelDiff = 5;
    bool preferZoneQuests = true;
    float groupQuestPriority = 1.5f;
};

bool QuestManager::LoadConfig()
{
    _questConfig.enabled = sPlayerbotConfig->GetBool("Playerbot.Quest.Enable", true);
    _questConfig.autoAccept = sPlayerbotConfig->GetBool("Playerbot.Quest.AutoAccept", true);
    _questConfig.autoComplete = sPlayerbotConfig->GetBool("Playerbot.Quest.AutoComplete", true);
    _questConfig.maxActive = sPlayerbotConfig->GetInt("Playerbot.Quest.MaxActive", 25);
    _questConfig.minLevel = sPlayerbotConfig->GetInt("Playerbot.Quest.MinLevel", 1);
    _questConfig.maxLevelDiff = sPlayerbotConfig->GetInt("Playerbot.Quest.MaxLevelDiff", 5);
    _questConfig.preferZoneQuests = sPlayerbotConfig->GetBool("Playerbot.Quest.PreferZoneQuests", true);
    _questConfig.groupQuestPriority = sPlayerbotConfig->GetFloat("Playerbot.Quest.GroupQuestPriority", 1.5f);
    return true;
}
```

### TradeSystem

**File:** `src/modules/Playerbot/Social/TradeSystem.cpp`

```cpp
struct TradeConfig
{
    bool enabled = true;
    bool autoVendor = true;
    bool autoRepair = true;
    bool sellGrayItems = true;
    bool sellWhiteItems = false;
    uint32 minGoldToTrade = 100;
    float maxTradeDistance = 10.0f;
};

bool TradeSystem::LoadConfig()
{
    _tradeConfig.enabled = sPlayerbotConfig->GetBool("Playerbot.Trade.Enable", true);
    _tradeConfig.autoVendor = sPlayerbotConfig->GetBool("Playerbot.Trade.AutoVendor", true);
    _tradeConfig.autoRepair = sPlayerbotConfig->GetBool("Playerbot.Trade.AutoRepair", true);
    _tradeConfig.sellGrayItems = sPlayerbotConfig->GetBool("Playerbot.Trade.SellGrayItems", true);
    _tradeConfig.sellWhiteItems = sPlayerbotConfig->GetBool("Playerbot.Trade.SellWhiteItems", false);
    _tradeConfig.minGoldToTrade = sPlayerbotConfig->GetInt("Playerbot.Trade.MinGoldToTrade", 100);
    _tradeConfig.maxTradeDistance = sPlayerbotConfig->GetFloat("Playerbot.Trade.MaxTradeDistance", 10.0f);
    return true;
}
```

### BotTalentManager

**File:** `src/modules/Playerbot/Talents/BotTalentManager.cpp`

```cpp
struct TalentConfig
{
    bool enabled = true;
    bool useOptimalBuilds = true;
    bool randomizeMinor = true;
    bool adaptToContent = true;
    uint32 respecFrequency = 24; // hours
    bool useHeroTalents = true;
};

bool BotTalentManager::LoadConfig()
{
    _talentConfig.enabled = sPlayerbotConfig->GetBool("Playerbot.TalentManager.Enable", true);
    _talentConfig.useOptimalBuilds = sPlayerbotConfig->GetBool("Playerbot.TalentManager.UseOptimalBuilds", true);
    _talentConfig.randomizeMinor = sPlayerbotConfig->GetBool("Playerbot.TalentManager.RandomizeMinor", true);
    _talentConfig.adaptToContent = sPlayerbotConfig->GetBool("Playerbot.TalentManager.AdaptToContent", true);
    _talentConfig.respecFrequency = sPlayerbotConfig->GetInt("Playerbot.TalentManager.RespecFrequency", 24);
    _talentConfig.useHeroTalents = sPlayerbotConfig->GetBool("Playerbot.TalentManager.UseHeroTalents", true);
    return true;
}
```

### BotGearFactory

**File:** `src/modules/Playerbot/Equipment/BotGearFactory.cpp`

```cpp
struct GearConfig
{
    bool enabled = true;
    uint8 qualityMin = 2;  // Green
    uint8 qualityMax = 4;  // Epic
    uint8 levelRange = 5;
    bool useSpecAppropriate = true;
    bool enchantItems = true;
    bool gemItems = true;
    uint32 refreshInterval = 60; // minutes
};

bool BotGearFactory::LoadConfig()
{
    _gearConfig.enabled = sPlayerbotConfig->GetBool("Playerbot.GearFactory.Enable", true);
    _gearConfig.qualityMin = sPlayerbotConfig->GetInt("Playerbot.GearFactory.QualityMin", 2);
    _gearConfig.qualityMax = sPlayerbotConfig->GetInt("Playerbot.GearFactory.QualityMax", 4);
    _gearConfig.levelRange = sPlayerbotConfig->GetInt("Playerbot.GearFactory.LevelRange", 5);
    _gearConfig.useSpecAppropriate = sPlayerbotConfig->GetBool("Playerbot.GearFactory.UseSpecAppropriate", true);
    _gearConfig.enchantItems = sPlayerbotConfig->GetBool("Playerbot.GearFactory.EnchantItems", true);
    _gearConfig.gemItems = sPlayerbotConfig->GetBool("Playerbot.GearFactory.GemItems", true);
    _gearConfig.refreshInterval = sPlayerbotConfig->GetInt("Playerbot.GearFactory.RefreshInterval", 60);
    return true;
}
```

---

## Validation Checklist

### Per-System Wiring Validation

For each system wired, verify:

- [ ] Config struct defined with all options
- [ ] LoadConfig() method added
- [ ] LoadConfig() called from Initialize()
- [ ] Config values actually used in relevant methods
- [ ] Default values match playerbots.conf.dist
- [ ] Config hot-reload works (if applicable)
- [ ] Unit test added for config loading

### Integration Validation

- [ ] Server starts without errors
- [ ] Changing config values affects behavior
- [ ] Profile system overrides work
- [ ] No regressions in existing functionality

---

## Timeline Summary

| Phase | Tasks | Estimated Effort |
|-------|-------|-----------------|
| Tier 1 Quick Wins | 4 systems | 4-6 hours |
| Tier 2 Core Systems | 5 systems | 10-15 hours |
| Tier 3 Complex Systems | 4 systems | 12-16 hours |
| sConfigMgr Conversion | 3 files | 2-3 hours |
| **Total Config Wiring** | **16 systems** | **28-40 hours** |
| Duplicate Resolution | 4 pairs | 8-16 hours (optional) |

**Key Insight:** The module is **98% implemented**. Only config wiring remains.

---

## Appendix: Full System Inventory Reference

See `docs/PLAYERBOT_SYSTEMS_INVENTORY.md` for complete system inventory including:
- All 46 major system classes
- File sizes and locations
- Config wiring status
- Duplicate system analysis
- Implementation quality notes

---

**Document End**

*Last Updated: 2026-01-06*
*Audit Corrected: Systems properly classified as IMPLEMENTED vs STUB*
