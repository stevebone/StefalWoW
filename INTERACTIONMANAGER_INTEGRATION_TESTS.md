# InteractionManager Integration Test Specification

**Document Version**: 1.0
**Date**: 2025-10-04
**Author**: Integration Test Orchestrator Agent
**Purpose**: Comprehensive integration test suite for InteractionManager state machine architecture

---

## Test Categories

### 1. State Machine Integration Tests

#### Test 1.1: Full State Transition Flow - Vendor Interaction
```cpp
TEST(InteractionManager_StateMachine, FullVendorFlow_AllStatesTransition)
{
    // SETUP
    Player* bot = CreateTestBot(POSITION_NEAR_VENDOR);
    Creature* vendor = CreateTestVendor(VENDOR_POSITION);
    InteractionManager* mgr = InteractionManager::Instance();

    // EXECUTE
    InteractionResult result = mgr->StartInteraction(bot, vendor, InteractionType::Vendor);

    // VERIFY: Initial state
    ASSERT_EQ(result, InteractionResult::Pending);
    InteractionContext* ctx = mgr->GetInteractionContext(bot);
    ASSERT_NE(ctx, nullptr);
    ASSERT_EQ(ctx->state, InteractionState::Approaching);

    // SIMULATE: Bot approaches vendor
    SimulateMovement(bot, vendor, 4.0f);  // Within interaction range
    mgr->ProcessInteractionState(bot, 100);

    // VERIFY: Approaching -> Initiating transition
    ASSERT_EQ(ctx->state, InteractionState::Initiating);

    // SIMULATE: Initiating completes (face target, set selection)
    mgr->ProcessInteractionState(bot, 100);

    // VERIFY: Initiating -> ExecutingAction (no gossip required)
    ASSERT_EQ(ctx->state, InteractionState::ExecutingAction);

    // SIMULATE: VendorInteraction handler completes
    MockVendorInteractionSuccess();
    mgr->ProcessInteractionState(bot, 100);

    // VERIFY: ExecutingAction -> Completing
    ASSERT_EQ(ctx->state, InteractionState::Completing);

    // FINAL: Interaction completed and cleaned up
    ASSERT_FALSE(mgr->HasActiveInteraction(bot));

    // METRICS
    InteractionMetrics metrics = mgr->GetMetrics(InteractionType::Vendor);
    ASSERT_EQ(metrics.successCount, 1);
    ASSERT_GT(metrics.successRate, 99.0f);
}
```

**Expected Outcome**:
- All state transitions occur in correct order
- No states are skipped
- Context is cleaned up on completion
- Metrics reflect successful interaction

---

#### Test 1.2: Gossip Navigation - Multi-Step Interaction
```cpp
TEST(InteractionManager_StateMachine, GossipNavigation_MultipleMenusBeforeVendor)
{
    // SETUP: Vendor requires gossip path: Menu1->Option0->Menu2->Option1->Vendor
    Player* bot = CreateTestBot();
    Creature* vendor = CreateTestVendorWithGossip();
    InteractionManager* mgr = InteractionManager::Instance();

    // EXECUTE
    mgr->StartInteraction(bot, vendor, InteractionType::Vendor);
    InteractionContext* ctx = mgr->GetInteractionContext(bot);

    // VERIFY: Gossip path detected
    ASSERT_TRUE(ctx->needsGossip);
    ASSERT_EQ(ctx->gossipPath.size(), 2);  // Two menu selections required

    // SIMULATE: Approaching -> Initiating -> WaitingGossip
    SimulateStateTransition(bot, InteractionState::WaitingGossip);

    // SIMULATE: Server sends gossip menu 1
    WorldPacket gossipPacket1 = CreateGossipMenuPacket(MENU_ID_1);
    mgr->HandleGossipMessage(bot, gossipPacket1);

    // VERIFY: WaitingGossip -> ProcessingMenu
    ASSERT_EQ(ctx->state, InteractionState::ProcessingMenu);

    // SIMULATE: Process menu selects option 0
    mgr->ProcessInteractionState(bot, 100);

    // VERIFY: Gossip path consumed first option
    ASSERT_EQ(ctx->gossipPath.size(), 1);  // One option remaining
    ASSERT_EQ(ctx->state, InteractionState::WaitingGossip);  // Back to waiting for menu 2

    // SIMULATE: Server sends gossip menu 2
    WorldPacket gossipPacket2 = CreateGossipMenuPacket(MENU_ID_2);
    mgr->HandleGossipMessage(bot, gossipPacket2);

    // SIMULATE: Process menu selects option 1 (final)
    mgr->ProcessInteractionState(bot, 100);

    // VERIFY: Gossip path fully consumed, ready for vendor
    ASSERT_EQ(ctx->gossipPath.size(), 0);
    ASSERT_EQ(ctx->state, InteractionState::ExecutingAction);
}
```

