# Claude Code Prompt: Session/Lifecycle Critical Fixes

**Projekt**: TrinityCore Playerbot Module  
**Modul**: Session/ und Lifecycle/ Subsysteme  
**Priorität**: P0 + P1 Fixes  
**Geschätzter Aufwand**: 20-30 Stunden  

---

## 📋 KONTEXT

Die Zenflow-Analyse hat kritische Issues im Session/Lifecycle System identifiziert:
- 1× P0 Memory Leak (Destruktor)
- 4× P1 Race Conditions  
- 2× P1 Memory Management (raw pointers)
- 1× P1 Code Consolidation

**Analyse-Reports lesen**:
```
C:\Users\daimon\.zenflow\worktrees\session-lifecycle-88c8\.zenflow\tasks\session-lifecycle-88c8\
├── ARCHITECTURE_ANALYSIS.md
├── THREAD_SAFETY_REPORT.md
├── MEMORY_REPORT.md
├── RECOMMENDATIONS.md
```

**Summary lesen**:
```
C:\TrinityBots\TrinityCore\.claude\analysis\SESSION_LIFECYCLE_ZENFLOW_SUMMARY.md
```

---

## 🎯 TASKS

### TASK 1: P0 - Fix Packet Queue Memory Leak (1h)

**Problem**: `BotSession::~BotSession()` leaked Packet Queues wenn Mutex blockiert ist.

**Location**: `src/modules/Playerbot/Session/BotSession.cpp:496-514`

**Aktueller Code**:
```cpp
~BotSession() {
    try {
        std::unique_lock<std::mutex> lock(_packetMutex, std::defer_lock);
        if (lock.try_lock()) {
            std::queue<std::unique_ptr<WorldPacket>> empty1, empty2;
            _incomingPackets.swap(empty1);
            _outgoingPackets.swap(empty2);
        } else {
            // ⚠️ LEAK: Packets nicht aufgeräumt!
            TC_LOG_WARN(..., "Could not acquire mutex for packet cleanup");
        }
    }
}
```

**Fix**: Spin-wait mit Timeout implementieren:
```cpp
~BotSession() {
    try {
        std::unique_lock<std::mutex> lock(_packetMutex, std::defer_lock);
        
        // Spin-wait mit 2s Timeout
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (!lock.try_lock() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        if (!lock.owns_lock()) {
            TC_LOG_ERROR("module.playerbot.session", 
                "BotSession[{}]: FORCED packet cleanup - mutex timeout after 2s", 
                GetAccountId());
        }
        
        // IMMER aufräumen (mit oder ohne Lock - Destruktor-Kontext ist sicher)
        std::queue<std::unique_ptr<WorldPacket>> empty1, empty2;
        _incomingPackets.swap(empty1);
        _outgoingPackets.swap(empty2);
    }
    catch (...) {
        // Destruktor darf nicht werfen
    }
}
```

**Verification**:
- Kompilieren ohne Errors
- AddressSanitizer Build testen
- Stress-Test: 1000 schnelle Bot Logouts

---

### TASK 2: P1 - Fix DespawnAllBots Iterator Race (2-3h)

**Problem**: Range-based for über TBB concurrent_hash_map ist NICHT atomic.

**Location**: `src/modules/Playerbot/Lifecycle/BotSpawner.cpp:1256-1273`

**Aktueller Code**:
```cpp
void BotSpawner::DespawnAllBots()
{
    ::std::vector<ObjectGuid> botsToRemove;
    {
        for (auto const& [guid, zoneId] : _activeBots)  // ❌ UNSAFE!
        {
            botsToRemove.push_back(guid);
        }
    }
    
    for (ObjectGuid guid : botsToRemove)
        DespawnBot(guid, true);
}
```

**Fix**: Atomic Swap Pattern:
```cpp
void BotSpawner::DespawnAllBots()
{
    TC_LOG_INFO("module.playerbot.spawner", "DespawnAllBots: Starting atomic despawn of all bots");
    
    // Atomic swap - neue Spawns gehen in leere Map
    decltype(_activeBots) oldBots;
    _activeBots.swap(oldBots);  // TBB concurrent_hash_map::swap ist atomic
    
    uint32 despawnCount = 0;
    
    // Despawn alle Bots aus alter Map (kein Race - Map ist jetzt lokal)
    for (auto const& [guid, zoneId] : oldBots)
    {
        DespawnBot(guid, true);
        ++despawnCount;
    }
    
    TC_LOG_INFO("module.playerbot.spawner", "DespawnAllBots: Despawned {} bots", despawnCount);
}
```

**Verification**:
- Spawn 100 Bots
- DespawnAllBots() während async Spawn Thread aktiv
- Verify: `_activeBots.size() == 0` nach Despawn

---

### TASK 3: P1 - Fix TOCTOU in ValidateSpawnRequest (3-5h)

