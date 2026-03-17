/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// Combat/ThreatManager.h removed - not used in this file
#include "Action.h"
#include "BotAI.h"
#include "Core/PlayerBotHelpers.h"
#include "Player.h"
#include "Unit.h"
#include "Object.h"
#include "Item.h"
#include "Group.h"
#include "MotionMaster.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "Spell.h"
#include "SpellHistory.h"
#include "Log.h"
#include "Chat.h"
#include "GridNotifiers.h"
#include "../../Spatial/SpatialGridManager.h"  // Lock-free spatial grid for deadlock fix
#include "CellImpl.h"
#include "ObjectAccessor.h"
#include "DBCEnums.h"
#include "SharedDefines.h"
#include "CommonActions.h"
#include "../ClassAI/SpellValidation_WoW120.h"
#include "../ClassAI/SpellValidation_WoW120_Part2.h"

namespace Playerbot
{

// Action implementation
Action::Action(::std::string const& name) : _name(name)
{
    _lastExecution = ::std::chrono::steady_clock::now();
}

bool Action::IsOnCooldown() const
{
    if (GetCooldown() <= 0.0f)
        return false;

    auto now = ::std::chrono::steady_clock::now();
    auto elapsed = ::std::chrono::duration_cast<::std::chrono::milliseconds>(now - _lastExecution);
    return elapsed.count() < GetCooldown();
}

void Action::AddPrerequisite(::std::shared_ptr<Action> action)
{
    if (action)
        _prerequisites.push_back(action);
}

float Action::GetSuccessRate() const
{
    uint32 executions = _executionCount.load();
    if (executions == 0)
        return 0.0f;

    uint32 successes = _successCount.load();
    return static_cast<float>(successes) / static_cast<float>(executions);
}

// Helper methods
bool Action::CanCast(BotAI* ai, uint32 spellId, ::Unit* target) const
{
    if (!ai)
        return false;

    Player* bot = ai->GetBot();
    if (!bot)
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    // Check if bot can cast this spell
    if (!bot->HasSpell(spellId))
        return false;

    // Check mana/energy requirements
    ::std::vector<SpellPowerCost> costs = spellInfo->CalcPowerCost(bot, spellInfo->GetSchoolMask());
    for (SpellPowerCost const& cost : costs)
    {
        if (bot->GetPower(cost.Power) < cost.Amount)
            return false;
    }

    // Check cooldown using TrinityCore's SpellHistory API
    if (bot->GetSpellHistory()->HasCooldown(spellId))
        return false;

    // Check if target is valid (if spell requires target)
    if (spellInfo->IsTargetingArea() && !target)
        return false;

    // Check range if target specified
    if (target)
    {
        float range = spellInfo->GetMaxRange();
        float rangeSq = range * range;
        if (bot->GetExactDistSq(target) > rangeSq)
            return false;
    }

    return true;
}

bool Action::DoCast(BotAI* ai, uint32 spellId, ::Unit* target)
{
    if (!CanCast(ai, spellId, target))
        return false;

    Player* bot = ai->GetBot();
    if (!bot)
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    // Cast the spell
    if (target)
        bot->CastSpell(CastSpellTargetArg(target), spellId);
    else
        bot->CastSpell(CastSpellTargetArg(bot), spellId);

    return true;
}

bool Action::DoMove(BotAI* ai, float x, float y, float z)
{
    if (!ai)
        return false;

    Player* bot = ai->GetBot();
    if (!bot)
        return false;

    Position dest(x, y, z, 0.0f);
    if (BotAI* ai = GetBotAI(bot))
    {
        if (ai->MoveTo(dest, true))
            return true;
        // Fallback to legacy if validation fails
    }

    // Non-bot player or validation failed - use standard movement
    bot->GetMotionMaster()->MovePoint(0, x, y, z);
    return true;
}

bool Action::DoSay(BotAI* ai, ::std::string const& text)
{
    if (!ai || text.empty())
        return false;

    Player* bot = ai->GetBot();
    if (!bot)
        return false;

    bot->Say(text, LANG_UNIVERSAL);
    return true;
}

bool Action::DoEmote(BotAI* ai, uint32 emoteId)
{
    if (!ai)
        return false;

    Player* bot = ai->GetBot();
    if (!bot)
        return false;

    bot->HandleEmoteCommand(static_cast<Emote>(emoteId));
    return true;
}

bool Action::UseItem(BotAI* ai, uint32 itemId, ::Unit* target)
{
    if (!ai)
        return false;

    Player* bot = ai->GetBot();
    if (!bot)
        return false;

    Item* item = bot->GetItemByEntry(itemId);
    if (!item)
        return false;

    // Use the item with proper SpellCastTargets
    SpellCastTargets targets;
    if (target)
        targets.SetUnitTarget(target);
    else
        targets.SetUnitTarget(bot);

    // WoW 12.0: CastItemUseSpell signature changed to std::array<int32, 3>
    std::array<int32, 3> misc = { 0, 0, 0 };
    bot->CastItemUseSpell(item, targets, ObjectGuid::Empty, misc);
    return true;
}

::Unit* Action::GetNearestEnemy(BotAI* ai, float range) const
{
    if (!ai)
        return nullptr;

    Player* bot = ai->GetBot();
    if (!bot)
        return nullptr;

    ::Unit* nearestEnemy = nullptr;
    float nearestDistanceSq = range * range; // Use squared distance for comparison

    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = bot->GetMap();
    if (!map)
        return nullptr;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return nullptr;
    }

    // Query nearby creature GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        bot->GetPosition(), range);

    // SPATIAL GRID MIGRATION COMPLETE (2025-11-26):
    // ObjectAccessor is intentionally retained here because we need:
    // 1. Real-time IsAlive() check (creature may have died since snapshot)
    // 2. bot->IsHostileTo() requires live Unit* for faction/reputation checks
    // 3. GetExactDistSq() for precise current position comparison
    // The spatial grid pre-filters candidates to reduce ObjectAccessor calls.
    for (ObjectGuid guid : nearbyGuids)
    {
        ::Unit* unit = ObjectAccessor::GetUnit(*bot, guid);
        if (!unit || !unit->IsAlive())
            continue;
        // Check if hostile
    if (!bot->IsHostileTo(unit))
            continue;

        float distanceSq = bot->GetExactDistSq(unit);
        if (distanceSq < nearestDistanceSq)
        {
            nearestDistanceSq = distanceSq;
            nearestEnemy = unit;
        }
    }

    return nearestEnemy;
}

::Unit* Action::GetLowestHealthAlly(BotAI* ai, float range) const
{
    if (!ai)
        return nullptr;

    Player* bot = ai->GetBot();
    if (!bot)
        return nullptr;

    ::Unit* lowestHealthAlly = nullptr;
    float lowestHealthPct = 100.0f;

    // Check group members first
    if (Group* group = bot->GetGroup())
    {
        for (GroupReference const& ref : group->GetMembers())
        {
            if (Player* member = ref.GetSource())
            {
                if (member == bot || !member->IsAlive())
                    continue;

                float rangeSq = range * range;
                if (bot->GetExactDistSq(member) > rangeSq)
                    continue;

                float healthPct = member->GetHealthPct();
                if (healthPct < lowestHealthPct)
                {
                    lowestHealthPct = healthPct;
                    lowestHealthAlly = member;
                }
            }
        }
    }

    return lowestHealthAlly;
}

Player* Action::GetNearestPlayer(BotAI* ai, float range) const
{
    if (!ai)
        return nullptr;

    Player* bot = ai->GetBot();
    if (!bot)
        return nullptr;

    Player* nearestPlayer = nullptr;
    float nearestDistanceSq = range * range; // Use squared distance for comparison

    // Use Map API to find nearby players
    Map* map = bot->GetMap();
    if (map)
    {
        Map::PlayerList const& players = map->GetPlayers();
        for (Map::PlayerList::const_iterator iter = players.begin(); iter != players.end(); ++iter)
        {
            Player* player = iter->GetSource();
            if (!player || player == bot || !player->IsInWorld())
                continue;

            float distanceSq = bot->GetExactDistSq(player);
            float rangeSq = range * range;
            if (distanceSq <= rangeSq && distanceSq < nearestDistanceSq)
            {
                nearestDistanceSq = distanceSq;
                nearestPlayer = player;
            }
        }
    }

    return nearestPlayer;
}

// MovementAction implementation
bool MovementAction::IsPossible(BotAI* ai) const
{
    if (!ai)
        return false;

    Player* bot = ai->GetBot();
    if (!bot)
        return false;

    // Can't move if rooted or stunned
    if (bot->HasUnitState(UNIT_STATE_ROOT) || bot->HasUnitState(UNIT_STATE_STUNNED))
        return false;

    return true;
}

ActionResult MovementAction::Execute(BotAI* ai, ActionContext const& /*context*/)
{
    if (!IsPossible(ai))
        return ActionResult::IMPOSSIBLE;

    // Default movement implementation - derived classes should override
    return ActionResult::SUCCESS;
}

bool MovementAction::GeneratePath(BotAI* ai, float x, float y, float z)
{
    if (!ai)
        return false;

    Player* bot = ai->GetBot();
    if (!bot)
        return false;

    // Basic pathfinding - just move directly for now
    // In a full implementation, this would use proper pathfinding
    _path.clear();
    _path.push_back(G3D::Vector3(bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ()));
    _path.push_back(G3D::Vector3(x, y, z));

    return true;
}

// CombatAction implementation
bool CombatAction::IsUseful(BotAI* ai) const
{
    if (!ai)
        return false;

    Player* bot = ai->GetBot();
    if (!bot)
        return false;

    // Combat actions are useful when in combat or when there are enemies nearby
    return bot->IsInCombat() || GetNearestEnemy(ai, 30.0f) != nullptr;
}

// SpellAction implementation
bool SpellAction::IsPossible(BotAI* ai) const
{
    return CanCast(ai, _spellId);
}

bool SpellAction::IsUseful(BotAI* ai) const
{
    // Check base combat utility
    if (!CombatAction::IsUseful(ai))
        return false;

    // Additional spell-specific checks could go here
    return true;
}

ActionResult SpellAction::Execute(BotAI* ai, ActionContext const& context)
{
    if (!IsPossible(ai))
        return ActionResult::IMPOSSIBLE;

    ::Unit* target = context.target ? context.target->ToUnit() : nullptr;

    if (DoCast(ai, _spellId, target))
    {
        _executionCount++;
        _successCount++;
        _lastExecution = ::std::chrono::steady_clock::now();
        return ActionResult::SUCCESS;
    }

    _executionCount++;
    return ActionResult::FAILED;
}

// ActionFactory implementation
ActionFactory* ActionFactory::instance()
{
    static ActionFactory instance;
    return &instance;
}

void ActionFactory::RegisterAction(::std::string const& name,
                                 ::std::function<::std::shared_ptr<Action>()> creator)
{
    _creators[name] = creator;
}

::std::shared_ptr<Action> ActionFactory::CreateAction(::std::string const& name)
{
    auto it = _creators.find(name);
    if (it != _creators.end())
        return it->second();

    return nullptr;
}

