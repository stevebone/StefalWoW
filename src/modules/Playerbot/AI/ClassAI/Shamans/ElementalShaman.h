/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
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

#ifndef PLAYERBOT_ELEMENTALSHAMANREFACTORED_H
#define PLAYERBOT_ELEMENTALSHAMANREFACTORED_H

#include "../CombatSpecializationTemplates.h"
#include "Player.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "ObjectGuid.h"
#include <unordered_map>
#include "Log.h"
// Phase 5 Integration: Decision Systems
#include "../../Decision/ActionPriorityQueue.h"
#include "../../Decision/BehaviorTree.h"
#include "../BotAI.h"

// Central Spell Registry - See WoW120Spells::Shaman namespace
#include "../SpellValidation_WoW120.h"
#include "../SpellValidation_WoW120_Part2.h"
#include "../HeroTalentDetector.h"      // Hero talent tree detection

namespace Playerbot
{


// Import BehaviorTree helper functions (avoid conflict with Playerbot::Action)
using bot::ai::Sequence;
using bot::ai::Selector;
using bot::ai::Condition;
using bot::ai::Inverter;
using bot::ai::Repeater;
using bot::ai::NodeStatus;
using bot::ai::SpellPriority;
using bot::ai::SpellCategory;

// Note: bot::ai::Action() conflicts with Playerbot::Action, use bot::ai::Action() explicitly
// WoW 12.0 (The War Within) - Elemental Shaman Spell IDs
// Using central registry: WoW120Spells::Shaman and WoW120Spells::Shaman::Elemental
constexpr uint32 ELEM_LIGHTNING_BOLT = WoW120Spells::Shaman::LIGHTNING_BOLT;
constexpr uint32 ELEM_LAVA_BURST = WoW120Spells::Shaman::LAVA_BURST;
constexpr uint32 ELEM_FLAME_SHOCK = WoW120Spells::Shaman::FLAME_SHOCK;
constexpr uint32 ELEM_EARTH_SHOCK = WoW120Spells::Shaman::EARTH_SHOCK;
constexpr uint32 ELEM_FROST_SHOCK = WoW120Spells::Shaman::FROST_SHOCK;
constexpr uint32 ELEM_CHAIN_LIGHTNING = WoW120Spells::Shaman::CHAIN_LIGHTNING;
constexpr uint32 ELEM_EARTHQUAKE = WoW120Spells::Shaman::Elemental::EARTHQUAKE;
constexpr uint32 ELEM_FIRE_ELEMENTAL = WoW120Spells::Shaman::Elemental::FIRE_ELEMENTAL;
constexpr uint32 ELEM_STORMKEEPER = WoW120Spells::Shaman::Elemental::STORMKEEPER;
constexpr uint32 ELEM_ASCENDANCE = WoW120Spells::Shaman::Elemental::ASCENDANCE;
constexpr uint32 ELEM_LIQUID_MAGMA_TOTEM = WoW120Spells::Shaman::Elemental::LIQUID_MAGMA_TOTEM;
constexpr uint32 ELEM_ICEFURY = WoW120Spells::Shaman::Elemental::ICEFURY;
constexpr uint32 ELEM_ELEMENTAL_BLAST = WoW120Spells::Shaman::Elemental::ELEMENTAL_BLAST;
constexpr uint32 ELEM_ECHOING_SHOCK = WoW120Spells::Shaman::Elemental::ECHOING_SHOCK;
constexpr uint32 ELEM_PRIMORDIAL_WAVE = WoW120Spells::Shaman::Elemental::PRIMORDIAL_WAVE;
constexpr uint32 ELEM_ASTRAL_SHIFT = WoW120Spells::Shaman::ASTRAL_SHIFT;
constexpr uint32 ELEM_EARTH_SHIELD = WoW120Spells::Shaman::Restoration::EARTH_SHIELD;
constexpr uint32 ELEM_WIND_SHEAR = WoW120Spells::Shaman::WIND_SHEAR;
constexpr uint32 ELEM_CAPACITOR_TOTEM = WoW120Spells::Shaman::CAPACITOR_TOTEM;

// ManaResource is already defined in CombatSpecializationTemplates.h
// No need to redefine it here

// Maelstrom tracker (tracked separately, not part of resource concept)
class MaelstromTracker
{
public:
    MaelstromTracker() : _maelstrom(0), _maxMaelstrom(100) {}

    void Generate(uint32 amount)
    {
        _maelstrom = ::std::min(_maelstrom + amount, _maxMaelstrom);
    }

    void Spend(uint32 amount)
    {
        if (_maelstrom >= amount)
            _maelstrom -= amount;
        else
            _maelstrom = 0;
    }

