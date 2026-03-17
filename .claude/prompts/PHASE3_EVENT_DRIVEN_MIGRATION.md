# Phase 3: Event-Driven Migration - Detailed Implementation Plan

**Version**: 1.0  
**Date**: 2026-01-26  
**Prerequisites**: Phase 1 + Phase 2 complete  
**Duration**: ~4 weeks (160 hours)

---

## Overview

Phase 3 transforms the combat system from 100% polling to event-driven architecture. This is the largest and most impactful phase, reducing CPU usage by 60-90% for combat-related operations.

### Current State (100% Polling)

| Component | Update Pattern | Waste |
|-----------|---------------|-------|
| ThreatCoordinator | Poll every 100ms | Checks unchanged threat |
| InterruptCoordinator | Poll every frame | Scans for casts constantly |
| CrowdControlManager | Poll every 500ms | Checks CC status repeatedly |
| BotThreatManager | Poll every 500ms | Recalculates unchanged threat |

### Target State (Event-Driven)

| Component | Event Pattern | Benefit |
|-----------|--------------|---------|
| ThreatCoordinator | OnDamageTaken, OnHealingDone | Only updates on actual threat changes |
| InterruptCoordinator | OnSpellCastStart | Only activates when enemy casts |
| CrowdControlManager | OnAuraApplied/Removed | Only updates on CC changes |
| BotThreatManager | OnDamageDealt, OnThreatChanged | Only updates on threat events |

---

## Task 3.1: Create Event Infrastructure

**Priority**: P0 (Foundation for all other tasks)  
**Effort**: 16 hours  
**Dependencies**: Phase 2 complete

### Files to Create

```
src/modules/Playerbot/Core/Events/CombatEvent.h
src/modules/Playerbot/Core/Events/CombatEventType.h
src/modules/Playerbot/Core/Events/ICombatEventSubscriber.h
src/modules/Playerbot/Core/Events/CombatEventRouter.h
src/modules/Playerbot/Core/Events/CombatEventRouter.cpp
```

### Step 3.1.1: Create CombatEventType.h

```cpp
#pragma once

#include <cstdint>

namespace Playerbot {

enum class CombatEventType : uint32 {
    NONE = 0,
    
    // Damage Events (0x0001 - 0x000F)
    DAMAGE_TAKEN        = 0x0001,
    DAMAGE_DEALT        = 0x0002,
    DAMAGE_ABSORBED     = 0x0004,
    
    // Healing Events (0x0010 - 0x00F0)
    HEALING_RECEIVED    = 0x0010,
    HEALING_DONE        = 0x0020,
    OVERHEAL            = 0x0040,
    
    // Spell Events (0x0100 - 0x0F00)
    SPELL_CAST_START    = 0x0100,
    SPELL_CAST_SUCCESS  = 0x0200,
    SPELL_CAST_FAILED   = 0x0400,
    SPELL_INTERRUPTED   = 0x0800,
    
    // Threat Events (0x1000 - 0xF000)
    THREAT_CHANGED      = 0x1000,
    TAUNT_APPLIED       = 0x2000,
    THREAT_WIPE         = 0x4000,
    
    // Aura Events (0x10000 - 0xF0000)
    AURA_APPLIED        = 0x10000,
    AURA_REMOVED        = 0x20000,
    AURA_REFRESHED      = 0x40000,
    AURA_STACK_CHANGED  = 0x80000,
    
    // Combat State Events (0x100000 - 0xF00000)
    COMBAT_STARTED      = 0x100000,
    COMBAT_ENDED        = 0x200000,
    
    // Unit Events (0x1000000 - 0xF000000)
    UNIT_DIED           = 0x1000000,
    UNIT_RESURRECTED    = 0x2000000,
    UNIT_TARGET_CHANGED = 0x4000000,
    
    // Encounter Events (0x10000000 - 0xF0000000)
    ENCOUNTER_START     = 0x10000000,
    ENCOUNTER_END       = 0x20000000,
    BOSS_PHASE_CHANGED  = 0x40000000,
    
    // Convenience masks
    ALL_DAMAGE          = DAMAGE_TAKEN | DAMAGE_DEALT | DAMAGE_ABSORBED,
    ALL_HEALING         = HEALING_RECEIVED | HEALING_DONE | OVERHEAL,
    ALL_SPELL           = SPELL_CAST_START | SPELL_CAST_SUCCESS | SPELL_CAST_FAILED | SPELL_INTERRUPTED,
    ALL_THREAT          = THREAT_CHANGED | TAUNT_APPLIED | THREAT_WIPE,
    ALL_AURA            = AURA_APPLIED | AURA_REMOVED | AURA_REFRESHED | AURA_STACK_CHANGED,
    ALL_EVENTS          = 0xFFFFFFFF
};

// Enable bitwise operations
inline CombatEventType operator|(CombatEventType a, CombatEventType b) {
    return static_cast<CombatEventType>(static_cast<uint32>(a) | static_cast<uint32>(b));
}

inline CombatEventType operator&(CombatEventType a, CombatEventType b) {
    return static_cast<CombatEventType>(static_cast<uint32>(a) & static_cast<uint32>(b));
}

inline bool HasFlag(CombatEventType mask, CombatEventType flag) {
    return (static_cast<uint32>(mask) & static_cast<uint32>(flag)) != 0;
}

} // namespace Playerbot
```

