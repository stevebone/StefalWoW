# ZenAI Prompt: Core System Optimization

Kopiere diesen Prompt in ZenAI um die Analyse zu starten:

---

## TASK: Core System - Vollumfängliche Analyse & Optimization

**Ziel**: System SCHNELL und SCHLANK machen

**Location**: `src/modules/Playerbot/Core/`

**Scope**: ~120 Dateien, ~15,000+ LOC

### Kritische Komponenten:

1. **DI Container** (`Core/DI/`)
   - ServiceContainer.h (398 LOC)
   - **90 Interface-Dateien** in `Core/DI/Interfaces/`
   - FRAGE: Wie viele Interfaces sind tatsächlich genutzt?

2. **Event System** (`Core/Events/`)
   - GenericEventBus.h (901 LOC) - Template-basiert
   - EventDispatcher.cpp (297 LOC) - Runtime-basiert
   - FRAGE: Warum existieren ZWEI Event-Systeme?

3. **Manager System** (`Core/Managers/`)
   - GameSystemsManager.cpp (695 LOC)
   - ManagerRegistry.cpp (416 LOC)
   - FRAGE: Überlappung mit ServiceContainer?

4. **State Machine** (`Core/StateMachine/`)
   - BotStateMachine.cpp (510 LOC)
   - BotInitStateMachine.cpp

### Analyse-Aufgaben:

1. **DI Interface Audit**: Für alle 90 Interfaces prüfen:
   - Ist es im ServiceContainer registriert?
   - Wird es per Resolve<>() aufgerufen?
   - Hat es mehr als eine Implementierung?
   - → Eliminierungskandidaten identifizieren

2. **Performance-Analyse**:
   - Hot Paths identifizieren
   - Indirection Overhead bewerten
   - Template Bloat in GenericEventBus?
   - Mutex-Contention in ServiceContainer?

3. **Redundanz-Analyse**:
   - EventBus vs EventDispatcher konsolidieren?
   - ServiceContainer vs GameSystemsManager vs ManagerRegistry?
   - Dead Code finden

4. **Enhancement-Möglichkeiten**:
   - Vereinfachungen vorschlagen
   - Lock-free Alternativen
   - Compile-Time Optimierungen

### Erwartete Deliverables:

- ARCHITECTURE_ANALYSIS.md
- PERFORMANCE_REPORT.md
- REDUNDANCY_REPORT.md
- DI_INTERFACE_AUDIT.md (Liste aller 90 Interfaces mit Status)
- ENHANCEMENT_OPPORTUNITIES.md
- RECOMMENDATIONS.md (P0/P1/P2/P3 priorisiert)
- IMPLEMENTATION_PLAN.md

### Success Criteria:

- Keine unnötigen Indirections in Hot Paths
- Minimale Interface-Anzahl (nur was gebraucht wird)
- Einheitliches Event-System (nicht zwei)
- Kein Dead Code

**Geschätzter Aufwand**: 40-60 Stunden

---

Vollständige Task-Beschreibung: `C:\TrinityBots\TrinityCore\.claude\prompts\ZENFLOW_CORE_SYSTEM_ANALYSIS.md`
