#pragma once

#include <vector>
#include "Define.h" // for uint32

struct FSBotSpells
{
    uint32 spellId;
    uint32 cooldownMs;
    float  chance;            // 0-100
    uint32 nextReadyMs = 0;   // runtime state
};

extern std::vector<FSBotSpells> AssistHealSpells;
extern std::vector<FSBotSpells> BalancedHealSpells;
extern std::vector<FSBotSpells> SelfHealSpells;
extern std::vector<FSBotSpells> CombatSpells;