    [[nodiscard]] bool Has(uint32 amount) const { return _maelstrom >= amount; }
    [[nodiscard]] uint32 GetPercent() const { return (_maelstrom * 100) / _maxMaelstrom; }
    [[nodiscard]] uint32 GetCurrent() const { return _maelstrom; }

    void Update(Player* bot)
    {
        // Maelstrom is tracked via game mechanics, this is simplified tracking
    }

private:
    uint32 _maelstrom;
    uint32 _maxMaelstrom;
};

// Flame Shock DoT tracker
class FlameShockTracker
{
public:
    FlameShockTracker() = default;

    void ApplyFlameShock(ObjectGuid guid, uint32 duration = 18000)
    {
        _flameShockTargets[guid] = GameTime::GetGameTimeMS() + duration;
    }

    void RemoveFlameShock(ObjectGuid guid)
    {
        _flameShockTargets.erase(guid);
    }

    [[nodiscard]] bool HasFlameShock(ObjectGuid guid) const
    {
        auto it = _flameShockTargets.find(guid);
        if (it == _flameShockTargets.end())
            return false;
        return GameTime::GetGameTimeMS() < it->second;
    }

    [[nodiscard]] uint32 GetFlameShockTimeRemaining(ObjectGuid guid) const
    {
        auto it = _flameShockTargets.find(guid);
        if (it == _flameShockTargets.end())
            return 0;

        uint32 now = GameTime::GetGameTimeMS();
        if (now >= it->second)
            return 0;

        return it->second - now;
    }

    [[nodiscard]] bool NeedsFlameShockRefresh(ObjectGuid guid, uint32 pandemicWindow = 5400) const
    {
        uint32 remaining = GetFlameShockTimeRemaining(guid);
        return remaining < pandemicWindow;
    }

    [[nodiscard]] uint32 GetActiveFlameShockCount() const
    {
        uint32 count = 0;
        uint32 now = GameTime::GetGameTimeMS();
        for (const auto& pair : _flameShockTargets)
        {
            if (now < pair.second)
                ++count;
        }
        return count;
    }

    void Update(Player* bot)
    {
        if (!bot)
            return;

        // Clean up expired Flame Shocks
        uint32 now = GameTime::GetGameTimeMS();
        for (auto it = _flameShockTargets.begin(); it != _flameShockTargets.end();)
        {
            if (now >= it->second)
                it = _flameShockTargets.erase(it);
            else
                ++it;
        }
    }

private:
    ::std::unordered_map<ObjectGuid, uint32> _flameShockTargets; // GUID -> expiration time
};

// Lava Surge proc tracker (instant Lava Burst)
class LavaSurgeTracker
{
public:
    LavaSurgeTracker() : _lavaSurgeActive(false), _lavaSurgeEndTime(0) {}

    void ActivateProc()
    {
        _lavaSurgeActive = true;
        _lavaSurgeEndTime = GameTime::GetGameTimeMS() + 15000; // 15 sec duration
    }

    void ConsumeProc()
    {
        _lavaSurgeActive = false;
    }

    [[nodiscard]] bool IsActive() const
    {
        return _lavaSurgeActive && GameTime::GetGameTimeMS() < _lavaSurgeEndTime;
    }

    void Update(Player* bot)
    {
        if (!bot)
            return;

        // Check if Lava Surge buff is active
        if (bot->HasAura(77762)) // Lava Surge buff ID
        {
            _lavaSurgeActive = true;
            if (Aura* aura = bot->GetAura(77762))
                _lavaSurgeEndTime = GameTime::GetGameTimeMS() + aura->GetDuration();
        }
        else
        {
            _lavaSurgeActive = false;
        }

        // Expire if time is up
        if (_lavaSurgeActive && GameTime::GetGameTimeMS() >= _lavaSurgeEndTime)
            _lavaSurgeActive = false;
    }

private:
    bool _lavaSurgeActive;
    uint32 _lavaSurgeEndTime;
};

// Stormkeeper proc tracker (instant Lightning Bolts)
class StormkeeperTracker
{
public:
    StormkeeperTracker() : _stormkeeperStacks(0), _stormkeeperEndTime(0) {}

    void ActivateProc(uint32 stacks = 2)
    {
        _stormkeeperStacks = stacks;
        _stormkeeperEndTime = GameTime::GetGameTimeMS() + 15000; // 15 sec duration
    }

    void ConsumeStack()
    {
        if (_stormkeeperStacks > 0)
            _stormkeeperStacks--;
    }

    [[nodiscard]] bool HasStack() const
    {
        return _stormkeeperStacks > 0 && GameTime::GetGameTimeMS() < _stormkeeperEndTime;
    }

    [[nodiscard]] uint32 GetStacks() const { return _stormkeeperStacks; }

