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

#include "SocialMgr.h"
#include "DatabaseEnv.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "RBAC.h"
#include "SocialPackets.h"
#include "World.h"
#include "WorldSession.h"

PlayerSocial::PlayerSocial() = default;
PlayerSocial::PlayerSocial(PlayerSocial const&) = default;
PlayerSocial::PlayerSocial(PlayerSocial&&) noexcept = default;
PlayerSocial& PlayerSocial::operator=(PlayerSocial const&) = default;
PlayerSocial& PlayerSocial::operator=(PlayerSocial&&) noexcept = default;
PlayerSocial::~PlayerSocial() = default;

uint32 PlayerSocial::GetNumberOfSocialsWithFlag(SocialFlag flag)
{
    uint32 counter = 0;
    for (PlayerSocialMap::const_iterator itr = _playerSocialMap.begin(); itr != _playerSocialMap.end(); ++itr)
        if ((itr->second.Flags & flag) != 0)
            ++counter;

    return counter;
}

bool PlayerSocial::AddToSocialList(ObjectGuid const& friendGuid, ObjectGuid const& accountGuid, SocialFlag flag)
{
    // check client limits
    if (GetNumberOfSocialsWithFlag(flag) >= (((flag & SOCIAL_FLAG_FRIEND) != 0) ? SOCIALMGR_FRIEND_LIMIT : SOCIALMGR_IGNORE_LIMIT))
        return false;

    PlayerSocialMap::iterator itr = _playerSocialMap.find(friendGuid);
    if (itr != _playerSocialMap.end())
    {
        itr->second.Flags |= flag;
        itr->second.WowAccountGuid = accountGuid;

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHARACTER_SOCIAL_FLAGS);

        stmt->setUInt8(0, itr->second.Flags);
        stmt->setUInt64(1, GetPlayerGUID().GetCounter());
        stmt->setUInt64(2, friendGuid.GetCounter());

        CharacterDatabase.Execute(stmt);
    }
    else
    {
        itr = _playerSocialMap.emplace(std::piecewise_construct, std::forward_as_tuple(friendGuid), std::forward_as_tuple()).first;

        itr->second.Flags |= flag;
        itr->second.WowAccountGuid = accountGuid;

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER_SOCIAL);

        stmt->setUInt64(0, GetPlayerGUID().GetCounter());
        stmt->setUInt64(1, friendGuid.GetCounter());
        stmt->setUInt8(2, flag);

        CharacterDatabase.Execute(stmt);
    }

    if (flag & SOCIAL_FLAG_IGNORED)
        _ignoredAccounts.insert(accountGuid);

    return true;
}

void PlayerSocial::RemoveFromSocialList(ObjectGuid const& friendGuid, SocialFlag flag)
{
    PlayerSocialMap::iterator itr = _playerSocialMap.find(friendGuid);
    if (itr == _playerSocialMap.end())
        return;

    itr->second.Flags &= ~flag;

    if (!itr->second.Flags)
    {
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHARACTER_SOCIAL);

        stmt->setUInt64(0, GetPlayerGUID().GetCounter());
        stmt->setUInt64(1, friendGuid.GetCounter());

        CharacterDatabase.Execute(stmt);

        ObjectGuid accountGuid = itr->second.WowAccountGuid;

        _playerSocialMap.erase(itr);

        if (flag & SOCIAL_FLAG_IGNORED)
        {
            auto otherIgnoreForAccount = std::find_if(_playerSocialMap.begin(), _playerSocialMap.end(), [&](PlayerSocialMap::value_type const& social)
            {
                return social.second.Flags & SOCIAL_FLAG_IGNORED && social.second.WowAccountGuid == accountGuid;
            });

            if (otherIgnoreForAccount == _playerSocialMap.end())
                _ignoredAccounts.erase(accountGuid);
        }
    }
    else
    {
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHARACTER_SOCIAL_FLAGS);

        stmt->setUInt8(0, itr->second.Flags);
        stmt->setUInt64(1, GetPlayerGUID().GetCounter());
        stmt->setUInt64(2, friendGuid.GetCounter());

        CharacterDatabase.Execute(stmt);
    }
}

