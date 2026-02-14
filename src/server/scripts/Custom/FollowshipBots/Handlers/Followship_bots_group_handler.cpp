#include "Followship_bots_utils.h"
#include "Followship_bots_mgr.h"

#include "Followship_bots_group_handler.h"

namespace FSBGroup
{
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

    Unit* BotGetFirstGroupHealer(const std::vector<Unit*>& group)
    {
        for (Unit* member : group)
        {
            if (!member || !member->IsAlive())
                continue;

            FSB_Roles role = FSBUtils::GetRole(member->ToCreature());

            // Check healer role
            if (role == FSB_Roles::FSB_ROLE_HEALER)
            {
                return member; // first healer found
            }
        }

        return nullptr; // no healer in group
    }

    Unit* BotGetFirstGroupTank(const std::vector<Unit*>& group)
    {
        for (Unit* member : group)
        {
            if (!member || !member->IsAlive())
                continue;

            FSB_Roles role = FSBUtils::GetRole(member->ToCreature());

            // Check healer role
            if (role == FSB_Roles::FSB_ROLE_TANK)
            {
                return member; // first healer found
            }
        }

        return nullptr; // no healer in group
    }
}
