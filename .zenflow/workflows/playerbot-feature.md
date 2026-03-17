---
# Playerbot Feature Workflow
# Spec-Driven Development für neue Bot-Features
---

## Configuration
- *Artifacts Path*: {@artifacts_path} → `.zenflow/tasks/{task_id}`
- *Primary Model*: Claude Opus 4
- *Verification Model*: GPT-4o

---

## Agent Instructions

You are implementing a new feature for the TrinityCore Playerbot module, a large C++ codebase (~636K LOC) that creates AI-controlled player bots for World of Warcraft.

### Critical Rules
1. **Thread Safety**: Use std::shared_mutex for shared data, follow lock hierarchy (Map > Grid > Object > Session)
2. **Memory Management**: Prefer smart pointers, no raw new/delete
3. **Code Style**: PascalCase classes, camelCase methods, m_ prefix for members
4. **API Compatibility**: Use WoW 11.x typed packets (WorldPackets::), not legacy WorldPacket
5. **Testing**: Ensure changes compile with all 3 configurations (Debug, RelWithDebInfo, Release)

### Project Context
- Location: `C:\TrinityBots\TrinityCore`
- Main module: `src/modules/Playerbot/`
- Build: CMake with MSVC (Visual Studio 2025)
- Dependencies: Boost, MySQL, OpenSSL, Intel TBB

---

## Workflow Steps

### [ ] Step 1: Research & Understanding
Analyze the codebase to understand:
1. Related existing implementations
2. Dependencies and interfaces
3. Threading model implications
4. Database schema if applicable

Save findings to `{@artifacts_path}/research.md`

### [ ] Step 2: Technical Specification
Create a detailed technical spec including:
1. **Objective**: Clear description of the feature
2. **Architecture**: How it fits into existing systems
3. **Data Structures**: New classes/structs needed
4. **Interfaces**: Public API design
5. **Threading**: Synchronization requirements
6. **Database**: Schema changes if any
7. **Configuration**: New playerbots.conf options
8. **Edge Cases**: Error handling and recovery

Save spec to `{@artifacts_path}/spec.md`

**STOP FOR REVIEW**: Wait for user approval of spec before proceeding.

### [ ] Step 3: Implementation
Implement the feature following the approved spec:
1. Create/modify header files (.h)
2. Implement source files (.cpp)
3. Update CMakeLists.txt if needed
4. Add configuration options to playerbots.conf.dist

Follow these patterns:
```cpp
// Safe bot access pattern
if (Player* bot = ObjectAccessor::FindPlayer(botGuid))
{
    if (bot->IsInWorld() && bot->GetPlayerbotAI())
    {
        // Safe to use bot
    }
}

// Thread-safe update pattern
{
    std::unique_lock lock(m_mutex);
    // Modify shared data
}
```

### [ ] Step 4: Build Verification
Run build for all configurations:
```powershell
cmake --build build --config Debug -- /m
cmake --build build --config RelWithDebInfo -- /m
cmake --build build --config Release -- /m
```

Fix any compilation errors or warnings.

### [ ] Step 5: Cross-Model Review
**Agent Switch**: Use different AI model for verification.

Review the implementation for:
1. Thread safety issues
2. Memory leaks
3. API compatibility
4. Code style compliance
5. Edge case handling
6. Performance concerns

Save review to `{@artifacts_path}/review.md`

### [ ] Step 6: Address Review Feedback
Fix any issues identified in the review.
Document changes made.

### [ ] Step 7: Final Documentation
Create/update documentation:
1. Code comments and docstrings
2. Update CLAUDE.md if architecture changed
3. Add entry to changelog

Save summary to `{@artifacts_path}/summary.md`

---

## Completion Criteria
- [ ] All builds pass (Debug, RelWithDebInfo, Release)
- [ ] No new warnings
- [ ] Cross-model review passed
- [ ] Documentation complete
- [ ] Code follows project conventions
