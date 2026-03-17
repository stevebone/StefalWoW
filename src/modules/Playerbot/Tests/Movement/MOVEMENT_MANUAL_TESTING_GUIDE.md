# BotMovement System - Manual Testing Guide

## Overview

This guide provides step-by-step manual testing procedures for validating the BotMovementController system in-game. These tests complement the automated unit tests and verify real-world behavior.

**Task 6 Coverage:**
- Water Test (Swimming State)
- Wall Test (Collision Detection)
- Cliff Test (Void Detection)
- Stuck Test (Recovery System)
- Performance Test (5000 Bots)

## Prerequisites

### Configuration Setup

Before testing, ensure the BotMovement system is properly configured in `playerbots.conf`:

```ini
# Enable the system
BotMovement.Enable = 1

# Enable all validations for full testing
BotMovement.Validation.Ground = 1
BotMovement.Validation.Collision = 1
BotMovement.Validation.Liquid = 1

# Enable stuck detection
BotMovement.StuckDetection.Enable = 1
BotMovement.StuckDetection.Threshold = 2.0
BotMovement.StuckDetection.RecoveryMaxAttempts = 5

# Enable debug logging
BotMovement.Debug.LogStateChanges = 1
BotMovement.Debug.LogValidationFailures = 1
```

### Log Monitoring

Enable movement-related logging:

```ini
Logger.movement.bot=6,Console Server
Logger.movement.bot.state=6,Console Server
```

Start the server and monitor logs for:
- `[movement.bot]` - General movement events
- `[movement.bot.state]` - State transitions

## Test 1: Water Test (Swimming State Validation)

### Objective
Verify that bots correctly detect water and transition to swimming state, with proper movement flags applied.

### Test Locations

**Elwynn Forest Lake**:
- Position: `-9449, -2062, 62` (in lake)
- Deep water: Good for swimming test
- Nearby land: Good for swim-to-land transition

**Durotar Coast**:
- Position: `270, -4760, 10` (ocean)
- Shallow to deep transition available

### Test Procedure

1. **Spawn Test Bot**
   ```
   .bot add Swimmer 1 warrior  # Create level 1 warrior bot
   .appear Swimmer             # Teleport to bot
   ```

2. **Position on Land Near Water**
   ```
   .go xyz -9400 -2000 60 0    # Elwynn Forest shore
   .appear Swimmer             # Bot follows
   ```

3. **Command Bot Into Water**
   ```
   .bot command Swimmer move   # Command movement
   .go xyz -9449 -2062 62 0    # Click location in lake
   ```

4. **Monitor State Transition**

   **Expected Log Output:**
   ```
   [movement.bot.state] BotMovementController: Auto-transition for Swimmer from 0 to 1
   [movement.bot] DetermineAppropriateState: IsSwimmingRequired = true, transitioning to Swimming
   ```

5. **Verify Swimming Behavior**
   - [ ] Bot should smoothly enter water without hopping
   - [ ] Swimming animation should play
   - [ ] Bot should maintain consistent depth
   - [ ] Movement speed should reflect swimming speed

6. **Test Water-to-Land Transition**
   ```
   .go xyz -9400 -2000 60 0    # Back to shore
   # Command bot to follow
   ```

   **Expected Log Output:**
   ```
   [movement.bot.state] BotMovementController: Auto-transition for Swimmer from 1 to 0
   ```

7. **Validation Checklist**
   - [ ] State transitions from Ground/Idle to Swimming
   - [ ] MOVEMENTFLAG_SWIMMING is set (check debug output)
   - [ ] No underwater falling or bouncing
   - [ ] Clean transition back to ground movement
   - [ ] No stuck detection triggered during valid swimming

### Known Issues to Watch For
- ⚠️ Bot bounces at water surface → Swimming detection threshold issue
- ⚠️ Bot walks on water bottom → Swimming flags not applied
- ⚠️ Stuck detection triggers in water → Position threshold too strict

---

## Test 2: Wall Test (Collision Detection)

### Objective
Verify that validated pathfinding detects walls and prevents bots from attempting to walk through solid objects.

### Test Locations

**Stormwind Keep Interior**:
- Position: `-8400, 340, 120` (throne room)
- Thick stone walls
- Multiple doorways for pathfinding

**Orgrimmar Valley of Honor**:
- Position: `1574, -4360, 16`
- Buildings with walls
- Open pathways for alternate routes