### Step 3.1.2: Create CombatEvent.h

```cpp
#pragma once

#include "CombatEventType.h"
#include "ObjectGuid.h"
#include <cstdint>

class SpellInfo;
class Aura;
class Unit;

namespace Playerbot {

struct CombatEvent {
    // Event identification
    CombatEventType type = CombatEventType::NONE;
    uint32 timestamp = 0;
    
    // Participants
    ObjectGuid source;      // Who caused the event
    ObjectGuid target;      // Who was affected
    
    // Damage/Healing data
    uint32 amount = 0;
    uint32 overkill = 0;    // Overkill damage or overheal
    uint32 absorbed = 0;    // Absorbed amount
    uint32 resisted = 0;    // Resisted amount
    bool isCritical = false;
    
    // Spell data
    uint32 spellId = 0;
    const SpellInfo* spellInfo = nullptr;
    
    // Aura data
    const Aura* aura = nullptr;
    uint8 auraStacks = 0;
    uint32 auraDuration = 0;
    
    // Threat data
    float oldThreat = 0.0f;
    float newThreat = 0.0f;
    float threatDelta = 0.0f;
    
    // Encounter data
    uint32 encounterId = 0;
    uint8 encounterPhase = 0;
    
    // Utility
    bool IsValid() const { return type != CombatEventType::NONE; }
    bool IsDamageEvent() const { return HasFlag(type, CombatEventType::ALL_DAMAGE); }
    bool IsHealingEvent() const { return HasFlag(type, CombatEventType::ALL_HEALING); }
    bool IsSpellEvent() const { return HasFlag(type, CombatEventType::ALL_SPELL); }
    bool IsThreatEvent() const { return HasFlag(type, CombatEventType::ALL_THREAT); }
    bool IsAuraEvent() const { return HasFlag(type, CombatEventType::ALL_AURA); }
    
    // Factory methods for common events
    static CombatEvent CreateDamageTaken(ObjectGuid victim, ObjectGuid attacker, 
                                          uint32 damage, uint32 overkill = 0,
                                          const SpellInfo* spell = nullptr);
    
    static CombatEvent CreateDamageDealt(ObjectGuid attacker, ObjectGuid victim,
                                          uint32 damage, const SpellInfo* spell = nullptr);
    
    static CombatEvent CreateHealingDone(ObjectGuid healer, ObjectGuid target,
                                          uint32 healing, uint32 overheal = 0,
                                          const SpellInfo* spell = nullptr);
    
    static CombatEvent CreateSpellCastStart(ObjectGuid caster, const SpellInfo* spell,
                                             ObjectGuid target = ObjectGuid::Empty);
    
    static CombatEvent CreateSpellCastSuccess(ObjectGuid caster, const SpellInfo* spell);
    
    static CombatEvent CreateSpellInterrupted(ObjectGuid caster, const SpellInfo* spell,
                                               ObjectGuid interrupter);
    
    static CombatEvent CreateAuraApplied(ObjectGuid target, const Aura* aura,
                                          ObjectGuid caster);
    
    static CombatEvent CreateAuraRemoved(ObjectGuid target, const Aura* aura);
    
    static CombatEvent CreateThreatChanged(ObjectGuid unit, ObjectGuid target,
                                            float oldThreat, float newThreat);
    
    static CombatEvent CreateUnitDied(ObjectGuid unit, ObjectGuid killer = ObjectGuid::Empty);
};

} // namespace Playerbot
```

### Step 3.1.3: Create ICombatEventSubscriber.h

```cpp
#pragma once

#include "CombatEventType.h"

namespace Playerbot {

struct CombatEvent;

class ICombatEventSubscriber {
public:
    virtual ~ICombatEventSubscriber() = default;
    
    // Called when a subscribed event occurs
    virtual void OnCombatEvent(const CombatEvent& event) = 0;
    
    // Return bitmask of event types this subscriber wants
    virtual CombatEventType GetSubscribedEventTypes() const = 0;
    
    // Optional: Priority for event delivery (higher = earlier)
    virtual int32 GetEventPriority() const { return 0; }
    
    // Optional: Filter events by source/target
    virtual bool ShouldReceiveEvent(const CombatEvent& event) const { return true; }
};

} // namespace Playerbot
```

