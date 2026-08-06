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

// Battle for Stromgarde (Arathi) instanced assault controllers - maps 1876 (Horde) & 1943 (Alliance). These maps
// carry no DB spawns, so the shared BattleInstanceScript (warfront_common.h) drives the scenario steps on a scripted
// timer and musters High Warlord Volrath (82877) at the Stromgarde keep for the final step. See WARFRONTS_DESIGN.md
// §4 and WARFRONTS_STATUS.md for what is real vs scripted-placeholder here.

#include "ScriptMgr.h"
#include "warfront_common.h"

class instance_warfront_arathi : public InstanceMapScript
{
public:
    instance_warfront_arathi(char const* name, uint32 mapId) : InstanceMapScript(name, mapId) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new WarfrontBattle::BattleInstanceScript(map);
    }
};

void AddSC_instance_warfront_arathi()
{
    // One InstanceMapScript per map id (each needs a unique script name matched by instance_template.script).
    new instance_warfront_arathi("instance_warfront_arathi_horde", 1876);
    new instance_warfront_arathi("instance_warfront_arathi_alliance", 1943);
}
