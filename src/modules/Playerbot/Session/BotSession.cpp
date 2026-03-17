/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#include "BotSession.h"
#include "DatabaseEnv.h"
#include "CharacterDatabase.h"
#include "BotPacketRelay.h"
#include "BotPacketSimulator.h"  // PHASE 1: Packet forging infrastructure
#include "PacketDeferralClassifier.h"  // Selective main thread deferral
#include "Config/PlayerbotConfig.h"  // For configuration access
#include "AccountMgr.h"
#include "Log.h"
#include "WorldPacket.h"
#include "Player.h"
#include "Corpse.h"  // For CORPSE_RECLAIM_RADIUS in safe resurrection
#include "Creature.h"  // For loot processing
#include "GameObject.h"  // For object use processing
#include "Loot.h"  // For SendLoot
#include "QueryHolder.h"
#include "QueryCallback.h"
#include "CharacterPackets.h"
#include "Database/PlayerbotCharacterDBInterface.h"
#include "World.h"
#include "Map.h"
#include "MapManager.h"
#include "AI/BotAI.h"
#include "Lifecycle/DeathRecoveryManager.h"  // For death recovery check at login
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
// BotAIFactory is declared in BotAI.h
#include <thread>
#include "GameTime.h"
#include "Lifecycle/BotSpawner.h"
#include <condition_variable>
#include <future>
#include <unordered_set>
#include <set>
#include <unordered_map>
#include <boost/asio/io_context.hpp>
#include "Chat/Chat.h"
#include "../Group/GroupMemberResolver.h"
#include "../Core/Diagnostics/GroupMemberDiagnostics.h"
#include "Database/QueryHolder.h"
#include "Group/GroupInvitationHandler.h"
#include "PartyPackets.h"
#include "Opcodes.h"
#include "ByteBuffer.h"          // For ByteBufferException
#include "WorldSession.h"        // For WorldPackets exception types
#include "Group.h"
#include "GroupMgr.h"
#include "RealmList.h"
#include "AuthenticationPackets.h"
#include "Core/Events/BotEventTypes.h"  // PHASE 0 - Quick Win #3: For GROUP_JOINED event
#include "Core/Events/EventDispatcher.h"  // PHASE 0 - Quick Win #3: For event dispatch
#include "PhasingHandler.h"  // For bot phase initialization
#include "Spatial/SpatialGridQueryHelpers.h"  // PHASE 2F: For snapshot-based player validation
#include "MotionMaster.h"  // For ProcessPendingStopMovement
#include "DB2Stores.h"     // For sDB2Manager, ChrSpecializationEntry (auto-spec fix)
#include "SharedDefines.h" // For MIN_SPECIALIZATION_LEVEL, LOCALE_enUS (auto-spec fix)
#include "LFGMgr.h"        // For sLFGMgr->UpdateProposal (LFG auto-accept)
#include "LFGPacketsCommon.h" // For RideTicket parsing (LFG auto-accept)
#include "Lifecycle/Instance/BotPostLoginConfigurator.h"  // Post-login configuration for JIT bots
#include "BattlegroundMgr.h"  // For queue state checking (instance bot idle detection)
#include "BattlegroundQueue.h"  // For IsPlayerInvitedToRatedMatch etc.

namespace Playerbot {

// BotLoginQueryHolder::Initialize implementation
bool BotSession::BotLoginQueryHolder::Initialize()
{
    bool res = true;
    ObjectGuid::LowType lowGuid = m_guid.GetCounter();

    TC_LOG_DEBUG("module.playerbot.session", "Initializing BotLoginQueryHolder with {} queries for character GUID {}",
                 MAX_PLAYER_LOGIN_QUERY, lowGuid);

    // Set the size first
    SetSize(MAX_PLAYER_LOGIN_QUERY);

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_FROM, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_CUSTOMIZATIONS);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_CUSTOMIZATIONS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GROUP_MEMBER);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GROUP, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_AURAS);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_AURAS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_AURA_EFFECTS);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_AURA_EFFECTS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_AURA_STORED_LOCATIONS);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_AURA_STORED_LOCATIONS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SPELL);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SPELLS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SPELL_FAVORITES);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SPELL_FAVORITES, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_QUEST_STATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_OBJECTIVES);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_QUEST_STATUS_OBJECTIVES, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_OBJECTIVES_CRITERIA);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_QUEST_STATUS_OBJECTIVES_CRITERIA, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_OBJECTIVES_CRITERIA_PROGRESS);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_QUEST_STATUS_OBJECTIVES_CRITERIA_PROGRESS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_OBJECTIVES_SPAWN_TRACKING);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_QUEST_STATUS_OBJECTIVES_SPAWN_TRACKING, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_DAILY);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_DAILY_QUEST_STATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_WEEKLY);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_WEEKLY_QUEST_STATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_MONTHLY);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_MONTHLY_QUEST_STATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_SEASONAL);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SEASONAL_QUEST_STATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_REPUTATION);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_REPUTATION, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_INVENTORY);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_INVENTORY, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ITEM_INSTANCE_ARTIFACT);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ARTIFACTS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ITEM_INSTANCE_AZERITE);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_AZERITE, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ITEM_INSTANCE_AZERITE_MILESTONE_POWER);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_AZERITE_MILESTONE_POWERS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ITEM_INSTANCE_AZERITE_UNLOCKED_ESSENCE);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_AZERITE_UNLOCKED_ESSENCES, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ITEM_INSTANCE_AZERITE_EMPOWERED);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_AZERITE_EMPOWERED, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_MAIL);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_MAILS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_MAILITEMS);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_MAIL_ITEMS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_MAILITEMS_ARTIFACT);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_MAIL_ITEMS_ARTIFACT, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_MAILITEMS_AZERITE);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_MAIL_ITEMS_AZERITE, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_MAILITEMS_AZERITE_MILESTONE_POWER);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_MAIL_ITEMS_AZERITE_MILESTONE_POWER, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_MAILITEMS_AZERITE_UNLOCKED_ESSENCE);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_MAIL_ITEMS_AZERITE_UNLOCKED_ESSENCE, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_MAILITEMS_AZERITE_EMPOWERED);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_MAIL_ITEMS_AZERITE_EMPOWERED, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SOCIALLIST);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SOCIAL_LIST, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_HOMEBIND);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_HOME_BIND, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SPELLCOOLDOWNS);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SPELL_COOLDOWNS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SPELL_CHARGES);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SPELL_CHARGES, stmt);

    // Handle conditional queries properly
    if (sWorld->getBoolConfig(CONFIG_DECLINED_NAMES_USED))
    {
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_DECLINEDNAMES);
        stmt->setUInt64(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_DECLINED_NAMES, stmt);
    }

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GUILD_MEMBER);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GUILD, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ARENAINFO);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ARENA_INFO, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ACHIEVEMENTS);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ACHIEVEMENTS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_CRITERIAPROGRESS);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_CRITERIA_PROGRESS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_EQUIPMENTSETS);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_EQUIPMENT_SETS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_TRANSMOG_OUTFITS);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_TRANSMOG_OUTFITS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_CUF_PROFILES);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_CUF_PROFILES, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_BGDATA);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_BG_DATA, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_GLYPHS);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GLYPHS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_TALENTS);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_TALENTS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_PVP_TALENTS);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_PVP_TALENTS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PLAYER_ACCOUNT_DATA);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ACCOUNT_DATA, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SKILLS);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SKILLS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_RANDOMBG);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_RANDOM_BG, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_BANNED);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_BANNED, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUSREW);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_QUEST_STATUS_REW, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ACCOUNT_INSTANCELOCKTIMES);
    stmt->setUInt32(0, m_accountId);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_INSTANCE_LOCK_TIMES, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PLAYER_CURRENCY);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_CURRENCY, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CORPSE_LOCATION);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_CORPSE_LOCATION, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_PETS);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_PET_SLOTS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_GARRISON);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GARRISON, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_GARRISON_BLUEPRINTS);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GARRISON_BLUEPRINTS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_GARRISON_BUILDINGS);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GARRISON_BUILDINGS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_GARRISON_FOLLOWERS);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GARRISON_FOLLOWERS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_GARRISON_FOLLOWER_ABILITIES);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GARRISON_FOLLOWER_ABILITIES, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_TRAIT_ENTRIES);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_TRAIT_ENTRIES, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_TRAIT_CONFIGS);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_TRAIT_CONFIGS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PLAYER_DATA_ELEMENTS_CHARACTER);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_DATA_ELEMENTS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PLAYER_DATA_FLAGS_CHARACTER);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_DATA_FLAGS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_BANK_TAB_SETTINGS);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_BANK_TAB_SETTINGS, stmt);

    TC_LOG_DEBUG("module.playerbot.session", "BotLoginQueryHolder::Initialize() completed with result: {}", res);
    return res;
}

// BotLoginQueryHolder implementation (uses async pattern like regular WorldSession)

// Global io_context for bot sockets
static boost::asio::io_context g_botIoContext;

BotSession::BotSession(uint32 bnetAccountId)
    : WorldSession(
        bnetAccountId,                  // Use battlenet account as account ID for now
        std::string(""),               // Empty username (generated by Trinity)
        bnetAccountId,                 // BattleNet account ID
        std::string(""),               // BattleNet account email (new in 12.0.7)
        std::shared_ptr<WorldSocket>(), // No socket (empty shared_ptr for bots)
        SEC_PLAYER,                    // Security level
        EXPANSION_LEVEL_CURRENT,       // Current expansion
        0,                             // Mute time
        std::string(""),               // OS
        Minutes(0),                    // Timezone
        0,                             // Build
        ClientBuild::VariantId{},      // Client build variant
        LOCALE_enUS,                   // Locale
        0,                             // Recruiter
        false,                         // Is recruiter
        true),                         // is_bot = true  CRITICAL FIX: Makes IsBot() work correctly!
    _bnetAccountId(bnetAccountId),
    _simulatedLatency(sPlayerbotConfig->GetUInt("Playerbot.Session.SimulatedLatency", 50))
{
    // CRITICAL FIX: Validate account IDs and ensure proper initialization
    if (bnetAccountId == 0) {
        TC_LOG_ERROR("module.playerbot.session", "BotSession constructor called with invalid account ID: {}", bnetAccountId);
        _active.store(false);
        return;
    }

    if (GetAccountId() == 0) {
        TC_LOG_ERROR("module.playerbot.session", "BotSession GetAccountId() returned 0 after construction with ID: {}", bnetAccountId);
        _active.store(false);
        return;
    }

    // Initialize atomic values explicitly
    _active.store(true);
    _loginState.store(LoginState::NONE);

    TC_LOG_INFO("module.playerbot.session", " BotSession constructor complete for account {} (GetAccountId: {})", bnetAccountId, GetAccountId());
}

// Factory method that creates BotSession with better socket handling
::std::shared_ptr<BotSession> BotSession::Create(uint32 bnetAccountId)
{
    TC_LOG_INFO("module.playerbot.session", " BotSession::Create() factory method called for account {}", bnetAccountId);

    // Create BotSession using regular constructor
    auto session = ::std::make_shared<BotSession>(bnetAccountId);

    // NOTE: Bots operate with a null socket. BotSession overrides all socket-dependent
    // methods (SendPacket, KickPlayer, etc.) to handle the socketless case.

    return session;
}

// Override PlayerDisconnected to always return false for bot sessions
bool BotSession::PlayerDisconnected() const
{
    // Bot sessions are never considered disconnected since they don't rely on network sockets
    return false;
}

BotSession::~BotSession()
{
    uint32 accountId = 0;
    try {
        accountId = GetAccountId();
    } catch (...)
    {
        accountId = _bnetAccountId; // Fallback to stored value
    }

    TC_LOG_DEBUG("module.playerbot.session", "BotSession destructor called for account {}", accountId);

    // CRITICAL SAFETY: Mark as destroyed ATOMICALLY first to prevent any new operations
    _destroyed.store(true);
    _active.store(false);

    // DEADLOCK PREVENTION: Wait for any ongoing packet processing to complete
    // Use a reasonable timeout to prevent hanging during shutdown
    auto waitStart = ::std::chrono::steady_clock::now();
    constexpr auto MAX_WAIT_TIME = ::std::chrono::milliseconds(500);

    while (_packetProcessing.load() &&
           (::std::chrono::steady_clock::now() - waitStart) < MAX_WAIT_TIME)
           {
        ::std::this_thread::sleep_for(::std::chrono::milliseconds(1));
    }

    if (_packetProcessing.load())
    {
        TC_LOG_WARN("module.playerbot.session", "BotSession destructor: Packet processing still active after 500ms wait for account {}", accountId);
    }

    // CRITICAL FIX: Clear m_spellModTakingSpell BEFORE player cleanup to prevent crash
    // When bot logs out, delayed spells (like Detect Sparkle Aura 84459) are still in EventProcessor
    // The SpellEvent destructor will fire and Spell::~Spell() checks m_spellModTakingSpell
    // We MUST clear this pointer before the base WorldSession destructor cleans up the player
    if (Player* player = GetPlayer()) {
        try {
            // CRITICAL FIX (Map.cpp:1945 crash): Remove from _updateObjects BEFORE destruction
            // This is a defensive fallback in case RemovePlayerBot() wasn't called (e.g., session
            // destroyed through a different code path). ClearUpdateMask(true) calls
            // RemoveFromObjectUpdate() which removes the player from Map::_updateObjects,
            // preventing MapUpdater worker threads from accessing a destroyed player.
            //
            // Note: Player::ClearUpdateMask is protected, but Object::ClearUpdateMask is public.
            // We cast to Object* to access the base class public method.
            static_cast<Object*>(player)->ClearUpdateMask(true);

            // Core Fix Applied: SpellEvent::~SpellEvent() now automatically clears m_spellModTakingSpell (Spell.cpp:8455)
            player->m_Events.KillAllEvents(false);
            TC_LOG_DEBUG("module.playerbot.session", "Bot {} cleared spell events during logout", player->GetName());
        } catch (...)
        {
            TC_LOG_ERROR("module.playerbot.session", "Exception clearing spell events during logout for account {}", accountId);
        }
    }

    // P1 FIX: AI cleanup now automatic via std::unique_ptr
    // The unique_ptr destructor will automatically delete the BotAI object
    // No manual delete needed - exception-safe by design
    // Old code (REMOVED):
    //   if (_ai) { delete _ai; _ai = nullptr; }

    // THREAD SAFETY: Login state cleanup (synchronous mode requires minimal cleanup)
    try {
        _loginState.store(LoginState::NONE);
    } catch (...)
    {
        TC_LOG_ERROR("module.playerbot.session", "Exception clearing login state for account {}", accountId);
    }

    // P0 FIX: PACKET CLEANUP WITH SPIN-WAIT TIMEOUT
    // CRITICAL: Must cleanup packets even if mutex is contested to prevent memory leak
    // Strategy: Spin-wait with 2 second timeout, force cleanup if timeout expires
    try {
        ::std::unique_lock<::std::mutex> lock(_packetMutex, ::std::defer_lock);

        // Spin-wait with 2 second timeout
        auto deadline = ::std::chrono::steady_clock::now() + ::std::chrono::seconds(2);
        while (!lock.try_lock() && ::std::chrono::steady_clock::now() < deadline) {
            ::std::this_thread::sleep_for(::std::chrono::milliseconds(10));
        }

        if (!lock.owns_lock()) {
            TC_LOG_ERROR("module.playerbot.session",
                "BotSession[{}]: FORCED packet cleanup - mutex timeout after 2s (destructor context is safe)",
                accountId);
        }

        // ALWAYS cleanup (with or without lock - destructor context guarantees safety)
        // No other thread can access this session after destructor starts
        ::std::queue<::std::unique_ptr<WorldPacket>> empty1, empty2;
        _incomingPackets.swap(empty1);
        _outgoingPackets.swap(empty2);
        // Queues will be destroyed when they go out of scope, freeing all packets
    } catch (...)
    {
        // CRITICAL: Never throw from destructor - just log and continue
        TC_LOG_ERROR("module.playerbot.session", "BotSession destructor: Exception during packet cleanup for account {}", accountId);
    }

    TC_LOG_DEBUG("module.playerbot.session", "BotSession destructor completed for account {}", accountId);
}

CharacterDatabasePreparedStatement* BotSession::GetSafePreparedStatement(CharacterDatabaseStatements statementId, const char* statementName)
{
    // CRITICAL FIX: Add statement index validation before accessing to prevent assertion failure
    if (statementId >= MAX_CHARACTERDATABASE_STATEMENTS) {
        TC_LOG_ERROR("module.playerbot", "BotSession::GetSafePreparedStatement: Invalid statement index {} >= {} for {}",
                     static_cast<uint32>(statementId), MAX_CHARACTERDATABASE_STATEMENTS, statementName);
        return nullptr;
    }

    // Use CharacterDatabase directly for standard TrinityCore character operations
    TC_LOG_DEBUG("module.playerbot.session",
        "Getting prepared statement {} ({}) directly from CharacterDatabase",
        static_cast<uint32>(statementId), statementName);

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(statementId);
    if (!stmt)
    {
        TC_LOG_ERROR("module.playerbot", "BotSession::GetSafePreparedStatement: Failed to get prepared statement {} (index: {})",
                     statementName, static_cast<uint32>(statementId));
        return nullptr;
    }
    return stmt;
}

