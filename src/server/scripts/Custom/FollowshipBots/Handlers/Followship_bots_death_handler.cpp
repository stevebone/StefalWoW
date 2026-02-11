#include "Followship_bots.h"
#include "Followship_bots_config.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_utils.h"

#include "Followship_bots_death_handler.h"
#include "Followship_bots_group_handler.h"
#include "Followship_bots_teleport_handler.h"

#include "Followship_bots_warlock.h"

namespace FSBDeath
{
    void HandlerJustDied(Creature* bot, const std::vector<Unit*>& botGroup, bool hasSS)
    {
        if (!bot || bot->IsAlive())
            return;

        TC_LOG_DEBUG("scripts.ai.fsb", "FSB DeathHandler - Just Died for bot {}", bot->GetName());

        // handle chatter after death
        // TO-DO add more chatter when player is not around or when player is the attacker
        Player* player = FSBMgr::GetBotOwner(bot);
        if (player)
        {
            if (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
            {
                std::string msg = FSBUtilsTexts::BuildNPCSayText(player->GetName(), NULL, FSBSayType::BotDeath, "");
                bot->Yell(msg, LANG_UNIVERSAL);
            }
        }

        // handle death with soulstone
        if (hasSS)
        {
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB DeathHandler - Just Died with SS for bot {}", bot->GetName());
            bot->AI()->DoAction(FSB_ACTION_SOULSTONE_RESSURECT);
            return;
        }

        // handle death with healer present
        bool healerPresent = FSBGroup::BotGetFirstGroupHealer(botGroup);
        if (healerPresent)
        {
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB DeathHandler - Just Died with healer present for bot {}", bot->GetName());
            bot->AI()->DoAction(FSB_ACTION_WAIT_HEALER_RESSURECT);
            return;
        }

        // handle death with graveyard teleport
        if (!bot->GetMap()->IsDungeon())
        {
            bot->AI()->DoAction(FSB_ACTION_TELEPORT_GRAVEYARD);
            return;
        }
        else bot->AI()->DoAction(FSB_ACTION_TELEPORT_DUNGEON);
        
    }

    void HandleDeathWithGraveyard(Creature* bot, Position botCorpse)
    {
        if (!bot || bot->IsAlive())
            return;

        bot->setDeathState(ALIVE);
        bot->CastSpell(bot, SPELL_SPECIAL_GHOST);
        bot->SetHealth(1);

        bot->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
        bot->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
            
        bot->GetMotionMaster()->MovePoint(2, botCorpse, true);

    }

    void HandleDeathWithSoulstone(Creature* bot, bool& hasSS)
    {
        if (!bot || bot->IsAlive())
            return;

        bot->CastSpell(bot, SPELL_WARLOCK_SOULSTONE_VISUAL, false);
        bot->setDeathState(ALIVE);
        bot->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
        bot->RemoveAllAuras();
        hasSS = false;
    }

    void HandleDeathInDungeon(Creature* bot, float fDistance, float fAngle)
    {
        if (!bot || bot->IsAlive())
            return;

        bot->setDeathState(ALIVE);
        bot->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
        FSBUtilsMovement::ResumeFollow(bot, fDistance, fAngle);
    }

    void BotSetStateAfterCorpseRevive(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return;

        bot->RemoveAllAuras();
        bot->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
        bot->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
        bot->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
        bot->Say("This was a long way...", LANG_UNIVERSAL); //TO-DO add more texts here
    }
}
