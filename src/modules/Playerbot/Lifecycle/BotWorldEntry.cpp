/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#include "BotWorldEntry.h"
#include "BotSession.h"
#include "BotLoginQueryHolder.h"
#include "QueryHolder.h"
#include "Player.h"
#include "World.h"
#include "WorldSession.h"
#include "Map.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "CharacterCache.h"
#include "CharacterDatabase.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "GameTime.h"
#include "AI/BotAI.h"
#include "AI/BotAIFactory.h"
#include "SpellMgr.h"
#include "Chat/Chat.h"
#include "Group.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "SocialMgr.h"
#include "CharacterPackets.h"
#include "MiscPackets.h"
#include "DB2Stores.h"
#include "Equipment/BotGearFactory.h"
#include "Config/PlayerbotConfig.h"
#include "PhasingHandler.h"
#include <thread>
#include <fstream>

// Platform-specific includes for memory tracking
#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
#elif defined(__linux__)
    #include <unistd.h>
    #include <fstream>
#elif defined(__APPLE__)
    #include <mach/mach.h>
    #include <mach/task.h>
#endif

namespace Playerbot {

BotWorldEntry::BotWorldEntry(::std::shared_ptr<BotSession> session, ObjectGuid characterGuid)
    : _session(session)
    , _characterGuid(characterGuid)
    , _player(nullptr)
    , _state(BotWorldEntryState::NONE)
    , _processing(false)
    , _retryCount(0)
{
    _metrics.startTime = ::std::chrono::steady_clock::now();

    // ========================================================================
    // HIGH PRIORITY TODO FIXED: Implement memory tracking
    // ========================================================================
    // Platform-specific memory tracking implementation
    _metrics.memoryBeforeEntry = GetCurrentMemoryUsage();
}

// Helper function to get current process memory usage (in bytes)
size_t BotWorldEntry::GetCurrentMemoryUsage() const
{
#ifdef _WIN32
    // Windows: Use GetProcessMemoryInfo
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
    {
        return static_cast<size_t>(pmc.WorkingSetSize); // Physical memory usage
    }
    return 0;
#elif defined(__linux__)
    // Linux: Read /proc/self/statm
    // Fields: size resident shared text lib data dt
    // We want resident set size (RSS) in bytes
    ::std::ifstream statm("/proc/self/statm");
    if (statm.is_open())
    {
        unsigned long size = 0;
        unsigned long resident = 0;
        statm >> size >> resident;
        statm.close();

        // Convert pages to bytes (typically 4096 bytes per page)
        long pageSize = sysconf(_SC_PAGESIZE);
        if (pageSize > 0)
            return static_cast<size_t>(resident * pageSize);
    }
    return 0;
#elif defined(__APPLE__)
    // macOS: Use task_info
    struct task_basic_info t_info;
    mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;

    if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&t_info, &t_info_count) == KERN_SUCCESS)
    {
        return static_cast<size_t>(t_info.resident_size);
    }
    return 0;
#else
    // Unsupported platform
    return 0;
#endif
}

BotWorldEntry::~BotWorldEntry()
{
    if (_state != BotWorldEntryState::FULLY_ACTIVE && _state != BotWorldEntryState::CLEANUP)
    {
        Cleanup();
    }
}

bool BotWorldEntry::BeginWorldEntry(EntryCallback callback)
{
    if (_processing.exchange(true))
    {
        TC_LOG_ERROR("module.playerbot.worldentry", "Bot {} world entry already in progress",
                     _characterGuid.ToString());
        return false;
    }

    {
        ::std::lock_guard lock(_callbackMutex);
        _callback = callback;
    }

    TC_LOG_INFO("module.playerbot.worldentry", "Beginning world entry for bot {}",
                _characterGuid.ToString());

    // Start with loading character data
    if (!TransitionToState(BotWorldEntryState::CHARACTER_LOADED))
    {
        _processing = false;
        return false;
    }

    // Begin the entry process
    return LoadCharacterData();
}