void BotSession::SendPacket(WorldPacket const* packet, bool forced)
{
    if (!packet) return;

    // Note: forced parameter is not used for bot sessions but required for interface
    (void)forced;

    // Reactive movement ACK: notify simulator of SMSG packets requiring ACK response
    if (_packetSimulator)
        _packetSimulator->OnPacketSent(packet->GetOpcode());

    // CRITICAL: Intercept party invitation packets for bot handling
    if (packet->GetOpcode() == SMSG_PARTY_INVITE)
    {
        TC_LOG_INFO("module.playerbot.group", "BotSession: Intercepted SMSG_PARTY_INVITE packet for bot session");
        HandleGroupInvitation(*packet);
    }

    // ========================================================================
    // LFG PROPOSAL AUTO-ACCEPT
    // When a bot receives an LFG proposal packet, automatically accept it.
    // This is a module-only solution that intercepts the outgoing packet
    // without modifying TrinityCore core files.
    //
    // INFINITE LOOP FIX: UpdateProposal() sends SMSG_LFG_PROPOSAL_UPDATE to all
    // players in the proposal (to update the UI), which would trigger this
    // intercept again causing an infinite loop. We track which proposals have
    // been auto-accepted to prevent re-processing the same proposal.
    // ========================================================================
    if (packet->GetOpcode() == SMSG_LFG_PROPOSAL_UPDATE)
    {
        Player* bot = GetPlayer();
        if (!bot)
        {
            TC_LOG_ERROR("module.playerbot.lfg", "❌ BotSession: Received SMSG_LFG_PROPOSAL_UPDATE but GetPlayer() is NULL!");
            return;
        }

        try
        {
            // Clone the packet data for reading (packet is const)
            WorldPacket packetCopy(*packet);
            packetCopy.rpos(0);

            // Parse the packet structure:
            // 1. RideTicket (RequesterGuid + Id + Type + Time + IsCrossFaction)
            // 2. uint64 InstanceID
            // 3. uint32 ProposalID

            // Read RideTicket
            WorldPackets::LFG::RideTicket ticket;
            packetCopy >> ticket;

            // Read InstanceID
            uint64 instanceId;
            packetCopy >> instanceId;

            // Read ProposalID
            uint32 proposalId;
            packetCopy >> proposalId;

            // INFINITE LOOP FIX: Check if we've already auto-accepted this proposal
            bool shouldAccept = false;
            {
                std::lock_guard<std::mutex> lock(_lfgProposalMutex);
                if (_autoAcceptedProposals.count(proposalId) == 0)
                {
                    // First time seeing this proposal - mark it and accept
                    _autoAcceptedProposals.insert(proposalId);
                    shouldAccept = true;

                    // Cleanup: Limit set size to prevent memory growth (keep last 10 proposals)
                    if (_autoAcceptedProposals.size() > 10)
                    {
                        _autoAcceptedProposals.clear();
                        _autoAcceptedProposals.insert(proposalId);
                    }
                }
            }

            if (shouldAccept)
            {
                TC_LOG_INFO("module.playerbot.lfg", "🎮 BotSession: Queuing LFG proposal {} accept for bot {} (deferred to main thread)",
                            proposalId, bot->GetName());

                // RE-ENTRANT CRASH FIX: Do NOT call UpdateProposal() here!
                // We are inside LFGMgr::UpdateProposal() iteration when this packet is sent.
                // Calling UpdateProposal() again would invalidate the iterator and crash.
                // Instead, queue the accept to be processed on main thread later.
                QueueLfgProposalAccept(proposalId);

                TC_LOG_INFO("module.playerbot.lfg", "✅ BotSession: Queued LFG proposal {} for bot {} (will accept on main thread)",
                            proposalId, bot->GetName());
            }
            else
            {
                TC_LOG_DEBUG("module.playerbot.lfg", "BotSession: Skipping already-processed LFG proposal {} for bot {}",
                            proposalId, bot->GetName());
            }
        }
        catch (ByteBufferPositionException const& ex)
        {
            TC_LOG_ERROR("module.playerbot.lfg", "❌ BotSession: Failed to parse LFG proposal packet for bot {}: {}",
                         bot->GetName(), ex.what());
        }
    }

    // ========================================================================
    // LFG BOOT VOTE AUTO-ACCEPT
    // When a bot receives a boot proposal packet (vote kick), automatically
    // vote YES to allow kicking malfunctioning or offline bots from dungeons.
    // This is a module-only solution that intercepts the outgoing packet
    // without modifying TrinityCore core files.
    // ========================================================================
    if (packet->GetOpcode() == SMSG_LFG_BOOT_PLAYER)
    {
        Player* bot = GetPlayer();
        if (bot)
        {
            try
            {
                // Clone the packet data for reading (packet is const)
                WorldPacket packetCopy(*packet);
                packetCopy.rpos(0);

                // Parse LfgBootInfo structure:
                // - Bits<1> VoteInProgress
                // - Bits<1> VotePassed
                // - Bits<1> MyVoteCompleted
                // - Bits<1> MyVote
                // - SizedString::BitsSize<9> Reason
                // - ObjectGuid Target
                // - uint32 TotalVotes, BootVotes
                // - int32 TimeLeft
                // - uint32 VotesNeeded
                // - SizedString::Data Reason

                // Read the first byte containing the 4 bools + start of reason length
                uint8 firstByte;
                packetCopy >> firstByte;

                bool voteInProgress = (firstByte & 0x01) != 0;
                // bool votePassed = (firstByte & 0x02) != 0;  // Not needed for auto-vote
                bool myVoteCompleted = (firstByte & 0x04) != 0;
                // bool myVote = (firstByte & 0x08) != 0;  // Not needed for auto-vote

                // Read rest of reason string length (bits 4-12, 9 bits total starting at bit 4)
                uint8 secondByte;
                packetCopy >> secondByte;
                uint32 reasonLength = ((firstByte >> 4) & 0x0F) | ((secondByte & 0x1F) << 4);

                // Read Target GUID
                ObjectGuid target;
                packetCopy >> target;

                // Skip TotalVotes, BootVotes, TimeLeft, VotesNeeded (4 uint32s)
                uint32 totalVotes, bootVotes, votesNeeded;
                int32 timeLeft;
                packetCopy >> totalVotes >> bootVotes >> timeLeft >> votesNeeded;
                (void)totalVotes; (void)bootVotes; (void)timeLeft; (void)votesNeeded;  // Suppress unused warnings

                // Read reason string
                std::string reason;
                if (reasonLength > 0)
                {
                    reason.resize(reasonLength);
                    packetCopy.read(reinterpret_cast<uint8*>(reason.data()), reasonLength);
                }

                TC_LOG_DEBUG("module.playerbot.lfg",
                    "🗳️ BotSession: Boot packet for {} - inProgress={}, completed={}, target={}, reason={}",
                    bot->GetName(), voteInProgress, myVoteCompleted, target.ToString(), reason);

                // Only vote if:
                // 1. Vote is in progress
                // 2. We haven't already voted
                // 3. We're not the target (can't vote to kick yourself)
                if (voteInProgress && !myVoteCompleted && target != bot->GetGUID())
                {
                    TC_LOG_INFO("module.playerbot.lfg",
                        "🗳️ BotSession: Bot {} auto-voting YES to kick {} (reason: {})",
                        bot->GetName(), target.ToString(), reason);

                    // Auto-vote YES to allow kicking
                    sLFGMgr->UpdateBoot(bot->GetGUID(), true);

                    TC_LOG_INFO("module.playerbot.lfg",
                        "✅ BotSession: Bot {} voted YES to kick {}",
                        bot->GetName(), target.ToString());
                }
                else if (target == bot->GetGUID())
                {
                    TC_LOG_INFO("module.playerbot.lfg",
                        "⚠️ BotSession: Bot {} is being vote-kicked (reason: {})",
                        bot->GetName(), reason);
                }
            }
            catch (ByteBufferPositionException const& ex)
            {
                TC_LOG_ERROR("module.playerbot.lfg",
                    "❌ BotSession: Failed to parse LFG boot packet for bot {}: {}",
                    bot->GetName(), ex.what());
            }
        }
    }

    // PHASE 1: Relay packets to human group members
    // This enables bot damage/chat to appear in human combat logs and chat
    BotPacketRelay::RelayToGroupMembers(this, packet);

    // Store packet in outgoing queue for debugging/logging
    // ST-2 FIX: Simple mutex instead of recursive_timed_mutex
    ::std::lock_guard<::std::mutex> lock(_packetMutex);

    // Create a copy of the packet
    auto packetCopy = ::std::make_unique<WorldPacket>(*packet);
    _outgoingPackets.push(::std::move(packetCopy));
}

// TrinityCore 12.0: Override for new WorldSession::QueuePacket signature
void BotSession::QueuePacket(WorldPacket&& packet)
{
    // Store the packet in incoming queue
    // ST-2 FIX: Simple mutex instead of recursive_timed_mutex
    ::std::lock_guard<::std::mutex> lock(_packetMutex);

    // Move the packet into a unique_ptr for storage
    auto packetCopy = ::std::make_unique<WorldPacket>(::std::move(packet));
    _incomingPackets.push(::std::move(packetCopy));
}

// Legacy compatibility method (deprecated - prefer QueuePacket(WorldPacket&&))
void BotSession::QueuePacketLegacy(WorldPacket* packet)
{
    if (!packet) return;

    // Simple packet handling - just store in incoming queue
    // ST-2 FIX: Simple mutex instead of recursive_timed_mutex
    ::std::lock_guard<::std::mutex> lock(_packetMutex);

    // Create a copy of the packet
    auto packetCopy = ::std::make_unique<WorldPacket>(*packet);
    _incomingPackets.push(::std::move(packetCopy));
}

