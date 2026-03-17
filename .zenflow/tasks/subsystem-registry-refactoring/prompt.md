# Task: PlayerbotModule Subsystem Registry Refactoring + Startup Log Harmonisierung

## Ziel

Refactoring der drei "God Functions" in `PlayerbotModule.cpp`:
- `InitializeManagers()` (~300 Zeilen Boilerplate)
- `OnWorldUpdate()` (~200 Zeilen mit manuellen Timer-Variablen t1–t17)
- `Shutdown()` (~100 Zeilen Copy-Paste Shutdown-Calls)

Ersetze das manuelle, hart-codierte Subsystem-Management durch ein **SubsystemRegistry-Pattern**, das Init, Update und Shutdown automatisch verwaltet. Zusätzlich: **Harmonisierung und Verschlankung des Startup-Log-Outputs** für bessere Übersichtlichkeit.

---

## Kontext

### Projekt
- **Pfad**: `C:\TrinityBots\TrinityCore`
- **Modul**: `src/modules/Playerbot/`
- **Sprache**: C++20, MSVC (Visual Studio 2025)
- **Build**: `cmake --build build --config RelWithDebInfo -- /m`
- **Branch**: `playerbot-dev`
- **Codebasis**: ~636K LOC, ~1010 Dateien im Playerbot-Modul

### Wichtige Dateien
- `src/modules/Playerbot/PlayerbotModule.h` — Hauptklasse (Header)
- `src/modules/Playerbot/PlayerbotModule.cpp` — Hauptklasse (Implementation, ~600 Zeilen)
- `src/modules/Playerbot/PlayerbotModuleAdapter.h/.cpp` — ModuleManager-Integration
- `src/modules/Common/Update/ModuleUpdateManager.h/.cpp` — Bestehendes Update-Registry (NICHT ändern)
- `src/modules/Playerbot/CMakeLists.txt` — Build-Konfiguration (Multi-Library Architektur)

### Bestehende Architektur
`PlayerbotModuleAdapter` registriert sich beim `ModuleManager`. Bei jedem World-Tick wird `PlayerbotModuleAdapter::OnModuleUpdate()` → `PlayerbotModule::OnWorldUpdate()` aufgerufen. PlayerbotModule ruft dann manuell 17+ Manager-Update-Funktionen auf.

### Coding Standards (aus CLAUDE.md)
- **Naming**: PascalCase Klassen, PascalCase Methoden, camelCase Variablen, `m_` Prefix für Members
- **Threading**: `std::shared_mutex` für read-heavy, `std::mutex` für write-heavy
- **Memory**: Smart Pointers bevorzugen
- **Include Order**: Own header → Project headers → TrinityCore headers → External → STL
- **Singletons**: Nutze `#define sXxxManager XxxManager::instance()` Pattern wie im restlichen TC-Code

---

## Problem-Analyse

### 1. OnWorldUpdate() — Manuelles Timing-Boilerplate

Aktuell sieht die Funktion so aus (gekürzt):

```cpp
void PlayerbotModule::OnWorldUpdate(uint32 diff)
{
    auto timeStart = std::chrono::high_resolution_clock::now();
    auto lastTime = timeStart;

    sBotAccountMgr->Update(diff);
    auto t1 = std::chrono::high_resolution_clock::now();
    auto accountTime = std::chrono::duration_cast<std::chrono::microseconds>(t1 - lastTime).count();
    lastTime = t1;

    Playerbot::sBotSpawner->Update(diff);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto spawnerTime = std::chrono::duration_cast<std::chrono::microseconds>(t2 - lastTime).count();
    lastTime = t2;

    // ... 15 weitere identische Blöcke mit t3–t17 ...

    if (totalUpdateTime > 100000)
    {
        TC_LOG_WARN("module.playerbot.performance",
            "PERFORMANCE: OnWorldUpdate took {:.2f}ms - Account:{:.2f}ms, ...",
            totalUpdateTime / 1000.0f, accountTime / 1000.0f, ...); // 17 Parameter!
    }
}
```