### Step 3.1.4: Create CombatEventRouter.h

```cpp
#pragma once

#include "CombatEventType.h"
#include "CombatEvent.h"
#include "ICombatEventSubscriber.h"
#include <vector>
#include <unordered_map>
#include <shared_mutex>
#include <queue>
#include <atomic>

namespace Playerbot {

class CombatEventRouter {
public:
    // Singleton access
    static CombatEventRouter& Instance();
    
    // Prevent copying
    CombatEventRouter(const CombatEventRouter&) = delete;
    CombatEventRouter& operator=(const CombatEventRouter&) = delete;
    
    // Subscription management
    void Subscribe(ICombatEventSubscriber* subscriber);
    void Subscribe(ICombatEventSubscriber* subscriber, CombatEventType eventTypes);
    void Unsubscribe(ICombatEventSubscriber* subscriber);
    void UnsubscribeAll();
    
    // Event dispatch - immediate (call from main thread only)
    void Dispatch(const CombatEvent& event);
    
    // Event dispatch - queued (thread-safe, processed on main thread)
    void QueueEvent(CombatEvent event);
    void ProcessQueuedEvents();
    
    // Async dispatch for high-frequency events
    void DispatchAsync(CombatEvent event);
    
    // Statistics
    uint64 GetTotalEventsDispatched() const { return _totalEventsDispatched; }
    uint64 GetTotalEventsQueued() const { return _totalEventsQueued; }
    size_t GetSubscriberCount() const;
    size_t GetQueueSize() const;
    
    // Configuration
    void SetMaxQueueSize(size_t maxSize) { _maxQueueSize = maxSize; }
    void SetDropOldestOnOverflow(bool drop) { _dropOldestOnOverflow = drop; }
    
private:
    CombatEventRouter();
    ~CombatEventRouter();
    
    // Subscriber storage: eventType -> list of subscribers
    std::unordered_map<CombatEventType, std::vector<ICombatEventSubscriber*>> _subscribers;
    
    // All subscribers for quick unsubscribe
    std::vector<ICombatEventSubscriber*> _allSubscribers;
    
    // Thread safety
    mutable std::shared_mutex _subscriberMutex;
    
    // Event queue for async/deferred processing
    std::queue<CombatEvent> _eventQueue;
    mutable std::mutex _queueMutex;
    
    // Statistics
    std::atomic<uint64> _totalEventsDispatched{0};
    std::atomic<uint64> _totalEventsQueued{0};
    
    // Configuration
    size_t _maxQueueSize = 10000;
    bool _dropOldestOnOverflow = true;
    
    // Internal dispatch
    void DispatchToSubscribers(const CombatEvent& event);
    std::vector<ICombatEventSubscriber*> GetSubscribersForEvent(CombatEventType type);
};

// Convenience macros for TrinityCore hooks
#define DISPATCH_COMBAT_EVENT(event) \
    Playerbot::CombatEventRouter::Instance().Dispatch(event)

#define QUEUE_COMBAT_EVENT(event) \
    Playerbot::CombatEventRouter::Instance().QueueEvent(event)

} // namespace Playerbot
```

### Step 3.1.5: Create CombatEventRouter.cpp

