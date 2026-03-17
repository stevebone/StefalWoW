# Automated World Population System - Design Document
## WoW 12.0 (The War Within) Implementation

**Status**: Design Phase
**Priority**: High - Foundation for living, populated game world
**Timeline**: 4-6 weeks implementation
**Dependencies**: BotCharacterCreator, BotSpawner, BotLifecycleMgr

---

## Executive Summary

This document outlines the design for an **Automated World Population System** that distributes AI-controlled player bots across level brackets and zones to create a living, populated game world in WoW 12.0. The system automatically creates, levels, gears, and positions bots to ensure players encounter appropriate bot companions/opponents regardless of their level or location.

### Key Goals

1. **Even Level Distribution**: Maintain configured percentages of bots across level brackets (1-10, 11-20, 21-30, etc.)
2. **Dynamic Zone Population**: Automatically position bots in zones appropriate for their level
3. **Instant Readiness**: Create higher-level bots fully geared and talented without manual leveling
4. **Player-Responsive**: Adjust bot distribution based on real player activity
5. **Faction Balance**: Maintain separate distribution for Alliance and Horde

---

## Reference Analysis: mod-playerbots (3.3.5)

### What Works Well (Inspiration)

#### 1. **Level Assignment & Gear System**
```cpp
// From RandomPlayerbotMgr::IncreaseLevel()
void IncreaseLevel(Player* bot) {
    uint8 level = bot->GetLevel() + 1;
    PlayerbotFactory factory(bot, level);
    factory.Randomize(true);  // Full gear, talents, spells
}

// From PlayerbotFactory::Randomize()
void Randomize(bool incremental) {
    bot->GiveLevel(level);              // Instant level-up
    bot->InitStatsForLevel(true);       // Set stats
    InitTalentsTree();                  // Apply talents
    InitEquipment(incremental);         // Full gear
    InitSpells();                       // Learn spells
    InitMounts();                       // Add mounts
    InitConsumables();                  // Add consumables
}
```

**Insight**: Creating a higher-level bot is a **single-pass operation** that:
- Sets character level instantly
- Applies appropriate talents for spec
- Equips level-appropriate gear
- Learns all class spells up to that level
- Adds mounts, consumables, and reagents

---

## TrinityCore Native Leveling System (CRITICAL)

### How TrinityCore GM `.levelup` Command Works

After researching TrinityCore's implementation, we discovered the **proper way to level up characters**:

**GM Command Implementation** (`cs_character.cpp:783`):
```cpp
static bool HandleLevelUpCommand(ChatHandler* handler, Optional<PlayerIdentifier> player, int16 level)
{
    uint8 oldlevel = player->GetConnectedPlayer()->GetLevel();
    int16 newlevel = static_cast<int16>(oldlevel) + level;

    if (Player* target = player->GetConnectedPlayer())
    {
        target->GiveLevel(static_cast<uint8>(newlevel));  // ← THE KEY FUNCTION
        target->InitTalentForLevel();
        target->SetXP(0);
    }
}
```

### What `Player::GiveLevel()` Does (Player.cpp:2262)

**Critical Discovery**: `GiveLevel()` is the **complete, proper TrinityCore leveling system** that handles EVERYTHING:

```cpp
void Player::GiveLevel(uint8 level)
{
    uint8 oldLevel = GetLevel();

    // 1. Guild updates
    if (Guild* guild = GetGuild())
        guild->UpdateMemberData(this, GUILD_MEMBER_DATA_LEVEL, level);

    // 2. Get level info from database
    PlayerLevelInfo info;
    sObjectMgr->GetPlayerLevelInfo(GetRace(), GetClass(), level, &info);

    // 3. Send level-up packet to client
    WorldPackets::Misc::LevelUpInfo packet;
    packet.Level = level;
    packet.NumNewTalents = DB2Manager::GetNumTalentsAtLevel(level, Classes(GetClass()))
                         - DB2Manager::GetNumTalentsAtLevel(oldLevel, Classes(GetClass()));
    SendDirectMessage(packet.Write());

    // 4. Set new level
    SetLevel(level);

    // 5. ⭐ Learn ALL skills for new level
    UpdateSkillsForLevel();
    LearnDefaultSkills();           // Learns ALL skills where MinLevel <= level
    LearnSpecializationSpells();    // Learns ALL spec spells where SpellLevel <= level

    // 6. Initialize base stats
    for (uint8 i = STAT_STRENGTH; i < MAX_STATS; ++i)
        SetCreateStat(Stats(i), info.stats[i]);

    // 7. Initialize talents
    InitTalentForLevel();           // Sets talent tiers, removes invalid talents
    InitTaxiNodesForLevel();        // Unlocks flight paths

    // 8. Recalculate everything
    UpdateAllStats();

    // 9. Set health/mana to full
    SetFullHealth();
    for (PowerTypeEntry const* powerType : sPowerTypeStore)
        if (powerType->GetFlags().HasFlag(PowerTypeFlags::SetToMaxOnLevelUp))
            SetFullPower(Powers(powerType->PowerTypeEnum));

    // 10. Sync pet level
    if (Pet* pet = GetPet())
        pet->SynchronizeLevelWithOwner();

    // 11. Send level-up mail rewards
    if (MailLevelReward const* mailReward = sObjectMgr->GetMailLevelReward(level, GetRace()))
        MailDraft(mailReward->mailTemplateId).SendMailTo(trans, this, ...);

    // 12. Update achievements/criteria
    StartCriteria(CriteriaStartEvent::ReachLevel, level);
    UpdateCriteria(CriteriaType::ReachLevel);

    // 13. Trigger scripts
    sScriptMgr->OnPlayerLevelChanged(this, oldLevel);
}
```

### Why This Is Perfect for Us

**Multi-Level Jumps Work!**
- ✅ `bot->GiveLevel(1)` → `bot->GiveLevel(50)` **works correctly**
- ✅ Learns **ALL** skills for levels 1-50 (not just level 50)
- ✅ Learns **ALL** spells for levels 1-50 (verified in code)
- ✅ Goes through TrinityCore's proper systems
- ✅ Triggers all hooks, events, and scripts
- ✅ No need for `InitStatsForLevel(true)` separately

**Proof from Code**:
```cpp
// LearnDefaultSkills() - iterates ALL skills
for (PlayerCreateInfoSkills::const_iterator itr = info->skills.begin(); ...)
{
    if (rcInfo->MinLevel > GetLevel())  // ← Learns ALL where MinLevel <= current level
        continue;
    LearnDefaultSkill(rcInfo);
}

// LearnSpecializationSpells() - learns ALL spec spells
for (SpecializationSpellsEntry const* specSpell : *specSpells)
{
    if (spellInfo->SpellLevel > GetLevel())  // ← Learns ALL where SpellLevel <= current level
        continue;
    LearnSpell(specSpell->SpellID, true);
}
```

### Recommended Approach for Bot Leveling

**DO THIS** ✅ (Match GM Command Exactly):
```cpp
// Follow TrinityCore's official GM .levelup command pattern
bot->GiveLevel(targetLevel);        // Instant level-up with all skills/spells
bot->InitTalentForLevel();           // Ensure client receives talent data
bot->SetXP(0);                       // Reset XP
```

**Why call `InitTalentForLevel()` twice?**
- `GiveLevel()` calls it internally during level-up
- GM command calls it **again** to ensure `SendTalentsInfoData()` updates the client UI
- This guarantees the talent frame is properly synchronized for online players

**NOT THIS** ❌:
```cpp
// Old mod-playerbots approach - unnecessary
bot->GiveLevel(targetLevel);
bot->InitStatsForLevel(true);  // ← Redundant! GiveLevel already does this
InitTalentsTree();              // ← Redundant! GiveLevel calls InitTalentForLevel()
InitSpells();                   // ← Redundant! GiveLevel calls LearnDefaultSkills() + LearnSpecializationSpells()
```

### Benefits of Using Native System

1. **Correctness**: Uses TrinityCore's tested, proven leveling logic
2. **Maintainability**: Automatically benefits from TrinityCore updates
3. **Completeness**: No risk of missing steps (mail rewards, achievements, scripts)
4. **Performance**: Single optimized function vs multiple separate calls
5. **Compatibility**: Works with all TrinityCore hooks and addons

