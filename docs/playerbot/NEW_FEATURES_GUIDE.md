# Playerbot New Features Guide

This document covers all new systems added in the February 2026 feature batch. Each section explains what the feature does, how to enable it, and how to configure it.

---

## Table of Contents

1. [TTK Estimator (Cast-Time vs Time-to-Kill)](#1-ttk-estimator)
2. [Healing Mana Efficiency Tiers](#2-healing-mana-efficiency-tiers)
3. [RPG World Simulation](#3-rpg-world-simulation)
4. [Spell Fallback Chains](#4-spell-fallback-chains)
5. [ClassSpellDatabase](#5-classspelldatabase)
6. [Population PID Controller](#6-population-pid-controller)
7. [Enchant/Gem Template System](#7-enchantgem-template-system)
8. [Chat/Emote Templates](#8-chatemote-templates)
9. [Dynamic Reaction Delay](#9-dynamic-reaction-delay)
10. [DirtyValue Lazy Evaluation](#10-dirtyvalue-lazy-evaluation)
11. [Guild Task System](#11-guild-task-system)
12. [Account Linking System](#12-account-linking-system)
13. [BotCheatMask (Debug/Testing)](#13-botcheatmask)

---

## 1. TTK Estimator

**Purpose**: Prevents bots from wasting long cast-time spells on targets that are about to die.

**How it works**: The TTK (Time-to-Kill) Estimator tracks group DPS over a rolling 5-second window and calculates how long each target will survive. If a spell's cast time exceeds 80% of the target's estimated remaining life, the bot skips it and picks a faster alternative.

**Activation**: Enabled automatically. No configuration required. The system is integrated into `CombatBehaviorIntegration` and activates whenever bots are in combat.

**Key classes**:
- `AI/Combat/TTKEstimator.h` - Core estimation engine
- `AI/Combat/CombatBehaviorIntegration.h` - Integration with bot combat loop
- `AI/Combat/TargetSelector.cpp` - `EstimateTimeToKill()` delegates to TTKEstimator

**Behavior**:
- Solo bots use a 1.0x cast-time-to-TTK ratio (more lenient)
- Group bots use a 0.8x ratio (stricter, since group DPS is higher)
- New targets with no DPS history use max HP / average group DPS
- Targets with no health change for 3+ seconds get TTK = infinity (boss invulnerability phases)

---

## 2. Healing Mana Efficiency Tiers

**Purpose**: Prevents healer bots from going OOM by restricting expensive spells based on current mana percentage.

**How it works**: Each healer spec has its healing spells categorized into efficiency tiers. As mana drops, more expensive tiers become locked.

**Activation**: Enabled automatically for all 7 healer specs. No configuration required.

**Tier System**:

| Tier | Mana Threshold | Examples |
|------|---------------|----------|
| VERY_HIGH | Always allowed | Heal, Renew, Rejuvenation, Riptide, Holy Light |
| HIGH | Blocked below 30% mana | Flash Heal, Flash of Light, Healing Surge, Regrowth |
| MEDIUM | Blocked below 50% mana | Chain Heal, Wild Growth, Healing Rain, Penance |
| LOW | Blocked below 70% mana | Power Word: Radiance, Swiftmend, Spirit Bloom |
| EMERGENCY | Always allowed | Guardian Spirit, Lay on Hands, Life Cocoon |

**Tank targeting**: When healing a tank, mana thresholds are relaxed by +20% (e.g., HIGH tier is blocked at 10% mana instead of 30%).

**Supported specs**:
- Holy Priest, Discipline Priest
- Restoration Druid, Restoration Shaman
- Holy Paladin
- Mistweaver Monk
- Preservation Evoker

**Key files**:
- `AI/ClassAI/HealingEfficiencyManager.h/.cpp` - Core tier system
- `AI/ClassAI/HealingSpellTierData.h` - Per-class spell tier mappings
- Each healer spec header (e.g., `Priests/HolyPriest.h`) - `IsHealAllowedByMana()` checks

---

## 3. RPG World Simulation

**Purpose**: Gives idle bots (no master, no group) autonomous daily routines driven by their personality profiles.

**How it works**: Bots without a human master cycle through RPG activities based on their personality type. The system generates a daily schedule and transitions bots between states like grinding, questing, exploring, city life, and resting.

**Activation**: Enabled automatically for bots not assigned to a player. Requires the existing `PersonalityProfile` system to be active.

**RPG States**:
```
IDLE -> CITY_LIFE -> GRINDING -> QUESTING -> TRAVELING
     -> GATHERING -> EXPLORING -> RESTING -> DUNGEON -> SOCIALIZING
```

**Personality-Driven Schedules**:
- **Adventurous**: 60% grinding/questing, 20% exploration, 10% city, 10% rest
- **Social**: 30% city, 25% dungeon, 20% questing, 15% socializing, 10% rest
- **Cautious**: 40% grinding (safe zones), 30% city, 20% training, 10% rest

**Key files**:
- `Humanization/Activities/RPGDailyRoutineManager.h/.cpp`
- `Humanization/Activities/RPGActivityScheduler.h/.cpp`
- `Humanization/Activities/ZoneSelector.h/.cpp`

**Integration**: Hooked into `BotAI::Update()` - when a bot has no master or group, the RPG routine manager takes over decision-making.

---

## 4. Spell Fallback Chains

**Purpose**: When a bot's primary spell fails (on cooldown, out of range, insufficient mana), it automatically tries alternatives instead of wasting the GCD.

**How it works**: Each spec defines ordered fallback chains for common spell categories (single-target heal, AoE damage, etc.). When the primary spell cannot be cast, the system iterates through alternatives ranked by efficiency.

**Activation**: Enabled automatically. Each spec registers its own fallback chains during initialization.

**Example chain** (Holy Priest single-target heal):
```
Flash Heal (primary)
  -> Heal (more efficient, 1.2x weight)
  -> Renew (instant, 1.5x weight)
  -> Prayer of Mending (instant, if available)
```

**Integration with other systems**:
- **TTK Estimator**: Fallback chains skip spells that would exceed the target's TTK
- **Mana Tiers**: Fallback chains skip spells blocked by the current mana tier
- Both checks happen transparently during `SelectBestAvailable()`

**Key files**:
- `AI/ClassAI/SpellFallbackChain.h/.cpp` - Core fallback logic
- Used by individual spec files via `chain.SelectBestAvailable(bot, target, canCastFn)`

---

## 5. ClassSpellDatabase

**Purpose**: Centralized static database of class/spec spell data, eliminating redundant per-bot spell lookups.

**How it works**: A singleton database initialized at startup containing rotation templates, stat weights, interrupt/defensive/cooldown spell lists for all 13 classes and their specializations. Follows the `InterruptDatabase` pattern.

**Activation**: Initialized automatically at startup (initOrder=170 via ClassBehaviorTreeRegistry). No configuration needed.

**Query examples**:
```cpp
// Get rotation for Frost Mage
auto const* rotation = ClassSpellDatabase::GetRotation(CLASS_MAGE, SPEC_FROST);

// Get stat weights for Holy Paladin
auto const* weights = ClassSpellDatabase::GetStatWeights(CLASS_PALADIN, SPEC_HOLY);

// Check if a spell is a defensive cooldown
bool isDefensive = ClassSpellDatabase::IsDefensiveCooldown(spellId);
```

**Key files**:
- `AI/ClassAI/ClassSpellDatabase.h/.cpp`

---

## 6. Population PID Controller

**Purpose**: Replaces the reactive deficit-based bot population system with a PID (Proportional-Integral-Derivative) controller for smoother, more stable bot spawning.

**How it works**: Instead of spawning/despawning large batches of bots reactively, the PID controller smoothly adjusts the population toward the target. This prevents oscillation (too many bots -> despawn -> too few -> spawn -> repeat).

**Activation**: Integrated into `DemandCalculator`. Enabled automatically.

**Configuration** (in `DemandCalculator` or config):
- `Kp = 0.3` - Proportional gain (how fast to react to current error)
- `Ki = 0.05` - Integral gain (how fast to eliminate steady-state error)
- `Kd = 0.1` - Derivative gain (how much to dampen oscillation)
- Update cycle: every 5 seconds
- Anti-windup: integral term clamped to prevent runaway spawning
- Deadband: no action taken when error < 2 bots (prevents micro-adjustments)

**Key files**:
- `Lifecycle/Demand/PopulationPIDController.h/.cpp`
- `Lifecycle/Demand/DemandCalculator.h/.cpp` (integration)

---

## 7. Enchant/Gem Template System

**Purpose**: Automatically applies optimal enchants and gems to bot equipment based on class, spec, and role.

**How it works**: A database of enchant and gem recommendations is loaded at startup. When bots receive gear (via BotGearFactory), the system applies the best available enchant and gems to each equipment slot.

**Activation**: Enabled automatically when `BotGearFactory` is active. Controlled by two existing config flags:
- `Playerbot.GearFactory.Enchants = 1` (enable auto-enchanting)
- `Playerbot.GearFactory.Gems = 1` (enable auto-gemming)

**Database setup**: Run the SQL file to create template tables:
```sql
source sql/playerbot/10_enchant_gem_templates.sql
```

This creates two tables:
- `playerbot_enchant_recommendations` - Enchant templates by slot/class/spec
- `playerbot_gem_recommendations` - Gem templates by socket color/class/spec

**Default data**: The SQL file includes default enchant and gem recommendations for all roles (tank, melee DPS, ranged DPS, healer).

**Key files**:
- `Equipment/EnchantGemDatabase.h/.cpp` - Template storage and lookup
- `Equipment/EnchantGemManager.h/.cpp` - Application logic
- `Equipment/BotGearFactory.cpp` - Integration point (Phase 3.5)

---

## 8. Chat/Emote Templates

**Purpose**: Makes bots chat contextually during gameplay events (combat start, death, OOM, loot, etc.) using database-driven text templates.

**How it works**: A database table stores chat messages categorized by event type (greeting, aggro, death, OOM, loot, etc.). Messages are filtered by class, race, and locale, then selected with weighted randomness. A cooldown system prevents spam.

**Activation**: Enabled automatically.

**Database setup**: Run the SQL file:
```sql
source sql/playerbot/11_chat_templates.sql
```

**Categories**: greeting, farewell, aggro, low_health, death, oom, loot_excited, ready_check, thank_you, buff_request, pull_ready, wipe, victory, res_thanks, afk_return

**Customization**: Add your own chat messages to the `playerbot_chat_templates` table:
```sql
INSERT INTO playerbot_chat_templates (category, message, class_id, race_id, locale, weight)
VALUES ('aggro', 'For the Horde!', 0, 2, 'enUS', 1.5);
```

- `class_id = 0` means all classes
- `race_id = 0` means all races
- Higher `weight` = more likely to be selected

**Key files**:
- `Chat/ChatTemplateManager.h/.cpp`

---

## 9. Dynamic Reaction Delay

**Purpose**: Scales bot reaction time based on server load. Under heavy load, bots react slower (100ms -> 500ms) to reduce CPU pressure. When idle, bots react faster for more responsive gameplay.

**How it works**: The ServerLoadMonitor tracks the server's tick time ratio (actual tick time / target tick time). Based on this ratio, it calculates a reaction delay using piecewise linear interpolation.

**Activation**: Enabled automatically (registered as subsystem at updateOrder=700). No configuration needed.

**Load levels and delays**:

| Server Load | Tick Ratio | Reaction Delay |
|-------------|-----------|----------------|
| IDLE | < 30% | 100ms (minimum) |
| LIGHT | 30-60% | 100-200ms (interpolated) |
| NORMAL | 60-80% | 200ms |
| HEAVY | 80-100% | 200-350ms (interpolated) |
| OVERLOADED | > 100% | 350-500ms (interpolated) |

**Querying from code**:
```cpp
#include "Session/ServerLoadMonitor.h"

// Get current recommended delay
uint32 delay = Playerbot::ServerLoadMonitor::instance()->GetReactionDelay();

// Get current load level
auto level = Playerbot::ServerLoadMonitor::instance()->GetLoadLevel();

// Get tick time ratio (0.0 = idle, 1.0 = at target)
float ratio = Playerbot::ServerLoadMonitor::instance()->GetTickTimeRatio();
```

**Key files**:
- `Session/ServerLoadMonitor.h/.cpp`

---

## 10. DirtyValue Lazy Evaluation

**Purpose**: A utility template that eliminates redundant per-tick recomputation for values that change infrequently.

**How it works**: `DirtyValue<T>` wraps a value with a dirty flag. The value is only recomputed when explicitly invalidated. `TimedDirtyValue<T>` adds automatic TTL-based invalidation.

**Activation**: This is a utility template - not a standalone feature. Used internally by other systems. Available for any new code to use.

**Usage**:
```cpp
#include "Core/DirtyValue.h"

// Basic lazy value
Playerbot::DirtyValue<float> threatScore([this]() {
    return ComputeExpensiveThreatScore();
});

threatScore.Invalidate();           // Mark as stale
float val = threatScore.Get();      // Recomputes only if dirty
float cached = threatScore.GetCached(); // Get without recompute

// TTL-based auto-invalidation (recomputes every 5 seconds)
Playerbot::TimedDirtyValue<float> cachedScore(
    [this]() { return ComputeScore(); },
    5000  // TTL in milliseconds
);

float val = cachedScore.Get(getMSTime()); // Recomputes if dirty OR TTL expired
```

**Key files**:
- `Core/DirtyValue.h` (header-only template)

---

## 11. Guild Task System

**Purpose**: Generates tasks for guild bot members (kill, gather, craft, fish, mine, herb, dungeon, deliver, scout) and assigns them for autonomous completion.

**How it works**: The GuildTaskManager periodically generates tasks from weighted templates. Tasks are assigned to capable bots based on level, skills, and profession matching. Bots receive gold and reputation rewards upon completion.

**Activation**: Enabled automatically (registered as subsystem at initOrder=420, updateOrder=800).

**Database setup**: Run the SQL file to create task templates:
```sql
source sql/playerbot/12_guild_tasks.sql
```

This creates the `playerbot_guild_task_templates` table with 17 default task templates.

**Task types**:

| Type | Description | Requirements |
|------|-------------|-------------|
| KILL | Kill N creatures | Level-based |
| GATHER | Gather N resources | None |
| CRAFT | Craft N items | Profession skill |
| FISH | Catch N fish | Fishing skill (356) |
| MINE | Mine N ore nodes | Mining skill (186) |
| HERB | Pick N herbs | Herbalism skill (182) |
| SKIN | Skin N creatures | Skinning skill (393) |
| DUNGEON | Complete a dungeon | Level 15+ |
| DELIVER | Deposit to guild bank | None |
| SCOUT | Explore a zone | None |

**Configuration constants** (in `GuildTaskManager.h`):
- `GENERATION_INTERVAL_MS = 300000` (new tasks generated every 5 minutes)
- `ASSIGNMENT_INTERVAL_MS = 30000` (assignment check every 30 seconds)
- `MAX_ACTIVE_TASKS_PER_GUILD = 10` (cap on simultaneous tasks)
- `MAX_TASKS_PER_BOT = 2` (each bot handles up to 2 tasks)

**Custom templates**: Add your own task types to the database:
```sql
INSERT INTO playerbot_guild_task_templates
(task_type, difficulty, title_format, description_format, min_count, max_count,
 required_level, required_skill, required_skill_value, base_gold_reward,
 base_rep_reward, duration_hours, weight)
VALUES
(0, 1, 'Dragon Slayer', 'Kill dragons for the guild.', 3, 5, 60, 0, 0, 2000, 10, 48, 0.5);
```

**Querying from code**:
```cpp
#include "Social/GuildTaskManager.h"

auto* mgr = Playerbot::GuildTaskManager::instance();

// Get available tasks for a guild
auto tasks = mgr->GetAvailableTasks(guildId);

// Get tasks assigned to a specific bot
auto botTasks = mgr->GetBotTasks(botGuid);

// Manually create a task
uint32 taskId = mgr->CreateTask(guildId, GuildTaskType::KILL, 0, 20, 24);

// Report progress
mgr->ReportProgress(taskId, 5);
```

**Key files**:
- `Social/GuildTaskManager.h/.cpp`
- `sql/playerbot/12_guild_tasks.sql`

---

## 12. Account Linking System

**Purpose**: Links human player accounts with bot accounts, enabling permission-based access to bot features (inventory viewing, trading, control, guild sharing, etc.).

**How it works**: A human player can claim ownership of bot accounts, granting specific permissions. Each link has a bitmask of allowed actions.

**Activation**: Enabled automatically (registered as subsystem at initOrder=430).

**Database setup**: Run the SQL file:
```sql
source sql/playerbot/13_account_linking.sql
```

**Permission flags**:

| Permission | Hex | Description |
|-----------|-----|-------------|
| VIEW_INVENTORY | 0x0001 | See bot's inventory |
| TRADE | 0x0002 | Trade items with bot |
| CONTROL | 0x0004 | Issue movement/combat commands |
| SHARE_GUILD | 0x0008 | Bot auto-joins owner's guild |
| SHARE_FRIENDS | 0x0010 | Share friends list |
| SUMMON | 0x0020 | Summon bot to location |
| DISMISS | 0x0040 | Log bot out |
| RENAME | 0x0080 | Rename the bot |
| EQUIP | 0x0100 | Change bot equipment |
| SPEC | 0x0200 | Change bot spec/talents |

**Permission presets**:
- `BASIC` (0x0061): VIEW_INVENTORY + SUMMON + DISMISS
- `STANDARD` (0x006F): BASIC + TRADE + CONTROL + SHARE_GUILD
- `FULL` (0x03FF): All permissions

**Usage from code**:
```cpp
#include "Account/AccountLinkingManager.h"

auto* mgr = Playerbot::AccountLinkingManager::instance();

// Create a link (human account 1 -> bot account 100)
uint32 linkId = mgr->CreateLink(1, 100, LinkPermission::STANDARD);

// Create a link to a specific bot character
uint32 linkId = mgr->CreateLink(1, botGuid, LinkPermission::FULL);

// Check permissions
bool canTrade = mgr->HasPermission(1, botGuid, LinkPermission::TRADE);

// Get all links owned by a player
auto links = mgr->GetOwnedLinks(playerAccountId);

// Remove a link
mgr->RemoveLink(linkId);
```

**Key files**:
- `Account/AccountLinkingManager.h/.cpp`
- `sql/playerbot/13_account_linking.sql`

---

## 13. BotCheatMask

**Purpose**: Debug and testing cheat system that allows selective per-bot cheats (speed boost, god mode, infinite mana, one-shot kills, etc.).

**How it works**: Each bot can have a bitmask of active cheat flags. Cheats are toggled via code (future: `.bot cheat` chat commands). Multipliers for speed, damage, and XP are customizable per bot.

**Activation**: Enabled automatically (registered as subsystem at initOrder=440). Cheats are OFF by default for all bots - they must be explicitly enabled.

**Available cheats**:

| Cheat | Flag | Description |
|-------|------|-------------|
| speed | 0x0001 | 2x movement speed (customizable) |
| fly | 0x0002 | Enable flying |
| nofall | 0x0004 | No fall damage |
| teleport | 0x0008 | Instant teleport to target |
| damage | 0x0010 | 10x damage (customizable) |
| health | 0x0020 | Infinite health (auto-heal to full) |
| mana | 0x0040 | Infinite mana/resources |
| cooldowns | 0x0080 | No spell cooldowns |
| god | 0x0100 | Immune to all damage |
| oneshot | 0x0200 | Kill targets in one hit |
| instant | 0x0400 | Instant cast all spells |
| noaggro | 0x0800 | NPCs won't aggro |
| lootall | 0x1000 | Auto-loot everything |
| unlimitedbag | 0x2000 | Never run out of bag space |
| xpboost | 0x4000 | 10x XP gain (customizable) |

**Cheat presets**:
- `combat` - All combat cheats (damage + health + mana + cooldowns + god + oneshot + instant)
- `movement` - All movement cheats (speed + fly + nofall + teleport)
- `all` - Everything enabled

**Usage from code**:
```cpp
#include "Core/Diagnostics/BotCheatMask.h"

auto* cheats = Playerbot::BotCheatMask::instance();

// Toggle god mode on a bot
cheats->ToggleCheat(botGuid, BotCheatFlag::GOD_MODE);

// Enable all combat cheats
cheats->EnableCheat(botGuid, BotCheatFlag::ALL_COMBAT);

// Set custom damage multiplier (50x instead of default 10x)
cheats->SetDamageMultiplier(botGuid, 50.0f);

// Check if a bot has a cheat
if (cheats->HasCheat(botGuid, BotCheatFlag::SPEED))
    // apply speed boost...

// Get formatted list of active cheats
std::string active = cheats->FormatActiveCheats(botGuid);
// Returns: "speed, god, damage, mana"

// Clear all cheats on all bots
cheats->ClearAllBotCheats();

// In combat systems, use the helper methods:
uint32 modifiedDmg = cheats->ModifyDamage(botGuid, baseDamage);
bool takeDamage = cheats->ShouldTakeDamage(botGuid);
bool hasCooldown = cheats->ShouldHaveCooldown(botGuid);
```

**Key files**:
- `Core/Diagnostics/BotCheatMask.h/.cpp`

---

## SQL Setup Summary

Run all SQL files against the **characters** database:

```sql
source sql/playerbot/10_enchant_gem_templates.sql
source sql/playerbot/11_chat_templates.sql
source sql/playerbot/12_guild_tasks.sql
source sql/playerbot/13_account_linking.sql
```

---

## Build Requirements

All features are compiled into the worldserver automatically when `BUILD_PLAYERBOT=1` is set in CMake:

```bash
cmake -B build -DBUILD_PLAYERBOT=1 -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --config RelWithDebInfo --target worldserver
```

No additional build flags or dependencies are required.

---

## Subsystem Registration Order

All new systems are registered via the SubsystemRegistry and initialize automatically:

| Order | System | Type |
|-------|--------|------|
| 195 | EnchantGemDatabase | Init |
| 420 | GuildTaskManager | Init + Update |
| 430 | AccountLinkingManager | Init |
| 440 | BotCheatMask | Init |
| 700 | ServerLoadMonitor | Update |
| 800 | GuildTaskManager | Update |
