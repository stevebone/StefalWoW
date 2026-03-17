/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "GossipHandler.h"
#include "Player.h"
#include "Creature.h"
#include "WorldSession.h"
#include "GossipDef.h"
#include "Log.h"
#include "ObjectMgr.h"
#include <algorithm>
#include <cctype>

namespace Playerbot
{
    GossipHandler::GossipHandler()
    {
        InitializeKeywordMappings();
        LoadKnownPaths();
        m_lastCacheCleanup = ::std::chrono::steady_clock::now();
        m_initialized = true;
    }

    GossipHandler::~GossipHandler()
    {
        ClearCache();
    }

    void GossipHandler::InitializeKeywordMappings()
    {
        // Vendor keywords
        m_serviceKeywords[InteractionType::Vendor] = {
            "buy", "sell", "purchase", "goods", "wares", "supplies", "equipment",
            "repair", "fix", "mend", "broken", "damaged", "trade", "merchant"
        };

        // Trainer keywords
        m_serviceKeywords[InteractionType::Trainer] = {
            "train", "teach", "learn", "skill", "ability", "spell", "talent",
            "instruction", "knowledge", "master", "apprentice", "study"
        };

        // Quest keywords
        m_serviceKeywords[InteractionType::QuestGiver] = {
            "quest", "task", "mission", "help", "need", "problem", "trouble",
            "adventure", "journey", "request", "favor", "duty"
        };

        // Innkeeper keywords
        m_serviceKeywords[InteractionType::Innkeeper] = {
            "inn", "rest", "home", "bind", "hearthstone", "stay", "room",
            "bed", "sleep", "comfort", "hearth"
        };

        // Flight master keywords
        m_serviceKeywords[InteractionType::FlightMaster] = {
            "fly", "flight", "travel", "transport", "gryphon", "wyvern",
            "wind rider", "hippogryph", "bat", "taxi", "destination"
        };

        // Bank keywords
        m_serviceKeywords[InteractionType::Bank] = {
            "bank", "vault", "storage", "deposit", "withdraw", "safe",
            "locker", "account", "savings", "gold", "items"
        };

        // Guild bank keywords
        m_serviceKeywords[InteractionType::GuildBank] = {
            "guild", "guild bank", "guild vault", "guild storage",
            "guild deposit", "guild withdraw"
        };

        // Mailbox keywords
        m_serviceKeywords[InteractionType::Mailbox] = {
            "mail", "post", "letter", "package", "send", "receive",
            "delivery", "correspondence", "message"
        };

        // Auctioneer keywords
        m_serviceKeywords[InteractionType::Auctioneer] = {
            "auction", "bid", "buyout", "sell", "market", "trade house",
            "listing", "offer", "sale"
        };

        // Battlemaster keywords
        m_serviceKeywords[InteractionType::Battlemaster] = {
            "battle", "battleground", "arena", "pvp", "combat", "war",
            "fight", "queue", "honor", "conquest"
        };

        // Stable master keywords
        m_serviceKeywords[InteractionType::StableMaster] = {
            "stable", "pet", "beast", "animal", "companion", "hunter",
            "tame", "feed", "care"
        };

        // Spirit healer keywords
        m_serviceKeywords[InteractionType::SpiritHealer] = {
            "resurrect", "spirit", "healer", "death", "graveyard", "grave",
            "return", "life", "release", "revive", "rebirth", "corpse",
            "restore", "risen", "dead", "soul", "afterlife"
        };

        // Icon mappings (GOSSIP_ICON_*)
        m_iconMappings[0] = GossipSelectType::Option;      // GOSSIP_ICON_CHAT
        m_iconMappings[1] = GossipSelectType::Vendor;      // GOSSIP_ICON_VENDOR
        m_iconMappings[2] = GossipSelectType::Taxi;        // GOSSIP_ICON_TAXI
        m_iconMappings[3] = GossipSelectType::Trainer;     // GOSSIP_ICON_TRAINER
        m_iconMappings[4] = GossipSelectType::Option;      // GOSSIP_ICON_INTERACT_1
        m_iconMappings[5] = GossipSelectType::Option;      // GOSSIP_ICON_INTERACT_2
        m_iconMappings[6] = GossipSelectType::Bank;        // GOSSIP_ICON_MONEY_BAG
        m_iconMappings[7] = GossipSelectType::Option;      // GOSSIP_ICON_TALK
        m_iconMappings[8] = GossipSelectType::Tabard;      // GOSSIP_ICON_TABARD
        m_iconMappings[9] = GossipSelectType::Battlemaster;// GOSSIP_ICON_BATTLE
        m_iconMappings[10] = GossipSelectType::Option;     // GOSSIP_ICON_DOT

        // Common gossip codes
        m_gossipCodes["password"] = "password";
        m_gossipCodes["secret"] = "secret";
        m_gossipCodes["code"] = "12345";
    }