bool BotWorldEntry::ProcessWorldEntry(uint32 diff)
{
    if (!_processing.load())
        return false;

    // Check for phase timeout
    auto now = ::std::chrono::steady_clock::now();
    if (now - _phaseStartTime > PHASE_TIMEOUT)
    {
        HandleWorldEntryFailure("Phase timeout exceeded");
        return false;
    }

    bool result = false;

    switch (_state.load())
    {
        case BotWorldEntryState::CHARACTER_LOADED:
            result = CreatePlayerObject();
            break;

        case BotWorldEntryState::PLAYER_CREATED:
            result = LoadTargetMap();
            break;

        case BotWorldEntryState::MAP_LOADED:
            result = AddPlayerToWorld();
            break;

        case BotWorldEntryState::IN_WORLD:
            result = InitializeAI();
            break;

        case BotWorldEntryState::AI_ACTIVE:
            result = FinalizeBotActivation();
            break;

        case BotWorldEntryState::FULLY_ACTIVE:
        {
            // Calculate final metrics
            _metrics.endTime = ::std::chrono::steady_clock::now();
            _metrics.totalTime = static_cast<uint32>(
                ::std::chrono::duration_cast<::std::chrono::microseconds>(
                    _metrics.endTime - _metrics.startTime).count());

            TC_LOG_INFO("module.playerbot.worldentry",
                       "Bot {} world entry completed in {} ms",
                       _characterGuid.ToString(),
                       _metrics.totalTime / 1000);

            // Invoke callback if set
            {
                ::std::lock_guard lock(_callbackMutex);
                if (_callback)
                {
                    _callback(true, _metrics);
                    _callback = nullptr;
                }
            }

            _processing = false;
            return false; // Entry complete
        }

        case BotWorldEntryState::FAILED:
        case BotWorldEntryState::CLEANUP:
            _processing = false;
            return false;

        default:
            TC_LOG_ERROR("module.playerbot.worldentry",
                        "Bot {} in unexpected state: {}",
                        _characterGuid.ToString(),
                        static_cast<uint32>(_state.load()));
            HandleWorldEntryFailure("Unexpected state");
            return false;
    }

    if (!result)
    {
        if (++_retryCount >= MAX_RETRY_COUNT)
        {
            HandleWorldEntryFailure("Maximum retry count exceeded");
            return false;
        }

        TC_LOG_WARN("module.playerbot.worldentry",
                   "Bot {} world entry phase failed, retrying ({}/{})",
                   _characterGuid.ToString(),
                   _retryCount,
                   MAX_RETRY_COUNT);
    }
    else
    {
        _retryCount = 0; // Reset retry count on success
    }

    return true;
}

bool BotWorldEntry::EnterWorldSync(uint32 timeoutMs)
{
    if (!BeginWorldEntry())
        return false;

    auto startTime = ::std::chrono::steady_clock::now();
    auto timeout = ::std::chrono::milliseconds(timeoutMs);

    while (_processing.load())
    {
        if (::std::chrono::steady_clock::now() - startTime > timeout)
        {
            TC_LOG_ERROR("module.playerbot.worldentry",
                        "Bot {} world entry sync timeout after {} ms",
                        _characterGuid.ToString(),
                        timeoutMs);
            HandleWorldEntryFailure("Synchronous entry timeout");
            return false;
        }

        // Process one step
        ProcessWorldEntry(100);

        // Small sleep to prevent CPU spinning
        ::std::this_thread::sleep_for(::std::chrono::milliseconds(10));
    }

    return _state == BotWorldEntryState::FULLY_ACTIVE;
}

bool BotWorldEntry::IsProcessing() const
{
    BotWorldEntryState state = _state.load();
    return state != BotWorldEntryState::NONE &&
           state != BotWorldEntryState::FULLY_ACTIVE &&
           state != BotWorldEntryState::FAILED &&
           state != BotWorldEntryState::CLEANUP;
}

uint32 BotWorldEntry::GetElapsedTime() const
{
    auto now = ::std::chrono::steady_clock::now();
    return static_cast<uint32>(
        ::std::chrono::duration_cast<::std::chrono::milliseconds>(
            now - _metrics.startTime).count());
}

void BotWorldEntry::SetError(::std::string const& error)
{
    _metrics.lastError = error;
    _metrics.failedState = _state.load();
}

