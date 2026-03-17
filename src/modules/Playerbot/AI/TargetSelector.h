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
#include "Player.h"
#include "Unit.h"
#include <vector>

namespace Playerbot
{

class TC_GAME_API TargetSelector
{
public:
    explicit TargetSelector(Player* bot);
    ~TargetSelector() = default;

    // Target selection
    ::Unit* SelectBestTarget(const ::std::vector<::Unit*>& candidates);
    ::Unit* GetCurrentTarget() const;
    void SetTarget(::Unit* target);

    // Target priority calculation
    float CalculateTargetPriority(::Unit* target);

private:
    Player* _bot;
    ::Unit* _currentTarget;
};

} // namespace Playerbot