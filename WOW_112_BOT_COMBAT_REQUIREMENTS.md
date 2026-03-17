# WoW 11.2 (The War Within) Bot Combat System Requirements

## Executive Summary
This document outlines the complete combat behavior requirements for enterprise-grade bot AI in World of Warcraft 11.2 (The War Within). Based on analysis of the current TrinityCore PlayerBot implementation, I've identified critical missing components needed for authentic WoW 11.2 combat simulation.

## Current Implementation Status
The existing PlayerBot module has ~119 ClassAI implementations with:
- Basic rotation management (100ms polling via OnCombatUpdate)
- Resource tracking systems (mana, energy, rage, etc.)
- Spell cooldown management
- Basic interrupt framework
- Threat detection primitives

## 1. WOW 11.2 COMBAT MECHANICS

### 1.1 Hero Talents System (P0 - CRITICAL)
**Status**: MISSING - Partial spell IDs present, no implementation

**Required Implementation**:
- **39 Hero Talent Trees** (3 per class)
  - Death Knight: Deathbringer, San'layn, Rider of the Apocalypse
  - Demon Hunter: Aldrachi Reaver, Fel-Scarred
  - Druid: Keeper of the Grove, Wildstalker, Elune's Chosen, Dream of Cenarius
  - Evoker: Scalecommander, Flameshaper, Chronowarden
  - Hunter: Dark Ranger, Sentinel, Pack Leader
  - Mage: Frostfire, Sunfury, Spellslinger
  - Monk: Conduit of the Celestials, Master of Harmony, Shado-Pan
  - Paladin: Lightsmith, Templar, Herald of the Sun
  - Priest: Voidweaver, Oracle, Archon
  - Rogue: Deathstalker, Trickster, Fatebound
  - Shaman: Totemic, Stormbringer, Farseer
  - Warlock: Hellcaller, Diabolist, Soul Harvester
  - Warrior: Mountain Thane, Colossus, Slayer

**Performance Target**:
- Memory: +2KB per bot for talent state tracking
- CPU: <0.5ms per talent evaluation cycle

### 1.2 Modern Stat Scaling (P0 - CRITICAL)
**Status**: MISSING - No stat weight calculation system

**Required Stats**:
- **Primary**: Strength, Agility, Intellect (item level 480-639)
- **Secondary**:
  - Critical Strike (base 5%, soft cap ~40%)
  - Haste (affects GCD, resource regen, DoT tick rate)
  - Mastery (spec-specific scaling)
  - Versatility (damage/healing increase + reduction)
  - Avoidance (AoE damage reduction, M+ critical)
  - Leech (self-healing percentage)
  - Speed (movement speed bonus)

**Implementation Requirements**:
```cpp
struct StatWeights {
    float criticalStrike;  // Typically 0.8-1.2
    float haste;           // Varies by spec 0.6-1.4
    float mastery;         // Spec-dependent 0.4-1.6
    float versatility;     // Usually 0.6-0.9
    float mainStat;        // Always 1.0 baseline
};
```

**Performance Target**:
- Stat recalculation: <1ms per gear change
- Cache duration: 5 seconds minimum

### 1.3 Mythic+ Combat Requirements (P0 - CRITICAL)
**Status**: PARTIALLY MISSING - Basic mention, no implementation

**Scaling Formula**:
- Base health/damage × (1.08)^keystoneLevel
- M+2: 108% base
- M+10: 185% base
- M+20: 367% base

**Weekly Affixes** (Rotating pool):
- **Fortified**: +20% non-boss health, +30% damage
- **Tyrannical**: +30% boss health, +15% damage
- **Afflicted**: Spawns afflicted souls requiring dispel
- **Entangling**: Roots players periodically
- **Incorporeal**: Spawns adds requiring CC
- **Spiteful**: Dead mobs spawn shades
- **Volcanic**: Ground eruptions requiring movement
- **Storming**: Tornado mechanics

**Combat Behaviors Required**:
- Affix-specific positioning logic
- Interrupt priority adjustment per affix
- Defensive cooldown timing for high keys
- Route optimization awareness

**Performance Target**:
- Affix detection: <0.1ms per check
- Movement decision: <2ms including pathfinding

