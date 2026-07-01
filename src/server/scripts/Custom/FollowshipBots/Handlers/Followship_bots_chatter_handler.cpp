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

#include "Log.h"

#include "DB2Stores.h"

#include "Followship_bots_config.h"
#include "Followship_bots_mgr.h"

#include "AI/Followship_bots_ai_base.h"

#include "Followship_bots_chatter_handler.h"
#include "Followship_bots_events_handler.h"

std::unordered_map<uint32, std::vector<FSBChatterDBLine>> FSBChatter::BotChatterLinesMap;
std::vector<FSBEmoteTextEntry> FSBChatter::FSBEmoteTextTable =
{
    {
        FSB_ChatterEmotes::emote_kiss,
        {
            "{bot} blows a kiss to {target}.",
            "{bot} sends a warm kiss toward {target}.",
            "{bot} winks and blows a kiss at {target}.",
        }
    },
    {
        FSB_ChatterEmotes::emote_flirt,
        {
            "{bot} gives {target} a playful wink.",
            "{bot} flashes {target} a charming smile.",
            "{bot} tilts their head and gives {target} a flirty grin.",
            "{bot} shoots {target} a mischievous look.",
            "{bot} gives {target} a slow, teasing wink.",
            "{bot} smirks softly in {target}'s direction.",
            "{bot} gives {target} a warm, inviting smile.",
            "{bot} glances at {target} with a hint of playful interest.",
            "{bot} offers {target} a coy, knowing smile.",
            "{bot} gives {target} a subtle but unmistakably flirty look."
        }
    },
    {
        FSB_ChatterEmotes::emote_joke,
        {
            "{bot} tells a joke.",
            "{bot} tells a funny quote.",
            "{bot} whispers a small joke.",
            "{bot} makes a funny face and jokes.",
            "{bot} tells a joke to themselves.",
            "{bot} tells a funny story.",
            "{bot} recollects a funny event."
        }
    },
    {
        FSB_ChatterEmotes::emote_whistle,
        {
            "{bot} whistles idly.",
            "{bot} lets out a long, bored whistle.",
            "{bot} whistles a wandering little tune.",
            "{bot} glances around and whistles softly.",
            "{bot} taps their foot and whistles impatiently.",
            "{bot} hums a tune before breaking into a whistle.",
            "{bot} whistles to pass the time.",
            "{bot} looks around, clearly bored, and whistles.",
            "{bot} gives a casual whistle into the air.",
            "{bot} whistles while waiting for something to happen."
        }
    },
    {
        FSB_ChatterEmotes::emote_sigh,
        {
            "{bot} lets out a long, weary sigh.",
            "{bot} sighs softly, looking a bit drained.",
            "{bot} exhales heavily, clearly tired of it all.",
            "{bot} sighs and stares off into the distance.",
            "{bot} releases a deep, frustrated sigh.",
            "{bot} sighs as if carrying the weight of the world.",
            "{bot} takes a slow breath and sighs quietly.",
            "{bot} sighs, sounding resigned.",
            "{bot} gives a tired sigh and shakes their head.",
            "{bot} sighs, clearly not impressed."
        }
    },
    {
        FSB_ChatterEmotes::emote_sleep,
        {
            "{bot} yawns loudly.",
            "{bot} lays down for a nap.",
            "{bot} feels sleepy.",
            "{bot} goes for a power nap.",
            "{bot} uses the ground as a bed.",
            "{bot} lays down to rest.",
            "{bot} takes a quick rest."
        }
    },
};

