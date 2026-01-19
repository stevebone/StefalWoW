// =========================================
// TO-DO
// =========================================


#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"

#include "followship_bots.h"
#include "followship_bots_ai_base.h"
#include "followship_bots_config.h"
#include "followship_bots_priest.h"
#include "followship_bots_utils.h"
#include "followship_bots_utils_stats.h"
#include "followship_bots_utils_priest.h"
#include "Followship_bots_utils_combat.h"
#include "Followship_bots_utils_gossip.h"
//#include "Followship_bots_db.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_utils_spells.h"



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

        uint32 _recuperateReadyMs = 0;
        bool _pendingResurrection = false;
        bool _playerDead = false;
        uint32 _nextCombatSayMs = 0;

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

                FSBUtils::SetInitialState(me, hired, moveState);
                FSBUtils::SetBotClass(me, botClass);
                FSBUtilsStats::ApplyBotBaseClassStats(me, FSBUtils::GetBotClassForEntry(me->GetEntry()));

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
                    FSBUtilsStats::ApplyBotBaseRegen(me, _baseStatsMods, botClass);

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
        void IsSummonedBy(WorldObject* summoner) override
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

        bool OnGossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override // Runs once when gossip item selected
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

                    events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 100ms);

                    std::string msg = FSBUtilsTexts::BuildNPCSayText(player->GetName(), NULL, FSBSayType::PHire, "");
                    me->Say(msg, LANG_UNIVERSAL);
                    //me->Say(FSB_SAY_HIRED120, LANG_UNIVERSAL);
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
                events.ScheduleEvent(FSB_EVENT_MOVE_STAY, 100ms);
                break;
            }

                // Bot Follow Option
            case GOSSIP_ACTION_INFO_DEF + 21:
            {
                events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 100ms);
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

                // Bot Role Option Healer
            case GOSSIP_ACTION_INFO_DEF + 23:
            {
                FSBUtils::SetRole(me, FSB_Roles::FSB_ROLE_HEALER);

                //me->SetReactState(REACT_DEFENSIVE);
                me->SetReactState(REACT_AGGRESSIVE);
                if (me->HasAura(SPELL_PRIEST_SHADOWFORM))
                    me->RemoveAurasDueToSpell(SPELL_PRIEST_SHADOWFORM);
                me->Say(FSB_SAY_FOLLOW_INFO_CHANGED, LANG_UNIVERSAL);
                break;
            }

                // Bot Role Option Damage Deal
            case GOSSIP_ACTION_INFO_DEF + 24:
            {
                FSBUtils::SetRole(me, FSB_Roles::FSB_ROLE_RANGED_DAMAGE);
                me->SetReactState(REACT_AGGRESSIVE);
                me->CastSpell(me, SPELL_PRIEST_SHADOWFORM);
                me->Say(FSB_SAY_FOLLOW_INFO_CHANGED, LANG_UNIVERSAL);
                break;
            }

            // Bot Role Option Balanced
            case GOSSIP_ACTION_INFO_DEF + 25:
            {
                FSBUtils::SetRole(me, FSB_Roles::FSB_ROLE_ASSIST);
                //me->SetReactState(REACT_ASSIST);
                me->SetReactState(REACT_AGGRESSIVE);
                if (me->HasAura(SPELL_PRIEST_SHADOWFORM))
                    me->RemoveAurasDueToSpell(SPELL_PRIEST_SHADOWFORM);
                me->Say(FSB_SAY_FOLLOW_INFO_CHANGED, LANG_UNIVERSAL);
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

            if(FSBUtils::GetRole(me) == FSB_Roles::FSB_ROLE_ASSIST || FSBUtils::GetRole(me) == FSB_Roles:: FSB_ROLE_RANGED_DAMAGE)
                events.ScheduleEvent(FSB_EVENT_PRIEST_INITIAL_COMBAT_SPELLS_SELF, 100ms);

            // Target might be NULL if called from spell with invalid cast targets
            if (!who || !me->IsAlive())
                return;

            // Passive pets don't do anything
            if (me->HasReactState(REACT_PASSIVE))
                return;

            // Prevent pet from disengaging from current target
            if (me->GetVictim() && me->EnsureVictim()->IsAlive())
                return;

            // Continue to evaluate and attack if necessary
            FSBUtilsBotCombat::BotAttackStart(me, who, moveState);

            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: OnBotOwnerAttacked target: {}, move state: {}", who->GetName(), moveState);
        }

        void JustEnteredCombat(Unit* who) override
        {
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: JustEnteredCombat Triggered");
        }

        void JustExitedCombat() override
        {
            
        }        

        void AttackStart(Unit* /*target*/) override
        {

        }

        // Runs every time creature takes damage
        void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (!me->GetVictim() && attacker)
            {
                TC_LOG_DEBUG("scripts.ai.fsb", "FSB: DamageTaken - bot: {}, has no victim but is attacked by: {}", me->GetName(), attacker->GetName());
                FSBUtilsBotCombat::BotAttackStart(me, attacker, moveState);
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
            if (Unit* nextTarget = FSBUtilsBotCombat::BotSelectNextTarget(me, false))
                AttackStart(nextTarget);
            else
                FSBUtilsBotCombat::BotHandleReturnMovement(me, moveState, followDistance, followAngle); // Return

            
        }

        void OnSpellCast(SpellInfo const* spell) override // Runs every time the creature casts a spell
        {
            TC_LOG_DEBUG("scripts.core.fsb", "FSB Priest Casting spell id {}", spell->Id);

            if (spell->Id == SPELL_PRIEST_DEVOURING_PLAGUE || spell->Id == SPELL_PRIEST_PENANCE)
            {
                //_regenMods.manaPctBonus -= 2.0f;
                TC_LOG_DEBUG("scripts.ai.fsb", "FSB Priest Casting spell id {} we need to subtract 2% mana", spell->Id);
            }
        }

        void SpellHit(WorldObject* caster, SpellInfo const* spellInfo) override
        {

        }

        void SpellHitTarget(WorldObject* target, SpellInfo const* spellInfo) override
        {

        }

        void OnAuraApplied(AuraApplication const* aurApp) override
        {
            if (!aurApp)
                return;

            switch (aurApp->GetBase()->GetId())
            {
            case SPELL_PRIEST_DRINK_CONJURED_CRYSTAL_WATER:
            {
                int32 amount = FSBUtilsSpells::GetDrinkManaRegen(me->GetLevel());
                _statsMods.flatManaPerTick += amount;
                break;
            }

            case SPELL_PRIEST_POWER_WORD_FORTITUDE:
                _statsMods.flatMaxHealth += 10 * me->GetLevel();      // flat
                FSBUtilsStats::RecalculateMods(me, _statsMods);
                break;
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
            case SPELL_PRIEST_DRINK_CONJURED_CRYSTAL_WATER: // Conjured Crystal water
            {
                if (me->GetStandState() == UNIT_STAND_STATE_SIT)
                {
                    me->SetStandState(UNIT_STAND_STATE_STAND);
                    int32 amount = FSBUtilsSpells::GetDrinkManaRegen(me->GetLevel());
                    _statsMods.flatManaPerTick -= amount;
                }
                break;
            }

            case SPELL_PRIEST_POWER_WORD_FORTITUDE:
                _statsMods.flatMaxHealth -= 10 * me->GetLevel();      // flat
                FSBUtilsStats::RecalculateMods(me, _statsMods);

                if (me->GetHealth() > me->GetMaxHealth())
                    me->SetHealth(me->GetMaxHealth());

                break;
            }
        }

        void JustSummoned(Creature* summon) override // Runs every time the creature summons another creature
        {

        }

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override // Runs everytime the creature's summon dies - pet or minion
        {

        }

        void JustDied(Unit* /*killer*/) override // Runs once when creature dies
        {
            Player* player = FSBMgr::GetBotOwner(me)->ToPlayer();
            if (player)
            {
                std::string msg = FSBUtilsTexts::BuildNPCSayText(player->GetName(), NULL, FSBSayType::BotDeath, "");
                me->Yell(msg, LANG_UNIVERSAL);
            }
        }

        uint32 GetData(uint32 type) const override // Runs once to check what data exists on the creature
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
            case FSB_ACTION_RESURRECT_PLAYER:
            {
                if (!_pendingResurrection)
                    return;

                uint32 now = getMSTime();

                //Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);
                Unit* owner = me->GetOwner();
                Player* player = owner ? owner->ToPlayer() : nullptr;

                if (!player)
                    return;

                // Must be fully safe
                if (me->IsInCombat() || player->IsInCombat())
                    return;

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (player->IsAlive())
                {
                    _pendingResurrection = false;
                    return;
                }

                // Must be close enough (important!)
                if (!me->IsWithinDistInMap(player, 30.0f))
                    return;

                TC_LOG_DEBUG("scripts.ai.fsb", "FSB: ACTION Resurrect Player");

                FSBUtilsMovement::StopFollow(me);

                me->CastSpell(player, SPELL_PRIEST_RESURRECTION, false);
                _globalCooldownUntil = now + NPC_GCD_MS;

                ResumeFollow(player);

                break;
            }
            case FSB_ACTION_INITIAL_COMBAT_SPELLS_SELF:
            {
                uint32 now = getMSTime();

                if (_globalCooldownUntil < now && !me->HasUnitState(UNIT_STATE_CASTING))
                {

                    if (!me->HasAura(SPELL_PRIEST_POWER_WORD_SHIELD))
                    {
                        me->CastSpell(me, SPELL_PRIEST_POWER_WORD_SHIELD);
                        _globalCooldownUntil = now + NPC_GCD_MS;
                        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Initial Combat SELF Spell Cast: PWS");
                        events.ScheduleEvent(FSB_EVENT_PRIEST_INITIAL_COMBAT_SPELLS_SELF, 1500ms);
                    }
                    else if (!me->HasAura(SPELL_PRIEST_RENEW))
                    {
                        me->CastSpell(me, SPELL_PRIEST_RENEW);
                        _globalCooldownUntil = now + NPC_GCD_MS;
                        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Initial Combat SELF Spell Cast: Renew");
                    }
                }
                else {
                    events.ScheduleEvent(FSB_EVENT_PRIEST_INITIAL_COMBAT_SPELLS_SELF, 1500ms);
                    TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Initial Combat SELF Spell Cast: GCD");
                }
                break;
            }

            case FSB_ACTION_INITIAL_COMBAT_SPELLS_PLAYER:
            {
                uint32 now = getMSTime();

                Player* player = FSBMgr::GetBotOwner(me)->ToPlayer();
                if (!player)
                {
                    events.ScheduleEvent(FSB_EVENT_PRIEST_INITIAL_COMBAT_SPELLS_PLAYER, 1500ms);
                    TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Initial Combat PLAYER Spell Cast: No player found");
                    break;
                }


                if (FSBUtilsSpells::CanCastNow(me, now, _globalCooldownUntil))
                {
                    if (!player->HasAura(SPELL_PRIEST_POWER_WORD_SHIELD))
                    {
                        me->CastSpell(player, SPELL_PRIEST_POWER_WORD_SHIELD);
                        _globalCooldownUntil = now + NPC_GCD_MS;
                        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Initial Combat PLAYER Spell Cast: PWS");
                        events.ScheduleEvent(FSB_EVENT_PRIEST_INITIAL_COMBAT_SPELLS_PLAYER, 1500ms);
                    }
                    else if (!player->HasAura(SPELL_PRIEST_RENEW))
                    {
                        me->CastSpell(player, SPELL_PRIEST_RENEW);
                        _globalCooldownUntil = now + NPC_GCD_MS;
                        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Initial Combat PLAYER Spell Cast: Renew");
                    }
                }
                else {
                    events.ScheduleEvent(FSB_EVENT_PRIEST_INITIAL_COMBAT_SPELLS_PLAYER, 1500ms);
                    TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Initial Combat PLAYER Spell Cast: GCD");
                }
                break;
            }

            case FSB_ACTION_OOC_MANA:
            {
                if (me->GetPowerPct(POWER_MANA) > 30)
                    break;

                uint32 now = getMSTime();

                if (FSBUtilsSpells::CanCastNow(me, now, _globalCooldownUntil))
                {
                    if (!me->HasAura(22734))
                    {
                        Unit* owner = me->GetOwner();
                        Player* player = owner ? owner->ToPlayer() : nullptr;

                        FSBUtilsMovement::StopFollow(me);
                        me->Say("I'm gonna take a mana break!", LANG_UNIVERSAL); // TO-DO maybe turn this into a choice of lines
                        me->CastSpell(me, SPELL_PRIEST_DRINK_CONJURED_CRYSTAL_WATER); // Conjured Crystal Water
                        _globalCooldownUntil = now + 30000; // set cooldown to 30s to not interrup the drink spell which lasts 30 seconds max

                        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: ACTION OOC Mana");

                        events.ScheduleEvent(FSB_EVENT_RESUME_FOLLOW, 30s);
                    }
                }
                else
                    events.ScheduleEvent(FSB_EVENT_PRIEST_OOC_MANA, 1500ms);

                break;
            }

            case FSB_ACTION_OOC_RECUPERATE:
            {
                if (me->GetHealthPct() > 50)
                    break;

                uint32 now = getMSTime();

                if (FSBUtilsSpells::CanCastNow(me, now, _globalCooldownUntil))
                {
                        if (!me->HasAura(SPELL_PRIEST_RECUPERATE) && now >= _recuperateReadyMs)
                        {
                            FSBUtilsMovement::StopFollow(me);

                            me->CastSpell(me, SPELL_PRIEST_RECUPERATE, false);
                            _recuperateReadyMs = now + 60000; // 1 minute
                            _globalCooldownUntil = now + 10000; // use 10s cooldown to not interrup duration of channel spell

                            me->Say("I need a breather!", LANG_UNIVERSAL); // TO-DO maybe turn this into a choice of lines

                            TC_LOG_DEBUG("scripts.ai.core", "FSB Out-of-combat: Recuperate");
                            events.ScheduleEvent(FSB_EVENT_RESUME_FOLLOW, 10s);
                            break;
                        }
                   
                }
                else events.ScheduleEvent(FSB_EVENT_PRIEST_OOC_RECUPERATE, 1500ms);

                break;
            }

            case FSB_ACTION_OOC_HEAL_SELF:
            {
                if (me->GetHealthPct() > 80)
                    break;

                uint32 now = getMSTime();

                if (FSBUtilsSpells::CanCastNow(me, now, _globalCooldownUntil))
                {
                    FSBUtilsMovement::StopFollow(me);
                    me->CastSpell(me, SPELL_PRIEST_HEAL, false);

                    _globalCooldownUntil = now + NPC_GCD_MS;

                    std::string msg = FSBUtilsTexts::BuildNPCSayText("", NULL, FSBSayType::HealSelf, "");
                    me->Say(msg, LANG_UNIVERSAL);

                    events.ScheduleEvent(FSB_EVENT_RESUME_FOLLOW, 2s);
                }
                else events.ScheduleEvent(FSB_EVENT_PRIEST_OOC_HEAL_SELF, 1500ms);

                break;
            }

            case FSB_ACTION_OOC_HEAL_PLAYER:
            {
                uint32 now = getMSTime();

                Unit* owner = me->GetOwner();
                Player* player = owner ? owner->ToPlayer() : nullptr;

                if (!player || !player->IsAlive())
                    break;

                if (player->GetHealthPct() > 70)
                    break;

                if (FSBUtilsSpells::CanCastNow(me, now, _globalCooldownUntil))
                {
                    

                    if (player->GetHealthPct() <= 50)
                    {
                        TC_LOG_DEBUG("scripts.ai.core", "FSB Out-of-combat: Player Heal < 50");

                        FSBUtilsMovement::StopFollow(me);
                        me->CastSpell(player, SPELL_PRIEST_HEAL, false);
                        _globalCooldownUntil = now + NPC_GCD_MS;

                        if (urand(0, 99) <= REACT_BUFFED_CHANCE_PERCENT)
                            me->Say(FSBUtilsTexts::BuildNPCSayText(player->GetName(), NULL, FSBSayType::HealTarget, ""), LANG_UNIVERSAL);

                        events.ScheduleEvent(FSB_EVENT_RESUME_FOLLOW, 3s);
                    }
                    else if (player->GetHealthPct() <= 70)
                    {
                        TC_LOG_DEBUG("scripts.ai.core", "FSB Out-of-combat: Player Heal < 70");

                        FSBUtilsMovement::StopFollow(me);
                        me->CastSpell(player, SPELL_PRIEST_FLASH_HEAL, false);
                        _globalCooldownUntil = now + NPC_GCD_MS;

                        if (urand(0, 99) <= REACT_BUFFED_CHANCE_PERCENT)
                            me->Say(FSBUtilsTexts::BuildNPCSayText(player->GetName(), NULL, FSBSayType::HealTarget, ""), LANG_UNIVERSAL);

                        events.ScheduleEvent(FSB_EVENT_RESUME_FOLLOW, 3s);
                    }
                    else break;

                }
                else events.ScheduleEvent(FSB_EVENT_PRIEST_OOC_HEAL_PLAYER, 1500ms);

                break;
            }

            case FSB_ACTION_IC_MANA:
            {
                if (me->GetPowerPct(POWER_MANA) > 20)
                    break;

                //Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);
                Unit* owner = me->GetOwner();
                Player* player = owner ? owner->ToPlayer() : nullptr;

                uint32 now = getMSTime();

                if (ManaPotionSpellId)
                {
                    me->CastSpell(me, ManaPotionSpellId, false);
                    _botManaPotionUsed = true;

                    std::string spellName = FSBUtilsSpells::GetSpellName(ManaPotionSpellId);
                    std::string msg = FSBUtilsTexts::BuildNPCSayText(player->GetName(), NULL, FSBSayType::CombatMana, spellName);
                    me->Say(msg, LANG_UNIVERSAL);

                    TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Combat mana potion used with spell id: {}", ManaPotionSpellId);
                }

                break;
            }

            default:
                break;
            }
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                // =================================================== //
                // ========= PERIODIC MAINTENANCE ==================== //
                // Events that are triggered periodically              //
                // =================================================== //

                case FSB_EVENT_PERIODIC_MAINTENANCE:
                {
                    uint32 now = getMSTime();

                    // Check if custom regen is enabled and we can schedule every 2s
                    if (FollowshipBotsConfig::configFSBUseCustomRegen && now >= _nextRegenMs)
                        events.ScheduleEvent(FSB_EVENT_PERIODIC_CUSTOM_REGEN, 2s);



                    

                    bool castedThisTick = false;

                    Unit* owner = me->GetOwner();
                    Player* player = owner ? owner->ToPlayer() : nullptr;

                    // BOT check if player dead
                    if (player && !player->IsAlive() && !_pendingResurrection)
                    {
                        if (me->IsAlive() && !_playerDead)
                        {
                            std::string msg = FSBUtilsTexts::BuildNPCSayText(player->GetName(), NULL, FSBSayType::PlayerDead, "");
                            me->Yell(msg, LANG_UNIVERSAL);
                        }

                        _playerDead = true; // This will stop the bot spamming the SAY
                        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Player found dead in Periodic Check");

                        if (!me->IsInCombat())
                        {
                            _pendingResurrection = true;
                            DoAction(FSB_ACTION_RESURRECT_PLAYER);
                        }
                    }
                    else if (player && player->IsAlive() && _pendingResurrection && _playerDead)
                    {
                        _pendingResurrection = false;
                        _playerDead = false;

                        std::string msg = FSBUtilsTexts::BuildNPCSayText(player->GetName(), NULL, FSBSayType::Resurrect, "");
                        me->Say(msg, LANG_UNIVERSAL);
                    }

                    

                    // BOT Check OOC Actions
                    if (!me->IsInCombat() && me->IsAlive())
                    {
                        _botOutCombat = true;
                        _botInCombat = false;
                        _botManaPotionUsed = false;

                        if (!me->HasUnitState(UNIT_STATE_CASTING) && _playerCombatEnded)
                        {
                            events.ScheduleEvent(FSB_EVENT_PRIEST_OOC_MANA, 100ms);
                            events.ScheduleEvent(FSB_EVENT_PRIEST_OOC_RECUPERATE, 100ms);
                            events.ScheduleEvent(FSB_EVENT_PRIEST_OOC_HEAL_SELF, 100ms);
                        }
                    }

                    // BOT Check In Combat Actions
                    if (me->IsInCombat() && me->IsAlive())
                    {
                        _botInCombat = true;
                        _botOutCombat = false;

                        if (!me->HasUnitState(UNIT_STATE_CASTING))
                            events.ScheduleEvent(FSB_EVENT_PRIEST_IC_MANA, 100ms);


                    }

                    // Fortitude Buff check
                    if (!me->IsInCombat() && me->IsAlive())
                    {
                        Unit* fortTarget = nullptr;

                        if (player && player->IsAlive() && !player->HasAura(SPELL_PRIEST_POWER_WORD_FORTITUDE))
                            fortTarget = player;
                        else if (!me->HasAura(SPELL_PRIEST_POWER_WORD_FORTITUDE))
                            fortTarget = me;

                        if (fortTarget && !castedThisTick && FSBUtilsSpells::CanCastNow(me, now, _globalCooldownUntil))
                        {
                            me->CastSpell(fortTarget, SPELL_PRIEST_POWER_WORD_FORTITUDE);
                            castedThisTick = true;
                            _globalCooldownUntil = now + NPC_GCD_MS;

                            //TC_LOG_DEBUG("scripts.ai.core", "FSB: UpdateAI Event Maintenance Fortitude buff check"); // TEMP LOG

                            if (fortTarget == me)
                            {
                                me->Say("Am gonna need this buff!", LANG_UNIVERSAL); // TO-DO maybe turn this into a choice of lines
                            }
                            else
                                me->Say("Here's something for ya!", LANG_UNIVERSAL); // TO-DO maybe turn this into a choice of lines
                        }
                    }

                    // BOT PLAYER OOC Heal
                    if (_botOutCombat && _playerCombatEnded)
                    {
                        //events.ScheduleEvent(FSB_EVENT_PRIEST_OOC_HEAL_PLAYER, 100ms);
                    }

                    // PLAYER check on combat for buffs and combat spells
                    // Different Triggers per role
                    if (player && player->IsAlive() && player->IsInCombat() && !_playerCombatStarted)
                    {
                        _playerCombatStarted = true;
                        _playerCombatEnded = false;

                        if (FSBUtils::GetRole(me) == FSB_Roles::FSB_ROLE_HEALER)
                            events.ScheduleEvent(FSB_EVENT_PRIEST_INITIAL_COMBAT_SPELLS_PLAYER, 100ms);

                        
                    }

                    if (player && player->IsAlive() && !player->IsInCombat() && _playerCombatStarted)
                    {
                        _playerCombatStarted = false;
                        _playerCombatEnded = true;

                        //if (me->HasUnitFlag(UNIT_FLAG_IN_COMBAT))
                        //    me->RemoveUnitFlag(UNIT_FLAG_IN_COMBAT);
                    }

                    

                    // Teleport if too far away
                    if (player && me->GetMapId() == player->GetMapId() && me->GetDistance(player) > 100.0f)
                    {
                        me->NearTeleportTo(
                            player->GetPositionX() + urand(3.f, 10.f),
                            player->GetPositionY(),
                            player->GetPositionZ(),
                            player->GetOrientation());

                        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Teleported bot {} to player {} due to distance > 100.", me->GetName(), player->GetName());

                    }

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

                    events.ScheduleEvent(FSB_EVENT_PERIODIC_MAINTENANCE, 1s);

                    //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Event Periodic Maintenance Reached the end"); // TEMP LOG

                    break;
                }

                // Applies custom regeneration & mods
                case FSB_EVENT_PERIODIC_CUSTOM_REGEN:
                {
                    uint32 now = getMSTime();

                    if (me->GetHealthPct() < 100 || me->GetPowerPct(me->GetPowerType()) < 100)
                    {
                        FSBUtilsStats::ProcessBotCustomRegenTick(me, botClass, _baseStatsMods, _statsMods);

                        // ? lock regen for next 2 seconds
                        _nextRegenMs = now + 2000;
                    }

                    break;
                }


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
                        

                        if (now >= _5secondsCheckMs)
                        {
                            events.ScheduleEvent(FSB_EVENT_HIRED_CHECK_ALLIES, 5s);
                            events.ScheduleEvent(FSB_EVENT_HIRED_UPDATE_BOT_LEVEL, 5s);
                        }
                    }

                    events.ScheduleEvent(FSB_EVENT_HIRED_MAINTENANCE, 1ms);

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

                            FSBUtilsOwnerCombat::OnBotOwnerAttacked(FSBMgr::GetBotOwner(me)->GetVictim(), me, moveState);
                        }

                        // OWNER WAS ATTACKED
                        if (Unit* attacker = FSBUtilsOwnerCombat::CheckBotOwnerAttackedBy(FSBMgr::GetBotOwner(me)))
                        {
                            //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Owner was attacked by {}", attacker->GetName());

                            FSBUtilsOwnerCombat::OnBotOwnerAttackedBy(attacker, me, moveState);
                        }
                    }

                    break;
                }

                    // Check to dermine what friendlies we have in our "group"
                    // Includes: bot, owner and other bots owner by its owner
                    // TO-DO: Add check to include other players in the group of the owner
                case FSB_EVENT_HIRED_CHECK_ALLIES:
                {
                    uint32 now = getMSTime();

                    if (now >= _5secondsCheckMs)
                        FSBUtilsCombat::CheckBotAllies(me->ToCreature(), botGroup_, 50.0f);

                    // ? lock check for next 5 seconds
                    _5secondsCheckMs = now + 5000;

                    break;
                }

                case FSB_EVENT_HIRED_UPDATE_BOT_LEVEL:
                {
                    uint32 now = getMSTime();

                    if (now >= _5secondsCheckMs)
                        FSBUtilsStats::UpdateBotLevelToPlayer(me, _statsMods);

                    // ? lock check for next 5 seconds
                    _5secondsCheckMs = now + 5000;

                    break;
                }

                // =================================================== //
                // ========= COMBAT MAINTENANCE ====================== //
                // Events that are checked during combat               //
                // =================================================== //
                    
                case FSB_EVENT_COMBAT_MAINTENANCE:
                {
                    if (me->IsInCombat() && me->IsAlive())
                    {
                        if(!me->HasUnitState(UNIT_STATE_CASTING))
                            events.ScheduleEvent(FSB_EVENT_COMBAT_SPELL_CHECK, 1s);
                    }

                    events.ScheduleEvent(FSB_EVENT_COMBAT_MAINTENANCE, 1s);

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

                // =================================================== //
                // ========= TIMED ACTION EVENTS ===================== //
                // Events that are checked during combat               //
                // =================================================== //   

                    // BOT Follows player again
                    // For timed events
                case FSB_EVENT_RESUME_FOLLOW:
                {
                    Player* player = FSBMgr::GetBotOwner(me)->ToPlayer();

                    if(player)
                        ResumeFollow(player);

                    break;
                }






                    // Priest casts Renew and PWS on Engagement
                    // Only in Assist and Damage roles
                    // Triggered from: JustEngagedWith()
                case FSB_EVENT_PRIEST_INITIAL_COMBAT_SPELLS_SELF:
                {
                    if (_botInCombat)
                    {
                        DoAction(FSB_ACTION_INITIAL_COMBAT_SPELLS_SELF);
                    }

                    break;
                }

                    // Priest casts Renew and PWS on Player
                    // Only in Healer role
                    // Triggered from Periodic Maintenance check and flag
                case FSB_EVENT_PRIEST_INITIAL_COMBAT_SPELLS_PLAYER:
                {
                    DoAction(FSB_ACTION_INITIAL_COMBAT_SPELLS_PLAYER);
                    break;
                }

                case FSB_EVENT_HIRE_EXPIRED:
                case FSB_EVENT_HIRE_DISMISSED:
                {
                    Unit* owner = me->GetOwner();
                    Player* player = owner ? owner->ToPlayer() : nullptr;

                    me->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP);
                    events.Reset();
                    events.ScheduleEvent(FSB_EVENT_MOVE_STAY, 100ms);
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
                    moveState = FSB_MOVE_STATE_STAY;

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
                    moveState = FSB_MOVE_STATE_FOLLOWING;

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

                case FSB_EVENT_PRIEST_IC_MANA:
                {
                    if (_botInCombat && me->GetPowerPct(POWER_MANA) <= 20 && !_botManaPotionUsed)
                        DoAction(FSB_ACTION_IC_MANA);

                    break;
                }

                case FSB_EVENT_PRIEST_OOC_MANA:
                {
                    if (_botOutCombat && me->GetPowerPct(POWER_MANA) < 30)
                        DoAction(FSB_ACTION_OOC_MANA);
                    
                    break;
                }
                case FSB_EVENT_PRIEST_OOC_RECUPERATE:
                {
                    if (_botOutCombat && me->GetHealthPct() <= 50)
                        DoAction(FSB_ACTION_OOC_RECUPERATE);

                    break;
                }

                case FSB_EVENT_PRIEST_OOC_HEAL_SELF:
                {
                    if (_botOutCombat && me->GetHealthPct() <= 80)
                        DoAction(FSB_ACTION_OOC_HEAL_SELF);

                    break;
                }

                case FSB_EVENT_PRIEST_OOC_HEAL_PLAYER:
                {
                //    DoAction(FSB_ACTION_OOC_HEAL_PLAYER);
                //    break;
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
        uint32 ManaPotionSpellId = FSBUtilsSpells::GetManaPotionSpellForLevel(me->GetLevel());
        std::unordered_map<uint32 /*spellId*/, uint32 /*nextReadyMs*/> _spellCooldowns;

        
        private:
            EventMap events;
            ObjectGuid _playerGuid;

            FSBUtilsStatsMods _statsMods;
            FSBUtilsStatsMods _baseStatsMods;

            

            bool combatEmergency = false;
            bool groupHealthy = false;

            uint32 hireTimeLeft = 0;

            bool botMounted = false;

            // Bot Stats
            uint32 _nextRegenMs = 0;

            // ----------
            // Combat
            ObjectGuid _lastOwnerVictim;
            bool _ownerWasInCombat = false;
            std::vector<FSBSpellRuntime> _runtimeSpells; // runtime for spells cooldowns
            uint32 _globalCooldownUntil; // global cooldown
            //void InitSpellRuntime();

            // ----------
            // Movement States
            uint16 moveState;

            // ----------
            // Allies & Group
            std::vector<Unit*> botGroup_;
            uint32 _5secondsCheckMs = 0;            
            
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
