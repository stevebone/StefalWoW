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

#include "LFGRoleDetector.h"
#include "Player.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "LFGMgr.h"
#include "Log.h"
#include "SharedDefines.h"

LFGRoleDetector::LFGRoleDetector()
{
}

LFGRoleDetector::~LFGRoleDetector()
{
}

LFGRoleDetector* LFGRoleDetector::instance()
{
    static LFGRoleDetector instance;
    return &instance;
}

uint8 LFGRoleDetector::DetectPlayerRole(Player* player)
{
    if (!player)
        return lfg::PLAYER_ROLE_NONE;

    // Priority 1: Detect from specialization
    uint8 specRole = DetectRoleFromSpec(player);
    if (specRole != lfg::PLAYER_ROLE_NONE)
        return specRole;

    // Priority 2: Detect from equipped gear
    uint8 gearRole = DetectRoleFromGear(player);
    if (gearRole != lfg::PLAYER_ROLE_NONE)
        return gearRole;

    // Priority 3: Use class default
    uint8 classRole = GetDefaultRoleForClass(player->GetClass());
    return classRole;
}

uint8 LFGRoleDetector::DetectBotRole(Player* bot)
{
    if (!bot)
        return lfg::PLAYER_ROLE_NONE;

    // For bots, prioritize spec detection as they should have proper specs set
    uint8 role = DetectRoleFromSpec(bot);
    if (role != lfg::PLAYER_ROLE_NONE)
        return role;

    // Fallback to standard detection
    return DetectPlayerRole(bot);
}

bool LFGRoleDetector::CanPerformRole(Player* player, uint8 role)
{
    if (!player)
        return false;

    uint8 playerClass = player->GetClass();
    switch (role)
    {
        case lfg::PLAYER_ROLE_TANK:
            return ClassCanTank(playerClass);
        case lfg::PLAYER_ROLE_HEALER:
            return ClassCanHeal(playerClass);
        case lfg::PLAYER_ROLE_DAMAGE:
            return ClassCanDPS(playerClass);
        default:
            return false;
    }
}

uint8 LFGRoleDetector::GetBestRoleForPlayer(Player* player)
{
    if (!player)
        return lfg::PLAYER_ROLE_DAMAGE; // Default to DPS

    // Try spec-based detection first
    uint8 specRole = DetectRoleFromSpec(player);
    if (specRole == lfg::PLAYER_ROLE_TANK || specRole == lfg::PLAYER_ROLE_HEALER)
        return specRole; // Tank and healer specs are definitive

    // For DPS or unknown, check gear to see if they might be tank/healer
    uint32 tankScore = CalculateTankScore(player);
    uint32 healerScore = CalculateHealerScore(player);
    uint32 dpsScore = CalculateDPSScore(player);
    // Find highest score
    if (tankScore > healerScore && tankScore > dpsScore && ClassCanTank(player->GetClass()))
        return lfg::PLAYER_ROLE_TANK;

    if (healerScore > tankScore && healerScore > dpsScore && ClassCanHeal(player->GetClass()))
        return lfg::PLAYER_ROLE_HEALER;

    return lfg::PLAYER_ROLE_DAMAGE; // Default to DPS
}

uint8 LFGRoleDetector::GetAllPerformableRoles(Player* player)
{
    if (!player)
        return lfg::PLAYER_ROLE_NONE;

    uint8 roles = lfg::PLAYER_ROLE_NONE;
    uint8 playerClass = player->GetClass();
    if (ClassCanTank(playerClass))
        roles |= lfg::PLAYER_ROLE_TANK;

    if (ClassCanHeal(playerClass))
        roles |= lfg::PLAYER_ROLE_HEALER;

    if (ClassCanDPS(playerClass))
        roles |= lfg::PLAYER_ROLE_DAMAGE;

    return roles;
}

