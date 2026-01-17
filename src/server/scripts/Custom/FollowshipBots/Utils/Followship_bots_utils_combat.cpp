#include <algorithm>

#include "ScriptMgr.h"
#include "Unit.h"
#include "Creature.h"

#include "Followship_bots_utils_combat.h"
#include "Followship_bots_utils.h"
#include "Followship_bots.h"
#include "Followship_bots_mgr.h"

namespace FSBUtilsCombat
{
    bool IsCombatActive(Unit* me)
    {
        if (!me)
            return false;

        Unit* owner = me->GetOwner();
        Player* player = owner ? owner->ToPlayer() : nullptr;

        return me->IsInCombat() || (player && player->IsInCombat());
    }

    uint8 CountActiveAttackers(Unit* me)
    {
        if (!me)
            return 0;

        uint8 count = 0;

        for (ThreatReference const* ref : me->GetThreatManager().GetUnsortedThreatList())
        {
            Unit* u = ref->GetVictim();
            if (!u || !u->IsAlive())
                continue;

            if (!me->IsInMap(u))
                continue;

            ++count;
        }

        return count;
    }

    uint8 CountAttackersOn(Unit* who)
    {
        if (!who)
            return 0;

        uint8 count = 0;

        for (ThreatReference const* ref : who->GetThreatManager().GetUnsortedThreatList())
        {
            Unit* u = ref->GetVictim();
            if (u && u->IsAlive() && u->GetVictim() == who)
                ++count;
        }

        return count;
    }

    void SayCombatMessage(Creature* me, Unit* target, uint32 integer, FSBSayType sayType, uint32 spellId)
    {
        if (!me)
            return;

        uint32 now = getMSTime();

        // Cooldown to prevent spamming
        static uint32 nextSayMs = 0;
        if (now < nextSayMs)
            return;

        // Spell name lookup
        std::string spellName;
        if (spellId)
            spellName = FSBUtilsSpells::GetSpellName(spellId);

        // Target name
        std::string targetName;
        if (target)
            targetName = target->GetName();

        // Build the message
        std::string msg = FSBUtilsTexts::BuildNPCSayText(targetName, integer, sayType, spellName);
        me->Say(msg, LANG_UNIVERSAL);

        // Set next say cooldown: random between 3-5 minutes
        nextSayMs = now + urand(3 * MINUTE * IN_MILLISECONDS, 5 * MINUTE * IN_MILLISECONDS);
    }

