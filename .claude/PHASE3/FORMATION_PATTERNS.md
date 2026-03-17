# Formation Patterns Implementation Guide

## Overview

Formation patterns allow groups of bots to move together in coordinated formations, essential for dungeons, raids, and realistic group behavior.

## Formation Types

### 1. Standard Formations

```cpp
namespace Playerbot
{

enum class FormationType
{
    NONE = 0,
    FOLLOW_CHAIN,      // Single file line
    COMBAT_SPREAD,     // Spread for combat
    TIGHT_GROUP,       // Close formation for AoE healing
    RAID_FORMATION,    // Standard raid positioning
    CUSTOM             // User-defined positions
};

enum class FormationRole
{
    LEADER = 0,
    MAIN_TANK,
    OFF_TANK,
    MELEE_DPS,
    RANGED_DPS,
    HEALER
};

struct FormationPosition
{
    FormationRole role;
    float angle;        // Angle from leader (radians)
    float distance;     // Distance from leader
    float zOffset;      // Vertical offset
    uint8 priority;     // Position priority (lower = higher priority)
};

} // namespace Playerbot
```

### 2. Formation Manager (`src/modules/Playerbot/Movement/FormationMgr.h/cpp`)

```cpp
#pragma once

#include "Position.h"
#include <tbb/concurrent_hash_map.h>
#include <phmap.h>

namespace Playerbot
{

class Player;
class Group;

class TC_GAME_API Formation
{
public:
    Formation(ObjectGuid leaderGuid, FormationType type);
    ~Formation();

    // Member management
    void AddMember(Player* bot, FormationRole role);
    void RemoveMember(ObjectGuid guid);
    void UpdateMemberRole(ObjectGuid guid, FormationRole newRole);
    
    // Position calculation
    Position GetPosition(Player* bot) const;
    Position GetPosition(uint8 index) const;
    void UpdateLeaderPosition(Position const& pos);
    
    // Formation control
    void SetType(FormationType type);
    void SetSpacing(float spacing) { _spacing = spacing; }
    void SetSpread(float spread) { _spread = spread; }
    void Rotate(float angle);
    
    // Queries
    FormationType GetType() const { return _type; }
    ObjectGuid GetLeader() const { return _leaderGuid; }
    size_t GetMemberCount() const { return _members.size(); }
    bool HasMember(ObjectGuid guid) const;
    
    // Dynamic adjustments
    void AdjustForTerrain();
    void AdjustForCombat(Unit* target);
    void CompressFormation();
    void ExpandFormation();

private:
    struct Member
    {
        ObjectGuid guid;
        FormationRole role;
        uint8 positionIndex;
        Position lastPosition;
    };
    
    // Calculate positions based on formation type
    void CalculatePositions();
    Position CalculateFollowChainPosition(uint8 index) const;
    Position CalculateCombatSpreadPosition(uint8 index) const;
    Position CalculateTightGroupPosition(uint8 index) const;
    Position CalculateRaidFormationPosition(uint8 index) const;
    
    // Collision avoidance
    void ResolveCollisions();
    bool CheckCollision(Position const& pos, float radius = 2.0f) const;

private:
    ObjectGuid _leaderGuid;
    FormationType _type;
    Position _leaderPosition;
    float _leaderOrientation;
    
    // Members
    phmap::parallel_flat_hash_map<ObjectGuid, Member> _members;
    std::vector<FormationPosition> _positions;
    
    // Configuration
    float _spacing;      // Base spacing between members
    float _spread;       // Spread multiplier for combat
    float _rotation;     // Formation rotation
    
    // State
    uint32 _lastUpdateTime;
    bool _needsRecalculation;
};

class TC_GAME_API FormationMgr
{
    FormationMgr();
    ~FormationMgr();
    
public:
    static FormationMgr* instance();
    
    // Formation management
    Formation* CreateFormation(Group* group, FormationType type);
    Formation* GetFormation(ObjectGuid leaderGuid);
    void DeleteFormation(ObjectGuid leaderGuid);
    
    // Update all formations
    void Update(uint32 diff);
    
    // Preset configurations
    void LoadFormationTemplates();
    FormationPosition const* GetTemplate(FormationType type, FormationRole role) const;
    
    // Statistics
    struct Stats
    {
        uint32 activeFormations;
        uint32 totalMembers;
        float averageSpacing;
    };
    Stats GetStats() const;

private:
    // Formation templates
    void InitializeTemplates();
    void LoadCustomTemplates();

private:
    // Active formations
    tbb::concurrent_hash_map<ObjectGuid, std::unique_ptr<Formation>> _formations;
    
    // Templates
    std::map<std::pair<FormationType, FormationRole>, FormationPosition> _templates;
    
    // Update timer
    uint32 _updateTimer;
};

#define sFormationMgr FormationMgr::instance()

} // namespace Playerbot
```