std::vector<FSBChatterReplyEntry> FSBReplyTable =
{
    {
        FSB_ChatterCategory::emote_oom, FSB_ChatterType::None,
        {
            "I'm out of mana!",
            "No mana left - I need a moment!",
            "I'm dry here, can't cast anything!",
            "Out of mana, someone cover me!",
            "I need mana, now!",
            "I'm tapped out - can't help much!",
            "No mana! I'm useless like this!",
            "I'm empty - need time to recover!",
            "I can't cast, I'm out!",
            "Mana's gone - watch my back!",
            "I'm running on fumes here!",
            "I've got nothing left to cast!",
            "I'm drained - need a breather!",
            "I'm completely out of mana!",
            "I can't keep this up without mana!",
            "Mana's gone - someone take over!",
            "I'm dry! Need mana badly!",
            "I'm out - can't support right now!",
            "No mana! I need a second!",
            "I'm spent - someone else handle this!"
        }
    },

    {
        FSB_ChatterCategory::emote_help, FSB_ChatterType::None,
        {
            "I need help over here!",
            "Someone get this thing off me!",
            "I can't handle this alone!",
            "Little help would be great!",
            "I'm getting overwhelmed!",
            "Help! I'm in trouble!",
            "I need backup, now!",
            "This one's too much for me!",
            "I can't keep this up alone!",
            "Anyone? A little help!",
            "I'm getting torn apart here!",
            "I need assistance!",
            "Someone cover me!",
            "I can't hold this thing!",
            "Help me out here!",
            "I'm struggling - need support!",
            "This is bad - help!",
            "I need someone on this target!",
            "I can't take much more of this!",
            "Back me up!"
        }
    },

    {
        FSB_ChatterCategory::emote_heal, FSB_ChatterType::None,
        {
            "I need healing!",
            "I'm not gonna last much longer!",
            "Healer! I need you!",
            "I'm badly hurt - help!",
            "I need a heal, now!",
            "I'm going down if I don't get healed!",
            "I'm bleeding out here!",
            "Someone patch me up!",
            "I can't take another hit!",
            "I'm in trouble - heal me!",
            "I'm hurt bad!",
            "I need healing support!",
            "I'm barely standing!",
            "I'm about to drop!",
            "Heals! I need heals!",
            "I'm critical - help!",
            "I can't survive like this!",
            "I'm one hit from dying!",
            "Please heal me!",
            "I need urgent healing!"
        }
    },

    {
        FSB_ChatterCategory::botCombatMana, FSB_ChatterType::None,
        {
            "Ugh. my brain's running on empty!",
            "Time for a little {spell} snack!",
            "If I run out of mana now, it's over. gulp!",
            "Someone toss me a {spell} potion, stat!",
            "I can almost feel my powers fading.",
            "Nothing a sip of magic can't fix!",
            "Mana low. desperation high!",
            "This {spell} better work. or I'm toast!",
            "Friend have a {spell} for me? No...?! Ok, I'll use mine..."
        }
    },

    {
        FSB_ChatterCategory::botCombatHealth, FSB_ChatterType::None,
        {
            "This {spell} is my last... wish I had more!",
            "No! Am too young to meet the spirit healer...",
            "{spell} I choose you! Don't let me down!",
            "Ugh. Think am gonna faint soon!",
            "Time for a little {spell} snack!",
            "If I run out of juice now, it's over. gulp!",
            "Someone toss me a {spell} potion, stat!",
            "I can almost feel my strength fading.",
            "Nothing a sip of magic can't fix!",
            "Health low. desperation high!",
            "This {spell} better work. or I have a date with the spirit healer!",
            "Can you spare a {spell} for me? No...?! Ok, I'll use mine..."
        }
    },

};

namespace FSBChatter
{
    static std::vector<FSBChatterDBLine const*> FilterChatterCandidates(Creature* bot, std::vector<FSBChatterDBLine> const& lines)
    {
        std::vector<FSBChatterDBLine const*> candidates;
        if (!bot)
            return candidates;

        uint32 botZoneId = bot->GetZoneId();
        uint8 botRaceId  = static_cast<uint8>(FSBMgr::Get()->GetBotRaceForEntry(bot->GetEntry()));
        uint8 botClassId = static_cast<uint8>(FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry()));

