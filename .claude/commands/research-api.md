---
description: Research TrinityCore APIs, game mechanics, or workflows using both MCP (game knowledge) and Serena (C++ implementation)
---

# TrinityCore API Research Command

You are researching a TrinityCore API, game mechanic, workflow, or bot feature. This command enforces the **mandatory dual-research protocol** using both MCP and Serena tools.

---

## Research Protocol

Follow this **exact sequence** for every research task:

### Phase 1: Game Knowledge (MCP) - ALWAYS FIRST

Use TrinityCore MCP tools to understand WoW game mechanics context.

#### Select Appropriate MCP Tool:

**For Workflows/Patterns:**
```typescript
mcp__trinitycore__get-trinity-workflow <workflow-name>
```
Examples: "bot-death-recovery", "graveyard-resurrection", "combat-rotation"

**For Spells:**
```typescript
mcp__trinitycore__get-spell-info <spell-id>
```
Examples: 8326 (Ghost), 15007 (Resurrection Sickness), 2641 (Dismiss Pet)

**For Creatures/NPCs:**
```typescript
mcp__trinitycore__get-creature-full-info <creature-entry>
```
Examples: 6491 (Spirit Healer), 32638 (Hakmud of Argus)

**For Quests:**
```typescript
mcp__trinitycore__get-quest-info <quest-id>
```

**For DBC/DB2 Data:**
```typescript
mcp__trinitycore__query-dbc <dbc-file> <record-id>
```
Examples: Spell.dbc, ChrRaces.dbc, ChrClasses.dbc

**For APIs:**
```typescript
mcp__trinitycore__get-trinity-api <class-name>
```
Examples: Player, Creature, Unit, Spell, Aura

**For Combat/Stats:**
```typescript
mcp__trinitycore__get-combat-rating <level> <stat-name>
mcp__trinitycore__get-character-stats <level>
```

#### Analyze MCP Results:

Record the following from MCP responses:
- [ ] Spell IDs involved
- [ ] Creature entries needed
- [ ] Zone IDs referenced
- [ ] Quest IDs involved
- [ ] Workflow steps
- [ ] Common pitfalls
- [ ] Best practices
- [ ] DBC/DB2 references
- [ ] Database table references

---

### Phase 2: Implementation Details (Serena) - ALWAYS SECOND

Use Serena to understand the C++ implementation.

#### Find Class/Method Definitions:

```typescript
mcp__serena__find_symbol "<ClassName>/<MethodName>" --include_body true
```
Examples:
- "Player/RepopAtGraveyard"
- "Creature/DespawnOrUnsummon"
- "Unit/CastSpell"

**Options:**
- `--include_body true` - Include full method implementation
- `--depth 1` - Include class members/methods (for class overview)

#### Find Usage Examples:

```typescript
mcp__serena__find_referencing_symbols "<ClassName>/<MethodName>"
```

This shows:
- Where the method is called
- How it's used in real code
- Common usage patterns

#### Search for Patterns:

```typescript
mcp__serena__search_for_pattern "<pattern>" --path <directory>
```
Examples:
- `"HandleMoveTeleportAck"` - Find teleport handling
- `"Ghost.*8326"` - Find Ghost spell usage
- `"DeathRecovery"` - Find death recovery code

**Useful Paths:**
- `src/server/game` - Core game logic
- `src/modules/Playerbot` - Bot module
- `src/server/game/Entities` - Player/Creature/Unit

#### Get File Overview (if needed):

```typescript
mcp__serena__get_symbols_overview "<file-path>"
```
Shows all classes and methods in a file without full implementation.

#### Analyze Serena Results:

Record the following from Serena responses:
- [ ] Method signatures
- [ ] Parameter types and meanings
- [ ] Return values
- [ ] Implementation logic
- [ ] Error handling
- [ ] Edge cases
- [ ] Real usage patterns
- [ ] Code locations (file:line)

---

### Phase 3: Synthesis (MANDATORY)

Combine both perspectives into **complete understanding**.

#### Present Results in This Format:

```markdown
## [Research Topic]

### 🎮 Game Mechanics (from MCP)
- **Spells:** [List spell IDs and names]
- **Creatures:** [List creature entries and names]
- **Zones:** [List zone IDs if applicable]
- **Workflow:** [High-level steps]
- **Common Pitfalls:** [Known issues from MCP]
- **Best Practices:** [Recommended patterns from MCP]

### 💻 C++ Implementation (from Serena)
- **Primary Methods:**
  - `ClassName::MethodName()` - [Purpose]
  - Location: [file:line]

- **Implementation Details:**
  [Key logic from C++ code]

- **Real Usage Examples:**
  [Code snippets from Serena references]

### ⚠️ Critical Implementation Notes
- [Combine insights from both MCP and Serena]
- [Highlight crashes, race conditions, timing issues]
- [Document required order of operations]

### ✅ Complete Implementation Example

```cpp
// [Complete working example combining MCP game knowledge + Serena C++ patterns]
```

### 🔗 Cross-References
- MCP Tools Used: [List]
- Serena Symbols: [List]
- Related Files: [List]
- Database Tables: [List if applicable]
```

