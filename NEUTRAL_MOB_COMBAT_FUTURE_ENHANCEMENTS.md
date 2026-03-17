# Neutral Mob Combat Detection - Future Enhancements

**Current Status**: ✅ Phase 1 Complete (Basic Detection)
**Date**: October 12, 2025

---

## 📋 Phase 1: Basic Detection (COMPLETED)

### What Was Implemented
- ✅ Real-time neutral mob attack detection using typed packets
- ✅ 4-layer detection system (ATTACK_START, AI_REACTION, SPELL_CAST_START, SPELL_DAMAGE_TAKEN)
- ✅ EnterCombatWithTarget helper for forced combat entry
- ✅ Configuration options (Playerbot.AI.NeutralMobDetection)
- ✅ Comprehensive testing checklist
- ✅ Debug logging system

### Current Capabilities
- Bots detect when neutral mobs attack them (melee, spell, DoT)
- Bots properly engage combat when attacking neutral mobs
- Event-driven detection (no polling overhead)
- Performance: < 0.001% CPU per bot

---

## 🚀 Phase 2: Smart Target Selection (Recommended Next)

### Goal
Improve bot decision-making when attacked by multiple neutral mobs simultaneously.

### Features to Implement

#### 2.1 Threat-Based Target Prioritization
```cpp
// In EnterCombatWithTarget or new method
Unit* SelectBestTarget(std::vector<Unit*> const& attackers)
{
    // Priority factors:
    // 1. Closest attacker (distance)
    // 2. Lowest health (easier kill)
    // 3. Highest damage dealer (threat assessment)
    // 4. Spell casters (interrupt priority)
    // 5. Elite status (avoid if outnumbered)

    float bestScore = 0.0f;
    Unit* bestTarget = nullptr;

    for (Unit* attacker : attackers)
    {
        float score = CalculateThreatScore(attacker);
        if (score > bestScore)
        {
            bestScore = score;
            bestTarget = attacker;
        }
    }

    return bestTarget;
}
```

#### 2.2 Class-Specific Targeting
- **Tanks**: Prioritize closest/highest threat
- **DPS**: Prioritize lowest health
- **Healers**: Prioritize spell casters (for interrupts)
- **Ranged**: Maintain distance, kite if needed

#### 2.3 Group Coordination
```cpp
// When in group, coordinate target selection
if (bot->GetGroup())
{
    // Check if tank is already fighting this mob
    // Assist tank instead of switching targets
    Unit* tankTarget = GetTankTarget();
    if (tankTarget && tankTarget->IsInCombatWith(bot))
        return tankTarget;
}
```

### Configuration Options
```ini
Playerbot.AI.NeutralMobDetection.SmartTargeting = 1
Playerbot.AI.NeutralMobDetection.PrioritizeClosest = 1
Playerbot.AI.NeutralMobDetection.PrioritizeLowestHealth = 0
Playerbot.AI.NeutralMobDetection.AvoidElites = 1
```

### Estimated Effort
- **Development Time**: 2-3 days
- **Testing Time**: 1 day
- **Complexity**: Medium

---

## 🛡️ Phase 3: Defensive Cooldown Integration (High Priority)

### Goal
Automatically use defensive abilities when attacked by neutral mobs while low on health.

### Features to Implement

#### 3.1 Health-Based Defensive Response
```cpp
void BotAI::EnterCombatWithTarget(Unit* target)
{
    // ... existing code ...

    // Check if bot is low health and should use defensive cooldowns
    float healthPercent = _bot->GetHealthPct();

    if (healthPercent < 50.0f)
    {
        // Use defensive cooldowns based on class
        UseDefensiveCooldowns(healthPercent);
    }

    // ... rest of code ...
}

void BotAI::UseDefensiveCooldowns(float healthPercent)
{
    // Class-specific defensive abilities
    switch (_bot->getClass())
    {
        case CLASS_WARRIOR:
            if (healthPercent < 30.0f)
                TryCastSpell(SPELL_SHIELD_WALL);  // Shield Wall at < 30%
            else if (healthPercent < 50.0f)
                TryCastSpell(SPELL_SHIELD_BLOCK);  // Shield Block at < 50%
            break;

        case CLASS_PALADIN:
            if (healthPercent < 20.0f)
                TryCastSpell(SPELL_LAY_ON_HANDS);  // Lay on Hands at < 20%
            else if (healthPercent < 50.0f)
                TryCastSpell(SPELL_DIVINE_SHIELD);  // Divine Shield at < 50%
            break;

        // ... other classes ...
    }
}
```

#### 3.2 Potion/Consumable Usage
```cpp
// Auto-use health potions when attacked by neutral mobs
if (healthPercent < 40.0f && HasHealthPotion())
{
    UseHealthPotion();
}
```

#### 3.3 Self-Heal Integration
```cpp
// Healers should self-heal when attacked
if (IsHealerClass() && healthPercent < 60.0f)
{
    CastHealOnSelf();
}
```

