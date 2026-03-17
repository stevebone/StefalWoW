/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "RoleAssignment.h"
#include "GameTime.h"
#include "Item.h"
#include "SharedDefines.h"
#include "DBCEnums.h"
#include "ItemTemplate.h"
#include "Player.h"
#include "Group.h"
#include "GroupMgr.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include <algorithm>

namespace Playerbot
{

RoleAssignment::RoleAssignment(Player* bot) : _bot(bot)
{
    if (!_bot) TC_LOG_ERROR("playerbot.group", "RoleAssignment: null bot!");
    InitializeClassRoleMappings();
    _globalStatistics.Reset();
}

RoleAssignment::~RoleAssignment() {}



bool RoleAssignment::AssignRoles(Group* group, RoleAssignmentStrategy strategy)
{
    if (!group)
        return false;

    uint32 groupId = group->GetGUID().GetCounter();
    _groupStrategies[groupId] = strategy;

    TC_LOG_DEBUG("playerbot", "RoleAssignment: Assigning roles for group %u with strategy %u",
                 groupId, static_cast<uint8>(strategy));

    // Analyze all group members
    for (GroupReference const& itr : group->GetMembers())
    {
        if (Player* member = itr.GetSource())
        {
            // For each member, build their profile
            PlayerRoleProfile profile(member->GetGUID().GetCounter(), member->GetClass(), 0, member->GetLevel());
            BuildPlayerProfile(profile);
            CalculateRoleCapabilities(profile);
            _playerProfiles[member->GetGUID().GetCounter()] = profile;
        }
    }

    // Execute assignment strategy
    switch (strategy)
    {
        case RoleAssignmentStrategy::OPTIMAL_ASSIGNMENT:
            ExecuteOptimalStrategy(group);
            break;
        case RoleAssignmentStrategy::BALANCED_ASSIGNMENT:
            ExecuteBalancedStrategy(group);
            break;
        case RoleAssignmentStrategy::FLEXIBLE_ASSIGNMENT:
            ExecuteFlexibleStrategy(group);
            break;
        case RoleAssignmentStrategy::STRICT_PRIMARY_ONLY:
            ExecuteStrictStrategy(group);
            break;
        case RoleAssignmentStrategy::HYBRID_CLASS_FRIENDLY:
            ExecuteHybridStrategy(group);
            break;
        case RoleAssignmentStrategy::DUNGEON_CONTENT_FOCUSED:
            ExecuteDungeonStrategy(group);
            break;
        case RoleAssignmentStrategy::RAID_CONTENT_FOCUSED:
            ExecuteRaidStrategy(group);
            break;
        case RoleAssignmentStrategy::PVP_CONTENT_FOCUSED:
            ExecutePvPStrategy(group);
            break;
        default:
            ExecuteOptimalStrategy(group);
            break;
    }

    // Validate final assignment
    bool isValid = ValidateRoleAssignment(group);

    _globalStatistics.totalAssignments.fetch_add(1);
    if (isValid)
        _globalStatistics.successfulAssignments.fetch_add(1);

    return isValid;
}

bool RoleAssignment::AssignRole(GroupRole role, Group* group)
{
    if (!group || !_bot)
        return false;

    uint32 botGuid = _bot->GetGUID().GetCounter();

    // Update player profile
    auto it = _playerProfiles.find(botGuid);
    if (it != _playerProfiles.end())
    {
        it->second.assignedRole = role;
        it->second.lastRoleUpdate = GameTime::GetGameTimeMS();
    }
    else
    {
        PlayerRoleProfile profile = AnalyzePlayerCapabilities();
        profile.assignedRole = role;
        _playerProfiles[botGuid] = profile;
    }

    NotifyRoleAssignment(role, group);

    TC_LOG_DEBUG("playerbot", "RoleAssignment: Assigned role %u to player %s in group %u",
                 static_cast<uint8>(role), _bot->GetName().c_str(), group->GetGUID().GetCounter());

    return true;
}

bool RoleAssignment::SwapRoles(uint32 player1Guid, uint32 player2Guid, Group* group)
{
    if (!group)
        return false;

    auto it1 = _playerProfiles.find(player1Guid);
    auto it2 = _playerProfiles.find(player2Guid);

    if (it1 == _playerProfiles.end() || it2 == _playerProfiles.end())
        return false;

    GroupRole role1 = it1->second.assignedRole;
    GroupRole role2 = it2->second.assignedRole;

    // Check if players can perform swapped roles
    if (!CanPlayerSwitchRole(role2, group) ||
        !CanPlayerSwitchRole(role1, group))
    {
        return false;
    }

    // Perform the swap
    it1->second.assignedRole = role2;
    it2->second.assignedRole = role1;

    TC_LOG_DEBUG("playerbot", "RoleAssignment: Swapped roles between players %u and %u in group %u",
                 player1Guid, player2Guid, group->GetGUID().GetCounter());

    return true;
}

PlayerRoleProfile RoleAssignment::AnalyzePlayerCapabilities()
{
    if (!_bot)
        return PlayerRoleProfile(0, 0, 0, 0);

    PlayerRoleProfile profile(_bot->GetGUID().GetCounter(), _bot->GetClass(), 0, _bot->GetLevel());

    BuildPlayerProfile(profile);
    CalculateRoleCapabilities(profile);
    AnalyzePlayerGear(profile);
    UpdateRoleExperience(profile);

    return profile;
}

std::vector<RoleScore> RoleAssignment::CalculateRoleScores( Group* group)
{
    std::vector<RoleScore> scores;

    if (!_bot)
        return scores;

    // Calculate scores for all possible roles
    for (int roleInt = 0; roleInt <= static_cast<int>(GroupRole::UTILITY); ++roleInt)
    {
        GroupRole role = static_cast<GroupRole>(roleInt);
        RoleScore score(role);

        score.effectiveness = CalculateClassRoleEffectiveness(_bot->GetClass(), 0, role);
        score.gearScore = CalculateGearScore(role);
        score.experienceScore = CalculateExperienceScore(role);
        score.synergy = CalculateSynergyScore(role, group);
        score.availabilityScore = 1.0f; // Default availability

        score.CalculateTotalScore();
        scores.push_back(score);
    }

    // Sort by total score (highest first)
    std::sort(scores.begin(), scores.end(),
              [](const RoleScore& a, const RoleScore& b) {
                  return a.totalScore > b.totalScore;
              });

    return scores;
}

GroupRole RoleAssignment::RecommendRole( Group* group)
{
    if (!_bot)
        return GroupRole::NONE;

    std::vector<RoleScore> scores = CalculateRoleScores(group);

    if (!scores.empty())
        return scores[0].role;

    return GroupRole::NONE;
}

GroupComposition RoleAssignment::AnalyzeGroupComposition(Group* group)
{
    GroupComposition composition;

    if (!group)
        return composition;

    // Count assigned roles
    for (GroupReference const& itr : group->GetMembers())
    {
        if (Player* member = itr.GetSource())
        {
            uint32 memberGuid = member->GetGUID().GetCounter();
            auto profileIt = _playerProfiles.find(memberGuid);

            if (profileIt != _playerProfiles.end())
            {
                GroupRole assignedRole = profileIt->second.assignedRole;
                composition.roleAssignments[assignedRole].push_back(memberGuid);
                composition.roleFulfillment[assignedRole]++;
            }

            composition.totalMembers++;
        }
    }

    // Check for main tank and healer
    composition.hasMainTank = composition.roleFulfillment[GroupRole::TANK] > 0;
    composition.hasMainHealer = composition.roleFulfillment[GroupRole::HEALER] > 0;
    composition.dpsCount = composition.roleFulfillment[GroupRole::MELEE_DPS] +
                          composition.roleFulfillment[GroupRole::RANGED_DPS];

    // Calculate composition score
    composition.compositionScore = CalculateCompositionScore(composition);
    composition.isValid = IsCompositionViable(composition);

    return composition;
}

bool RoleAssignment::IsCompositionViable(const GroupComposition& composition)
{
    // DESIGN NOTE: Basic composition viability checks
    // Current behavior: Only checks for presence of tank and healer
    // Full implementation should:
    // - Validate minimum DPS count for content type (dungeons need 3 DPS, raids vary)
    // - Check for buff coverage (Bloodlust, Battle Res, key utility abilities)
    // - Verify interrupt availability for dungeon content
    // - Consider composition score threshold for different content difficulties
    // - Account for special role requirements (e.g., tank swaps in raids)
    // Reference: _contentRequirements map for content-specific requirements
    if (!composition.hasMainTank || !composition.hasMainHealer)
        return false;

    if (composition.dpsCount == 0)
        return false;

    if (composition.totalMembers < 3 || composition.totalMembers > 25)
        return false;
    return composition.compositionScore >= COMPOSITION_SCORE_THRESHOLD;
}

std::vector<GroupRole> RoleAssignment::GetMissingRoles(Group* group)
{
    std::vector<GroupRole> missingRoles;

    GroupComposition composition = AnalyzeGroupComposition(group);

    for (const auto& [role, required] : composition.roleRequirements)
    {
        uint32 fulfilled = composition.roleFulfillment[role];
        if (fulfilled < required)
        {
            for (uint32 i = fulfilled; i < required; ++i)
            {
                missingRoles.push_back(role);
            }
        }
    }

    return missingRoles;
}

bool RoleAssignment::CanPlayerSwitchRole( GroupRole newRole, Group* /*group*/)
{
    if (!_bot)
        return false;

    auto profileIt = _playerProfiles.find(_bot->GetGUID().GetCounter());

    if (profileIt == _playerProfiles.end())
        return false;

    // Check role capability
    auto capabilityIt = profileIt->second.roleCapabilities.find(newRole);
    if (capabilityIt == profileIt->second.roleCapabilities.end())
        return false;

    RoleCapability capability = capabilityIt->second;
    if (capability == RoleCapability::INCAPABLE)
        return false;

    // Check if role switch is on cooldown
    uint32 lastUpdate = profileIt->second.lastRoleUpdate;
    if (GameTime::GetGameTimeMS() - lastUpdate < ROLE_SWITCH_COOLDOWN)
        return false;

    return true;
}

void RoleAssignment::Update(uint32 diff)
{
    static uint32 lastUpdate = 0;

    if (GameTime::GetGameTimeMS() - lastUpdate < PROFILE_UPDATE_INTERVAL)
        return;

    RefreshPlayerProfiles();
    CleanupInactiveProfiles();
    UpdateRoleStatistics();

    lastUpdate = GameTime::GetGameTimeMS();
}

void RoleAssignment::InitializeClassRoleMappings()
{
    // Death Knight
    _classSpecRoles[CLASS_DEATH_KNIGHT][0] = { // Blood
        {GroupRole::TANK, RoleCapability::PRIMARY},
        {GroupRole::MELEE_DPS, RoleCapability::SECONDARY}
    };
    _classSpecRoles[CLASS_DEATH_KNIGHT][1] = { // Frost
        {GroupRole::MELEE_DPS, RoleCapability::PRIMARY},
        {GroupRole::TANK, RoleCapability::EMERGENCY}
    };
    _classSpecRoles[CLASS_DEATH_KNIGHT][2] = { // Unholy
        {GroupRole::MELEE_DPS, RoleCapability::PRIMARY}
    };

    // Warrior
    _classSpecRoles[CLASS_WARRIOR][0] = { // Arms
        {GroupRole::MELEE_DPS, RoleCapability::PRIMARY},
        {GroupRole::TANK, RoleCapability::EMERGENCY}
    };
    _classSpecRoles[CLASS_WARRIOR][1] = { // Fury
        {GroupRole::MELEE_DPS, RoleCapability::PRIMARY}
    };
    _classSpecRoles[CLASS_WARRIOR][2] = { // Protection
        {GroupRole::TANK, RoleCapability::PRIMARY},
        {GroupRole::MELEE_DPS, RoleCapability::SECONDARY}
    };

    // Paladin
    _classSpecRoles[CLASS_PALADIN][0] = { // Holy
        {GroupRole::HEALER, RoleCapability::PRIMARY},
        {GroupRole::SUPPORT, RoleCapability::SECONDARY}
    };
    _classSpecRoles[CLASS_PALADIN][1] = { // Protection
        {GroupRole::TANK, RoleCapability::PRIMARY},
        {GroupRole::HEALER, RoleCapability::EMERGENCY}
    };
    _classSpecRoles[CLASS_PALADIN][2] = { // Retribution
        {GroupRole::MELEE_DPS, RoleCapability::PRIMARY},
        {GroupRole::SUPPORT, RoleCapability::SECONDARY}
    };

    // Hunter
    _classSpecRoles[CLASS_HUNTER][0] = { // Beast Mastery
        {GroupRole::RANGED_DPS, RoleCapability::PRIMARY},
        {GroupRole::UTILITY, RoleCapability::SECONDARY}
    };
    _classSpecRoles[CLASS_HUNTER][1] = { // Marksmanship
        {GroupRole::RANGED_DPS, RoleCapability::PRIMARY}
    };
    _classSpecRoles[CLASS_HUNTER][2] = { // Survival
        {GroupRole::MELEE_DPS, RoleCapability::PRIMARY},
        {GroupRole::UTILITY, RoleCapability::SECONDARY}
    };

    // Rogue
    _classSpecRoles[CLASS_ROGUE][0] = { // Assassination
        {GroupRole::MELEE_DPS, RoleCapability::PRIMARY},
        {GroupRole::UTILITY, RoleCapability::SECONDARY}
    };
    _classSpecRoles[CLASS_ROGUE][1] = { // Combat
        {GroupRole::MELEE_DPS, RoleCapability::PRIMARY}
    };
    _classSpecRoles[CLASS_ROGUE][2] = { // Subtlety
        {GroupRole::MELEE_DPS, RoleCapability::PRIMARY},
        {GroupRole::UTILITY, RoleCapability::SECONDARY}
    };

    // Priest
    _classSpecRoles[CLASS_PRIEST][0] = { // Discipline
        {GroupRole::HEALER, RoleCapability::PRIMARY},
        {GroupRole::SUPPORT, RoleCapability::SECONDARY}
    };
    _classSpecRoles[CLASS_PRIEST][1] = { // Holy
        {GroupRole::HEALER, RoleCapability::PRIMARY}
    };
    _classSpecRoles[CLASS_PRIEST][2] = { // Shadow
        {GroupRole::RANGED_DPS, RoleCapability::PRIMARY},
        {GroupRole::UTILITY, RoleCapability::SECONDARY}
    };

    // Shaman
    _classSpecRoles[CLASS_SHAMAN][0] = { // Elemental
        {GroupRole::RANGED_DPS, RoleCapability::PRIMARY},
        {GroupRole::UTILITY, RoleCapability::SECONDARY}
    };
    _classSpecRoles[CLASS_SHAMAN][1] = { // Enhancement
        {GroupRole::MELEE_DPS, RoleCapability::PRIMARY},
        {GroupRole::UTILITY, RoleCapability::SECONDARY}
    };
    _classSpecRoles[CLASS_SHAMAN][2] = { // Restoration
        {GroupRole::HEALER, RoleCapability::PRIMARY},
        {GroupRole::SUPPORT, RoleCapability::SECONDARY}
    };

    // Mage
    _classSpecRoles[CLASS_MAGE][0] = { // Arcane
        {GroupRole::RANGED_DPS, RoleCapability::PRIMARY},
        {GroupRole::UTILITY, RoleCapability::SECONDARY}
    };
    _classSpecRoles[CLASS_MAGE][1] = { // Fire
        {GroupRole::RANGED_DPS, RoleCapability::PRIMARY}
    };
    _classSpecRoles[CLASS_MAGE][2] = { // Frost
        {GroupRole::RANGED_DPS, RoleCapability::PRIMARY},
        {GroupRole::UTILITY, RoleCapability::SECONDARY}
    };

    // Warlock
    _classSpecRoles[CLASS_WARLOCK][0] = { // Affliction
        {GroupRole::RANGED_DPS, RoleCapability::PRIMARY},
        {GroupRole::UTILITY, RoleCapability::SECONDARY}
    };
    _classSpecRoles[CLASS_WARLOCK][1] = { // Demonology
        {GroupRole::RANGED_DPS, RoleCapability::PRIMARY},
        {GroupRole::SUPPORT, RoleCapability::SECONDARY}
    };
    _classSpecRoles[CLASS_WARLOCK][2] = { // Destruction
        {GroupRole::RANGED_DPS, RoleCapability::PRIMARY}
    };

    // Monk
    _classSpecRoles[CLASS_MONK][0] = { // Brewmaster
        {GroupRole::TANK, RoleCapability::PRIMARY},
        {GroupRole::MELEE_DPS, RoleCapability::SECONDARY}
    };
    _classSpecRoles[CLASS_MONK][1] = { // Mistweaver
        {GroupRole::HEALER, RoleCapability::PRIMARY},
        {GroupRole::SUPPORT, RoleCapability::SECONDARY}
    };
    _classSpecRoles[CLASS_MONK][2] = { // Windwalker
        {GroupRole::MELEE_DPS, RoleCapability::PRIMARY},
        {GroupRole::UTILITY, RoleCapability::SECONDARY}
    };

    // Druid
    _classSpecRoles[CLASS_DRUID][0] = { // Balance
        {GroupRole::RANGED_DPS, RoleCapability::PRIMARY},
        {GroupRole::UTILITY, RoleCapability::SECONDARY}
    };
    _classSpecRoles[CLASS_DRUID][1] = { // Feral
        {GroupRole::MELEE_DPS, RoleCapability::PRIMARY},
        {GroupRole::TANK, RoleCapability::HYBRID}
    };
    _classSpecRoles[CLASS_DRUID][2] = { // Guardian
        {GroupRole::TANK, RoleCapability::PRIMARY},
        {GroupRole::MELEE_DPS, RoleCapability::SECONDARY}
    };
    _classSpecRoles[CLASS_DRUID][3] = { // Restoration
        {GroupRole::HEALER, RoleCapability::PRIMARY},
        {GroupRole::SUPPORT, RoleCapability::SECONDARY}
    };

    // Demon Hunter
    _classSpecRoles[CLASS_DEMON_HUNTER][0] = { // Havoc
        {GroupRole::MELEE_DPS, RoleCapability::PRIMARY}
    };
    _classSpecRoles[CLASS_DEMON_HUNTER][1] = { // Vengeance
        {GroupRole::TANK, RoleCapability::PRIMARY},
        {GroupRole::MELEE_DPS, RoleCapability::SECONDARY}
    };

    // Evoker
    _classSpecRoles[CLASS_EVOKER][0] = { // Devastation
        {GroupRole::RANGED_DPS, RoleCapability::PRIMARY},
        {GroupRole::UTILITY, RoleCapability::SECONDARY}
    };
    _classSpecRoles[CLASS_EVOKER][1] = { // Preservation
        {GroupRole::HEALER, RoleCapability::PRIMARY},
        {GroupRole::SUPPORT, RoleCapability::SECONDARY}
    };
    _classSpecRoles[CLASS_EVOKER][2] = { // Augmentation
        {GroupRole::SUPPORT, RoleCapability::PRIMARY},
        {GroupRole::RANGED_DPS, RoleCapability::SECONDARY}
    };

    TC_LOG_INFO("playerbot", "RoleAssignment: Initialized class role mappings for all 13 classes");
}

void RoleAssignment::BuildPlayerProfile(PlayerRoleProfile& profile)
{
    if (!_bot)
        return;

    profile.playerClass = _bot->GetClass();
    profile.playerLevel = _bot->GetLevel();
    profile.lastRoleUpdate = GameTime::GetGameTimeMS();

    // Set default preferences based on class
    profile.preferredRole = DetermineOptimalRole(nullptr, RoleAssignmentStrategy::OPTIMAL_ASSIGNMENT);
    profile.isFlexible = true;
    profile.overallRating = 5.0f;

    // Initialize alternative roles
    std::vector<RoleScore> scores = CalculateRoleScores(nullptr);
    for (size_t i = 1; i < std::min(scores.size(), size_t(3)); ++i)
    {
        if (scores[i].totalScore >= MIN_ROLE_EFFECTIVENESS)
            profile.alternativeRoles.push_back(scores[i].role);
    }
}

void RoleAssignment::CalculateRoleCapabilities(PlayerRoleProfile& profile)
{
    if (!_bot)
        return;

    uint8 playerClass = _bot->GetClass();

    // Get player's spec index (0-3) using the centralized utility
    // This queries ChrSpecializationEntry::OrderIndex for the correct mapping
    uint8 playerSpecIndex = GetPlayerSpecIndex(_bot);

    // Store the spec index in the profile
    profile.playerSpec = playerSpecIndex;

    auto classIt = _classSpecRoles.find(playerClass);
    if (classIt != _classSpecRoles.end())
    {
        auto specIt = classIt->second.find(playerSpecIndex);
        if (specIt != classIt->second.end())
        {
            for (const auto& [role, capability] : specIt->second)
            {
                profile.roleCapabilities[role] = capability;
            }
        }
        else
        {
            // Spec index not found in our mapping - use the centralized role detection
            // This handles cases where our static mapping is out of date
            GroupRole detectedRole = GetPlayerSpecRole(_bot);
            if (detectedRole != GroupRole::NONE)
            {
                profile.roleCapabilities[detectedRole] = RoleCapability::PRIMARY;
            }
        }
    }

    // Initialize all roles to incapable by default if not already set
    for (int roleInt = 0; roleInt <= static_cast<int>(GroupRole::UTILITY); ++roleInt)
    {
        GroupRole role = static_cast<GroupRole>(roleInt);
        if (profile.roleCapabilities.find(role) == profile.roleCapabilities.end())
        {
            profile.roleCapabilities[role] = RoleCapability::INCAPABLE;
        }
    }
}

float RoleAssignment::CalculateClassRoleEffectiveness(uint8 playerClass, uint8 playerSpec, GroupRole role)
{
    auto classIt = _classSpecRoles.find(playerClass);
    if (classIt == _classSpecRoles.end())
        return 0.0f;

    auto specIt = classIt->second.find(playerSpec);
    if (specIt == classIt->second.end())
        return 0.0f;

    for (const auto& [classRole, capability] : specIt->second)
    {
        if (classRole == role)
        {
            switch (capability)
            {
                case RoleCapability::PRIMARY:
                    return 1.0f;
                case RoleCapability::SECONDARY:
                    return 0.8f;
                case RoleCapability::HYBRID:
                    return 0.9f;
                case RoleCapability::EMERGENCY:
                    return 0.5f;
                default:
                    return 0.0f;
            }
        }
    }
    return 0.0f;
}

float RoleAssignment::CalculateGearScore(GroupRole role)
{
    if (!_bot)
        return 0.0f;

    uint32 itemCount = 0;
    float totalItemLevel = 0.0f;

    // Analyze equipped items - using item level based scoring
    // TrinityCore 11.x doesn't expose ItemStat array directly
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        Item* item = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item)
            continue;

