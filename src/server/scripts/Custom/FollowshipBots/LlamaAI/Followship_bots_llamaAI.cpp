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

#include "Followship_bots_llamaAI.h"
#include "Followship_bots_config.h"

#include "Log.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>

#include <openssl/ssl.h>
#include <boost/beast/http.hpp>

#include "Errors.h" // rapidjson depends on WPAssert

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include "Common.h"

namespace FSBLlamaAI
{
    bool IsEnabled()
    {
        return FollowshipBotsConfig::configFSBLlamaAIEnabled;
    }

    std::string BuildChatRequestJson(std::string const& systemPrompt, std::string const& userMessage)
    {
        rapidjson::Document doc;
        doc.SetObject();
        rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

        rapidjson::Value modelValue;
        modelValue.SetString(FollowshipBotsConfig::configFSBLlamaAIModel.c_str(),
                             static_cast<rapidjson::SizeType>(FollowshipBotsConfig::configFSBLlamaAIModel.length()), allocator);
        doc.AddMember("model", modelValue, allocator);

        doc.AddMember("stream", false, allocator);
        doc.AddMember("max_tokens", FollowshipBotsConfig::configFSBLlamaAIMaxTokens, allocator);
        if (FollowshipBotsConfig::configFSBLlamaAITemperature != -1.0f)
            doc.AddMember("temperature", FollowshipBotsConfig::configFSBLlamaAITemperature, allocator);
        if (FollowshipBotsConfig::configFSBLlamaAITopP != -1.0f)
            doc.AddMember("top_p", FollowshipBotsConfig::configFSBLlamaAITopP, allocator);
        if (FollowshipBotsConfig::configFSBLlamaAISendPenalties)
        {
            doc.AddMember("frequency_penalty", FollowshipBotsConfig::configFSBLlamaAIFrequencyPenalty, allocator);
            doc.AddMember("presence_penalty", FollowshipBotsConfig::configFSBLlamaAIPresencePenalty, allocator);
        }

        if (FollowshipBotsConfig::configFSBLlamaAIUseSystemParameter)
        {
            rapidjson::Value systemValue;
            systemValue.SetString(systemPrompt.c_str(), static_cast<rapidjson::SizeType>(systemPrompt.length()), allocator);
            doc.AddMember("system", systemValue, allocator);
        }

        rapidjson::Value messages(rapidjson::kArrayType);

        if (!FollowshipBotsConfig::configFSBLlamaAIUseSystemParameter)
        {
            rapidjson::Value systemMsg(rapidjson::kObjectType);
            rapidjson::Value systemRole;
            systemRole.SetString("system", allocator);
            rapidjson::Value systemContent;
            systemContent.SetString(systemPrompt.c_str(), static_cast<rapidjson::SizeType>(systemPrompt.length()), allocator);
            systemMsg.AddMember("role", systemRole, allocator);
            systemMsg.AddMember("content", systemContent, allocator);
            messages.PushBack(systemMsg, allocator);
        }

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

    namespace
    {
        void CancelSocket(boost::asio::ip::tcp::socket& s) { s.cancel(); }
        void CancelSocket(boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& s) { s.next_layer().cancel(); }

        void ShutdownSocket(boost::asio::ip::tcp::socket& s, boost::beast::error_code& ec)
        {
            s.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        }
        void ShutdownSocket(boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& s, boost::beast::error_code& ec)
        {
            s.shutdown(ec);
        }

        template<typename Stream>
        std::string DoHttpExchange(Stream& stream, boost::asio::io_context& ioContext,
            std::string const& requestBody, std::string const& host,
            std::string const& target, std::string const& apiKey,
            std::string const& customHeader, int32 timeoutMs)
        {
            boost::beast::http::request<boost::beast::http::string_body> req;
            req.method(boost::beast::http::verb::post);
            req.target(target);
            req.version(11);
            req.set(boost::beast::http::field::host, host);
            req.set(boost::beast::http::field::content_type, "application/json");
            req.set(boost::beast::http::field::accept, "application/json");

            if (!apiKey.empty())
            {
                std::string const& apiKeyHeader = FollowshipBotsConfig::configFSBLlamaAIApiKeyHeader;
                std::string const& apiKeyPrefix = FollowshipBotsConfig::configFSBLlamaAIApiKeyPrefix;
                std::string headerValue = apiKeyPrefix + apiKey;
                if (apiKeyHeader == "Authorization")
                    req.set(boost::beast::http::field::authorization, headerValue);
                else
                    req.set(apiKeyHeader, headerValue);
            }

            if (!customHeader.empty())
            {
                auto pos = customHeader.find(':');
                if (pos != std::string::npos)
                {
                    std::string name = customHeader.substr(0, pos);
                    std::string value = customHeader.substr(pos + 1);
                    auto valStart = value.find_first_not_of(" \t");
                    if (valStart != std::string::npos)
                        value = value.substr(valStart);
                    if (!name.empty())
                        req.set(name, value);
                }
            }

            req.body() = requestBody;
            req.prepare_payload();

            boost::beast::http::write(stream, req);

            boost::beast::flat_buffer buffer;
            boost::beast::http::response<boost::beast::http::string_body> res;

            boost::asio::steady_timer timer(ioContext);
            timer.expires_after(std::chrono::milliseconds(timeoutMs));

            bool timedOut = false;
            boost::system::error_code readEc;

            timer.async_wait([&stream, &timedOut](boost::system::error_code ec) {
                if (!ec) // timer expired, not cancelled
                {
                    timedOut = true;
                    CancelSocket(stream);
                }
            });

            boost::beast::http::async_read(stream, buffer, res,
                [&timer, &readEc](boost::system::error_code ec, std::size_t) {
                    readEc = ec;
                    timer.cancel();
                });

            ioContext.run();

            if (timedOut)
            {
                TC_LOG_ERROR("scripts.fsb.llama", "FSB LlamaAI: request timed out after {}ms", timeoutMs);
                boost::beast::error_code closeEc;
                ShutdownSocket(stream, closeEc);
                return {};
            }

            if (readEc)
            {
                TC_LOG_ERROR("scripts.fsb.llama", "FSB LlamaAI: HTTP read error: {}", readEc.message());
                boost::beast::error_code closeEc;
                ShutdownSocket(stream, closeEc);
                return {};
            }

            if (res.result() != boost::beast::http::status::ok)
            {
                TC_LOG_ERROR("scripts.fsb.llama", "FSB LlamaAI: HTTP status {} - body: {}",
                    static_cast<int>(res.result()), res.body());
                boost::beast::error_code closeEc;
                ShutdownSocket(stream, closeEc);
                return {};
            }

            boost::beast::error_code closeEc;
            ShutdownSocket(stream, closeEc);

            return res.body();
        }
    }

    std::string SendChatRequest(std::string const& requestBody)
    {
        try
        {
            TC_LOG_INFO("scripts.fsb.llama", "FSB LlamaAI: sending request...");
            std::string const& host = FollowshipBotsConfig::configFSBLlamaAIHost;
            int32 port = FollowshipBotsConfig::configFSBLlamaAIPort;
            std::string target = "/" + FollowshipBotsConfig::configFSBLlamaAIEndpoint;
            int32 timeoutMs = FollowshipBotsConfig::configFSBLlamaAITimeoutMs;
            bool useSSL = FollowshipBotsConfig::configFSBLlamaAIUseSSL;

            std::string service = (port == -1)
                ? (useSSL ? "https" : "http")
                : std::to_string(port);

            std::string hostHeader = host;
            if (port != -1)
                hostHeader += ":" + std::to_string(port);

            std::string const& apiKey = FollowshipBotsConfig::configFSBLlamaAIApiKey;
            std::string const& customHeader = FollowshipBotsConfig::configFSBLlamaAIRequestHeader;

            TC_LOG_INFO("scripts.fsb.llama", "FSB LlamaAI: connecting to {}://{} (service={})",
                useSSL ? "https" : "http", host, service);

            boost::asio::io_context ioContext;
            boost::asio::ip::tcp::resolver resolver(ioContext);
            auto const results = resolver.resolve(host, service);

            if (useSSL)
            {
                boost::asio::ssl::context sslCtx(boost::asio::ssl::context::tlsv12_client);
                sslCtx.set_verify_mode(boost::asio::ssl::verify_none);

                boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream(ioContext, sslCtx);
                boost::asio::connect(stream.next_layer(), results);

                if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
                {
                    boost::system::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
                    TC_LOG_ERROR("scripts.fsb.llama", "FSB LlamaAI: SSL_set_tlsext_host_name failed: {}", ec.message());
                    return {};
                }

                stream.handshake(boost::asio::ssl::stream_base::client);

                return DoHttpExchange(stream, ioContext, requestBody, hostHeader, target, apiKey, customHeader, timeoutMs);
            }
            else
            {
                boost::asio::ip::tcp::socket socket(ioContext);
                boost::asio::connect(socket, results);

                return DoHttpExchange(socket, ioContext, requestBody, hostHeader, target, apiKey, customHeader, timeoutMs);
            }
        }
        catch (std::exception const& e)
        {
            TC_LOG_ERROR("scripts.fsb.llama", "FSB LlamaAI: Request exception: {}", e.what());
            return {};
        }
    }

    std::string ExtractResponseContent(std::string const& responseJson)
    {
        rapidjson::Document doc;
        doc.Parse(responseJson.c_str(), responseJson.length());

        if (doc.HasParseError() || !doc.IsObject())
        {
            TC_LOG_ERROR("scripts.fsb.llama", "FSB LlamaAI: JSON parse error: {}", responseJson);
            return {};
        }

        // Try OpenAI format first
        if (doc.HasMember("choices") && doc["choices"].IsArray() && !doc["choices"].Empty())
        {
            rapidjson::Value const& choice = doc["choices"][0];
            if (choice.IsObject() && choice.HasMember("message") && choice["message"].IsObject())
            {
                rapidjson::Value const& message = choice["message"];
                if (message.HasMember("content") && message["content"].IsString())
                    return message["content"].GetString();
            }
        }

        // Try Anthropic format
        if (doc.HasMember("content") && doc["content"].IsArray() && !doc["content"].Empty())
        {
            rapidjson::Value const& content = doc["content"][0];
            if (content.IsObject() && content.HasMember("text") && content["text"].IsString())
                return content["text"].GetString();
        }

        TC_LOG_ERROR("scripts.fsb.llama", "FSB LlamaAI: unrecognized response format: {}", responseJson);
        return {};
    }

    std::string GetBotResponse(std::string const& systemPrompt, std::string const& userMessage)
    {
        std::string requestBody = BuildChatRequestJson(systemPrompt, userMessage);
        TC_LOG_DEBUG("scripts.fsb.llama", "FSB LlamaAI: === REQUEST BODY ===\n{}", requestBody);

        std::string responseJson = SendChatRequest(requestBody);
        TC_LOG_DEBUG("scripts.fsb.llama", "FSB LlamaAI: === RESPONSE BODY ===\n{}", responseJson);

        return ExtractResponseContent(responseJson);
    }

    std::string GetStructuredBotResponse(std::string const& systemPrompt, std::string const& userMessage)
    {
        rapidjson::Document doc;
        doc.SetObject();
        rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

        rapidjson::Value modelValue;
        modelValue.SetString(FollowshipBotsConfig::configFSBLlamaAIModel.c_str(),
                             static_cast<rapidjson::SizeType>(FollowshipBotsConfig::configFSBLlamaAIModel.length()), allocator);
        doc.AddMember("model", modelValue, allocator);

        doc.AddMember("stream", false, allocator);
        doc.AddMember("max_tokens", FollowshipBotsConfig::configFSBLlamaAIMaxTokens, allocator);
        if (FollowshipBotsConfig::configFSBLlamaAITemperature != -1.0f)
            doc.AddMember("temperature", FollowshipBotsConfig::configFSBLlamaAITemperature, allocator);
        if (FollowshipBotsConfig::configFSBLlamaAITopP != -1.0f)
            doc.AddMember("top_p", FollowshipBotsConfig::configFSBLlamaAITopP, allocator);
        if (FollowshipBotsConfig::configFSBLlamaAISendPenalties)
        {
            doc.AddMember("frequency_penalty", FollowshipBotsConfig::configFSBLlamaAIFrequencyPenalty, allocator);
            doc.AddMember("presence_penalty", FollowshipBotsConfig::configFSBLlamaAIPresencePenalty, allocator);
        }

        if (!FollowshipBotsConfig::configFSBLlamaAIUseSystemParameter)
        {
            // Add response_format for JSON mode (OpenAI only)
            rapidjson::Value responseFormat(rapidjson::kObjectType);
            rapidjson::Value typeValue;
            typeValue.SetString("json_object", allocator);
            responseFormat.AddMember("type", typeValue, allocator);
            doc.AddMember("response_format", responseFormat, allocator);
        }

        if (FollowshipBotsConfig::configFSBLlamaAIUseSystemParameter)
        {
            rapidjson::Value systemValue;
            systemValue.SetString(systemPrompt.c_str(), static_cast<rapidjson::SizeType>(systemPrompt.length()), allocator);
            doc.AddMember("system", systemValue, allocator);
        }

        rapidjson::Value messages(rapidjson::kArrayType);

        if (!FollowshipBotsConfig::configFSBLlamaAIUseSystemParameter)
        {
            rapidjson::Value systemMsg(rapidjson::kObjectType);
            rapidjson::Value systemRole;
            systemRole.SetString("system", allocator);
            rapidjson::Value systemContent;
            systemContent.SetString(systemPrompt.c_str(), static_cast<rapidjson::SizeType>(systemPrompt.length()), allocator);
            systemMsg.AddMember("role", systemRole, allocator);
            systemMsg.AddMember("content", systemContent, allocator);
            messages.PushBack(systemMsg, allocator);
        }

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

        std::string requestBody(buffer.GetString(), buffer.GetSize());
        TC_LOG_DEBUG("scripts.fsb.llama", "FSB LlamaAI: === STRUCTURED REQUEST BODY ===\n{}", requestBody);

        std::string responseJson = SendChatRequest(requestBody);
        TC_LOG_DEBUG("scripts.fsb.llama", "FSB LlamaAI: === STRUCTURED RESPONSE BODY ===\n{}", responseJson);

        return ExtractResponseContent(responseJson);
    }

    bool ParseStructuredResponse(std::string const& jsonStr, std::string& reply, std::string& action, uint32& amount)
    {
        reply.clear();
        action = "none";
        amount = 0;

        if (jsonStr.empty())
            return false;

        rapidjson::Document doc;
        doc.Parse(jsonStr.c_str(), jsonStr.length());

        if (doc.HasParseError() || !doc.IsObject())
        {
            TC_LOG_ERROR("scripts.fsb.llama", "FSB LlamaAI: structured JSON parse error: {}", jsonStr);
            return false;
        }

        if (!doc.HasMember("reply") || !doc["reply"].IsString())
        {
            TC_LOG_ERROR("scripts.fsb.llama", "FSB LlamaAI: missing/invalid 'reply' field: {}", jsonStr);
            return false;
        }

        reply = doc["reply"].GetString();

        if (doc.HasMember("action") && doc["action"].IsString())
            action = doc["action"].GetString();

        if (doc.HasMember("amount") && doc["amount"].IsUint())
            amount = doc["amount"].GetUint();
        else if (doc.HasMember("amount") && doc["amount"].IsInt() && doc["amount"].GetInt() >= 0)
            amount = static_cast<uint32>(doc["amount"].GetInt());

        return true;
    }
}
