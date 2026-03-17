/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Role-Based Specialization Templates
 *
 * This file provides additional role-specific template specializations
 * and utility classes for the combat specialization system.
 */

#pragma once

#include "CombatSpecializationTemplates.h"
#include "ObjectGuid.h"
#include "../../../Spatial/SpatialGridManager.h"
#include "Creature.h"
#include "Map.h"
#include "ResourceTypes.h"
#include "SpellMgr.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "GridNotifiers.h"
#include "CellImpl.h"
#include "../../Services/HealingTargetSelector.h"  // Phase 5B: Unified healing service
#include <ranges>
#include <span>
#include <vector>

namespace Playerbot
{

// ============================================================================
// HYBRID SPECIALIZATIONS - For classes that can fulfill multiple roles
// ============================================================================

/**
 * Hybrid DPS/Healer Specialization (e.g., Discipline Priest, Mistweaver Monk)
 */
template<typename ResourceType>
    requires ValidResource<ResourceType>
class HybridDpsHealerSpecialization : public CombatSpecializationTemplate<ResourceType>
{
public:
    explicit HybridDpsHealerSpecialization(Player* bot)
        : CombatSpecializationTemplate<ResourceType>(bot)
        , _healingMode(false)
        , _damageDealtThisCombat(0)
        , _healingDoneThisCombat(0)
    {
    }

    float GetOptimalRange(::Unit* target) override final
    {
        return _healingMode ? 30.0f : 25.0f;
    }

protected:
    /**
     * Dynamically switch between damage and healing based on situation
     */
    void UpdateMode()
    {
        // Check if allies need healing
        uint32 injuredAllies = CountInjuredAllies(0.7f);
        bool criticalHealing = HasCriticallyInjuredAlly(0.3f);

        if (criticalHealing || injuredAllies >= 2)
        {

            _healingMode = true;
        }
        else if (injuredAllies == 0)
        {

            _healingMode = false;
        }

        // Stay in current mode if situation is unclear
    }

    /**
     * Select target based on current mode
     */
    Unit* SelectTarget()
    {
        if (_healingMode)
        {

            return SelectHealingTarget();
        }
        else
        {

            return SelectDamageTarget();
        }
    }

    Unit* SelectHealingTarget()
    {
        // Use unified HealingTargetSelector service (Phase 5B integration)
        // Eliminates 40+ lines of duplicated healing target logic
        return bot::ai::HealingTargetSelector::SelectTarget(this->GetBot());
    }

    Unit* SelectDamageTarget()
    {
        // Prioritize low health enemies we can finish off
        Unit* bestTarget = nullptr;
        float lowestHealth = 100.0f;

        ::std::list<Unit*> hostileUnits;
        Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(this->GetBot(), this->GetBot(), 40.0f);
        Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> u_search(this->GetBot(), hostileUnits, u_check);
        this->GetBot()->VisitNearbyObject(40.0f, u_search);

        for (Unit* hostile : hostileUnits)
        {

            if (hostile->IsAlive() && this->GetBot()->CanSeeOrDetect(hostile))

            {

                float healthPct = hostile->GetHealthPct();

                if (healthPct < lowestHealth)

                {

                    lowestHealth = healthPct;

                    bestTarget = hostile;

                }

            }
        }

        return bestTarget;
    }    uint32 CountInjuredAllies(float threshold) const
    {
        uint32 count = 0;

        if (Group* group = this->GetBot()->GetGroup())
        {

            for (GroupReference* ref : *group)

            {

                if (Player* member = ref->GetSource())

                {

                    if (member->IsAlive() && member->GetHealthPct() < threshold * 100.0f)
                    count++;

                }

            }
        }

        return count;
    }

    bool HasCriticallyInjuredAlly(float threshold) const
    {
        if (Group* group = this->GetBot()->GetGroup())
        {

            for (GroupReference* ref : *group)
            {
            if (Player* member = ref->GetSource())
            {

                    if (member->IsAlive() && member->GetHealthPct() < threshold * 100.0f)
                    return true;

                }

            }
        }

        return false;
    }