    void GossipHandler::LoadKnownPaths()
    {
        // Hardcode known complex gossip paths for specific NPCs
        // Example: Orgrimmar Bank (NPC entry 3368)
        m_knownPaths.push_back({3368, InteractionType::Bank, {0}});  // First option opens bank

        // Example: Stormwind Bank (NPC entry 5049)
        m_knownPaths.push_back({5049, InteractionType::Bank, {0}});  // First option opens bank

        // Example: Dalaran Flight Master (NPC entry 28674)
        m_knownPaths.push_back({28674, InteractionType::FlightMaster, {0}});  // Direct flight menu

        // Multi-service NPCs often have predictable patterns
        // Innkeeper + Vendor: Option 1 = vendor, Option 2 = make home
        // Trainer + Vendor: Option 0 = train, Option 1 = browse goods
    }

    bool GossipHandler::NeedsGossipNavigation(Creature* creature, InteractionType type) const
    {
        if (!creature)
            return false;

        uint64 npcFlags = creature->GetNpcFlags();

        // Some NPCs provide direct service without gossip
    switch (type)
        {
            case InteractionType::Vendor:
                // Pure vendors often don't need gossip
    if ((npcFlags & UNIT_NPC_FLAG_VENDOR) && !(npcFlags & UNIT_NPC_FLAG_GOSSIP))
                    return false;
                break;

            case InteractionType::Trainer:
                // Pure trainers often don't need gossip
    if ((npcFlags & UNIT_NPC_FLAG_TRAINER) && !(npcFlags & UNIT_NPC_FLAG_GOSSIP))
                    return false;
                break;

            case InteractionType::FlightMaster:
                // Flight masters usually have gossip for discovering paths
                return true;

            case InteractionType::Bank:
            case InteractionType::Innkeeper:
                // These almost always use gossip
                return true;

            default:
                break;
        }

        // If NPC has gossip flag, assume navigation needed
        return (npcFlags & UNIT_NPC_FLAG_GOSSIP) != 0;
    }

    ::std::vector<uint32> GossipHandler::GetGossipPath(Creature* creature, InteractionType type) const
    {
        if (!creature)
            return {};

        uint32 entry = creature->GetEntry();
        // Check cached paths first
        ::std::vector<uint32> cachedPath = GetCachedPath(entry, type);
        if (!cachedPath.empty())
            return cachedPath;

        // Check known hardcoded paths
    for (const auto& known : m_knownPaths)
        {
            if (known.creatureEntry == entry && known.serviceType == type)
                return known.optionSequence;
        }

        // Return empty path - will need to discover dynamically
        return {};
    }

    int32 GossipHandler::ProcessGossipMenu(::Player* bot, uint32 menuId, ::WorldObject* target, InteractionType desiredType)
    {
        if (!bot || !target)
            return -1;

        ::std::lock_guard lock(m_mutex);

        // Get or create session
        GossipSession& session = m_activeSessions[bot->GetGUID()];
        session.botGuid = bot->GetGUID();
        session.npcGuid = target->GetGUID();
        session.menuId = menuId;
        session.targetService = desiredType;

        // Parse menu options
        session.options = ParseGossipMenu(bot, menuId);

        if (session.options.empty())
            return -1;

        // Check depth to prevent infinite loops
    if (++session.currentDepth > session.MAX_DEPTH)
        {
            TC_LOG_WARN("playerbot", "Bot {} reached max gossip depth with NPC {}",
                     bot->GetName(), target->GetName());
            m_activeSessions.erase(bot->GetGUID());
            return -1;
        }

        // Select best option
        int32 bestOption = SelectBestOption(session.options, desiredType);

        if (bestOption >= 0)
        {
            // Cache successful navigation
    if (Creature* creature = target->ToCreature())
            {
                uint32 entry = creature->GetEntry();
                ::std::vector<uint32> currentPath;
                currentPath.push_back(static_cast<uint32>(bestOption));
                CacheGossipPath(entry, desiredType, currentPath);
            }
        }

        return bestOption;
    }

    void GossipHandler::HandleGossipPacket(::Player* bot, ::WorldPacket const& packet, InteractionType desiredType)
    {
        if (!bot)
            return;

        // Parse gossip packet to extract menu information
        // This would normally parse the SMSG_GOSSIP_MESSAGE packet
        // For now, we'll rely on the game's gossip system

        // The actual packet handling would be done by the session
        // We just track the state here
        ::std::lock_guard lock(m_mutex);

        auto it = m_activeSessions.find(bot->GetGUID());
        if (it != m_activeSessions.end())
        {
            // Update session with new menu data
            // This is where we'd parse the packet if needed
        }
    }

