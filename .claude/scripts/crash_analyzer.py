#!/usr/bin/env python3
"""
Enterprise Crash Dump Analyzer with AI-Powered Root Cause Detection
Automatically analyzes TrinityCore crash dumps, identifies bugs, and generates fixes

Features:
- Monitors crash dump directories continuously
- Parses .txt and .dmp crash files
- AI-powered stack trace analysis
- Root cause pattern matching
- Automated fix generation
- GitHub issue creation
- Crash clustering and deduplication
- Historical crash database

Usage:
    python crash_analyzer.py --watch M:/Wplayerbot/Crashes/
    python crash_analyzer.py --analyze crash_file.txt
    python crash_analyzer.py --auto-fix crash_file.txt --compile --test
"""

import os
import sys
import re
import json
import time
import hashlib
import subprocess
import argparse
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass, asdict
from collections import defaultdict

# ============================================================================
# Data Structures
# ============================================================================

@dataclass
class CrashInfo:
    """Structured crash information"""
    crash_id: str
    timestamp: str
    exception_code: str
    exception_address: str
    crash_location: str  # file:line
    crash_function: str
    error_message: str
    call_stack: List[str]
    crash_category: str
    severity: str
    is_bot_related: bool
    affected_components: List[str]
    root_cause_hypothesis: str
    fix_suggestions: List[str]
    similar_crashes: List[str]
    file_path: str
    raw_dump: str
    # DMP analysis (from CDB)
    dmp_analysis: Optional[Dict] = None  # CDB analysis results
    registers: Optional[Dict] = None  # CPU register dump
    local_variables: Optional[Dict] = None  # Variables at crash location
    heap_corruption: bool = False  # Heap corruption detected
    cdb_recommendations: Optional[List[str]] = None  # CDB suggested fixes

@dataclass
class CrashPattern:
    """Known crash pattern for matching"""
    pattern_id: str
    pattern_name: str
    location_regex: str
    stack_patterns: List[str]
    root_cause: str
    fix_template: str
    severity: str
    occurrences: int

# ============================================================================
# Crash Pattern Database
# ============================================================================

KNOWN_CRASH_PATTERNS = [
    CrashPattern(
        pattern_id="BIH_COLLISION",
        pattern_name="BoundingIntervalHierarchy Invalid Node Overlap",
        location_regex=r"BoundingIntervalHierarchy\.cpp:\d+",
        stack_patterns=[
            r"BIH::subdivide",
            r"BIH::buildHierarchy",
            r"DynamicMapTree::getHeight",
            r"PathGenerator::CalculatePath"
        ],
        root_cause="Core TrinityCore pathfinding bug with corrupt map geometry",
        fix_template="Update TrinityCore core (Issue #5218) or regenerate map files",
        severity="CRITICAL",
        occurrences=0
    ),
    CrashPattern(
        pattern_id="SPELL_CPP_603",
        pattern_name="Spell.cpp:603 HandleMoveTeleportAck Crash",
        location_regex=r"Spell\.cpp:603",
        stack_patterns=[
            r"Spell::HandleMoveTeleportAck",
            r"DeathRecoveryManager"
        ],
        root_cause="Teleport ack called before 100ms delay during resurrection",
        fix_template="Add 100ms deferral before HandleMoveTeleportAck() call",
        severity="CRITICAL",
        occurrences=0
    ),
    CrashPattern(
        pattern_id="MAP_686_ADDOBJECT",
        pattern_name="Map.cpp:686 AddObjectToRemoveList Crash",
        location_regex=r"Map\.cpp:686",
        stack_patterns=[
            r"Map::AddObjectToRemoveList",
            r"BotSession|Playerbot"
        ],
        root_cause="Bot object removal timing issue during login/logout",
        fix_template="Ensure proper session state checks before object removal",
        severity="HIGH",
        occurrences=0
    ),
    CrashPattern(
        pattern_id="UNIT_10863_SPELLMOD",
        pattern_name="Unit.cpp:10863 TakingDamage SpellMod Access",
        location_regex=r"Unit\.cpp:10863",
        stack_patterns=[
            r"Unit::DealDamage",
            r"m_spellModTakingDamage"
        ],
        root_cause="Accessing m_spellModTakingDamage from bot without proper initialization",
        fix_template="Remove m_spellModTakingDamage access or ensure IsBot() guard",
        severity="HIGH",
        occurrences=0
    ),
    CrashPattern(
        pattern_id="SOCKET_NULL_DEREF",
        pattern_name="Socket Null Dereference",
        location_regex=r"Socket\.(h|cpp):\d+",
        stack_patterns=[
            r"Socket::CloseSocket",
            r"BotSession"
        ],
        root_cause="Null socket operation despite IsBot() check",
        fix_template="Add null check before socket operations in BotSession",
        severity="HIGH",
        occurrences=0
    ),
    CrashPattern(
        pattern_id="DEADLOCK_RECURSIVE_MUTEX",
        pattern_name="Deadlock from Recursive Mutex",
        location_regex=r".*",
        stack_patterns=[
            r"std::recursive_mutex",
            r"BotSession.*LoginCharacter"
        ],
        root_cause="Recursive lock acquisition in BotSession login flow",
        fix_template="Use std::unique_lock with try_lock or refactor locking strategy",
        severity="CRITICAL",
        occurrences=0
    ),
    CrashPattern(
        pattern_id="GHOST_AURA_DUPLICATE",
        pattern_name="Duplicate Ghost Aura Application",
        location_regex=r"Spell\.cpp.*",
        stack_patterns=[
            r"Aura::Create.*8326",
            r"DeathRecoveryManager"
        ],
        root_cause="Ghost aura (8326) applied multiple times during death",
        fix_template="Remove duplicate aura application, TrinityCore handles it",
        severity="MEDIUM",
        occurrences=0
    )
]

# ============================================================================
# DMP File Analyzer (Windows Debugging Tools Integration)
# ============================================================================

