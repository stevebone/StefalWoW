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

#include "WorldSession.h"   // must come first for stupid clang
#include "ScriptMgr.h"
#include "Chat.h"
#include "ChatPackets.h"
#include "Channel.h"
#include "ChannelMgr.h"
#include "Containers.h"
#include "DB2Stores.h"
#include "GameTime.h"
#include "Log.h"
#include "World.h"
#include "Creature.h"
#include "ObjectMgr.h"
#include "Language.h"

#include "Followship_bots_mgr.h"
#include "Followship_bots_chat_handler.h"

namespace FSBChat
{
    // -------------------------------------------------------
    // Helpers to get built-in channels
    // General: channelId = 1
    // Trade:  channelId = 2
    // LocalDefense: 22
    // LFG: 26
    // -------------------------------------------------------

    static Channel* GetZoneChannel(Creature* bot, uint32 channelId)
    {
        if (!bot)
            return nullptr;

        Team botTeam = FSBUtils::GetTeamFromFSBRace(bot);

        ChannelMgr* mgr = ChannelMgr::ForTeam(botTeam);
        if (!mgr)
            return nullptr;

        uint32 zoneId = bot->GetZoneId();
        AreaTableEntry const* zone = sAreaTableStore.LookupEntry(zoneId);
        if (!zone)
            return nullptr;

        Channel* channel = mgr->GetChannel(channelId, "", nullptr, false, zone);

        return channel;
    }

    static Channel* GetGeneralChannel(Creature* bot)
    {
        // General channelId is 1 in Channel.db2
        return GetZoneChannel(bot, 1);
    }

    static Channel* GetTradeChannel(Creature* bot)
    {
        // Trade channelId is 2
        return GetZoneChannel(bot, 2);
    }

    static Channel* GetLocalDefenseChannel(Creature* bot)
    {
        // LocalDefense channelId is 22
        return GetZoneChannel(bot, 22);
    }

    static Channel* GetLFGChannel(Creature* bot)
    {
        // LookingForGroup channelId is 26
        return GetZoneChannel(bot, 26);
    }

    static void SendBotChannelMessage(Creature* bot, Channel* channel, std::string const& msg)
    {
        if (!bot || !channel || msg.empty())
            return;

        channel->SendBotMessage(bot, msg);
    }

    inline bool IsValidBot(Creature* c)
    {
        if (!c || !c->IsInWorld() || !c->IsAlive())
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(c->GetAI());
        if (!baseAI)
            return false;

        if (baseAI->botHired)
            return false;

        return true;
    }

    // -------------------------------------------------------
    // Public API: Bot chat helpers
    // -------------------------------------------------------

    void BotSendGeneralChat(Creature* bot, std::string const& msg)
    {
        if (Channel* channel = GetGeneralChannel(bot))
            SendBotChannelMessage(bot, channel, msg);
        else TC_LOG_DEBUG("scripts.fsb.chatter", "FSB: BotSendGeneralChat Channel not found for bot {} with message {}", bot->GetName(), msg);
    }

    void BotSendTradeChat(Creature* bot, std::string const& msg)
    {
        if (Channel* channel = GetTradeChannel(bot))
            SendBotChannelMessage(bot, channel, msg);
        else TC_LOG_DEBUG("scripts.fsb.chatter", "FSB: BotSendTradeChat Channel not found for bot {} with message {}", bot->GetName(), msg);
    }

    void BotSendLocalDefenseChat(Creature* bot, std::string const& msg)
    {
        if (Channel* channel = GetLocalDefenseChannel(bot))
            SendBotChannelMessage(bot, channel, msg);
        else TC_LOG_DEBUG("scripts.fsb.chatter", "FSB: BotSendLocalDefenseChat Channel not found for bot {} with message {}", bot->GetName(), msg);
    }

    void BotSendLFGChat(Creature* bot, std::string const& msg)
    {
        if (Channel* channel = GetLFGChannel(bot))
            SendBotChannelMessage(bot, channel, msg);
        else TC_LOG_DEBUG("scripts.fsb.chatter", "FSB: BotSendLFGChat Channel not found for bot {} with message {}", bot->GetName(), msg);
    }

    // Global (or file-static) storage for all running conversations
    std::vector<ActiveConversation> activeConversations;

