/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "GrindStrategy.h"
#include "AI/BotAI.h"
#include "QuestStrategy.h"
#include "Player.h"
#include "Creature.h"
#include "GameObject.h"
#include "Map.h"
#include "CreatureAI.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "Movement/BotMovementUtil.h"
#include "Professions/GatheringManager.h"
#include "Professions/ProfessionManager.h"
#include "Quest/QuestHubDatabase.h"
#include "Core/Threading/SafeGridOperations.h"  // SEH-protected grid operations
#include "GameTime.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "QuestDef.h"

namespace Playerbot
{

GrindStrategy::GrindStrategy()
    : Strategy("grind")
{
    _priority = 40; // Below Quest (50), above Solo (10)
}

void GrindStrategy::InitializeActions()
{
    // GrindStrategy uses direct behavior rather than action system
}

void GrindStrategy::InitializeTriggers()
{
    // Triggers handled in UpdateBehavior
}

void GrindStrategy::InitializeValues()
{
    // Values handled internally
}

void GrindStrategy::OnActivate(BotAI* ai)
{
    if (!ai || !ai->GetBot())
        return;

    Player* bot = ai->GetBot();

    _isGrinding.store(true, std::memory_order_release);
    _state = GrindState::SCANNING;
    _lastKnownLevel = bot->GetLevel();
    _lastLevelXP = bot->GetXP();

    TC_LOG_INFO("module.playerbot.grind",
        "🎯 GrindStrategy ACTIVATED for bot {} (Level {}) - No quests available, entering grinding mode",
        bot->GetName(), bot->GetLevel());

    // Log profession integration status
    if (_professionIntegrationEnabled)
    {
        TC_LOG_INFO("module.playerbot.grind",
            "⛏️ GrindStrategy: Profession integration ENABLED - will gather ore/herbs and skin beasts");
    }
}

void GrindStrategy::OnDeactivate(BotAI* ai)
{
    if (!ai || !ai->GetBot())
        return;

    Player* bot = ai->GetBot();

    _isGrinding.store(false, std::memory_order_release);
    _state = GrindState::IDLE;
    _currentTarget = nullptr;
    _currentTargetGuid.Clear();

    TC_LOG_INFO("module.playerbot.grind",
        "🏁 GrindStrategy DEACTIVATED for bot {} - Killed {} mobs, gathered {} nodes, gained {} XP",
        bot->GetName(), _mobsKilled.load(), _gatheringNodesCollected.load(), _xpGained.load());
}

bool GrindStrategy::IsActive(BotAI* ai) const
{
    if (!_active)
        return false;

    return _isGrinding.load(std::memory_order_acquire);
}

float GrindStrategy::GetRelevance(BotAI* ai) const
{
    if (!ai || !ai->GetBot())
        return 0.0f;

    // Only relevant when grinding conditions are met
    if (!ShouldGrind(ai))
        return 0.0f;

    return 40.0f; // Priority 40
}

bool GrindStrategy::ShouldGrind(BotAI* ai) const
{
    if (!ai || !ai->GetBot())
        return false;

    Player* bot = ai->GetBot();

    // Don't grind if in a group (follow leader instead)
    if (bot->GetGroup())
        return false;

    // Don't grind if already in combat (let combat strategy handle)
    if (bot->IsInCombat())
        return false;

    // Check if QuestStrategy has exhausted options
    QuestStrategy* questStrategy = ai->GetStrategy<QuestStrategy>("quest");
    if (questStrategy)
    {
        // Only grind when quest strategy has failed 3+ times to find quests
        // This means: no quests, no quest givers within 300 yards, no quest hubs for level
        if (!questStrategy->HasExhaustedQuestOptions())
            return false;
    }

    // Check if bot has any active quests
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = bot->GetQuestSlotQuestId(slot);
        if (questId != 0)
        {
            // Bot has quests - don't grind
            return false;
        }
    }

    return true;
}