        itemCount++;
        totalItemLevel += static_cast<float>(item->GetItemLevel(_bot));
    }

    if (itemCount == 0)
        return 0.0f;

    // Normalize score to 0.0 - 1.0 range based on item level
    float averageItemLevel = totalItemLevel / static_cast<float>(itemCount);

    // Normalize around TWW item levels (roughly 400-600 range)
    float itemLevelScore = std::min(1.0f, averageItemLevel / 500.0f);

    // Role-based adjustment using player stats
    float roleModifier = 1.0f;

    switch (role)
    {
        case GroupRole::TANK:
            // Tanks benefit from high stamina
            roleModifier = 1.0f + (static_cast<float>(_bot->GetStat(STAT_STAMINA)) / 50000.0f);
            break;

        case GroupRole::HEALER:
            // Healers benefit from intellect and spirit (WoW 12.0: Spirit re-added)
            roleModifier = 1.0f + (static_cast<float>(_bot->GetStat(STAT_INTELLECT)) / 30000.0f)
                               + (static_cast<float>(_bot->GetStat(STAT_SPIRIT)) / 50000.0f);
            break;

        case GroupRole::MELEE_DPS:
            // Melee DPS benefits from strength/agility
            roleModifier = 1.0f + (static_cast<float>(std::max(_bot->GetStat(STAT_STRENGTH), _bot->GetStat(STAT_AGILITY))) / 30000.0f);
            break;

        case GroupRole::RANGED_DPS:
            // Ranged DPS benefits from intellect/agility
            roleModifier = 1.0f + (static_cast<float>(std::max(_bot->GetStat(STAT_INTELLECT), _bot->GetStat(STAT_AGILITY))) / 30000.0f);
            break;

        default:
            break;
    }

    // Cap role modifier at 1.5x
    roleModifier = std::min(1.5f, roleModifier);

    float finalScore = itemLevelScore * roleModifier;
    finalScore = std::min(1.0f, finalScore); // Cap at 1.0

    TC_LOG_DEBUG("playerbot", "RoleAssignment: Gear score for {} in role {}: {:.2f} (iLvl: {:.1f})",
                 _bot->GetName(), static_cast<uint8>(role), finalScore, averageItemLevel);

    return finalScore;
}