bool BotSession::Update(uint32 diff, PacketFilter& updater)
{
    // OPTION 1: TIMED LOCK WITH SHUTDOWN SAFEGUARDS
    // - Prevents Ghost aura race condition (concurrent ResurrectPlayer calls)
    // - Prevents shutdown deadlock (100ms timeout)
    // - Prevents strategy freezing (skips update instead of blocking)
    ::std::unique_lock<::std::timed_mutex> lock(_updateMutex, ::std::defer_lock);

    // Try to acquire lock with 100ms timeout
    if (!lock.try_lock_for(::std::chrono::milliseconds(100)))
    {
        // Failed to acquire lock within 100ms

        // Check if shutting down - bail immediately
        if (_destroyed.load() || !_active.load())
        {
            TC_LOG_DEBUG("module.playerbot.session", "BotSession::Update lock timeout - shutting down (account {})", GetAccountId());
            return false;  // Shutdown detected - exit gracefully (triggers disconnect)
        }

        // Otherwise: Another thread is updating this bot
        // CRITICAL FIX: Return TRUE to indicate "session is healthy, just busy"
        // Returning false here was incorrectly triggering bot disconnections!
        // The caller (BotWorldSessionMgr) treats false as "session needs removal"
        // but lock contention is temporary - the bot should remain connected.
        TC_LOG_DEBUG("module.playerbot.session", "BotSession::Update LOCK CONTENTION for account {} - skipping this update cycle", GetAccountId());
        return true;  // Session is healthy, just skip this update cycle
    }

    // Successfully acquired lock - proceed with update
    // DIAGNOSTIC: Track update entry for debugging - PER SESSION instead of global
    static thread_local uint32 updateCounter = 0;
    uint32 thisUpdateId = ++updateCounter;

    // CRITICAL MEMORY CORRUPTION DETECTION: Comprehensive session validation
    if (!_active.load() || _destroyed.load())
    {
        if (thisUpdateId <= 200) { // Log first 200 per-session failures
            TC_LOG_WARN("module.playerbot.session", " Update #{} EARLY RETURN: _active={} _destroyed={}",
                thisUpdateId, _active.load(), _destroyed.load());
        }
        return false;
    }

    // PLAYERBOT FIX: Check for logout/kick request - return false to trigger safe deletion
    // This prevents Map.cpp:686 crash by deferring player removal to main thread
    // forceExit is set by KickPlayer() when BotWorldEntry::Cleanup() is called
    // m_playerLogout is set by LogoutPlayer()
    if (forceExit || m_playerLogout)
    {
        TC_LOG_DEBUG("module.playerbot.session",
            "BotSession logout/kick requested for account {} (forceExit={}, m_playerLogout={}) - returning false for safe deletion",
            GetAccountId(), forceExit, m_playerLogout);
        return false;  // Triggers session removal in BotWorldSessionMgr::UpdateSessions()
    }

    // CRITICAL SAFETY: Validate session integrity before any operations
    uint32 accountId = GetAccountId();
    if (accountId == 0) {
        TC_LOG_ERROR("module.playerbot.session", " Update #{} EARLY RETURN: GetAccountId() returned 0", thisUpdateId);
        _active.store(false);
        return false;
    }

    // MEMORY CORRUPTION DETECTION: Validate critical member variables
    if (_bnetAccountId == 0 || _bnetAccountId != accountId) {
        TC_LOG_ERROR("module.playerbot.session", " Update #{} EARLY RETURN: Account ID mismatch - BnetAccount: {}, GetAccount: {}",
            thisUpdateId, _bnetAccountId, accountId);
        _active.store(false);
        return false;
    }

    // THREAD SAFETY: Validate we're not in a recursive Update call
    static thread_local bool inUpdateCall = false;
    if (inUpdateCall)
    {
        TC_LOG_ERROR("module.playerbot.session", " Update #{} EARLY RETURN: Recursive call detected for account {}", thisUpdateId, accountId);
        return false;
    }

    // RAII guard to prevent recursive calls
    struct UpdateGuard {
        bool& flag;
        explicit UpdateGuard(bool& f) : flag(f) { flag = true; }
        ~UpdateGuard() { flag = false; }
    } guard(inUpdateCall);

    try {

        // Process async login callbacks
        ProcessPendingLogin();

        // CRITICAL FIX: Process query holder callbacks (missing from original implementation)
        // This is required for async login callbacks to be processed
        ProcessQueryCallbacks();

        // PHASE 1 REFACTORING: Update packet simulator for periodic time synchronization
    if (_packetSimulator)
        {
            _packetSimulator->Update(diff);
        }

        // Process bot-specific packets
        ProcessBotPackets();

        // ========================================================================
        // FAR TELEPORT COMPLETION (LFG Dungeon Entry Fix)
        // ========================================================================
        // When a bot is teleported to a different map (e.g., entering a dungeon via LFG),
        // TrinityCore removes them from the old map and waits for a client response
        // (CMSG_SUSPEND_TOKEN_RESPONSE) before adding them to the new map.
        // Since bots have no client, we must simulate this response by calling
        // HandleMoveWorldportAck() when the bot is in "teleporting far" state.
        //
        // INSTANCE SYNC FIX: For LFG dungeon teleports, bots must wait until another
        // group member (the human player) has entered the dungeon first. This ensures
        // all group members enter the SAME instance. Without this, there's a race
        // condition where bots might create their own instance before the group's
        // instance is established.
        // ========================================================================
        if (GetPlayer() && GetPlayer()->IsBeingTeleportedFar())
        {
            Player* bot = GetPlayer();
            TeleportLocation const& dest = bot->GetTeleportDest();
            MapEntry const* destMapEntry = sMapStore.LookupEntry(dest.Location.GetMapId());

            bool shouldWaitForGroup = false;
            uint32 targetInstanceId = 0;

            // Check if teleporting to a dungeon with a group (LFG scenario)
            if (destMapEntry && destMapEntry->IsDungeon())
            {
                Group* group = bot->GetGroup();
                if (group && group->isLFGGroup())
                {
                    TC_LOG_DEBUG("module.playerbot.session",
                        "Bot {} instance sync check: DestMap={}, GroupSize={}, LfgDungeonId={}",
                        bot->GetName(), dest.Location.GetMapId(), group->GetMembersCount(),
                        dest.LfgDungeonsId.value_or(0));

                    // Check if any group member is already in the destination dungeon
                    bool groupMemberInDungeon = false;
                    bool hasHumanWaitingToTeleport = false;
                    bool thisIsFirstBot = true; // Used when all members are bots
                    uint32 membersFound = 0;

                    // FIXED: Use GroupMemberResolver instead of GetMembers()
                    for (auto const& slot : group->GetMemberSlots())
                    {
                        Player* member = GroupMemberResolver::ResolveMember(slot.guid);
                        if (!member || member == bot)
                            continue;

                        membersFound++;
                        WorldSession* memberSession = member->GetSession();
                        bool isBot = memberSession ? memberSession->IsBot() : false;

                        TC_LOG_DEBUG("module.playerbot.session",
                            "Bot {} checking member {}: IsInWorld={}, MapId={}, IsBeingTeleportedFar={}, IsBot={}",
                            bot->GetName(), member->GetName(),
                            member->IsInWorld(), member->GetMapId(),
                            member->IsBeingTeleportedFar(), isBot);

                        // Check if member is already in the dungeon
                        if (member->IsInWorld() && member->GetMapId() == dest.Location.GetMapId())
                        {
                            groupMemberInDungeon = true;
                            targetInstanceId = member->GetInstanceId();
                            TC_LOG_INFO("module.playerbot.session",
                                "Bot {} found group member {} already in dungeon (MapId={}, InstanceId={})",
                                bot->GetName(), member->GetName(), member->GetMapId(), targetInstanceId);
                            break;
                        }

                        // Check if a human player is also waiting to teleport
                        // Human players have real WorldSession with socket connection
                        if (member->IsBeingTeleportedFar())
                        {
                            WorldSession* memberSession = member->GetSession();
                            if (memberSession && !memberSession->IsBot())
                            {
                                hasHumanWaitingToTeleport = true;
                                TC_LOG_DEBUG("module.playerbot.session",
                                    "Bot {} detected human {} also waiting to teleport - will wait",
                                    bot->GetName(), member->GetName());
                            }
                            else
                            {
                                // Another bot is also waiting - check if we should go first
                                // Use GUID comparison to ensure deterministic ordering
                                if (member->GetGUID() < bot->GetGUID())
                                    thisIsFirstBot = false;
                            }
                        }
                    }

                    TC_LOG_DEBUG("module.playerbot.session",
                        "Bot {} instance sync summary: MembersFound={}, InDungeon={}, HumanWaiting={}, FirstBot={}",
                        bot->GetName(), membersFound, groupMemberInDungeon, hasHumanWaitingToTeleport, thisIsFirstBot);

                    // Decide whether to wait:
                    // - If a group member is already in dungeon: DON'T wait (join their instance)
                    // - If a human is waiting to teleport: WAIT for them (with timeout)
                    // - If only bots are waiting: First bot (by GUID) goes, others wait
                    if (!groupMemberInDungeon)
                    {
                        if (hasHumanWaitingToTeleport)
                        {
                            shouldWaitForGroup = true;
                            TC_LOG_DEBUG("module.playerbot.session",
                                "Bot {} waiting for human player to enter dungeon first (MapId={})",
                                bot->GetName(), dest.Location.GetMapId());
                        }
                        else if (!thisIsFirstBot)
                        {
                            shouldWaitForGroup = true;
                            TC_LOG_DEBUG("module.playerbot.session",
                                "Bot {} waiting for another bot to enter dungeon first (MapId={})",
                                bot->GetName(), dest.Location.GetMapId());
                        }
                        else
                        {
                            TC_LOG_DEBUG("module.playerbot.session",
                                "Bot {} is first to enter dungeon (MapId={}) - proceeding",
                                bot->GetName(), dest.Location.GetMapId());
                        }
                    }

                    // TIMEOUT SAFETY: If waiting too long, proceed anyway to avoid being stuck
                    // This handles edge cases like human disconnecting during teleport
                    constexpr uint32 INSTANCE_SYNC_TIMEOUT_MS = 30 * IN_MILLISECONDS; // 30 seconds
                    uint32 currentTimeMs = GameTime::GetGameTimeMS();

                    if (shouldWaitForGroup)
                    {
                        // Start tracking wait time if not already waiting
                        uint32 waitStart = _instanceSyncWaitStartMs.load();
                        if (waitStart == 0)
                        {
                            _instanceSyncWaitStartMs.store(currentTimeMs);
                            TC_LOG_DEBUG("module.playerbot.session",
                                "Bot {} started instance sync wait at {}ms",
                                bot->GetName(), currentTimeMs);
                        }
                        else
                        {
                            uint32 waitDuration = currentTimeMs - waitStart;
                            if (waitDuration >= INSTANCE_SYNC_TIMEOUT_MS)
                            {
                                TC_LOG_WARN("module.playerbot.session",
                                    "Bot {} instance sync TIMEOUT after {}ms - proceeding anyway (MapId={})",
                                    bot->GetName(), waitDuration, dest.Location.GetMapId());
                                shouldWaitForGroup = false;
                                _instanceSyncWaitStartMs.store(0); // Reset for next teleport
                            }
                            else
                            {
                                TC_LOG_DEBUG("module.playerbot.session",
                                    "Bot {} waiting for instance sync ({}ms / {}ms)",
                                    bot->GetName(), waitDuration, INSTANCE_SYNC_TIMEOUT_MS);
                            }
                        }
                    }
                    else
                    {
                        // Not waiting anymore, reset timer
                        _instanceSyncWaitStartMs.store(0);
                    }
                }
            }

            // Only complete teleport if:
            // 1. Not a dungeon (no instance sync needed)
            // 2. Not in an LFG group (no sync needed)
            // 3. A group member is already in the dungeon (instance exists)
            // 4. Timeout exceeded (safety mechanism)
            if (!shouldWaitForGroup)
            {
                // Reset wait timer since we're proceeding
                _instanceSyncWaitStartMs.store(0);

                TC_LOG_INFO("module.playerbot.session",
                    "Bot {} completing far teleport via deferred HandleMoveWorldportAck() (TargetInstanceId={}, MapId={})",
                    bot->GetName(), targetInstanceId, dest.Location.GetMapId());

                // ============================================================================
                // CRITICAL FIX: Defer HandleMoveWorldportAck to main thread
                // ============================================================================
                // The crash at Creature::AddToWorld (unordered_map::try_emplace during rehash)
                // occurs because HandleMoveWorldportAck() calls Map::AddPlayerToMap() which
                // triggers grid loading and creature spawning. These Map operations are NOT
                // thread-safe and must run on the main thread.
                //
                // Solution: Queue CMSG_WORLD_PORT_RESPONSE packet to deferred queue.
                // This packet will be processed by ProcessDeferredPackets() on the main thread
                // during World::UpdateSessions(), ensuring thread safety with Map::Update().
                // ============================================================================
                auto worldportPacket = ::std::make_unique<WorldPacket>(CMSG_WORLD_PORT_RESPONSE, 0);
                QueueDeferredPacket(::std::move(worldportPacket));

                TC_LOG_DEBUG("module.playerbot.session",
                    "Bot {} queued CMSG_WORLD_PORT_RESPONSE for main thread processing",
                    bot->GetName());
            }
        }

        // =======================================================================
        // ENTERPRISE-GRADE _recvQueue PACKET PROCESSING
        // =======================================================================
        // CRITICAL FIX: Process _recvQueue for bot-initiated packets
        // Problem: WorldSession::Update() line 385 checks m_Socket[CONNECTION_TYPE_REALM]
        //          which is nullptr for bots, preventing _recvQueue processing
        // Solution: Replicate WorldSession packet processing WITHOUT socket check
        // Benefits: Enables ALL packet-based bot features (resurrection, trades, etc.)
        // Thread Safety: Packets processed in bot worker thread (same as BotAI::Update)
        // =======================================================================

        WorldPacket* packet = nullptr;
        uint32 processedPackets = 0;
        time_t currentTime = GameTime::GetGameTime();

        // Performance limit: Same as WorldSession::Update() to prevent infinite loops
        constexpr uint32 MAX_PROCESSED_PACKETS_IN_SAME_WORLDSESSION_UPDATE = 100;

        // Process all queued packets (resurrection, future features, etc.)
        // Note: No socket check needed for bots (sockets are nullptr by design)
    while (processedPackets < MAX_PROCESSED_PACKETS_IN_SAME_WORLDSESSION_UPDATE &&
               _recvQueue.next(packet))
        {
            OpcodeClient opcode = static_cast<OpcodeClient>(packet->GetOpcode());
            ClientOpcodeHandler const* opHandle = opcodeTable[opcode];

            // ======================================================================
            // CRITICAL: SELECTIVE DEFERRAL - Only defer packets that need main thread
            // ======================================================================
    if (PacketDeferralClassifier::RequiresMainThread(opcode))
            {
                TC_LOG_TRACE("playerbot.packets",
                    "Bot {} DEFERRING packet opcode {} ({}) to main thread - Reason: {}",
                    GetPlayerName(),
                    static_cast<uint32>(opcode),
                    opHandle->Name,
                    PacketDeferralClassifier::GetDeferralReason(opcode));

                // Transfer ownership to deferred queue (processed by World::UpdateSessions)
                QueueDeferredPacket(::std::unique_ptr<WorldPacket>(packet));
                packet = nullptr; // Ownership transferred
                processedPackets++;
                continue; // Skip to next packet
            }
            // ======================================================================

            TC_LOG_TRACE("playerbot.packets",
                "Bot {} processing packet opcode {} ({}) with status {} on WORKER THREAD",
                GetPlayerName(),
                static_cast<uint32>(opcode),
                opHandle->Name,
                static_cast<uint32>(opHandle->Status));

            try
            {
                // Process based on opcode status (mirrors WorldSession::Update logic)
    switch (opHandle->Status)
                {
                    case STATUS_LOGGEDIN:
                    {
                        // Most common case: Player must be logged in and in world
                        // CMSG_RECLAIM_CORPSE falls into this category
                        Player* player = GetPlayer();
                        if (!player)
                        {
                            TC_LOG_WARN("playerbot.packets",
                                "Bot {} received STATUS_LOGGEDIN opcode {} but player is nullptr",
                                GetPlayerName(), opHandle->Name);
                            break;
                        }

                        if (!player->IsInWorld())
                        {
                            TC_LOG_WARN("playerbot.packets",
                                "Bot {} received STATUS_LOGGEDIN opcode {} but player not in world",
                                GetPlayerName(), opHandle->Name);
                            break;
                        }

                        // Execute opcode handler (e.g., HandleReclaimCorpse)
                        // This is thread-safe: Handler runs in bot worker thread context
                        opHandle->Call(this, *packet);

                        TC_LOG_DEBUG("playerbot.packets",
                            " Bot {} executed opcode {} ({}) handler successfully",
                            GetPlayerName(),
                            opHandle->Name,
                            static_cast<uint32>(opcode));
                        break;
                    }

                    case STATUS_LOGGEDIN_OR_RECENTLY_LOGGOUT:
                    {
                        // Opcodes valid during logout process
                        // Bots don't have traditional logout, but include for completeness
                        opHandle->Call(this, *packet);
                        TC_LOG_DEBUG("playerbot.packets",
                            " Bot {} executed opcode {} (STATUS_LOGGEDIN_OR_RECENTLY_LOGGOUT)",
                            GetPlayerName(), opHandle->Name);
                        break;
                    }

                    case STATUS_TRANSFER:
                    {
                        // Opcodes valid during map transfer
                        Player* player = GetPlayer();
                        if (player && !player->IsInWorld())
                        {
                            opHandle->Call(this, *packet);

                            TC_LOG_DEBUG("playerbot.packets",
                                " Bot {} executed opcode {} (STATUS_TRANSFER)",
                                GetPlayerName(), opHandle->Name);
                        }
                        else
                        {
                            TC_LOG_WARN("playerbot.packets",
                                "Bot {} received STATUS_TRANSFER opcode {} but player state invalid",
                                GetPlayerName(), opHandle->Name);
                        }
                        break;
                    }

                    case STATUS_AUTHED:
                    {
                        // Opcodes valid after authentication (character select, etc.)
                        // Bots typically skip character select, but include for future features
                        opHandle->Call(this, *packet);

                        TC_LOG_DEBUG("playerbot.packets",
                            " Bot {} executed opcode {} (STATUS_AUTHED)",
                            GetPlayerName(), opHandle->Name);
                        break;
                    }

                    case STATUS_NEVER:
                    {
                        TC_LOG_ERROR("playerbot.packets",
                            " Bot {} received NEVER-allowed opcode {} ({})",
                            GetPlayerName(),
                            opHandle->Name,
                            static_cast<uint32>(opcode));
                        break;
                    }

                    case STATUS_UNHANDLED:
                    {
                        TC_LOG_ERROR("playerbot.packets",
                            " Bot {} received UNHANDLED opcode {} ({})",
                            GetPlayerName(),
                            opHandle->Name,
                            static_cast<uint32>(opcode));
                        break;
                    }

                    case STATUS_IGNORED:
                    {
                        // Silently ignore (e.g., deprecated opcodes)
                        TC_LOG_TRACE("playerbot.packets",
                            "Bot {} ignored opcode {} (STATUS_IGNORED)",
                            GetPlayerName(), opHandle->Name);
                        break;
                    }

                    default:
                    {
                        TC_LOG_ERROR("playerbot.packets",
                            " Bot {} received opcode {} with UNKNOWN status {}",
                            GetPlayerName(),
                            opHandle->Name,
                            static_cast<uint32>(opHandle->Status));
                        break;
                    }
                }
            }
            catch (WorldPackets::InvalidHyperlinkException const& ihe)
            {
                TC_LOG_ERROR("playerbot.packets",
                    "InvalidHyperlinkException processing opcode {} for bot {}: {}",
                    opHandle->Name, GetPlayerName(), ihe.GetInvalidValue());
                // Continue processing other packets
            }
            catch (WorldPackets::PacketArrayMaxCapacityException const& pamce)
            {
                TC_LOG_ERROR("playerbot.packets",
                    "PacketArrayMaxCapacityException processing opcode {} for bot {}: {}",
                    opHandle->Name, GetPlayerName(), pamce.what());
                // Continue processing other packets
            }
            catch (ByteBufferException const& bbe)
            {
                TC_LOG_ERROR("playerbot.packets",
                    "ByteBufferException processing opcode {} for bot {}: {}",
                    opHandle->Name, GetPlayerName(), bbe.what());
                packet->hexlike();
                // Continue processing other packets
            }
            catch (::std::exception const& ex)
            {
                TC_LOG_ERROR("playerbot.packets",
                    "Unexpected exception processing opcode {} for bot {}: {}",
                    opHandle->Name, GetPlayerName(), ex.what());
                // Continue processing other packets
            }
            catch (...)
            {
                TC_LOG_ERROR("playerbot.packets",
                    "Unknown exception processing opcode {} for bot {}",
                    opHandle->Name, GetPlayerName());
                // Continue processing other packets
            }

            // Always delete packet after processing (prevents memory leaks)
            delete packet;
            packet = nullptr;
            processedPackets++;
        }

        // Log packet processing statistics (TRACE level to avoid spam)
    if (processedPackets > 0)
        {
            TC_LOG_TRACE("playerbot.packets",
                "Bot {} processed {} packets this update cycle (elapsed: {}ms)",
                GetPlayerName(), processedPackets, diff);
        }

        // Update AI if available and player is valid
        // CRITICAL FIX: Add comprehensive memory safety validation to prevent ACCESS_VIOLATION
        Player* player = GetPlayer();

        // DIAGNOSTIC: Log if initial conditions fail (throttled)
        if (!(_ai && player && _active.load() && !_destroyed.load()))
        {
            static ::std::unordered_map<uint32, uint32> lastInitFailLog;
            uint32 now = GameTime::GetGameTimeMS();
            if (lastInitFailLog.find(accountId) == lastInitFailLog.end() || (now - lastInitFailLog[accountId]) > 10000)
            {
                TC_LOG_ERROR("module.playerbot.session",
                    "⚠️ AI BLOCK SKIPPED for account {} - INITIAL CHECK FAILED: ai={}, player={}, active={}, destroyed={}",
                    accountId, _ai != nullptr, player != nullptr, _active.load(), _destroyed.load());
                lastInitFailLog[accountId] = now;
            }
        }

    if (_ai && player && _active.load() && !_destroyed.load())
    {

            // MEMORY CORRUPTION DETECTION: Validate player object pointer before access
            // Check for common corruption patterns (null and debug heap patterns only)
            uintptr_t playerPtr = reinterpret_cast<uintptr_t>(player);
            if (playerPtr == 0 || playerPtr == 0xDDDDDDDD || playerPtr == 0xCDCDCDCD ||
                playerPtr == 0xFEEEFEEE || playerPtr == 0xCCCCCCCC) {
                TC_LOG_ERROR("module.playerbot.session", "MEMORY CORRUPTION: Invalid player pointer 0x{:X} for account {}", playerPtr, accountId);
                _active.store(false);
                _ai = nullptr; // Clear AI to prevent further access
                return false;
            }

            // Additional check: pointer should be within reasonable address space (not too low)
    if (playerPtr < 0x10000)
    {
                TC_LOG_ERROR("module.playerbot.session", "MEMORY CORRUPTION: Player pointer 0x{:X} in low address space for account {}", playerPtr, accountId);
                _active.store(false);
                _ai = nullptr;
                return false;
            }

            // Wrap ALL player object access in structured exception handling
            try {
                // MEMORY SAFETY: Multi-layered validation before calling IsInWorld()
                bool playerIsValid = false;
                bool playerIsInWorld = false;

                // Layer 1: Basic object validation
                try {
                    // Test minimal access first - GetGUID is usually safe
                    ObjectGuid playerGuid = player->GetGUID();
                    if (playerGuid.IsEmpty())
                    {
                        TC_LOG_ERROR("module.playerbot.session", "Player has invalid GUID for account {}", accountId);
                        playerIsValid = false;
                    } else {
                        playerIsValid = true;
                    }
                }
                catch (...)
                {
                    TC_LOG_ERROR("module.playerbot.session", "Access violation in Player::GetGUID() for account {}", accountId);
                    playerIsValid = false;
                }

                // Layer 2: World state validation (only if basic validation passed)
    if (playerIsValid)
    {
                    try {
                        playerIsInWorld = player->IsInWorld();
                    }
                    catch (...)
                    {
                        TC_LOG_ERROR("module.playerbot.session", "Access violation in Player::IsInWorld() for account {}", accountId);
                        playerIsValid = false;
                        playerIsInWorld = false;
                    }
                }

                // Layer 3: AI update - ATOMIC SNAPSHOT to prevent race conditions
                // FIX: Capture all values ONCE before any checks to prevent race conditions
                // Problem: Between debug log and if statement, another thread could change values
                // Solution: Single atomic read of all conditions
                bool validSnapshot = playerIsValid;
                bool inWorldSnapshot = playerIsInWorld;
                BotAI* aiSnapshot = _ai.get();  // P1 FIX: Get raw pointer from unique_ptr
                bool activeSnapshot = _active.load(::std::memory_order_acquire);  // Acquire semantics

                // DEBUG LOGGING THROTTLE: Only log for test bots every 50 seconds
                static const ::std::set<::std::string> testBots = {"Anderenz", "Boone", "Nelona", "Sevtap"};
                static ::std::unordered_map<::std::string, uint32> sessionLogAccumulators;
                Player* botPlayer = GetPlayer();
                bool isTestBot = botPlayer && (testBots.find(botPlayer->GetName()) != testBots.end());
                bool shouldLog = false;

                if (isTestBot)
                {
                    ::std::string botName = botPlayer->GetName();
                    // Throttle by call count (every 1000 calls ~= 50s)
                    sessionLogAccumulators[botName]++;
                    if (sessionLogAccumulators[botName] >= 1000)
                    {
                        shouldLog = true;
                        sessionLogAccumulators[botName] = 0;
                    }
                }

                if (shouldLog)
                {
                    TC_LOG_INFO("module.playerbot.session", " BotSession Update Check - valid:{}, inWorld:{}, ai:{}, active:{}, account:{}",
                                validSnapshot, inWorldSnapshot, aiSnapshot != nullptr, activeSnapshot, accountId);
                }

                // Use SNAPSHOT values (not original variables) to prevent race conditions
    if (validSnapshot && inWorldSnapshot && aiSnapshot && activeSnapshot)
    {
                    if (shouldLog)
                    {
                        TC_LOG_INFO("module.playerbot.session", " ALL CONDITIONS MET - Calling UpdateAI for account {}", accountId);
                    }
                    try {
                        // Call UpdateAI using snapshot pointer (guaranteed non-null and stable)
                        aiSnapshot->UpdateAI(diff);
                    }
                    catch (::std::exception const& e)
                    {
                        TC_LOG_ERROR("module.playerbot.session", "Exception in BotAI::Update for account {}: {}", accountId, e.what());
                        // Don't propagate AI exceptions to prevent session crashes
                    }
                    catch (...)
                    {
                        TC_LOG_ERROR("module.playerbot.session", "Access violation in BotAI::Update for account {}", accountId);
                        // Clear AI to prevent further crashes
                        _ai = nullptr;
                    }
                } else {
                    // DIAGNOSTIC: Log why UpdateAI is NOT being called (throttled to 1 per 10s per account)
                    static ::std::unordered_map<uint32, uint32> lastFailLog;
                    uint32 now = GameTime::GetGameTimeMS();
                    if (lastFailLog.find(accountId) == lastFailLog.end() || (now - lastFailLog[accountId]) > 10000)
                    {
                        TC_LOG_ERROR("module.playerbot.session",
                            "⚠️ UpdateAI NOT called for account {} - CONDITIONS FAILED: valid={}, inWorld={}, ai={}, active={}",
                            accountId, validSnapshot, inWorldSnapshot, aiSnapshot != nullptr, activeSnapshot);
                        lastFailLog[accountId] = now;
                    }
                }
            }
            catch (...)
            {
                TC_LOG_ERROR("module.playerbot.session", "Critical exception in AI processing for account {}", accountId);
                // Deactivate session completely to prevent memory corruption cascade
                _active.store(false);
                _ai = nullptr;
                TC_LOG_ERROR("module.playerbot.session", "Deactivated BotSession {} due to critical memory corruption", accountId);
                return false; // Signal failure to caller
            }
        }

        // CRITICAL BUG FIX (2025-10-30): DO NOT call WorldSession::Update() from worker threads!
        //
        // PROBLEM: WorldSession::Update() is designed to run on the MAIN WORLD THREAD (see World::UpdateSessions line 3039).
        //          Calling it from bot worker threads creates RACE CONDITIONS with Map::Update() which runs on Map worker threads.
        //
        // EVIDENCE: Crash at GameObject::Update() → Map::GetCreature() → GUID comparison nullptr dereference
        //           Crash dump: 3f33f776e3ba+_worldserver.exe_[2025_10_30_7_49_6].txt
        //           Location: Map.cpp:3529 during GameObject ownership lookup
        //
        // ROOT CAUSE: Packet handlers executing on bot worker threads modify game state while Map::Update() runs concurrently,
        //             causing use-after-free/dangling reference crashes.
        //
        // SOLUTION: Bot packets (like CMSG_RECLAIM_CORPSE) must be processed on MAIN THREAD or via a thread-safe mechanism.
        //           Current approach is UNSAFE and causes crashes.
        //
        // KNOWN LIMITATION: Bot packet processing thread safety
        // Options for future fix:
        //   1. Process bot session packets in World::UpdateSessions() on main thread
        //   2. Use deferred packet queue that main thread processes
        //   3. Implement resurrection without packet-based approach (direct ResurrectPlayer call)
        //
        // TEMPORARY: Resurrection is broken but server won't crash from race conditions.

        return true; // Bot sessions always return success
    }
    catch (::std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot.session",
            "Exception in BotSession::Update for account {}: {}", GetAccountId(), e.what());
        return false;
    }
    catch (...)
    {
        TC_LOG_ERROR("module.playerbot.session",
            "Unknown exception in BotSession::Update for account {}", GetAccountId());
        return false;
    }
}