**Expected Outcome**:
- Gossip path is correctly navigated
- Each menu selection transitions to WaitingGossip for next menu
- Final option leads to ExecutingAction
- No gossip options are skipped or duplicated

---

#### Test 1.3: Timeout Handling - Interaction Expires
```cpp
TEST(InteractionManager_StateMachine, Timeout_InteractionExpires)
{
    // SETUP
    Player* bot = CreateTestBot();
    Creature* vendor = CreateTestVendor();
    InteractionManager* mgr = InteractionManager::Instance();

    mgr->StartInteraction(bot, vendor, InteractionType::Vendor);
    InteractionContext* ctx = mgr->GetInteractionContext(bot);

    // CONFIGURE: Very short timeout
    ctx->timeout = std::chrono::milliseconds(100);

    // SIMULATE: Time passes beyond timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // EXECUTE: Update processes timeout
    mgr->ProcessInteractionState(bot, 150);

    // VERIFY: Interaction timed out and was removed
    ASSERT_FALSE(mgr->HasActiveInteraction(bot));

    // METRICS: Timeout recorded
    InteractionMetrics metrics = mgr->GetMetrics(InteractionType::Vendor);
    ASSERT_EQ(metrics.timeoutCount, 1);
    ASSERT_EQ(metrics.failureCount, 1);
}
```

**Expected Outcome**:
- Timeout is detected by `CheckTimeout()`
- Interaction is completed with failure
- Context is cleaned up
- Metrics reflect timeout

---

#### Test 1.4: Retry Logic - Transient Failure Recovery
```cpp
TEST(InteractionManager_StateMachine, RetryLogic_RecoverFromTransientFailure)
{
    // SETUP
    Player* bot = CreateTestBot();
    Creature* vendor = CreateTestVendor();
    InteractionManager* mgr = InteractionManager::Instance();

    mgr->StartInteraction(bot, vendor, InteractionType::Vendor);
    InteractionContext* ctx = mgr->GetInteractionContext(bot);

    // SIMULATE: First attempt fails (too far away)
    SimulateStateTransition(bot, InteractionState::ExecutingAction);
    mgr->HandleInteractionError(bot, InteractionResult::TooFarAway);

    // VERIFY: Retry initiated
    ASSERT_EQ(ctx->attemptCount, 1);
    ASSERT_EQ(ctx->state, InteractionState::Approaching);  // Reset to beginning
    ASSERT_TRUE(mgr->HasActiveInteraction(bot));  // Not canceled

    // SIMULATE: Second attempt succeeds
    SimulateSuccessfulInteraction(bot, vendor);

    // VERIFY: Interaction completed successfully after retry
    ASSERT_FALSE(mgr->HasActiveInteraction(bot));

    // METRICS: Success after retry
    InteractionMetrics metrics = mgr->GetMetrics(InteractionType::Vendor);
    ASSERT_EQ(metrics.successCount, 1);
    ASSERT_EQ(metrics.totalAttempts, 1);  // Retries don't count as separate attempts
}
```

**Expected Outcome**:
- First failure triggers retry logic
- State machine resets to Approaching
- Second attempt succeeds
- Final result is success

---

