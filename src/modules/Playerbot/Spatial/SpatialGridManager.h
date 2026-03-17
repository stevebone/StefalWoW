/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#ifndef PLAYERBOT_SPATIAL_GRID_MANAGER_H
#define PLAYERBOT_SPATIAL_GRID_MANAGER_H

#include "Define.h"
#include "DoubleBufferedSpatialGrid.h"
#include "Threading/LockHierarchy.h"
#include <unordered_map>
#include <memory>
#include <shared_mutex>

class Map;

namespace Playerbot
{

/**
 * @brief Global manager for all spatial grids across all maps
 *
 * MEMORY LIFECYCLE MANAGEMENT (v2):
 * - Grids are created on-demand when bots enter a map
 * - Grids are automatically destroyed when inactive for GRID_IDLE_TIMEOUT_SEC
 * - CleanupInactiveGrids() should be called periodically (every 60s recommended)
 * - Memory usage is tracked and logged
 *
 * Implements ISpatialGridManager for dependency injection compatibility.
 * Uses singleton pattern for backward compatibility (transitional).
 *
 * **Migration Path:**
 * - Old code: sSpatialGridManager.GetGrid(mapId)
 * - New code: Services::Container().Resolve<ISpatialGridManager>()->GetGrid(mapId)
 */
class TC_GAME_API SpatialGridManager final
{
public:
    // Configuration constants
    static constexpr uint32 GRID_IDLE_TIMEOUT_SEC = 300;  // 5 minutes of inactivity before cleanup
    static constexpr uint32 CLEANUP_INTERVAL_SEC = 60;     // Check for inactive grids every 60 seconds

    static SpatialGridManager& Instance()
    {
        static SpatialGridManager instance;
        return instance;
    }

    // ISpatialGridManager interface implementation
    void CreateGrid(Map* map);
    void DestroyGrid(uint32 mapId);
    DoubleBufferedSpatialGrid* GetGrid(uint32 mapId);
    DoubleBufferedSpatialGrid* GetGrid(Map* map);
    void DestroyAllGrids();
    void UpdateGrid(uint32 mapId);
    void UpdateGrid(Map* map);
    size_t GetGridCount() const;

    // ========================================================================
    // PERFORMANCE OPTIMIZATION: Combined Get + Create
    // ========================================================================
    /**
     * @brief Get grid for map, creating it if it doesn't exist (OPTIMAL)
     *
     * PERFORMANCE: This is the PREFERRED method for accessing grids!
     * - Uses optimized double-checked locking internally
     * - Eliminates the redundant pattern: if (!GetGrid()) { CreateGrid(); GetGrid(); }
     * - Single method call instead of 3
     *
     * @param map The map to get/create grid for
     * @return Pointer to the spatial grid (never null if map is valid)
     */
    DoubleBufferedSpatialGrid* GetOrCreateGrid(Map* map);

    // ========================================================================
    // MEMORY LIFECYCLE MANAGEMENT (NEW)
    // ========================================================================

    /**
     * @brief Clean up grids that have been inactive for GRID_IDLE_TIMEOUT_SEC
     *
     * Should be called periodically (every CLEANUP_INTERVAL_SEC) from a timer hook.
     * Destroys grids with zero population that haven't been accessed recently.
     *
     * @return Number of grids cleaned up
     */
    size_t CleanupInactiveGrids();

    /**
     * @brief Get total memory usage across all grids in bytes
     */
    size_t GetTotalMemoryUsage() const;

    /**
     * @brief Get memory usage summary for logging
     */
    struct MemoryStats
    {
        size_t totalGrids{0};
        size_t totalMemoryBytes{0};
        size_t peakMemoryBytes{0};
        size_t totalPopulation{0};
        size_t totalActiveCells{0};
    };
    MemoryStats GetMemoryStats() const;

    /**
     * @brief Log memory statistics
     */
    void LogMemoryStats() const;

    /**
     * @brief Mark a grid as recently accessed (prevents cleanup)
     */
    void TouchGrid(uint32 mapId);

private:
    SpatialGridManager() = default;
    ~SpatialGridManager() { DestroyAllGrids(); }

    // Grid metadata for lifecycle management
    struct GridInfo
    {
        ::std::unique_ptr<DoubleBufferedSpatialGrid> grid;
        ::std::chrono::steady_clock::time_point lastAccessTime;
        ::std::chrono::steady_clock::time_point creationTime;
    };

    ::std::unordered_map<uint32, GridInfo> _grids;
    mutable OrderedSharedMutex<LockOrder::SPATIAL_GRID> _mutex;  // Allow concurrent reads with deadlock prevention

    // Memory tracking
    mutable ::std::atomic<size_t> _peakMemoryUsage{0};

    // Non-copyable
    SpatialGridManager(SpatialGridManager const&) = delete;
    SpatialGridManager& operator=(SpatialGridManager const&) = delete;
};

// Convenience accessor
#define sSpatialGridManager Playerbot::SpatialGridManager::Instance()

} // namespace Playerbot

#endif // PLAYERBOT_SPATIAL_GRID_MANAGER_H
