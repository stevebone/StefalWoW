/*
 * DoomCore Scripts - Atal'Aman Delve (Map 2962)
 * Instance script with Scenario 3147 progression.
 *
 * Scenario 3147 Steps:
 * Step 0 (16161): Speak to Fleek           ? OnGossipSelect
 * Step 1 (16163): Rescue Spiritpaw Cubs 7 ? Cage GO interaction
 * Step 2 (16007): Disrupt Ritual           ? Kill Consumption of Nalorakk
 * Step 3 (16008): Kill Spiritflayer Jin'Ma ? OnUnitDeath
 *
 * Flow after Step 2 completes:
 *   Consumption of Nalorakk dies ? ritual disrupted ? Jin'Ma spawns + yells
 *   Jin'Ma dies ? loot chests spawn ? Leave-O-Bot flies in after 15s
 */

#include "DelveData.h"
#include "ScriptMgr.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "DB2Stores.h"
#include "EventMap.h"
#include "CriteriaHandler.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "GossipDef.h"
#include "InstanceScript.h"
#include "Log.h"
#include "Map.h"
#include "MapManager.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ReputationMgr.h"
#include "InstanceScenario.h"
#include "ScenarioMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "TemporarySummon.h"

enum AtalAmanData
{
    // Map
    MAP_ATAL_AMAN                   = 2962,

    // Scenario IDs
    SCENARIO_ATAL_AMAN              = 3147,
    SCENARIO_DELVE_REWARD           = 3424,

    // Factions
    FACTION_DELVES_SEASON_1         = 2742,
    FACTION_COMPANION_XP            = 2744,
    DELVE_REPUTATION_REWARD         = 200,  // reputation per delve completion
    COMPANION_XP_REWARD             = 100,  // companion XP per delve completion

    // Scenario step asset IDs (GameEvent)
    GAME_EVENT_TALK_TO_FLEEK        = 100421,
    GAME_EVENT_RESCUE_CUB           = 100433,
    GAME_EVENT_DISRUPT_RITUAL       = 99588,

    // Creatures
    NPC_SPIRITFLAYER_JINMA          = 258372,
    NPC_FLEEK                       = 252930,
    NPC_HEXBOUND_RITUALIST          = 253321,
    NPC_CONSUMPTION_OF_NALORAKK     = 247106,
    NPC_LEAVE_O_BOT                 = 205496,
    NPC_VALEERA_COMPANION           = 248567,
    NPC_SPIRITPAW_CUBS              = 252948,

    // GameObjects
    GO_SPIRITPAW_CAGE               = 579108,
    GO_LOOT_CHEST_1                 = 584519,
    GO_LOOT_CHEST_2                 = 584517,

    // Instance data indices
    DATA_FLEEK_SPOKEN               = 0,
    DATA_CUBS_RESCUED               = 1,
    DATA_RITUAL_DISRUPTED           = 2,
    DATA_JINMA_KILLED               = 3,
    DATA_RITUALIST_KILLED           = 4,
    DATA_RITUAL_ADDS_KILLED         = 5,
    DATA_NALORAKK_HOSTILE           = 6,

    // Content Tuning
    CONTENT_TUNING_ID               = 2832,
};

// Positions from sniff
static constexpr float JINMA_SPAWN_X = 4850.001f;
static constexpr float JINMA_SPAWN_Y = -6258.0957f;
static constexpr float JINMA_SPAWN_Z = 232.90218f;
static constexpr float JINMA_SPAWN_O = 1.536084f;

static constexpr float CHEST1_X = 4844.512f;
static constexpr float CHEST1_Y = -6258.384f;
static constexpr float CHEST1_Z = 232.80148f;
static constexpr float CHEST1_O = 1.5359f;

static constexpr float CHEST2_X = 4859.2812f;
static constexpr float CHEST2_Y = -6251.184f;
static constexpr float CHEST2_Z = 232.80148f;
static constexpr float CHEST2_O = 3.2812f;

