# Movement and Pathfinding System - Phase 3 Sprint 3C
## Intelligent Navigation and Positioning

## Overview

This document details the implementation of advanced movement and pathfinding systems for bots, including A* pathfinding integration, formation movement, obstacle avoidance, and dynamic positioning during combat.

## Core Pathfinding System

### A* Pathfinding Integration

```cpp
// src/modules/Playerbot/AI/Movement/PathfindingManager.h

class PathfindingManager {
public:
    struct PathNode {
        G3D::Vector3 position;
        float g_cost;  // Distance from start
        float h_cost;  // Heuristic distance to end
        float f_cost() const { return g_cost + h_cost; }
        PathNode* parent;
        
        bool operator<(const PathNode& other) const {
            return f_cost() > other.f_cost();  // For priority queue
        }
    };
    
    using Path = std::vector<G3D::Vector3>;
    
    PathfindingManager(Player* bot) : _bot(bot) {}
    
    // Main pathfinding
    Path FindPath(const G3D::Vector3& start, const G3D::Vector3& end);
    Path FindPathAroundObstacles(const G3D::Vector3& start, const G3D::Vector3& end);
    Path FindCoverPath(Unit* threat);
    
    // Path optimization
    Path SmoothPath(const Path& path);
    Path SimplifyPath(const Path& path);
    
    // Path validation
    bool IsPathClear(const Path& path) const;
    bool HasLineOfSight(const G3D::Vector3& from, const G3D::Vector3& to) const;
    
    // Dynamic obstacles
    void AddTemporaryObstacle(const G3D::Vector3& center, float radius, uint32 durationMs);
    void UpdateObstacles(uint32 diff);
    
private:
    Player* _bot;
    
    struct Obstacle {
        G3D::Vector3 center;
        float radius;
        uint32 expiresAt;
    };
    
    std::vector<Obstacle> _temporaryObstacles;
    
    // A* implementation
    Path AStar(const G3D::Vector3& start, const G3D::Vector3& end) {
        std::priority_queue<PathNode> openSet;
        phmap::flat_hash_set<uint64> closedSet;
        phmap::flat_hash_map<uint64, PathNode> allNodes;
        
        // Initialize start node
        PathNode startNode;
        startNode.position = start;
        startNode.g_cost = 0;
        startNode.h_cost = (end - start).length();
        startNode.parent = nullptr;
        
        openSet.push(startNode);
        allNodes[HashPosition(start)] = startNode;
        
        while (!openSet.empty()) {
            PathNode current = openSet.top();
            openSet.pop();
            
            // Check if reached goal
            if ((current.position - end).length() < 1.0f) {
                return ReconstructPath(current, allNodes);
            }
            
            closedSet.insert(HashPosition(current.position));
            
            // Check neighbors
            for (const auto& neighbor : GetNeighbors(current.position)) {
                if (closedSet.count(HashPosition(neighbor)))
                    continue;
                
                if (!IsWalkable(neighbor))
                    continue;
                
                float tentative_g = current.g_cost + (neighbor - current.position).length();
                
                auto it = allNodes.find(HashPosition(neighbor));
                if (it == allNodes.end() || tentative_g < it->second.g_cost) {
                    PathNode neighborNode;
                    neighborNode.position = neighbor;
                    neighborNode.g_cost = tentative_g;
                    neighborNode.h_cost = (end - neighbor).length();
                    neighborNode.parent = &allNodes[HashPosition(current.position)];
                    
                    allNodes[HashPosition(neighbor)] = neighborNode;
                    openSet.push(neighborNode);
                }
            }
        }
        
        return Path();  // No path found
    }
    
    std::vector<G3D::Vector3> GetNeighbors(const G3D::Vector3& pos) {
        std::vector<G3D::Vector3> neighbors;
        const float stepSize = 2.0f;
        
        // 8-directional movement
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                if (dx == 0 && dy == 0)
                    continue;
                
                G3D::Vector3 neighbor = pos;
                neighbor.x += dx * stepSize;
                neighbor.y += dy * stepSize;
                
                // Adjust Z for terrain
                neighbor.z = _bot->GetMap()->GetHeight(neighbor.x, neighbor.y, neighbor.z);
                
                neighbors.push_back(neighbor);
            }
        }
        
        return neighbors;
    }
    
    bool IsWalkable(const G3D::Vector3& pos) {
        // Check terrain
        if (!_bot->IsWithinLOS(pos.x, pos.y, pos.z))
            return false;
        
        // Check temporary obstacles
        for (const auto& obstacle : _temporaryObstacles) {
            if ((pos - obstacle.center).length() < obstacle.radius)
                return false;
        }
        
        // Check for steep slopes
        float slope = CalculateSlope(pos);
        if (slope > 60.0f)  // degrees
            return false;
        
        return true;
    }
    
    uint64 HashPosition(const G3D::Vector3& pos) {
        // Grid-based hashing for position
        int32 gridX = int32(pos.x / 1.0f);
        int32 gridY = int32(pos.y / 1.0f);
        return (uint64(gridX) << 32) | uint64(gridY);
    }
    
    Path ReconstructPath(const PathNode& end, const phmap::flat_hash_map<uint64, PathNode>& nodes) {
        Path path;
        const PathNode* current = &end;
        
        while (current != nullptr) {
            path.push_back(current->position);
            current = current->parent;
        }
        
        std::reverse(path.begin(), path.end());
        return path;
    }
};
```

