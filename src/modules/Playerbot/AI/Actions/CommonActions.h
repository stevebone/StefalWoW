/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Action.h"

namespace Playerbot
{

// Move to position action
class TC_GAME_API MoveToPositionAction : public MovementAction
{
public:
    MoveToPositionAction();

    bool IsPossible(BotAI* ai) const;
    bool IsUseful(BotAI* ai) const;
    ActionResult Execute(BotAI* ai, ActionContext const& context);
};

// Follow target action
class TC_GAME_API FollowAction : public MovementAction
{
public:
    FollowAction();

    bool IsPossible(BotAI* ai) const;
    bool IsUseful(BotAI* ai) const;
    ActionResult Execute(BotAI* ai, ActionContext const& context);

protected:
    ::Unit* GetFollowTarget(BotAI* ai) const;
    float GetFollowDistance() const;
    float GetFollowAngle() const;
};

// Attack action
class TC_GAME_API AttackAction : public CombatAction
{
public:
    AttackAction();

    bool IsPossible(BotAI* ai) const;
    bool IsUseful(BotAI* ai) const;
    ActionResult Execute(BotAI* ai, ActionContext const& context);

    float GetRange() const;

protected:
    ::Unit* GetAttackTarget(BotAI* ai) const;
};

// Heal action
class TC_GAME_API HealAction : public SpellAction
{
public:
    explicit HealAction(uint32 spellId);

    bool IsUseful(BotAI* ai) const;
    ActionResult Execute(BotAI* ai, ActionContext const& context);

protected:
    ::Unit* GetHealTarget(BotAI* ai) const;
};

// Buff action
class TC_GAME_API BuffAction : public SpellAction
{
public:
    explicit BuffAction(uint32 spellId);

    bool IsUseful(BotAI* ai) const;
    ActionResult Execute(BotAI* ai, ActionContext const& context);

protected:
    ::Unit* GetBuffTarget(BotAI* ai) const;
};

} // namespace Playerbot