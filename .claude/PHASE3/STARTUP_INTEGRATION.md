# Playerbot Startup Integration

## Overview

The Playerbot module is now fully integrated with TrinityCore's worldserver startup and shutdown sequence.

## Integration Points

### 1. Module Loading

**Location:** `src/server/worldserver/Main.cpp`

```cpp
#ifdef PLAYERBOT_ENABLED
#include "modules/Playerbot/PlayerbotModule.h"
#endif
```

The module header is conditionally included only when `PLAYERBOT_ENABLED` is defined during compilation.

### 2. Initialization

**When:** After `sWorld->SetInitialWorldSettings()` succeeds
**Location:** `src/server/worldserver/Main.cpp:361-368`

```cpp
#ifdef PLAYERBOT_ENABLED
    // Initialize Playerbot Module after world is set up
    if (!PlayerbotModule::Initialize())
    {
        TC_LOG_ERROR("server.worldserver", "Failed to initialize Playerbot Module");
        // Continue startup even if playerbot fails to initialize
    }
#endif
```

**Key Behaviors:**
- ✅ Only initializes when `PLAYERBOT_ENABLED` is compiled in
- ✅ Server continues startup even if playerbot initialization fails
- ✅ Runs after world setup is complete
- ✅ Logs errors appropriately

### 3. Shutdown

**When:** During server shutdown, after `sScriptMgr->OnShutdown()`
**Location:** `src/server/worldserver/Main.cpp:469-472`

```cpp
#ifdef PLAYERBOT_ENABLED
    // Shutdown Playerbot Module
    PlayerbotModule::Shutdown();
#endif
```

**Key Behaviors:**
- ✅ Clean shutdown of all playerbot resources
- ✅ Executed before realm is marked offline
- ✅ No conditional checks - always safe to call

## Initialization Sequence

When `PlayerbotModule::Initialize()` is called, it follows this sequence:

1. **Configuration Loading** - Loads `playerbots.conf`
2. **Enable Check** - Checks `Playerbot.Enable` setting (default: 0)
3. **Configuration Validation** - Validates all settings
4. **Logging Setup** - Initializes specialized log channels
5. **Bot Account Manager** - Loads bot accounts from database
6. **Bot Name Manager** - Loads available names and tracks usage
7. **Bot Lifecycle Manager** - Initializes bot spawning and scheduling
8. **Hook Registration** - Registers with TrinityCore event system

If any step fails, initialization is aborted and logged appropriately.

## Shutdown Sequence

When `PlayerbotModule::Shutdown()` is called:

1. **Hook Unregistration** - Removes all TrinityCore event hooks
2. **Lifecycle Manager Shutdown** - Stops all bot spawning/scheduling
3. **Name Manager Shutdown** - Saves name usage data
4. **Account Manager Shutdown** - Closes database connections
5. **State Reset** - Marks module as uninitialized

## Configuration Control

The module respects these configuration settings:

```ini
# Core enable/disable switch
Playerbot.Enable = 0  # 0 = Disabled, 1 = Enabled

# If disabled, the module initializes but does nothing
# All startup sequence still runs for consistency
```

## Error Handling

### Initialization Failures
- **Configuration errors:** Module disabled, server continues
- **Database errors:** Module disabled, server continues
- **Validation errors:** Module disabled, server continues
- **Fatal errors:** Only if `Playerbot.StrictCharacterLimit = 1` and violations exist

### Runtime Failures
- **Individual bot failures:** Bot disabled, others continue
- **System failures:** Module gracefully degrades
- **Resource exhaustion:** Module throttles operations

## Logging Integration

The module uses TrinityCore's logging system with these channels:

- `server.loading` - Module startup/shutdown messages
- `module.playerbot` - General module logging
- `module.playerbot.ai` - AI decision logging
- `module.playerbot.performance` - Performance metrics
- `module.playerbot.account` - Account management
- `module.playerbot.character` - Character management
- `module.playerbot.names` - Name allocation

## Testing the Integration

### 1. Compile-time Test
```bash
# Build without playerbot (default)
cmake .. -DBUILD_PLAYERBOT=0
make

# Build with playerbot
cmake .. -DBUILD_PLAYERBOT=1
make
```

### 2. Runtime Test (Disabled)
```ini
# playerbots.conf
Playerbot.Enable = 0
```

Expected log output:
```
[INFO] Initializing Playerbot Module...
[INFO] Playerbot Module: Disabled in configuration
[INFO] Playerbot Module: Successfully initialized (Version 1.0.0)
```

### 3. Runtime Test (Enabled)
```ini
# playerbots.conf
Playerbot.Enable = 1
```

Expected log output:
```
[INFO] Initializing Playerbot Module...
[INFO] Initializing Bot Account Manager...
[INFO] Initializing Bot Name Manager...
[INFO] Initializing Bot Lifecycle Manager...
[INFO] Playerbot Module: Successfully initialized (Version 1.0.0)
[INFO]   - X bot accounts loaded
[INFO]   - Y names available (Z used)
```

## Integration Verification

The integration is verified by:

1. ✅ **Conditional Compilation** - Only compiles when `PLAYERBOT_ENABLED`
2. ✅ **Safe Initialization** - Server starts even if playerbot fails
3. ✅ **Proper Timing** - Initializes after world setup, before services
4. ✅ **Clean Shutdown** - All resources properly released
5. ✅ **Configuration Respect** - Honors enable/disable settings
6. ✅ **Error Logging** - All failures appropriately logged

## Future Enhancements

The integration provides hooks for:

- **World Update Integration** - Can be added to world update loop
- **Player Event Hooks** - Login/logout event handling
- **Configuration Reload** - Runtime configuration updates
- **Performance Monitoring** - Integration with TrinityCore metrics

---

**Status:** ✅ Complete and Integrated
**Last Updated:** Phase 3, Week 12
**Integration Points:** 3 (Include, Initialize, Shutdown)
**Safety Level:** High (Non-breaking, graceful degradation)