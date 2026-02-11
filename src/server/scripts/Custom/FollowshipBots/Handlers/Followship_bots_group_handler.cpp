#include "Followship_bots_utils.h"

#include "Followship_bots_group_handler.h"

namespace FSBGroup
{
    Unit* BotGetFirstGroupHealer(const std::vector<Unit*>& group)
    {
        for (Unit* member : group)
        {
            if (!member || !member->IsAlive())
                continue;

            FSB_Roles role = FSBUtils::GetRole(member->ToCreature());

            // Check healer role
            if (role == FSB_ROLE_HEALER)
            {
                return member; // first healer found
            }
        }

        return nullptr; // no healer in group
    }
}
