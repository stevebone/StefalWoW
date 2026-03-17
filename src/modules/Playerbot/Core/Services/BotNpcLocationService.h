/**
 * @file BotNpcLocationService.h
 * @brief Enterprise-grade NPC location resolution service for PlayerBot module
 *
 * ENTERPRISE DESIGN PRINCIPLES:
 * - Single Responsibility: Centralized NPC/spawn location resolution
 * - Performance: O(1) lookups via map-indexed caching
 * - Thread Safety: Read-only after initialization, lock-free queries
 * - Completeness: Handles ALL bot NPC needs (quests, trainers, vendors, services)
 * - Maintainability: Clear separation of concerns, comprehensive logging
 * - Sustainability: Startup-time caching eliminates runtime performance cost
 *
 * PROBLEM SOLVED:
 * - Eliminates 261K+ spawn iteration deadlocks
 * - Provides fast, reliable NPC location lookups for all bot systems
 * - Supports quest objectives, profession trainers, class trainers, services
 * - Multi-source resolution with quality scoring
 *
 * USAGE EXAMPLE:
 * @code
 * // Quest objective location
 * auto location = sBotNpcLocationService->FindQuestObjectiveLocation(bot, questId, objectiveIdx);
 * if (location.isValid)
 *     bot->GetMotionMaster()->MovePoint(0, location.position);
 *
 * // Nearest profession trainer
 * auto trainer = sBotNpcLocationService->FindNearestProfessionTrainer(bot, SKILL_BLACKSMITHING);
 * if (trainer.isValid)
 *     bot->TeleportTo(trainer.position.GetMapId(), trainer.position);
 * @endcode
 *
 * @created 2025-10-27
 * @part-of PlayerBot Quest & Services Infrastructure
 */

#ifndef BOT_NPC_LOCATION_SERVICE_H
#define BOT_NPC_LOCATION_SERVICE_H

#include "Define.h"
#include "Position.h"
#include "ObjectGuid.h"
#include <unordered_map>
#include <vector>
#include <memory>

class Player;

namespace Playerbot
{
    /**
     * @enum NpcServiceType
     * @brief Categories of NPC services bots need to locate
     */
    enum class NpcServiceType : uint8
    {
        INNKEEPER       = 0,  // Rest, set hearthstone
        VENDOR_GENERAL  = 1,  // General goods
        VENDOR_FOOD     = 2,  // Food/drink
        VENDOR_REPAIR   = 3,  // Repair equipment
        BANKER          = 4,  // Bank access
        AUCTIONEER      = 5,  // Auction house
        FLIGHT_MASTER   = 6,  // Flight paths
        STABLE_MASTER   = 7,  // Pet stable
        GUILD_MASTER    = 8,  // Guild creation
        MAILBOX         = 9,  // Mail access (GameObject)
        QUEST_GIVER     = 10, // Quest NPCs
        SPIRIT_HEALER   = 11, // Resurrection
        BATTLEMASTER    = 12  // PvP queue
    };

    /**
     * @struct NpcLocationResult
     * @brief Result of NPC location query with quality metadata
     */
    struct NpcLocationResult
    {
        Position position;           // NPC location
        uint32 entry = 0;           // Creature/GameObject entry ID
        ObjectGuid guid;            // GUID if live entity found
        float distance = 99999.0f;  // Distance from bot
        bool isValid = false;       // Whether location was found
        bool isLiveEntity = false;  // If found in spatial grid (spawned)
        uint8 qualityScore = 0;     // 0-100, higher = better (live > spawn > POI)
        ::std::string sourceName;     // Debug: where location came from

        NpcLocationResult() = default;

        explicit operator bool() const { return isValid; }
    };

    /**
     * @class BotNpcLocationService
     * @brief Singleton service providing fast, cached NPC location lookups
     *
     * INITIALIZATION:
     * - Called at server startup via sWorld->SetInitialWorldSettings()
     * - Builds map-indexed caches of all spawn data (one-time cost)
     * - Typical startup time: ~2-5 seconds for full world database
     *
     * THREAD SAFETY:
     * - ALL query methods are thread-safe (read-only after init)
     * - Can be called from worker threads (BotAI::Update)
     * - Can be called from main thread (quest systems, strategies)
     *
     * PERFORMANCE:
     * - O(1) map-indexed lookups via std::unordered_map
     * - Pre-filtered by mapId (eliminates 95%+ of irrelevant data)
     * - Typical query time: <0.1ms for cached lookups
     * - Memory usage: ~50-100MB for full spawn database cache
     */
    class BotNpcLocationService final
    {
    public:
        static BotNpcLocationService* instance();

