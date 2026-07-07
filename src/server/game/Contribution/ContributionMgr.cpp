/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ContributionMgr.h"
#include "ConditionMgr.h"
#include "Creature.h"
#include "DB2Stores.h"
#include "Log.h"
#include "ManagedWorldStateMgr.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"
#include "Timer.h"
#include <algorithm>

ContributionMgr::ContributionMgr() = default;
ContributionMgr::~ContributionMgr() = default;

ContributionMgr* ContributionMgr::instance()
{
    static ContributionMgr instance;
    return &instance;
}

void ContributionMgr::Load()
{
    uint32 const oldMSTime = getMSTime();
    _contributionsByCreature.clear();

    for (CreatureXContributionEntry const* entry : sCreatureXContributionStore)
        _contributionsByCreature[uint32(entry->CreatureID)].push_back(uint32(entry->ContributionID));

    TC_LOG_INFO("server.loading", ">> Loaded contribution collectors for {} creatures in {} ms",
        _contributionsByCreature.size(), GetMSTimeDiffToNow(oldMSTime));
}

bool ContributionMgr::CreatureOffersContribution(uint32 creatureEntry, uint32 contributionId) const
{
    auto itr = _contributionsByCreature.find(creatureEntry);
    if (itr == _contributionsByCreature.end())
        return false;

    return std::find(itr->second.begin(), itr->second.end(), contributionId) != itr->second.end();
}

void ContributionMgr::Contribute(Player* player, ObjectGuid collectorGuid, uint32 contributionId)
{
    // The collector must be an NPC the player can currently interact with (alive, in range, not hostile).
    Creature* collector = player->GetNPCIfCanInteractWith(collectorGuid, UNIT_NPC_FLAG_NONE, UNIT_NPC_FLAG_2_NONE);
    if (!collector)
        return;

    // ...and it must actually offer this contribution (CreatureXContribution authorization).
    if (!CreatureOffersContribution(collector->GetEntry(), contributionId))
        return;

    ContributionEntry const* contribution = sContributionStore.LookupEntry(contributionId);
    if (!contribution)
        return;

    ManagedWorldStateInputEntry const* input = sManagedWorldStateInputStore.LookupEntry(contribution->ManagedWorldStateInputID);
    if (!input)
        return;

    // Eligibility gate for this contribution input (PlayerCondition).
    if (input->ValidInputConditionID && !ConditionMgr::IsPlayerMeetingCondition(player, input->ValidInputConditionID))
        return;

    // The turn-in cost is the requirement quest's item/currency objectives.
    Quest const* quest = sObjectMgr->GetQuestTemplate(input->QuestID);
    if (!quest)
        return;

    // First pass: verify the player can pay the full cost (never consume anything unless everything is affordable).
    for (QuestObjective const& objective : quest->GetObjectives())
    {
        switch (objective.Type)
        {
            case QUEST_OBJECTIVE_ITEM:
                if (!player->HasItemCount(uint32(objective.ObjectID), uint32(std::max<int32>(objective.Amount, 0))))
                    return;
                break;
            case QUEST_OBJECTIVE_CURRENCY:
                if (!player->HasCurrency(uint32(objective.ObjectID), uint32(std::max<int32>(objective.Amount, 0))))
                    return;
                break;
            default:
                break;
        }
    }

    // Second pass: consume the cost and accumulate the contributed amount as progress. The progress increment is the
    // total quantity turned in (progress bar == resources contributed); if a given managed world state is meant to
    // count turn-ins rather than resources this would be a fixed 1 (documented modelling choice, DB2-content driven).
    int32 contributed = 0;
    for (QuestObjective const& objective : quest->GetObjectives())
    {
        int32 const amount = std::max<int32>(objective.Amount, 0);
        switch (objective.Type)
        {
            case QUEST_OBJECTIVE_ITEM:
                player->DestroyItemCount(uint32(objective.ObjectID), uint32(amount), true);
                contributed += amount;
                break;
            case QUEST_OBJECTIVE_CURRENCY:
                player->RemoveCurrency(uint32(objective.ObjectID), amount, CurrencyDestroyReason::QuestTurnin);
                contributed += amount;
                break;
            default:
                break;
        }
    }

    // A contribution with no consumable objectives still counts as a single unit of progress.
    if (contributed <= 0)
        contributed = 1;

    sManagedWorldStateMgr->AddProgress(uint32(input->ManagedWorldStateID), contributed);
}
