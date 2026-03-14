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
#include "Map.h"

#include "Followship_bots.h"
#include "Followship_bots_config.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_utils.h"

#include "Followship_bots_chatter_handler.h"
#include "Followship_bots_death_handler.h"
#include "Followship_bots_events_handler.h"
#include "Followship_bots_group_handler.h"
#include "Followship_bots_movement_handler.h"
#include "Followship_bots_teleport_handler.h"

namespace FSBDeath
{
    void HandlerJustDied(Creature* bot, Unit* killer)
    {
        if (!bot || bot->IsAlive())
            return;

        if (!killer)
            return;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return;

        auto& hasSS = baseAI->botHasSoulstone;
        auto& botGroup = baseAI->botLogicalGroup;

        TC_LOG_DEBUG("scripts.fsb.death", "FSB: Death Bot {} JustDied from attacker {}.", bot->GetName(), killer->GetName());

        // handle chatter after death
        if (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
        {
            Player* player = FSBMgr::Get()->GetBotOwner(bot);
            if (player)
            {
                FSBChatter::DemandBotChatter(bot, killer, FSB_ChatterCategory::botDeathHired, FSB_ReplyType::Yell, FSB_ChatterSource::None);
            }
            else FSBChatter::DemandBotChatter(bot, killer, FSB_ChatterCategory::botDeath, FSB_ReplyType::Yell, FSB_ChatterSource::None);
        }
        
        // handle death with soulstone or Self Resurrect (Shaman)
        if (hasSS)
        {
            TC_LOG_DEBUG("scripts.fsb.death", "FSB: Death Bot {} JustDied with Soulstone and will self resurrect.", bot->GetName());
            bot->AI()->DoAction(FSB_ACTION_SOULSTONE_RESSURECT);
            return;
        }

        // handle death with healer present
        bool healerPresent = FSBGroup::BotGetFirstGroupHealer(botGroup);
        if (healerPresent)
        {
            TC_LOG_DEBUG("scripts.fsb.death", "FSB: Death Bot {} JustDied and is waiting for healer resurrect.", bot->GetName());
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
            
        bot->GetMotionMaster()->MovePoint(2, botCorpse, false);
        TC_LOG_DEBUG("scripts.fsb.death", "FSB: Death Bot {} Started the corpse run from graveyard.", bot->GetName());

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
        TC_LOG_DEBUG("scripts.fsb.death", "FSB: Death Bot {} Revived from Soulstone.", bot->GetName());
    }

    void HandleDeathInDungeon(Creature* bot, float fDistance, float fAngle)
    {
        if (!bot || bot->IsAlive())
            return;

        bot->setDeathState(ALIVE);
        bot->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
        FSBMovement::ResumeFollow(bot, fDistance, fAngle);
        if (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
        {
            FSBChatter::DemandTimedReply(bot, nullptr, FSB_ChatterCategory::botRevived, FSB_ReplyType::Say, FSB_ChatterSource::Bot);
        }
        TC_LOG_DEBUG("scripts.fsb.death", "FSB: Death Bot {} Revived at dungeon entrance.", bot->GetName());
    }

    void BotSetStateAfterCorpseRevive(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return;

        bot->RemoveAllAuras();
        bot->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
        bot->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
        bot->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
        if (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
        {
            FSBChatter::DemandTimedReply(bot, nullptr, FSB_ChatterCategory::botRevived, FSB_ReplyType::Say, FSB_ChatterSource::Bot);
        }
        TC_LOG_DEBUG("scripts.fsb.death", "FSB: Death Bot {} Revived at corpse location after graveyard run.", bot->GetName());
    }

    bool CheckBotMemberDeath(Creature* bot)
    {
        if (!bot)
            return false;

        if (!bot->IsAlive())
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        auto& botGroup = baseAI->botLogicalGroup;
        auto& botSayMemberDead = baseAI->botSayMemberDead;

        if (!baseAI->botResurrectTargetGuid.IsEmpty())
            return false;

        if (baseAI->botSayMemberDead)
            return false;

        Unit* deadTarget = FSBGroup::BotGetFirstDeadMember(botGroup);

        if (!deadTarget || !deadTarget->IsInWorld() || deadTarget->IsDuringRemoveFromWorld())
            return false;

        // Announce death (only once)
        if (!botSayMemberDead && bot->IsAlive() &&
            urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
        {
            std::string chatter = FSBChatter::GetRandomReply(bot, deadTarget, FSB_ChatterCategory::botMemberDied, FSBMgr::Get()->GetBotChatterTypeForEntry(bot->GetEntry()), 0);
            bot->Yell(chatter, LANG_UNIVERSAL);
            FSBChatter::DemandTimedReply(bot, deadTarget, FSB_ChatterCategory::botMemberDied, FSB_ReplyType::Yell, FSB_ChatterSource::Bot);
            baseAI->botSayMemberDead = true;
            TC_LOG_DEBUG("scripts.fsb.death", "FSB: Death Bot {} announced dead unit {}", bot->GetName(), deadTarget->GetName());
        }

        baseAI->botResurrectTargetGuid = deadTarget->GetGUID();

        TC_LOG_DEBUG("scripts.fsb.death", "FSB: Death Bot {} found dead unit {} for resurrection", bot->GetName(), deadTarget->GetName());
        return true;
    }

    void HandleSpellResurrection(Creature* bot, uint32 spellId)
    {
        if (!bot || bot->IsAlive())
            return;

        switch (spellId)
        {
        case SPELL_SHAMAN_ANCESTRAL_SPIRIT:
        case SPELL_MONK_RESUSCITATE:
        case SPELL_PALADIN_REDEMPTION:
        case SPELL_PRIEST_RESURRECTION:
        case SPELL_DRUID_REVIVE:
        case SPELL_DRUID_REBIRTH:
            FSBEvents::ScheduleBotEvent(bot, FSB_EVENT_HIRED_SPELL_RESURRECT_STATE, 3s, 5s);
            TC_LOG_DEBUG("scripts.fsb.death", "FSB: Death Bot {} was resurrected by spell {}", bot->GetName(), FSBSpellsUtils::GetSpellName(spellId));
            break;
        default:
            break;
        }
    }

    void HandleSpellResurrectionDelayedAction(Creature* bot)
    {
        if (!bot || bot->IsAlive())
            return;

        bool isCombatRes = FSBCombatUtils::IsCombatActive(bot);

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