## Movement Controller

### Advanced Movement Execution

```cpp
// src/modules/Playerbot/AI/Movement/MovementController.h

class MovementController {
public:
    enum MovementState {
        MOVEMENT_IDLE,
        MOVEMENT_FOLLOWING,
        MOVEMENT_PURSUING,
        MOVEMENT_FLEEING,
        MOVEMENT_POSITIONING,
        MOVEMENT_PATROLLING
    };
    
    enum MovementSpeed {
        SPEED_WALK = 0,
        SPEED_RUN = 1,
        SPEED_MOUNT = 2,
        SPEED_FLIGHT = 3
    };
    
    MovementController(Player* bot) : _bot(bot), _pathfinder(bot) {}
    
    // Movement commands
    void MoveTo(const G3D::Vector3& destination, MovementSpeed speed = SPEED_RUN);
    void Follow(Unit* target, float distance = 3.0f);
    void Flee(Unit* threat, float safeDistance = 20.0f);
    void Strafe(Unit* target, bool left = true);
    void BackPedal(float distance = 5.0f);
    
    // Complex movements
    void Kite(Unit* target, float distance = 8.0f);
    void Circle(Unit* target, float radius = 10.0f);
    void Charge(Unit* target);
    void Intercept(Unit* target);
    
    // Formation movement
    void MoveInFormation(Formation* formation, uint8 position);
    void MaintainFormation();
    
    // Path execution
    void UpdateMovement(uint32 diff);
    void StopMovement();
    bool IsMoving() const { return _state != MOVEMENT_IDLE; }
    
    // Jump system
    bool ShouldJump(const G3D::Vector3& obstacle);
    void Jump();
    
private:
    Player* _bot;
    PathfindingManager _pathfinder;
    MovementState _state = MOVEMENT_IDLE;
    
    // Current path
    PathfindingManager::Path _currentPath;
    size_t _currentWaypoint = 0;
    
    // Follow target
    Unit* _followTarget = nullptr;
    float _followDistance = 3.0f;
    
    // Movement timing
    uint32 _lastMovementUpdate = 0;
    uint32 _stuckTimer = 0;
    G3D::Vector3 _lastPosition;
    
    void ExecutePath() {
        if (_currentPath.empty() || _currentWaypoint >= _currentPath.size()) {
            StopMovement();
            return;
        }
        
        const G3D::Vector3& waypoint = _currentPath[_currentWaypoint];
        float distance = (_bot->GetPosition() - waypoint).length();
        
        // Reached waypoint
        if (distance < 2.0f) {
            _currentWaypoint++;
            if (_currentWaypoint >= _currentPath.size()) {
                StopMovement();
                return;
            }
        }
        
        // Move to waypoint
        float angle = _bot->GetAngle(waypoint.x, waypoint.y);
        _bot->SetFacingTo(angle);
        
        // Use appropriate movement generator
        if (_state == MOVEMENT_PURSUING) {
            _bot->GetMotionMaster()->MoveChase(_followTarget);
        } else {
            _bot->GetMotionMaster()->MovePoint(0, waypoint);
        }
        
        // Stuck detection
        float movedDistance = (_bot->GetPosition() - _lastPosition).length();
        if (movedDistance < 0.5f) {
            _stuckTimer += diff;
            if (_stuckTimer > 3000) {
                HandleStuck();
            }
        } else {
            _stuckTimer = 0;
            _lastPosition = _bot->GetPosition();
        }
    }
    
    void HandleStuck() {
        TC_LOG_DEBUG("playerbot", "Bot %s is stuck, recalculating path", 
                     _bot->GetName().c_str());
        
        // Try jumping first
        if (ShouldJump(_currentPath[_currentWaypoint])) {
            Jump();
            return;
        }
        
        // Recalculate path
        if (_followTarget) {
            _currentPath = _pathfinder.FindPath(
                _bot->GetPosition(), 
                _followTarget->GetPosition()
            );
        } else if (!_currentPath.empty()) {
            _currentPath = _pathfinder.FindPath(
                _bot->GetPosition(),
                _currentPath.back()
            );
        }
        
        _currentWaypoint = 0;
        _stuckTimer = 0;
    }
};
```

