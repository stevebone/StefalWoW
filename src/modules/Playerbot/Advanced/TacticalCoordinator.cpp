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

#include "TacticalCoordinator.h"
#include "../AI/Combat/InterruptCoordinator.h"
#include "../AI/Combat/CrowdControlManager.h"
#include "Group.h"
#include "Player.h"
#include "Unit.h"
#include "Log.h"
#include "Timer.h"
#include "Creature.h"
#include "ThreatManager.h"
#include "ObjectAccessor.h"
#include <algorithm>
#include <chrono>

namespace Playerbot
{
namespace Advanced
{

// ============================================================================
// CONSTRUCTOR & DESTRUCTOR
// ============================================================================

TacticalCoordinator::TacticalCoordinator(Group* group)
    : m_group(group)
    , m_lastUpdateTime(0)
    , m_updateInterval(200)
{
    if (!m_group)
    {
        TC_LOG_ERROR("playerbot", "TacticalCoordinator: Constructed with null group!");
    }
}

TacticalCoordinator::~TacticalCoordinator()
{
    Shutdown();
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void TacticalCoordinator::Initialize()
{
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    // CRITICAL: Do NOT access m_group->GetGUID() here!
    // The Group pointer may be valid but the group's internal state may not be
    // fully initialized during bot login phase. Just log existence/null status.
    TC_LOG_DEBUG("playerbot", "TacticalCoordinator::Initialize() - Initializing (group ptr: {})",
                 m_group ? "valid" : "null");

    // Clear all state
    m_tacticalState = GroupTacticalState();
    m_assignments.clear();
    m_lastUpdateTime = getMSTime();
    m_statistics.Reset();

    TC_LOG_DEBUG("playerbot", "TacticalCoordinator::Initialize() - Initialization complete");
}

void TacticalCoordinator::Update(uint32 /*diff*/)
{
    if (!m_group)
        return;

    uint32 currentTime = getMSTime();

    // Check if we should update based on interval
    if (currentTime - m_lastUpdateTime < m_updateInterval)
        return;

    // Performance tracking
    auto startTime = std::chrono::high_resolution_clock::now();

    {
        std::lock_guard<decltype(m_mutex)> lock(m_mutex);

        // Update tactical state
        m_tacticalState.lastUpdateTime = currentTime;
        m_lastUpdateTime = currentTime;

        // Cleanup expired data
        CleanupExpiredData(currentTime);

        // Update focus target if in combat
        if (m_tacticalState.inCombat.load(std::memory_order_acquire))
        {
            UpdateFocusTarget();
            UpdateInterruptRotation();
            UpdateDispelAssignments();
        }

        m_statistics.totalUpdates.fetch_add(1, std::memory_order_relaxed);
    }

    // Performance tracking
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    m_statistics.totalUpdateTimeUs.fetch_add(duration.count(), std::memory_order_relaxed);

    // Warn if update took too long
    if (duration.count() > 1000) // >1ms
    {
        TC_LOG_WARN("playerbot", "TacticalCoordinator::Update() - Slow update: {} microseconds",
                    duration.count());
    }
}

void TacticalCoordinator::Reset()
{
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    TC_LOG_DEBUG("playerbot", "TacticalCoordinator::Reset() - Resetting tactical state");

    // Clear combat state
    m_tacticalState.inCombat.store(false, std::memory_order_release);
    m_tacticalState.combatStartTime = 0;
    m_tacticalState.focusTarget.Clear();
    m_tacticalState.priorityTargets.clear();
    m_tacticalState.crowdControlTargets.clear();
    m_tacticalState.interruptQueue.clear();
    m_tacticalState.dispelAssignments.clear();
    m_tacticalState.groupCooldowns.clear();
    m_tacticalState.lastInterruptTime = 0;
    m_tacticalState.lastInterrupter.Clear();
    m_tacticalState.lastDispelTime = 0;

    // Clear assignments
    m_assignments.clear();
}

void TacticalCoordinator::Shutdown()
{
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    TC_LOG_DEBUG("playerbot", "TacticalCoordinator::Shutdown() - Shutting down");

    Reset();
    m_group = nullptr;
}

// ============================================================================
// FOCUS TARGET COORDINATION
// ============================================================================

ObjectGuid TacticalCoordinator::GetFocusTarget() const
{
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);
    return m_tacticalState.focusTarget;
}

void TacticalCoordinator::SetFocusTarget(ObjectGuid targetGuid)
{
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    if (m_tacticalState.focusTarget != targetGuid)
    {
        m_tacticalState.focusTarget = targetGuid;
        m_statistics.focusTargetChanges.fetch_add(1, std::memory_order_relaxed);

        TC_LOG_DEBUG("playerbot", "TacticalCoordinator::SetFocusTarget() - Focus target changed to {}",
                     targetGuid.ToString());
    }
}

std::vector<ObjectGuid> TacticalCoordinator::GetPriorityTargets() const
{
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);
    return m_tacticalState.priorityTargets;
}

void TacticalCoordinator::AddPriorityTarget(ObjectGuid targetGuid, uint32 priority)
{
    if (targetGuid.IsEmpty())
        return;

    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    // Check if already in list
    auto it = std::find(m_tacticalState.priorityTargets.begin(),
                        m_tacticalState.priorityTargets.end(),
                        targetGuid);

    if (it == m_tacticalState.priorityTargets.end())
    {
        // Add to priority list
        // Higher priority targets go first
        m_tacticalState.priorityTargets.push_back(targetGuid);

        TC_LOG_DEBUG("playerbot", "TacticalCoordinator::AddPriorityTarget() - Added target {} with priority {}",
                     targetGuid.ToString(), priority);
    }
}

// ============================================================================
// INTERRUPT COORDINATION
// ============================================================================

ObjectGuid TacticalCoordinator::AssignInterrupt(ObjectGuid targetGuid)
{
    if (targetGuid.IsEmpty())
        return ObjectGuid::Empty;

    // Phase 2 Architecture: Delegate to InterruptCoordinator (single authority)
    if (_interruptCoordinator)
    {
        // InterruptCoordinator handles all interrupt assignment logic
        // Get the caster unit to pass spell info
        Unit* caster = ObjectAccessor::GetUnit(*ObjectAccessor::FindPlayer(targetGuid), targetGuid);
        if (caster)
        {
            // The InterruptCoordinator will handle assignment internally
            // via OnEnemyCastStart when it detects the cast
            TC_LOG_DEBUG("playerbot", "TacticalCoordinator::AssignInterrupt() - Delegated to InterruptCoordinator for {}",
                         targetGuid.ToString());
        }

        m_statistics.interruptsAssigned.fetch_add(1, std::memory_order_relaxed);

        // Query who was assigned (InterruptCoordinator manages rotation)
        auto assignments = _interruptCoordinator->GetPendingAssignments();
        for (auto const& assignment : assignments)
        {
            if (assignment.targetCaster == targetGuid)
                return assignment.assignedBot;
        }

        return ObjectGuid::Empty;
    }

    // Fallback: Legacy implementation (only used if no InterruptCoordinator set)
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    uint32 currentTime = getMSTime();
    ObjectGuid bestInterrupter = GetNextInterrupter();

    if (!bestInterrupter.IsEmpty())
    {
        // Create interrupt assignment
        TacticalAssignment assignment;
        assignment.targetGuid = targetGuid;
        assignment.taskType = "interrupt";
        assignment.priority = 90;
        assignment.assignedTime = currentTime;
        assignment.expirationTime = currentTime + 5000;
        assignment.assignedBot = bestInterrupter;

        m_assignments[bestInterrupter] = assignment;
        m_tacticalState.interruptQueue[bestInterrupter] = currentTime + 24000;
        m_tacticalState.lastInterruptTime = currentTime;
        m_tacticalState.lastInterrupter = bestInterrupter;

        m_statistics.interruptsAssigned.fetch_add(1, std::memory_order_relaxed);

        TC_LOG_DEBUG("playerbot", "TacticalCoordinator::AssignInterrupt() - Fallback: Assigned {} to interrupt {}",
                     bestInterrupter.ToString(), targetGuid.ToString());

        return bestInterrupter;
    }

    TC_LOG_DEBUG("playerbot", "TacticalCoordinator::AssignInterrupt() - No interrupter available for target {}",
                 targetGuid.ToString());

    return ObjectGuid::Empty;
}

void TacticalCoordinator::RegisterInterrupter(ObjectGuid botGuid, uint32 cooldownMs)
{
    if (botGuid.IsEmpty())
        return;

    // Phase 2 Architecture: InterruptCoordinator handles registration via RegisterBot()
    // This method is kept for backward compatibility but InterruptCoordinator should be
    // the primary registration point via BotAI initialization
    if (_interruptCoordinator)
    {
        TC_LOG_DEBUG("playerbot", "TacticalCoordinator::RegisterInterrupter() - InterruptCoordinator handles registration for {}",
                     botGuid.ToString());
        // InterruptCoordinator's RegisterBot() is called elsewhere during bot init
        return;
    }

    // Fallback: Legacy implementation
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);
    m_tacticalState.interruptQueue[botGuid] = 0;

