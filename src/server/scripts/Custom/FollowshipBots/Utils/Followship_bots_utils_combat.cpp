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
    bool IsCombatActive(Creature* bot)
    {
        if (!bot)
            return false;

        auto owner = FSBMgr::GetBotOwner(bot);
        Player* player = owner ? owner->ToPlayer() : nullptr;

        return bot->IsInCombat() || (player && player->IsInCombat());
    }

    Unit* GetRandomAttacker(Creature* bot)
    {
        auto const& attackers = bot->getAttackers();

        if (attackers.empty())
            return nullptr;

        auto it = attackers.begin();
        std::advance(it, urand(0, attackers.size() - 1));

        return *it;
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



    std::vector<Unit*> BotGetHealCandidates(const std::vector<Unit*>& group, float lowHpThreshold)
    {
        std::vector<Unit*> candidates;

        for (Unit* unit : group)
        {
            if (!unit || !unit->IsAlive())
                continue;

            // Emergency 1: HP below threshold
            if (unit->GetHealthPct() <= lowHpThreshold)
            {
                candidates.push_back(unit);
                //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Heal / Emergency Candidate: {}", unit->GetName());
            }
        }

        // Optional: sort by urgency (lowest HP first)
        std::sort(candidates.begin(), candidates.end(),
            [](Unit* a, Unit* b) { return a->GetHealthPct() < b->GetHealthPct(); });

        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Heal / Emergency list size: {}", candidates.size());
        return candidates;
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

    void CheckBotAllies(Unit* me, std::vector<Unit*>& outGroup, float searchRange)
    {
        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: BuildBotGroup triggered"); // TEMP-LOG

        outGroup.clear();

        if (!me || !me->ToCreature()->IsBot())
            return;

        Player* owner = FSBMgr::GetBotOwner(me);
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

            if (!unit->ToCreature()->IsBot())
                continue;

            if (unit->GetOwnerGUID() != owner->GetGUID())
                continue;

            outGroup.push_back(unit);
        }

        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: CheckBotAllies. Bot: {} got allies {} and a group of: {}", me->GetName(), nearbyAllies.size(), outGroup.size());  // TEMP-LOG
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
            return (bot->IsWithinMeleeRange(target) || bot->IsWithinCombatRange(target, GetBotChaseDistance(bot)));
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

    void BotDoAttack(Creature* bot, Unit* target, bool chase, uint16 /*moveState*/)
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

                // Bots with ranged attacks should not care about the chase angle at all.
                float chaseDistance = GetBotChaseDistance(bot);
                float angle = chaseDistance == 2.f ? float(M_PI) : frand(-2.f, 2.f);
                float tolerance = chaseDistance == 0.f ? float(M_PI_4) : float(M_PI * 2);
                bot->GetMotionMaster()->MoveChase(target, ChaseRange(0.f, chaseDistance), ChaseAngle(angle, tolerance));
            }
        }
    }

    Unit* BotSelectNextTarget(Creature* bot, bool allowAutoSelect, std::vector<Unit*> botGroup_)
    {
        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: BotSelectNextTarget with allowAutoSelect: {}", allowAutoSelect);

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
            //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: BotSelectNextTarget with myAttacker: {}", myAttacker->GetName());
            if (!myAttacker->HasBreakableByDamageCrowdControlAura())
                return myAttacker;
        }

        // 1. Bot is being attacked ? defend self
        if (!bot->getAttackers().empty())
        {
            // Pick the highest-threat attacker on the bot
            Unit* attacker = bot->GetVictim();

            if (!attacker)
                attacker = *bot->getAttackers().begin();

            if (attacker && attacker->IsAlive())
                return attacker;
        }

        // Not sure why we wouldn't have an owner but just in case...
        Player* owner = FSBMgr::GetBotOwner(bot);
        if (!owner)
            return nullptr;

        // Check owner attackers
        if (Unit* ownerAttacker = owner->getAttackerForHelper())
        {
            //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: BotSelectNextTarget with ownerAttacker: {}", ownerAttacker->GetName());
            if (!ownerAttacker->HasBreakableByDamageCrowdControlAura())
                return ownerAttacker;
        }

        // Check owner victim
        // 3.0.2 - Pets now start attacking their owners victim in defensive mode as soon as the hunter does
        if (Unit* ownerVictim = owner->GetVictim())
            return ownerVictim;

        // 3. Check other bots owned by the same player
        for (Unit* otherBot : botGroup_)
        {
            if (!otherBot || otherBot == bot)
                continue;

            if (!otherBot->IsAlive() || !otherBot->IsInCombat())
                continue;

            Unit* otherVictim = otherBot->GetVictim();
            if (!otherVictim || !otherVictim->IsAlive())
                continue;

            // Don't break CC
            if (otherVictim->HasBreakableByDamageCrowdControlAura())
                continue;

            TC_LOG_DEBUG("scripts.ai.fsb",
                "FSB: BotSelectNextTarget using sibling bot {} target {}",
                otherBot->GetName(), otherVictim->GetName());

            return otherVictim;
        }

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
        bot->RemoveUnitFlag(UNIT_FLAG_IN_COMBAT); 
    }

    float GetBotChaseDistance(Creature* bot)
    {
        // 1?? HARD OVERRIDES (state-based, must win)
        if (ShouldForceMeleeRange(bot))
            return 2.0f;

        FSB_Roles role = FSBUtils::GetRole(bot);
        FSB_Class cls = FSBUtils::GetBotClassForEntry(bot->GetEntry());

        switch (role)
        {
        case FSB_ROLE_TANK:
            return 2.0f;   // glue yourself to target

        case FSB_ROLE_MELEE_DAMAGE:
            return 2.5f;

        case FSB_ROLE_RANGED_ARCANE:
        case FSB_ROLE_RANGED_FIRE:
        case FSB_ROLE_RANGED_FROST:
        case FSB_ROLE_RANGED_DAMAGE:
            return 28.0f;

        case FSB_ROLE_HEALER:
            return 30.0f;

        default:
            break;
        }

        return GetFallbackChaseDistanceForClass(cls);
    }

    float GetFallbackChaseDistanceForClass(FSB_Class cls)
    {
        switch (cls)
        {
        case FSB_Class::Mage:
        case FSB_Class::Warlock:
            return 30.0f;

        case FSB_Class::Priest:
            return 25.0f; // priest tends to step closer for fear / dispel

        case FSB_Class::Hunter:
            return 35.0f; // dead zone avoidance

        case FSB_Class::Rogue:
        case FSB_Class::Warrior:
        case FSB_Class::Paladin:
            return 2.5f;

        default:
            return 5.0f;
        }
    }

    bool ShouldForceMeleeRange(Creature* bot)
    {
        if (!bot)
            return false;

        // Only mana users
        if (bot->GetPowerType() != POWER_MANA)
            return false;

        uint32 maxMana = bot->GetMaxPower(POWER_MANA);
        if (maxMana == 0)
            return false;

        float manaPct = 100.0f * bot->GetPower(POWER_MANA) / maxMana;

        // < 5% mana = desperate mode
        if (manaPct >= 5.0f)
            return false;

        // Optional: must be in combat
        if (!bot->IsInCombat())
            return false;

        return true;
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

        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: OnBotOwnerAttacked target: {}, move state: {}", victim->GetName(), moveState);
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

        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: OnBotOwnerAttackedBy attacker: {}, move state: {}", attacker->GetName(), moveState);
    }
}