void PlayerSocial::SetFriendNote(ObjectGuid const& friendGuid, std::string const& note)
{
    PlayerSocialMap::iterator itr = _playerSocialMap.find(friendGuid);
    if (itr == _playerSocialMap.end())                  // not exist
        return;

    itr->second.Note = note;
    utf8truncate(itr->second.Note, 48);                 // DB and client size limitation

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHARACTER_SOCIAL_NOTE);

    stmt->setString(0, itr->second.Note);
    stmt->setUInt64(1, GetPlayerGUID().GetCounter());
    stmt->setUInt64(2, friendGuid.GetCounter());

    CharacterDatabase.Execute(stmt);
}

void PlayerSocial::SendSocialList(Player* player, uint32 flags)
{
    ASSERT(player);

    uint32 friendsCount = 0;
    uint32 ignoredCount = 0;

    WorldPackets::Social::ContactList contactList;
    contactList.Flags = flags;                              // 0x1 = Friendlist update. 0x2 = Ignorelist update. 0x4 = Mutelist update.

    for (PlayerSocialMap::value_type& v : _playerSocialMap)
    {
        uint8 contactFlags = v.second.Flags;
        if (!(contactFlags & flags))
            continue;

        // Check client limit for friends list
        if (contactFlags & SOCIAL_FLAG_FRIEND)
            if (++friendsCount > SOCIALMGR_FRIEND_LIMIT)
                continue;

        // Check client limit for ignore list
        if (contactFlags & SOCIAL_FLAG_IGNORED)
            if (++ignoredCount > SOCIALMGR_IGNORE_LIMIT)
                continue;

        SocialMgr::GetFriendInfo(player, v.first, v.second);

        WorldPackets::Social::ContactInfo& contact = contactList.Contacts.emplace_back();
        contact.Guid = v.first;
        contact.WowAccountGuid = v.second.WowAccountGuid;
        contact.VirtualRealmAddr = GetVirtualRealmAddress();
        contact.NativeRealmAddr = GetVirtualRealmAddress();
        contact.TypeFlags = v.second.Flags;
        contact.Notes = v.second.Note;
        contact.Status = v.second.Status;
        contact.AreaID = v.second.Area;
        contact.Level = v.second.Level;
        contact.ClassID = v.second.Class;
    }

    player->SendDirectMessage(contactList.Write());
}

bool PlayerSocial::_HasContact(ObjectGuid const& guid, SocialFlag flags)
{
    PlayerSocialMap::const_iterator itr = _playerSocialMap.find(guid);
    if (itr != _playerSocialMap.end())
        return (itr->second.Flags & flags) != 0;

    return false;
}

bool PlayerSocial::HasFriend(ObjectGuid const& friendGuid)
{
    return _HasContact(friendGuid, SOCIAL_FLAG_FRIEND);
}

bool PlayerSocial::HasIgnore(ObjectGuid const& ignoreGuid, ObjectGuid const& ignoreAccountGuid)
{
    return _HasContact(ignoreGuid, SOCIAL_FLAG_IGNORED) || _ignoredAccounts.find(ignoreAccountGuid) != _ignoredAccounts.end();
}

SocialMgr::SocialMgr() = default;
SocialMgr::~SocialMgr() = default;

SocialMgr* SocialMgr::instance()
{
    static SocialMgr instance;
    return &instance;
}

