/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "DungeonScriptMgr.h"
#include "EncounterStrategy.h"
#include "Log.h"

namespace Playerbot
{

// ============================================================================
// SINGLETON
// ============================================================================

DungeonScriptMgr* DungeonScriptMgr::instance()
{
    static DungeonScriptMgr instance;
    return &instance;
}

DungeonScriptMgr::DungeonScriptMgr() : _initialized(false)
{
    TC_LOG_INFO("playerbot", "DungeonScriptMgr initialized");
}

DungeonScriptMgr::~DungeonScriptMgr()
{
    // unique_ptr handles cleanup automatically
    _mapScripts.clear();
    _bossScripts.clear();
    _namedScripts.clear();
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void DungeonScriptMgr::Initialize()
{
    if (_initialized)
        return;

    ::std::lock_guard lock(_mutex);

    TC_LOG_INFO("playerbot", "DungeonScriptMgr: Initializing dungeon script system...");

    // Load all scripts
    LoadScripts();

    _initialized = true;

    TC_LOG_INFO("playerbot", "DungeonScriptMgr: Loaded {} dungeon scripts with {} boss mappings",
        _scriptCount.load(), _bossMappingCount.load());
}

void DungeonScriptMgr::LoadScripts()
{
    // Scripts are loaded via DungeonScriptLoader
    // This method is called by Initialize() but actual registration
    // happens in individual AddSC_*() functions

    TC_LOG_DEBUG("playerbot", "DungeonScriptMgr: Script loading initiated");
}

// ============================================================================
// SCRIPT REGISTRATION
// ============================================================================

void DungeonScriptMgr::RegisterScript(DungeonScript* script)
{

    ::std::lock_guard lock(_mutex);

    uint32 mapId = script->GetMapId();
    char const* name = script->GetName();

    // Check for duplicate registration (unique_ptr auto-deletes old script)
    if (_mapScripts.count(mapId))
    {
        TC_LOG_WARN("playerbot", "DungeonScriptMgr: Script '{}' already registered for map {}, overwriting",
            name, mapId);
        // No manual delete needed - unique_ptr will auto-delete when replaced
    }

    // Register by map ID (take ownership with unique_ptr)
    _mapScripts[mapId] = ::std::unique_ptr<DungeonScript>(script);

    // Register by name (non-owning raw pointer for lookup)
    _namedScripts[name] = script;

    _scriptCount++;

    TC_LOG_INFO("playerbot", "DungeonScriptMgr: Registered script '{}' for map {}",
        name, mapId);
}

void DungeonScriptMgr::RegisterBossScript(uint32 bossEntry, DungeonScript* script)
{

    ::std::lock_guard lock(_mutex);

    // Check for duplicate
    if (_bossScripts.count(bossEntry))
    {
        TC_LOG_WARN("playerbot", "DungeonScriptMgr: Boss {} already has script registered, overwriting",
            bossEntry);
    }

    _bossScripts[bossEntry] = script;
    _bossMappingCount++;

    TC_LOG_DEBUG("playerbot", "DungeonScriptMgr: Registered boss {} to script '{}'",
        bossEntry, script->GetName());
}

// ============================================================================
// SCRIPT LOOKUP
// ============================================================================

DungeonScript* DungeonScriptMgr::GetScriptForMap(uint32 mapId) const
{
    ::std::lock_guard lock(_mutex);

    auto it = _mapScripts.find(mapId);
    if (it != _mapScripts.end())
    {
        _scriptHits++;
        return it->second.get();  // Return raw pointer from unique_ptr
    }

    _scriptMisses++;
    return nullptr;
}

DungeonScript* DungeonScriptMgr::GetScriptForBoss(uint32 bossEntry) const
{
    ::std::lock_guard lock(_mutex);

    auto it = _bossScripts.find(bossEntry);
    if (it != _bossScripts.end())
    {
        _scriptHits++;
        return it->second;
    }

    _scriptMisses++;
    return nullptr;
}

bool DungeonScriptMgr::HasScriptForMap(uint32 mapId) const
{
    ::std::lock_guard lock(_mutex);
    return _mapScripts.count(mapId) > 0;
}

bool DungeonScriptMgr::HasScriptForBoss(uint32 bossEntry) const
{
    ::std::lock_guard lock(_mutex);
    return _bossScripts.count(bossEntry) > 0;
}

// ============================================================================
// MECHANIC EXECUTION (with fallback)
// ============================================================================

void DungeonScriptMgr::ExecuteBossMechanic(::Player* player, ::Creature* boss,
    MechanicType mechanic)
{
    if (!player || !boss)
        return;

    _mechanicExecutions++;

    // Step 1: Try to get boss-specific script
    DungeonScript* script = GetScriptForBoss(boss->GetEntry());

    // Step 2: If no boss script, try map script
    if (!script)
        script = GetScriptForMap(player->GetMapId());
    if (script)
    {
        // Script exists - delegate to it
        // Script's virtual method may call generic or implement custom logic
    switch (mechanic)
        {
            case MechanicType::INTERRUPT:
                script->HandleInterruptPriority(player, boss);
                break;

            case MechanicType::GROUND_AVOID:
                script->HandleGroundAvoidance(player, boss);
                break;

            case MechanicType::ADD_PRIORITY:
                script->HandleAddPriority(player, boss);
                break;

            case MechanicType::POSITIONING:
                script->HandlePositioning(player, boss);
                break;

            case MechanicType::DISPEL:
                script->HandleDispelMechanic(player, boss);
                break;

            case MechanicType::MOVEMENT:
                script->HandleMovementMechanic(player, boss);
                break;

            case MechanicType::TANK_SWAP:
                script->HandleTankSwap(player, boss);
                break;

            case MechanicType::SPREAD:
                script->HandleSpreadMechanic(player, boss);
                break;

            case MechanicType::STACK:
                script->HandleStackMechanic(player, boss);
                break;

            default:
                TC_LOG_WARN("playerbot", "DungeonScriptMgr: Unknown mechanic type {}",
                    static_cast<uint32>(mechanic));
                break;
        }
    }
    else
    {
        // Step 3: No script found - use generic fallback DIRECTLY
        TC_LOG_DEBUG("playerbot",
            "DungeonScriptMgr: No script for boss {} (map {}) - using generic mechanics",
            boss->GetEntry(), player->GetMapId());

        switch (mechanic)
        {
            case MechanicType::INTERRUPT:
                EncounterStrategy::HandleGenericInterrupts(player, boss);
                break;

            case MechanicType::GROUND_AVOID:
                EncounterStrategy::HandleGenericGroundAvoidance(player, boss);
                break;

            case MechanicType::ADD_PRIORITY:
                EncounterStrategy::HandleGenericAddPriority(player, boss);
                break;

            case MechanicType::POSITIONING:
                EncounterStrategy::HandleGenericPositioning(player, boss);
                break;

            case MechanicType::DISPEL:
                EncounterStrategy::HandleGenericDispel(player, boss);
                break;

            case MechanicType::MOVEMENT:
                EncounterStrategy::HandleGenericMovement(player, boss);
                break;

            case MechanicType::TANK_SWAP:
                // No generic tank swap
                TC_LOG_DEBUG("playerbot", "DungeonScriptMgr: No tank swap for boss {}",
                    boss->GetEntry());
                break;

            case MechanicType::SPREAD:
                EncounterStrategy::HandleGenericSpread(player, boss, 10.0f);
                break;

            case MechanicType::STACK:
                EncounterStrategy::HandleGenericStack(player, boss);
                break;

            default:
                break;
        }
    }
}

// ============================================================================
// STATISTICS
// ============================================================================

DungeonScriptMgr::ScriptStats DungeonScriptMgr::GetStats() const
{
    ScriptStats stats;
    stats.scriptsRegistered = _scriptCount.load();
    stats.bossMappings = _bossMappingCount.load();
    stats.scriptHits = _scriptHits.load();
    stats.scriptMisses = _scriptMisses.load();
    stats.mechanicExecutions = _mechanicExecutions.load();
    return stats;
}

// ============================================================================
// DEBUGGING
// ============================================================================

void DungeonScriptMgr::ListAllScripts() const
{
    ::std::lock_guard lock(_mutex);

    TC_LOG_INFO("playerbot", "DungeonScriptMgr: === Registered Scripts ===");

    for (auto const& pair : _mapScripts)
    {
        TC_LOG_INFO("playerbot", "  Map {}: '{}'", pair.first, pair.second->GetName());
    }

    TC_LOG_INFO("playerbot", "DungeonScriptMgr: === Boss Mappings ===");

    for (auto const& pair : _bossScripts)
    {
        TC_LOG_INFO("playerbot", "  Boss {}: '{}'", pair.first, pair.second->GetName());
    }

    TC_LOG_INFO("playerbot", "DungeonScriptMgr: Total: {} scripts, {} boss mappings",
        _scriptCount.load(), _bossMappingCount.load());
}

DungeonScript* DungeonScriptMgr::GetScriptByName(::std::string const& name) const
{
    ::std::lock_guard lock(_mutex);

    auto it = _namedScripts.find(name);
    if (it != _namedScripts.end())
        return it->second;

    return nullptr;
}

} // namespace Playerbot
