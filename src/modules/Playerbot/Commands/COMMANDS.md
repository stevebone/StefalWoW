# PlayerBot Chat Commands

This document provides comprehensive documentation for all PlayerBot chat commands available in-game.

## Overview

PlayerBot commands allow you to spawn, control, and manage AI-controlled player bots directly from the game chat interface. All commands are available under two command prefixes:
- `.bot <command>` - Primary command prefix
- `.pbot <command>` - Alternative command prefix

## Command Reference

### Bot Management Commands

#### `.bot add <class> <spec> [name]`

Spawns a new bot with the specified class and specialization.

**Parameters:**
- `class` (required) - Class ID (1-13)
  - 1 = Warrior
  - 2 = Paladin
  - 3 = Hunter
  - 4 = Rogue
  - 5 = Priest
  - 6 = Death Knight
  - 7 = Shaman
  - 8 = Mage
  - 9 = Warlock
  - 10 = Monk
  - 11 = Druid
  - 12 = Demon Hunter
  - 13 = Evoker
- `spec` (required) - Specialization ID (1-4, varies by class)
  - Generally: 1 = First spec, 2 = Second spec, 3 = Third spec
- `name` (optional) - Custom bot name (auto-generated if omitted)

**Examples:**
```
.bot add 1 1              # Spawn Warrior (Arms spec)
.bot add 8 3              # Spawn Mage (Frost spec)
.bot add 5 2 HealBot      # Spawn Priest (Discipline) named "HealBot"
```

**RBAC Permission:** 1000 (RBAC_PERM_COMMAND_PLAYERBOT_ADD)

---

#### `.bot remove <name|all>`

Removes a specific bot or all your bots.

**Parameters:**
- `name|all` (required) - Bot name or "all" to remove all bots

**Examples:**
```
.bot remove MyBot         # Remove bot named "MyBot"
.bot remove all           # Remove all your bots
```

**RBAC Permission:** 1001 (RBAC_PERM_COMMAND_PLAYERBOT_REMOVE)

---

#### `.bot list`

Lists all your currently active bots with their level, class, and specialization.

**Examples:**
```
.bot list
```

**Output:**
```
Your active bots (3):
  - WarriorBot (Level 80 Warrior Primary)
  - MageBot (Level 80 Mage Primary)
  - HealBot (Level 80 Priest Primary)
```

**RBAC Permission:** 1002 (RBAC_PERM_COMMAND_PLAYERBOT_LIST)

---

#### `.bot info <name>`

Shows detailed information about a specific bot including health, mana/power, position, and AI state.

**Parameters:**
- `name` (required) - Bot name

**Examples:**
```
.bot info WarriorBot
```

**Output:**
```
=== Bot Information: WarriorBot ===
Level: 80 | Class: Warrior | Spec: Primary
Health: 45000/50000 | Rage: 0/100
Position: Map 0, X:1234.56 Y:7890.12 Z:34.56
AI State: Active | In Combat: No
```

**RBAC Permission:** 1003 (RBAC_PERM_COMMAND_PLAYERBOT_INFO)

---

#### `.bot stats`

Shows overall PlayerBot system statistics including total bots, active bots, and system status.

**Examples:**
```
.bot stats
```

**Output:**
```
=== PlayerBot Statistics ===
Total Bots: 150 | Active: 150
Max Bots Allowed: 500
System Status: Enabled
```

**RBAC Permission:** 1004 (RBAC_PERM_COMMAND_PLAYERBOT_STATS)

---

### Bot Control Commands

#### `.bot follow <name>`

Makes a bot follow you.

**Parameters:**
- `name` (required) - Bot name

**Examples:**
```
.bot follow WarriorBot
```

**RBAC Permission:** 1005 (RBAC_PERM_COMMAND_PLAYERBOT_FOLLOW)

---

#### `.bot stay <name>`

Makes a bot stay at its current position.

**Parameters:**
- `name` (required) - Bot name

**Examples:**
```
.bot stay WarriorBot
```

**RBAC Permission:** 1006 (RBAC_PERM_COMMAND_PLAYERBOT_STAY)

---

#### `.bot attack [name]`

Makes bot(s) attack your currently selected target.

**Parameters:**
- `name` (optional) - Bot name. If omitted, ALL your bots will attack.

**Examples:**
```
.bot attack WarriorBot    # Single bot attacks
.bot attack               # All bots attack
```

**Requirements:**
- You must have a valid target selected
- Target must be attackable

**RBAC Permission:** 1007 (RBAC_PERM_COMMAND_PLAYERBOT_ATTACK)

---

#### `.bot defend <name>`

Sets a bot to defensive mode (will only attack when attacked).

**Parameters:**
- `name` (required) - Bot name

**Examples:**
```
.bot defend HealBot
```

**RBAC Permission:** 1008 (RBAC_PERM_COMMAND_PLAYERBOT_DEFEND)

---

#### `.bot heal <name>`

Makes a bot prioritize healing you.

**Parameters:**
- `name` (required) - Bot name (must be a healer class)

**Examples:**
```
.bot heal HealBot
```

**RBAC Permission:** 1009 (RBAC_PERM_COMMAND_PLAYERBOT_HEAL)

---

### Debugging & Testing Commands

#### `.bot debug <name>`

Toggles debug mode for a specific bot, enabling detailed AI decision logging.

**Parameters:**
- `name` (required) - Bot name

**Examples:**
```
.bot debug WarriorBot     # Toggle debug mode ON/OFF
```