static constexpr float LEAVEBOT_X = 4855.278f;
static constexpr float LEAVEBOT_Y = -6230.502f;
static constexpr float LEAVEBOT_Z = 226.43198f;
static constexpr float LEAVEBOT_O = 0.7845f;

// Companion spawn position (near entry area)
static constexpr float VALEERA_X = 5130.0f;
static constexpr float VALEERA_Y = -5864.0f;
static constexpr float VALEERA_Z = 217.2f;
static constexpr float VALEERA_O = 0.0f;

static constexpr char const* AtalAmanScriptName = "instance_atal_aman_delve";

// ============================================================
// Instance Script
// ============================================================
class instance_atal_aman_delve : public InstanceMapScript
{
    public:
        instance_atal_aman_delve() : InstanceMapScript(AtalAmanScriptName, MAP_ATAL_AMAN) { }

        struct instance_atal_aman_delve_InstanceMapScript : public InstanceScript
        {
            instance_atal_aman_delve_InstanceMapScript(InstanceMap* map) : InstanceScript(map)
            {
                _cubsRescued = 0;
                _ritualAddsKilled = 0;
                _selectedTier = 0;
                _fleekSpoken = false;
                _ritualDisrupted = false;
                _bossKilled = false;
                _ritualistKilled = false;
                _nalorakkHostile = false;
                _companionSpawned = false;
            }

            void OnPlayerEnter(Player* player) override
            {
                InstanceScript::OnPlayerEnter(player);

                // Read and consume pending delve entry data (tier selection from gossip)
                auto& pending = GetPendingDelveEntries();
                auto itr = pending.find(player->GetGUID());
                if (itr != pending.end())
                {
                    _selectedTier = itr->second.TierIndex;
                    pending.erase(itr);
                    TC_LOG_DEBUG("scripts.delve", "Atal'Aman: Player {} entered at Tier {} (0-based)",
                        player->GetName(), _selectedTier);
                }

                // Sniff confirmed: WS_DELVE_TIER remains at the tier value inside the delve
                // (e.g. value=3 for Tier 3, set during gossip select and NOT cleared on entry).
                // Re-send it here in case the client missed the gossip-time update.
                player->SendUpdateWorldState(WS_DELVE_TIER, _selectedTier + 1);

                // Send instance-local WorldState: LfgDungeonsID (from sniff - only WS confirmed inside instance)
                DelveInfo const* info = GetDelveInfoByMap(instance->GetId());
                if (info)
                    player->SendUpdateWorldState(WS_DELVE_LFG_DUNGEONS_ID, info->LfgDungeonsId);

                // Populate DelveData UpdateField on the player (from sniff SMSG_UPDATE_OBJECT)
                // Field_0 = MapID, SpellID = tier entry spell, Field_8/Field_10 set to 0 for now
                player->SetDelveData(
                    instance->GetId(),                      // Field_0 = MapID
                    TIER_SPELL_IDS[_selectedTier],          // SpellID = tier spell
                    0,                                      // Field_8 = LootHistoryInstanceID (populated later)
                    0                                       // Field_10 = unknown (sniff: 356)
                );

                // Spawn companion Valeera at entry area (once per instance)
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
                    case NPC_SPIRITFLAYER_JINMA:
                        _jinmaGUID = creature->GetGUID();
                        break;
                    case NPC_HEXBOUND_RITUALIST:
                        _ritualistGUID = creature->GetGUID();
                        break;
                    case NPC_CONSUMPTION_OF_NALORAKK:
                        _nalorakkGUID = creature->GetGUID();
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

                switch (unit->GetEntry())
                {
                    case NPC_HEXBOUND_RITUALIST:
                    {
                        if (!_ritualistKilled)
                        {
                            _ritualistKilled = true;
                            TC_LOG_DEBUG("scripts.delve", "Atal'Aman: Hexbound Ritualist killed - making Nalorakk hostile");

                            SetData(DATA_NALORAKK_HOSTILE, 1);
                        }
                        break;
                    }
                    case NPC_CONSUMPTION_OF_NALORAKK:
                    {
                        if (!_ritualDisrupted)
                        {
                            _ritualDisrupted = true;
                            TC_LOG_DEBUG("scripts.delve", "Atal'Aman: Consumption of Nalorakk killed - ritual disrupted, spawning Jin'Ma");

                            ForceCompleteCurrentScenarioStep();

                            instance->SummonCreature(NPC_SPIRITFLAYER_JINMA,
                                { JINMA_SPAWN_X, JINMA_SPAWN_Y, JINMA_SPAWN_Z, JINMA_SPAWN_O });
                        }
                        break;
                    }
                    case NPC_SPIRITFLAYER_JINMA:
                    {
                        if (!_bossKilled)
                        {
                            _bossKilled = true;
                            TC_LOG_DEBUG("scripts.delve", "Atal'Aman: Spiritflayer Jin'Ma killed - scenario complete");

                            ForceCompleteCurrentScenarioStep();
                            TransitionToRewardScenario();
                            RewardDelveReputation();
                            SpawnGameObject(GO_LOOT_CHEST_1, { CHEST1_X, CHEST1_Y, CHEST1_Z, CHEST1_O });
                            SpawnGameObject(GO_LOOT_CHEST_2, { CHEST2_X, CHEST2_Y, CHEST2_Z, CHEST2_O });
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
                            TC_LOG_DEBUG("scripts.delve", "Atal'Aman: Spawning Leave-O-Bot");
                            instance->SummonCreature(NPC_LEAVE_O_BOT,
                                { LEAVEBOT_X, LEAVEBOT_Y, LEAVEBOT_Z, LEAVEBOT_O });
                            break;
                        }
                        default:
                            break;
                    }
                }
            }

