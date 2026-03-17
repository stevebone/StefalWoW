# BotSession Complete Implementation Specification

## Overview
High-performance, socketless WorldSession implementation with hibernation, object pooling, and enterprise-grade optimizations.

## Class Declaration

### Header File: `src/modules/Playerbot/Session/BotSession.h`

```cpp
/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#ifndef BOT_SESSION_H
#define BOT_SESSION_H

#include "WorldSession.h"
#include <atomic>
#include <array>
#include <boost/circular_buffer.hpp>
#include <boost/pool/object_pool.hpp>
#include <tbb/concurrent_queue.h>
#include <chrono>
#include <memory>

/**
 * CRITICAL IMPLEMENTATION RULES:
 *
 * 1. MEMORY RULES:
 *    - MUST use boost::circular_buffer for bounded queues
 *    - MUST use boost::object_pool for packet allocation
 *    - MUST implement hibernation after 5 minutes inactivity
 *    - MUST achieve < 500KB RAM per active session
 *    - MUST achieve < 5KB RAM per hibernated session
 *
 * 2. CPU RULES:
 *    - NO mutex in hot paths (use atomics only)
 *    - MUST use Intel TBB concurrent structures
 *    - MUST batch process minimum 16 packets
 *    - MUST skip hibernated sessions in Update()
 *    - MUST achieve < 500ns per packet processing
 *
 * 3. TRINITY INTEGRATION RULES:
 *    - ONLY override: SendPacket, QueuePacket, Update
 *    - NEVER modify other WorldSession methods
 *    - MUST use WorldSession's existing Player* management
 *    - MUST initialize with nullptr socket
 */
class TC_GAME_API BotSession final : public WorldSession
{
public:
    explicit BotSession(uint32 accountId);
    virtual ~BotSession();

    // === MINIMAL WorldSession Overrides ===
    void SendPacket(WorldPacket const* packet, bool forced = false) override;
    void QueuePacket(WorldPacket&& packet) override;
    void Update(uint32 diff) override;

    // Query methods
    bool IsConnectionIdle() const override { return false; }
    uint32 GetLatency() const override { return _simulatedLatency; }

    // === Bot-Specific High-Performance Methods ===

    // Lock-free packet processing with time budget
    void ProcessBotPackets(uint32 maxProcessingTimeUs);

    // Memory optimization via hibernation
    void Hibernate();
    void Reactivate();
    bool IsHibernated() const { return _state.load() == State::HIBERNATED; }

    // Bot metadata queries - Links to Trinity's standard account/character data
    void ExecuteBotMetadataQuery(PreparedStatement* stmt);

    // AI Integration
    void SetAI(std::unique_ptr<BotAI> ai) { _ai = std::move(ai); }
    BotAI* GetAI() const { return _ai.get(); }

    // Performance monitoring
    struct Metrics {
        std::atomic<uint32> packetsProcessed{0};
        std::atomic<uint32> bytesProcessed{0};
        std::atomic<uint32> cpuTimeUs{0};
        std::atomic<size_t> memoryUsage{0};
        std::atomic<uint32> hibernationCount{0};
        std::atomic<uint32> cacheHits{0};
        std::atomic<uint32> cacheMisses{0};
    };

    Metrics const& GetMetrics() const { return _metrics; }
    bool IsBot() const { return true; }
    bool IsActive() const { return _state.load() == State::ACTIVE; }

private:
    // Cache-line aligned state for performance
    alignas(64) struct {
        enum class State : uint8 {
            INITIALIZING,
            ACTIVE,
            HIBERNATED,
            DESTROYING
        };
        std::atomic<State> _state{State::INITIALIZING};
        std::atomic<bool> _processingPackets{false};
        std::atomic<uint32> _simulatedLatency{50}; // 50ms simulated latency
    };

    // === HIGH-PERFORMANCE PACKET QUEUES ===

    // Intel TBB concurrent queues for lock-free operation
    tbb::concurrent_bounded_queue<WorldPacket> _incomingPackets;
    tbb::concurrent_bounded_queue<WorldPacket> _outgoingPackets;

    // Object pool for zero-allocation packet processing
    static thread_local boost::object_pool<WorldPacket> s_packetPool;

    // === HIBERNATION SYSTEM ===

    struct HibernatedState {
        // LZ4 compressed session data
        std::vector<uint8> compressedData;
        uint32 lastUpdateTime;

        // Essential player state
        struct PlayerState {
            float posX, posY, posZ, orientation;
            uint32 mapId;
            uint32 level;
            uint32 health, mana;
            uint32 gold;
        } playerState;

        // AI state compression
        std::vector<uint8> aiStateCompressed;

        size_t GetMemorySize() const {
            return sizeof(*this) + compressedData.size() + aiStateCompressed.size();
        }
    };
    std::unique_ptr<HibernatedState> _hibernatedState;

    // === CPU OPTIMIZATION STRUCTURES ===

    // Batch processing for CPU efficiency
    static constexpr size_t BATCH_SIZE = 32;
    std::array<WorldPacket*, BATCH_SIZE> _batchBuffer;
    std::atomic<uint8> _batchCount{0};

    // Skip-frame optimization for hibernated sessions
    std::atomic<uint8> _skipFrames{0};
    static constexpr uint8 MAX_SKIP_FRAMES = 10;

    // SIMD-aligned memory pools
    alignas(32) std::array<uint8, 4096> _scratchBuffer;
    std::atomic<size_t> _scratchUsed{0};

    // === PERFORMANCE METRICS ===
    mutable Metrics _metrics;
    std::chrono::steady_clock::time_point _lastActivity;
    std::chrono::steady_clock::time_point _creationTime;

    // === AI INTEGRATION ===
    std::unique_ptr<BotAI> _ai;
    std::atomic<bool> _aiEnabled{false};

    // === INTERNAL HIGH-PERFORMANCE METHODS ===

    // Inline hot-path methods for maximum performance
    inline bool ShouldHibernate() const;
    inline bool ShouldSkipUpdate() const;
    inline void UpdateMetrics(size_t packetSize, uint32 processingTimeUs);
    inline WorldPacket* AcquirePacket();
    inline void ReleasePacket(WorldPacket* packet);

    // Batch processing - CRITICAL PERFORMANCE PATH
    void ProcessPacketBatch();
    void FlushBatchBuffer();

    // Memory management with object pools
    void InitializeObjectPools();
    void CompactMemoryFootprint();
    void ReleaseAllPooledObjects();

    // Hibernation implementation with LZ4 compression
    void SerializeToHibernation();
    void DeserializeFromHibernation();
    void CompressSessionState();
    void DecompressSessionState();

    // SIMD optimized operations where applicable
    void SimdCopyPacketData(const uint8* src, uint8* dst, size_t size);
    void SimdClearBuffer(uint8* buffer, size_t size);

    // Database query caching
    struct QueryCache {
        static constexpr size_t CACHE_SIZE = 256;
        struct CacheEntry {
            uint32 queryHash;
            PreparedQueryResult result;
            std::chrono::steady_clock::time_point expiry;
        };
        std::array<CacheEntry, CACHE_SIZE> entries;
        std::atomic<size_t> nextIndex{0};
    };
    QueryCache _queryCache;

    uint32 HashQuery(PreparedStatement const* stmt) const;
    bool GetCachedQuery(uint32 hash, PreparedQueryResult& result);
    void CacheQueryResult(uint32 hash, PreparedQueryResult const& result);
};

// === INLINE IMPLEMENTATIONS FOR MAXIMUM PERFORMANCE ===

inline bool BotSession::ShouldHibernate() const
{
    auto now = std::chrono::steady_clock::now();
    auto inactiveTime = std::chrono::duration_cast<std::chrono::minutes>(now - _lastActivity);
    return inactiveTime.count() >= 5 && _state.load(std::memory_order_relaxed) == State::ACTIVE;
}

inline bool BotSession::ShouldSkipUpdate() const
{
    return _skipFrames.load(std::memory_order_relaxed) > 0 &&
           _state.load(std::memory_order_relaxed) == State::HIBERNATED;
}

inline void BotSession::UpdateMetrics(size_t packetSize, uint32 processingTimeUs)
{
    _metrics.packetsProcessed.fetch_add(1, std::memory_order_relaxed);
    _metrics.bytesProcessed.fetch_add(packetSize, std::memory_order_relaxed);
    _metrics.cpuTimeUs.fetch_add(processingTimeUs, std::memory_order_relaxed);
    _lastActivity = std::chrono::steady_clock::now();
}

inline WorldPacket* BotSession::AcquirePacket()
{
    return s_packetPool.construct();
}

inline void BotSession::ReleasePacket(WorldPacket* packet)
{
    if (packet) {
        packet->clear();
        s_packetPool.destroy(packet);
    }
}

#endif // BOT_SESSION_H
```

