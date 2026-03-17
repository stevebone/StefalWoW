/*
 * ClassRoleResolver - Central Class/Role Mapping
 * 
 * Replaces 28 scattered switch statements with a single source of truth.
 * 
 * WoW 11.2 Class/Spec IDs:
 * - Spec IDs follow the pattern: (ClassId * 100) + SpecIndex
 * - This file defines the canonical role mapping for all specs
 */

#pragma once

#include "CombatSystemInterfaces.h"
#include <unordered_map>
#include <array>

namespace Playerbot::Combat
{

// ============================================================================
// WOW 11.2 CLASS CONSTANTS
// ============================================================================

enum class WoWClass : uint8
{
    WARRIOR = 1,
    PALADIN = 2,
    HUNTER = 3,
    ROGUE = 4,
    PRIEST = 5,
    DEATH_KNIGHT = 6,
    SHAMAN = 7,
    MAGE = 8,
    WARLOCK = 9,
    MONK = 10,
    DRUID = 11,
    DEMON_HUNTER = 12,
    EVOKER = 13
};

// ============================================================================
// WOW 11.2 SPEC CONSTANTS
// ============================================================================

namespace Specs
{
    // Warrior (1)
    constexpr uint32 WARRIOR_ARMS = 71;
    constexpr uint32 WARRIOR_FURY = 72;
    constexpr uint32 WARRIOR_PROTECTION = 73;
    
    // Paladin (2)
    constexpr uint32 PALADIN_HOLY = 65;
    constexpr uint32 PALADIN_PROTECTION = 66;
    constexpr uint32 PALADIN_RETRIBUTION = 70;
    
    // Hunter (3)
    constexpr uint32 HUNTER_BEAST_MASTERY = 253;
    constexpr uint32 HUNTER_MARKSMANSHIP = 254;
    constexpr uint32 HUNTER_SURVIVAL = 255;
    
    // Rogue (4)
    constexpr uint32 ROGUE_ASSASSINATION = 259;
    constexpr uint32 ROGUE_OUTLAW = 260;
    constexpr uint32 ROGUE_SUBTLETY = 261;
    
    // Priest (5)
    constexpr uint32 PRIEST_DISCIPLINE = 256;
    constexpr uint32 PRIEST_HOLY = 257;
    constexpr uint32 PRIEST_SHADOW = 258;
    
    // Death Knight (6)
    constexpr uint32 DK_BLOOD = 250;
    constexpr uint32 DK_FROST = 251;
    constexpr uint32 DK_UNHOLY = 252;
    
    // Shaman (7)
    constexpr uint32 SHAMAN_ELEMENTAL = 262;
    constexpr uint32 SHAMAN_ENHANCEMENT = 263;
    constexpr uint32 SHAMAN_RESTORATION = 264;
    
    // Mage (8)
    constexpr uint32 MAGE_ARCANE = 62;
    constexpr uint32 MAGE_FIRE = 63;
    constexpr uint32 MAGE_FROST = 64;
    
    // Warlock (9)
    constexpr uint32 WARLOCK_AFFLICTION = 265;
    constexpr uint32 WARLOCK_DEMONOLOGY = 266;
    constexpr uint32 WARLOCK_DESTRUCTION = 267;
    
    // Monk (10)
    constexpr uint32 MONK_BREWMASTER = 268;
    constexpr uint32 MONK_MISTWEAVER = 270;
    constexpr uint32 MONK_WINDWALKER = 269;
    
    // Druid (11)
    constexpr uint32 DRUID_BALANCE = 102;
    constexpr uint32 DRUID_FERAL = 103;
    constexpr uint32 DRUID_GUARDIAN = 104;
    constexpr uint32 DRUID_RESTORATION = 105;
    
    // Demon Hunter (12)
    constexpr uint32 DH_HAVOC = 577;
    constexpr uint32 DH_VENGEANCE = 581;
    
    // Evoker (13)
    constexpr uint32 EVOKER_DEVASTATION = 1467;
    constexpr uint32 EVOKER_PRESERVATION = 1468;
    constexpr uint32 EVOKER_AUGMENTATION = 1473;
}

// ============================================================================
// SPEC INFO STRUCTURE
// ============================================================================

struct SpecInfo
{
    uint32 specId;
    uint8 classId;
    BotRole primaryRole;
    bool canTank;
    bool canHeal;
    bool isMelee;
    std::string specName;
    std::string className;
};

// ============================================================================
// CLASS ROLE RESOLVER IMPLEMENTATION
// ============================================================================

/**
 * @class ClassRoleResolver
 * @brief Singleton implementation of IClassRoleResolver
 *
 * This replaces the 28 switch statements found across the codebase:
 * - AdaptiveBehaviorManager.cpp:502-520
 * - BotThreatManager.cpp:48
 * - Strategy.cpp:952
 * - And 25 more locations
 *
 * All spec/role information is defined here in one place.
 */
class ClassRoleResolver : public IClassRoleResolver
{
public:
    static ClassRoleResolver& Instance()
    {
        static ClassRoleResolver instance;
        return instance;
    }
    
