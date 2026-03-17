/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Group Member Lookup Diagnostics
 * 
 * PURPOSE:
 * This diagnostic system tracks and reports issues with group member lookups.
 * It helps identify when bots or players cannot be found through standard
 * ObjectAccessor methods, which causes combat coordination and healing to fail.
 *
 * USAGE:
 * 1. Enable diagnostics: sGroupMemberDiagnostics->SetEnabled(true)
 * 2. Run dungeons/group content
 * 3. Check logs for patterns: TC_LOG filtered by "GroupMemberDiag"
 * 4. Call GetReport() for summary statistics
 *
 * PROBLEM BEING DIAGNOSED:
 * - group->GetMembers() iterates via GroupReference
 * - ref.GetSource() uses ObjectAccessor::FindPlayer internally
 * - Bots managed by BotWorldSessionMgr are often NOT found
 * - Result: Healers can't find targets, DPS don't assist, coordination fails
 */

#ifndef PLAYERBOT_GROUP_MEMBER_DIAGNOSTICS_H
#define PLAYERBOT_GROUP_MEMBER_DIAGNOSTICS_H

#include "Define.h"
#include "ObjectGuid.h"
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <string>
#include <chrono>

class Player;
class Group;

namespace Playerbot
{

// Lookup method that succeeded (or none)
enum class LookupMethod : uint8
{
    NONE = 0,                    // All methods failed
    OBJECT_ACCESSOR = 1,         // ObjectAccessor::FindPlayer()
    CONNECTED_PLAYER = 2,        // ObjectAccessor::FindConnectedPlayer()
    BOT_SESSION_MGR = 3,         // BotWorldSessionMgr::GetPlayerBot()
    GROUP_REFERENCE = 4,         // GroupReference::GetSource() direct
    SPATIAL_GRID = 5             // SpatialGridQueryHelpers
};

// Statistics for a single lookup attempt
struct LookupAttempt
{
    ObjectGuid guid;
    uint32 timestamp;
    LookupMethod successMethod;
    bool isBot;
    std::string callerFunction;
    std::string callerFile;
    uint32 callerLine;
};

// Aggregated statistics per caller location
struct CallerStatistics
{
    std::string function;
    std::string file;
    uint32 line;
    
    uint32 totalAttempts = 0;
    uint32 successCount = 0;
    uint32 failureCount = 0;
    
    // Success by method
    uint32 successObjectAccessor = 0;
    uint32 successConnectedPlayer = 0;
    uint32 successBotSessionMgr = 0;
    uint32 successGroupReference = 0;
    uint32 successSpatialGrid = 0;
    
    // Failure details
    uint32 failedBotLookups = 0;      // Known bots that couldn't be found
    uint32 failedPlayerLookups = 0;   // Players that couldn't be found
    
    float GetSuccessRate() const 
    { 
        return totalAttempts > 0 ? (float)successCount / totalAttempts * 100.0f : 0.0f; 
    }
};

/**
 * @class GroupMemberDiagnostics
 * @brief Singleton that tracks group member lookup success/failure rates
 *
 * This helps identify:
 * 1. Which code locations have lookup failures
 * 2. Whether failures are for bots vs players
 * 3. Which fallback methods would have worked
 */
class TC_GAME_API GroupMemberDiagnostics
{
public:
    static GroupMemberDiagnostics* Instance();
    
    // Enable/disable diagnostics (disabled by default for performance)
    void SetEnabled(bool enabled) { _enabled = enabled; }
    bool IsEnabled() const { return _enabled; }
    
    // Set verbose logging (logs every lookup, not just failures)
    void SetVerbose(bool verbose) { _verbose = verbose; }
    bool IsVerbose() const { return _verbose; }
    
    /**
     * @brief Record a lookup attempt with full diagnostics
     * 
     * Call this when attempting to find a group member. It will:
     * 1. Try all lookup methods
     * 2. Record which succeeded/failed
     * 3. Return the found player (or nullptr)
     * 
     * @param guid The GUID to look up
     * @param callerFunc __FUNCTION__ of caller
     * @param callerFile __FILE__ of caller
     * @param callerLine __LINE__ of caller
     * @return Player* if found by any method, nullptr otherwise
     */
    Player* DiagnosticLookup(
        ObjectGuid guid,
        const char* callerFunc,
        const char* callerFile,
        uint32 callerLine);
    
    /**
     * @brief Record a lookup that already happened (for wrapping existing code)
     */
    void RecordLookupResult(
        ObjectGuid guid,
        Player* result,
        LookupMethod methodUsed,
        const char* callerFunc,
        const char* callerFile,
        uint32 callerLine);
    
    /**
     * @brief Check if a GUID belongs to a bot (for statistics)
     */
    bool IsKnownBot(ObjectGuid guid) const;
    
    /**
     * @brief Get diagnostic report as string
     */
    std::string GetReport() const;
    
    /**
     * @brief Get statistics for a specific caller location
     */
    CallerStatistics GetCallerStats(const std::string& function) const;
    
    /**
     * @brief Reset all statistics
     */
    void Reset();
    
    /**
     * @brief Log current statistics summary
     */
    void LogSummary() const;
    
    // Quick access stats
    uint32 GetTotalLookups() const { return _totalLookups; }
    uint32 GetFailedLookups() const { return _failedLookups; }
    uint32 GetBotLookupFailures() const { return _botLookupFailures; }
    float GetOverallSuccessRate() const;
    
private:
    GroupMemberDiagnostics();
    ~GroupMemberDiagnostics() = default;
    
    // Try each lookup method and return result + method used
    std::pair<Player*, LookupMethod> TryAllLookupMethods(ObjectGuid guid);
    
    // Generate caller key for statistics map
    std::string MakeCallerKey(const char* func, const char* file, uint32 line) const;
    
    std::atomic<bool> _enabled{false};
    std::atomic<bool> _verbose{false};
    
    // Global counters
    std::atomic<uint32> _totalLookups{0};
    std::atomic<uint32> _failedLookups{0};
    std::atomic<uint32> _botLookupFailures{0};
    std::atomic<uint32> _playerLookupFailures{0};
    
    // Per-caller statistics
    mutable std::mutex _statsMutex;
    std::unordered_map<std::string, CallerStatistics> _callerStats;
    
    // Recent failures for detailed logging (ring buffer)
    static constexpr uint32 MAX_RECENT_FAILURES = 100;
    mutable std::mutex _recentMutex;
    std::vector<LookupAttempt> _recentFailures;
    uint32 _recentIndex = 0;
    
    // Singleton
    GroupMemberDiagnostics(const GroupMemberDiagnostics&) = delete;
    GroupMemberDiagnostics& operator=(const GroupMemberDiagnostics&) = delete;
};

#define sGroupMemberDiagnostics GroupMemberDiagnostics::Instance()

// Convenience macro for diagnostic lookup with automatic caller info
#define DIAG_LOOKUP_MEMBER(guid) \
    sGroupMemberDiagnostics->DiagnosticLookup(guid, __FUNCTION__, __FILE__, __LINE__)

// Macro to record existing lookup result
#define DIAG_RECORD_LOOKUP(guid, result, method) \
    if (sGroupMemberDiagnostics->IsEnabled()) \
        sGroupMemberDiagnostics->RecordLookupResult(guid, result, method, __FUNCTION__, __FILE__, __LINE__)

} // namespace Playerbot

#endif // PLAYERBOT_GROUP_MEMBER_DIAGNOSTICS_H