#### Test 1.5: Max Retry Exhaustion - Permanent Failure
```cpp
TEST(InteractionManager_StateMachine, MaxRetries_PermanentFailure)
{
    // SETUP
    Player* bot = CreateTestBot();
    Creature* vendor = CreateTestVendor();
    InteractionManager* mgr = InteractionManager::Instance();

    mgr->StartInteraction(bot, vendor, InteractionType::Vendor);
    InteractionContext* ctx = mgr->GetInteractionContext(bot);
    ctx->maxAttempts = 3;  // Allow 3 retries

    // SIMULATE: Fail 3 times
    for (int i = 0; i < 3; ++i)
    {
        SimulateStateTransition(bot, InteractionState::ExecutingAction);
        mgr->HandleInteractionError(bot, InteractionResult::InvalidTarget);
        ASSERT_EQ(ctx->attemptCount, i + 1);
    }

    // VERIFY: After 3rd failure, no more retries
    ASSERT_FALSE(mgr->HasActiveInteraction(bot));

    // METRICS: Failed after exhausting retries
    InteractionMetrics metrics = mgr->GetMetrics(InteractionType::Vendor);
    ASSERT_EQ(metrics.failureCount, 1);
    ASSERT_EQ(metrics.successCount, 0);
}
```

**Expected Outcome**:
- Retry logic attempts up to maxAttempts
- After final attempt, interaction fails permanently
- Context is cleaned up
- Metrics reflect permanent failure

---

### 2. Asynchronous Queue Integration Tests

#### Test 2.1: Queue Processing - FIFO with Priority
```cpp
TEST(InteractionManager_Queue, QueueProcessing_PriorityOrder)
{
    // SETUP
    Player* bot = CreateTestBot();
    Creature* vendor1 = CreateTestVendor();
    Creature* vendor2 = CreateTestVendor();
    Creature* vendor3 = CreateTestVendor();
    InteractionManager* mgr = InteractionManager::Instance();

    // QUEUE: Three interactions with different priorities
    InteractionRequest req1;
    req1.botGuid = bot->GetGUID();
    req1.targetGuid = vendor1->GetGUID();
    req1.type = InteractionType::Vendor;
    req1.priority = 10;  // Low priority

    InteractionRequest req2;
    req2.botGuid = bot->GetGUID();
    req2.targetGuid = vendor2->GetGUID();
    req2.type = InteractionType::Vendor;
    req2.priority = 100;  // High priority

    InteractionRequest req3;
    req3.botGuid = bot->GetGUID();
    req3.targetGuid = vendor3->GetGUID();
    req3.type = InteractionType::Vendor;
    req3.priority = 50;  // Medium priority

    mgr->QueueInteraction(bot, req1);
    mgr->QueueInteraction(bot, req2);
    mgr->QueueInteraction(bot, req3);

    // VERIFY: Queue size
    ASSERT_EQ(mgr->GetQueuedInteractions(), 3);

    // EXECUTE: Process queue (maxConcurrentInteractions = 1)
    mgr->Update(0);

    // VERIFY: Highest priority processed first
    InteractionContext* ctx = mgr->GetInteractionContext(bot);
    ASSERT_EQ(ctx->targetGuid, vendor2->GetGUID());  // req2 (priority 100)

    // COMPLETE: First interaction
    SimulateSuccessfulInteraction(bot, vendor2);
    mgr->Update(0);

    // VERIFY: Medium priority processed second
    ctx = mgr->GetInteractionContext(bot);
    ASSERT_EQ(ctx->targetGuid, vendor3->GetGUID());  // req3 (priority 50)

    // COMPLETE: Second interaction
    SimulateSuccessfulInteraction(bot, vendor3);
    mgr->Update(0);

    // VERIFY: Low priority processed last
    ctx = mgr->GetInteractionContext(bot);
    ASSERT_EQ(ctx->targetGuid, vendor1->GetGUID());  // req1 (priority 10)
}
```

**Expected Outcome**:
- Queue processes highest priority first
- Concurrent interaction limit is respected
- All queued interactions eventually process

---