    // IClassRoleResolver implementation
    BotRole GetPrimaryRole(uint8 classId, uint8 specId) const override
    {
        auto it = _specInfo.find(specId);
        if (it != _specInfo.end())
            return it->second.primaryRole;
        
        // Fallback based on class if spec not found
        return GetDefaultRoleForClass(classId);
    }
    
    std::vector<BotRole> GetAvailableRoles(uint8 classId) const override
    {
        std::vector<BotRole> roles;
        
        for (auto const& [specId, info] : _specInfo)
        {
            if (info.classId == classId)
            {
                if (std::find(roles.begin(), roles.end(), info.primaryRole) == roles.end())
                    roles.push_back(info.primaryRole);
            }
        }
        
        return roles;
    }
    
    bool CanPerformRole(uint8 classId, uint8 specId, BotRole role) const override
    {
        auto it = _specInfo.find(specId);
        if (it == _specInfo.end())
            return false;
        
        switch (role)
        {
            case BotRole::TANK:
                return it->second.canTank;
            case BotRole::HEALER:
                return it->second.canHeal;
            case BotRole::MELEE_DPS:
                return it->second.isMelee && !it->second.canTank && !it->second.canHeal;
            case BotRole::RANGED_DPS:
                return !it->second.isMelee && !it->second.canTank && !it->second.canHeal;
            default:
                return false;
        }
    }
    
    bool IsTankSpec(uint8 classId, uint8 specId) const override
    {
        auto it = _specInfo.find(specId);
        return it != _specInfo.end() && it->second.canTank;
    }
    
    bool IsHealerSpec(uint8 classId, uint8 specId) const override
    {
        auto it = _specInfo.find(specId);
        return it != _specInfo.end() && it->second.canHeal;
    }
    
    bool IsMeleeSpec(uint8 classId, uint8 specId) const override
    {
        auto it = _specInfo.find(specId);
        return it != _specInfo.end() && it->second.isMelee;
    }
    
    std::string GetSpecName(uint8 classId, uint8 specId) const override
    {
        auto it = _specInfo.find(specId);
        if (it != _specInfo.end())
            return it->second.specName;
        return "Unknown";
    }
    
    // Additional helpers
    std::string GetClassName(uint8 classId) const
    {
        switch (classId)
        {
            case 1: return "Warrior";
            case 2: return "Paladin";
            case 3: return "Hunter";
            case 4: return "Rogue";
            case 5: return "Priest";
            case 6: return "Death Knight";
            case 7: return "Shaman";
            case 8: return "Mage";
            case 9: return "Warlock";
            case 10: return "Monk";
            case 11: return "Druid";
            case 12: return "Demon Hunter";
            case 13: return "Evoker";
            default: return "Unknown";
        }
    }
    
    SpecInfo const* GetSpecInfo(uint32 specId) const
    {
        auto it = _specInfo.find(specId);
        return it != _specInfo.end() ? &it->second : nullptr;
    }
    
    // Get all tank specs
    std::vector<uint32> GetAllTankSpecs() const
    {
        std::vector<uint32> result;
        for (auto const& [specId, info] : _specInfo)
        {
            if (info.canTank)
                result.push_back(specId);
        }
        return result;
    }
    
    // Get all healer specs
    std::vector<uint32> GetAllHealerSpecs() const
    {
        std::vector<uint32> result;
        for (auto const& [specId, info] : _specInfo)
        {
            if (info.canHeal)
                result.push_back(specId);
        }
        return result;
    }
    
private:
    ClassRoleResolver()
    {
        InitializeSpecInfo();
    }
    
