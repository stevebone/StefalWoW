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

#include "WorldSession.h"
#include "ClubFinderMgr.h"
#include "ClubFinderPackets.h"
#include "GameTime.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "RealmList.h"

void WorldSession::HandleClubFinderRequestPendingClubsList(WorldPackets::ClubFinder::RequestPendingClubsList& packet)
{
    TC_LOG_DEBUG("clubfinder", "CMSG_CLUB_FINDER_REQUEST_PENDING_CLUBS_LIST [{}]: SearchType: {}",
        GetPlayerInfo(), packet.SearchType);

    Player* player = GetPlayer();
    if (!player)
        return;

    WorldPackets::ClubFinder::ResponseCharacterApplicationList response;

    std::vector<ClubFinderApplicant const*> applications = sClubFinderMgr->GetApplicationsByPlayer(player->GetGUID());
    for (ClubFinderApplicant const* applicant : applications)
    {
        WorldPackets::ClubFinder::ClubApplicationEntry entry;
        entry.PostingID = applicant->PostingID;
        entry.ApplicantGUID = applicant->PlayerGUID;
        entry.Timestamp = applicant->Timestamp;
        entry.Status = applicant->Status;
        response.Entries.push_back(entry);
    }

    SendPacket(response.Write());
}

void WorldSession::HandleClubFinderRequestClubsData(WorldPackets::ClubFinder::RequestClubsData& packet)
{
    TC_LOG_DEBUG("clubfinder", "CMSG_CLUB_FINDER_REQUEST_CLUBS_DATA [{}]: Type: {}, Flags: {}, Count: {}",
        GetPlayerInfo(), packet.Type, packet.Flags, uint32(packet.PostingIDsLow.size()));

    WorldPackets::ClubFinder::LookupClubPostingsList response;

    for (uint32 postingIDLow : packet.PostingIDsLow)
    {
        ClubFinderPosting const* posting = sClubFinderMgr->GetPosting(postingIDLow);
        if (!posting)
            continue;

        Guild* guild = sGuildMgr->GetGuildById(posting->GuildID);
        if (!guild)
            continue;

        WorldPackets::ClubFinder::ClubPostingEntry entry;
        entry.PostingID = posting->PostingID;
        entry.ClubID = posting->GuildID;
        entry.GuildGUID = guild->GetGUID();
        entry.ApplicantCount = sClubFinderMgr->GetApplicantCount(posting->PostingID);
        entry.MaxApplicants = posting->MaxApplicants;
        entry.SlotsAvailable = posting->SlotsAvailable;
        entry.Interests = posting->Interests;
        entry.Timestamp = posting->Timestamp;
        entry.GuildName = guild->GetName();
        entry.Description = posting->Description;
        entry.RealmName = sRealmList->GetCurrentRealm()->Name;
        response.Entries.push_back(std::move(entry));
    }

    SendPacket(response.Write());
}

void WorldSession::HandleClubFinderRequestSubscribedClubPostingIDs(WorldPackets::ClubFinder::RequestSubscribedClubPostingIDs& packet)
{
    TC_LOG_DEBUG("clubfinder", "CMSG_CLUB_FINDER_REQUEST_SUBSCRIBED_CLUB_POSTING_IDS [{}]: Count: {}",
        GetPlayerInfo(), uint32(packet.PostingIDs.size()));

    WorldPackets::ClubFinder::GetClubPostingIDsResponse response;

    for (uint64 postingID : packet.PostingIDs)
    {
        ClubFinderPosting const* posting = sClubFinderMgr->GetPosting(postingID);
        if (!posting)
            continue;

        WorldPackets::ClubFinder::ClubPostingIDEntry entry;
        entry.PostingGUID = posting->PostingID;
        entry.ClubID = posting->GuildID;
        response.Entries.push_back(entry);
    }

    SendPacket(response.Write());
}