## Implementation Requirements

### Dependency Integration
```cmake
# Required in CMakeLists.txt
find_package(TBB REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

target_link_libraries(playerbot-session
    PRIVATE
        TBB::tbb
        Boost::system)
```

### Constructor Implementation
```cpp
BotSession::BotSession(uint32 accountId)
    : WorldSession(accountId, "", 0, nullptr, SEC_PLAYER,
                  EXPANSION_LEVEL_CURRENT, 0, "", Minutes(0), 0,
                  ClientBuild::Classic, LOCALE_enUS, 0, false)
{
    // Initialize TBB queues with capacity limits
    _incomingPackets.set_capacity(256);
    _outgoingPackets.set_capacity(256);

    // Initialize timestamps
    _lastActivity = std::chrono::steady_clock::now();
    _creationTime = _lastActivity;

    // Initialize object pools
    InitializeObjectPools();

    // Set initial state
    _state.store(State::INITIALIZING, std::memory_order_release);

    TC_LOG_DEBUG("module.playerbot.session",
        "Created high-performance BotSession for account {}", accountId);
}
```

### Performance-Critical Methods

#### SendPacket Implementation
```cpp
void BotSession::SendPacket(WorldPacket const* packet, bool forced)
{
    if (!packet) return;

    auto startTime = std::chrono::high_resolution_clock::now();

    // Update activity timestamp (hot path optimization)
    _lastActivity = std::chrono::steady_clock::now();

    // Skip if hibernated unless forced
    if (!forced && IsHibernated()) {
        _metrics.packetsProcessed.fetch_add(1, std::memory_order_relaxed);
        return;
    }

    // Try lock-free queue push
    WorldPacket outPacket(*packet);
    if (!_outgoingPackets.try_push(std::move(outPacket))) {
        // Queue full - this is a performance issue, log it
        TC_LOG_WARN("module.playerbot.session",
            "Outgoing packet queue full for account {}, packet dropped. "
            "Performance degradation detected.", GetAccountId());
        return;
    }

    // Update performance metrics
    auto endTime = std::chrono::high_resolution_clock::now();
    uint32 processingTimeUs = static_cast<uint32>(
        std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count());

    UpdateMetrics(packet->size(), processingTimeUs);
}
```

