# Analyze Crash - AUTONOMOUS MODE

**This command watches for triggers and processes crashes automatically**

## Auto-Watch Mode

```bash
/analyze-crash auto-watch
```

This mode:
1. Watches `.claude/crash_analysis_queue/triggers/` for new trigger files
2. Automatically processes each trigger
3. Runs full analysis pipeline
4. Applies fixes
5. Creates response
6. Never stops (daemon mode)

## How It Works

### Python Daemon → Trigger File
Python creates: `.claude/crash_analysis_queue/triggers/trigger_{request_id}.json`

```json
{
  "request_id": "abc123",
  "timestamp": "2025-10-31T...",
  "action": "analyze",
  "autonomous": true
}
```

### Claude Code → Automatic Processing
1. Detects trigger file
2. Reads corresponding request
3. Performs comprehensive analysis (Trinity MCP + Serena MCP)
4. Generates enterprise-grade fix
5. Writes response JSON
6. Applies fix to filesystem
7. Deletes trigger file
8. Continues watching

### Result
Zero human intervention from crash detection to fix application!

## Usage

**Start autonomous watcher:**
```bash
# In one terminal - Python daemon
python .claude/scripts/crash_watcher_daemon.py

# In another terminal (or VS Code) - Claude Code auto-watch
/analyze-crash auto-watch
```

**Both processes work together:**
- Python: Detects crashes, creates triggers, compiles, creates PRs
- Claude Code: Analyzes crashes, generates fixes, writes code

**Human only needed for:** PR approval

## Implementation

When you run `/analyze-crash auto-watch`, I will:

```python
import time
from pathlib import Path

triggers_dir = Path(".claude/crash_analysis_queue/triggers")
triggers_dir.mkdir(parents=True, exist_ok=True)

processed = set()

print("🤖 Auto-Watch Mode: Monitoring for crash triggers...")

while True:
    # Scan for trigger files
    for trigger_file in triggers_dir.glob("trigger_*.json"):
        trigger_id = trigger_file.stem.replace("trigger_", "")

        if trigger_id in processed:
            continue

        print(f"\n🔔 NEW TRIGGER: {trigger_id}")

        # Read trigger
        with open(trigger_file) as f:
            trigger = json.load(f)

        request_id = trigger['request_id']

        # Process the request (full /analyze-crash logic)
        print(f"🤖 Processing request {request_id}...")

        # Run full analysis pipeline
        process_crash_request(request_id)  # Your existing logic

        # Delete trigger when done
        trigger_file.unlink()
        processed.add(trigger_id)

        print(f"✅ Request {request_id} complete!")

    time.sleep(5)  # Check every 5 seconds
```

## Benefits

**Before (Manual):**
- Crash → Python creates request → **[WAIT FOR HUMAN]** → /analyze-crash → Response

**After (Autonomous):**
- Crash → Python creates trigger → **[AUTOMATIC]** Claude processes → Response → Fix applied → **[WAIT FOR APPROVAL]**

**Time saved:** Hours → Minutes
**Human involvement:** 100% → 5% (approval only)

## Quick Start

1. Start Python daemon:
```bash
cd C:\TrinityBots\TrinityCore
python .claude\scripts\crash_watcher_daemon.py
```

2. Start Claude auto-watch:
```bash
/analyze-crash auto-watch
```

3. Wait for crashes to be processed automatically!

4. Review PRs when ready

## Status

- ✅ Python daemon implemented
- ⏳ Claude auto-watch mode (you implement this when `/analyze-crash auto-watch` is called)
- ⏳ Full integration testing

---

**This is how the system should work - fully autonomous with human approval gates only!**