void WorldSession::HandleClubFinderApplicationResponse(WorldPackets::ClubFinder::ApplicationResponse& packet)
{
    TC_LOG_DEBUG("clubfinder", "CMSG_CLUB_FINDER_APPLICATION_RESPONSE [{}]: PostingID: {}, ApplicationID: {}, Response: {}",
        GetPlayerInfo(), packet.PostingID, packet.ApplicationID, packet.Response);

    // TODO: exact field layout needs additional sniff data - for now, just set applicant status
    sClubFinderMgr->SetApplicantStatus(packet.ApplicationID, uint8(packet.Response));
}

void WorldSession::HandleClubFinderPost(WorldPackets::ClubFinder::ClubFinderPost& packet)
{
    TC_LOG_DEBUG("clubfinder", "CMSG_CLUB_FINDER_POST [{}]: Playstyle: {}, Interests: {}, ClassMask: {}, MinLevel: {}, MaxLevel: {}",
        GetPlayerInfo(), packet.Playstyle, packet.Interests, packet.ClassMask, packet.MinLevel, packet.MaxLevel);

    Player* player = GetPlayer();
    if (!player)
        return;

    Guild* guild = player->GetGuild();
    if (!guild)
        return;

    // Must have guild info edit permissions (leader or officer with rights)
    if (!guild->HasAnyRankRight(GuildRankId(player->GetGuildRank()), GuildRankRights(GR_RIGHT_MODIFY_GUILD_INFO)))
        return;

    WorldPackets::ClubFinder::ResponsePostRecruitmentMessage response;

    ClubFinderPosting const* existingPosting = sClubFinderMgr->GetPostingByGuild(guild->GetId());
    if (existingPosting)
    {
        sClubFinderMgr->UpdatePosting(existingPosting->PostingID, packet.Description, packet.Playstyle,
            packet.Interests, packet.ClassMask, packet.MinLevel, packet.MaxLevel);
        response.Result = 0;
    }
    else
    {
        uint64 postingID = sClubFinderMgr->AddPosting(guild->GetId(), packet.Description, packet.Playstyle,
            packet.Interests, packet.ClassMask, packet.MinLevel, packet.MaxLevel);
        response.Result = postingID > 0 ? 0 : 1;
    }

    SendPacket(response.Write());
}

void WorldSession::HandleClubFinderRequestClubsList(WorldPackets::ClubFinder::RequestClubsList& packet)
{
    TC_LOG_DEBUG("clubfinder", "CMSG_CLUB_FINDER_REQUEST_CLUBS_LIST [{}]: Offset: {}, MaxResults: {}, SearchType: {}",
        GetPlayerInfo(), packet.Offset, packet.MaxResults, packet.SearchType);

    WorldPackets::ClubFinder::LookupClubPostingsList response;

    std::vector<ClubFinderPosting const*> postings = sClubFinderMgr->GetAllActivePostings();

    uint32 offset = packet.Offset;
    uint32 maxResults = packet.MaxResults > 0 ? packet.MaxResults : 100;
    uint32 count = 0;

    for (ClubFinderPosting const* posting : postings)
    {
        if (offset > 0)
        {
            --offset;
            continue;
        }

        if (count >= maxResults)
            break;

        Guild* guild = sGuildMgr->GetGuildById(posting->GuildID);
        if (!guild)
            continue;

        WorldPackets::ClubFinder::ClubPostingEntry entry;
        entry.PostingID = posting->PostingID;
        entry.ClubID = posting->GuildID;
        entry.GuildGUID = guild->GetGUID();
        entry.ApplicantCount = sClubFinderMgr->GetApplicantCount(posting->PostingID);
        entry.MaxApplicants = posting->MaxApplicants;
        entry.SlotsAvailable = posting->SlotsAvailable;
        entry.Interests = posting->Interests;
        entry.Timestamp = posting->Timestamp;
        entry.GuildName = guild->GetName();
        entry.Description = posting->Description;
        entry.RealmName = sRealmList->GetCurrentRealm()->Name;
        response.Entries.push_back(std::move(entry));
        ++count;
    }

    SendPacket(response.Write());
}

