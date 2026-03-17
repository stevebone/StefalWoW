# Complete Code Review Workflow

## Workflow Overview
This workflow orchestrates a comprehensive code review process with automatic issue fixing.

## Workflow Stages

### Stage 1: Coordination & Planning
**Agent**: playerbot-project-coordinator
```yaml
input:
  - source_directory: /src/game/playerbot
  - review_scope: full|partial|changed_files
  - fix_mode: critical_only|all|manual
output:
  - review_plan
  - file_list
  - priority_areas
```

### Stage 2: TrinityCore Integration Check
**Agent**: trinity-integration-tester
```yaml
input:
  - file_list: from_stage_1
  - trinity_version: 3.3.5a
  - check_apis: true
  - check_hooks: true
  - check_database: true
output:
  - compatibility_score
  - api_violations
  - hook_issues
  - database_mismatches
```

### Stage 3: Code Quality Review
**Agent**: code-quality-reviewer
```yaml
input:
  - file_list: from_stage_1
  - trinity_issues: from_stage_2
  - standards:
    - clean_code
    - solid_principles
    - dry_kiss_yagni
output:
  - quality_score
  - code_smells
  - refactoring_suggestions
  - complexity_issues
```

### Stage 4: Security Audit
**Agent**: security-auditor
```yaml
input:
  - file_list: from_stage_1
  - known_issues: from_stage_3
  - scan_depth: deep|normal|quick
output:
  - security_vulnerabilities
  - risk_assessment
  - immediate_fixes_needed
```

### Stage 5: Performance Analysis
**Agent**: performance-analyzer
```yaml
input:
  - file_list: from_stage_1
  - runtime_data: if_available
  - profile_mode: static|dynamic
output:
  - performance_bottlenecks
  - memory_issues
  - optimization_opportunities
```

### Stage 6: Architecture Optimization
**Agent**: cpp-architecture-optimizer
```yaml
input:
  - all_issues: from_stages_2_to_5
  - optimization_level: aggressive|balanced|conservative
output:
  - architecture_improvements
  - design_pattern_suggestions
  - module_restructuring
```

### Stage 7: Enterprise Compliance
**Agent**: enterprise-compliance-checker
```yaml
input:
  - all_results: from_stages_2_to_6
  - compliance_level: strict|standard|relaxed
output:
  - compliance_report
  - enterprise_readiness
  - missing_requirements
```

### Stage 8: Automated Fixing
**Agent**: automated-fix-agent
```yaml
input:
  - all_issues: from_all_stages
  - fix_priority: critical|high|all
  - auto_commit: true|false
output:
  - fixes_applied
  - fixes_pending_review
  - rollback_points
```

### Stage 9: Test Automation
**Agent**: test-automation-engineer
```yaml
input:
  - changed_files: from_stage_8
  - test_scope: unit|integration|full
output:
  - test_results
  - coverage_report
  - regression_status
```

### Stage 10: Daily Report
**Agent**: daily-report-generator
```yaml
input:
  - all_results: from_all_stages
  - report_format: html|markdown|json|all
output:
  - daily_report
  - action_items
  - trend_analysis
```

## Execution Modes

### 1. Quick Review (15-30 minutes)
```bash
Stages: 1 → 3 → 10
Focus: Basic quality check
Auto-fix: Formatting only
```

### 2. Standard Review (1-2 hours)
```bash
Stages: 1 → 2 → 3 → 4 → 8 → 9 → 10
Focus: Quality + Security + Trinity
Auto-fix: Critical issues
```

### 3. Deep Review (4-6 hours)
```bash
Stages: All (1-10)
Focus: Complete analysis
Auto-fix: All issues possible
```

### 4. Continuous Integration Mode
```bash
Trigger: On commit/PR
Stages: 2 → 3 → 4 → 9 → 10
Focus: Changed files only
Auto-fix: Block if critical
```

## Error Handling

### Stage Failures
- **Retry Logic**: 3 attempts with exponential backoff
- **Fallback**: Skip non-critical stages
- **Notification**: Alert on critical stage failure
- **Recovery**: Checkpoint system for resume

### Data Flow Issues
- **Validation**: Check output format between stages
- **Transformation**: Adapt data formats as needed
- **Logging**: Full trace of data flow
- **Debugging**: Stage isolation mode

## Parallel Execution
Stages that can run in parallel:
- Group A: Stages 3, 4, 5 (after Stage 2)
- Group B: Stage 6 (after Group A)
- Sequential: Stages 1, 2, 7, 8, 9, 10

## Configuration
```json
{
  "workflow": {
    "name": "complete_code_review",
    "timeout_minutes": 360,
    "parallel_execution": true,
    "checkpoint_enabled": true,
    "notification_channels": ["email", "slack"],
    "auto_fix_threshold": {
      "critical": "immediate",
      "high": "auto",
      "medium": "suggest",
      "low": "report"
    }
  }
}
```
