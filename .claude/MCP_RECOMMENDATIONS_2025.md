# MCP Server Empfehlungen für Playerbot-Projekt

## Aktuelle Konfiguration (`.mcp.json`)

### ✅ Aktiv

| Server | Beschreibung | Nutzen |
|--------|--------------|--------|
| **trinitycore** | Custom MCP für WoW-Daten | Spell/Item/Quest-Daten, DBC/DB2, GameTables |
| **sequential-thinking** | Strukturiertes Problemlösen | Komplexe Architekturentscheidungen |
| **filesystem** | Dateizugriff | Sichere Dateioperationen im Playerbot-Modul |
| **memory** | Persistenter Kontext | Merkt sich Projektkontext zwischen Sessions |

### ⏸️ Deaktiviert (Optional aktivieren)

| Server | Setup | Nutzen |
|--------|-------|--------|
| **cpp-tools** | `cargo install mcp-cpp-server` | C++ Symbol-Navigation, clangd-Integration |
| **github** | `GITHUB_TOKEN` env setzen | Issue/PR-Management, CI/CD |
| **brave-search** | Brave API Key | Web-Recherche für WoW/C++ Docs |

---

## 🚀 Empfohlene Zusätzliche MCP Server

### 1. **mcp-cpp** (HÖCHSTE PRIORITÄT für C++)

```bash
# Installation
cargo install mcp-cpp-server

# Voraussetzung: clangd installieren
winget install LLVM.LLVM
```

**Konfiguration** (in `.mcp.json`):
```json
"cpp-tools": {
  "command": "mcp-cpp-server",
  "env": {
    "CLANGD_PATH": "C:\\Program Files\\LLVM\\bin\\clangd.exe",
    "COMPILE_COMMANDS_DIR": "C:\\TrinityBots\\TrinityCore\\build"
  }
}
```

**Funktionen**:
- Symbol-Suche mit Vererbungs-Hierarchien
- Call-Graph-Analyse
- Template-Instanziierung verstehen
- CMake Build-Configuration Wechsel

**Warum wichtig**: Bei 636K LOC C++ ist semantische Navigation essentiell!

---

### 2. **Context7** (Dokumentation)

```bash
# Installation
npm install -g context7-mcp
```

**Konfiguration**:
```json
"context7": {
  "command": "context7-mcp",
  "env": {
    "CONTEXT7_API_KEY": "your-api-key"
  }
}
```

**Funktionen**:
- Aktuelle Boost-Dokumentation
- MySQL C++ Connector Docs
- OpenSSL API Reference

---

### 3. **Database MCP** (MySQL)

```json
"mysql": {
  "command": "npx",
  "args": ["-y", "@executeautomation/database-server"],
  "env": {
    "DB_TYPE": "mysql",
    "DB_HOST": "localhost",
    "DB_USER": "playerbot",
    "DB_PASSWORD": "playerbot"
  }
}
```

**Funktionen**:
- Direkte SQL-Queries
- Schema-Inspektion
- Query-Optimierung

**Hinweis**: Bereits in `trinitycore` MCP integriert, aber für komplexere Queries nützlich.

---

### 4. **Serena** (Semantic Navigation)

Du hast bereits `.serena` konfiguriert. Aktiviere den MCP:

```json
"serena": {
  "command": "serena-mcp",
  "env": {
    "SERENA_PROJECT_PATH": "C:\\TrinityBots\\TrinityCore"
  }
}
```

**Funktionen**:
- Symbol-Navigation
- Reference-Finding
- Memory-System für Projektkontext

---

## 🤖 Agent-Empfehlungen

Deine bestehenden Agents in `.claude/agents/` sind gut! Hier meine Priorisierung:

### Täglich nutzen:
1. **playerbot-project-coordinator** - Projektübersicht
2. **code-quality-reviewer** - Code Reviews
3. **cpp-architecture-optimizer** - Refactoring-Vorschläge
4. **concurrency-threading-specialist** - Deadlock-Prävention

### Bei Bedarf:
5. **security-auditor** - Sicherheitsüberprüfung
6. **performance-analyzer** - Profiling
7. **wow-mechanics-expert** - Spielmechanik-Fragen
8. **trinity-integration-tester** - API-Kompatibilität

### Automatisiert (Overnight):
9. **daily-report-generator** - Tägliche Zusammenfassung
10. **automated-fix-agent** - Automatische Fixes

---

## 📋 Setup-Checkliste

```
[ ] CLAUDE.md erstellt ✅
[ ] .mcp.json aktualisiert ✅
[ ] clangd installieren: winget install LLVM.LLVM
[ ] mcp-cpp-server installieren: cargo install mcp-cpp-server
[ ] compile_commands.json generieren:
    cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
[ ] GITHUB_TOKEN setzen (optional)
[ ] Brave API Key holen (optional)
```

---

## 🔧 Empfohlener Workflow

```
┌─────────────────────────────────────────────────────────────┐
│                    DEVELOPMENT WORKFLOW                      │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. SESSION START                                           │
│     └─▶ Claude Code liest CLAUDE.md automatisch             │
│     └─▶ memory MCP lädt vorherigen Kontext                  │
│                                                             │
│  2. CODE NAVIGATION                                         │
│     └─▶ cpp-tools für Symbol-Suche                          │
│     └─▶ serena für Reference-Finding                        │
│                                                             │
│  3. IMPLEMENTATION                                          │
│     └─▶ trinitycore MCP für Spell/Item Daten               │
│     └─▶ sequential-thinking für komplexe Logik              │
│                                                             │
│  4. VERIFICATION                                            │
│     └─▶ Build testen                                        │
│     └─▶ code-quality-reviewer Agent                         │
│                                                             │
│  5. SESSION END                                             │
│     └─▶ memory MCP speichert Kontext                        │
│     └─▶ Handover-Dokument aktualisieren                     │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## ⚠️ Wichtige Hinweise

1. **compile_commands.json**: Für cpp-tools MUSS diese Datei existieren:
   ```bash
   cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBUILD_PLAYERBOT=1
   ```

2. **Windows-Pfade**: Immer `\\` oder `/` verwenden, nie einzelnes `\`

3. **Token-Limits**: Bei großen Dateien Memory-MCP nutzen statt alles in Context zu laden

4. **Deadlock-Risiko**: Nie zwei MCP-Server gleichzeitig für Datei-Schreiboperationen nutzen