#### Update Method Implementation
```cpp
void BotSession::Update(uint32 diff)
{
    // Skip update optimization for hibernated sessions
    if (ShouldSkipUpdate()) {
        uint8 currentSkip = _skipFrames.load(std::memory_order_relaxed);
        if (currentSkip > 0) {
            _skipFrames.store(currentSkip - 1, std::memory_order_relaxed);
        }
        return;
    }

    auto updateStartTime = std::chrono::high_resolution_clock::now();

    // Hibernation check with hysteresis
    if (ShouldHibernate()) {
        Hibernate();
        return;
    }

    // High-performance packet processing with time budget
    uint32 maxProcessingTimeUs = sPlayerbotConfig->GetUInt(
        "Playerbot.Session.MaxUpdateTimeUs", 1000); // 1ms default

    ProcessBotPackets(maxProcessingTimeUs);

    // Conditional base class update with null filter
    if (GetPlayer() && _state.load(std::memory_order_relaxed) == State::ACTIVE) {
        struct NullPacketFilter : public PacketFilter {
            explicit NullPacketFilter(WorldSession* session) : PacketFilter(session) {}
            bool Process(WorldPacket*) override { return false; }
            bool ProcessUnsafe() const override { return false; }
        } nullFilter(this);

        WorldSession::Update(diff, nullFilter);
    }

    // Update performance metrics
    auto updateEndTime = std::chrono::high_resolution_clock::now();
    uint32 totalUpdateTimeUs = static_cast<uint32>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            updateEndTime - updateStartTime).count());

    _metrics.cpuTimeUs.fetch_add(totalUpdateTimeUs, std::memory_order_relaxed);
}
```

