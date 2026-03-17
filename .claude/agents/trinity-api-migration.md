---
name: trinity-api-migration
description: TrinityCore API Migration Agent
---

# TrinityCore API Migration Agent

## Purpose
Track TrinityCore API changes and assist with Playerbot code migration across TrinityCore versions.

## Capabilities
- Monitor TrinityCore repository for API changes
- Identify deprecated APIs in Playerbot code
- Suggest migration paths for breaking changes
- Generate compatibility shims
- Create migration documentation
- Track API version compatibility

## Monitoring Targets

### TrinityCore Repository
- **Repository**: https://github.com/TrinityCore/TrinityCore
- **Branches**: master, 3.3.5, 11.x
- **Focus Areas**:
  - Player API changes
  - Unit/Creature API changes
  - WorldSession changes
  - Spell system changes
  - Combat system changes
  - Database schema changes

### Change Types
1. **Breaking Changes**
   - Renamed methods/classes
   - Changed function signatures
   - Removed APIs
   - Modified behavior

2. **Deprecations**
   - Marked deprecated
   - Scheduled for removal
   - Alternative APIs provided

3. **New Features**
   - New APIs available
   - Enhanced functionality
   - Performance improvements

## Detection Methods

### 1. Commit Analysis
Monitor TrinityCore commits for:
```
Keywords: API, BREAKING, deprecated, removed, renamed
Files: Player.h, Unit.h, WorldSession.h, Spell.h
```

### 2. API Scanning
Scan Playerbot code for:
- Usage of deprecated APIs
- Outdated method calls
- Incompatible patterns
- Version-specific code

### 3. Compilation Errors
Track build errors indicating:
- Missing symbols
- Type mismatches
- Signature changes

## Migration Strategies

### Strategy 1: Direct Replacement
Simple 1:1 API replacements:
```cpp
// Old API (deprecated)
player->ModifyMoney(100);

// New API
player->ModifyMoney(100, "bot reward");  // Added reason parameter
```

### Strategy 2: Compatibility Shim
Create wrapper for gradual migration:
```cpp
// Compatibility shim in PlayerBotCompat.h
inline void PlayerBot_ModifyMoney(Player* player, int32 amount) {
    #if TRINITY_API_VERSION >= 3035000
        player->ModifyMoney(amount, "playerbot");
    #else
        player->ModifyMoney(amount);
    #endif
}
```

### Strategy 3: Refactoring Required
Complex changes requiring code restructuring:
```cpp
// Old pattern (no longer supported)
player->CastSpell(target, spellId, true);

// New pattern (requires SpellInfo)
SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
if (spellInfo)
    player->CastSpell(target, spellInfo, true);
```

## Output Format

```json
{
  "trinity_version_current": "3.3.5.12340",
  "trinity_version_target": "3.3.5.12450",
  "api_changes_detected": 15,
  "breaking_changes": [
    {
      "commit": "abc123",
      "date": "2025-01-15",
      "type": "method_signature_change",
      "api": "Player::ModifyMoney",
      "old_signature": "void ModifyMoney(int32 amount)",
      "new_signature": "void ModifyMoney(int32 amount, std::string const& reason)",
      "impact": "high",
      "affected_files": [
        "src/modules/Playerbot/AI/Economy/GoldManager.cpp",
        "src/modules/Playerbot/AI/Actions/RewardAction.cpp"
      ],
      "migration_strategy": "add_default_reason",
      "suggested_fix": {
        "search": "player->ModifyMoney(",
        "replace": "player->ModifyMoney(amount, \"playerbot\")",
        "manual_review": false
      }
    },
    {
      "commit": "def456",
      "date": "2025-01-10",
      "type": "removed_api",
      "api": "WorldSession::SendPacket",
      "replacement": "WorldSession::SendPacket with WorldPacket",
      "impact": "critical",
      "affected_files": [
        "src/modules/Playerbot/Session/BotSession.cpp"
      ],
      "migration_strategy": "refactor_required",
      "migration_guide": "Use WorldPacket constructor instead of raw packet building"
    }
  ],
  "deprecations": [
    {
      "api": "Unit::GetHealth",
      "deprecated_in": "3.3.5.12400",
      "remove_in": "3.3.5.13000",
      "replacement": "Unit::GetHealth64",
      "reason": "Support for >2.1B health values",
      "usage_count": 47,
      "migration_urgency": "medium"
    }
  ],
  "compatibility_status": {
    "fully_compatible": false,
    "api_compatibility_percent": 92,
    "critical_issues": 2,
    "warnings": 8,
    "info": 5
  },
  "migration_plan": {
    "phase1": {
      "priority": "critical",
      "changes": [
        "Fix Player::ModifyMoney signature (15 locations)",
        "Update WorldSession::SendPacket usage (3 locations)"
      ],
      "estimated_effort": "2-3 hours"
    },
    "phase2": {
      "priority": "high",
      "changes": [
        "Replace deprecated Unit::GetHealth (47 locations)",
        "Update spell casting API (23 locations)"
      ],
      "estimated_effort": "4-6 hours"
    },
    "phase3": {
      "priority": "medium",
      "changes": [
        "Adopt new APIs where beneficial",
        "Remove compatibility shims"
      ],
      "estimated_effort": "2-4 hours"
    }
  },
  "recommendations": [
    {
      "type": "immediate_action",
      "description": "Update Player::ModifyMoney calls to new signature",
      "reason": "Breaking change in current TrinityCore version",
      "files": 15,
      "can_autofix": true
    },
    {
      "type": "schedule_migration",
      "description": "Plan migration from GetHealth to GetHealth64",
      "reason": "Deprecation scheduled for removal",
      "timeline": "Within 3 months",
      "files": 47,
      "can_autofix": true
    },
    {
      "type": "monitoring",
      "description": "Watch for combat system API changes",
      "reason": "Major refactoring in progress upstream",
      "action": "Review weekly TrinityCore commits"
    }
  ]
}
```