### Test Procedure

1. **Spawn Test Bot**
   ```
   .bot add Collider 10 paladin
   .appear Collider
   ```

2. **Position Bot on One Side of Wall**
   ```
   .go xyz -8400 340 120 0 0   # Stormwind throne room
   .appear Collider
   ```

3. **Command Bot to Target Through Wall**
   ```
   # Target a position on opposite side of thick wall
   .go xyz -8410 340 120 0 0   # Other side of wall
   # Use waypoint or follow command to make bot path through wall
   ```

4. **Monitor Validation**

   **Expected Log Output:**
   ```
   [movement.bot] BotMovementController: Path validation failed for Collider: Collision detected
   [movement.bot] ValidatedPathGenerator: VMAP collision check failed at waypoint 5
   ```

5. **Verify Collision Handling**
   - [ ] Bot should stop at wall, not attempt to walk through
   - [ ] MoveToPosition() should return false
   - [ ] Bot should wait or idle, not spam movement attempts
   - [ ] If alternate route exists, bot should find it

6. **Test Valid Pathfinding Around Obstacle**
   ```
   # Position bot where valid path exists (through doorway)
   .go xyz -8395 350 120 0 0   # Position with doorway access
   # Command to destination
   ```

   **Expected:**
   - [ ] Path validation succeeds
   - [ ] Bot takes valid route through doorway
   - [ ] No collision warnings in log

7. **Validation Checklist**
   - [ ] Direct wall paths are rejected
   - [ ] Collision validator called and functional
   - [ ] VMAP data is loaded and accessible
   - [ ] Alternate routes are found when available
   - [ ] No false positives (valid paths rejected)

### Known Issues to Watch For
- ⚠️ Bot walks through walls → VMAP not loaded or collision check disabled
- ⚠️ All paths rejected → Overly strict collision validation
- ⚠️ Bot stuck at wall → No fallback or recovery

---

## Test 3: Cliff Test (Void Detection)

### Objective
Verify that ground validation detects void areas and prevents bots from walking off cliffs or into empty space.

### Test Locations

**Thousand Needles Cliffs**:
- Position: `-4800, -2000, 200` (high mesa)
- Dramatic height changes
- Void detection critical

**Thunder Bluff Elevators**:
- Position: `-1280, 120, 130` (edge of lift)
- Test void detection at structure edges

### Test Procedure

1. **Spawn Test Bot**
   ```
   .bot add Cliffdiver 20 druid
   .appear Cliffdiver
   ```

2. **Position Bot Near Cliff Edge**
   ```
   .go xyz -4800 -2000 200 0 0  # Thousand Needles mesa
   .appear Cliffdiver
   ```

3. **Command Bot Past Cliff Edge**
   ```
   # Target a position in the air beyond cliff
   .go xyz -4800 -2050 50 0 0   # 150 yards down in void
   # Command bot to move there
   ```

4. **Monitor Ground Validation**

   **Expected Log Output:**
   ```
   [movement.bot] ValidatedPathGenerator: Ground validation failed at waypoint 3
   [movement.bot] GroundValidator: Void detected - height check failed (z=-500)
   [movement.bot] BotMovementController: Path validation failed for Cliffdiver: Ground unsafe
   ```

5. **Verify Cliff Behavior**
   - [ ] Bot should stop at cliff edge
   - [ ] No falling off cliff
   - [ ] MoveToPosition() returns false
   - [ ] No repeated path attempts

6. **Test Valid Downward Movement**
   ```
   # Find location with valid downward ramp/path
   # Command bot to lower elevation via valid route
   ```

   **Expected:**
   - [ ] Valid downward paths are accepted
   - [ ] Ramps and stairs work correctly
   - [ ] Only actual void is rejected

7. **Test Falling State Trigger**
   ```
   # Manually teleport bot into air
   .go xyz -4800 -2000 300 0 0  # 100y above ground
   .appear Cliffdiver
   ```

   **Expected Log Output:**
   ```
   [movement.bot.state] BotMovementController: Auto-transition for Cliffdiver from 0 to 2
   [movement.bot] DetermineAppropriateState: Not on ground, entering Falling state
   ```

8. **Validation Checklist**
   - [ ] Void areas correctly detected
   - [ ] Cliff edges respected
   - [ ] Valid slopes accepted
   - [ ] Falling state triggers when airborne
   - [ ] Bot lands safely without stuck detection