            void SetData(uint32 type, uint32 /*data*/) override
            {
                switch (type)
                {
                    case DATA_FLEEK_SPOKEN:
                        if (!_fleekSpoken)
                        {
                            _fleekSpoken = true;
                            ForceCompleteCurrentScenarioStep();
                        }
                        break;

                    case DATA_CUBS_RESCUED:
                        if (_cubsRescued < 7)
                        {
                            _cubsRescued++;
                            TC_LOG_DEBUG("scripts.delve", "Atal'Aman: Spiritpaw Cub rescued ({}/7)", _cubsRescued);
                            SendScenarioCubProgress();
                            if (_cubsRescued >= 7)
                                ForceCompleteCurrentScenarioStep();
                        }
                        break;

                    case DATA_RITUAL_DISRUPTED:
                        if (!_ritualDisrupted)
                        {
                            _ritualDisrupted = true;
                            ForceCompleteCurrentScenarioStep();
                        }
                        break;

                    case DATA_NALORAKK_HOSTILE:
                    {
                        if (!_nalorakkHostile)
                        {
                            _nalorakkHostile = true;
                            if (Creature* nalorakk = instance->GetCreature(_nalorakkGUID))
                            {
                                nalorakk->AI()->DoAction(DATA_NALORAKK_HOSTILE);
                                TC_LOG_DEBUG("scripts.delve", "Atal'Aman: Consumption of Nalorakk is now hostile");
                            }
                        }
                        break;
                    }

                    default:
                        break;
                }
            }

