# PLAYERBOT SYSTEMS INVENTORY - COMPLETE REFERENCE

**⚠️ CRITICAL: ALWAYS CHECK THIS FILE BEFORE IMPLEMENTING NEW FEATURES**

This document catalogs ALL existing systems in the Playerbot module to **prevent duplicate implementations, redundant work, and compatibility issues**. Before implementing ANY new feature, verify it doesn't already exist here.

**Last Updated:** 2026-01-28 (GOD_TIER Phase 3 completion)
**Total Systems:** 60+ major subsystems across 58 directories
**Total Files:** 1010+ C++ files
**Compilation Status:** ✅ 0 errors, 0 warnings
**Test Status:** ✅ 2/2 tests passing (100%)

---

## How to Use This Inventory

**BEFORE implementing ANY feature:**
1. ✅ Search this inventory for related systems
2. ✅ Check the "Key Files" list for existing implementations
3. ✅ Review "Helper Methods" to avoid recreating utilities
4. ✅ Verify API compatibility with "TrinityCore API Wrappers"
5. ✅ If in doubt, grep the codebase first: `grep -r "YourFeature" src/modules/Playerbot`

**NEVER:**
- ❌ Create stub files when real implementations exist (e.g., QuestHubDatabase stub was wrong)
- ❌ Reimplement wrapper methods (e.g., IsFlightMaster() already exists in NPCInteractionManager)
- ❌ Duplicate database schemas (check sql/ directory first)
- ❌ Bypass existing managers (e.g., use BotSpawnOrchestrator, not custom spawn logic)

---

## Directory Structure Overview

```
src/modules/Playerbot/
├── AI/                    # Bot artificial intelligence (Combat, ClassAI, Strategies)
├── Account/               # Bot account creation and management
├── Adapters/              # Interface adapters for legacy compatibility
├── Advanced/              # Advanced bot coordination (GroupCoordinator, etc.)
├── Auction/               # Auction house event bus
├── Aura/                  # Aura/buff event bus
├── Character/             # Character generation (names, race/class, customization)
├── Chat/                  # Chat command handlers
├── Combat/                # Combat event bus and coordination
├── Commands/              # GM commands for bot management
├── Companion/             # Pet/minion management
├── Config/                # Configuration management
├── Cooldown/              # Cooldown event bus
├── Core/                  # Core infrastructure (hooks, events, managers)
├── Data/                  # Static data (WoW 11.2 creation data, etc.)
├── Database/              # Database layer (queries, migrations, persistence)
├── Diagnostics/           # Performance monitoring and diagnostics
├── Dungeon/               # Dungeon/raid coordination
├── Economy/               # Auction house and economy systems
├── Equipment/             # Gear management and optimization
├── Events/                # Event data structures
├── Game/                  # Game interactions (NPCs, vendors, flight masters, quests)
├── Group/                 # Group formation, invitations, roles
├── Humanization/          # Human-like behavior simulation (AFK, mounts, pets, achievements)
├── Instance/              # Instance event bus
├── Interaction/           # NPC interaction systems
├── Interfaces/            # Abstract interfaces (IBotSpawner, IBotSession, etc.)
├── LFG/                   # Looking For Group systems
├── Lifecycle/             # Bot spawning, despawning, death recovery
├── Loot/                  # Loot distribution and event bus
├── Monitoring/            # Real-time monitoring and alerting
├── Movement/              # Movement, pathfinding, formations, quest navigation
├── Network/               # Packet parsing and network handling
├── NPC/                   # NPC event bus
├── Packets/               # Packet builders (SpellPacketBuilder, etc.)
├── Performance/           # Performance optimization (object pools, packet pools)
├── Professions/           # Profession management, gathering, farming
├── PvP/                   # PvP systems
├── Quest/                 # Quest management (pickup, tracking, navigation, hubs)
├── Resource/              # Resource event bus (mana, health, etc.)
├── Scripting/             # Script hooks (resurrection, etc.)
├── Session/               # Session management (BotSession, packet relay, priority)
├── Social/                # Social systems (trade, guilds, loot distribution)
├── Spatial/               # Spatial partitioning and grid systems
├── Talents/               # Talent management
├── Tests/                 # Unit tests (GTest framework)
└── Threading/             # Threading utilities
```

---

## Core Systems Reference

### Quick Lookup: "I need [X]" → "Check here first"

