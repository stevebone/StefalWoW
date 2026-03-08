#include <algorithm>

#include "Creature.h"
#include "GridNotifiers.h"
#include "ScriptMgr.h"
#include "Unit.h"

#include "Followship_bots_config.h"

#include "Followship_bots_utils.h"
#include "Followship_bots.h"
#include "Followship_bots_mgr.h"

#include "Followship_bots_group_handler.h"

namespace FSBUtilsCombat
{
    

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
}

namespace FSBCombatUtils
{
    bool IsCombatActive(Creature* bot)
    {
        if (!bot)
            return false;

        auto owner = FSBMgr::Get()->GetBotOwner(bot);

        return bot->IsInCombat() || (owner && owner->IsInCombat());
    }

    float GetBotChaseDistance(Creature* bot)
    {
        // 1?? HARD OVERRIDES (state-based, must win)
        //if (ShouldForceMeleeRange(bot))
        //    return 2.0f;

        FSB_Roles role = FSBMgr::Get()->GetRole(bot);
        FSB_Class cls = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());

        switch (role)
        {
        case FSB_ROLE_TANK:
            return 1.0f;   // glue yourself to target

        case FSB_ROLE_MELEE_DAMAGE:
        case FSB_ROLE_MELEE_DAMAGE_2:
        case FSB_ROLE_MELEE_DAMAGE_3:
            return 2.f;

        case FSB_ROLE_RANGED_ARCANE:
        case FSB_ROLE_RANGED_FIRE:
        case FSB_ROLE_RANGED_FROST:
        case FSB_ROLE_RANGED_DAMAGE:
        case FSB_ROLE_RANGED_DESTRUCTION:
        case FSB_ROLE_RANGED_AFFLICTION:
        case FSB_ROLE_RANGED_DEMONOLOGY:
            return 28.0f;

        case FSB_ROLE_ASSIST:
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

        case FSB_Class::Shaman:
        case FSB_Class::Druid:
        case FSB_Class::Priest:
            return 25.0f; // priest tends to step closer for fear / dispel

        case FSB_Class::Hunter:
            return 35.0f; // dead zone avoidance

        case FSB_Class::Rogue:
        case FSB_Class::Warrior:
        case FSB_Class::Paladin:
            return 2.f;

        default:
            return 5.0f;
        }
    }

    bool HasHostileInRange(Unit* bot, float range, uint32 count)
    {
        if (!bot)
            return false;

        uint32 found = 0;

        std::list<Unit*> targets;
        Trinity::AnyUnfriendlyUnitInObjectRangeCheck check(bot, bot, range);
        Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(bot, targets, check);

        Cell::VisitAllObjects(bot, searcher, range);

        for (Unit* u : targets)
        {
            if (!u->IsAlive())
                continue;

            if (!bot->IsValidAttackTarget(u))
                continue;

            if (++found >= count)
                return true;
        }

        return false;
    }

    bool IsOutOfCombatFor(Creature* bot, uint32 ms)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (bot->IsInCombat())
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        auto combatTimer = baseAI->botOutOfCombatTimer;

        // Prevent triggering before first combat
        if (combatTimer == 0)
            return false;


        return GetMSTimeDiffToNow(combatTimer) >= ms;
    }
}
