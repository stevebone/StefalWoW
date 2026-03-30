/*
 * DoomCore Scripts - Shadow Enclave Delve (Map 2952)
 * Instance script with Scenario 3154 progression.
 *
 * Scenario 3154 Steps (from DB2 ScenarioStep + sniff):
 * Step 0 (16032): Pursue Antenorian      ? First combat kill
 * Step 1 (16028): Destroy Void Focuses ×2 ? Void Focus (250266) death
 * Step 2 (16029): Stop Rituals ×3         ? Darkcaller deaths (3 unique)
 * Step 3 (16030): Slay Devoted ×3         ? Antenorian's Devoted (250275) deaths
 * Step 4 (16031): Slay Lord Antenorian    ? OnUnitDeath
 *
 * Flow:
 *   Enter ? clear trash ? destroy 2 Void Focuses ? kill 3 Darkcallers ?
 *   kill 3 Antenorian's Devoted ? kill Lord Antenorian ?
 *   loot chests spawn ? Leave-O-Bot flies in after 15s
 */

#include "DelveData.h"
#include "ScriptMgr.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "DB2Stores.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "InstanceScript.h"
#include "Log.h"
#include "Map.h"
#include "MotionMaster.h"
#include "Player.h"
#include "ReputationMgr.h"
#include "InstanceScenario.h"
#include "ScenarioMgr.h"
#include "TemporarySummon.h"

enum ShadowEnclaveData
{
    MAP_SHADOW_ENCLAVE              = 2952,
    SCENARIO_SHADOW_ENCLAVE         = 3154,
    SCENARIO_DELVE_REWARD           = 3424,
    FACTION_DELVES_SEASON_1         = 2742,
    FACTION_COMPANION_XP            = 2744,
    DELVE_REPUTATION_REWARD         = 200,
    COMPANION_XP_REWARD             = 100,

    // Creatures
    NPC_VOID_FOCUS                  = 250266,
    NPC_DARKCALLER_LYSAILLE         = 250242,
    NPC_DARKCALLER_CIMBERON         = 251899,
    NPC_DARKCALLER_THELAMORN        = 251898,
    NPC_ANTENORIAN_DEVOTED          = 250275,
    NPC_LORD_ANTENORIAN             = 246717,
    NPC_LEAVE_O_BOT                 = 205496,
    NPC_VALEERA_COMPANION           = 248567,
    NPC_BLADESWORN_CULTIST          = 246352,
    NPC_SHADOWSPAWN                 = 246348,
    NPC_SHADOWBURN_VORTEX           = 246357,
    NPC_EYE_OF_ANTENORIAN           = 257834,

    // GameObjects
    GO_RITUAL_FOCUS                 = 619141,
    GO_LOOT_CHEST_1                 = 584517,
    GO_LOOT_CHEST_2                 = 584519,

    // Instance data indices
    DATA_PURSUIT_STARTED            = 0,
    DATA_VOID_FOCUSES_KILLED        = 1,
    DATA_RITUALS_STOPPED            = 2,
    DATA_DEVOTED_KILLED             = 3,
    DATA_ANTENORIAN_KILLED          = 4,
};

// Boss position (from sniff)
static constexpr float ANTENORIAN_X = 127.458f;
static constexpr float ANTENORIAN_Y = -56.413f;
static constexpr float ANTENORIAN_Z = 215.583f;
static constexpr float ANTENORIAN_O = 5.563f;

// Loot chest positions (from sniff - marked as temporary spawns)
static constexpr float CHEST1_X = 166.078f;
static constexpr float CHEST1_Y = -101.342f;
static constexpr float CHEST1_Z = 218.528f;
static constexpr float CHEST1_O = 1.518f;

static constexpr float CHEST2_X = 173.568f;
static constexpr float CHEST2_Y = -97.356f;
static constexpr float CHEST2_Z = 218.654f;
static constexpr float CHEST2_O = 2.286f;

// Leave-O-Bot position (from sniff)
static constexpr float LEAVEBOT_X = 151.821f;
static constexpr float LEAVEBOT_Y = -65.059f;
static constexpr float LEAVEBOT_Z = 226.183f;
static constexpr float LEAVEBOT_O = 4.674f;

// Companion spawn position (from sniff - Map 2952)
static constexpr float VALEERA_X = -31.575f;
static constexpr float VALEERA_Y = 222.411f;
static constexpr float VALEERA_Z = 264.518f;
static constexpr float VALEERA_O = 3.820f;