void GrindStrategy::UpdateBehavior(BotAI* ai, uint32 diff)
{
    if (!ai || !ai->GetBot())
        return;

    Player* bot = ai->GetBot();

    // Safety check
    if (!bot->IsInWorld())
        return;

    // ========================================================================
    // LEVEL-UP CHECK - Re-evaluate quest availability on level up
    // ========================================================================
    uint32 currentTime = GameTime::GetGameTimeMS();

    if (currentTime - _lastLevelUpCheck >= LEVEL_CHECK_INTERVAL)
    {
        _lastLevelUpCheck = currentTime;

        uint8 currentLevel = bot->GetLevel();
        if (currentLevel > _lastKnownLevel)
        {
            _lastKnownLevel = currentLevel;
            OnLevelUp(ai);

            // If quests became available, deactivate grinding
            if (CheckQuestAvailability(ai))
            {
                TC_LOG_INFO("module.playerbot.grind",
                    "📈 GrindStrategy: Bot {} leveled to {} - quests now available, deactivating grind mode",
                    bot->GetName(), currentLevel);

                SetActive(false);
                return;
            }
        }

        // Track XP gained
        uint32 currentXP = bot->GetXP();
        if (currentXP > _lastLevelXP)
        {
            _xpGained.fetch_add(currentXP - _lastLevelXP);
            _lastLevelXP = currentXP;
        }
    }

    // ========================================================================
    // STATE MACHINE
    // ========================================================================
    switch (_state)
    {
        case GrindState::IDLE:
            SetState(GrindState::SCANNING);
            break;

        case GrindState::SCANNING:
        {
            // Throttle scanning
            if (currentTime - _lastScanTime < SCAN_INTERVAL)
                return;
            _lastScanTime = currentTime;

            // Periodically check for quest givers (every 10 seconds during scanning)
            // This allows quick return to questing without having to wander first
            static constexpr uint32 QUEST_CHECK_INTERVAL = 10000; // 10 seconds
            if (currentTime - _lastQuestCheckTime >= QUEST_CHECK_INTERVAL)
            {
                _lastQuestCheckTime = currentTime;

                if (CheckQuestAvailability(ai))
                {
                    TC_LOG_INFO("module.playerbot.grind",
                        "🎯 GrindStrategy: Bot {} found quest givers during scan - returning to quest mode",
                        bot->GetName());

                    // Reset quest strategy failure counter so it will try again
                    QuestStrategy* questStrategy = ai->GetStrategy<QuestStrategy>("quest");
                    if (questStrategy)
                    {
                        questStrategy->ResetQuestSearchFailures();
                    }

                    SetActive(false);
                    return;
                }
            }

            // Check for gathering nodes first (if profession integration enabled)
            if (_professionIntegrationEnabled && CheckForGatheringNodes(ai))
            {
                SetState(GrindState::GATHERING);
                return;
            }

            // Find grind target
            Creature* target = FindGrindTarget(ai);
            if (target)
            {
                _currentTarget = target;
                _currentTargetGuid = target->GetGUID();
                _lastTargetPosition = target->GetPosition();

                TC_LOG_DEBUG("module.playerbot.grind",
                    "🎯 GrindStrategy: Bot {} found target {} (Level {}, HP: {}%)",
                    bot->GetName(), target->GetName(), target->GetLevel(),
                    static_cast<uint32>(target->GetHealthPct()));

                SetState(GrindState::MOVING);
            }
            else
            {
                // No targets found - wander to new area
                if (currentTime - _lastWanderTime >= WANDER_INTERVAL)
                {
                    _lastWanderTime = currentTime;
                    SetState(GrindState::WANDERING);
                }
            }
            break;
        }

        case GrindState::MOVING:
        {
            // Validate target still exists and is valid
            if (!_currentTarget || !_currentTarget->IsAlive() || !_currentTarget->IsInWorld())
            {
                _currentTarget = nullptr;
                _currentTargetGuid.Clear();
                SetState(GrindState::SCANNING);
                return;
            }

            // Check if we're close enough to engage
            float distance = bot->GetDistance(_currentTarget);
            if (distance <= MIN_TARGET_DISTANCE)
            {
                // Engage target
                bot->Attack(_currentTarget, true);
                _combatStartTime = currentTime;
                SetState(GrindState::COMBAT);

                TC_LOG_DEBUG("module.playerbot.grind",
                    "⚔️ GrindStrategy: Bot {} engaging {} at {:.1f} yards",
                    bot->GetName(), _currentTarget->GetName(), distance);
            }
            else if (distance > _pullRange)
            {
                // Target moved too far, find new target
                _currentTarget = nullptr;
                _currentTargetGuid.Clear();
                SetState(GrindState::SCANNING);
            }
            else
            {
                // Move towards target
                MoveToTarget(ai, _currentTarget);
            }
            break;
        }

        case GrindState::COMBAT:
        {
            // Combat is handled by SoloCombatStrategy and ClassAI
            // We just track when combat ends
            if (!bot->IsInCombat())
            {
                // Combat ended - check if we killed the target
                if (_currentTarget && !_currentTarget->IsAlive())
                {
                    _mobsKilled.fetch_add(1);

                    TC_LOG_DEBUG("module.playerbot.grind",
                        "💀 GrindStrategy: Bot {} killed {} (Total kills: {})",
                        bot->GetName(), _currentTarget->GetName(), _mobsKilled.load());

                    // Try skinning if applicable
                    if (_professionIntegrationEnabled && TrySkinCreature(ai, _currentTarget))
                    {
                        SetState(GrindState::SKINNING);
                    }
                    else
                    {
                        SetState(GrindState::LOOTING);
                    }
                }
                else
                {
                    // Combat ended but target not dead (evade, flee, etc.)
                    _currentTarget = nullptr;
                    _currentTargetGuid.Clear();
                    SetState(GrindState::SCANNING);
                }
            }
            break;
        }

        case GrindState::LOOTING:
        {
            // Looting is handled by LootStrategy
            // Wait briefly then return to scanning
            // LootStrategy will handle the actual looting
            SetState(GrindState::SCANNING);
            _currentTarget = nullptr;
            _currentTargetGuid.Clear();
            break;
        }

        case GrindState::SKINNING:
        {
            // Skinning handled by GatheringManager
            // Wait for skinning to complete, then return to scanning
            GatheringManager* gatherMgr = ai->GetGatheringManager();
            if (!gatherMgr || !gatherMgr->IsGathering())
            {
                SetState(GrindState::SCANNING);
                _currentTarget = nullptr;
                _currentTargetGuid.Clear();
            }
            break;
        }

        case GrindState::GATHERING:
        {
            // Gathering handled by GatheringManager
            GatheringManager* gatherMgr = ai->GetGatheringManager();
            if (!gatherMgr || !gatherMgr->IsGathering())
            {
                _gatheringNodesCollected.fetch_add(1);
                SetState(GrindState::SCANNING);
            }
            break;
        }

        case GrindState::WANDERING:
        {
            // Move to new area
            if (WanderToNewArea(ai))
            {
                // Wait for movement to complete
                if (!bot->isMoving())
                {
                    // After arriving at new location, check for quest givers
                    // This allows quick return to questing if we wandered into range of quest givers
                    if (CheckQuestAvailability(ai))
                    {
                        TC_LOG_INFO("module.playerbot.grind",
                            "🎯 GrindStrategy: Bot {} found quest givers after wandering - returning to quest mode",
                            bot->GetName());

                        // Reset quest strategy failure counter so it will try again
                        QuestStrategy* questStrategy = ai->GetStrategy<QuestStrategy>("quest");
                        if (questStrategy)
                        {
                            questStrategy->ResetQuestSearchFailures();
                        }

                        SetActive(false);
                        return;
                    }

                    SetState(GrindState::SCANNING);
                }
            }
            else
            {
                SetState(GrindState::SCANNING);
            }
            break;
        }

        case GrindState::RESTING:
        {
            // RestStrategy handles recovery
            // Check if ready to resume
            float healthPct = bot->GetHealthPct() / 100.0f;
            float manaPct = bot->GetPowerType() == POWER_MANA ?
                (bot->GetPower(POWER_MANA) / static_cast<float>(bot->GetMaxPower(POWER_MANA))) : 1.0f;

            if (healthPct >= 0.8f && manaPct >= 0.6f)
            {
                SetState(GrindState::SCANNING);
            }
            break;
        }
    }
}

