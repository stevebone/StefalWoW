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
#include "ObjectAccessor.h"
#include "QuestDef.h"
#include "Timer.h"
#include "UnitDefines.h"
#include "Warfront.h"
#include "World.h"
#include "WorldSession.h"
#include <algorithm>

namespace
{
    // Retail's ContributionResult::MustBeNearNpc is a proximity check, not a gossip interaction: the warfront
    // collectors ("Warfront Alliance/Horde Contribution Dummy" 143709 / 143707) ship with CreatureDisplayID 13069
    // (the invisible stalker) and UNIT_FLAG_UNINTERACTIBLE, so they can never satisfy GetNPCIfCanInteractWith.
    // We therefore validate the collector by flag + distance, exactly like the client's own error vocabulary does.
    constexpr float CONTRIBUTION_COLLECTOR_RANGE = 100.0f;

    // Fallback donation cost, used when the contribution's ManagedWorldStateInput carries no QuestID.
    //
    // This is the live situation for all four warfront bars in build 68275: ManagedWorldStateInput 10/115/116/117
    // all have QuestID = 0. The value was only ever populated for the 8.0 Arathi launch build (8.0.1.27980 had
    // QuestID 52048 / ValidInputConditionID 61946 on inputs 10 and 115) and was blanked again in 8.1.0.28724; quest
    // 52048 does not exist in any modern quest_template, so its objectives - the actual retail cost - cannot be
    // recovered offline. War Resources (currency 1560) is the BfA war-effort currency and is the server-side
    // stand-in; the amount and the resulting bar movement are operator-tunable.
    constexpr uint32 WAR_EFFORT_CURRENCY = 1560;    // War Resources
    constexpr uint32 WAR_EFFORT_DEFAULT_AMOUNT = 100;

    // One donation is worth this many minutes of the bar's own passive accumulation
    // (ManagedWorldState.AccumulationAmountPerMinute), so a donation is always meaningful relative to the target
    // regardless of which bar it feeds.
    constexpr float WAR_EFFORT_DEFAULT_MINUTES = 60.0f;
}

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

uint32 ContributionMgr::GetManagedWorldStateForContribution(uint32 contributionId) const
{
    ContributionEntry const* contribution = sContributionStore.LookupEntry(contributionId);
    if (!contribution)
        return 0;

    ManagedWorldStateInputEntry const* input = sManagedWorldStateInputStore.LookupEntry(contribution->ManagedWorldStateInputID);
    if (!input || input->ManagedWorldStateID <= 0)
        return 0;

    return uint32(input->ManagedWorldStateID);
}

bool ContributionMgr::IsCollectorFor(Creature const* creature, uint32 contributionId) const
{
    if (!creature)
        return false;

    // The client only ever routes C_ContributionCollector.Contribute at an NPC carrying the collector flag.
    if (!creature->HasNpcFlag2(UNIT_NPC_FLAG_2_CONTRIBUTION_COLLECTOR))
        return false;

    return CreatureOffersContribution(creature->GetEntry(), contributionId);
}

uint32 ContributionMgr::GetContributionForCreatureAndManagedWorldState(uint32 creatureEntry, uint32 managedWorldStateId) const
{
    if (!managedWorldStateId)
        return 0;

    auto itr = _contributionsByCreature.find(creatureEntry);
    if (itr == _contributionsByCreature.end())
        return 0;

    for (uint32 contributionId : itr->second)
        if (GetManagedWorldStateForContribution(contributionId) == managedWorldStateId)
            return contributionId;

    return 0;
}

bool ContributionMgr::GetContributionCost(uint32 contributionId, ContributionCost& out) const
{
    out = ContributionCost();

    ContributionEntry const* contribution = sContributionStore.LookupEntry(contributionId);
    if (!contribution)
        return false;

    ManagedWorldStateInputEntry const* input = sManagedWorldStateInputStore.LookupEntry(contribution->ManagedWorldStateInputID);
    if (!input)
        return false;

    out.QuestID = uint32(input->QuestID);

    // Preferred: the client data defines a turn-in quest, whose item/currency objectives ARE the cost - this is also
    // what C_ContributionCollector.GetRequiredContributionCurrency/Item reads to paint the button.
    if (Quest const* quest = out.QuestID ? sObjectMgr->GetQuestTemplate(out.QuestID) : nullptr)
    {
        int32 contributed = 0;
        for (QuestObjective const& objective : quest->GetObjectives())
        {
            int32 const amount = std::max<int32>(objective.Amount, 0);
            switch (objective.Type)
            {
                case QUEST_OBJECTIVE_ITEM:
                    out.ItemID = uint32(objective.ObjectID);
                    out.ItemAmount = uint32(amount);
                    contributed += amount;
                    break;
                case QUEST_OBJECTIVE_CURRENCY:
                    out.CurrencyID = uint32(objective.ObjectID);
                    out.CurrencyAmount = uint32(amount);
                    contributed += amount;
                    break;
                default:
                    break;
            }
        }

        out.ProgressAmount = contributed > 0 ? contributed : 1;
        return true;
    }

    // Fallback: the modern client ships QuestID = 0 for every warfront bar (see the note at the top of this file),
    // so the cost is server-side. It is expressed as War Resources, and the bar movement is expressed in minutes of
    // the bar's own passive accumulation so it scales with whatever ManagedWorldState it feeds.
    out.QuestID = 0;
    out.CurrencyID = sConfigMgr->GetIntDefault("Warfront.Contribution.CurrencyID", WAR_EFFORT_CURRENCY);
    out.CurrencyAmount = sConfigMgr->GetIntDefault("Warfront.Contribution.CurrencyAmount", WAR_EFFORT_DEFAULT_AMOUNT);

    int32 progress = 1;
    if (ManagedWorldStateEntry const* state = sManagedWorldStateStore.LookupEntry(uint32(input->ManagedWorldStateID)))
    {
        float const minutes = std::max(0.0f, sConfigMgr->GetFloatDefault("Warfront.Contribution.MinutesPerDonation", WAR_EFFORT_DEFAULT_MINUTES));
        progress = int32(float(state->AccumulationAmountPerMinute) * minutes);
    }
    out.ProgressAmount = std::max(1, progress);

    return true;
}

