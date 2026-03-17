# Playerbot Development Backlog

This file tracks planned features and architectural improvements for future development after current refactoring is complete.

---

## Priority: High

### RAID-001: Raid Encounter Architecture

**Status:** Planned
**Added:** 2026-01-26
**Context:** Phase 3 Event-Driven Migration discussion

**Problem Statement:**
Raid boss fights require dynamic behavior changes that current combat managers don't support:
- Formation changes during combat phases
- Raid split into subgroups (e.g., Thaddius platforms, Festergut spore groups)
- Position adaptation for boss mechanics (void zones, cleaves)
- Kiting for specific boss abilities

**Proposed Architecture:**

Current managers (FormationManager, PositionManager, KitingManager) provide **capabilities** (how to do things), but raid encounters need **orchestration** (when and what to do).

**Hybrid Approach:**
1. **Managers = Capabilities Layer** (existing)
   - FormationManager: How to form up
   - PositionManager: How to position
   - KitingManager: How to kite

2. **RaidEncounterCoordinator = Orchestration Layer** (NEW)
   - Subscribes to encounter events (ENCOUNTER_START, BOSS_PHASE_CHANGE, SPELL_CAST)
   - Loads boss-specific scripts
   - Tells managers WHAT to do and WHEN

3. **SubgroupManager** (NEW)
   - Handles raid split mechanics
   - Assigns bots to subgroups dynamically
   - Coordinates group-specific positioning

**Example Flow (Festergut ICC):**
```
1. ENCOUNTER_START (Festergut)
   └─> RaidEncounterCoordinator loads Festergut script

2. SPELL_CAST (Gaseous Blight - Phase 1)
   └─> Script tells FormationManager: "spread formation, 8 yard spacing"
   └─> Script tells PositionManager: "ranged stay at max range"

3. DEBUFF_APPLIED (Inoculated on 3 players)
   └─> Script tells SubgroupManager: "create spore group with inoculated players"
   └─> Script tells PositionManager: "spore group collapse to marked position"

4. BOSS_PHASE_CHANGE (Pungent Blight incoming)
   └─> Script tells PositionManager: "all stack on tank"
```

**Implementation Tasks:**
- [ ] Create `RaidEncounterCoordinator` class
- [ ] Create `SubgroupManager` class
- [ ] Add event-driven commands to FormationManager
- [ ] Add event-driven commands to PositionManager
- [ ] Add event-driven commands to KitingManager
- [ ] Create encounter event types (ENCOUNTER_START, BOSS_PHASE_CHANGE, etc.)
- [ ] Design boss script loading system
- [ ] Implement example scripts (Festergut, Thaddius, etc.)

**Dependencies:**
- Phase 3 Event-Driven Migration (COMPLETE)
- CombatEventRouter infrastructure (COMPLETE)

---

## Priority: Medium

(Empty - add items as needed)

---

## Priority: Low

(Empty - add items as needed)

---

## Completed Items

(Move items here when done)

---

## Notes

- Focus on completing current refactoring before implementing new features
- Raid encounter system should leverage existing event-driven infrastructure from Phase 3