float RoleAssignment::CalculateExperienceScore(GroupRole role)
{
    auto playerIt = _rolePerformance.find(_bot->GetGUID().GetCounter());
    if (playerIt == _rolePerformance.end())
        return 0.5f; // Default experience

    auto roleIt = playerIt->second.find(role);
    if (roleIt == playerIt->second.end())
        return 0.5f; // No experience in this role

    return roleIt->second.averageEffectiveness.load();
}

float RoleAssignment::CalculateSynergyScore( GroupRole role, Group* group)
{
    if (!_bot || !group)
        return 0.5f;

    float synergyScore = 0.5f; // Base score
    uint8 playerClass = _bot->GetClass();

    // Get group composition
    std::unordered_map<uint8, uint32> classCounts; // class -> count
    std::unordered_map<GroupRole, uint32> roleCounts; // role -> count

    for (GroupReference const& itr : group->GetMembers())
    {
        if (Player* member = itr.GetSource())
        {
            if (member->GetGUID() == _bot->GetGUID())
                continue; // Skip the player we're evaluating

            uint8 memberClass = member->GetClass();
            classCounts[memberClass]++;

            // Use centralized role detection utilities from GroupRoleEnums.h
            // This queries ChrSpecializationEntry to get authoritative role data
            GroupRole memberRole = GetPlayerSpecRole(member);

            roleCounts[memberRole]++;
        }
    }

    // Calculate class-specific synergies
    switch (playerClass)
    {
        case CLASS_WARRIOR:
            // Warriors synergize with healers (more risk taking)
            if (roleCounts[GroupRole::HEALER] > 0)
                synergyScore += 0.1f;
            // Battle Shout benefits physical DPS
            if (roleCounts[GroupRole::MELEE_DPS] > 1)
                synergyScore += 0.15f;
            break;

        case CLASS_PALADIN:
            // Paladins provide great utility and buffs
            synergyScore += 0.1f; // Always valuable
            // Blessings benefit all roles
            if (group->GetMembersCount() >= 4)
                synergyScore += 0.1f;
            // Auras benefit everyone
            if (roleCounts[GroupRole::MELEE_DPS] > 1)
                synergyScore += 0.1f;
            break;

        case CLASS_PRIEST:
            // Priests provide Power Word: Fortitude (stamina buff)
            synergyScore += 0.15f;
            // Shadow priests benefit from having other casters
            if (role == GroupRole::RANGED_DPS && roleCounts[GroupRole::RANGED_DPS] > 0)
                synergyScore += 0.2f;
            break;

        case CLASS_MAGE:
            // Mages provide Arcane Intellect (intellect buff)
            synergyScore += 0.15f;
            // Benefits from having tank for aggro control
            if (roleCounts[GroupRole::TANK] > 0)
                synergyScore += 0.1f;
            // Food/water utility always valuable
            synergyScore += 0.05f;
            break;

        case CLASS_WARLOCK:
            // Warlocks provide healthstones and soulstones
            synergyScore += 0.1f;
            // Summons provide utility
            synergyScore += 0.05f;
            // Benefits from having tank
            if (roleCounts[GroupRole::TANK] > 0)
                synergyScore += 0.1f;
            break;

        case CLASS_DRUID:
            // Druids are extremely flexible and synergize with everything
            synergyScore += 0.15f;
            // Mark of the Wild benefits all
            if (group->GetMembersCount() >= 4)
                synergyScore += 0.1f;
            // Battle res is invaluable
            synergyScore += 0.1f;
            break;

        case CLASS_SHAMAN:
            // Totems benefit the entire group
            synergyScore += 0.2f;
            // Bloodlust/Heroism is extremely valuable
            synergyScore += 0.15f;
            // Benefits melee groups especially
            if (roleCounts[GroupRole::MELEE_DPS] > 1)
                synergyScore += 0.1f;
            break;

        case CLASS_ROGUE:
            // Rogues benefit from having a tank
            if (roleCounts[GroupRole::TANK] > 0)
                synergyScore += 0.15f;
            // Tricks of the Trade benefits tanks
            if (role == GroupRole::MELEE_DPS && roleCounts[GroupRole::TANK] > 0)
                synergyScore += 0.1f;
            break;

        case CLASS_HUNTER:
            // Hunters are self-sufficient
            synergyScore += 0.05f;
            // Aspect of the Pack helps with travel
            synergyScore += 0.05f;
            // Misdirection benefits tanks
            if (roleCounts[GroupRole::TANK] > 0)
                synergyScore += 0.1f;
            break;

        case CLASS_DEATH_KNIGHT:
            // Death Grip utility
            synergyScore += 0.1f;
            // Battle res
            synergyScore += 0.1f;
            // Horn of Winter benefits physical DPS
            if (roleCounts[GroupRole::MELEE_DPS] > 1)
                synergyScore += 0.1f;
            break;

        case CLASS_MONK:
            // Monks are versatile
            synergyScore += 0.1f;
            // Legacy of the White Tiger benefits all
            if (group->GetMembersCount() >= 4)
                synergyScore += 0.1f;
            break;

        case CLASS_DEMON_HUNTER:
            // Chaos Brand debuff benefits all damage dealers
            if (roleCounts[GroupRole::MELEE_DPS] + roleCounts[GroupRole::RANGED_DPS] > 2)
                synergyScore += 0.2f;
            break;

        case CLASS_EVOKER:
            // Evokers provide unique buffs
            synergyScore += 0.15f;
            // Blessing of the Bronze is valuable
            if (group->GetMembersCount() >= 4)
                synergyScore += 0.1f;
            break;

        default:
            break;
    }

    // Role-specific synergies
    switch (role)
    {
        case GroupRole::TANK:
            // Tanks synergize with healers
            if (roleCounts[GroupRole::HEALER] > 0)
                synergyScore += 0.2f;
            // Multiple tanks have anti-synergy unless needed
            if (roleCounts[GroupRole::TANK] >= 2)
                synergyScore -= 0.3f;
            break;

        case GroupRole::HEALER:
            // Healers synergize with tanks
            if (roleCounts[GroupRole::TANK] > 0)
                synergyScore += 0.2f;
            // Multiple healers can be redundant in 5-man
            if (roleCounts[GroupRole::HEALER] >= 2 && group->GetMembersCount() < 10)
                synergyScore -= 0.2f;
            break;

        case GroupRole::MELEE_DPS:
            // Melee DPS benefit from other melee (cleave synergy)
            if (roleCounts[GroupRole::MELEE_DPS] > 1)
                synergyScore += 0.1f;
            // Too many melee can be problematic
            if (roleCounts[GroupRole::MELEE_DPS] >= 4)
                synergyScore -= 0.2f;
            break;

        case GroupRole::RANGED_DPS:
            // Ranged DPS are safe and flexible
            synergyScore += 0.1f;
            // Balance with melee is good
            if (roleCounts[GroupRole::MELEE_DPS] > 0 && roleCounts[GroupRole::RANGED_DPS] > 0)
                synergyScore += 0.1f;
            break;

        default:
            break;
    }

    // Cap synergy score between 0.0 and 1.0
    synergyScore = std::max(0.0f, std::min(1.0f, synergyScore));

    TC_LOG_DEBUG("playerbot", "RoleAssignment: Synergy score for player {} (class {}) in role {}: {:.2f}",
                 _bot->GetName(), playerClass, static_cast<uint8>(role), synergyScore);

    return synergyScore;
}

