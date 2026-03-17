# Enterprise .dmp Crash Dump Analysis Architecture

**Date:** 2025-10-31
**Status:** 🚧 Design Phase
**Goal:** Best-in-class automated crash dump analysis for TrinityCore

---

## 🎯 Architecture Overview

### Multi-Tool Analysis Pipeline

```
┌─────────────────────────────────────────────────────────────────┐
│                     CRASH DETECTED                               │
│              worldserver.exe crashes                             │
└────────────────────┬────────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│              FILE PAIR GENERATED                                 │
│  1. crash_text.txt (5-6 MB) ✅ Currently parsed                 │
│  2. crash_dump.dmp (85-200 KB) ⚠️  Not analyzed                 │
└────────────────────┬────────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│           PARALLEL ANALYSIS (NEW)                                │
│                                                                   │
│  ┌──────────────────┐         ┌──────────────────────┐          │
│  │ Text Parser      │         │ DMP Analyzer (NEW)   │          │
│  │ (Enhanced)       │         │                      │          │
│  │                  │         │ Tool 1: CDB          │          │
│  │ - Exception code │         │ Tool 2: SuperDump    │          │
│  │ - Call stack     │         │ Tool 3: mcp-windbg   │          │
│  │ - Thread analysis│         │                      │          │
│  └──────────────────┘         └──────────────────────┘          │
│           │                             │                        │
└───────────┼─────────────────────────────┼────────────────────────┘
            │                             │
            ▼                             ▼
┌─────────────────────────────────────────────────────────────────┐
│              COMPREHENSIVE CRASH REPORT                          │
│                                                                   │
│  From .txt:                  From .dmp:                          │
│  - Thread stacks             - Memory state                      │
│  - Exception info            - Register values                   │
│  - File/line location        - Heap analysis                     │
│                              - Variable values                   │
│                              - Object states                     │
│                              - Memory corruption                 │
│                                                                   │
│              → Sent to Claude Code for Analysis                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🔧 Tool Selection (Best-in-Class)

### Tool 1: CDB (Console Debugger) ⭐⭐⭐⭐⭐
**Provider:** Microsoft Windows SDK
**Status:** ✅ Already installed
**Location:** `C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\cdb.exe`

**Capabilities:**
- Official Microsoft debugger (industry standard)
- `!analyze -v` - Automated crash analysis
- Symbol server integration (Microsoft + local PDB)
- Register dump, memory inspection
- Call stack with local variables
- Heap corruption detection
- Thread state analysis
- Exception record details

**Usage:**
```bash
cdb -z crash.dmp -y "SRV*C:\Symbols*https://msdl.microsoft.com/download/symbols;M:\Wplayerbot" -c "!analyze -v; .ecxr; k; q" -logo analysis.txt
```

**Output Includes:**
- Exception analysis
- Faulting module/function
- Stack trace with symbols
- Register dump
- Memory dump around crash address
- Recommended fixes

**Advantages:**
- ✅ Zero-cost (free)
- ✅ Already installed
- ✅ Command-line friendly
- ✅ Symbol support for TrinityCore PDB files
- ✅ Industry-standard accuracy

### Tool 2: SuperDump ⭐⭐⭐⭐
**Provider:** Dynatrace (Open Source)
**Status:** ⚠️ Requires installation
**Repository:** https://github.com/Dynatrace/superdump

**Capabilities:**
- Automated analysis pipeline
- REST API interface
- Batch processing (multiple dumps)
- JSON structured output
- Web UI for manual review
- Integrates WinDbg + DebugDiag
- Historical crash comparison
- Pattern matching across dumps

**Architecture:**
```
SuperDump Service (Web/REST)
    ├─> WinDbg Analysis Engine
    ├─> DebugDiag Analysis
    └─> JSON Report Generator
```

**Advantages:**
- ✅ Structured JSON output (easy to parse)
- ✅ REST API (programmatic access)
- ✅ Batch processing (analyze multiple dumps)
- ✅ Historical comparison
- ⚠️ Requires .NET service installation

**Use Case:**
- Batch analysis of historical crashes
- Pattern detection across multiple dumps
- Automated regression detection

### Tool 3: mcp-windbg (MCP Server) ⭐⭐⭐⭐⭐
**Provider:** Open Source Community
**Status:** ⚠️ Requires installation
**Repository:** https://github.com/modelcontextprotocol/mcp-windbg

**Capabilities:**
- **Natural language queries to WinDbg**
- MCP protocol integration (Claude Code native)
- Spawns CDB subprocess
- Parses WinDbg output to structured format
- Interactive debugging through Claude
- Python-based (easy to integrate)

**Architecture:**
```
Claude Code
    │
    ├─> MCP Protocol
    │
    └─> mcp-windbg (Python)
            │
            └─> CDB.exe
                    │
                    └─> crash.dmp
