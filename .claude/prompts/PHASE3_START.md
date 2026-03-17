# Phase 3: Event-Driven Migration - Claude Code Start Prompt

Copy this entire prompt into Claude Code to begin Phase 3 implementation:

---

## PROMPT START

Phase 1 and Phase 2 are complete. Now implement Phase 3: Event-Driven Migration.

### Context
- **Repository**: `C:\TrinityBots\TrinityCore`
- **Detailed Plan**: Read `.claude/prompts/PHASE3_EVENT_DRIVEN_MIGRATION.md` for full specifications
- **Master Plan**: Read `.claude/plans/COMBAT_REFACTORING_MASTER_PLAN.md`

### Phase 3 Goal
Transform the combat system from 100% polling to event-driven architecture. This eliminates constant checking and enables reactive behavior, reducing CPU usage by 60-90%.

**Current**: Every component polls every frame/tick for changes  
**Target**: Components react only when relevant events occur

---

### Phase 3 Tasks (Execute in Order)

---

**Task 3.1: Create Event Infrastructure** (16h)

Create the event system foundation.

**Files to CREATE** in `src/modules/Playerbot/Core/Events/`:

1. **CombatEventType.h** - Event type enum with bitmask support:
```cpp
enum class CombatEventType : uint32 {
    DAMAGE_TAKEN = 0x0001, DAMAGE_DEALT = 0x0002,
    HEALING_DONE = 0x0020,
    SPELL_CAST_START = 0x0100, SPELL_CAST_SUCCESS = 0x0200, SPELL_INTERRUPTED = 0x0800,
    THREAT_CHANGED = 0x1000, TAUNT_APPLIED = 0x2000,
    AURA_APPLIED = 0x10000, AURA_REMOVED = 0x20000,
    UNIT_DIED = 0x1000000,
    // Convenience masks
    ALL_SPELL = SPELL_CAST_START | SPELL_CAST_SUCCESS | SPELL_INTERRUPTED,
    ALL_AURA = AURA_APPLIED | AURA_REMOVED
};
// Add bitwise operators: |, &, HasFlag()
```

2. **CombatEvent.h** - Event data structure:
```cpp
struct CombatEvent {
    CombatEventType type;
    uint32 timestamp;
    ObjectGuid source, target;
    uint32 amount = 0;
    uint32 spellId = 0;
    const SpellInfo* spellInfo = nullptr;
    const Aura* aura = nullptr;
    float oldThreat = 0, newThreat = 0;
    
    // Factory methods
    static CombatEvent CreateDamageTaken(...);
    static CombatEvent CreateSpellCastStart(...);
    static CombatEvent CreateAuraApplied(...);
    // etc.
};
```

3. **ICombatEventSubscriber.h** - Subscriber interface:
```cpp
class ICombatEventSubscriber {
public:
    virtual void OnCombatEvent(const CombatEvent& event) = 0;
    virtual CombatEventType GetSubscribedEventTypes() const = 0;
    virtual int32 GetEventPriority() const { return 0; }
    virtual bool ShouldReceiveEvent(const CombatEvent& event) const { return true; }
};
```

4. **CombatEventRouter.h/.cpp** - Singleton event dispatcher:
```cpp
class CombatEventRouter {
public:
    static CombatEventRouter& Instance();
    void Subscribe(ICombatEventSubscriber* subscriber);
    void Unsubscribe(ICombatEventSubscriber* subscriber);
    void Dispatch(const CombatEvent& event);      // Immediate
    void QueueEvent(CombatEvent event);           // Deferred
    void ProcessQueuedEvents();                   // Call from main loop
private:
    std::unordered_map<CombatEventType, std::vector<ICombatEventSubscriber*>> _subscribers;
    std::queue<CombatEvent> _eventQueue;
    std::shared_mutex _subscriberMutex;
    std::mutex _queueMutex;
};
```

---

**Task 3.2: Add TrinityCore Hooks** (16h)

Add hooks to TrinityCore core files to generate events.

**Files to MODIFY** (wrap in `#ifdef PLAYERBOT`):

1. **Unit.cpp** - `DealDamage()`:
```cpp
#ifdef PLAYERBOT
auto event = Playerbot::CombatEvent::CreateDamageDealt(GetGUID(), victim->GetGUID(), damage, spellProto);
Playerbot::CombatEventRouter::Instance().QueueEvent(event);
#endif
```

2. **Spell.cpp** - `prepare()` and `finish()`:
```cpp
#ifdef PLAYERBOT
// In prepare():
auto event = Playerbot::CombatEvent::CreateSpellCastStart(m_caster->GetGUID(), m_spellInfo, targetGuid);
Playerbot::CombatEventRouter::Instance().Dispatch(event);  // Immediate for interrupts!
#endif
```

3. **SpellAuras.cpp** - `_ApplyAura()` and `_UnapplyAura()`:
```cpp
#ifdef PLAYERBOT
auto event = Playerbot::CombatEvent::CreateAuraApplied(GetGUID(), aura, casterGuid);
Playerbot::CombatEventRouter::Instance().QueueEvent(event);
#endif
```