    Unit* FSBUtilsCombat::SelectDamageTarget(Creature* me, const std::vector<Unit*>& group, Unit* playerTarget)
    {
        if (!me)
            return nullptr;

        Unit* owner = me->GetOwner();
        Player* player = owner ? owner->ToPlayer() : nullptr;

        FSB_Roles role = FSBUtils::GetRole(me);

        std::vector<Unit*> combatTargets;
        auto const& threatList = me->GetThreatManager().GetSortedThreatList();
        for (ThreatReference const* ref : threatList)
        {
            if (Unit* u = ref->GetVictim())
                combatTargets.push_back(u);
        }

        

        // Player target exists?
        if (playerTarget && playerTarget->IsAlive())
        {
            if (role == FSB_ROLE_DAMAGE || role == FSB_ROLE_ASSIST || role == FSB_ROLE_TANK)
                return playerTarget;
        }

        switch (role)
        {
        case FSB_ROLE_TANK:
        {
            // Tank bots prefer player target first
            if (playerTarget && playerTarget->IsAlive())
                return playerTarget;

            if (player)
            {
                auto const& pthreatList = player->GetThreatManager().GetSortedThreatList();
                for (ThreatReference const* ref : pthreatList)
                {
                    if (Unit* u = ref->GetVictim())
                        combatTargets.push_back(u);
                }
            }

            std::sort(combatTargets.begin(), combatTargets.end());
            combatTargets.erase(std::unique(combatTargets.begin(), combatTargets.end()), combatTargets.end());

            // Then bot's threat list
            if (!combatTargets.empty())
                return combatTargets.front(); // could prioritize highest threat

            break;
        }

        case FSB_ROLE_HEALER:
        {
            // Healers prioritize tank
            Unit* tank = nullptr;
            for (Unit* u : group)
            {
                if (!u || !u->IsAlive())
                    continue;
                if (FSBUtils::GetRole(u->ToCreature()) == FSB_ROLE_TANK)
                {
                    tank = u;
                    break;
                }
            }

            if (tank)
                return tank->GetVictim();

            if (playerTarget && playerTarget->IsAlive())
                return playerTarget;

            std::sort(combatTargets.begin(), combatTargets.end());
            combatTargets.erase(std::unique(combatTargets.begin(), combatTargets.end()), combatTargets.end());

            // Fallback: bot's own threat list
            if (!combatTargets.empty())
                return combatTargets.front();

            break;
        }

        case FSB_ROLE_DAMAGE:
        case FSB_ROLE_ASSIST:
        default:
        {
            if (playerTarget && playerTarget->IsAlive())
                return playerTarget;

            Unit* tank = nullptr;
            for (Unit* u : group)
            {
                if (!u || !u->IsAlive())
                    continue;
                if (FSBUtils::GetRole(u->ToCreature()) == FSB_ROLE_TANK)
                {
                    tank = u;
                    break;
                }
            }

            if (tank)
                return tank->GetVictim();
            else
            {

                if (player)
                {
                    auto const& pthreatList = player->GetThreatManager().GetSortedThreatList();
                    for (ThreatReference const* ref : pthreatList)
                    {
                        if (Unit* u = ref->GetVictim())
                            combatTargets.push_back(u);
                    }
                }
            }

            std::sort(combatTargets.begin(), combatTargets.end());
            combatTargets.erase(std::unique(combatTargets.begin(), combatTargets.end()), combatTargets.end());

            if (!combatTargets.empty())
                return combatTargets.front();

            break;
        }
        }

        // Fallback: pick any alive enemy in threat list
        for (Unit* u : combatTargets)
        {
            if (u && u->IsAlive())
                return u;
        }

        return nullptr;
    }
    Unit* SelectHealTarget(Unit* me, const std::vector<Unit*>& group)
    {
        TC_LOG_DEBUG("scripts.ai.fsb", "FSB Combat: SelectHealTarget Triggered");
        TC_LOG_DEBUG("scripts.ai.fsb", "SelectHealTarget called: me={}, role={}, hp={}", me->GetName(), FSBUtils::GetRole(me->ToCreature()), me->GetHealthPct());

        if (!me)
            return nullptr;

        FSB_Roles myRole = FSBUtils::GetRole(me->ToCreature());
        Player* owner = me->GetOwner() ? me->GetOwner()->ToPlayer() : nullptr;

        TC_LOG_DEBUG("scripts.ai.fsb", "FSB Combat: SelectHealTarget Bot Role: {}", myRole);

        // -------------------------------
        // ROLE: HEALER
        // -------------------------------
        if (myRole == FSB_ROLE_HEALER)
        {
            // 1. Tank @ 80%
            for (Unit* u : group)
            {
                if (!u || !u->IsAlive())
                    continue;

                if (FSBUtils::GetRole(u->ToCreature()) == FSB_ROLE_TANK &&
                    u->GetHealthPct() < 80.0f)
                {
                    TC_LOG_DEBUG("scripts.ai.fsb", "FSB Combat: SelectHealTarget Found Tank to heal");
                    return u;
                }
            }

            // 2. Owner player @ 80%
            if (owner && owner->IsAlive() && owner->GetHealthPct() < 80.0f)
            {
                TC_LOG_DEBUG("scripts.ai.fsb", "FSB Combat: SelectHealTarget Found Owner to heal");
                return owner;
            }

            // 3. Any other group member @ 50%
            Unit* lowest = nullptr;
            float lowestPct = 50.0f;

            for (Unit* u : group)
            {
                if (!u || !u->IsAlive())
                    continue;

                if (u->GetHealthPct() < lowestPct)
                {
                    lowestPct = u->GetHealthPct();
                    lowest = u;
                }
            }

            return lowest; // nullptr if nobody qualifies
        }

        // -------------------------------
        // ROLE: ASSIST
        // -------------------------------
        if (myRole == FSB_ROLE_ASSIST)
        {
            Unit* lowest = nullptr;
            float lowestPct = 50.0f;

            for (Unit* u : group)
            {
                if (!u || !u->IsAlive())
                    continue;

                if (u->GetHealthPct() < lowestPct)
                {
                    lowestPct = u->GetHealthPct();
                    lowest = u;
                }
            }

            return lowest;
        }

        // -------------------------------
        // ROLE: TANK
        // -------------------------------
        if (myRole == FSB_ROLE_TANK)
        {
            if (me->GetHealthPct() < 80.0f)
                return me;

            return nullptr;
        }

        // -------------------------------
        // ROLE: DAMAGE
        // -------------------------------
        if (myRole == FSB_ROLE_DAMAGE)
        {
            if (me->GetHealthPct() < 50.0f)
            {
                TC_LOG_DEBUG("scripts.ai.fsb", "FSB Combat Damage : Bot Role Damage HP < 50. ");
                return me;
            }

            return nullptr;
        }

        return nullptr;
    }

