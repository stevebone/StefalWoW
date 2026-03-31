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

#include "ClubFinderMgr.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "Log.h"
#include "ObjectGuid.h"
#include "Timer.h"
#include "Util.h"

ClubFinderMgr::ClubFinderMgr() = default;
ClubFinderMgr::~ClubFinderMgr() = default;

ClubFinderMgr* ClubFinderMgr::instance()
{
    static ClubFinderMgr instance;
    return &instance;
}

void ClubFinderMgr::LoadFromDB()
{
    uint32 oldMSTime = getMSTime();

    _postings.clear();
    _applicants.clear();

    PreparedQueryResult result = CharacterDatabase.Query(CharacterDatabase.GetPreparedStatement(CHAR_SEL_CLUB_FINDER_POSTS));
    if (result)
    {
        uint32 count = 0;
        do
        {
            Field* fields = result->Fetch();

            ClubFinderPosting posting;
            posting.PostingID       = fields[0].GetUInt64();
            posting.GuildID         = fields[1].GetUInt64();
            posting.Description     = fields[2].GetString();
            posting.Playstyle       = fields[3].GetUInt32();
            posting.Interests       = fields[4].GetUInt32();
            posting.SpecIDs         = fields[5].GetString();
            posting.ClassMask       = fields[6].GetUInt32();
            posting.MinLevel        = fields[7].GetUInt8();
            posting.MaxLevel        = fields[8].GetUInt8();
            posting.SlotsAvailable  = fields[9].GetUInt32();
            posting.MaxApplicants   = fields[10].GetUInt32();
            posting.Language        = fields[11].GetUInt32();
            posting.Status          = fields[12].GetUInt8();
            posting.Timestamp       = fields[13].GetUInt32();

            _postings[posting.PostingID] = std::move(posting);
            ++count;
        } while (result->NextRow());

        TC_LOG_INFO("server.loading", ">> Loaded {} club finder postings in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    }
    else
        TC_LOG_INFO("server.loading", ">> Loaded 0 club finder postings");

    oldMSTime = getMSTime();

    PreparedQueryResult result2 = CharacterDatabase.Query(CharacterDatabase.GetPreparedStatement(CHAR_SEL_CLUB_FINDER_APPLICANTS));
    if (result2)
    {
        uint32 count = 0;
        do
        {
            Field* fields = result2->Fetch();

            ClubFinderApplicant applicant;
            applicant.ID          = fields[0].GetUInt64();
            applicant.PostingID   = fields[1].GetUInt64();
            applicant.PlayerGUID  = ObjectGuid::Create<HighGuid::Player>(fields[2].GetUInt64());
            applicant.Status      = fields[3].GetUInt8();
            applicant.Comment     = fields[4].GetString();
            applicant.Timestamp   = fields[5].GetUInt32();

            _applicants[applicant.PostingID].push_back(std::move(applicant));
            ++count;
        } while (result2->NextRow());

        TC_LOG_INFO("server.loading", ">> Loaded {} club finder applicants in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    }
    else
        TC_LOG_INFO("server.loading", ">> Loaded 0 club finder applicants");
}

ClubFinderPosting const* ClubFinderMgr::GetPosting(uint64 postingID) const
{
    auto itr = _postings.find(postingID);
    return itr != _postings.end() ? &itr->second : nullptr;
}

ClubFinderPosting const* ClubFinderMgr::GetPostingByGuild(uint64 guildID) const
{
    for (auto const& [id, posting] : _postings)
        if (posting.GuildID == guildID)
            return &posting;

    return nullptr;
}

std::vector<ClubFinderPosting const*> ClubFinderMgr::GetAllActivePostings() const
{
    std::vector<ClubFinderPosting const*> result;
    for (auto const& [id, posting] : _postings)
        if (posting.Status == 0)
            result.push_back(&posting);

    return result;
}

uint64 ClubFinderMgr::AddPosting(uint64 guildID, std::string description, uint32 playstyle, uint32 interests, uint32 classMask, uint8 minLevel, uint8 maxLevel)
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CLUB_FINDER_POST);
    stmt->setUInt64(0, guildID);
    stmt->setString(1, description);
    stmt->setUInt32(2, playstyle);
    stmt->setUInt32(3, interests);
    stmt->setUInt32(4, classMask);
    stmt->setUInt8(5, minLevel);
    stmt->setUInt8(6, maxLevel);
    stmt->setUInt32(7, uint32(GameTime::GetGameTime()));
    CharacterDatabase.Execute(stmt);

    // TODO: retrieve auto-increment ID properly; for now reload from DB
    // In a production implementation, use DirectExecute + LastInsertId
    LoadFromDB();

    if (ClubFinderPosting const* posting = GetPostingByGuild(guildID))
        return posting->PostingID;

    return 0;
}

void ClubFinderMgr::UpdatePosting(uint64 postingID, std::string description, uint32 playstyle, uint32 interests, uint32 classMask, uint8 minLevel, uint8 maxLevel)
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CLUB_FINDER_POST);
    stmt->setString(0, description);
    stmt->setUInt32(1, playstyle);
    stmt->setUInt32(2, interests);
    stmt->setUInt32(3, classMask);
    stmt->setUInt8(4, minLevel);
    stmt->setUInt8(5, maxLevel);
    stmt->setUInt64(6, postingID);
    CharacterDatabase.Execute(stmt);

    auto itr = _postings.find(postingID);
    if (itr != _postings.end())
    {
        itr->second.Description = std::move(description);
        itr->second.Playstyle = playstyle;
        itr->second.Interests = interests;
        itr->second.ClassMask = classMask;
        itr->second.MinLevel = minLevel;
        itr->second.MaxLevel = maxLevel;
    }
}

