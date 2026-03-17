/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "RoleAssignment.h"
#include <unordered_map>
#include <vector>

namespace Playerbot
{

/**
 * @brief Comprehensive role definitions and class/spec mappings for WoW classes
 *
 * This system maps all WoW classes and specializations to their appropriate roles,
 * defining primary, secondary, and hybrid capabilities for intelligent role assignment.
 */
class TC_GAME_API RoleDefinitions
{
public:
    // Class definitions (matching TrinityCore's Classes enum)
    enum Classes : uint8
    {
        CLASS_WARRIOR       = 1,
        CLASS_PALADIN       = 2,
        CLASS_HUNTER        = 3,
        CLASS_ROGUE         = 4,
        CLASS_PRIEST        = 5,
        CLASS_DEATH_KNIGHT  = 6,
        CLASS_SHAMAN        = 7,
        CLASS_MAGE          = 8,
        CLASS_WARLOCK       = 9,
        CLASS_MONK          = 10,
        CLASS_DRUID         = 11,
        CLASS_DEMON_HUNTER  = 12,
        CLASS_EVOKER        = 13
    };

    // Specialization definitions for each class
    struct SpecializationData
    {
        uint8 specId;
        ::std::string name;
        GroupRole primaryRole;
        ::std::vector<::std::pair<GroupRole, RoleCapability>> roleCapabilities;
        float baseEffectiveness;
        ::std::vector<::std::string> keyAbilities;

        SpecializationData() : specId(0), primaryRole(GroupRole::NONE), baseEffectiveness(0.0f) {}
        SpecializationData(uint8 id, const ::std::string& n, GroupRole primary, float effectiveness)
            : specId(id), name(n), primaryRole(primary), baseEffectiveness(effectiveness) {}
    };

    struct ClassData
    {
        uint8 classId;
        ::std::string className;
        ::std::vector<SpecializationData> specializations;
        ::std::vector<GroupRole> hybridCapabilities;
        bool isHybridClass;
        float overallVersatility;

        ClassData() : classId(0), isHybridClass(false), overallVersatility(0.0f) {}
        ClassData(uint8 id, const ::std::string& name, bool hybrid = false, float versatility = 0.5f)
            : classId(id), className(name), isHybridClass(hybrid), overallVersatility(versatility) {}
    };

    // Get role definitions
    static const ClassData& GetClassData(uint8 classId);
    static const SpecializationData& GetSpecializationData(uint8 classId, uint8 specId);
    static ::std::vector<::std::pair<GroupRole, RoleCapability>> GetRoleCapabilities(uint8 classId, uint8 specId);

    // Role effectiveness calculations
    static float GetRoleEffectiveness(uint8 classId, uint8 specId, GroupRole role);
    static bool CanPerformRole(uint8 classId, uint8 specId, GroupRole role, RoleCapability minCapability = RoleCapability::SECONDARY);
    static GroupRole GetPrimaryRole(uint8 classId, uint8 specId);

    // Class analysis
    static bool IsHybridClass(uint8 classId);
    static bool IsPureClass(uint8 classId);
    static float GetClassVersatility(uint8 classId);
    static ::std::vector<GroupRole> GetAvailableRoles(uint8 classId, uint8 specId);

    // Role priority for group formation
    static ::std::vector<::std::pair<uint8, uint8>> GetPreferredClassSpecsForRole(GroupRole role);
    static float GetRolePriorityScore(uint8 classId, uint8 specId, GroupRole role);

    // Initialize role definitions
    static void Initialize();

private:
    // Meyer's singleton accessors for DLL-safe static data
    static ::std::unordered_map<uint8, ClassData>& GetClassDefinitions();
    static bool& GetInitialized();

