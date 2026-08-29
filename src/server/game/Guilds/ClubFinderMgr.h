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

#ifndef TRINITYCORE_CLUBFINDERMGR_H
#define TRINITYCORE_CLUBFINDERMGR_H

#include "Define.h"
#include "ObjectGuid.h"
#include <string>
#include <unordered_map>
#include <vector>

struct ClubFinderPosting
{
    uint64 PostingID = 0;
    uint64 GuildID = 0;
    std::string Description;
    uint32 Playstyle = 0;
    uint32 Interests = 0;
    std::string SpecIDs; // comma-separated
    uint32 ClassMask = 0;
    uint8 MinLevel = 10;
    uint8 MaxLevel = 80;
    uint32 SlotsAvailable = 0;
    uint32 MaxApplicants = 0xFFFFFFFF;
    uint32 Language = 0;
    uint8 Status = 0;
    uint32 Timestamp = 0;
};

struct ClubFinderApplicant
{
    uint64 ID = 0;
    uint64 PostingID = 0;
    ObjectGuid PlayerGUID;
    uint8 Status = 0; // 0=Pending, 1=Accepted, 2=Declined
    std::string Comment;
    uint32 Timestamp = 0;
};

class TC_GAME_API ClubFinderMgr
{
private:
    ClubFinderMgr();
    ~ClubFinderMgr();

public:
    ClubFinderMgr(ClubFinderMgr const&) = delete;
    ClubFinderMgr(ClubFinderMgr&&) = delete;
    ClubFinderMgr& operator=(ClubFinderMgr const&) = delete;
    ClubFinderMgr& operator=(ClubFinderMgr&&) = delete;

    static ClubFinderMgr* instance();

    void LoadFromDB();

    // Posts
    ClubFinderPosting const* GetPosting(uint64 postingID) const;
    ClubFinderPosting const* GetPostingByGuild(uint64 guildID) const;
    std::vector<ClubFinderPosting const*> GetAllActivePostings() const;
    uint64 AddPosting(uint64 guildID, std::string description, uint32 playstyle, uint32 interests, uint32 classMask, uint8 minLevel, uint8 maxLevel);
    void UpdatePosting(uint64 postingID, std::string description, uint32 playstyle, uint32 interests, uint32 classMask, uint8 minLevel, uint8 maxLevel);
    void RemovePosting(uint64 postingID);
    void RemovePostingByGuild(uint64 guildID);

    // Applicants
    std::vector<ClubFinderApplicant const*> GetApplicantsByPosting(uint64 postingID) const;
    std::vector<ClubFinderApplicant const*> GetApplicationsByPlayer(ObjectGuid playerGUID) const;
    uint32 GetApplicantCount(uint64 postingID) const;
    void AddApplicant(uint64 postingID, ObjectGuid playerGUID, std::string comment);
    void SetApplicantStatus(uint64 applicantID, uint8 status);
    void RemoveApplicant(uint64 applicantID);
    void RemoveApplicantsByPosting(uint64 postingID);

private:
    std::unordered_map<uint64, ClubFinderPosting> _postings;       // postingID -> posting
    std::unordered_map<uint64, std::vector<ClubFinderApplicant>> _applicants; // postingID -> applicants
};

#define sClubFinderMgr ClubFinderMgr::instance()

#endif // TRINITYCORE_CLUBFINDERMGR_H