bool BotWorldEntry::TransitionToState(BotWorldEntryState newState)
{
    ::std::lock_guard lock(_stateMutex);

    BotWorldEntryState oldState = _state.exchange(newState);
    _phaseStartTime = ::std::chrono::steady_clock::now();

    RecordStateTransition(oldState, newState);

    TC_LOG_DEBUG("module.playerbot.worldentry",
                "Bot {} transitioned from state {} to state {}",
                _characterGuid.ToString(),
                static_cast<uint32>(oldState),
                static_cast<uint32>(newState));

    return true;
}

void BotWorldEntry::RecordStateTransition(BotWorldEntryState oldState, BotWorldEntryState newState)
{
    auto now = ::std::chrono::steady_clock::now();
    uint32 phaseDuration = static_cast<uint32>(
        ::std::chrono::duration_cast<::std::chrono::microseconds>(
            now - _phaseStartTime).count());

    switch (oldState)
    {
        case BotWorldEntryState::CHARACTER_LOADED:
            _metrics.databaseLoadTime = phaseDuration;
            break;
        case BotWorldEntryState::PLAYER_CREATED:
            _metrics.playerCreationTime = phaseDuration;
            break;
        case BotWorldEntryState::MAP_LOADED:
            _metrics.mapLoadTime = phaseDuration;
            break;
        case BotWorldEntryState::IN_WORLD:
            _metrics.worldEntryTime = phaseDuration;
            break;
        case BotWorldEntryState::AI_ACTIVE:
            _metrics.aiInitTime = phaseDuration;
            break;
        default:
            break;
    }
}

bool BotWorldEntry::LoadCharacterData()
{
    TC_LOG_DEBUG("module.playerbot.worldentry",
                "Loading character data for bot {}",
                _characterGuid.ToString());

    // The session should handle the database loading
    // Here we just verify it was successful
    if (!_session)
    {
        SetError("Invalid session");
        return false;
    }

    // Check if character exists
    CharacterCacheEntry const* cacheEntry = sCharacterCache->GetCharacterCacheByGuid(_characterGuid);
    if (!cacheEntry)
    {
        // For bots, we may need to query directly
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_DATA_BY_GUID);
        stmt->setUInt64(0, _characterGuid.GetCounter());
        PreparedQueryResult result = CharacterDatabase.Query(stmt);

        if (!result)
        {
            SetError("Character not found in database");
            return false;
        }
    }

    // Mark data as loaded
    TransitionToState(BotWorldEntryState::CHARACTER_LOADED);
    return true;
}

bool BotWorldEntry::CreatePlayerObject()
{
    TC_LOG_DEBUG("module.playerbot.worldentry",
                "Creating player object for bot {}",
                _characterGuid.ToString());

    // Create the Player object
    _player = new Player(_session.get());
    _player->GetMotionMaster()->Initialize();

    // CRITICAL FIX: Use BotLoginQueryHolder to properly load ALL player data including spells
    // The old approach only loaded basic character data from CHAR_SEL_CHARACTER,
    // which does NOT include spells - causing crashes in Player::HasSpell()
    auto holder = ::std::make_shared<BotLoginQueryHolder>(_session->GetAccountId(), _characterGuid);
    if (!holder->Initialize())
    {
        SetError("Failed to initialize login query holder");
        delete _player;
        _player = nullptr;
        return false;
    }

    // Execute all login queries via async mechanism but wait synchronously
    // This uses the standard TrinityCore query holder pattern
    SQLQueryHolderCallback callback = CharacterDatabase.DelayQueryHolder(holder);

    // Wait for async queries to complete (blocking wait)
    callback.m_future.wait();

    // Now load the player using the properly populated holder
    if (!_player->LoadFromDB(_characterGuid, *holder))
    {
        SetError("Failed to load player data from query holder");
        delete _player;
        _player = nullptr;
        return false;
    }

    // Set player in session
    _session->SetPlayer(_player);

    // Initialize player for bot
    _player->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_IS_OUT_OF_BOUNDS); // Mark as bot
    _player->SetInitialized(true);

    TC_LOG_DEBUG("module.playerbot.worldentry",
                "Bot {} loaded successfully with {} spells",
                _player->GetName(), _player->GetSpellMap().size());

    TransitionToState(BotWorldEntryState::PLAYER_CREATED);
    return true;
}