    TC_LOG_DEBUG("playerbot", "TacticalCoordinator::RegisterInterrupter() - Fallback: Registered {} with {}ms cooldown",
                 botGuid.ToString(), cooldownMs);
}

void TacticalCoordinator::ReportInterruptUsed(ObjectGuid botGuid, uint32 cooldownMs)
{
    if (botGuid.IsEmpty())
        return;

    // Phase 2 Architecture: Delegate to InterruptCoordinator (single authority)
    if (_interruptCoordinator)
    {
        _interruptCoordinator->UpdateBotCooldown(botGuid, cooldownMs);
        TC_LOG_DEBUG("playerbot", "TacticalCoordinator::ReportInterruptUsed() - Delegated to InterruptCoordinator for {}",
                     botGuid.ToString());
        return;
    }

    // Fallback: Legacy implementation
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    uint32 currentTime = getMSTime();
    m_tacticalState.interruptQueue[botGuid] = currentTime + cooldownMs;
    m_tacticalState.lastInterruptTime = currentTime;
    m_tacticalState.lastInterrupter = botGuid;

    TC_LOG_DEBUG("playerbot", "TacticalCoordinator::ReportInterruptUsed() - Fallback: {} used interrupt, ready at {}",
                 botGuid.ToString(), currentTime + cooldownMs);
}

