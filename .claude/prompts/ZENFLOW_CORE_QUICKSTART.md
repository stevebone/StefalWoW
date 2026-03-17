# Zenflow Quick Start: Core System Optimization

## Sofort starten

```bash
zenflow task create --name "core-system-optimization" --description "Vollumfängliche Core System Analyse - schnell & schlank"
```

## Erste Schritte

### 1. DI Interface Audit (KRITISCH)

Es gibt **90 DI Interfaces** - vermutlich sind viele ungenutzt!

```bash
# Zähle Registrierungen im ServiceContainer
grep -rn "RegisterSingleton<" src/modules/Playerbot/ | wc -l

# Zähle Resolve-Aufrufe
grep -rn "Resolve<I" src/modules/Playerbot/ | wc -l

# Liste alle Interfaces
ls src/modules/Playerbot/Core/DI/Interfaces/
```

### 2. Event System Analyse

Zwei Event-Systeme existieren parallel:

| System | Datei | LOC |
|--------|-------|-----|
| GenericEventBus | Core/Events/GenericEventBus.h | 901 |
| EventDispatcher | Core/Events/EventDispatcher.cpp | 297 |

**Frage**: Warum zwei? Können sie konsolidiert werden?

### 3. Manager Pattern Analyse

Drei Manager-Systeme überlappen:

| System | Datei | LOC |
|--------|-------|-----|
| ServiceContainer | Core/DI/ServiceContainer.h | 398 |
| GameSystemsManager | Core/Managers/GameSystemsManager.cpp | 695 |
| ManagerRegistry | Core/Managers/ManagerRegistry.cpp | 416 |

**Frage**: Können diese vereinheitlicht werden?

## Kern-Dateien lesen

```
PRIORITÄT 1:
Core/DI/ServiceContainer.h
Core/Events/GenericEventBus.h
Core/Managers/GameSystemsManager.cpp

PRIORITÄT 2:
Core/Events/EventDispatcher.cpp
Core/Managers/ManagerRegistry.cpp
Core/StateMachine/BotStateMachine.cpp

PRIORITÄT 3:
Core/DI/MIGRATION_GUIDE.md
Core/Threading/SafeGridOperations.h
```

## Analyse-Fokus: SCHNELL & SCHLANK

| Aspekt | Frage |
|--------|-------|
| **DI Overhead** | Sind vtable lookups in Hot Paths? |
| **Event Dispatch** | Async vs Sync - Performance Impact? |
| **Template Bloat** | GenericEventBus 901 LOC - Compile-Time? |
| **Lock Contention** | Mutex in ServiceContainer nötig? |
| **Dead Interfaces** | Wie viele der 90 sind ungenutzt? |

## Erwartete Reports

1. `ARCHITECTURE_ANALYSIS.md`
2. `PERFORMANCE_REPORT.md`
3. `REDUNDANCY_REPORT.md`
4. `DI_INTERFACE_AUDIT.md`
5. `ENHANCEMENT_OPPORTUNITIES.md`
6. `RECOMMENDATIONS.md`
7. `IMPLEMENTATION_PLAN.md`

## Vollständige Task-Beschreibung

Siehe: `C:\TrinityBots\TrinityCore\.claude\prompts\ZENFLOW_CORE_SYSTEM_ANALYSIS.md`
