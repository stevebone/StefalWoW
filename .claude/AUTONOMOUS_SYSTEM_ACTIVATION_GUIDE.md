# Autonomous Crash Processing System - Activation Guide

## 🎯 System Overview

The autonomous crash processing system eliminates all manual intervention from crash detection to fix deployment, requiring human approval only at the final PR review stage.

## 📊 Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                    AUTONOMOUS CRASH PIPELINE                        │
└─────────────────────────────────────────────────────────────────────┘

[CRASH DETECTED]
      ↓
┌────────────────────────────────────────────┐
│ Python Monitor (autonomous_crash_monitor)  │
│ • Detects new .dmp files                   │
│ • Runs CDB analysis                        │
│ • Creates request JSON                     │
│ • Creates marker file for Claude           │
└────────────────────────────────────────────┘
      ↓
┌────────────────────────────────────────────┐
│ Claude Auto-Processor                      │
│ • Detects marker files (30s interval)     │
│ • Reads crash analysis                    │
│ • Uses Trinity MCP + Serena MCP           │
│ • Generates enterprise fix                │
│ • Writes response JSON                    │
│ • Applies fix to filesystem               │
└────────────────────────────────────────────┘
      ↓
┌────────────────────────────────────────────┐
│ Python Deployment Pipeline                 │
│ • Detects response JSON                   │
│ • Compiles worldserver with fix           │
│ • Runs unit tests                         │
│ • Creates git commit                      │
│ • Creates GitHub PR (draft)               │
└────────────────────────────────────────────┘
      ↓
┌────────────────────────────────────────────┐
│ HUMAN APPROVAL (Only Human Touchpoint)     │
│ • Review PR                               │
│ • Approve or request changes              │
│ • Deploy to production                    │
└────────────────────────────────────────────┘
```

## 🚀 Quick Start (5 Minutes)

### Step 1: Start Python Monitor
```bash
cd C:\TrinityBots\TrinityCore
python .claude\scripts\autonomous_crash_monitor_enhanced.py
```

**What it does:**
- Watches `.claude/crash_analysis_queue/requests/` for pending requests
- Checks every 30 seconds
- Creates marker files for Claude Code to process

### Step 2: Start Claude Auto-Processor
```bash
# In a separate terminal or background process
cd C:\TrinityBots\TrinityCore
python .claude\scripts\claude_auto_processor.py
```

**What it does:**
- Watches `.claude/crash_analysis_queue/auto_process/` for markers
- Checks every 30 seconds
- Triggers Claude Code analysis automatically

### Step 3: Verify System Running
```bash
# Check logs
tail -f .claude/logs/autonomous_monitor_enhanced.log
tail -f .claude/logs/claude_auto_processor.log
```

**Expected output:**
```
[2025-10-31T...] [INFO] ENHANCED AUTONOMOUS CRASH MONITOR v2.0 - STARTED
[2025-10-31T...] [INFO] 🤖 Autonomous monitoring active - waiting for crash requests...
[2025-10-31T...] [Claude] [INFO] CLAUDE CODE AUTO-PROCESSOR - STARTED
[2025-10-31T...] [Claude] [INFO] 🤖 Monitoring for auto-process markers...
```

## 📁 File Structure

```
.claude/
├── crash_analysis_queue/
│   ├── requests/              # Crash analysis requests (from Python)
│   │   └── request_{id}.json
│   ├── responses/             # Analysis results (from Claude)
│   │   └── response_{id}.json
│   ├── auto_process/          # Marker files for auto-trigger
│   │   ├── process_{id}.json  # Created by Python monitor
│   │   └── ANALYZE_{id}.txt   # Created by Claude processor
│   └── triggers/              # Legacy trigger directory
├── scripts/
│   ├── autonomous_crash_monitor_enhanced.py  # Python side
│   └── claude_auto_processor.py               # Claude side
└── logs/
    ├── autonomous_monitor_enhanced.log        # Python monitor logs
    └── claude_auto_processor.log              # Claude processor logs
