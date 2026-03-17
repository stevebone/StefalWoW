# Quest/Leveling & Humanization System Analysis

**Date**: 2026-01-27  
**Focus**: Menschlicheres Bot-Verhalten ("Humanization")  
**Priority**: HIGH - Kritisch für Anti-Detection & User Experience

---

## Executive Summary

Das System hat solide Grundlagen, aber **Bots verhalten sich wie Roboter**:
- ✅ Quest System: 93.8% implementiert (74 Stubs)
- ✅ Gathering: Funktioniert
- ✅ Professions: Cooking, Fishing existieren
- ✅ Idle Behavior: Basis vorhanden
- ❌ **KRITISCH: Keine "menschliche" Verhaltenslogik**

### Hauptproblem

Bots spielen **zu perfekt und zu schnell**:
- Questen non-stop ohne Pause
- Gathering 24/7 optimal
- Keine natürlichen Aktivitätswechsel
- Keine "Downtime" in Städten
- Keine Session-basierte Aktivitäten

---

## Teil 1: Bestehende Systeme

### 1.1 Quest System

**Status**: 93.8% implementiert (74 Stubs bleiben)

| Komponente | Status | Qualität |
|------------|--------|----------|
| QuestPickup | 95.4% | ⭐⭐⭐⭐ |
| QuestCompletion | 84.7% | ⭐⭐⭐ |
| QuestTurnIn | 97.6% | ⭐⭐⭐⭐⭐ |
| QuestValidation | 94.9% | ⭐⭐⭐⭐ |
| DynamicQuestSystem | 90.4% | ⭐⭐⭐⭐ |
| ObjectiveTracker | 97.1% | ⭐⭐⭐⭐⭐ |

**Fehlend für Humanization**:
- Keine Quest-Pausen (z.B. nach 5 Quests → Stadt)
- Keine "Langeweile"-Simulation (Quest abbrechen, andere anfangen)
- Keine ineffizienten Routen (Menschen machen Fehler)

### 1.2 Profession/Gathering System

**Status**: Funktioniert, aber roboterhaft

| Komponente | Status | Humanization |
|------------|--------|--------------|
| ProfessionManager | ✅ | ❌ Keine Sessions |
| GatheringManager | ✅ | ❌ Keine Zeitdauer |
| Cooking | ✅ | ❌ Keine Sessions |
| Fishing | ✅ | ❌ Keine Sessions |

**Fehlend**:
- Gathering-Sessions (30-60 Min am Stück)
- Cooking nach dem Sammeln
- Fishing als "Hobby" (30+ Min an einem Spot)

### 1.3 Idle Behavior System

**Status**: Basis vorhanden, aber limitiert

**Vorhandene Contexts**:
```cpp
enum class IdleContext : uint8
{
    NONE,           // Kein Idle
    TOWN_IDLE,      // Stadt-Wandern
    GROUP_WAIT,     // Auf Gruppe warten
    QUEST_WAIT,     // Quest-Wartezeit
    COMBAT_READY,   // Kampfbereit
    FISHING,        // Angeln
    GUARD_PATROL,   // Patrouille
    INSTANCE_WAIT,  // Instanz-Warten
    REST_AREA       // Gasthof (kann sitzen)
};
```

**Fehlend**:
- AH_BROWSING (Am Auktionshaus stehen)
- MAILBOX_CHECK (Post checken)
- TRAINER_VISIT (Trainer besuchen)
- BANK_VISIT (Bank besuchen)
- SOCIALIZING (Mit anderen Bots/Spielern interagieren)
- AFK_SIMULATION (Kurze Pausen)

---

## Teil 2: Humanization Gap Analysis

### 2.1 KRITISCH: Activity Session System

**Problem**: Bots wechseln Aktivitäten sofort und optimal.

**Menschliches Verhalten**:
```
Mensch: Gathering 45 Min → Stadt 15 Min → Quest 1h → Pause 5 Min → Dungeon
Bot:    Gathering 2 Min → Quest → Gathering → Quest → Quest → Quest...
```

