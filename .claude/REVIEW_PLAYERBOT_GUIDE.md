# How to Review Entire Playerbot Codebase

## 🎯 Quick Answer

**After restarting Claude Code**, use one of these approaches:

### Option 1: Full Review (Recommended)
```
/review-project src/modules/Playerbot
```

This will systematically review **all** C++ files in the Playerbot module.

### Option 2: Incremental Review (Faster)
Review one module at a time:

```
/review-project src/modules/Playerbot/Core
/review-project src/modules/Playerbot/AI
/review-project src/modules/Playerbot/Database
/review-project src/modules/Playerbot/Strategies
```

### Option 3: Single File Testing
Start with one file to test the system:

```
/review-code src/modules/Playerbot/Core/PlayerBotHooks.cpp
```

---

## 📊 What to Expect

### Playerbot Module Structure

Based on the typical structure:
```
src/modules/Playerbot/
├── Core/           (~15 files)
│   ├── PlayerBotHooks.cpp
│   ├── PlayerBotManager.cpp
│   └── ...
├── AI/             (~25 files)
│   ├── BotAI.cpp
│   ├── strategies/
│   └── ...
├── Database/       (~10 files)
│   ├── BotDatabase.cpp
│   └── ...
├── Strategies/     (~30 files)
│   └── ...
└── Utils/          (~20 files)
    └── ...

Total: ~100 files
```

### Review Time Estimates

| Approach | Files | Time | Detail Level |
|----------|-------|------|--------------|
| Single File | 1 | 30 sec | Full details + AI |
| Single Module | 10-30 | 5-15 min | Full details + AI |
| Full Playerbot | ~100 | 30-60 min | Summary + Top 50 AI |

---

## 🚀 Step-by-Step: Full Playerbot Review

### Step 1: Prepare

Ensure:
- ✅ LM Studio running with qwen3-coder-30b loaded
- ✅ Serena MCP configured in Claude Code
- ✅ Claude Code restarted to load slash commands

### Step 2: Start Full Review

```
/review-project src/modules/Playerbot
```

### Step 3: Watch Progress

You'll see updates like:
```
Scanning directory: src/modules/Playerbot
Found 98 C++ files

Categorizing by module...
- Core: 15 files
- AI: 25 files
- Database: 10 files
- Strategies: 30 files
- Utils: 18 files

Reviewing Core (15 files)...
[===>      ] 30% (4/15) - Current: PlayerBotManager.cpp
  Critical: 2 | Major: 5 | Minor: 3

Reviewing AI (25 files)...
[==>       ] 12% (3/25) - Current: BotAI.cpp
  Critical: 1 | Major: 7 | Minor: 12
```

### Step 4: Get Summary Report

After ~30-60 minutes, you'll receive:

```markdown
# Project Code Review: Playerbot Module

## Executive Summary

Files Reviewed: 98
Lines of Code: ~45,000
Total Issues: 327

### Issue Distribution
- Critical: 23 🔴
- Major: 87 🟠
- Minor: 154 🟡
- Info: 63 ℹ️

**Overall Health Score**: 7.2/10

## Top 10 Critical Issues

1. [NULL-015] Missing AI pointer validation
   File: AI/BotAI.cpp:234
   Impact: Server crash risk

2. [CONC-008] Race condition in statistics
   File: Core/PlayerBotHooks.cpp:566
   Impact: Data corruption

[... continue ...]

## Hot Spots (Files Needing Attention)

1. AI/BotAI.cpp - 45 issues (5 critical)
2. Core/PlayerBotManager.cpp - 32 issues (3 critical)
3. Database/BotDatabase.cpp - 28 issues (4 critical)

## Recommendations

### Immediate (This Week)
1. Fix BotAI.cpp null pointer issues
2. Add mutex to PlayerBotHooks statistics
3. Validate input in BotDatabase.cpp

### This Sprint
1. Refactor BotAI.cpp (high complexity)
2. Add tests for core managers
3. Document public APIs

### Next Sprint
1. Performance optimization in strategy selection
2. Improve error handling across modules
3. Consolidate utility functions
```

---

## 🎛️ Customization Options

### Focus on Specific Issues

```
/review-project src/modules/Playerbot

Focus on:
- Memory leaks
- Thread safety
- SQL injection risks
```

### Severity Filter

```
/review-project src/modules/Playerbot

Show only critical and major issues
```

### Category Focus

```
/review-project src/modules/Playerbot

Security audit: check for SQL injection, buffer overflows, input validation
```

---

## 📈 Incremental Review Strategy (Recommended for First Time)

This approach gives you faster feedback and lets you fix issues as you go:

### Week 1: Core Systems
```
/review-project src/modules/Playerbot/Core
```
**Action**: Fix all critical issues in core before continuing

### Week 2: AI Components
```
/review-project src/modules/Playerbot/AI
```
**Action**: Address major design issues

### Week 3: Database Layer
```
/review-project src/modules/Playerbot/Database
```
**Action**: Security hardening

### Week 4: Strategies & Utils
```
/review-project src/modules/Playerbot/Strategies
/review-project src/modules/Playerbot/Utils
```
**Action**: Performance optimization

### Week 5: Final Sweep
```
/review-project src/modules/Playerbot
```
**Action**: Verify all fixes, final polish

---

## 💡 Pro Tips

### 1. Review in Batches

Don't try to review everything at once:
```
# Monday: Core
/review-project src/modules/Playerbot/Core

# Tuesday: Fix critical issues from Monday

# Wednesday: AI
/review-project src/modules/Playerbot/AI

# Thursday: Fix critical issues from Wednesday

# Friday: Summary review
/review-project src/modules/Playerbot
```

### 2. Focus on Hot Spots First