bool BotWorldEntry::LoadTargetMap()
{
    if (!_player)
    {
        SetError("Player object not created");
        return false;
    }

    TC_LOG_DEBUG("module.playerbot.worldentry",
                "Loading map {} for bot {}",
                _player->GetMapId(),
                _characterGuid.ToString());

    // Get the map where the player should be
    Map* map = sMapMgr->CreateMap(_player->GetMapId(), _player);
    if (!map)
    {
        SetError("Failed to create/load map");
        return false;
    }

    // Verify map is valid and can accept player
    if (map->CannotEnter(_player) != TRANSFER_ABORT_NONE)
    {
        SetError("Map cannot accept player");
        return false;
    }

    TransitionToState(BotWorldEntryState::MAP_LOADED);
    return true;
}

bool BotWorldEntry::AddPlayerToWorld()
{
    if (!_player)
    {
        SetError("Player object not created");
        return false;
    }

    TC_LOG_DEBUG("module.playerbot.worldentry",
                "Adding bot {} to world",
                _characterGuid.ToString());

    // Send initial packets before adding to map
    SendInitialPacketsBeforeMap();

    // Get the target map
    Map* map = sMapMgr->CreateMap(_player->GetMapId(), _player);
    if (!map)
    {
        SetError("Failed to get map for player");
        return false;
    }

    // Add player to map
    if (!map->AddPlayerToMap(_player))
    {
        SetError("Failed to add player to map");
        return false;
    }

    // Player is now in world
    _player->SetInWorld(true);

    // Initialize visibility
    _player->UpdateObjectVisibility();

    // Send initial packets after adding to map
    SendInitialPacketsAfterMap();

    // Initialize position and movement
    InitializeBotPosition();

    // Set up appearance
    InitializeBotAppearance();

    // Register with object accessor for finding
    sObjectAccessor->AddObject(_player);

    TC_LOG_INFO("module.playerbot.worldentry",
               "Bot {} successfully added to world at position ({:.2f}, {:.2f}, {:.2f}) in map {}",
               _player->GetName(),
               _player->GetPositionX(),
               _player->GetPositionY(),
               _player->GetPositionZ(),
               _player->GetMapId());

    TransitionToState(BotWorldEntryState::IN_WORLD);
    return true;
}

bool BotWorldEntry::InitializeAI()
{
    if (!_player || !_player->IsInWorld())
    {
        SetError("Player not in world");
        return false;
    }

    TC_LOG_DEBUG("module.playerbot.worldentry",
                "Initializing AI for bot {}",
                _characterGuid.ToString());

    // CRITICAL FIX: Use BotAIFactory to create class-specific AI (WarriorAI, MageAI, etc.)
    // instead of base BotAI. This ensures QuestManager and all automation systems are initialized.
    auto botAI = sBotAIFactory->CreateAI(_player);
    if (!botAI)
    {
        SetError("Failed to create bot AI");
        return false;
    }

    BotAI* ai = botAI.get();

    // Initialize AI systems
    ai->Reset();

    // CRITICAL FIX: ALWAYS register AI with Player so Unit::UpdateAI() can find it!
    // The Unit system calls GetAI()->UpdateAI(diff) every frame - if GetAI() returns nullptr,
    // the bot will never update and will stand idle doing nothing.
    _player->SetAI(ai);

    // ALSO store AI in session for reference (session owns the AI memory)
    if (BotSession* botSession = dynamic_cast<BotSession*>(_session.get()))
    {
        // P1 FIX: Pass unique_ptr directly via std::move (no .release() needed)
        botSession->SetAI(::std::move(botAI));
    }
    else
    {
        // If not BotSession, transfer ownership to player
        botAI.release(); // Player now owns the AI
    }

    // Start AI updates
    ai->OnRespawn();

    // CRITICAL FIX: If bot is already in a group at server startup, activate follow behavior
    Group* group = _player->GetGroup();
    if (group)
    {
        TC_LOG_INFO("module.playerbot.worldentry",
                   "Bot {} is in group at startup - activating follow behavior for leader {}",
                   _player->GetName(),
                   ObjectAccessor::FindPlayer(group->GetLeaderGUID()) ?
                   ObjectAccessor::FindPlayer(group->GetLeaderGUID())->GetName() : "Unknown");

        // Trigger group join to initialize follow strategy
        ai->OnGroupJoined(group);
    }

    TC_LOG_INFO("module.playerbot.worldentry",
               "AI initialized for bot {}",
               _player->GetName());

    TransitionToState(BotWorldEntryState::AI_ACTIVE);
    return true;
}