    ::std::vector<GossipMenuOption> GossipHandler::ParseGossipMenu(::Player* bot, uint32 menuId) const
    {
        if (!bot)
            return {};

        ::std::vector<GossipMenuOption> options;

        // Get gossip menu items from player's current gossip
        // TrinityCore 12.0: GetGossipMenu() returns reference, not pointer
        GossipMenu const& menu = bot->PlayerTalkClass->GetGossipMenu();
        {
            uint32 index = 0;
            for (auto const& item : menu.GetMenuItems())
            {
                GossipMenuOption option;
                option.index = index++;  // Use sequential index
                option.icon = static_cast<uint8>(item.OptionNpc);  // OptionNpc is the icon type
                option.text = item.OptionText;
                option.sender = item.Sender;
                option.action = item.Action;
                option.boxText = item.BoxText;
                option.boxMoney = item.BoxMoney;
                option.coded = item.BoxCoded;

                options.push_back(option);
            }
        }

        return options;
    }

    int32 GossipHandler::SelectBestOption(const ::std::vector<GossipMenuOption>& options, InteractionType desiredType) const
    {
        if (options.empty())
            return -1;

        int32 bestIndex = -1;
        int32 bestScore = -1;

        for (size_t i = 0; i < options.size(); ++i)
        {
            const GossipMenuOption& option = options[i];

            // Check icon first - most reliable indicator
            GossipSelectType iconType = GetIconType(option.icon);
            if (iconType != GossipSelectType::Option)
            {
                // Direct service icon match
    if ((desiredType == InteractionType::Vendor && iconType == GossipSelectType::Vendor) ||
                    (desiredType == InteractionType::Trainer && iconType == GossipSelectType::Trainer) ||
                    (desiredType == InteractionType::FlightMaster && iconType == GossipSelectType::Taxi) ||
                    (desiredType == InteractionType::Bank && iconType == GossipSelectType::Bank) ||
                    (desiredType == InteractionType::Battlemaster && iconType == GossipSelectType::Battlemaster) ||
                    (desiredType == InteractionType::SpiritHealer && iconType == GossipSelectType::Resurrect))
                {
                    return static_cast<int32>(i);  // Immediate match
                }
            }

            // Score based on text analysis
            int32 score = ScoreOption(option, desiredType);
            if (score > bestScore)
            {
                bestScore = score;
                bestIndex = static_cast<int32>(i);
            }
        }

        // Return best scored option if score is positive
        return (bestScore > 0) ? bestIndex : -1;
    }

    bool GossipHandler::OptionLeadsToService(const GossipMenuOption& option, InteractionType desiredType) const
    {
        // Check icon match
        GossipSelectType iconType = GetIconType(option.icon);
        if ((desiredType == InteractionType::Vendor && iconType == GossipSelectType::Vendor) ||
            (desiredType == InteractionType::Trainer && iconType == GossipSelectType::Trainer) ||
            (desiredType == InteractionType::FlightMaster && iconType == GossipSelectType::Taxi) ||
            (desiredType == InteractionType::Bank && iconType == GossipSelectType::Bank))
        {
            return true;
        }

        // Analyze text
        InteractionType detectedType = AnalyzeGossipText(option.text);
        return detectedType == desiredType;
    }

    InteractionType GossipHandler::AnalyzeGossipText(const ::std::string& text) const
    {
        if (text.empty())
            return InteractionType::None;

        ::std::string lowerText = ToLowerCase(text);
        InteractionType bestMatch = InteractionType::None;
        int bestMatchCount = 0;

        // Check each service type's keywords
    for (const auto& [type, keywords] : m_serviceKeywords)
        {
            int matchCount = 0;
            for (const auto& keyword : keywords)
            {
                if (lowerText.find(keyword) != ::std::string::npos)
                    ++matchCount;
            }

            if (matchCount > bestMatchCount)
            {
                bestMatchCount = matchCount;
                bestMatch = type;
            }
        }

        return (bestMatchCount > 0) ? bestMatch : InteractionType::None;
    }

    bool GossipHandler::HandleSpecialGossip(::Player* bot, const GossipMenuOption& option)
    {
        if (!bot)
            return false;

        // Handle coded options (require text input)
    if (option.coded)
        {
            ::std::string response = GenerateResponse(bot, option.boxText);
            if (!response.empty())
            {
                // Send coded response
    if (WorldSession* session = bot->GetSession())
                {
                    // This would send the code through gossip
                    // session->HandleGossipSelectOptionOpcode with code
                    return true;
                }
            }
            return false;
        }

        // Handle money requirements
    if (option.boxMoney > 0)
        {
            if (!CanAffordOption(bot, option))
                return false;
        }

        return true;
    }

