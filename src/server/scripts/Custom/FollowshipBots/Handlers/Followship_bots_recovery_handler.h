/*
 * This file is part of the Stefal WoW Project.
 * It is designed to work exclusively with the TrinityCore framework.
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
 * This code is provided for personal and educational use within the
 * Stefal WoW Project. It is not intended for commercial distribution,
 * resale, or any form of monetization.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Followship_bots_config.h"
#include "Followship_bots_utils.h"
#include "Followship_bots_paladin.h"
#include "Followship_bots_priest.h"



constexpr float BOT_RECOVERY_HP_PCT = 50.f;
constexpr float BOT_RECOVERY_MP_PCT = 50.f;




enum class BotRecoveryIntent
{
    None,
    RecoverHealth,
    RecoverMana,
    RecoverHealthAndMana
};

enum class BotRecoverAction
{
    None,
    DrinkEat,
    Drink,
    ClassDrinkEat,
    Eat,
    Recuperate,
    ClassHeal,
    RaceHeal
};

enum BotDrinkEatType
{
    DRINK = 0,
    EAT = 1,
    DRINK_EAT = 2,
    CLASS_DRINK_EAT = 3,
    RECUPERATE = 4,
    CLASS_HEAL = 5,
    RACE_HEAL = 6
};

struct DrinkFoodPerLevel
{
    uint8 minLevel;
    uint8 maxLevel;
    int32 value;
};

// min level, max level, value
static constexpr DrinkFoodPerLevel DrinkFoodTable[] =
{
    { 1, 10,  10 }, // level 1-10
    { 11, 20, 25 }, // level 11-20
    { 21, 30, 45 },
    { 31, 40, 90 },
    { 41, 50, 120 },
    { 51, 60, 220 },
};

namespace FSBRecovery
{
    BotRecoveryIntent DetermineRecoveryIntent(Creature* bot);
    BotRecoverAction GetRecoveryAction(Creature* bot, BotRecoveryIntent intent);

    bool TryRecoverAction(Creature* bot, BotRecoverAction action, uint32& globalCooldown);

    bool TryOOCRecovery(Creature* bot, BotRecoveryIntent intent, uint32& globalCooldown);

    // Gets drink amount of mana regen per level
    int32 GetDrinkFoodPerLevel(uint8 level, DrinkFoodPerLevel const* table, size_t tableSize);

    // Wrapper for GetDrinkManaRegenPerLevel
    int32 GetDrinkFood(uint16 level);

    bool BotHasRecoveryActive(Creature* bot);
    void BotCancelRecoveryAtFull(Creature* bot);
}