**Probleme**: Massiver Code-Duplikat, jedes neue Subsystem erfordert Änderungen an 3 Stellen (Init, Update, Shutdown + neue Timer-Variable + Log-Format-String).

### 2. InitializeManagers() — Identisches Boilerplate × 25

```cpp
TC_LOG_INFO("server.loading", "Initializing Bot Protection Registry...");
if (!sBotProtectionRegistry->Initialize())
{
    TC_LOG_WARN("server.loading", "Bot Protection Registry initialization failed...");
}
else
{
    TC_LOG_INFO("server.loading", "Bot Protection Registry initialized successfully");
}
```

Dieses Pattern wiederholt sich ~25 Mal mit minimalem Unterschied.

### 3. Shutdown() — Spiegelbildliches Boilerplate

```cpp
TC_LOG_INFO("server.loading", "Shutting down Bot Protection Registry...");
sBotProtectionRegistry->Shutdown();
TC_LOG_INFO("server.loading", "Bot Protection Registry shutdown complete");
```

~20 Mal wiederholt.

### 4. Startup-Logging — Unübersichtlich und inkonsistent

- Manche Subsysteme loggen auf `"server.loading"`, andere auf `"module.playerbot"`
- Einige nutzen `TC_LOG_ERROR` für nicht-kritische Meldungen (z.B. `PlayerbotModuleAdapter::OnModuleStartup` loggt mit `TC_LOG_ERROR`)
- Jedes Subsystem produziert 2-3 Zeilen (init start + init success/fail)
- Bei 25+ Subsystemen sind das ~75 Log-Zeilen nur für Init
- Kein visueller Überblick — alles sieht gleich aus

---

## Anforderungen

### A. Neues SubsystemRegistry-Interface

Erstelle ein `IPlayerbotSubsystem`-Interface und eine `PlayerbotSubsystemRegistry`:

```cpp
// Core/PlayerbotSubsystem.h
namespace Playerbot
{

enum class SubsystemPriority : uint8
{
    CRITICAL = 0,    // Muss initialisiert werden, Fehler = Abort
    HIGH     = 1,    // Sollte initialisiert werden, Fehler = Warning
    NORMAL   = 2,    // Standard-Subsystem
    LOW      = 3,    // Optional, Fehler wird ignoriert
};

struct SubsystemInfo
{
    std::string name;                    // Display-Name für Logging
    SubsystemPriority priority;          // Init-Priorität
    uint32 initOrder;                    // Init-Reihenfolge (niedrig = zuerst)
    uint32 updateOrder;                  // Update-Reihenfolge (niedrig = zuerst)
    uint32 shutdownOrder;               // Shutdown-Reihenfolge (niedrig = zuerst)
    bool needsUpdate;                    // Ob dieses Subsystem per-tick updates braucht
};

class IPlayerbotSubsystem
{
public:
    virtual ~IPlayerbotSubsystem() = default;

    virtual SubsystemInfo GetInfo() const = 0;
    virtual bool Initialize() = 0;
    virtual void Update(uint32 diff) = 0;
    virtual void Shutdown() = 0;
};

} // namespace Playerbot
```

