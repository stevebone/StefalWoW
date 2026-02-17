#include "Followship_bots_utils.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_db.h"

#include "Followship_bots_group_handler.h"

namespace FSBGroup
{
    void BuildLogicalBotGroup(Creature* bot, std::vector<Unit*>& outGroup)
    {
        outGroup.clear();

        Player* owner = FSBMgr::Get()->GetBotOwner(bot);

        if (!owner || !bot)
            return;

        outGroup.push_back(owner);

        auto botsPtr = FSBMgr::Get()->GetPersistentBotsForPlayer(owner);
        if (!botsPtr)
        {
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: BuildLogicalGroup. Player has no bots");
            return;
        }

        for (auto const& botData : *botsPtr)
        {
            if (botData.runtimeGuid.IsEmpty())
                continue;

            if (Unit* botUnit = ObjectAccessor::GetUnit(*owner, botData.runtimeGuid))
            {
                outGroup.push_back(botUnit);
                //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: BuildLogicalGroup. Bot: {} got added {} to group of: {}", bot->GetName(), botUnit->GetName(), outGroup.size());  // TEMP-LOG
            }
        }

        
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

    Unit* BotGetDispelMember(Creature* bot, const DispelAbility& ability)
    {
        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return nullptr;

        for (Unit* member : baseAI->botLogicalGroup)
        {
            if (!member || !member->IsInWorld())
                continue;

            // Scan all auras on the member
            for (auto const& auraPair : member->GetAppliedAuras())
            {
                Aura* aura = auraPair.second->GetBase();
                if (!aura)
                    continue;

                SpellInfo const* info = aura->GetSpellInfo();
                if (!info)
                    continue;

                // Only negative auras
                if (info->IsPositive())
                    continue;


                DispelType type = FSBSpellsUtils::ConvertAuraToDispelType(aura);
                if (std::find(ability.dispels.begin(), ability.dispels.end(), type) != ability.dispels.end())
                {
                    return member; // Found someone we can dispel
                }
            }
        }

        return nullptr;
    }
}
