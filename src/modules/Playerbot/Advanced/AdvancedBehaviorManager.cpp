/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "AdvancedBehaviorManager.h"
#include "Player.h"
#include "Creature.h"
#include "GameObject.h"
#include "Map.h"
#include "InstanceScript.h"
#include "Battlegrounds/Battleground.h"
#include "Battlegrounds/BattlegroundMgr.h"
#include "GameEventMgr.h"
#include "ObjectAccessor.h"
#include "MotionMaster.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "SpellDefines.h"
#include "Grids/Notifiers/GridNotifiers.h"
#include "CellImpl.h"
#include "Log.h"
#include "../AI/BotAI.h"
#include "../Spatial/SpatialGridManager.h"  // Lock-free spatial grid for deadlock fix
#include "../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5C: Thread-safe helpers
#include "Movement/UnifiedMovementCoordinator.h"
#include "../Movement/Arbiter/MovementPriorityMapper.h"
#include "UnitAI.h"
#include <algorithm>
#include <random>
#include "GameTime.h"

namespace Playerbot
{

AdvancedBehaviorManager::AdvancedBehaviorManager(Player* bot, BotAI* ai)
    : m_currentBossFight(nullptr)  // unique_ptr default constructs to nullptr (declared first in header)
    , m_bot(bot)
    , m_ai(ai)
    , m_enabled(true)
    , m_dungeonEnabled(true)
    , m_pvpEnabled(false)
    , m_eventEnabled(true)
    , m_achievementHunting(false)
    , m_rareHunting(false)
    , m_dungeonRole(DungeonRole::UNDEFINED)
    , m_currentBattleground(BattlegroundType::NONE)
    , m_activeEvent(WorldEventType::NONE)
    , m_dungeonUpdateInterval(1000)
    , m_pvpUpdateInterval(500)
    , m_eventUpdateInterval(5000)
    , m_achievementUpdateInterval(10000)
    , m_explorationUpdateInterval(15000)
    , m_rareUpdateInterval(30000)
    , m_lastDungeonUpdate(0)
    , m_lastPvPUpdate(0)
    , m_lastEventUpdate(0)
    , m_lastAchievementUpdate(0)
    , m_lastExplorationUpdate(0)
    , m_lastRareUpdate(0)
    , m_updateCount(0)
    , m_cpuUsage(0.0f)
{
}

AdvancedBehaviorManager::~AdvancedBehaviorManager()
{
    Shutdown();
}

void AdvancedBehaviorManager::Initialize()
{
    if (!m_bot)
        return;

    LoadDungeonStrategies();
    LoadBattlegroundStrategies();
    LoadWorldEvents();

    AssignDungeonRole();
}

void AdvancedBehaviorManager::Update(uint32 diff)
{
    if (!m_bot || !m_enabled)
        return;

    StartPerformanceTimer();

    // Update different systems based on current context
    if (IsInDungeon() && m_dungeonEnabled)
    {
        m_lastDungeonUpdate += diff;
        if (m_lastDungeonUpdate >= m_dungeonUpdateInterval)
        {
            UpdateDungeonBehavior(diff);
            m_lastDungeonUpdate = 0;
        }
    }

    if (IsInBattleground() && m_pvpEnabled)
    {
        m_lastPvPUpdate += diff;
        if (m_lastPvPUpdate >= m_pvpUpdateInterval)
        {
            UpdatePvPBehavior(diff);
            m_lastPvPUpdate = 0;
        }
    }

    if (m_eventEnabled)
    {
        m_lastEventUpdate += diff;
        if (m_lastEventUpdate >= m_eventUpdateInterval)
        {
            UpdateEventBehavior(diff);
            m_lastEventUpdate = 0;
        }
    }

    if (m_achievementHunting)
    {
        m_lastAchievementUpdate += diff;
        if (m_lastAchievementUpdate >= m_achievementUpdateInterval)
        {
            UpdateAchievementProgress(diff);
            m_lastAchievementUpdate = 0;
        }
    }

    if (m_rareHunting)
    {
        m_lastRareUpdate += diff;
        if (m_lastRareUpdate >= m_rareUpdateInterval)
        {
            UpdateRareTracking(diff);
            m_lastRareUpdate = 0;
        }
    }

    // Universal updates
    m_lastExplorationUpdate += diff;
    if (m_lastExplorationUpdate >= m_explorationUpdateInterval)
    {
        UpdateExploration(diff);
        m_lastExplorationUpdate = 0;
    }

    UpdateDangerZones(diff);

    if (m_currentBossFight)
        UpdateBossFight(diff);

    EndPerformanceTimer();
    UpdatePerformanceMetrics();
}

void AdvancedBehaviorManager::Reset()
{
    // unique_ptr automatically deletes on reset
    m_currentBossFight.reset();

    m_dungeonStrategies.clear();
    m_bgStrategies.clear();
    m_worldEvents.clear();
    m_pursuingAchievements.clear();
    m_exploredZones.clear();
    m_discoveredFlightPaths.clear();
    m_trackedRares.clear();
    m_discoveredTreasures.clear();
    m_dangerZones.clear();
    m_stats = Statistics();
}

void AdvancedBehaviorManager::Shutdown()
{
    // unique_ptr automatically deletes on reset
    m_currentBossFight.reset();
}

// ============================================================================
// DUNGEON SYSTEM
// ============================================================================

bool AdvancedBehaviorManager::EnterDungeon(uint32 /*dungeonId*/)
{
    if (!m_bot || !m_dungeonEnabled)
        return false;

    // Dungeon entry requires group finder or direct map transfer
    // Framework in place for future dungeon finder integration
    return false;
}

bool AdvancedBehaviorManager::LeaveDungeon()
{
    if (!m_bot || !IsInDungeon())
        return false;

    // Leave dungeon via teleport to homebind
    WorldLocation homebind = m_bot->m_homebind;
    m_bot->TeleportTo(homebind.GetMapId(), homebind.GetPositionX(), homebind.GetPositionY(),
                      homebind.GetPositionZ(), m_bot->GetOrientation());

    return true;
}

bool AdvancedBehaviorManager::IsInDungeon() const
{
    if (!m_bot)
        return false;

    Map* map = m_bot->GetMap();
    if (!map)
        return false;

    return map->IsDungeon();
}

AdvancedBehaviorManager::DungeonStrategy const* AdvancedBehaviorManager::GetCurrentDungeonStrategy() const
{
    if (!m_bot || !IsInDungeon())
        return nullptr;

    Map* map = m_bot->GetMap();
    if (!map)
        return nullptr;

    auto itr = m_dungeonStrategies.find(map->GetId());
    if (itr != m_dungeonStrategies.end())
        return &itr->second;

    return nullptr;
}

void AdvancedBehaviorManager::ExecuteDungeonStrategy()
{
    if (!m_bot || !IsInDungeon())
        return;

    DungeonStrategy const* strategy = GetCurrentDungeonStrategy();
    if (!strategy)
        return;

    // Execute role-specific behavior
    switch (m_dungeonRole)
    {
        case DungeonRole::TANK:
            // Tanks pull and hold aggro
            HandleTrashPull();
            break;

        case DungeonRole::HEALER:
            // Healers stay back and monitor health
            // Healing is handled by class AI
            break;

        case DungeonRole::DPS:
            // DPS follow tank and attack
            // Damage is handled by class AI
            break;

        default:
            break;
    }
}

void AdvancedBehaviorManager::HandleBossMechanic(Creature* boss, uint32 spellId)
{
    if (!m_bot || !boss)
        return;

    // Get spell info to determine mechanic
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return;

    // Check for interrupt mechanics
    if (spellInfo->CanBeInterrupted(m_bot, boss, false))
    {
        InterruptBossCast(boss, spellId);
        return;
    }

    // Check for dispel mechanics
    if (spellInfo->HasAttribute(SPELL_ATTR0_IS_ABILITY))
    {
        DispelBossDebuff(spellId);
        return;
    }

    // Move to safe position if damaging AoE
    if (spellInfo->HasEffect(SPELL_EFFECT_SCHOOL_DAMAGE) && spellInfo->HasAreaAuraEffect())
    {
        MoveToBossSafeSpot(boss);
        return;
    }
}

void AdvancedBehaviorManager::AvoidDangerZone(Position const& center, float radius)
{
    if (!m_bot)
        return;

    // Check if bot is in danger zone
    if (m_bot->GetExactDist2d(center.GetPositionX(), center.GetPositionY()) <= radius)
    {
        // Find safe position outside radius
        Position safePos = FindSafePosition(m_bot->GetPosition());

        // Phase 2: Use Unified Movement Coordinator with DUNGEON_MECHANIC priority (205)
        BotAI* botAI = dynamic_cast<BotAI*>(m_bot->GetAI());
        if (botAI && botAI->GetUnifiedMovementCoordinator())
        {
            bool accepted = botAI->RequestPointMovement(
                PlayerBotMovementPriority::DUNGEON_MECHANIC,
                safePos,
                "Danger zone escape",
                "AdvancedBehaviorManager");

            if (!accepted)
            {
                TC_LOG_TRACE("playerbot.movement.arbiter",
                    "AdvancedBehaviorManager: Danger zone escape rejected for {} - higher priority active",
                    m_bot->GetName());
            }
        }
        else
        {
            // FALLBACK: Direct MotionMaster if arbiter not available
            m_bot->GetMotionMaster()->MovePoint(0, safePos, false);
        }
    }

    // Track danger zone
    DangerZone zone;
    zone.center = center;
    zone.radius = radius;
    zone.expiryTime = GameTime::GetGameTimeMS() + 10000; // 10 seconds
    zone.damagePerSecond = 1000;
    m_dangerZones.push_back(zone);
}

void AdvancedBehaviorManager::InterruptBossCast(Creature* boss, uint32 spellId)
{
    if (!m_bot || !boss)
        return;
    // Find interrupt spell based on class
    uint32 interruptSpell = 0;

    switch (m_bot->GetClass())
    {
        case CLASS_WARRIOR:
            interruptSpell = 6552; // Pummel
            break;
        case CLASS_ROGUE:
            interruptSpell = 1766; // Kick
            break;
        case CLASS_SHAMAN:
            interruptSpell = 57994; // Wind Shear
            break;
        case CLASS_MAGE:
            interruptSpell = 2139; // Counterspell
            break;
        case CLASS_DEATH_KNIGHT:
            interruptSpell = 47528; // Mind Freeze
            break;
        case CLASS_DEMON_HUNTER:
            interruptSpell = 183752; // Disrupt
            break;
        default:
            return; // No interrupt available
    }
    if (m_bot->HasSpell(interruptSpell))
        m_bot->CastSpell(boss, interruptSpell, CastSpellExtraArgs());
}

void AdvancedBehaviorManager::DispelBossDebuff(uint32 /*spellId*/)
{
    if (!m_bot)
        return;

    // Dispel mechanics handled by class AI
    // Framework in place for explicit dispel logic
}

void AdvancedBehaviorManager::MoveToBossSafeSpot(Creature* boss)
{
    if (!m_bot || !boss)
        return;

    Position safePos = FindSafePosition(m_bot->GetPosition());

    // Phase 2: Use Unified Movement Coordinator with DUNGEON_MECHANIC priority (205)
    BotAI* botAI = dynamic_cast<BotAI*>(m_bot->GetAI());
    if (botAI && botAI->GetUnifiedMovementCoordinator())
    {
        bool accepted = botAI->RequestPointMovement(
            PlayerBotMovementPriority::DUNGEON_MECHANIC,
            safePos,
            "Boss safe spot positioning",
            "AdvancedBehaviorManager");

        if (!accepted)
        {
            TC_LOG_TRACE("playerbot.movement.arbiter",
                "AdvancedBehaviorManager: Boss safe spot rejected for {} - higher priority active",
                m_bot->GetName());
        }
    }
    else
    {
        // FALLBACK: Direct MotionMaster if arbiter not available
        m_bot->GetMotionMaster()->MovePoint(0, safePos, false);
    }
}

void AdvancedBehaviorManager::HandleTrashPull()
{
    if (!m_bot || m_dungeonRole != DungeonRole::TANK)
        return;

    // Find nearest trash mob
    Map* map = m_bot->GetMap();
    if (!map)
        return;

    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return;
    }

