# Phase 3: Cross-Bot Event Coordination — Master Execution Plan
## Vollständige Group Coordination Überarbeitung mit Code-Audit
## Execution Target: Claude Code

---

## 1. Executive Summary

Dieses Dokument ist der vollständige Ausführungsplan für Phase 3 Cross-Bot Event Coordination. Es baut auf dem EVENT_FLOW_MAP.md (188 Events, 13 Event Buses, 7,283 Zeilen) auf und überarbeitet die gesamte Group Coordination Infrastruktur.

**Scope**: Vollständiger Overhaul — Code-Audit aller existierenden Coordination-Systeme + Gap-Fixes + neues Bot-to-Bot Messaging + Content-Koordination für Dungeons, Raids und PvP gleichgewichtet.

**Ausführung**: Claude Code auf Branch `playerbot-dev`
**Basis-Verzeichnis**: `C:\TrinityBots\TrinityCore\src\modules\Playerbot\`
**Plan-Verzeichnis**: `C:\TrinityBots\TrinityCore\.claude\PHASE3\`

---

## 2. Ist-Zustand: Existierende Coordination-Infrastruktur

### 2.1 Event System (Core/Events/)
| Datei | Zweck | Status |
|-------|-------|--------|
| EventDispatcher.cpp/h | Zentraler Event-Router | ✅ Aktiv |
| GenericEventBus.h | Template-basierter Event Bus | ✅ Aktiv |
| BotEventTypes.h | Event-Typ-Definitionen | ✅ Aktiv |
| CombatEventRouter.cpp/h | Combat-Event-Routing | ✅ Aktiv |
| BatchedEventSubscriber.cpp/h | Batch-Processing für Events | ✅ Aktiv |
| CombatEvent.cpp/h | Combat Event Datenstruktur | ✅ Aktiv |
| ICombatEventSubscriber.h | Subscriber Interface | ✅ Aktiv |

### 2.2 Coordination Systems (AI/Coordination/)
| Datei | Zweck | Status |
|-------|-------|--------|
| RaidOrchestrator.cpp/h | Raid-weite Koordination | 🔍 AUDIT NEEDED |
| RoleCoordinator.cpp/h | Rollen-Zuweisung | 🔍 AUDIT NEEDED |
| ZoneOrchestrator.cpp/h | Zonen-basierte Koordination | 🔍 AUDIT NEEDED |

### 2.3 Dungeon Coordination (AI/Coordination/Dungeon/)
| Datei | Zweck | Status |
|-------|-------|--------|
| DungeonCoordinator.cpp/h | Dungeon-Koordination | 🔍 AUDIT NEEDED |
| BossEncounterManager.cpp/h | Boss-Strategien | 🔍 AUDIT NEEDED |
| MythicPlusManager.cpp/h | M+ Affix-Handling | 🔍 AUDIT NEEDED |
| TrashPullManager.cpp/h | Trash-Pull-Planung | 🔍 AUDIT NEEDED |
| WipeRecoveryManager.cpp/h | Wipe-Recovery | 🔍 AUDIT NEEDED |

### 2.4 Raid Coordination (AI/Coordination/Raid/)
| Datei | Zweck | Status |
|-------|-------|--------|
| RaidCoordinator.cpp/h | Raid-Koordination | 🔍 AUDIT NEEDED |
| RaidCooldownRotation.cpp/h | CD-Rotation | 🔍 AUDIT NEEDED |
| RaidEncounterManager.cpp/h | Encounter-Strategien | 🔍 AUDIT NEEDED |
| RaidGroupManager.cpp/h | Raid-Gruppen | 🔍 AUDIT NEEDED |
| RaidHealCoordinator.cpp/h | Heal-Koordination | 🔍 AUDIT NEEDED |
| RaidPositioningManager.cpp/h | Raid-Positioning | 🔍 AUDIT NEEDED |
| RaidTankCoordinator.cpp/h | Tank-Koordination | 🔍 AUDIT NEEDED |
| AddManagementSystem.cpp/h | Add-Management | 🔍 AUDIT NEEDED |
| KitingManager.cpp/h | Kiting-Verhalten | 🔍 AUDIT NEEDED |

### 2.5 Arena Coordination (AI/Coordination/Arena/)
| Datei | Zweck | Status |
|-------|-------|--------|
| ArenaCoordinator.cpp/h | Arena-Hauptkoordination | 🔍 AUDIT NEEDED |
| ArenaPositioning.cpp/h | Arena-Positioning | 🔍 AUDIT NEEDED |
| BurstCoordinator.cpp/h | Burst-Fenster-Koordination | 🔍 AUDIT NEEDED |
| CCChainManager.cpp/h | CC-Chain-Management | 🔍 AUDIT NEEDED |
| DefensiveCoordinator.cpp/h | Defensive CD-Koordination | 🔍 AUDIT NEEDED |
| KillTargetManager.cpp/h | Kill-Target-Selektion | 🔍 AUDIT NEEDED |

### 2.6 Battleground Coordination (AI/Coordination/Battleground/)
| Datei | Zweck | Status |
|-------|-------|--------|
| BattlegroundCoordinator.cpp/h | BG-Hauptkoordination | 🔍 AUDIT NEEDED |
| BattlegroundCoordinatorManager.cpp/h | BG-Manager | 🔍 AUDIT NEEDED |
| BGStrategyEngine.cpp/h | BG-Strategien | 🔍 AUDIT NEEDED |
| BGRoleManager.cpp/h | BG-Rollen | 🔍 AUDIT NEEDED |
| FlagCarrierManager.cpp/h | Flag-Carrier-Logik | 🔍 AUDIT NEEDED |
| NodeController.cpp/h | Node-Kontrolle | 🔍 AUDIT NEEDED |
| ObjectiveManager.cpp/h | BG-Objectives | 🔍 AUDIT NEEDED |

### 2.7 Combat Behaviors (AI/CombatBehaviors/)
| Datei | Zweck | Status |
|-------|-------|--------|
| InterruptRotationManager.cpp/h | Interrupt-Rotation | 🔍 AUDIT NEEDED |
| DispelCoordinator.cpp/h | Dispel-Koordination | ⚠️ GAP 2: Keine Zuweisung |
| DefensiveBehaviorManager.cpp/h | Defensive CDs | ⚠️ GAP 3: Keine externe CD-Rotation |
| CooldownStackingOptimizer.cpp/h | CD-Stacking | 🔍 AUDIT NEEDED |
| AoEDecisionManager.cpp/h | AoE-Entscheidungen | 🔍 AUDIT NEEDED |

### 2.8 Group Systems (Group/)
| Datei | Zweck | Status |
|-------|-------|--------|
| GroupCoordination.cpp/h | Gruppen-Koordination | 🔍 AUDIT NEEDED |
| GroupEventHandler.cpp/h | Group Event Handling | 🔍 AUDIT NEEDED |
| GroupFormation.cpp/h | Formationen | 🔍 AUDIT NEEDED |
| RoleAssignment.cpp/h | Rollen-Zuweisung | 🔍 AUDIT NEEDED |
| GroupStateMachine.cpp/h | Gruppen-Zustandsautomat | 🔍 AUDIT NEEDED |

### 2.9 Advanced Coordination (Advanced/)
| Datei | Zweck | Status |
|-------|-------|--------|
| GroupCoordinator.cpp/h | Gruppen-Koordination | ⚠️ DUPLIKAT zu Group/GroupCoordination? |
| TacticalCoordinator.cpp/h | Taktische Koordination | 🔍 AUDIT NEEDED |
| SocialManager.cpp/h | Soziale Interaktion | 🔍 AUDIT NEEDED |

### 2.10 Combat Coordination (AI/Combat/)
| Datei | Zweck | Status |
|-------|-------|--------|
| InterruptCoordinator.cpp/h | Interrupt-Koordination | ⚠️ DUPLIKAT zu CombatBehaviors/InterruptRotationManager? |
| ThreatCoordinator.cpp/h | Threat-Koordination | 🔍 AUDIT NEEDED |
| DefensiveManager.cpp/h | Defensive CDs | ⚠️ DUPLIKAT zu CombatBehaviors/DefensiveBehaviorManager? |
| FormationManager.cpp/h | Formationen | ⚠️ DUPLIKAT zu Group/GroupFormation? |
| PositionManager.cpp/h | Positioning | 🔍 AUDIT NEEDED |
| TargetManager.cpp/h | Target-Selektion | 🔍 AUDIT NEEDED |
| UnifiedInterruptSystem.cpp/h | Unified Interrupt | ⚠️ DUPLIKAT zu InterruptCoordinator? |
| CrowdControlManager.cpp/h | CC-Management | 🔍 AUDIT NEEDED |


### 2.11 Event Buses — Status aus EVENT_FLOW_MAP.md

| # | Event Bus | Events | Status |
|---|-----------|--------|--------|
| 1 | CombatEventBus | 22 | ✅ Aktiv |
| 2 | GroupEventBus | 18 | ✅ Aktiv |
| 3 | LootEventBus | 12 | ✅ Aktiv |
| 4 | QuestEventBus | 14 | ✅ Aktiv |
| 5 | AuctionEventBus | 10 | ✅ Aktiv |
| 6 | ResourceEventBus | 8 | ✅ Aktiv |
| 7 | SocialEventBus | 11 | ✅ Aktiv |
| 8 | InstanceEventBus | 9 | ✅ Aktiv |
| 9 | NPCEventBus | 7 | ✅ Aktiv |
| 10 | AuraEventBus | 6 | ✅ Aktiv |
| 11 | ProfessionEventBus | 8 | ✅ Aktiv |
| 12 | BotSpawnEventBus | 12 | ✅ Aktiv |
| 13 | **CooldownEventBus** | **0** | 🔴 **DEAD — GAP 1** |

---

## 3. Identifizierte Probleme

### 3.1 Kritische Gaps (aus EVENT_FLOW_MAP.md)

#### GAP 1: CooldownEventBus — DEAD (🔴 KRITISCH)
- **Problem**: Keine Events werden published. 0 aktive Events.
- **Impact**: Keine Cooldown-Koordination zwischen Bots. Bots wissen nicht, welche CDs ihre Gruppe aktuell hat oder verbraucht hat.
- **Root Cause**: SMSG_SPELL_COOLDOWN und SMSG_SPELL_CATEGORY_COOLDOWN Packet-Handler sind nicht implementiert.
- **Fix**: 
  1. Implementiere `ParseCooldownPacket_Typed.cpp` Handlers für SMSG_SPELL_COOLDOWN, SMSG_COOLDOWN_EVENT, SMSG_SPELL_CATEGORY_COOLDOWN
  2. Publish Events: COOLDOWN_STARTED, COOLDOWN_ENDED, CATEGORY_COOLDOWN_UPDATED
  3. Subscriber: RaidCooldownRotation, DefensiveBehaviorManager, BurstCoordinator
- **Aufwand**: 2-3 Tage

#### GAP 2: Dispel Rotation — Keine Zuweisung (🟡 MITTEL)
- **Problem**: DISPELLABLE_DETECTED wird published, aber kein Dispeller-Zuweisungssystem existiert.
- **Impact**: Mehrere Healer dispellen gleichzeitig dasselbe Target, verschwenden GCDs und Mana.
- **Fix**:
  1. Erweitere DispelCoordinator mit Zuweisungslogik
  2. Implementiere Dispeller-Priority-Queue (nächster Dispeller, kürzester CD, passender Dispel-Typ)
  3. Claim-System: Bot claimed Dispel-Target, andere Bots ignorieren es für X ms
- **Aufwand**: 1-2 Tage

#### GAP 3: External Defensive CD Rotation (🟡 MITTEL)
- **Problem**: DEFENSIVE_NEEDED trigger fired, aber keine Zuweisung von externen CDs.
- **Impact**: Duplizierte externe CDs (Pain Suppression + Guardian Spirit auf dasselbe Target).
- **Fix**:
  1. Implementiere External CD Assignment in DefensiveBehaviorManager
  2. Danger-Window-System: 6s Fenster in dem ein Bot als "geschützt" gilt
  3. CD-Priority: Major CDs (Guardian Spirit, Pain Suppression) vs Minor CDs (Life Cocoon, Ironbark)
  4. Synchronisierung mit RaidCooldownRotation
- **Aufwand**: 2-3 Tage

#### GAP 4: DBM/BigWigs Integration (🟡 MITTEL)
- **Problem**: Keine Pull-Timer oder Boss-Ability-Warning Integration.
- **Impact**: Bots pre-potten nicht, pre-positionieren nicht, nutzen keine Defensiv-CDs für große Hits.
- **Fix**:
  1. Option A: Parse CHAT_MSG_ADDON für DBM/BigWigs Timer-Messages
  2. Option B: Parse Combat Log für bekannte Boss-Abilities (SPELL_CAST_START)
  3. Option C (empfohlen): Eigenes BossAbilityDatabase — mappt Boss Entry + Spell ID → Warnung + benötigte Reaktion
  4. Implementiere Pre-Pull-Sequenz: Flask → Food → Pre-Pot → Burst-CDs
  5. Implementiere Boss-Ability-Reaktionen: Stack → Spread → Soak → Dodge → Defensive
- **Aufwand**: 3-5 Tage

#### GAP 5: M+ Affix Support — Limitiert (🟡 NIEDRIG)
- **Problem**: Nur 4 von 12+ Affixen handled (Sanguine, Quaking, Explosive, Necrotic)
- **Fehlend**: Incorporeal, Afflicted, Entangling, Bursting, Spiteful, Storming, Volcanic, Raging
- **Fix**: Implementiere pro Affix ein AffixHandler in MythicPlusManager
- **Aufwand**: 5-7 Tage


### 3.2 Verdächtige Duplikate (Code-Audit benötigt)

| Duplikat-Paar | Verzeichnis A | Verzeichnis B | Audit-Frage |
|---------------|--------------|--------------|-------------|
| Interrupt Coordination | `AI/Combat/InterruptCoordinator` | `AI/CombatBehaviors/InterruptRotationManager` + `AI/Combat/UnifiedInterruptSystem` | 3 Dateien für Interrupt-Koordination? Welche ist kanonisch? |
| Defensive Management | `AI/Combat/DefensiveManager` | `AI/CombatBehaviors/DefensiveBehaviorManager` | Welche wird tatsächlich benutzt? |
| Formation Management | `AI/Combat/FormationManager` | `Group/GroupFormation` | Überlappung bei Formations-Logik? |
| Group Coordination | `Advanced/GroupCoordinator` | `Group/GroupCoordination` | Zwei Group Coordinators in verschiedenen Verzeichnissen? |
| Kiting | `AI/Combat/KitingManager` | `AI/Coordination/Raid/KitingManager` | Generisch vs Raid-spezifisch? |

### 3.3 Architektur-Probleme

1. **Kein einheitliches Bot-to-Bot Messaging**: Bots kommunizieren nicht direkt. Stattdessen wird alles über globale Manager geroutet. Das skaliert schlecht bei 5000 Bots.
2. **Kein Claim-System**: Wenn ein Boss einen interruptbaren Cast startet, reagieren potenziell alle Bots gleichzeitig. Es gibt kein "Ich übernehme das, ihr nicht."
3. **Event-Subscriber wissen nichts voneinander**: DispelCoordinator und DefensiveBehaviorManager können auf dasselbe Event reagieren, ohne zu wissen, dass der jeweils andere bereits reagiert.
4. **Keine Content-Awareness**: Der ZoneOrchestrator weiß nicht, ob die Gruppe in einem M+, einem Raid oder einer Arena ist. Koordinationsverhalten sollte kontextabhängig sein.
5. **CooldownEventBus ist tot**: Ohne Cooldown-Events ist jede CD-Rotation blind.

---

## 4. Ziel-Architektur

### 4.1 Neue Komponente: BotMessageBus

```
┌─────────────────────────────────────────────────────────┐
│                    BotMessageBus                         │
│  (Direkte Bot-to-Bot Kommunikation innerhalb Gruppe)     │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  Messages:                                               │
│  ├─ CLAIM_INTERRUPT(spellId, targetGuid)                │
│  ├─ CLAIM_DISPEL(targetGuid, auraId)                    │
│  ├─ CLAIM_DEFENSIVE(targetGuid, spellId)                │
│  ├─ REQUEST_HEAL(urgency, healthPct)                    │
│  ├─ ANNOUNCE_BURST_WINDOW(duration)                     │
│  ├─ ANNOUNCE_CC(targetGuid, spellId, duration)          │
│  ├─ REQUEST_TANK_SWAP(bossGuid, reason)                 │
│  ├─ ANNOUNCE_POSITION_CHANGE(newPos, reason)            │
│  ├─ ANNOUNCE_CD_USAGE(spellId, cdRemaining)             │
│  └─ ANNOUNCE_DEATH(killerGuid)                          │
│                                                          │
│  Delivery:                                               │
│  ├─ GROUP_BROADCAST (alle in der Gruppe)                │
│  ├─ ROLE_BROADCAST (alle Tanks/Healer/DPS)              │
│  ├─ DIRECT (an einen bestimmten Bot)                    │
│  └─ PRIORITY_BROADCAST (nur nächste N Bots)             │
│                                                          │
│  Claim Resolution:                                       │
│  ├─ First-Claim-Wins mit Timeout (200ms)                │
│  ├─ Priority-Based (niedrigerer CD gewinnt)             │
│  └─ Role-Based (Healer vor DPS bei Dispel)              │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

