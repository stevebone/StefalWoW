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

#ifndef TRINITYCORE_CLUBFINDERPACKETS_H
#define TRINITYCORE_CLUBFINDERPACKETS_H

#include "Packet.h"
#include "ObjectGuid.h"
#include "PacketUtilities.h"

namespace WorldPackets
{
    namespace ClubFinder
    {
        class RequestPendingClubsList final : public ClientPacket
        {
        public:
            explicit RequestPendingClubsList(WorldPacket&& packet) : ClientPacket(CMSG_CLUB_FINDER_REQUEST_PENDING_CLUBS_LIST, std::move(packet)) { }

            void Read() override;

            uint8 SearchType = 0; // 0x60=guild, 0x20=community
        };

        class RequestClubsData final : public ClientPacket
        {
        public:
            explicit RequestClubsData(WorldPacket&& packet) : ClientPacket(CMSG_CLUB_FINDER_REQUEST_CLUBS_DATA, std::move(packet)) { }

            void Read() override;

            std::vector<uint32> PostingIDsLow;
            uint32 Type = 0;
            uint8 Flags = 0;
        };

        class RequestSubscribedClubPostingIDs final : public ClientPacket
        {
        public:
            explicit RequestSubscribedClubPostingIDs(WorldPacket&& packet) : ClientPacket(CMSG_CLUB_FINDER_REQUEST_SUBSCRIBED_CLUB_POSTING_IDS, std::move(packet)) { }

            void Read() override;

            std::vector<uint64> PostingIDs;
        };

        class ApplicationResponse final : public ClientPacket
        {
        public:
            explicit ApplicationResponse(WorldPacket&& packet) : ClientPacket(CMSG_CLUB_FINDER_APPLICATION_RESPONSE, std::move(packet)) { }

            void Read() override;

            // TODO: exact field layout needs additional sniff data
            uint64 PostingID = 0;
            uint32 ApplicationID = 0;
            uint16 Response = 0;
        };

        class ClubFinderPost final : public ClientPacket
        {
        public:
            explicit ClubFinderPost(WorldPacket&& packet) : ClientPacket(CMSG_CLUB_FINDER_POST, std::move(packet)) { }

            void Read() override;

            // TODO: needs sniff - guild leader posting a listing
            std::string Description;
            uint32 Playstyle = 0;
            uint32 Interests = 0;
            uint32 ClassMask = 0;
            uint8 MinLevel = 0;
            uint8 MaxLevel = 0;
        };

        class RequestClubsList final : public ClientPacket
        {
        public:
            explicit RequestClubsList(WorldPacket&& packet) : ClientPacket(CMSG_CLUB_FINDER_REQUEST_CLUBS_LIST, std::move(packet)) { }

            void Read() override;

            // TODO: needs sniff - paginated browse
            uint32 Offset = 0;
            uint32 MaxResults = 0;
            uint8 SearchType = 0;
        };

        class RequestMembershipToClub final : public ClientPacket
        {
        public:
            explicit RequestMembershipToClub(WorldPacket&& packet) : ClientPacket(CMSG_CLUB_FINDER_REQUEST_MEMBERSHIP_TO_CLUB, std::move(packet)) { }

            void Read() override;

            // TODO: needs sniff - player applies to guild
            uint64 PostingID = 0;
            std::string Comment;
        };

        class GetApplicantsList final : public ClientPacket
        {
        public:
            explicit GetApplicantsList(WorldPacket&& packet) : ClientPacket(CMSG_CLUB_FINDER_GET_APPLICANTS_LIST, std::move(packet)) { }

            void Read() override;

            // TODO: needs sniff - guild leader views applicants
        };

        class RespondToApplicant final : public ClientPacket
        {
        public:
            explicit RespondToApplicant(WorldPacket&& packet) : ClientPacket(CMSG_CLUB_FINDER_RESPOND_TO_APPLICANT, std::move(packet)) { }

            void Read() override;

            // TODO: needs sniff - accept/decline applicant
            uint64 PostingID = 0;
            ObjectGuid ApplicantGUID;
            uint8 Response = 0; // 0=accept, 1=decline
        };

        class WhisperApplicantRequest final : public ClientPacket
        {
        public:
            explicit WhisperApplicantRequest(WorldPacket&& packet) : ClientPacket(CMSG_CLUB_FINDER_WHISPER_APPLICANT_REQUEST, std::move(packet)) { }

            void Read() override;

            // TODO: needs sniff
            ObjectGuid ApplicantGUID;
        };

        struct ClubPostingIDEntry
        {
            uint64 PostingGUID = 0;
            uint64 ClubID = 0;
        };

        class GetClubPostingIDsResponse final : public ServerPacket
        {
        public:
            explicit GetClubPostingIDsResponse() : ServerPacket(SMSG_CLUB_FINDER_GET_CLUB_POSTING_IDS_RESPONSE, 20) { }

            WorldPacket const* Write() override;

            std::vector<ClubPostingIDEntry> Entries;
        };

        struct ClubPostingEntry
        {
            uint64 PostingID = 0;
            uint64 ClubID = 0;
            ObjectGuid GuildGUID;
            uint32 ApplicantCount = 0;
            uint32 MaxApplicants = 0xFFFFFFFF;
            uint32 SlotsAvailable = 0;
            uint32 Interests = 0;
            uint64 Timestamp = 0;
            std::string GuildName;
            std::string Description;
            std::string RealmName;
        };

        class LookupClubPostingsList final : public ServerPacket
        {
        public:
            explicit LookupClubPostingsList() : ServerPacket(SMSG_CLUB_FINDER_LOOKUP_CLUB_POSTINGS_LIST, 200) { }

            WorldPacket const* Write() override;

            std::vector<ClubPostingEntry> Entries;
        };

        struct ClubApplicationEntry
        {
            uint64 PostingID = 0;
            ObjectGuid ApplicantGUID;
            uint64 Timestamp = 0;
            uint8 Status = 0;
        };

        class ResponseCharacterApplicationList final : public ServerPacket
        {
        public:
            explicit ResponseCharacterApplicationList() : ServerPacket(SMSG_CLUB_FINDER_RESPONSE_CHARACTER_APPLICATION_LIST, 150) { }

            WorldPacket const* Write() override;

            std::vector<ClubApplicationEntry> Entries;
        };

        class UpdateApplications final : public ServerPacket
        {
        public:
            explicit UpdateApplications() : ServerPacket(SMSG_CLUB_FINDER_UPDATE_APPLICATIONS, 40) { }

            WorldPacket const* Write() override;

            std::vector<ClubApplicationEntry> Entries;
        };

        class ResponsePostRecruitmentMessage final : public ServerPacket
        {
        public:
            explicit ResponsePostRecruitmentMessage() : ServerPacket(SMSG_CLUB_FINDER_RESPONSE_POST_RECRUITMENT_MESSAGE, 4) { }

            WorldPacket const* Write() override;

            int32 Result = 0;
        };

        class ClubFinderErrorMessage final : public ServerPacket
        {
        public:
            explicit ClubFinderErrorMessage() : ServerPacket(SMSG_CLUB_FINDER_ERROR_MESSAGE, 4) { }

            WorldPacket const* Write() override;

            int32 Error = 0;
        };
    }
}

#endif // TRINITYCORE_CLUBFINDERPACKETS_H