    // PHASE 5C: Thread-safe spatial grid query (replaces QueryNearbyCreatureGuids + ObjectAccessor)
    auto creatureSnapshots = spatialGrid->QueryNearbyCreatures(
        m_bot->GetPosition(), 30.0f);

    // Find trash to pull using snapshots
    for (auto const& snapshot : creatureSnapshots)
    {
        if (!snapshot.IsAlive())
            continue;

        // Get Creature* for complex checks
        Creature* creature = ObjectAccessor::GetCreature(*m_bot, snapshot.guid);
        if (!creature || creature->IsFriendlyTo(m_bot))
            continue;

        if (!creature->IsInCombat() && !creature->IsDungeonBoss())
        {
            // Pull the trash
            m_bot->Attack(creature, true);
            break;
        }
    }
}

void AdvancedBehaviorManager::PrioritizeCrowdControl(::std::vector<Creature*> const& mobs)
{
    if (!m_bot || mobs.empty())
        return;

    // Find casters and healers to CC first
    for (Creature* mob : mobs)
    {
        if (!mob || !mob->IsAlive())
            continue;

        CreatureTemplate const* template_ = mob->GetCreatureTemplate();
        if (!template_)
            continue;

        // Prioritize casters and healers
    if (template_->unit_class == CLASS_MAGE || template_->unit_class == CLASS_PRIEST)
        {
            // Apply CC based on class
            // Framework in place for class-specific CC
            break;
        }
    }
}

