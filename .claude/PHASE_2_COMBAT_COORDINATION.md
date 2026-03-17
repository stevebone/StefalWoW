# Phase 2: Combat Coordination Enhancement

**Duration**: 7 Days (Days 8-14)
**Priority**: HIGH - Advanced Group Combat
**Prerequisites**: Phase 1 Group Core Functionality Complete

## Overview

Phase 2 builds upon the basic group functionality from Phase 1 to create sophisticated combat coordination systems. Focus shifts from basic engagement to intelligent role-based combat tactics, interrupt coordination, and advanced threat management.

## Feature Enhancement Areas

### Day 8-9: Role-Based Combat Positioning
**Primary Agent**: `wow-mechanics-expert`
**Support Agents**: `wow-bot-behavior-designer`, `cpp-architecture-optimizer`

#### Implementation Goals
```cpp
// Enhanced Combat Positioning System
class RoleBasedCombatPositioning {
    Position CalculateTankPosition(Unit* target);
    Position CalculateHealerPosition(Group* group);
    Position CalculateDPSPosition(Unit* target, Role role);
    void HandlePositionalRequirements(Spell* spell);
};
```

**Tank Positioning Logic:**
- Face boss away from group
- Maintain threat positioning
- Handle cleave mechanics
- Emergency repositioning for AOE

**Healer Positioning Logic:**
- Maximum healing range optimization
- Line of sight maintenance
- Safety positioning from AOE
- Mana efficiency considerations

**DPS Positioning Logic:**
- Optimal damage range per spec
- Avoid frontal cleaves
- Stack for buffs when beneficial
- Spread for AOE mechanics

#### Advanced Features
```cpp
// Mechanical Awareness System
class MechanicAwareness {
    bool DetectAOECast(Unit* caster);
    void HandleCleaveMechanic(Unit* target);
    void RespondToPositionalRequirement(Spell* spell);
    Position CalculateSafePosition(Player* bot);
};
```

---

### Day 10-11: Interrupt Coordination System
**Primary Agent**: `wow-mechanics-expert`
**Support Agents**: `trinity-integration-tester`, `wow-bot-behavior-designer`

#### Implementation Goals
```cpp
// Group Interrupt Coordination
class InterruptCoordinator {
    bool ShouldInterrupt(Unit* target, Spell* spell);
    Player* SelectInterrupter(Unit* target);
    void CoordinateInterruptRotation();
    void HandleInterruptFailure(Unit* target);
};
```

**Interrupt Priority System:**
1. **Critical Heals** - Highest priority, multiple interrupts allowed
2. **Damage Spells** - High priority, rotate responsibilities
3. **Buffs/Utilities** - Medium priority, single interrupt sufficient
4. **Channeled Abilities** - Variable priority based on effect

**Rotation Algorithm:**
```cpp
struct InterruptAssignment {
    Player* assignedBot;
    uint32 spellId;
    uint32 cooldownRemaining;
    uint8 priority;
};

class InterruptRotationManager {
    std::vector<InterruptAssignment> activeAssignments;
    Player* GetNextInterrupter(uint8 priority);
    void UpdateCooldowns(uint32 diff);
};
```

#### Class-Specific Interrupt Integration
- **Warriors**: Pummel, Spell Reflection
- **Death Knights**: Mind Freeze, Death Grip
- **Demon Hunters**: Disrupt, Consume Magic
- **Rogues**: Kick, Blind
- **Mages**: Counterspell, Spellsteal
- **Shamans**: Wind Shear, Purge
- **Hunters**: Counter Shot, Binding Shot

---

### Day 12-13: Advanced Threat Management
**Primary Agent**: `wow-mechanics-expert`
**Support Agents**: `cpp-architecture-optimizer`, `resource-monitor-limiter`

#### Implementation Goals
```cpp
// Enhanced Threat Management System
class AdvancedThreatManager {
    float CalculateOptimalThreat(Player* bot, Unit* target);
    void ManageTankThreat(Player* tank, Group* group);
    void ManageDPSThreat(Player* dps, Unit* target);
    void HandleThreatEmergency(Player* bot);
};
```

**Tank Threat Strategies:**
```cpp
class TankThreatStrategy {
    void EstablishInitialThreat(Unit* target);
    void MaintainThreatLead(float dpsAggregation);
    void HandleThreatLoss(Unit* target);
    void ManageMultiTarget(std::vector<Unit*> targets);
};
```

**DPS Threat Management:**
```cpp
class DPSThreatStrategy {
    bool ShouldReduceThreat(Unit* target);
    void OptimizeDamageVsThreat(Unit* target);
    void HandleHighThreatSituation();
    void CoordinateWithTank(Player* tank);
};
```

#### Threat Calculation Improvements
- Real-time threat monitoring per target
- Predictive threat modeling for ability planning
- Cross-group threat comparison
- Emergency threat reduction protocols

---

### Day 14: Combat AI Integration & Testing
**Primary Agent**: `test-automation-engineer`
**Support Agents**: `trinity-integration-tester`, `wow-bot-behavior-designer`

#### Integration Tasks
```cpp
// Combat System Integration Hub
class CombatSystemIntegrator {
    void InitializeRoleStrategies(Player* bot);
    void UpdateCombatCoordination(uint32 diff);
    void HandleCombatStateChanges(CombatEvent event);
    void ValidateSystemIntegrity();
};
```