Creature* GrindStrategy::FindGrindTarget(BotAI* ai)
{
    if (!ai || !ai->GetBot())
        return nullptr;

    Player* bot = ai->GetBot();

    if (!bot->IsInWorld())
        return nullptr;

    // THREAD-SAFE: Use SafeGridOperations with SEH protection to catch access violations
    std::list<Creature*> nearbyCreatures;
    if (!SafeGridOperations::GetCreatureListSafe(bot, nearbyCreatures, 0, _scanRange))
        return nullptr;

    if (nearbyCreatures.empty())
        return nullptr;

    Creature* bestTarget = nullptr;
    float bestScore = 0.0f;

    for (Creature* creature : nearbyCreatures)
    {
        if (!IsValidGrindTarget(bot, creature))
            continue;

        float score = CalculateTargetScore(bot, creature);
        if (score > bestScore)
        {
            bestScore = score;
            bestTarget = creature;
        }
    }

    return bestTarget;
}

float GrindStrategy::CalculateTargetScore(Player* bot, Creature* creature) const
{
    if (!bot || !creature)
        return 0.0f;

    float score = 100.0f; // Base score

    // Distance factor (closer = better)
    float distance = bot->GetDistance(creature);
    score -= distance * 0.5f; // -0.5 points per yard

    // Level factor (same level = best, lower = easier, higher = more XP)
    int levelDiff = static_cast<int>(creature->GetLevel()) - static_cast<int>(bot->GetLevel());
    if (levelDiff < 0)
        score -= std::abs(levelDiff) * 5.0f; // Lower level = less desirable
    else if (levelDiff > 0)
        score += levelDiff * 2.0f; // Higher level = slightly more desirable (more XP)

    // Health factor (full health = better target)
    if (creature->GetHealthPct() < 100)
        score -= (100 - creature->GetHealthPct()) * 0.2f;

    // Profession priority modifier
    score *= GetProfessionPriorityModifier(bot, creature);

    // Prefer creatures not in combat
    if (creature->IsInCombat())
        score *= 0.3f;

    // Avoid creatures with many adds nearby
    if (!IsSafeToPull(bot, creature))
        score *= 0.5f;

    return std::max(0.0f, score);
}