### Known Issues to Watch For
- ⚠️ Bot walks off cliff → Ground validation disabled or heightmap missing
- ⚠️ False positives on slopes → Validation too strict
- ⚠️ Bot stuck at cliff edge forever → No path rejection feedback

---

## Test 4: Stuck Test (Recovery System)

### Objective
Verify that stuck detection correctly identifies immobile bots and executes appropriate recovery strategies.

### Test Locations

**Stormwind Cathedral Corner**:
- Position: `-8600, 860, 100` (tight corner)
- Easy to create stuck scenario

**Durotar Cave Dead-End**:
- Position: `450, -4420, 20` (inside Burning Blade Coven)
- Narrow passages

### Test Procedure

1. **Spawn Test Bot**
   ```
   .bot add Stuckbot 15 hunter
   .appear Stuckbot
   ```

2. **Create Stuck Scenario Method 1: Tight Corner**
   ```
   # Teleport bot into very tight corner
   .go xyz -8600 860 100 0 0
   .appear Stuckbot

   # Command bot to move into wall repeatedly
   .go xyz -8605 860 100 0 0    # Position blocked by wall
   # Use /follow or move commands
   ```

3. **Monitor Stuck Detection**

   **Expected Log Output (after 3 seconds):**
   ```
   [movement.bot] StuckDetector: Bot moving but position unchanged for 3000ms
   [movement.bot] StuckDetector: Stuck detected - total distance: 0.5 yards in 3 seconds
   [movement.bot.state] BotMovementController: Auto-transition for Stuckbot from 0 to 4
   ```

4. **Monitor Recovery Attempts**

   **Level 1 Recovery (Reverse Movement):**
   ```
   [movement.bot] HandleStuckState: Attempting recovery for Stuckbot (attempt 1/5)
   [movement.bot] RecoveryStrategies: Level 1 - Reversing 5 yards
   ```

5. **Verify Recovery Behavior**
   - [ ] Stuck detected after ~3 seconds
   - [ ] State transitions to MovementStateType::Stuck
   - [ ] Recovery Level 1 attempted (reverse)
   - [ ] If Level 1 fails, Level 2 attempted (jump)
   - [ ] If Level 2 fails, Level 3 attempted (teleport)

6. **Test Recovery Success**
   ```
   [movement.bot] RecoveryStrategies: Recovery succeeded (level 1): Moved to safe position
   [movement.bot] StuckDetector: Reset - recovery successful
   [movement.bot.state] BotMovementController: Auto-transition for Stuckbot from 4 to 0
   ```

7. **Test Recovery Escalation**
   ```
   # Create scenario where Level 1 fails
   # (Very tight space, reverse still blocked)
   ```

   **Expected:**
   ```
   [movement.bot] RecoveryStrategies: Level 1 failed, escalating to Level 2
   [movement.bot] RecoveryStrategies: Level 2 - Attempting jump
   ```

8. **Test Max Attempts**
   ```
   # Create unrecoverable stuck (enclosed space)
   # Wait for 5 recovery attempts
   ```

   **Expected:**
   ```
   [movement.bot] RecoveryStrategies: Max attempts (5) reached
   [movement.bot] RecoveryStrategies: Level 3 - Teleporting to last safe position
   ```

9. **Validation Checklist**
   - [ ] Stuck detection triggers at 3-second threshold
   - [ ] Distance threshold (2.0 yards) respected
   - [ ] Recovery Level 1 attempts reverse movement
   - [ ] Recovery Level 2 attempts jump
   - [ ] Recovery Level 3 teleports to safety
   - [ ] Max attempts (5) enforced
   - [ ] Successful recovery resets stuck state
   - [ ] Bot resumes normal movement after recovery

### Known Issues to Watch For
- ⚠️ Stuck never detected → Threshold too high or detection disabled
- ⚠️ False positives during combat → Position recording during rooted/stunned
- ⚠️ Recovery fails repeatedly → Teleport positions not saved
- ⚠️ Infinite recovery loop → Reset logic not working

---

## Test 5: Performance Test (5000 Bots)

### Objective
Verify that BotMovement system scales to 5000 concurrent bots with acceptable performance (<0.1ms per bot, <500ms total).

### Prerequisites
- High-performance test server
- Configuration: `MaxBots = 5000`
- Adequate system resources (16GB+ RAM recommended)

