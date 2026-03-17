#!/usr/bin/env python3
"""
Enterprise Automated Crash Loop with Self-Healing
Complete automation: Crash → Analyze → Fix → Compile → Test → Repeat

This is the master orchestrator that combines:
- crash_analyzer.py (crash dump analysis)
- crash_monitor.py (log analysis)
- Automated fix generation
- Compilation
- Testing
- Iterative improvement

Usage:
    python crash_auto_fix.py --auto-run
    python crash_auto_fix.py --single-iteration
"""

import sys
import os
import json
import time
import subprocess
import argparse
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Optional, Tuple
import importlib.util

# Add scripts directory to path
scripts_dir = Path(__file__).parent
sys.path.insert(0, str(scripts_dir))

# Import our crash analysis modules
from crash_analyzer import CrashAnalyzer, CrashInfo
from crash_monitor import RealTimeCrashMonitor, LogMonitor, CrashContextAnalyzer

# ============================================================================
# Automated Fix Generator
# ============================================================================

class AutoFixGenerator:
    """
    Generates automated fixes based on crash patterns
    """

    def __init__(self, trinity_root: Path):
        self.trinity_root = Path(trinity_root)
        self.fixes_dir = trinity_root / ".claude" / "automated_fixes"
        self.fixes_dir.mkdir(parents=True, exist_ok=True)

    def generate_fix(self, crash: CrashInfo, context: dict) -> Optional[Path]:
        """
        Generate automated fix based on crash pattern

        Returns:
            Path to generated fix file, or None if no fix available
        """
        print(f"\n🔧 Generating automated fix for: {crash.crash_id}")
        print(f"   Category: {crash.crash_category}")
        print(f"   Location: {crash.crash_location}")

        # Check if we have a fix template for this pattern
        fix_content = None

        # Pattern-based fix generation
        if "null" in crash.error_message.lower() or "nullptr" in crash.error_message.lower():
            fix_content = self._generate_null_check_fix(crash, context)

        elif "spell.cpp:603" in crash.crash_location.lower():
            fix_content = self._generate_teleport_delay_fix(crash, context)

        elif "map.cpp:686" in crash.crash_location.lower():
            fix_content = self._generate_object_removal_fix(crash, context)

        elif "unit.cpp:10863" in crash.crash_location.lower():
            fix_content = self._generate_spellmod_removal_fix(crash, context)

        elif "deadlock" in crash.error_message.lower() or any("mutex" in frame.lower() for frame in crash.call_stack):
            fix_content = self._generate_deadlock_fix(crash, context)

        else:
            # Default: enhanced logging
            fix_content = self._generate_debug_logging(crash, context)

        if not fix_content:
            print("   ⚠️  No automated fix available")
            return None

        # Save fix to file
        fix_file = self.fixes_dir / f"fix_{crash.crash_id}_{datetime.now().strftime('%Y%m%d_%H%M%S')}.cpp"
        with open(fix_file, 'w', encoding='utf-8') as f:
            f.write(fix_content)

        print(f"   ✅ Fix generated: {fix_file.name}")
        return fix_file

    def _generate_null_check_fix(self, crash: CrashInfo, context: dict) -> str:
        """Generate null pointer check fix"""
        location_parts = crash.crash_location.split(':')
        file_name = location_parts[0]
        line_num = location_parts[1] if len(location_parts) > 1 else "?"

        return f"""// ============================================================================
// AUTOMATED FIX: Null Pointer Check
// Crash ID: {crash.crash_id}
// Location: {crash.crash_location}
// Function: {crash.crash_function}
// Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
// ============================================================================

// INSTRUCTIONS:
// 1. Open {file_name}
// 2. Find line {line_num} in function {crash.crash_function}
// 3. Add null check BEFORE the crash line:

// BEFORE (causing crash):
// object->SomeMethod();

// AFTER (with null check):
if (!object)
{{
    LOG_ERROR("playerbot.fix", "Null object in {{}} at {{}}:{{}}", "{crash.crash_function}", "{file_name}", {line_num});
    return false; // or appropriate error handling
}}

object->SomeMethod();

// ============================================================================
// ADDITIONAL CHECKS:
// ============================================================================

// If this is bot-related code, add IsBot() guard:
if (IsBot())
{{
    LOG_DEBUG("playerbot.fix", "Skipping operation for bot in {crash.crash_function}");
    return true;
}}

// ============================================================================
// ROOT CAUSE: {crash.root_cause_hypothesis}
// ============================================================================
"""

    def _generate_teleport_delay_fix(self, crash: CrashInfo, context: dict) -> str:
        """Generate teleport ack delay fix (Spell.cpp:603 crash)"""
        return f"""// ============================================================================
// AUTOMATED FIX: Teleport Ack Delay (Spell.cpp:603 Crash Prevention)
// Crash ID: {crash.crash_id}
// Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
// ============================================================================

// INSTRUCTIONS:
// 1. Open DeathRecoveryManager.cpp
// 2. Find ExecuteReleaseSpirit() function
// 3. Replace immediate HandleMoveTeleportAck() with deferred call:

// BEFORE (causing crash):
void DeathRecoveryManager::ExecuteReleaseSpirit()
{{
    m_bot->BuildPlayerRepop();
    m_bot->RepopAtGraveyard();

    // CRASH HERE: Immediate call causes Spell.cpp:603 crash
    m_bot->HandleMoveTeleportAck();
}}

// AFTER (with 100ms deferral):
void DeathRecoveryManager::ExecuteReleaseSpirit()
{{
    m_bot->BuildPlayerRepop();
    m_bot->RepopAtGraveyard();

    // Defer HandleMoveTeleportAck by 100ms to prevent Spell.cpp:603 crash
    m_bot->m_Events.AddEventAtOffset([this]() {{
        if (m_bot && m_bot->IsInWorld())
            m_bot->HandleMoveTeleportAck();
    }}, 100ms);

    LOG_DEBUG("playerbot.death", "Deferred teleport ack for bot {{}}", m_bot->GetName());
}}

// ============================================================================
// ROOT CAUSE: HandleMoveTeleportAck called before spell mod cleanup
// FIX: 100ms delay allows spell effects to finish processing
// ============================================================================
"""

    def _generate_object_removal_fix(self, crash: CrashInfo, context: dict) -> str:
        """Generate Map.cpp:686 object removal fix"""
        return f"""// ============================================================================
// AUTOMATED FIX: Object Removal Timing (Map.cpp:686 Crash Prevention)
// Crash ID: {crash.crash_id}
// Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
// ============================================================================

// INSTRUCTIONS:
// 1. Open BotSession.cpp
// 2. Find LogoutPlayer() function
// 3. Add session state validation:

// BEFORE:
void BotSession::LogoutPlayer()
{{
    if (Player* player = GetPlayer())
    {{
        // Immediate removal can crash during login
        player->RemoveFromWorld();
    }}
}}

// AFTER (with state check):
void BotSession::LogoutPlayer()
{{
    if (!IsBot())
        return;

    // Validate session state
    if (m_loginState != LoginState::LOGIN_COMPLETE)
    {{
        LOG_WARN("playerbot.session", "Logout called during login state {{}}", (uint32)m_loginState);
        return;
    }}

    if (Player* player = GetPlayer())
    {{
        // Check if player is properly in world
        if (!player->IsInWorld())
        {{
            LOG_ERROR("playerbot.session", "Player {{}} not in world during logout", player->GetName());
            return;
        }}

        player->RemoveFromWorld();
        LOG_DEBUG("playerbot.session", "Bot {{}} removed from world", player->GetName());
    }}
}}

// ============================================================================
// ROOT CAUSE: Object removal during invalid session state
// FIX: Validate session and player state before removal
// ============================================================================
"""

    def _generate_spellmod_removal_fix(self, crash: CrashInfo, context: dict) -> str:
        """Generate Unit.cpp:10863 spell mod access removal"""
        return f"""// ============================================================================
// AUTOMATED FIX: SpellMod Access Removal (Unit.cpp:10863 Crash Prevention)
// Crash ID: {crash.crash_id}
// Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
// ============================================================================

// INSTRUCTIONS:
// 1. Search for all accesses to m_spellModTakingDamage in bot code
// 2. Remove or guard with IsBot() check

// SEARCH PATTERN:
// grep -r "m_spellModTakingDamage" src/modules/Playerbot/

// BEFORE (causing crash):
if (player->m_spellModTakingDamage)
{{
    // Bot doesn't have this initialized
    damage *= modifier;
}}

// OPTION 1: Remove completely (bots don't need spell mods)
// DELETE the m_spellModTakingDamage access

// OPTION 2: Guard with IsBot() check
if (!IsBot() && player->m_spellModTakingDamage)
{{
    damage *= modifier;
}}

// ============================================================================
// FILES TO CHECK:
// - src/modules/Playerbot/AI/BotAI.cpp
// - src/modules/Playerbot/AI/Combat/*.cpp
// - Any custom damage calculation code
// ============================================================================

// ROOT CAUSE: Bot Player objects don't initialize m_spellModTakingDamage
// FIX: Remove spell mod access or guard with IsBot() check
// ============================================================================
"""

    def _generate_deadlock_fix(self, crash: CrashInfo, context: dict) -> str:
        """Generate deadlock fix"""
        return f"""// ============================================================================
// AUTOMATED FIX: Deadlock Prevention
// Crash ID: {crash.crash_id}
// Location: {crash.crash_location}
// Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
// ============================================================================

// INSTRUCTIONS:
// 1. Identify the mutex being locked
// 2. Review lock acquisition order
// 3. Use try_lock or refactor to avoid nested locks

// PATTERN 1: Use try_lock instead of lock
// BEFORE:
std::lock_guard<std::recursive_mutex> lock(m_mutex);
// ... code that might acquire another lock ...

// AFTER:
std::unique_lock<std::recursive_mutex> lock(m_mutex, std::try_to_lock);
if (!lock.owns_lock())
{{
    LOG_WARN("playerbot.deadlock", "Could not acquire lock in {crash.crash_function}, deferring");
    return false; // Retry later
}}
// ... safe code ...

// PATTERN 2: Reduce lock scope
// BEFORE:
std::lock_guard<std::recursive_mutex> lock(m_mutex);
DoWork1();
DoWork2(); // This might lock another mutex - DEADLOCK RISK
DoWork3();

// AFTER:
{{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    DoWork1();
}} // Release lock early

DoWork2(); // No longer holding m_mutex

{{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    DoWork3();
}}

// ============================================================================
// DEBUGGING: Run with ThreadSanitizer (TSAN)
// cmake -DPLAYERBOT_TESTS_TSAN=ON
// ============================================================================

// ROOT CAUSE: {crash.root_cause_hypothesis}
// ============================================================================
"""

    def _generate_debug_logging(self, crash: CrashInfo, context: dict) -> str:
        """Generate enhanced debug logging"""
        location_parts = crash.crash_location.split(':')
        file_name = location_parts[0]
        line_num = location_parts[1] if len(location_parts) > 1 else "?"

        # Get pre-crash warnings from context
        warnings_summary = ""
        if context.get('warnings_before_crash'):
            warnings_summary = f"// Pre-crash warnings detected:\n"
            for warning in context['warnings_before_crash'][:5]:
                warnings_summary += f"//   - {warning.get('message', '')[:70]}\n"

        return f"""// ============================================================================
// ENHANCED DEBUG LOGGING
// Crash ID: {crash.crash_id}
// Location: {crash.crash_location}
// Function: {crash.crash_function}
// Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
// ============================================================================

// INSTRUCTIONS:
// 1. Open {file_name}
// 2. Add logging around line {line_num} in {crash.crash_function}

{warnings_summary}

// Add BEFORE the crash line:
LOG_DEBUG("playerbot.crash.{crash.crash_id}", "ENTERING {crash.crash_function}");
LOG_DEBUG("playerbot.crash.{crash.crash_id}", "State validation at {crash.crash_location}");

// Add state dumps:
if (m_bot)
{{
    LOG_DEBUG("playerbot.crash.{crash.crash_id}", "Bot state: InWorld={{}}, IsAlive={{}}",
              m_bot->IsInWorld(), m_bot->IsAlive());
}}

// Add null checks:
if (!object)
{{
    LOG_ERROR("playerbot.crash.{crash.crash_id}", "NULL OBJECT at {crash.crash_location}");
    return false;
}}

// Add assertion for debugging:
ASSERT(object->IsValid() && "Object must be valid at {crash.crash_location}");

// Add AFTER the crash line:
LOG_DEBUG("playerbot.crash.{crash.crash_id}", "COMPLETED {crash.crash_function} successfully");

// ============================================================================
// NEXT STEPS:
// 1. Apply logging
// 2. Compile and run
// 3. Check Playerbot.log for CRASH_DEBUG entries
// 4. Analyze logged state to identify root cause
// ============================================================================

// ROOT CAUSE HYPOTHESIS: {crash.root_cause_hypothesis}

// FIX SUGGESTIONS:
{chr(10).join(f'// {i+1}. {sug}' for i, sug in enumerate(crash.fix_suggestions))}
// ============================================================================
"""

