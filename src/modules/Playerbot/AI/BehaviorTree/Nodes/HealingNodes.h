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

#ifndef TRINITYCORE_HEALING_NODES_H
#define TRINITYCORE_HEALING_NODES_H

#include "AI/BehaviorTree/BehaviorTree.h"
#include "AI/BotAI.h"
#include "AI/Cache/AuraStateCache.h"
#include "Player.h"
#include "Unit.h"
#include "Group.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "GameTime.h"

namespace Playerbot
{

/**
 * @brief Find the most wounded ally in group
 */
class TC_GAME_API BTFindWoundedAlly : public BTLeaf
{
public:
    BTFindWoundedAlly(float healthThreshold = 0.95f)
        : BTLeaf("FindWoundedAlly"), _healthThreshold(healthThreshold)
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

        Group* group = bot->GetGroup();

        Unit* mostWoundedAlly = nullptr;
        float lowestHealthPct = _healthThreshold;

        // Check self first
        if (bot->GetHealthPct() / 100.0f < lowestHealthPct)
        {
            mostWoundedAlly = bot;
            lowestHealthPct = bot->GetHealthPct() / 100.0f;
        }

        // Check group members
        if (group)
        {
            for (GroupReference& ref : group->GetMembers())
            {
                Player* member = ref.GetSource();
                if (!member || !member->IsInWorld() || member->isDead())
                    continue;

                // Check if in range (40 yards)
                if (bot->GetDistance(member) > 40.0f)
                    continue;

                float memberHealthPct = member->GetHealthPct() / 100.0f;
                if (memberHealthPct < lowestHealthPct)
                {
                    mostWoundedAlly = member;
                    lowestHealthPct = memberHealthPct;
                }
            }
        }

        if (mostWoundedAlly)
        {
            blackboard.Set<Unit*>("HealTarget", mostWoundedAlly);
            blackboard.Set<float>("HealTargetHealthPct", lowestHealthPct);
            _status = BTStatus::SUCCESS;
        }
        else
        {
            _status = BTStatus::FAILURE;
        }

        return _status;
    }

private:
    float _healthThreshold;
};

/**
 * @brief Find ally with specific debuff that needs dispelling
 */
class TC_GAME_API BTFindDispelTarget : public BTLeaf
{
public:
    enum class DispelType
    {
        MAGIC,
        CURSE,
        DISEASE,
        POISON
    };

    BTFindDispelTarget(DispelType type)
        : BTLeaf("FindDispelTarget"), _dispelType(type)
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

        Group* group = bot->GetGroup();

        Unit* dispelTarget = nullptr;

        // Check self first
        if (HasDispellableDebuff(bot))
        {
            dispelTarget = bot;
        }

        // Check group members
        if (!dispelTarget && group)
        {
            for (GroupReference& ref : group->GetMembers())
            {
                Player* member = ref.GetSource();
                if (!member || !member->IsInWorld() || member->isDead())
                    continue;

                // Check if in range (40 yards)
                if (bot->GetDistance(member) > 40.0f)
                    continue;

                if (HasDispellableDebuff(member))
                {
                    dispelTarget = member;
                    break;
                }
            }
        }

        if (dispelTarget)
        {
            blackboard.Set<Unit*>("DispelTarget", dispelTarget);
            _status = BTStatus::SUCCESS;
        }
        else
        {
            _status = BTStatus::FAILURE;
        }

        return _status;
    }

private:
    DispelType _dispelType;

    bool HasDispellableDebuff(Unit* target) const
    {
        if (!target)
            return false;

        // ENHANCEMENT: Debuff detection and dispel type matching
        // Future implementation should:
        // - Iterate through target's active auras via Unit::GetAuraEffectsByType()
        // - Match aura dispel type against _dispelType (DISPEL_MAGIC, DISPEL_CURSE, DISPEL_DISEASE, DISPEL_POISON)
        // - Prioritize debuffs by severity (damage-over-time, crowd control, stat reduction)
        // - Return true if any matching dispellable debuff found
        // For now, return false as placeholder
        return false;
    }
};

/**
 * @brief Check if heal target is in line of sight
 */
class TC_GAME_API BTCheckHealTargetLoS : public BTCondition
{
public:
    BTCheckHealTargetLoS()
        : BTCondition("CheckHealTargetLoS",
            [](BotAI* ai, BTBlackboard& blackboard) -> bool
            {
                if (!ai)
                    return false;

                Player* bot = ai->GetBot();
                if (!bot)
                    return false;

                Unit* healTarget = nullptr;
                if (!blackboard.Get<Unit*>("HealTarget", healTarget) || !healTarget)
                    return false;

                return bot->IsWithinLOSInMap(healTarget);
            })
    {}
};

/**
 * @brief Check if heal target is in range
 */
