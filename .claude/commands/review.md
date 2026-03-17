# Quick Code Review Command

Run a targeted code review on changed files or specific components.

## Usage

```bash
/review                    # Review all changed files
/review --full             # Full codebase review
/review --path <path>      # Review specific path
/review --mode quick       # Quick review mode
/review --mode standard    # Standard review mode (default)
/review --mode deep        # Deep review mode
/review --auto-fix         # Enable automatic fixing of critical issues
```

## Examples

```bash
# Review only changed files (default)
/review

# Review specific directory
/review --path src/modules/Playerbot/AI

# Quick review with auto-fix
/review --mode quick --auto-fix

# Full codebase deep review
/review --full --mode deep
```

## Execution Steps

1. Identify files to review (changed files, specific path, or full codebase)
2. Select appropriate agents using smart agent selector
3. Run selected agents in parallel where possible
4. Aggregate results and generate summary
5. Apply critical fixes if --auto-fix is enabled
6. Display summary report with actionable items

## Agent Selection

**Quick Mode** (15-30 min):
- playerbot-project-coordinator
- code-quality-reviewer
- security-auditor (critical only)
- trinity-integration-tester

**Standard Mode** (1-2 hours):
- All quick mode agents
- performance-analyzer
- wow-mechanics-expert
- test-automation-engineer

**Deep Mode** (4-6 hours):
- All agents enabled
- Comprehensive analysis
- All optimizations

## Output

The command will generate:
- Console summary with key findings
- Detailed HTML report in `.claude/reports/`
- JSON data export for integration
- Action items list

## Notes

- Always runs security scan for critical vulnerabilities
- Trinity integration check is mandatory
- Auto-fix only applies to critical, safe-to-fix issues
- Results are cached for 4 hours to speed up repeated reviews