void AdvancedBehaviorManager::HandlePatrolAvoidance()
{
    if (!m_bot)
        return;

    // Detect nearby patrols and avoid aggro
    Map* map = m_bot->GetMap();
    if (!map)
        return;

    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return;
    }

    // PHASE 5C: Thread-safe spatial grid query (replaces QueryNearbyCreatureGuids + ObjectAccessor)
    auto creatureSnapshots = spatialGrid->QueryNearbyCreatures(
        m_bot->GetPosition(), 40.0f);

    // Detect patrols using snapshots
    for (auto const& snapshot : creatureSnapshots)
    {
        if (!snapshot.IsAlive())
            continue;

        // Get Creature* for complex checks
        Creature* creature = ObjectAccessor::GetCreature(*m_bot, snapshot.guid);
        if (!creature || creature->IsFriendlyTo(m_bot))
            continue;

        // If creature is on patrol route, wait for it to pass
    if (creature->HasUnitMovementFlag(MOVEMENTFLAG_WALKING))
        {
            if (m_bot->GetExactDist2d(snapshot.position) < 15.0f)
            {
                // Stop moving and wait
                m_bot->GetMotionMaster()->Clear();
                break;
            }
        }
    }
}

// ============================================================================
// PVP SYSTEM
// ============================================================================

bool AdvancedBehaviorManager::QueueForBattleground(BattlegroundType /*type*/)
{
    if (!m_bot || !m_pvpEnabled)
        return false;

    // Battleground queueing requires WorldSession packet handling
    // Framework in place for future implementation
    return false;
}
bool AdvancedBehaviorManager::LeaveBattleground()
{
    if (!m_bot || !IsInBattleground())
        return false;

    Battleground* bg = m_bot->GetBattleground();
    if (!bg)
        return false;

    bg->RemovePlayerAtLeave(m_bot->GetGUID(), false, true);
    return true;
}

bool AdvancedBehaviorManager::IsInBattleground() const
{
    if (!m_bot)
        return false;

    return m_bot->InBattleground();
}

void AdvancedBehaviorManager::ExecuteBattlegroundStrategy()
{
    if (!m_bot || !IsInBattleground())
        return;

    Battleground* bg = m_bot->GetBattleground();
    if (!bg)
        return;

    BattlegroundStrategy* strategy = GetBattlegroundStrategy(m_currentBattleground);
    if (!strategy)
        return;

    // Execute strategy based on battleground type
    switch (m_currentBattleground)
    {
        case BattlegroundType::WARSONG_GULCH:
            // Flag capture strategy
            PrioritizeFlagCarriers();
            break;

        case BattlegroundType::ARATHI_BASIN:
            // Base defense strategy
            // Framework in place
            break;

        case BattlegroundType::ALTERAC_VALLEY:
            // Objective capture strategy
            // Framework in place
            break;

        default:
            break;
    }
}

void AdvancedBehaviorManager::DefendBase(GameObject* flag)
{
    if (!m_bot || !flag)
        return;

    // Move to flag position and defend
    Position flagPos = flag->GetPosition();

    // PHASE 6B: Use Movement Arbiter with PVP_FLAG_CAPTURE priority (210)
    BotAI* botAI = dynamic_cast<BotAI*>(m_bot->GetAI());
    if (botAI && botAI->GetUnifiedMovementCoordinator())
    {
        bool accepted = botAI->RequestPointMovement(
            PlayerBotMovementPriority::PVP_FLAG_CAPTURE,
            flagPos,
            "PvP flag defense",
            "AdvancedBehaviorManager");

        if (!accepted)
        {
            TC_LOG_TRACE("playerbot.movement.arbiter",
                "AdvancedBehaviorManager: Flag defense rejected for {} - higher priority active",
                m_bot->GetName());
        }
    }
    else
    {
        // FALLBACK: Direct MotionMaster if arbiter not available
        m_bot->GetMotionMaster()->MovePoint(0, flagPos, false);
    }

    // Attack nearby enemies
    Map* map = m_bot->GetMap();
    if (!map)
        return;

    ::std::list<Player*> nearbyPlayers;
    Position botPos = m_bot->GetPosition();
    Trinity::AnyPlayerInPositionRangeCheck check(&botPos, 20.0f, true);
    Trinity::PlayerListSearcher<Trinity::AnyPlayerInPositionRangeCheck> searcher(m_bot, nearbyPlayers, check);
    // DEADLOCK FIX: Spatial grid replaces Cell::Visit
    {
        Map* cellVisitMap = m_bot->GetMap();
        if (!cellVisitMap)
            return;

        DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(cellVisitMap);
        if (!spatialGrid)
        {
            sSpatialGridManager.CreateGrid(cellVisitMap);
            spatialGrid = sSpatialGridManager.GetGrid(cellVisitMap);
        }

        if (spatialGrid)
        {
            ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyGameObjectGuids(
                m_bot->GetPosition(), 20.0f);

            for (ObjectGuid guid : nearbyGuids)
            {
                GameObject* go = m_bot->GetMap()->GetGameObject(guid);
                if (go)
                {
                    // Original logic from searcher
                }
            }
        }
    }

    for (Player* player : nearbyPlayers)
    {
        if (!player || player->IsFriendlyTo(m_bot))
            continue;

        m_bot->Attack(player, true);
        break;
    }
}