    bool IsTank(Player* player) const
    {
        // Simple check - could be enhanced with role detection

        return player->GetClass() == CLASS_WARRIOR ||
        player->GetClass() == CLASS_PALADIN ||
        player->GetClass() == CLASS_DEATH_KNIGHT;
        }

protected:
    bool _healingMode;
    uint64 _damageDealtThisCombat;
    uint64 _healingDoneThisCombat;
};

// ============================================================================
// SPECIALIZED TANK TYPES
// ============================================================================

/**
 * Avoidance Tank Specialization (e.g., Brewmaster Monk)
 */
template<typename ResourceType>
    requires ValidResource<ResourceType>
class AvoidanceTankSpecialization : public TankSpecialization<ResourceType>
{
public:
    explicit AvoidanceTankSpecialization(Player* bot)
        : TankSpecialization<ResourceType>(bot)
        , _staggerAmount(0)
        , _lastStaggerPurge(0)
    {
    }

protected:
    /**
     * Manage stagger mechanic (Brewmaster specific)
     */
    void ManageStagger()
    {
        uint32 currentTime = GameTime::GetGameTimeMS();

        // Accumulate stagger from recent damage
        float staggerPct = static_cast<float>(_staggerAmount) / this->GetBot()->GetMaxHealth();

        if (staggerPct > 0.6f) // Heavy stagger
        {

            if (currentTime - _lastStaggerPurge > 3000) // 3 second cooldown

            {

                PurgeStagger();

                _lastStaggerPurge = currentTime;

            }
        }
    }

    virtual void PurgeStagger() = 0; // Class-specific implementation

    void OnDamageTaken(Unit* /*attacker*/, uint32 damage) override
    {
        // Convert portion of damage to stagger
        uint32 staggeredDamage = damage * 0.4f; // 40% staggered
        _staggerAmount += staggeredDamage;
    }

private:
    uint32 _staggerAmount;
    uint32 _lastStaggerPurge;
};

/**
 * Shield Tank Specialization (e.g., Protection Warrior/Paladin)
 */
template<typename ResourceType>
    requires ValidResource<ResourceType>
class ShieldTankSpecialization : public TankSpecialization<ResourceType>
{
public:
    explicit ShieldTankSpecialization(Player* bot)
        : TankSpecialization<ResourceType>(bot)
        , _lastShieldBlock(0)
        , _shieldBlockCharges(2)
    {
    }

protected:
    /**
     * Manage active mitigation through shield blocks
     */
    void ManageShieldBlock()
    {
        uint32 currentTime = GameTime::GetGameTimeMS();

        // Regenerate shield block charges
        if (currentTime - _lastShieldBlock > 12000) // 12 second recharge
        {

            _shieldBlockCharges = ::std::min<uint32>(_shieldBlockCharges + 1, 2);
        }

        // Use shield block on high damage
        if (this->GetBot()->GetHealthPct() < 70.0f && _shieldBlockCharges > 0)
        {

            UseShieldBlock();

            _shieldBlockCharges--;

            _lastShieldBlock = currentTime;
        }
    }

    virtual void UseShieldBlock() = 0; // Class-specific implementation

private:
    uint32 _lastShieldBlock;
    uint32 _shieldBlockCharges;
};

// ============================================================================
// SPECIALIZED DPS TYPES
// ============================================================================

/**
 * DoT (Damage over Time) Specialization (e.g., Affliction Warlock, Shadow Priest)
 */
template<typename ResourceType>
    requires ValidResource<ResourceType>
class DotDpsSpecialization : public RangedDpsSpecialization<ResourceType>
{
public:
    explicit DotDpsSpecialization(Player* bot)
        : RangedDpsSpecialization<ResourceType>(bot)
        , _maxDotsPerTarget(5)
    {
    }

protected:
    /**
     * Manage DoT applications across multiple targets
     */
    void ManageDoTs()
    {
        // Get all valid targets
        ::std::list<Unit*> targets = GetValidDotTargets();

        // Sort by missing DoTs
        targets.sort([this](Unit* a, Unit* b) {

            return GetMissingDotCount(a) > GetMissingDotCount(b);
        });

        // Apply DoTs to targets
        for (Unit* target : targets)
        {

            if (GetMissingDotCount(target) > 0)

            {

                ApplyMissingDoTs(target);

            }

            else

            {

                RefreshExpiringDoTs(target);

            }
        }
    }

