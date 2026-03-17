/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * CooldownSyncCoordinator - Raid-wide cooldown synchronization
 *
 * See CooldownSyncCoordinator.h for design overview and usage documentation.
 */

#include "CooldownSyncCoordinator.h"
#include "Player.h"
#include "Map.h"
#include "Log.h"
#include "GameTime.h"
#include "SharedDefines.h"
#include <algorithm>

namespace Playerbot
{

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

CooldownSyncCoordinator::CooldownSyncCoordinator()
{
}

CooldownSyncCoordinator::~CooldownSyncCoordinator()
{
    Shutdown();
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void CooldownSyncCoordinator::Initialize()
{
    if (_initialized.exchange(true))
        return; // Already initialized

    std::unique_lock lock(_mutex);
    _encounterStates.clear();
    _cooldowns.clear();
    _botToInstance.clear();
    _bloodlustOnPullOverride.clear();
    _updateTimer = 0;
    _config = CooldownSyncConfig();

    TC_LOG_DEBUG("module.playerbot", "CooldownSyncCoordinator::Initialize - Coordinator initialized");
}

void CooldownSyncCoordinator::Shutdown()
{
    if (!_initialized.exchange(false))
        return; // Already shut down

    std::unique_lock lock(_mutex);
    _encounterStates.clear();
    _cooldowns.clear();
    _botToInstance.clear();
    _bloodlustOnPullOverride.clear();

    TC_LOG_DEBUG("module.playerbot", "CooldownSyncCoordinator::Shutdown - Coordinator shut down");
}

void CooldownSyncCoordinator::Update(uint32 diff)
{
    if (!_initialized.load(std::memory_order_relaxed))
        return;

    _updateTimer += diff;
    if (_updateTimer < _config.updateIntervalMs)
        return;

    uint32 elapsed = _updateTimer;
    _updateTimer = 0;

    std::unique_lock lock(_mutex);
    UpdateCooldownTimers(elapsed);
    UpdateBurstWindows(elapsed);
}

// ============================================================================
// ENCOUNTER LIFECYCLE
// ============================================================================

void CooldownSyncCoordinator::OnEncounterStart(uint32 instanceId, uint32 encounterId)
{
    if (!_initialized.load(std::memory_order_relaxed))
        return;

    std::unique_lock lock(_mutex);

    EncounterCooldownState& state = _encounterStates[instanceId];
    state.instanceId = instanceId;
    state.encounterId = encounterId;
    state.isActive = true;
    state.encounterStartTime = GetServerTimeMs();
    state.lastPhaseChangeTime = state.encounterStartTime;
    state.currentPhase = 1;
    state.bloodlustUsed = false;
    state.bloodlustCaster = ObjectGuid();
    state.bloodlustTime = 0;
    state.healerCDRotationIndex = 0;
    state.lastEmergencyTime = 0;
    state.totalBurstWindows = 0;
    state.totalEmergencies = 0;

    // Determine if we should use Bloodlust on pull
    bool blOnPull = _config.bloodlustOnPull;
    auto overrideIt = _bloodlustOnPullOverride.find(instanceId);
    if (overrideIt != _bloodlustOnPullOverride.end())
        blOnPull = overrideIt->second;

    // Open pull burst window
    state.currentWindow.window = CooldownWindow::PULL_BURST;
    state.currentWindow.windowStartTime = state.encounterStartTime;
    state.currentWindow.windowDurationMs = _config.pullBurstDurationMs;
    state.currentWindow.instanceId = instanceId;
    state.currentWindow.phaseNumber = 1;
    state.currentWindow.bossHealthPct = 100.0f;
    state.currentWindow.raidHealthPct = 100.0f;
    state.currentWindow.bloodlustUsedThisEncounter = false;
    state.totalBurstWindows++;

    TC_LOG_DEBUG("module.playerbot",
        "CooldownSyncCoordinator::OnEncounterStart - Encounter %u started in instance %u, "
        "PULL_BURST window open for %u ms, BL on pull: %s",
        encounterId, instanceId, _config.pullBurstDurationMs,
        blOnPull ? "YES" : "NO");
}

void CooldownSyncCoordinator::OnEncounterEnd(uint32 instanceId, uint32 encounterId, bool success)
{
    if (!_initialized.load(std::memory_order_relaxed))
        return;

    std::unique_lock lock(_mutex);

    auto it = _encounterStates.find(instanceId);
    if (it == _encounterStates.end())
        return;

    EncounterCooldownState& state = it->second;

    TC_LOG_DEBUG("module.playerbot",
        "CooldownSyncCoordinator::OnEncounterEnd - Encounter %u in instance %u ended (%s). "
        "Stats: %u burst windows, %u emergencies, BL used: %s",
        encounterId, instanceId, success ? "KILL" : "WIPE",
        state.totalBurstWindows, state.totalEmergencies,
        state.bloodlustUsed ? "YES" : "NO");

    state.isActive = false;
    state.currentWindow.window = CooldownWindow::NONE;

    // Remove encounter state after a short delay to allow final queries
    // (In practice, we clear on next encounter start for same instance)
}

void CooldownSyncCoordinator::OnPhaseChange(uint32 instanceId, uint8 newPhase)
{
    if (!_initialized.load(std::memory_order_relaxed))
        return;

    std::unique_lock lock(_mutex);

    EncounterCooldownState* state = GetEncounterStateMut(instanceId);
    if (!state || !state->isActive)
        return;

    uint8 oldPhase = state->currentPhase;
    state->currentPhase = newPhase;
    state->lastPhaseChangeTime = GetServerTimeMs();

    // Open phase transition burst window
    state->currentWindow.window = CooldownWindow::PHASE_TRANSITION;
    state->currentWindow.windowStartTime = state->lastPhaseChangeTime;
    state->currentWindow.windowDurationMs = _config.phaseTransitionBurstMs;
    state->currentWindow.phaseNumber = newPhase;
    state->totalBurstWindows++;

    TC_LOG_DEBUG("module.playerbot",
        "CooldownSyncCoordinator::OnPhaseChange - Instance %u phase %u -> %u, "
        "PHASE_TRANSITION burst window open for %u ms",
        instanceId, oldPhase, newPhase, _config.phaseTransitionBurstMs);
}

// ============================================================================
// COOLDOWN REGISTRATION & TRACKING
// ============================================================================

void CooldownSyncCoordinator::RegisterCooldown(ObjectGuid botGuid, uint32 spellId, uint32 durationMs)
{
    if (!_initialized.load(std::memory_order_relaxed))
        return;

    std::unique_lock lock(_mutex);

    TrackedCooldown& cd = _cooldowns[botGuid][spellId];
    cd.botGuid = botGuid;
    cd.spellId = spellId;
    cd.cooldownDurationMs = durationMs;
    cd.remainingMs = durationMs;
    cd.registeredAt = GetServerTimeMs();
    cd.isReady = false;

    // Categorize the spell for the bot
    // We need to release the lock briefly to look up the player, but
    // since we only read class info (which doesn't change), we can
    // do the categorization without the player pointer if we stored
    // category on first registration. For now, check if category is
    // already set from a previous registration.
    if (cd.category == CooldownCategory::PERSONAL)
    {
        // Try to categorize based on known spell IDs
        if (CooldownSpells::IsBloodlustSpell(spellId))
            cd.category = CooldownCategory::BLOODLUST;
        else if (CooldownSpells::IsRaidHealingCD(spellId))
            cd.category = CooldownCategory::HEALER_RAID;
        else if (CooldownSpells::IsRaidDefensiveCD(spellId))
            cd.category = CooldownCategory::RAID_DEFENSIVE;
        else if (durationMs >= 120000)
            cd.category = CooldownCategory::DPS_MAJOR;
        else if (durationMs >= 30000)
            cd.category = CooldownCategory::DPS_MINOR;
    }

    // Track Bloodlust usage at encounter level
    if (CooldownSpells::IsBloodlustSpell(spellId))
    {
        uint32 instanceId = GetInstanceForBot(botGuid);
        if (instanceId != 0)
        {
            EncounterCooldownState* state = GetEncounterStateMut(instanceId);
            if (state && state->isActive && !state->bloodlustUsed)
            {
                state->bloodlustUsed = true;
                state->bloodlustCaster = botGuid;
                state->bloodlustTime = GetServerTimeMs();
                state->currentWindow.bloodlustUsedThisEncounter = true;

                TC_LOG_DEBUG("module.playerbot",
                    "CooldownSyncCoordinator::RegisterCooldown - Bloodlust used in instance %u "
                    "by bot " UI64FMTD " at server time %u",
                    instanceId, botGuid.GetCounter(), state->bloodlustTime);
            }
        }
    }

    TC_LOG_DEBUG("module.playerbot",
        "CooldownSyncCoordinator::RegisterCooldown - Bot " UI64FMTD " used spell %u "
        "(category: %s, CD: %u ms)",
        botGuid.GetCounter(), spellId,
        CooldownCategoryToString(cd.category), durationMs);
}

void CooldownSyncCoordinator::UnregisterBot(ObjectGuid botGuid)
{
    if (!_initialized.load(std::memory_order_relaxed))
        return;

    std::unique_lock lock(_mutex);

    _cooldowns.erase(botGuid);
    _botToInstance.erase(botGuid);

    TC_LOG_DEBUG("module.playerbot",
        "CooldownSyncCoordinator::UnregisterBot - Bot " UI64FMTD " unregistered",
        botGuid.GetCounter());
}

void CooldownSyncCoordinator::ReportRaidHealth(uint32 instanceId, float raidHealthPct)
{
    if (!_initialized.load(std::memory_order_relaxed))
        return;

    std::unique_lock lock(_mutex);

    EncounterCooldownState* state = GetEncounterStateMut(instanceId);
    if (!state || !state->isActive)
        return;

    state->currentWindow.raidHealthPct = raidHealthPct;

    // Evaluate emergency conditions with the updated health
    EvaluateEmergencyConditions(*state, GetServerTimeMs());
}

void CooldownSyncCoordinator::ReportBossHealth(uint32 instanceId, float bossHealthPct)
{
    if (!_initialized.load(std::memory_order_relaxed))
        return;

    std::unique_lock lock(_mutex);

    EncounterCooldownState* state = GetEncounterStateMut(instanceId);
    if (!state || !state->isActive)
        return;

    state->currentWindow.bossHealthPct = bossHealthPct;

    // Evaluate execute phase
    EvaluateExecutePhase(*state);
}

// ============================================================================
// DECISION QUERIES
// ============================================================================

bool CooldownSyncCoordinator::ShouldUseBurstCD(Player* bot) const
{
    if (!_initialized.load(std::memory_order_relaxed) || !bot)
        return false;

    std::shared_lock lock(_mutex);

    ObjectGuid botGuid = bot->GetGUID();
    uint32 instanceId = GetInstanceForBot(botGuid);
    if (instanceId == 0)
    {
        // Bot not tracked to a specific instance; try to find by map instance
        if (bot->GetMap())
            instanceId = bot->GetMap()->GetInstanceId();
    }

    const EncounterCooldownState* state = GetEncounterState(instanceId);
    if (!state || !state->isActive)
        return false;

    // Burst CDs should be used during any active burst window
    CooldownWindow window = state->currentWindow.window;
    switch (window)
    {
        case CooldownWindow::PULL_BURST:
        case CooldownWindow::PHASE_TRANSITION:
        case CooldownWindow::EXECUTE_PHASE:
        case CooldownWindow::INTERMISSION_END:
            return true;
        case CooldownWindow::EMERGENCY:
            // During emergency, only healers/defensives matter, not DPS burst
            return false;
        case CooldownWindow::NONE:
        default:
            return false;
    }
}

bool CooldownSyncCoordinator::ShouldUseBloodlust(Player* bot) const
{
    if (!_initialized.load(std::memory_order_relaxed) || !bot)
        return false;

    std::shared_lock lock(_mutex);

    ObjectGuid botGuid = bot->GetGUID();
    uint32 instanceId = GetInstanceForBot(botGuid);
    if (instanceId == 0)
    {
        if (bot->GetMap())
            instanceId = bot->GetMap()->GetInstanceId();
    }

    const EncounterCooldownState* state = GetEncounterState(instanceId);
    if (!state || !state->isActive)
        return false;

    // Already used this encounter
    if (state->bloodlustUsed)
        return false;

    // Check if this bot can actually cast a Bloodlust-type spell
    if (!HasBloodlustSpellAvailable(botGuid))
    {
        // Also check by class — Shaman, Mage, Hunter can provide BL
        uint8 playerClass = bot->GetClass();
        if (playerClass != CLASS_SHAMAN && playerClass != CLASS_MAGE && playerClass != CLASS_HUNTER)
            return false;
    }

    // Determine if it's time for Bloodlust
    bool blOnPull = _config.bloodlustOnPull;
    auto overrideIt = _bloodlustOnPullOverride.find(instanceId);
    if (overrideIt != _bloodlustOnPullOverride.end())
        blOnPull = overrideIt->second;

    CooldownWindow window = state->currentWindow.window;

    // If BL on pull is configured and we're in pull burst window
    if (blOnPull && window == CooldownWindow::PULL_BURST)
    {
        // Only the first eligible provider should cast
        ::std::vector<ObjectGuid> providers = GetBloodlustProviders(instanceId);
        if (!providers.empty() && providers.front() == botGuid)
            return true;
        return false;
    }

    // Otherwise, use BL when boss is below threshold (execute phase) or
    // during phase transition if boss health is already low
    float bossHealth = state->currentWindow.bossHealthPct;
    if (bossHealth <= _config.bloodlustHealthThreshold)
    {
        // Select the first available provider
        ::std::vector<ObjectGuid> providers = GetBloodlustProviders(instanceId);
        if (!providers.empty() && providers.front() == botGuid)
            return true;
    }

    return false;
}

bool CooldownSyncCoordinator::ShouldUseHealerCD(Player* bot) const
{
    if (!_initialized.load(std::memory_order_relaxed) || !bot)
        return false;

    std::shared_lock lock(_mutex);

    ObjectGuid botGuid = bot->GetGUID();
    uint32 instanceId = GetInstanceForBot(botGuid);
    if (instanceId == 0)
    {
        if (bot->GetMap())
            instanceId = bot->GetMap()->GetInstanceId();
    }

    const EncounterCooldownState* state = GetEncounterState(instanceId);
    if (!state || !state->isActive)
        return false;

    // Healer CDs are used during EMERGENCY windows
    if (state->currentWindow.window != CooldownWindow::EMERGENCY)
        return false;

    // Check if this bot has a healer CD available
    if (!HasHealerCDAvailable(botGuid))
        return false;

    // Check minimum interval between healer CDs
    if (!IsHealerCDIntervalElapsed(*state))
        return false;

    // Get the rotation order and check if it's this bot's turn
    ::std::vector<ObjectGuid> providers = GetHealerCDProviders(instanceId);
    if (providers.empty())
        return false;

    // Use rotation index to determine whose turn it is
    uint32 rotationIndex = state->healerCDRotationIndex % static_cast<uint32>(providers.size());
    return providers[rotationIndex] == botGuid;
}

bool CooldownSyncCoordinator::ShouldUseRaidDefensive(Player* bot) const
{
    if (!_initialized.load(std::memory_order_relaxed) || !bot)
        return false;

    std::shared_lock lock(_mutex);

    ObjectGuid botGuid = bot->GetGUID();
    uint32 instanceId = GetInstanceForBot(botGuid);
    if (instanceId == 0)
    {
        if (bot->GetMap())
            instanceId = bot->GetMap()->GetInstanceId();
    }

    const EncounterCooldownState* state = GetEncounterState(instanceId);
    if (!state || !state->isActive)
        return false;

    // Raid defensives used during EMERGENCY when raid health is critical
    if (state->currentWindow.window != CooldownWindow::EMERGENCY)
        return false;

    // Only trigger raid defensives at critical threshold (more severe than healer CDs)
    if (state->currentWindow.raidHealthPct > _config.criticalRaidHealthPct)
        return false;

    // Check if this bot has a raid defensive available
    if (!HasRaidDefensiveAvailable(botGuid))
        return false;

    // For raid defensives, use the first available provider (most urgent)
    ::std::vector<ObjectGuid> providers = GetRaidDefensiveProviders(instanceId);
    if (!providers.empty() && providers.front() == botGuid)
        return true;

    return false;
}

// ============================================================================
// WINDOW MANAGEMENT
// ============================================================================

void CooldownSyncCoordinator::OpenBurstWindow(uint32 instanceId, CooldownWindow window, uint32 durationMs)
{
    if (!_initialized.load(std::memory_order_relaxed))
        return;

    std::unique_lock lock(_mutex);

    EncounterCooldownState* state = GetEncounterStateMut(instanceId);
    if (!state || !state->isActive)
        return;

    uint32 duration = durationMs > 0 ? durationMs : GetDefaultWindowDuration(window);

    state->currentWindow.window = window;
    state->currentWindow.windowStartTime = GetServerTimeMs();
    state->currentWindow.windowDurationMs = duration;
    state->totalBurstWindows++;

    TC_LOG_DEBUG("module.playerbot",
        "CooldownSyncCoordinator::OpenBurstWindow - Instance %u: %s window opened for %u ms",
        instanceId, CooldownWindowToString(window), duration);
}

void CooldownSyncCoordinator::CloseBurstWindow(uint32 instanceId)
{
    if (!_initialized.load(std::memory_order_relaxed))
        return;

    std::unique_lock lock(_mutex);

    EncounterCooldownState* state = GetEncounterStateMut(instanceId);
    if (!state)
        return;

    CooldownWindow previousWindow = state->currentWindow.window;
    state->currentWindow.window = CooldownWindow::NONE;
    state->currentWindow.windowDurationMs = 0;

    TC_LOG_DEBUG("module.playerbot",
        "CooldownSyncCoordinator::CloseBurstWindow - Instance %u: %s window closed",
        instanceId, CooldownWindowToString(previousWindow));
}

BurstWindowState CooldownSyncCoordinator::GetCurrentWindow(uint32 instanceId) const
{
    if (!_initialized.load(std::memory_order_relaxed))
        return {};

    std::shared_lock lock(_mutex);

    const EncounterCooldownState* state = GetEncounterState(instanceId);
    if (!state)
        return {};

    return state->currentWindow;
}

bool CooldownSyncCoordinator::IsBurstWindowActive(uint32 instanceId) const
{
    if (!_initialized.load(std::memory_order_relaxed))
        return false;

    std::shared_lock lock(_mutex);

    const EncounterCooldownState* state = GetEncounterState(instanceId);
    if (!state || !state->isActive)
        return false;

    return state->currentWindow.window != CooldownWindow::NONE;
}

// ============================================================================
// QUERY METHODS
// ============================================================================

::std::vector<TrackedCooldown> CooldownSyncCoordinator::GetBotCooldowns(ObjectGuid botGuid) const
{
    if (!_initialized.load(std::memory_order_relaxed))
        return {};

    std::shared_lock lock(_mutex);

    ::std::vector<TrackedCooldown> result;
    auto it = _cooldowns.find(botGuid);
    if (it != _cooldowns.end())
    {
        result.reserve(it->second.size());
        for (const auto& [spellId, cd] : it->second)
            result.push_back(cd);
    }
    return result;
}

::std::vector<TrackedCooldown> CooldownSyncCoordinator::GetAvailableCooldowns(uint32 instanceId, CooldownCategory category) const
{
    if (!_initialized.load(std::memory_order_relaxed))
        return {};

    std::shared_lock lock(_mutex);

    ::std::vector<TrackedCooldown> result;

    for (const auto& [botGuid, spellMap] : _cooldowns)
    {
        // Only include bots in the target instance
        auto instIt = _botToInstance.find(botGuid);
        if (instIt != _botToInstance.end() && instIt->second != instanceId)
            continue;

        for (const auto& [spellId, cd] : spellMap)
        {
            if (cd.category == category && cd.IsAvailable())
                result.push_back(cd);
        }
    }

    return result;
}

bool CooldownSyncCoordinator::IsBloodlustUsed(uint32 instanceId) const
{
    if (!_initialized.load(std::memory_order_relaxed))
        return false;

    std::shared_lock lock(_mutex);

    const EncounterCooldownState* state = GetEncounterState(instanceId);
    if (!state)
        return false;

    return state->bloodlustUsed;
}

uint32 CooldownSyncCoordinator::GetActiveEncounterCount() const
{
    if (!_initialized.load(std::memory_order_relaxed))
        return 0;

    std::shared_lock lock(_mutex);

    uint32 count = 0;
    for (const auto& [instanceId, state] : _encounterStates)
    {
        if (state.isActive)
            ++count;
    }
    return count;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void CooldownSyncCoordinator::SetConfig(const CooldownSyncConfig& config)
{
    std::unique_lock lock(_mutex);
    _config = config;

    TC_LOG_DEBUG("module.playerbot",
        "CooldownSyncCoordinator::SetConfig - Configuration updated: "
        "pullBurst=%u ms, phaseTransition=%u ms, execute=%.1f%%, "
        "emergency=%.1f%%, critical=%.1f%%",
        _config.pullBurstDurationMs, _config.phaseTransitionBurstMs,
        _config.executePhaseThreshold, _config.emergencyRaidHealthPct,
        _config.criticalRaidHealthPct);
}

void CooldownSyncCoordinator::SetBloodlustOnPull(uint32 instanceId, bool onPull)
{
    std::unique_lock lock(_mutex);
    _bloodlustOnPullOverride[instanceId] = onPull;

    TC_LOG_DEBUG("module.playerbot",
        "CooldownSyncCoordinator::SetBloodlustOnPull - Instance %u: BL on pull = %s",
        instanceId, onPull ? "YES" : "NO");
}

// ============================================================================
// INTERNAL: TIMER UPDATES
// ============================================================================

void CooldownSyncCoordinator::UpdateCooldownTimers(uint32 diff)
{
    // Caller must hold unique lock
    for (auto& [botGuid, spellMap] : _cooldowns)
    {
        for (auto& [spellId, cd] : spellMap)
        {
            if (cd.remainingMs > 0)
            {
                if (cd.remainingMs <= diff)
                {
                    cd.remainingMs = 0;
                    cd.isReady = true;

                    TC_LOG_DEBUG("module.playerbot",
                        "CooldownSyncCoordinator::UpdateCooldownTimers - Bot " UI64FMTD
                        " spell %u is now ready (category: %s)",
                        botGuid.GetCounter(), spellId,
                        CooldownCategoryToString(cd.category));
                }
                else
                {
                    cd.remainingMs -= diff;
                }
            }
        }
    }
}

void CooldownSyncCoordinator::UpdateBurstWindows(uint32 diff)
{
    // Caller must hold unique lock
    uint32 serverTime = GetServerTimeMs();

    for (auto& [instanceId, state] : _encounterStates)
    {
        if (!state.isActive)
            continue;

        BurstWindowState& window = state.currentWindow;

        // Skip NONE and indefinite windows (EXECUTE_PHASE has no duration)
        if (window.window == CooldownWindow::NONE)
            continue;

        // EXECUTE_PHASE is indefinite until encounter ends
        if (window.window == CooldownWindow::EXECUTE_PHASE)
            continue;

        // Check if timed window has expired
        if (window.windowDurationMs > 0)
        {
            uint32 elapsed = serverTime - window.windowStartTime;
            if (elapsed >= window.windowDurationMs)
            {
                CooldownWindow expiredWindow = window.window;
                window.window = CooldownWindow::NONE;
                window.windowDurationMs = 0;

                TC_LOG_DEBUG("module.playerbot",
                    "CooldownSyncCoordinator::UpdateBurstWindows - Instance %u: "
                    "%s window expired after %u ms",
                    instanceId, CooldownWindowToString(expiredWindow), elapsed);
            }
        }
    }
}

void CooldownSyncCoordinator::EvaluateEmergencyConditions(EncounterCooldownState& state, uint32 serverTimeMs)
{
    // Caller must hold unique lock
    if (!state.isActive)
        return;

    float raidHealth = state.currentWindow.raidHealthPct;

    // Check if raid health is below emergency threshold
    if (raidHealth > _config.emergencyRaidHealthPct)
    {
        // If we're in an emergency window and health recovered, close it
        if (state.currentWindow.window == CooldownWindow::EMERGENCY && raidHealth > _config.emergencyRaidHealthPct + 10.0f)
        {
            state.currentWindow.window = CooldownWindow::NONE;
            state.currentWindow.windowDurationMs = 0;

            TC_LOG_DEBUG("module.playerbot",
                "CooldownSyncCoordinator::EvaluateEmergencyConditions - Instance %u: "
                "EMERGENCY window closed, raid health recovered to %.1f%%",
                state.instanceId, raidHealth);
        }
        return;
    }

    // Don't open emergency if we already have one active
    if (state.currentWindow.window == CooldownWindow::EMERGENCY)
        return;

    // Enforce minimum interval between emergencies to prevent spam
    if (state.lastEmergencyTime > 0 &&
        (serverTimeMs - state.lastEmergencyTime) < state.emergencyCooldownMs)
        return;

    // Don't override higher-priority windows (pull burst, execute)
    // Pull burst and execute should not be interrupted by emergency
    if (state.currentWindow.window == CooldownWindow::PULL_BURST ||
        state.currentWindow.window == CooldownWindow::EXECUTE_PHASE)
        return;

    // Open emergency window
    state.currentWindow.window = CooldownWindow::EMERGENCY;
    state.currentWindow.windowStartTime = serverTimeMs;
    state.currentWindow.windowDurationMs = 0; // Emergency lasts until health recovers
    state.lastEmergencyTime = serverTimeMs;
    state.totalEmergencies++;

    // Advance healer CD rotation so the next healer in line responds
    state.healerCDRotationIndex++;

    TC_LOG_DEBUG("module.playerbot",
        "CooldownSyncCoordinator::EvaluateEmergencyConditions - Instance %u: "
        "EMERGENCY window opened! Raid health: %.1f%%, healer rotation index: %u",
        state.instanceId, raidHealth, state.healerCDRotationIndex);
}

void CooldownSyncCoordinator::EvaluateExecutePhase(EncounterCooldownState& state)
{
    // Caller must hold unique lock
    if (!state.isActive)
        return;

    float bossHealth = state.currentWindow.bossHealthPct;

    // Already in execute phase
    if (state.currentWindow.window == CooldownWindow::EXECUTE_PHASE)
        return;

    // Check if boss is below execute threshold
    if (bossHealth > _config.executePhaseThreshold)
        return;

    // Don't override pull burst (it will expire naturally)
    if (state.currentWindow.window == CooldownWindow::PULL_BURST)
        return;

    // Enter execute phase — this is an indefinite window (no duration)
    state.currentWindow.window = CooldownWindow::EXECUTE_PHASE;
    state.currentWindow.windowStartTime = GetServerTimeMs();
    state.currentWindow.windowDurationMs = 0; // Indefinite
    state.totalBurstWindows++;

    TC_LOG_DEBUG("module.playerbot",
        "CooldownSyncCoordinator::EvaluateExecutePhase - Instance %u: "
        "EXECUTE_PHASE entered, boss health: %.1f%%",
        state.instanceId, bossHealth);
}

// ============================================================================
// INTERNAL: CATEGORIZATION
// ============================================================================

CooldownCategory CooldownSyncCoordinator::CategorizeSpell(uint32 spellId, Player* bot) const
{
    // Check known spell types first
    if (CooldownSpells::IsBloodlustSpell(spellId))
        return CooldownCategory::BLOODLUST;

    if (CooldownSpells::IsRaidHealingCD(spellId))
        return CooldownCategory::HEALER_RAID;

    if (CooldownSpells::IsRaidDefensiveCD(spellId))
        return CooldownCategory::RAID_DEFENSIVE;

    // Default categorization based on the spell's cooldown duration would
    // require SpellInfo lookup. For registered cooldowns, we categorize by
    // duration at registration time (see RegisterCooldown).
    return CooldownCategory::PERSONAL;
}

// ============================================================================
// INTERNAL: LOOKUP HELPERS
// ============================================================================

uint32 CooldownSyncCoordinator::GetInstanceForBot(ObjectGuid botGuid) const
{
    // Caller must hold at least shared lock
    auto it = _botToInstance.find(botGuid);
    if (it != _botToInstance.end())
        return it->second;
    return 0;
}

const EncounterCooldownState* CooldownSyncCoordinator::GetEncounterState(uint32 instanceId) const
{
    // Caller must hold at least shared lock
    auto it = _encounterStates.find(instanceId);
    if (it != _encounterStates.end())
        return &it->second;
    return nullptr;
}

EncounterCooldownState* CooldownSyncCoordinator::GetEncounterStateMut(uint32 instanceId)
{
    // Caller must hold unique lock
    auto it = _encounterStates.find(instanceId);
    if (it != _encounterStates.end())
        return &it->second;
    return nullptr;
}

bool CooldownSyncCoordinator::HasBloodlustSpellAvailable(ObjectGuid botGuid) const
{
    // Caller must hold at least shared lock
    auto it = _cooldowns.find(botGuid);
    if (it == _cooldowns.end())
        return false;

    for (const auto& [spellId, cd] : it->second)
    {
        if (cd.category == CooldownCategory::BLOODLUST && cd.IsAvailable())
            return true;
    }
    return false;
}

bool CooldownSyncCoordinator::HasHealerCDAvailable(ObjectGuid botGuid) const
{
    // Caller must hold at least shared lock
    auto it = _cooldowns.find(botGuid);
    if (it == _cooldowns.end())
        return false;

    for (const auto& [spellId, cd] : it->second)
    {
        if (cd.category == CooldownCategory::HEALER_RAID && cd.IsAvailable())
            return true;
    }
    return false;
}

bool CooldownSyncCoordinator::HasRaidDefensiveAvailable(ObjectGuid botGuid) const
{
    // Caller must hold at least shared lock
    auto it = _cooldowns.find(botGuid);
    if (it == _cooldowns.end())
        return false;

    for (const auto& [spellId, cd] : it->second)
    {
        if (cd.category == CooldownCategory::RAID_DEFENSIVE && cd.IsAvailable())
            return true;
    }
    return false;
}

::std::vector<ObjectGuid> CooldownSyncCoordinator::GetHealerCDProviders(uint32 instanceId) const
{
    // Caller must hold at least shared lock
    ::std::vector<ObjectGuid> providers;

    for (const auto& [botGuid, spellMap] : _cooldowns)
    {
        // Filter to bots in this instance
        auto instIt = _botToInstance.find(botGuid);
        if (instIt != _botToInstance.end() && instIt->second != instanceId)
            continue;

        for (const auto& [spellId, cd] : spellMap)
        {
            if (cd.category == CooldownCategory::HEALER_RAID && cd.IsAvailable())
            {
                providers.push_back(botGuid);
                break; // Only add each bot once
            }
        }
    }

    // Sort by GUID for deterministic rotation order
    std::sort(providers.begin(), providers.end());
    return providers;
}

::std::vector<ObjectGuid> CooldownSyncCoordinator::GetRaidDefensiveProviders(uint32 instanceId) const
{
    // Caller must hold at least shared lock
    ::std::vector<ObjectGuid> providers;

    for (const auto& [botGuid, spellMap] : _cooldowns)
    {
        // Filter to bots in this instance
        auto instIt = _botToInstance.find(botGuid);
        if (instIt != _botToInstance.end() && instIt->second != instanceId)
            continue;

        for (const auto& [spellId, cd] : spellMap)
        {
            if (cd.category == CooldownCategory::RAID_DEFENSIVE && cd.IsAvailable())
            {
                providers.push_back(botGuid);
                break; // Only add each bot once
            }
        }
    }

    // Sort by GUID for deterministic order
    std::sort(providers.begin(), providers.end());
    return providers;
}

::std::vector<ObjectGuid> CooldownSyncCoordinator::GetBloodlustProviders(uint32 instanceId) const
{
    // Caller must hold at least shared lock
    ::std::vector<ObjectGuid> providers;

    for (const auto& [botGuid, spellMap] : _cooldowns)
    {
        // Filter to bots in this instance
        auto instIt = _botToInstance.find(botGuid);
        if (instIt != _botToInstance.end() && instIt->second != instanceId)
            continue;

        for (const auto& [spellId, cd] : spellMap)
        {
            if (cd.category == CooldownCategory::BLOODLUST && cd.IsAvailable())
            {
                providers.push_back(botGuid);
                break;
            }
        }
    }

    // Sort by GUID for deterministic selection (first in list = designated caster)
    std::sort(providers.begin(), providers.end());
    return providers;
}

uint32 CooldownSyncCoordinator::GetServerTimeMs() const
{
    return GameTime::GetGameTimeMS();
}

uint32 CooldownSyncCoordinator::GetDefaultWindowDuration(CooldownWindow window) const
{
    switch (window)
    {
        case CooldownWindow::PULL_BURST:        return _config.pullBurstDurationMs;
        case CooldownWindow::PHASE_TRANSITION:  return _config.phaseTransitionBurstMs;
        case CooldownWindow::EXECUTE_PHASE:     return 0; // Indefinite
        case CooldownWindow::EMERGENCY:         return 0; // Until health recovers
        case CooldownWindow::INTERMISSION_END:  return _config.phaseTransitionBurstMs;
        case CooldownWindow::NONE:
        default:                                return 0;
    }
}

bool CooldownSyncCoordinator::IsHealerCDIntervalElapsed(const EncounterCooldownState& state) const
{
    // The healer CD rotation index advances each time an emergency opens.
    // We use the emergency open time as the reference point for the interval.
    if (state.lastEmergencyTime == 0)
        return true;

    uint32 elapsed = GetServerTimeMs() - state.lastEmergencyTime;
    return elapsed >= _config.healerCDMinIntervalMs;
}

} // namespace Playerbot
