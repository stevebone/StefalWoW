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

#include "Define.h"

namespace Scripts::EasternKingdoms::StormwindCity
{
    namespace Creatures
    {
        static constexpr uint32 GeneralMarcusJonathan = 466;
        static constexpr uint32 AysaSpawnedAtGate = 61792;
        static constexpr uint32 JojoSpawnedAtGate = 61793;
        static constexpr uint32 AlynBlack = 61834;
        static constexpr uint32 MoniWiddlesprock = 61836;
        static constexpr uint32 LeriaNightwind = 61837;
        static constexpr uint32 GavinMarlsbury = 61838;
        static constexpr uint32 LucasSevering = 61839;
        static constexpr uint32 Naanae = 61840;
        static constexpr uint32 BrunnGoldenmug = 61841;
        static constexpr uint32 Marty = 61895;
        static constexpr uint32 Josie = 61896;
    }

    namespace Spawns
    {
        static constexpr uint32 StormwindCityGuardOnBridge = 313908;
    }

    namespace Quests
    {
        static constexpr uint32 JoiningTheAlliance = 30987;
    }

    namespace Spells
    {
        static constexpr uint32 Stealth = 86603;
    }

    namespace Positions
    {
        static constexpr Position AysaStormwindCityGeneralMarcus = { -8957.971f, 517.9630f, 96.3556f, 0.7743f };
        static constexpr Position AysaStormwindCityCenter = { -8826.584f, 627.582f, 94.3396f, 0.7692f };
    }

    namespace AreaTriggers
    {
        static constexpr uint32 StormwindTradeDistrictMoniAlyn = 7990;
    }
}
