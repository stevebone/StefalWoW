#include "Followship_bots_config.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_utils.h"

#include "Followship_bots_gossip_handler.h"
#include "Followship_bots_movement_handler.h"

#include "Followship_bots_mage.h"
#include "Followship_bots_paladin.h"
#include "Followship_bots_priest.h"
#include "Followship_bots_warlock.h"
#include "Followship_bots_warrior.h"

namespace FSBGossip
{
    bool HandleDefaultGossipHello(Creature* bot, Player* player, bool hired, ObjectGuid& outPlayerGuid)
    {
        if (!bot || !player)
            return false;

        if (FSBMgr::IsBotOwned(bot) && !FSBMgr::IsBotOwnedByPlayer(player, bot))
            return false;

        // TO-DO make this a method to check when to enable certain bots
        if (bot->GetEntry() == 16502 && !player->GetQuestRewardStatus(9283))
            return false;

        if (FSBMgr::CheckPlayerHasBotWithEntry(player, bot->GetEntry()) && !hired)
        {
            bot->Say(FSB_SAY_DUPLICATE_FOLLOWER, LANG_UNIVERSAL);
            ClearGossipMenuFor(player);
            CloseGossipMenuFor(player);
            return true;
        }

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

            if (FSBUtils::GetBotClassForEntry(bot->GetEntry()) == FSB_Class::Mage)
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

        SendGossipMenuFor(player, player->GetGossipTextId(bot), bot->GetGUID());
        return true;
    }

