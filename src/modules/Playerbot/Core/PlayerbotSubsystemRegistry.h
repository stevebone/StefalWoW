/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "PlayerbotSubsystem.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Playerbot
{

struct SubsystemMetrics
{
    std::string name;
    uint64 totalInitTimeUs   = 0;
    uint64 lastUpdateTimeUs  = 0;
    uint64 maxUpdateTimeUs   = 0;
    uint64 totalUpdateTimeUs = 0;
    uint64 updateCount       = 0;

    double GetAvgUpdateTimeMs() const
    {
        return updateCount > 0
            ? static_cast<double>(totalUpdateTimeUs) / updateCount / 1000.0
            : 0.0;
    }

    double GetMaxUpdateTimeMs() const
    {
        return static_cast<double>(maxUpdateTimeUs) / 1000.0;
    }
};

class TC_GAME_API PlayerbotSubsystemRegistry final
{
public:
    static PlayerbotSubsystemRegistry* instance();

    void RegisterSubsystem(std::unique_ptr<IPlayerbotSubsystem> subsystem);

    bool InitializeAll(std::string const& moduleVersion);

    void UpdateAll(uint32 diff);

    void ShutdownAll();

    SubsystemMetrics const* GetMetrics(std::string const& name) const;
    std::vector<SubsystemMetrics> GetAllMetrics() const;

    uint32 GetSubsystemCount() const { return static_cast<uint32>(_subsystems.size()); }

    std::string const& GetLastError() const { return _lastError; }

private:
    PlayerbotSubsystemRegistry() = default;
    ~PlayerbotSubsystemRegistry() = default;

    PlayerbotSubsystemRegistry(PlayerbotSubsystemRegistry const&) = delete;
    PlayerbotSubsystemRegistry& operator=(PlayerbotSubsystemRegistry const&) = delete;

    struct SubsystemEntry
    {
        std::unique_ptr<IPlayerbotSubsystem> subsystem;
        SubsystemInfo info;
    };

    std::vector<SubsystemEntry> _subsystems;

    // Cached sorted indices for update loop (built once on first UpdateAll)
    std::vector<size_t> _updateOrder;
    bool _updateOrderCached = false;

    std::unordered_map<std::string, SubsystemMetrics> _metrics;

    std::string _lastError;

    static constexpr uint64 UPDATE_WARN_THRESHOLD_US = 100000; // 100ms
};

} // namespace Playerbot

#define sPlayerbotSubsystemRegistry Playerbot::PlayerbotSubsystemRegistry::instance()
