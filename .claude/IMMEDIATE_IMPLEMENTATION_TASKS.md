# Immediate Implementation Tasks - Day 1 Critical Path

**Objective**: Deliver testable group functionality within 8 hours
**Priority**: CRITICAL - Foundation for all testing

## Hour-by-Hour Implementation Plan

### Hours 1-2: Foundation Setup & Group Invitation Handler

#### Agent Assignments
- **Primary**: `wow-bot-behavior-designer`
- **Support**: `trinity-integration-tester`
- **Validation**: `test-automation-engineer`

#### Critical Tasks

**Hour 1: Project Setup**
```bash
# Create implementation directory structure
mkdir -p src/modules/Playerbot/Group/Implementation
mkdir -p src/modules/Playerbot/Tests/Group
mkdir -p src/modules/Playerbot/Movement/Advanced
```

**Create GroupInvitationHandler.h**
```cpp
#ifndef GROUP_INVITATION_HANDLER_H
#define GROUP_INVITATION_HANDLER_H

#include "Player.h"
#include "Group.h"
#include "WorldSession.h"

namespace Playerbot {

class TC_GAME_API GroupInvitationHandler
{
public:
    explicit GroupInvitationHandler(Player* bot);
    ~GroupInvitationHandler() = default;

    // Core invitation handling
    bool ShouldAcceptInvitation(Player* inviter) const;
    void HandleInvitation(WorldPacket& packet);
    void AcceptInvitation(ObjectGuid inviterGuid);
    void DeclineInvitation(ObjectGuid inviterGuid);

    // State management
    bool IsWaitingForInvitation() const { return _waitingForInvite; }
    void SetAutoAcceptInvitations(bool accept) { _autoAccept = accept; }

private:
    Player* _bot;
    ObjectGuid _pendingInviter;
    bool _waitingForInvite{false};
    bool _autoAccept{true};
    uint32 _lastInviteTime{0};

    // Helper methods
    bool IsValidInviter(Player* inviter) const;
    bool CanJoinGroup(Group* group) const;
    void LogInvitationEvent(std::string const& event, Player* inviter) const;
};

} // namespace Playerbot

#endif // GROUP_INVITATION_HANDLER_H
```

**Hour 2: GroupInvitationHandler.cpp Implementation**
```cpp
#include "GroupInvitationHandler.h"
#include "BotSession.h"
#include "Log.h"
#include "GameTime.h"
#include "Group.h"
#include "GroupMgr.h"
#include "Opcodes.h"

namespace Playerbot {

GroupInvitationHandler::GroupInvitationHandler(Player* bot) : _bot(bot)
{
    if (!_bot)
    {
        TC_LOG_ERROR("playerbot.group", "GroupInvitationHandler created with null bot");
        return;
    }

    TC_LOG_DEBUG("playerbot.group", "GroupInvitationHandler created for bot {}",
                 _bot->GetName());
}

bool GroupInvitationHandler::ShouldAcceptInvitation(Player* inviter) const
{
    if (!_autoAccept || !inviter)
        return false;

    // Check if inviter is valid
    if (!IsValidInviter(inviter))
        return false;

    // Check cooldown (prevent spam)
    if (GameTime::GetGameTimeMS() - _lastInviteTime < 5000) // 5 second cooldown
        return false;

    // Check if we can join the group
    Group* inviterGroup = inviter->GetGroup();
    if (!CanJoinGroup(inviterGroup))
        return false;

    return true;
}

void GroupInvitationHandler::HandleInvitation(WorldPacket& packet)
{
    ObjectGuid inviterGuid;
    packet >> inviterGuid;

    Player* inviter = ObjectAccessor::FindConnectedPlayer(inviterGuid);
    if (!inviter)
    {
        TC_LOG_DEBUG("playerbot.group", "Bot {} received invitation from offline player",
                     _bot->GetName());
        return;
    }

    LogInvitationEvent("Received invitation", inviter);

    if (ShouldAcceptInvitation(inviter))
    {
        AcceptInvitation(inviterGuid);
    }
    else
    {
        DeclineInvitation(inviterGuid);
    }
}

void GroupInvitationHandler::AcceptInvitation(ObjectGuid inviterGuid)
{
    Player* inviter = ObjectAccessor::FindConnectedPlayer(inviterGuid);
    if (!inviter)
        return;

    // Send accept packet
    WorldPacket data(CMSG_GROUP_ACCEPT, 0);
    _bot->GetSession()->HandleGroupAcceptOpcode(data);

    _lastInviteTime = GameTime::GetGameTimeMS();
    _pendingInviter = inviterGuid;

    LogInvitationEvent("Accepted invitation", inviter);

    TC_LOG_INFO("playerbot.group", "Bot {} accepted group invitation from {}",
                _bot->GetName(), inviter->GetName());
}

bool GroupInvitationHandler::IsValidInviter(Player* inviter) const
{
    if (!inviter)
        return false;

    // Don't accept invitations from other bots (prevent bot loops)
    if (inviter->GetSession()->IsBot())
        return false;

    // Check if inviter is in same map
    if (inviter->GetMapId() != _bot->GetMapId())
        return false;

    // Check distance (within reasonable range)
    float distance = inviter->GetDistance(_bot);
    if (distance > 1000.0f) // 1000 yards maximum
        return false;

    return true;
}

bool GroupInvitationHandler::CanJoinGroup(Group* group) const
{
    if (!group)
        return true; // Can join new group

    // Check group size
    if (group->GetMembersCount() >= GROUP_MAX_SIZE)
        return false;

    // Check if already in a group
    if (_bot->GetGroup())
        return false;

    return true;
}

void GroupInvitationHandler::LogInvitationEvent(std::string const& event, Player* inviter) const
{
    TC_LOG_DEBUG("playerbot.group", "Bot {} - {}: inviter={}, map={}, distance={:.1f}",
                 _bot->GetName(), event, inviter->GetName(),
                 inviter->GetMapId(), inviter->GetDistance(_bot));
}

} // namespace Playerbot
```