## Formation Implementations

### 3. Group Movement AI (`src/modules/Playerbot/Movement/GroupMovementAI.h/cpp`)

```cpp
#pragma once

#include "Formation.h"
#include "BotMovementAI.h"

namespace Playerbot
{

class TC_GAME_API GroupMovementAI
{
public:
    GroupMovementAI(Group* group);
    ~GroupMovementAI();
    
    // Formation control
    void SetFormation(FormationType type);
    void ChangeFormation(FormationType newType, uint32 transitionTimeMs = 2000);
    
    // Movement commands
    void MoveGroup(Position const& destination);
    void FollowLeader(Player* leader);
    void SpreadOut(float radius = 10.0f);
    void GatherUp(float radius = 5.0f);
    
    // Combat formations
    void EngageTarget(Unit* target);
    void DefensiveFormation();
    void OffensiveFormation();
    
    // Special movements
    void NavigateNarrowPath();
    void CrossWater();
    void AvoidAoE(Position const& center, float radius);
    
    // Updates
    void Update(uint32 diff);
    
private:
    // Internal formation management
    void AssignRoles();
    void UpdateMemberPositions();
    void SynchronizeMovement();
    
    // Path coordination
    void CalculateGroupPath(Position const& destination);
    void StaggerMovement(uint32 delayMs = 500);
    
    // Collision resolution
    void ResolveInternalCollisions();
    void AvoidExternalObstacles();

private:
    Group* _group;
    Formation* _formation;
    
    // Movement state
    Position _groupDestination;
    bool _isMoving;
    uint32 _moveStartTime;
    
    // Formation transition
    FormationType _targetFormationType;
    uint32 _transitionTimer;
    float _transitionProgress;
    
    // Member tracking
    struct MemberState
    {
        Player* bot;
        FormationRole role;
        Position targetPosition;
        bool inPosition;
        uint32 lastUpdateTime;
    };
    
    std::vector<MemberState> _members;
};

} // namespace Playerbot
```

## Formation Position Calculations

### Follow Chain Formation

```cpp
Position Formation::CalculateFollowChainPosition(uint8 index) const
{
    // Single file behind leader
    Position pos = _leaderPosition;
    float distancePerMember = _spacing * 1.5f; // Slightly spread
    
    // Calculate position behind previous member
    pos.RelocateOffset(
        -distancePerMember * (index + 1) * cos(_leaderOrientation),
        -distancePerMember * (index + 1) * sin(_leaderOrientation),
        0.0f
    );
    
    return pos;
}
```

### Combat Spread Formation

```cpp
Position Formation::CalculateCombatSpreadPosition(uint8 index) const
{
    Position pos = _leaderPosition;
    
    // Determine role-based positioning
    Member const& member = GetMemberByIndex(index);
    
    switch (member.role)
    {
        case FormationRole::MAIN_TANK:
            // Front center
            pos.RelocateOffset(5.0f, 0.0f, 0.0f);
            break;
            
        case FormationRole::OFF_TANK:
            // Front side
            pos.RelocateOffset(4.0f, 3.0f, 0.0f);
            break;
            
        case FormationRole::MELEE_DPS:
        {
            // Behind/side of target in arc
            float meleeAngle = M_PI + (M_PI/4) * (index % 4);
            pos.RelocatePolarOffset(meleeAngle, 3.0f * _spread);
            break;
        }
        
        case FormationRole::RANGED_DPS:
        {
            // Spread in back arc
            float rangedAngle = M_PI + (M_PI/3) + (M_PI/6) * (index % 6);
            pos.RelocatePolarOffset(rangedAngle, 25.0f * _spread);
            break;
        }
        
        case FormationRole::HEALER:
        {
            // Max range, spread evenly
            float healerAngle = (2 * M_PI / GetHealerCount()) * GetHealerIndex(member.guid);
            pos.RelocatePolarOffset(healerAngle, 30.0f * _spread);
            break;
        }
    }
    
    return pos;
}
```

