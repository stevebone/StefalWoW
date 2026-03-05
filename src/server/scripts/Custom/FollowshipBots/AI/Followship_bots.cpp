// =========================================
// TO-DO
// =========================================

#include "Log.h"
#include "Map.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "SpellInfo.h"

#include "Followship_bots.h"
#include "Followship_bots_ai_base.h"
#include "Followship_bots_mgr.h"

#include "Followship_bots_auras_handler.h"
#include "Followship_bots_chatter_handler.h"
#include "Followship_bots_combat_handler.h"
#include "Followship_bots_death_handler.h"
#include "Followship_bots_events_handler.h"
#include "Followship_bots_gossip_handler.h"
#include "Followship_bots_group_handler.h"
#include "Followship_bots_incombat_handler.h"
#include "Followship_bots_movement_handler.h"
#include "Followship_bots_outofcombat_handler.h"
#include "Followship_bots_pet_handler.h"
#include "Followship_bots_powers_handler.h"
#include "Followship_bots_recovery_handler.h"
#include "Followship_bots_regen_handler.h"
#include "Followship_bots_spells_handler.h"
#include "Followship_bots_stats_handler.h"
#include "Followship_bots_teleport_handler.h"



class npc_followship_bots : public CreatureScript
{
public:
    npc_followship_bots() : CreatureScript("npc_followship_bots") { }

    struct npc_followship_botsAI : public FSB_BaseAI
    {
        npc_followship_botsAI(Creature* creature) : FSB_BaseAI(creature)
        {
            // Runs once when creature is spawned
            static bool tablesInitialized = false;
            if (!tablesInitialized)
            {
                FSBSpells::InitBotSpellTables();
                tablesInitialized = true;
            }

            FSBSpells::InitSpellRuntime(me, botRuntimeSpells);
        }        

        bool updateFollowInfo = false;

        void InitializeAI() override // Runs once after creature is spawned and AI not loaded
        {
            ScriptedAI::InitializeAI(); // always call base first            
        }

        void Reset() override // Runs at creature respawn, evade or when triggered
        {
            if (FollowshipBotsConfig::configFSBEnabled)
            {
                events.Reset();

                FSBMgr::Get()->SetInitialBotState(me);

                TC_LOG_DEBUG("scripts.fsb.general", "FSB: Reset() triggered for bot: {}", me->GetName());

                // Schedule Generic Events

                // Periodic events happen as long as the bot exists in the world
                events.ScheduleEvent(FSB_EVENT_PERIODIC_MAINTENANCE, 1s);
                // Hire events happen when bot is hired (on duty)
                events.ScheduleEvent(FSB_EVENT_HIRED_MAINTENANCE, 1s);
                // Combat events happen when bot is in combat
                events.ScheduleEvent(FSB_EVENT_COMBAT_MAINTENANCE, 1s);

                if (FollowshipBotsConfig::configFSBUseCustomRegen)
                {
                    botRegenMods = FSBRegenMods(); // zero

                    // Disable npc flags if any in DB
                    me->RemoveUnitFlag2(UNIT_FLAG2_REGENERATE_POWER);
                    me->SetRegenerateHealth(false); // no health regen

                    //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Reset() Using Custom Regen for bot: {}", me->GetName());
                }
                else
                {
                    me->SetUnitFlag2(UNIT_FLAG2_REGENERATE_POWER);
                    me->SetRegenerateHealth(true);  // restore normal regen
                }

            }

            else
            {
                // Completely disable gossip
                me->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP);
            }            
        }

        // Runs once when creature is summoned by another unit
        // This only works if creature is spawned by summon (NOT DB spawn)
        void IsSummonedBy(WorldObject* /*summoner*/) override
        {
            TC_LOG_DEBUG("scripts.fsb.general", "FSB: IsSummonedBy() triggered for bot: {}", me->GetName());
        }

