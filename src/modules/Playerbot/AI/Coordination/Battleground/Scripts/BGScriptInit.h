/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

namespace Playerbot::Coordination::Battleground
{

/**
 * @brief Force inclusion of BG scripts in static library linking
 *
 * MSVC static library linker discards object files that have no direct references.
 * This function must be called from somewhere in playerbot-core to force the linker
 * to include all BG script object files.
 *
 * Call this from PlayerbotModule initialization.
 */
void InitializeBGScripts();

} // namespace Playerbot::Coordination::Battleground