#### Test 2.2: Rate Limiting - MIN_INTERACTION_DELAY Enforced
```cpp
TEST(InteractionManager_Queue, RateLimiting_MinimumDelay)
{
    // SETUP
    Player* bot = CreateTestBot();
    Creature* vendor1 = CreateTestVendor();
    Creature* vendor2 = CreateTestVendor();
    InteractionManager* mgr = InteractionManager::Instance();

    // EXECUTE: Start first interaction
    auto start = std::chrono::steady_clock::now();
    mgr->StartInteraction(bot, vendor1, InteractionType::Vendor);
    SimulateSuccessfulInteraction(bot, vendor1);

    // QUEUE: Second interaction immediately after first
    InteractionRequest req;
    req.botGuid = bot->GetGUID();
    req.targetGuid = vendor2->GetGUID();
    req.type = InteractionType::Vendor;
    mgr->QueueInteraction(bot, req);

    // EXECUTE: Update immediately (should NOT process due to rate limit)
    mgr->Update(0);

    // VERIFY: Second interaction not started yet
    ASSERT_EQ(mgr->GetQueuedInteractions(), 1);
    ASSERT_FALSE(mgr->HasActiveInteraction(bot));

    // SIMULATE: Wait for MIN_INTERACTION_DELAY (100ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(MIN_INTERACTION_DELAY + 10));

    // EXECUTE: Update after delay
    mgr->Update(MIN_INTERACTION_DELAY + 10);

    // VERIFY: Second interaction now started
    ASSERT_EQ(mgr->GetQueuedInteractions(), 0);
    ASSERT_TRUE(mgr->HasActiveInteraction(bot));

    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // VERIFY: At least MIN_INTERACTION_DELAY passed
    ASSERT_GE(elapsed.count(), MIN_INTERACTION_DELAY);
}
```

**Expected Outcome**:
- Rate limiting prevents interactions within MIN_INTERACTION_DELAY
- Queue holds interaction until delay passes
- Delay is enforced per bot, not globally

---

#### Test 2.3: Concurrent Interaction Limit
```cpp
TEST(InteractionManager_Queue, ConcurrentLimit_MaximumActive)
{
    // SETUP: 10 bots, maxConcurrentInteractions = 3
    std::vector<Player*> bots;
    std::vector<Creature*> vendors;
    InteractionManager* mgr = InteractionManager::Instance();
    mgr->m_config.maxConcurrentInteractions = 3;

    for (int i = 0; i < 10; ++i)
    {
        bots.push_back(CreateTestBot());
        vendors.push_back(CreateTestVendor());
    }

    // QUEUE: All 10 bots request interactions
    for (int i = 0; i < 10; ++i)
    {
        InteractionRequest req;
        req.botGuid = bots[i]->GetGUID();
        req.targetGuid = vendors[i]->GetGUID();
        req.type = InteractionType::Vendor;
        mgr->QueueInteraction(bots[i], req);
    }

    // VERIFY: Queue has 10 requests
    ASSERT_EQ(mgr->GetQueuedInteractions(), 10);

    // EXECUTE: Update processes queue
    mgr->Update(0);

    // VERIFY: Only 3 interactions active (concurrent limit)
    ASSERT_EQ(mgr->GetActiveInteractions(), 3);
    ASSERT_EQ(mgr->GetQueuedInteractions(), 7);  // 7 still queued

    // COMPLETE: 2 interactions
    SimulateSuccessfulInteraction(bots[0], vendors[0]);
    SimulateSuccessfulInteraction(bots[1], vendors[1]);

    // EXECUTE: Update processes more from queue
    mgr->Update(0);

    // VERIFY: 3 active again (2 completed, 2 new started)
    ASSERT_EQ(mgr->GetActiveInteractions(), 3);
    ASSERT_EQ(mgr->GetQueuedInteractions(), 5);
}
```

**Expected Outcome**:
- Concurrent interaction limit is enforced
- Queue processes additional interactions as slots open
- No deadlocks or starvation

---

### 3. Handler Routing Integration Tests

#### Test 3.1: VendorInteraction Handler - BuyItem
```cpp
TEST(InteractionManager_Handlers, VendorHandler_BuyItem)
{
    // SETUP
    Player* bot = CreateTestBot();
    bot->SetMoney(1000000);  // 100 gold
    Creature* vendor = CreateTestVendor();
    AddItemToVendor(vendor, ITEM_HEALTH_POTION, 10);  // 10 silver each
    InteractionManager* mgr = InteractionManager::Instance();

    // EXECUTE
    InteractionResult result = mgr->BuyItem(bot, vendor, ITEM_HEALTH_POTION, 5);

    // PROCESS: State machine to completion
    while (mgr->HasActiveInteraction(bot))
    {
        mgr->ProcessInteractionState(bot, 100);
        mgr->Update(100);
    }

    // VERIFY: Item purchased
    ASSERT_EQ(result, InteractionResult::Success);
    ASSERT_TRUE(bot->HasItemCount(ITEM_HEALTH_POTION, 5));
    ASSERT_EQ(bot->GetMoney(), 1000000 - (10 * 5));  // 50 silver spent

    // METRICS
    InteractionMetrics metrics = mgr->GetMetrics(InteractionType::Vendor);
    ASSERT_EQ(metrics.successCount, 1);
}
```