#### 2. **Zone-to-Level Mapping**
```cpp
// From RandomPlayerbotMgr::PrepareZone2LevelBracket()
zone2LevelBracket[1] = {5, 12};      // Dun Morogh
zone2LevelBracket[12] = {5, 12};     // Elwynn Forest
zone2LevelBracket[14] = {5, 12};     // Durotar
zone2LevelBracket[17] = {10, 25};    // Barrens
zone2LevelBracket[38] = {10, 20};    // Loch Modan
zone2LevelBracket[3483] = {58, 66};  // Hellfire Peninsula (TBC)
zone2LevelBracket[65] = {71, 77};    // Dragonblight (WotLK)
```

**Insight**: Static mapping of zone IDs to level brackets, overridable by configuration.

#### 3. **Teleport Location Cache**
```cpp
// From RandomPlayerbotMgr
std::map<uint8, std::vector<WorldLocation>> locsPerLevelCache;
std::map<uint8, std::vector<WorldLocation>> allianceStarterPerLevelCache;
std::map<uint8, std::vector<WorldLocation>> hordeStarterPerLevelCache;
std::map<uint8, std::vector<WorldLocation>> bankerLocsPerLevelCache;

// Populated from database query
QueryResult results = WorldDatabase.Query(
    "SELECT g.map, position_x, position_y, position_z, t.minlevel, t.maxlevel "
    "FROM creatures WHERE ...");
```

**Insight**: Pre-cached teleport destinations based on:
- Creature spawn locations in appropriate zones
- Flight master locations
- Banker/innkeeper locations
- Faction-specific starting areas

#### 4. **Smart Teleportation**
```cpp
void RandomTeleportForLevel(Player* bot) {
    uint32 level = bot->GetLevel();
    uint8 race = bot->getRace();
    std::vector<WorldLocation>* locs =
        IsAlliance(race) ? &allianceStarterPerLevelCache[level]
                        : &hordeStarterPerLevelCache[level];

    // Chance to teleport to banker/inn
    if (level >= 10 && urand(0, 100) < probTeleToBankers * 100) {
        RandomTeleport(bot, bankerLocsPerLevelCache[level], true);
    } else {
        RandomTeleport(bot, *locs);
    }
}
```

**Insight**: Bots are teleported to:
- Faction-appropriate zones for their level
- Sometimes to cities (bankers/inns) for variety
- Never to enemy faction zones

---

## Reference Analysis: mod-player-bot-level-brackets

### What It Adds (Essential Features)

#### 1. **Bracket-Based Distribution**
```
Configuration Example (9 brackets for level 1-80):
- Range1: Levels 1-9   → 12% of bots
- Range2: Levels 10-19 → 11% of bots
- Range3: Levels 20-29 → 11% of bots
- Range4: Levels 30-39 → 11% of bots
- Range5: Levels 40-49 → 11% of bots
- Range6: Levels 50-59 → 11% of bots
- Range7: Levels 60-69 → 11% of bots
- Range8: Levels 70-79 → 11% of bots
- Range9: Level 80     → 11% of bots
```

**Key Features**:
- Separate brackets for Alliance and Horde
- Percentages must sum to 100%
- Configurable bracket ranges (can isolate specific levels)
- Periodic rebalancing to maintain percentages

#### 2. **Dynamic Distribution**
```
When enabled, bot distribution adjusts based on real player activity:
- More players at level 60? → More bots at level 60
- Uses weighted algorithm: botCount = baseCount + (playerCount * weight)
- Prevents empty zones while maintaining population
```

#### 3. **Smart Exclusions**
- **Guild Protection**: Bots in guilds with real players are never adjusted
- **Friend List Protection**: Bots on real players' friend lists are protected
- **Combat Safety**: Never adjusts bots in combat or unsafe states
- **Death Knight Minimum**: DKs never go below level 55

#### 4. **Periodic Rebalancing**
```
Default timings:
- BotLevelBrackets.CheckFrequency = 300 seconds (5 minutes)
- BotLevelBrackets.CheckFlaggedFrequency = 15 seconds
- BotLevelBrackets.FlaggedProcessLimit = 5 per cycle
```

**Process**:
1. Check current distribution every 5 minutes
2. Identify overpopulated brackets
3. Flag bots for adjustment (if safe)
4. Process flagged bots when they become safe (not in combat, etc.)
5. Randomize flagged bots to target level in deficit bracket
6. Teleport to appropriate zone for new level

---

## WoW 12.0 Adaptation Requirements

### Major Differences from 3.3.5

| Aspect | 3.3.5 (WotLK) | 12.0 (The War Within) | Adaptation Needed |
|--------|---------------|----------------------|-------------------|
| **Max Level** | 80 | 80 | ✅ Same (but different progression curve) |
| **Talent System** | Talent trees (3 trees per class) | Specialization + Hero Talents | ⚠️ Requires new implementation |
| **Gear System** | Item level 200-284 | Item level 590-639 (Season 1) | ⚠️ New item pools required |
| **Zones** | Classic + TBC + WotLK | Khaz Algar (new continent) + all previous | ✅ Database query approach works |
| **Expansion Content** | Northrend endgame | Hallowfall, Azj-Kahet, etc. | ⚠️ New zone mappings |
| **Professions** | Simpler system | Profession specializations | ⚠️ May skip for initial version |
| **Delves** | N/A | New solo/group content type | ⚠️ Future enhancement |

### Critical Adaptations

1. **Talent System**: WoW 12.0 uses a completely different talent structure
   - Main spec talents (class tree + spec tree)
   - Hero talents (unlocked at 71+)
   - Need database of valid talent loadouts per spec

2. **Gear Acquisition**: Item database is WoW 12.0 specific
   - Query world database for items by ilvl and slot
   - Respect armor type restrictions (plate/mail/leather/cloth)
   - Consider stat weights for specs

3. **Zone Mapping**: New continents and zones
   - Khaz Algar zones (71-80)
   - Legacy content still accessible
   - Flying/pathfinding considerations

---

## Proposed Architecture for TrinityCore PlayerBot Module

### Component Overview

```
src/modules/Playerbot/
├── Character/
│   ├── BotCharacterDistribution.cpp/h      [EXISTS] - Class/race distribution
│   └── BotLevelDistribution.cpp/h          [NEW] - Level bracket management
├── Lifecycle/
│   ├── BotSpawner.cpp/h                    [EXISTS] - Spawning infrastructure
│   ├── BotLevelManager.cpp/h               [NEW] - Level assignment & rebalancing
│   └── BotWorldPositioner.cpp/h            [NEW] - Zone teleportation system
├── Equipment/
│   └── BotGearFactory.cpp/h                [NEW] - Level-appropriate gear creation
├── Talents/
│   └── BotTalentManager.cpp/h              [NEW] - Talent loadout application
└── Config/
    └── playerbots.conf                     [EXTEND] - Add bracket configuration
```

### New Components Design

---

## 1. BotLevelDistribution

**Purpose**: Manages level bracket configuration and distribution logic.

### Data Structures

```cpp
namespace Playerbot {

struct LevelBracket {
    uint32 minLevel;
    uint32 maxLevel;
    float targetPercentage;     // 0.0 to 1.0
    uint32 currentCount;
    TeamId faction;             // TEAM_ALLIANCE or TEAM_HORDE

    bool ContainsLevel(uint32 level) const {
        return level >= minLevel && level <= maxLevel;
    }

    uint32 GetRandomLevel() const {
        return urand(minLevel, maxLevel);
    }
};

struct LevelDistributionConfig {
    std::vector<LevelBracket> allianceBrackets;
    std::vector<LevelBracket> hordeBrackets;

    bool dynamicDistribution;
    float realPlayerWeight;       // How much real players influence distribution
    bool syncFactions;            // Use same brackets for both factions

    uint32 rebalanceInterval;     // Seconds between rebalance checks
    uint32 flaggedCheckInterval;  // Seconds between flagged bot checks
    uint32 maxFlaggedPerCycle;    // Max bots to process per cycle

    bool excludeGuildBots;
    bool excludeFriendListedBots;
    std::vector<std::string> excludedNames;
};

class TC_GAME_API BotLevelDistribution {
public:
    static BotLevelDistribution* instance();

    void Initialize();
    void LoadConfig();
    void ValidateConfig();

    // Bracket queries
    LevelBracket const* GetBracketForLevel(uint32 level, TeamId faction) const;
    std::vector<LevelBracket> const& GetBrackets(TeamId faction) const;

    // Distribution analysis
    void AnalyzeCurrentDistribution();
    std::vector<uint32> GetOverpopulatedBrackets(TeamId faction) const;
    std::vector<uint32> GetUnderpopulatedBrackets(TeamId faction) const;

    // Dynamic distribution
    void UpdateRealPlayerCounts();
    float CalculateDynamicTargetPercentage(LevelBracket const& bracket) const;

private:
    LevelDistributionConfig _config;
    std::map<TeamId, std::map<uint32, uint32>> _realPlayerCounts; // [faction][bracketIndex] = count
    std::mutex _mutex;
};

#define sBotLevelDistribution BotLevelDistribution::instance()

}
```