Die Registry:
```cpp
// Core/PlayerbotSubsystemRegistry.h
namespace Playerbot
{

class PlayerbotSubsystemRegistry
{
public:
    static PlayerbotSubsystemRegistry* instance();

    // Registrierung
    void RegisterSubsystem(std::unique_ptr<IPlayerbotSubsystem> subsystem);

    // Lifecycle
    bool InitializeAll();          // Ersetzt InitializeManagers()
    void UpdateAll(uint32 diff);   // Ersetzt OnWorldUpdate()-Inhalt
    void ShutdownAll();            // Ersetzt Shutdown()-Inhalt

    // Diagnostik
    struct SubsystemMetrics
    {
        std::string name;
        uint64 totalUpdateTimeUs = 0;
        uint64 lastUpdateTimeUs = 0;
        uint64 maxUpdateTimeUs = 0;
        uint32 updateCount = 0;
    };
    std::vector<SubsystemMetrics> GetMetrics() const;
    uint32 GetSubsystemCount() const;

private:
    PlayerbotSubsystemRegistry() = default;

    struct SubsystemEntry
    {
        std::unique_ptr<IPlayerbotSubsystem> subsystem;
        SubsystemInfo info;
        SubsystemMetrics metrics;
        bool initialized = false;
    };

    std::vector<SubsystemEntry> m_subsystems;
    bool m_allInitialized = false;
};

} // namespace Playerbot

#define sPlayerbotSubsystemRegistry Playerbot::PlayerbotSubsystemRegistry::instance()
```

### B. Subsystem-Adapter für bestehende Manager

Da die bestehenden Singletons (sBotAccountMgr, sBotSpawner, etc.) NICHT geändert werden sollen, erstelle Thin-Wrapper-Adapter. Beispiel:

```cpp
class BotAccountMgrSubsystem : public IPlayerbotSubsystem
{
public:
    SubsystemInfo GetInfo() const override
    {
        return { "BotAccountMgr", SubsystemPriority::CRITICAL,
                 /*initOrder=*/100, /*updateOrder=*/100, /*shutdownOrder=*/900,
                 /*needsUpdate=*/true };
    }
    bool Initialize() override { return sBotAccountMgr->Initialize(); }
    void Update(uint32 diff) override { sBotAccountMgr->Update(diff); }
    void Shutdown() override { sBotAccountMgr->Shutdown(); }
};
```

Erstelle Adapter für ALLE ~25 Subsysteme die aktuell in InitializeManagers/OnWorldUpdate/Shutdown manuell aufgerufen werden. Die Adapter können in einer einzigen Datei leben: `Core/SubsystemAdapters.cpp`.

**WICHTIG**: Analysiere die aktuelle Reihenfolge in PlayerbotModule.cpp sorgfältig und bilde sie exakt in den Order-Werten ab!

### C. Vereinfachte PlayerbotModule-Funktionen

Nach dem Refactoring sollte PlayerbotModule.cpp drastisch schrumpfen:

```cpp
bool PlayerbotModule::InitializeManagers()
{
    RegisterAllSubsystems();
    return sPlayerbotSubsystemRegistry->InitializeAll();
}

void PlayerbotModule::OnWorldUpdate(uint32 diff)
{
    if (!_enabled || !_initialized)
        return;

    try
    {
        // One-time trigger to complete login for existing sessions
        static bool loginTriggered = false;
        static uint32 totalTime = 0;
        totalTime += diff;
        if (!loginTriggered && totalTime > 5000)
        {
            TriggerBotCharacterLogins();
            loginTriggered = true;
        }

        sPlayerbotSubsystemRegistry->UpdateAll(diff);
    }
    catch (std::exception const& ex)
    {
        TC_LOG_ERROR("module.playerbot", "CRITICAL EXCEPTION in OnWorldUpdate: {}", ex.what());
        _enabled = false;
    }
    catch (...)
    {
        TC_LOG_ERROR("module.playerbot", "CRITICAL UNKNOWN EXCEPTION in OnWorldUpdate");
        _enabled = false;
    }
}

void PlayerbotModule::Shutdown()
{
    if (!_initialized)
        return;

    TC_LOG_INFO("module.playerbot", "Shutting down Playerbot Module...");
    sPlayerbotSubsystemRegistry->ShutdownAll();
    ShutdownDatabase();
    sPlayerbotCharDB->Shutdown();
    _initialized = false;
    _enabled = false;
    TC_LOG_INFO("module.playerbot", "Playerbot Module shutdown complete.");
}
```

### D. Automatisches Performance-Profiling in der Registry

Die Registry misst die Update-Zeit pro Subsystem automatisch. Bei Überschreitung von 100ms werden die **Top-5 langsamsten Subsysteme** geloggt (statt alle 17):