**Expected Outcome**:
- VendorInteraction handler processes purchase
- Item is added to bot inventory
- Money is deducted correctly
- Metrics reflect success

---

#### Test 3.2: TrainerInteraction Handler - LearnSpell
```cpp
TEST(InteractionManager_Handlers, TrainerHandler_LearnSpell)
{
    // SETUP
    Player* bot = CreateTestBot();
    bot->SetLevel(10);
    bot->SetMoney(1000000);
    Creature* trainer = CreateTestTrainer();
    AddSpellToTrainer(trainer, SPELL_FIREBALL_RANK_2, 50);  // 50 silver cost, requires level 10
    InteractionManager* mgr = InteractionManager::Instance();

    // EXECUTE
    InteractionResult result = mgr->LearnOptimalSpells(bot, trainer);

    // PROCESS: State machine to completion
    while (mgr->HasActiveInteraction(bot))
    {
        mgr->ProcessInteractionState(bot, 100);
        mgr->Update(100);
    }

    // VERIFY: Spell learned
    ASSERT_EQ(result, InteractionResult::Success);
    ASSERT_TRUE(bot->HasSpell(SPELL_FIREBALL_RANK_2));
    ASSERT_EQ(bot->GetMoney(), 1000000 - 50);

    // METRICS
    InteractionMetrics metrics = mgr->GetMetrics(InteractionType::Trainer);
    ASSERT_EQ(metrics.successCount, 1);
}
```

**Expected Outcome**:
- TrainerInteraction handler processes learning
- Spell is added to bot spellbook
- Training cost is deducted
- Metrics reflect success

---

### 4. Performance and Scalability Tests

#### Test 4.1: 500-Bot Concurrent Interactions
```cpp
TEST(InteractionManager_Performance, FiveHundredBots_ConcurrentLoad)
{
    // SETUP
    InteractionManager* mgr = InteractionManager::Instance();
    mgr->m_config.maxConcurrentInteractions = 100;

    std::vector<Player*> bots;
    std::vector<Creature*> vendors;

    for (int i = 0; i < 500; ++i)
    {
        bots.push_back(CreateTestBot());
        vendors.push_back(CreateTestVendor());
    }

    // MEASURE: Start time
    auto startTime = std::chrono::high_resolution_clock::now();
    uint64_t startCpu = GetProcessCpuTime();

    // EXECUTE: All 500 bots start interactions
    for (int i = 0; i < 500; ++i)
    {
        mgr->StartInteraction(bots[i], vendors[i], InteractionType::Vendor);
    }

    // PROCESS: All interactions to completion
    int updateCount = 0;
    while (mgr->GetActiveInteractions() > 0)
    {
        mgr->Update(100);
        for (auto bot : bots)
            mgr->ProcessInteractionState(bot, 100);
        ++updateCount;

        // SAFETY: Prevent infinite loop
        ASSERT_LT(updateCount, 10000);
    }

    // MEASURE: End time
    auto endTime = std::chrono::high_resolution_clock::now();
    uint64_t endCpu = GetProcessCpuTime();

    auto wallTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    uint64_t cpuTime = endCpu - startCpu;
    float cpuPercent = (float)cpuTime / wallTime.count() / GetCpuCoreCount() * 100.0f;

    // VERIFY: Performance targets
    ASSERT_LT(wallTime.count(), 60000);  // All 500 complete in < 60 seconds
    ASSERT_LT(cpuPercent, 50.0f);        // CPU usage < 50% (target < 50% for 500 bots)

    // METRICS
    InteractionMetrics metrics = mgr->GetMetrics(InteractionType::Vendor);
    ASSERT_EQ(metrics.totalAttempts, 500);
    ASSERT_GT(metrics.successRate, 95.0f);  // > 95% success
    ASSERT_LT(metrics.avgDuration.count(), 200);  // < 200ms average

    // LOG RESULTS
    TC_LOG_INFO("test", "500-bot test completed:");
    TC_LOG_INFO("test", "  Wall time: {}ms", wallTime.count());
    TC_LOG_INFO("test", "  CPU usage: {:.2f}%", cpuPercent);
    TC_LOG_INFO("test", "  Success rate: {:.2f}%", metrics.successRate);
    TC_LOG_INFO("test", "  Avg duration: {}ms", metrics.avgDuration.count());
}
```