### Configuration (playerbots.conf)

```conf
###############################################################################
# AUTOMATED WORLD POPULATION
###############################################################################

# Enable automated level distribution and world population
Playerbot.Population.Enabled = 1

# Number of level brackets to define (every 5 levels)
Playerbot.Population.NumBrackets = 17

# Rebalance check interval (seconds)
Playerbot.Population.RebalanceInterval = 300

# Flagged bot check interval (seconds)
Playerbot.Population.FlaggedCheckInterval = 15

# Max flagged bots to process per cycle
Playerbot.Population.MaxFlaggedPerCycle = 5

# Dynamic distribution (adjust based on real player levels)
Playerbot.Population.DynamicDistribution = 0
Playerbot.Population.RealPlayerWeight = 1.0
Playerbot.Population.SyncFactions = 0

# Exclusions
Playerbot.Population.ExcludeGuildBots = 1
Playerbot.Population.ExcludeFriendListed = 1
Playerbot.Population.ExcludedNames = ""

#
# Alliance Level Brackets (Every 5 Levels)
# Format: Playerbot.Population.Alliance.RangeX.Min/Max/Pct
# Percentages must sum to 100
# NOTE: Level 1-4 bots level naturally (no instant gear)
#

# Natural Leveling Bracket
Playerbot.Population.Alliance.Range1.Min = 1
Playerbot.Population.Alliance.Range1.Max = 4
Playerbot.Population.Alliance.Range1.Pct = 5

# Geared Brackets (5+ get instant level-up + gear)
Playerbot.Population.Alliance.Range2.Min = 5
Playerbot.Population.Alliance.Range2.Max = 9
Playerbot.Population.Alliance.Range2.Pct = 6

Playerbot.Population.Alliance.Range3.Min = 10
Playerbot.Population.Alliance.Range3.Max = 14
Playerbot.Population.Alliance.Range3.Pct = 6

Playerbot.Population.Alliance.Range4.Min = 15
Playerbot.Population.Alliance.Range4.Max = 19
Playerbot.Population.Alliance.Range4.Pct = 6

Playerbot.Population.Alliance.Range5.Min = 20
Playerbot.Population.Alliance.Range5.Max = 24
Playerbot.Population.Alliance.Range5.Pct = 6

Playerbot.Population.Alliance.Range6.Min = 25
Playerbot.Population.Alliance.Range6.Max = 29
Playerbot.Population.Alliance.Range6.Pct = 6

Playerbot.Population.Alliance.Range7.Min = 30
Playerbot.Population.Alliance.Range7.Max = 34
Playerbot.Population.Alliance.Range7.Pct = 6

Playerbot.Population.Alliance.Range8.Min = 35
Playerbot.Population.Alliance.Range8.Max = 39
Playerbot.Population.Alliance.Range8.Pct = 6

Playerbot.Population.Alliance.Range9.Min = 40
Playerbot.Population.Alliance.Range9.Max = 44
Playerbot.Population.Alliance.Range9.Pct = 6

Playerbot.Population.Alliance.Range10.Min = 45
Playerbot.Population.Alliance.Range10.Max = 49
Playerbot.Population.Alliance.Range10.Pct = 6

Playerbot.Population.Alliance.Range11.Min = 50
Playerbot.Population.Alliance.Range11.Max = 54
Playerbot.Population.Alliance.Range11.Pct = 6

Playerbot.Population.Alliance.Range12.Min = 55
Playerbot.Population.Alliance.Range12.Max = 59
Playerbot.Population.Alliance.Range12.Pct = 6

Playerbot.Population.Alliance.Range13.Min = 60
Playerbot.Population.Alliance.Range13.Max = 64
Playerbot.Population.Alliance.Range13.Pct = 6

Playerbot.Population.Alliance.Range14.Min = 65
Playerbot.Population.Alliance.Range14.Max = 69
Playerbot.Population.Alliance.Range14.Pct = 6

Playerbot.Population.Alliance.Range15.Min = 70
Playerbot.Population.Alliance.Range15.Max = 74
Playerbot.Population.Alliance.Range15.Pct = 6

Playerbot.Population.Alliance.Range16.Min = 75
Playerbot.Population.Alliance.Range16.Max = 79
Playerbot.Population.Alliance.Range16.Pct = 6

# Endgame Bracket (higher population)
Playerbot.Population.Alliance.Range17.Min = 80
Playerbot.Population.Alliance.Range17.Max = 80
Playerbot.Population.Alliance.Range17.Pct = 13

# Horde brackets (same structure)
Playerbot.Population.Horde.Range1.Min = 1
Playerbot.Population.Horde.Range1.Max = 10
Playerbot.Population.Horde.Range1.Pct = 12
# ... (same as Alliance for default even distribution)
```

---

## 2. BotLevelManager

**Purpose**: Handles level assignment, rebalancing, and bot flagging.

### Core Logic

```cpp
namespace Playerbot {

struct FlaggedBot {
    ObjectGuid guid;
    uint32 currentLevel;
    uint32 targetLevel;
    uint32 targetBracketIndex;
    time_t flaggedTime;
    std::string reason;
};

class TC_GAME_API BotLevelManager {
public:
    static BotLevelManager* instance();

    void Initialize();
    void Update(uint32 diff);

    // Level assignment for new bots
    uint32 DetermineInitialLevel(TeamId faction) const;

    // Rebalancing
    void CheckAndRebalance();
    void ProcessFlaggedBots();

    // Bot evaluation
    bool CanAdjustBot(Player* bot) const;
    bool IsBotSafe(Player* bot) const;
    bool IsExcluded(Player* bot) const;

    // Level change
    void FlagBotForLevelChange(Player* bot, uint32 targetLevel, std::string reason);
    void ApplyLevelChange(Player* bot, uint32 newLevel);

private:
    void AnalyzeAndFlagBots();
    void ProcessFlaggedBot(FlaggedBot const& flagged);

    uint32 _lastRebalanceCheck;
    uint32 _lastFlaggedCheck;
    std::vector<FlaggedBot> _flaggedBots;
    std::mutex _flaggedMutex;
};

#define sBotLevelManager BotLevelManager::instance()

}
```

### Rebalancing Algorithm

```cpp
void BotLevelManager::CheckAndRebalance() {
    // 1. Get current distribution
    sBotLevelDistribution->AnalyzeCurrentDistribution();

    // 2. For each faction
    for (TeamId faction : {TEAM_ALLIANCE, TEAM_HORDE}) {
        auto overpopulated = sBotLevelDistribution->GetOverpopulatedBrackets(faction);
        auto underpopulated = sBotLevelDistribution->GetUnderpopulatedBrackets(faction);

        if (overpopulated.empty() || underpopulated.empty())
            continue;

        // 3. Select bots from overpopulated brackets
        std::vector<Player*> candidates;
        for (uint32 bracketIdx : overpopulated) {
            // Query bots in this bracket
            auto bots = GetBotsInBracket(faction, bracketIdx);
            for (Player* bot : bots) {
                if (CanAdjustBot(bot)) {
                    candidates.push_back(bot);
                }
            }
        }

        // 4. Shuffle and flag for adjustment
        std::shuffle(candidates.begin(), candidates.end(), std::mt19937{std::random_device{}()});

        uint32 toAdjust = CalculateBotsToAdjust(overpopulated, underpopulated);
        for (uint32 i = 0; i < std::min(toAdjust, (uint32)candidates.size()); ++i) {
            // Pick random underpopulated bracket
            uint32 targetBracketIdx = underpopulated[urand(0, underpopulated.size() - 1)];
            auto& bracket = sBotLevelDistribution->GetBrackets(faction)[targetBracketIdx];
            uint32 targetLevel = bracket.GetRandomLevel();

            FlagBotForLevelChange(candidates[i], targetLevel,
                                 "Rebalancing from overpopulated bracket");
        }
    }
}
```

