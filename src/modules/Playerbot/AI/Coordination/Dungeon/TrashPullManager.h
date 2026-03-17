/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "DungeonState.h"
#include "Define.h"
#include "ObjectGuid.h"
#include <vector>
#include <map>
#include <queue>
#include <optional>

namespace Playerbot {

class DungeonCoordinator;

/**
 * @struct PullPlan
 * @brief Plan for pulling a trash pack
 */
struct PullPlan
{
    uint32 packId = 0;
    ::std::vector<::std::pair<ObjectGuid, RaidMarker>> markerAssignments;  // target -> marker
    ::std::vector<::std::pair<ObjectGuid, ObjectGuid>> ccAssignments;       // CCer -> target
    ObjectGuid puller;
    bool useLOS = false;
    float pullPositionX = 0.0f;
    float pullPositionY = 0.0f;
    float pullPositionZ = 0.0f;

    [[nodiscard]] bool IsValid() const { return packId > 0 && !puller.IsEmpty(); }
};

/**
 * @class TrashPullManager
 * @brief Manages trash pack pulling, CC assignments, and markers
 *
 * Responsibilities:
 * - Track all trash packs in dungeon
 * - Plan pulls with CC and marker assignments
 * - Determine optimal clear order
 * - Manage LOS pulls and positioning
 *
 * Usage:
 * @code
 * TrashPullManager manager(&coordinator);
 * manager.Initialize(dungeonId);
 *
 * // Get next pull
 * PullPlan* plan = manager.GetCurrentPullPlan();
 * if (plan && manager.IsSafeToPull())
 * {
 *     manager.ApplyMarkers(*plan);
 *     manager.ExecutePull(*plan);
 * }
 * @endcode
 */
class TC_GAME_API TrashPullManager
{
public:
    explicit TrashPullManager(DungeonCoordinator* coordinator);
    ~TrashPullManager() = default;

    // ====================================================================
    // LIFECYCLE
    // ====================================================================

    /**
     * @brief Initialize for specific dungeon
     * @param dungeonId Dungeon map ID
     */
    void Initialize(uint32 dungeonId);

    /**
     * @brief Update logic
     * @param diff Time since last update
     */
    void Update(uint32 diff);

    /**
     * @brief Reset state
     */
    void Reset();

    // ====================================================================
    // PACK MANAGEMENT
    // ====================================================================

    /**
     * @brief Register a trash pack
     * @param pack Pack to register
     */
    void RegisterPack(const TrashPack& pack);

    /**
     * @brief Mark pack as cleared
     * @param packId Cleared pack ID
     */
    void OnPackCleared(uint32 packId);

    /**
     * @brief Mark pack as pulled (in combat)
     * @param packId Pulled pack ID
     */
    void OnPackPulled(uint32 packId);

    /**
     * @brief Get pack by ID
     * @param packId Pack ID
     * @return Pointer to pack or nullptr
     */
    [[nodiscard]] TrashPack* GetPack(uint32 packId);

    /**
     * @brief Get all packs
     */
    [[nodiscard]] const ::std::map<uint32, TrashPack>& GetAllPacks() const { return _packs; }

    /**
     * @brief Get remaining pack count
     */
    [[nodiscard]] uint32 GetRemainingPackCount() const;

    // ====================================================================
    // PULL PLANNING
    // ====================================================================

    /**
     * @brief Get current pull plan
     * @return Pointer to current plan or nullptr
     */
    [[nodiscard]] PullPlan* GetCurrentPullPlan();

    /**
     * @brief Create a pull plan for pack
     * @param packId Pack to plan for
     * @return true if plan created successfully
     */
    bool CreatePullPlan(uint32 packId);

    /**
     * @brief Execute the pull
     * @param plan Plan to execute
     */
    void ExecutePull(const PullPlan& plan);

    /**
     * @brief Clear current pull plan
     */
    void ClearCurrentPlan();

    /**
     * @brief Check if has active pull plan
     */
    [[nodiscard]] bool HasPullPlan() const { return _hasPlan; }

    // ====================================================================
    // CC MANAGEMENT
    // ====================================================================

    /**
     * @brief Assign CC for a pack
     * @param pack Pack to assign CC for
     */
    void AssignCC(const TrashPack& pack);

    /**
     * @brief Get CC responsible for target
     * @param target CC target
     * @return GUID of CCer or empty
     */
    [[nodiscard]] ObjectGuid GetCCResponsible(ObjectGuid target) const;

    /**
     * @brief Check if target is CC'd
     * @param target Target to check
     * @return true if CC'd
     */
    [[nodiscard]] bool IsTargetCCd(ObjectGuid target) const;

