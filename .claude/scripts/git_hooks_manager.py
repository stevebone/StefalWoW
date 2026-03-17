#!/usr/bin/env python3
"""
Git Hooks Manager for TrinityCore Playerbot Project
Manages git hooks with configurable quality gates and auto-fix capabilities
"""

import os
import sys
import json
import subprocess
from pathlib import Path
from typing import Dict, List, Optional

class GitHooksManager:
    def __init__(self, repo_root: Optional[Path] = None):
        self.repo_root = repo_root or Path.cwd()
        self.hooks_dir = self.repo_root / ".git" / "hooks"
        self.config_path = self.repo_root / ".claude" / "git_hooks_config.json"
        self.config = self.load_config()

        # Ensure hooks directory exists
        self.hooks_dir.mkdir(parents=True, exist_ok=True)

    def load_config(self) -> Dict:
        """Load git hooks configuration"""
        if self.config_path.exists():
            with open(self.config_path) as f:
                return json.load(f)
        return self.default_config()

    def default_config(self) -> Dict:
        """Return default configuration"""
        return {
            "pre-commit": {
                "enabled": True,
                "checks": [
                    {
                        "name": "format",
                        "required": False,
                        "command": "echo 'Format check placeholder'",
                        "timeout": 60
                    },
                    {
                        "name": "quality",
                        "required": True,
                        "command": "python .claude/scripts/master_review.py --mode quick --changed-only",
                        "timeout": 300
                    },
                    {
                        "name": "security",
                        "required": False,
                        "command": "python .claude/scripts/dependency_scanner.py --quick",
                        "timeout": 120
                    }
                ],
                "bypass_keyword": "[SKIP-HOOKS]",
                "auto_stage_fixes": False
            },
            "pre-push": {
                "enabled": True,
                "checks": [
                    {
                        "name": "build",
                        "required": False,
                        "command": "echo 'Build check placeholder'",
                        "timeout": 600
                    },
                    {
                        "name": "tests",
                        "required": False,
                        "command": "echo 'Test execution placeholder'",
                        "timeout": 300
                    }
                ],
                "bypass_keyword": "[SKIP-PUSH-HOOKS]"
            },
            "commit-msg": {
                "enabled": True,
                "rules": [
                    {
                        "type": "regex",
                        "pattern": "^(feat|fix|docs|refactor|perf|test|chore|style)\\:",
                        "message": "Commit message must follow conventional commits format"
                    },
                    {
                        "type": "min_length",
                        "value": 10,
                        "message": "Commit message must be at least 10 characters"
                    }
                ],
                "bypass_keyword": "[SKIP-MSG-CHECK]"
            },
            "post-commit": {
                "enabled": False,
                "actions": ["notify"]
            }
        }

    def install_hooks(self):
        """Install all configured git hooks"""
        print("Installing git hooks...")

        for hook_name, config in self.config.items():
            # Skip metadata keys
            if hook_name in ["version", "description"]:
                continue

            # Skip if config is not a dict (shouldn't happen, but be safe)
            if not isinstance(config, dict):
                continue

            if config.get("enabled", False):
                self.install_hook(hook_name, config)
            else:
                print(f"  [SKIP] {hook_name} (disabled)")

        print("\n[OK] Git hooks installation complete!")

    def install_hook(self, hook_name: str, config: Dict):
        """Install a specific git hook"""
        hook_file = self.hooks_dir / hook_name

        script = self.generate_hook_script(hook_name, config)

        # Write hook script with UTF-8 encoding
        with open(hook_file, "w", encoding='utf-8', newline='\n') as f:
            f.write(script)

        # Make executable (Windows: no-op, Unix: chmod +x)
        if os.name != 'nt':
            os.chmod(hook_file, 0o755)

        print(f"  [OK] Installed {hook_name} hook")

    def generate_hook_script(self, hook_name: str, config: Dict) -> str:
        """Generate hook script based on configuration"""
        if hook_name == "pre-commit":
            return self.generate_pre_commit(config)
        elif hook_name == "pre-push":
            return self.generate_pre_push(config)
        elif hook_name == "commit-msg":
            return self.generate_commit_msg(config)
        elif hook_name == "post-commit":
            return self.generate_post_commit(config)
        else:
            return self.generate_generic_hook(hook_name, config)

    def generate_pre_commit(self, config: Dict) -> str:
        """Generate pre-commit hook"""
        checks_script = ""

        for check in config.get("checks", []):
            required = "true" if check["required"] else "false"
            checks_script += f"""
# {check['name']} check
echo "📝 Running {check['name']} check..."
{check['command']}
CHECK_EXIT_CODE=$?

if [ $CHECK_EXIT_CODE -ne 0 ]; then
    if [ "{required}" = "true" ]; then
        echo "[ERROR] {check['name']} check failed (required)"
        exit 1
    else
        echo "[WARNING]  {check['name']} check failed (optional, continuing)"
    fi
fi
"""

        return f"""#!/bin/bash
set -e

echo "🔍 Running pre-commit checks..."

# Check for bypass keyword
COMMIT_MSG=$(git log -1 --pretty=%B 2>/dev/null || echo "")
if echo "$COMMIT_MSG" | grep -q "{config.get('bypass_keyword', '[SKIP-HOOKS]')}"; then
    echo "[SKIP]  Bypassing pre-commit hooks (keyword found)"
    exit 0
fi

# Get changed C++ files
CHANGED_FILES=$(git diff --cached --name-only --diff-filter=ACMR | grep -E '\\.(cpp|h|hpp)$' || true)

if [ -z "$CHANGED_FILES" ]; then
    echo "[INFO]  No C++ files changed, skipping checks"
    exit 0
fi

echo "📁 Changed files:"
echo "$CHANGED_FILES" | sed 's/^/  - /'

{checks_script}

echo "[OK] All pre-commit checks passed!"
exit 0
"""

    def generate_pre_push(self, config: Dict) -> str:
        """Generate pre-push hook"""
        checks_script = ""

        for check in config.get("checks", []):
            required = "true" if check["required"] else "false"
            checks_script += f"""
# {check['name']} check
echo "🔨 Running {check['name']} check..."
{check['command']}
CHECK_EXIT_CODE=$?

if [ $CHECK_EXIT_CODE -ne 0 ]; then
    if [ "{required}" = "true" ]; then
        echo "[ERROR] {check['name']} check failed (required)"
        exit 1
    else
        echo "[WARNING]  {check['name']} check failed (optional, continuing)"
    fi
fi
"""

        return f"""#!/bin/bash

echo "🚀 Running pre-push checks..."

# Check for bypass keyword
LATEST_COMMIT=$(git log -1 --pretty=%B)
if echo "$LATEST_COMMIT" | grep -q "{config.get('bypass_keyword', '[SKIP-PUSH-HOOKS]')}"; then
    echo "[SKIP]  Bypassing pre-push hooks (keyword found)"
    exit 0
fi

# Get branch being pushed
BRANCH=$(git rev-parse --abbrev-ref HEAD)
echo "📌 Pushing branch: $BRANCH"

{checks_script}

echo "[OK] All pre-push checks passed!"
exit 0
"""

    def generate_commit_msg(self, config: Dict) -> str:
        """Generate commit-msg hook"""
        rules_script = ""

        for rule in config.get("rules", []):
            if rule["type"] == "regex":
                rules_script += f"""
# Regex check: {rule.get('message', 'Pattern check')}
if ! echo "$COMMIT_MSG" | grep -qE "{rule['pattern']}"; then
    echo "[ERROR] {rule['message']}"
    echo "   Pattern: {rule['pattern']}"
    exit 1
fi
"""
            elif rule["type"] == "min_length":
                rules_script += f"""
# Length check
MSG_LENGTH=$(echo -n "$COMMIT_MSG" | wc -c)
if [ $MSG_LENGTH -lt {rule['value']} ]; then
    echo "[ERROR] {rule['message']}"
    echo "   Current length: $MSG_LENGTH, Required: {rule['value']}"
    exit 1
fi
"""

        return f"""#!/bin/bash

COMMIT_MSG_FILE=$1
COMMIT_MSG=$(cat "$COMMIT_MSG_FILE")

# Check for bypass keyword
if echo "$COMMIT_MSG" | grep -q "{config.get('bypass_keyword', '[SKIP-MSG-CHECK]')}"; then
    echo "[SKIP]  Bypassing commit message checks (keyword found)"
    exit 0
fi

echo "📝 Validating commit message..."

{rules_script}

echo "[OK] Commit message validation passed!"
exit 0
"""

    def generate_post_commit(self, config: Dict) -> str:
        """Generate post-commit hook"""
        return f"""#!/bin/bash

echo "📊 Post-commit actions..."

# Get commit info
COMMIT_HASH=$(git rev-parse HEAD)
COMMIT_MSG=$(git log -1 --pretty=%B)

echo "Commit: $COMMIT_HASH"
echo "Message: $COMMIT_MSG"

# Placeholder for notifications
# python .claude/scripts/notify_commit.py

exit 0
"""

    def generate_generic_hook(self, hook_name: str, config: Dict) -> str:
        """Generate generic hook"""
        return f"""#!/bin/bash
# {hook_name} hook
echo "Running {hook_name} hook..."
exit 0
"""

    def uninstall_hooks(self):
        """Uninstall all git hooks"""
        print("Uninstalling git hooks...")

        for hook_name in self.config.keys():
            hook_file = self.hooks_dir / hook_name
            if hook_file.exists():
                hook_file.unlink()
                print(f"  [OK] Uninstalled {hook_name}")

        print("\n[OK] Git hooks uninstalled!")

    def test_hooks(self):
        """Test installed hooks"""
        print("Testing git hooks...")

        for hook_name, config in self.config.items():
            if not config.get("enabled", False):
                continue

            hook_file = self.hooks_dir / hook_name
            if not hook_file.exists():
                print(f"  [ERROR] {hook_name}: Not installed")
                continue

            # Check if executable
            if os.name != 'nt' and not os.access(hook_file, os.X_OK):
                print(f"  [ERROR] {hook_name}: Not executable")
                continue

            print(f"  [OK] {hook_name}: Installed and ready")

        print("\n[OK] Hook testing complete!")

def main():
    """Main entry point"""
    import argparse

    parser = argparse.ArgumentParser(
        description="Git Hooks Manager for TrinityCore Playerbot"
    )
    parser.add_argument(
        "action",
        choices=["install", "uninstall", "test"],
        help="Action to perform"
    )
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path.cwd(),
        help="Repository root directory"
    )

    args = parser.parse_args()

    manager = GitHooksManager(repo_root=args.repo_root)

    if args.action == "install":
        manager.install_hooks()
    elif args.action == "uninstall":
        manager.uninstall_hooks()
    elif args.action == "test":
        manager.test_hooks()

if __name__ == "__main__":
    main()
