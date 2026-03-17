# TODO Cleanup Categorization - 135 Items

## HIGH PRIORITY (Critical Functionality) - 25 Items

### Database Integration (3 items)
- [ ] Line 1: BotAccountMgr.cpp:703 - Implement database storage when BotDatabasePool is available
- [ ] Line 126-128: BotLifecycleMgr.cpp - Database access, insert events, cleanup events (3 TODOs)

### Resource Management - Player API Integration (10 items)
- [ ] Line 2: Action.cpp:89 - Implement proper cooldown checking with TrinityCore API
- [ ] Line 10: CombatSpecializationTemplate_WoW112.h:243 - Get haste from player stats
- [ ] Line 11: CooldownManager.cpp:715 - Apply haste calculations and spell-specific GCD modifications
- [ ] Line 12: CooldownManager.cpp:765 - Implement charge detection from spell data
- [ ] Line 13: CooldownManager.cpp:783 - Apply various cooldown reduction effects
- [ ] Line 14-19: BloodSpecialization.cpp - Rune consumption, counting, runic power, disease refresh, D&D tracking (6 TODOs)
- [ ] Line 121: RoleAssignment.cpp:553 - Get actual spec when available

### Thread Safety (3 items)
- [ ] Line 130: BotSpawner.h:181 - Replace _zoneMutex with lock-free hash map
- [ ] Line 131: BotSpawner.h:185 - Replace _botMutex with concurrent hash map
- [ ] Line 132: BotSpawner.h:190 - Replace _spawnQueueMutex with lock-free queue

### Core Functionality (9 items)
- [ ] Line 3: Action.cpp:182 - Implement proper SpellCastTargets when needed
- [ ] Line 5-7: BotAI.cpp - Action execution, possibility check, priority retrieval (3 TODOs)
- [ ] Line 8: ActionPriority.cpp:348 - Analyze enemy spell for interrupt priority
- [ ] Line 9: ActionPriority.cpp:373 - Implement threat calculation
- [ ] Line 104: TargetSelector.cpp:707 - Fix type_flags API (currently not available)
- [ ] Line 129: BotSpawner.cpp:599 - Add level/race/class filtering with proper query
- [ ] Line 133: BotWorldEntry.cpp:40 - Implement memory tracking

## MEDIUM PRIORITY (Feature Completeness) - 70 Items

### Demon Hunter Complete Implementation (47 items)
- Lines 21-79: DemonHunterAI.cpp and Enhanced files
  - Advanced functionality, specialization detection
  - Metamorphosis system (exit, decision, activation)
  - Resource management (pain/fury: spending, generation, checking, management, decay, retrieval)
  - Rotation logic (Havoc, Vengeance)
  - Ability implementations (Eye Beam, Chaos Strike, Blade Dance, Demon's Bite, Soul Cleave, Shear)
  - Soul fragment system (tracking, consumption)
  - Combat lifecycle (start, end)
  - Resource checks and consumption
  - Positioning and range calculation
  - Buff and cooldown management
  - AoE target gathering

### Class-Specific Feature Completion (11 items)
- [ ] Line 81: BeastMasterySpecialization.cpp:1034 - Implement pet feeding
- [ ] Line 82-88: HunterAI.cpp - Combat systems, positioning, shared abilities, tracking, aspects, tank detection, metrics (7 TODOs)
- [ ] Line 89-94: MonkAI.cpp - Combat rotation, buff management, cooldown management, resource checking/consumption (6 TODOs)
- [ ] Line 99: AfflictionWarlockRefactored.h:435 - Multi-target DoT tracking
- [ ] Line 100: DestructionWarlockRefactored.h:384 - Find secondary target for Havoc

### Group System Features (12 items)
- [ ] Line 111-114: GroupCoordination.cpp - Tank threat, healer, DPS, support coordination (4 TODOs)
- [ ] Line 115-120: GroupFormation.cpp - Formation algorithms (wedge, diamond, defensive square, arrow), collision resolution, flexibility (6 TODOs)
- [ ] Line 122-125: RoleAssignment.cpp - Gear scoring, synergy calculation, gear analysis, experience scores (4 TODOs)

## LOW PRIORITY (Deferred/Phase Tracking) - 40 Items

### Template Refactoring Issues (6 items) - BLOCKED
- [ ] Line 47: DemonHunterAI.cpp:300 - Re-enable refactored specialization classes
- [ ] Line 80: DruidAI.cpp:61 - Re-enable refactored specialization classes
- [ ] Line 95: PaladinAI.cpp:184 - Re-enable refactored specialization classes
- [ ] Line 97: PriestAI_Specialization.cpp:78 - Re-enable refactored specialization classes
- [ ] Line 98: ShamanAI_Specialization.cpp:50 - Re-enable refactored specialization classes
- [ ] Line 101: WarriorAI.cpp:178 - Re-enable refactored specialization classes

### Phase 4 Features (5 items) - DEFERRED
- [ ] Line 4: BotAI.cpp:431 - Add social interactions (chat, emotes)
- [ ] Line 102: CombatAIIntegrator.cpp:795 - Implement looting system
- [ ] Line 106-108: EnhancedBotAI.cpp - Questing logic, social interactions, looting (3 TODOs)

### Performance Optimizations (5 items) - DEFERRED
- [ ] Line 103: PositionManager.cpp:880 - Implement IsInAoEZone function
- [ ] Line 105: ThreatAbilities.cpp:202 - Filter by specialization if needed
- [ ] Line 109-110: BotCharacterDistribution.cpp - Gender preference weight processing (2 TODOs)
- [ ] Line 94: MonkAI.h:416 - Implement role detection
- [ ] Line 96: PriestAI.cpp:198 - CheckMajorCooldowns implementation

### Low Impact (24 items)
- [ ] Line 20: UnholySpecialization.cpp:1263 - Implement threat reduction if needed
- [ ] Line 134: PlayerbotModule.cpp:240 - Unregister event hooks
- [ ] Line 135: BotSession.cpp:378 - Future BotSocket implementation note

## Resolution Strategy

### Immediate Implementation (HIGH PRIORITY)
1. Resource Management TODOs - Use Player API (GetPower, ModifyPower, etc.)
2. Cooldown checking - Use SpellHistory API
3. Spec detection - Use Player::GetPrimaryTalentTree()
4. Thread safety - Evaluate if current mutexes are performance bottlenecks
5. Database TODOs - Keep as placeholders until PBDB system is ready

### Feature Complete (MEDIUM PRIORITY)
1. Demon Hunter - Complete stub implementation with proper resource tracking
2. Hunter/Monk features - Implement using existing patterns from other classes
3. Group system - Implement formation algorithms with proper pathfinding

### Document & Defer (LOW PRIORITY)
1. Template issues - Track separately, architectural decision needed
2. Phase 4 features - Move to Phase 4 planning document
3. Performance opts - Profile first, optimize if needed
4. Low impact - Remove or convert to design notes
