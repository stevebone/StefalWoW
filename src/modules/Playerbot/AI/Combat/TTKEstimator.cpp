/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Time-to-Kill (TTK) Estimator implementation.
 * Tracks rolling 5s DPS window per target using health sampling.
 */

#include "TTKEstimator.h"
#include "Player.h"
#include "Unit.h"
#include "Group.h"
#include "Map.h"
#include "GameTime.h"
#include "ObjectAccessor.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "Log.h"
#include <algorithm>
#include <cmath>

namespace Playerbot
{

TTKEstimator::TTKEstimator(Player* bot)
    : _bot(bot)
    , _sampleTimer(0)
    , _pruneTimer(0)
{
}

void TTKEstimator::Update(uint32 diff)
{
    if (!_bot || !_bot->IsInWorld() || !_bot->IsAlive() || !_bot->IsInCombat())
        return;

    _sampleTimer += diff;
    _pruneTimer += diff;

    // Sample enemy health at regular intervals
    if (_sampleTimer >= SAMPLE_INTERVAL_MS)
    {
        SampleTargetHealth();
        _sampleTimer = 0;
    }

    // Prune stale targets every 5 seconds
    if (_pruneTimer >= 5000)
    {
        PruneStaleTargets();
        _pruneTimer = 0;
    }
}

void TTKEstimator::SampleTargetHealth()
{
    std::lock_guard<OrderedMutex<LockOrder::BOT_AI_STATE>> lock(_mutex);

    uint32 now = GameTime::GetGameTimeMS();

    // Get all hostile units the bot or its group are fighting
    // We check the bot's threat list and nearby hostiles
    std::vector<Unit*> targets;

    // Check bot's current target
    if (Unit* victim = _bot->GetVictim())
        targets.push_back(victim);

    // Check group members' targets for group DPS tracking
    if (Group* group = _bot->GetGroup())
    {
        for (Group::MemberSlot const& slot : group->GetMemberSlots())
        {
            Player* member = ObjectAccessor::FindPlayer(slot.guid);
            if (!member || !member->IsInWorld() || !member->IsAlive())
                continue;

            if (Unit* memberTarget = member->GetVictim())
            {
                // Avoid duplicates
                bool found = false;
                for (Unit* existing : targets)
                {
                    if (existing->GetGUID() == memberTarget->GetGUID())
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                    targets.push_back(memberTarget);
            }
        }
    }

    // Sample health for each target
    for (Unit* target : targets)
    {
        if (!target || !target->IsAlive())
            continue;

        ObjectGuid guid = target->GetGUID();
        uint64 currentHealth = target->GetHealth();
        uint64 maxHealth = target->GetMaxHealth();

        auto& data = _targetData[guid];

        // Initialize if first sight
        if (data.totalHealthAtFirstSight == 0)
        {
            data.targetGuid = guid;
            data.totalHealthAtFirstSight = maxHealth;
            data.lastKnownHealth = currentHealth;
            data.lastUpdated = now;
            data.noHealthChangeStart = now;
            continue; // Need at least 2 samples to calculate DPS
        }

        // Calculate damage since last sample
        if (currentHealth < data.lastKnownHealth)
        {
            uint32 damage = static_cast<uint32>(data.lastKnownHealth - currentHealth);
            data.damageHistory.push_back({now, damage});
            data.noHealthChangeStart = now; // Reset invuln detection
            data.invulnerable = false;
        }
        else
        {
            // Health didn't decrease (could be healing or invuln)
            DetectInvulnerability(data, currentHealth);
        }

        data.lastKnownHealth = currentHealth;
        data.lastUpdated = now;
        data.cacheTimestamp = 0; // Invalidate cache on new data

        // Trim old entries outside the rolling window
        while (!data.damageHistory.empty() &&
               (now - data.damageHistory.front().timestamp) > DAMAGE_WINDOW_MS)
        {
            data.damageHistory.pop_front();
        }
    }
}

float TTKEstimator::EstimateTTK(Unit* target) const
{
    if (!target || !target->IsAlive())
        return 0.0f;

    std::lock_guard<OrderedMutex<LockOrder::BOT_AI_STATE>> lock(_mutex);

    ObjectGuid guid = target->GetGUID();
    auto it = _targetData.find(guid);
    if (it == _targetData.end())
    {
        // Unknown target - estimate from max health / average group DPS
        // Use a conservative high estimate
        return std::numeric_limits<float>::infinity();
    }

    const TargetTTKData& data = it->second;

    // Check cache
    uint32 now = GameTime::GetGameTimeMS();
    if (data.cacheTimestamp > 0 && (now - data.cacheTimestamp) < CACHE_DURATION_MS)
        return data.cachedTTK;

    // Invulnerable targets never die
    if (data.invulnerable)
    {
        const_cast<TargetTTKData&>(data).cachedTTK = std::numeric_limits<float>::infinity();
        const_cast<TargetTTKData&>(data).cacheTimestamp = now;
        return std::numeric_limits<float>::infinity();
    }

    float ttk = CalculateTTK(data);

    // Update cache
    const_cast<TargetTTKData&>(data).cachedTTK = ttk;
    const_cast<TargetTTKData&>(data).cacheTimestamp = now;

    return ttk;
}

float TTKEstimator::CalculateTTK(const TargetTTKData& data) const
{
    if (data.damageHistory.empty())
        return std::numeric_limits<float>::infinity();

    uint32 now = GameTime::GetGameTimeMS();

    // Calculate total damage in the rolling window
    uint64 totalDamage = 0;
    uint32 windowStart = UINT32_MAX;
    uint32 windowEnd = 0;

    for (const auto& event : data.damageHistory)
    {
        // Only count events within the window
        if ((now - event.timestamp) <= DAMAGE_WINDOW_MS)
        {
            totalDamage += event.damage;
            windowStart = std::min(windowStart, event.timestamp);
            windowEnd = std::max(windowEnd, event.timestamp);
        }
    }

    if (totalDamage == 0 || windowStart == UINT32_MAX)
        return std::numeric_limits<float>::infinity();

    // Calculate time span of damage events
    float windowDurationSec = static_cast<float>(windowEnd - windowStart) / 1000.0f;
    if (windowDurationSec < 0.25f)
    {
        // Not enough time span to estimate DPS reliably
        // Use the sample interval as minimum window
        windowDurationSec = static_cast<float>(SAMPLE_INTERVAL_MS) / 1000.0f;
    }

    // DPS = total damage / time
    float dps = static_cast<float>(totalDamage) / windowDurationSec;

    if (dps <= 0.0f)
        return std::numeric_limits<float>::infinity();

    // TTK = current health / DPS
    float ttk = static_cast<float>(data.lastKnownHealth) / dps;

    return ttk;
}

void TTKEstimator::DetectInvulnerability(TargetTTKData& data, uint64 currentHealth)
{
    uint32 now = GameTime::GetGameTimeMS();

    // If health hasn't changed at all and damage is expected
    if (currentHealth == data.lastKnownHealth || currentHealth > data.lastKnownHealth)
    {
        // Check if we've been dealing damage (have history) but health isn't dropping
        if (!data.damageHistory.empty() &&
            (now - data.noHealthChangeStart) >= INVULN_DETECT_MS)
        {
            data.invulnerable = true;
        }
    }
    else
    {
        // Health is dropping, not invulnerable
        data.noHealthChangeStart = now;
        data.invulnerable = false;
    }
}

bool TTKEstimator::ShouldSkipLongCast(uint32 castTimeMs, Unit* target) const
{
    if (!target || castTimeMs == 0)
        return false;

    float ttk = EstimateTTK(target);

    // Never skip on invulnerable or unknown targets
    if (std::isinf(ttk))
        return false;

    // Determine threshold ratio based on group status
    bool inGroup = _bot && _bot->GetGroup() && _bot->GetGroup()->GetMembersCount() > 1;
    float ratio = inGroup ? GROUP_TTK_RATIO : SOLO_TTK_RATIO;

    float castTimeSec = static_cast<float>(castTimeMs) / 1000.0f;

    // Skip if cast time exceeds the ratio of TTK
    // e.g., in group: 3s cast vs 2s TTK → 3 > 2*0.8 (1.6) → skip
    return castTimeSec > (ttk * ratio);
}

float TTKEstimator::GetGroupDPSOnTarget(Unit* target) const
{
    if (!target)
        return 0.0f;

    std::lock_guard<OrderedMutex<LockOrder::BOT_AI_STATE>> lock(_mutex);

    ObjectGuid guid = target->GetGUID();
    auto it = _targetData.find(guid);
    if (it == _targetData.end())
        return 0.0f;

    const TargetTTKData& data = it->second;
    if (data.damageHistory.empty())
        return 0.0f;

    uint32 now = GameTime::GetGameTimeMS();
    uint64 totalDamage = 0;
    uint32 windowStart = UINT32_MAX;
    uint32 windowEnd = 0;

    for (const auto& event : data.damageHistory)
    {
        if ((now - event.timestamp) <= DAMAGE_WINDOW_MS)
        {
            totalDamage += event.damage;
            windowStart = std::min(windowStart, event.timestamp);
            windowEnd = std::max(windowEnd, event.timestamp);
        }
    }

    if (totalDamage == 0 || windowStart == UINT32_MAX)
        return 0.0f;

    float windowDurationSec = static_cast<float>(windowEnd - windowStart) / 1000.0f;
    if (windowDurationSec < 0.25f)
        windowDurationSec = static_cast<float>(SAMPLE_INTERVAL_MS) / 1000.0f;

    return static_cast<float>(totalDamage) / windowDurationSec;
}

void TTKEstimator::Reset()
{
    std::lock_guard<OrderedMutex<LockOrder::BOT_AI_STATE>> lock(_mutex);
    _targetData.clear();
    _sampleTimer = 0;
    _pruneTimer = 0;
}

void TTKEstimator::PruneStaleTargets()
{
    std::lock_guard<OrderedMutex<LockOrder::BOT_AI_STATE>> lock(_mutex);

    uint32 now = GameTime::GetGameTimeMS();

    for (auto it = _targetData.begin(); it != _targetData.end(); )
    {
        if ((now - it->second.lastUpdated) > STALE_TARGET_MS)
            it = _targetData.erase(it);
        else
            ++it;
    }
}

} // namespace Playerbot
