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

#include "Followship_bots_buffs_handler.h"

#include <algorithm>

#include "Battleground.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "SpellHistory.h"
#include "SpellInfo.h"

#include "Followship_bots_ai_base.h"
#include "Followship_bots_config.h"
#include "Followship_bots_druid.h"
#include "Followship_bots_group_handler.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_paladin.h"
#include "Followship_bots_rogue.h"
#include "Followship_bots_shaman.h"
#include "Followship_bots_utils.h"
#include "Followship_bots_warrior.h"
#include "Followship_bots_warlock.h"
#include "GenAI_chatter_prompts.h"

namespace FSBBuffs
{
    std::vector<Unit*> ResolveBuffTargets(Creature* bot, FSBBuffTargetScope scope)
    {
        std::vector<Unit*> targets;

        switch (scope)
        {
        case FSBBuffTargetScope::Self:
            targets.push_back(bot);
            break;

        case FSBBuffTargetScope::Group:
        {
            auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
            if (!baseAI || !baseAI->botHired)
                break;

            float range = 30.0f;
            auto group = FSBGroup::ResolveGroup(bot, baseAI->botLogicalGroup);
            for (Unit* member : group)
            {
                if (!member || !member->IsAlive())
                    continue;

                if (!bot->IsWithinDistInMap(member, range))
                    continue;

                if (!bot->IsWithinLOSInMap(member))
                    continue;

                targets.push_back(member);
            }
            break;
        }

        case FSBBuffTargetScope::BGAllies:
        {
            BattlegroundMap* bgMap = bot->GetMap()->ToBattlegroundMap();
            if (!bgMap)
                break;

            Battleground* bg = bgMap->GetBG();
            if (!bg)
                break;

            Team botTeam = FSBUtils::GetTeamFromFSBRace(FSBMgr::Get()->GetBotRaceForEntry(bot->GetEntry()));
            if (botTeam != ALLIANCE && botTeam != HORDE)
                break;

            float range = 30.0f;

            // Same-faction players in the battleground
            for (auto const& [guid, bgPlayer] : bg->GetPlayers())
            {
                if (bgPlayer.Team != botTeam)
                    continue;

                Player* player = ObjectAccessor::GetPlayer(bgMap, guid);
                if (!player || !player->IsAlive())
                    continue;

                if (!bot->IsWithinDistInMap(player, range))
                    continue;

                if (!bot->IsWithinLOSInMap(player))
                    continue;

                targets.push_back(player);
            }

            // Same-faction bots in the battleground
            for (auto const& [guid, creature] : bgMap->GetObjectsStore().Data.Head)
            {
                if (!creature || !creature->IsBot())
                    continue;

                Team creatureTeam = FSBUtils::GetTeamFromFSBRace(FSBMgr::Get()->GetBotRaceForEntry(creature->GetEntry()));
                if (creatureTeam != botTeam)
                    continue;

                if (!creature->IsAlive())
                    continue;

                if (!bot->IsWithinDistInMap(creature, range))
                    continue;

                if (!bot->IsWithinLOSInMap(creature))
                    continue;

                targets.push_back(creature);
            }

            // Add self if not already there
            if (std::find(targets.begin(), targets.end(), bot) == targets.end())
                targets.push_back(bot);
            break;
        }
        }

        return targets;
    }

    bool BotCastPaladinBeacon(Creature* bot, FSBBuffTargetScope scope, uint32& outSpellId)
    {
        outSpellId = 0;
        if (!bot || !bot->IsAlive())
            return false;

        if (scope == FSBBuffTargetScope::BGAllies)
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        FSB_Class botClass = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());
        if (botClass != FSB_Class::Paladin)
            return false;

        uint32 spellId = SPELL_PALADIN_BEACON_OF_LIGHT;
        Unit* target = nullptr;
        ObjectGuid botGuid = bot->GetGUID();