GroupRole RoleAssignment::DetermineOptimalRole( Group* group, RoleAssignmentStrategy strategy)
{
    if (!_bot)
        return GroupRole::NONE;

    std::vector<RoleScore> scores = CalculateRoleScores(group);

    if (scores.empty())
        return GroupRole::NONE;

    // Apply strategy-specific filtering
    switch (strategy)
    {
        case RoleAssignmentStrategy::STRICT_PRIMARY_ONLY:
            // Only consider primary roles
            for (const auto& score : scores)
            {
                auto profileIt = _playerProfiles.find(_bot->GetGUID().GetCounter());
                if (profileIt != _playerProfiles.end())
                {
                    auto capIt = profileIt->second.roleCapabilities.find(score.role);
                    if (capIt != profileIt->second.roleCapabilities.end() &&
                        capIt->second == RoleCapability::PRIMARY)
                    {
                        return score.role;
                    }
                }
            }
            break;

        default:
            // Return highest scoring role
            return scores[0].role;
    }

    return GroupRole::NONE;
}

float RoleAssignment::CalculateCompositionScore(const GroupComposition& composition)
{
    float score = 5.0f; // Base score
    // Tank requirements
    if (composition.hasMainTank)
        score += 2.0f;
    else
        score -= 3.0f;

    // Healer requirements
    if (composition.hasMainHealer)
        score += 2.0f;
    else
        score -= 3.0f;

    // DPS balance
    if (composition.dpsCount >= 2)
        score += 1.0f;
    else if (composition.dpsCount == 1)
        score += 0.5f;
    else
        score -= 2.0f;

    // Group size
    if (composition.totalMembers >= 3 && composition.totalMembers <= 5)
        score += 1.0f;
    else if (composition.totalMembers > 5)
        score -= 0.5f;

    return std::max(0.0f, std::min(10.0f, score));
}