```cpp
#include "CombatEventRouter.h"
#include "GameTime.h"
#include <algorithm>

namespace Playerbot {

CombatEventRouter& CombatEventRouter::Instance() {
    static CombatEventRouter instance;
    return instance;
}

CombatEventRouter::CombatEventRouter() = default;
CombatEventRouter::~CombatEventRouter() = default;

void CombatEventRouter::Subscribe(ICombatEventSubscriber* subscriber) {
    if (!subscriber) return;
    Subscribe(subscriber, subscriber->GetSubscribedEventTypes());
}

void CombatEventRouter::Subscribe(ICombatEventSubscriber* subscriber, CombatEventType eventTypes) {
    if (!subscriber) return;
    
    std::unique_lock lock(_subscriberMutex);
    
    // Add to all subscribers list if not already present
    auto it = std::find(_allSubscribers.begin(), _allSubscribers.end(), subscriber);
    if (it == _allSubscribers.end()) {
        _allSubscribers.push_back(subscriber);
    }
    
    // Add to each event type's subscriber list
    for (uint32 i = 0; i < 32; ++i) {
        CombatEventType eventType = static_cast<CombatEventType>(1u << i);
        if (HasFlag(eventTypes, eventType)) {
            auto& subs = _subscribers[eventType];
            if (std::find(subs.begin(), subs.end(), subscriber) == subs.end()) {
                subs.push_back(subscriber);
                // Sort by priority (higher priority first)
                std::sort(subs.begin(), subs.end(), 
                    [](ICombatEventSubscriber* a, ICombatEventSubscriber* b) {
                        return a->GetEventPriority() > b->GetEventPriority();
                    });
            }
        }
    }
}

void CombatEventRouter::Unsubscribe(ICombatEventSubscriber* subscriber) {
    if (!subscriber) return;
    
    std::unique_lock lock(_subscriberMutex);
    
    // Remove from all subscriber lists
    for (auto& [eventType, subs] : _subscribers) {
        subs.erase(std::remove(subs.begin(), subs.end(), subscriber), subs.end());
    }
    
    // Remove from all subscribers list
    _allSubscribers.erase(
        std::remove(_allSubscribers.begin(), _allSubscribers.end(), subscriber),
        _allSubscribers.end());
}

void CombatEventRouter::UnsubscribeAll() {
    std::unique_lock lock(_subscriberMutex);
    _subscribers.clear();
    _allSubscribers.clear();
}

void CombatEventRouter::Dispatch(const CombatEvent& event) {
    if (event.type == CombatEventType::NONE) return;
    
    DispatchToSubscribers(event);
    ++_totalEventsDispatched;
}

void CombatEventRouter::QueueEvent(CombatEvent event) {
    std::lock_guard lock(_queueMutex);
    
    if (_eventQueue.size() >= _maxQueueSize) {
        if (_dropOldestOnOverflow) {
            _eventQueue.pop();  // Drop oldest
        } else {
            return;  // Drop newest (don't queue)
        }
    }
    
    event.timestamp = GameTime::GetGameTimeMS();
    _eventQueue.push(std::move(event));
    ++_totalEventsQueued;
}

void CombatEventRouter::ProcessQueuedEvents() {
    std::queue<CombatEvent> eventsToProcess;
    
    {
        std::lock_guard lock(_queueMutex);
        std::swap(eventsToProcess, _eventQueue);
    }
    
    while (!eventsToProcess.empty()) {
        Dispatch(eventsToProcess.front());
        eventsToProcess.pop();
    }
}

void CombatEventRouter::DispatchAsync(CombatEvent event) {
    QueueEvent(std::move(event));
}

void CombatEventRouter::DispatchToSubscribers(const CombatEvent& event) {
    std::shared_lock lock(_subscriberMutex);
    
    auto it = _subscribers.find(event.type);
    if (it == _subscribers.end()) return;
    
    for (ICombatEventSubscriber* subscriber : it->second) {
        if (subscriber && subscriber->ShouldReceiveEvent(event)) {
            subscriber->OnCombatEvent(event);
        }
    }
}

std::vector<ICombatEventSubscriber*> CombatEventRouter::GetSubscribersForEvent(CombatEventType type) {
    std::shared_lock lock(_subscriberMutex);
    
    auto it = _subscribers.find(type);
    if (it != _subscribers.end()) {
        return it->second;
    }
    return {};
}

size_t CombatEventRouter::GetSubscriberCount() const {
    std::shared_lock lock(_subscriberMutex);
    return _allSubscribers.size();
}

size_t CombatEventRouter::GetQueueSize() const {
    std::lock_guard lock(_queueMutex);
    return _eventQueue.size();
}

} // namespace Playerbot
```

### Step 3.1.6: Create CombatEvent.cpp (Factory Methods)