        // Beacon is unique per caster. Track the target explicitly so recasting does not remove
        // the buff from the current target and put it on a higher priority target in a loop.
        if (!baseAI->botSpellsData.beaconTargetGuid.IsEmpty())
        {
            Unit* existingTarget = ObjectAccessor::GetUnit(*bot, baseAI->botSpellsData.beaconTargetGuid);
            if (existingTarget && existingTarget->HasAura(spellId, botGuid))
            {
                TC_LOG_DEBUG("scripts.fsb.buffs", "FSB: Paladin bot {} Beacon still active on {}, skip",
                    bot->GetName(), existingTarget->GetName());
                return false;
            }
            baseAI->botSpellsData.beaconTargetGuid.Clear();
        }

        if (scope == FSBBuffTargetScope::Group)
        {
            Unit* tank = FSBGroup::BotGetFirstGroupTank(bot);
            Player* owner = FSBMgr::Get()->GetBotOwner(bot);

            if (tank && tank->IsAlive() && !tank->HasAura(spellId) &&
                bot->IsWithinDistInMap(tank, 30.0f) && bot->IsWithinLOSInMap(tank))
                target = tank;
            else if (owner && owner->IsAlive() && !owner->HasAura(spellId) &&
                bot->IsWithinDistInMap(owner, 30.0f) && bot->IsWithinLOSInMap(owner))
                target = owner;
            else if (!bot->HasAura(spellId))
                target = bot;
        }
        else if (scope == FSBBuffTargetScope::Self)
        {
            if (!bot->HasAura(spellId))
                target = bot;
        }

        if (!target)
            return false;

        SpellCastResult result = bot->CastSpell(target, spellId, false);
        if (result != SPELL_CAST_OK)
            return false;