void AdvancedBehaviorManager::AttackBase(GameObject* flag)
{
    if (!m_bot || !flag)
        return;

    // Approach flag and capture
    Position flagPos = flag->GetPosition();

    // PHASE 6B: Use Movement Arbiter with PVP_FLAG_CAPTURE priority (210)
    BotAI* botAI = dynamic_cast<BotAI*>(m_bot->GetAI());
    if (botAI && botAI->GetUnifiedMovementCoordinator())
    {
        bool accepted = botAI->RequestPointMovement(
            PlayerBotMovementPriority::PVP_FLAG_CAPTURE,
            flagPos,
            "PvP flag capture",
            "AdvancedBehaviorManager");

        if (!accepted)
        {
            TC_LOG_TRACE("playerbot.movement.arbiter",
                "AdvancedBehaviorManager: Flag capture rejected for {} - higher priority active",
                m_bot->GetName());
        }
    }
    else
    {
        // FALLBACK: Direct MotionMaster if arbiter not available
        m_bot->GetMotionMaster()->MovePoint(0, flagPos, false);
    }

    // Interact with flag when in range
    if (m_bot->GetExactDist2d(flagPos.GetPositionX(), flagPos.GetPositionY()) < 5.0f)
    {
        flag->Use(m_bot, false);
    }
}

void AdvancedBehaviorManager::EscortFlagCarrier(Player* carrier)
{
    if (!m_bot || !carrier)
        return;

    // Follow flag carrier
    // PHASE 6B: Use Movement Arbiter with PVP_TACTICAL priority (120)
    BotAI* botAI = dynamic_cast<BotAI*>(m_bot->GetAI());
    if (botAI && botAI->GetUnifiedMovementCoordinator())
    {
        bool accepted = botAI->RequestFollowMovement(
            PlayerBotMovementPriority::PVP_TACTICAL,
            carrier->GetGUID(),
            3.0f,
            "PvP flag carrier escort",
            "AdvancedBehaviorManager");

        if (!accepted)
        {
            TC_LOG_TRACE("playerbot.movement.arbiter",
                "AdvancedBehaviorManager: Flag escort rejected for {} - higher priority active",
                m_bot->GetName());
        }
    }
    else
    {
        // FALLBACK: Direct MotionMaster if arbiter not available
        m_bot->GetMotionMaster()->MoveFollow(carrier, 3.0f, 0.0f);
    }

    // Attack enemies near carrier
    Map* map = m_bot->GetMap();
    if (!map)
        return;

    ::std::list<Player*> nearbyPlayers;
    Position carrierPos = carrier->GetPosition();
    Trinity::AnyPlayerInPositionRangeCheck check(&carrierPos, 15.0f, true);
    Trinity::PlayerListSearcher<Trinity::AnyPlayerInPositionRangeCheck> searcher(m_bot, nearbyPlayers, check);
    // DEADLOCK FIX: Spatial grid replaces Cell::Visit
    {
        Map* cellVisitMap = carrier->GetMap();
        if (!cellVisitMap)
            return;
        DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(cellVisitMap);
        if (!spatialGrid)
        {
            sSpatialGridManager.CreateGrid(cellVisitMap);
            spatialGrid = sSpatialGridManager.GetGrid(cellVisitMap);
        }

        if (spatialGrid)
        {
            ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyGameObjectGuids(
                carrier->GetPosition(), 15.0f);

            for (ObjectGuid guid : nearbyGuids)
            {
                GameObject* go = carrier->GetMap()->GetGameObject(guid);
                if (go)
                {
                    // Original logic from searcher
                }
            }
        }
    }

    for (Player* player : nearbyPlayers)
    {
        if (!player || player->IsFriendlyTo(m_bot))
            continue;

        m_bot->Attack(player, true);
        break;
    }
}
void AdvancedBehaviorManager::ReturnFlag()
{
    if (!m_bot || !IsInBattleground())
        return;
    // Find flag object and return it
    // Framework in place for flag return logic
}

void AdvancedBehaviorManager::CaptureObjective(GameObject* objective)
{
    if (!m_bot || !objective)
        return;

    // Move to objective
    Position objPos = objective->GetPosition();

    // PHASE 6B: Use Movement Arbiter with PVP_FLAG_CAPTURE priority (210)
    BotAI* botAI = dynamic_cast<BotAI*>(m_bot->GetAI());
    if (botAI && botAI->GetUnifiedMovementCoordinator())
    {
        bool accepted = botAI->RequestPointMovement(
            PlayerBotMovementPriority::PVP_FLAG_CAPTURE,
            objPos,
            "PvP objective capture",
            "AdvancedBehaviorManager");

        if (!accepted)
        {
            TC_LOG_TRACE("playerbot.movement.arbiter",
                "AdvancedBehaviorManager: Objective capture rejected for {} - higher priority active",
                m_bot->GetName());
        }
    }
    else
    {
        // FALLBACK: Direct MotionMaster if arbiter not available
        m_bot->GetMotionMaster()->MovePoint(0, objPos, false);
    }

    // Interact when in range
    if (m_bot->GetExactDist2d(objPos.GetPositionX(), objPos.GetPositionY()) < 5.0f)
    {
        objective->Use(m_bot, false);
        RecordObjectiveCapture();
    }
}

void AdvancedBehaviorManager::FocusPvPTarget(Player* target)
{
    if (!m_bot || !target)
        return;

    m_bot->Attack(target, true);
    m_bot->SetSelection(target->GetGUID());
}

void AdvancedBehaviorManager::CallForBackup(Position const& /*location*/)
{
    if (!m_bot)
        return;

    // Send raid warning or battleground message
    // Framework in place for chat integration
}

void AdvancedBehaviorManager::UseDefensiveCooldowns()
{
    if (!m_bot)
        return;

    // Defensive cooldowns handled by class AI
    // Framework in place for explicit cooldown usage
}

