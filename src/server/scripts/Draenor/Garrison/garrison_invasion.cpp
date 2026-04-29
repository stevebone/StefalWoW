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
#include "Chat.h"
#include "ChatCommand.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "GameTime.h"
#include "Garrison.h"
#include "Item.h"
#include "Log.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptedCreature.h"
#include "TemporarySummon.h"
#include "WorldSession.h"

enum GarrisonInvasionConstants
{
    // Invasion states
    INVASION_STATE_NONE         = 0,
    INVASION_STATE_PENDING      = 1,
    INVASION_STATE_WAVE_1       = 2,
    INVASION_STATE_WAVE_2       = 3,
    INVASION_STATE_WAVE_3       = 4,
    INVASION_STATE_COMPLETE     = 5,

    // Timing
    INVASION_WAVE_INTERVAL      = 60000,    // 60 seconds between waves
    INVASION_SPAWN_INTERVAL     = 3000,     // 3 seconds between spawns within a wave
    INVASION_TOTAL_DURATION     = 600000,   // 10 minutes max

    // Wave sizes
    WAVE_1_COUNT                = 6,
    WAVE_2_COUNT                = 8,
    WAVE_3_COUNT                = 10,

    // Score
    SCORE_PER_KILL              = 10,
    SCORE_SPEED_BONUS           = 50,       // Bonus for clearing wave quickly
    SCORE_GOLD_REWARD           = 1000,     // Gold reward (in copper) per 100 score
    INVASION_SUPPLY_BAG         = 120205,   // Bag of Garrison Supplies (invasion reward)

    // NOTE: This invasion script is framework only. The creature IDs below are
    // approximate starting-point ranges from the Draenor garrison invasion data;
    // real WoD invasions require a populated creature_template + creature_template_addon
    // in the world DB plus phase-aware spawning matching the player's garrison level.
    // The script never inserts into s_activeInvasions in normal play — invasions are
    // triggered manually via the .garrison invasion start GM command for testing.
    //
    // Iron Horde
    NPC_IRON_HORDE_GRUNT        = 82057,
    NPC_IRON_HORDE_BERSERKER    = 82058,
    NPC_IRON_HORDE_WARLORD      = 82059,

    // Shadow Council
    NPC_SHADOW_COUNCIL_WARLOCK  = 82060,
    NPC_SHADOW_COUNCIL_DARK     = 82061,
    NPC_SHADOW_COUNCIL_MASTER   = 82062,

    // Botani
    NPC_BOTANI_TENDER           = 84834,
    NPC_BOTANI_GUARDIAN         = 84835,
    NPC_BOTANI_ANCIENT          = 84836,
};

enum GarrisonInvasionType
{
    INVASION_TYPE_IRON_HORDE        = 0,
    INVASION_TYPE_SHADOW_COUNCIL    = 1,
    INVASION_TYPE_BOTANI            = 2,
    INVASION_TYPE_MAX
};

struct InvasionWaveTemplate
{
    uint32 CreatureEntries[3]; // grunt, elite, boss (per invasion type)
};

static InvasionWaveTemplate const InvasionCreatures[INVASION_TYPE_MAX] =
{
    // Iron Horde
    { { NPC_IRON_HORDE_GRUNT, NPC_IRON_HORDE_BERSERKER, NPC_IRON_HORDE_WARLORD } },
    // Shadow Council
    { { NPC_SHADOW_COUNCIL_WARLOCK, NPC_SHADOW_COUNCIL_DARK, NPC_SHADOW_COUNCIL_MASTER } },
    // Botani
    { { NPC_BOTANI_TENDER, NPC_BOTANI_GUARDIAN, NPC_BOTANI_ANCIENT } },
};

// Spawn positions (relative offsets from garrison center)
// These represent entry points to the garrison where invaders appear
static Position const InvasionSpawnPoints[] =
{
    { 5575.0f, 4568.0f, 136.0f, 3.14f },   // North entrance
    { 5565.0f, 4540.0f, 136.0f, 2.35f },   // Northeast
    { 5585.0f, 4540.0f, 136.0f, 3.93f },   // Northwest
    { 5575.0f, 4530.0f, 136.0f, 1.57f },   // South path
};

