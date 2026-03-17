#!C:\Program Files\Python313\python.exe
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
    
    print("\nAvailable rollback points:")
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
    print(f"\n⚠️  Performing emergency rollback to: {latest.name}")
    
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