```

**Example Usage:**
```
Claude: "What caused the crash in worldserver.exe?"
mcp-windbg: [Executes !analyze -v, parses output]
Claude: "Show me the value of variable X at crash time"
mcp-windbg: [Executes dv /t /v, returns structured data]
```

**Advantages:**
- ✅ Native Claude Code integration
- ✅ Natural language interface
- ✅ Interactive debugging
- ✅ No manual command scripting
- ⚠️ Requires Python + Node.js setup

**PERFECT FOR:** Real-time crash investigation by Claude Code

---

## 📊 Recommended Implementation (3-Tier Approach)

### Tier 1: Immediate Analysis (CDB)
**When:** Every crash (automated)
**Tool:** CDB with `!analyze -v`
**Output:** Text report with exception, stack, registers
**Integration:** Python subprocess call
**Latency:** <5 seconds

**Implementation:**
```python
class DmpAnalyzer:
    def analyze_with_cdb(self, dmp_file: Path) -> dict:
        """
        Analyze .dmp with CDB for immediate crash info
        """
        cdb_path = Path("C:/Program Files (x86)/Windows Kits/10/Debuggers/x64/cdb.exe")
        symbol_path = f"SRV*C:\\Symbols*https://msdl.microsoft.com/download/symbols;{self.pdb_dir}"

        cmd = [
            str(cdb_path),
            "-z", str(dmp_file),
            "-y", symbol_path,
            "-c", "!analyze -v; .ecxr; k; dv /t /v; q",
            "-logo", str(output_file)
        ]

        subprocess.run(cmd, capture_output=True, timeout=30)

        return self._parse_cdb_output(output_file)
```

**Benefits:**
- Fast (< 5 seconds)
- No external services
- Works offline
- Accurate exception info

### Tier 2: Deep Analysis (mcp-windbg MCP Server)
**When:** Claude Code analysis phase
**Tool:** mcp-windbg through MCP protocol
**Output:** Interactive Q&A with crash dump
**Integration:** MCP server in Claude Code config
**Latency:** <1 second per query

**Implementation:**
```json
// .claude.json MCP server config
{
  "mcpServers": {
    "windbg": {
      "command": "node",
      "args": ["path/to/mcp-windbg/dist/index.js"],
      "env": {
        "CDB_PATH": "C:\\Program Files (x86)\\Windows Kits\\10\\Debuggers\\x64\\cdb.exe",
        "SYMBOL_PATH": "SRV*C:\\Symbols*https://msdl.microsoft.com/download/symbols"
      }
    }
  }
}
```

**Claude Code Usage:**
```
User: /analyze-crash

Claude: [Reads request JSON with .dmp path]
Claude: [Calls mcp-windbg to analyze crash.dmp]
Claude: "What was the value of BotSession->m_owner at crash time?"
mcp-windbg: [Returns pointer value + object state]
Claude: "Show me the last 10 function calls before crash"
mcp-windbg: [Returns detailed call stack with parameters]
```

**Benefits:**
- Interactive debugging
- Natural language queries
- Deep variable inspection
- Memory state analysis

### Tier 3: Batch Historical Analysis (SuperDump)
**When:** Weekly pattern detection, regression analysis
**Tool:** SuperDump service
**Output:** JSON reports, trend analysis
**Integration:** REST API calls
**Latency:** ~10-30 seconds per dump

**Use Cases:**
- Analyzing 100+ historical crashes
- Detecting crash patterns
- Regression detection (new crash vs old crash)
- Stability metrics

---

## 🚀 Implementation Plan

### Phase 1: CDB Integration (PRIORITY HIGH) ⭐
**Effort:** ~4 hours
**Impact:** Immediate improvement in crash analysis quality

**Tasks:**
1. Create `DmpAnalyzer` class in `crash_analyzer.py`
2. Implement `analyze_with_cdb()` method
3. Parse CDB output (exception, stack, registers, locals)
4. Enhance `CrashInfo` dataclass with .dmp analysis results
5. Integrate into hybrid orchestrator
6. Test with real crash dumps

**Deliverables:**
- ✅ Every crash analyzed with CDB
- ✅ Memory state, register values, variable values
- ✅ Enhanced crash reports to Claude Code

### Phase 2: mcp-windbg Integration (PRIORITY MEDIUM) ⭐⭐
**Effort:** ~3 hours
**Impact:** Interactive debugging capability for Claude Code

**Tasks:**
1. Install mcp-windbg via npm
2. Configure MCP server in `.claude.json`
3. Test connection from Claude Code
4. Update `/analyze-crash` command to use mcp-windbg
5. Create example interactive debugging workflows
6. Document usage patterns

**Deliverables:**
- ✅ Claude Code can interactively debug crashes
- ✅ Natural language crash investigation
- ✅ Deep memory inspection

### Phase 3: SuperDump Integration (PRIORITY LOW) ⭐
**Effort:** ~6 hours
**Impact:** Batch analysis and historical pattern detection

**Tasks:**
1. Install SuperDump service (Docker or native)
2. Create REST API client in Python
3. Implement batch upload script
4. Create pattern detection reports
5. Dashboard for crash trends

**Deliverables:**
- ✅ Batch analysis of historical crashes
- ✅ Pattern detection across dumps
- ✅ Regression detection

---

## 📋 Required Tools Installation

### Already Installed ✅
- CDB (Console Debugger) - `C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\cdb.exe`

### To Install (Phase 2)
```bash
# mcp-windbg
npm install -g @modelcontextprotocol/mcp-windbg
```

### To Install (Phase 3 - Optional)
```bash
# SuperDump (Docker)
docker pull dynatrace/superdump
docker run -p 8080:80 dynatrace/superdump
```

---

## 🎓 Expected Benefits

### Current State (Text Parser Only)
```
Exception Code: C0000005 ✅
Fault Address: 00007FF65911FB20 ✅
Crash Location: EventDispatcher.cpp:106 ✅
Crash Function: Playerbot::Events::EventDispatcher::UnsubscribeAll ✅
Call Stack: [15 frames] ✅