void RoleAssignment::ExecuteOptimalStrategy(Group* group)
{
    if (!group)
        return;

    // Get all members and their optimal roles
    std::vector<std::pair<Player*, GroupRole>> assignments;

    for (GroupReference const& itr : group->GetMembers())
    {
        if (Player* member = itr.GetSource())
        {
            // Use RecommendRole for each member (we only handle our bot's assignment)
            GroupRole optimalRole = RecommendRole(group);
            assignments.emplace_back(member, optimalRole);
        }
    }

    // Sort by role priority (tank > healer > dps)
    std::sort(assignments.begin(), assignments.end(),
              [](const auto& a, const auto& b) {
                  return static_cast<int>(a.second) < static_cast<int>(b.second);
              });

    // Assign roles - each bot assigns itself
    for (const auto& [member, role] : assignments)
    {
        if (member == _bot)
        {
            AssignRole(role, group);
        }
    }
}

// DESIGN NOTE: Role assignment strategy implementations
// Currently use ExecuteOptimalStrategy as fallback behavior
// Full implementations should provide strategy-specific role assignment logic:

void RoleAssignment::ExecuteBalancedStrategy(Group* group)
{
    // DESIGN NOTE: Balanced role distribution strategy
    // Should balance role distribution evenly across all members
    // - Prioritize even distribution of tanks, healers, DPS
    // - Consider player preferences but maintain balance
    // - Optimize for sustained group performance
    // Reference: GroupComposition analysis
    ExecuteOptimalStrategy(group);
}

void RoleAssignment::ExecuteFlexibleStrategy(Group* group)
{
    // DESIGN NOTE: Flexible role switching strategy
    // Should allow dynamic role changes based on situation
    // - Enable hybrid classes to switch roles mid-content
    // - Respond to changing group needs in real-time
    // - Prioritize adaptability over specialization
    // Reference: RoleCapability::HYBRID handling
    ExecuteOptimalStrategy(group);
}

void RoleAssignment::ExecuteStrictStrategy(Group* group)
{
    // DESIGN NOTE: Strict primary role enforcement strategy
    // Should enforce primary role assignments only
    // - Only assign roles where RoleCapability::PRIMARY exists
    // - Reject assignments outside primary specialization
    // - Maximize individual role performance
    // Reference: _classSpecRoles primary mappings
    ExecuteOptimalStrategy(group);
}

void RoleAssignment::ExecuteHybridStrategy(Group* group)
{
    // DESIGN NOTE: Hybrid class optimization strategy
    // Should maximize value of hybrid classes (Druid, Paladin, Shaman, Monk)
    // - Leverage multi-role capabilities of hybrid classes
    // - Apply HYBRID_CLASS_BONUS to role scoring
    // - Enable seamless role transitions
    // Reference: CalculateFlexibilityBonus(), hybrid class detection
    ExecuteOptimalStrategy(group);
}

void RoleAssignment::ExecuteDungeonStrategy(Group* group)
{
    // DESIGN NOTE: 5-man dungeon optimized strategy
    // Should optimize for dungeon content (1 tank, 1 healer, 3 DPS)
    // - Enforce standard dungeon composition
    // - Prioritize interrupt capabilities and crowd control
    // - Balance melee/ranged DPS for mechanic handling
    // Reference: _contentRequirements dungeon templates
    ExecuteOptimalStrategy(group);
}

void RoleAssignment::ExecuteRaidStrategy(Group* group)
{
    // DESIGN NOTE: Raid content optimized strategy
    // Should optimize for raid content (2-3 tanks, 5-6 healers, rest DPS)
    // - Scale role requirements based on raid size (10/25/30)
    // - Ensure buff coverage and utility distribution
    // - Balance healing assignments across raid groups
    // Reference: _contentRequirements raid templates
    ExecuteOptimalStrategy(group);
}

void RoleAssignment::ExecutePvPStrategy(Group* group)
{
    // DESIGN NOTE: PvP/Battleground optimized strategy
    // Should optimize for PvP content
    // - Balance between offense and defense roles
    // - Prioritize mobility and survivability
    // - Ensure flag carriers, healers, and damage dealers
    // - Consider PvP-specific utility (stealth, crowd control)
    // Reference: Battleground role requirements
    ExecuteOptimalStrategy(group);
}