**Expected Outcome**:
- All 500 interactions complete successfully
- Wall time < 60 seconds
- CPU usage < 50%
- Success rate > 95%
- Average interaction time < 200ms

---

#### Test 4.2: Memory Leak Detection - Long-Running Stress
```cpp
TEST(InteractionManager_Performance, MemoryLeak_LongRunning)
{
    // SETUP
    InteractionManager* mgr = InteractionManager::Instance();
    Player* bot = CreateTestBot();
    Creature* vendor = CreateTestVendor();

    // MEASURE: Initial memory
    uint64_t initialMemory = GetProcessMemoryUsage();

    // EXECUTE: 10,000 interactions in a loop
    for (int i = 0; i < 10000; ++i)
    {
        mgr->StartInteraction(bot, vendor, InteractionType::Vendor);

        while (mgr->HasActiveInteraction(bot))
        {
            mgr->ProcessInteractionState(bot, 100);
            mgr->Update(100);
        }

        // VERIFY: No context leak
        ASSERT_FALSE(mgr->HasActiveInteraction(bot));
        ASSERT_EQ(mgr->GetActiveInteractions(), 0);
    }

    // MEASURE: Final memory
    uint64_t finalMemory = GetProcessMemoryUsage();
    uint64_t memoryGrowth = finalMemory - initialMemory;

    // VERIFY: Memory growth < 10MB (should be near zero for no leaks)
    ASSERT_LT(memoryGrowth, 10 * 1024 * 1024);

    TC_LOG_INFO("test", "Memory growth after 10,000 interactions: {} KB", memoryGrowth / 1024);
}
```

**Expected Outcome**:
- No memory leaks after 10,000 interactions
- Memory growth < 10 MB
- All contexts properly cleaned up

---

#### Test 4.3: Thread Safety - Concurrent Access
```cpp
TEST(InteractionManager_Performance, ThreadSafety_ConcurrentAccess)
{
    // SETUP
    InteractionManager* mgr = InteractionManager::Instance();
    std::vector<Player*> bots;
    std::vector<Creature*> vendors;

    for (int i = 0; i < 100; ++i)
    {
        bots.push_back(CreateTestBot());
        vendors.push_back(CreateTestVendor());
    }

    // EXECUTE: Multiple threads starting interactions concurrently
    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};
    std::atomic<int> failureCount{0};

    for (int i = 0; i < 100; ++i)
    {
        threads.emplace_back([&, i]()
        {
            InteractionResult result = mgr->StartInteraction(bots[i], vendors[i], InteractionType::Vendor);
            if (result == InteractionResult::Pending)
                ++successCount;
            else
                ++failureCount;

            // Process to completion
            while (mgr->HasActiveInteraction(bots[i]))
            {
                mgr->ProcessInteractionState(bots[i], 100);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }

    // WAIT: All threads complete
    for (auto& thread : threads)
        thread.join();

    // VERIFY: No race conditions, all interactions processed
    ASSERT_EQ(successCount.load() + failureCount.load(), 100);
    ASSERT_EQ(mgr->GetActiveInteractions(), 0);  // All completed

    // VERIFY: No crashes, no deadlocks
    ASSERT_GT(successCount.load(), 90);  // At least 90% succeeded
}
```

**Expected Outcome**:
- No race conditions or crashes
- No deadlocks
- All interactions eventually complete
- Shared mutex protects concurrent access

---

### 5. TrinityCore API Compliance Tests