    void InitializeSpecInfo()
    {
        using namespace Specs;
        
        // ====================================================================
        // WARRIOR
        // ====================================================================
        _specInfo[WARRIOR_ARMS] = {
            WARRIOR_ARMS, 1, BotRole::MELEE_DPS,
            false, false, true,
            "Arms", "Warrior"
        };
        _specInfo[WARRIOR_FURY] = {
            WARRIOR_FURY, 1, BotRole::MELEE_DPS,
            false, false, true,
            "Fury", "Warrior"
        };
        _specInfo[WARRIOR_PROTECTION] = {
            WARRIOR_PROTECTION, 1, BotRole::TANK,
            true, false, true,
            "Protection", "Warrior"
        };
        
        // ====================================================================
        // PALADIN
        // ====================================================================
        _specInfo[PALADIN_HOLY] = {
            PALADIN_HOLY, 2, BotRole::HEALER,
            false, true, false,
            "Holy", "Paladin"
        };
        _specInfo[PALADIN_PROTECTION] = {
            PALADIN_PROTECTION, 2, BotRole::TANK,
            true, false, true,
            "Protection", "Paladin"
        };
        _specInfo[PALADIN_RETRIBUTION] = {
            PALADIN_RETRIBUTION, 2, BotRole::MELEE_DPS,
            false, false, true,
            "Retribution", "Paladin"
        };
        
        // ====================================================================
        // HUNTER
        // ====================================================================
        _specInfo[HUNTER_BEAST_MASTERY] = {
            HUNTER_BEAST_MASTERY, 3, BotRole::RANGED_DPS,
            false, false, false,
            "Beast Mastery", "Hunter"
        };
        _specInfo[HUNTER_MARKSMANSHIP] = {
            HUNTER_MARKSMANSHIP, 3, BotRole::RANGED_DPS,
            false, false, false,
            "Marksmanship", "Hunter"
        };
        _specInfo[HUNTER_SURVIVAL] = {
            HUNTER_SURVIVAL, 3, BotRole::MELEE_DPS,
            false, false, true,  // Survival is melee in modern WoW
            "Survival", "Hunter"
        };
        
        // ====================================================================
        // ROGUE
        // ====================================================================
        _specInfo[ROGUE_ASSASSINATION] = {
            ROGUE_ASSASSINATION, 4, BotRole::MELEE_DPS,
            false, false, true,
            "Assassination", "Rogue"
        };
        _specInfo[ROGUE_OUTLAW] = {
            ROGUE_OUTLAW, 4, BotRole::MELEE_DPS,
            false, false, true,
            "Outlaw", "Rogue"
        };
        _specInfo[ROGUE_SUBTLETY] = {
            ROGUE_SUBTLETY, 4, BotRole::MELEE_DPS,
            false, false, true,
            "Subtlety", "Rogue"
        };
        
        // ====================================================================
        // PRIEST
        // ====================================================================
        _specInfo[PRIEST_DISCIPLINE] = {
            PRIEST_DISCIPLINE, 5, BotRole::HEALER,
            false, true, false,
            "Discipline", "Priest"
        };
        _specInfo[PRIEST_HOLY] = {
            PRIEST_HOLY, 5, BotRole::HEALER,
            false, true, false,
            "Holy", "Priest"
        };
        _specInfo[PRIEST_SHADOW] = {
            PRIEST_SHADOW, 5, BotRole::RANGED_DPS,
            false, false, false,
            "Shadow", "Priest"
        };
        
        // ====================================================================
        // DEATH KNIGHT
        // ====================================================================
        _specInfo[DK_BLOOD] = {
            DK_BLOOD, 6, BotRole::TANK,
            true, false, true,
            "Blood", "Death Knight"
        };
        _specInfo[DK_FROST] = {
            DK_FROST, 6, BotRole::MELEE_DPS,
            false, false, true,
            "Frost", "Death Knight"
        };
        _specInfo[DK_UNHOLY] = {
            DK_UNHOLY, 6, BotRole::MELEE_DPS,
            false, false, true,
            "Unholy", "Death Knight"
        };
        
        // ====================================================================
        // SHAMAN
        // ====================================================================
        _specInfo[SHAMAN_ELEMENTAL] = {
            SHAMAN_ELEMENTAL, 7, BotRole::RANGED_DPS,
            false, false, false,
            "Elemental", "Shaman"
        };
        _specInfo[SHAMAN_ENHANCEMENT] = {
            SHAMAN_ENHANCEMENT, 7, BotRole::MELEE_DPS,
            false, false, true,
            "Enhancement", "Shaman"
        };
        _specInfo[SHAMAN_RESTORATION] = {
            SHAMAN_RESTORATION, 7, BotRole::HEALER,
            false, true, false,
            "Restoration", "Shaman"
        };
        
        // ====================================================================
        // MAGE
        // ====================================================================
        _specInfo[MAGE_ARCANE] = {
            MAGE_ARCANE, 8, BotRole::RANGED_DPS,
            false, false, false,
            "Arcane", "Mage"
        };
        _specInfo[MAGE_FIRE] = {
            MAGE_FIRE, 8, BotRole::RANGED_DPS,
            false, false, false,
            "Fire", "Mage"
        };
        _specInfo[MAGE_FROST] = {
            MAGE_FROST, 8, BotRole::RANGED_DPS,
            false, false, false,
            "Frost", "Mage"
        };
        
        // ====================================================================
        // WARLOCK
        // ====================================================================
        _specInfo[WARLOCK_AFFLICTION] = {
            WARLOCK_AFFLICTION, 9, BotRole::RANGED_DPS,
            false, false, false,
            "Affliction", "Warlock"
        };
        _specInfo[WARLOCK_DEMONOLOGY] = {
            WARLOCK_DEMONOLOGY, 9, BotRole::RANGED_DPS,
            false, false, false,
            "Demonology", "Warlock"
        };
        _specInfo[WARLOCK_DESTRUCTION] = {
            WARLOCK_DESTRUCTION, 9, BotRole::RANGED_DPS,
            false, false, false,
            "Destruction", "Warlock"
        };
        
        // ====================================================================
        // MONK
        // ====================================================================
        _specInfo[MONK_BREWMASTER] = {
            MONK_BREWMASTER, 10, BotRole::TANK,
            true, false, true,
            "Brewmaster", "Monk"
        };
        _specInfo[MONK_MISTWEAVER] = {
            MONK_MISTWEAVER, 10, BotRole::HEALER,
            false, true, false,
            "Mistweaver", "Monk"
        };
        _specInfo[MONK_WINDWALKER] = {
            MONK_WINDWALKER, 10, BotRole::MELEE_DPS,
            false, false, true,
            "Windwalker", "Monk"
        };
        
        // ====================================================================
        // DRUID
        // ====================================================================
        _specInfo[DRUID_BALANCE] = {
            DRUID_BALANCE, 11, BotRole::RANGED_DPS,
            false, false, false,
            "Balance", "Druid"
        };
        _specInfo[DRUID_FERAL] = {
            DRUID_FERAL, 11, BotRole::MELEE_DPS,
            false, false, true,
            "Feral", "Druid"
        };
        _specInfo[DRUID_GUARDIAN] = {
            DRUID_GUARDIAN, 11, BotRole::TANK,
            true, false, true,
            "Guardian", "Druid"
        };
        _specInfo[DRUID_RESTORATION] = {
            DRUID_RESTORATION, 11, BotRole::HEALER,
            false, true, false,
            "Restoration", "Druid"
        };
        
        // ====================================================================
        // DEMON HUNTER
        // ====================================================================
        _specInfo[DH_HAVOC] = {
            DH_HAVOC, 12, BotRole::MELEE_DPS,
            false, false, true,
            "Havoc", "Demon Hunter"
        };
        _specInfo[DH_VENGEANCE] = {
            DH_VENGEANCE, 12, BotRole::TANK,
            true, false, true,
            "Vengeance", "Demon Hunter"
        };
        
        // ====================================================================
        // EVOKER
        // ====================================================================
        _specInfo[EVOKER_DEVASTATION] = {
            EVOKER_DEVASTATION, 13, BotRole::RANGED_DPS,
            false, false, false,
            "Devastation", "Evoker"
        };
        _specInfo[EVOKER_PRESERVATION] = {
            EVOKER_PRESERVATION, 13, BotRole::HEALER,
            false, true, false,
            "Preservation", "Evoker"
        };
        _specInfo[EVOKER_AUGMENTATION] = {
            EVOKER_AUGMENTATION, 13, BotRole::RANGED_DPS,  // Support DPS
            false, false, false,
            "Augmentation", "Evoker"
        };
    }
    
    BotRole GetDefaultRoleForClass(uint8 classId) const
    {
        // Fallback when spec is unknown
        switch (classId)
        {
            case 1:  // Warrior
            case 4:  // Rogue
            case 10: // Monk
            case 12: // Demon Hunter
                return BotRole::MELEE_DPS;
                
            case 3:  // Hunter
            case 8:  // Mage
            case 9:  // Warlock
            case 13: // Evoker
                return BotRole::RANGED_DPS;
                
            case 2:  // Paladin
            case 5:  // Priest
            case 7:  // Shaman
            case 11: // Druid
                return BotRole::HEALER;  // Default to healer for hybrids
                
            case 6:  // Death Knight
                return BotRole::MELEE_DPS;
                
            default:
                return BotRole::UNKNOWN;
        }
    }
    
    std::unordered_map<uint32, SpecInfo> _specInfo;
};

// ============================================================================
// GLOBAL ACCESSOR
// ============================================================================

inline IClassRoleResolver& GetClassRoleResolver()
{
    return ClassRoleResolver::Instance();
}

} // namespace Playerbot::Combat
