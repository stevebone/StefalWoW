/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRINITYCORE_COMBAT_NODES_H
#define TRINITYCORE_COMBAT_NODES_H

#include "AI/BehaviorTree/BehaviorTree.h"
#include "AI/BotAI.h"
#include "AI/Cache/AuraStateCache.h"
#include "Player.h"
#include "Unit.h"
#include "Creature.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellHistory.h"
#include "SharedDefines.h"
#include "GameTime.h"

namespace Playerbot
{

/**
 * @brief Check if bot has a valid combat target
 */
class TC_GAME_API BTCheckHasTarget : public BTCondition
{
public:
    BTCheckHasTarget()
        : BTCondition("CheckHasTarget",
            [](BotAI* ai, BTBlackboard& blackboard) -> bool
            {
                if (!ai)
                    return false;

                Player* bot = ai->GetBot();
                if (!bot)
                    return false;

                Unit* target = bot->GetSelectedUnit();
                if (!target || !target->IsAlive() || target->HasStealthAura())
                    return false;

                // Store target in blackboard for subsequent nodes
                blackboard.Set<Unit*>("CurrentTarget", target);
                return true;
            })
    {}
};

/**
 * @brief Check if target is within specified range
 */
class TC_GAME_API BTCheckInRange : public BTLeaf
{
public:
    BTCheckInRange(float minRange, float maxRange)
        : BTLeaf("CheckInRange"), _minRange(minRange), _maxRange(maxRange)
    {}

    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard) override
    {
        if (!ai)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        Player* bot = ai->GetBot();
        if (!bot)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        Unit* target = nullptr;
        if (!blackboard.Get<Unit*>("CurrentTarget", target) || !target)
        {
            _status = BTStatus::FAILURE;
            return _status;
        }

        float distance = bot->GetDistance(target);

        if (distance >= _minRange && distance <= _maxRange)
        {
            blackboard.Set<float>("TargetDistance", distance);
            _status = BTStatus::SUCCESS;
        }
        else
        {
            _status = BTStatus::FAILURE;
        }

        return _status;
    }

private:
    float _minRange;
    float _maxRange;
};

/**
 * @brief Check if bot is facing target
 */
class TC_GAME_API BTCheckFacingTarget : public BTCondition
{
public:
    BTCheckFacingTarget()
        : BTCondition("CheckFacingTarget",
            [](BotAI* ai, BTBlackboard& blackboard) -> bool
            {
                if (!ai)
                    return false;

                Player* bot = ai->GetBot();
                if (!bot)
                    return false;

                Unit* target = nullptr;
                if (!blackboard.Get<Unit*>("CurrentTarget", target) || !target)
                    return false;

                return bot->HasInArc(static_cast<float>(M_PI) / 6.0f, target); // 30 degree arc
            })
    {}
};

/**
 * @brief Face target
 */
class TC_GAME_API BTFaceTarget : public BTAction
{
public:
    BTFaceTarget()
        : BTAction("FaceTarget",
            [](BotAI* ai, BTBlackboard& blackboard) -> BTStatus
            {
                if (!ai)
                    return BTStatus::INVALID;

                Player* bot = ai->GetBot();
                if (!bot)
                    return BTStatus::INVALID;

                Unit* target = nullptr;
                if (!blackboard.Get<Unit*>("CurrentTarget", target) || !target)
                    return BTStatus::FAILURE;

                bot->SetFacingToObject(target);
                return BTStatus::SUCCESS;
            })
    {}
};

/**
 * @brief Check if spell is ready to cast
 */
class TC_GAME_API BTCheckSpellReady : public BTLeaf
{
public:
    BTCheckSpellReady(uint32 spellId)
        : BTLeaf("CheckSpellReady"), _spellId(spellId)
    {}

    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard) override
    {
        if (!ai)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        Player* bot = ai->GetBot();
        if (!bot)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        // Check if spell is known
        if (!bot->HasSpell(_spellId))
        {
            _status = BTStatus::FAILURE;
            return _status;
        }

        // Check if spell is on cooldown
        if (bot->GetSpellHistory()->HasCooldown(_spellId))
        {
            _status = BTStatus::FAILURE;
            return _status;
        }

        // Check if bot has enough resources (mana/rage/energy)
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(_spellId, DIFFICULTY_NONE);
        if (!spellInfo)
        {
            _status = BTStatus::FAILURE;
            return _status;
        }

        // TrinityCore 12.0: CalcPowerCost returns vector of all power costs
        auto powerCosts = spellInfo->CalcPowerCost(bot, spellInfo->GetSchoolMask());
        for (auto const& cost : powerCosts)
        {
            if (bot->GetPower(cost.Power) < cost.Amount)
            {
                _status = BTStatus::FAILURE;
                return _status;
            }
        }

        blackboard.Set<uint32>("ReadySpell", _spellId);
        _status = BTStatus::SUCCESS;
        return _status;
    }