---

## 3. BotGearFactory - Fully Automated Item Distribution

**Purpose**: Creates realistic, level-appropriate gear for bots instantly with ZERO manual curation.

**Design Philosophy**:
- 100% automated - queries existing WoW database
- Realistic gear mix (not perfect matching sets)
- Quality-based on level: Green/Blue for leveling, Blue/Purple for endgame
- Stat-weighted scoring for spec appropriateness
- All slots guaranteed filled (weapons, armor, trinkets, rings, etc.)

### Core Architecture

```cpp
namespace Playerbot {

struct GearSet {
    std::map<EquipmentSlots, uint32> items; // slot -> item entry
    uint32 averageIlvl;
    float totalScore;  // Combined stat score

    bool HasWeapon() const {
        return items.count(EQUIPMENT_SLOT_MAINHAND) > 0;
    }

    bool IsComplete() const {
        // Check all critical slots are filled
        return items.size() >= 10;  // At minimum: weapon, armor, accessories
    }
};

struct ItemScore {
    uint32 itemEntry;
    float score;
    uint32 ilvl;
    uint32 quality;

    bool operator>(ItemScore const& other) const { return score > other.score; }
};

class TC_GAME_API BotGearFactory {
public:
    static BotGearFactory* instance();

    void Initialize();

    // Main function: equip bot with appropriate gear for level/spec
    bool EquipBotForLevel(Player* bot, uint32 level);

private:
    // Level -> ilvl mapping
    uint32 GetTargetIlvl(uint32 level) const;

    // Quality determination
    std::vector<uint32> GetAllowedQualities(uint32 level) const;

    // Item queries
    std::vector<ItemScore> QueryItemsForSlot(
        uint32 level,
        EquipmentSlots slot,
        Classes cls,
        uint32 spec,
        uint8 armorType
    ) const;

    // Stat scoring system
    float CalculateItemScore(ItemTemplate const* item, Classes cls, uint32 spec) const;
    std::map<ItemModType, float> GetStatWeights(Classes cls, uint32 spec) const;

    // Slot filling
    GearSet BuildGearSet(Player* bot, uint32 level);
    bool FillSlot(GearSet& gear, EquipmentSlots slot, std::vector<ItemScore> const& candidates);
    void EnsureAllSlotsFilled(GearSet& gear, Player* bot, uint32 level);

    // Validation
    bool IsValidItemForBot(ItemTemplate const* item, Player* bot) const;
    uint8 GetArmorTypeForClass(Classes cls) const;

    // Caching (optional - for performance)
    std::map<uint32, std::vector<ItemScore>> _slotCache;  // [slot+level+class] -> scored items
    bool _cacheBuilt;
};

#define sBotGearFactory BotGearFactory::instance()

}
```

---

### Item Level Mapping (WoW 12.0)

Based on The War Within Season 1 itemization:

```cpp
uint32 BotGearFactory::GetTargetIlvl(uint32 level) const {
    // WoW 12.0 ilvl progression
    static const std::map<uint32, uint32> levelToIlvl = {
        // Leveling 1-70
        {1,   5},    {5,   10},   {10,  20},   {15,  30},   {20,  40},
        {25,  50},   {30,  60},   {35,  70},   {40,  80},   {45,  90},
        {50, 100},   {55, 110},   {60, 120},   {65, 130},   {70, 140},

        // The War Within (71-80)
        {71, 558},   {72, 562},   {73, 566},   {74, 570},   {75, 574},
        {76, 578},   {77, 582},   {78, 586},   {79, 590},

        // Endgame (80)
        {80, 593}    // Normal Dungeon / World Quest baseline
    };

    auto it = levelToIlvl.find(level);
    if (it != levelToIlvl.end())
        return it->second;

    // Fallback: interpolate for missing levels
    return 5 + (level * 2);
}
```

---

### Quality Distribution System

```cpp
std::vector<uint32> BotGearFactory::GetAllowedQualities(uint32 level) const {
    if (level >= 80) {
        // Endgame: Mix of Rare (Blue) and Epic (Purple)
        // 60% Blue, 40% Purple
        return {ITEM_QUALITY_RARE, ITEM_QUALITY_EPIC};
    }
    else if (level >= 60) {
        // Pre-endgame: Mostly Blue with some Green
        // 70% Blue, 30% Green
        return {ITEM_QUALITY_UNCOMMON, ITEM_QUALITY_RARE};
    }
    else {
        // Leveling: Mix of Green and Blue
        // 50% Green, 50% Blue
        return {ITEM_QUALITY_UNCOMMON, ITEM_QUALITY_RARE};
    }
}

// Apply quality selection with randomness for realism
uint32 SelectQualityForSlot(std::vector<uint32> const& allowedQualities, EquipmentSlots slot) {
    // Important slots (weapons, chest, legs) bias toward higher quality
    bool isImportantSlot = (slot == EQUIPMENT_SLOT_MAINHAND ||
                           slot == EQUIPMENT_SLOT_CHEST ||
                           slot == EQUIPMENT_SLOT_LEGS);

    if (isImportantSlot && allowedQualities.size() > 1) {
        // 70% chance for higher quality on important slots
        return urand(0, 100) < 70 ? allowedQualities.back() : allowedQualities.front();
    }

    // Random selection for other slots
    return allowedQualities[urand(0, allowedQualities.size() - 1)];
}
```

---

### Stat Weight System (Per Spec)

```cpp
std::map<ItemModType, float> BotGearFactory::GetStatWeights(Classes cls, uint32 spec) const {
    // Stat weights: higher = more valuable for this spec
    std::map<ItemModType, float> weights;

    switch (cls) {
        case CLASS_WARRIOR:
            if (spec == SPEC_WARRIOR_ARMS || spec == SPEC_WARRIOR_FURY) {
                // DPS Warriors: Str > Crit > Haste > Mastery > Vers
                weights[ITEM_MOD_STRENGTH] = 1.0f;
                weights[ITEM_MOD_CRITICAL_STRIKE_RATING] = 0.8f;
                weights[ITEM_MOD_HASTE_RATING] = 0.7f;
                weights[ITEM_MOD_MASTERY_RATING] = 0.6f;
                weights[ITEM_MOD_VERSATILITY] = 0.5f;
            }
            else if (spec == SPEC_WARRIOR_PROTECTION) {
                // Tank Warriors: Stam > Armor > Vers > Mastery > Haste
                weights[ITEM_MOD_STAMINA] = 1.0f;
                weights[ITEM_MOD_ARMOR] = 0.9f;
                weights[ITEM_MOD_VERSATILITY] = 0.8f;
                weights[ITEM_MOD_MASTERY_RATING] = 0.7f;
                weights[ITEM_MOD_HASTE_RATING] = 0.6f;
            }
            break;

        case CLASS_MAGE:
            // All mage specs: Int > Crit/Haste/Mastery (spec dependent) > Vers
            weights[ITEM_MOD_INTELLECT] = 1.0f;
            weights[ITEM_MOD_CRITICAL_STRIKE_RATING] = 0.7f;
            weights[ITEM_MOD_HASTE_RATING] = 0.7f;
            weights[ITEM_MOD_MASTERY_RATING] = 0.7f;
            weights[ITEM_MOD_VERSATILITY] = 0.5f;
            break;

        case CLASS_PRIEST:
            if (spec == SPEC_PRIEST_SHADOW) {
                // Shadow: Int > Haste > Crit > Mastery > Vers
                weights[ITEM_MOD_INTELLECT] = 1.0f;
                weights[ITEM_MOD_HASTE_RATING] = 0.8f;
                weights[ITEM_MOD_CRITICAL_STRIKE_RATING] = 0.7f;
                weights[ITEM_MOD_MASTERY_RATING] = 0.6f;
                weights[ITEM_MOD_VERSATILITY] = 0.5f;
            }
            else {
                // Holy/Disc: Int > Haste > Crit > Vers > Mastery
                weights[ITEM_MOD_INTELLECT] = 1.0f;
                weights[ITEM_MOD_HASTE_RATING] = 0.8f;
                weights[ITEM_MOD_CRITICAL_STRIKE_RATING] = 0.7f;
                weights[ITEM_MOD_VERSATILITY] = 0.6f;
                weights[ITEM_MOD_MASTERY_RATING] = 0.5f;
            }
            break;

        // ... continue for all 13 classes

        default:
            // Generic fallback - primary stat is always best
            weights[ITEM_MOD_STRENGTH] = 1.0f;
            weights[ITEM_MOD_AGILITY] = 1.0f;
            weights[ITEM_MOD_INTELLECT] = 1.0f;
            weights[ITEM_MOD_STAMINA] = 0.5f;
            break;
    }

    return weights;
}
```