### Test Procedure

1. **Configure Performance Logging**
   ```ini
   # In playerbots.conf
   BotMovement.Debug.LogStateChanges = 0  # Disable verbose logging
   BotMovement.Debug.LogValidationFailures = 0

   # Monitor only errors
   Logger.movement.bot=3,Console Server
   ```

2. **Spawn Bot Population**
   ```
   # Use automated spawner
   .bot add 5000

   # Or use warm pool
   .reload config
   # Wait for warm pool to initialize
   ```

3. **Baseline Performance Measurement**
   ```
   # Before enabling BotMovement
   .server info
   # Note: Update loop time, CPU usage, memory usage
   ```

4. **Enable BotMovement System**
   ```ini
   BotMovement.Enable = 1
   ```
   ```
   .reload config
   ```

5. **Monitor Performance Metrics**

   **Key Metrics to Track:**
   - World update loop time (should stay < 50ms)
   - Bot update overhead (incremental from baseline)
   - Memory usage increase
   - CPU usage per core

6. **Stress Test: All Bots Moving**
   ```
   # Command bots to spread out across map
   .bot command all spread

   # Or use zone-based distribution
   # This forces pathfinding and validation on all bots
   ```

7. **Performance Targets**
   - [ ] Average bot update: <0.1ms per bot
   - [ ] Total 5000 bot update cycle: <500ms
   - [ ] Memory overhead: <10MB per 1000 bots
   - [ ] CPU usage: <10% increase from baseline
   - [ ] No significant lag spikes (>100ms frame time)

8. **Path Cache Efficiency Check**
   ```
   # Log path cache statistics (if available)
   .bot debug pathcache
   ```

   **Expected:**
   - Cache hit rate: 40-60%
   - Average lookup time: <0.1ms
   - Memory usage: ~50-100MB for 5000 cache entries

9. **Validation Performance Check**
   ```
   # With all validations enabled, measure overhead
   # Then disable each validation and measure again
   ```

   **Breakdown:**
   - Ground validation: ~30% of validation time
   - Collision validation: ~50% of validation time
   - Liquid validation: ~20% of validation time

10. **Long-Running Stability Test**
    - Run server with 5000 bots for 1+ hours
    - Monitor for memory leaks
    - Check for performance degradation over time
    - Verify no deadlocks or hangs

11. **Validation Checklist**
    - [ ] 5000 bots running simultaneously
    - [ ] Update performance targets met
    - [ ] No server crashes or hangs
    - [ ] Memory usage stable (no leaks)
    - [ ] Path cache operating efficiently
    - [ ] Validation overhead acceptable
    - [ ] Long-term stability confirmed

### Known Issues to Watch For
- ⚠️ Performance degrades over time → Memory leak or cache overflow
- ⚠️ Lag spikes during bot spawning → Initialization not amortized
- ⚠️ High CPU with movement disabled → Update overhead in wrong place
- ⚠️ Deadlocks with many bots → Lock contention in movement manager

---

## Test 6: Configuration Validation

### Objective
Verify that all configuration options work correctly and independently.

### Test Procedure

1. **Test Master Toggle**
   ```ini
   BotMovement.Enable = 0
   ```
   - [ ] System completely disabled
   - [ ] Falls back to legacy MotionMaster
   - [ ] No validation performed

2. **Test Ground Validation Toggle**
   ```ini
   BotMovement.Validation.Ground = 0
   ```
   - [ ] Void detection disabled
   - [ ] Bots can walk off cliffs (expected behavior)
   - [ ] Other validations still work

3. **Test Collision Validation Toggle**
   ```ini
   BotMovement.Validation.Collision = 0
   ```
   - [ ] Wall detection disabled
   - [ ] Bots may attempt to path through walls
   - [ ] Other validations still work

4. **Test Liquid Validation Toggle**
   ```ini
   BotMovement.Validation.Liquid = 0
   ```
   - [ ] Water detection disabled
   - [ ] Swimming state may not trigger
   - [ ] Other validations still work

5. **Test Stuck Detection Toggle**
   ```ini
   BotMovement.StuckDetection.Enable = 0
   ```
   - [ ] Stuck detection completely disabled
   - [ ] Bots may remain stuck indefinitely
   - [ ] Movement system otherwise functional