    /**
     * @brief Called when CC breaks
     * @param target Target whose CC broke
     */
    void OnCCBroken(ObjectGuid target);

    /**
     * @brief Get all CC assignments
     */
    [[nodiscard]] const ::std::map<ObjectGuid, ObjectGuid>& GetCCAssignments() const { return _ccAssignments; }

    // ====================================================================
    // MARKER MANAGEMENT
    // ====================================================================

    /**
     * @brief Apply markers for pull plan
     * @param plan Plan with marker assignments
     */
    void ApplyMarkers(const PullPlan& plan);

    /**
     * @brief Clear all markers
     */
    void ClearMarkers();

    /**
     * @brief Get marker for target
     * @param target Target to check
     * @return Marker or NONE
     */
    [[nodiscard]] RaidMarker GetMarkerForTarget(ObjectGuid target) const;

    /**
     * @brief Set marker for target
     * @param target Target
     * @param marker Marker to assign
     */
    void SetMarker(ObjectGuid target, RaidMarker marker);

    // ====================================================================
    // SAFETY CHECKS
    // ====================================================================

    /**
     * @brief Check if safe to pull
     * @return true if safe
     */
    [[nodiscard]] bool IsSafeToPull() const;

    /**
     * @brief Check if group is ready for pull
     * @return true if ready
     */
    [[nodiscard]] bool IsGroupReadyForPull() const;

    /**
     * @brief Estimate pull difficulty
     * @param packId Pack to evaluate
     * @return Difficulty score (0-100)
     */
    [[nodiscard]] uint32 GetEstimatedPullDifficulty(uint32 packId) const;

    // ====================================================================
    // PATHING & ROUTING
    // ====================================================================

    /**
     * @brief Get optimal clear order
     * @return Ordered list of pack IDs
     */
    [[nodiscard]] ::std::vector<uint32> GetOptimalClearOrder() const;

    /**
     * @brief Check if pack can be skipped
     * @param packId Pack to check
     * @return true if can skip
     */
    [[nodiscard]] bool CanSkipPack(uint32 packId) const;

    /**
     * @brief Get next pack to pull
     * @return Pack ID or 0 if none
     */
    [[nodiscard]] uint32 GetNextPackToPull() const;

    /**
     * @brief Queue a pack for pulling
     * @param packId Pack to queue
     */
    void QueuePack(uint32 packId);

private:
    DungeonCoordinator* _coordinator;

    // Pack tracking
    ::std::map<uint32, TrashPack> _packs;
    ::std::vector<uint32> _clearedPacks;
    ::std::vector<uint32> _pulledPacks;  // Currently in combat
    ::std::queue<uint32> _pullQueue;

    // Current plan
    PullPlan _currentPlan;
    bool _hasPlan = false;

    // CC tracking
    ::std::map<ObjectGuid, ObjectGuid> _ccAssignments;   // target -> CCer
    ::std::map<ObjectGuid, uint32> _ccSpells;            // target -> spell used
    ::std::map<ObjectGuid, bool> _ccActive;              // target -> is CC active

    // Marker tracking
    ::std::map<ObjectGuid, RaidMarker> _markerAssignments;

    // Optimal route cache
    mutable ::std::vector<uint32> _cachedRoute;
    mutable bool _routeDirty = true;

    // ====================================================================
    // HELPERS
    // ====================================================================

    /**
     * @brief Select best CCer for target
     */
    [[nodiscard]] ObjectGuid SelectBestCCer(ObjectGuid target) const;

    /**
     * @brief Check if player can CC target
     */
    [[nodiscard]] bool CanCC(ObjectGuid player, ObjectGuid target) const;

    /**
     * @brief Select marker for target
     * @param killOrder Position in kill order (0 = first)
     * @param isCC true if CC target
     */
    [[nodiscard]] RaidMarker SelectMarkerForRole(uint8 killOrder, bool isCC) const;

    /**
     * @brief Calculate kill order for pack
     * @param pack Pack to analyze
     * @param order Output vector of target GUIDs
     */
    void CalculateKillOrder(const TrashPack& pack, ::std::vector<ObjectGuid>& order);

    /**
     * @brief Calculate optimal route (updates cache)
     */
    void CalculateOptimalRoute() const;

    /**
     * @brief Get CC spells available to player
     */
    [[nodiscard]] ::std::vector<uint32> GetAvailableCCSpells(ObjectGuid player) const;

    /**
     * @brief Check if target is immune to CC type
     */
    [[nodiscard]] bool IsImmuneToCC(ObjectGuid target, uint32 ccSpellId) const;
};

} // namespace Playerbot