void BotSession::ProcessBotPackets()
{
    // CRITICAL SAFETY CHECK: Prevent access to destroyed objects
    if (_destroyed.load() || !_active.load())
    {
        return; // Object is being destroyed or inactive, abort immediately
    }

    // Use batch processing with optimized batch sizes for better performance
    constexpr size_t BATCH_SIZE = 32; // Optimized size for L1 cache efficiency

    // CRITICAL DEADLOCK FIX: Implement completely lock-free packet processing
    // Use atomic operations instead of mutex to prevent thread pool deadlocks

    // Batch process packets with atomic queue operations (lock-free)
    ::std::vector<::std::unique_ptr<WorldPacket>> incomingBatch;
    ::std::vector<::std::unique_ptr<WorldPacket>> outgoingBatch;
    incomingBatch.reserve(BATCH_SIZE);
    outgoingBatch.reserve(BATCH_SIZE);

    // LOCK-FREE IMPLEMENTATION: Use double-checked locking with atomic flag
    // This eliminates the recursive_timed_mutex that was causing deadlocks
    bool expected = false;
    if (!_packetProcessing.compare_exchange_strong(expected, true))
    {
        // Another thread is already processing packets - safe to skip
        TC_LOG_DEBUG("module.playerbot.session", "Packet processing already in progress for account {}, skipping", GetAccountId());
        return;
    }

    // Ensure processing flag is cleared on exit (RAII pattern)
    struct PacketProcessingGuard {
        ::std::atomic<bool>& flag;
        explicit PacketProcessingGuard(::std::atomic<bool>& f) : flag(f) {}
        ~PacketProcessingGuard() { flag.store(false); }
    } guard(_packetProcessing);

    // Double-check destroyed flag after acquiring processing rights
    if (_destroyed.load() || !_active.load())
    {
        return; // Object was destroyed while waiting
    }

    // PHASE 1: Quick extraction with minimal lock time
    // ST-2 FIX: Replace try_lock_for(5ms) with blocking lock_guard
    // Analysis shows lock hold time is ~100µs (queue push/pop only).
    // Blocking wait is more reliable than timeout failures which
    // caused cascading packet processing deferrals under high load.
    {
        ::std::lock_guard<::std::mutex> lock(_packetMutex);

        // Extract incoming packets atomically
        for (size_t i = 0; i < BATCH_SIZE && !_incomingPackets.empty(); ++i) {
            incomingBatch.emplace_back(::std::move(_incomingPackets.front()));
            _incomingPackets.pop();
        }

        // Extract outgoing packets (for logging/debugging)
        for (size_t i = 0; i < BATCH_SIZE && !_outgoingPackets.empty(); ++i) {
            outgoingBatch.emplace_back(::std::move(_outgoingPackets.front()));
            _outgoingPackets.pop();
        }
    } // Release lock immediately

    // PHASE 2: Process packets without holding any locks (deadlock-free)
    for (auto& packet : incomingBatch)
    {
        if (_destroyed.load() || !_active.load())
        {
            break; // Stop processing if session is being destroyed
        }

        try {
            // Process packet through WorldSession's standard queue system
            // This is safe to call without locks
            // TrinityCore 12.0: QueuePacket now takes WorldPacket&& instead of WorldPacket*
            WorldSession::QueuePacket(std::move(*packet));
        }
        catch (::std::exception const& e)
        {
            TC_LOG_ERROR("module.playerbot.session", "Exception processing incoming packet for account {}: {}", GetAccountId(), e.what());
        }
        catch (...)
        {
            TC_LOG_ERROR("module.playerbot.session", "Unknown exception processing incoming packet for account {}", GetAccountId());
        }
    }

    // Removed noisy packet statistics logging - was spamming logs
    // if (!outgoingBatch.empty())
    // {
    //     TC_LOG_DEBUG("module.playerbot.session", "Processed {} outgoing packets for account {}", outgoingBatch.size(), GetAccountId());
    // }
}

// ============================================================================
// DEFERRED PACKET SYSTEM - Main Thread Processing for Race Condition Prevention
// ============================================================================

void BotSession::QueueDeferredPacket(::std::unique_ptr<WorldPacket> packet)
{
    if (!packet)
        return;

    // Log BEFORE moving (packet will be invalid after move)
    OpcodeClient opcode = static_cast<OpcodeClient>(packet->GetOpcode());

    ::std::lock_guard lock(_deferredPacketMutex);
    _deferredPackets.emplace(::std::move(packet));

    TC_LOG_TRACE("playerbot.packets.deferred",
        "Bot {} queued packet opcode {} for main thread processing",
        GetPlayerName(), static_cast<uint32>(opcode));
}

uint32 BotSession::ProcessDeferredPackets()
{
    // CRITICAL: This must ONLY be called from World::UpdateSessions() on main thread!
    // Processing deferred packets on worker threads defeats the entire purpose.

    uint32 processed = 0;
    constexpr uint32 MAX_DEFERRED_PACKETS_PER_UPDATE = 50; // Prevent main thread starvation
    while (processed < MAX_DEFERRED_PACKETS_PER_UPDATE)
    {
        ::std::unique_ptr<WorldPacket> packet;

        // Quick lock to extract one packet
        {
            ::std::lock_guard lock(_deferredPacketMutex);
            if (_deferredPackets.empty())
                break;

            packet = ::std::move(_deferredPackets.front());
            _deferredPackets.pop();
        }

        // Process packet outside lock (avoid holding mutex during execution)
        try
        {
            OpcodeClient opcode = static_cast<OpcodeClient>(packet->GetOpcode());
            ClientOpcodeHandler const* opHandle = opcodeTable[opcode];

            TC_LOG_TRACE("playerbot.packets.deferred",
                "Bot {} processing deferred packet opcode {} ({}) on main thread",
                GetPlayerName(),
                static_cast<uint32>(opcode),
                opHandle->Name);

            // Process based on opcode status (same logic as ProcessBotPackets)
    switch (opHandle->Status)
            {
                case STATUS_LOGGEDIN:
                {
                    Player* player = GetPlayer();
                    if (!player)
                    {
                        TC_LOG_WARN("playerbot.packets.deferred",
                            "Bot {} deferred packet {} but player is nullptr",
                            GetPlayerName(), opHandle->Name);
                        break;
                    }

                    if (!player->IsInWorld())
                    {
                        TC_LOG_WARN("playerbot.packets.deferred",
                            "Bot {} deferred packet {} but player not in world",
                            GetPlayerName(), opHandle->Name);
                        break;
                    }

                    // Execute opcode handler on main thread (thread-safe with Map::Update)
                    opHandle->Call(this, *packet);

                    TC_LOG_DEBUG("playerbot.packets.deferred",
                        " Bot {} executed deferred opcode {} ({}) on main thread",
                        GetPlayerName(),
                        opHandle->Name,
                        static_cast<uint32>(opcode));
                    break;
                }

                case STATUS_LOGGEDIN_OR_RECENTLY_LOGGOUT:
                case STATUS_AUTHED:
                {
                    opHandle->Call(this, *packet);
                    break;
                }

                case STATUS_TRANSFER:
                {
                    // STATUS_TRANSFER packets (like CMSG_WORLD_PORT_RESPONSE) should only be processed
                    // when the player is in a valid transfer state: BeingTeleportedFar AND NOT in world.
                    // If player IS in world while BeingTeleportedFar, there's a state inconsistency
                    // that could cause ASSERT failure in Map::RemovePlayerFromMap (Map.cpp:935)
                    // when trying to remove from map with player not in grid.
                    Player* player = GetPlayer();
                    if (!player)
                    {
                        TC_LOG_WARN("playerbot.packets.deferred",
                            "Bot {} deferred STATUS_TRANSFER packet {} but player is nullptr",
                            GetPlayerName(), opHandle->Name);
                        break;
                    }

                    // Validate transfer state: Should be teleporting far and NOT in world
                    // (TeleportTo calls RemoveFromWorld before setting the far teleport semaphore)
                    if (player->IsInWorld())
                    {
                        // State inconsistency detected - player is in world but receiving transfer packet
                        // This could happen if player was re-added to world between TeleportTo and
                        // processing this deferred packet. Skip to prevent crash in RemovePlayerFromMap.
                        TC_LOG_ERROR("playerbot.packets.deferred",
                            "Bot {} deferred STATUS_TRANSFER packet {} SKIPPED - player in world but "
                            "receiving transfer packet (IsInWorld={}, IsBeingTeleportedFar={}, Map={})",
                            GetPlayerName(), opHandle->Name, player->IsInWorld(),
                            player->IsBeingTeleportedFar(), player->GetMapId());

                        // If player is in world and grid, they're already on a map - don't process transfer
                        // If player is in world but NOT in grid, that's a critical bug - log and skip
                        if (!player->IsInGrid())
                        {
                            TC_LOG_ERROR("playerbot.packets.deferred",
                                "CRITICAL: Bot {} is in world but NOT in grid (Map {}) - skipping "
                                "STATUS_TRANSFER to prevent assertion failure",
                                GetPlayerName(), player->GetMapId());
                        }
                        break;
                    }

                    // Player is NOT in world - safe to process transfer packet
                    opHandle->Call(this, *packet);

                    TC_LOG_DEBUG("playerbot.packets.deferred",
                        " Bot {} executed deferred STATUS_TRANSFER opcode {} on main thread",
                        GetPlayerName(), opHandle->Name);
                    break;
                }

                case STATUS_NEVER:
                case STATUS_UNHANDLED:
                case STATUS_IGNORED:
                default:
                {
                    TC_LOG_ERROR("playerbot.packets.deferred",
                        " Bot {} deferred packet has invalid status: {} (opcode {})",
                        GetPlayerName(),
                        static_cast<uint32>(opHandle->Status),
                        opHandle->Name);
                    break;
                }
            }

            processed++;
        }
        catch (::std::exception const& ex)
        {
            TC_LOG_ERROR("playerbot.packets.deferred",
                "Exception processing deferred packet for bot {}: {}",
                GetPlayerName(), ex.what());
        }
        catch (...)
        {
            TC_LOG_ERROR("playerbot.packets.deferred",
                "Unknown exception processing deferred packet for bot {}",
                GetPlayerName());
        }
    }

    if (processed > 0)
    {
        TC_LOG_DEBUG("playerbot.packets.deferred",
            "Bot {} processed {} deferred packets on main thread",
            GetPlayerName(), processed);
    }

    return processed;
}

