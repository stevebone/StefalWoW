/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Status Effect Tracking System
 *
 * Provides unified tracking for DoTs, HoTs, Buffs, and Debuffs
 * Eliminates hundreds of duplicate tracker implementations across specs
 */

#pragma once

#include "ObjectGuid.h"
#include "Define.h"
#include <unordered_map>
#include <vector>
#include <optional>
#include "GameTime.h"

namespace Playerbot
{

// ============================================================================
// EFFECT INFO - Tracks a single status effect instance
// ============================================================================

struct EffectInfo
{
    uint32 spellId;
    uint32 endTime;      // GameTime::GetGameTimeMS() when effect expires
    uint32 duration;     // Total duration in ms
    uint32 stacks;       // Stack count (1 for non-stacking effects)
    bool active;

    EffectInfo() : spellId(0), endTime(0), duration(0), stacks(1), active(false) {}

    EffectInfo(uint32 id, uint32 dur, uint32 stackCount = 1)
        : spellId(id)
        , endTime(0)
        , duration(dur)
        , stacks(stackCount)
        , active(false)
    {}

    [[nodiscard]] uint32 GetTimeRemaining() const
    {
        if (!active)
            return 0;

        uint32 now = GameTime::GetGameTimeMS();
        return endTime > now ? endTime - now : 0;
    }

    [[nodiscard]] bool IsExpired() const
    {
        return !active || GameTime::GetGameTimeMS() >= endTime;
    }

    [[nodiscard]] bool NeedsRefresh(uint32 pandemicWindow = 5400) const
    {
        // Pandemic window: 30% of duration (5.4s for 18s effects)
        return !active || GetTimeRemaining() < pandemicWindow;
    }

    [[nodiscard]] float GetRemainingPercent() const
    {
        if (!active || duration == 0)
            return 0.0f;

        return static_cast<float>(GetTimeRemaining()) / static_cast<float>(duration);
    }
};

// ============================================================================
// DOT TRACKER - Damage over Time effects on enemies
// ============================================================================

class DotTracker
{
public:
    DotTracker() = default;

    void RegisterDot(uint32 spellId, uint32 baseDuration)
    {
        _dotDefinitions[spellId] = EffectInfo(spellId, baseDuration);
    }

    void ApplyDot(ObjectGuid targetGuid, uint32 spellId, uint32 customDuration = 0, uint32 stacks = 1)
    {
        auto it = _dotDefinitions.find(spellId);
        if (it == _dotDefinitions.end())
            return; // Spell not registered

        uint32 duration = customDuration > 0 ? customDuration : it->second.duration;

        EffectInfo& dot = _activeDots[targetGuid][spellId];
        dot.spellId = spellId;
        dot.duration = duration;
        dot.stacks = stacks;
        dot.active = true;
        dot.endTime = GameTime::GetGameTimeMS() + duration;
    }

    void RemoveDot(ObjectGuid targetGuid, uint32 spellId)
    {
        auto targetIt = _activeDots.find(targetGuid);
        if (targetIt != _activeDots.end())
            targetIt->second.erase(spellId);
    }

    [[nodiscard]] bool IsActive(ObjectGuid targetGuid, uint32 spellId) const
    {
        auto targetIt = _activeDots.find(targetGuid);
        if (targetIt == _activeDots.end())
            return false;

        auto dotIt = targetIt->second.find(spellId);
        if (dotIt == targetIt->second.end())
            return false;

        return dotIt->second.active && !dotIt->second.IsExpired();
    }

    [[nodiscard]] bool NeedsRefresh(ObjectGuid targetGuid, uint32 spellId, uint32 customPandemicWindow = 0) const
    {
        auto targetIt = _activeDots.find(targetGuid);
        if (targetIt == _activeDots.end())
            return true; // Not applied, needs application

        auto dotIt = targetIt->second.find(spellId);
        if (dotIt == targetIt->second.end())
            return true;

        uint32 pandemic = customPandemicWindow > 0 ? customPandemicWindow :
                         static_cast<uint32>(dotIt->second.duration * 0.3f);
        return dotIt->second.NeedsRefresh(pandemic);
    }

    [[nodiscard]] uint32 GetTimeRemaining(ObjectGuid targetGuid, uint32 spellId) const
    {
        auto targetIt = _activeDots.find(targetGuid);
        if (targetIt == _activeDots.end())
            return 0;

        auto dotIt = targetIt->second.find(spellId);
        return dotIt != targetIt->second.end() ? dotIt->second.GetTimeRemaining() : 0;
    }