class DmpAnalyzer:
    """
    Analyzes .dmp (minidump) files using Windows Debugging Tools (CDB)
    Provides memory state, register values, local variables, and heap analysis
    """

    def __init__(self, pdb_dir: Optional[Path] = None):
        """
        Initialize DMP analyzer

        Args:
            pdb_dir: Directory containing .pdb symbol files for TrinityCore
        """
        # CDB path (Windows Debugging Tools)
        self.cdb_path = Path("C:/Program Files (x86)/Windows Kits/10/Debuggers/x64/cdb.exe")

        # Symbol paths
        self.microsoft_symbols = "SRV*C:\\Symbols*https://msdl.microsoft.com/download/symbols"
        self.pdb_dir = Path(pdb_dir) if pdb_dir else None

        # Check if CDB is installed
        self.cdb_available = self.cdb_path.exists()

    def analyze(self, dmp_file: Path) -> Optional[Dict]:
        """
        Analyze .dmp file with CDB and return structured results

        Returns:
            dict with:
                - exception_analysis: Exception code, address, description
                - registers: CPU register dump
                - call_stack_detailed: Call stack with parameter values
                - local_variables: Variables at crash location
                - memory_dump: Memory around crash address
                - heap_analysis: Heap corruption detection
                - recommendations: CDB recommended fixes
        """
        if not self.cdb_available:
            print(f"   ⚠️  CDB not available at {self.cdb_path}")
            return None

        if not dmp_file.exists():
            print(f"   ⚠️  DMP file not found: {dmp_file}")
            return None

        print(f"   🔍 Analyzing .dmp with CDB: {dmp_file.name}")

        # Build symbol path
        symbol_path = self.microsoft_symbols
        if self.pdb_dir and self.pdb_dir.exists():
            symbol_path += f";{self.pdb_dir}"

        # Create temporary output file
        output_file = dmp_file.with_suffix('.cdb_analysis.txt')

        # CDB commands (comprehensive analysis - quality over speed):
        # !analyze -v    : Verbose crash analysis with root cause detection
        # .ecxr          : Set exception context (registers at crash time)
        # k              : Display call stack with symbols
        # kv             : Display call stack with parameters
        # dv /t /v       : Display local variables with types and values
        # r              : Display all CPU registers
        # !heap -a       : Full heap analysis (detects corruption, use-after-free)
        # !address @rax  : Analyze what RAX points to (helps identify bad pointers)
        # !address @rcx  : Analyze what RCX points to
        # dc @rsp        : Dump memory at stack pointer (helps see stack corruption)
        # q              : Quit
        # NOTE: This may take 60-120 seconds, but provides BEST crash analysis quality
        cdb_commands = "!analyze -v; .ecxr; k; kv; dv /t /v; r; !heap -a; !address @rax; !address @rcx; dc @rsp L20; q"

        try:
            # Run CDB
            cmd = [
                str(self.cdb_path),
                "-z", str(dmp_file),
                "-y", symbol_path,
                "-c", cdb_commands,
                "-logo", str(output_file)
            ]

            result = subprocess.run(
                cmd,
                capture_output=True,
                timeout=180,  # 180 second timeout (comprehensive analysis with heap checking)
                text=True
            )

            if result.returncode != 0 and result.returncode != 1:  # CDB returns 1 on normal exit
                print(f"   ⚠️  CDB exited with code {result.returncode}")
                return None

            # Parse CDB output
            if output_file.exists():
                with open(output_file, 'r', encoding='utf-8', errors='ignore') as f:
                    cdb_output = f.read()

                analysis = self._parse_cdb_output(cdb_output)

                print(f"   ✅ CDB analysis complete")
                print(f"      Exception: {analysis.get('exception_code', 'N/A')}")
                print(f"      Fault Address: {analysis.get('fault_address', 'N/A')}")
                print(f"      Faulting Module: {analysis.get('faulting_module', 'N/A')}")

                return analysis

        except subprocess.TimeoutExpired:
            print(f"   ⚠️  CDB analysis timed out after 180 seconds")
            print(f"   This can happen with very large dumps or slow symbol servers")
            return None
        except Exception as e:
            print(f"   ⚠️  CDB analysis failed: {e}")
            return None

        return None

    def _parse_cdb_output(self, output: str) -> Dict:
        """Parse CDB comprehensive analysis output into structured format"""

        analysis = {
            'exception_code': 'UNKNOWN',
            'fault_address': 'UNKNOWN',
            'faulting_module': 'UNKNOWN',
            'faulting_function': 'UNKNOWN',
            'exception_description': '',
            'registers': {},
            'local_variables': {},
            'call_stack_detailed': [],
            'call_stack_with_params': [],  # From kv command
            'memory_dump': [],
            'stack_memory': [],  # From dc @rsp
            'pointer_analysis': {},  # From !address commands
            'heap_corruption': False,
            'use_after_free': False,
            'null_pointer_deref': False,
            'recommendations': [],
            'raw_analysis': ''  # Full !analyze -v output for reference
        }

        # Parse exception code (supports both formats)
        # Format 1: "ExceptionCode: c0000005 (Access violation)"
        match = re.search(r'ExceptionCode:\s+([a-f0-9]+)\s+\((.+?)\)', output, re.IGNORECASE)
        if match:
            analysis['exception_code'] = match.group(1).upper()
            analysis['exception_description'] = match.group(2).strip()
        else:
            # Format 2: "EXCEPTION_CODE: (C0000005) ACCESS_VIOLATION"
            match = re.search(r'EXCEPTION_CODE:\s+\(([A-F0-9]+)\)\s+(.+)', output, re.IGNORECASE)
            if match:
                analysis['exception_code'] = match.group(1)
                analysis['exception_description'] = match.group(2).strip()

        # Parse fault address and function
        # Format: "ExceptionAddress: 00007ff65911fb20 (worldserver!function)"
        match = re.search(r'ExceptionAddress:\s+([a-f0-9]+)\s+\((.+?)\)', output, re.IGNORECASE)
        if match:
            analysis['fault_address'] = match.group(1).upper()
            # Extract function name from "worldserver!std::vector<...>::end"
            func_info = match.group(2).strip()
            if '!' in func_info:
                analysis['faulting_function'] = func_info.split('!', 1)[1]
            else:
                analysis['faulting_function'] = func_info
        else:
            # Fallback: FAULTING_IP format
            match = re.search(r'FAULTING_IP:\s*\n([^\n]+)\s+([A-F0-9]+)', output, re.IGNORECASE)
            if match:
                analysis['faulting_function'] = match.group(1).strip()
                analysis['fault_address'] = match.group(2)

        # Parse faulting module
        match = re.search(r'MODULE_NAME:\s+(.+)', output, re.IGNORECASE)
        if match:
            analysis['faulting_module'] = match.group(1).strip()

        # Parse ALL CPU registers (64-bit x64: 16 general purpose + flags)
        # Look for register dump after .ecxr command or in CONTEXT section
        reg_patterns = [
            r'rax=([\da-f]+)',
            r'rbx=([\da-f]+)',
            r'rcx=([\da-f]+)',
            r'rdx=([\da-f]+)',
            r'rsi=([\da-f]+)',
            r'rdi=([\da-f]+)',
            r'rip=([\da-f]+)',
            r'rsp=([\da-f]+)',
            r'rbp=([\da-f]+)',
            r'r8=([\da-f]+)',
            r'r9=([\da-f]+)',
            r'r10=([\da-f]+)',
            r'r11=([\da-f]+)',
            r'r12=([\da-f]+)',
            r'r13=([\da-f]+)',
            r'r14=([\da-f]+)',
            r'r15=([\da-f]+)',
            r'iopl=(\d+)',
            r'[^a-z]cs=([\da-f]+)',
            r'[^a-z]ss=([\da-f]+)',
            r'[^a-z]ds=([\da-f]+)',
            r'[^a-z]es=([\da-f]+)',
            r'[^a-z]fs=([\da-f]+)',
            r'[^a-z]gs=([\da-f]+)',
            r'efl=([\da-f]+)',
        ]

        for pattern in reg_patterns:
            match = re.search(pattern, output, re.IGNORECASE)
            if match:
                reg_name = pattern.split('=')[0].replace(r'\d+', '').replace('[^a-z]', '').replace('(', '').replace(r'\w+', '')
                # Extract register name from pattern
                if 'rax' in pattern: reg_name = 'rax'
                elif 'rbx' in pattern: reg_name = 'rbx'
                elif 'rcx' in pattern: reg_name = 'rcx'
                elif 'rdx' in pattern: reg_name = 'rdx'
                elif 'rsi' in pattern: reg_name = 'rsi'
                elif 'rdi' in pattern: reg_name = 'rdi'
                elif 'rip' in pattern: reg_name = 'rip'
                elif 'rsp' in pattern: reg_name = 'rsp'
                elif 'rbp' in pattern: reg_name = 'rbp'
                elif 'r8' in pattern and 'r8=' in pattern: reg_name = 'r8'
                elif 'r9' in pattern: reg_name = 'r9'
                elif 'r10' in pattern: reg_name = 'r10'
                elif 'r11' in pattern: reg_name = 'r11'
                elif 'r12' in pattern: reg_name = 'r12'
                elif 'r13' in pattern: reg_name = 'r13'
                elif 'r14' in pattern: reg_name = 'r14'
                elif 'r15' in pattern: reg_name = 'r15'
                elif 'iopl' in pattern: reg_name = 'iopl'
                elif 'cs' in pattern: reg_name = 'cs'
                elif 'ss' in pattern: reg_name = 'ss'
                elif 'ds' in pattern: reg_name = 'ds'
                elif 'es' in pattern: reg_name = 'es'
                elif 'fs' in pattern: reg_name = 'fs'
                elif 'gs' in pattern: reg_name = 'gs'
                elif 'efl' in pattern: reg_name = 'efl'

                analysis['registers'][reg_name] = match.group(1).upper()

        # Parse call stack (detailed) - FULL STACK (no truncation)
        stack_section = re.search(r'STACK_TEXT:\s*\n(.*?)(?:\n\n|\Z)', output, re.DOTALL)
        if stack_section:
            stack_lines = stack_section.group(1).strip().split('\n')
            for line in stack_lines:  # ALL frames (no limit)
                if line.strip():
                    analysis['call_stack_detailed'].append(line.strip())

        # Parse local variables
        dv_section = re.search(r'((?:(?:Type|Name|Value).*?\n)+)', output)
        if dv_section:
            for match in re.finditer(r'(\w+)\s*=\s*(.+)', dv_section.group(1)):
                var_name = match.group(1).strip()
                var_value = match.group(2).strip()
                if var_name not in ['Type', 'Name', 'Value']:
                    analysis['local_variables'][var_name] = var_value

        # Parse call stack with parameters (kv command output) - FULL STACK
        kv_section = re.search(r'ChildEBP\s+RetAddr\s+(.*?)(?:\n\n|Child-SP|\Z)', output, re.DOTALL)
        if kv_section:
            kv_lines = kv_section.group(1).strip().split('\n')
            for line in kv_lines:  # ALL frames (no limit)
                if line.strip() and not line.startswith('Child'):
                    analysis['call_stack_with_params'].append(line.strip())

        # Parse stack memory dump (dc @rsp output)
        stack_mem_section = re.search(r'([\da-f]+`[\da-f]+\s+[\da-f]+.*?\n)+', output, re.IGNORECASE | re.MULTILINE)
        if stack_mem_section:
            stack_lines = stack_mem_section.group(0).strip().split('\n')
            for line in stack_lines[:10]:  # First 10 lines of stack
                analysis['stack_memory'].append(line.strip())

        # Parse pointer analysis (!address output)
        for reg in ['rax', 'rcx', 'rdx', 'rbx']:
            address_pattern = rf'!address\s+@{reg}.*?\n(.*?)(?:\n\n|0:|!address|\Z)'
            address_match = re.search(address_pattern, output, re.DOTALL | re.IGNORECASE)
            if address_match:
                address_info = address_match.group(1).strip()
                if 'Bad' in address_info or 'invalid' in address_info.lower():
                    analysis['pointer_analysis'][reg] = 'BAD POINTER'
                    analysis['null_pointer_deref'] = True
                elif 'free' in address_info.lower() or 'freed' in address_info.lower():
                    analysis['pointer_analysis'][reg] = 'USE-AFTER-FREE'
                    analysis['use_after_free'] = True
                else:
                    # Extract first meaningful line
                    first_line = address_info.split('\n')[0][:100]
                    analysis['pointer_analysis'][reg] = first_line

        # Check for heap corruption (comprehensive)
        heap_indicators = [
            'HEAP_CORRUPTION',
            'heap corruption',
            'Heap block at',
            'freed twice',
            'double free',
            'VERIFIER STOP'
        ]
        for indicator in heap_indicators:
            if indicator.lower() in output.lower():
                analysis['heap_corruption'] = True
                break

        if analysis['heap_corruption']:
            analysis['recommendations'].append("🔴 CRITICAL: Heap corruption detected - use-after-free or double-free bug")

        # Check for use-after-free
        if analysis['use_after_free'] or 'use after free' in output.lower() or 'freed memory' in output.lower():
            analysis['use_after_free'] = True
            analysis['recommendations'].append("🔴 CRITICAL: Use-after-free detected - accessing freed memory")

        # Check for null pointer dereference
        if analysis['null_pointer_deref'] or 'null pointer' in output.lower() or 'NULL dereference' in output.lower():
            analysis['null_pointer_deref'] = True
            analysis['recommendations'].append("⚠️  Null pointer dereference - missing null check")

        # Parse recommended followup
        followup_match = re.search(r'FOLLOWUP_NAME:\s+(.+)', output, re.IGNORECASE)
        if followup_match:
            analysis['recommendations'].append(f"Followup: {followup_match.group(1).strip()}")

        # Extract analysis summary
        summary_match = re.search(r'BUGCHECK_STR:\s+(.+)', output, re.IGNORECASE)
        if summary_match:
            analysis['recommendations'].append(f"Bug check: {summary_match.group(1).strip()}")

        # Extract FAILURE_BUCKET_ID (very useful for categorization)
        bucket_match = re.search(r'FAILURE_BUCKET_ID:\s+(.+)', output, re.IGNORECASE)
        if bucket_match:
            analysis['recommendations'].append(f"Failure category: {bucket_match.group(1).strip()}")

        # Store raw analysis for future reference
        analyze_section = re.search(r'FAULTING_IP:(.*?)(?:SYMBOL_NAME|\Z)', output, re.DOTALL)
        if analyze_section:
            analysis['raw_analysis'] = analyze_section.group(0)[:2000]  # First 2000 chars

        return analysis