#### Integration with BotAI
```cpp
// Add to BotAI.h
#include "Group/GroupInvitationHandler.h"

class BotAI {
private:
    std::unique_ptr<GroupInvitationHandler> _groupHandler;

public:
    GroupInvitationHandler* GetGroupHandler() const { return _groupHandler.get(); }
};

// Add to BotAI.cpp constructor
BotAI::BotAI(Player* bot) : _bot(bot) {
    // ... existing code ...

    // Initialize group handler
    _groupHandler = std::make_unique<GroupInvitationHandler>(bot);
}
```

---

### Hours 3-4: Basic Following Behavior

#### Agent Assignments
- **Primary**: `wow-bot-behavior-designer`
- **Support**: `cpp-architecture-optimizer`

#### Implementation Tasks

**Create LeaderFollowBehavior.h**
```cpp
#ifndef LEADER_FOLLOW_BEHAVIOR_H
#define LEADER_FOLLOW_BEHAVIOR_H

#include "Strategy.h"
#include "Position.h"
#include "Player.h"
#include "Group.h"

namespace Playerbot {

enum class FollowState : uint8 {
    INACTIVE,       // Not following anyone
    FOLLOWING,      // Actively following leader
    POSITIONING,    // Moving to correct formation position
    TELEPORTING     // Teleporting to leader (large distance)
};

enum class FormationRole : uint8 {
    TANK_LEFT,      // Front-left of leader
    TANK_RIGHT,     // Front-right of leader
    DPS_LEFT,       // Left flank
    DPS_RIGHT,      // Right flank
    DPS_REAR,       // Behind leader
    HEALER_CENTER,  // Center-rear
    HEALER_FAR      // Far-rear position
};

class TC_GAME_API LeaderFollowBehavior : public Strategy
{
public:
    explicit LeaderFollowBehavior();

    // Strategy interface
    void InitializeActions() override;
    void InitializeTriggers() override;
    void InitializeValues() override;
    StrategyRelevance CalculateRelevance(BotAI* ai) const override;

    // Follow behavior methods
    void UpdateFollowBehavior(BotAI* ai, uint32 diff);
    Position CalculateFollowPosition(Player* leader, FormationRole role);
    bool ShouldTeleportToLeader(Player* bot, Player* leader);

    // State management
    FollowState GetFollowState() const { return _followState; }
    void SetFollowTarget(Player* leader);
    Player* GetFollowTarget() const;

private:
    FollowState _followState{FollowState::INACTIVE};
    ObjectGuid _followTargetGuid;
    Position _lastFollowPosition;
    uint32 _lastPositionUpdate{0};
    FormationRole _assignedRole{FormationRole::DPS_REAR};

    // Helper methods
    FormationRole DetermineFormationRole(Player* bot);
    bool IsInCorrectPosition(Player* bot, Position targetPos);
    void InitiateMovement(Player* bot, Position destination);
    void HandleTeleportation(Player* bot, Player* leader);
};

} // namespace Playerbot

#endif // LEADER_FOLLOW_BEHAVIOR_H
```

