#pragma once

#include "Logging/ModuleLogManager.h"
#include "Update/ModuleUpdateManager.h"

class TestModule
{
public:
    static bool Initialize();
    static void TestSharedLogging();
    static void TestSharedUpdate(uint32 diff);
    static void Shutdown();
};