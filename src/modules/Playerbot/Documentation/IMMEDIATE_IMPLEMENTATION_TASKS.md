# Immediate Implementation Tasks - Agent Coordination
## Priority: Group Functionality for Testing

### CRITICAL PATH - Day 1 Implementation (NOW)

## Task 1: Group Invitation Handler [HIGHEST PRIORITY]
**Primary Agent**: wow-bot-behavior-designer
**Support Agent**: trinity-integration-tester
**Time Allocation**: 4 hours

### Implementation Requirements
```cpp
// File: src/modules/Playerbot/Group/GroupInvitationHandler.h
class GroupInvitationHandler {
public:
    GroupInvitationHandler(BotSession* session);

    // Core functionality - MUST IMPLEMENT
    void HandleGroupInvite(ObjectGuid inviterGuid);
    bool AutoAcceptInvitation(Player* inviter);
    void OnGroupJoined();

    // Configuration
    void SetAutoAcceptEnabled(bool enabled) { m_autoAccept = enabled; }
    void AddTrustedInviter(ObjectGuid guid);

private:
    bool ValidateInviter(Player* inviter);
    void SendAcceptPacket();

    BotSession* m_session;
    bool m_autoAccept = true;
    std::set<ObjectGuid> m_trustedInviters;
};
```

### Specific Agent Instructions

#### wow-bot-behavior-designer:
1. Create behavior node for invitation handling
2. Implement state machine for group membership
3. Define decision logic for accepting/rejecting
4. Add group event callbacks

#### trinity-integration-tester:
1. Verify opcode CMSG_GROUP_INVITE_RESPONSE
2. Test WorldSession integration points
3. Validate group roster synchronization
4. Check for memory leaks in group operations

### Integration Points
```cpp
// Hook into existing WorldSession
void BotWorldSession::HandleGroupInviteOpcode(WorldPacket& recv_data) {
    // Extract inviter GUID
    ObjectGuid inviterGuid;
    recv_data >> inviterGuid;

    // Pass to bot handler
    if (m_bot && m_bot->GetGroupInvitationHandler()) {
        m_bot->GetGroupInvitationHandler()->HandleGroupInvite(inviterGuid);
    }
}
```

---

## Task 2: Leader Following System [HIGHEST PRIORITY]
**Primary Agent**: wow-bot-behavior-designer
**Support Agent**: cpp-architecture-optimizer
**Time Allocation**: 4 hours (parallel with Task 1)

### Implementation Requirements
```cpp
// File: src/modules/Playerbot/Movement/LeaderFollowBehavior.h
class LeaderFollowBehavior {
public:
    LeaderFollowBehavior(Bot* bot);

    // Core functionality - MUST IMPLEMENT
    void Update(uint32 diff);
    void SetFollowTarget(Unit* target);
    Position CalculateFollowPosition();

    // Formation support
    void SetFormationType(FormationType type);
    void SetFollowDistance(float distance) { m_followDistance = distance; }

private:
    void UpdateMovement();
    bool NeedsRepositioning();
    void HandleObstacles();

    Bot* m_bot;
    Unit* m_followTarget = nullptr;
    float m_followDistance = 5.0f;
    FormationType m_formation = FORMATION_NONE;
    uint32 m_updateTimer = 0;
};
```

### Specific Agent Instructions

#### wow-bot-behavior-designer:
1. Create follow behavior tree node
2. Implement formation position calculations
3. Add obstacle avoidance logic
4. Define follow state transitions

#### cpp-architecture-optimizer:
1. Optimize position calculations for 5000 bots
2. Design efficient update loop
3. Minimize memory allocations
4. Create spatial indexing for collision detection

---

## Task 3: Combat Engagement Trigger [HIGH PRIORITY]
**Primary Agent**: wow-mechanics-expert
**Time Allocation**: 2 hours

### Implementation Requirements
```cpp
// File: src/modules/Playerbot/Combat/GroupCombatTrigger.h
class GroupCombatTrigger {
public:
    GroupCombatTrigger(Bot* bot);

    // Core functionality - MUST IMPLEMENT
    void CheckGroupCombatStatus();
    bool ShouldEngageCombat();
    void EngageTarget(Unit* target);

private:
    bool IsGroupInCombat();
    Unit* GetGroupTarget();

    Bot* m_bot;
    uint32 m_combatCheckTimer = 0;
};
```

### Specific Agent Instructions

#### wow-mechanics-expert:
1. Implement combat detection from group events
2. Define combat engagement rules
3. Handle threat initialization
4. Validate spell casting mechanics

---

## Task 4: Target Assistance System [HIGH PRIORITY]
**Primary Agent**: wow-mechanics-expert
**Support Agent**: trinity-integration-tester
**Time Allocation**: 2 hours

### Implementation Requirements
```cpp
// File: src/modules/Playerbot/Combat/TargetAssistance.h
class TargetAssistance {
public:
    TargetAssistance(Bot* bot);

    // Core functionality - MUST IMPLEMENT
    Unit* GetAssistTarget();
    void AssistGroupLeader();
    void UpdateTargetSelection();

private:
    Unit* GetLeaderTarget();
    bool IsValidTarget(Unit* target);

    Bot* m_bot;
    ObjectGuid m_currentTargetGuid;
};
```