bool BotSession::HasDeferredPackets() const
{
    ::std::lock_guard lock(_deferredPacketMutex);
    return !_deferredPackets.empty();
}

// ============================================================================

bool BotSession::LoginCharacter(ObjectGuid characterGuid)
{
    // Validate inputs
    if (characterGuid.IsEmpty())
    {
        TC_LOG_ERROR("module.playerbot.session", "BotSession::LoginCharacter called with empty character GUID");
        return false;
    }

    if (GetAccountId() == 0)
    {
        TC_LOG_ERROR("module.playerbot.session", "BotSession::LoginCharacter called with invalid account ID");
        return false;
    }

    // Check if already logging in or logged in
    LoginState expected = LoginState::NONE;
    if (!_loginState.compare_exchange_strong(expected, LoginState::LOGIN_IN_PROGRESS))
    {
        TC_LOG_ERROR("module.playerbot.session", "BotSession: Already logging in (state: {})", static_cast<uint8>(_loginState.load()));
        return false;
    }

    TC_LOG_INFO("module.playerbot.session", "Starting ASYNC login for character {} using TrinityCore LoginQueryHolder pattern", characterGuid.ToString());

    try
    {
        // PROPER ASYNC APPROACH: Use TrinityCore's standard LoginQueryHolder pattern
        // This fixes the database assertion failure by using the correct async connection pool

        // Store the character GUID for the pending login
        m_playerLoading = characterGuid;

        // Create LoginQueryHolder exactly like WorldSession does
        ::std::shared_ptr<BotLoginQueryHolder> holder = ::std::make_shared<BotLoginQueryHolder>(GetAccountId(), characterGuid);
        if (!holder->Initialize())
        {
            TC_LOG_ERROR("module.playerbot.session", "Failed to initialize BotLoginQueryHolder for character {}", characterGuid.ToString());
            m_playerLoading.Clear();
            _loginState.store(LoginState::LOGIN_FAILED);
            return false;
        }

        TC_LOG_INFO("module.playerbot.session", "Submitting async LoginQueryHolder for character {}", characterGuid.ToString());

        // Use the EXACT same pattern as WorldSession - AddQueryHolderCallback with DelayQueryHolder
        AddQueryHolderCallback(CharacterDatabase.DelayQueryHolder(holder)).AfterComplete([this](SQLQueryHolderBase const& holder)
        {
            TC_LOG_INFO("module.playerbot.session", "Async LoginQueryHolder callback received, processing bot player login");
            HandleBotPlayerLogin(static_cast<BotLoginQueryHolder const&>(holder));
        });

        TC_LOG_INFO("module.playerbot.session", " ASYNC bot login initiated for character {} - waiting for database callback", characterGuid.ToString());
        // Login state will be updated in HandleBotPlayerLogin callback
        return true;
    }
    catch (::std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot.session", "Exception in LoginCharacter: {}", e.what());
        m_playerLoading.Clear();
        _loginState.store(LoginState::LOGIN_FAILED);
        return false;
    }
    catch (...)
    {
        TC_LOG_ERROR("module.playerbot.session", "Unknown exception in LoginCharacter");
        m_playerLoading.Clear();
        _loginState.store(LoginState::LOGIN_FAILED);
        return false;
    }
}

void BotSession::ProcessPendingLogin()
{
    // Process any pending async login operations
    // The actual login is handled by the HandleBotPlayerLogin callback

    LoginState currentState = _loginState.load();
    if (currentState == LoginState::LOGIN_IN_PROGRESS)
    {
        TC_LOG_DEBUG("module.playerbot.session", "ProcessPendingLogin: Async login in progress, waiting for callback");
    }
}

// Handle the asynchronous login query holder callback (replaces LoadCharacterDataSynchronously)
void BotSession::HandleBotPlayerLogin(BotLoginQueryHolder const& holder)
{
    if (!IsActive() || !_active.load())
    {
        TC_LOG_ERROR("module.playerbot.session", "BotSession is not active during HandleBotPlayerLogin");
        _loginState.store(LoginState::LOGIN_FAILED);
        m_playerLoading.Clear();
        return;
    }

    ObjectGuid characterGuid = holder.GetGuid();
    TC_LOG_INFO("module.playerbot.session", "Processing async login callback for character {}", characterGuid.ToString());

    try
    {
        // P1 FIX: Create Player object with unique_ptr for automatic memory management
        // This eliminates the need for manual delete on error paths
        auto pCurrChar = ::std::make_unique<Player>(this);

        // Note: make_unique never returns nullptr, so this check is technically unnecessary
        // but kept for consistency with exception handling below
        if (!pCurrChar)
        {
            TC_LOG_ERROR("module.playerbot.session", "Failed to create Player object for character {}", characterGuid.ToString());
            _loginState.store(LoginState::LOGIN_FAILED);
            m_playerLoading.Clear();
            return;
        }

        // Use the async query holder to load character data
        // This uses the proper async connection pool, preventing the assertion failure
        // DEBUG: Log session info before LoadFromDB - the account mismatch check is at line 17874 of Player.cpp
        TC_LOG_INFO("module.playerbot.session", "LoadFromDB: Attempting load for {}, session accountId={}, holder accountId={}",
                    characterGuid.ToString(), GetAccountId(), holder.GetAccountId());

        // DEBUG: Check critical query results before LoadFromDB
        // PLAYER_LOGIN_QUERY_LOAD_FROM = 0 - Main character data
        // PLAYER_LOGIN_QUERY_LOAD_BANNED = 6 - Ban check
        // PLAYER_LOGIN_QUERY_LOAD_HOME_BIND = 7 - Home bind position
        // PLAYER_LOGIN_QUERY_LOAD_CUSTOMIZATIONS = 1 - Customization data
        auto mainResult = holder.GetPreparedResult(PLAYER_LOGIN_QUERY_LOAD_FROM);
        auto banResult = holder.GetPreparedResult(PLAYER_LOGIN_QUERY_LOAD_BANNED);
        auto homeBindResult = holder.GetPreparedResult(PLAYER_LOGIN_QUERY_LOAD_HOME_BIND);
        auto customizationsResult = holder.GetPreparedResult(PLAYER_LOGIN_QUERY_LOAD_CUSTOMIZATIONS);

        TC_LOG_INFO("module.playerbot.session", "LoadFromDB: Query results - LOAD_FROM={}, BANNED={}, HOME_BIND={}, CUSTOMIZATIONS={}",
                    mainResult ? "valid" : "NULL",
                    banResult ? "HAS_BAN" : "not_banned",
                    homeBindResult ? "valid" : "NULL",
                    customizationsResult ? "valid" : "NULL");

        // If main result is null, character doesn't exist in database
        if (!mainResult)
        {
            TC_LOG_ERROR("module.playerbot.session", "LoadFromDB: CRITICAL - PLAYER_LOGIN_QUERY_LOAD_FROM returned NULL for {}! Character may not exist.",
                        characterGuid.ToString());
        }

        // If banned, login will fail
        if (banResult)
        {
            TC_LOG_ERROR("module.playerbot.session", "LoadFromDB: Character {} is BANNED - login will fail",
                        characterGuid.ToString());
        }

        // If no home bind, first login flag may be needed
        if (!homeBindResult)
        {
            TC_LOG_WARN("module.playerbot.session", "LoadFromDB: No home bind for {} - may cause issues if not AT_LOGIN_FIRST",
                       characterGuid.ToString());
        }

    if (!pCurrChar->LoadFromDB(characterGuid, holder))
        {
            // P1 FIX: No manual delete needed - unique_ptr automatically cleans up
            // Old code (REMOVED): delete pCurrChar;
            TC_LOG_ERROR("module.playerbot.session", "Failed to load bot character {} from database - LoadFromDB returned false",
                        characterGuid.ToString());
            _loginState.store(LoginState::LOGIN_FAILED);
            m_playerLoading.Clear();
            return;
        }

        // Bot-specific initialization
        pCurrChar->SetVirtualPlayerRealm(GetVirtualRealmAddress());

        // CRITICAL FIX: Auto-assign proper specialization for level 10+ bots
        // TrinityCore assigns all new characters the "Initial" spec (index 4, e.g., Warlock=1454)
        // which has NO specialization spells. Real players pick a spec at level 10 via UI.
        // Bots need this done programmatically on login.
        if (pCurrChar->GetLevel() >= MIN_SPECIALIZATION_LEVEL)
        {
            ChrSpecializationEntry const* currentSpec = pCurrChar->GetPrimarySpecializationEntry();
            ChrSpecializationEntry const* initialSpec = sDB2Manager.GetDefaultChrSpecializationForClass(pCurrChar->GetClass());

            // Check if bot still has the "Initial" spec (no real spec chosen yet)
            if (currentSpec && initialSpec && currentSpec->ID == initialSpec->ID)
            {
                // Get the first real spec for this class (index 0 = first spec like Affliction, Arms, etc.)
                ChrSpecializationEntry const* firstSpec = sDB2Manager.GetChrSpecializationByIndex(pCurrChar->GetClass(), 0);
                if (firstSpec)
                {
                    TC_LOG_INFO("module.playerbot.session", "Bot {} has Initial spec ({}) at level {} - auto-assigning spec {}",
                        pCurrChar->GetName(), initialSpec->ID, pCurrChar->GetLevel(), firstSpec->ID);

                    // Set the real specialization
                    pCurrChar->SetPrimarySpecialization(firstSpec->ID);
                    pCurrChar->SetActiveTalentGroup(firstSpec->OrderIndex);

                    // Learn specialization spells using standard TrinityCore method
                    // This is safe because IsInWorld() is false at this point (before AddPlayerToMap),
                    // so LearnSpell() won't attempt to send packets
                    pCurrChar->LearnSpecializationSpells();

                    TC_LOG_INFO("module.playerbot.session", "Bot {} now has spec {} - specialization spells learned via LearnSpecializationSpells()",
                        pCurrChar->GetName(), firstSpec->ID);
                }
            }
        }

        // NOTE: Specialization spells are NOT saved to database in modern WoW
        // LoadFromDB() at line 18356 calls LearnSpecializationSpells() which loads spells from DB2 data
        // This is by design - spells are learned dynamically on each login

        // CRITICAL FIX: Ensure ALL class skills and spells are learned
        // LearnDefaultSkills() teaches class skills (e.g., Affliction skill for warlocks)
        // SetSkill() triggers LearnSkillRewardedSpells() which teaches spells like Summon Imp (688)
        // Without this, bots miss base class spells that aren't specialization-specific
        TC_LOG_DEBUG("module.playerbot.session", "Bot {} - Learning default skills and spells for level {}",
            pCurrChar->GetName(), pCurrChar->GetLevel());
        pCurrChar->LearnDefaultSkills();
        pCurrChar->UpdateSkillsForLevel();  // Ensures skill values match current level

        // Verify spell learning - in modern WoW (12.0), ALL combat spells are automatic
        // TrinityCore's LearnDefaultSkills() + LearnSpecializationSpells() should teach everything
        // This call is a safety net that re-calls the native methods in case of timing issues
        sBotPostLoginConfigurator->ApplyClassSpells(pCurrChar.get());  // P1 FIX: Use .get() to pass raw pointer
        TC_LOG_DEBUG("module.playerbot.session", "Bot {} - Spell learning verification complete",
            pCurrChar->GetName());

        // Clear the loading state
        m_playerLoading.Clear();

        // CRITICAL FIX: Add bot to world (missing step that prevented bots from entering world)
        pCurrChar->SendInitialPacketsBeforeAddToMap();

        // CRITICAL FIX: Player::LoadFromDB() already sets the map via SetMap() at Player.cpp:18257
        // We MUST use pCurrChar->GetMap() to get the map that was set during LoadFromDB()
        // DO NOT call CreateMap() separately - this can return a different map instance for
        // instanced dungeons, causing the assertion "player->GetMap() == this" to fail in AddPlayerToMap
        Map* map = pCurrChar->GetMap();
        if (!map)
        {
            TC_LOG_ERROR("module.playerbot.session",
                "CRITICAL: Bot {} has no map set after LoadFromDB! MapId={} InstanceId={} - Login FAILED",
                pCurrChar->GetName(), pCurrChar->GetMapId(), pCurrChar->GetInstanceId());
            _loginState.store(LoginState::LOGIN_FAILED);
            m_playerLoading.Clear();
            return;
        }

        TC_LOG_DEBUG("module.playerbot.session", "Bot {} using map from LoadFromDB: MapId={} InstanceId={} MapPtr=0x{:X}",
            pCurrChar->GetName(), map->GetId(), map->GetInstanceId(), reinterpret_cast<uintptr_t>(map));

        // Now safely add bot to the map (matching CharacterHandler.cpp:1273 pattern)
        if (!map->AddPlayerToMap(pCurrChar.get()))  // P1 FIX: Use .get() to pass raw pointer
        {
            TC_LOG_ERROR("module.playerbot.session", "Failed to add bot player {} to map", characterGuid.ToString());
            _loginState.store(LoginState::LOGIN_FAILED);
            m_playerLoading.Clear();
            return;
        }

        ObjectAccessor::AddObject(pCurrChar.get());  // P1 FIX: Use .get() to pass raw pointer

        // CRITICAL FIX: Following TrinityCore "golden source" pattern (CharacterHandler.cpp:1299)
        // SendInitialPacketsAfterAddToMap() internally calls:
        //   - PhasingHandler::OnMapChange(this) at Player.cpp:24805
        // This happens AFTER AddPlayerToMap(), matching real player login flow
        pCurrChar->SendInitialPacketsAfterAddToMap();

        // PHASE 1 REFACTORING: Create packet simulator for this bot session
        // This replaces manual workarounds with proper packet forging
        _packetSimulator = ::std::make_unique<BotPacketSimulator>(this);

        // PHASE 1 REFACTORING: Simulate CMSG_QUEUED_MESSAGES_END packet
        // Real clients send this after SMSG_RESUME_COMMS to resume communication
        // Triggers time synchronization and allows login to proceed
        _packetSimulator->SimulateQueuedMessagesEnd();
        // PHASE 1 REFACTORING: Simulate CMSG_MOVE_INIT_ACTIVE_MOVER_COMPLETE packet
        // Real clients send this after SMSG_MOVE_INIT_ACTIVE_MOVER
        // Automatically sets PLAYER_LOCAL_FLAG_OVERRIDE_TRANSPORT_SERVER_TIME
        // Enables object visibility (fixes CanNeverSee() check)
        // Updates player visibility
        _packetSimulator->SimulateMoveInitActiveMoverComplete();

        // PHASE 1 REFACTORING: Enable periodic time synchronization
        // Maintains clock delta calculations over time
        // Prevents movement prediction drift
        _packetSimulator->EnablePeriodicTimeSync();

        // DIAGNOSTIC: Log phase information after SendInitialPacketsAfterAddToMap
        PhaseShift const& phaseShift = pCurrChar->GetPhaseShift();
        ::std::string botPhases;
        auto const& phases = phaseShift.GetPhases();
        botPhases.reserve(phases.size() * 8); // Pre-allocate approximate size
    for (auto const& phaseRef : phases)
        {
            if (!botPhases.empty()) botPhases += ",";
            botPhases += ::std::to_string(phaseRef.Id);
        }
        if (botPhases.empty()) botPhases = "NONE";

        TC_LOG_INFO("module.playerbot.session", " Bot {} phase initialization complete (TrinityCore pattern + packet simulation) - Phases: [{}]",
            pCurrChar->GetName(), botPhases);
        TC_LOG_INFO("module.playerbot.session", "Bot player {} successfully added to world", pCurrChar->GetName());

        // ============================================================================
        // POST-LOGIN CONFIGURATION (JIT Bot Setup)
        // ============================================================================
        // For JIT-created bots, apply pending configuration (level, spec, talents, gear)
        // using proper Player APIs now that the bot is fully in the world.
        // This must happen BEFORE BotAI creation so the bot has correct stats/abilities.
        // ============================================================================

        // DIAGNOSTIC: Log bot state BEFORE config check
        TC_LOG_INFO("module.playerbot.session", "JIT CONFIG CHECK: Bot {} (GUID={}) PRE-CONFIG state: Level={}, Class={}, Race={}",
            pCurrChar->GetName(), characterGuid.ToString(), pCurrChar->GetLevel(), pCurrChar->GetClass(), pCurrChar->GetRace());

        if (sBotPostLoginConfigurator->HasPendingConfiguration(characterGuid))
        {
            TC_LOG_INFO("module.playerbot.session", "Bot {} has pending JIT configuration - applying post-login setup",
                pCurrChar->GetName());

            // P1 FIX: Use .get() to pass raw pointer to API that expects Player*
            if (sBotPostLoginConfigurator->ApplyPendingConfiguration(pCurrChar.get()))
            {
                TC_LOG_INFO("module.playerbot.session", "Bot {} post-login configuration applied successfully (Level={}, GS={})",
                    pCurrChar->GetName(), pCurrChar->GetLevel(), 0); // NOTE: Gear score calculation deferred to post-login systems
            }
            else
            {
                TC_LOG_ERROR("module.playerbot.session", "Bot {} post-login configuration FAILED - bot may have incomplete setup",
                    pCurrChar->GetName());
            }
        }
        else
        {
            // No pending config found - this is normal for regular bots, but for JIT bots it's a problem
            TC_LOG_WARN("module.playerbot.session", "JIT CONFIG CHECK: Bot {} (GUID={}) has NO PENDING CONFIG - Level stays at {}",
                pCurrChar->GetName(), characterGuid.ToString(), pCurrChar->GetLevel());
        }

        // P1 FIX: Transfer ownership of Player to WorldSession
        // CRITICAL: This must happen AFTER all pCurrChar usage above, but BEFORE GetPlayer() usage below
        // release() extracts the raw pointer, relinquishes ownership, and sets pCurrChar to nullptr
        SetPlayer(pCurrChar.release());

        // Create and assign BotAI to take control of the character
    if (GetPlayer())
        {
            auto botAI = sBotAIFactory->CreateAI(GetPlayer());
            if (botAI)
            {
                // P1 FIX: Pass unique_ptr directly via std::move (no .release() needed)
                SetAI(::std::move(botAI)); // Transfer ownership to BotSession
                TC_LOG_INFO("module.playerbot.session", "Successfully created BotAI for character {}", characterGuid.ToString());
                // CRITICAL FIX: Check if bot is already in a group at login and activate strategies
                // This fixes the "reboot breaks groups" issue where strategies aren't activated
    if (Player* player = GetPlayer())
                {
                    Group* group = player->GetGroup();
                    TC_LOG_ERROR("module.playerbot.session", " Bot {} login group check: player={}, group={}",
                                player->GetName(), (void*)player, (void*)group);
                    if (group)
                    {
                        TC_LOG_INFO("module.playerbot.session", " Bot {} is already in group at login - activating strategies", player->GetName());
                        if (BotAI* ai = GetAI())
                        {
                            TC_LOG_ERROR("module.playerbot.session", " About to call OnGroupJoined with group={}", (void*)group);
                            ai->OnGroupJoined(group);
                        }
                    }

                    // CRITICAL FIX: Check if bot is dead OR in ghost form at login and trigger death recovery
                    // This fixes server restart where dead bots don't resurrect
                    // BUG FIX: isDead() only checks DEAD/CORPSE states, but ghosts are ALIVE with PLAYER_FLAGS_GHOST
                    //          After server restart, dead bots load as ghosts (1 HP, Ghost aura) but m_deathState == ALIVE
                    //          Must check HasPlayerFlag(PLAYER_FLAGS_GHOST) to detect ghost state
    if (player->isDead() || player->HasPlayerFlag(PLAYER_FLAGS_GHOST))
                    {
                        TC_LOG_INFO("module.playerbot.session", " Bot {} is dead/ghost at login (isDead={}, isGhost={}, deathState={}, health={}/{}) - triggering death recovery",
                            player->GetName(),
                            player->isDead(),
                            player->HasPlayerFlag(PLAYER_FLAGS_GHOST),
                            static_cast<int>(player->getDeathState()),
                            player->GetHealth(),
                            player->GetMaxHealth());

                        if (BotAI* ai = GetAI())
                        {
                            if (ai->GetDeathRecoveryManager())
                            {
                                TC_LOG_INFO("module.playerbot.session", " Calling OnDeath to initialize death recovery for bot {}", player->GetName());
                                ai->GetDeathRecoveryManager()->OnDeath();
                            }
                            else
                            {
                                TC_LOG_ERROR("module.playerbot.session", " DeathRecoveryManager not initialized for dead bot {}", player->GetName());
                            }
                        }
                    }
                }
            }
            else
            {
                TC_LOG_ERROR("module.playerbot.session", "Failed to create BotAI for character {}", characterGuid.ToString());
            }
        }

        // BOT-SPECIFIC LOGIN SPELL CLEANUP: Clear all pending spell events to prevent m_spellModTakingSpell crash
        // Issue: LOGINEFFECT (Spell 836) and other login spells are queued in EventProcessor during bot login
        // These spells modify spell behavior (m_spellModTakingSpell) but bots don't send client ACK packets
        // When the SpellEvent destructor fires, it tries to destroy a spell that's still referenced → ASSERTION FAILURE
        // Root Cause: When KillAllEvents() destroys a delayed spell, handle_delayed() never runs to clear m_spellModTakingSpell
        // Solution: Clear m_spellModTakingSpell FIRST, then kill events to prevent Spell::~Spell assertion failure
    if (Player* player = GetPlayer())
        {
            // Core Fix Applied: SpellEvent::~SpellEvent() now automatically clears m_spellModTakingSpell (Spell.cpp:8455)
            // No longer need to manually clear - KillAllEvents() will properly clean up spell mods
            player->m_Events.KillAllEvents(false);  // false = don't force, let graceful shutdown happen
            TC_LOG_DEBUG("module.playerbot.session", " Bot {} cleared login spell events to prevent m_spellModTakingSpell crash", player->GetName());
        }

        // Mark login as complete
        _loginState.store(LoginState::LOGIN_COMPLETE);

        TC_LOG_INFO("module.playerbot.session", " ASYNC bot login successful for character {}", characterGuid.ToString());
    }
    catch (::std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot.session", "Exception in HandleBotPlayerLogin: {}", e.what());
        // CRITICAL FIX: Clean up AI BEFORE deleting player to prevent stale _bot pointer!
        // If AI was created but exception occurs later, AI holds a reference to Player.
        // We must delete AI first to prevent use-after-free crash in UpdateAI().
        //
        // P1 FIX: Use unique_ptr::reset() instead of manual delete
        _ai.reset();  // Equivalent to: if (_ai) { delete _ai; _ai = nullptr; }

        if (GetPlayer())
        {
            delete GetPlayer();
            SetPlayer(nullptr);
        }
        _loginState.store(LoginState::LOGIN_FAILED);
        m_playerLoading.Clear();
    }
    catch (...)
    {
        TC_LOG_ERROR("module.playerbot.session", "Unknown exception in HandleBotPlayerLogin");
        // CRITICAL FIX: Clean up AI BEFORE deleting player to prevent stale _bot pointer!
        //
        // P1 FIX: Use unique_ptr::reset() instead of manual delete
        _ai.reset();  // Equivalent to: if (_ai) { delete _ai; _ai = nullptr; }

        if (GetPlayer())
        {
            delete GetPlayer();
            SetPlayer(nullptr);
        }
        _loginState.store(LoginState::LOGIN_FAILED);
        m_playerLoading.Clear();
    }
}