### 4.2 Neue Komponente: ContentContextManager

```
┌─────────────────────────────────────────────────────────┐
│                ContentContextManager                     │
│  (Erkennt automatisch den Content-Typ)                   │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  Context Types:                                          │
│  ├─ SOLO           → Keine Koordination                 │
│  ├─ OPEN_WORLD_5   → Leichte Koordination               │
│  ├─ DUNGEON_NORMAL → Standard-Koordination              │
│  ├─ DUNGEON_HEROIC → Erhöhte Koordination               │
│  ├─ MYTHIC_PLUS    → Volle Koordination + Affix-Aware   │
│  ├─ RAID_NORMAL    → Raid-Koordination                  │
│  ├─ RAID_HEROIC    → Volle Raid-Koordination            │
│  ├─ RAID_MYTHIC    → Maximale Koordination              │
│  ├─ ARENA_2V2      → Arena-spezifisch                   │
│  ├─ ARENA_3V3      → Arena-spezifisch                   │
│  ├─ BATTLEGROUND   → BG-spezifisch                      │
│  └─ RATED_BG       → Erhöhte BG-Koordination            │
│                                                          │
│  Liefert an alle Coordinators:                           │
│  ├─ coordinationLevel (0.0 - 1.0)                       │
│  ├─ requiredFeatures[] (interrupt, dispel, defensive...) │
│  ├─ activeMechanics[] (affixes, boss abilities...)       │
│  └─ groupSize (für Skalierung)                           │
│                                                          │
└─────────────────────────────────────────────────────────┘
```