### 1.4 Delve Encounter Mechanics (P1 - IMPORTANT)
**Status**: MISSING - No delve support

**Delve Tiers**: 1-11 with progressive scaling
**Zekvir Difficulty**: Special boss with unique mechanics

**Required Behaviors**:
- Brann Bronzebeard companion coordination
- Role flexibility (Brann as healer/DPS)
- Treasure event management
- Death counter awareness (limited resurrections)

### 1.5 Modern Threat Mechanics (P0 - CRITICAL)
**Status**: BASIC - Primitive threat detection only

**Threat Formulas**:
- Base threat = Damage × ThreatModifier
- Tank stance: 650% threat modifier
- DPS baseline: 100% threat
- Healing threat: 50% of effective healing

**Required Implementation**:
- Threat table tracking per mob
- Threat drop calculation (130% to pull aggro)
- Threat transfer abilities (Tricks, Misdirection)
- Skittish affix handling (reduced tank threat)

**Performance Target**:
- Threat update: <0.5ms per target
- Full recalculation: <5ms for 10 targets

## 2. REQUIRED COMBAT BEHAVIORS

### 2.1 Defensive Cooldown Usage (P0 - CRITICAL)
**Status**: MINIMAL - UseDefensiveCooldowns() stub exists

**Required Defensive Categories**:
```cpp
enum class DefensiveType {
    IMMUNITY,           // Ice Block, Divine Shield
    MAJOR_REDUCTION,    // 40%+ reduction
    MINOR_REDUCTION,    // 20-39% reduction
    SELF_HEAL,         // Instant self-healing
    ABSORB_SHIELD,     // Damage absorbs
    CHEAT_DEATH,       // Prevent death once
    MOBILITY,          // Escape abilities
    CROWD_CONTROL      // Self-CC removal
};
```

**Trigger Conditions**:
- Health below 30%
- Incoming damage spike detected
- Multiple debuffs active
- Healer dead/incapacitated
- Predictive for known mechanics

**Performance Target**:
- Decision time: <1ms
- Ability prioritization: <2ms

### 2.2 Interrupt Priority System (P0 - CRITICAL)
**Status**: BASIC - Framework exists, needs enhancement

**Priority Levels**:
1. **CRITICAL**: Healing spells, Mass CC, Wipes
2. **HIGH**: Major damage, Single target CC
3. **MODERATE**: Buffs, Standard damage
4. **LOW**: Minor abilities
5. **IGNORE**: Unimportant casts

**Coordination Requirements**:
- Interrupt rotation assignment
- Lockout tracking (6 schools)
- Backup interrupt designation
- Voice/marker coordination simulation

**Performance Target**:
- Cast detection: <0.5ms
- Assignment calculation: <1ms for 5 bots

### 2.3 Dispel/Purge Priorities (P0 - CRITICAL)
**Status**: MISSING - No dispel system

**Dispel Categories**:
- **Magic**: Mage, Priest, Paladin, Evoker, Shaman
- **Disease**: Priest, Paladin, Monk
- **Poison**: Druid, Paladin, Monk, Evoker
- **Curse**: Druid, Mage, Shaman
- **Enrage**: Hunter, Druid, Rogue

**Priority Algorithm**:
```cpp
float CalculateDispelPriority(Aura* aura, Unit* target) {
    float priority = 0.0f;
    if (target->IsHealer()) priority += 10.0f;
    if (target->IsTank() && aura->IsCC()) priority += 8.0f;
    if (aura->GetRemainingTime() < 3000) priority += 5.0f;
    if (IsMythicPlusAfflicted(aura)) priority = 100.0f;
    return priority;
}
```

### 2.4 Movement During Combat (P0 - CRITICAL)
**Status**: BASIC - Simple positioning exists

**Required Movement Behaviors**:
- **Void zones** (stand in bad detection)
- **Spread mechanics** (player spacing)
- **Stack mechanics** (group positioning)
- **Frontal cone avoidance**
- **Charge/knockback preparation**
- **Soak assignment** (designated soakers)
- **Line of sight usage**
- **Kiting patterns** (tank/hunter kiting)

**Performance Target**:
- Position evaluation: <2ms
- Path calculation: <5ms
- Collision detection: <1ms per check

