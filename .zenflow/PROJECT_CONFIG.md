# Zenflow Projekt-Konfiguration für TrinityCore Playerbot

## Projekt-Einstellungen (in Zenflow App konfigurieren)

### 1. Project Settings
```
Project Name: TrinityCore-Playerbot
Git Repository Path: C:\TrinityBots\TrinityCore
```

### 2. Scripts & Automation

**Setup Script** (wird vor jedem Task ausgeführt):
```powershell
# Ensure build directory exists
if (!(Test-Path "build")) { 
    cmake -B build -DBUILD_PLAYERBOT=1 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON 
}
```

**Dev Server Script** (optional):
```powershell
# Start worldserver for testing (optional)
# .\build\bin\RelWithDebInfo\worldserver.exe
```

**Cleanup Script** (nach jedem Task):
```powershell
# Clean up temporary files
Remove-Item -Path "*.tmp" -ErrorAction SilentlyContinue
```

### 3. Copy Files (in jeden Worktree kopieren)
```
.env.local
playerbots.conf.dist
```

### 4. AI Rules Folders
```
.zenflow/rules
.claude/agents
```

### 5. Always-Include Rule Files
```
CLAUDE.md
.zenflow/rules/cpp-standards.md
.zenflow/rules/playerbot-patterns.md
```

---

## Model-Konfiguration

### Empfohlene Konfiguration für C++

**Primary Agent (Implementation)**: Claude Opus 4
- Beste Reasoning-Fähigkeit
- Versteht komplexe C++ Patterns
- Ideal für Architektur-Entscheidungen

**Verification Agent**: GPT-4o oder Gemini 2.0
- Cross-Model Verification
- Findet blinde Flecken von Claude

**Quick Tasks**: Claude Sonnet 4
- Schneller und günstiger
- Für einfache Fixes und Refactoring

---

## MCP Server Integration

In Zenflow unter "Integrations > MCP Servers" hinzufügen:

### Context7 (Boost/MySQL Docs)
```json
{
  "name": "context7",
  "type": "http",
  "url": "https://mcp.context7.com/v1",
  "headers": {
    "Authorization": "Bearer YOUR_API_KEY"
  }
}
```

### TrinityCore MCP (falls kompatibel)
```json
{
  "name": "trinitycore",
  "type": "stdio",
  "command": "node",
  "args": ["C:\\TrinityBots\\trinity-mcp-server\\dist\\index.js"]
}
```

---

## Worktree-Isolation

Zenflow erstellt für jeden Task einen eigenen Git Worktree:
```
C:\TrinityBots\TrinityCore\           # Hauptrepo
C:\TrinityBots\TrinityCore\.zenflow\
  └── worktrees\
      ├── task-001-new-combat-ai\     # Isolierter Worktree
      ├── task-002-packet-migration\  # Isolierter Worktree
      └── task-003-memory-fix\        # Isolierter Worktree
```

**Vorteil**: Mehrere Features parallel entwickeln ohne Konflikte!

---

## Nächste Schritte

1. Öffne Zenflow Desktop App
2. Gehe zu Settings > Project Config
3. Konfiguriere wie oben beschrieben
4. Importiere die Custom Workflows aus `.zenflow/workflows/`