| Feature Needed | Existing System | Location | Status |
|----------------|-----------------|----------|--------|
| Bot spawning | BotSpawnOrchestrator | Lifecycle/ | ✅ COMPLETE |
| Quest navigation | QuestPathfinder + QuestHubDatabase | Movement/, Quest/ | ✅ COMPLETE |
| Quest hub database | QuestHubDatabase | Quest/ | ✅ **ALREADY EXISTS** |
| NPC interaction | NPCInteractionManager | Game/ | ✅ COMPLETE |
| Flight paths | FlightMasterManager | Game/ | ✅ COMPLETE |
| Spell casting | SpellPacketBuilder | Packets/ | ✅ COMPLETE |
| Combat AI | ClassAI + InterruptCoordinator | AI/ | ✅ COMPLETE |
| Group coordination | GroupStateMachine | Group/ | ✅ COMPLETE |
| Death recovery | DeathRecoveryManager | Lifecycle/ | ✅ COMPLETE |
| Database queries | PlayerbotDatabaseStatements | Database/ | ✅ COMPLETE |
| Event handling | [Type]EventBus | Combat/, Quest/, etc. | ✅ COMPLETE |
| Spatial queries | SpatialGrid systems | Spatial/ | ✅ COMPLETE |
| Mount collection | MountCollectionManager | Humanization/Mounts/ | ✅ COMPLETE |
| Pet collection | PetCollectionManager | Humanization/Collections/ | ✅ COMPLETE |
| Achievement grinding | AchievementGrinder | Humanization/Achievements/ | ✅ COMPLETE |
| AFK behavior | AFKSimulator | Humanization/AFKSimulator/ | ✅ COMPLETE |
| Fishing sessions | FishingSessionManager | Humanization/Fishing/ | ✅ COMPLETE |
| Humanization orchestration | HumanizationManager | Humanization/Core/ | ✅ COMPLETE |

---

## Critical Systems (Always Check These First)

### 1. Quest Systems ⚠️ IMPORTANT

**QuestHubDatabase** - `Quest/QuestHubDatabase.cpp/.h`
**Status:** ✅ **COMPLETE - PRODUCTION READY**

**What It Provides:**
- Spatial clustering of quest NPCs into logical hubs
- Efficient quest giver lookup by location
- Distance-based hub finding for navigation
- Level-appropriate quest suggestions
- Automatic hub database building at server startup

**How to Use:**
```cpp
#include "Quest/QuestHubDatabase.h"  // ✅ CORRECT path

auto& hubDb = QuestHubDatabase::Instance();
if (!hubDb.IsInitialized()) {
    LOG_ERROR("Quest hub database not initialized!");
    return;
}

// Find nearest hub to player
auto nearestHub = hubDb.FindNearestHub(player->GetPosition(), player->GetLevel());
```

**CRITICAL:**
- ❌ **DO NOT** create `Movement/QuestHubDatabase.h` - this was a mistake in Phase 4C
- ✅ **ALWAYS** use `#include "Quest/QuestHubDatabase.h"`
- ✅ QuestPathfinder uses this automatically - no manual integration needed

---

### 2. NPC Interaction - Helper Methods

**NPCInteractionManager** - `Game/NPCInteractionManager.cpp/.h`
**Status:** ✅ COMPLETE

**Helper Methods Available:**
```cpp
bool IsFlightMaster(Creature* npc) const;  // ✅ Wraps IsTaxi()
bool IsVendor(Creature* npc) const;
bool IsQuestGiver(Creature* npc) const;
bool IsAuctioneer(Creature* npc) const;
bool IsTrainer(Creature* npc) const;
bool IsInnkeeper(Creature* npc) const;
bool IsBanker(Creature* npc) const;
bool IsGuildMaster(Creature* npc) const;
```

**When to Use:**
- ✅ Use `NPCInteractionManager::IsFlightMaster()` in bot logic
- ✅ Use `creature->IsTaxi()` directly when NPCInteractionManager not available
- ❌ **DO NOT** call `creature->IsFlightMaster()` - this method doesn't exist in TrinityCore 11.2

---

### 3. TrinityCore 11.2 API Changes ⚠️ CRITICAL REFERENCE

**ALWAYS use these modern APIs:**

| OLD (Pre-11.2) | NEW (TrinityCore 11.2) | File |
|----------------|------------------------|------|
| `SpellInfo::Effects[i]` | `GetEffect(SpellEffIndex(i))` | Any spell code |
| `GetSpellInfo(id)` | `GetSpellInfo(id, DIFFICULTY_NONE)` | Any spell lookup |
| `Group::GetFirstMember()` loop | range-based `for (auto& itr : group->GetMembers())` | Group iteration |
| `Trinity::ChatCommands::Optional<T>` | `std::optional<T>` | Command handlers |
| `RBAC_PERM_COMMAND_GM_NOTIFY` | `RBAC_PERM_COMMAND_GMNOTIFY` | RBAC checks |
| `Player::GetEquipSlotForItem()` | `Player::FindEquipSlot(Item*)` | Equipment |
| `Creature::IsFlightMaster()` | `Creature::IsTaxi()` | NPC checks |
| `Item::GetUInt32Value(ITEM_FIELD_DURABILITY)` | `item->m_itemData->Durability` | Item fields |
| `ITEM_SUBCLASS_FOOD` | `ITEM_SUBCLASS_FOOD_DRINK` | Item constants |
| `ITEM_SUBCLASS_JUNK_PET` | `ITEM_SUBCLASS_MISCELLANEOUS_COMPANION_PET` | Item constants |

