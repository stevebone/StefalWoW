# Zenflow + Obsidian Setup - Quick Start Checklist

**Goal:** Get orchestration running in 2-3 hours

**Full Guide:** See `ZENFLOW_OBSIDIAN_SETUP_GUIDE.md` for detailed instructions

---

## ☑️ Pre-Setup (15 minutes)

- [ ] **Download Obsidian** from https://obsidian.md/download
- [ ] **Download Zenflow** from https://zencoder.ai/zenflow
- [ ] **Check Node.js:** `node --version` (need v18+)
  - If not installed: https://nodejs.org/
- [ ] **Find Anthropic API Key** (for Zenflow)
  - Dashboard: https://console.anthropic.com/

---

## ☑️ Obsidian Setup (30 minutes)

- [ ] **Install Obsidian Desktop**
- [ ] **Create new vault:**
  - Name: `TrinityCore-Playerbot-Memory`
  - Location: `C:\TrinityBots\PlayerBotProjectMemory`
- [ ] **Enable Community Plugins:**
  - Settings → Community plugins → Turn off Restricted mode
- [ ] **Install "Local REST API" plugin:**
  - Browse plugins → Search "Local REST API"
  - Install and enable
  - Copy API key from settings (SAVE THIS!)
- [ ] **Install "Dataview" plugin** (optional but recommended)
- [ ] **Create folder structure:**
  - `Architecture/`
  - `Refactorings/`
  - `Gotchas/`
  - `Tasks/`
  - `Sessions/`
  - `Decisions/`
- [ ] **Create core files** (copy from setup guide):
  - `Architecture/System Overview.md`
  - `Gotchas/Git Workflow Gotchas.md`
  - `Refactorings/DI Cleanup.md`
  - `Tasks/Active Instances.md`
  - `Tasks/Task Template.md`

---

## ☑️ MCP Server Setup (20 minutes)

- [ ] **Clone repository:**
  ```bash
  cd C:\TrinityBots
  git clone https://github.com/cyanheads/obsidian-mcp-server.git
  cd obsidian-mcp-server
  ```
- [ ] **Install dependencies:**
  ```bash
  npm install
  ```
- [ ] **Build:**
  ```bash
  npm run build
  ```
- [ ] **Create config.json:**
  - Location: `C:\TrinityBots\obsidian-mcp-server\config.json`
  - Content: See setup guide Section 2.4
  - **IMPORTANT:** Use your Obsidian API key!
- [ ] **Test server:**
  ```bash
  node dist/index.js
  # Should start without errors
  # Ctrl+C to stop
  ```

---

## ☑️ Zenflow Setup (20 minutes)

- [ ] **Install Zenflow Desktop**
- [ ] **Sign in / Create account**
- [ ] **Create project:**
  - Name: `TrinityCore Playerbot`
  - Path: `C:\TrinityBots\TrinityCore`
  - Type: C++ / CMake
- [ ] **Configure AI Provider:**
  - Settings → AI Providers → Add Anthropic
  - Enter API key
  - Select models: Opus 4.5, Sonnet 4.5, Haiku
- [ ] **Connect Git:**
  - Project Settings → Version Control
  - Enable "Branch per agent" ✅
  - Prefix: `zenflow/agent-{agent-id}/`

---

## ☑️ Claude Code Integration (15 minutes)

- [ ] **Edit MCP config:**
  - Location: `%APPDATA%\Roaming\claude-code\mcp.json`
  - Add `obsidian-memory` server (see setup guide 4.1)
- [ ] **Set environment variable:**
  - Windows: Environment Variables → New
  - Name: `CLAUDE_CODE_TASK_LIST_ID`
  - Value: `trinitycore-playerbot-shared`
- [ ] **Restart all terminals**
- [ ] **Test MCP:**
  ```bash
  claude-code
  > Can you list available MCP tools?
  # Should see obsidian_* tools
  ```
- [ ] **Test Obsidian read:**
  ```bash
  > Read "Architecture/System Overview" from Obsidian vault
  # Should display content
  ```

---

## ☑️ Create First Workflow (30 minutes)

- [ ] **Open Zenflow Desktop**
- [ ] **New Workflow:**
  - Name: `DI Cleanup Refactoring`
  - Template: Spec-Driven Development
