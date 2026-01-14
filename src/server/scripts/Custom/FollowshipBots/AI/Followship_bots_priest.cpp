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



class npc_followship_bots_priest : public CreatureScript
{
public:
    npc_followship_bots_priest() : CreatureScript("npc_followship_bots_priest") { }

    struct npc_followship_bots_priestAI : public FSB_BaseAI
    {
        npc_followship_bots_priestAI(Creature* creature) : FSB_BaseAI(creature)
        {
            // Runs once when creature is spawned
        }

        

        // Bot Operations
        bool hired = false;

        bool updateFollowInfo = false;
        float followDistance = FOLLOW_DISTANCE_NORMAL;
        float followAngle = FOLLOW_ANGLE_BEHIND;

        uint32 _globalCooldownUntil = 0;
        uint32 _desperatePrayerReadyMs = 0;
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

        // Bot Stats
        uint32 _nextRegenMs = 0;


        void InitializeAI() override // Runs once after creature is spawned and AI not loaded
        {
            ScriptedAI::InitializeAI(); // always call base first            
        }

        void Reset() override // Runs at creature respawn, evade or when triggered
        {
            if (FollowshipBotsConfig::configFSBEnabled)
            {
                events.Reset();

                me->SetBot(true);

                // Initial Flags and States
                me->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
                me->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);

                me->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
                me->SetReactState(REACT_DEFENSIVE);
                me->SetFaction(1);

                _regenMods = FSBUtilsStatsMods(); // zero everything
                FSBUtilsStats::RecalculateStats(me, _regenMods);

                TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Entered Reset. Is bot: {}", me->IsBot());
                // Schedule Generic Events
                // These can take place even when BOT is NOT hired or no player is around
                events.ScheduleEvent(FSB_EVENT_PERIODIC_MAINTENANCE, 1s);

                if (FollowshipBotsConfig::configFSBUseCustomRegen)
                {
                    me->RemoveUnitFlag2(UNIT_FLAG2_REGENERATE_POWER);
                    me->SetRegenerateHealth(false); // no health regen

                    events.ScheduleEvent(FSB_EVENT_USE_CUSTOM_REGEN, 2s);
                    TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Reset. Custom Regen active for bots");
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

        }

        void JustAppeared() override // Runs once when creature appeared in world, works for DB spawns
        {

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
                FSBUtils::SetRole(me, FSB_Roles::FSB_ROLE_DAMAGE);
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

            if(FSBUtils::GetRole(me) == FSB_Roles::FSB_ROLE_ASSIST || FSBUtils::GetRole(me) == FSB_Roles:: FSB_ROLE_DAMAGE)
                events.ScheduleEvent(FSB_EVENT_PRIEST_INITIAL_COMBAT_SPELLS_SELF, 100ms);
        }

        // Runs every time creature takes damage
        void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
        }

        void EnterEvadeMode(EvadeReason /*why*/) override // Runs every time creature evades
        {

        }

        void KilledUnit(Unit* victim) override // Runs every time the creature kills an unit
        {
            Unit* owner = me->GetOwner();
            Player* player = owner ? owner->ToPlayer() : nullptr;

            std::string pName = player ? player->GetName() : "";

            std::string msg = FSBUtilsTexts::BuildNPCSayText(pName, NULL, FSBSayType::TargetDeath, victim->GetName());
            me->Say(msg, LANG_UNIVERSAL);
        }