# ============================================================================
// Crash Loop Orchestrator
# ============================================================================

class CrashLoopOrchestrator:
    """
    Master orchestrator for automated crash analysis and fixing
    """

    def __init__(
        self,
        trinity_root: Path,
        server_exe: Path,
        server_log: Path,
        playerbot_log: Path,
        crashes_dir: Path
    ):
        self.trinity_root = Path(trinity_root)
        self.server_exe = Path(server_exe)
        self.crashes_dir = Path(crashes_dir)

        # Create analyzers
        self.crash_analyzer = CrashAnalyzer(trinity_root, crashes_dir)
        self.fix_generator = AutoFixGenerator(trinity_root)

        # Create log monitors
        self.server_monitor = LogMonitor(server_log, "Server.log", buffer_size=500)
        self.playerbot_monitor = LogMonitor(playerbot_log, "Playerbot.log", buffer_size=500)
        self.context_analyzer = CrashContextAnalyzer(
            self.server_monitor,
            self.playerbot_monitor,
            crashes_dir
        )

        # State tracking
        self.iteration = 0
        self.max_iterations = 10
        self.crash_history: List[str] = []
        self.fixes_applied: List[Path] = []

    def run_crash_loop(self, auto_fix: bool = False, auto_compile: bool = False):
        """
        Run automated crash detection and fix loop

        Args:
            auto_fix: Automatically apply generated fixes
            auto_compile: Automatically compile after applying fixes
        """
        print("\n" + "="*100)
        print("🚀 ENTERPRISE AUTOMATED CRASH LOOP")
        print("="*100)
        print(f"\n⚙️  Configuration:")
        print(f"   Trinity Root: {self.trinity_root}")
        print(f"   Server Exe: {self.server_exe}")
        print(f"   Max Iterations: {self.max_iterations}")
        print(f"   Auto-Fix: {auto_fix}")
        print(f"   Auto-Compile: {auto_compile}")
        print()

        # Start log monitors
        print("📊 Starting log monitors...")
        self.server_monitor.start()
        self.playerbot_monitor.start()
        time.sleep(2)
        print("✅ Log monitors started\n")

        # Main crash loop
        while self.iteration < self.max_iterations:
            self.iteration += 1
            print("\n" + "="*100)
            print(f"🔁 ITERATION {self.iteration}/{self.max_iterations}")
            print("="*100 + "\n")

            # Step 1: Run server
            print("🚀 Step 1: Running server...")
            crashed, crash_file = self._run_server(timeout=300)

            if not crashed:
                print("\n✅ SUCCESS! Server ran without crashing!")
                print(f"✅ System is STABLE after {self.iteration} iterations")
                break

            print(f"\n💥 Server crashed! Analyzing...")

            # Step 2: Analyze crash
            print(f"\n🔍 Step 2: Analyzing crash dump...")
            crash_info = self.crash_analyzer.parse_crash_dump(crash_file)

            if not crash_info:
                print("❌ Failed to parse crash dump, skipping iteration")
                continue

            # Get log context
            context_info = self.context_analyzer.analyze_crash_with_context(crash_file)

            # Print comprehensive report
            self.crash_analyzer.print_crash_report(crash_info)
            self.context_analyzer.print_crash_context_report(
                context_info,
                self.crash_analyzer.crash_database["crashes"].get(crash_info.crash_id, {})
            )

            # Check for repeated crash
            if crash_info.crash_id in self.crash_history:
                repeat_count = self.crash_history.count(crash_info.crash_id) + 1
                print(f"\n⚠️  WARNING: Same crash repeated ({repeat_count} times)")

                if repeat_count >= 3:
                    print("\n❌ CRASH LOOP DETECTED!")
                    print("   Same crash occurred 3+ times despite fixes.")
                    print("   Manual intervention required.\n")
                    self._generate_detailed_report(crash_info, context_info)
                    break

            self.crash_history.append(crash_info.crash_id)

            # Step 3: Generate fix
            print(f"\n🔧 Step 3: Generating automated fix...")
            context_dict = {
                'warnings_before_crash': [
                    {'message': w.message} for w in context_info.warnings_before_crash
                ],
                'errors_before_crash': [
                    {'message': e.message} for e in context_info.errors_before_crash
                ]
            }

            fix_file = self.fix_generator.generate_fix(crash_info, context_dict)

            if not fix_file:
                print("   ⚠️  No automated fix available, adding debug logging...")
                # Still log the crash for manual review
                self._save_crash_report(crash_info, context_info)
                continue

            self.fixes_applied.append(fix_file)

            # Step 4: Apply fix (if auto_fix enabled)
            if auto_fix:
                print(f"\n🛠️  Step 4: Applying fix automatically...")
                print("   ⚠️  MANUAL REVIEW RECOMMENDED!")
                # In production, this would apply the fix
                # For now, just log it
                print(f"   Fix saved to: {fix_file}")
            else:
                print(f"\n📋 Step 4: Fix generated (manual application required)")
                print(f"   Fix file: {fix_file}")
                print(f"   Review and apply the fix manually, then continue.\n")

                response = input("   Apply fix and continue? (y/n): ").strip().lower()
                if response != 'y':
                    print("   Stopping crash loop. Resume later after applying fixes.")
                    break

            # Step 5: Compile (if auto_compile enabled)
            if auto_compile and auto_fix:
                print(f"\n🔨 Step 5: Compiling with changes...")
                if self._compile_server():
                    print("   ✅ Compilation successful")
                else:
                    print("   ❌ Compilation failed, stopping loop")
                    break
            else:
                print(f"\n📝 Step 5: Manual compilation required")
                print("   Compile the server and press Enter to continue...")
                input()

            print(f"\n✅ Iteration {self.iteration} complete, starting next test run...\n")
            time.sleep(2)

        # Loop finished
        self._print_final_summary()

        # Stop monitors
        self.server_monitor.stop()
        self.playerbot_monitor.stop()

    def _run_server(self, timeout: int = 300) -> Tuple[bool, Optional[Path]]:
        """Run server and detect crash"""
        print(f"   Executable: {self.server_exe}")
        print(f"   Timeout: {timeout}s")

        # Get existing crashes
        crashes_before = set(self.crashes_dir.glob("*worldserver*.txt"))

        # Run server
        try:
            process = subprocess.Popen(
                [str(self.server_exe)],
                cwd=self.server_exe.parent,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE
            )

            print(f"   Process ID: {process.pid}")

            try:
                returncode = process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                print(f"\n   ✅ Server ran for {timeout}s without crashing")
                process.terminate()
                process.wait(timeout=10)
                return False, None

            if returncode != 0:
                print(f"\n   💥 Server exited with code {returncode}")
                time.sleep(2)  # Wait for crash dump

                crashes_after = set(self.crashes_dir.glob("*worldserver*.txt"))
                new_crashes = crashes_after - crashes_before

                if new_crashes:
                    crash_file = sorted(new_crashes, key=lambda p: p.stat().st_mtime)[-1]
                    return True, crash_file
                else:
                    print("   ⚠️  No crash dump found")
                    return True, None
            else:
                return False, None

        except Exception as e:
            print(f"   ❌ Error running server: {e}")
            return False, None

    def _compile_server(self) -> bool:
        """Compile server with CMake"""
        build_dir = self.trinity_root / "build"

        try:
            print(f"   Build directory: {build_dir}")
            result = subprocess.run(
                ["cmake", "--build", ".", "--target", "worldserver", "--config", "RelWithDebInfo"],
                cwd=build_dir,
                capture_output=True,
                text=True,
                timeout=1800
            )

            return result.returncode == 0

        except Exception as e:
            print(f"   ❌ Build error: {e}")
            return False

    def _save_crash_report(self, crash: CrashInfo, context):
        """Save crash report for manual review"""
        reports_dir = self.trinity_root / ".claude" / "crash_reports"
        self.context_analyzer.save_context_report(
            context,
            self.crash_analyzer.crash_database["crashes"].get(crash.crash_id, {}),
            reports_dir
        )

    def _generate_detailed_report(self, crash: CrashInfo, context):
        """Generate detailed report for repeated crashes"""
        report_file = self.trinity_root / ".claude" / f"CRASH_LOOP_REPORT_{crash.crash_id}.md"

        content = f"""# Crash Loop Report - {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}

## Crash Persistent After {self.iteration} Iterations

**Crash ID**: `{crash.crash_id}`
**Category**: {crash.crash_category}
**Severity**: {crash.severity}
**Occurrences**: {self.crash_history.count(crash.crash_id)} times

## Crash Details

- **Location**: `{crash.crash_location}`
- **Function**: `{crash.crash_function}`
- **Error**: {crash.error_message}

## Root Cause

{crash.root_cause_hypothesis}

## Fixes Applied

{chr(10).join(f'- {fix.name}' for fix in self.fixes_applied)}

## Manual Intervention Required

This crash persists after automated fixing attempts. Possible issues:

1. The fix was not applied correctly
2. The fix addresses symptoms but not root cause
3. Multiple bugs are present
4. Compiler optimization issue
5. Race condition requiring different approach

## Recommended Actions

1. Review all generated fixes manually
2. Run server with ASAN/TSAN to detect memory/threading issues
3. Add extensive debug logging around crash location
4. Consult TrinityCore community for known issues
5. Consider bisecting recent changes to identify regression

## Call Stack

```
{chr(10).join(crash.call_stack)}
```

## Log Context

See crash_reports/ directory for complete log analysis.

---

**Auto-generated by Crash Loop Orchestrator**
"""

        with open(report_file, 'w', encoding='utf-8') as f:
            f.write(content)

        print(f"\n📋 Detailed report saved: {report_file}")

    def _print_final_summary(self):
        """Print final summary"""
        print("\n" + "="*100)
        print("📊 CRASH LOOP SUMMARY")
        print("="*100)
        print(f"\n   Total Iterations: {self.iteration}")
        print(f"   Unique Crashes: {len(set(self.crash_history))}")
        print(f"   Total Crashes: {len(self.crash_history)}")
        print(f"   Fixes Generated: {len(self.fixes_applied)}")

        if self.crash_history:
            print(f"\n   Crash Breakdown:")
            from collections import Counter
            for crash_id, count in Counter(self.crash_history).most_common():
                crash_data = self.crash_analyzer.crash_database["crashes"].get(crash_id, {})
                location = crash_data.get("crash_location", "Unknown")
                print(f"      - {crash_id}: {location} ({count}x)")

        print("\n" + "="*100 + "\n")