// Process query callbacks - ensures async login callbacks are handled
// This is called by BotSessionManager::ProcessBotCallbacks()

void BotSession::HandleGroupInvitation(WorldPacket const& packet)
{
    // Validate session and player
    Player* bot = GetPlayer();
    if (!bot)
    {
        TC_LOG_DEBUG("module.playerbot.group", "HandleGroupInvitation: No player for bot session");
        return;
    }

    // Check if bot has AI
    if (!_ai)
    {
        TC_LOG_DEBUG("module.playerbot.group", "HandleGroupInvitation: No AI for bot {}", bot->GetName());
        return;
    }

    try
    {
        // Complete implementation: Parse the SMSG_PARTY_INVITE packet using TrinityCore APIs
        // This follows TrinityCore's standard packet parsing approach
        WorldPacket packetCopy = packet;

        // Parse the packet structure according to PartyPackets.cpp Write() method
        // Bit fields are read in the same order they were written
        bool canAccept = packetCopy.ReadBit();
        bool isXRealm = packetCopy.ReadBit();
        bool isXNativeRealm = packetCopy.ReadBit();
        bool shouldSquelch = packetCopy.ReadBit();
        bool allowMultipleRoles = packetCopy.ReadBit();
        bool questSessionActive = packetCopy.ReadBit();
        uint32 inviterNameSize = packetCopy.ReadBits(6);
        bool isCrossFaction = packetCopy.ReadBit();

        packetCopy.FlushBits();

        // Read VirtualRealmInfo fields manually (no operator>> available)
        // According to AuthenticationPackets.cpp operator<<:
        // - uint32 RealmAddress
        // - VirtualRealmNameInfo (2 bits + 2 strings with size bits)
        uint32 realmAddress;
        packetCopy >> realmAddress;

        // Read VirtualRealmNameInfo bit fields
        bool isLocalRealm = packetCopy.ReadBit();
        bool isInternalRealm = packetCopy.ReadBit();
        uint32 realmNameActualSize = packetCopy.ReadBits(8);
        uint32 realmNameNormalizedSize = packetCopy.ReadBits(8);
        packetCopy.FlushBits();

        ::std::string realmNameActual = ::std::string(packetCopy.ReadString(realmNameActualSize));
        ::std::string realmNameNormalized = ::std::string(packetCopy.ReadString(realmNameNormalizedSize));

        ObjectGuid inviterGUID;
        ObjectGuid inviterBNetAccountId;
        uint16 inviterCfgRealmID;
        uint8 proposedRoles;
        uint32 lfgSlotCount;
        uint32 lfgCompletedMask;

        packetCopy >> inviterGUID;
        packetCopy >> inviterBNetAccountId;
        packetCopy >> inviterCfgRealmID;
        packetCopy >> proposedRoles;
        packetCopy >> lfgSlotCount;
        packetCopy >> lfgCompletedMask;

        // Read the inviter name
        ::std::string inviterName = ::std::string(packetCopy.ReadString(inviterNameSize));

        // Read LFG slots if present
        ::std::vector<uint32> lfgSlots;
        for (uint32 i = 0; i < lfgSlotCount; ++i)
        {
            uint32 lfgSlot;
            packetCopy >> lfgSlot;
            lfgSlots.push_back(lfgSlot);
        }

        TC_LOG_INFO("module.playerbot.group", "Bot {} received group invitation from {} (GUID: {}, CanAccept: {}, Roles: {}, XRealm: {})",
            bot->GetName(), inviterName, inviterGUID.ToString(), canAccept, proposedRoles, isXRealm);

        // PHASE 2F: Hybrid validation pattern (snapshot + ObjectAccessor fallback)
        // Quick snapshot check first (fast, lock-free)
        auto inviterSnapshot = SpatialGridQueryHelpers::FindPlayerByGuid(nullptr, inviterGUID);
        if (!inviterSnapshot)
        {
            TC_LOG_DEBUG("module.playerbot.group", "HandleGroupInvitation: Inviter {} not in spatial grid (likely offline)", inviterName);
            return;
        }

        // Fallback to ObjectAccessor for full validation
        Player* inviter = ObjectAccessor::FindPlayer(inviterGUID);
        if (!inviter)
        {
            TC_LOG_ERROR("module.playerbot.group", "HandleGroupInvitation: Inviter {} (GUID: {}) not found for bot {}",
                inviterName, inviterGUID.ToString(), bot->GetName());
            return;
        }

        // Validate the invitation can be accepted
    if (!canAccept)
        {
            TC_LOG_DEBUG("module.playerbot.group", "HandleGroupInvitation: Bot {} cannot accept invitation from {} (canAccept=false)",
                bot->GetName(), inviterName);
            return;
        }

        // FIX: Use TrinityCore's standard group invite acceptance flow
        // TrinityCore's HandlePartyInviteOpcode already:
        // 1. Created/retrieved the group
        // 2. Called AddInvite(bot) which sets bot->SetGroupInvite(group)
        // 3. Sent SMSG_PARTY_INVITE to us
        // We just need to "accept" like HandlePartyInviteResponseOpcode does

        // Get the pending invite group that TrinityCore already set up
        Group* group = bot->GetGroupInvite();

        TC_LOG_INFO("module.playerbot.group", "Bot {} invitation diagnostics:", bot->GetName());
        TC_LOG_INFO("module.playerbot.group", "  - Bot current group: {}",
            bot->GetGroup() ? bot->GetGroup()->GetGUID().ToString() : "None");
        TC_LOG_INFO("module.playerbot.group", "  - Bot pending invite: {}",
            group ? group->GetGUID().ToString() : "None");
        TC_LOG_INFO("module.playerbot.group", "  - Inviter GUID: {}", inviterGUID.ToString());

        if (!group)
        {
            TC_LOG_ERROR("module.playerbot.group", "HandleGroupInvitation: Bot {} has no pending group invite! "
                "This means TrinityCore's AddInvite() was not called or was cleared.", bot->GetName());
            return;
        }

        // Safety check: bot shouldn't already be in a group
        if (bot->GetGroup())
        {
            TC_LOG_WARN("module.playerbot.group", "Bot {} is already in group {}, declining invite",
                bot->GetName(), bot->GetGroup()->GetGUID().ToString());
            group->RemoveInvite(bot);
            return;
        }

        // Follow TrinityCore's HandlePartyInviteResponseOpcode logic:
        // 1. Remove from invitee list
        group->RemoveInvite(bot);

        // 2. Sanity check - bot shouldn't be the leader
        if (group->GetLeaderGUID() == bot->GetGUID())
        {
            TC_LOG_ERROR("module.playerbot.group", "HandleGroupInvitation: Bot {} tried to accept invite to own group!",
                bot->GetName());
            return;
        }

        // 3. Check if group is full
        if (group->IsFull())
        {
            TC_LOG_WARN("module.playerbot.group", "HandleGroupInvitation: Group is full, bot {} cannot join",
                bot->GetName());
            return;
        }

        // 4. Get the leader
        Player* leader = ObjectAccessor::FindPlayer(group->GetLeaderGUID());

        // 5. If group not yet created (first member accepting), create it now
        if (!group->IsCreated())
        {
            if (!leader)
            {
                TC_LOG_ERROR("module.playerbot.group", "HandleGroupInvitation: Leader not found for uncreated group");
                group->RemoveAllInvites();
                return;
            }

            group->RemoveInvite(leader);
            group->Create(leader);
            sGroupMgr->AddGroup(group);

            TC_LOG_INFO("module.playerbot.group", "Created group {} with leader {} for bot {} to join",
                group->GetGUID().ToString(), leader->GetName(), bot->GetName());
        }

        // 6. Add bot as member (this is the actual "accept")
        if (!group->AddMember(bot))
        {
            TC_LOG_ERROR("module.playerbot.group", "HandleGroupInvitation: Failed to add bot {} to group {}",
                bot->GetName(), group->GetGUID().ToString());
            return;
        }

        // 7. Broadcast update to all group members
        group->BroadcastGroupUpdate();

        TC_LOG_INFO("module.playerbot.group", "Bot {} successfully joined group {} (leader: {})",
            bot->GetName(), group->GetGUID().ToString(),
            leader ? leader->GetName() : group->GetLeaderGUID().ToString());

        // 8. Dispatch GROUP_JOINED event for instant bot reaction
        if (_ai && _ai->GetEventDispatcher())
        {
            Events::BotEvent evt(StateMachine::EventType::GROUP_JOINED, bot->GetGUID(), group->GetLeaderGUID());
            _ai->GetEventDispatcher()->Dispatch(::std::move(evt));
            TC_LOG_DEBUG("module.playerbot.group", "GROUP_JOINED event dispatched for bot {}", bot->GetName());
        }

        // 9. Activate follow behavior through BotAI lifecycle hook
        if (_ai)
        {
            TC_LOG_INFO("module.playerbot.group", "Calling OnGroupJoined for bot {} with group {}",
                bot->GetName(), group->GetGUID().ToString());
            _ai->OnGroupJoined(group);
        }
        else
        {
            TC_LOG_ERROR("module.playerbot.group", "CRITICAL: _ai is NULL for bot {}", bot->GetName());
        }
    }
    catch (::std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot.group", "Exception handling party invitation for bot {}: {}",
            bot->GetName(), e.what());
    }
}

// ============================================================================
// THREAD-SAFE FACING SYSTEM IMPLEMENTATION
// ============================================================================

void BotSession::QueueFacingTarget(ObjectGuid targetGuid)
{
    // Thread-safe: Can be called from any thread (worker threads via ClassAI)
    std::lock_guard<std::mutex> lock(_facingMutex);
    _pendingFacingTarget = targetGuid;

    TC_LOG_TRACE("module.playerbot.facing",
                 "Queued facing target {} for bot session",
                 targetGuid.ToString());
}

