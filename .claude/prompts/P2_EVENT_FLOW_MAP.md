# P2: Event-Flow-Map erstellen — ZenFlow Prompt

## Kontext

Du arbeitest am TrinityCore Playerbot-Modul (`C:\TrinityBots\TrinityCore\src\modules\Playerbot\`).
**P0** (12 Event Buses aktivieren, Memory Leak Fix) und **P1** (15 Wrapper-Klassen entfernt, 1750 LOC Boilerplate eliminiert) sind abgeschlossen.

**Aufgabe P2**: Erstelle eine vollständige **Event-Flow-Map** als Referenzdokumentation, die exakt dokumentiert:
- Welches TrinityCore-Event → welcher Detektionspfad → welcher Event Bus → welcher Handler/Manager

---

## Architektur-Überblick (Ist-Zustand nach P0/P1)

Das System hat **3 parallele Event-Detektionspfade**, die alle in den zentralen `EventDispatcher` münden:

```
┌─────────────────────────────────────────────────────────────────────┐
│                    TrinityCore Game Engine                          │
├──────────────┬────────────────────┬─────────────────────────────────┤
│  Pfad A:     │  Pfad B:           │  Pfad C:                       │
│  ScriptHooks │  PacketSniffer     │  Core Hooks (PlayerBotHooks.h) │
│  (53 Hooks)  │  (14 Parser)       │  (30+ static function ptrs)    │
└──────┬───────┴────────┬───────────┴──────────┬──────────────────────┘
       │                │                      │
       ▼                ▼                      ▼
  ┌─────────────────────────────────────────────────┐
  │         BotEvent + EventDispatcher               │
  │    (per-Bot Queue, Priority-basiert)             │
  └───────────────────┬─────────────────────────────┘
                      │ RouteEvent()
                      ▼
  ┌─────────────────────────────────────────────────┐
  │            IManagerBase::OnEvent()               │
  │   (Subscribe pro EventType zu Managern)          │
  └─────────────────────────────────────────────────┘
```

---

## Dateien die du lesen MUSST (lies sie vollständig!)

### Pfad A — Script Hooks (Primärer Event-Eingang)
- `src/modules/Playerbot/scripts/PlayerbotEventScripts.cpp` — 53 ScriptHook-Callbacks (PlayerScript, UnitScript, GroupScript, VehicleScript, ItemScript, QuestScript)
- `src/modules/Playerbot/scripts/PlayerbotWorldScript.cpp` — WorldScript (OnStartup, OnUpdate)
- `src/modules/Playerbot/scripts/PlayerbotBGScript.cpp` — Battleground-Hooks
- `src/modules/Playerbot/scripts/PlayerbotLFGScript.cpp` — LFG-Hooks
- `src/modules/Playerbot/scripts/PlayerbotArenaScript.cpp` — Arena-Hooks

### Pfad B — Packet Sniffer (Typed Packet Parser)
- `src/modules/Playerbot/Network/PlayerbotPacketSniffer.cpp` — Hauptklasse
- `src/modules/Playerbot/Network/PlayerbotPacketSniffer.h`
- `src/modules/Playerbot/Network/Parse*Packet_Typed.cpp` — 14 spezialisierte Parser:
  - `ParseAuctionPacket_Typed.cpp`
  - `ParseAuraPacket_Typed.cpp`
  - `ParseBattlegroundPacket_Typed.cpp`
  - `ParseCombatPacket_Typed.cpp`
  - `ParseCooldownPacket_Typed.cpp`
  - `ParseGroupPacket_Typed.cpp` + `ParseGroupPacket_v2.cpp`
  - `ParseInstancePacket_Typed.cpp`
  - `ParseLFGPacket_Typed.cpp`
  - `ParseLootPacket_Typed.cpp`
  - `ParseNPCPacket_Typed.cpp`
  - `ParseQuestPacket_Typed.cpp`
  - `ParseResourcePacket_Typed.cpp`
  - `ParseSocialPacket_Typed.cpp`

### Pfad C — Core Hooks (direkte TrinityCore-Modifikation)
- `src/modules/Playerbot/Core/PlayerBotHooks.h` — 30+ `static std::function<>` Hook-Pointer
- `src/modules/Playerbot/Core/PlayerBotHooks.cpp` — Registration/Initialization

### Event-Typen & Bus-System
- `src/modules/Playerbot/Core/StateMachine/BotStateTypes.h` — `enum class EventType` (150+ Event-Typen)
- `src/modules/Playerbot/Core/Events/BotEventTypes.h` — `struct BotEvent`, Priority, Callbacks
- `src/modules/Playerbot/Events/BotEventData.h` — 40+ spezialisierte `EventDataVariant`-Structs
- `src/modules/Playerbot/Core/Events/EventDispatcher.h/.cpp` — Zentraler Dispatcher (Subscribe/Dispatch/ProcessQueue)
- `src/modules/Playerbot/Core/Events/GenericEventBus.h` — Generischer Bus

### Die 12 Event Buses (P0)
- `Combat/CombatEventBus.h`
- `Group/GroupEventBus.h` (oder via GroupScript)
- `Quest/QuestEventBus.h`
- `Aura/AuraEventBus.h`
- `Cooldown/CooldownEventBus.h`
- `Loot/LootEventBus.h`
- `Resource/ResourceEventBus.h`
- `Social/SocialEventBus.h`
- `Auction/AuctionEventBus.h`
- `NPC/NPCEventBus.h`
- `Instance/InstanceEventBus.h`
- `Lifecycle/BotSpawnEventBus.h`
- `AI/Combat/HostileEventBus.h`

### Manager (Event-Konsumenten)
Suche in diesen Verzeichnissen nach Klassen die `IManagerBase` implementieren und `OnEvent()` overriden:
- `src/modules/Playerbot/AI/` — BotAI, ClassAI
- `src/modules/Playerbot/Combat/` — CombatManager
- `src/modules/Playerbot/Movement/` — MovementArbiter
- `src/modules/Playerbot/Quest/` — QuestManager
- `src/modules/Playerbot/Group/` — PlayerbotGroupManager
- `src/modules/Playerbot/Loot/` — LootManager
- `src/modules/Playerbot/Equipment/` — EquipmentManager
- `src/modules/Playerbot/Session/` — BotSession, BotPriorityManager

### Bestehende Doku (zum Abgleich, aber NICHT blindlings kopieren — verifiziere gegen Code!)
- `src/modules/Playerbot/docs/EventBusArchitecture.md`
- `src/modules/Playerbot/PHASE4_EVENT_BUS_TEMPLATE.md`
- `src/modules/Playerbot/PHASE4_EVENT_INTEGRATION_PLAN.md`

---

## Dein Auftrag: Event-Flow-Map

Erstelle die Datei `src/modules/Playerbot/docs/EVENT_FLOW_MAP.md` mit folgendem Aufbau:

### 1. Master-Tabelle (Pflicht!)

Eine vollständige Tabelle mit JEDER Event-Kette. Format:

```markdown
| TC Source Event | Detektionspfad | EventType (Enum) | Priority | Bus/Dispatcher | Handler/Manager | Status |
|---|---|---|---|---|---|---|
| Unit::DealDamage() | Pfad A: PlayerbotUnitScript::OnDamage | DAMAGE_TAKEN (70) | 180 | EventDispatcher | BotAI→CombatManager | ✅ Aktiv |
| Unit::DealDamage() | Pfad A: PlayerbotUnitScript::OnDamage | DAMAGE_DEALT (71) | 100 | EventDispatcher | BotAI→CombatManager | ✅ Aktiv |
| Unit::DealDamage() | Pfad A: PlayerbotUnitScript::OnDamage | GROUP_MEMBER_ATTACKED (43) | 190 | EventDispatcher | BotAI (Gruppen-Bots) | ✅ Aktiv |
| Unit::HealBySpell() | Pfad A: PlayerbotUnitScript::OnHeal | HEAL_RECEIVED (72) | 120 | EventDispatcher | BotAI | ✅ Aktiv |
| Group::AddMember() | Pfad C: PlayerBotHooks::OnGroupMemberAdded | MEMBER_JOINED (39) | 200 | EventDispatcher | GroupManager | ✅ Aktiv |
| SMSG_SPELL_START | Pfad B: ParseCombatPacket_Typed | SPELL_CAST_START (73) | HIGH | CombatEventBus | ClassAI→InterruptCoordinator | ⚠️ Prüfen |
| ... | ... | ... | ... | ... | ... | ... |
```

**Status-Legende:**
- ✅ Aktiv — Event wird gesendet UND von mindestens einem Handler konsumiert
- ⚠️ Prüfen — Event wird gesendet, aber kein Handler gefunden (oder Bus nicht connected)
- ❌ Tot — EventType existiert in Enum, aber kein Sender publiziert ihn
- 🔧 Stub — Handler existiert, aber Logik ist TODO/leer

### 2. Detektionspfad-Analyse

Für jeden der 3 Pfade:
- Liste ALLE Hooks/Parser auf die tatsächlich Events erzeugen
- Dokumentiere welche `BotEvent`-Konstruktoren aufgerufen werden
- Notiere wo `DispatchToBotEventDispatcher()` aufgerufen wird (Pfad A) vs. wo Events direkt auf Buses publiziert werden (Pfad B/C)

### 3. Dead-Code-Analyse (WICHTIG für P3!)

Identifiziere:
- **Tote EventTypes**: Events in `BotStateTypes.h::EventType` die NIRGENDWO dispatched werden
- **Verwaiste Subscriber**: Manager die auf Events subscriben die nie gesendet werden
- **Doppelte Pfade**: Dasselbe TC-Event wird über 2+ Pfade detektiert (z.B. Spell Cast über ScriptHook UND PacketSniffer) — potentielle Duplikate
- **Fehlende Verbindungen**: PacketSniffer-Parser die Events erzeugen, aber kein Bus/Dispatcher sie weiterleitet

### 4. Event-Flow-Diagramme (Mermaid)

Erstelle Mermaid-Diagramme für die 5 kritischsten Flows:
1. **Combat Damage Flow** (Schaden → Reaktion → Gegenschlag)
2. **Spell Interrupt Flow** (Enemy Cast → Detect → Interrupt)
3. **Group Assist Flow** (Member attacked → alle Bots reagieren)
4. **Death Recovery Flow** (Tod → Spirit Release → Corpse Run → Resurrect)
5. **Quest Progress Flow** (Accept → Objectives → Complete → Turn In)

### 5. Bus-Nutzungs-Matrix

```markdown
| Event Bus | Events/sec (geschätzt) | Publisher (Quelle) | Subscriber (Konsument) | ProcessEvents im Loop? |
|---|---|---|---|---|
| CombatEventBus | 150k | ParseCombatPacket_Typed | CombatManager, ClassAI | ✅ Ja (P0) |
| GroupEventBus | 500 | PlayerbotGroupScript, PlayerBotHooks | GroupManager | ✅ Ja (P0) |
| ...
```

### 6. Empfehlungen für P3 (Cross-Bot Events)

Basierend auf deiner Analyse:
- Welche Events eignen sich für Cross-Bot-Koordination?
- Wo fehlen Events für Group Coordination (z.B. "Bot A sagt Bot B: ich interrute, du kannst weiter DPS machen")?
- Welche bestehenden Event-Duplikate können für P3 konsolidiert werden?

---

## Regeln

1. **Verifiziere ALLES gegen den tatsächlichen Code** — die bestehende Doku (`EventBusArchitecture.md`) wurde teilweise vor P0/P1 geschrieben und kann veraltet sein
2. **Lies jeden Parser/Script vollständig** bevor du die Tabelle füllst
3. **Suche nach `Dispatch`, `Publish`, `Subscribe`, `OnEvent`** in der gesamten Playerbot-Codebase um keine Events zu übersehen
4. **Markiere Unsicherheiten** — wenn du dir nicht sicher bist ob ein Pfad aktiv ist, markiere ihn als ⚠️ nicht als ✅
5. **Zähle die tatsächlichen Events** — die Master-Tabelle sollte 100+ Zeilen haben (150+ EventTypes × 3 Pfade, abzüglich inaktiver)

## Ergebnis-Datei

```
src/modules/Playerbot/docs/EVENT_FLOW_MAP.md
```

Dateigröße: Erwarte 800-1500 Zeilen. Das ist absichtlich umfangreich — dies ist DIE zentrale Referenz für P3 Cross-Bot Events.