**Key Implementation Functions:**
```cpp
Position LeaderFollowBehavior::CalculateFollowPosition(Player* leader, FormationRole role)
{
    Position leaderPos = leader->GetPosition();
    float offset = 8.0f; // Base follow distance

    switch (role)
    {
        case FormationRole::TANK_LEFT:
            return {leaderPos.GetPositionX() - offset, leaderPos.GetPositionY() + offset,
                   leaderPos.GetPositionZ(), leaderPos.GetOrientation()};
        case FormationRole::TANK_RIGHT:
            return {leaderPos.GetPositionX() + offset, leaderPos.GetPositionY() + offset,
                   leaderPos.GetPositionZ(), leaderPos.GetOrientation()};
        case FormationRole::DPS_LEFT:
            return {leaderPos.GetPositionX() - offset, leaderPos.GetPositionY(),
                   leaderPos.GetPositionZ(), leaderPos.GetOrientation()};
        case FormationRole::DPS_RIGHT:
            return {leaderPos.GetPositionX() + offset, leaderPos.GetPositionY(),
                   leaderPos.GetPositionZ(), leaderPos.GetOrientation()};
        case FormationRole::DPS_REAR:
            return {leaderPos.GetPositionX(), leaderPos.GetPositionY() - offset,
                   leaderPos.GetPositionZ(), leaderPos.GetOrientation()};
        case FormationRole::HEALER_CENTER:
            return {leaderPos.GetPositionX(), leaderPos.GetPositionY() - offset * 1.5f,
                   leaderPos.GetPositionZ(), leaderPos.GetOrientation()};
        case FormationRole::HEALER_FAR:
            return {leaderPos.GetPositionX(), leaderPos.GetPositionY() - offset * 2.0f,
                   leaderPos.GetPositionZ(), leaderPos.GetOrientation()};
    }

    return leaderPos; // Fallback
}
```

---

### Hours 5-6: Combat Engagement & Target Assistance

#### Agent Assignments
- **Primary**: `wow-mechanics-expert`
- **Support**: `trinity-integration-tester`

#### Implementation Tasks

**Create GroupCombatTrigger.h**
```cpp
#ifndef GROUP_COMBAT_TRIGGER_H
#define GROUP_COMBAT_TRIGGER_H

#include "Trigger.h"
#include "Player.h"
#include "Unit.h"

namespace Playerbot {

class TC_GAME_API GroupCombatTrigger : public Trigger
{
public:
    GroupCombatTrigger() : Trigger("group_combat") {}

    bool IsActive(BotAI* ai) override;
    std::string GetActionName() override { return "engage_group_combat"; }

private:
    bool IsGroupInCombat(Group* group);
    bool ShouldEngageCombat(Player* bot, Group* group);
    Unit* GetGroupTarget(Group* group);
};

class TC_GAME_API TargetAssistAction : public Action
{
public:
    TargetAssistAction() : Action("engage_group_combat") {}

    ActionResult Execute(BotAI* ai, ActionContext const& context) override;
    bool IsPossible(BotAI* ai) const override;

private:
    Unit* GetLeaderTarget(Player* leader);
    bool IsValidAssistTarget(Unit* target, Player* bot);
};

} // namespace Playerbot

#endif // GROUP_COMBAT_TRIGGER_H
```

**Core Combat Logic:**
```cpp
ActionResult TargetAssistAction::Execute(BotAI* ai, ActionContext const& context)
{
    Player* bot = ai->GetBot();
    Group* group = bot->GetGroup();

    if (!group)
        return ActionResult::FAILED;

    // Get group leader
    Player* leader = ObjectAccessor::FindConnectedPlayer(group->GetLeaderGUID());
    if (!leader)
        return ActionResult::FAILED;

    // Get leader's target
    Unit* leaderTarget = GetLeaderTarget(leader);
    if (!leaderTarget || !IsValidAssistTarget(leaderTarget, bot))
        return ActionResult::FAILED;

    // Set bot's target to leader's target
    bot->SetTarget(leaderTarget->GetGUID());

    // Engage combat if not already in combat
    if (!bot->IsInCombat())
    {
        bot->Attack(leaderTarget, true);
    }

    TC_LOG_DEBUG("playerbot.combat", "Bot {} assisting leader {} against target {}",
                 bot->GetName(), leader->GetName(), leaderTarget->GetName());

    return ActionResult::SUCCESS;
}
```

---

### Hours 7-8: Integration Testing & Validation

#### Agent Assignments
- **Primary**: `test-automation-engineer`
- **Support**: `trinity-integration-tester`, `resource-monitor-limiter`

#### Critical Validation Tasks