**Problem**: Time-of-check-time-of-use Race bei Population Cap Validation.

**Location**: `src/modules/Playerbot/Lifecycle/BotSpawner.cpp:536-540, 698-758`

**Aktueller Code**:
```cpp
bool BotSpawner::SpawnBot(SpawnRequest const& request)
{
    if (!ValidateSpawnRequest(request))  // CHECK (T1)
        return false;
    
    return SpawnBotInternal(request);    // USE (T2) - TIME GAP!
}
```

**Fix**: Atomic Pre-Increment mit Rollback:
```cpp
bool BotSpawner::SpawnBot(SpawnRequest const& request)
{
    // Basic validation (non-population checks)
    if (!ValidateSpawnRequestBasic(request))
        return false;
    
    // Atomic pre-increment - reserviert Slot
    uint32 newCount = _activeBotCount.fetch_add(1, std::memory_order_acquire);
    
    // Check global cap NACH Increment
    if (_config.respectPopulationCaps && newCount >= _config.maxBotsTotal)
    {
        _activeBotCount.fetch_sub(1, std::memory_order_release);  // Rollback
        TC_LOG_DEBUG("module.playerbot.spawner", 
            "SpawnBot: Rejected - global cap reached ({}/{})", 
            newCount, _config.maxBotsTotal);
        return false;
    }
    
    // Zone cap check (wenn aktiviert)
    if (request.zoneId != 0 && _config.respectPopulationCaps)
    {
        // TODO: Per-zone atomic counter für perfekte Genauigkeit
        // Für jetzt: Akzeptiere kleine Überschreitung
    }
    
    // Spawn durchführen
    if (!SpawnBotInternal(request))
    {
        _activeBotCount.fetch_sub(1, std::memory_order_release);  // Rollback bei Fehler
        return false;
    }
    
    return true;
}

// Neue Methode: Non-population validation
bool BotSpawner::ValidateSpawnRequestBasic(SpawnRequest const& request) const
{
    if (!_enabled.load())
        return false;
    
    if (!request.IsValid())
        return false;
    
    // Map validation etc. (alles außer Population Caps)
    return true;
}
```

**Header Update** (`BotSpawner.h`):
```cpp
private:
    bool ValidateSpawnRequestBasic(SpawnRequest const& request) const;
```

**Verification**:
- Set `maxBotsTotal = 100`
- 4 Threads spawnen je 50 Bots concurrent
- Verify: Exakt 100 Bots (kein Overflow)

---

### TASK 4: P1 - Fix ProcessingQueue Flag Race (1h)

**Problem**: Check-then-set Pattern nicht atomic.

**Location**: `src/modules/Playerbot/Lifecycle/BotSpawner.cpp:283-388`

**Aktueller Code**:
```cpp
void BotSpawner::Update(uint32 diff)
{
    if (!_processingQueue.load() && queueHasItems)  // CHECK
    {
        _processingQueue.store(true);                // SET - RACE!
        // Process queue...
        _processingQueue.store(false);
    }
}
```

**Fix**: Atomic Compare-Exchange:
```cpp
void BotSpawner::Update(uint32 diff)
{
    // ... existing code ...
    
    bool queueHasItems = !_spawnQueue.empty();
    
    // Atomic compare-exchange - nur EIN Thread betritt Critical Section
    bool expected = false;
    if (queueHasItems && _processingQueue.compare_exchange_strong(
            expected, true, std::memory_order_acquire, std::memory_order_relaxed))
    {
        // Nur EIN Thread kommt hier rein
        try
        {
            ProcessSpawnQueue();
        }
        catch (...)
        {
            TC_LOG_ERROR("module.playerbot.spawner", "Exception in ProcessSpawnQueue");
        }
        
        _processingQueue.store(false, std::memory_order_release);
    }
    
    // ... rest of Update() ...
}
```

---

### TASK 5: P1 - Convert BotAI* to std::unique_ptr (4-6h)

**Problem**: Raw pointer erfordert manuelles delete in Destruktor und Exception Handlers.

**Location**: `src/modules/Playerbot/Session/BotSession.h:447`

**Schritt 1 - Header ändern**:
```cpp
// BEFORE (BotSession.h:447)
BotAI* _ai{nullptr};

// AFTER
std::unique_ptr<BotAI> _ai;
```

**Schritt 2 - Interface anpassen**:
```cpp
// BEFORE
void SetAI(BotAI* ai) { _ai = ai; }
BotAI* GetAI() const { return _ai; }

// AFTER
void SetAI(std::unique_ptr<BotAI> ai) { _ai = std::move(ai); }
BotAI* GetAI() const { return _ai.get(); }
```