bool GrindStrategy::IsValidGrindTarget(Player* bot, Creature* creature) const
{
    if (!bot || !creature)
        return false;

    // Must be alive
    if (!creature->IsAlive())
        return false;

    // Must be in world
    if (!creature->IsInWorld())
        return false;

    // CRITICAL: Bot must also be in world
    if (!bot->IsInWorld())
        return false;

    // CRITICAL: Re-verify creature has valid map (TOCTOU race)
    // NOTE: Use FindMap() instead of GetMap() - GetMap() has ASSERT(m_currMap) which crashes
    if (!creature->FindMap())
        return false;

    // NOTE: CanSeeOrDetect() is NOT SAFE to call from worker thread!
    // It accesses Map data which can cause ASSERTION FAILED: !IsInWorld() in ResetMap
    // Use same-map check instead. Phase visibility validated during actual combat.
    if (creature->GetMapId() != bot->GetMapId())
        return false;

    // Must be hostile
    if (!bot->IsHostileTo(creature))
        return false;

    // Must be attackable
    if (!bot->IsValidAttackTarget(creature))
        return false;

    // Must be in level range
    if (!IsLevelAppropriate(bot, creature))
        return false;

    // Avoid elites unless significantly lower level
    if (creature->IsElite())
    {
        int levelDiff = static_cast<int>(bot->GetLevel()) - static_cast<int>(creature->GetLevel());
        if (levelDiff < 5) // Need to be 5+ levels above elite
            return false;
    }

    // Avoid rare/boss mobs
    if (creature->IsDungeonBoss())
        return false;

    // Avoid evading creatures
    if (creature->IsEvadingAttacks())
        return false;

    // Avoid creatures already in combat with others (tagging)
    if (creature->IsInCombat() && creature->GetVictim() != bot)
    {
        // Check if it's another player's mob
        Unit* victim = creature->GetVictim();
        if (victim && victim->GetTypeId() == TYPEID_PLAYER)
            return false;
    }

    return true;
}

bool GrindStrategy::IsLevelAppropriate(Player* bot, Creature* creature) const
{
    if (!bot || !creature)
        return false;

    int botLevel = static_cast<int>(bot->GetLevel());
    int creatureLevel = static_cast<int>(creature->GetLevel());
    int diff = std::abs(botLevel - creatureLevel);

    // Within configured level range
    return diff <= static_cast<int>(_levelRange);
}

bool GrindStrategy::IsSafeToPull(Player* bot, Creature* creature) const
{
    if (!bot || !creature)
        return false;

    // CRITICAL: Must be in world before any grid/map operations
    if (!creature->IsInWorld())
        return false;

    // THREAD-SAFE: Use SafeGridOperations with SEH protection to catch access violations
    std::list<Creature*> nearbyMobs;
    if (!SafeGridOperations::GetCreatureListFromCreatureSafe(creature, nearbyMobs, 0, 10.0f)) // 10 yard radius
        return false;

    uint32 hostileCount = 0;
    for (Creature* nearby : nearbyMobs)
    {
        if (nearby == creature)
            continue;

        if (!nearby->IsAlive())
            continue;

        if (bot->IsHostileTo(nearby))
            hostileCount++;
    }

    // Safe if 2 or fewer additional mobs nearby
    return hostileCount <= 2;
}

