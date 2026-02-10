#include "Followship_bots_utils.h"

#include "Followship_bots_group_handler.h"

namespace FSBGroup
{
    Unit* BotGetFirstGroupHealer(const std::vector<Unit*>& group)
    {
        for (Unit* bot : group)
        {
            if (!bot || !bot->IsAlive())
                continue;

            FSB_Roles role = FSBUtils::GetRole(bot->ToCreature());

            // Check healer role
            if (role == FSB_ROLE_HEALER)
            {
                return bot; // first healer found
            }
        }

        return nullptr; // no healer in group
    }
}