### Raid Formation

```cpp
Position Formation::CalculateRaidFormationPosition(uint8 index) const
{
    // Standard raid positioning: tanks front, melee behind, ranged/healers back
    Position pos = _leaderPosition;
    Member const& member = GetMemberByIndex(index);
    
    // Grid-based positioning
    struct GridPosition
    {
        int8 row;    // 0 = front, positive = behind
        int8 column; // 0 = center, negative = left, positive = right
    };
    
    GridPosition grid;
    
    switch (member.role)
    {
        case FormationRole::MAIN_TANK:
            grid = {0, 0};
            break;
            
        case FormationRole::OFF_TANK:
            grid = {0, 2};
            break;
            
        case FormationRole::MELEE_DPS:
            // Form rows behind tanks
            grid.row = 1 + (GetMeleeIndex(member.guid) / 5);
            grid.column = (GetMeleeIndex(member.guid) % 5) - 2;
            break;
            
        case FormationRole::RANGED_DPS:
            // Spread in back rows
            grid.row = 4 + (GetRangedIndex(member.guid) / 6);
            grid.column = (GetRangedIndex(member.guid) % 6) - 3;
            break;
            
        case FormationRole::HEALER:
            // Corners and back center
            grid.row = 5;
            grid.column = (GetHealerIndex(member.guid) - GetHealerCount()/2) * 3;
            break;
    }
    
    // Apply grid position with spacing
    float rowSpacing = _spacing * 2.0f;
    float columnSpacing = _spacing * 1.5f;
    
    pos.RelocateOffset(
        -grid.row * rowSpacing * cos(_leaderOrientation) + grid.column * columnSpacing * sin(_leaderOrientation),
        -grid.row * rowSpacing * sin(_leaderOrientation) - grid.column * columnSpacing * cos(_leaderOrientation),
        0.0f
    );
    
    // Apply rotation if set
    if (_rotation != 0.0f)
    {
        float dist = pos.GetExactDist2d(_leaderPosition);
        float angle = _leaderPosition.GetAngle(&pos) + _rotation;
        pos = _leaderPosition;
        pos.RelocatePolarOffset(angle, dist);
    }
    
    return pos;
}
```

## Dynamic Formation Adjustments

### Terrain Adjustment

```cpp
void Formation::AdjustForTerrain()
{
    for (auto& [guid, member] : _members)
    {
        Player* bot = ObjectAccessor::FindPlayer(guid);
        if (!bot)
            continue;
        
        Position targetPos = GetPosition(member.positionIndex);
        
        // Check for obstacles
        if (!bot->GetMap()->isInLineOfSight(
            bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ() + 2.0f,
            targetPos.GetPositionX(), targetPos.GetPositionY(), targetPos.GetPositionZ() + 2.0f))
        {
            // Find alternative position
            for (float angle = 0; angle < 2 * M_PI; angle += M_PI / 8)
            {
                Position altPos = targetPos;
                altPos.RelocatePolarOffset(angle, 3.0f);
                
                // Update Z to ground level
                float groundZ = bot->GetMap()->GetHeight(
                    bot->GetPhaseShift(),
                    altPos.GetPositionX(),
                    altPos.GetPositionY(),
                    altPos.GetPositionZ()
                );
                altPos.m_positionZ = groundZ + 0.5f;
                
                if (bot->GetMap()->isInLineOfSight(
                    bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ() + 2.0f,
                    altPos.GetPositionX(), altPos.GetPositionY(), altPos.GetPositionZ() + 2.0f))
                {
                    member.lastPosition = altPos;
                    break;
                }
            }
        }
        else
        {
            // Update Z to ground level
            float groundZ = bot->GetMap()->GetHeight(
                bot->GetPhaseShift(),
                targetPos.GetPositionX(),
                targetPos.GetPositionY(),
                targetPos.GetPositionZ()
            );
            targetPos.m_positionZ = groundZ + 0.5f;
            member.lastPosition = targetPos;
        }
    }
}
```

