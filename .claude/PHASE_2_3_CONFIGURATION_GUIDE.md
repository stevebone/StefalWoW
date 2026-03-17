# Phase 2/3 Configuration Guide - Adaptive Throttling System

**Version:** 1.0
**Date:** 2025-10-29
**Target:** TrinityCore PlayerBot Module (Phases 2 & 3)
**Quality:** Enterprise-Grade Production Configuration

---

## Table of Contents

1. [Overview](#overview)
2. [Configuration File Location](#configuration-file-location)
3. [Startup Orchestrator Configuration](#startup-orchestrator-configuration)
4. [Adaptive Throttler Configuration](#adaptive-throttler-configuration)
5. [Circuit Breaker Configuration](#circuit-breaker-configuration)
6. [Resource Monitor Thresholds](#resource-monitor-thresholds)
7. [Recommended Configurations](#recommended-configurations)
8. [Troubleshooting](#troubleshooting)
9. [Performance Tuning](#performance-tuning)

---

## Overview

The Phase 2/3 Adaptive Throttling System introduces sophisticated bot spawn rate control based on:
- **Server resource monitoring** (CPU, memory, DB connections)
- **Circuit breaker protection** (prevents cascade failures)
- **Priority-based queueing** (CRITICAL > HIGH > NORMAL > LOW)
- **Phased startup orchestration** (graduated bot spawning)

All configuration values are loaded from `playerbots.conf` and can be tuned without recompilation.

---

## Configuration File Location

**File:** `<TrinityCore>/etc/playerbots.conf`

**Note:** After modifying configuration values, restart the worldserver for changes to take effect.

---

## Startup Orchestrator Configuration

### Master Controls

```ini
###############################################################################
# PHASE 2: STARTUP ORCHESTRATOR - Phased Bot Spawning
###############################################################################
#
# Controls graduated bot spawning during server startup to prevent resource
# spikes and database overload. Spawns bots in 4 priority-based phases over
# 30 minutes.
#

# Enable phased startup (strongly recommended for >500 bots)
#   Default: true
Playerbot.Startup.EnablePhased = true

# Enable parallel database character loading during spawning
#   Default: false (not yet implemented)
Playerbot.Startup.EnableParallelLoading = false

# Maximum concurrent database character loads
#   Default: 10
#   Range: 1-50
Playerbot.Startup.MaxConcurrentDbLoads = 10

# Initial delay before starting Phase 1 (seconds)
#   Default: 5
#   Range: 0-60
#   Recommendation: 5-10 seconds to allow server initialization
Playerbot.Startup.InitialDelaySeconds = 5
```

### Phase 1: CRITICAL Bots (0-2 minutes)

**Target:** Guild leaders, raid leaders
**Priority:** CRITICAL
**Spawn Rate:** 15-20 bots/sec (150% of base rate)

```ini
# Phase 1: Target bot count (guild leaders, raid leaders)
#   Default: 100
#   Range: 10-500
#   Recommendation: ~2% of total bot population
Playerbot.Startup.Phase1.TargetBots = 100

# Phase 1: Spawn rate multiplier
#   Default: 1.5 (150% of normal rate)
#   Range: 1.0-3.0
#   Recommendation: 1.5-2.0 for priority bots
Playerbot.Startup.Phase1.RateMultiplier = 1.5
```

### Phase 2: HIGH PRIORITY Bots (2-5 minutes)

**Target:** Party members, friends
**Priority:** HIGH
**Spawn Rate:** 10-15 bots/sec (120% of base rate)

```ini
# Phase 2: Target bot count (party members, friends)
#   Default: 500
#   Range: 50-2000
#   Recommendation: ~10% of total bot population
Playerbot.Startup.Phase2.TargetBots = 500

# Phase 2: Spawn rate multiplier
#   Default: 1.2 (120% of normal rate)
#   Range: 1.0-2.0
#   Recommendation: 1.2-1.5 for social bots
Playerbot.Startup.Phase2.RateMultiplier = 1.2
```

### Phase 3: NORMAL Bots (5-15 minutes)

**Target:** Standard bots (bulk population)
**Priority:** NORMAL
**Spawn Rate:** 5-10 bots/sec (100% of base rate)

```ini
# Phase 3: Target bot count (standard bots)
#   Default: 3000
#   Range: 500-8000
#   Recommendation: ~60% of total bot population
Playerbot.Startup.Phase3.TargetBots = 3000

# Phase 3: Spawn rate multiplier
#   Default: 1.0 (100% - normal rate)
#   Range: 0.8-1.5
#   Recommendation: 1.0 for bulk spawning
Playerbot.Startup.Phase3.RateMultiplier = 1.0
```

### Phase 4: LOW PRIORITY Bots (15-30 minutes)

**Target:** Background/filler bots
**Priority:** LOW
**Spawn Rate:** 2-5 bots/sec (80% of base rate)

```ini
# Phase 4: Target bot count (background bots)
#   Default: 1400
#   Range: 0-5000
#   Recommendation: ~28% of total bot population
Playerbot.Startup.Phase4.TargetBots = 1400

# Phase 4: Spawn rate multiplier
#   Default: 0.8 (80% of normal rate)
#   Range: 0.5-1.0
#   Recommendation: 0.8-1.0 for background bots
Playerbot.Startup.Phase4.RateMultiplier = 0.8
```

**Phase Duration Notes:**
- Minimum duration is enforced (prevents skipping phases too quickly)
- Maximum duration is enforced (prevents phases from stalling)
- Early transition allowed if target quota met and queue empty

---

## Adaptive Throttler Configuration

### Spawn Rate Control

```ini
###############################################################################
# PHASE 2: ADAPTIVE THROTTLER - Dynamic Spawn Rate Control
###############################################################################
#
# Controls bot spawn rate based on server resource pressure, circuit breaker
# state, and burst prevention. Automatically adjusts spawn interval from
# 50ms (20 bots/sec) to 5000ms (0.2 bots/sec) based on conditions.
#

# Enable adaptive throttling (highly recommended)
#   Default: true
Playerbot.Throttler.EnableAdaptive = true

# Enable circuit breaker integration
#   Default: true
Playerbot.Throttler.EnableCircuitBreaker = true

# Enable burst prevention (limits rapid spawning)
#   Default: true
Playerbot.Throttler.EnableBurstPrevention = true
```

### Spawn Interval Configuration

```ini
# Base spawn interval (milliseconds) under normal conditions
#   Default: 100 (10 bots/sec)
#   Range: 50-500
#   Recommendation: 100ms for balanced performance
Playerbot.Throttler.BaseSpawnIntervalMs = 100

# Minimum spawn interval (fastest spawn rate)
#   Default: 50 (20 bots/sec)
#   Range: 50-200
#   Recommendation: 50ms for peak performance
Playerbot.Throttler.MinSpawnIntervalMs = 50

# Maximum spawn interval (slowest spawn rate)
#   Default: 5000 (0.2 bots/sec)
#   Range: 1000-10000
#   Recommendation: 5000ms for critical pressure
Playerbot.Throttler.MaxSpawnIntervalMs = 5000
```

### Resource Pressure Multipliers

**How it works:** Spawn interval is multiplied based on resource pressure level.

**Formula:** `actual_interval = base_interval / multiplier`

```ini
# NORMAL pressure multiplier (<60% CPU, <70% memory)
#   Default: 1.0 (100% spawn rate)
#   Range: 0.8-1.5
#   Recommendation: 1.0 (no throttling)
Playerbot.Throttler.NormalPressureMultiplier = 1.0

# ELEVATED pressure multiplier (60-75% CPU, 70-80% memory)
#   Default: 0.5 (50% spawn rate)
#   Range: 0.3-0.8
#   Recommendation: 0.5 (moderate throttling)
Playerbot.Throttler.ElevatedPressureMultiplier = 0.5

# HIGH pressure multiplier (75-85% CPU, 80-90% memory)
#   Default: 0.25 (25% spawn rate)
#   Range: 0.1-0.5
#   Recommendation: 0.25 (aggressive throttling)
Playerbot.Throttler.HighPressureMultiplier = 0.25

# CRITICAL pressure multiplier (>85% CPU, >90% memory)
#   Default: 0.0 (pause spawning)
#   Range: 0.0-0.2
#   Recommendation: 0.0 (full pause)
Playerbot.Throttler.CriticalPressureMultiplier = 0.0
```

### Burst Prevention

**Purpose:** Prevents rapid spawning bursts that could spike CPU/memory.

```ini
# Burst window duration (milliseconds)
#   Default: 10000 (10 seconds)
#   Range: 5000-30000
#   Recommendation: 10000ms (10 second sliding window)
Playerbot.Throttler.BurstWindowMs = 10000

# Maximum spawns allowed per burst window
#   Default: 50
#   Range: 20-200
#   Recommendation: 50 (5 bots/sec average over 10s)
Playerbot.Throttler.MaxBurstsPerWindow = 50
```

**Burst Prevention Logic:**
- Tracks spawns in a 10-second sliding window
- If >50 spawns in last 10 seconds, throttles to prevent burst
- Prevents resource spikes from rapid spawning

---

## Circuit Breaker Configuration

### Failure Protection

```ini
###############################################################################
# PHASE 2: CIRCUIT BREAKER - Failure Rate Protection
###############################################################################
#
# Implements circuit breaker pattern to prevent cascade failures when spawn
# failure rate exceeds threshold. Automatically blocks spawning and enters
# recovery mode when too many spawn attempts fail.
#
# States:
#   CLOSED: Normal operation, allowing spawn attempts
#   OPEN: Failure threshold exceeded, blocking all spawns
#   HALF_OPEN: Recovery testing, allowing limited spawns
#

# Failure rate threshold to OPEN circuit (percent)
#   Default: 10.0% (if >10% of spawns fail, open circuit)
#   Range: 5.0-50.0
#   Recommendation: 10.0% for production
Playerbot.CircuitBreaker.OpenThreshold = 10.0

# Success rate threshold to CLOSE circuit (percent)
#   Default: 5.0% (if <5% of spawns fail, close circuit)
#   Range: 0.0-20.0
#   Recommendation: 5.0% for production
Playerbot.CircuitBreaker.CloseThreshold = 5.0
```

### Circuit Timing

```ini
# Cooldown duration before entering HALF_OPEN state (seconds)
#   Default: 60 (1 minute)
#   Range: 30-300
#   Recommendation: 60 seconds for quick recovery
Playerbot.CircuitBreaker.CooldownSeconds = 60

# Recovery duration before fully closing circuit (seconds)
#   Default: 120 (2 minutes)
#   Range: 60-600
#   Recommendation: 120 seconds for stability
Playerbot.CircuitBreaker.RecoverySeconds = 120

# Sliding window duration for failure rate calculation (seconds)
#   Default: 60 (1 minute)
#   Range: 30-300
#   Recommendation: 60 seconds for balanced detection
Playerbot.CircuitBreaker.WindowSeconds = 60

# Minimum spawn attempts before circuit can open
#   Default: 10
#   Range: 5-100
#   Recommendation: 10 to avoid premature triggering
Playerbot.CircuitBreaker.MinimumAttempts = 10
```

**Circuit Breaker State Transitions:**
1. **CLOSED → OPEN:** Failure rate >10% (blocks all spawns)
2. **OPEN → HALF_OPEN:** After 60 second cooldown (tests recovery)
3. **HALF_OPEN → CLOSED:** Failure rate <5% (resumes normal operation)
4. **HALF_OPEN → OPEN:** Failure rate >10% (back to blocking)

---

## Resource Monitor Thresholds

**Note:** Resource thresholds are currently **hardcoded** in `ResourceMetrics::GetPressureLevel()`.

**Future Enhancement:** Configuration values will be added in a future update.

### Current Hardcoded Thresholds

| Pressure Level | CPU Threshold | Memory Threshold |
|----------------|---------------|------------------|
| NORMAL         | <60%          | <70%             |
| ELEVATED       | 60-75%        | 70-80%           |
| HIGH           | 75-85%        | 80-90%           |
| CRITICAL       | >85%          | >90%             |

**Behavior:**
- Uses **30-second CPU average** (smooths spikes)
- Uses **current memory usage** (immediate response)
- Returns **worst pressure level** (CPU or memory)
- DB connection pressure threshold: 80% of pool size

---

## Recommended Configurations

### Small Server (100-500 bots)

**Hardware:** 4 cores, 8GB RAM, SSD

```ini
# Startup Orchestrator
Playerbot.Startup.EnablePhased = true
Playerbot.Startup.Phase1.TargetBots = 20
Playerbot.Startup.Phase2.TargetBots = 80
Playerbot.Startup.Phase3.TargetBots = 300
Playerbot.Startup.Phase4.TargetBots = 100

# Throttler
Playerbot.Throttler.BaseSpawnIntervalMs = 100
Playerbot.Throttler.MinSpawnIntervalMs = 50
Playerbot.Throttler.MaxSpawnIntervalMs = 3000
Playerbot.Throttler.NormalPressureMultiplier = 1.0
Playerbot.Throttler.ElevatedPressureMultiplier = 0.6

# Circuit Breaker
Playerbot.CircuitBreaker.OpenThreshold = 15.0
Playerbot.CircuitBreaker.CloseThreshold = 8.0
Playerbot.CircuitBreaker.CooldownSeconds = 45
```

### Medium Server (500-2000 bots)

**Hardware:** 8 cores, 16GB RAM, NVMe SSD

```ini
# Startup Orchestrator
Playerbot.Startup.EnablePhased = true
Playerbot.Startup.Phase1.TargetBots = 50
Playerbot.Startup.Phase2.TargetBots = 250
Playerbot.Startup.Phase3.TargetBots = 1200
Playerbot.Startup.Phase4.TargetBots = 500

# Throttler
Playerbot.Throttler.BaseSpawnIntervalMs = 80
Playerbot.Throttler.MinSpawnIntervalMs = 50
Playerbot.Throttler.MaxSpawnIntervalMs = 4000
Playerbot.Throttler.NormalPressureMultiplier = 1.0
Playerbot.Throttler.ElevatedPressureMultiplier = 0.5

# Circuit Breaker
Playerbot.CircuitBreaker.OpenThreshold = 12.0
Playerbot.CircuitBreaker.CloseThreshold = 6.0
Playerbot.CircuitBreaker.CooldownSeconds = 60
```

### Large Server (2000-5000 bots) ⭐ **TARGET**

**Hardware:** 16+ cores, 32GB+ RAM, Enterprise NVMe, 10Gbps network

```ini
# Startup Orchestrator
Playerbot.Startup.EnablePhased = true
Playerbot.Startup.Phase1.TargetBots = 100
Playerbot.Startup.Phase2.TargetBots = 500
Playerbot.Startup.Phase3.TargetBots = 3000
Playerbot.Startup.Phase4.TargetBots = 1400

# Throttler
Playerbot.Throttler.BaseSpawnIntervalMs = 100
Playerbot.Throttler.MinSpawnIntervalMs = 50
Playerbot.Throttler.MaxSpawnIntervalMs = 5000
Playerbot.Throttler.NormalPressureMultiplier = 1.0
Playerbot.Throttler.ElevatedPressureMultiplier = 0.5
Playerbot.Throttler.HighPressureMultiplier = 0.25
Playerbot.Throttler.CriticalPressureMultiplier = 0.0

# Circuit Breaker
Playerbot.CircuitBreaker.OpenThreshold = 10.0
Playerbot.CircuitBreaker.CloseThreshold = 5.0
Playerbot.CircuitBreaker.CooldownSeconds = 60
Playerbot.CircuitBreaker.RecoverySeconds = 120
```

---

## Troubleshooting

### Problem: Bots spawn too slowly

**Symptoms:** Startup takes >45 minutes for 5000 bots

**Solutions:**
1. **Increase phase target bots:**
   ```ini
   Playerbot.Startup.Phase3.TargetBots = 4000  # Up from 3000
   ```

2. **Increase spawn rate multipliers:**
   ```ini
   Playerbot.Startup.Phase1.RateMultiplier = 2.0  # Up from 1.5
   Playerbot.Startup.Phase2.RateMultiplier = 1.5  # Up from 1.2
   ```

3. **Reduce base spawn interval:**
   ```ini
   Playerbot.Throttler.BaseSpawnIntervalMs = 80  # Down from 100
   Playerbot.Throttler.MinSpawnIntervalMs = 40  # Down from 50
   ```

### Problem: Server crashes during startup

**Symptoms:** Crash during bot spawning, "out of memory" errors

**Solutions:**
1. **Enable and tune phased startup:**
   ```ini
   Playerbot.Startup.EnablePhased = true
   Playerbot.Startup.Phase1.TargetBots = 50  # Reduce from 100
   ```

2. **Increase spawn intervals (slow down):**
   ```ini
   Playerbot.Throttler.BaseSpawnIntervalMs = 150  # Up from 100
   Playerbot.Throttler.MaxSpawnIntervalMs = 8000  # Up from 5000
   ```

3. **Lower pressure multipliers (more aggressive throttling):**
   ```ini
   Playerbot.Throttler.ElevatedPressureMultiplier = 0.3  # Down from 0.5
   Playerbot.Throttler.HighPressureMultiplier = 0.1  # Down from 0.25
   ```

### Problem: Circuit breaker triggers too often

**Symptoms:** Logs show frequent "Circuit OPEN" messages

**Solutions:**
1. **Increase failure threshold:**
   ```ini
   Playerbot.CircuitBreaker.OpenThreshold = 15.0  # Up from 10.0
   ```

2. **Increase minimum attempts:**
   ```ini
   Playerbot.CircuitBreaker.MinimumAttempts = 20  # Up from 10
   ```

3. **Investigate spawn failures:** Check logs for actual failure causes
   - Database connection issues?
   - Account/character corruption?
   - Race conditions?

### Problem: Spawning pauses for long periods

**Symptoms:** No bots spawning for 1-2 minutes, then resumes

**Possible Causes:**
1. **Resource pressure:** Check CPU/memory usage
2. **Circuit breaker:** Check if circuit is OPEN
3. **Burst prevention:** Too many spawns in burst window

**Solutions:**
1. **Check resource pressure logs:**
   ```
   grep "pressure:" worldserver.log | tail -20
   ```

2. **Adjust burst prevention:**
   ```ini
   Playerbot.Throttler.MaxBurstsPerWindow = 100  # Up from 50
   ```

3. **Disable circuit breaker temporarily (testing only):**
   ```ini
   Playerbot.Throttler.EnableCircuitBreaker = false
   ```

---

## Performance Tuning

### Maximizing Spawn Speed (Fast Startup)

**Goal:** Spawn 5000 bots in <20 minutes

```ini
# Aggressive phase targets
Playerbot.Startup.Phase1.TargetBots = 150
Playerbot.Startup.Phase2.TargetBots = 800
Playerbot.Startup.Phase3.TargetBots = 3500
Playerbot.Startup.Phase4.TargetBots = 550

# Fast spawn rates
Playerbot.Startup.Phase1.RateMultiplier = 2.0
Playerbot.Startup.Phase2.RateMultiplier = 1.8
Playerbot.Startup.Phase3.RateMultiplier = 1.5
Playerbot.Startup.Phase4.RateMultiplier = 1.0

# Fast throttler
Playerbot.Throttler.BaseSpawnIntervalMs = 50
Playerbot.Throttler.MinSpawnIntervalMs = 30

# Lenient circuit breaker
Playerbot.CircuitBreaker.OpenThreshold = 20.0
```

**Warning:** May cause high CPU/memory spikes. Monitor carefully.

### Maximizing Stability (Safe Startup)

**Goal:** Guaranteed stability, slower startup acceptable

```ini
# Conservative phase targets
Playerbot.Startup.Phase1.TargetBots = 50
Playerbot.Startup.Phase2.TargetBots = 250
Playerbot.Startup.Phase3.TargetBots = 2500
Playerbot.Startup.Phase4.TargetBots = 2200

# Gradual spawn rates
Playerbot.Startup.Phase1.RateMultiplier = 1.0
Playerbot.Startup.Phase2.RateMultiplier = 1.0
Playerbot.Startup.Phase3.RateMultiplier = 0.8
Playerbot.Startup.Phase4.RateMultiplier = 0.5

# Conservative throttler
Playerbot.Throttler.BaseSpawnIntervalMs = 150
Playerbot.Throttler.NormalPressureMultiplier = 0.8
Playerbot.Throttler.ElevatedPressureMultiplier = 0.4

# Strict circuit breaker
Playerbot.CircuitBreaker.OpenThreshold = 5.0
Playerbot.CircuitBreaker.MinimumAttempts = 5
```

**Result:** Slower startup (~45 minutes), but rock-solid stability.

---

## Configuration Validation

### Quick Validation Checklist

✅ **Total Phase Targets:** Should equal desired total bot count
```
Phase1 + Phase2 + Phase3 + Phase4 = Total Bots
100 + 500 + 3000 + 1400 = 5000 ✅
```

✅ **Spawn Interval Range:** `MinSpawnIntervalMs < BaseSpawnIntervalMs < MaxSpawnIntervalMs`
```
50 < 100 < 5000 ✅
```

✅ **Circuit Thresholds:** `CloseThreshold < OpenThreshold`
```
5.0 < 10.0 ✅
```

✅ **Pressure Multipliers:** Should decrease as pressure increases
```
Normal (1.0) > Elevated (0.5) > High (0.25) > Critical (0.0) ✅
```

---

## Logging and Monitoring

### Key Log Messages

**Startup Progress:**
```
🔴 Startup phase transition: IDLE → CRITICAL_BOTS (bots spawned: 0)
🟠 Startup phase transition: CRITICAL_BOTS → HIGH_PRIORITY (bots spawned: 100)
🟢 Startup phase transition: HIGH_PRIORITY → NORMAL_BOTS (bots spawned: 600)
🔵 Startup phase transition: NORMAL_BOTS → LOW_PRIORITY (bots spawned: 3600)
✅ Startup phase transition: LOW_PRIORITY → COMPLETED (bots spawned: 5000)
```

**Throttling Active:**
```
Phase 2 throttling active - spawn deferred (pressure: 2, circuit: 0, phase: 3)
```
- `pressure: 2` = HIGH pressure
- `circuit: 0` = CLOSED (ok)
- `phase: 3` = NORMAL_BOTS phase

**Circuit Breaker:**
```
Circuit breaker transitioned: CLOSED → OPEN (failure rate: 12.5%)
Circuit breaker transitioned: OPEN → HALF_OPEN (testing recovery)
Circuit breaker transitioned: HALF_OPEN → CLOSED (recovery successful)
```

### Monitoring Commands

**Check current startup phase:**
```
.bot debug phase
```

**Check resource pressure:**
```
.bot debug resources
```

**Check circuit breaker state:**
```
.bot debug circuit
```

---

## Summary

The Phase 2/3 Adaptive Throttling System provides enterprise-grade spawn rate control with:
- ✅ **Phased startup** (4 priority-based phases)
- ✅ **Resource monitoring** (CPU/memory/DB adaptive)
- ✅ **Circuit breaker** (failure rate protection)
- ✅ **Burst prevention** (prevents resource spikes)
- ✅ **Full configurability** (tune without recompilation)

**Default configuration targets:**
- 5000 concurrent bots
- 30-minute startup
- <10MB memory per bot
- <0.1% CPU per bot
- 99.9% uptime

---

**Configuration Support:** See `.claude/PHASE_2_IMPLEMENTATION_COMPLETE.md` for technical details
**Version:** Phase 2/3 (Complete)
**Status:** Production-Ready ✅