void RoleAssignment::AnalyzePlayerGear(PlayerRoleProfile& profile)
{
    if (!_bot)
        return;

    // Analyze gear for each possible role
    std::vector<GroupRole> roles = {
        GroupRole::TANK,
        GroupRole::HEALER,
        GroupRole::MELEE_DPS,
        GroupRole::RANGED_DPS,
        GroupRole::SUPPORT
    };

    for (GroupRole role : roles)
    {
        // Calculate gear score for this role
        float gearScore = CalculateGearScore(role);

        // Update role score with gear information
        auto& roleScore = profile.roleScores[role];
        roleScore.gearScore = gearScore;

        TC_LOG_DEBUG("playerbot", "RoleAssignment: Player {} gear score for role {}: {:.2f}",
                     _bot->GetName(), static_cast<uint8>(role), gearScore);
    }

    // Analyze overall gear quality using item level
    uint32 totalItemLevel = 0;
    uint32 itemCount = 0;

    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        Item* item = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item)
            continue;

        itemCount++;
        totalItemLevel += item->GetItemLevel(_bot);
    }

    if (itemCount > 0)
    {
        float averageItemLevel = static_cast<float>(totalItemLevel) / static_cast<float>(itemCount);
        profile.overallRating = averageItemLevel / 50.0f; // Normalize to ~0-10 scale for TWW

        TC_LOG_DEBUG("playerbot", "RoleAssignment: Player {} average item level: {:.1f}, overall rating: {:.1f}",
                     _bot->GetName(), averageItemLevel, profile.overallRating);
    }

    // Determine gear type based on primary stat (using player stats, not item stats)
    // High Stamina indicates tanking potential
    bool hasTankGear = _bot->GetStat(STAT_STAMINA) > (_bot->GetLevel() * 50);
    // High Intellect with mana indicates healing potential
    bool hasHealerGear = _bot->GetPowerType() == POWER_MANA && _bot->GetStat(STAT_INTELLECT) > (_bot->GetLevel() * 30);
    // High primary DPS stat indicates damage potential
    bool hasDPSGear = std::max({_bot->GetStat(STAT_STRENGTH), _bot->GetStat(STAT_AGILITY), _bot->GetStat(STAT_INTELLECT)}) > (_bot->GetLevel() * 20);

    // Update role capabilities based on gear
    if (hasTankGear)
    {
        if (profile.roleCapabilities[GroupRole::TANK] == RoleCapability::INCAPABLE)
            profile.roleCapabilities[GroupRole::TANK] = RoleCapability::EMERGENCY;
    }

    if (hasHealerGear)
    {
        if (profile.roleCapabilities[GroupRole::HEALER] == RoleCapability::INCAPABLE)
            profile.roleCapabilities[GroupRole::HEALER] = RoleCapability::EMERGENCY;
    }

    if (hasDPSGear)
    {
        if (profile.roleCapabilities[GroupRole::MELEE_DPS] == RoleCapability::INCAPABLE)
            profile.roleCapabilities[GroupRole::MELEE_DPS] = RoleCapability::EMERGENCY;
        if (profile.roleCapabilities[GroupRole::RANGED_DPS] == RoleCapability::INCAPABLE)
            profile.roleCapabilities[GroupRole::RANGED_DPS] = RoleCapability::EMERGENCY;
    }

    profile.lastRoleUpdate = GameTime::GetGameTimeMS();
}

void RoleAssignment::UpdateRoleExperience(PlayerRoleProfile& profile)
{
    if (!_bot)
        return;

    // Get performance data
    auto playerIt = _rolePerformance.find(_bot->GetGUID().GetCounter());
    if (playerIt == _rolePerformance.end())
        return; // No performance data yet

    // Update experience scores for each role based on performance history
    for (auto& rolePair : playerIt->second)
    {
        GroupRole role = rolePair.first;
        auto& performance = rolePair.second;

        // Calculate experience score based on:
        // 1. Number of encounters (more experience = higher score)
        // 2. Average effectiveness (better performance = higher score)
        // 3. Recent performance trend (improving = bonus)

        uint32 successCount = performance.successfulEncounters.load();
        uint32 failCount = performance.failedEncounters.load();
        uint32 encounterCount = successCount + failCount; // Total encounters from success + fail
        float avgEffectiveness = performance.averageEffectiveness.load();

        // Base experience from encounter count (logarithmic growth)
        float experienceBase = std::min(1.0f, std::log10(static_cast<float>(encounterCount) + 1.0f) / 2.0f);

        // Performance modifier (50% weight)
        float performanceMod = avgEffectiveness * 0.5f;

        // Success rate modifier (20% weight)
        float successRate = (encounterCount > 0) ?
                            static_cast<float>(successCount) / encounterCount : 0.5f;
        float successMod = successRate * 0.2f;

        // Consistency bonus (10% weight) - reward players with stable performance
        float consistencyMod = (avgEffectiveness > 0.6f) ? 0.1f : 0.0f;

        // Calculate final experience score
        float experienceScore = experienceBase + performanceMod + successMod + consistencyMod;
        experienceScore = std::max(0.0f, std::min(1.0f, experienceScore)); // Clamp to [0, 1]

        // Update role score
        auto& roleScore = profile.roleScores[role];
        roleScore.experienceScore = experienceScore;

        // Update effectiveness
        roleScore.effectiveness = avgEffectiveness;

        TC_LOG_DEBUG("playerbot", "RoleAssignment: Player {} experience for role {}: {:.2f} (encounters: {}, effectiveness: {:.2f}, success rate: {:.2f})",
                     _bot->GetName(), static_cast<uint8>(role), experienceScore,
                     encounterCount, avgEffectiveness, successRate);

        // Update role capability based on experience
        if (experienceScore >= 0.8f && avgEffectiveness >= 0.7f)
        {
            // Very experienced and effective - upgrade to PRIMARY if currently SECONDARY
            if (profile.roleCapabilities[role] == RoleCapability::SECONDARY)
                profile.roleCapabilities[role] = RoleCapability::PRIMARY;
        }
        else if (experienceScore >= 0.5f && avgEffectiveness >= 0.5f)
        {
            // Moderately experienced - upgrade to SECONDARY if currently EMERGENCY
            if (profile.roleCapabilities[role] == RoleCapability::EMERGENCY)
                profile.roleCapabilities[role] = RoleCapability::SECONDARY;
        }
        else if (experienceScore < 0.2f && avgEffectiveness < 0.3f && encounterCount >= 5)
        {
            // Poor experience after many encounters - downgrade
            if (profile.roleCapabilities[role] == RoleCapability::SECONDARY)
                profile.roleCapabilities[role] = RoleCapability::EMERGENCY;
        }
    }

    // Update alternative roles based on experience
    profile.alternativeRoles.clear();
    for (auto& roleScorePair : profile.roleScores)
    {
        GroupRole role = roleScorePair.first;
        const RoleScore& score = roleScorePair.second;

        // Add to alternatives if capable and experienced enough
        if (role != profile.preferredRole &&
            profile.roleCapabilities[role] != RoleCapability::INCAPABLE &&
            score.experienceScore >= 0.4f)
        {
            profile.alternativeRoles.push_back(role);
        }
    }

    // Sort alternative roles by experience score (descending)
    std::sort(profile.alternativeRoles.begin(), profile.alternativeRoles.end(),
              [&profile](GroupRole a, GroupRole b) {
                  return profile.roleScores[a].experienceScore > profile.roleScores[b].experienceScore;
              });

    profile.lastRoleUpdate = GameTime::GetGameTimeMS();

    TC_LOG_DEBUG("playerbot", "RoleAssignment: Player {} role experience updated, {} alternative roles available",
                 _bot->GetName(), profile.alternativeRoles.size());
}

bool RoleAssignment::ValidateRoleAssignment(Group* group)
{
    GroupComposition composition = AnalyzeGroupComposition(group);
    return IsCompositionViable(composition);
}

void RoleAssignment::NotifyRoleAssignment( GroupRole role, Group* /*group*/)
{
    if (!_bot)
        return;

    const char* roleNames[] = {
        "Tank", "Healer", "Melee DPS", "Ranged DPS", "Support", "Utility", "None"
    };

    TC_LOG_DEBUG("playerbot", "RoleAssignment: Player %s assigned role %s",
                 _bot->GetName().c_str(), roleNames[static_cast<int>(role)]);
}

void RoleAssignment::RefreshPlayerProfiles()
{
    // Update profiles for active players
    for (auto& [playerGuid, profile] : _playerProfiles)
    {
        if (Player* player = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(playerGuid)))
        {
            BuildPlayerProfile(profile);
        }
    }
}

void RoleAssignment::CleanupInactiveProfiles()
{
    uint32 currentTime = GameTime::GetGameTimeMS();
    const uint32 CLEANUP_THRESHOLD = 24 * 60 * 60 * 1000; // 24 hours

    auto it = _playerProfiles.begin();
    while (it != _playerProfiles.end())
    {
        if (currentTime - it->second.lastRoleUpdate > CLEANUP_THRESHOLD)
        {
            // Check if player is still online
            if (!ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(it->first)))
            {
                it = _playerProfiles.erase(it);
                continue;
            }
        }
        ++it;
    }
}

void RoleAssignment::UpdateRoleStatistics()
{
    _globalStatistics.lastStatsUpdate = std::chrono::steady_clock::now();
}

