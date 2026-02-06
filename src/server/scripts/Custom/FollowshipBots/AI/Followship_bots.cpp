// =========================================
// TO-DO
// =========================================


#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"

#include "Followship_bots.h"
#include "Followship_bots_ai_base.h"
#include "Followship_bots_config.h"
#include "Followship_bots_paladin.h"
#include "Followship_bots_priest.h"
#include "Followship_bots_mage.h"
#include "Followship_bots_warrior.h"
#include "Followship_bots_utils_combat.h"
#include "Followship_bots_utils_gossip.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_utils_spells.h"

#include "Followship_bots_outofcombat_handler.h"
#include "Followship_bots_powers_handler.h"
#include "Followship_bots_recovery_handler.h"
#include "Followship_bots_regen_handler.h"
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
        bool hired = false;

        bool updateFollowInfo = false;
        float followDistance = frand(2.f, 8.f);
        float followAngle = frand(0.0f, float(M_PI * 2.0f));

        bool _pendingResurrection = false;
        bool _announceMemberDead = false;
        //uint32 _nextCombatSayMs = 0;

        bool _botOutCombat = false;
        bool _botInCombat = false;

        bool _playerCombatStarted = false;
        bool _playerCombatEnded = false;

        bool _botManaPotionUsed = false;
        bool _botIsDrinking = false;

        void InitializeAI() override // Runs once after creature is spawned and AI not loaded
        {
            ScriptedAI::InitializeAI(); // always call base first            
        }

        void Reset() override // Runs at creature respawn, evade or when triggered
        {
            if (FollowshipBotsConfig::configFSBEnabled)
            {
                events.Reset();

                _statsMods = FSBUtilsStatsMods();     // now resets caller state

                FSBUtils::SetInitialState(me, hired, botMoveState);
                FSBUtils::SetBotClassAndRace(me, botClass, botRace);
                FSBUtilsStats::ApplyBotBaseClassStats(me, FSBUtils::GetBotClassForEntry(me->GetEntry()));

                _recoveryActions.clear();

                FSBRecovery::BuildRecoveryActions(me, _recoveryActions);

                _nextRecoveryCheckMs = 0;

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
                    _baseStatsMods = FSBUtilsStatsMods(); // zero

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
            //me->SetHealth(me->GetMaxHealth());
            //me->SetPower(me->GetPowerType(), me->GetMaxPower(me->GetPowerType()));
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: JustAppeared() triggered for bot: {}", me->GetName());
        }

        bool OnGossipHello(Player* player) override // Runs once when opening creature gossip
        {
            return FSBUtilsGossip::HandleDefaultGossipHello(me, player, hired, _playerGuid);
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
                return FSBUtilsGossip::HandleHireGossipSelect(me, player);

                // Bot Info
            case GOSSIP_ACTION_INFO_DEF + 2:
            {
                FSBUtilsGossip::HandleInfoGossipSelect(me, player);
                return true;
            }

                // Roles Menu
            case GOSSIP_ACTION_INFO_DEF + 3:
                return FSBUtilsGossip::HandleRolesGossipSelect(me, player);

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
                return FSBUtilsGossip::HandleInstructionsGossipSelect(me, player);

                // Bot Follow Distance Menu
            case GOSSIP_ACTION_INFO_DEF + 7:
            {
                return FSBUtilsGossip::HandleFollowDistanceGossipSelect(me, player);
            }

                // Bot Follow Angle Menu
            case GOSSIP_ACTION_INFO_DEF + 8:
                return FSBUtilsGossip::HandleFollowAngleGossipSelect(me, player);

                // Bot Hire Option 1
            case GOSSIP_ACTION_INFO_DEF + 10:
            {
                // End price is base cost per level times player level times duration of contract
                if (!FSBUtils::TryChargeHire(player, FollowshipBotsConfig::configFSBHireDuration1))
                {
                    player->PlayerTalkClass->SendCloseGossip();
                    return true;
                }

                hired = true;
                FSBMgr::HandleBotHire(player, me, FollowshipBotsConfig::configFSBHireDuration1);
                FSBUtilsStats::RecalculateMods(me, _statsMods);

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

                hired = true;
                FSBMgr::HandleBotHire(player, me, FollowshipBotsConfig::configFSBHireDuration2);
                FSBUtilsStats::RecalculateMods(me, _statsMods);

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

                hired = true;
                FSBMgr::HandleBotHire(player, me, FollowshipBotsConfig::configFSBHireDuration3);
                FSBUtilsStats::RecalculateMods(me, _statsMods);

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

                    hired = true;
                    FSBMgr::HandleBotHire(player, me, 0);
                    FSBUtilsStats::RecalculateMods(me, _statsMods);

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
                followDistance = FOLLOW_DISTANCE_CLOSE;
                updateFollowInfo = true;
                events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 100ms);
                break;
            }

                // Bot Follow Distance Option 2
            case GOSSIP_ACTION_INFO_DEF + 15:
            {
                followDistance = FOLLOW_DISTANCE_NORMAL;
                updateFollowInfo = true;
                events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 100ms);
                break;
            }

                // Bot Follow Distance Option 3
            case GOSSIP_ACTION_INFO_DEF + 16:
            {
                followDistance = FOLLOW_DISTANCE_WIDE;
                updateFollowInfo = true;
                events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 100ms);
                break;
            }

                // Bot Follow Angle Option 1
            case GOSSIP_ACTION_INFO_DEF + 17:
            {
                followAngle = FOLLOW_ANGLE_FRONT;
                updateFollowInfo = true;
                events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 1s);
                break;
            }

                // Bot Follow Angle Option 2
            case GOSSIP_ACTION_INFO_DEF + 18:
            {
                followAngle = FOLLOW_ANGLE_BEHIND;
                updateFollowInfo = true;
                events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 1s);
                break;
            }

                // Bot Follow Angle Option 3
            case GOSSIP_ACTION_INFO_DEF + 19:
            {
                followAngle = FSBUtils::GetRandomRightAngle(); //FOLLOW_ANGLE_RIGHT;
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
                followAngle = FSBUtils::GetRandomLeftAngle(); //FOLLOW_ANGLE_LEFT;
                updateFollowInfo = true;
                events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 1s);
                break;
            }

                // Bot Role Option 1
            case GOSSIP_ACTION_INFO_DEF + 23:
            {
                switch (botClass)
                {
                case FSB_Class::Paladin:
                    FSBUtils::SetRole(me, FSB_Roles::FSB_ROLE_TANK);
                    FSBPaladin::BotSetRoleAuras(me, FSB_ROLE_TANK);
                    break;
                case FSB_Class::Warrior:
                    FSBUtils::SetRole(me, FSB_Roles::FSB_ROLE_TANK);
                    me->CastSpell(me, SPELL_WARRIOR_DEFENSIVE_STANCE);
                    break;
                case FSB_Class::Priest:
                    FSBUtils::SetRole(me, FSB_Roles::FSB_ROLE_HEALER);
                    if (me->HasAura(SPELL_PRIEST_SHADOWFORM))
                        me->RemoveAurasDueToSpell(SPELL_PRIEST_SHADOWFORM);
                    break;
                case FSB_Class::Mage:
                    FSBUtils::SetRole(me, FSB_Roles::FSB_ROLE_RANGED_ARCANE);
                    break;
                default:
                    break;
                }

                me->SetReactState(REACT_AGGRESSIVE);
                me->Say(FSB_SAY_FOLLOW_INFO_CHANGED, LANG_UNIVERSAL);
                break;
            }

                // Bot Role Option 2
            case GOSSIP_ACTION_INFO_DEF + 24:
            {
                switch (botClass)
                {
                case FSB_Class::Paladin:
                    FSBUtils::SetRole(me, FSB_Roles::FSB_ROLE_MELEE_DAMAGE);
                    FSBPaladin::BotSetRoleAuras(me, FSB_ROLE_MELEE_DAMAGE);
                    break;
                case FSB_Class::Warrior:
                {
                    FSBUtils::SetRole(me, FSB_Roles::FSB_ROLE_MELEE_DAMAGE);
                    uint32 spellId = urand(0, 1) ? SPELL_WARRIOR_BATTLE_STANCE : SPELL_WARRIOR_BERSERKER_STANCE;
                    me->CastSpell(me, spellId);
                    break;
                }
                case FSB_Class::Priest:
                    FSBUtils::SetRole(me, FSB_Roles::FSB_ROLE_RANGED_DAMAGE);
                    me->CastSpell(me, SPELL_PRIEST_SHADOWFORM);
                    break;
                case FSB_Class::Mage:
                    FSBUtils::SetRole(me, FSB_Roles::FSB_ROLE_RANGED_FROST);
                    break;
                default:
                    break;
                }

                me->SetReactState(REACT_AGGRESSIVE);                 
                me->Say(FSB_SAY_FOLLOW_INFO_CHANGED, LANG_UNIVERSAL);
                break;
            }

            // Bot Role Option 3
            case GOSSIP_ACTION_INFO_DEF + 25:
            {
                switch (botClass)
                {
                case FSB_Class::Paladin:
                    FSBUtils::SetRole(me, FSB_Roles::FSB_ROLE_HEALER);
                    FSBPaladin::BotSetRoleAuras(me, FSB_ROLE_HEALER);
                    break;
                case FSB_Class::Priest:
                    FSBUtils::SetRole(me, FSB_Roles::FSB_ROLE_ASSIST);
                    if (me->HasAura(SPELL_PRIEST_SHADOWFORM))
                        me->RemoveAurasDueToSpell(SPELL_PRIEST_SHADOWFORM);
                    break;
                case FSB_Class::Mage:
                    FSBUtils::SetRole(me, FSB_Roles::FSB_ROLE_RANGED_FIRE);
                    break;
                default:
                    break;
                }

                me->SetReactState(REACT_AGGRESSIVE);                
                me->Say(FSB_SAY_FOLLOW_INFO_CHANGED, LANG_UNIVERSAL);
                break;
            }

            case GOSSIP_ACTION_INFO_DEF + 26:
                return FSBUtilsGossip::HandlePortalGossipSelect(me, player);

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
            FSBUtilsBotCombat::BotHandleReturnMovement(me, botMoveState, followDistance, followAngle); // Return
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

            damage = uint32(damage * FSBUtilsStats::ApplyBotDamageDoneReduction(me));
        }

        // Runs every time creature takes damage
        void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            FSBPowers::GenerateRageFromDamageTaken(me, damage);

            damage = uint32(damage * FSBUtilsStats::ApplyBotDamageTakenReduction(me));

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
            if (Unit* nextTarget = FSBUtilsBotCombat::BotSelectNextTarget(me, false, botGroup_))
                //AttackStart(nextTarget);
                FSBUtilsBotCombat::BotAttackStart(me, nextTarget, botMoveState);
            else
                FSBUtilsBotCombat::BotHandleReturnMovement(me, botMoveState, followDistance, followAngle); // Return

            
        }

        void OnSpellCast(SpellInfo const* spell) override // Runs every time the creature casts a spell
        {
            switch (botClass)
            {
            case FSB_Class::Warrior:
                FSBWarrior::HandleOnSpellCast(me, spell->Id);
                break;
            case FSB_Class::Priest:
                break;
            case FSB_Class::Mage:
                break;
            case FSB_Class::Rogue:
                break;
            case FSB_Class::Druid:
                break;
            case FSB_Class::Paladin:
                FSBPaladin::HandleOnSpellCast(me, spell->Id);
                break;
            case FSB_Class::Hunter:
                break;
            case FSB_Class::Warlock:
                break;
            default:
                break;
            }
        }

        void SpellHit(WorldObject* /*caster*/, SpellInfo const* spellInfo) override
        {
            if (!spellInfo)
                return;

            // When bot is resurrected we need to set it back to death state alive
            if (spellInfo->Id == SPELL_PRIEST_RESURRECTION || spellInfo->Id == SPELL_PALADIN_REDEMPTION)
            {
                TC_LOG_DEBUG("scripts.ai.fsb",
                    "FSB: Bot {} was resurrected by spell {}",
                    me->GetName(),
                    spellInfo->Id);

                me->setDeathState(ALIVE);
            }
        }

        void SpellHitTarget(WorldObject* /*target*/, SpellInfo const* /*spellInfo*/) override
        {

        }

        void OnAuraApplied(AuraApplication const* aurApp) override
        {
            if (!aurApp)
                return;

            switch (aurApp->GetBase()->GetId())
            {
            case SPELL_DRINK_CONJURED_CRYSTAL_WATER:
            {
                int32 amount = FSBRecovery::GetDrinkFood(me->GetLevel());
                _statsMods.flatManaPerTick += amount;
                break;
            }

            case SPELL_FOOD_SCALED_WITH_LVL:
            {
                int32 amount = FSBRecovery::GetDrinkFood(me->GetLevel());
                _statsMods.flatHealthPerTick += amount;
                break;
            }

            case SPELL_WARRIOR_BATTLE_SHOUT:
            {
                float totalPct = me->GetPctModifierValue(UNIT_MOD_ATTACK_POWER, TOTAL_PCT);

                me->SetStatPctModifier(UNIT_MOD_ATTACK_POWER, TOTAL_PCT, totalPct + 0.05f);
                FSBUtilsStats::RecalculateMods(me, _statsMods);

                break; 
            }

            case SPELL_WARRIOR_BATTLE_STANCE:
            {
                _statsMods.flatRagePerTick += 20;
                break;
            }

            case SPELL_PRIEST_POWER_WORD_FORTITUDE:
                _statsMods.flatMaxHealth += 5 * me->GetLevel();      // flat
                FSBUtilsStats::RecalculateMods(me, _statsMods);
                break;

            case SPELL_MAGE_ARCANE_INTELLECT:
                _statsMods.pctMaxManaBonus += 0.02f;
                FSBUtilsStats::RecalculateMods(me, _statsMods);
                break;

            case SPELL_MAGE_EVOCATION:
                _statsMods.pctManaPerTick += 1500.f;
                FSBUtilsStats::RecalculateMods(me, _statsMods);
                break;

            case SPELL_PALADIN_RITE_OF_SANCTIFICATION:
            {
                me->ApplyStatPctModifier(UNIT_MOD_ARMOR, TOTAL_PCT, 0.05f);
                FSBUtilsStats::RecalculateMods(me, _statsMods);
                break;
            }
            }

            

        }

        void OnAuraRemoved(AuraApplication const* aurApp) override
        {
            if (!aurApp)
                return;

            uint32 spellId = aurApp->GetBase()->GetId();

            // Example: generic drink spells
            switch (spellId)
            {
            case SPELL_DRINK_CONJURED_CRYSTAL_WATER: // Conjured Crystal water
            {
                if (me->GetStandState() == UNIT_STAND_STATE_SIT)
                {
                    me->SetStandState(UNIT_STAND_STATE_STAND);
                    int32 amount = FSBRecovery::GetDrinkFood(me->GetLevel());
                    _statsMods.flatManaPerTick -= amount;
                    events.ScheduleEvent(FSB_EVENT_RESUME_FOLLOW, 500ms);
                }
                break;
            }

            case SPELL_FOOD_SCALED_WITH_LVL: // Food
            {
                if (me->GetStandState() == UNIT_STAND_STATE_SIT)
                {
                    me->SetStandState(UNIT_STAND_STATE_STAND);
                    int32 amount = FSBRecovery::GetDrinkFood(me->GetLevel());
                    _statsMods.flatHealthPerTick -= amount;
                    events.ScheduleEvent(FSB_EVENT_RESUME_FOLLOW, 500ms);
                }
                break;
            }

            case SPELL_WARRIOR_BATTLE_SHOUT:
            {
                float totalPct = me->GetPctModifierValue(UNIT_MOD_ATTACK_POWER, TOTAL_PCT);

                me->SetStatPctModifier(UNIT_MOD_ATTACK_POWER, TOTAL_PCT, totalPct - 0.05f);
                FSBUtilsStats::RecalculateMods(me, _statsMods);

                break;
            }

            case SPELL_WARRIOR_BATTLE_STANCE:
            {
                _statsMods.flatRagePerTick -= 20;
                break;
            }

            case SPELL_MAGE_CONJURED_MANA_PUDDING:
            {
                if (me->GetStandState() == UNIT_STAND_STATE_SIT)
                {
                    me->SetStandState(UNIT_STAND_STATE_STAND);
                    events.ScheduleEvent(FSB_EVENT_RESUME_FOLLOW, 500ms);
                }
                break;
            }

            case SPELL_PRIEST_POWER_WORD_FORTITUDE:
                _statsMods.flatMaxHealth -= 5 * me->GetLevel();      // flat
                FSBUtilsStats::RecalculateMods(me, _statsMods);

                if (me->GetHealth() > me->GetMaxHealth())
                    me->SetHealth(me->GetMaxHealth());

                break;

            case SPELL_MAGE_ARCANE_INTELLECT:
                _statsMods.pctMaxManaBonus -= 0.02f;
                FSBUtilsStats::RecalculateMods(me, _statsMods);

                if (me->GetPower(POWER_MANA) > me->GetMaxPower(POWER_MANA))
                    me->SetPower(POWER_MANA, me->GetMaxPower(POWER_MANA));

                break;

            case SPELL_MAGE_EVOCATION:
                _statsMods.pctManaPerTick -= 1500.f;
                FSBUtilsStats::RecalculateMods(me, _statsMods);

                break;

            case SPELL_PALADIN_RITE_OF_SANCTIFICATION:
            {
                me->ApplyStatPctModifier(UNIT_MOD_ARMOR, TOTAL_PCT, -0.05f);
                FSBUtilsStats::RecalculateMods(me, _statsMods);
                break;
            }
            }
        }

        void JustSummoned(Creature* /*summon*/) override // Runs every time the creature summons another creature
        {

        }

        void SummonedCreatureDies(Creature* /*summon*/, Unit* /*killer*/) override // Runs everytime the creature's summon dies - pet or minion
        {

        }

        void JustDied(Unit* /*killer*/) override // Runs once when creature dies
        {
            Player* player = FSBMgr::GetBotOwner(me);
            if (player)
            {
                if (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
                {
                    std::string msg = FSBUtilsTexts::BuildNPCSayText(player->GetName(), NULL, FSBSayType::BotDeath, "");
                    me->Yell(msg, LANG_UNIVERSAL);
                }
            }

            events.ScheduleEvent(FSB_EVENT_HIRED_TELEPORT_DEATH, 90s);
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
                hired = value;

                TC_LOG_DEBUG("scripts.ai.fsb", "FSB SetData: Bot {} received hired value: {}", me->GetName(), hired);

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
            case FSB_ACTION_COMBAT_IC_ACTIONS:
            {
                
                if (me->IsInCombat() && me->IsAlive())
                {
                    // 1. Generic mana potions for bots with mana
                    if (me->GetMaxPower(POWER_MANA) > 0 && me->GetPowerPct(POWER_MANA) < 20)
                    {
                        if (!_botManaPotionUsed)
                        {
                            uint32 ManaPotionSpellId = FSBUtilsSpells::GetManaPotionSpellForLevel(me->GetLevel());

                            if (ManaPotionSpellId)
                            {
                                // Global Cooldown does NOT apply for potions
                                // Limit of 1 potion per type (MP or HP) per combat 
                                me->CastSpell(me, ManaPotionSpellId, false);
                                _botManaPotionUsed = true;

                                if (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
                                {
                                    std::string spellName = FSBUtilsSpells::GetSpellName(ManaPotionSpellId);
                                    std::string msg = FSBUtilsTexts::BuildNPCSayText("", NULL, FSBSayType::CombatMana, spellName);
                                    me->Say(msg, LANG_UNIVERSAL);
                                }

                                TC_LOG_DEBUG("scripts.ai.fsb", "FSB: IC Action mana potion used by bot: {} with spell id: {}", me->GetName(), ManaPotionSpellId);
                            }
                        }
                            
                    }

                    // 2. On Enter Combat spells
                    uint32 now = getMSTime();
                    if (FSBUtilsSpells::CanCastNow(me, now, _globalCooldownUntil))
                    {
                        switch (botClass)
                        {
                        case FSB_Class::Priest:
                        {
                            if (FSBUtils::GetRole(me) == FSB_Roles::FSB_ROLE_ASSIST) // self cast for Assist Role
                            {
                                if (FSBPriest::BotInitialCombatSpells(me, _globalCooldownUntil, _ownerWasInCombat, _appliedInitialCBuffs, true))
                                    break;
                            }
                            else if (FSBUtils::GetRole(me) == FSB_Roles::FSB_ROLE_HEALER) // player cast for Healer role
                            {
                                if (FSBPriest::BotInitialCombatSpells(me, _globalCooldownUntil, _ownerWasInCombat, _appliedInitialCBuffs, false))
                                    break;
                            }
                            break;
                        }
                        default:
                            break;
                        }
                    }
                }

                break;
            }
            
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
                    Unit* newTarget = FSBUtilsBotCombat::BotSelectNextTarget(me, true, botGroup_);
                    if (newTarget)
                    {
                        //AttackStart(newTarget);
                        FSBUtilsBotCombat::BotAttackStart(me, newTarget, botMoveState);
                        TC_LOG_DEBUG("scripts.ai.fsb",
                            "FSB: Bot {} selected new target {}",
                            me->GetName(), newTarget->GetName());
                    }
                    else FSBUtilsBotCombat::BotHandleReturnMovement(me, botMoveState, followDistance, followAngle); // Return
                }
                
            }

            events.Update(diff);

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
                    uint32 now = getMSTime();

                    // Check if custom regen is enabled and we can schedule every 2s
                    if (FollowshipBotsConfig::configFSBUseCustomRegen && now >= _nextRegenMs)
                    {
                        if (me->GetHealthPct() < 100 || me->GetPowerPct(me->GetPowerType()) < 100)
                        {
                            FSBRegen::ProcessBotCustomRegenTick(me, botClass, _baseStatsMods, _statsMods);

                            // ? lock regen for next 2 seconds
                            _nextRegenMs = now + 2000;
                            
                        }
                    }

                    // Check if combat is NOT taking place and schedule OOC actions
                    if (!FSBUtilsCombat::IsCombatActive(me))
                    {
                        if (now >= _1secondsCheckMs)
                        {
                            if (!_isRecovering && FSBOOC::BotOOCActions(me, _globalCooldownUntil, _buffsTimerMs, _selfBuffsTimerMs, botGroup_))
                                events.ScheduleEvent(FSB_EVENT_RESUME_FOLLOW, 1s, 3s);

                            // ? lock regen for next 2 seconds
                            _1secondsCheckMs = now + 1000;
                        }

                        if (now >= _nextRecoveryCheckMs)
                        {
                            FSBRecovery::TryRecovery(me, _recoveryActions, _globalCooldownUntil, _isRecovering, _recoveryLockUntil);
                            _nextRecoveryCheckMs = now + 1000; // once per second is plenty
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
                    if (hired)
                    {
                        uint32 now = getMSTime();

                        events.ScheduleEvent(FSB_EVENT_HIRED_CHECK_OWNER_COMBAT, 500ms);
                        events.ScheduleEvent(FSB_EVENT_HIRED_CHECK_TELEPORT, 3s, 5s);
                        events.ScheduleEvent(FSB_EVENT_HIRED_CHECK_MOUNT, 3s, 5s);

                        if(now >= _1secondsCheckMs)
                            if(FSBUtils::GetRole(me) == FSB_Roles::FSB_ROLE_HEALER)
                                events.ScheduleEvent(FSB_EVENT_HIRED_CHECK_RESS_TARGETS, 1s, 3s);

                        // After a bot is resurrected we need to determine if they are alive
                        // and then perform after ress stuff
                        if (!_pendingResTarget.IsEmpty() && _announceMemberDead)
                        {
                            Unit* target = ObjectAccessor::GetUnit(*me, _pendingResTarget);
                            if (target && target->IsAlive())
                            {
                                _announceMemberDead = false;
                                _pendingResurrection = false;

                                if (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
                                {
                                    // SAY after ressurect
                                    std::string msg = FSBUtilsTexts::BuildNPCSayText(target->GetName(), NULL, FSBSayType::Resurrect, "");
                                    me->Say(msg, LANG_UNIVERSAL);
                                }

                                _pendingResTarget.Clear();
                            }
                        }
                        

                        if (now >= _5secondsCheckMs)
                        {
                            FSBUtilsStats::UpdateBotLevelToPlayer(me, _statsMods);

                            // Check to dermine what friendlies we have in our "group"
                            // Includes: bot, owner and other bots owner by its owner
                            // TO-DO: Add check to include other players in the group of the owner
                            FSBUtilsCombat::CheckBotAllies(me->ToCreature(), botGroup_, 50.0f);

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
                    Player* owner = FSBMgr::GetBotOwner(me);

                    if (owner && me->IsAlive())
                    {
                        // OWNER ATTACKED SOMEONE
                        if (FSBUtilsOwnerCombat::CheckBotOwnerAttacked(FSBMgr::GetBotOwner(me), _lastOwnerVictim))
                        {
                            //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Owner attacked a new target");

                            FSBUtilsOwnerCombat::OnBotOwnerAttacked(FSBMgr::GetBotOwner(me)->GetVictim(), me, botMoveState);
                        }

                        // OWNER WAS ATTACKED
                        if (Unit* attacker = FSBUtilsOwnerCombat::CheckBotOwnerAttackedBy(FSBMgr::GetBotOwner(me)))
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
                    if(botMoveState != FSB_MOVE_STATE_STAY)
                        FSBTeleport::BotTeleport(me, BOT_TOO_FAR);
                    break;
                }

                    // After death bot is teleported to graveyard or dungeon entrance
                case FSB_EVENT_HIRED_TELEPORT_DEATH:
                {
                    if (FSBTeleport::BotTeleport(me, BOT_DEATH))
                    {
                        me->setDeathState(ALIVE);
                        events.ScheduleEvent(FSB_EVENT_RESUME_FOLLOW, 1s);
                    }
                    break;

                }

                    // Check if Bot needs to mount or dismount
                case FSB_EVENT_HIRED_CHECK_MOUNT:
                {
                    if (!me || !me->IsAlive())
                        break;

                    if (me->HasUnitState(UNIT_STATE_CASTING))
                        break;

                    Player* player = FSBMgr::GetBotOwner(me);

                    if (player && player->HasAuraType(SPELL_AURA_MOUNTED) && !botMounted)
                    {
                        FSBUtilsSpells::CastRandomMountLevelSpell(me);
                        botMounted = true;
                    }
                    else if (player && !player->HasAuraType(SPELL_AURA_MOUNTED) && botMounted)
                    {
                        botMounted = false;
                        me->Dismount();
                        me->RemoveAurasByType(SPELL_AURA_MOUNTED);
                    }

                    break;
                }

                    // Check if someone in our group died (bot or player)
                case FSB_EVENT_HIRED_CHECK_RESS_TARGETS:
                {
                    uint32 now = getMSTime();
                    if (now >= _1secondsCheckMs)
                    {

                        if (!_pendingResurrection && me->IsAlive() && !_pendingResTarget)
                        {
                            Unit* deadTarget = FSBUtilsSpells::FindBotDeadResTarget(me, botGroup_);

                            if (deadTarget)
                            {
                                _pendingResurrection = true;
                                _pendingResTarget = deadTarget->GetGUID();

                                // announce target died
                                // we need extra flag so chatter does not repeat
                                if (me->IsAlive() && !_announceMemberDead && (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate))
                                {
                                    std::string msg = FSBUtilsTexts::BuildNPCSayText(deadTarget->GetName(), NULL, FSBSayType::PlayerOrMemberDead, "");
                                    me->Yell(msg, LANG_UNIVERSAL);
                                }
                                _announceMemberDead = true;


                                TC_LOG_DEBUG("scripts.ai.fsb", "FSB: {} found dead unit {} for resurrection", me->GetName(), deadTarget->GetName());

                                events.ScheduleEvent(FSB_EVENT_HIRED_RESS_TARGET, 3s, 5s);

                                break;
                            }
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

                            FSBUtilsMovement::StopFollow(me);

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
                            _globalCooldownUntil = now + NPC_GCD_MS;

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

                    Player* player = FSBMgr::GetBotOwner(me);

                    // 1. Player check combat state for initial buffs
                    if (player && player->IsAlive() && player->IsInCombat())
                    {
                        _ownerWasInCombat = true;
                    }
                    else if (player && player->IsAlive() && !player->IsInCombat())
                    {
                        _ownerWasInCombat = false;
                        _appliedInitialCBuffs = 0;
                    }

                    // 2. Combat spells loop
                    if (me->IsInCombat())
                    {
                        if (!me->HasUnitState(UNIT_STATE_CASTING))
                        {
                            // IC Actions (ex mana potions or health potions)
                            events.ScheduleEvent(FSB_EVENT_COMBAT_IC_ACTIONS, 100ms);
                            // Regular spells
                            events.ScheduleEvent(FSB_EVENT_COMBAT_SPELL_CHECK, 1s);
                        }
                    }

                    // 3. Complete off Combat State check
                    if(!FSBUtilsCombat::IsCombatActive(me))
                    {
                        _botManaPotionUsed = false; // we need to reset this when combat ends so that bot can reapply on the next combat state
                        _appliedInitialCBuffs = 0;
                    }

                    events.ScheduleEvent(FSB_EVENT_COMBAT_MAINTENANCE, 1s);

                    //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Event Combat Maintenance Reached the end"); // TEMP LOG

                    break;
                }

                case FSB_EVENT_COMBAT_SPELL_CHECK:
                {
                    std::vector<FSBSpellRuntime*> available = FSBUtilsCombatSpells::BotGetAvailableSpells(me, _runtimeSpells, _globalCooldownUntil);

                    Unit* target = nullptr;

                    FSBSpellRuntime* toCast = FSBUtilsCombatSpells::BotSelectSpell(me, available, botGroup_, target);

                    if(toCast && target)
                        FSBUtilsCombatSpells::BotCastSpell(me, target, toCast, _globalCooldownUntil);

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
                {
                    Player* player = FSBMgr::GetBotOwner(me);

                    if(player)
                        ResumeFollow(player);

                    break;
                }


                case FSB_EVENT_HIRE_EXPIRED:
                case FSB_EVENT_HIRE_DISMISSED:
                {
                    Unit* owner = me->GetOwner();
                    Player* player = owner ? owner->ToPlayer() : nullptr;

                    me->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP);
                    events.Reset();
                    me->StopMoving();
                    me->GetMotionMaster()->Clear();
                    if (player)
                    {
                        std::string msg = FSBUtilsTexts::BuildNPCSayText(player->GetName(), NULL, FSBSayType::Fire, "");
                        me->Say(msg, LANG_UNIVERSAL);
                    }
                    FSBMgr::DismissBot(player, me);
                    events.ScheduleEvent(FSB_EVENT_HIRE_LEAVE, 5s);
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

                    FSBUtilsMovement::StopFollow(me);
                    botMoveState = FSB_MOVE_STATE_STAY;

                    if (hired && player)
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

                    ResumeFollow(player);
                    botMoveState = FSB_MOVE_STATE_FOLLOWING;

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





                default:
                    break;
                }
            }
        }

        // Helper methods

        // Resume Follow
        void ResumeFollow(Player* player)
        {
            if (!player)
                return;

            me->GetMotionMaster()->MoveFollow(player, followDistance, followAngle);
        }

        uint8 attackers = FSBUtilsCombat::CountActiveAttackers(me); // count active attackers on bot

        
        private:
            EventMap events;
            ObjectGuid _playerGuid;

            FSBUtilsStatsMods _statsMods;
            FSBUtilsStatsMods _baseStatsMods;


            uint32 hireTimeLeft = 0;

            bool botMounted = false;

            // Bot Stats
            uint32 _nextRegenMs = 0;

            // ----------
            // Combat
            ObjectGuid _pendingResTarget;
            ObjectGuid _lastOwnerVictim;
            bool _ownerWasInCombat = false;
            uint8 _appliedInitialCBuffs = 0;
            std::vector<FSBSpellRuntime> _runtimeSpells; // runtime for spells cooldowns
            std::vector<std::unique_ptr<BotRecoveryAction>> _recoveryActions;
            uint32 _globalCooldownUntil = 0; // global cooldown
            uint32 _buffsTimerMs = 0;
            uint32 _selfBuffsTimerMs = 0;

            // ----------
            // Movement States
            //uint16 moveState = 0;

            // ----------
            // Allies & Group
            std::vector<Unit*> botGroup_;

            uint32 _60secondsCheckMs = 0;
            uint32 _5secondsCheckMs = 0;
            uint32 _1secondsCheckMs = 0;

            uint32 _nextRecoveryCheckMs = 0;
            bool _isRecovering = false;
            uint32 _recoveryLockUntil = 0;
            
    };

    

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_followship_botsAI(creature);
    }
};


void AddSC_followship_bots()
{
    FollowshipBotsConfig::Load();
    FSBUtilsCombatSpells::InitBotSpellTables();

    new npc_followship_bots();
}
