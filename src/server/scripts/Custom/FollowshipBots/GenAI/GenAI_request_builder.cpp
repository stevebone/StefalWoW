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

#include "GenAI_request_builder.h"
#include "GenAI_provider.h"
#include "Errors.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace FSBGenAI
{
    namespace
    {
        class OpenAIRequestBuilder : public IGenAIRequestBuilder
        {
        public:
            std::string BuildChatRequest(
                std::string const& model,
                int32 maxTokens,
                float temperature,
                float topP,
                float frequencyPenalty,
                float presencePenalty,
                std::string const& systemPrompt,
                std::string const& userMessage,
                bool supportsPenalties) const override
            {
                return BuildRequest(model, maxTokens, temperature, topP, frequencyPenalty, presencePenalty,
                    systemPrompt, userMessage, false, supportsPenalties, false);
            }

            std::string BuildStructuredRequest(
                std::string const& model,
                int32 maxTokens,
                float temperature,
                float topP,
                float frequencyPenalty,
                float presencePenalty,
                std::string const& systemPrompt,
                std::string const& userMessage,
                bool supportsPenalties,
                bool supportsResponseFormat) const override
            {
                return BuildRequest(model, maxTokens, temperature, topP, frequencyPenalty, presencePenalty,
                    systemPrompt, userMessage, true, supportsPenalties, supportsResponseFormat);
            }

        private:
            std::string BuildRequest(
                std::string const& model,
                int32 maxTokens,
                float temperature,
                float topP,
                float frequencyPenalty,
                float presencePenalty,
                std::string const& systemPrompt,
                std::string const& userMessage,
                bool isStructured,
                bool supportsPenalties,
                bool supportsResponseFormat) const
            {
                rapidjson::Document doc;
                doc.SetObject();
                rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

                rapidjson::Value modelValue;
                modelValue.SetString(model.c_str(), static_cast<rapidjson::SizeType>(model.length()), allocator);
                doc.AddMember("model", modelValue, allocator);

                doc.AddMember("stream", false, allocator);
                doc.AddMember("max_tokens", maxTokens, allocator);
                if (temperature != -1.0f)
                    doc.AddMember("temperature", temperature, allocator);
                if (topP != -1.0f)
                    doc.AddMember("top_p", topP, allocator);
                if (supportsPenalties)
                {
                    doc.AddMember("frequency_penalty", frequencyPenalty, allocator);
                    doc.AddMember("presence_penalty", presencePenalty, allocator);
                }

                if (isStructured && supportsResponseFormat)
                {
                    rapidjson::Value responseFormat(rapidjson::kObjectType);
                    rapidjson::Value typeValue;
                    typeValue.SetString("json_object", allocator);
                    responseFormat.AddMember("type", typeValue, allocator);
                    doc.AddMember("response_format", responseFormat, allocator);
                }

                rapidjson::Value messages(rapidjson::kArrayType);

                rapidjson::Value systemMsg(rapidjson::kObjectType);
                rapidjson::Value systemRole;
                systemRole.SetString("system", allocator);
                rapidjson::Value systemContent;
                systemContent.SetString(systemPrompt.c_str(), static_cast<rapidjson::SizeType>(systemPrompt.length()), allocator);
                systemMsg.AddMember("role", systemRole, allocator);
                systemMsg.AddMember("content", systemContent, allocator);
                messages.PushBack(systemMsg, allocator);

                rapidjson::Value userMsg(rapidjson::kObjectType);
                rapidjson::Value userRole;
                userRole.SetString("user", allocator);
                rapidjson::Value userContent;
                userContent.SetString(userMessage.c_str(), static_cast<rapidjson::SizeType>(userMessage.length()), allocator);
                userMsg.AddMember("role", userRole, allocator);
                userMsg.AddMember("content", userContent, allocator);
                messages.PushBack(userMsg, allocator);

                doc.AddMember("messages", messages, allocator);

                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                doc.Accept(writer);

                return std::string(buffer.GetString(), buffer.GetSize());
            }
        };

        class AnthropicRequestBuilder : public IGenAIRequestBuilder
        {
        public:
            std::string BuildChatRequest(
                std::string const& model,
                int32 maxTokens,
                float temperature,
                float topP,
                float /*frequencyPenalty*/,
                float /*presencePenalty*/,
                std::string const& systemPrompt,
                std::string const& userMessage,
                bool /*supportsPenalties*/) const override
            {
                return BuildRequest(model, maxTokens, temperature, topP, systemPrompt, userMessage);
            }

            std::string BuildStructuredRequest(
                std::string const& model,
                int32 maxTokens,
                float temperature,
                float topP,
                float /*frequencyPenalty*/,
                float /*presencePenalty*/,
                std::string const& systemPrompt,
                std::string const& userMessage,
                bool /*supportsPenalties*/,
                bool /*supportsResponseFormat*/) const override
            {
                return BuildRequest(model, maxTokens, temperature, topP, systemPrompt, userMessage);
            }

        private:
            std::string BuildRequest(
                std::string const& model,
                int32 maxTokens,
                float temperature,
                float topP,
                std::string const& systemPrompt,
                std::string const& userMessage) const
            {
                rapidjson::Document doc;
                doc.SetObject();
                rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

                rapidjson::Value modelValue;
                modelValue.SetString(model.c_str(), static_cast<rapidjson::SizeType>(model.length()), allocator);
                doc.AddMember("model", modelValue, allocator);

                doc.AddMember("stream", false, allocator);
                doc.AddMember("max_tokens", maxTokens, allocator);
                if (temperature != -1.0f)
                    doc.AddMember("temperature", temperature, allocator);
                if (topP != -1.0f)
                    doc.AddMember("top_p", topP, allocator);

                rapidjson::Value systemValue;
                systemValue.SetString(systemPrompt.c_str(), static_cast<rapidjson::SizeType>(systemPrompt.length()), allocator);
                doc.AddMember("system", systemValue, allocator);

                rapidjson::Value messages(rapidjson::kArrayType);

                rapidjson::Value userMsg(rapidjson::kObjectType);
                rapidjson::Value userRole;
                userRole.SetString("user", allocator);
                rapidjson::Value userContent;
                userContent.SetString(userMessage.c_str(), static_cast<rapidjson::SizeType>(userMessage.length()), allocator);
                userMsg.AddMember("role", userRole, allocator);
                userMsg.AddMember("content", userContent, allocator);
                messages.PushBack(userMsg, allocator);

                doc.AddMember("messages", messages, allocator);

                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                doc.Accept(writer);

                return std::string(buffer.GetString(), buffer.GetSize());
            }
        };

        static OpenAIRequestBuilder s_openAIBuilder;
        static AnthropicRequestBuilder s_anthropicBuilder;
    }

    IGenAIRequestBuilder const* GetRequestBuilder(GenAIProvider provider)
    {
        GenAIProviderPreset const& preset = GetPreset(provider);
        switch (preset.requestFormat)
        {
            case GenAIRequestFormat::AnthropicMessages:
                return &s_anthropicBuilder;
            case GenAIRequestFormat::OpenAIChat:
            default:
                return &s_openAIBuilder;
        }
    }
}
