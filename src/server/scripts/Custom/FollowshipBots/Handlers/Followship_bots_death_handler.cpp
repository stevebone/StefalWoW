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

#include "Creature.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ScriptedCreature.h"
#include "SpellInfo.h"

#include "Followship_bots.h"
#include "Followship_bots_config.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_utils.h"

#include "Followship_bots_chatter_handler.h"
#include "Followship_bots_death_handler.h"
#include "Followship_bots_dungeon_handler.h"
#include "Followship_bots_events_handler.h"
#include "Followship_bots_group_handler.h"
#include "Followship_bots_movement_handler.h"
#include "Followship_bots_spells_handler.h"
#include "Followship_bots_teleport_handler.h"
#include "Followship_bots_utils.h"

using namespace FSBUtils;

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

        // Ensure group is built before checking for healer
        FSBGroup::BuildLogicalBotGroup(bot, botGroup);

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
        Unit* healer = FSBGroup::BotGetFirstGroupHealer(botGroup);
        if (healer)
        {
            TC_LOG_DEBUG("scripts.fsb.death", "FSB: Death Bot {} JustDied, found healer: {}", bot->GetName(), healer->GetName());
            AddToHealerResurrectQueue(bot, healer->ToCreature());
            bot->AI()->DoAction(FSB_ACTION_WAIT_HEALER_RESSURECT);
            return;
        }

        // handle death with graveyard teleport
        if (!bot->GetMap()->IsDungeon())
        {
            FSBEvents::ScheduleBotEvent(bot, FSB_EVENT_GENERIC_TELEPORT_GRAVEYARD, 4s, 6s);
            return;
        }
        else bot->AI()->DoAction(FSB_ACTION_TELEPORT_DUNGEON);
        
    }

    void HandleDeathWithGraveyard(Creature* bot, Position botCorpse)
    {
        if (!bot || bot->IsAlive())
            return;

        bot->setDeathState(ALIVE);
        bot->SetHealth(1);

        bot->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
        bot->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);

        bot->GetMotionMaster()->Clear();
        bot->GetMotionMaster()->MovePoint(FSB_MOVEMENT_POINT_CORPSE, botCorpse, false);
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

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (baseAI)
        {
            // Resume follow movement
            FSBMovement::ResumeFollow(bot, baseAI->botFollowDistance, baseAI->botFollowAngle);

            // Set flag for healer classes to check dead units after arriving at player
            if (BotIsHealerClass(bot))
                baseAI->botNeedsDeadUnitCheck = true;
        }

        TC_LOG_DEBUG("scripts.fsb.death", "FSB: Death Bot {} Revived from Soulstone.", bot->GetName());
    }

    void HandleDeathInDungeon(Creature* bot, float fDistance, float fAngle)
    {
        if (!bot || bot->IsAlive())
            return;

        bot->GetThreatManager().ResetAllThreat();

        bot->setDeathState(ALIVE);
        bot->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
        
        // Restore health and mana to proper values
        uint64 maxHealth = bot->GetMaxHealth();
        uint64 maxMana = bot->GetMaxPower(POWER_MANA);
        bot->SetHealth(maxHealth * 0.35f);
        if (maxMana > 1)
            bot->SetPower(POWER_MANA, maxMana * 0.35f);
        
        // Remove any death-related auras
        bot->RemoveAllAuras();

        FSBMovement::ResumeFollow(bot, fDistance, fAngle);

        // Set flag for resurrect-capable classes to check dead units after arriving at player
        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (baseAI && BotIsHealerClass(bot))
        {
            baseAI->botNeedsDeadUnitCheck = true;
        }

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

    void AddToHealerResurrectQueue(Unit* deadUnit, Creature* healer)
    {
        if (!deadUnit || !healer)
            return;

        auto healerAI = dynamic_cast<FSB_BaseAI*>(healer->AI());
        if (!healerAI)
            return;

        // Add dead unit GUID to healer's queue
        healerAI->botResurrectQueue.push(deadUnit->GetGUID());
        TC_LOG_DEBUG("scripts.fsb.death", "FSB: AddToHealerResurrectQueue Added dead unit {} to healer {} resurrect queue", deadUnit->GetName(), healer->GetName());

        // Chatter announcement (once when added to queue)
        if (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
        {
            std::string chatter = FSBChatter::GetRandomReply(healer, deadUnit, FSB_ChatterCategory::botMemberDied, FSBMgr::Get()->GetBotChatterTypeForEntry(healer->GetEntry()), 0);
            healer->Yell(chatter, LANG_UNIVERSAL);
            FSBChatter::DemandTimedReply(healer, deadUnit, FSB_ChatterCategory::botMemberDied, FSB_ReplyType::Yell, FSB_ChatterSource::Bot);
        }

        // Schedule resurrect event
        FSBEvents::ScheduleBotEvent(healer, FSB_EVENT_HIRED_RESURRECT_TARGET, 3s, 5s);
    }

    void ProcessResurrectQueue(Creature* healer)
    {
        if (!healer || !healer->IsAlive())
            return;

        auto healerAI = dynamic_cast<FSB_BaseAI*>(healer->AI());
        if (!healerAI)
            return;

        auto& resurrectQueue = healerAI->botResurrectQueue;
        if (resurrectQueue.empty())
            return;

        // Check if healer can resurrect (out of combat, or Druid for combat res)
        bool canCombatRes = healerAI->botClass == FSB_Class::Druid;
        if (healer->IsInCombat() && !canCombatRes)
        {
            FSBEvents::ScheduleBotEvent(healer, FSB_EVENT_HIRED_RESURRECT_TARGET, 5s);
            return;
        }

        if (healer->HasUnitState(UNIT_STATE_CASTING))
        {
            FSBEvents::ScheduleBotEvent(healer, FSB_EVENT_HIRED_RESURRECT_TARGET, 2s);
            return;
        }

        // Get next target from queue
        ObjectGuid targetGuid = resurrectQueue.front();
        Unit* target = ObjectAccessor::GetUnit(*healer, targetGuid);

        // Validate target
        if (!target || target->IsAlive() || !target->IsInWorld() || target->IsDuringRemoveFromWorld())
        {
            resurrectQueue.pop();
            if (!resurrectQueue.empty())
                FSBEvents::ScheduleBotEvent(healer, FSB_EVENT_HIRED_RESURRECT_TARGET, 1s);
            return;
        }

        // Check distance
        if (healer->GetMapId() == target->GetMapId() && healer->GetDistance(target) > 30.0f)
        {
            healer->GetMotionMaster()->Clear();
            healer->GetMotionMaster()->MoveCloserAndStop(4, target, 28.f);
            FSBEvents::ScheduleBotEvent(healer, FSB_EVENT_HIRED_RESURRECT_TARGET, 5s);
            return;
        }

        // Determine spell based on class
        uint32 spellId = 0;
        switch (healerAI->botClass)
        {
        case FSB_Class::Priest:
            spellId = SPELL_PRIEST_RESURRECTION;
            break;
        case FSB_Class::Druid:
            if (canCombatRes && healer->IsInCombat())
                spellId = SPELL_DRUID_REBIRTH;
            else
                spellId = SPELL_DRUID_REVIVE;
            break;
        case FSB_Class::Monk:
            spellId = SPELL_MONK_RESUSCITATE;
            break;
        case FSB_Class::Paladin:
            spellId = SPELL_PALADIN_REDEMPTION;
            break;
        case FSB_Class::Shaman:
            spellId = SPELL_SHAMAN_ANCESTRAL_SPIRIT;
            break;
        default:
            resurrectQueue.pop();
            if (!resurrectQueue.empty())
                FSBEvents::ScheduleBotEvent(healer, FSB_EVENT_HIRED_RESURRECT_TARGET, 1s);
            return;
        }

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (!spellInfo)
        {
            resurrectQueue.pop();
            if (!resurrectQueue.empty())
                FSBEvents::ScheduleBotEvent(healer, FSB_EVENT_HIRED_RESURRECT_TARGET, 1s);
            return;
        }

        int32 castTimeMs = spellInfo->CalcCastTime();
        auto costs = spellInfo->CalcPowerCost(healer, spellInfo->GetSchoolMask());
        uint32 manaCost = 0;
        if (!costs.empty())
        {
            auto manaIt = std::find_if(costs.begin(), costs.end(), [](SpellPowerCost const& cost) { return cost.Power == POWER_MANA; });
            if (manaIt != costs.end())
                manaCost = manaIt->Amount;
            else
                manaCost = costs[0].Amount;
        }
        uint32 currentMana = healer->GetPower(POWER_MANA);

        // Check if healer has enough mana
        if (currentMana < manaCost)
        {
            FSBEvents::ScheduleBotEvent(healer, FSB_EVENT_HIRED_RESURRECT_TARGET, 10s);
            return;
        }

        // Cast resurrection on dead target
        if (FSBSpells::BotCastSpellOnDeadTarget(healer, spellId, target))
        {
            uint32 now = getMSTime();
            healerAI->botGlobalCooldown = now + 1500;
            resurrectQueue.pop();

            TC_LOG_DEBUG("scripts.fsb.death", "FSB: ProcessResurrectQueue Healer {} resurrected {}", healer->GetName(), target->GetName());

            if (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
            {
                FSBChatter::DemandBotChatter(healer, target, FSB_ChatterCategory::botRevivedTarget, FSB_ReplyType::Say, FSB_ChatterSource::None, 0);
            }

            FSBEvents::ScheduleBotEvent(healer, FSB_EVENT_HIRED_RESUME_FOLLOW, std::chrono::milliseconds(castTimeMs + 1000));

            if (!resurrectQueue.empty())
                FSBEvents::ScheduleBotEvent(healer, FSB_EVENT_HIRED_RESURRECT_TARGET, std::chrono::milliseconds(castTimeMs + 2000));
        }
        else
        {
            FSBEvents::ScheduleBotEvent(healer, FSB_EVENT_HIRED_RESURRECT_TARGET, 5s);
        }
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