uint8 LFGRoleDetector::GetRoleFromSpecialization(Player* player, uint32 specId)
{
    if (!player || specId == 0)
        return lfg::PLAYER_ROLE_NONE;

    if (IsTankSpec(specId))
        return lfg::PLAYER_ROLE_TANK;

    if (IsHealerSpec(specId))
        return lfg::PLAYER_ROLE_HEALER;

    if (IsDPSSpec(specId))
        return lfg::PLAYER_ROLE_DAMAGE;

    return lfg::PLAYER_ROLE_NONE;
}

uint8 LFGRoleDetector::DetectRoleFromSpec(Player* player)
{
    if (!player)
        return lfg::PLAYER_ROLE_NONE;

    // Get active specialization
    // GetPrimarySpecialization returns ChrSpecialization which is a numeric type
    uint32 specId = static_cast<uint32>(player->GetPrimarySpecialization());
    if (specId == 0)
        return lfg::PLAYER_ROLE_NONE;

    return GetRoleFromSpecialization(player, specId);
}

uint8 LFGRoleDetector::DetectRoleFromGear(Player* player)
{
    if (!player)
        return lfg::PLAYER_ROLE_NONE;

    uint32 tankScore = CalculateTankScore(player);
    uint32 healerScore = CalculateHealerScore(player);
    uint32 dpsScore = CalculateDPSScore(player);

    // Require significant score difference to make a determination
    const uint32 THRESHOLD = 100;

    uint8 playerClass = player->GetClass();
    // Check for tank gear
    if (tankScore > healerScore + THRESHOLD && tankScore > dpsScore + THRESHOLD && ClassCanTank(playerClass))
        return lfg::PLAYER_ROLE_TANK;

    // Check for healer gear
    if (healerScore > tankScore + THRESHOLD && healerScore > dpsScore + THRESHOLD && ClassCanHeal(playerClass))
        return lfg::PLAYER_ROLE_HEALER;

    // Check for DPS gear
    if (dpsScore > tankScore + THRESHOLD && dpsScore > healerScore + THRESHOLD)
        return lfg::PLAYER_ROLE_DAMAGE;

    // Scores too close to determine
    return lfg::PLAYER_ROLE_NONE;
}

uint8 LFGRoleDetector::GetDefaultRoleForClass(uint8 playerClass)
{
    switch (playerClass)
    {
        case CLASS_WARRIOR:
        case CLASS_PALADIN:
        case CLASS_DEATH_KNIGHT:
        case CLASS_MONK:
        case CLASS_DEMON_HUNTER:
            return lfg::PLAYER_ROLE_TANK; // Hybrid classes default to tank

        case CLASS_PRIEST:
        case CLASS_SHAMAN:
        case CLASS_DRUID:
            return lfg::PLAYER_ROLE_HEALER; // Healing classes default to healer
        case CLASS_ROGUE:
        case CLASS_HUNTER:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
            return lfg::PLAYER_ROLE_DAMAGE; // Pure DPS classes

        case CLASS_EVOKER:
            return lfg::PLAYER_ROLE_HEALER; // Evoker defaults to healer (Preservation)

        default:
            return lfg::PLAYER_ROLE_DAMAGE; // Safe default
    }
}

uint32 LFGRoleDetector::CalculateTankScore(Player* player)
{
    if (!player)
        return 0;

    uint32 score = 0;

    // Base score from stats
    score += player->GetStat(STAT_STAMINA) / 2;           // Stamina is primary tank stat
    score += player->GetArmor() / 100;                     // Armor value
    score += player->GetRatingBonusValue(CR_DODGE) * 2;   // Dodge rating
    score += player->GetRatingBonusValue(CR_PARRY) * 2;   // Parry rating
    score += player->GetRatingBonusValue(CR_BLOCK) * 2;   // Block rating

    // Bonus for shield equipped
    Item* offhand = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
    if (offhand && offhand->GetTemplate()->GetInventoryType() == INVTYPE_SHIELD)
    {
        score += 150; // Significant bonus for shield
    }

    // Check for tank-specific set bonuses or items
    // This is a simplified check - real implementation would query item properties

    return score;
}