float GrindStrategy::GetProfessionPriorityModifier(Player* bot, Creature* creature) const
{
    if (!bot || !creature || !_professionIntegrationEnabled)
        return 1.0f;

    float modifier = 1.0f;

    uint32 creatureType = creature->GetCreatureType();

    // Humanoid priority for cloth farming (tailoring)
    if (creatureType == CREATURE_TYPE_HUMANOID)
    {
        // Check if bot has tailoring
        if (bot->HasSkill(SKILL_TAILORING))
            modifier *= _humanoidPriority;
    }

    // Beast priority for skinning
    if (creatureType == CREATURE_TYPE_BEAST)
    {
        // Check if bot has skinning and creature is skinnable
        if (bot->HasSkill(SKILL_SKINNING))
        {
            // Check creature flags for skinnable
            if (creature->GetCreatureDifficulty()->SkinLootID > 0)
                modifier *= _beastPriority;
        }
    }

    // Dragonkin can also be skinned
    if (creatureType == CREATURE_TYPE_DRAGONKIN)
    {
        if (bot->HasSkill(SKILL_SKINNING))
        {
            if (creature->GetCreatureDifficulty()->SkinLootID > 0)
                modifier *= _beastPriority;
        }
    }

    return modifier;
}

bool GrindStrategy::CheckForGatheringNodes(BotAI* ai)
{
    if (!ai)
        return false;

    GatheringManager* gatherMgr = ai->GetGatheringManager();
    if (!gatherMgr)
        return false;

    // Let GatheringManager handle detection and gathering
    if (gatherMgr->HasNearbyResources())
    {
        auto nodes = gatherMgr->ScanForNodes(_scanRange);
        if (!nodes.empty())
        {
            // GatheringManager will handle pathing and gathering
            return true;
        }
    }

    return false;
}

bool GrindStrategy::TrySkinCreature(BotAI* ai, Creature* creature)
{
    if (!ai || !creature)
        return false;

    Player* bot = ai->GetBot();
    if (!bot)
        return false;

    // Check if bot has skinning
    if (!bot->HasSkill(SKILL_SKINNING))
        return false;

    // Check if creature is skinnable
    if (creature->GetCreatureDifficulty()->SkinLootID == 0)
        return false;

    // Check creature type
    uint32 creatureType = creature->GetCreatureType();
    if (creatureType != CREATURE_TYPE_BEAST &&
        creatureType != CREATURE_TYPE_DRAGONKIN)
        return false;

    // Use GatheringManager for skinning
    GatheringManager* gatherMgr = ai->GetGatheringManager();
    if (gatherMgr)
    {
        return gatherMgr->SkinCreature(creature);
    }

    return false;
}

void GrindStrategy::OnLevelUp(BotAI* ai)
{
    if (!ai || !ai->GetBot())
        return;

    Player* bot = ai->GetBot();

    TC_LOG_INFO("module.playerbot.grind",
        "📈 GrindStrategy: Bot {} leveled up to {} while grinding!",
        bot->GetName(), bot->GetLevel());

    // Reset XP tracking
    _lastLevelXP = 0;

    // Set flag to check quest availability on next update
    _questCheckPending.store(true);
}

