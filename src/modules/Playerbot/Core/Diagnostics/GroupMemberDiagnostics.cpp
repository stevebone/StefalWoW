/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Group Member Lookup Diagnostics - Implementation
 */

#include "GroupMemberDiagnostics.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Log.h"
#include "GameTime.h"
#include "../../Session/BotWorldSessionMgr.h"
#include <sstream>
#include <iomanip>

namespace Playerbot
{

GroupMemberDiagnostics* GroupMemberDiagnostics::Instance()
{
    static GroupMemberDiagnostics instance;
    return &instance;
}

GroupMemberDiagnostics::GroupMemberDiagnostics()
{
    _recentFailures.resize(MAX_RECENT_FAILURES);
}

Player* GroupMemberDiagnostics::DiagnosticLookup(
    ObjectGuid guid,
    const char* callerFunc,
    const char* callerFile,
    uint32 callerLine)
{
    if (!_enabled)
    {
        // When disabled, just do a simple lookup chain
        if (Player* p = ObjectAccessor::FindPlayer(guid))
            return p;
        if (Player* p = ObjectAccessor::FindConnectedPlayer(guid))
            return p;
        if (Player* p = sBotWorldSessionMgr->GetPlayerBot(guid))
            return p;
        return nullptr;
    }
    
    ++_totalLookups;
    
    auto [player, method] = TryAllLookupMethods(guid);
    
    // Update statistics
    std::string callerKey = MakeCallerKey(callerFunc, callerFile, callerLine);
    {
        std::lock_guard<std::mutex> lock(_statsMutex);
        auto& stats = _callerStats[callerKey];
        stats.function = callerFunc;
        stats.file = callerFile;
        stats.line = callerLine;
        stats.totalAttempts++;
        
        if (player)
        {
            stats.successCount++;
            switch (method)
            {
                case LookupMethod::OBJECT_ACCESSOR:
                    stats.successObjectAccessor++;
                    break;
                case LookupMethod::CONNECTED_PLAYER:
                    stats.successConnectedPlayer++;
                    break;
                case LookupMethod::BOT_SESSION_MGR:
                    stats.successBotSessionMgr++;
                    break;
                case LookupMethod::GROUP_REFERENCE:
                    stats.successGroupReference++;
                    break;
                case LookupMethod::SPATIAL_GRID:
                    stats.successSpatialGrid++;
                    break;
                default:
                    break;
            }
        }
        else
        {
            stats.failureCount++;
            if (IsKnownBot(guid))
                stats.failedBotLookups++;
            else
                stats.failedPlayerLookups++;
        }
    }
    
    // Log based on result and verbosity
    if (!player)
    {
        ++_failedLookups;
        bool isBot = IsKnownBot(guid);
        if (isBot)
            ++_botLookupFailures;
        else
            ++_playerLookupFailures;
        
        // Always log failures
        TC_LOG_ERROR("module.playerbot.diag.group", 
            "⚠️ [GroupMemberDiag] LOOKUP FAILED: GUID={} IsBot={} Caller={}:{} ({})",
            guid.ToString(), isBot ? "YES" : "NO", 
            callerFile, callerLine, callerFunc);
        
        // Store in recent failures
        {
            std::lock_guard<std::mutex> lock(_recentMutex);
            auto& attempt = _recentFailures[_recentIndex % MAX_RECENT_FAILURES];
            attempt.guid = guid;
            attempt.timestamp = GameTime::GetGameTimeMS();
            attempt.successMethod = LookupMethod::NONE;
            attempt.isBot = isBot;
            attempt.callerFunction = callerFunc;
            attempt.callerFile = callerFile;
            attempt.callerLine = callerLine;
            ++_recentIndex;
        }
    }
    else if (_verbose)
    {
        const char* methodName = "UNKNOWN";
        switch (method)
        {
            case LookupMethod::OBJECT_ACCESSOR: methodName = "ObjectAccessor"; break;
            case LookupMethod::CONNECTED_PLAYER: methodName = "ConnectedPlayer"; break;
            case LookupMethod::BOT_SESSION_MGR: methodName = "BotSessionMgr"; break;
            case LookupMethod::GROUP_REFERENCE: methodName = "GroupReference"; break;
            case LookupMethod::SPATIAL_GRID: methodName = "SpatialGrid"; break;
            default: break;
        }
        
        TC_LOG_DEBUG("module.playerbot.diag.group",
            "✓ [GroupMemberDiag] LOOKUP OK: {} via {} Caller={}:{}",
            player->GetName(), methodName, callerFile, callerLine);
    }
    
    return player;
}

void GroupMemberDiagnostics::RecordLookupResult(
    ObjectGuid guid,
    Player* result,
    LookupMethod methodUsed,
    const char* callerFunc,
    const char* callerFile,
    uint32 callerLine)
{
    if (!_enabled)
        return;
    
    ++_totalLookups;
    
    std::string callerKey = MakeCallerKey(callerFunc, callerFile, callerLine);
    {
        std::lock_guard<std::mutex> lock(_statsMutex);
        auto& stats = _callerStats[callerKey];
        stats.function = callerFunc;
        stats.file = callerFile;
        stats.line = callerLine;
        stats.totalAttempts++;
        
        if (result)
        {
            stats.successCount++;
            switch (methodUsed)
            {
                case LookupMethod::OBJECT_ACCESSOR:
                    stats.successObjectAccessor++;
                    break;
                case LookupMethod::CONNECTED_PLAYER:
                    stats.successConnectedPlayer++;
                    break;
                case LookupMethod::BOT_SESSION_MGR:
                    stats.successBotSessionMgr++;
                    break;
                case LookupMethod::GROUP_REFERENCE:
                    stats.successGroupReference++;
                    break;
                case LookupMethod::SPATIAL_GRID:
                    stats.successSpatialGrid++;
                    break;
                default:
                    break;
            }
        }
        else
        {
            stats.failureCount++;
            ++_failedLookups;
            
            if (IsKnownBot(guid))
            {
                stats.failedBotLookups++;
                ++_botLookupFailures;
            }
            else
            {
                stats.failedPlayerLookups++;
                ++_playerLookupFailures;
            }
        }
    }
}

std::pair<Player*, LookupMethod> GroupMemberDiagnostics::TryAllLookupMethods(ObjectGuid guid)
{
    if (guid.IsEmpty())
        return {nullptr, LookupMethod::NONE};
    
    // Method 1: ObjectAccessor::FindPlayer (fastest, same map)
    if (Player* player = ObjectAccessor::FindPlayer(guid))
        return {player, LookupMethod::OBJECT_ACCESSOR};
    
    // Method 2: ObjectAccessor::FindConnectedPlayer (any connected player)
    if (Player* player = ObjectAccessor::FindConnectedPlayer(guid))
        return {player, LookupMethod::CONNECTED_PLAYER};
    
    // Method 3: BotWorldSessionMgr (for bots not in ObjectAccessor)
    if (Player* player = sBotWorldSessionMgr->GetPlayerBot(guid))
        return {player, LookupMethod::BOT_SESSION_MGR};
    
    // All methods failed
    return {nullptr, LookupMethod::NONE};
}

bool GroupMemberDiagnostics::IsKnownBot(ObjectGuid guid) const
{
    // Check if this GUID is registered as a bot
    return sBotWorldSessionMgr->GetPlayerBot(guid) != nullptr;
}

std::string GroupMemberDiagnostics::MakeCallerKey(
    const char* func, 
    const char* file, 
    uint32 line) const
{
    std::ostringstream ss;
    ss << func << "@" << file << ":" << line;
    return ss.str();
}

float GroupMemberDiagnostics::GetOverallSuccessRate() const
{
    uint32 total = _totalLookups.load();
    uint32 failed = _failedLookups.load();
    if (total == 0)
        return 100.0f;
    return (float)(total - failed) / total * 100.0f;
}

CallerStatistics GroupMemberDiagnostics::GetCallerStats(const std::string& function) const
{
    std::lock_guard<std::mutex> lock(_statsMutex);
    for (const auto& [key, stats] : _callerStats)
    {
        if (stats.function == function)
            return stats;
    }
    return CallerStatistics{};
}

void GroupMemberDiagnostics::Reset()
{
    _totalLookups = 0;
    _failedLookups = 0;
    _botLookupFailures = 0;
    _playerLookupFailures = 0;
    
    {
        std::lock_guard<std::mutex> lock(_statsMutex);
        _callerStats.clear();
    }
    
    {
        std::lock_guard<std::mutex> lock(_recentMutex);
        _recentFailures.clear();
        _recentFailures.resize(MAX_RECENT_FAILURES);
        _recentIndex = 0;
    }
    
    TC_LOG_INFO("module.playerbot.diag.group", "[GroupMemberDiag] Statistics reset");
}

std::string GroupMemberDiagnostics::GetReport() const
{
    std::ostringstream report;
    
    report << "\n";
    report << "╔══════════════════════════════════════════════════════════════════════╗\n";
    report << "║           GROUP MEMBER LOOKUP DIAGNOSTICS REPORT                     ║\n";
    report << "╠══════════════════════════════════════════════════════════════════════╣\n";
    
    // Global statistics
    uint32 total = _totalLookups.load();
    uint32 failed = _failedLookups.load();
    uint32 botFailed = _botLookupFailures.load();
    uint32 playerFailed = _playerLookupFailures.load();
    
    report << "║ GLOBAL STATISTICS                                                    ║\n";
    report << "║   Total Lookups:       " << std::setw(10) << total << "                                  ║\n";
    report << "║   Successful:          " << std::setw(10) << (total - failed) << "                                  ║\n";
    report << "║   Failed:              " << std::setw(10) << failed << "                                  ║\n";
    report << "║   Success Rate:        " << std::setw(9) << std::fixed << std::setprecision(1) 
           << GetOverallSuccessRate() << "%                                  ║\n";
    report << "║                                                                      ║\n";
    report << "║ FAILURE BREAKDOWN                                                    ║\n";
    report << "║   Bot Lookup Failures: " << std::setw(10) << botFailed << "  <-- LIKELY THE PROBLEM!       ║\n";
    report << "║   Player Failures:     " << std::setw(10) << playerFailed << "                                  ║\n";
    report << "╠══════════════════════════════════════════════════════════════════════╣\n";
    
    // Per-caller statistics (sorted by failure count)
    std::vector<std::pair<std::string, CallerStatistics>> sortedStats;
    {
        std::lock_guard<std::mutex> lock(_statsMutex);
        for (const auto& [key, stats] : _callerStats)
        {
            sortedStats.emplace_back(key, stats);
        }
    }
    
    std::sort(sortedStats.begin(), sortedStats.end(),
        [](const auto& a, const auto& b) {
            return a.second.failureCount > b.second.failureCount;
        });
    
    report << "║ TOP PROBLEM LOCATIONS (by failure count)                             ║\n";
    report << "╠══════════════════════════════════════════════════════════════════════╣\n";
    
    uint32 shown = 0;
    for (const auto& [key, stats] : sortedStats)
    {
        if (stats.failureCount == 0 || shown >= 10)
            break;
        
        report << "║ " << std::left << std::setw(40) << stats.function.substr(0, 40) << "                              ║\n";
        report << "║   Attempts: " << std::setw(6) << stats.totalAttempts 
               << "  Failures: " << std::setw(6) << stats.failureCount
               << "  Rate: " << std::setw(5) << std::fixed << std::setprecision(1) 
               << stats.GetSuccessRate() << "%        ║\n";
        report << "║   Bot Failures: " << std::setw(6) << stats.failedBotLookups
               << "  Player Failures: " << std::setw(6) << stats.failedPlayerLookups << "              ║\n";
        
        // Show which methods would have worked
        if (stats.successBotSessionMgr > 0)
        {
            report << "║   → BotSessionMgr would fix " << stats.successBotSessionMgr << " lookups!                       ║\n";
        }
        
        report << "╠──────────────────────────────────────────────────────────────────────╣\n";
        ++shown;
    }
    
    report << "╚══════════════════════════════════════════════════════════════════════╝\n";
    
    return report.str();
}

void GroupMemberDiagnostics::LogSummary() const
{
    std::string report = GetReport();
    
    // Split by newlines and log each line
    std::istringstream stream(report);
    std::string line;
    while (std::getline(stream, line))
    {
        if (!line.empty())
            TC_LOG_INFO("module.playerbot.diag.group", "{}", line);
    }
}

} // namespace Playerbot