// Data structure to track an active invasion
struct GarrisonInvasionData
{
    uint8 State = INVASION_STATE_NONE;
    uint8 InvasionType = INVASION_TYPE_IRON_HORDE;
    uint32 Timer = 0;
    uint32 WaveSpawnTimer = 0;
    uint32 TotalDuration = 0;
    uint32 Score = 0;
    uint32 KillCount = 0;
    uint32 WaveEnemiesSpawned = 0;
    uint32 WaveEnemiesKilled = 0;
    uint32 WaveEnemiesTotal = 0;
    ObjectGuid OwnerGuid;
    std::vector<ObjectGuid> SpawnedCreatures;
};

// Global storage for active invasions (keyed by player GUID)
static std::unordered_map<ObjectGuid, GarrisonInvasionData> s_activeInvasions;

// Invasion enemy AI
struct npc_garrison_invasion_enemy : public ScriptedAI
{
    npc_garrison_invasion_enemy(Creature* creature) : ScriptedAI(creature),
        _ownerGuid() { }

    void SetInvasionOwner(ObjectGuid ownerGuid)
    {
        _ownerGuid = ownerGuid;
    }

    void JustDied(Unit* killer) override
    {
        auto itr = s_activeInvasions.find(_ownerGuid);
        if (itr == s_activeInvasions.end())
            return;

        GarrisonInvasionData& data = itr->second;
        data.KillCount++;
        data.WaveEnemiesKilled++;
        data.Score += SCORE_PER_KILL;

        // Check if wave is cleared
        if (data.WaveEnemiesKilled >= data.WaveEnemiesTotal)
        {
            data.Score += SCORE_SPEED_BONUS;
            // Advance to next wave
            data.Timer = INVASION_WAVE_INTERVAL;
            data.WaveEnemiesKilled = 0;
            data.WaveEnemiesSpawned = 0;

            if (data.State < INVASION_STATE_WAVE_3)
                data.State++;
            else
                data.State = INVASION_STATE_COMPLETE;
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
        {
            // If no target, find the garrison owner and attack
            if (Player* owner = ObjectAccessor::FindPlayer(_ownerGuid))
            {
                if (me->IsWithinDistInMap(owner, 40.0f))
                    AttackStart(owner);
            }
            return;
        }

        me->DoMeleeAttackIfReady();
    }

private:
    ObjectGuid _ownerGuid;
};

// World script to manage invasion timers
class world_garrison_invasion : public WorldScript
{
public:
    world_garrison_invasion() : WorldScript("world_garrison_invasion") { }

    void OnUpdate(uint32 diff) override
    {
        for (auto itr = s_activeInvasions.begin(); itr != s_activeInvasions.end(); )
        {
            GarrisonInvasionData& data = itr->second;

            Player* owner = ObjectAccessor::FindPlayer(data.OwnerGuid);
            if (!owner || !owner->IsInWorld())
            {
                // Clean up invasion if player is gone
                CleanupInvasion(data);
                itr = s_activeInvasions.erase(itr);
                continue;
            }

            data.TotalDuration += diff;

            // Timeout check
            if (data.TotalDuration >= INVASION_TOTAL_DURATION)
            {
                EndInvasion(data, owner, false);
                itr = s_activeInvasions.erase(itr);
                continue;
            }

            if (data.State == INVASION_STATE_COMPLETE)
            {
                EndInvasion(data, owner, true);
                itr = s_activeInvasions.erase(itr);
                continue;
            }

            // Wave spawning logic
            if (data.State >= INVASION_STATE_WAVE_1 && data.State <= INVASION_STATE_WAVE_3)
            {
                uint32 waveSize = GetWaveSizeForState(data.State);
                data.WaveEnemiesTotal = waveSize;

                if (data.WaveEnemiesSpawned < waveSize)
                {
                    data.WaveSpawnTimer += diff;
                    if (data.WaveSpawnTimer >= INVASION_SPAWN_INTERVAL)
                    {
                        data.WaveSpawnTimer = 0;
                        SpawnInvasionEnemy(data, owner);
                    }
                }
            }
            else if (data.State == INVASION_STATE_PENDING)
            {
                data.Timer += diff;
                if (data.Timer >= 5000) // 5 second delay before first wave
                {
                    data.State = INVASION_STATE_WAVE_1;
                    data.Timer = 0;
                }
            }

            ++itr;
        }
    }

private:
    static uint32 GetWaveSizeForState(uint8 state)
    {
        switch (state)
        {
            case INVASION_STATE_WAVE_1: return WAVE_1_COUNT;
            case INVASION_STATE_WAVE_2: return WAVE_2_COUNT;
            case INVASION_STATE_WAVE_3: return WAVE_3_COUNT;
            default: return 0;
        }
    }

