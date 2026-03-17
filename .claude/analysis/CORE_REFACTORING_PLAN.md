# Core System Refactoring - Implementation Plan

**Ziel**: SCHNELL & SCHLANK - kein Over-Engineering mehr!

---

## PHASE 1: DEAD CODE ENTFERNEN (2h)

### Task 1.1: DI Interfaces löschen

```bash
# PowerShell - alle 90 Interface-Dateien löschen
Remove-Item "C:\TrinityBots\TrinityCore\src\modules\Playerbot\Core\DI\Interfaces\*.h" -Force
```

### Task 1.2: ServiceContainer löschen

```bash
Remove-Item "C:\TrinityBots\TrinityCore\src\modules\Playerbot\Core\DI\ServiceContainer.h" -Force
Remove-Item "C:\TrinityBots\TrinityCore\src\modules\Playerbot\Core\DI\ServiceRegistration.h" -Force
Remove-Item "C:\TrinityBots\TrinityCore\src\modules\Playerbot\Core\DI\MIGRATION_GUIDE.md" -Force
```

### Task 1.3: CMakeLists.txt bereinigen

Entferne alle Referenzen zu gelöschten Dateien aus:
- `src/modules/Playerbot/CMakeLists.txt`

### Task 1.4: Include-Statements bereinigen

```bash
# Finde alle Includes der gelöschten Dateien
Get-ChildItem -Path "src\modules\Playerbot" -Recurse -Include *.cpp,*.h | 
    Select-String -Pattern '#include.*DI/Interfaces|#include.*ServiceContainer|#include.*ServiceRegistration'
```

Entferne alle gefundenen Includes.

### Task 1.5: Kompilieren & Testen

```bash
cmake --build build --target Playerbot
```

**Erwartetes Ergebnis**: Keine Fehler, Code kompiliert ohne die ungenutzten Dateien.

---

## PHASE 2: EVENT SYSTEM EVALUIERUNG (4h)

### Task 2.1: Nutzungsanalyse

```bash
# GenericEventBus Nutzung
Get-ChildItem -Recurse -Include *.cpp | Select-String "EventBus<" | Group-Object Path | Sort-Object Count -Desc

# EventDispatcher Nutzung  
Get-ChildItem -Recurse -Include *.cpp | Select-String "EventDispatcher::" | Group-Object Path | Sort-Object Count -Desc
```

### Task 2.2: Feature-Nutzung in GenericEventBus

Prüfe welche Features der 901 LOC tatsächlich genutzt werden:

| Feature | Genutzt? | Zeilen | Kandidat für Entfernung |
|---------|----------|--------|------------------------|
| Priority Queue | ? | ~100 | ? |
| Batch Processing | ? | ~150 | ? |
| Async Publish | ? | ~100 | ? |
| Statistics | ? | ~100 | ? |
| Expiry Handling | ? | ~50 | ? |

### Task 2.3: Entscheidung dokumentieren

Erstelle: `CORE_EVENT_SYSTEM_DECISION.md`
- Welches System für welchen Use Case
- Was kann entfernt werden
- Migration Plan falls nötig

---

## PHASE 3: EVENT SYSTEM REFACTORING (8-16h)

### Option A: GenericEventBus vereinfachen

```cpp
// VORHER: 901 LOC mit allem
template<typename TEvent>
class EventBus { ... };

// NACHHER: ~300 LOC Kern-Funktionalität
template<typename TEvent>
class EventBus
{
public:
    static EventBus& instance();
    void Subscribe(std::function<void(TEvent const&)> handler);
    void Unsubscribe(/* handle */);
    void Publish(TEvent const& event);
    void ProcessPending();  // Nur wenn async nötig
};
```

### Option B: EventDispatcher als Standard

Alle Domain EventBus auf EventDispatcher migrieren:
```cpp
// VORHER
GroupEventBus::instance()->PublishEvent(event);

// NACHHER
sEventDispatcher->Dispatch(BotEvent::FromGroup(event));
```

---

## PHASE 4: MANAGER KONSOLIDIERUNG (4h)

### Task 4.1: Analyse

```bash
# GameSystemsManager Nutzung
Get-ChildItem -Recurse -Include *.cpp | Select-String "GameSystemsManager" 

# ManagerRegistry Nutzung
Get-ChildItem -Recurse -Include *.cpp | Select-String "ManagerRegistry"
```

### Task 4.2: Entscheidung

| Szenario | Aktion |
|----------|--------|
| Nur einer wird genutzt | Den anderen löschen |
| Beide genutzt, überlappend | Konsolidieren |
| Beide genutzt, unterschiedlich | Klar dokumentieren |

---

## CHECKLISTE VOR MERGE

- [ ] Code kompiliert ohne Warnings
- [ ] Alle Tests bestehen
- [ ] Keine `#include` auf gelöschte Dateien
- [ ] CMakeLists.txt aktualisiert
- [ ] Dokumentation aktualisiert
- [ ] Performance nicht verschlechtert

---

## RISIKO-MITIGATION

### Low Risk (Phase 1)
- DI wird nicht genutzt → Löschen ist sicher
- Git erlaubt Rollback falls nötig

### Medium Risk (Phase 2-3)
- Events sind überall → Gründlich testen
- Incremental vorgehen: Ein EventBus nach dem anderen

### Rollback Plan
```bash
git stash
# oder
git checkout HEAD~1 -- src/modules/Playerbot/Core/
```

---

## TIMELINE

| Phase | Aufwand | Woche |
|-------|---------|-------|
| Phase 1: Dead Code | 2h | KW06 |
| Phase 2: Event Analyse | 4h | KW06 |
| Phase 3: Event Refactor | 8-16h | KW07 |
| Phase 4: Manager | 4h | KW07 |
| **TOTAL** | **18-26h** | **2 Wochen** |

---

## ERFOLGS-METRIKEN

| Metrik | Vorher | Nachher | Ziel |
|--------|--------|---------|------|
| Dateien in Core/DI | 93 | 0 | ✓ |
| LOC in Core/ | ~15,000 | <10,000 | -33% |
| Compile Time | ? | ? | -10% |
| Event Systeme | 2 | 1 | ✓ |
| Manager Systeme | 3 | 1-2 | ✓ |
