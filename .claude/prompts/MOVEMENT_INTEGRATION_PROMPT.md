# MOVEMENT SYSTEM INTEGRATION - Claude Code Prompt

**Project**: TrinityBots TrinityCore Playerbot  
**Task**: Integration des BotMovement Systems in Playerbot  
**Priority**: CRITICAL  
**Estimated Effort**: 15-20h

---

## 🚨 PROBLEM STATEMENT

Das **komplette BotMovement System** (45 Dateien, ~8000 LOC) wurde implementiert, aber **NICHT integriert**!

### Aktueller Zustand:
```
✅ IMPLEMENTIERT (aber ungenutzt):
src/server/game/Movement/BotMovement/
├── Core/                    # Manager, Controller, Config
├── Validation/              # Ground, Collision, Liquid, Position
├── StateMachine/            # Swimming, Falling, Ground, Stuck States
├── Pathfinding/             # ValidatedPathGenerator, PathCache
├── Detection/               # StuckDetector, RecoveryStrategies
└── Generators/              # Point, Follow Movement Generators

❌ PLAYERBOT NUTZT NOCH:
src/modules/Playerbot/Spatial/PathCache.cpp
└── Standard PathGenerator OHNE Validierung!
```

### Resultierende Bugs (die das neue System lösen würde):
- **Bots laufen durch Wände** → CollisionValidator nicht aktiv
- **Bots laufen ins Void** → GroundValidator nicht aktiv  
- **Bots hüpfen über Wasser** → SwimmingMovementState nicht aktiv
- **Bots hüpfen in der Luft** → FallingMovementState nicht aktiv
- **Bots bleiben stecken** → StuckDetector nicht aktiv

---

## 📁 DATEIEN ZUM LESEN

**Vor dem Start unbedingt lesen:**

```
# Technische Spezifikation (1449 Zeilen)
C:\TrinityBots\TrinityCore\.claude\prompts\MOVEMENT_TECHNICAL_SPEC.md

# Status-Dokument
C:\TrinityBots\TrinityCore\.claude\analysis\MOVEMENT_ZENFLOW_STATUS.md

# Bestehende Movement-Dateien
C:\TrinityBots\TrinityCore\src\server\game\Movement\BotMovement\Core\BotMovementManager.h
C:\TrinityBots\TrinityCore\src\server\game\Movement\BotMovement\Core\BotMovementController.h
C:\TrinityBots\TrinityCore\src\server\game\Movement\BotMovement\Pathfinding\ValidatedPathGenerator.h

# Playerbot Integration Points
C:\TrinityBots\TrinityCore\src\modules\Playerbot\AI\BotAI.h
C:\TrinityBots\TrinityCore\src\modules\Playerbot\AI\BotAI.cpp
C:\TrinityBots\TrinityCore\src\modules\Playerbot\Spatial\PathCache.cpp
```

---

## 🎯 INTEGRATION TASKS

### TASK 1: BotAI Integration (5h)

**Ziel**: BotAI soll BotMovementController für jeden Bot nutzen.

**Datei**: `src/modules/Playerbot/AI/BotAI.h`

```cpp
// HINZUFÜGEN am Anfang der Datei:
#include "Movement/BotMovement/Core/BotMovementController.h"
#include "Movement/BotMovement/Core/BotMovementManager.h"

// HINZUFÜGEN in class BotAI:
class TC_GAME_API BotAI
{
public:
    // ... existing ...
    
    // NEW: Movement System Integration
    BotMovementController* GetMovementController() { return _movementController.get(); }
    BotMovementController const* GetMovementController() const { return _movementController.get(); }
    
    // Movement convenience methods
    bool MoveTo(Position const& dest, bool validated = true);
    bool MoveToUnit(Unit* target, float distance = 0.0f);
    bool IsMovementBlocked() const;
    bool IsStuck() const;
    
private:
    // NEW: Movement controller per bot
    std::unique_ptr<BotMovementController> _movementController;
};
```

**Datei**: `src/modules/Playerbot/AI/BotAI.cpp`