    std::vector<Unit*> GetEmergencyCandidates(const std::vector<Unit*>& group, float lowHpThreshold)
    {
        std::vector<Unit*> emergencies;

        for (Unit* unit : group)
        {
            if (!unit || !unit->IsAlive())
                continue;

            // Emergency 1: HP below threshold
            if (unit->GetHealthPct() <= lowHpThreshold)
            {
                emergencies.push_back(unit);
                continue;
            }

            // Emergency 2: dispellable debuff
            if (FSBUtilsSpells::HasDispellableDebuff(unit))
            {
                emergencies.push_back(unit);
                continue;
            }

            // You can add more emergency checks here in the future
            // e.g., specific negative auras, debuffs, status effects
        }

        // Optional: sort by urgency (lowest HP first)
        std::sort(emergencies.begin(), emergencies.end(),
            [](Unit* a, Unit* b) { return a->GetHealthPct() < b->GetHealthPct(); });

        return emergencies;
    }
    float CalculateEmergencyPriority(Unit* unit)
    {
        if (!unit)
            return 0.f;

        // Get role (works for any bot)
        FSB_Roles role = FSBUtils::GetRole(unit->ToCreature());

        // Assign priority values (higher = more urgent)
        if (role == FSB_ROLE_HEALER)           // healer
            return 80.f;
        if (role == FSB_ROLE_TANK)             // tank
            return 100.f;

        // If player, slightly lower than tank
        if (unit->IsPlayer())
            return 90.f;

        // Default: based on missing health percentage
        return 50.f + (100.f - unit->GetHealthPct()); // 50..150 based on HP
    }
    void SortEmergencyTargets(std::vector<Unit*>& targets)
    {
        std::sort(targets.begin(), targets.end(), [](Unit* a, Unit* b)
            {
                return FSBUtilsCombat::CalculateEmergencyPriority(a) > FSBUtilsCombat::CalculateEmergencyPriority(b);
            });
    }

    void BuildBotGroup(Unit* me, std::vector<Unit*>& outGroup, float searchRange)
    {
        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: BuildBotGroup triggered"); // TEMP-LOG

        outGroup.clear();

        if (!me || !me->ToCreature()->IsBot())
            return;

        Player* owner = me->GetOwner() ? me->GetOwner()->ToPlayer() : nullptr;
        if (!owner)
            return;

        // Always include owner and self
        outGroup.push_back(owner);
        outGroup.push_back(me);

        // Find nearby allies
        std::list<Unit*> nearbyAllies;

        Trinity::AnyFriendlyUnitInObjectRangeCheck checker(me, me, searchRange, false);
        Trinity::UnitListSearcher<Trinity::AnyFriendlyUnitInObjectRangeCheck> searcher(me, nearbyAllies, checker);

        Cell::VisitAllObjects(me, searcher, searchRange);

        for (Unit* unit : nearbyAllies)
        {
            if (!unit || unit == me || unit == owner)
                continue;

            if (!unit->IsBot())
                continue;

            if (unit->GetOwnerGUID() != owner->GetGUID())
                continue;

            outGroup.push_back(unit);
        }

        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: BuildBotGroup. We got allies of {}", nearbyAllies.size());  // TEMP-LOG
        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: BuildBotGroup. We got group of {}", outGroup.size());  // TEMP-LOG
    }
}

