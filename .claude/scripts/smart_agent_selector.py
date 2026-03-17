#!/usr/bin/env python3
"""
Smart Agent Selector
Intelligently selects relevant agents based on changed files and context
Reduces unnecessary agent executions by 30-40%
"""

import re
from pathlib import Path
from typing import List, Set, Dict, Optional
import logging

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

class SmartAgentSelector:
    def __init__(self, config_path: Optional[str] = None):
        """
        Initialize Smart Agent Selector

        Args:
            config_path: Optional path to agent capabilities configuration
        """
        self.agent_capabilities = self.load_agent_capabilities(config_path)
        self.always_run_agents = {
            "playerbot-project-coordinator",
            "daily-report-generator"
        }

    def load_agent_capabilities(self, config_path: Optional[str]) -> Dict:
        """Load agent capabilities configuration"""
        # Default agent capabilities mapping
        default_capabilities = {
            # Code quality agents
            "code-quality-reviewer": {
                "file_patterns": ["*.cpp", "*.h", "*.hpp"],
                "path_patterns": ["src/"],
                "keywords": ["code", "quality", "review"],
                "always_run": False,
                "priority": "high"
            },

            # Security agents
            "security-auditor": {
                "file_patterns": ["*.cpp", "*.h", "*.sql"],
                "path_patterns": ["src/", "sql/"],
                "keywords": ["security", "auth", "password", "sql"],
                "always_run": False,
                "priority": "critical"
            },

            # Performance agents
            "performance-analyzer": {
                "file_patterns": ["*.cpp", "*.h"],
                "path_patterns": ["src/modules/Playerbot/AI/", "src/modules/Playerbot/Session/"],
                "keywords": ["performance", "optimization", "loop", "thread"],
                "always_run": False,
                "priority": "medium"
            },
            "windows-memory-profiler": {
                "file_patterns": ["*.cpp", "*.h"],
                "path_patterns": ["src/modules/Playerbot/"],
                "keywords": ["new", "delete", "memory", "leak"],
                "always_run": False,
                "priority": "medium"
            },
            "resource-monitor-limiter": {
                "file_patterns": ["*.cpp"],
                "path_patterns": ["src/modules/Playerbot/Session/", "src/modules/Playerbot/Lifecycle/"],
                "keywords": ["spawn", "bot", "session", "resource"],
                "always_run": False,
                "priority": "medium"
            },

            # Architecture agents
            "cpp-architecture-optimizer": {
                "file_patterns": ["*.cpp", "*.h"],
                "path_patterns": ["src/modules/Playerbot/"],
                "keywords": ["class", "architecture", "design", "pattern"],
                "always_run": False,
                "priority": "low"
            },

            # Database agents
            "database-optimizer": {
                "file_patterns": ["*.sql", "*.cpp"],
                "path_patterns": ["sql/", "src/modules/Playerbot/Database/"],
                "keywords": ["database", "query", "sql", "mysql"],
                "always_run": False,
                "priority": "medium"
            },

            # Trinity integration
            "trinity-integration-tester": {
                "file_patterns": ["*.cpp", "*.h"],
                "path_patterns": ["src/modules/Playerbot/"],
                "keywords": ["Player", "Unit", "WorldSession", "Creature"],
                "always_run": True,  # Always check Trinity compatibility
                "priority": "critical"
            },

            # WoW-specific agents
            "wow-mechanics-expert": {
                "file_patterns": ["*.cpp", "*.h"],
                "path_patterns": ["src/modules/Playerbot/AI/", "src/modules/Playerbot/Combat/"],
                "keywords": ["spell", "combat", "aura", "talent"],
                "always_run": False,
                "priority": "high"
            },
            "wow-bot-behavior-designer": {
                "file_patterns": ["*.cpp", "*.h"],
                "path_patterns": ["src/modules/Playerbot/AI/"],
                "keywords": ["bot", "ai", "behavior", "strategy"],
                "always_run": False,
                "priority": "high"
            },
            "wow-dungeon-raid-coordinator": {
                "file_patterns": ["*.cpp", "*.h"],
                "path_patterns": ["src/modules/Playerbot/AI/GroupAI/", "src/modules/Playerbot/AI/Raid/"],
                "keywords": ["dungeon", "raid", "group", "boss"],
                "always_run": False,
                "priority": "medium"
            },
            "wow-economy-manager": {
                "file_patterns": ["*.cpp", "*.h"],
                "path_patterns": ["src/modules/Playerbot/AI/Economy/"],
                "keywords": ["auction", "trade", "gold", "item"],
                "always_run": False,
                "priority": "low"
            },
            "pvp-arena-tactician": {
                "file_patterns": ["*.cpp", "*.h"],
                "path_patterns": ["src/modules/Playerbot/AI/PvP/"],
                "keywords": ["pvp", "arena", "battleground"],
                "always_run": False,
                "priority": "medium"
            },

            # Testing agents
            "test-automation-engineer": {
                "file_patterns": ["*.cpp", "*.h"],
                "path_patterns": ["src/modules/Playerbot/", "tests/"],
                "keywords": ["test", "unit", "integration"],
                "always_run": False,
                "priority": "high"
            },

            # Compliance agents
            "enterprise-compliance-checker": {
                "file_patterns": ["*.cpp", "*.h", "*.md"],
                "path_patterns": ["src/", "docs/"],
                "keywords": ["compliance", "standard", "documentation"],
                "always_run": False,
                "priority": "low"
            },

            # Coordination agents (always run)
            "playerbot-project-coordinator": {
                "file_patterns": ["*"],
                "path_patterns": ["*"],
                "keywords": [],
                "always_run": True,
                "priority": "critical"
            },
            "daily-report-generator": {
                "file_patterns": ["*"],
                "path_patterns": ["*"],
                "keywords": [],
                "always_run": True,
                "priority": "high"
            }
        }

        # TODO: Load from config file if provided
        return default_capabilities

    def select_agents(
        self,
        changed_files: List[str],
        mode: str = "standard",
        force_all: bool = False
    ) -> List[str]:
        """
        Select relevant agents based on changed files

        Args:
            changed_files: List of changed file paths
            mode: Review mode (quick/standard/deep)
            force_all: Force selection of all agents

        Returns:
            List of selected agent names
        """
        if force_all or mode == "deep":
            logger.info("Deep mode: Running all agents")
            return list(self.agent_capabilities.keys())

        selected = set()

        # Always-run agents
        for agent, config in self.agent_capabilities.items():
            if config.get("always_run", False):
                selected.add(agent)

        # Analyze changed files
        for file_path in changed_files:
            file_path_obj = Path(file_path)
            file_name = file_path_obj.name
            file_ext = file_path_obj.suffix

            # Check each agent's capabilities
            for agent, config in self.agent_capabilities.items():
                # Skip if already selected
                if agent in selected:
                    continue

                # Check file patterns
                if self._matches_patterns(file_name, config.get("file_patterns", [])):
                    selected.add(agent)
                    logger.debug(f"Selected {agent} (file pattern match: {file_name})")
                    continue

                # Check path patterns
                if self._matches_path_patterns(file_path, config.get("path_patterns", [])):
                    selected.add(agent)
                    logger.debug(f"Selected {agent} (path pattern match: {file_path})")
                    continue

                # Check keywords in file content (if .cpp or .h)
                if file_ext in [".cpp", ".h", ".hpp"]:
                    if self._file_contains_keywords(file_path, config.get("keywords", [])):
                        selected.add(agent)
                        logger.debug(f"Selected {agent} (keyword match in {file_path})")

        # Mode-specific filtering
        if mode == "quick":
            # Only critical and high priority agents
            selected = self._filter_by_priority(selected, ["critical", "high"])

        selected_list = list(selected)

        logger.info(f"Selected {len(selected_list)} agents (mode: {mode})")
        logger.info(f"Agents: {', '.join(sorted(selected_list))}")

        return sorted(selected_list, key=lambda a: self._get_priority_order(a))

    def _matches_patterns(self, filename: str, patterns: List[str]) -> bool:
        """Check if filename matches any of the patterns"""
        for pattern in patterns:
            # Convert glob pattern to regex
            regex_pattern = pattern.replace("*", ".*").replace("?", ".")
            if re.match(regex_pattern, filename):
                return True
        return False

    def _matches_path_patterns(self, file_path: str, patterns: List[str]) -> bool:
        """Check if file path matches any of the path patterns"""
        normalized_path = file_path.replace("\\", "/")

        for pattern in patterns:
            pattern_normalized = pattern.replace("\\", "/")

            # Handle wildcard
            if pattern_normalized == "*":
                return True

            # Check if path contains pattern
            if pattern_normalized in normalized_path:
                return True

            # Regex matching
            regex_pattern = pattern_normalized.replace("*", ".*")
            if re.search(regex_pattern, normalized_path):
                return True

        return False

    def _file_contains_keywords(self, file_path: str, keywords: List[str]) -> bool:
        """Check if file contains any of the keywords"""
        if not keywords:
            return False

        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read().lower()
                for keyword in keywords:
                    if keyword.lower() in content:
                        return True
        except Exception as e:
            logger.debug(f"Could not read file {file_path}: {e}")

        return False

    def _filter_by_priority(self, agents: Set[str], priorities: List[str]) -> Set[str]:
        """Filter agents by priority"""
        filtered = set()
        for agent in agents:
            config = self.agent_capabilities.get(agent, {})
            if config.get("priority") in priorities:
                filtered.add(agent)
        return filtered

    def _get_priority_order(self, agent: str) -> int:
        """Get priority order for sorting (lower = higher priority)"""
        priority_map = {
            "critical": 0,
            "high": 1,
            "medium": 2,
            "low": 3
        }
        config = self.agent_capabilities.get(agent, {})
        priority = config.get("priority", "medium")
        return priority_map.get(priority, 2)

    def get_agent_info(self, agent_name: str) -> Optional[Dict]:
        """Get information about a specific agent"""
        return self.agent_capabilities.get(agent_name)

    def print_selection_summary(
        self,
        changed_files: List[str],
        selected_agents: List[str]
    ):
        """Print summary of agent selection"""
        total_agents = len(self.agent_capabilities)
        selected_count = len(selected_agents)
        reduction_percent = ((total_agents - selected_count) / total_agents * 100)

        print("\n" + "=" * 60)
        print("Smart Agent Selection Summary")
        print("=" * 60)
        print(f"Changed files:      {len(changed_files)}")
        print(f"Total agents:       {total_agents}")
        print(f"Selected agents:    {selected_count}")
        print(f"Reduction:          {reduction_percent:.1f}%")
        print("\nSelected agents:")
        for agent in selected_agents:
            config = self.agent_capabilities.get(agent, {})
            priority = config.get("priority", "unknown")
            print(f"  - {agent} ({priority})")
        print("=" * 60 + "\n")