    // Role mapping initialization
    static void InitializeWarriorRoles();
    static void InitializePaladinRoles();
    static void InitializeHunterRoles();
    static void InitializeRogueRoles();
    static void InitializePriestRoles();
    static void InitializeDeathKnightRoles();
    static void InitializeShamanRoles();
    static void InitializeMageRoles();
    static void InitializeWarlockRoles();
    static void InitializeMonkRoles();
    static void InitializeDruidRoles();
    static void InitializeDemonHunterRoles();
    static void InitializeEvokerRoles();
};

/**
 * COMPREHENSIVE ROLE DEFINITIONS
 *
 * Each class and specialization is mapped to roles with specific capabilities:
 *
 * PRIMARY: Main role the spec is designed for (highest effectiveness)
 * SECONDARY: Can perform well in off-spec role
 * HYBRID: Can effectively perform multiple roles simultaneously
 * EMERGENCY: Can fill role in desperate situations
 * INCAPABLE: Cannot perform this role effectively
 *
 * WARRIORS:
 * - Protection: PRIMARY Tank, EMERGENCY Melee DPS
 * - Fury: PRIMARY Melee DPS, EMERGENCY Tank (low level)
 * - Arms: PRIMARY Melee DPS, EMERGENCY Tank (low level)
 *
 * PALADINS (Hybrid Class):
 * - Holy: PRIMARY Healer, SECONDARY Support, EMERGENCY Tank
 * - Protection: PRIMARY Tank, SECONDARY Healer, EMERGENCY Support
 * - Retribution: PRIMARY Melee DPS, SECONDARY Tank, SECONDARY Healer
 *
 * HUNTERS:
 * - Beast Mastery: PRIMARY Ranged DPS, SECONDARY Utility (pet tank)
 * - Marksmanship: PRIMARY Ranged DPS, SECONDARY Utility
 * - Survival: PRIMARY Melee DPS, SECONDARY Ranged DPS, SECONDARY Utility
 *
 * ROGUES:
 * - Assassination: PRIMARY Melee DPS, SECONDARY Utility (stealth/cc)
 * - Combat: PRIMARY Melee DPS, SECONDARY Utility
 * - Subtlety: PRIMARY Melee DPS, PRIMARY Utility (stealth specialist)
 *
 * PRIESTS:
 * - Discipline: PRIMARY Healer, SECONDARY Support (shields/buffs)
 * - Holy: PRIMARY Healer, SECONDARY Support
 * - Shadow: PRIMARY Ranged DPS, SECONDARY Support (mana/buffs)
 *
 * DEATH KNIGHTS:
 * - Blood: PRIMARY Tank, SECONDARY Melee DPS
 * - Frost: PRIMARY Melee DPS, EMERGENCY Tank
 * - Unholy: PRIMARY Melee DPS, SECONDARY Utility (diseases/minions)
 *
 * SHAMANS (Hybrid Class):
 * - Elemental: PRIMARY Ranged DPS, SECONDARY Healer, SECONDARY Support
 * - Enhancement: PRIMARY Melee DPS, SECONDARY Healer, SECONDARY Support
 * - Restoration: PRIMARY Healer, SECONDARY Support, SECONDARY Ranged DPS
 *
 * MAGES:
 * - Arcane: PRIMARY Ranged DPS, SECONDARY Utility (portals/food)
 * - Fire: PRIMARY Ranged DPS, SECONDARY Utility
 * - Frost: PRIMARY Ranged DPS, SECONDARY Utility (crowd control)
 *
 * WARLOCKS:
 * - Affliction: PRIMARY Ranged DPS, SECONDARY Utility (curses/fear)
 * - Demonology: PRIMARY Ranged DPS, SECONDARY Tank (pet), SECONDARY Utility
 * - Destruction: PRIMARY Ranged DPS, SECONDARY Utility
 *
 * MONKS (Hybrid Class):
 * - Brewmaster: PRIMARY Tank, SECONDARY Melee DPS
 * - Mistweaver: PRIMARY Healer, SECONDARY Melee DPS, SECONDARY Support
 * - Windwalker: PRIMARY Melee DPS, SECONDARY Support (utility abilities)
 *
 * DRUIDS (Ultimate Hybrid Class):
 * - Guardian: PRIMARY Tank, SECONDARY Melee DPS, EMERGENCY Healer
 * - Feral: PRIMARY Melee DPS, SECONDARY Tank, EMERGENCY Healer
 * - Balance: PRIMARY Ranged DPS, SECONDARY Healer, SECONDARY Support
 * - Restoration: PRIMARY Healer, SECONDARY Support, SECONDARY Ranged DPS
 *
 * DEMON HUNTERS:
 * - Havoc: PRIMARY Melee DPS, SECONDARY Utility (mobility/aoe)
 * - Vengeance: PRIMARY Tank, SECONDARY Melee DPS
 *
 * EVOKERS (Hybrid Class):
 * - Devastation: PRIMARY Ranged DPS, SECONDARY Support (utility spells)
 * - Preservation: PRIMARY Healer, SECONDARY Ranged DPS, SECONDARY Support
 * - Augmentation: PRIMARY Support, SECONDARY Ranged DPS, SECONDARY Healer
 */

// Inline implementation for performance-critical functions
inline bool RoleDefinitions::IsHybridClass(uint8 classId)
{
    return classId == CLASS_PALADIN || classId == CLASS_SHAMAN ||
           classId == CLASS_DRUID || classId == CLASS_MONK ||
           classId == CLASS_EVOKER;
}

inline bool RoleDefinitions::IsPureClass(uint8 classId)
{
    return classId == CLASS_MAGE || classId == CLASS_WARLOCK ||
           classId == CLASS_ROGUE || classId == CLASS_HUNTER;
}

inline GroupRole RoleDefinitions::GetPrimaryRole(uint8 classId, uint8 specId)
{
    if (!GetInitialized()) Initialize();

    auto it = GetClassDefinitions().find(classId);
    if (it != GetClassDefinitions().end())
    {
        for (const auto& spec : it->second.specializations)
        {
            if (spec.specId == specId)
                return spec.primaryRole;
        }
    }
    return GroupRole::NONE;
}

} // namespace Playerbot