---

## Subsystem Details

### AI Systems (`AI/`)

**ClassAI/** - All 13 WoW classes implemented
- Death Knight (Blood, Frost, Unholy)
- Demon Hunter (Havoc, Vengeance)
- Druid (Balance, Feral, Guardian, Restoration)
- Evoker (Devastation, Preservation, Augmentation)
- Hunter (Beast Mastery, Marksmanship, Survival)
- Mage (Arcane, Fire, Frost)
- Monk (Brewmaster, Mistweaver, Windwalker)
- Paladin (Holy, Protection, Retribution)
- Priest (Discipline, Holy, Shadow)
- Rogue (Assassination, Outlaw, Subtlety)
- Shaman (Elemental, Enhancement, Restoration)
- Warlock (Affliction, Demonology, Destruction)
- Warrior (Arms, Fury, Protection)

**Combat/** - Combat Coordination
- InterruptCoordinator - Group interrupt rotation
- InterruptManager - Interrupt execution
- ThreatCoordinator - Group threat management
- TargetSelector - Intelligent target selection
- CombatStateManager - Combat state tracking

---

### Lifecycle Management (`Lifecycle/`)

**Key Components:**
- BotSpawnOrchestrator - Master coordinator ✅ USE THIS
- StartupSpawnOrchestrator - Startup logic
- DeathRecoveryManager - Death/resurrection
- BotSpawner - Individual bot spawning
- ResourceMonitor - Performance monitoring
- AdaptiveSpawnThrottler - Dynamic throttling

**DO NOT:**
- ❌ Implement custom spawn logic
- ❌ Manually handle bot deaths
- ✅ Always use BotSpawnOrchestrator

---

### Session Management (`Session/`)

**Key Components:**
- BotSession - Core session wrapper
- BotSessionMgr - Lifecycle management
- BotPacketSimulator - Packet simulation
- BotPacketRelay - Packet routing
- BotPerformanceMonitor - Performance tracking

---

### Movement & Navigation (`Movement/`)

**Key Components:**
- QuestPathfinder - Quest navigation ✅ Uses QuestHubDatabase
- LeaderFollowBehavior - Leader following
- GroupFormationManager - Formation management
- BotMovementUtil - Movement utilities

**Dependencies:**
- Quest/QuestHubDatabase ✅ COMPLETE

---

### Event Bus Architecture

**12 Specialized Event Buses:**
1. Combat/CombatEventBus
2. Cooldown/CooldownEventBus
3. Aura/AuraEventBus
4. Loot/LootEventBus
5. Quest/QuestEventBus
6. Resource/ResourceEventBus
7. Social/SocialEventBus
8. Auction/AuctionEventBus
9. NPC/NPCEventBus
10. Instance/InstanceEventBus
11. Group/GroupEventBus
12. Lifecycle/BotSpawnEventBus

**Pattern:** Publish/Subscribe, thread-safe, decoupled

---

## Common Pitfalls & Solutions

### ❌ Pitfall 1: Creating Stub Files
**Example:** Creating `Movement/QuestHubDatabase.h` stub
**Why Wrong:** Real implementation exists at `Quest/QuestHubDatabase.cpp/.h`
**Solution:** Always grep first: `grep -r "QuestHubDatabase" src/modules/Playerbot`

### ❌ Pitfall 2: Wrong Include Paths
**Example:** `#include "QuestHubDatabase.h"` in QuestPathfinder
**Why Wrong:** Finds wrong file (stub in Movement/ instead of real in Quest/)
**Solution:** Use full path: `#include "Quest/QuestHubDatabase.h"`

### ❌ Pitfall 3: Using Removed APIs
**Example:** `creature->IsFlightMaster()`
**Why Wrong:** Method removed in TrinityCore 11.2
**Solution:** Use `creature->IsTaxi()` or `NPCInteractionManager::IsFlightMaster()`

### ❌ Pitfall 4: Bypassing Managers
**Example:** Custom bot spawn loop
**Why Wrong:** Ignores throttling, monitoring, circuit breakers
**Solution:** Use `BotSpawnOrchestrator`

### ❌ Pitfall 5: Tight Coupling
**Example:** Directly calling methods between unrelated systems
**Why Wrong:** Creates dependencies, hard to test
**Solution:** Use appropriate EventBus

---

### Humanization Systems (`Humanization/`)

**Phase 3: GOD_TIER Humanization Core**

**AFKSimulator/** - AFK Behavior Simulation
- AFKSimulator - Simulates human-like AFK patterns ✅ COMPLETE
- Controls idle durations, bio breaks, food breaks
- Integrates with HumanizationManager

**Fishing/** - Fishing Behavior
- FishingSessionManager - Manages fishing sessions ✅ COMPLETE
- Natural session timing, zone preferences
- Equipment handling and auto-bait

**Mounts/** - Mount Collection System
- MountCollectionManager - Mount farming and collection ✅ COMPLETE (GOD_TIER Task 7)
- Tracks collectible mounts by source (Vendor, Rep, Raid Drop, Dungeon Drop, etc.)
- Priority-based farm selection
- Session tracking with statistics
- File: `Humanization/Mounts/MountCollectionManager.h/.cpp`

**Collections/** - Pet Collection System
- PetCollectionManager - Battle pet farming and collection ✅ COMPLETE (GOD_TIER Task 8)
- Wild capture, vendor purchase, drop farming
- Quality-based capture filtering
- Coordinates with BattlePetManager for captures
- File: `Humanization/Collections/PetCollectionManager.h/.cpp`

**Achievements/** - Achievement Grinding
- AchievementGrinder - Achievement grind execution ✅ COMPLETE (GOD_TIER Task 10)
- Exploration, Kill, Quest, Dungeon, Raid achievements
- Navigates to objectives and tracks progress
- Coordinates with AchievementManager for goal selection
- File: `Humanization/Achievements/AchievementGrinder.h/.cpp`

**Core/** - Humanization Core
- HumanizationManager - Master coordinator for humanization behaviors ✅ COMPLETE
- Orchestrates all humanization subsystems
- Natural behavior patterns, activity variance

**Pattern:** All managers extend BehaviorManager base class with:
- Static shared databases across bots
- Per-bot session tracking
- Atomic statistics
- Callback notifications
- Update interval control

---

## File Count Summary

```
Total Files: ~1010+

By Category:
- AI Systems:           ~280 files
- Lifecycle:            ~26 files
- Session:              ~18 files
- Combat:               ~30 files
- Quest:                ~8 files
- Movement:             ~10 files
- Database:             ~14 files
- Event Buses:          ~24 files (12 pairs)
- Network:              ~13 files
- Group:                ~12 files
- Game Interaction:     ~12 files
- Spatial:              ~8 files
- Character:            ~10 files
- Equipment:            ~4 files
- Professions:          ~8 files
- Economy:              ~6 files
- Social:               ~8 files
- Configuration:        ~8 files
- Humanization:         ~12 files (NEW - GOD_TIER Phase 3)
- Tests:                ~5 files (active)
- Other:                ~100+ files
```

---

## System Status Legend

| Symbol | Status | Meaning |
|--------|--------|---------|
| ✅ | COMPLETE | Production-ready, fully tested |
| ⚠️ | PARTIAL | Core complete, enhancements ongoing |
| 🔧 | DISABLED | Temporarily excluded (e.g., UnifiedInterruptSystem) |
| ❌ | MISSING | Planned but not implemented |

---

## Maintenance Protocol

**Update this document when:**
- ✅ Completing major features or subsystems
- ✅ Adding new directories or managers
- ✅ After each development phase completion
- ✅ When TrinityCore API changes occur
- ✅ When discovering duplicate/redundant code

**Update Process:**
1. Document new systems in appropriate section
2. Add to quick lookup table
3. Update file counts
4. Add common pitfalls if discovered
5. Update "Last Updated" timestamp

---

## References

**Related Documentation:**
- `CLAUDE.md` - Development guidelines and workflows
- `.claude/PHASE_4C_4D_COMPLETION_REPORT_2025-11-01.md` - Latest completion report
- `src/modules/Playerbot/Documentation/` - Technical documentation
- `src/modules/Playerbot/Tests/` - Test suite documentation

**External References:**
- TrinityCore 11.2 API: `src/server/game/` headers
- WoW 11.2 (The War Within) mechanics
- C++20 standard library

---

**Document Version:** 1.0
**Last Updated:** 2025-11-01
**Phase:** 4C/4D Complete
**Next Review:** After Phase 5 completion