void SocialMgr::GetFriendInfo(Player* player, ObjectGuid const& friendGUID, FriendInfo& friendInfo)
{
    if (!player)
        return;

    friendInfo.Status = FRIEND_STATUS_OFFLINE;
    friendInfo.Area = 0;
    friendInfo.Level = 0;
    friendInfo.Class = 0;

    Player* target = ObjectAccessor::FindPlayer(friendGUID);
    if (!target)
        return;

    PlayerSocial::PlayerSocialMap::iterator itr = player->GetSocial()->_playerSocialMap.find(friendGUID);
    if (itr != player->GetSocial()->_playerSocialMap.end())
        friendInfo.Note = itr->second.Note;

    // PLAYER see his team only and PLAYER can't see MODERATOR, GAME MASTER, ADMINISTRATOR characters
    // MODERATOR, GAME MASTER, ADMINISTRATOR can see all

    if (!player->GetSession()->HasPermission(rbac::RBAC_PERM_WHO_SEE_ALL_SEC_LEVELS) &&
        target->GetSession()->GetSecurity() > AccountTypes(sWorld->getIntConfig(CONFIG_GM_LEVEL_IN_WHO_LIST)))
        return;

    // player can see member of other team only if CONFIG_ALLOW_TWO_SIDE_WHO_LIST
    if (target->GetTeam() != player->GetTeam() && !player->GetSession()->HasPermission(rbac::RBAC_PERM_TWO_SIDE_WHO_LIST))
        return;

    if (target->IsVisibleGloballyFor(player))
    {
        if (target->isDND())
            friendInfo.Status = FRIEND_STATUS_DND;
        else if (target->isAFK())
            friendInfo.Status = FRIEND_STATUS_AFK;
        else
        {
            friendInfo.Status = FRIEND_STATUS_ONLINE;

            if (target->GetSession()->GetRecruiterId() == player->GetSession()->GetAccountId() || target->GetSession()->GetAccountId() == player->GetSession()->GetRecruiterId())
                friendInfo.Status = FriendStatus(uint32(friendInfo.Status) | FRIEND_STATUS_RAF);
        }

        friendInfo.Area = target->GetZoneId();
        friendInfo.Level = target->GetLevel();
        friendInfo.Class = target->GetClass();
    }
}

void SocialMgr::SendFriendStatus(Player* player, FriendsResult result, ObjectGuid const& friendGuid, bool broadcast /*= false*/)
{
    FriendInfo fi;
    GetFriendInfo(player, friendGuid, fi);

    WorldPackets::Social::FriendStatus friendStatus;
    friendStatus.VirtualRealmAddress = GetVirtualRealmAddress();
    friendStatus.Notes = fi.Note;
    friendStatus.ClassID = fi.Class;
    friendStatus.Status = fi.Status;
    friendStatus.Guid = friendGuid;
    friendStatus.WowAccountGuid = fi.WowAccountGuid;
    friendStatus.Level = fi.Level;
    friendStatus.AreaID = fi.Area;
    friendStatus.FriendResult = result;

    if (broadcast)
        BroadcastToFriendListers(player, friendStatus.Write());
    else
        player->SendDirectMessage(friendStatus.Write());
}

void SocialMgr::BroadcastToFriendListers(Player* player, WorldPacket const* packet)
{
    ASSERT(player);

    AccountTypes gmSecLevel = AccountTypes(sWorld->getIntConfig(CONFIG_GM_LEVEL_IN_WHO_LIST));
    for (SocialMap::const_iterator itr = _socialMap.begin(); itr != _socialMap.end(); ++itr)
    {
        PlayerSocial::PlayerSocialMap::const_iterator itr2 = itr->second._playerSocialMap.find(player->GetGUID());
        if (itr2 != itr->second._playerSocialMap.end() && (itr2->second.Flags & SOCIAL_FLAG_FRIEND) != 0)
        {
            Player* target = ObjectAccessor::FindPlayer(itr->first);
            if (!target)
                continue;

            WorldSession* session = target->GetSession();
            if (!session->HasPermission(rbac::RBAC_PERM_WHO_SEE_ALL_SEC_LEVELS) && player->GetSession()->GetSecurity() > gmSecLevel)
                continue;

            if (target->GetTeam() != player->GetTeam() && !session->HasPermission(rbac::RBAC_PERM_TWO_SIDE_WHO_LIST))
                continue;

            if (player->IsVisibleGloballyFor(target))
                session->SendPacket(packet);
        }
    }
}

PlayerSocial* SocialMgr::LoadFromDB(PreparedQueryResult result, ObjectGuid const& guid)
{
    PlayerSocial* social = &_socialMap[guid];
    social->SetPlayerGUID(guid);

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();

            ObjectGuid friendGuid = ObjectGuid::Create<HighGuid::Player>(fields[0].GetUInt64());
            ObjectGuid friendAccountGuid = ObjectGuid::Create<HighGuid::WowAccount>(uint64(fields[1].GetUInt32()));

            uint8 flag = fields[2].GetUInt8();
            social->_playerSocialMap[friendGuid] = FriendInfo(friendAccountGuid, flag, fields[3].GetString());
            if (flag & SOCIAL_FLAG_IGNORED)
                social->_ignoredAccounts.insert(friendAccountGuid);
        }
        while (result->NextRow());
    }

    return social;
}