### 4.3 Konsolidierte Coordination-Architektur

```
                    ┌──────────────────────────┐
                    │  ContentContextManager    │
                    │  (Erkennt Content-Typ)    │
                    └─────────┬────────────────┘
                              │
                    ┌─────────▼────────────────┐
                    │  GroupCoordinationHub     │
                    │  (Zentraler Einstieg)     │
                    │  Ersetzt: Advanced/       │
                    │  GroupCoordinator +        │
                    │  Group/GroupCoordination   │
                    └─────────┬────────────────┘
                              │
              ┌───────────────┼───────────────┐
              │               │               │
    ┌─────────▼──────┐ ┌─────▼──────┐ ┌──────▼──────────┐
    │ CombatCoord    │ │ RoleCoord  │ │ ContentCoord     │
    │ ─────────────  │ │ ──────────── │ │ ──────────────── │
    │ InterruptMgr   │ │ TankCoord  │ │ DungeonCoord     │
    │ DispelMgr      │ │ HealCoord  │ │ RaidCoord        │
    │ DefensiveMgr   │ │ DPSCoord   │ │ ArenaCoord       │
    │ CooldownMgr    │ │ AssignMgr  │ │ BGCoord          │
    │ CCManager      │ │            │ │ M+Manager        │
    └────────────────┘ └────────────┘ └──────────────────┘
              │               │               │
              └───────────────┼───────────────┘
                              │
                    ┌─────────▼────────────────┐
                    │     BotMessageBus         │
                    │  (Bot-to-Bot Messaging)   │
                    │  Claims, Announcements,   │
                    │  Requests, Broadcasts     │
                    └─────────┬────────────────┘
                              │
                    ┌─────────▼────────────────┐
                    │   Event Bus Layer         │
                    │  (Bestehende 13 Buses)    │
                    │  + CooldownEventBus FIX   │
                    └──────────────────────────┘
```

### 4.4 Duplikat-Auflösung (Ziel)

| Problem | Lösung | Neue kanonische Datei |
|---------|--------|-----------------------|
| 3x Interrupt | Konsolidiere zu UnifiedInterruptSystem | `AI/Combat/UnifiedInterruptSystem.cpp/h` |
| 2x Defensive | Konsolidiere zu DefensiveBehaviorManager | `AI/CombatBehaviors/DefensiveBehaviorManager.cpp/h` |
| 2x Formation | Konsolidiere zu GroupFormation | `Group/GroupFormation.cpp/h` |
| 2x GroupCoord | Konsolidiere zu GroupCoordinationHub (NEU) | `AI/Coordination/GroupCoordinationHub.cpp/h` |
| 2x Kiting | Behalte beide: Generisch + Raid-spezifisch erbt | `AI/Combat/KitingManager` → Base, `Raid/KitingManager` → Override |

---

## 5. Sprint-Plan

### Übersicht: 6 Sprints, ~6-8 Wochen

| Sprint | Name | Dauer | Abhängigkeit |
|--------|------|-------|-------------|
| S1 | Code-Audit & Duplikat-Analyse | 3-4 Tage | — |
| S2 | Infrastruktur: BotMessageBus + CooldownEventBus Fix | 4-5 Tage | S1 |
| S3 | Combat Coordination Overhaul | 5-7 Tage | S2 |
| S4 | Dungeon & M+ Coordination | 5-7 Tage | S3 |
| S5 | Raid Coordination | 5-7 Tage | S3 |
| S6 | PvP Coordination (Arena + BG) | 5-7 Tage | S3 |

> S4, S5, S6 können teilweise parallel laufen nach S3-Abschluss.


---

### Sprint 1: Code-Audit & Duplikat-Analyse (3-4 Tage)

**Ziel**: Vollständiges Bild aller existierenden Coordination-Dateien, ihrer Abhängigkeiten, aktiven Nutzung, und Duplikate.