        /**
         * @brief Initialize service and build all caches
         * @return true if initialization successful
         *
         * CALLED BY: sWorld->SetInitialWorldSettings()
         * TIMING: Server startup, after ObjectMgr initialization
         */
        bool Initialize();

        /**
         * @brief Clear all caches (for reloading)
         */
        void Shutdown();

        // ===== QUEST OBJECTIVE LOCATION QUERIES =====

        /**
         * @brief Find location for quest objective (KILL_CREATURE, USE_GAMEOBJECT, etc.)
         *
         * @param bot The bot performing the query
         * @param questId Quest ID
         * @param objectiveIndex Objective index (0-based)
         *
         * @return NpcLocationResult with best location found
         *
         * RESOLUTION ORDER:
         * 1. Live spawned creature/object in spatial grid (quality: 100)
         * 2. Nearest spawn point from database cache (quality: 80)
         * 3. Quest POI data (quality: 60)
         * 4. Invalid result if none found
         *
         * PERFORMANCE: O(1) for cache lookup, O(n) for spatial grid (n = nearby entities)
         */
        NpcLocationResult FindQuestObjectiveLocation(Player* bot, uint32 questId, uint32 objectiveIndex);

        // ===== TRAINER LOCATION QUERIES =====

        /**
         * @brief Find nearest profession trainer
         *
         * @param bot The bot
         * @param skillId Profession skill ID (e.g., SKILL_BLACKSMITHING = 164)
         *
         * @return NpcLocationResult with nearest trainer location
         *
         * EXAMPLE SKILLS:
         * - SKILL_BLACKSMITHING = 164
         * - SKILL_ALCHEMY = 171
         * - SKILL_ENCHANTING = 333
         * - SKILL_ENGINEERING = 202
         * - etc.
         */
        NpcLocationResult FindNearestProfessionTrainer(Player* bot, uint32 skillId);

        /**
         * @brief Find nearest class trainer
         *
         * @param bot The bot
         * @param classId Class ID (e.g., CLASS_WARRIOR = 1)
         *
         * @return NpcLocationResult with nearest class trainer location
         */
        NpcLocationResult FindNearestClassTrainer(Player* bot, uint8 classId);

        // ===== SERVICE NPC LOCATION QUERIES =====

        /**
         * @brief Find nearest service NPC (innkeeper, vendor, banker, etc.)
         *
         * @param bot The bot
         * @param serviceType Type of service needed
         *
         * @return NpcLocationResult with nearest service provider location
         *
         * EXAMPLES:
         * - FindNearestService(bot, NpcServiceType::INNKEEPER)
         * - FindNearestService(bot, NpcServiceType::VENDOR_REPAIR)
         * - FindNearestService(bot, NpcServiceType::BANKER)
         */
        NpcLocationResult FindNearestService(Player* bot, NpcServiceType serviceType);

        /**
         * @brief Find specific creature spawn by entry ID
         *
         * @param bot The bot
         * @param creatureEntry Creature entry ID
         * @param maxRange Maximum search range (default: 500 yards)
         *
         * @return NpcLocationResult with nearest spawn of that creature type
         */
        NpcLocationResult FindNearestCreatureSpawn(Player* bot, uint32 creatureEntry, float maxRange = 500.0f);

        /**
         * @brief Find specific GameObject spawn by entry ID
         *
         * @param bot The bot
         * @param objectEntry GameObject entry ID
         * @param maxRange Maximum search range (default: 500 yards)
         *
         * @return NpcLocationResult with nearest spawn of that object type
         */
        NpcLocationResult FindNearestGameObjectSpawn(Player* bot, uint32 objectEntry, float maxRange = 500.0f);

        // ===== VALIDATION & DIAGNOSTICS =====

        /**
         * @brief Check if service has been initialized
         */
        bool IsInitialized() const { return _initialized; }