        baseAI->botGlobalCooldown = getMSTime() + 1500;
        baseAI->botSpellsData.beaconTargetGuid = target->GetGUID();
        outSpellId = spellId;
        TC_LOG_DEBUG("scripts.fsb.buffs", "FSB: Paladin bot {} cast Beacon ({}) on {}",
            bot->GetName(), spellId, target->GetName());
        return true;
    }

    bool BotCastWarlockSoulstone(Creature* bot, FSBBuffTargetScope scope, uint32& outSpellId)
    {
        outSpellId = 0;
        if (!bot || !bot->IsAlive())
            return false;

        if (scope == FSBBuffTargetScope::BGAllies)
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        FSB_Class botClass = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());
        if (botClass != FSB_Class::Warlock)
            return false;

        uint32 spellId = SPELL_WARLOCK_SOULSTONE;
        if (bot->GetSpellHistory()->HasCooldown(spellId))
            return false;

        Unit* target = nullptr;
        ObjectGuid botGuid = bot->GetGUID();

        // Soulstone is unique per caster. Track the target explicitly so recasting does not remove
        // the buff from the current target and put it on a higher priority target in a loop.
        if (!baseAI->botSpellsData.soulstoneTargetGuid.IsEmpty())
        {
            Unit* existingTarget = ObjectAccessor::GetUnit(*bot, baseAI->botSpellsData.soulstoneTargetGuid);
            if (existingTarget && existingTarget->HasAura(spellId, botGuid))
            {
                TC_LOG_DEBUG("scripts.fsb.buffs", "FSB: Warlock bot {} Soulstone still active on {}, skip",
                    bot->GetName(), existingTarget->GetName());
                return false;
            }
            baseAI->botSpellsData.soulstoneTargetGuid.Clear();
        }

        if (scope == FSBBuffTargetScope::Group)
        {
            Unit* healer = FSBGroup::BotGetFirstGroupHealer(bot);
            Player* owner = FSBMgr::Get()->GetBotOwner(bot);

            if (healer && healer->IsAlive() && !healer->HasAura(spellId) &&
                bot->IsWithinDistInMap(healer, 30.0f) && bot->IsWithinLOSInMap(healer))
                target = healer;
            else if (owner && owner->IsAlive() && !owner->HasAura(spellId) &&
                bot->IsWithinDistInMap(owner, 30.0f) && bot->IsWithinLOSInMap(owner))
                target = owner;
            else if (!bot->HasAura(spellId))
                target = bot;
        }
        else if (scope == FSBBuffTargetScope::Self)
        {
            if (!bot->HasAura(spellId))
                target = bot;
        }

        if (!target)
            return false;

        SpellCastResult result = bot->CastSpell(target, spellId, false);
        if (result != SPELL_CAST_OK)
            return false;

        baseAI->botGlobalCooldown = getMSTime() + 1500;
        baseAI->botSpellsData.soulstoneTargetGuid = target->GetGUID();
        outSpellId = spellId;
        TC_LOG_DEBUG("scripts.fsb.buffs", "FSB: Warlock bot {} cast Soulstone ({}) on {}",
            bot->GetName(), spellId, target->GetName());
        return true;
    }

    bool BotCastRogueBuffs(Creature* bot, FSBBuffTargetScope scope, uint32& outSpellId)
    {
        outSpellId = 0;
        if (!bot || !bot->IsAlive())
            return false;

        if (scope != FSBBuffTargetScope::Self)
            return false;

        FSB_Class botClass = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());
        if (botClass != FSB_Class::Rogue)
            return false;

        if (bot->IsInCombat())
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        uint32 now = getMSTime();

        auto tryCastSelfBuff = [&](uint32 spellId) -> bool
        {
            SpellCastResult result = bot->CastSpell(bot, spellId, false);
            if (result != SPELL_CAST_OK)
                return false;

            baseAI->botGlobalCooldown = now + 1500;
            outSpellId = spellId;
            return true;
        };

        auto hasAnyAura = [&](std::vector<uint32> const& spells) -> bool
        {
            for (uint32 spellId : spells)
                if (bot->HasAura(spellId))
                    return true;
            return false;
        };

        std::vector<uint32> lethalPoisons = {
            SPELL_ROGUE_INSTANT_POISON,
            SPELL_ROGUE_WOUND_POISON,
            SPELL_ROGUE_DEADLY_POISON,
            SPELL_ROGUE_AMPLIFYING_POISON
        };

        if (!hasAnyAura(lethalPoisons))
        {
            uint32 spellId = lethalPoisons[urand(0, uint32(lethalPoisons.size()) - 1)];
            if (tryCastSelfBuff(spellId))
            {
                TC_LOG_DEBUG("scripts.fsb.buffs", "FSB: Rogue bot {} cast lethal poison ({})",
                    bot->GetName(), spellId);
                return true;
            }
            return false;
        }

        std::vector<uint32> nonLethalPoisons = {
            SPELL_ROGUE_CRIPPLING_POISON,
            SPELL_ROGUE_NUMBING_POISON,
            SPELL_ROGUE_ATROPHIC_POISON
        };

        if (!hasAnyAura(nonLethalPoisons))
        {
            uint32 spellId = nonLethalPoisons[urand(0, uint32(nonLethalPoisons.size()) - 1)];
            if (tryCastSelfBuff(spellId))
            {
                TC_LOG_DEBUG("scripts.fsb.buffs", "FSB: Rogue bot {} cast non-lethal poison ({})",
                    bot->GetName(), spellId);
                return true;
            }
            return false;
        }

        BattlegroundMap* bgMap = bot->GetMap()->ToBattlegroundMap();
        if (!bgMap)
        {
            TC_LOG_DEBUG("scripts.fsb.buffs", "FSB: Rogue bot {} skip Stealth - not in battleground map", bot->GetName());
            return false;
        }

        Battleground* bg = bgMap->GetBG();
        if (!bg || bg->GetStatus() != STATUS_IN_PROGRESS)
        {
            TC_LOG_DEBUG("scripts.fsb.buffs", "FSB: Rogue bot {} skip Stealth - BG not in progress", bot->GetName());
            return false;
        }

        if (bot->HasAura(SPELL_ROGUE_STEALTH))
        {
            TC_LOG_DEBUG("scripts.fsb.buffs", "FSB: Rogue bot {} skip Stealth - already stealthed", bot->GetName());
            return false;
        }

        if (tryCastSelfBuff(SPELL_ROGUE_STEALTH))
        {
            TC_LOG_DEBUG("scripts.fsb.buffs", "FSB: Rogue bot {} cast Stealth in battleground",
                bot->GetName());
            return true;
        }

        TC_LOG_DEBUG("scripts.fsb.buffs", "FSB: Rogue bot {} skip Stealth - cast failed", bot->GetName());
        return false;
    }

    bool BotCastGenericBuff(Creature* bot, FSBBuffTargetScope scope, uint32& outSpellId)
    {
        outSpellId = 0;
        if (!bot || !bot->IsAlive())
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        FSB_Class botClass = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());

        // Druid bypasses generic path.
        if (botClass == FSB_Class::Druid)
            return BotCastDruidBuffs(bot, scope, outSpellId);

        if (botClass == FSB_Class::Paladin)
            if (BotCastPaladinBeacon(bot, scope, outSpellId))
                return true;

        if (botClass == FSB_Class::Warlock)
            if (BotCastWarlockSoulstone(bot, scope, outSpellId))
                return true;

        if (botClass == FSB_Class::Rogue)
            if (BotCastRogueBuffs(bot, scope, outSpellId))
                return true;

        FSBSpellQuery query;
        query.type = FSBSpellType::Buff;
        query.allowedClass = botClass;
        query.roleMask = RoleToMask(FSBMgr::Get()->GetRole(bot));
        query.includeSelfCast = (scope == FSBBuffTargetScope::Self);
        query.respectCooldown = true;
        query.excludeAlreadyPresentAura = true;

        std::vector<Unit*> targets = ResolveBuffTargets(bot, scope);

        Unit* tank = (scope == FSBBuffTargetScope::Group)
            ? FSBGroup::BotGetFirstGroupTank(bot)
            : nullptr;
        Player* owner = FSBMgr::Get()->GetBotOwner(bot);

        for (Unit* target : targets)
        {
            if (!target)
                continue;

            query.target = target;

            auto spells = FSBSpellsDB::QuerySpells(query, bot);
            if (spells.empty())
                continue;

            for (FSBSpellDefinition const* def : spells)
            {
                if (!def)
                    continue;

                if (def->spellId == SPELL_SHAMAN_EARTH_SHIELD)
                {
                    bool isTank = tank && target->GetGUID() == tank->GetGUID();
                    bool isOwner = owner && target->GetGUID() == owner->GetGUID();
                    if (!isTank && !isOwner)
                        continue;
                }

                if (def->spellId == SPELL_WARRIOR_BATTLE_SHOUT && scope == FSBBuffTargetScope::BGAllies)
                    continue;

                if (target->HasAura(def->spellId))
                    continue;

                SpellCastResult result = bot->CastSpell(target, def->spellId, false);
                if (result == SPELL_CAST_OK)
                {
                    uint32 now = getMSTime();
                    baseAI->botGlobalCooldown = now + 1500;
                    outSpellId = def->spellId;

                    TC_LOG_DEBUG("scripts.fsb.buffs", "FSB: Bot {} cast generic buff {} on {}",
                        bot->GetName(), def->spellId, target->GetName());

                    return true;
                }
            }
        }

        return false;
    }

    bool BotCastDruidBuffs(Creature* bot, FSBBuffTargetScope scope, uint32& outSpellId)
    {
        if (!bot || !bot->IsAlive())
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        FSB_Roles role = FSBMgr::Get()->GetRole(bot);
        uint32 desiredForm = 0;
        switch (role)
        {
        case FSB_ROLE_TANK:        desiredForm = SPELL_DRUID_BEAR; break;
        case FSB_ROLE_MELEE_DAMAGE: desiredForm = SPELL_DRUID_CAT; break;
        case FSB_ROLE_HEALER:      desiredForm = SPELL_DRUID_TREE_OF_LIFE; break;
        case FSB_ROLE_RANGED_DAMAGE: desiredForm = SPELL_DRUID_MOONKIN; break;
        default: break;
        }

        // Remove shapeshift forms before casting buffs.
        uint32 formsToRemove[] = {
            SPELL_DRUID_BEAR, SPELL_DRUID_CAT, SPELL_DRUID_TRAVEL,
            SPELL_DRUID_MOONKIN, SPELL_DRUID_TREE_OF_LIFE
        };
        for (uint32 form : formsToRemove)
        {
            if (bot->HasAura(form))
                bot->RemoveAurasDueToSpell(form);
        }

        // Now run the generic logic with Druid class filter.
        FSBSpellQuery query;
        query.type = FSBSpellType::Buff;
        query.allowedClass = FSB_Class::Druid;
        query.roleMask = RoleToMask(role);
        query.includeSelfCast = (scope == FSBBuffTargetScope::Self);
        query.respectCooldown = true;
        query.excludeAlreadyPresentAura = true;

        std::vector<Unit*> targets = ResolveBuffTargets(bot, scope);
        for (Unit* target : targets)
        {
            if (!target)
                continue;

            query.target = target;
            auto spells = FSBSpellsDB::QuerySpells(query, bot);
            for (FSBSpellDefinition const* def : spells)
            {
                if (target->HasAura(def->spellId))
                    continue;

                SpellCastResult result = bot->CastSpell(target, def->spellId, false);
                if (result == SPELL_CAST_OK)
                {
                    uint32 now = getMSTime();
                    baseAI->botGlobalCooldown = now + 1500;
                    outSpellId = def->spellId;

                    TC_LOG_DEBUG("scripts.fsb.buffs", "FSB: Druid bot {} cast buff {} on {}",
                        bot->GetName(), def->spellId, target->GetName());

                    if (desiredForm && !bot->HasAura(desiredForm))
                        bot->CastSpell(bot, desiredForm, false);

                    return true;
                }
            }
        }

        return false;
    }

    bool HandlePeriodicBuffs(Creature* bot, uint32& selfBuffTimer, uint32& buffTimer)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (bot->GetStandState() == UNIT_STAND_STATE_SIT)
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI || baseAI->botDoingRandomEvent)
            return false;

        uint32 now = getMSTime();

        // Path 1: self buffs.
        if (now >= selfBuffTimer)
        {
            uint32 spellId = 0;
            if (BotCastGenericBuff(bot, FSBBuffTargetScope::Self, spellId))
            {
                if (spellId && urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
                    FSBGenAIPrompts::DispatchBotBuff(bot, bot->GetGUID(), spellId);

                return true;
            }

            selfBuffTimer = now + 60000;
        }

        // Paths 2 & 3 share buffTimer: group first, then BG allies.
        bool socialBuffAttempted = false;

        if (baseAI->botHired && now >= buffTimer)
        {
            socialBuffAttempted = true;

            uint32 spellId = 0;
            if (BotCastGenericBuff(bot, FSBBuffTargetScope::Group, spellId))
            {
                if (spellId && urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
                    FSBGenAIPrompts::DispatchBotBuff(bot, bot->GetGUID(), spellId);

                return true;
            }
        }

        if (bot->GetMap()->IsBattleground() && now >= buffTimer)
        {
            socialBuffAttempted = true;

            uint32 spellId = 0;
            if (BotCastGenericBuff(bot, FSBBuffTargetScope::BGAllies, spellId))
            {
                if (spellId && urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
                    FSBGenAIPrompts::DispatchBotBuff(bot, bot->GetGUID(), spellId);

                return true;
            }
        }

        if (socialBuffAttempted)
            buffTimer = now + 60000;

        return false;
    }
}