---

### Item Scoring Algorithm

```cpp
float BotGearFactory::CalculateItemScore(ItemTemplate const* item, Classes cls, uint32 spec) const {
    if (!item)
        return 0.0f;

    auto statWeights = GetStatWeights(cls, spec);
    float totalScore = 0.0f;

    // Score each stat on the item
    for (uint32 i = 0; i < MAX_ITEM_PROTO_STATS; ++i) {
        ItemModType statType = ItemModType(item->ItemStat[i].ItemStatType);
        int32 statValue = item->ItemStat[i].ItemStatValue;

        if (statValue > 0 && statWeights.count(statType)) {
            totalScore += statValue * statWeights[statType];
        }
    }

    // Bonus for item level (higher ilvl = better base stats)
    totalScore += item->ItemLevel * 0.1f;

    // Penalty for wrong armor type
    uint8 expectedArmor = GetArmorTypeForClass(cls);
    if (item->SubClass != expectedArmor && item->Class == ITEM_CLASS_ARMOR) {
        totalScore *= 0.5f;  // 50% penalty for wrong armor type
    }

    return totalScore;
}
```

---

### Database Query Implementation

```cpp
std::vector<ItemScore> BotGearFactory::QueryItemsForSlot(
    uint32 level,
    EquipmentSlots slot,
    Classes cls,
    uint32 spec,
    uint8 armorType) const
{
    uint32 targetIlvl = GetTargetIlvl(level);
    auto allowedQualities = GetAllowedQualities(level);

    // Convert EquipmentSlots to InventoryType
    uint32 inventoryType = GetInventoryTypeForSlot(slot);

    // Build quality filter
    std::string qualityFilter;
    for (size_t i = 0; i < allowedQualities.size(); ++i) {
        if (i > 0) qualityFilter += " OR ";
        qualityFilter += "Quality = " + std::to_string(allowedQualities[i]);
    }

    // Query database
    QueryResult result = WorldDatabase.Query(
        "SELECT entry FROM item_template "
        "WHERE ItemLevel BETWEEN {} AND {} "
        "AND InventoryType = {} "
        "AND ({}) "  // Quality filter
        "AND RequiredLevel <= {} "
        "AND AllowableClass & {} "  // Class mask
        "AND Flags & {} = 0 "  // Exclude quest items, etc.
        "ORDER BY ItemLevel DESC "
        "LIMIT 100",
        targetIlvl - 10, targetIlvl + 10,
        inventoryType,
        qualityFilter,
        level,
        GetClassMask(cls),
        ITEM_FLAG_QUEST_ITEM | ITEM_FLAG_NO_EQUIP
    );

    if (!result)
        return {};

    // Score all results
    std::vector<ItemScore> scoredItems;
    do {
        uint32 itemEntry = result->Fetch()[0].Get<uint32>();
        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemEntry);

        if (!itemTemplate || !IsValidItemForBot(itemTemplate, bot))
            continue;

        ItemScore scored;
        scored.itemEntry = itemEntry;
        scored.score = CalculateItemScore(itemTemplate, cls, spec);
        scored.ilvl = itemTemplate->ItemLevel;
        scored.quality = itemTemplate->Quality;

        scoredItems.push_back(scored);

    } while (result->NextRow());

    // Sort by score (highest first)
    std::sort(scoredItems.begin(), scoredItems.end(), std::greater<ItemScore>());

    return scoredItems;
}
```

---

### Gear Set Building

```cpp
GearSet BotGearFactory::BuildGearSet(Player* bot, uint32 level) {
    Classes cls = bot->getClass();
    uint32 spec = GetBotSpecialization(bot);
    uint8 armorType = GetArmorTypeForClass(cls);

    GearSet gear;

    // Priority order: most important slots first
    std::vector<EquipmentSlots> slotPriority = {
        EQUIPMENT_SLOT_MAINHAND,      // Weapon is CRITICAL
        EQUIPMENT_SLOT_CHEST,          // Major armor
        EQUIPMENT_SLOT_LEGS,           // Major armor
        EQUIPMENT_SLOT_HEAD,           // Major armor
        EQUIPMENT_SLOT_SHOULDERS,
        EQUIPMENT_SLOT_FEET,
        EQUIPMENT_SLOT_HANDS,
        EQUIPMENT_SLOT_WRIST,
        EQUIPMENT_SLOT_WAIST,
        EQUIPMENT_SLOT_OFFHAND,        // Shield/offhand weapon
        EQUIPMENT_SLOT_BACK,
        EQUIPMENT_SLOT_NECK,
        EQUIPMENT_SLOT_FINGER1,
        EQUIPMENT_SLOT_FINGER2,
        EQUIPMENT_SLOT_TRINKET1,
        EQUIPMENT_SLOT_TRINKET2,
        EQUIPMENT_SLOT_RANGED,         // Relic/ranged
    };

    // Fill each slot
    for (EquipmentSlots slot : slotPriority) {
        auto candidates = QueryItemsForSlot(level, slot, cls, spec, armorType);

        if (!candidates.empty()) {
            // Add randomness: pick from top 3 candidates for realism
            uint32 selectedIdx = std::min((uint32)candidates.size() - 1, urand(0, 2));
            gear.items[slot] = candidates[selectedIdx].itemEntry;
            gear.averageIlvl += candidates[selectedIdx].ilvl;
            gear.totalScore += candidates[selectedIdx].score;
        }
    }

    // Calculate average ilvl
    if (!gear.items.empty())
        gear.averageIlvl /= gear.items.size();

    // Ensure critical slots are filled
    EnsureAllSlotsFilled(gear, bot, level);

    return gear;
}

void BotGearFactory::EnsureAllSlotsFilled(GearSet& gear, Player* bot, uint32 level) {
    // If weapon is missing, BOT CANNOT FUNCTION - use fallback
    if (!gear.HasWeapon()) {
        LOG_ERROR("playerbot", "BotGearFactory: No weapon found for {} level {}, using fallback",
                  GetClassName(bot->getClass()), level);

        // Use basic weapon from starting gear
        uint32 fallbackWeapon = GetFallbackWeapon(bot->getClass());
        gear.items[EQUIPMENT_SLOT_MAINHAND] = fallbackWeapon;
    }

    // For other empty slots, try broader query (ignore ilvl range)
    // ... fallback logic ...
}
```

---

### Main Equip Function

```cpp
bool BotGearFactory::EquipBotForLevel(Player* bot, uint32 level) {
    if (!bot)
        return false;

    LOG_DEBUG("playerbot", "Equipping bot {} (level {})", bot->GetName(), level);

    // 1. Build gear set
    GearSet gear = BuildGearSet(bot, level);

    if (!gear.IsComplete()) {
        LOG_WARN("playerbot", "Incomplete gear set for bot {} (only {} slots filled)",
                 bot->GetName(), gear.items.size());
    }

    // 2. Equip all items
    for (auto const& [slot, itemEntry] : gear.items) {
        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemEntry);
        if (!itemTemplate)
            continue;

        // Create item
        Item* item = Item::CreateItem(itemEntry, 1, bot);
        if (!item)
            continue;

        // Equip to slot
        uint16 dest;
        InventoryResult msg = bot->CanEquipItem(slot, dest, item, false);
        if (msg == EQUIP_ERR_OK) {
            bot->EquipItem(dest, item, true);
            LOG_DEBUG("playerbot", "  Equipped {} to slot {}",
                     itemTemplate->Name1, slot);
        }
        else {
            delete item;
            LOG_WARN("playerbot", "  Failed to equip {} (error {})",
                    itemTemplate->Name1, msg);
        }
    }

    // 3. Update stats
    bot->UpdateAllStats();

    LOG_INFO("playerbot", "Bot {} equipped: avg ilvl {}, {} items, score {:.1f}",
             bot->GetName(), gear.averageIlvl, gear.items.size(), gear.totalScore);

    return true;
}

---

## 4. BotTalentManager

**Purpose**: Applies talent loadouts appropriate for level and spec.

### Talent System Adaptation

```cpp
namespace Playerbot {

struct TalentLoadout {
    uint32 classId;
    uint32 specId;
    uint32 minLevel;
    uint32 maxLevel;
    std::vector<uint32> talents;  // Talent entry IDs in order
    std::vector<uint32> heroTalents; // For level 71+
};

class TC_GAME_API BotTalentManager {
public:
    static BotTalentManager* instance();

