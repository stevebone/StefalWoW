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

namespace Scripts::TheWanderingIsle::Defines
{
    namespace SpellsQ29422
    {
        static constexpr uint32 spell_fan_the_flames_throw_wood = 109090;
        static constexpr uint32 spell_fan_the_flames_blow_air = 109095;
        static constexpr uint32 spell_fan_the_flames_blow_air_big = 109105;
        static constexpr uint32 spell_fan_the_flames_blow_air_bigger = 109109;
        static constexpr uint32 spell_fan_the_flames_credit = 109107;
    }

    namespace Npcs
    {
        static constexpr uint32 npc_huo_q29422 = 57779;
    }

    namespace Quests
    {
        static constexpr uint32 quest_huo_the_spirit_of_fire = 29422;
    }
}

enum Custom_TheWanderingIsle_Events
{
    THEWANDERINGISLE_EVENT_NONE = 0
};