### Combat Adjustment

```cpp
void Formation::AdjustForCombat(Unit* target)
{
    if (!target)
    {
        SetType(FormationType::FOLLOW_CHAIN);
        return;
    }
    
    // Switch to combat formation
    SetType(FormationType::COMBAT_SPREAD);
    
    // Adjust spread based on target type
    if (target->GetTypeId() == TYPEID_UNIT)
    {
        Creature* creature = target->ToCreature();
        
        // Check for cleave/AoE abilities
        if (creature->HasSpellInSpellbook(SPELL_CLEAVE) ||
            creature->HasSpellInSpellbook(SPELL_WHIRLWIND))
        {
            // Increase melee spread
            _spread = 1.5f;
        }
        
        // Check for raid boss
        if (creature->isWorldBoss() || creature->IsDungeonBoss())
        {
            // Use raid formation
            SetType(FormationType::RAID_FORMATION);
            _spacing = 5.0f; // Increase spacing
        }
    }
    
    // Face formation toward target
    _leaderOrientation = _leaderPosition.GetAngle(target);
    
    CalculatePositions();
}
```

## Formation Transitions

### Smooth Formation Change

```cpp
void GroupMovementAI::ChangeFormation(FormationType newType, uint32 transitionTimeMs)
{
    if (_formation->GetType() == newType)
        return;
    
    _targetFormationType = newType;
    _transitionTimer = 0;
    _transitionProgress = 0.0f;
    
    // Store current positions
    for (auto& member : _members)
    {
        member.startPosition = member.bot->GetPosition();
        
        // Calculate target position in new formation
        _formation->SetType(newType);
        member.targetPosition = _formation->GetPosition(member.bot);
        _formation->SetType(_currentFormationType); // Restore for transition
    }
    
    // Start transition
    _isTransitioning = true;
    _transitionDuration = transitionTimeMs;
}

void GroupMovementAI::Update(uint32 diff)
{
    if (_isTransitioning)
    {
        _transitionTimer += diff;
        _transitionProgress = float(_transitionTimer) / float(_transitionDuration);
        
        if (_transitionProgress >= 1.0f)
        {
            // Transition complete
            _formation->SetType(_targetFormationType);
            _isTransitioning = false;
        }
        else
        {
            // Interpolate positions
            for (auto& member : _members)
            {
                Position currentPos;
                
                // Smooth interpolation
                float t = SmoothStep(0.0f, 1.0f, _transitionProgress);
                
                currentPos.m_positionX = member.startPosition.GetPositionX() + 
                    (member.targetPosition.GetPositionX() - member.startPosition.GetPositionX()) * t;
                currentPos.m_positionY = member.startPosition.GetPositionY() + 
                    (member.targetPosition.GetPositionY() - member.startPosition.GetPositionY()) * t;
                currentPos.m_positionZ = member.startPosition.GetPositionZ() + 
                    (member.targetPosition.GetPositionZ() - member.startPosition.GetPositionZ()) * t;
                
                member.bot->GetBotMovementAI()->MoveTo(currentPos, MovementPriority::FORCED);
            }
        }
    }
}

float GroupMovementAI::SmoothStep(float edge0, float edge1, float x)
{
    // Smooth hermite interpolation
    x = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}
```

## Performance Considerations

### Parallel Position Updates

```cpp
void FormationMgr::Update(uint32 diff)
{
    // Update all formations in parallel
    std::vector<Formation*> formations;
    
    // Collect active formations
    for (auto& [guid, formation] : _formations)
    {
        formations.push_back(formation.get());
    }
    
    // Parallel update using TBB
    tbb::parallel_for(
        tbb::blocked_range<size_t>(0, formations.size()),
        [&](tbb::blocked_range<size_t> const& range)
        {
            for (size_t i = range.begin(); i != range.end(); ++i)
            {
                formations[i]->AdjustForTerrain();
                formations[i]->ResolveCollisions();
            }
        }
    );
}
```

### Collision Optimization

