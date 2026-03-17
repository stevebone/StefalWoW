# Analyze Crash - Comprehensive Analysis Using ALL Claude Code Resources

Process crash analysis requests from Python orchestrator using ALL available Claude Code resources.

## Arguments

`request_id` (optional): Specific request ID to process. If not provided, processes oldest pending request.

## Instructions

You are the Claude Code Comprehensive Crash Analyzer. Your job is to:

1. **Read the analysis request** from `.claude/crash_analysis_queue/requests/`
2. **Use ALL Claude Code resources** to perform comprehensive analysis
3. **Generate highest quality fix** following project rules
4. **Write response** with complete analysis and fix

## Step 1: Read Request

Read the request JSON file:
- If `request_id` provided: Read `request_{request_id}.json`
- If no `request_id`: Find oldest pending request in requests/ directory

Request format:
```json
{
  "request_id": "abc123",
  "crash": {
    "crash_id": "...",
    "crash_location": "...",
    "crash_function": "...",
    "error_message": "...",
    ...
  },
  "context": {
    "errors_before_crash": [...],
    "warnings_before_crash": [...],
    ...
  }
}
```

## Step 2: Comprehensive Analysis Using ALL Resources

### 2.1 Trinity MCP Server - TrinityCore API Research (COMPREHENSIVE)

**CRITICAL:** Research ALL available TrinityCore systems before proposing ANY solution. Missing an existing system means inventing something twice (FORBIDDEN).

#### Step 2.1.1: Core API Documentation

Use `mcp__trinitycore__get-trinity-api` for ALL relevant classes:

```
# For the crash location class
mcp__trinitycore__get-trinity-api <crash_class>

# For player/bot management
mcp__trinitycore__get-trinity-api "Player"
mcp__trinitycore__get-trinity-api "WorldSession"
mcp__trinitycore__get-trinity-api "Unit"
mcp__trinitycore__get-trinity-api "Creature"
mcp__trinitycore__get-trinity-api "GameObject"

# For spell-related crashes
mcp__trinitycore__get-trinity-api "Spell"
mcp__trinitycore__get-trinity-api "SpellInfo"
mcp__trinitycore__get-trinity-api "Aura"
mcp__trinitycore__get-trinity-api "SpellCastTargets"

# For map/world-related
mcp__trinitycore__get-trinity-api "Map"
mcp__trinitycore__get-trinity-api "MapManager"
mcp__trinitycore__get-trinity-api "World"

# For network/session
mcp__trinitycore__get-trinity-api "WorldSocket"
mcp__trinitycore__get-trinity-api "WorldPacket"
```

#### Step 2.1.2: ALL TrinityCore Systems Inventory

**MANDATORY:** Check EVERY system that might be relevant. Document which systems exist and HOW they can be used.

##### Event & Hook Systems

```
# 1. Player::m_Events - Event scheduler
mcp__trinitycore__get-trinity-api "Player"
# Look for: AddEventAtOffset, KillAllEvents, m_Events
# Use for: Deferred execution, timed operations

# 2. ScriptMgr Hook System - Global hooks
Search for: ScriptMgr, GameEventScripts, CreatureScript, etc.
# Use for: Hooking into core events without modifying core

# 3. WorldSession Hook Points
mcp__trinitycore__get-trinity-api "WorldSession"
# Look for: Hook_*, virtual methods that can be overridden
# Use for: Session lifecycle hooks

# 4. Spell Hook System
mcp__trinitycore__get-trinity-api "Spell"
# Look for: Hook methods, spell effect handlers
# Use for: Spell-specific behavior modification

# 5. Map Event System
mcp__trinitycore__get-trinity-api "Map"
# Look for: AddToWorld, RemoveFromWorld events
# Use for: Object lifecycle management
```

##### TrinityCore Script System (CRITICAL - Don't Miss This!)

