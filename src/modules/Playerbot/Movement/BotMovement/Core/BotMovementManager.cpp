/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "BotMovementManager.h"
#include "BotMovementController.h"
#include "Unit.h"
#include "Log.h"

BotMovementManager::BotMovementManager()
{
    _config.Load();
}

BotMovementManager::~BotMovementManager()
{
    for (auto const& [guid, controller] : _controllers)
        delete controller;

    _controllers.clear();
}

BotMovementManager* BotMovementManager::Instance()
{
    static BotMovementManager instance;
    return &instance;
}

BotMovementController* BotMovementManager::GetControllerForUnit(Unit* unit)
{
    if (!unit)
        return nullptr;

    auto itr = _controllers.find(unit->GetGUID());
    if (itr != _controllers.end())
        return itr->second;

    return nullptr;
}

BotMovementController* BotMovementManager::RegisterController(Unit* unit)
{
    if (!unit)
    {
        TC_LOG_ERROR("movement.bot", "BotMovementManager::RegisterController - Attempted to register null unit");
        return nullptr;
    }

    ObjectGuid guid = unit->GetGUID();

    auto itr = _controllers.find(guid);
    if (itr != _controllers.end())
    {
        TC_LOG_WARN("movement.bot", "BotMovementManager::RegisterController - Controller already exists for unit {}", guid.ToString());
        return itr->second;
    }

    BotMovementController* controller = new BotMovementController(unit);
    _controllers[guid] = controller;

    TC_LOG_DEBUG("movement.bot", "BotMovementManager::RegisterController - Registered controller for unit {}", guid.ToString());

    return controller;
}

void BotMovementManager::UnregisterController(Unit* unit)
{
    if (!unit)
        return;

    UnregisterController(unit->GetGUID());
}

void BotMovementManager::UnregisterController(ObjectGuid const& guid)
{
    auto itr = _controllers.find(guid);
    if (itr == _controllers.end())
        return;

    TC_LOG_DEBUG("movement.bot", "BotMovementManager::UnregisterController - Unregistering controller for unit {}", guid.ToString());

    delete itr->second;
    _controllers.erase(itr);
}

void BotMovementManager::ReloadConfig()
{
    TC_LOG_INFO("movement.bot", "BotMovementManager::ReloadConfig - Reloading bot movement configuration");
    _config.Reload();
    _globalCache.Clear();
}

MovementMetrics BotMovementManager::GetGlobalMetrics() const
{
    return _metrics;
}

void BotMovementManager::ResetMetrics()
{
    TC_LOG_INFO("movement.bot", "BotMovementManager::ResetMetrics - Resetting global movement metrics");
    _metrics.Reset();
}
