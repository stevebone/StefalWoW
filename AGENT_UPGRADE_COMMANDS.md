# Agent Model Upgrade Commands

**Date:** 2025-09-30
**Target Model:** claude-sonnet-4-5-20250929

Use the `/agents` command to update each agent's model configuration.

---

## HIGH PRIORITY AGENTS (Upgrade to Sonnet 4.5)

### 1. cpp-server-debugger
```
/agents edit cpp-server-debugger
```
Set model to: `claude-sonnet-4-5-20250929`

### 2. cpp-architecture-optimizer
```
/agents edit cpp-architecture-optimizer
```
Set model to: `claude-sonnet-4-5-20250929`

### 3. concurrency-threading-specialist
```
/agents edit concurrency-threading-specialist
```
Set model to: `claude-sonnet-4-5-20250929`

### 4. wow-mechanics-expert
```
/agents edit wow-mechanics-expert
```
Set model to: `claude-sonnet-4-5-20250929`

### 5. playerbot-project-coordinator
```
/agents edit playerbot-project-coordinator
```
Set model to: `claude-sonnet-4-5-20250929`

### 6. wow-dungeon-raid-coordinator
```
/agents edit wow-dungeon-raid-coordinator
```
Set model to: `claude-sonnet-4-5-20250929`

### 7. pvp-arena-tactician
```
/agents edit pvp-arena-tactician
```
Set model to: `claude-sonnet-4-5-20250929`

### 8. bot-learning-system
```
/agents edit bot-learning-system
```
Set model to: `claude-sonnet-4-5-20250929`

---

## MEDIUM PRIORITY AGENTS (Consider Upgrading)

### 9. wow-bot-behavior-designer
```
/agents edit wow-bot-behavior-designer
```
Set model to: `claude-sonnet-4-5-20250929`

### 10. wow-economy-manager
```
/agents edit wow-economy-manager
```
Set model to: `claude-sonnet-4-5-20250929`

### 11. trinity-integration-tester
```
/agents edit trinity-integration-tester
```
Set model to: `claude-sonnet-4-5-20250929`

---

## Quick Reference: Model Identifiers

- **Sonnet 4.5:** `claude-sonnet-4-5-20250929`
- **Sonnet 3.5:** `claude-sonnet-3-5-20241022` (or similar)
- **Sonnet 3:** `claude-sonnet-3-20240229` (or similar)

---

## Verification

After updating, verify with:
```
/agents list
```

This will show all agents and their current model configurations.

---

## Rationale Summary

**HIGH Priority (8 agents):**
- Complex debugging, architecture, concurrency, game mechanics
- Multi-agent coordination, advanced strategy
- Machine learning systems
- Critical path activities requiring advanced reasoning

**MEDIUM Priority (3 agents):**
- AI behavior design (complex but iterative)
- Economy systems (nuanced but not critical)
- Integration testing (important but established patterns)

**LOW Priority (Keep current model):**
- Test automation, code review, database optimization
- Simple configuration tasks
- Well-defined, rule-based operations

---

## Cost Impact Estimate

- **Current:** All 18 agents on current model
- **After HIGH priority upgrade:** 8 agents on Sonnet 4.5, 10 on current = ~1.4X cost
- **After MEDIUM priority upgrade:** 11 agents on Sonnet 4.5, 7 on current = ~1.6X cost
- **Full upgrade:** All 18 on Sonnet 4.5 = ~2.0X cost

**Recommended:** Start with HIGH priority, evaluate results, then decide on MEDIUM priority.