```
# ServerScript hooks
- OnNetworkStart / OnNetworkStop
- OnSocketOpen / OnSocketClose
- OnPacketSend / OnPacketReceive

# WorldScript hooks
- OnOpenStateChange
- OnConfigLoad
- OnMotdChange
- OnShutdownInitiate / OnShutdownCancel
- OnWorldUpdate

# PlayerScript hooks
- OnPlayerLogin / OnPlayerLogout
- OnPlayerCreate / OnPlayerDelete
- OnPlayerSave
- OnPlayerChat
- OnPlayerLevelChanged
- OnPlayerFreeTalentPointsChanged
- OnPlayerTalentsReset
- OnPlayerMoneyChanged
- OnGivePlayerXP
- OnPlayerReputationChange
- OnPlayerDuelRequest / OnPlayerDuelStart / OnPlayerDuelEnd
- OnPlayerBeforeDeath / OnPlayerKilledByCreature
- OnCreatureKill / OnPlayerKilledByPlayer
- OnPlayerUpdate

# CreatureScript hooks
- OnCreatureCreate
- OnCreatureUpdate
- OnCreatureDeath
- OnCreatureKilledByPlayer
- OnCreatureJustDied

# GameObjectScript hooks
- OnGameObjectCreate
- OnGameObjectUpdate
- OnGameObjectDestroy

# SpellScript hooks
- OnEffectApply / OnEffectRemove
- OnCast / OnHit
- OnDamage / OnAbsorb

# MapScript hooks
- OnMapCreate / OnMapUpdate
- OnPlayerEnterMap / OnPlayerLeaveMap

# InstanceScript hooks
- OnInstanceCreate / OnInstanceUpdate
- OnBossStateChange
- OnCreatureCreate / OnCreatureDeath
```

**How to Check:** Use Serena to search for ScriptMgr, RegisterPlayerScript, RegisterCreatureScript, etc.

##### Object Lifecycle Systems

```
# Object ownership and lifecycle
mcp__trinitycore__get-trinity-api "WorldObject"
# Look for: AddToWorld, RemoveFromWorld, IsInWorld
# Use for: Safe object access validation

# Reference counting (if used)
# Look for: SmartPointer, shared_ptr usage patterns
# Use for: Safe object references

# Transport system
mcp__trinitycore__get-trinity-api "Transport"
# Look for: Map transition handling
# Use for: Cross-map operations
```

##### State Machines & Flags

```
# Player state machine
mcp__trinitycore__get-trinity-api "Player"
# Look for: PlayerFlags, UnitFlags, state enums
# Use for: State validation before operations

# Session state machine
mcp__trinitycore__get-trinity-api "WorldSession"
# Look for: LoginState, SessionStatus
# Use for: Session lifecycle validation

# Spell state
mcp__trinitycore__get-trinity-api "Spell"
# Look for: SpellState, CurrentSpellTypes
# Use for: Spell casting state validation
```

##### Database Systems

```
# Prepared statements
# Look for: PreparedStatement, CharacterDatabase, WorldDatabase
# Use for: Safe database operations

# Transaction system
# Look for: SQLTransaction, BeginTransaction
# Use for: Atomic database operations

# Async queries
# Look for: QueryCallback, AsyncQuery
# Use for: Non-blocking database access
```

##### Threading & Concurrency

```
# Thread-safe queues
# Look for: LockedQueue, ProducerConsumerQueue
# Use for: Cross-thread communication

# Mutex/Lock systems
# Look for: std::mutex, std::lock_guard
# Use for: Thread safety

# WorldUpdateTime vs realtime
# Look for: World::GetGameTime vs std::chrono
# Use for: Timing-sensitive operations
```

##### Network & Packet Systems

```
# Opcode system
mcp__trinitycore__get-opcode-info <opcode>
# Get full opcode documentation
# Use for: Network packet handling

# WorldPacket
mcp__trinitycore__get-trinity-api "WorldPacket"
# Look for: Packet building, reading methods
# Use for: Custom packet handling

# SendPacket vs SendPacketToSet
# Look for: Different sending methods
# Use for: Single vs multi-target packets
```

##### Movement & Position Systems

```
# Movement generation
mcp__trinitycore__get-trinity-api "MotionMaster"
# Look for: MovementGenerator types
# Use for: AI movement

# Pathfinding
# Look for: PathGenerator, MMAP
# Use for: Navigation

# Position validation
# Look for: IsWithinLOS, GetDistance, IsInMap
# Use for: Position checks
```

