# Overnight Workflow Guide v2.0

## Übersicht

Das Overnight-System führt automatisierte Analysen, Builds und Fixes durch, während du schläfst.

## Quick Start

### Vor dem Schlafengehen
```cmd
cd C:\TrinityBots\TrinityCore
overnight_automation.bat
```

### Am Morgen
1. Check `.claude/reports/OVERNIGHT_REPORT_*.md`
2. Review generierte Fixes
3. Commit oder Revert

## Workflow-Modi

### 🌙 Overnight Mode (Standard)
**Dauer**: 8 Stunden  
**Startet**: 22:00  
**Endet**: 06:00

| Task | Beschreibung | Auto-Fix |
|------|--------------|----------|
| Full Build | Alle 3 Konfigurationen | Nein |
| Crash Analysis | Analysiert neue .dmp Files | Ja |
| Memory Scan | Sucht nach Memory Leaks | Ja (safe) |
| Threading Audit | Prüft Deadlock-Risiken | Ja (safe) |
| Packet Migration | Findet Legacy-Packets | Nein |
| Performance Check | Benchmarks | Nein |
| Security Scan | Sicherheitsprüfung | Nein |

### 📅 Weekend Mode (Erweitert)
**Dauer**: 24 Stunden  
**Startet**: Samstag 22:00  
**Endet**: Sonntag 22:00

Zusätzlich zu Overnight:
- Kompletter Codebase-Audit
- Architektur-Review
- Refactoring-Vorschläge
- Technical Debt Assessment
- Dokumentations-Update

## Dateien & Verzeichnisse

```
.claude/
├── automation_config.json     # Konfiguration
├── overnight_mode.lock        # Lock während Ausführung
├── reports/
│   ├── OVERNIGHT_REPORT_*.md  # Hauptbericht
│   ├── memory_patterns_*.txt  # Memory-Analyse
│   ├── threading_patterns_*.txt
│   ├── legacy_packets_*.txt
│   └── crash_dumps_*.txt
├── logs/
│   ├── build_debug_*.log
│   ├── build_relwithdebinfo_*.log
│   └── build_release_*.log
├── rollback_points/
│   └── checkpoint_*.txt       # Git commit hashes
└── crash_analysis_queue/      # Pending crash analyses
```

## Checkpoint & Rollback

### Automatischer Checkpoint
Vor jeder Overnight-Session wird automatisch ein Git-Stash erstellt:
```bash
git stash push -m "overnight_checkpoint_TIMESTAMP"
```

### Manueller Rollback
```bash
# Liste alle Checkpoints
git stash list

# Rollback zu Checkpoint
git stash pop stash@{0}
```

## Quality Gates

Das System prüft automatisch:

| Gate | Schwellwert | Aktion bei Fail |
|------|-------------|-----------------|
| Build Errors | 0 | ❌ Stopp |
| Build Warnings | <50 | ⚠️ Warnung |
| Memory Leaks | 0 | ❌ Stopp |
| Deadlock Risk | 0 | ❌ Stopp |
| Security Critical | 0 | ❌ Stopp |
| Security High | 0 | ⚠️ Warnung |

## Auto-Fix Policies

### Safe Fixes (automatisch angewendet)
- Fehlende `#include` Statements
- Offensichtliche Null-Checks
- Einfache API-Migrations
- Code-Formatierung

### Manual Review Required
- Threading-Änderungen
- Architektur-Änderungen
- Komplexe Refactorings
- Security-Fixes

## Benachrichtigungen

### Bei Kritischen Issues
Erstellt: `.claude/alerts/critical_TIMESTAMP.md`

### Bei Completion
Erstellt: `.claude/OVERNIGHT_COMPLETE_DATE.md`

## Integration mit Claude Code

### Session starten nach Overnight
```
claude code
> /load .claude/reports/OVERNIGHT_REPORT_*.md
```

### Agent-Nutzung
```
claude code
> /agent playerbot-project-coordinator
> Analysiere den Overnight-Report und priorisiere Aufgaben
```

## Troubleshooting

### Build schlägt fehl
1. Check `.claude/logs/build_*.log`
2. Suche nach "error C"
3. Nutze `build-error-fixer` Agent

### Lock-File bleibt
```cmd
del .claude\overnight_mode.lock
```

### Kein Report generiert
1. Check Windows Task Scheduler
2. Verify Permissions
3. Run manually: `overnight_automation.bat`

## Anpassung

### Eigene Tasks hinzufügen
Edit `automation_config.json`:
```json
"tasks": {
  "my_custom_task": {
    "command": "my_script.bat",
    "timeout_minutes": 30,
    "on_failure": "log_and_continue"
  }
}
```

### Zeitplan ändern
```json
"schedule": {
  "start_time": "23:00",  // Später starten
  "end_time": "07:00",    // Später enden
  "days": ["Mon", "Wed", "Fri"]  // Nur bestimmte Tage
}
```

## Best Practices

1. **Immer vor Overnight**: `git status` prüfen
2. **Keine uncommitted Changes**: Sonst werden sie gestasht
3. **Report lesen**: Nicht blind mergen
4. **Rollback bereit**: Checkpoint-Hash notieren
5. **Logs aufräumen**: Wöchentlich alte Logs löschen

## Metriken & KPIs

Das System trackt:
- Build-Erfolgsrate
- Durchschnittliche Build-Zeit
- Anzahl gefundener Issues
- Auto-Fix-Erfolgsrate
- Regressions nach Overnight

Reports in: `.claude/metrics/`
