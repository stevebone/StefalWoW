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

// Battle for Darkshore instanced assault controllers - maps 2105 (Alliance) & 2111 (Horde). Unlike Arathi these maps
// already carry ambient spawns (Alliance: ~272, Horde: ~567, incl. the final bosses), but the shared
// BattleInstanceScript (warfront_common.h) still drives the server-authored scenario steps on a scripted timer and
// musters the scenario-critical final boss - Sira Moonwarden (146628) on the Alliance path, Maiev Shadowsong (149098)
// on the Horde path - for the final step. See WARFRONTS_DESIGN.md §4 and WARFRONTS_STATUS.md.

#include "ScriptMgr.h"
#include "warfront_common.h"

class instance_warfront_darkshore : public InstanceMapScript
{
public:
    instance_warfront_darkshore(char const* name, uint32 mapId) : InstanceMapScript(name, mapId) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new WarfrontBattle::BattleInstanceScript(map);
    }
};

void AddSC_instance_warfront_darkshore()
{
    // One InstanceMapScript per map id (each needs a unique script name matched by instance_template.script).
    new instance_warfront_darkshore("instance_warfront_darkshore_alliance", 2105);
    new instance_warfront_darkshore("instance_warfront_darkshore_horde", 2111);
}