static constexpr char const* ShadowEnclaveScriptName = "instance_shadow_enclave_delve";

class instance_shadow_enclave_delve : public InstanceMapScript
{
    public:
        instance_shadow_enclave_delve() : InstanceMapScript(ShadowEnclaveScriptName, MAP_SHADOW_ENCLAVE) { }

        struct instance_shadow_enclave_delve_InstanceMapScript : public InstanceScript
        {
            instance_shadow_enclave_delve_InstanceMapScript(InstanceMap* map) : InstanceScript(map)
            {
                _voidFocusesKilled = 0;
                _ritualsCompleted = 0;
                _devotedKilled = 0;
                _selectedTier = 0;
                _pursuitStarted = false;
                _bossKilled = false;
                _companionSpawned = false;
            }

            void OnPlayerEnter(Player* player) override
            {
                InstanceScript::OnPlayerEnter(player);

                auto& pending = GetPendingDelveEntries();
                auto itr = pending.find(player->GetGUID());
                if (itr != pending.end())
                {
                    _selectedTier = itr->second.TierIndex;
                    pending.erase(itr);
                    TC_LOG_DEBUG("scripts.delve", "Shadow Enclave: Player {} entered at Tier {} (0-based)",
                        player->GetName(), _selectedTier);
                }

                player->SendUpdateWorldState(WS_DELVE_TIER, _selectedTier + 1);

                DelveInfo const* info = GetDelveInfoByMap(instance->GetId());
                if (info)
                    player->SendUpdateWorldState(WS_DELVE_LFG_DUNGEONS_ID, info->LfgDungeonsId);

                player->SetDelveData(
                    instance->GetId(),
                    TIER_SPELL_IDS[_selectedTier],
                    0, 0
                );

                if (!_companionSpawned)
                {
                    _companionSpawned = true;
                    instance->SummonCreature(NPC_VALEERA_COMPANION,
                        { VALEERA_X, VALEERA_Y, VALEERA_Z, VALEERA_O });
                }
            }

            void OnCreatureCreate(Creature* creature) override
            {
                InstanceScript::OnCreatureCreate(creature);

                switch (creature->GetEntry())
                {
                    case NPC_LORD_ANTENORIAN:
                        _antenorianGUID = creature->GetGUID();
                        break;
                    default:
                        break;
                }
            }

            void OnUnitDeath(Unit* unit) override
            {
                InstanceScript::OnUnitDeath(unit);

                if (!unit || !unit->ToCreature())
                    return;

                // Step 0: First combat kill starts the pursuit
                if (!_pursuitStarted)
                {
                    uint32 entry = unit->GetEntry();
                    if (entry == NPC_BLADESWORN_CULTIST || entry == NPC_SHADOWSPAWN ||
                        entry == NPC_SHADOWBURN_VORTEX || entry == NPC_EYE_OF_ANTENORIAN)
                    {
                        _pursuitStarted = true;
                        TC_LOG_DEBUG("scripts.delve", "Shadow Enclave: First combat kill - pursuit started");
                        ForceCompleteCurrentScenarioStep();
                    }
                }

                switch (unit->GetEntry())
                {
                    case NPC_VOID_FOCUS:
                    {
                        if (_voidFocusesKilled < 2)
                        {
                            _voidFocusesKilled++;
                            TC_LOG_DEBUG("scripts.delve", "Shadow Enclave: Void Focus destroyed ({}/2)", _voidFocusesKilled);
                            if (_voidFocusesKilled >= 2)
                                ForceCompleteCurrentScenarioStep();
                        }
                        break;
                    }
                    case NPC_DARKCALLER_LYSAILLE:
                    case NPC_DARKCALLER_CIMBERON:
                    case NPC_DARKCALLER_THELAMORN:
                    {
                        if (_ritualsCompleted < 3)
                        {
                            _ritualsCompleted++;
                            TC_LOG_DEBUG("scripts.delve", "Shadow Enclave: Darkcaller killed - ritual stopped ({}/3)", _ritualsCompleted);

                            DespawnNearbyRitualFocuses(unit);

                            if (_ritualsCompleted >= 3)
                                ForceCompleteCurrentScenarioStep();
                        }
                        break;
                    }
                    case NPC_ANTENORIAN_DEVOTED:
                    {
                        if (_devotedKilled < 3)
                        {
                            _devotedKilled++;
                            TC_LOG_DEBUG("scripts.delve", "Shadow Enclave: Antenorian's Devoted slain ({}/3)", _devotedKilled);
                            if (_devotedKilled >= 3)
                                ForceCompleteCurrentScenarioStep();
                        }
                        break;
                    }
                    case NPC_LORD_ANTENORIAN:
                    {
                        if (!_bossKilled)
                        {
                            _bossKilled = true;
                            TC_LOG_DEBUG("scripts.delve", "Shadow Enclave: Lord Antenorian killed - scenario complete");

                            // Complete Step 4 (Kill Boss)
                            ForceCompleteCurrentScenarioStep();

                            // Transition to reward scenario (3424)
                            TransitionToRewardScenario();
                            RewardDelveReputation();

                            // Spawn loot chests
                            SpawnGameObject(GO_LOOT_CHEST_1, { CHEST1_X, CHEST1_Y, CHEST1_Z, CHEST1_O });
                            SpawnGameObject(GO_LOOT_CHEST_2, { CHEST2_X, CHEST2_Y, CHEST2_Z, CHEST2_O });

                            // Spawn Leave-O-Bot after 15 seconds
                            _events.ScheduleEvent(1, 15s);
                        }
                        break;
                    }
                    default:
                        break;
                }
            }