    [[nodiscard]] uint32 GetActiveDotCount(ObjectGuid targetGuid) const
    {
        auto targetIt = _activeDots.find(targetGuid);
        if (targetIt == _activeDots.end())
            return 0;

        uint32 count = 0;
        for (const auto& [spellId, dot] : targetIt->second)
        {
            if (!dot.IsExpired())
                ++count;
        }
        return count;
    }

    [[nodiscard]] ::std::optional<EffectInfo> GetDotInfo(ObjectGuid targetGuid, uint32 spellId) const
    {
        auto targetIt = _activeDots.find(targetGuid);
        if (targetIt == _activeDots.end())
            return ::std::nullopt;

        auto dotIt = targetIt->second.find(spellId);
        if (dotIt == targetIt->second.end())
            return ::std::nullopt;

        return dotIt->second;
    }

    void Update()
    {
        // Clean up expired DoTs
        for (auto targetIt = _activeDots.begin(); targetIt != _activeDots.end();)
        {
            auto& dots = targetIt->second;

            for (auto dotIt = dots.begin(); dotIt != dots.end();)
            {
                if (dotIt->second.IsExpired())
                {
                    dotIt->second.active = false;
                    dotIt = dots.erase(dotIt);
                }
                else
                {
                    ++dotIt;
                }
            }

            // Remove target entry if no active DoTs
            if (dots.empty())
                targetIt = _activeDots.erase(targetIt);
            else
                ++targetIt;
        }
    }

    void Clear()
    {
        _activeDots.clear();
    }

    void ClearTarget(ObjectGuid targetGuid)
    {
        _activeDots.erase(targetGuid);
    }

private:
    ::std::unordered_map<uint32, EffectInfo> _dotDefinitions;
    ::std::unordered_map<ObjectGuid, ::std::unordered_map<uint32, EffectInfo>> _activeDots;
};

// ============================================================================
// HOT TRACKER - Healing over Time effects on allies
// ============================================================================

class HotTracker
{
public:
    HotTracker() = default;

    void RegisterHot(uint32 spellId, uint32 baseDuration)
    {
        _hotDefinitions[spellId] = EffectInfo(spellId, baseDuration);
    }

    void ApplyHot(ObjectGuid targetGuid, uint32 spellId, uint32 customDuration = 0)
    {
        auto it = _hotDefinitions.find(spellId);
        if (it == _hotDefinitions.end())
            return;

        uint32 duration = customDuration > 0 ? customDuration : it->second.duration;

        EffectInfo& hot = _activeHots[targetGuid][spellId];
        hot.spellId = spellId;
        hot.duration = duration;
        hot.active = true;
        hot.endTime = GameTime::GetGameTimeMS() + duration;
    }

    void RemoveHot(ObjectGuid targetGuid, uint32 spellId)
    {
        auto targetIt = _activeHots.find(targetGuid);
        if (targetIt != _activeHots.end())
            targetIt->second.erase(spellId);
    }

    [[nodiscard]] bool IsActive(ObjectGuid targetGuid, uint32 spellId) const
    {
        auto targetIt = _activeHots.find(targetGuid);
        if (targetIt == _activeHots.end())
            return false;

        auto hotIt = targetIt->second.find(spellId);
        if (hotIt == targetIt->second.end())
            return false;

        return hotIt->second.active && !hotIt->second.IsExpired();
    }

    [[nodiscard]] bool NeedsRefresh(ObjectGuid targetGuid, uint32 spellId, uint32 customPandemicWindow = 0) const
    {
        auto targetIt = _activeHots.find(targetGuid);
        if (targetIt == _activeHots.end())
            return true;

        auto hotIt = targetIt->second.find(spellId);
        if (hotIt == targetIt->second.end())
            return true;

        uint32 pandemic = customPandemicWindow > 0 ? customPandemicWindow :
                         static_cast<uint32>(hotIt->second.duration * 0.3f);
        return hotIt->second.NeedsRefresh(pandemic);
    }

    [[nodiscard]] uint32 GetActiveHotCount(ObjectGuid targetGuid) const
    {
        auto targetIt = _activeHots.find(targetGuid);
        if (targetIt == _activeHots.end())
            return 0;

        uint32 count = 0;
        for (const auto& [spellId, hot] : targetIt->second)
        {
            if (!hot.IsExpired())
                ++count;
        }
        return count;
    }