namespace FSBUtilsBotCombat
{
    // checks to determine if bot cannot attack a target
    // result false means bot cannot attack
    bool BotCanAttack(Unit* target, Creature* bot, uint16 moveState)
    {
        // Evaluates wether a bot can attack a specific target based on MoveState, ReactState and other flags
        // IMPORTANT: The order in which things are checked is important, be careful if you add or remove checks

        // Hmmm...
        if (!bot->IsAlive())
            return false;

        if (!target)
            return false;

        if (!target->IsAlive())
        {
            // if target is invalid, pet should evade automaticly
            // Clear target to prevent getting stuck on dead targets
            //me->AttackStop();
            //me->InterruptNonMeleeSpells(false);
            return false;
        }

        // Passive - passive bots cannot attack
        if (bot->HasReactState(REACT_PASSIVE))
            return false;

        // CC - mobs under crowd control can be attacked if owner commanded
        // Perhaps check on role and allow role damamge to attack
        //if (target->HasBreakableByDamageCrowdControlAura())
        //    return me->GetCharmInfo()->IsCommandAttack();

        // Returning - pets ignore attacks only if owner clicked follow
        //if (me->GetCharmInfo()->IsReturning())
        //    return !me->GetCharmInfo()->IsCommandFollow();

        // Stay - can attack if target is within range or commanded to
        if (moveState == FSB_MOVE_STATE_STAY)
            return (bot->IsWithinMeleeRange(target));
        else return true;

        //  Bots attacking something (or chasing) should only switch targets if owner tells them to
        // TO-DO add later based on roles

        // Follow / returning
        // TO-DO check later

        // default, though we shouldn't ever get here
        //return false;
    }

    void BotAttackStart(Creature* bot, Unit* target, uint16 moveState)
    {
        if (!BotCanAttack(target, bot, moveState))
            return;

        // Do not switch target mid-fight
        if (bot->GetVictim() && bot->GetVictim()->IsAlive())
            return;

        bool chase = false;
        if (moveState != FSB_EVENT_MOVE_STAY)
            chase = true;

        BotDoAttack(bot, target, chase, moveState);
    }

    void BotDoAttack(Creature* bot, Unit* target, bool chase, uint16 moveState)
    {
        // Handles attack with or without chase and also resets flags
        // for next update / creature kill

        if (bot->Attack(target, true))
        {
            bot->SetUnitFlag(UNIT_FLAG_IN_COMBAT); // on player bots, this flag indicates we're actively going after a target - that's what we're doing, so set it
            
            if (chase)
            {
                if (bot->HasUnitState(UNIT_STATE_FOLLOW))
                    bot->GetMotionMaster()->Remove(FOLLOW_MOTION_TYPE);

                // Pets with ranged attacks should not care about the chase angle at all.
                float chaseDistance = 2.f;// TO-DO create a dedicated method bot->GetBotChaseDistance();
                float angle = chaseDistance == 0.f ? float(M_PI) : 0.f;
                float tolerance = chaseDistance == 0.f ? float(M_PI_4) : float(M_PI * 2);
                bot->GetMotionMaster()->MoveChase(target, ChaseRange(0.f, chaseDistance), ChaseAngle(angle, tolerance));
            }
            else // (Stay && ((Aggressive || Defensive) && In Melee Range)))
            {
                if (bot->HasUnitState(UNIT_STATE_FOLLOW))
                    bot->GetMotionMaster()->Remove(FOLLOW_MOTION_TYPE);

                bot->GetMotionMaster()->MoveIdle();
            }
        }
    }

    Unit* BotSelectNextTarget(Creature* bot, bool allowAutoSelect)
    {
        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: BotSelectNextTarget with allowAutoSelect: {}", allowAutoSelect);
        // Provides next target selection after current target death.
        // This function should only be called internally by the AI
        // Targets are not evaluated here for being valid targets, that is done in _CanAttack()
        // The parameter: allowAutoSelect lets us disable aggressive pet auto targeting for certain situations

        // Passive bots don't do next target selection
        if (bot->HasReactState(REACT_PASSIVE))
            return nullptr;

        // Check pet attackers first so we don't drag a bunch of targets to the owner
        if (Unit* myAttacker = bot->getAttackerForHelper())
        {
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: BotSelectNextTarget with myAttacker: {}", myAttacker->GetName());
            if (!myAttacker->HasBreakableByDamageCrowdControlAura())
                return myAttacker;
        }

        // Not sure why we wouldn't have an owner but just in case...
        Player* owner = FSBMgr::GetBotOwner(bot);
        if (!owner)
            return nullptr;

        // Check owner attackers
        if (Unit* ownerAttacker = owner->getAttackerForHelper())
        {
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: BotSelectNextTarget with ownerAttacker: {}", ownerAttacker->GetName());
            if (!ownerAttacker->HasBreakableByDamageCrowdControlAura())
                return ownerAttacker;
        }

        // Check owner victim
        // 3.0.2 - Pets now start attacking their owners victim in defensive mode as soon as the hunter does
        if (Unit* ownerVictim = owner->GetVictim())
            return ownerVictim;

        // Neither pet or owner had a target and aggressive pets can pick any target
        // To prevent aggressive pets from chain selecting targets and running off, we
        //  only select a random target if certain conditions are met.
        if (bot->HasReactState(REACT_AGGRESSIVE) && allowAutoSelect)
        {
            // TO-DO add this for damage role
            //if (!me->GetCharmInfo()->IsReturning() || me->GetCharmInfo()->IsFollowing() || me->GetCharmInfo()->IsAtStay())
            //    if (Unit* nearTarget = me->SelectNearestHostileUnitInAggroRange(true, true))
            //        return nearTarget;
        }

        // Default - no valid targets
        return nullptr;
    }

