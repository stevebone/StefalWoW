# TrinityCore Playerbot User Guide

**Version:** WoW 11.2 Compatible
**Target Audience:** Server administrators, developers, and power users
**Module:** Optional TrinityCore enhancement

---

## Table of Contents

1. [Introduction](#introduction)
2. [Getting Started](#getting-started)
3. [Configuration Guide](#configuration-guide)
4. [Chat Commands](#chat-commands)
5. [Common Use Cases](#common-use-cases)
6. [Troubleshooting](#troubleshooting)
7. [Advanced Topics](#advanced-topics)
8. [FAQ](#faq)

---

## Introduction

### What is TrinityCore Playerbot?

TrinityCore Playerbot is an optional module that adds AI-controlled player characters (bots) to your World of Warcraft server. It enables:

- **Single-Player Experience**: Play the full MMORPG without other human players
- **Dungeon Practice**: Run dungeons with AI-controlled party members
- **Testing**: Test raid mechanics, quests, and game systems
- **Population Enhancement**: Add life to low-population servers

### Key Features

- **5000 Bot Capacity**: Designed to support up to 5000 concurrent bots
- **13 Classes Supported**: All WoW classes with specialized AI
- **Smart Following**: Bots automatically follow group leaders
- **Combat Assistance**: Bots engage in combat when group members fight
- **Quest Automation**: Bots can pickup and complete quests (when idle)
- **Group Coordination**: Proper role-based positioning and targeting
- **Performance Optimized**: <10% server impact with proper configuration

### System Requirements

**Minimum:**
- TrinityCore 11.2+ (master branch)
- 8 GB RAM (for ~100 bots)
- 4 CPU cores
- MySQL 9.4+

**Recommended (500+ bots):**
- 32 GB RAM
- 8+ CPU cores
- SSD storage
- MySQL 9.4+ with optimized configuration

---

## Getting Started

### Step 1: Compilation

The Playerbot module is **optional** and disabled by default. To enable it:

#### Windows (Visual Studio)

```powershell
# Navigate to build directory
cd C:\TrinityCore\build

# Generate with Playerbot enabled
cmake .. -DBUILD_PLAYERBOT=1

# Open solution and build
start TrinityCore.sln
# Build → Build Solution (F7)
```

#### Linux/Mac (Make)

```bash
# Navigate to build directory
cd ~/TrinityCore/build

# Generate with Playerbot enabled
cmake .. -DBUILD_PLAYERBOT=1

# Compile
make -j$(nproc)

# Install
make install
```

**Verification:**
```bash
# Check if module compiled
ls -la server/game/libplayerbot.a  # Linux/Mac
dir server\game\playerbot.lib      # Windows
```

### Step 2: Configuration File

The Playerbot module uses a **separate configuration file** from `worldserver.conf`.

#### Create Configuration File

1. **Locate Template:**
   ```
   etc/playerbots.conf.dist
   ```

2. **Copy to Active Config:**
   ```bash
   # Linux/Mac
   cp etc/playerbots.conf.dist etc/playerbots.conf

   # Windows
   copy etc\playerbots.conf.dist etc\playerbots.conf
   ```

3. **Edit Configuration:**
   Open `etc/playerbots.conf` in your preferred text editor.

#### Essential Settings

```ini
###################################################################################################
# ESSENTIAL SETTINGS - Must Configure
###################################################################################################

# Enable the playerbot system
Playerbot.Enable = 1

# Maximum bots allowed globally (adjust based on RAM)
# Guideline: 100 bots ≈ 1 GB RAM
Playerbot.GlobalMaxBots = 100

# Maximum bots per player account
Playerbot.MaxBotsPerAccount = 5

# Bot-to-player ratio (2.0 = 2 bots per real player)
# Set to 0 to disable dynamic spawning
Playerbot.BotToPlayerRatio = 2.0

# Enable dynamic spawning (auto-spawn based on player count)
Playerbot.EnableDynamicSpawning = 1
```

### Step 3: Database Setup

The Playerbot module can use either:
1. **Existing characters database** (recommended)
2. **Separate playerbot database** (advanced)

#### Option 1: Use Existing Characters Database (Recommended)

No additional setup required. Bots will be stored in the standard `characters` database.

#### Option 2: Create Separate Database (Advanced)

```sql
-- Create database
CREATE DATABASE `playerbot_characters` DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

-- Grant permissions
GRANT ALL PRIVILEGES ON `playerbot_characters`.* TO 'trinity'@'localhost';
FLUSH PRIVILEGES;
```

**Update Configuration:**
```ini
Playerbot.Database.Name = "playerbot_characters"
```

### Step 4: First Launch

1. **Start worldserver:**
   ```bash
   ./worldserver
   ```

2. **Check Logs:**
   Look for initialization messages:
   ```
   [INFO] PlayerbotConfig: Successfully loaded from ./etc/playerbots.conf
   [INFO] BotSpawner initialized - Max Total: 100, Max Per Zone: 10, Max Per Map: 50
   [INFO] Playerbot: Module loaded successfully
   ```

3. **Login to Game:**
   Use your GM account (security level 3+)

4. **Verify Module:**
   Type in-game:
   ```
   .bot
   ```
   You should see available bot commands.

---

## Configuration Guide

### Core Configuration

```ini
###################################################################################################
# CORE SETTINGS
###################################################################################################

# Enable/disable entire playerbot system
# Values: 0 (disabled), 1 (enabled)
# Default: 0
Playerbot.Enable = 1

# Global maximum bots (server-wide)
# Higher values require more RAM
# Guideline: 100 bots ≈ 1 GB RAM, 1000 bots ≈ 10 GB RAM
# Default: 500
# Recommended: 100 (low-end), 500 (mid-range), 1000 (high-end)
Playerbot.GlobalMaxBots = 500

# Maximum bots per player account
# Default: 10
# Recommended: 5-10 for normal play, 50+ for testing
Playerbot.MaxBotsPerAccount = 10
```

### Performance Tuning

```ini
###################################################################################################
# PERFORMANCE SETTINGS
###################################################################################################

# Bot update interval (milliseconds)
# Lower = more responsive, higher = better performance
# Range: 50-1000 ms
# Default: 100
# Recommended: 100 (smooth), 200 (balanced), 500 (performance)
Playerbot.UpdateInterval = 100

# AI decision time limit (milliseconds)
# Maximum time AI can spend per decision
# Prevents lag from complex AI calculations
# Default: 50
# Recommended: 50 (normal), 100 (complex AI)
Playerbot.AIDecisionTimeLimit = 50

# Spawn batch size (bots spawned per batch)
# Larger batches = faster spawning but brief lag spikes
# Default: 10
# Recommended: 5 (smooth), 10 (balanced), 20 (fast)
Playerbot.SpawnBatchSize = 10

# Delay between spawn batches (milliseconds)
# Higher values = smoother spawning
# Default: 100
# Recommended: 100-500
Playerbot.SpawnBatchDelay = 100
```

### Dynamic Spawning

```ini
###################################################################################################
# DYNAMIC SPAWNING
###################################################################################################

# Enable dynamic spawning (auto-spawn based on players)
# Values: 0 (disabled), 1 (enabled)
# Default: 1
Playerbot.EnableDynamicSpawning = 1

# Bot-to-player ratio
# How many bots to spawn per real player
# 0.0 = no automatic spawning
# 2.0 = 2 bots per player (recommended)
# 5.0 = 5 bots per player (crowded)
# Default: 2.0
Playerbot.BotToPlayerRatio = 2.0

# Minimum bots per zone (even with 0 players)
# Keeps zones populated for testing
# Default: 10
Playerbot.MinimumBotsPerZone = 5

# Maximum bots per zone
# Prevents overcrowding
# Default: 50
Playerbot.MaxBotsPerZone = 50

# Maximum bots per map
# Global limit per continent/instance
# Default: 200
Playerbot.MaxBotsPerMap = 200
```

### Database Configuration

```ini
###################################################################################################
# DATABASE SETTINGS
###################################################################################################

# Database connection timeout (seconds)
# Default: 30
Playerbot.Database.Timeout = 30

# Enable connection pooling (recommended)
# Improves performance for many bots
# Default: 1
Playerbot.Database.EnablePooling = 1

# Maximum database connections for bot operations
# Higher = better performance with many bots
# Too high = MySQL connection limit issues
# Default: 10
# Recommended: 5 (< 100 bots), 10 (100-500 bots), 20 (500+ bots)
Playerbot.Database.MaxConnections = 10
```

### Logging Configuration

```ini
###################################################################################################
# LOGGING SETTINGS
###################################################################################################

# Log level (higher = more detailed)
# 0 = Disabled
# 1 = Fatal errors only
# 2 = Errors
# 3 = Warnings
# 4 = Info (recommended for normal use)
# 5 = Debug (for troubleshooting)
# 6 = Trace (very detailed, huge logs)
# Default: 4
Playerbot.Log.Level = 4

# Log file path (relative to worldserver binary)
# Default: "Playerbot.log"
Playerbot.Log.File = "Playerbot.log"

# Enable timestamp in logs
# Default: 1
Playerbot.Log.Timestamp = 1

# Separate log files by category
# Creates files like Playerbot_AI.log, Playerbot_Combat.log
# Useful for debugging specific systems
# Default: 0
Playerbot.Log.SeparateCategories = 0
```

### Security Settings

```ini
###################################################################################################
# SECURITY SETTINGS
###################################################################################################

# Prevent direct login to bot accounts
# Bots can only be controlled via commands
# Recommended: 1
# Default: 1
Playerbot.Security.PreventBotLogin = 1

# Log all bot actions for audit trail
# Creates large logs, use only for debugging
# Default: 0
Playerbot.Security.LogBotActions = 0

# Maximum gold a bot can carry (prevents exploits)
# Default: 1000 (in copper, = 10 gold)
# 0 = no limit
Playerbot.Security.MaxGoldPerBot = 100000
```

### AI Behavior

```ini
###################################################################################################
# AI BEHAVIOR SETTINGS
###################################################################################################

# Follow distance (yards)
# How far behind leader bots should follow
# Default: 5.0
Playerbot.AI.FollowDistance = 5.0

# Combat assist range (yards)
# Distance at which bots assist group members in combat
# Default: 30.0
Playerbot.AI.CombatAssistRange = 30.0

# Auto-loot enabled
# Bots automatically loot corpses
# Default: 1
Playerbot.AI.AutoLoot = 1

# Auto-accept group invites
# Bots automatically accept group invitations
# Default: 1
Playerbot.AI.AutoAcceptGroupInvites = 1

# Quest automation enabled (when idle)
# Bots pickup and complete quests when not in combat/following
# Default: 0 (disabled for now - experimental)
Playerbot.AI.EnableQuestAutomation = 0
```

---

## Chat Commands

### Command Prefix

All bot commands start with `.bot` or `.pbot`:
```
.bot <subcommand> [arguments]
.pbot <subcommand> [arguments]
```

### Basic Commands

#### `.bot spawn <class> <race> <gender> <name>`

Creates a new bot character and spawns it.

**Parameters:**
- `class` - Character class name (warrior, paladin, hunter, rogue, priest, shaman, mage, warlock, druid, deathknight, monk, demonhunter, evoker)
- `race` - Race name (human, orc, dwarf, nightelf, undead, tauren, gnome, troll, bloodelf, draenei, worgen, goblin, pandaren, etc.)
- `gender` - Gender (male, female)
- `name` - Character name (must be unique)

**Examples:**
```
.bot spawn warrior human male BotWarrior
.bot spawn priest nightelf female HealBot
.bot spawn mage bloodelf female Fireblast
.bot spawn paladin dwarf male HolyTank
```

**What Happens:**
1. Creates new character in database
2. Spawns character in starting zone
3. Bot immediately enters world
4. Character appears in `.bot list`

---

#### `.bot add <characterName>`

Adds an existing character as a bot.

**Parameters:**
- `characterName` - Name of existing character on your account

**Example:**
```
.bot add MyExistingMage
```

**Requirements:**
- Character must exist in database
- Character must belong to your account
- Character must not already be logged in

**Use Case:** Convert your alt characters to bots

---

#### `.bot remove <name|all>`

Removes one or all bots.

**Parameters:**
- `name` - Specific bot name
- `all` - Remove all your bots

**Examples:**
```
.bot remove BotWarrior    # Remove specific bot
.bot remove all           # Remove all your bots
```

**What Happens:**
1. Bot logs out gracefully
2. Removes from active bot list
3. Character remains in database (can re-add later)

---

#### `.bot list`

Lists all your active bots.

**Output:**
```
Your active bots (3):
  - BotWarrior (Level 80 Warrior)
  - HealBot (Level 70 Priest)
  - Fireblast (Level 60 Mage)
```

**Shows:**
- Bot name
- Level
- Class

---

#### `.bot info <name>`

Displays detailed bot information.

**Example:**
```
.bot info BotWarrior
```

**Output:**
```
Bot Information: BotWarrior
  Class: Warrior
  Race: Human
  Level: 80
  Specialization: Arms
  Health: 45000 / 50000 (90%)
  Resource: 80 / 100 Rage
  AI State: FOLLOWING
  Group: [GroupName] (Tank role)
  Current Action: Following [YourCharacter]
  Active Strategies: follow, group_combat
  Performance:
    - Average Update Time: 0.8ms
    - Actions Executed: 1523
    - Total Updates: 15234
```

---

#### `.bot stats`

Shows global bot statistics.

**Output:**
```
Playerbot System Statistics:
  Total Spawned: 150
  Currently Active: 45
  Peak Concurrent: 52
  Failed Spawns: 2
  Success Rate: 98.7%
  Average Spawn Time: 125ms
  Total Despawned: 105

Performance Metrics:
  Average Update Time: 1.2ms
  Total Memory Usage: ~450MB
  Database Queries/sec: 42
```

---

### Control Commands

#### `.bot follow`

Makes all your bots follow you.

**Example:**
```
.bot follow
```

**Effect:**
- All your bots activate "follow" strategy
- Bots move to follow distance behind you
- Bots maintain formation based on role

**Note:** Bots automatically follow when invited to your group

---

#### `.bot stay`

Makes all your bots stop following.

**Example:**
```
.bot stay
```

**Effect:**
- Deactivates "follow" strategy
- Bots stop moving
- Bots stay at current location

---

#### `.bot attack`

Makes all your bots attack your current target.

**Example:**
```
.bot attack
```

**Requirements:**
- You must have a target selected
- Target must be attackable

**Effect:**
- Bots engage your target
- Uses appropriate class rotation
- Continues attacking until target dies or combat ends

---

#### `.bot defend`

Makes bots defend you from attackers.

**Example:**
```
.bot defend
```

**Effect:**
- Bots attack anything attacking you
- Prioritizes threats to you
- Stays near you for protection

---

#### `.bot heal`

Makes healer bots focus on healing.

**Example:**
```
.bot heal
```

**Effect:**
- Healer bots prioritize healing
- Focus on lowest health targets
- Uses efficient healing rotation

**Note:** Only affects healer classes (Priest, Paladin, Shaman, Druid, Monk, Evoker)

---

### Debug Commands

#### `.bot debug <name> <on|off>`

Enables detailed logging for specific bot.

**Example:**
```
.bot debug BotWarrior on   # Enable debug
.bot debug BotWarrior off  # Disable debug
```

**Output Location:** `Playerbot.log` with DEBUG level entries

**Use Case:** Troubleshooting AI behavior, combat issues, movement problems

---

## Common Use Cases

### Solo Dungeon Practice

**Goal:** Practice dungeon mechanics with AI party.

**Setup:**
1. **Spawn a balanced group:**
   ```
   .bot spawn warrior human male Tank         # Tank
   .bot spawn priest nightelf female Healer   # Healer
   .bot spawn mage human female DPS1          # DPS
   .bot spawn rogue human male DPS2           # DPS
   ```

2. **Invite to group:**
   - Use standard group invitation (`/invite BotName`)
   - Bots auto-accept and start following

3. **Enter dungeon:**
   - Queue for dungeon or enter manually
   - Bots follow and engage in combat when you fight

**Tips:**
- Ensure "follow" strategy is active (`.bot follow`)
- Pull carefully - bots will assist but may not kite
- Healer bots prioritize lowest health ally
- Tank bots use threat abilities when spec supports it

---

### Testing Raid Mechanics

**Goal:** Test raid encounter mechanics with bots.

**Setup:**
1. **Spawn raid-sized group:**
   ```bash
   # Use a script to spawn 10-25 bots
   for i in {1..10}; do
       .bot spawn warrior human male Tank$i
       .bot spawn priest human female Healer$i
       .bot spawn mage human male DPS$i
   done
   ```

2. **Form raid:**
   - Create raid group
   - Invite all bots
   - Assign roles in raid frames

3. **Enter raid instance:**
   - Bots follow raid leader
   - Engage when raid leader attacks

**Limitations:**
- Bots may not handle complex mechanics (don't move from fire, etc.)
- Best for testing basic encounter flow
- Advanced mechanics require manual control

---

### Leveling with Bots

**Goal:** Level with AI companions.

**Setup:**
1. **Spawn appropriate level bots:**
   ```
   .bot spawn hunter human male BowBot
   .bot spawn priest human female HealyBot
   ```

2. **Invite to group:**
   - Standard group invitation
   - Bots follow and assist in combat

3. **Quest together:**
   - Bots help kill quest mobs
   - Share quest completion credit
   - Healers keep you alive

**Tips:**
- Bots scale to nearby player levels (if enabled in config)
- Bring a healer bot for safety
- Bots automatically loot if configured

---

### Populating Low-Pop Server

**Goal:** Add life to empty zones.

**Configuration:**
```ini
# In playerbots.conf
Playerbot.EnableDynamicSpawning = 1
Playerbot.BotToPlayerRatio = 5.0
Playerbot.MinimumBotsPerZone = 20
Playerbot.GlobalMaxBots = 500
```

**Effect:**
- Bots spawn automatically in popular zones
- When players login, more bots spawn (ratio 5:1)
- Zones feel populated even with few players
- Bots despawn when players logout to conserve resources

**Management:**
- Monitor via `.bot stats`
- Adjust ratio based on performance
- Use lower ratios on weaker servers

---

### Testing New Content

**Goal:** Test quest chains, dungeons, raids with bots.

**Setup:**
1. **Create test characters:**
   ```
   .bot spawn warrior human male TestTank
   .bot spawn priest human female TestHeal
   ```

2. **Set to max level (GM command):**
   ```
   .character level TestTank 80
   .character level TestHeal 80
   ```

3. **Gear them:**
   ```
   .additem [item_id]
   ```

4. **Test content:**
   - Invite to group
   - Run through quest chains
   - Enter dungeons/raids
   - Observe bot behavior

**Benefits:**
- Fast iteration on encounter design
- No need for human testers
- Consistent testing environment

---

## Troubleshooting

### Bot Not Spawning

**Symptoms:**
- Command completes but bot doesn't appear
- Error message: "Failed to spawn bot"

**Diagnosis:**
```
# Check logs
tail -f Playerbot.log

# Check active bots
.bot stats
```

**Common Causes:**

1. **Global bot limit reached**
   ```ini
   # In playerbots.conf
   Playerbot.GlobalMaxBots = 500  # Increase this
   ```

2. **Account bot limit reached**
   ```ini
   Playerbot.MaxBotsPerAccount = 10  # Increase this
   ```

3. **Database connection failure**
   - Check `Playerbot.log` for database errors
   - Verify MySQL is running
   - Check database credentials in `worldserver.conf`

4. **Invalid character data**
   - Ensure race/class combination is valid
   - Check name doesn't already exist
   - Verify character creation limits

**Solutions:**
```bash
# Increase limits
vi etc/playerbots.conf
# Edit GlobalMaxBots and MaxBotsPerAccount

# Restart worldserver
killall worldserver
./worldserver

# Verify database
mysql -u trinity -p
> USE characters;
> SELECT COUNT(*) FROM characters WHERE account = [your_account_id];
```

---

### Bots Not Following

**Symptoms:**
- Bots invited to group but don't move
- Bots stay in one place
- Message: "Bot is following" but no movement

**Diagnosis:**
```
.bot info BotName
```
Check "Active Strategies" - should include "follow"

**Common Causes:**

1. **Follow strategy not activated**
   ```
   .bot follow  # Manually activate
   ```

2. **Bot stuck in combat state**
   - Kill all nearby mobs
   - Wait for combat to end
   - Bot should resume following

3. **Pathfinding failure**
   - Bot can't find path to you
   - Teleport closer: `.appear BotName`
   - Or teleport bot to you: `.summon BotName`

4. **Group not properly formed**
   - Re-invite bot to group
   - Check bot appears in group frames
   - Verify group leader is set

**Solutions:**
```
# Force follow activation
.bot follow

# Summon bot if stuck
.summon BotName

# Re-invite to group
/invite BotName

# Check AI state
.bot info BotName
# AI State should be: FOLLOWING
```

---

### Bots Not Attacking

**Symptoms:**
- Bots follow but don't engage in combat
- Bots stand idle while you fight

**Diagnosis:**
```
.bot info BotName
```
Check "Active Strategies" - should include "group_combat"

**Common Causes:**

1. **Combat strategy not activated**
   - Happens if bot joined group before system was ready
   - Solution: Re-invite bot or restart worldserver

2. **Bot out of range**
   - Check combat assist range in config
   ```ini
   Playerbot.AI.CombatAssistRange = 30.0  # Increase if needed
   ```

3. **No valid target**
   - Bot needs clear line of sight
   - Remove obstacles between bot and enemy

4. **Class-specific issues**
   - Mages may be out of mana
   - Melee may be too far from target
   - Healers prioritize healing over DPS

**Solutions:**
```
# Manually command attack
.bot attack

# Check combat range
vi etc/playerbots.conf
# Increase Playerbot.AI.CombatAssistRange

# Summon bot closer
.summon BotName

# Re-invite to refresh strategies
/invite BotName
```

---

### Performance Issues / Lag

**Symptoms:**
- Server lag with many bots
- High CPU usage
- Slow response times

**Diagnosis:**
```bash
# Check server stats
.server info

# Check bot stats
.bot stats

# Monitor CPU
top -p $(pgrep worldserver)

# Check memory
free -h
```

**Common Causes:**

1. **Too many bots for hardware**
   - Reduce `Playerbot.GlobalMaxBots`
   - Lower `Playerbot.BotToPlayerRatio`

2. **Update interval too low**
   ```ini
   # In playerbots.conf
   Playerbot.UpdateInterval = 100  # Increase to 200-500
   ```

3. **Database bottleneck**
   - Increase connection pool size
   - Optimize MySQL configuration
   - Use SSD for database

4. **Memory exhaustion**
   - Each bot uses ~10 MB
   - 500 bots = ~5 GB minimum
   - Check with `free -h`

**Solutions:**

**Quick Fix (Reduce Load):**
```ini
# In playerbots.conf
Playerbot.GlobalMaxBots = 50        # Reduce from 500
Playerbot.UpdateInterval = 200      # Increase from 100
Playerbot.SpawnBatchSize = 5        # Reduce from 10
```

**Database Optimization:**
```sql
-- In MySQL
SET GLOBAL max_connections = 200;
SET GLOBAL innodb_buffer_pool_size = 2G;
SET GLOBAL query_cache_size = 128M;
```

**System Tuning:**
```bash
# Increase file descriptors
ulimit -n 65536

# Use performance CPU governor
cpupower frequency-set -g performance
```

**Monitoring:**
```bash
# Watch resource usage
watch -n 1 'free -h && ps aux | grep worldserver | head -n 1'
```

---

### Database Errors

**Symptoms:**
- Errors in `Playerbot.log`
- "Failed to execute query"
- "Database connection timeout"

**Diagnosis:**
```bash
# Check Playerbot.log
tail -n 100 Playerbot.log | grep -i "error\|failed"

# Test MySQL connection
mysql -u trinity -p -h localhost
```

**Common Errors:**

1. **"Too many connections"**
   ```sql
   -- In MySQL
   SHOW VARIABLES LIKE 'max_connections';
   SET GLOBAL max_connections = 500;
   ```

2. **"Prepared statement failed"**
   - Outdated database schema
   - Missing table indexes
   - Corrupted prepared statements

   **Solution:**
   ```bash
   # Re-run database updates
   mysql -u trinity -p world < sql/updates/world/*.sql
   ```

3. **"Lock wait timeout exceeded"**
   - Database deadlock
   - Too many concurrent queries

   **Solution:**
   ```ini
   # In playerbots.conf
   Playerbot.Database.Timeout = 60  # Increase from 30
   Playerbot.Database.MaxConnections = 20  # Increase pool
   ```

4. **"Access denied"**
   - Wrong database credentials
   - Missing permissions

   **Solution:**
   ```sql
   GRANT ALL PRIVILEGES ON characters.* TO 'trinity'@'localhost';
   GRANT ALL PRIVILEGES ON world.* TO 'trinity'@'localhost';
   FLUSH PRIVILEGES;
   ```

---

### Compilation Errors

**Symptoms:**
- Build fails with Playerbot-related errors
- Linker errors mentioning Playerbot classes

**Common Errors:**

1. **"BUILD_PLAYERBOT not defined"**
   ```bash
   # Solution: Add flag to cmake
   cmake .. -DBUILD_PLAYERBOT=1
   ```

2. **"Cannot find playerbots.conf"**
   - Module compiled but config missing
   ```bash
   # Copy config file
   cp etc/playerbots.conf.dist etc/playerbots.conf
   ```

3. **Linker errors (unresolved symbols)**
   - Incomplete compilation
   ```bash
   # Clean and rebuild
   make clean
   cmake .. -DBUILD_PLAYERBOT=1
   make -j$(nproc)
   ```

4. **CMake errors**
   ```
   CMake Error: The following variables are used but not set:
     BUILD_PLAYERBOT
   ```
   **Solution:**
   ```bash
   # Explicitly set
   cmake .. -DBUILD_PLAYERBOT=ON
   ```

---

## Advanced Topics

### Creating Custom AI Strategies

For developers who want to extend bot behavior:

**1. Create Strategy Class:**
```cpp
// src/modules/Playerbot/AI/Strategy/MyCustomStrategy.h
#include "Strategy.h"

namespace Playerbot {

class MyCustomStrategy : public Strategy {
public:
    std::string GetName() const override {
        return "my_custom";
    }

    bool IsActive(BotAI const* ai) const override {
        // Determine when strategy should be active
        return true;
    }

    void UpdateBehavior(BotAI* ai, uint32 diff) override {
        // Implement custom behavior
        Player* bot = ai->GetBot();
        // ... custom logic
    }
};

} // namespace Playerbot
```

**2. Register Strategy:**
```cpp
// In BotAI.cpp or custom initializer
void InitializeCustomStrategies(BotAI* ai) {
    auto customStrategy = std::make_unique<MyCustomStrategy>();
    ai->AddStrategy(std::move(customStrategy));
    ai->ActivateStrategy("my_custom");
}
```

**3. Use Strategy:**
```
# In-game (if added to commands)
.bot strategy activate my_custom
```

---

### Performance Profiling

**Enable Detailed Logging:**
```ini
# In playerbots.conf
Playerbot.Log.Level = 5  # Debug level
Playerbot.Performance.EnableMonitoring = 1
```

**Analyze Performance:**
```cpp
// Access performance metrics
SpawnStats const& stats = sBotSpawner->GetStats();
TC_LOG_INFO("module.playerbot", "Performance Report:");
TC_LOG_INFO("module.playerbot", "  Average Update: {:.2f}ms", stats.GetAverageSpawnTime());
TC_LOG_INFO("module.playerbot", "  Peak Concurrent: {}", stats.peakConcurrent.load());
```

**Monitor in Real-Time:**
```bash
# Watch Playerbot.log for performance entries
tail -f Playerbot.log | grep "Performance\|Update Time"
```

---

### Contributing to Playerbot

**Development Workflow:**

1. **Fork Repository:**
   ```bash
   git clone https://github.com/YourUsername/TrinityCore.git
   cd TrinityCore
   git checkout playerbot-dev  # Development branch
   ```

2. **Create Feature Branch:**
   ```bash
   git checkout -b feature/my-improvement
   ```

3. **Make Changes:**
   - Edit files in `src/modules/Playerbot/`
   - Follow existing code style
   - Add comments and documentation

4. **Test Changes:**
   ```bash
   # Rebuild
   cmake .. -DBUILD_PLAYERBOT=1
   make -j$(nproc)

   # Test in-game
   ./worldserver
   # ... test your changes
   ```

5. **Submit Pull Request:**
   ```bash
   git add .
   git commit -m "Add feature: improved bot targeting"
   git push origin feature/my-improvement
   # Create PR on GitHub
   ```

**Code Style:**
- Follow TrinityCore C++ coding standards
- Use namespace `Playerbot`
- Add logging with `TC_LOG_*` macros
- Include comprehensive error handling

---

### MySQL Optimization for High Bot Counts

**For 500+ bots, optimize MySQL:**

**1. Configuration (`my.cnf` or `my.ini`):**
```ini
[mysqld]
# Connection limits
max_connections = 500

# Buffer pool (set to 50-70% of available RAM)
innodb_buffer_pool_size = 4G

# Query cache
query_cache_size = 256M
query_cache_limit = 2M

# Threads
thread_cache_size = 100

# Temporary tables
tmp_table_size = 128M
max_heap_table_size = 128M

# InnoDB settings
innodb_log_file_size = 512M
innodb_log_buffer_size = 64M
innodb_flush_log_at_trx_commit = 2  # Faster, slightly less safe

# Query optimization
join_buffer_size = 4M
sort_buffer_size = 4M
```

**2. Index Optimization:**
```sql
-- Add indexes for bot queries
ALTER TABLE characters
ADD INDEX idx_bot_account_online (account, online);

ALTER TABLE characters
ADD INDEX idx_bot_level_class (level, class);

-- Optimize tables periodically
OPTIMIZE TABLE characters;
OPTIMIZE TABLE item_instance;
```

**3. Monitoring:**
```sql
-- Check connection usage
SHOW STATUS LIKE 'Threads_connected';
SHOW STATUS LIKE 'Max_used_connections';

-- Check slow queries
SHOW VARIABLES LIKE 'slow_query_log';
SET GLOBAL slow_query_log = 'ON';
SET GLOBAL long_query_time = 1;  # Log queries > 1 second
```

---

## FAQ

### General Questions

**Q: How many bots can I run on my server?**

**A:** Depends on hardware:
- **Low-end** (4 GB RAM, 2 cores): 50-100 bots
- **Mid-range** (8 GB RAM, 4 cores): 200-500 bots
- **High-end** (16+ GB RAM, 8+ cores): 500-1000 bots
- **Server-grade** (32+ GB RAM, 16+ cores): 1000-5000 bots

Rule of thumb: **100 bots ≈ 1 GB RAM**

---

**Q: Can bots do quests automatically?**

**A:** Partially. Bots can:
- ✅ Pick up quests (when idle)
- ✅ Kill quest mobs
- ✅ Turn in quests
- ❌ Navigate complex quest chains
- ❌ Use quest items properly
- ❌ Complete escort quests

Currently experimental. Enable with:
```ini
Playerbot.AI.EnableQuestAutomation = 1
```

---

**Q: Do bots work in PvP/Battlegrounds?**

**A:** Limited support:
- ✅ Can enter battlegrounds
- ✅ Will attack enemy players
- ✅ Follow group in BGs
- ❌ Don't understand objectives (flags, bases)
- ❌ Limited tactical awareness
- ❌ May not use PvP abilities optimally

Best for testing, not competitive PvP.

---

**Q: Can I make money (gold) with bots?**

**A:** Not recommended:
- Bots can farm mobs (slowly)
- Auction house automation is disabled by default
- Exploiting bots for gold is against spirit of module
- Security limits prevent excessive gold accumulation

**Intended use:** Enhancing gameplay, not gold farming.

---

**Q: Will bots work with my custom scripts/modules?**

**A:** Usually yes:
- Bots use standard TrinityCore APIs
- Compatible with most server modifications
- May conflict with other AI systems
- Test thoroughly before production use

---

**Q: How do I update bots after server restart?**

**A:** Bots persist in database:
```sql
-- Bots remain in characters table
SELECT name, class, level FROM characters WHERE account IN (
    SELECT account_id FROM playerbot_accounts
);
```

After restart:
1. Worldserver starts
2. Playerbot module initializes
3. Bots automatically log back in (if dynamic spawning enabled)
4. Or manually re-add: `.bot add BotName`

---

### Technical Questions

**Q: Are bots visible to other players?**

**A:** Yes:
- Bots appear as normal players
- Other players can:
  - See bots in world
  - Target bots
  - Trade with bots (if enabled)
  - Invite bots to groups
  - Communicate with bots (limited responses)

---

**Q: Do bots use resources (bandwidth, CPU) like real players?**

**A:** Partially:
- **CPU:** Yes, ~0.1% per bot (AI calculations)
- **Memory:** Yes, ~10 MB per bot (Player objects)
- **Bandwidth:** No (socketless, no network packets sent)
- **Database:** Yes (queries for character data, updates)

**Bots are much lighter than real players** (no network overhead).

---

**Q: Can bots learn from player behavior?**

**A:** Experimental:
- Machine learning module exists
- Pattern recognition for player movements
- Adaptive difficulty based on player skill
- Currently disabled by default

Enable with:
```ini
Playerbot.Experimental.BehaviorLearning = 1
```

**Warning:** Very CPU-intensive, not recommended for production.

---

**Q: How do I backup bot data?**

**A:**
```bash
# Backup characters database (includes bots)
mysqldump -u trinity -p characters > characters_backup.sql

# Backup specific bot accounts
mysqldump -u trinity -p characters --where="account IN (SELECT account_id FROM playerbot_accounts)" > bots_backup.sql

# Restore
mysql -u trinity -p characters < characters_backup.sql
```

---

**Q: Can I control individual bot abilities?**

**A:** Not directly via commands (yet). But you can:
1. **Modify AI scripts** in `src/modules/Playerbot/AI/ClassAI/`
2. **Create custom strategies** (see Advanced Topics)
3. **Use debug mode** to see what abilities bots use
4. **Disable certain abilities** in class AI code

---

**Q: Why do my bots keep dying?**

**A:** Common reasons:
1. **No healer in group** - Add a healer bot
2. **Pulling too many mobs** - Bots have limited survival instincts
3. **Undergeared** - Equip bots with appropriate gear
4. **Wrong spec** - Check bot specialization matches role
5. **Not using cooldowns** - Some class AIs need tuning

**Debug:**
```
.bot info BotName  # Check health, buffs, spec
.bot debug BotName on  # Enable detailed combat logging
```

---

## Getting Help

### Official Channels

- **GitHub Issues:** https://github.com/TrinityCore/TrinityCore/issues
- **Discord:** TrinityCore Discord server (#playerbot-module channel)
- **Forum:** TrinityCore forums (Module Development section)

### Reporting Bugs

**Include:**
1. TrinityCore version/commit hash
2. Playerbot module version
3. Server specs (RAM, CPU, OS)
4. Number of bots when issue occurred
5. Relevant log excerpts from `Playerbot.log`
6. Steps to reproduce
7. Expected vs actual behavior

**Example:**
```
**Bug:** Bots not following after group invite

**Environment:**
- TrinityCore: 11.2.0 (commit ae80d8c)
- Playerbot: Latest (commit ae80d8c)
- Server: Ubuntu 22.04, 16 GB RAM, 8 cores
- Bots: 45 active

**Steps:**
1. Spawn bot: .bot spawn warrior human male TestBot
2. Invite to group: /invite TestBot
3. Bot accepts but doesn't move

**Logs:**
[2025-10-03 14:23:45] BotAI: OnGroupJoined() called for TestBot
[2025-10-03 14:23:45] Strategy "follow" activated
[2025-10-03 14:23:45] ERROR: LeaderFollowBehavior - No group leader found

**Expected:** Bot should follow me
**Actual:** Bot stands still
```

---

## Conclusion

The TrinityCore Playerbot module provides a powerful way to enhance your World of Warcraft server experience with AI-controlled players. Whether you're running a solo server, testing content, or adding life to a low-population realm, the Playerbot system offers flexible, performant bot management.

**Remember:**
- Start with low bot counts and scale up
- Monitor performance regularly
- Keep logs for troubleshooting
- Report bugs to help improve the module
- Have fun!

**Happy Botting!**

---

**Document Version:** 1.0
**Last Updated:** 2025-10-03
**For:** TrinityCore 11.2 + Playerbot Module
