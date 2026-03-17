# PlayerBot Automation System

## Overview
Complete automation system for PlayerBot code review, quality assurance, and continuous integration.

## Features
- 🔍 **Comprehensive Code Review**: Multi-agent analysis system
- 🔒 **Security Scanning**: Vulnerability detection and auto-fixing
- 🚀 **Performance Analysis**: Profiling and optimization
- ✅ **Automated Testing**: Unit and integration test execution
- 📊 **Daily Reports**: HTML, Markdown, and JSON reporting
- 🔧 **Auto-Fix**: Automatic correction of critical issues
- ⏰ **Scheduled Execution**: Windows Task Scheduler integration
- 📧 **Notifications**: Email and Slack alerts

## Architecture

### Agent System
The system uses specialized agents for different aspects of code review:

#### Core Review Agents
- **playerbot-project-coordinator**: Orchestrates the review process
- **trinity-integration-tester**: Validates TrinityCore compatibility
- **code-quality-reviewer**: Analyzes code quality and standards
- **cpp-architecture-optimizer**: Optimizes C++ architecture

#### Security & Performance
- **security-auditor**: Detects vulnerabilities and security issues
- **performance-analyzer**: Profiles and identifies bottlenecks
- **windows-memory-profiler**: Memory leak detection

#### Automation & Reporting
- **automated-fix-agent**: Automatically fixes critical issues
- **test-automation-engineer**: Runs and manages tests
- **daily-report-generator**: Creates comprehensive reports

#### Specialized Agents
- **enterprise-compliance-checker**: Ensures enterprise standards
- **database-optimizer**: Database performance tuning
- **resource-monitor-limiter**: Resource usage monitoring

## Installation

### Prerequisites
- Windows 10/11 or Windows Server 2019+
- PowerShell 5.1 or higher
- Python 3.8+ (for advanced features)
- Claude Code installed and configured
- Administrator privileges (for Task Scheduler)

### Quick Setup
```powershell
# Run as Administrator
cd C:\TrinityBots\TrinityCore\.claude\scripts
.\setup_automation.ps1 -InstallDependencies -ConfigureScheduler -TestRun
```

### Manual Setup
1. Clone/copy all files to `.claude` directory
2. Install Python dependencies:
   ```bash
   pip install -r scripts/requirements.txt
   ```
3. Configure Task Scheduler:
   ```powershell
   schtasks /create /xml scripts/task_scheduler_config.xml /tn PlayerBot_DailyReview
   ```

## Usage

### Command Line Interface

#### PowerShell Script
```powershell
# Full review
.\daily_automation.ps1 -CheckType full -AutoFix

# Morning checks only
.\daily_automation.ps1 -CheckType morning

# Security scan with auto-fix
.\daily_automation.ps1 -CheckType critical -AutoFix -EmailReport
```

#### Python Script
```bash
# Standard review
python master_review.py --mode standard

# Deep review with all agents
python master_review.py --mode deep --fix all

# Quick review
python master_review.py --mode quick
```

#### Batch Script
```batch
# Run with specific check type
run_daily_checks.bat --type full --autofix --email
```

### Desktop Shortcuts
After setup, use the created desktop shortcuts:
- **PlayerBot Daily Review**: Full code review
- **PlayerBot Quick Check**: Morning health checks
- **PlayerBot Reports**: Open reports folder

## Workflow Modes

### Quick Review (15-30 min)
- Basic quality check
- Format verification
- Simple fixes only

### Standard Review (1-2 hours)
- Complete quality analysis
- Security scanning
- Trinity compatibility check
- Critical issue auto-fix

### Deep Review (4-6 hours)
- All agents executed
- Complete analysis
- All possible auto-fixes
- Comprehensive reporting

## Daily Schedule

| Time | Check Type | Description |
|------|------------|-------------|
| 06:00 | Morning Health | Smoke tests, basic checks |
| 07:00 | Build Verification | Compilation, static analysis |
| 08:00 | Security Scan | Vulnerability detection |
| 12:00 | Performance | Profiling, metrics |
| 14:00 | Database | Integrity, optimization |
| 18:00 | Full Review | Complete analysis & report |

## Configuration

### Main Configuration File
`C:\TrinityBots\TrinityCore\.claude\automation_config.json`

```json
{
  "automation": {
    "auto_fix": {
      "critical": true,
      "high": false
    },
    "notifications": {
      "email": {
        "enabled": true,
        "recipients": ["team@project.com"]
      }
    }
  },
  "thresholds": {
    "quality_score": 80,
    "security_score": 90
  }
}
```

### Customizing Agents
Edit agent files in `.claude/agents/` to modify behavior:
```markdown
# agent-name.md
## Tasks
- Custom task 1
- Custom task 2

## Output Format
```

## Reports

### Report Types
- **HTML**: Visual dashboard with metrics
- **Markdown**: Detailed text report
- **JSON**: Machine-readable data

### Report Locations
- Daily: `.claude/reports/daily_report_YYYY-MM-DD.html`
- Final: `.claude/reports/final_report_YYYY-MM-DD.json`
- Logs: `.claude/logs/automation_YYYY-MM-DD.log`

### Metrics Tracked
- Code quality score
- Security vulnerabilities
- Performance bottlenecks
- Test coverage
- Trinity compatibility
- Enterprise compliance

## Troubleshooting

### Common Issues

#### Task Scheduler Not Running
```powershell
# Check task status
Get-ScheduledTask -TaskName "PlayerBot_*"

# Enable task
Enable-ScheduledTask -TaskName "PlayerBot_DailyReview"
```

#### Python Dependencies Missing
```bash
python -m pip install --upgrade pip
pip install -r scripts/requirements.txt
```

#### Permission Errors
- Run PowerShell as Administrator
- Check file permissions on `.claude` directory

### Logs
Check logs for detailed error information:
- Automation log: `.claude/automation.log`
- Daily logs: `.claude/logs/automation_YYYY-MM-DD.log`
- Agent-specific logs in reports

## Advanced Features

### Custom Workflows
Create custom workflow in `.claude/workflows/`:
```yaml
name: custom_workflow
agents:
  - agent1:
      config: value
  - agent2:
      depends_on: agent1
```

### Parallel Execution
Enable in config for faster execution:
```json
{
  "workflow": {
    "parallel_execution": true,
    "max_parallel_agents": 4
  }
}
```

### CI/CD Integration
```yaml
# GitHub Actions example
- name: Run PlayerBot Review
  run: |
    cd .claude/scripts
    python master_review.py --mode standard
```

## Best Practices

1. **Regular Reviews**: Run at least daily
2. **Fix Critical Issues**: Always auto-fix critical vulnerabilities
3. **Monitor Trends**: Track metrics over time
4. **Update Agents**: Keep agent definitions current
5. **Test Changes**: Run quick check after major changes
6. **Archive Reports**: Keep monthly summaries

## Support

### Documentation
- Agent specifications: `.claude/agents/*.md`
- Workflow definitions: `.claude/workflows/*.md`
- Phase documentation: `.claude/PHASE*/`

### Maintenance
- Update agents monthly
- Review thresholds quarterly
- Clean old reports (>90 days)
- Backup configuration

## License
Internal use only - PlayerBot Project

## Changelog

### Version 1.0.0 (2024-01-23)
- Initial release
- Complete agent system
- Automated scheduling
- Multi-format reporting
- Auto-fix capabilities

---

For questions or issues, contact the PlayerBot development team.