    void Initialize();
    void LoadTalentLoadouts();

    // Apply talents to bot based on spec and level
    bool ApplyTalentsForLevel(Player* bot, uint32 level);
    bool ApplySpecialization(Player* bot, uint32 specId);

private:
    TalentLoadout const* GetLoadout(uint32 classId, uint32 specId, uint32 level) const;
    void LearnTalent(Player* bot, uint32 talentEntry);

    // Cache: [classId][specId][level] -> loadout
    std::map<uint32, std::map<uint32, std::map<uint32, TalentLoadout>>> _loadouts;
};

#define sBotTalentManager BotTalentManager::instance()

}
```

### Talent Database Schema

```sql
-- New table: playerbot_talent_loadouts
CREATE TABLE IF NOT EXISTS `playerbot_talent_loadouts` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `class_id` TINYINT UNSIGNED NOT NULL,
  `spec_id` TINYINT UNSIGNED NOT NULL,
  `min_level` TINYINT UNSIGNED NOT NULL,
  `max_level` TINYINT UNSIGNED NOT NULL,
  `talent_string` TEXT NOT NULL COMMENT 'Comma-separated talent entry IDs',
  `hero_talent_string` TEXT COMMENT 'Hero talents for 71+',
  `description` VARCHAR(255),
  PRIMARY KEY (`id`),
  KEY `idx_class_spec_level` (`class_id`, `spec_id`, `min_level`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Example entries
INSERT INTO playerbot_talent_loadouts
(class_id, spec_id, min_level, max_level, talent_string, description)
VALUES
(1, 1, 10, 70, '117,202,301,...', 'Warrior Arms - Leveling Build'),
(1, 1, 71, 80, '117,202,301,...', 'Warrior Arms - Hero Talents (Slayer)'),
(1, 2, 10, 70, '123,215,320,...', 'Warrior Fury - Leveling Build');
```

---

## 5. BotWorldPositioner

**Purpose**: Teleports bots to appropriate zones based on level and faction.

### Zone Database & Cache

```cpp
namespace Playerbot {

struct ZoneInfo {
    uint32 zoneId;
    uint32 areaId;
    uint32 minLevel;
    uint32 maxLevel;
    uint32 mapId;
    TeamId faction;  // 0 = neutral, TEAM_ALLIANCE, TEAM_HORDE
    std::vector<WorldLocation> spawnPoints;
};

class TC_GAME_API BotWorldPositioner {
public:
    static BotWorldPositioner* instance();

    void Initialize();
    void BuildZoneCache();
    void BuildTeleportLocationCache();

    // Teleport bot to appropriate zone
    bool TeleportBotForLevel(Player* bot);
    bool TeleportBotToZone(Player* bot, uint32 zoneId);

    // Zone queries
    std::vector<ZoneInfo> GetZonesForLevel(uint32 level, TeamId faction) const;
    std::vector<WorldLocation> GetSpawnPointsInZone(uint32 zoneId) const;

private:
    WorldLocation SelectRandomSpawnPoint(std::vector<ZoneInfo> const& zones) const;
    void QueryCreatureSpawnPoints();
    void QueryGraveyard Locations();
    void QueryInnLocations();

    // Cache: [level][faction] -> eligible zones
    std::map<uint32, std::map<TeamId, std::vector<ZoneInfo>>> _zoneCache;
};

#define sBotWorldPositioner BotWorldPositioner::instance()

}
```

### Zone Query Strategy

```sql
-- Build zone-to-level mappings from DBC data
-- Use AreaTable.dbc for zone information
-- Cross-reference with creature_template/creature for spawn levels

-- Example: Find zones with level 10-20 creatures
SELECT DISTINCT
    c.zoneId,
    MIN(ct.minlevel) as min_creature_level,
    MAX(ct.maxlevel) as max_creature_level
FROM creature c
JOIN creature_template ct ON c.id = ct.entry
WHERE ct.minlevel >= 10 AND ct.maxlevel <= 20
GROUP BY c.zoneId
HAVING COUNT(*) > 10;  -- Ensure sufficient spawn density
```

### Teleport Location Sources

1. **Creature Spawn Locations**: Use existing creature spawn points as safe teleport locations
2. **Flight Master Locations**: Cities and outposts
3. **Innkeeper Locations**: Rest areas in appropriate zones
4. **Graveyard Locations**: Always safe spawn points

```cpp
void BotWorldPositioner::BuildTeleportLocationCache() {
    // Query creature spawns grouped by zone and level
    QueryResult result = WorldDatabase.Query(
        "SELECT c.map, c.zoneId, c.position_x, c.position_y, c.position_z, "
        "       MIN(ct.minlevel) as minlvl, MAX(ct.maxlevel) as maxlvl "
        "FROM creature c "
        "JOIN creature_template ct ON c.id = ct.entry "
        "WHERE ct.type = 7 " // Humanoids (more likely to be in leveling zones)
        "GROUP BY c.zoneId, FLOOR(c.position_x / 10), FLOOR(c.position_y / 10)"
    );

    if (!result)
        return;

    do {
        Field* fields = result->Fetch();
        uint32 mapId = fields[0].Get<uint32>();
        uint32 zoneId = fields[1].Get<uint32>();
        float x = fields[2].Get<float>();
        float y = fields[3].Get<float>();
        float z = fields[4].Get<float>();
        uint32 minLvl = fields[5].Get<uint32>();
        uint32 maxLvl = fields[6].Get<uint32>();

        // Store in cache
        for (uint32 level = minLvl; level <= maxLvl; ++level) {
            WorldLocation loc(mapId, x, y, z, 0.0f);
            // Add to appropriate faction cache...
        }
    } while (result->NextRow());
}
```

---

## 6. Integration: Complete Bot Creation Flow

### Corrected Workflow (Based on User Requirements)

```
WORKFLOW STEPS:
1. Determine bot level from distribution (brackets: 1-4, 5-9, 10-14, 15-19...)
   ├─ Level 1-4: Create bot at level 1, NO gear, let level naturally
   └─ Level 5+: Instant level-up with full gear setup

2. IF level >= 5:
   └─ Level up bot to target (±15% distribution tolerance)

3. IF level >= 5:
   └─ Choose 1st specialization and distribute talents

4. IF level >= 10 (WoW 12.0 dual-spec unlock):
   ├─ Choose 2nd specialization
   ├─ Activate 2nd spec
   └─ Distribute talent points for 2nd spec

5. IF level >= 5:
   ├─ Add bags to bot inventory
   └─ Bot equips bags

6. IF level >= 5:
   ├─ Equip gear for 1st spec (active)
   ├─ Add gear for 2nd spec to bags (unequipped)
   └─ Switch back to 1st spec
```

---

### Level Distribution Brackets

```cpp
namespace Playerbot {

struct LevelBracket {
    uint32 minLevel;
    uint32 maxLevel;
    float targetPercentage;     // 0.0 to 1.0
    uint32 currentCount;
    TeamId faction;

    bool IsNaturalLeveling() const {
        // Bots in 1-4 bracket level naturally, no instant gear
        return minLevel <= 4;
    }

    bool SupportsDualSpec() const {
        // Dual spec unlocks at level 10 in WoW 12.0
        return minLevel >= 10;
    }
};

// Example bracket configuration
std::vector<LevelBracket> allianceBrackets = {
    {1,  4,  0.05},  // 5% - Natural leveling in starter zones
    {5,  9,  0.10},  // 10% - First geared bracket
    {10, 14, 0.10},  // 10% - Dual spec available
    {15, 19, 0.10},  // 10%
    {20, 24, 0.10},  // 10%
    // ... continues to 80
};

}
```

---

### Full Lifecycle Implementation

```cpp
bool CreateAndSetupBot(uint32 accountId, std::string const& name,
                       Races race, Classes cls, Gender gender)
{
    // STEP 1: Determine target level from distribution
    TeamId faction = GetFactionForRace(race);
    LevelBracket const* bracket = sBotLevelDistribution->SelectBracket(faction);
    uint32 targetLevel = bracket->GetRandomLevel();  // Random within bracket

    LOG_INFO("playerbot", "Creating bot '{}' - target level {} (bracket {}-{})",
             name, targetLevel, bracket->minLevel, bracket->maxLevel);

    // STEP 2: Create character at level 1
    BotCharacterCreator creator;
    ObjectGuid botGuid = creator.CreateCharacter(accountId, name, race, cls, gender);

    Player* bot = ObjectAccessor::FindPlayer(botGuid);
    if (!bot) {
        LOG_ERROR("playerbot", "Failed to create bot character '{}'", name);
        return false;
    }

    // STEP 3: If level 1-4, stop here (natural leveling)
    if (bracket->IsNaturalLeveling()) {
        LOG_INFO("playerbot", "Bot '{}' created at level 1 - will level naturally in starter zone",
                 name);

        // Teleport to starter zone
        sBotWorldPositioner->TeleportToStarterZone(bot);
        bot->SaveToDB(false, false);
        return true;
    }

    // STEP 4: Level up bot (level 5+)
    LOG_DEBUG("playerbot", "Leveling bot '{}' from 1 to {}", name, targetLevel);

    bot->GiveLevel(targetLevel);        // TrinityCore native instant level-up
    bot->InitTalentForLevel();          // Ensure client talent data sent
    bot->SetXP(0);                      // Reset XP

    // STEP 5: Choose and apply 1st specialization
    uint32 spec1 = sBotTalentManager->SelectSpecialization(bot, 1);
    sBotTalentManager->ApplySpecialization(bot, spec1);
    sBotTalentManager->ApplyTalentLoadout(bot, spec1, targetLevel);

    LOG_DEBUG("playerbot", "Bot '{}' assigned spec 1: {}",
              name, GetSpecializationName(cls, spec1));

    // STEP 6: If dual-spec available (level 10+), setup 2nd spec
    uint32 spec2 = 0;
    if (bracket->SupportsDualSpec()) {
        spec2 = sBotTalentManager->SelectSpecialization(bot, 2);

        // Activate 2nd spec temporarily
        sBotTalentManager->ActivateSpecialization(bot, spec2);
        sBotTalentManager->ApplyTalentLoadout(bot, spec2, targetLevel);

        LOG_DEBUG("playerbot", "Bot '{}' assigned spec 2: {}",
                  name, GetSpecializationName(cls, spec2));

        // Switch back to spec 1
        sBotTalentManager->ActivateSpecialization(bot, spec1);
    }

    // STEP 7: Add bags to bot
    std::vector<uint32> bagItems = sBotGearFactory->GetAppropriatebags(targetLevel);
    for (uint32 bagEntry : bagItems) {
        Item* bag = Item::CreateItem(bagEntry, 1, bot);
        if (bag) {
            bot->StoreNewItemInInventorySlot(bag, INVENTORY_SLOT_BAG_START);
            LOG_DEBUG("playerbot", "Bot '{}' given bag: {}", name, bagEntry);
        }
    }

    // STEP 8: Equip bags (bot now has storage)
    bot->AutoStoreLoot();  // Ensures bags are equipped

    // STEP 9: Gear for spec 1 (active spec)
    LOG_DEBUG("playerbot", "Equipping bot '{}' for spec 1 ({})",
              name, GetSpecializationName(cls, spec1));

    sBotGearFactory->EquipBotForLevel(bot, targetLevel, spec1);

    // STEP 10: If dual-spec, add spec 2 gear to bags (UNEQUIPPED)
    if (spec2 > 0) {
        LOG_DEBUG("playerbot", "Adding spec 2 gear to bags for bot '{}'", name);

        GearSet spec2Gear = sBotGearFactory->BuildGearSet(bot, targetLevel, spec2);
        for (auto const& [slot, itemEntry] : spec2Gear.items) {
            Item* item = Item::CreateItem(itemEntry, 1, bot);
            if (item) {
                // Store in bags, NOT equipped
                bot->StoreNewItem(INVENTORY_SLOT_BAG_START, BAG_SLOT_END, itemEntry, 1);
                LOG_DEBUG("playerbot", "  Added {} to bags (spec 2)", itemEntry);
            }
        }
    }

    // STEP 11: Add consumables (food, water, potions)
    sBotGearFactory->AddConsumables(bot, targetLevel);

    // STEP 12: Teleport to appropriate zone for level
    sBotWorldPositioner->TeleportBotForLevel(bot);

    // STEP 13: Save and add to world
    bot->SaveToDB(false, false);
    bot->GetMap()->AddToMap(bot);

    // STEP 14: Log completion
    LOG_INFO("playerbot",
             "Bot '{}' created successfully:\n"
             "  Level: {}\n"
             "  Spec 1: {} (active)\n"
             "  Spec 2: {}\n"
             "  Gear: {} items equipped, {} in bags\n"
             "  Avg ilvl: {}",
             name, targetLevel,
             GetSpecializationName(cls, spec1),
             spec2 > 0 ? GetSpecializationName(cls, spec2) : "None",
             bot->GetEquippedItemsCount(),
             bot->GetBagItemsCount(),
             bot->GetAverageItemLevel());

    return true;
}
```

---

### Distribution Tolerance (±15%)

```cpp
bool BotLevelManager::IsDistributionBalanced() const {
    const float TOLERANCE = 0.15f;  // ±15%

    for (auto const& bracket : sBotLevelDistribution->GetBrackets(faction)) {
        float currentPercentage = bracket.currentCount / (float)totalBots;
        float targetPercentage = bracket.targetPercentage;

        float lowerBound = targetPercentage * (1.0f - TOLERANCE);  // -15%
        float upperBound = targetPercentage * (1.0f + TOLERANCE);  // +15%

        if (currentPercentage < lowerBound || currentPercentage > upperBound) {
            // Example: Target 10%, acceptable range: 8.5% - 11.5%
            LOG_DEBUG("playerbot",
                     "Bracket {}-{}: Current {}%, Target {}%, OUT OF BALANCE",
                     bracket.minLevel, bracket.maxLevel,
                     currentPercentage * 100, targetPercentage * 100);
            return false;
        }
    }

    return true;
}
```

---

### Dual Spec Implementation

```cpp
namespace Playerbot {

class BotTalentManager {
public:
    // Select appropriate spec for bot (considers class, role distribution)
    uint32 SelectSpecialization(Player* bot, uint8 specSlot) const;

    // Activate a specific spec (switches active spec)
    void ActivateSpecialization(Player* bot, uint32 specId);

    // Apply talent points for a spec
    void ApplyTalentLoadout(Player* bot, uint32 specId, uint32 level);
};

uint32 BotTalentManager::SelectSpecialization(Player* bot, uint8 specSlot) const {
    Classes cls = bot->getClass();

    // Get all specs for this class
    std::vector<uint32> availableSpecs = GetSpecsForClass(cls);

    if (specSlot == 1) {
        // Spec 1: Weighted selection (prefer popular specs)
        return SelectWeightedSpec(availableSpecs, cls);
    }
    else {
        // Spec 2: Different from spec 1, consider role diversity
        uint32 spec1 = bot->GetPrimarySpecialization();
        return SelectComplementarySpec(availableSpecs, spec1);
    }
}

void BotTalentManager::ActivateSpecialization(Player* bot, uint32 specId) {
    // Use TrinityCore's native spec switching
    bot->ActivateTalentGroup(specId == 1 ? 0 : 1);  // 0-indexed internally
    bot->SendTalentsInfoData();

    LOG_DEBUG("playerbot", "Bot '{}' switched to spec {}",
              bot->GetName(), specId);
}

}
```

---

## Implementation Phases

### Phase 1: Foundation (Week 1-2)
- [ ] BotLevelDistribution class
  - [ ] Load bracket configuration from playerbots.conf
  - [ ] Validate configuration (percentages sum to 100)
  - [ ] Implement bracket queries
- [ ] BotLevelManager skeleton
  - [ ] Update() ticker for periodic checks
  - [ ] CanAdjustBot() safety checks
  - [ ] FlagBotForLevelChange() infrastructure

### Phase 2: Gear & Talents (Week 2-3)
- [ ] BotGearFactory implementation
  - [ ] Build gear cache from item_template database
  - [ ] Implement stat weight scoring per spec
  - [ ] EquipBotForLevel() main function
- [ ] BotTalentManager implementation
  - [ ] Create playerbot_talent_loadouts table
  - [ ] Load default talent builds for all 13 classes × 3+ specs
  - [ ] ApplyTalentsForLevel() implementation

### Phase 3: World Positioning (Week 3-4)
- [ ] BotWorldPositioner implementation
  - [ ] Query zone data from DBC/database
  - [ ] Build teleport location cache
  - [ ] TeleportBotForLevel() implementation
- [ ] Integration with BotSpawner
  - [ ] Modify spawning flow to use new level assignment
  - [ ] Ensure instant readiness (no gradual leveling)

### Phase 4: Rebalancing System (Week 4-5)
- [ ] Implement CheckAndRebalance() algorithm
- [ ] Implement ProcessFlaggedBots() cycle
- [ ] Add exclusion logic (guilds, friend lists)
- [ ] Dynamic distribution based on real players

### Phase 5: Testing & Tuning (Week 5-6)
- [ ] Create test scenarios
  - [ ] Spawn 1000 bots, verify distribution
  - [ ] Test rebalancing with overpopulation
  - [ ] Verify exclusions work
- [ ] Performance testing
  - [ ] Measure rebalance algorithm overhead
  - [ ] Optimize database queries
- [ ] Configuration tuning
  - [ ] Adjust default brackets for WoW 12.0 content
  - [ ] Balance percentages for leveling vs endgame

---

## Database Requirements

### New Tables

```sql
-- Talent loadouts
CREATE TABLE IF NOT EXISTS `playerbot_talent_loadouts` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `class_id` TINYINT UNSIGNED NOT NULL,
  `spec_id` TINYINT UNSIGNED NOT NULL,
  `min_level` TINYINT UNSIGNED NOT NULL,
  `max_level` TINYINT UNSIGNED NOT NULL,
  `talent_string` TEXT NOT NULL,
  `hero_talent_string` TEXT,
  `description` VARCHAR(255),
  PRIMARY KEY (`id`),
  KEY `idx_class_spec_level` (`class_id`, `spec_id`, `min_level`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Zone level mappings (optional - can be built from creature data)
CREATE TABLE IF NOT EXISTS `playerbot_zone_levels` (
  `zone_id` INT UNSIGNED NOT NULL,
  `area_id` INT UNSIGNED,
  `min_level` TINYINT UNSIGNED NOT NULL,
  `max_level` TINYINT UNSIGNED NOT NULL,
  `faction` TINYINT UNSIGNED COMMENT '0=neutral, 1=Alliance, 2=Horde',
  PRIMARY KEY (`zone_id`),
  KEY `idx_level` (`min_level`, `max_level`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Flagged bots (for rebalancing - can be in-memory instead)
CREATE TABLE IF NOT EXISTS `playerbot_flagged_for_rebalance` (
  `guid` BIGINT UNSIGNED NOT NULL,
  `target_level` TINYINT UNSIGNED NOT NULL,
  `flagged_time` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  `reason` VARCHAR(255),
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Guild tracking for exclusions
CREATE TABLE IF NOT EXISTS `playerbot_guild_real_players` (
  `guild_id` INT UNSIGNED NOT NULL,
  `has_real_players` TINYINT(1) NOT NULL DEFAULT 0,
  `last_updated` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`guild_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

---

## Configuration Examples

### Example 1: Even Distribution (Default)
```conf
# Spread bots evenly across all levels 1-80
Playerbot.Population.NumBrackets = 8
Playerbot.Population.Alliance.Range1.Min = 1
Playerbot.Population.Alliance.Range1.Max = 10
Playerbot.Population.Alliance.Range1.Pct = 12.5

# ... (all 8 brackets = 12.5% each)
```

### Example 2: Endgame Focus
```conf
# Concentrate bots at max level for dungeon/raid queues
Playerbot.Population.NumBrackets = 5
Playerbot.Population.Alliance.Range1.Min = 1
Playerbot.Population.Alliance.Range1.Max = 30
Playerbot.Population.Alliance.Range1.Pct = 10  # 10% low level

Playerbot.Population.Alliance.Range2.Min = 31
Playerbot.Population.Alliance.Range2.Max = 60
Playerbot.Population.Alliance.Range2.Pct = 15  # 15% mid level

Playerbot.Population.Alliance.Range3.Min = 61
Playerbot.Population.Alliance.Range3.Max = 70
Playerbot.Population.Alliance.Range3.Pct = 15  # 15% pre-max

Playerbot.Population.Alliance.Range4.Min = 71
Playerbot.Population.Alliance.Range4.Max = 79
Playerbot.Population.Alliance.Range4.Pct = 20  # 20% leveling endgame

Playerbot.Population.Alliance.Range5.Min = 80
Playerbot.Population.Alliance.Range5.Max = 80
Playerbot.Population.Alliance.Range5.Pct = 40  # 40% at max level!
```

### Example 3: Dynamic Distribution
```conf
# Bots follow real players
Playerbot.Population.DynamicDistribution = 1
Playerbot.Population.RealPlayerWeight = 2.0  # Strong player influence
Playerbot.Population.SyncFactions = 1        # Alliance & Horde share brackets
```

---

## Performance Considerations

### Memory Impact
- Gear cache: ~50MB (all items × all slots × all levels × all specs)
- Zone cache: ~10MB (all zones × spawn points)
- Talent cache: ~5MB (all loadouts)
- **Total**: ~65MB additional memory

### CPU Impact
- Rebalance check: O(n) where n = bot count, runs every 5 minutes
- Flagged bot processing: O(5) bots per cycle, every 15 seconds
- Gear scoring: O(k) where k = items per slot (~100), once per bot creation
- **Total**: <0.5% CPU overhead with 5000 bots

### Database Impact
- Initial cache build: ~50 queries (one-time on startup)
- Per-bot creation: ~10 queries (gear, talents, position)
- Rebalancing: 1-2 queries per flagged bot
- **Total**: Well within acceptable range for bot creation cadence

---

## Success Metrics

1. **Distribution Accuracy**: Actual bot percentages within ±2% of configured targets
2. **Readiness Time**: Bots fully geared/talented within 5 seconds of spawning
3. **Rebalance Responsiveness**: System adapts to player activity within 10 minutes
4. **Exclusion Reliability**: 100% of guild/friend-listed bots protected from adjustment
5. **Performance**: <1% CPU overhead, <100MB memory increase

---

## Future Enhancements

### Phase 2 Features (Post-Initial Release)
- [ ] Profession assignments based on level
- [ ] Reputation pre-grinding for appropriate factions
- [ ] Quest completion history for immersion
- [ ] Mount collections based on level/faction
- [ ] Pet collections for hunter/warlock bots

### Phase 3 Features (Advanced)
- [ ] Delve-ready bot groups
- [ ] Mythic+ dungeon queuing
- [ ] Raid-ready bot formations
- [ ] PvP bracket distribution
- [ ] Honor level progression

---

## Conclusion

This design adapts the proven concepts from mod-playerbots (WotLK 3.3.5) and mod-player-bot-level-brackets to the modern WoW 12.0 architecture, while respecting TrinityCore's patterns and our module-first approach.

**Key Innovations**:
1. **TrinityCore Native Integration**: Uses `Player::GiveLevel()` - the PROPER TrinityCore leveling system
   - Single function call handles instant multi-level jumps (1 → 80)
   - Automatically learns ALL skills and spells up to target level
   - Triggers all hooks, achievements, and mail rewards
   - No shortcuts or hacks - 100% TrinityCore-compliant
2. **Module-Only Implementation**: No core modifications, all in `src/modules/Playerbot/`
3. **Database-Driven Flexibility**: Gear, talents, and zones configurable via database
4. **Performance-Conscious**: Caching and periodic updates minimize overhead
5. **Dynamic & Adaptive**: Responds to real player activity automatically
6. **WoW 12.0 Native**: Hero talents, modern ilvl system, new zones

**Next Steps**:
1. Review and approve this design
2. Create detailed task breakdown for Phase 1
3. Begin implementation with BotLevelDistribution and configuration
4. Iterate through phases with testing at each milestone

---

**Author**: Claude Code
**Date**: 2025-10-18
**Version**: 1.0
**Status**: Pending Review & Approval