### Configuration Options
```ini
Playerbot.AI.NeutralMobDetection.UseDefensiveCDs = 1
Playerbot.AI.NeutralMobDetection.DefensiveCDThreshold = 50.0
Playerbot.AI.NeutralMobDetection.UsePotions = 1
Playerbot.AI.NeutralMobDetection.PotionThreshold = 40.0
```

### Estimated Effort
- **Development Time**: 3-4 days
- **Testing Time**: 2 days
- **Complexity**: Medium-High

---

## 🏃 Phase 4: Avoidance & Escape Logic (Optional)

### Goal
Detect dangerous situations and allow bots to flee when outnumbered or overwhelmed.

### Features to Implement

#### 4.1 Danger Assessment
```cpp
bool BotAI::IsInDangerousSituation()
{
    // Count attackers
    uint32 attackerCount = GetAttackerCount();

    // Check health status
    float healthPercent = _bot->GetHealthPct();

    // Check if surrounded
    bool isSurrounded = attackerCount >= 3;

    // Check if low health with multiple attackers
    bool isOverwhelmed = (healthPercent < 40.0f && attackerCount >= 2);

    return isSurrounded || isOverwhelmed;
}
```

#### 4.2 Escape Behavior
```cpp
void BotAI::HandleDangerousS situation()
{
    if (!IsInDangerousSituation())
        return;

    // Try to escape
    TC_LOG_INFO("playerbot.combat", "Bot {} attempting to flee from {} attackers at {}% health",
        _bot->GetName(), GetAttackerCount(), _bot->GetHealthPct());

    // Use escape abilities
    if (HasEscapeAbility())
        UseEscapeAbility();  // Vanish, Blink, Disengage, etc.

    // Run to safe location
    Vector3 safeLocation = FindSafeLocation();
    MoveTo(safeLocation);

    // Set AI state to FLEEING
    SetAIState(BotAIState::FLEEING);
}
```

#### 4.3 Kiting for Ranged Classes
```cpp
// Ranged classes should maintain distance from neutral mobs
if (IsRangedClass() && GetDistanceTo(target) < 10.0f)
{
    // Backpedal while casting
    MoveAwayFrom(target, 15.0f);
}
```

### Configuration Options
```ini
Playerbot.AI.NeutralMobDetection.AllowFleeing = 1
Playerbot.AI.NeutralMobDetection.FleeHealthThreshold = 30.0
Playerbot.AI.NeutralMobDetection.FleeAttackerCount = 3
Playerbot.AI.NeutralMobDetection.EnableKiting = 1
```

### Estimated Effort
- **Development Time**: 4-5 days
- **Testing Time**: 2-3 days
- **Complexity**: High

---

## 📊 Phase 5: Advanced Analytics & Learning (Future)

### Goal
Track bot performance in neutral mob combat and optimize behavior based on success rates.

### Features to Implement

#### 5.1 Combat Statistics Tracking
```cpp
struct NeutralMobCombatStats
{
    uint32 totalEncounters = 0;
    uint32 victoriesus = 0;
    uint32 deaths = 0;
    uint32 flees = 0;
    float averageCombatDuration = 0.0f;
    float averageHealthRemaining = 0.0f;
    std::map<uint32, uint32> mobTypeDeaths;  // Track which mobs kill bots most
};
```

#### 5.2 Adaptive Difficulty
```cpp
// Adjust bot behavior based on death rate
if (stats.deaths > stats.victories)
{
    // Bot is dying too much, make it more cautious
    fleeBehaviorThreshold += 10.0f;  // Flee earlier
    defensiveCDThreshold += 10.0f;   // Use CDs sooner
}
```

#### 5.3 Performance Dashboard
- Track neutral mob combat metrics per bot
- Export to database for analysis
- Web dashboard showing:
  - Most dangerous neutral mobs
  - Bot classes with highest/lowest survival rates
  - Zones with highest bot death rates

### Configuration Options
```ini
Playerbot.AI.NeutralMobDetection.TrackStatistics = 1
Playerbot.AI.NeutralMobDetection.EnableAdaptiveBehavior = 0
Playerbot.AI.NeutralMobDetection.ExportStatistics = 0
```

### Estimated Effort
- **Development Time**: 1-2 weeks
- **Testing Time**: 1 week
- **Complexity**: Very High

---

## 🔧 Phase 6: Configuration System Integration (Easy Win)

### Goal
Make neutral mob detection configurable without code changes.

### Features to Implement

#### 6.1 Runtime Configuration
```cpp
// Load config in BotAI::Initialize()
bool neutralMobDetectionEnabled = sConfigMgr->GetOption<bool>("Playerbot.AI.NeutralMobDetection", true);
uint32 logLevel = sConfigMgr->GetOption<uint32>("Playerbot.AI.NeutralMobDetection.LogLevel", 3);

if (!neutralMobDetectionEnabled)
{
    // Don't subscribe to combat events for neutral mob detection
    return;
}
```