**Lösung**: Activity Session Manager

```cpp
struct ActivitySession
{
    ActivityType type;          // QUESTING, GATHERING, CITY_LIFE, FISHING, etc.
    Milliseconds minDuration;   // Mindestdauer (z.B. 30 Min)
    Milliseconds maxDuration;   // Maximaldauer (z.B. 90 Min)
    Milliseconds currentTime;   // Aktuelle Zeit in Session
    float completionChance;     // Chance, früher aufzuhören (Langeweile)
};
```

### 2.2 KRITISCH: City Life Simulation

**Problem**: Bots gehen nur in Städte für Quests/Vendoren.

**Menschliches Verhalten**:
- 10-30 Min am AH stehen (browsen, Preise checken)
- 5-10 Min bei Mailbox
- In Gasthäusern sitzen (Rested XP)
- Trainer besuchen (auch ohne neue Skills)
- Mit anderen Spielern "interagieren" (Emotes)

**Lösung**: CityLifeBehaviorManager

```cpp
enum class CityActivity : uint8
{
    AH_BROWSING,        // Am AH stehen, Preise checken
    MAILBOX_CHECK,      // Post checken
    BANK_VISIT,         // Bank besuchen
    TRAINER_VISIT,      // Trainer besuchen (auch ohne Skills)
    INN_REST,           // Im Gasthof sitzen
    WANDERING,          // In der Stadt rumlaufen
    SOCIALIZING,        // Emotes, "chatten"
    VENDOR_SHOPPING,    // Vendoren ansehen
    PROFESSION_TRAINER  // Berufs-Trainer besuchen
};
```

### 2.3 KRITISCH: Session-basiertes Gathering

**Problem**: Bots sammeln nur im Vorbeigehen.

**Menschliches Verhalten**:
- Dedizierte Farming-Sessions (30-60+ Min)
- In einer Zone bleiben
- Route wiederholen
- Pausen einlegen

**Lösung**: GatheringSessionManager

```cpp
struct GatheringSession
{
    uint32 zoneId;
    Milliseconds duration;           // Geplante Dauer
    Milliseconds elapsed;            // Verstrichene Zeit
    uint32 nodesGathered;
    Position startPosition;
    bool dedicated;                  // Dedizierte Session vs. nebenbei
    
    // Humanization
    float pauseChance;               // Chance für Pause
    Milliseconds nextPauseAt;        // Nächste Pause
    Milliseconds pauseDuration;      // Pause-Dauer (1-5 Min)
};
```

### 2.4 HOCH: Fishing als Hobby

**Problem**: Fishing nur für Quests/Skill.

**Menschliches Verhalten**:
- Angeln als "Entspannung" (30-60 Min)
- An einem Spot bleiben
- Idle-Animationen
- Manchmal aufstehen, wieder hinsetzen

**Lösung**: FishingSessionManager

```cpp
struct FishingSession
{
    Position fishingSpot;
    Milliseconds duration;           // 30-90 Min
    Milliseconds elapsed;
    uint32 fishCaught;
    
    // Humanization
    bool isSitting;                  // Manchmal sitzen
    Milliseconds nextStandUp;        // Aufstehen/Hinsetzen
    float emoteChance;               // Chance für Emote (yawn, stretch)
};
```

### 2.5 HOCH: AFK/Pause Simulation

**Problem**: Bots sind 24/7 aktiv ohne Pause.

**Menschliches Verhalten**:
- Kurze AFK (1-5 Min) alle 30-60 Min
- Längere Pausen (10-30 Min) alle paar Stunden
- "Bio break" Simulation

**Lösung**: AFKSimulator

```cpp
struct AFKProfile
{
    Milliseconds minTimeBetweenAFK;  // Min Zeit zwischen AFKs
    Milliseconds maxTimeBetweenAFK;  // Max Zeit zwischen AFKs
    Milliseconds minAFKDuration;     // Min AFK-Dauer
    Milliseconds maxAFKDuration;     // Max AFK-Dauer
    float afkChance;                 // Basis-Chance für AFK
    
    // Während AFK
    bool canSitDown;                 // Hinsetzen während AFK
    bool canUseEmotes;               // /afk, /yawn, etc.
    bool stayInSafeArea;             // In sicherer Zone bleiben
};
```