            void Update(uint32 diff) override
            {
                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case 1:
                        {
                            TC_LOG_DEBUG("scripts.delve", "Shadow Enclave: Spawning Leave-O-Bot");
                            instance->SummonCreature(NPC_LEAVE_O_BOT,
                                { LEAVEBOT_X, LEAVEBOT_Y, LEAVEBOT_Z, LEAVEBOT_O });
                            break;
                        }
                        default:
                            break;
                    }
                }
            }

            uint32 GetData(uint32 type) const override
            {
                switch (type)
                {
                    case DATA_PURSUIT_STARTED:      return _pursuitStarted ? 1 : 0;
                    case DATA_VOID_FOCUSES_KILLED:  return _voidFocusesKilled;
                    case DATA_RITUALS_STOPPED:       return _ritualsCompleted;
                    case DATA_DEVOTED_KILLED:        return _devotedKilled;
                    case DATA_ANTENORIAN_KILLED:     return _bossKilled ? 1 : 0;
                    default:                         return 0;
                }
            }

        private:
            void ForceCompleteCurrentScenarioStep()
            {
                if (InstanceMap* instanceMap = instance->ToInstanceMap())
                {
                    if (InstanceScenario* scenario = instanceMap->GetInstanceScenario())
                    {
                        ScenarioStepEntry const* currentStep = scenario->GetStep();
                        if (currentStep)
                        {
                            TC_LOG_DEBUG("scripts.delve", "ForceCompleteCurrentScenarioStep: Completing step {} (OrderIndex {})",
                                currentStep->ID, currentStep->OrderIndex);
                            scenario->SetStepState(currentStep, SCENARIO_STEP_DONE);
                            scenario->CompleteStep(currentStep);
                        }
                    }
                }
            }

            void DespawnNearbyRitualFocuses(Unit* source)
            {
                std::list<GameObject*> focuses;
                source->GetGameObjectListWithEntryInGrid(focuses, GO_RITUAL_FOCUS, 20.0f);
                for (GameObject* go : focuses)
                    go->DespawnOrUnsummon();
            }

            void RewardDelveReputation()
            {
                FactionEntry const* seasonFaction = sFactionStore.LookupEntry(FACTION_DELVES_SEASON_1);
                FactionEntry const* companionFaction = sFactionStore.LookupEntry(FACTION_COMPANION_XP);

                if (!seasonFaction)
                    TC_LOG_ERROR("scripts.delve", "Shadow Enclave: Faction {} (Delves: Season 1) not found in DB2!", FACTION_DELVES_SEASON_1);
                if (!companionFaction)
                    TC_LOG_ERROR("scripts.delve", "Shadow Enclave: Faction {} (Companion XP) not found in DB2!", FACTION_COMPANION_XP);

                Map::PlayerList const& players = instance->GetPlayers();
                for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                {
                    if (Player* player = itr->GetSource())
                    {
                        if (seasonFaction)
                        {
                            player->GetReputationMgr().ModifyReputation(seasonFaction, DELVE_REPUTATION_REWARD);
                            TC_LOG_DEBUG("scripts.delve", "Shadow Enclave: Granted {} rep with Season 1 (faction {}) to {}",
                                DELVE_REPUTATION_REWARD, FACTION_DELVES_SEASON_1, player->GetName());
                        }
                        if (companionFaction)
                        {
                            player->GetReputationMgr().ModifyReputation(companionFaction, COMPANION_XP_REWARD);
                            TC_LOG_DEBUG("scripts.delve", "Shadow Enclave: Granted {} companion XP (faction {}) to {}",
                                COMPANION_XP_REWARD, FACTION_COMPANION_XP, player->GetName());
                        }
                    }
                }
            }

            void TransitionToRewardScenario()
            {
                InstanceMap* instanceMap = instance->ToInstanceMap();
                if (!instanceMap)
                    return;

                DelveInfo const* info = GetDelveInfoByMap(instance->GetId());
                if (!info)
                    return;

                TC_LOG_DEBUG("scripts.delve", "Shadow Enclave: Transitioning to reward scenario {}",
                    info->RewardScenarioId);

                InstanceScenario* rewardScenario = sScenarioMgr->CreateInstanceScenario(
                    instanceMap, info->RewardScenarioId);
                if (rewardScenario)
                    instanceMap->SetInstanceScenario(rewardScenario);
                else
                    TC_LOG_ERROR("scripts.delve", "Shadow Enclave: Failed to create reward scenario {}",
                        info->RewardScenarioId);
            }

            void SpawnGameObject(uint32 entry, Position const& pos)
            {
                QuaternionData rot = QuaternionData::fromEulerAnglesZYX(pos.GetOrientation(), 0.0f, 0.0f);
                if (GameObject* go = GameObject::CreateGameObject(entry, instance, pos, rot, 255, GO_STATE_READY))
                    instance->AddToMap(go);
            }

            EventMap _events;
            ObjectGuid _antenorianGUID;
            uint32 _voidFocusesKilled;
            uint32 _ritualsCompleted;
            uint32 _devotedKilled;
            uint32 _selectedTier;
            bool _pursuitStarted;
            bool _bossKilled;
            bool _companionSpawned;
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new instance_shadow_enclave_delve_InstanceMapScript(map);
        }
};

