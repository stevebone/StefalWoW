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

#include "BotAI.h"
#include "HybridAIController.h"
#include "Log.h"

namespace Playerbot
{

// ============================================================================
// HYBRID AI INITIALIZATION
// ============================================================================

void BotAI::InitializeHybridAI()
{
    TC_LOG_DEBUG("playerbot.ai", "Bot {} - Initializing Hybrid AI Decision System (Utility AI + Behavior Trees)",
        _bot->GetName());

    // HybridAI is now managed by GameSystemsManager - just initialize through facade
    if (auto* hybridAI = GetHybridAI())
    {
        hybridAI->Initialize();
        TC_LOG_INFO("playerbot.ai", "Bot {} - Hybrid AI initialized successfully with SharedBlackboard integration", _bot->GetName());
    }
    else
    {
        TC_LOG_ERROR("playerbot.ai", "Bot {} - Failed to get HybridAI from GameSystemsManager", _bot->GetName());
    }
}

// ============================================================================
// HYBRID AI UPDATE
// ============================================================================

void BotAI::UpdateHybridAI(uint32 diff)
{
    // Update Hybrid AI (Utility AI decision + Behavior Tree execution) through facade
    if (auto* hybridAI = GetHybridAI())
    {
        bool success = hybridAI->Update(diff);

        if (!success)
        {
            TC_LOG_TRACE("playerbot.ai", "Bot {} - Hybrid AI update returned false", _bot->GetName());
        }

        // Log behavior transitions
        if (hybridAI->BehaviorChangedThisFrame())
        {
            TC_LOG_DEBUG("playerbot.ai", "Bot {} switched to behavior: {} (tree status: {})",
                _bot->GetName(),
                hybridAI->GetCurrentBehaviorName(),
                uint32(hybridAI->GetCurrentTreeStatus()));
        }
    }
}

} // namespace Playerbot
