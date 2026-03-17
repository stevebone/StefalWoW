/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Unified Cooldown Management System
 *
 * Eliminates duplicate InitializeCooldowns() methods across all 36 specs
 * Provides centralized cooldown tracking and management
 */

#pragma once

#include "Define.h"
#include <unordered_map>
#include <vector>
#include <initializer_list>

namespace Playerbot
{

// ============================================================================
// COOLDOWN INFO - Tracks a single cooldown
// ============================================================================

struct CooldownInfo
{
    uint32 spellId;
    uint32 baseDuration;   // Base cooldown duration in ms
    uint32 remaining;      // Remaining cooldown in ms
    uint32 chargesMax;     // Maximum charges (1 for non-charge abilities)
    uint32 chargesAvail;   // Available charges
    uint32 chargeRecharge; // Time until next charge in ms

    CooldownInfo()
        : spellId(0)
        , baseDuration(0)
        , remaining(0)
        , chargesMax(1)
        , chargesAvail(1)
        , chargeRecharge(0)
    {}

    CooldownInfo(uint32 id, uint32 duration, uint32 maxCharges = 1)
        : spellId(id)
        , baseDuration(duration)
        , remaining(0)
        , chargesMax(maxCharges)
        , chargesAvail(maxCharges)
        , chargeRecharge(0)
    {}

    [[nodiscard]] bool IsReady() const
    {
        return chargesAvail > 0 || remaining == 0;
    }

    [[nodiscard]] bool HasCharges() const
    {
        return chargesAvail > 0;
    }

    [[nodiscard]] uint32 GetTimeRemaining() const
    {
        return remaining;
    }

    [[nodiscard]] float GetReadyPercent() const
    {
        if (baseDuration == 0)
            return 1.0f;

        if (remaining == 0)
            return 1.0f;

        return 1.0f - (static_cast<float>(remaining) / static_cast<float>(baseDuration));
    }
};

// ============================================================================
// COOLDOWN MANAGER - Centralized cooldown tracking
// ============================================================================

class CooldownManager
{
public:
    CooldownManager() = default;

    // ========================================================================
    // REGISTRATION - Define cooldowns at initialization
    // ========================================================================

    /**
     * Register a single cooldown
     */
    void Register(uint32 spellId, uint32 durationMs, uint32 maxCharges = 1)
    {
        _cooldowns[spellId] = CooldownInfo(spellId, durationMs, maxCharges);
    }

    /**
     * Register multiple cooldowns at once
     * Usage: mgr.RegisterBatch({{SPELL_1, 60000}, {SPELL_2, 120000, 2}});
     */
    void RegisterBatch(::std::initializer_list<::std::tuple<uint32, uint32, uint32>> cooldowns)
    {
        for (const auto& [spellId, duration, charges] : cooldowns)
        {
            Register(spellId, duration, charges);
        }
    }

    /**
     * Convenience method for common cooldown durations
     */
    void RegisterCommon(uint32 spellId, uint32 seconds)
    {
        Register(spellId, seconds * 1000, 1);
    }

    // ========================================================================
    // COOLDOWN MANAGEMENT
    // ========================================================================

    /**
     * Trigger a cooldown (called when spell is cast)
     */
    void Trigger(uint32 spellId)
    {
        auto it = _cooldowns.find(spellId);
        if (it == _cooldowns.end())
            return;

        CooldownInfo& cd = it->second;

        if (cd.chargesMax > 1)
        {
            // Charge-based ability
            if (cd.chargesAvail > 0)
                --cd.chargesAvail;

            // Start recharge timer if not already charging
            if (cd.chargeRecharge == 0 && cd.chargesAvail < cd.chargesMax)
                cd.chargeRecharge = cd.baseDuration;
        }
        else
        {
            // Standard cooldown
            cd.remaining = cd.baseDuration;
        }
    }

    /**
     * Check if ability is ready to use
     */
    [[nodiscard]] bool IsReady(uint32 spellId) const
    {
        auto it = _cooldowns.find(spellId);
        if (it == _cooldowns.end())
            return true; // Not tracked, assume ready

        return it->second.IsReady();
    }

    /**
     * Get remaining cooldown time
     */
    [[nodiscard]] uint32 GetRemaining(uint32 spellId) const
    {
        auto it = _cooldowns.find(spellId);
        return it != _cooldowns.end() ? it->second.remaining : 0;
    }

    /**
     * Get available charges
     */
    [[nodiscard]] uint32 GetCharges(uint32 spellId) const
    {
        auto it = _cooldowns.find(spellId);
        return it != _cooldowns.end() ? it->second.chargesAvail : 0;
    }

    /**
     * Force reset a cooldown (for procs, resets, etc.)
     */
    void Reset(uint32 spellId)
    {
        auto it = _cooldowns.find(spellId);
        if (it == _cooldowns.end())
            return;

        CooldownInfo& cd = it->second;
        cd.remaining = 0;
        cd.chargesAvail = cd.chargesMax;
        cd.chargeRecharge = 0;
    }

