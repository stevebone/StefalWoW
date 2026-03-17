/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TYPED_PACKET_HOOKS_H
#define TYPED_PACKET_HOOKS_H

class WorldSession;
class Player;
class ObjectGuid;
class Group;

namespace Playerbot
{

/**
 * @brief Typed packet hook functions
 * These are called from core template implementations to forward typed packets to the sniffer
 */
class TypedPacketHooks
{
public:
    /**
     * Hook for WorldSession::SendPacket template
     */
    template<typename PacketType>
    static void OnWorldSessionSendPacket(WorldSession* session, PacketType const& packet);

    /**
     * Hook for Group::BroadcastPacket template
     */
    template<typename PacketType>
    static void OnGroupBroadcastPacket(Group const* group, PacketType const& packet, bool ignorePlayersInBGRaid, int groupFilter, ObjectGuid ignoredPlayer);
};

} // namespace Playerbot

#endif // TYPED_PACKET_HOOKS_H
