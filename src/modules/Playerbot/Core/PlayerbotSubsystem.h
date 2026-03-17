/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include <string>

namespace Playerbot
{

enum class SubsystemPriority : uint8
{
    CRITICAL = 0,   // Failure = abort module initialization
    HIGH     = 1,   // Failure = warning, continue (non-critical)
    NORMAL   = 2,   // Standard subsystem
    LOW      = 3,   // Optional subsystem
};

struct SubsystemInfo
{
    std::string name;
    SubsystemPriority priority = SubsystemPriority::NORMAL;
    uint32 initOrder     = 0;   // 0 = skip InitializeAll
    uint32 updateOrder   = 0;   // 0 = skip UpdateAll
    uint32 shutdownOrder = 0;   // 0 = skip ShutdownAll
};

class TC_GAME_API IPlayerbotSubsystem
{
public:
    virtual ~IPlayerbotSubsystem() = default;

    virtual SubsystemInfo GetInfo() const = 0;

    virtual bool Initialize() = 0;

    virtual void Update(uint32 /*diff*/) {}

    virtual void Shutdown() {}
};

} // namespace Playerbot