#### Test 5.1: Player API Usage - Movement and Facing
```cpp
TEST(InteractionManager_TrinityCore, PlayerAPI_MovementAndFacing)
{
    // SETUP
    Player* bot = CreateTestBot(Position(0, 0, 0));
    Creature* vendor = CreateTestVendor(Position(10, 0, 0));  // 10 yards away
    InteractionManager* mgr = InteractionManager::Instance();

    // EXECUTE: Start interaction (requires movement)
    mgr->StartInteraction(bot, vendor, InteractionType::Vendor);
    InteractionContext* ctx = mgr->GetInteractionContext(bot);

    // VERIFY: State is Approaching
    ASSERT_EQ(ctx->state, InteractionState::Approaching);

    // VERIFY: TrinityCore movement API called
    ASSERT_TRUE(bot->GetMotionMaster()->HasMovementType(POINT_MOTION_TYPE));

    // SIMULATE: Bot reaches vendor
    bot->SetPosition(9, 0, 0, 0);  // 9 yards away, still out of range
    mgr->ProcessInteractionState(bot, 100);

    // VERIFY: Still approaching
    ASSERT_EQ(ctx->state, InteractionState::Approaching);

    // SIMULATE: Bot within range
    bot->SetPosition(4, 0, 0, 0);  // 4 yards away, in range
    mgr->ProcessInteractionState(bot, 100);

    // VERIFY: Transitioned to Initiating, facing set
    ASSERT_EQ(ctx->state, InteractionState::Initiating);
    ASSERT_FLOAT_EQ(bot->GetOrientation(), bot->GetAngle(vendor), 0.1f);  // Facing vendor
    ASSERT_EQ(bot->GetSelection(), vendor->GetGUID());  // Selection set
}
```

**Expected Outcome**:
- TrinityCore movement APIs used correctly
- Facing and selection APIs work as expected
- No crashes or undefined behavior

---

#### Test 5.2: Creature API Usage - NPC Flags and Gossip
```cpp
TEST(InteractionManager_TrinityCore, CreatureAPI_NPCFlagsAndGossip)
{
    // SETUP
    Creature* vendor = CreateTestCreature();
    vendor->SetNpcFlags(UNIT_NPC_FLAG_VENDOR | UNIT_NPC_FLAG_REPAIR);
    InteractionManager* mgr = InteractionManager::Instance();

    // EXECUTE: Detect NPC type
    InteractionType type = mgr->DetectNPCType(vendor);

    // VERIFY: Correctly detected as vendor
    ASSERT_EQ(type, InteractionType::Vendor);

    // VERIFY: NPC type cached for performance
    InteractionType cachedType = mgr->DetectNPCType(vendor);
    ASSERT_EQ(cachedType, InteractionType::Vendor);

    // SETUP: Add trainer flag
    vendor->SetNpcFlags(UNIT_NPC_FLAG_VENDOR | UNIT_NPC_FLAG_TRAINER);

    // CLEAR CACHE: Force re-detection
    mgr->m_npcTypeCache.clear();

    // EXECUTE: Detect again
    InteractionType newType = mgr->DetectNPCType(vendor);

    // VERIFY: Trainer takes priority over vendor
    ASSERT_EQ(newType, InteractionType::Trainer);
}
```

**Expected Outcome**:
- NPC flags are read correctly via TrinityCore API
- Flag priority is correct
- Caching works properly

---

#### Test 5.3: ObjectAccessor API - Guid Resolution
```cpp
TEST(InteractionManager_TrinityCore, ObjectAccessorAPI_GuidResolution)
{
    // SETUP
    Player* bot = CreateTestBot();
    Creature* vendor = CreateTestVendor();
    InteractionManager* mgr = InteractionManager::Instance();

    // EXECUTE: Start interaction
    mgr->StartInteraction(bot, vendor, InteractionType::Vendor);

    // VERIFY: Interaction context stores GUIDs
    InteractionContext* ctx = mgr->GetInteractionContext(bot);
    ASSERT_EQ(ctx->botGuid, bot->GetGUID());
    ASSERT_EQ(ctx->targetGuid, vendor->GetGUID());

    // SIMULATE: Vendor despawns (removed from world)
    RemoveCreatureFromWorld(vendor);

    // EXECUTE: Update tries to resolve GUID
    mgr->Update(100);

    // VERIFY: Interaction canceled due to invalid target
    ASSERT_FALSE(mgr->HasActiveInteraction(bot));

    // METRICS: Failure due to invalid target
    InteractionMetrics metrics = mgr->GetMetrics(InteractionType::Vendor);
    ASSERT_GT(metrics.failureCount, 0);
}
```

**Expected Outcome**:
- ObjectAccessor correctly resolves GUIDs
- Invalid GUIDs are handled gracefully
- Interaction fails safely when target despawns

---

### 6. Edge Case and Error Handling Tests

