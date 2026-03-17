---
# WoW 11.x Packet Migration Workflow
# Legacy WorldPacket → Typed Packet Migration
---

## Configuration
- *Artifacts Path*: {@artifacts_path} → `.zenflow/tasks/{task_id}`
- *Primary Model*: Claude Opus 4
- *Verification Model*: GPT-4o
- *Parallel Execution*: Enabled (multiple files simultaneously)

---

## Agent Instructions

You are migrating legacy packet code to the new WoW 11.x typed packet system in TrinityCore Playerbot.

### Migration Pattern

**BEFORE (Legacy):**
```cpp
WorldPacket data(SMSG_SPELL_START, 50);
data << m_caster->GetPackGUID();
data << m_caster->GetPackGUID();
data << uint8(m_castCount);
data << uint32(m_spellInfo->Id);
data << uint32(castFlags);
data << uint32(m_casttime);
SendPacket(&data);
```

**AFTER (WoW 11.x):**
```cpp
WorldPackets::Spells::SpellStart packet;
packet.Cast.CasterGUID = m_caster->GetGUID();
packet.Cast.CasterUnit = m_caster->GetGUID();
packet.Cast.CastID = m_castCount;
packet.Cast.SpellID = m_spellInfo->Id;
packet.Cast.CastFlags = castFlags;
packet.Cast.CastTime = m_casttime;
GetSession()->SendPacket(packet.Write());
```

### Packet Mapping Reference
| Legacy Opcode | New Class |
|---------------|-----------|
| SMSG_SPELL_START | WorldPackets::Spells::SpellStart |
| SMSG_SPELL_GO | WorldPackets::Spells::SpellGo |
| SMSG_CAST_RESULT | WorldPackets::Spells::CastFailed |
| SMSG_CHANNEL_START | WorldPackets::Spells::ChannelStart |
| CMSG_CAST_SPELL | WorldPackets::Spells::CastSpell |
| SMSG_AI_REACTION | WorldPackets::Combat::AIReaction |
| SMSG_ATTACK_START | WorldPackets::Combat::AttackStart |
| SMSG_ATTACK_STOP | WorldPackets::Combat::AttackStop |

### Required Includes
```cpp
#include "SpellPackets.h"
#include "CombatPackets.h"
#include "MiscPackets.h"
#include "MovementPackets.h"
#include "ChatPackets.h"
```

---

## Workflow Steps

### [ ] Step 1: Discovery
Scan for legacy packet usage:
```powershell
findstr /s /n "WorldPacket" src\modules\Playerbot\*.cpp
```

Create inventory of files needing migration.
Save to `{@artifacts_path}/packet_inventory.md`

### [ ] Step 2: Prioritization
Prioritize files by:
1. **High**: Core gameplay (combat, spells)
2. **Medium**: Social features (chat, group)
3. **Low**: Utility functions

### [ ] Step 3: Migration (Per File)
For each file:

1. **Identify** all WorldPacket usages
2. **Map** each opcode to typed packet
3. **Migrate** following the pattern above
4. **Add** required includes
5. **Remove** legacy code

### [ ] Step 4: Build Verification
After each file migration:
```powershell
cmake --build build --config RelWithDebInfo -- /m
```

### [ ] Step 5: Cross-Model Review
**Agent Switch**: Use GPT-4o to review Claude's migration.

Check for:
1. Correct field mapping
2. Missing Optional<> fields
3. Proper GUID handling (GetGUID() not GetPackGUID())
4. Session validity checks

### [ ] Step 6: Batch Verification
After batch completion, run full build:
```powershell
cmake --build build --config Debug -- /m
cmake --build build --config RelWithDebInfo -- /m
cmake --build build --config Release -- /m
```

### [ ] Step 7: Documentation
Update migration tracking:
```markdown
## Migration Progress

| File | Status | Packets Migrated |
|------|--------|------------------|
| Combat.cpp | ✅ Done | 5 |
| Spell.cpp | 🔄 In Progress | 3/8 |
| Social.cpp | ⏳ Pending | 0 |
```

Save to `{@artifacts_path}/migration_progress.md`

---

## Parallel Execution Strategy

Run multiple migrations in parallel:
- Each file in isolated worktree
- No conflicts between migrations
- Merge after verification

**Example**: Migrate 5 files simultaneously:
1. Task 1: Combat packets
2. Task 2: Spell packets
3. Task 3: Movement packets
4. Task 4: Chat packets
5. Task 5: Group packets

---

## Completion Criteria
- [ ] All targeted files migrated
- [ ] All builds pass
- [ ] Cross-model review passed
- [ ] No legacy WorldPacket in migrated code
- [ ] Migration progress documented