        void JustAppeared() override // Runs once when creature appeared in world, works for DB spawns
        {
            TC_LOG_DEBUG("scripts.fsb.general", "FSB: JustAppeared() triggered for bot: {}", me->GetName());
        }

        bool OnGossipHello(Player* player) override // Runs once when opening creature gossip
        {
            return FSBGossip::HandleDefaultGossipHello(me, player, botHired, _playerGuid);
        }

        bool OnGossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) override // Runs once when gossip item selected
        {
            uint32 const action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);
            ClearGossipMenuFor(player);

            switch (action)
            {
                // Hire Menu
            case GOSSIP_ACTION_INFO_DEF + 1:
                return FSBGossip::HandleGossipMenuHire(me, player);

                // Bot Info
            case GOSSIP_ACTION_INFO_DEF + 2:
                FSBGossip::HandleGossipItemInfo(me, player);
                return true;

                // Roles Menu
            case GOSSIP_ACTION_INFO_DEF + 3:
                return FSBGossip::HandleGossipMenuRoles(me, player);

                // Bot Gossip Back to main
            case GOSSIP_ACTION_INFO_DEF + 4:
                OnGossipHello(player);
                return true;

                // Bot Dismiss
            case GOSSIP_ACTION_INFO_DEF + 5:
                FSBEvents::ScheduleBotEvent(me, FSB_EVENT_HIRED_EXPIRED, 1s, 3s);
                break;

                // Bot Instructions Menu
            case GOSSIP_ACTION_INFO_DEF + 6:
                return FSBGossip::HandleGossipMenuInstructions(me, player);

                // Bot Follow Distance Menu
            case GOSSIP_ACTION_INFO_DEF + 7:
                return FSBGossip::HandleGossipMenuFollowDistance(me, player);

                // Bot Follow Angle Menu
            case GOSSIP_ACTION_INFO_DEF + 8:
                return FSBGossip::HandleGossipMenuFollowAngle(me, player);

                // Bot Hire Option 1
            case GOSSIP_ACTION_INFO_DEF + 10:
                return FSBGossip::HandleGossipItemHire(me, player, FollowshipBotsConfig::configFSBHireDuration1);
                

                // Bot Hire Option 2
            case GOSSIP_ACTION_INFO_DEF + 11:
                return FSBGossip::HandleGossipItemHire(me, player, FollowshipBotsConfig::configFSBHireDuration2);

                // Bot Hire Option 3
            case GOSSIP_ACTION_INFO_DEF + 12:
                return FSBGossip::HandleGossipItemHire(me, player, FollowshipBotsConfig::configFSBHireDuration3);

                // Bot Hire Option Permanent
            case GOSSIP_ACTION_INFO_DEF + 13:
            {
                return FSBGossip::HandleGossipItemHirePermanent(me, player);
            }

                // Bot Follow Distance Option 1
            case GOSSIP_ACTION_INFO_DEF + 14:
            {
                botFollowDistance = FOLLOW_DISTANCE_CLOSE;
                FSBMovement::ResumeFollow(me, botFollowDistance, botFollowAngle);
                FSBChatter::DemandTimedReply(me, nullptr, FSB_ChatterCategory::botAcknowledge, FSB_ReplyType::Say, FSB_ChatterSource::None);
                break;
            }

                // Bot Follow Distance Option 2
            case GOSSIP_ACTION_INFO_DEF + 15:
            {
                botFollowDistance = FOLLOW_DISTANCE_NORMAL;
                FSBMovement::ResumeFollow(me, botFollowDistance, botFollowAngle);
                FSBChatter::DemandTimedReply(me, nullptr, FSB_ChatterCategory::botAcknowledge, FSB_ReplyType::Say, FSB_ChatterSource::None);
                break;
            }

                // Bot Follow Distance Option 3
            case GOSSIP_ACTION_INFO_DEF + 16:
            {
                botFollowDistance = FOLLOW_DISTANCE_WIDE;
                FSBMovement::ResumeFollow(me, botFollowDistance, botFollowAngle);
                FSBChatter::DemandTimedReply(me, nullptr, FSB_ChatterCategory::botAcknowledge, FSB_ReplyType::Say, FSB_ChatterSource::None);
                break;
            }

                // Bot Follow Angle Option 1
            case GOSSIP_ACTION_INFO_DEF + 17:
            {
                botFollowAngle = FOLLOW_ANGLE_FRONT;
                FSBMovement::ResumeFollow(me, botFollowDistance, botFollowAngle);
                FSBChatter::DemandTimedReply(me, nullptr, FSB_ChatterCategory::botAcknowledge, FSB_ReplyType::Say, FSB_ChatterSource::None);
                break;
            }

                // Bot Follow Angle Option 2
            case GOSSIP_ACTION_INFO_DEF + 18:
            {
                botFollowAngle = FOLLOW_ANGLE_BEHIND;
                FSBMovement::ResumeFollow(me, botFollowDistance, botFollowAngle);
                FSBChatter::DemandTimedReply(me, nullptr, FSB_ChatterCategory::botAcknowledge, FSB_ReplyType::Say, FSB_ChatterSource::None);
                break;
            }

                // Bot Follow Angle Option 3
            case GOSSIP_ACTION_INFO_DEF + 19:
            {
                botFollowAngle = FSBUtils::GetRandomRightAngle(); //FOLLOW_ANGLE_RIGHT;
                FSBMovement::ResumeFollow(me, botFollowDistance, botFollowAngle);
                FSBChatter::DemandTimedReply(me, nullptr, FSB_ChatterCategory::botAcknowledge, FSB_ReplyType::Say, FSB_ChatterSource::None);
                break;
            }

                // Bot Stay Option
            case GOSSIP_ACTION_INFO_DEF + 20:
            {
                FSBMovement::StopFollow(me);
                FSBChatter::DemandTimedReply(me, nullptr, FSB_ChatterCategory::botStay, FSB_ReplyType::Say, FSB_ChatterSource::None);
                break;
            }

                // Bot Follow Option
            case GOSSIP_ACTION_INFO_DEF + 21:
            {
                FSBMovement::ResumeFollow(me, botFollowDistance, botFollowAngle);
                FSBChatter::DemandTimedReply(me, nullptr, FSB_ChatterCategory::botFollow, FSB_ReplyType::Say, FSB_ChatterSource::None);
                break;
            }

                // Bot Follow Angle Option 4
            case GOSSIP_ACTION_INFO_DEF + 22:
            {
                botFollowAngle = FSBUtils::GetRandomLeftAngle(); //FOLLOW_ANGLE_LEFT;
                FSBMovement::ResumeFollow(me, botFollowDistance, botFollowAngle);
                FSBChatter::DemandTimedReply(me, nullptr, FSB_ChatterCategory::botAcknowledge, FSB_ReplyType::Say, FSB_ChatterSource::None);
                break;
            }

                // Bot Role Option 1
            case GOSSIP_ACTION_INFO_DEF + 23:
                FSBGossip::HandleGossipItemRole(me, botClass, FSB_GOSSIP_ROLE_1, botHasDemon);
                break;

                // Bot Role Option 2
            case GOSSIP_ACTION_INFO_DEF + 24:
                FSBGossip::HandleGossipItemRole(me, botClass, FSB_GOSSIP_ROLE_2, botHasDemon);
                break;

            // Bot Role Option 3
            case GOSSIP_ACTION_INFO_DEF + 25:
                FSBGossip::HandleGossipItemRole(me, botClass, FSB_GOSSIP_ROLE_3, botHasDemon);
                break;

            case GOSSIP_ACTION_INFO_DEF + 26:
                return FSBGossip::HandleGossipMenuPortals(me, player);

            case GOSSIP_ACTION_INFO_DEF + 27:
            {
                me->CastSpell(me, SPELL_MAGE_PORTAL_STORMWIND);
                me->Say("Okay one portal coming up!", LANG_UNIVERSAL);
                break;
            }

            case GOSSIP_ACTION_INFO_DEF + 28:
            {
                player->CastSpell(me, 121849);
                me->Say("Okay one portal coming up!", LANG_UNIVERSAL);
                break;
            }

            case GOSSIP_ACTION_INFO_DEF + 29:
            {
                player->CastSpell(player, 121851);
                me->Say("Okay one portal coming up!", LANG_UNIVERSAL);
                break;
            }

            case GOSSIP_ACTION_INFO_DEF + 30:
            {
                player->CastSpell(player, 121850);
                me->Say("Okay one portal coming up!", LANG_UNIVERSAL);
                break;
            }

            // Bot Role Option 4 - Druid
            case GOSSIP_ACTION_INFO_DEF + 31:
                FSBGossip::HandleGossipItemRole(me, botClass, FSB_GOSSIP_ROLE_4, botHasDemon);
                break;

            default:
                break;
            }
            CloseGossipMenuFor(player);
            return true;
        }

        void JustEngagedWith(Unit* /*who*/) override // Runs every time creature gets in combat
        {
            TC_LOG_DEBUG("scripts.fsb.general", "FSB: JustEngagedWith() triggered for bot: {}", me->GetName());
        }

        void JustEnteredCombat(Unit* /*who*/) override
        {
        }

        void JustExitedCombat() override
        {
            botOutOfCombatTimer = getMSTime();
        }        

        void HealReceived(Unit* /*done_by*/, uint32& /*addhealth*/) override
        {
        }

        int32 GetBotSpellPower() const override
        {
            TC_LOG_DEBUG("scripts.fsb.general", "FSB: GetBotSpellPower() triggered for bot: {}", me->GetName());

            return FSBStats::BotGetSpellPower(me);
        }

        void DamageDealt(Unit* victim, uint32& damage, DamageEffectType /*damageType*/) override
        {
            damage = FSBStats::CalculateScaledBotDamage(me, victim, damage);
            damage = uint32(damage * FSBStats::ApplyBotDamageDoneReduction(me));
            FSBPowers::GenerateRageFromDamageDone(me, damage);
        }

        // Runs every time creature takes damage
        void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            damage = uint32(damage * FSBStats::ApplyBotDamageTakenReduction(me));
            FSBPowers::GenerateRageFromDamageTaken(me, damage);
        }

        void EnterEvadeMode(EvadeReason /*why*/) override // Runs every time creature evades
        {

        }

        void KilledUnit(Unit* victim) override // Runs every time the creature kills an unit
        {
            Player* player = FSBMgr::Get()->GetBotOwner(me);
            if(player)
                FSBChatter::DemandBotChatter(me, victim, FSB_ChatterCategory::targetKilledHired);
            else FSBChatter::DemandBotChatter(me, victim, FSB_ChatterCategory::targetKilled);            
        }

        void OnSpellCast(SpellInfo const* spell) override // Runs every time the creature casts a spell
        {
            uint32 spellId = spell->Id;

            FSBSpells::HandleOnSpellCast(me, botClass, spellId);
        }

        void SpellHit(WorldObject* /*caster*/, SpellInfo const* spellInfo) override
        {
            if (!spellInfo)
                return;

            FSBDeath::HandleSpellResurrection(me, spellInfo->Id);
        }

        void SpellHitTarget(WorldObject* target, SpellInfo const* spellInfo) override
        {
            TC_LOG_DEBUG("scripts.fsb.general", "FSB: SpellHitTarget() triggered for bot: {} with target {} and Spell {}", me->GetName(), target->GetName(), spellInfo->Id);

            Player* player = target->ToPlayer();

            if (spellInfo->Id == 461537 && player && !player->IsAlive() && player->GetGUID() == me->GetOwnerGUID())
                player->ResurrectPlayer(0.35f, false);
        }

        void OnAuraApplied(AuraApplication const* aurApp) override
        {
            if (!aurApp)
                return;

            FSBAuras::BotOnAuraApplied(me, aurApp, true, botRegenMods, botHasSoulstone, botStats);
        }

        void OnAuraRemoved(AuraApplication const* aurApp) override
        {
            if (!aurApp)
                return;

            FSBAuras::BotOnAuraApplied(me, aurApp, false, botRegenMods, botHasSoulstone, botStats);
        }

        void JustSummoned(Creature* summon) override // Runs every time the creature summons another creature
        {
            if (summon && botClass == FSB_Class::Warlock)
            {
                FSBWarlock::AdjustSummonHealth(me, summon);
                botHasDemon = true;
                TC_LOG_DEBUG("scripts.fsb.general", "FSB: JustSummoned Triggered for Warlock bot {} with summon {}", me->GetName(), summon->GetName());
            }
                
        }

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override // Runs everytime the creature's summon dies - pet or minion
        {
            if (summon && botClass == FSB_Class::Warlock)
            {
                botHasDemon = false;
                TC_LOG_DEBUG("scripts.fsb.general", "FSB: SummonedCreatureDies Triggered for Warlock bot {} with summon {}", me->GetName(), summon->GetName());
            }
        }

        void JustDied(Unit* killer) override // Runs once when creature dies
        {
            botCorpsePos = me->GetPosition();
            FSBDeath::HandlerJustDied(me, killer);
        }

        void MovementInform(uint32 /*type*/, uint32 id) override
        {
            switch (id)
            {
            case FSB_MOVEMENT_POINT_CORPSE:
            {
                FSBDeath::BotSetStateAfterCorpseRevive(me);
                break;
            }

            case FSB_MOVEMENT_POINT_NEAR_FIRE:
            {
                FSBEvents::ScheduleBotEvent(me, FSB_EVENT_RANDOM_ACTION_SIT_BY_FIRE, 1s, 3s);
                break;
            }
            default:
                break;
            }
        }

        uint32 GetData(uint32 /*type*/) const override // Runs once to check what data exists on the creature
        {
            return 0;
        }

        void SetData(uint32 type, uint32 value) override // Runs once to check set data is SET on the creature
        {
            switch (type)
            {
            case FSB_DATA_HIRED:
            {
                botHired = value;
                TC_LOG_DEBUG("scripts.fsb.general", "FSB: SetData FSB_DATA_HIRED for bot {} with hired value {}", me->GetName(), botHired);

                break;
            }

            case FSB_DATA_HIRE_TIME_LEFT:
            {
                hireTimeLeft = value;

                if (hireTimeLeft > 0)
                {
                    FSBEvents::ScheduleBotEvent(me, FSB_EVENT_HIRED_EXPIRED, std::chrono::seconds(hireTimeLeft));
                    TC_LOG_DEBUG("scripts.fsb.general", "FSB: SetData FSB_DATA_HIRE_TIME_LEFT for bot {} with hireTimeLeft value {}", me->GetName(), hireTimeLeft);
                }
                
                FSBMovement::ResumeFollow(me, botFollowDistance, botFollowAngle);
                TC_LOG_DEBUG("scripts.fsb.general", "FSB: SetData FSB_DATA_HIRE_TIME_LEFT for bot {} with permanent hire.", me->GetName());

                break;
            }

            }
        }

        void DoAction(int32 action) override
        {
            switch (action)
            {
            case FSB_ACTION_WAIT_HEALER_RESSURECT:
                events.ScheduleEvent(FSB_EVENT_HIRED_WAIT_HEALER_RESSURECT, 90s);
                break;

            case FSB_ACTION_SOULSTONE_RESSURECT:
                events.ScheduleEvent(FSB_EVENT_SOULSTONE_RESSURECT, 4s, 6s);
                break;

            case FSB_ACTION_TELEPORT_GRAVEYARD:
                events.ScheduleEvent(FSB_EVENT_TELEPORT_GRAVEYARD, 5s, 6s);
                break;

            case FSB_ACTION_TELEPORT_DUNGEON:
                events.ScheduleEvent(FSB_EVENT_HIRED_TELEPORT_DUNGEON, 5s, 6s);
                break;
            
            default:
                break;
            }
        }

        void UpdateAI(uint32 diff) override
        {
            FSBCombat::EvaluateAttackNeeded(me);
            
            events.Update(diff);
            botEvents.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                // =================================================== //
                // ========= PERIODIC MAINTENANCE ==================== //
                // Events that are triggered periodically              //
                // These events can trigger even when bot is NOT hired //
                // =================================================== //

                case FSB_EVENT_PERIODIC_MAINTENANCE:
                {
                    // bot events
                    FSBEvents::ScheduleBotEvent(me, FSB_EVENT_GENERIC_CHECK_HIRED_TIME, 10s);
                    FSBEvents::ScheduleBotEvent(me, FSB_EVENT_HIRED_DESPAWN_TEMP_BOT, 1s);

                    uint32 now = getMSTime();

                    // Check if custom regen is enabled and we can schedule every 2s
                    if (FollowshipBotsConfig::configFSBUseCustomRegen && now >= _nextRegenMs && !me->HasAura(SPELL_SPECIAL_GHOST))
                    {
                        if (me->GetHealthPct() < 100 || me->GetPowerPct(me->GetPowerType()) < 100)
                        {
                            FSBRegen::ProcessBotCustomRegenTick(me, botClass, botRegenMods);

                            // ? lock regen for next 2 seconds
                            _nextRegenMs = now + 2000;
                            
                        }
                    }

                    // Check if combat is NOT taking place and schedule OOC actions
                    if (!FSBUtilsCombat::IsCombatActive(me))
                    {
                        if (FollowshipBotsConfig::configFSBUseOOCActions && now >= _1secondsCheckMs && !me->HasAura(SPELL_SPECIAL_GHOST))
                        {
                            auto baseAI = dynamic_cast<FSB_BaseAI*>(me->AI());

                            if (FSBOOC::BotOOCActions(baseAI))
                                FSBEvents::ScheduleBotEvent(me, FSB_EVENT_HIRED_RESUME_FOLLOW, std::chrono::milliseconds(botGlobalCooldown - now));

                            // ? lock timer for next 2 seconds
                            _1secondsCheckMs = now + 1000;
                        }
                    }

                    events.ScheduleEvent(FSB_EVENT_PERIODIC_MAINTENANCE, 1s);

                    //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Event Periodic Maintenance Reached the end"); // TEMP LOG

                    break;
                }

                // =====================================
                // =====================================
                // =================================================== //
                // ========= HIRED MAINTENANCE ======================= //
                // Important polls that are needed when bot is hired   //
                // (on duty)                                           //
                // =================================================== //
                    
                case FSB_EVENT_HIRED_MAINTENANCE:
                {
                    if (botHired)
                    {
                        uint32 now = getMSTime();

                        events.ScheduleEvent(FSB_EVENT_HIRED_CHECK_TELEPORT, 3s, 5s);
                        events.ScheduleEvent(FSB_EVENT_HIRED_CHECK_MOUNT, 3s, 5s);

                        // bot events
                        FSBEvents::ScheduleBotEvent(me, FSB_EVENT_HIRED_CHECK_MEMBER_DEATH, 3s, 5s);                        

                        if (now >= _5secondsCheckMs)
                        {
                            FSBStats::UpdateBotLevelToPlayer(me);

                            // Check to dermine what friendlies we have in our "group"
                            // Includes: bot, owner and other bots owner by its owner
                            // TO-DO: Add check to include other players in the group of the owner
                            FSBGroup::BuildLogicalBotGroup(me, botLogicalGroup);

                            // ? lock check for next 5 seconds
                            _5secondsCheckMs = now + 5000;
                        }
                    }

                    events.ScheduleEvent(FSB_EVENT_HIRED_MAINTENANCE, 1ms);

                    //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Event Hired Maintenance Reached the end"); // TEMP LOG

                    break;
                }

                    // Check if player is too far (teleport) and bring bot
                case FSB_EVENT_HIRED_CHECK_TELEPORT:
                {
                    if(botMoveState != FSB_MOVE_STATE_STAY || !me->HasAura(SPELL_SPECIAL_GHOST))
                        FSBTeleport::BotTeleport(me, BOT_TOO_FAR);

                    if (FSBPet::BotHasPet(me))
                        FSBTeleport::BotPetTeleport(me);

                    break;
                }

                case FSB_EVENT_HIRED_WAIT_HEALER_RESSURECT:
                    if (me->GetMap()->IsDungeon())
                    {
                        events.ScheduleEvent(FSB_EVENT_HIRED_TELEPORT_DUNGEON, 4s, 6s);
                    } else
                    me->AI()->DoAction(FSB_ACTION_TELEPORT_GRAVEYARD);
                    break;

                case FSB_EVENT_HIRED_TELEPORT_DUNGEON:
                    FSBTeleport::BotTeleport(me, BOT_DEATH);
                    events.ScheduleEvent(FSB_EVENT_HIRED_DUNGEON_RESSURECT, 1s);
                    break;

                case FSB_EVENT_HIRED_DUNGEON_RESSURECT:
                    FSBDeath::HandleDeathInDungeon(me, botFollowDistance, botFollowAngle);
                    break;

                    // Check if Bot needs to mount or dismount
                case FSB_EVENT_HIRED_CHECK_MOUNT:
                    FSBMovement::BotSetMountedState(me, botMounted);
                    break;

                // =================================================== //
                // ========= COMBAT MAINTENANCE ====================== //
                // Events that are checked during combat               //
                // =================================================== //
                    
                case FSB_EVENT_COMBAT_MAINTENANCE:
                {
                    if (me->IsAlive() && me->IsInCombat())
                        FSBIC::BotICActions(me, botManaPotionUsed, botHealthPotionUsed, botGlobalCooldown, botCastedCombatBuffs, botLogicalGroup);

                    if (botClass == FSB_Class::Hunter && FSBPet::BotHasPet(me))
                        FSBPet::DoAttackSpell(me);

                    events.ScheduleEvent(FSB_EVENT_COMBAT_MAINTENANCE, 1s);

                    //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Event Combat Maintenance Reached the end"); // TEMP LOG

                    break;
                }

                // =================================================== //
                // ========= TIMED ACTION EVENTS ===================== //
                // Events that are needed for actions                  //
                // =================================================== //   

                case FSB_EVENT_SOULSTONE_RESSURECT:
                    FSBDeath::HandleDeathWithSoulstone(me, botHasSoulstone);
                    break;

                case FSB_EVENT_TELEPORT_GRAVEYARD:
                    FSBTeleport::BotTeleport(me, BOT_DEATH);
                    events.ScheduleEvent(FSB_EVENT_GRAVEYARD_RESSURECT, 2s);
                    break;

                case FSB_EVENT_GRAVEYARD_RESSURECT:
                    FSBDeath::HandleDeathWithGraveyard(me, botCorpsePos);
                    break;

                default:
                    break;
                }
            }

            while (uint32 eventId = botEvents.ExecuteEvent())
            {
                HandleBotEvent(this, eventId);
            }

        }
        
        private:
            EventMap events;
            ObjectGuid _playerGuid;


            uint32 hireTimeLeft = 0;

            // Bot Stats
            uint32 _nextRegenMs = 0;

            // ----------
            // Combat
            ObjectGuid _lastOwnerVictim;

            uint32 _5secondsCheckMs = 0;
            uint32 _1secondsCheckMs = 0;            
    };

    

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_followship_botsAI(creature);
    }
};


void AddSC_followship_bots()
{
    FollowshipBotsConfig::Load();
    FSBMgr::Get()->LoadAllPersistentBots();
    FSBSpells::InitBotSpellTables();

    new npc_followship_bots();
}