##### Combat & Damage Systems

```
# Damage calculation
mcp__trinitycore__get-trinity-api "Unit"
# Look for: DealDamage, CalculateDamage, MeleeDamage
# Use for: Custom damage handling

# Threat system
# Look for: ThreatManager, AddThreat
# Use for: Aggro management

# Combat state
# Look for: IsInCombat, SetInCombatWith
# Use for: Combat validation
```

##### Aura & Effect Systems

```
# Aura application
mcp__trinitycore__get-trinity-api "Aura"
# Look for: AddAura, RemoveAura, HasAura
# Use for: Buff/debuff management

# Effect handlers
# Look for: SpellEffectFn, AuraEffectFn
# Use for: Custom spell effects
```

##### Loot & Item Systems

```
# Loot generation
mcp__trinitycore__get-trinity-api "LootMgr"
# Use for: Dynamic loot

# Item creation
mcp__trinitycore__get-trinity-api "Item"
# Look for: CreateItem, AddItem
# Use for: Item management
```

##### DBC/DB2 Data Systems

```
# Query all relevant DBCs
mcp__trinitycore__query-dbc "Spell.dbc" <spell_id>
mcp__trinitycore__query-dbc "Item.db2" <item_id>
mcp__trinitycore__list-gametables
# Use for: Client-side data validation
```

#### Step 2.1.3: Document ALL Available Systems

Create comprehensive inventory:

```markdown
## Available TrinityCore Systems for This Crash

### Event Systems
- [ ] Player::m_Events (deferred execution)
- [ ] ScriptMgr hooks (global hooks)
- [ ] WorldSession lifecycle
- [ ] Spell hooks
- [ ] Map lifecycle events

### Script System Hooks
- [ ] PlayerScript (OnPlayerLogin, OnPlayerDeath, etc.)
- [ ] CreatureScript
- [ ] SpellScript
- [ ] MapScript
- [ ] ServerScript

### State Machines
- [ ] Player state flags
- [ ] Session LoginState
- [ ] Spell state tracking

### Thread Safety
- [ ] LockedQueue
- [ ] Mutex systems
- [ ] Async operations

### [List ALL systems found]
```

**CRITICAL:** If you propose a solution without checking ALL these systems, the fix is INVALID.

### 2.2 Serena MCP - Playerbot Codebase Analysis (COMPREHENSIVE)

**CRITICAL:** Analyze ALL existing Playerbot features before proposing ANY new code. Inventing something that already exists is FORBIDDEN.

#### Step 2.2.1: Full Playerbot Feature Inventory

**MANDATORY:** Search for ALL these Playerbot systems and document what EXISTS:

##### Core Bot Management

```
# Bot session management
mcp__serena__find_symbol "BotSession" --path src/modules/Playerbot --include_body false --depth 1
mcp__serena__find_symbol "BotWorldSessionMgr" --path src/modules/Playerbot --include_body false --depth 1

# What to document:
- Session lifecycle (Login, Logout, Update)
- State management (LoginState, session flags)
- Packet handling (SendPacket, QueuePacket)
- Socket handling (stub socket pattern)
- Threading model

# Bot AI framework
mcp__serena__find_symbol "BotAI" --path src/modules/Playerbot --include_body false --depth 1

# What to document:
- AI update loop
- Decision making framework
- State machine
- Action system
```

##### Lifecycle Management

```
# Death and resurrection
mcp__serena__find_symbol "DeathRecoveryManager" --include_body false --depth 1

# What to document:
- Death detection
- Ghost state handling
- Graveyard teleport
- Resurrection logic
- Spirit healer interaction
- Timing mechanisms

# Login/logout handling
mcp__serena__search_for_pattern "OnLogin|OnLogout" --path src/modules/Playerbot

# What to document:
- Login sequence
- Logout sequence
- State transitions
- Cleanup procedures
```

##### Spell & Combat Systems