bool TacticalCoordinator::IsNextInterrupter(ObjectGuid botGuid) const
{
    if (botGuid.IsEmpty())
        return false;

    // Phase 2 Architecture: Delegate to InterruptCoordinator (single authority)
    if (_interruptCoordinator)
    {
        ObjectGuid targetGuid;
        uint32 spellId;
        return _interruptCoordinator->ShouldBotInterrupt(botGuid, targetGuid, spellId);
    }

    // Fallback: Legacy implementation
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    ObjectGuid next = GetNextInterrupter();
    return next == botGuid;
}

// ============================================================================
// DISPEL COORDINATION
// ============================================================================

ObjectGuid TacticalCoordinator::AssignDispel(ObjectGuid targetGuid)
{
    if (targetGuid.IsEmpty())
        return ObjectGuid::Empty;

    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    uint32 currentTime = getMSTime();
    ObjectGuid bestDispeller = FindBestDispeller(targetGuid);

    if (!bestDispeller.IsEmpty())
    {
        // Create dispel assignment
        TacticalAssignment assignment;
        assignment.targetGuid = targetGuid;
        assignment.taskType = "dispel";
        assignment.priority = 85; // High priority
        assignment.assignedTime = currentTime;
        assignment.expirationTime = currentTime + 3000; // 3 second window
        assignment.assignedBot = bestDispeller;

        m_assignments[bestDispeller] = assignment;
        m_tacticalState.dispelAssignments[bestDispeller] = targetGuid;
        m_tacticalState.lastDispelTime = currentTime;

        m_statistics.dispelsAssigned.fetch_add(1, std::memory_order_relaxed);

        TC_LOG_DEBUG("playerbot", "TacticalCoordinator::AssignDispel() - Assigned {} to dispel {}",
                     bestDispeller.ToString(), targetGuid.ToString());

        return bestDispeller;
    }

    TC_LOG_DEBUG("playerbot", "TacticalCoordinator::AssignDispel() - No dispeller available for target {}",
                 targetGuid.ToString());

    return ObjectGuid::Empty;
}

ObjectGuid TacticalCoordinator::GetDispelAssignment(ObjectGuid botGuid) const
{
    if (botGuid.IsEmpty())
        return ObjectGuid::Empty;

    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    auto it = m_tacticalState.dispelAssignments.find(botGuid);
    if (it != m_tacticalState.dispelAssignments.end())
        return it->second;

    return ObjectGuid::Empty;
}