bool BotWorldEntry::FinalizeBotActivation()
{
    if (!_player || !_player->IsInWorld())
    {
        SetError("Player not in world");
        return false;
    }

    TC_LOG_DEBUG("module.playerbot.worldentry",
                "Finalizing activation for bot {}",
                _characterGuid.ToString());

    // Update last login time
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHARACTER_ONLINE);
    stmt->setUInt8(0, 1); // online
    stmt->setUInt64(1, _characterGuid.GetCounter());
    CharacterDatabase.Execute(stmt);

    // Join world channel if configured
    if (sWorld->getBoolConfig(CONFIG_CHAT_WORLD_ENABLE))
    {
        if (ChannelMgr* cMgr = ChannelMgr::forTeam(_player->GetTeam()))
        {
            if (Channel* world = cMgr->GetJoinChannel(sWorld->getStringConfig(CONFIG_GLOBAL_CHANNEL_NAME), 0))
            {
                world->JoinChannel(_player);
            }
        }
    }

    // Load social lists
    sSocialMgr->LoadFromDB(nullptr, _characterGuid);

    // Update guild information if in guild
    if (Guild* guild = sGuildMgr->GetGuildById(_player->GetGuildId()))
    {
        guild->OnLogin(_player);
    }

    // ========================================================================
    // LEARN ALL FLIGHT PATHS (using sTaxiNodesMask for complete coverage)
    // ========================================================================
    // Bots spawn across all zones and need complete taxi knowledge for the
    // multi-station travel system to work. Using sTaxiNodesMask (ALL nodes)
    // instead of faction masks to ensure bots can reach all destinations.
    //
    // NOTE: Faction masks (sHordeTaxiNodesMask/sAllianceTaxiNodesMask) only
    // include nodes with ShowOnHordeMap/ShowOnAllianceMap flags, which
    // excludes many neutral and expansion-specific nodes like Draenor 1476.
    // ========================================================================
    {
        // Use the complete taxi mask (all valid taxi network nodes)
        TaxiMask const& allNodesMask = sTaxiNodesMask;

        // Learn ALL taxi nodes for comprehensive travel capability
        uint32 nodesLearned = 0;
        for (size_t i = 0; i < allNodesMask.size(); ++i)
        {
            TaxiMask::value_type mask = allNodesMask[i];
            if (mask == 0)
                continue;

            // Check each bit in this mask element
            for (size_t bit = 0; bit < sizeof(TaxiMask::value_type) * 8; ++bit)
            {
                if (mask & (TaxiMask::value_type(1) << bit))
                {
                    // Calculate node ID: (element index * bits per element) + bit position + 1
                    uint32 nodeId = static_cast<uint32>(i * (sizeof(TaxiMask::value_type) * 8) + bit + 1);

                    // Learn this taxi node if not already known
                    if (_player->m_taxi.SetTaximaskNode(nodeId))
                    {
                        ++nodesLearned;
                    }
                }
            }
        }

        TC_LOG_INFO("module.playerbot.worldentry",
                    "Bot {} learned {} flight paths (ALL nodes)",
                    _player->GetName(),
                    nodesLearned);
    }

    // ========================================================================
    // EQUIP BOT WITH LEVEL-APPROPRIATE GEAR
    // ========================================================================
    // Bots created via the spawner may not have equipment. Check if the bot
    // is missing key equipment slots and generate/apply gear if needed.
    // ========================================================================
    {
        // Check if bot needs gear - check critical slots (weapon + chest)
        bool needsGear = false;
        Item* mainHand = _player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
        Item* chest = _player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_CHEST);

        if (!mainHand || !chest)
        {
            needsGear = true;
            TC_LOG_INFO("module.playerbot.worldentry",
                "Bot {} (L{}) missing equipment: mainhand={}, chest={}",
                _player->GetName(), _player->GetLevel(),
                mainHand ? "yes" : "NO", chest ? "yes" : "NO");
        }

        // Only equip if gear factory is ready and bot needs gear
        if (needsGear && sBotGearFactory->IsReady())
        {
            uint8 cls = _player->GetClass();
            uint32 level = _player->GetLevel();
            TeamId faction = _player->GetTeamId();

            // Use spec 0 (first spec) for gear generation
            uint32 specId = 0;

            TC_LOG_INFO("module.playerbot.worldentry",
                "Generating gear for bot {} (Class: {}, Level: {}, Faction: {})",
                _player->GetName(), uint32(cls), level, faction == TEAM_HORDE ? "Horde" : "Alliance");

            // Generate gear set
            GearSet gearSet = sBotGearFactory->BuildGearSet(cls, specId, level, faction);

            if (gearSet.IsComplete())
            {
                // Apply gear set to bot
                bool success = sBotGearFactory->ApplyGearSet(_player, gearSet);

                if (success)
                {
                    TC_LOG_INFO("module.playerbot.worldentry",
                        "Bot {} equipped with {} items (avg ilvl: {:.1f})",
                        _player->GetName(), gearSet.items.size(), gearSet.averageIlvl);

                    // Note: ApplyGearSet already calls SaveToDB (with Item.cpp:1304 crash protection)
                    // No need for duplicate save here
                }
                else
                {
                    TC_LOG_WARN("module.playerbot.worldentry",
                        "Failed to apply gear set to bot {}", _player->GetName());
                }
            }
            else
            {
                TC_LOG_WARN("module.playerbot.worldentry",
                    "Generated incomplete gear set for bot {} (items: {})",
                    _player->GetName(), gearSet.items.size());
            }
        }
        else if (needsGear && !sBotGearFactory->IsReady())
        {
            TC_LOG_WARN("module.playerbot.worldentry",
                "Bot {} needs gear but BotGearFactory is not ready", _player->GetName());
        }
    }

    // Set bot as fully active
    _player->SetCanModifyStats(true);
    _player->UpdateAllStats();
    _player->UpdateAllCritPercentages();
    _player->UpdateAllSpellCritChances();
    _player->UpdateArmor();

    // Send final initialization complete
    WorldPackets::Misc::SetTimeZoneInformation packet;
    _session->SendPacket(packet.Write());

    TC_LOG_INFO("module.playerbot.worldentry",
               "Bot {} is now fully active in world",
               _player->GetName());

    // CRITICAL FIX: Check if bot is already in a group (player logged in while grouped)
    // If so, activate follow behavior immediately
    if (Group* group = _player->GetGroup())
    {
        TC_LOG_INFO("module.playerbot.worldentry",
                   "Bot {} is already in group on login, activating follow behavior",
                   _player->GetName());

        // Get AI and call OnGroupJoined to activate follow
    if (BotAI* botAI = dynamic_cast<BotAI*>(_player->GetAI()))
        {
            botAI->OnGroupJoined(group);
        }
    }

    // Record memory usage after successful world entry
    _metrics.memoryAfterEntry = GetCurrentMemoryUsage();

    // Calculate total time
    _metrics.endTime = ::std::chrono::steady_clock::now();
    _metrics.totalTime = static_cast<uint32>(
        ::std::chrono::duration_cast<::std::chrono::microseconds>(
            _metrics.endTime - _metrics.startTime
        ).count()
    );

    TC_LOG_DEBUG("module.playerbot.worldentry",
                "Bot {} world entry complete: Total time: {} ms, Memory delta: {} KB",
                _player->GetName(),
                _metrics.totalTime / 1000,
                (_metrics.memoryAfterEntry - _metrics.memoryBeforeEntry) / 1024);

    TransitionToState(BotWorldEntryState::FULLY_ACTIVE);
    return true;
}