bool BotSession::ProcessPendingFacing()
{
    // CRITICAL: Must only be called from main thread!
    // This method is called from BotSession::Update() which runs on main thread

    ObjectGuid targetGuid;
    {
        std::lock_guard<std::mutex> lock(_facingMutex);
        if (_pendingFacingTarget.IsEmpty())
            return false;

        targetGuid = _pendingFacingTarget;
        _pendingFacingTarget = ObjectGuid::Empty;  // Clear the pending request
    }

    // Get the player and target
    Player* bot = GetPlayer();
    if (!bot || !bot->IsInWorld())
    {
        TC_LOG_TRACE("module.playerbot.facing",
                     "ProcessPendingFacing: Bot not available or not in world");
        return false;
    }

    // Find the target unit
    Unit* target = ObjectAccessor::GetUnit(*bot, targetGuid);
    if (!target)
    {
        TC_LOG_TRACE("module.playerbot.facing",
                     "ProcessPendingFacing: Target {} not found",
                     targetGuid.ToString());
        return false;
    }

    // Execute facing on main thread - this is now safe!
    bot->SetFacingToObject(target);

    TC_LOG_TRACE("module.playerbot.facing",
                 "Bot {} now facing {}",
                 bot->GetName(), target->GetName());

    return true;
}

void BotSession::QueueStopMovement()
{
    // Thread-safe: atomic flag set
    _pendingStopMovement.store(true);

    TC_LOG_TRACE("module.playerbot.movement",
                 "Queued stop movement for bot session");
}

bool BotSession::ProcessPendingStopMovement()
{
    // CRITICAL: Must only be called from main thread!
    // This method is called from BotWorldSessionMgr::ProcessAllDeferredPackets()

    // Atomic exchange - check and clear in one operation
    if (!_pendingStopMovement.exchange(false))
        return false;  // No pending request

    // Get the player
    Player* bot = GetPlayer();
    if (!bot || !bot->IsInWorld())
    {
        TC_LOG_TRACE("module.playerbot.movement",
                     "ProcessPendingStopMovement: Bot not available or not in world");
        return false;
    }

    // Execute stop movement on main thread - this is now safe!
    bot->StopMoving();
    bot->GetMotionMaster()->Clear();

    TC_LOG_TRACE("module.playerbot.movement",
                 "Bot {} stopped movement on main thread",
                 bot->GetName());

    return true;
}

// ============================================================================
// SAFE RESURRECTION SYSTEM (SpawnCorpseBones Crash Fix)
// ============================================================================
// HandleReclaimCorpse → SpawnCorpseBones → Map::RemoveWorldObject crashes
// due to corrupted i_worldObjects tree structure (infinite loop in _Erase).
//
// FIX: Call ResurrectPlayer() directly on main thread WITHOUT SpawnCorpseBones.
// The corpse will decay naturally via TrinityCore's corpse cleanup system.
// ============================================================================

void BotSession::QueueSafeResurrection()
{
    // Thread-safe: atomic flag set
    _pendingSafeResurrection.store(true);

    TC_LOG_DEBUG("playerbot.death",
                 "Bot {} queued safe resurrection (bypasses SpawnCorpseBones crash)",
                 GetPlayerName());
}

bool BotSession::ProcessPendingSafeResurrection()
{
    // CRITICAL: Must only be called from main thread!
    // This method is called from BotWorldSessionMgr::ProcessAllDeferredPackets()

    // Atomic exchange - check and clear in one operation
    if (!_pendingSafeResurrection.exchange(false))
        return false;  // No pending request

    // Get the player
    Player* bot = GetPlayer();
    if (!bot)
    {
        TC_LOG_ERROR("playerbot.death",
                     "ProcessPendingSafeResurrection: Bot player is nullptr");
        return false;
    }

    if (!bot->IsInWorld())
    {
        TC_LOG_WARN("playerbot.death",
                    "ProcessPendingSafeResurrection: Bot {} not in world, deferring resurrection",
                    bot->GetName());
        // Re-queue for next update
        _pendingSafeResurrection.store(true);
        return false;
    }

    // Validation checks (mirror HandleReclaimCorpse checks)
    if (bot->IsAlive())
    {
        TC_LOG_INFO("playerbot.death",
                    "ProcessPendingSafeResurrection: Bot {} is already alive",
                    bot->GetName());
        return true;  // Already alive, success
    }

    if (!bot->HasPlayerFlag(PLAYER_FLAGS_GHOST))
    {
        TC_LOG_WARN("playerbot.death",
                    "ProcessPendingSafeResurrection: Bot {} has no ghost flag, cannot resurrect",
                    bot->GetName());
        return false;
    }

    // Get corpse for distance check
    Corpse* corpse = bot->GetCorpse();
    if (!corpse)
    {
        TC_LOG_WARN("playerbot.death",
                    "ProcessPendingSafeResurrection: Bot {} has no corpse, forcing resurrection anyway",
                    bot->GetName());
        // Still proceed with resurrection - bot is stuck as ghost without corpse
    }
    else
    {
        // Check distance to corpse (must be within 39 yards) - same as HandleReclaimCorpse
        if (!corpse->IsWithinDistInMap(bot, CORPSE_RECLAIM_RADIUS, true))
        {
            TC_LOG_WARN("playerbot.death",
                        "ProcessPendingSafeResurrection: Bot {} too far from corpse ({:.1f}y)",
                        bot->GetName(), bot->GetDistance2d(corpse));
            return false;
        }
    }

    // ========================================================================
    // SAFE RESURRECTION - NO SpawnCorpseBones!
    // ========================================================================
    // HandleReclaimCorpse calls:
    //   1. ResurrectPlayer(healthPct)  <- SAFE
    //   2. SpawnCorpseBones()          <- CRASHES (infinite loop in Map::RemoveWorldObject)
    //
    // We ONLY call ResurrectPlayer. The corpse will:
    //   - Decay naturally via TrinityCore's corpse cleanup timer
    //   - Or be cleaned up when the bot dies again
    // ========================================================================

    TC_LOG_INFO("playerbot.death",
                "ProcessPendingSafeResurrection: Bot {} - SAFE resurrection starting (NO SpawnCorpseBones)",
                bot->GetName());

    // Resurrect with 50% health (same as normal corpse reclaim)
    // In battlegrounds, use 100% health
    float healthPct = bot->InBattleground() ? 1.0f : 0.5f;
    bot->ResurrectPlayer(healthPct);

    TC_LOG_INFO("playerbot.death",
                "ProcessPendingSafeResurrection: Bot {} - ResurrectPlayer() complete! IsAlive={} (corpse left to decay)",
                bot->GetName(), bot->IsAlive());

    // NOTE: We intentionally DO NOT call SpawnCorpseBones() here!
    // The corpse will be cleaned up by TrinityCore's natural corpse decay system.
    // This avoids the Map::RemoveWorldObject crash in the corrupted i_worldObjects tree.

    return true;
}

// ============================================================================
// THREAD-SAFE LOOT QUEUE
// ============================================================================
// FIX: SendLoot() causes ACCESS_VIOLATION when called from worker threads
// because it modifies _updateObjects which must only be touched on main thread.
//
// Solution: Queue loot targets from worker threads, process on main thread.
// ============================================================================

void BotSession::QueueLootTarget(ObjectGuid creatureGuid)
{
    std::lock_guard<std::mutex> lock(_pendingLootMutex);
    _pendingLootTargets.push_back(creatureGuid);

    TC_LOG_DEBUG("module.playerbot.loot",
                 "Bot {} queued loot target {} for main thread processing",
                 GetPlayerName(), creatureGuid.ToString());
}

bool BotSession::HasPendingLoot() const
{
    std::lock_guard<std::mutex> lock(_pendingLootMutex);
    return !_pendingLootTargets.empty();
}

bool BotSession::ProcessPendingLoot()
{
    // CRITICAL: Must only be called from main thread!

    // Get pending targets atomically
    std::vector<ObjectGuid> targets;
    {
        std::lock_guard<std::mutex> lock(_pendingLootMutex);
        if (_pendingLootTargets.empty())
            return false;
        targets = std::move(_pendingLootTargets);
        _pendingLootTargets.clear();
    }

    Player* bot = GetPlayer();
    if (!bot || !bot->IsInWorld())
    {
        TC_LOG_DEBUG("module.playerbot.strategy", "ProcessPendingLoot: Bot not in world, skipping {} targets",
                     targets.size());
        return false;
    }

    TC_LOG_DEBUG("module.playerbot.strategy", "ProcessPendingLoot: Bot {} processing {} loot targets",
                 bot->GetName(), targets.size());

    bool lootedAny = false;

    for (ObjectGuid const& targetGuid : targets)
    {
        if (!targetGuid.IsCreature())
            continue;

        Creature* creature = bot->GetMap()->GetCreature(targetGuid);
        if (!creature || !creature->isDead())
        {
            TC_LOG_DEBUG("module.playerbot.strategy",
                         "ProcessPendingLoot: Bot {} - creature {} not found or not dead",
                         bot->GetName(), targetGuid.ToString());
            continue;
        }

        // Check distance
        if (!creature->IsWithinDistInMap(bot, INTERACTION_DISTANCE))
        {
            TC_LOG_DEBUG("module.playerbot.strategy",
                         "ProcessPendingLoot: Bot {} too far from corpse {} ({:.1f}y)",
                         bot->GetName(), targetGuid.ToString(), bot->GetDistance(creature));
            continue;
        }

        // Check if creature has loot - use GetLootForPlayer which checks personal loot too
        Loot* loot = creature->GetLootForPlayer(bot);

        // Debug: Log loot state in detail
        TC_LOG_DEBUG("module.playerbot.strategy",
                     "ProcessPendingLoot: Bot {} - corpse {} loot state: loot={}, isLooted={}, items={}, gold={}",
                     bot->GetName(), targetGuid.ToString(),
                     loot ? "valid" : "NULL",
                     loot ? (loot->isLooted() ? "YES" : "NO") : "N/A",
                     loot ? loot->items.size() : 0,
                     loot ? loot->gold : 0);

        if (!loot || (loot->isLooted() && loot->items.empty() && loot->gold == 0))
        {
            TC_LOG_DEBUG("module.playerbot.strategy",
                         "ProcessPendingLoot: Bot {} - corpse {} (entry {}) has no loot (loot={}, m_loot={}, personalLoot={})",
                         bot->GetName(), targetGuid.ToString(), creature->GetEntry(),
                         loot ? "valid" : "NULL",
                         creature->m_loot ? "exists" : "NULL",
                         creature->m_personalLoot.count(bot->GetGUID()) ? "exists" : "NONE");
            continue;
        }

        // CRITICAL FIX: Actually loot items instead of just sending loot window
        // SendLoot() only opens the loot UI for clients - bots need to StoreLootItem() directly
        ObjectGuid lootOwnerGuid = loot->GetOwnerGUID();
        uint32 itemsLooted = 0;

        // Check if bot is in a group with special loot rules
        Group* group = bot->GetGroup();
        bool useGroupLoot = group && group->GetLootMethod() != FREE_FOR_ALL;

        if (useGroupLoot)
        {
            // For group loot, use SendLoot to trigger proper roll mechanics
            // The group loot system will handle Need/Greed/Master Looter rules
            bot->SendLoot(*loot, false);
            TC_LOG_DEBUG("module.playerbot.strategy",
                         "ProcessPendingLoot: Bot {} in group with loot rules, using SendLoot for corpse {}",
                         bot->GetName(), targetGuid.ToString());
            lootedAny = true;
            continue;
        }

        // Solo bot or FreeForAll - directly store items
        // Iterate through all loot items and store them
        for (uint8 lootSlot = 0; lootSlot < loot->items.size(); ++lootSlot)
        {
            LootItem* item = loot->LootItemInSlot(lootSlot, bot);
            if (!item || item->is_looted)
                continue;

            // Skip blocked items (pending roll) - shouldn't happen for solo but safety check
            if (item->is_blocked)
            {
                TC_LOG_DEBUG("module.playerbot.strategy",
                             "ProcessPendingLoot: Bot {} skipping blocked item {} (pending roll)",
                             bot->GetName(), item->itemid);
                continue;
            }

            // Store the item in bot's inventory
            bot->StoreLootItem(lootOwnerGuid, lootSlot, loot);
            itemsLooted++;

            TC_LOG_INFO("module.playerbot.strategy",
                         "ProcessPendingLoot: Bot {} looted item {} (entry {}) from corpse {}",
                         bot->GetName(), item->itemid, creature->GetEntry(), targetGuid.ToString());
        }

        // Also loot gold if any
        if (loot->gold > 0)
        {
            bot->ModifyMoney(loot->gold);
            loot->gold = 0;
            TC_LOG_DEBUG("module.playerbot.strategy",
                         "ProcessPendingLoot: Bot {} looted gold from corpse {}",
                         bot->GetName(), targetGuid.ToString());
        }

        if (itemsLooted > 0)
            lootedAny = true;

        TC_LOG_INFO("module.playerbot.strategy",
                     "ProcessPendingLoot: Bot {} LOOTED {} items from corpse {} (entry {})",
                     bot->GetName(), itemsLooted, targetGuid.ToString(), creature->GetEntry());
    }

    return lootedAny;
}

// ============================================================================
// THREAD-SAFE OBJECT USE QUEUE (GameObject::Use Crash Fix)
// ============================================================================
// GameObject::Use() causes ACCESS_VIOLATION when called from worker threads
// because it modifies game object state and triggers Map updates.
//
// Solution: Queue object use from worker threads, process on main thread.
// ============================================================================

void BotSession::QueueObjectUse(ObjectGuid objectGuid)
{
    std::lock_guard<std::mutex> lock(_pendingObjectUseMutex);
    _pendingObjectUseTargets.push_back(objectGuid);

    TC_LOG_DEBUG("module.playerbot.loot",
                 "Bot {} queued object {} for Use() on main thread",
                 GetPlayerName(), objectGuid.ToString());
}

bool BotSession::HasPendingObjectUse() const
{
    std::lock_guard<std::mutex> lock(_pendingObjectUseMutex);
    return !_pendingObjectUseTargets.empty();
}

bool BotSession::ProcessPendingObjectUse()
{
    // CRITICAL: Must only be called from main thread!

    // Get pending targets atomically
    std::vector<ObjectGuid> targets;
    {
        std::lock_guard<std::mutex> lock(_pendingObjectUseMutex);
        if (_pendingObjectUseTargets.empty())
            return false;
        targets = std::move(_pendingObjectUseTargets);
        _pendingObjectUseTargets.clear();
    }

    Player* bot = GetPlayer();
    if (!bot || !bot->IsInWorld())
    {
        TC_LOG_DEBUG("module.playerbot.strategy", "ProcessPendingObjectUse: Bot not in world, skipping {} targets",
                     targets.size());
        return false;
    }

    TC_LOG_DEBUG("module.playerbot.strategy", "ProcessPendingObjectUse: Bot {} processing {} object use requests",
                 bot->GetName(), targets.size());

    bool usedAny = false;

    for (ObjectGuid const& objectGuid : targets)
    {
        if (!objectGuid.IsGameObject())
            continue;

        GameObject* object = bot->GetMap()->GetGameObject(objectGuid);
        if (!object)
        {
            TC_LOG_DEBUG("module.playerbot.strategy",
                         "ProcessPendingObjectUse: Bot {} - object {} not found",
                         bot->GetName(), objectGuid.ToString());
            continue;
        }

        // Check distance
        if (!object->IsWithinDistInMap(bot, INTERACTION_DISTANCE))
        {
            TC_LOG_DEBUG("module.playerbot.strategy",
                         "ProcessPendingObjectUse: Bot {} too far from object {} ({:.1f}y)",
                         bot->GetName(), objectGuid.ToString(), bot->GetDistance(object));
            continue;
        }

        // SAFE: Now on main thread, can call Use()
        object->Use(bot);
        usedAny = true;

        TC_LOG_INFO("module.playerbot.strategy",
                     "ProcessPendingObjectUse: Bot {} USED object {} (entry {})",
                     bot->GetName(), objectGuid.ToString(), object->GetEntry());
    }

    return usedAny;
}

// ============================================================================
// THREAD-SAFE LFG PROPOSAL AUTO-ACCEPT (Re-entrant Crash Fix)
// ============================================================================

void BotSession::QueueLfgProposalAccept(uint32 proposalId)
{
    std::lock_guard<std::mutex> lock(_pendingLfgAcceptsMutex);
    _pendingLfgProposalAccepts.push_back(proposalId);

    Player* bot = GetPlayer();
    TC_LOG_DEBUG("module.playerbot.lfg",
                 "QueueLfgProposalAccept: Bot {} queued proposal {} for main thread accept",
                 bot ? bot->GetName() : "unknown", proposalId);
}

bool BotSession::HasPendingLfgProposalAccepts() const
{
    std::lock_guard<std::mutex> lock(_pendingLfgAcceptsMutex);
    return !_pendingLfgProposalAccepts.empty();
}

