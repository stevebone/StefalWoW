# TrinityCore Playerbot Module - Database Installation Guide

## Overview

The TrinityCore Playerbot module now features **automatic database setup**. In most cases, **NO MANUAL SQL INSTALLATION IS REQUIRED**. The system will:

1. **Auto-create the database** if it doesn't exist (with appropriate MySQL privileges)
2. **Auto-apply migrations** from `sql/migrations/` on server startup
3. **Track schema versions** to ensure source code matches database

## Quick Start (Recommended - Zero Manual Steps)

### Step 1: Configure Database Connection

Edit `playerbots.conf` (or let the guided setup create it for you):

```ini
Playerbot.Database.Host = "127.0.0.1"
Playerbot.Database.Port = 3306
Playerbot.Database.Name = "playerbot_playerbot"
Playerbot.Database.User = "trinity"
Playerbot.Database.Password = "trinity"
```

### Step 2: Start the Server

The module will automatically:
- Create the database if it doesn't exist
- Apply all pending migrations from `sql/migrations/`
- Log progress to the console

**That's it!** The database is ready.

---

## Automatic Features

### Auto-Create Database

If the database doesn't exist and the MySQL user has CREATE privilege:

```
[INFO] PlayerbotDatabaseConnection: Database 'playerbot_playerbot' does not exist, attempting auto-create...
[INFO] PlayerbotDatabaseConnection: Successfully created database 'playerbot_playerbot'
```

If CREATE privilege is missing, clear instructions are displayed:

```
================================================================================
  PLAYERBOT DATABASE SETUP REQUIRED
================================================================================

  Database 'playerbot_playerbot' does not exist and auto-creation failed.

  OPTION 1: Grant CREATE privilege for auto-creation
  ------------------------------------------------
  GRANT CREATE ON *.* TO 'trinity'@'localhost';
  FLUSH PRIVILEGES;

  OPTION 2: Create the database manually
  --------------------------------------
  CREATE DATABASE playerbot_playerbot CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
  GRANT ALL ON playerbot_playerbot.* TO 'trinity'@'localhost';
  FLUSH PRIVILEGES;

  After creating the database, restart the server.
  Schema migrations will be applied automatically.

================================================================================
```

### Auto-Migrations

The `PlayerbotMigrationMgr` automatically discovers and applies SQL files:

- **Location:** `sql/migrations/`
- **Pattern:** `001_*.sql`, `002_*.sql`, etc.
- **Tracking:** Applied migrations stored in `playerbot_migrations` table
- **Version Sync:** Database version checked against source code version

---

## Manual Installation (Legacy Method)

Only use this if automatic setup fails or you prefer manual control.

### SQL Files

| File | Purpose |
|------|---------|
| `migrations/001_playerbot_base.sql` | Complete schema + initial data |
| `optional/dragonriding_tables.sql` | Dragonriding hotfixes (manual apply) |

### Manual Steps

```bash
# Create database
mysql -u root -p -e "CREATE DATABASE playerbot_playerbot CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"

# Apply base migration
mysql -u trinity -p playerbot_playerbot < sql/migrations/001_playerbot_base.sql

# (Optional) Apply dragonriding tables
mysql -u trinity -p playerbot_playerbot < sql/optional/dragonriding_tables.sql
```

---

## Configuration

### Primary Config: playerbots.conf

All Playerbot settings are in `playerbots.conf` (NOT worldserver.conf).

**First-time setup:** If `playerbots.conf` doesn't exist, the guided setup will:
1. Display setup instructions
2. Optionally copy `playerbots.conf.dist` to `playerbots.conf`
3. Use tester-friendly defaults

### Quick Setup with Profiles

Use a pre-configured profile for instant setup:

```ini
# In playerbots.conf
Playerbot.Profile = "standard"
```

**Available Profiles:**
| Profile | Bots | Description |
|---------|------|-------------|
| `minimal` | 10 | Testing/development |
| `standard` | 100 | Recommended defaults |
| `performance` | 500 | High-population servers |
| `singleplayer` | varies | Solo play optimized |

### Essential Settings

```ini
# Enable the playerbot module
Playerbot.Enable = 1

# Maximum bots server-wide
Playerbot.MaxBots = 100

# Spawn bots on server start (no player login required)
Playerbot.Spawn.OnServerStart = 1

# Database connection
Playerbot.Database.Host = "127.0.0.1"
Playerbot.Database.Port = 3306
Playerbot.Database.Name = "playerbot_playerbot"
Playerbot.Database.User = "trinity"
Playerbot.Database.Password = "trinity"
```

## Database Schema Overview

### Core Tables

| Table | Purpose | Records |
|-------|---------|---------|
| `playerbot_activity_patterns` | Bot behavior patterns | 16 default patterns |
| `playerbot_schedules` | Bot login/logout scheduling | Dynamic (per bot) |
| `playerbot_spawn_log` | Spawn event tracking | Dynamic (rotated) |
| `playerbot_zone_populations` | Zone population control | 47 configured zones |
| `playerbot_lifecycle_events` | General event logging | Dynamic (rotated) |

### Character Management

| Table | Purpose | Records |
|-------|---------|---------|
| `playerbots_names` | Available bot names | 100 sample names |
| `playerbots_names_used` | Names currently in use | Dynamic |
| `playerbots_class_popularity` | Class distribution weights | 10 classes |
| `playerbots_gender_distribution` | Gender ratios by race | 10 races |
| `playerbots_race_class_distribution` | Valid race-class combinations | 65 combinations |