    void Update(Player* bot)
    {
        if (!bot)
            return;

        // Check if Stormkeeper buff is active
        if (Aura* aura = bot->GetAura(ELEM_STORMKEEPER))
        {
            _stormkeeperStacks = aura->GetStackAmount();
            _stormkeeperEndTime = GameTime::GetGameTimeMS() + aura->GetDuration();
        }
        else
        {
            _stormkeeperStacks = 0;
        }

        // Expire if time is up
        if (_stormkeeperStacks > 0 && GameTime::GetGameTimeMS() >= _stormkeeperEndTime)
            _stormkeeperStacks = 0;
    }

private:
    uint32 _stormkeeperStacks;
    uint32 _stormkeeperEndTime;
};

class ElementalShamanRefactored : public RangedDpsSpecialization<ManaResource>
{
public:
    // Use base class members with type alias for cleaner syntax
    using Base = RangedDpsSpecialization<ManaResource>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::_resource;
    explicit ElementalShamanRefactored(Player* bot)
        : RangedDpsSpecialization<ManaResource>(bot)
        , _maelstromTracker()
        , _flameShockTracker()
        , _lavaSurgeTracker()
        , _stormkeeperTracker()
        , _ascendanceActive(false)
        , _ascendanceEndTime(0)
        , _lastAscendanceTime(0)
        , _lastEchoingShockTime(0)
        , _lastPrimordialWaveTime(0)
        , _lastFireElementalTime(0)
        , _lastStormkeeperTime(0)
    {
        // Register cooldowns for major abilities
        // COMMENTED OUT:         _cooldowns.RegisterBatch({
        // COMMENTED OUT:             {ELEMENTAL_FIRE_ELEMENTAL, 150000, 1},
        // COMMENTED OUT:             {ELEMENTAL_STORM_ELEMENTAL, 150000, 1},
        // COMMENTED OUT:             {ELEMENTAL_STORMKEEPER, 60000, 1},
        // COMMENTED OUT:             {ELEMENTAL_LIQUID_MAGMA_TOTEM, 60000, 1},
        // COMMENTED OUT:             {ELEMENTAL_ASCENDANCE, 180000, 1}
        // COMMENTED OUT:         });

        // Resource initialization handled by base class CombatSpecializationTemplate
        // SAFETY: GetName() removed from constructor to prevent ACCESS_VIOLATION crash during worker thread bot login

        // Phase 5: Initialize decision systems
        InitializeElementalMechanics();
    }

    void UpdateRotation(::Unit* target) override
    {
        Player* bot = this->GetBot();
        if (!target || !bot)
            return;

        // Detect hero talents if not yet cached
        if (!_heroTalents.detected)
            _heroTalents.Refresh(this->GetBot());

        // Hero talent rotation branches
        if (_heroTalents.IsTree(HeroTalentTree::FARSEER))
        {
            // Farseer: Ancestral Swiftness for instant-cast Lightning Bolt/Lava Burst
            if (this->CanCastSpell(WoW120Spells::Shaman::Elemental::ANCESTRAL_SWIFTNESS, bot))
            {
                this->CastSpell(WoW120Spells::Shaman::Elemental::ANCESTRAL_SWIFTNESS, bot);
                return;
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::STORMBRINGER))
        {
            // Stormbringer: Tempest Strikes for additional nature damage procs
            if (this->CanCastSpell(WoW120Spells::Shaman::Elemental::TEMPEST_STRIKES, target))
            {
                this->CastSpell(WoW120Spells::Shaman::Elemental::TEMPEST_STRIKES, target);
                return;
            }
        }

        UpdateElementalState();

        uint32 enemyCount = this->GetEnemiesInRange(40.0f);

        if (enemyCount >= 3)
            ExecuteAoERotation(target, enemyCount);
        else
            ExecuteSingleTargetRotation(target);
    }

    void UpdateBuffs() override
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        // Earth Shield (self-protection)
        if (!bot->HasAura(ELEM_EARTH_SHIELD))
        {
            if (this->CanCastSpell(ELEM_EARTH_SHIELD, bot))
            {
                this->CastSpell(ELEM_EARTH_SHIELD, bot);
            }
        }
    }

    void UpdateDefensives()
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        float healthPct = bot->GetHealthPct();

        // Astral Shift (damage reduction)
        if (healthPct < 40.0f && this->CanCastSpell(ELEM_ASTRAL_SHIFT, bot))
        {
            this->CastSpell(ELEM_ASTRAL_SHIFT, bot);
            return;
        }

        // Capacitor Totem (AoE stun for escape)
        if (healthPct < 50.0f && bot->GetThreatManager().GetThreatListSize() >= 2)
        {
            if (this->CanCastSpell(ELEM_CAPACITOR_TOTEM, bot))
            {
                this->CastSpell(ELEM_CAPACITOR_TOTEM, bot);
                return;
            }
        }
    }

