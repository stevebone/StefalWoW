/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
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
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ZoneOrchestrator.h"
#include "Player.h"
#include "Group.h"
#include "Creature.h"
#include "Log.h"
#include "GameTime.h"
#include "ObjectAccessor.h"

namespace Playerbot
{
namespace Coordination
{

// ============================================================================
// ZoneObjective
// ============================================================================

bool ZoneObjective::IsActive() const
{
    uint32 now = GameTime::GetGameTimeMS();
    return now < expirationTime && !IsComplete();
}

bool ZoneObjective::IsComplete() const
{
    return assignedBots >= requiredBots;
}

// ============================================================================
// ZoneOrchestrator
// ============================================================================

ZoneOrchestrator::ZoneOrchestrator(uint32 zoneId)
    : _zoneId(zoneId)
{
    TC_LOG_DEBUG("playerbot.coordination", "ZoneOrchestrator created for zone {}", _zoneId);
}

void ZoneOrchestrator::Update(uint32 diff)
{
    _lastUpdateTime += diff;
    if (_lastUpdateTime < _updateInterval)
        return;

    _lastUpdateTime = 0;

    UpdateRaids(diff);
    UpdateObjectives(diff);
    UpdateThreatAssessment(diff);
    UpdateBotActivity(diff);
    UpdateLoadBalancing(diff);

    // Update statistics every 5s
    uint32 now = GameTime::GetGameTimeMS();
    if (now > _lastStatsUpdate + 5000)
    {
        _lastStatsUpdate = now;
        _cachedStats = GetZoneStats();
    }
}

void ZoneOrchestrator::RegisterBot(Player* bot)
{
    if (!bot)
        return;

    ObjectGuid botGuid = bot->GetGUID();

    // Check if already registered
    if (::std::find(_bots.begin(), _bots.end(), botGuid) != _bots.end())
        return;

    _bots.push_back(botGuid);

    TC_LOG_DEBUG("playerbot.coordination", "Bot {} registered in zone {} (total: {})",
        bot->GetName(), _zoneId, _bots.size());

    // Auto-balance if needed
    if (_bots.size() % 40 == 0) // Every 40 bots, check balancing
    {
        BalanceBotDistribution();
    }
}

void ZoneOrchestrator::UnregisterBot(ObjectGuid botGuid)
{
    auto it = ::std::find(_bots.begin(), _bots.end(), botGuid);
    if (it != _bots.end())
    {
        _bots.erase(it);

        TC_LOG_DEBUG("playerbot.coordination", "Bot {} unregistered from zone {} (remaining: {})",
            botGuid.ToString(), _zoneId, _bots.size());
    }
}

void ZoneOrchestrator::AddRaid(::std::unique_ptr<RaidOrchestrator> raid)
{
    if (!raid)
        return;

    _raids.push_back(::std::move(raid));

    TC_LOG_DEBUG("playerbot.coordination", "Raid added to zone {} (total raids: {})",
        _zoneId, _raids.size());
}

RaidOrchestrator* ZoneOrchestrator::GetRaid(uint32 raidIndex)
{
    if (raidIndex >= _raids.size())
        return nullptr;

    return _raids[raidIndex].get();
}

void ZoneOrchestrator::SetActivity(ZoneActivity activity)
{
    if (_currentActivity != activity)
    {
        ZoneActivity oldActivity = _currentActivity;
        _currentActivity = activity;

        TC_LOG_DEBUG("playerbot.coordination", "Zone {} activity changed: {} -> {}",
            _zoneId, static_cast<uint8>(oldActivity), static_cast<uint8>(activity));
    }
}

void ZoneOrchestrator::SetThreatLevel(ThreatLevel level)
{
    if (_threatLevel != level)
    {
        ThreatLevel oldLevel = _threatLevel;
        _threatLevel = level;

        TC_LOG_DEBUG("playerbot.coordination", "Zone {} threat level changed: {} -> {}",
            _zoneId, static_cast<uint8>(oldLevel), static_cast<uint8>(level));

        // React to threat changes
    if (_threatLevel == ThreatLevel::CRITICAL)
        {
            // Request assembly for world boss
            // Position would be determined by threat source
        }
    }
}

void ZoneOrchestrator::CreateObjective(ZoneObjective const& objective)
{
    _objectives.push_back(objective);

    TC_LOG_DEBUG("playerbot.coordination", "Zone {} objective created: {} (priority: {}, required bots: {})",
        _zoneId, objective.objectiveType, objective.priority, objective.requiredBots);

    PrioritizeObjectives();
}

::std::vector<ZoneObjective> ZoneOrchestrator::GetActiveObjectives() const
{
    ::std::vector<ZoneObjective> active;

    for (auto const& objective : _objectives)
    {
        if (objective.IsActive())
            active.push_back(objective);
    }

    return active;
}

void ZoneOrchestrator::CompleteObjective(::std::string const& objectiveType)
{
    auto it = ::std::remove_if(_objectives.begin(), _objectives.end(),
        [&objectiveType](ZoneObjective const& obj) {
            return obj.objectiveType == objectiveType;
        });

    if (it != _objectives.end())
    {
        _objectives.erase(it, _objectives.end());

        TC_LOG_DEBUG("playerbot.coordination", "Zone {} objective completed: {}",
            _zoneId, objectiveType);
    }
}

uint32 ZoneOrchestrator::AssignBotsToObjective(::std::string const& objectiveType, uint32 botCount)
{
    // Find objective
    ZoneObjective* objective = nullptr;
    for (auto& obj : _objectives)
    {
        if (obj.objectiveType == objectiveType)
        {
            objective = &obj;
            break;
        }
    }

    if (!objective)
        return 0;

    // Calculate how many bots we can assign
    uint32 available = static_cast<uint32>(_bots.size()) - objective->assignedBots;
    uint32 needed = objective->requiredBots - objective->assignedBots;
    uint32 toAssign = ::std::min({botCount, available, needed});

    objective->assignedBots += toAssign;

    TC_LOG_DEBUG("playerbot.coordination", "Zone {} assigned {} bots to objective {} ({}/{})",
        _zoneId, toAssign, objectiveType, objective->assignedBots, objective->requiredBots);

    return toAssign;
}

void ZoneOrchestrator::BroadcastMessage(::std::string const& message, uint32 priority)
{
    TC_LOG_DEBUG("playerbot.coordination", "Zone {} broadcast (priority {}): {}",
        _zoneId, priority, message);

    // This would send messages to all bots in the zone
    // Implementation would depend on chat/communication system
}

uint32 ZoneOrchestrator::RequestAssembly(Position const& position, float radius)
{
    uint32 responding = 0;

    for (ObjectGuid botGuid : _bots)
    {
        Player* bot = ObjectAccessor::FindPlayer(botGuid);
        if (!bot)
            continue;

        // Check if bot is within reasonable distance
        float distance = bot->GetDistance(position);
        if (distance < radius * 5.0f) // Within 5x radius = can respond
        {
            // Bot would move to assembly point
            responding++;
        }
    }

    TC_LOG_DEBUG("playerbot.coordination", "Zone {} assembly requested at ({:.1f}, {:.1f}, {:.1f}): {} bots responding",
        _zoneId, position.GetPositionX(), position.GetPositionY(), position.GetPositionZ(), responding);

    return responding;
}

void ZoneOrchestrator::BalanceBotDistribution()
{
    if (_bots.empty())
        return;

    // Create new raid if needed (every 40 bots)
    uint32 expectedRaids = (_bots.size() + 39) / 40; // Ceiling division
    while (_raids.size() < expectedRaids)
    {
        // Create new raid group
        // This would require a Group* from TrinityCore
        // For now, we'll just log the intention
        TC_LOG_DEBUG("playerbot.coordination", "Zone {} needs {} raids for {} bots",
            _zoneId, expectedRaids, _bots.size());
        break; // Exit to prevent infinite loop without actual raid creation
    }

    RebalanceRaids();
}

ZoneOrchestrator::ZoneStats ZoneOrchestrator::GetZoneStats() const
{
    ZoneStats stats = {};
    stats.totalBots = static_cast<uint32>(_bots.size());
    stats.raidCount = static_cast<uint32>(_raids.size());
    stats.threatLevel = _threatLevel;
    stats.currentActivity = _currentActivity;

    uint32 activeBots = 0;
    float totalLevel = 0.0f;

    for (ObjectGuid botGuid : _bots)
    {
        Player* bot = ObjectAccessor::FindPlayer(botGuid);
        if (!bot)
            continue;

        if (bot->IsInCombat() || bot->GetGroup() != nullptr)
            activeBots++;

        totalLevel += static_cast<float>(bot->GetLevel());
    }

    stats.activeBots = activeBots;
    stats.idleBots = stats.totalBots - activeBots;

    if (stats.totalBots > 0)
        stats.avgBotLevel = totalLevel / static_cast<float>(stats.totalBots);

    stats.activeObjectives = 0;
    for (auto const& obj : _objectives)
    {
        if (obj.IsActive())
            stats.activeObjectives++;
    }

    return stats;
}

void ZoneOrchestrator::UpdateRaids(uint32 diff)
{
    for (auto& raid : _raids)
    {
        raid->Update(diff);
    }
}

void ZoneOrchestrator::UpdateObjectives(uint32 /*diff*/)
{
    CleanupExpiredObjectives();

    // Check for auto-complete objectives
    for (auto& objective : _objectives)
    {
        if (objective.IsComplete())
        {
            CompleteObjective(objective.objectiveType);
        }
    }
}

void ZoneOrchestrator::UpdateThreatAssessment(uint32 /*diff*/)
{
    ScanForThreats();
    DetectWorldBoss();
    DetectZoneEvents();

    // Update threat level based on findings
    // This is simplified - would use actual threat data
    if (_currentActivity == ZoneActivity::WORLD_BOSS)
    {
        SetThreatLevel(ThreatLevel::CRITICAL);
    }
    else if (_currentActivity == ZoneActivity::CITY_RAID)
    {
        SetThreatLevel(ThreatLevel::HIGH);
    }
    else
    {
        SetThreatLevel(ThreatLevel::PEACEFUL);
    }
}

void ZoneOrchestrator::UpdateBotActivity(uint32 /*diff*/)
{
    // Monitor bot activity and adjust zone activity accordingly
    uint32 combatBots = 0;

    for (ObjectGuid botGuid : _bots)
    {
        Player* bot = ObjectAccessor::FindPlayer(botGuid);
        if (bot && bot->IsInCombat())
            combatBots++;
    }

    // If majority in combat, likely a zone event
    if (combatBots > _bots.size() / 2)
    {
        if (_currentActivity == ZoneActivity::IDLE)
        {
            SetActivity(ZoneActivity::ZONE_EVENT);
        }
    }
    else if (combatBots == 0)
    {
        if (_currentActivity == ZoneActivity::ZONE_EVENT)
        {
            SetActivity(ZoneActivity::IDLE);
        }
    }
}

void ZoneOrchestrator::UpdateLoadBalancing(uint32 /*diff*/)
{
    // Periodically rebalance raids every 30s
    static uint32 lastBalance = 0;
    uint32 now = GameTime::GetGameTimeMS();

    if (now > lastBalance + 30000)
    {
        lastBalance = now;
        BalanceBotDistribution();
    }
}

void ZoneOrchestrator::ScanForThreats()
{
    // Scan for hostile NPCs, enemy players, etc.
    // This would query the world for threats in the zone
}

void ZoneOrchestrator::DetectWorldBoss()
{
    // Check if any bots are targeting a world boss
    for (ObjectGuid botGuid : _bots)
    {
        Player* bot = ObjectAccessor::FindPlayer(botGuid);
        if (!bot)
            continue;

        Unit* target = bot->GetSelectedUnit();
        if (target && target->IsCreature())
        {
            Creature* creature = target->ToCreature();
            if (creature && creature->isWorldBoss())
            {
                SetActivity(ZoneActivity::WORLD_BOSS);

                // Create world boss objective
                ZoneObjective objective;
                objective.objectiveType = "kill_world_boss";
                objective.priority = 100;
                objective.requiredBots = 40; // Requires full raid
                objective.assignedBots = 0;
                objective.targetGuid = creature->GetGUID();
                objective.targetPosition = creature->GetPosition();
                objective.timestamp = GameTime::GetGameTimeMS();
                objective.expirationTime = objective.timestamp + 3600000; // 1 hour

                CreateObjective(objective);
                return;
            }
        }
    }
}

void ZoneOrchestrator::DetectZoneEvents()
{
    // Detect zone-wide events (invasions, etc.)
    // This would integrate with TrinityCore event system
}

void ZoneOrchestrator::CleanupExpiredObjectives()
{
    _objectives.erase(
        ::std::remove_if(_objectives.begin(), _objectives.end(),
            [](ZoneObjective const& obj) {
                return !obj.IsActive();
            }),
        _objectives.end()
    );
}

void ZoneOrchestrator::PrioritizeObjectives()
{
    // Sort objectives by priority (highest first)
    ::std::sort(_objectives.begin(), _objectives.end(),
        [](ZoneObjective const& a, ZoneObjective const& b) {
            return a.priority > b.priority;
        });
}

void ZoneOrchestrator::AssignBotsToRaids()
{
    // Assign bots to raids based on availability
    // This would require actual raid/group management
}

void ZoneOrchestrator::RebalanceRaids()
{
    // Rebalance bots across raids for optimal composition
    // This would move bots between raids to maintain balance
}

void ZoneOrchestrator::OptimizeRaidComposition()
{
    // Optimize raid composition (tanks, healers, DPS ratios)
    // This would require role detection and assignment
}

// ============================================================================
// ZoneOrchestratorManager
// ============================================================================

::std::unordered_map<uint32, ::std::unique_ptr<ZoneOrchestrator>> ZoneOrchestratorManager::_orchestrators;

ZoneOrchestrator* ZoneOrchestratorManager::GetOrchestrator(uint32 zoneId)
{
    auto it = _orchestrators.find(zoneId);
    if (it != _orchestrators.end())
        return it->second.get();

    return nullptr;
}

ZoneOrchestrator* ZoneOrchestratorManager::CreateOrchestrator(uint32 zoneId)
{
    auto orchestrator = ::std::make_unique<ZoneOrchestrator>(zoneId);
    ZoneOrchestrator* ptr = orchestrator.get();
    _orchestrators[zoneId] = ::std::move(orchestrator);

    TC_LOG_INFO("playerbot.coordination", "Created zone orchestrator for zone {}", zoneId);

    return ptr;
}

void ZoneOrchestratorManager::RemoveOrchestrator(uint32 zoneId)
{
    auto it = _orchestrators.find(zoneId);
    if (it != _orchestrators.end())
    {
        _orchestrators.erase(it);
        TC_LOG_INFO("playerbot.coordination", "Removed zone orchestrator for zone {}", zoneId);
    }
}

void ZoneOrchestratorManager::UpdateAll(uint32 diff)
{
    for (auto& pair : _orchestrators)
    {
        pair.second->Update(diff);
    }
}

::std::unordered_map<uint32, ::std::unique_ptr<ZoneOrchestrator>> const& ZoneOrchestratorManager::GetAll()
{
    return _orchestrators;
}

void ZoneOrchestratorManager::Clear()
{
    _orchestrators.clear();
    TC_LOG_INFO("playerbot.coordination", "Cleared all zone orchestrators");
}

ZoneOrchestratorManager::GlobalStats ZoneOrchestratorManager::GetGlobalStats()
{
    GlobalStats stats = {};
    stats.totalZones = static_cast<uint32>(_orchestrators.size());

    for (auto const& pair : _orchestrators)
    {
        ZoneOrchestrator::ZoneStats zoneStats = pair.second->GetZoneStats();

        stats.totalBots += zoneStats.totalBots;
        stats.totalRaids += zoneStats.raidCount;
        stats.activeObjectives += zoneStats.activeObjectives;

        if (zoneStats.threatLevel == ThreatLevel::CRITICAL)
            stats.criticalZones++;
    }

    return stats;
}

} // namespace Coordination
} // namespace Playerbot
