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
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include "Errors.h" // rapidjson depends on WPAssert

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

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

    std::string SendChatRequest(std::string const& requestBody)
    {
        try
        {
            TC_LOG_INFO("scripts.fsb.llama", "FSB LlamaAI: sending request...");
            std::string const& host = FollowshipBotsConfig::configFSBLlamaAIHost;
            int32 port = FollowshipBotsConfig::configFSBLlamaAIPort;
            std::string target = "/" + FollowshipBotsConfig::configFSBLlamaAIEndpoint;
            int32 timeoutMs = FollowshipBotsConfig::configFSBLlamaAITimeoutMs;

            boost::asio::io_context ioContext;
            boost::asio::ip::tcp::resolver resolver(ioContext);
            auto const results = resolver.resolve(host, std::to_string(port));

            boost::asio::ip::tcp::socket socket(ioContext);
            boost::asio::connect(socket, results);

            boost::beast::http::request<boost::beast::http::string_body> req;
            req.method(boost::beast::http::verb::post);
            req.target(target);
            req.version(11);
            req.set(boost::beast::http::field::host, host);
            req.set(boost::beast::http::field::content_type, "application/json");
            req.set(boost::beast::http::field::accept, "application/json");

            std::string const& apiKey = FollowshipBotsConfig::configFSBLlamaAIApiKey;
            if (!apiKey.empty())
                req.set(boost::beast::http::field::authorization, "Bearer " + apiKey);

            req.body() = requestBody;
            req.prepare_payload();

            boost::beast::http::write(socket, req);

            boost::beast::flat_buffer buffer;
            boost::beast::http::response<boost::beast::http::string_body> res;

            boost::asio::steady_timer timer(ioContext);
            timer.expires_after(std::chrono::milliseconds(timeoutMs));

            bool timedOut = false;
            boost::system::error_code readEc;

            timer.async_wait([&socket, &timedOut](boost::system::error_code ec) {
                if (!ec) // timer expired, not cancelled
                {
                    timedOut = true;
                    socket.cancel();
                }
            });

            boost::beast::http::async_read(socket, buffer, res,
                [&timer, &readEc](boost::system::error_code ec, std::size_t) {
                    readEc = ec;
                    timer.cancel();
                });

            ioContext.run();

            if (timedOut)
            {
                TC_LOG_ERROR("scripts.fsb.llama", "FSB LlamaAI: request timed out after {}ms", timeoutMs);
                boost::beast::error_code closeEc;
                socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, closeEc);
                return {};
            }

            if (readEc)
            {
                TC_LOG_ERROR("scripts.fsb.llama", "FSB LlamaAI: HTTP read error: {}", readEc.message());
                boost::beast::error_code closeEc;
                socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, closeEc);
                return {};
            }

            if (res.result() != boost::beast::http::status::ok)
            {
                TC_LOG_ERROR("scripts.fsb.llama", "FSB LlamaAI: HTTP status {} - body: {}",
                    static_cast<int>(res.result()), res.body());
                boost::beast::error_code closeEc;
                socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, closeEc);
                return {};
            }

            boost::beast::error_code closeEc;
            socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, closeEc);

            return res.body();
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

        if (!doc.HasMember("choices") || !doc["choices"].IsArray() || doc["choices"].Empty())
        {
            TC_LOG_ERROR("scripts.fsb.llama", "FSB LlamaAI: missing/empty choices: {}", responseJson);
            return {};
        }

        rapidjson::Value const& choice = doc["choices"][0];
        if (!choice.IsObject() || !choice.HasMember("message") || !choice["message"].IsObject())
        {
            TC_LOG_ERROR("scripts.fsb.llama", "FSB LlamaAI: invalid choice structure: {}", responseJson);
            return {};
        }

        rapidjson::Value const& message = choice["message"];
        if (!message.HasMember("content") || !message["content"].IsString())
        {
            TC_LOG_ERROR("scripts.fsb.llama", "FSB LlamaAI: missing/invalid content: {}", responseJson);
            return {};
        }

        return message["content"].GetString();
    }

    std::string GetBotResponse(std::string const& systemPrompt, std::string const& userMessage)
    {
        std::string requestBody = BuildChatRequestJson(systemPrompt, userMessage);
        TC_LOG_DEBUG("scripts.fsb.llama", "FSB LlamaAI: === REQUEST BODY ===\n{}", requestBody);

        std::string responseJson = SendChatRequest(requestBody);
        TC_LOG_DEBUG("scripts.fsb.llama", "FSB LlamaAI: === RESPONSE BODY ===\n{}", responseJson);

        return ExtractResponseContent(responseJson);
    }
}