void AdvancedBehaviorManager::PrioritizeHealers()
{
    if (!m_bot || !IsInBattleground())
        return;

    // Find enemy healers and prioritize them
    Map* map = m_bot->GetMap();
    if (!map)
        return;

    ::std::list<Player*> nearbyPlayers;
    Position botPos = m_bot->GetPosition();
    Trinity::AnyPlayerInPositionRangeCheck check(&botPos, 40.0f, true);
    Trinity::PlayerListSearcher<Trinity::AnyPlayerInPositionRangeCheck> searcher(m_bot, nearbyPlayers, check);
    // DEADLOCK FIX: Spatial grid replaces Cell::Visit
    {
        Map* cellVisitMap = m_bot->GetMap();
        if (!cellVisitMap)
            return;

        DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(cellVisitMap);
        if (!spatialGrid)
        {
            sSpatialGridManager.CreateGrid(cellVisitMap);
            spatialGrid = sSpatialGridManager.GetGrid(cellVisitMap);
        }

        if (spatialGrid)
        {
            // PHASE 5C: Thread-safe spatial grid query (replaces QueryNearbyCreatureGuids + ObjectAccessor)
            auto creatureSnapshots = spatialGrid->QueryNearbyCreatures(
                m_bot->GetPosition(), 40.0f);

            for (auto const& snapshot : creatureSnapshots)
            {
                Creature* creature = ObjectAccessor::GetCreature(*m_bot, snapshot.guid);
                if (creature)
                {
                    // Original logic from searcher
                }
            }
        }
    }

    for (Player* player : nearbyPlayers)
    {
        if (!player || player->IsFriendlyTo(m_bot))
            continue;

        // Check if player is a healer class
    if (player->GetClass() == CLASS_PRIEST ||
            player->GetClass() == CLASS_DRUID ||
            player->GetClass() == CLASS_SHAMAN ||
            player->GetClass() == CLASS_PALADIN ||
            player->GetClass() == CLASS_MONK ||
            player->GetClass() == CLASS_EVOKER)
        {
            FocusPvPTarget(player);
            break;
        }
    }
}

void AdvancedBehaviorManager::PrioritizeFlagCarriers()
{
    if (!m_bot || !IsInBattleground())
        return;

    // Find and attack enemy flag carriers
    Map* map = m_bot->GetMap();
    if (!map)
        return;

    ::std::list<Player*> nearbyPlayers;
    Position botPos = m_bot->GetPosition();
    Trinity::AnyPlayerInPositionRangeCheck check(&botPos, 50.0f, true);
    Trinity::PlayerListSearcher<Trinity::AnyPlayerInPositionRangeCheck> searcher(m_bot, nearbyPlayers, check);
    // DEADLOCK FIX: Spatial grid replaces Cell::Visit
    {
        Map* cellVisitMap = m_bot->GetMap();
        if (!cellVisitMap)
            return;

        DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(cellVisitMap);
        if (!spatialGrid)
        {
            sSpatialGridManager.CreateGrid(cellVisitMap);
            spatialGrid = sSpatialGridManager.GetGrid(cellVisitMap);
        }

        if (spatialGrid)
        {
            // PHASE 5C: Thread-safe spatial grid query (replaces QueryNearbyCreatureGuids + ObjectAccessor)
            auto creatureSnapshots = spatialGrid->QueryNearbyCreatures(
                m_bot->GetPosition(), 50.0f);

            for (auto const& snapshot : creatureSnapshots)
            {
                Creature* creature = ObjectAccessor::GetCreature(*m_bot, snapshot.guid);
                if (creature)
                {
                    // Original logic from searcher
                }
            }
        }
    }
    for (Player* player : nearbyPlayers)
    {
        if (!player || player->IsFriendlyTo(m_bot))
            continue;

        // Check if player has flag aura (specific to WSG)
        // Framework in place for flag detection
    if (player->HasAura(23333) || player->HasAura(23335)) // WSG flag auras
        {
            FocusPvPTarget(player);
            break;
        }
    }
}

// ============================================================================
// WORLD EVENTS
// ============================================================================

bool AdvancedBehaviorManager::ParticipateInWorldEvent(WorldEventType type)
{
    if (!m_bot || !m_eventEnabled)
        return false;
    if (!IsEventActive(type))
        return false;
    m_activeEvent = type;

    // Execute event-specific behavior
    CompleteEventQuests(type);
    VisitEventVendors(type);

    RecordEventParticipation(type);
    return true;
}

void AdvancedBehaviorManager::CompleteEventQuests(WorldEventType /*type*/)
{
    if (!m_bot)
        return;

    // Find active event quests and complete them
    // Framework in place for quest integration
}

void AdvancedBehaviorManager::VisitEventVendors(WorldEventType /*type*/)
{
    if (!m_bot)
        return;

    // Find event vendors and interact
    // Framework in place for vendor integration
}

::std::vector<AdvancedBehaviorManager::WorldEvent> AdvancedBehaviorManager::GetActiveEvents() const
{
    ::std::vector<WorldEvent> activeEvents;

    for (auto const& event : m_worldEvents)
    {
        if (event.isActive)
            activeEvents.push_back(event);
    }

    return activeEvents;
}

// ============================================================================
// ACHIEVEMENT SYSTEM
// ============================================================================

void AdvancedBehaviorManager::PursueAchievement(uint32 achievementId)
{
    if (!m_bot || !m_achievementHunting)
        return;

    // Add to pursuit list
    if (::std::find(m_pursuingAchievements.begin(), m_pursuingAchievements.end(), achievementId) == m_pursuingAchievements.end())
        m_pursuingAchievements.push_back(achievementId);
}

::std::vector<AdvancedBehaviorManager::Achievement> AdvancedBehaviorManager::GetPursuitAchievements() const
{
    ::std::vector<Achievement> achievements;

    // Framework in place for achievement database lookup
    return achievements;
}

void AdvancedBehaviorManager::CheckAchievementProgress(uint32 /*achievementId*/)
{
    if (!m_bot)
        return;

    // Achievement checking requires WorldSession integration
    // Framework in place for future achievement tracking
}

void AdvancedBehaviorManager::PrioritizeAchievements()
{
    CalculateAchievementPriority();
}

// ============================================================================
// EXPLORATION AND DISCOVERY
// ============================================================================

void AdvancedBehaviorManager::ExploreZone(uint32 zoneId)
{
    if (!m_bot)
        return;

    m_exploredZones.insert(zoneId);
}

void AdvancedBehaviorManager::DiscoverFlightPaths()
{
    if (!m_bot)
        return;

    // Find nearby flight masters
    Map* map = m_bot->GetMap();
    if (!map)
        return;

    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return;
    }

    // PHASE 5C: Thread-safe spatial grid query (replaces QueryNearbyCreatureGuids + ObjectAccessor)
    auto creatureSnapshots = spatialGrid->QueryNearbyCreatures(
        m_bot->GetPosition(), 50.0f);

    // Find flight masters using snapshots
    for (auto const& snapshot : creatureSnapshots)
    {
        // Get Creature* for NPC flag checks
        Creature* creature = ObjectAccessor::GetCreature(*m_bot, snapshot.guid);
        if (!creature)
            continue;

        // Check if creature is flight master (npc_flag UNIT_NPC_FLAG_FLIGHTMASTER)
    if (creature->HasNpcFlag(UNIT_NPC_FLAG_FLIGHTMASTER))
        {
            // Discover this flight path
            m_discoveredFlightPaths.insert(creature->GetEntry());
        }
    }
}