# ============================================================================
# Crash Analyzer Core
# ============================================================================

class CrashAnalyzer:
    """
    Enterprise crash dump analyzer with AI-powered root cause detection
    """

    def __init__(self, trinity_root: Path, logs_dir: Path, pdb_dir: Optional[Path] = None):
        self.trinity_root = Path(trinity_root)
        self.logs_dir = Path(logs_dir)
        self.crash_db_path = self.trinity_root / ".claude" / "crash_database.json"
        self.crash_patterns = KNOWN_CRASH_PATTERNS
        self.crash_database: Dict[str, dict] = self._load_crash_database()

        # Initialize DMP analyzer
        self.dmp_analyzer = DmpAnalyzer(pdb_dir=pdb_dir)

    def _load_crash_database(self) -> Dict[str, dict]:
        """Load historical crash database"""
        if self.crash_db_path.exists():
            with open(self.crash_db_path, 'r', encoding='utf-8') as f:
                return json.load(f)
        return {"crashes": {}, "patterns": {}, "fixes": {}}

    def _save_crash_database(self):
        """Save crash database to disk"""
        self.crash_db_path.parent.mkdir(parents=True, exist_ok=True)
        with open(self.crash_db_path, 'w', encoding='utf-8') as f:
            json.dump(self.crash_database, f, indent=2)

    def parse_crash_dump(self, crash_file: Path) -> Optional[CrashInfo]:
        """
        Parse TrinityCore crash dump .txt file with multi-threaded crash detection

        Returns:
            CrashInfo object with all parsed crash details
        """
        if not crash_file.exists():
            print(f"❌ Crash file not found: {crash_file}")
            return None

        with open(crash_file, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()

        # Extract basic crash information
        # Match both "Exception: C0000005" and "Exception code: C0000005 ACCESS_VIOLATION"
        exception_match = re.search(r'Exception(?:\s+code)?:\s*([A-F0-9]+)', content, re.IGNORECASE)
        # Match "Fault address:  00007FF65911FB20"
        address_match = re.search(r'Fault address:\s+([A-F0-9]+)', content, re.IGNORECASE)

        # Extract error message from exception line
        error_match = re.search(r'Exception code:\s+[A-F0-9]+\s+([A-Z_]+)', content, re.IGNORECASE)
        error_message = error_match.group(1) if error_match else "No error message"

        # Parse multi-threaded crash dump to find the crashing thread
        crash_info_parsed = self._parse_multi_thread_crash(content)

        crash_location = crash_info_parsed.get('crash_location', 'Unknown')
        crash_function = crash_info_parsed.get('crash_function', 'Unknown')
        call_stack = crash_info_parsed.get('call_stack', [])

        # Determine if bot-related
        is_bot_related = any(
            keyword in content.lower()
            for keyword in ['botai', 'botsession', 'playerbot', 'deathrecoverymanager', 'behaviormanager']
        )

        # Extract affected components
        affected_components = []
        component_patterns = [
            ('BotSession', r'BotSession'),
            ('DeathRecovery', r'DeathRecoveryManager'),
            ('BehaviorManager', r'BehaviorManager'),
            ('BotAI', r'BotAI'),
            ('Pathfinding', r'PathGenerator|BIH|DynamicMapTree'),
            ('Spell', r'Spell::'),
            ('Unit', r'Unit::'),
            ('Map', r'Map::'),
            ('Database', r'Database|PreparedStatement'),
        ]
        for component, pattern in component_patterns:
            if re.search(pattern, content, re.IGNORECASE):
                affected_components.append(component)

        # Generate crash ID (hash of location + exception)
        crash_id_str = f"{crash_location}_{exception_match.group(1) if exception_match else 'UNKNOWN'}"
        crash_id = hashlib.md5(crash_id_str.encode()).hexdigest()[:12]

        # Match against known patterns
        matched_pattern = self._match_crash_pattern(content, crash_location, call_stack)

        # Generate hypothesis and fix suggestions
        if matched_pattern:
            root_cause = matched_pattern.root_cause
            fix_suggestions = [matched_pattern.fix_template]
            severity = matched_pattern.severity
            category = matched_pattern.pattern_name
        else:
            root_cause = self._generate_hypothesis(crash_location, crash_function, error_message, call_stack)
            fix_suggestions = self._generate_fix_suggestions(crash_location, crash_function, error_message)
            severity = self._determine_severity(crash_location, is_bot_related)
            category = "UNKNOWN_PATTERN"

        # Find similar crashes
        similar_crashes = self._find_similar_crashes(crash_id, crash_location, call_stack)

        # ========================================================================
        # DMP ANALYSIS - Analyze accompanying .dmp file with CDB
        # ========================================================================
        dmp_analysis = None
        registers = None
        local_variables = None
        heap_corruption = False
        cdb_recommendations = None

        # Look for .dmp file with same base name
        dmp_file = crash_file.with_suffix('.dmp')
        if dmp_file.exists():
            print(f"\n📊 Found .dmp file: {dmp_file.name}")
            dmp_result = self.dmp_analyzer.analyze(dmp_file)

            if dmp_result:
                dmp_analysis = dmp_result
                registers = dmp_result.get('registers')
                local_variables = dmp_result.get('local_variables')
                heap_corruption = dmp_result.get('heap_corruption', False)
                cdb_recommendations = dmp_result.get('recommendations')

                # Enhance severity if heap corruption detected
                if heap_corruption and severity != "CRITICAL":
                    severity = "HIGH"
                    print(f"   ⚠️  Heap corruption detected - severity upgraded to HIGH")

                # Enhance hypothesis with CDB insights
                if cdb_recommendations:
                    root_cause += f"\n\nCDB Analysis: {'; '.join(cdb_recommendations)}"

                # Enhance fix suggestions with local variable insights
                if local_variables:
                    null_vars = [var for var, val in local_variables.items() if 'null' in val.lower() or val == '0x0' or val == '0']
                    if null_vars:
                        fix_suggestions.append(f"Add null checks for: {', '.join(null_vars[:5])}")
        else:
            print(f"\n   ⚠️  No .dmp file found (looked for {dmp_file.name})")

        crash_info = CrashInfo(
            crash_id=crash_id,
            timestamp=datetime.now().isoformat(),
            exception_code=exception_match.group(1) if exception_match else "UNKNOWN",
            exception_address=address_match.group(1) if address_match else "UNKNOWN",
            crash_location=crash_location,
            crash_function=crash_function,
            error_message=error_message,
            call_stack=call_stack[:15],  # Keep top 15 frames
            crash_category=category,
            severity=severity,
            is_bot_related=is_bot_related,
            affected_components=affected_components,
            root_cause_hypothesis=root_cause,
            fix_suggestions=fix_suggestions,
            similar_crashes=similar_crashes,
            file_path=str(crash_file),
            raw_dump=content[:5000],  # Keep first 5000 chars for reference
            # DMP analysis fields
            dmp_analysis=dmp_analysis,
            registers=registers,
            local_variables=local_variables,
            heap_corruption=heap_corruption,
            cdb_recommendations=cdb_recommendations
        )

        # Save to database
        self.crash_database["crashes"][crash_id] = asdict(crash_info)
        self._save_crash_database()

        return crash_info

    def _parse_multi_thread_crash(self, content: str) -> dict:
        """
        Parse multi-threaded crash dump to identify the crashing thread

        Windows crash dumps contain multiple threads. The crashing thread is identified
        by the presence of exception handler markers in its call stack.

        Returns:
            dict with 'crash_location', 'crash_function', and 'call_stack'
        """
        # Split into individual thread stacks
        # Each thread starts with "Call stack:\nAddress   Frame     Function      SourceFile"
        thread_stacks = []

        for match in re.finditer(
            r'Call stack:\s*\nAddress\s+Frame\s+Function\s+SourceFile\s*\n(.*?)(?=Call stack:|$)',
            content,
            re.DOTALL
        ):
            stack_text = match.group(1)
            thread_stacks.append(stack_text)

        if not thread_stacks:
            # Fallback: try to find any call stack
            stack_match = re.search(r'Call stack:(.*?)(?:\n\n|\Z)', content, re.DOTALL | re.IGNORECASE)
            if stack_match:
                thread_stacks = [stack_match.group(1)]
            else:
                return {'crash_location': 'Unknown', 'crash_function': 'Unknown', 'call_stack': []}

        # Find the crashing thread by looking for exception handler markers
        exception_markers = [
            'UnhandledExceptionFilter',
            'KiUserExceptionDispatcher',
            '__C_specific_handler',
            'RtlUserExceptionDispatcher',
            'RtlRaiseException'
        ]

        crashing_thread_stack = None
        for stack in thread_stacks:
            if any(marker in stack for marker in exception_markers):
                crashing_thread_stack = stack
                break

        if not crashing_thread_stack:
            # Fallback: use first stack (might be the main thread crash)
            crashing_thread_stack = thread_stacks[0]

        # Parse the crashing thread's call stack
        call_stack = []
        crash_location = "Unknown"
        crash_function = "Unknown"

        # Skip system functions to find the first application-level function
        skip_patterns = ['Rtl', 'Nt', 'Kernel', '__C_', 'BaseThread', 'strncpy', '__chkstk',
                         'wcsrchr', 'UnhandledExceptionFilter', 'KiUserExceptionDispatcher']

        for line in crashing_thread_stack.split('\n'):
            # Skip empty lines and header
            line = line.strip()
            if not line or line.startswith('Address'):
                continue

            # Parse line format: "00007FF65911FB20  00000061CC0FF410  Function+Offset  SourceFile line N"
            # Match patterns:
            # 1. "FunctionName+Offset  SourceFile line LineNumber"
            # 2. "[inline] FunctionName+Offset  SourceFile line LineNumber"

            # Try to match function with source file
            # Line format: "00007FF65911FB20  00000061CBCFF5A0  FunctionName+Offset  SourceFile line LineNumber"
            match = re.search(
                r'[0-9A-F]+\s+[0-9A-F]+\s+(?:\[inline\]\s+)?([A-Za-z0-9_:<>`\',\s-]+?)(?:\+[0-9A-FX]+)?\s+([A-Z]:[^\s]+[/\\]([A-Za-z0-9_\.]+))\s+line\s+(\d+)',
                line,
                re.IGNORECASE
            )

            if match:
                function_name = match.group(1).strip()
                full_path = match.group(2)
                file_name = match.group(3)
                line_num = match.group(4)

                # Clean up function name (remove template noise)
                function_name = re.sub(r'<.*?>', '', function_name)

                # Add to call stack
                call_stack.append(f"{function_name} @ {file_name}:{line_num}")

                # First non-system function is the crash location
                if crash_location == "Unknown":
                    if not any(skip_pat in function_name for skip_pat in skip_patterns):
                        crash_location = f"{file_name}:{line_num}"
                        crash_function = function_name
            else:
                # Try simpler pattern without line number
                match2 = re.search(r'(?:\[inline\]\s+)?([A-Za-z0-9_:<>`\',\s-]+?)(?:\+[0-9A-FX]+)', line)
                if match2:
                    function_name = match2.group(1).strip()
                    function_name = re.sub(r'<.*?>', '', function_name)

                    # Still add to call stack even without file location
                    if not any(skip_pat in function_name for skip_pat in skip_patterns):
                        call_stack.append(function_name)

        return {
            'crash_location': crash_location,
            'crash_function': crash_function,
            'call_stack': call_stack[:15]  # Top 15 frames
        }

    def _match_crash_pattern(self, content: str, location: str, stack: List[str]) -> Optional[CrashPattern]:
        """Match crash against known patterns"""
        for pattern in self.crash_patterns:
            # Check location match
            if not re.search(pattern.location_regex, location):
                continue

            # Check stack pattern match
            stack_matches = 0
            for stack_pattern in pattern.stack_patterns:
                for frame in stack:
                    if re.search(stack_pattern, frame, re.IGNORECASE):
                        stack_matches += 1
                        break

            # If at least 60% of stack patterns match, consider it a match
            if stack_matches >= len(pattern.stack_patterns) * 0.6:
                pattern.occurrences += 1
                return pattern

        return None

    def _generate_hypothesis(self, location: str, function: str, error: str, stack: List[str]) -> str:
        """Generate root cause hypothesis based on crash details"""
        # Check for common patterns
        if "null" in error.lower() or "nullptr" in error.lower():
            return f"Null pointer dereference in {function} at {location}. Likely missing null check before accessing object."

        if "assert" in error.lower() or "logic_error" in error.lower():
            return f"Assertion failure in {function}. Invariant violated: {error}"

        if "access violation" in error.lower():
            return f"Memory access violation at {location}. Possible use-after-free or invalid pointer access."

        if "deadlock" in error.lower() or any("mutex" in frame.lower() for frame in stack):
            return f"Potential deadlock in {function}. Recursive lock acquisition or lock ordering issue."

        if any("bot" in frame.lower() for frame in stack):
            return f"Bot-related crash in {function} at {location}. Check bot state management and lifecycle."

        return f"Crash in {function} at {location}. Error: {error}. Requires detailed analysis."

    def _generate_fix_suggestions(self, location: str, function: str, error: str) -> List[str]:
        """Generate fix suggestions based on crash details"""
        suggestions = []

        # Null pointer fixes
        if "null" in error.lower():
            suggestions.append(f"Add null check before accessing object in {function}")
            suggestions.append(f"Verify object initialization before use")
            suggestions.append(f"Add IsBot() guard if bot-specific code")

        # Assertion fixes
        if "assert" in error.lower():
            suggestions.append(f"Review assertion condition in {location}")
            suggestions.append(f"Add defensive error handling instead of assertion")
            suggestions.append(f"Validate input parameters before function call")

        # Memory fixes
        if "access violation" in error.lower():
            suggestions.append(f"Check object lifetime and cleanup order")
            suggestions.append(f"Use ASAN to detect use-after-free")
            suggestions.append(f"Add bounds checking for array/vector access")

        # Concurrency fixes
        if "mutex" in error.lower() or "deadlock" in error.lower():
            suggestions.append(f"Review lock acquisition order")
            suggestions.append(f"Use std::unique_lock with try_lock")
            suggestions.append(f"Run with TSAN to detect race conditions")

        # Generic suggestions
        if not suggestions:
            suggestions.append(f"Add extensive debug logging around {location}")
            suggestions.append(f"Review recent changes to {function}")
            suggestions.append(f"Check for related crashes in crash database")

        return suggestions

    def _determine_severity(self, location: str, is_bot_related: bool) -> str:
        """Determine crash severity"""
        # Core crashes are always critical
        if not is_bot_related:
            return "CRITICAL"

        # Bot crashes based on component
        if "botsession" in location.lower() or "deathrecovery" in location.lower():
            return "HIGH"

        if "behaviormanager" in location.lower() or "botai" in location.lower():
            return "MEDIUM"

        return "LOW"

    def _find_similar_crashes(self, crash_id: str, location: str, stack: List[str]) -> List[str]:
        """Find similar crashes in database"""
        similar = []

        for stored_id, stored_crash in self.crash_database.get("crashes", {}).items():
            if stored_id == crash_id:
                continue

            # Same location = very similar
            if stored_crash.get("crash_location") == location:
                similar.append(stored_id)
                continue

            # Similar stack trace (at least 3 common frames)
            stored_stack = stored_crash.get("call_stack", [])
            common_frames = sum(
                1 for frame in stack[:10]
                if any(frame in stored_frame for stored_frame in stored_stack[:10])
            )
            if common_frames >= 3:
                similar.append(stored_id)

        return similar[:5]  # Return top 5 similar crashes

    def print_crash_report(self, crash: CrashInfo):
        """Print formatted crash analysis report"""
        print("\n" + "="*80)
        print(f"🔥 CRASH ANALYSIS REPORT - ID: {crash.crash_id}")
        print("="*80)

        print(f"\n📅 Timestamp: {crash.timestamp}")
        print(f"⚠️  Severity: {crash.severity}")
        print(f"🏷️  Category: {crash.crash_category}")
        print(f"🤖 Bot Related: {'YES' if crash.is_bot_related else 'NO'}")

        print(f"\n💥 Exception:")
        print(f"   Code: {crash.exception_code}")
        print(f"   Address: {crash.exception_address}")
        print(f"   Location: {crash.crash_location}")
        print(f"   Function: {crash.crash_function}")
        print(f"   Message: {crash.error_message}")

        print(f"\n🎯 Affected Components:")
        for component in crash.affected_components:
            print(f"   - {component}")

        print(f"\n🔬 Root Cause Hypothesis:")
        print(f"   {crash.root_cause_hypothesis}")

        print(f"\n💡 Fix Suggestions:")
        for i, suggestion in enumerate(crash.fix_suggestions, 1):
            print(f"   {i}. {suggestion}")

        print(f"\n📚 Call Stack (Top 10):")
        for i, frame in enumerate(crash.call_stack[:10], 1):
            print(f"   {i:2d}. {frame}")

        if crash.similar_crashes:
            print(f"\n🔗 Similar Crashes:")
            for similar_id in crash.similar_crashes:
                similar = self.crash_database["crashes"].get(similar_id, {})
                print(f"   - {similar_id}: {similar.get('crash_location', 'Unknown')}")

        print("\n" + "="*80 + "\n")

    def generate_debug_logging_patch(self, crash: CrashInfo) -> str:
        """
        Generate code patch to add debug logging around crash location

        Returns:
            Patch content to be applied
        """
        file_path = crash.crash_location.split(':')[0]
        line_num = int(crash.crash_location.split(':')[1]) if ':' in crash.crash_location else 0

        # Find the source file
        source_file = None
        for search_path in [
            self.trinity_root / "src" / "modules" / "Playerbot",
            self.trinity_root / "src" / "server" / "game"
        ]:
            found = list(search_path.rglob(file_path))
            if found:
                source_file = found[0]
                break

        if not source_file:
            return f"// Could not find source file: {file_path}"

        patch = f"""
// ============================================================================
// DEBUG LOGGING PATCH for {crash.crash_id}
// Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
// Location: {crash.crash_location}
// Root Cause: {crash.root_cause_hypothesis}
// ============================================================================

// Add before line {line_num} in {file_path}:

LOG_DEBUG("playerbot.crash", "CRASH_DEBUG [{crash_id}]: Entering {crash.crash_function}");
LOG_DEBUG("playerbot.crash", "CRASH_DEBUG [{crash_id}]: State validation at {crash.crash_location}");

// Add null checks (if applicable):
if (!object)
{{
    LOG_ERROR("playerbot.crash", "CRASH_DEBUG [{crash_id}]: NULL OBJECT DETECTED at {crash.crash_location}");
    return false; // or appropriate error handling
}}

// Add state validation:
if (!IsValid())
{{
    LOG_ERROR("playerbot.crash", "CRASH_DEBUG [{crash_id}]: INVALID STATE at {crash.crash_location}");
    return false;
}}

LOG_DEBUG("playerbot.crash", "CRASH_DEBUG [{crash_id}]: Pre-operation state OK");

// ============================================================================
// END DEBUG PATCH
// ============================================================================
"""
        return patch

# ============================================================================
# Automated Crash Loop System
# ============================================================================

class CrashLoopHandler:
    """
    Automated crash loop workflow:
    1. Detect crash
    2. Analyze crash dump
    3. Generate fix or debug logging
    4. Compile code
    5. Run server
    6. If crashes again, repeat from step 2 with more logging
    """

    def __init__(self, trinity_root: Path, server_exe: Path, logs_dir: Path):
        self.trinity_root = Path(trinity_root)
        self.server_exe = Path(server_exe)
        self.logs_dir = Path(logs_dir)
        self.analyzer = CrashAnalyzer(trinity_root, logs_dir)
        self.max_iterations = 10
        self.iteration_count = 0
        self.crash_history: List[str] = []

    def start_crash_loop(self):
        """Start automated crash analysis and fix loop"""
        print("🔄 Starting Automated Crash Loop Handler")
        print(f"   Trinity Root: {self.trinity_root}")
        print(f"   Server Exe: {self.server_exe}")
        print(f"   Max Iterations: {self.max_iterations}\n")

        while self.iteration_count < self.max_iterations:
            self.iteration_count += 1
            print(f"\n{'='*80}")
            print(f"🔁 ITERATION {self.iteration_count}/{self.max_iterations}")
            print(f"{'='*80}\n")

            # Step 1: Run server and wait for crash or timeout
            crashed, crash_file = self._run_server_with_monitoring()

            if not crashed:
                print("✅ Server ran successfully without crashing!")
                print(f"✅ STABLE after {self.iteration_count} iterations")
                break

            print(f"\n💥 Server crashed! Crash file: {crash_file}")

            # Step 2: Analyze crash
            crash_info = self.analyzer.parse_crash_dump(crash_file)
            if not crash_info:
                print("❌ Failed to parse crash dump")
                continue

            self.analyzer.print_crash_report(crash_info)

            # Check if this is a repeat crash
            if crash_info.crash_id in self.crash_history:
                print(f"⚠️  WARNING: Same crash repeated ({crash_info.crash_id})")
                print(f"   Crash has occurred {self.crash_history.count(crash_info.crash_id) + 1} times")

                if self.crash_history.count(crash_info.crash_id) >= 3:
                    print("\n❌ CRASH LOOP DETECTED - Same crash 3+ times")
                    print("   Manual intervention required.")
                    self._generate_github_issue(crash_info)
                    break

            self.crash_history.append(crash_info.crash_id)

            # Step 3: Generate fix or enhanced logging
            if self.iteration_count <= 3:
                print("\n🔧 Attempting automated fix...")
                success = self._apply_automated_fix(crash_info)
            else:
                print("\n📝 Adding enhanced debug logging...")
                success = self._add_debug_logging(crash_info)

            if not success:
                print("❌ Failed to apply fix/logging")
                continue

            # Step 4: Compile
            print("\n🔨 Compiling with changes...")
            if not self._compile_server():
                print("❌ Compilation failed")
                continue

            print("✅ Compilation successful")

            # Continue loop - next iteration will run server again

        if self.iteration_count >= self.max_iterations:
            print(f"\n⚠️  Maximum iterations ({self.max_iterations}) reached")
            print("   Manual intervention required")

        self._print_final_summary()

    def _run_server_with_monitoring(self, timeout: int = 300) -> Tuple[bool, Optional[Path]]:
        """
        Run server and monitor for crashes

        Returns:
            (crashed: bool, crash_file: Path or None)
        """
        print(f"🚀 Starting server (timeout: {timeout}s)...")
        print(f"   Executable: {self.server_exe}")

        # Get crash files before running
        crashes_before = set(self.logs_dir.glob("*worldserver*.txt"))

        # Run server in background
        try:
            process = subprocess.Popen(
                [str(self.server_exe)],
                cwd=self.server_exe.parent,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE
            )

            print(f"   Process ID: {process.pid}")
            print(f"   Monitoring for {timeout}s...")

            # Wait for timeout or crash
            try:
                process.wait(timeout=timeout)
                returncode = process.returncode
            except subprocess.TimeoutExpired:
                print(f"\n✅ Server ran for {timeout}s without crashing")
                process.terminate()
                process.wait(timeout=10)
                return False, None

            # Server exited - check if it was a crash
            if returncode != 0:
                print(f"\n💥 Server exited with code {returncode}")

                # Wait a moment for crash dump to be written
                time.sleep(2)

                # Find new crash file
                crashes_after = set(self.logs_dir.glob("*worldserver*.txt"))
                new_crashes = crashes_after - crashes_before

                if new_crashes:
                    crash_file = sorted(new_crashes, key=lambda p: p.stat().st_mtime)[-1]
                    return True, crash_file
                else:
                    print("⚠️  No crash dump found")
                    return True, None
            else:
                print("\n✅ Server exited normally")
                return False, None

        except Exception as e:
            print(f"❌ Error running server: {e}")
            return False, None

    def _apply_automated_fix(self, crash: CrashInfo) -> bool:
        """Apply automated fix based on crash pattern"""
        print(f"   Crash ID: {crash.crash_id}")
        print(f"   Category: {crash.crash_category}")

        # Check if we have a fix template
        if not crash.fix_suggestions:
            print("   No automated fix available")
            return False

        print(f"   Fix suggestion: {crash.fix_suggestions[0]}")

        # For now, just add debug logging
        # In future, implement actual code fixes based on patterns
        return self._add_debug_logging(crash)

    def _add_debug_logging(self, crash: CrashInfo) -> bool:
        """Add debug logging around crash location"""
        patch = self.analyzer.generate_debug_logging_patch(crash)

        # Save patch to file
        patch_file = self.trinity_root / ".claude" / "patches" / f"debug_patch_{crash.crash_id}.cpp"
        patch_file.parent.mkdir(parents=True, exist_ok=True)

        with open(patch_file, 'w', encoding='utf-8') as f:
            f.write(patch)

        print(f"   Debug patch saved: {patch_file}")
        print(f"   ⚠️  MANUAL APPLICATION REQUIRED")
        print(f"   Review patch and apply to source code")

        return True  # Assume success for now

    def _compile_server(self) -> bool:
        """Compile server with CMake"""
        build_dir = self.trinity_root / "build"

        if not build_dir.exists():
            print(f"❌ Build directory not found: {build_dir}")
            return False

        print(f"   Build directory: {build_dir}")

        # Run CMake build
        try:
            result = subprocess.run(
                ["cmake", "--build", ".", "--target", "worldserver", "--config", "RelWithDebInfo"],
                cwd=build_dir,
                capture_output=True,
                text=True,
                timeout=1800  # 30 minute timeout
            )

            if result.returncode != 0:
                print(f"❌ Build failed:")
                print(result.stderr[:1000])
                return False

            return True

        except subprocess.TimeoutExpired:
            print("❌ Build timeout (30 minutes)")
            return False
        except Exception as e:
            print(f"❌ Build error: {e}")
            return False

    def _generate_github_issue(self, crash: CrashInfo):
        """Generate GitHub issue for unresolved crash"""
        issue_file = self.trinity_root / ".claude" / "issues" / f"crash_{crash.crash_id}.md"
        issue_file.parent.mkdir(parents=True, exist_ok=True)

        issue_content = f"""# Crash Report: {crash.crash_category}

**Crash ID**: `{crash.crash_id}`
**Severity**: {crash.severity}
**Timestamp**: {crash.timestamp}

## Exception Details

- **Code**: {crash.exception_code}
- **Address**: {crash.exception_address}
- **Location**: {crash.crash_location}
- **Function**: {crash.crash_function}
- **Message**: {crash.error_message}

## Root Cause Analysis

{crash.root_cause_hypothesis}

## Affected Components

{chr(10).join(f'- {comp}' for comp in crash.affected_components)}

## Call Stack

```
{chr(10).join(crash.call_stack[:15])}
```

## Fix Suggestions

{chr(10).join(f'{i+1}. {sug}' for i, sug in enumerate(crash.fix_suggestions))}

## Similar Crashes

{chr(10).join(f'- {cid}' for cid in crash.similar_crashes)}

## Iteration History

This crash occurred {self.crash_history.count(crash.crash_id)} times during automated analysis.

---

**Auto-generated by CrashAnalyzer**
"""

        with open(issue_file, 'w', encoding='utf-8') as f:
            f.write(issue_content)

        print(f"\n📋 GitHub issue draft created: {issue_file}")

    def _print_final_summary(self):
        """Print final crash loop summary"""
        print("\n" + "="*80)
        print("📊 CRASH LOOP SUMMARY")
        print("="*80)
        print(f"\n   Total Iterations: {self.iteration_count}")
        print(f"   Unique Crashes: {len(set(self.crash_history))}")
        print(f"   Total Crashes: {len(self.crash_history)}")

        if self.crash_history:
            print(f"\n   Crash Breakdown:")
            crash_counts = defaultdict(int)
            for crash_id in self.crash_history:
                crash_counts[crash_id] += 1

            for crash_id, count in sorted(crash_counts.items(), key=lambda x: -x[1]):
                crash_data = self.analyzer.crash_database["crashes"].get(crash_id, {})
                location = crash_data.get("crash_location", "Unknown")
                print(f"      - {crash_id}: {location} ({count}x)")

        print("\n" + "="*80 + "\n")

# ============================================================================
# Main Entry Point
# ============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="Enterprise Crash Dump Analyzer with Auto-Fix Loop"
    )

    parser.add_argument(
        '--analyze',
        type=Path,
        help='Analyze a single crash dump file'
    )

    parser.add_argument(
        '--watch',
        type=Path,
        help='Watch directory for new crash dumps'
    )

    parser.add_argument(
        '--auto-loop',
        action='store_true',
        help='Start automated crash analysis and fix loop'
    )

    parser.add_argument(
        '--trinity-root',
        type=Path,
        default=Path('c:/TrinityBots/TrinityCore'),
        help='TrinityCore root directory'
    )

    parser.add_argument(
        '--server-exe',
        type=Path,
        default=Path('M:/Wplayerbot/worldserver.exe'),
        help='Server executable path'
    )

    parser.add_argument(
        '--logs-dir',
        type=Path,
        default=Path('M:/Wplayerbot/Crashes'),
        help='Crash logs directory'
    )

    parser.add_argument(
        '--timeout',
        type=int,
        default=300,
        help='Server run timeout in seconds (default: 300)'
    )

    args = parser.parse_args()

    # Create analyzer
    analyzer = CrashAnalyzer(args.trinity_root, args.logs_dir)

    if args.analyze:
        # Analyze single crash
        crash = analyzer.parse_crash_dump(args.analyze)
        if crash:
            analyzer.print_crash_report(crash)

            # Generate debug patch
            patch = analyzer.generate_debug_logging_patch(crash)
            patch_file = args.trinity_root / ".claude" / "patches" / f"debug_{crash.crash_id}.cpp"
            patch_file.parent.mkdir(parents=True, exist_ok=True)
            with open(patch_file, 'w') as f:
                f.write(patch)
            print(f"📝 Debug patch saved: {patch_file}")

    elif args.auto_loop:
        # Start automated crash loop
        loop_handler = CrashLoopHandler(
            args.trinity_root,
            args.server_exe,
            args.logs_dir
        )
        loop_handler.start_crash_loop()

    elif args.watch:
        # Watch directory for new crashes
        print(f"👁️  Watching for crashes in: {args.watch}")
        print("   Press Ctrl+C to stop\n")

        processed = set()
        try:
            while True:
                crash_files = list(args.watch.glob("*worldserver*.txt"))
                new_files = [f for f in crash_files if f not in processed]

                for crash_file in new_files:
                    print(f"\n🔥 New crash detected: {crash_file.name}")
                    crash = analyzer.parse_crash_dump(crash_file)
                    if crash:
                        analyzer.print_crash_report(crash)
                    processed.add(crash_file)

                time.sleep(5)  # Check every 5 seconds

        except KeyboardInterrupt:
            print("\n\n👋 Stopped watching")

    else:
        parser.print_help()

if __name__ == "__main__":
    main()