class TC_GAME_API BTCheckHealTargetInRange : public BTLeaf
{
public:
    BTCheckHealTargetInRange(float maxRange)
        : BTLeaf("CheckHealTargetInRange"), _maxRange(maxRange)
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

        Unit* healTarget = nullptr;
        if (!blackboard.Get<Unit*>("HealTarget", healTarget) || !healTarget)
        {
            _status = BTStatus::FAILURE;
            return _status;
        }

        float distance = bot->GetDistance(healTarget);

        if (distance <= _maxRange)
        {
            blackboard.Set<float>("HealTargetDistance", distance);
            _status = BTStatus::SUCCESS;
        }
        else
        {
            _status = BTStatus::FAILURE;
        }

        return _status;
    }

private:
    float _maxRange;
};

/**
 * @brief Cast heal spell on heal target
 */
class TC_GAME_API BTCastHeal : public BTLeaf
{
public:
    BTCastHeal(uint32 spellId)
        : BTLeaf("CastHeal"), _spellId(spellId), _castStartTime(0)
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

        Unit* healTarget = nullptr;
        if (!blackboard.Get<Unit*>("HealTarget", healTarget) || !healTarget)
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

            // Check if spell is ready
            if (!bot->HasSpell(_spellId) || bot->GetSpellHistory()->HasCooldown(_spellId))
            {
                _status = BTStatus::FAILURE;
                Reset();
                return _status;
            }

            // Check mana
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(_spellId, DIFFICULTY_NONE);
            if (spellInfo)
            {
                auto costs = spellInfo->CalcPowerCost(bot, spellInfo->GetSchoolMask());
                for (auto const& cost : costs)
                {
                    if (cost.Power == POWER_MANA && bot->GetPower(POWER_MANA) < cost.Amount)
                    {
                        _status = BTStatus::FAILURE;
                        Reset();
                        return _status;
                    }
                }
            }

            // Attempt to cast
            ::SpellCastResult result = ai->CastSpell(_spellId, healTarget);

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
 * @brief Select appropriate heal spell based on target health deficit
 */
class TC_GAME_API BTSelectHealSpell : public BTLeaf
{
public:
    struct HealSpellOption
    {
        uint32 spellId;
        float healthThreshold; // Use this spell if target health < threshold
        float manaCost; // Relative mana cost (for efficiency)
    };

    BTSelectHealSpell(::std::vector<HealSpellOption> const& spells)
        : BTLeaf("SelectHealSpell"), _spells(spells)
    {
        // Sort by health threshold (descending)
        ::std::sort(_spells.begin(), _spells.end(),
            [](HealSpellOption const& a, HealSpellOption const& b)
            {
                return a.healthThreshold > b.healthThreshold;
            });
    }

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

        float targetHealthPct = 1.0f;
        if (!blackboard.Get<float>("HealTargetHealthPct", targetHealthPct))
        {
            _status = BTStatus::FAILURE;
            return _status;
        }

        // Find appropriate heal spell
        for (auto const& spellOption : _spells)
        {
            if (targetHealthPct < spellOption.healthThreshold)
            {
                // Check if spell is ready
                if (bot->HasSpell(spellOption.spellId) && !bot->GetSpellHistory()->HasCooldown(spellOption.spellId))
                {
                    blackboard.Set<uint32>("SelectedHealSpell", spellOption.spellId);
                    _status = BTStatus::SUCCESS;
                    return _status;
                }
            }
        }

        _status = BTStatus::FAILURE;
        return _status;
    }

private:
    ::std::vector<HealSpellOption> _spells;
};

/**
 * @brief Cast dispel spell on dispel target
 */
class TC_GAME_API BTCastDispel : public BTAction
{
public:
    BTCastDispel(uint32 spellId)
        : BTAction("CastDispel",
            [spellId](BotAI* ai, BTBlackboard& blackboard) -> BTStatus
            {
                if (!ai)
                    return BTStatus::INVALID;

                Player* bot = ai->GetBot();
                if (!bot)
                    return BTStatus::INVALID;

                Unit* dispelTarget = nullptr;
                if (!blackboard.Get<Unit*>("DispelTarget", dispelTarget) || !dispelTarget)
                    return BTStatus::FAILURE;

                // Check if spell is ready
                if (!bot->HasSpell(spellId) || bot->GetSpellHistory()->HasCooldown(spellId))
                    return BTStatus::FAILURE;

                ::SpellCastResult result = ai->CastSpell(spellId, dispelTarget);
                return (result == SPELL_CAST_OK) ? BTStatus::SUCCESS : BTStatus::FAILURE;
            })
    {}
};

/**
 * @brief Check if group needs area heal (multiple wounded members)
 */