::std::vector<::std::shared_ptr<Action>> ActionFactory::CreateClassActions(uint8 classId, uint8 spec)
{
    ::std::vector<::std::shared_ptr<Action>> actions;
    ChrSpecialization specEnum = static_cast<ChrSpecialization>(spec);

    using namespace WoW120Spells;

    switch (classId)
    {
        case CLASS_WARRIOR:
        {
            // Core warrior abilities
            actions.push_back(::std::make_shared<SpellAction>("charge", Warrior::CHARGE));
            actions.push_back(::std::make_shared<SpellAction>("pummel", Warrior::PUMMEL));
            actions.push_back(::std::make_shared<SpellAction>("heroic_leap", Warrior::HEROIC_LEAP));
            actions.push_back(::std::make_shared<SpellAction>("spell_reflection", Warrior::SPELL_REFLECTION));
            actions.push_back(::std::make_shared<SpellAction>("berserker_rage", Warrior::BERSERKER_RAGE));
            actions.push_back(::std::make_shared<SpellAction>("storm_bolt", Warrior::STORM_BOLT));

            if (specEnum == ChrSpecialization::WarriorArms)
            {
                actions.push_back(::std::make_shared<SpellAction>("mortal_strike", Warrior::Arms::MORTAL_STRIKE));
                actions.push_back(::std::make_shared<SpellAction>("overpower", Warrior::Arms::OVERPOWER));
                actions.push_back(::std::make_shared<SpellAction>("execute", Warrior::Arms::EXECUTE));
                actions.push_back(::std::make_shared<SpellAction>("slam", Warrior::Arms::SLAM));
                actions.push_back(::std::make_shared<SpellAction>("whirlwind", Warrior::Arms::WHIRLWIND));
                actions.push_back(::std::make_shared<SpellAction>("bladestorm", Warrior::Arms::BLADESTORM));
                actions.push_back(::std::make_shared<SpellAction>("colossus_smash", Warrior::Arms::COLOSSUS_SMASH));
                actions.push_back(::std::make_shared<SpellAction>("warbreaker", Warrior::Arms::WARBREAKER));
                actions.push_back(::std::make_shared<SpellAction>("sweeping_strikes", Warrior::Arms::SWEEPING_STRIKES));
                actions.push_back(::std::make_shared<SpellAction>("die_by_the_sword", Warrior::Arms::DIE_BY_THE_SWORD));
                actions.push_back(::std::make_shared<SpellAction>("avatar", Warrior::Arms::AVATAR));
                actions.push_back(::std::make_shared<SpellAction>("rend", Warrior::Arms::REND));
                actions.push_back(::std::make_shared<SpellAction>("thunderous_roar", Warrior::Arms::THUNDEROUS_ROAR));
            }
            else if (specEnum == ChrSpecialization::WarriorFury)
            {
                actions.push_back(::std::make_shared<SpellAction>("bloodthirst", Warrior::Fury::BLOODTHIRST));
                actions.push_back(::std::make_shared<SpellAction>("raging_blow", Warrior::Fury::RAGING_BLOW));
                actions.push_back(::std::make_shared<SpellAction>("rampage", Warrior::Fury::RAMPAGE));
                actions.push_back(::std::make_shared<SpellAction>("execute", Warrior::Fury::EXECUTE));
                actions.push_back(::std::make_shared<SpellAction>("whirlwind", Warrior::Fury::WHIRLWIND));
                actions.push_back(::std::make_shared<SpellAction>("bladestorm", Warrior::Fury::BLADESTORM));
                actions.push_back(::std::make_shared<SpellAction>("recklessness", Warrior::Fury::RECKLESSNESS));
                actions.push_back(::std::make_shared<SpellAction>("enraged_regeneration", Warrior::Fury::ENRAGED_REGENERATION));
                actions.push_back(::std::make_shared<SpellAction>("onslaught", Warrior::Fury::ONSLAUGHT));
                actions.push_back(::std::make_shared<SpellAction>("odyn_fury", Warrior::Fury::ODYN_FURY));
                actions.push_back(::std::make_shared<SpellAction>("thunderous_roar", Warrior::Fury::THUNDEROUS_ROAR));
            }
            else if (specEnum == ChrSpecialization::WarriorProtection)
            {
                actions.push_back(::std::make_shared<SpellAction>("shield_slam", Warrior::Protection::SHIELD_SLAM));
                actions.push_back(::std::make_shared<SpellAction>("thunder_clap", Warrior::Protection::THUNDER_CLAP));
                actions.push_back(::std::make_shared<SpellAction>("revenge", Warrior::Protection::REVENGE));
                actions.push_back(::std::make_shared<SpellAction>("shield_block", Warrior::Protection::SHIELD_BLOCK));
                actions.push_back(::std::make_shared<SpellAction>("ignore_pain", Warrior::Protection::IGNORE_PAIN));
                actions.push_back(::std::make_shared<SpellAction>("demoralizing_shout", Warrior::Protection::DEMORALIZING_SHOUT));
                actions.push_back(::std::make_shared<SpellAction>("last_stand", Warrior::Protection::LAST_STAND));
                actions.push_back(::std::make_shared<SpellAction>("shield_wall", Warrior::Protection::SHIELD_WALL));
                actions.push_back(::std::make_shared<SpellAction>("avatar", Warrior::Protection::AVATAR));
                actions.push_back(::std::make_shared<SpellAction>("shield_charge", Warrior::Protection::SHIELD_CHARGE));
                actions.push_back(::std::make_shared<SpellAction>("thunderous_roar", Warrior::Protection::THUNDEROUS_ROAR));
            }
            break;
        }

        case CLASS_PALADIN:
        {
            // Core paladin abilities
            actions.push_back(::std::make_shared<SpellAction>("rebuke", Paladin::REBUKE));
            actions.push_back(::std::make_shared<SpellAction>("hammer_of_justice", Paladin::HAMMER_OF_JUSTICE));
            actions.push_back(::std::make_shared<SpellAction>("divine_shield", Paladin::DIVINE_SHIELD));
            actions.push_back(::std::make_shared<SpellAction>("blessing_of_freedom", Paladin::BLESSING_OF_FREEDOM));
            actions.push_back(::std::make_shared<SpellAction>("blessing_of_protection", Paladin::BLESSING_OF_PROTECTION));
            actions.push_back(::std::make_shared<SpellAction>("lay_on_hands", Paladin::LAY_ON_HANDS));
            actions.push_back(::std::make_shared<SpellAction>("crusader_strike", Paladin::CRUSADER_STRIKE));
            actions.push_back(::std::make_shared<SpellAction>("judgment", Paladin::JUDGMENT));
            actions.push_back(::std::make_shared<SpellAction>("consecration", Paladin::CONSECRATION));
            actions.push_back(::std::make_shared<SpellAction>("avenging_wrath", Paladin::AVENGING_WRATH));
            actions.push_back(::std::make_shared<SpellAction>("hammer_of_wrath", Paladin::HAMMER_OF_WRATH));

            if (specEnum == ChrSpecialization::PaladinHoly)
            {
                actions.push_back(::std::make_shared<SpellAction>("holy_shock", Paladin::Holy::HOLY_SHOCK));
                actions.push_back(::std::make_shared<SpellAction>("light_of_dawn", Paladin::Holy::LIGHT_OF_DAWN));
                actions.push_back(::std::make_shared<SpellAction>("beacon_of_light", Paladin::Holy::BEACON_OF_LIGHT));
                actions.push_back(::std::make_shared<SpellAction>("aura_mastery", Paladin::Holy::AURA_MASTERY));
                actions.push_back(::std::make_shared<SpellAction>("divine_favor", Paladin::Holy::DIVINE_FAVOR));
                actions.push_back(::std::make_shared<SpellAction>("holy_prism", Paladin::Holy::HOLY_PRISM));
            }
            else if (specEnum == ChrSpecialization::PaladinProtection)
            {
                actions.push_back(::std::make_shared<SpellAction>("avengers_shield", Paladin::Protection::AVENGERS_SHIELD));
                actions.push_back(::std::make_shared<SpellAction>("shield_of_the_righteous", Paladin::Protection::SHIELD_OF_THE_RIGHTEOUS));
                actions.push_back(::std::make_shared<SpellAction>("hammer_of_the_righteous", Paladin::Protection::HAMMER_OF_THE_RIGHTEOUS));
                actions.push_back(::std::make_shared<SpellAction>("ardent_defender", Paladin::Protection::ARDENT_DEFENDER));
                actions.push_back(::std::make_shared<SpellAction>("guardian_of_ancient_kings", Paladin::Protection::GUARDIAN_OF_ANCIENT_KINGS));
                actions.push_back(::std::make_shared<SpellAction>("divine_toll", Paladin::Protection::DIVINE_TOLL));
                actions.push_back(::std::make_shared<SpellAction>("sentinel", Paladin::Protection::SENTINEL));
                actions.push_back(::std::make_shared<SpellAction>("eye_of_tyr", Paladin::Protection::EYE_OF_TYR));
            }
            else if (specEnum == ChrSpecialization::PaladinRetribution)
            {
                actions.push_back(::std::make_shared<SpellAction>("blade_of_justice", Paladin::Retribution::BLADE_OF_JUSTICE));
                actions.push_back(::std::make_shared<SpellAction>("wake_of_ashes", Paladin::Retribution::WAKE_OF_ASHES));
                actions.push_back(::std::make_shared<SpellAction>("templars_verdict", Paladin::Retribution::TEMPLARS_VERDICT));
                actions.push_back(::std::make_shared<SpellAction>("final_verdict", Paladin::Retribution::FINAL_VERDICT));
                actions.push_back(::std::make_shared<SpellAction>("divine_storm", Paladin::Retribution::DIVINE_STORM));
                actions.push_back(::std::make_shared<SpellAction>("execution_sentence", Paladin::Retribution::EXECUTION_SENTENCE));
                actions.push_back(::std::make_shared<SpellAction>("final_reckoning", Paladin::Retribution::FINAL_RECKONING));
                actions.push_back(::std::make_shared<SpellAction>("crusade", Paladin::Retribution::CRUSADE));
            }
            break;
        }

        case CLASS_HUNTER:
        {
            // Core hunter abilities
            actions.push_back(::std::make_shared<SpellAction>("counter_shot", Hunter::COUNTER_SHOT));
            actions.push_back(::std::make_shared<SpellAction>("disengage", Hunter::DISENGAGE));
            actions.push_back(::std::make_shared<SpellAction>("aspect_of_the_turtle", Hunter::ASPECT_OF_THE_TURTLE));
            actions.push_back(::std::make_shared<SpellAction>("exhilaration", Hunter::EXHILARATION));
            actions.push_back(::std::make_shared<SpellAction>("freezing_trap", Hunter::FREEZING_TRAP));
            actions.push_back(::std::make_shared<SpellAction>("tar_trap", Hunter::TAR_TRAP));
            actions.push_back(::std::make_shared<SpellAction>("misdirection", Hunter::MISDIRECTION));
            actions.push_back(::std::make_shared<SpellAction>("feign_death", Hunter::FEIGN_DEATH));
            actions.push_back(::std::make_shared<SpellAction>("kill_shot", Hunter::KILL_SHOT));
            actions.push_back(::std::make_shared<SpellAction>("hunters_mark", Hunter::HUNTERS_MARK));

            if (specEnum == ChrSpecialization::HunterBeastMastery)
            {
                actions.push_back(::std::make_shared<SpellAction>("barbed_shot", Hunter::BeastMastery::BARBED_SHOT));
                actions.push_back(::std::make_shared<SpellAction>("kill_command", Hunter::BeastMastery::KILL_COMMAND));
                actions.push_back(::std::make_shared<SpellAction>("cobra_shot", Hunter::BeastMastery::COBRA_SHOT));
                actions.push_back(::std::make_shared<SpellAction>("bestial_wrath", Hunter::BeastMastery::BESTIAL_WRATH));
                actions.push_back(::std::make_shared<SpellAction>("aspect_of_the_wild", Hunter::BeastMastery::ASPECT_OF_THE_WILD));
                actions.push_back(::std::make_shared<SpellAction>("dire_beast", Hunter::BeastMastery::DIRE_BEAST));
                actions.push_back(::std::make_shared<SpellAction>("bloodshed", Hunter::BeastMastery::BLOODSHED));
                actions.push_back(::std::make_shared<SpellAction>("call_of_the_wild", Hunter::BeastMastery::CALL_OF_THE_WILD));
                actions.push_back(::std::make_shared<SpellAction>("mend_pet", Hunter::BeastMastery::MEND_PET));
                actions.push_back(::std::make_shared<SpellAction>("revive_pet", Hunter::BeastMastery::REVIVE_PET));
            }
            else if (specEnum == ChrSpecialization::HunterMarksmanship)
            {
                actions.push_back(::std::make_shared<SpellAction>("aimed_shot", Hunter::Marksmanship::AIMED_SHOT_MM));
                actions.push_back(::std::make_shared<SpellAction>("rapid_fire", Hunter::Marksmanship::RAPID_FIRE_MM));
                actions.push_back(::std::make_shared<SpellAction>("arcane_shot", Hunter::Marksmanship::ARCANE_SHOT_MM));
                actions.push_back(::std::make_shared<SpellAction>("steady_shot", Hunter::Marksmanship::STEADY_SHOT_MM));
                actions.push_back(::std::make_shared<SpellAction>("trueshot", Hunter::Marksmanship::TRUESHOT));
                actions.push_back(::std::make_shared<SpellAction>("double_tap", Hunter::Marksmanship::DOUBLE_TAP));
                actions.push_back(::std::make_shared<SpellAction>("explosive_shot", Hunter::Marksmanship::EXPLOSIVE_SHOT));
                actions.push_back(::std::make_shared<SpellAction>("volley", Hunter::Marksmanship::VOLLEY));
            }
            else if (specEnum == ChrSpecialization::HunterSurvival)
            {
                actions.push_back(::std::make_shared<SpellAction>("raptor_strike", Hunter::Survival::RAPTOR_STRIKE));
                actions.push_back(::std::make_shared<SpellAction>("mongoose_bite", Hunter::Survival::MONGOOSE_BITE));
                actions.push_back(::std::make_shared<SpellAction>("kill_command", Hunter::Survival::KILL_COMMAND_SURVIVAL));
                actions.push_back(::std::make_shared<SpellAction>("wildfire_bomb", Hunter::Survival::WILDFIRE_BOMB));
                actions.push_back(::std::make_shared<SpellAction>("serpent_sting", Hunter::Survival::SERPENT_STING));
                actions.push_back(::std::make_shared<SpellAction>("coordinated_assault", Hunter::Survival::COORDINATED_ASSAULT));
                actions.push_back(::std::make_shared<SpellAction>("flanking_strike", Hunter::Survival::FLANKING_STRIKE));
                actions.push_back(::std::make_shared<SpellAction>("harpoon", Hunter::Survival::HARPOON));
            }
            break;
        }

        case CLASS_ROGUE:
        {
            // Core rogue abilities
            actions.push_back(::std::make_shared<SpellAction>("kick", Rogue::KICK));
            actions.push_back(::std::make_shared<SpellAction>("vanish", Rogue::VANISH));
            actions.push_back(::std::make_shared<SpellAction>("cheap_shot", Rogue::CHEAP_SHOT));
            actions.push_back(::std::make_shared<SpellAction>("kidney_shot", Rogue::KIDNEY_SHOT));
            actions.push_back(::std::make_shared<SpellAction>("blind", Rogue::BLIND));
            actions.push_back(::std::make_shared<SpellAction>("sap", Rogue::SAP));
            actions.push_back(::std::make_shared<SpellAction>("sprint", Rogue::SPRINT));
            actions.push_back(::std::make_shared<SpellAction>("evasion", Rogue::EVASION));
            actions.push_back(::std::make_shared<SpellAction>("cloak_of_shadows", Rogue::CLOAK_OF_SHADOWS));
            actions.push_back(::std::make_shared<SpellAction>("crimson_vial", Rogue::CRIMSON_VIAL));
            actions.push_back(::std::make_shared<SpellAction>("tricks_of_the_trade", Rogue::TRICKS_OF_THE_TRADE));
            actions.push_back(::std::make_shared<SpellAction>("shadowstep", Rogue::SHADOWSTEP));

            if (specEnum == ChrSpecialization::RogueAssassination)
            {
                actions.push_back(::std::make_shared<SpellAction>("mutilate", Rogue::Assassination::MUTILATE));
                actions.push_back(::std::make_shared<SpellAction>("envenom", Rogue::Assassination::ENVENOM));
                actions.push_back(::std::make_shared<SpellAction>("garrote", Rogue::Assassination::GARROTE));
                actions.push_back(::std::make_shared<SpellAction>("rupture", Rogue::Assassination::RUPTURE));
                actions.push_back(::std::make_shared<SpellAction>("vendetta", Rogue::Assassination::VENDETTA));
                actions.push_back(::std::make_shared<SpellAction>("exsanguinate", Rogue::Assassination::EXSANGUINATE));
                actions.push_back(::std::make_shared<SpellAction>("crimson_tempest", Rogue::Assassination::CRIMSON_TEMPEST));
                actions.push_back(::std::make_shared<SpellAction>("deathmark", Rogue::Assassination::DEATHMARK));
                actions.push_back(::std::make_shared<SpellAction>("kingsbane", Rogue::Assassination::KINGSBANE));
            }
            else if (specEnum == ChrSpecialization::RogueOutlaw)
            {
                actions.push_back(::std::make_shared<SpellAction>("sinister_strike", Rogue::Outlaw::SINISTER_STRIKE));
                actions.push_back(::std::make_shared<SpellAction>("pistol_shot", Rogue::Outlaw::PISTOL_SHOT));
                actions.push_back(::std::make_shared<SpellAction>("dispatch", Rogue::Outlaw::DISPATCH));
                actions.push_back(::std::make_shared<SpellAction>("between_the_eyes", Rogue::Outlaw::BETWEEN_THE_EYES));
                actions.push_back(::std::make_shared<SpellAction>("slice_and_dice", Rogue::Outlaw::SLICE_AND_DICE));
                actions.push_back(::std::make_shared<SpellAction>("roll_the_bones", Rogue::Outlaw::ROLL_THE_BONES));
                actions.push_back(::std::make_shared<SpellAction>("blade_flurry", Rogue::Outlaw::BLADE_FLURRY));
                actions.push_back(::std::make_shared<SpellAction>("adrenaline_rush", Rogue::Outlaw::ADRENALINE_RUSH));
                actions.push_back(::std::make_shared<SpellAction>("killing_spree", Rogue::Outlaw::KILLING_SPREE));
                actions.push_back(::std::make_shared<SpellAction>("grappling_hook", Rogue::Outlaw::GRAPPLING_HOOK));
            }
            else if (specEnum == ChrSpecialization::RogueSubtely)
            {
                actions.push_back(::std::make_shared<SpellAction>("backstab", Rogue::Subtlety::BACKSTAB));
                actions.push_back(::std::make_shared<SpellAction>("shadowstrike", Rogue::Subtlety::SHADOWSTRIKE));
                actions.push_back(::std::make_shared<SpellAction>("eviscerate", Rogue::Subtlety::EVISCERATE));
                actions.push_back(::std::make_shared<SpellAction>("shadow_dance", Rogue::Subtlety::SHADOW_DANCE));
                actions.push_back(::std::make_shared<SpellAction>("symbols_of_death", Rogue::Subtlety::SYMBOLS_OF_DEATH));
                actions.push_back(::std::make_shared<SpellAction>("shadow_blades", Rogue::Subtlety::SHADOW_BLADES));
                actions.push_back(::std::make_shared<SpellAction>("shuriken_storm", Rogue::Subtlety::SHURIKEN_STORM));
                actions.push_back(::std::make_shared<SpellAction>("secret_technique", Rogue::Subtlety::SECRET_TECHNIQUE));
                actions.push_back(::std::make_shared<SpellAction>("flagellation", Rogue::Subtlety::FLAGELLATION));
            }
            break;
        }

        case CLASS_PRIEST:
        {
            // Core priest abilities
            actions.push_back(::std::make_shared<SpellAction>("power_word_shield", Priest::POWER_WORD_SHIELD));
            actions.push_back(::std::make_shared<SpellAction>("power_word_fortitude", Priest::POWER_WORD_FORTITUDE));
            actions.push_back(::std::make_shared<SpellAction>("shadow_word_pain", Priest::SHADOW_WORD_PAIN));
            actions.push_back(::std::make_shared<SpellAction>("shadow_word_death", Priest::SHADOW_WORD_DEATH));
            actions.push_back(::std::make_shared<SpellAction>("psychic_scream", Priest::PSYCHIC_SCREAM));
            actions.push_back(::std::make_shared<SpellAction>("mass_dispel", Priest::MASS_DISPEL));
            actions.push_back(::std::make_shared<SpellAction>("fade", Priest::FADE));
            actions.push_back(::std::make_shared<SpellAction>("desperate_prayer", Priest::DESPERATE_PRAYER));
            actions.push_back(::std::make_shared<SpellAction>("leap_of_faith", Priest::LEAP_OF_FAITH));
            actions.push_back(::std::make_shared<SpellAction>("power_infusion", Priest::POWER_INFUSION));

            if (specEnum == ChrSpecialization::PriestDiscipline)
            {
                actions.push_back(::std::make_shared<SpellAction>("penance", Priest::Discipline::PENANCE));
                actions.push_back(::std::make_shared<SpellAction>("power_word_radiance", Priest::Discipline::POWER_WORD_RADIANCE));
                actions.push_back(::std::make_shared<SpellAction>("schism", Priest::Discipline::SCHISM));
                actions.push_back(::std::make_shared<SpellAction>("mind_blast", Priest::Discipline::MIND_BLAST));
                actions.push_back(::std::make_shared<SpellAction>("pain_suppression", Priest::Discipline::PAIN_SUPPRESSION));
                actions.push_back(::std::make_shared<SpellAction>("power_word_barrier", Priest::Discipline::POWER_WORD_BARRIER));
                actions.push_back(::std::make_shared<SpellAction>("rapture", Priest::Discipline::RAPTURE));
                actions.push_back(::std::make_shared<SpellAction>("shadowfiend", Priest::Discipline::SHADOWFIEND));
                actions.push_back(::std::make_shared<SpellAction>("ultimate_penitence", Priest::Discipline::ULTIMATE_PENITENCE));
            }
            else if (specEnum == ChrSpecialization::PriestHoly)
            {
                actions.push_back(::std::make_shared<SpellAction>("holy_word_serenity", Priest::HolyPriest::HOLY_WORD_SERENITY));
                actions.push_back(::std::make_shared<SpellAction>("holy_word_sanctify", Priest::HolyPriest::HOLY_WORD_SANCTIFY));
                actions.push_back(::std::make_shared<SpellAction>("prayer_of_mending", Priest::HolyPriest::PRAYER_OF_MENDING));
                actions.push_back(::std::make_shared<SpellAction>("circle_of_healing", Priest::HolyPriest::CIRCLE_OF_HEALING));
                actions.push_back(::std::make_shared<SpellAction>("divine_hymn", Priest::HolyPriest::DIVINE_HYMN));
                actions.push_back(::std::make_shared<SpellAction>("guardian_spirit", Priest::HolyPriest::GUARDIAN_SPIRIT));
                actions.push_back(::std::make_shared<SpellAction>("renew", Priest::HolyPriest::RENEW));
                actions.push_back(::std::make_shared<SpellAction>("apotheosis", Priest::HolyPriest::APOTHEOSIS));
                actions.push_back(::std::make_shared<SpellAction>("holy_word_salvation", Priest::HolyPriest::HOLY_WORD_SALVATION));
            }
            else if (specEnum == ChrSpecialization::PriestShadow)
            {
                actions.push_back(::std::make_shared<SpellAction>("vampiric_touch", Priest::Shadow::VAMPIRIC_TOUCH));
                actions.push_back(::std::make_shared<SpellAction>("devouring_plague", Priest::Shadow::DEVOURING_PLAGUE));
                actions.push_back(::std::make_shared<SpellAction>("mind_blast", Priest::Shadow::MIND_BLAST_SHADOW));
                actions.push_back(::std::make_shared<SpellAction>("mind_flay", Priest::Shadow::MIND_FLAY));
                actions.push_back(::std::make_shared<SpellAction>("mind_sear", Priest::Shadow::MIND_SEAR));
                actions.push_back(::std::make_shared<SpellAction>("void_eruption", Priest::Shadow::VOID_ERUPTION));
                actions.push_back(::std::make_shared<SpellAction>("void_bolt", Priest::Shadow::VOID_BOLT));
                actions.push_back(::std::make_shared<SpellAction>("dark_ascension", Priest::Shadow::DARK_ASCENSION));
                actions.push_back(::std::make_shared<SpellAction>("void_torrent", Priest::Shadow::VOID_TORRENT));
                actions.push_back(::std::make_shared<SpellAction>("dispersion", Priest::Shadow::DISPERSION));
                actions.push_back(::std::make_shared<SpellAction>("silence", Priest::Shadow::SILENCE));
            }
            break;
        }

        case CLASS_DEATH_KNIGHT:
        {
            // Core death knight abilities
            actions.push_back(::std::make_shared<SpellAction>("death_strike", DeathKnight::DEATH_STRIKE));
            actions.push_back(::std::make_shared<SpellAction>("death_and_decay", DeathKnight::DEATH_AND_DECAY));
            actions.push_back(::std::make_shared<SpellAction>("death_grip", DeathKnight::DEATH_GRIP));
            actions.push_back(::std::make_shared<SpellAction>("anti_magic_shell", DeathKnight::ANTI_MAGIC_SHELL));
            actions.push_back(::std::make_shared<SpellAction>("anti_magic_zone", DeathKnight::ANTI_MAGIC_ZONE));
            actions.push_back(::std::make_shared<SpellAction>("icebound_fortitude", DeathKnight::ICEBOUND_FORTITUDE));
            actions.push_back(::std::make_shared<SpellAction>("chains_of_ice", DeathKnight::CHAINS_OF_ICE));
            actions.push_back(::std::make_shared<SpellAction>("mind_freeze", DeathKnight::MIND_FREEZE));
            actions.push_back(::std::make_shared<SpellAction>("raise_dead", DeathKnight::RAISE_DEAD));
            actions.push_back(::std::make_shared<SpellAction>("death_coil", DeathKnight::DEATH_COIL));

            if (specEnum == ChrSpecialization::DeathKnightBlood)
            {
                actions.push_back(::std::make_shared<SpellAction>("marrowrend", DeathKnight::Blood::MARROWREND));
                actions.push_back(::std::make_shared<SpellAction>("heart_strike", DeathKnight::Blood::HEART_STRIKE));
                actions.push_back(::std::make_shared<SpellAction>("blood_boil", DeathKnight::Blood::BLOOD_BOIL));
                actions.push_back(::std::make_shared<SpellAction>("rune_tap", DeathKnight::Blood::RUNE_TAP));
                actions.push_back(::std::make_shared<SpellAction>("vampiric_blood", DeathKnight::Blood::VAMPIRIC_BLOOD));
                actions.push_back(::std::make_shared<SpellAction>("dancing_rune_weapon", DeathKnight::Blood::DANCING_RUNE_WEAPON));
                actions.push_back(::std::make_shared<SpellAction>("bonestorm", DeathKnight::Blood::BONESTORM));
                actions.push_back(::std::make_shared<SpellAction>("consumption", DeathKnight::Blood::CONSUMPTION));
                actions.push_back(::std::make_shared<SpellAction>("gorefiends_grasp", DeathKnight::Blood::GOREFIENDS_GRASP));
            }
            else if (specEnum == ChrSpecialization::DeathKnightFrost)
            {
                actions.push_back(::std::make_shared<SpellAction>("frost_strike", DeathKnight::Frost::FROST_STRIKE));
                actions.push_back(::std::make_shared<SpellAction>("howling_blast", DeathKnight::Frost::HOWLING_BLAST));
                actions.push_back(::std::make_shared<SpellAction>("obliterate", DeathKnight::Frost::OBLITERATE));
                actions.push_back(::std::make_shared<SpellAction>("remorseless_winter", DeathKnight::Frost::REMORSELESS_WINTER));
                actions.push_back(::std::make_shared<SpellAction>("pillar_of_frost", DeathKnight::Frost::PILLAR_OF_FROST));
                actions.push_back(::std::make_shared<SpellAction>("empower_rune_weapon", DeathKnight::Frost::EMPOWER_RUNE_WEAPON));
                actions.push_back(::std::make_shared<SpellAction>("glacial_advance", DeathKnight::Frost::GLACIAL_ADVANCE));
                actions.push_back(::std::make_shared<SpellAction>("breath_of_sindragosa", DeathKnight::Frost::BREATH_OF_SINDRAGOSA));
                actions.push_back(::std::make_shared<SpellAction>("frostwyrms_fury", DeathKnight::Frost::FROSTWYRMS_FURY));
            }
            else if (specEnum == ChrSpecialization::DeathKnightUnholy)
            {
                actions.push_back(::std::make_shared<SpellAction>("festering_strike", DeathKnight::Unholy::FESTERING_STRIKE));
                actions.push_back(::std::make_shared<SpellAction>("scourge_strike", DeathKnight::Unholy::SCOURGE_STRIKE));
                actions.push_back(::std::make_shared<SpellAction>("epidemic", DeathKnight::Unholy::EPIDEMIC));
                actions.push_back(::std::make_shared<SpellAction>("outbreak", DeathKnight::Unholy::OUTBREAK));
                actions.push_back(::std::make_shared<SpellAction>("dark_transformation", DeathKnight::Unholy::DARK_TRANSFORMATION));
                actions.push_back(::std::make_shared<SpellAction>("apocalypse", DeathKnight::Unholy::APOCALYPSE));
                actions.push_back(::std::make_shared<SpellAction>("army_of_the_dead", DeathKnight::Unholy::ARMY_OF_THE_DEAD));
                actions.push_back(::std::make_shared<SpellAction>("summon_gargoyle", DeathKnight::Unholy::SUMMON_GARGOYLE));
                actions.push_back(::std::make_shared<SpellAction>("unholy_assault", DeathKnight::Unholy::UNHOLY_ASSAULT));
            }
            break;
        }

        case CLASS_SHAMAN:
        {
            // Core shaman abilities
            actions.push_back(::std::make_shared<SpellAction>("lightning_bolt", Shaman::LIGHTNING_BOLT));
            actions.push_back(::std::make_shared<SpellAction>("chain_lightning", Shaman::CHAIN_LIGHTNING));
            actions.push_back(::std::make_shared<SpellAction>("flame_shock", Shaman::FLAME_SHOCK));
            actions.push_back(::std::make_shared<SpellAction>("frost_shock", Shaman::FROST_SHOCK));
            actions.push_back(::std::make_shared<SpellAction>("wind_shear", Shaman::WIND_SHEAR));
            actions.push_back(::std::make_shared<SpellAction>("hex", Shaman::HEX));
            actions.push_back(::std::make_shared<SpellAction>("bloodlust", Shaman::BLOODLUST));
            actions.push_back(::std::make_shared<SpellAction>("capacitor_totem", Shaman::CAPACITOR_TOTEM));
            actions.push_back(::std::make_shared<SpellAction>("healing_stream_totem", Shaman::HEALING_STREAM_TOTEM));
            actions.push_back(::std::make_shared<SpellAction>("ghost_wolf", Shaman::GHOST_WOLF));

            if (specEnum == ChrSpecialization::ShamanElemental)
            {
                actions.push_back(::std::make_shared<SpellAction>("lava_burst", Shaman::LAVA_BURST));
                actions.push_back(::std::make_shared<SpellAction>("earth_shock", Shaman::EARTH_SHOCK));
                actions.push_back(::std::make_shared<SpellAction>("elemental_blast", Shaman::Elemental::ELEMENTAL_BLAST));
                actions.push_back(::std::make_shared<SpellAction>("earthquake", Shaman::Elemental::EARTHQUAKE));
                actions.push_back(::std::make_shared<SpellAction>("stormkeeper", Shaman::Elemental::STORMKEEPER));
                actions.push_back(::std::make_shared<SpellAction>("ascendance", Shaman::Elemental::ASCENDANCE));
                actions.push_back(::std::make_shared<SpellAction>("fire_elemental", Shaman::Elemental::FIRE_ELEMENTAL));
                actions.push_back(::std::make_shared<SpellAction>("storm_elemental", Shaman::Elemental::STORM_ELEMENTAL));
                actions.push_back(::std::make_shared<SpellAction>("primordial_wave", Shaman::Elemental::PRIMORDIAL_WAVE));
                actions.push_back(::std::make_shared<SpellAction>("tempest", Shaman::Elemental::TEMPEST));
            }
            else if (specEnum == ChrSpecialization::ShamanEnhancement)
            {
                actions.push_back(::std::make_shared<SpellAction>("stormstrike", Shaman::Enhancement::STORMSTRIKE));
                actions.push_back(::std::make_shared<SpellAction>("lava_lash", Shaman::Enhancement::LAVA_LASH));
                actions.push_back(::std::make_shared<SpellAction>("crash_lightning", Shaman::Enhancement::CRASH_LIGHTNING));
                actions.push_back(::std::make_shared<SpellAction>("sundering", Shaman::Enhancement::SUNDERING));
                actions.push_back(::std::make_shared<SpellAction>("feral_spirit", Shaman::Enhancement::FERAL_SPIRIT));
                actions.push_back(::std::make_shared<SpellAction>("doom_winds", Shaman::Enhancement::DOOM_WINDS));
                actions.push_back(::std::make_shared<SpellAction>("windfury_totem", Shaman::Enhancement::WINDFURY_TOTEM));
                actions.push_back(::std::make_shared<SpellAction>("ascendance", Shaman::Enhancement::ASCENDANCE_ENH));
            }
            else if (specEnum == ChrSpecialization::ShamanRestoration)
            {
                actions.push_back(::std::make_shared<SpellAction>("healing_wave", Shaman::Restoration::HEALING_WAVE));
                actions.push_back(::std::make_shared<SpellAction>("healing_surge", Shaman::Restoration::HEALING_SURGE));
                actions.push_back(::std::make_shared<SpellAction>("chain_heal", Shaman::Restoration::CHAIN_HEAL));
                actions.push_back(::std::make_shared<SpellAction>("riptide", Shaman::Restoration::RIPTIDE));
                actions.push_back(::std::make_shared<SpellAction>("healing_rain", Shaman::Restoration::HEALING_RAIN));
                actions.push_back(::std::make_shared<SpellAction>("spirit_link_totem", Shaman::Restoration::SPIRIT_LINK_TOTEM));
                actions.push_back(::std::make_shared<SpellAction>("healing_tide_totem", Shaman::Restoration::HEALING_TIDE_TOTEM));
                actions.push_back(::std::make_shared<SpellAction>("earth_shield", Shaman::Restoration::EARTH_SHIELD));
                actions.push_back(::std::make_shared<SpellAction>("ascendance", Shaman::Restoration::ASCENDANCE_RESTO));
            }
            break;
        }

        case CLASS_MAGE:
        {
            // Core mage abilities
            actions.push_back(::std::make_shared<SpellAction>("counterspell", Mage::COUNTERSPELL));
            actions.push_back(::std::make_shared<SpellAction>("ice_block", Mage::ICE_BLOCK));
            actions.push_back(::std::make_shared<SpellAction>("blink", Mage::BLINK));
            actions.push_back(::std::make_shared<SpellAction>("polymorph", Mage::POLYMORPH));
            actions.push_back(::std::make_shared<SpellAction>("frost_nova", Mage::FROST_NOVA));
            actions.push_back(::std::make_shared<SpellAction>("mirror_image", Mage::MIRROR_IMAGE));
            actions.push_back(::std::make_shared<SpellAction>("time_warp", Mage::TIME_WARP));
            actions.push_back(::std::make_shared<SpellAction>("arcane_intellect", Mage::ARCANE_INTELLECT));
            actions.push_back(::std::make_shared<SpellAction>("spellsteal", Mage::SPELLSTEAL));
            actions.push_back(::std::make_shared<SpellAction>("remove_curse", Mage::REMOVE_CURSE));

            if (specEnum == ChrSpecialization::MageArcane)
            {
                actions.push_back(::std::make_shared<SpellAction>("arcane_blast", Mage::Arcane::ARCANE_BLAST));
                actions.push_back(::std::make_shared<SpellAction>("arcane_missiles", Mage::Arcane::ARCANE_MISSILES));
                actions.push_back(::std::make_shared<SpellAction>("arcane_barrage", Mage::Arcane::ARCANE_BARRAGE));
                actions.push_back(::std::make_shared<SpellAction>("arcane_explosion", Mage::Arcane::ARCANE_EXPLOSION));
                actions.push_back(::std::make_shared<SpellAction>("arcane_power", Mage::Arcane::ARCANE_POWER));
                actions.push_back(::std::make_shared<SpellAction>("evocation", Mage::Arcane::EVOCATION));
                actions.push_back(::std::make_shared<SpellAction>("arcane_orb", Mage::Arcane::ARCANE_ORB));
                actions.push_back(::std::make_shared<SpellAction>("arcane_surge", Mage::Arcane::ARCANE_SURGE));
                actions.push_back(::std::make_shared<SpellAction>("touch_of_the_magi", Mage::Arcane::TOUCH_OF_THE_MAGI));
            }
            else if (specEnum == ChrSpecialization::MageFire)
            {
                actions.push_back(::std::make_shared<SpellAction>("fireball", Mage::FIREBALL));
                actions.push_back(::std::make_shared<SpellAction>("pyroblast", Mage::Fire::PYROBLAST));
                actions.push_back(::std::make_shared<SpellAction>("fire_blast", Mage::Fire::FIRE_BLAST));
                actions.push_back(::std::make_shared<SpellAction>("phoenix_flames", Mage::Fire::PHOENIX_FLAMES));
                actions.push_back(::std::make_shared<SpellAction>("scorch", Mage::Fire::SCORCH));
                actions.push_back(::std::make_shared<SpellAction>("flamestrike", Mage::Fire::FLAMESTRIKE));
                actions.push_back(::std::make_shared<SpellAction>("combustion", Mage::Fire::COMBUSTION));
                actions.push_back(::std::make_shared<SpellAction>("living_bomb", Mage::Fire::LIVING_BOMB));
                actions.push_back(::std::make_shared<SpellAction>("meteor", Mage::Fire::METEOR));
                actions.push_back(::std::make_shared<SpellAction>("dragons_breath", Mage::DRAGONS_BREATH));
            }
            else if (specEnum == ChrSpecialization::MageFrost)
            {
                actions.push_back(::std::make_shared<SpellAction>("frostbolt", Mage::FROSTBOLT));
                actions.push_back(::std::make_shared<SpellAction>("ice_lance", Mage::Frost::ICE_LANCE));
                actions.push_back(::std::make_shared<SpellAction>("flurry", Mage::Frost::FLURRY));
                actions.push_back(::std::make_shared<SpellAction>("frozen_orb", Mage::Frost::FROZEN_ORB));
                actions.push_back(::std::make_shared<SpellAction>("blizzard", Mage::Frost::BLIZZARD));
                actions.push_back(::std::make_shared<SpellAction>("cone_of_cold", Mage::Frost::CONE_OF_COLD));
                actions.push_back(::std::make_shared<SpellAction>("icy_veins", Mage::Frost::ICY_VEINS));
                actions.push_back(::std::make_shared<SpellAction>("glacial_spike", Mage::Frost::GLACIAL_SPIKE));
                actions.push_back(::std::make_shared<SpellAction>("comet_storm", Mage::Frost::COMET_STORM));
                actions.push_back(::std::make_shared<SpellAction>("ray_of_frost", Mage::Frost::RAY_OF_FROST));
            }
            break;
        }

        case CLASS_WARLOCK:
        {
            // Core warlock abilities
            actions.push_back(::std::make_shared<SpellAction>("shadow_bolt", Warlock::SHADOW_BOLT));
            actions.push_back(::std::make_shared<SpellAction>("corruption", Warlock::CORRUPTION));
            actions.push_back(::std::make_shared<SpellAction>("drain_life", Warlock::DRAIN_LIFE));
            actions.push_back(::std::make_shared<SpellAction>("unending_resolve", Warlock::UNENDING_RESOLVE));
            actions.push_back(::std::make_shared<SpellAction>("fear", Warlock::FEAR));
            actions.push_back(::std::make_shared<SpellAction>("shadowfury", Warlock::SHADOWFURY));
            actions.push_back(::std::make_shared<SpellAction>("spell_lock", Warlock::SPELL_LOCK));
            actions.push_back(::std::make_shared<SpellAction>("health_funnel", Warlock::HEALTH_FUNNEL));
            actions.push_back(::std::make_shared<SpellAction>("soulstone", Warlock::SOULSTONE));
            actions.push_back(::std::make_shared<SpellAction>("demonic_circle_teleport", Warlock::DEMONIC_CIRCLE_TELEPORT));

            if (specEnum == ChrSpecialization::WarlockAffliction)
            {
                actions.push_back(::std::make_shared<SpellAction>("agony", Warlock::Affliction::AGONY));
                actions.push_back(::std::make_shared<SpellAction>("unstable_affliction", Warlock::Affliction::UNSTABLE_AFFLICTION));
                actions.push_back(::std::make_shared<SpellAction>("seed_of_corruption", Warlock::Affliction::SEED_OF_CORRUPTION));
                actions.push_back(::std::make_shared<SpellAction>("haunt", Warlock::Affliction::HAUNT));
                actions.push_back(::std::make_shared<SpellAction>("malefic_rapture", Warlock::Affliction::MALEFIC_RAPTURE));
                actions.push_back(::std::make_shared<SpellAction>("drain_soul", Warlock::Affliction::DRAIN_SOUL));
                actions.push_back(::std::make_shared<SpellAction>("phantom_singularity", Warlock::Affliction::PHANTOM_SINGULARITY));
                actions.push_back(::std::make_shared<SpellAction>("summon_darkglare", Warlock::Affliction::SUMMON_DARKGLARE));
                actions.push_back(::std::make_shared<SpellAction>("soul_rot", Warlock::Affliction::SOUL_ROT));
            }
            else if (specEnum == ChrSpecialization::WarlockDemonology)
            {
                actions.push_back(::std::make_shared<SpellAction>("demonbolt", Warlock::Demonology::DEMONBOLT));
                actions.push_back(::std::make_shared<SpellAction>("hand_of_guldan", Warlock::Demonology::HAND_OF_GULDAN));
                actions.push_back(::std::make_shared<SpellAction>("call_dreadstalkers", Warlock::Demonology::CALL_DREADSTALKERS));
                actions.push_back(::std::make_shared<SpellAction>("implosion", Warlock::Demonology::IMPLOSION));
                actions.push_back(::std::make_shared<SpellAction>("summon_demonic_tyrant", Warlock::Demonology::SUMMON_DEMONIC_TYRANT));
                actions.push_back(::std::make_shared<SpellAction>("power_siphon", Warlock::Demonology::POWER_SIPHON));
                actions.push_back(::std::make_shared<SpellAction>("demonic_strength", Warlock::Demonology::DEMONIC_STRENGTH));
                actions.push_back(::std::make_shared<SpellAction>("summon_vilefiend", Warlock::Demonology::SUMMON_VILEFIEND));
                actions.push_back(::std::make_shared<SpellAction>("guillotine", Warlock::Demonology::GUILLOTINE));
                actions.push_back(::std::make_shared<SpellAction>("nether_portal", Warlock::Demonology::NETHER_PORTAL));
            }
            else if (specEnum == ChrSpecialization::WarlockDestruction)
            {
                actions.push_back(::std::make_shared<SpellAction>("incinerate", Warlock::Destruction::INCINERATE));
                actions.push_back(::std::make_shared<SpellAction>("immolate", Warlock::Destruction::IMMOLATE));
                actions.push_back(::std::make_shared<SpellAction>("conflagrate", Warlock::Destruction::CONFLAGRATE));
                actions.push_back(::std::make_shared<SpellAction>("chaos_bolt", Warlock::Destruction::CHAOS_BOLT));
                actions.push_back(::std::make_shared<SpellAction>("rain_of_fire", Warlock::Destruction::RAIN_OF_FIRE));
                actions.push_back(::std::make_shared<SpellAction>("havoc", Warlock::Destruction::HAVOC));
                actions.push_back(::std::make_shared<SpellAction>("shadowburn", Warlock::Destruction::SHADOWBURN));
                actions.push_back(::std::make_shared<SpellAction>("cataclysm", Warlock::Destruction::CATACLYSM));
                actions.push_back(::std::make_shared<SpellAction>("summon_infernal", Warlock::Destruction::SUMMON_INFERNAL));
                actions.push_back(::std::make_shared<SpellAction>("channel_demonfire", Warlock::Destruction::CHANNEL_DEMONFIRE));
            }
            break;
        }

        case CLASS_DRUID:
        {
            // Core druid abilities
            actions.push_back(::std::make_shared<SpellAction>("bear_form", Druid::BEAR_FORM));
            actions.push_back(::std::make_shared<SpellAction>("cat_form", Druid::CAT_FORM));
            actions.push_back(::std::make_shared<SpellAction>("moonkin_form", Druid::MOONKIN_FORM));
            actions.push_back(::std::make_shared<SpellAction>("travel_form", Druid::TRAVEL_FORM));
            actions.push_back(::std::make_shared<SpellAction>("barkskin", Druid::BARKSKIN));
            actions.push_back(::std::make_shared<SpellAction>("dash", Druid::DASH));
            actions.push_back(::std::make_shared<SpellAction>("stampeding_roar", Druid::STAMPEDING_ROAR));
            actions.push_back(::std::make_shared<SpellAction>("entangling_roots", Druid::ENTANGLING_ROOTS));
            actions.push_back(::std::make_shared<SpellAction>("cyclone", Druid::CYCLONE));
            actions.push_back(::std::make_shared<SpellAction>("moonfire", Druid::MOONFIRE));
            actions.push_back(::std::make_shared<SpellAction>("sunfire", Druid::SUNFIRE));
            actions.push_back(::std::make_shared<SpellAction>("rebirth", Druid::REBIRTH));
            actions.push_back(::std::make_shared<SpellAction>("innervate", Druid::INNERVATE));

            if (specEnum == ChrSpecialization::DruidBalance)
            {
                actions.push_back(::std::make_shared<SpellAction>("solar_beam", Druid::SOLAR_BEAM));
                actions.push_back(::std::make_shared<SpellAction>("wrath", Druid::Balance::WRATH));
                actions.push_back(::std::make_shared<SpellAction>("starfire", Druid::Balance::STARFIRE));
                actions.push_back(::std::make_shared<SpellAction>("starsurge", Druid::Balance::STARSURGE));
                actions.push_back(::std::make_shared<SpellAction>("starfall", Druid::Balance::STARFALL));
                actions.push_back(::std::make_shared<SpellAction>("celestial_alignment", Druid::Balance::CELESTIAL_ALIGNMENT));
                actions.push_back(::std::make_shared<SpellAction>("convoke_the_spirits", Druid::Balance::CONVOKE_THE_SPIRITS));
                actions.push_back(::std::make_shared<SpellAction>("fury_of_elune", Druid::Balance::FURY_OF_ELUNE));
                actions.push_back(::std::make_shared<SpellAction>("force_of_nature", Druid::Balance::FORCE_OF_NATURE));
            }
            else if (specEnum == ChrSpecialization::DruidFeral)
            {
                actions.push_back(::std::make_shared<SpellAction>("skull_bash", Druid::SKULL_BASH));
                actions.push_back(::std::make_shared<SpellAction>("rake", Druid::Feral::RAKE));
                actions.push_back(::std::make_shared<SpellAction>("shred", Druid::Feral::SHRED));
                actions.push_back(::std::make_shared<SpellAction>("ferocious_bite", Druid::Feral::FEROCIOUS_BITE));
                actions.push_back(::std::make_shared<SpellAction>("rip", Druid::Feral::RIP));
                actions.push_back(::std::make_shared<SpellAction>("savage_roar", Druid::Feral::SAVAGE_ROAR));
                actions.push_back(::std::make_shared<SpellAction>("tigers_fury", Druid::Feral::TIGERS_FURY));
                actions.push_back(::std::make_shared<SpellAction>("berserk", Druid::Feral::BERSERK));
                actions.push_back(::std::make_shared<SpellAction>("primal_wrath", Druid::Feral::PRIMAL_WRATH));
                actions.push_back(::std::make_shared<SpellAction>("feral_frenzy", Druid::Feral::FERAL_FRENZY));
            }
            else if (specEnum == ChrSpecialization::DruidGuardian)
            {
                actions.push_back(::std::make_shared<SpellAction>("skull_bash", Druid::SKULL_BASH));
                actions.push_back(::std::make_shared<SpellAction>("mangle", Druid::Guardian::MANGLE));
                actions.push_back(::std::make_shared<SpellAction>("thrash", Druid::Guardian::THRASH_BEAR));
                actions.push_back(::std::make_shared<SpellAction>("swipe", Druid::Guardian::SWIPE_BEAR));
                actions.push_back(::std::make_shared<SpellAction>("maul", Druid::Guardian::MAUL));
                actions.push_back(::std::make_shared<SpellAction>("ironfur", Druid::Guardian::IRONFUR));
                actions.push_back(::std::make_shared<SpellAction>("frenzied_regeneration", Druid::Guardian::FRENZIED_REGENERATION));
                actions.push_back(::std::make_shared<SpellAction>("survival_instincts", Druid::Guardian::SURVIVAL_INSTINCTS));
                actions.push_back(::std::make_shared<SpellAction>("berserk", Druid::Guardian::BERSERK_GUARDIAN));
                actions.push_back(::std::make_shared<SpellAction>("rage_of_the_sleeper", Druid::Guardian::RAGE_OF_THE_SLEEPER));
            }
            else if (specEnum == ChrSpecialization::DruidRestoration)
            {
                actions.push_back(::std::make_shared<SpellAction>("regrowth", Druid::REGROWTH));
                actions.push_back(::std::make_shared<SpellAction>("rejuvenation", Druid::REJUVENATION));
                actions.push_back(::std::make_shared<SpellAction>("swiftmend", Druid::SWIFTMEND));
                actions.push_back(::std::make_shared<SpellAction>("wild_growth", Druid::WILD_GROWTH));
                actions.push_back(::std::make_shared<SpellAction>("lifebloom", Druid::Restoration::LIFEBLOOM));
                actions.push_back(::std::make_shared<SpellAction>("tranquility", Druid::Restoration::TRANQUILITY));
                actions.push_back(::std::make_shared<SpellAction>("flourish", Druid::Restoration::FLOURISH));
                actions.push_back(::std::make_shared<SpellAction>("tree_of_life", Druid::Restoration::TREE_OF_LIFE));
                actions.push_back(::std::make_shared<SpellAction>("overgrowth", Druid::Restoration::OVERGROWTH));
            }
            break;
        }

        case CLASS_DEMON_HUNTER:
        {
            // Core demon hunter abilities
            actions.push_back(::std::make_shared<SpellAction>("fel_rush", DemonHunter::FEL_RUSH));
            actions.push_back(::std::make_shared<SpellAction>("vengeful_retreat", DemonHunter::VENGEFUL_RETREAT));
            actions.push_back(::std::make_shared<SpellAction>("throw_glaive", DemonHunter::THROW_GLAIVE));
            actions.push_back(::std::make_shared<SpellAction>("disrupt", DemonHunter::DISRUPT));
            actions.push_back(::std::make_shared<SpellAction>("imprison", DemonHunter::IMPRISON));
            actions.push_back(::std::make_shared<SpellAction>("darkness", DemonHunter::DARKNESS));
            actions.push_back(::std::make_shared<SpellAction>("chaos_nova", DemonHunter::CHAOS_NOVA));
            actions.push_back(::std::make_shared<SpellAction>("sigil_of_flame", DemonHunter::SIGIL_OF_FLAME));
            actions.push_back(::std::make_shared<SpellAction>("sigil_of_misery", DemonHunter::SIGIL_OF_MISERY));
            actions.push_back(::std::make_shared<SpellAction>("sigil_of_silence", DemonHunter::SIGIL_OF_SILENCE));

            if (specEnum == ChrSpecialization::DemonHunterHavoc)
            {
                actions.push_back(::std::make_shared<SpellAction>("metamorphosis", DemonHunter::METAMORPHOSIS_HAVOC));
                actions.push_back(::std::make_shared<SpellAction>("demons_bite", DemonHunter::Havoc::DEMONS_BITE));
                actions.push_back(::std::make_shared<SpellAction>("chaos_strike", DemonHunter::Havoc::CHAOS_STRIKE));
                actions.push_back(::std::make_shared<SpellAction>("blade_dance", DemonHunter::Havoc::BLADE_DANCE));
                actions.push_back(::std::make_shared<SpellAction>("immolation_aura", DemonHunter::Havoc::IMMOLATION_AURA));
                actions.push_back(::std::make_shared<SpellAction>("eye_beam", DemonHunter::Havoc::EYE_BEAM));
                actions.push_back(::std::make_shared<SpellAction>("glaive_tempest", DemonHunter::Havoc::GLAIVE_TEMPEST));
                actions.push_back(::std::make_shared<SpellAction>("essence_break", DemonHunter::Havoc::ESSENCE_BREAK));
                actions.push_back(::std::make_shared<SpellAction>("the_hunt", DemonHunter::Havoc::THE_HUNT));
            }
            else if (specEnum == ChrSpecialization::DemonHunterVengeance)
            {
                actions.push_back(::std::make_shared<SpellAction>("metamorphosis", DemonHunter::METAMORPHOSIS_VENGEANCE));
                actions.push_back(::std::make_shared<SpellAction>("shear", DemonHunter::Vengeance::SHEAR));
                actions.push_back(::std::make_shared<SpellAction>("fracture", DemonHunter::Vengeance::FRACTURE));
                actions.push_back(::std::make_shared<SpellAction>("soul_cleave", DemonHunter::Vengeance::SOUL_CLEAVE));
                actions.push_back(::std::make_shared<SpellAction>("immolation_aura", DemonHunter::Vengeance::IMMOLATION_AURA_VENG));
                actions.push_back(::std::make_shared<SpellAction>("demon_spikes", DemonHunter::Vengeance::DEMON_SPIKES));
                actions.push_back(::std::make_shared<SpellAction>("fiery_brand", DemonHunter::Vengeance::FIERY_BRAND));
                actions.push_back(::std::make_shared<SpellAction>("infernal_strike", DemonHunter::Vengeance::INFERNAL_STRIKE));
                actions.push_back(::std::make_shared<SpellAction>("spirit_bomb", DemonHunter::Vengeance::SPIRIT_BOMB));
                actions.push_back(::std::make_shared<SpellAction>("fel_devastation", DemonHunter::Vengeance::FEL_DEVASTATION));
            }
            break;
        }

        case CLASS_EVOKER:
        {
            // Core evoker abilities
            actions.push_back(::std::make_shared<SpellAction>("living_flame", Evoker::LIVING_FLAME));
            actions.push_back(::std::make_shared<SpellAction>("azure_strike", Evoker::AZURE_STRIKE));
            actions.push_back(::std::make_shared<SpellAction>("hover", Evoker::HOVER));
            actions.push_back(::std::make_shared<SpellAction>("quell", Evoker::QUELL));
            actions.push_back(::std::make_shared<SpellAction>("tail_swipe", Evoker::TAIL_SWIPE));
            actions.push_back(::std::make_shared<SpellAction>("wing_buffet", Evoker::WING_BUFFET));
            actions.push_back(::std::make_shared<SpellAction>("expunge", Evoker::EXPUNGE));
            actions.push_back(::std::make_shared<SpellAction>("cauterizing_flame", Evoker::CAUTERIZING_FLAME));
            actions.push_back(::std::make_shared<SpellAction>("rescue", Evoker::RESCUE));
            actions.push_back(::std::make_shared<SpellAction>("verdant_embrace", Evoker::VERDANT_EMBRACE));
            actions.push_back(::std::make_shared<SpellAction>("emerald_blossom", Evoker::EMERALD_BLOSSOM));

            if (specEnum == ChrSpecialization::EvokerDevastation)
            {
                actions.push_back(::std::make_shared<SpellAction>("fire_breath", Evoker::FIRE_BREATH));
                actions.push_back(::std::make_shared<SpellAction>("disintegrate", Evoker::DISINTEGRATE));
                actions.push_back(::std::make_shared<SpellAction>("pyre", Evoker::Devastation::PYRE));
                actions.push_back(::std::make_shared<SpellAction>("eternity_surge", Evoker::Devastation::ETERNITY_SURGE));
                actions.push_back(::std::make_shared<SpellAction>("shattering_star", Evoker::Devastation::SHATTERING_STAR));
                actions.push_back(::std::make_shared<SpellAction>("dragonrage", Evoker::Devastation::DRAGONRAGE));
                actions.push_back(::std::make_shared<SpellAction>("firestorm", Evoker::Devastation::FIRESTORM));
                actions.push_back(::std::make_shared<SpellAction>("deep_breath", Evoker::DEEP_BREATH));
            }
            else if (specEnum == ChrSpecialization::EvokerPreservation)
            {
                actions.push_back(::std::make_shared<SpellAction>("echo", Evoker::Preservation::ECHO));
                actions.push_back(::std::make_shared<SpellAction>("reversion", Evoker::Preservation::REVERSION));
                actions.push_back(::std::make_shared<SpellAction>("temporal_anomaly", Evoker::Preservation::TEMPORAL_ANOMALY));
                actions.push_back(::std::make_shared<SpellAction>("time_dilation", Evoker::Preservation::TIME_DILATION));
                actions.push_back(::std::make_shared<SpellAction>("dream_breath", Evoker::Preservation::DREAM_BREATH));
                actions.push_back(::std::make_shared<SpellAction>("spiritbloom", Evoker::Preservation::SPIRITBLOOM));
                actions.push_back(::std::make_shared<SpellAction>("rewind", Evoker::Preservation::REWIND));
                actions.push_back(::std::make_shared<SpellAction>("emerald_communion", Evoker::Preservation::EMERALD_COMMUNION));
                actions.push_back(::std::make_shared<SpellAction>("stasis", Evoker::Preservation::STASIS));
            }
            else if (specEnum == ChrSpecialization::EvokerAugmentation)
            {
                actions.push_back(::std::make_shared<SpellAction>("ebon_might", Evoker::Augmentation::EBON_MIGHT));
                actions.push_back(::std::make_shared<SpellAction>("prescience", Evoker::Augmentation::PRESCIENCE));
                actions.push_back(::std::make_shared<SpellAction>("breath_of_eons", Evoker::Augmentation::BREATH_OF_EONS));
                actions.push_back(::std::make_shared<SpellAction>("time_skip", Evoker::Augmentation::TIME_SKIP));
                actions.push_back(::std::make_shared<SpellAction>("blistering_scales", Evoker::Augmentation::BLISTERING_SCALES));
                actions.push_back(::std::make_shared<SpellAction>("upheaval", Evoker::Augmentation::UPHEAVAL));
                actions.push_back(::std::make_shared<SpellAction>("eruption", Evoker::Augmentation::ERUPTION));
            }
            break;
        }

        // Note: CLASS_MONK (10) is intentionally excluded per requirements
        // CLASS_ADVENTURER (14) and CLASS_TRAVELER (15) are NPCs, not player classes

        default:
            TC_LOG_DEBUG("module.playerbot.action", "ActionFactory: Unknown class {} for CreateClassActions", classId);
            break;
    }

    TC_LOG_DEBUG("module.playerbot.action", "ActionFactory: Created {} actions for class {} spec {}",
        actions.size(), classId, spec);

    return actions;
}