## Formation System

### Group Formation Movement

```cpp
// src/modules/Playerbot/AI/Movement/FormationManager.h

class FormationManager {
public:
    enum FormationType {
        FORMATION_LINE,
        FORMATION_ARROW,
        FORMATION_CIRCLE,
        FORMATION_SCATTER,
        FORMATION_CUSTOM
    };
    
    struct FormationPosition {
        float relativeX;
        float relativeY;
        float relativeAngle;
        BotRole preferredRole;
    };
    
    class Formation {
    public:
        Formation(FormationType type) : _type(type) {
            InitializePositions();
        }
        
        G3D::Vector3 GetPosition(uint8 slot, const G3D::Vector3& anchor, float facing) {
            if (slot >= _positions.size())
                return anchor;
            
            const FormationPosition& pos = _positions[slot];
            
            float angle = facing + pos.relativeAngle;
            float x = anchor.x + cos(angle) * pos.relativeX;
            float y = anchor.y + sin(angle) * pos.relativeY;
            float z = anchor.z;
            
            return G3D::Vector3(x, y, z);
        }
        
        uint8 AssignSlot(BotRole role) {
            // Find best slot for role
            for (uint8 i = 0; i < _positions.size(); ++i) {
                if (_positions[i].preferredRole == role && !IsSlotTaken(i)) {
                    _takenSlots.insert(i);
                    return i;
                }
            }
            
            // Take any available slot
            for (uint8 i = 0; i < _positions.size(); ++i) {
                if (!IsSlotTaken(i)) {
                    _takenSlots.insert(i);
                    return i;
                }
            }
            
            return 0;
        }
        
    private:
        FormationType _type;
        std::vector<FormationPosition> _positions;
        phmap::flat_hash_set<uint8> _takenSlots;
        
        void InitializePositions() {
            switch (_type) {
                case FORMATION_ARROW:
                    // Tank in front
                    _positions.push_back({0, 0, 0, ROLE_TANK});
                    // Melee DPS on sides
                    _positions.push_back({-3, -3, 0, ROLE_DPS});
                    _positions.push_back({3, -3, 0, ROLE_DPS});
                    // Ranged DPS behind
                    _positions.push_back({-5, -8, 0, ROLE_DPS});
                    _positions.push_back({5, -8, 0, ROLE_DPS});
                    // Healers in back
                    _positions.push_back({0, -10, 0, ROLE_HEALER});
                    break;
                    
                case FORMATION_CIRCLE:
                    float radius = 8.0f;
                    for (int i = 0; i < 8; ++i) {
                        float angle = (2 * M_PI * i) / 8;
                        _positions.push_back({
                            radius * cos(angle),
                            radius * sin(angle),
                            angle,
                            ROLE_DPS
                        });
                    }
                    break;
            }
        }
        
        bool IsSlotTaken(uint8 slot) const {
            return _takenSlots.count(slot) > 0;
        }
    };
    
    FormationManager(Group* group) : _group(group) {}
    
    // Formation management
    void SetFormation(FormationType type) {
        _currentFormation = std::make_unique<Formation>(type);
        AssignFormationSlots();
    }
    
    void UpdateFormation() {
        if (!_currentFormation)
            return;
        
        Unit* leader = GetFormationAnchor();
        if (!leader)
            return;
        
        G3D::Vector3 anchorPos = leader->GetPosition();
        float facing = leader->GetOrientation();
        
        // Move bots to formation positions
        for (const auto& [bot, slot] : _assignments) {
            G3D::Vector3 targetPos = _currentFormation->GetPosition(
                slot, anchorPos, facing);
            
            float distance = (bot->GetPosition() - targetPos).length();
            if (distance > 2.0f) {
                MovementController* controller = GetMovementController(bot);
                controller->MoveTo(targetPos);
            }
        }
    }
    
private:
    Group* _group;
    std::unique_ptr<Formation> _currentFormation;
    phmap::flat_hash_map<Player*, uint8> _assignments;
    
    void AssignFormationSlots() {
        _assignments.clear();
        
        for (GroupReference* ref = _group->GetFirstMember(); 
             ref != nullptr; ref = ref->next()) {
            if (Player* member = ref->GetSource()) {
                if (member->IsBot()) {
                    BotRole role = GetBotRole(member);
                    uint8 slot = _currentFormation->AssignSlot(role);
                    _assignments[member] = slot;
                }
            }
        }
    }
    
    Unit* GetFormationAnchor() {
        // Use tank as anchor, or first player
        for (GroupReference* ref = _group->GetFirstMember(); 
             ref != nullptr; ref = ref->next()) {
            if (Player* member = ref->GetSource()) {
                if (!member->IsBot() || GetBotRole(member) == ROLE_TANK)
                    return member;
            }
        }
        return nullptr;
    }
};
```

