# Neutral Mob Combat Detection - Testing Checklist

**Purpose**: Verify neutral mob combat detection works correctly in all scenarios
**Date**: October 12, 2025
**Tester**: _______________
**Build**: Release x64

---

## 📋 Pre-Testing Setup

### 1. Enable Debug Logging

Edit `worldserver.conf`:
```ini
Logger.playerbot.combat=6,Console Server
```

### 2. Restart World Server
```bash
# Stop server
taskkill /F /IM worldserver.exe

# Start server (from server directory)
worldserver.exe -c worldserver.conf
```

### 3. Create Test Bot
```
# In-game commands
.bot add <name>            # Create a bot
.bot <name> summon         # Summon bot to your location
```

---

## ✅ Test Scenarios

### Scenario 1: Neutral Mob Attacks Bot (Melee)
**Location**: Mulgore (Tauren starting zone) - Plainstriders
**Expected**: Bot immediately fights back

- [ ] Spawn bot near neutral mobs (Plainstriders, level 1-3)
- [ ] Pull a neutral mob to attack the bot
- [ ] **VERIFY**: Bot starts attacking within 1 second
- [ ] **CHECK LOGS**: Look for:
  ```
  [DEBUG] Bot <name> detected neutral mob Plainstrider attacking via ATTACK_START
  [INFO]  Bot <name> force-entering combat with Plainstrider (GUID: ...)
  ```
- [ ] **VERIFY**: Bot continues fighting until mob dies
- [ ] **VERIFY**: Bot exits combat properly after mob dies

**Result**: ⬜ PASS ⬜ FAIL
**Notes**: _______________________________________

---

### Scenario 2: Bot Attacks Neutral Mob
**Location**: Any zone with neutral mobs
**Expected**: Bot properly engages combat

- [ ] Command bot to attack a neutral mob: `.bot <name> attack`
- [ ] **VERIFY**: Bot attacks immediately
- [ ] **CHECK LOGS**: Look for AI state change to COMBAT
- [ ] **VERIFY**: Bot maintains combat until mob dies
- [ ] **VERIFY**: Bot doesn't lose target mid-combat

**Result**: ⬜ PASS ⬜ FAIL
**Notes**: _______________________________________

---

### Scenario 3: Neutral Mob Casts Hostile Spell
**Location**: Durotar - Voodoo Trolls (cast Shadow Bolt)
**Expected**: Bot detects spell cast and fights back

- [ ] Spawn bot near caster mob (Voodoo Troll, Witch Doctor)
- [ ] Aggro caster mob to target bot
- [ ] **VERIFY**: Bot reacts when spell cast starts
- [ ] **CHECK LOGS**: Look for:
  ```
  [DEBUG] Bot <name> detected neutral mob <mob> casting hostile spell <spellId> via SPELL_CAST_START
  [INFO]  Bot <name> force-entering combat with <mob> (GUID: ...)
  ```
- [ ] **VERIFY**: Bot interrupts cast if possible (has interrupt ability)
- [ ] **VERIFY**: Bot fights back even if spell lands

**Result**: ⬜ PASS ⬜ FAIL
**Notes**: _______________________________________

---

### Scenario 4: Neutral Mob Applies DoT
**Location**: Any zone with DoT-casting neutral mobs
**Expected**: Bot detects damage and fights back

- [ ] Find mob that applies DoT (poison, bleed, etc.)
- [ ] Let mob apply DoT to bot
- [ ] **VERIFY**: Bot enters combat when DoT ticks
- [ ] **CHECK LOGS**: Look for SPELL_DAMAGE_TAKEN event
- [ ] **VERIFY**: Bot pursues mob even if mob runs away

**Result**: ⬜ PASS ⬜ FAIL
**Notes**: _______________________________________

---

### Scenario 5: Multiple Neutral Mobs Attack
**Location**: Any dense neutral mob area
**Expected**: Bot detects all attackers

- [ ] Pull 3 neutral mobs to attack bot
- [ ] **VERIFY**: Bot detects first attacker immediately
- [ ] **VERIFY**: Bot switches target appropriately
- [ ] **CHECK LOGS**: Verify 3 separate ATTACK_START events
- [ ] **VERIFY**: Bot fights all 3 mobs to death or until bot dies
- [ ] **VERIFY**: No stuck-in-combat issues