::std::vector<::std::shared_ptr<Action>> ActionFactory::CreateCombatActions(uint8 classId)
{
    using namespace WoW120Spells;

    ::std::vector<::std::shared_ptr<Action>> actions;

    // Universal combat actions for all classes
    actions.push_back(::std::make_shared<AttackAction>());

    // Class-specific universal combat actions (available to all specs of the class)
    switch (classId)
    {
        case CLASS_WARRIOR:
        {
            // Warrior universal combat abilities
            actions.push_back(::std::make_shared<SpellAction>("battle_shout", Warrior::BATTLE_SHOUT));
            actions.push_back(::std::make_shared<SpellAction>("rallying_cry", Warrior::RALLYING_CRY));
            actions.push_back(::std::make_shared<SpellAction>("victory_rush", Warrior::VICTORY_RUSH));
            actions.push_back(::std::make_shared<SpellAction>("heroic_throw", Warrior::HEROIC_THROW));
            actions.push_back(::std::make_shared<SpellAction>("intimidating_shout", Warrior::INTIMIDATING_SHOUT));
            actions.push_back(::std::make_shared<SpellAction>("hamstring", Warrior::HAMSTRING));
            actions.push_back(::std::make_shared<SpellAction>("berserker_rage", Warrior::BERSERKER_RAGE));
            actions.push_back(::std::make_shared<SpellAction>("spell_reflection", Warrior::SPELL_REFLECTION));
            actions.push_back(::std::make_shared<SpellAction>("intervene", Warrior::INTERVENE));
            break;
        }

        case CLASS_PALADIN:
        {
            // Paladin universal combat abilities
            actions.push_back(::std::make_shared<SpellAction>("divine_shield", Paladin::DIVINE_SHIELD));
            actions.push_back(::std::make_shared<SpellAction>("lay_on_hands", Paladin::LAY_ON_HANDS));
            actions.push_back(::std::make_shared<SpellAction>("blessing_of_protection", Paladin::BLESSING_OF_PROTECTION));
            actions.push_back(::std::make_shared<SpellAction>("blessing_of_freedom", Paladin::BLESSING_OF_FREEDOM));
            actions.push_back(::std::make_shared<SpellAction>("hammer_of_justice", Paladin::HAMMER_OF_JUSTICE));
            actions.push_back(::std::make_shared<SpellAction>("cleanse_toxins", Paladin::CLEANSE_TOXINS));
            actions.push_back(::std::make_shared<SpellAction>("crusader_aura", Paladin::CRUSADER_AURA));
            actions.push_back(::std::make_shared<SpellAction>("devotion_aura", Paladin::DEVOTION_AURA));
            actions.push_back(::std::make_shared<SpellAction>("retribution_aura", Paladin::RETRIBUTION_AURA));
            actions.push_back(::std::make_shared<SpellAction>("concentration_aura", Paladin::CONCENTRATION_AURA));
            break;
        }

        case CLASS_HUNTER:
        {
            // Hunter universal combat abilities
            actions.push_back(::std::make_shared<SpellAction>("aspect_of_the_cheetah", Hunter::ASPECT_OF_THE_CHEETAH));
            actions.push_back(::std::make_shared<SpellAction>("aspect_of_the_turtle", Hunter::ASPECT_OF_THE_TURTLE));
            actions.push_back(::std::make_shared<SpellAction>("exhilaration", Hunter::EXHILARATION));
            actions.push_back(::std::make_shared<SpellAction>("feign_death", Hunter::FEIGN_DEATH));
            actions.push_back(::std::make_shared<SpellAction>("misdirection", Hunter::MISDIRECTION));
            actions.push_back(::std::make_shared<SpellAction>("disengage", Hunter::DISENGAGE));
            actions.push_back(::std::make_shared<SpellAction>("flare", Hunter::FLARE));
            actions.push_back(::std::make_shared<SpellAction>("freezing_trap", Hunter::FREEZING_TRAP));
            actions.push_back(::std::make_shared<SpellAction>("tar_trap", Hunter::TAR_TRAP));
            actions.push_back(::std::make_shared<SpellAction>("tranquilizing_shot", Hunter::TRANQUILIZING_SHOT));
            actions.push_back(::std::make_shared<SpellAction>("call_pet", Hunter::CALL_PET_1));
            actions.push_back(::std::make_shared<SpellAction>("dismiss_pet", Hunter::DISMISS_PET));
            actions.push_back(::std::make_shared<SpellAction>("revive_pet", Hunter::REVIVE_PET));
            actions.push_back(::std::make_shared<SpellAction>("mend_pet", Hunter::MEND_PET));
            break;
        }

        case CLASS_ROGUE:
        {
            // Rogue universal combat abilities
            actions.push_back(::std::make_shared<SpellAction>("sprint", Rogue::SPRINT));
            actions.push_back(::std::make_shared<SpellAction>("vanish", Rogue::VANISH));
            actions.push_back(::std::make_shared<SpellAction>("evasion", Rogue::EVASION));
            actions.push_back(::std::make_shared<SpellAction>("cloak_of_shadows", Rogue::CLOAK_OF_SHADOWS));
            actions.push_back(::std::make_shared<SpellAction>("feint", Rogue::FEINT));
            actions.push_back(::std::make_shared<SpellAction>("stealth", Rogue::STEALTH));
            actions.push_back(::std::make_shared<SpellAction>("sap", Rogue::SAP));
            actions.push_back(::std::make_shared<SpellAction>("cheap_shot", Rogue::CHEAP_SHOT));
            actions.push_back(::std::make_shared<SpellAction>("kidney_shot", Rogue::KIDNEY_SHOT));
            actions.push_back(::std::make_shared<SpellAction>("blind", Rogue::BLIND));
            actions.push_back(::std::make_shared<SpellAction>("crimson_vial", Rogue::CRIMSON_VIAL));
            actions.push_back(::std::make_shared<SpellAction>("tricks_of_the_trade", Rogue::TRICKS_OF_THE_TRADE));
            actions.push_back(::std::make_shared<SpellAction>("pick_pocket", Rogue::PICK_POCKET));
            actions.push_back(::std::make_shared<SpellAction>("pick_lock", Rogue::PICK_LOCK));
            break;
        }

        case CLASS_PRIEST:
        {
            // Priest universal combat abilities
            actions.push_back(::std::make_shared<SpellAction>("power_word_shield", Priest::POWER_WORD_SHIELD));
            actions.push_back(::std::make_shared<SpellAction>("power_word_fortitude", Priest::POWER_WORD_FORTITUDE));
            actions.push_back(::std::make_shared<SpellAction>("fade", Priest::FADE));
            actions.push_back(::std::make_shared<SpellAction>("mass_dispel", Priest::MASS_DISPEL));
            actions.push_back(::std::make_shared<SpellAction>("dispel_magic", Priest::DISPEL_MAGIC));
            actions.push_back(::std::make_shared<SpellAction>("leap_of_faith", Priest::LEAP_OF_FAITH));
            actions.push_back(::std::make_shared<SpellAction>("psychic_scream", Priest::PSYCHIC_SCREAM));
            actions.push_back(::std::make_shared<SpellAction>("mind_control", Priest::MIND_CONTROL));
            actions.push_back(::std::make_shared<SpellAction>("shackle_undead", Priest::SHACKLE_UNDEAD));
            actions.push_back(::std::make_shared<SpellAction>("levitate", Priest::LEVITATE));
            break;
        }

        case CLASS_DEATH_KNIGHT:
        {
            // Death Knight universal combat abilities
            actions.push_back(::std::make_shared<SpellAction>("death_grip", DeathKnight::DEATH_GRIP));
            actions.push_back(::std::make_shared<SpellAction>("icebound_fortitude", DeathKnight::ICEBOUND_FORTITUDE));
            actions.push_back(::std::make_shared<SpellAction>("anti_magic_shell", DeathKnight::ANTI_MAGIC_SHELL));
            actions.push_back(::std::make_shared<SpellAction>("death_and_decay", DeathKnight::DEATH_AND_DECAY));
            actions.push_back(::std::make_shared<SpellAction>("mind_freeze", DeathKnight::MIND_FREEZE));
            actions.push_back(::std::make_shared<SpellAction>("chains_of_ice", DeathKnight::CHAINS_OF_ICE));
            actions.push_back(::std::make_shared<SpellAction>("death_strike", DeathKnight::DEATH_STRIKE));
            actions.push_back(::std::make_shared<SpellAction>("raise_ally", DeathKnight::RAISE_ALLY));
            actions.push_back(::std::make_shared<SpellAction>("control_undead", DeathKnight::CONTROL_UNDEAD));
            actions.push_back(::std::make_shared<SpellAction>("path_of_frost", DeathKnight::PATH_OF_FROST));
            actions.push_back(::std::make_shared<SpellAction>("raise_dead", DeathKnight::RAISE_DEAD));
            break;
        }

        case CLASS_SHAMAN:
        {
            // Shaman universal combat abilities
            actions.push_back(::std::make_shared<SpellAction>("wind_shear", Shaman::WIND_SHEAR));
            actions.push_back(::std::make_shared<SpellAction>("purge", Shaman::PURGE));
            actions.push_back(::std::make_shared<SpellAction>("hex", Shaman::HEX));
            actions.push_back(::std::make_shared<SpellAction>("astral_shift", Shaman::ASTRAL_SHIFT));
            actions.push_back(::std::make_shared<SpellAction>("ghost_wolf", Shaman::GHOST_WOLF));
            actions.push_back(::std::make_shared<SpellAction>("capacitor_totem", Shaman::CAPACITOR_TOTEM));
            actions.push_back(::std::make_shared<SpellAction>("earthbind_totem", Shaman::EARTHBIND_TOTEM));
            actions.push_back(::std::make_shared<SpellAction>("tremor_totem", Shaman::TREMOR_TOTEM));
            actions.push_back(::std::make_shared<SpellAction>("bloodlust", Shaman::BLOODLUST));
            actions.push_back(::std::make_shared<SpellAction>("heroism", Shaman::HEROISM));
            actions.push_back(::std::make_shared<SpellAction>("reincarnation", Shaman::REINCARNATION));
            break;
        }

        case CLASS_MAGE:
        {
            // Mage universal combat abilities
            actions.push_back(::std::make_shared<SpellAction>("ice_block", Mage::ICE_BLOCK));
            actions.push_back(::std::make_shared<SpellAction>("invisibility", Mage::INVISIBILITY));
            actions.push_back(::std::make_shared<SpellAction>("blink", Mage::BLINK));
            actions.push_back(::std::make_shared<SpellAction>("counterspell", Mage::COUNTERSPELL));
            actions.push_back(::std::make_shared<SpellAction>("polymorph", Mage::POLYMORPH));
            actions.push_back(::std::make_shared<SpellAction>("frost_nova", Mage::FROST_NOVA));
            actions.push_back(::std::make_shared<SpellAction>("remove_curse", Mage::REMOVE_CURSE));
            actions.push_back(::std::make_shared<SpellAction>("spellsteal", Mage::SPELLSTEAL));
            actions.push_back(::std::make_shared<SpellAction>("time_warp", Mage::TIME_WARP));
            actions.push_back(::std::make_shared<SpellAction>("arcane_intellect", Mage::ARCANE_INTELLECT));
            actions.push_back(::std::make_shared<SpellAction>("conjure_refreshment", Mage::CONJURE_REFRESHMENT));
            actions.push_back(::std::make_shared<SpellAction>("slow_fall", Mage::SLOW_FALL));
            break;
        }

        case CLASS_WARLOCK:
        {
            // Warlock universal combat abilities
            actions.push_back(::std::make_shared<SpellAction>("unending_resolve", Warlock::UNENDING_RESOLVE));
            actions.push_back(::std::make_shared<SpellAction>("fear", Warlock::FEAR));
            actions.push_back(::std::make_shared<SpellAction>("banish", Warlock::BANISH));
            actions.push_back(::std::make_shared<SpellAction>("mortal_coil", Warlock::MORTAL_COIL));
            actions.push_back(::std::make_shared<SpellAction>("demonic_gateway", Warlock::DEMONIC_GATEWAY));
            actions.push_back(::std::make_shared<SpellAction>("demonic_circle", Warlock::DEMONIC_CIRCLE));
            actions.push_back(::std::make_shared<SpellAction>("health_funnel", Warlock::HEALTH_FUNNEL));
            actions.push_back(::std::make_shared<SpellAction>("create_healthstone", Warlock::CREATE_HEALTHSTONE));
            actions.push_back(::std::make_shared<SpellAction>("create_soulwell", Warlock::CREATE_SOULWELL));
            actions.push_back(::std::make_shared<SpellAction>("ritual_of_summoning", Warlock::RITUAL_OF_SUMMONING));
            actions.push_back(::std::make_shared<SpellAction>("soulstone", Warlock::SOULSTONE));
            actions.push_back(::std::make_shared<SpellAction>("unending_breath", Warlock::UNENDING_BREATH));
            break;
        }

        // Note: CLASS_MONK (10) is intentionally excluded per requirements

        case CLASS_DRUID:
        {
            // Druid universal combat abilities
            actions.push_back(::std::make_shared<SpellAction>("barkskin", Druid::BARKSKIN));
            actions.push_back(::std::make_shared<SpellAction>("survival_instincts", Druid::SURVIVAL_INSTINCTS));
            actions.push_back(::std::make_shared<SpellAction>("bear_form", Druid::BEAR_FORM));
            actions.push_back(::std::make_shared<SpellAction>("cat_form", Druid::CAT_FORM));
            actions.push_back(::std::make_shared<SpellAction>("travel_form", Druid::TRAVEL_FORM));
            actions.push_back(::std::make_shared<SpellAction>("moonkin_form", Druid::MOONKIN_FORM));
            actions.push_back(::std::make_shared<SpellAction>("dash", Druid::DASH));
            actions.push_back(::std::make_shared<SpellAction>("stampeding_roar", Druid::STAMPEDING_ROAR));
            actions.push_back(::std::make_shared<SpellAction>("entangling_roots", Druid::ENTANGLING_ROOTS));
            actions.push_back(::std::make_shared<SpellAction>("hibernate", Druid::HIBERNATE));
            actions.push_back(::std::make_shared<SpellAction>("soothe", Druid::SOOTHE));
            actions.push_back(::std::make_shared<SpellAction>("rebirth", Druid::REBIRTH));
            actions.push_back(::std::make_shared<SpellAction>("mark_of_the_wild", Druid::MARK_OF_THE_WILD));
            actions.push_back(::std::make_shared<SpellAction>("remove_corruption", Druid::REMOVE_CORRUPTION));
            break;
        }

        case CLASS_DEMON_HUNTER:
        {
            // Demon Hunter universal combat abilities
            actions.push_back(::std::make_shared<SpellAction>("blur", DemonHunter::BLUR));
            actions.push_back(::std::make_shared<SpellAction>("darkness", DemonHunter::DARKNESS));
            actions.push_back(::std::make_shared<SpellAction>("fel_rush", DemonHunter::FEL_RUSH));
            actions.push_back(::std::make_shared<SpellAction>("vengeful_retreat", DemonHunter::VENGEFUL_RETREAT));
            actions.push_back(::std::make_shared<SpellAction>("consume_magic", DemonHunter::CONSUME_MAGIC));
            actions.push_back(::std::make_shared<SpellAction>("disrupt", DemonHunter::DISRUPT));
            actions.push_back(::std::make_shared<SpellAction>("imprison", DemonHunter::IMPRISON));
            actions.push_back(::std::make_shared<SpellAction>("spectral_sight", DemonHunter::SPECTRAL_SIGHT));
            actions.push_back(::std::make_shared<SpellAction>("glide", DemonHunter::GLIDE));
            break;
        }

        case CLASS_EVOKER:
        {
            // Evoker universal combat abilities
            actions.push_back(::std::make_shared<SpellAction>("obsidian_scales", Evoker::OBSIDIAN_SCALES));
            actions.push_back(::std::make_shared<SpellAction>("renewing_blaze", Evoker::RENEWING_BLAZE));
            actions.push_back(::std::make_shared<SpellAction>("wing_buffet", Evoker::WING_BUFFET));
            actions.push_back(::std::make_shared<SpellAction>("tail_swipe", Evoker::TAIL_SWIPE));
            actions.push_back(::std::make_shared<SpellAction>("hover", Evoker::HOVER));
            actions.push_back(::std::make_shared<SpellAction>("soar", Evoker::SOAR));
            actions.push_back(::std::make_shared<SpellAction>("landslide", Evoker::LANDSLIDE));
            actions.push_back(::std::make_shared<SpellAction>("oppressing_roar", Evoker::OPPRESSING_ROAR));
            actions.push_back(::std::make_shared<SpellAction>("quell", Evoker::QUELL));
            actions.push_back(::std::make_shared<SpellAction>("blessing_of_the_bronze", Evoker::BLESSING_OF_THE_BRONZE));
            actions.push_back(::std::make_shared<SpellAction>("source_of_magic", Evoker::SOURCE_OF_MAGIC));
            actions.push_back(::std::make_shared<SpellAction>("cauterizing_flame", Evoker::CAUTERIZING_FLAME));
            break;
        }

        default:
            TC_LOG_DEBUG("module.playerbot.action", "ActionFactory: Unknown class {} for CreateCombatActions", classId);
            break;
    }

    TC_LOG_DEBUG("module.playerbot.action", "ActionFactory: Created {} combat actions for class {}",
        actions.size(), classId);

    return actions;
}