```
PERFORMANCE: UpdateAll took 142.5ms - Top offenders: 
  BotWorldSessionMgr: 68.3ms, BotSpawner: 32.1ms, DomainEventBus: 18.7ms, ...
```

### E. Startup-Log Harmonisierung

**Ziel**: Kompakter, übersichtlicher Startup-Output statt ~75 Zeilen.

#### Format-Vorgabe:

```
[module.playerbot] ═══════════════════════════════════════════════════════════════
[module.playerbot]  Playerbot Module v1.0.0 initializing...
[module.playerbot] ─────────────────────────────────────────────────────────────── 
[module.playerbot]  Database       : Connected (127.0.0.1:3306/characters)
[module.playerbot]  Migrations     : 12 applied (schema v12)
[module.playerbot] ───────────────────────────────────────────────────────────────
[module.playerbot]  Initializing 25 subsystems...
[module.playerbot]    ✓ BotAccountMgr              [  45ms]
[module.playerbot]    ✓ BotNameMgr                 [  12ms]
[module.playerbot]    ✓ BotCharacterDistribution   [  89ms]
[module.playerbot]    ✓ BotWorldSessionMgr         [  23ms]
[module.playerbot]    ⚠ BotProtectionRegistry      [   3ms] (non-critical, continuing)
[module.playerbot]    ✓ QuestHubDatabase           [ 234ms] (847 quest hubs)
[module.playerbot]    ... (weitere Subsysteme)
[module.playerbot] ───────────────────────────────────────────────────────────────
[module.playerbot]  Result: 24/25 OK | 1 warning | 0 failed
[module.playerbot]  Total init time: 1,247ms
[module.playerbot] ═══════════════════════════════════════════════════════════════
[module.playerbot]  Playerbot Module v1.0.0 ready.
```

#### Regeln:
1. **Einheitlicher Log-Channel**: Alles über `"module.playerbot"` (nicht `"server.loading"`)
2. **Log-Level**: `TC_LOG_INFO` für normale Meldungen, `TC_LOG_WARN` für non-fatal, `TC_LOG_ERROR` nur für fatale Fehler
3. **Kein doppeltes Logging**: Subsysteme selbst loggen NICHT mehr "Initializing..." / "...initialized". Die Registry macht das zentral.
4. **Alle manuellen TC_LOG_INFO/WARN-Aufrufe** vor/nach Init-Calls in PlayerbotModule.cpp ENTFERNEN

#### PlayerbotModuleAdapter aufräumen:
- `TC_LOG_ERROR("server.loading", "=== PlayerbotModuleAdapter::OnModuleStartup() CALLED ===")` → ENTFERNEN
- `if (++logCounter % 100000 == 0)` periodic logging → ENTFERNEN
- Alle verbleibenden Logs auf `"module.playerbot"` Channel umstellen

---

## Schritte

### Phase 1: Interface & Registry erstellen
1. Erstelle `src/modules/Playerbot/Core/PlayerbotSubsystem.h` (Interface)
2. Erstelle `src/modules/Playerbot/Core/PlayerbotSubsystemRegistry.h/.cpp` (Registry)
3. Füge die neuen Dateien zum CMakeLists.txt hinzu (in `PLAYERBOT_CORE_SOURCES`)

### Phase 2: Subsystem-Adapter erstellen
4. Erstelle `src/modules/Playerbot/Core/SubsystemAdapters.h/.cpp` — Thin-Wrapper für alle Manager
5. Erstelle `RegisterAllSubsystems()` Funktion die alle Adapter registriert

**KRITISCH**: Die Reihenfolge der Registrierung (initOrder, updateOrder, shutdownOrder) muss die aktuelle Reihenfolge in PlayerbotModule.cpp exakt beibehalten! Analysiere die aktuelle Reihenfolge sorgfältig.