---

## Task 5: Integration Testing Framework [CRITICAL]
**Primary Agent**: test-automation-engineer
**Time Allocation**: Continuous throughout Day 1

### Test Requirements
```cpp
// File: src/modules/Playerbot/Tests/GroupFunctionalityTests.cpp
class GroupFunctionalityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test environment
        m_testGroup = CreateTestGroup(5);
    }

    void TearDown() override {
        // Cleanup
        DestroyTestGroup(m_testGroup);
    }

    TestGroup* m_testGroup;
};

// MUST PASS TESTS
TEST_F(GroupFunctionalityTest, AutoAcceptInvitation) {
    auto bot = CreateTestBot();
    auto player = CreateTestPlayer();

    player->InviteToGroup(bot);

    // Bot should accept within 2 seconds
    EXPECT_TRUE(WaitForCondition([&]() {
        return bot->IsInGroup();
    }, 2000));
}

TEST_F(GroupFunctionalityTest, FollowLeader) {
    auto leader = m_testGroup->GetLeader();
    auto follower = m_testGroup->GetMember(1);

    leader->Move(100, 100, 0);

    // Follower should maintain distance
    EXPECT_TRUE(WaitForCondition([&]() {
        float distance = follower->GetDistance(leader);
        return distance >= 5.0f && distance <= 10.0f;
    }, 5000));
}

TEST_F(GroupFunctionalityTest, AssistTarget) {
    auto leader = m_testGroup->GetLeader();
    auto member = m_testGroup->GetMember(1);
    auto enemy = SpawnTestEnemy();

    leader->SetTarget(enemy);

    // Member should target same enemy
    EXPECT_TRUE(WaitForCondition([&]() {
        return member->GetTarget() == enemy;
    }, 1000));
}

TEST_F(GroupFunctionalityTest, EngageCombat) {
    auto enemy = SpawnTestEnemy();
    m_testGroup->GetLeader()->Attack(enemy);

    // All members should enter combat
    EXPECT_TRUE(WaitForCondition([&]() {
        return m_testGroup->AllInCombat();
    }, 2000));
}
```

---

## Coordination Timeline - Day 1

### Hour 1-2: Setup and Initial Implementation
- **wow-bot-behavior-designer**: Start GroupInvitationHandler and LeaderFollowBehavior
- **trinity-integration-tester**: Set up test environment, verify opcodes
- **cpp-architecture-optimizer**: Design optimal data structures
- **test-automation-engineer**: Create test framework

### Hour 3-4: Core Implementation
- **wow-bot-behavior-designer**: Complete invitation logic, continue follow behavior
- **wow-mechanics-expert**: Start combat engagement system
- **trinity-integration-tester**: Test invitation packets
- **test-automation-engineer**: Write first test cases

### Hour 5-6: Integration and Combat
- **wow-mechanics-expert**: Complete combat trigger and target assistance
- **wow-bot-behavior-designer**: Finalize follow behavior
- **trinity-integration-tester**: Validate all integrations
- **cpp-architecture-optimizer**: Optimize critical paths

### Hour 7-8: Testing and Validation
- **ALL AGENTS**: Run integration tests
- **test-automation-engineer**: Execute full test suite
- **trinity-integration-tester**: Final validation
- **wow-bot-behavior-designer**: Fix any behavior issues

---

## Critical Success Checkpoints

### End of Hour 2
- [ ] Group invitation handler skeleton complete
- [ ] Test environment operational
- [ ] Basic follow behavior drafted

### End of Hour 4
- [ ] Bots can accept invitations
- [ ] Basic following works
- [ ] Combat detection implemented

### End of Hour 6
- [ ] All four core features implemented
- [ ] Integration points connected
- [ ] Basic tests passing

### End of Hour 8
- [ ] All tests passing
- [ ] 5-bot group functional
- [ ] Ready for live testing

---

## Emergency Fallback Plan

If any critical component fails:

1. **Invitation Fails**: Use manual `/bot join` command
2. **Following Fails**: Use teleport to leader
3. **Combat Fails**: Use aggressive stance mode
4. **Assist Fails**: Use nearest target mode

---

## Communication Protocol

### Slack Channels (Simulated via Comments)
- `#phase1-critical`: Blocking issues only
- `#phase1-dev`: Development updates
- `#phase1-test`: Test results

### Status Updates
Every 2 hours, each agent reports:
1. Components completed
2. Current blockers
3. Next 2-hour goals
4. Help needed

### Blocker Resolution
1. Post in `#phase1-critical`
2. Tag responsible agent
3. Coordinator assigns helper if needed
4. Resolution within 30 minutes

---

## Next Steps After Day 1

Upon successful completion:
1. Run overnight stress test with 100 bots
2. Review performance metrics
3. Plan Day 2 optimizations
4. Begin Phase 2 preparation

---

## CRITICAL REMINDER

**THE GOAL**: Get bots into groups and following/fighting together TODAY.

**NOT THE GOAL**: Perfect implementation, all edge cases, full optimization.

**Focus on**: Making it work, then making it better.

**Success is**: A video of 5 bots in a group killing a mob together.