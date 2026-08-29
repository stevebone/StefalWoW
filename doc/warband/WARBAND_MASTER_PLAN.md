# Warband System - Master Implementation Plan

## Scope

Complete, blizzlike implementation of the entire Warband system for TrinityCore (12.0.x / Midnight).
The Warband system was introduced in The War Within (11.0) and carries forward into Midnight (12.0).

---

## Current Implementation Status

### Completed
| Component | Status | Details |
|-----------|--------|---------|
| WarbandScene DB2 | Done | Struct, store, load info, hotfix selectors |
| WarbandScenePlacement DB2 | Done | Struct, store, load info, placement-by-scene index |
| Scene collection system | Done | CollectionMgr: Load/Save/Add/Has/SetFavorite/Send |
| Scene update fields | Done | ActivePlayerData::WarbandScenes dynamic bitmask |
| Scene collection packets | Done | AccountItemCollectionData, AccountWarbandSceneUpdate |
| SPELL_EFFECT_LEARN_WARBAND_SCENE | Done | Effect 341, handler EffectLearnWarbandScene |
| Warband group management | Done | HandleCharEnum auto-creates, HandleSetupWarbandGroups persists |
| Character DB tables | Done | character_warband_groups, character_warband_group_members |
| Auth DB table | Done | battlenet_account_warband_scenes |
| Favorite/Fanfare flags | Done | CollectionItemSetFavorite handler for WarbandScene type |

### Not Implemented
| Component | Priority | Complexity |
|-----------|----------|------------|
| 7 missing DB2 structures | High | Medium |
| Warband bank | High | High |
| Warbound item binding | High | High |
| Account-wide reputation | Medium | High |
| Currency transfer | Medium | Medium |
| Account-wide achievements | Medium | Very High |
| Flight path sharing | Medium | Low |
| XP bonus per max-level char | Medium | Low |
| Transmog armor-type agnostic | Low | High |
| Delve companion progression | Low | Medium |
| Exploration map sharing | Low | Low |

---

## Phase Overview

| Phase | Name | Focus | Depends On |
|-------|------|-------|------------|
| 1 | Warband Scene Complete | Complete the campsite/scene system | - |
| 2 | Warbound Items | New binding types (BtW, BtWuE) | - |
| 3 | Warband Bank | Account-wide shared storage | Phase 2 |
| 4 | Account-Wide Reputation | Reputation sharing across characters | - |
| 5 | Currency Transfer | Cross-character currency transfer | - |
| 6 | Flight Path & XP Sharing | Account-wide flight paths, alt XP bonus | - |
| 7 | Account-Wide Achievements | ~2000 achievements converted | - |
| 8 | Transmog Overhaul | Armor-type agnostic collection | - |
| 9 | Delve Companion | Warband-wide companion progression | - |

---

## Phase 1: Warband Scene Complete

**Goal:** 100% blizzlike warband campsite/scene system on the character selection screen.

### 1.1 Missing DB2 Structures

Add C++ structs, load info, stores, hotfix selectors, and helper functions for:

#### WarbandSceneAnimation (FileDataId: 5754555)
Animation keyframe data for character poses within scenes.
```
Fields: ID, AnimKitID(?), WarbandScenePlacementID(?), AnimDuration(?),
        AnimDelay(?), BlendTime(?), Priority(?), LoopCount(?),
        Flags(?), IntArray[2](?)
```
Metadata: 10 fields (INT×7, FLOAT×1, BYTE×3, INT[2]×1)

#### WarbandSceneAnimChrSpec (FileDataId: 6238760)
Maps character class/spec to specific animations.
```
Fields: WarbandSceneAnimationID(?), ChrSpecializationID(?)
```
Metadata: 2 fields (INT×2)