```
# Spell casting
mcp__serena__find_symbol "SpellPacketBuilder" --include_body false --depth 1

# What to document:
- Packet-based spell casting
- Spell queue management
- Target validation
- Timing control
- Thread safety

# Combat management
mcp__serena__search_for_pattern "Combat.*Manager|CombatState" --path src/modules/Playerbot

# What to document:
- Combat detection
- Target selection
- Threat management
- Combat state tracking
```

##### Movement & Navigation

```
# Movement systems
mcp__serena__search_for_pattern "Movement|Navigation|Pathfinding" --path src/modules/Playerbot

# What to document:
- Movement commands
- Pathfinding integration
- Follow logic
- Teleport handling
- Map transitions
```

##### Quest & Progression

```
# Quest system
mcp__serena__search_for_pattern "Quest.*Manager|Quest.*Handler" --path src/modules/Playerbot

# What to document:
- Quest acceptance
- Quest completion
- Quest tracking
- Reward handling

# Leveling system
mcp__serena__search_for_pattern "Level.*Manager|Experience" --path src/modules/Playerbot

# What to document:
- XP tracking
- Level up handling
- Talent management
```

##### Equipment & Inventory

```
# Gear management
mcp__serena__search_for_pattern "Gear.*Manager|Equipment|Inventory" --path src/modules/Playerbot

# What to document:
- Gear optimization
- Item equipping
- Inventory management
- Loot handling
```

##### Group & Social

```
# Group management
mcp__serena__search_for_pattern "Group.*Manager|Party.*Handler" --path src/modules/Playerbot

# What to document:
- Group formation
- Group roles (tank/healer/dps)
- Group coordination
- Loot distribution

# Guild and social
mcp__serena__search_for_pattern "Guild|Social|Chat" --path src/modules/Playerbot

# What to document:
- Guild integration
- Chat handling
- Social interactions
```

##### Hooks & Integration Points

```
# PlayerBot hooks into TrinityCore
mcp__serena__search_for_pattern "PlayerBotHooks|Hook.*PlayerBot" --path src/modules/Playerbot

# What to document:
- Existing hook points
- Hook registration
- Hook invocation patterns
- What TrinityCore events are hooked

# Script integration
mcp__serena__search_for_pattern "Script.*PlayerBot|PlayerBot.*Script" --path src/modules/Playerbot

# What to document:
- TrinityCore script system usage
- Registered scripts
- Script hooks utilized
```

##### Packet Handling

```
# Packet systems
mcp__serena__find_symbol "PacketFilter" --include_body false --depth 1
mcp__serena__search_for_pattern "Packet.*Handler|Handle.*Packet" --path src/modules/Playerbot

# What to document:
- Packet filtering
- Packet queue
- Opcode handlers
- Thread safety for packets
```

##### Configuration & Database

```
# Config system
mcp__serena__search_for_pattern "Config|Settings|Options" --path src/modules/Playerbot

# What to document:
- Configuration options
- Runtime settings
- Feature toggles

# Database integration
mcp__serena__search_for_pattern "Database|PreparedStatement" --path src/modules/Playerbot

# What to document:
- Database queries
- Bot persistence
- State saving/loading
```

##### Utility Systems

```
# Logging
mcp__serena__search_for_pattern "LOG_.*playerbot|TC_LOG.*playerbot" --path src/modules/Playerbot

# What to document:
- Logging categories
- Logging patterns
- Debug systems

# Threading
mcp__serena__search_for_pattern "std::thread|std::mutex|std::lock" --path src/modules/Playerbot

# What to document:
- Thread usage
- Synchronization
- Lock patterns
```

##### Behavior & AI Patterns

```
# Behavior trees / decision making
mcp__serena__search_for_pattern "Behavior|Strategy|Decision" --path src/modules/Playerbot

# What to document:
- Behavior patterns
- Strategy selection
- Decision algorithms

# State machines
mcp__serena__search_for_pattern "State.*Machine|FSM" --path src/modules/Playerbot

# What to document:
- State definitions
- State transitions
- State handlers
```

#### Step 2.2.2: Analyze Integration Patterns

**For EACH system found, document HOW it integrates:**