    /**
     * Reset all cooldowns
     */
    void ResetAll()
    {
        for (auto& [spellId, cd] : _cooldowns)
        {
            cd.remaining = 0;
            cd.chargesAvail = cd.chargesMax;
            cd.chargeRecharge = 0;
        }
    }

    /**
     * Reduce cooldown (for CDR effects)
     */
    void Reduce(uint32 spellId, uint32 amountMs)
    {
        auto it = _cooldowns.find(spellId);
        if (it == _cooldowns.end())
            return;

        CooldownInfo& cd = it->second;
        cd.remaining = (cd.remaining > amountMs) ? cd.remaining - amountMs : 0;
    }

    /**
     * Update all cooldowns (called each frame with diff in ms)
     */
    void Update(uint32 diff)
    {
        for (auto& [spellId, cd] : _cooldowns)
        {
            // Update standard cooldown
            if (cd.remaining > 0)
            {
                cd.remaining = (cd.remaining > diff) ? cd.remaining - diff : 0;
            }

            // Update charge recharge
            if (cd.chargeRecharge > 0)
            {
                if (cd.chargeRecharge > diff)
                {
                    cd.chargeRecharge -= diff;
                }
                else
                {
                    // Charge recharged
                    cd.chargeRecharge = 0;
                    if (cd.chargesAvail < cd.chargesMax)
                    {
                        ++cd.chargesAvail;

                        // Start next charge recharge if not at max
                        if (cd.chargesAvail < cd.chargesMax)
                            cd.chargeRecharge = cd.baseDuration;
                    }
                }
            }
        }
    }

    /**
     * Get cooldown info for display/debugging
     */
    [[nodiscard]] const CooldownInfo* GetInfo(uint32 spellId) const
    {
        auto it = _cooldowns.find(spellId);
        return it != _cooldowns.end() ? &it->second : nullptr;
    }

    /**
     * Check if cooldown is tracked
     */
    [[nodiscard]] bool IsTracked(uint32 spellId) const
    {
        return _cooldowns.find(spellId) != _cooldowns.end();
    }

    /**
     * Get all cooldowns currently on cooldown
     */
    [[nodiscard]] ::std::vector<uint32> GetActiveCooldowns() const
    {
        ::std::vector<uint32> active;
        for (const auto& [spellId, cd] : _cooldowns)
        {
            if (cd.remaining > 0 || cd.chargesAvail < cd.chargesMax)
                active.push_back(spellId);
        }
        return active;
    }

    /**
     * Get count of abilities ready to use
     */
    [[nodiscard]] uint32 GetReadyCount() const
    {
        uint32 count = 0;
        for (const auto& [spellId, cd] : _cooldowns)
        {
            if (cd.IsReady())
                ++count;
        }
        return count;
    }

    void Clear()
    {
        _cooldowns.clear();
    }

private:
    ::std::unordered_map<uint32, CooldownInfo> _cooldowns;
};

// ============================================================================
// COOLDOWN PRESETS - Common cooldown configurations
// ============================================================================

namespace CooldownPresets
{
    // Offensive cooldowns
    constexpr uint32 BLOODLUST = 600000;      // 10 min
    constexpr uint32 MAJOR_OFFENSIVE = 180000; // 3 min
    constexpr uint32 MINOR_OFFENSIVE = 120000; // 2 min
    constexpr uint32 OFFENSIVE_60 = 60000;     // 1 min
    constexpr uint32 OFFENSIVE_45 = 45000;     // 45 sec
    constexpr uint32 OFFENSIVE_30 = 30000;     // 30 sec

    // Defensive cooldowns
    constexpr uint32 MAJOR_DEFENSIVE = 180000; // 3 min
    constexpr uint32 MINOR_DEFENSIVE = 120000; // 2 min
    constexpr uint32 DEFENSIVE_60 = 60000;     // 1 min
    constexpr uint32 DEFENSIVE_45 = 45000;     // 45 sec
    constexpr uint32 DEFENSIVE_30 = 30000;     // 30 sec

    // Utility cooldowns
    constexpr uint32 INTERRUPT = 15000;        // 15 sec
    constexpr uint32 DISPEL = 8000;            // 8 sec
    constexpr uint32 CC_LONG = 60000;          // 1 min
    constexpr uint32 CC_SHORT = 30000;         // 30 sec

    // Movement cooldowns
    constexpr uint32 MOVEMENT_MAJOR = 120000;  // 2 min
    constexpr uint32 MOVEMENT_MINOR = 60000;   // 1 min
    constexpr uint32 MOVEMENT_SHORT = 30000;   // 30 sec
}

} // namespace Playerbot