```cpp
#include "CombatEvent.h"
#include "GameTime.h"

namespace Playerbot {

CombatEvent CombatEvent::CreateDamageTaken(ObjectGuid victim, ObjectGuid attacker,
                                            uint32 damage, uint32 overkill,
                                            const SpellInfo* spell) {
    CombatEvent event;
    event.type = CombatEventType::DAMAGE_TAKEN;
    event.timestamp = GameTime::GetGameTimeMS();
    event.target = victim;
    event.source = attacker;
    event.amount = damage;
    event.overkill = overkill;
    event.spellInfo = spell;
    event.spellId = spell ? spell->Id : 0;
    return event;
}

CombatEvent CombatEvent::CreateDamageDealt(ObjectGuid attacker, ObjectGuid victim,
                                            uint32 damage, const SpellInfo* spell) {
    CombatEvent event;
    event.type = CombatEventType::DAMAGE_DEALT;
    event.timestamp = GameTime::GetGameTimeMS();
    event.source = attacker;
    event.target = victim;
    event.amount = damage;
    event.spellInfo = spell;
    event.spellId = spell ? spell->Id : 0;
    return event;
}

CombatEvent CombatEvent::CreateHealingDone(ObjectGuid healer, ObjectGuid target,
                                            uint32 healing, uint32 overheal,
                                            const SpellInfo* spell) {
    CombatEvent event;
    event.type = CombatEventType::HEALING_DONE;
    event.timestamp = GameTime::GetGameTimeMS();
    event.source = healer;
    event.target = target;
    event.amount = healing;
    event.overkill = overheal;  // Using overkill field for overheal
    event.spellInfo = spell;
    event.spellId = spell ? spell->Id : 0;
    return event;
}

CombatEvent CombatEvent::CreateSpellCastStart(ObjectGuid caster, const SpellInfo* spell,
                                               ObjectGuid target) {
    CombatEvent event;
    event.type = CombatEventType::SPELL_CAST_START;
    event.timestamp = GameTime::GetGameTimeMS();
    event.source = caster;
    event.target = target;
    event.spellInfo = spell;
    event.spellId = spell ? spell->Id : 0;
    return event;
}

CombatEvent CombatEvent::CreateSpellCastSuccess(ObjectGuid caster, const SpellInfo* spell) {
    CombatEvent event;
    event.type = CombatEventType::SPELL_CAST_SUCCESS;
    event.timestamp = GameTime::GetGameTimeMS();
    event.source = caster;
    event.spellInfo = spell;
    event.spellId = spell ? spell->Id : 0;
    return event;
}

CombatEvent CombatEvent::CreateSpellInterrupted(ObjectGuid caster, const SpellInfo* spell,
                                                 ObjectGuid interrupter) {
    CombatEvent event;
    event.type = CombatEventType::SPELL_INTERRUPTED;
    event.timestamp = GameTime::GetGameTimeMS();
    event.source = interrupter;
    event.target = caster;
    event.spellInfo = spell;
    event.spellId = spell ? spell->Id : 0;
    return event;
}

CombatEvent CombatEvent::CreateAuraApplied(ObjectGuid target, const Aura* aura,
                                            ObjectGuid caster) {
    CombatEvent event;
    event.type = CombatEventType::AURA_APPLIED;
    event.timestamp = GameTime::GetGameTimeMS();
    event.target = target;
    event.source = caster;
    event.aura = aura;
    return event;
}

CombatEvent CombatEvent::CreateAuraRemoved(ObjectGuid target, const Aura* aura) {
    CombatEvent event;
    event.type = CombatEventType::AURA_REMOVED;
    event.timestamp = GameTime::GetGameTimeMS();
    event.target = target;
    event.aura = aura;
    return event;
}

CombatEvent CombatEvent::CreateThreatChanged(ObjectGuid unit, ObjectGuid target,
                                              float oldThreat, float newThreat) {
    CombatEvent event;
    event.type = CombatEventType::THREAT_CHANGED;
    event.timestamp = GameTime::GetGameTimeMS();
    event.source = unit;
    event.target = target;
    event.oldThreat = oldThreat;
    event.newThreat = newThreat;
    event.threatDelta = newThreat - oldThreat;
    return event;
}

CombatEvent CombatEvent::CreateUnitDied(ObjectGuid unit, ObjectGuid killer) {
    CombatEvent event;
    event.type = CombatEventType::UNIT_DIED;
    event.timestamp = GameTime::GetGameTimeMS();
    event.target = unit;
    event.source = killer;
    return event;
}

} // namespace Playerbot
```

### Verification for Task 3.1
- [ ] All 6 files created
- [ ] Project compiles without errors
- [ ] CombatEventRouter::Instance() works
- [ ] Subscribe/Unsubscribe works
- [ ] Dispatch delivers to subscribers
- [ ] QueueEvent and ProcessQueuedEvents work

---

## Task 3.2: Add TrinityCore Hooks

**Priority**: P0  
**Effort**: 16 hours  
**Dependencies**: Task 3.1 complete

### Files to Modify (TrinityCore Core)

```
src/server/game/Entities/Unit/Unit.cpp
src/server/game/Spells/Spell.cpp
src/server/game/Spells/Auras/SpellAuras.cpp
src/server/game/Combat/ThreatManager.cpp
```

### Step 3.2.1: Add Damage Hooks to Unit.cpp

Find `Unit::DealDamage()` and add hook:

```cpp
uint32 Unit::DealDamage(Unit* victim, uint32 damage, CleanDamage const* cleanDamage,
                        DamageEffectType damagetype, SpellSchoolMask damageSchoolMask,
                        SpellInfo const* spellProto, bool durabilityLoss) {
    // ... existing code at the START of function ...
    
    #ifdef PLAYERBOT
    // Playerbot event: Damage dealt
    if (IsPlayer() || victim->IsPlayer()) {
        auto event = Playerbot::CombatEvent::CreateDamageDealt(
            GetGUID(), victim->GetGUID(), damage, spellProto);
        Playerbot::CombatEventRouter::Instance().QueueEvent(event);
        
        auto eventTaken = Playerbot::CombatEvent::CreateDamageTaken(
            victim->GetGUID(), GetGUID(), damage, 0, spellProto);
        Playerbot::CombatEventRouter::Instance().QueueEvent(eventTaken);
    }
    #endif
    
    // ... rest of existing code ...
}
```

### Step 3.2.2: Add Healing Hooks to Unit.cpp

Find `Unit::HealBySpell()` or healing functions:

```cpp
uint32 Unit::HealBySpell(HealInfo& healInfo, bool critical) {
    // ... existing code ...
    
    uint32 heal = healInfo.GetHeal();
    
    #ifdef PLAYERBOT
    if (IsPlayer() || healInfo.GetTarget()->IsPlayer()) {
        auto event = Playerbot::CombatEvent::CreateHealingDone(
            GetGUID(), 
            healInfo.GetTarget()->GetGUID(),
            heal,
            healInfo.GetAbsorb(),  // overheal
            healInfo.GetSpellInfo());
        Playerbot::CombatEventRouter::Instance().QueueEvent(event);
    }
    #endif
    
    // ... rest of existing code ...
}
```

### Step 3.2.3: Add Spell Cast Hooks to Spell.cpp

Find `Spell::prepare()`:

```cpp
SpellCastResult Spell::prepare(SpellCastTargets const* targets, AuraEffect const* triggeredByAura) {
    // ... after initial validation, before cast starts ...
    
    #ifdef PLAYERBOT
    if (m_caster->IsPlayer() || (m_targets.GetUnitTarget() && m_targets.GetUnitTarget()->IsPlayer())) {
        auto event = Playerbot::CombatEvent::CreateSpellCastStart(
            m_caster->GetGUID(),
            m_spellInfo,
            m_targets.GetUnitTarget() ? m_targets.GetUnitTarget()->GetGUID() : ObjectGuid::Empty);
        Playerbot::CombatEventRouter::Instance().Dispatch(event);  // Immediate for interrupts
    }
    #endif
    
    // ... rest of existing code ...
}
```

Find `Spell::finish()` or spell completion:

```cpp
void Spell::finish(bool ok) {
    #ifdef PLAYERBOT
    if (ok && (m_caster->IsPlayer() || (m_targets.GetUnitTarget() && m_targets.GetUnitTarget()->IsPlayer()))) {
        auto event = Playerbot::CombatEvent::CreateSpellCastSuccess(
            m_caster->GetGUID(), m_spellInfo);
        Playerbot::CombatEventRouter::Instance().QueueEvent(event);
    }
    #endif
    
    // ... existing code ...
}
```

### Step 3.2.4: Add Interrupt Hook

Find where interrupts are processed (likely in `Unit::InterruptSpell()` or similar):

```cpp
void Unit::InterruptSpell(CurrentSpellTypes spellType, bool withDelayed, bool withInstant) {
    Spell* spell = m_currentSpells[spellType];
    if (spell) {
        #ifdef PLAYERBOT
        // Find who interrupted (this is the interrupter)
        auto event = Playerbot::CombatEvent::CreateSpellInterrupted(
            spell->m_caster->GetGUID(),
            spell->m_spellInfo,
            GetGUID());  // 'this' is the interrupter
        Playerbot::CombatEventRouter::Instance().Dispatch(event);
        #endif
    }
    
    // ... existing code ...
}
```

### Step 3.2.5: Add Aura Hooks to SpellAuras.cpp

Find aura application:

```cpp
void Unit::_ApplyAura(AuraApplication* aurApp, uint8 effMask) {
    // ... existing code ...
    
    #ifdef PLAYERBOT
    if (IsPlayer() || aurApp->GetBase()->GetCaster()->IsPlayer()) {
        auto event = Playerbot::CombatEvent::CreateAuraApplied(
            GetGUID(),
            aurApp->GetBase(),
            aurApp->GetBase()->GetCasterGUID());
        Playerbot::CombatEventRouter::Instance().QueueEvent(event);
    }
    #endif
    
    // ... rest of code ...
}
```

Find aura removal:

```cpp
void Unit::_UnapplyAura(AuraApplication* aurApp, AuraRemoveMode removeMode) {
    #ifdef PLAYERBOT
    if (IsPlayer()) {
        auto event = Playerbot::CombatEvent::CreateAuraRemoved(
            GetGUID(), aurApp->GetBase());
        Playerbot::CombatEventRouter::Instance().QueueEvent(event);
    }
    #endif
    
    // ... existing code ...
}
```

### Step 3.2.6: Add Threat Hooks to ThreatManager.cpp

Find threat modification:

```cpp
void ThreatManager::AddThreat(Unit* victim, float threat, SpellInfo const* spell, bool ignoreModifiers) {
    float oldThreat = GetThreat(victim);
    
    // ... existing threat calculation ...
    
    float newThreat = GetThreat(victim);
    
    #ifdef PLAYERBOT
    if (_owner->IsPlayer() || victim->IsPlayer()) {
        auto event = Playerbot::CombatEvent::CreateThreatChanged(
            _owner->GetGUID(), victim->GetGUID(), oldThreat, newThreat);
        Playerbot::CombatEventRouter::Instance().QueueEvent(event);
    }
    #endif
}
```

### Step 3.2.7: Add Death Hook

Find unit death handling:

```cpp
void Unit::Kill(Unit* victim, bool durabilityLoss) {
    // ... at the end of function ...
    
    #ifdef PLAYERBOT
    auto event = Playerbot::CombatEvent::CreateUnitDied(
        victim->GetGUID(), GetGUID());
    Playerbot::CombatEventRouter::Instance().Dispatch(event);
    #endif
}
```

### Step 3.2.8: Process Event Queue in World Update

Find main world update loop and add:

```cpp
void World::Update(uint32 diff) {
    // ... existing updates ...
    
    #ifdef PLAYERBOT
    // Process queued combat events on main thread
    Playerbot::CombatEventRouter::Instance().ProcessQueuedEvents();
    #endif
    
    // ... rest of updates ...
}
```

### Verification for Task 3.2
- [ ] All hooks compile with #ifdef PLAYERBOT
- [ ] Hooks don't break non-Playerbot builds
- [ ] Events fire correctly when testing
- [ ] No performance regression in combat

---

## Task 3.3: Convert InterruptCoordinator to Event-Driven

**Priority**: P0  
**Effort**: 12 hours  
**Dependencies**: Tasks 3.1 + 3.2 complete

### Files to Modify

```
src/modules/Playerbot/AI/Combat/InterruptCoordinator.h
src/modules/Playerbot/AI/Combat/InterruptCoordinator.cpp
```

### Step 3.3.1: Add ICombatEventSubscriber Interface

**InterruptCoordinator.h**:
```cpp
#include "Core/Events/ICombatEventSubscriber.h"
#include "Core/Events/CombatEventType.h"

class InterruptCoordinator : public ICombatEventSubscriber {
public:
    // ICombatEventSubscriber implementation
    void OnCombatEvent(const CombatEvent& event) override;
    CombatEventType GetSubscribedEventTypes() const override;
    int32 GetEventPriority() const override { return 100; }  // High priority
    
    // Existing public interface remains...
    
private:
    // Event handlers
    void HandleSpellCastStart(const CombatEvent& event);
    void HandleSpellInterrupted(const CombatEvent& event);
    void HandleSpellCastSuccess(const CombatEvent& event);
};
```

### Step 3.3.2: Implement Event Handling