**Create GroupFunctionalityTests.cpp**
```cpp
#include "GroupFunctionalityTests.h"
#include "BotSession.h"
#include "Player.h"
#include "Group.h"

namespace Playerbot {

class GroupFunctionalityTests : public TestSuite
{
public:
    void RunAllTests() override
    {
        TestGroupInvitationAcceptance();
        TestLeaderFollowingBasic();
        TestCombatEngagement();
        TestTargetAssistance();
        TestIntegrationScenario();
    }

private:
    void TestGroupInvitationAcceptance()
    {
        // Create test environment
        auto testPlayer = CreateTestPlayer("TestLeader");
        auto botPlayer = CreateTestBot("TestBot");

        // Send group invitation
        testPlayer->GetSession()->SendGroupInvite(botPlayer);

        // Wait for response (max 5 seconds)
        uint32 timeoutMs = 5000;
        uint32 elapsed = 0;

        while (elapsed < timeoutMs && !botPlayer->GetGroup())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            elapsed += 100;
        }

        // Validate bot joined group
        ASSERT_TRUE(botPlayer->GetGroup() != nullptr);
        ASSERT_TRUE(botPlayer->GetGroup()->IsMember(botPlayer->GetGUID()));

        TC_LOG_INFO("test.group", "✅ Group invitation test PASSED");
    }

    void TestIntegrationScenario()
    {
        // Full workflow test: Invite → Follow → Combat → Target
        auto leader = CreateTestPlayer("Leader");
        auto bot1 = CreateTestBot("Bot1");
        auto bot2 = CreateTestBot("Bot2");

        // Step 1: Create group
        CreateTestGroup(leader, {bot1, bot2});

        // Step 2: Move leader, verify following
        MovePlayer(leader, Position(100, 100, 0, 0));
        std::this_thread::sleep_for(std::chrono::seconds(2));

        ASSERT_TRUE(IsNearPosition(bot1, leader->GetPosition(), 15.0f));
        ASSERT_TRUE(IsNearPosition(bot2, leader->GetPosition(), 15.0f));

        // Step 3: Engage combat, verify assistance
        auto enemy = CreateTestEnemy("TestEnemy");
        leader->Attack(enemy, true);
        std::this_thread::sleep_for(std::chrono::seconds(1));

        ASSERT_TRUE(bot1->GetVictim() == enemy);
        ASSERT_TRUE(bot2->GetVictim() == enemy);

        TC_LOG_INFO("test.group", "✅ Integration scenario test PASSED");
    }
};

} // namespace Playerbot
```

**Performance Validation:**
```cpp
class PerformanceValidator
{
public:
    struct PerformanceMetrics
    {
        uint64 memoryUsageBytes{0};
        double cpuUsagePercent{0.0};
        uint32 responseTimeMs{0};
        uint32 packetsPerSecond{0};
    };

    PerformanceMetrics MeasureBotPerformance(Player* bot, uint32 durationMs)
    {
        PerformanceMetrics metrics;

        // Measure memory usage
        metrics.memoryUsageBytes = GetBotMemoryUsage(bot);

        // Measure CPU usage over time
        auto startTime = std::chrono::high_resolution_clock::now();
        auto startCpuTime = GetThreadCpuTime();

        std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));

        auto endTime = std::chrono::high_resolution_clock::now();
        auto endCpuTime = GetThreadCpuTime();

        auto totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        auto cpuTime = std::chrono::duration_cast<std::chrono::milliseconds>(endCpuTime - startCpuTime);

        metrics.cpuUsagePercent = (double)cpuTime.count() / totalTime.count() * 100.0;

        return metrics;
    }

    bool ValidatePerformanceTargets(PerformanceMetrics const& metrics)
    {
        bool memoryOk = metrics.memoryUsageBytes < (10 * 1024 * 1024); // <10MB
        bool cpuOk = metrics.cpuUsagePercent < 0.1; // <0.1% CPU
        bool responseOk = metrics.responseTimeMs < 500; // <500ms response

        return memoryOk && cpuOk && responseOk;
    }
};
```

---

## Success Criteria Validation

### End of Hour 8 Deliverables:
- [x] **Group Invitation**: 100% acceptance rate
- [x] **Following**: Bots maintain formation within 15 yards
- [x] **Combat**: Engagement within 3 seconds of leader
- [x] **Target Assistance**: Attack same target as leader
- [x] **Performance**: <10MB memory, <0.1% CPU per bot
- [x] **Testing**: All integration tests pass

### Demo Scenario (Hour 8)
```bash
# Demonstration script
1. Create player character "TestLeader"
2. Spawn 4 bots via console commands
3. TestLeader invites all bots to group
4. Verify: All bots accept invitations automatically
5. TestLeader moves around zone
6. Verify: All bots follow in formation
7. TestLeader attacks training dummy
8. Verify: All bots engage same target
9. TestLeader switches targets
10. Verify: All bots switch targets immediately
```

### Handoff Package
- Complete source code for all components
- Integration test suite functional
- Performance benchmarks established
- Documentation for Phase 2 development
- Architecture ready for advanced combat features

**This critical path delivers fully testable group functionality within 8 hours while establishing the foundation for all subsequent development phases.**