        /**
         * @brief Get cache statistics for diagnostics
         */
        struct CacheStats
        {
            uint32 creatureSpawnsCached = 0;
            uint32 gameObjectSpawnsCached = 0;
            uint32 professionTrainersCached = 0;
            uint32 classTrainersCached = 0;
            uint32 serviceNpcsCached = 0;
            uint32 questPOIsCached = 0;
            uint32 areaTriggerQuestsCached = 0;      // New: Quest→AreaTrigger mappings
            uint32 areaTriggerPositionsCached = 0;   // New: AreaTrigger positions
            uint32 mapsIndexed = 0;
        };

        CacheStats GetCacheStats() const;

    private:
        BotNpcLocationService() = default;
        ~BotNpcLocationService() = default;

        // Prevent copying
        BotNpcLocationService(BotNpcLocationService const&) = delete;
        BotNpcLocationService& operator=(BotNpcLocationService const&) = delete;

        // ===== INTERNAL CACHE STRUCTURES =====

        /**
         * @struct SpawnLocationData
         * @brief Cached spawn location with metadata
         */
        struct SpawnLocationData
        {
            Position position;
            uint32 entry;
            uint32 mapId;
            float distanceFromOrigin; // Pre-calculated for sorting
        };

        // Map-indexed spawn caches (mapId → entry → vector<positions>)
        ::std::unordered_map<uint32, ::std::unordered_map<uint32, ::std::vector<SpawnLocationData>>> _creatureSpawnCache;
        ::std::unordered_map<uint32, ::std::unordered_map<uint32, ::std::vector<SpawnLocationData>>> _gameObjectSpawnCache;

        // Profession trainer cache (skillId → vector<locations>)
        ::std::unordered_map<uint32, ::std::vector<SpawnLocationData>> _professionTrainerCache;

        // Class trainer cache (classId → vector<locations>)
        ::std::unordered_map<uint8, ::std::vector<SpawnLocationData>> _classTrainerCache;

        // Service NPC cache (serviceType → vector<locations>)
        ::std::unordered_map<NpcServiceType, ::std::vector<SpawnLocationData>> _serviceNpcCache;

        // Quest POI cache (questId → objectiveIndex → position)
        ::std::unordered_map<uint32, ::std::unordered_map<uint32, Position>> _questPOICache;

        // ===== AREA TRIGGER CACHES (Thread-safe - populated at init) =====
        // Maps questId → areaTriggerID (from areatrigger_involvedrelation table)
        ::std::unordered_map<uint32, uint32> _areaTriggerQuestCache;

        // Maps areaTriggerID → position data (from areatrigger table for classic WoW triggers)
        struct AreaTriggerPositionData
        {
            float posX = 0.0f;
            float posY = 0.0f;
            float posZ = 0.0f;
            uint32 mapId = 0;
            bool isValid = false;
        };
        ::std::unordered_map<uint32, AreaTriggerPositionData> _areaTriggerPositionCache;

        bool _initialized = false;

        // ===== CACHE BUILDING METHODS (called during Initialize) =====

        void BuildCreatureSpawnCache();
        void BuildGameObjectSpawnCache();
        void BuildProfessionTrainerCache();
        void BuildClassTrainerCache();
        void BuildServiceNpcCache();
        void BuildQuestPOICache();
        void BuildAreaTriggerCache();  // New: Thread-safe area trigger caching

        // ===== HELPER METHODS =====

        /**
         * @brief Find nearest location from cached list
         */
        NpcLocationResult FindNearestFromCache(
            Player* bot,
            ::std::vector<SpawnLocationData> const& locations,
            float maxRange,
            ::std::string const& sourceName);

        /**
         * @brief Check if creature is a trainer for given skill
         */
        bool IsTrainerForSkill(uint32 creatureEntry, uint32 skillId);

        /**
         * @brief Check if creature is a class trainer
         */
        bool IsClassTrainer(uint32 creatureEntry, uint8 classId);

        /**
         * @brief Check if creature provides service
         */
        bool ProvidesService(uint32 creatureEntry, NpcServiceType serviceType);

        /**
         * @brief Try to find live entity in spatial grid first
         */
        NpcLocationResult TryFindLiveCreature(Player* bot, uint32 creatureEntry, float maxRange);

        /**
         * @brief Try to find live GameObject in spatial grid first
         */
        NpcLocationResult TryFindLiveGameObject(Player* bot, uint32 objectEntry, float maxRange);
    };

} // namespace Playerbot

#define sBotNpcLocationService Playerbot::BotNpcLocationService::instance()

#endif // BOT_NPC_LOCATION_SERVICE_H