RoleStatistics RoleAssignment::GetGlobalRoleStatistics()
{
    return _globalStatistics;
}

// ============================================================================
// MISSING METHOD IMPLEMENTATIONS
// ============================================================================

void RoleAssignment::OptimizeRoleDistribution(Group* group)
{
    if (!group)
        return;

    // Re-analyze and optimize the role distribution
    AssignRoles(group, RoleAssignmentStrategy::OPTIMAL_ASSIGNMENT);
}

float RoleAssignment::CalculateRoleSynergy(GroupRole role, Group* group)
{
    return CalculateSynergyScore(role, group);
}

std::vector<uint32> RoleAssignment::FindPlayersForRole(GroupRole role, const std::vector<Player*>& candidates)
{
    std::vector<uint32> suitablePlayers;

    for (Player* candidate : candidates)
    {
        if (!candidate)
            continue;

        uint32 candidateGuid = candidate->GetGUID().GetCounter();

        // Check if player can perform this role
        auto profileIt = _playerProfiles.find(candidateGuid);
        if (profileIt != _playerProfiles.end())
        {
            auto capIt = profileIt->second.roleCapabilities.find(role);
            if (capIt != profileIt->second.roleCapabilities.end() &&
                capIt->second != RoleCapability::INCAPABLE)
            {
                suitablePlayers.push_back(candidateGuid);
            }
        }
        else
        {
            // Build profile for this player if not exists
            PlayerRoleProfile profile(candidateGuid, candidate->GetClass(), 0, candidate->GetLevel());
            CalculateRoleCapabilities(profile);
            _playerProfiles[candidateGuid] = profile;

            auto capIt = profile.roleCapabilities.find(role);
            if (capIt != profile.roleCapabilities.end() &&
                capIt->second != RoleCapability::INCAPABLE)
            {
                suitablePlayers.push_back(candidateGuid);
            }
        }
    }

    return suitablePlayers;
}

void RoleAssignment::HandleRoleConflict(Group* group, GroupRole conflictedRole)
{
    if (!group)
        return;

    _globalStatistics.roleConflicts.fetch_add(1);
    ResolveRoleConflict(group, conflictedRole);
}

void RoleAssignment::RebalanceRoles(Group* group)
{
    if (!group)
        return;

    // Re-analyze group composition and rebalance
    GroupComposition composition = AnalyzeGroupComposition(group);

    std::vector<GroupRole> missing = GetMissingRoles(group);
    for (GroupRole missingRole : missing)
    {
        // Find players who can fill the missing role
        std::vector<Player*> candidates;
        for (GroupReference const& itr : group->GetMembers())
        {
            if (Player* member = itr.GetSource())
            {
                candidates.push_back(member);
            }
        }

        std::vector<uint32> suitablePlayers = FindPlayersForRole(missingRole, candidates);
        if (!suitablePlayers.empty())
        {
            // Assign the first suitable player to the missing role
            auto profileIt = _playerProfiles.find(suitablePlayers[0]);
            if (profileIt != _playerProfiles.end())
            {
                profileIt->second.assignedRole = missingRole;
            }
        }
    }
}

void RoleAssignment::AdaptToGroupChanges(Group* group, Player* newMember, Player* leavingMember)
{
    if (!group)
        return;

    if (newMember)
    {
        // Add new member's profile
        uint32 newGuid = newMember->GetGUID().GetCounter();
        PlayerRoleProfile profile(newGuid, newMember->GetClass(), 0, newMember->GetLevel());
        BuildPlayerProfile(profile);
        CalculateRoleCapabilities(profile);
        _playerProfiles[newGuid] = profile;

        // Assign role to new member
        GroupRole recommendedRole = RecommendRole(group);
        profile.assignedRole = recommendedRole;
    }

    if (leavingMember)
    {
        // Remove leaving member's profile
        uint32 leavingGuid = leavingMember->GetGUID().GetCounter();
        _playerProfiles.erase(leavingGuid);
    }

    // Rebalance roles after change
    RebalanceRoles(group);
}

void RoleAssignment::OptimizeForDungeon(Group* group, uint32 dungeonId)
{
    if (!group)
        return;

    // Set dungeon-specific requirements (standard 5-man: 1 tank, 1 healer, 3 DPS)
    std::unordered_map<GroupRole, uint32> requirements;
    requirements[GroupRole::TANK] = 1;
    requirements[GroupRole::HEALER] = 1;
    requirements[GroupRole::MELEE_DPS] = 2;
    requirements[GroupRole::RANGED_DPS] = 1;

    _contentRequirements[dungeonId] = requirements;

    ExecuteDungeonStrategy(group);
}

void RoleAssignment::OptimizeForRaid(Group* group, uint32 raidId)
{
    if (!group)
        return;

    // Raid requirements (standard 25-man: 2-3 tanks, 5-6 healers, rest DPS)
    std::unordered_map<GroupRole, uint32> requirements;
    requirements[GroupRole::TANK] = 3;
    requirements[GroupRole::HEALER] = 6;
    requirements[GroupRole::MELEE_DPS] = 8;
    requirements[GroupRole::RANGED_DPS] = 8;

    _contentRequirements[raidId] = requirements;

    ExecuteRaidStrategy(group);
}

void RoleAssignment::OptimizeForPvP(Group* group, uint32 battlegroundId)
{
    if (!group)
        return;

    // PvP requirements (balanced team)
    std::unordered_map<GroupRole, uint32> requirements;
    requirements[GroupRole::TANK] = 1;
    requirements[GroupRole::HEALER] = 2;
    requirements[GroupRole::MELEE_DPS] = 2;
    requirements[GroupRole::RANGED_DPS] = 2;

    _contentRequirements[battlegroundId] = requirements;

    ExecutePvPStrategy(group);
}

void RoleAssignment::OptimizeForQuesting(Group* group, uint32 /*questId*/)
{
    if (!group)
        return;

    // Quest requirements (flexible)
    ExecuteFlexibleStrategy(group);
}

void RoleAssignment::SetPlayerRolePreference(GroupRole preferredRole)
{
    if (!_bot)
        return;

    uint32 botGuid = _bot->GetGUID().GetCounter();
    auto it = _playerProfiles.find(botGuid);
    if (it != _playerProfiles.end())
    {
        it->second.preferredRole = preferredRole;
    }
}

GroupRole RoleAssignment::GetPlayerRolePreference()
{
    if (!_bot)
        return GroupRole::NONE;

    uint32 botGuid = _bot->GetGUID().GetCounter();
    auto it = _playerProfiles.find(botGuid);
    if (it != _playerProfiles.end())
    {
        return it->second.preferredRole;
    }

    return GroupRole::NONE;
}

void RoleAssignment::SetRoleFlexibility(bool isFlexible)
{
    if (!_bot)
        return;

    uint32 botGuid = _bot->GetGUID().GetCounter();
    auto it = _playerProfiles.find(botGuid);
    if (it != _playerProfiles.end())
    {
        it->second.isFlexible = isFlexible;
    }
}

void RoleAssignment::AddRoleConstraint(GroupRole role, RoleCapability capability)
{
    if (!_bot)
        return;

    uint32 botGuid = _bot->GetGUID().GetCounter();
    auto it = _playerProfiles.find(botGuid);
    if (it != _playerProfiles.end())
    {
        it->second.roleCapabilities[role] = capability;
    }
}

RolePerformance RoleAssignment::GetPlayerRolePerformance(GroupRole role)
{
    if (!_bot)
        return RolePerformance();

    uint32 botGuid = _bot->GetGUID().GetCounter();
    auto playerIt = _rolePerformance.find(botGuid);
    if (playerIt == _rolePerformance.end())
        return RolePerformance();

    auto roleIt = playerIt->second.find(role);
    if (roleIt == playerIt->second.end())
        return RolePerformance();

    // Return copy of performance (atomic values need explicit load)
    RolePerformance result;
    result.assignmentsAccepted.store(roleIt->second.assignmentsAccepted.load());
    result.assignmentsDeclined.store(roleIt->second.assignmentsDeclined.load());
    result.performanceRating.store(roleIt->second.performanceRating.load());
    result.successfulEncounters.store(roleIt->second.successfulEncounters.load());
    result.failedEncounters.store(roleIt->second.failedEncounters.load());
    result.averageEffectiveness.store(roleIt->second.averageEffectiveness.load());
    result.lastPerformanceUpdate = roleIt->second.lastPerformanceUpdate;
    return result;
}