// npc_lord_antenorian - Final boss (Step 4)
// Has cosmetic aura 1251036 on spawn. Combat AI basic for now.

enum AntenorianSpells
{
    SPELL_ANTENORIAN_COSMETIC_AURA  = 1251036,
};

struct npc_lord_antenorian : public ScriptedAI
{
    npc_lord_antenorian(Creature* creature) : ScriptedAI(creature) { }

    void JustAppeared() override
    {
        DoCastSelf(SPELL_ANTENORIAN_COSMETIC_AURA);
    }

    void JustDied(Unit* /*killer*/) override
    {
        me->Yell("The shadows... will consume... everything...", LANG_UNIVERSAL);
    }
};


// npc_darkcaller - Ritual channelers (Step 2)
// Channel a ritual spell on spawn, break on aggro.
// Entries: 250242 (Lysaille), 251899 (Cimberon), 251898 (Thelamorn)


enum DarkcallerSpells
{
    SPELL_SHADOW_BOLT           = 325386,   // Reuse necrotic bolt for now
    SPELL_RITUAL_CHANNEL_SE     = 1251057,  // Void ritual channel (from Void Focus aura)
};

enum DarkcallerEvents
{
    EVENT_SHADOW_BOLT           = 1,
};

struct npc_darkcaller : public ScriptedAI
{
    npc_darkcaller(Creature* creature) : ScriptedAI(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->InterruptNonMeleeSpells(false);
        _events.ScheduleEvent(EVENT_SHADOW_BOLT, 0s);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_SHADOW_BOLT:
                    DoCastVictim(SPELL_SHADOW_BOLT);
                    _events.Repeat(2500ms);
                    break;
                default:
                    break;
            }
        }
    }

private:
    EventMap _events;
};

// npc_void_focus_se - Void Focus (Step 1)
// Sessile NPC with spell click interaction. Killed to destroy.
// Has aura 1251057 on spawn (void channel visual).
struct npc_void_focus_se : public ScriptedAI
{
    npc_void_focus_se(Creature* creature) : ScriptedAI(creature)
    {
        me->SetReactState(REACT_PASSIVE);
    }

    void JustAppeared() override
    {
        DoCastSelf(SPELL_RITUAL_CHANNEL_SE);
    }
};

void AddSC_instance_shadow_enclave_delve()
{
    new instance_shadow_enclave_delve();
    RegisterCreatureAI(npc_lord_antenorian);
    RegisterCreatureAI(npc_darkcaller);
    RegisterCreatureAI(npc_void_focus_se);
}