void BotWorldEntry::SendInitialPacketsBeforeMap()
{
    if (!_player || !_session)
        return;

    // Send login verification
    WorldPackets::Character::LoginVerifyWorld packet;
    packet.MapID = _player->GetMapId();
    packet.Pos = _player->GetPosition();
    packet.Orientation = _player->GetOrientation();
    _session->SendPacket(packet.Write());

    // Account data times (empty for bots)
    WorldPackets::ClientConfig::AccountDataTimes accountData;
    accountData.PlayerGuid = _player->GetGUID();
    accountData.ServerTime = GameTime::GetGameTime();
    _session->SendPacket(accountData.Write());

    // Features
    WorldPackets::Character::FeatureSystemStatus features;
    features.ComplaintStatus = 2;
    features.ScrollOfResurrectionRequestsRemaining = 0;
    features.ScrollOfResurrectionMaxRequestsPerDay = 0;
    features.CfgRealmID = 0;
    features.CfgRealmRecID = 0;
    features.EuropaTicketSystemStatus = false;
    features.GameRulesEnabled = false;
    features.BpayStoreAvailable = false;
    features.BpayStoreDisabledByParentalControls = false;
    features.ItemRestorationButtonEnabled = false;
    features.BrowserEnabled = false;
    _session->SendPacket(features.Write());

    // MOTD
    WorldPackets::Character::MOTD motd;
    motd.Text = sWorld->GetMotd();
    _session->SendPacket(motd.Write());

    // Set rest bonus
    _player->SetRestBonus(_player->GetRestBonus());
}