::std::vector<::std::shared_ptr<Action>> ActionFactory::CreateMovementActions()
{
    ::std::vector<::std::shared_ptr<Action>> actions;

    // Core movement actions
    actions.push_back(::std::make_shared<MoveToPositionAction>());
    actions.push_back(::std::make_shared<FollowAction>());

    // Create specialized movement action subclasses inline
    // These extend MovementAction with specific behaviors

    // Flee action - Move away from danger/threats
    class FleeAction : public MovementAction
    {
    public:
        FleeAction() : MovementAction("flee") {}

        bool IsUseful(BotAI* ai) const override
        {
            // Useful when in combat and low health or facing overwhelming odds
            if (!ai)
                return false;
            return true; // Will be refined based on combat situation
        }

        ActionResult Execute(BotAI* ai, ActionContext const& context) override
        {
            if (!ai)
                return ActionResult::FAILED;

            // Calculate flee direction (opposite of threat/target)
            float x = context.x;
            float y = context.y;
            float z = context.z;

            if (x == 0 && y == 0 && z == 0)
            {
                // No specific position provided, flee from context target if available
                if (context.target)
                {
                    // Move opposite direction from target
                    return DoMove(ai, x, y, z) ? ActionResult::SUCCESS : ActionResult::FAILED;
                }
                return ActionResult::IMPOSSIBLE;
            }

            return DoMove(ai, x, y, z) ? ActionResult::SUCCESS : ActionResult::FAILED;
        }
    };
    actions.push_back(::std::make_shared<FleeAction>());

    // Spread action - Spread out from group members
    class SpreadAction : public MovementAction
    {
    public:
        SpreadAction() : MovementAction("spread") {}

        bool IsUseful(BotAI* ai) const override
        {
            // Useful during AoE mechanics or when grouped too closely
            if (!ai)
                return false;
            return true;
        }

        ActionResult Execute(BotAI* ai, ActionContext const& context) override
        {
            if (!ai)
                return ActionResult::FAILED;

            // Move to position that maximizes distance from other group members
            if (context.x == 0 && context.y == 0 && context.z == 0)
                return ActionResult::IMPOSSIBLE;

            return DoMove(ai, context.x, context.y, context.z) ? ActionResult::SUCCESS : ActionResult::FAILED;
        }
    };
    actions.push_back(::std::make_shared<SpreadAction>());

    // Stack action - Stack on a marker or player
    class StackAction : public MovementAction
    {
    public:
        StackAction() : MovementAction("stack") {}

        bool IsUseful(BotAI* ai) const override
        {
            // Useful for healing cooldowns, group mechanics
            if (!ai)
                return false;
            return true;
        }

        ActionResult Execute(BotAI* ai, ActionContext const& context) override
        {
            if (!ai)
                return ActionResult::FAILED;

            // Move to stack position (usually on a target player or marker)
            if (context.target)
            {
                WorldObject* target = context.target;
                return DoMove(ai, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ())
                    ? ActionResult::SUCCESS : ActionResult::FAILED;
            }

            if (context.x != 0 || context.y != 0 || context.z != 0)
                return DoMove(ai, context.x, context.y, context.z) ? ActionResult::SUCCESS : ActionResult::FAILED;

            return ActionResult::IMPOSSIBLE;
        }
    };
    actions.push_back(::std::make_shared<StackAction>());

    // Circle strafe action - Circle strafe around target
    class CircleStrafeAction : public MovementAction
    {
    public:
        CircleStrafeAction() : MovementAction("circle_strafe") {}

        bool IsUseful(BotAI* ai) const override
        {
            // Useful for melee DPS to avoid frontal attacks
            if (!ai)
                return false;
            return true;
        }

        ActionResult Execute(BotAI* ai, ActionContext const& context) override
        {
            if (!ai)
                return ActionResult::FAILED;

            // Move in a circular pattern around the target
            if (!context.target)
                return ActionResult::IMPOSSIBLE;

            // Calculate position on circle around target
            // The actual position should be provided by the positioning system
            if (context.x != 0 || context.y != 0 || context.z != 0)
                return DoMove(ai, context.x, context.y, context.z) ? ActionResult::SUCCESS : ActionResult::FAILED;

            return ActionResult::IMPOSSIBLE;
        }
    };
    actions.push_back(::std::make_shared<CircleStrafeAction>());

    // Kite action - Kite enemy while maintaining distance
    class KiteAction : public MovementAction
    {
    public:
        KiteAction() : MovementAction("kite") {}

        bool IsUseful(BotAI* ai) const override
        {
            // Useful for ranged classes or when slowing enemy
            if (!ai)
                return false;
            return true;
        }

        ActionResult Execute(BotAI* ai, ActionContext const& context) override
        {
            if (!ai)
                return ActionResult::FAILED;

            // Move away from target while maintaining attack range
            if (context.x == 0 && context.y == 0 && context.z == 0)
                return ActionResult::IMPOSSIBLE;

            return DoMove(ai, context.x, context.y, context.z) ? ActionResult::SUCCESS : ActionResult::FAILED;
        }
    };
    actions.push_back(::std::make_shared<KiteAction>());

    // Approach action - Move into optimal attack range
    class ApproachAction : public MovementAction
    {
    public:
        ApproachAction() : MovementAction("approach") {}

        bool IsUseful(BotAI* ai) const override
        {
            if (!ai)
                return false;
            return true;
        }

        ActionResult Execute(BotAI* ai, ActionContext const& context) override
        {
            if (!ai)
                return ActionResult::FAILED;

            if (!context.target)
                return ActionResult::IMPOSSIBLE;

            // Move to position within attack range of target
            if (context.x != 0 || context.y != 0 || context.z != 0)
                return DoMove(ai, context.x, context.y, context.z) ? ActionResult::SUCCESS : ActionResult::FAILED;

            // Default: move directly to target position
            WorldObject* target = context.target;
            return DoMove(ai, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ())
                ? ActionResult::SUCCESS : ActionResult::FAILED;
        }
    };
    actions.push_back(::std::make_shared<ApproachAction>());

    // Retreat action - Tactical retreat maintaining facing
    class RetreatAction : public MovementAction
    {
    public:
        RetreatAction() : MovementAction("retreat") {}

        bool IsUseful(BotAI* ai) const override
        {
            if (!ai)
                return false;
            return true;
        }

        ActionResult Execute(BotAI* ai, ActionContext const& context) override
        {
            if (!ai)
                return ActionResult::FAILED;

            // Move backward while maintaining facing toward threat
            if (context.x == 0 && context.y == 0 && context.z == 0)
                return ActionResult::IMPOSSIBLE;

            return DoMove(ai, context.x, context.y, context.z) ? ActionResult::SUCCESS : ActionResult::FAILED;
        }
    };
    actions.push_back(::std::make_shared<RetreatAction>());

    // Intercept action - Move to intercept a target
    class InterceptAction : public MovementAction
    {
    public:
        InterceptAction() : MovementAction("intercept") {}

        bool IsUseful(BotAI* ai) const override
        {
            if (!ai)
                return false;
            return true;
        }

        ActionResult Execute(BotAI* ai, ActionContext const& context) override
        {
            if (!ai)
                return ActionResult::FAILED;

            // Move to intercept position (where target will be)
            if (context.x == 0 && context.y == 0 && context.z == 0)
                return ActionResult::IMPOSSIBLE;

            return DoMove(ai, context.x, context.y, context.z) ? ActionResult::SUCCESS : ActionResult::FAILED;
        }
    };
    actions.push_back(::std::make_shared<InterceptAction>());

    // Patrol action - Move along patrol waypoints
    class PatrolAction : public MovementAction
    {
    public:
        PatrolAction() : MovementAction("patrol") {}

        bool IsUseful(BotAI* ai) const override
        {
            if (!ai)
                return false;
            return true;
        }

        ActionResult Execute(BotAI* ai, ActionContext const& context) override
        {
            if (!ai)
                return ActionResult::FAILED;

            // Move to next patrol waypoint
            if (context.x == 0 && context.y == 0 && context.z == 0)
                return ActionResult::IMPOSSIBLE;

            return DoMove(ai, context.x, context.y, context.z) ? ActionResult::SUCCESS : ActionResult::FAILED;
        }
    };
    actions.push_back(::std::make_shared<PatrolAction>());

    // Stop action - Halt all movement
    class StopAction : public MovementAction
    {
    public:
        StopAction() : MovementAction("stop") {}

        bool IsUseful(BotAI* /*ai*/) const override
        {
            return true; // Always useful to have stop capability
        }

        ActionResult Execute(BotAI* ai, ActionContext const& /*context*/) override
        {
            if (!ai)
                return ActionResult::FAILED;

            // Stopping movement handled by BotAI system
            // This action signals intent to stop
            return ActionResult::SUCCESS;
        }
    };
    actions.push_back(::std::make_shared<StopAction>());

    TC_LOG_DEBUG("module.playerbot.action", "ActionFactory: Created {} movement actions", actions.size());

    return actions;
}

::std::vector<::std::string> ActionFactory::GetAvailableActions() const
{
    ::std::vector<::std::string> actions;
    actions.reserve(_creators.size());

    for (auto const& pair : _creators)
        actions.push_back(pair.first);

    return actions;
}

bool ActionFactory::HasAction(::std::string const& name) const
{
    return _creators.find(name) != _creators.end();
}

} // namespace Playerbot
