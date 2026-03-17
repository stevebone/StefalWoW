# Module Logging Configuration

This document describes how to configure logging for TrinityCore modules using the new ModuleLogManager system.

## Configuration in Module Config Files

Each module configures its logging in its own configuration file to maintain proper separation:

### Playerbot Module - playerbots.conf

Add these lines to your `playerbots.conf` to configure Playerbot logging:

```ini
###################################################################################################
# PLAYERBOT LOGGING CONFIGURATION
#
# This section configures logging for the Playerbot module using the new ModuleLogManager.
#
###################################################################################################

#
#    Playerbot.Log.Level
#        Description: Log level for Playerbot module
#        Levels:      0=Fatal, 1=Error, 2=Warn, 3=Info, 4=Debug, 5=Trace
#        Default:     3 (Info)
#
#    Playerbot.Log.File
#        Description: Log file name for Playerbot module
#        Default:     Playerbot.log
#

# Playerbot Module Logging Configuration
Playerbot.Log.Level = 4
Playerbot.Log.File = Playerbot.log
```

### Other Modules

Other modules would follow the same pattern in their own config files:

```ini
# Example: AuctionModule.conf
Auction.Log.Level = 3
Auction.Log.File = Auction.log

# Example: CustomModule.conf
Custom.Log.Level = 2
Custom.Log.File = Custom.log
```

## Usage in Module Code

### 1. Register Your Module

In your module's initialization:

```cpp
#include "Config/ModuleLogManager.h"

bool MyModule::Initialize()
{
    // Register module with default log level 3 (Info)
    if (!sModuleLogManager->RegisterModule("mymodule", 3, "MyModule.log"))
    {
        TC_LOG_ERROR("server.loading", "Failed to register module logging");
        return false;
    }

    // Initialize logging
    if (!sModuleLogManager->InitializeModuleLogging("mymodule"))
    {
        TC_LOG_ERROR("server.loading", "Failed to initialize module logging");
        return false;
    }

    return true;
}
```

### 2. Use Module Logging Macros

Throughout your module code:

```cpp
// Different log levels
TC_LOG_MODULE_FATAL("mymodule", "Critical error: {}", errorMessage);
TC_LOG_MODULE_ERROR("mymodule", "Error occurred: {}", details);
TC_LOG_MODULE_WARN("mymodule", "Warning: {}", warningMessage);
TC_LOG_MODULE_INFO("mymodule", "Information: {}", info);
TC_LOG_MODULE_DEBUG("mymodule", "Debug info: {}", debugData);
TC_LOG_MODULE_TRACE("mymodule", "Trace: {}", traceInfo);
```

### 3. Shutdown

In your module's shutdown:

```cpp
void MyModule::Shutdown()
{
    // ModuleLogManager handles cleanup automatically
    // No manual cleanup required
}
```

## Benefits

1. **Standardized**: All modules use the same logging API
2. **Centralized**: Configuration in worldserver.conf only
3. **Flexible**: Per-module log levels and files
4. **Integrated**: Works with existing TrinityCore logging
5. **Performance**: Built on TrinityCore's optimized logging system

## Log Files

With the example configuration above:
- `Server.log` - Contains all server logs plus module logs
- `Playerbot.log` - Contains only Playerbot module logs (level 4 and below)
- Console output shows all configured module logs

## Troubleshooting

### Module logs not appearing
1. Check that the module is registered: `sModuleLogManager->RegisterModule()`
2. Check that logging is initialized: `sModuleLogManager->InitializeModuleLogging()`
3. Verify worldserver.conf configuration syntax
4. Check log level settings

### Log file not created
1. Verify file path and permissions
2. Check LogAppender configuration in worldserver.conf
3. Ensure appender name matches logger configuration
4. Check server startup logs for errors

### Performance considerations
- Use appropriate log levels (avoid TRACE in production)
- Configure separate files for high-volume modules
- Monitor disk space for module log files