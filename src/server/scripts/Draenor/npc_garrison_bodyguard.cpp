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

#include "ScriptMgr.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "DB2Stores.h"
#include "DB2Structure.h"
#include "Garrison.h"
#include "GarrisonMgr.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "TemporarySummon.h"

namespace
{
    constexpr uint32 GARRISON_BUILDING_TYPE_BARRACKS     = 26;
    constexpr uint32 GARRISON_BUILDING_LEVEL_3           = 3;
    constexpr float  BODYGUARD_FOLLOW_DISTANCE           = 3.0f;
    constexpr float  BODYGUARD_FOLLOW_ANGLE              = float(M_PI); // Behind player
    constexpr uint32 BODYGUARD_REGEN_PCT                 = 5;           // 5% HP regen per tick
    constexpr uint32 BODYGUARD_REGEN_INTERVAL            = 5000;        // 5 seconds
    constexpr float  BODYGUARD_ASSIST_RANGE              = 30.0f;

    // Draenor continent map
    constexpr uint32 MAP_DRAENOR                         = 1116;

    // Bodyguard ability category in GarrAbility DB2
    constexpr uint8  GARR_ABILITY_CATEGORY_BODYGUARD     = 14;
}

// Bodyguard AI script
struct npc_garrison_bodyguard : public ScriptedAI
{
    npc_garrison_bodyguard(Creature* creature) : ScriptedAI(creature),
        _ownerGuid(), _regenTimer(BODYGUARD_REGEN_INTERVAL) { }

    void InitializeAI() override
    {
        ScriptedAI::InitializeAI();
        me->SetReactState(REACT_DEFENSIVE);
    }

    void SetOwner(ObjectGuid ownerGuid)
    {
        _ownerGuid = ownerGuid;
        if (Player* owner = ObjectAccessor::FindPlayer(_ownerGuid))
        {
            me->GetMotionMaster()->MoveFollow(owner, BODYGUARD_FOLLOW_DISTANCE, BODYGUARD_FOLLOW_ANGLE);
            me->SetFaction(owner->GetFaction());
        }
    }

    void UpdateAI(uint32 diff) override
    {
        // Regenerate health
        if (_regenTimer <= diff)
        {
            _regenTimer = BODYGUARD_REGEN_INTERVAL;
            if (me->IsAlive() && me->GetHealth() < me->GetMaxHealth())
            {
                uint64 regen = me->GetMaxHealth() * BODYGUARD_REGEN_PCT / 100;
                me->SetHealth(std::min(me->GetHealth() + regen, me->GetMaxHealth()));
            }
        }
        else
            _regenTimer -= diff;

        Player* owner = ObjectAccessor::FindPlayer(_ownerGuid);
        if (!owner || !owner->IsInWorld() || owner->GetMapId() != me->GetMapId())
        {
            me->DespawnOrUnsummon();
            return;
        }

        // Assist owner in combat
        if (!me->IsInCombat() && owner->IsInCombat())
        {
            if (Unit* target = owner->GetVictim())
            {
                if (me->IsWithinDistInMap(target, BODYGUARD_ASSIST_RANGE))
                {
                    AttackStart(target);
                }
            }
        }

        if (!UpdateVictim())
        {
            // Return to following if not in combat
            if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() != FOLLOW_MOTION_TYPE)
                me->GetMotionMaster()->MoveFollow(owner, BODYGUARD_FOLLOW_DISTANCE, BODYGUARD_FOLLOW_ANGLE);
            return;
        }

        me->DoMeleeAttackIfReady();
    }

    void JustDied(Unit* /*killer*/) override
    {
        // Bodyguards respawn quickly - they are effectively unkillable
        me->SetRespawnTime(10); // 10 second respawn
    }

    void JustReachedHome() override
    {
        if (Player* owner = ObjectAccessor::FindPlayer(_ownerGuid))
        {
            me->GetMotionMaster()->MoveFollow(owner, BODYGUARD_FOLLOW_DISTANCE, BODYGUARD_FOLLOW_ANGLE);
            me->SetFaction(owner->GetFaction());
        }
    }

private:
    ObjectGuid _ownerGuid;
    uint32 _regenTimer;
};

// Helper: check if a follower has a bodyguard-type trait
static bool HasBodyguardTrait(Garrison::Follower const& follower)
{
    for (GarrAbilityEntry const* ability : follower.PacketInfo.AbilityID)
    {
        if (!ability)
            continue;
        // Bodyguard abilities have the TRAIT flag and belong to the bodyguard category
        if ((ability->Flags & GARRISON_ABILITY_FLAG_TRAIT) &&
            ability->GarrAbilityCategoryID == GARR_ABILITY_CATEGORY_BODYGUARD)
            return true;
    }
    return false;
}