bool GrindStrategy::CheckQuestAvailability(BotAI* ai)
{
    if (!ai || !ai->GetBot())
        return false;

    Player* bot = ai->GetBot();

    // CRITICAL: Must be in world before any grid/map operations
    if (!bot->IsInWorld())
        return false;

    // Check quest hub database for level-appropriate hubs
    auto& hubDb = QuestHubDatabase::Instance();
    if (!hubDb.IsInitialized())
        return false;

    auto questHubs = hubDb.GetQuestHubsForPlayer(bot, 3);
    if (!questHubs.empty())
    {
        TC_LOG_INFO("module.playerbot.grind",
            "✅ GrindStrategy: Found {} quest hubs for level {} bot {}",
            questHubs.size(), bot->GetLevel(), bot->GetName());
        return true;
    }

    // Also check for nearby quest givers (300 yard range)
    // THREAD-SAFE: Use SafeGridOperations with SEH protection to catch access violations
    std::list<Creature*> nearbyCreatures;
    if (!SafeGridOperations::GetCreatureListSafe(bot, nearbyCreatures, 0, 300.0f))
        return false;

    for (Creature* creature : nearbyCreatures)
    {
        // Full validity check before accessing creature methods
        // With 300-yard range, creatures may despawn/become invalid during iteration
        if (!creature || !creature->IsAlive() || !creature->IsInWorld())
            continue;

        // CRITICAL: Double-check bot is still in world
        if (!bot->IsInWorld())
            return false;

        // CRITICAL: Re-verify creature validity (TOCTOU race)
        // NOTE: Use FindMap() instead of GetMap() - GetMap() has ASSERT(m_currMap) which crashes
        if (!creature->IsInWorld() || !creature->FindMap())
            continue;

        // NOTE: CanSeeOrDetect() is NOT SAFE to call from worker thread!
        // Use same-map check instead. Phase visibility validated during actual interaction.
        if (creature->GetMapId() != bot->GetMapId())
            continue;

        if (!creature->IsQuestGiver())
            continue;

        // Check if creature has quests for this bot
        auto questRelations = sObjectMgr->GetCreatureQuestRelations(creature->GetEntry());
        for (uint32 questId : questRelations)
        {
            Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
            if (!quest)
                continue;

            // Check if bot can take quest (handles all eligibility checks including level)
            if (bot->CanTakeQuest(quest, false))
            {
                TC_LOG_INFO("module.playerbot.grind",
                    "✅ GrindStrategy: Found quest giver {} with available quest {} for bot {}",
                    creature->GetName(), quest->GetLogTitle(), bot->GetName());
                return true;
            }
        }
    }

    return false;
}

bool GrindStrategy::MoveToTarget(BotAI* ai, Creature* target)
{
    if (!ai || !ai->GetBot() || !target)
        return false;

    Player* bot = ai->GetBot();

    Position targetPos;
    targetPos.Relocate(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ());

    return BotMovementUtil::MoveToPosition(bot, targetPos);
}

bool GrindStrategy::WanderToNewArea(BotAI* ai)
{
    if (!ai || !ai->GetBot())
        return false;

    Player* bot = ai->GetBot();

    // Generate random position within wander distance
    float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * static_cast<float>(M_PI);
    float distance = WANDER_DISTANCE * 0.5f + (static_cast<float>(rand()) / RAND_MAX * WANDER_DISTANCE * 0.5f);

    float x = bot->GetPositionX() + distance * std::cos(angle);
    float y = bot->GetPositionY() + distance * std::sin(angle);
    float z = bot->GetPositionZ();

    // Get proper ground height
    Map* map = bot->GetMap();
    if (map)
    {
        z = map->GetHeight(bot->GetPhaseShift(), x, y, z);
    }

    Position wanderPos;
    wanderPos.Relocate(x, y, z);

    TC_LOG_DEBUG("module.playerbot.grind",
        "🚶 GrindStrategy: Bot {} wandering to new area ({:.1f}, {:.1f}, {:.1f})",
        bot->GetName(), x, y, z);

    return BotMovementUtil::MoveToPosition(bot, wanderPos);
}

bool GrindStrategy::HasTargetsInArea(BotAI* ai) const
{
    if (!ai || !ai->GetBot())
        return false;

    Player* bot = ai->GetBot();

    // CRITICAL: Must be in world before any grid/map operations
    if (!bot->IsInWorld())
        return false;

    // THREAD-SAFE: Use SafeGridOperations with SEH protection to catch access violations
    std::list<Creature*> nearbyCreatures;
    if (!SafeGridOperations::GetCreatureListSafe(bot, nearbyCreatures, 0, _scanRange))
        return false;

    for (Creature* creature : nearbyCreatures)
    {
        if (IsValidGrindTarget(bot, creature))
            return true;
    }

    return false;
}

void GrindStrategy::SetState(GrindState state)
{
    if (_state != state)
    {
        TC_LOG_TRACE("module.playerbot.grind",
            "GrindStrategy: State change {} -> {}",
            GetStateName(), GetStateName());
        _state = state;
    }
}

const char* GrindStrategy::GetStateName() const
{
    switch (_state)
    {
        case GrindState::IDLE:      return "IDLE";
        case GrindState::SCANNING:  return "SCANNING";
        case GrindState::MOVING:    return "MOVING";
        case GrindState::COMBAT:    return "COMBAT";
        case GrindState::LOOTING:   return "LOOTING";
        case GrindState::SKINNING:  return "SKINNING";
        case GrindState::GATHERING: return "GATHERING";
        case GrindState::WANDERING: return "WANDERING";
        case GrindState::RESTING:   return "RESTING";
        default:                    return "UNKNOWN";
    }
}

} // namespace Playerbot
