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
#include "Config.h"
#include "ContributionPackets.h"
#include "Creature.h"
#include "DB2Stores.h"
#include "GameTime.h"
#include "Log.h"
#include "ManagedWorldStateMgr.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"
#include "Timer.h"
#include "World.h"
#include "WorldSession.h"
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
    _contributionsByManagedWorldState.clear();

    for (CreatureXContributionEntry const* entry : sCreatureXContributionStore)
        _contributionsByCreature[uint32(entry->CreatureID)].push_back(uint32(entry->ContributionID));

    // Reverse index for the display push: Contribution -> ManagedWorldStateInput -> ManagedWorldState.
    for (ContributionEntry const* contribution : sContributionStore)
        if (ManagedWorldStateInputEntry const* input = sManagedWorldStateInputStore.LookupEntry(contribution->ManagedWorldStateInputID))
            if (input->ManagedWorldStateID > 0)
                _contributionsByManagedWorldState[uint32(input->ManagedWorldStateID)].push_back(contribution->ID);

    TC_LOG_INFO("server.loading", ">> Loaded contribution collectors for {} creatures ({} managed world states) in {} ms",
        _contributionsByCreature.size(), _contributionsByManagedWorldState.size(), GetMSTimeDiffToNow(oldMSTime));
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

    // Compute the total contribution (progress bar == resources contributed; a contribution with no consumable
    // objectives still counts as one unit). The progress increment is the total quantity that would be turned in.
    int32 contributed = 0;
    for (QuestObjective const& objective : quest->GetObjectives())
    {
        switch (objective.Type)
        {
            case QUEST_OBJECTIVE_ITEM:
            case QUEST_OBJECTIVE_CURRENCY:
                contributed += std::max<int32>(objective.Amount, 0);
                break;
            default:
                break;
        }
    }
    if (contributed <= 0)
        contributed = 1;

    // Record the progress BEFORE consuming the cost. AddProgress no-ops (returns false) when the managed world state
    // is unknown or already clamped at its target - if we destroyed the items/currency first the player would lose
    // the cost for zero progress with no refund. Only consume once the progress is actually recorded.
    if (!sManagedWorldStateMgr->AddProgress(uint32(input->ManagedWorldStateID), contributed))
        return;

    for (QuestObjective const& objective : quest->GetObjectives())
    {
        int32 const amount = std::max<int32>(objective.Amount, 0);
        switch (objective.Type)
        {
            case QUEST_OBJECTIVE_ITEM:
                player->DestroyItemCount(uint32(objective.ObjectID), uint32(amount), true);
                break;
            case QUEST_OBJECTIVE_CURRENCY:
                player->RemoveCurrency(uint32(objective.ObjectID), amount, CurrencyDestroyReason::QuestTurnin);
                break;
            default:
                break;
        }
    }

    // ManagedWorldStateMgr::AddProgress -> PushProgress writes the new value through WorldStateMgr, whose realm-wide
    // SMSG_UPDATE_WORLD_STATE broadcast is what actually animates the donor's bar (and everyone else's).
}

/* ------------------------------------------------------------------------------------------------------------------
 * Native Contribution Collector last-update ack (SMSG_CONTRIBUTION_LAST_UPDATE_RESPONSE, 0x4202C4).
 *
 * Twelve bytes: uint32 Data (unix time of the last update), uint32 ContributionID, uint32 ContributionGUID. It is
 * only an acknowledgement - the bar itself is painted client-side from Contribution.db2 / ManagedWorldState.db2
 * against the world-state values ManagedWorldStateMgr pushes through WorldStateMgr (SMSG_UPDATE_WORLD_STATE), so
 * nothing here computes state, percentages or results.
 * ---------------------------------------------------------------------------------------------------------------- */

bool ContributionMgr::IsNativeUiEnabled()
{
    return sConfigMgr->GetBoolDefault("Warfront.NativeUI.Enable", false);
}

uint32 ContributionMgr::GetLastUpdateTime(uint32 contributionId) const
{
    auto itr = _lastUpdateTimes.find(contributionId);
    return itr != _lastUpdateTimes.end() ? itr->second : 0u;
}

void ContributionMgr::SendLastUpdate(Player* player, uint32 contributionId, uint32 contributionGuid /*= 0*/) const
{
    if (!player || !IsNativeUiEnabled())
        return;

    // Unknown contribution ids are not acknowledged - the client asked about something that does not exist.
    if (!sContributionStore.LookupEntry(contributionId))
        return;

    WorldPackets::Contribution::ContributionLastUpdateResponse response;
    response.Data = GetLastUpdateTime(contributionId);
    response.ContributionID = contributionId;
    response.ContributionGUID = contributionGuid;
    player->SendDirectMessage(response.Write());
}

void ContributionMgr::BroadcastManagedWorldStateUpdate(uint32 managedWorldStateId)
{
    auto itr = _contributionsByManagedWorldState.find(managedWorldStateId);
    if (itr == _contributionsByManagedWorldState.end())
        return;

    // The bar moved, so every contribution fed by this managed world state has a new "last update" time. This is
    // recorded unconditionally: it is server state, not wire traffic.
    uint32 const now = uint32(GameTime::GetGameTime());
    for (uint32 contributionId : itr->second)
        _lastUpdateTimes[contributionId] = now;

    if (!IsNativeUiEnabled())
        return;

    // The bar values themselves already reached every client through WorldStateMgr's realm-wide
    // SMSG_UPDATE_WORLD_STATE broadcast; this only re-acks the collector's last-update timestamp.
    for (auto const& [accountId, session] : sWorld->GetAllSessions())
    {
        Player* player = session->GetPlayer();
        if (!player || !player->IsInWorld())
            continue;

        for (uint32 contributionId : itr->second)
            SendLastUpdate(player, contributionId);
    }
}