        for (auto const& line : lines)
        {
            if ((line.zoneId == 0 || line.zoneId == static_cast<int32>(botZoneId)) &&
                (line.fsbRaceId == 0 || line.fsbRaceId == botRaceId) &&
                (line.fsbClassId == 0 || line.fsbClassId == botClassId))
            {
                candidates.push_back(&line);
            }
        }
        return candidates;
    }

    std::string GetRandomEmoteText(Creature* bot, Unit* target, FSB_ChatterEmotes emote)
    {
        for (auto const& entry : FSBEmoteTextTable)
        {
            if (entry.emote == emote)
            {
                if (entry.lines.empty())
                {
                    TC_LOG_ERROR("scripts.fsb.chatter", "FSB: GetRandomEmoteText: No lines found for emote {}", static_cast<uint8>(emote));
                    return "";
                }

                std::string line = entry.lines[urand(0, entry.lines.size() - 1)];

                if (bot)
                    ReplaceAll(line, "{bot}", bot->GetName());
                if (target)
                    ReplaceAll(line, "{target}", target->GetName());

                TC_LOG_DEBUG("scripts.fsb.chatter", "FSB: GetRandomEmoteText: String [{}] selected for emote {}", line, static_cast<uint8>(emote));
                return line;
            }
        }

        TC_LOG_ERROR("scripts.fsb.chatter", "FSB: GetRandomEmoteText: Emote {} not found in FSBEmoteTextTable", static_cast<uint8>(emote));
        return "";
    }

    std::string GetRandomReply(Creature* bot, Unit* target, FSB_ChatterCategory category, FSB_ChatterType chatterType, uint32 spellId, uint8 duration)
    {
        Player* player = FSBMgr::Get()->GetBotOwner(bot);

        // --- DB-driven path (emote_kiss, emote_flirt, emote_joke, emote_talk, emote_cooking, whisper_afk, targetKilled, targetKilledHired, botDismissed, botAcknowledge, botFollow, botStay, botMemberDied, botRevived, botRevivedSelf, botDeathHired, botDeath) ---
        if (category == FSB_ChatterCategory::emote_kiss ||
            category == FSB_ChatterCategory::emote_flirt ||
            category == FSB_ChatterCategory::emote_joke ||
            category == FSB_ChatterCategory::emote_talk ||
            category == FSB_ChatterCategory::emote_cooking ||
            category == FSB_ChatterCategory::whisper_afk ||
            category == FSB_ChatterCategory::targetKilled ||
            category == FSB_ChatterCategory::targetKilledHired ||
            category == FSB_ChatterCategory::botDismissed ||
            category == FSB_ChatterCategory::botAcknowledge ||
            category == FSB_ChatterCategory::botFollow ||
            category == FSB_ChatterCategory::botStay ||
            category == FSB_ChatterCategory::botMemberDied ||
            category == FSB_ChatterCategory::botRevived ||
            category == FSB_ChatterCategory::botRevivedSelf ||
            category == FSB_ChatterCategory::botDeathHired ||
            category == FSB_ChatterCategory::botDeath ||
            category == FSB_ChatterCategory::botHiredPermanent ||
            category == FSB_ChatterCategory::botHire ||
            category == FSB_ChatterCategory::botRevivedTarget ||
            category == FSB_ChatterCategory::botHealTarget ||
            category == FSB_ChatterCategory::botHealSelf ||
            category == FSB_ChatterCategory::botBuffSelf ||
            category == FSB_ChatterCategory::botBuffTarget ||
            category == FSB_ChatterCategory::botOOCRecovery ||
            category == FSB_ChatterCategory::botOOCRecoveryHired ||
            category == FSB_ChatterCategory::botCombatSpell)
        {
            uint32 key = (static_cast<uint32>(category) << 8) | static_cast<uint32>(chatterType);
            auto it = BotChatterLinesMap.find(key);
            if (it == BotChatterLinesMap.end() || it->second.empty())
            {
                TC_LOG_ERROR("scripts.fsb.chatter", "FSB: No DB chatter lines found for category {} key {}", static_cast<uint8>(category), key);
                return "";
            }

            std::vector<FSBChatterDBLine const*> candidates = FilterChatterCandidates(bot, it->second);
            if (candidates.empty())
            {
                TC_LOG_ERROR("scripts.fsb.chatter", "FSB: DB lines exist for category {} key {} but none match bot zone/race/class filters", static_cast<uint8>(category), key);
                return "";
            }

            FSBChatterDBLine const* chosen = candidates[urand(0, candidates.size() - 1)];
            std::string line = chosen->lineText;

            // Token replacement
            if (bot)
                ReplaceAll(line, "{bot}", bot->GetName());
            if (target)
                ReplaceAll(line, "{target}", target->GetName());
            if (player)
                ReplaceAll(line, "{player}", player->GetName());
            if (spellId)
                ReplaceAll(line, "{spell}", FSBSpellsUtils::GetSpellName(spellId));
            if (duration == 1)
                ReplaceAll(line, "{duration}", "1 hour");
            if (duration > 1)
                ReplaceAll(line, "{duration}", std::to_string(duration) + " hours");

            TC_LOG_DEBUG("scripts.fsb.chatter", "FSB: Chatter GetRandomReply (DB): String [{}] selected for emote_kiss key {}", line, key);
            return line;
        }

        // --- Legacy static-table path (all other categories) ---
        for (auto const& entry : FSBReplyTable)
        {
            if (entry.category == category && entry.chatterType == chatterType)
            {
                if (entry.lines.empty())
                {
                    TC_LOG_WARN("scripts.fsb.chatter", "FSB: Chatter GetRandomReply: No string found for category {} and chatterType {}", category, chatterType);
                    return "";
                }

                std::string line = entry.lines[urand(0, (entry.lines.size() - 1))];

                // Token replacement
                if (bot)
                    ReplaceAll(line, "{bot}", bot->GetName());
                
                if (target)
                    ReplaceAll(line, "{target}", target->GetName());

                if(player)
                    ReplaceAll(line, "{player}", player->GetName());

                if (spellId)
                    ReplaceAll(line, "{spell}", FSBSpellsUtils::GetSpellName(spellId));

                if(duration == 1)
                    ReplaceAll(line, "{duration}", "1 hour");

                if (duration > 1)
                    ReplaceAll(line, "{duration}", std::to_string(duration) + " hours");

                TC_LOG_DEBUG("scripts.fsb.chatter", "FSB: Chatter GetRandomReply: String [{}] selected for category {} and chatterType {}", line, category, chatterType);

                return line;
            }
        }

        return "";
    }

    std::string GetDummyEmoteString(FSB_ChatterCategory category, FSB_ChatterType type)
    {
        if (category == FSB_ChatterCategory::emote_joke && type == FSB_ChatterType::Positive)
            return "emote:laugh";
        if ((category == FSB_ChatterCategory::emote_flirt || category == FSB_ChatterCategory::emote_kiss) && type == FSB_ChatterType::Negative)
            return "emote:rudeno";
        if (category == FSB_ChatterCategory::emote_oom && type == FSB_ChatterType::None)
            return "emote:oom";
        if (category == FSB_ChatterCategory::emote_heal && type == FSB_ChatterType::None)
            return "emote:heal";
        if (category == FSB_ChatterCategory::emote_help && type == FSB_ChatterType::None)
            return "emote:help";
        return "";
    }

    void PlayDummyEmote(Creature* bot, const std::string& dummyEmoteString)
    {
        if (!bot || dummyEmoteString.empty())
            return;

        auto* ai = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!ai)
            return;

        auto botRace = ai->botRace;
        auto botGender = FSBMgr::Get()->GetBotGenderForEntry(bot->GetEntry());

        TextEmotes tEmote = TEXT_EMOTE_AGREE;
        Emote emote = EMOTE_ONESHOT_NONE;

        if (dummyEmoteString == "emote:laugh")
        {
            tEmote = TEXT_EMOTE_LAUGH;
            emote = EMOTE_ONESHOT_LAUGH;
        }
        else if (dummyEmoteString == "emote:rudeno")
        {
            tEmote = RAND(TEXT_EMOTE_NO, TEXT_EMOTE_RASP);
            emote = (tEmote == TEXT_EMOTE_NO) ? EMOTE_ONESHOT_NO : EMOTE_ONESHOT_RUDE;
        }
        else if (dummyEmoteString == "emote:oom")
            tEmote = TEXT_EMOTE_OOM;
        else if (dummyEmoteString == "emote:heal")
            tEmote = TEXT_EMOTE_HEALME;
        else if (dummyEmoteString == "emote:help")
            tEmote = TEXT_EMOTE_HELPME;

        auto soundInfo = sDB2Manager.GetTextSoundEmoteFor(tEmote, FSBUtils::BotRaceToTC(botRace), botGender, 0);
        if (soundInfo)
            bot->PlayDistanceSound(soundInfo->SoundID);

        bot->HandleEmoteCommand(emote);
    }

    void DemandTimedReply(Creature* bot, Unit* target, FSB_ChatterCategory category, FSB_ReplyType replyType, FSB_ChatterSource chatterSource)
    {
        if (!bot)
            return;

        // we only expect the bot to be dead for those 2 categories
        if ((category != FSB_ChatterCategory::botDeath &&
            category != FSB_ChatterCategory::botDeathHired) &&
            !bot->IsAlive())
            return;

        if (target && !target->IsAlive())
            return;

        FSB_ChatterType type = FSB_ChatterType::None;

        if(target && chatterSource == FSB_ChatterSource::Target)
            type = FSBMgr::Get()->GetBotChatterTypeForEntry(target->GetEntry());

        else if (chatterSource == FSB_ChatterSource::Bot)
            type = FSBMgr::Get()->GetBotChatterTypeForEntry(bot->GetEntry());

        std::string replyString = GetRandomReply(bot, target, category, type, 0);

        TC_LOG_DEBUG("scripts.fsb.chatter", "FSB: Chatter DemandTimedReply: Bot {} String {} selected for category {} and chatterType {}", bot->GetName(), replyString, category, type);

        if (category == FSB_ChatterCategory::botDeath ||
            category == FSB_ChatterCategory::botDeathHired ||
            category == FSB_ChatterCategory::botMemberDied ||
            category == FSB_ChatterCategory::botRevivedTarget)
            target = nullptr;

        FSBEvents::ScheduleBotEventWithChatter(bot, FSB_EVENT_HIRED_TIMED_CHATTER_REPLY, 3s, 5s, replyType, replyString, target);

        std::string dummyEmote = GetDummyEmoteString(category, type);
        if (!dummyEmote.empty())
            FSBEvents::ScheduleBotEventWithChatter(bot, FSB_EVENT_HIRED_TIMED_DUMMY_EMOTE, 3s, 5s, replyType, dummyEmote, target);
    }

    void DemandTargetReply(Creature* bot, Creature* target, FSB_ChatterCategory category, FSB_ReplyType replyType)
    {
        if (!bot)
            return;

        if (!bot->IsAlive())
            return;

        if (!target || !target->IsAlive())
            return;

        FSB_ChatterType type = FSBMgr::Get()->GetBotChatterTypeForEntry(target->GetEntry());

        std::string replyString = GetRandomReply(bot, target, category, type, 0);

        TC_LOG_DEBUG("scripts.fsb.chatter", "FSB: Chatter DemandTargetReply: Bot {} Target {} String {} selected for category {} and chatterType {}", bot->GetName(), target->GetName(), replyString, category, type);

        FSBEvents::ScheduleBotEventWithChatter(bot, FSB_EVENT_HIRED_TIMED_CHATTER_REPLY, 3s, 5s, replyType, replyString, target);

        std::string dummyEmote = GetDummyEmoteString(category, type);
        if (!dummyEmote.empty())
            FSBEvents::ScheduleBotEventWithChatter(bot, FSB_EVENT_HIRED_TIMED_DUMMY_EMOTE, 3s, 5s, replyType, dummyEmote, target);
    }

    void DemandBotChatter(Creature* bot, Unit* target, FSB_ChatterCategory category, FSB_ReplyType replyType, FSB_ChatterSource chatterSource, uint32 spellId, uint16 duration)
    {
        if (!bot)
            return;

        if (urand(0, 99) > FollowshipBotsConfig::configFSBChatterRate)
            return;

        FSB_ChatterType type = FSB_ChatterType::None;

        if (chatterSource == FSB_ChatterSource::Bot)
            type = FSBMgr::Get()->GetBotChatterTypeForEntry(bot->GetEntry());

        std::string chatter = GetRandomReply(bot, target, category, type, spellId, duration);

        if (chatter.empty())
            return;

        if (replyType == FSB_ReplyType::Say)
            bot->Say(chatter, LANG_UNIVERSAL);

        if (replyType == FSB_ReplyType::Yell)
            bot->Yell(chatter, LANG_UNIVERSAL);

        if (replyType == FSB_ReplyType::Whisper)
        {
            Player* player = FSBMgr::Get()->GetBotOwner(bot);

            if(player)
                bot->Whisper(chatter, LANG_UNIVERSAL, player);
        }

    }

    void ReplaceAll(std::string& text, const std::string& from, const std::string& to)
    {
        size_t pos = 0;
        while ((pos = text.find(from, pos)) != std::string::npos)
        {
            text.replace(pos, from.length(), to);
            pos += to.length();
        }
    }
}