### Phase 3: PlayerbotModule.cpp refactorn
6. `InitializeManagers()` → `RegisterAllSubsystems()` + `sPlayerbotSubsystemRegistry->InitializeAll()`
7. `OnWorldUpdate()` → `sPlayerbotSubsystemRegistry->UpdateAll(diff)` (Login-Trigger und Exception-Safety beibehalten)
8. `Shutdown()` → `sPlayerbotSubsystemRegistry->ShutdownAll()`
9. Entferne ALLE manuellen Init/Shutdown-Log-Aufrufe

### Phase 4: Log-Harmonisierung
10. Implementiere formatierte Startup-Ausgabe in `InitializeAll()`
11. Implementiere Performance-Warning (Top-5) in `UpdateAll()`
12. Räume `PlayerbotModuleAdapter.cpp` auf (Debug-Leftovers, Log-Channel)
13. Stelle alle Playerbot-Logs in `PlayerbotModule.cpp` auf `"module.playerbot"` um
14. Database-Init-Meldungen (`InitializeDatabase()`) ebenfalls harmonisieren

### Phase 5: Build & Verify
15. Build: `cmake --build build --config RelWithDebInfo -- /m`
16. Alle Compile-Fehler beheben
17. Verifiziere: Keine funktionalen Änderungen — reines Refactoring

---

## Spezielle Anforderungen

### Was NICHT geändert werden darf
- **Bestehende Singleton-Interfaces** (sBotAccountMgr, sBotSpawner, etc.) — nur Wrapper erstellen
- **ModuleUpdateManager** in `src/modules/Common/` — fremder Code
- **ModuleManager-Integration** — PlayerbotModuleAdapter bleibt Einstiegspunkt
- **Die funktionale Reihenfolge** von Init, Update und Shutdown
- **EventBus-Processing** in OnWorldUpdate() — als Subsystem wrappen
- **Queue Health Monitoring** — als Teil des EventBus-Subsystems wrappen

### Was besonders beachtet werden muss
- Die **Bot-Login-Trigger-Logik** (`static bool loginTriggered`) muss erhalten bleiben
- **Dependency-Wiring** zwischen Subsystemen muss erhalten bleiben. Z.B.:
  ```cpp
  sDemandCalculator->SetActivityTracker(sPlayerActivityTracker);
  sDemandCalculator->SetProtectionRegistry(sBotProtectionRegistry);
  sDemandCalculator->SetFlowPredictor(sBracketFlowPredictor);
  ```
  Diese Calls gehören in den `Initialize()` des DemandCalculator-Adapters.
- Die **BotRetirementManager** braucht ebenfalls Wiring: `sBotRetirementManager->SetProtectionRegistry(sBotProtectionRegistry)`
- **Exception-Safety** in OnWorldUpdate() beibehalten (try-catch mit Module-Disable)
- Das **NOTE über doppelte Updates** (ModuleUpdateManager vs ModuleManager) beachten

### Shutdown-Reihenfolge
Die Shutdown-Reihenfolge ist NICHT einfach die umgekehrte Init-Reihenfolge. Die Registry braucht ein separates `shutdownOrder`-Feld. Analysiere die aktuelle Shutdown-Reihenfolge in `PlayerbotModule::Shutdown()` und bilde sie korrekt ab.

### Thread-Safety
Die Registry wird NUR vom World Thread aufgerufen — kein Multi-Thread-Zugriff. Daher braucht die Registry selbst KEINE Mutex-Protection.

---

## Subsystem-Liste (aus aktueller PlayerbotModule.cpp extrahiert)