        void OnSpellCast(SpellInfo const* spell) override // Runs every time the creature casts a spell
        {
            TC_LOG_DEBUG("scripts.core.fsb", "FSB Priest Casting spell id {}", spell->Id);

            if (spell->Id == SPELL_PRIEST_DEVOURING_PLAGUE || spell->Id == SPELL_PRIEST_PENANCE)
            {
                _regenMods.manaPctBonus -= 2.0f;
                TC_LOG_DEBUG("scripts.core.fsb", "FSB Priest Casting spell id {} we need to subtract 2% mana", spell->Id);
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
                _regenMods.flatManaPerTick += 90 * 2;
                break;

            case SPELL_PRIEST_POWER_WORD_FORTITUDE:
                _regenMods.flatMaxHealth += 10 * me->GetLevel();      // flat
                FSBUtilsStats::RecalculateStats(me, _regenMods);
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
                if (me->GetStandState() == UNIT_STAND_STATE_SIT)
                {
                    me->SetStandState(UNIT_STAND_STATE_STAND);
                    _regenMods.flatManaPerTick -= 90 * 2;
                }
                break;

            case SPELL_PRIEST_POWER_WORD_FORTITUDE:
                _regenMods.flatMaxHealth -= 10 * me->GetLevel();      // flat
                FSBUtilsStats::RecalculateStats(me, _regenMods);

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
            Unit* owner = me->GetOwner();
            Player* player = owner ? owner->ToPlayer() : nullptr;

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
                    Reset();
                    events.ScheduleEvent(FSB_EVENT_HIRE_EXPIRED, std::chrono::seconds(hireTimeLeft));
                    events.ScheduleEvent(FSB_EVENT_RESUME_FOLLOW, 1s);

                    TC_LOG_DEBUG("scripts.ai.fsb", "FSB SetData: Bot {} received hired left time value: {}", me->GetName(), hireTimeLeft);
                }
                else
                {
                    Reset();
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
            case FSB_ACTION_COMBAT_HEAL:
            {
                groupHealthy = false;
                bool isSelfTarget = false;

                Unit* healTarget = FSBUtilsCombat::SelectHealTarget(me, botGroup_);
                //Unit* healTarget = nullptr;

                if (healTarget)
                {
                    TC_LOG_DEBUG("scripts.ai.fsb", "FSB DoAction::Combat Heal target is: {}", healTarget->GetName());
                }

                if (!healTarget)
                {
                    TC_LOG_DEBUG("scripts.ai.fsb", "FSB DoAction::Combat Heal targets empty");
                    groupHealthy = true;
                    return; // nothing to do
                }

                FSBSpellCandidate* spell = FSBUtilsSpells::SelectSpell(
                    me,
                    healTarget,
                    PriestSpellsTable,      // or bot-specific table
                    FSBSpellType::Heal,
                    false
                );

                if (!spell)
                {
                    TC_LOG_DEBUG("scripts.ai.fsb", "{} no valid heal spell selected", me->GetName());
                    return;
                }

                // If not self-cast, ensure we are in range first
                if (!spell->isSelfCast)
                {
                    if (FSBUtilsMovement::EnsureInRange(me, healTarget, spell->dist))
                        return; // movement started, casting deferred
                }

                // LOS check
                if (!spell->isSelfCast)
                {
                    if (FSBUtilsMovement::EnsureLOS(me, healTarget))
                        return;
                }

                // Purify check for Debuff
                if (spell->spellId == SPELL_PRIEST_PURIFY)
                {
                    if (!FSBUtilsSpells::HasDispellableDebuff(healTarget))
                    {
                        TC_LOG_DEBUG("scripts.ai.fsb", "{} Selected Purify spell cast on target: {} but no Debuff found", me->GetName(), healTarget->GetName());
                        break;
                    }
                }

                if (FSBUtilsSpells::TryCast(me, healTarget, spell, _globalCooldownUntil))
                {
                    TC_LOG_DEBUG("scripts.ai.fsb", "{} Tried cast {} on target: {}", me->GetName(), FSBUtilsSpells::GetSpellName(spell->spellId), healTarget->GetName());

                    if (healTarget == me)
                        FSBUtilsCombat::SayCombatMessage(me, healTarget, 0, FSBSayType::HealSelf, spell->spellId);
                    else FSBUtilsCombat::SayCombatMessage(me, healTarget, 0, FSBSayType::HealTarget, spell->spellId);

                }

                break;
            }
            case FSB_ACTION_COMBAT_EMERGENCY:
            {
                bool isSelfTarget = false;

                std::vector<Unit*> emergencyTargets = FSBUtilsCombat::GetEmergencyCandidates(botGroup_);
                if (emergencyTargets.empty())
                {
                    TC_LOG_DEBUG("scripts.ai.fsb", "FSB DoAction::Combat Emergency targets empty");
                    return; // nothing to do
                }

                if (FSBUtilsMovement::GetMovementType(me) == CHASE_MOTION_TYPE)
                    return;

                // Sort by priority
                FSBUtilsCombat::SortEmergencyTargets(emergencyTargets);

                // Pick the highest priority target
                Unit* emergencyTarget = emergencyTargets.front();
                if (!emergencyTarget)
                    return;

                // Check if the emergency target is the bot itself
                isSelfTarget = (emergencyTarget == me);

                // Now you can pass this info to your spell-casting logic
                TC_LOG_DEBUG("scripts.ai.fsb", "FSB Combat Emergency target: {} | isSelfTarget: {}",
                    emergencyTarget->GetName(), isSelfTarget);

                // Example: trying to select a heal spell for a target
                FSBSpellCandidate* spell = FSBUtilsSpells::SelectSpell(
                    me,                      // caster
                    emergencyTarget,                  // target
                    PriestSpellsTable,         // vector<FSBSpellCandidate> for heals
                    FSBSpellType::Heal,      // type of spell
                    false                     // preferSelfCast first
                );

                if (!spell)
                {
                    TC_LOG_DEBUG("scripts.ai.fsb", "{} no valid spell selected", me->GetName());
                    return;
                }

                // If not self-cast, ensure we are in range first
                if (!spell->isSelfCast)
                {
                    if (FSBUtilsMovement::EnsureInRange(me, emergencyTarget, spell->dist))
                        return; // movement started, casting deferred
                }

                // LOS check
                if (!spell->isSelfCast)
                {
                    if (FSBUtilsMovement::EnsureLOS(me, emergencyTarget))
                        return;
                }

                // Purify check for Debuff
                if (spell->spellId == SPELL_PRIEST_PURIFY)
                {
                    if (!FSBUtilsSpells::HasDispellableDebuff(emergencyTarget))
                    {
                        TC_LOG_DEBUG("scripts.ai.fsb", "{} Selected Purify spell cast on target: {} but no Debuff found", me->GetName(), emergencyTarget->GetName());
                        break;
                    }
                }

                if (FSBUtilsSpells::TryCast(me, emergencyTarget, spell, _globalCooldownUntil))
                {
                    TC_LOG_DEBUG("scripts.ai.fsb", "{} Tried cast {} on target: {}", me->GetName(), FSBUtilsSpells::GetSpellName(spell->spellId), emergencyTarget->GetName());

                    if (spell->spellId == SPELL_PRIEST_DESPERATE_PRAYER)
                        FSBUtilsCombat::SayCombatMessage(me, emergencyTarget, 0, FSBSayType::HealSelf, spell->spellId);

                }

                break;
            }

            case FSB_ACTION_COMBAT_DAMAGE:
            {
                TC_LOG_DEBUG("scripts.ai.fsb", "FSB DoAction::Damage Spells triggered");

                Unit* playerTarget = nullptr;
                Player* player = me->GetOwner() ? me->GetOwner()->ToPlayer() : nullptr;

                if (player)
                {
                    playerTarget = player->GetSelectedUnit(); // The player's target
                }


                Unit* dmgTarget = FSBUtilsCombat::SelectDamageTarget(me, botGroup_, playerTarget);

                if (!dmgTarget)
                {
                    TC_LOG_DEBUG("scripts.ai.fsb", "FSB DoAction::Combat Spell targets empty");
                    break; // nothing to do
                }

                FSBSpellCandidate* spell = FSBUtilsSpells::SelectSpell(
                    me,
                    dmgTarget,
                    PriestSpellsTable,      // or bot-specific table
                    FSBSpellType::Damage,
                    false
                );

                if (!spell)
                {
                    TC_LOG_DEBUG("scripts.ai.fsb", "{} no valid damage spell selected", me->GetName());
                    break;
                }
                else if (!spell && dmgTarget)
                {
                    me->Attack(dmgTarget, true); // melee attack
                }

                // If not self-cast, ensure we are in range first
                if (!spell->isSelfCast)
                {
                    if (FSBUtilsMovement::EnsureInRange(me, dmgTarget, spell->dist))
                        return; // movement started, casting deferred
                }

                // LOS check
                if (!spell->isSelfCast)
                {
                    if (FSBUtilsMovement::EnsureLOS(me, dmgTarget))
                        return;
                }

                // Sync combat state with target
                if (!me->GetThreatManager().IsThreatenedBy(dmgTarget))
                {
                    me->GetThreatManager().AddThreat(dmgTarget, 10.0f);
                    me->Attack(dmgTarget, true);
                }

                // =============================
                // Class Related Handling
                // =============================
                // 
                // ---------- Psychic Scream special case ----------
                if (spell->spellId == SPELL_PRIEST_PSYCHIC_SCREAM)
                {
                    if (FSBUtilsCombat::CountAttackersOn(me) + (player ? FSBUtilsCombat::CountAttackersOn(player) : 0) < 3)
                        return;
                }

                // =============================

                if (FSBUtilsSpells::TryCast(me, dmgTarget, spell, _globalCooldownUntil))
                {
                    TC_LOG_DEBUG("scripts.ai.fsb", "{} Tried cast {} on target: {}", me->GetName(), FSBUtilsSpells::GetSpellName(spell->spellId), dmgTarget->GetName());

                    FSBUtilsCombat::SayCombatMessage(me, dmgTarget, 0, FSBSayType::SpellOnTarget, spell->spellId);

                }

                break;
            }

            case FSB_ACTION_INITIATE_COMBAT:
            {
                TC_LOG_DEBUG("scripts.ai.fsb", "FSB DoAction::Combat Spells triggered");
                combatEmergency = false;

                // Check Hard Requirements
                if (!me->IsAlive())
                    return;


                // =============================
                // HEALING EMERGENCY: Player or Bots based on Role
                // =============================
                // Check for emergencies - targets < 30% hP
                std::vector<Unit*> emergencyTargets = FSBUtilsCombat::GetEmergencyCandidates(botGroup_);
                if (!emergencyTargets.empty())
                {
                    TC_LOG_DEBUG("scripts.ai.fsb", "FSB DoAction::Combat Spells emergency targets NOT empty");

                    combatEmergency = true;
                    DoAction(FSB_ACTION_COMBAT_EMERGENCY);
                    return;
                }

                // This needs to be here so that we do not interrupt emergency chase motion
                if (FSBUtilsMovement::GetMovementType(me) == CHASE_MOTION_TYPE)
                    return;

                // =============================
                // HEALING: Player or Bot based on Role
                // =============================
                if (!me->HasUnitState(UNIT_STATE_CASTING) && !groupHealthy)
                {
                    DoAction(FSB_ACTION_COMBAT_HEAL);
                    break;
                }

                // =============================
                // DAMAGE: Bot based on Role
                // =============================
                else
                {
                    DoAction(FSB_ACTION_COMBAT_DAMAGE);
                    break;
                }

                break;
            }

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

                if (_globalCooldownUntil < now)
                {
                    Unit* owner = me->GetOwner();
                    Player* player = owner ? owner->ToPlayer() : nullptr;

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
                    // Applies custom regeneration
                case FSB_EVENT_USE_CUSTOM_REGEN:
                {
                    if (!me->IsBot() || !me->IsAlive())
                        break;

                    //FSBUtilsStats::ApplyRegen(me); // picks default/class + combat state automatically
                    FSBUtilsStats::ApplyRegen(me, _regenMods);

                    //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: UpdateAI Event Custom Regen tick");  // TEMP-LOG

                    events.ScheduleEvent(FSB_EVENT_USE_CUSTOM_REGEN, 2s); // tick interval

                    break;
                }

                    // BOT Follows player again
                    // For timed events
                case FSB_EVENT_RESUME_FOLLOW:
                {
                    Unit* owner = me->GetOwner();
                    Player* player = owner ? owner->ToPlayer() : nullptr;

                    ResumeFollow(player);

                    break;
                }
                case FSB_EVENT_INITIATE_COMBAT:
                {
                    TC_LOG_DEBUG("scripts.ai.fsb", "FSB: UpdateAI Event Priest Combat triggered with role = {}", roleState);

                    DoAction(FSB_ACTION_INITIATE_COMBAT);

                    // ?? ALWAYS reschedule while combat exists for player OR bot
                    if (FSBUtilsCombat::IsCombatActive(me))
                    {
                        events.ScheduleEvent(FSB_EVENT_INITIATE_COMBAT, 1s);
                    }

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

                    hired = false;
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

                case FSB_EVENT_PERIODIC_MAINTENANCE:
                {
                    uint32 now = getMSTime();

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

                    // BOT LEVEL Update
                    if (player && hired)
                    {
                        FSBUtilsStats::UpdateBotLevelToPlayer(me, player, _regenMods);
                    }

                    // BOT Check OOC Actions
                    if(!me->IsInCombat() && me->IsAlive())
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

                        if(!me->HasUnitState(UNIT_STATE_CASTING))
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

                            if(fortTarget == me)
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
                        events.ScheduleEvent(FSB_EVENT_PRIEST_OOC_HEAL_PLAYER, 100ms);
                    }

                    // PLAYER check on combat for buffs and combat spells
                    // Different Triggers per role
                    if (player && player->IsAlive() && player->IsInCombat() && !_playerCombatStarted)
                    {
                        _playerCombatStarted = true;
                        _playerCombatEnded = false;

                        if(FSBUtils::GetRole(me) == FSB_Roles::FSB_ROLE_HEALER)
                            events.ScheduleEvent(FSB_EVENT_PRIEST_INITIAL_COMBAT_SPELLS_PLAYER, 100ms);

                        if (!me->HasUnitFlag(UNIT_FLAG_IN_COMBAT))
                        {
                            me->SetUnitFlag(UNIT_FLAG_IN_COMBAT);
                        }

                        events.ScheduleEvent(FSB_EVENT_INITIATE_COMBAT, 2s);
                    }

                    if (player && player->IsAlive() && !player->IsInCombat() && _playerCombatStarted)
                    {
                        _playerCombatStarted = false;
                        _playerCombatEnded = true;
                    }

                    FSBUtilsCombat::BuildBotGroup(me->ToCreature(), botGroup_, 100.0f);

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

                    if (player && player->HasAuraType(SPELL_AURA_MOUNTED))
                    {
                        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Maintenance check: Player has mounted aura");
                    }

                    events.ScheduleEvent(FSB_EVENT_PERIODIC_MAINTENANCE, 1s);

                    //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Event Periodic Maintenance Reached the end"); // TEMP LOG

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
                    DoAction(FSB_ACTION_OOC_HEAL_PLAYER);
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
        uint32 ManaPotionSpellId = FSBUtilsSpells::GetManaPotionSpellForLevel(me->GetLevel());
        std::unordered_map<uint32 /*spellId*/, uint32 /*nextReadyMs*/> _spellCooldowns;

        private:
            EventMap events;
            ObjectGuid _playerGuid;

            FSBUtilsStatsMods _regenMods;
            std::vector<Unit*> botGroup_;

            bool combatEmergency = false;
            bool groupHealthy = false;

            uint32 hireTimeLeft = 0;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_followship_bots_priestAI(creature);
    }
};


void AddSC_followship_bots_priest()
{
    FollowshipBotsConfig::Load();

    new npc_followship_bots_priest();
}
