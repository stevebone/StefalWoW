# Zenflow Optimal Nutzen - 30-Tage Guide

**Dein Setup**: Visual Studio Enterprise 2025 + Zenflow + Claude Code

---

## 🚀 Quick Start (Tag 1)

### 1. Zenflow Desktop App konfigurieren

1. **Öffne Zenflow** Desktop App
2. **Settings > Project Config**:
   - Project Name: `TrinityCore-Playerbot`
   - Git Repository Path: `C:\TrinityBots\TrinityCore`
   
3. **AI Rules Folder hinzufügen**:
   - `.zenflow/rules`
   
4. **Always-Include Files**:
   - `CLAUDE.md`
   - `.zenflow/rules/cpp-standards.md`
   - `.zenflow/rules/playerbot-patterns.md`

### 2. Workflows importieren

Die Custom Workflows in `.zenflow/workflows/` werden automatisch erkannt:
- `playerbot-feature.md` - Neue Features mit SDD
- `playerbot-bugfix.md` - Schnelle Bug-Fixes
- `packet-migration.md` - Legacy → Typed Packets

---

## 📋 Täglicher Workflow

### Morgens: Planen

```
1. Zenflow öffnen
2. Neuen Task erstellen
3. Workflow auswählen:
   - "Playerbot Feature" für neue Features
   - "Playerbot Bugfix" für Bugs
   - "Packet Migration" für API-Migration
4. Beschreibung eingeben
5. Task starten
```

### Während Entwicklung: Parallel arbeiten

**Zenflow-Vorteil**: Mehrere Tasks gleichzeitig!

```
Task 1: Neuer Combat-AI (Zenflow arbeitet)
Task 2: Bug Fix #123 (Zenflow arbeitet)
Task 3: Du reviewst in VS Enterprise
```

### Abends: Review & Merge

```
1. Zenflow: Review abgeschlossene Tasks
2. VS Enterprise: Code inspizieren
3. Akzeptieren oder Feedback geben
4. Merge wenn OK
```

---

## 🎯 Best Practices für 30 Tage

### Woche 1: Basics (Tag 1-7)

**Ziel**: Zenflow-Workflows verstehen

| Tag | Aufgabe |
|-----|---------|
| 1 | Setup, erster einfacher Task |
| 2 | Bug-Fix Workflow testen |
| 3 | Feature-Workflow testen |
| 4 | Multi-Agent Verification verstehen |
| 5 | Parallele Tasks ausprobieren |
| 6-7 | Erste echte Feature-Entwicklung |

**Tipp**: Starte mit kleinen Tasks um Zenflow kennenzulernen!

### Woche 2: Produktivität (Tag 8-14)

**Ziel**: Parallel entwickeln

| Fokus | Beschreibung |
|-------|--------------|
| Parallelisierung | 3-5 Tasks gleichzeitig laufen lassen |
| Review-Workflow | Claude prüft GPT-Code |
| VS Integration | Zenflow-Output in VS Enterprise bearbeiten |

**Tipp**: Nutze die Zeit während Zenflow arbeitet für andere Aufgaben!

### Woche 3: Optimierung (Tag 15-21)

**Ziel**: Workflows anpassen

| Fokus | Beschreibung |
|-------|--------------|
| Custom Workflows | Eigene Workflows für häufige Tasks |
| AI Rules verfeinern | Projekt-spezifische Regeln |
| Performance | Herausfinden welche Tasks am besten funktionieren |

### Woche 4: Evaluation (Tag 22-30)

**Ziel**: Entscheiden ob Abo verlängern

| Frage | Kriterium |
|-------|-----------|
| Code-Qualität | Besser als ohne Zenflow? |
| Produktivität | Mehr Features geschafft? |
| Fehlerrate | Weniger Bugs? |
| ROI | Zeitersparnis > Kosten? |

---

## 🔧 Zenflow + VS Enterprise Kombination

### Workflow-Integration

```
┌─────────────────────────────────────────────────────────┐
│                    ZENFLOW                               │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐  │
│  │ Task Start  │───▶│ AI Working  │───▶│ Review Ready│  │
│  └─────────────┘    └─────────────┘    └─────────────┘  │
└──────────────────────────────┬──────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────┐
│              VISUAL STUDIO ENTERPRISE                    │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐  │
│  │ Code Review │───▶│   Debug     │───▶│   Merge     │  │
│  │ IntelliSense│    │  Profiling  │    │   Commit    │  │
│  └─────────────┘    └─────────────┘    └─────────────┘  │
└─────────────────────────────────────────────────────────┘
```