uint32 LFGRoleDetector::CalculateHealerScore(Player* player)
{
    if (!player)
        return 0;

    // Only healing classes can get healer score
    if (!ClassCanHeal(player->GetClass()))
        return 0;

    uint32 score = 0;

    // Base score from stats
    score += player->GetStat(STAT_INTELLECT) / 2;         // Intellect for spell power
    score += player->GetStat(STAT_SPIRIT) / 3;            // WoW 12.0: Spirit re-added for mana regen
    score += player->GetRatingBonusValue(CR_HASTE_SPELL) * 2; // Spell haste
    score += player->GetRatingBonusValue(CR_CRIT_SPELL);      // Spell crit
    score += player->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_ALL) / 2; // Spell power

    // Bonus for healing-specific gear
    // In real implementation, would check for +healing stats on gear

    return score;
}

uint32 LFGRoleDetector::CalculateDPSScore(Player* player)
{
    if (!player)
        return 0;

    uint32 score = 0;

    // Check if player is primarily physical or spell DPS
    uint32 physicalScore = 0;
    uint32 spellScore = 0;

    // Physical DPS score
    physicalScore += player->GetStat(STAT_STRENGTH);
    physicalScore += player->GetStat(STAT_AGILITY);
    physicalScore += player->GetRatingBonusValue(CR_CRIT_MELEE) * 2;
    physicalScore += player->GetRatingBonusValue(CR_HASTE_MELEE) * 2;
    physicalScore += player->GetTotalAttackPowerValue(BASE_ATTACK) / 5;

    // Spell DPS score
    spellScore += player->GetStat(STAT_INTELLECT);
    spellScore += player->GetRatingBonusValue(CR_CRIT_SPELL) * 2;
    spellScore += player->GetRatingBonusValue(CR_HASTE_SPELL) * 2;
    spellScore += player->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_SPELL) / 2;

    // Use higher of the two
    score = std::max(physicalScore, spellScore);

    return score;
}

bool LFGRoleDetector::ClassCanTank(uint8 playerClass)
{
    switch (playerClass)
    {
        case CLASS_WARRIOR:
        case CLASS_PALADIN:
        case CLASS_DEATH_KNIGHT:
        case CLASS_DRUID:
        case CLASS_MONK:
        case CLASS_DEMON_HUNTER:
            return true;
        default:
            return false;
    }
}

bool LFGRoleDetector::ClassCanHeal(uint8 playerClass)
{
    switch (playerClass)
    {
        case CLASS_PRIEST:
        case CLASS_PALADIN:
        case CLASS_SHAMAN:
        case CLASS_DRUID:
        case CLASS_MONK:
        case CLASS_EVOKER:
            return true;
        default:
            return false;
    }
}

bool LFGRoleDetector::ClassCanDPS(uint8 playerClass)
{
    // All classes can DPS in some form
    return true;
}

bool LFGRoleDetector::IsTankSpec(uint32 specId)
{
    // Tank specialization IDs (from ChrSpecialization.dbc/db2)
    // These IDs are for modern WoW - adjust based on expansion
    switch (specId)
    {
        // Warrior - Protection
        case 73:
        // Paladin - Protection
        case 66:
        // Death Knight - Blood
        case 250:
        // Druid - Guardian
        case 104:
        // Monk - Brewmaster
        case 268:
        // Demon Hunter - Vengeance
        case 581:
            return true;
        default:
            return false;
    }
}

bool LFGRoleDetector::IsHealerSpec(uint32 specId)
{
    // Healer specialization IDs (from ChrSpecialization.dbc/db2)
    switch (specId)
    {
        // Priest - Discipline
        case 256:
        // Priest - Holy
        case 257:
        // Paladin - Holy
        case 65:
        // Shaman - Restoration
        case 264:
        // Druid - Restoration
        case 105:
        // Monk - Mistweaver
        case 270:
        // Evoker - Preservation
        case 1468:
        // Evoker - Augmentation (support, counted as healer for simplicity)
        case 1473:
            return true;
        default:
            return false;
    }
}

bool LFGRoleDetector::IsDPSSpec(uint32 specId)
{
    // If not tank or healer, it's DPS
    return !IsTankSpec(specId) && !IsHealerSpec(specId);
}