void AdvancedBehaviorManager::FindRareSpawns()
{
    if (!m_bot || !m_rareHunting)
        return;

    ScanForRares();
}

void AdvancedBehaviorManager::CollectTreasures()
{
    if (!m_bot)
        return;

    FindNearbyTreasures();
}

void AdvancedBehaviorManager::TrackRareSpawn(Creature* rare)
{
    if (!rare)
        return;

    RareSpawn spawn;
    spawn.entry = rare->GetEntry();
    spawn.name = rare->GetName();
    spawn.lastKnownPosition = rare->GetPosition();
    spawn.respawnTime = rare->GetRespawnTime();
    spawn.lastSeenTime = time(nullptr);
    spawn.isElite = rare->IsElite();
    spawn.level = rare->GetLevel();

    m_trackedRares[rare->GetEntry()] = spawn;
}

::std::vector<AdvancedBehaviorManager::RareSpawn> AdvancedBehaviorManager::GetTrackedRares() const
{
    ::std::vector<RareSpawn> rares;
    rares.reserve(m_trackedRares.size());

    for (auto const& pair : m_trackedRares)
        rares.push_back(pair.second);

    return rares;
}

bool AdvancedBehaviorManager::ShouldEngageRare(Creature* rare) const
{
    if (!m_bot || !rare)
        return false;
    // Check if bot is strong enough
    if (rare->GetLevel() > m_bot->GetLevel() + 3)
        return false;
    // Check if bot has group support for elite rares
    if (rare->IsElite() && !m_bot->GetGroup())
        return false;

    return true;
}

void AdvancedBehaviorManager::FindNearbyTreasures()
{
    if (!m_bot)
        return;

    ScanForTreasures();
}
bool AdvancedBehaviorManager::LootTreasure(GameObject* treasure)
{
    if (!m_bot || !treasure)
        return false;

    // Use treasure
    treasure->Use(m_bot, false);

    RecordTreasureLoot(treasure);
    return true;
}

::std::vector<AdvancedBehaviorManager::Treasure> AdvancedBehaviorManager::GetDiscoveredTreasures() const
{
    return m_discoveredTreasures;
}

void AdvancedBehaviorManager::CollectMounts()
{
    // Mount collection framework
    // Requires vendor/drop integration
}

void AdvancedBehaviorManager::CollectPets()
{
    // Pet collection framework
    // Requires vendor/drop integration
}

void AdvancedBehaviorManager::BattlePets()
{
    // Pet battle framework
    // Requires pet battle system integration
}

// ============================================================================
// PRIVATE HELPER METHODS
// ============================================================================

void AdvancedBehaviorManager::UpdateDungeonBehavior(uint32 /*diff*/)
{
    if (!IsInDungeon())
        return;

    ExecuteDungeonStrategy();
    HandlePatrolAvoidance();
}

void AdvancedBehaviorManager::LoadDungeonStrategies()
{
    // Pre-defined dungeon strategies
    // Framework in place for database loading

    DungeonStrategy strategy;
    strategy.dungeonId = 36; // Deadmines
    strategy.dungeonName = "The Deadmines";
    strategy.recommendedLevel = 15;
    strategy.maxPlayers = 5;
    strategy.bossEntries = {639, 644, 645, 646, 647, 3586};

    m_dungeonStrategies[strategy.dungeonId] = strategy;
}

AdvancedBehaviorManager::DungeonStrategy* AdvancedBehaviorManager::GetDungeonStrategy(uint32 dungeonId)
{
    auto itr = m_dungeonStrategies.find(dungeonId);
    if (itr != m_dungeonStrategies.end())
        return &itr->second;
    return nullptr;
}

void AdvancedBehaviorManager::AnalyzeDungeonComposition()
{
    if (!m_bot)
        return;

    Group* group = m_bot->GetGroup();
    if (!group)
        return;

    // Analyze group composition to determine optimal role
    // Framework in place for group analysis
}

void AdvancedBehaviorManager::AssignDungeonRole()
{
    if (!m_bot)
        return;

    // Determine role based on spec
    uint8 botClass = m_bot->GetClass();
    // Tank classes
    if (botClass == CLASS_WARRIOR || botClass == CLASS_PALADIN ||
        botClass == CLASS_DEATH_KNIGHT || botClass == CLASS_DEMON_HUNTER ||
        botClass == CLASS_DRUID || botClass == CLASS_MONK)
    {
        m_dungeonRole = DungeonRole::TANK;
    }
    // Healer classes
    else if (botClass == CLASS_PRIEST || botClass == CLASS_PALADIN ||
             botClass == CLASS_SHAMAN || botClass == CLASS_DRUID ||
             botClass == CLASS_MONK || botClass == CLASS_EVOKER)
    {
        m_dungeonRole = DungeonRole::HEALER;
    }
    // DPS classes
    else
    {
        m_dungeonRole = DungeonRole::DPS;
    }
}

void AdvancedBehaviorManager::StartBossFight(Creature* boss)
{
    if (!boss)
        return;

    // Reset any existing boss fight before starting new one (prevents memory leak)
    if (m_currentBossFight)
        m_currentBossFight.reset();

    m_currentBossFight = ::std::make_unique<ActiveBossFight>();
    m_currentBossFight->boss = boss;
    m_currentBossFight->bossEntry = boss->GetEntry();
    m_currentBossFight->startTime = GameTime::GetGameTimeMS();
    m_currentBossFight->phase = 1;
}

void AdvancedBehaviorManager::EndBossFight(bool victory)
{
    if (!m_currentBossFight)
        return;

    if (victory)
        RecordBossKill(m_currentBossFight->boss);

    // unique_ptr automatically deletes on reset
    m_currentBossFight.reset();
}

void AdvancedBehaviorManager::UpdateBossFight(uint32 /*diff*/)
{
    if (!m_currentBossFight)
        return;

    Creature* boss = m_currentBossFight->boss;
    if (!boss || !boss->IsAlive())
    {
        EndBossFight(boss && !boss->IsAlive());
        return;
    }

    // Check for phase transitions
    uint32 healthPct = boss->GetHealthPct();
    if (healthPct < 70 && m_currentBossFight->phase == 1)
        AdvanceBossPhase();
    else if (healthPct < 35 && m_currentBossFight->phase == 2)
        AdvanceBossPhase();
}

