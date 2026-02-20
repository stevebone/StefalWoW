# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

TrinityCore WoW server emulator (C++), forked at `github.com/agatho/TrinityCore`. Current branch `feature/warband-scene` implements the warband system starting with the warband scene (campsite) shown on the character selection screen. Target expansion: Midnight (12.0.x).

## Build Commands

```bash
# Configure (Windows with Visual Studio / Ninja)
cmake -S . -B ./build -GNinja \
  -DSERVERS=1 -DTOOLS=1 -DSCRIPTS=static \
  -DUSE_COREPCH=1 -DUSE_SCRIPTPCH=1 \
  -DBUILD_TESTING=1

# Build
cmake --build ./build

# Install (copies binaries + conf.dist files)
cmake --install ./build

# Run tests (Catch2)
ctest --test-dir ./build
```

Key CMake options: `SCRIPTS` (none/static/dynamic), `WITH_WARNINGS=1`, `WITH_WARNINGS_AS_ERRORS=1`, `WITH_COREDEBUG=1`, `WITH_DETAILED_METRICS=1`.

## Database Setup

Four MySQL databases: `world`, `characters`, `auth`, `hotfixes`. Created via `sql/create/create_mysql.sql`. Base schemas in `sql/base/`. Updates in `sql/updates/{auth,characters,world,hotfixes}/master/`.

SQL migration naming: `YYYY_MM_DD_i_database.sql`. When changing `auth` or `characters` schema, also update `sql/base/`.

## Code Style

