# Core System Analyse - REFACTORING FOKUS

**Datum**: 2026-02-04  
**Ziel**: System SCHNELL und SCHLANK - keine Neuimplementierung!

---

## 🚨 KRITISCHE FINDINGS

### 1. DI CONTAINER IST KOMPLETT UNGENUTZT!

```
Registrierungen:  92 (in Kommentaren/Beispielen)
Echte Resolve():   0 (ZERO in .cpp Dateien!)
Interfaces:       90 Dateien
```

**BEWEIS:**
```powershell
# Suche nach echten Resolve-Aufrufen in .cpp:
Get-ChildItem *.cpp | Select-String "Services::Container().Resolve"
# ERGEBNIS: LEER!
```

**Das heißt:**
- 90 Interface-Dateien (Core/DI/Interfaces/) sind **DEAD CODE**
- ServiceContainer.h (398 LOC) ist **UNGENUTZT**
- ServiceRegistration.h ist **UNGENUTZT**

**REFACTORING-EMPFEHLUNG**: 
- [ ] Alle 90 Interface-Dateien LÖSCHEN
- [ ] ServiceContainer.h LÖSCHEN
- [ ] Direkte Singletons beibehalten (funktionieren bereits)

---

### 2. ZWEI EVENT-SYSTEME - UNNÖTIGE DUPLIZIERUNG

| System | Datei | LOC | Pattern |
|--------|-------|-----|---------|
| GenericEventBus | Core/Events/GenericEventBus.h | 901 | Template Singleton |
| EventDispatcher | Core/Events/EventDispatcher.cpp | 297 | Queue + Subscription |

**Plus 15+ spezialisierte EventBus:**
```
AI/Combat/HostileEventBus.h
Auction/AuctionEventBus.h
Aura/AuraEventBus.h
Combat/CombatEventBus.h
Cooldown/CooldownEventBus.h
Group/GroupEventBus.h
Instance/InstanceEventBus.h
Loot/LootEventBus.h
NPC/NPCEventBus.h
Professions/ProfessionEventBus.h
Quest/QuestEventBus.h
Resource/ResourceEventBus.h
Social/SocialEventBus.h
Lifecycle/BotSpawnEventBus.h
```

**PROBLEM**: 
- EventDispatcher UND GenericEventBus<T> existieren parallel
- Keine klare Abgrenzung wann welches System nutzen

**REFACTORING-EMPFEHLUNG**:
- [ ] Entscheiden: GenericEventBus ODER EventDispatcher
- [ ] GenericEventBus.h (901 LOC) vereinfachen wenn beibehalten
- [ ] Einheitliches Pattern für alle Event-Typen

---

### 3. DREI MANAGER-SYSTEME - REDUNDANZ

| System | Datei | LOC | Zweck |
|--------|-------|-----|-------|
| ServiceContainer | Core/DI/ServiceContainer.h | 398 | DI (UNGENUTZT!) |
| GameSystemsManager | Core/Managers/GameSystemsManager.cpp | 695 | System Lifecycle |
| ManagerRegistry | Core/Managers/ManagerRegistry.cpp | 416 | Manager Lookup |

**REFACTORING-EMPFEHLUNG**:
- [ ] ServiceContainer LÖSCHEN (ungenutzt)
- [ ] GameSystemsManager + ManagerRegistry konsolidieren ODER klare Trennung

---

## 📊 TRINITYCORE INTEGRATION POINTS

### Tatsächlich genutzte Hooks:

#### 1. TrinityCore Script System (scripts/)
```cpp
// PlayerbotWorldScript.cpp - WorldScript
class PlayerbotWorldScript : public WorldScript
{
    void OnUpdate(uint32 diff);        // Haupt-Update-Loop
    void OnConfigLoad(bool reload);    // Config Reload
    void OnStartup();                  // Server Start
};

// PlayerbotEventScripts.cpp - 66+ Player Hooks
// Combat, Death, Group, Leveling, Chat, Economy, Vehicles, Items, Instances
```

#### 2. Core PlayerBotHooks (Core/PlayerBotHooks.cpp)
```cpp
// 891 LOC
OnGroupMemberAdded      → GroupEventBus::PublishEvent()
OnGroupMemberRemoved    → GroupEventBus::PublishEvent()
OnGroupLeaderChanged    → GroupEventBus::PublishEvent()
OnGroupDisbanding       → GroupEventBus::PublishEvent()
// etc.
```

#### 3. Spezialisierte Hooks
```
Lifecycle/DeathHookIntegration.cpp    - Death Events
Lifecycle/Instance/InstanceBotHooks.cpp - Instance Events
Network/TypedPacketHooks.h            - Packet Hooks
Group/PlayerbotGroupScript.cpp        - Group Events
```

### Was FEHLT oder nicht integriert:

| Feature | Status | Notiz |
|---------|--------|-------|
| Packet Sniffer | ❓ | TypedPacketHooks.h existiert aber Nutzung unklar |
| AuctionHouse Hooks | ⚠️ | Kommentiert aus (Phase 6 TODO) |
| LLM Integration | ✅ | BotChatCommandHandler existiert |
| Vehicle Events | ✅ | In PlayerbotEventScripts.cpp |

---

## 🔧 REFACTORING-PLAN (PRIORISIERT)

### P0 - SOFORT (Spart ~100 Dateien, ~5000 LOC)

```
1. LÖSCHEN: Core/DI/Interfaces/*.h (90 Dateien)
   - Keine einzige wird genutzt!
   
2. LÖSCHEN: Core/DI/ServiceContainer.h
   - Null Resolve-Aufrufe in Produktion
   
3. LÖSCHEN: Core/DI/ServiceRegistration.h
   - Teil des ungenutzten DI Systems
```

**Effort**: 2h  
**Risiko**: NIEDRIG (Code ist ungenutzt)  
**Impact**: Kompilierzeit reduziert, weniger Verwirrung

### P1 - DIESE WOCHE (Event System Konsolidierung)

```
1. ENTSCHEIDEN: GenericEventBus vs EventDispatcher
   - GenericEventBus: Type-safe Templates, Singleton
   - EventDispatcher: Runtime Queue, IManagerBase Subscription
   
2. KONSOLIDIEREN oder KLARE TRENNUNG:
   Option A: Alles auf GenericEventBus<T> migrieren
   Option B: EventDispatcher für Cross-System, GenericEventBus für Domain
   
3. GenericEventBus.h (901 LOC) VEREINFACHEN:
   - Welche Features werden tatsächlich genutzt?
   - Batch Processing? Priority Queue? Async?
```

**Effort**: 8-16h  
**Risiko**: MITTEL (Events sind überall)  
**Impact**: Klarere Architektur, weniger Duplikation

### P2 - DIESE MONAT (Manager Konsolidierung)

```
1. GameSystemsManager ANALYSIEREN:
   - Welche Systems werden verwaltet?
   - Lazy Init Pattern nötig?
   
2. ManagerRegistry ANALYSIEREN:
   - Überlappung mit GameSystemsManager?
   - Kann entfernt oder gemerged werden?
```

**Effort**: 4-8h  
**Risiko**: NIEDRIG  
**Impact**: Weniger Indirection

### P3 - BACKLOG (Performance)

```
1. GenericEventBus Template Bloat messen
2. Lock-free Alternativen evaluieren
3. Compile-Time Impact messen
```

---

## 📁 DATEIEN ZUM LÖSCHEN (P0)

