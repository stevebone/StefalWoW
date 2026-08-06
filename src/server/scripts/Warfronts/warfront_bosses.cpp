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

// Final-boss AI for the warfront assault scenarios: High Warlord Volrath (Stromgarde, 82877), Maiev Shadowsong
// (Darkshore Horde, 149098) and Sira Moonwarden (Darkshore Alliance, 146628). Bound via creature_template.ScriptName.
// The boss's death is the win condition: it completes the scenario's final step (paying the reward quest) and then
// notifies WarfrontMgr, which flips zone control and spawns the outdoor world boss (P4).

#include "ScriptMgr.h"
#include "Creature.h"
#include "Map.h"
#include "ScriptedCreature.h"
#include "WarfrontMgr.h"
#include "warfront_common.h"

// Aggro/faction note: this AI is deliberately passive about faction - it does NOT set one. The mustering instance
// script (WarfrontBattle::BattleInstanceScript::PrepareBossForBattle) forces the DEFENDER's warfront faction template
// (2958 Horde / 2959 Alliance), clears the template's un-attackable unit flags and sets REACT_AGGRESSIVE before the
// creature ever ticks. That is required, not cosmetic: 146628 Sira Moonwarden ships as FactionTemplate 2854 (The
// Wardens - FriendGroup 0 / EnemyGroup 0, hostile to nobody) and 82877 High Warlord Volrath ships as 877 (FriendGroup
// 4 = Horde), i.e. friendly to the Horde raid that assaults map 1876. Left alone, neither could be attacked and the
// assault could never be won. With the forced faction the boss is hostile to the attacker, so REACT_AGGRESSIVE +
// MoveInLineOfSight pull it into combat as soon as the raid walks up.
struct npc_warfront_final_boss : public ScriptedAI
{
    npc_warfront_final_boss(Creature* creature) : ScriptedAI(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        // Pull the whole raid into the fight - this is the scenario's climactic encounter.
        DoZoneInCombat();
    }

    void UpdateAI(uint32 /*diff*/) override
    {
        // TODO(P3): minimal-but-real melee encounter - the base ScriptedAI drives auto-attack while in combat.
        // Volrath/Maiev/Sira retail spell kits are a content-tuning follow-up (no reliable server spell data for
        // these entries offline) - see WARFRONTS_STATUS.md.
        UpdateVictim();
    }

    void JustDied(Unit* /*killer*/) override
    {
        Map* map = me->GetMap();
        WarfrontBattle::MapInfo const* info = WarfrontBattle::GetMapInfo(map->GetId());
        if (!info)
            return;   // this template is spawned somewhere that is not a warfront battle map - ignore the death

        // Only the scenario-critical kill (on the final step) wins the assault. This guards against ambient,
        // pre-placed Darkshore bosses of the same entry being killed early in the run.
        InstanceMap* instance = map->ToInstanceMap();
        if (InstanceScenario* scenario = instance ? instance->GetInstanceScenario() : nullptr)
        {
            ScenarioStepEntry const* step = scenario->GetStep();
            if (step && step != scenario->GetLastStep())
                return;
            if (step)
            {
                scenario->SetStepState(step, SCENARIO_STEP_DONE);
                scenario->CompleteStep(step);   // completes the scenario + pays the weekly reward quest
            }
        }

        // Flip zone control (P4): control -> attacking faction, spawn the loser's world boss, reset the challenger's
        // contribution bar, close the queue. OnScenarioComplete is idempotent (it no-ops unless the zone is in SIEGE
        // and the completing team is the challenger).
        sWarfrontMgr->OnScenarioComplete(info->WarfrontId, info->AttackingTeam);
    }
};

void AddSC_warfront_bosses()
{
    RegisterCreatureAI(npc_warfront_final_boss);
}