## Hibernation System Specification

### Memory Compression Requirements
- **Target**: 99% memory reduction during hibernation
- **Method**: LZ4 compression for session state
- **Trigger**: 5 minutes of inactivity
- **Recovery**: < 10ms reactivation time

### Hibernation Implementation
```cpp
void BotSession::Hibernate()
{
    if (_state.load(std::memory_order_relaxed) == State::HIBERNATED) return;

    TC_LOG_DEBUG("module.playerbot.session",
        "Hibernating session for account {} after inactivity", GetAccountId());

    auto hibernationStart = std::chrono::high_resolution_clock::now();

    // Serialize current state with compression
    SerializeToHibernation();

    // Clear all packet queues to free memory
    WorldPacket dummy;
    while (_incomingPackets.try_pop(dummy)) { /* drain queue */ }
    while (_outgoingPackets.try_pop(dummy)) { /* drain queue */ }

    // Release pooled objects
    ReleaseAllPooledObjects();

    // Compact memory footprint
    CompactMemoryFootprint();

    // Update state atomically
    _state.store(State::HIBERNATED, std::memory_order_release);
    _skipFrames.store(MAX_SKIP_FRAMES, std::memory_order_relaxed);

    // Update metrics
    auto hibernationEnd = std::chrono::high_resolution_clock::now();
    uint32 hibernationTimeUs = static_cast<uint32>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            hibernationEnd - hibernationStart).count());

    _metrics.hibernationCount.fetch_add(1, std::memory_order_relaxed);
    _metrics.memoryUsage.store(
        _hibernatedState ? _hibernatedState->GetMemorySize() : 0,
        std::memory_order_relaxed);

    TC_LOG_DEBUG("module.playerbot.session",
        "Session hibernated in {}μs, memory reduced to {} bytes",
        hibernationTimeUs, _metrics.memoryUsage.load());
}
```

## Performance Testing Requirements

### Benchmark Specifications
```cpp
// MANDATORY: All these benchmarks MUST pass
static void BM_PacketProcessing(benchmark::State& state) {
    BotSession session(10000);

    for (auto _ : state) {
        WorldPacket packet(CMSG_MESSAGECHAT, 64);
        session.SendPacket(&packet);
        session.ProcessBotPackets(1000); // 1ms budget
    }

    // REQUIREMENT: < 500ns per packet
    state.SetItemsProcessed(state.iterations());
}

static void BM_SessionCreation(benchmark::State& state) {
    for (auto _ : state) {
        auto session = std::make_unique<BotSession>(10000 + state.iterations());
        benchmark::DoNotOptimize(session.get());
    }

    // REQUIREMENT: < 1ms per session creation
}

static void BM_HibernationCycle(benchmark::State& state) {
    BotSession session(10000);

    for (auto _ : state) {
        session.Hibernate();
        benchmark::DoNotOptimize(session.IsHibernated());
        session.Reactivate();
        benchmark::DoNotOptimize(session.IsActive());
    }

    // REQUIREMENT: < 10ms hibernation, < 10ms reactivation
}
```

### Memory Requirements Validation
```cpp
TEST(BotSessionMemoryTest, ActiveSessionMemoryLimit) {
    size_t baselineMemory = GetProcessMemoryUsage();

    auto session = std::make_unique<BotSession>(10000);
    // Simulate active usage
    for (int i = 0; i < 1000; ++i) {
        WorldPacket packet(CMSG_MESSAGECHAT, 64);
        session->SendPacket(&packet);
    }

    size_t sessionMemory = GetProcessMemoryUsage() - baselineMemory;

    // REQUIREMENT: < 500KB per active session
    EXPECT_LT(sessionMemory, 500 * 1024);
}

TEST(BotSessionMemoryTest, HibernatedSessionMemoryLimit) {
    auto session = std::make_unique<BotSession>(10000);

    // Measure active memory
    size_t activeMemory = session->GetMetrics().memoryUsage.load();

    // Hibernate and measure
    session->Hibernate();
    size_t hibernatedMemory = session->GetMetrics().memoryUsage.load();

    // REQUIREMENT: < 5KB hibernated, 99% reduction
    EXPECT_LT(hibernatedMemory, 5 * 1024);
    EXPECT_LT(hibernatedMemory, activeMemory / 100);
}
```