```

## 🔄 Workflow Details

### Phase 1: Crash Detection (Automated)
1. Server crashes, creates `.dmp` file in `M:/Wplayerbot/Crashes/`
2. Python monitor detects new `.dmp` file
3. Runs CDB analysis: `cdb.exe -z crash.dmp -c "!analyze -v; q"`
4. Parses CDB output, extracts stack trace, exception info
5. Creates `request_{id}.json` with crash details
6. Creates `process_{id}.json` marker for Claude

**No human involvement.**

### Phase 2: Analysis (Automated)
1. Claude processor detects marker file (checks every 30s)
2. Reads `request_{id}.json`
3. Reads CDB analysis `.txt` file directly
4. Uses Trinity MCP to research:
   - Relevant APIs (Player, Creature, Spell, etc.)
   - Game mechanics context
   - Common patterns and pitfalls
5. Uses Serena MCP to analyze:
   - Codebase structure
   - Method implementations
   - Usage patterns
6. Identifies root cause with precision
7. Designs enterprise-grade fix (no shortcuts!)
8. Writes `response_{id}.json` with comprehensive analysis
9. Applies fix to source files
10. Deletes marker files

**No human involvement.**

### Phase 3: Deployment (Automated)
1. Python monitor detects `response_{id}.json`
2. Compiles worldserver: `cmake --build . --target worldserver`
3. Runs unit tests (if configured)
4. Creates git commit with detailed message
5. Creates GitHub PR as draft
6. Posts PR link to monitoring channel

**No human involvement.**

### Phase 4: Approval (HUMAN REQUIRED)
1. Human reviews PR on GitHub
2. Checks fix quality, tests, and code quality
3. Approves or requests changes
4. Merges to production branch

**Only human touchpoint in entire pipeline.**

## ⏱️ Performance Metrics

### Target SLAs
- **Detection → Analysis Start:** <30 seconds
- **Analysis → Fix Complete:** 5-10 minutes
- **Fix → Compilation Done:** 5-15 minutes
- **Compilation → PR Created:** <1 minute
- **Total (Crash → PR):** <30 minutes

### Resource Usage
- **Python Monitor:** <50MB RAM, <1% CPU
- **Claude Processor:** <100MB RAM, <5% CPU
- **Disk I/O:** Minimal (only marker files and logs)

## 🧪 Testing the System

### Test 1: Simulate Crash Request
```bash
# Create a test request
cat > .claude/crash_analysis_queue/requests/request_test123.json << 'EOF'
{
  "request_id": "test123",
  "timestamp": "2025-10-31T12:00:00.000000",
  "status": "pending",
  "crash": {
    "crash_id": "test_crash_001",
    "exception_code": "C0000005",
    "crash_location": "TestClass::TestMethod",
    "error_message": "ACCESS_VIOLATION"
  },
  "request_type": "comprehensive_analysis",
  "priority": "high"
}
EOF

# Monitor logs to see autonomous processing
tail -f .claude/logs/autonomous_monitor_enhanced.log
```

**Expected:**
1. Python monitor detects request within 30 seconds
2. Creates marker file: `.claude/crash_analysis_queue/auto_process/process_test123.json`
3. Claude processor detects marker within 30 seconds
4. Creates analysis instruction
5. (In real scenario) Processes and creates response

### Test 2: End-to-End with Real Crash
1. Cause a known crash (e.g., NULL pointer dereference)
2. Wait for server to write `.dmp` file
3. Monitor logs for autonomous detection
4. Verify marker file created
5. Verify analysis instruction created
6. Verify response JSON generated
7. Verify fix applied
8. Verify compilation successful
9. Verify PR created

**Total time:** Should be <30 minutes

## 🔧 Configuration

### Python Monitor Configuration
Edit `.claude/scripts/autonomous_crash_monitor_enhanced.py`:

```python
# Check interval (seconds)
CHECK_INTERVAL = 30  # Default: 30 seconds

# Crash directories to monitor
CRASH_DIRS = [
    Path("M:/Wplayerbot/Crashes"),
    trinity_root / "crashes"
]
```

### Claude Processor Configuration
Edit `.claude/scripts/claude_auto_processor.py`:

```python
# Check interval (seconds)
CHECK_INTERVAL = 30  # Default: 30 seconds

