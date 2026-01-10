#pragma once

#include <string>
#include <vector>

#include "Player.h"
#include "ScriptMgr.h"
#include "SpellMgr.h"
#include "Unit.h"

namespace FSBUtilsSpells
{
    /// Returns the English name of a spell by its ID, or "unknown spell" if not found.
    std::string GetSpellName(uint32 spellId);

    // Checks global cooldown
    bool CanCastNow(Unit* me, uint32 now, uint32 globalCooldownUntil);
}

uint32 GetManaPotionSpellForLevel(uint8 level);