**Result**: ⬜ PASS ⬜ FAIL
**Notes**: _______________________________________

---

### Scenario 6: Rapid Aggro Changes
**Location**: Any zone
**Expected**: Bot handles aggro drops/re-aggros

- [ ] Aggro neutral mob to attack bot
- [ ] Run far away to drop aggro
- [ ] Return and re-aggro same mob
- [ ] **VERIFY**: Bot detects re-aggro correctly
- [ ] **VERIFY**: No duplicate combat entry
- [ ] **CHECK LOGS**: Verify EnterCombatWithTarget doesn't log twice for same target

**Result**: ⬜ PASS ⬜ FAIL
**Notes**: _______________________________________

---

### Scenario 7: AoE Damage from Neutral Mob
**Location**: Find mobs with AoE abilities
**Expected**: Bot detects AoE damage

- [ ] Stand bot near AoE-casting neutral mob
- [ ] Let mob cast AoE that hits bot
- [ ] **VERIFY**: Bot enters combat from AoE damage
- [ ] **CHECK LOGS**: Look for SPELL_DAMAGE_TAKEN event

**Result**: ⬜ PASS ⬜ FAIL
**Notes**: _______________________________________

---

### Scenario 8: Bot Group Combat
**Location**: Any zone
**Expected**: Multiple bots coordinate

- [ ] Spawn 3 bots in same group
- [ ] Aggro neutral mob to attack one bot
- [ ] **VERIFY**: Attacked bot fights back immediately
- [ ] **VERIFY**: Group assists (if group assist is enabled)
- [ ] **VERIFY**: All bots exit combat properly

**Result**: ⬜ PASS ⬜ FAIL
**Notes**: _______________________________________

---

### Scenario 9: Low Health Response
**Location**: Any zone
**Expected**: Bot uses defensive abilities when low HP

- [ ] Damage bot to ~30% health (using higher level mob or multiple mobs)
- [ ] Let neutral mob attack
- [ ] **VERIFY**: Bot fights back
- [ ] **VERIFY**: Bot uses defensive cooldowns if available
- [ ] **VERIFY**: Bot attempts to heal if healer class

**Result**: ⬜ PASS ⬜ FAIL
**Notes**: _______________________________________

---

### Scenario 10: Ranged Pull
**Location**: Any zone
**Expected**: Bot handles ranged combat initiation

- [ ] Use ranged bot class (Hunter, Mage, Warlock)
- [ ] Command bot to attack neutral mob from max range
- [ ] **VERIFY**: Bot enters combat immediately
- [ ] **VERIFY**: Bot maintains distance if ranged class
- [ ] **VERIFY**: Combat state persists until mob dies

**Result**: ⬜ PASS ⬜ FAIL
**Notes**: _______________________________________

---

## 📊 Performance Testing

### Performance Scenario 1: 10 Bots in Combat
- [ ] Spawn 10 bots in neutral mob area
- [ ] Aggro mobs to attack all 10 bots
- [ ] **MEASURE**: Server CPU usage
- [ ] **MEASURE**: Server frame time
- [ ] **VERIFY**: < 5% CPU increase with 10 bots in combat
- [ ] **CHECK LOGS**: No performance warnings

**CPU Usage**: ____% (baseline) → ____% (with bots fighting)
**Frame Time**: ____ms (baseline) → ____ms (with bots fighting)
**Result**: ⬜ PASS ⬜ FAIL

---

### Performance Scenario 2: 100 Bots in Combat
- [ ] Spawn 100 bots across multiple zones
- [ ] Aggro mobs to attack multiple bots
- [ ] **MEASURE**: Server CPU usage
- [ ] **MEASURE**: Server memory usage
- [ ] **VERIFY**: < 10% CPU increase with 100 bots
- [ ] **CHECK LOGS**: No crash or memory leaks

**CPU Usage**: ____% → ____%
**Memory Usage**: ____MB → ____MB
**Result**: ⬜ PASS ⬜ FAIL

---

## 🐛 Edge Cases & Bug Checks

### Edge Case 1: Friendly Fire
- [ ] Bot attacks neutral mob
- [ ] Friendly player/bot nearby takes accidental AoE
- [ ] **VERIFY**: Bot doesn't attack friendly target
- [ ] **VERIFY**: No false positive combat entry