---

## Quality Checklist

Before presenting your research answer, verify **ALL** checkboxes:

### MCP Research (Game Knowledge)
- [ ] Queried appropriate MCP tool(s)
- [ ] Identified all spell IDs
- [ ] Identified all creature entries
- [ ] Identified all zone/quest IDs (if applicable)
- [ ] Documented workflow steps
- [ ] Documented common pitfalls
- [ ] Documented best practices

### Serena Research (C++ Implementation)
- [ ] Found primary class/method definitions
- [ ] Included method signatures
- [ ] Found real usage examples
- [ ] Identified error handling patterns
- [ ] Identified edge cases
- [ ] Documented code locations (file:line)

### Synthesis
- [ ] Combined MCP + Serena perspectives
- [ ] Provided complete working example
- [ ] Highlighted critical implementation details
- [ ] Cross-referenced all sources

**If ANY checkbox is unchecked, your research is INCOMPLETE.**

---

## Anti-Patterns (FORBIDDEN)

### ❌ Using Only MCP
```
User: /research-api bot resurrection
Bad: [Only checks MCP, provides workflow but no C++ code]
```
**Why Wrong:** Missing implementation details, cannot verify current codebase

### ❌ Using Only Serena
```
User: /research-api ghost spell
Bad: [Only checks C++, finds code but misses spell ID 8326]
```
**Why Wrong:** Missing game context, no spell ID information

### ✅ Correct Approach
```
User: /research-api bot resurrection
Good:
1. MCP: Get workflow, spell 8326, creature 6491
2. Serena: Find BuildPlayerRepop(), RepopAtGraveyard()
3. Synthesize: Complete answer with both perspectives
```

---

## Example Usage

### Example 1: Simple API

```
/research-api Player TeleportTo
```

**Expected Process:**
1. MCP: Get Player API overview with TeleportTo description
2. Serena: Find Player::TeleportTo implementation
3. Serena: Find TeleportTo usage examples
4. Present: Complete answer with signature, logic, examples

---

### Example 2: Complex Workflow

```
/research-api bot death recovery
```

**Expected Process:**
1. MCP: Get bot-death-recovery workflow
2. MCP: Get spell 8326 (Ghost) info
3. MCP: Get creature 6491 (Spirit Healer) info
4. Serena: Find Player::BuildPlayerRepop()
5. Serena: Find Player::RepopAtGraveyard()
6. Serena: Find DeathRecoveryManager implementation
7. Present: Multi-section complete answer

---

### Example 3: Spell Research

```
/research-api spell 8326
```

**Expected Process:**
1. MCP: Get spell 8326 (Ghost) complete info
2. Serena: Search for "8326" in codebase
3. Serena: Find where Ghost spell is applied
4. Present: Spell data + C++ usage

---

### Example 4: Game Mechanic

```
/research-api ghost state mechanics
```

**Expected Process:**
1. MCP: Get Ghost spell (8326) info
2. MCP: Get workflow for becoming/exiting ghost
3. Serena: Find BuildPlayerRepop(), ResurrectPlayer()
4. Serena: Search for PLAYER_FLAGS_GHOST
5. Present: Complete ghost state explanation

---

## Enforcement

### Self-Correction Protocol

If you catch yourself using only ONE tool, **immediately stop and self-correct:**

```
⚠️ INCOMPLETE RESEARCH DETECTED

I used only [MCP/Serena] but this requires BOTH tools.

Correcting now by querying [missing tool]...

[Perform missing queries]

[Provide complete synthesis]
```

### Quality Validation

After providing your answer, perform self-validation:

```
## Research Quality Validation

✅ Used MCP for game context
✅ Used Serena for C++ implementation
✅ Included spell IDs / creature entries
✅ Included method signatures
✅ Included real usage examples
✅ Provided complete working code
✅ Documented common pitfalls

Research quality: COMPLETE
```

---

## Notes

- **Always start with MCP** to get game context first
- **Always follow with Serena** to get implementation details
- **Always synthesize** both perspectives
- **Never skip either tool** - this is a quality violation
- **Document all sources** - show which tools you used

---

## Command Variations

You can research:
- **APIs:** `/research-api Player GetHealth`
- **Workflows:** `/research-api death recovery`
- **Spells:** `/research-api spell 8326`
- **Creatures:** `/research-api creature 6491`
- **Mechanics:** `/research-api ghost state`
- **Features:** `/research-api bot resurrection`

All variations follow the same **MCP → Serena → Synthesis** protocol.

---

**This command enforces enterprise-grade research quality standards.**
**No shortcuts. No incomplete research. Both tools. Every time.**
