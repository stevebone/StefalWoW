# Phase 1: Warband Scene Complete - Detailed Implementation Plan

## Overview
Complete the warband campsite/scene system to 100% blizzlike behavior. This is the character
selection screen feature where characters are arranged in a 3D scene.

## Current State
- 2 of 9 DB2 structures implemented (WarbandScene, WarbandScenePlacement)
- Basic group management works (create/load/save groups with members)
- Scene collection system works (add/favorite/save scenes)
- Spell effect for learning scenes works

## Tasks

### Task 1.1: Add Missing DB2 Structures
**Files to modify:**
- `src/server/game/DataStores/DB2Structure.h` — Add 7 new struct definitions
- `src/server/game/DataStores/DB2LoadInfo.h` — Add 7 new LoadInfo classes
- `src/server/game/DataStores/DB2Stores.h` — Add 7 new extern store declarations + helper method declarations
- `src/server/game/DataStores/DB2Stores.cpp` — Instantiate 7 stores, LOAD_DB2 calls, build indexes, implement helpers
- `src/server/game/DataStores/DBCEnums.h` — Add any new enums if needed

#### 1.1.1 WarbandSceneAnimation
```cpp
struct WarbandSceneAnimationEntry
{
    uint32 ID;
    int32 Field_11_0_0_54210_001;   // Likely WarbandScenePlacementID or AnimKitID
    int32 Field_11_0_0_54210_002;   // Likely AnimKitID or AnimDuration
    int32 Field_11_0_0_54210_003;   // Unknown
    int32 Field_11_0_0_54210_004;   // Unknown
    float Field_11_0_0_54210_005;   // Likely blend time or speed
    uint8 Field_11_0_0_54210_006;   // Likely priority or flags
    uint8 Field_11_0_0_54210_007;   // Likely loop count or type
    int8 Field_11_0_0_54210_008;    // Signed byte - unknown
    std::array<int32, 2> Field_11_0_0_54210_009; // Array of 2 ints
};
```

#### 1.1.2 WarbandSceneAnimChrSpec
```cpp
struct WarbandSceneAnimChrSpecEntry
{
    int32 WarbandSceneAnimationID;  // FK to WarbandSceneAnimation
    int32 ChrSpecializationID;      // FK to ChrSpecialization
};
```

#### 1.1.3 WarbandScenePlacementOption
```cpp
struct WarbandScenePlacementOptionEntry
{
    DBCPosition3D Position;
    uint32 ID;
    int32 WarbandSceneID;           // Parent FK
    float Field_11_1_0_58221_003;   // Likely Rotation
    float Field_11_1_0_58221_004;   // Likely Scale
    int32 Field_11_1_0_58221_005;   // Unknown
    int32 Field_11_1_0_58221_006;   // Unknown
};
```

#### 1.1.4 WarbandScenePlacementFilterReq
```cpp
struct WarbandScenePlacementFilterReqEntry
{
    int64 Field_11_0_2_56421_000;   // Likely RaceMask (64-bit)
    uint32 ID;
    int32 WarbandSceneID;           // Parent FK
    int32 Field_11_0_2_56421_003;   // Likely ClassMask or PlacementID
    int32 Field_11_0_2_56421_004;   // Unknown
    int8 Field_11_0_2_56421_005;    // Likely flags or type
};
```

#### 1.1.5 WarbandScenePlcmntAnimOverride
```cpp
struct WarbandScenePlcmntAnimOverrideEntry
{
    int32 WarbandScenePlacementID;  // FK to WarbandScenePlacement
    int32 WarbandSceneAnimationID;  // FK to WarbandSceneAnimation
};
```

#### 1.1.6 WarbandPlacementDisplayInfo
```cpp
struct WarbandPlacementDisplayInfoEntry
{
    int32 Field_11_1_5_60797_000;   // Likely WarbandScenePlacementID (parent)
    int32 Field_11_1_5_60797_001;   // Unknown
    int32 Field_11_1_5_60797_002;   // Unknown
    int32 Field_11_1_5_60797_003;   // Unknown
    int32 Field_11_1_5_60797_004;   // Unknown
};
```

#### 1.1.7 WarbandSceneSourceInfo
```cpp
struct WarbandSceneSourceInfoEntry
{
    LocalizedString SourceDescription;
    uint32 ID;
    int32 WarbandSceneID;           // Parent FK
    int8 SourceType;                // Enum: how obtained
};
```

### Task 1.2: Add Hotfix Database Tables
**Files to modify:**
- `src/server/database/Database/Implementation/HotfixDatabase.h` — Add prepared statement enums
- `src/server/database/Database/Implementation/HotfixDatabase.cpp` — Register prepared statements
- `sql/updates/hotfixes/master/` — New SQL file creating tables

Create hotfix tables for each new DB2:
- `warband_scene_animation`
- `warband_scene_anim_chr_spec`
- `warband_scene_placement_option`
- `warband_scene_placement_filter_req`
- `warband_scene_plcmnt_anim_override`
- `warband_placement_display_info`
- `warband_scene_source_info`

### Task 1.3: Update Group Limit
**Files to modify:**
- `src/server/game/Handlers/CharacterHandler.cpp` — Change max groups from 5 to 20

Retail (Patch 11.1+) supports up to 20 camp groups.

### Task 1.4: Populate Scene Data
**Files to create:**
- `sql/updates/hotfixes/master/2026_02_20_00_hotfixes.sql` — Insert known warband scene data

Insert data for known scenes into the hotfixes warband_scene table with correct
positions, camera angles, maps, FOV, and flags.

### Task 1.5: Verify Packet Structures
Review and verify that all packet serialization matches retail wire format:
- `CharacterPackets.h/cpp` — WarbandGroup serialization in EnumCharactersResult
- `MiscPackets.h/cpp` — AccountWarbandSceneUpdate
- `CollectionPackets.h/cpp` — AccountItemCollectionData for WarbandScene

### Task 1.6: Test & Validate
- Verify character enum shows warband groups correctly
- Verify group setup saves and reloads properly
- Verify scene collection add/favorite/send works
- Verify spell effect learns new scenes
- Verify default scene auto-creation for new accounts
