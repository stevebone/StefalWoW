# Optimierter Zenflow + Claude Code Workflow

## Übersicht: Wann welches Tool?

```
┌─────────────────────────────────────────────────────────────────────┐
│                         TASK-TYPEN                                   │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  🔧 IMPLEMENTATION TASKS          →  ZENFLOW                        │
│     - Neue Features                                                  │
│     - Bug Fixes                                                      │
│     - Refactoring                                                    │
│     - Code nach Review-Feedback                                      │
│     Warum: Multi-Agent Verification, Isolated Worktrees              │
│                                                                      │
│  🔍 ANALYSE & RESEARCH TASKS      →  CLAUDE CODE                    │
│     - Code Review erstellen                                          │
│     - Architektur-Analyse                                            │
│     - WoW-Daten abfragen (MCP)                                       │
│     - Crash-Analyse                                                  │
│     - Codebase durchsuchen                                           │
│     Warum: MCP Integration, 200K Context, Memory                     │
│                                                                      │
│  🐛 DEBUGGING & PROFILING         →  VS ENTERPRISE                  │
│     - Breakpoints & Step-Through                                     │
│     - Memory Profiling                                               │
│     - Performance Analysis                                           │
│     - IntelliTrace                                                   │
│     Warum: Beste Debugging-Tools für C++                             │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Workflow A: Code Review → Implementation (EMPFOHLEN)

### Schritt 1: Claude Code macht Review
```
Claude Code:
> Analysiere src/modules/Playerbot/Combat/*.cpp
> Erstelle Code Review mit konkreten Verbesserungen
> Speichere als .claude/reviews/combat_review_2025-01-25.md
```

### Schritt 2: Zenflow setzt um MIT Verification
```
Zenflow:
1. Neuer Task: "Implement Combat Review Findings"
2. Workflow: "Playerbot Bugfix" oder "Quick Change"
3. Beschreibung: Review-MD einfügen oder referenzieren
4. Zenflow implementiert UND verifiziert mit zweitem Model
```

### Schritt 3: VS Enterprise für Final Review
```
VS Enterprise:
1. Änderungen inspizieren
2. Build & Test
3. Bei Bedarf: Debug/Profile
4. Merge
```

**Vorteil**: Du bekommst Cross-Model Verification (Claude prüft GPT oder umgekehrt)

---

## Workflow B: Direkt in Zenflow (für neue Features)

### Alles in Zenflow
```
Zenflow:
1. Task: "Implement new Combat AI for Warrior"
2. Workflow: "Playerbot Feature"
3. Zenflow:
   - Erstellt Spec (du reviewst)
   - Implementiert
   - Zweites Model verifiziert
   - Du reviewst final
```

**Vorteil**: Spec-Driven Development verhindert "Code Drift"

---

## Workflow C: Parallele Entwicklung (für viele kleine Tasks)

### Wenn Review viele Tasks enthält:
```
Review enthält:
- Task 1: Fix null pointer in CombatAI.cpp
- Task 2: Add thread safety to MovementManager
- Task 3: Migrate packets in SpellHandler.cpp
- Task 4: Optimize database queries
- Task 5: Add missing error handling

Zenflow Parallel:
- Starte alle 5 als separate Tasks
- Jeder läuft in eigenem Worktree
- Keine Konflikte
- Alle werden parallel verifiziert
```

**Vorteil**: 5 Tasks in der Zeit von 1 Task

---

## Wann Claude Code statt Zenflow?

| Situation | Tool | Grund |
|-----------|------|-------|
| Brauche WoW-Daten (Spells, Items) | Claude Code | trinitycore MCP |
| Suche in 636K LOC Codebase | Claude Code | 200K Context |
| Crash-Dump analysieren | Claude Code | Agents + MCP |
| Overnight Builds & Reports | Claude Code | Automation |
| Brauche Kontext aus vorheriger Session | Claude Code | Memory MCP |

---

## Quick Reference

```
FRAGE: Was will ich tun?

Neues Feature bauen?        → ZENFLOW (Feature Workflow)
Bug fixen?                  → ZENFLOW (Bugfix Workflow)
Code Review erstellen?      → CLAUDE CODE
Review-Tasks umsetzen?      → ZENFLOW (mit Verification!)
WoW-Daten nachschlagen?     → CLAUDE CODE (MCP)
Codebase durchsuchen?       → CLAUDE CODE
Debuggen/Profilen?          → VS ENTERPRISE
Performance-Problem finden? → VS ENTERPRISE + CLAUDE CODE
```

---

## Beispiel: Dein heutiger Workflow optimiert

### Was du gemacht hast:
```
1. Zenflow: Code Review → MD
2. Claude Code: Tasks umsetzen
```

### Optimiert:
```
1. Claude Code: Code Review → MD (besser für Analyse mit MCP)
2. Zenflow: Tasks umsetzen MIT Verification (Claude prüft GPT)
3. VS Enterprise: Final Debug wenn nötig
```

Oder noch besser:
```
1. Claude Code: Tiefe Analyse mit MCP, erstellt Review-MD
2. Zenflow: 
   - Importiere Review als Spec
   - Starte parallele Tasks für jeden Finding
   - Multi-Agent Implementation + Verification
3. VS Enterprise: Final Review & Merge
```
