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

constexpr float BOT_IC_THRESHOLD_POTION_MP = 20.0f;
constexpr float BOT_IC_THRESHOLD_POTION_HP = 10.0f;

namespace FSBIC
{
    bool BotICActions(Creature* bot, bool& botManaPotionUsed, bool& botHealthPotionUsed, uint32& globalCooldown, bool& botCastedCombatBuffs, const std::vector<Unit*>& botGroup);
    bool BotICPotions(Creature* bot, bool& botManaPotionUsed, bool& botHealthPotionUsed);
    bool BotICInitialBuffs(Creature* bot, uint32 globalCooldown, bool& botCastedCombatBuffs, const std::vector<Unit*>& botGroup);
    bool BotICResurrect(Creature* bot, ObjectGuid& resTargetGuid);
    bool BotICMeleeMode(Creature* bot);
    bool BotICTryDispel(Creature* bot);
    bool BotICTryOffensiveDispel(Creature* bot);
    bool BotICHealSelf(Creature* bot);
    bool BotICHealGroup(Creature* bot);
    bool BotICTryOffensiveSpell(Creature* bot);

    bool BotICRandomActions(Creature* bot);
    bool BotICAnnounceLowManaHealth(Creature* bot);

}
