#include "TestModule.h"
#include "Log.h"

bool TestModule::Initialize()
{
    // Register this test module with the shared logging API
    if (!sModuleLogManager->RegisterModule("testmodule", 4, "TestModule.log"))
    {
        TC_LOG_ERROR("server.loading", "TestModule: Failed to register with shared ModuleLogManager");
        return false;
    }

    // Initialize the logging
    if (!sModuleLogManager->InitializeModuleLogging("testmodule"))
    {
        TC_LOG_ERROR("server.loading", "TestModule: Failed to initialize module logging");
        return false;
    }

    TC_LOG_INFO("server.loading", "TestModule: Successfully initialized with shared Module Logging API");

    // Test the shared logging
    TestSharedLogging();

    // Register with the shared ModuleUpdateManager for world updates
    if (!sModuleUpdateManager->RegisterModule("testmodule", [](uint32 diff) { TestSharedUpdate(diff); }))
    {
        TC_LOG_ERROR("server.loading", "TestModule: Failed to register with shared ModuleUpdateManager");
        return false;
    }

    TC_LOG_INFO("server.loading", "TestModule: Successfully registered with shared Module Update API");

    return true;
}

void TestModule::TestSharedLogging()
{
    // Test using the TC_LOG macros with our module logger
    TC_LOG_INFO("module.testmodule.file", "TEST: This message should appear in TestModule.log");
    TC_LOG_ERROR("module.testmodule.file", "TEST: Error level message from TestModule");
    TC_LOG_WARN("module.testmodule.file", "TEST: Warning level message from TestModule");

    // Test using the convenience macros
    TC_LOG_MODULE_INFO("testmodule", "TEST: Using TC_LOG_MODULE_INFO macro");
    TC_LOG_MODULE_ERROR("testmodule", "TEST: Using TC_LOG_MODULE_ERROR macro");

    TC_LOG_INFO("server.loading", "TestModule: Shared logging API test completed - check TestModule.log");
}

void TestModule::TestSharedUpdate(uint32 diff)
{
    static uint32 updateCounter = 0;
    static uint32 lastLogTime = 0;

    updateCounter++;
    lastLogTime += diff;

    // Log every 10 seconds to prove the update system is working
    if (lastLogTime >= 10000) // 10000ms = 10 seconds
    {
        TC_LOG_INFO("module.testmodule.file", "TEST: TestModule update called {} times in 10 seconds (diff={}ms)",
                   updateCounter, diff);
        lastLogTime = 0;
        updateCounter = 0;
    }
}

void TestModule::Shutdown()
{
    TC_LOG_INFO("server.loading", "TestModule: Shutting down...");

    // Unregister from ModuleUpdateManager
    sModuleUpdateManager->UnregisterModule("testmodule");
    TC_LOG_INFO("server.loading", "TestModule: Unregistered from ModuleUpdateManager");

    TC_LOG_INFO("server.loading", "TestModule: Shutdown complete");
}