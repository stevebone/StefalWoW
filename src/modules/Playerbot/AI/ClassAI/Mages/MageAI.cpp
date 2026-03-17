/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#include "MageAI.h"
#include "ArcaneMage.h"
#include "FireMage.h"
#include "FrostMage.h"
#include "../BaselineRotationManager.h"
#include "Player.h"
#include "Log.h"

namespace Playerbot
{

MageAI::MageAI(Player* bot) : ClassAI(bot)
{
    // SAFETY: GetName() removed from constructor to prevent ACCESS_VIOLATION crash during worker thread bot login

    // QW-4 FIX: Initialize per-instance specialization objects
    // These were previously static, causing all bots to share the same object with stale bot pointers
    _arcaneSpec = ::std::make_unique<ArcaneMageRefactored>(bot);
    _fireSpec = ::std::make_unique<FireMageRefactored>(bot);
    _frostSpec = ::std::make_unique<FrostMageRefactored>(bot);
}

MageAI::~MageAI() = default;

void MageAI::UpdateRotation(::Unit* target)
{
    if (!target || !_bot)
        return;

    // NOTE: Baseline rotation check is now handled at the dispatch level in
    // ClassAI::OnCombatUpdate(). This method is ONLY called when the bot has
    // already chosen a specialization (level 10+ with talents).

    // Delegate to specialization-specific AI
    // QW-4 FIX: Use per-instance specialization objects instead of static locals
    ChrSpecialization spec = _bot->GetPrimarySpecialization();

    switch (static_cast<uint32>(spec))
    {
        case 62: // Arcane
        {
            if (_arcaneSpec)
                _arcaneSpec->UpdateRotation(target);
            break;
        }
        case 63: // Fire
        {
            if (_fireSpec)
                _fireSpec->UpdateRotation(target);
            break;
        }
        case 64: // Frost
        {
            if (_frostSpec)
                _frostSpec->UpdateRotation(target);
            break;
        }
        default:
            // No spec or unknown spec, use basic rotation
            if (CanUseAbility(FROSTBOLT))
                CastSpell(FROSTBOLT, target);
            break;
    }
}

void MageAI::UpdateBuffs()
{
    if (!_bot)
        return;

    // Delegate to specialization-specific AI
    // QW-4 FIX: Use per-instance specialization objects instead of static locals
    ChrSpecialization spec = _bot->GetPrimarySpecialization();

    switch (static_cast<uint32>(spec))
    {
        case 62: // Arcane
        {
            if (_arcaneSpec)
                _arcaneSpec->UpdateBuffs();
            break;
        }
        case 63: // Fire
        {
            if (_fireSpec)
                _fireSpec->UpdateBuffs();
            break;
        }
        case 64: // Frost
        {
            if (_frostSpec)
                _frostSpec->UpdateBuffs();
            break;
        }
        default:
            break;
    }
}

void MageAI::UpdateCooldowns(uint32 diff)
{
    // Base cooldown management handled by ClassAI
    ClassAI::UpdateCooldowns(diff);
}
bool MageAI::CanUseAbility(uint32 spellId)
{
    if (!_bot)
        return false;
        if (!_bot->HasSpell(spellId))
        return false;

    // Delegate to base ClassAI implementation for spell readiness and resource checks
    return ClassAI::CanUseAbility(spellId);
}

void MageAI::OnCombatStart(::Unit* target)
{
    ClassAI::OnCombatStart(target);
}

void MageAI::OnCombatEnd()
{
    ClassAI::OnCombatEnd();
}

bool MageAI::HasEnoughResource(uint32 spellId)
{
    // Mages use Mana
    if (!_bot)
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return true;    // Calculate mana cost properly from PowerCosts vector
    uint32 manaCost = 0;
    for (SpellPowerCost const& cost : spellInfo->CalcPowerCost(_bot, spellInfo->GetSchoolMask()))
    {
        if (cost.Power == POWER_MANA)
        {
            manaCost = cost.Amount;
            break;
        }
    }
    return _bot->GetPower(POWER_MANA) >= manaCost;}

void MageAI::ConsumeResource(uint32 spellId)
{
    // Resource consumption handled by spell casting system
}

Position MageAI::GetOptimalPosition(::Unit* target)
{
    if (!target)
        return _bot->GetPosition();    // Mages are ranged DPS, maintain maximum range
    // Calculate optimal position at maximum spell range (30-40 yards for most mage spells)
    float optimalRange = 35.0f;

    if (!target || target == _bot)        return _bot->GetPosition();

    // Get direction from bot to target (absolute angle)
    float angle = _bot->GetAbsoluteAngle(target);
    float distance = _bot->GetExactDist2d(target);

    // If too close, move back to optimal range
    if (distance < optimalRange)
    {
        // Get position near target at optimal range
        Position pos = target->GetNearPosition(optimalRange, angle);
        return pos;
    }

    // Already at good range
    return _bot->GetPosition();
}

float MageAI::GetOptimalRange(::Unit* target)
{
    // Mages are ranged casters with 30-35 yard range
    return 30.0f;
}

} // namespace Playerbot