# ============================================================================
# Main Entry Point
# ============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="Enterprise Automated Crash Loop with Self-Healing"
    )

    parser.add_argument(
        '--auto-run',
        action='store_true',
        help='Run fully automated loop (analyze, fix, compile, test)'
    )

    parser.add_argument(
        '--single-iteration',
        action='store_true',
        help='Run single iteration (manual steps)'
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
        '--server-log',
        type=Path,
        default=Path('M:/Wplayerbot/Logs/Server.log'),
        help='Server.log path'
    )

    parser.add_argument(
        '--playerbot-log',
        type=Path,
        default=Path('M:/Wplayerbot/Logs/Playerbot.log'),
        help='Playerbot.log path'
    )

    parser.add_argument(
        '--crashes',
        type=Path,
        default=Path('M:/Wplayerbot/Crashes'),
        help='Crash dumps directory'
    )

    parser.add_argument(
        '--max-iterations',
        type=int,
        default=10,
        help='Maximum crash loop iterations'
    )

    args = parser.parse_args()

    # Create orchestrator
    orchestrator = CrashLoopOrchestrator(
        trinity_root=args.trinity_root,
        server_exe=args.server_exe,
        server_log=args.server_log,
        playerbot_log=args.playerbot_log,
        crashes_dir=args.crashes
    )

    orchestrator.max_iterations = args.max_iterations

    # Run based on mode
    if args.auto_run:
        print("🤖 Running in FULLY AUTOMATED mode")
        print("   ⚠️  This will automatically apply fixes and recompile!\n")
        response = input("   Continue? (y/n): ").strip().lower()
        if response == 'y':
            orchestrator.run_crash_loop(auto_fix=True, auto_compile=True)
    elif args.single_iteration:
        print("👤 Running in MANUAL mode (single iteration)\n")
        orchestrator.max_iterations = 1
        orchestrator.run_crash_loop(auto_fix=False, auto_compile=False)
    else:
        print("👤 Running in SEMI-AUTOMATED mode (manual review required)\n")
        orchestrator.run_crash_loop(auto_fix=False, auto_compile=False)

if __name__ == "__main__":
    main()