#### WarbandScenePlacementOption (FileDataId: 6316088)
Variant placement positions (alternative spots within a scene).
```
Fields: Position[3], ID, WarbandSceneID(parent), Rotation(?), Scale(?),
        SlotType(?), SlotID(?)
```
Metadata: 7 fields (FLOAT[3]×1, INT×4, FLOAT×2)

#### WarbandScenePlacementFilterReq (FileDataId: 6251586)
Requirements/conditions for showing specific placements.
```
Fields: RaceMask(?)(LONG), ID, WarbandSceneID(parent),
        ClassMask(?), LevelReq(?), Flags(?)
```
Metadata: 6 fields (LONG×1, INT×3, BYTE×1)

#### WarbandScenePlcmntAnimOverride (FileDataId: 5756286)
Per-placement animation overrides.
```
Fields: WarbandScenePlacementID(?), WarbandSceneAnimationID(?)
```
Metadata: 2 fields (INT×2)

#### WarbandPlacementDisplayInfo (FileDataId: 6655794)
Display/model rendering properties for placements.
```
Fields: WarbandScenePlacementID(?), ModelFileDataID(?),
        TextureKitID(?), Scale(?), Flags(?)
```
Metadata: 5 fields (INT×5)

#### WarbandSceneSourceInfo (FileDataId: 6388961)
How/where scenes are obtained (drops, quests, achievements, vendors).
```
Fields: SourceDescription(STRING), ID, WarbandSceneID(parent), SourceType(?)
```
Metadata: 4 fields (STRING×1, INT×2, BYTE×1)

### 1.2 DB2 Helper Functions

- `GetWarbandSceneAnimations(uint32 placementId)` - Get animations for a placement
- `GetWarbandSceneAnimChrSpec(uint32 animId)` - Get class/spec filter for animation
- `GetWarbandScenePlacementOptions(uint32 sceneId)` - Get placement options for scene
- `GetWarbandScenePlacementFilterReqs(uint32 sceneId)` - Get filter requirements
- `GetWarbandScenePlcmntAnimOverrides(uint32 placementId)` - Get animation overrides
- `GetWarbandPlacementDisplayInfo(uint32 placementId)` - Get display info
- `GetWarbandSceneSourceInfos(uint32 sceneId)` - Get source info for scene

### 1.3 Hotfix Database Tables

Create hotfix SQL tables for all 7 new DB2 stores with appropriate columns matching metadata.

### 1.4 Scene Acquisition System

Implement scene unlock mechanics:
- Spell effect already works (SPELL_EFFECT_LEARN_WARBAND_SCENE)
- Need hotfix data for actual scenes (populate warband_scene table with known scenes)
- Need source info display (WarbandSceneSourceInfo)

### 1.5 Known Warband Scenes Data

Populate hotfixes database with known scenes:
| ID | Name | Source |
|----|------|--------|
| 1 | Adventurer's Rest | Default (AwardedAutomatically) |
| 2+ | Ohn'ahran Overlook | Auto-unlocked post-11.1 |
| 3+ | Cultists' Quay | Vendor (10k Undercoin) |
| 4+ | Freywold Spring | Achievement: "All That Khaz" |
| 5+ | Gallagio Grand Gallery | Achievement: "Going Goblin Mode" |
| 6+ | Founder's Point | Quest: Housing (Alliance) |
| 7+ | Razorwind Shores | Quest: Housing (Horde) |

### 1.6 Group Management Enhancements

Current: Max 5 groups. Retail (11.1+): Max 20 groups.
- Update group limit from 5 to 20
- Validate group limit matches retail behavior
- Ensure proper character drag-and-drop support (client-side, server validates)

### 1.7 Packet Verification

Verify all packet structures match retail wire format:
- EnumCharactersResult warband group serialization
- SetupWarbandGroups deserialization
- AccountWarbandSceneUpdate serialization
- AccountItemCollectionData for WarbandScene type

---

## Phase 2: Warbound Items

**Goal:** Implement new item binding types for account-wide item sharing.