void BotWorldEntry::SendInitialPacketsAfterMap()
{
    if (!_player || !_session)
        return;

    // Update time speed
    WorldPackets::Misc::LoginSetTimeSpeed loginSetTimeSpeed;
    loginSetTimeSpeed.ServerTime = GameTime::GetGameTime();
    loginSetTimeSpeed.GameTime = GameTime::GetGameTime();
    loginSetTimeSpeed.NewSpeed = 0.01666667f;
    _session->SendPacket(loginSetTimeSpeed.Write());

    // Send initial spells
    _player->SendInitialSpells();

    // Send action bars
    _player->SendInitialActionButtons();

    // Initialize factions
    _player->GetReputationMgr().SendInitialReputations();

    // Send taxi nodes
    _player->SendTaxiMenu(_player);

    // Send equipment sets
    _player->SendEquipmentSetList();

    // Update zone
    _player->UpdateZone(_player->GetZoneId(), _player->GetAreaId());

    // CRITICAL FIX: Initialize phasing data for the bot
    // Without this, the bot's PhaseShift VisibleMapIds container is uninitialized
    // causing crashes in PhasingHandler::GetTerrainMapId() when path generation tries
    // to query terrain height during bot movement.
    PhasingHandler::OnMapChange(_player);
}

void BotWorldEntry::InitializeBotPosition()
{
    if (!_player)
        return;

    // Ensure player is at a valid position
    if (!_player->IsPositionValid())
    {
        TC_LOG_WARN("module.playerbot.worldentry",
                   "Bot {} has invalid position, relocating to homebind",
                   _player->GetName());

        // Teleport to homebind
        _player->TeleportTo(_player->m_homebind);
    }

    // Make sure player is not falling
    if (_player->HasUnitMovementFlag(MOVEMENTFLAG_FALLING))
    {
        _player->RemoveUnitMovementFlag(MOVEMENTFLAG_FALLING);
        _player->SetFallInformation(GameTime::GetGameTimeMS(), _player->GetPositionZ());
    }

    // Stop any movement
    _player->StopMoving();
}

void BotWorldEntry::InitializeBotAppearance()
{
    if (!_player)
        return;

    // Ensure player has proper display
    if (!_player->GetDisplayId())
    {
        _player->SetDisplayId(_player->GetNativeDisplayId());
    }

    // Update model/race/gender if needed
    _player->InitDisplayIds();

    // Make visible
    _player->SetVisibility(VISIBILITY_ON);
}

void BotWorldEntry::HandleWorldEntryFailure(::std::string const& reason)
{
    TC_LOG_ERROR("module.playerbot.worldentry",
                "Bot {} world entry failed: {}",
                _characterGuid.ToString(),
                reason);

    SetError(reason);
    TransitionToState(BotWorldEntryState::FAILED);

    // Invoke callback with failure
    {
        ::std::lock_guard lock(_callbackMutex);
        if (_callback)
        {
            _callback(false, _metrics);
            _callback = nullptr;
        }
    }

    // Clean up
    Cleanup();

    _processing = false;
}

