/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PlayerBotHelpers.h"
#include "Player.h"
#include "UnitAI.h"
#include "../AI/BotAI.h"
#include "Managers/GameSystemsManager.h"

namespace Playerbot
{

BotAI* GetBotAI(Player* player)
{
    if (!player)
        return nullptr;

    return dynamic_cast<BotAI*>(player->GetAI());
}

IGameSystemsManager* GetGameSystems(Player* player)
{
    BotAI* botAI = GetBotAI(player);
    return botAI ? botAI->GetGameSystems() : nullptr;
}

} // namespace Playerbot