❌ Missing: Register values
❌ Missing: Variable values
❌ Missing: Memory state
❌ Missing: Heap analysis
❌ Missing: Object state at crash
```

### With CDB Analysis (Phase 1)
```
Exception Code: C0000005 ✅
Fault Address: 00007FF65911FB20 ✅
Crash Location: EventDispatcher.cpp:106 ✅
Crash Function: Playerbot::Events::EventDispatcher::UnsubscribeAll ✅
Call Stack: [15 frames with parameter values] ✅

✅ Register dump: RAX=0, RCX=invalid_ptr
✅ Local variables: subscriber_ptr=nullptr ← ROOT CAUSE!
✅ Memory state: Heap corruption detected
✅ Object state: EventDispatcher::m_subscribers invalid
✅ Recommendation: Null pointer dereference in UnsubscribeAll
```

### With mcp-windbg (Phase 2)
```
Claude: "What was the value of subscriber_ptr when it crashed?"
mcp-windbg: "0x0000000000000000 (nullptr)"

Claude: "Show me the EventDispatcher object state"
mcp-windbg: "m_subscribers: std::vector<Subscriber*> size=5, corrupted"

Claude: "Was this a double-free?"
mcp-windbg: "Yes, heap corruption detected. Subscriber was freed twice."

→ IMMEDIATE ROOT CAUSE IDENTIFICATION
→ NO MANUAL DEBUGGING NEEDED
```

---

## ✅ Quality Standards

### Phase 1 (CDB) Success Criteria
- ✅ 100% of crashes analyzed with CDB
- ✅ Exception analysis integrated into crash reports
- ✅ Register dump available to Claude Code
- ✅ Local variable values captured
- ✅ <5 second analysis latency

### Phase 2 (mcp-windbg) Success Criteria
- ✅ Interactive debugging from Claude Code
- ✅ Natural language crash queries
- ✅ Memory inspection on demand
- ✅ <1 second per query latency

### Phase 3 (SuperDump) Success Criteria
- ✅ Batch analysis capability
- ✅ Historical pattern detection
- ✅ Trend reports generated

---

## 🎯 Recommendation: START WITH PHASE 1 (CDB)

**Rationale:**
1. ✅ Tool already installed (zero setup cost)
2. ✅ Immediate quality improvement (register dump, locals, memory state)
3. ✅ Fast implementation (~4 hours)
4. ✅ High ROI (best bang for buck)
5. ✅ Foundation for Phase 2 (mcp-windbg)

**Next Steps:**
1. Implement `DmpAnalyzer` class with CDB integration
2. Test with real crash dumps
3. Integrate into hybrid orchestrator
4. Document usage

---

**Status:** 📋 Architecture Complete - Ready for Implementation
**Recommended Start:** Phase 1 (CDB Integration)
**Estimated Time:** ~4 hours for Phase 1

🤖 Generated with [Claude Code](https://claude.com/claude-code)