### InitializeManagers() Reihenfolge:
1. BotAccountMgr (CRITICAL)
2. BotNameMgr (CRITICAL)
3. BotCharacterDistribution (CRITICAL)
4. BotWorldSessionMgr (CRITICAL)
5. BotPacketRelay
6. BotChatCommandHandler
7. ProfessionDatabase
8. ProfessionEventBus (lazy init, evtl. kein Adapter nötig)
9. ClassBehaviorTreeRegistry
10. QuestHubDatabase
11. PortalDatabase (non-fatal)
12. BotGearFactory
13. PlayerbotPacketSniffer
14. BG/LFG Typed Packet Handlers
15. MajorCooldownTracker
16. BotActionManager
17. BotProtectionRegistry (non-fatal)
18. BotRetirementManager (non-fatal, braucht ProtectionRegistry)
19. BracketFlowPredictor (non-fatal)
20. PlayerActivityTracker (non-fatal)
21. DemandCalculator (non-fatal, braucht ActivityTracker/ProtectionRegistry/FlowPredictor)
22. PopulationLifecycleController (non-fatal)
23. ContentRequirementDb (non-fatal)
24. BotTemplateRepository (non-fatal)
25. BotCloneEngine (non-fatal)
26. BotPostLoginConfigurator (non-fatal)
27. InstanceBotPool (non-fatal)
28. JITBotFactory (non-fatal)
29. QueueStatePoller (non-fatal)
30. QueueShortageSubscriber (non-fatal)
31. InstanceBotOrchestrator (non-fatal)
32. InstanceBotHooks (non-fatal)
33. BotOperationTracker

### OnWorldUpdate() Reihenfolge:
1. BotAccountMgr
2. BotSpawner
3. BotWorldSessionMgr
4. PlayerbotCharDB
5. GroupEventBus
6. Domain EventBuses (Combat, Loot, Quest, Aura, Cooldown, Resource, Social, Auction, NPC, Instance, Profession)
7. Queue Health Monitoring (every 60s, kann Teil des EventBus-Subsystems sein)
8. BotProtectionRegistry
9. BotRetirementManager
10. BracketFlowPredictor
11. PlayerActivityTracker
12. DemandCalculator
13. PopulationLifecycleController
14. InstanceBotPool
15. InstanceBotOrchestrator
16. JITBotFactory
17. QueueStatePoller

### Shutdown() Reihenfolge (aktuell):
1. BotActionManager
2. PopulationLifecycleController
3. InstanceBotHooks
4. InstanceBotOrchestrator
5. QueueShortageSubscriber
6. QueueStatePoller
7. JITBotFactory
8. InstanceBotPool
9. BotCloneEngine
10. BotPostLoginConfigurator
11. BotTemplateRepository
12. BotOperationTracker (PrintStatus + Shutdown)
13. DemandCalculator
14. PlayerActivityTracker
15. BracketFlowPredictor
16. BotRetirementManager
17. BotProtectionRegistry
18. UnregisterHooks + UnregisterModule
19. BotChatCommandHandler
20. BotPacketRelay
21. PlayerbotPacketSniffer
22. BotWorldSessionMgr
23. BotNameMgr
24. BotAccountMgr
25. PlayerbotDatabase (ShutdownDatabase)
26. PlayerbotCharDB

---

## Definition of Done

- [ ] Alle 3 God-Functions (Init/Update/Shutdown) sind auf je ~10-20 Zeilen geschrumpft
- [ ] SubsystemRegistry existiert mit automatischem Performance-Profiling
- [ ] Alle Subsysteme sind als Adapter registriert
- [ ] Init/Update/Shutdown-Reihenfolge ist exakt wie vorher
- [ ] Startup-Log ist kompakt und übersichtlich (gruppiert, mit Timing, mit Zusammenfassung)
- [ ] Alle Logs nutzen einheitlich `"module.playerbot"` Channel
- [ ] Keine `TC_LOG_ERROR` für informative Meldungen
- [ ] Debug-Leftovers in PlayerbotModuleAdapter entfernt
- [ ] Performance-Warning zeigt Top-5 langsamste Subsysteme
- [ ] Build kompiliert fehlerfrei mit `RelWithDebInfo`
- [ ] Keine funktionalen Änderungen — reines Refactoring
- [ ] Dependency-Wiring zwischen Subsystemen funktioniert korrekt
- [ ] Bot-Login-Trigger-Logik funktioniert weiterhin