            uint32 GetData(uint32 type) const override
            {
                switch (type)
                {
                    case DATA_FLEEK_SPOKEN:      return _fleekSpoken ? 1 : 0;
                    case DATA_CUBS_RESCUED:      return _cubsRescued;
                    case DATA_RITUAL_DISRUPTED:  return _ritualDisrupted ? 1 : 0;
                    case DATA_JINMA_KILLED:      return _bossKilled ? 1 : 0;
                    case DATA_RITUALIST_KILLED:  return _ritualistKilled ? 1 : 0;
                    default:                     return 0;
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

            void SendScenarioCubProgress()
            {
                InstanceMap* instanceMap = instance->ToInstanceMap();
                if (!instanceMap)
                    return;

                InstanceScenario* scenario = instanceMap->GetInstanceScenario();
                if (!scenario)
                    return;
                Map::PlayerList const& players = instance->GetPlayers();
                for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                {
                    if (Player* player = itr->GetSource())
                    {
                        scenario->UpdateCriteria(CriteriaType::AnyoneTriggerGameEventScenario,
                            GAME_EVENT_RESCUE_CUB, 0, 0, nullptr, player);
                    }
                }
            }

            void RewardDelveReputation()
            {
                FactionEntry const* seasonFaction = sFactionStore.LookupEntry(FACTION_DELVES_SEASON_1);
                FactionEntry const* companionFaction = sFactionStore.LookupEntry(FACTION_COMPANION_XP);

                if (!seasonFaction)
                    TC_LOG_ERROR("scripts.delve", "Atal'Aman: Faction {} (Delves: Season 1) not found in DB2!", FACTION_DELVES_SEASON_1);
                if (!companionFaction)
                    TC_LOG_ERROR("scripts.delve", "Atal'Aman: Faction {} (Companion XP) not found in DB2!", FACTION_COMPANION_XP);

                Map::PlayerList const& players = instance->GetPlayers();
                for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                {
                    if (Player* player = itr->GetSource())
                    {
                        if (seasonFaction)
                        {
                            player->GetReputationMgr().ModifyReputation(seasonFaction, DELVE_REPUTATION_REWARD);
                            TC_LOG_DEBUG("scripts.delve", "Atal'Aman: Granted {} rep with Season 1 (faction {}) to {}",
                                DELVE_REPUTATION_REWARD, FACTION_DELVES_SEASON_1, player->GetName());
                        }
                        if (companionFaction)
                        {
                            player->GetReputationMgr().ModifyReputation(companionFaction, COMPANION_XP_REWARD);
                            TC_LOG_DEBUG("scripts.delve", "Atal'Aman: Granted {} companion XP (faction {}) to {}",
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

                TC_LOG_DEBUG("scripts.delve", "Atal'Aman: Transitioning to reward scenario {}",
                    info->RewardScenarioId);

                InstanceScenario* rewardScenario = sScenarioMgr->CreateInstanceScenario(
                    instanceMap, info->RewardScenarioId);
                if (rewardScenario)
                    instanceMap->SetInstanceScenario(rewardScenario);
                else
                    TC_LOG_ERROR("scripts.delve", "Atal'Aman: Failed to create reward scenario {}",
                        info->RewardScenarioId);
            }

            void SpawnGameObject(uint32 entry, Position const& pos)
            {
                QuaternionData rot = QuaternionData::fromEulerAnglesZYX(pos.GetOrientation(), 0.0f, 0.0f);
                if (GameObject* go = GameObject::CreateGameObject(entry, instance, pos, rot, 255, GO_STATE_READY))
                    instance->AddToMap(go);
            }

            EventMap _events;
            ObjectGuid _jinmaGUID;
            ObjectGuid _ritualistGUID;
            ObjectGuid _nalorakkGUID;
            uint32 _cubsRescued;
            uint32 _ritualAddsKilled;
            uint32 _selectedTier;
            bool _fleekSpoken;
            bool _ritualDisrupted;
            bool _bossKilled;
            bool _ritualistKilled;
            bool _nalorakkHostile;
            bool _companionSpawned;
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new instance_atal_aman_delve_InstanceMapScript(map);
        }
};

struct npc_fleek : public ScriptedAI
{
    npc_fleek(Creature* creature) : ScriptedAI(creature) { }

    bool OnGossipSelect(Player* player, uint32 /*menuId*/, uint32 /*gossipListId*/) override
    {
        CloseGossipMenuFor(player);

        if (InstanceScript* instance = me->GetInstanceScript())
            instance->SetData(DATA_FLEEK_SPOKEN, 1);

        return true;
    }
};

struct go_spiritpaw_cage : public GameObjectAI
{
    go_spiritpaw_cage(GameObject* go) : GameObjectAI(go) { }

    bool OnGossipHello(Player* player) override
    {
        if (!player)
            return true;

        InstanceScript* instance = player->GetInstanceScript();
        if (!instance)
            return true;

        if (instance->GetData(DATA_CUBS_RESCUED) >= 7)
            return true;

        me->SetFlag(GO_FLAG_NOT_SELECTABLE);
        me->SetGoState(GO_STATE_ACTIVE);

        instance->SetData(DATA_CUBS_RESCUED, 1);

        std::list<Creature*> cubs;
        me->GetCreatureListWithEntryInGrid(cubs, NPC_SPIRITPAW_CUBS, 10.0f);
        for (Creature* cub : cubs)
        {
            if (cub->IsAlive())
            {
                cub->AI()->Talk(0, player);

                cub->SetWalk(false);
                float angle = cub->GetAbsoluteAngle(me) + float(M_PI); // opposite direction?
                float x = cub->GetPositionX() + 15.0f * std::cos(angle);
                float y = cub->GetPositionY() + 15.0f * std::sin(angle);
                cub->GetMotionMaster()->MovePoint(0, x, y, cub->GetPositionZ());
                cub->DespawnOrUnsummon(5s);
            }
        }

        TC_LOG_DEBUG("scripts.delve", "go_spiritpaw_cage: Player {} opened cage, cubs rescued: {}/7",
            player->GetName(), instance->GetData(DATA_CUBS_RESCUED));

        return true;
    }
};

enum RitualistSpells
{
    SPELL_NECROTIC_BOLT     = 325386,   // Spammed on victim in combat
    SPELL_RITUAL_CHANNEL    = 1257652,  // Ritual visual - cast on spawn, removed on aggro
    SPELL_DEATHLY_DECAY     = 1258667,  // Channeled on a player
};

enum RitualistEvents
{
    EVENT_NECROTIC_BOLT     = 1,
    EVENT_DEATHLY_DECAY     = 2,
};

struct npc_hexbound_ritualist : public ScriptedAI
{
    npc_hexbound_ritualist(Creature* creature) : ScriptedAI(creature) { }

    void JustAppeared() override
    {
        // Begin ritual channel on spawn
        DoCastSelf(SPELL_RITUAL_CHANNEL);
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        // Interrupt ritual and remove the aura
        me->InterruptNonMeleeSpells(false);
        me->RemoveAurasDueToSpell(SPELL_RITUAL_CHANNEL);

        _events.ScheduleEvent(EVENT_NECROTIC_BOLT, 0s);
        _events.ScheduleEvent(EVENT_DEATHLY_DECAY, 8s);
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
                case EVENT_NECROTIC_BOLT:
                    DoCastVictim(SPELL_NECROTIC_BOLT);
                    _events.Repeat(2500ms);
                    break;
                case EVENT_DEATHLY_DECAY:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true))
                        DoCast(target, SPELL_DEATHLY_DECAY);
                    _events.Repeat(15s, 20s);
                    break;
                default:
                    break;
            }
        }
    }

private:
    EventMap _events;
};

// npc_consumption_of_nalorakk - Passive until instance triggers hostile
// SmartAI spells migrated to C++:
//   Spell 1279202 - every 1.5-6.1s (event_type 0, in combat)
//   Spell 1263251 - cosmetic ghostly aura (applied on spawn)
//   Spell 1270562 - every 2.5s (event_type 0, in combat)

enum NalorakkEvents
{
    EVENT_NALORAKK_SPELL_1 = 1, // 1279202
    EVENT_NALORAKK_SPELL_3 = 2, // 1270562
};

enum NalorakkSpells
{
    SPELL_NALORAKK_COSMETIC_AURA = 1263251,
    SPELL_NALORAKK_1             = 1279202,
    SPELL_NALORAKK_3             = 1270562,
};

struct npc_consumption_of_nalorakk : public ScriptedAI
{
    npc_consumption_of_nalorakk(Creature* creature) : ScriptedAI(creature) { }