char const* ContributionMgr::GetResultText(ContributionResult result)
{
    switch (result)
    {
        case ContributionResult::Success:                return "Your donation has been added to the war effort.";
        case ContributionResult::MustBeNearNpc:          return "You must be at the war table to donate.";
        case ContributionResult::IncorrectState:         return "The war effort cannot accept any more supplies right now.";
        case ContributionResult::InvalidID:              return "That donation is not being collected here.";
        case ContributionResult::QuestDataMissing:       return "The quartermaster has lost the requisition orders.";
        case ContributionResult::FailedConditionCheck:   return "You are not eligible to donate to this war effort.";
        case ContributionResult::UnableToCompleteTurnIn: return "You do not have the supplies the war effort needs.";
        case ContributionResult::InternalError:
        default:                                         return "The donation could not be completed.";
    }
}

ContributionResult ContributionMgr::Contribute(Player* player, ObjectGuid collectorGuid, uint32 contributionId)
{
    if (!player)
        return ContributionResult::InternalError;

    // The collector cannot be resolved with GetNPCIfCanInteractWith: the warfront collectors are invisible,
    // UNINTERACTIBLE dummies. Retail's own failure mode here is MustBeNearNpc, i.e. a proximity check.
    Creature* collector = ObjectAccessor::GetCreature(*player, collectorGuid);
    if (!collector || !collector->IsInWorld() || !player->IsWithinDistInMap(collector, CONTRIBUTION_COLLECTOR_RANGE))
        return ContributionResult::MustBeNearNpc;

    // ...and it must actually offer this contribution (collector flag + CreatureXContribution authorization).
    if (!IsCollectorFor(collector, contributionId))
        return ContributionResult::InvalidID;

    ContributionEntry const* contribution = sContributionStore.LookupEntry(contributionId);
    if (!contribution)
        return ContributionResult::InvalidID;

    ManagedWorldStateInputEntry const* input = sManagedWorldStateInputStore.LookupEntry(contribution->ManagedWorldStateInputID);
    if (!input || input->ManagedWorldStateID <= 0)
        return ContributionResult::InvalidID;

    // Eligibility gate for this contribution input (PlayerCondition).
    if (input->ValidInputConditionID && !ConditionMgr::IsPlayerMeetingCondition(player, input->ValidInputConditionID))
        return ContributionResult::FailedConditionCheck;

    ContributionCost cost;
    if (!GetContributionCost(contributionId, cost))
        return ContributionResult::QuestDataMissing;

    // Verify the player can pay the full cost first - never consume anything unless everything is affordable.
    if (cost.ItemID && cost.ItemAmount && !player->HasItemCount(cost.ItemID, cost.ItemAmount))
        return ContributionResult::UnableToCompleteTurnIn;
    if (cost.CurrencyID && cost.CurrencyAmount && !player->HasCurrency(cost.CurrencyID, cost.CurrencyAmount))
        return ContributionResult::UnableToCompleteTurnIn;

    // Record the progress BEFORE consuming the cost. AddProgress returns false when the managed world state is
    // unknown or already clamped at its target - if we destroyed the items/currency first the player would lose the
    // cost for zero progress with no refund. Only consume once the progress is actually recorded. A full bar is
    // exactly retail's ContributionResult::IncorrectState ("only while under construction").
    if (!sManagedWorldStateMgr->AddProgress(uint32(input->ManagedWorldStateID), cost.ProgressAmount))
        return ContributionResult::IncorrectState;

    if (cost.ItemID && cost.ItemAmount)
        player->DestroyItemCount(cost.ItemID, cost.ItemAmount, true);
    if (cost.CurrencyID && cost.CurrencyAmount)
        player->RemoveCurrency(cost.CurrencyID, int32(cost.CurrencyAmount), CurrencyDestroyReason::QuestTurnin);

    // Retail closes the loop through a kill credit: the weekly "Warfront Contribution" quests (53185 Alliance /
    // 53209 Horde) are completed by objective type 0 on creature 143337 "Donation Credit".
    player->KilledMonsterCredit(WarfrontDonationCreditCreature);

    TC_LOG_DEBUG("warfront", "Contribution {} by {} ({}) at collector {}: +{} progress on managed world state {}.",
        contributionId, player->GetName(), player->GetGUID().ToString(), collector->GetEntry(),
        cost.ProgressAmount, input->ManagedWorldStateID);

    // ManagedWorldStateMgr::AddProgress -> PushProgress writes the new value through WorldStateMgr, whose realm-wide
    // SMSG_UPDATE_WORLD_STATE broadcast is what actually animates the donor's bar (and everyone else's), and whose
    // OnReachedTarget hop is what flips a warfront from CONTRIBUTION to SIEGE.
    return ContributionResult::Success;
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