## Obstacle Avoidance

### Dynamic Obstacle Detection

```cpp
// src/modules/Playerbot/AI/Movement/ObstacleAvoidance.h

class ObstacleAvoidance {
public:
    struct Obstacle {
        enum Type {
            STATIC_OBJECT,
            DYNAMIC_OBJECT,
            UNIT,
            SPELL_AREA,
            ENVIRONMENTAL
        };
        
        Type type;
        G3D::Vector3 position;
        float radius;
        float height;
        bool isMoving;
        G3D::Vector3 velocity;
    };
    
    ObstacleAvoidance(Player* bot) : _bot(bot) {}
    
    // Obstacle detection
    std::vector<Obstacle> DetectObstacles(float range = 20.0f);
    bool HasObstacleInPath(const G3D::Vector3& destination);
    
    // Avoidance calculation
    G3D::Vector3 CalculateAvoidanceVector(const std::vector<Obstacle>& obstacles);
    PathfindingManager::Path GetPathAroundObstacles(
        const G3D::Vector3& start,
        const G3D::Vector3& end,
        const std::vector<Obstacle>& obstacles
    );
    
    // Specific avoidance
    void AvoidSpellArea(uint32 spellId, const G3D::Vector3& center, float radius);
    void AvoidUnit(Unit* unit, float safeDistance = 5.0f);
    
private:
    Player* _bot;
    
    std::vector<Obstacle> ScanEnvironment(float range) {
        std::vector<Obstacle> obstacles;
        
        // Scan for game objects
        std::list<GameObject*> objects;
        _bot->GetGameObjectListWithEntryInGrid(objects, 0, range);
        
        for (GameObject* obj : objects) {
            if (IsObstacle(obj)) {
                Obstacle obs;
                obs.type = Obstacle::STATIC_OBJECT;
                obs.position = obj->GetPosition();
                obs.radius = obj->GetObjectSize();
                obstacles.push_back(obs);
            }
        }
        
        // Scan for dynamic objects (spell areas)
        std::list<DynamicObject*> dynObjects;
        _bot->GetDynamicObjectList(dynObjects, range);
        
        for (DynamicObject* dyn : dynObjects) {
            if (IsHazardous(dyn->GetSpellId())) {
                Obstacle obs;
                obs.type = Obstacle::SPELL_AREA;
                obs.position = dyn->GetPosition();
                obs.radius = dyn->GetRadius();
                obstacles.push_back(obs);
            }
        }
        
        // Scan for units
        std::list<Unit*> units;
        _bot->GetUnitList(units, range);
        
        for (Unit* unit : units) {
            if (ShouldAvoid(unit)) {
                Obstacle obs;
                obs.type = Obstacle::UNIT;
                obs.position = unit->GetPosition();
                obs.radius = unit->GetCombatReach() + 2.0f;
                obs.isMoving = unit->IsMoving();
                if (obs.isMoving) {
                    obs.velocity = unit->GetVelocity();
                }
                obstacles.push_back(obs);
            }
        }
        
        return obstacles;
    }
    
    bool IsObstacle(GameObject* obj) {
        // Check if object blocks movement
        return obj->GetGoType() == GAMEOBJECT_TYPE_WALL ||
               obj->GetGoType() == GAMEOBJECT_TYPE_DESTRUCTIBLE_BUILDING;
    }
    
    bool IsHazardous(uint32 spellId) {
        static phmap::flat_hash_set<uint32> hazardousSpells = {
            SPELL_VOID_ZONE,
            SPELL_DEATH_AND_DECAY,
            SPELL_RAIN_OF_FIRE,
            SPELL_BLIZZARD,
            SPELL_CONSECRATION,
            // ... more hazardous spells
        };
        return hazardousSpells.count(spellId) > 0;
    }
    
    bool ShouldAvoid(Unit* unit) {
        // Avoid large enemies
        if (unit->GetCreatureType() == CREATURE_TYPE_GIANT)
            return true;
        
        // Avoid units with dangerous auras
        if (unit->HasAura(SPELL_WHIRLWIND))
            return true;
        
        return false;
    }
};
```