    ::std::list<Unit*> GetValidDotTargets() const
    {
        ::std::list<Unit*> targets;        ::std::list<Unit*> hostileUnits;
        Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(this->GetBot(), this->GetBot(), 40.0f);
        Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> u_search(this->GetBot(), hostileUnits, u_check);
        this->GetBot()->VisitNearbyObject(40.0f, u_search);

        for (Unit* hostile : hostileUnits)
        {

            if (hostile->IsAlive() &&

                this->GetBot()->CanSeeOrDetect(hostile) &&

                hostile->GetHealthPct() > 20.0f) // Don't DoT low health targets

            {

                targets.push_back(hostile);

            }
        }

        return targets;
    }

    uint32 GetMissingDotCount(Unit* target) const
    {
        uint32 missingCount = 0;
        auto it = this->_activeDots.find(target->GetGUID().GetRawValue());
        if (it == this->_activeDots.end())
        {

            return _maxDotsPerTarget; // No DoTs applied
        }

        // Count missing DoTs based on specialization
        for (uint32 dotSpell : GetRequiredDotSpells())
        {

            if (it->second.find(dotSpell) == it->second.end())

            {

                missingCount++;

            }
        }

        return missingCount;
    }

    void RefreshExpiringDoTs(Unit* target)
    {
        auto it = this->_activeDots.find(target->GetGUID().GetRawValue());
        if (it == this->_activeDots.end())

            return;

        for (auto& [spellId, duration] : it->second)
        {

            if (duration < 3000) // Refresh if less than 3 seconds

            {

                RefreshDot(target, spellId);

            }
        }
    }

    virtual void ApplyMissingDoTs(Unit* target) = 0;
    virtual void RefreshDot(Unit* target, uint32 spellId) = 0;
    virtual ::std::vector<uint32> GetRequiredDotSpells() const = 0;

private:
    uint32 _maxDotsPerTarget;
};

/**
 * Burst DPS Specialization (e.g., Arcane Mage, Subtlety Rogue)
 */
template<typename ResourceType>
    requires ValidResource<ResourceType>
class BurstDpsSpecialization : public CombatSpecializationTemplate<ResourceType>
{
public:
    explicit BurstDpsSpecialization(Player* bot)
        : CombatSpecializationTemplate<ResourceType>(bot)
        , _burstWindowActive(false)
        , _lastBurstTime(0)
        , _burstCooldown(120000) // 2 minutes
    {
    }

protected:
    /**
     * Manage burst windows for maximum damage
     */
    void ManageBurstWindow()
    {
        uint32 currentTime = GameTime::GetGameTimeMS();

        // Check if burst is available
        if (!_burstWindowActive && currentTime - _lastBurstTime > _burstCooldown)
        {

            if (ShouldStartBurst())

            {

                StartBurstWindow();

                _burstWindowActive = true;

                _lastBurstTime = currentTime;

                _burstEndTime = currentTime + GetBurstDuration();

            }
        }

        // Check if burst should end
        if (_burstWindowActive && currentTime > _burstEndTime)
        {

            EndBurstWindow();

            _burstWindowActive = false;
        }
    }

    virtual bool ShouldStartBurst() const
    {
        // Default logic - start burst on high health boss or multiple enemies
        if (this->_currentTarget)
        {

            return this->_currentTarget->GetHealthPct() > 80.0f ||

                   CountNearbyEnemies() >= 3;
        }
        return false;
    }