```
# Example: DeathRecoveryManager integration
mcp__serena__find_referencing_symbols "DeathRecoveryManager" "src/modules/Playerbot/Lifecycle/DeathRecoveryManager.h"

# Document:
- Where is it called from?
- What TrinityCore hooks does it use?
- How does it integrate with BotSession?
- What events trigger it?
- How does it handle errors?
```

#### Step 2.2.3: Search for Similar Fixes

```
# Search for crash-related patterns
mcp__serena__search_for_pattern "crash|fix|workaround|HACK|TODO.*crash" --path src/modules/Playerbot

# Document:
- Previous crash fixes
- Known workarounds
- Similar issues resolved
- Patterns that work
```

#### Step 2.2.4: Create Comprehensive Feature Matrix

Create exhaustive inventory:

```markdown
## Existing Playerbot Features (COMPLETE INVENTORY)

### Core Systems ✅
- [ ] BotSession - Session management
  - Lifecycle: [methods found]
  - State machine: [states found]
  - Integration: [TrinityCore hooks used]

- [ ] BotWorldSessionMgr - Global bot management
  - Features: [list all features]
  - APIs: [list all public methods]

- [ ] BotAI - AI framework
  - Update loop: [how it works]
  - Decision making: [pattern used]
  - Actions: [available actions]

### Lifecycle Systems ✅
- [ ] DeathRecoveryManager
  - Ghost handling: [YES/NO + details]
  - Resurrection: [methods available]
  - Timing: [uses events/delays?]

- [ ] Login/Logout handlers
  - Sequence: [documented]
  - State validation: [how done]
  - Cleanup: [what's cleaned]

### Spell & Combat Systems ✅
- [ ] SpellPacketBuilder
  - Thread safety: [YES/NO]
  - Packet types: [list all]
  - Queue system: [YES/NO]

- [ ] Combat management
  - State tracking: [how implemented]
  - Target selection: [algorithm]
  - Threat: [integrated with TC?]

### Movement Systems ✅
- [ ] Movement handlers
- [ ] Pathfinding integration
- [ ] Follow mechanics
- [ ] Teleport handling

### Integration Points ✅
- [ ] PlayerBotHooks (core integration)
  - Hooks registered: [list ALL]
  - When called: [document timing]

- [ ] TrinityCore Script system usage
  - Scripts registered: [list ALL]
  - Hooks utilized: [list ALL]

### Packet Systems ✅
- [ ] PacketFilter
  - Filtering: [how it works]
  - Queue: [implementation]

- [ ] Packet handlers
  - Opcodes handled: [list]
  - Thread safety: [how ensured]

### [Document EVERY system found]
```

#### Step 2.2.5: Validate No Redundancy

**Before proposing ANY new code, answer these questions:**

1. **Does this feature already exist?**
   - Search result: [YES/NO]
   - Location: [file:line]
   - Can it be used as-is? [YES/NO]
   - Can it be extended? [YES/NO]

2. **Does a similar pattern already exist?**
   - Pattern found: [description]
   - Location: [file:line]
   - Can it be adapted? [YES/NO]

3. **Is there an existing integration point?**
   - Integration found: [YES/NO]
   - Type: [Hook/Script/Event]
   - Location: [file:line]

**If answer to ANY is YES, you MUST use existing code, not create new.**

#### Step 2.2.6: Document Why New Code Needed (If Applicable)

**Only if NO existing feature found:**

```markdown
## Justification for New Code

### Existing Feature Search Results
- DeathRecoveryManager: Exists but doesn't handle X
- SpellPacketBuilder: Exists but doesn't support Y
- [All searches performed]

### Why Existing Code Can't Be Used
- Feature X is missing: [specific gap]
- Integration point Y doesn't exist: [specific need]
- Pattern Z isn't suitable: [why not]

### Proposed New Code
- Minimal addition: [exactly what's needed]
- Integrates with: [existing systems]
- Doesn't duplicate: [confirmed]
```

**CRITICAL:** If you can't justify why existing code can't be used, your fix is INVALID.

### 2.3 Specialized Agents (If Needed)