### 2.6 MEDIUM: Tageszeit-Verhalten

**Problem**: Bots sind 24/7 gleich aktiv.

**Menschliches Verhalten**:
- Nachts weniger aktiv oder offline
- Morgens/Abends Peak-Aktivität
- Wochenende anders als Wochentage

**Lösung**: ActivityScheduler

```cpp
struct DailySchedule
{
    // Aktivitätslevel pro Stunde (0.0 = offline, 1.0 = voll aktiv)
    std::array<float, 24> hourlyActivityLevel;
    
    // Bevorzugte Aktivitäten zu bestimmten Zeiten
    std::map<uint8, std::vector<ActivityType>> preferredActivities;
    
    // Schlafsimulation
    uint8 sleepStartHour;            // z.B. 23:00
    uint8 sleepEndHour;              // z.B. 07:00
    bool sleepInInn;                 // In Gasthof "schlafen"
    bool logoutForSleep;             // Oder ausloggen
};
```

### 2.7 MEDIUM: Variabilität & "Fehler"

**Problem**: Bots spielen perfekt.

**Menschliches Verhalten**:
- Manchmal falsche Route nehmen
- Quest vergessen und zurücklaufen
- Suboptimale Spell-Rotation
- Items vergessen zu verkaufen

**Lösung**: HumanErrorSimulator

```cpp
struct HumanErrorProfile
{
    float wrongPathChance;           // Chance für falsche Route
    float forgotQuestChance;         // Quest vergessen
    float suboptimalRotationChance;  // Suboptimale Rotation
    float forgotVendorChance;        // Vergessen zu verkaufen
    float distractionChance;         // Ablenkung (kurz stehenbleiben)
    
    // Severity
    float errorRecoveryTime;         // Wie schnell Fehler korrigiert wird
};
```

---

## Teil 3: Vorgeschlagene Architektur

### 3.1 Neue Komponenten

```
src/modules/Playerbot/Humanization/
├── Core/
│   ├── HumanizationManager.h/cpp        # Zentrale Koordination
│   ├── HumanizationConfig.h/cpp         # Konfiguration
│   └── PersonalityProfile.h/cpp         # Bot-"Persönlichkeit"
│
├── Sessions/
│   ├── ActivitySessionManager.h/cpp     # Session-Verwaltung
│   ├── GatheringSessionManager.h/cpp    # Gathering-Sessions
│   ├── FishingSessionManager.h/cpp      # Fishing-Sessions
│   ├── QuestingSessionManager.h/cpp     # Questing-Sessions
│   └── CookingSessionManager.h/cpp      # Cooking-Sessions
│
├── CityLife/
│   ├── CityLifeBehaviorManager.h/cpp    # Stadt-Verhalten
│   ├── AuctionHouseBehavior.h/cpp       # AH-Browsing
│   ├── MailboxBehavior.h/cpp            # Mailbox
│   ├── InnBehavior.h/cpp                # Gasthof/Rested XP
│   └── SocialBehavior.h/cpp             # Emotes, "Chatten"
│
├── Simulation/
│   ├── AFKSimulator.h/cpp               # AFK-Simulation
│   ├── HumanErrorSimulator.h/cpp        # "Fehler"-Simulation
│   ├── DistractionSimulator.h/cpp       # Ablenkungen
│   └── FatigueSimulator.h/cpp           # "Müdigkeit"
│
├── Scheduling/
│   ├── ActivityScheduler.h/cpp          # Tageszeit-Planung
│   ├── WeeklyScheduler.h/cpp            # Wochenplan
│   └── SessionPlanner.h/cpp             # Session-Planung
│
└── Profiles/
    ├── CasualPlayerProfile.h/cpp        # Gelegenheitsspieler
    ├── HardcorePlayerProfile.h/cpp      # Hardcore-Spieler
    ├── SocialPlayerProfile.h/cpp        # Sozialer Spieler
    └── FarmerPlayerProfile.h/cpp        # Farmer-Spieler
```