4. **ThreatManager.cpp** - `AddThreat()`:
```cpp
#ifdef PLAYERBOT
auto event = Playerbot::CombatEvent::CreateThreatChanged(ownerGuid, victimGuid, oldThreat, newThreat);
Playerbot::CombatEventRouter::Instance().QueueEvent(event);
#endif
```

5. **World.cpp** - Main update loop:
```cpp
#ifdef PLAYERBOT
Playerbot::CombatEventRouter::Instance().ProcessQueuedEvents();
#endif
```

---

**Task 3.3: Convert InterruptCoordinator to Event-Driven** (12h)

**Files to modify**:
- `src/modules/Playerbot/AI/Combat/InterruptCoordinator.h`
- `src/modules/Playerbot/AI/Combat/InterruptCoordinator.cpp`

**Changes**:
1. Inherit from `ICombatEventSubscriber`
2. Implement `OnCombatEvent()`, `GetSubscribedEventTypes()`
3. Subscribe in `Initialize()`, unsubscribe in `Shutdown()`
4. Add event handlers:
   - `HandleSpellCastStart()` → Assign interrupter immediately
   - `HandleSpellInterrupted()` → Track success
   - `HandleSpellCastSuccess()` → Track missed interrupts
5. REMOVE most of `Update()` - keep only maintenance (1/sec)

```cpp
CombatEventType InterruptCoordinator::GetSubscribedEventTypes() const {
    return CombatEventType::SPELL_CAST_START | 
           CombatEventType::SPELL_INTERRUPTED | 
           CombatEventType::SPELL_CAST_SUCCESS;
}

void InterruptCoordinator::OnCombatEvent(const CombatEvent& event) {
    switch (event.type) {
        case CombatEventType::SPELL_CAST_START:
            if (IsInterruptible(event.spellInfo) && IsEnemy(event.source))
                AssignInterrupter(event.source, event.spellId);
            break;
        // ... etc
    }
}
```

---

**Task 3.4: Convert ThreatCoordinator to Event-Driven** (16h)

**Files to modify**:
- `src/modules/Playerbot/AI/Combat/ThreatCoordinator.h`
- `src/modules/Playerbot/AI/Combat/ThreatCoordinator.cpp`

**Subscribe to**:
- `DAMAGE_TAKEN` - Update threat for victim
- `DAMAGE_DEALT` - Update threat for attacker
- `HEALING_DONE` - Update threat for healer
- `THREAT_CHANGED` - Direct threat update
- `TAUNT_APPLIED` - Handle taunt

Keep 50ms emergency check for safety situations.

---

**Task 3.5: Convert CrowdControlManager to Event-Driven** (8h)

**Files to modify**:
- `src/modules/Playerbot/AI/Combat/CrowdControlManager.h`
- `src/modules/Playerbot/AI/Combat/CrowdControlManager.cpp`

**Subscribe to**:
- `AURA_APPLIED` - Track new CC, update DR
- `AURA_REMOVED` - Remove CC tracking

Filter to only CC-type auras.

---

**Task 3.6: Convert BotThreatManager to Event-Driven** (12h)

**Files to modify**:
- `src/modules/Playerbot/AI/Combat/BotThreatManager.h`
- `src/modules/Playerbot/AI/Combat/BotThreatManager.cpp`

**Subscribe to**:
- `DAMAGE_DEALT` - Add threat when this bot deals damage
- `HEALING_DONE` - Add threat when this bot heals
- `THREAT_CHANGED` - Direct update
- `UNIT_DIED` - Remove dead targets from threat table

**Important**: Use `ShouldReceiveEvent()` to filter events to only those involving this specific bot (via `_botGuid`).

---

### Instructions
1. Read the detailed plan first: `.claude/prompts/PHASE3_EVENT_DRIVEN_MIGRATION.md`
2. Execute tasks in order (3.1 → 3.2 → 3.3 → 3.4 → 3.5 → 3.6)
3. Task 3.1 MUST be complete before any other tasks
4. Task 3.2 MUST be complete before 3.3-3.6
5. Verify compilation after each task
6. Test event flow after Task 3.2
7. Commit after each working task

### Success Criteria
- [ ] Event infrastructure compiles and works
- [ ] TrinityCore hooks fire events correctly
- [ ] InterruptCoordinator reacts to SPELL_CAST_START
- [ ] ThreatCoordinator reacts to DAMAGE/THREAT events
- [ ] CrowdControlManager reacts to AURA events
- [ ] BotThreatManager filters events to own bot
- [ ] Update() methods reduced to maintenance only
- [ ] ~60% reduction in combat CPU usage
- [ ] Project compiles without errors

### Critical Notes

1. **SPELL_CAST_START must be Dispatch() not QueueEvent()** - Interrupts need immediate response
2. **All hooks must be wrapped in #ifdef PLAYERBOT** - Don't break non-bot builds
3. **Keep emergency checks** - 50ms safety polling for edge cases
4. **Test incrementally** - Each component should work before moving to next

Start with Task 3.1 (Create Event Infrastructure).

---

## PROMPT END
