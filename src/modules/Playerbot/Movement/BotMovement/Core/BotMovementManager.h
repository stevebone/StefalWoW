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

#ifndef TRINITY_BOTMOVEMENTMANAGER_H
#define TRINITY_BOTMOVEMENTMANAGER_H

#include "Define.h"
#include "ObjectGuid.h"
#include "BotMovementConfig.h"
#include "MovementMetrics.h"
#include "../Pathfinding/PathCache.h"
#include <unordered_map>

class BotMovementController;
class Unit;

class TC_GAME_API BotMovementManager
{
private:
    BotMovementManager();
    ~BotMovementManager();

public:
    BotMovementManager(BotMovementManager const&) = delete;
    BotMovementManager(BotMovementManager&&) = delete;
    BotMovementManager& operator=(BotMovementManager const&) = delete;
    BotMovementManager& operator=(BotMovementManager&&) = delete;

    static BotMovementManager* Instance();

    BotMovementController* GetControllerForUnit(Unit* unit);
    BotMovementController* RegisterController(Unit* unit);
    void UnregisterController(Unit* unit);
    void UnregisterController(ObjectGuid const& guid);

    BotMovementConfig const& GetConfig() const { return _config; }
    void ReloadConfig();

    PathCache* GetPathCache() { return &_globalCache; }

    MovementMetrics GetGlobalMetrics() const;
    void ResetMetrics();

private:
    BotMovementConfig _config;
    PathCache _globalCache;
    std::unordered_map<ObjectGuid, BotMovementController*> _controllers;
    MovementMetrics _metrics;
};

#define sBotMovementManager BotMovementManager::Instance()

#endif