## Database Integration (CORRECTED)

### Proper Trinity Database Usage
```cpp
void BotSession::InitializeBotData()
{
    // Bot account exists in auth.account (created via AccountMgr::CreateAccount)
    // Bot character exists in characters.characters (created via Player::Create)

    // Load ONLY bot-specific metadata from playerbot database
    PreparedStatement* stmt = sBotDBPool->GetPreparedStatement(BOT_SEL_AI_STATE);
    stmt->SetData(0, GetPlayer() ? GetPlayer()->GetGUID().GetCounter() : 0);

    sBotDBPool->ExecuteAsync(stmt, [this](PreparedQueryResult result) {
        if (result) {
            // Load AI strategy, behavior settings, etc.
            ProcessAIState(result);
        }
    });
}

void BotSession::SaveBotMetadata()
{
    if (!GetPlayer()) return;

    // Save ONLY bot-specific metadata (NOT account/character data)
    PreparedStatement* stmt = sBotDBPool->GetPreparedStatement(BOT_UPD_AI_STATE);
    stmt->SetData(0, "current_strategy");
    stmt->SetData(1, GetAI() ? GetAI()->GetBehaviorData() : "");
    stmt->SetData(2, GetPlayer()->GetGUID().GetCounter());

    sBotDBPool->ExecuteAsyncNoResult(stmt);
}

void BotSession::LoadPlayerCharacter(uint32 characterGuid)
{
    // Use Trinity's standard character loading (characters.characters)
    // This happens automatically through WorldSession::LoadCharacterFromDB

    // After character loads, load bot-specific AI state
    if (GetPlayer()) {
        InitializeBotData();
    }
}
```

### Account/Character Creation Integration
```cpp
// CORRECT: BattleNet account creation (handled by BotAccountMgr using Trinity's AccountMgr)
uint32 bnetAccountId = sAccountMgr->CreateBattlenetAccount(email, password); // auth.bnet_account

// Get linked legacy account ID (automatically created)
uint32 legacyAccountId = sAccountMgr->GetAccountIdByBattlenetAccount(bnetAccountId); // auth.account

// Character creation (handled by BotCharacterMgr using Trinity's Player class)
uint32 characterGuid = Player::Create(legacyAccountId, ...); // characters.characters (uses legacy account ID)

// Link bot metadata (handled by playerbot database)
PreparedStatement* stmt = sBotDBPool->GetPreparedStatement(BOT_INS_CHARACTER_INFO);
stmt->SetData(0, characterGuid);
stmt->SetData(1, legacyAccountId);  // Legacy account ID for character linkage
stmt->SetData(2, bnetAccountId);    // BNet account ID for reference
stmt->SetData(3, "default");        // Initial AI strategy
sBotDBPool->ExecuteAsyncNoResult(stmt);
```

## Integration with BotSessionMgr

### Session Lifecycle Management
```cpp
class BotSessionMgr {
    // Session pool management
    BotSession* CreateSession(uint32 accountId) {
        auto session = std::make_unique<BotSession>(accountId);

        // Account already exists in auth.account
        // Character already exists in characters.characters
        // Initialize bot-specific metadata only
        session->InitializeBotData();

        // Register for updates
        RegisterForUpdates(session.get());

        return session.release();
    }

    void UpdateAllSessions(uint32 diff) {
        // Parallel update using Intel TBB
        tbb::parallel_for_each(_activeSessions.begin(), _activeSessions.end(),
            [diff](BotSession* session) {
                session->Update(diff);
            });
    }
};
```

**ENTERPRISE-GRADE IMPLEMENTATION. CORRECT DATABASE USAGE. NO SHORTCUTS.**