### 2.5 Target Switching Logic (P0 - CRITICAL)
**Status**: BASIC - SelectBestTarget() exists

**Priority Factors**:
```cpp
struct TargetPriority {
    float healthPercent;      // Lower = higher priority
    float executionRange;     // <20% or <35% for some specs
    bool isPriorityAdd;       // Explosive, Incorporeal
    bool hasVulnerability;    // Increased damage taken
    float distanceWeight;     // Closer targets preferred
    bool isInterruptTarget;   // Currently casting priority spell
    float dotUptime;          // Maintain DoTs on multiple targets
    uint8 targetMarker;       // Skull > X > Square > Moon
};
```

**Performance Target**:
- Target evaluation: <1ms per target
- Full scan (10 enemies): <10ms

### 2.6 Multi-target Optimization (P0 - CRITICAL)
**Status**: PARTIAL - Basic AoE detection

**AoE Breakpoints** (typical):
- 2+ targets: Cleave abilities
- 3+ targets: AoE rotation
- 5+ targets: Full AoE priority
- 8+ targets: Capped AoE consideration

**Resource Management**:
- Resource pooling for AoE phases
- Cooldown alignment for burst windows
- DoT spreading optimization

## 3. GROUP COMBAT COORDINATION

### 3.1 Tank Threat Management (P0 - CRITICAL)
**Status**: MISSING - No tank coordination

**Required Behaviors**:
- Active mitigation timing
- Taunt swap coordination
- Threat generation priority
- Positioning for group (mobs face away)
- Kiting when necessary
- Magic mitigation timing

**Tank Swap Triggers**:
- Debuff stacks (typically 2-4)
- Specific mechanics
- Tank death recovery
- Threat loss recovery

### 3.2 Healer Priority Systems (P0 - CRITICAL)
**Status**: MISSING - No healing logic

**Healing Priority**:
```cpp
enum class HealPriority {
    SELF_PRESERVATION,     // Healer must survive
    TANK_CRITICAL,        // Tank below 30%
    RAID_WIDE_INCOMING,   // Predictive healing
    DEBUFF_REMOVAL,       // Dispel priority
    GROUP_SUSTAIN,        // General healing
    DPS_CRITICAL,         // DPS emergency
    TOP_OFF              // Non-urgent healing
};
```

**Triage Decision Tree**:
- Mana conservation vs throughput
- Cooldown usage timing
- External cooldown coordination
- Battle resurrection priority

### 3.3 DPS Coordination (P1 - IMPORTANT)
**Status**: MISSING - No DPS coordination

**Coordination Elements**:
- Bloodlust/Heroism timing
- Burst window alignment
- Add priority assignment
- Interrupt rotation participation
- Defensive external usage
- Combat resurrection priority

### 3.4 Interrupt Rotation (P0 - CRITICAL)
**Status**: MISSING - No rotation system

**Rotation Management**:
```cpp
struct InterruptRotation {
    std::vector<PlayerGuid> rotation;
    uint8 currentIndex;
    uint32 lockoutExpiration[6]; // 6 spell schools
    std::map<uint32, PlayerGuid> assignments; // spellId -> player
};
```

### 3.5 Defensive Coordination (P1 - IMPORTANT)
**Status**: MISSING

**External Cooldowns**:
- Blessing of Protection/Sacrifice
- Life Cocoon
- Guardian Spirit
- Ironbark
- Pain Suppression
- Anti-Magic Zone (raid-wide)

### 3.6 Battle Resurrection Priority (P1 - IMPORTANT)
**Status**: MISSING

**Priority Order**:
1. Main tank (if solo tank)
2. Healer (if solo healer)
3. Battle res capable DPS
4. Highest DPS
5. Utility roles

## 4. PERFORMANCE REQUIREMENTS

### 4.1 Decision Frequency
**Current**: 100ms polling
**Required**:
- GCD-locked abilities: 50ms
- Instant reactions (interrupts): 20ms
- Movement updates: 50ms
- Resource updates: 100ms
- Threat updates: 200ms

### 4.2 Memory Per Bot
**Current Estimate**: ~10MB base
**Required Additions**:
- Combat state machine: +500KB
- Threat tables (10 mobs): +100KB
- Ability priorities: +200KB
- Group coordination: +300KB
- **Total Required**: ~11.1MB per bot