```cpp
// In BotAI constructor:
BotAI::BotAI(Player* bot)
    : _bot(bot)
    // ... existing initialization ...
{
    // NEW: Initialize movement controller
    _movementController = std::make_unique<BotMovementController>(bot);
    
    // Register with global manager
    sBotMovementManager->RegisterController(bot->GetGUID(), _movementController.get());
}

// In BotAI destructor:
BotAI::~BotAI()
{
    // NEW: Unregister movement controller
    if (_movementController)
    {
        sBotMovementManager->UnregisterController(_bot->GetGUID());
    }
}

// In BotAI::UpdateAI(uint32 diff):
void BotAI::UpdateAI(uint32 diff)
{
    // NEW: Update movement controller FIRST
    if (_movementController)
    {
        _movementController->Update(diff);
        
        // Check for stuck and handle recovery
        if (_movementController->IsStuck())
        {
            LOG_DEBUG("playerbot.movement", "Bot %s is stuck, initiating recovery",
                _bot->GetName().c_str());
            // Recovery is handled internally by controller
        }
    }
    
    // ... rest of existing UpdateAI ...
}

// NEW: Movement helper methods
bool BotAI::MoveTo(Position const& dest, bool validated)
{
    if (!_movementController)
        return false;
    
    if (validated)
    {
        return _movementController->MoveToValidated(dest);
    }
    else
    {
        // Fallback to unvalidated movement (legacy)
        _bot->GetMotionMaster()->MovePoint(0, dest);
        return true;
    }
}

bool BotAI::MoveToUnit(Unit* target, float distance)
{
    if (!_movementController || !target)
        return false;
    
    return _movementController->MoveToTarget(target, distance);
}

bool BotAI::IsMovementBlocked() const
{
    return _movementController && _movementController->IsBlocked();
}

bool BotAI::IsStuck() const
{
    return _movementController && _movementController->IsStuck();
}
```

---

### TASK 2: PathCache Migration (4h)

**Ziel**: Playerbot's PathCache soll ValidatedPathGenerator nutzen statt Standard PathGenerator.

**Datei**: `src/modules/Playerbot/Spatial/PathCache.cpp`

```cpp
// ÄNDERN: Include hinzufügen
#include "Movement/BotMovement/Pathfinding/ValidatedPathGenerator.h"
#include "Movement/BotMovement/Core/BotMovementConfig.h"

// ÄNDERN: CalculateNewPath Methode
PathCache::PathResult PathCache::CalculateNewPath(Position const& src, Position const& dest, WorldObject const* owner)
{
    PathResult result;
    
    // NEW: Use ValidatedPathGenerator if enabled
    if (BotMovementConfig::IsEnabled())
    {
        ValidatedPathGenerator validatedPath(owner);
        ValidatedPathResult vpResult = validatedPath.GenerateValidatedPath(src, dest);
        
        if (vpResult.IsSuccess())
        {
            result.waypoints = vpResult.GetPath();
            result.pathType = vpResult.GetPathType();
            result.isValid = true;
            
            // Log validation info for debugging
            LOG_DEBUG("playerbot.movement", "ValidatedPath: %zu waypoints, type=%u, validations=%u",
                result.waypoints.size(), 
                static_cast<uint32>(result.pathType),
                vpResult.GetValidationCount());
        }
        else
        {
            // Validation failed - log reason
            LOG_WARN("playerbot.movement", "ValidatedPath failed: %s (reason=%u)",
                vpResult.GetErrorMessage().c_str(),
                static_cast<uint32>(vpResult.GetFailureReason()));
            
            // Fallback to standard pathfinding
            result = CalculateNewPathLegacy(src, dest, owner);
        }
    }
    else
    {
        // Legacy mode - use standard PathGenerator
        result = CalculateNewPathLegacy(src, dest, owner);
    }
    
    return result;
}

// NEW: Legacy fallback method (existing code)
PathCache::PathResult PathCache::CalculateNewPathLegacy(Position const& src, Position const& dest, WorldObject const* owner)
{
    PathResult result;
    
    PathGenerator path(owner);
    bool success = path.CalculatePath(
        dest.GetPositionX(),
        dest.GetPositionY(),
        dest.GetPositionZ(),
        false  // forceDestination
    );
    
    if (success)
    {
        Movement::PointsArray const& points = path.GetPath();
        result.waypoints.reserve(points.size());
        for (auto const& point : points)
        {
            result.waypoints.emplace_back(point.x, point.y, point.z, 0.0f);
        }
        result.pathType = path.GetPathType();
        result.isValid = true;
    }
    
    return result;
}
```