    uint32 CountNearbyEnemies() const
    {
        uint32 count = 0;
        Player* bot = this->GetBot();
        if (!bot)

            return 0;

        ::std::list<Unit*> hostileUnits;
        Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(bot, bot, 10.0f);
        Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> u_searcher(bot, hostileUnits, u_check);
        // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitAllObjects
        Map* map = bot->GetMap();
        if (map)
        {

            auto* spatialGrid = Playerbot::SpatialGridManager::Instance().GetGrid(map);

            if (spatialGrid)

            {

                auto guids = spatialGrid->QueryNearbyCreatureGuids(*bot, 10.0f);

                for (ObjectGuid guid : guids)

                {

                    if (Creature* creature = ObjectAccessor::GetCreature(*bot, guid))

                    {

                        if (u_check(creature))

                            hostileUnits.push_back(creature);

                    }

                }

            }
        }

        for (Unit* hostile : hostileUnits)
        {

            if (hostile->IsAlive())

                count++;
        }

        return count;
    }

    virtual void StartBurstWindow() = 0;
    virtual void EndBurstWindow() = 0;
    virtual uint32 GetBurstDuration() const { return 15000; } // 15 seconds default

    bool IsInBurstWindow() const { return _burstWindowActive; }

private:
    bool _burstWindowActive;
    uint32 _lastBurstTime;
    uint32 _burstEndTime;
    uint32 _burstCooldown;
};

// ============================================================================
// UTILITY CLASSES FOR SPECIALIZATIONS
// ============================================================================

/**
 * Rotation priority system for ability usage
 */
class RotationPriority
{
public:
    struct AbilityPriority
    {
        uint32 spellId;
        float priority;
        ::std::function<bool()> condition;
    };

    void AddAbility(uint32 spellId, float priority, ::std::function<bool()> condition = nullptr)
    {
        _abilities.push_back({spellId, priority, condition});
        SortAbilities();
    }

    uint32 GetNextAbility()
    {
        for (const auto& ability : _abilities)
        {

            if (!ability.condition || ability.condition())

            {

                return ability.spellId;

            }
        }
        return 0;
    }    void UpdatePriority(uint32 spellId, float newPriority)
    {
        auto it = ::std::find_if(_abilities.begin(), _abilities.end(),

            [spellId](const AbilityPriority& a) { return a.spellId == spellId; });

        if (it != _abilities.end())
        {

            it->priority = newPriority;

            SortAbilities();
        }
    }

private:
    void SortAbilities()
    {
        ::std::sort(_abilities.begin(), _abilities.end(),

            [](const AbilityPriority& a, const AbilityPriority& b) {

                return a.priority > b.priority;

            });
    }

    ::std::vector<AbilityPriority> _abilities;
};

/**
 * Snapshot system for DoT/HoT calculations
 */
class SnapshotManager{
public:
    struct Snapshot
    {
        uint32 spellId;
        uint32 targetGuid;
        float spellPower;
        float critChance;
        float haste;
        uint32 timestamp;
    };

    void TakeSnapshot(uint32 spellId, Unit* target, Player* caster)
    {
        Snapshot snap;
        snap.spellId = spellId;
        snap.targetGuid = target->GetGUID().GetCounter();
        snap.spellPower = caster->GetTotalAttackPowerValue(BASE_ATTACK);
        snap.critChance = caster->GetUnitCriticalChanceAgainst(BASE_ATTACK, target);
        snap.haste = caster->GetRatingBonusValue(CR_HASTE_MELEE);
        snap.timestamp = GameTime::GetGameTimeMS();

        _snapshots[GetKey(spellId, target)] = snap;
    }

    bool HasSnapshot(uint32 spellId, Unit* target) const
    {
        return _snapshots.find(GetKey(spellId, target)) != _snapshots.end();
    }

    const Snapshot& GetSnapshot(uint32 spellId, Unit* target) const
    {
        static Snapshot empty;
        auto it = _snapshots.find(GetKey(spellId, target));
        return (it != _snapshots.end()) ? it->second : empty;
    }

    void RemoveSnapshot(uint32 spellId, Unit* target)
    {
        _snapshots.erase(GetKey(spellId, target));
    }

private:
    uint64 GetKey(uint32 spellId, Unit* target) const
    {
        return (static_cast<uint64>(spellId) << 32) | target->GetGUID().GetCounter();
    }

    ::std::unordered_map<uint64, Snapshot> _snapshots;
};

} // namespace Playerbot