#### Test 6.1: Bot Logs Out During Interaction
```cpp
TEST(InteractionManager_EdgeCases, BotLogout_DuringInteraction)
{
    // SETUP
    Player* bot = CreateTestBot();
    Creature* vendor = CreateTestVendor();
    InteractionManager* mgr = InteractionManager::Instance();

    // EXECUTE: Start interaction
    mgr->StartInteraction(bot, vendor, InteractionType::Vendor);
    ASSERT_TRUE(mgr->HasActiveInteraction(bot));

    // SIMULATE: Bot logs out
    LogoutPlayer(bot);

    // EXECUTE: Update processes logout
    mgr->Update(100);

    // VERIFY: Interaction cleaned up (bot GUID no longer resolves)
    ASSERT_FALSE(mgr->HasActiveInteraction(bot));
}
```

**Expected Outcome**:
- Interaction is cleaned up when bot logs out
- No dangling contexts
- No crashes

---

#### Test 6.2: Vendor Moves Out of Range During Interaction
```cpp
TEST(InteractionManager_EdgeCases, VendorMoves_OutOfRange)
{
    // SETUP
    Player* bot = CreateTestBot(Position(0, 0, 0));
    Creature* vendor = CreateTestVendor(Position(4, 0, 0));  // In range
    InteractionManager* mgr = InteractionManager::Instance();

    // EXECUTE: Start interaction
    mgr->StartInteraction(bot, vendor, InteractionType::Vendor);

    // SIMULATE: Advance to ExecutingAction state
    SimulateStateTransition(bot, InteractionState::ExecutingAction);

    // SIMULATE: Vendor moves far away
    vendor->SetPosition(100, 0, 0, 0);  // Out of range

    // EXECUTE: Update detects out of range
    mgr->ProcessInteractionState(bot, 100);

    // VERIFY: Interaction fails or retries with movement
    InteractionContext* ctx = mgr->GetInteractionContext(bot);
    // Either failed or moved back to Approaching for retry
    ASSERT_TRUE(ctx->state == InteractionState::Failed ||
                ctx->state == InteractionState::Approaching);
}
```

**Expected Outcome**:
- Out-of-range condition is detected
- Interaction fails or retries
- No infinite loops

---

## Test Execution Strategy

### Phase 1: Unit Tests (Isolated Components)
- State machine state transitions
- Context lifecycle
- Metrics calculation
- Handler routing logic

### Phase 2: Integration Tests (Component Interactions)
- State machine + queue
- State machine + handlers
- Queue + rate limiting
- Metrics + performance

### Phase 3: Performance Tests (Scalability)
- 100-bot concurrent load
- 500-bot concurrent load
- Memory leak detection
- Thread safety validation

### Phase 4: TrinityCore Compliance Tests
- All TrinityCore API usage
- Guid resolution
- Packet handling
- Object lifecycle

### Phase 5: Production Validation
- Run with real bots in test environment
- Monitor metrics for 24 hours
- Identify performance bottlenecks
- Tune configuration based on results

---

## Success Criteria

### Functionality
- [ ] All state transitions work correctly
- [ ] Gossip navigation handles multi-step paths
- [ ] Retry logic recovers from transient failures
- [ ] Timeout handling prevents infinite loops
- [ ] Queue processes by priority
- [ ] Rate limiting enforced correctly
- [ ] All handlers route properly

### Performance
- [ ] 500 bots complete interactions in < 60 seconds
- [ ] CPU usage < 50% for 500 bots
- [ ] Average interaction time < 200ms
- [ ] Success rate > 95%
- [ ] Memory growth < 10 MB over 10,000 interactions
- [ ] No crashes or deadlocks under concurrent load

### TrinityCore Compliance
- [ ] All TrinityCore APIs used correctly
- [ ] No core file modifications
- [ ] No memory leaks
- [ ] Thread-safe concurrent access
- [ ] Proper GUID resolution
- [ ] Graceful handling of object despawns

---

## Continuous Integration

### Automated Test Runs
- **Daily**: Full test suite on dev branch
- **Pre-Commit**: State machine and handler tests
- **Pre-Release**: Performance and scalability tests

### Metrics Dashboard
- Success rate trend over time
- Average interaction duration trend
- CPU/memory usage trends
- Failure type distribution

---

**Document Status**: Complete
**Implementation Priority**: High
**Next Steps**: Implement test framework, run Phase 1 tests, iterate based on results