```cpp
void Formation::ResolveCollisions()
{
    // Spatial hashing for collision detection
    phmap::flat_hash_map<uint32, std::vector<ObjectGuid>> spatialHash;
    
    float cellSize = 5.0f; // 5 yard cells
    
    // Hash all members
    for (auto const& [guid, member] : _members)
    {
        Player* bot = ObjectAccessor::FindPlayer(guid);
        if (!bot)
            continue;
        
        uint32 hashKey = GetSpatialHash(bot->GetPosition(), cellSize);
        spatialHash[hashKey].push_back(guid);
    }
    
    // Check collisions only within same and adjacent cells
    for (auto& [guid, member] : _members)
    {
        Player* bot = ObjectAccessor::FindPlayer(guid);
        if (!bot)
            continue;
        
        Position pos = member.lastPosition;
        uint32 hashKey = GetSpatialHash(pos, cellSize);
        
        // Check surrounding cells
        for (int dx = -1; dx <= 1; ++dx)
        {
            for (int dy = -1; dy <= 1; ++dy)
            {
                uint32 checkKey = hashKey + dx + dy * 1000;
                
                auto it = spatialHash.find(checkKey);
                if (it == spatialHash.end())
                    continue;
                
                for (ObjectGuid otherGuid : it->second)
                {
                    if (guid == otherGuid)
                        continue;
                    
                    Player* other = ObjectAccessor::FindPlayer(otherGuid);
                    if (!other)
                        continue;
                    
                    float dist = pos.GetExactDist(other->GetPosition());
                    if (dist < 2.0f) // Too close
                    {
                        // Push apart
                        float angle = pos.GetAngle(other->GetPosition());
                        pos.RelocatePolarOffset(angle + M_PI, 2.0f - dist);
                        member.lastPosition = pos;
                    }
                }
            }
        }
    }
}

uint32 Formation::GetSpatialHash(Position const& pos, float cellSize) const
{
    int32 gridX = int32(pos.GetPositionX() / cellSize);
    int32 gridY = int32(pos.GetPositionY() / cellSize);
    
    // Simple spatial hash
    return (gridX & 0xFFFF) | ((gridY & 0xFFFF) << 16);
}
```

## Testing

```cpp
TEST_F(FormationTest, BasicFormation)
{
    auto group = CreateTestGroup(5);
    auto formation = sFormationMgr->CreateFormation(group, FormationType::FOLLOW_CHAIN);
    
    // Test member positions
    for (int i = 0; i < 5; ++i)
    {
        Position pos = formation->GetPosition(i);
        
        // Should be in a line
        if (i > 0)
        {
            Position prevPos = formation->GetPosition(i - 1);
            float dist = pos.GetExactDist(prevPos);
            EXPECT_NEAR(dist, formation->GetSpacing() * 1.5f, 0.1f);
        }
    }
}

TEST_F(FormationTest, CombatFormation)
{
    auto group = CreateTestRaidGroup(25);
    auto formation = sFormationMgr->CreateFormation(group, FormationType::RAID_FORMATION);
    
    // Create mock target
    auto target = CreateTestCreature();
    formation->AdjustForCombat(target);
    
    // Verify tank positioning
    auto tank = GetMemberByRole(FormationRole::MAIN_TANK);
    float tankDist = tank->GetExactDist(target);
    EXPECT_LT(tankDist, 10.0f); // Tank should be close
    
    // Verify healer positioning  
    auto healer = GetMemberByRole(FormationRole::HEALER);
    float healerDist = healer->GetExactDist(target);
    EXPECT_GT(healerDist, 25.0f); // Healers should be at range
}

TEST_F(FormationTest, FormationTransition)
{
    auto group = CreateTestGroup(5);
    auto movement = std::make_unique<GroupMovementAI>(group);
    
    // Start with follow formation
    movement->SetFormation(FormationType::FOLLOW_CHAIN);
    
    // Transition to combat
    movement->ChangeFormation(FormationType::COMBAT_SPREAD, 2000);
    
    // Simulate updates
    for (uint32 time = 0; time < 2000; time += 100)
    {
        movement->Update(100);
    }
    
    // Verify final formation
    EXPECT_EQ(movement->GetFormationType(), FormationType::COMBAT_SPREAD);
}
```

---

**Next**: Implement [POSITIONING_STRATEGIES.md](POSITIONING_STRATEGIES.md) for dynamic combat positioning