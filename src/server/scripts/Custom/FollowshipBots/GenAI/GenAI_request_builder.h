/*
 * This file is part of the Stefal WoW Project.
 * It is designed to work exclusively with the TrinityCore framework.
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
 * This code is provided for personal and educational use within the
 * Stefal WoW Project. It is not intended for commercial distribution,
 * resale, or any form of monetization.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <string>
#include "GenAI_provider.h"

namespace FSBGenAI
{
    class IGenAIRequestBuilder
    {
    public:
        virtual std::string BuildChatRequest(
            std::string const& model,
            int32 maxTokens,
            float temperature,
            float topP,
            float frequencyPenalty,
            float presencePenalty,
            std::string const& systemPrompt,
            std::string const& userMessage,
            bool supportsPenalties) const = 0;

        virtual std::string BuildStructuredRequest(
            std::string const& model,
            int32 maxTokens,
            float temperature,
            float topP,
            float frequencyPenalty,
            float presencePenalty,
            std::string const& systemPrompt,
            std::string const& userMessage,
            bool supportsPenalties,
            bool supportsResponseFormat) const = 0;

        virtual ~IGenAIRequestBuilder() = default;
    };

    IGenAIRequestBuilder const* GetRequestBuilder(GenAIProvider provider);
}