void ClubFinderMgr::RemovePosting(uint64 postingID)
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CLUB_FINDER_POST);
    stmt->setUInt64(0, postingID);
    CharacterDatabase.Execute(stmt);

    _postings.erase(postingID);
    RemoveApplicantsByPosting(postingID);
}

void ClubFinderMgr::RemovePostingByGuild(uint64 guildID)
{
    for (auto itr = _postings.begin(); itr != _postings.end(); ++itr)
    {
        if (itr->second.GuildID == guildID)
        {
            uint64 postingID = itr->first;

            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CLUB_FINDER_POST);
            stmt->setUInt64(0, postingID);
            CharacterDatabase.Execute(stmt);

            RemoveApplicantsByPosting(postingID);
            _postings.erase(itr);
            return;
        }
    }
}

std::vector<ClubFinderApplicant const*> ClubFinderMgr::GetApplicantsByPosting(uint64 postingID) const
{
    std::vector<ClubFinderApplicant const*> result;
    auto itr = _applicants.find(postingID);
    if (itr != _applicants.end())
        for (auto const& applicant : itr->second)
            result.push_back(&applicant);

    return result;
}

std::vector<ClubFinderApplicant const*> ClubFinderMgr::GetApplicationsByPlayer(ObjectGuid playerGUID) const
{
    std::vector<ClubFinderApplicant const*> result;
    for (auto const& [postingID, applicants] : _applicants)
        for (auto const& applicant : applicants)
            if (applicant.PlayerGUID == playerGUID)
                result.push_back(&applicant);

    return result;
}

uint32 ClubFinderMgr::GetApplicantCount(uint64 postingID) const
{
    auto itr = _applicants.find(postingID);
    return itr != _applicants.end() ? uint32(itr->second.size()) : 0;
}

void ClubFinderMgr::AddApplicant(uint64 postingID, ObjectGuid playerGUID, std::string comment)
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CLUB_FINDER_APPLICANT);
    stmt->setUInt64(0, postingID);
    stmt->setUInt64(1, playerGUID.GetCounter());
    stmt->setString(2, comment);
    stmt->setUInt32(3, uint32(GameTime::GetGameTime()));
    CharacterDatabase.Execute(stmt);

    ClubFinderApplicant applicant;
    applicant.PostingID = postingID;
    applicant.PlayerGUID = playerGUID;
    applicant.Comment = std::move(comment);
    applicant.Timestamp = uint32(GameTime::GetGameTime());
    _applicants[postingID].push_back(std::move(applicant));
}

void ClubFinderMgr::SetApplicantStatus(uint64 applicantID, uint8 status)
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CLUB_FINDER_APPLICANT_STATUS);
    stmt->setUInt8(0, status);
    stmt->setUInt64(1, applicantID);
    CharacterDatabase.Execute(stmt);

    for (auto& [postingID, applicants] : _applicants)
    {
        for (auto& applicant : applicants)
        {
            if (applicant.ID == applicantID)
            {
                applicant.Status = status;
                return;
            }
        }
    }
}

void ClubFinderMgr::RemoveApplicant(uint64 applicantID)
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CLUB_FINDER_APPLICANT);
    stmt->setUInt64(0, applicantID);
    CharacterDatabase.Execute(stmt);

    for (auto& [postingID, applicants] : _applicants)
    {
        for (auto itr = applicants.begin(); itr != applicants.end(); ++itr)
        {
            if (itr->ID == applicantID)
            {
                applicants.erase(itr);
                return;
            }
        }
    }
}

void ClubFinderMgr::RemoveApplicantsByPosting(uint64 postingID)
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CLUB_FINDER_APPLICANTS_BY_POSTING);
    stmt->setUInt64(0, postingID);
    CharacterDatabase.Execute(stmt);

    _applicants.erase(postingID);
}