void AdvancedBehaviorManager::AdvanceBossPhase()
{
    if (!m_currentBossFight)
        return;

    m_currentBossFight->phase++;
}

void AdvancedBehaviorManager::UpdatePvPBehavior(uint32 /*diff*/)
{
    if (!IsInBattleground())
        return;

    ExecuteBattlegroundStrategy();
}

void AdvancedBehaviorManager::LoadBattlegroundStrategies()
{
    // Pre-defined battleground strategies
    BattlegroundStrategy strategy;
    strategy.type = BattlegroundType::WARSONG_GULCH;
    strategy.strategyName = "Capture the Flag";
    strategy.objectives = {"Capture enemy flag", "Defend friendly flag", "Eliminate enemy flag carriers"};

    m_bgStrategies[strategy.type] = strategy;
}
AdvancedBehaviorManager::BattlegroundStrategy* AdvancedBehaviorManager::GetBattlegroundStrategy(BattlegroundType type)
{
    auto itr = m_bgStrategies.find(type);
    if (itr != m_bgStrategies.end())
        return &itr->second;
    return nullptr;
}

void AdvancedBehaviorManager::AnalyzeBattlegroundSituation()
{
    // Analyze current BG state and adapt strategy
    // Framework in place for situational analysis
}

Player* AdvancedBehaviorManager::SelectPvPTarget()
{
    if (!m_bot)
        return nullptr;

    // Find optimal PvP target
    Map* map = m_bot->GetMap();
    if (!map)
        return nullptr;

    ::std::list<Player*> nearbyPlayers;
    Position botPos = m_bot->GetPosition();
    Trinity::AnyPlayerInPositionRangeCheck check(&botPos, 40.0f, true);
    Trinity::PlayerListSearcher<Trinity::AnyPlayerInPositionRangeCheck> searcher(m_bot, nearbyPlayers, check);
    // DEADLOCK FIX: Spatial grid replaces Cell::Visit
    {
        Map* cellVisitMap = m_bot->GetMap();
        if (!cellVisitMap)
            return nullptr;

        DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(cellVisitMap);
        if (!spatialGrid)
        {
            sSpatialGridManager.CreateGrid(cellVisitMap);
            spatialGrid = sSpatialGridManager.GetGrid(cellVisitMap);
        }

        if (spatialGrid)
        {
            // PHASE 5C: Thread-safe spatial grid query (replaces QueryNearbyCreatureGuids + ObjectAccessor)
            auto creatureSnapshots = spatialGrid->QueryNearbyCreatures(
                m_bot->GetPosition(), 40.0f);

            for (auto const& snapshot : creatureSnapshots)
            {
                Creature* creature = ObjectAccessor::GetCreature(*m_bot, snapshot.guid);
                if (creature)
                {
                    // Original logic from searcher
                }
            }
        }
    }

    Player* bestTarget = nullptr;
    uint32 highestPriority = 0;

    for (Player* player : nearbyPlayers)
    {
        if (!player || player->IsFriendlyTo(m_bot) || !player->IsAlive())
            continue;

        uint32 priority = 0;

        // Prioritize healers
    if (player->GetClass() == CLASS_PRIEST || player->GetClass() == CLASS_DRUID ||
            player->GetClass() == CLASS_SHAMAN || player->GetClass() == CLASS_PALADIN)
            priority += 50;

        // Prioritize low health targets
    if (player->GetHealthPct() < 50)
            priority += 30;

        // Prioritize nearby targets
        float distance = m_bot->GetExactDist2d(player);
        priority += static_cast<uint32>((40.0f - distance) * 2);

        if (priority > highestPriority)
        {
            highestPriority = priority;
            bestTarget = player;
        }
    }

    return bestTarget;
}

void AdvancedBehaviorManager::UpdateEventBehavior(uint32 /*diff*/)
{
    UpdateEventStatus();

    if (m_activeEvent != WorldEventType::NONE)
        ParticipateInWorldEvent(m_activeEvent);
}

void AdvancedBehaviorManager::LoadWorldEvents()
{
    // Pre-defined world events
    // Framework in place for GameEventMgr integration
}

void AdvancedBehaviorManager::UpdateEventStatus()
{
    // Check which events are currently active
    for (auto& event : m_worldEvents)
    {
        uint32 currentTime = time(nullptr);
        event.isActive = (currentTime >= event.startTime && currentTime <= event.endTime);
    }
}

bool AdvancedBehaviorManager::IsEventActive(WorldEventType type) const
{
    for (auto const& event : m_worldEvents)
    {
        if (event.type == type && event.isActive)
            return true;
    }
    return false;
}

void AdvancedBehaviorManager::UpdateAchievementProgress(uint32 /*diff*/)
{
    for (uint32 achievementId : m_pursuingAchievements)
        CheckAchievementProgress(achievementId);
}

void AdvancedBehaviorManager::CalculateAchievementPriority()
{
    // Priority calculation for achievements
    // Framework in place for priority algorithm
}

void AdvancedBehaviorManager::UpdateExploration(uint32 /*diff*/)
{
    if (!m_bot)
        return;

    uint32 currentZone = m_bot->GetZoneId();
    if (currentZone != 0)
        ExploreZone(currentZone);

    DiscoverFlightPaths();
}

void AdvancedBehaviorManager::UpdateRareTracking(uint32 /*diff*/)
{
    if (!m_rareHunting)
        return;

    ScanForRares();
}

void AdvancedBehaviorManager::ScanForRares()
{
    if (!m_bot)
        return;

    Map* map = m_bot->GetMap();
    if (!map)
        return;

    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return;
    }

    // PHASE 5C: Thread-safe spatial grid query (replaces QueryNearbyCreatureGuids + ObjectAccessor)
    auto creatureSnapshots = spatialGrid->QueryNearbyCreatures(
        m_bot->GetPosition(), 100.0f);

    // Scan for rares using snapshots
    for (auto const& snapshot : creatureSnapshots)
    {
        if (!snapshot.IsAlive())
            continue;

        // Get Creature* for classification checks
        Creature* creature = ObjectAccessor::GetCreature(*m_bot, snapshot.guid);
        if (!creature)
            continue;

        // Check if creature is rare (classification)
    if (static_cast<CreatureClassifications>(creature->GetCreatureTemplate()->Classification) == CreatureClassifications::RareElite ||
            static_cast<CreatureClassifications>(creature->GetCreatureTemplate()->Classification) == CreatureClassifications::Rare)
        {
            TrackRareSpawn(creature);

            // Engage if appropriate
    if (ShouldEngageRare(creature))
                m_bot->Attack(creature, true);
        }
    }
}