### 2.1 Binding Type Flags
- Add `BIND_TO_WARBAND` binding type
- Add `BIND_TO_WARBAND_UNTIL_EQUIPPED` binding type
- Items with BtW stay warbound even after equipping
- Items with BtWuE become soulbound on equip

### 2.2 Item Template Changes
- Update item_template to support new binding flags
- Ensure proper binding transitions (BtWuE → BoP on equip)

### 2.3 Mail System
- Allow warbound items to be mailed cross-realm within the warband
- Validate binding rules on mail send/receive

### 2.4 Trade System
- Allow warbound items to be traded between warband members
- Block soulbound items from warband trade

---

## Phase 3: Warband Bank

**Goal:** Account-wide shared storage accessible by all characters.

### 3.1 Database Schema
```sql
CREATE TABLE warband_bank_tab (
    battlenetAccountId INT UNSIGNED NOT NULL,
    tabId TINYINT UNSIGNED NOT NULL,
    tabName VARCHAR(128) DEFAULT '',
    tabIcon INT UNSIGNED DEFAULT 0,
    PRIMARY KEY (battlenetAccountId, tabId)
);

CREATE TABLE warband_bank_item (
    battlenetAccountId INT UNSIGNED NOT NULL,
    tabId TINYINT UNSIGNED NOT NULL,
    slotId TINYINT UNSIGNED NOT NULL,
    itemGuid BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (battlenetAccountId, tabId, slotId)
);
```

### 3.2 Storage Rules
- 5 tabs, 98 slots per tab (490 total)
- Tab unlock costs: 1k, 25k, 100k, 500k, 2.5M gold
- Only non-soulbound items can be stored
- Warbound and WuE items can be stored
- Gold deposit/withdraw cross-realm

### 3.3 Tab Customization
- Rename tabs with custom names
- Assign custom icons to tabs

### 3.4 Mobile Bank Access
- Quest chain reward: Warband Bank Distance Inhibitor spell
- Summons portable bank portal for 10 minutes, 4-hour cooldown

### 3.5 Opcodes
- Bank open/close
- Deposit/withdraw items
- Deposit/withdraw gold
- Tab purchase
- Tab rename/icon change
- "Deposit All Warbound Items" button

### 3.6 Crafting Integration
- Crafting system should check warband bank for reagents
- Similar to personal reagent bank functionality

---

## Phase 4: Account-Wide Reputation

**Goal:** Share reputation progress across all characters in the warband.

### 4.1 Data Model
- Track highest reputation per faction per account
- Display highest value on all characters
- New table: `warband_reputation` keyed by battlenetAccountId + factionId

### 4.2 Eligible Factions
War Within factions (fully account-wide):
- Council of Dornogal
- The Assembly of the Deeps
- Hallowfall Arathi
- The Severed Threads

### 4.3 First-Time Quest Bonus
- First character gets full reputation
- Alts get bonus currency instead
- Track which quests have been completed account-wide

### 4.4 Renown System
- Display highest renown level across characters
- Unlock rewards at renown thresholds for all characters

---

## Phase 5: Currency Transfer

**Goal:** Cross-character currency transfer system.

### 5.1 Transfer Mechanics
- Character-to-character transfer via Currency tab
- Receiving character initiates transfer
- Transfer log tracking

### 5.2 Transfer Rates
Most currencies: 1:1. Some with loss:
- Honor: 20% loss
- Valorstones: 20% loss
- War Resources: 24% loss

### 5.3 Fully Account-Wide Currencies
- Trader's Tender (same balance on all characters)
- Plunder

### 5.4 UI/Packets
- CMSG for initiating transfer
- SMSG for transfer result
- Currency log packet

---

## Phase 6: Flight Path & XP Sharing

**Goal:** Account-wide flight paths and alt XP bonuses.

### 6.1 Flight Path Sharing
- All discovered flight paths unlock account-wide
- Sync on login: merge all characters' flight paths
- Store in account-level table