void WorldSession::HandleClubFinderRequestMembershipToClub(WorldPackets::ClubFinder::RequestMembershipToClub& packet)
{
    TC_LOG_DEBUG("clubfinder", "CMSG_CLUB_FINDER_REQUEST_MEMBERSHIP_TO_CLUB [{}]: PostingID: {}",
        GetPlayerInfo(), packet.PostingID);

    Player* player = GetPlayer();
    if (!player)
        return;

    ClubFinderPosting const* posting = sClubFinderMgr->GetPosting(packet.PostingID);
    if (!posting)
        return;
    std::vector<ClubFinderApplicant const*> existing = sClubFinderMgr->GetApplicationsByPlayer(player->GetGUID());
    for (ClubFinderApplicant const* applicant : existing)
    {
        if (applicant->PostingID == packet.PostingID && applicant->Status == 0)
            return;
    }

    sClubFinderMgr->AddApplicant(packet.PostingID, player->GetGUID(), packet.Comment);
}

void WorldSession::HandleClubFinderGetApplicantsList(WorldPackets::ClubFinder::GetApplicantsList& /*packet*/)
{
    TC_LOG_DEBUG("clubfinder", "CMSG_CLUB_FINDER_GET_APPLICANTS_LIST [{}]", GetPlayerInfo());

    Player* player = GetPlayer();
    if (!player)
        return;

    Guild* guild = player->GetGuild();
    if (!guild)
        return;

    ClubFinderPosting const* posting = sClubFinderMgr->GetPostingByGuild(guild->GetId());
    if (!posting)
        return;

    WorldPackets::ClubFinder::UpdateApplications response;

    std::vector<ClubFinderApplicant const*> applicants = sClubFinderMgr->GetApplicantsByPosting(posting->PostingID);
    for (ClubFinderApplicant const* applicant : applicants)
    {
        WorldPackets::ClubFinder::ClubApplicationEntry entry;
        entry.PostingID = applicant->PostingID;
        entry.ApplicantGUID = applicant->PlayerGUID;
        entry.Timestamp = applicant->Timestamp;
        entry.Status = applicant->Status;
        response.Entries.push_back(entry);
    }

    SendPacket(response.Write());
}

void WorldSession::HandleClubFinderRespondToApplicant(WorldPackets::ClubFinder::RespondToApplicant& packet)
{
    TC_LOG_DEBUG("clubfinder", "CMSG_CLUB_FINDER_RESPOND_TO_APPLICANT [{}]: PostingID: {}, Applicant: {}, Response: {}",
        GetPlayerInfo(), packet.PostingID, packet.ApplicantGUID.ToString(), packet.Response);

    Player* player = GetPlayer();
    if (!player)
        return;

    Guild* guild = player->GetGuild();
    if (!guild)
        return;

    // TODO: verify player has permission to manage applicants

    ClubFinderPosting const* posting = sClubFinderMgr->GetPostingByGuild(guild->GetId());
    if (!posting)
        return;

    std::vector<ClubFinderApplicant const*> applicants = sClubFinderMgr->GetApplicantsByPosting(posting->PostingID);
    for (ClubFinderApplicant const* applicant : applicants)
    {
        if (applicant->PlayerGUID == packet.ApplicantGUID)
        {
            // 0=accept, 1=decline (from packet); map to DB status: 1=Accepted, 2=Declined
            uint8 newStatus = packet.Response == 0 ? 1 : 2;
            sClubFinderMgr->SetApplicantStatus(applicant->ID, newStatus);

            // TODO: if accepted, invite the player to the guild
            break;
        }
    }
}

void WorldSession::HandleClubFinderWhisperApplicantRequest(WorldPackets::ClubFinder::WhisperApplicantRequest& packet)
{
    TC_LOG_DEBUG("clubfinder", "CMSG_CLUB_FINDER_WHISPER_APPLICANT_REQUEST [{}]: Applicant: {}",
        GetPlayerInfo(), packet.ApplicantGUID.ToString());

    // TODO: needs sniff - initiate whisper to applicant
    // For now this is a no-op; the client may handle whisper initiation locally
}
