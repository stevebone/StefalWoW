---
# Playerbot Bug Fix Workflow
# Schnelle, verifizierte Bug-Fixes
---

## Configuration
- *Artifacts Path*: {@artifacts_path} → `.zenflow/tasks/{task_id}`
- *Primary Model*: Claude Sonnet 4
- *Verification Model*: Claude Opus 4
- *Max Iterations*: 3

---

## Agent Instructions

You are fixing a bug in the TrinityCore Playerbot module. Focus on minimal, targeted fixes that don't introduce regressions.

### Critical Rules
1. **Minimal Changes**: Fix only what's broken, don't refactor
2. **Root Cause**: Understand why the bug occurs before fixing
3. **Regression Prevention**: Ensure fix doesn't break related functionality
4. **Thread Safety**: Check if bug is related to threading issues

---

## Workflow Steps

### [ ] Step 1: Bug Analysis
Analyze the bug:
1. Reproduce the issue (understand symptoms)
2. Identify the root cause
3. Check crash logs if available (`.claude/crash_analysis_queue/`)
4. Determine affected components

Save analysis to `{@artifacts_path}/analysis.md`

### [ ] Step 2: Fix Planning
Plan the fix:
1. **Root Cause**: What exactly causes the bug?
2. **Fix Strategy**: How will we fix it?
3. **Risk Assessment**: What could break?
4. **Files to Modify**: List specific files

### [ ] Step 3: Implementation
Implement the fix:

**For Null Pointer Crashes:**
```cpp
// Before
bot->GetPlayerbotAI()->DoSomething();

// After
if (bot && bot->IsInWorld())
{
    if (PlayerbotAI* ai = bot->GetPlayerbotAI())
    {
        ai->DoSomething();
    }
}
```

**For Race Conditions:**
```cpp
// Add proper synchronization
std::shared_lock lock(m_mutex);
// Read shared data
```

**For Memory Leaks:**
```cpp
// Replace raw pointers
std::unique_ptr<Object> obj = std::make_unique<Object>();
```

### [ ] Step 4: Build & Test
```powershell
cmake --build build --config RelWithDebInfo -- /m
```

If build fails, fix errors and retry (max 3 iterations).

### [ ] Step 5: Verification
**Agent Switch**: Use different model to verify.

Verify the fix:
1. Does it address the root cause?
2. Are there any side effects?
3. Is thread safety maintained?
4. Are there similar bugs elsewhere?

### [ ] Step 6: Documentation
Document the fix:
```markdown
## Bug Fix: [Title]
- **Root Cause**: [Explanation]
- **Fix**: [What was changed]
- **Files Modified**: [List]
- **Testing**: [How to verify]
```

Save to `{@artifacts_path}/fix_report.md`

---

## Completion Criteria
- [ ] Build passes
- [ ] Root cause identified and documented
- [ ] Fix verified by second model
- [ ] No new warnings introduced