    void GossipHandler::CacheGossipPath(uint32 creatureEntry, InteractionType type, const ::std::vector<uint32>& path)
    {
        ::std::lock_guard lock(m_mutex);

        m_gossipPathCache[creatureEntry][type] = path;

        // Update statistics
        m_pathStatistics[creatureEntry].successCount++;
        m_pathStatistics[creatureEntry].lastUsed = ::std::chrono::steady_clock::now();

        // Clean cache if needed
        auto now = ::std::chrono::steady_clock::now();
        if (now - m_lastCacheCleanup > ::std::chrono::milliseconds(CACHE_CLEANUP_INTERVAL))
        {
            // Remove old unused entries
    for (auto it = m_pathStatistics.begin(); it != m_pathStatistics.end();)
            {
                if (now - it->second.lastUsed > ::std::chrono::hours(24))
                {
                    m_gossipPathCache.erase(it->first);
                    it = m_pathStatistics.erase(it);
                }
                else
                {
                    ++it;
                }
            }
            m_lastCacheCleanup = now;
        }
    }

    ::std::vector<uint32> GossipHandler::GetCachedPath(uint32 creatureEntry, InteractionType type) const
    {
        ::std::lock_guard lock(m_mutex);

        auto creatureIt = m_gossipPathCache.find(creatureEntry);
        if (creatureIt != m_gossipPathCache.end())
        {
            auto typeIt = creatureIt->second.find(type);
            if (typeIt != creatureIt->second.end())
                return typeIt->second;
        }

        return {};
    }

    void GossipHandler::ClearCache()
    {
        ::std::lock_guard lock(m_mutex);
        m_gossipPathCache.clear();
        m_pathStatistics.clear();
        m_activeSessions.clear();
    }
    GossipSelectType GossipHandler::GetIconType(uint8 icon) const
    {
        auto it = m_iconMappings.find(icon);
        if (it != m_iconMappings.end())
            return it->second;

        return GossipSelectType::Option;
    }

    bool GossipHandler::CanAffordOption(::Player* bot, const GossipMenuOption& option) const
    {
        if (!bot)
            return false;

        return bot->GetMoney() >= option.boxMoney;
    }

    ::std::string GossipHandler::GenerateResponse(::Player* bot, const ::std::string& boxText) const
    {
        if (!bot || boxText.empty())
            return "";

        ::std::string lowerText = ToLowerCase(boxText);

        // Check for known codes
    for (const auto& [prompt, response] : m_gossipCodes)
        {
            if (lowerText.find(prompt) != ::std::string::npos)
                return response;
        }

        // Default responses for common prompts
    if (lowerText.find("name") != ::std::string::npos)
            return bot->GetName();

        if (lowerText.find("guild") != ::std::string::npos)
            return bot->GetGuildName();

        if (lowerText.find("level") != ::std::string::npos)
            return ::std::to_string(bot->GetLevel());

        // No suitable response
        return "";
    }

    bool GossipHandler::ContainsKeywords(const ::std::string& text, const ::std::vector<::std::string>& keywords) const
    {
        ::std::string lowerText = ToLowerCase(text);
        for (const auto& keyword : keywords)
        {
            if (lowerText.find(keyword) != ::std::string::npos)
                return true;
        }
        return false;
    }

    ::std::string GossipHandler::ToLowerCase(const ::std::string& text) const
    {
        ::std::string result = text;
        ::std::transform(result.begin(), result.end(), result.begin(),
                      [](unsigned char c) { return ::std::tolower(c); });
        return result;
    }

    int32 GossipHandler::ScoreOption(const GossipMenuOption& option, InteractionType desiredType) const
    {
        int32 score = 0;

        // Icon match is worth a lot
        GossipSelectType iconType = GetIconType(option.icon);
        if ((desiredType == InteractionType::Vendor && iconType == GossipSelectType::Vendor) ||
            (desiredType == InteractionType::Trainer && iconType == GossipSelectType::Trainer) ||
            (desiredType == InteractionType::FlightMaster && iconType == GossipSelectType::Taxi) ||
            (desiredType == InteractionType::Bank && iconType == GossipSelectType::Bank))
        {
            score += 100;
        }

        // Text keyword matching
        auto it = m_serviceKeywords.find(desiredType);
        if (it != m_serviceKeywords.end())
        {
            ::std::string lowerText = ToLowerCase(option.text);
            for (const auto& keyword : it->second)
            {
                if (lowerText.find(keyword) != ::std::string::npos)
                    score += 10;
            }
        }

        // Penalize coded options unless we know the code
    if (option.coded)
        {
            bool knowCode = false;
            ::std::string lowerBoxText = ToLowerCase(option.boxText);
            for (const auto& [prompt, response] : m_gossipCodes)
            {
                if (lowerBoxText.find(prompt) != ::std::string::npos)
                {
                    knowCode = true;
                    break;
                }
            }
            if (!knowCode)
                score -= 50;
        }

        // Penalize expensive options
    if (option.boxMoney > 10000)  // More than 1 gold
            score -= 20;

        return score;
    }
}