    static void SpawnInvasionEnemy(GarrisonInvasionData& data, Player* owner)
    {
        if (!owner->GetMap())
            return;

        // Determine creature entry based on invasion type and wave
        uint32 creatureEntry;
        if (data.InvasionType >= INVASION_TYPE_MAX)
            data.InvasionType = INVASION_TYPE_IRON_HORDE;

        InvasionWaveTemplate const& tmpl = InvasionCreatures[data.InvasionType];

        // Wave 1: all grunts, Wave 2: mix of grunts and elites, Wave 3: elites and boss
        if (data.State == INVASION_STATE_WAVE_1)
            creatureEntry = tmpl.CreatureEntries[0];
        else if (data.State == INVASION_STATE_WAVE_2)
            creatureEntry = (data.WaveEnemiesSpawned % 3 == 0) ? tmpl.CreatureEntries[1] : tmpl.CreatureEntries[0];
        else // Wave 3
        {
            if (data.WaveEnemiesSpawned == 0)
                creatureEntry = tmpl.CreatureEntries[2]; // Boss first
            else
                creatureEntry = (data.WaveEnemiesSpawned % 2 == 0) ? tmpl.CreatureEntries[1] : tmpl.CreatureEntries[0];
        }

        // Pick a spawn point
        uint32 spawnIndex = data.WaveEnemiesSpawned % (sizeof(InvasionSpawnPoints) / sizeof(InvasionSpawnPoints[0]));
        Position spawnPos = InvasionSpawnPoints[spawnIndex];

        // Add some randomization to spawn position
        spawnPos.m_positionX += frand(-3.0f, 3.0f);
        spawnPos.m_positionY += frand(-3.0f, 3.0f);

        if (TempSummon* enemy = owner->SummonCreature(creatureEntry, spawnPos, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 5min))
        {
            // Scale enemy to player level
            enemy->SetLevel(owner->GetLevel());

            // Set invasion owner for kill tracking
            if (npc_garrison_invasion_enemy* ai = dynamic_cast<npc_garrison_invasion_enemy*>(enemy->AI()))
                ai->SetInvasionOwner(data.OwnerGuid);

            data.SpawnedCreatures.push_back(enemy->GetGUID());
            data.WaveEnemiesSpawned++;

            TC_LOG_DEBUG("garrison", "Garrison invasion: Spawned enemy {} (entry {}) for player {} [Wave {}, Enemy {}/{}]",
                enemy->GetGUID().ToString().c_str(), creatureEntry,
                data.OwnerGuid.ToString().c_str(),
                data.State - INVASION_STATE_WAVE_1 + 1,
                data.WaveEnemiesSpawned, data.WaveEnemiesTotal);
        }
    }

    static void EndInvasion(GarrisonInvasionData& data, Player* owner, bool success)
    {
        CleanupInvasion(data);

        TC_LOG_DEBUG("garrison", "Garrison invasion ended for player {}: {} (Score: {}, Kills: {})",
            owner->GetGUID().ToString().c_str(),
            success ? "SUCCESS" : "FAILED",
            data.Score, data.KillCount);

        if (success)
        {
            // Award invasion supply bag
            ItemPosCountVec dest;
            if (owner->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, INVASION_SUPPLY_BAG, 1) == EQUIP_ERR_OK)
            {
                if (Item* item = owner->StoreNewItem(dest, INVASION_SUPPLY_BAG, true))
                    owner->SendNewItem(item, 1, true, false);
            }

            // Award bonus gold based on score
            uint32 goldReward = (data.Score / 100) * SCORE_GOLD_REWARD;
            if (goldReward > 0)
                owner->ModifyMoney(goldReward, false);

            // Award garrison resources based on performance
            uint32 resourceReward = std::min(data.Score / 10, 100u);
            if (resourceReward > 0)
                owner->AddCurrency(824 /*Garrison Resources*/, resourceReward, CurrencyGainSource(7) /*Garrison*/);
        }
    }