**Schritt 3 - Destruktor vereinfachen** (BotSession.cpp):
```cpp
// BEFORE
~BotSession() {
    if (_ai) {
        delete _ai;
        _ai = nullptr;
    }
}

// AFTER
~BotSession() {
    // _ai automatisch deleted via unique_ptr
}
```

**Schritt 4 - Exception Handlers aufräumen**:
Suche alle `delete _ai` und entferne sie (unique_ptr räumt automatisch auf).

**Schritt 5 - Call Sites updaten**:
```cpp
// BEFORE
_ai = sBotAIFactory->CreateAI(ctx.player);

// AFTER
_ai = std::unique_ptr<BotAI>(sBotAIFactory->CreateAI(ctx.player));
// ODER wenn Factory unique_ptr returned:
_ai = sBotAIFactory->CreateAI(ctx.player);
```

**Verification**:
- Kompilieren ohne Errors/Warnings
- AddressSanitizer: Kein AI Memory Leak
- Exception Test: Simulate DB Failure bei Login

---

### TASK 6: P1 - Convert Player* to std::unique_ptr During Login (2-3h)

**Problem**: Player Objekt bei Login verwendet raw pointer.

**Location**: `src/modules/Playerbot/Session/BotSession.cpp:1900-1960`

**Fix**:
```cpp
// BEFORE
Player* pCurrChar = new Player(this);
if (!pCurrChar->LoadFromDB(characterGuid, holder)) {
    delete pCurrChar;  // Manual cleanup
    TC_LOG_ERROR(...);
    return;
}
SetPlayer(pCurrChar);

// AFTER
std::unique_ptr<Player> pCurrChar = std::make_unique<Player>(this);
if (!pCurrChar->LoadFromDB(characterGuid, holder)) {
    // Automatic cleanup via unique_ptr
    TC_LOG_ERROR(...);
    return;
}
SetPlayer(pCurrChar.release());  // Transfer ownership to WorldSession
```

**Alle manuellen deletes entfernen** (suche nach `delete pCurrChar` und `delete player`).

---

### TASK 7: P1 - Merge Corpse Crash Mitigation Components (4-6h)

**Problem**: CorpsePreventionManager + SafeCorpseManager haben überlappende Funktionalität.

**Locations**:
- `src/modules/Playerbot/Lifecycle/CorpsePreventionManager.h/.cpp` (156 LOC)
- `src/modules/Playerbot/Lifecycle/SafeCorpseManager.h/.cpp` (168 LOC)

**Neue Datei erstellen**: `CorpseCrashMitigation.h/.cpp`

```cpp
// CorpseCrashMitigation.h
#pragma once

#include "Common.h"
#include <unordered_map>
#include <shared_mutex>
#include <atomic>

namespace Playerbot
{

struct CorpseLocation
{
    uint32 mapId;
    float x, y, z;
    std::chrono::steady_clock::time_point deathTime;
};

/**
 * @class CorpseCrashMitigation
 * @brief Unified corpse crash prevention with dual-strategy pattern
 * 
 * Strategy 1: Prevention - Try to prevent corpse creation entirely
 * Strategy 2: Safe Tracking - If prevention fails, track corpse safely
 */
class TC_GAME_API CorpseCrashMitigation
{
public:
    static CorpseCrashMitigation& Instance();
    
    // Unified entry points
    void OnBotDeath(Player* bot);
    void OnCorpseCreated(Player* bot, Corpse* corpse);
    void OnBotResurrection(Player* bot);
    
    // Query methods
    bool IsCorpseSafeToDelete(ObjectGuid corpseGuid) const;
    CorpseLocation const* GetDeathLocation(ObjectGuid botGuid) const;
    
    // Configuration
    void SetPreventionEnabled(bool enabled) { _preventionEnabled = enabled; }
    bool IsPreventionEnabled() const { return _preventionEnabled; }
    
    // Statistics
    uint32 GetPreventedCorpses() const { return _preventedCorpses.load(); }
    uint32 GetTrackedCorpses() const { return _trackedCorpses.load(); }
    
private:
    CorpseCrashMitigation() = default;
    
    // Strategy 1: Prevention (try first)
    bool TryPreventCorpse(Player* bot);
    
    // Strategy 2: Safe Tracking (fallback)
    void TrackCorpseSafely(Player* bot, Corpse* corpse);
    void UntrackCorpse(ObjectGuid corpseGuid);
    
    // Unified corpse location cache
    mutable std::shared_mutex _mutex;
    std::unordered_map<ObjectGuid, CorpseLocation> _deathLocations;
    std::unordered_map<ObjectGuid, std::atomic<uint32>> _corpseRefCounts;
    
    // Configuration
    bool _preventionEnabled{true};
    
    // Statistics
    std::atomic<uint32> _preventedCorpses{0};
    std::atomic<uint32> _trackedCorpses{0};
};

#define sCorpseCrashMitigation CorpseCrashMitigation::Instance()

} // namespace Playerbot
```