- [ ] **Configure 5 stages:**
  1. Analyze Dependencies (Opus 4.5)
  2. Create Technical Spec (Opus 4.5)
  3. Implement Changes (Sonnet 4.5)
  4. Verify Build (Sonnet 4.5)
  5. Code Review (Opus 4.5)
- [ ] **Set dependencies:**
  - Stage 2 depends on Stage 1
  - Stage 3 depends on Stage 2
  - Stage 4 depends on Stage 3
  - Stage 5 depends on Stage 4
- [ ] **Configure agent settings:**
  - Enable "Isolated Branches" ✅
  - Enable "Shared Memory" → Obsidian ✅
  - Vault: `C:\TrinityBots\PlayerBotProjectMemory`
- [ ] **Add instructions to each stage** (see setup guide 5.2)

---

## ☑️ Run Test (20 minutes)

- [ ] **Create test task in Obsidian:**
  - File: `Tasks/TEST-2026-02-06-01 - Verify Setup.md`
  - Copy template from setup guide 6.1
- [ ] **Start workflow in Zenflow:**
  - Run Workflow → DI Cleanup Refactoring
  - Sprint: `TEST`
  - Click Start
- [ ] **Watch progress:**
  - Zenflow: See DAG visualization
  - Obsidian: Open Graph View (Ctrl+G)
  - Watch new notes appear in `Sessions/`
- [ ] **Verify results:**
  - Check for 5 new markdown files in Obsidian
  - Check git branches: `git branch | grep zenflow`
  - Check task list: `claude-code` → `/tasks`
- [ ] **Cleanup test:**
  ```bash
  git branch -D zenflow/di-cleanup-sprint-TEST/*
  # Delete test notes in Obsidian
  ```

---

## ☑️ Verify Everything Works

- [ ] **Multiple instances can share tasks** ✅
  - Open 2 Claude Code terminals
  - Both should see same tasks: `/tasks`
- [ ] **Obsidian memory is accessible** ✅
  - Claude can read notes: `> Read "Architecture/System Overview"`
  - Claude can write notes: `> Write a test note to Obsidian`
- [ ] **Zenflow shows progress** ✅
  - GUI displays running workflows
  - Stage logs are visible
- [ ] **No git conflicts** ✅
  - Each agent uses separate branch
  - Clean merge possible

---

## ✅ You're Done! What's Next?

### Immediate Next Steps:
1. **Update Obsidian notes** with your TrinityCore-specific info
2. **Run real refactoring sprint** (not TEST)
3. **Monitor and refine** workflows based on experience

### Advanced Features to Explore:
- File locking pattern (Section 7.1)
- Session handover (Section 7.2)
- Emergency stop (Section 7.3)
- Cross-repository coordination (Section 7.5)

### Daily Workflow:
See Section "Daily Workflow" in full guide for:
- Morning startup checklist
- Starting new tasks
- Hourly checks
- End of day routine

---

## 🆘 Quick Troubleshooting

| Problem | Solution |
|---------|----------|
| MCP server won't start | Check Obsidian REST API plugin enabled, API key correct |
| Zenflow can't find repo | Re-scan repository in project settings |
| Tasks not shared | Set `CLAUDE_CODE_TASK_LIST_ID` env var, restart terminals |
| Obsidian links don't show | Use `[[Note]]` syntax not `[Note](path)` |
| Workflow hangs | Check logs in Zenflow, cancel and restart stage |

See "Troubleshooting" section in full guide for detailed fixes.

---

## 📚 Resources

- **Full Setup Guide:** `.claude/ZENFLOW_OBSIDIAN_SETUP_GUIDE.md`
- **Zenflow Docs:** https://zencoder.ai/docs
- **Obsidian MCP Server:** https://github.com/cyanheads/obsidian-mcp-server
- **Claude Code Docs:** https://code.claude.com/docs
- **MCP Protocol:** https://modelcontextprotocol.io/

---

**Total Time:** ~2-3 hours for complete setup
**Complexity:** Moderate (some terminal/config work)
**Payoff:** Massive (prevents disasters, enables safe parallel work)

**Last Updated:** 2026-02-06
