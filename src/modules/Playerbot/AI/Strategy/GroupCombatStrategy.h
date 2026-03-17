/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Strategy.h"
#include "ObjectGuid.h"
#include <vector>

namespace Playerbot
{

/**
 * @class GroupCombatStrategy
 * @brief Minimal strategy that makes bots attack when group members are in combat
 *
 * This strategy simply monitors group combat state and triggers ClassAI combat behaviors.
 * It doesn't implement complex actions - just coordinates existing bot combat systems.
 *
 * Performance: Uses member caching to avoid O(N²) group iteration every frame.
 * Cache is refreshed every CACHE_REFRESH_INTERVAL ms or when OnGroupChanged() is called.
 */
class TC_GAME_API GroupCombatStrategy : public Strategy
{
public:
    GroupCombatStrategy();
    virtual ~GroupCombatStrategy() = default;

    // Strategy interface
    void InitializeActions();
    void InitializeTriggers();
    void InitializeValues();
    float GetRelevance(BotAI* ai) const;

    // CRITICAL FIX: Only active when bot or group is in combat
    bool IsActive(BotAI* ai) const;

    // CRITICAL FIX: UpdateBehavior is called every frame, not GetRelevance!
    void UpdateBehavior(BotAI* ai, uint32 diff);

    // Call when group composition changes to invalidate cache
    void OnGroupChanged() { _memberCacheDirty = true; }

private:
    bool IsGroupInCombat(BotAI* ai) const;

    /**
     * @brief Find a valid attack target from group combat
     *
     * Priority order:
     * 1. Enemy attacking a group member (threat list check)
     * 2. Group member's victim (what they're fighting)
     * 3. Group member's selected unit (UI target)
     *
     * @param ai Bot AI context
     * @return Unit* Valid hostile target, or nullptr if none found
     */
    Unit* FindGroupCombatTarget(BotAI* ai) const;

    // Performance optimization: Member caching to avoid O(N²) iteration
    void RefreshMemberCache(BotAI* ai);

    // Member cache - stores GUIDs of group members (excluding self)
    mutable ::std::vector<ObjectGuid> _memberCache;
    mutable uint32 _lastCacheUpdate = 0;
    mutable bool _memberCacheDirty = true;

    // Cache refresh interval (1 second) - group composition rarely changes mid-combat
    static constexpr uint32 CACHE_REFRESH_INTERVAL = 1000;
};

} // namespace Playerbot