# Automated World Population System - Configuration Reference

## Overview

This comprehensive guide documents all 52 configuration settings for the Automated World Population System. Each setting is explained with its purpose, valid values, default value, performance impact, and usage examples.

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Global Settings](#global-settings)
3. [Level Distribution Settings](#level-distribution-settings)
4. [Gear Factory Settings](#gear-factory-settings)
5. [Talent Manager Settings](#talent-manager-settings)
6. [World Positioner Settings](#world-positioner-settings)
7. [Level Manager Settings](#level-manager-settings)
8. [Configuration Templates](#configuration-templates)
9. [Validation & Testing](#validation--testing)

---

## Quick Start

### Minimal Configuration

For basic operation, only these 3 settings are required:

```conf
# Enable the system
Playerbot.LevelManager.Enabled = 1

# Process 10 bots per server tick (default)
Playerbot.LevelManager.MaxBotsPerUpdate = 10

# Enable all subsystems
Playerbot.GearFactory.EnableCache = 1
```

All other settings use sensible defaults.

### Recommended Production Configuration

```conf
# System Control
Playerbot.LevelManager.Enabled = 1
Playerbot.LevelManager.MaxBotsPerUpdate = 20
Playerbot.LevelManager.ThreadPoolWorkers = 4

# Distribution Tolerance
Playerbot.LevelManager.DistributionTolerance = 15

# Caching
Playerbot.GearFactory.EnableCache = 1
Playerbot.TalentManager.CacheLoadouts = 1

# Logging
Playerbot.LevelManager.VerboseLogging = 0
Playerbot.GearFactory.LogStats = 0
```

---

## Global Settings

### System Control

#### `Playerbot.LevelManager.Enabled`

**Purpose:** Master switch for the entire Automated World Population System.

**Type:** Boolean (0 or 1)
**Default:** `0` (disabled)
**Valid Values:**
- `0` = System disabled
- `1` = System enabled

**Performance Impact:** None when disabled

**Usage:**
```conf
# Production: Enable system
Playerbot.LevelManager.Enabled = 1

# Maintenance: Temporarily disable
Playerbot.LevelManager.Enabled = 0
```

**Effects When Disabled:**
- No bots will be processed
- Existing bots remain unchanged
- Queue processing stops
- Caches remain loaded (no unload)

**Recommendations:**
- Enable during normal operation
- Disable during maintenance windows
- Disable if experiencing performance issues

---

#### `Playerbot.LevelManager.MaxBotsPerUpdate`

**Purpose:** Maximum number of bots processed per World Update tick.

**Type:** Unsigned Integer
**Default:** `10`
**Valid Range:** `1` to `100`
**Recommended:** `10` to `20` for production

**Performance Impact:** **HIGH**
- Higher values = faster bot creation
- Higher values = more CPU per tick
- May cause lag spikes if set too high

**Usage:**
```conf
# Conservative (low-pop servers, 50-500 players)
Playerbot.LevelManager.MaxBotsPerUpdate = 5

# Balanced (medium-pop servers, 500-2000 players)
Playerbot.LevelManager.MaxBotsPerUpdate = 10

# Aggressive (high-pop servers, 2000+ players, powerful hardware)
Playerbot.LevelManager.MaxBotsPerUpdate = 20

# Maximum (dedicated bot servers, testing only)
Playerbot.LevelManager.MaxBotsPerUpdate = 50
```

**Calculation Guide:**
```
Queue Drain Time = (Total Bots in Queue) / (MaxBotsPerUpdate × TPS)

Example:
- Queue Size: 1,000 bots
- MaxBotsPerUpdate: 10
- Server TPS: 50

Drain Time = 1,000 / (10 × 50) = 2 seconds
```

**Recommendations:**
- Start with `10` and increase gradually
- Monitor server TPS (should stay above 48)
- If TPS drops below 45, reduce this value
- Use `5` during peak hours, `20` during off-peak

---

#### `Playerbot.LevelManager.ThreadPoolWorkers`

**Purpose:** Number of worker threads for asynchronous bot preparation.

**Type:** Unsigned Integer
**Default:** `4`
**Valid Range:** `1` to `16`
**Recommended:** `2` to `8`

**Performance Impact:** **MEDIUM**
- More threads = faster preparation (Phase 1)
- More threads = higher CPU usage
- Diminishing returns beyond CPU core count

**Usage:**
```conf
# Low-end hardware (2-4 cores)
Playerbot.LevelManager.ThreadPoolWorkers = 2

# Mid-range hardware (4-8 cores)
Playerbot.LevelManager.ThreadPoolWorkers = 4

# High-end hardware (8+ cores)
Playerbot.LevelManager.ThreadPoolWorkers = 8
```

**Calculation Guide:**
```
Recommended = min(CPU_Cores - 2, 8)

Examples:
- 4 CPU cores: 2 workers (4 - 2 = 2)
- 8 CPU cores: 6 workers (8 - 2 = 6)
- 16 CPU cores: 8 workers (capped at 8)
```

**Recommendations:**
- Leave 2 cores free for main thread + OS
- Monitor CPU usage per worker thread
- Optimal range: 50-70% total CPU utilization

---

#### `Playerbot.LevelManager.DistributionTolerance`

**Purpose:** Percentage deviation allowed from target bracket distribution.

**Type:** Unsigned Integer (percentage)
**Default:** `15` (±15%)
**Valid Range:** `5` to `30`
**Recommended:** `10` to `20`

**Performance Impact:** None

**Usage:**
```conf
# Strict distribution (slower to reach balance)
Playerbot.LevelManager.DistributionTolerance = 10

# Balanced (recommended)
Playerbot.LevelManager.DistributionTolerance = 15

# Loose distribution (faster creation, less balanced)
Playerbot.LevelManager.DistributionTolerance = 25
```

**Examples:**
```
Target: 500 bots in L1-4 bracket (10% of 5,000 total)

Tolerance = 10%:
- Minimum: 450 bots (500 × 0.90)
- Maximum: 550 bots (500 × 1.10)

Tolerance = 15%:
- Minimum: 425 bots (500 × 0.85)
- Maximum: 575 bots (500 × 1.15)

Tolerance = 30%:
- Minimum: 350 bots (500 × 0.70)
- Maximum: 650 bots (500 × 1.30)
```

**Recommendations:**
- Use `15%` for most scenarios
- Use `10%` if exact distribution critical
- Use `20-25%` for faster bot creation

---

#### `Playerbot.LevelManager.VerboseLogging`

**Purpose:** Enable detailed debug logging for troubleshooting.

**Type:** Boolean (0 or 1)
**Default:** `0` (disabled)
**Valid Values:**
- `0` = Normal logging (errors and warnings only)
- `1` = Verbose logging (all operations)

**Performance Impact:** **LOW** (minor I/O overhead)

**Usage:**
```conf
# Production: Disable verbose logging
Playerbot.LevelManager.VerboseLogging = 0

# Development/Debugging: Enable verbose logging
Playerbot.LevelManager.VerboseLogging = 1
```

**Log Output Examples:**

**Normal Logging (0):**
```
INFO: BotLevelManager initialized successfully
WARN: Task timeout detected (taskId: 12345)
ERROR: Failed to apply bot GUID 0x...
```

**Verbose Logging (1):**
```
DEBUG: Task 12345 submitted for bot "Testbot"
DEBUG: Phase 1: Selecting level bracket (faction: Alliance)
DEBUG: Selected bracket L20-24 (current: 87/100, target: 6%)
DEBUG: Phase 1: Generating gear set (class: Warrior, spec: Protection, level: 22)
DEBUG: Generated 14-slot gear set (avgIlvl: 24.3, score: 342.7)
DEBUG: Phase 1: Selecting zone (level: 22, faction: Alliance)
DEBUG: Selected zone: Redridge Mountains (L15-25)
DEBUG: Phase 1 complete: Queued for main thread (elapsed: 2.3ms)
DEBUG: Phase 2: Applying level 22 to bot
DEBUG: Phase 2: Applying talents (spec: Protection)
DEBUG: Phase 2: Equipping gear (14 items)
DEBUG: Phase 2: Teleporting to zone (Redridge: -9447.83, 64.17, 56.48)
DEBUG: Task 12345 completed successfully (total time: 48.7ms)
```

**Recommendations:**
- Keep disabled in production (reduces log spam)
- Enable temporarily for troubleshooting
- Enable during initial deployment for validation

---

## Level Distribution Settings

### Overview

34 settings define the percentage distribution of bots across 17 level brackets per faction.

**Constraints:**
- Must have exactly 17 brackets per faction
- All percentages must sum to 100.0% (±0.1%)
- Each percentage must be between 1.0% and 20.0%

### Configuration Format

```conf
Playerbot.LevelDistribution.<Faction>.Bracket.<LevelRange> = <Percentage>
```

**Parameters:**
- `<Faction>`: `Alliance` or `Horde`
- `<LevelRange>`: `L1`, `L5`, `L10`, ..., `L80` (17 total)
- `<Percentage>`: Float value (1.0 to 20.0)

---

### Alliance Distribution Settings

#### Default Balanced Distribution

```conf
# Starter Zone (L1-4)
Playerbot.LevelDistribution.Alliance.Bracket.L1 = 3.0

# Early Leveling (L5-19)
Playerbot.LevelDistribution.Alliance.Bracket.L5 = 5.0
Playerbot.LevelDistribution.Alliance.Bracket.L10 = 6.0
Playerbot.LevelDistribution.Alliance.Bracket.L15 = 6.5

# Mid Leveling (L20-39)
Playerbot.LevelDistribution.Alliance.Bracket.L20 = 7.0
Playerbot.LevelDistribution.Alliance.Bracket.L25 = 7.5
Playerbot.LevelDistribution.Alliance.Bracket.L30 = 8.0
Playerbot.LevelDistribution.Alliance.Bracket.L35 = 8.0

# Late Leveling (L40-59)
Playerbot.LevelDistribution.Alliance.Bracket.L40 = 7.0
Playerbot.LevelDistribution.Alliance.Bracket.L45 = 6.5
Playerbot.LevelDistribution.Alliance.Bracket.L50 = 6.0
Playerbot.LevelDistribution.Alliance.Bracket.L55 = 5.5

# Pre-Endgame (L60-69)
Playerbot.LevelDistribution.Alliance.Bracket.L60 = 7.0
Playerbot.LevelDistribution.Alliance.Bracket.L65 = 6.0

# The War Within (L70-80)
Playerbot.LevelDistribution.Alliance.Bracket.L70 = 6.0
Playerbot.LevelDistribution.Alliance.Bracket.L75 = 7.0
Playerbot.LevelDistribution.Alliance.Bracket.L80 = 8.0

# Total: 100.0%
```

**Rationale:**
- **L1-4 (3%)**: Small starter population, most level naturally
- **L5-19 (17.5%)**: Active leveling, gradual increase
- **L20-39 (30.5%)**: Peak leveling engagement
- **L40-59 (25%)**: Late leveling, declining interest
- **L60-69 (13%)**: Pre-expansion content
- **L70-80 (21%)**: Current expansion content (The War Within)

---

### Horde Distribution Settings

#### Default Balanced Distribution

```conf
# Starter Zone (L1-4)
Playerbot.LevelDistribution.Horde.Bracket.L1 = 3.0

# Early Leveling (L5-19)
Playerbot.LevelDistribution.Horde.Bracket.L5 = 5.0
Playerbot.LevelDistribution.Horde.Bracket.L10 = 6.0
Playerbot.LevelDistribution.Horde.Bracket.L15 = 6.5

# Mid Leveling (L20-39)
Playerbot.LevelDistribution.Horde.Bracket.L20 = 7.0
Playerbot.LevelDistribution.Horde.Bracket.L25 = 7.5
Playerbot.LevelDistribution.Horde.Bracket.L30 = 8.0
Playerbot.LevelDistribution.Horde.Bracket.L35 = 8.0

# Late Leveling (L40-59)
Playerbot.LevelDistribution.Horde.Bracket.L40 = 7.0
Playerbot.LevelDistribution.Horde.Bracket.L45 = 6.5
Playerbot.LevelDistribution.Horde.Bracket.L50 = 6.0
Playerbot.LevelDistribution.Horde.Bracket.L55 = 5.5

# Pre-Endgame (L60-69)
Playerbot.LevelDistribution.Horde.Bracket.L60 = 7.0
Playerbot.LevelDistribution.Horde.Bracket.L65 = 6.0

# The War Within (L70-80)
Playerbot.LevelDistribution.Horde.Bracket.L70 = 6.0
Playerbot.LevelDistribution.Horde.Bracket.L75 = 7.0
Playerbot.LevelDistribution.Horde.Bracket.L80 = 8.0

# Total: 100.0%
```

**Note:** Default distribution is identical for both factions. Customize based on server faction balance preferences.

---

### Alternative Distribution Profiles

#### Profile 1: Endgame-Heavy (Raiding Server)

```conf
# Alliance/Horde (same distribution)
Bracket.L1 = 2.0      # Minimal low-level
Bracket.L5 = 3.0
Bracket.L10 = 4.0
Bracket.L15 = 4.0
Bracket.L20 = 5.0
Bracket.L25 = 5.0
Bracket.L30 = 5.0
Bracket.L35 = 5.0
Bracket.L40 = 5.0
Bracket.L45 = 5.0
Bracket.L50 = 5.0
Bracket.L55 = 5.0
Bracket.L60 = 7.0
Bracket.L65 = 7.0
Bracket.L70 = 10.0    # Heavy endgame
Bracket.L75 = 13.0
Bracket.L80 = 15.0    # Maximum L80 population
# Total: 100.0%
```

**Use Case:** Raiding-focused servers, M+ dungeon groups, endgame content focus

---

#### Profile 2: Leveling-Heavy (New Player Server)

```conf
# Alliance/Horde (same distribution)
Bracket.L1 = 5.0      # Active starter zones
Bracket.L5 = 8.0
Bracket.L10 = 9.0
Bracket.L15 = 10.0
Bracket.L20 = 10.0
Bracket.L25 = 9.0
Bracket.L30 = 8.0
Bracket.L35 = 7.0
Bracket.L40 = 6.0
Bracket.L45 = 5.0
Bracket.L50 = 4.0
Bracket.L55 = 4.0
Bracket.L60 = 4.0
Bracket.L65 = 4.0
Bracket.L70 = 3.0
Bracket.L75 = 2.0
Bracket.L80 = 2.0     # Minimal L80
# Total: 100.0%
```

**Use Case:** Fresh servers, leveling guilds, low-level dungeon runs

---

#### Profile 3: Even Distribution (Testing/Development)

```conf
# Alliance/Horde (same distribution)
# All brackets equal
Bracket.L1 = 5.88     # 100% ÷ 17 ≈ 5.88%
Bracket.L5 = 5.88
Bracket.L10 = 5.88
Bracket.L15 = 5.88
Bracket.L20 = 5.88
Bracket.L25 = 5.88
Bracket.L30 = 5.88
Bracket.L35 = 5.88
Bracket.L40 = 5.88
Bracket.L45 = 5.88
Bracket.L50 = 5.88
Bracket.L55 = 5.88
Bracket.L60 = 5.88
Bracket.L65 = 5.88
Bracket.L70 = 5.88
Bracket.L75 = 5.88
Bracket.L80 = 5.96    # Remainder
# Total: 100.0%
```

**Use Case:** Testing, development, stress testing all level ranges

---

### Validation Script

```bash
#!/bin/bash
# validate_distribution.sh

FACTION=$1  # "Alliance" or "Horde"

SUM=$(grep "Playerbot.LevelDistribution.${FACTION}.Bracket" playerbots.conf | \
      awk -F'=' '{s += $2} END {print s}')

if (( $(echo "$SUM >= 99.9 && $SUM <= 100.1" | bc -l) )); then
    echo "✅ ${FACTION} distribution valid: ${SUM}%"
else
    echo "❌ ${FACTION} distribution INVALID: ${SUM}% (must be 100.0%)"
    exit 1
fi
```

**Usage:**
```bash
./validate_distribution.sh Alliance
./validate_distribution.sh Horde
```

---

## Gear Factory Settings

### `Playerbot.GearFactory.EnableCache`

**Purpose:** Enable immutable gear cache for lock-free item queries.

**Type:** Boolean (0 or 1)
**Default:** `1` (enabled)
**Valid Values:**
- `0` = Cache disabled (queries database every time - SLOW!)
- `1` = Cache enabled (recommended)

**Performance Impact:** **CRITICAL**
- Disabled: ~500ms per bot (database query overhead)
- Enabled: ~2-3ms per bot (cache lookup)

**Usage:**
```conf
# Production: Always enable
Playerbot.GearFactory.EnableCache = 1

# Testing: Disable to test database queries
Playerbot.GearFactory.EnableCache = 0
```

**Cache Statistics:**
```
Memory Usage: ~8-12 MB (45,000 items × 200 bytes/item)
Build Time: 5-10 seconds at startup
Speedup: 166x faster (500ms → 3ms)
```

**Recommendations:**
- **Always enable in production**
- Disable only for testing/debugging
- Rebuild cache after item_template changes

---

### `Playerbot.GearFactory.MinItemLevel`

**Purpose:** Minimum item level for gear cache eligibility.

**Type:** Unsigned Integer
**Default:** `5`
**Valid Range:** `1` to `100`
**Recommended:** `5` to `10`

**Performance Impact:** None

**Usage:**
```conf
# Include low-level starter items (L1-4 gear)
Playerbot.GearFactory.MinItemLevel = 1

# Exclude very low-level items (default)
Playerbot.GearFactory.MinItemLevel = 5

# Only include leveling+ items
Playerbot.GearFactory.MinItemLevel = 10
```

**Effects:**
```
MinItemLevel = 1:  ~52,000 items cached (includes L1-4 gray items)
MinItemLevel = 5:  ~45,000 items cached (default)
MinItemLevel = 10: ~38,000 items cached (excludes early gear)
```

**Recommendations:**
- Use `5` for balanced caching
- Use `1` if L1-4 bots need better gear
- Use `10` to reduce memory footprint

---

### `Playerbot.GearFactory.MinQuality`

**Purpose:** Minimum item quality for gear cache eligibility.

**Type:** Unsigned Integer (Quality Enum)
**Default:** `2` (Uncommon/Green)
**Valid Values:**
- `0` = Poor (Gray) - Not recommended
- `1` = Common (White) - Not recommended
- `2` = Uncommon (Green) - **Recommended**
- `3` = Rare (Blue)
- `4` = Epic (Purple)

**Performance Impact:** Low

**Usage:**
```conf
# Include green items (recommended)
Playerbot.GearFactory.MinQuality = 2

# Only blue+ items (endgame focus)
Playerbot.GearFactory.MinQuality = 3

# Only epic items (unrealistic, not recommended)
Playerbot.GearFactory.MinQuality = 4
```

**Effects:**
```
MinQuality = 1:  ~65,000 items cached (includes white/gray)
MinQuality = 2:  ~45,000 items cached (default, green+)
MinQuality = 3:  ~18,000 items cached (blue+ only)
MinQuality = 4:  ~3,500 items cached (epic only)
```

**Quality Distribution:**

Leveling bots (L1-59):
- 50% Green (Quality 2)
- 50% Blue (Quality 3)
- 0% Purple (Quality 4)

Pre-Endgame bots (L60-69):
- 30% Green (Quality 2)
- 70% Blue (Quality 3)
- 0% Purple (Quality 4)

Endgame bots (L70-80):
- 0% Green (Quality 2)
- 60% Blue (Quality 3)
- 40% Purple (Quality 4)

**Recommendations:**
- **Always use 2 (Green+) in production**
- Do not use 0 or 1 (gray/white items are useless)
- Use 3 only for specialized endgame servers

---

### `Playerbot.GearFactory.LogStats`

**Purpose:** Enable detailed gear generation logging.

**Type:** Boolean (0 or 1)
**Default:** `0` (disabled)
**Valid Values:**
- `0` = No gear stats logged
- `1` = Log gear statistics (per-bot)

**Performance Impact:** **LOW** (minor I/O overhead)

**Usage:**
```conf
# Production: Disable
Playerbot.GearFactory.LogStats = 0

# Development: Enable for verification
Playerbot.GearFactory.LogStats = 1
```

**Log Output Example (when enabled):**
```
INFO: [Bot: Testbot] Gear Set Generated:
  Class: Warrior (1)
  Spec: Protection (0)
  Level: 22
  Gear Score: 342.7
  Average iLvl: 24.3
  Slots Filled: 14/14
  Quality Distribution:
    - Green (2): 7 items (50%)
    - Blue (3): 7 items (50%)
  Items:
    - [HEAD] Item 12345: Green Helmet (iLvl 23, score 28.4)
    - [CHEST] Item 12346: Blue Chestplate (iLvl 25, score 35.7)
    - ...
```

**Recommendations:**
- Disable in production (reduces log size)
- Enable during initial deployment for validation
- Enable temporarily for debugging gear issues

---

### `Playerbot.GearFactory.UseStatWeighting`

**Purpose:** Enable stat-based item scoring for spec-appropriate gear.

**Type:** Boolean (0 or 1)
**Default:** `1` (enabled)
**Valid Values:**
- `0` = Random item selection (no stat scoring)
- `1` = Stat-weighted item selection (recommended)

**Performance Impact:** **NEGLIGIBLE** (scoring pre-computed at cache build)

**Usage:**
```conf
# Production: Enable for optimal gear
Playerbot.GearFactory.UseStatWeighting = 1

# Testing: Disable for random gear
Playerbot.GearFactory.UseStatWeighting = 0
```

**Stat Weights by Role:**

**Tank (Protection Warrior, Guardian Druid, etc.):**
```
Stamina: 1.5
Armor: 1.0
Strength/Agility: 1.2
Dodge/Parry: 0.8
Intellect: 0.0 (ignored)
```

**Melee DPS (Arms Warrior, Rogue, etc.):**
```
Primary Stat (Str/Agi): 1.5
Critical Strike: 1.2
Haste: 1.0
Mastery: 0.8
Stamina: 0.5
Intellect: 0.0 (ignored)
```

**Caster DPS (Mage, Warlock, etc.):**
```
Intellect: 1.5
Critical Strike: 1.2
Haste: 1.0
Mastery: 0.8
Stamina: 0.5
Strength/Agility: 0.0 (ignored)
```

**Healer (Holy Priest, Restoration Druid, etc.):**
```
Intellect: 1.5
Mastery: 1.2
Haste: 1.0
Critical Strike: 0.8
Stamina: 0.5
Strength/Agility: 0.0 (ignored)
```

**Recommendations:**
- **Always enable in production** for realistic gear
- Disable only for testing random distribution

---

## Talent Manager Settings

### `Playerbot.TalentManager.CacheLoadouts`

**Purpose:** Enable in-memory caching of talent loadouts from database.

**Type:** Boolean (0 or 1)
**Default:** `1` (enabled)
**Valid Values:**
- `0` = Query database every time (SLOW)
- `1` = Cache loadouts at startup (recommended)

**Performance Impact:** **HIGH**
- Disabled: ~50ms per bot (database query)
- Enabled: ~0.05ms per bot (cache lookup)

**Usage:**
```conf
# Production: Always enable
Playerbot.TalentManager.CacheLoadouts = 1

# Testing: Disable to test database queries
Playerbot.TalentManager.CacheLoadouts = 0
```

**Cache Statistics:**
```
Memory Usage: ~100-200 KB (127 loadouts × 1-2 KB/loadout)
Build Time: <1 second at startup
Speedup: 1000x faster (50ms → 0.05ms)
```

**Recommendations:**
- **Always enable in production**
- Reload cache after modifying `playerbot_talent_loadouts` table

---

### `Playerbot.TalentManager.EnableDualSpec`

**Purpose:** Enable dual-specialization setup for bots level 10+.

**Type:** Boolean (0 or 1)
**Default:** `1` (enabled)
**Valid Values:**
- `0` = Single spec only
- `1` = Dual spec for L10+ bots (WoW 12.0 behavior)

**Performance Impact:** **LOW** (~20% more talent API calls)

**Usage:**
```conf
# Production: Enable for realistic dual-spec
Playerbot.TalentManager.EnableDualSpec = 1

# Single-spec only (simpler setup)
Playerbot.TalentManager.EnableDualSpec = 0
```

**Dual-Spec Behavior:**
```
Level 1-9:   Single spec only (can't learn second spec)
Level 10-80: Dual spec enabled (primary + secondary)

Example: Warrior L25
  Primary Spec:   Protection (tanking)
  Secondary Spec: Arms (leveling DPS)

Bot logs in with Primary Spec active
```

**Recommendations:**
- Enable for authentic WoW 12.0 experience
- Disable to simplify bot creation (~10% faster)

---

### `Playerbot.TalentManager.LogApplications`

**Purpose:** Enable detailed talent application logging.

**Type:** Boolean (0 or 1)
**Default:** `0` (disabled)
**Valid Values:**
- `0` = No talent logging
- `1` = Log each talent learned

**Performance Impact:** **LOW** (minor I/O overhead)

**Usage:**
```conf
# Production: Disable
Playerbot.TalentManager.LogApplications = 0

# Development: Enable for verification
Playerbot.TalentManager.LogApplications = 1
```

**Log Output Example (when enabled):**
```
INFO: [Bot: Testbot] Applying Talents:
  Class: Priest (5)
  Primary Spec: Shadow (2)
  Secondary Spec: Holy (1)
  Level: 45
  Dual Spec: Enabled

Primary Spec Talents (Shadow):
  - Learned: Mind Blast (SpellID 12345)
  - Learned: Shadow Word: Pain (SpellID 23456)
  - Learned: Vampiric Touch (SpellID 34567)
  ... (18 total talents)

Secondary Spec Talents (Holy):
  - Learned: Flash Heal (SpellID 45678)
  - Learned: Renew (SpellID 56789)
  ... (18 total talents)

✅ Talent application complete: 36 talents learned
```

**Recommendations:**
- Disable in production (reduces log size)
- Enable for debugging talent issues

---

## World Positioner Settings

### `Playerbot.WorldPositioner.UseStarterZones`

**Purpose:** Enable specialized starter zone selection for L1-4 bots.

**Type:** Boolean (0 or 1)
**Default:** `1` (enabled)
**Valid Values:**
- `0` = Use general zones for all levels
- `1` = Use starter zones for L1-4 (recommended)

**Performance Impact:** None

**Usage:**
```conf
# Production: Enable for proper starter placement
Playerbot.WorldPositioner.UseStarterZones = 1

# Testing: Disable to test all zones
Playerbot.WorldPositioner.UseStarterZones = 0
```

**Starter Zones:**

**Alliance:**
- Elwynn Forest (Humans) - L1-10
- Dun Morogh (Dwarves/Gnomes) - L1-10
- Teldrassil (Night Elves) - L1-10
- Azuremyst Isle (Draenei) - L1-10
- Gilneas (Worgen) - L1-10

**Horde:**
- Durotar (Orcs/Trolls) - L1-10
- Mulgore (Tauren) - L1-10
- Tirisfal Glades (Undead) - L1-10
- Eversong Woods (Blood Elves) - L1-10
- Kezan (Goblins) - L1-10

**Recommendations:**
- **Always enable** for proper racial starter zones
- Disable only for testing zone fallback logic

---

### `Playerbot.WorldPositioner.UseCapitalFallback`

**Purpose:** Teleport bots to faction capital if no suitable zone found.

**Type:** Boolean (0 or 1)
**Default:** `1` (enabled)
**Valid Values:**
- `0` = Leave bot at current location (not recommended)
- `1` = Teleport to capital as fallback

**Performance Impact:** None

**Usage:**
```conf
# Production: Enable for safe fallback
Playerbot.WorldPositioner.UseCapitalFallback = 1

# Testing: Disable to detect missing zones
Playerbot.WorldPositioner.UseCapitalFallback = 0
```

**Fallback Capitals:**
- **Alliance**: Stormwind City (-8833.38, 628.62, 94.00)
- **Horde**: Orgrimmar (1676.21, -4315.29, 61.53)

**Recommendations:**
- **Always enable** to prevent bots stuck in invalid locations
- Disable temporarily to identify missing zone definitions

---

### `Playerbot.WorldPositioner.RandomizeCoordinates`

**Purpose:** Add random offset to placement coordinates to avoid bot stacking.

**Type:** Boolean (0 or 1)
**Default:** `1` (enabled)
**Valid Values:**
- `0` = Use exact coordinates (all bots stack)
- `1` = Randomize ±50 yards (recommended)

**Performance Impact:** None

**Usage:**
```conf
# Production: Enable for spread-out placement
Playerbot.WorldPositioner.RandomizeCoordinates = 1

# Testing: Disable for exact positioning
Playerbot.WorldPositioner.RandomizeCoordinates = 0
```

**Randomization:**
```cpp
float x_offset = (rand() % 100) - 50;  // ±50 yards
float y_offset = (rand() % 100) - 50;
float z_offset = (rand() % 10) - 5;    // ±5 yards vertical

final_x = base_x + x_offset;
final_y = base_y + y_offset;
final_z = base_z + z_offset;
```

**Recommendations:**
- **Always enable** to prevent bot clustering
- Disable only for testing specific coordinates

---

### `Playerbot.WorldPositioner.LoadZonesFromDatabase`

**Purpose:** Load zone definitions from database instead of hardcoded list.

**Type:** Boolean (0 or 1)
**Default:** `0` (hardcoded zones)
**Valid Values:**
- `0` = Use 40+ hardcoded zones (default)
- `1` = Load zones from `playerbot_zone_placements` table

**Performance Impact:** **LOW** (database query at startup)

**Usage:**
```conf
# Production: Use hardcoded zones
Playerbot.WorldPositioner.LoadZonesFromDatabase = 0

# Custom zones: Load from database
Playerbot.WorldPositioner.LoadZonesFromDatabase = 1
```

**Database Table (optional):**
```sql
CREATE TABLE `playerbot_zone_placements` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `zone_id` INT UNSIGNED NOT NULL,
  `map_id` INT UNSIGNED NOT NULL,
  `x` FLOAT NOT NULL,
  `y` FLOAT NOT NULL,
  `z` FLOAT NOT NULL,
  `orientation` FLOAT DEFAULT 0.0,
  `min_level` TINYINT UNSIGNED NOT NULL,
  `max_level` TINYINT UNSIGNED NOT NULL,
  `faction` TINYINT UNSIGNED NOT NULL COMMENT '0=Alliance, 1=Horde, 2=Neutral',
  `zone_name` VARCHAR(100) NOT NULL,
  `is_starter_zone` TINYINT(1) DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `idx_level_faction` (`min_level`, `max_level`, `faction`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

**Recommendations:**
- Use hardcoded zones (default) for simplicity
- Use database zones for custom zone definitions

---

### `Playerbot.WorldPositioner.LogPlacements`

**Purpose:** Enable detailed zone placement logging.

**Type:** Boolean (0 or 1)
**Default:** `0` (disabled)
**Valid Values:**
- `0` = No placement logging
- `1` = Log each bot placement

**Performance Impact:** **LOW** (minor I/O overhead)

**Usage:**
```conf
# Production: Disable
Playerbot.WorldPositioner.LogPlacements = 0

# Development: Enable for verification
Playerbot.WorldPositioner.LogPlacements = 1
```

**Log Output Example (when enabled):**
```
INFO: [Bot: Testbot] Zone Placement:
  Level: 22
  Faction: Alliance (0)
  Selected Zone: Redridge Mountains
    Zone ID: 44
    Map ID: 0 (Eastern Kingdoms)
    Level Range: L15-25
    Coordinates: -9447.83, 64.17, 56.48, 1.23
    Randomized Offset: +12.4, -8.7, +2.1
    Final Position: -9435.43, 55.47, 58.58

✅ Teleportation successful
```

**Recommendations:**
- Disable in production (reduces log size)
- Enable for debugging placement issues

---

## Level Manager Settings

All Level Manager settings are documented in [Global Settings](#global-settings) section:

- `Playerbot.LevelManager.Enabled`
- `Playerbot.LevelManager.MaxBotsPerUpdate`
- `Playerbot.LevelManager.ThreadPoolWorkers`
- `Playerbot.LevelManager.DistributionTolerance`
- `Playerbot.LevelManager.VerboseLogging`

See [Global Settings](#global-settings) for details.

---

## Configuration Templates

### Production Server (5000 Bots)

**File:** `playerbots.conf.production`

```conf
###############################################
# AUTOMATED WORLD POPULATION - PRODUCTION
###############################################

# System Control
Playerbot.LevelManager.Enabled = 1
Playerbot.LevelManager.MaxBotsPerUpdate = 20
Playerbot.LevelManager.ThreadPoolWorkers = 4
Playerbot.LevelManager.DistributionTolerance = 15
Playerbot.LevelManager.VerboseLogging = 0

# Gear Factory
Playerbot.GearFactory.EnableCache = 1
Playerbot.GearFactory.MinItemLevel = 5
Playerbot.GearFactory.MinQuality = 2
Playerbot.GearFactory.LogStats = 0
Playerbot.GearFactory.UseStatWeighting = 1

# Talent Manager
Playerbot.TalentManager.CacheLoadouts = 1
Playerbot.TalentManager.EnableDualSpec = 1
Playerbot.TalentManager.LogApplications = 0

# World Positioner
Playerbot.WorldPositioner.UseStarterZones = 1
Playerbot.WorldPositioner.UseCapitalFallback = 1
Playerbot.WorldPositioner.RandomizeCoordinates = 1
Playerbot.WorldPositioner.LoadZonesFromDatabase = 0
Playerbot.WorldPositioner.LogPlacements = 0

# Level Distribution (Balanced - 100% total)
Playerbot.LevelDistribution.Alliance.Bracket.L1 = 3.0
Playerbot.LevelDistribution.Alliance.Bracket.L5 = 5.0
Playerbot.LevelDistribution.Alliance.Bracket.L10 = 6.0
Playerbot.LevelDistribution.Alliance.Bracket.L15 = 6.5
Playerbot.LevelDistribution.Alliance.Bracket.L20 = 7.0
Playerbot.LevelDistribution.Alliance.Bracket.L25 = 7.5
Playerbot.LevelDistribution.Alliance.Bracket.L30 = 8.0
Playerbot.LevelDistribution.Alliance.Bracket.L35 = 8.0
Playerbot.LevelDistribution.Alliance.Bracket.L40 = 7.0
Playerbot.LevelDistribution.Alliance.Bracket.L45 = 6.5
Playerbot.LevelDistribution.Alliance.Bracket.L50 = 6.0
Playerbot.LevelDistribution.Alliance.Bracket.L55 = 5.5
Playerbot.LevelDistribution.Alliance.Bracket.L60 = 7.0
Playerbot.LevelDistribution.Alliance.Bracket.L65 = 6.0
Playerbot.LevelDistribution.Alliance.Bracket.L70 = 6.0
Playerbot.LevelDistribution.Alliance.Bracket.L75 = 7.0
Playerbot.LevelDistribution.Alliance.Bracket.L80 = 8.0

# Horde distribution (same as Alliance)
Playerbot.LevelDistribution.Horde.Bracket.L1 = 3.0
Playerbot.LevelDistribution.Horde.Bracket.L5 = 5.0
Playerbot.LevelDistribution.Horde.Bracket.L10 = 6.0
Playerbot.LevelDistribution.Horde.Bracket.L15 = 6.5
Playerbot.LevelDistribution.Horde.Bracket.L20 = 7.0
Playerbot.LevelDistribution.Horde.Bracket.L25 = 7.5
Playerbot.LevelDistribution.Horde.Bracket.L30 = 8.0
Playerbot.LevelDistribution.Horde.Bracket.L35 = 8.0
Playerbot.LevelDistribution.Horde.Bracket.L40 = 7.0
Playerbot.LevelDistribution.Horde.Bracket.L45 = 6.5
Playerbot.LevelDistribution.Horde.Bracket.L50 = 6.0
Playerbot.LevelDistribution.Horde.Bracket.L55 = 5.5
Playerbot.LevelDistribution.Horde.Bracket.L60 = 7.0
Playerbot.LevelDistribution.Horde.Bracket.L65 = 6.0
Playerbot.LevelDistribution.Horde.Bracket.L70 = 6.0
Playerbot.LevelDistribution.Horde.Bracket.L75 = 7.0
Playerbot.LevelDistribution.Horde.Bracket.L80 = 8.0
```

---

### Development/Testing Server

**File:** `playerbots.conf.testing`

```conf
###############################################
# AUTOMATED WORLD POPULATION - TESTING
###############################################

# System Control (verbose logging enabled)
Playerbot.LevelManager.Enabled = 1
Playerbot.LevelManager.MaxBotsPerUpdate = 5
Playerbot.LevelManager.ThreadPoolWorkers = 2
Playerbot.LevelManager.DistributionTolerance = 20
Playerbot.LevelManager.VerboseLogging = 1

# Gear Factory (all logging enabled)
Playerbot.GearFactory.EnableCache = 1
Playerbot.GearFactory.MinItemLevel = 1
Playerbot.GearFactory.MinQuality = 2
Playerbot.GearFactory.LogStats = 1
Playerbot.GearFactory.UseStatWeighting = 1

# Talent Manager (all logging enabled)
Playerbot.TalentManager.CacheLoadouts = 1
Playerbot.TalentManager.EnableDualSpec = 1
Playerbot.TalentManager.LogApplications = 1

# World Positioner (all logging enabled)
Playerbot.WorldPositioner.UseStarterZones = 1
Playerbot.WorldPositioner.UseCapitalFallback = 1
Playerbot.WorldPositioner.RandomizeCoordinates = 1
Playerbot.WorldPositioner.LoadZonesFromDatabase = 0
Playerbot.WorldPositioner.LogPlacements = 1

# Even distribution for testing (5.88% each)
Playerbot.LevelDistribution.Alliance.Bracket.L1 = 5.88
Playerbot.LevelDistribution.Alliance.Bracket.L5 = 5.88
Playerbot.LevelDistribution.Alliance.Bracket.L10 = 5.88
Playerbot.LevelDistribution.Alliance.Bracket.L15 = 5.88
Playerbot.LevelDistribution.Alliance.Bracket.L20 = 5.88
Playerbot.LevelDistribution.Alliance.Bracket.L25 = 5.88
Playerbot.LevelDistribution.Alliance.Bracket.L30 = 5.88
Playerbot.LevelDistribution.Alliance.Bracket.L35 = 5.88
Playerbot.LevelDistribution.Alliance.Bracket.L40 = 5.88
Playerbot.LevelDistribution.Alliance.Bracket.L45 = 5.88
Playerbot.LevelDistribution.Alliance.Bracket.L50 = 5.88
Playerbot.LevelDistribution.Alliance.Bracket.L55 = 5.88
Playerbot.LevelDistribution.Alliance.Bracket.L60 = 5.88
Playerbot.LevelDistribution.Alliance.Bracket.L65 = 5.88
Playerbot.LevelDistribution.Alliance.Bracket.L70 = 5.88
Playerbot.LevelDistribution.Alliance.Bracket.L75 = 5.88
Playerbot.LevelDistribution.Alliance.Bracket.L80 = 5.96

# Horde (same even distribution)
Playerbot.LevelDistribution.Horde.Bracket.L1 = 5.88
# ... (same as Alliance)
```

---

## Validation & Testing

### Pre-Deployment Validation Checklist

```bash
#!/bin/bash
# validate_config.sh

echo "=== Automated World Population Config Validation ==="

# 1. Check file exists
if [ ! -f "etc/playerbots.conf" ]; then
    echo "❌ FAIL: playerbots.conf not found"
    exit 1
fi
echo "✅ PASS: Config file exists"

# 2. Validate Alliance distribution
ALLIANCE_SUM=$(grep "Playerbot.LevelDistribution.Alliance.Bracket" etc/playerbots.conf | \
               awk -F'=' '{s += $2} END {print s}')
if (( $(echo "$ALLIANCE_SUM >= 99.9 && $ALLIANCE_SUM <= 100.1" | bc -l) )); then
    echo "✅ PASS: Alliance distribution = ${ALLIANCE_SUM}%"
else
    echo "❌ FAIL: Alliance distribution = ${ALLIANCE_SUM}% (must be 100.0%)"
    exit 1
fi

# 3. Validate Horde distribution
HORDE_SUM=$(grep "Playerbot.LevelDistribution.Horde.Bracket" etc/playerbots.conf | \
            awk -F'=' '{s += $2} END {print s}')
if (( $(echo "$HORDE_SUM >= 99.9 && $HORDE_SUM <= 100.1" | bc -l) )); then
    echo "✅ PASS: Horde distribution = ${HORDE_SUM}%"
else
    echo "❌ FAIL: Horde distribution = ${HORDE_SUM}% (must be 100.0%)"
    exit 1
fi

# 4. Check required settings
REQUIRED_SETTINGS=(
    "Playerbot.LevelManager.Enabled"
    "Playerbot.GearFactory.EnableCache"
    "Playerbot.TalentManager.CacheLoadouts"
)

for SETTING in "${REQUIRED_SETTINGS[@]}"; do
    if grep -q "$SETTING" etc/playerbots.conf; then
        echo "✅ PASS: $SETTING defined"
    else
        echo "❌ FAIL: $SETTING missing"
        exit 1
    fi
done

echo "==================================="
echo "✅ ALL VALIDATION CHECKS PASSED"
echo "==================================="
```

---

### Runtime Testing Commands

```
# 1. Verify system initialized
.playerbot status

# 2. Check subsystem health
.playerbot distribution status
.playerbot gear status
.playerbot talents status
.playerbot zones status

# 3. Test individual components
.playerbot distribution test 0  # Alliance bracket selection
.playerbot gear test 1 0 20 0   # Warrior/Protection/L20 gear
.playerbot talents test 5 2 45  # Priest/Shadow/L45 talents
.playerbot zone test 60 1       # Horde L60 zone

# 4. Monitor queue
.playerbot queue status

# 5. Check statistics
.playerbot stats
```

---

## Contact & Support

**For Configuration Assistance:**

- **Documentation**: See DEPLOYMENT_GUIDE.md and API_REFERENCE.md
- **Troubleshooting**: See TROUBLESHOOTING_FAQ.md
- **GitHub Issues**: Report configuration problems
- **Community Forums**: Share custom configurations

---

**Last Updated:** 2025-01-18
**Version:** 1.0.0
**Compatible With:** TrinityCore WoW 12.0 (The War Within)
**Total Settings:** 52 (5 global + 34 distribution + 5 gear + 3 talent + 5 positioner)