### Account Management

| Table | Purpose | Records |
|-------|---------|---------|
| `playerbot_accounts` | Bot account tracking | Dynamic |
| `playerbot_character_distribution` | Character distribution per account | Dynamic |

## Maintenance

### Automatic Cleanup

The schema includes stored procedures for maintenance:

```sql
-- Clean old spawn logs (30+ days)
CALL sp_cleanup_spawn_logs();

-- Clean old lifecycle events (7+ days, except errors)
CALL sp_cleanup_lifecycle_events();

-- Update zone population counts
CALL sp_update_zone_populations();

-- Get system statistics
CALL sp_get_system_stats();
```

### Recommended Cron Job

```bash
# Daily cleanup at 3 AM
0 3 * * * mysql -u playerbotuser -p'password' playerbot -e "CALL sp_cleanup_spawn_logs(); CALL sp_cleanup_lifecycle_events();"

# Hourly population updates
0 * * * * mysql -u playerbotuser -p'password' playerbot -e "CALL sp_update_zone_populations();"
```

## Troubleshooting

### Common Issues

1. **Foreign Key Constraint Errors**
   ```sql
   -- Disable temporarily during installation
   SET FOREIGN_KEY_CHECKS = 0;
   -- Re-enable after installation
   SET FOREIGN_KEY_CHECKS = 1;
   ```

2. **Character Set Issues**
   ```sql
   -- Verify database charset
   SELECT DEFAULT_CHARACTER_SET_NAME, DEFAULT_COLLATION_NAME
   FROM information_schema.SCHEMATA
   WHERE SCHEMA_NAME = 'playerbot';
   ```

3. **Permission Issues**
   ```sql
   -- Verify user permissions
   SHOW GRANTS FOR 'playerbotuser'@'localhost';
   ```

### Validation Queries

```sql
-- Verify schema compatibility
SELECT
    TABLE_NAME,
    COLUMN_NAME,
    DATA_TYPE,
    IS_NULLABLE
FROM INFORMATION_SCHEMA.COLUMNS
WHERE TABLE_SCHEMA = 'playerbot'
AND TABLE_NAME LIKE 'playerbot%'
ORDER BY TABLE_NAME, ORDINAL_POSITION;

-- Check for any missing indexes
SELECT
    TABLE_NAME,
    INDEX_NAME,
    GROUP_CONCAT(COLUMN_NAME ORDER BY SEQ_IN_INDEX) as Columns
FROM INFORMATION_SCHEMA.STATISTICS
WHERE TABLE_SCHEMA = 'playerbot'
GROUP BY TABLE_NAME, INDEX_NAME
ORDER BY TABLE_NAME, INDEX_NAME;
```

## Performance Considerations

### Recommended Settings

```sql
-- For high-volume bot installations (500+ bots)
SET GLOBAL innodb_buffer_pool_size = 1073741824;  -- 1GB
SET GLOBAL query_cache_size = 67108864;           -- 64MB
SET GLOBAL max_connections = 200;
```

### Monitoring Queries

```sql
-- Monitor table sizes
SELECT
    TABLE_NAME,
    ROUND(((DATA_LENGTH + INDEX_LENGTH) / 1024 / 1024), 2) AS 'Size_MB',
    TABLE_ROWS
FROM information_schema.TABLES
WHERE TABLE_SCHEMA = 'playerbot'
ORDER BY (DATA_LENGTH + INDEX_LENGTH) DESC;

-- Monitor recent activity
SELECT
    event_category,
    COUNT(*) as event_count,
    MAX(event_timestamp) as last_event
FROM playerbot_lifecycle_events
WHERE event_timestamp >= DATE_SUB(NOW(), INTERVAL 1 HOUR)
GROUP BY event_category
ORDER BY event_count DESC;
```

## Migration from Legacy Systems

If migrating from existing `mod-playerbots` installations:

1. **Backup existing data**
   ```bash
   mysqldump -u root -p characters playerbots_names > legacy_names_backup.sql
   ```

2. **Import legacy names** (if desired)
   ```sql
   -- Map legacy names to new structure
   INSERT INTO playerbots_names (name, gender, race_mask)
   SELECT name, gender, 0 as race_mask
   FROM legacy_characters_db.playerbots_names
   WHERE name NOT IN (SELECT name FROM playerbots_names);
   ```

## Version Tracking

The Playerbot module tracks database schema versions to ensure compatibility:

```
[INFO] PlayerbotMigrationMgr: Database version: 1
[INFO] PlayerbotMigrationMgr: Expected source version: 1
[INFO] PlayerbotMigrationMgr: Database schema is up to date
```

If versions mismatch, the server logs detailed instructions for resolution.

## Support

For issues or questions:

1. Check TrinityCore playerbot module documentation
2. Verify the server log for migration status messages
3. Check `playerbot_migrations` table for applied migrations
4. Ensure `BUILD_PLAYERBOT=1` was used during compilation

## File History

- **001_playerbot_base.sql**: Complete schema from database dump (January 2026)
- **Migration System**: PlayerbotMigrationMgr auto-discovers and applies migrations
- **Version Tracking**: PLAYERBOT_DB_VERSION constant in source code
- **Auto-Create**: Database created automatically if MySQL user has CREATE privilege
- **Compatibility**: TrinityCore playerbot-dev branch