class TC_GAME_API BTCheckGroupNeedsAoEHeal : public BTLeaf
{
public:
    BTCheckGroupNeedsAoEHeal(float healthThreshold, uint32 minWoundedCount)
        : BTLeaf("CheckGroupNeedsAoEHeal"), _healthThreshold(healthThreshold), _minWoundedCount(minWoundedCount)
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

        Group* group = bot->GetGroup();
        if (!group)
        {
            _status = BTStatus::FAILURE;
            return _status;
        }

        uint32 woundedCount = 0;

        for (GroupReference& ref : group->GetMembers())
        {
            Player* member = ref.GetSource();
            if (!member || !member->IsInWorld() || member->isDead())
                continue;

            // Check if in range (40 yards)
            if (bot->GetDistance(member) > 40.0f)
                continue;

            if (member->GetHealthPct() / 100.0f < _healthThreshold)
                woundedCount++;
        }

        if (woundedCount >= _minWoundedCount)
        {
            blackboard.Set<uint32>("WoundedAllyCount", woundedCount);
            _status = BTStatus::SUCCESS;
        }
        else
        {
            _status = BTStatus::FAILURE;
        }

        return _status;
    }

private:
    float _healthThreshold;
    uint32 _minWoundedCount;
};

/**
 * @brief Cast area heal spell (Circle of Healing, Chain Heal, Prayer of Healing, etc.)
 */
class TC_GAME_API BTCastAoEHeal : public BTLeaf
{
public:
    BTCastAoEHeal(uint32 spellId)
        : BTLeaf("CastAoEHeal"), _spellId(spellId), _castStartTime(0)
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

        // If not casting, start cast
        if (_castStartTime == 0)
        {
            if (bot->IsNonMeleeSpellCast(false))
            {
                _status = BTStatus::RUNNING;
                return _status;
            }

            // Check if spell is ready
            if (!bot->HasSpell(_spellId) || bot->GetSpellHistory()->HasCooldown(_spellId))
            {
                _status = BTStatus::FAILURE;
                Reset();
                return _status;
            }

            // Check mana
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(_spellId, DIFFICULTY_NONE);
            if (spellInfo)
            {
                auto costs = spellInfo->CalcPowerCost(bot, spellInfo->GetSchoolMask());
                for (auto const& cost : costs)
                {
                    if (cost.Power == POWER_MANA && bot->GetPower(POWER_MANA) < cost.Amount)
                    {
                        _status = BTStatus::FAILURE;
                        Reset();
                        return _status;
                    }
                }
            }

            // Cast on self (AoE heal will affect nearby allies)
            ::SpellCastResult result = ai->CastSpell(_spellId, bot);

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
 * @brief Check if bot has HoT (Heal over Time) buff active on target
 */
class TC_GAME_API BTCheckHoTActive : public BTLeaf
{
public:
    BTCheckHoTActive(uint32 spellId)
        : BTLeaf("CheckHoTActive"), _spellId(spellId)
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

        Unit* healTarget = nullptr;
        if (!blackboard.Get<Unit*>("HealTarget", healTarget) || !healTarget)
        {
            _status = BTStatus::FAILURE;
            return _status;
        }

        // THREAD-SAFETY: Use AuraStateCache instead of direct HasAura() call.
        // The cache is populated from the main thread and provides thread-safe
        // read access for worker threads. This prevents race conditions with
        // Unit::_appliedAuras that caused ACCESS_VIOLATION crashes.
        bool hasHoT = sAuraStateCache->HasCachedAura(healTarget->GetGUID(), _spellId, bot->GetGUID());

        _status = hasHoT ? BTStatus::SUCCESS : BTStatus::FAILURE;
        return _status;
    }

private:
    uint32 _spellId;
};

/**
 * @brief Check if bot role is healer
 */
class TC_GAME_API BTCheckIsHealer : public BTCondition
{
public:
    BTCheckIsHealer()
        : BTCondition("CheckIsHealer",
            [](BotAI* ai, BTBlackboard& /*blackboard*/) -> bool
            {
                if (!ai)
                    return false;

                Player* bot = ai->GetBot();
                if (!bot)
                    return false;

                uint8 classId = bot->GetClass();
                uint8 spec = uint8(bot->GetPrimarySpecialization());

                // Healer specs
                if (classId == CLASS_PRIEST && (spec == 1 || spec == 2)) // Discipline or Holy
                    return true;
                if (classId == CLASS_PALADIN && spec == 0) // Holy
                    return true;
                if (classId == CLASS_SHAMAN && spec == 2) // Restoration
                    return true;
                if (classId == CLASS_DRUID && spec == 2) // Restoration
                    return true;

                return false;
            })
    {}
};

} // namespace Playerbot

#endif // TRINITYCORE_HEALING_NODES_H