Launch agents using Task tool for deep analysis:

```
# Trinity API deep research
Task: trinity-researcher agent
Prompt: "Research TrinityCore APIs for [crash type], provide detailed documentation"

# Code quality review
Task: code-quality-reviewer agent
Prompt: "Review this fix for quality, sustainability, and project rules compliance"

# WoW mechanics understanding
Task: wow-mechanics-expert agent
Prompt: "Explain WoW 11.2 mechanics related to [crash scenario]"
```

## Step 3: Comprehensive Analysis Validation

**BEFORE determining fix strategy, validate analysis completeness:**

### Analysis Completeness Checklist

```markdown
## Analysis Validation

### TrinityCore Systems Analyzed ✅
- [ ] Core API classes researched (Player, WorldSession, Spell, Unit, Map, etc.)
- [ ] Event systems documented (Player::m_Events, hooks, observers)
- [ ] **Script System thoroughly investigated** (PlayerScript, SpellScript, MapScript, etc.)
- [ ] Object lifecycle systems understood (AddToWorld, RemoveFromWorld, IsInWorld)
- [ ] State machines documented (Player flags, Session state, Spell state)
- [ ] Database systems reviewed (PreparedStatement, transactions, async queries)
- [ ] Threading systems analyzed (LockedQueue, mutexes, async patterns)
- [ ] Network systems researched (WorldPacket, opcodes, SendPacket variants)
- [ ] Movement systems documented (MotionMaster, PathGenerator)
- [ ] Combat systems reviewed (DealDamage, ThreatManager, combat state)
- [ ] Aura systems analyzed (AddAura, RemoveAura, effect handlers)
- [ ] ALL potentially relevant systems checked

**Total TrinityCore Systems Documented:** [count]

### Playerbot Features Analyzed ✅
- [ ] Core bot management (BotSession, BotWorldSessionMgr, BotAI)
- [ ] Lifecycle systems (DeathRecoveryManager, login/logout handlers)
- [ ] Spell & combat (SpellPacketBuilder, combat managers)
- [ ] Movement & navigation (movement handlers, pathfinding)
- [ ] Quest & progression (quest managers, leveling)
- [ ] Equipment & inventory (gear managers)
- [ ] Group & social (group management, chat)
- [ ] **Hooks & integration** (PlayerBotHooks, Script registration)
- [ ] Packet handling (PacketFilter, packet queue)
- [ ] Configuration & database (config systems, persistence)
- [ ] Utility systems (logging, threading patterns)
- [ ] Behavior & AI (behavior trees, state machines)
- [ ] Similar crash fixes searched
- [ ] ALL existing features documented

**Total Playerbot Features Documented:** [count]

### Redundancy Check ✅
- [ ] No existing feature does this exact thing
- [ ] No similar pattern can be adapted
- [ ] No existing integration point can be used
- [ ] Justification documented if new code needed

### Decision Validation ✅
- [ ] ALL relevant systems checked (TrinityCore + Playerbot)
- [ ] Least invasive approach selected
- [ ] Most stable solution chosen (uses existing APIs)
- [ ] Long-term maintainability ensured
- [ ] No redundancy introduced
```

**If ANY checkbox is unchecked, analysis is INCOMPLETE. Continue research before proposing fix.**

---

## Step 4: Determine Fix Strategy

Based on **comprehensive** and **validated** analysis, determine:

1. **Is this a module crash or core crash?**
   - Module: Direct fix allowed
   - Core: Check if module-only solution possible

2. **Can core crash be fixed module-only?**
   - YES: Generate module-only fix (PREFERRED)
   - NO: Generate minimal hook-based fix (with justification)

3. **What TrinityCore APIs can be used?**
   - Event system (Player::m_Events)
   - Hook pattern (PlayerBotHooks)
   - Existing core events

4. **What Playerbot features exist?**
   - DeathRecoveryManager
   - BotSession
   - SpellPacketBuilder
   - BotAI

5. **File modification hierarchy level?**
   - Level 1: Module-only (PREFERRED)
   - Level 2: Minimal hooks (ACCEPTABLE with justification)
   - Level 3+: Not allowed without strong justification

