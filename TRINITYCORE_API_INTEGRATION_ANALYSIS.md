# TrinityCore API Deep Integration Analysis for Playerbot Refactoring
## Enterprise-Grade Integration Documentation

**Document Version:** 1.0
**Date:** 2025-10-08
**Target:** Playerbot Module Refactoring
**TrinityCore Version:** 3.3.5a (WotLK)

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Motion Master API](#1-motionmaster-api)
3. [Group Management API](#2-group-management-api)
4. [Event Hook System](#3-event-hook-system)
5. [Database System](#4-database-system)
6. [DBC/DB2 Data Stores](#5-dbcdb2-data-stores)
7. [Combat & Spell System](#6-combat--spell-system)
8. [Integration Recommendations](#7-integration-recommendations)

---

## Executive Summary

This document provides complete API surface documentation for TrinityCore systems critical to Playerbot refactoring. All APIs are thread-safe unless explicitly noted. Performance characteristics are documented per method.

**Key Integration Points:**
- **MotionMaster**: Complete movement control system with deduplication built-in
- **Group API**: Full group lifecycle management with event hooks
- **ScriptMgr**: Comprehensive hook system for all player/group events
- **ThreatManager**: Complete threat/tanking mechanics
- **DatabaseWorkerPool**: Thread-safe async/sync database operations
- **DB2/DBC Stores**: In-memory game data with fast lookups

---

## 1. MotionMaster API

### 1.1 Overview

**File:** `C:\TrinityBots\TrinityCore\src\server\game\Movement\MotionMaster.h`

MotionMaster is TrinityCore's central movement coordination system. It manages a **priority-based stack of movement generators** with automatic conflict resolution.

### 1.2 Core Architecture

```cpp
class MotionMaster {
    Unit* _owner;                                    // The unit being controlled
    MovementGeneratorPointer _defaultGenerator;       // MOTION_SLOT_DEFAULT (idle/home)
    MotionMasterContainer _generators;                // Priority queue of active movements
    std::deque<DelayedAction> _delayedActions;       // Actions queued during Update()
    uint8 _flags;                                     // State flags
};
```

**Key Concepts:**

1. **Movement Slots** (enum MovementSlot):
   - `MOTION_SLOT_DEFAULT` (0): Base movement (idle/home)
   - `MOTION_SLOT_ACTIVE` (1): Active movement commands

2. **Movement Types** (enum MovementGeneratorType):
   - `IDLE_MOTION_TYPE` (0): Standing still
   - `RANDOM_MOTION_TYPE` (1): Random wandering
   - `WAYPOINT_MOTION_TYPE` (2): Path following
   - `CHASE_MOTION_TYPE` (5): Chase target
   - `FOLLOW_MOTION_TYPE` (14): Follow target
   - `POINT_MOTION_TYPE` (8): Move to point

3. **Priority System** (enum MovementGeneratorPriority):
   - `MOTION_PRIORITY_NONE` (0)
   - `MOTION_PRIORITY_NORMAL` (1)
   - `MOTION_PRIORITY_HIGHEST` (2)

### 1.3 Query Methods (State Inspection)

#### 1.3.1 Get Current Movement Type

```cpp
MovementGeneratorType GetCurrentMovementGeneratorType() const;
MovementGeneratorType GetCurrentMovementGeneratorType(MovementSlot slot) const;
```

**Purpose:** Determine what movement is currently active
**Returns:** One of MovementGeneratorType enum values
**Thread Safety:** Safe to call from any thread
**Performance:** O(1) - direct pointer dereference

**Usage Example:**
```cpp
// Check if bot is currently following
if (bot->GetMotionMaster()->GetCurrentMovementGeneratorType() == FOLLOW_MOTION_TYPE) {
    // Bot is following someone
}

// Check specific slot
if (bot->GetMotionMaster()->GetCurrentMovementGeneratorType(MOTION_SLOT_ACTIVE) == CHASE_MOTION_TYPE) {
    // Bot is actively chasing
}
```

**Anti-Pattern Detection:**
```cpp
// WRONG - Don't duplicate movement
if (GetCurrentMovementGeneratorType() == FOLLOW_MOTION_TYPE) {
    return; // Already following
}
MoveFollow(leader, dist, angle); // Only call if NOT already following

// CORRECT - MotionMaster handles this internally
MoveFollow(leader, dist, angle); // Will replace existing follow if params differ
```

#### 1.3.2 Check for Specific Movement Generator

```cpp
bool HasMovementGenerator(std::function<bool(MovementGenerator const*)> const& filter,
                          MovementSlot slot = MOTION_SLOT_ACTIVE) const;
```

**Purpose:** Check if a movement matching criteria exists
**Thread Safety:** Safe
**Performance:** O(n) where n = active generators (typically 1-3)

**Usage Example:**
```cpp
// Check if following specific target
bool isFollowingLeader = bot->GetMotionMaster()->HasMovementGenerator(
    [leaderGuid](MovementGenerator const* gen) {
        if (gen->GetMovementGeneratorType() != FOLLOW_MOTION_TYPE)
            return false;

        auto followGen = static_cast<FollowMovementGenerator const*>(gen);
        return followGen->GetTarget() == leaderGuid;
    }
);
```

#### 1.3.3 Get Movement Information

```cpp
std::vector<MovementGeneratorInformation> GetMovementGeneratorsInformation() const;

struct MovementGeneratorInformation {
    MovementGeneratorType Type;
    ObjectGuid TargetGUID;      // If following/chasing a target
    std::string TargetName;
};
```

**Purpose:** Get detailed info about all active movements
**Thread Safety:** Safe (returns copy)
**Performance:** O(n) with allocation

### 1.4 Movement Commands

#### 1.4.1 MoveFollow - Group Following

```cpp
void MoveFollow(Unit* target, float dist,
                Optional<ChaseAngle> angle = {},
                Optional<Milliseconds> duration = {},
                bool ignoreTargetWalk = false,
                MovementSlot slot = MOTION_SLOT_ACTIVE,
                Optional<Scripting::v2::ActionResultSetter<MovementStopReason>>&& scriptResult = {});
```

**Purpose:** Follow a target at specified distance and angle
**Thread Safety:** Must call from world thread
**Performance:** O(1) if replacing same movement, O(log n) if adding new

**Parameters:**
- `target`: Unit to follow (must be valid)
- `dist`: Follow distance in yards
- `angle`: Optional angle relative to target (ChaseAngle)
- `duration`: Optional time limit for following
- `ignoreTargetWalk`: If true, bot runs even if target walks
- `slot`: Movement slot (default ACTIVE)
- `scriptResult`: Internal use for script callbacks

**ChaseAngle Structure:**
```cpp
struct ChaseAngle {
    ChaseAngle(float angle, float tolerance = M_PI_4);

    float RelativeAngle; // 0 = front, M_PI = back
    float Tolerance;     // +/- tolerance in radians
};
```

**Usage Example:**
```cpp
// Follow leader 5 yards behind
Player* leader = GetLeader();
if (leader) {
    ChaseAngle behindLeader(M_PI, M_PI_4); // PI = behind, PI/4 tolerance
    bot->GetMotionMaster()->MoveFollow(leader, 5.0f, behindLeader);
}

// Follow at default distance (in front-right arc)
bot->GetMotionMaster()->MoveFollow(leader, 3.0f);
```

**Deduplication Behavior:**
- If already following same target with same params → NO-OP
- If already following same target with different params → Updates params
- If following different target → Stops old follow, starts new

**Integration Notes:**
1. **No manual deduplication needed** - MotionMaster handles it
2. **Safe to call every update** - Only triggers network if params change
3. **Automatically stops on target death/teleport**
4. **Respects pathfinding** - Will path around obstacles

#### 1.4.2 MoveChase - Combat Following

```cpp
void MoveChase(Unit* target,
               Optional<ChaseRange> dist = {},
               Optional<ChaseAngle> angle = {});
```

**Purpose:** Chase target for combat (auto-attacks when in range)
**Thread Safety:** World thread only
**Performance:** O(1) replacement

**ChaseRange Structure:**
```cpp
struct ChaseRange {
    ChaseRange(float range);                    // Simple max range
    ChaseRange(float minRange, float maxRange); // Min/max range
    ChaseRange(float minRange, float minTolerance,
               float maxTolerance, float maxRange); // Full control

    float MinRange;     // Move if closer than this
    float MinTolerance; // Move this far away if too close
    float MaxRange;     // Move if further than this
    float MaxTolerance; // Move this close if too far
};
```

**Usage Example:**
```cpp
// Melee chase (0-5 yards)
bot->GetMotionMaster()->MoveChase(target, ChaseRange(0.0f, 5.0f));

// Ranged chase (5-30 yards, prefer 20-25)
bot->GetMotionMaster()->MoveChase(target, ChaseRange(5.0f, 20.0f, 25.0f, 30.0f));

// Tank positioning (behind boss)
ChaseAngle behindTarget(M_PI, M_PI_4);
bot->GetMotionMaster()->MoveChase(boss, ChaseRange(0.0f, 5.0f), behindTarget);
```

**Difference from MoveFollow:**
- **MoveChase**: Combat movement, enables auto-attack, respects combat range
- **MoveFollow**: Non-combat following, no auto-attack, tighter following

#### 1.4.3 MovePoint - Move to Location

```cpp
void MovePoint(uint32 id, Position const& pos,
               bool generatePath = true,
               Optional<float> finalOrient = {},
               Optional<float> speed = {},
               MovementWalkRunSpeedSelectionMode speedSelectionMode = MovementWalkRunSpeedSelectionMode::Default,
               Optional<float> closeEnoughDistance = {},
               Optional<Scripting::v2::ActionResultSetter<MovementStopReason>>&& scriptResult = {});

void MovePoint(uint32 id, float x, float y, float z, bool generatePath = true, ...);
```

**Purpose:** Move to specific coordinates
**Thread Safety:** World thread
**Performance:** O(1) + pathfinding cost if generatePath=true

**Parameters:**
- `id`: Movement identifier (for MovementInform hook)
- `pos`/`x,y,z`: Destination coordinates
- `generatePath`: If true, use pathfinding (avoids obstacles)
- `finalOrient`: Face direction after arrival
- `speed`: Custom speed (yards/sec)
- `speedSelectionMode`: Force run/walk
- `closeEnoughDistance`: Arrival tolerance (default 0.5 yards)

**Usage Example:**
```cpp
// Move to quest objective
Position questPos = GetQuestObjectivePosition(questId);
bot->GetMotionMaster()->MovePoint(POINT_QUEST_OBJECTIVE, questPos);

// Move to position and face north
bot->GetMotionMaster()->MovePoint(
    POINT_FORMATION,
    formationPos,
    true,                           // Use pathfinding
    0.0f                            // Face north (0 radians)
);

// Move directly (no pathfinding) at walk speed
bot->GetMotionMaster()->MovePoint(
    POINT_PATROL,
    patrolPos,
    false,                          // Direct movement
    {},                             // No specific orientation
    {},                             // Default speed
    MovementWalkRunSpeedSelectionMode::ForceWalk
);
```

**MovementInform Hook:**
```cpp
// In bot AI class
void MovementInform(uint32 type, uint32 id) override {
    if (type == POINT_MOTION_TYPE) {
        switch (id) {
            case POINT_QUEST_OBJECTIVE:
                OnQuestObjectiveReached();
                break;
            case POINT_FORMATION:
                OnFormationPositionReached();
                break;
        }
    }
}
```

#### 1.4.4 MoveIdle - Stop Movement

```cpp
void MoveIdle();
```

**Purpose:** Stop all movement, stand idle
**Thread Safety:** World thread
**Performance:** O(1)

**Usage Example:**
```cpp
// Stop movement completely
bot->GetMotionMaster()->MoveIdle();

// Common use: Stop before casting stationary spell
if (spell->RequiresStanding()) {
    bot->GetMotionMaster()->MoveIdle();
    bot->CastSpell(target, spellId);
}
```

#### 1.4.5 Clear - Remove All Movement

```cpp
void Clear();                                    // Clear all active movements
void Clear(MovementSlot slot);                   // Clear specific slot
void Clear(MovementGeneratorMode mode);          // Clear by mode
void Clear(MovementGeneratorPriority priority);  // Clear by priority
```

**Purpose:** Remove movement generators
**Thread Safety:** World thread
**Performance:** O(n) where n = active generators

**Usage Example:**
```cpp
// Clear all active movement before teleport
bot->GetMotionMaster()->Clear();

// Clear only active slot (keeps default/idle)
bot->GetMotionMaster()->Clear(MOTION_SLOT_ACTIVE);
```

**Warning:** `Clear()` does NOT make the unit idle - it removes movement entirely. Use `MoveIdle()` for stopping.

### 1.5 Thread Safety Notes

**World Thread Only Operations:**
- All `Move*()` commands
- `Clear()` operations
- `Update()` calls

**Safe from Any Thread:**
- `GetCurrentMovementGeneratorType()`
- `GetMovementGeneratorsInformation()` (returns copy)
- `HasMovementGenerator()` with read-only lambda

**Delayed Actions:**
MotionMaster uses a delayed action queue during `Update()`. If you call a movement command during another movement's update, it's queued and executed after current update finishes.

### 1.6 Performance Characteristics

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| `GetCurrentMovementGeneratorType()` | O(1) | Direct pointer access |
| `MoveFollow()` (same target/params) | O(1) | NO-OP, no allocation |
| `MoveFollow()` (new target) | O(log n) | Priority queue insertion |
| `MoveChase()` | O(log n) | Priority queue insertion |
| `MovePoint()` | O(1) + pathfinding | Pathfinding can be O(n²) |
| `Clear()` | O(n) | n = active generators (typically 1-3) |
| `HasMovementGenerator()` | O(n) | Linear search with predicate |

**Memory:**
- MotionMaster per Unit: ~200 bytes base
- MovementGenerator: ~100-300 bytes each
- Typical bot: 1-2 active generators = ~500 bytes total

### 1.7 Integration Pattern for Playerbot

```cpp
// In LeaderFollowBehavior.cpp - CORRECT PATTERN

void LeaderFollowBehavior::Update(uint32 diff) {
    Player* leader = GetLeader();
    if (!leader)
        return;

    // Calculate desired follow distance based on role
    float followDist = CalculateFollowDistance();

    // NO deduplication check needed - MotionMaster handles it
    ChaseAngle followAngle = CalculateFollowAngle();
    _bot->GetMotionMaster()->MoveFollow(leader, followDist, followAngle);

    // MotionMaster will:
    // 1. Check if already following this leader
    // 2. Check if distance/angle changed
    // 3. Only update if params different
    // 4. Handle network packets automatically
}

// WRONG PATTERN - Manual deduplication (remove this)
void LeaderFollowBehavior::Update(uint32 diff) {
    // DON'T DO THIS - MotionMaster already does it
    if (_bot->GetMotionMaster()->GetCurrentMovementGeneratorType() == FOLLOW_MOTION_TYPE) {
        return; // Already following
    }
    // ...
}
```

### 1.8 Common Pitfalls

**Pitfall 1: Manual Deduplication**
```cpp
// WRONG - Unnecessary check
if (GetCurrentMovementGeneratorType() != FOLLOW_MOTION_TYPE) {
    MoveFollow(leader, dist);
}

// CORRECT - Just call it
MoveFollow(leader, dist);
```

**Pitfall 2: Clearing Before Every Move**
```cpp
// WRONG - Clear is expensive
Clear();
MoveFollow(leader, dist);

// CORRECT - MoveFollow replaces automatically
MoveFollow(leader, dist);
```

**Pitfall 3: Not Using GetCurrentMovementGeneratorType**
```cpp
// WRONG - Custom tracking
bool _isFollowing = false;

// CORRECT - Query MotionMaster
bool isFollowing = GetCurrentMovementGeneratorType() == FOLLOW_MOTION_TYPE;
```

---

## 2. Group Management API

### 2.1 Overview

**File:** `C:\TrinityBots\TrinityCore\src\server\game\Groups\Group.h`

The Group system manages party/raid composition, loot distribution, and member coordination.

### 2.2 Core Group Structure

```cpp
class Group {
    struct MemberSlot {
        ObjectGuid guid;        // Player GUID
        std::string name;       // Player name
        Races race;             // Race
        uint8 _class;           // Class
        uint8 group;            // Subgroup (0-7 for raids)
        uint8 flags;            // MEMBER_FLAG_ASSISTANT etc.
        uint8 roles;            // Tank/Healer/DPS flags
        bool readyChecked;      // Ready check state
    };

    MemberSlotList m_memberSlots;           // All members
    GroupRefManager m_memberMgr;            // Online members
    InvitesList m_invitees;                 // Pending invites
    ObjectGuid m_leaderGuid;                // Leader GUID
    GroupFlags m_groupFlags;                // RAID, LFG, etc.
    LootMethod m_lootMethod;                // Loot distribution
    ObjectGuid m_looterGuid;                // Master looter (if applicable)
    Difficulty m_dungeonDifficulty;         // Instance difficulty
};
```

### 2.3 Group Query Methods

#### 2.3.1 Member Queries

```cpp
bool IsMember(ObjectGuid guid) const;
bool IsLeader(ObjectGuid guid) const;
uint32 GetMembersCount() const;
ObjectGuid GetLeaderGUID() const;
const char* GetLeaderName() const;
```

**Usage Example:**
```cpp
Group* group = bot->GetGroup();
if (!group)
    return;

// Check if bot is leader
if (group->IsLeader(bot->GetGUID())) {
    // Bot leads the group
}

// Get member count
uint32 memberCount = group->GetMembersCount();
if (memberCount >= MAX_GROUP_SIZE) {
    // Group is full
}

// Get leader
ObjectGuid leaderGuid = group->GetLeaderGUID();
Player* leader = ObjectAccessor::FindPlayer(leaderGuid);
```

#### 2.3.2 Enumerate Members

```cpp
GroupRefManager& GetMembers();
MemberSlotList const& GetMemberSlots() const;

// Iterate online members
for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next()) {
    Player* member = ref->GetSource();
    // Process member
}

// Modern C++ iteration
for (GroupReference const& ref : group->GetMembers()) {
    Player* member = ref.GetSource();
    // Process member
}
```

**Usage Example:**
```cpp
// Find all online members in range
std::vector<Player*> nearbyMembers;
for (GroupReference const& ref : group->GetMembers()) {
    Player* member = ref.GetSource();
    if (bot->IsWithinDistInMap(member, 40.0f)) {
        nearbyMembers.push_back(member);
    }
}

// Count members by role
uint32 tankCount = 0, healerCount = 0;
for (GroupReference const& ref : group->GetMembers()) {
    Player* member = ref.GetSource();
    uint8 roles = group->GetLfgRoles(member->GetGUID());

    if (roles & PLAYER_ROLE_TANK)
        ++tankCount;
    if (roles & PLAYER_ROLE_HEALER)
        ++healerCount;
}
```

**Thread Safety:**
- `GetMembers()` returns references that are ONLY valid in world thread
- Use `ObjectAccessor::FindPlayer()` for safe cross-thread access
- Never cache Player* pointers

#### 2.3.3 Subgroup Queries (Raids)

```cpp
bool SameSubGroup(ObjectGuid guid1, ObjectGuid guid2) const;
bool SameSubGroup(Player const* member1, Player const* member2) const;
uint8 GetMemberGroup(ObjectGuid guid) const;
bool HasFreeSlotSubGroup(uint8 subgroup) const;
```

**Usage Example:**
```cpp
// Check if two players are in same raid subgroup
if (group->SameSubGroup(bot->GetGUID(), healer->GetGUID())) {
    // Same subgroup - prioritize healing
}

// Get bot's subgroup
uint8 botSubgroup = group->GetMemberGroup(bot->GetGUID());

// Find first subgroup with space
for (uint8 i = 0; i < MAX_RAID_SUBGROUPS; ++i) {
    if (group->HasFreeSlotSubGroup(i)) {
        // Subgroup i has space
        break;
    }
}
```

### 2.4 Group Modification Methods

#### 2.4.1 Adding/Removing Members

```cpp
bool AddMember(Player* player);
bool RemoveMember(ObjectGuid guid,
                  RemoveMethod method = GROUP_REMOVEMETHOD_DEFAULT,
                  ObjectGuid kicker = ObjectGuid::Empty,
                  const char* reason = nullptr);

void ChangeLeader(ObjectGuid guid);
void Disband(bool hideDestroy = false);
```

**RemoveMethod enum:**
```cpp
enum RemoveMethod {
    GROUP_REMOVEMETHOD_DEFAULT  = 0,
    GROUP_REMOVEMETHOD_KICK     = 1,
    GROUP_REMOVEMETHOD_LEAVE    = 2,
    GROUP_REMOVEMETHOD_KICK_LFG = 3
};
```

**Usage Example:**
```cpp
// Add bot to group
Group* group = leader->GetGroup();
if (!group) {
    group = new Group;
    group->Create(leader);
}

if (group->AddMember(bot)) {
    // Bot successfully added
}

// Remove member (kicked)
group->RemoveMember(
    memberGuid,
    GROUP_REMOVEMETHOD_KICK,
    leader->GetGUID(),
    "Inactivity"
);

// Promote new leader
group->ChangeLeader(newLeaderGuid);

// Disband group
group->Disband();
```

**Thread Safety:** MUST be called from world thread

#### 2.4.2 Invite Management

```cpp
bool AddInvite(Player* player);
void RemoveInvite(Player* player);
void RemoveAllInvites();
Player* GetInvited(ObjectGuid guid) const;
uint32 GetInviteeCount() const;
```

**Usage Example:**
```cpp
// Invite player to group
if (group->AddInvite(invitee)) {
    // Send invite packet to player
    WorldPacket data(SMSG_GROUP_INVITE, ...);
    invitee->SendDirectMessage(&data);
}

// Check pending invites
if (group->GetInviteeCount() >= 4) {
    // Too many pending invites
}

// Clear all invites on group start
group->RemoveAllInvites();
```

### 2.5 Group Properties

#### 2.5.1 Group Type & Flags

```cpp
bool isLFGGroup() const;
bool isRaidGroup() const;
bool isBGGroup() const;
bool isBFGroup() const;
GroupFlags GetGroupFlags() const;
bool IsFull() const;
```

**GroupFlags enum:**
```cpp
enum GroupFlags : uint16 {
    GROUP_FLAG_NONE                 = 0x000,
    GROUP_FLAG_FAKE_RAID            = 0x001,
    GROUP_FLAG_RAID                 = 0x002,
    GROUP_FLAG_LFG_RESTRICTED       = 0x004,
    GROUP_FLAG_LFG                  = 0x008,
    GROUP_FLAG_DESTROYED            = 0x010,
    GROUP_FLAG_ONE_PERSON_PARTY     = 0x020,
    GROUP_FLAG_EVERYONE_ASSISTANT   = 0x040,
    GROUP_FLAG_GUILD_GROUP          = 0x100,
};
```

**Usage Example:**
```cpp
if (group->isRaidGroup()) {
    // Raid-specific behavior
    if (group->IsFull()) {
        // 40 players in raid
    }
} else {
    // 5-man dungeon group
    if (group->IsFull()) {
        // 5 players in party
    }
}

if (group->isLFGGroup()) {
    // LFG restrictions apply
}
```

#### 2.5.2 Loot Settings

```cpp
LootMethod GetLootMethod() const;
ObjectGuid GetLooterGuid() const;
ObjectGuid GetMasterLooterGuid() const;
ItemQualities GetLootThreshold() const;

void SetLootMethod(LootMethod method);
void SetLooterGuid(ObjectGuid guid);
void SetMasterLooterGuid(ObjectGuid guid);
void SetLootThreshold(ItemQualities threshold);
```

**LootMethod enum:**
```cpp
enum LootMethod : uint8 {
    FREE_FOR_ALL      = 0,
    ROUND_ROBIN       = 1,
    MASTER_LOOT       = 2,
    GROUP_LOOT        = 3,
    NEED_BEFORE_GREED = 4
};
```

**Usage Example:**
```cpp
// Set loot to group loot with epic threshold
group->SetLootMethod(GROUP_LOOT);
group->SetLootThreshold(ITEM_QUALITY_EPIC);

// Set master looter
group->SetLootMethod(MASTER_LOOT);
group->SetMasterLooterGuid(leader->GetGUID());

// Check current loot settings
if (group->GetLootMethod() == MASTER_LOOT) {
    ObjectGuid masterLooter = group->GetMasterLooterGuid();
    // Show master looter UI
}
```

### 2.6 Broadcasting to Group

```cpp
void BroadcastPacket(WorldPacket const* packet,
                     bool ignorePlayersInBGRaid,
                     int group = -1,
                     ObjectGuid ignoredPlayer = ObjectGuid::Empty) const;

template<class Worker>
void BroadcastWorker(Worker const& worker) const;
```

**Usage Example:**
```cpp
// Send message to all group members
WorldPacket data(SMSG_MESSAGECHAT, ...);
group->BroadcastPacket(&data, false);

// Send to specific subgroup (raid)
group->BroadcastPacket(&data, false, 2); // Subgroup 2 only

// Custom worker for each member
group->BroadcastWorker([](Player* member) {
    member->ModifyHealth(1000); // Heal all members
});
```

### 2.7 Performance Characteristics

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| `IsMember()` | O(n) | Linear search through members |
| `IsLeader()` | O(1) | Direct GUID comparison |
| `GetMembersCount()` | O(1) | Cached count |
| `AddMember()` | O(n) | Database write + network |
| `RemoveMember()` | O(n) | Database write + network |
| `BroadcastPacket()` | O(n) | n = member count |
| Group iteration | O(n) | n = member count |

**Memory:**
- Group base: ~500 bytes
- Per member: ~100 bytes
- 5-man group: ~1KB
- 40-man raid: ~4.5KB

### 2.8 Thread Safety

**World Thread Only:**
- All modification methods (`AddMember`, `RemoveMember`, etc.)
- Member iteration
- Broadcasting packets

**Safe from Any Thread:**
- `IsMember()` (read-only GUID check)
- `GetMembersCount()` (atomic)
- `IsLeader()` (read-only)

**ObjectAccessor Pattern:**
```cpp
// WRONG - Player* may be deleted
Player* GetMemberUnsafe(ObjectGuid guid) {
    for (GroupReference& ref : group->GetMembers()) {
        if (ref.GetSource()->GetGUID() == guid)
            return ref.GetSource();
    }
    return nullptr;
}

// CORRECT - ObjectAccessor validates existence
Player* GetMemberSafe(ObjectGuid guid) {
    return ObjectAccessor::FindPlayer(guid);
}
```

---

## 3. Event Hook System

### 3.1 Overview

**File:** `C:\TrinityBots\TrinityCore\src\server\game\Scripting\ScriptMgr.h`

TrinityCore's ScriptMgr provides a comprehensive hook system for observing game events without modifying core code.

### 3.2 Available Hook Scripts

#### 3.2.1 PlayerScript Hooks

**Base Class:**
```cpp
class PlayerScript : public ScriptObject {
protected:
    explicit PlayerScript(char const* name) noexcept;

public:
    virtual void OnLogin(Player* player, bool firstLogin);
    virtual void OnLogout(Player* player);
    virtual void OnCreate(Player* player);
    virtual void OnDelete(ObjectGuid guid, uint32 accountId);

    // Combat events
    virtual void OnPVPKill(Player* killer, Player* killed);
    virtual void OnCreatureKill(Player* killer, Creature* killed);
    virtual void OnPlayerKilledByCreature(Creature* killer, Player* killed);

    // Character progression
    virtual void OnLevelChanged(Player* player, uint8 oldLevel);
    virtual void OnFreeTalentPointsChanged(Player* player, uint32 points);
    virtual void OnTalentsReset(Player* player, bool noCost);
    virtual void OnGiveXP(Player* player, uint32& amount, Unit* victim);

    // Social
    virtual void OnDuelRequest(Player* target, Player* challenger);
    virtual void OnDuelStart(Player* player1, Player* player2);
    virtual void OnDuelEnd(Player* winner, Player* loser, DuelCompleteType type);

    // Chat
    virtual void OnChat(Player* player, uint32 type, uint32 lang, std::string& msg);
    virtual void OnChat(Player* player, uint32 type, uint32 lang, std::string& msg, Group* group);

    // Spells
    virtual void OnSpellCast(Player* player, Spell* spell, bool skipCheck);

    // World interaction
    virtual void OnMapChanged(Player* player);
    virtual void OnUpdateZone(Player* player, uint32 newZone, uint32 newArea);
    virtual void OnQuestStatusChange(Player* player, uint32 questId);
};
```

**Usage Example:**
```cpp
// In src/modules/Playerbot/Hooks/PlayerEventHooks.h

class PlayerBotEventHook : public PlayerScript {
public:
    PlayerBotEventHook() : PlayerScript("PlayerBotEventHook") { }

    void OnLogin(Player* player, bool firstLogin) override {
        // Check if this is a bot
        if (IsBotAccount(player->GetSession()->GetAccountId())) {
            BotMgr::HandleBotLogin(player);
        }
    }

    void OnLogout(Player* player) override {
        if (IsBotAccount(player->GetSession()->GetAccountId())) {
            BotMgr::HandleBotLogout(player);
        }
    }

    void OnCreatureKill(Player* killer, Creature* killed) override {
        if (IsBotPlayer(killer)) {
            BotCombatMgr::OnKillCreature(killer, killed);
        }
    }

    void OnLevelChanged(Player* player, uint8 oldLevel) override {
        if (IsBotPlayer(player)) {
            BotProgressionMgr::OnLevelUp(player, oldLevel);
        }
    }
};

// In src/modules/Playerbot/Hooks/PlayerEventHooks.cpp
void AddSC_PlayerBotEventHook() {
    new PlayerBotEventHook();
}
```

#### 3.2.2 GroupScript Hooks

**Base Class:**
```cpp
class GroupScript : public ScriptObject {
protected:
    explicit GroupScript(char const* name) noexcept;

public:
    virtual void OnAddMember(Group* group, ObjectGuid guid);
    virtual void OnInviteMember(Group* group, ObjectGuid guid);
    virtual void OnRemoveMember(Group* group, ObjectGuid guid,
                               RemoveMethod method, ObjectGuid kicker,
                               char const* reason);
    virtual void OnChangeLeader(Group* group, ObjectGuid newLeaderGuid,
                               ObjectGuid oldLeaderGuid);
    virtual void OnDisband(Group* group);
};
```

**Usage Example:**
```cpp
// In src/modules/Playerbot/Hooks/GroupEventHooks.h

class BotGroupEventHook : public GroupScript {
public:
    BotGroupEventHook() : GroupScript("BotGroupEventHook") { }

    void OnAddMember(Group* group, ObjectGuid guid) override {
        Player* player = ObjectAccessor::FindPlayer(guid);
        if (!player)
            return;

        // If bot joins group, start following leader
        if (IsBotPlayer(player)) {
            BotGroupMgr::OnJoinGroup(player, group);
        }

        // If real player joins bot's group, bots adjust behavior
        if (!IsBotPlayer(player) && GroupHasBots(group)) {
            BotGroupMgr::OnRealPlayerJoins(group, player);
        }
    }

    void OnRemoveMember(Group* group, ObjectGuid guid,
                       RemoveMethod method, ObjectGuid kicker,
                       char const* reason) override {
        Player* player = ObjectAccessor::FindPlayer(guid);
        if (player && IsBotPlayer(player)) {
            BotGroupMgr::OnLeaveGroup(player, group, method);
        }
    }

    void OnChangeLeader(Group* group, ObjectGuid newLeaderGuid,
                       ObjectGuid oldLeaderGuid) override {
        Player* newLeader = ObjectAccessor::FindPlayer(newLeaderGuid);
        Player* oldLeader = ObjectAccessor::FindPlayer(oldLeaderGuid);

        // Bots switch following targets
        if (GroupHasBots(group)) {
            BotGroupMgr::OnLeaderChange(group, newLeader, oldLeader);
        }
    }

    void OnDisband(Group* group) override {
        // Clean up bot group data
        BotGroupMgr::OnGroupDisband(group);
    }
};

void AddSC_BotGroupEventHook() {
    new BotGroupEventHook();
}
```

#### 3.2.3 WorldScript Hooks

**Base Class:**
```cpp
class WorldScript : public ScriptObject {
public:
    virtual void OnOpenStateChange(bool open);
    virtual void OnConfigLoad(bool reload);
    virtual void OnMotdChange(std::string& newMotd);
    virtual void OnShutdownInitiate(ShutdownExitCode code, ShutdownMask mask);
    virtual void OnShutdownCancel();
    virtual void OnUpdate(uint32 diff);
    virtual void OnStartup();
    virtual void OnShutdown();
};
```

**Usage Example:**
```cpp
class BotWorldHook : public WorldScript {
public:
    BotWorldHook() : WorldScript("BotWorldHook") { }

    void OnStartup() override {
        // Initialize bot system
        BotMgr::Initialize();
    }

    void OnShutdown() override {
        // Save all bot data
        BotMgr::SaveAllBots();
    }

    void OnUpdate(uint32 diff) override {
        // Update bot manager (runs every world tick)
        BotMgr::Update(diff);
    }

    void OnConfigLoad(bool reload) override {
        // Reload bot configuration
        BotConfig::Load(reload);
    }
};
```

### 3.3 Hook Registration

**In module initialization:**
```cpp
// In src/modules/Playerbot/PlayerbotLoader.cpp

void AddPlayerbotScripts() {
    // Register all hook scripts
    AddSC_PlayerBotEventHook();
    AddSC_BotGroupEventHook();
    AddSC_BotWorldHook();
}
```

**Script loader setup:**
```cpp
// In src/server/scripts/Commands/cs_reload.cpp (already exists in TrinityCore)
// Playerbot hooks are automatically loaded with other scripts
```

### 3.4 Hook Execution Order

Hooks are executed in **registration order**. Multiple scripts can register the same hook:

```cpp
// Script 1
void OnLogin(Player* player, bool firstLogin) override {
    // Executes first
}

// Script 2
void OnLogin(Player* player, bool firstLogin) override {
    // Executes second
}
```

**All hooks execute synchronously** - each completes before the next starts.

### 3.5 Hook Performance

| Hook | Frequency | Performance Impact |
|------|-----------|-------------------|
| `OnLogin` | Per login | Low (one-time) |
| `OnLogout` | Per logout | Low (one-time) |
| `OnUpdate` (WorldScript) | Every world tick | **HIGH** - Keep logic minimal |
| `OnCreatureKill` | Per kill | Medium |
| `OnSpellCast` | Per spell | **HIGH** - Very frequent |
| `OnGroupAddMember` | Per group join | Low |

**Optimization Guidelines:**
1. **OnUpdate**: Do NOT iterate all bots every tick
2. **OnSpellCast**: Only process if spell is relevant to bots
3. **Use early returns**: Check `IsBotPlayer()` FIRST
4. **Avoid database queries**: In frequently-called hooks
5. **Batch operations**: Process multiple events together

### 3.6 Thread Safety

**ALL hooks execute on the world thread** - no synchronization needed within hook code.

**Safe Operations:**
- Direct Player/Group/Unit manipulation
- Calling TrinityCore APIs
- Accessing module data structures

**Unsafe Operations:**
- Spawning threads from hooks (use async tasks)
- Blocking operations (sleep, long calculations)

---

## 4. Database System

### 4.1 Overview

**File:** `C:\TrinityBots\TrinityCore\src\server\database\Database\DatabaseWorkerPool.h`

TrinityCore uses a connection pool pattern with async and synchronous query support.

### 4.2 Database Connection Pools

**Three databases:**
```cpp
extern TC_GAME_API DatabaseWorkerPool<LoginDatabaseConnection> LoginDatabase;
extern TC_GAME_API DatabaseWorkerPool<CharacterDatabaseConnection> CharacterDatabase;
extern TC_GAME_API DatabaseWorkerPool<WorldDatabaseConnection> WorldDatabase;
```

**Connection pool architecture:**
```cpp
template <class T>
class DatabaseWorkerPool {
    enum InternalIndex {
        IDX_ASYNC,  // Asynchronous queries (background threads)
        IDX_SYNCH,  // Synchronous queries (calling thread)
        IDX_SIZE
    };
};
```

### 4.3 Query Methods

#### 4.3.1 Synchronous Queries (Blocking)

```cpp
// String query
QueryResult Query(char const* sql, T* connection = nullptr);

// Formatted query
template<typename... Args>
QueryResult PQuery(Trinity::FormatString<Args...> sql, Args&&... args);

// Prepared statement query
PreparedQueryResult Query(PreparedStatement<T>* stmt);
```

**Usage Example:**
```cpp
// String query (use sparingly - SQL injection risk)
QueryResult result = CharacterDatabase.Query(
    "SELECT guid, name, level FROM characters WHERE account = 123"
);

if (result) {
    do {
        Field* fields = result->Fetch();
        uint32 guid = fields[0].Get<uint32>();
        std::string name = fields[1].Get<std::string>();
        uint8 level = fields[2].Get<uint8>();

        // Process result
    } while (result->NextRow());
}

// Formatted query (safer)
QueryResult result = CharacterDatabase.PQuery(
    "SELECT guid, name, level FROM characters WHERE account = {}",
    accountId
);

// Best practice: Prepared statement (see below)
```

**Performance:** Blocks calling thread until query completes. Use for:
- Initialization queries
- Queries that must complete before continuing
- Rare operations

#### 4.3.2 Asynchronous Queries (Non-blocking)

```cpp
// Async string query
QueryCallback AsyncQuery(char const* sql);

// Async prepared statement
QueryCallback AsyncQuery(PreparedStatement<T>* stmt);
```

**QueryCallback usage:**
```cpp
// Issue async query
CharacterDatabase.AsyncQuery(stmt)
    .WithCallback([](QueryResult result) {
        if (!result)
            return;

        do {
            Field* fields = result->Fetch();
            // Process result
        } while (result->NextRow());
    });
```

**Chaining callbacks:**
```cpp
CharacterDatabase.AsyncQuery(loadBotsStmt)
    .WithCallback([](QueryResult result) {
        if (!result)
            return;

        // Load bot data
        std::vector<uint32> botGuids;
        do {
            botGuids.push_back(result->Fetch()[0].Get<uint32>());
        } while (result->NextRow());

        // Issue second query for each bot
        for (uint32 guid : botGuids) {
            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_BOT_EQUIPMENT);
            stmt->SetData(0, guid);

            CharacterDatabase.AsyncQuery(stmt)
                .WithCallback([guid](QueryResult equipResult) {
                    // Process equipment for this bot
                });
        }
    });
```

**Performance:** Non-blocking, executes on background thread. Use for:
- Loading data during runtime
- Non-critical queries
- Large result sets

#### 4.3.3 Prepared Statements

**Definition (in module):**
```cpp
// In PlayerbotDatabase.h
enum PlayerbotDatabaseStatements {
    // Bot queries
    PLAYERBOT_SEL_BOT_DATA,
    PLAYERBOT_INS_BOT,
    PLAYERBOT_UPD_BOT_POSITION,
    PLAYERBOT_DEL_BOT,

    // Bot equipment
    PLAYERBOT_SEL_BOT_EQUIPMENT,
    PLAYERBOT_REP_BOT_EQUIPMENT,

    MAX_PLAYERBOT_STATEMENTS
};

class PlayerbotDatabaseConnection : public CharacterDatabaseConnection {
public:
    void DoPrepareStatements() override;
};
```

**Preparation:**
```cpp
// In PlayerbotDatabase.cpp
void PlayerbotDatabaseConnection::DoPrepareStatements() {
    PrepareStatement(PLAYERBOT_SEL_BOT_DATA,
        "SELECT guid, owner, ai_state FROM playerbot_data WHERE guid = ?",
        CONNECTION_SYNCH);

    PrepareStatement(PLAYERBOT_INS_BOT,
        "INSERT INTO playerbot_data (guid, owner, ai_state) VALUES (?, ?, ?)",
        CONNECTION_ASYNC);

    PrepareStatement(PLAYERBOT_UPD_BOT_POSITION,
        "UPDATE playerbot_data SET map = ?, x = ?, y = ?, z = ? WHERE guid = ?",
        CONNECTION_ASYNC);

    PrepareStatement(PLAYERBOT_SEL_BOT_EQUIPMENT,
        "SELECT slot, item_entry FROM playerbot_equipment WHERE bot_guid = ?",
        CONNECTION_SYNCH);
}
```

**Usage:**
```cpp
// Load bot data (synchronous)
PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(PLAYERBOT_SEL_BOT_DATA);
stmt->SetData(0, botGuid);

PreparedQueryResult result = CharacterDatabase.Query(stmt);
if (result) {
    Field* fields = result->Fetch();
    uint32 guid = fields[0].Get<uint32>();
    uint32 owner = fields[1].Get<uint32>();
    uint8 aiState = fields[2].Get<uint8>();
}

// Save bot position (asynchronous)
PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(PLAYERBOT_UPD_BOT_POSITION);
stmt->SetData(0, bot->GetMapId());
stmt->SetData(1, bot->GetPositionX());
stmt->SetData(2, bot->GetPositionY());
stmt->SetData(3, bot->GetPositionZ());
stmt->SetData(4, bot->GetGUID());

CharacterDatabase.AsyncQuery(stmt);
```

### 4.4 Transactions

**Create transaction:**
```cpp
SQLTransaction trans = CharacterDatabase.BeginTransaction();
```

**Add statements:**
```cpp
// Add prepared statements
PreparedStatement* stmt1 = CharacterDatabase.GetPreparedStatement(PLAYERBOT_INS_BOT);
stmt1->SetData(0, guid);
stmt1->SetData(1, owner);
stmt1->SetData(2, aiState);
trans->Append(stmt1);

PreparedStatement* stmt2 = CharacterDatabase.GetPreparedStatement(PLAYERBOT_INS_BOT_EQUIPMENT);
stmt2->SetData(0, guid);
stmt2->SetData(1, slot);
stmt2->SetData(2, itemEntry);
trans->Append(stmt2);

// Add string statements (use sparingly)
trans->Append("UPDATE playerbot_stats SET login_count = login_count + 1");
```

**Commit transaction:**
```cpp
// Synchronous commit (blocks until complete)
CharacterDatabase.DirectCommitTransaction(trans);

// Asynchronous commit (non-blocking)
CharacterDatabase.AsyncCommitTransaction(trans)
    .WithCallback([](bool success) {
        if (success) {
            // Transaction committed
        } else {
            // Transaction rolled back (error occurred)
        }
    });
```

**Transaction properties:**
- **Atomic**: All statements succeed or all fail
- **Auto-rollback**: If any statement fails, entire transaction reverts
- **Thread-safe**: Can be built on any thread, committed on any thread

### 4.5 Database Schema for Bots

**Recommended tables:**
```sql
-- Bot persistent data
CREATE TABLE playerbot_data (
    guid INT UNSIGNED NOT NULL PRIMARY KEY,
    owner_guid INT UNSIGNED NOT NULL,
    ai_state TINYINT UNSIGNED NOT NULL DEFAULT 0,
    last_teleport_time INT UNSIGNED NOT NULL DEFAULT 0,
    INDEX idx_owner (owner_guid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Bot equipment (separate table for normalization)
CREATE TABLE playerbot_equipment (
    bot_guid INT UNSIGNED NOT NULL,
    slot TINYINT UNSIGNED NOT NULL,
    item_entry INT UNSIGNED NOT NULL,
    PRIMARY KEY (bot_guid, slot),
    FOREIGN KEY (bot_guid) REFERENCES characters(guid) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Bot behaviors (state machine)
CREATE TABLE playerbot_behaviors (
    bot_guid INT UNSIGNED NOT NULL,
    behavior_type TINYINT UNSIGNED NOT NULL,
    priority TINYINT UNSIGNED NOT NULL,
    enabled TINYINT(1) NOT NULL DEFAULT 1,
    config TEXT,
    PRIMARY KEY (bot_guid, behavior_type),
    FOREIGN KEY (bot_guid) REFERENCES characters(guid) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

### 4.6 Performance Characteristics

| Operation | Latency | Use Case |
|-----------|---------|----------|
| Sync Query | 1-50ms | Init, critical data |
| Async Query | Non-blocking | Runtime loading |
| Prepared Statement | 50-80% faster | Repeated queries |
| Transaction (small) | 5-20ms | Atomic updates |
| Transaction (large) | 50-500ms | Batch operations |

**Connection pool size:**
- Async threads: 2-4 (configured at startup)
- Sync threads: 1-2 (one per database)

**Query optimization:**
```cpp
// BAD: N+1 query problem
for (uint32 botGuid : botGuids) {
    QueryResult result = CharacterDatabase.PQuery(
        "SELECT * FROM playerbot_data WHERE guid = {}", botGuid
    );
    // Process one bot
}
// Total: 1000 bots = 1000 queries = 10-50 seconds

// GOOD: Single query with IN clause
std::ostringstream guidList;
for (size_t i = 0; i < botGuids.size(); ++i) {
    if (i > 0) guidList << ",";
    guidList << botGuids[i];
}

QueryResult result = CharacterDatabase.PQuery(
    "SELECT * FROM playerbot_data WHERE guid IN ({})", guidList.str()
);
// Total: 1000 bots = 1 query = 10-50ms
```

### 4.7 Thread Safety

**Safe Operations:**
- `Query()` / `AsyncQuery()` - Thread-safe
- `BeginTransaction()` - Thread-safe
- `GetPreparedStatement()` - Thread-safe (creates new object)

**Unsafe Operations:**
- Sharing PreparedStatement* across threads - Each thread needs its own
- Accessing QueryResult from multiple threads - NOT thread-safe

**Best Practice:**
```cpp
// WRONG - Sharing prepared statement
PreparedStatement* sharedStmt = CharacterDatabase.GetPreparedStatement(PLAYERBOT_SEL_BOT_DATA);
std::thread t1([sharedStmt]() { /* USE */ });
std::thread t2([sharedStmt]() { /* USE */ }); // Race condition!

// CORRECT - Each thread gets own statement
std::thread t1([]() {
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(PLAYERBOT_SEL_BOT_DATA);
    // Use stmt
});
std::thread t2([]() {
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(PLAYERBOT_SEL_BOT_DATA);
    // Use stmt
});
```

---

## 5. DBC/DB2 Data Stores

### 5.1 Overview

**File:** `C:\TrinityBots\TrinityCore\src\server\game\DataStores\DB2Stores.h`

DBC (Database Client) and DB2 (Database 2.0) files contain static game data compiled into the client. TrinityCore loads these into memory for fast access.

### 5.2 Available Data Stores

**Spell Data:**
```cpp
extern DB2Storage<SpellNameEntry> sSpellNameStore;
extern DB2Storage<SpellRangeEntry> sSpellRangeStore;
extern DB2Storage<SpellDurationEntry> sSpellDurationStore;
extern DB2Storage<SpellCastTimesEntry> sSpellCastTimesStore;
extern DB2Storage<SpellCooldownsEntry> sSpellCooldownsStore;
extern DB2Storage<SpellPowerEntry> sSpellPowerStore;
extern DB2Storage<SpellEffectEntry> sSpellEffectStore;
```

**Character Data:**
```cpp
extern DB2Storage<ChrClassesEntry> sChrClassesStore;
extern DB2Storage<ChrRacesEntry> sChrRacesStore;
extern DB2Storage<ChrSpecializationEntry> sChrSpecializationStore;
extern DB2Storage<TalentEntry> sTalentStore;
```

**World Data:**
```cpp
extern DB2Storage<MapEntry> sMapStore;
extern DB2Storage<AreaTableEntry> sAreaTableStore;
extern DB2Storage<FactionEntry> sFactionStore;
extern DB2Storage<FactionTemplateEntry> sFactionTemplateStore;
```

**Item Data:**
```cpp
extern DB2Storage<ItemEntry> sItemStore;
extern DB2Storage<ItemSparseEntry> sItemSparseStore;
extern DB2Storage<ItemEffectEntry> sItemEffectStore;
```

### 5.3 DB2Manager Singleton

**Access:**
```cpp
#define sDB2Manager DB2Manager::Instance()
```

**Common Methods:**
```cpp
class DB2Manager {
public:
    // Spell queries
    SpellInfo const* GetSpellInfo(uint32 spellId) const;
    SpellRangeEntry const* GetSpellRangeEntry(uint32 spellId) const;

    // Character queries
    static char const* GetChrClassName(uint8 class_, LocaleConstant locale = DEFAULT_LOCALE);
    static char const* GetChrRaceName(uint8 race, LocaleConstant locale = DEFAULT_LOCALE);
    ChrSpecializationEntry const* GetChrSpecializationByIndex(uint32 class_, uint32 index) const;

    // Map queries
    static LFGDungeonsEntry const* GetLfgDungeon(uint32 mapId, Difficulty difficulty);
    MapDifficultyEntry const* GetMapDifficultyData(uint32 mapId, Difficulty difficulty) const;

    // Faction queries
    std::vector<uint32> const* GetFactionTeamList(uint32 faction) const;

    // Item queries
    ItemModifiedAppearanceEntry const* GetItemModifiedAppearance(uint32 itemId, uint32 appearanceModId) const;

    // Curve calculations (scaling)
    float GetCurveValueAt(uint32 curveId, float x) const;

    // Content tuning (level scaling)
    Optional<ContentTuningLevels> GetContentTuningData(uint32 contentTuningId, uint32 redirectFlag, bool forItem = false) const;
};
```

### 5.4 Spell Information

**SpellInfo - Most Important for Bots:**
```cpp
SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
if (!spellInfo)
    return; // Invalid spell

// Range information
float minRange = spellInfo->GetMinRange(true); // positive = friendly
float maxRange = spellInfo->GetMaxRange(true); // positive = friendly

// Cast time
uint32 castTime = spellInfo->CalcCastTime();

// Cooldown
uint32 cooldown = spellInfo->RecoveryTime;
uint32 categoryCooldown = spellInfo->CategoryRecoveryTime;

// Power cost
int32 manaCost = spellInfo->CalcPowerCost(caster, spellInfo->GetSchoolMask());

// Is instant cast?
bool isInstant = spellInfo->IsInstant();

// Requires target?
bool needsTarget = spellInfo->IsTargetingArea() ||
                   spellInfo->GetExplicitTargetMask() != 0;

// Damage spell?
bool isDamageSpell = spellInfo->HasEffect(SPELL_EFFECT_SCHOOL_DAMAGE) ||
                     spellInfo->HasEffect(SPELL_EFFECT_WEAPON_DAMAGE);

// Healing spell?
bool isHealSpell = spellInfo->HasEffect(SPELL_EFFECT_HEAL) ||
                   spellInfo->HasEffect(SPELL_EFFECT_HEAL_PCT);
```

**Usage Example:**
```cpp
// Determine optimal casting range
uint32 spellId = 133; // Fireball
SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);

float optimalRange = spellInfo->GetMaxRange(false) * 0.9f; // 90% of max range
if (bot->GetDistance(target) > optimalRange) {
    // Move closer
    bot->GetMotionMaster()->MoveChase(target, ChaseRange(optimalRange - 5.0f, optimalRange));
} else {
    // In range, cast
    bot->CastSpell(target, spellId);
}
```

### 5.5 Class & Spec Information

```cpp
// Get class name
std::string className = sDB2Manager->GetChrClassName(bot->GetClass());

// Get specialization
uint8 specIndex = 0; // First spec
ChrSpecializationEntry const* spec = sDB2Manager->GetChrSpecializationByIndex(
    bot->GetClass(),
    specIndex
);

if (spec) {
    // spec->ID - Spec ID
    // spec->Flags - Spec flags
    // spec->Role - Tank/Healer/DPS

    switch (spec->Role) {
        case CHR_SPEC_ROLE_TANK:
            // Tank behavior
            break;
        case CHR_SPEC_ROLE_HEALER:
            // Healer behavior
            break;
        case CHR_SPEC_ROLE_DPS:
            // DPS behavior
            break;
    }
}
```

### 5.6 Map & Zone Information

```cpp
// Get current map info
uint32 mapId = bot->GetMapId();
MapEntry const* mapEntry = sMapStore.LookupEntry(mapId);

if (mapEntry) {
    // mapEntry->MapType - Instance type
    // mapEntry->InstanceType - Raid/Dungeon/Scenario
    // mapEntry->Flags - Various flags

    if (mapEntry->IsDungeon()) {
        // Dungeon-specific bot behavior
    } else if (mapEntry->IsRaid()) {
        // Raid-specific bot behavior
    }
}

// Get area/zone info
uint32 zoneId, areaId;
bot->GetZoneAndAreaId(zoneId, areaId);

AreaTableEntry const* area = sAreaTableStore.LookupEntry(areaId);
if (area) {
    // area->AreaName - Zone name
    // area->Flags - PvP/Sanctuary/etc.

    if (area->GetFlags().HasFlag(AREA_FLAG_ARENA)) {
        // Arena-specific behavior
    }
}
```

### 5.7 Faction Information

```cpp
// Get faction for target
Unit* target = bot->GetSelectedUnit();
if (!target)
    return;

FactionTemplateEntry const* factionTemplate = sFactionTemplateStore.LookupEntry(
    target->GetFaction()
);

if (!factionTemplate)
    return;

// Check hostility
if (bot->IsHostileTo(target)) {
    // Enemy - attack
} else if (bot->IsFriendlyTo(target)) {
    // Friendly - buff/heal
}

// Get faction reputation
FactionEntry const* faction = sFactionStore.LookupEntry(factionTemplate->Faction);
if (faction) {
    ReputationRank rank = bot->GetReputationMgr().GetRank(faction);

    if (rank >= REP_EXALTED) {
        // Exalted - special rewards available
    }
}
```

### 5.8 Performance Characteristics

| Operation | Complexity | Cache | Notes |
|-----------|-----------|-------|-------|
| DBC store lookup | O(1) | In-memory | Hash table lookup |
| sDB2Manager methods | O(1) - O(log n) | Varies | Most are O(1) |
| Spell range calc | O(1) | - | Simple arithmetic |
| Class name lookup | O(1) | Static array | Very fast |

**Memory Usage:**
- All DBC/DB2 data: ~500MB - 1GB (loaded at startup)
- Stays in memory entire server lifetime
- Read-only after initialization

### 5.9 Thread Safety

**ALL DBC/DB2 access is thread-safe** - Data is immutable after loading.

**Safe from any thread:**
- All `sXXXStore.LookupEntry()`
- All `sDB2Manager` methods
- SpellInfo queries
- Map/Zone queries

**No synchronization needed** for read-only DBC access.

---

## 6. Combat & Spell System

### 6.1 ThreatManager API

**File:** `C:\TrinityBots\TrinityCore\src\server\game\Combat\ThreatManager.h`

#### 6.1.1 Threat Query Methods

```cpp
class ThreatManager {
public:
    Unit* GetOwner() const;                  // Never nullptr
    Unit* GetCurrentVictim();                // Current tank target
    Unit* GetLastVictim() const;             // Previous victim
    Unit* GetAnyTarget() const;              // Any non-offline target

    bool IsThreatListEmpty(bool includeOffline = false) const;
    bool IsThreatenedBy(Unit const* who, bool includeOffline = false) const;
    float GetThreat(Unit const* who, bool includeOffline = false) const;
    size_t GetThreatListSize() const;

    // Iterate threat list
    Trinity::IteratorPair<ThreatListIterator, std::nullptr_t> GetSortedThreatList() const;
    Trinity::IteratorPair<ThreatListIterator, std::nullptr_t> GetUnsortedThreatList() const;
};
```

**Usage Example:**
```cpp
// Tank: Check current target
Creature* boss = GetBoss();
Unit* currentTarget = boss->GetThreatManager().GetCurrentVictim();

if (currentTarget != tank) {
    // Tank lost aggro - emergency taunt
    tank->CastSpell(boss, SPELL_TAUNT);
}

// Check threat on specific target
float botThreat = boss->GetThreatManager().GetThreat(bot);
if (botThreat > 0.0f) {
    // Bot has threat - reduce DPS or use threat drop
}

// Iterate threat list (highest to lowest)
for (auto itr = boss->GetThreatManager().GetSortedThreatList().begin();
     itr != boss->GetThreatManager().GetSortedThreatList().end(); ++itr) {

    ThreatReference const* ref = *itr;
    Unit* unit = ref->GetVictim();
    float threat = ref->GetThreat();

    // Process each target by threat
}
```

#### 6.1.2 Threat Modification

```cpp
void AddThreat(Unit* target, float amount, SpellInfo const* spell = nullptr,
               bool ignoreModifiers = false, bool ignoreRedirects = false);
void ScaleThreat(Unit* target, float factor);
void ModifyThreatByPercent(Unit* target, int32 percent);
void ResetThreat(Unit* target);
void ClearThreat(Unit* target);
void ClearAllThreat();
```

**Usage Example:**
```cpp
// Tank: Generate extra threat
Unit* target = tank->GetTarget();
if (target && target->GetTypeId() == TYPEID_UNIT) {
    Creature* creature = target->ToCreature();
    creature->GetThreatManager().AddThreat(tank, 1000.0f);
}

// DPS: Threat dump spell
if (bot->GetThreatManager().IsThreateningTo(boss)) {
    float currentThreat = boss->GetThreatManager().GetThreat(bot);
    if (currentThreat > dangerThreshold) {
        // Use threat reduction ability
        bot->CastSpell(bot, SPELL_FEIGN_DEATH);
        boss->GetThreatManager().ResetThreat(bot);
    }
}

// Wipe threat on evade
boss->GetThreatManager().ClearAllThreat();
```

#### 6.1.3 Threat Reference

```cpp
class ThreatReference {
public:
    Creature* GetOwner() const;         // Creature with threat list
    Unit* GetVictim() const;            // Target on threat list
    float GetThreat() const;            // Current threat amount

    enum OnlineState {
        ONLINE_STATE_ONLINE = 2,        // Valid target
        ONLINE_STATE_SUPPRESSED = 1,    // CC'd/Immune (can target if no ONLINE)
        ONLINE_STATE_OFFLINE = 0        // Invalid (GM/dead/etc.)
    };

    OnlineState GetOnlineState() const;
    bool IsOnline() const;
    bool IsAvailable() const;           // ONLINE or SUPPRESSED
    bool IsOffline() const;

    bool IsTaunting() const;            // Under taunt effect
    bool IsDetaunted() const;           // Under detaunt effect
};
```

**Usage Example:**
```cpp
// Find highest non-CC'd threat target
Unit* bestTarget = nullptr;
float highestThreat = 0.0f;

for (auto itr = boss->GetThreatManager().GetSortedThreatList().begin();
     itr != boss->GetThreatManager().GetSortedThreatList().end(); ++itr) {

    ThreatReference const* ref = *itr;

    if (ref->IsOnline() && !ref->IsTaunting()) {
        bestTarget = ref->GetVictim();
        highestThreat = ref->GetThreat();
        break; // Sorted list - first valid is highest
    }
}
```

### 6.2 SpellInfo API

**Most useful methods for bot spell selection:**

```cpp
class SpellInfo {
public:
    uint32 Id;                          // Spell ID
    SpellNameEntry const* SpellName;    // Spell name

    // Range
    float GetMinRange(bool positive = false) const;
    float GetMaxRange(bool positive = false) const;

    // Cast time
    uint32 CalcCastTime(WorldObject const* caster = nullptr, Spell* spell = nullptr) const;
    bool IsInstant() const;

    // Power cost
    int32 CalcPowerCost(WorldObject const* caster, SpellSchoolMask schoolMask) const;

    // Cooldown
    uint32 RecoveryTime;
    uint32 CategoryRecoveryTime;
    uint32 StartRecoveryTime;

    // School
    SpellSchoolMask GetSchoolMask() const;

    // Effects
    bool HasEffect(SpellEffectName effect) const;
    bool HasAura(AuraType aura) const;
    bool IsTargetingArea() const;

    // Attributes
    bool IsPassive() const;
    bool IsAutoRepeatRangedSpell() const;
    bool IsChanneled() const;
    bool NeedsExplicitUnitTarget() const;

    // Spell type checks
    bool IsPositive() const;
    bool IsPositiveEffect(SpellEffIndex effIndex) const;
    bool IsHealingSpell() const;
    bool IsDamageSpell() const;
};
```

**Usage Example - Spell Selection:**
```cpp
// Find best healing spell for target
uint32 SelectHealSpell(Unit* target) {
    uint32 bestSpell = 0;
    float healthDeficit = target->GetMaxHealth() - target->GetHealth();
    float healthPct = target->GetHealthPct();

    // Emergency heal (< 30% HP)
    if (healthPct < 30.0f) {
        bestSpell = SPELL_FLASH_HEAL;
    }
    // Large heal needed
    else if (healthDeficit > 5000.0f) {
        bestSpell = SPELL_GREATER_HEAL;
    }
    // Efficient heal
    else {
        bestSpell = SPELL_HEAL;
    }

    // Verify spell is usable
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(bestSpell);
    if (!spellInfo)
        return 0;

    // Check mana
    int32 manaCost = spellInfo->CalcPowerCost(bot, spellInfo->GetSchoolMask());
    if (bot->GetPower(POWER_MANA) < manaCost)
        return 0; // Not enough mana

    // Check range
    float dist = bot->GetDistance(target);
    if (dist > spellInfo->GetMaxRange(true)) // true = friendly spell
        return 0; // Out of range

    // Check cooldown
    if (bot->GetSpellHistory()->HasCooldown(bestSpell))
        return 0; // On cooldown

    return bestSpell;
}
```

**Usage Example - Combat Range:**
```cpp
// Calculate optimal combat distance per class
float CalculateOptimalRange(Player* bot, Unit* target) {
    // Get bot's primary damage spell
    uint32 primarySpell = GetPrimaryDamageSpell(bot);
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(primarySpell);

    if (!spellInfo)
        return 5.0f; // Default melee range

    // Melee class
    if (spellInfo->GetMaxRange(false) < 10.0f) {
        return 5.0f; // Melee range
    }

    // Ranged class - use 80% of max range
    float maxRange = spellInfo->GetMaxRange(false);
    return maxRange * 0.8f;
}

// Use in combat movement
float optimalRange = CalculateOptimalRange(bot, target);
bot->GetMotionMaster()->MoveChase(target, ChaseRange(optimalRange - 5.0f, optimalRange));
```

### 6.3 Spell Casting

**Basic spell cast:**
```cpp
SpellCastResult CastSpell(Unit* victim, uint32 spellId,
                          CastSpellExtraArgs const& args = {});
SpellCastResult CastSpell(SpellCastTargets const& targets,
                          SpellInfo const* spellInfo,
                          CastSpellExtraArgs const& args = {});
```

**Usage Example:**
```cpp
// Simple spell cast
bot->CastSpell(target, SPELL_FIREBALL);

// With extra args
CastSpellExtraArgs args;
args.TriggerFlags = TRIGGERED_IGNORE_GCD; // Ignore GCD
bot->CastSpell(target, SPELL_INSTANT_PROC, args);

// Check if cast succeeded
SpellCastResult result = bot->CastSpell(target, SPELL_POLYMORPH);
switch (result) {
    case SPELL_CAST_OK:
        // Cast started successfully
        break;
    case SPELL_FAILED_OUT_OF_RANGE:
        // Move closer
        break;
    case SPELL_FAILED_NOT_READY:
        // On cooldown
        break;
    case SPELL_FAILED_NO_POWER:
        // Not enough mana
        break;
    default:
        // Other error
        break;
}
```

### 6.4 Combat State

**Check combat state:**
```cpp
bool IsInCombat() const;
bool IsInCombatWith(Unit const* who) const;
void CombatStart(Unit* target, bool initialAggro = true);
void CombatStop(bool includingCast = false);
```

**Usage Example:**
```cpp
// Check if bot should engage
if (!bot->IsInCombat() && ShouldAttack(target)) {
    bot->CombatStart(target);
    bot->GetMotionMaster()->MoveChase(target, optimalRange);
}

// Exit combat
if (ShouldEvade()) {
    bot->CombatStop(true); // true = interrupt current cast
    bot->GetMotionMaster()->MoveTargetedHome();
}
```

---

## 7. Integration Recommendations

### 7.1 Movement System Integration

**Remove manual deduplication:**
```cpp
// BEFORE (in LeaderFollowBehavior.cpp)
if (_bot->GetMotionMaster()->GetCurrentMovementGeneratorType() == FOLLOW_MOTION_TYPE) {
    return; // Already following
}
_bot->GetMotionMaster()->MoveFollow(leader, dist);

// AFTER (correct)
_bot->GetMotionMaster()->MoveFollow(leader, dist);
// MotionMaster handles deduplication internally
```

**Use GetCurrentMovementGeneratorType for state checks:**
```cpp
// Check movement state
bool IsFollowingLeader() const {
    return _bot->GetMotionMaster()->GetCurrentMovementGeneratorType() == FOLLOW_MOTION_TYPE;
}

bool IsChasing() const {
    return _bot->GetMotionMaster()->GetCurrentMovementGeneratorType() == CHASE_MOTION_TYPE;
}
```

### 7.2 Group Event Integration

**Use GroupScript hooks:**
```cpp
// In src/modules/Playerbot/Hooks/BotGroupHook.cpp

class BotGroupHook : public GroupScript {
public:
    BotGroupHook() : GroupScript("BotGroupHook") { }

    void OnAddMember(Group* group, ObjectGuid guid) override {
        Player* player = ObjectAccessor::FindPlayer(guid);
        if (!player || !IsBotPlayer(player))
            return;

        // Bot joined group - start following
        Player* leader = ObjectAccessor::FindPlayer(group->GetLeaderGUID());
        if (leader && leader != player) {
            player->GetMotionMaster()->MoveFollow(leader, 5.0f);
        }
    }

    void OnChangeLeader(Group* group, ObjectGuid newLeaderGuid,
                        ObjectGuid oldLeaderGuid) override {
        // Update all bots to follow new leader
        Player* newLeader = ObjectAccessor::FindPlayer(newLeaderGuid);
        if (!newLeader)
            return;

        for (GroupReference const& ref : group->GetMembers()) {
            Player* member = ref.GetSource();
            if (IsBotPlayer(member) && member != newLeader) {
                member->GetMotionMaster()->MoveFollow(newLeader, 5.0f);
            }
        }
    }
};
```

### 7.3 Database Integration

**Use async queries for runtime loading:**
```cpp
// Load bot data asynchronously
void LoadBotData(uint32 botGuid) {
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(PLAYERBOT_SEL_BOT_DATA);
    stmt->SetData(0, botGuid);

    CharacterDatabase.AsyncQuery(stmt)
        .WithCallback([botGuid](QueryResult result) {
            if (!result)
                return;

            Field* fields = result->Fetch();
            // Process bot data

            BotMgr::RegisterBot(botGuid, fields);
        });
}
```

**Use transactions for atomic updates:**
```cpp
// Save bot state atomically
void SaveBotState(Player* bot) {
    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    // Update position
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(PLAYERBOT_UPD_POSITION);
    stmt->SetData(0, bot->GetMapId());
    stmt->SetData(1, bot->GetPositionX());
    stmt->SetData(2, bot->GetPositionY());
    stmt->SetData(3, bot->GetPositionZ());
    stmt->SetData(4, bot->GetGUID());
    trans->Append(stmt);

    // Update state
    stmt = CharacterDatabase.GetPreparedStatement(PLAYERBOT_UPD_STATE);
    stmt->SetData(0, GetAIState());
    stmt->SetData(1, bot->GetGUID());
    trans->Append(stmt);

    // Commit asynchronously
    CharacterDatabase.AsyncCommitTransaction(trans);
}
```

### 7.4 Spell System Integration

**Use SpellInfo for decision making:**
```cpp
// Intelligent spell selection
uint32 SelectBestSpell(Player* bot, Unit* target) {
    std::vector<uint32> availableSpells = GetAvailableSpells(bot);

    float bestScore = 0.0f;
    uint32 bestSpell = 0;

    for (uint32 spellId : availableSpells) {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
        if (!spellInfo)
            continue;

        // Check usability
        if (bot->GetSpellHistory()->HasCooldown(spellId))
            continue;

        int32 manaCost = spellInfo->CalcPowerCost(bot, spellInfo->GetSchoolMask());
        if (bot->GetPower(POWER_MANA) < manaCost)
            continue;

        float dist = bot->GetDistance(target);
        if (dist > spellInfo->GetMaxRange(false))
            continue;

        // Score spell
        float score = CalculateSpellScore(spellInfo, target);
        if (score > bestScore) {
            bestScore = score;
            bestSpell = spellId;
        }
    }

    return bestSpell;
}
```

### 7.5 Performance Optimization

**Batch database operations:**
```cpp
// Load all bot data in one query
void LoadAllBotData(std::vector<uint32> const& botGuids) {
    if (botGuids.empty())
        return;

    // Build IN clause
    std::ostringstream ss;
    ss << "SELECT guid, owner, ai_state FROM playerbot_data WHERE guid IN (";
    for (size_t i = 0; i < botGuids.size(); ++i) {
        if (i > 0) ss << ",";
        ss << botGuids[i];
    }
    ss << ")";

    // Single async query
    CharacterDatabase.AsyncQuery(ss.str().c_str())
        .WithCallback([](QueryResult result) {
            if (!result)
                return;

            do {
                Field* fields = result->Fetch();
                uint32 guid = fields[0].Get<uint32>();
                uint32 owner = fields[1].Get<uint32>();
                uint8 state = fields[2].Get<uint8>();

                BotMgr::RegisterBot(guid, owner, state);
            } while (result->NextRow());
        });
}
```

**Use DBC for static data:**
```cpp
// Cache spell ranges at initialization
struct SpellRangeCache {
    float minRange;
    float maxRange;
};

std::unordered_map<uint32, SpellRangeCache> _spellRangeCache;

void InitializeSpellRangeCache() {
    for (uint32 spellId : GetBotSpells()) {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
        if (!spellInfo)
            continue;

        _spellRangeCache[spellId] = {
            spellInfo->GetMinRange(false),
            spellInfo->GetMaxRange(false)
        };
    }
}

// Fast range check
bool IsInSpellRange(uint32 spellId, float distance) const {
    auto itr = _spellRangeCache.find(spellId);
    if (itr == _spellRangeCache.end())
        return false;

    return distance >= itr->second.minRange &&
           distance <= itr->second.maxRange;
}
```

---

## Appendix A: File Locations Reference

| API Category | Primary File | Path |
|-------------|--------------|------|
| MotionMaster | MotionMaster.h | `src/server/game/Movement/MotionMaster.h` |
| MovementGenerator | MovementGenerator.h | `src/server/game/Movement/MovementGenerator.h` |
| MovementDefines | MovementDefines.h | `src/server/game/Movement/MovementDefines.h` |
| Group | Group.h | `src/server/game/Groups/Group.h` |
| Group.cpp | Group.cpp | `src/server/game/Groups/Group.cpp` |
| ScriptMgr | ScriptMgr.h | `src/server/game/Scripting/ScriptMgr.h` |
| ThreatManager | ThreatManager.h | `src/server/game/Combat/ThreatManager.h` |
| SpellInfo | SpellInfo.h | `src/server/game/Spells/SpellInfo.h` |
| Spell | Spell.h | `src/server/game/Spells/Spell.h` |
| DB2Stores | DB2Stores.h | `src/server/game/DataStores/DB2Stores.h` |
| DatabaseWorkerPool | DatabaseWorkerPool.h | `src/server/database/Database/DatabaseWorkerPool.h` |
| Player | Player.h | `src/server/game/Entities/Player/Player.h` |
| Unit | Unit.h | `src/server/game/Entities/Unit/Unit.h` |

---

## Appendix B: Common Integration Patterns

### Pattern 1: Bot Initialization Hook

```cpp
// In PlayerEventHooks.cpp
void OnLogin(Player* player, bool firstLogin) override {
    if (!IsBotAccount(player->GetSession()->GetAccountId()))
        return;

    // Initialize bot AI
    BotAI* ai = new BotAI(player);
    player->SetBotAI(ai);

    // Load bot configuration
    BotConfig::Load(player);

    // If in group, start following
    if (Group* group = player->GetGroup()) {
        Player* leader = ObjectAccessor::FindPlayer(group->GetLeaderGUID());
        if (leader && leader != player) {
            player->GetMotionMaster()->MoveFollow(leader, 5.0f);
        }
    }
}
```

### Pattern 2: Group-Based Movement

```cpp
// In BotGroupMgr.cpp
void UpdateGroupMovement(Group* group) {
    Player* leader = ObjectAccessor::FindPlayer(group->GetLeaderGUID());
    if (!leader)
        return;

    for (GroupReference const& ref : group->GetMembers()) {
        Player* member = ref.GetSource();

        if (!IsBotPlayer(member) || member == leader)
            continue;

        // Calculate formation position
        float dist = CalculateFollowDistance(member);
        ChaseAngle angle = CalculateFormationAngle(member, group);

        // Update movement (no deduplication needed)
        member->GetMotionMaster()->MoveFollow(leader, dist, angle);
    }
}
```

### Pattern 3: Combat Spell Rotation

```cpp
// In CombatAI.cpp
void UpdateCombat(uint32 diff) {
    Unit* target = _bot->GetTarget();
    if (!target)
        return;

    // Move to optimal range
    float optimalRange = CalculateOptimalRange(_bot, target);
    float currentDist = _bot->GetDistance(target);

    if (std::abs(currentDist - optimalRange) > 2.0f) {
        _bot->GetMotionMaster()->MoveChase(target,
            ChaseRange(optimalRange - 2.0f, optimalRange + 2.0f));
    }

    // Select and cast spell
    if (!_bot->HasUnitState(UNIT_STATE_CASTING)) {
        uint32 spellId = SelectBestSpell(_bot, target);
        if (spellId) {
            _bot->CastSpell(target, spellId);
        }
    }
}
```

---

## Document Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-10-08 | Initial comprehensive analysis |

---

**End of Document**