**Result**: ⬜ PASS ⬜ FAIL

---

### Edge Case 2: Dead Attacker
- [ ] Neutral mob starts attacking bot
- [ ] Kill mob before event processes
- [ ] **VERIFY**: Bot doesn't crash
- [ ] **VERIFY**: Bot exits combat properly
- [ ] **CHECK LOGS**: No NULL pointer errors

**Result**: ⬜ PASS ⬜ FAIL

---

### Edge Case 3: Bot Teleport During Combat
- [ ] Bot in combat with neutral mob
- [ ] Teleport bot to different zone: `.bot <name> teleport <location>`
- [ ] **VERIFY**: Combat ends properly
- [ ] **VERIFY**: No stuck-in-combat state

**Result**: ⬜ PASS ⬜ FAIL

---

### Edge Case 4: Bot Death
- [ ] Let bot die to neutral mob
- [ ] **VERIFY**: Bot releases properly
- [ ] **VERIFY**: No memory leaks on death
- [ ] **VERIFY**: Bot can respawn and fight again

**Result**: ⬜ PASS ⬜ FAIL

---

### Edge Case 5: Server Restart During Combat
- [ ] Bot in combat with neutral mob
- [ ] Restart server gracefully
- [ ] **VERIFY**: No crash on shutdown
- [ ] **VERIFY**: Bot state clean on restart

**Result**: ⬜ PASS ⬜ FAIL

---

## 📈 Test Summary

**Total Tests**: 15 scenarios + 2 performance + 5 edge cases = **22 tests**

### Results
- **PASSED**: _____ / 22
- **FAILED**: _____ / 22
- **SKIPPED**: _____ / 22

### Critical Issues Found
1. _____________________________________
2. _____________________________________
3. _____________________________________

### Non-Critical Issues Found
1. _____________________________________
2. _____________________________________
3. _____________________________________

### Performance Notes
- CPU overhead per bot: _____%
- Memory overhead per bot: _____MB
- Event processing time: _____μs

---

## ✅ Sign-Off

**Tested By**: _______________
**Date**: _______________
**Build Version**: _______________
**Overall Result**: ⬜ APPROVED ⬜ NEEDS FIXES

**Recommendation**:
- ⬜ Ready for production
- ⬜ Needs minor fixes
- ⬜ Needs major rework

**Notes**:
_______________________________________
_______________________________________
_______________________________________

---

## 🔍 Debugging Tips

### If Bot Doesn't React to Neutral Mob Attack

1. **Check debug logging is enabled**:
   ```bash
   # Search worldserver.conf for:
   Logger.playerbot.combat=6,Console Server
   ```

2. **Verify event bus subscriptions**:
   ```
   # Look for in logs:
   Bot <name> subscribed to all 11 event buses
   ```

3. **Check if packet handler is registered**:
   ```
   # Look for in logs:
   PlayerbotPacketSniffer: Registered 10 Combat packet typed handlers
   ```

4. **Verify bot is receiving packets**:
   ```
   # Look for ANY combat event logs for the bot
   Bot <name> received ATTACK_START (typed)
   ```

5. **Check if bot is already in combat**:
   ```
   # The detection only fires if !_bot->IsInCombat()
   # Check bot's current combat state
   ```

### If Bot Attacks Friendly Targets

1. **Check IsHostileTo() logic**:
   ```cpp
   // In ProcessCombatInterrupt
   if (!caster->IsHostileTo(_bot))
       return;  // Should prevent friendly fire
   ```

2. **Verify faction data**:
   ```
   # Check creature_template.faction for the mob
   # Neutral mobs should have faction 7 or 35
   ```

### If Performance Is Poor

1. **Check event count**:
   ```
   # Count ATTACK_START events in logs
   # Should be ~5-10 per combat encounter
   ```

2. **Monitor CPU with Task Manager / htop**

3. **Check for event bus memory leaks**:
   ```
   # Verify UnsubscribeFromEventBuses called on bot deletion
   ```

---

## 📞 Support

**Issue Reporting**: https://github.com/anthropics/claude-code/issues
**Documentation**: `NEUTRAL_MOB_COMBAT_IMPLEMENTATION.md`
**Solution Design**: `SOLUTION_NEUTRAL_MOB_COMBAT_FIX.md`