// Helper: find the bodyguard follower assigned to barracks
static Garrison::Follower const* FindBodyguardFollower(Garrison* garrison)
{
    // Find active Barracks Level 3 building
    bool hasBarracksL3 = false;
    uint32 barracksPlotBuildingId = 0;

    for (Garrison::Plot* plot : garrison->GetPlots())
    {
        if (!plot->BuildingInfo.PacketInfo || !plot->BuildingInfo.PacketInfo->Active)
            continue;

        GarrBuildingEntry const* building = sGarrBuildingStore.LookupEntry(
            plot->BuildingInfo.PacketInfo->GarrBuildingID);
        if (!building)
            continue;

        if (building->BuildingType == GARRISON_BUILDING_TYPE_BARRACKS &&
            building->UpgradeLevel >= GARRISON_BUILDING_LEVEL_3)
        {
            hasBarracksL3 = true;
            barracksPlotBuildingId = building->ID;
            break;
        }
    }

    if (!hasBarracksL3)
        return nullptr;

    // Find a follower assigned to the barracks that has the bodyguard trait
    for (auto const& [dbId, follower] : garrison->GetFollowerMap())
    {
        if (follower.PacketInfo.CurrentBuildingID != barracksPlotBuildingId)
            continue;

        if (follower.PacketInfo.FollowerStatus & FOLLOWER_STATUS_INACTIVE)
            continue;

        if (follower.PacketInfo.CurrentMissionID != 0)
            continue;

        if (HasBodyguardTrait(follower))
            return &follower;
    }

    return nullptr;
}

static bool IsDraenorOutdoorMap(uint32 mapId)
{
    // Main Draenor continent only - garrison maps are instanced and excluded
    return mapId == MAP_DRAENOR;
}

// Player script to manage bodyguard spawning/despawning
class player_garrison_bodyguard : public PlayerScript
{
public:
    player_garrison_bodyguard() : PlayerScript("player_garrison_bodyguard") { }

    void OnMapChanged(Player* player) override
    {
        UpdateBodyguard(player);
    }

private:
    void UpdateBodyguard(Player* player)
    {
        // Despawn existing bodyguard summon if any
        DespawnBodyguard(player);

        if (!IsDraenorOutdoorMap(player->GetMapId()))
            return;

        Garrison* garrison = player->GetGarrison();
        if (!garrison || garrison->GetType() != GARRISON_TYPE_GARRISON)
            return;

        Garrison::Follower const* bodyguardFollower = FindBodyguardFollower(garrison);
        if (!bodyguardFollower)
            return;

        SpawnBodyguard(player, bodyguardFollower);
    }

    void SpawnBodyguard(Player* player, Garrison::Follower const* follower)
    {
        GarrFollowerEntry const* followerEntry = sGarrFollowerStore.LookupEntry(follower->PacketInfo.GarrFollowerID);
        if (!followerEntry)
            return;

        GarrisonFactionIndex faction = Garrison::GetFaction(player->GetTeam());
        uint32 creatureId = faction == GARRISON_FACTION_INDEX_HORDE ?
            followerEntry->HordeCreatureID : followerEntry->AllianceCreatureID;

        if (!creatureId)
            return;

        Position spawnPos = *player;
        spawnPos.RelocateOffset({ -2.0f, 0.0f, 0.0f, 0.0f });

        if (TempSummon* bodyguard = player->SummonCreature(creatureId, spawnPos, TEMPSUMMON_MANUAL_DESPAWN))
        {
            // Scale up the bodyguard's health to be tanky
            bodyguard->SetMaxHealth(player->GetMaxHealth() * 5);
            bodyguard->SetFullHealth();
            bodyguard->SetLevel(player->GetLevel());

            // Set the AI owner
            if (npc_garrison_bodyguard* ai = dynamic_cast<npc_garrison_bodyguard*>(bodyguard->AI()))
                ai->SetOwner(player->GetGUID());
            else
            {
                // Fallback: just make it follow
                bodyguard->GetMotionMaster()->MoveFollow(player, BODYGUARD_FOLLOW_DISTANCE, BODYGUARD_FOLLOW_ANGLE);
                bodyguard->SetFaction(player->GetFaction());
            }
        }
    }

    void DespawnBodyguard(Player* player)
    {
        // Iterate the player's controlled units and despawn any bodyguard-type summons
        // The TempSummon system tracks summoned creatures via the control list
        std::list<uint32> entries = GetBodyguardCreatureEntries(player);

        std::list<TempSummon*> bodyguards;
        for (uint32 entry : entries)
            player->GetAllMinionsByEntry(bodyguards, entry);

        for (TempSummon* summon : bodyguards)
            summon->UnSummon();
    }

    // Returns a list of creature entry IDs that could be bodyguard NPCs for this player
    static std::list<uint32> GetBodyguardCreatureEntries(Player* player)
    {
        std::list<uint32> entries;

        Garrison* garrison = player->GetGarrison();
        if (!garrison)
            return entries;

        for (auto const& [dbId, follower] : garrison->GetFollowerMap())
        {
            if (!HasBodyguardTrait(follower))
                continue;

            GarrFollowerEntry const* followerEntry = sGarrFollowerStore.LookupEntry(follower.PacketInfo.GarrFollowerID);
            if (!followerEntry)
                continue;

            if (followerEntry->HordeCreatureID)
                entries.push_back(followerEntry->HordeCreatureID);
            if (followerEntry->AllianceCreatureID)
                entries.push_back(followerEntry->AllianceCreatureID);
        }

        return entries;
    }
};

void AddSC_npc_garrison_bodyguard()
{
    RegisterCreatureAI(npc_garrison_bodyguard);
    new player_garrison_bodyguard();
}