    void JustAppeared() override
    {
        DoCastSelf(SPELL_NALORAKK_COSMETIC_AURA);
    }

    void DoAction(int32 action) override
    {
        if (action == DATA_NALORAKK_HOSTILE)
        {
            me->SetFaction(16); // Hostile
            me->SetReactState(REACT_AGGRESSIVE);
            me->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
            me->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
            me->Yell("You disrupt de ritual, now you be de new sacrifice!", LANG_UNIVERSAL);

            if (Player* player = me->SelectNearestPlayer(100.0f))
                AttackStart(player);
        }
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _events.ScheduleEvent(EVENT_NALORAKK_SPELL_1, 1500ms, 6138ms);
        _events.ScheduleEvent(EVENT_NALORAKK_SPELL_3, 2500ms);
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
                case EVENT_NALORAKK_SPELL_1:
                    DoCastVictim(SPELL_NALORAKK_1);
                    _events.Repeat(1500ms, 6138ms);
                    break;
                case EVENT_NALORAKK_SPELL_3:
                    DoCastVictim(SPELL_NALORAKK_3);
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


// npc_spiritflayer_jinma - Final boss (Step 3)
// Combat AI handled by SmartAI, this script handles spawn yell, migrate spells to c++ later

struct npc_spiritflayer_jinma : public ScriptedAI
{
    npc_spiritflayer_jinma(Creature* creature) : ScriptedAI(creature)
    {
        _yellDone = false;
    }

    void JustAppeared() override
    {
        if (!_yellDone)
        {
            _yellDone = true;
            me->Yell("Show your face, $p. I make you pay for meddling!", LANG_UNIVERSAL);
        }
    }

    void JustDied(Unit* /*killer*/) override
    {
        // huh?
    }

private:
    bool _yellDone;
};

static constexpr uint32 OVERWORLD_MAP    = 0;
static constexpr float OVERWORLD_EXIT_X  = 5121.0f;
static constexpr float OVERWORLD_EXIT_Y  = -5861.4f;
static constexpr float OVERWORLD_EXIT_Z  = 217.1f;
static constexpr float OVERWORLD_EXIT_O  = 3.22f;

enum LeaveBotEvents
{
    EVENT_LEAVEBOT_FLY_UP   = 1,
    EVENT_LEAVEBOT_TELEPORT = 2,
};

struct npc_leave_o_bot : public ScriptedAI
{
    npc_leave_o_bot(Creature* creature) : ScriptedAI(creature)
    {
        me->SetReactState(REACT_PASSIVE);
    }

    void PassengerBoarded(Unit* passenger, int8 /*seatId*/, bool apply) override
    {
        if (!apply || !passenger || !passenger->ToPlayer())
            return;

        _passengerGUID = passenger->GetGUID();

        _events.ScheduleEvent(EVENT_LEAVEBOT_FLY_UP, 500ms);
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type == POINT_MOTION_TYPE && id == 1)
        {
            _events.ScheduleEvent(EVENT_LEAVEBOT_TELEPORT, 1s);
        }
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_LEAVEBOT_FLY_UP:
                {
                    me->SetDisableGravity(true);
                    me->GetMotionMaster()->MovePoint(1,
                        me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() + 30.0f);
                    break;
                }
                case EVENT_LEAVEBOT_TELEPORT:
                {
                    if (Player* player = ObjectAccessor::GetPlayer(*me, _passengerGUID))
                    {
                        player->ExitVehicle();
                        player->ClearDelveData();
                        player->TeleportTo(OVERWORLD_MAP,
                            OVERWORLD_EXIT_X, OVERWORLD_EXIT_Y, OVERWORLD_EXIT_Z, OVERWORLD_EXIT_O,
                            TELE_TO_SEAMLESS);
                    }
                    me->DespawnOrUnsummon(1s);
                    break;
                }
                default:
                    break;
            }
        }
    }

private:
    EventMap _events;
    ObjectGuid _passengerGUID;
};

void AddSC_instance_atal_aman_delve()
{
    new instance_atal_aman_delve();
    RegisterCreatureAI(npc_fleek);
    RegisterGameObjectAI(go_spiritpaw_cage);
    RegisterCreatureAI(npc_hexbound_ritualist);
    RegisterCreatureAI(npc_consumption_of_nalorakk);
    RegisterCreatureAI(npc_spiritflayer_jinma);
    RegisterCreatureAI(npc_leave_o_bot);
}