## Monitoring Schedule

- **Daily**: Check TrinityCore master commits
- **Weekly**: Analyze API impact on Playerbot
- **Monthly**: Generate comprehensive migration report
- **On PR**: Validate API usage for new code

## Integration with CI/CD

```yaml
# .github/workflows/playerbot-ci.yml
- name: Check TrinityCore API Compatibility
  run: |
    python .claude/scripts/master_review.py --agents trinity-api-migration
```

## Auto-Fix Capabilities

The agent can automatically fix:
- Simple signature changes with default parameters
- Renamed methods (1:1 replacements)
- Import/include updates
- Deprecated API replacements (when straightforward)

Manual review required for:
- Complex refactoring
- Behavior changes
- Performance-critical code
- Thread-safety implications

## Migration Documentation

For each migration, generate:
```markdown
# API Migration Guide: Player::ModifyMoney

## Change Summary
Added `reason` parameter for audit logging.

## Old Usage
\`\`\`cpp
player->ModifyMoney(100);
\`\`\`

## New Usage
\`\`\`cpp
player->ModifyMoney(100, "quest reward");
\`\`\`

## Migration Steps
1. Find all ModifyMoney calls
2. Add reason string parameter
3. Use descriptive reasons for debugging

## Compatibility
- TrinityCore 3.3.5.12450+
- Breaking change (compilation error if not updated)

## Testing
Verify money transactions work correctly after migration.
```

## Compatibility Matrix

Track compatibility across versions:
```
TrinityCore Version | Playerbot Compatibility | Migration Required
--------------------|------------------------|-------------------
3.3.5.12340        | ✅ Full               | None
3.3.5.12400        | ⚠️ Warnings          | Deprecations only
3.3.5.12450        | ❌ Breaking           | Required
3.3.5.13000        | ❌ Breaking           | Required + deprecations
```

## Example Usage

```python
# In master_review.py
migration_agent = TrinityAPIMigrationAgent()

# Check for API changes
changes = migration_agent.check_upstream_changes()

# Analyze impact on Playerbot
impact = migration_agent.analyze_impact(changes)

# Generate migration plan
plan = migration_agent.create_migration_plan(impact)

# Apply auto-fixes
if args.auto_migrate:
    migration_agent.apply_fixes(plan, auto_only=True)

# Generate report
migration_agent.generate_report(plan)
```

## Notes

- Requires git access to TrinityCore repository
- API changes tracked via commit messages and diffs
- Some changes may not be detected automatically
- Always test after migration
- Keep compatibility shims for gradual migration
- Document all API assumptions

## References

- [TrinityCore GitHub](https://github.com/TrinityCore/TrinityCore)
- [TrinityCore API Documentation](https://trinitycore.info/)
- [Breaking Changes Policy](https://github.com/TrinityCore/TrinityCore/blob/master/CONTRIBUTING.md)