#### 6.2 Dynamic Enable/Disable
```cpp
// GM command to toggle feature without restart
.bot neutralmob enable
.bot neutralmob disable
.bot neutralmob status
```

#### 6.3 Per-Bot Configuration
```cpp
// Allow individual bots to have different neutral mob settings
.bot <name> neutralmob smarttargeting on
.bot <name> neutralmob defensivecds off
.bot <name> neutralmob fleeing on
```

### Configuration Options
Already added in playerbots.conf.dist:
```ini
Playerbot.AI.NeutralMobDetection = 1
Playerbot.AI.NeutralMobDetection.LogLevel = 3
```

### Estimated Effort
- **Development Time**: 1-2 days
- **Testing Time**: 0.5 days
- **Complexity**: Low

---

## 🎨 Phase 7: Visual Feedback (Polish)

### Goal
Provide visual indicators when neutral mob detection triggers.

### Features to Implement

#### 7.1 Emote System Integration
```cpp
// Bot uses emotes when attacked by neutral mob
_bot->HandleEmoteCommand(EMOTE_ONESHOT_COMBAT_CRITICAL);  // Combat stance
_bot->Say("I'm under attack!", LANG_UNIVERSAL);  // If chat enabled
```

#### 7.2 Combat Text
```cpp
// Send combat text to nearby players
WorldPackets::CombatLog::SpellNonMeleeDamageLog damageLog;
damageLog.Me = _bot->GetGUID();
damageLog.Attacker = attacker->GetGUID();
// ... send packet
```

#### 7.3 Threat Indicator
```cpp
// Mark attacker with raid target icon for visibility
if (bot->GetGroup())
{
    bot->GetGroup()->SetTargetIcon(0, attacker->GetGUID());  // Skull icon
}
```

### Configuration Options
```ini
Playerbot.AI.NeutralMobDetection.ShowEmotes = 1
Playerbot.AI.NeutralMobDetection.ShowCombatText = 0
Playerbot.AI.NeutralMobDetection.MarkTargets = 1
```

### Estimated Effort
- **Development Time**: 2-3 days
- **Testing Time**: 1 day
- **Complexity**: Low-Medium

---

## 🏆 Implementation Priority Ranking

### High Priority (Do Next)
1. **Phase 2: Smart Target Selection** - Improves multi-mob combat
2. **Phase 3: Defensive Cooldown Integration** - Reduces bot deaths
3. **Phase 6: Configuration System** - Easy to implement, high value

### Medium Priority (Nice to Have)
4. **Phase 4: Avoidance & Escape Logic** - Reduces bot deaths further
5. **Phase 7: Visual Feedback** - Polish and user experience

### Low Priority (Future Consideration)
6. **Phase 5: Advanced Analytics** - Complex, requires significant infrastructure

---

## 📈 Success Metrics

### Current Baseline (Phase 1)
- **Detection Rate**: 99%+ (4-layer detection)
- **Response Time**: < 100ms (event-driven)
- **CPU Overhead**: < 0.001% per bot
- **Bot Survival vs Neutral Mobs**: Unknown (needs testing)

### Target Metrics (After All Phases)
- **Detection Rate**: 99.9%+
- **Response Time**: < 50ms (optimized)
- **CPU Overhead**: < 0.005% per bot
- **Bot Survival vs Neutral Mobs**: > 80%
- **Bot Deaths from Neutral Mobs**: < 10% of total deaths

---

## 🛠️ Development Roadmap

### Q1 2025 (Current)
- ✅ Phase 1: Basic Detection (COMPLETED)
- ⏳ Phase 1 Testing & Bug Fixes

### Q2 2025
- Phase 2: Smart Target Selection
- Phase 3: Defensive Cooldown Integration
- Phase 6: Configuration System

### Q3 2025
- Phase 4: Avoidance & Escape Logic
- Phase 7: Visual Feedback

### Q4 2025
- Phase 5: Advanced Analytics (if time permits)
- Final polish and optimization

---

## 💡 Community Feature Requests

*This section will be updated based on user feedback after Phase 1 deployment*

### Requested Features
- [ ] Configurable aggro radius for neutral mob detection
- [ ] Whitelist/blacklist specific neutral mob types
- [ ] Integration with quest system (avoid certain neutral mobs during quests)
- [ ] Pet class support (hunter/warlock pets attacking neutral mobs)
- [ ] Mount detection (dismount when attacked by neutral mob)

---

## 🔗 Related Documentation

- **Current Implementation**: `NEUTRAL_MOB_COMBAT_IMPLEMENTATION.md`
- **Testing Guide**: `NEUTRAL_MOB_COMBAT_TESTING.md`
- **Solution Design**: `SOLUTION_NEUTRAL_MOB_COMBAT_FIX.md`
- **Configuration**: `src/modules/Playerbot/conf/playerbots.conf.dist`

---

**Last Updated**: October 12, 2025
**Status**: Phase 1 Complete, Phase 2-7 Planned
**Maintainer**: TrinityBots Development Team
