/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_BOT_GOSSIP_HANDLER_H
#define TRINITYCORE_BOT_GOSSIP_HANDLER_H

#include "InteractionTypes.h"
#include "Threading/LockHierarchy.h"
#include "Define.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <shared_mutex>

class Player;
class Creature;
class WorldObject;
class WorldPacket;

namespace Playerbot
{
    /**
     * @class GossipHandler
     * @brief Intelligent gossip menu navigation for bots
     *
     * Handles:
     * - Parsing gossip menu options from NPCs
     * - Selecting correct options to reach desired services
     * - Managing multi-layer menu navigation
     * - Caching NPC-specific gossip paths for efficiency
     * - Handling special cases (code input, money requirements)
     */
    class GossipHandler
    {
    public:
        GossipHandler();
        ~GossipHandler();

        /**
         * @brief Check if NPC requires gossip navigation for interaction type
         * @param creature The NPC to check
         * @param type The desired interaction type
         * @return True if gossip navigation is needed
         */
        bool NeedsGossipNavigation(Creature* creature, InteractionType type) const;

        /**
         * @brief Get the gossip path (sequence of menu options) to reach service
         * @param creature The NPC providing gossip
         * @param type The target service type
         * @return Vector of menu option indices to select in order
         */
        ::std::vector<uint32> GetGossipPath(Creature* creature, InteractionType type) const;

        /**
         * @brief Process gossip menu received from server
         * @param bot The bot player
         * @param menuId The gossip menu ID
         * @param target The NPC showing gossip
         * @param desiredType The service type bot wants to reach
         * @return The option index to select, or -1 if none found
         */
        int32 ProcessGossipMenu(::Player* bot, uint32 menuId, ::WorldObject* target, InteractionType desiredType);

        /**
         * @brief Handle gossip packet from server
         * @param bot The bot player
         * @param packet The gossip packet
         * @param desiredType The service type bot wants
         */
        void HandleGossipPacket(::Player* bot, ::WorldPacket const& packet, InteractionType desiredType);

        /**
         * @brief Parse gossip options from menu
         * @param bot The bot player
         * @param menuId The menu ID
         * @return Vector of parsed gossip options
         */
        ::std::vector<GossipMenuOption> ParseGossipMenu(::Player* bot, uint32 menuId) const;

        /**
         * @brief Select best gossip option for desired service
         * @param options Available gossip options
         * @param desiredType The service type wanted
         * @return Index of best option, or -1 if none suitable
         */
        int32 SelectBestOption(const ::std::vector<GossipMenuOption>& options, InteractionType desiredType) const;

        /**
         * @brief Check if gossip option leads to desired service
         * @param option The gossip option to check
         * @param desiredType The service type wanted
         * @return True if option likely leads to service
         */
        bool OptionLeadsToService(const GossipMenuOption& option, InteractionType desiredType) const;

        /**
         * @brief Analyze gossip text to determine service type
         * @param text The gossip option text
         * @return Detected service type, or None if uncertain
         */
        InteractionType AnalyzeGossipText(const ::std::string& text) const;

        /**
         * @brief Handle special gossip cases (code input, etc)
         * @param bot The bot player
         * @param option The gossip option requiring special handling
         * @return True if handled successfully
         */
        bool HandleSpecialGossip(::Player* bot, const GossipMenuOption& option);

        /**
         * @brief Cache a successful gossip path for future use
         * @param creatureEntry The NPC entry ID
         * @param type The service type reached
         * @param path The successful path taken
         */
        void CacheGossipPath(uint32 creatureEntry, InteractionType type, const ::std::vector<uint32>& path);

        /**
         * @brief Get cached gossip path if exists
         * @param creatureEntry The NPC entry ID
         * @param type The desired service type
         * @return Cached path, or empty vector if not cached
         */
        ::std::vector<uint32> GetCachedPath(uint32 creatureEntry, InteractionType type) const;

        /**
         * @brief Clear gossip path cache
         */
        void ClearCache();

        /**
         * @brief Get icon type for gossip option
         * @param icon The icon ID from gossip
         * @return The interpreted icon type
         */
        GossipSelectType GetIconType(uint8 icon) const;

        /**
         * @brief Check if bot can afford gossip option
         * @param bot The bot player
         * @param option The gossip option to check
         * @return True if bot has enough money
         */
        bool CanAffordOption(::Player* bot, const GossipMenuOption& option) const;

        /**
         * @brief Generate response for NPC dialog if needed
         * @param bot The bot player
         * @param boxText The text prompt from NPC
         * @return Generated response string
         */
        ::std::string GenerateResponse(::Player* bot, const ::std::string& boxText) const;

    private:
        /**
         * @brief Initialize keyword mappings for text analysis
         */
        void InitializeKeywordMappings();

        /**
         * @brief Load known gossip paths from configuration
         */
        void LoadKnownPaths();

        /**
         * @brief Check if text contains keywords for service type
         * @param text The text to analyze
         * @param keywords The keywords to search for
         * @return True if any keyword found
         */
        bool ContainsKeywords(const ::std::string& text, const ::std::vector<::std::string>& keywords) const;

        /**
         * @brief Convert text to lowercase for comparison
         * @param text The text to convert
         * @return Lowercase version of text
         */
        ::std::string ToLowerCase(const ::std::string& text) const;

        /**
         * @brief Score a gossip option for relevance to desired service
         * @param option The gossip option to score
         * @param desiredType The service type wanted
         * @return Score (higher = more relevant)
         */
        int32 ScoreOption(const GossipMenuOption& option, InteractionType desiredType) const;

    private:
        // Thread safety
        mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::ACTION_PRIORITY> m_mutex;

        // Gossip path cache: [creatureEntry][interactionType] = path
        ::std::unordered_map<uint32, ::std::unordered_map<InteractionType, ::std::vector<uint32>>> m_gossipPathCache;

        // Keyword mappings for text analysis
        ::std::unordered_map<InteractionType, ::std::vector<::std::string>> m_serviceKeywords;

        // Icon to service type mappings
        ::std::unordered_map<uint8, GossipSelectType> m_iconMappings;

        // Known NPC gossip patterns (hardcoded for specific NPCs)
        struct KnownGossipPath
        {
            uint32 creatureEntry;
            InteractionType serviceType;
            ::std::vector<uint32> optionSequence;
        };
        ::std::vector<KnownGossipPath> m_knownPaths;

        // Current gossip session tracking
        struct GossipSession
        {
            ObjectGuid botGuid;
            ObjectGuid npcGuid;
            uint32 menuId = 0;
            ::std::vector<GossipMenuOption> options;
            InteractionType targetService = InteractionType::None;
            uint32 currentDepth = 0;
            const uint32 MAX_DEPTH = 5;
        };
        ::std::unordered_map<ObjectGuid, GossipSession> m_activeSessions;

        // Statistics for learning
        struct GossipStatistics
        {
            uint32 successCount = 0;
            uint32 failureCount = 0;
            float successRate = 0.0f;
            ::std::chrono::steady_clock::time_point lastUsed;
        };
        ::std::unordered_map<uint32, GossipStatistics> m_pathStatistics;

        // Common gossip codes/passwords
        ::std::unordered_map<::std::string, ::std::string> m_gossipCodes;

        // Performance optimization
        const uint32 CACHE_CLEANUP_INTERVAL = 300000; // 5 minutes
        ::std::chrono::steady_clock::time_point m_lastCacheCleanup;

        bool m_initialized = false;
    };
}

#endif // TRINITYCORE_BOT_GOSSIP_HANDLER_H