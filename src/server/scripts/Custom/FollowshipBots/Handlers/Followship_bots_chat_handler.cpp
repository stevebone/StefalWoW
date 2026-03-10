#include "ScriptMgr.h"
#include "Chat.h"
#include "ChatPackets.h"
#include "Channel.h"
#include "ChannelMgr.h"
#include "Log.h"
#include "World.h"
#include "Player.h"
#include "Creature.h"
#include "ObjectMgr.h"
#include "Language.h"

#include "Followship_bots_mgr.h"

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

        channel->SendBotMessage(bot,msg);
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
}