After first full review, identify hot spots and deep dive:
```
# If BotAI.cpp had 45 issues:
/review-code src/modules/Playerbot/AI/BotAI.cpp

# Get detailed analysis with full AI enhancement
```

### 3. Track Progress

Create a tracking document:
```markdown
# Playerbot Code Review Progress

## Week 1
- [x] Core review complete
- [x] 5 critical issues fixed
- [ ] 12 major issues remaining

## Week 2
- [x] AI review complete
- [ ] Refactoring in progress
```

### 4. Use Git Branches

```bash
# Create review branch
git checkout -b code-review-fixes

# Fix issues incrementally
git commit -m "fix: resolve null pointer in BotAI.cpp"

# Create PR when done
git push origin code-review-fixes
```

---

## 🔍 What Gets Checked

### All 870 Rules Applied

Every file is checked for:

#### Null Safety (221 rules)
- Null pointer dereferences
- Missing null checks
- Unsafe casts
- Optional<T> misuse

#### Memory Management (155 rules)
- Memory leaks
- Use-after-free
- Double delete
- RAII violations
- Smart pointer issues

#### Concurrency (101 rules)
- Race conditions
- Deadlocks
- Unsynchronized access
- Static initialization
- Thread-local storage

#### Security (150 rules)
- SQL injection
- Buffer overflows
- Integer overflows
- Input validation
- Format string vulnerabilities

#### Performance (100 rules)
- Inefficient algorithms
- Unnecessary copies
- String operations
- Container misuse
- Cache misses

#### Conventions (40 rules)
- Naming standards
- Code style
- Documentation
- Error handling

#### Architecture (50 rules)
- God classes
- Circular dependencies
- Layer violations
- SOLID principles

---

## 📋 Sample Output Structure

```markdown
# Playerbot Module Review

## 1. Core Systems (15 files) ✅ REVIEWED

### PlayerBotHooks.cpp
- **Status**: ⚠️ Needs Attention
- **Issues**: 8 (2 critical, 3 major, 3 minor)
- **Top Issue**: Race condition in statistics updates
- **LOC**: 648

### PlayerBotManager.cpp
- **Status**: ⚠️ Needs Attention
- **Issues**: 12 (1 critical, 6 major, 5 minor)
- **Top Issue**: Null pointer in bot creation
- **LOC**: 1,234

[... all Core files ...]

## 2. AI Components (25 files) ✅ REVIEWED

### BotAI.cpp
- **Status**: 🔴 Critical Issues
- **Issues**: 45 (5 critical, 15 major, 25 minor)
- **Top Issue**: Multiple null pointer risks
- **LOC**: 2,456

[... all AI files ...]

## Summary Across All Modules

### Files by Status
- 🔴 Critical Issues: 8 files
- ⚠️ Needs Attention: 35 files
- ✅ Minor Issues Only: 42 files
- 💚 Clean: 13 files

### Top Priority Files
1. AI/BotAI.cpp (45 issues, 5 critical)
2. Core/PlayerBotManager.cpp (32 issues, 3 critical)
3. Database/BotDatabase.cpp (28 issues, 4 critical)
```

---

## 🎯 Expected Results

### What You'll Discover

1. **Critical Bugs**: Null pointers, memory leaks, crashes
2. **Security Issues**: SQL injection risks, buffer overflows
3. **Performance Problems**: Inefficient code, unnecessary allocations
4. **Architecture Issues**: Design flaws, technical debt
5. **Best Practices Violations**: Style, naming, documentation

### Health Score Interpretation

| Score | Status | Action |
|-------|--------|--------|
| 9-10 | Excellent | Maintain quality |
| 7-8 | Good | Address major issues |
| 5-6 | Fair | Needs improvement |
| 3-4 | Poor | Serious refactoring needed |
| 0-2 | Critical | Major overhaul required |

---

## 🚨 Common Issues in Playerbot Code

Based on initial review of PlayerBotHooks.cpp, expect to find:

1. **Thread Safety** (High Priority)
   - Static variable races
   - Unsynchronized counters
   - Shared state without locks

2. **Null Safety** (High Priority)
   - Missing AI pointer checks
   - Unsafe session casts
   - Unvalidated pointers

3. **Performance** (Medium Priority)
   - String allocations in hot paths
   - Inefficient comparisons
   - Unnecessary overhead

4. **Architecture** (Medium Priority)
   - Counter sharing anti-patterns
   - Missing abstraction boundaries
   - Tight coupling

---

## ✅ Next Steps After Review

1. **Triage Issues**
   - Critical: Fix immediately
   - Major: Schedule for next sprint
   - Minor: Add to backlog

2. **Create Tasks**
   ```
   - [ ] Fix null pointer in BotAI.cpp:234
   - [ ] Add mutex to PlayerBotHooks statistics
   - [ ] Refactor BotAI class (God class)
   ```

3. **Add Tests**
   - Unit tests for critical paths
   - Integration tests for bot lifecycle
   - Stress tests for concurrency

4. **Track Metrics**
   - Issues fixed per week
   - Code health score improvement
   - Technical debt reduction

---

## 📞 Support

### If Review Takes Too Long
- Cancel and switch to incremental review
- Focus on specific categories only
- Review subset of files first

### If Too Many Issues Found
- Don't panic! This is normal for large codebases
- Focus on critical issues first
- Tackle one module at a time

### If Unclear on Fixes
- Request detailed explanation for specific issue
- Ask for code examples
- Review TrinityCore coding standards

---

**Ready to Start?**

```
/review-project src/modules/Playerbot
```

Or start small:
```
/review-code src/modules/Playerbot/Core/PlayerBotHooks.cpp
```

The choice is yours! 🚀
