# Playerbot Chat Commands Reference

This document provides a complete reference for all Playerbot commands available in-game.

## GM Commands (`.bot` prefix)

These are TrinityCore GM commands that require GM permissions (RBAC_PERM_COMMAND_GMNOTIFY).

### Bot Management

| Command | Usage | Description |
|---------|-------|-------------|
| `.bot spawn` | `.bot spawn <name> [race] [class]` | Create and spawn a new bot with optional race/class |
| `.bot delete` | `.bot delete <name>` | Remove bot from world (character data preserved in DB) |
| `.bot list` | `.bot list` | Show all currently active bots |
| `.bot info` | `.bot info <name>` | Show detailed information about a specific bot |

**Examples:**
```
.bot spawn Healer 1 5       -- Creates Human Priest named "Healer"
.bot spawn Tank             -- Creates bot with your race/class
.bot delete Healer          -- Removes "Healer" from world
.bot info Tank              -- Shows Tank's stats, position, group info
```

### Teleportation

| Command | Usage | Description |
|---------|-------|-------------|
| `.bot teleport` | `.bot teleport <name>` | Teleport yourself to the bot's location |
| `.bot summon` | `.bot summon <name>` | Summon a specific bot to your location |
| `.bot summon all` | `.bot summon all` | Summon all bots in your group to your location |

**Examples:**
```
.bot teleport Tank          -- You teleport to Tank
.bot summon Healer          -- Healer teleports to you
.bot summon all             -- All group bots teleport to you
```

### Formation Commands

| Command | Usage | Description |
|---------|-------|-------------|
| `.bot formation` | `.bot formation <type>` | Set the formation for all bots in your group |
| `.bot formation list` | `.bot formation list` | List all available formation types |

**Available Formations:**

| Formation | Description |
|-----------|-------------|
| `wedge` | V-shaped penetration formation (tank at point) |
| `diamond` | Balanced 4-point diamond with interior fill |
| `square` | Defensive box (tanks corners, healers center) |
| `line` | Horizontal line for maximum width coverage |
| `column` | Vertical single-file march formation |
| `scatter` | Spread formation for anti-AoE tactics |
| `circle` | 360° perimeter coverage formation |
| `dungeon` | Optimized dungeon formation (tank/healer/dps roles) |
| `raid` | Raid formation with 5-person groups |

**Examples:**
```
.bot formation dungeon      -- Sets dungeon formation
.bot formation wedge        -- Sets wedge/arrow formation
.bot formation list         -- Shows all formations
```

### Statistics & Monitoring

| Command | Usage | Description |
|---------|-------|-------------|
| `.bot stats` | `.bot stats` | Show global bot population and performance statistics |
| `.bot monitor` | `.bot monitor` | Show bot monitor summary dashboard |
| `.bot monitor trends` | `.bot monitor trends` | Show detailed performance trends (CPU, memory, queries) |

**Example Output:**
```
=== Bot Population ===
  Active Bots:      25
  Peak Concurrent:  30
  Total Spawned:    150
  Total Despawned:  125

=== Performance Metrics ===
  CPU Usage:        2.5% (avg)
  Memory Usage:     512 MB (avg)
  DB Query Time:    5.2 ms (avg)
```

### Alert Management

| Command | Usage | Description |
|---------|-------|-------------|
| `.bot alerts` | `.bot alerts` | Show active performance alerts |
| `.bot alerts history` | `.bot alerts history` | Show last 20 alerts |
| `.bot alerts clear` | `.bot alerts clear` | Clear all active alerts |

**Alert Levels:**
- `INFO` - Informational notices
- `WARNING` - Performance degradation warnings
- `CRITICAL` - Severe issues requiring attention

### Configuration

| Command | Usage | Description |
|---------|-------|-------------|
| `.bot config` | `.bot config <key> <value>` | Set a configuration value |
| `.bot config show` | `.bot config show` | Show all configuration settings |

**Configuration Categories:**
- **Bot Limits** - MaxBots, GlobalBotLimit
- **AI Behavior** - EnableAI, AITickRate
- **Formations** - FormationSpacing, FormationUpdateRate
- **Performance** - BotUpdateInterval, DecisionCacheTime
- **Logging** - LogLevel, LogBotActions

