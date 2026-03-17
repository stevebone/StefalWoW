#!C:\Program Files\Python313\python.exe
"""
Agent Status Dashboard - Shows which agents are running when
"""

import json
import datetime
from pathlib import Path
from typing import Dict, List

class AgentDashboard:
    def __init__(self):
        self.agents = {
            # Code Review Agents
            "playerbot-project-coordinator": {
                "workflows": ["code-review"],
                "schedule": None,
                "priority": "HIGH",
                "category": "Coordination"
            },
            "trinity-integration-tester": {
                "workflows": ["code-review", "daily-checks"],
                "schedule": ["06:00", "14:00"],
                "priority": "CRITICAL",
                "category": "Integration"
            },
            "code-quality-reviewer": {
                "workflows": ["code-review", "daily-checks"],
                "schedule": ["07:00"],
                "priority": "HIGH",
                "category": "Quality"
            },
            "security-auditor": {
                "workflows": ["code-review", "daily-checks"],
                "schedule": ["08:00"],
                "priority": "CRITICAL",
                "category": "Security"
            },
            "performance-analyzer": {
                "workflows": ["code-review", "daily-checks"],
                "schedule": ["06:00", "12:00"],
                "priority": "HIGH",
                "category": "Performance"
            },
            "cpp-architecture-optimizer": {
                "workflows": ["code-review", "daily-checks"],
                "schedule": ["07:00"],
                "priority": "MEDIUM",
                "category": "Architecture"
            },
            "enterprise-compliance-checker": {
                "workflows": ["code-review"],
                "schedule": None,
                "priority": "MEDIUM",
                "category": "Compliance"
            },
            "automated-fix-agent": {
                "workflows": ["code-review", "daily-checks"],
                "schedule": "on-trigger",
                "priority": "CRITICAL",
                "category": "Automation"
            },
            "test-automation-engineer": {
                "workflows": ["code-review", "daily-checks"],
                "schedule": ["06:00"],
                "priority": "HIGH",
                "category": "Testing"
            },
            "daily-report-generator": {
                "workflows": ["code-review", "daily-checks"],
                "schedule": ["18:00"],
                "priority": "HIGH",
                "category": "Reporting"
            },
            
            # Support Agents
            "database-optimizer": {
                "workflows": ["daily-checks"],
                "schedule": ["14:00"],
                "priority": "MEDIUM",
                "category": "Database"
            },
            "resource-monitor-limiter": {
                "workflows": ["daily-checks"],
                "schedule": ["16:00"],
                "priority": "MEDIUM",
                "category": "Resources"
            },
            "windows-memory-profiler": {
                "workflows": ["daily-checks"],
                "schedule": ["12:00"],
                "priority": "HIGH",
                "category": "Memory"
            },
            "concurrency-threading-specialist": {
                "workflows": ["on-demand"],
                "schedule": None,
                "priority": "LOW",
                "category": "Threading"
            },
            "cpp-server-debugger": {
                "workflows": ["on-demand"],
                "schedule": None,
                "priority": "LOW",
                "category": "Debugging"
            },
            
            # Gaming Agents
            "bot-learning-system": {
                "workflows": ["gameplay-testing"],
                "schedule": None,
                "priority": "LOW",
                "category": "Gaming/AI"
            },
            "pvp-arena-tactician": {
                "workflows": ["gameplay-testing"],
                "schedule": None,
                "priority": "LOW",
                "category": "Gaming/PvP"
            },
            "wow-bot-behavior-designer": {
                "workflows": ["gameplay-testing"],
                "schedule": None,
                "priority": "LOW",
                "category": "Gaming/Behavior"
            },
            "wow-dungeon-raid-coordinator": {
                "workflows": ["gameplay-testing"],
                "schedule": None,
                "priority": "LOW",
                "category": "Gaming/PvE"
            },
            "wow-economy-manager": {
                "workflows": ["gameplay-testing"],
                "schedule": None,
                "priority": "LOW",
                "category": "Gaming/Economy"
            },
            "wow-mechanics-expert": {
                "workflows": ["gameplay-testing"],
                "schedule": None,
                "priority": "LOW",
                "category": "Gaming/Combat"
            }
        }
    
    def get_schedule_timeline(self) -> Dict[str, List[str]]:
        """Get agents organized by scheduled time"""
        timeline = {}
        
        for agent, info in self.agents.items():
            if info["schedule"]:
                if info["schedule"] == "on-trigger":
                    if "on-trigger" not in timeline:
                        timeline["on-trigger"] = []
                    timeline["on-trigger"].append(agent)
                else:
                    for time in info["schedule"]:
                        if time not in timeline:
                            timeline[time] = []
                        timeline[time].append(agent)
        
        return dict(sorted(timeline.items()))
    
    def get_agents_by_workflow(self) -> Dict[str, List[str]]:
        """Get agents organized by workflow"""
        workflows = {}
        
        for agent, info in self.agents.items():
            for workflow in info["workflows"]:
                if workflow not in workflows:
                    workflows[workflow] = []
                workflows[workflow].append(agent)
        
        return workflows
    
    def get_agents_by_priority(self) -> Dict[str, List[str]]:
        """Get agents organized by priority"""
        priorities = {"CRITICAL": [], "HIGH": [], "MEDIUM": [], "LOW": []}
        
        for agent, info in self.agents.items():
            priorities[info["priority"]].append(agent)
        
        return priorities
    
    def print_dashboard(self):
        """Print a formatted dashboard"""
        print("\n" + "="*80)
        print(" " * 25 + "PLAYERBOT AGENT DASHBOARD")
        print("="*80)
        
        # Timeline view
        print("\n📅 DAILY SCHEDULE")
        print("-" * 40)
        timeline = self.get_schedule_timeline()
        for time, agents in timeline.items():
            if time == "on-trigger":
                print(f"⚡ On-Trigger:")
            else:
                print(f"🕐 {time}:")
            for agent in agents:
                category = self.agents[agent]["category"]
                priority = self.agents[agent]["priority"]
                emoji = "🔴" if priority == "CRITICAL" else "🟡" if priority == "HIGH" else "🟢"
                print(f"   {emoji} {agent} ({category})")
        
        # Workflow view
        print("\n🔄 WORKFLOW ASSIGNMENTS")
        print("-" * 40)
        workflows = self.get_agents_by_workflow()
        for workflow, agents in workflows.items():
            print(f"\n📋 {workflow.replace('-', ' ').title()}: {len(agents)} agents")
            if len(agents) <= 5:
                for agent in agents[:5]:
                    print(f"   • {agent}")
            else:
                for agent in agents[:3]:
                    print(f"   • {agent}")
                print(f"   • ... and {len(agents)-3} more")
        
        # Priority view
        print("\n⚠️ PRIORITY LEVELS")
        print("-" * 40)
        priorities = self.get_agents_by_priority()
        for priority, agents in priorities.items():
            emoji = "🔴" if priority == "CRITICAL" else "🟡" if priority == "HIGH" else "🟢" if priority == "MEDIUM" else "⚪"
            print(f"{emoji} {priority}: {len(agents)} agents")
        
        # Statistics
        print("\n📊 STATISTICS")
        print("-" * 40)
        total_agents = len(self.agents)
        scheduled = sum(1 for a in self.agents.values() if a["schedule"])
        active = sum(1 for a in self.agents.values() if "daily-checks" in a["workflows"] or "code-review" in a["workflows"])
        gaming = sum(1 for a in self.agents.values() if "gameplay-testing" in a["workflows"])
        
        print(f"Total Agents: {total_agents}")
        print(f"Scheduled Agents: {scheduled}")
        print(f"Active in Automation: {active}")
        print(f"Gaming-Specific: {gaming}")
        
        print("\n" + "="*80)
        print(f"Generated: {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        print("="*80 + "\n")
    
    def export_json(self, filepath: str):
        """Export dashboard data as JSON"""
        data = {
            "generated": datetime.datetime.now().isoformat(),
            "agents": self.agents,
            "timeline": self.get_schedule_timeline(),
            "workflows": self.get_agents_by_workflow(),
            "priorities": self.get_agents_by_priority(),
            "statistics": {
                "total": len(self.agents),
                "scheduled": sum(1 for a in self.agents.values() if a["schedule"]),
                "active": sum(1 for a in self.agents.values() if "daily-checks" in a["workflows"] or "code-review" in a["workflows"]),
                "gaming": sum(1 for a in self.agents.values() if "gameplay-testing" in a["workflows"])
            }
        }
        
        with open(filepath, 'w') as f:
            json.dump(data, f, indent=2)
        
        print(f"Dashboard data exported to: {filepath}")

def main():
    dashboard = AgentDashboard()
    
    # Print to console
    dashboard.print_dashboard()
    
    # Export to JSON
    dashboard.export_json("agent_dashboard.json")
    
    # Quick status check
    current_hour = datetime.datetime.now().hour
    current_time = f"{current_hour:02d}:00"
    
    timeline = dashboard.get_schedule_timeline()
    if current_time in timeline:
        print(f"\n⏰ AGENTS SCHEDULED FOR NOW ({current_time}):")
        for agent in timeline[current_time]:
            print(f"   ▶️ {agent}")
    else:
        print(f"\n⏰ No agents scheduled for {current_time}")

if __name__ == "__main__":
    main()