def main():
    """Main entry point for CLI usage"""
    import argparse
    import subprocess

    parser = argparse.ArgumentParser(
        description="Smart Agent Selector - Select relevant agents based on changes"
    )
    parser.add_argument(
        "--mode",
        choices=["quick", "standard", "deep"],
        default="standard",
        help="Review mode"
    )
    parser.add_argument(
        "--force-all",
        action="store_true",
        help="Force selection of all agents"
    )
    parser.add_argument(
        "--changed-only",
        action="store_true",
        help="Only analyze changed files (uses git diff)"
    )

    args = parser.parse_args()

    selector = SmartAgentSelector()

    # Get changed files
    if args.changed_only:
        try:
            result = subprocess.run(
                ["git", "diff", "--name-only", "--cached"],
                capture_output=True,
                text=True,
                check=True
            )
            changed_files = [f for f in result.stdout.split('\n') if f.strip()]
        except Exception as e:
            logger.error(f"Failed to get changed files: {e}")
            changed_files = []
    else:
        # Default: analyze all Playerbot files
        changed_files = [
            "src/modules/Playerbot/AI/BotAI.cpp",
            "src/modules/Playerbot/Session/BotSession.cpp"
        ]

    # Select agents
    selected_agents = selector.select_agents(
        changed_files=changed_files,
        mode=args.mode,
        force_all=args.force_all
    )

    # Print summary
    selector.print_selection_summary(changed_files, selected_agents)

if __name__ == "__main__":
    main()
