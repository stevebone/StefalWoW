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

#include "ClubFinderPackets.h"
#include "PacketOperators.h"

namespace WorldPackets::ClubFinder
{
// --- CMSG Read implementations ---

void RequestPendingClubsList::Read()
{
    _worldPacket >> SearchType;
}

void RequestClubsData::Read()
{
    uint32 count;
    _worldPacket >> count;
    _worldPacket >> Type;

    PostingIDsLow.resize(count);
    for (uint32 i = 0; i < count; ++i)
        _worldPacket >> PostingIDsLow[i];

    _worldPacket >> Flags;
}

void RequestSubscribedClubPostingIDs::Read()
{
    uint32 count;
    _worldPacket >> count;

    PostingIDs.resize(count);
    for (uint32 i = 0; i < count; ++i)
        _worldPacket >> PostingIDs[i];
}

void ApplicationResponse::Read()
{
    // TODO: exact field layout needs additional sniff data - 14 bytes total
    _worldPacket >> PostingID;
    _worldPacket >> ApplicationID;
    _worldPacket >> Response;
}

void ClubFinderPost::Read()
{
    // TODO: needs sniff — guild leader posting a listing
    _worldPacket >> SizedString::BitsSize<11>(Description);
    _worldPacket >> Bits<32>(Playstyle);
    _worldPacket >> Bits<32>(Interests);
    _worldPacket >> Bits<32>(ClassMask);
    _worldPacket.FlushBits();

    _worldPacket >> MinLevel;
    _worldPacket >> MaxLevel;
    _worldPacket >> SizedString::Data(Description);
}

void RequestClubsList::Read()
{
    // TODO: needs sniff — paginated browse
    _worldPacket >> Offset;
    _worldPacket >> MaxResults;
    _worldPacket >> SearchType;
}

void RequestMembershipToClub::Read()
{
    // TODO: needs sniff — player applies to guild
    _worldPacket >> PostingID;
    _worldPacket >> SizedString::BitsSize<11>(Comment);
    _worldPacket.FlushBits();
    _worldPacket >> SizedString::Data(Comment);
}

void GetApplicantsList::Read()
{
    // TODO: needs sniff — guild leader views applicants
}

void RespondToApplicant::Read()
{
    // TODO: needs sniff — accept/decline applicant
    _worldPacket >> PostingID;
    _worldPacket >> ApplicantGUID;
    _worldPacket >> Response;
}

void WhisperApplicantRequest::Read()
{
    // TODO: needs sniff
    _worldPacket >> ApplicantGUID;
}

// --- SMSG Write implementations ---

WorldPacket const* GetClubPostingIDsResponse::Write()
{
    _worldPacket << Size<uint32>(Entries);
    for (ClubPostingIDEntry const& entry : Entries)
    {
        _worldPacket << uint64(entry.PostingGUID);
        _worldPacket << uint64(entry.ClubID);
    }

    return &_worldPacket;
}

WorldPacket const* LookupClubPostingsList::Write()
{
    _worldPacket << Size<uint32>(Entries);
    for (ClubPostingEntry const& entry : Entries)
    {
        _worldPacket << SizedString::BitsSize<8>(entry.GuildName);
        _worldPacket << SizedString::BitsSize<11>(entry.Description);
        _worldPacket << SizedString::BitsSize<8>(entry.RealmName);
        _worldPacket.FlushBits();

        _worldPacket << entry.GuildGUID;
        _worldPacket << uint64(entry.PostingID);
        _worldPacket << uint64(entry.ClubID);
        _worldPacket << uint32(entry.ApplicantCount);
        _worldPacket << uint32(entry.MaxApplicants);
        _worldPacket << uint32(entry.SlotsAvailable);
        _worldPacket << uint32(entry.Interests);
        _worldPacket << uint64(entry.Timestamp);

        _worldPacket << SizedString::Data(entry.GuildName);
        _worldPacket << SizedString::Data(entry.Description);
        _worldPacket << SizedString::Data(entry.RealmName);
    }

    return &_worldPacket;
}

WorldPacket const* ResponseCharacterApplicationList::Write()
{
    _worldPacket << Size<uint32>(Entries);
    for (ClubApplicationEntry const& entry : Entries)
    {
        _worldPacket << uint8(entry.Status);
        _worldPacket << uint64(entry.PostingID);
        _worldPacket << entry.ApplicantGUID;
        _worldPacket << uint64(entry.Timestamp);
    }

    return &_worldPacket;
}

WorldPacket const* UpdateApplications::Write()
{
    _worldPacket << Size<uint32>(Entries);
    for (ClubApplicationEntry const& entry : Entries)
    {
        _worldPacket << uint8(entry.Status);
        _worldPacket << uint64(entry.PostingID);
        _worldPacket << entry.ApplicantGUID;
        _worldPacket << uint64(entry.Timestamp);
    }

    return &_worldPacket;
}

WorldPacket const* ResponsePostRecruitmentMessage::Write()
{
    _worldPacket << int32(Result);

    return &_worldPacket;
}

WorldPacket const* ClubFinderErrorMessage::Write()
{
    _worldPacket << int32(Error);

    return &_worldPacket;
}
}