void TacticalCoordinator::ReportDispelCompleted(ObjectGuid botGuid, ObjectGuid targetGuid)
{
    if (botGuid.IsEmpty())
        return;

    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    // Remove dispel assignment
    m_tacticalState.dispelAssignments.erase(botGuid);

    // Clear assignment if it was a dispel
    auto it = m_assignments.find(botGuid);
    if (it != m_assignments.end() && it->second.taskType == "dispel")
    {
        m_assignments.erase(it);
    }

    TC_LOG_DEBUG("playerbot", "TacticalCoordinator::ReportDispelCompleted() - {} completed dispel on {}",
                 botGuid.ToString(), targetGuid.ToString());
}

// ============================================================================
// CROWD CONTROL COORDINATION
// ============================================================================

void TacticalCoordinator::AssignCrowdControl(ObjectGuid targetGuid, ObjectGuid assignedBot)
{
    if (targetGuid.IsEmpty() || assignedBot.IsEmpty())
        return;

    // Phase 2 Architecture: Delegate to CrowdControlManager (single authority)
    if (_ccManager)
    {
        // CrowdControlManager handles all CC logic - just log that we're delegating
        // The actual CC application is done by the bot's CrowdControlManager instance
        TC_LOG_DEBUG("playerbot", "TacticalCoordinator::AssignCrowdControl() - Delegated to CrowdControlManager (target: {}, bot: {})",
                     targetGuid.ToString(), assignedBot.ToString());
        return;
    }

    // Fallback: Legacy implementation (only used if no CrowdControlManager set)
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    // Add to CC targets if not already there
    auto it = std::find(m_tacticalState.crowdControlTargets.begin(),
                        m_tacticalState.crowdControlTargets.end(),
                        targetGuid);

    if (it == m_tacticalState.crowdControlTargets.end())
    {
        m_tacticalState.crowdControlTargets.push_back(targetGuid);
    }

    // Create CC assignment
    uint32 currentTime = getMSTime();
    TacticalAssignment assignment;
    assignment.targetGuid = targetGuid;
    assignment.taskType = "cc";
    assignment.priority = 80;
    assignment.assignedTime = currentTime;
    assignment.expirationTime = currentTime + 30000; // 30 second window
    assignment.assignedBot = assignedBot;

    m_assignments[assignedBot] = assignment;

    TC_LOG_DEBUG("playerbot", "TacticalCoordinator::AssignCrowdControl() - Assigned {} to CC {}",
                 assignedBot.ToString(), targetGuid.ToString());
}

std::vector<ObjectGuid> TacticalCoordinator::GetCrowdControlTargets() const
{
    // Phase 2 Architecture: Delegate to CrowdControlManager (single authority)
    if (_ccManager)
    {
        // CrowdControlManager returns vector<Unit*>, convert to vector<ObjectGuid>
        std::vector<ObjectGuid> targets;
        auto ccdUnits = _ccManager->GetCCdTargets();
        targets.reserve(ccdUnits.size());
        for (Unit* unit : ccdUnits)
        {
            if (unit)
                targets.push_back(unit->GetGUID());
        }
        return targets;
    }

    // Fallback: Legacy implementation (only used if no CrowdControlManager set)
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);
    return m_tacticalState.crowdControlTargets;
}

bool TacticalCoordinator::IsTargetCrowdControlled(ObjectGuid targetGuid) const
{
    if (targetGuid.IsEmpty())
        return false;

    // Phase 2 Architecture: Delegate to CrowdControlManager (single authority)
    if (_ccManager)
    {
        // Need to find Unit* from GUID to call CrowdControlManager
        // CrowdControlManager::IsTargetCCd() takes Unit* parameter
        // For now, iterate CC'd targets to find match by GUID
        auto ccdUnits = _ccManager->GetCCdTargets();
        for (Unit* unit : ccdUnits)
        {
            if (unit && unit->GetGUID() == targetGuid)
                return true;
        }
        return false;
    }

    // Fallback: Legacy implementation (only used if no CrowdControlManager set)
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    auto it = std::find(m_tacticalState.crowdControlTargets.begin(),
                        m_tacticalState.crowdControlTargets.end(),
                        targetGuid);

    return it != m_tacticalState.crowdControlTargets.end();
}

// ============================================================================
// GROUP COOLDOWN COORDINATION
// ============================================================================

bool TacticalCoordinator::IsGroupCooldownAvailable(std::string const& cooldownName) const
{
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    auto it = m_tacticalState.groupCooldowns.find(cooldownName);
    if (it == m_tacticalState.groupCooldowns.end())
        return true; // Not on cooldown

    uint32 currentTime = getMSTime();
    return currentTime >= it->second;
}

