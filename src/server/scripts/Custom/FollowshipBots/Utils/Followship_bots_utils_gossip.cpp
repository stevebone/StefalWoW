
#include "ScriptedGossip.h"

#include "Followship_bots_utils.h"
#include "Followship_bots_utils_gossip.h"
#include "Followship_bots_config.h"
#include "Followship_bots_mgr.h"

#include "Followship_bots_mage.h"
#include "Followship_bots_priest.h"
#include "Followship_bots_warrior.h"

namespace FSBUtilsGossip
{
    bool HandleDefaultGossipHello(Creature* me, Player* player, bool hired, ObjectGuid& outPlayerGuid)
    {
        if (!me || !player)
            return false;

        if (FSBMgr::CheckPlayerHasBotWithEntry(player, me->GetEntry()) && !hired)
        {
            me->Say(FSB_SAY_DUPLICATE_FOLLOWER, LANG_UNIVERSAL);
            ClearGossipMenuFor(player);
            CloseGossipMenuFor(player);
            return true;
        }

        me->SetOwnerGUID(player->GetGUID());
        outPlayerGuid = player->GetGUID();

        InitGossipMenuFor(player, FSB_GOSSIP_DEFAULT_MENU);

        if (hired)
        {
            AddGossipItemFor(player, GossipOptionNpc::Auctioneer,
                FSB_GOSSIP_ITEM_FIRE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);

            AddGossipItemFor(player, GossipOptionNpc::None,
                FSB_GOSSIP_ITEM_INFO, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);

            AddGossipItemFor(player, GossipOptionNpc::None,
                FSB_GOSSIP_MENU_ROLES, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);

            AddGossipItemFor(player, GossipOptionNpc::Trainer,
                FSB_GOSSIP_MENU_INSTRUCTIONS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 6);

            if (FSBUtils::GetBotClassForEntry(me->GetEntry()) == FSB_Class::Mage)
                AddGossipItemFor(player, GossipOptionNpc::None,
                    FSB_GOSSIP_MENU_PORTAL, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 26);
        }
        else
        {
            AddGossipItemFor(player, GossipOptionNpc::Auctioneer,
                FSB_GOSSIP_MENU_HIRE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

            AddGossipItemFor(player, GossipOptionNpc::None,
                FSB_GOSSIP_ITEM_INFO, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
        }

        SendGossipMenuFor(player, player->GetGossipTextId(me), me->GetGUID());
        return true;
    }

    bool HandleHireGossipSelect(Creature* me, Player* player)
    {
        uint32 pLevel = player->GetLevel();

        // we first calculate hire price based configs base price and hire duration options and actual player level
        int64 hirePrice1 = (FollowshipBotsConfig::configFSBPricePerLevel * pLevel * FollowshipBotsConfig::configFSBHireDuration1);
        std::string hireText1 = FSBUtilsTexts::BuildHireText(hirePrice1, FollowshipBotsConfig::configFSBHireDuration1);

        int64 hirePrice2 = (FollowshipBotsConfig::configFSBPricePerLevel * pLevel * FollowshipBotsConfig::configFSBHireDuration2);
        std::string hireText2 = FSBUtilsTexts::BuildHireText(hirePrice2, FollowshipBotsConfig::configFSBHireDuration2);

        int64 hirePrice3 = (FollowshipBotsConfig::configFSBPricePerLevel * pLevel * FollowshipBotsConfig::configFSBHireDuration3);
        std::string hireText3 = FSBUtilsTexts::BuildHireText(hirePrice3, FollowshipBotsConfig::configFSBHireDuration3);

        AddGossipItemFor(player, GossipOptionNpc::Auctioneer, hireText1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 10);
        AddGossipItemFor(player, GossipOptionNpc::Auctioneer, hireText2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 11);
        AddGossipItemFor(player, GossipOptionNpc::Auctioneer, hireText3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 12);
        AddGossipItemFor(player, GossipOptionNpc::Auctioneer, FSB_GOSSIP_ITEM_PHIRE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 13);
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_BACKMAIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);

        player->PlayerTalkClass->SendGossipMenu(FSB_GOSSIP_HIRE_MENU, me->GetGUID());
        return true;
    }

    void HandleInfoGossipSelect(Creature* me, Player* player)
    {
        if (!me || !player)
            return;

        // TO-DO for multiple classes we need separate switch/table here
        me->Say(FSB_GOSSIP_ITEM_PRIEST_INFO, LANG_UNIVERSAL);

        player->PlayerTalkClass->SendCloseGossip();
    }

    bool HandleRolesGossipSelect(Creature* me, Player* player)
    {
        FSB_Class botClass = FSBUtils::GetBotClassForEntry(me->GetEntry());
        switch (botClass)
        {
        case FSB_Class::None:
            break;
        case FSB_Class::Warrior:
            AddGossipItemFor(player, GossipOptionNpc::Trainer, FSB_GOSSIP_ITEM_ROLE_TANK, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 23);
            AddGossipItemFor(player, GossipOptionNpc::Trainer, FSB_GOSSIP_ITEM_ROLE_MELEE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 24);
            break;
        case FSB_Class::Priest:
            AddGossipItemFor(player, GossipOptionNpc::Trainer, FSB_GOSSIP_ITEM_ROLE_ASSIST, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 23);
            AddGossipItemFor(player, GossipOptionNpc::Trainer, FSB_GOSSIP_ITEM_ROLE_DAMAGE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 24);
            AddGossipItemFor(player, GossipOptionNpc::Trainer, FSB_GOSSIP_ITEM_ROLE_BALANCED, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 25);
            break;
        case FSB_Class::Mage:
            AddGossipItemFor(player, GossipOptionNpc::Trainer, FSB_GOSSIP_ITEM_ROLE_ARCANE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 23);
            AddGossipItemFor(player, GossipOptionNpc::Trainer, FSB_GOSSIP_ITEM_ROLE_FROST, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 24);
            AddGossipItemFor(player, GossipOptionNpc::Trainer, FSB_GOSSIP_ITEM_ROLE_FIRE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 25);
            break;
        case FSB_Class::Rogue:
            break;
        case FSB_Class::Druid:
            break;
        case FSB_Class::Paladin:
            break;
        case FSB_Class::Hunter:
            break;
        case FSB_Class::Warlock:
            break;
        default:
            break;
        }

        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_BACKMAIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);

        player->PlayerTalkClass->SendGossipMenu(FSB_GOSSIP_ROLES_MENU, me->GetGUID());
        return true;
    }