**Datei**: `src/modules/Playerbot/Spatial/PathCache.h`

```cpp
// HINZUFÜGEN: Private method declaration
private:
    // Legacy pathfinding fallback
    PathResult CalculateNewPathLegacy(Position const& src, Position const& dest, WorldObject const* owner);
```

---

### TASK 3: Movement Generator Replacement (4h)

**Ziel**: BotFollowMovementGenerator und BotPointMovementGenerator in Playerbot nutzen.

**Suche nach allen Stellen wo MotionMaster verwendet wird:**

```cpp
// PATTERN ZU FINDEN:
bot->GetMotionMaster()->MovePoint(...)
bot->GetMotionMaster()->MoveFollow(...)
bot->GetMotionMaster()->MoveChase(...)

// ERSETZEN MIT:
if (BotAI* ai = GetBotAI(bot))
{
    ai->GetMovementController()->MoveToValidated(dest);
    // oder
    ai->GetMovementController()->FollowTarget(target, distance, angle);
    // oder
    ai->GetMovementController()->ChaseTarget(target, distance);
}
```

**Hauptdateien zu ändern:**

1. `src/modules/Playerbot/Movement/BotMovementManager.cpp` (Playerbot's eigener)
2. `src/modules/Playerbot/AI/Actions/MovementActions.cpp`
3. `src/modules/Playerbot/AI/Actions/FollowActions.cpp`
4. `src/modules/Playerbot/Quest/QuestNavigation.cpp`
5. `src/modules/Playerbot/Combat/CombatPositioning.cpp`

**Beispiel für MovementActions.cpp:**

```cpp
// VORHER:
bool MoveToAction::Execute(Player* bot, Position const& dest)
{
    bot->GetMotionMaster()->MovePoint(0, dest);
    return true;
}

// NACHHER:
bool MoveToAction::Execute(Player* bot, Position const& dest)
{
    if (BotAI* ai = GetBotAI(bot))
    {
        return ai->MoveTo(dest, true);  // validated = true
    }
    
    // Fallback for non-bot players
    bot->GetMotionMaster()->MovePoint(0, dest);
    return true;
}
```

---

### TASK 4: State Machine Activation (3h)

**Ziel**: MovementStateMachine wird automatisch vom BotMovementController genutzt.

**Prüfen in**: `src/server/game/Movement/BotMovement/Core/BotMovementController.cpp`

```cpp
// Sicherstellen dass StateMachine korrekt initialisiert wird:
BotMovementController::BotMovementController(Unit* owner)
    : _owner(owner)
{
    // Initialize state machine with all states
    _stateMachine = std::make_unique<MovementStateMachine>(owner);
    _stateMachine->RegisterState(std::make_unique<IdleState>());
    _stateMachine->RegisterState(std::make_unique<GroundMovementState>());
    _stateMachine->RegisterState(std::make_unique<SwimmingMovementState>());
    _stateMachine->RegisterState(std::make_unique<FallingMovementState>());
    _stateMachine->RegisterState(std::make_unique<StuckState>());
    
    // Start in idle state
    _stateMachine->SetState(MovementStateType::Idle);
    
    // Initialize stuck detector
    _stuckDetector = std::make_unique<StuckDetector>(owner);
    
    // Initialize validators
    _groundValidator = std::make_unique<GroundValidator>();
    _collisionValidator = std::make_unique<CollisionValidator>();
    _liquidValidator = std::make_unique<LiquidValidator>();
    
    LOG_DEBUG("playerbot.movement", "BotMovementController initialized for %s",
        owner->GetName().c_str());
}
```

**Prüfen in Update():**

```cpp
void BotMovementController::Update(uint32 diff)
{
    if (!_owner || !_owner->IsAlive())
        return;
    
    // Update stuck detection
    if (_stuckDetector)
    {
        _stuckDetector->Update(diff, _owner->GetPosition());
        
        if (_stuckDetector->IsStuck() && !_isRecovering)
        {
            InitiateRecovery(_stuckDetector->GetStuckType());
        }
    }
    
    // Update state machine
    if (_stateMachine)
    {
        // Check for state transitions
        UpdateStateTransitions();
        
        // Update current state
        _stateMachine->Update(diff);
    }
    
    // Update position history for stuck detection
    UpdatePositionHistory();
}

void BotMovementController::UpdateStateTransitions()
{
    MovementStateType currentState = _stateMachine->GetCurrentStateType();
    MovementStateType newState = DetermineAppropriateState();
    
    if (newState != currentState)
    {
        LOG_DEBUG("playerbot.movement", "Bot %s state transition: %s -> %s",
            _owner->GetName().c_str(),
            _stateMachine->GetCurrentState()->GetName(),
            GetStateName(newState));
        
        _stateMachine->SetState(newState);
    }
}

MovementStateType BotMovementController::DetermineAppropriateState()
{
    // Priority order: Stuck > Swimming > Falling > Ground > Idle
    
    if (_isRecovering)
        return MovementStateType::Stuck;
    
    // Check if in water
    if (_liquidValidator && _liquidValidator->IsInWater(_owner))
        return MovementStateType::Swimming;
    
    // Check if falling
    if (!_owner->HasUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT) &&
        !_groundValidator->IsOnGround(_owner))
        return MovementStateType::Falling;
    
    // Check if moving
    if (_owner->isMoving())
        return MovementStateType::Ground;
    
    return MovementStateType::Idle;
}
```

---

### TASK 5: Configuration (2h)

**Ziel**: worldserver.conf Optionen für BotMovement.

**Datei**: `worldserver.conf.dist` (oder Playerbot's config)

```ini
###################################################################################################
# BOT MOVEMENT SYSTEM
#
#    BotMovement.Enable
#        Description: Enable the validated movement system for bots.
#        Default:     1 (Enabled)
#
#    BotMovement.Validation.Ground
#        Description: Enable ground validation (void detection, cliff detection).
#        Default:     1 (Enabled)
#
#    BotMovement.Validation.Collision
#        Description: Enable collision validation (wall detection, LOS checks).
#        Default:     1 (Enabled)
#
#    BotMovement.Validation.Liquid
#        Description: Enable liquid validation (water detection, swimming).
#        Default:     1 (Enabled)
#
#    BotMovement.StuckDetection.Enable
#        Description: Enable stuck detection and auto-recovery.
#        Default:     1 (Enabled)
#
#    BotMovement.StuckDetection.Threshold
#        Description: Distance in yards bot must move in 5 seconds to not be "stuck".
#        Default:     2.0
#
#    BotMovement.StuckDetection.RecoveryMaxAttempts
#        Description: Maximum recovery attempts before teleporting to safety.
#        Default:     3
#
#    BotMovement.PathCache.Enable
#        Description: Enable path caching for performance.
#        Default:     1 (Enabled)
#
#    BotMovement.PathCache.MaxSize
#        Description: Maximum number of cached paths.
#        Default:     5000
#
#    BotMovement.PathCache.TTL
#        Description: Path cache time-to-live in milliseconds.
#        Default:     30000
#
#    BotMovement.Debug.LogStateChanges
#        Description: Log movement state changes (verbose).
#        Default:     0 (Disabled)
#
#    BotMovement.Debug.LogValidationFailures
#        Description: Log validation failures (useful for debugging).
#        Default:     1 (Enabled)
#
###################################################################################################

BotMovement.Enable = 1
BotMovement.Validation.Ground = 1
BotMovement.Validation.Collision = 1
BotMovement.Validation.Liquid = 1
BotMovement.StuckDetection.Enable = 1
BotMovement.StuckDetection.Threshold = 2.0
BotMovement.StuckDetection.RecoveryMaxAttempts = 3
BotMovement.PathCache.Enable = 1
BotMovement.PathCache.MaxSize = 5000
BotMovement.PathCache.TTL = 30000
BotMovement.Debug.LogStateChanges = 0
BotMovement.Debug.LogValidationFailures = 1
```

**Update BotMovementConfig::Load():**

```cpp
void BotMovementConfig::Load()
{
    _enabled = sConfigMgr->GetBoolDefault("BotMovement.Enable", true);
    
    // Validation settings
    _groundValidation = sConfigMgr->GetBoolDefault("BotMovement.Validation.Ground", true);
    _collisionValidation = sConfigMgr->GetBoolDefault("BotMovement.Validation.Collision", true);
    _liquidValidation = sConfigMgr->GetBoolDefault("BotMovement.Validation.Liquid", true);
    
    // Stuck detection
    _stuckDetectionEnabled = sConfigMgr->GetBoolDefault("BotMovement.StuckDetection.Enable", true);
    _stuckThreshold = sConfigMgr->GetFloatDefault("BotMovement.StuckDetection.Threshold", 2.0f);
    _maxRecoveryAttempts = sConfigMgr->GetIntDefault("BotMovement.StuckDetection.RecoveryMaxAttempts", 3);
    
    // Path cache
    _pathCacheEnabled = sConfigMgr->GetBoolDefault("BotMovement.PathCache.Enable", true);
    _pathCacheMaxSize = sConfigMgr->GetIntDefault("BotMovement.PathCache.MaxSize", 5000);
    _pathCacheTTL = Milliseconds(sConfigMgr->GetIntDefault("BotMovement.PathCache.TTL", 30000));
    
    // Debug
    _logStateChanges = sConfigMgr->GetBoolDefault("BotMovement.Debug.LogStateChanges", false);
    _logValidationFailures = sConfigMgr->GetBoolDefault("BotMovement.Debug.LogValidationFailures", true);
    
    LOG_INFO("server.loading", ">> Loaded BotMovement configuration (enabled=%s)", 
        _enabled ? "true" : "false");
}
```

---

### TASK 6: Testing & Validation (2h)

**Test Cases:**

```cpp
// Test 1: Wasser-Erkennung
// Teleportiere Bot zu Wasser, prüfe ob SwimmingState aktiv wird
TEST_CASE("Bot enters swimming state in water")
{
    Player* bot = CreateTestBot();
    TeleportTo(bot, /* Elwynn Forest Lake */);
    
    Wait(1000);
    
    auto* controller = GetBotAI(bot)->GetMovementController();
    REQUIRE(controller->GetCurrentState() == MovementStateType::Swimming);
    REQUIRE(bot->HasUnitMovementFlag(MOVEMENTFLAG_SWIMMING));
}

// Test 2: Stuck Detection
// Teleportiere Bot in eine Ecke, prüfe ob Stuck erkannt wird
TEST_CASE("Bot detects stuck condition")
{
    Player* bot = CreateTestBot();
    TeleportTo(bot, /* Corner position */);
    
    // Try to move through wall
    Position wallTarget = /* position behind wall */;
    GetBotAI(bot)->MoveTo(wallTarget);
    
    Wait(6000);  // Stuck threshold is 5 seconds
    
    auto* controller = GetBotAI(bot)->GetMovementController();
    REQUIRE(controller->IsStuck());
}

// Test 3: Validated Path avoids void
TEST_CASE("Validated path avoids void areas")
{
    Player* bot = CreateTestBot();
    Position start = /* safe position */;
    Position end = /* position past void */;
    
    auto* controller = GetBotAI(bot)->GetMovementController();
    bool result = controller->MoveToValidated(end);
    
    // Should either find safe path or reject movement
    if (result)
    {
        // Path should not go through void
        auto path = controller->GetCurrentPath();
        for (auto& point : path)
        {
            REQUIRE(IsValidGround(point));
        }
    }
}

// Test 4: Falling state
TEST_CASE("Bot enters falling state when airborne")
{
    Player* bot = CreateTestBot();
    TeleportTo(bot, /* High position */);
    
    // Push bot off edge
    bot->KnockbackFrom(/* position */, 10.0f, 5.0f);
    
    Wait(500);
    
    auto* controller = GetBotAI(bot)->GetMovementController();
    REQUIRE(controller->GetCurrentState() == MovementStateType::Falling);
}
```

**Manuelle Tests:**

1. **Wasser-Test**:
   - Teleportiere einen Bot an einen See
   - Bot sollte schwimmen, nicht hüpfen
   - Log sollte zeigen: "State transition: Ground -> Swimming"

2. **Wand-Test**:
   - Befehle Bot, durch eine Wand zu laufen
   - Bot sollte davor stoppen oder um die Wand herum gehen
   - Log sollte zeigen: "Validation failed: Collision detected"

3. **Klippen-Test**:
   - Befehle Bot, über eine Klippe zu laufen
   - Bot sollte am Rand stoppen
   - Log sollte zeigen: "Validation failed: Void detected"

4. **Stuck-Test**:
   - Teleportiere Bot in eine enge Ecke
   - Nach 5+ Sekunden sollte Recovery starten
   - Log sollte zeigen: "Bot is stuck, initiating recovery"

---

## 📋 IMPLEMENTATION CHECKLIST

### Phase 1: Core Integration
- [ ] Add includes to BotAI.h
- [ ] Add BotMovementController member to BotAI
- [ ] Initialize controller in BotAI constructor
- [ ] Cleanup in BotAI destructor
- [ ] Call controller->Update() in BotAI::UpdateAI()
- [ ] Add MoveTo/MoveToUnit helper methods

### Phase 2: PathCache Migration
- [ ] Add ValidatedPathGenerator include
- [ ] Modify CalculateNewPath to use ValidatedPathGenerator
- [ ] Add CalculateNewPathLegacy fallback
- [ ] Add config check for enabled state

### Phase 3: Movement Generator Usage
- [ ] Find all MovePoint/MoveFollow/MoveChase calls
- [ ] Replace with controller methods where appropriate
- [ ] Keep fallback for non-bot players
- [ ] Test each replacement

### Phase 4: State Machine
- [ ] Verify state machine initialization
- [ ] Verify state transitions work
- [ ] Test Swimming state
- [ ] Test Falling state
- [ ] Test Stuck state

### Phase 5: Configuration
- [ ] Add config options to worldserver.conf
- [ ] Update BotMovementConfig::Load()
- [ ] Test enable/disable toggle
- [ ] Test validation toggles

### Phase 6: Testing
- [ ] Water test (swimming)
- [ ] Wall test (collision)
- [ ] Cliff test (void detection)
- [ ] Stuck test (recovery)
- [ ] Performance test (5000 bots)

---

## 🔧 BUILD & TEST

```bash
# Build
cd C:\TrinityBots\TrinityCore\build
cmake --build . --config RelWithDebInfo --target worldserver

# Test
# 1. Start server
# 2. Login with GM character
# 3. Spawn bots: .playerbot bot add
# 4. Test movement: .playerbot bot follow
# 5. Teleport to water and observe
# 6. Check logs for state transitions
```

---

## 📊 SUCCESS CRITERIA

| Test | Expected Result |
|------|-----------------|
| Bot in Water | SwimmingState aktiv, MOVEMENTFLAG_SWIMMING gesetzt |
| Bot vor Wand | Stoppt oder geht drumherum |
| Bot vor Void | Stoppt, "Void detected" Log |
| Bot steckt fest | Recovery nach 5s, Teleport falls nötig |
| 5000 Bots | < 5% CPU Overhead vs. ohne BotMovement |
| Config disabled | Altes Verhalten (legacy PathGenerator) |

---

## 💡 TIPS

1. **Start klein**: Erst nur BotAI Integration, dann schrittweise erweitern
2. **Logging nutzen**: `LOG_DEBUG("playerbot.movement", ...)` für alle wichtigen Events
3. **Config-Toggle**: Immer prüfen ob `BotMovementConfig::IsEnabled()` bevor neuer Code läuft
4. **Fallback**: Legacy-Code behalten für Notfälle
5. **Inkrementell testen**: Nach jeder Änderung kompilieren und testen

---

## 🚀 START

```
1. Lies MOVEMENT_TECHNICAL_SPEC.md
2. Lies BotMovementController.h verstehen
3. Starte mit Task 1: BotAI Integration
4. Kompiliere und teste
5. Weiter mit Task 2, etc.
```

**Viel Erfolg bei der Integration!**