    [[nodiscard]] uint32 GetTotalActiveHots() const
    {
        uint32 total = 0;
        for (const auto& [guid, hots] : _activeHots)
        {
            for (const auto& [spellId, hot] : hots)
            {
                if (!hot.IsExpired())
                    ++total;
            }
        }
        return total;
    }

    void Update()
    {
        for (auto targetIt = _activeHots.begin(); targetIt != _activeHots.end();)
        {
            auto& hots = targetIt->second;

            for (auto hotIt = hots.begin(); hotIt != hots.end();)
            {
                if (hotIt->second.IsExpired())
                {
                    hotIt->second.active = false;
                    hotIt = hots.erase(hotIt);
                }
                else
                {
                    ++hotIt;
                }
            }

            if (hots.empty())
                targetIt = _activeHots.erase(targetIt);
            else
                ++targetIt;
        }
    }

    void Clear()
    {
        _activeHots.clear();
    }

private:
    ::std::unordered_map<uint32, EffectInfo> _hotDefinitions;
    ::std::unordered_map<ObjectGuid, ::std::unordered_map<uint32, EffectInfo>> _activeHots;
};

// ============================================================================
// BUFF TRACKER - Self and group buff tracking
// ============================================================================

class BuffTracker
{
public:
    BuffTracker() = default;

    void RegisterBuff(uint32 spellId, uint32 baseDuration, uint32 maxStacks = 1)
    {
        _buffDefinitions[spellId] = EffectInfo(spellId, baseDuration, maxStacks);
    }

    void ApplyBuff(uint32 spellId, uint32 customDuration = 0, uint32 stacks = 1)
    {
        auto it = _buffDefinitions.find(spellId);
        if (it == _buffDefinitions.end())
            return;

        uint32 duration = customDuration > 0 ? customDuration : it->second.duration;
        uint32 maxStacks = it->second.stacks;

        EffectInfo& buff = _activeBuffs[spellId];
        buff.spellId = spellId;
        buff.duration = duration;
        buff.stacks = ::std::min(stacks, maxStacks);
        buff.active = true;
        buff.endTime = GameTime::GetGameTimeMS() + duration;
    }

    void RemoveBuff(uint32 spellId)
    {
        _activeBuffs.erase(spellId);
    }

    void AddStack(uint32 spellId)
    {
        auto it = _activeBuffs.find(spellId);
        if (it == _activeBuffs.end())
        {
            ApplyBuff(spellId, 0, 1);
            return;
        }

        auto defIt = _buffDefinitions.find(spellId);
        if (defIt == _buffDefinitions.end())
            return;

        uint32 maxStacks = defIt->second.stacks;
        if (it->second.stacks < maxStacks)
            ++it->second.stacks;

        // Refresh duration
        it->second.endTime = GameTime::GetGameTimeMS() + it->second.duration;
    }

    [[nodiscard]] bool IsActive(uint32 spellId) const
    {
        auto it = _activeBuffs.find(spellId);
        return it != _activeBuffs.end() && it->second.active && !it->second.IsExpired();
    }

    [[nodiscard]] uint32 GetStacks(uint32 spellId) const
    {
        auto it = _activeBuffs.find(spellId);
        return it != _activeBuffs.end() && !it->second.IsExpired() ? it->second.stacks : 0;
    }

    [[nodiscard]] bool NeedsRefresh(uint32 spellId, uint32 customPandemicWindow = 0) const
    {
        auto it = _activeBuffs.find(spellId);
        if (it == _activeBuffs.end())
            return true;

        uint32 pandemic = customPandemicWindow > 0 ? customPandemicWindow :
                         static_cast<uint32>(it->second.duration * 0.3f);
        return it->second.NeedsRefresh(pandemic);
    }

    void Update()
    {
        for (auto it = _activeBuffs.begin(); it != _activeBuffs.end();)
        {
            if (it->second.IsExpired())
            {
                it->second.active = false;
                it = _activeBuffs.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void Clear()
    {
        _activeBuffs.clear();
    }

private:
    ::std::unordered_map<uint32, EffectInfo> _buffDefinitions;
    ::std::unordered_map<uint32, EffectInfo> _activeBuffs;
};

} // namespace Playerbot
