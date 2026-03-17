# Claude Code Task: DI Interface Cleanup

## Ziel
Entferne das ungenutzte Dependency Injection System aus dem Playerbot-Modul.

## Kontext
- 90 DI Interface-Dateien wurden bereits gelöscht aus `Core/DI/`
- 96 Dateien haben noch Referenzen auf diese gelöschten Interfaces
- Die Interfaces wurden NIE tatsächlich für DI genutzt (keine Resolve<> Aufrufe)
- Die Klassen erben von den Interfaces, nutzen aber direkte Singletons

## Was zu tun ist

### Für jede Datei die einen Compile-Fehler hat:

**1. Include entfernen:**
```cpp
// LÖSCHEN:
#include "Core/DI/Interfaces/IBotSpawner.h"
```

**2. Vererbung entfernen:**
```cpp
// VORHER:
class BotSpawner final : public IBotSpawner

// NACHHER:
class BotSpawner final
```

**3. `override` Keywords entfernen:**
```cpp
// VORHER:
void SpawnBot() override;
bool Initialize() override;

// NACHHER:
void SpawnBot();
bool Initialize();
```

**4. Interface-Typ-Aliase durch direkte Structs ersetzen:**
```cpp
// VORHER:
using BotAccountInfo = IBotAccountMgr::BotAccountInfo;

// NACHHER (Struct direkt in Klasse definieren):
struct BotAccountInfo {
    uint32 bnetAccountId{0};
    // ... kopiere Felder aus dem alten Interface
};
```

## WICHTIG - Was NICHT ändern:

- ❌ Methoden-Implementierungen NICHT ändern
- ❌ Methoden-Signaturen NICHT ändern (außer `override` entfernen)
- ❌ Singleton-Pattern NICHT ändern (`sBotSpawner`, etc.)
- ❌ Funktionalität NICHT ändern

## Vorgehen

```bash
# 1. Kompilieren versuchen
cd C:\TrinityBots\TrinityCore\build
cmake --build . --target Playerbot 2>&1 | head -100

# 2. Ersten Fehler identifizieren (z.B. "cannot open include file")

# 3. Diese Datei fixen (Include, Vererbung, override entfernen)

# 4. Wiederholen bis keine Fehler mehr
```

## Beispiel-Transformation

### Vorher (BotSpawner.h):
```cpp
#pragma once
#include "Core/DI/Interfaces/IBotSpawner.h"  // ← LÖSCHEN

class BotSpawner final : public IBotSpawner  // ← : public IBotSpawner LÖSCHEN
{
public:
    void SpawnBot() override;                 // ← override LÖSCHEN
    bool Initialize() override;               // ← override LÖSCHEN
};
```

### Nachher (BotSpawner.h):
```cpp
#pragma once

class BotSpawner final
{
public:
    void SpawnBot();
    bool Initialize();
};
```

## Betroffene Dateien (96 Stück)

Diese Dateien haben Referenzen auf gelöschte DI Interfaces:

```
Account/BotAccountMgr.h                    ← BEREITS GEFIXT
Adapters/BotSpawnerAdapter.h
Advanced/GroupCoordinator.h
AI/Actions/Action.h
AI/ClassAI/PositionStrategyBase.h
AI/Combat/FormationManager.h
AI/Combat/PositionManager.h
AI/Combat/UnifiedInterruptSystem.h
AI/Coordination/RaidOrchestrator.h
AI/Strategy/QuestStrategy.cpp
AI/Strategy/SoloStrategy.cpp
AI/Strategy/Strategy.h
AI/Triggers/Trigger.h
AI/BotAI.h
Auction/AuctionEventBus.h
Aura/AuraEventBus.h
Character/BotCharacterDistribution.h
Character/BotLevelDistribution.h
Character/BotLevelManager.h
Character/BotNameMgr.h
Combat/CombatEventBus.h
Companion/BattlePetManager.h
Companion/MountManager.h
Companion/RidingManager.h
Config/ConfigManager.h
Config/PlayerbotConfig.h
Cooldown/CooldownEventBus.h
Core/Services/BotNpcLocationService.h
Database/BotDatabasePool.h
Database/PlayerbotCharacterDBInterface.h
Database/PlayerbotDatabase.h
Database/PlayerbotMigrationMgr.h
Diagnostics/DeadlockDetector.h
Dungeon/DungeonBehavior.h
Dungeon/DungeonScriptMgr.h
Dungeon/EncounterStrategy.h
Dungeon/InstanceCoordination.h
Economy/AuctionManager.h
Equipment/BotGearFactory.h
Equipment/EquipmentManager.h
Group/GroupEventBus.h
Group/PlayerbotGroupManager.h
Group/RoleAssignment.h
Instance/InstanceEventBus.h
LFG/LFGBotManager.h
LFG/LFGBotSelector.h
LFG/LFGGroupCoordinator.h
LFG/LFGRoleDetector.h
Lifecycle/BotLifecycleManager.h
Lifecycle/BotLifecycleMgr.h
Lifecycle/BotResourcePool.h
Lifecycle/BotScheduler.h
Lifecycle/BotSpawner.h
Lifecycle/BotSpawnEventBus.h
Lifecycle/BotWorldEntry.h
Loot/LootEventBus.h
Monitoring/BotMonitor.h
Movement/BotWorldPositioner.h
Movement/UnifiedMovementCoordinator.h
NPC/NPCEventBus.h
Professions/FarmingCoordinator.h
Professions/ProfessionAuctionBridge.h
Professions/ProfessionManager.h
PvP/ArenaAI.h
PvP/ArenaBotManager.h
PvP/BattlegroundAI.h
PvP/BGBotManager.h
PvP/PvPCombatAI.h
Quest/DynamicQuestSystem.h
Quest/ObjectiveTracker.h
Quest/QuestCompletion.h
Quest/QuestEventBus.h
Quest/QuestPickup.h
Quest/QuestTurnIn.h
Quest/QuestValidation.h
Quest/UnifiedQuestManager.h
Resource/ResourceEventBus.h
Session/BotHealthCheck.h
Session/BotPerformanceMonitor.h
Session/BotPriorityManager.h
Session/BotWorldSessionMgr.h
Social/AuctionHouse.h
Social/GuildBankManager.h
Social/GuildEventCoordinator.h
Social/GuildIntegration.h
Social/LootDistribution.h
Social/MarketAnalysis.h
Social/SocialEventBus.h
Social/TradeSystem.h
Social/UnifiedLootManager.h
Spatial/SpatialGridManager.h
Talents/BotTalentManager.h
Tests/Mocks/MockConfigManager.h
Tests/PerformanceBenchmark.h
Tests/SystemValidation.h
```

## Erfolgs-Kriterium

```bash
cmake --build . --target Playerbot
# Muss ohne Fehler durchlaufen
```

## Hinweise

- Manche Interfaces haben Nested Types (structs, enums) - diese müssen in die Klasse kopiert werden
- EventBus Klassen (z.B. AuctionEventBus.h) erben von GenericEventBus<T>, NICHT von DI Interfaces - prüfen!
- Wenn unsicher, schau dir die Implementierung (.cpp) an um zu verstehen was die Klasse braucht

## Projekt-Pfade

- Projekt Root: `C:\TrinityBots\TrinityCore`
- Playerbot Modul: `C:\TrinityBots\TrinityCore\src\modules\Playerbot`
- Build Ordner: `C:\TrinityBots\TrinityCore\build`
