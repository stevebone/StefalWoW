# AI Code Review Slash Command Guide

## 🚀 Quick Start

Use the `/review-code` slash command for automated AI-powered code reviews with full two-step orchestration.

### Basic Usage

```
/review-code src/modules/Playerbot/Core/PlayerBotHooks.cpp
```

This will:
1. ✅ Automatically call Serena MCP to parse C++ code
2. ✅ Apply all 870 code review rules
3. ✅ Use LM Studio (qwen3-coder-30b) for AI enhancement
4. ✅ Generate comprehensive review report

---

## Examples

### Example 1: Review Single File
```
/review-code src/modules/Playerbot/Core/PlayerBotHooks.cpp
```

**Output**: Complete code review with critical, major, and minor issues

---

### Example 2: Review with Focus
```
/review-code src/server/game/Entities/Player/Player.cpp

Focus on:
- Null safety issues
- Memory leaks
- Thread safety
```

**Output**: Targeted review on specified categories

---

### Example 3: Security Audit
```
/review-code src/modules/Playerbot/Database/BotDatabase.cpp

Security audit focusing on:
- SQL injection
- Buffer overflows
- Input validation
```

**Output**: Security-focused review with detailed explanations

---

## How It Works

### Behind the Scenes

```mermaid
graph LR
    A[You: /review-code file.cpp] --> B[Step 1: Serena MCP]
    B --> C[Parse C++ → AST]
    C --> D[Step 2: Apply 870 Rules]
    D --> E[Step 3: LM Studio AI]
    E --> F[Comprehensive Report]
```

### Step-by-Step Process

**Step 1: Serena MCP Analysis**
- Calls `mcp__serena__get_symbols_overview`
- Gets all functions, classes, methods
- Builds symbol table

**Step 2: Rule Application**
- Null Safety: 221 rules
- Memory: 155 rules
- Concurrency: 101 rules
- Conventions: 40 rules
- Security: 150 rules
- Performance: 100 rules
- Architecture: 50 rules

**Step 3: AI Enhancement**
- Critical/Major issues → LM Studio analysis
- Detailed explanations
- Fix suggestions
- Best practices

**Step 4: Report Generation**
- Markdown format
- Code snippets
- Line numbers
- Confidence scores

---

## Report Format

### Sample Output

```markdown
# Code Review: PlayerBotHooks.cpp

## Summary
- Files Analyzed: 1
- Total Issues: 5
- Critical: 2
- Major: 3
- Minor: 0

## Critical Issues

### 1. [NULL-015] Missing null check before AI pointer usage
**Location**: Line 460
**Severity**: CRITICAL

**Code**:
```cpp
if (BotAI* ai = botSession->GetAI())
{
    ai->OnDeath();  // ⚠️ GetAI() could return nullptr
}
```

**Issue**: GetAI() can return nullptr if AI isn't initialized

**AI Analysis** (qwen3-coder-30b):
A BotSession can exist without an initialized AI during startup
or after errors. Calling OnDeath() on nullptr will crash the server.

**Recommended Fix**:
```cpp
if (BotAI* ai = botSession->GetAI())
{
    ai->OnDeath();
}
else
{
    TC_LOG_WARN("module.playerbot", "Bot died without AI");
}
```

**Confidence**: 95%

---

[Additional issues...]
```

---

## Advanced Usage

### Review Multiple Files

For multiple files, run the command sequentially:

```
/review-code src/modules/Playerbot/Core/PlayerBotHooks.cpp
```

Then:

```
/review-code src/modules/Playerbot/Core/PlayerBotManager.cpp
```

### Batch Review

For batch reviews, use the MCP tool directly (after reconnecting):

```
Use review-code-pattern to analyze src/modules/Playerbot/**/*.cpp
```

---

## Configuration

### LM Studio Settings

The slash command uses these LM Studio settings:
- **Model**: qwen/qwen3-coder-30b
- **Endpoint**: http://localhost:1234/v1/chat/completions
- **Temperature**: 0.3 (consistent analysis)
- **Max Tokens**: 600

### Serena MCP Settings

Uses your configured Serena MCP instance from Claude Code settings.

---

## Troubleshooting

### Issue: "Serena MCP not available"
**Solution**: Check that Serena is configured in Claude Code MCP settings

### Issue: "LM Studio timeout"
**Solution**:
1. Verify LM Studio is running: `curl http://localhost:1234/v1/models`
2. Check that qwen3-coder-30b is loaded
3. Increase timeout if model is slow

### Issue: "No issues found"
**Possible causes**:
1. Code is genuinely clean (rare!)
2. Serena didn't parse the file correctly
3. Rule filters too restrictive

**Debug**: Run `/review-code` again with verbose output requested

---

## Best Practices

### When to Use

✅ **Use for**:
- Pre-commit reviews
- Code review assistance
- Security audits
- Learning best practices
- Finding subtle bugs

❌ **Don't use for**:
- Real-time editing (too slow)
- Simple syntax checks (use IDE)
- Non-C++ files

### Performance Tips

1. **Focus reviews**: Specify categories to speed up analysis
2. **Batch wisely**: Review related files together
3. **Critical first**: Fix critical issues before reviewing again
4. **Cache results**: Save reports for later reference

---

## Integration with Workflow

### Pre-Commit Hook

```bash
#!/bin/bash
# .git/hooks/pre-commit

# Get changed C++ files
CHANGED_FILES=$(git diff --cached --name-only --diff-filter=ACMR | grep -E '\.(cpp|h)$')

if [ -n "$CHANGED_FILES" ]; then
    echo "Running code review on changed files..."
    # Use Claude Code CLI (when available) or manual review
    for file in $CHANGED_FILES; do
        echo "/review-code $file"
    done
fi
```

### CI/CD Integration

```yaml
# .github/workflows/code-review.yml
name: AI Code Review

on: [pull_request]

jobs:
  review:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Run AI Code Review
        run: |
          # Call review API or generate reports
          # (Full automation requires API integration)
```

---

## FAQ

**Q: How long does a review take?**
A: 10-30 seconds for typical files (depending on size and AI enhancement)

**Q: Can I customize the rules?**
A: Currently no, but you can filter by category and severity

**Q: What if I disagree with a finding?**
A: Check the confidence score. Lower scores mean higher uncertainty.

**Q: Can I review header files?**
A: Yes! The command works with both .cpp and .h files

**Q: Does it support C++20 features?**
A: Yes, Serena MCP and the rules are C++20 aware

---

## Next Steps

1. **Try it now**: `/review-code src/modules/Playerbot/Core/PlayerBotHooks.cpp`
2. **Review the output**: Understand the findings
3. **Fix issues**: Start with critical items
4. **Re-review**: Verify fixes with another run

---

**Ready to use!** Just type `/review-code <file-path>` and let the AI handle the rest.