**Examples:**
```
.bot config MaxBots 100             -- Set max bots to 100
.bot config EnableAI true           -- Enable AI processing
.bot config show                    -- Show all settings
```

---

## Whisper Commands (Player to Bot)

Players can whisper commands directly to bots. These commands are processed by the BotChatCommandHandler system.

### Basic Commands

| Command | Description |
|---------|-------------|
| `follow` | Bot follows you |
| `follow me` | Bot follows you (natural language variant) |
| `stay` | Bot stays at current position |
| `stop` | Bot stops current action |
| `wait` | Bot waits at current position |
| `come` | Bot comes to your location |
| `come here` | Bot comes to your location |

### Combat Commands

| Command | Description |
|---------|-------------|
| `attack` | Bot attacks your current target |
| `attack [target]` | Bot attacks specified target |
| `assist` | Bot assists you in combat |
| `assist me` | Bot assists you in combat |
| `defend` | Bot enters defensive mode |
| `passive` | Bot enters passive mode (won't attack) |

### Role Commands

| Command | Description |
|---------|-------------|
| `tank` | Bot switches to tank role |
| `heal` | Bot switches to healer role |
| `dps` | Bot switches to DPS role |

### Information Commands

| Command | Description |
|---------|-------------|
| `help` | Show available whisper commands |
| `status` | Bot reports its current status |
| `stats` | Bot reports its statistics |

### Permission Levels

Whisper command access depends on relationship:
- **ANYONE** - Basic commands (help, status)
- **GROUP_MEMBER** - Combat and movement commands
- **GROUP_LEADER** - Role assignment commands
- **OWNER** - Full control commands

---

## Race and Class IDs

For use with `.bot spawn <name> <race> <class>`:

### Race IDs
| ID | Race |
|----|------|
| 1 | Human |
| 2 | Orc |
| 3 | Dwarf |
| 4 | Night Elf |
| 5 | Undead |
| 6 | Tauren |
| 7 | Gnome |
| 8 | Troll |
| 10 | Blood Elf |
| 11 | Draenei |
| 22 | Worgen |
| 24 | Pandaren (Neutral) |
| 25 | Pandaren (Alliance) |
| 26 | Pandaren (Horde) |
| 27 | Nightborne |
| 28 | Highmountain Tauren |
| 29 | Void Elf |
| 30 | Lightforged Draenei |
| 31 | Zandalari Troll |
| 32 | Kul Tiran |
| 34 | Dark Iron Dwarf |
| 35 | Vulpera |
| 36 | Mag'har Orc |
| 37 | Mechagnome |
| 52 | Dracthyr (Alliance) |
| 70 | Dracthyr (Horde) |
| 84 | Earthen (Alliance) |
| 85 | Earthen (Horde) |

### Class IDs
| ID | Class |
|----|-------|
| 1 | Warrior |
| 2 | Paladin |
| 3 | Hunter |
| 4 | Rogue |
| 5 | Priest |
| 6 | Death Knight |
| 7 | Shaman |
| 8 | Mage |
| 9 | Warlock |
| 10 | Monk |
| 11 | Druid |
| 12 | Demon Hunter |
| 13 | Evoker |

---

## Troubleshooting

### Command Not Found
- Ensure you have GM permissions
- Check spelling of command
- Use `.help bot` for TrinityCore's built-in help

### Bot Not Responding to Whispers
- Ensure bot is online and in world
- Check if you have permission level required
- Verify BotChatCommandHandler is enabled in config

### Formation Not Working
- Must be in a group with bots
- Bots need UnifiedMovementCoordinator active
- Check bot AI is enabled

### Performance Issues
- Use `.bot monitor trends` to identify bottlenecks
- Check `.bot alerts` for active warnings
- Reduce bot count if CPU/memory high

---

## See Also

- `PlayerbotCommands.cpp` - GM command implementation
- `BotChatCommandHandler.cpp` - Whisper command processing
- `ConfigManager.cpp` - Configuration system
- `BotMonitor.cpp` - Performance monitoring