### 4.3 CPU Per Bot During Combat
**Target**: <0.1% CPU per bot (on 4GHz processor)
**Breakdown**:
- Rotation logic: 20%
- Target selection: 15%
- Movement calculation: 25%
- Threat management: 10%
- Group coordination: 15%
- Resource management: 10%
- State updates: 5%

## 5. IMPLEMENTATION PRIORITY MATRIX

| Component | Status | Priority | Complexity | Performance Impact |
|-----------|--------|----------|------------|-------------------|
| Hero Talents | MISSING | P0 | HIGH | MEDIUM |
| Stat Weights | MISSING | P0 | MEDIUM | LOW |
| Defensive CDs | MINIMAL | P0 | MEDIUM | LOW |
| Interrupts | BASIC | P0 | MEDIUM | MEDIUM |
| Dispels | MISSING | P0 | LOW | LOW |
| Movement | BASIC | P0 | HIGH | HIGH |
| Target Priority | BASIC | P0 | MEDIUM | MEDIUM |
| Tank Threat | MISSING | P0 | HIGH | MEDIUM |
| Healer Priority | MISSING | P0 | HIGH | MEDIUM |
| M+ Affixes | MISSING | P1 | HIGH | MEDIUM |
| Delves | MISSING | P2 | MEDIUM | LOW |
| DPS Coordination | MISSING | P1 | MEDIUM | LOW |
| Battle Res | MISSING | P1 | LOW | LOW |

## 6. MISSING CRITICAL SYSTEMS

### 6.1 Predictive Mechanics System (P0)
Bots need to anticipate known mechanics rather than react:
```cpp
class MechanicPredictor {
    void RegisterBossAbility(uint32 spellId, float castTime, uint32 cooldown);
    void PredictNextMechanic();
    Position GetSafePosition(uint32 mechanicId);
    bool ShouldPreemptivelyDefend(float timeWindow);
};
```

### 6.2 Crowd Control Chain Coordination (P1)
Sequential CC application without overlap:
```cpp
class CCChainManager {
    void PlanCCChain(Unit* target, float requiredDuration);
    void AssignCCOrder(std::vector<Player*> ccCapable);
    bool IsTargetCCImmune(Unit* target, CCType type);
    float GetDRMultiplier(Unit* target, CCCategory category);
};
```

### 6.3 Mechanic Learning System (P2)
Adaptive behavior based on encounter experience:
```cpp
class MechanicLearning {
    void RecordMechanicDamage(uint32 spellId, uint32 damage);
    void UpdateAvoidancePriority(uint32 spellId);
    void ShareKnowledgeWithGroup();
    float GetMechanicDanger(uint32 spellId);
};
```

### 6.4 Resource Pooling Intelligence (P1)
Strategic resource management for burst phases:
```cpp
class ResourcePooling {
    bool ShouldPoolResources(float timeUntilBurst);
    uint32 GetOptimalPoolAmount(ResourceType type);
    void NotifyBurstPhaseStarting(float duration);
    bool IsResourceCapped();
};
```

### 6.5 Positioning Optimizer (P0)
Optimal positioning considering multiple factors:
```cpp
class PositionOptimizer {
    Position CalculateOptimalPosition(
        Unit* target,
        std::vector<Unit*> allies,
        std::vector<GroundEffect> hazards,
        CombatRole role,
        float preferredRange
    );
    bool IsPositionSafe(Position pos, float timeWindow);
    float GetPositionScore(Position pos);
};
```

## 7. DATABASE REQUIREMENTS

