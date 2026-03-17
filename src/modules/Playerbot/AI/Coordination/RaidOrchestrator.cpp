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

#include "RaidOrchestrator.h"
#include "../../Advanced/GroupCoordinator.h"  // Concrete implementation - included in .cpp, not .h
#include "Group.h"
#include "Player.h"
#include "Unit.h"
#include "Creature.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "GameTime.h"
#include "Session/BotSession.h"
#include "Session/BotSessionManager.h"
#include "AI/BotAI.h"
#include "Core/Managers/IGameSystemsManager.h"

using Playerbot::BotSession;
using Playerbot::BotSessionManager;
using Playerbot::BotAI;
using Playerbot::IGameSystemsManager;
using Playerbot::Advanced::GroupCoordinator;

namespace Playerbot
{
namespace Coordination
{

// ============================================================================
// RaidDirective
// ============================================================================

bool RaidDirective::IsActive() const
{
    uint32 now = GameTime::GetGameTimeMS();
    return now < timestamp + duration;
}

// ============================================================================
// RaidOrchestrator
// ============================================================================

RaidOrchestrator::RaidOrchestrator(Group* raid)
    : _raid(raid)
{
    // GroupCoordinator is now per-bot (not per-group)
    // Each bot has its own GroupCoordinator via BotAI::GetGameSystems()->GetGroupCoordinator()
    // We track subgroups that have bot members for coordination purposes
    uint32 subgroupsWithBots = 0;
    for (uint8 groupId = 0; groupId < MAX_RAID_SUBGROUPS; ++groupId)
    {
        for (GroupReference& ref : _raid->GetMembers())
        {
            Player* member = ref.GetSource();
            if (member && member->GetSubGroup() == groupId)
            {
                // Check if this is a bot by trying to get their BotSession
                if (BotSession* botSession = BotSessionManager::GetBotSession(member->GetSession()))
                {
                    subgroupsWithBots++;
                    break; // Found at least one bot in this subgroup
                }
            }
        }
    }

    TC_LOG_DEBUG("playerbot.coordination", "RaidOrchestrator created - {} subgroups have bot members",
        subgroupsWithBots);
}

void RaidOrchestrator::Update(uint32 diff)
{
    if (!_raid)
        return;

    _lastUpdateTime += diff;
    if (_lastUpdateTime < _updateInterval)
        return;

    _lastUpdateTime = 0;

    UpdateCombatState();
    UpdateGroupCoordinators(diff);
    UpdateRoleCoordinators(diff);
    UpdateDirectives(diff);
    UpdateFormation(diff);
    UpdateEncounterPhase(diff);
    UpdateRaidStats();
}

GroupCoordinatorPtr* RaidOrchestrator::GetGroupCoordinator(uint32 groupIndex)
{
    // GroupCoordinator is now per-bot, not per-group
    // Return the GroupCoordinator of the first bot found in the specified subgroup
    if (!_raid || groupIndex >= MAX_RAID_SUBGROUPS)
        return nullptr;

    for (GroupReference& ref : _raid->GetMembers())
    {
        Player* member = ref.GetSource();
        if (!member || member->GetSubGroup() != static_cast<uint8>(groupIndex))
            continue;

        // Check if this member is a bot
        BotSession* botSession = BotSessionManager::GetBotSession(member->GetSession());
        if (!botSession)
            continue;

        // Get the BotAI from the session
        BotAI* botAI = botSession->GetAI();
        if (!botAI)
            continue;

        // Get the GameSystems manager and return its GroupCoordinator
        IGameSystemsManager* gameSystems = botAI->GetGameSystems();
        if (!gameSystems)
            continue;

        return gameSystems->GetGroupCoordinator();
    }

    // No bot found in this subgroup
    return nullptr;
}

void RaidOrchestrator::IssueDirective(RaidDirective const& directive)
{
    _activeDirectives.push_back(directive);

    TC_LOG_DEBUG("playerbot.coordination", "Raid directive issued: {} (priority: {}, duration: {}ms)",
        directive.directiveType, directive.priority, directive.duration);
}

::std::vector<RaidDirective> RaidOrchestrator::GetActiveDirectives() const
{
    ::std::vector<RaidDirective> active;

    for (auto const& directive : _activeDirectives)
    {
        if (directive.IsActive())
            active.push_back(directive);
    }

    return active;
}

void RaidOrchestrator::SetFormation(RaidFormation formation)
{
    if (_currentFormation != formation)
    {
        _currentFormation = formation;

        TC_LOG_DEBUG("playerbot.coordination", "Raid formation changed to {}",
            static_cast<uint8>(formation));

        // Issue formation directive to all groups
        RaidDirective directive;
        directive.directiveType = "formation_change";
        directive.priority = 70;
        directive.timestamp = GameTime::GetGameTimeMS();
        directive.duration = 30000; // 30s
        directive.parameters["formation"] = static_cast<float>(formation);

        IssueDirective(directive);
    }
}

void RaidOrchestrator::SetEncounterPhase(EncounterPhase phase)
{
    if (_currentPhase != phase)
    {
        EncounterPhase oldPhase = _currentPhase;
        _currentPhase = phase;

        TC_LOG_DEBUG("playerbot.coordination", "Encounter phase changed: {} -> {}",
            static_cast<uint8>(oldPhase), static_cast<uint8>(phase));

        HandleEncounterPhaseChange();
    }
}

bool RaidOrchestrator::RequestBloodlust()
{
    uint32 now = GameTime::GetGameTimeMS();

    // Check if on cooldown
    if (now < _bloodlustTime + _bloodlustCooldown)
    {
        TC_LOG_DEBUG("playerbot.coordination", "Bloodlust on cooldown ({}s remaining)",
            (_bloodlustTime + _bloodlustCooldown - now) / 1000);
        return false;
    }

    // Check if already active
    if (_bloodlustActive)
    {
        TC_LOG_DEBUG("playerbot.coordination", "Bloodlust already active");
        return false;
    }

    _bloodlustActive = true;
    _bloodlustTime = now;

    // Issue bloodlust directive
    RaidDirective directive;
    directive.directiveType = "bloodlust";
    directive.priority = 100; // Highest priority
    directive.timestamp = now;
    directive.duration = 40000; // 40s duration

    IssueDirective(directive);

    TC_LOG_DEBUG("playerbot.coordination", "Raid bloodlust activated!");

    return true;
}

bool RaidOrchestrator::IsBloodlustActive() const
{
    if (!_bloodlustActive)
        return false;

    uint32 now = GameTime::GetGameTimeMS();
    return now < _bloodlustTime + 40000; // 40s duration
}

bool RaidOrchestrator::RequestRaidDefensiveCooldown(::std::string const& cooldownType)
{
    uint32 now = GameTime::GetGameTimeMS();

    // Check if this cooldown is available
    auto it = _raidCooldowns.find(cooldownType);
    if (it != _raidCooldowns.end() && now < it->second)
    {
        TC_LOG_DEBUG("playerbot.coordination", "Raid cooldown {} on CD ({}s remaining)",
            cooldownType, (it->second - now) / 1000);
        return false;
    }

    // Use cooldown (3min default)
    _raidCooldowns[cooldownType] = now + 180000;

    // Issue directive
    RaidDirective directive;
    directive.directiveType = "defensive_cd";
    directive.priority = 90;
    directive.timestamp = now;
    directive.duration = 10000; // 10s window to use
    directive.parameters["cooldown_type"] = 0.0f; // Placeholder

    IssueDirective(directive);

    TC_LOG_DEBUG("playerbot.coordination", "Raid defensive cooldown requested: {}", cooldownType);

    return true;
}

void RaidOrchestrator::SetAddPriorities(::std::vector<ObjectGuid> const& targetGuids)
{
    _addPriorities = targetGuids;

    TC_LOG_DEBUG("playerbot.coordination", "Raid add priorities set: {} targets",
        _addPriorities.size());
}

RaidOrchestrator::RaidStats RaidOrchestrator::GetRaidStats() const
{
    return _cachedStats;
}

uint32 RaidOrchestrator::GetCombatDuration() const
{
    if (!_inCombat || _combatStartTime == 0)
        return 0;

    return getMSTimeDiff(_combatStartTime, GameTime::GetGameTimeMS());
}

void RaidOrchestrator::UpdateGroupCoordinators(uint32 diff)
{
    // GroupCoordinator is now per-bot, not per-group
    // Each bot's GroupCoordinator is updated via their BotAI::Update() cycle
    // Here we coordinate raid-wide information across all bot GroupCoordinators
    if (!_raid)
        return;

    for (GroupReference& ref : _raid->GetMembers())
    {
        Player* member = ref.GetSource();
        if (!member)
            continue;

        // Check if this member is a bot
        BotSession* botSession = BotSessionManager::GetBotSession(member->GetSession());
        if (!botSession)
            continue;

        BotAI* botAI = botSession->GetAI();
        if (!botAI)
            continue;

        IGameSystemsManager* gameSystems = botAI->GetGameSystems();
        if (!gameSystems)
            continue;

        GroupCoordinator* coordinator = gameSystems->GetGroupCoordinator();
        if (!coordinator)
            continue;

        // Synchronize raid-wide state to each bot's GroupCoordinator
        // This includes formation, encounter phase, and active directives
        coordinator->Update(diff);
    }
}

void RaidOrchestrator::UpdateRoleCoordinators(uint32 diff)
{
    // Role coordinators operate at raid-wide level
    _roleCoordinatorManager.Update(nullptr, diff); // Pass null for group, use raid instead
}

void RaidOrchestrator::UpdateDirectives(uint32 /*diff*/)
{
    // Clean up expired directives
    _activeDirectives.erase(
        ::std::remove_if(_activeDirectives.begin(), _activeDirectives.end(),
            [](RaidDirective const& directive) {
                return !directive.IsActive();
            }),
        _activeDirectives.end()
    );

    // Process active directives
    for (auto const& directive : _activeDirectives)
    {
        if (directive.directiveType == "bloodlust")
        {
            CoordinateBloodlustTiming();
        }
        else if (directive.directiveType == "defensive_cd")
        {
            RotateRaidDefensiveCooldowns();
        }
        else if (directive.directiveType == "focus_adds")
        {
            AssignDPSToAdds();
        }
    }
}

void RaidOrchestrator::UpdateFormation(uint32 /*diff*/)
{
    // Formation-specific positioning logic
    switch (_currentFormation)
    {
        case RaidFormation::SPREAD:
            // Bots should spread out (>10 yards apart)
            // This would be communicated via blackboard/directives
            break;

        case RaidFormation::STACKED:
            // Bots should stack together (<5 yards)
            break;

        case RaidFormation::RANGED_SPLIT:
            // Split ranged into two groups
            break;

        case RaidFormation::MELEE_HEAVY:
            // Melee close to boss, ranged far
            break;

        case RaidFormation::DEFENSIVE:
            // Tanks front, healers back, DPS middle
            break;
    }
}

void RaidOrchestrator::UpdateEncounterPhase(uint32 /*diff*/)
{
    if (!_inCombat)
        return;

    DetectBossEncounter();

    // Phase-specific logic
    switch (_currentPhase)
    {
        case EncounterPhase::NORMAL:
            // Standard combat rotation
            break;

        case EncounterPhase::BURN:
            // Execute/burn phase - use all cooldowns
    if (!IsBloodlustActive())
            {
                RequestBloodlust();
            }
            break;

        case EncounterPhase::ADD_PHASE:
            // Focus on adds
            UpdateAddPriorities();
            break;

        case EncounterPhase::TRANSITION:
            // Conserve resources during transition
            break;

        case EncounterPhase::INTERMISSION:
            // Boss untargetable, handle mechanics
            break;
    }
}

void RaidOrchestrator::UpdateCombatState()
{
    bool wasInCombat = _inCombat;
    _inCombat = false;

    for (GroupReference& ref : _raid->GetMembers())
    {
        Player* member = ref.GetSource();
        if (member && member->IsInCombat())
        {
            _inCombat = true;
            break;
        }
    }

    // Combat started
    if (_inCombat && !wasInCombat)
    {
        _combatStartTime = GameTime::GetGameTimeMS();
        _currentPhase = EncounterPhase::NORMAL;
        _bloodlustActive = false;

        TC_LOG_DEBUG("playerbot.coordination", "Raid entered combat");
    }

    // Combat ended
    if (!_inCombat && wasInCombat)
    {
        TC_LOG_DEBUG("playerbot.coordination", "Raid combat ended (duration: {}s)",
            GetCombatDuration() / 1000);

        // Reset state
        _currentPhase = EncounterPhase::NORMAL;
        _activeDirectives.clear();
        _addPriorities.clear();
    }
}

void RaidOrchestrator::UpdateRaidStats()
{
    uint32 now = GameTime::GetGameTimeMS();
    if (now < _lastStatsUpdate + 1000) // Update stats every 1s
        return;

    _lastStatsUpdate = now;

    RaidStats stats = {};

    float totalHealth = 0.0f;
    float maxHealth = 0.0f;
    float totalMana = 0.0f;
    float maxMana = 0.0f;

    for (GroupReference& ref : _raid->GetMembers())
    {
        Player* member = ref.GetSource();
        if (!member)
            continue;

        stats.totalBots++;

        if (member->IsAlive())
        {
            stats.aliveBots++;
            totalHealth += static_cast<float>(member->GetHealth());
            maxHealth += static_cast<float>(member->GetMaxHealth());

            if (member->GetPowerType() == POWER_MANA)
            {
                totalMana += static_cast<float>(member->GetPower(POWER_MANA));
                maxMana += static_cast<float>(member->GetMaxPower(POWER_MANA));
            }
        }
        else
        {
            stats.deadBots++;
        }
    }

    if (maxHealth > 0.0f)
        stats.avgHealthPct = (totalHealth / maxHealth) * 100.0f;

    if (maxMana > 0.0f)
        stats.avgManaPct = (totalMana / maxMana) * 100.0f;

    stats.combatDuration = GetCombatDuration();

    _cachedStats = stats;
}

void RaidOrchestrator::DetectBossEncounter()
{
    // Find boss enemy (skull marker or highest level enemy)
    ObjectGuid bossGuid;
    uint32 bossEntry = 0;

    for (GroupReference& ref : _raid->GetMembers())
    {
        Player* member = ref.GetSource();
        if (!member || !member->IsInCombat())
            continue;

        Unit* target = member->GetSelectedUnit();
        if (!target || !target->IsCreature())
            continue;

        Creature* creature = target->ToCreature();
        if (creature->IsDungeonBoss() || creature->isWorldBoss())
        {
            bossGuid = creature->GetGUID();
            bossEntry = creature->GetEntry();
            break;
        }
    }

    if (bossEntry == 0)
        return;

    // Check for registered strategy
    auto strategy = BossStrategyRegistry::GetStrategy(bossEntry);
    if (strategy)
    {
        Creature* boss = ObjectAccessor::GetCreature(*_raid->GetMembers().begin()->GetSource(), bossGuid);
        if (boss)
        {
            float healthPct = boss->GetHealthPct();
            EncounterPhase newPhase = strategy->DetectPhase(healthPct);

            if (newPhase != _currentPhase)
            {
                SetEncounterPhase(newPhase);
            }

            strategy->Execute(this, _currentPhase);
        }
    }
}

void RaidOrchestrator::HandleEncounterPhaseChange()
{
    // React to phase changes
    switch (_currentPhase)
    {
        case EncounterPhase::BURN:
            // Activate all DPS cooldowns
            RequestBloodlust();
            break;

        case EncounterPhase::ADD_PHASE:
            // Switch to add focus
            SetFormation(RaidFormation::SPREAD);
            break;

        case EncounterPhase::DEFENSIVE:
            // Use defensive cooldowns
            RequestRaidDefensiveCooldown("barrier");
            break;

        default:
            break;
    }
}

void RaidOrchestrator::RotateRaidDefensiveCooldowns()
{
    // Rotate defensive cooldowns among healers
    HealerCoordinator* healers = _roleCoordinatorManager.GetHealerCoordinator();
    if (!healers)
        return;

    ObjectGuid healer = healers->GetNextCooldownHealer("raid_cd");
    if (!healer.IsEmpty())
    {
        healers->UseHealingCooldown(healer, "raid_cd", 180000); // 3min CD
        TC_LOG_DEBUG("playerbot.coordination", "Raid defensive cooldown assigned to {}",
            healer.ToString());
    }
}

void RaidOrchestrator::CoordinateBloodlustTiming()
{
    // Ensure bloodlust is used at optimal time
    // This would trigger actual spell casting via bot AI
}

void RaidOrchestrator::UpdateAddPriorities()
{
    // Scan for adds and prioritize by threat/health/type
    ::std::vector<ObjectGuid> adds;

    for (GroupReference& ref : _raid->GetMembers())
    {
        Player* member = ref.GetSource();
        if (!member || !member->IsInCombat())
            continue;

        Unit* target = member->GetSelectedUnit();
        if (!target || !target->IsCreature())
            continue;

        Creature* creature = target->ToCreature();
        if (!creature->IsDungeonBoss() && !creature->isWorldBoss())
        {
            // This is an add
    if (::std::find(adds.begin(), adds.end(), creature->GetGUID()) == adds.end())
            {
                adds.push_back(creature->GetGUID());
            }
        }
    }

    // Sort adds by priority (health, threat, etc.)
    // For now, just use the order found
    SetAddPriorities(adds);

    if (!adds.empty())
    {
        // Issue focus_adds directive
        RaidDirective directive;
        directive.directiveType = "focus_adds";
        directive.priority = 85;
        directive.timestamp = GameTime::GetGameTimeMS();
        directive.duration = 10000; // 10s

        IssueDirective(directive);
    }
}

void RaidOrchestrator::AssignDPSToAdds()
{
    DPSCoordinator* dps = _roleCoordinatorManager.GetDPSCoordinator();
    if (!dps || _addPriorities.empty())
        return;

    // Assign focus target to highest priority add
    dps->SetFocusTarget(_addPriorities[0]);
}

// ============================================================================
// BossEncounterStrategy
// ============================================================================

EncounterPhase BossEncounterStrategy::DetectPhase(float bossHealthPct) const
{
    // Default phase detection (override in specific strategies)
    if (bossHealthPct < 20.0f)
        return EncounterPhase::BURN;

    return EncounterPhase::NORMAL;
}

// ============================================================================
// BossStrategyRegistry
// ============================================================================

::std::unordered_map<uint32, ::std::shared_ptr<BossEncounterStrategy>> BossStrategyRegistry::_strategies;

void BossStrategyRegistry::RegisterStrategy(uint32 bossEntry, ::std::shared_ptr<BossEncounterStrategy> strategy)
{
    _strategies[bossEntry] = strategy;
    TC_LOG_INFO("playerbot.coordination", "Registered boss strategy for entry {}", bossEntry);
}

::std::shared_ptr<BossEncounterStrategy> BossStrategyRegistry::GetStrategy(uint32 bossEntry)
{
    auto it = _strategies.find(bossEntry);
    if (it != _strategies.end())
        return it->second;

    return nullptr;
}

void BossStrategyRegistry::Clear()
{
    _strategies.clear();
}

// ============================================================================
// OnyxiaStrategy
// ============================================================================

void OnyxiaStrategy::Execute(RaidOrchestrator* orchestrator, EncounterPhase phase)
{
    switch (phase)
    {
        case EncounterPhase::NORMAL:
            // Phase 1: Ground phase
            orchestrator->SetFormation(RaidFormation::DEFENSIVE);
            break;

        case EncounterPhase::TRANSITION:
            // Phase 2: Air phase
            orchestrator->SetFormation(RaidFormation::SPREAD);
            // Handle whelps
            break;

        case EncounterPhase::BURN:
            // Phase 3: Final burn
            orchestrator->RequestBloodlust();
            orchestrator->SetFormation(RaidFormation::DEFENSIVE);
            break;

        default:
            break;
    }
}

EncounterPhase OnyxiaStrategy::DetectPhase(float bossHealthPct) const
{
    if (bossHealthPct < 40.0f)
        return EncounterPhase::BURN; // Phase 3
    if (bossHealthPct < 65.0f)
        return EncounterPhase::TRANSITION; // Phase 2 (air)

    return EncounterPhase::NORMAL; // Phase 1
}

} // namespace Coordination
} // namespace Playerbot
