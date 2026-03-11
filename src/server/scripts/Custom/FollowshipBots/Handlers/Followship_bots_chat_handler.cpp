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
#include "Player.h"
#include "Creature.h"
#include "ObjectMgr.h"
#include "Language.h"

#include "Followship_bots_mgr.h"
#include "Followship_bots_chat_handler.h"

namespace FSBChat
{
    Player* GetAnyPlayer()
    {
        SessionMap const& sessions = sWorld->GetAllSessions();

        for (auto const& itr : sessions)
        {
            if (Player* plr = itr.second->GetPlayer())
                return plr;
        }

        return nullptr;
    }

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

        Player* fake = GetAnyPlayer();
        if (!fake)
            return nullptr;

        Races botTCRace = FSBUtils::BotRaceToTC(FSBMgr::Get()->GetBotRaceForEntry(bot->GetEntry()));

        Team botTeam = Player::TeamForRace(botTCRace);

        ChannelMgr* mgr = ChannelMgr::ForTeam(botTeam);
        if (!mgr)
            return nullptr;

        uint32 zoneId = bot->GetZoneId();
        AreaTableEntry const* zone = sAreaTableStore.LookupEntry(zoneId);
        if (!zone)
            return nullptr;

        Channel* channel = mgr->GetChannel(channelId, "", fake, false, zone);

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

    // -------------------------------------------------------
    // Public API: Bot chat helpers
    // -------------------------------------------------------

    void BotSendGeneralChat(Creature* bot, std::string const& msg)
    {
        if (Channel* channel = GetGeneralChannel(bot))
            SendBotChannelMessage(bot, channel, msg);
        else TC_LOG_WARN("scripts.fsb.chatter", "FSB: BotSendGeneralChat Channel not found for bot {} with message {}", bot->GetName(), msg);
    }

    void BotSendTradeChat(Creature* bot, std::string const& msg)
    {
        if (Channel* channel = GetTradeChannel(bot))
            SendBotChannelMessage(bot, channel, msg);
        else TC_LOG_WARN("scripts.fsb.chatter", "FSB: BotSendTradeChat Channel not found for bot {} with message {}", bot->GetName(), msg);
    }

    void BotSendLocalDefenseChat(Creature* bot, std::string const& msg)
    {
        if (Channel* channel = GetLocalDefenseChannel(bot))
            SendBotChannelMessage(bot, channel, msg);
        else TC_LOG_WARN("scripts.fsb.chatter", "FSB: BotSendLocalDefenseChat Channel not found for bot {} with message {}", bot->GetName(), msg);
    }

    void BotSendLFGChat(Creature* bot, std::string const& msg)
    {
        if (Channel* channel = GetLFGChannel(bot))
            SendBotChannelMessage(bot, channel, msg);
        else TC_LOG_WARN("scripts.fsb.chatter", "FSB: BotSendLFGChat Channel not found for bot {} with message {}", bot->GetName(), msg);
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

        if (bots.size() < needed - 1) // minus 1 because starter is included
            return; // not enough bots nearby

        std::vector<Creature*> participants;
        participants.push_back(starter);

        // Shuffle the nearby bots
        Trinity::Containers::RandomShuffle(bots);

        // Add as many as needed
        for (size_t i = 0; i < needed - 1; ++i)
            participants.push_back(bots[i]);

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

    };
}