### New Tables Needed:
```sql
-- Hero talent configurations
CREATE TABLE `playerbot_hero_talents` (
    `class` TINYINT UNSIGNED NOT NULL,
    `spec` TINYINT UNSIGNED NOT NULL,
    `hero_tree` TINYINT UNSIGNED NOT NULL,
    `talent_id` INT UNSIGNED NOT NULL,
    `rank` TINYINT UNSIGNED NOT NULL,
    PRIMARY KEY (`class`, `spec`, `talent_id`)
);

-- Stat weight templates
CREATE TABLE `playerbot_stat_weights` (
    `class` TINYINT UNSIGNED NOT NULL,
    `spec` TINYINT UNSIGNED NOT NULL,
    `item_level` SMALLINT UNSIGNED NOT NULL,
    `stat_type` TINYINT UNSIGNED NOT NULL,
    `weight` FLOAT NOT NULL,
    PRIMARY KEY (`class`, `spec`, `item_level`, `stat_type`)
);

-- Interrupt priorities for specific NPCs/spells
CREATE TABLE `playerbot_interrupt_priority` (
    `spell_id` INT UNSIGNED NOT NULL,
    `npc_entry` INT UNSIGNED DEFAULT NULL,
    `priority` TINYINT UNSIGNED NOT NULL,
    `interrupt_type` TINYINT UNSIGNED NOT NULL,
    PRIMARY KEY (`spell_id`)
);

-- Mythic+ affix behaviors
CREATE TABLE `playerbot_affix_behaviors` (
    `affix_id` INT UNSIGNED NOT NULL,
    `behavior_type` VARCHAR(50) NOT NULL,
    `param1` FLOAT DEFAULT NULL,
    `param2` FLOAT DEFAULT NULL,
    `param3` FLOAT DEFAULT NULL,
    PRIMARY KEY (`affix_id`, `behavior_type`)
);
```

## 8. TESTING REQUIREMENTS

### Unit Tests Required:
- Rotation optimality verification
- Interrupt timing accuracy
- Defensive cooldown trigger conditions
- Target priority calculation
- Resource pooling decisions
- Positioning safety checks

### Integration Tests:
- 5-man dungeon completion (Normal/Heroic/M+)
- Raid encounter mechanics handling
- PvP arena coordination
- World quest completion
- Delve progression

### Performance Tests:
- 100 bot concurrent combat
- 500 bot world simulation
- Memory leak detection
- CPU spike identification
- Database query optimization

## 9. CONFIGURATION REQUIREMENTS

### playerbots.conf Additions:
```ini
# Combat Behavior Settings
Playerbot.Combat.EnableHeroTalents = 1
Playerbot.Combat.UseStatWeights = 1
Playerbot.Combat.InterruptCoordination = 1
Playerbot.Combat.DefensiveThreshold = 30
Playerbot.Combat.MovementPrediction = 1
Playerbot.Combat.TargetPriorityMode = 2
# 0 = Simple, 1 = Advanced, 2 = Optimal

# Mythic+ Settings
Playerbot.MythicPlus.Enable = 1
Playerbot.MythicPlus.MaxKeyLevel = 20
Playerbot.MythicPlus.AffixHandling = 1
Playerbot.MythicPlus.RouteOptimization = 0

# Group Coordination
Playerbot.Group.EnableCoordination = 1
Playerbot.Group.InterruptRotation = 1
Playerbot.Group.DefensiveExternals = 1
Playerbot.Group.BattleResurrection = 1
Playerbot.Group.TankSwaps = 1

# Performance Tuning
Playerbot.Performance.CombatTickRate = 50
Playerbot.Performance.MaxTargets = 20
Playerbot.Performance.EnablePrediction = 1
Playerbot.Performance.CacheTimeout = 5000
```

## 10. CONCLUSION

The current PlayerBot implementation provides a foundation but lacks critical WoW 11.2 systems. Priority should focus on:

1. **Immediate (Week 1-2)**:
   - Hero Talent framework
   - Defensive cooldown intelligence
   - Interrupt coordination system

2. **Short-term (Week 3-4)**:
   - Stat weight calculations
   - Movement prediction system
   - Tank/Healer priority logic

3. **Medium-term (Week 5-8)**:
   - Mythic+ affix handling
   - Full group coordination
   - Resource pooling intelligence

4. **Long-term (Week 9-12)**:
   - Delve support
   - Mechanic learning system
   - PvP behavior patterns

Total estimated implementation time: 12-16 weeks for complete system.
Memory overhead: ~1.1MB additional per bot.
CPU overhead: ~40% increase during combat (still within 0.1% target).

The system must maintain TrinityCore's existing API compatibility while providing authentic WoW 11.2 combat behavior that passes the "Turing test" of being indistinguishable from human players in group content.