## Performance Optimization

### Path Caching System

```cpp
// src/modules/Playerbot/AI/Movement/PathCache.h

class PathCache {
public:
    struct CachedPath {
        PathfindingManager::Path path;
        uint32 timestamp;
        float confidence;
    };
    
    // Cache management
    void CachePath(const G3D::Vector3& start, const G3D::Vector3& end, 
                   const PathfindingManager::Path& path) {
        uint64 key = HashEndpoints(start, end);
        
        CachedPath cached;
        cached.path = path;
        cached.timestamp = getMSTime();
        cached.confidence = 1.0f;
        
        _cache[key] = cached;
        
        // Limit cache size
        if (_cache.size() > MAX_CACHE_SIZE) {
            RemoveOldest();
        }
    }
    
    bool GetCachedPath(const G3D::Vector3& start, const G3D::Vector3& end,
                       PathfindingManager::Path& path) {
        uint64 key = HashEndpoints(start, end);
        auto it = _cache.find(key);
        
        if (it == _cache.end())
            return false;
        
        // Check if path is still valid
        uint32 age = getMSTime() - it->second.timestamp;
        if (age > MAX_PATH_AGE) {
            _cache.erase(it);
            return false;
        }
        
        path = it->second.path;
        return true;
    }
    
private:
    static constexpr size_t MAX_CACHE_SIZE = 100;
    static constexpr uint32 MAX_PATH_AGE = 30000;  // 30 seconds
    
    phmap::flat_hash_map<uint64, CachedPath> _cache;
    
    uint64 HashEndpoints(const G3D::Vector3& start, const G3D::Vector3& end) {
        // Simple spatial hashing
        uint32 startHash = (uint32(start.x) << 16) | uint32(start.y);
        uint32 endHash = (uint32(end.x) << 16) | uint32(end.y);
        return (uint64(startHash) << 32) | endHash;
    }
    
    void RemoveOldest() {
        if (_cache.empty())
            return;
        
        auto oldest = _cache.begin();
        uint32 oldestTime = oldest->second.timestamp;
        
        for (auto it = _cache.begin(); it != _cache.end(); ++it) {
            if (it->second.timestamp < oldestTime) {
                oldest = it;
                oldestTime = it->second.timestamp;
            }
        }
        
        _cache.erase(oldest);
    }
};
```

## Testing Framework

```cpp
// src/modules/Playerbot/Tests/MovementTest.cpp

TEST_F(MovementTest, PathfindingBasic) {
    auto bot = CreateTestBot();
    PathfindingManager pathfinder(bot);
    
    G3D::Vector3 start(0, 0, 0);
    G3D::Vector3 end(100, 100, 0);
    
    auto path = pathfinder.FindPath(start, end);
    
    ASSERT_FALSE(path.empty());
    EXPECT_EQ(path.front(), start);
    EXPECT_NEAR(path.back().x, end.x, 1.0f);
    EXPECT_NEAR(path.back().y, end.y, 1.0f);
}

TEST_F(MovementTest, ObstacleAvoidance) {
    auto bot = CreateTestBot();
    PathfindingManager pathfinder(bot);
    
    // Add obstacle in direct path
    pathfinder.AddTemporaryObstacle(G3D::Vector3(50, 50, 0), 10.0f, 60000);
    
    G3D::Vector3 start(0, 0, 0);
    G3D::Vector3 end(100, 100, 0);
    
    auto path = pathfinder.FindPath(start, end);
    
    // Path should go around obstacle
    bool avoidsObstacle = true;
    for (const auto& point : path) {
        float distToObstacle = (point - G3D::Vector3(50, 50, 0)).length();
        if (distToObstacle < 10.0f) {
            avoidsObstacle = false;
            break;
        }
    }
    
    EXPECT_TRUE(avoidsObstacle);
}

TEST_F(MovementTest, FormationMovement) {
    auto group = CreateTestGroup(5);
    FormationManager formations(group);
    
    formations.SetFormation(FormationManager::FORMATION_ARROW);
    formations.UpdateFormation();
    
    // Check bots are in formation
    // ... verify positions
}
```

## Next Steps

1. **Implement Pathfinder** - A* algorithm integration
2. **Add Movement Controller** - Execute paths smoothly
3. **Create Formation System** - Group movement patterns
4. **Add Obstacle Avoidance** - Dynamic hazard detection
5. **Performance Testing** - Ensure <10ms pathfinding

---

**Status**: Ready for Implementation
**Dependencies**: Combat System
**Estimated Time**: Sprint 3C Days 7-9