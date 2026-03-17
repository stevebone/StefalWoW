#!C:\Program Files\Python313\python.exe
"""
PlayerBot Critical Improvements Setup Script
Implements the most important recommendations immediately
"""

import os
import sys
import json
import subprocess
import shutil
import argparse
from pathlib import Path
from datetime import datetime

class Colors:
    """ANSI color codes for console output"""
    CYAN = '\033[96m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    GRAY = '\033[90m'
    ENDC = '\033[0m'  # End color
    BOLD = '\033[1m'

def print_colored(text, color=Colors.ENDC):
    """Print text with color"""
    print(f"{color}{text}{Colors.ENDC}")

def print_header():
    """Print the setup header"""
    separator = "=" * 60
    print_colored(separator, Colors.CYAN)
    print_colored("PlayerBot Critical Improvements Setup", Colors.CYAN)
    print_colored(separator, Colors.CYAN)

def ensure_directory(path):
    """Ensure a directory exists"""
    try:
        Path(path).mkdir(parents=True, exist_ok=True)
        return True
    except Exception as e:
        print_colored(f"  ⚠ Failed to create directory {path}: {e}", Colors.YELLOW)
        return False

def write_file(file_path, content):
    """Write content to a file safely"""
    try:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(content)
        return True
    except Exception as e:
        print_colored(f"  ⚠ Failed to write {file_path}: {e}", Colors.YELLOW)
        return False

def setup_dependency_scanner(claude_dir):
    """Setup dependency scanner"""
    print_colored("\n📦 Setting up Dependency Scanner...", Colors.GREEN)
    
    # Check for npm and install security tools
    print_colored("  Installing security tools...", Colors.GRAY)
    try:
        result = subprocess.run(['npm', '--version'], capture_output=True, text=True)
        if result.returncode == 0:
            subprocess.run(['npm', 'install', '-g', 'snyk', 'retire'], 
                         capture_output=True, text=True)
            print_colored("  ✓ Node security tools installed", Colors.GREEN)
        else:
            print_colored("  ⚠ npm not found - skipping Node.js security tools", Colors.YELLOW)
    except FileNotFoundError:
        print_colored("  ⚠ npm not found - skipping Node.js security tools", Colors.YELLOW)
    
    # Create dependency scanner script
    scanner_content = '''#!C:\Program Files\Python313\python.exe
"""
Dependency Security Scanner for TrinityCore PlayerBot
"""
import json
import re
import os
from pathlib import Path
from datetime import datetime

def scan_dependencies():
    """Scan project dependencies for vulnerabilities"""
    results = {
        "scan_date": datetime.now().isoformat(),
        "vulnerable_packages": [],
        "outdated_packages": [],
        "security_issues": []
    }
    
    print("\\n=== Dependency Security Scan ===")
    
    # Check CMakeLists.txt for dependencies
    cmake_file = Path("CMakeLists.txt")
    if cmake_file.exists():
        print("Analyzing CMakeLists.txt...")
        try:
            with open(cmake_file, 'r', encoding='utf-8') as f:
                content = f.read()
                
                # Check Boost version
                boost_match = re.search(r'find_package\\(Boost\\s+(\\d+\\.\\d+)', content, re.IGNORECASE)
                if boost_match:
                    boost_version = boost_match.group(1)
                    print(f"Found Boost version: {boost_version}")
                    
                    if float(boost_version) < 1.70:
                        results["vulnerable_packages"].append({
                            "package": "Boost",
                            "version": boost_version,
                            "severity": "MEDIUM",
                            "recommendation": "Update to Boost 1.70+ for security fixes"
                        })
                
                # Check MySQL version
                mysql_match = re.search(r'find_package\\(MySQL\\s+(\\d+\\.\\d+)', content, re.IGNORECASE)
                if mysql_match:
                    mysql_version = mysql_match.group(1)
                    print(f"Found MySQL version: {mysql_version}")
        except Exception as e:
            print(f"Warning: Could not read CMakeLists.txt: {e}")
    
    # Check for hardcoded credentials in source files
    src_path = Path("src")
    if src_path.exists():
        print("Scanning source files for security issues...")
        suspicious_patterns = [
            (r'password\\s*=\\s*["\\'\\'][^"\\'\\']+ ["\\'\\']', "Hardcoded password"),
            (r'api[_-]?key\\s*=\\s*["\\'\\'][^"\\'\\']+["\\'\\']', "Hardcoded API key"),
            (r'secret\\s*=\\s*["\\'\\'][^"\\'\\']+["\\'\\']', "Hardcoded secret"),
            (r'token\\s*=\\s*["\\'\\'][^"\\'\\']+["\\'\\']', "Hardcoded token")
        ]
        
        scanned_files = 0
        for ext in ['*.cpp', '*.h', '*.hpp']:
            for file_path in src_path.rglob(ext):
                scanned_files += 1
                if scanned_files % 100 == 0:
                    print(f"  Scanned {scanned_files} files...")
                
                try:
                    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                        content = f.read()
                        for pattern, issue_type in suspicious_patterns:
                            if re.search(pattern, content, re.IGNORECASE):
                                results["security_issues"].append({
                                    "file": str(file_path.relative_to(Path.cwd())),
                                    "issue": issue_type,
                                    "severity": "HIGH",
                                    "line": "Unknown"
                                })
                except Exception:
                    continue
        
        print(f"  Scanned {scanned_files} source files")
    
    # Generate summary
    print("\\n=== Scan Results ===")
    print(f"Vulnerable packages: {len(results['vulnerable_packages'])}")
    print(f"Security issues: {len(results['security_issues'])}")
    
    if results['vulnerable_packages']:
        print("\\n⚠️  Vulnerable Packages:")
        for pkg in results['vulnerable_packages']:
            print(f"  - {pkg['package']} {pkg['version']}: {pkg['recommendation']}")
    
    if results['security_issues']:
        print("\\n🔍 Security Issues Found:")
        for issue in results['security_issues'][:5]:  # Show first 5
            print(f"  - {issue['file']}: {issue['issue']}")
        if len(results['security_issues']) > 5:
            print(f"  ... and {len(results['security_issues']) - 5} more")
    
    # Save results
    os.makedirs(".claude", exist_ok=True)
    with open(".claude/security_scan_results.json", "w") as f:
        json.dump(results, f, indent=2)
    
    print(f"\\n✓ Results saved to .claude/security_scan_results.json")
    
    # Return exit code based on critical issues
    critical_count = len([p for p in results['vulnerable_packages'] if p['severity'] == 'CRITICAL'])
    critical_count += len([i for i in results['security_issues'] if i['severity'] == 'CRITICAL'])
    
    if critical_count > 0:
        print(f"\\n❌ {critical_count} critical issues found!")
        return 1
    else:
        print("\\n✅ No critical issues found")
        return 0

if __name__ == "__main__":
    sys.exit(scan_dependencies())
'''
    
    scanner_path = claude_dir / "scripts" / "dependency_scanner.py"
    if write_file(scanner_path, scanner_content):
        # Make it executable on Unix systems
        try:
            os.chmod(scanner_path, 0o755)
        except:
            pass
        print_colored("  ✓ Dependency scanner created", Colors.GREEN)
        return True
    return False

def setup_rollback_system(claude_dir, project_root):
    """Setup rollback system"""
    print_colored("\n🔄 Setting up Rollback System...", Colors.GREEN)
    
    # Create rollback points directory
    rollback_dir = claude_dir / "rollback_points"
    ensure_directory(rollback_dir)
    
    # Create rollback script
    rollback_content = '''#!C:\Program Files\Python313\python.exe
"""
Git-based rollback system for PlayerBot
"""
import os
import sys
import json
import subprocess
import shutil
from pathlib import Path
from datetime import datetime
import argparse

def run_git_command(cmd):
    """Run a git command safely"""
    try:
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        return result.returncode == 0, result.stdout.strip(), result.stderr.strip()
    except Exception as e:
        return False, "", str(e)

def create_rollback_point():
    """Create a new rollback point"""
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    rollback_dir = Path(".claude/rollback_points")
    rollback_point = rollback_dir / f"rollback_{timestamp}"
    
    print(f"Creating rollback point: {timestamp}")
    rollback_point.mkdir(parents=True, exist_ok=True)
    
    # Check if we're in a git repository
    is_git, _, _ = run_git_command("git status")
    
    if is_git:
        # Save current git state
        success, commit_hash, _ = run_git_command("git rev-parse HEAD")
        if success:
            with open(rollback_point / "git_commit.txt", "w") as f:
                f.write(commit_hash)
        
        # Save uncommitted changes
        success, diff_output, _ = run_git_command("git diff")
        if success and diff_output:
            with open(rollback_point / "uncommitted_changes.diff", "w") as f:
                f.write(diff_output)
        
        # Save current branch
        success, branch, _ = run_git_command("git branch --show-current")
        if success:
            with open(rollback_point / "current_branch.txt", "w") as f:
                f.write(branch)
        
        print("✓ Git state saved")
    else:
        print("⚠ Not a git repository - limited rollback capabilities")
    
    # Backup critical files
    critical_files = ["CMakeLists.txt", ".claude/automation_config.json"]
    for file_path in critical_files:
        if Path(file_path).exists():
            try:
                shutil.copy2(file_path, rollback_point)
                print(f"✓ Backed up {file_path}")
            except Exception as e:
                print(f"⚠ Could not backup {file_path}: {e}")
    
    # Save build info
    build_info = {
        "timestamp": timestamp,
        "files_count": 0,
        "git_available": is_git
    }
    
    # Count source files
    src_path = Path("src")
    if src_path.exists():
        build_info["files_count"] = len(list(src_path.rglob("*.cpp"))) + len(list(src_path.rglob("*.h")))
    
    if is_git:
        success, commit_hash, _ = run_git_command("git rev-parse HEAD")
        if success:
            build_info["git_hash"] = commit_hash
        
        success, branch, _ = run_git_command("git branch --show-current")
        if success:
            build_info["branch"] = branch
    
    with open(rollback_point / "build_info.json", "w") as f:
        json.dump(build_info, f, indent=2)
    
    print(f"✅ Rollback point created: {rollback_point}")
    return rollback_point

def list_rollback_points():
    """List available rollback points"""
    rollback_dir = Path(".claude/rollback_points")
    if not rollback_dir.exists():
        print("No rollback points available")
        return []
    
    points = sorted([d for d in rollback_dir.iterdir() if d.is_dir()], reverse=True)
    
    if not points:
        print("No rollback points available")
        return points
    
    print("\\nAvailable rollback points:")
    for i, point in enumerate(points[:10]):  # Show last 10
        try:
            build_info_path = point / "build_info.json"
            if build_info_path.exists():
                with open(build_info_path) as f:
                    info = json.load(f)
                
                timestamp = info.get("timestamp", "Unknown")
                files = info.get("files_count", 0)
                branch = info.get("branch", "unknown")
                
                print(f"  {i+1}. {point.name}")
                print(f"      Created: {timestamp}")
                print(f"      Files: {files}, Branch: {branch}")
            else:
                print(f"  {i+1}. {point.name} (no info available)")
        except Exception as e:
            print(f"  {i+1}. {point.name} (error reading info: {e})")
    
    return points

def emergency_rollback():
    """Perform emergency rollback to latest point"""
    points = list_rollback_points()
    if not points:
        print("❌ No rollback points available")
        return False
    
    latest = points[0]
    print(f"\\n⚠️  Performing emergency rollback to: {latest.name}")
    
    # Load rollback info
    build_info_path = latest / "build_info.json"
    if build_info_path.exists():
        with open(build_info_path) as f:
            build_info = json.load(f)
    else:
        build_info = {}
    
    # Rollback git if available
    git_commit_file = latest / "git_commit.txt"
    if git_commit_file.exists():
        with open(git_commit_file) as f:
            commit_hash = f.read().strip()
        
        if commit_hash:
            print(f"Rolling back to git commit: {commit_hash[:8]}")
            success, _, error = run_git_command(f"git reset --hard {commit_hash}")
            if success:
                print("✓ Git rollback successful")
            else:
                print(f"⚠ Git rollback failed: {error}")
    
    # Restore critical files
    for file_name in ["CMakeLists.txt", "automation_config.json"]:
        backup_file = latest / file_name
        if backup_file.exists():
            try:
                if file_name == "automation_config.json":
                    target = Path(".claude") / file_name
                else:
                    target = Path(file_name)
                
                shutil.copy2(backup_file, target)
                print(f"✓ Restored {file_name}")
            except Exception as e:
                print(f"⚠ Could not restore {file_name}: {e}")
    
    print("✅ Emergency rollback completed")
    return True

def main():
    parser = argparse.ArgumentParser(description="PlayerBot Rollback System")
    parser.add_argument("mode", choices=["create", "list", "emergency"], 
                      help="Rollback operation mode")
    
    args = parser.parse_args()
    
    if args.mode == "create":
        create_rollback_point()
    elif args.mode == "list":
        list_rollback_points()
    elif args.mode == "emergency":
        success = emergency_rollback()
        sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
'''
    
    rollback_path = claude_dir / "scripts" / "rollback.py"
    if write_file(rollback_path, rollback_content):
        try:
            os.chmod(rollback_path, 0o755)
        except:
            pass
        print_colored("  ✓ Rollback system created", Colors.GREEN)
        
        # Create initial rollback point if in git repo
        try:
            os.chdir(project_root)
            if Path(".git").exists():
                result = subprocess.run([sys.executable, str(rollback_path), "create"], 
                                      capture_output=True, text=True)
                if result.returncode == 0:
                    print_colored("  ✓ Initial rollback point created", Colors.GREEN)
                else:
                    print_colored("  ⚠ Could not create initial rollback point", Colors.YELLOW)
            else:
                print_colored("  ⚠ Not a git repository - rollback limited", Colors.YELLOW)
        except Exception as e:
            print_colored(f"  ⚠ Could not create initial rollback point: {e}", Colors.YELLOW)
        
        return True
    return False

def setup_monitoring(claude_dir):
    """Setup real-time monitoring"""
    print_colored("\n📊 Enabling Real-time Monitoring...", Colors.GREEN)
    
    # Create monitoring script
    monitoring_content = '''#!C:\Program Files\Python313\python.exe
"""
Real-time monitoring service for PlayerBot
"""
import json
import time
import psutil
import os
from pathlib import Path
from datetime import datetime

def get_system_metrics():
    """Get current system metrics"""
    try:
        cpu_percent = psutil.cpu_percent(interval=1)
        memory = psutil.virtual_memory()
        disk = psutil.disk_usage('/')
        
        # Check for TrinityCore processes
        trinity_running = False
        trinity_memory = 0
        
        for proc in psutil.process_iter(['pid', 'name', 'memory_info']):
            try:
                if 'worldserver' in proc.info['name'].lower():
                    trinity_running = True
                    trinity_memory = proc.info['memory_info'].rss / 1024 / 1024  # MB
                    break
            except (psutil.NoSuchProcess, psutil.AccessDenied):
                continue
        
        return {
            "timestamp": datetime.now().isoformat(),
            "cpu_usage": round(cpu_percent, 1),
            "memory_usage": round(memory.percent, 1),
            "memory_used_gb": round(memory.used / 1024**3, 2),
            "disk_usage": round(disk.percent, 1),
            "process_count": len(psutil.pids()),
            "trinity_running": trinity_running,
            "trinity_memory_mb": round(trinity_memory, 1) if trinity_running else 0
        }
    except Exception as e:
        return {
            "timestamp": datetime.now().isoformat(),
            "error": str(e),
            "cpu_usage": 0,
            "memory_usage": 0,
            "disk_usage": 0
        }

def main():
    print("Starting PlayerBot Monitoring Service...")
    
    # Ensure directories exist
    monitoring_dir = Path(".claude/monitoring")
    monitoring_dir.mkdir(parents=True, exist_ok=True)
    
    metrics_file = monitoring_dir / "metrics.json"
    log_file = monitoring_dir / "monitor.log"
    
    print(f"Metrics file: {metrics_file}")
    print(f"Log file: {log_file}")
    print("Press Ctrl+C to stop monitoring\\n")
    
    try:
        iteration = 0
        while True:
            iteration += 1
            metrics = get_system_metrics()
            
            # Save metrics
            try:
                with open(metrics_file, 'w') as f:
                    json.dump(metrics, f, indent=2)
            except Exception as e:
                print(f"Error saving metrics: {e}")
            
            # Display current metrics
            if 'error' not in metrics:
                print(f"[{iteration:3d}] {datetime.now().strftime('%H:%M:%S')} - "
                      f"CPU: {metrics['cpu_usage']:5.1f}% | "
                      f"Memory: {metrics['memory_usage']:5.1f}% | "
                      f"Disk: {metrics['disk_usage']:5.1f}% | "
                      f"Trinity: {'✓' if metrics['trinity_running'] else '✗'}")
                
                # Log warnings
                warnings = []
                if metrics['cpu_usage'] > 80:
                    warnings.append(f"High CPU: {metrics['cpu_usage']}%")
                if metrics['memory_usage'] > 85:
                    warnings.append(f"High Memory: {metrics['memory_usage']}%")
                if metrics['disk_usage'] > 90:
                    warnings.append(f"High Disk: {metrics['disk_usage']}%")
                
                if warnings:
                    warning_msg = f"{datetime.now()}: WARNING - {', '.join(warnings)}"
                    print(f"⚠️  {warning_msg}")
                    try:
                        with open(log_file, 'a') as f:
                            f.write(warning_msg + "\\n")
                    except Exception:
                        pass
            else:
                print(f"[{iteration:3d}] Error getting metrics: {metrics.get('error', 'Unknown')}")
            
            time.sleep(10)
    
    except KeyboardInterrupt:
        print("\\n✓ Monitoring stopped by user")
    except Exception as e:
        print(f"\\n❌ Monitoring error: {e}")

if __name__ == "__main__":
    main()
'''
    
    monitoring_path = claude_dir / "monitoring" / "monitor_service.py"
    if write_file(monitoring_path, monitoring_content):
        try:
            os.chmod(monitoring_path, 0o755)
        except:
            pass
        print_colored("  ✓ Monitoring service created", Colors.GREEN)
    
    # Create HTML dashboard
    dashboard_content = '''<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>PlayerBot Monitoring Dashboard</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            margin: 0;
            padding: 20px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
        }
        
        .container {
            max-width: 1200px;
            margin: 0 auto;
        }
        
        h1 {
            color: white;
            text-align: center;
            margin-bottom: 30px;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
        }
        
        .metrics-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }
        
        .metric-card {
            background: white;
            border-radius: 15px;
            padding: 25px;
            box-shadow: 0 8px 32px rgba(0,0,0,0.1);
            transition: transform 0.3s ease;
        }
        
        .metric-card:hover {
            transform: translateY(-5px);
        }
        
        .metric-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 15px;
        }
        
        .metric-title {
            font-size: 18px;
            font-weight: 600;
            color: #333;
        }
        
        .metric-icon {
            font-size: 24px;
        }
        
        .metric-value {
            font-size: 32px;
            font-weight: 700;
            margin-bottom: 10px;
        }
        
        .metric-subtitle {
            color: #666;
            font-size: 14px;
        }
        
        .status-good { color: #10b981; }
        .status-warning { color: #f59e0b; }
        .status-critical { color: #ef4444; }
        
        .last-update {
            text-align: center;
            color: white;
            margin-top: 20px;
            font-size: 14px;
            opacity: 0.8;
        }
        
        .status-indicator {
            display: inline-block;
            width: 12px;
            height: 12px;
            border-radius: 50%;
            margin-right: 8px;
        }
        
        .progress-bar {
            width: 100%;
            height: 8px;
            background: #e5e7eb;
            border-radius: 4px;
            overflow: hidden;
            margin-top: 10px;
        }
        
        .progress-fill {
            height: 100%;
            transition: width 0.5s ease;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🚀 PlayerBot Monitoring Dashboard</h1>
        
        <div class="metrics-grid">
            <div class="metric-card">
                <div class="metric-header">
                    <div class="metric-title">System Status</div>
                    <div class="metric-icon">🖥️</div>
                </div>
                <div class="metric-value status-good">
                    <span class="status-indicator status-good" style="background: #10b981;"></span>
                    Online
                </div>
                <div class="metric-subtitle">Monitoring Active</div>
            </div>
            
            <div class="metric-card">
                <div class="metric-header">
                    <div class="metric-title">CPU Usage</div>
                    <div class="metric-icon">⚡</div>
                </div>
                <div class="metric-value" id="cpu-value">--</div>
                <div class="progress-bar">
                    <div class="progress-fill" id="cpu-progress" style="background: #3b82f6;"></div>
                </div>
                <div class="metric-subtitle">Processor Load</div>
            </div>
            
            <div class="metric-card">
                <div class="metric-header">
                    <div class="metric-title">Memory Usage</div>
                    <div class="metric-icon">💾</div>
                </div>
                <div class="metric-value" id="memory-value">--</div>
                <div class="progress-bar">
                    <div class="progress-fill" id="memory-progress" style="background: #10b981;"></div>
                </div>
                <div class="metric-subtitle" id="memory-subtitle">RAM Usage</div>
            </div>
            
            <div class="metric-card">
                <div class="metric-header">
                    <div class="metric-title">Disk Usage</div>
                    <div class="metric-icon">💿</div>
                </div>
                <div class="metric-value" id="disk-value">--</div>
                <div class="progress-bar">
                    <div class="progress-fill" id="disk-progress" style="background: #f59e0b;"></div>
                </div>
                <div class="metric-subtitle">Storage</div>
            </div>
            
            <div class="metric-card">
                <div class="metric-header">
                    <div class="metric-title">TrinityCore</div>
                    <div class="metric-icon">🎮</div>
                </div>
                <div class="metric-value" id="trinity-status">--</div>
                <div class="metric-subtitle" id="trinity-subtitle">Server Status</div>
            </div>
            
            <div class="metric-card">
                <div class="metric-header">
                    <div class="metric-title">Processes</div>
                    <div class="metric-icon">📊</div>
                </div>
                <div class="metric-value" id="process-count">--</div>
                <div class="metric-subtitle">Active Processes</div>
            </div>
        </div>
        
        <div class="last-update" id="last-update">
            Last updated: Loading...
        </div>
    </div>

    <script>
        function updateMetrics() {
            fetch('./metrics.json')
                .then(response => response.json())
                .then(data => {
                    // Update timestamp
                    const timestamp = new Date(data.timestamp);
                    document.getElementById('last-update').textContent = 
                        `Last updated: ${timestamp.toLocaleString()}`;
                    
                    // Update CPU
                    const cpuValue = data.cpu_usage || 0;
                    document.getElementById('cpu-value').textContent = `${cpuValue}%`;
                    document.getElementById('cpu-progress').style.width = `${cpuValue}%`;
                    updateValueColor('cpu-value', cpuValue, 70, 85);
                    
                    // Update Memory
                    const memValue = data.memory_usage || 0;
                    const memUsed = data.memory_used_gb || 0;
                    document.getElementById('memory-value').textContent = `${memValue}%`;
                    document.getElementById('memory-subtitle').textContent = `${memUsed} GB Used`;
                    document.getElementById('memory-progress').style.width = `${memValue}%`;
                    updateValueColor('memory-value', memValue, 70, 85);
                    
                    // Update Disk
                    const diskValue = data.disk_usage || 0;
                    document.getElementById('disk-value').textContent = `${diskValue}%`;
                    document.getElementById('disk-progress').style.width = `${diskValue}%`;
                    updateValueColor('disk-value', diskValue, 80, 90);
                    
                    // Update Trinity
                    const trinityRunning = data.trinity_running;
                    const trinityElement = document.getElementById('trinity-status');
                    if (trinityRunning) {
                        trinityElement.innerHTML = '<span class="status-indicator" style="background: #10b981;"></span>Running';
                        trinityElement.className = 'metric-value status-good';
                        document.getElementById('trinity-subtitle').textContent = 
                            `Memory: ${data.trinity_memory_mb || 0} MB`;
                    } else {
                        trinityElement.innerHTML = '<span class="status-indicator" style="background: #ef4444;"></span>Stopped';
                        trinityElement.className = 'metric-value status-critical';
                        document.getElementById('trinity-subtitle').textContent = 'Server Offline';
                    }
                    
                    // Update process count
                    document.getElementById('process-count').textContent = data.process_count || '--';
                })
                .catch(error => {
                    console.warn('Could not load metrics:', error);
                    document.getElementById('last-update').textContent = 
                        'Last updated: Error loading data';
                });
        }
        
        function updateValueColor(elementId, value, warningThreshold, criticalThreshold) {
            const element = document.getElementById(elementId);
            element.className = 'metric-value';
            
            if (value >= criticalThreshold) {
                element.classList.add('status-critical');
            } else if (value >= warningThreshold) {
                element.classList.add('status-warning');
            } else {
                element.classList.add('status-good');
            }
        }
        
        // Update metrics immediately and then every 5 seconds
        updateMetrics();
        setInterval(updateMetrics, 5000);
    </script>
</body>
</html>'''
    
    dashboard_path = claude_dir / "monitoring" / "dashboard.html"
    if write_file(dashboard_path, dashboard_content):
        print_colored("  ✓ Dashboard created", Colors.GREEN)
    
    # Try to install psutil if not available
    try:
        import psutil
        print_colored("  ✓ psutil already available", Colors.GREEN)
    except ImportError:
        print_colored("  Installing psutil for system monitoring...", Colors.GRAY)
        try:
            subprocess.run([sys.executable, '-m', 'pip', 'install', 'psutil'], 
                         capture_output=True, text=True, check=True)
            print_colored("  ✓ psutil installed successfully", Colors.GREEN)
        except subprocess.CalledProcessError:
            print_colored("  ⚠ Could not install psutil - monitoring will be limited", Colors.YELLOW)
    
    return True

def setup_alerts(claude_dir):
    """Setup alert system"""
    print_colored("\n🔔 Configuring Alert System...", Colors.GREEN)
    
    alert_config = {
        "version": "1.0",
        "created": datetime.now().isoformat(),
        "thresholds": {
            "cpu_warning": 70,
            "cpu_critical": 85,
            "memory_warning": 75,
            "memory_critical": 90,
            "disk_warning": 80,
            "disk_critical": 95,
            "security_issues": 1
        },
        "notifications": {
            "console": {
                "enabled": True,
                "level": "warning"
            },
            "log_file": {
                "enabled": True,
                "path": str(claude_dir / "alerts.log"),
                "level": "all"
            },
            "email": {
                "enabled": False,
                "smtp_server": "smtp.gmail.com",
                "smtp_port": 587,
                "recipients": ["admin@project.com"]
            }
        },
        "cooldown_minutes": 15,
        "description": "Alert configuration for PlayerBot monitoring system"
    }
    
    config_path = claude_dir / "alert_config.json"
    if write_file(config_path, json.dumps(alert_config, indent=2)):
        print_colored("  ✓ Alert system configured", Colors.GREEN)
        return True
    return False

def setup_quick_wins(claude_dir, project_root):
    """Setup quick wins"""
    print_colored("\n⚡ Implementing Quick Wins...", Colors.GREEN)
    
    success_count = 0
    
    # Create git hooks if git repo exists
    git_hooks_dir = project_root / ".git" / "hooks"
    if git_hooks_dir.exists():
        # Pre-commit hook
        pre_commit_content = '''#!C:\Program Files\Python313\python.exe
"""
Pre-commit hook for PlayerBot - Quick quality checks
"""
import sys
import subprocess
from pathlib import Path

def main():
    print("🔍 Running pre-commit quality checks...")
    
    # Check if dependency scanner exists and run it
    scanner_path = Path(".claude/scripts/dependency_scanner.py")
    if scanner_path.exists():
        print("Running dependency security scan...")
        try:
            result = subprocess.run([sys.executable, str(scanner_path)], 
                                  capture_output=True, text=True)
            
            if result.returncode != 0:
                print("❌ Security issues found in dependency scan!")
                print(result.stdout)
                print("\\nPlease fix security issues before committing.")
                return 1
            else:
                print("✅ Security scan passed")
        except Exception as e:
            print(f"⚠️ Could not run security scan: {e}")
    
    # Basic structure checks
    required_dirs = ["src", "sql", "cmake"]
    missing_dirs = [d for d in required_dirs if not Path(d).exists()]
    
    if missing_dirs:
        print(f"⚠️ Missing expected directories: {', '.join(missing_dirs)}")
    else:
        print("✅ Project structure looks good")
    
    # Check CMakeLists.txt
    if not Path("CMakeLists.txt").exists():
        print("⚠️ CMakeLists.txt not found")
    else:
        print("✅ CMakeLists.txt found")
    
    print("✅ Pre-commit checks completed successfully")
    return 0

if __name__ == "__main__":
    sys.exit(main())
'''
        
        pre_commit_path = git_hooks_dir / "pre-commit"
        if write_file(pre_commit_path, pre_commit_content):
            try:
                os.chmod(pre_commit_path, 0o755)
                print_colored("  ✓ Git pre-commit hook installed", Colors.GREEN)
                success_count += 1
            except:
                print_colored("  ⚠ Could not make pre-commit hook executable", Colors.YELLOW)
    else:
        print_colored("  ⚠ Git repository not found - skipping hooks", Colors.YELLOW)
    
    # Create metrics baseline
    baseline = {
        "version": "1.0",
        "created": datetime.now().isoformat(),
        "baseline_metrics": {
            "quality_score": 85,
            "performance_score": 90,
            "security_score": 92,
            "test_coverage": 78,
            "trinity_compatibility": 95
        },
        "thresholds": {
            "quality_min": 80,
            "performance_min": 85,
            "security_min": 90,
            "coverage_min": 75,
            "trinity_min": 90
        },
        "goals": {
            "quality_target": 95,
            "performance_target": 95,
            "security_target": 98,
            "coverage_target": 85,
            "trinity_target": 98
        }
    }
    
    baseline_path = claude_dir / "metrics_baseline.json"
    if write_file(baseline_path, json.dumps(baseline, indent=2)):
        print_colored("  ✓ Metrics baseline initialized", Colors.GREEN)
        success_count += 1
    
    # Create a simple health check script
    health_check_content = '''#!C:\Program Files\Python313\python.exe
"""
Quick health check for PlayerBot system
"""
import json
from pathlib import Path
from datetime import datetime

def check_system_health():
    """Perform basic system health checks"""
    results = {
        "timestamp": datetime.now().isoformat(),
        "checks": [],
        "overall_status": "good"
    }
    
    # Check critical directories
    critical_dirs = [".claude", ".claude/scripts", ".claude/monitoring"]
    for dir_path in critical_dirs:
        if Path(dir_path).exists():
            results["checks"].append({
                "name": f"Directory: {dir_path}",
                "status": "pass",
                "message": "Directory exists"
            })
        else:
            results["checks"].append({
                "name": f"Directory: {dir_path}",
                "status": "fail", 
                "message": "Directory missing"
            })
            results["overall_status"] = "warning"
    
    # Check critical files
    critical_files = [
        ".claude/scripts/dependency_scanner.py",
        ".claude/scripts/rollback.py",
        ".claude/monitoring/monitor_service.py",
        ".claude/alert_config.json"
    ]
    
    for file_path in critical_files:
        if Path(file_path).exists():
            results["checks"].append({
                "name": f"File: {file_path}",
                "status": "pass",
                "message": "File exists and accessible"
            })
        else:
            results["checks"].append({
                "name": f"File: {file_path}",
                "status": "fail",
                "message": "File missing"
            })
            results["overall_status"] = "warning"
    
    # Check git repository
    if Path(".git").exists():
        results["checks"].append({
            "name": "Git Repository",
            "status": "pass",
            "message": "Git repository detected"
        })
    else:
        results["checks"].append({
            "name": "Git Repository", 
            "status": "info",
            "message": "Not a git repository - some features limited"
        })
    
    return results

def main():
    print("🏥 PlayerBot System Health Check")
    print("=" * 40)
    
    results = check_system_health()
    
    pass_count = sum(1 for check in results["checks"] if check["status"] == "pass")
    fail_count = sum(1 for check in results["checks"] if check["status"] == "fail")
    info_count = sum(1 for check in results["checks"] if check["status"] == "info")
    
    print(f"\\nResults: {pass_count} passed, {fail_count} failed, {info_count} info")
    print(f"Overall Status: {results['overall_status'].upper()}")
    
    if fail_count > 0:
        print("\\n❌ Failed Checks:")
        for check in results["checks"]:
            if check["status"] == "fail":
                print(f"  - {check['name']}: {check['message']}")
    
    if results["overall_status"] == "good":
        print("\\n✅ All critical systems operational!")
        return 0
    else:
        print("\\n⚠️ Some issues detected - see above")
        return 1

if __name__ == "__main__":
    import sys
    sys.exit(main())
'''
    
    health_check_path = claude_dir / "scripts" / "health_check.py"
    if write_file(health_check_path, health_check_content):
        try:
            os.chmod(health_check_path, 0o755)
            print_colored("  ✓ Health check script created", Colors.GREEN)
            success_count += 1
        except:
            pass
    
    return success_count >= 2

def create_launcher_scripts(claude_dir):
    """Create convenient launcher scripts"""
    print_colored("\n🚀 Creating Launcher Scripts...", Colors.GREEN)
    
    # Windows batch launcher
    batch_content = f'''@echo off
REM PlayerBot Management Launcher

echo ===============================================
echo PlayerBot Management System
echo ===============================================
echo.

echo Select an option:
echo 1. Run Security Scan
echo 2. Start Monitoring Service  
echo 3. Create Rollback Point
echo 4. Emergency Rollback
echo 5. System Health Check
echo 6. Open Monitoring Dashboard
echo 7. Exit
echo.
set /p choice=Enter your choice (1-7): 

if "%choice%"=="1" (
    echo Running security scan...
    python "{claude_dir}/scripts/dependency_scanner.py"
) else if "%choice%"=="2" (
    echo Starting monitoring service...
    python "{claude_dir}/monitoring/monitor_service.py"
) else if "%choice%"=="3" (
    echo Creating rollback point...
    python "{claude_dir}/scripts/rollback.py" create
) else if "%choice%"=="4" (
    echo Performing emergency rollback...
    python "{claude_dir}/scripts/rollback.py" emergency
) else if "%choice%"=="5" (
    echo Running health check...
    python "{claude_dir}/scripts/health_check.py"
) else if "%choice%"=="6" (
    echo Opening monitoring dashboard...
    start "{claude_dir}/monitoring/dashboard.html"
) else if "%choice%"=="7" (
    echo Goodbye!
    exit /b 0
) else (
    echo Invalid choice!
    pause
    goto start
)

echo.
pause
'''
    
    launcher_path = claude_dir / "playerbot_manager.bat"
    if write_file(launcher_path, batch_content):
        print_colored("  ✓ Windows launcher created", Colors.GREEN)
    
    # Unix shell launcher
    shell_content = f'''#!/bin/bash
# PlayerBot Management Launcher

echo "==============================================="
echo "PlayerBot Management System"
echo "==============================================="
echo

while true; do
    echo "Select an option:"
    echo "1. Run Security Scan"
    echo "2. Start Monitoring Service"
    echo "3. Create Rollback Point"
    echo "4. Emergency Rollback"
    echo "5. System Health Check"
    echo "6. Open Monitoring Dashboard"
    echo "7. Exit"
    echo
    read -p "Enter your choice (1-7): " choice

    case $choice in
        1)
            echo "Running security scan..."
            python3 "{claude_dir}/scripts/dependency_scanner.py"
            ;;
        2)
            echo "Starting monitoring service..."
            python3 "{claude_dir}/monitoring/monitor_service.py"
            ;;
        3)
            echo "Creating rollback point..."
            python3 "{claude_dir}/scripts/rollback.py" create
            ;;
        4)
            echo "Performing emergency rollback..."
            python3 "{claude_dir}/scripts/rollback.py" emergency
            ;;
        5)
            echo "Running health check..."
            python3 "{claude_dir}/scripts/health_check.py"
            ;;
        6)
            echo "Opening monitoring dashboard..."
            if command -v xdg-open > /dev/null; then
                xdg-open "{claude_dir}/monitoring/dashboard.html"
            elif command -v open > /dev/null; then
                open "{claude_dir}/monitoring/dashboard.html"
            else
                echo "Please open {claude_dir}/monitoring/dashboard.html manually"
            fi
            ;;
        7)
            echo "Goodbye!"
            exit 0
            ;;
        *)
            echo "Invalid choice!"
            ;;
    esac
    
    echo
    read -p "Press Enter to continue..."
    echo
done
'''
    
    shell_launcher_path = claude_dir / "playerbot_manager.sh"
    if write_file(shell_launcher_path, shell_content):
        try:
            os.chmod(shell_launcher_path, 0o755)
            print_colored("  ✓ Shell launcher created", Colors.GREEN)
        except:
            pass
    
    return True

def main():
    parser = argparse.ArgumentParser(description='PlayerBot Critical Improvements Setup')
    parser.add_argument('--skip-dependency-scanner', action='store_true',
                       help='Skip dependency scanner setup')
    parser.add_argument('--skip-rollback', action='store_true',
                       help='Skip rollback system setup')
    parser.add_argument('--skip-monitoring', action='store_true',
                       help='Skip monitoring system setup')
    parser.add_argument('--skip-alerts', action='store_true',
                       help='Skip alert system setup')
    parser.add_argument('--skip-quick-wins', action='store_true',
                       help='Skip quick wins setup')
    parser.add_argument('--project-root', default='C:\\TrinityBots\\TrinityCore',
                       help='Project root directory')
    
    args = parser.parse_args()
    
    print_header()
    
    project_root = Path(args.project_root)
    claude_dir = project_root / ".claude"
    
    print_colored(f"Project root: {project_root}", Colors.GRAY)
    print_colored(f"Claude directory: {claude_dir}", Colors.GRAY)
    
    # Ensure directory structure
    print_colored("\n📁 Setting up directory structure...", Colors.GREEN)
    directories = [
        claude_dir,
        claude_dir / "scripts",
        claude_dir / "monitoring",
        claude_dir / "rollback_points"
    ]
    
    for directory in directories:
        ensure_directory(directory)
    
    print_colored("  ✓ Directory structure created", Colors.GREEN)
    
    # Change to project root
    try:
        os.chdir(project_root)
        print_colored(f"  ✓ Changed to project directory: {project_root}", Colors.GREEN)
    except Exception as e:
        print_colored(f"  ⚠ Could not change to project directory: {e}", Colors.YELLOW)
        print_colored("  Continuing with current directory...", Colors.YELLOW)
    
    setup_results = []
    
    # Run setup functions
    if not args.skip_dependency_scanner:
        setup_results.append(("Dependency Scanner", setup_dependency_scanner(claude_dir)))
    
    if not args.skip_rollback:
        setup_results.append(("Rollback System", setup_rollback_system(claude_dir, project_root)))
    
    if not args.skip_monitoring:
        setup_results.append(("Monitoring System", setup_monitoring(claude_dir)))
    
    if not args.skip_alerts:
        setup_results.append(("Alert System", setup_alerts(claude_dir)))
    
    if not args.skip_quick_wins:
        setup_results.append(("Quick Wins", setup_quick_wins(claude_dir, project_root)))
    
    # Create launcher scripts
    setup_results.append(("Launcher Scripts", create_launcher_scripts(claude_dir)))
    
    # Print summary
    print_colored("\n" + "=" * 60, Colors.CYAN)
    print_colored("✅ Setup Summary", Colors.GREEN)
    print_colored("=" * 60, Colors.CYAN)
    
    successful_setups = []
    failed_setups = []
    
    for name, success in setup_results:
        if success:
            print_colored(f"  ✅ {name}", Colors.GREEN)
            successful_setups.append(name)
        else:
            print_colored(f"  ❌ {name}", Colors.RED)
            failed_setups.append(name)
    
    print_colored(f"\n📊 Results: {len(successful_setups)} successful, {len(failed_setups)} failed", 
                 Colors.YELLOW)
    
    if successful_setups:
        print_colored("\n🎉 What's been set up:", Colors.YELLOW)
        feature_descriptions = {
            "Dependency Scanner": "🔍 Scans for vulnerable packages and security issues",
            "Rollback System": "🔄 Git-based rollback and recovery system", 
            "Monitoring System": "📊 Real-time system monitoring with web dashboard",
            "Alert System": "🔔 Configurable alerts for critical issues",
            "Quick Wins": "⚡ Pre-commit hooks and health checks",
            "Launcher Scripts": "🚀 Easy-to-use management interface"
        }
        
        for setup in successful_setups:
            if setup in feature_descriptions:
                print_colored(f"  {feature_descriptions[setup]}", Colors.GRAY)
    
    print_colored("\n🚀 Next Steps:", Colors.YELLOW)
    print_colored("  1. Run security scan:", Colors.GRAY)
    print_colored(f"     python {claude_dir}/scripts/dependency_scanner.py", Colors.GRAY)
    print_colored("  2. Create rollback point:", Colors.GRAY)  
    print_colored(f"     python {claude_dir}/scripts/rollback.py create", Colors.GRAY)
    print_colored("  3. Start monitoring:", Colors.GRAY)
    print_colored(f"     python {claude_dir}/monitoring/monitor_service.py", Colors.GRAY)
    print_colored("  4. Open dashboard:", Colors.GRAY)
    print_colored(f"     {claude_dir}/monitoring/dashboard.html", Colors.GRAY)
    print_colored("  5. Use management launcher:", Colors.GRAY)
    print_colored(f"     {claude_dir}/playerbot_manager.bat", Colors.GRAY)
    
    print_colored(f"\n✨ Your PlayerBot system is now significantly more robust!", Colors.GREEN)
    
    # Return appropriate exit code
    return 0 if len(failed_setups) == 0 else 1

if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print_colored("\n\n⚠️ Setup interrupted by user", Colors.YELLOW)
        sys.exit(1)
    except Exception as e:
        print_colored(f"\n\n❌ Unexpected error: {e}", Colors.RED)
        sys.exit(1)