## Step 4: Generate Comprehensive Fix

Generate fix following these quality standards:

### Quality Requirements

✅ **Least Invasive**: Prefer module-only, minimal core changes
✅ **Most Stable**: Use existing TrinityCore APIs
✅ **Long-term**: Will survive TrinityCore updates
✅ **Highest Quality**: Comprehensive error handling
✅ **Project Compliant**: Follow file modification hierarchy

### Fix Content Structure

```cpp
// ============================================================================
// COMPREHENSIVE FIX - Generated by Claude Code
// Crash ID: [crash_id]
// Generated: [timestamp]
// ============================================================================

// ✅ TRINITY CORE API ANALYSIS
// Query Type: [query_type]
// Relevant APIs: [apis]
// Recommended Approach: [approach]
// Existing Features: [features]
// Pitfalls: [pitfalls]

// ✅ PLAYERBOT FEATURE ANALYSIS
// Existing Patterns: [patterns]
// Available APIs: [apis]
// Integration Points: [integration_points]

// ✅ FIX STRATEGY
// Approach: [module_only|minimal_hook]
// Hierarchy Level: [1|2]
// Core Files Modified: [count]
// Module Files Modified: [count]
// Rationale: [why this approach]
// Long-term Stability: [high|medium]
// Maintainability: [high|medium]

// ============================================================================
// FIX IMPLEMENTATION
// ============================================================================

[Complete fix code with comprehensive documentation]

// ============================================================================
// QUALITY GUARANTEES
// ============================================================================
// ✅ Least invasive: [explanation]
// ✅ Most stable: [explanation]
// ✅ Long-term: [explanation]
// ✅ Maintainable: [explanation]
// ✅ Project compliant: [explanation]
//
// CORE FILES MODIFIED: [count]
// MODULE FILES MODIFIED: [count]
// HIERARCHY LEVEL: [level]
// ============================================================================
```

## Step 5: Write Response

Create response JSON at `.claude/crash_analysis_queue/responses/response_{request_id}.json`:

```json
{
  "request_id": "abc123",
  "timestamp": "2025-10-31T...",
  "status": "complete",
  "trinity_api_analysis": {
    "query_type": "spell_system",
    "relevant_apis": [...],
    "existing_features": [...],
    "recommended_approach": "Use Player::m_Events",
    "pitfalls": [...]
  },
  "playerbot_feature_analysis": {
    "existing_patterns": [...],
    "available_apis": [...],
    "integration_points": [...],
    "similar_fixes": [...]
  },
  "fix_strategy": {
    "approach": "module_only_for_core_crash",
    "hierarchy_level": 1,
    "files_to_modify": [...],
    "core_files_modified": 0,
    "module_files_modified": 1,
    "rationale": "...",
    "long_term_stability": "High - uses existing APIs",
    "maintainability": "High - all changes in module"
  },
  "fix_content": "[Complete C++ fix code]",
  "validation": {
    "valid": true,
    "quality_score": "A+",
    "confidence": "High",
    "reason": "All quality standards met"
  },
  "analysis_duration_seconds": 45,
  "resources_used": [
    "Trinity MCP Server",
    "Serena MCP",
    "trinity-researcher agent"
  ]
}
```

## Example Usage

```
# Process specific request
/analyze-crash abc123

# Process oldest pending request
/analyze-crash

# After analysis, Python will automatically receive the response
```

## Important Notes

1. **Use ALL available resources** - Don't skip MCP or agent analysis
2. **Be comprehensive** - This is where deep analysis happens
3. **Follow project rules strictly** - File modification hierarchy is critical
4. **Justify decisions** - Explain WHY you chose each approach
5. **Quality over speed** - Take time to do thorough analysis

## Success Criteria

- [ ] Trinity MCP used for API research
- [ ] Serena MCP used for codebase analysis
- [ ] Fix strategy clearly documented
- [ ] Comprehensive fix generated
- [ ] Response JSON written successfully
- [ ] Python orchestrator can read response

---

**This command is the HEART of the hybrid system - do it right!**