- `.editorconfig`: 4-space indent, 160-char max line, UTF-8 (Latin-1 for .c/.cpp/.h/.hpp)
- Follow [TrinityCore C++ Development Standards](https://trinitycore.atlassian.net/wiki/spaces/tc/pages/2130103/C+Development+Standards)

## Architecture

### Server Binaries

- **worldserver** — Game world server (ports 8085-8086). Config: `worldserver.conf.dist`
- **bnetserver** — Battle.net auth/login server (port 1119, REST 8081). Config: `bnetserver.conf.dist`

### Source Layout (`src/`)

- `server/game/` — Core game logic (~60 subsystems). The largest component.
  - `Handlers/` — Packet handlers (one per system: `CharacterHandler`, `CollectionsHandler`, etc.)
  - `Entities/` — Game objects: `Player`, `Creature`, `GameObject`, `Item`, `Unit`, `Object`, `SceneObject`
  - `DataStores/` — DB2 file loading. Structures in `DB2Structure.h`, stores in `DB2Stores.h/.cpp`, load metadata in `DB2LoadInfo.h`
  - `Server/Packets/` — Network packet serialization structs (e.g., `CharacterPackets.h`, `CollectionPackets.h`)
  - `Server/Protocol/Opcodes.cpp` — Opcode registry (CMSG/SMSG)
  - `Spells/`, `Combat/`, `Movement/`, `AI/`, `Maps/`, `Quests/` — Major subsystems
- `server/database/Database/Implementation/` — Database layer with prepared statements (`CharacterDatabase.h`, `WorldDatabase.h`, etc.)
- `server/scripts/` — Zone/instance/spell scripts organized by continent
- `server/bnetserver/` — Auth server
- `server/shared/` — Shared networking utilities
- `common/` — Crypto, threading, utilities shared across all targets
- `tools/` — Map/VMap/MMap extractors

### Data Flow Pattern (Packets)

```
Client → CMSG opcode → WorldSession handler → Business logic / Manager
  → Database (prepared statements) → Response → SMSG opcode → Client
```

Handlers validate input, call into managers/entities, persist via `CharacterDatabaseTransaction`, and send response packets.

### DB2 System

Game data definitions (items, spells, scenes, etc.) come from DB2 files loaded at startup. Pattern for adding new DB2 data:
1. Define struct in `DB2Structure.h`
2. Declare store in `DB2Stores.h` (extern), instantiate in `DB2Stores.cpp`
3. Add load info in `DB2LoadInfo.h`
4. Add hotfixes SQL to `sql/updates/hotfixes/master/`

### Prepared Statements Pattern

Database access always uses parameterized prepared statements:
1. Add enum to `CharacterDatabase.h` (e.g., `CHAR_SEL_WARBAND_GROUPS`)
2. Register SQL in `CharacterDatabase.cpp`
3. Use via `CharacterDatabase.GetPreparedStatement(ENUM)` + `SetData()`

## Warband Scene Implementation (Current Work)

### What Exists

- **Warband groups**: Up to 5 groups per account, each with a scene backdrop and up to 4 character members
- **DB tables**: `character_warband_groups`, `character_warband_group_members` (in characters DB); `warband_scene_placement` (in hotfixes DB)
- **DB2 structures**: `WarbandSceneEntry`, `WarbandScenePlacementEntry`
- **Collection integration**: `WarbandScene` is `ItemCollectionType::WarbandScene` (7) in `CollectionMgr`
- **Handlers**: `HandleCharEnum()` loads/auto-creates groups; `HandleSetupWarbandGroups()` persists changes
- **Update fields**: `ActivePlayerData::WarbandScenes` dynamic bitmask array

### Key Files

| Area | Files |
|------|-------|
| Group management | `Handlers/CharacterHandler.cpp`, `Server/Packets/CharacterPackets.h` |
| Collection system | `Entities/Player/CollectionMgr.h/.cpp`, `Server/Packets/CollectionPackets.h/.cpp` |
| DB2 definitions | `DataStores/DB2Structure.h`, `DataStores/DB2Stores.h/.cpp` |
| Database layer | `database/Database/Implementation/CharacterDatabase.h/.cpp` |
| Opcodes | `Server/Protocol/Opcodes.cpp` |
| Update fields | `Entities/Object/Updates/UpdateFields.h` |
| Shared enums | `Miscellaneous/SharedDefines.h` |

### What's Done (Phase 1 Scene Complete)

All 9 warband DB2 structures are now defined, loaded, and hotfix-supported:
- `WarbandScene`, `WarbandScenePlacement` (original)
- `WarbandSceneAnimation`, `WarbandSceneAnimChrSpec`, `WarbandScenePlacementOption`
- `WarbandScenePlacementFilterReq`, `WarbandScenePlcmntAnimOverride`
- `WarbandPlacementDisplayInfo`, `WarbandSceneSourceInfo`

Group limit updated from 5 to 20 (retail 11.1+ behavior).

### What's Next (Remaining Phases)

See `doc/warband/WARBAND_MASTER_PLAN.md` for the full multi-phase plan:
- Phase 2: Warbound item binding types (BtW, BtWuE)
- Phase 3: Warband bank (account-wide shared storage, 5 tabs, 490 slots)
- Phase 4: Account-wide reputation sharing
- Phase 5: Currency transfer system
- Phase 6: Flight path sharing & alt XP bonus
- Phase 7: Account-wide achievements (~2000 conversions)
- Phase 8: Transmog overhaul (armor-type agnostic collection)
- Phase 9: Delve companion warband-wide progression

## Reference Data Locations

### IDA Decompilation Database (`C:\dump2`)

- `wow_dump.bin.i64` — Main IDA database
- `deep_analysis/warband_decompiled.c` — Decompiled warband client functions
- `deep_analysis/warband_api_reference.json` — Warband API reference
- `deep_analysis/warband_safe_analysis.json` — Analysis results
- `ida_full_extraction/` — Function database, signatures, xrefs, named data
- `scripts/phase17_warband_*.py` — Warband-specific IDA extraction scripts

### Client Data (`M:\WorldofWarcraft`)

- `dbc/enUS/` — DB2 files (includes all `WarbandScene*.db2` files)
- `gt/` — GameTable text files (combat ratings, XP curves, etc.)
- `Data/` — Raw game data archive

## Similar Systems for Reference

The **Garrison** system (`src/server/game/Garrison/`) follows a similar account-wide customization pattern and can serve as architectural reference for warband implementation.
