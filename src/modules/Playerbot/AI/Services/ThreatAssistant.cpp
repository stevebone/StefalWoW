/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 */

#include "ThreatAssistant.h"
#include "GameTime.h"
#include "Creature.h"
#include "CreatureData.h"
#include "Player.h"
#include "Unit.h"
#include "Group.h"
#include "ThreatManager.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellAuraEffects.h"
#include "Log.h"
#include "../../Group/RoleDefinitions.h"  // For tank detection in GetPlayerRole
#include "../../Group/GroupMemberResolver.h"  // For safe group member iteration
#include <algorithm>

namespace Playerbot {
namespace bot { namespace ai {

// Static member initialization
std::unordered_map<ObjectGuid, uint32> ThreatAssistant::_lostAggroTimestamps;

// ThreatTarget implementation
float ThreatTarget::CalculateTauntPriority() const
{
    if (!unit)
        return 0.0f;

    // Base score: threat percentage × danger rating
    float score = (threatPercent / 100.0f) * dangerRating;

    // Distance penalty (further = lower priority)
    float distancePenalty = 1.0f / (1.0f + (distanceToTank / 10.0f));
    score *= distancePenalty;

    // Attacking vulnerable ally (healer/DPS)?
    if (isDangerous)
        score *= 2.0f;

    // Out of control bonus
    if (timeOutOfControl > 3000)
        score *= 1.5f;

    return score;
}

// ThreatAssistant implementation
Unit* ThreatAssistant::GetTauntTarget(Player* tank)
{
    if (!tank)
        return nullptr;

    std::vector<ThreatTarget> dangerous = GetDangerousTargets(tank, 60.0f);

    if (dangerous.empty())
        return nullptr;

    // Filter out targets already on tank
    std::vector<ThreatTarget> needsTaunt;
    for (const auto& target : dangerous)
    {
        if (!IsTargetOnTank(tank, target.unit))
            needsTaunt.push_back(target);
    }

    if (needsTaunt.empty())
        return nullptr;

    // Sort by taunt priority
    std::sort(needsTaunt.begin(), needsTaunt.end(),
        [](const ThreatTarget& a, const ThreatTarget& b) {
            return a.CalculateTauntPriority() > b.CalculateTauntPriority();
        });

    // Return highest priority target
    return needsTaunt.front().unit;
}

bool ThreatAssistant::ExecuteTaunt(Player* tank, Unit* target, uint32 tauntSpellId)
{
    if (!tank || !target)
        return false;

    // Check taunt immunity
    if (IsTauntImmune(target))
    {
        TC_LOG_DEBUG("playerbot", "ThreatAssistant: Target {} is taunt immune",
            target->GetName());
        return false;
    }

    // Get spell info
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(tauntSpellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    // Check range
    float range = spellInfo->GetMaxRange(false);
    if (tank->GetDistance(target) > range)
        return false;

    // Check line of sight
    if (!tank->IsWithinLOSInMap(target))
        return false;

    // Cast taunt spell
    ::SpellCastResult result = tank->CastSpell(target, tauntSpellId, false);

    if (result == SPELL_CAST_OK)
    {
        TC_LOG_DEBUG("playerbot", "ThreatAssistant: Tank {} successfully taunted {} with spell {}",
            tank->GetName(), target->GetName(), tauntSpellId);
        return true;
    }
    else
    {
        TC_LOG_DEBUG("playerbot", "ThreatAssistant: Tank {} failed to taunt {} with spell {} (result: {})",
            tank->GetName(), target->GetName(), tauntSpellId, static_cast<uint32>(result));
        return false;
    }
}

std::vector<ThreatTarget> ThreatAssistant::GetDangerousTargets(Player* tank, float minThreatPercent)
{
    std::vector<ThreatTarget> targets;

    if (!tank)
        return targets;

    std::vector<Unit*> enemies = GetCombatEnemies(tank, 40.0f);

    for (Unit* enemy : enemies)
    {
        if (!enemy || enemy->isDead())
            continue;

        ThreatTarget tt;
        tt.unit = enemy;
        tt.threatPercent = GetThreatPercentage(tank, enemy);
        tt.distanceToTank = tank->GetDistance(enemy);
        tt.isDangerous = IsAttackingVulnerableAlly(enemy);
        tt.timeOutOfControl = GetTimeOutOfControl(enemy);
        tt.dangerRating = CalculateDangerRating(enemy);

        Unit* victim = GetTargetVictim(enemy);
        if (victim)
            tt.currentTarget = victim->GetGUID();

        // Only include targets above threat threshold
    if (tt.threatPercent >= minThreatPercent || tt.isDangerous)
            targets.push_back(tt);
    }

    return targets;
}

bool ThreatAssistant::ShouldAoETaunt(Player* tank, uint32 minTargets)
{
    if (!tank)
        return false;

    std::vector<Unit*> enemies = GetCombatEnemies(tank, 40.0f);

    uint32 targetsNotOnTank = 0;

    for (Unit* enemy : enemies)
    {
        if (!enemy || enemy->isDead())
            continue;

        if (!IsTargetOnTank(tank, enemy))
            ++targetsNotOnTank;
    }

    return targetsNotOnTank >= minTargets;
}

float ThreatAssistant::GetThreatPercentage(Player* tank, Unit* target)
{
    if (!tank || !target)
        return 0.0f;

    ThreatManager& threatMgr = target->GetThreatManager();

    // Get tank's threat
    float tankThreat = threatMgr.GetThreat(tank);
    if (tankThreat <= 0.0f)
        return 0.0f;

    // Get highest threat (current victim)
    Unit* victim = target->GetVictim();
    if (!victim)
        return 0.0f;  // No victim = tank doesn't have aggro

    float highestThreat = threatMgr.GetThreat(victim);
    if (highestThreat <= 0.0f)
        return 100.0f;

    // Calculate percentage
    return (tankThreat / highestThreat) * 100.0f;
}

bool ThreatAssistant::IsTargetOnTank(Player* tank, Unit* target)
{
    if (!tank || !target)
        return false;

    Unit* victim = GetTargetVictim(target);
    return victim && victim->GetGUID() == tank->GetGUID();
}

Unit* ThreatAssistant::GetTargetVictim(Unit* target)
{
    if (!target)
        return nullptr;

    return target->GetVictim();
}

bool ThreatAssistant::IsTauntImmune(Unit* target)
{
    if (!target)
        return true;

    // Check if target is a creature with taunt immunity flags
    if (Creature* creature = target->ToCreature())
    {
        CreatureTemplate const* tmpl = creature->GetCreatureTemplate();
        if (tmpl)
        {
            // 1. Check explicit taunt immune flag (CREATURE_FLAG_EXTRA_NO_TAUNT)
            if (tmpl->flags_extra & CREATURE_FLAG_EXTRA_NO_TAUNT)
            {
                TC_LOG_DEBUG("playerbot", "ThreatAssistant::IsTauntImmune - {} is taunt immune via CREATURE_FLAG_EXTRA_NO_TAUNT",
                             target->GetName());
                return true;
            }
        }

        // 2. Check boss flags - dungeon bosses and world bosses are typically taunt immune
        // Note: Not all bosses are taunt immune, but most raid bosses are
        if (creature->IsDungeonBoss() || creature->isWorldBoss())
        {
            // Check type_flags for CREATURE_TYPE_FLAG_BOSS_MOB which indicates "??" level
            auto const* difficulty = creature->GetCreatureDifficulty();
            if (difficulty && (difficulty->TypeFlags & CREATURE_TYPE_FLAG_BOSS_MOB))
            {
                TC_LOG_DEBUG("playerbot", "ThreatAssistant::IsTauntImmune - {} is boss mob (typically taunt immune)",
                             target->GetName());
                return true;
            }
        }
    }

    // 3. Check for active mechanic immunity auras (e.g., from player abilities or boss mechanics)
    if (target->HasAuraType(SPELL_AURA_MECHANIC_IMMUNITY))
    {
        Unit::AuraEffectList const& immunities =
            target->GetAuraEffectsByType(SPELL_AURA_MECHANIC_IMMUNITY);

        for (AuraEffect const* aurEff : immunities)
        {
            // MECHANIC_TAUNTED = 36 in TrinityCore 11.x
            if (aurEff->GetMiscValue() == MECHANIC_TAUNTED)
            {
                TC_LOG_DEBUG("playerbot", "ThreatAssistant::IsTauntImmune - {} has taunt immunity aura",
                             target->GetName());
                return true;
            }
        }
    }

    return false;
}

std::vector<Unit*> ThreatAssistant::GetCombatEnemies(Player* tank, float range)
{
    std::vector<Unit*> enemies;

    if (!tank)
        return enemies;

    // Get all units in combat with tank's group
    Group* group = tank->GetGroup();
    if (!group)
    {
        // Solo: get tank's combat enemies
        ThreatManager& threatMgr = tank->GetThreatManager();
        auto const& threatList = threatMgr.GetSortedThreatList();

        for (auto const* ref : threatList)
        {
            if (!ref)
                continue;

            Unit* enemy = ref->GetVictim();
            if (enemy && tank->GetDistance(enemy) <= range)
                enemies.push_back(enemy);
        }

        return enemies;
    }

    // Group: aggregate all group members' threat lists
    // FIXED: Use GroupMemberResolver pattern for safe member iteration
    std::set<Unit*> uniqueEnemies;

    for (auto const& slot : group->GetMemberSlots())
    {
        Player* member = GroupMemberResolver::ResolveMember(slot.guid);
        if (!member || member->isDead())
            continue;

        ThreatManager& threatMgr = member->GetThreatManager();
        auto const& threatList = threatMgr.GetSortedThreatList();

        for (auto const* hostileRef : threatList)
        {
            if (!hostileRef)
                continue;

            Unit* enemy = hostileRef->GetVictim();
            if (enemy && tank->GetDistance(enemy) <= range)
                uniqueEnemies.insert(enemy);
        }
    }

    enemies.assign(uniqueEnemies.begin(), uniqueEnemies.end());
    return enemies;
}

bool ThreatAssistant::ShouldThisTankTaunt(
    Player* tank,
    const std::vector<Player*>& otherTanks,
    Unit* target)
{
    if (!tank || !target)
        return false;

    // If no other tanks, this tank should taunt
    if (otherTanks.empty())
        return true;

    // Assign based on proximity
    float closestDistance = tank->GetDistance(target);
    Player* closestTank = tank;

    for (Player* otherTank : otherTanks)
    {
        if (!otherTank)
            continue;

        float distance = otherTank->GetDistance(target);
        if (distance < closestDistance)
        {
            closestDistance = distance;
            closestTank = otherTank;
        }
    }

    // This tank should taunt if it's the closest
    return closestTank == tank;
}

// Private helper functions

float ThreatAssistant::CalculateDangerRating(Unit* target)
{
    if (!target)
        return 0.0f;

    float danger = 5.0f;  // Base danger

    // Elite/boss bonus
    if (Creature* creature = target->ToCreature())
    {
        if (creature->IsElite())
            danger += 2.0f;

        if (creature->isWorldBoss())
            danger = 10.0f;

        // Assess creature special abilities from template
        CreatureTemplate const* creatureTemplate = creature->GetCreatureTemplate();
        if (creatureTemplate)
        {
            // Check for heal capability - creatures with healing spells are higher priority
            for (uint8 i = 0; i < MAX_CREATURE_SPELLS; ++i)
            {
                uint32 spellId = creatureTemplate->spells[i];
                if (spellId == 0)
                    continue;

                SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
                if (spellInfo)
                {
                    // Healing ability detection
                    if (spellInfo->HasEffect(SPELL_EFFECT_HEAL) ||
                        spellInfo->HasEffect(SPELL_EFFECT_HEAL_PCT))
                    {
                        danger += 2.0f;  // Healers are high priority
                        break;
                    }

                    // Dangerous spell detection (fear, stun, charm)
                    if (spellInfo->HasAura(SPELL_AURA_MOD_FEAR) ||
                        spellInfo->HasAura(SPELL_AURA_MOD_STUN) ||
                        spellInfo->HasAura(SPELL_AURA_MOD_CHARM))
                    {
                        danger += 1.5f;  // CC abilities make target more dangerous
                    }
                }
            }

            // Creature type modifiers
            uint32 creatureType = creatureTemplate->type;
            if (creatureType == CREATURE_TYPE_HUMANOID)
                danger += 0.5f;  // Humanoids often have more complex AI
        }
    }

    // Caster bonus (ranged attacks, spells)
    if (target->GetPowerType() == POWER_MANA)
        danger += 1.0f;

    // Recent damage assessment - check if target is currently attacking
    if (Unit* victim = target->GetVictim())
    {
        if (victim->IsPlayer())
        {
            // Target is actively attacking a player
            danger += 1.0f;

            // Extra danger if attacking a non-tank player
            if (Player* victimPlayer = victim->ToPlayer())
            {
                PlayerRole role = GetPlayerRole(victimPlayer);
                if (role == PlayerRole::HEALER)
                    danger += 2.0f;  // Attacking healer = very dangerous
                else if (role == PlayerRole::DPS)
                    danger += 1.0f;  // Attacking DPS = moderately dangerous
            }
        }
    }

    // Health-based danger adjustment - low health targets are less dangerous
    float healthPct = target->GetHealthPct();
    if (healthPct < 20.0f)
        danger *= 0.7f;  // Low health = less threat
    else if (healthPct > 80.0f)
        danger *= 1.1f;  // Full health = slightly more dangerous

    return std::min(danger, 10.0f);
}

bool ThreatAssistant::IsAttackingVulnerableAlly(Unit* target)
{
    if (!target)
        return false;

    Unit* victim = GetTargetVictim(target);
    if (!victim || !victim->IsPlayer())
        return false;

    Player* victimPlayer = victim->ToPlayer();
    PlayerRole role = GetPlayerRole(victimPlayer);

    // Healer or DPS = vulnerable
    return role == PlayerRole::HEALER || role == PlayerRole::DPS;
}

ThreatAssistant::PlayerRole ThreatAssistant::GetPlayerRole(Player* player)
{
    if (!player)
        return PlayerRole::UNKNOWN;

    // Use RoleDefinitions system for accurate spec-based role detection
    uint8 classId = player->GetClass();
    uint8 specId = static_cast<uint8>(player->GetPrimarySpecialization());

    GroupRole groupRole = RoleDefinitions::GetPrimaryRole(classId, specId);

    switch (groupRole)
    {
        case GroupRole::TANK:
            return PlayerRole::TANK;

        case GroupRole::HEALER:
            return PlayerRole::HEALER;

        case GroupRole::MELEE_DPS:
        case GroupRole::RANGED_DPS:
        case GroupRole::SUPPORT:
            return PlayerRole::DPS;

        case GroupRole::NONE:
        default:
            return PlayerRole::UNKNOWN;
    }
}

float ThreatAssistant::CalculateDistancePenalty(Player* tank, Unit* target)
{
    if (!tank || !target)
        return 0.0f;

    float distance = tank->GetDistance(target);

    // Linear penalty: 1.0 at 0 yards, 0.5 at 20 yards, 0.0 at 40+ yards
    return std::max(0.0f, 1.0f - (distance / 40.0f));
}

uint32 ThreatAssistant::GetTimeOutOfControl(Unit* target)
{
    if (!target)
        return 0;

    ObjectGuid guid = target->GetGUID();
    auto it = _lostAggroTimestamps.find(guid);

    if (it == _lostAggroTimestamps.end())
        return 0;

    uint32 currentTime = GameTime::GetGameTimeMS();
    return currentTime - it->second;
}

}} // namespace bot::ai
} // namespace Playerbot