# Analysis timeout (seconds)
ANALYSIS_TIMEOUT = 600  # Default: 10 minutes
```

## 📊 Monitoring Dashboard

### Real-Time Status
```bash
# Watch all autonomous activity
tail -f .claude/logs/autonomous_monitor_enhanced.log \
        .claude/logs/claude_auto_processor.log
```

### Queue Status
```bash
# Check pending requests
ls -la .claude/crash_analysis_queue/requests/

# Check responses
ls -la .claude/crash_analysis_queue/responses/

# Check markers
ls -la .claude/crash_analysis_queue/auto_process/
```

### System Health
```bash
# Check Python monitor is running
ps aux | grep autonomous_crash_monitor_enhanced

# Check Claude processor is running
ps aux | grep claude_auto_processor

# Check last activity
tail -20 .claude/logs/autonomous_monitor_enhanced.log
```

## 🚨 Troubleshooting

### Problem: No markers being created
**Check:**
1. Is Python monitor running? `ps aux | grep autonomous`
2. Are there pending requests? `ls .claude/crash_analysis_queue/requests/`
3. Check logs: `tail -50 .claude/logs/autonomous_monitor_enhanced.log`

**Fix:**
- Restart Python monitor
- Verify request files have `"status": "pending"`
- Check file permissions

### Problem: Markers not being processed
**Check:**
1. Is Claude processor running? `ps aux | grep claude_auto_processor`
2. Are marker files present? `ls .claude/crash_analysis_queue/auto_process/`
3. Check logs: `tail -50 .claude/logs/claude_auto_processor.log`

**Fix:**
- Restart Claude processor
- Verify marker files are valid JSON
- Check for analysis instruction files

### Problem: Analysis hangs
**Check:**
1. Look for `ANALYZE_{id}.txt` files
2. Check if Claude Code is processing
3. Verify Trinity MCP and Serena MCP are available

**Fix:**
- Check MCP server status
- Verify crash analysis files exist
- Restart Claude processor if needed

## ✅ Success Criteria

System is working correctly when:
- ✅ Crash detected within seconds
- ✅ Request created automatically
- ✅ Marker file created within 30 seconds
- ✅ Analysis instruction created within 60 seconds
- ✅ Response JSON generated within 10 minutes
- ✅ Fix applied automatically
- ✅ Compilation successful
- ✅ PR created within 30 minutes total
- ✅ Zero manual interventions until PR approval

## 📈 Metrics to Track

### Autonomous System Effectiveness
- **Autonomous Processing Rate:** Target >95%
- **Average Crash-to-PR Time:** Target <30 minutes
- **Fix Success Rate:** Target >90%
- **False Positive Rate:** Target <5%

### Human Involvement
- **Manual Interventions:** Target <5% of crashes
- **PR Approval Time:** (Human-dependent, not measured)
- **PR Approval Rate:** Target >90%

## 🔄 Maintenance

### Daily
- Check log file sizes: `du -h .claude/logs/*.log`
- Verify both processes running
- Review any failed processing attempts

### Weekly
- Clean up old marker files: `find .claude/crash_analysis_queue/auto_process -mtime +7 -delete`
- Archive old logs: `gzip .claude/logs/*.log.1`
- Review autonomous processing success rate

### Monthly
- Update crash patterns database
- Review and improve analysis algorithms
- Update documentation based on learnings

## 📝 Next Steps

After autonomous system is running:

1. **Monitor First Week:** Track all crashes and verify autonomous processing
2. **Tune Parameters:** Adjust check intervals, timeouts based on performance
3. **Add Notifications:** Integrate with Slack/Discord for PR notifications
4. **Expand Coverage:** Add more crash patterns to database
5. **Performance Optimization:** Reduce crash-to-PR time further

---

🤖 **Status:** Ready for Production
📅 **Last Updated:** 2025-10-31
🔧 **Maintainer:** Autonomous System Team

Generated with [Claude Code](https://claude.com/claude-code)