### 3.2 Integration mit bestehendem System

```
BotAI
    │
    ├── HybridAIController (existiert)
    │       └── Utility AI entscheidet WELCHE Aktivität
    │
    ├── HumanizationManager (NEU)
    │       ├── WIE LANGE Aktivität
    │       ├── WANN Pausen
    │       ├── WIE MENSCHLICH (Fehler, Variabilität)
    │       └── Persönlichkeits-Modifikation
    │
    ├── ActivitySessionManager (NEU)
    │       ├── GatheringSession (30-60 Min)
    │       ├── QuestingSession (30-90 Min)
    │       ├── CityLifeSession (10-30 Min)
    │       └── FishingSession (30-60 Min)
    │
    └── CityLifeBehaviorManager (NEU)
            ├── AH_BROWSING
            ├── MAILBOX_CHECK
            ├── INN_REST
            └── SOCIALIZING
```

---

## Teil 4: Implementierungsplan

### Phase 1: Core Humanization (40h)

| Task | Effort | Priority |
|------|--------|----------|
| HumanizationManager | 8h | P0 |
| ActivitySessionManager | 12h | P0 |
| PersonalityProfile | 8h | P0 |
| HumanizationConfig | 4h | P0 |
| Integration mit BotAI | 8h | P0 |

### Phase 2: Session Manager (50h)

| Task | Effort | Priority |
|------|--------|----------|
| GatheringSessionManager (30+ Min) | 12h | P0 |
| QuestingSessionManager | 10h | P1 |
| FishingSessionManager (30+ Min) | 10h | P1 |
| CookingSessionManager | 8h | P1 |
| Session-Transitionen | 10h | P1 |

### Phase 3: City Life (35h)

| Task | Effort | Priority |
|------|--------|----------|
| CityLifeBehaviorManager | 10h | P0 |
| AuctionHouseBehavior (10-30 Min) | 8h | P1 |
| InnBehavior (Rested XP) | 6h | P1 |
| MailboxBehavior | 4h | P2 |
| SocialBehavior (Emotes) | 7h | P2 |

### Phase 4: Simulation (30h)

| Task | Effort | Priority |
|------|--------|----------|
| AFKSimulator | 10h | P1 |
| HumanErrorSimulator | 10h | P2 |
| DistractionSimulator | 5h | P2 |
| FatigueSimulator | 5h | P3 |

### Phase 5: Scheduling (25h)

| Task | Effort | Priority |
|------|--------|----------|
| ActivityScheduler (Tageszeit) | 10h | P2 |
| WeeklyScheduler | 8h | P3 |
| SessionPlanner | 7h | P2 |

### Phase 6: Profiles (20h)

| Task | Effort | Priority |
|------|--------|----------|
| CasualPlayerProfile | 5h | P2 |
| HardcorePlayerProfile | 5h | P2 |
| SocialPlayerProfile | 5h | P3 |
| FarmerPlayerProfile | 5h | P3 |

**Gesamt: ~200h**

---

## Teil 5: Konfiguration

### 5.1 worldserver.conf Erweiterungen

