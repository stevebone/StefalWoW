/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "TankSwapCoordinator.h"
#include "Core/Events/CombatEvent.h"
#include "Core/Events/CombatEventRouter.h"
#include "Player.h"
#include "Unit.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include "GameTime.h"

namespace Playerbot {

// ============================================================================
// SINGLETON
// ============================================================================

TankSwapCoordinator& TankSwapCoordinator::Instance()
{
    static TankSwapCoordinator instance;
    return instance;
}

TankSwapCoordinator::TankSwapCoordinator() = default;
TankSwapCoordinator::~TankSwapCoordinator()
{
    Shutdown();
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void TankSwapCoordinator::Initialize()
{
    if (_initialized.load())
        return;

    // Subscribe to CombatEventRouter for aura and death events
    if (CombatEventRouter::Instance().IsInitialized())
    {
        CombatEventRouter::Instance().Subscribe(this);
        _subscribed = true;
        TC_LOG_DEBUG("module.playerbot",
            "TankSwapCoordinator: Subscribed to CombatEventRouter (event-driven mode)");
    }
    else
    {
        TC_LOG_DEBUG("module.playerbot",
            "TankSwapCoordinator: CombatEventRouter not ready, will operate in polling mode");
    }

    _initialized = true;

    TC_LOG_DEBUG("module.playerbot",
        "TankSwapCoordinator: Initialized (defaultThreshold=%u, swapCooldown=%ums, signalTimeout=%ums)",
        static_cast<uint32>(_defaultStackThreshold), _swapCooldownMs, _signalTimeoutMs);
}

void TankSwapCoordinator::Shutdown()
{
    if (!_initialized.load())
        return;

    // Unsubscribe from combat events
    if (_subscribed.load() && CombatEventRouter::Instance().IsInitialized())
    {
        CombatEventRouter::Instance().Unsubscribe(this);
        _subscribed = false;
        TC_LOG_DEBUG("module.playerbot",
            "TankSwapCoordinator: Unsubscribed from CombatEventRouter");
    }

    Reset();
    _initialized = false;

    TC_LOG_DEBUG("module.playerbot", "TankSwapCoordinator: Shutdown complete");
}

void TankSwapCoordinator::Update(uint32 diff)
{
    if (!_initialized.load())
        return;

    _cleanupTimerMs += diff;
    if (_cleanupTimerMs >= CLEANUP_INTERVAL_MS)
    {
        _cleanupTimerMs = 0;
        uint32 nowMs = GameTime::GetGameTimeMS();

        std::unique_lock lock(_mutex);
        CleanupExpiredSignals(nowMs);
    }
}

void TankSwapCoordinator::Reset()
{
    std::unique_lock lock(_mutex);

    _tanks.clear();
    _activeTank.Clear();
    _swapDebuffs.clear();
    _swapDebuffSpellIds.clear();
    _pendingTaunts.clear();
    _lastSwapTimeMs = 0;
    _cleanupTimerMs = 0;
    _totalSwapsCoordinated = 0;
    _totalTauntFailures = 0;

    TC_LOG_DEBUG("module.playerbot", "TankSwapCoordinator: State reset");
}

// ============================================================================
// ICombatEventSubscriber IMPLEMENTATION
// ============================================================================

void TankSwapCoordinator::OnCombatEvent(const CombatEvent& event)
{
    switch (event.type)
    {
        case CombatEventType::AURA_APPLIED:
            HandleAuraApplied(event);
            break;
        case CombatEventType::AURA_STACK_CHANGED:
            HandleAuraStackChanged(event);
            break;
        case CombatEventType::AURA_REMOVED:
            HandleAuraRemoved(event);
            break;
        case CombatEventType::UNIT_DIED:
            HandleUnitDied(event);
            break;
        default:
            break;
    }
}

bool TankSwapCoordinator::ShouldReceiveEvent(const CombatEvent& event) const
{
    // For UNIT_DIED events, check if it's one of our tanks
    if (event.type == CombatEventType::UNIT_DIED)
    {
        std::shared_lock lock(_mutex);
        return _tanks.count(event.target) > 0;
    }

    // For aura events, check if the spell is a registered swap debuff AND target is a tank
    if (event.IsAuraEvent())
    {
        std::shared_lock lock(_mutex);
        if (_swapDebuffSpellIds.count(event.spellId) == 0)
            return false;
        return _tanks.count(event.target) > 0;
    }

    return false;
}

// ============================================================================
// TANK REGISTRATION
// ============================================================================

void TankSwapCoordinator::RegisterTank(ObjectGuid tankGuid, uint32 tauntSpell, bool isActive)
{
    std::unique_lock lock(_mutex);

    TankSwapTankInfo info(tankGuid);
    info.tauntSpellId = tauntSpell;
    info.isActiveTank = isActive;

    _tanks[tankGuid] = info;

    if (isActive)
        _activeTank = tankGuid;

    TC_LOG_DEBUG("module.playerbot",
        "TankSwapCoordinator: Registered tank (tauntSpell=%u, active=%s, totalTanks=%zu)",
        tauntSpell, isActive ? "yes" : "no", _tanks.size());
}

void TankSwapCoordinator::UnregisterTank(ObjectGuid tankGuid)
{
    std::unique_lock lock(_mutex);

    _tanks.erase(tankGuid);
    _pendingTaunts.erase(tankGuid);

    // If the unregistered tank was active, try to assign another
    if (_activeTank == tankGuid)
    {
        _activeTank.Clear();
        for (auto& [guid, info] : _tanks)
        {
            if (info.isAlive)
            {
                _activeTank = guid;
                info.isActiveTank = true;
                TC_LOG_DEBUG("module.playerbot",
                    "TankSwapCoordinator: Active tank unregistered, reassigned to next alive tank");
                break;
            }
        }
    }

    TC_LOG_DEBUG("module.playerbot",
        "TankSwapCoordinator: Unregistered tank (remaining=%zu)", _tanks.size());
}

bool TankSwapCoordinator::IsTankRegistered(ObjectGuid guid) const
{
    std::shared_lock lock(_mutex);
    return _tanks.count(guid) > 0;
}

ObjectGuid TankSwapCoordinator::GetActiveTank() const
{
    std::shared_lock lock(_mutex);
    return _activeTank;
}

void TankSwapCoordinator::SetActiveTank(ObjectGuid tankGuid)
{
    std::unique_lock lock(_mutex);

    // Clear previous active flag
    for (auto& [guid, info] : _tanks)
        info.isActiveTank = false;

    _activeTank = tankGuid;

    auto it = _tanks.find(tankGuid);
    if (it != _tanks.end())
        it->second.isActiveTank = true;

    TC_LOG_DEBUG("module.playerbot", "TankSwapCoordinator: Active tank set manually");
}

uint32 TankSwapCoordinator::GetRegisteredTankCount() const
{
    std::shared_lock lock(_mutex);
    return static_cast<uint32>(_tanks.size());
}

// ============================================================================
// SWAP DEBUFF REGISTRATION
// ============================================================================

void TankSwapCoordinator::RegisterSwapDebuff(const TankSwapDebuffDef& def)
{
    std::unique_lock lock(_mutex);

    _swapDebuffs[def.spellId] = def;
    _swapDebuffSpellIds.insert(def.spellId);

    TC_LOG_DEBUG("module.playerbot",
        "TankSwapCoordinator: Registered swap debuff spellId=%u threshold=%u category=%u desc='%s'",
        def.spellId, static_cast<uint32>(def.stackThreshold),
        static_cast<uint32>(def.category), def.description.c_str());
}

void TankSwapCoordinator::RegisterSwapDebuff(uint32 spellId, uint8 stackThreshold,
                                              const std::string& description)
{
    TankSwapDebuffDef def;
    def.spellId = spellId;
    def.stackThreshold = (stackThreshold > 0) ? stackThreshold : _defaultStackThreshold;
    def.category = TankSwapDebuffCategory::STACKING_DAMAGE_AMP;
    def.description = description.empty()
        ? ("Swap debuff (spell " + std::to_string(spellId) + ")")
        : description;

    RegisterSwapDebuff(def);
}

void TankSwapCoordinator::UnregisterSwapDebuff(uint32 spellId)
{
    std::unique_lock lock(_mutex);

    _swapDebuffs.erase(spellId);
    _swapDebuffSpellIds.erase(spellId);

    // Remove tracked stacks for this debuff from all tanks
    for (auto& [guid, info] : _tanks)
        info.debuffStacks.erase(spellId);

    TC_LOG_DEBUG("module.playerbot",
        "TankSwapCoordinator: Unregistered swap debuff spellId=%u", spellId);
}

void TankSwapCoordinator::ClearSwapDebuffs()
{
    std::unique_lock lock(_mutex);

    _swapDebuffs.clear();
    _swapDebuffSpellIds.clear();

    // Clear all debuff stack tracking
    for (auto& [guid, info] : _tanks)
        info.debuffStacks.clear();

    TC_LOG_DEBUG("module.playerbot", "TankSwapCoordinator: All swap debuffs cleared");
}

bool TankSwapCoordinator::IsSwapDebuff(uint32 spellId) const
{
    std::shared_lock lock(_mutex);
    return _swapDebuffSpellIds.count(spellId) > 0;
}

uint32 TankSwapCoordinator::GetSwapDebuffCount() const
{
    std::shared_lock lock(_mutex);
    return static_cast<uint32>(_swapDebuffs.size());
}

// ============================================================================
// SWAP THRESHOLD CONFIGURATION
// ============================================================================

void TankSwapCoordinator::SetDefaultStackThreshold(uint8 threshold)
{
    std::unique_lock lock(_mutex);
    _defaultStackThreshold = (threshold > 0) ? threshold : 3;

    TC_LOG_DEBUG("module.playerbot",
        "TankSwapCoordinator: Default stack threshold set to %u",
        static_cast<uint32>(_defaultStackThreshold));
}

uint8 TankSwapCoordinator::GetDefaultStackThreshold() const
{
    std::shared_lock lock(_mutex);
    return _defaultStackThreshold;
}

void TankSwapCoordinator::SetSwapCooldown(uint32 cooldownMs)
{
    std::unique_lock lock(_mutex);
    _swapCooldownMs = cooldownMs;
}

void TankSwapCoordinator::SetSignalTimeout(uint32 timeoutMs)
{
    std::unique_lock lock(_mutex);
    _signalTimeoutMs = timeoutMs;
}

// ============================================================================
// SWAP QUERY INTERFACE
// ============================================================================

bool TankSwapCoordinator::ShouldTaunt(ObjectGuid tankGuid) const
{
    std::shared_lock lock(_mutex);

    auto it = _pendingTaunts.find(tankGuid);
    if (it == _pendingTaunts.end())
        return false;

    return !it->second.consumed;
}

ObjectGuid TankSwapCoordinator::GetTauntTarget(ObjectGuid tankGuid) const
{
    std::shared_lock lock(_mutex);

    auto it = _pendingTaunts.find(tankGuid);
    if (it == _pendingTaunts.end() || it->second.consumed)
        return ObjectGuid::Empty;

    return it->second.tauntTargetGuid;
}

void TankSwapCoordinator::OnTauntExecuted(ObjectGuid tankGuid)
{
    std::unique_lock lock(_mutex);

    auto signalIt = _pendingTaunts.find(tankGuid);
    if (signalIt == _pendingTaunts.end() || signalIt->second.consumed)
        return;

    PendingTauntSignal& signal = signalIt->second;
    signal.consumed = true;

    // The off-tank that just taunted becomes the new active tank
    ObjectGuid previousActive = _activeTank;

    // Update active tank flags
    for (auto& [guid, info] : _tanks)
        info.isActiveTank = false;

    _activeTank = tankGuid;
    auto tankIt = _tanks.find(tankGuid);
    if (tankIt != _tanks.end())
    {
        tankIt->second.isActiveTank = true;
        tankIt->second.lastTauntTimeMs = GameTime::GetGameTimeMS();
    }

    _lastSwapTimeMs = GameTime::GetGameTimeMS();
    _totalSwapsCoordinated.fetch_add(1, std::memory_order_relaxed);

    // Remove the consumed signal
    _pendingTaunts.erase(signalIt);

    TC_LOG_DEBUG("module.playerbot",
        "TankSwapCoordinator: Tank swap completed (triggerSpell=%u, triggerStacks=%u, totalSwaps=%u)",
        signal.triggerSpellId, static_cast<uint32>(signal.triggerStacks),
        _totalSwapsCoordinated.load(std::memory_order_relaxed));
}

void TankSwapCoordinator::OnTauntFailed(ObjectGuid tankGuid)
{
    std::unique_lock lock(_mutex);

    _totalTauntFailures.fetch_add(1, std::memory_order_relaxed);

    auto signalIt = _pendingTaunts.find(tankGuid);
    if (signalIt == _pendingTaunts.end())
        return;

    PendingTauntSignal failedSignal = signalIt->second;
    _pendingTaunts.erase(signalIt);

    TC_LOG_DEBUG("module.playerbot",
        "TankSwapCoordinator: Taunt failed, attempting reassignment (totalFailures=%u)",
        _totalTauntFailures.load(std::memory_order_relaxed));

    // Try to find another off-tank to take over
    // Exclude both the failed tank and the current active tank
    ObjectGuid alternateOffTank;
    uint8 lowestStacks = 255;

    for (const auto& [guid, info] : _tanks)
    {
        if (guid == tankGuid || guid == _activeTank || !info.isAlive)
            continue;

        // Check taunt cooldown
        uint32 nowMs = GameTime::GetGameTimeMS();
        if (info.lastTauntTimeMs > 0 && (nowMs - info.lastTauntTimeMs) < info.tauntCooldownMs)
            continue;

        // Prefer tank with lowest swap debuff stacks
        uint8 stacks = 0;
        for (const auto& [spellId, count] : info.debuffStacks)
        {
            if (_swapDebuffSpellIds.count(spellId) > 0)
                stacks = std::max(stacks, count);
        }

        if (stacks < lowestStacks)
        {
            lowestStacks = stacks;
            alternateOffTank = guid;
        }
    }

    if (!alternateOffTank.IsEmpty())
    {
        PendingTauntSignal newSignal;
        newSignal.offTankGuid = alternateOffTank;
        newSignal.tauntTargetGuid = failedSignal.tauntTargetGuid;
        newSignal.triggerSpellId = failedSignal.triggerSpellId;
        newSignal.triggerStacks = failedSignal.triggerStacks;
        newSignal.createdTimeMs = GameTime::GetGameTimeMS();
        newSignal.expirationTimeMs = newSignal.createdTimeMs + _signalTimeoutMs;
        newSignal.consumed = false;

        _pendingTaunts[alternateOffTank] = newSignal;

        TC_LOG_DEBUG("module.playerbot",
            "TankSwapCoordinator: Reassigned taunt signal to alternate off-tank");
    }
    else
    {
        TC_LOG_DEBUG("module.playerbot",
            "TankSwapCoordinator: No alternate off-tank available for reassignment");
    }
}

// ============================================================================
// DEBUFF STACK QUERIES
// ============================================================================

uint8 TankSwapCoordinator::GetDebuffStacks(ObjectGuid tankGuid, uint32 spellId) const
{
    std::shared_lock lock(_mutex);

    auto tankIt = _tanks.find(tankGuid);
    if (tankIt == _tanks.end())
        return 0;

    auto stackIt = tankIt->second.debuffStacks.find(spellId);
    if (stackIt == tankIt->second.debuffStacks.end())
        return 0;

    return stackIt->second;
}

uint8 TankSwapCoordinator::GetHighestSwapDebuffStacks(ObjectGuid tankGuid) const
{
    std::shared_lock lock(_mutex);

    auto tankIt = _tanks.find(tankGuid);
    if (tankIt == _tanks.end())
        return 0;

    uint8 highest = 0;
    for (const auto& [spellId, stacks] : tankIt->second.debuffStacks)
    {
        if (_swapDebuffSpellIds.count(spellId) > 0)
            highest = std::max(highest, stacks);
    }

    return highest;
}

bool TankSwapCoordinator::IsSwapImminent(ObjectGuid tankGuid) const
{
    std::shared_lock lock(_mutex);

    auto tankIt = _tanks.find(tankGuid);
    if (tankIt == _tanks.end())
        return false;

    for (const auto& [spellId, stacks] : tankIt->second.debuffStacks)
    {
        auto defIt = _swapDebuffs.find(spellId);
        if (defIt == _swapDebuffs.end())
            continue;

        uint8 threshold = defIt->second.stackThreshold;
        if (threshold > 0 && stacks >= (threshold - 1))
            return true;
    }

    return false;
}

// ============================================================================
// KNOWN WOW 12.0 DEBUFF PRESETS
// ============================================================================

void TankSwapCoordinator::LoadKnownDebuffPresets()
{
    // WoW 12.0 (The War Within / Midnight) common tank-swap debuff patterns.
    // These are generic category-based patterns that match common encounter designs.
    // Encounter-specific scripts should register exact spell IDs for their bosses.
    //
    // The presets below use placeholder spell IDs since actual 12.0 encounter spell IDs
    // depend on the specific raid tier. They serve as templates showing the pattern
    // categories and typical thresholds used in modern WoW encounter design.
    //
    // Category: STACKING_DAMAGE_AMP (most common)
    //   - Pattern: Boss melee applies stacking debuff increasing damage taken
    //   - Typical threshold: 2-4 stacks
    //   - Examples: "Overwhelming Power", "Decaying Armor", "Searing Wound"
    //
    // Category: STACKING_DOT
    //   - Pattern: Boss applies a DoT that stacks, dealing more damage per stack
    //   - Typical threshold: 3-5 stacks
    //   - Examples: "Burning Agony", "Festering Wound", "Shadow Burns"
    //
    // Category: STACKING_VULNERABILITY
    //   - Pattern: Debuff increases damage taken from a specific school
    //   - Typical threshold: 2-3 stacks
    //   - Examples: "Shadow Vulnerability", "Fire Weakness"
    //
    // Category: TIMED_DEBUFF
    //   - Pattern: Debuff applied once, swap when it's active (no stacking)
    //   - swapOnApplication = true for these
    //   - Examples: "Mark of Death", "Doom Brand"
    //
    // Category: FRONTAL_CLEAVE
    //   - Pattern: Boss frontal cone applies debuff; tanks alternate soaking
    //   - Typical threshold: 1-2 stacks
    //   - Examples: "Cleaving Strike", "Devastation"
    //
    // NOTE: Actual spell IDs must be registered per-encounter by the encounter
    //       manager or raid scripts. These presets document the design patterns
    //       and their typical configurations.

    TC_LOG_DEBUG("module.playerbot",
        "TankSwapCoordinator: Known debuff preset categories loaded "
        "(register actual spell IDs per encounter via RegisterSwapDebuff)");
}

// ============================================================================
// STATISTICS / DEBUG
// ============================================================================

uint32 TankSwapCoordinator::GetTotalSwapsCoordinated() const
{
    return _totalSwapsCoordinated.load(std::memory_order_relaxed);
}

uint32 TankSwapCoordinator::GetTotalTauntFailures() const
{
    return _totalTauntFailures.load(std::memory_order_relaxed);
}

bool TankSwapCoordinator::IsActive() const
{
    return _initialized.load() && _subscribed.load();
}

// ============================================================================
// EVENT HANDLERS (Private)
// ============================================================================

void TankSwapCoordinator::HandleAuraApplied(const CombatEvent& event)
{
    std::unique_lock lock(_mutex);

    auto tankIt = _tanks.find(event.target);
    if (tankIt == _tanks.end())
        return;

    auto defIt = _swapDebuffs.find(event.spellId);
    if (defIt == _swapDebuffs.end())
        return;

    const TankSwapDebuffDef& debuffDef = defIt->second;

    // Set initial stack count from event data, defaulting to 1 on first application
    uint8 stacks = (event.auraStacks > 0) ? event.auraStacks : 1;
    tankIt->second.debuffStacks[event.spellId] = stacks;

    TC_LOG_DEBUG("module.playerbot",
        "TankSwapCoordinator: Swap debuff applied on tank (spell=%u, stacks=%u, desc='%s')",
        event.spellId, static_cast<uint32>(stacks), debuffDef.description.c_str());

    // Check for swapOnApplication debuffs (TIMED_DEBUFF category typically)
    if (debuffDef.swapOnApplication)
    {
        TC_LOG_DEBUG("module.playerbot",
            "TankSwapCoordinator: Swap-on-application debuff detected, triggering swap");
        // Unlock before calling EvaluateSwapCondition which needs unique_lock
        // (we already hold it, so call directly)
        EvaluateSwapCondition(event.target, event.spellId, stacks);
        return;
    }

    // Evaluate if threshold reached
    EvaluateSwapCondition(event.target, event.spellId, stacks);
}

void TankSwapCoordinator::HandleAuraStackChanged(const CombatEvent& event)
{
    std::unique_lock lock(_mutex);

    auto tankIt = _tanks.find(event.target);
    if (tankIt == _tanks.end())
        return;

    if (_swapDebuffSpellIds.count(event.spellId) == 0)
        return;

    // Update stack count from event
    uint8 stacks = event.auraStacks;
    tankIt->second.debuffStacks[event.spellId] = stacks;

    TC_LOG_DEBUG("module.playerbot",
        "TankSwapCoordinator: Swap debuff stacks changed on tank (spell=%u, stacks=%u)",
        event.spellId, static_cast<uint32>(stacks));

    EvaluateSwapCondition(event.target, event.spellId, stacks);
}

void TankSwapCoordinator::HandleAuraRemoved(const CombatEvent& event)
{
    std::unique_lock lock(_mutex);

    auto tankIt = _tanks.find(event.target);
    if (tankIt == _tanks.end())
        return;

    if (_swapDebuffSpellIds.count(event.spellId) == 0)
        return;

    tankIt->second.debuffStacks.erase(event.spellId);

    TC_LOG_DEBUG("module.playerbot",
        "TankSwapCoordinator: Swap debuff removed from tank (spell=%u)",
        event.spellId);
}

void TankSwapCoordinator::HandleUnitDied(const CombatEvent& event)
{
    std::unique_lock lock(_mutex);

    auto tankIt = _tanks.find(event.target);
    if (tankIt == _tanks.end())
        return;

    tankIt->second.isAlive = false;
    tankIt->second.debuffStacks.clear();

    TC_LOG_DEBUG("module.playerbot",
        "TankSwapCoordinator: Tank died");

    // If the dead tank was active, emergency reassignment
    if (event.target == _activeTank)
    {
        // Clear any pending taunts for the dead tank
        _pendingTaunts.erase(event.target);

        // Find any alive tank to become active
        ObjectGuid newActive;
        for (auto& [guid, info] : _tanks)
        {
            if (guid != event.target && info.isAlive)
            {
                newActive = guid;
                break;
            }
        }

        if (!newActive.IsEmpty())
        {
            for (auto& [guid, info] : _tanks)
                info.isActiveTank = false;

            _activeTank = newActive;
            _tanks[newActive].isActiveTank = true;

            TC_LOG_DEBUG("module.playerbot",
                "TankSwapCoordinator: Active tank died, emergency reassignment to next alive tank");

            // Create an emergency taunt signal for the new active tank
            ObjectGuid bossGuid = ResolveBossTarget(newActive);
            if (!bossGuid.IsEmpty())
            {
                PendingTauntSignal signal;
                signal.offTankGuid = newActive;
                signal.tauntTargetGuid = bossGuid;
                signal.triggerSpellId = 0;  // Emergency, no specific debuff
                signal.triggerStacks = 0;
                signal.createdTimeMs = GameTime::GetGameTimeMS();
                signal.expirationTimeMs = signal.createdTimeMs + _signalTimeoutMs;
                signal.consumed = false;

                _pendingTaunts[newActive] = signal;

                TC_LOG_DEBUG("module.playerbot",
                    "TankSwapCoordinator: Emergency taunt signal created for replacement tank");
            }
        }
        else
        {
            _activeTank.Clear();
            TC_LOG_DEBUG("module.playerbot",
                "TankSwapCoordinator: Active tank died, no replacement available");
        }
    }
}

// ============================================================================
// INTERNAL SWAP LOGIC (Private -- must be called with _mutex held as unique_lock)
// ============================================================================

void TankSwapCoordinator::EvaluateSwapCondition(ObjectGuid tankGuid, uint32 spellId, uint8 stacks)
{
    // This method is always called while _mutex is already held as unique_lock.
    // Do NOT acquire _mutex here.

    // Only the active tank's debuff stacks trigger a swap
    if (tankGuid != _activeTank)
        return;

    auto defIt = _swapDebuffs.find(spellId);
    if (defIt == _swapDebuffs.end())
        return;

    const TankSwapDebuffDef& debuffDef = defIt->second;

    // Check if swapOnApplication was already handled in HandleAuraApplied
    // For stack-based debuffs, check threshold
    bool shouldSwap = false;

    if (debuffDef.swapOnApplication)
    {
        shouldSwap = true;
    }
    else if (debuffDef.stackThreshold > 0 && stacks >= debuffDef.stackThreshold)
    {
        shouldSwap = true;
    }

    if (!shouldSwap)
        return;

    // Enforce swap cooldown to prevent rapid ping-ponging
    uint32 nowMs = GameTime::GetGameTimeMS();
    if (_lastSwapTimeMs > 0 && (nowMs - _lastSwapTimeMs) < _swapCooldownMs)
    {
        TC_LOG_DEBUG("module.playerbot",
            "TankSwapCoordinator: Swap threshold reached but cooldown active (remaining=%ums)",
            _swapCooldownMs - (nowMs - _lastSwapTimeMs));
        return;
    }

    // Check if there's already a pending signal for any off-tank
    for (const auto& [guid, signal] : _pendingTaunts)
    {
        if (!signal.consumed)
        {
            TC_LOG_DEBUG("module.playerbot",
                "TankSwapCoordinator: Swap threshold reached but pending signal already exists");
            return;
        }
    }

    // Resolve boss target from the active tank
    ObjectGuid bossGuid = ResolveBossTarget(_activeTank);

    TC_LOG_DEBUG("module.playerbot",
        "TankSwapCoordinator: Swap threshold reached (spell=%u, stacks=%u/%u), creating taunt signal",
        spellId, static_cast<uint32>(stacks), static_cast<uint32>(debuffDef.stackThreshold));

    CreateTauntSignal(_activeTank, bossGuid, spellId, stacks);
}

bool TankSwapCoordinator::CreateTauntSignal(ObjectGuid currentActiveTank, ObjectGuid bossGuid,
                                             uint32 triggerSpellId, uint8 triggerStacks)
{
    // Must be called with _mutex held as unique_lock.

    ObjectGuid offTank = FindBestOffTank(currentActiveTank);
    if (offTank.IsEmpty())
    {
        TC_LOG_DEBUG("module.playerbot",
            "TankSwapCoordinator: Cannot create taunt signal - no available off-tank");
        return false;
    }

    uint32 nowMs = GameTime::GetGameTimeMS();

    PendingTauntSignal signal;
    signal.offTankGuid = offTank;
    signal.tauntTargetGuid = bossGuid;
    signal.triggerSpellId = triggerSpellId;
    signal.triggerStacks = triggerStacks;
    signal.createdTimeMs = nowMs;
    signal.expirationTimeMs = nowMs + _signalTimeoutMs;
    signal.consumed = false;

    _pendingTaunts[offTank] = signal;

    TC_LOG_DEBUG("module.playerbot",
        "TankSwapCoordinator: Taunt signal created for off-tank (trigger=%u, stacks=%u, timeout=%ums)",
        triggerSpellId, static_cast<uint32>(triggerStacks), _signalTimeoutMs);

    return true;
}

ObjectGuid TankSwapCoordinator::FindBestOffTank(ObjectGuid excludeTank) const
{
    // Must be called with _mutex held (shared or unique).

    ObjectGuid bestTank;
    uint8 lowestStacks = 255;
    uint32 nowMs = GameTime::GetGameTimeMS();

    for (const auto& [guid, info] : _tanks)
    {
        // Skip the tank being excluded (the one that needs to be relieved)
        if (guid == excludeTank)
            continue;

        // Must be alive
        if (!info.isAlive)
            continue;

        // Verify the player is actually in world (non-null check)
        Player* player = ObjectAccessor::FindPlayer(guid);
        if (!player || !player->IsInWorld() || !player->IsAlive())
            continue;

        // Check taunt cooldown
        if (info.lastTauntTimeMs > 0 && (nowMs - info.lastTauntTimeMs) < info.tauntCooldownMs)
            continue;

        // Prefer the tank with the lowest swap debuff stacks
        uint8 maxStacks = 0;
        for (const auto& [spellId, stacks] : info.debuffStacks)
        {
            if (_swapDebuffSpellIds.count(spellId) > 0)
                maxStacks = std::max(maxStacks, stacks);
        }

        if (maxStacks < lowestStacks)
        {
            lowestStacks = maxStacks;
            bestTank = guid;
        }
    }

    return bestTank;
}

void TankSwapCoordinator::CleanupExpiredSignals(uint32 nowMs)
{
    // Must be called with _mutex held as unique_lock.

    auto it = _pendingTaunts.begin();
    while (it != _pendingTaunts.end())
    {
        if (it->second.consumed || nowMs >= it->second.expirationTimeMs)
        {
            if (!it->second.consumed && nowMs >= it->second.expirationTimeMs)
            {
                TC_LOG_DEBUG("module.playerbot",
                    "TankSwapCoordinator: Pending taunt signal expired (was pending for %ums)",
                    nowMs - it->second.createdTimeMs);
            }
            it = _pendingTaunts.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Update alive status of registered tanks
    for (auto& [guid, info] : _tanks)
    {
        Player* player = ObjectAccessor::FindPlayer(guid);
        info.isAlive = (player && player->IsInWorld() && player->IsAlive());
    }
}

ObjectGuid TankSwapCoordinator::ResolveBossTarget(ObjectGuid activeTankGuid) const
{
    // Must be called with _mutex held (shared or unique).

    Player* tank = ObjectAccessor::FindPlayer(activeTankGuid);
    if (!tank || !tank->IsInWorld())
        return ObjectGuid::Empty;

    // The active tank's current target is presumed to be the boss
    Unit* target = tank->GetVictim();
    if (target && target->IsAlive())
        return target->GetGUID();

    // Fallback: check selected target
    ObjectGuid selection = tank->GetTarget();
    if (!selection.IsEmpty())
        return selection;

    return ObjectGuid::Empty;
}

} // namespace Playerbot
