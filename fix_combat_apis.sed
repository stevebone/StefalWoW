# Fix missing includes - add after SpellHistory.h
/^#include "SpellHistory\.h"$/ a\
#include "SpellAuras.h"\
#include "SpellAuraEffects.h"

# Fix Group iteration pattern - replace GetFirstMember with range-based for
s/for (GroupReference\* ref = group->GetFirstMember(); ref != nullptr; ref = ref->next())/for (GroupReference const\& groupRef : group->GetMembers())/g

# Fix ref->GetSource() to groupRef.GetSource()
s/Player\* member = ref->GetSource();/Player* member = groupRef.GetSource();/g

# Fix Creature::IsWorldBoss() to IsDungeonBoss()
s/creature->IsWorldBoss()/creature->IsDungeonBoss()/g
