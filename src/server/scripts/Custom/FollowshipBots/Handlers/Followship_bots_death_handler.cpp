#include "Followship_bots.h"
#include "Followship_bots_config.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_utils.h"

#include "Followship_bots_death_handler.h"
#include "Followship_bots_events_handler.h"
#include "Followship_bots_group_handler.h"
#include "Followship_bots_movement_handler.h"
#include "Followship_bots_teleport_handler.h"

#include "Followship_bots_druid.h"
#include "Followship_bots_paladin.h"
#include "Followship_bots_priest.h"
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
        Player* player = FSBMgr::Get()->GetBotOwner(bot);
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
        FSBMovement::ResumeFollow(bot, fDistance, fAngle);
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

    bool CheckBotMemberDeath(Creature* bot)
    {
        if (!bot)
            return false;

        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: {} found {} units in group for check resurrection", bot->GetName(), botGroup.size());

        if (!bot->IsAlive())
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        auto& botGroup = baseAI->botLogicalGroup;
        auto& botSayMemberDead = baseAI->botSayMemberDead;
        auto& resTargetGuid = baseAI->botResurrectTargetGuid;

        if (!resTargetGuid.IsEmpty())
            return false;

        if (botSayMemberDead)
            return false;

        Unit* deadTarget = FSBGroup::BotGetFirstDeadMember(botGroup);

        // Validate pointer before doing anything else
        if (!deadTarget || !deadTarget->IsInWorld() || deadTarget->IsDuringRemoveFromWorld())
            return false;

        // Build safe names for logging and chatter
        const char* botName = (bot && bot->IsInWorld()) ? bot->GetName().c_str() : "";
        const char* targetName = (deadTarget && deadTarget->IsInWorld()) ? deadTarget->GetName().c_str() : "";

        // Announce death (only once)
        if (!botSayMemberDead && bot->IsAlive() &&
            urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
        {
            std::string msg = FSBUtilsTexts::BuildNPCSayText(
                targetName, 0, FSBSayType::PlayerOrMemberDead, "");
            bot->Yell(msg, LANG_UNIVERSAL);

            botSayMemberDead = true;
        }

        resTargetGuid = deadTarget->GetGUID();

        // Safe logging
        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: {} found dead unit {} for resurrection", botName, targetName);
        return true;
    }

    void HandleSpellResurrection(Creature* bot, uint32 spellId)
    {
        if (!bot || bot->IsAlive())
            return;

        switch (spellId)
        {
        case SPELL_PALADIN_REDEMPTION:
        case SPELL_PRIEST_RESURRECTION:
        case SPELL_DRUID_REVIVE:
        case SPELL_DRUID_REBIRTH:
            FSBEvents::ScheduleBotEvent(bot, FSB_EVENT_HIRED_SPELL_RESURRECT_STATE, 3s, 5s);
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Bot {} was resurrected by spell {}", bot->GetName(), FSBSpellsUtils::GetSpellName(spellId));
            break;
        default:
            break;
        }
    }

    void HandleSpellResurrectionDelayedAction(Creature* bot)
    {
        if (!bot || bot->IsAlive())
            return;

        bool isCombatRes = FSBUtilsCombat::IsCombatActive(bot);

        // When bot is resurrected we need to set it back to death state alive
        bot->setDeathState(ALIVE);
        bot->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
        uint64 maxHealth = bot->GetMaxHealth();
        uint64 maxMana = bot->GetMaxPower(POWER_MANA);
        float healthPct = 0.35f;
        float manaPct = 0.35f;

        if (isCombatRes)
        {
            healthPct = 1.f;
            manaPct = 0.2f;
        }


        bot->SetHealth(maxHealth * healthPct);
        if (maxMana > 1) bot->SetPower(POWER_MANA, maxMana * manaPct);
    }
}