### VS Enterprise für Review nutzen

Nach Zenflow-Task:
1. **Solution Explorer**: Geänderte Dateien anzeigen
2. **IntelliSense**: Code auf Fehler prüfen
3. **Live Unit Testing**: Tests automatisch ausführen
4. **Code Analysis**: Statische Analyse
5. **Memory Profiler**: Bei Performance-kritischem Code

### Debugging in VS Enterprise

Wenn Zenflow-Code Probleme hat:
1. **Breakpoints** in geändertem Code setzen
2. **IntelliTrace** für Threading-Issues
3. **Parallel Stacks** für Deadlock-Analyse
4. **Memory Diagnostics** für Leaks

---

## 💡 Pro-Tipps

### 1. Spec First, Code Later

```
RICHTIG:
1. Task erstellen mit klarer Beschreibung
2. Spec review BEVOR Implementation
3. Dann erst implementieren lassen

FALSCH:
1. "Fix the bug" als Beschreibung
2. Hoffen dass AI versteht was gemeint ist
```

### 2. Multi-Agent Verification nutzen

```
Konfiguration:
- Primary: Claude Opus 4 (Implementation)
- Verifier: GPT-4o (Review)

Warum: Jedes Modell hat blinde Flecken.
       Cross-Verification findet ~20% mehr Issues!
```

### 3. Parallele Execution maximieren

```
Gut für Parallelisierung:
✅ Unabhängige Bug-Fixes
✅ Verschiedene Module
✅ Packet-Migration (verschiedene Dateien)

Schlecht für Parallelisierung:
❌ Features die aufeinander aufbauen
❌ Änderungen an denselben Dateien
```

### 4. Iterationen begrenzen

```yaml
# In Workflow-Config
Max Iterations: 3

Warum:
- Nach 3 Versuchen: menschliche Hilfe nötig
- Verhindert Token-Verschwendung ($50-100+)
- Fokussiert auf lösbare Probleme
```

### 5. Rules aktuell halten

```
Wenn AI immer wieder denselben Fehler macht:
1. Regel in .zenflow/rules/ hinzufügen
2. Zenflow neu starten
3. Regel wird automatisch geladen
```

---

## 📊 Metriken tracken

### Was messen?

| Metrik | Vor Zenflow | Mit Zenflow | Ziel |
|--------|-------------|-------------|------|
| Features/Woche | ? | ? | +50% |
| Bug-Fix Zeit | ? | ? | -40% |
| Build-Fehler | ? | ? | -30% |
| Code Review Zeit | ? | ? | -50% |

### Tracking-Vorlage

```markdown
## Woche X Summary

### Tasks abgeschlossen
- Feature A: 4h (geschätzt 8h) ✅
- Bug #123: 30min (geschätzt 2h) ✅

### Qualität
- Build-Fehler: 0
- Review-Runden: 1

### Learnings
- [Was gut funktioniert hat]
- [Was verbessert werden kann]
```

---

## ❓ FAQ

### Q: Zenflow vs Claude Code - wann was?

| Situation | Tool |
|-----------|------|
| Neue Feature entwickeln | Zenflow |
| Bug-Fix mit Spec | Zenflow |
| Tiefe Architektur-Analyse | Claude Code |
| WoW-Daten abfragen (MCP) | Claude Code |
| Overnight-Automation | Claude Code |

### Q: Wie mit großen Dateien umgehen?

Zenflow hat ~200K Token Context. Bei sehr großen Dateien:
1. In Spec genau beschreiben welche Funktionen betroffen
2. Rules nutzen um Kontext zu geben
3. Bei Bedarf: File aufteilen (Refactoring-Task)

### Q: Was wenn Zenflow falsch implementiert?

1. Feedback im Task geben
2. Iteration starten
3. Nach 3 Versuchen: manuell in VS Enterprise korrigieren
4. Regel hinzufügen um Problem in Zukunft zu vermeiden

---

## 📁 Erstellte Dateien

```
.zenflow/
├── PROJECT_CONFIG.md           # Setup-Anleitung
├── workflows/
│   ├── playerbot-feature.md    # Feature-Workflow
│   ├── playerbot-bugfix.md     # Bug-Fix-Workflow
│   └── packet-migration.md     # Migration-Workflow
└── rules/
    ├── cpp-standards.md        # C++ Coding Standards
    └── playerbot-patterns.md   # Playerbot-spezifische Patterns
```

---

*Viel Erfolg mit Zenflow! Nach 30 Tagen evaluieren ob Abo verlängern.*
