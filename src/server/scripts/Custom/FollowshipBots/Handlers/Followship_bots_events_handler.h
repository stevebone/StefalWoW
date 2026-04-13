/*
 * This file is part of the Stefal WoW Project.
 * It is designed to work exclusively with the TrinityCore framework.
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
 * This code is provided for personal and educational use within the
 * Stefal WoW Project. It is not intended for commercial distribution,
 * resale, or any form of monetization.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

enum FSB_BOT_EVENTS
{
    FSB_EVENT_HIRED_MOUNT_AURA = 1,
    FSB_EVENT_HIRED_RESURRECT_TARGET,
    FSB_EVENT_HIRED_RESUME_FOLLOW,
    FSB_EVENT_HIRED_DESPAWN_TEMP_BOT,
    FSB_EVENT_HIRED_CHECK_MEMBER_DEATH,
    FSB_EVENT_HIRED_SPELL_RESURRECT_STATE,
    FSB_EVENT_HIRED_TIMED_CHATTER_REPLY,
    FSB_EVENT_HIRED_TIMED_DUMMY_EMOTE,
    FSB_EVENT_HIRED_EXPIRED,
    FSB_EVENT_HIRED_LEAVE,

    // Generic periodic checks for non / hired bots
    FSB_EVENT_GENERIC_MAINTENANCE,
    FSB_EVENT_GENERIC_TELEPORT_GRAVEYARD,
    FSB_EVENT_GENERIC_GRAVEYARD_RESSURECT,
    FSB_EVENT_GENERIC_CHECK_HIRED_TIME,

    FSB_EVENT_RANDOM_ACTION_SIT_BY_FIRE,
    FSB_EVENT_RANDOM_ACTION_MOVE_FIRE,
    FSB_EVENT_RANDOM_ACTION_FINISH,
};

namespace FSBEvents
{
    // Wrapper for scheduling a bot event directly from Creature*
    void ScheduleBotEvent(Creature* bot, uint32 eventId, std::chrono::milliseconds minTime, std::chrono::milliseconds maxTime = std::chrono::milliseconds(0));
    void ScheduleBotEventWithChatter(Creature* bot, uint32 eventId, std::chrono::milliseconds minTime, std::chrono::milliseconds maxTime, FSB_ReplyType replyType, std::string chatterReply, Unit* target);
}
