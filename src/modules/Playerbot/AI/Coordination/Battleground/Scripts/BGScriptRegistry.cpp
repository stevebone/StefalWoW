/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2021+ WarheadCore <https://github.com/AzerothCore/WarheadCore>
 * Copyright (C) 2025+ TrinityCore Playerbot Integration
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BGScriptRegistry.h"
#include "Log.h"
#include <sstream>

namespace Playerbot::Coordination::Battleground
{

BGScriptRegistry& BGScriptRegistry::Instance()
{
    static BGScriptRegistry instance;
    return instance;
}

bool BGScriptRegistry::RegisterScript(uint32 mapId, ScriptFactory factory, const std::string& name)
{
    std::unique_lock lock(m_mutex);

    // Check if already registered
    if (m_scripts.find(mapId) != m_scripts.end())
    {
        TC_LOG_WARN("playerbots.bg.script",
            "BGScriptRegistry: Script already registered for map {} ({}), replacing with {}",
            mapId, m_scripts[mapId].name, name);
    }

    // Register the script
    ScriptEntry entry;
    entry.factory = std::move(factory);
    entry.name = name.empty() ? "Unknown" : name;
    entry.createCount = 0;

    m_scripts[mapId] = std::move(entry);

    TC_LOG_INFO("playerbots.bg.script",
        "BGScriptRegistry: Registered script '{}' for map {}",
        m_scripts[mapId].name, mapId);

    return true;
}

bool BGScriptRegistry::UnregisterScript(uint32 mapId)
{
    std::unique_lock lock(m_mutex);

    auto it = m_scripts.find(mapId);
    if (it == m_scripts.end())
    {
        TC_LOG_WARN("playerbots.bg.script",
            "BGScriptRegistry: No script registered for map {}", mapId);
        return false;
    }

    TC_LOG_INFO("playerbots.bg.script",
        "BGScriptRegistry: Unregistered script '{}' for map {}",
        it->second.name, mapId);

    m_scripts.erase(it);
    return true;
}

void BGScriptRegistry::RegisterScriptMultiple(const std::vector<uint32>& mapIds,
    ScriptFactory factory, const std::string& name)
{
    if (mapIds.empty())
        return;

    for (uint32 mapId : mapIds)
    {
        // Create a copy of the factory for each registration
        RegisterScript(mapId, factory, name);
    }

    TC_LOG_INFO("playerbots.bg.script",
        "BGScriptRegistry: Registered script '{}' for {} map variants",
        name, mapIds.size());
}

bool BGScriptRegistry::HasScript(uint32 mapId) const
{
    std::shared_lock lock(m_mutex);
    return m_scripts.find(mapId) != m_scripts.end();
}

std::unique_ptr<IBGScript> BGScriptRegistry::CreateScript(uint32 mapId) const
{
    // First, try to find with shared lock
    {
        std::shared_lock lock(m_mutex);
        auto it = m_scripts.find(mapId);
        if (it == m_scripts.end())
        {
            TC_LOG_DEBUG("playerbots.bg.script",
                "BGScriptRegistry: No script registered for map {}", mapId);
            return nullptr;
        }

        // We need to modify createCount, so we'll upgrade to unique lock
    }

    // Upgrade to unique lock to increment counter
    std::unique_lock lock(m_mutex);
    auto it = m_scripts.find(mapId);
    if (it == m_scripts.end())
        return nullptr;

    // Create the script
    auto script = it->second.factory();
    if (script)
    {
        // Increment create count (cast away const since we have unique lock now)
        const_cast<ScriptEntry&>(it->second).createCount++;

        TC_LOG_DEBUG("playerbots.bg.script",
            "BGScriptRegistry: Created script '{}' for map {} (total: {})",
            it->second.name, mapId, it->second.createCount);
    }

    return script;
}

std::vector<uint32> BGScriptRegistry::GetRegisteredMapIds() const
{
    std::shared_lock lock(m_mutex);

    std::vector<uint32> result;
    result.reserve(m_scripts.size());

    for (const auto& [mapId, entry] : m_scripts)
    {
        result.push_back(mapId);
    }

    return result;
}

std::string BGScriptRegistry::GetScriptName(uint32 mapId) const
{
    std::shared_lock lock(m_mutex);

    auto it = m_scripts.find(mapId);
    if (it == m_scripts.end())
        return "";

    return it->second.name;
}

size_t BGScriptRegistry::GetScriptCount() const
{
    std::shared_lock lock(m_mutex);
    return m_scripts.size();
}

void BGScriptRegistry::LogRegisteredScripts() const
{
    std::shared_lock lock(m_mutex);

    TC_LOG_INFO("playerbots.bg.script",
        "BGScriptRegistry: {} registered scripts:", m_scripts.size());

    for (const auto& [mapId, entry] : m_scripts)
    {
        TC_LOG_INFO("playerbots.bg.script",
            "  - Map {}: {} (created {} times)",
            mapId, entry.name, entry.createCount);
    }
}

std::string BGScriptRegistry::GetStatistics() const
{
    std::shared_lock lock(m_mutex);

    std::ostringstream ss;
    ss << "BGScriptRegistry Statistics:\n";
    ss << "  Total scripts: " << m_scripts.size() << "\n";
    ss << "  Registered battlegrounds:\n";

    uint32 totalCreated = 0;
    for (const auto& [mapId, entry] : m_scripts)
    {
        ss << "    - " << entry.name << " (Map " << mapId << "): "
           << entry.createCount << " instances\n";
        totalCreated += entry.createCount;
    }

    ss << "  Total scripts created: " << totalCreated;

    return ss.str();
}

} // namespace Playerbot::Coordination::Battleground
