# AI Code Review - Project/Directory Review

You are performing an AI-powered code review of an entire project directory using two-step orchestration.

## Your Task

Review all C++ files in the specified directory systematically.

### Step 1: Discover Files

Use glob patterns to find all C++ files:
- `**/*.cpp` - All implementation files
- `**/*.h` - All header files
- `**/*.hpp` - Alternative header files

### Step 2: Categorize Files

Group files by subdirectory/module for organized review:
- Core systems (managers, orchestrators)
- AI components (strategies, behaviors)
- Database layer (repositories, queries)
- Utilities (helpers, tools)

### Step 3: Review Each File

For each file:
1. **Parse with Serena MCP**: Get symbols and structure
2. **Apply 870+ Rules**: Check all categories
3. **AI Enhancement**: Use LM Studio for critical/major issues only (to save time)
4. **Track Progress**: Show current file being reviewed

### Step 4: Generate Summary Report

Create comprehensive project-level report:
- Total files reviewed
- Total issues found by category and severity
- Top 10 most critical issues across all files
- Files with most issues (hot spots)
- Overall project health score

## Priority Focus

Since full project reviews can be extensive, prioritize:

1. **Critical Issues First**: Focus on null safety, memory leaks, security
2. **Core Files**: Review managers and orchestrators before utilities
3. **AI Enhancement**: Only for top 20 critical issues (not all)
4. **Batch Processing**: Review 5-10 files at a time with progress updates

## Output Format

```markdown
# Project Code Review: {project-name}

## Executive Summary

**Review Date**: {date}
**Files Reviewed**: {total-files}
**Lines of Code**: ~{loc}
**Total Issues**: {count}

### Issue Distribution
- Critical: {count} 🔴
- Major: {count} 🟠
- Minor: {count} 🟡
- Info: {count} ℹ️

### Category Breakdown
| Category | Issues | % of Total |
|----------|--------|------------|
| Null Safety | {count} | {percent}% |
| Memory | {count} | {percent}% |
| Concurrency | {count} | {percent}% |
| Security | {count} | {percent}% |
| Performance | {count} | {percent}% |
| Conventions | {count} | {percent}% |
| Architecture | {count} | {percent}% |

**Overall Health Score**: {score}/10

---

## Top 10 Critical Issues

### 1. [{rule-id}] {title}
**File**: {filename}:{line}
**Impact**: HIGH - {why-it-matters}

**AI Analysis** (qwen3-coder-30b):
{detailed-explanation}

**Recommended Fix**:
```cpp
{fix-code}
```

---

[Continue for top 10]

---

## Files Reviewed

### Core Systems ({count} files)

#### ✅ {filename} - Clean
- Issues: 0
- Lines: {loc}

#### ⚠️ {filename} - Needs Attention
- Critical: {count}
- Major: {count}
- Lines: {loc}
- **Top Issue**: {brief-description}

[Continue for all files...]

---

## Hot Spots (Files with Most Issues)

1. **{filename}** - {total-issues} issues ({critical} critical)
2. **{filename}** - {total-issues} issues ({critical} critical)
3. **{filename}** - {total-issues} issues ({critical} critical)

---

## Recommendations

### Immediate Actions (Critical)
1. Fix {filename}:{line} - {issue-summary}
2. Fix {filename}:{line} - {issue-summary}
3. Fix {filename}:{line} - {issue-summary}

### Before Next Release (Major)
1. Refactor {filename} - {issue-summary}
2. Optimize {filename} - {issue-summary}
3. Document {filename} - {issue-summary}

### Technical Debt (Minor)
1. Cleanup {filename} - {issue-summary}
2. Improve {filename} - {issue-summary}

---

## Architecture Insights

### Design Patterns Observed
- Singleton: {count} instances
- Factory: {count} instances
- Observer: {count} instances
- Strategy: {count} instances

### Architectural Issues
- God classes detected: {count}
- Circular dependencies: {count}
- Layer violations: {count}

### Positive Patterns
- Good RAII usage in {count} files
- Proper null checks in {count} locations
- Consistent naming in {count} files

---

## Test Coverage Recommendations

Files that need unit tests:
1. {filename} - Complex logic, no tests
2. {filename} - Critical functionality, no tests
3. {filename} - High cyclomatic complexity

---

## Security Assessment

**Security Score**: {score}/10

### Vulnerabilities Found
- SQL Injection risks: {count}
- Buffer overflows: {count}
- Input validation issues: {count}

### Security Recommendations
1. {recommendation}
2. {recommendation}
3. {recommendation}

---

## Performance Assessment

**Performance Score**: {score}/10

### Performance Issues
- Inefficient algorithms: {count}
- Unnecessary copies: {count}
- String operations in loops: {count}

### Performance Recommendations
1. {recommendation}
2. {recommendation}

---

## Next Steps

1. **Priority 1 (This Week)**
   - Fix all critical issues
   - Review hot spot files in detail

2. **Priority 2 (This Sprint)**
   - Address major issues
   - Add tests for critical paths

3. **Priority 3 (Next Sprint)**
   - Refactor problematic areas
   - Improve documentation

---

**Review Complete** ✅
```

## Workflow

### Phase 1: Discovery (Quick)
```
Scanning directory: {path}
Found {count} C++ files
Categorizing by module...
```

### Phase 2: Batch Review (10-30 min depending on size)
```
Reviewing Core Systems (15 files)...
[===>      ] 30% (4/15) - Current: PlayerBotManager.cpp
  - Critical: 2 found
  - Major: 5 found

Reviewing AI Components (23 files)...
[===>      ] 15% (3/23) - Current: BotAI.cpp
  - Critical: 0 found
  - Major: 3 found
```

### Phase 3: Aggregation & Analysis (1-2 min)
```
Aggregating results across {count} files...
Identifying hot spots...
Running project-level analysis...
Generating summary report...
```

### Phase 4: AI Enhancement (5-10 min)
```
AI enhancing top 20 critical issues with LM Studio...
[====>     ] 40% (8/20)
```

## Optimization Strategies

### For Large Projects (100+ files)

1. **Incremental Review**: Review one module at a time
2. **Filter by Severity**: Start with critical/major only
3. **Sample Strategy**: Review 20% of files first, then full scan
4. **Category Focus**: One category at a time (e.g., security first)

### Example: Staged Review

```
# Stage 1: Core systems only
/review-project src/modules/Playerbot/Core

# Stage 2: AI components
/review-project src/modules/Playerbot/AI

# Stage 3: Database layer
/review-project src/modules/Playerbot/Database

# Stage 4: Utilities
/review-project src/modules/Playerbot/Utils
```

## Time Estimates

| Project Size | Files | Estimated Time | AI Enhancement |
|--------------|-------|----------------|----------------|
| Small | 1-10 | 2-5 min | Top 10 issues |
| Medium | 11-50 | 10-15 min | Top 20 issues |
| Large | 51-100 | 20-30 min | Top 30 issues |
| Very Large | 100+ | 40-60 min | Top 50 issues |

## Guidelines

1. **Show Progress**: Update user every 5 files reviewed
2. **Be Selective with AI**: Don't enhance every issue (too slow)
3. **Prioritize**: Critical issues first, info issues last
4. **Group Results**: By module/category for easier navigation
5. **Actionable**: Focus on what to fix now vs. later

## Example Commands

### Full Playerbot Review
```
/review-project src/modules/Playerbot
```

### Specific Module
```
/review-project src/modules/Playerbot/AI
```

### With Focus
```
/review-project src/modules/Playerbot/Database

Focus on security issues: SQL injection, input validation
```

Now perform the project review on the directory specified by the user.
