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

enum FSB_ChatterCategory
{
    chatter_none,
    targetKilled,
    targetKilledHired,
    botDismissed,
    botAcknowledge,
    botFollow,
    botStay,
    botMemberDied,
    botRevived,
    botDeathHired,
    botDeath,
    botHiredPermanent,
    botRevivedTarget,
    botHealTarget,
    botHealSelf,
    botCombatMana,
    botCombatHealth,
    botCombatSpell,
    botBuffSelf,
    botBuffTarget,
    botOOCRecovery,
    botOOCRecoveryHired,
    botHire,

    emote_oom,
    emote_heal,
    emote_help,
    emote_whistle,
    emote_sleep,
    emote_sigh,

    emote_kiss,
    emote_flirt,
    emote_joke,
    emote_talk,
    emote_cooking,

    whisper_afk,
    

};

struct FSBChatterReplyEntry
{
    FSB_ChatterCategory category;
    FSB_ChatterType chatterType;
    std::vector<std::string> lines;
};

enum FSB_ChatterSource
{
    None,
    Bot,
    Target
};

namespace FSBChatter
{
    std::string GetRandomReply(Creature* bot, Unit* target, FSB_ChatterCategory category, FSB_ChatterType chatterType, uint32 spellId = 0, uint8 duration = 0);
    void DemandTimedReply(Creature* bot, Unit* target, FSB_ChatterCategory category, FSB_ReplyType replyType, FSB_ChatterSource chatterSource);
    void DemandBotChatter(Creature* bot, Unit* target, FSB_ChatterCategory category, FSB_ReplyType replyType = FSB_ReplyType::Say, FSB_ChatterSource chatterSource = FSB_ChatterSource::None, uint32 spellId = 0, uint16 duration = 0);

    void ReplaceAll(std::string& text, const std::string& from, const std::string& to);
}
