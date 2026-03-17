/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 */

#include "HealingTargetSelector.h"
#include "Spell.h"
#include "Player.h"
#include "Group.h"
#include "Unit.h"
#include "Creature.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "SpellAuraDefines.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "ThreatManager.h"
#include "Log.h"
#include "LFG.h"
#include "LFGMgr.h"
#include "../../Group/GroupMemberResolver.h"
#include "../../Core/Diagnostics/GroupMemberDiagnostics.h"
#include <algorithm>

// Talent spec constants for role detection (used internally)
// These map to TrinityCore's specialization IDs
namespace {
    enum TalentSpecs : uint32
    {
        // Warrior
        TALENT_SPEC_WARRIOR_ARMS        = 71,
        TALENT_SPEC_WARRIOR_FURY        = 72,
        TALENT_SPEC_WARRIOR_PROTECTION  = 73,

        // Paladin
        TALENT_SPEC_PALADIN_HOLY        = 65,
        TALENT_SPEC_PALADIN_PROTECTION  = 66,
        TALENT_SPEC_PALADIN_RETRIBUTION = 70,

        // Hunter
        TALENT_SPEC_HUNTER_BEASTMASTERY = 253,
        TALENT_SPEC_HUNTER_MARKSMANSHIP = 254,
        TALENT_SPEC_HUNTER_SURVIVAL     = 255,

        // Rogue
        TALENT_SPEC_ROGUE_ASSASSINATION = 259,
        TALENT_SPEC_ROGUE_OUTLAW        = 260,
        TALENT_SPEC_ROGUE_SUBTLETY      = 261,

        // Priest
        TALENT_SPEC_PRIEST_DISCIPLINE   = 256,
        TALENT_SPEC_PRIEST_HOLY         = 257,
        TALENT_SPEC_PRIEST_SHADOW       = 258,

        // Death Knight
        TALENT_SPEC_DEATHKNIGHT_BLOOD   = 250,
        TALENT_SPEC_DEATHKNIGHT_FROST   = 251,
        TALENT_SPEC_DEATHKNIGHT_UNHOLY  = 252,

        // Shaman
        TALENT_SPEC_SHAMAN_ELEMENTAL    = 262,
        TALENT_SPEC_SHAMAN_ENHANCEMENT  = 263,
        TALENT_SPEC_SHAMAN_RESTORATION  = 264,

        // Mage
        TALENT_SPEC_MAGE_ARCANE         = 62,
        TALENT_SPEC_MAGE_FIRE           = 63,
        TALENT_SPEC_MAGE_FROST          = 64,

        // Warlock
        TALENT_SPEC_WARLOCK_AFFLICTION  = 265,
        TALENT_SPEC_WARLOCK_DEMONOLOGY  = 266,
        TALENT_SPEC_WARLOCK_DESTRUCTION = 267,

        // Monk
        TALENT_SPEC_MONK_BREWMASTER     = 268,
        TALENT_SPEC_MONK_MISTWEAVER     = 270,
        TALENT_SPEC_MONK_WINDWALKER     = 269,

        // Druid
        TALENT_SPEC_DRUID_BALANCE       = 102,
        TALENT_SPEC_DRUID_FERAL         = 103,
        TALENT_SPEC_DRUID_BEAR          = 104,  // Guardian
        TALENT_SPEC_DRUID_RESTORATION   = 105,

        // Demon Hunter
        TALENT_SPEC_DEMONHUNTER_HAVOC     = 577,
        TALENT_SPEC_DEMONHUNTER_VENGEANCE = 581,

        // Evoker
        TALENT_SPEC_EVOKER_DEVASTATION   = 1467,
        TALENT_SPEC_EVOKER_PRESERVATION  = 1468,
        TALENT_SPEC_EVOKER_AUGMENTATION  = 1473
    };
}