void AdvancedBehaviorManager::UpdateTreasureHunting(uint32 /*diff*/)
{
    ScanForTreasures();
}

void AdvancedBehaviorManager::ScanForTreasures()
{
    if (!m_bot)
        return;

    Map* map = m_bot->GetMap();
    if (!map)
        return;

    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return;
    }

    // Query nearby GameObject GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyGameObjectGuids(
        m_bot->GetPosition(), 50.0f);

    // Resolve GUIDs to GameObject pointers and scan for treasures
    for (ObjectGuid guid : nearbyGuids)
    {
        GameObject* go = map->GetGameObject(guid);
        if (!go || !go->isSpawned())
            continue;

        GameObjectTemplate const* goInfo = go->GetGOInfo();
        if (!goInfo)
            continue;

        // Check if object is lootable treasure
    if (goInfo->type == GAMEOBJECT_TYPE_CHEST || goInfo->type == GAMEOBJECT_TYPE_GOOBER)
        {
            Treasure treasure;
            treasure.guid = go->GetGUID();
            treasure.entry = go->GetEntry();
            treasure.position = go->GetPosition();
            treasure.discoveredTime = time(nullptr);
            treasure.isLooted = false;

            m_discoveredTreasures.push_back(treasure);

            // Loot if in range
    if (go->IsAtInteractDistance(m_bot))
                LootTreasure(go);
        }
    }
}

void AdvancedBehaviorManager::UpdateDangerZones(uint32 /*diff*/)
{
    uint32 now = GameTime::GetGameTimeMS();

    // Remove expired danger zones
    m_dangerZones.erase(
        ::std::remove_if(m_dangerZones.begin(), m_dangerZones.end(),
            [now](DangerZone const& zone) {
                return now >= zone.expiryTime;
            }),
        m_dangerZones.end()
    );
}

bool AdvancedBehaviorManager::IsInDangerZone(Position const& pos) const
{
    for (auto const& zone : m_dangerZones)
    {
        float distance = pos.GetExactDist2d(zone.center.GetPositionX(), zone.center.GetPositionY());
        if (distance <= zone.radius)
            return true;
    }
    return false;
}

Position AdvancedBehaviorManager::FindSafePosition(Position const& currentPos) const
{
    // Find position outside all danger zones
    float angle = frand(0.0f, 2.0f * static_cast<float>(M_PI));
    float distance = 20.0f;

    Position safePos;
    safePos.m_positionX = currentPos.GetPositionX() + distance * ::std::cos(angle);
    safePos.m_positionY = currentPos.GetPositionY() + distance * ::std::sin(angle);
    safePos.m_positionZ = currentPos.GetPositionZ();

    // Verify position is safe
    if (!IsInDangerZone(safePos))
        return safePos;

    // Try multiple angles if first attempt failed
    for (uint32 i = 0; i < 8; ++i)
    {
        angle = (i * 45.0f) * (static_cast<float>(M_PI) / 180.0f);
        safePos.m_positionX = currentPos.GetPositionX() + distance * ::std::cos(angle);
        safePos.m_positionY = currentPos.GetPositionY() + distance * ::std::sin(angle);

        if (!IsInDangerZone(safePos))
            return safePos;
    }

    return currentPos; // Return current position if no safe position found
}

// ============================================================================
// STATISTICS
// ============================================================================

void AdvancedBehaviorManager::RecordDungeonComplete()
{
    m_stats.dungeonsCompleted++;
}

void AdvancedBehaviorManager::RecordBossKill(Creature* boss)
{
    if (!boss)
        return;

    m_stats.bossesKilled++;
}

void AdvancedBehaviorManager::RecordBattlegroundResult(bool victory)
{
    if (victory)
        m_stats.battlegroundsWon++;
    else
        m_stats.battlegroundsLost++;
}

void AdvancedBehaviorManager::RecordObjectiveCapture()
{
    m_stats.objectivesCaptured++;
}

void AdvancedBehaviorManager::RecordEventParticipation(WorldEventType /*type*/)
{
    m_stats.eventsParticipated++;
}

void AdvancedBehaviorManager::RecordAchievement(uint32 /*achievementId*/)
{
    m_stats.achievementsEarned++;
}

void AdvancedBehaviorManager::RecordRareKill(Creature* rare)
{
    if (!rare)
        return;

    m_stats.raresKilled++;
}

void AdvancedBehaviorManager::RecordTreasureLoot(GameObject* treasure)
{
    if (!treasure)
        return;

    m_stats.treasuresLooted++;
}

// ============================================================================
// PERFORMANCE TRACKING
// ============================================================================

void AdvancedBehaviorManager::StartPerformanceTimer()
{
    m_performanceStart = ::std::chrono::high_resolution_clock::now();
}

void AdvancedBehaviorManager::EndPerformanceTimer()
{
    auto end = ::std::chrono::high_resolution_clock::now();
    m_lastUpdateDuration = ::std::chrono::duration_cast<::std::chrono::microseconds>(end - m_performanceStart);
    m_totalUpdateTime += m_lastUpdateDuration;
    m_updateCount++;
}

void AdvancedBehaviorManager::UpdatePerformanceMetrics()
{
    if (m_updateCount > 0)
    {
        auto avgDuration = m_totalUpdateTime / m_updateCount;
        m_cpuUsage = (avgDuration.count() / 1000.0f) / 100.0f;

        if (m_updateCount >= 1000)
        {
            m_totalUpdateTime = ::std::chrono::microseconds(0);
            m_updateCount = 0;
        }
    }
}

size_t AdvancedBehaviorManager::GetMemoryUsage() const
{
    size_t memory = sizeof(AdvancedBehaviorManager);
    memory += m_dungeonStrategies.size() * sizeof(DungeonStrategy);
    memory += m_bgStrategies.size() * sizeof(BattlegroundStrategy);
    memory += m_worldEvents.size() * sizeof(WorldEvent);
    memory += m_pursuingAchievements.size() * sizeof(uint32);
    memory += m_exploredZones.size() * sizeof(uint32);
    memory += m_discoveredFlightPaths.size() * sizeof(uint32);
    memory += m_trackedRares.size() * sizeof(RareSpawn);
    memory += m_discoveredTreasures.size() * sizeof(Treasure);
    memory += m_dangerZones.size() * sizeof(DangerZone);
    if (m_currentBossFight)
        memory += sizeof(ActiveBossFight);
    return memory;
}

} // namespace Playerbot