    bool HandleGossipMenuFollowAngle(Creature* bot, Player* player)
    {
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_FOLLOW_ANGLE_FRONT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 17);
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_FOLLOW_ANGLE_BACK, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 18);
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_FOLLOW_ANGLE_RIGHT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 19);
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_FOLLOW_ANGLE_LEFT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 22);
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_BACKMAIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);

        player->PlayerTalkClass->SendGossipMenu(FSB_GOSSIP_FOLLOW_ANGLE_MENU, bot->GetGUID());
        return true;
    }
    bool HandleGossipMenuFollowDistance(Creature* bot, Player* player)
    {
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_FOLLOW_DIST1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 14);
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_FOLLOW_DIST2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 15);
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_FOLLOW_DIST3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 16);
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_BACKMAIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);

        player->PlayerTalkClass->SendGossipMenu(FSB_GOSSIP_FOLLOW_DIST_MENU, bot->GetGUID());
        return true;
    }
    bool HandleGossipMenuHire(Creature* bot, Player* player)
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

        player->PlayerTalkClass->SendGossipMenu(FSB_GOSSIP_HIRE_MENU, bot->GetGUID());
        return true;
    }
    bool HandleGossipMenuInstructions(Creature* bot, Player* player)
    {
        if (FSBMovement::GetMovementType(bot) == FOLLOW_MOTION_TYPE)
        {
            AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_STAY_HERE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 20);
            AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_MENU_FOLLOW_DIST, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 7);
            AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_MENU_FOLLOW_ANGLE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 8);
            AddGossipItemFor(player, GossipOptionNpc::Auctioneer, FSB_GOSSIP_ITEM_BACKMAIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
        }
        else if (FSBMovement::GetMovementType(bot) == IDLE_MOTION_TYPE)
        {
            AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_FOLLOW, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 21);
            AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_MENU_FOLLOW_DIST, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 7);
            AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_MENU_FOLLOW_ANGLE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 8);
            AddGossipItemFor(player, GossipOptionNpc::Auctioneer, FSB_GOSSIP_ITEM_BACKMAIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
        }

        player->PlayerTalkClass->SendGossipMenu(FSB_GOSSIP_INSTRUCTIONS_MENU, bot->GetGUID());
        return true;
    }
    bool HandleGossipMenuPortals(Creature* bot, Player* player)
    {
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_PORTAL_STORMWIND, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 27);
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_PORTAL_DARNASSUS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 28);
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_PORTAL_IRONFORGE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 29);
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_PORTAL_EXODAR, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 30);
        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_BACKMAIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);

        player->PlayerTalkClass->SendGossipMenu(FSB_GOSSIP_PORTAL_MENU, bot->GetGUID());
        return true;
    }
    bool HandleGossipMenuRoles(Creature* bot, Player* player)
    {
        FSB_Class botClass = FSBUtils::GetBotClassForEntry(bot->GetEntry());
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
            AddGossipItemFor(player, GossipOptionNpc::Trainer, FSB_GOSSIP_ITEM_ROLE_TANK, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 23);
            AddGossipItemFor(player, GossipOptionNpc::Trainer, FSB_GOSSIP_ITEM_ROLE_MELEE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 24);
            AddGossipItemFor(player, GossipOptionNpc::Trainer, FSB_GOSSIP_ITEM_ROLE_HEALER, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 25);
            break;
        case FSB_Class::Hunter:
            break;
        case FSB_Class::Warlock:
            AddGossipItemFor(player, GossipOptionNpc::Trainer, FSB_GOSSIP_ITEM_ROLE_AFFLICTION, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 23);
            AddGossipItemFor(player, GossipOptionNpc::Trainer, FSB_GOSSIP_ITEM_ROLE_DEMONOLOGY, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 24);
            AddGossipItemFor(player, GossipOptionNpc::Trainer, FSB_GOSSIP_ITEM_ROLE_DESTRUCTION, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 25);
            break;
        default:
            break;
        }

        AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_BACKMAIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);

        player->PlayerTalkClass->SendGossipMenu(FSB_GOSSIP_ROLES_MENU, bot->GetGUID());
        return true;
    }

    void HandleGossipItemInfo(Creature* bot, Player* player)
    {
        if (!bot || !player)
            return;

        // TO-DO for multiple classes we need separate switch/table here
        bot->Say(FSB_GOSSIP_ITEM_PRIEST_INFO, LANG_UNIVERSAL);

        player->PlayerTalkClass->SendCloseGossip();
    }
    void HandleGossipItemRole(Creature* bot, FSB_Class botClass, uint8 roleOption, bool& demonDead)
    {
        if (!bot)
            return;

        switch (botClass)
        {
        case FSB_Class::Paladin:
            if (roleOption == FSB_GOSSIP_ROLE_1)
            {
                FSBUtils::SetRole(bot, FSB_Roles::FSB_ROLE_TANK);
                FSBPaladin::BotSetRoleAuras(bot, FSB_ROLE_TANK);
                break;
            }
            else if (roleOption == FSB_GOSSIP_ROLE_2)
            {
                FSBUtils::SetRole(bot, FSB_Roles::FSB_ROLE_MELEE_DAMAGE);
                FSBPaladin::BotSetRoleAuras(bot, FSB_ROLE_MELEE_DAMAGE);
                break;
            }
            else if (roleOption == FSB_GOSSIP_ROLE_3)
            {
                FSBUtils::SetRole(bot, FSB_Roles::FSB_ROLE_HEALER);
                FSBPaladin::BotSetRoleAuras(bot, FSB_ROLE_HEALER);
                break;
            }
            break;
        case FSB_Class::Warrior:
            if (roleOption == FSB_GOSSIP_ROLE_1)
            {
                FSBUtils::SetRole(bot, FSB_Roles::FSB_ROLE_TANK);
                bot->CastSpell(bot, SPELL_WARRIOR_DEFENSIVE_STANCE);
                break;
            }
            else if (roleOption == FSB_GOSSIP_ROLE_2)
            {
                FSBUtils::SetRole(bot, FSB_Roles::FSB_ROLE_MELEE_DAMAGE);
                uint32 spellId = urand(0, 1) ? SPELL_WARRIOR_BATTLE_STANCE : SPELL_WARRIOR_BERSERKER_STANCE;
                bot->CastSpell(bot, spellId);
                break;
            }
            else if (roleOption == FSB_GOSSIP_ROLE_3)
            {
                // we do not have a third role for warrior
                break;
            }
            break;
        case FSB_Class::Priest:
            if (roleOption == FSB_GOSSIP_ROLE_1)
            {
                FSBUtils::SetRole(bot, FSB_Roles::FSB_ROLE_HEALER);
                if (bot->HasAura(SPELL_PRIEST_SHADOWFORM))
                    bot->RemoveAurasDueToSpell(SPELL_PRIEST_SHADOWFORM);
                break;
            }
            else if (roleOption == FSB_GOSSIP_ROLE_2)
            {
                FSBUtils::SetRole(bot, FSB_Roles::FSB_ROLE_RANGED_DAMAGE);
                bot->CastSpell(bot, SPELL_PRIEST_SHADOWFORM);
                break;
            }
            else if (roleOption == FSB_GOSSIP_ROLE_3)
            {
                FSBUtils::SetRole(bot, FSB_Roles::FSB_ROLE_ASSIST);
                if (bot->HasAura(SPELL_PRIEST_SHADOWFORM))
                    bot->RemoveAurasDueToSpell(SPELL_PRIEST_SHADOWFORM);
                break;
            }
            break;
        case FSB_Class::Mage:
            if (roleOption == FSB_GOSSIP_ROLE_1)
            {
                FSBUtils::SetRole(bot, FSB_Roles::FSB_ROLE_RANGED_ARCANE);
                break;
            }
            else if (roleOption == FSB_GOSSIP_ROLE_2)
            {
                FSBUtils::SetRole(bot, FSB_Roles::FSB_ROLE_RANGED_FROST);
                break;
            }
            else if (roleOption == FSB_GOSSIP_ROLE_3)
            {
                FSBUtils::SetRole(bot, FSB_Roles::FSB_ROLE_RANGED_FIRE);
                break;
            }
            break;
        case FSB_Class::Warlock:
            if (roleOption == FSB_GOSSIP_ROLE_1)
            {
                FSBUtils::SetRole(bot, FSB_Roles::FSB_ROLE_RANGED_AFFLICTION);
                if (bot->HasAura(SPELL_WARLOCK_DEMON_ARMOR))
                    bot->RemoveAurasDueToSpell(SPELL_WARLOCK_DEMON_ARMOR);
            }
            else if (roleOption == FSB_GOSSIP_ROLE_2)
            {
                FSBUtils::SetRole(bot, FSB_Roles::FSB_ROLE_RANGED_DEMONOLOGY);
            }
            else if (roleOption == FSB_GOSSIP_ROLE_3)
            {
                FSBUtils::SetRole(bot, FSB_Roles::FSB_ROLE_RANGED_DESTRUCTION);
                if (bot->HasAura(SPELL_WARLOCK_DEMON_ARMOR))
                    bot->RemoveAurasDueToSpell(SPELL_WARLOCK_DEMON_ARMOR);
            }

            demonDead = true;

            break;
        default:
            break;
        }

        bot->SetReactState(REACT_AGGRESSIVE);
        bot->Say(FSB_SAY_FOLLOW_INFO_CHANGED, LANG_UNIVERSAL);
    }
}