private:
    

    void UpdateElementalState()
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        // ManaResource (uint32) doesn't have Update method - handled by base class
        _maelstromTracker.Update(bot);
        _flameShockTracker.Update(bot);
        _lavaSurgeTracker.Update(bot);
        _stormkeeperTracker.Update(bot);
        UpdateCooldownStates();
    }

    void UpdateCooldownStates()
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        // Ascendance state
        if (_ascendanceActive && GameTime::GetGameTimeMS() >= _ascendanceEndTime)
            _ascendanceActive = false;

        if (bot->HasAura(ELEM_ASCENDANCE))
        {
            _ascendanceActive = true;
            if (Aura* aura = bot->GetAura(ELEM_ASCENDANCE))
                _ascendanceEndTime = GameTime::GetGameTimeMS() + aura->GetDuration();
        }
    }

    void ExecuteSingleTargetRotation(::Unit* target)
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        uint32 maelstrom = _maelstromTracker.GetCurrent();

        // Fire Elemental (major DPS cooldown)
        if ((GameTime::GetGameTimeMS() - _lastFireElementalTime) >= 150000) // 2.5 min CD
        {
            if (this->CanCastSpell(ELEM_FIRE_ELEMENTAL, bot))
            {
                this->CastSpell(ELEM_FIRE_ELEMENTAL, bot);
                _lastFireElementalTime = GameTime::GetGameTimeMS();
                return;
            }
        }

        // Ascendance (burst mode)
        if (maelstrom >= 60 && (GameTime::GetGameTimeMS() - _lastAscendanceTime) >= 180000) // 3 min CD
        {
            
            if (bot->HasSpell(ELEM_ASCENDANCE))
            {
                if (this->CanCastSpell(ELEM_ASCENDANCE, bot))
                {
                    this->CastSpell(ELEM_ASCENDANCE, bot);
                    _ascendanceActive = true;
                    _ascendanceEndTime = GameTime::GetGameTimeMS() + 15000;
                    _lastAscendanceTime = GameTime::GetGameTimeMS();
                    return;
                }
            }
        }

        // Stormkeeper (instant Lightning Bolts)
        if ((GameTime::GetGameTimeMS() - _lastStormkeeperTime) >= 60000) // 60 sec CD
        {
            if (this->CanCastSpell(ELEM_STORMKEEPER, bot))
            {
                this->CastSpell(ELEM_STORMKEEPER, bot);
                _stormkeeperTracker.ActivateProc(2);
                _lastStormkeeperTime = GameTime::GetGameTimeMS();
                return;
            }
        }

        // Primordial Wave (buff + Flame Shock application)
        if ((GameTime::GetGameTimeMS() - _lastPrimordialWaveTime) >= 45000) // 45 sec CD
        {
            if (bot->HasSpell(ELEM_PRIMORDIAL_WAVE))
            {
                if (this->CanCastSpell(ELEM_PRIMORDIAL_WAVE, target))
                {
                    this->CastSpell(ELEM_PRIMORDIAL_WAVE, target);
                    _flameShockTracker.ApplyFlameShock(target->GetGUID(), 18000);
                    _lastPrimordialWaveTime = GameTime::GetGameTimeMS();
                    return;
                }
            }
        }

        // Flame Shock (maintain DoT)
        if (!_flameShockTracker.HasFlameShock(target->GetGUID()) ||
            _flameShockTracker.NeedsFlameShockRefresh(target->GetGUID()))
        {
            if (this->CanCastSpell(ELEM_FLAME_SHOCK, target))
            {
                this->CastSpell(ELEM_FLAME_SHOCK, target);
                _flameShockTracker.ApplyFlameShock(target->GetGUID(), 18000);
                _maelstromTracker.Generate(5);
                return;
            }
        }

        // Lava Burst (with Lava Surge proc for instant cast)
        if (_lavaSurgeTracker.IsActive())
        {
            if (this->CanCastSpell(ELEM_LAVA_BURST, target))
            {
                this->CastSpell(ELEM_LAVA_BURST, target);
                _lavaSurgeTracker.ConsumeProc();
                _maelstromTracker.Generate(10);
                return;
            }
        }

        // Lava Burst (normal cast on Flame Shock target)
        if (_flameShockTracker.HasFlameShock(target->GetGUID()))
        {
            if (this->CanCastSpell(ELEM_LAVA_BURST, target))
            {
                this->CastSpell(ELEM_LAVA_BURST, target);
                _maelstromTracker.Generate(10);

                // Chance to proc Lava Surge
                if (rand() % 100 < 15) // 15% chance (simplified)
                    _lavaSurgeTracker.ActivateProc();

                return;
            }
        }

        // Earth Shock (Maelstrom spender - high damage)
        if (maelstrom >= 60)
        {
            if (this->CanCastSpell(ELEM_EARTH_SHOCK, target))
            {
                this->CastSpell(ELEM_EARTH_SHOCK, target);
                _maelstromTracker.Spend(60);
                return;
            }
        }

        // Elemental Blast (talent - generates Maelstrom + random buff)
        if (bot->HasSpell(ELEM_ELEMENTAL_BLAST))
        {
            if (this->CanCastSpell(ELEM_ELEMENTAL_BLAST, target))
            {
                this->CastSpell(ELEM_ELEMENTAL_BLAST, target);
                _maelstromTracker.Generate(12);
                return;
            }
        }

        // Icefury (talent - empowers Frost Shock)
        
        if (bot->HasSpell(ELEM_ICEFURY))
        {
            if (this->CanCastSpell(ELEM_ICEFURY, target))
            {
                this->CastSpell(ELEM_ICEFURY, target);
                _maelstromTracker.Generate(15);
                return;
            }
        }

        // Lightning Bolt with Stormkeeper proc (instant cast)
        if (_stormkeeperTracker.HasStack())
        {
            if (this->CanCastSpell(ELEM_LIGHTNING_BOLT, target))
            {
                this->CastSpell(ELEM_LIGHTNING_BOLT, target);
                _stormkeeperTracker.ConsumeStack();
                _maelstromTracker.Generate(8);
                return;
            }
        }

        // Echoing Shock (duplicates next spell)
        if ((GameTime::GetGameTimeMS() - _lastEchoingShockTime) >= 30000) // 30 sec CD
        {
            if (bot->HasSpell(ELEM_ECHOING_SHOCK))
            {
                if (this->CanCastSpell(ELEM_ECHOING_SHOCK, bot))
                {
                    this->CastSpell(ELEM_ECHOING_SHOCK, bot);
                    _lastEchoingShockTime = GameTime::GetGameTimeMS();
                    return;
                }
            }
        }

        // Lightning Bolt (builder)
        if (this->CanCastSpell(ELEM_LIGHTNING_BOLT, target))
        {
            this->CastSpell(ELEM_LIGHTNING_BOLT, target);
            _maelstromTracker.Generate(8);
            return;
        }
    }

    void ExecuteAoERotation(::Unit* target, uint32 enemyCount)
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        uint32 maelstrom = _maelstromTracker.GetCurrent();

        // Fire Elemental for AoE burst
        if ((GameTime::GetGameTimeMS() - _lastFireElementalTime) >= 150000 && enemyCount >= 4)
        {
            if (this->CanCastSpell(ELEM_FIRE_ELEMENTAL, bot))
            {
                this->CastSpell(ELEM_FIRE_ELEMENTAL, bot);
                _lastFireElementalTime = GameTime::GetGameTimeMS();
                return;
            }
        }

        // Ascendance for AoE burst
        if (maelstrom >= 60 && (GameTime::GetGameTimeMS() - _lastAscendanceTime) >= 180000 && enemyCount >= 5)
        {
            if (bot->HasSpell(ELEM_ASCENDANCE))
            {
                if (this->CanCastSpell(ELEM_ASCENDANCE, bot))
                {
                    this->CastSpell(ELEM_ASCENDANCE, bot);
                    _ascendanceActive = true;
                    _ascendanceEndTime = GameTime::GetGameTimeMS() + 15000;
                    _lastAscendanceTime = GameTime::GetGameTimeMS();
                    return;
                }
            }
        }

        // Liquid Magma Totem (AoE DoT)
        if (bot->HasSpell(ELEM_LIQUID_MAGMA_TOTEM) && enemyCount >= 3)
        {
            if (this->CanCastSpell(ELEM_LIQUID_MAGMA_TOTEM, bot))
            {
                this->CastSpell(ELEM_LIQUID_MAGMA_TOTEM, bot);
                return;
            }
        }

        // Stormkeeper for AoE (empowered Chain Lightning)
        if ((GameTime::GetGameTimeMS() - _lastStormkeeperTime) >= 60000 && enemyCount >= 3)
        {
            if (this->CanCastSpell(ELEM_STORMKEEPER, bot))
            {
                this->CastSpell(ELEM_STORMKEEPER, bot);
                _stormkeeperTracker.ActivateProc(2);
                _lastStormkeeperTime = GameTime::GetGameTimeMS();
                return;
            }
        }

        // Flame Shock on multiple targets (up to 3)
        if (enemyCount <= 3 && _flameShockTracker.GetActiveFlameShockCount() < 3)
        {
            if (!_flameShockTracker.HasFlameShock(target->GetGUID()))
            {
                if (this->CanCastSpell(ELEM_FLAME_SHOCK, target))
                {
                    this->CastSpell(ELEM_FLAME_SHOCK, target);
                    _flameShockTracker.ApplyFlameShock(target->GetGUID(), 18000);
                    _maelstromTracker.Generate(5);
                    return;
                }
            }
        }

        // Earthquake (AoE Maelstrom spender)
        if (maelstrom >= 60 && enemyCount >= 3)
        {
            if (this->CanCastSpell(ELEM_EARTHQUAKE, target))
            {
                this->CastSpell(ELEM_EARTHQUAKE, target);
                _maelstromTracker.Spend(60);
                return;
            }
        }

        // Chain Lightning with Stormkeeper proc
        if (_stormkeeperTracker.HasStack() && enemyCount >= 2)
        {
            if (this->CanCastSpell(ELEM_CHAIN_LIGHTNING, target))
            {
                this->CastSpell(ELEM_CHAIN_LIGHTNING, target);
                _stormkeeperTracker.ConsumeStack();
                _maelstromTracker.Generate(4 * ::std::min(enemyCount, 5u)); // 4 per target hit
                return;
            }
        }

        // Chain Lightning (AoE builder)
        if (enemyCount >= 2)
        {
            if (this->CanCastSpell(ELEM_CHAIN_LIGHTNING, target))
            {
                this->CastSpell(ELEM_CHAIN_LIGHTNING, target);
                _maelstromTracker.Generate(4 * ::std::min(enemyCount, 5u));
                return;
            }
        }

        // Lightning Bolt (single-target filler)
        if (this->CanCastSpell(ELEM_LIGHTNING_BOLT, target))
        {
            this->CastSpell(ELEM_LIGHTNING_BOLT, target);
            _maelstromTracker.Generate(8);
            return;
        }
    }

    [[nodiscard]] uint32 GetEnemiesInRange(float range) const
    {
        Player* bot = this->GetBot();
        if (!bot)
            return 0;

        uint32 count = 0;
        // Simplified enemy counting
        return ::std::min(count, 10u);
    }

    void InitializeElementalMechanics()
    {
        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)
        BotAI* ai = this;
        if (!ai) return;

        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            // EMERGENCY: Defensive cooldowns
            queue->RegisterSpell(ELEM_ASTRAL_SHIFT, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
            queue->AddCondition(ELEM_ASTRAL_SHIFT, [this](Player* bot, Unit*) {
                return bot && bot->GetHealthPct() < 40.0f;
            }, "HP < 40% (damage reduction)");

            // CRITICAL: Major burst cooldowns
            queue->RegisterSpell(ELEM_FIRE_ELEMENTAL, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(ELEM_FIRE_ELEMENTAL, [this](Player*, Unit* target) {
                return target && this->_maelstromTracker.GetCurrent() >= 40;
            }, "40+ Maelstrom (major CD, 2.5min)");

            queue->RegisterSpell(ELEM_ASCENDANCE, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(ELEM_ASCENDANCE, [this](Player* bot, Unit* target) {
                return bot && target && bot->HasSpell(ELEM_ASCENDANCE) && !this->_ascendanceActive;
            }, "Transform burst (15s, 3min CD, talent)");

            queue->RegisterSpell(ELEM_STORMKEEPER, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(ELEM_STORMKEEPER, [this](Player*, Unit*) {
                return !this->_stormkeeperTracker.HasStack();
            }, "2 instant Lightning Bolts (60s CD)");

            // HIGH: DoT maintenance & Maelstrom spenders
            queue->RegisterSpell(ELEM_FLAME_SHOCK, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(ELEM_FLAME_SHOCK, [this](Player*, Unit* target) {
                return target && this->_flameShockTracker.NeedsFlameShockRefresh(target->GetGUID());
            }, "Refresh Flame Shock (pandemic window)");

            queue->RegisterSpell(ELEM_LAVA_BURST, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(ELEM_LAVA_BURST, [this](Player*, Unit* target) {
                return target && (this->_lavaSurgeTracker.IsActive() ||
                       this->_flameShockTracker.HasFlameShock(target->GetGUID()));
            }, "Lava Surge proc or Flame Shock active");

            queue->RegisterSpell(ELEM_EARTH_SHOCK, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(ELEM_EARTH_SHOCK, [this](Player*, Unit* target) {
                return target && this->_maelstromTracker.GetCurrent() >= 60;
            }, "60+ Maelstrom (spender)");

            queue->RegisterSpell(ELEM_EARTHQUAKE, SpellPriority::HIGH, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(ELEM_EARTHQUAKE, [this](Player*, Unit* target) {
                return target && this->_maelstromTracker.GetCurrent() >= 60 &&
                       this->GetEnemiesInRange(40.0f) >= 3;
            }, "60+ Maelstrom, 3+ enemies (AoE spender)");

            // MEDIUM: Cooldowns & talents
            queue->RegisterSpell(ELEM_ELEMENTAL_BLAST, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(ELEM_ELEMENTAL_BLAST, [this](Player* bot, Unit* target) {
                return bot && target && bot->HasSpell(ELEM_ELEMENTAL_BLAST);
            }, "Maelstrom gen + stat buff (talent)");

            queue->RegisterSpell(ELEM_ECHOING_SHOCK, SpellPriority::MEDIUM, SpellCategory::OFFENSIVE);
            queue->AddCondition(ELEM_ECHOING_SHOCK, [this](Player* bot, Unit* target) {
                return bot && target && bot->HasSpell(ELEM_ECHOING_SHOCK);
            }, "Next spell duplicated (talent)");

            queue->RegisterSpell(ELEM_PRIMORDIAL_WAVE, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(ELEM_PRIMORDIAL_WAVE, [this](Player* bot, Unit* target) {
                return bot && target && bot->HasSpell(ELEM_PRIMORDIAL_WAVE);
            }, "Flame Shock + Lava Burst buff (talent)");

            queue->RegisterSpell(ELEM_LIQUID_MAGMA_TOTEM, SpellPriority::MEDIUM, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(ELEM_LIQUID_MAGMA_TOTEM, [this](Player* bot, Unit*) {
                return bot && bot->HasSpell(ELEM_LIQUID_MAGMA_TOTEM) &&
                       this->GetEnemiesInRange(40.0f) >= 2;
            }, "2+ enemies (AoE totem, talent)");

            queue->RegisterSpell(ELEM_ICEFURY, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(ELEM_ICEFURY, [this](Player* bot, Unit* target) {
                return bot && target && bot->HasSpell(ELEM_ICEFURY);
            }, "4 Frost Shock buffs (talent)");

            // LOW: Builders
            queue->RegisterSpell(ELEM_CHAIN_LIGHTNING, SpellPriority::LOW, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(ELEM_CHAIN_LIGHTNING, [this](Player*, Unit* target) {
                return target && this->GetEnemiesInRange(40.0f) >= 2;
            }, "2+ enemies (Maelstrom builder)");

            queue->RegisterSpell(ELEM_LIGHTNING_BOLT, SpellPriority::LOW, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(ELEM_LIGHTNING_BOLT, [](Player*, Unit* target) {
                return target != nullptr;
            }, "Filler (Maelstrom builder)");
        }

        auto* behaviorTree = ai->GetBehaviorTree();
        if (behaviorTree)
        {
            auto root = Selector("Elemental Shaman DPS", {
                // Tier 1: Burst Cooldowns (Fire Elemental, Ascendance, Stormkeeper)
                Sequence("Burst Cooldowns", {
                    Condition("Has Maelstrom and target", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim() && this->_maelstromTracker.GetCurrent() >= 40;
                    }),
                    Selector("Use burst cooldowns", {
                        Sequence("Fire Elemental", {
                            bot::ai::Action("Summon Fire Elemental", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(ELEM_FIRE_ELEMENTAL, bot))
                                {
                                    this->CastSpell(ELEM_FIRE_ELEMENTAL, bot);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Ascendance (talent)", {
                            Condition("Has Ascendance and not active", [this](Player* bot, Unit*) {
                                return bot->HasSpell(ELEM_ASCENDANCE) && !this->_ascendanceActive;
                            }),
                            bot::ai::Action("Cast Ascendance", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(ELEM_ASCENDANCE, bot))
                                {
                                    this->CastSpell(ELEM_ASCENDANCE, bot);
                                    this->_ascendanceActive = true;
                                    this->_ascendanceEndTime = GameTime::GetGameTimeMS() + 15000;
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Stormkeeper", {
                            Condition("Not active", [this](Player*, Unit*) {
                                return !this->_stormkeeperTracker.HasStack();
                            }),
                            bot::ai::Action("Cast Stormkeeper", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(ELEM_STORMKEEPER, bot))
                                {
                                    this->CastSpell(ELEM_STORMKEEPER, bot);
                                    this->_stormkeeperTracker.ActivateProc(2);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 2: DoT Maintenance & Priority Abilities
                Sequence("DoT & Priority", {
                    Condition("Has target", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim();
                    }),
                    Selector("Maintain DoT and use priority", {
                        Sequence("Flame Shock", {
                            Condition("Needs refresh", [this](Player* bot, Unit* target) {
                                target = bot->GetVictim();
                                return target && this->_flameShockTracker.NeedsFlameShockRefresh(target->GetGUID());
                            }),
                            bot::ai::Action("Cast Flame Shock", [this](Player* bot, Unit* target) {
                                target = bot->GetVictim();
                                if (target && this->CanCastSpell(ELEM_FLAME_SHOCK, target))
                                {
                                    this->CastSpell(ELEM_FLAME_SHOCK, target);
                                    this->_flameShockTracker.ApplyFlameShock(target->GetGUID(), 18000);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Lava Burst (proc or Flame Shock)", {
                            Condition("Lava Surge or Flame Shock active", [this](Player* bot, Unit* target) {
                                target = bot->GetVictim();
                                return target && (this->_lavaSurgeTracker.IsActive() ||
                                       this->_flameShockTracker.HasFlameShock(target->GetGUID()));
                            }),
                            bot::ai::Action("Cast Lava Burst", [this](Player* bot, Unit* target) {
                                target = bot->GetVictim();
                                if (target && this->CanCastSpell(ELEM_LAVA_BURST, target))
                                {
                                    this->CastSpell(ELEM_LAVA_BURST, target);
                                    this->_maelstromTracker.Generate(10);
                                    if (this->_lavaSurgeTracker.IsActive())
                                        this->_lavaSurgeTracker.ConsumeProc();
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Primordial Wave (talent)", {
                            Condition("Has talent", [this](Player* bot, Unit*) {
                                return bot->HasSpell(ELEM_PRIMORDIAL_WAVE);
                            }),
                            bot::ai::Action("Cast Primordial Wave", [this](Player* bot, Unit* target) {
                                target = bot->GetVictim();
                                if (target && this->CanCastSpell(ELEM_PRIMORDIAL_WAVE, target))
                                {
                                    this->CastSpell(ELEM_PRIMORDIAL_WAVE, target);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 3: Maelstrom Spender (Earth Shock, Earthquake)
                Sequence("Maelstrom Spender", {
                    Condition("60+ Maelstrom and target", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim() && this->_maelstromTracker.GetCurrent() >= 60;
                    }),
                    Selector("Spend Maelstrom", {
                        Sequence("Earthquake (AoE)", {
                            Condition("3+ enemies", [this](Player*, Unit*) {
                                return this->GetEnemiesInRange(40.0f) >= 3;
                            }),
                            bot::ai::Action("Cast Earthquake", [this](Player* bot, Unit* target) {
                                target = bot->GetVictim();
                                if (target && this->CanCastSpell(ELEM_EARTHQUAKE, target))
                                {
                                    this->CastSpell(ELEM_EARTHQUAKE, target);
                                    this->_maelstromTracker.Spend(60);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Earth Shock (ST)", {
                            bot::ai::Action("Cast Earth Shock", [this](Player* bot, Unit* target) {
                                target = bot->GetVictim();
                                if (target && this->CanCastSpell(ELEM_EARTH_SHOCK, target))
                                {
                                    this->CastSpell(ELEM_EARTH_SHOCK, target);
                                    this->_maelstromTracker.Spend(60);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 4: Maelstrom Builder (Chain Lightning, Lightning Bolt)
                Sequence("Maelstrom Builder", {
                    Condition("Has target", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim();
                    }),
                    Selector("Generate Maelstrom", {
                        Sequence("Chain Lightning (AoE)", {
                            Condition("2+ enemies", [this](Player*, Unit*) {
                                return this->GetEnemiesInRange(40.0f) >= 2;
                            }),
                            bot::ai::Action("Cast Chain Lightning", [this](Player* bot, Unit* target) {
                                target = bot->GetVictim();
                                if (target && this->CanCastSpell(ELEM_CHAIN_LIGHTNING, target))
                                {
                                    this->CastSpell(ELEM_CHAIN_LIGHTNING, target);
                                    uint32 enemies = ::std::min(this->GetEnemiesInRange(40.0f), 5u);
                                    this->_maelstromTracker.Generate(4 * enemies);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Lightning Bolt (ST)", {
                            bot::ai::Action("Cast Lightning Bolt", [this](Player* bot, Unit* target) {
                                target = bot->GetVictim();
                                if (target && this->CanCastSpell(ELEM_LIGHTNING_BOLT, target))
                                {
                                    this->CastSpell(ELEM_LIGHTNING_BOLT, target);
                                    this->_maelstromTracker.Generate(8);
                                    if (this->_stormkeeperTracker.HasStack())
                                        this->_stormkeeperTracker.ConsumeStack();
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                })
            });

            behaviorTree->SetRoot(root);
        }
    }

    // Member variables
    MaelstromTracker _maelstromTracker;
    FlameShockTracker _flameShockTracker;
    LavaSurgeTracker _lavaSurgeTracker;
    StormkeeperTracker _stormkeeperTracker;

    bool _ascendanceActive;
    uint32 _ascendanceEndTime;

    uint32 _lastAscendanceTime;
    uint32 _lastEchoingShockTime;
    uint32 _lastPrimordialWaveTime;
    uint32 _lastFireElementalTime;
    uint32 _lastStormkeeperTime;

    // Hero talent detection cache (refreshed on combat start)
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot

#endif // PLAYERBOT_ELEMENTALSHAMANREFACTORED_H