bool BotSession::ProcessPendingLfgProposalAccepts()
{
    // Move pending accepts to local vector to minimize lock time
    std::vector<uint32> proposalsToAccept;
    {
        std::lock_guard<std::mutex> lock(_pendingLfgAcceptsMutex);
        proposalsToAccept = std::move(_pendingLfgProposalAccepts);
        _pendingLfgProposalAccepts.clear();
    }

    if (proposalsToAccept.empty())
        return false;

    Player* bot = GetPlayer();
    if (!bot)
    {
        TC_LOG_ERROR("module.playerbot.lfg",
                     "ProcessPendingLfgProposalAccepts: Bot player is nullptr, re-queuing {} proposals",
                     proposalsToAccept.size());
        // Re-queue proposals for next attempt
        {
            std::lock_guard<std::mutex> lock(_pendingLfgAcceptsMutex);
            for (uint32 proposalId : proposalsToAccept)
                _pendingLfgProposalAccepts.push_back(proposalId);
        }
        return false;
    }

    if (!bot->IsInWorld())
    {
        TC_LOG_DEBUG("module.playerbot.lfg",
                    "ProcessPendingLfgProposalAccepts: Bot {} not in world, re-queuing {} proposals",
                    bot->GetName(), proposalsToAccept.size());
        // Re-queue proposals for next attempt instead of discarding
        {
            std::lock_guard<std::mutex> lock(_pendingLfgAcceptsMutex);
            for (uint32 proposalId : proposalsToAccept)
                _pendingLfgProposalAccepts.push_back(proposalId);
        }
        return false;
    }

    TC_LOG_DEBUG("module.playerbot.lfg",
                 "ProcessPendingLfgProposalAccepts: Bot {} processing {} proposals on main thread",
                 bot->GetName(), proposalsToAccept.size());

    bool acceptedAny = false;
    for (uint32 proposalId : proposalsToAccept)
    {
        // Pre-validation: Check if proposal still exists and bot is in it
        // UpdateProposal() silently returns if these conditions aren't met
        lfg::LfgState currentState = sLFGMgr->GetState(bot->GetGUID());

        TC_LOG_INFO("module.playerbot.lfg",
                    "ProcessPendingLfgProposalAccepts: Bot {} ({}) accepting proposal {} - LFG state: {} - IsInWorld: {} - MapId: {}",
                    bot->GetName(), bot->GetGUID().ToString(), proposalId, static_cast<uint32>(currentState),
                    bot->IsInWorld(), bot->GetMapId());

        // SAFE: Now on main thread, outside of LFGMgr iteration
        sLFGMgr->UpdateProposal(proposalId, bot->GetGUID(), true);

        // Post-validation: Check state after accept
        lfg::LfgState newState = sLFGMgr->GetState(bot->GetGUID());

        // ENHANCED DIAGNOSTICS: Check if teleport was initiated
        bool isBeingTeleportedFar = bot->IsBeingTeleportedFar();
        bool isBeingTeleportedNear = bot->IsBeingTeleportedNear();
        Group* group = bot->GetGroup();
        bool hasGroup = group != nullptr;
        bool isLfgGroup = hasGroup && group->isLFGGroup();

        TC_LOG_INFO("module.playerbot.lfg",
                    "ProcessPendingLfgProposalAccepts: Bot {} post-accept - state: {} -> {} - IsInWorld: {} - TeleportFar: {} - TeleportNear: {} - HasGroup: {} - IsLfgGroup: {}",
                    bot->GetName(), static_cast<uint32>(currentState), static_cast<uint32>(newState),
                    bot->IsInWorld(), isBeingTeleportedFar, isBeingTeleportedNear, hasGroup, isLfgGroup);

        // If state changed to DUNGEON, teleport should have been triggered
        if (newState == lfg::LFG_STATE_DUNGEON && !isBeingTeleportedFar && !isBeingTeleportedNear)
        {
            TC_LOG_WARN("module.playerbot.lfg",
                        "⚠️ ProcessPendingLfgProposalAccepts: Bot {} state is DUNGEON but NOT teleporting! "
                        "This indicates TeleportPlayer() was not called or failed. Checking ObjectAccessor...",
                        bot->GetName());

            // Diagnostic: Check if bot is findable by ObjectAccessor
            Player* foundByFind = ObjectAccessor::FindPlayer(bot->GetGUID());
            Player* foundByConnected = ObjectAccessor::FindConnectedPlayer(bot->GetGUID());
            TC_LOG_WARN("module.playerbot.lfg",
                        "⚠️ ObjectAccessor diagnostic: FindPlayer={}, FindConnectedPlayer={}",
                        foundByFind != nullptr, foundByConnected != nullptr);

            // WORKAROUND: If bot wasn't teleported by LFGMgr (possibly due to ObjectAccessor::FindPlayer
            // failing because bot's IsInWorld() was false during iteration), manually trigger teleport
            if (bot->IsInWorld() && isLfgGroup)
            {
                // COMPREHENSIVE DIAGNOSTICS: Check all LFGMgr::TeleportPlayer conditions
                uint32 dungeonId = sLFGMgr->GetDungeon(group->GetGUID());
                uint32 dungeonMapId = sLFGMgr->GetDungeonMapId(group->GetGUID());

                TC_LOG_INFO("module.playerbot.lfg",
                    "🔧 ProcessPendingLfgProposalAccepts: TELEPORT PRE-CHECK for bot {}:\n"
                    "   - GroupGUID: {}\n"
                    "   - isLFGGroup: {}\n"
                    "   - DungeonId: {}\n"
                    "   - DungeonMapId: {}\n"
                    "   - Bot conditions: IsAlive={}, IsFalling={}, IsJumping={}, InVehicle={}, IsCharmed={}\n"
                    "   - Bot MapId: {}, Session: {}",
                    bot->GetName(),
                    group->GetGUID().ToString(),
                    group->isLFGGroup(),
                    dungeonId,
                    dungeonMapId,
                    bot->IsAlive(),
                    bot->IsFalling(),
                    bot->HasUnitState(UNIT_STATE_JUMPING),
                    bot->GetVehicle() != nullptr,
                    !bot->GetCharmedGUID().IsEmpty(),
                    bot->GetMapId(),
                    bot->GetSession() ? "VALID" : "NULL");

                if (dungeonId == 0)
                {
                    TC_LOG_ERROR("module.playerbot.lfg",
                        "❌ TELEPORT WILL FAIL: DungeonId is 0. LFGMgr::TeleportPlayer requires valid dungeon.");
                }

                TC_LOG_INFO("module.playerbot.lfg",
                            "🔧 ProcessPendingLfgProposalAccepts: Manually triggering TeleportPlayer for bot {}",
                            bot->GetName());
                sLFGMgr->TeleportPlayer(bot, false);

                // Check if teleport was initiated after manual trigger
                if (bot->IsBeingTeleportedFar() || bot->IsBeingTeleportedNear())
                {
                    TC_LOG_INFO("module.playerbot.lfg",
                                "✅ ProcessPendingLfgProposalAccepts: Manual teleport succeeded for bot {} - TeleportFar: {} TeleportNear: {}",
                                bot->GetName(), bot->IsBeingTeleportedFar(), bot->IsBeingTeleportedNear());
                }
                else
                {
                    TC_LOG_ERROR("module.playerbot.lfg",
                                "❌ ProcessPendingLfgProposalAccepts: Manual TeleportPlayer FAILED for bot {} - TeleportPlayer returned but teleport state not set",
                                bot->GetName());

                    // FALLBACK: Query database for dungeon entrance and do direct teleport
                    if (dungeonId != 0)
                    {
                        TC_LOG_INFO("module.playerbot.lfg",
                            "🔧 Attempting FALLBACK: Query lfg_dungeon_template for dungeon {} entrance",
                            dungeonId);

                        QueryResult result = WorldDatabase.PQuery(
                            "SELECT position_x, position_y, position_z, orientation FROM lfg_dungeon_template WHERE dungeonId = {}",
                            dungeonId);

                        if (result)
                        {
                            Field* fields = result->Fetch();
                            float x = fields[0].GetFloat();
                            float y = fields[1].GetFloat();
                            float z = fields[2].GetFloat();
                            float o = fields[3].GetFloat();

                            TC_LOG_INFO("module.playerbot.lfg",
                                "🔧 Attempting FALLBACK: Direct TeleportTo for bot {} to Map {} ({:.1f}, {:.1f}, {:.1f})",
                                bot->GetName(), dungeonMapId, x, y, z);

                            // Store entry point before teleport
                            if (!bot->GetMap()->IsDungeon())
                                bot->SetBattlegroundEntryPoint();

                            bool teleportResult = bot->TeleportTo(
                                { .Location = WorldLocation(dungeonMapId, x, y, z, o),
                                  .LfgDungeonsId = dungeonId });

                            if (teleportResult)
                            {
                                TC_LOG_INFO("module.playerbot.lfg",
                                    "✅ FALLBACK TeleportTo succeeded for bot {}",
                                    bot->GetName());
                            }
                            else
                            {
                                TC_LOG_ERROR("module.playerbot.lfg",
                                    "❌ FALLBACK TeleportTo ALSO FAILED for bot {}",
                                    bot->GetName());
                            }
                        }
                        else
                        {
                            TC_LOG_ERROR("module.playerbot.lfg",
                                "❌ FALLBACK: No entrance data found for dungeon {} in lfg_dungeon_template",
                                dungeonId);
                        }
                    }
                }
            }
        }

        acceptedAny = true;
    }

    return acceptedAny;
}

// ============================================================================
// INSTANCE BOT LIFECYCLE MANAGEMENT
// ============================================================================
// Instance bots (JIT/Warm Pool) auto-logout after 60 seconds of being idle
// (not in queue, not in group, not in instance).
// ============================================================================

void BotSession::SetInstanceBot(bool isInstanceBot)
{
    _isInstanceBot.store(isInstanceBot);

    if (isInstanceBot)
    {
        // Reset idle and queue state when marked as instance bot
        _idleAccumulatorMs.store(0);
        _queueAccumulatorMs.store(0);
        _wasActiveLastCheck.store(true);
        _hasEnteredInstance.store(false);
        _instanceBotStartTime.store(GameTime::GetGameTimeMS());

        Player* player = GetPlayer();
        TC_LOG_INFO("module.playerbot.instance",
            "Bot {} marked as INSTANCE BOT - idle timeout: {}s, queue timeout: {}s",
            player ? player->GetName() : "unknown",
            GetInstanceBotIdleTimeout() / 1000,
            GetInstanceBotQueueTimeout() / 1000);
    }
}

bool BotSession::IsInActiveInstanceState() const
{
    Player* player = GetPlayer();
    if (!player)
        return false;

    // Check 1: Is bot in a group?
    if (player->GetGroup())
    {
        return true;
    }

    // Check 2: Is bot inside an instance (dungeon/raid/BG/arena)?
    Map* map = player->GetMap();
    if (map && (map->IsDungeon() || map->IsRaid() || map->IsBattleground() || map->IsBattleArena()))
    {
        return true;
    }

    // Check 3: Is bot in LFG queue?
    // LFGMgr tracks queued players
    lfg::LfgState lfgState = sLFGMgr->GetState(player->GetGUID());
    if (lfgState != lfg::LFG_STATE_NONE && lfgState != lfg::LFG_STATE_RAIDBROWSER)
    {
        // States that indicate active queue:
        // LFG_STATE_QUEUED, LFG_STATE_PROPOSAL, LFG_STATE_DUNGEON, LFG_STATE_BOOT
        return true;
    }

    // Check 4: Is bot in BG queue?
    // Iterate through all possible BG queue types to check if player is queued
    for (uint8 i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
    {
        BattlegroundQueueTypeId bgQueueTypeId = player->GetBattlegroundQueueTypeId(i);
        if (bgQueueTypeId != BATTLEGROUND_QUEUE_NONE)
        {
            // Player has an active BG queue slot
            return true;
        }
    }

    // Check 5: Is bot in Arena queue? (handled by BG queue check above for rated)

    // Not in any active state
    return false;
}

bool BotSession::UpdateIdleStateAndCheckLogout(uint32 diff)
{
    // Only applies to instance bots
    if (!_isInstanceBot.load())
        return false;

    Player* player = GetPlayer();
    if (!player)
        return false;

    // ========================================================================
    // CHECK 1: Has bot entered actual instanced content?
    // ========================================================================
    // If bot is inside an instance (dungeon/raid/BG/arena), mark it as having
    // "entered" - this means it's actively participating, not just waiting.
    Map* map = player->GetMap();
    bool inInstancedContent = map && (map->IsDungeon() || map->IsRaid() ||
                                       map->IsBattleground() || map->IsBattleArena());

    if (inInstancedContent)
    {
        // Bot is actually in instanced content - mark it and reset queue timer
        if (!_hasEnteredInstance.exchange(true))
        {
            TC_LOG_INFO("module.playerbot.instance",
                "Bot {} ENTERED instanced content - queue timeout disabled",
                player->GetName());
        }
        _queueAccumulatorMs.store(0);
        _idleAccumulatorMs.store(0);
        _wasActiveLastCheck.store(true);
        return false;
    }

    // ========================================================================
    // CHECK 2: Is bot in an "active" state (queue/group)?
    // ========================================================================
    bool isActive = IsInActiveInstanceState();

    if (isActive)
    {
        // Bot is in queue or group but NOT in actual instanced content
        // Reset idle timer but accumulate queue time (if never entered instance)
        if (_idleAccumulatorMs.load() > 0)
        {
            TC_LOG_DEBUG("module.playerbot.instance",
                "Bot {} became ACTIVE (in queue/group) - resetting idle timer",
                player->GetName());
        }
        _idleAccumulatorMs.store(0);
        _wasActiveLastCheck.store(true);

        // If bot has never entered instance, accumulate queue time
        if (!_hasEnteredInstance.load())
        {
            uint32 currentQueue = _queueAccumulatorMs.load();
            uint32 newQueue = currentQueue + diff;
            _queueAccumulatorMs.store(newQueue);

            // Check if queue timeout exceeded
            if (newQueue >= GetInstanceBotQueueTimeout())
            {
                TC_LOG_INFO("module.playerbot.instance",
                    "⏰ Bot {} QUEUE TIMEOUT ({} seconds in queue without content starting) - "
                    "scheduling logout to prevent accumulation",
                    player->GetName(), GetInstanceBotQueueTimeout() / 1000);
                return true;
            }

            // Log progress every 60 seconds
            uint32 oldMinutes = currentQueue / 60000;
            uint32 newMinutes = newQueue / 60000;
            if (newMinutes > oldMinutes)
            {
                TC_LOG_DEBUG("module.playerbot.instance",
                    "Bot {} waiting in queue for {}m / {}m",
                    player->GetName(), newQueue / 60000, GetInstanceBotQueueTimeout() / 60000);
            }
        }

        return false;
    }

    // ========================================================================
    // CHECK 3: Bot is idle (not in queue/group/instance)
    // ========================================================================
    bool wasActive = _wasActiveLastCheck.exchange(false);
    if (wasActive)
    {
        // Just became idle - log the transition
        TC_LOG_INFO("module.playerbot.instance",
            "Bot {} became IDLE (not in queue/group/instance) - starting {}s logout timer",
            player->GetName(), GetInstanceBotIdleTimeout() / 1000);
    }

    uint32 currentIdle = _idleAccumulatorMs.load();
    uint32 newIdle = currentIdle + diff;
    _idleAccumulatorMs.store(newIdle);

    // Check if idle timeout exceeded
    if (newIdle >= GetInstanceBotIdleTimeout())
    {
        TC_LOG_INFO("module.playerbot.instance",
            "⏰ Bot {} IDLE TIMEOUT ({} seconds) - scheduling logout to reduce server load",
            player->GetName(), GetInstanceBotIdleTimeout() / 1000);
        return true;
    }

    // Log progress every 15 seconds
    uint32 oldSeconds = currentIdle / 15000;
    uint32 newSeconds = newIdle / 15000;
    if (newSeconds > oldSeconds)
    {
        TC_LOG_DEBUG("module.playerbot.instance",
            "Bot {} idle for {}s / {}s",
            player->GetName(), newIdle / 1000, GetInstanceBotIdleTimeout() / 1000);
    }

    return false;
}

uint32 BotSession::GetIdleDurationMs() const
{
    if (!_isInstanceBot.load())
        return 0;

    return _idleAccumulatorMs.load();
}

uint32 BotSession::GetInstanceBotIdleTimeout()
{
    return sPlayerbotConfig->GetUInt("Playerbot.Session.IdleTimeout", 60000);
}

uint32 BotSession::GetInstanceBotQueueTimeout()
{
    return sPlayerbotConfig->GetUInt("Playerbot.Session.QueueTimeout", 300000);
}

} // namespace Playerbot