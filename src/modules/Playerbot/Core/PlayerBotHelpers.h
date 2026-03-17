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

class Player;

namespace Playerbot
{

class BotAI;
class IGameSystemsManager;

/**
 * @brief Helper function to safely get BotAI from a Player
 *
 * This helper safely casts Player's AI to BotAI, returning nullptr if:
 * - Player is null
 * - Player has no AI
 * - Player's AI is not a BotAI instance
 *
 * @param player The player to get BotAI from
 * @return BotAI* if player is a valid bot, nullptr otherwise
 *
 * @example
 * if (BotAI* botAI = GetBotAI(player))
 * {
 *     // Safe to use botAI here
 *     botAI->GetGameSystems()->GetQuestManager()->DoSomething();
 * }
 */
BotAI* GetBotAI(Player* player);

/**
 * @brief Helper function to safely get GameSystemsManager from a Player
 *
 * This is a convenience wrapper that combines GetBotAI + GetGameSystems.
 * Returns nullptr if player is not a valid bot.
 *
 * @param player The player to get GameSystemsManager from
 * @return IGameSystemsManager* if player is a valid bot, nullptr otherwise
 *
 * @example
 * if (auto* gameSystems = GetGameSystems(player))
 * {
 *     gameSystems->GetQuestManager()->AcceptQuest(questId);
 * }
 */
IGameSystemsManager* GetGameSystems(Player* player);

} // namespace Playerbot