    void StartBotConversation(Creature* starter)
    {
        if (!starter || !starter->IsAlive())
            return;

        // Pick a random template
        size_t index = urand(0, (ChatTables.size() - 1));
        ConversationTemplate* tmpl = &ChatTables[index];

        // find nearby bots
        auto bots = FSBUtils::FindNearbyBots(starter);

        size_t needed = CountUniqueRoles(*tmpl);

        // Starter is always valid (we checked earlier)
        std::vector<Creature*> participants;
        participants.push_back(starter);

        // Shuffle the nearby bots
        Trinity::Containers::RandomShuffle(bots);

        // Add valid bots only
        for (size_t i = 0; i < bots.size() && participants.size() < needed; ++i)
        {
            Creature* candidate = bots[i];

            if (!IsValidBot(candidate))
                continue;

            participants.push_back(candidate);
        }

        // If we still don't have enough valid bots, abort
        if (participants.size() < needed)
            return;

        // Build role ? participant index map
        std::map<ConversationRole, size_t> roleMap;

        // Starter ALWAYS gets index 0
        roleMap[ConversationRole::Starter] = 0;

        // Collect unique non-starter roles in order of appearance
        std::vector<ConversationRole> orderedRoles;
        for (auto const& line : tmpl->lines)
        {
            if (line.role == ConversationRole::Starter)
                continue;

            if (std::find(orderedRoles.begin(), orderedRoles.end(), line.role) == orderedRoles.end())
                orderedRoles.push_back(line.role);
        }

        // Assign remaining roles starting at index 1
        for (size_t i = 0; i < orderedRoles.size(); ++i)
            roleMap[orderedRoles[i]] = i + 1;

        // Build conversation
        ActiveConversation conv;
        conv.tmpl = tmpl;
        conv.participants = participants;
        conv.roleMap = roleMap;
        conv.currentLine = 0;
        conv.nextSpeakTime = GameTime::GetGameTime() + urand(1, 5);

        activeConversations.push_back(conv);
    }

    void UpdateBotConversations()
    {
        uint32 now = GameTime::GetGameTime();

        for (auto it = activeConversations.begin(); it != activeConversations.end(); )
        {
            ActiveConversation& conv = *it;

            if (now >= conv.nextSpeakTime)
            {
                auto& line = conv.tmpl->lines[conv.currentLine];

                // Find which participant should speak
                auto itRole = conv.roleMap.find(line.role);
                if (itRole != conv.roleMap.end())
                {
                    size_t speakerIndex = itRole->second;
                    if (speakerIndex < conv.participants.size())
                    {
                        Creature* speaker = conv.participants[speakerIndex];
                        if (!IsValidBot(speaker))
                        {
                            it = activeConversations.erase(it);
                            continue;
                        }

                        BotSendGeneralChat(speaker, line.text);
                    }
                }

                conv.currentLine++;

                // End of conversation?
                if (conv.currentLine >= conv.tmpl->lines.size())
                {
                    it = activeConversations.erase(it);
                    continue;
                }
                else
                {
                    conv.nextSpeakTime = now + urand(2, 5);
                }
            }

            ++it;
        }
    }

    bool IsBotInConversation(Creature* bot)
    {
        for (auto const& conv : activeConversations)
            for (auto* p : conv.participants)
                if (p == bot)
                    return true;

        return false;
    }

    size_t CountUniqueRoles(const ConversationTemplate& tmpl)
    {
        std::set<ConversationRole> roles;
        for (auto const& line : tmpl.lines)
            roles.insert(line.role);
        return roles.size();
    }

    inline const char* GetColorCode(uint32 quality)
    {
        switch (quality)
        {
        case 0: return "|cff9d9d9d"; // Poor (gray)
        case 1: return "|cffffffff"; // Common (white)
        case 2: return "|cff1eff00"; // Uncommon (green)
        case 3: return "|cff0070dd"; // Rare (blue)
        case 4: return "|cffa335ee"; // Epic (purple)
        case 5: return "|cffff8000"; // Legendary (orange)
        case 6: return "|cffe6cc80"; // Artifact (gold)
        default: return "|cffffffff"; // fallback white
        }
    }

    inline std::string BuildItemLink(uint32 itemId)
    {
        if (!itemId)
            return "";

        std::string name = "<unknown>";
        const char* qualityTag = "|cffffffff"; // default white

        if (ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId))
        {
            name = proto->GetName(LOCALE_enUS);
            qualityTag = GetColorCode(proto->GetQuality());
        }

