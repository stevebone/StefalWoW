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

#include "Followship_bots_utils.h"


constexpr float RAGE_FROM_DAMAGE_COEFF = 0.15f;

namespace FSBPowers
{
    Powers GetBotPowerType(Creature* bot);

    bool SpendPowerPct(Creature* bot, float pct);
    bool SpendManaFlat(Creature* bot, int32 cost);

    bool IsRageUser(Creature* bot);
    bool IsEnergyUser(Creature* bot);
    bool IsFocusUser(Creature* bot);

    void SetBotToEnergy(Creature* bot);
    void SetBotToMana(Creature* bot);
    void SetBotToChi(Creature* bot);

    void GenerateRageFromDamageTaken(Creature* bot, uint32 damage);
    void GenerateRageFromDamageDone(Creature* bot, uint32 damage);
}