void BotWorldEntry::Cleanup()
{
    TransitionToState(BotWorldEntryState::CLEANUP);

    // PLAYERBOT FIX: Do NOT call LogoutPlayer() or RemoveFromWorld() directly!
    // Problem: BotWorldEntry::Cleanup() can be called from ANY thread (including Map worker threads)
    //          If we call LogoutPlayer() here, it removes player from map IMMEDIATELY (WorldSession.cpp:715)
    //          This invalidates Map iterators, causing Map.cpp:686 crash
    //
    // Solution: Use KickPlayer() → sets forceExit flag → BotSession::Update() returns false
    //           → BotWorldSessionMgr removes session from map → ~WorldSession() calls LogoutPlayer()
    //
    // Flow: Cleanup() → KickPlayer() (sets forceExit) → _session.reset() (releases our reference)
    //       → Update() returns false → _botSessions.erase() (last reference gone)
    //       → ~WorldSession() → LogoutPlayer() on main thread during next UpdateSessions() = SAFE!
    if (_session && _player)
    {
        // Signal session termination - BotSession::Update() will return false next cycle
        _session->KickPlayer("BotWorldEntry::Cleanup - Bot lifecycle ended");
        _player = nullptr;

        // Release our reference to session - when _botSessions.erase() is called,
        // this will be the last reference, triggering destructor on main thread
        _session.reset();
    }
}

// === BotWorldEntryQueue Implementation ===

BotWorldEntryQueue* BotWorldEntryQueue::instance()
{
    static BotWorldEntryQueue instance;
    return &instance;
}

uint32 BotWorldEntryQueue::QueueEntry(::std::shared_ptr<BotWorldEntry> entry)
{
    if (!entry)
        return 0;

    ::std::lock_guard lock(_queueMutex);

    _pendingQueue.push(entry);
    return static_cast<uint32>(_pendingQueue.size());
}

void BotWorldEntryQueue::ProcessQueue(uint32 maxConcurrent)
{
    ::std::lock_guard lock(_queueMutex);

    // Remove completed entries
    _activeEntries.erase(
        ::std::remove_if(_activeEntries.begin(), _activeEntries.end(),
            [this](::std::shared_ptr<BotWorldEntry> const& entry)
            {
                if (!entry->IsProcessing())
                {
                    if (entry->IsComplete())
                    {
                        _totalCompleted++;
                        _totalEntryTime += entry->GetMetrics().totalTime;
                    }
                    else if (entry->IsFailed())
                    {
                        _totalFailed++;
                    }
                    return true;  // Remove completed or failed entries
                }
                return false;  // Keep entries that are still processing
            }),
        _activeEntries.end()
    );

    // Process active entries
    for (auto& entry : _activeEntries)
    {
        entry->ProcessWorldEntry(100);
    }

    // Start new entries if below concurrent limit
    while (_activeEntries.size() < maxConcurrent && !_pendingQueue.empty())
    {
        auto entry = _pendingQueue.front();
        _pendingQueue.pop();

        if (entry->BeginWorldEntry())
        {
            _activeEntries.push_back(entry);
        }
    }
}

BotWorldEntryQueue::QueueStats BotWorldEntryQueue::GetStats() const
{
    ::std::lock_guard lock(_queueMutex);

    QueueStats stats;
    stats.queuedEntries = static_cast<uint32>(_pendingQueue.size());
    stats.activeEntries = static_cast<uint32>(_activeEntries.size());
    stats.completedEntries = _totalCompleted.load();
    stats.failedEntries = _totalFailed.load();

    if (_totalCompleted > 0)
    {
        stats.averageEntryTime = static_cast<float>(_totalEntryTime.load()) /
                                 static_cast<float>(_totalCompleted.load()) / 1000000.0f; // Convert to seconds
    }
    else
    {
        stats.averageEntryTime = 0.0f;
    }

    return stats;
}

void BotWorldEntryQueue::ClearQueue()
{
    ::std::lock_guard lock(_queueMutex);

    while (!_pendingQueue.empty())
    {
        _pendingQueue.pop();
    }

    _activeEntries.clear();

    TC_LOG_WARN("module.playerbot.worldentry", "Bot world entry queue cleared");
}

} // namespace Playerbot