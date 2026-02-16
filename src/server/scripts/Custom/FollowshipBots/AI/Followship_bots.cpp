// =========================================
// TO-DO
// =========================================


#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"

#include "Followship_bots.h"
#include "Followship_bots_ai_base.h"
#include "Followship_bots_config.h"

#include "Followship_bots_druid.h"
#include "Followship_bots_paladin.h"
#include "Followship_bots_priest.h"
#include "Followship_bots_mage.h"
#include "Followship_bots_warlock.h"
#include "Followship_bots_warrior.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_utils.h"

#include "Followship_bots_auras_handler.h"
#include "Followship_bots_death_handler.h"
#include "Followship_bots_events_handler.h"
#include "Followship_bots_gossip_handler.h"
#include "Followship_bots_group_handler.h"
#include "Followship_bots_incombat_handler.h"
#include "Followship_bots_movement_handler.h"
#include "Followship_bots_outofcombat_handler.h"
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
                FSBUtilsCombatSpells::InitBotSpellTables();
                tablesInitialized = true;
            }

            FSBUtilsCombatSpells::InitSpellRuntime(me, _runtimeSpells);            
        }        

        // Bot Operations

        bool updateFollowInfo = false;
        

        bool _pendingResurrection = false;
        bool _announceMemberDead = false;
        //uint32 _nextCombatSayMs = 0;

        bool _botOutCombat = false;
        bool _botInCombat = false;

        bool _playerCombatStarted = false;
        bool _playerCombatEnded = false;

        void InitializeAI() override // Runs once after creature is spawned and AI not loaded
        {
            ScriptedAI::InitializeAI(); // always call base first            
        }

        void Reset() override // Runs at creature respawn, evade or when triggered
        {
            if (FollowshipBotsConfig::configFSBEnabled)
            {
                events.Reset();

                FSBUtils::SetInitialState(me, botHired, botMoveState);
                FSBUtils::SetBotClassAndRace(me, botClass, botRace);
                FSBStats::ApplyBotBaseClassStats(me, FSBUtils::GetBotClassForEntry(me->GetEntry()));

                botHasDemon = false;
                botStats = FSBBotStats();
                botFollowDistance = frand(2.f, 8.f);
                botFollowAngle = frand(0.0f, float(M_PI * 2.0f));

                TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Reset() triggered for bot: {}", me->GetName());

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

                    TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Reset() Using Custom Regen for bot: {}", me->GetName());
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
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: IsSummonedBy() triggered for bot: {}", me->GetName());
        }

        void JustAppeared() override // Runs once when creature appeared in world, works for DB spawns
        {
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: JustAppeared() triggered for bot: {}", me->GetName());
        }

        bool OnGossipHello(Player* player) override // Runs once when opening creature gossip
        {
            return FSBGossip::HandleDefaultGossipHello(me, player, botHired, _playerGuid);
        }

        bool OnGossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) override // Runs once when gossip item selected
        {
            int32 pLevel = player->GetLevel();

            uint32 const action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);
            ClearGossipMenuFor(player);

            switch (action)
            {
                // Hire Menu
            case GOSSIP_ACTION_INFO_DEF + 1:
                return FSBGossip::HandleGossipMenuHire(me, player);

                // Bot Info
            case GOSSIP_ACTION_INFO_DEF + 2:
            {
                FSBGossip::HandleGossipItemInfo(me, player);
                return true;
            }

                // Roles Menu
            case GOSSIP_ACTION_INFO_DEF + 3:
                return FSBGossip::HandleGossipMenuRoles(me, player);

                // Bot Gossip Back to main
            case GOSSIP_ACTION_INFO_DEF + 4:
            {
                OnGossipHello(player);
                return true;
            }

                // Bot Dismiss
            case GOSSIP_ACTION_INFO_DEF + 5:
            {
                events.ScheduleEvent(FSB_EVENT_HIRE_DISMISSED, 1s);
                break;
            }

                // Bot Instructions Menu
            case GOSSIP_ACTION_INFO_DEF + 6:
                return FSBGossip::HandleGossipMenuInstructions(me, player);

                // Bot Follow Distance Menu
            case GOSSIP_ACTION_INFO_DEF + 7:
            {
                return FSBGossip::HandleGossipMenuFollowDistance(me, player);
            }

                // Bot Follow Angle Menu
            case GOSSIP_ACTION_INFO_DEF + 8:
                return FSBGossip::HandleGossipMenuFollowAngle(me, player);

                // Bot Hire Option 1
            case GOSSIP_ACTION_INFO_DEF + 10:
            {
                // End price is base cost per level times player level times duration of contract
                if (!FSBUtils::TryChargeHire(player, FollowshipBotsConfig::configFSBHireDuration1))
                {
                    player->PlayerTalkClass->SendCloseGossip();
                    return true;
                }

                botHired = true;
                //FSBMgr::Get()->HandleBotHire(player, me, FollowshipBotsConfig::configFSBHireDuration1);
                FSBMgr::Get()->HirePersistentBot(player, me, FollowshipBotsConfig::configFSBHireDuration1);
                //FSBStats::RecalculateStats(me, true, true);

                events.ScheduleEvent(FSB_EVENT_HIRE_EXPIRED, std::chrono::minutes(FollowshipBotsConfig::configFSBHireDuration1 * 60));
                events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 100ms);

                std::string msg = FSBUtilsTexts::BuildNPCSayText(player->GetName(), FollowshipBotsConfig::configFSBHireDuration1, FSBSayType::Hire, "");
                me->Say(msg, LANG_UNIVERSAL);

                player->PlayerTalkClass->SendCloseGossip();
                return true;
            }

                // Bot Hire Option 2
            case GOSSIP_ACTION_INFO_DEF + 11:
            {
                // End price is base cost per level times player level times duration of contract
                if (!FSBUtils::TryChargeHire(player, FollowshipBotsConfig::configFSBHireDuration2))
                {
                    player->PlayerTalkClass->SendCloseGossip();
                    return true;
                }

                botHired = true;
                FSBMgr::Get()->HirePersistentBot(player, me, FollowshipBotsConfig::configFSBHireDuration2);
                //FSBStats::RecalculateStats(me, true, true);

                events.ScheduleEvent(FSB_EVENT_HIRE_EXPIRED, std::chrono::minutes(FollowshipBotsConfig::configFSBHireDuration2 * 60));
                events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 100ms);

                std::string msg = FSBUtilsTexts::BuildNPCSayText(player->GetName(), FollowshipBotsConfig::configFSBHireDuration2, FSBSayType::Hire, "");
                me->Say(msg, LANG_UNIVERSAL);

                player->PlayerTalkClass->SendCloseGossip();
                return true;
            }

                // Bot Hire Option 3
            case GOSSIP_ACTION_INFO_DEF + 12:
            {
                // End price is base cost per level times player level times duration of contract
                if (!FSBUtils::TryChargeHire(player, FollowshipBotsConfig::configFSBHireDuration3))
                {
                    player->PlayerTalkClass->SendCloseGossip();
                    return true;
                }

                botHired = true;
                FSBMgr::Get()->HirePersistentBot(player, me, FollowshipBotsConfig::configFSBHireDuration3);
                //FSBStats::RecalculateStats(me, true, true);

                events.ScheduleEvent(FSB_EVENT_HIRE_EXPIRED, std::chrono::minutes(FollowshipBotsConfig::configFSBHireDuration3 * 60));
                events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 100ms);

                std::string msg = FSBUtilsTexts::BuildNPCSayText(player->GetName(), FollowshipBotsConfig::configFSBHireDuration3, FSBSayType::Hire, "");
                me->Say(msg, LANG_UNIVERSAL);

                player->PlayerTalkClass->SendCloseGossip();
                return true;
            }

                // Bot Hire Option Permanent
            case GOSSIP_ACTION_INFO_DEF + 13:
            {
                // End price is base cost per level times player level
                if (player->HasEnoughMoney(int64(FollowshipBotsConfig::configFSBPermanentPricePerLevel * pLevel)))
                {
                    player->ModifyMoney(-int64(FollowshipBotsConfig::configFSBPermanentPricePerLevel * pLevel));
                    player->GetSession()->SendNotification(FSB_PLAYER_NOTIFICATION_PAYMENT_SUCCESS);

                    botHired = true;
                    FSBMgr::Get()->HirePersistentBot(player, me, 0);
                    //FSBStats::RecalculateStats(me, true, true);

                    events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 100ms);

                    std::string msg = FSBUtilsTexts::BuildNPCSayText(player->GetName(), NULL, FSBSayType::PHire, "");
                    me->Say(msg, LANG_UNIVERSAL);
                }
                else
                    player->GetSession()->SendNotification(FSB_PLAYER_NOTIFICATION_PAYMENT_FAIL);

                player->PlayerTalkClass->SendCloseGossip();
                return true;
            }

                // Bot Follow Distance Option 1
            case GOSSIP_ACTION_INFO_DEF + 14:
            {
                botFollowDistance = FOLLOW_DISTANCE_CLOSE;
                updateFollowInfo = true;
                events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 100ms);
                break;
            }

                // Bot Follow Distance Option 2
            case GOSSIP_ACTION_INFO_DEF + 15:
            {
                botFollowDistance = FOLLOW_DISTANCE_NORMAL;
                updateFollowInfo = true;
                events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 100ms);
                break;
            }

                // Bot Follow Distance Option 3
            case GOSSIP_ACTION_INFO_DEF + 16:
            {
                botFollowDistance = FOLLOW_DISTANCE_WIDE;
                updateFollowInfo = true;
                events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 100ms);
                break;
            }

                // Bot Follow Angle Option 1
            case GOSSIP_ACTION_INFO_DEF + 17:
            {
                botFollowAngle = FOLLOW_ANGLE_FRONT;
                updateFollowInfo = true;
                events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 1s);
                break;
            }

                // Bot Follow Angle Option 2
            case GOSSIP_ACTION_INFO_DEF + 18:
            {
                botFollowAngle = FOLLOW_ANGLE_BEHIND;
                updateFollowInfo = true;
                events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 1s);
                break;
            }

                // Bot Follow Angle Option 3
            case GOSSIP_ACTION_INFO_DEF + 19:
            {
                botFollowAngle = FSBUtils::GetRandomRightAngle(); //FOLLOW_ANGLE_RIGHT;
                updateFollowInfo = true;
                events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 1s);
                break;
            }

                // Bot Stay Option
            case GOSSIP_ACTION_INFO_DEF + 20:
            {
                events.ScheduleEvent(FSB_EVENT_MOVE_STAY, 1s, 3s);
                break;
            }

                // Bot Follow Option
            case GOSSIP_ACTION_INFO_DEF + 21:
            {
                events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 1s, 3s);
                break;
            }

                // Bot Follow Angle Option 4
            case GOSSIP_ACTION_INFO_DEF + 22:
            {
                botFollowAngle = FSBUtils::GetRandomLeftAngle(); //FOLLOW_ANGLE_LEFT;
                updateFollowInfo = true;
                events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 1s);
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

        void JustEngagedWith(Unit* who) override // Runs every time creature gets in combat
        {
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Entered JustEngagedWith: {}", who->GetName());

            // Target might be NULL if called from spell with invalid cast targets
            if (!who || !me->IsAlive())
                return;

            // Passive pets don't do anything
            if (me->HasReactState(REACT_PASSIVE))
                return;

            // Prevent bot from disengaging from current target
            if (me->GetVictim() && me->EnsureVictim()->IsAlive())
                return;

            // Continue to evaluate and attack if necessary
            FSBUtilsBotCombat::BotAttackStart(me, who, botMoveState);

            //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: OnBotOwnerAttacked target: {}, move state: {}", who->GetName(), moveState);
        }

        void JustEnteredCombat(Unit* /*who*/) override
        {
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: JustEnteredCombat Triggered");
        }

        void JustExitedCombat() override
        {
            FSBUtilsBotCombat::BotHandleReturnMovement(me, botMoveState, botFollowDistance, botFollowAngle); // Return
        }        

        void AttackStart(Unit* /*target*/) override
        {

        }

        void HealReceived(Unit* /*done_by*/, uint32& /*addhealth*/) override
        {
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Heal Received");
        }

        void DamageDealt(Unit* /*victim*/, uint32& damage, DamageEffectType /*damageType*/) override
        {
            FSBPowers::GenerateRageFromDamageDone(me, damage);

            damage = uint32(damage * FSBStats::ApplyBotDamageDoneReduction(me));
        }

        // Runs every time creature takes damage
        void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            FSBPowers::GenerateRageFromDamageTaken(me, damage);

            damage = uint32(damage * FSBStats::ApplyBotDamageTakenReduction(me));

            if (!me->GetVictim() && attacker)
            {
                TC_LOG_DEBUG("scripts.ai.fsb", "FSB: DamageTaken - bot: {}, has no victim but is attacked by: {}", me->GetName(), attacker->GetName());
                FSBUtilsBotCombat::BotAttackStart(me, attacker, botMoveState);
            }
        }

        void EnterEvadeMode(EvadeReason /*why*/) override // Runs every time creature evades
        {

        }

        void KilledUnit(Unit* victim) override // Runs every time the creature kills an unit
        {
            FSBUtilsTexts::OnKilledTargetSay(me, victim);
            // Called from Unit::Kill() in case where pet or owner kills something
            // if owner killed this victim, pet may still be attacking something else
            if (me->GetVictim() && me->GetVictim() != victim)
                return;

            // Clear target just in case. May help problem where health / focus / mana
            // regen gets stuck. Also resets attack command.
            // Can't use StopAttack() because that activates movement handlers and ignores
            // next target selection
            me->AttackStop();
            me->InterruptNonMeleeSpells(false);


            // TO-DO Add autoselect for damage role
            // Before returning to owner, see if there are more things to attack
            if (Unit* nextTarget = FSBUtilsBotCombat::BotSelectNextTarget(me, false, botLogicalGroup))
                //AttackStart(nextTarget);
                FSBUtilsBotCombat::BotAttackStart(me, nextTarget, botMoveState);
            else
                FSBUtilsBotCombat::BotHandleReturnMovement(me, botMoveState, botFollowDistance, botFollowAngle); // Return

            
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

        void SpellHitTarget(WorldObject* /*target*/, SpellInfo const* /*spellInfo*/) override
        {

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
                TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Warlock summon {} for bot {} appeared", summon->GetName(), me->GetName());
            }
                
        }

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override // Runs everytime the creature's summon dies - pet or minion
        {
            botHasDemon = false;
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Warlock summon {} for bot {} died", summon->GetName(), me->GetName());
        }

        void JustDied(Unit* /*killer*/) override // Runs once when creature dies
        {
            botCorpsePos = me->GetPosition();
            FSBDeath::HandlerJustDied(me, botLogicalGroup, botHasSoulstone);
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

                TC_LOG_DEBUG("scripts.ai.fsb", "FSB SetData: Bot {} received hired value: {}", me->GetName(), botHired);

                break;
            }

            case FSB_DATA_HIRE_TIME_LEFT:
            {
                hireTimeLeft = value;

                if (hireTimeLeft > 0)
                {
                    events.ScheduleEvent(FSB_EVENT_HIRE_EXPIRED, std::chrono::seconds(hireTimeLeft));
                    events.ScheduleEvent(FSB_EVENT_RESUME_FOLLOW, 1s);

                    TC_LOG_DEBUG("scripts.ai.fsb", "FSB SetData: Bot {} received hired left time value: {}", me->GetName(), hireTimeLeft);
                }
                else
                {
                    events.ScheduleEvent(FSB_EVENT_RESUME_FOLLOW, 1s);

                    TC_LOG_DEBUG("scripts.ai.fsb", "FSB SetData: Bot {} received permanent hired", me->GetName());
                }

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
                events.ScheduleEvent(FSB_EVENT_TELEPORT_GRAVEYARD, 4s, 6s);
                break;

            case FSB_ACTION_TELEPORT_DUNGEON:
                events.ScheduleEvent(FSB_EVENT_HIRED_TELEPORT_DUNGEON, 4s, 6s);
                break;
            
            default:
                break;
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (me->IsInCombat() && !me->HasUnitState(UNIT_STATE_CASTING))
            {
                Unit* victim = me->GetVictim();

                if (!victim || !victim->IsAlive())
                {
                    Unit* newTarget = FSBUtilsBotCombat::BotSelectNextTarget(me, true, botLogicalGroup);
                    if (newTarget)
                    {
                        //AttackStart(newTarget);
                        FSBUtilsBotCombat::BotAttackStart(me, newTarget, botMoveState);
                        TC_LOG_DEBUG("scripts.ai.fsb",
                            "FSB: Bot {} selected new target {}",
                            me->GetName(), newTarget->GetName());
                    }
                    else
                    {
                        me->AttackStop();
                        me->CombatStop();
                        Player* owner = FSBMgr::Get()->GetBotOwner(me);

                        if(owner)
                            FSBUtilsBotCombat::BotHandleReturnMovement(me, botMoveState, botFollowDistance, botFollowAngle); // Return
                    }
                }
                
            }

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
                                events.ScheduleEvent(FSB_EVENT_RESUME_FOLLOW, std::chrono::milliseconds(botGlobalCooldown - now));

                            // ? lock regen for next 2 seconds
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

                        events.ScheduleEvent(FSB_EVENT_HIRED_CHECK_OWNER_COMBAT, 500ms);
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

                    // Checks to determine if owner is attacked or attacks target
                    // This is used to determine when assist is needed
                case FSB_EVENT_HIRED_CHECK_OWNER_COMBAT:
                {
                    Player* owner = FSBMgr::Get()->GetBotOwner(me);

                    if (owner && me->IsAlive())
                    {
                        // OWNER ATTACKED SOMEONE
                        if (FSBUtilsOwnerCombat::CheckBotOwnerAttacked(FSBMgr::Get()->GetBotOwner(me), _lastOwnerVictim))
                        {
                            //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Owner attacked a new target");

                            FSBUtilsOwnerCombat::OnBotOwnerAttacked(FSBMgr::Get()->GetBotOwner(me)->GetVictim(), me, botMoveState);
                        }

                        // OWNER WAS ATTACKED
                        if (Unit* attacker = FSBUtilsOwnerCombat::CheckBotOwnerAttackedBy(FSBMgr::Get()->GetBotOwner(me)))
                        {
                            //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Owner was attacked by {}", attacker->GetName());

                            FSBUtilsOwnerCombat::OnBotOwnerAttackedBy(attacker, me, botMoveState);
                        }
                    }

                    break;
                }

                    // Check if player is too far (teleport) and bring bot
                case FSB_EVENT_HIRED_CHECK_TELEPORT:
                {
                    if(botMoveState != FSB_MOVE_STATE_STAY || !me->HasAura(SPELL_SPECIAL_GHOST))
                        FSBTeleport::BotTeleport(me, BOT_TOO_FAR);
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

                    // Check if someone in our group died (bot or player)
                case FSB_EVENT_HIRED_CHECK_RESS_TARGETS:
                {
                    uint32 now = getMSTime();
                    if (now >= _1secondsCheckMs)
                    {

                        if (!_pendingResurrection && me->IsAlive() && !_pendingResTarget)
                        {
                            Unit* deadTarget = FSBGroup::BotGetFirstDeadMember(botLogicalGroup);

                            // Validate pointer before doing anything else
                            if (!deadTarget || !deadTarget->IsInWorld() || deadTarget->IsDuringRemoveFromWorld())
                                return;

                            // Store GUID early (safe even if unit despawns later)
                            _pendingResurrection = true;
                            _pendingResTarget = deadTarget->GetGUID();

                            // Build safe names for logging and chatter
                            const char* botName = (me && me->IsInWorld()) ? me->GetName().c_str() : "";
                            const char* targetName = (deadTarget && deadTarget->IsInWorld()) ? deadTarget->GetName().c_str() : "";

                            // Announce death (only once)
                            if (!_announceMemberDead && me->IsAlive() &&
                                urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
                            {
                                std::string msg = FSBUtilsTexts::BuildNPCSayText(
                                    targetName, 0, FSBSayType::PlayerOrMemberDead, "");
                                me->Yell(msg, LANG_UNIVERSAL);
                            }

                            _announceMemberDead = true;

                            // Safe logging
                            TC_LOG_DEBUG("scripts.ai.fsb",
                                "FSB: {} found dead unit {} for resurrection",
                                botName, targetName);

                            // Schedule resurrection
                            events.ScheduleEvent(FSB_EVENT_HIRED_RESS_TARGET, 3s, 5s);

                            break;
                        }
                    }
                    break;
                }

                case FSB_EVENT_HIRED_RESS_TARGET:
                {
                    if (!_pendingResurrection)
                        break;

                    if (!_pendingResTarget)
                        break;

                    Unit* target = ObjectAccessor::GetUnit(*me, _pendingResTarget);

                    if (!target || target->IsAlive())
                    {
                        _pendingResurrection = false;
                        break;
                    }

                    if (!me->IsInCombat() && !me->HasUnitState(UNIT_STATE_CASTING))
                    {

                        if (me->GetMapId() == target->GetMapId() && me->GetDistance(target) > 30.0f)
                        {
                            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Ressurect target {} too far from bot: {}", target->GetName(), me->GetName());
                            me->GetMotionMaster()->MoveChase(target, 28.f);
                        }
                        else
                        {

                            uint32 now = getMSTime();

                            FSBMovement::StopFollow(me);

                            uint32 spellId = 0;

                            switch (botClass)
                            {
                            case FSB_Class::Priest:
                                spellId = SPELL_PRIEST_RESURRECTION;
                                break;
                            case FSB_Class::Druid:
                                break;
                            case FSB_Class::Paladin:
                                spellId = SPELL_PALADIN_REDEMPTION;
                                break;
                            default:
                                break;
                            }


                            me->CastSpell(target, spellId, false);
                            botGlobalCooldown = now + NPC_GCD_MS;

                            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Ressurect Bot {} tried ressurect spell: {} on {}", me->GetName(), spellId, target->GetName());

                            events.ScheduleEvent(FSB_EVENT_RESUME_FOLLOW, 5s);
                        }
                    }
                    else events.ScheduleEvent(FSB_EVENT_HIRED_RESS_TARGET, 3s, 5s);

                    break;
                }

                // =================================================== //
                // ========= COMBAT MAINTENANCE ====================== //
                // Events that are checked during combat               //
                // =================================================== //
                    
                case FSB_EVENT_COMBAT_MAINTENANCE:
                {
                    if (me->IsAlive() && me->IsInCombat())
                        FSBIC::BotICActions(me, botManaPotionUsed, botHealthPotionUsed, botGlobalCooldown, botCastedCombatBuffs, botLogicalGroup);

                    Player* player = FSBMgr::Get()->GetBotOwner(me);

                    // 1. Player check combat state for initial buffs
                    if (player && player->IsAlive() && player->IsInCombat())
                    {
                        _ownerWasInCombat = true;
                    }
                    else if (player && player->IsAlive() && !player->IsInCombat())
                    {
                        _ownerWasInCombat = false;
                    }

                    // 2. Combat spells loop
                    if (me->IsInCombat())
                    {
                        if (!me->HasUnitState(UNIT_STATE_CASTING))
                        {
                            // IC Actions (ex mana potions or health potions)
                            //events.ScheduleEvent(FSB_EVENT_COMBAT_IC_ACTIONS, 100ms);
                            // Regular spells
                            events.ScheduleEvent(FSB_EVENT_COMBAT_SPELL_CHECK, 1s);
                        }
                    }

                    // 3. Complete off Combat State check
                    if(!FSBUtilsCombat::IsCombatActive(me))
                    {
                        _appliedInitialCBuffs = 0;
                    }

                    events.ScheduleEvent(FSB_EVENT_COMBAT_MAINTENANCE, 1s);

                    //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Event Combat Maintenance Reached the end"); // TEMP LOG

                    break;
                }

                case FSB_EVENT_COMBAT_SPELL_CHECK:
                {
                    std::vector<FSBSpellRuntime*> available = FSBUtilsCombatSpells::BotGetAvailableSpells(me, _runtimeSpells, botGlobalCooldown);

                    Unit* target = nullptr;

                    FSBSpellRuntime* toCast = FSBUtilsCombatSpells::BotSelectSpell(me, available, botLogicalGroup, target);

                    if(toCast && target)
                        FSBUtilsCombatSpells::BotCastSpell(me, target, toCast, botGlobalCooldown);

                    break;
                }

                case FSB_EVENT_COMBAT_IC_ACTIONS:
                {
                    if (!me->IsAlive())
                        break;

                    if (!me->IsInCombat())
                        break;

                    if (!me->HasUnitState(UNIT_STATE_CASTING))
                    {
                        DoAction(FSB_ACTION_COMBAT_IC_ACTIONS);
                    }

                    break;
                }

                // =================================================== //
                // ========= TIMED ACTION EVENTS ===================== //
                // Events that are needed for actions                  //
                // =================================================== //   

                    // BOT Follows player again
                    // For timed events
                case FSB_EVENT_RESUME_FOLLOW:
                    FSBMovement::ResumeFollow(me, botFollowDistance, botFollowAngle);
                    break;

                case FSB_EVENT_HIRE_EXPIRED:
                case FSB_EVENT_HIRE_DISMISSED:
                {
                    if (!me->IsInCombat() || !me->HasUnitState(UNIT_STAND_STATE_SIT))
                    {
                        events.Reset();
                        botHired = false;
                        FSBMgr::Get()->DismissPersistentBot(me);
                        events.ScheduleEvent(FSB_EVENT_HIRE_LEAVE, 5s);
                        break;
                    }
                    else events.ScheduleEvent(FSB_EVENT_HIRE_DISMISSED, 5s);                                     
                    break;
                }

                case FSB_EVENT_HIRE_LEAVE:
                {
                    //float x, y, z;
                    //me->GetRandomPoint(me->GetPosition(), 10.0f, x, y, z); // 10 yd radius
                    //me->GetMotionMaster()->MovePoint(1, me->GetPositionX() + 30, me->GetPositionY(), me->GetPositionZ(), true);
                    float x, y, z;
                    me->GetRandomPoint(me->GetPosition(), 30.0f, x, y, z);
                    me->GetMotionMaster()->MovePoint(1, x, y, z);
                    me->DespawnOrUnsummon(10s);
                    
                    break;
                }

                case FSB_EVENT_MOVE_STAY:
                {
                    Unit* owner = me->GetOwner();
                    Player* player = owner ? owner->ToPlayer() : nullptr;

                    FSBMovement::StopFollow(me);

                    if (botHired && player)
                    {
                        std::string msg = FSBUtilsTexts::BuildNPCSayText(player->GetName(), NULL, FSBSayType::Stay, "");
                        me->Say(msg, LANG_UNIVERSAL);
                    }
                    break;
                }

                case FSB_EVENT_MOVE_FOLLOW:
                {
                    Unit* owner = me->GetOwner();
                    Player* player = owner ? owner->ToPlayer() : nullptr;

                    FSBMovement::ResumeFollow(me, botFollowDistance, botFollowAngle);

                    if (updateFollowInfo)
                    {
                        updateFollowInfo = false;
                        me->Say(FSB_SAY_FOLLOW_INFO_CHANGED, LANG_UNIVERSAL);
                    }
                    else
                    {
                        std::string pName = player ? player->GetName() : "";
                        std::string msg = FSBUtilsTexts::BuildNPCSayText(pName, NULL, FSBSayType::Follow, "");
                        me->Say(msg, LANG_UNIVERSAL);
                    }
                    break;
                }

                case FSB_EVENT_SOULSTONE_RESSURECT:
                    FSBDeath::HandleDeathWithSoulstone(me, botHasSoulstone);
                    break;

                case FSB_EVENT_TELEPORT_GRAVEYARD:
                    FSBTeleport::BotTeleport(me, BOT_DEATH);
                    events.ScheduleEvent(FSB_EVENT_GRAVEYARD_RESSURECT, 1s);
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

        // Helper methods

        uint8 attackers = FSBUtilsCombat::CountActiveAttackers(me); // count active attackers on bot

        
        private:
            EventMap events;
            ObjectGuid _playerGuid;


            uint32 hireTimeLeft = 0;

            // Bot Stats
            uint32 _nextRegenMs = 0;

            // ----------
            // Combat
            ObjectGuid _pendingResTarget;
            ObjectGuid _lastOwnerVictim;
            bool _ownerWasInCombat = false;
            uint8 _appliedInitialCBuffs = 0;
            std::vector<FSBSpellRuntime> _runtimeSpells; // runtime for spells cooldowns
            uint32 _buffsTimerMs = 0;
            uint32 _selfBuffsTimerMs = 0;

            // ----------
            // Movement States
            //uint16 moveState = 0;

            // ----------
            // Allies & Group
            //std::vector<Unit*> botGroup_;

            uint32 _60secondsCheckMs = 0;
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
    FSBUtilsCombatSpells::InitBotSpellTables();

    new npc_followship_bots();
}