namespace Playerbot {
namespace bot { namespace ai {

// TargetPriority implementation
float TargetPriority::CalculateScore() const
{
    if (!player || player->isDead())
        return 0.0f;

    // Base score: health deficit × role priority × distance
    float score = healthDeficit * rolePriority * distanceFactor;

    // Add debuff urgency (10 points per dispellable debuff)
    score += debuffCount * 10.0f;

    // Add threat factor bonus (high threat targets are more important to keep alive)
    score += threatFactor * 15.0f;

    // Reduce priority if already being healed (avoid overheal)
    if (hasIncomingHeals)
        score *= 0.7f;

    // Boost main tank priority
    if (isMainTank)
        score *= 1.2f;

    return score;
}

// HealingTargetSelector implementation
Player* HealingTargetSelector::SelectTarget(Player* healer, float range, float minHealthPercent)
{
    if (!healer)
        return nullptr;

    std::vector<TargetPriority> priorities = GetInjuredAllies(healer, range, minHealthPercent);

    if (priorities.empty())
        return nullptr;

    // Return highest priority target
    return priorities.front().player;
}

std::vector<TargetPriority> HealingTargetSelector::GetInjuredAllies(
    Player* healer,
    float range,
    float minHealthPercent)
{
    std::vector<TargetPriority> priorities;

    if (!healer)
        return priorities;

    Group* group = healer->GetGroup();
    if (!group)
    {
        // Solo: heal self if injured
        float healthPct = healer->GetHealthPct();
        if (healthPct < minHealthPercent)
        {
            TargetPriority priority;
            priority.player = healer;
            priority.healthDeficit = 100.0f - healthPct;
            priority.rolePriority = CalculateRolePriority(healer);
            priority.distanceFactor = 1.0f;
            priority.hasIncomingHeals = HasIncomingHeals(healer);
            priority.debuffCount = CountDispellableDebuffs(healer, DISPEL_ALL);
            priority.threatFactor = CalculateThreatFactor(healer);
            priority.isMainTank = false;

            priorities.push_back(priority);
        }
        return priorities;
    }

    // Group: evaluate all members
    std::vector<Player*> members = GetGroupMembersInRange(healer, range);

    for (Player* member : members)
    {
        if (!member || member->isDead())
            continue;

        float healthPct = member->GetHealthPct();
        if (healthPct >= minHealthPercent)
            continue;

        TargetPriority priority;
        priority.player = member;
        priority.healthDeficit = 100.0f - healthPct;
        priority.rolePriority = CalculateRolePriority(member);
        priority.distanceFactor = CalculateDistanceFactor(healer, member, range);
        priority.hasIncomingHeals = HasIncomingHeals(member);
        priority.debuffCount = CountDispellableDebuffs(member, DISPEL_ALL);
        priority.threatFactor = CalculateThreatFactor(member);
        priority.isMainTank = IsMainTank(member);

        priorities.push_back(priority);
    }

    // Sort by priority (highest first)
    std::sort(priorities.begin(), priorities.end(),
        [](const TargetPriority& a, const TargetPriority& b) {
            return a.CalculateScore() > b.CalculateScore();
        });

    return priorities;
}

bool HealingTargetSelector::NeedsDispel(Player* target, DispelType type)
{
    return CountDispellableDebuffs(target, type) > 0;
}

std::vector<Player*> HealingTargetSelector::GetTargetsNeedingDispel(
    Player* healer,
    DispelType type,
    float range)
{
    std::vector<Player*> targets;

    if (!healer)
        return targets;

    std::vector<Player*> members = GetGroupMembersInRange(healer, range);

    // Build list with priority
    struct DispelTarget
    {
        Player* player;
        uint32 debuffCount;
        float priority;
    };

    std::vector<DispelTarget> dispelTargets;

    for (Player* member : members)
    {
        if (!member || member->isDead())
            continue;

        uint32 debuffs = CountDispellableDebuffs(member, type);
        if (debuffs == 0)
            continue;

        DispelTarget dt;
        dt.player = member;
        dt.debuffCount = debuffs;

        // Priority: role × (1.0 - health) × debuffCount
        float rolePriority = CalculateRolePriority(member);
        float healthFactor = 1.0f - (member->GetHealthPct() / 100.0f);
        dt.priority = rolePriority * healthFactor * debuffs;

        dispelTargets.push_back(dt);
    }

    // Sort by priority
    std::sort(dispelTargets.begin(), dispelTargets.end(),
        [](const DispelTarget& a, const DispelTarget& b) {
            return a.priority > b.priority;
        });

    // Extract sorted players
    for (const auto& dt : dispelTargets)
        targets.push_back(dt.player);

    return targets;
}

Unit* HealingTargetSelector::SelectAoEHealingTarget(
    Player* healer,
    uint32 minTargets,
    float range)
{
    if (!healer)
        return nullptr;

    std::vector<Player*> members = GetGroupMembersInRange(healer, range * 1.5f);

    Unit* bestTarget = nullptr;
    float bestScore = 0.0f;

    // Evaluate each member as potential AoE center
    for (Player* center : members)
    {
        if (!center)
            continue;

        float score = CalculateAoEHealingScore(center, healer, range);

        if (score > bestScore)
        {
            bestScore = score;
            bestTarget = center;
        }
    }

    // Only return if we found a cluster with enough targets
    if (bestTarget)
    {
        uint32 targetCount = 0;
        for (Player* member : members)
        {
            if (!member || member->isDead())
                continue;

            if (member->GetDistance(bestTarget) <= range && member->GetHealthPct() < 95.0f)
                ++targetCount;
        }

        if (targetCount < minTargets)
            return nullptr;
    }

    return bestTarget;
}

bool HealingTargetSelector::IsHealingNeeded(Player* healer, float urgencyThreshold)
{
    if (!healer)
        return false;

    Group* group = healer->GetGroup();
    if (!group)
    {
        // Solo: check self
        return healer->GetHealthPct() < urgencyThreshold;
    }

    // Group: check any member
    std::vector<Player*> members = GetGroupMembersInRange(healer, 40.0f);

    for (Player* member : members)
    {
        if (member && !member->isDead() && member->GetHealthPct() < urgencyThreshold)
            return true;
    }

    return false;
}

float HealingTargetSelector::GetIncomingHealAmount(Player* target)
{
    if (!target)
        return 0.0f;

    float incomingHeals = 0.0f;

    // Track active HoTs (periodic healing auras)
    Unit::AuraApplicationMap const& auras = target->GetAppliedAuras();
    for (auto const& [spellId, aurApp] : auras)
    {
        if (!aurApp)
            continue;

        Aura* aura = aurApp->GetBase();
        if (!aura)
            continue;

        SpellInfo const* spellInfo = aura->GetSpellInfo();
        if (!spellInfo || !spellInfo->IsPositive())
            continue;

        // Check for periodic healing effects (HoTs like Renew, Rejuvenation, Riptide)
        std::vector<SpellEffectInfo> const& effects = spellInfo->GetEffects();
        for (size_t i = 0; i < effects.size(); ++i)
        {
            SpellEffectInfo const& effectInfo = effects[i];
            if (effectInfo.ApplyAuraName == SPELL_AURA_PERIODIC_HEAL ||
                effectInfo.ApplyAuraName == SPELL_AURA_OBS_MOD_HEALTH ||
                effectInfo.ApplyAuraName == SPELL_AURA_PERIODIC_HEALTH_FUNNEL)
            {
                // Estimate remaining healing from HoT
                int32 remainingDuration = aura->GetDuration();
                int32 amplitude = effectInfo.Amplitude;
                if (amplitude > 0 && remainingDuration > 0)
                {
                    int32 remainingTicks = remainingDuration / amplitude;
                    // Get heal amount per tick from the aura effect
                    if (AuraEffect* effect = aura->GetEffect(static_cast<SpellEffIndex>(i)))
                    {
                        incomingHeals += effect->GetAmount() * remainingTicks;
                    }
                }
            }
        }
    }

    // Track pending direct heals from other group members casting on target
    Group* group = target->GetGroup();
    if (group)
    {
        for (GroupReference const& itr : group->GetMembers())
        {
            Player* member = itr.GetSource();
            if (!member || member == target)
                continue;

            // Check if member is casting a healing spell on target
            if (Spell* spell = member->GetCurrentSpell(CURRENT_GENERIC_SPELL))
            {
                if (spell->m_targets.GetUnitTarget() == target)
                {
                    SpellInfo const* spellInfo = spell->GetSpellInfo();
                    if (spellInfo && spellInfo->IsPositive())
                    {
                        for (SpellEffectInfo const& effectInfo : spellInfo->GetEffects())
                        {
                            if (effectInfo.Effect == SPELL_EFFECT_HEAL ||
                                effectInfo.Effect == SPELL_EFFECT_HEAL_PCT)
                            {
                                // Estimate heal amount based on spell power
                                int32 basePoints = effectInfo.CalcValue(member, nullptr, target);
                                incomingHeals += basePoints;
                            }
                        }
                    }
                }
            }
        }
    }

    // Track absorb shields (Power Word: Shield, Earth Shield, etc.)
    for (auto const& [spellId, aurApp] : auras)
    {
        if (!aurApp)
            continue;

        Aura* aura = aurApp->GetBase();
        if (!aura)
            continue;

        SpellInfo const* spellInfo = aura->GetSpellInfo();
        if (!spellInfo || !spellInfo->IsPositive())
            continue;

        // Check for absorb effects
        std::vector<SpellEffectInfo> const& absorbEffects = spellInfo->GetEffects();
        for (size_t i = 0; i < absorbEffects.size(); ++i)
        {
            if (absorbEffects[i].ApplyAuraName == SPELL_AURA_SCHOOL_ABSORB)
            {
                if (AuraEffect* effect = aura->GetEffect(static_cast<SpellEffIndex>(i)))
                {
                    // Add remaining absorb amount
                    incomingHeals += effect->GetAmount();
                }
            }
        }
    }

    TC_LOG_TRACE("playerbot", "HealingTargetSelector: Target {} has {} incoming heals",
                target->GetName(), incomingHeals);

    return incomingHeals;
}

float HealingTargetSelector::PredictHealthInSeconds(Player* target, float seconds)
{
    if (!target)
        return 0.0f;

    float currentHealth = target->GetHealthPct();
    float incomingHeals = GetIncomingHealAmount(target);
    float incomingDamage = 0.0f;

    // Calculate incoming damage from DoTs (damage over time effects)
    Unit::AuraApplicationMap const& auras = target->GetAppliedAuras();
    for (auto const& [spellId, aurApp] : auras)
    {
        if (!aurApp)
            continue;

        Aura* aura = aurApp->GetBase();
        if (!aura)
            continue;

        SpellInfo const* spellInfo = aura->GetSpellInfo();
        if (!spellInfo)
            continue;

        // Check for negative periodic damage effects (DoTs)
        if (!spellInfo->IsPositive())
        {
            std::vector<SpellEffectInfo> const& dotEffects = spellInfo->GetEffects();
            for (size_t i = 0; i < dotEffects.size(); ++i)
            {
                SpellEffectInfo const& effectInfo = dotEffects[i];
                if (effectInfo.ApplyAuraName == SPELL_AURA_PERIODIC_DAMAGE ||
                    effectInfo.ApplyAuraName == SPELL_AURA_PERIODIC_DAMAGE_PERCENT ||
                    effectInfo.ApplyAuraName == SPELL_AURA_PERIODIC_LEECH)
                {
                    // Estimate damage over the prediction window
                    int32 remainingDuration = aura->GetDuration();
                    int32 amplitude = effectInfo.Amplitude;
                    if (amplitude > 0)
                    {
                        // Calculate how many ticks will occur in our prediction window
                        int32 predictWindowMs = static_cast<int32>(seconds * 1000.0f);
                        int32 timeToProcess = std::min(remainingDuration, predictWindowMs);
                        int32 ticksInWindow = timeToProcess / amplitude;

                        if (AuraEffect* effect = aura->GetEffect(static_cast<SpellEffIndex>(i)))
                        {
                            incomingDamage += effect->GetAmount() * ticksInWindow;
                        }
                    }
                }
            }
        }
    }

    // Check if target is being attacked (estimate damage from current attacker)
    if (Unit* attacker = target->GetVictim())
    {
        // Rough estimate: attacker's DPS × seconds
        // Use average damage from last few hits if available
        float estimatedDps = attacker->GetTotalAttackPowerValue(BASE_ATTACK) / 14.0f;  // Rough estimate
        incomingDamage += estimatedDps * seconds;
    }

    // Also check who is attacking the target
    for (auto const& itr : target->GetThreatManager().GetThreatenedByMeList())
    {
        if (Creature* attackerCreature = itr.second->GetOwner())
        {
            if (attackerCreature->GetVictim() == target)
            {
                // This unit is actively attacking our target
                float estimatedDps = attackerCreature->GetTotalAttackPowerValue(BASE_ATTACK) / 14.0f;
                incomingDamage += estimatedDps * seconds;
            }
        }
    }

    // Convert to percentages
    float maxHealth = static_cast<float>(target->GetMaxHealth());
    float incomingHealPercent = (incomingHeals / maxHealth) * 100.0f;
    float incomingDamagePercent = (incomingDamage / maxHealth) * 100.0f;

    float predictedHealth = currentHealth + incomingHealPercent - incomingDamagePercent;

    TC_LOG_TRACE("playerbot", "HealingTargetSelector: Predicted health for {} in {}s: {} (current={}, +heal={}, -dmg={})",
                target->GetName(), seconds, predictedHealth, currentHealth, incomingHealPercent, incomingDamagePercent);

    return std::clamp(predictedHealth, 0.0f, 100.0f);
}

// Private helper functions

float HealingTargetSelector::CalculateRolePriority(Player* target)
{
    if (!target)
        return 1.0f;

    // Try to use LFGRoleDetector if available (integrates with playerbot spec detection)
    // This provides accurate role detection based on player's current spec and talents

    // First check group flags for designated main tank/assist
    if (Group* group = target->GetGroup())
    {
        uint8 memberFlags = group->GetMemberFlags(target->GetGUID());
        if (memberFlags & MEMBER_FLAG_MAINTANK)
        {
            TC_LOG_TRACE("playerbot", "HealingTargetSelector: {} is designated main tank (group flag)", target->GetName());
            return 2.5f;  // Highest priority for designated main tank
        }
        if (memberFlags & MEMBER_FLAG_MAINASSIST)
        {
            TC_LOG_TRACE("playerbot", "HealingTargetSelector: {} is main assist (group flag)", target->GetName());
            return 1.8f;  // High priority for main assist
        }
    }

    // Detect role based on specialization (spec-based detection)
    uint8 playerClass = target->GetClass();
    uint32 spec = static_cast<uint32>(target->GetPrimarySpecialization());

    // Spec-based role detection
    switch (playerClass)
    {
        case CLASS_WARRIOR:
            if (spec == TALENT_SPEC_WARRIOR_PROTECTION)
                return 2.0f;  // Tank
            break;

        case CLASS_PALADIN:
            if (spec == TALENT_SPEC_PALADIN_PROTECTION)
                return 2.0f;  // Tank
            if (spec == TALENT_SPEC_PALADIN_HOLY)
                return 1.5f;  // Healer
            break;

        case CLASS_DEATH_KNIGHT:
            if (spec == TALENT_SPEC_DEATHKNIGHT_BLOOD)
                return 2.0f;  // Tank
            break;

        case CLASS_DRUID:
            if (spec == TALENT_SPEC_DRUID_BEAR)
                return 2.0f;  // Tank
            if (spec == TALENT_SPEC_DRUID_RESTORATION)
                return 1.5f;  // Healer
            break;

        case CLASS_MONK:
            if (spec == TALENT_SPEC_MONK_BREWMASTER)
                return 2.0f;  // Tank
            if (spec == TALENT_SPEC_MONK_MISTWEAVER)
                return 1.5f;  // Healer
            break;

        case CLASS_DEMON_HUNTER:
            if (spec == TALENT_SPEC_DEMONHUNTER_VENGEANCE)
                return 2.0f;  // Tank
            break;

        case CLASS_PRIEST:
            if (spec == TALENT_SPEC_PRIEST_DISCIPLINE || spec == TALENT_SPEC_PRIEST_HOLY)
                return 1.5f;  // Healer
            break;

        case CLASS_SHAMAN:
            if (spec == TALENT_SPEC_SHAMAN_RESTORATION)
                return 1.5f;  // Healer
            break;

        case CLASS_EVOKER:
            if (spec == TALENT_SPEC_EVOKER_PRESERVATION)
                return 1.5f;  // Healer
            break;
    }

    // Default: DPS priority
    return 1.0f;
}

float HealingTargetSelector::CalculateDistanceFactor(Player* healer, Player* target, float maxRange)
{
    if (!healer || !target)
        return 0.0f;

    float distance = healer->GetDistance(target);

    if (distance > maxRange)
        return 0.0f;

    // Linear falloff: 1.0 at 0 yards, 0.0 at maxRange
    return 1.0f - (distance / maxRange);
}

bool HealingTargetSelector::HasIncomingHeals(Player* target)
{
    if (!target)
        return false;

    // Check if target is being healed by other group members
    Group* group = target->GetGroup();
    if (!group)
        return false;

    // Iterate through group members to find healing spells being cast on target
    for (GroupReference const& itr : group->GetMembers())
    {
        Player* member = itr.GetSource();
        if (!member || member == target)
            continue;

        // Check if member is currently casting
    if (Spell* spell = member->GetCurrentSpell(CURRENT_GENERIC_SPELL))
        {
            // Check if spell targets our target
    if (spell->m_targets.GetUnitTarget() == target)
            {
                // Check if spell is a healing spell
                SpellInfo const* spellInfo = spell->GetSpellInfo();
                if (spellInfo && spellInfo->IsPositive() && spellInfo->HasEffect(SPELL_EFFECT_HEAL))
                {
                    TC_LOG_TRACE("playerbot", "HealingTargetSelector: Target {} has incoming heal from {}",
                                target->GetName(), member->GetName());
                    return true;
                }
            }
        }

        // Also check channeled spells
    if (Spell* channelSpell = member->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
        {
            if (channelSpell->m_targets.GetUnitTarget() == target)
            {
                SpellInfo const* spellInfo = channelSpell->GetSpellInfo();
                if (spellInfo && spellInfo->IsPositive() && spellInfo->HasEffect(SPELL_EFFECT_HEAL))
                {
                    TC_LOG_TRACE("playerbot", "HealingTargetSelector: Target {} has incoming channeled heal from {}",
                                target->GetName(), member->GetName());
                    return true;
                }
            }
        }
    }

    return false;
}

uint32 HealingTargetSelector::CountDispellableDebuffs(Player* target, DispelType type)
{
    if (!target)
        return 0;

    uint32 count = 0;

    // Iterate through auras
    Unit::AuraApplicationMap const& auras = target->GetAppliedAuras();
    for (auto const& [spellId, aurApp] : auras)
    {
        if (!aurApp)
            continue;

        Aura* aura = aurApp->GetBase();
        if (!aura)
            continue;

        SpellInfo const* spellInfo = aura->GetSpellInfo();
        if (!spellInfo)
            continue;

        // Check if negative aura (debuff)
    if (!spellInfo->IsPositive())
        {
            // Check dispel type
    if (type == DISPEL_ALL || spellInfo->Dispel == type)
                ++count;
        }
    }

    return count;
}

float HealingTargetSelector::CalculateThreatFactor(Player* target)
{
    if (!target)
        return 0.0f;

    // Calculate threat factor based on target's threat across all engaged enemies
    // High threat targets are more important to keep alive (especially tanks holding aggro)

    float maxThreatPct = 0.0f;
    float totalThreat = 0.0f;
    uint32 engagedEnemies = 0;

    // Check threat on all hostile units engaged with the target
    for (auto const& itr : target->GetThreatManager().GetThreatenedByMeList())
    {
        Creature* enemyCreature = itr.second->GetOwner();
        if (!enemyCreature || !enemyCreature->IsAlive())
            continue;

        ++engagedEnemies;

        // Get target's threat on this enemy
        float myThreat = itr.second->GetThreat();
        totalThreat += myThreat;

        // Get the threat of the enemy's current target for comparison
        if (Unit* enemyTarget = enemyCreature->GetVictim())
        {
            float topThreat = enemyCreature->GetThreatManager().GetThreat(enemyTarget);
            if (topThreat > 0.0f)
            {
                float threatPct = myThreat / topThreat;
                if (threatPct > maxThreatPct)
                    maxThreatPct = threatPct;
            }
        }
    }

    // Normalize to 0.0-1.0 range
    float threatFactor = 0.0f;

    if (engagedEnemies > 0)
    {
        // Factor in both relative threat (maxThreatPct) and engagement level
        // Being main threat target (maxThreatPct >= 1.0) = high priority
        // Engaged with many enemies = higher priority

        // Main threat factor: 0 = no threat, 1 = top threat target
        threatFactor = std::min(maxThreatPct, 1.0f);

        // Boost factor if engaged with multiple enemies (tanks pulling multiple mobs)
        if (engagedEnemies > 1)
        {
            float engagementBonus = std::min(engagedEnemies * 0.1f, 0.3f);  // Max 30% bonus
            threatFactor = std::min(threatFactor + engagementBonus, 1.0f);
        }
    }

    TC_LOG_TRACE("playerbot", "HealingTargetSelector: {} threat factor={} (engaged={}, maxPct={})",
                target->GetName(), threatFactor, engagedEnemies, maxThreatPct);

    return threatFactor;
}

bool HealingTargetSelector::IsMainTank(Player* target)
{
    if (!target)
        return false;

    // Check group membership
    Group* group = target->GetGroup();
    if (!group)
        return false;  // Solo players aren't "main tank"

    // 1. Check explicit main tank flag (highest priority - manually assigned by raid leader)
    uint8 memberFlags = group->GetMemberFlags(target->GetGUID());
    if (memberFlags & MEMBER_FLAG_MAINTANK)
    {
        TC_LOG_TRACE("playerbot", "HealingTargetSelector: {} is main tank (group flag)", target->GetName());
        return true;
    }

    // 2. Check if player has tank role AND highest threat among group tanks
    bool hasTankRole = false;

    // Check spec-based tank role
    uint32 spec = static_cast<uint32>(target->GetPrimarySpecialization());
    switch (spec)
    {
        case TALENT_SPEC_WARRIOR_PROTECTION:
        case TALENT_SPEC_PALADIN_PROTECTION:
        case TALENT_SPEC_DRUID_BEAR:
        case TALENT_SPEC_DEATHKNIGHT_BLOOD:
        case TALENT_SPEC_MONK_BREWMASTER:
        case TALENT_SPEC_DEMONHUNTER_VENGEANCE:
            hasTankRole = true;
            break;
        default:
            break;
    }

    if (!hasTankRole)
        return false;

    // 3. For tank-spec players, check if they have the highest threat engagement
    // This identifies the "active" main tank in multi-tank setups
    float myThreatScore = CalculateThreatFactor(target);

    // Check against other tanks in the group
    for (GroupReference& ref : group->GetMembers())
    {
        Player* member = ref.GetSource();
        if (!member || member == target || member->isDead())
            continue;

        // Check if this member is also a tank
        bool memberIsTank = false;
        uint8 otherMemberFlags = group->GetMemberFlags(member->GetGUID());
        if (otherMemberFlags & MEMBER_FLAG_MAINTANK)
            continue;  // Skip - explicit main tank handled above

        // Check spec-based tank role for this member
        uint32 otherSpec = static_cast<uint32>(member->GetPrimarySpecialization());
        switch (otherSpec)
        {
            case TALENT_SPEC_WARRIOR_PROTECTION:
            case TALENT_SPEC_PALADIN_PROTECTION:
            case TALENT_SPEC_DRUID_BEAR:
            case TALENT_SPEC_DEATHKNIGHT_BLOOD:
            case TALENT_SPEC_MONK_BREWMASTER:
            case TALENT_SPEC_DEMONHUNTER_VENGEANCE:
                memberIsTank = true;
                break;
            default:
                break;
        }

        // If another tank has higher threat, we're not main tank
        if (memberIsTank)
        {
            float otherThreatScore = CalculateThreatFactor(member);
            if (otherThreatScore > myThreatScore + 0.1f)  // Small threshold for stability
            {
                TC_LOG_TRACE("playerbot", "HealingTargetSelector: {} not main tank - {} has higher threat",
                            target->GetName(), member->GetName());
                return false;
            }
        }
    }

    // Tank role with highest (or tied highest) threat = main tank
    TC_LOG_TRACE("playerbot", "HealingTargetSelector: {} is main tank (highest threat tank)", target->GetName());
    return true;
}

std::vector<Player*> HealingTargetSelector::GetGroupMembersInRange(Player* healer, float range)
{
    std::vector<Player*> members;

    if (!healer)
        return members;

    Group* group = healer->GetGroup();
    if (!group)
    {
        // Solo: only self
        members.push_back(healer);
        return members;
    }

    // FIXED: Use GroupMemberResolver instead of direct iteration
    // This ensures bots are properly found via BotWorldSessionMgr fallback
    float rangeSq = range * range;
    
    for (auto const& slot : group->GetMemberSlots())
    {
        // Use diagnostic lookup if enabled, otherwise regular resolver
        Player* member = sGroupMemberDiagnostics->IsEnabled()
            ? sGroupMemberDiagnostics->DiagnosticLookup(slot.guid, __FUNCTION__, __FILE__, __LINE__)
            : GroupMemberResolver::ResolveMember(slot.guid);
            
        if (!member || member->isDead())
            continue;

        // Check range (must be on same map)
        if (member->GetMapId() != healer->GetMapId())
            continue;

        if (healer->GetExactDistSq(member) <= rangeSq)
            members.push_back(member);
    }

    return members;
}

float HealingTargetSelector::CalculateAoEHealingScore(Unit* position, Player* healer, float range)
{
    if (!position || !healer)
        return 0.0f;

    std::vector<Player*> members = GetGroupMembersInRange(healer, range * 1.5f);

    uint32 injuredCount = 0;
    float totalDeficit = 0.0f;

    for (Player* member : members)
    {
        if (!member || member->isDead())
            continue;

        float distance = member->GetDistance(position);
        if (distance > range)
            continue;

        float healthPct = member->GetHealthPct();
        if (healthPct >= 95.0f)
            continue;

        ++injuredCount;
        totalDeficit += (100.0f - healthPct);
    }

    // Score = number of injured × average deficit
    if (injuredCount == 0)
        return 0.0f;

    float avgDeficit = totalDeficit / injuredCount;
    return injuredCount * avgDeficit;
}

}} // namespace bot::ai
} // namespace Playerbot