    static void CleanupInvasion(GarrisonInvasionData& data)
    {
        // Despawn all remaining invasion creatures
        for (ObjectGuid const& guid : data.SpawnedCreatures)
        {
            if (Creature* creature = ObjectAccessor::GetCreature(*ObjectAccessor::FindPlayer(data.OwnerGuid), guid))
            {
                if (creature->IsAlive())
                    creature->DespawnOrUnsummon();
            }
        }
        data.SpawnedCreatures.clear();
    }
};

// Chat command handler for starting invasions (GM command integration)
using namespace Trinity::ChatCommands;

class commandscript_garrison_invasion : public CommandScript
{
public:
    commandscript_garrison_invasion() : CommandScript("commandscript_garrison_invasion") { }

    std::span<Trinity::ChatCommands::ChatCommandBuilder const> GetCommands() const override
    {
        static ChatCommandTable garrisonInvasionCommandTable =
        {
            { "start",  HandleGarrisonInvasionStart, rbac::RBAC_PERM_COMMAND_DEBUG, Console::No },
            { "stop",   HandleGarrisonInvasionStop,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::No },
            { "score",  HandleGarrisonInvasionScore, rbac::RBAC_PERM_COMMAND_DEBUG, Console::No },
        };

        static ChatCommandTable garrisonCommandTable =
        {
            { "invasion", garrisonInvasionCommandTable },
        };

        static ChatCommandTable commandTable =
        {
            { "garrison", garrisonCommandTable },
        };

        return commandTable;
    }

    static bool HandleGarrisonInvasionStart(ChatHandler* handler, Optional<uint32> type)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        Garrison* garrison = player->GetGarrison();
        if (!garrison)
        {
            handler->SendSysMessage("You don't have a garrison.");
            return true;
        }

        if (s_activeInvasions.count(player->GetGUID()))
        {
            handler->SendSysMessage("An invasion is already in progress.");
            return true;
        }

        uint8 invasionType = INVASION_TYPE_IRON_HORDE;
        if (type && *type < INVASION_TYPE_MAX)
            invasionType = static_cast<uint8>(*type);

        GarrisonInvasionData& data = s_activeInvasions[player->GetGUID()];
        data.State = INVASION_STATE_PENDING;
        data.InvasionType = invasionType;
        data.Timer = 0;
        data.WaveSpawnTimer = 0;
        data.TotalDuration = 0;
        data.Score = 0;
        data.KillCount = 0;
        data.WaveEnemiesSpawned = 0;
        data.WaveEnemiesKilled = 0;
        data.WaveEnemiesTotal = 0;
        data.OwnerGuid = player->GetGUID();

        handler->PSendSysMessage("Garrison invasion started! Type: %u", invasionType);
        return true;
    }

    static bool HandleGarrisonInvasionStop(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        auto itr = s_activeInvasions.find(player->GetGUID());
        if (itr == s_activeInvasions.end())
        {
            handler->SendSysMessage("No active invasion.");
            return true;
        }

        // Cleanup and remove
        for (ObjectGuid const& guid : itr->second.SpawnedCreatures)
            if (Creature* creature = ObjectAccessor::GetCreature(*player, guid))
                creature->DespawnOrUnsummon();

        s_activeInvasions.erase(itr);
        handler->SendSysMessage("Garrison invasion stopped.");
        return true;
    }

    static bool HandleGarrisonInvasionScore(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        auto itr = s_activeInvasions.find(player->GetGUID());
        if (itr == s_activeInvasions.end())
        {
            handler->SendSysMessage("No active invasion.");
            return true;
        }

        GarrisonInvasionData const& data = itr->second;
        handler->PSendSysMessage("Invasion Score: %u | Kills: %u | Wave: %u | Time: %u sec",
            data.Score, data.KillCount, data.State - INVASION_STATE_WAVE_1 + 1, data.TotalDuration / 1000);
        return true;
    }
};

void AddSC_garrison_invasion()
{
    RegisterCreatureAI(npc_garrison_invasion_enemy);
    new world_garrison_invasion();
    new commandscript_garrison_invasion();
}
