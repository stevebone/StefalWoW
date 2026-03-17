#!C:\Program Files\Python313\python.exe
"""
Master Review Script for PlayerBot Project
Orchestrates complete code review with all agents
"""

import json
import sys
import os
import subprocess
import argparse
import datetime
import time
from pathlib import Path
from typing import Dict, List, Any, Optional
import logging

# Setup logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('review.log'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger(__name__)

class AgentOrchestrator:
    """Orchestrates agent execution for code review"""
    
    def __init__(self, project_root: str, claude_dir: str):
        self.project_root = Path(project_root)
        self.claude_dir = Path(claude_dir)
        self.agents_dir = self.claude_dir / "agents"
        self.workflows_dir = self.claude_dir / "workflows"
        self.reports_dir = self.claude_dir / "reports"
        self.reports_dir.mkdir(exist_ok=True)
        
        # Agent execution order for complete review
        self.agent_chain = [
            "playerbot-project-coordinator",
            "trinity-integration-tester",
            "code-quality-reviewer",
            "security-auditor",
            "performance-analyzer",
            "cpp-architecture-optimizer",
            "enterprise-compliance-checker",
            "automated-fix-agent",
            "test-automation-engineer",
            "daily-report-generator"
        ]
        
        self.results = {}
        self.start_time = datetime.datetime.now()
    
    def execute_agent(self, agent_name: str, input_data: Dict[str, Any]) -> Dict[str, Any]:
        """Execute a single agent via Claude Code"""
        logger.info(f"Executing agent: {agent_name}")
        
        agent_file = self.agents_dir / f"{agent_name}.md"
        if not agent_file.exists():
            logger.error(f"Agent file not found: {agent_file}")
            return {"error": f"Agent {agent_name} not found"}
        
        # Prepare Claude Code command
        command = {
            "agent": agent_name,
            "input": input_data,
            "project_root": str(self.project_root),
            "timestamp": datetime.datetime.now().isoformat()
        }
        
        # Write command file for Claude Code
        command_file = self.claude_dir / f"command_{agent_name}.json"
        with open(command_file, 'w') as f:
            json.dump(command, f, indent=2)
        
        # Execute via Claude Code (simulated - replace with actual Claude Code API)
        try:
            # This would be the actual Claude Code execution
            # result = claude_code.execute(agent_file, command_file)
            
            # Simulated result for demonstration
            result = self.simulate_agent_execution(agent_name, input_data)
            
            logger.info(f"Agent {agent_name} completed successfully")
            return result
            
        except Exception as e:
            logger.error(f"Agent {agent_name} failed: {e}")
            return {"error": str(e), "agent": agent_name}
        finally:
            # Cleanup command file
            if command_file.exists():
                command_file.unlink()
    
    def simulate_agent_execution(self, agent_name: str, input_data: Dict) -> Dict:
        """Simulate agent execution for demonstration"""
        # This is a placeholder - in production, this would call actual Claude Code
        
        simulated_results = {
            "playerbot-project-coordinator": {
                "status": "success",
                "review_plan": "Complete analysis planned",
                "file_list": ["Bot.cpp", "BotAI.cpp", "BotMgr.cpp"],
                "priority_areas": ["memory management", "thread safety"]
            },
            "trinity-integration-tester": {
                "status": "success",
                "compatibility_score": 92,
                "api_violations": [],
                "hook_issues": ["UpdateAI hook needs update"],
                "database_mismatches": []
            },
            "code-quality-reviewer": {
                "status": "success",
                "quality_score": 85,
                "code_smells": ["Long method in BotAI::UpdateAI"],
                "complexity_issues": ["Cyclomatic complexity: 15 in Bot::HandleCombat"]
            },
            "security-auditor": {
                "status": "success",
                "vulnerabilities": {
                    "critical": [],
                    "high": ["Potential buffer overflow in chat parser"],
                    "medium": ["Missing input validation in command handler"]
                }
            },
            "performance-analyzer": {
                "status": "success",
                "performance_score": 78,
                "bottlenecks": ["Database queries in main thread"],
                "memory_issues": ["Potential leak in Bot destructor"]
            },
            "cpp-architecture-optimizer": {
                "status": "success",
                "improvements": ["Consider using smart pointers", "Implement RAII pattern"],
                "design_patterns": ["Strategy pattern for AI behaviors"]
            },
            "enterprise-compliance-checker": {
                "status": "success",
                "compliance_score": 81,
                "violations": {
                    "critical": [],
                    "major": ["Missing error handling in 3 functions"],
                    "minor": ["Naming convention violations"]
                }
            },
            "automated-fix-agent": {
                "status": "success",
                "fixes_applied": [
                    {"file": "Bot.cpp", "line": 234, "issue": "Memory leak", "fixed": True}
                ],
                "fixes_pending": ["Complex refactoring in BotAI.cpp"]
            },
            "test-automation-engineer": {
                "status": "success",
                "test_results": {
                    "passed": 145,
                    "failed": 2,
                    "skipped": 5
                },
                "coverage": 76.5
            },
            "daily-report-generator": {
                "status": "success",
                "report_generated": True,
                "report_location": str(self.reports_dir / f"report_{datetime.date.today()}.html")
            }
        }
        
        return simulated_results.get(agent_name, {"status": "not_implemented"})
    
    def run_complete_review(self, mode: str = "standard") -> Dict[str, Any]:
        """Run complete code review workflow"""
        logger.info(f"Starting complete review in {mode} mode")
        
        # Define execution modes
        modes = {
            "quick": ["playerbot-project-coordinator", "code-quality-reviewer", "daily-report-generator"],
            "standard": self.agent_chain[:-1],  # Exclude some agents for faster execution
            "deep": self.agent_chain  # All agents
        }
        
        agents_to_run = modes.get(mode, modes["standard"])
        
        # Execute agents in sequence
        previous_output = {}
        for agent in agents_to_run:
            # Prepare input based on previous outputs
            agent_input = self.prepare_agent_input(agent, previous_output)
            
            # Execute agent
            result = self.execute_agent(agent, agent_input)
            
            # Store result
            self.results[agent] = result
            previous_output[agent] = result
            
            # Check for critical failures
            if result.get("status") == "critical_failure":
                logger.error(f"Critical failure in {agent}, stopping workflow")
                break
        
        # Generate final report
        self.generate_final_report()
        
        return {
            "status": "completed",
            "mode": mode,
            "duration": str(datetime.datetime.now() - self.start_time),
            "results": self.results,
            "report": str(self.reports_dir / f"final_report_{datetime.date.today()}.json")
        }
    
    def prepare_agent_input(self, agent: str, previous_outputs: Dict) -> Dict:
        """Prepare input for agent based on previous outputs"""
        base_input = {
            "source_directory": str(self.project_root / "src/game/playerbot"),
            "timestamp": datetime.datetime.now().isoformat()
        }
        
        # Add agent-specific inputs based on dependencies
        if agent == "automated-fix-agent":
            base_input["issues"] = {
                "security": previous_outputs.get("security-auditor", {}).get("vulnerabilities", {}),
                "quality": previous_outputs.get("code-quality-reviewer", {}).get("code_smells", []),
                "performance": previous_outputs.get("performance-analyzer", {}).get("bottlenecks", [])
            }
        elif agent == "daily-report-generator":
            base_input["all_results"] = previous_outputs
        
        return base_input
    
    def generate_final_report(self):
        """Generate comprehensive final report"""
        report = {
            "timestamp": datetime.datetime.now().isoformat(),
            "project": "PlayerBot",
            "review_type": "Complete Code Review",
            "summary": {
                "overall_score": self.calculate_overall_score(),
                "critical_issues": self.collect_critical_issues(),
                "action_items": self.generate_action_items()
            },
            "detailed_results": self.results,
            "metrics": self.collect_metrics(),
            "recommendations": self.generate_recommendations()
        }
        
        # Save JSON report
        report_file = self.reports_dir / f"final_report_{datetime.date.today()}.json"
        with open(report_file, 'w') as f:
            json.dump(report, f, indent=2)
        
        logger.info(f"Final report saved to {report_file}")
        
        # Generate HTML report
        self.generate_html_report(report)
    
    def calculate_overall_score(self) -> float:
        """Calculate overall project score"""
        scores = []
        
        if "trinity-integration-tester" in self.results:
            scores.append(self.results["trinity-integration-tester"].get("compatibility_score", 0))
        if "code-quality-reviewer" in self.results:
            scores.append(self.results["code-quality-reviewer"].get("quality_score", 0))
        if "performance-analyzer" in self.results:
            scores.append(self.results["performance-analyzer"].get("performance_score", 0))
        if "enterprise-compliance-checker" in self.results:
            scores.append(self.results["enterprise-compliance-checker"].get("compliance_score", 0))
        
        return sum(scores) / len(scores) if scores else 0
    
    def collect_critical_issues(self) -> List[Dict]:
        """Collect all critical issues from agents"""
        critical = []
        
        # Check security issues
        if "security-auditor" in self.results:
            vulns = self.results["security-auditor"].get("vulnerabilities", {})
            for vuln in vulns.get("critical", []):
                critical.append({"type": "security", "issue": vuln, "severity": "critical"})
        
        # Check performance issues
        if "performance-analyzer" in self.results:
            if self.results["performance-analyzer"].get("memory_issues"):
                for issue in self.results["performance-analyzer"]["memory_issues"]:
                    if "leak" in issue.lower():
                        critical.append({"type": "performance", "issue": issue, "severity": "critical"})
        
        return critical
    
    def generate_action_items(self) -> List[Dict]:
        """Generate prioritized action items"""
        actions = []
        
        # Critical actions
        for issue in self.collect_critical_issues():
            actions.append({
                "priority": "critical",
                "action": f"Fix {issue['type']} issue: {issue['issue']}",
                "deadline": "immediate"
            })
        
        # High priority actions
        if "code-quality-reviewer" in self.results:
            for smell in self.results["code-quality-reviewer"].get("complexity_issues", []):
                actions.append({
                    "priority": "high",
                    "action": f"Refactor: {smell}",
                    "deadline": "this_week"
                })
        
        return actions
    
    def collect_metrics(self) -> Dict:
        """Collect all metrics from agents"""
        return {
            "quality": self.results.get("code-quality-reviewer", {}).get("quality_score", 0),
            "security": 100 - len(self.collect_critical_issues()) * 10,
            "performance": self.results.get("performance-analyzer", {}).get("performance_score", 0),
            "trinity_compatibility": self.results.get("trinity-integration-tester", {}).get("compatibility_score", 0),
            "test_coverage": self.results.get("test-automation-engineer", {}).get("coverage", 0),
            "enterprise_compliance": self.results.get("enterprise-compliance-checker", {}).get("compliance_score", 0)
        }
    
    def generate_recommendations(self) -> List[str]:
        """Generate recommendations based on analysis"""
        recommendations = []
        
        metrics = self.collect_metrics()
        
        if metrics["quality"] < 80:
            recommendations.append("Focus on code quality improvements and refactoring")
        if metrics["test_coverage"] < 80:
            recommendations.append("Increase test coverage to at least 80%")
        if metrics["security"] < 90:
            recommendations.append("Prioritize security vulnerability fixes")
        if metrics["trinity_compatibility"] < 95:
            recommendations.append("Review and update TrinityCore integration points")
        
        return recommendations
    
    def generate_html_report(self, report_data: Dict):
        """Generate HTML report"""
        html = f"""
        <!DOCTYPE html>
        <html>
        <head>
            <title>PlayerBot Code Review Report - {datetime.date.today()}</title>
            <style>
                body {{ font-family: Arial, sans-serif; margin: 20px; }}
                h1 {{ color: #333; }}
                .metric {{ 
                    display: inline-block; 
                    margin: 10px; 
                    padding: 15px; 
                    border: 1px solid #ddd; 
                    border-radius: 5px; 
                }}
                .critical {{ background-color: #ffcccc; }}
                .warning {{ background-color: #ffffcc; }}
                .success {{ background-color: #ccffcc; }}
                table {{ border-collapse: collapse; width: 100%; margin-top: 20px; }}
                th, td {{ border: 1px solid #ddd; padding: 8px; text-align: left; }}
                th {{ background-color: #f2f2f2; }}
            </style>
        </head>
        <body>
            <h1>PlayerBot Code Review Report</h1>
            <p>Generated: {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>
            
            <h2>Overall Score: {report_data['summary']['overall_score']:.1f}/100</h2>
            
            <div class="metrics">
                <div class="metric">Quality: {report_data['metrics']['quality']:.1f}</div>
                <div class="metric">Security: {report_data['metrics']['security']:.1f}</div>
                <div class="metric">Performance: {report_data['metrics']['performance']:.1f}</div>
                <div class="metric">Trinity: {report_data['metrics']['trinity_compatibility']:.1f}</div>
                <div class="metric">Coverage: {report_data['metrics']['test_coverage']:.1f}%</div>
            </div>
            
            <h2>Critical Issues</h2>
            <ul>
                {''.join(f"<li class='critical'>{issue['issue']}</li>" for issue in report_data['summary']['critical_issues'])}
            </ul>
            
            <h2>Action Items</h2>
            <table>
                <tr><th>Priority</th><th>Action</th><th>Deadline</th></tr>
                {''.join(f"<tr><td>{item['priority']}</td><td>{item['action']}</td><td>{item['deadline']}</td></tr>" for item in report_data['summary']['action_items'])}
            </table>
            
            <h2>Recommendations</h2>
            <ul>
                {''.join(f"<li>{rec}</li>" for rec in report_data['recommendations'])}
            </ul>
        </body>
        </html>
        """
        
        html_file = self.reports_dir / f"report_{datetime.date.today()}.html"
        with open(html_file, 'w') as f:
            f.write(html)
        
        logger.info(f"HTML report saved to {html_file}")

def main():
    parser = argparse.ArgumentParser(description="PlayerBot Code Review Orchestrator")
    parser.add_argument("--mode", choices=["quick", "standard", "deep"], default="standard",
                       help="Review mode")
    parser.add_argument("--project-root", default="C:/TrinityBots/TrinityCore",
                       help="Project root directory")
    parser.add_argument("--fix", choices=["critical", "all", "none"], default="critical",
                       help="Auto-fix mode")
    parser.add_argument("--report-only", action="store_true",
                       help="Generate report from existing results")
    
    args = parser.parse_args()
    
    # Initialize orchestrator
    orchestrator = AgentOrchestrator(
        project_root=args.project_root,
        claude_dir=Path(args.project_root) / ".claude"
    )
    
    # Run review
    if not args.report_only:
        results = orchestrator.run_complete_review(mode=args.mode)
        print(f"\nReview completed: {results['status']}")
        print(f"Duration: {results['duration']}")
        print(f"Report: {results['report']}")
    else:
        orchestrator.generate_final_report()
        print("Report generated from existing results")

if __name__ == "__main__":
    main()