void TacticalCoordinator::UseGroupCooldown(std::string const& cooldownName, uint32 durationMs)
{
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    uint32 currentTime = getMSTime();
    m_tacticalState.groupCooldowns[cooldownName] = currentTime + durationMs;

    m_statistics.cooldownsUsed.fetch_add(1, std::memory_order_relaxed);

    TC_LOG_DEBUG("playerbot", "TacticalCoordinator::UseGroupCooldown() - {} used, available at {}",
                 cooldownName, currentTime + durationMs);
}

uint32 TacticalCoordinator::GetGroupCooldownRemaining(std::string const& cooldownName) const
{
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    auto it = m_tacticalState.groupCooldowns.find(cooldownName);
    if (it == m_tacticalState.groupCooldowns.end())
        return 0;

    uint32 currentTime = getMSTime();
    if (currentTime >= it->second)
        return 0;

    return it->second - currentTime;
}

// ============================================================================
// TACTICAL ASSIGNMENTS
// ============================================================================

TacticalAssignment const* TacticalCoordinator::GetAssignment(ObjectGuid botGuid) const
{
    if (botGuid.IsEmpty())
        return nullptr;

    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    auto it = m_assignments.find(botGuid);
    if (it != m_assignments.end())
    {
        // Check if still valid
        if (it->second.IsValid(getMSTime()))
            return &it->second;
    }

    return nullptr;
}

void TacticalCoordinator::ClearAssignment(ObjectGuid botGuid)
{
    if (botGuid.IsEmpty())
        return;

    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    m_assignments.erase(botGuid);

    TC_LOG_DEBUG("playerbot", "TacticalCoordinator::ClearAssignment() - Cleared assignment for {}",
                 botGuid.ToString());
}

void TacticalCoordinator::ClearAllAssignments()
{
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    size_t count = m_assignments.size();
    m_assignments.clear();

    TC_LOG_DEBUG("playerbot", "TacticalCoordinator::ClearAllAssignments() - Cleared {} assignments", count);
}

// ============================================================================
// COMBAT STATE
// ============================================================================

uint32 TacticalCoordinator::GetCombatDuration() const
{
    if (!m_tacticalState.inCombat.load(std::memory_order_acquire))
        return 0;

    uint32 currentTime = getMSTime();
    return currentTime - m_tacticalState.combatStartTime;
}

void TacticalCoordinator::SetCombatState(bool inCombat)
{
    bool wasInCombat = m_tacticalState.inCombat.load(std::memory_order_acquire);
    m_tacticalState.inCombat.store(inCombat, std::memory_order_release);

    if (inCombat && !wasInCombat)
    {
        // Entering combat
        std::lock_guard<decltype(m_mutex)> lock(m_mutex);
        m_tacticalState.combatStartTime = getMSTime();

        TC_LOG_DEBUG("playerbot", "TacticalCoordinator::SetCombatState() - Entering combat");
    }
    else if (!inCombat && wasInCombat)
    {
        // Leaving combat
        TC_LOG_DEBUG("playerbot", "TacticalCoordinator::SetCombatState() - Leaving combat");
        Reset();
    }
}

// ============================================================================
// TACTICAL STATE ACCESS
// ============================================================================

GroupTacticalState const& TacticalCoordinator::GetTacticalState() const
{
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);
    return m_tacticalState;
}

// ============================================================================
// INTERNAL UPDATE METHODS
// ============================================================================

void TacticalCoordinator::UpdateFocusTarget()
{
    // Called with lock already held

    ObjectGuid newFocus = FindBestFocusTarget();

    if (newFocus != m_tacticalState.focusTarget)
    {
        m_tacticalState.focusTarget = newFocus;
        m_statistics.focusTargetChanges.fetch_add(1, std::memory_order_relaxed);

        TC_LOG_DEBUG("playerbot", "TacticalCoordinator::UpdateFocusTarget() - Focus changed to {}",
                     newFocus.ToString());
    }
}