**Output:**
```
Bot 'WarriorBot' debug mode: ON
```

**RBAC Permission:** 1010 (RBAC_PERM_COMMAND_PLAYERBOT_DEBUG)

---

#### `.bot spawn`

Spawns test bots - one of each class (13 total) for testing purposes.

**Examples:**
```
.bot spawn
```

**Output:**
```
Spawning test bots (one of each class)...
  - Warrior bot created
  - Paladin bot created
  - Hunter bot created
  ... (continues for all 13 classes)
Successfully spawned 13/13 test bots
```

**RBAC Permission:** 1011 (RBAC_PERM_COMMAND_PLAYERBOT_SPAWN_ALL)

---

## RBAC Permissions

All PlayerBot commands use custom RBAC permissions in the 1000+ range (reserved for custom modules):

| Command | Permission ID | Permission Name |
|---------|---------------|-----------------|
| `.bot add` | 1000 | RBAC_PERM_COMMAND_PLAYERBOT_ADD |
| `.bot remove` | 1001 | RBAC_PERM_COMMAND_PLAYERBOT_REMOVE |
| `.bot list` | 1002 | RBAC_PERM_COMMAND_PLAYERBOT_LIST |
| `.bot info` | 1003 | RBAC_PERM_COMMAND_PLAYERBOT_INFO |
| `.bot stats` | 1004 | RBAC_PERM_COMMAND_PLAYERBOT_STATS |
| `.bot follow` | 1005 | RBAC_PERM_COMMAND_PLAYERBOT_FOLLOW |
| `.bot stay` | 1006 | RBAC_PERM_COMMAND_PLAYERBOT_STAY |
| `.bot attack` | 1007 | RBAC_PERM_COMMAND_PLAYERBOT_ATTACK |
| `.bot defend` | 1008 | RBAC_PERM_COMMAND_PLAYERBOT_DEFEND |
| `.bot heal` | 1009 | RBAC_PERM_COMMAND_PLAYERBOT_HEAL |
| `.bot debug` | 1010 | RBAC_PERM_COMMAND_PLAYERBOT_DEBUG |
| `.bot spawn` | 1011 | RBAC_PERM_COMMAND_PLAYERBOT_SPAWN_ALL |

### Granting Permissions

To grant permissions to an account, use the RBAC commands:

```sql
-- Grant all playerbot permissions to GM account (account_id = 1)
INSERT INTO rbac_account_permissions (accountId, permissionId, granted, realmId) VALUES
(1, 1000, 1, -1),  -- bot add
(1, 1001, 1, -1),  -- bot remove
(1, 1002, 1, -1),  -- bot list
(1, 1003, 1, -1),  -- bot info
(1, 1004, 1, -1),  -- bot stats
(1, 1005, 1, -1),  -- bot follow
(1, 1006, 1, -1),  -- bot stay
(1, 1007, 1, -1),  -- bot attack
(1, 1008, 1, -1),  -- bot defend
(1, 1009, 1, -1),  -- bot heal
(1, 1010, 1, -1),  -- bot debug
(1, 1011, 1, -1);  -- bot spawn
```

Or use in-game commands:
```
.account set addon account_name 1000
```

---

## Configuration Requirements

Ensure PlayerBot is enabled in your `worldserver.conf`:

```ini
[worldserver.conf]
Playerbot.Enable = 1
Playerbot.MaxBots = 500
Playerbot.MaxBotsPerAccount = 50
```

---

## Troubleshooting

### "PlayerBot system is disabled in configuration"
**Solution:** Enable PlayerBot in worldserver.conf and restart the server.

### "Bot not found"
**Solution:** Check bot name with `.bot list`. Names are case-sensitive.

### "Failed to create bot. Check logs for details."
**Solution:** Check worldserver logs for specific error messages. Common issues:
- Database connection problems
- Invalid class/spec combination
- Account bot limit reached
- Server bot limit reached

### "You don't own this bot"
**Solution:** You can only control bots owned by your account.

### "No target selected"
**Solution:** Use `/target <name>` or click on a target before using `.bot attack`.

---

## Example Workflows

### Creating a 5-Man Dungeon Group

```
.bot add 1 3              # Spawn Protection Warrior (tank)
.bot add 5 2              # Spawn Discipline Priest (healer)
.bot add 8 3              # Spawn Frost Mage (dps)
.bot add 3 1              # Spawn Beast Mastery Hunter (dps)
.bot add 4 1              # Spawn Assassination Rogue (dps)
.bot list                 # Verify all bots created
```

### Group Combat Control

```
/target [Enemy Name]      # Select enemy target
.bot attack               # All bots attack
.bot follow TankBot       # Follow tank after combat
```

### Quick Testing Setup

```
.bot spawn               # Spawn one of each class (13 bots)
.bot stats               # Check system status
.bot list                # See all spawned bots
```

---

## Advanced Usage

### Bot Naming Conventions

If you don't specify a name, bots are auto-named using the pattern:
- Format: `<ClassName><Number>`
- Examples: `Warrior1`, `Mage2`, `Priest3`

Custom names must:
- Be 2-12 characters long
- Contain only letters (no numbers or special characters)
- Be unique across the server

### Bot Ownership

- Bots are owned by the account that created them
- Only the owner can control their bots
- Bots persist across server restarts (stored in database)
- Bots are automatically cleaned up when account is deleted

---

## See Also

- [PlayerBot Configuration Guide](../Config/README.md)
- [Bot AI Behavior Guide](../AI/README.md)
- [Database Schema](../Database/README.md)
