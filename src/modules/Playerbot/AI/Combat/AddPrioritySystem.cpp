/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "AddPrioritySystem.h"
#include "Player.h"
#include "Creature.h"
#include "Unit.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "Map.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "GameTime.h"
#include "Log.h"
#include "../../Spatial/SpatialGridQueryHelpers.h"
#include "../../Spatial/SpatialGridManager.h"

namespace Playerbot
{

// ============================================================================
// CONSTRUCTOR
// ============================================================================

AddPrioritySystem::AddPrioritySystem(Player* bot)
    : _bot(bot)
{
    if (!_bot)
    {
        TC_LOG_ERROR("module.playerbot", "AddPrioritySystem: Created with null bot!");
        return;
    }

    _roleContext = DetectRoleContext();
    UpdateEncounterContext();

    TC_LOG_DEBUG("module.playerbot", "AddPrioritySystem: Initialized for bot {} (role={})",
        _bot->GetName(), static_cast<uint8>(_roleContext));
}

// ============================================================================
// CORE UPDATE
// ============================================================================

void AddPrioritySystem::Update(uint32 diff)
{
    if (!_bot || !_bot->IsInWorld() || !_bot->IsAlive() || !_bot->IsInCombat())
        return;

    _updateTimer += diff;
    if (_updateTimer < UPDATE_INTERVAL_MS)
        return;
    _updateTimer = 0;

    // Periodically refresh encounter context
    _contextRefreshTimer += diff;
    if (_contextRefreshTimer >= CONTEXT_REFRESH_INTERVAL_MS)
    {
        _contextRefreshTimer = 0;
        UpdateEncounterContext();
        _roleContext = DetectRoleContext();
    }

    ScanAndClassifyAdds();
    UpdateSituation();
}

void AddPrioritySystem::Reset()
{
    _classifiedAdds.clear();
    _situation.Reset();
    _updateTimer = 0;
    _contextRefreshTimer = 0;
}

// ============================================================================
// CLASSIFICATION QUERIES
// ============================================================================

AddClassification const* AddPrioritySystem::GetClassification(ObjectGuid guid) const
{
    auto it = _classifiedAdds.find(guid);
    return (it != _classifiedAdds.end()) ? &it->second : nullptr;
}

float AddPrioritySystem::GetAddPriorityScore(ObjectGuid guid) const
{
    auto it = _classifiedAdds.find(guid);
    if (it == _classifiedAdds.end())
        return 0.0f;

    return it->second.priorityScore;
}

ObjectGuid AddPrioritySystem::GetHighestPriorityAdd() const
{
    return _situation.highestPriorityGuid;
}

std::vector<AddClassification> AddPrioritySystem::GetAddsByPriority() const
{
    std::vector<AddClassification> sorted;
    sorted.reserve(_classifiedAdds.size());

    for (auto const& pair : _classifiedAdds)
    {
        if (!pair.second.isCrowdControlled)
            sorted.push_back(pair.second);
    }

    std::sort(sorted.begin(), sorted.end(), std::greater<AddClassification>());
    return sorted;
}

bool AddPrioritySystem::ShouldSwitchToAdd(ObjectGuid currentTarget) const
{
    if (!_situation.needsImmediateSwitch)
        return false;

    if (currentTarget == _situation.highestPriorityGuid)
        return false; // Already on the best target

    // Check if the best add has a significantly higher priority
    auto currentIt = _classifiedAdds.find(currentTarget);
    float currentScore = (currentIt != _classifiedAdds.end()) ? currentIt->second.priorityScore : 0.0f;

    return (_situation.highestPriorityScore - currentScore) >= _switchThreshold;
}

// ============================================================================
// SCAN AND CLASSIFY
// ============================================================================

void AddPrioritySystem::ScanAndClassifyAdds()
{
    uint32 now = GameTime::GetGameTimeMS();

    // Use spatial grid to find all hostile creatures in range
    auto hostileSnapshots = SpatialGridQueryHelpers::FindHostileCreaturesInRange(_bot, _scanRange, true);

    // Track which GUIDs are still present
    std::unordered_set<ObjectGuid> presentGuids;
    presentGuids.reserve(hostileSnapshots.size());

    for (auto const& snapshot : hostileSnapshots)
    {
        presentGuids.insert(snapshot.guid);

        // Check if we already have a recent classification
        auto existingIt = _classifiedAdds.find(snapshot.guid);
        if (existingIt != _classifiedAdds.end())
        {
            // Update distance and health from snapshot
            existingIt->second.distance = _bot->GetExactDist2d(snapshot.position);
            if (snapshot.maxHealth > 0)
                existingIt->second.healthPercent = (static_cast<float>(snapshot.health) / static_cast<float>(snapshot.maxHealth)) * 100.0f;

            // Reclassify if stale
            if ((now - existingIt->second.lastClassifiedMs) < CLASSIFICATION_STALENESS_MS)
            {
                // Just update priority score with new health/distance
                existingIt->second.priorityScore = CalculatePriorityScore(existingIt->second);
                existingIt->second.urgency = DetermineUrgency(existingIt->second);
                continue;
            }
        }

        // Full classification needed - resolve to Creature*
        Unit* unit = ObjectAccessor::GetUnit(*_bot, snapshot.guid);
        if (!unit || !unit->IsAlive())
            continue;

        Creature* creature = unit->ToCreature();
        if (!creature)
            continue;

        AddClassification classification = ClassifyCreature(creature);
        classification.distance = _bot->GetExactDist2d(creature);
        classification.lastClassifiedMs = now;

        // Calculate priority score
        classification.priorityScore = CalculatePriorityScore(classification);
        classification.urgency = DetermineUrgency(classification);

        _classifiedAdds[snapshot.guid] = std::move(classification);
    }

    // Remove adds that are no longer present
    for (auto it = _classifiedAdds.begin(); it != _classifiedAdds.end(); )
    {
        if (presentGuids.find(it->first) == presentGuids.end())
            it = _classifiedAdds.erase(it);
        else
            ++it;
    }
}

AddClassification AddPrioritySystem::ClassifyCreature(Creature* creature) const
{
    AddClassification result;

    if (!creature)
        return result;

    CreatureTemplate const* tmpl = creature->GetCreatureTemplate();
    if (!tmpl)
        return result;

    result.guid = creature->GetGUID();
    result.creatureEntry = tmpl->Entry;
    result.healthPercent = creature->GetHealthPct();
    result.isElite = (creature->GetCreatureClassification() == CreatureClassifications::Elite ||
                      creature->GetCreatureClassification() == CreatureClassifications::RareElite);
    result.isBoss = creature->isWorldBoss();

    // Check CC state
    result.isCrowdControlled = creature->HasUnitState(UNIT_STATE_STUNNED | UNIT_STATE_ROOT |
                                                       UNIT_STATE_CONFUSED | UNIT_STATE_FLEEING);
    if (creature->HasBreakableByDamageAuraType(SPELL_AURA_MOD_STUN) ||
        creature->HasBreakableByDamageAuraType(SPELL_AURA_MOD_CONFUSE))
    {
        result.isCrowdControlled = true;
    }

    if (result.isCrowdControlled)
    {
        result.primaryType = AddType::CROWD_CONTROLLED;
        result.reason = "Crowd controlled - do not break";
        return result;
    }

    // Check casting state
    if (Spell* currentSpell = creature->GetCurrentSpell(CURRENT_GENERIC_SPELL))
    {
        result.castingSpellId = currentSpell->GetSpellInfo()->Id;
        SpellInfo const* spellInfo = currentSpell->GetSpellInfo();
        if (spellInfo && spellInfo->CastTimeEntry && spellInfo->CastTimeEntry->Base > 0)
            result.isInterruptible = true;
    }

    // Determine who this creature is targeting
    Unit* creatureTarget = creature->GetVictim();
    if (creatureTarget)
    {
        if (creatureTarget->GetGUID() == _bot->GetGUID())
            result.isTargetingBot = true;

        if (Player* targetPlayer = creatureTarget->ToPlayer())
        {
            if (IsGroupHealer(targetPlayer))
                result.isTargetingHealer = true;
            if (IsGroupTank(targetPlayer))
                result.isTargetingTank = true;
        }
    }

    // Determine primary type
    result.primaryType = DetermineCreatureType(creature, tmpl);

    // Build reason string
    switch (result.primaryType)
    {
        case AddType::HEALER:
            result.reason = "Healer add - kill first to prevent healing";
            break;
        case AddType::EXPLOSIVE:
            result.reason = "Explosive add - must be killed immediately";
            break;
        case AddType::FIXATE:
            result.reason = "Fixate add - kite or burn";
            break;
        case AddType::ENRAGED:
            result.reason = "Enraged add - soothe or burst down";
            break;
        case AddType::SHIELDING:
            result.reason = "Shielding add - interrupt shield or kill";
            break;
        case AddType::SUMMONER:
            result.reason = "Summoner add - kill to stop reinforcements";
            break;
        case AddType::BERSERKER:
            result.reason = "Berserker add - stacking damage, kill fast";
            break;
        case AddType::INTERRUPTIBLE:
            result.reason = "Casting dangerous spell - interrupt";
            break;
        case AddType::CASTER_DPS:
            result.reason = "Caster DPS add - focus/interrupt";
            break;
        case AddType::MELEE_DPS:
            result.reason = "Melee DPS add";
            break;
        case AddType::TANK_MOB:
            result.reason = "Tank mob - low priority";
            break;
        default:
            result.reason = "Unclassified add";
            break;
    }

    // Append targeting info
    if (result.isTargetingHealer)
        result.reason += " [TARGETING HEALER]";
    else if (result.isTargetingBot && _roleContext != BotRoleContext::TANK)
        result.reason += " [TARGETING ME]";

    return result;
}

// ============================================================================
// CREATURE TYPE DETERMINATION
// ============================================================================

AddType AddPrioritySystem::DetermineCreatureType(Creature* creature, CreatureTemplate const* tmpl) const
{
    if (!creature || !tmpl)
        return AddType::UNKNOWN;

    // Priority order of checks (most dangerous first):

    // 1. Explosive adds (M+ affix orbs, timed bomb creatures)
    if (IsExplosiveAdd(creature, tmpl))
        return AddType::EXPLOSIVE;

    // 2. Currently enraged
    if (IsCreatureEnraged(creature))
        return AddType::ENRAGED;

    // 3. Currently fixated on someone
    if (IsCreatureFixated(creature))
        return AddType::FIXATE;

    // 4. Berserker (stacking damage buff)
    if (IsCreatureBerserking(creature))
        return AddType::BERSERKER;

    // 5. Healing adds - actively casting heals
    if (IsCreatureHealing(creature))
        return AddType::HEALER;

    // 6. Shielding/buffing adds
    if (IsCreatureShielding(creature))
        return AddType::SHIELDING;

    // 7. Summoner adds
    if (IsCreatureSummoning(creature))
        return AddType::SUMMONER;

    // 8. Check if currently casting a dangerous interruptible spell
    if (creature->HasUnitState(UNIT_STATE_CASTING))
    {
        if (Spell* currentSpell = creature->GetCurrentSpell(CURRENT_GENERIC_SPELL))
        {
            SpellInfo const* spellInfo = currentSpell->GetSpellInfo();
            if (spellInfo && spellInfo->CastTimeEntry && spellInfo->CastTimeEntry->Base > 0)
            {
                // Check if it's a dangerous cast worth flagging
                bool isDangerous = false;

                for (SpellEffectInfo const& effect : spellInfo->GetEffects())
                {
                    if (effect.IsEffect(SPELL_EFFECT_SCHOOL_DAMAGE) ||
                        effect.IsEffect(SPELL_EFFECT_APPLY_AURA))
                    {
                        // High base damage or debuff application
                        isDangerous = true;
                        break;
                    }
                }

                if (isDangerous)
                    return AddType::INTERRUPTIBLE;
            }
        }
    }

    // 9. Creature unit class based classification
    uint8 unitClass = tmpl->unit_class;

    // Caster classes (Paladin=2 can heal, Mage=8 is pure caster)
    if (unitClass == UNIT_CLASS_MAGE)
        return AddType::CASTER_DPS;

    if (unitClass == UNIT_CLASS_PALADIN)
    {
        // Check if creature's spell list has heals
        for (uint8 i = 0; i < MAX_CREATURE_SPELLS; ++i)
        {
            uint32 spellId = tmpl->spells[i];
            if (spellId == 0)
                continue;

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
            if (spellInfo && spellInfo->HasEffect(SPELL_EFFECT_HEAL))
                return AddType::HEALER;
        }
        return AddType::CASTER_DPS;
    }

    // Rogue class = melee DPS
    if (unitClass == UNIT_CLASS_ROGUE)
        return AddType::MELEE_DPS;

    // Warrior class - check if it's a "tank-like" mob (high health)
    if (unitClass == UNIT_CLASS_WARRIOR)
    {
        // High health mobs are tank-type
        if (creature->GetMaxHealth() > _bot->GetMaxHealth() * 3)
            return AddType::TANK_MOB;
        return AddType::MELEE_DPS;
    }

    // Default to melee DPS for unknown
    return AddType::MELEE_DPS;
}

// ============================================================================
// CREATURE BEHAVIOR DETECTION
// ============================================================================

bool AddPrioritySystem::IsCreatureHealing(Creature* creature) const
{
    if (!creature)
        return false;

    // Check if currently casting a heal spell
    Spell* currentSpell = creature->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    if (currentSpell)
    {
        SpellInfo const* spellInfo = currentSpell->GetSpellInfo();
        if (spellInfo && spellInfo->HasEffect(SPELL_EFFECT_HEAL))
            return true;
    }

    // Check channeled spells
    Spell* channelSpell = creature->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
    if (channelSpell)
    {
        SpellInfo const* spellInfo = channelSpell->GetSpellInfo();
        if (spellInfo && spellInfo->HasEffect(SPELL_EFFECT_HEAL))
            return true;
    }

    // Check creature template spells for heal capability
    CreatureTemplate const* tmpl = creature->GetCreatureTemplate();
    if (tmpl)
    {
        uint32 healSpellCount = 0;
        for (uint8 i = 0; i < MAX_CREATURE_SPELLS; ++i)
        {
            uint32 spellId = tmpl->spells[i];
            if (spellId == 0)
                continue;

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
            if (spellInfo && spellInfo->HasEffect(SPELL_EFFECT_HEAL))
                healSpellCount++;
        }

        // If creature has multiple heal spells, it's a healer
        if (healSpellCount >= 2)
            return true;
    }

    return false;
}

bool AddPrioritySystem::IsCreatureShielding(Creature* creature) const
{
    if (!creature)
        return false;

    // Check if currently casting a buff/shield on allies
    Spell* currentSpell = creature->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    if (currentSpell)
    {
        SpellInfo const* spellInfo = currentSpell->GetSpellInfo();
        if (spellInfo)
        {
            // Check if spell targets allies and applies defensive auras
            for (SpellEffectInfo const& effect : spellInfo->GetEffects())
            {
                if (effect.IsEffect(SPELL_EFFECT_APPLY_AURA))
                {
                    if (effect.ApplyAuraName == SPELL_AURA_SCHOOL_ABSORB ||
                        effect.ApplyAuraName == SPELL_AURA_SCHOOL_IMMUNITY ||
                        effect.ApplyAuraName == SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN)
                    {
                        // Verify it's targeting an ally, not self-buff
                        Unit* target = currentSpell->m_targets.GetUnitTarget();
                        if (target && target != creature && !target->IsHostileTo(creature))
                            return true;
                    }
                }
            }
        }
    }

    return false;
}

bool AddPrioritySystem::IsCreatureSummoning(Creature* creature) const
{
    if (!creature)
        return false;

    // Check current cast for summon effects
    Spell* currentSpell = creature->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    if (currentSpell)
    {
        SpellInfo const* spellInfo = currentSpell->GetSpellInfo();
        if (spellInfo)
        {
            for (SpellEffectInfo const& effect : spellInfo->GetEffects())
            {
                if (effect.IsEffect(SPELL_EFFECT_SUMMON) ||
                    effect.IsEffect(SPELL_EFFECT_SUMMON_PET))
                    return true;
            }
        }
    }

    return false;
}

bool AddPrioritySystem::IsCreatureFixated(Creature* creature) const
{
    if (!creature)
        return false;

    // Check for fixate/taunt immunity auras that indicate fixate behavior
    // Fixate mobs typically have SPELL_AURA_MOD_TAUNT_AURA_IMMUNITY or similar
    if (creature->HasAuraType(SPELL_AURA_MOD_FIXATE))
        return true;

    // Check for the common fixate mechanic pattern:
    // The creature ignores threat and chases a specific player
    // Heuristic: if the creature has threat immunity and is targeting a non-tank, it's fixating
    if (creature->HasAuraType(SPELL_AURA_MOD_TAUNT))
    {
        Unit* target = creature->GetVictim();
        if (target && target->ToPlayer() && !IsGroupTank(target->ToPlayer()))
            return true;
    }

    return false;
}

bool AddPrioritySystem::IsCreatureEnraged(Creature* creature) const
{
    if (!creature)
        return false;

    // Check for enrage auras - creatures gain enrage buffs that increase damage
    // Common enrage aura types:
    if (creature->HasAuraType(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE) ||
        creature->HasAuraType(SPELL_AURA_MOD_ATTACK_POWER_PCT))
    {
        // Check if any of these are enrage-mechanic dispellable (druids can Soothe)
        Unit::AuraApplicationMap const& auras = creature->GetAppliedAuras();
        for (auto const& pair : auras)
        {
            Aura const* aura = pair.second->GetBase();
            if (!aura)
                continue;

            SpellInfo const* spellInfo = aura->GetSpellInfo();
            if (!spellInfo)
                continue;

            // Enrage dispel type = DISPEL_ENRAGE (9)
            if (spellInfo->Dispel == DISPEL_ENRAGE)
                return true;
        }
    }

    return false;
}

bool AddPrioritySystem::IsExplosiveAdd(Creature* creature, CreatureTemplate const* tmpl) const
{
    if (!creature || !tmpl)
        return false;

    // M+ Explosive affix creates orbs with very low health that must be killed quickly
    // These are typically non-targetable-by-tank, low HP, short-lived creatures

    // Heuristic: Very low max health relative to content level suggests explosive-type
    uint32 maxHealth = creature->GetMaxHealth();
    if (maxHealth > 0 && maxHealth < (_bot->GetMaxHealth() / 10))
    {
        // Extremely low HP creature in combat = likely explosive orb or similar
        // Additional check: not a critter or totem
        if (tmpl->type != CREATURE_TYPE_CRITTER && tmpl->type != CREATURE_TYPE_TOTEM)
            return true;
    }

    // Check creature spells for self-destruct / area damage on timer
    for (uint8 i = 0; i < MAX_CREATURE_SPELLS; ++i)
    {
        uint32 spellId = tmpl->spells[i];
        if (spellId == 0)
            continue;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (spellInfo)
        {
            for (SpellEffectInfo const& effect : spellInfo->GetEffects())
            {
                // Self-destruct pattern: instakill self + area damage
                if (effect.IsEffect(SPELL_EFFECT_INSTAKILL) ||
                    effect.IsEffect(SPELL_EFFECT_KILL_CREDIT))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

bool AddPrioritySystem::IsCreatureBerserking(Creature* creature) const
{
    if (!creature)
        return false;

    // Check for stacking damage buffs
    Unit::AuraApplicationMap const& auras = creature->GetAppliedAuras();
    for (auto const& pair : auras)
    {
        Aura const* aura = pair.second->GetBase();
        if (!aura)
            continue;

        SpellInfo const* spellInfo = aura->GetSpellInfo();
        if (!spellInfo)
            continue;

        // Stacking damage buff (stackable + modifies damage done)
        if (spellInfo->StackAmount > 1)
        {
            for (SpellEffectInfo const& effect : spellInfo->GetEffects())
            {
                if (effect.ApplyAuraName == SPELL_AURA_MOD_DAMAGE_PERCENT_DONE)
                {
                    // Has stacking damage buff with multiple stacks active
                    if (aura->GetStackAmount() >= 3)
                        return true;
                }
            }
        }
    }

    return false;
}

// ============================================================================
// PRIORITY SCORING
// ============================================================================

float AddPrioritySystem::CalculatePriorityScore(AddClassification& classification) const
{
    float score = GetBaseTypePriority(classification.primaryType);

    score = ApplyRoleAdjustment(score, classification);
    score = ApplyEncounterAdjustment(score, classification);
    score = ApplyHealthAdjustment(score, classification);
    score = ApplyDistanceAdjustment(score, classification.distance);

    // Targeting a healer is always high priority
    if (classification.isTargetingHealer && !classification.isTargetingTank)
        score += 80.0f;

    // Boss adds get a slight bump (not as much as type-based priority)
    if (classification.isBoss)
        score += 20.0f;

    // Elite adds are more important than normal
    if (classification.isElite)
        score += 15.0f;

    return std::max(0.0f, score);
}

float AddPrioritySystem::GetBaseTypePriority(AddType type) const
{
    switch (type)
    {
        case AddType::EXPLOSIVE:       return 250.0f; // Must die immediately
        case AddType::HEALER:          return 200.0f; // Kill first, prevents all progress
        case AddType::SHIELDING:       return 180.0f; // Prevents damage on other adds
        case AddType::SUMMONER:        return 170.0f; // Creates more problems over time
        case AddType::FIXATE:          return 160.0f; // Dangerous to non-tanks
        case AddType::BERSERKER:       return 150.0f; // Gets worse over time
        case AddType::ENRAGED:         return 140.0f; // High damage output
        case AddType::INTERRUPTIBLE:   return 130.0f; // Casting dangerous spell
        case AddType::CASTER_DPS:      return 100.0f; // Standard caster priority
        case AddType::MELEE_DPS:       return  80.0f; // Standard melee
        case AddType::TANK_MOB:        return  40.0f; // High HP, low urgency
        case AddType::CROWD_CONTROLLED: return  0.0f; // Do not attack
        case AddType::UNKNOWN:         return  60.0f; // Default
        default:                       return  60.0f;
    }
}

float AddPrioritySystem::ApplyRoleAdjustment(float basePriority, AddClassification const& add) const
{
    float adjusted = basePriority;

    switch (_roleContext)
    {
        case BotRoleContext::TANK:
            // Tanks prioritize picking up loose adds (not on any tank)
            if (!add.isTargetingTank && add.primaryType != AddType::CROWD_CONTROLLED)
                adjusted += 40.0f;
            // Tanks de-prioritize explosive (DPS should handle)
            if (add.primaryType == AddType::EXPLOSIVE)
                adjusted -= 50.0f;
            // Tanks prioritize adds targeting healers
            if (add.isTargetingHealer)
                adjusted += 60.0f;
            break;

        case BotRoleContext::MELEE_DPS:
            // Melee DPS prefer nearby targets
            if (add.distance < 8.0f)
                adjusted += 20.0f;
            // Melee should help with fixate adds
            if (add.primaryType == AddType::FIXATE && add.isTargetingBot)
                adjusted += 50.0f;
            break;

        case BotRoleContext::RANGED_DPS:
            // Ranged DPS should handle explosive adds
            if (add.primaryType == AddType::EXPLOSIVE)
                adjusted += 30.0f;
            // Ranged should prioritize healer adds (can reach them easily)
            if (add.primaryType == AddType::HEALER)
                adjusted += 20.0f;
            // Ranged should interrupt casters
            if (add.primaryType == AddType::INTERRUPTIBLE)
                adjusted += 25.0f;
            break;

        case BotRoleContext::HEALER:
            // Healers rarely switch targets, but flag danger
            // Drastically reduce all priority - healers should heal, not DPS
            adjusted *= 0.3f;
            // Exception: explosive adds that threaten the group
            if (add.primaryType == AddType::EXPLOSIVE)
                adjusted = basePriority * 0.6f;
            break;
    }

    return adjusted;
}

float AddPrioritySystem::ApplyEncounterAdjustment(float priority, AddClassification const& add) const
{
    float adjusted = priority;

    if (_encounterContext.isInMythicPlus)
    {
        // In M+, most add types are more urgent
        adjusted *= 1.15f;

        // Bolstering: Don't kill small adds near larger ones
        if (_encounterContext.hasBolsteringAffix && add.healthPercent < 30.0f)
        {
            // Check if there are higher HP adds nearby - reduce priority to avoid bolstering
            bool hasHighHealthNearby = false;
            for (auto const& pair : _classifiedAdds)
            {
                if (pair.first != add.guid &&
                    pair.second.healthPercent > 50.0f &&
                    pair.second.distance < 15.0f)
                {
                    hasHighHealthNearby = true;
                    break;
                }
            }
            if (hasHighHealthNearby)
                adjusted -= 40.0f;
        }

        // Bursting: Reduce priority if too many recent kills would stack bursting
        if (_encounterContext.hasBurstingAffix && add.healthPercent < 20.0f)
            adjusted -= 20.0f; // Slight reduction to stagger kills

        // Raging: At 30% HP mobs enrage, need to be killed fast at that point
        if (_encounterContext.hasRagingAffix && add.healthPercent < 30.0f)
            adjusted += 30.0f;

        // Spiteful: Ghosts fixate random players, high priority
        if (_encounterContext.hasSpitefulAffix && add.primaryType == AddType::FIXATE)
            adjusted += 40.0f;

        // Incorporeal: Must CC these, not kill
        if (_encounterContext.hasIncorporealAffix)
        {
            // If the creature is an incorporeal being, it should be CC'd not killed
            // Reduce priority as DPS target, the CC system should handle it
        }
    }

    if (_encounterContext.isInRaid)
    {
        // In raids, adds during a boss fight are critical
        if (_encounterContext.activeBossEncounterId > 0)
            adjusted *= 1.20f;
    }

    return adjusted;
}

float AddPrioritySystem::ApplyHealthAdjustment(float priority, AddClassification const& add) const
{
    // Execute bonus: finish off low HP adds
    if (add.healthPercent < 20.0f)
        priority += 25.0f;
    else if (add.healthPercent < 35.0f)
        priority += 10.0f;

    // High HP adds are slightly lower priority (take long to kill)
    if (add.healthPercent > 90.0f && add.primaryType == AddType::TANK_MOB)
        priority -= 15.0f;

    return priority;
}

float AddPrioritySystem::ApplyDistanceAdjustment(float priority, float distance) const
{
    // Close targets get a small bonus (less travel time)
    if (distance < 8.0f)
        priority += 10.0f;
    else if (distance < 15.0f)
        priority += 5.0f;
    else if (distance > 30.0f)
        priority -= 10.0f;

    return priority;
}

// ============================================================================
// URGENCY ASSESSMENT
// ============================================================================

AddUrgency AddPrioritySystem::DetermineUrgency(AddClassification const& add) const
{
    if (add.priorityScore >= EMERGENCY_THRESHOLD)
        return AddUrgency::EMERGENCY;
    if (add.priorityScore >= CRITICAL_THRESHOLD)
        return AddUrgency::CRITICAL;
    if (add.priorityScore >= HIGH_THRESHOLD)
        return AddUrgency::HIGH;
    if (add.priorityScore >= MODERATE_THRESHOLD)
        return AddUrgency::MODERATE;
    if (add.priorityScore > 0.0f)
        return AddUrgency::LOW;
    return AddUrgency::NONE;
}

void AddPrioritySystem::UpdateSituation()
{
    _situation.Reset();

    float highestScore = 0.0f;
    AddUrgency maxUrgency = AddUrgency::NONE;

    for (auto const& pair : _classifiedAdds)
    {
        AddClassification const& add = pair.second;

        if (add.isCrowdControlled)
        {
            _situation.crowdControlledAdds++;
            continue;
        }

        _situation.totalAdds++;

        // Count by type
        switch (add.primaryType)
        {
            case AddType::HEALER:       _situation.healerAdds++;       _situation.hasHealerAdd = true; break;
            case AddType::EXPLOSIVE:    _situation.explosiveAdds++;    _situation.hasExplosiveAdd = true; break;
            case AddType::FIXATE:
                _situation.fixateAdds++;
                if (add.isTargetingBot)
                    _situation.hasFixateOnBot = true;
                if (add.isTargetingHealer)
                    _situation.hasFixateOnHealer = true;
                break;
            case AddType::ENRAGED:      _situation.enragedAdds++;      _situation.hasEnragedAdd = true; break;
            case AddType::SHIELDING:    _situation.shieldingAdds++;    break;
            case AddType::SUMMONER:     _situation.summonerAdds++;     break;
            case AddType::INTERRUPTIBLE: _situation.interruptibleAdds++; break;
            default: break;
        }

        // Track highest priority
        if (add.priorityScore > highestScore)
        {
            highestScore = add.priorityScore;
            _situation.highestPriorityGuid = add.guid;
            _situation.highestPriorityScore = add.priorityScore;
            _situation.mostDangerousType = add.primaryType;
        }

        // Track max urgency
        if (add.urgency > maxUrgency)
            maxUrgency = add.urgency;
    }

    _situation.overallUrgency = maxUrgency;

    // Determine if immediate switch is needed
    _situation.needsImmediateSwitch =
        (maxUrgency >= AddUrgency::CRITICAL) ||
        _situation.hasExplosiveAdd ||
        (_situation.hasHealerAdd && _roleContext != BotRoleContext::HEALER) ||
        (_situation.hasFixateOnHealer && _roleContext == BotRoleContext::TANK);

    if (_situation.totalAdds > 0)
    {
        TC_LOG_TRACE("module.playerbot", "AddPrioritySystem [{}]: {} adds classified, urgency={}, "
            "healers={} explosive={} fixate={} enraged={}",
            _bot->GetName(), _situation.totalAdds, static_cast<uint8>(_situation.overallUrgency),
            _situation.healerAdds, _situation.explosiveAdds, _situation.fixateAdds, _situation.enragedAdds);
    }
}

// ============================================================================
// HELPERS
// ============================================================================

bool AddPrioritySystem::IsGroupHealer(Unit* unit) const
{
    if (!unit)
        return false;

    Player* player = unit->ToPlayer();
    if (!player)
        return false;

    // Check if in same group
    if (!_bot->GetGroup() || !player->GetGroup())
        return false;
    if (_bot->GetGroup() != player->GetGroup())
        return false;

    ChrSpecialization spec = player->GetPrimarySpecialization();
    switch (spec)
    {
        case ChrSpecialization::PriestDiscipline:
        case ChrSpecialization::PriestHoly:
        case ChrSpecialization::PaladinHoly:
        case ChrSpecialization::DruidRestoration:
        case ChrSpecialization::ShamanRestoration:
        case ChrSpecialization::MonkMistweaver:
        case ChrSpecialization::EvokerPreservation:
            return true;
        default:
            return false;
    }
}

bool AddPrioritySystem::IsGroupTank(Unit* unit) const
{
    if (!unit)
        return false;

    Player* player = unit->ToPlayer();
    if (!player)
        return false;

    if (!_bot->GetGroup() || !player->GetGroup())
        return false;
    if (_bot->GetGroup() != player->GetGroup())
        return false;

    ChrSpecialization spec = player->GetPrimarySpecialization();
    switch (spec)
    {
        case ChrSpecialization::WarriorProtection:
        case ChrSpecialization::PaladinProtection:
        case ChrSpecialization::DeathKnightBlood:
        case ChrSpecialization::DruidGuardian:
        case ChrSpecialization::MonkBrewmaster:
        case ChrSpecialization::DemonHunterVengeance:
            return true;
        default:
            return false;
    }
}

Unit* AddPrioritySystem::GetCreatureTarget(Creature* creature) const
{
    if (!creature)
        return nullptr;
    return creature->GetVictim();
}

BotRoleContext AddPrioritySystem::DetectRoleContext() const
{
    if (!_bot)
        return BotRoleContext::MELEE_DPS;

    ChrSpecialization spec = _bot->GetPrimarySpecialization();

    // Tank specs
    switch (spec)
    {
        case ChrSpecialization::WarriorProtection:
        case ChrSpecialization::PaladinProtection:
        case ChrSpecialization::DeathKnightBlood:
        case ChrSpecialization::DruidGuardian:
        case ChrSpecialization::MonkBrewmaster:
        case ChrSpecialization::DemonHunterVengeance:
            return BotRoleContext::TANK;
        default:
            break;
    }

    // Healer specs
    switch (spec)
    {
        case ChrSpecialization::PriestDiscipline:
        case ChrSpecialization::PriestHoly:
        case ChrSpecialization::PaladinHoly:
        case ChrSpecialization::DruidRestoration:
        case ChrSpecialization::ShamanRestoration:
        case ChrSpecialization::MonkMistweaver:
        case ChrSpecialization::EvokerPreservation:
            return BotRoleContext::HEALER;
        default:
            break;
    }

    // Ranged DPS specs
    switch (spec)
    {
        case ChrSpecialization::MageArcane:
        case ChrSpecialization::MageFire:
        case ChrSpecialization::MageFrost:
        case ChrSpecialization::WarlockAffliction:
        case ChrSpecialization::WarlockDemonology:
        case ChrSpecialization::WarlockDestruction:
        case ChrSpecialization::PriestShadow:
        case ChrSpecialization::HunterBeastMastery:
        case ChrSpecialization::HunterMarksmanship:
        case ChrSpecialization::DruidBalance:
        case ChrSpecialization::ShamanElemental:
        case ChrSpecialization::EvokerDevastation:
        case ChrSpecialization::EvokerAugmentation:
            return BotRoleContext::RANGED_DPS;
        default:
            break;
    }

    // Everything else is melee DPS
    return BotRoleContext::MELEE_DPS;
}

void AddPrioritySystem::UpdateEncounterContext()
{
    if (!_bot || !_bot->IsInWorld())
        return;

    Map* map = _bot->GetMap();
    if (!map)
        return;

    _encounterContext.isInDungeon = map->IsDungeon();
    _encounterContext.isInRaid = map->IsRaid();

    // M+ detection: check if map is a mythic dungeon with a keystone level
    // In TrinityCore, this would be detected via the challenge mode system
    // For now we check if it's a dungeon with difficulty that indicates M+
    if (_encounterContext.isInDungeon)
    {
        Difficulty diff = map->GetDifficultyID();
        // Mythic difficulty = 23 (DIFFICULTY_MYTHIC_DUNGEON)
        // Mythic+ = 8 (DIFFICULTY_MYTHIC_KEYSTONE)
        if (diff == DIFFICULTY_MYTHIC_KEYSTONE)
        {
            _encounterContext.isInMythicPlus = true;
            // Keystone level would come from the challenge mode data
            // For now, default to 0 (affix detection relies on aura checks)
        }
        else
        {
            _encounterContext.isInMythicPlus = false;
        }
    }

    // Group size detection
    if (Group* group = _bot->GetGroup())
        _encounterContext.groupSize = group->GetMembersCount();
    else
        _encounterContext.groupSize = 1;
}

} // namespace Playerbot