6. **Test Threshold Tuning**
   ```ini
   BotMovement.StuckDetection.Threshold = 5.0  # Increase to 5 yards
   ```
   - [ ] Stuck detection less sensitive
   - [ ] Requires 5 yards of movement in 3 seconds
   - [ ] May reduce false positives

7. **Test Path Cache Settings**
   ```ini
   BotMovement.PathCache.Enable = 0
   ```
   - [ ] Path caching disabled
   - [ ] Every path calculated fresh
   - [ ] Performance impact measurable

---

## Reporting Test Results

### Test Report Template

```markdown
# BotMovement Test Report

**Date:** YYYY-MM-DD
**Tester:** [Your Name]
**Server Build:** [Git commit hash]
**Configuration:** [Relevant config settings]

## Test Results Summary

| Test | Status | Issues Found |
|------|--------|--------------|
| Water Test | ✅ PASS | None |
| Wall Test | ⚠️ PARTIAL | See issue #1 |
| Cliff Test | ✅ PASS | None |
| Stuck Test | ❌ FAIL | See issue #2 |
| Performance | ✅ PASS | None |

## Detailed Results

### Test 1: Water Test
- Status: PASS
- Bot correctly entered swimming state at position X,Y,Z
- State transition logged: [paste log]
- Swimming animation played correctly
- No issues observed

### Test 2: Wall Test
- Status: PARTIAL
- Collision detection worked for thick walls
- **Issue #1:** Thin walls (<0.5 yards) not detected
  - Reproduction: Position bot at X,Y,Z, command to X2,Y2,Z2
  - Expected: Path rejected
  - Actual: Bot walked through thin wall
  - Log output: [paste]

[Continue for each test...]

## Issues Found

### Issue #1: Thin Wall Detection Failure
- **Severity:** Medium
- **Component:** CollisionValidator
- **Reproduction Steps:**
  1. Position bot at -8400, 340, 120
  2. Command to -8400.3, 340, 120 (through thin wall)
  3. Observe bot walks through
- **Expected:** Path rejected by collision validation
- **Actual:** Path accepted, bot clips through wall
- **Logs:** [paste relevant logs]
- **Suggested Fix:** Reduce collision ray width threshold

[Continue for each issue...]

## Performance Metrics

- Baseline update time: 25ms
- With BotMovement (5000 bots): 420ms total update time
- Per-bot average: 0.084ms ✅ (target: <0.1ms)
- Memory overhead: 45MB ✅ (target: <50MB)
- Cache hit rate: 52% ✅ (target: 40-60%)

## Recommendations

1. Fix thin wall detection (Issue #1)
2. Tune stuck detection threshold for combat scenarios
3. Consider adding configuration for collision ray width
4. Document known limitations with thin geometry

## Sign-off

- [ ] All critical tests passed
- [ ] Known issues documented
- [ ] Performance targets met
- [ ] System ready for integration

**Tester Signature:** _________________________
**Date:** _________________________
```

---

## Troubleshooting Common Issues

### Bot Not Entering Swimming State
1. Check `BotMovement.Enable = 1`
2. Check `BotMovement.Validation.Liquid = 1`
3. Verify map liquid data loaded: `.debug liquid`
4. Check logs for `IsSwimmingRequired` call

### Path Validation Always Failing
1. Check if VMAP/MMAP data loaded
2. Verify `BotMovement.Validation.*` settings
3. Check for overly strict thresholds
4. Review `LogValidationFailures` output

### Stuck Detection Too Sensitive
1. Increase `BotMovement.StuckDetection.Threshold` (default 2.0)
2. Increase `BotMovement.StuckDetection.PositionThreshold` (default 3000ms)
3. Check if bot is rooted/stunned during detection

### Performance Issues
1. Disable verbose logging (`LogStateChanges = 0`)
2. Reduce `PathCache.MaxSize` if memory constrained
3. Consider disabling expensive validations
4. Profile with built-in performance tools

---

## Conclusion

Complete all tests in this guide and document results using the provided template. All critical tests must pass before considering the Movement Integration complete.

**Task 6 Checklist:**
- [ ] Water test completed
- [ ] Wall test completed
- [ ] Cliff test completed
- [ ] Stuck test completed
- [ ] Performance test completed
- [ ] Configuration test completed
- [ ] Test report generated
- [ ] Known issues documented
- [ ] System approved for production

Once all tests pass, Task 6: Testing & Validation is complete.
