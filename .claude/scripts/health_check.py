#!C:\Program Files\Python313\python.exe
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
    
    print(f"\nResults: {pass_count} passed, {fail_count} failed, {info_count} info")
    print(f"Overall Status: {results['overall_status'].upper()}")
    
    if fail_count > 0:
        print("\n❌ Failed Checks:")
        for check in results["checks"]:
            if check["status"] == "fail":
                print(f"  - {check['name']}: {check['message']}")
    
    if results["overall_status"] == "good":
        print("\n✅ All critical systems operational!")
        return 0
    else:
        print("\n⚠️ Some issues detected - see above")
        return 1

if __name__ == "__main__":
    import sys
    sys.exit(main())