```
Core/DI/Interfaces/IActionFactory.h
Core/DI/Interfaces/IArenaAI.h
Core/DI/Interfaces/IArenaBotManager.h
Core/DI/Interfaces/IAuctionEventBus.h
Core/DI/Interfaces/IAuctionHouse.h
Core/DI/Interfaces/IAuraEventBus.h
Core/DI/Interfaces/IBattlegroundAI.h
Core/DI/Interfaces/IBattlePetManager.h
Core/DI/Interfaces/IBehaviorManager.h
Core/DI/Interfaces/IBGBotManager.h
Core/DI/Interfaces/IBotAccountMgr.h
Core/DI/Interfaces/IBotAIFactory.h
Core/DI/Interfaces/IBotCharacterDistribution.h
Core/DI/Interfaces/IBotDatabasePool.h
Core/DI/Interfaces/IBotGearFactory.h
Core/DI/Interfaces/IBotHealthCheck.h
Core/DI/Interfaces/IBotLevelDistribution.h
Core/DI/Interfaces/IBotLevelManager.h
Core/DI/Interfaces/IBotLifecycleManager.h
Core/DI/Interfaces/IBotLifecycleMgr.h
Core/DI/Interfaces/IBotMonitor.h
Core/DI/Interfaces/IBotNameMgr.h
Core/DI/Interfaces/IBotNpcLocationService.h
Core/DI/Interfaces/IBotPerformanceMonitor.h
Core/DI/Interfaces/IBotPriorityManager.h
Core/DI/Interfaces/IBotResourcePool.h
Core/DI/Interfaces/IBotScheduler.h
Core/DI/Interfaces/IBotSpawner.h
Core/DI/Interfaces/IBotSpawnEventBus.h
Core/DI/Interfaces/IBotTalentManager.h
Core/DI/Interfaces/IBotWorldEntryQueue.h
Core/DI/Interfaces/IBotWorldPositioner.h
Core/DI/Interfaces/IBotWorldSessionMgr.h
Core/DI/Interfaces/ICombatEventBus.h
Core/DI/Interfaces/IConfigManager.h
Core/DI/Interfaces/ICooldownEventBus.h
Core/DI/Interfaces/IDatabasePool.h
Core/DI/Interfaces/IDeadlockDetector.h
Core/DI/Interfaces/IDungeonBehavior.h
Core/DI/Interfaces/IDungeonScriptMgr.h
Core/DI/Interfaces/IDynamicQuestSystem.h
Core/DI/Interfaces/IEncounterStrategy.h
Core/DI/Interfaces/IEquipmentManager.h
Core/DI/Interfaces/IFarmingCoordinator.h
Core/DI/Interfaces/IGroupCoordinator.h
Core/DI/Interfaces/IGroupEventBus.h
Core/DI/Interfaces/IGuildBankManager.h
Core/DI/Interfaces/IGuildEventCoordinator.h
Core/DI/Interfaces/IGuildIntegration.h
Core/DI/Interfaces/IInstanceCoordination.h
Core/DI/Interfaces/IInstanceEventBus.h
Core/DI/Interfaces/ILFGBotManager.h
Core/DI/Interfaces/ILFGBotSelector.h
Core/DI/Interfaces/ILFGGroupCoordinator.h
Core/DI/Interfaces/ILFGRoleDetector.h
Core/DI/Interfaces/ILootAnalysis.h
Core/DI/Interfaces/ILootCoordination.h
Core/DI/Interfaces/ILootDistribution.h
Core/DI/Interfaces/ILootEventBus.h
Core/DI/Interfaces/IMarketAnalysis.h
Core/DI/Interfaces/IMountManager.h
Core/DI/Interfaces/INPCEventBus.h
Core/DI/Interfaces/IObjectiveTracker.h
Core/DI/Interfaces/IPerformanceBenchmark.h
Core/DI/Interfaces/IPlayerbotCharacterDBInterface.h
Core/DI/Interfaces/IPlayerbotConfig.h
Core/DI/Interfaces/IPlayerbotDatabaseManager.h
Core/DI/Interfaces/IPlayerbotGroupManager.h
Core/DI/Interfaces/IPlayerbotMigrationMgr.h
Core/DI/Interfaces/IProfessionAuctionBridge.h
Core/DI/Interfaces/IProfessionManager.h
Core/DI/Interfaces/IPvPCombatAI.h
Core/DI/Interfaces/IQuestCompletion.h
Core/DI/Interfaces/IQuestEventBus.h
Core/DI/Interfaces/IQuestPickup.h
Core/DI/Interfaces/IQuestTurnIn.h
Core/DI/Interfaces/IQuestValidation.h
Core/DI/Interfaces/IResourceEventBus.h
Core/DI/Interfaces/IRidingManager.h
Core/DI/Interfaces/IRoleAssignment.h
Core/DI/Interfaces/ISocialEventBus.h
Core/DI/Interfaces/ISpatialGridManager.h
Core/DI/Interfaces/IStrategyFactory.h
Core/DI/Interfaces/ISystemValidation.h
Core/DI/Interfaces/ITradeSystem.h
Core/DI/Interfaces/ITriggerFactory.h
Core/DI/Interfaces/IUnifiedInterruptSystem.h
Core/DI/Interfaces/IUnifiedLootManager.h
Core/DI/Interfaces/IUnifiedMovementCoordinator.h
Core/DI/Interfaces/IUnifiedQuestManager.h
Core/DI/Interfaces/IVendorInteraction.h
Core/DI/ServiceContainer.h
Core/DI/ServiceRegistration.h
Core/DI/MIGRATION_GUIDE.md
```

**Total**: 93 Dateien, ~5000+ LOC

---

## ✅ WAS FUNKTIONIERT (NICHT ANFASSEN)

1. **TrinityCore Script Integration** ✅
   - WorldScript, PlayerScript, GroupScript funktionieren
   
2. **Direkte Singletons** ✅
   - sBotSpawner, sBotLevelManager, GroupEventBus::instance()
   - Werden tatsächlich genutzt
   
3. **Event Publishing** ✅
   - GroupEventBus::PublishEvent() funktioniert
   - EventDispatcher::Dispatch() funktioniert

4. **Core Hooks** ✅
   - PlayerBotHooks.cpp (891 LOC) funktioniert
   - 66+ Hooks in PlayerbotEventScripts.cpp

---

## 📊 ZUSAMMENFASSUNG

| Bereich | Status | Aktion |
|---------|--------|--------|
| DI Container | ❌ UNGENUTZT | LÖSCHEN |
| 90 Interfaces | ❌ DEAD CODE | LÖSCHEN |
| GenericEventBus | ⚠️ BLOATED | VEREINFACHEN |
| EventDispatcher | ✅ GENUTZT | BEHALTEN |
| GameSystemsManager | ⚠️ UNKLAR | ANALYSIEREN |
| ManagerRegistry | ⚠️ REDUNDANT? | ANALYSIEREN |
| TrinityCore Scripts | ✅ FUNKTIONIERT | NICHT ANFASSEN |
| PlayerBotHooks | ✅ FUNKTIONIERT | NICHT ANFASSEN |

**Geschätzte Einsparung durch P0**:
- 93 Dateien weniger
- ~5000+ LOC weniger
- Schnellere Kompilierung
- Klarere Architektur
