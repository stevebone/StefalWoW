/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_BOT_AI_QUEST_INTEGRATION_H
#define TRINITYCORE_BOT_AI_QUEST_INTEGRATION_H

#include "BotAI.h"
#include "../Game/QuestManager.h"
#include <memory>

namespace Playerbot
{
    /**
     * BotAI Extension for Quest Manager Integration
     *
     * This header provides the integration point for QuestManager into BotAI.
     * Add the following to BotAI class declaration in BotAI.h:
     *
     * In protected members section:
     *     // Game system managers
     *     std::unique_ptr<QuestManager> _questManager;
     *
     * In public methods section:
     *     QuestManager* GetQuestManager() { return _questManager.get(); }
     *     QuestManager const* GetQuestManager() const { return _questManager.get(); }
     */

    /**
     * Integration methods to add to BotAI.cpp
     */
    class BotAIQuestIntegration
    {
    public:
        /**
         * Initialize quest manager - call this in BotAI constructor
         */
        static void InitializeQuestManager(BotAI* ai, Player* bot)
        {
            if (!ai || !bot)
                return;

            // Create and initialize quest manager
            // This would be added to BotAI constructor:
            // _questManager = std::make_unique<QuestManager>(bot, this);
            // _questManager->Initialize();
        }

        /**
         * Update quest manager - call this in UpdateIdleBehaviors
         */
        static void UpdateQuestManager(BotAI* ai, uint32 diff)
        {
            if (!ai)
                return;

            // This would be added to UpdateIdleBehaviors:
            // if (_questManager && _questManager->IsEnabled())
            // {
            //     _questManager->Update(diff);
            // }
        }

        /**
         * Shutdown quest manager - call this in BotAI destructor
         */
        static void ShutdownQuestManager(BotAI* ai)
        {
            if (!ai)
                return;

            // This would be added to BotAI destructor:
            // if (_questManager)
            // {
            //     _questManager->Shutdown();
            // }
        }

        /**
         * Example integration code for UpdateIdleBehaviors
         *
         * Replace the existing quest automation code with:
         *
         * void BotAI::UpdateIdleBehaviors(uint32 diff)
         * {
         *     // Only run idle behaviors when truly idle
         *     if (IsInCombat() || IsFollowing())
         *         return;
         *
         *     // Update quest manager with intelligent throttling
         *     if (_questManager && _questManager->IsEnabled())
         *     {
         *         _questManager->Update(diff);
         *     }
         *
         *     // ... other idle behaviors ...
         * }
         */
    };

} // namespace Playerbot

#endif // TRINITYCORE_BOT_AI_QUEST_INTEGRATION_H