private:
    uint32 _spellId;
};

/**
 * @brief Cast spell on current target
 */
class TC_GAME_API BTCastSpell : public BTLeaf
{
public:
    BTCastSpell(uint32 spellId)
        : BTLeaf("CastSpell"), _spellId(spellId), _castStartTime(0)
    {}

    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard) override
    {
        if (!ai)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        Player* bot = ai->GetBot();
        if (!bot)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        Unit* target = nullptr;
        if (!blackboard.Get<Unit*>("CurrentTarget", target) || !target)
        {
            _status = BTStatus::FAILURE;
            return _status;
        }

        // If not casting, start cast
        if (_castStartTime == 0)
        {
            if (bot->IsNonMeleeSpellCast(false))
            {
                _status = BTStatus::RUNNING;
                return _status;
            }

            // Attempt to cast
            ::SpellCastResult result = ai->CastSpell(_spellId, target);

            if (result != SPELL_CAST_OK)
            {
                _status = BTStatus::FAILURE;
                Reset();
                return _status;
            }

            _castStartTime = GameTime::GetGameTimeMS();
            _status = BTStatus::RUNNING;
            return _status;
        }

        // Check if cast is complete
        if (!bot->IsNonMeleeSpellCast(false))
        {
            _status = BTStatus::SUCCESS;
            Reset();
            return _status;
        }

        // Still casting
        _status = BTStatus::RUNNING;
        return _status;
    }

    void Reset() override
    {
        BTLeaf::Reset();
        _castStartTime = 0;
    }

private:
    uint32 _spellId;
    uint32 _castStartTime;
};

/**
 * @brief Wait for global cooldown
 */
class TC_GAME_API BTWaitForGCD : public BTAction
{
public:
    BTWaitForGCD()
        : BTAction("WaitForGCD",
            [](BotAI* ai, BTBlackboard& /*blackboard*/) -> BTStatus
            {
                if (!ai)
                    return BTStatus::INVALID;

                Player* bot = ai->GetBot();
                if (!bot)
                    return BTStatus::INVALID;

                // Check if GCD is active
                if (bot->HasUnitState(UNIT_STATE_CASTING))
                    return BTStatus::RUNNING;

                return BTStatus::SUCCESS;
            })
    {}
};

/**
 * @brief Check if target has specific aura/debuff
 */
class TC_GAME_API BTCheckTargetHasAura : public BTLeaf
{
public:
    BTCheckTargetHasAura(uint32 spellId, bool shouldHave = true)
        : BTLeaf("CheckTargetHasAura"), _spellId(spellId), _shouldHave(shouldHave)
    {}

    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard) override
    {
        if (!ai)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        Player* bot = ai->GetBot();
        if (!bot)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        Unit* target = nullptr;
        if (!blackboard.Get<Unit*>("CurrentTarget", target) || !target)
        {
            _status = BTStatus::FAILURE;
            return _status;
        }

        // THREAD-SAFETY: Use AuraStateCache instead of direct HasAura() call.
        // The cache is populated from the main thread and provides thread-safe
        // read access for worker threads. This prevents race conditions with
        // Unit::_appliedAuras that caused ACCESS_VIOLATION crashes.
        bool hasAura = sAuraStateCache->HasCachedAura(target->GetGUID(), _spellId, bot->GetGUID());

        _status = (hasAura == _shouldHave) ? BTStatus::SUCCESS : BTStatus::FAILURE;
        return _status;
    }

private:
    uint32 _spellId;
    bool _shouldHave;
};

/**
 * @brief Check bot's health percentage threshold
 */
class TC_GAME_API BTCheckHealthPercent : public BTLeaf
{
public:
    enum class Comparison
    {
        LESS_THAN,
        LESS_EQUAL,
        GREATER_THAN,
        GREATER_EQUAL
    };

    BTCheckHealthPercent(float threshold, Comparison comparison)
        : BTLeaf("CheckHealthPercent"), _threshold(threshold), _comparison(comparison)
    {}

    BTStatus Tick(BotAI* ai, BTBlackboard& /*blackboard*/) override
    {
        if (!ai)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        Player* bot = ai->GetBot();
        if (!bot)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        float healthPct = bot->GetHealthPct() / 100.0f;

        bool result = false;
        switch (_comparison)
        {
            case Comparison::LESS_THAN:
                result = healthPct < _threshold;
                break;
            case Comparison::LESS_EQUAL:
                result = healthPct <= _threshold;
                break;
            case Comparison::GREATER_THAN:
                result = healthPct > _threshold;
                break;
            case Comparison::GREATER_EQUAL:
                result = healthPct >= _threshold;
                break;
        }

        _status = result ? BTStatus::SUCCESS : BTStatus::FAILURE;
        return _status;
    }

private:
    float _threshold;
    Comparison _comparison;
};