### 6.2 Alt XP Bonus
- Each level 80 character grants 5% XP bonus
- Stacks up to 5 times (25% max)
- Apply as aura/modifier on login
- Track count of max-level characters per account

---

## Phase 7: Account-Wide Achievements

**Goal:** Convert ~2000 achievements to account-wide.

### 7.1 Achievement Framework Changes
- Flag achievements as account-wide vs character-specific
- Aggregate progress across characters
- Use highest value for level/progression-based achievements
- Single warband achievement score

### 7.2 Shared Rewards
- Titles become account-wide
- Dungeon teleports shared
- Other perks shared

### 7.3 Exceptions
- Keep character-specific: Insane in the Membrane, seasonal PvP titles, etc.

---

## Phase 8: Transmog Overhaul

**Goal:** Armor-type agnostic appearance collection.

### 8.1 Collection Changes
- Any character can collect any armor type appearance
- Quest rewards unlock ALL armor type options
- Transmog roll option in loot

### 8.2 UI Changes
- Dropdown to view appearances for any class
- Cross-class appearance browsing

---

## Phase 9: Delve Companion

**Goal:** Warband-wide companion progression.

### 9.1 Companion Data
- Shared companion level and XP across characters
- Weekly XP cap (warband-wide)
- Role selection (healer/DPS)

### 9.2 Database
- Account-level companion table
- XP tracking with weekly reset

---

## Git Branching Strategy

Each phase is developed on its own branch and submitted as a separate PR when the full featureset is complete.

### Branch Naming
```
warband/phase1-scene
warband/phase2-items
warband/phase3-bank
warband/phase4-reputation
warband/phase5-currency
warband/phase6-flightpath-xp
warband/phase7-achievements
warband/phase8-transmog
warband/phase9-delves
```

### Workflow
1. Each phase branches from the previous phase (stacked branches)
2. Base commit: `0b6d4eaf8e` (Core: Updated to 12.0.1)
3. When starting a new phase: `git checkout warband/phaseN-xxx && git checkout -b warband/phaseN+1-yyy`
4. PRs are created when the entire warband featureset is complete
5. PRs are submitted in order, each targeting `master`
6. After each PR merges, rebase the next branch onto updated `master`

### Current Branch Status
| Branch | Status | Base |
|--------|--------|------|
| `warband/phase1-scene` | Complete | `0b6d4eaf8e` |
| `warband/phase2-items` | Not started | Will branch from phase1 |
| `warband/phase3-bank` | Not started | Will branch from phase2 |
| ... | ... | ... |

---

## Reference Resources

### IDA Decompilation (C:\dump2)
- `deep_analysis/warband_decompiled.c` - 2,380 decompiled client functions
- `deep_analysis/warband_api_reference.json` - Lua API catalog (150 core functions)
- `deep_analysis/warband_safe_analysis.json` - Structural analysis
- `ida_full_extraction/func_database.json` - Full function database

### Client Data (M:\WorldofWarcraft)
- `dbc/enUS/WarbandScene*.db2` - All 9 warband scene DB2 files
- `gt/` - GameTable files

### Key Client Lua APIs (from IDA)
- `ChangeGroupWarbandScene(groupID, warbandSceneID)`
- `IsValidWarbandGroupName(name)`
- `SetWarbandGroupCollapsedState(groupID, collapsed)`
- `ChangeAllWarbandScenes(sceneID)`
- `IsMapSceneLoaded()`
- `GetLoadedMapScene()`

### Client Events (from IDA)
- `WARBAND_GROUP_SCENE_UPDATED` - Scene changed for a group
- `WARBAND_SCENE_FAVORITES_UPDATED` - Favorites modified
- `WarbandsLoaded` - Initial account data loaded
- `WarbandScenesLoaded` - Scene data ready
- `WarbandScenesLoadedDone` - Initialization complete
