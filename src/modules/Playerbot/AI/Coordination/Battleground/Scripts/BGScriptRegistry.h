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

#ifndef PLAYERBOT_AI_COORDINATION_BG_BGSCRIPTREGISTRY_H
#define PLAYERBOT_AI_COORDINATION_BG_BGSCRIPTREGISTRY_H

#include "IBGScript.h"
#include "Define.h"
#include <functional>
#include <map>
#include <memory>
#include <shared_mutex>
#include <string>
#include <vector>

namespace Playerbot::Coordination::Battleground
{

/**
 * @brief Registry for battleground-specific scripts
 *
 * The BGScriptRegistry is a singleton that manages all registered BG scripts.
 * Scripts are registered at static initialization time using the REGISTER_BG_SCRIPT macro.
 * At runtime, the BattlegroundCoordinator queries this registry to get the appropriate
 * script for a given map ID.
 *
 * Thread-safe for concurrent access.
 *
 * Example registration:
 * @code
 * // In WarsongGulchScript.cpp
 * REGISTER_BG_SCRIPT(WarsongGulchScript, 489);
 * @endcode
 */
class BGScriptRegistry
{
public:
    using ScriptFactory = std::function<std::unique_ptr<IBGScript>()>;

    /**
     * @brief Get the singleton instance
     */
    static BGScriptRegistry& Instance();

    // Delete copy and move
    BGScriptRegistry(const BGScriptRegistry&) = delete;
    BGScriptRegistry& operator=(const BGScriptRegistry&) = delete;
    BGScriptRegistry(BGScriptRegistry&&) = delete;
    BGScriptRegistry& operator=(BGScriptRegistry&&) = delete;

    // ========================================================================
    // REGISTRATION
    // ========================================================================

    /**
     * @brief Register a script factory for a map ID
     * @param mapId The battleground map ID
     * @param factory Factory function to create the script
     * @param name Human-readable name for logging
     * @return true if registered successfully
     */
    bool RegisterScript(uint32 mapId, ScriptFactory factory, const std::string& name = "");

    /**
     * @brief Unregister a script for a map ID
     * @param mapId The battleground map ID
     * @return true if unregistered successfully
     */
    bool UnregisterScript(uint32 mapId);

    /**
     * @brief Register a script for multiple map IDs (for remakes/variants)
     * @param mapIds List of map IDs
     * @param factory Factory function
     * @param name Human-readable name
     */
    void RegisterScriptMultiple(const std::vector<uint32>& mapIds,
        ScriptFactory factory, const std::string& name = "");

    // ========================================================================
    // QUERY
    // ========================================================================

    /**
     * @brief Check if a script exists for a map ID
     * @param mapId The battleground map ID
     */
    bool HasScript(uint32 mapId) const;

    /**
     * @brief Create a script instance for a map ID
     * @param mapId The battleground map ID
     * @return New script instance or nullptr if not registered
     */
    std::unique_ptr<IBGScript> CreateScript(uint32 mapId) const;

    /**
     * @brief Get all registered map IDs
     */
    std::vector<uint32> GetRegisteredMapIds() const;

    /**
     * @brief Get the registered script name for a map ID
     * @param mapId The battleground map ID
     */
    std::string GetScriptName(uint32 mapId) const;

    /**
     * @brief Get count of registered scripts
     */
    size_t GetScriptCount() const;

    // ========================================================================
    // STATISTICS
    // ========================================================================

    /**
     * @brief Log all registered scripts (for debugging)
     */
    void LogRegisteredScripts() const;

    /**
     * @brief Get registry statistics as a string
     */
    std::string GetStatistics() const;

private:
    BGScriptRegistry() = default;
    ~BGScriptRegistry() = default;

    struct ScriptEntry
    {
        ScriptFactory factory;
        std::string name;
        uint32 createCount = 0;
    };

    std::map<uint32, ScriptEntry> m_scripts;
    mutable std::shared_mutex m_mutex;
};

// ============================================================================
// REGISTRATION MACROS
// ============================================================================

/**
 * @brief Register a BG script for a single map ID
 *
 * Usage:
 * @code
 * // At file scope in the .cpp file
 * REGISTER_BG_SCRIPT(WarsongGulchScript, 489);
 * @endcode
 */
#define REGISTER_BG_SCRIPT(ScriptClass, MapId) \
    static struct ScriptClass##_Registrar_##MapId { \
        ScriptClass##_Registrar_##MapId() { \
            ::Playerbot::Coordination::Battleground::BGScriptRegistry::Instance().RegisterScript( \
                MapId, \
                []() -> ::std::unique_ptr<::Playerbot::Coordination::Battleground::IBGScript> { \
                    return ::std::make_unique<ScriptClass>(); \
                }, \
                #ScriptClass); \
        } \
    } g_##ScriptClass##_Registrar_##MapId

/**
 * @brief Register a BG script for multiple map IDs
 *
 * Usage:
 * @code
 * // For battlegrounds with map variants
 * REGISTER_BG_SCRIPT_MULTI(WarsongGulchScript, {489, 2106});
 * @endcode
 */
#define REGISTER_BG_SCRIPT_MULTI(ScriptClass, ...) \
    static struct ScriptClass##_MultiRegistrar { \
        ScriptClass##_MultiRegistrar() { \
            ::std::vector<uint32> ids = __VA_ARGS__; \
            ::Playerbot::Coordination::Battleground::BGScriptRegistry::Instance().RegisterScriptMultiple( \
                ids, \
                []() -> ::std::unique_ptr<::Playerbot::Coordination::Battleground::IBGScript> { \
                    return ::std::make_unique<ScriptClass>(); \
                }, \
                #ScriptClass); \
        } \
    } g_##ScriptClass##_MultiRegistrar

/**
 * @brief Helper to get the registry instance
 */
#define sBGScriptRegistry ::Playerbot::Coordination::Battleground::BGScriptRegistry::Instance()

} // namespace Playerbot::Coordination::Battleground

#endif // PLAYERBOT_AI_COORDINATION_BG_BGSCRIPTREGISTRY_H