        std::ostringstream ss;
        ss << qualityTag
            << "|Hitem:" << itemId << ":0:0:0:0:0:0:0:0|h["
            << name << "]|h|r";

        return ss.str();
    }

    inline std::string BuildSpellLink(uint32 spellId)
    {
        if (!spellId)
            return "";

        const char* color = "|cff71d5ff"; // standard spell link color
        std::string name = FSBSpellsUtils::GetSpellName(spellId);

        std::ostringstream ss;
        ss << color
            << "|Hspell:" << spellId << "|h["
            << name << "]|h|r";

        return ss.str();
    }

    inline void ReplaceAll(std::string& text, const std::string& from, const std::string& to)
    {
        size_t pos = 0;
        while ((pos = text.find(from, pos)) != std::string::npos)
        {
            text.replace(pos, from.length(), to);
            pos += to.length();
        }
    }

    inline std::string GetAreaName(uint32 areaId)
    {
        if (AreaTableEntry const* area = sAreaTableStore.LookupEntry(areaId))
        {
            std::string name = area->AreaName[LOCALE_enUS];
            return name;
        }

        return "<unknown area>";
    }

    std::vector<uint32> tradeItems = { 2840, 2835, 2589, 2318 };

    inline uint32 GetRandomItemId(const std::vector<uint32>& items)
    {
        if (items.empty())
            return 0;

        return items[urand(0, items.size() - 1)];
    }


    inline RandomChatTemplate* GetRandomMatchingLine(Creature* bot, ChatChannelType channel)
    {
        if (!bot)
            return nullptr;

        uint32 zoneId = bot->GetZoneId();
        Team team = FSBUtils::GetTeamFromFSBRace(bot);

        std::vector<RandomChatTemplate*> matches;

        for (auto& entry : RandomChatTables)
        {
            if (entry.channel != channel)
                continue;

            if (entry.zoneId != 0 && entry.zoneId != zoneId)
                continue;

            if (entry.team != Team::PANDARIA_NEUTRAL && entry.team != team)
                continue;

            matches.push_back(&entry);
        }

        if (matches.empty())
            return nullptr;

        return matches[urand(0, matches.size() - 1)];
    }

    void StartBotRandomChat(Creature* bot, ChatChannelType channel)
    {
        if (!bot)
            return;

        RandomChatTemplate* line = GetRandomMatchingLine(bot, channel);
        if (!line)
            return;

        // Replace tags before sending
        std::string msg = line->text;

        if (line->spellId)
            ReplaceAll(msg, "{spell}", BuildSpellLink(line->spellId));

        if (line->itemId)
            ReplaceAll(msg, "{item}", BuildItemLink(line->itemId));

        ReplaceAll(msg, "{area}", GetAreaName(bot->GetAreaId()));
        ReplaceAll(msg, "{item}", BuildItemLink(GetRandomItemId(tradeItems)));

        switch (channel)
        {
        case ChatChannelType::General:
            BotSendGeneralChat(bot, msg);
            break;

        case ChatChannelType::Trade:
            BotSendTradeChat(bot, msg);
            break;

        case ChatChannelType::LocalDefense:
            BotSendLocalDefenseChat(bot, msg);
            break;

        case ChatChannelType::LFG:
            BotSendLFGChat(bot, msg);
            break;

        case ChatChannelType::Custom:
        default:
            BotSendGeneralChat(bot, msg);
            break;
        }
    }

    std::vector<RandomChatTemplate> RandomChatTables =
    {
        // Stormwind
        { 1519, ChatChannelType::General,      Team::ALLIANCE,          0,      0,      "Stormwind sure is crowded today..." },
        { 1519, ChatChannelType::LFG,          Team::ALLIANCE,          0,      0,      "LFM for Deadmines, need tank and healer!" },

        // Elwyn
        {   12, ChatChannelType::General,      Team::ALLIANCE,          0,      0,      "The marshal in Goldshire was not looking happy today..." },

        {   12, ChatChannelType::LocalDefense, Team::ALLIANCE,          0,      0,      "Another wolf attack near the farms!" },

        // No zone - generic
        {    0, ChatChannelType::General,      Team::PANDARIA_NEUTRAL,  0,      0,      "Anyone else feel like the respawn timers are getting shorter?" },
        {    0, ChatChannelType::General,      Team::PANDARIA_NEUTRAL,  0,      0,      "I am in {area} today... who knows where I will be tomorrow!" },
        {    0, ChatChannelType::General,      Team::PANDARIA_NEUTRAL,  0,      0,      "Yesterday I was in the Blasted Lands, today I am back in {area}." },
        {    0, ChatChannelType::General,      Team::PANDARIA_NEUTRAL,  1953,   0,      "Mages should just stop with their {spell} inside the inn." },

        {    0, ChatChannelType::Trade,        Team::PANDARIA_NEUTRAL,  0,      2770,   "WTS 20x {item} cheap!" },
        {    0, ChatChannelType::Trade,        Team::PANDARIA_NEUTRAL,  0,      0,      "WTS tons of {item} cheap!" },
        {    0, ChatChannelType::Trade,        Team::PANDARIA_NEUTRAL,  0,      0,      "WTB {item} please send offer!" },
        {    0, ChatChannelType::Trade,        Team::PANDARIA_NEUTRAL,  0,      0,      "Anyone have some {item} I can buy?" },
        {    0, ChatChannelType::Trade,        Team::PANDARIA_NEUTRAL,  0,      0,      "Giving some {item} for free, contact me!" },
        {    0, ChatChannelType::Trade,        Team::PANDARIA_NEUTRAL,  0,      0,      "Does anyone need {item} otherwise they go on auction..." },
        
    };

    std::vector<ConversationTemplate> ChatTables = {
    {
        "TavernGossip",
        {
            { ConversationRole::Starter,   "Did you hear what happened in Goldshire last night?" },
            { ConversationRole::Responder, "Oh, don't tell me. another drunken mage blew up the inn?" },
            { ConversationRole::Follower,  "No no, this time it was a paladin. Much louder." }
        }
    },
    {
        "StormwindShenanigans",
        {
            { ConversationRole::Starter,   "Did you see that mage trying to teleport onto the Cathedral roof again?" },
            { ConversationRole::Responder, "He missed and landed in the canal. The fish are still traumatized." },
            { ConversationRole::Follower,  "I heard the guards thought he was a murloc and almost threw a net on him." },
            { ConversationRole::Extra,     "Honestly, with the noises he made climbing out, I don't blame them." }
        }
    },
    {
        "StormwindKingDebate",
        {
            { ConversationRole::Starter,   "If there was a vote tomorrow for who should be king of Stormwind, who would win?" },
            { ConversationRole::Responder, "Ragnaros...?" },
            { ConversationRole::Starter,   "That was a serious question..." },
            { ConversationRole::Responder, "That was a serious answer." },
            { ConversationRole::Starter,   "You can't be serious, really!" },
            { ConversationRole::Responder, "Ok, sorry, I'll try again." },
            { ConversationRole::Responder, "Hogger, yeah that's the one... Hogger!" },
            { ConversationRole::Starter,   "Anyone else... please!" },
        }
    },
    {
        "MageQuarterDisaster",
        {
            { ConversationRole::Starter,   "Why does the Mage Quarter always smell like burnt eyebrows?" },
            { ConversationRole::Responder, "Because mages keep trying spells they found scribbled on napkins." },
            { ConversationRole::Follower,  "Last week someone summoned a sheep that exploded into more sheep." },
            { ConversationRole::Extra,     "Honestly, that was an improvement over the usual smell." }
        }
    },

    {
        "DwarvenEngineeringDebate",
        {
            { ConversationRole::Starter,   "Is it true dwarves invented a cannon that fires smaller cannons?" },
            { ConversationRole::Responder, "Aye, but the recoil launched Ironforge three inches to the left." },
            { ConversationRole::Follower,  "They said it was 'within acceptable engineering tolerances'." }
        }
    },

    {
        "StormwindCanalMystery",
        {
            { ConversationRole::Starter,   "Something big is living in the Stormwind canals." },
            { ConversationRole::Responder, "If it's big, it's probably a mage experiment gone wrong." },
            { ConversationRole::Follower,  "If it's small, it's definitely a mage experiment gone wrong." }
        }
    },

    {
        "GoldshireRumors",
        {
            { ConversationRole::Starter,   "I heard Goldshire had to hire extra guards last night." },
            { ConversationRole::Responder, "To stop trouble?" },
            { ConversationRole::Starter,   "No, to stop people from *starting* trouble." }
        }
    },

    {
        "StormwindPolitics",
        {
            { ConversationRole::Starter,   "Stormwind politics are getting weird again." },
            { ConversationRole::Responder, "Again? They never stopped." },
            { ConversationRole::Follower,  "Last council meeting ended with a pie fight." },
            { ConversationRole::Extra,     "And the pie won." }
        }
    },

    {
        "ElwynnForestFacts",
        {
            { ConversationRole::Starter,   "Fun fact: Elwynn Forest has more wolves than trees." },
            { ConversationRole::Responder, "That can't be true." },
            { ConversationRole::Starter,   "Have you *seen* how fast they... uhm *respawn*?" },
            { ConversationRole::Responder, "Aahaha I see what you mean..." },
        }
    },

    {
        "StormwindGuardLife",
        {
            { ConversationRole::Starter,   "Being a Stormwind guard must be exhausting." },
            { ConversationRole::Responder, "Standing still all day?" },
            { ConversationRole::Starter,   "No, pretending not to see half the nonsense happening around them." }
        }
    },

    {
        "CathedralSquareDrama",
        {
            { ConversationRole::Starter,   "Why was Cathedral Square blocked off this morning?" },
            { ConversationRole::Responder, "A paladin tried to bless the fountain." },
            { ConversationRole::Follower,  "He slipped, fell in, and declared it holy water." },
            { ConversationRole::Extra,     "Honestly, I respect the commitment." }
        }
    },

    {
        "StormwindAuctionChaos",
        {
            { ConversationRole::Starter,   "The Auction House was wild today." },
            { ConversationRole::Responder, "What happened?" },
            { ConversationRole::Starter,   "Someone listed a single linen cloth for 999 gold." },
            { ConversationRole::Follower,  "And someone bought it, didn't they?" },
            { ConversationRole::Starter,   "Instantly." },
            { ConversationRole::Responder, "You're joking..." },
            { ConversationRole::Starter,   "Wish I was..." }
        }
    },

    {
        "ParkPhilosophers",
        {
            { ConversationRole::Starter,   "Why do people in the Stormwind Park always talk like philosophers?" },
            { ConversationRole::Responder, "Because they're druids. They can't help it." },
            { ConversationRole::Follower,  "Last one asked me if my spirit animal was 'emotionally fulfilled'." }
        }
    },

    {
        "StormwindFoodDebate",
        {
            { ConversationRole::Starter,   "Stormwind needs better food vendors." },
            { ConversationRole::Responder, "What's wrong with the current ones?" },
            { ConversationRole::Starter,   "I ordered soup. It came in a bucket." },
            { ConversationRole::Extra,     "That's just 'rustic presentation'." }
        }
    },

    {
        "GryphonRiderConfessions",
        {
            { ConversationRole::Starter,   "Gryphon riders must have nerves of steel." },
            { ConversationRole::Responder, "Or no nerves at all." },
            { ConversationRole::Follower,  "One told me he fell asleep mid-flight once." },
            { ConversationRole::Extra,     "The gryphon landed better than he does." }
        }
    },

    {
        "StormwindBankSecrets",
        {
            { ConversationRole::Starter,   "What do you think is really inside the Stormwind Bank vault?" },
            { ConversationRole::Responder, "Gold." },
            { ConversationRole::Starter,   "No, I mean *really*." },
            { ConversationRole::Follower,  "Probably a very confused kobold." }
        }
    },

    {
        "TradeDistrictChaos",
        {
            { ConversationRole::Starter,   "Trade District was on fire again." },
            { ConversationRole::Responder, "Literal or metaphorical?" },
            { ConversationRole::Starter,   "Both." }
        }
    },

    {
        "StormwindFashionPolice",
        {
            { ConversationRole::Starter,   "Stormwind needs a fashion police force." },
            { ConversationRole::Responder, "Why?" },
            { ConversationRole::Starter,   "I saw a mage wearing plate boots with a robe." },
            { ConversationRole::Follower,  "That should be illegal." },
            { ConversationRole::Responder, "It is..." },
        }
    },

    {
        "StormwindBridgePhobia",
        {
            { ConversationRole::Starter,   "Why do people avoid the bridge near the Mage Tower?" },
            { ConversationRole::Responder, "Because it collapses every Tuesday." },
            { ConversationRole::Follower,  "Maintenance day?" },
            { ConversationRole::Starter,   "No, mage experiments." }
        }
    },

    {
        "StormwindZooRumors",
        {
            { ConversationRole::Starter,   "Is it true Stormwind once had a zoo?" },
            { ConversationRole::Responder, "Yes, but everything escaped." },
            { ConversationRole::Follower,  "Including the staff." }
        }
    },

    {
        "StormwindLibraryFacts",
        {
            { ConversationRole::Starter,   "Stormwind Library has a book that screams when opened." },
            { ConversationRole::Responder, "Sounds like me on Monday mornings." }
        }
    },

    {
        "StormwindInnDrama",
        {
            { ConversationRole::Starter,   "The innkeeper threw someone out again." },
            { ConversationRole::Responder, "What did they do?" },
            { ConversationRole::Starter,   "They asked for a refund on their 'rested experience'." }
        }
    },

    {
        "StormwindRandomFacts",
        {
            { ConversationRole::Starter,   "Random fact: Stormwind has exactly 37 benches." },
            { ConversationRole::Responder, "Who counted them?" },
            { ConversationRole::Starter,   "Someone with too much time and not enough quests." }
        }
    },

    {
        "StormwindBridgePhilosophy",
        {
            { ConversationRole::Starter,   "Have you ever noticed how everyone walks slower on this bridge?" },
            { ConversationRole::Responder, "It's because the view makes people think deep thoughts." },
            { ConversationRole::Follower,  "Last week I saw a mage staring at the water for ten minutes." },
            { ConversationRole::Extra,     "Was he meditating?" },
            { ConversationRole::Follower,  "No, he dropped his wand and was trying to fish it out with telekinesis." },
            { ConversationRole::Responder, "That explains the explosion I heard." },
            { ConversationRole::Starter,   "Stormwind bridges are never boring." }
        }
    },
    {
        "ElwynnForestEconomics",
        {
            { ConversationRole::Starter,   "Why do wolves in Elwynn drop copper coins?" },
            { ConversationRole::Responder, "Maybe they're saving up for armor upgrades." },
            { ConversationRole::Follower,  "Or maybe adventurers keep dropping money while running away." },
            { ConversationRole::Extra,     "Honestly, wolves probably have a better savings account than I do." },
            { ConversationRole::Responder, "At this point, I'd trust a wolf with my finances." },
            { ConversationRole::Starter,   "Elwynn Forest: where the wildlife is wealthier than the guards." }
        }
    },
    {
        "StormwindInnConspiracy",
        {
            { ConversationRole::Starter,   "Have you noticed the innkeeper never sleeps?" },
            { ConversationRole::Responder, "I've never seen him blink either." },
            { ConversationRole::Follower,  "Maybe he's secretly a titan construct." },
            { ConversationRole::Extra,     "A titan construct serving ale?" },
            { ConversationRole::Follower,  "Hey, everyone needs a hobby." },
            { ConversationRole::Responder, "Explains why the ale never runs out." },
            { ConversationRole::Starter,   "Stormwind's greatest mystery isn't the canals. it's the innkeeper." }
        }
    },
    {
        "MageQuarterTimeTravel",
        {
            { ConversationRole::Starter,   "A mage told me he traveled five minutes into the future." },
            { ConversationRole::Responder, "That's just called taking a nap." },
            { ConversationRole::Follower,  "No, he insists it was a temporal displacement." },
            { ConversationRole::Extra,     "Did anything change?" },
            { ConversationRole::Follower,  "He said his tea got cold." },
            { ConversationRole::Responder, "Revolutionary." },
            { ConversationRole::Starter,   "Mages will call anything a spell if it makes them feel important." }
        }
    },
    {
        "StormwindGuardUnion",
        {
            { ConversationRole::Starter,   "Did you hear the guards are forming a union?" },
            { ConversationRole::Responder, "About time. Standing still all day can't be healthy." },
            { ConversationRole::Follower,  "Their demands include chairs, better boots, and fewer dramatic speeches." },
            { ConversationRole::Extra,     "Fewer speeches? But that's half their job!" },
            { ConversationRole::Follower,  "Exactly. They want to cut it down to one heroic line per hour." },
            { ConversationRole::Responder, "Honestly, I'd sign that petition." },
            { ConversationRole::Starter,   "Stormwind might finally become a quieter place." }
        }
    },
    {
        "CathedralSquareConfessions",
        {
            { ConversationRole::Starter,   "A priest told me he once tried to bless a squirrel." },
            { ConversationRole::Responder, "Why would he do that?" },
            { ConversationRole::Follower,  "He said it looked 'spiritually confused'." },
            { ConversationRole::Extra,     "And what happened?" },
            { ConversationRole::Follower,  "The squirrel stole his lunch and ran off." },
            { ConversationRole::Responder, "A holy squirrel with a sandwich. Terrifying." },
            { ConversationRole::Starter,   "Cathedral Square is getting stranger every day." }
        }
    },
    {
        "StormwindMarketInflation",
        {
            { ConversationRole::Starter,   "Why is bread so expensive in Stormwind lately?" },
            { ConversationRole::Responder, "Supply issues. The baker's oven exploded again." },
            { ConversationRole::Follower,  "That's the third time this month!" },
            { ConversationRole::Extra,     "Maybe he should stop experimenting with 'arcane yeast'." },
            { ConversationRole::Follower,  "He said it would rise faster." },
            { ConversationRole::Responder, "It rose so fast it hit the ceiling." },
            { ConversationRole::Starter,   "Stormwind's economy is held together by duct tape and hope." }
        }
    },
    {
        "GryphonAerialAccidents",
        {
            { ConversationRole::Starter,   "I heard a gryphon rider dropped his lunch mid-flight." },
            { ConversationRole::Responder, "That's unfortunate." },
            { ConversationRole::Follower,  "Unfortunate for him, delicious for the wolves below." },
            { ConversationRole::Extra,     "Do gryphons get upset when food falls off?" },
            { ConversationRole::Follower,  "Apparently this one circled back to look for it." },
            { ConversationRole::Responder, "A noble creature. with priorities." },
            { ConversationRole::Starter,   "Stormwind's skies are full of drama." }
        }
    },
    {
        "StormwindLibraryDebate",
        {
            { ConversationRole::Starter,   "The library banned a book for being 'too loud'." },
            { ConversationRole::Responder, "How can a book be loud?" },
            { ConversationRole::Follower,  "It screamed every time someone opened it." },
            { ConversationRole::Extra,     "Sounds cursed." },
            { ConversationRole::Follower,  "The librarian said it was just 'overly dramatic'." },
            { ConversationRole::Responder, "Honestly, same." },
            { ConversationRole::Starter,   "Stormwind literature is wild." }
        }
    },
    {
        "StormwindParkDruidCircle",
        {
            { ConversationRole::Starter,   "The druids in the park held a meeting today." },
            { ConversationRole::Responder, "About what?" },
            { ConversationRole::Follower,  "Apparently the trees are 'feeling underappreciated'." },
            { ConversationRole::Extra,     "Did the trees say that?" },
            { ConversationRole::Follower,  "The druids said they could 'sense it'." },
            { ConversationRole::Responder, "I sense they need a hobby." },
            { ConversationRole::Starter,   "Stormwind Park is basically a nature-themed soap opera." }
        }
    },
    
    {
        "ThunderfuryDebate",
        {
            { ConversationRole::Starter,   "Alright, be honest. if someone actually looted |cffff8000|Hitem:19019:0:0:0:0:0:0:0:0|h[Thunderfury, Blessed Blade of the Windseeker]|h|r today, what would happen?" },
            { ConversationRole::Responder, "Stormwind would hear the scream from Ironforge." },
            { ConversationRole::Follower,  "Half the city would whisper 'Did someone say Thunderfury?' for hours." },
            { ConversationRole::Extra,     "And the other half would pretend they don't care while secretly crying." },
            { ConversationRole::Responder, "Let's be real, even the gryphon riders would stop mid-air to look." },
            { ConversationRole::Follower,  "I once saw a mage faint when someone linked it in trade chat." },
            { ConversationRole::Starter,   "Thunderfury: the only item that causes emotional damage just by existing." }
        }
    },
    {
        "PaladinBubbleArgument",
        {
            { ConversationRole::Starter,   "I swear, paladins rely on |cff71d5ff|Hspell:642|h[Divine Shield]|h|r way too much." },
            { ConversationRole::Responder, "It's not reliance, it's 'strategic invulnerability'." },
            { ConversationRole::Follower,  "Strategic? They bubble when they stub their toe." },
            { ConversationRole::Extra,     "I saw one bubble because a squirrel startled him." },
            { ConversationRole::Responder, "Hey, squirrels in Elwynn are vicious." },
            { ConversationRole::Follower,  "Still doesn't justify bubbling during a cooking fire mishap." },
            { ConversationRole::Starter,   "Paladins don't tank danger. they tank embarrassment." }
        }
    },

    };
}