void RoleAssignment::UpdateRolePerformance(GroupRole role, bool wasSuccessful, float effectiveness)
{
    if (!_bot)
        return;

    uint32 botGuid = _bot->GetGUID().GetCounter();

    // Ensure performance tracking exists for this player and role
    auto& playerPerf = _rolePerformance[botGuid];
    auto& rolePerf = playerPerf[role];

    if (wasSuccessful)
        rolePerf.successfulEncounters.fetch_add(1);
    else
        rolePerf.failedEncounters.fetch_add(1);

    // Update average effectiveness using exponential moving average
    float currentAvg = rolePerf.averageEffectiveness.load();
    float newAvg = (currentAvg * 0.8f) + (effectiveness * 0.2f);
    rolePerf.averageEffectiveness.store(newAvg);

    rolePerf.lastPerformanceUpdate = std::chrono::steady_clock::now();
}

std::vector<std::string> RoleAssignment::GetRoleAssignmentIssues(Group* group)
{
    std::vector<std::string> issues;

    if (!group)
    {
        issues.push_back("No group provided");
        return issues;
    }

    GroupComposition composition = AnalyzeGroupComposition(group);

    if (!composition.hasMainTank)
        issues.push_back("Missing main tank");

    if (!composition.hasMainHealer)
        issues.push_back("Missing main healer");

    if (composition.dpsCount < 2)
        issues.push_back("Insufficient DPS (need at least 2)");

    if (composition.totalMembers < 3)
        issues.push_back("Group too small (minimum 3 members)");

    if (composition.compositionScore < COMPOSITION_SCORE_THRESHOLD)
        issues.push_back("Composition score below threshold");

    return issues;
}

bool RoleAssignment::CanGroupFunction(Group* group)
{
    if (!group)
        return false;

    GroupComposition composition = AnalyzeGroupComposition(group);

    // Minimum requirements: at least 1 tank and 1 healer
    return composition.hasMainTank && composition.hasMainHealer;
}

bool RoleAssignment::FillEmergencyRole(Group* group, GroupRole urgentRole)
{
    if (!group)
        return false;

    _globalStatistics.emergencyFills.fetch_add(1);

    // Find a player who can emergency fill the role
    std::vector<Player*> candidates;
    for (GroupReference const& itr : group->GetMembers())
    {
        if (Player* member = itr.GetSource())
        {
            candidates.push_back(member);
        }
    }

    std::vector<uint32> suitablePlayers = FindPlayersForRole(urgentRole, candidates);

    if (suitablePlayers.empty())
        return false;

    // Find player with emergency capability
    for (uint32 playerGuid : suitablePlayers)
    {
        auto profileIt = _playerProfiles.find(playerGuid);
        if (profileIt != _playerProfiles.end())
        {
            auto capIt = profileIt->second.roleCapabilities.find(urgentRole);
            if (capIt != profileIt->second.roleCapabilities.end() &&
                capIt->second == RoleCapability::EMERGENCY)
            {
                profileIt->second.assignedRole = urgentRole;
                TC_LOG_INFO("playerbot", "RoleAssignment: Emergency role %u filled by player %u",
                           static_cast<uint8>(urgentRole), playerGuid);
                return true;
            }
        }
    }

    // Fall back to first suitable player
    auto profileIt = _playerProfiles.find(suitablePlayers[0]);
    if (profileIt != _playerProfiles.end())
    {
        profileIt->second.assignedRole = urgentRole;
        return true;
    }

    return false;
}

std::vector<uint32> RoleAssignment::FindEmergencyReplacements(GroupRole role, uint32 minLevel, uint32 maxLevel)
{
    std::vector<uint32> replacements;

    for (const auto& [playerGuid, profile] : _playerProfiles)
    {
        // Check level requirements
        if (profile.playerLevel < minLevel || profile.playerLevel > maxLevel)
            continue;

        // Check if player can perform this role at emergency level
        auto capIt = profile.roleCapabilities.find(role);
        if (capIt != profile.roleCapabilities.end() &&
            capIt->second != RoleCapability::INCAPABLE)
        {
            replacements.push_back(playerGuid);
        }
    }

    return replacements;
}

void RoleAssignment::HandleRoleEmergency(Group* group, uint32 disconnectedPlayerGuid)
{
    if (!group)
        return;

    // Find what role the disconnected player had
    auto profileIt = _playerProfiles.find(disconnectedPlayerGuid);
    if (profileIt == _playerProfiles.end())
        return;

    GroupRole vacatedRole = profileIt->second.assignedRole;

    // Try to fill the emergency role
    FillEmergencyRole(group, vacatedRole);

    // Remove disconnected player's profile
    _playerProfiles.erase(profileIt);
}

void RoleAssignment::SetRoleAssignmentStrategy(Group* group, RoleAssignmentStrategy strategy)
{
    if (!group)
        return;

    uint32 groupId = group->GetGUID().GetCounter();
    _groupStrategies[groupId] = strategy;
}

void RoleAssignment::SetContentTypeRequirements(uint32 contentId, const std::unordered_map<GroupRole, uint32>& requirements)
{
    _contentRequirements[contentId] = requirements;
}

// ============================================================================
// PRIVATE HELPER METHOD IMPLEMENTATIONS
// ============================================================================

bool RoleAssignment::HasRoleConflict(Group* group, GroupRole role)
{
    if (!group)
        return false;

    GroupComposition composition = AnalyzeGroupComposition(group);

    auto requirementIt = composition.roleRequirements.find(role);
    auto fulfillmentIt = composition.roleFulfillment.find(role);

    if (requirementIt == composition.roleRequirements.end() ||
        fulfillmentIt == composition.roleFulfillment.end())
        return false;

    // Conflict exists if there are more players than needed for a role
    return fulfillmentIt->second > requirementIt->second;
}

void RoleAssignment::ResolveRoleConflict(Group* group, GroupRole role)
{
    if (!group)
        return;

    // Find players with this role and move excess to alternative roles
    std::vector<uint32> alternativePlayers = GetAlternativePlayers(role, group);

    for (uint32 playerGuid : alternativePlayers)
    {
        auto profileIt = _playerProfiles.find(playerGuid);
        if (profileIt != _playerProfiles.end())
        {
            // Find best alternative role
            if (!profileIt->second.alternativeRoles.empty())
            {
                GroupRole altRole = profileIt->second.alternativeRoles[0];
                profileIt->second.assignedRole = altRole;
            }
        }
    }
}

std::vector<uint32> RoleAssignment::GetAlternativePlayers(GroupRole role, Group* group)
{
    std::vector<uint32> alternativePlayers;

    if (!group)
        return alternativePlayers;

    for (const auto& [playerGuid, profile] : _playerProfiles)
    {
        if (profile.assignedRole == role && !profile.alternativeRoles.empty())
        {
            alternativePlayers.push_back(playerGuid);
        }
    }

    return alternativePlayers;
}

void RoleAssignment::HandleRoleAssignmentFailure(Group* group, const std::string& reason)
{
    TC_LOG_WARN("playerbot", "RoleAssignment: Assignment failed for group %u - %s",
                group ? group->GetGUID().GetCounter() : 0, reason.c_str());
}

float RoleAssignment::CalculateFlexibilityBonus(Group* group)
{
    if (!group)
        return 0.0f;

    float flexibleCount = 0.0f;
    float totalCount = 0.0f;

    for (const auto& [playerGuid, profile] : _playerProfiles)
    {
        totalCount += 1.0f;
        if (profile.isFlexible)
            flexibleCount += 1.0f;
    }

    if (totalCount == 0.0f)
        return 0.0f;

    return (flexibleCount / totalCount) * HYBRID_CLASS_BONUS;
}

} // namespace Playerbot