```ini
###############################################################################
# PLAYERBOT HUMANIZATION SETTINGS
###############################################################################

# Enable humanization system
Playerbot.Humanization.Enabled = true

# Activity session settings
Playerbot.Humanization.Session.MinDuration = 1800000      # 30 Min minimum
Playerbot.Humanization.Session.MaxDuration = 5400000      # 90 Min maximum
Playerbot.Humanization.Session.GatheringMinDuration = 1800000  # 30 Min Gathering min

# City life settings
Playerbot.Humanization.CityLife.Enabled = true
Playerbot.Humanization.CityLife.AHBrowsingDuration = 600000    # 10 Min AH
Playerbot.Humanization.CityLife.InnRestChance = 0.3            # 30% Chance für Inn

# AFK simulation
Playerbot.Humanization.AFK.Enabled = true
Playerbot.Humanization.AFK.MinInterval = 1800000          # Min 30 Min zwischen AFKs
Playerbot.Humanization.AFK.MaxInterval = 3600000          # Max 60 Min zwischen AFKs
Playerbot.Humanization.AFK.MinDuration = 60000            # Min 1 Min AFK
Playerbot.Humanization.AFK.MaxDuration = 300000           # Max 5 Min AFK

# Human error simulation
Playerbot.Humanization.Errors.Enabled = true
Playerbot.Humanization.Errors.WrongPathChance = 0.05      # 5% falsche Route
Playerbot.Humanization.Errors.ForgotQuestChance = 0.02    # 2% Quest vergessen

# Day/night cycle
Playerbot.Humanization.DayNight.Enabled = true
Playerbot.Humanization.DayNight.NightActivityReduction = 0.5  # 50% weniger nachts
Playerbot.Humanization.DayNight.SleepStartHour = 23
Playerbot.Humanization.DayNight.SleepEndHour = 7
```

---

## Teil 6: Beispiel-Tagesablauf

### Bot mit "CasualPlayerProfile"

```
07:00 - Einloggen in Gasthof (Rested XP)
07:05 - Mailbox checken (5 Min)
07:10 - Zum AH gehen, Preise checken (15 Min)
07:25 - Questing starten (45 Min Session)
08:10 - Kurze AFK-Pause (3 Min)
08:13 - Questing fortsetzen (30 Min)
08:43 - Zurück zur Stadt
08:50 - Cooking Session (alle gesammelten Mats) (10 Min)
09:00 - Wieder Questing (60 Min Session)
10:00 - Gathering Session starten (45 Min)
10:45 - Zurück zur Stadt
10:50 - AH: Crafted Items verkaufen (10 Min)
11:00 - Fishing Session am See (40 Min)
11:40 - Zurück zur Stadt
11:45 - Im Gasthof sitzen (15 Min - "Mittagspause")
12:00 - Questing (60 Min)
...
23:00 - In Gasthof einloggen, "schlafen"
```

---

## Fazit

### Aktueller Status
| Aspekt | Status | Humanization |
|--------|--------|--------------|
| Quest System | ⭐⭐⭐⭐ 93.8% | ❌ Roboterhaft |
| Gathering | ⭐⭐⭐⭐ Funktioniert | ❌ Keine Sessions |
| Professions | ⭐⭐⭐⭐ Funktioniert | ❌ Keine Sessions |
| Idle Behavior | ⭐⭐⭐ Basis | ❌ Limitiert |
| City Life | ❌ Fehlt | ❌ Nicht vorhanden |
| AFK Simulation | ❌ Fehlt | ❌ Nicht vorhanden |

### Nach Implementierung
| Aspekt | Status | Humanization |
|--------|--------|--------------|
| Quest System | ⭐⭐⭐⭐⭐ | ✅ Mit Sessions & Pausen |
| Gathering | ⭐⭐⭐⭐⭐ | ✅ 30-60 Min Sessions |
| Professions | ⭐⭐⭐⭐⭐ | ✅ Cooking/Fishing Sessions |
| Idle Behavior | ⭐⭐⭐⭐⭐ | ✅ Erweiterte Contexts |
| City Life | ⭐⭐⭐⭐⭐ | ✅ AH, Inn, Mailbox |
| AFK Simulation | ⭐⭐⭐⭐⭐ | ✅ Natürliche Pausen |

### Priorität

**P0 (Kritisch)**:
1. HumanizationManager + ActivitySessionManager
2. GatheringSessionManager (30+ Min)
3. CityLifeBehaviorManager

**P1 (Hoch)**:
4. FishingSessionManager
5. AFKSimulator
6. InnBehavior (Rested XP)

**P2 (Medium)**:
7. Tageszeit-Scheduling
8. Human Error Simulation
9. Persönlichkeits-Profile
