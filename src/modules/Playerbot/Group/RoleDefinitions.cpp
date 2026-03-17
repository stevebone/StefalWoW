/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "RoleDefinitions.h"
#include "Log.h"

namespace Playerbot
{

// Meyer's singleton accessors for DLL-safe static data
::std::unordered_map<uint8, RoleDefinitions::ClassData>& RoleDefinitions::GetClassDefinitions()
{
    static ::std::unordered_map<uint8, ClassData> classDefinitions;
    return classDefinitions;
}

bool& RoleDefinitions::GetInitialized()
{
    static bool initialized = false;
    return initialized;
}

// Get class data
const RoleDefinitions::ClassData& RoleDefinitions::GetClassData(uint8 classId)
{
    if (!GetInitialized())
        Initialize();

    auto it = GetClassDefinitions().find(classId);
    if (it != GetClassDefinitions().end())
        return it->second;

    // Return a default/empty ClassData if not found
    static ClassData emptyData(0, "Unknown", false, 0.0f);
    TC_LOG_ERROR("playerbot.roles", "RoleDefinitions::GetClassData: Class {} not found in definitions", classId);
    return emptyData;
}

// Get specialization data
const RoleDefinitions::SpecializationData& RoleDefinitions::GetSpecializationData(uint8 classId, uint8 specId)
{
    if (!GetInitialized())
        Initialize();

    auto classIt = GetClassDefinitions().find(classId);
    if (classIt != GetClassDefinitions().end())
    {
        for (auto const& spec : classIt->second.specializations)
        {
            if (spec.specId == specId)
                return spec;
        }
    }

    // Return a default/empty SpecializationData if not found
    static SpecializationData emptySpec(0, "Unknown", GroupRole::NONE, 0.0f);
    TC_LOG_ERROR("playerbot.roles", "RoleDefinitions::GetSpecializationData: Spec {} for class {} not found", specId, classId);
    return emptySpec;
}

// Get role capabilities for a spec
::std::vector<::std::pair<GroupRole, RoleCapability>> RoleDefinitions::GetRoleCapabilities(uint8 classId, uint8 specId)
{
    const SpecializationData& spec = GetSpecializationData(classId, specId);
    return spec.roleCapabilities;
}

// Get role effectiveness
float RoleDefinitions::GetRoleEffectiveness(uint8 classId, uint8 specId, GroupRole role)
{
    const SpecializationData& spec = GetSpecializationData(classId, specId);

    if (spec.primaryRole == role)
        return spec.baseEffectiveness;

    // Check role capabilities
    for (auto const& [capRole, capability] : spec.roleCapabilities)
    {
        if (capRole == role)
        {
            switch (capability)
            {
                case RoleCapability::PRIMARY:
                    return spec.baseEffectiveness;
                case RoleCapability::SECONDARY:
                    return spec.baseEffectiveness * 0.7f;
                case RoleCapability::HYBRID:
                    return spec.baseEffectiveness * 0.5f;
                case RoleCapability::EMERGENCY:
                    return spec.baseEffectiveness * 0.3f;
                default:
                    return 0.0f;
            }
        }
    }

    return 0.0f;
}

// Check if spec can perform role
bool RoleDefinitions::CanPerformRole(uint8 classId, uint8 specId, GroupRole role, RoleCapability minCapability)
{
    const SpecializationData& spec = GetSpecializationData(classId, specId);

    if (spec.primaryRole == role)
        return true;

    for (auto const& [capRole, capability] : spec.roleCapabilities)
    {
        if (capRole == role && capability >= minCapability)
            return true;
    }

    return false;
}

// GetPrimaryRole, IsHybridClass, IsPureClass are implemented inline in the header

// Get class versatility
float RoleDefinitions::GetClassVersatility(uint8 classId)
{
    const ClassData& classData = GetClassData(classId);
    return classData.overallVersatility;
}

// Get available roles
::std::vector<GroupRole> RoleDefinitions::GetAvailableRoles(uint8 classId, uint8 specId)
{
    ::std::vector<GroupRole> roles;
    const SpecializationData& spec = GetSpecializationData(classId, specId);

    roles.push_back(spec.primaryRole);

    for (auto const& [role, capability] : spec.roleCapabilities)
    {
        if (role != spec.primaryRole && capability >= RoleCapability::SECONDARY)
            roles.push_back(role);
    }

    return roles;
}

// Get preferred class/specs for role
::std::vector<::std::pair<uint8, uint8>> RoleDefinitions::GetPreferredClassSpecsForRole(GroupRole role)
{
    if (!GetInitialized())
        Initialize();

    ::std::vector<::std::pair<uint8, uint8>> result;

    for (auto const& [classId, classData] : GetClassDefinitions())
    {
        for (auto const& spec : classData.specializations)
        {
            if (spec.primaryRole == role)
                result.push_back({classId, spec.specId});
        }
    }

    return result;
}

// Get role priority score
float RoleDefinitions::GetRolePriorityScore(uint8 classId, uint8 specId, GroupRole role)
{
    return GetRoleEffectiveness(classId, specId, role);
}

// Initialize role definitions
void RoleDefinitions::Initialize()
{
    if (GetInitialized())
        return;

    TC_LOG_INFO("playerbot.roles", "RoleDefinitions: Initializing role definitions...");

    InitializeWarriorRoles();
    InitializePaladinRoles();
    InitializeHunterRoles();
    InitializeRogueRoles();
    InitializePriestRoles();
    InitializeDeathKnightRoles();
    InitializeShamanRoles();
    InitializeMageRoles();
    InitializeWarlockRoles();
    InitializeMonkRoles();
    InitializeDruidRoles();
    InitializeDemonHunterRoles();
    InitializeEvokerRoles();

    GetInitialized() = true;

    TC_LOG_INFO("playerbot.roles", "RoleDefinitions: Initialized {} class definitions", GetClassDefinitions().size());
}

// Initialize Warrior roles
void RoleDefinitions::InitializeWarriorRoles()
{
    ClassData warrior(CLASS_WARRIOR, "Warrior", false, 0.6f);

    // Protection (Tank)
    SpecializationData prot(0, "Protection", GroupRole::TANK, 0.95f);
    prot.roleCapabilities = {
        {GroupRole::TANK, RoleCapability::PRIMARY},
        {GroupRole::MELEE_DPS, RoleCapability::EMERGENCY}
    };
    warrior.specializations.push_back(prot);

    // Fury (Melee DPS)
    SpecializationData fury(1, "Fury", GroupRole::MELEE_DPS, 0.95f);
    fury.roleCapabilities = {
        {GroupRole::MELEE_DPS, RoleCapability::PRIMARY}
    };
    warrior.specializations.push_back(fury);

    // Arms (Melee DPS)
    SpecializationData arms(2, "Arms", GroupRole::MELEE_DPS, 0.95f);
    arms.roleCapabilities = {
        {GroupRole::MELEE_DPS, RoleCapability::PRIMARY}
    };
    warrior.specializations.push_back(arms);

    GetClassDefinitions()[CLASS_WARRIOR] = warrior;
}

// Initialize Paladin roles
void RoleDefinitions::InitializePaladinRoles()
{
    ClassData paladin(CLASS_PALADIN, "Paladin", true, 0.9f);

    // Holy (Healer)
    SpecializationData holy(0, "Holy", GroupRole::HEALER, 0.95f);
    holy.roleCapabilities = {
        {GroupRole::HEALER, RoleCapability::PRIMARY},
        {GroupRole::TANK, RoleCapability::EMERGENCY}
    };
    paladin.specializations.push_back(holy);

    // Protection (Tank)
    SpecializationData prot(1, "Protection", GroupRole::TANK, 0.95f);
    prot.roleCapabilities = {
        {GroupRole::TANK, RoleCapability::PRIMARY},
        {GroupRole::HEALER, RoleCapability::SECONDARY}
    };
    paladin.specializations.push_back(prot);

    // Retribution (Melee DPS)
    SpecializationData ret(2, "Retribution", GroupRole::MELEE_DPS, 0.90f);
    ret.roleCapabilities = {
        {GroupRole::MELEE_DPS, RoleCapability::PRIMARY},
        {GroupRole::HEALER, RoleCapability::SECONDARY}
    };
    paladin.specializations.push_back(ret);

    paladin.hybridCapabilities = {GroupRole::TANK, GroupRole::HEALER, GroupRole::MELEE_DPS};
    GetClassDefinitions()[CLASS_PALADIN] = paladin;
}

// Initialize Hunter roles
void RoleDefinitions::InitializeHunterRoles()
{
    ClassData hunter(CLASS_HUNTER, "Hunter", false, 0.5f);

    // Beast Mastery
    SpecializationData bm(0, "Beast Mastery", GroupRole::RANGED_DPS, 0.95f);
    bm.roleCapabilities = {
        {GroupRole::RANGED_DPS, RoleCapability::PRIMARY}
    };
    hunter.specializations.push_back(bm);

    // Marksmanship
    SpecializationData mm(1, "Marksmanship", GroupRole::RANGED_DPS, 0.95f);
    mm.roleCapabilities = {
        {GroupRole::RANGED_DPS, RoleCapability::PRIMARY}
    };
    hunter.specializations.push_back(mm);

    // Survival
    SpecializationData surv(2, "Survival", GroupRole::MELEE_DPS, 0.90f);
    surv.roleCapabilities = {
        {GroupRole::MELEE_DPS, RoleCapability::PRIMARY}
    };
    hunter.specializations.push_back(surv);

    GetClassDefinitions()[CLASS_HUNTER] = hunter;
}

// Initialize Rogue roles
void RoleDefinitions::InitializeRogueRoles()
{
    ClassData rogue(CLASS_ROGUE, "Rogue", false, 0.5f);

    // Assassination
    SpecializationData assassin(0, "Assassination", GroupRole::MELEE_DPS, 0.95f);
    assassin.roleCapabilities = {
        {GroupRole::MELEE_DPS, RoleCapability::PRIMARY}
    };
    rogue.specializations.push_back(assassin);

    // Outlaw
    SpecializationData outlaw(1, "Outlaw", GroupRole::MELEE_DPS, 0.95f);
    outlaw.roleCapabilities = {
        {GroupRole::MELEE_DPS, RoleCapability::PRIMARY}
    };
    rogue.specializations.push_back(outlaw);

    // Subtlety
    SpecializationData sub(2, "Subtlety", GroupRole::MELEE_DPS, 0.95f);
    sub.roleCapabilities = {
        {GroupRole::MELEE_DPS, RoleCapability::PRIMARY}
    };
    rogue.specializations.push_back(sub);

    GetClassDefinitions()[CLASS_ROGUE] = rogue;
}

// Initialize Priest roles
void RoleDefinitions::InitializePriestRoles()
{
    ClassData priest(CLASS_PRIEST, "Priest", false, 0.7f);

    // Discipline
    SpecializationData disc(0, "Discipline", GroupRole::HEALER, 0.95f);
    disc.roleCapabilities = {
        {GroupRole::HEALER, RoleCapability::PRIMARY},
        {GroupRole::RANGED_DPS, RoleCapability::SECONDARY}
    };
    priest.specializations.push_back(disc);

    // Holy
    SpecializationData holy(1, "Holy", GroupRole::HEALER, 0.95f);
    holy.roleCapabilities = {
        {GroupRole::HEALER, RoleCapability::PRIMARY}
    };
    priest.specializations.push_back(holy);

    // Shadow
    SpecializationData shadow(2, "Shadow", GroupRole::RANGED_DPS, 0.95f);
    shadow.roleCapabilities = {
        {GroupRole::RANGED_DPS, RoleCapability::PRIMARY},
        {GroupRole::HEALER, RoleCapability::EMERGENCY}
    };
    priest.specializations.push_back(shadow);

    GetClassDefinitions()[CLASS_PRIEST] = priest;
}

// Initialize Death Knight roles
void RoleDefinitions::InitializeDeathKnightRoles()
{
    ClassData dk(CLASS_DEATH_KNIGHT, "Death Knight", false, 0.6f);

    // Blood (Tank)
    SpecializationData blood(0, "Blood", GroupRole::TANK, 0.95f);
    blood.roleCapabilities = {
        {GroupRole::TANK, RoleCapability::PRIMARY}
    };
    dk.specializations.push_back(blood);

    // Frost (Melee DPS)
    SpecializationData frost(1, "Frost", GroupRole::MELEE_DPS, 0.95f);
    frost.roleCapabilities = {
        {GroupRole::MELEE_DPS, RoleCapability::PRIMARY}
    };
    dk.specializations.push_back(frost);

    // Unholy (Melee DPS)
    SpecializationData unholy(2, "Unholy", GroupRole::MELEE_DPS, 0.95f);
    unholy.roleCapabilities = {
        {GroupRole::MELEE_DPS, RoleCapability::PRIMARY}
    };
    dk.specializations.push_back(unholy);

    GetClassDefinitions()[CLASS_DEATH_KNIGHT] = dk;
}

// Initialize Shaman roles
void RoleDefinitions::InitializeShamanRoles()
{
    ClassData shaman(CLASS_SHAMAN, "Shaman", true, 0.8f);

    // Elemental (Ranged DPS)
    SpecializationData ele(0, "Elemental", GroupRole::RANGED_DPS, 0.95f);
    ele.roleCapabilities = {
        {GroupRole::RANGED_DPS, RoleCapability::PRIMARY},
        {GroupRole::HEALER, RoleCapability::EMERGENCY}
    };
    shaman.specializations.push_back(ele);

    // Enhancement (Melee DPS)
    SpecializationData enh(1, "Enhancement", GroupRole::MELEE_DPS, 0.95f);
    enh.roleCapabilities = {
        {GroupRole::MELEE_DPS, RoleCapability::PRIMARY},
        {GroupRole::HEALER, RoleCapability::EMERGENCY}
    };
    shaman.specializations.push_back(enh);

    // Restoration (Healer)
    SpecializationData resto(2, "Restoration", GroupRole::HEALER, 0.95f);
    resto.roleCapabilities = {
        {GroupRole::HEALER, RoleCapability::PRIMARY},
        {GroupRole::RANGED_DPS, RoleCapability::SECONDARY}
    };
    shaman.specializations.push_back(resto);

    shaman.hybridCapabilities = {GroupRole::HEALER, GroupRole::RANGED_DPS, GroupRole::MELEE_DPS};
    GetClassDefinitions()[CLASS_SHAMAN] = shaman;
}

// Initialize Mage roles
void RoleDefinitions::InitializeMageRoles()
{
    ClassData mage(CLASS_MAGE, "Mage", false, 0.5f);

    // Arcane
    SpecializationData arcane(0, "Arcane", GroupRole::RANGED_DPS, 0.95f);
    arcane.roleCapabilities = {
        {GroupRole::RANGED_DPS, RoleCapability::PRIMARY}
    };
    mage.specializations.push_back(arcane);

    // Fire
    SpecializationData fire(1, "Fire", GroupRole::RANGED_DPS, 0.95f);
    fire.roleCapabilities = {
        {GroupRole::RANGED_DPS, RoleCapability::PRIMARY}
    };
    mage.specializations.push_back(fire);

    // Frost
    SpecializationData frost(2, "Frost", GroupRole::RANGED_DPS, 0.95f);
    frost.roleCapabilities = {
        {GroupRole::RANGED_DPS, RoleCapability::PRIMARY}
    };
    mage.specializations.push_back(frost);

    GetClassDefinitions()[CLASS_MAGE] = mage;
}

// Initialize Warlock roles
void RoleDefinitions::InitializeWarlockRoles()
{
    ClassData warlock(CLASS_WARLOCK, "Warlock", false, 0.5f);

    // Affliction
    SpecializationData afflic(0, "Affliction", GroupRole::RANGED_DPS, 0.95f);
    afflic.roleCapabilities = {
        {GroupRole::RANGED_DPS, RoleCapability::PRIMARY}
    };
    warlock.specializations.push_back(afflic);

    // Demonology
    SpecializationData demo(1, "Demonology", GroupRole::RANGED_DPS, 0.95f);
    demo.roleCapabilities = {
        {GroupRole::RANGED_DPS, RoleCapability::PRIMARY}
    };
    warlock.specializations.push_back(demo);

    // Destruction
    SpecializationData destro(2, "Destruction", GroupRole::RANGED_DPS, 0.95f);
    destro.roleCapabilities = {
        {GroupRole::RANGED_DPS, RoleCapability::PRIMARY}
    };
    warlock.specializations.push_back(destro);

    GetClassDefinitions()[CLASS_WARLOCK] = warlock;
}

// Initialize Monk roles
void RoleDefinitions::InitializeMonkRoles()
{
    ClassData monk(CLASS_MONK, "Monk", true, 0.9f);

    // Brewmaster (Tank)
    SpecializationData brew(0, "Brewmaster", GroupRole::TANK, 0.95f);
    brew.roleCapabilities = {
        {GroupRole::TANK, RoleCapability::PRIMARY}
    };
    monk.specializations.push_back(brew);

    // Mistweaver (Healer)
    SpecializationData mist(1, "Mistweaver", GroupRole::HEALER, 0.95f);
    mist.roleCapabilities = {
        {GroupRole::HEALER, RoleCapability::PRIMARY},
        {GroupRole::MELEE_DPS, RoleCapability::SECONDARY}
    };
    monk.specializations.push_back(mist);

    // Windwalker (Melee DPS)
    SpecializationData wind(2, "Windwalker", GroupRole::MELEE_DPS, 0.95f);
    wind.roleCapabilities = {
        {GroupRole::MELEE_DPS, RoleCapability::PRIMARY}
    };
    monk.specializations.push_back(wind);

    monk.hybridCapabilities = {GroupRole::TANK, GroupRole::HEALER, GroupRole::MELEE_DPS};
    GetClassDefinitions()[CLASS_MONK] = monk;
}

// Initialize Druid roles
void RoleDefinitions::InitializeDruidRoles()
{
    ClassData druid(CLASS_DRUID, "Druid", true, 1.0f);

    // Balance (Ranged DPS)
    SpecializationData balance(0, "Balance", GroupRole::RANGED_DPS, 0.95f);
    balance.roleCapabilities = {
        {GroupRole::RANGED_DPS, RoleCapability::PRIMARY},
        {GroupRole::HEALER, RoleCapability::EMERGENCY}
    };
    druid.specializations.push_back(balance);

    // Feral (Melee DPS)
    SpecializationData feral(1, "Feral", GroupRole::MELEE_DPS, 0.95f);
    feral.roleCapabilities = {
        {GroupRole::MELEE_DPS, RoleCapability::PRIMARY},
        {GroupRole::TANK, RoleCapability::SECONDARY}
    };
    druid.specializations.push_back(feral);

    // Guardian (Tank)
    SpecializationData guardian(2, "Guardian", GroupRole::TANK, 0.95f);
    guardian.roleCapabilities = {
        {GroupRole::TANK, RoleCapability::PRIMARY},
        {GroupRole::MELEE_DPS, RoleCapability::SECONDARY}
    };
    druid.specializations.push_back(guardian);

    // Restoration (Healer)
    SpecializationData resto(3, "Restoration", GroupRole::HEALER, 0.95f);
    resto.roleCapabilities = {
        {GroupRole::HEALER, RoleCapability::PRIMARY},
        {GroupRole::RANGED_DPS, RoleCapability::SECONDARY}
    };
    druid.specializations.push_back(resto);

    druid.hybridCapabilities = {GroupRole::TANK, GroupRole::HEALER, GroupRole::RANGED_DPS, GroupRole::MELEE_DPS};
    GetClassDefinitions()[CLASS_DRUID] = druid;
}

// Initialize Demon Hunter roles
void RoleDefinitions::InitializeDemonHunterRoles()
{
    ClassData dh(CLASS_DEMON_HUNTER, "Demon Hunter", false, 0.6f);

    // Havoc (Melee DPS)
    SpecializationData havoc(0, "Havoc", GroupRole::MELEE_DPS, 0.95f);
    havoc.roleCapabilities = {
        {GroupRole::MELEE_DPS, RoleCapability::PRIMARY}
    };
    dh.specializations.push_back(havoc);

    // Vengeance (Tank)
    SpecializationData veng(1, "Vengeance", GroupRole::TANK, 0.95f);
    veng.roleCapabilities = {
        {GroupRole::TANK, RoleCapability::PRIMARY},
        {GroupRole::MELEE_DPS, RoleCapability::SECONDARY}
    };
    dh.specializations.push_back(veng);

    GetClassDefinitions()[CLASS_DEMON_HUNTER] = dh;
}

// Initialize Evoker roles
void RoleDefinitions::InitializeEvokerRoles()
{
    ClassData evoker(CLASS_EVOKER, "Evoker", true, 0.8f);

    // Devastation (Ranged DPS)
    SpecializationData dev(0, "Devastation", GroupRole::RANGED_DPS, 0.95f);
    dev.roleCapabilities = {
        {GroupRole::RANGED_DPS, RoleCapability::PRIMARY},
        {GroupRole::HEALER, RoleCapability::SECONDARY}
    };
    evoker.specializations.push_back(dev);

    // Preservation (Healer)
    SpecializationData pres(1, "Preservation", GroupRole::HEALER, 0.95f);
    pres.roleCapabilities = {
        {GroupRole::HEALER, RoleCapability::PRIMARY},
        {GroupRole::RANGED_DPS, RoleCapability::SECONDARY}
    };
    evoker.specializations.push_back(pres);

    // Augmentation (Support)
    SpecializationData aug(2, "Augmentation", GroupRole::RANGED_DPS, 0.85f);
    aug.roleCapabilities = {
        {GroupRole::RANGED_DPS, RoleCapability::PRIMARY}
    };
    evoker.specializations.push_back(aug);

    evoker.hybridCapabilities = {GroupRole::HEALER, GroupRole::RANGED_DPS};
    GetClassDefinitions()[CLASS_EVOKER] = evoker;
}

} // namespace Playerbot