**Implementation** (`CorpseCrashMitigation.cpp`):
- Move `TryPreventCorpse()` logic from CorpsePreventionManager
- Move `TrackCorpseSafely()` logic from SafeCorpseManager
- Consolidate `_deathLocations` (single source of truth)

**Update DeathHookIntegration.cpp**:
```cpp
// BEFORE
void DeathHookIntegration::OnPlayerPreDeath(Player* player) {
    sCorpsePreventionManager->PreventCorpseAndResurrect(player);
}

// AFTER
void DeathHookIntegration::OnPlayerPreDeath(Player* player) {
    sCorpseCrashMitigation.OnBotDeath(player);
}
```

**Delete old files**:
- CorpsePreventionManager.h/.cpp
- SafeCorpseManager.h/.cpp

**Update CMakeLists.txt** entsprechend.

---

## ✅ IMPLEMENTATION CHECKLIST

### Task 1: P0 Packet Queue Leak
- [ ] Spin-wait mit Timeout implementieren
- [ ] Error Logging bei Timeout
- [ ] Immer cleanup (auch ohne Lock)
- [ ] Kompilieren testen
- [ ] AddressSanitizer Test

### Task 2: DespawnAllBots Race
- [ ] Atomic swap implementieren
- [ ] Logging hinzufügen
- [ ] Concurrent Spawn Test

### Task 3: TOCTOU Fix
- [ ] ValidateSpawnRequestBasic() extrahieren
- [ ] Atomic pre-increment mit Rollback
- [ ] Header updaten
- [ ] Concurrent Spawn Test mit Cap

### Task 4: ProcessingQueue Race
- [ ] compare_exchange_strong verwenden
- [ ] Exception Handling hinzufügen
- [ ] Memory ordering korrekt

### Task 5: BotAI unique_ptr
- [ ] Header ändern
- [ ] Interface anpassen
- [ ] Destruktor vereinfachen
- [ ] Exception Handlers aufräumen
- [ ] Call Sites updaten
- [ ] Kompilieren testen

### Task 6: Player unique_ptr
- [ ] Login Code ändern
- [ ] Alle manuellen deletes entfernen
- [ ] Kompilieren testen

### Task 7: Corpse Mitigation Merge
- [ ] CorpseCrashMitigation.h erstellen
- [ ] CorpseCrashMitigation.cpp implementieren
- [ ] DeathHookIntegration updaten
- [ ] Alte Dateien löschen
- [ ] CMakeLists.txt updaten
- [ ] Death/Resurrection Test

---

## 📁 FILES TO READ FIRST

```cpp
// Zenflow Analysis Reports
C:\Users\daimon\.zenflow\worktrees\session-lifecycle-88c8\.zenflow\tasks\session-lifecycle-88c8\THREAD_SAFETY_REPORT.md
C:\Users\daimon\.zenflow\worktrees\session-lifecycle-88c8\.zenflow\tasks\session-lifecycle-88c8\MEMORY_REPORT.md
C:\Users\daimon\.zenflow\worktrees\session-lifecycle-88c8\.zenflow\tasks\session-lifecycle-88c8\RECOMMENDATIONS.md

// Target Files
src/modules/Playerbot/Session/BotSession.h
src/modules/Playerbot/Session/BotSession.cpp
src/modules/Playerbot/Lifecycle/BotSpawner.h
src/modules/Playerbot/Lifecycle/BotSpawner.cpp
src/modules/Playerbot/Lifecycle/CorpsePreventionManager.h
src/modules/Playerbot/Lifecycle/CorpsePreventionManager.cpp
src/modules/Playerbot/Lifecycle/SafeCorpseManager.h
src/modules/Playerbot/Lifecycle/SafeCorpseManager.cpp
src/modules/Playerbot/Lifecycle/DeathHookIntegration.cpp
```

---

## 🎯 SUCCESS CRITERIA

1. **P0 Fixed**: Kein Memory Leak im BotSession Destruktor
2. **No Race Conditions**: DespawnAllBots, SpawnBot, ProcessingQueue alle thread-safe
3. **RAII Compliant**: BotAI und Player verwenden unique_ptr
4. **Code Consolidated**: CorpsePreventionManager + SafeCorpseManager → CorpseCrashMitigation
5. **All Tests Pass**: Kompilieren, Unit Tests, AddressSanitizer clean
6. **Performance**: Keine Regression (Lock-free Architektur erhalten)

---

## 📝 NOTES

- TBB concurrent_hash_map::swap() ist atomic - sicher für DespawnAllBots
- compare_exchange_strong ist besser als compare_exchange_weak für diese Use Cases
- unique_ptr::release() transferiert ownership ohne delete
- Destruktor darf keine Exceptions werfen - immer try/catch
- Memory ordering: acquire/release für counter, seq_cst für control flags