    bool HandleInstructionsGossipSelect(Creature* me, Player* player)
    {
        if (FSBUtilsMovement::GetMovementType(me) == FOLLOW_MOTION_TYPE)
        {
            AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_STAY_HERE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 20);
            AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_MENU_FOLLOW_DIST, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 7);
            AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_MENU_FOLLOW_ANGLE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 8);
            AddGossipItemFor(player, GossipOptionNpc::Auctioneer, FSB_GOSSIP_ITEM_BACKMAIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
        }
        else if (FSBUtilsMovement::GetMovementType(me) == IDLE_MOTION_TYPE)
        {
            AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_FOLLOW, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 21);
            AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_MENU_FOLLOW_DIST, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 7);
            AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_MENU_FOLLOW_ANGLE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 8);
            AddGossipItemFor(player, GossipOptionNpc::Auctioneer, FSB_GOSSIP_ITEM_BACKMAIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
        }

        player->PlayerTalkClass->SendGossipMenu(FSB_GOSSIP_INSTRUCTIONS_MENU, me->GetGUID());
        return true;
    }

    bool HandleFollowDistanceGossipSelect(Creature* me, Player* player)
    {
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_FOLLOW_DIST1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 14);
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_FOLLOW_DIST2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 15);
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_FOLLOW_DIST3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 16);
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_BACKMAIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);

        player->PlayerTalkClass->SendGossipMenu(FSB_GOSSIP_FOLLOW_DIST_MENU, me->GetGUID());
        return true;
    }

    bool HandleFollowAngleGossipSelect(Creature* me, Player* player)
    {
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_FOLLOW_ANGLE_FRONT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 17);
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_FOLLOW_ANGLE_BACK, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 18);
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_FOLLOW_ANGLE_RIGHT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 19);
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_FOLLOW_ANGLE_LEFT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 22);
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_BACKMAIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);

        player->PlayerTalkClass->SendGossipMenu(FSB_GOSSIP_FOLLOW_ANGLE_MENU, me->GetGUID());
        return true;
    }

    bool HandlePortalGossipSelect(Creature* me, Player* player)
    {
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_PORTAL_STORMWIND, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 27);
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_PORTAL_DARNASSUS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 28);
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_PORTAL_IRONFORGE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 29);
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_PORTAL_EXODAR, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 30);
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_BACKMAIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);

        player->PlayerTalkClass->SendGossipMenu(FSB_GOSSIP_PORTAL_MENU, me->GetGUID());
        return true;
    }
    

}
