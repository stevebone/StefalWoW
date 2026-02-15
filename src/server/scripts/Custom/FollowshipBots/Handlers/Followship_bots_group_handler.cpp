#include "Followship_bots_utils.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_db.h"

#include "Followship_bots_group_handler.h"

namespace FSBGroup
{
    void BuildLogicalBotGroup(Creature* bot, std::vector<Unit*>& outGroup)
    {
        outGroup.clear();

        Player* owner = FSBMgr::GetBotOwner(bot);

        if (!owner || !bot)
            return;

        outGroup.push_back(owner);
        outGroup.push_back(bot);

        uint64 ownerGuid = owner->GetGUID().GetCounter();

        auto botsPtr = FSBMgr::GetCachedBotsForPlayer(owner);
        if (!botsPtr)
            return;

        for (auto const& botData : *botsPtr)
        {
            if (botData.runtimeGuid.IsEmpty())
                continue;

            if (Unit* botUnit = ObjectAccessor::GetUnit(*owner, botData.runtimeGuid))
            {
                outGroup.push_back(botUnit);
                TC_LOG_DEBUG("scripts.ai.fsb", "FSB: BuildLogicalGroup. Bot: {} got added {} to group of: {}", bot->GetName(), botUnit->GetName(), outGroup.size());  // TEMP-LOG
            }
        }

        
    }


    void CheckBotAllies(Unit* me, std::vector<Unit*>& outGroup, float searchRange)
    {
        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: BuildBotGroup triggered"); // TEMP-LOG

        outGroup.clear();

        if (!me || !me->ToCreature()->IsBot())
            return;

        outGroup.push_back(me);

        Player* owner = FSBMgr::GetBotOwner(me);

        // Always include owner and self
        if (owner)
        {
            outGroup.push_back(owner);

            // Find nearby allies
            std::list<Unit*> nearbyAllies;

            Trinity::AnyFriendlyUnitInObjectRangeCheck checker(me, me, searchRange, false);
            Trinity::UnitListSearcher<Trinity::AnyFriendlyUnitInObjectRangeCheck> searcher(me, nearbyAllies, checker);

            Cell::VisitAllObjects(me, searcher, searchRange);

            for (Unit* unit : nearbyAllies)
            {
                if (unit->ToCreature() && unit->ToCreature()->IsBot())
                {
                    TC_LOG_DEBUG("scripts.ai.fsb",
                        "Bot candidate {} | Alive: {} | Owner: {}",
                        unit->GetName(),
                        unit->IsAlive(),
                        unit->GetOwnerGUID().ToString());
                }

                if (!unit || unit == me || unit == owner)
                    continue;

                if (!unit->ToCreature()->IsBot())
                    continue;

                if (unit->GetOwnerGUID() != owner->GetGUID())
                    continue;

                outGroup.push_back(unit);
            }
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

    Unit* BotGetFirstGroupTank(const std::vector<Unit*>& botGroup)
    {
        for (Unit* member : botGroup)
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

    Unit* BotGetFirstDeadMember(const std::vector<Unit*>& botGroup)
    {

        for (Unit* unit : botGroup)
        {
            if (!unit)
                continue;

            if (!unit->IsAlive())
                return unit;
        }

        return nullptr;
    }
}
