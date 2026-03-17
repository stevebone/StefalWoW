/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "MajorCooldownTracker.h"
#include "CooldownEventBus.h"
#include "Log.h"

namespace Playerbot
{

void MajorCooldownTracker::Initialize()
{
    if (_initialized)
        return;

    // Subscribe to cooldown events via callback (not BotAI)
    _callbackId = CooldownEventBus::instance()->SubscribeCallback(
        [this](CooldownEvent const& event) {
            HandleCooldownEvent(event);
        },
        {
            CooldownEventType::SPELL_COOLDOWN_START,
            CooldownEventType::SPELL_COOLDOWN_CLEAR,
            CooldownEventType::SPELL_COOLDOWNS_CLEAR_ALL
        }
    );

    _initialized = true;
    TC_LOG_INFO("playerbot.cooldown", "MajorCooldownTracker initialized, callback ID: {}", _callbackId);
}

void MajorCooldownTracker::Shutdown()
{
    if (!_initialized)
        return;

    if (_callbackId > 0)
    {
        CooldownEventBus::instance()->UnsubscribeCallback(_callbackId);
        _callbackId = 0;
    }

    std::lock_guard lock(_mutex);
    _trackedCooldowns.clear();
    _initialized = false;

    TC_LOG_INFO("playerbot.cooldown", "MajorCooldownTracker shutdown");
}

void MajorCooldownTracker::HandleCooldownEvent(CooldownEvent const& event)
{
    switch (event.type)
    {
        case CooldownEventType::SPELL_COOLDOWN_START:
            HandleCooldownStart(event);
            break;
        case CooldownEventType::SPELL_COOLDOWN_CLEAR:
            HandleCooldownClear(event);
            break;
        case CooldownEventType::SPELL_COOLDOWNS_CLEAR_ALL:
            // Clear all cooldowns for this caster
            ClearBotCooldowns(event.casterGuid);
            break;
        default:
            break;
    }
}

void MajorCooldownTracker::HandleCooldownStart(CooldownEvent const& event)
{
    // Check if this spell is a major cooldown
    MajorCooldownInfo const* cdInfo = MajorCooldownDatabase::instance()->GetCooldownInfo(event.spellId);
    if (!cdInfo)
        return;  // Not a major cooldown, ignore

    // Track the cooldown
    {
        std::lock_guard lock(_mutex);

        TrackedMajorCooldown tracked;
        tracked.spellId = event.spellId;
        tracked.tier = cdInfo->tier;
        tracked.availableAt = std::chrono::steady_clock::now() +
                              std::chrono::milliseconds(event.cooldownMs > 0 ? event.cooldownMs : cdInfo->baseCooldownMs);
        tracked.isOnCooldown = true;

        _trackedCooldowns[event.casterGuid][event.spellId] = tracked;
    }

    // Publish MAJOR_CD_USED event
    PublishMajorCDUsed(event.casterGuid, event.spellId, cdInfo->tier,
                       event.cooldownMs > 0 ? event.cooldownMs : cdInfo->baseCooldownMs);

    TC_LOG_DEBUG("playerbot.cooldown", "Major CD used: {} ({}) by {}, CD: {}ms",
        cdInfo->name, event.spellId, event.casterGuid.ToString(),
        event.cooldownMs > 0 ? event.cooldownMs : cdInfo->baseCooldownMs);
}

void MajorCooldownTracker::HandleCooldownClear(CooldownEvent const& event)
{
    // Check if this spell is a major cooldown
    MajorCooldownInfo const* cdInfo = MajorCooldownDatabase::instance()->GetCooldownInfo(event.spellId);
    if (!cdInfo)
        return;  // Not a major cooldown, ignore

    // Check if we were tracking this
    {
        std::lock_guard lock(_mutex);

        auto botIt = _trackedCooldowns.find(event.casterGuid);
        if (botIt == _trackedCooldowns.end())
            return;

        auto spellIt = botIt->second.find(event.spellId);
        if (spellIt == botIt->second.end())
            return;

        // Update state
        spellIt->second.isOnCooldown = false;
        spellIt->second.availableAt = std::chrono::steady_clock::now();
    }

    // Publish MAJOR_CD_AVAILABLE event
    PublishMajorCDAvailable(event.casterGuid, event.spellId, cdInfo->tier);

    TC_LOG_DEBUG("playerbot.cooldown", "Major CD available: {} ({}) for {}",
        cdInfo->name, event.spellId, event.casterGuid.ToString());
}

void MajorCooldownTracker::PublishMajorCDUsed(ObjectGuid caster, uint32 spellId, MajorCooldownTier tier, uint32 cooldownMs)
{
    CooldownEvent event = CooldownEvent::MajorCDUsed(caster, spellId, tier, cooldownMs);
    CooldownEventBus::instance()->PublishEvent(event);
    _majorCDUsedCount++;
}

void MajorCooldownTracker::PublishMajorCDAvailable(ObjectGuid caster, uint32 spellId, MajorCooldownTier tier)
{
    CooldownEvent event = CooldownEvent::MajorCDAvailable(caster, spellId, tier);
    CooldownEventBus::instance()->PublishEvent(event);
    _majorCDAvailableCount++;
}

TrackedMajorCooldown const* MajorCooldownTracker::GetCooldownState(ObjectGuid botGuid, uint32 spellId) const
{
    std::lock_guard lock(_mutex);

    auto botIt = _trackedCooldowns.find(botGuid);
    if (botIt == _trackedCooldowns.end())
        return nullptr;

    auto spellIt = botIt->second.find(spellId);
    if (spellIt == botIt->second.end())
        return nullptr;

    return &spellIt->second;
}

bool MajorCooldownTracker::IsCooldownAvailable(ObjectGuid botGuid, uint32 spellId) const
{
    std::lock_guard lock(_mutex);

    auto botIt = _trackedCooldowns.find(botGuid);
    if (botIt == _trackedCooldowns.end())
        return true;  // Not tracked = assumed available

    auto spellIt = botIt->second.find(spellId);
    if (spellIt == botIt->second.end())
        return true;  // Not tracked = assumed available

    // Check if cooldown has expired
    if (spellIt->second.isOnCooldown)
    {
        return std::chrono::steady_clock::now() >= spellIt->second.availableAt;
    }

    return true;
}

std::vector<ObjectGuid> MajorCooldownTracker::GetBotsWithCDAvailable(uint32 spellId) const
{
    std::vector<ObjectGuid> result;
    std::lock_guard lock(_mutex);

    for (auto const& [botGuid, cooldowns] : _trackedCooldowns)
    {
        auto spellIt = cooldowns.find(spellId);
        if (spellIt == cooldowns.end())
        {
            // Not tracked = assumed available (bot may have the spell)
            continue;
        }

        if (!spellIt->second.isOnCooldown ||
            std::chrono::steady_clock::now() >= spellIt->second.availableAt)
        {
            result.push_back(botGuid);
        }
    }

    return result;
}

std::vector<std::tuple<ObjectGuid, uint32, MajorCooldownTier>> MajorCooldownTracker::GetAvailableExternalCDs() const
{
    std::vector<std::tuple<ObjectGuid, uint32, MajorCooldownTier>> result;
    std::lock_guard lock(_mutex);

    auto now = std::chrono::steady_clock::now();

    for (auto const& [botGuid, cooldowns] : _trackedCooldowns)
    {
        for (auto const& [spellId, tracked] : cooldowns)
        {
            // Only external CDs
            if (tracked.tier != MajorCooldownTier::EXTERNAL_MAJOR &&
                tracked.tier != MajorCooldownTier::EXTERNAL_MODERATE)
                continue;

            // Check if available
            if (!tracked.isOnCooldown || now >= tracked.availableAt)
            {
                result.emplace_back(botGuid, spellId, tracked.tier);
            }
        }
    }

    return result;
}

std::vector<std::tuple<ObjectGuid, uint32, MajorCooldownTier>> MajorCooldownTracker::GetAvailableRaidCDs() const
{
    std::vector<std::tuple<ObjectGuid, uint32, MajorCooldownTier>> result;
    std::lock_guard lock(_mutex);

    auto now = std::chrono::steady_clock::now();

    for (auto const& [botGuid, cooldowns] : _trackedCooldowns)
    {
        for (auto const& [spellId, tracked] : cooldowns)
        {
            // Only raid-wide CDs
            if (tracked.tier != MajorCooldownTier::RAID_OFFENSIVE &&
                tracked.tier != MajorCooldownTier::RAID_DEFENSIVE)
                continue;

            // Check if available
            if (!tracked.isOnCooldown || now >= tracked.availableAt)
            {
                result.emplace_back(botGuid, spellId, tracked.tier);
            }
        }
    }

    return result;
}

void MajorCooldownTracker::ClearBotCooldowns(ObjectGuid botGuid)
{
    std::lock_guard lock(_mutex);
    _trackedCooldowns.erase(botGuid);
    TC_LOG_DEBUG("playerbot.cooldown", "Cleared tracked cooldowns for bot {}", botGuid.ToString());
}

MajorCooldownTracker::Statistics MajorCooldownTracker::GetStatistics() const
{
    std::lock_guard lock(_mutex);

    Statistics stats;
    stats.trackedBots = static_cast<uint32>(_trackedCooldowns.size());
    stats.trackedCooldowns = 0;
    for (auto const& [_, cooldowns] : _trackedCooldowns)
    {
        stats.trackedCooldowns += static_cast<uint32>(cooldowns.size());
    }
    stats.majorCDUsedEventsPublished = _majorCDUsedCount.load();
    stats.majorCDAvailableEventsPublished = _majorCDAvailableCount.load();

    return stats;
}

} // namespace Playerbot