**Performance Integration:**
- Memory usage optimization for combat calculations
- CPU load balancing across group members
- Network packet optimization for coordination
- Database query reduction for combat lookups

#### Advanced Testing Scenarios
```cpp
class AdvancedCombatTests {
    void TestMultiTargetThreat();
    void TestInterruptRotation();
    void TestPositionalMechanics();
    void TestEmergencyProcedures();
    void TestPerformanceUnderLoad();
};
```

---

## Architecture Evolution

### Enhanced Combat State Machine
```cpp
enum class AdvancedCombatState {
    POSITIONING,        // Moving to optimal combat position
    ESTABLISHING,       // Building initial threat/engagement
    SUSTAINED_COMBAT,   // Normal rotation execution
    MECHANICAL_RESPONSE,// Responding to boss mechanics
    EMERGENCY_RECOVERY, // Handling critical situations
    COMBAT_TRANSITION   // Switching targets/phases
};
```

### System Integration Architecture
```
Phase 1 Group Core
├── GroupInvitationHandler
├── LeaderFollowBehavior
├── GroupCombatTrigger
└── TargetAssistance

Phase 2 Combat Enhancement (NEW)
├── RoleBasedCombatPositioning
├── InterruptCoordinator
├── AdvancedThreatManager
└── CombatSystemIntegrator

Integration Layer
├── TrinityCore Combat Hooks
├── Spell Database Interface
├── Threat System Integration
└── Movement System Coordination
```

---

## Performance Optimization

### Combat Calculation Optimization
```cpp
// Batch processing for group calculations
class CombatCalculationBatcher {
    void BatchThreatCalculations(Group* group);
    void BatchPositionUpdates(std::vector<Player*> bots);
    void BatchInterruptChecks(Unit* target);
};
```

### Memory Management
- Object pooling for combat calculations
- Cached positioning data
- Optimized spell lookup tables
- Reduced database queries through caching

### CPU Load Distribution
- Staggered update cycles across group members
- Priority-based processing queues
- Interrupt-driven emergency responses
- Background precalculation of common scenarios

---

## Quality Assurance

### Performance Benchmarks
- **Memory Increase**: <1MB per bot for Phase 2 features
- **CPU Impact**: <0.05% additional per bot
- **Response Times**:
  - Interrupt coordination: <200ms
  - Position adjustments: <500ms
  - Threat calculations: <100ms

### Testing Framework Expansion
```cpp
class Phase2TestSuite {
    // Role-based positioning tests
    void TestTankPositioning();
    void TestHealerPositioning();
    void TestDPSPositioning();

    // Interrupt system tests
    void TestInterruptPriority();
    void TestInterruptRotation();
    void TestInterruptFailover();

    // Threat management tests
    void TestThreatEstablishment();
    void TestThreatMaintenance();
    void TestThreatEmergencies();

    // Integration tests
    void TestFullGroupCombat();
    void TestMultiGroupScenarios();
    void TestPerformanceStress();
};
```

### Stress Testing Scenarios
- **5 Groups (25 bots)** fighting simultaneously
- **Complex boss mechanics** simulation
- **Network latency** impact testing
- **High CPU load** stability testing
- **Memory leak** detection over extended periods

---

## Integration Points

### TrinityCore Hooks Enhanced
```cpp
// Additional hooks for advanced combat
class CombatHookExtensions {
    void OnSpellCastStart(Unit* caster, Spell* spell);
    void OnThreatUpdate(Unit* target, Player* attacker, float threat);
    void OnPositionalRequirement(Spell* spell, Position required);
    void OnCombatMechanicTrigger(Unit* source, uint32 mechanicType);
};
```

### Database Schema Extensions
```sql
-- Enhanced bot combat preferences
CREATE TABLE bot_combat_preferences (
    bot_guid BIGINT UNSIGNED,
    role ENUM('TANK', 'HEALER', 'MELEE_DPS', 'RANGED_DPS'),
    interrupt_priority TINYINT,
    threat_sensitivity FLOAT,
    positioning_preference ENUM('AGGRESSIVE', 'BALANCED', 'CONSERVATIVE')
);
```

---

## Success Criteria

### Phase 2 Complete When:
- [x] **Role Coordination**: Tanks, healers, DPS perform role-specific functions
- [x] **Interrupt System**: Coordinated interrupts with <5% miss rate
- [x] **Threat Management**: Stable threat tables, no accidental pulls
- [x] **Performance**: All benchmarks met with enhanced features
- [x] **Integration**: Seamless operation with Phase 1 systems
- [x] **Testing**: 90%+ pass rate on advanced combat scenarios

### Demonstration Scenarios
1. **Tank and Spank**: Basic threat management with positioning
2. **Interrupt Heavy**: Coordinated interrupt rotation on multiple casters
3. **Positional Fight**: Movement coordination for cleave mechanics
4. **Multi-Target**: Threat management across multiple enemies
5. **Emergency Recovery**: Response to tank death or healer OOM

### Handoff to Phase 3
- Combat coordination system fully functional
- Performance baselines maintained
- Advanced testing framework established
- Architecture ready for individual intelligence enhancement
- Documentation complete for further development

**Phase 2 delivers sophisticated group combat capabilities while maintaining the testing-focused approach and performance standards established in Phase 1.**