#### S1.1 — Statische Analyse aller Coordination-Dateien (Tag 1)
**Aktion**: Für jede Datei in der Audit-Liste (Sektion 2) analysiere:
- [ ] Wird die Klasse instantiiert? (Suche nach `new ClassName`, `make_unique<ClassName>`, `make_shared<ClassName>`)
- [ ] Hat sie aktive Subscriber/Publisher auf Event Buses?
- [ ] Wird sie von anderen Dateien referenziert? (#include-Analyse)
- [ ] Enthält sie Stub-Code oder TODO-Kommentare?
- [ ] Kompiliert sie (ist in CMakeLists.txt gelistet)?

**Output**: `AUDIT_RESULTS.md` mit Status pro Datei:
- 🟢 AKTIV — wird benutzt, funktional
- 🟡 PARTIAL — teilweise implementiert, hat Stubs
- 🔴 DEAD — wird nie instantiiert/aufgerufen
- ⚠️ DUPLIKAT — gleiche Funktionalität wie andere Datei

**Dateien zu auditieren (48 Dateien)**:
```
AI/Coordination/RaidOrchestrator.cpp/h
AI/Coordination/RoleCoordinator.cpp/h
AI/Coordination/ZoneOrchestrator.cpp/h
AI/Coordination/Dungeon/*.cpp/h          (10 Dateien)
AI/Coordination/Raid/*.cpp/h             (18 Dateien)
AI/Coordination/Arena/*.cpp/h            (12 Dateien)
AI/Coordination/Battleground/*.cpp/h     (14+ Dateien)
AI/CombatBehaviors/*.cpp/h               (10 Dateien)
AI/Combat/InterruptCoordinator.cpp/h
AI/Combat/UnifiedInterruptSystem.cpp/h
AI/Combat/DefensiveManager.cpp/h
AI/Combat/FormationManager.cpp/h
AI/Combat/ThreatCoordinator.cpp/h
AI/Combat/CrowdControlManager.cpp/h
AI/Combat/PositionManager.cpp/h
AI/Combat/TargetManager.cpp/h
Advanced/GroupCoordinator.cpp/h
Advanced/TacticalCoordinator.cpp/h
Group/GroupCoordination.cpp/h
Group/GroupEventHandler.cpp/h
Group/GroupFormation.cpp/h
Group/RoleAssignment.cpp/h
Group/GroupStateMachine.cpp/h
```

#### S1.2 — Duplikat-Bestätigung (Tag 2)
**Aktion**: Für jedes vermutete Duplikat-Paar (Sektion 3.2):
- [ ] Vergleiche die öffentlichen APIs (Methoden-Signaturen)
- [ ] Vergleiche die internen Algorithmen
- [ ] Identifiziere welche Version vollständiger/besser ist
- [ ] Entscheide: Merge, Keep Both, oder Delete One

**Output**: Erweiterung von `AUDIT_RESULTS.md` mit Duplikat-Resolutions

#### S1.3 — Event-Flow-Validierung (Tag 2-3)
**Aktion**: Für jeden Event Bus der coordination-relevant ist:
- [ ] Verifiziere alle Publisher → tatsächlich aktiv?
- [ ] Verifiziere alle Subscriber → reagieren sie korrekt?
- [ ] Identifiziere verwaiste Subscriber (Subscriber ohne passenden Publisher)
- [ ] Identifiziere tote Publisher (Publisher ohne Subscriber)

**Fokus-Buses**: CombatEventBus, GroupEventBus, CooldownEventBus, InstanceEventBus

**Output**: Event-Flow-Validierungs-Report in `AUDIT_RESULTS.md`

#### S1.4 — Konsolidierungsplan erstellen (Tag 3-4)
**Aktion**: Basierend auf Audit-Ergebnissen:
- [ ] Erstelle konkreten Merge-Plan für jedes Duplikat-Paar
- [ ] Definiere Migration-Steps (welche Referenzen müssen aktualisiert werden)
- [ ] Identifiziere Breaking Changes und erstelle Kompatibilitäts-Wrapper falls nötig
- [ ] Update CMakeLists.txt Plan

**Output**: `CONSOLIDATION_PLAN.md`


---

### Sprint 2: Infrastruktur — BotMessageBus + CooldownEventBus Fix (4-5 Tage)

**Ziel**: Die zwei fundamentalen Infrastruktur-Komponenten bauen, auf denen alles andere aufsetzt.

#### S2.1 — CooldownEventBus Fix (GAP 1) (Tag 1-2)

**Neue/Modifizierte Dateien**:
```
MODIFY: Network/ParseCooldownPacket_Typed.cpp
MODIFY: Cooldown/CooldownEvents.cpp/h
```

**Implementierung**:
- [ ] Implementiere Packet-Handler für:
  - `SMSG_SPELL_COOLDOWN` → Parsed Spell-ID + Cooldown-Dauer
  - `SMSG_COOLDOWN_EVENT` → Cooldown abgelaufen
  - `SMSG_SPELL_CATEGORY_COOLDOWN` → Kategorie-CD (z.B. Trinkets)
- [ ] Publish Events auf CooldownEventBus:
  - `COOLDOWN_STARTED { botGuid, spellId, duration, category }`
  - `COOLDOWN_ENDED { botGuid, spellId }`
  - `MAJOR_CD_AVAILABLE { botGuid, spellId, cdType }` (für Raid-CDs)
  - `MAJOR_CD_USED { botGuid, spellId, cdDuration }` (für Raid-CDs)
- [ ] Definiere MAJOR_CD Liste: Bloodlust, Rebirth, Innervate, Pain Suppression, Guardian Spirit, Rallying Cry, Darkness, Anti-Magic Zone, etc.
- [ ] Subscriber registrieren:
  - RaidCooldownRotation → MAJOR_CD_USED, MAJOR_CD_AVAILABLE
  - DefensiveBehaviorManager → MAJOR_CD_USED, COOLDOWN_ENDED
  - BurstCoordinator (Arena) → COOLDOWN_STARTED, COOLDOWN_ENDED

**Tests**:
- [ ] Unit Test: CooldownEvent parsing korrekt
- [ ] Unit Test: Subscriber werden benachrichtigt
- [ ] Integration Test: RaidCooldownRotation reagiert auf MAJOR_CD_USED

#### S2.2 — BotMessageBus Implementation (Tag 2-4)

**Neue Dateien**:
```
NEW: AI/Coordination/Messaging/BotMessageBus.cpp/h
NEW: AI/Coordination/Messaging/BotMessage.h
NEW: AI/Coordination/Messaging/ClaimResolver.cpp/h
NEW: AI/Coordination/Messaging/MessageTypes.h
```

**Implementierung**:
```cpp
// MessageTypes.h
enum class BotMessageType : uint8 {
    // Claims — "Ich übernehme das"
    CLAIM_INTERRUPT,        // Ich unterbreche diesen Cast
    CLAIM_DISPEL,           // Ich dispelle dieses Target
    CLAIM_DEFENSIVE_CD,     // Ich schütze dieses Target
    CLAIM_CC,               // Ich CC'e dieses Target
    CLAIM_SOAK,             // Ich soake diese Mechanic

    // Announcements — "Info für alle"
    ANNOUNCE_CD_USAGE,      // Ich habe CD X benutzt
    ANNOUNCE_BURST_WINDOW,  // Burst-Fenster offen für X sec
    ANNOUNCE_POSITION,      // Ich bewege mich nach X
    ANNOUNCE_DEATH,         // Ich bin gestorben
    ANNOUNCE_RESURRECT,     // Ich resse Target X
    ANNOUNCE_LOW_RESOURCE,  // Mana/Health niedrig

    // Requests — "Ich brauche Hilfe"
    REQUEST_HEAL,           // Brauche Heilung
    REQUEST_EXTERNAL_CD,    // Brauche externes CD
    REQUEST_TANK_SWAP,      // Brauche Tank-Swap
    REQUEST_RESCUE,         // Feststeckend/OOM/usw

    // Commands — "Alle sollen X tun" (nur Leader/Coordinator)
    CMD_FOCUS_TARGET,       // Alle auf Target X
    CMD_SPREAD,             // Alle spreaden
    CMD_STACK,              // Alle stacken
    CMD_MOVE_TO,            // Alle nach Position X
    CMD_USE_DEFENSIVES,     // Alle Defensiv-CDs
    CMD_BLOODLUST,          // Heroism/Bloodlust jetzt
};

enum class MessageScope : uint8 {
    GROUP_BROADCAST,    // An alle in der Gruppe
    ROLE_BROADCAST,     // An alle mit bestimmter Rolle (Tank/Healer/DPS)
    SUBGROUP_BROADCAST, // An alle in Raid-Subgruppe
    DIRECT,             // An einen bestimmten Bot
};
```

- [ ] Implementiere BotMessageBus als in-memory Publish/Subscribe pro Gruppe
- [ ] Implementiere ClaimResolver:
  - First-Claim-Wins mit 200ms Timeout
  - Priority-basiert (niedrigerer CD, passendere Rolle)
  - Automatischer Claim-Release wenn Bot stirbt/OOM/stunned
- [ ] Implementiere Message Batching (sammle Messages pro Update-Tick, verarbeite batch)
- [ ] Performance: O(1) Lookup per Group, max 100 Messages/sec pro Gruppe

**Tests**:
- [ ] Unit Test: Message Publishing + Subscribing
- [ ] Unit Test: ClaimResolver — First-Claim-Wins
- [ ] Unit Test: ClaimResolver — Priority-Override
- [ ] Unit Test: Claim-Release bei Bot-Tod
- [ ] Perf Test: 1000 Messages/sec Durchsatz

#### S2.3 — ContentContextManager (Tag 4-5)

**Neue Dateien**:
```
NEW: AI/Coordination/ContentContextManager.cpp/h
```

**Implementierung**:
- [ ] Erkennung des Content-Typs basierend auf:
  - `Map::IsInstanceType()` → Dungeon/Raid
  - `Battleground*` Pointer → PvP
  - Difficulty-Flags → Normal/Heroic/Mythic
  - M+ Key Level → Mythic Plus
  - Arena-Bracket → 2v2/3v3
- [ ] Liefere `ContentContext` Struct an alle Coordinators:
  ```cpp
  struct ContentContext {
      ContentType type;           // DUNGEON_HEROIC, RAID_MYTHIC, ARENA_3V3...
      float coordinationLevel;    // 0.0 (solo) - 1.0 (mythic raid)
      uint32 groupSize;
      uint32 difficultyId;
      uint32 mythicPlusLevel;     // 0 wenn kein M+
      std::vector<uint32> activeAffixes; // M+ Affixe
      uint32 encounterEntry;      // Aktueller Boss (0 wenn Trash)
  };
  ```
- [ ] Integriere mit ZoneOrchestrator (existierend)

**Tests**:
- [ ] Unit Test: Korrekte Erkennung für jeden Content-Typ
- [ ] Unit Test: coordinationLevel skaliert korrekt


---

### Sprint 3: Combat Coordination Overhaul (5-7 Tage)

**Ziel**: Konsolidiere Duplikate, implementiere Claim-System für alle Combat-Koordinations-Systeme, integriere BotMessageBus.

#### S3.1 — Duplikat-Konsolidierung (Tag 1-2)

Basierend auf S1 Audit-Ergebnissen, führe die definierten Merges durch:

**Interrupt-System-Konsolidierung**:
- [ ] Identifiziere beste Implementation aus: InterruptCoordinator, InterruptRotationManager, UnifiedInterruptSystem
- [ ] Migriere alle Features in die kanonische Datei
- [ ] Aktualisiere alle #include Referenzen
- [ ] Markiere deprecated Dateien mit `[[deprecated]]`
- [ ] Integriere BotMessageBus: `CLAIM_INTERRUPT` Message vor jedem Interrupt
- [ ] Entferne toten Code

**Defensive-System-Konsolidierung**:
- [ ] Merge DefensiveManager + DefensiveBehaviorManager
- [ ] Integriere BotMessageBus: `CLAIM_DEFENSIVE_CD` + `ANNOUNCE_CD_USAGE`
- [ ] Implementiere GAP 3 Fix: External CD Rotation mit Danger-Window

**Formation-System-Konsolidierung**:
- [ ] Merge FormationManager + GroupFormation
- [ ] Integriere BotMessageBus: `ANNOUNCE_POSITION` bei Formation-Wechsel

#### S3.2 — Dispel-Rotation Fix (GAP 2) (Tag 2-3)

**Modifizierte Dateien**:
```
MODIFY: AI/CombatBehaviors/DispelCoordinator.cpp/h
```

**Implementierung**:
- [ ] Implementiere Dispel-Priority-Algorithmus:
  ```
  Priority = basePriority
    + (canDispelType ? 100 : 0)       // Passender Dispel-Typ
    + (distanceToTarget < 30 ? 50 : 0) // In Reichweite
    + (dispelOnCD ? -200 : 0)          // CD verfügbar
    + (isHealer ? 30 : 0)              // Healer bevorzugt
    + (currentGCDRemaining < 0.5 ? 20 : 0) // GCD fast frei
  ```
- [ ] Implementiere Claim-System via BotMessageBus:
  1. DISPELLABLE_DETECTED Event kommt rein
  2. Jeder Dispel-fähige Bot berechnet seine Priority
  3. Bot mit höchster Priority sendet CLAIM_DISPEL
  4. ClaimResolver bestätigt oder lehnt ab (200ms Window)
  5. Nur bestätigter Bot führt Dispel aus
- [ ] Handling von Claim-Failures: Fallback zum nächst-besten Bot

#### S3.3 — External Defensive CD Rotation Fix (GAP 3) (Tag 3-4)

**Modifizierte Dateien**:
```
MODIFY: AI/CombatBehaviors/DefensiveBehaviorManager.cpp/h
MODIFY: AI/Coordination/Raid/RaidCooldownRotation.cpp/h
```

**Implementierung**:
- [ ] Definiere External-CD-Kategorien:
  ```cpp
  enum class ExternalCDTier {
      MAJOR,    // Guardian Spirit, Pain Suppression, Ironbark, Life Cocoon
      MODERATE, // Blessing of Sacrifice, Vigilance
      MINOR,    // Power Word: Barrier (group), Darkness (group), AMZ (group)
      RAID,     // Rallying Cry, Spirit Link, Devotion Aura, Healing Tide
  };
  ```
- [ ] Implementiere Danger-Detection:
  - Health unter 30% → DANGER
  - Incoming Boss-Ability (aus BossAbilityDatabase) → PRE_DANGER
  - Stacking DoT (>3 stacks) → MODERATE_DANGER
- [ ] Implementiere CD-Assignment via BotMessageBus:
  1. DEFENSIVE_NEEDED Event mit Urgency-Level
  2. Verfügbare Healer/Support checken ihre CDs
  3. CLAIM_DEFENSIVE_CD mit Tier + CD-Dauer
  4. ClaimResolver wählt passenden Tier (nicht MAJOR für MODERATE_DANGER)
  5. Nach Nutzung: ANNOUNCE_CD_USAGE → RaidCooldownRotation trackt es
- [ ] Implementiere Danger-Window: 6s nach External-CD-Nutzung ist Target "geschützt"

#### S3.4 — Interrupt-Rotation Enhancement (Tag 4-5)

**Modifizierte Dateien**:
```
MODIFY: AI/Combat/UnifiedInterruptSystem.cpp/h (kanonisch nach S3.1)
```

**Implementierung**:
- [ ] Integriere mit BotMessageBus CLAIM_INTERRUPT System
- [ ] Implementiere Interrupt-Rotation pro Boss-Encounter:
  - Rotation-Liste: Bot A → Bot B → Bot C → Bot A ...
  - Automatischer Skip wenn Bot's Interrupt auf CD
  - Priority-Override für "must-interrupt" Spells (z.B. Boss-Heal)
- [ ] Integriere mit CooldownEventBus (GAP 1 Fix):
  - COOLDOWN_STARTED(interrupt_spell) → Bot aus Rotation entfernen
  - COOLDOWN_ENDED(interrupt_spell) → Bot wieder in Rotation aufnehmen

#### S3.5 — Threat Coordination Enhancement (Tag 5-6)

**Modifizierte Dateien**:
```
MODIFY: AI/Combat/ThreatCoordinator.cpp/h
```

**Implementierung**:
- [ ] Integriere BotMessageBus für Tank-Swap-Kommunikation:
  - REQUEST_TANK_SWAP bei Boss-Debuff-Stack > Threshold
  - CLAIM_INTERRUPT für Taunt
  - ANNOUNCE_POSITION nach Tank-Swap
- [ ] Implementiere Threat-Table-Sharing zwischen Bots:
  - Jeder Bot teilt sein aktuelles Threat-Level per Tick
  - DPS-Bots können Threat-Reduction nutzen wenn nötig

#### S3.6 — CC Coordination (Tag 6-7)

**Modifizierte Dateien**:
```
MODIFY: AI/Combat/CrowdControlManager.cpp/h
```

**Implementierung**:
- [ ] Integriere BotMessageBus CLAIM_CC System
- [ ] Implementiere CC-Assignment:
  - CC-Priority basierend auf Target-Gefährlichkeit
  - CC-Typ-Matching (Polymorph für Humanoide, Hibernate für Beasts, etc.)
  - Automatische Re-CC vor Ablauf
- [ ] DR-Tracking (Diminishing Returns): Tracke DR pro Target + CC-Kategorie


---

### Sprint 4: Dungeon & M+ Coordination (5-7 Tage)

**Ziel**: Vollständige Dungeon-Koordination mit M+-Support für alle Affixe.

#### S4.1 — DungeonCoordinator Überarbeitung (Tag 1-2)

**Modifizierte Dateien**:
```
MODIFY: AI/Coordination/Dungeon/DungeonCoordinator.cpp/h
MODIFY: AI/Coordination/Dungeon/DungeonState.h
```

**Implementierung**:
- [ ] Integriere ContentContextManager für Dungeon-Difficulty-Awareness
- [ ] Integriere BotMessageBus für:
  - CMD_FOCUS_TARGET vom Tank (Pull-Target)
  - CMD_STACK / CMD_SPREAD für Trash-Pulls
  - ANNOUNCE_POSITION bei Pulls
- [ ] Implementiere Dungeon-Phase-Machine:
  ```
  IDLE → PULLING → IN_COMBAT → LOOTING → MOVING_TO_NEXT → BOSS_PREP → BOSS_COMBAT → BOSS_LOOT
  ```
- [ ] Implementiere Ready-Check vor Boss-Pulls:
  - Alle Bots > 80% Mana?
  - Alle Bots > 90% Health?
  - Alle wichtigen CDs available?
  - Pre-Buff check (Fortitude, Arcane Intellect, etc.)

#### S4.2 — TrashPullManager Enhancement (Tag 2-3)

**Modifizierte Dateien**:
```
MODIFY: AI/Coordination/Dungeon/TrashPullManager.cpp/h
```

**Implementierung**:
- [ ] Implementiere intelligente Pull-Planung:
  - Maximal X Mobs pro Pull basierend auf Gruppen-Stärke
  - Patrouille-Tracking: Warte bis Patrouille weg ist
  - LoS-Pull-Erkennung: Tank zieht um Ecke, Gruppe wartet
- [ ] Integriere mit GroupFormation:
  - Tank vorne, Healer hinten, Ranged hinter Melee
  - Auto-Adjust bei LoS-Pull
- [ ] M+ Count-Tracking: Tracke % des M+ Mob-Counts pro Pull

#### S4.3 — BossEncounterManager Enhancement (Tag 3-4)

**Modifizierte Dateien**:
```
MODIFY: AI/Coordination/Dungeon/BossEncounterManager.cpp/h
```

**Implementierung**:
- [ ] GAP 4 Partial Fix: BossAbilityDatabase für Dungeon-Bosse
  ```cpp
  struct BossAbility {
      uint32 spellId;
      BossAbilityType type;        // FRONTAL, CIRCLE_AOE, TANK_BUSTER, INTERRUPT_REQUIRED, SOAK, SPREAD
      float dangerLevel;           // 0.0 - 1.0
      float castTime;              // Sekunden (0 = instant)
      float warningTime;           // Sekunden vor Impact
      std::vector<BotReaction> reactions; // Was Bots tun sollen
  };
  
  enum class BotReaction {
      DODGE_FRONTAL,       // Aus dem Frontal-Cone raus
      SPREAD,              // Minimum X Yards Abstand
      STACK,               // Zusammenstehen zum Soaken
      INTERRUPT,           // Cast unterbrechen
      USE_DEFENSIVE,       // Persönliches Defensive-CD
      MOVE_TO_MARKER,      // Zu markierter Position
      STOP_CASTING,        // Cast abbrechen und bewegen
      DISPEL_SELF,         // Sich selbst dispellen
      SWITCH_TARGET,       // Target wechseln (Adds)
  };
  ```
- [ ] Implementiere Phase-Tracking pro Boss
- [ ] Implementiere Pre-Pull-Sequenz via BotMessageBus CMD_BLOODLUST Timing

#### S4.4 — MythicPlusManager: Affix-Erweiterung (GAP 5 Fix) (Tag 4-6)

**Modifizierte Dateien**:
```
MODIFY: AI/Coordination/Dungeon/MythicPlusManager.cpp/h
```

**Bestehende Affixe** (bereits implementiert):
- ✅ Sanguine — Aus Pool rausbewegen
- ✅ Quaking — Spreaden
- ✅ Explosive — Orbs targetten
- ✅ Necrotic — Kiting

**Neue Affix-Handler**:
- [ ] **Incorporeal** — Ranged DPS/Healer targetten und CC'en (Fear, Hex, etc.)
- [ ] **Afflicted** — Healer dispellen/heilen die Afflicted Spirits sofort
- [ ] **Entangling** — Entangle-Vines dispellen oder destroyen
- [ ] **Bursting** — Healer speichern Mana, spreaden Kills über Zeit
- [ ] **Spiteful** — Ranged DPS kiten, Melee bewegen sich weg
- [ ] **Storming** — Aus Tornado-Path rausbewegen
- [ ] **Volcanic** — Aus Volcanic-Circles rausbewegen
- [ ] **Raging** — Enrage bei 30% → Soothe (Druid, Hunter) oder defensives CD

**Pro Affix-Handler**:
```cpp
class IAffixHandler {
public:
    virtual uint32 GetAffixId() const = 0;
    virtual void OnAffixActive(ContentContext& ctx) = 0;   // Wenn Affix aktiv wird
    virtual void OnAffixTrigger(Unit* source) = 0;         // Wenn Affix triggered
    virtual void OnUpdate(uint32 diff) = 0;                 // Tick-basiert
    virtual AffixPriority GetPriority() const = 0;          // Wie dringend
};
```

#### S4.5 — WipeRecoveryManager Enhancement (Tag 6-7)

**Modifizierte Dateien**:
```
MODIFY: AI/Coordination/Dungeon/WipeRecoveryManager.cpp/h
```

**Implementierung**:
- [ ] Wipe-Detection: Alle Gruppenmitglieder tot
- [ ] Recovery-Sequenz:
  1. Soulstone/Ankh Check — hat jemand einen?
  2. Wenn ja: Ress → Ress andere → Buff → Ready
  3. Wenn nein: Release → Lauf zurück → Buff → Ready
- [ ] M+-spezifisch: Timer-Awareness bei Wipe-Recovery (ist es noch lohnenswert?)


---

### Sprint 5: Raid Coordination (5-7 Tage)

**Ziel**: Vollständige Raid-Koordination mit Tank-Swap, Heal-Assignment, CD-Rotation, und Boss-Mechanics.

#### S5.1 — RaidCoordinator Überarbeitung (Tag 1-2)

**Modifizierte Dateien**:
```
MODIFY: AI/Coordination/Raid/RaidCoordinator.cpp/h
MODIFY: AI/Coordination/Raid/RaidState.h
```

**Implementierung**:
- [ ] Integriere ContentContextManager für Raid-Difficulty-Awareness
- [ ] Integriere BotMessageBus für Raid-weite Kommunikation
- [ ] Implementiere Raid-Assignment-System:
  - Sub-Groups (bis zu 8 Gruppen à 5)
  - Healing-Assignments (Gruppe 1-4 → Healer A, Gruppe 5-8 → Healer B)
  - Tank-Assignments (Main Tank, Off-Tank, Add-Tank)
  - Interrupt-Rotation-Assignments pro Boss
- [ ] Implementiere RaidLeaderAI:
  - Automatische Rolle des Raid-Leaders (erster Bot oder designierter)
  - Gibt CMD_* Messages über BotMessageBus
  - Entscheidet über Pull-Timing, Bloodlust-Timing, Wipe-Calls

#### S5.2 — RaidTankCoordinator Enhancement (Tag 2-3)

**Modifizierte Dateien**:
```
MODIFY: AI/Coordination/Raid/RaidTankCoordinator.cpp/h
```

**Implementierung**:
- [ ] Tank-Swap-Mechanik:
  1. Boss-Debuff-Stack monitoren (über AuraEventBus)
  2. Bei Threshold: REQUEST_TANK_SWAP über BotMessageBus
  3. Off-Tank Taunt → CLAIM_INTERRUPT (Taunt)
  4. Main-Tank bewegt sich zur Off-Tank-Position
  5. Rollen tauschen
- [ ] Implementiere Taunt-Resist-Handling: Retry-Logik
- [ ] Add-Tank-Behaviour:
  - Erkennung neuer Adds über CombatEventBus
  - Priorisierung: Caster-Adds > Melee-Adds
  - Positioning: Adds von Raid weghalten

#### S5.3 — RaidHealCoordinator Enhancement (Tag 3-4)

**Modifizierte Dateien**:
```
MODIFY: AI/Coordination/Raid/RaidHealCoordinator.cpp/h
```

**Implementierung**:
- [ ] Heal-Assignment-System:
  - Tank-Healer (dedicated Tank-Healing)
  - Raid-Healer (AoE-Healing)
  - Spot-Healer (Einzelziel-Healing für kritische Targets)
- [ ] Heal-CD-Rotation:
  - Tracke alle Heal-CDs über CooldownEventBus
  - Rotation: Healing Tide → Revival → Tranquility → Divine Hymn
  - Automatische Zuweisung basierend auf Damage-Pattern
- [ ] Mana-Management:
  - Innervate-Zuweisung zum OOM-sten Healer
  - Mana-Potion-Timing
  - Shadowfiend/Mana-Tide Timing
- [ ] Integriere BotMessageBus:
  - REQUEST_HEAL mit Urgency für priorisierte Heilung
  - ANNOUNCE_LOW_RESOURCE für Mana-Warnungen
  - CLAIM_DEFENSIVE_CD für externe Heal-CDs

#### S5.4 — RaidCooldownRotation Enhancement (Tag 4-5)

**Modifizierte Dateien**:
```
MODIFY: AI/Coordination/Raid/RaidCooldownRotation.cpp/h
```

**Implementierung**:
- [ ] Integriere CooldownEventBus (GAP 1 Fix):
  - MAJOR_CD_USED → Aus Rotation entfernen, Timer starten
  - MAJOR_CD_AVAILABLE → Wieder in Rotation aufnehmen
- [ ] Implementiere CD-Rotation-Planer:
  ```cpp
  struct CDRotationEntry {
      ObjectGuid botGuid;
      uint32 spellId;
      ExternalCDTier tier;
      uint32 cooldownRemaining;
      bool isAvailable;
  };
  
  // Beispiel-Rotation für einen 5-Min-Kampf:
  // 0:00 → Rallying Cry (Warrior)
  // 0:30 → Spirit Link (Shaman) 
  // 1:00 → Darkness (DH)
  // 1:30 → Anti-Magic Zone (DK)
  // 2:00 → Rallying Cry wieder verfügbar
  ```
- [ ] Proactive CD-Assignment: Vor bekannter Boss-Ability das passende CD zuweisen
- [ ] Reactive CD-Assignment: Bei unerwartetem Damage schnellstes verfügbares CD nutzen

#### S5.5 — RaidEncounterManager + BossAbilityDatabase (GAP 4) (Tag 5-6)

**Modifizierte/Neue Dateien**:
```
MODIFY: AI/Coordination/Raid/RaidEncounterManager.cpp/h
NEW: AI/Coordination/BossAbilityDatabase.cpp/h
```

**Implementierung**:
- [ ] BossAbilityDatabase:
  - Speichert BossAbility Structs pro Boss-Entry + Phase
  - Ladbar aus Konfigurationsdatei oder hardcoded
  - Erweiterbar für neue Encounters
- [ ] Boss-Ability-Reaktionskette:
  ```
  SPELL_CAST_START (Combat Log) 
    → BossAbilityDatabase Lookup 
    → BotReaction[] 
    → BotMessageBus CMD_* 
    → Individuelle Bot-Aktionen
  ```
- [ ] Implementiere Encounter-Phases:
  - Phase-Transition-Detection (Boss HP %, Timer, Mechanic)
  - Phase-spezifische Strategien (Positioning, Targets, CD-Usage)

#### S5.6 — AddManagementSystem Enhancement (Tag 6-7)

**Modifizierte Dateien**:
```
MODIFY: AI/Coordination/Raid/AddManagementSystem.cpp/h
```

**Implementierung**:
- [ ] Add-Priority-System:
  ```
  Priority = basePriority
    + (isCaster ? 50 : 0)           // Caster first
    + (isHealer ? 100 : 0)          // Healer adds highest
    + (isEnraged ? 30 : 0)          // Enraged adds
    + (healthPct < 20 ? 40 : 0)     // Almost dead, finish off
    + (distToRaid < 10 ? 20 : 0)    // Close to raid, dangerous
  ```
- [ ] Add-Assignment über BotMessageBus:
  - CMD_FOCUS_TARGET für Add-Switches
  - Dedizierte Add-DPS-Zuweisung (Ranged > Melee für weit entfernte Adds)
- [ ] Cleave/Funnel-Erkennung: Sind Adds nah genug für Cleave-Damage?


---

### Sprint 6: PvP Coordination — Arena & Battleground (5-7 Tage)

**Ziel**: Koordinierte Arena-Teams und strategische BG-Teilnahme.

#### S6.1 — ArenaCoordinator Enhancement (Tag 1-2)

**Modifizierte Dateien**:
```
MODIFY: AI/Coordination/Arena/ArenaCoordinator.cpp/h
MODIFY: AI/Coordination/Arena/ArenaState.h
```

**Implementierung**:
- [ ] Integriere ContentContextManager (ARENA_2V2 vs ARENA_3V3)
- [ ] Integriere BotMessageBus für Arena-Kommunikation
- [ ] Implementiere Arena-Phase-Machine:
  ```
  GATES_CLOSED → OPENING → EVALUATING → ENGAGING → SWITCHING → DEFENSIVE → RESET → WIN/LOSE
  ```
- [ ] Implementiere Comp-Awareness:
  - Erkennung feindlicher Klassen/Specs in der Arena-Vorbereitungszeit
  - Strategie-Auswahl basierend auf eigener Comp vs. feindlicher Comp
  - Target-Priority basierend auf Comp (z.B. gegen Healer: Mage-Kill oder Healer-Kill?)

#### S6.2 — BurstCoordinator Enhancement (Tag 2-3)

**Modifizierte Dateien**:
```
MODIFY: AI/Coordination/Arena/BurstCoordinator.cpp/h
```

**Implementierung**:
- [ ] Integriere CooldownEventBus für Burst-CD-Tracking
- [ ] Burst-Window-Koordination:
  1. Alle DPS melden ihre Burst-CDs via CooldownEventBus
  2. BurstCoordinator identifiziert optimalen Burst-Zeitpunkt
  3. ANNOUNCE_BURST_WINDOW über BotMessageBus
  4. CMD_FOCUS_TARGET auf Kill-Target
  5. Alle DPS poppen CDs gleichzeitig
- [ ] Burst-Counter-Detection:
  - Feindlicher Burst erkannt → CMD_USE_DEFENSIVES
  - Feindliche Trinkets/CDs getrackt
- [ ] Mana-Awareness: Burst wenn feindlicher Healer OOM

#### S6.3 — CCChainManager Enhancement (Tag 3-4)

**Modifizierte Dateien**:
```
MODIFY: AI/Coordination/Arena/CCChainManager.cpp/h
```

**Implementierung**:
- [ ] CC-Chain-Planung:
  ```cpp
  struct CCChain {
      ObjectGuid targetGuid;
      std::vector<CCStep> steps;
      // Beispiel: Polymorph (8s) → Sheep breaks → Dragon's Breath (4s) → Ring of Frost (8s)
  };
  
  struct CCStep {
      ObjectGuid casterGuid;
      uint32 spellId;
      float duration;
      DRCategory drCategory;
      uint8 drStack;           // 1 = full, 2 = half, 3 = quarter, 4 = immune
  };
  ```
- [ ] DR-Tracking pro Target pro CC-Kategorie
- [ ] CC-Chain-Execution via BotMessageBus:
  - Step 1 Bot sendet ANNOUNCE_CC
  - Wenn CC bricht oder abläuft → Nächster Bot in Chain
  - CLAIM_CC vor jeder CC-Application
- [ ] Cross-CC-Awareness: Healer CC'en während DPS bursten

#### S6.4 — DefensiveCoordinator (Arena) Enhancement (Tag 4-5)

**Modifizierte Dateien**:
```
MODIFY: AI/Coordination/Arena/DefensiveCoordinator.cpp/h
```

**Implementierung**:
- [ ] Arena-spezifische Defensive-Logik:
  - Trinket-Usage-Tracking (eigene + feindliche)
  - "Trinket this CC?" Entscheidungslogik (nur trinken wenn sonst Tod droht)
  - Peeling: Wenn Partner focused wird, CC/Stun auf Angreifer
- [ ] Kiting-Coordination:
  - Wenn unter Druck: Pillar-Hug + LoS brechen
  - Partner heilt während Kiting
- [ ] Implementiere über BotMessageBus:
  - REQUEST_EXTERNAL_CD wenn unter Kill-Pressure
  - ANNOUNCE_CD_USAGE (Trinket, Major CD)
  - REQUEST_RESCUE (CC auf Angreifer)

#### S6.5 — BattlegroundCoordinator Enhancement (Tag 5-6)

**Modifizierte Dateien**:
```
MODIFY: AI/Coordination/Battleground/BattlegroundCoordinator.cpp/h
MODIFY: AI/Coordination/Battleground/BGStrategyEngine.cpp/h
```

**Implementierung**:
- [ ] BG-Strategie-Engine:
  - WSG/TP: Flag-Offense + Flag-Defense + Midfield-Control
  - AB/BFG: Node-Control (3-Cap-Strategie)
  - EOTS: 3-Node + Flag-Carry
  - AV/IoC: Offense-Rush vs Balanced
  - SSM/DG: Mine-Cart/Payload-Escort
- [ ] Rollen-Zuweisung pro BG:
  - Flag Carrier (Tank-Spec bevorzugt)
  - Flag Defense (Stealther + Healer)
  - Node Assault (Burst-DPS-Gruppe)
  - Node Defense (CC-schwere Comp + Healer)
  - Roaming (Ganker, Anti-Roaming)
- [ ] Integriere BotMessageBus:
  - CMD_MOVE_TO für strategische Positionierung
  - CMD_FOCUS_TARGET für Target-Calls
  - ANNOUNCE_POSITION für Rotations

#### S6.6 — Objective & Node Management (Tag 6-7)

**Modifizierte Dateien**:
```
MODIFY: AI/Coordination/Battleground/ObjectiveManager.cpp/h
MODIFY: AI/Coordination/Battleground/NodeController.cpp/h
MODIFY: AI/Coordination/Battleground/FlagCarrierManager.cpp/h
```

**Implementierung**:
- [ ] Node-Threat-Assessment:
  - Wie viele Feinde bei Node?
  - Können wir gewinnen? (Numerische + Qualitative Bewertung)
  - Lohnt sich Verstärkung oder lieber anderswo angreifen?
- [ ] Flag-Carrier-Support:
  - Healer follows Flag-Carrier
  - DPS escorted bis Midfield
  - Speed-Boost-Coordination (Stampeding Roar, Windwalk Totem)
- [ ] Reinforcement-System:
  - Node unter Angriff → Verstärkung senden
  - Priorisierung: Welche Node ist wichtiger?
  - Backup-Plan: Wenn Node verloren → Nächste angreifen


---

## 6. Performance-Anforderungen

### 6.1 BotMessageBus Performance

| Metrik | Ziel |
|--------|------|
| Message-Durchsatz | 1000 msg/sec pro Gruppe |
| Claim-Resolution-Time | < 200ms |
| Memory pro Gruppe | < 1KB (Message-Buffer) |
| Overhead pro Bot pro Tick | < 10µs |

### 6.2 Coordination-Overhead

| Szenario | Max CPU pro Bot | Max Latenz |
|----------|----------------|------------|
| 5-Man Dungeon | 50µs/tick | 20ms |
| 25-Man Raid | 30µs/tick | 30ms |
| 3v3 Arena | 40µs/tick | 10ms |
| 40-Man BG | 20µs/tick | 50ms |

### 6.3 Skalierungsziele

| Bot-Anzahl | Max CPU Total | Hinweis |
|------------|--------------|---------|
| 100 Bots | < 10% | Normalbetrieb |
| 500 Bots | < 40% | Standard-Server |
| 1000 Bots | < 70% | Großer Server |
| 5000 Bots | Graceful Degradation | Coordination-Level automatisch reduzieren |

---

## 7. Test-Strategie

### 7.1 Unit Tests (pro Sprint)

| Sprint | Test-Dateien | Fokus |
|--------|-------------|-------|
| S2 | CooldownEventBusTest, BotMessageBusTest, ClaimResolverTest | Infrastruktur |
| S3 | InterruptRotationTest, DispelAssignmentTest, DefensiveCDTest | Combat Coordination |
| S4 | DungeonCoordinatorTest, AffixHandlerTest, TrashPullTest | Dungeon/M+ |
| S5 | RaidCDRotationTest, TankSwapTest, HealAssignmentTest | Raid |
| S6 | BurstCoordinationTest, CCChainTest, BGStrategyTest | PvP |

### 7.2 Integration Tests

- [ ] **5-Man Dungeon Run**: 1 Tank + 1 Healer + 3 DPS durch eine komplette Dungeon-Instanz
- [ ] **M+ Run**: Wie oben, aber mit aktiven Affixen
- [ ] **Raid Encounter**: 2 Tanks + 5 Healer + 18 DPS gegen einen Raid-Boss mit Tank-Swap-Mechanic
- [ ] **Arena 3v3**: Team A (Warrior/Mage/Priest) vs Team B (Rogue/Warlock/Druid)
- [ ] **BG Warsong Gulch**: 10v10 mit Flag-Carrier-Koordination

### 7.3 Performance Tests

- [ ] Message-Durchsatz-Benchmark: 5000 Bots, 100 Gruppen
- [ ] Claim-Resolution unter Last: 50 gleichzeitige Claims
- [ ] Memory-Footprint: Vor und nach P3 messen
- [ ] CPU-Profiling: Per-Tick-Overhead der neuen Systeme

---

## 8. Ausführungsanweisungen für Claude Code

### 8.1 Branch & Build

```bash
# Branch: playerbot-dev
cd C:\TrinityBots\TrinityCore
git checkout playerbot-dev

# Build nach jedem Sprint
cmake --build build --config RelWithDebInfo --target Playerbot
```

### 8.2 Sprint-Ausführungsreihenfolge

```
S1 (Audit) → S2 (Infrastruktur) → S3 (Combat) → S4/S5/S6 (parallel möglich)
```

### 8.3 Pro-Sprint-Workflow

1. **Lese diesen Plan** — Sektion für den aktuellen Sprint
2. **Lese EVENT_FLOW_MAP.md** — Für Event-Details und Publisher/Subscriber-Referenzen
3. **Lese existierende Dateien** — Die unter "Modifizierte Dateien" gelisteten
4. **Implementiere** — Folge den Checkboxen
5. **Teste** — Mindestens die gelisteten Unit Tests
6. **Build** — Stelle sicher dass es kompiliert
7. **Dokumentiere** — Update PHASE3_PROGRESS.md

### 8.4 Code-Konventionen

- **NIEMALS Core-Dateien modifizieren** — Nur `src/modules/Playerbot/`
- **IMMER TrinityCore APIs nutzen** — Keine Workarounds
- **Performance**: `constexpr` wo möglich, keine Heap-Allokationen in Hot Paths
- **Thread-Safety**: Alle geteilten Datenstrukturen mit Locks oder Lock-Free
- **Logging**: `TC_LOG_DEBUG("playerbot.coordination", ...)` für alle neuen Koordination-Messages
- **Naming**: PascalCase für Klassen, camelCase für Methoden, UPPER_SNAKE_CASE für Enums

### 8.5 Referenz-Dokumente

| Dokument | Pfad | Relevanz |
|----------|------|----------|
| EVENT_FLOW_MAP.md | `worktrees/event-flow-map-de55/docs/EVENT_FLOW_MAP.md` | Alle Events, Publisher, Subscriber |
| PHASE3_MASTER_PLAN.md | `.claude/PHASE3/PHASE3_MASTER_PLAN.md` | Übergeordneter P3-Plan |
| GROUP_INTEGRATION.md | `.claude/PHASE3/GROUP_INTEGRATION.md` | Bestehende Gruppen-Specs |
| COMBAT_BEHAVIOR_DESIGN.md | `AI/CombatBehaviors/COMBAT_BEHAVIOR_DESIGN.md` | CombatBehavior-Architektur |
| EventBusArchitecture.md | `docs/EventBusArchitecture.md` | Event Bus Design |

---

## 9. Risiken & Mitigationen

| Risiko | Impact | Mitigation |
|--------|--------|------------|
| Duplikat-Merge bricht existierenden Code | Hoch | S1 Audit erst, dann schrittweiser Merge mit Compat-Wrappern |
| BotMessageBus-Performance bei 5000 Bots | Hoch | Batching + Group-lokales Messaging + Graceful Degradation |
| CooldownEventBus Packet-Parsing fehlerhaft | Mittel | Vergleich mit Retail-Packet-Logs, Unit Tests |
| BossAbilityDatabase unvollständig | Mittel | Erweiterbar designen, Fallback auf generische Reaktionen |
| Arena-Koordination zu vorhersehbar | Niedrig | Randomization + Comp-basierte Varianz |

---

## 10. Erfolgskriterien

| Kriterium | Messung |
|-----------|---------|
| Alle 5 Gaps gefixed | CooldownEventBus aktiv, Dispel/Defensive-Rotation, Boss-Abilities, M+ Affixe |
| Duplikate aufgelöst | Maximal 1 kanonische Datei pro Funktion |
| BotMessageBus funktional | Claims resolven korrekt in < 200ms |
| 5-Man Dungeon läuft | Bots clearen eine Dungeon-Instanz ohne Spieler-Eingriff |
| Raid-Boss mit Tank-Swap | Bots handlen Tank-Swap-Mechanic korrekt |
| Arena 3v3 funktional | Bots koordinieren Burst + CC-Chains |
| BG WSG funktional | Bots capturen Flags und verteidigen |
| Performance-Targets erreicht | < 50µs/tick pro Bot für Coordination-Overhead |
| Build clean | 0 Warnings, 0 Errors |

---

*Erstellt: 2026-02-06*
*Basierend auf: EVENT_FLOW_MAP.md (188 Events), PHASE3_MASTER_PLAN.md, GROUP_INTEGRATION.md*
*Ausführung: Claude Code auf Branch playerbot-dev*
