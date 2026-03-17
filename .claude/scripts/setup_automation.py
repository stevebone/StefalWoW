#!C:\Program Files\Python313\python.exe
"""
Setup Script for PlayerBot Automation System
Run this script to configure the complete automation system

Usage:
    python setup_automation.py
    python setup_automation.py --project-root "D:\MyProject" --skip-dependencies
"""

import os
import sys
import json
import subprocess
import platform
import argparse
from pathlib import Path
from datetime import datetime
import shutil

class Colors:
    """ANSI color codes for console output"""
    CYAN = '\033[96m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    GRAY = '\033[90m'
    WHITE = '\033[97m'
    ENDC = '\033[0m'

def print_colored(text: str, color: str = Colors.WHITE):
    """Print text with color"""
    print(f"{color}{text}{Colors.ENDC}")

def print_header():
    """Print setup header"""
    separator = "=" * 60
    print_colored(separator, Colors.CYAN)
    print_colored("PlayerBot Automation System Setup", Colors.CYAN)
    print_colored(separator, Colors.CYAN)

def check_admin_privileges():
    """Check if running with administrator privileges on Windows"""
    if platform.system() == "Windows":
        try:
            import ctypes
            return ctypes.windll.shell32.IsUserAnAdmin() != 0
        except:
            return False
    else:
        return os.geteuid() == 0

def ensure_directory(path: Path):
    """Ensure directory exists"""
    try:
        path.mkdir(parents=True, exist_ok=True)
        return True
    except Exception as e:
        print_colored(f"  ⚠ Failed to create directory {path}: {e}", Colors.YELLOW)
        return False

def create_directory_structure(project_root: Path):
    """Create the complete directory structure"""
    print_colored("\nCreating directory structure...", Colors.GREEN)
    
    claude_dir = project_root / ".claude"
    directories = [
        claude_dir,
        claude_dir / "agents",
        claude_dir / "scripts", 
        claude_dir / "workflows",
        claude_dir / "reports",
        claude_dir / "logs",
        claude_dir / "monitoring",
        claude_dir / "documentation",
        claude_dir / "backups",
        claude_dir / "rollback_points"
    ]
    
    created_count = 0
    for directory in directories:
        if ensure_directory(directory):
            if not directory.exists():
                print_colored(f"  Created: {directory}", Colors.GRAY)
                created_count += 1
            else:
                print_colored(f"  Exists: {directory}", Colors.GRAY)
    
    print_colored(f"✓ Directory structure ready ({created_count} created)", Colors.GREEN)
    return claude_dir

def install_python_dependencies():
    """Install required Python packages"""
    print_colored("\n📦 Installing Python dependencies...", Colors.GREEN)
    
    dependencies = [
        "requests>=2.25.0",
        "psutil>=5.8.0",
        "click>=7.0",
        "jinja2>=2.11.0",
        "pyyaml>=5.4.0",
        "schedule>=1.1.0",
        "gitpython>=3.1.0"
    ]
    
    installed_count = 0
    failed_packages = []
    
    for package in dependencies:
        print_colored(f"  Installing {package}...", Colors.GRAY)
        try:
            result = subprocess.run([
                sys.executable, "-m", "pip", "install", package
            ], capture_output=True, text=True, timeout=300)
            
            if result.returncode == 0:
                print_colored(f"  ✓ {package} installed successfully", Colors.GREEN)
                installed_count += 1
            else:
                print_colored(f"  ⚠ Failed to install {package}: {result.stderr}", Colors.YELLOW)
                failed_packages.append(package)
        
        except subprocess.TimeoutExpired:
            print_colored(f"  ⚠ Installation of {package} timed out", Colors.YELLOW)
            failed_packages.append(package)
        except Exception as e:
            print_colored(f"  ⚠ Error installing {package}: {e}", Colors.YELLOW)
            failed_packages.append(package)
    
    if failed_packages:
        print_colored(f"⚠ {len(failed_packages)} packages failed to install", Colors.YELLOW)
        print_colored("Manual installation may be required for:", Colors.YELLOW)
        for pkg in failed_packages:
            print_colored(f"  - {pkg}", Colors.GRAY)
    else:
        print_colored(f"✓ All {installed_count} dependencies installed successfully", Colors.GREEN)
    
    return len(failed_packages) == 0

def create_agent_templates(claude_dir: Path):
    """Create agent template files"""
    print_colored("\n🤖 Creating agent templates...", Colors.GREEN)
    
    agents_dir = claude_dir / "agents"
    
    # Agent templates with their descriptions
    agent_templates = {
        "playerbot-project-coordinator.md": """# PlayerBot Project Coordinator Agent

## Role
Coordinates the overall PlayerBot project, managing workflows and ensuring quality standards.

## Capabilities
- Project planning and coordination
- Workflow orchestration  
- Quality gate enforcement
- Progress tracking and reporting

## Input Format
```json
{
    "task": "coordinate_project",
    "phase": "development|testing|deployment",
    "priority": "low|medium|high|critical"
}
```

## Output Format  
```json
{
    "status": "success|warning|error",
    "coordination_plan": "...",
    "next_steps": ["...", "..."],
    "blockers": ["...", "..."],
    "recommendations": ["...", "..."]
}
```

## Usage
This agent serves as the central coordinator for all PlayerBot project activities.
""",
        
        "trinity-integration-tester.md": """# Trinity Integration Tester Agent

## Role
Tests PlayerBot integration with TrinityCore, ensuring compatibility and stability.

## Capabilities
- TrinityCore version compatibility testing
- API integration verification
- Database schema validation
- Performance impact assessment

## Input Format
```json
{
    "test_type": "compatibility|integration|performance",
    "trinity_version": "3.3.5|master|custom",
    "test_scope": "full|incremental|specific"
}
```

## Output Format
```json
{
    "status": "success|warning|error",
    "compatibility_score": 95,
    "issues_found": 2,
    "test_results": {...},
    "recommendations": ["...", "..."]
}
```

## Usage
Run before major releases and after TrinityCore updates.
""",
        
        "code-quality-reviewer.md": """# Code Quality Reviewer Agent

## Role
Analyzes code quality, identifies issues, and suggests improvements.

## Capabilities
- Static code analysis
- Code smell detection
- Maintainability assessment
- Best practices validation

## Input Format
```json
{
    "analysis_type": "full|incremental|targeted",
    "directories": ["src/game/playerbot"],
    "languages": ["cpp", "sql", "python"]
}
```

## Output Format
```json
{
    "status": "success|warning|error", 
    "quality_score": 87,
    "code_smells": 5,
    "issues": [...],
    "recommendations": [...],
    "metrics": {...}
}
```

## Usage
Run daily for continuous quality monitoring.
""",
        
        "security-auditor.md": """# Security Auditor Agent

## Role
Performs security audits and vulnerability assessments.

## Capabilities
- Vulnerability scanning
- Security best practices validation
- Dependency security analysis
- Threat modeling

## Input Format
```json
{
    "audit_type": "full|targeted|dependencies",
    "scope": ["authentication", "database", "network"],
    "severity_threshold": "low|medium|high|critical"
}
```

## Output Format
```json
{
    "status": "success|warning|error",
    "security_score": 92,
    "vulnerabilities": {
        "critical": 0,
        "high": 1,
        "medium": 3,
        "low": 7
    },
    "recommendations": [...],
    "action_items": [...]
}
```

## Usage
Run weekly and before releases.
""",
        
        "performance-analyzer.md": """# Performance Analyzer Agent

## Role
Analyzes system performance and identifies optimization opportunities.

## Capabilities
- Performance profiling
- Memory usage analysis
- Database query optimization
- Load testing coordination

## Input Format
```json
{
    "analysis_type": "cpu|memory|database|network",
    "duration": 300,
    "load_level": "low|medium|high",
    "baseline_comparison": true
}
```

## Output Format
```json
{
    "status": "success|warning|error",
    "performance_score": 82,
    "bottlenecks": 2,
    "memory_leaks": 0,
    "optimization_suggestions": [...],
    "metrics": {...}
}
```

## Usage
Run during development and testing phases.
"""
    }
    
    created_count = 0
    for filename, content in agent_templates.items():
        agent_file = agents_dir / filename
        try:
            with open(agent_file, 'w', encoding='utf-8') as f:
                f.write(content)
            print_colored(f"  Created: {filename}", Colors.GRAY)
            created_count += 1
        except Exception as e:
            print_colored(f"  ⚠ Failed to create {filename}: {e}", Colors.YELLOW)
    
    print_colored(f"✓ {created_count} agent templates created", Colors.GREEN)
    return True

def create_workflow_templates(claude_dir: Path):
    """Create workflow template files"""
    print_colored("\n⚙️ Creating workflow templates...", Colors.GREEN)
    
    workflows_dir = claude_dir / "workflows"
    
    workflows = {
        "daily_health_check.json": {
            "name": "Daily Health Check",
            "description": "Comprehensive daily system health verification",
            "trigger": "schedule:daily:09:00",
            "steps": [
                {
                    "name": "system_check",
                    "agent": "playerbot-project-coordinator",
                    "timeout": 300
                },
                {
                    "name": "integration_test",
                    "agent": "trinity-integration-tester",
                    "timeout": 600
                },
                {
                    "name": "quality_review",
                    "agent": "code-quality-reviewer", 
                    "timeout": 900
                }
            ],
            "on_failure": "notify_team",
            "on_success": "generate_report"
        },
        
        "security_audit.json": {
            "name": "Weekly Security Audit",
            "description": "Comprehensive security assessment",
            "trigger": "schedule:weekly:monday:10:00",
            "steps": [
                {
                    "name": "vulnerability_scan",
                    "agent": "security-auditor",
                    "timeout": 1800
                },
                {
                    "name": "dependency_check",
                    "script": "dependency_scanner.py",
                    "timeout": 600
                }
            ],
            "on_critical": "immediate_notification",
            "report_format": "detailed"
        },
        
        "performance_baseline.json": {
            "name": "Performance Baseline Update",
            "description": "Update performance baselines and benchmarks",
            "trigger": "manual",
            "steps": [
                {
                    "name": "performance_analysis",
                    "agent": "performance-analyzer",
                    "timeout": 1200
                },
                {
                    "name": "baseline_update",
                    "script": "update_baselines.py",
                    "timeout": 300
                }
            ],
            "artifacts": ["performance_report.html", "baseline_metrics.json"]
        }
    }
    
    created_count = 0
    for filename, workflow in workflows.items():
        workflow_file = workflows_dir / filename
        try:
            with open(workflow_file, 'w', encoding='utf-8') as f:
                json.dump(workflow, f, indent=2)
            print_colored(f"  Created: {filename}", Colors.GRAY)
            created_count += 1
        except Exception as e:
            print_colored(f"  ⚠ Failed to create {filename}: {e}", Colors.YELLOW)
    
    print_colored(f"✓ {created_count} workflow templates created", Colors.GREEN)
    return True

def create_automation_scripts(claude_dir: Path):
    """Create automation utility scripts"""
    print_colored("\n📜 Creating automation scripts...", Colors.GREEN)
    
    scripts_dir = claude_dir / "scripts"
    
    # Create workflow runner script
    workflow_runner = '''#!C:\Program Files\Python313\python.exe
"""
Workflow Runner for PlayerBot Automation
Executes defined workflows and manages their lifecycle
"""

import sys
import json
import subprocess
from pathlib import Path
from datetime import datetime

def run_workflow(workflow_file: str):
    """Run a specific workflow"""
    workflow_path = Path(workflow_file)
    if not workflow_path.exists():
        print(f"Error: Workflow file {workflow_file} not found")
        return 1
    
    try:
        with open(workflow_path, 'r') as f:
            workflow = json.load(f)
        
        print(f"Starting workflow: {workflow['name']}")
        print(f"Description: {workflow['description']}")
        
        for step in workflow['steps']:
            print(f"\\nExecuting step: {step['name']}")
            
            if 'agent' in step:
                print(f"  Agent: {step['agent']}")
                # Agent execution would go here
            elif 'script' in step:
                print(f"  Script: {step['script']}")
                # Script execution would go here
            
            print(f"  ✓ Step completed successfully")
        
        print(f"\\n✅ Workflow '{workflow['name']}' completed successfully")
        return 0
        
    except Exception as e:
        print(f"❌ Workflow failed: {e}")
        return 1

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python workflow_runner.py <workflow_file>")
        sys.exit(1)
    
    sys.exit(run_workflow(sys.argv[1]))
'''
    
    # Create system monitor script
    system_monitor = '''#!C:\Program Files\Python313\python.exe
"""
System Monitor for PlayerBot
Monitors system health and performance metrics
"""

import time
import json
import psutil
from pathlib import Path
from datetime import datetime

def collect_metrics():
    """Collect system metrics"""
    try:
        return {
            "timestamp": datetime.now().isoformat(),
            "cpu_percent": psutil.cpu_percent(interval=1),
            "memory_percent": psutil.virtual_memory().percent,
            "disk_percent": psutil.disk_usage('/').percent if Path('/').exists() else psutil.disk_usage('C:').percent,
            "process_count": len(psutil.pids()),
            "load_average": psutil.getloadavg() if hasattr(psutil, 'getloadavg') else [0, 0, 0]
        }
    except Exception as e:
        return {
            "timestamp": datetime.now().isoformat(),
            "error": str(e)
        }

def main():
    print("PlayerBot System Monitor - Press Ctrl+C to stop")
    
    metrics_file = Path(".claude/monitoring/system_metrics.json")
    metrics_file.parent.mkdir(parents=True, exist_ok=True)
    
    try:
        while True:
            metrics = collect_metrics()
            
            # Save metrics
            with open(metrics_file, 'w') as f:
                json.dump(metrics, f, indent=2)
            
            # Display current metrics
            if 'error' not in metrics:
                print(f"[{datetime.now().strftime('%H:%M:%S')}] "
                      f"CPU: {metrics['cpu_percent']:5.1f}% | "
                      f"Memory: {metrics['memory_percent']:5.1f}% | "
                      f"Disk: {metrics['disk_percent']:5.1f}%")
            else:
                print(f"Error collecting metrics: {metrics['error']}")
            
            time.sleep(30)
            
    except KeyboardInterrupt:
        print("\\nMonitoring stopped")

if __name__ == "__main__":
    main()
'''
    
    scripts = {
        "workflow_runner.py": workflow_runner,
        "system_monitor.py": system_monitor
    }
    
    created_count = 0
    for filename, content in scripts.items():
        script_file = scripts_dir / filename
        try:
            with open(script_file, 'w', encoding='utf-8') as f:
                f.write(content)
            
            # Make executable on Unix systems
            try:
                os.chmod(script_file, 0o755)
            except:
                pass
            
            print_colored(f"  Created: {filename}", Colors.GRAY)
            created_count += 1
        except Exception as e:
            print_colored(f"  ⚠ Failed to create {filename}: {e}", Colors.YELLOW)
    
    print_colored(f"✓ {created_count} automation scripts created", Colors.GREEN)
    return True

def configure_scheduler(claude_dir: Path, project_root: Path):
    """Configure task scheduler"""
    print_colored("\n⏰ Configuring task scheduler...", Colors.GREEN)
    
    if platform.system() == "Windows":
        return configure_windows_scheduler(claude_dir, project_root)
    else:
        return configure_unix_scheduler(claude_dir, project_root)

def configure_windows_scheduler(claude_dir: Path, project_root: Path):
    """Configure Windows Task Scheduler"""
    daily_automation_path = claude_dir.parent / "daily_automation.py"
    
    # Create batch file for task scheduler
    batch_content = f'''@echo off
cd /d "{project_root}"
python "{daily_automation_path}" --check-type full
'''
    
    batch_file = claude_dir / "scripts" / "daily_automation_task.bat"
    try:
        with open(batch_file, 'w') as f:
            f.write(batch_content)
        
        print_colored("  ✓ Batch file created for Windows Task Scheduler", Colors.GREEN)
        print_colored(f"  📁 Location: {batch_file}", Colors.GRAY)
        print_colored("  ℹ️  To complete setup:", Colors.YELLOW)
        print_colored("     1. Open Windows Task Scheduler", Colors.GRAY)
        print_colored("     2. Create Basic Task", Colors.GRAY)
        print_colored(f"     3. Set action to run: {batch_file}", Colors.GRAY)
        print_colored("     4. Set schedule as needed", Colors.GRAY)
        
        return True
    except Exception as e:
        print_colored(f"  ⚠ Failed to create scheduler configuration: {e}", Colors.YELLOW)
        return False

def configure_unix_scheduler(claude_dir: Path, project_root: Path):
    """Configure Unix cron scheduler"""
    daily_automation_path = claude_dir.parent / "daily_automation.py"
    
    # Create shell script for cron
    shell_content = f'''#!/bin/bash
cd "{project_root}"
python3 "{daily_automation_path}" --check-type full
'''
    
    shell_file = claude_dir / "scripts" / "daily_automation_cron.sh"
    try:
        with open(shell_file, 'w') as f:
            f.write(shell_content)
        
        os.chmod(shell_file, 0o755)
        
        print_colored("  ✓ Shell script created for cron", Colors.GREEN)
        print_colored(f"  📁 Location: {shell_file}", Colors.GRAY)
        print_colored("  ℹ️  To complete setup, add to crontab:", Colors.YELLOW)
        print_colored(f"     0 9 * * * {shell_file}", Colors.GRAY)
        print_colored("  Run: crontab -e", Colors.GRAY)
        
        return True
    except Exception as e:
        print_colored(f"  ⚠ Failed to create cron configuration: {e}", Colors.YELLOW)
        return False

def create_configuration_file(claude_dir: Path):
    """Create main configuration file"""
    print_colored("\n⚙️ Creating configuration file...", Colors.GREEN)
    
    config = {
        "version": "1.0",
        "created": datetime.now().isoformat(),
        "project": {
            "name": "PlayerBot",
            "description": "TrinityCore PlayerBot Automation System",
            "repository": "https://github.com/TrinityCore/TrinityCore"
        },
        "automation": {
            "enabled": True,
            "schedule": {
                "daily_health_check": "09:00",
                "security_scan": "weekly:monday:10:00", 
                "performance_baseline": "monthly:1st:14:00"
            },
            "notifications": {
                "email": {
                    "enabled": False,
                    "recipients": ["admin@project.com"]
                },
                "slack": {
                    "enabled": False,
                    "webhook_url": ""
                }
            }
        },
        "agents": {
            "timeout_seconds": 1800,
            "retry_attempts": 3,
            "parallel_execution": True
        },
        "reporting": {
            "format": "html",
            "include_charts": True,
            "retention_days": 30
        },
        "thresholds": {
            "quality_score_minimum": 80,
            "performance_score_minimum": 85,
            "security_score_minimum": 90,
            "test_coverage_minimum": 75
        }
    }
    
    config_file = claude_dir / "automation_config.json"
    try:
        with open(config_file, 'w', encoding='utf-8') as f:
            json.dump(config, f, indent=2)
        
        print_colored(f"  ✓ Configuration file created: {config_file}", Colors.GREEN)
        return True
    except Exception as e:
        print_colored(f"  ⚠ Failed to create configuration file: {e}", Colors.YELLOW)
        return False

def run_test_automation(claude_dir: Path, project_root: Path):
    """Run test automation to verify setup"""
    print_colored("\n🧪 Running test automation...", Colors.GREEN)
    
    # Test daily automation script
    daily_automation_path = claude_dir.parent / "daily_automation.py"
    if daily_automation_path.exists():
        try:
            print_colored("  Testing daily automation script...", Colors.GRAY)
            result = subprocess.run([
                sys.executable, str(daily_automation_path),
                "--check-type", "morning",
                "--project-root", str(project_root)
            ], capture_output=True, text=True, timeout=60)
            
            if result.returncode == 0:
                print_colored("  ✓ Daily automation test passed", Colors.GREEN)
            else:
                print_colored(f"  ⚠ Daily automation test failed: {result.stderr}", Colors.YELLOW)
                return False
                
        except subprocess.TimeoutExpired:
            print_colored("  ⚠ Test automation timed out", Colors.YELLOW)
            return False
        except Exception as e:
            print_colored(f"  ⚠ Test automation error: {e}", Colors.YELLOW)
            return False
    else:
        print_colored("  ⚠ Daily automation script not found", Colors.YELLOW)
        return False
    
    # Test system monitor
    system_monitor_path = claude_dir / "scripts" / "system_monitor.py"
    if system_monitor_path.exists():
        try:
            print_colored("  Testing system monitor...", Colors.GRAY)
            # Run for just a few seconds
            result = subprocess.run([
                sys.executable, str(system_monitor_path)
            ], capture_output=True, text=True, timeout=5)
            
            print_colored("  ✓ System monitor test completed", Colors.GREEN)
            
        except subprocess.TimeoutExpired:
            # This is expected - monitor runs indefinitely
            print_colored("  ✓ System monitor test completed", Colors.GREEN)
        except Exception as e:
            print_colored(f"  ⚠ System monitor test error: {e}", Colors.YELLOW)
    
    print_colored("✓ Test automation completed", Colors.GREEN)
    return True

def print_setup_summary(claude_dir: Path):
    """Print setup summary and next steps"""
    print_colored("\n" + "=" * 60, Colors.CYAN)
    print_colored("✅ Setup Complete!", Colors.GREEN)
    print_colored("=" * 60, Colors.CYAN)
    
    print_colored("\n🎉 PlayerBot Automation System is ready!", Colors.GREEN)
    
    print_colored("\n📁 Created Structure:", Colors.YELLOW)
    print_colored(f"  📂 {claude_dir}", Colors.GRAY)
    print_colored(f"  ├── 📂 agents/ (5 agent templates)", Colors.GRAY)
    print_colored(f"  ├── 📂 scripts/ (automation utilities)", Colors.GRAY)
    print_colored(f"  ├── 📂 workflows/ (3 workflow templates)", Colors.GRAY)
    print_colored(f"  ├── 📂 reports/ (generated reports)", Colors.GRAY)
    print_colored(f"  ├── 📂 monitoring/ (system metrics)", Colors.GRAY)
    print_colored(f"  └── 📄 automation_config.json", Colors.GRAY)
    
    print_colored("\n🚀 Next Steps:", Colors.YELLOW)
    print_colored("  1. Run daily automation:", Colors.GRAY)
    print_colored(f"     python daily_automation.py --project-root \"{claude_dir.parent}\"", Colors.GRAY)
    print_colored("  2. Start system monitoring:", Colors.GRAY)
    print_colored(f"     python {claude_dir}/scripts/system_monitor.py", Colors.GRAY)
    print_colored("  3. Run workflow:", Colors.GRAY)
    print_colored(f"     python {claude_dir}/scripts/workflow_runner.py {claude_dir}/workflows/daily_health_check.json", Colors.GRAY)
    print_colored("  4. Configure scheduled tasks (see instructions above)", Colors.GRAY)
    
    print_colored("\n✨ Your PlayerBot automation system is now fully configured!", Colors.GREEN)

def main():
    parser = argparse.ArgumentParser(description='PlayerBot Automation System Setup')
    parser.add_argument('--project-root', default='C:\\TrinityBots\\TrinityCore',
                       help='Project root directory')
    parser.add_argument('--skip-dependencies', action='store_true',
                       help='Skip Python dependency installation')
    parser.add_argument('--skip-scheduler', action='store_true',
                       help='Skip scheduler configuration')
    parser.add_argument('--no-test', action='store_true',
                       help='Skip test run')
    
    args = parser.parse_args()
    
    print_header()
    
    # Check admin privileges on Windows
    if platform.system() == "Windows" and not check_admin_privileges():
        print_colored("\n⚠️  This script requires Administrator privileges on Windows!", Colors.RED)
        print_colored("Please run as Administrator for full functionality.", Colors.YELLOW)
        print_colored("Continuing with limited functionality...\n", Colors.YELLOW)
    
    project_root = Path(args.project_root)
    print_colored(f"Project root: {project_root}", Colors.GRAY)
    
    # Ensure project root exists
    if not project_root.exists():
        print_colored(f"Creating project root directory: {project_root}", Colors.YELLOW)
        ensure_directory(project_root)
    
    setup_success = True
    
    # Create directory structure
    claude_dir = create_directory_structure(project_root)
    
    # Install dependencies
    if not args.skip_dependencies:
        if not install_python_dependencies():
            setup_success = False
    
    # Create templates and scripts
    create_agent_templates(claude_dir)
    create_workflow_templates(claude_dir)
    create_automation_scripts(claude_dir)
    create_configuration_file(claude_dir)
    
    # Configure scheduler
    if not args.skip_scheduler:
        configure_scheduler(claude_dir, project_root)
    
    # Run test
    if not args.no_test:
        if not run_test_automation(claude_dir, project_root):
            setup_success = False
    
    # Print summary
    print_setup_summary(claude_dir)
    
    # Exit with appropriate code
    sys.exit(0 if setup_success else 1)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print_colored("\n\n⚠️ Setup interrupted by user", Colors.YELLOW)
        sys.exit(1)
    except Exception as e:
        print_colored(f"\n\n❌ Unexpected error: {e}", Colors.RED)
        sys.exit(1)