void TacticalCoordinator::UpdateInterruptRotation()
{
    // Called with lock already held

    // Phase 2 Architecture: InterruptCoordinator handles all rotation
    // This is only used as fallback when no InterruptCoordinator is set
    if (_interruptCoordinator)
    {
        // InterruptCoordinator handles its own update cycle
        return;
    }

    // Fallback: Legacy implementation
    uint32 currentTime = getMSTime();

    // Clean up expired interrupt cooldowns
    for (auto it = m_tacticalState.interruptQueue.begin();
         it != m_tacticalState.interruptQueue.end(); )
    {
        if (it->second > 0 && currentTime >= it->second)
        {
            it->second = 0; // Mark as ready
            ++it;
        }
        else
        {
            ++it;
        }
    }
}

void TacticalCoordinator::UpdateDispelAssignments()
{
    // Called with lock already held

    uint32 currentTime = getMSTime();

    // Remove expired dispel assignments
    for (auto it = m_tacticalState.dispelAssignments.begin();
         it != m_tacticalState.dispelAssignments.end(); )
    {
        // Check if assignment expired
        auto assignmentIt = m_assignments.find(it->first);
        if (assignmentIt != m_assignments.end())
        {
            if (assignmentIt->second.IsExpired(currentTime))
            {
                it = m_tacticalState.dispelAssignments.erase(it);
                continue;
            }
        }
        ++it;
    }
}

void TacticalCoordinator::CleanupExpiredData(uint32 currentTime)
{
    // Called with lock already held

    // Clean up expired assignments
    for (auto it = m_assignments.begin(); it != m_assignments.end(); )
    {
        if (it->second.IsExpired(currentTime))
        {
            TC_LOG_DEBUG("playerbot", "TacticalCoordinator::CleanupExpiredData() - Removing expired assignment for {}",
                         it->first.ToString());
            it = m_assignments.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Clean up expired group cooldowns
    for (auto it = m_tacticalState.groupCooldowns.begin();
         it != m_tacticalState.groupCooldowns.end(); )
    {
        if (currentTime >= it->second)
        {
            TC_LOG_DEBUG("playerbot", "TacticalCoordinator::CleanupExpiredData() - Cooldown {} expired",
                         it->first);
            it = m_tacticalState.groupCooldowns.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Clean up invalid CC targets
    for (auto it = m_tacticalState.crowdControlTargets.begin();
         it != m_tacticalState.crowdControlTargets.end(); )
    {
        // ENHANCEMENT: Validate target still exists and is CC'd
        // Implementation: Use ObjectAccessor::GetUnit() to check existence, verify CC auras
        // For now, keep all targets (cleanup happens on combat end)
        ++it;
    }
}

ObjectGuid TacticalCoordinator::FindBestFocusTarget() const
{
    // Called with lock already held

    if (!m_group)
        return ObjectGuid::Empty;

    // Use first priority target if available
    if (!m_tacticalState.priorityTargets.empty())
        return m_tacticalState.priorityTargets.front();

    // ENHANCEMENT: Implement threat-based focus target selection
    // Implementation: Iterate group members, get ThreatManager, find target with highest cumulative threat
    // For now, return current focus or empty (manual priority targets only)
    return m_tacticalState.focusTarget;
}

ObjectGuid TacticalCoordinator::GetNextInterrupter() const
{
    // Called with lock already held

    uint32 currentTime = getMSTime();
    ObjectGuid bestInterrupter = ObjectGuid::Empty;
    uint32 earliestReady = std::numeric_limits<uint32>::max();

    for (auto const& pair : m_tacticalState.interruptQueue)
    {
        uint32 readyTime = pair.second;

        // If interrupt is ready now
        if (readyTime == 0 || currentTime >= readyTime)
        {
            return pair.first; // Return first ready interrupter
        }

        // Track earliest ready interrupter
        if (readyTime < earliestReady)
        {
            earliestReady = readyTime;
            bestInterrupter = pair.first;
        }
    }

    // Return empty if no interrupters registered or none ready
    return ObjectGuid::Empty;
}

ObjectGuid TacticalCoordinator::FindBestDispeller(ObjectGuid /*targetGuid*/) const
{
    // Called with lock already held

    if (!m_group)
        return ObjectGuid::Empty;

    // ENHANCEMENT: Implement proper dispeller selection based on:
    // - Class/spec capabilities (magic/curse/poison/disease) via CanDispelAuraType()
    // - Proximity to target (GetDistance < 40 yards)
    // - Current workload (check dispelAssignments count per bot)
    // Implementation: Iterate group members, score each by capability+proximity+workload

    // For now, return empty (no dispeller logic implemented - manual dispel only)
    return ObjectGuid::Empty;
}

} // namespace Advanced
} // namespace Playerbot
