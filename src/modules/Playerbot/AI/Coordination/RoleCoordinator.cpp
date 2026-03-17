/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "RoleCoordinator.h"
#include "../Combat/InterruptCoordinator.h"
#include "../../Advanced/GroupCoordinator.h"
#include "../../Session/BotSession.h"
#include "../../Session/BotSessionManager.h"
#include "../../Session/BotWorldSessionMgr.h"
#include "../../AI/BotAI.h"
#include "../../Core/Managers/IGameSystemsManager.h"
#include "Group.h"
#include "Player.h"
#include "Unit.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "GameTime.h"
#include "LFG.h"
#include "SpellHistory.h"
#include "SpellMgr.h"
#include <unordered_set>

namespace Playerbot
{
namespace Coordination
{

// ============================================================================
// Helper Functions for Group-Wide Role Queries
// ============================================================================

/**
 * @brief Get all group members with a specific role
 * @param group The per-bot GroupCoordinator
 * @param role The role to filter by
 * @return Vector of player GUIDs with the specified role
 *
 * This uses TrinityCore's Group API to iterate all members and checks each
 * member's role via their BotAI/GroupCoordinator. Works for both bots and human players.
 */
static ::std::vector<ObjectGuid> GetGroupMembersByRole(GroupCoordinator* group, GroupCoordinator::GroupRole role)
{
    ::std::vector<ObjectGuid> result;

    if (!group)
        return result;

    // Get the TrinityCore Group object
    Group* trinityGroup = group->GetGroup();
    if (!trinityGroup)
        return result;

    // Iterate all group members using TrinityCore 11.x API
    Group::MemberSlotList const& memberSlots = trinityGroup->GetMemberSlots();
    for (auto const& slot : memberSlots)
    {
        Player* member = ObjectAccessor::FindPlayer(slot.guid);
        if (!member || !member->IsAlive())
            continue;

        // Determine member's role
        GroupCoordinator::GroupRole memberRole = GroupCoordinator::GroupRole::UNDEFINED;

        // Check if this is a bot and get its GroupCoordinator
        // Use BotSessionManager to check if this player has a BotSession
        BotSession* botSession = BotSessionManager::GetBotSession(member->GetSession());
        if (botSession)
        {
            // This is a bot - get its assigned role from GroupCoordinator
            // BotSession::GetAI() provides access to the BotAI which has GameSystems
            BotAI* botAI = botSession->GetAI();
            if (botAI)
            {
                IGameSystemsManager* gameSystems = botAI->GetGameSystems();
                if (gameSystems)
                {
                    // Use static_cast since we know concrete type is GroupCoordinator
                    // This is needed to access GroupRole enum and GetRole() method
                    GroupCoordinator* botGroupCoord = static_cast<GroupCoordinator*>(gameSystems->GetGroupCoordinator());
                    if (botGroupCoord)
                    {
                        // Get the bot's assigned role from its own GroupCoordinator
                        memberRole = botGroupCoord->GetRole();
                    }
                }
            }

            // If role still undefined, fall back to LFG role assignment
            if (memberRole == GroupCoordinator::GroupRole::UNDEFINED)
            {
                uint8 lfgRoles = trinityGroup->GetLfgRoles(member->GetGUID());
                if (lfgRoles & lfg::PLAYER_ROLE_TANK)
                    memberRole = GroupCoordinator::GroupRole::TANK;
                else if (lfgRoles & lfg::PLAYER_ROLE_HEALER)
                    memberRole = GroupCoordinator::GroupRole::HEALER;
                else if (lfgRoles & lfg::PLAYER_ROLE_DAMAGE)
                {
                    // Distinguish melee vs ranged based on class/spec
                    switch (member->GetClass())
                    {
                        case CLASS_WARRIOR:
                        case CLASS_PALADIN:
                        case CLASS_ROGUE:
                        case CLASS_DEATH_KNIGHT:
                        case CLASS_MONK:
                        case CLASS_DEMON_HUNTER:
                            memberRole = GroupCoordinator::GroupRole::DPS_MELEE;
                            break;
                        case CLASS_HUNTER:
                        case CLASS_MAGE:
                        case CLASS_WARLOCK:
                        case CLASS_EVOKER:
                            memberRole = GroupCoordinator::GroupRole::DPS_RANGED;
                            break;
                        case CLASS_PRIEST:
                        case CLASS_SHAMAN:
                        case CLASS_DRUID:
                            // Could be either - check spec
                            memberRole = GroupCoordinator::GroupRole::DPS_RANGED;
                            break;
                        default:
                            memberRole = GroupCoordinator::GroupRole::DPS_MELEE;
                            break;
                    }
                }
            }
        }
        else
        {
            // Human player - determine role from specialization/group role assignment
            // Check TrinityCore's LFG role assignment
            uint8 lfgRoles = trinityGroup->GetLfgRoles(member->GetGUID());
            if (lfgRoles & lfg::PLAYER_ROLE_TANK)
                memberRole = GroupCoordinator::GroupRole::TANK;
            else if (lfgRoles & lfg::PLAYER_ROLE_HEALER)
                memberRole = GroupCoordinator::GroupRole::HEALER;
            else if (lfgRoles & lfg::PLAYER_ROLE_DAMAGE)
            {
                // Distinguish melee vs ranged based on class/spec
                switch (member->GetClass())
                {
                    case CLASS_WARRIOR:
                    case CLASS_PALADIN:
                    case CLASS_ROGUE:
                    case CLASS_DEATH_KNIGHT:
                    case CLASS_MONK:
                    case CLASS_DEMON_HUNTER:
                        memberRole = GroupCoordinator::GroupRole::DPS_MELEE;
                        break;
                    case CLASS_HUNTER:
                    case CLASS_MAGE:
                    case CLASS_WARLOCK:
                    case CLASS_EVOKER:
                        memberRole = GroupCoordinator::GroupRole::DPS_RANGED;
                        break;
                    case CLASS_PRIEST:
                    case CLASS_SHAMAN:
                    case CLASS_DRUID:
                        // Could be either - check spec
                        memberRole = GroupCoordinator::GroupRole::DPS_RANGED;
                        break;
                    default:
                        memberRole = GroupCoordinator::GroupRole::DPS_MELEE;
                        break;
                }
            }
        }

        // Check if role matches (handle DPS_MELEE/DPS_RANGED interchangeably when looking for DPS)
        bool roleMatches = (memberRole == role);
        if (!roleMatches && (role == GroupCoordinator::GroupRole::DPS_MELEE || role == GroupCoordinator::GroupRole::DPS_RANGED))
        {
            // When looking for DPS, accept both melee and ranged
            roleMatches = (memberRole == GroupCoordinator::GroupRole::DPS_MELEE ||
                           memberRole == GroupCoordinator::GroupRole::DPS_RANGED);
        }

        if (roleMatches)
        {
            result.push_back(member->GetGUID());
        }
    }

    return result;
}

/**
 * @brief Check if the group is currently in combat
 * @param group The per-bot GroupCoordinator
 * @return True if any group member is in combat
 */
static bool IsGroupInCombat(GroupCoordinator* group)
{
    if (!group)
        return false;

    Group* trinityGroup = group->GetGroup();
    if (!trinityGroup)
        return false;

    Group::MemberSlotList const& memberSlots = trinityGroup->GetMemberSlots();
    for (auto const& slot : memberSlots)
    {
        Player* member = ObjectAccessor::FindPlayer(slot.guid);
        if (member && member->IsInCombat())
            return true;
    }

    return false;
}

/**
 * @brief Get the group's focus target (from raid icons or main tank's target)
 * @param group The per-bot GroupCoordinator
 * @return GUID of the focus target, or empty if none
 */
static ObjectGuid GetGroupFocusTarget(GroupCoordinator* group)
{
    if (!group)
        return ObjectGuid::Empty;

    Group* trinityGroup = group->GetGroup();
    if (!trinityGroup)
        return ObjectGuid::Empty;

    // Check for skull (8) raid target marker - primary kill target
    // Index is 0-7, skull is 7 (TARGET_ICON_SKULL)
    ObjectGuid skullTarget = trinityGroup->GetTargetIcon(7);
    if (!skullTarget.IsEmpty())
    {
        Unit* target = ObjectAccessor::GetUnit(*trinityGroup->GetMembers().begin()->GetSource(), skullTarget);
        if (target && target->IsAlive())
            return skullTarget;
    }

    // Check for cross (X) marker - secondary kill target (index 6)
    ObjectGuid crossTarget = trinityGroup->GetTargetIcon(6);
    if (!crossTarget.IsEmpty())
    {
        Unit* target = ObjectAccessor::GetUnit(*trinityGroup->GetMembers().begin()->GetSource(), crossTarget);
        if (target && target->IsAlive())
            return crossTarget;
    }

    // Fall back to main tank's target
    ::std::vector<ObjectGuid> tanks = GetGroupMembersByRole(group, GroupCoordinator::GroupRole::TANK);
    if (!tanks.empty())
    {
        Player* mainTank = ObjectAccessor::FindPlayer(tanks[0]);
        if (mainTank && mainTank->IsInCombat())
        {
            Unit* target = mainTank->GetSelectedUnit();
            if (target && target->IsAlive() && target->IsHostileTo(mainTank))
                return target->GetGUID();
        }
    }

    return ObjectGuid::Empty;
}

/**
 * @brief Get combat duration for the group (time since first member entered combat)
 * @param group The per-bot GroupCoordinator
 * @return Combat duration in milliseconds, or 0 if not in combat
 */
static uint32 GetGroupCombatDuration(GroupCoordinator* group)
{
    // This would require tracking combat start time per group
    // For now, estimate based on whether we're in combat
    // A proper implementation would store combat start timestamp

    static ::std::unordered_map<uint64, uint32> s_combatStartTimes;

    if (!group)
        return 0;

    Group* trinityGroup = group->GetGroup();
    if (!trinityGroup)
        return 0;

    uint64 groupId = trinityGroup->GetGUID().GetCounter();
    uint32 now = GameTime::GetGameTimeMS();

    bool inCombat = IsGroupInCombat(group);

    if (inCombat)
    {
        // Track combat start time
        auto it = s_combatStartTimes.find(groupId);
        if (it == s_combatStartTimes.end())
        {
            s_combatStartTimes[groupId] = now;
            return 0;
        }
        return now - it->second;
    }
    else
    {
        // Clear combat start time when out of combat
        s_combatStartTimes.erase(groupId);
        return 0;
    }
}

// ============================================================================
// TankCoordinator
// ============================================================================

void TankCoordinator::Update(GroupCoordinator* group, uint32 diff)
{
    // Legacy method - creates temporary cache for backwards compatibility
    RoleCache tempCache;
    tempCache.tanks = GetGroupMembersByRole(group, GroupCoordinator::GroupRole::TANK);
    UpdateWithCache(group, diff, tempCache);
}

void TankCoordinator::UpdateWithCache(GroupCoordinator* group, uint32 diff, RoleCache const& cache)
{
    if (!group)
        return;

    _lastUpdateTime += diff;
    if (_lastUpdateTime < _updateInterval)
        return;

    _lastUpdateTime = 0;

    UpdateMainTank(group, cache);
    UpdateTankAssignments(group);
    UpdateTauntRotation(group);
}

ObjectGuid TankCoordinator::GetTankForTarget(ObjectGuid targetGuid) const
{
    auto it = _tankAssignments.find(targetGuid);
    if (it != _tankAssignments.end())
        return it->second;

    return ObjectGuid::Empty;
}

void TankCoordinator::RequestTankSwap()
{
    uint32 now = GameTime::GetGameTimeMS();

    // Check cooldown
    if (now < _lastTankSwapTime + _tankSwapCooldown)
    {
        TC_LOG_DEBUG("playerbot.coordination", "Tank swap on cooldown ({}ms remaining)",
            (_lastTankSwapTime + _tankSwapCooldown) - now);
        return;
    }

    // Swap main and off-tank
    ObjectGuid temp = _mainTank;
    _mainTank = _offTank;
    _offTank = temp;

    _lastTankSwapTime = now;

    TC_LOG_DEBUG("playerbot.coordination", "Tank swap executed: Main={}, Off={}",
        _mainTank.ToString(), _offTank.ToString());
}

bool TankCoordinator::NeedsTankSwap(ObjectGuid mainTankGuid) const
{
    Player* mainTank = ObjectAccessor::FindPlayer(mainTankGuid);
    if (!mainTank)
        return false;

    // Swap if main tank health critical (<20%)
    if (mainTank->GetHealthPct() < 20.0f)
        return true;

    // Swap if main tank has high stacks of tank debuff
    // Check for common tank swap debuffs - high stacks indicate need to swap
    // Common 11.x tank swap debuffs (stacks that increase damage taken):
    static const uint32 TANK_SWAP_DEBUFFS[] = {
        // Generic tank swap debuffs with stacking damage increase
        451288, // Darkflame Venom (Nerub-ar Palace)
        451277, // Acidic Eruption (various bosses)
        438708, // Void Corruption
        // Add more as boss content is encountered
    };

    for (uint32 debuffId : TANK_SWAP_DEBUFFS)
    {
        uint32 stacks = mainTank->GetAuraCount(debuffId);
        if (stacks >= 3) // Generally 3+ stacks requires swap
        {
            TC_LOG_DEBUG("playerbot.coordination", "Tank swap needed - {} has {} stacks of debuff {}",
                mainTank->GetName(), stacks, debuffId);
            return true;
        }
    }

    // Swap if main tank is out of defensive cooldowns
    // Check if tank has any active defensive buffs or cooldowns available
    // Common tank defensive cooldowns by class:
    static const uint32 TANK_DEFENSIVES[] = {
        // Warrior
        12975,  // Last Stand
        871,    // Shield Wall
        23920,  // Spell Reflection

        // Paladin
        642,    // Divine Shield
        31850,  // Ardent Defender
        86659,  // Guardian of Ancient Kings

        // Death Knight
        48792,  // Icebound Fortitude
        48707,  // Anti-Magic Shell
        55233,  // Vampiric Blood

        // Monk
        115203, // Fortifying Brew
        122278, // Dampen Harm
        122783, // Diffuse Magic

        // Druid
        22812,  // Barkskin
        61336,  // Survival Instincts

        // Demon Hunter
        187827, // Metamorphosis (tank form)
        204021, // Fiery Brand
        206803, // Void Reaver (damage reduction)
    };

    bool hasActiveDefensive = false;
    bool hasDefensiveAvailable = false;

    for (uint32 spellId : TANK_DEFENSIVES)
    {
        // Check if currently active
        if (mainTank->HasAura(spellId))
        {
            hasActiveDefensive = true;
            break;
        }

        // Check if on cooldown (if not on CD, it's available)
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (spellInfo && mainTank->HasSpell(spellId))
        {
            if (!mainTank->GetSpellHistory()->HasCooldown(spellId))
            {
                hasDefensiveAvailable = true;
            }
        }
    }

    // If tank is in danger (health < 40%) and has no active defensive and no cooldowns available, swap
    if (mainTank->GetHealthPct() < 40.0f && !hasActiveDefensive && !hasDefensiveAvailable)
    {
        TC_LOG_DEBUG("playerbot.coordination", "Tank swap needed - {} at {:.1f}% health with no defensives",
            mainTank->GetName(), mainTank->GetHealthPct());
        return true;
    }

    return false;
}

void TankCoordinator::UpdateMainTank(GroupCoordinator* group, RoleCache const& cache)
{
    // Use cached tank list - O(1) access instead of O(N) iteration
    ::std::vector<ObjectGuid> const& tanks = cache.tanks;

    if (tanks.empty())
    {
        _mainTank = ObjectGuid::Empty;
        _offTank = ObjectGuid::Empty;
        return;
    }

    // If we have a main tank and it's still alive, keep it
    if (!_mainTank.IsEmpty())
    {
        Player* mainTank = ObjectAccessor::FindPlayer(_mainTank);
        if (mainTank && mainTank->IsAlive())
        {
            // Update off-tank
    for (ObjectGuid tankGuid : tanks)
            {
                if (tankGuid != _mainTank)
                {
                    _offTank = tankGuid;
                    break;
                }
            }
            return;
        }
    }

    // Select new main tank (highest max health)
    float highestHealth = 0.0f;
    ObjectGuid bestTank;

    for (ObjectGuid tankGuid : tanks)
    {
        Player* tank = ObjectAccessor::FindPlayer(tankGuid);
        if (!tank || !tank->IsAlive())
            continue;

        float maxHealth = static_cast<float>(tank->GetMaxHealth());
        if (maxHealth > highestHealth)
        {
            highestHealth = maxHealth;
            bestTank = tankGuid;
        }
    }

    if (!bestTank.IsEmpty())
    {
        _mainTank = bestTank;

        // Select off-tank (second highest health)
        float secondHighestHealth = 0.0f;
        ObjectGuid secondBestTank;

        for (ObjectGuid tankGuid : tanks)
        {
            if (tankGuid == _mainTank)
                continue;

            Player* tank = ObjectAccessor::FindPlayer(tankGuid);
            if (!tank || !tank->IsAlive())
                continue;

            float maxHealth = static_cast<float>(tank->GetMaxHealth());
            if (maxHealth > secondHighestHealth)
            {
                secondHighestHealth = maxHealth;
                secondBestTank = tankGuid;
            }
        }

        _offTank = secondBestTank;

        TC_LOG_DEBUG("playerbot.coordination", "Tank roles assigned: Main={}, Off={}",
            _mainTank.ToString(), _offTank.ToString());
    }
}

void TankCoordinator::UpdateTankAssignments(GroupCoordinator* group)
{
    // Clear old assignments
    _tankAssignments.clear();

    if (_mainTank.IsEmpty())
        return;

    Player* mainTank = ObjectAccessor::FindPlayer(_mainTank);
    if (!mainTank || !mainTank->IsInCombat())
        return;

    // Assign main tank to current target
    Unit* mainTarget = mainTank->GetSelectedUnit();
    if (mainTarget && mainTarget->IsAlive())
    {
        _tankAssignments[mainTarget->GetGUID()] = _mainTank;
    }

    // Assign off-tank to adds
    if (!_offTank.IsEmpty())
    {
        Player* offTank = ObjectAccessor::FindPlayer(_offTank);
        if (offTank && offTank->IsInCombat())
        {
            Unit* offTarget = offTank->GetSelectedUnit();
            if (offTarget && offTarget->IsAlive() && offTarget->GetGUID() != mainTarget->GetGUID())
            {
                _tankAssignments[offTarget->GetGUID()] = _offTank;
            }
        }
    }
}

void TankCoordinator::UpdateTauntRotation(GroupCoordinator* group)
{
    // Check if tank swap is needed
    if (!_mainTank.IsEmpty() && NeedsTankSwap(_mainTank))
    {
        RequestTankSwap();
    }
}

// ============================================================================
// HealerCoordinator
// ============================================================================

void HealerCoordinator::Update(GroupCoordinator* group, uint32 diff)
{
    // Legacy method - creates temporary cache for backwards compatibility
    RoleCache tempCache;
    tempCache.healers = GetGroupMembersByRole(group, GroupCoordinator::GroupRole::HEALER);
    tempCache.tanks = GetGroupMembersByRole(group, GroupCoordinator::GroupRole::TANK);
    UpdateWithCache(group, diff, tempCache);
}

void HealerCoordinator::UpdateWithCache(GroupCoordinator* group, uint32 diff, RoleCache const& cache)
{
    if (!group)
        return;

    _lastUpdateTime += diff;
    if (_lastUpdateTime < _updateInterval)
        return;

    _lastUpdateTime = 0;

    UpdateHealingAssignments(group, cache);
    UpdateDispelCoordination(group);
    UpdateCooldownRotation(group, cache);
    UpdateManaManagement(group, cache);
}

ObjectGuid HealerCoordinator::GetHealerForTank(ObjectGuid tankGuid) const
{
    // O(1) lookup instead of O(N) vector search
    auto it = _tankToHealer.find(tankGuid);
    if (it != _tankToHealer.end())
        return it->second;

    return ObjectGuid::Empty;
}

void HealerCoordinator::AssignHealerToTank(ObjectGuid healerGuid, ObjectGuid tankGuid)
{
    // Check if this healer already has an assignment and remove old tank mapping
    auto oldAssignment = _healerToAssignment.find(healerGuid);
    if (oldAssignment != _healerToAssignment.end())
    {
        // Remove old tank → healer mapping
        _tankToHealer.erase(oldAssignment->second.targetGuid);
    }

    // Create new assignment
    HealingAssignment assignment;
    assignment.healerGuid = healerGuid;
    assignment.targetGuid = tankGuid;
    assignment.assignmentType = "tank";
    assignment.priority = 100; // Highest priority

    // Update O(1) lookup maps
    _healerToAssignment[healerGuid] = assignment;
    _tankToHealer[tankGuid] = healerGuid;

    // Also maintain vector for iteration (used in GetResurrectionPriority)
    _healingAssignments.erase(
        ::std::remove_if(_healingAssignments.begin(), _healingAssignments.end(),
            [healerGuid](HealingAssignment const& a) {
                return a.healerGuid == healerGuid;
            }),
        _healingAssignments.end()
    );
    _healingAssignments.push_back(assignment);

    TC_LOG_DEBUG("playerbot.coordination", "Assigned healer {} to tank {}",
        healerGuid.ToString(), tankGuid.ToString());
}

ObjectGuid HealerCoordinator::GetNextCooldownHealer(::std::string const& cooldownType) const
{
    uint32 now = GameTime::GetGameTimeMS();

    for (auto const& pair : _healerCooldowns)
    {
        ObjectGuid healerGuid = pair.first;
        auto const& cooldowns = pair.second;

        auto it = cooldowns.find(cooldownType);
        if (it == cooldowns.end() || now > it->second)
        {
            // Cooldown available
            return healerGuid;
        }
    }

    return ObjectGuid::Empty;
}

void HealerCoordinator::UseHealingCooldown(ObjectGuid healerGuid, ::std::string const& cooldownType, uint32 durationMs)
{
    uint32 expireTime = GameTime::GetGameTimeMS() + durationMs;
    _healerCooldowns[healerGuid][cooldownType] = expireTime;

    TC_LOG_DEBUG("playerbot.coordination", "Healer {} used cooldown: {} (expires in {}ms)",
        healerGuid.ToString(), cooldownType, durationMs);
}

::std::vector<ObjectGuid> HealerCoordinator::GetResurrectionPriority() const
{
    ::std::vector<ObjectGuid> priorities;

    // Priority order:
    // 1. Healers
    // 2. Tanks
    // 3. Ranged DPS
    // 4. Melee DPS
    for (auto const& assignment : _healingAssignments)
    {
        Player* target = ObjectAccessor::FindPlayer(assignment.targetGuid);
        if (target && !target->IsAlive())
        {
            priorities.push_back(assignment.targetGuid);
        }
    }

    return priorities;
}

void HealerCoordinator::UpdateHealingAssignments(GroupCoordinator* group, RoleCache const& cache)
{
    // Use cached healers and tanks - O(1) access instead of O(N) iteration
    ::std::vector<ObjectGuid> const& healers = cache.healers;
    ::std::vector<ObjectGuid> const& tanks = cache.tanks;

    if (healers.empty())
        return;

    // Assign healers to tanks (1:1 ratio if possible)
    size_t healerIndex = 0;
    for (ObjectGuid tankGuid : tanks)
    {
        if (healerIndex >= healers.size())
            break;

        ObjectGuid healerGuid = healers[healerIndex];

        // O(1) check if tank already has healer assigned
        auto it = _tankToHealer.find(tankGuid);
        if (it == _tankToHealer.end())
        {
            // Tank not assigned - assign this healer
            AssignHealerToTank(healerGuid, tankGuid);
            healerIndex++;
        }
        // else: tank already has healer, skip
    }

    // Remaining healers assigned to raid/group healing
    for (; healerIndex < healers.size(); ++healerIndex)
    {
        ObjectGuid healerGuid = healers[healerIndex];

        // Check if healer already assigned (O(1) lookup)
        auto existingAssignment = _healerToAssignment.find(healerGuid);
        if (existingAssignment != _healerToAssignment.end() &&
            existingAssignment->second.assignmentType != "raid")
        {
            // Already has a non-raid assignment, skip
            continue;
        }

        // Create raid healing assignment
        HealingAssignment assignment;
        assignment.healerGuid = healerGuid;
        assignment.targetGuid = ObjectGuid::Empty;
        assignment.assignmentType = "raid";
        assignment.priority = 50;

        _healerToAssignment[healerGuid] = assignment;

        // Also update vector
        _healingAssignments.erase(
            ::std::remove_if(_healingAssignments.begin(), _healingAssignments.end(),
                [healerGuid](HealingAssignment const& a) {
                    return a.healerGuid == healerGuid;
                }),
            _healingAssignments.end()
        );
        _healingAssignments.push_back(assignment);
    }
}

void HealerCoordinator::UpdateDispelCoordination(GroupCoordinator* group)
{
    // Dispel coordination is handled by GroupCoordinator::AssignDispel()
    // This method can be used for advanced dispel logic (e.g., prioritizing certain debuff types)

    // Clean up expired cooldowns
    uint32 now = GameTime::GetGameTimeMS();

    for (auto it = _healerCooldowns.begin(); it != _healerCooldowns.end(); ++it)
    {
        auto& cooldowns = it->second;
        for (auto cdIt = cooldowns.begin(); cdIt != cooldowns.end();)
        {
            if (now > cdIt->second)
                cdIt = cooldowns.erase(cdIt);
            else
                ++cdIt;
        }
    }
}

void HealerCoordinator::UpdateCooldownRotation(GroupCoordinator* group, RoleCache const& /*cache*/)
{
    // Rotate major healing cooldowns among healers
    // Examples: Tranquility, Aura Mastery, Divine Hymn, Revival

    // Check if group is in combat using helper function
    if (!IsGroupInCombat(group))
        return;

    // Get combat duration using helper function
    uint32 combatDuration = GetGroupCombatDuration(group);

    // Use cooldowns at specific combat milestones
    if (combatDuration > 30000 && combatDuration < 35000) // 30-35s into combat
    {
        ObjectGuid healer = GetNextCooldownHealer("major_cd");
        if (!healer.IsEmpty())
        {
            UseHealingCooldown(healer, "major_cd", 120000); // 2min CD
            TC_LOG_DEBUG("playerbot.coordination", "Rotating major healing cooldown at 30s mark");
        }
    }

    if (combatDuration > 60000 && combatDuration < 65000) // 60-65s into combat
    {
        ObjectGuid healer = GetNextCooldownHealer("major_cd");
        if (!healer.IsEmpty())
        {
            UseHealingCooldown(healer, "major_cd", 120000); // 2min CD
            TC_LOG_DEBUG("playerbot.coordination", "Rotating major healing cooldown at 60s mark");
        }
    }
}

void HealerCoordinator::UpdateManaManagement(GroupCoordinator* group, RoleCache const& cache)
{
    // Use cached healers - O(1) access instead of O(N) iteration
    ::std::vector<ObjectGuid> const& healers = cache.healers;

    float totalMana = 0.0f;
    float currentMana = 0.0f;
    uint32 healerCount = 0;

    for (ObjectGuid healerGuid : healers)
    {
        Player* healer = ObjectAccessor::FindPlayer(healerGuid);
        if (!healer)
            continue;

        totalMana += static_cast<float>(healer->GetMaxPower(POWER_MANA));
        currentMana += static_cast<float>(healer->GetPower(POWER_MANA));
        healerCount++;
    }

    if (healerCount == 0)
        return;

    float avgManaPct = (currentMana / totalMana) * 100.0f;

    // Coordinate mana conservation if group mana is low
    if (avgManaPct < 30.0f)
    {
        TC_LOG_DEBUG("playerbot.coordination", "Group healer mana low ({:.1f}%), coordinating conservation", avgManaPct);

        // Enter mana conservation mode if not already in it
        if (!_manaConservationMode)
        {
            _manaConservationMode = true;
            _manaConservationStartTime = GameTime::GetGameTimeMS();
            TC_LOG_DEBUG("playerbot.coordination", "Entering mana conservation mode");
        }

        // Signal each healer to conserve mana
        for (ObjectGuid healerGuid : healers)
        {
            Player* healer = ObjectAccessor::FindPlayer(healerGuid);
            if (!healer)
                continue;

            // Calculate individual conservation needs based on healer's mana
            float healerManaPct = (static_cast<float>(healer->GetPower(POWER_MANA)) /
                                    static_cast<float>(healer->GetMaxPower(POWER_MANA))) * 100.0f;

            // Set conservation flag for this healer
            // Healers with <50% mana should conserve, or all if group avg <30%
            bool shouldConserve = (healerManaPct < 50.0f || avgManaPct < 30.0f);
            _healerManaConservation[healerGuid] = shouldConserve;

            if (shouldConserve)
            {
                TC_LOG_DEBUG("playerbot.coordination", "Healer {} ({:.1f}% mana) flagged for mana conservation",
                    healer->GetName(), healerManaPct);
            }
        }
    }
    else if (_manaConservationMode && avgManaPct > 60.0f)
    {
        // Exit mana conservation mode when mana recovers above 60%
        _manaConservationMode = false;
        _healerManaConservation.clear();
        TC_LOG_DEBUG("playerbot.coordination", "Exiting mana conservation mode (avg mana {:.1f}%)", avgManaPct);
    }
}

bool HealerCoordinator::ShouldConserveMana(ObjectGuid healerGuid) const
{
    // Check global conservation mode
    if (!_manaConservationMode)
        return false;

    // Check individual healer flag
    auto it = _healerManaConservation.find(healerGuid);
    if (it != _healerManaConservation.end())
        return it->second;

    // Default to global mode
    return _manaConservationMode;
}

// ============================================================================
// DPSCoordinator
// ============================================================================

void DPSCoordinator::Update(GroupCoordinator* group, uint32 diff)
{
    // Legacy method - creates temporary cache for backwards compatibility
    RoleCache tempCache;
    tempCache.meleeDPS = GetGroupMembersByRole(group, GroupCoordinator::GroupRole::DPS_MELEE);
    tempCache.rangedDPS = GetGroupMembersByRole(group, GroupCoordinator::GroupRole::DPS_RANGED);
    tempCache.tanks = GetGroupMembersByRole(group, GroupCoordinator::GroupRole::TANK);
    UpdateWithCache(group, diff, tempCache);
}

void DPSCoordinator::UpdateWithCache(GroupCoordinator* group, uint32 diff, RoleCache const& cache)
{
    if (!group)
        return;

    _lastUpdateTime += diff;
    if (_lastUpdateTime < _updateInterval)
        return;

    _lastUpdateTime = 0;

    UpdateFocusTarget(group, cache);
    UpdateInterruptRotation(group, cache);
    UpdateCCAssignments(group);
    UpdateBurstWindows(group);
}

void DPSCoordinator::SetFocusTarget(ObjectGuid targetGuid)
{
    if (_focusTarget != targetGuid)
    {
        _focusTarget = targetGuid;
        TC_LOG_DEBUG("playerbot.coordination", "DPS focus target changed to {}", targetGuid.ToString());
    }
}

ObjectGuid DPSCoordinator::GetNextInterrupter() const
{
    // Phase 2 Architecture: Delegate to InterruptCoordinator (single authority)
    if (_interruptCoordinator)
    {
        // InterruptCoordinator manages all interrupt assignments centrally
        // GetPendingAssignments returns vector of InterruptAssignment structs
        auto pending = _interruptCoordinator->GetPendingAssignments();
        if (!pending.empty())
        {
            return pending.front().assignedBot; // Return first assigned bot
        }
        return ObjectGuid::Empty;
    }

    // Fallback: Legacy implementation (only used if no InterruptCoordinator set)
    uint32 now = GameTime::GetGameTimeMS();

    for (auto const& interrupt : _interruptRotation)
    {
        if (now > interrupt.cooldownExpire)
        {
            return interrupt.dpsGuid;
        }
    }

    return ObjectGuid::Empty;
}

void DPSCoordinator::AssignInterrupt(ObjectGuid dpsGuid, ObjectGuid targetGuid)
{
    // Phase 2 Architecture: Delegate to InterruptCoordinator (single authority)
    if (_interruptCoordinator)
    {
        // InterruptCoordinator handles interrupt assignment via OnEnemyCastStart events
        // Direct assignment is deprecated - interrupts are triggered by cast detection
        // Simply register the bot as available for interrupts (updates cooldown to ready)
        _interruptCoordinator->UpdateBotCooldown(dpsGuid, 0); // 0 = ready to interrupt

        TC_LOG_DEBUG("playerbot.coordination",
            "DPSCoordinator delegated interrupt for {} to InterruptCoordinator (target: {})",
            dpsGuid.ToString(), targetGuid.ToString());
        return;
    }

    // Fallback: Legacy implementation (only used if no InterruptCoordinator set)
    uint32 now = GameTime::GetGameTimeMS();

    InterruptAssignment assignment;
    assignment.dpsGuid = dpsGuid;
    assignment.targetGuid = targetGuid;
    assignment.assignedTime = now;
    assignment.cooldownExpire = now + 24000; // ~24s CD (most interrupts are 24s)

    // Remove old assignment for this DPS
    _interruptRotation.erase(
        ::std::remove_if(_interruptRotation.begin(), _interruptRotation.end(),
            [dpsGuid](InterruptAssignment const& assign) {
                return assign.dpsGuid == dpsGuid;
            }),
        _interruptRotation.end()
    );

    _interruptRotation.push_back(assignment);

    TC_LOG_DEBUG("playerbot.coordination", "Assigned interrupt to {} for target {}",
        dpsGuid.ToString(), targetGuid.ToString());
}

ObjectGuid DPSCoordinator::GetCCAssignment(ObjectGuid dpsGuid) const
{
    for (auto const& ccAssignment : _ccAssignments)
    {
        if (ccAssignment.dpsGuid == dpsGuid)
            return ccAssignment.targetGuid;
    }

    return ObjectGuid::Empty;
}

void DPSCoordinator::AssignCC(ObjectGuid dpsGuid, ObjectGuid targetGuid, ::std::string const& ccType)
{
    uint32 now = GameTime::GetGameTimeMS();

    // Remove old assignment
    _ccAssignments.erase(
        ::std::remove_if(_ccAssignments.begin(), _ccAssignments.end(),
            [dpsGuid](CCAssignment const& assign) {
                return assign.dpsGuid == dpsGuid;
            }),
        _ccAssignments.end()
    );

    // Create new assignment
    CCAssignment assignment;
    assignment.dpsGuid = dpsGuid;
    assignment.targetGuid = targetGuid;
    assignment.ccType = ccType;
    assignment.assignedTime = now;

    _ccAssignments.push_back(assignment);

    TC_LOG_DEBUG("playerbot.coordination", "Assigned CC ({}) to {} for target {}",
        ccType, dpsGuid.ToString(), targetGuid.ToString());
}

void DPSCoordinator::RequestBurstWindow(uint32 durationMs)
{
    if (_inBurstWindow)
    {
        TC_LOG_DEBUG("playerbot.coordination", "Burst window already active");
        return;
    }

    _inBurstWindow = true;
    _burstWindowStart = GameTime::GetGameTimeMS();
    _burstWindowDuration = durationMs;

    TC_LOG_DEBUG("playerbot.coordination", "Burst window activated for {}ms", durationMs);
}

bool DPSCoordinator::InBurstWindow() const
{
    if (!_inBurstWindow)
        return false;

    uint32 now = GameTime::GetGameTimeMS();
    return now < _burstWindowStart + _burstWindowDuration;
}

void DPSCoordinator::UpdateFocusTarget(GroupCoordinator* group, RoleCache const& cache)
{
    // Use cached tanks for focus target determination
    ObjectGuid groupFocus = ObjectGuid::Empty;

    Group* trinityGroup = group ? group->GetGroup() : nullptr;
    if (trinityGroup)
    {
        // Check for skull marker (primary kill target)
        ObjectGuid skullTarget = trinityGroup->GetTargetIcon(7);
        if (!skullTarget.IsEmpty())
        {
            auto it = trinityGroup->GetMembers().begin();
            if (it != trinityGroup->GetMembers().end())
            {
                Unit* target = ObjectAccessor::GetUnit(*it->GetSource(), skullTarget);
                if (target && target->IsAlive())
                    groupFocus = skullTarget;
            }
        }

        // Fall back to cross marker
        if (groupFocus.IsEmpty())
        {
            ObjectGuid crossTarget = trinityGroup->GetTargetIcon(6);
            if (!crossTarget.IsEmpty())
            {
                auto it = trinityGroup->GetMembers().begin();
                if (it != trinityGroup->GetMembers().end())
                {
                    Unit* target = ObjectAccessor::GetUnit(*it->GetSource(), crossTarget);
                    if (target && target->IsAlive())
                        groupFocus = crossTarget;
                }
            }
        }

        // Fall back to main tank's target using cached tanks
        if (groupFocus.IsEmpty() && !cache.tanks.empty())
        {
            Player* mainTank = ObjectAccessor::FindPlayer(cache.tanks[0]);
            if (mainTank && mainTank->IsInCombat())
            {
                Unit* target = mainTank->GetSelectedUnit();
                if (target && target->IsAlive() && target->IsHostileTo(mainTank))
                    groupFocus = target->GetGUID();
            }
        }
    }

    if (groupFocus != _focusTarget)
    {
        SetFocusTarget(groupFocus);
    }
}

void DPSCoordinator::UpdateInterruptRotation(GroupCoordinator* group, RoleCache const& cache)
{
    // Phase 2 Architecture: Skip when InterruptCoordinator is handling interrupts
    if (_interruptCoordinator)
    {
        // InterruptCoordinator maintains its own interrupt rotation via RegisterBot/UpdateBotCooldown
        // DPS registration should happen at bot creation/group join, not in periodic updates
        return;
    }

    // Fallback: Legacy implementation (only used if no InterruptCoordinator set)
    uint32 now = GameTime::GetGameTimeMS();

    // Clean up expired interrupt cooldowns
    _interruptRotation.erase(
        ::std::remove_if(_interruptRotation.begin(), _interruptRotation.end(),
            [now](InterruptAssignment const& assign) {
                return now > assign.cooldownExpire + 30000; // Remove after 30s past expiration
            }),
        _interruptRotation.end()
    );

    // Use cached DPS lists - O(1) access instead of O(N) iteration
    ::std::vector<ObjectGuid> const& meleeDPS = cache.meleeDPS;
    ::std::vector<ObjectGuid> const& rangedDPS = cache.rangedDPS;

    // Build set of existing DPS in rotation for O(1) lookup
    ::std::unordered_set<ObjectGuid> existingDPS;
    existingDPS.reserve(_interruptRotation.size());
    for (auto const& interrupt : _interruptRotation)
    {
        existingDPS.insert(interrupt.dpsGuid);
    }

    // Add new melee DPS to rotation
    for (ObjectGuid dpsGuid : meleeDPS)
    {
        if (existingDPS.find(dpsGuid) == existingDPS.end())
        {
            InterruptAssignment assignment;
            assignment.dpsGuid = dpsGuid;
            assignment.targetGuid = ObjectGuid::Empty;
            assignment.assignedTime = 0;
            assignment.cooldownExpire = 0; // Ready immediately

            _interruptRotation.push_back(assignment);
            existingDPS.insert(dpsGuid);
        }
    }

    // Add new ranged DPS to rotation
    for (ObjectGuid dpsGuid : rangedDPS)
    {
        if (existingDPS.find(dpsGuid) == existingDPS.end())
        {
            InterruptAssignment assignment;
            assignment.dpsGuid = dpsGuid;
            assignment.targetGuid = ObjectGuid::Empty;
            assignment.assignedTime = 0;
            assignment.cooldownExpire = 0; // Ready immediately

            _interruptRotation.push_back(assignment);
        }
    }
}

void DPSCoordinator::UpdateCCAssignments(GroupCoordinator* group)
{
    uint32 now = GameTime::GetGameTimeMS();

    // Clean up old CC assignments (>60s old)
    _ccAssignments.erase(
        ::std::remove_if(_ccAssignments.begin(), _ccAssignments.end(),
            [now](CCAssignment const& assign) {
                return now > assign.assignedTime + 60000;
            }),
        _ccAssignments.end()
    );
}

void DPSCoordinator::UpdateBurstWindows(GroupCoordinator* group)
{
    // Check if burst window expired
    if (_inBurstWindow && !InBurstWindow())
    {
        _inBurstWindow = false;
        TC_LOG_DEBUG("playerbot.coordination", "Burst window ended");
    }

    // Automatic burst windows at specific combat timings
    // Check group combat status using helper function
    if (!IsGroupInCombat(group))
        return;

    // Get combat duration using helper function
    uint32 combatDuration = GetGroupCombatDuration(group);

    // Initial burst (0-10s)
    if (combatDuration > 0 && combatDuration < 2000 && !_inBurstWindow)
    {
        RequestBurstWindow(10000); // 10s burst
    }

    // Coordinated burst at 2 minutes (execute phase simulation)
    if (combatDuration > 120000 && combatDuration < 122000 && !_inBurstWindow)
    {
        RequestBurstWindow(20000); // 20s burst
    }
}

// ============================================================================
// RoleCoordinatorManager
// ============================================================================

RoleCoordinatorManager::RoleCoordinatorManager()
{
    _tankCoordinator = ::std::make_unique<TankCoordinator>();
    _healerCoordinator = ::std::make_unique<HealerCoordinator>();
    _dpsCoordinator = ::std::make_unique<DPSCoordinator>();
}

void RoleCoordinatorManager::RefreshRoleCache(GroupCoordinator* group)
{
    _roleCache.tanks.clear();
    _roleCache.healers.clear();
    _roleCache.meleeDPS.clear();
    _roleCache.rangedDPS.clear();

    if (!group)
        return;

    Group* trinityGroup = group->GetGroup();
    if (!trinityGroup)
        return;

    // Single iteration through all group members - O(N) instead of 4 × O(N)
    Group::MemberSlotList const& memberSlots = trinityGroup->GetMemberSlots();
    for (auto const& slot : memberSlots)
    {
        Player* member = ObjectAccessor::FindPlayer(slot.guid);
        if (!member || !member->IsAlive())
            continue;

        // Determine member's role
        GroupCoordinator::GroupRole memberRole = GroupCoordinator::GroupRole::UNDEFINED;

        // Check if this is a bot
        BotSession* botSession = BotSessionManager::GetBotSession(member->GetSession());
        if (botSession)
        {
            BotAI* botAI = botSession->GetAI();
            if (botAI)
            {
                IGameSystemsManager* gameSystems = botAI->GetGameSystems();
                if (gameSystems)
                {
                    GroupCoordinator* botGroupCoord = static_cast<GroupCoordinator*>(gameSystems->GetGroupCoordinator());
                    if (botGroupCoord)
                        memberRole = botGroupCoord->GetRole();
                }
            }
        }

        // Fallback to LFG role
        if (memberRole == GroupCoordinator::GroupRole::UNDEFINED)
        {
            uint8 lfgRoles = trinityGroup->GetLfgRoles(member->GetGUID());
            if (lfgRoles & lfg::PLAYER_ROLE_TANK)
                memberRole = GroupCoordinator::GroupRole::TANK;
            else if (lfgRoles & lfg::PLAYER_ROLE_HEALER)
                memberRole = GroupCoordinator::GroupRole::HEALER;
            else if (lfgRoles & lfg::PLAYER_ROLE_DAMAGE)
            {
                // Determine melee vs ranged from class
                switch (member->GetClass())
                {
                    case CLASS_WARRIOR:
                    case CLASS_PALADIN:
                    case CLASS_ROGUE:
                    case CLASS_DEATH_KNIGHT:
                    case CLASS_MONK:
                    case CLASS_DEMON_HUNTER:
                        memberRole = GroupCoordinator::GroupRole::DPS_MELEE;
                        break;
                    default:
                        memberRole = GroupCoordinator::GroupRole::DPS_RANGED;
                        break;
                }
            }
        }

        // Sort into appropriate cache
        switch (memberRole)
        {
            case GroupCoordinator::GroupRole::TANK:
                _roleCache.tanks.push_back(member->GetGUID());
                break;
            case GroupCoordinator::GroupRole::HEALER:
                _roleCache.healers.push_back(member->GetGUID());
                break;
            case GroupCoordinator::GroupRole::DPS_MELEE:
                _roleCache.meleeDPS.push_back(member->GetGUID());
                break;
            case GroupCoordinator::GroupRole::DPS_RANGED:
                _roleCache.rangedDPS.push_back(member->GetGUID());
                break;
            default:
                break;
        }
    }

    _roleCache.lastRefreshTime = GameTime::GetGameTimeMS();
    _roleCache.dirty = false;

    TC_LOG_DEBUG("playerbot.coordination",
        "RoleCache refreshed: {} tanks, {} healers, {} melee, {} ranged",
        _roleCache.tanks.size(), _roleCache.healers.size(),
        _roleCache.meleeDPS.size(), _roleCache.rangedDPS.size());
}

void RoleCoordinatorManager::Update(GroupCoordinator* group, uint32 diff)
{
    if (!group)
        return;

    // Refresh cache once per update cycle if needed - all coordinators share this
    uint32 now = GameTime::GetGameTimeMS();
    if (_roleCache.NeedsRefresh(now))
    {
        RefreshRoleCache(group);
    }

    // Use cached roles for all coordinator updates
    _tankCoordinator->UpdateWithCache(group, diff, _roleCache);
    _healerCoordinator->UpdateWithCache(group, diff, _roleCache);
    _dpsCoordinator->UpdateWithCache(group, diff, _roleCache);
}

} // namespace Coordination
} // namespace Playerbot