/**
 * @brief Check bot's resource (mana/rage/energy) percentage
 */
class TC_GAME_API BTCheckResourcePercent : public BTLeaf
{
public:
    enum class Comparison
    {
        LESS_THAN,
        LESS_EQUAL,
        GREATER_THAN,
        GREATER_EQUAL
    };

    BTCheckResourcePercent(Powers powerType, float threshold, Comparison comparison)
        : BTLeaf("CheckResourcePercent"), _powerType(powerType), _threshold(threshold), _comparison(comparison)
    {}

    BTStatus Tick(BotAI* ai, BTBlackboard& /*blackboard*/) override
    {
        if (!ai)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        Player* bot = ai->GetBot();
        if (!bot)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        uint32 maxPower = bot->GetMaxPower(_powerType);
        if (maxPower == 0)
        {
            _status = BTStatus::FAILURE;
            return _status;
        }

        float powerPct = bot->GetPower(_powerType) / static_cast<float>(maxPower);

        bool result = false;
        switch (_comparison)
        {
            case Comparison::LESS_THAN:
                result = powerPct < _threshold;
                break;
            case Comparison::LESS_EQUAL:
                result = powerPct <= _threshold;
                break;
            case Comparison::GREATER_THAN:
                result = powerPct > _threshold;
                break;
            case Comparison::GREATER_EQUAL:
                result = powerPct >= _threshold;
                break;
        }

        _status = result ? BTStatus::SUCCESS : BTStatus::FAILURE;
        return _status;
    }

private:
    Powers _powerType;
    float _threshold;
    Comparison _comparison;
};

/**
 * @brief Execute melee auto-attack
 */
class TC_GAME_API BTMeleeAttack : public BTAction
{
public:
    BTMeleeAttack()
        : BTAction("MeleeAttack",
            [](BotAI* ai, BTBlackboard& blackboard) -> BTStatus
            {
                if (!ai)
                    return BTStatus::INVALID;

                Player* bot = ai->GetBot();
                if (!bot)
                    return BTStatus::INVALID;

                Unit* target = nullptr;
                if (!blackboard.Get<Unit*>("CurrentTarget", target) || !target)
                    return BTStatus::FAILURE;

                // Check if already attacking this target
                if (bot->GetVictim() != target)
                    bot->Attack(target, true);

                return BTStatus::SUCCESS;
            })
    {}
};

/**
 * @brief Check if in combat
 */
class TC_GAME_API BTCheckInCombat : public BTCondition
{
public:
    BTCheckInCombat()
        : BTCondition("CheckInCombat",
            [](BotAI* ai, BTBlackboard& /*blackboard*/) -> bool
            {
                if (!ai)
                    return false;

                Player* bot = ai->GetBot();
                if (!bot)
                    return false;

                return bot->IsInCombat();
            })
    {}
};

/**
 * @brief Use defensive cooldown
 */
class TC_GAME_API BTUseDefensiveCooldown : public BTLeaf
{
public:
    BTUseDefensiveCooldown(uint32 spellId, float healthThreshold)
        : BTLeaf("UseDefensiveCooldown"), _spellId(spellId), _healthThreshold(healthThreshold)
    {}

    BTStatus Tick(BotAI* ai, BTBlackboard& /*blackboard*/) override
    {
        if (!ai)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        Player* bot = ai->GetBot();
        if (!bot)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        // Check health threshold
        if (bot->GetHealthPct() > _healthThreshold * 100.0f)
        {
            _status = BTStatus::FAILURE;
            return _status;
        }

        // Check if spell is ready
        if (!bot->HasSpell(_spellId) || bot->GetSpellHistory()->HasCooldown(_spellId))
        {
            _status = BTStatus::FAILURE;
            return _status;
        }

        // Cast on self
        ::SpellCastResult result = ai->CastSpell(_spellId, bot);

        _status = (result == SPELL_CAST_OK) ? BTStatus::SUCCESS : BTStatus::FAILURE;
        return _status;
    }

private:
    uint32 _spellId;
    float _healthThreshold;
};

/**
 * @brief Check if target is boss or elite
 */
class TC_GAME_API BTCheckTargetElite : public BTCondition
{
public:
    BTCheckTargetElite()
        : BTCondition("CheckTargetElite",
            [](BotAI* ai, BTBlackboard& blackboard) -> bool
            {
                if (!ai)
                    return false;

                Unit* target = nullptr;
                if (!blackboard.Get<Unit*>("CurrentTarget", target) || !target)
                    return false;

                Creature* creature = target->ToCreature();
                if (!creature)
                    return false;

                return creature->IsElite() || creature->IsDungeonBoss();
            })
    {}
};

} // namespace Playerbot

#endif // TRINITYCORE_COMBAT_NODES_H
