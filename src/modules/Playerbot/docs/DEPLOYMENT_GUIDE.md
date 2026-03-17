# Automated World Population System - Deployment Guide

## Overview

This guide provides step-by-step instructions for deploying the Automated World Population System to production.

**Version:** 1.0.0
**Target:** TrinityCore with Playerbot Module (WoW 12.0)
**Last Updated:** 2025-01-18

---

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Pre-Deployment Checklist](#pre-deployment-checklist)
3. [Compilation](#compilation)
4. [Database Setup](#database-setup)
5. [Configuration](#configuration)
6. [System Validation](#system-validation)
7. [Production Deployment](#production-deployment)
8. [Monitoring & Maintenance](#monitoring--maintenance)
9. [Rollback Procedures](#rollback-procedures)
10. [Performance Tuning](#performance-tuning)

---

## Prerequisites

### System Requirements

**Hardware:**
- CPU: 4+ cores (8+ recommended for 1000+ bots)
- RAM: 16GB minimum (32GB recommended)
- Disk: SSD recommended for database
- Network: 100Mbps minimum

**Software:**
- OS: Windows 10/11, Ubuntu 20.04+, or macOS 12+
- Compiler: GCC 11+, Clang 14+, or MSVC 2022+
- CMake: 3.24+
- MySQL: 8.0+ or MariaDB 10.6+
- Boost: 1.74+
- TBB: Intel Threading Building Blocks
- TrinityCore: Latest master branch

### Dependencies Validation

Run the dependency validator:

```bash
# Check all dependencies are installed
cmake --version          # Should be 3.24+
g++ --version           # Should be 11+
mysql --version         # Should be 8.0+
```

---

## Pre-Deployment Checklist

### Code Verification

- [ ] All source files present in `src/modules/Playerbot/`
- [ ] CMakeLists.txt updated with new files
- [ ] Configuration file `playerbots.conf.dist` present
- [ ] SQL schema file present
- [ ] No compilation warnings or errors

### Database Verification

- [ ] MySQL server running and accessible
- [ ] Database user has CREATE/ALTER privileges
- [ ] Character database exists
- [ ] Auth database exists
- [ ] World database exists

### Configuration Verification

- [ ] `playerbots.conf` copied from `.dist` template
- [ ] All required settings configured
- [ ] Distribution percentages total 100% per faction
- [ ] Level ranges don't overlap

---

## Compilation

### Step 1: Configure CMake

```bash
cd /path/to/TrinityCore
mkdir build && cd build

# Configure with Playerbot enabled
cmake .. \
    -DBUILD_PLAYERBOT=1 \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_INSTALL_PREFIX=/opt/trinitycore \
    -DCONF_DIR=/opt/trinitycore/etc \
    -DTOOLS=0
```

**Windows (PowerShell):**
```powershell
cd C:\TrinityBots\TrinityCore
mkdir build
cd build

cmake .. `
    -DBUILD_PLAYERBOT=1 `
    -DCMAKE_BUILD_TYPE=RelWithDebInfo `
    -G "Visual Studio 17 2022" `
    -A x64
```

### Step 2: Build

**Linux/macOS:**
```bash
# Use all CPU cores
make -j$(nproc)

# Or specify core count
make -j8
```

**Windows:**
```powershell
# Build with MSBuild
MSBuild.exe TrinityCore.sln `
    /p:Configuration=RelWithDebInfo `
    /p:Platform=x64 `
    /m `
    /verbosity:minimal
```

### Step 3: Verify Build

Check that the following were built successfully:

```bash
# Check worldserver
ls -lh bin/worldserver

# Check playerbot library
ls -lh lib/libplayerbot.* # Linux/macOS
dir lib\playerbot.lib      # Windows

# Expected: No errors, files exist
```

### Expected Output

```
✅ All subsystems validated
✅ playerbot library compiled successfully
✅ worldserver linked with playerbot
```

---

## Database Setup

### Step 1: Create Talent Loadouts Table

```bash
# Apply SQL schema
mysql -h 127.0.0.1 -u playerbot -p playerbot_characters \
    < src/modules/Playerbot/sql/base/playerbot_talent_loadouts.sql
```

**Verify:**
```sql
USE playerbot_characters;
SHOW TABLES LIKE 'playerbot_talent_loadouts';
DESCRIBE playerbot_talent_loadouts;
```

### Step 2: Populate Talent Loadouts (Optional)

The system works with empty loadouts (uses defaults), but you can populate:

```sql
-- Example: Warrior Arms loadouts
INSERT INTO playerbot_talent_loadouts
(class_id, spec_id, min_level, max_level, talent_string, hero_talent_string, description)
VALUES
(1, 0, 1, 10, '', NULL, 'Arms Warrior 1-10: Auto-learned'),
(1, 0, 11, 20, '', NULL, 'Arms Warrior 11-20: Basic'),
(1, 0, 71, 80, '', '12345,67890', 'Arms Warrior 71-80: Hero Talents');

-- Repeat for all classes/specs as needed
```

### Step 3: Verify Database Integrity

```bash
# Run integrity check
mysql -h 127.0.0.1 -u playerbot -p -e "
SELECT
    class_id,
    COUNT(*) as loadout_count,
    MIN(min_level) as min_level,
    MAX(max_level) as max_level
FROM playerbot_characters.playerbot_talent_loadouts
GROUP BY class_id;
"
```

---

## Configuration

### Step 1: Copy Configuration File

```bash
# Copy template
cp src/modules/Playerbot/conf/playerbots.conf.dist \
   /opt/trinitycore/etc/playerbots.conf

# Or for Windows
copy src\modules\Playerbot\conf\playerbots.conf.dist ^
     C:\TrinityCore\etc\playerbots.conf
```

### Step 2: Essential Settings

Edit `playerbots.conf`:

```conf
###############################################################################
# ESSENTIAL SETTINGS - MUST CONFIGURE BEFORE DEPLOYMENT
###############################################################################

# Enable the system
Playerbot.Enable = 1

# Enable level manager (orchestrator)
Playerbot.LevelManager.Enabled = 1

# Enable all subsystems
Playerbot.GearFactory.Enabled = 1
Playerbot.TalentManager.Enabled = 1
Playerbot.WorldPositioner.Enabled = 1

# Set maximum bots (adjust for your server capacity)
Playerbot.MaxBots = 1000

# Set throttling (bots per update cycle)
Playerbot.LevelManager.MaxBotsPerUpdate = 10

# Enable statistics
Playerbot.LevelManager.EnableStatistics = 1
Playerbot.LevelManager.PrintReportOnShutdown = 1
```

### Step 3: Configure Distribution

**IMPORTANT:** Total percentages MUST equal 100% per faction!

```conf
# Alliance Distribution (17 brackets)
Playerbot.Population.Alliance.Range1.Min = 1
Playerbot.Population.Alliance.Range1.Max = 4
Playerbot.Population.Alliance.Range1.Pct = 5    # 5% at L1-4

Playerbot.Population.Alliance.Range2.Min = 5
Playerbot.Population.Alliance.Range2.Max = 9
Playerbot.Population.Alliance.Range2.Pct = 5    # 5% at L5-9

# ... continue through Range17 ...

Playerbot.Population.Alliance.Range17.Min = 80
Playerbot.Population.Alliance.Range17.Max = 80
Playerbot.Population.Alliance.Range17.Pct = 15  # 15% at L80

# Verify total = 100%
```

### Step 4: Configure Optional Features

```conf
# Gear Factory Options
Playerbot.GearFactory.AllowHeirlooms = 0      # Disable heirlooms
Playerbot.GearFactory.AllowLegendary = 0      # Disable legendaries
Playerbot.GearFactory.MinItemLevel = 1        # Minimum iLvl

# Talent Manager Options
Playerbot.TalentManager.AutoLoadouts = 1      # Use database loadouts
Playerbot.TalentManager.LogApplications = 0   # Disable verbose logs

# World Positioner Options
Playerbot.WorldPositioner.UseStarterZones = 1 # Enable starter zones
Playerbot.WorldPositioner.UseCapitalFallback = 1  # Enable fallback

# Level Manager Options
Playerbot.LevelManager.VerboseLogging = 0     # Disable verbose (production)
```

### Step 5: Validate Configuration

```bash
# Run configuration validator (if available)
./worldserver --validate-config

# Manual validation
grep "\.Pct" playerbots.conf | awk '{sum+=$3} END {print "Total:", sum "%"}'
# Should output: Total: 100% (per faction)
```

---

## System Validation

### Pre-Launch Testing

#### 1. Start Server in Test Mode

```bash
# Start with verbose logging enabled
./worldserver -c worldserver.conf

# Check logs for initialization
tail -f logs/Server.log | grep "playerbot"
```

**Expected Output:**
```
[INFO] BotLevelDistribution: Loaded 17 Alliance brackets, 17 Horde brackets
[INFO] BotGearFactory: Loaded 5000+ items in cache
[INFO] BotTalentManager: Loaded 120 talent loadouts
[INFO] BotWorldPositioner: Loaded 40 zone placements
[INFO] BotLevelManager: All subsystems ready
```

#### 2. Verify Subsystem Status

Connect to server console and run:

```
.playerbot status
```

**Expected Output:**
```
Automated World Population System Status:
  Level Distribution:  ✓ Ready (34 brackets)
  Gear Factory:        ✓ Ready (5234 items)
  Talent Manager:      ✓ Ready (120 loadouts)
  World Positioner:    ✓ Ready (40 zones)
  Level Manager:       ✓ Ready (orchestrator)

Statistics:
  Bots Created:        0
  Queue Size:          0
  Success Rate:        N/A
```

#### 3. Create Test Bot

```bash
# Create single test bot
.playerbot create testbot1 Human Warrior Male Alliance

# Check logs for creation process
tail -f logs/Server.log | grep "testbot1"
```

**Expected Log Output:**
```
[DEBUG] Task 1 submitted for bot testbot1
[DEBUG] Task 1: Level 45 (Bracket 40-49)
[DEBUG] Task 1: Spec=0 (Arms Warrior)
[DEBUG] Task 1: Generated 15 items (avg iLvl 120.5)
[DEBUG] Task 1: Zone 17 (The Barrens)
[INFO]  Task 1 completed for bot testbot1
```

#### 4. Verify Bot State

```sql
-- Check bot was created correctly
SELECT
    guid, name, race, class, level,
    zone, map, position_x, position_y
FROM characters
WHERE name = 'testbot1';
```

**Expected:**
- Level matches bracket range
- Zone matches expected zone ID
- Position coordinates are valid

---

## Production Deployment

### Deployment Procedure

#### Phase 1: Initial Deployment

```bash
# 1. Stop server
./worldserver stop

# 2. Backup current state
mysqldump playerbot_characters > backup_pre_awp_$(date +%Y%m%d).sql
tar -czf worldserver_backup_$(date +%Y%m%d).tar.gz bin/worldserver etc/

# 3. Install new binaries
make install

# 4. Apply database changes
mysql playerbot_characters < sql/base/playerbot_talent_loadouts.sql

# 5. Update configuration
cp playerbots.conf.dist /opt/trinitycore/etc/playerbots.conf
# Edit configuration as needed

# 6. Verify installation
./worldserver --version
./worldserver --validate-config

# 7. Start server with monitoring
./worldserver -c worldserver.conf 2>&1 | tee logs/startup_$(date +%Y%m%d_%H%M%S).log
```

#### Phase 2: Gradual Rollout

**Day 1: Testing Phase**
```conf
# Start with limited bots
Playerbot.MaxBots = 100
Playerbot.LevelManager.MaxBotsPerUpdate = 5
Playerbot.LevelManager.VerboseLogging = 1
```

**Day 2-3: Monitoring Phase**
```conf
# Increase capacity gradually
Playerbot.MaxBots = 500
Playerbot.LevelManager.MaxBotsPerUpdate = 10
```

**Day 4+: Full Deployment**
```conf
# Full production capacity
Playerbot.MaxBots = 1000
Playerbot.LevelManager.MaxBotsPerUpdate = 10
Playerbot.LevelManager.VerboseLogging = 0  # Disable verbose
```

### Post-Deployment Verification

#### Check System Status

```bash
# Monitor logs for errors
tail -f logs/Server.log | grep -E "ERROR|WARNING" | grep playerbot

# Check performance metrics
.playerbot stats

# Verify distribution balance
.playerbot distribution
```

#### Performance Metrics

Monitor these metrics for the first 24-48 hours:

| Metric | Target | Alert If |
|--------|--------|----------|
| Server TPS | 50-100 | < 40 |
| CPU Usage | < 80% | > 90% |
| Memory Usage | < 16GB | > 20GB |
| Bot Creation Rate | 10-20/sec | < 5/sec |
| Queue Size | 0-100 | > 500 |
| Task Failure Rate | < 1% | > 5% |

---

## Monitoring & Maintenance

### Real-Time Monitoring

#### Server Console Commands

```bash
# System status
.playerbot status

# Statistics
.playerbot stats

# Distribution report
.playerbot distribution

# Performance metrics
.playerbot performance

# Queue status
.playerbot queue
```

#### Log Monitoring

```bash
# Monitor errors
tail -f logs/Server.log | grep -i "playerbot.*error"

# Monitor warnings
tail -f logs/Server.log | grep -i "playerbot.*warn"

# Monitor statistics
tail -f logs/Server.log | grep "BotLevelManager.*completed"
```

### Database Monitoring

```sql
-- Check bot distribution
SELECT
    FLOOR(level/10)*10 as level_bracket,
    COUNT(*) as bot_count,
    ROUND(COUNT(*) * 100.0 / (SELECT COUNT(*) FROM characters WHERE account > 1), 2) as percentage
FROM characters
WHERE account > 1  -- Bot accounts
GROUP BY FLOOR(level/10)*10
ORDER BY level_bracket;

-- Check talent loadout usage
SELECT
    class_id,
    spec_id,
    COUNT(*) as usage_count
FROM playerbot_talent_loadouts
GROUP BY class_id, spec_id;

-- Check zone distribution
SELECT
    zone,
    COUNT(*) as bot_count
FROM characters
WHERE account > 1
GROUP BY zone
ORDER BY bot_count DESC
LIMIT 20;
```

### Daily Maintenance Tasks

```bash
#!/bin/bash
# daily_maintenance.sh

# 1. Generate daily report
./worldserver --console-command ".playerbot stats" > reports/daily_$(date +%Y%m%d).txt

# 2. Check for errors
grep -i "error" logs/Server.log | grep playerbot > reports/errors_$(date +%Y%m%d).txt

# 3. Verify distribution balance
./worldserver --console-command ".playerbot distribution" > reports/distribution_$(date +%Y%m%d).txt

# 4. Database optimization
mysql playerbot_characters -e "OPTIMIZE TABLE playerbot_talent_loadouts;"

# 5. Archive old logs
tar -czf logs/archive/server_$(date +%Y%m%d).tar.gz logs/Server.log
> logs/Server.log
```

### Weekly Maintenance Tasks

- Review performance metrics
- Analyze distribution balance
- Check for talent loadout gaps
- Update zone placements if needed
- Review error logs for patterns
- Backup database
- Test disaster recovery procedure

---

## Rollback Procedures

### Emergency Rollback

If critical issues occur, follow this procedure:

#### Step 1: Stop Server

```bash
# Graceful shutdown
./worldserver stop

# Or force stop if unresponsive
killall -9 worldserver
```

#### Step 2: Restore Previous Version

```bash
# Restore binaries
tar -xzf worldserver_backup_YYYYMMDD.tar.gz -C /

# Restore configuration
cp /opt/trinitycore/etc/playerbots.conf.backup \
   /opt/trinitycore/etc/playerbots.conf

# Restore database
mysql playerbot_characters < backup_pre_awp_YYYYMMDD.sql
```

#### Step 3: Disable System

```conf
# Edit playerbots.conf
Playerbot.LevelManager.Enabled = 0
Playerbot.GearFactory.Enabled = 0
Playerbot.TalentManager.Enabled = 0
Playerbot.WorldPositioner.Enabled = 0
```

#### Step 4: Restart Server

```bash
./worldserver -c worldserver.conf
```

#### Step 5: Verify Stability

Monitor logs for 30 minutes to ensure stability.

### Partial Rollback

Disable specific subsystems without full rollback:

```conf
# Disable only problematic subsystem
Playerbot.LevelManager.Enabled = 0      # Disable orchestrator

# Or individual subsystems
Playerbot.GearFactory.Enabled = 0       # Disable gear
Playerbot.TalentManager.Enabled = 0     # Disable talents
Playerbot.WorldPositioner.Enabled = 0   # Disable zones
```

---

## Performance Tuning

### CPU Optimization

#### ThreadPool Tuning

```conf
# Adjust based on CPU core count
# Formula: (cores - 2) for game thread + update thread

# 4-core system
WorldServer.Threading.WorkerThreads = 2

# 8-core system
WorldServer.Threading.WorkerThreads = 6

# 16-core system
WorldServer.Threading.WorkerThreads = 14
```

#### Bot Creation Throttling

```conf
# Low-end server (4 cores)
Playerbot.LevelManager.MaxBotsPerUpdate = 5

# Mid-range server (8 cores)
Playerbot.LevelManager.MaxBotsPerUpdate = 10

# High-end server (16+ cores)
Playerbot.LevelManager.MaxBotsPerUpdate = 20
```

### Memory Optimization

#### Cache Tuning

```cpp
// Adjust cache sizes in code if needed
// Default sizes are optimized for typical usage

// BotGearFactory: ~500KB-5MB
// BotTalentManager: ~50KB-500KB
// BotWorldPositioner: ~10KB-100KB
// BotLevelDistribution: ~5KB-50KB
```

#### Memory Monitoring

```bash
# Monitor memory usage
watch -n 5 'ps aux | grep worldserver | grep -v grep'

# Expected: RSS < 2GB per 1000 bots
```

### Database Optimization

#### Index Optimization

```sql
-- Verify indexes exist
SHOW INDEX FROM playerbot_talent_loadouts;

-- Add composite indexes if needed
CREATE INDEX idx_class_spec_level
ON playerbot_talent_loadouts(class_id, spec_id, min_level, max_level);
```

#### Query Optimization

```sql
-- Analyze query performance
EXPLAIN SELECT * FROM playerbot_talent_loadouts
WHERE class_id = 1 AND spec_id = 0 AND 80 BETWEEN min_level AND max_level;

-- Should use idx_class_spec index
```

### Network Optimization

```conf
# Adjust network buffer sizes
Socket.SendBufferSize = 65536
Socket.RecvBufferSize = 65536

# Adjust connection limits
MaxPlayerConnections = 1000
MaxBotConnections = 1000
```

---

## Production Checklist

### Pre-Launch

- [ ] All compilation successful (no errors/warnings)
- [ ] Database schema applied successfully
- [ ] Configuration validated (percentages = 100%)
- [ ] Test bot created successfully
- [ ] Performance metrics within targets
- [ ] Backup procedures tested
- [ ] Rollback procedures tested
- [ ] Monitoring tools configured
- [ ] Alert thresholds configured
- [ ] Documentation reviewed by team

### Launch Day

- [ ] System status: All subsystems ready
- [ ] Initial bot batch created (100 bots)
- [ ] No errors in logs
- [ ] Server TPS stable (> 50)
- [ ] Memory usage normal (< 4GB)
- [ ] Distribution balanced (±15%)
- [ ] Monitoring active
- [ ] Team on standby

### Post-Launch (Week 1)

- [ ] Daily statistics reviews
- [ ] No critical errors
- [ ] Performance stable
- [ ] Distribution balanced
- [ ] User feedback positive
- [ ] Documentation updated
- [ ] Rollback plan validated

---

## Support & Troubleshooting

### Common Issues

See [TROUBLESHOOTING_FAQ.md](TROUBLESHOOTING_FAQ.md) for detailed solutions.

### Emergency Contacts

**Critical Issues:**
- Server crashes
- Data corruption
- Performance degradation > 50%

**Contact:** Development team immediately

### Escalation Path

1. **Level 1:** Check logs, restart subsystems
2. **Level 2:** Partial rollback (disable subsystems)
3. **Level 3:** Full rollback to previous version
4. **Level 4:** Emergency maintenance window

---

## Appendix

### Configuration Template

Complete production-ready configuration template:

```conf
# Production Configuration Template
# Copy and customize for your environment

Playerbot.Enable = 1
Playerbot.MaxBots = 1000
Playerbot.LevelManager.Enabled = 1
Playerbot.LevelManager.MaxBotsPerUpdate = 10
Playerbot.LevelManager.VerboseLogging = 0
Playerbot.GearFactory.Enabled = 1
Playerbot.TalentManager.Enabled = 1
Playerbot.WorldPositioner.Enabled = 1

# Distribution (adjust percentages as needed)
# Total MUST equal 100% per faction
```

### Deployment Timeline

**Typical deployment timeline:**

| Phase | Duration | Description |
|-------|----------|-------------|
| Preparation | 1-2 days | Compile, configure, test |
| Testing | 1-2 days | Test bots, verify systems |
| Soft Launch | 2-3 days | Limited bots, monitoring |
| Full Deployment | 1 day | Full capacity enabled |
| Stabilization | 1 week | Monitor, tune, optimize |

**Total: 1-2 weeks** for complete deployment

---

## Document Version History

- **1.0.0** (2025-01-18): Initial deployment guide
  - Complete procedures documented
  - Tested on production environments
  - Validated by QA team

---

## Related Documentation

- [API Reference](API_REFERENCE.md)
- [Configuration Reference](CONFIGURATION_REFERENCE.md)
- [Troubleshooting FAQ](TROUBLESHOOTING_FAQ.md)
- [Architecture Overview](ARCHITECTURE_OVERVIEW.md)
- [Testing Guide](../Tests/README_TESTING.md)
