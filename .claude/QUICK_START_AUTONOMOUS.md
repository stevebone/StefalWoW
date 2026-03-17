# Autonomous Crash System - QUICK START ⚡

## Start in 30 Seconds

### Windows
```bash
cd C:\TrinityBots\TrinityCore
.\.claude\scripts\start_autonomous_system.bat
```

### Linux/macOS
```bash
cd /path/to/TrinityCore

# Terminal 1
python .claude/scripts/autonomous_crash_monitor_enhanced.py

# Terminal 2
python .claude/scripts/claude_auto_processor.py
```

## What You'll See

### Python Monitor Window
```
═══════════════════════════════════════════════════════════════════
ENHANCED AUTONOMOUS CRASH MONITOR v2.0 - STARTED
═══════════════════════════════════════════════════════════════════
Trinity Root: C:\TrinityBots\TrinityCore
Check Interval: 30 seconds
═══════════════════════════════════════════════════════════════════

🤖 Autonomous monitoring active - waiting for crash requests...
```

### Claude Processor Window
```
═══════════════════════════════════════════════════════════════════
CLAUDE CODE AUTO-PROCESSOR - STARTED
═══════════════════════════════════════════════════════════════════
Trinity Root: C:\TrinityBots\TrinityCore
Check Interval: 30 seconds
═══════════════════════════════════════════════════════════════════

🤖 Monitoring for auto-process markers...
```

## When a Crash Happens

### Python Monitor (30s later)
```
🔔 DETECTED 1 PENDING REQUEST(S)

╔═══════════════════════════════════════════════════════════
║ AUTONOMOUS PROCESSING INITIATED
╠═══════════════════════════════════════════════════════════
║ Request ID: abc123
║ Crash ID: 273f92f0f16d
╚═══════════════════════════════════════════════════════════

✅ Created auto-process marker: process_abc123.json
SUCCESS: Request abc123 queued for Claude Code processing
```

### Claude Processor (30s later)
```
🔔 DETECTED 1 MARKER FILE(S)

╔══════════════════════════════════════════════════════════
║ AUTONOMOUS ANALYSIS TRIGGER
╠══════════════════════════════════════════════════════════
║ Request ID: abc123
║ Crash ID: 273f92f0f16d
╚══════════════════════════════════════════════════════════

Found CDB analysis: M:/Wplayerbot/Crashes/crash_273f92f0f16d.cdb_analysis.txt
✅ Created analysis instruction: ANALYZE_abc123.txt
```

## Logs

```bash
# Monitor Python side
tail -f .claude/logs/autonomous_monitor_enhanced.log

# Monitor Claude side
tail -f .claude/logs/claude_auto_processor.log
```

## Test It

```bash
# Create test request
cat > .claude/crash_analysis_queue/requests/request_test.json << 'EOF'
{
  "request_id": "test",
  "timestamp": "2025-10-31T12:00:00",
  "status": "pending",
  "crash": {
    "crash_id": "test_001",
    "exception_code": "C0000005",
    "crash_location": "Test::Method",
    "error_message": "ACCESS_VIOLATION"
  }
}
EOF

# Watch it get processed automatically within 60 seconds
watch -n 1 'ls -la .claude/crash_analysis_queue/auto_process/'
```

## Stop It

Press `Ctrl+C` in each window, or close the windows.

## Full Documentation

See: `.claude/AUTONOMOUS_SYSTEM_ACTIVATION_GUIDE.md`

---

That's it! The system is now fully autonomous. 🎉