**InterruptCoordinator.cpp**:
```cpp
#include "Core/Events/CombatEventRouter.h"
#include "Core/Events/CombatEvent.h"

void InterruptCoordinator::Initialize() {
    // ... existing initialization ...
    
    // Subscribe to spell events
    CombatEventRouter::Instance().Subscribe(this);
}

void InterruptCoordinator::Shutdown() {
    // Unsubscribe from events
    CombatEventRouter::Instance().Unsubscribe(this);
    
    // ... existing cleanup ...
}

CombatEventType InterruptCoordinator::GetSubscribedEventTypes() const {
    return CombatEventType::SPELL_CAST_START |
           CombatEventType::SPELL_INTERRUPTED |
           CombatEventType::SPELL_CAST_SUCCESS;
}

void InterruptCoordinator::OnCombatEvent(const CombatEvent& event) {
    switch (event.type) {
        case CombatEventType::SPELL_CAST_START:
            HandleSpellCastStart(event);
            break;
        case CombatEventType::SPELL_INTERRUPTED:
            HandleSpellInterrupted(event);
            break;
        case CombatEventType::SPELL_CAST_SUCCESS:
            HandleSpellCastSuccess(event);
            break;
        default:
            break;
    }
}

void InterruptCoordinator::HandleSpellCastStart(const CombatEvent& event) {
    if (!event.spellInfo) return;
    
    // Check if spell is interruptible
    if (!IsInterruptibleSpell(event.spellInfo)) return;
    
    // Check if caster is enemy to any group member
    if (!IsEnemyCaster(event.source)) return;
    
    // Register the cast and assign interrupter
    RegisterEnemyCast(event.source, event.spellId, event.spellInfo);
    AssignBestInterrupter(event.source, event.spellInfo);
}

void InterruptCoordinator::HandleSpellInterrupted(const CombatEvent& event) {
    // Cast was successfully interrupted
    OnCastInterrupted(event.target, event.spellId);  // target is the caster
    
    // Update metrics
    _metrics.successfulInterrupts++;
    
    // Check if we interrupted it
    if (WasAssignedToInterrupt(event.source, event.target, event.spellId)) {
        _metrics.ourInterrupts++;
    }
}

void InterruptCoordinator::HandleSpellCastSuccess(const CombatEvent& event) {
    // Cast completed - was it one we were supposed to interrupt?
    if (WasAssignedToInterrupt(event.source)) {
        // We missed the interrupt
        OnCastCompleted(event.source, event.spellId);
        _metrics.missedInterrupts++;
    }
}

// REMOVE or significantly reduce Update() polling
void InterruptCoordinator::Update(uint32 diff) {
    // Only keep minimal maintenance tasks
    // - Clean up expired assignments
    // - Update cooldown timers
    
    _maintenanceTimer += diff;
    if (_maintenanceTimer < 1000) return;  // Only once per second
    _maintenanceTimer = 0;
    
    CleanupExpiredAssignments();
    UpdateCooldowns(diff);
}
```

### Verification for Task 3.3
- [ ] InterruptCoordinator subscribes to events
- [ ] HandleSpellCastStart triggers on enemy casts
- [ ] Interrupt assignments happen via events, not polling
- [ ] Metrics track success/failure correctly
- [ ] Update() is minimal (maintenance only)

---

## Task 3.4: Convert ThreatCoordinator to Event-Driven

**Priority**: P0  
**Effort**: 16 hours  
**Dependencies**: Tasks 3.1 + 3.2 complete

### Files to Modify

```
src/modules/Playerbot/AI/Combat/ThreatCoordinator.h
src/modules/Playerbot/AI/Combat/ThreatCoordinator.cpp
```

### Implementation Pattern

Similar to Task 3.3, but subscribes to:
- `DAMAGE_TAKEN` - Update threat for victim
- `DAMAGE_DEALT` - Update threat for attacker  
- `HEALING_DONE` - Update threat for healer
- `THREAT_CHANGED` - Direct threat update
- `TAUNT_APPLIED` - Handle taunt

Keep periodic emergency check (50ms) for safety situations.

---

## Task 3.5: Convert CrowdControlManager to Event-Driven

**Priority**: P1  
**Effort**: 8 hours  
**Dependencies**: Tasks 3.1 + 3.2 complete

### Implementation Pattern

Subscribe to:
- `AURA_APPLIED` - Track new CC
- `AURA_REMOVED` - Remove CC tracking

Update DR on aura application (from Phase 2).

---

## Task 3.6: Convert BotThreatManager to Event-Driven

**Priority**: P1  
**Effort**: 12 hours  
**Dependencies**: Tasks 3.1 + 3.2 complete

### Implementation Pattern

Subscribe to (filtered to this bot only):
- `DAMAGE_DEALT` - Add threat
- `HEALING_DONE` - Add healing threat
- `THREAT_CHANGED` - Direct update
- `UNIT_DIED` - Remove from threat table

Use `ShouldReceiveEvent()` to filter events for this specific bot.

---

## Summary

| Task | Files | Key Changes | Effort |
|------|-------|-------------|--------|
| 3.1 | 6 new files | Event infrastructure | 16h |
| 3.2 | 4 TC files | TrinityCore hooks | 16h |
| 3.3 | InterruptCoordinator | Event-driven interrupts | 12h |
| 3.4 | ThreatCoordinator | Event-driven threat | 16h |
| 3.5 | CrowdControlManager | Event-driven CC | 8h |
| 3.6 | BotThreatManager | Event-driven per-bot threat | 12h |
| **Total** | | | **80h** |

## Expected Outcomes

| Metric | Before | After |
|--------|--------|-------|
| Interrupt checks/sec | 100,000 | ~5,000 (events only) |
| Threat calculations/sec | 50,000 | ~10,000 (events only) |
| CC status checks/sec | 10,000 | ~500 (events only) |
| CPU usage (combat) | 100% | ~40% |
| Event-driven components | 0% | 50% |