    void BotHandleReturnMovement(Creature* bot, uint16 moveState, float followDist, float followAngle)
    {
        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: BotHandleReturnMovement move state: {}", moveState);
        // Handles moving the bot back to follow or owner
        Player* owner = FSBMgr::GetBotOwner(bot);
        if (!owner)
        {
            bot->GetMotionMaster()->MoveIdle();
            return;
        }

        if(moveState == FSB_MOVE_STATE_FOLLOWING || moveState == FSB_MOVE_STATE_IDLE) // COMMAND_FOLLOW
        {
                if (bot->HasUnitState(UNIT_STATE_CHASE))
                    bot->GetMotionMaster()->Remove(CHASE_MOTION_TYPE);

                bot->GetMotionMaster()->MoveFollow(owner, followDist, followAngle);
            
        }
        bot->RemoveUnitFlag(UNIT_FLAG_IN_COMBAT); // on player pets, this flag indicates that we're actively going after a target - we're returning, so remove it
    }
}

namespace FSBUtilsOwnerCombat
{
    // Owner attacked victim
    bool CheckBotOwnerAttacked(Player* owner, ObjectGuid& lastVictim)
    {
        if (!owner || !owner->IsInCombat())
            return false;

        Unit* victim = owner->GetVictim();
        if (!victim)
            return false;

        if (victim->GetGUID() == lastVictim)
            return false; // no change

        lastVictim = victim->GetGUID();
        return true;
    }

    // Owner was attacked by target
    Unit* CheckBotOwnerAttackedBy(Player* owner)
    {
        if (!owner || !owner->IsInCombat())
            return nullptr;

        for (Unit* attacker : owner->getAttackers())
        {
            if (attacker && attacker->IsAlive())
                return attacker;

        }

        return nullptr;
    }

    // Bot reaction when owner attacks target
    void OnBotOwnerAttacked(Unit* victim, Creature* bot, uint16 moveState)
    {
        // Called when owner attacks something. Allows defensive pets to know
        //  that they need to assist

        // Target might be NULL if called from spell with invalid cast targets
        if (!victim || !bot->IsAlive())
            return;

        // Passive pets don't do anything
        if (bot->HasReactState(REACT_PASSIVE))
            return;

        // Prevent pet from disengaging from current target
        if (bot->GetVictim() && bot->EnsureVictim()->IsAlive())
            return;

        // Continue to evaluate and attack if necessary
        FSBUtilsBotCombat::BotAttackStart(bot, victim, moveState);

        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: OnBotOwnerAttacked target: {}, move state: {}", victim->GetName(), moveState);
    }

    // Bot reaction when owner is attacked by target
    void OnBotOwnerAttackedBy(Unit* attacker, Creature* bot, uint16 moveState)
    {
        // Called when owner takes damage. This function helps keep bots from running off
        //  simply due to owner gaining aggro.

        if (!attacker || !bot->IsAlive())
            return;

        // Passive bots don't do anything
        if (bot->HasReactState(REACT_PASSIVE))
            return;

        // Prevent pet from disengaging from current target
        if (bot->GetVictim() && bot->EnsureVictim()->IsAlive())
            return;

        // Continue to evaluate and attack if necessary
        FSBUtilsBotCombat::BotAttackStart(bot, attacker, moveState);

        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: OnBotOwnerAttackedBy attacker: {}, move state: {}", attacker->GetName(), moveState);
    }
}
