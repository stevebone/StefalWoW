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

#ifndef PLAYERBOT_RESTORATIONSHAMANREFACTORED_H
#define PLAYERBOT_RESTORATIONSHAMANREFACTORED_H

#include "../CombatSpecializationTemplates.h"
#include "../Common/CooldownManager.h"
#include "Player.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "ObjectGuid.h"
#include <unordered_map>
#include "Log.h"
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
// WoW 12.0 (The War Within) - Restoration Shaman Spell IDs
// Using central registry: WoW120Spells::Shaman and WoW120Spells::Shaman::Restoration
constexpr uint32 REST_HEALING_WAVE = WoW120Spells::Shaman::Restoration::HEALING_WAVE;
constexpr uint32 REST_HEALING_SURGE = WoW120Spells::Shaman::Restoration::HEALING_SURGE;
constexpr uint32 REST_CHAIN_HEAL = WoW120Spells::Shaman::Restoration::CHAIN_HEAL;
constexpr uint32 REST_RIPTIDE = WoW120Spells::Shaman::Restoration::RIPTIDE;
constexpr uint32 REST_HEALING_RAIN = WoW120Spells::Shaman::Restoration::HEALING_RAIN;
constexpr uint32 REST_WELLSPRING = WoW120Spells::Shaman::Restoration::WELLSPRING;
constexpr uint32 REST_HEALING_TIDE_TOTEM = WoW120Spells::Shaman::Restoration::HEALING_TIDE_TOTEM;
constexpr uint32 REST_CLOUDBURST_TOTEM = WoW120Spells::Shaman::Restoration::CLOUDBURST_TOTEM;
constexpr uint32 REST_SPIRIT_LINK_TOTEM = WoW120Spells::Shaman::Restoration::SPIRIT_LINK_TOTEM;
constexpr uint32 REST_EARTHEN_WALL_TOTEM = WoW120Spells::Shaman::Restoration::EARTHEN_WALL_TOTEM;
constexpr uint32 REST_ANCESTRAL_PROTECTION_TOTEM = WoW120Spells::Shaman::Restoration::ANCESTRAL_PROTECTION_TOTEM;
constexpr uint32 REST_ASCENDANCE = WoW120Spells::Shaman::Restoration::ASCENDANCE_RESTO;
constexpr uint32 REST_UNLEASH_LIFE = WoW120Spells::Shaman::Restoration::UNLEASH_LIFE;
constexpr uint32 REST_EARTH_SHIELD = WoW120Spells::Shaman::Restoration::EARTH_SHIELD;
constexpr uint32 REST_WATER_SHIELD = WoW120Spells::Shaman::Restoration::WATER_SHIELD;
constexpr uint32 REST_PURIFY_SPIRIT = WoW120Spells::Shaman::Restoration::PURIFY_SPIRIT;
constexpr uint32 REST_SPIRITWALKERS_GRACE = WoW120Spells::Shaman::Restoration::SPIRITWALKERS_GRACE;
constexpr uint32 REST_ASTRAL_SHIFT = WoW120Spells::Shaman::ASTRAL_SHIFT;
constexpr uint32 REST_WIND_SHEAR = WoW120Spells::Shaman::WIND_SHEAR;

// ManaResource is already defined in CombatSpecializationTemplates.h
// No need to redefine it here

// Riptide HoT tracker
class RiptideTracker
{
public:
    RiptideTracker() = default;

    void ApplyRiptide(ObjectGuid guid, uint32 duration = 18000)
    {
        _riptideTargets[guid] = GameTime::GetGameTimeMS() + duration;
    }

    void RemoveRiptide(ObjectGuid guid)
    {
        _riptideTargets.erase(guid);
    }

    [[nodiscard]] bool HasRiptide(ObjectGuid guid) const
    {
        auto it = _riptideTargets.find(guid);
        if (it == _riptideTargets.end())

            return false;
        return GameTime::GetGameTimeMS() < it->second;
    }

    [[nodiscard]] uint32 GetRiptideTimeRemaining(ObjectGuid guid) const
    {
        auto it = _riptideTargets.find(guid);
        if (it == _riptideTargets.end())

            return 0;

        uint32 now = GameTime::GetGameTimeMS();
        if (now >= it->second)

            return 0;

        return it->second - now;
    }

    [[nodiscard]] bool NeedsRiptideRefresh(ObjectGuid guid, uint32 pandemicWindow = 5400) const
    {
        uint32 remaining = GetRiptideTimeRemaining(guid);
        return remaining < pandemicWindow;
    }

    [[nodiscard]] uint32 GetActiveRiptideCount() const
    {
        uint32 count = 0;
        uint32 now = GameTime::GetGameTimeMS();
        for (const auto& pair : _riptideTargets)
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

        // Clean up expired Riptides
        uint32 now = GameTime::GetGameTimeMS();
        for (auto it = _riptideTargets.begin(); it != _riptideTargets.end();)
        {

            if (now >= it->second)

                it = _riptideTargets.erase(it);

            else

                ++it;
        }
    }

private:
    CooldownManager _cooldowns;
    ::std::unordered_map<ObjectGuid, uint32> _riptideTargets; // GUID -> expiration time
};

// Earth Shield tracker
class EarthShieldTracker
{
public:
    EarthShieldTracker() = default;

    void ApplyEarthShield(ObjectGuid guid, uint32 duration = 600000)
    {
        _earthShieldTarget = guid;
        _earthShieldEndTime = GameTime::GetGameTimeMS() + duration;
    }

    void RemoveEarthShield()
    {
        _earthShieldTarget = ObjectGuid::Empty;
    }

    [[nodiscard]] bool HasEarthShield(ObjectGuid guid) const
    {
        return _earthShieldTarget == guid && GameTime::GetGameTimeMS() < _earthShieldEndTime;
    }

    [[nodiscard]] ObjectGuid GetEarthShieldTarget() const { return _earthShieldTarget; }

    [[nodiscard]] bool NeedsEarthShieldRefresh(uint32 refreshWindow = 60000) const
    {
        if (_earthShieldTarget.IsEmpty())

            return true;

        uint32 now = GameTime::GetGameTimeMS();
        if (now >= _earthShieldEndTime)

            return true;

        return (_earthShieldEndTime - now) < refreshWindow;
    }

    void Update(Player* bot)
    {
        if (!bot)

            return;

        // Check if Earth Shield expired
        if (!_earthShieldTarget.IsEmpty() && GameTime::GetGameTimeMS() >= _earthShieldEndTime)
        {

            _earthShieldTarget = ObjectGuid::Empty;
        }
    }

private:
    ObjectGuid _earthShieldTarget;
    uint32 _earthShieldEndTime = 0;
};

// ============================================================================
// TIDAL WAVES PROC TRACKER
// ============================================================================
// Tidal Waves: Casting Riptide or Chain Heal grants 2 stacks of Tidal Waves.
// - Healing Wave: 20% cast time reduction
// - Healing Surge: 40% additional critical strike chance
// Consumed on next Healing Wave or Healing Surge cast.

constexpr uint32 REST_TIDAL_WAVES = WoW120Spells::Shaman::Restoration::TIDAL_WAVES;

class TidalWavesTracker
{
public:
    TidalWavesTracker() : _stacks(0) {}

    /// Called when Riptide or Chain Heal is cast (generates 2 stacks)
    void OnRiptideOrChainHealCast() { _stacks = 2; }

    /// Called when Healing Wave or Healing Surge is cast (consumes 1 stack)
    void ConsumeStack()
    {
        if (_stacks > 0)
            _stacks--;
    }

    /// Check if Tidal Waves proc is active
    [[nodiscard]] bool IsActive() const { return _stacks > 0; }
    [[nodiscard]] uint32 GetStacks() const { return _stacks; }

    /// Sync with actual aura state
    void Update(Player* bot)
    {
        if (!bot)
            return;

        if (Aura* aura = bot->GetAura(REST_TIDAL_WAVES))
            _stacks = aura->GetStackAmount();
        else
            _stacks = 0;
    }

private:
    uint32 _stacks;
};

class RestorationShamanRefactored : public HealerSpecialization<ManaResource>
{
public:
    using Base = HealerSpecialization<ManaResource>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::_resource;
    explicit RestorationShamanRefactored(Player* bot)        : HealerSpecialization<ManaResource>(bot)
        , _riptideTracker()
        , _earthShieldTracker()
        , _tidalWavesTracker()
        , _ascendanceActive(false)
        , _ascendanceEndTime(0)
        , _lastAscendanceTime(0)
        , _lastHealingTideTotemTime(0)
        , _lastSpiritLinkTotemTime(0)
        , _lastCloudburstTotemTime(0)
        , _lastEarthenWallTotemTime(0)
        , _lastAncestralProtectionTotemTime(0)
        , _cooldowns()
    {
        // Register cooldowns for major abilities
        // COMMENTED OUT:         _cooldowns.RegisterBatch({
        // COMMENTED OUT: 
        // COMMENTED OUT:             {RESTO_SHAMAN_HEALING_TIDE_TOTEM, 180000, 1},
        // COMMENTED OUT: 
        // COMMENTED OUT:             {RESTO_SHAMAN_SPIRIT_LINK_TOTEM, 180000, 1},
        // COMMENTED OUT: 
        // COMMENTED OUT:             {RESTO_SHAMAN_ASCENDANCE, 180000, 1},
        // COMMENTED OUT: 
        // COMMENTED OUT:             {RESTO_SHAMAN_CLOUDBURST_TOTEM, 30000, 1}
        // COMMENTED OUT:         });

        // Resource initialization handled by base class CombatSpecializationTemplate

        // Phase 5: Initialize decision systems
        InitializeRestorationShamanMechanics();

        // Register healing spell efficiency tiers
        GetEfficiencyManager().RegisterSpell(REST_HEALING_WAVE, HealingSpellTier::VERY_HIGH, "Healing Wave");
        GetEfficiencyManager().RegisterSpell(REST_RIPTIDE, HealingSpellTier::VERY_HIGH, "Riptide");
        GetEfficiencyManager().RegisterSpell(REST_EARTH_SHIELD, HealingSpellTier::VERY_HIGH, "Earth Shield");
        GetEfficiencyManager().RegisterSpell(REST_HEALING_SURGE, HealingSpellTier::HIGH, "Healing Surge");
        GetEfficiencyManager().RegisterSpell(REST_CHAIN_HEAL, HealingSpellTier::MEDIUM, "Chain Heal");
        GetEfficiencyManager().RegisterSpell(REST_HEALING_RAIN, HealingSpellTier::MEDIUM, "Healing Rain");
        GetEfficiencyManager().RegisterSpell(REST_HEALING_TIDE_TOTEM, HealingSpellTier::EMERGENCY, "Healing Tide Totem");
        GetEfficiencyManager().RegisterSpell(REST_SPIRIT_LINK_TOTEM, HealingSpellTier::EMERGENCY, "Spirit Link Totem");
        GetEfficiencyManager().RegisterSpell(REST_WELLSPRING, HealingSpellTier::LOW, "Wellspring");

        // SAFETY: GetName() removed from constructor to prevent ACCESS_VIOLATION crash during worker thread bot login
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
            // Farseer: Ancestral Swiftness for instant-cast healing
            if (this->CanCastSpell(WoW120Spells::Shaman::Restoration::RESTO_ANCESTRAL_SWIFTNESS, bot))
            {
                this->CastSpell(WoW120Spells::Shaman::Restoration::RESTO_ANCESTRAL_SWIFTNESS, bot);
                return;
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::TOTEMIC))
        {
            // Totemic: Surging Totem for enhanced group healing
            if (this->CanCastSpell(WoW120Spells::Shaman::Restoration::RESTO_SURGING_TOTEM, bot))
            {
                this->CastSpell(WoW120Spells::Shaman::Restoration::RESTO_SURGING_TOTEM, bot);
                return;
            }
        }

        UpdateRestorationState();

        // Restoration is a healer - check group health first
        if (Group* group = bot->GetGroup())
        {

            ::std::vector<Unit*> groupMembers;

            for (GroupReference const& ref : group->GetMembers())

            {

                if (Player* member = ref.GetSource())
                {

                    if (member->IsAlive() && bot->IsInMap(member))

                        groupMembers.push_back(member);

                }

            }


            if (!groupMembers.empty())

            {

                if (HandleGroupHealing(groupMembers))

                    return;

            }
        }

        // Solo healing (self)
        if (bot->GetHealthPct() < 80.0f)
        {

            if (HandleSelfHealing())

                return;
        }

        // Deal damage when no healing is needed (maintain mana)
        ExecuteDamageRotation(target);
    }

    void UpdateBuffs() override
    {
        Player* bot = this->GetBot();
        if (!bot)

            return;

        // Water Shield (mana regeneration)
        if (!bot->HasAura(REST_WATER_SHIELD))
        {

            if (this->CanCastSpell(REST_WATER_SHIELD, bot))

            {

                this->CastSpell(REST_WATER_SHIELD, bot);

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
        if (healthPct < 40.0f && this->CanCastSpell(REST_ASTRAL_SHIFT, bot))
        {

            this->CastSpell(REST_ASTRAL_SHIFT, bot);

            return;
        }

        // Spiritwalker's Grace (heal while moving)
        if (healthPct < 60.0f && bot->HasUnitMovementFlag(MOVEMENTFLAG_FORWARD))
        {

            if (this->CanCastSpell(REST_SPIRITWALKERS_GRACE, bot))

            {

                this->CastSpell(REST_SPIRITWALKERS_GRACE, bot);

                return;

            }
        }
    }

private:
    // Member variables
    RiptideTracker _riptideTracker;
    EarthShieldTracker _earthShieldTracker;
    TidalWavesTracker _tidalWavesTracker;

    bool _ascendanceActive;
    uint32 _ascendanceEndTime;

    uint32 _lastAscendanceTime;
    uint32 _lastHealingTideTotemTime;
    uint32 _lastSpiritLinkTotemTime;
    uint32 _lastCloudburstTotemTime;
    uint32 _lastEarthenWallTotemTime;
    uint32 _lastAncestralProtectionTotemTime;
    CooldownManager _cooldowns;

    void UpdateRestorationState()
    {
        Player* bot = this->GetBot();
        if (!bot)

            return;

        // ManaResource (uint32) doesn't have Update method - handled by base class
        _riptideTracker.Update(bot);
        _earthShieldTracker.Update(bot);
        _tidalWavesTracker.Update(bot);
        UpdateCooldownStates();
    }    void UpdateCooldownStates()
    {
        Player* bot = this->GetBot();
        if (!bot)

            return;

        // Ascendance state (transform into Water Ascendant)
        if (_ascendanceActive && GameTime::GetGameTimeMS() >= _ascendanceEndTime)

            _ascendanceActive = false;

        if (bot->HasAura(REST_ASCENDANCE))
        {

            _ascendanceActive = true;

            if (Aura* aura = bot->GetAura(REST_ASCENDANCE))

                _ascendanceEndTime = GameTime::GetGameTimeMS() + aura->GetDuration();
                }
    }

    bool HandleGroupHealing(const ::std::vector<Unit*>& group)
    {
        // Emergency cooldowns
        if (HandleEmergencyCooldowns(group))

            return true;

        // Maintain HoTs and shields
        if (HandleHoTs(group))

            return true;

        // AoE healing
        if (HandleAoEHealing(group))

            return true;

        // Direct healing
        if (HandleDirectHealing(group))

            return true;
            return false;
    }

    bool HandleEmergencyCooldowns(const ::std::vector<Unit*>& group)
    {
        Player* bot = this->GetBot();
        if (!bot)
            return false;

        // Ancestral Protection Totem (resurrect on death)
        uint32 criticalHealthCount = 0;
        for (Unit* member : group)
        {

            if (member && member->GetHealthPct() < 20.0f)

                ++criticalHealthCount;
        }

        if (criticalHealthCount >= 2 && (GameTime::GetGameTimeMS() - _lastAncestralProtectionTotemTime) >= 300000) // 5 min CD
        {

            if (bot->HasSpell(REST_ANCESTRAL_PROTECTION_TOTEM))

            {

                if (this->CanCastSpell(REST_ANCESTRAL_PROTECTION_TOTEM, bot))

                {

                    this->CastSpell(REST_ANCESTRAL_PROTECTION_TOTEM, bot);

                    _lastAncestralProtectionTotemTime = GameTime::GetGameTimeMS();

                    return true;

                }

            }
        }

        // Healing Tide Totem (massive raid healing)
        uint32 lowHealthCount = 0;
        for (Unit* member : group)
        {

            if (member && member->GetHealthPct() < 60.0f)

                ++lowHealthCount;
        }

        if (lowHealthCount >= 4 && (GameTime::GetGameTimeMS() - _lastHealingTideTotemTime) >= 180000) // 3 min CD
        {

            if (this->CanCastSpell(REST_HEALING_TIDE_TOTEM, bot))

            {

                this->CastSpell(REST_HEALING_TIDE_TOTEM, bot);

                _lastHealingTideTotemTime = GameTime::GetGameTimeMS();

                return true;

            }
        }

        // Spirit Link Totem (equalize health)
        if (lowHealthCount >= 3 && (GameTime::GetGameTimeMS() - _lastSpiritLinkTotemTime) >= 180000) // 3 min CD
        {

            if (this->CanCastSpell(REST_SPIRIT_LINK_TOTEM, bot))

            {

                this->CastSpell(REST_SPIRIT_LINK_TOTEM, bot);

                _lastSpiritLinkTotemTime = GameTime::GetGameTimeMS();

                return true;

            }
        }

        // Ascendance (healing burst mode)
        if (lowHealthCount >= 3 && (GameTime::GetGameTimeMS() - _lastAscendanceTime) >= 180000) // 3 min CD
        {

            if (this->CanCastSpell(REST_ASCENDANCE, bot))

            {

                this->CastSpell(REST_ASCENDANCE, bot);

                _ascendanceActive = true;

                _ascendanceEndTime = GameTime::GetGameTimeMS() + 15000;

                _lastAscendanceTime = GameTime::GetGameTimeMS();

                return true;

            }
        }

        // Earthen Wall Totem (shield wall)
        if (lowHealthCount >= 3 && (GameTime::GetGameTimeMS() - _lastEarthenWallTotemTime) >= 60000) // 60 sec CD
        {
            if (bot->HasSpell(REST_EARTHEN_WALL_TOTEM))
            {
                if (this->CanCastSpell(REST_EARTHEN_WALL_TOTEM, bot))
                {
                    this->CastSpell(REST_EARTHEN_WALL_TOTEM, bot);
                    _lastEarthenWallTotemTime = GameTime::GetGameTimeMS();
                    return true;
                }
            }
        }

        return false;
    }

    bool HandleHoTs(const ::std::vector<Unit*>& group)
    {
        Player* bot = this->GetBot();
        if (!bot)
            return false;

        uint32 activeRiptides = _riptideTracker.GetActiveRiptideCount();

        // Earth Shield on tank
        Unit* tankTarget = nullptr;
        for (Unit* member : group)
        {
            if (member && IsTankRole(member))
            {
                tankTarget = member;
                break;
            }
        }

        if (tankTarget && !_earthShieldTracker.HasEarthShield(tankTarget->GetGUID()))
        {
            if (this->CanCastSpell(REST_EARTH_SHIELD, tankTarget))
            {
                this->CastSpell(REST_EARTH_SHIELD, tankTarget);
                _earthShieldTracker.ApplyEarthShield(tankTarget->GetGUID(), 600000);
                return true;
            }
        }

        // Riptide on injured allies
        if (activeRiptides < group.size())
        {
            for (Unit* member : group)
            {
                if (member && member->GetHealthPct() < 90.0f)
                {
                    if (_riptideTracker.NeedsRiptideRefresh(member->GetGUID()))
                    {
                        if (this->CanCastSpell(REST_RIPTIDE, member))
                        {
                            this->CastSpell(REST_RIPTIDE, member);
                            _riptideTracker.ApplyRiptide(member->GetGUID(), 18000);
                            _tidalWavesTracker.OnRiptideOrChainHealCast(); // Generate Tidal Waves
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    bool HandleAoEHealing(const ::std::vector<Unit*>& group)
    {
        Player* bot = this->GetBot();
        if (!bot)

            return false;

        // Healing Rain (ground AoE HoT)
        uint32 stackedAlliesCount = 0;
        Unit* stackedTarget = nullptr;

        for (Unit* member : group)
        {

            if (member && member->GetHealthPct() < 80.0f)

            {
                // Count nearby allies to this member

                uint32 nearbyCount = 0;

                for (Unit* other : group)

                {

                    if (other && member->GetDistance(other) <= 10.0f && other->GetHealthPct() < 80.0f)

                        ++nearbyCount;

                }


                if (nearbyCount > stackedAlliesCount)

                {

                    stackedAlliesCount = nearbyCount;

                    stackedTarget = member;

                }

            }
            }

        if (stackedAlliesCount >= 3 && stackedTarget)
        {

            if (IsHealAllowedByMana(REST_HEALING_RAIN) && this->CanCastSpell(REST_HEALING_RAIN, stackedTarget))

            {

                this->CastSpell(REST_HEALING_RAIN, stackedTarget);

                return true;

            }
        }

        // Wellspring (instant AoE heal)
        if (stackedAlliesCount >= 4 && stackedTarget)

        {
        if (bot->HasSpell(REST_WELLSPRING))

            {

                if (IsHealAllowedByMana(REST_WELLSPRING) && this->CanCastSpell(REST_WELLSPRING, stackedTarget))

                {

                    this->CastSpell(REST_WELLSPRING, stackedTarget);

                    return true;

                }

            }
        }

        // Chain Heal (bouncing heal)
        uint32 injuredCount = 0;
        for (Unit* member : group)
        {

            if (member && member->GetHealthPct() < 80.0f)

                ++injuredCount;
        }

        if (injuredCount >= 2)
        {

            for (Unit* member : group)

            {

                if (member && member->GetHealthPct() < 75.0f)

                {

                    if (IsHealAllowedByMana(REST_CHAIN_HEAL) && this->CanCastSpell(REST_CHAIN_HEAL, member))

                    {

                        this->CastSpell(REST_CHAIN_HEAL, member);
                        _tidalWavesTracker.OnRiptideOrChainHealCast(); // Generate Tidal Waves

                        return true;

                    }

                }

            }
        }

        // Cloudburst Totem (store healing and release)
        if (injuredCount >= 3 && (GameTime::GetGameTimeMS() - _lastCloudburstTotemTime) >= 30000) // 30 sec CD

        {
        if (bot->HasSpell(REST_CLOUDBURST_TOTEM))

            {

                if (this->CanCastSpell(REST_CLOUDBURST_TOTEM, bot))

                {

                    this->CastSpell(REST_CLOUDBURST_TOTEM, bot);

                    _lastCloudburstTotemTime = GameTime::GetGameTimeMS();

                    return true;

                }

            }
        }

        return false;
    }

    bool HandleDirectHealing(const ::std::vector<Unit*>& group)
    {
        // Priority 1: Consume Tidal Waves proc on Healing Surge for emergency (40% extra crit)
        if (_tidalWavesTracker.IsActive())
        {
            for (Unit* member : group)
            {
                if (member && member->GetHealthPct() < 50.0f)
                {
                    if (IsHealAllowedByMana(REST_HEALING_SURGE) && this->CanCastSpell(REST_HEALING_SURGE, member))
                    {
                        this->CastSpell(REST_HEALING_SURGE, member);
                        _tidalWavesTracker.ConsumeStack();
                        return true;
                    }
                }
            }

            // Priority 2: Consume Tidal Waves on Healing Wave for faster cast (20% faster)
            for (Unit* member : group)
            {
                if (member && member->GetHealthPct() < 80.0f)
                {
                    if (this->CanCastSpell(REST_HEALING_WAVE, member))
                    {
                        this->CastSpell(REST_HEALING_WAVE, member);
                        _tidalWavesTracker.ConsumeStack();
                        return true;
                    }
                }
            }
        }

        // Healing Surge for emergency (without Tidal Waves)
        for (Unit* member : group)
        {
            if (member && member->GetHealthPct() < 50.0f)
            {
                if (IsHealAllowedByMana(REST_HEALING_SURGE) && this->CanCastSpell(REST_HEALING_SURGE, member))
                {
                    this->CastSpell(REST_HEALING_SURGE, member);
                    return true;
                }
            }
        }

        // Healing Wave (efficient single-target, without Tidal Waves)
        for (Unit* member : group)
        {
            if (member && member->GetHealthPct() < 80.0f)
            {
                if (this->CanCastSpell(REST_HEALING_WAVE, member))
                {
                    this->CastSpell(REST_HEALING_WAVE, member);
                    return true;
                }
            }
        }

        return false;
    }

    bool HandleSelfHealing()
    {
        Player* bot = this->GetBot();
        if (!bot)

            return false;

        // Riptide
        if (_riptideTracker.NeedsRiptideRefresh(bot->GetGUID()))
        {

            if (this->CanCastSpell(REST_RIPTIDE, bot))

            {

                this->CastSpell(REST_RIPTIDE, bot);

                _riptideTracker.ApplyRiptide(bot->GetGUID(), 18000);
                _tidalWavesTracker.OnRiptideOrChainHealCast(); // Generate Tidal Waves

                return true;

            }
        }

        // Healing Surge
        if (bot->GetHealthPct() < 60.0f)
        {

            if (IsHealAllowedByMana(REST_HEALING_SURGE) && this->CanCastSpell(REST_HEALING_SURGE, bot))

            {

                this->CastSpell(REST_HEALING_SURGE, bot);

                return true;

            }
        }

        // Healing Wave
        if (bot->GetHealthPct() < 80.0f)
        {

            if (this->CanCastSpell(REST_HEALING_WAVE, bot))

            {

                this->CastSpell(REST_HEALING_WAVE, bot);

                return true;

            }
        }

        return false;
    }

    void ExecuteDamageRotation(::Unit* target)
    {
        // Restoration Shaman has minimal damage rotation
        // Just maintain mana and deal minimal damage when no healing needed
    }

    [[nodiscard]] bool IsTankRole(Unit* unit) const
    {
        if (!unit)

            return false;

        if (Player* player = unit->ToPlayer())
        {
            // Check if player has tank role based on class
            // Protection Paladin, Protection Warrior, Blood DK, Guardian Druid, Brewmaster Monk, Vengeance DH

            uint8 playerClass = player->GetClass();
            return (playerClass == CLASS_WARRIOR || playerClass == CLASS_PALADIN ||

                    playerClass == CLASS_DEATH_KNIGHT || playerClass == CLASS_DRUID ||

                    playerClass == CLASS_MONK || playerClass == CLASS_DEMON_HUNTER);
        }

        return false;
    }

    // ========================================================================
    // PHASE 5: DECISION SYSTEM INTEGRATION
    // ========================================================================

    void InitializeRestorationShamanMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)
        BotAI* ai = this;
        if (!ai) return;

        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            // EMERGENCY: Raid-wide emergency healing

            queue->RegisterSpell(REST_HEALING_TIDE_TOTEM, SpellPriority::EMERGENCY, SpellCategory::HEALING);

            queue->AddCondition(REST_HEALING_TIDE_TOTEM, [this](Player*, Unit*) {

                auto group = this->GetGroupMembers();

                uint32 low = 0;

                for (auto* m : group) if (m && m->GetHealthPct() < 60.0f) low++;

                return low >= 4;

            }, "4+ allies < 60% HP (totem, 3min CD)");


            queue->RegisterSpell(REST_ANCESTRAL_PROTECTION_TOTEM, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);

            queue->AddCondition(REST_ANCESTRAL_PROTECTION_TOTEM, [this](Player* bot, Unit*) {

                if (!bot->HasSpell(REST_ANCESTRAL_PROTECTION_TOTEM)) return false;

                auto group = this->GetGroupMembers();

                uint32 critical = 0;

                for (auto* m : group) if (m && m->GetHealthPct() < 20.0f) critical++;

                return critical >= 2;

            }, "2+ allies < 20% HP (resurrect totem, 5min CD)");

            // CRITICAL: Major healing cooldowns

            queue->RegisterSpell(REST_ASCENDANCE, SpellPriority::CRITICAL, SpellCategory::HEALING);

            queue->AddCondition(REST_ASCENDANCE, [this](Player*, Unit*) {

                if (this->_ascendanceActive) return false;

                auto group = this->GetGroupMembers();

                uint32 injured = 0;

                for (auto* m : group) if (m && m->GetHealthPct() < 60.0f) injured++;

                return injured >= 3;

            }, "3+ allies < 60% HP (15s burst, 3min CD)");


            queue->RegisterSpell(REST_SPIRIT_LINK_TOTEM, SpellPriority::CRITICAL, SpellCategory::HEALING);

            queue->AddCondition(REST_SPIRIT_LINK_TOTEM, [this](Player*, Unit*) {

                auto group = this->GetGroupMembers();

                uint32 injured = 0;

                for (auto* m : group) if (m && m->GetHealthPct() < 60.0f) injured++;

                return injured >= 3;

            }, "3+ allies < 60% HP (equalize health, 3min CD)");


            queue->RegisterSpell(REST_EARTHEN_WALL_TOTEM, SpellPriority::CRITICAL, SpellCategory::DEFENSIVE);

            queue->AddCondition(REST_EARTHEN_WALL_TOTEM, [this](Player* bot, Unit*) {

                if (!bot->HasSpell(REST_EARTHEN_WALL_TOTEM)) return false;

                auto group = this->GetGroupMembers();

                uint32 injured = 0;

                for (auto* m : group) if (m && m->GetHealthPct() < 60.0f) injured++;

                return injured >= 3;

            }, "3+ allies < 60% HP (shield wall, 60s CD)");

            // HIGH: Core HoT and shield maintenance

            queue->RegisterSpell(REST_EARTH_SHIELD, SpellPriority::HIGH, SpellCategory::DEFENSIVE);

            queue->AddCondition(REST_EARTH_SHIELD, [this](Player*, Unit*) {

                auto group = this->GetGroupMembers();

                for (auto* m : group)
                {

                    if (m && this->IsTankRole(m) && !this->_earthShieldTracker.HasEarthShield(m->GetGUID()))

                        return true;

                }

                return false;

            }, "Tank needs Earth Shield (10min)");


            queue->RegisterSpell(REST_RIPTIDE, SpellPriority::HIGH, SpellCategory::HEALING);

            queue->AddCondition(REST_RIPTIDE, [this](Player*, Unit*) {

                auto group = this->GetGroupMembers();

                for (auto* m : group)
                {

                    if (m && m->GetHealthPct() < 90.0f && this->_riptideTracker.NeedsRiptideRefresh(m->GetGUID()))

                        return true;

                }

                return false;

            }, "Ally < 90% HP needs Riptide (18s HoT)");

            // MEDIUM: AoE healing

            queue->RegisterSpell(REST_HEALING_RAIN, SpellPriority::MEDIUM, SpellCategory::HEALING);

            queue->AddCondition(REST_HEALING_RAIN, [this](Player*, Unit*) {

                auto group = this->GetGroupMembers();

                for (auto* anchor : group)
                {

                    if (!anchor || anchor->GetHealthPct() >= 80.0f) continue;

                    uint32 nearby = 0;

                    for (auto* m : group)

                        if (m && anchor->GetDistance(m) <= 10.0f && m->GetHealthPct() < 80.0f)

                            nearby++;

                    if (nearby >= 3) return true;

                }

                return false;

            }, "3+ stacked allies < 80% HP (ground AoE)");


            queue->RegisterSpell(REST_WELLSPRING, SpellPriority::MEDIUM, SpellCategory::HEALING);

            queue->AddCondition(REST_WELLSPRING, [this](Player* bot, Unit*) {

                if (!bot->HasSpell(REST_WELLSPRING)) return false;

                auto group = this->GetGroupMembers();

                for (auto* anchor : group)
                {

                    if (!anchor || anchor->GetHealthPct() >= 80.0f) continue;

                    uint32 nearby = 0;

                    for (auto* m : group)

                        if (m && anchor->GetDistance(m) <= 10.0f && m->GetHealthPct() < 80.0f)

                            nearby++;

                    if (nearby >= 4) return true;

                }

                return false;

            }, "4+ stacked allies < 80% HP (instant AoE)");


            queue->RegisterSpell(REST_CHAIN_HEAL, SpellPriority::MEDIUM, SpellCategory::HEALING);

            queue->AddCondition(REST_CHAIN_HEAL, [this](Player*, Unit*) {

                auto group = this->GetGroupMembers();

                uint32 injured = 0;

                for (auto* m : group) if (m && m->GetHealthPct() < 80.0f) injured++;

                return injured >= 2;

            }, "2+ allies < 80% HP (bouncing heal)");


            queue->RegisterSpell(REST_CLOUDBURST_TOTEM, SpellPriority::MEDIUM, SpellCategory::HEALING);

            queue->AddCondition(REST_CLOUDBURST_TOTEM, [this](Player* bot, Unit*) {

                if (!bot->HasSpell(REST_CLOUDBURST_TOTEM)) return false;

                auto group = this->GetGroupMembers();

                uint32 injured = 0;

                for (auto* m : group) if (m && m->GetHealthPct() < 80.0f) injured++;

                return injured >= 3;

            }, "3+ allies < 80% HP (store + release heal, 30s CD)");

            // LOW: Direct single-target healing

            queue->RegisterSpell(REST_HEALING_SURGE, SpellPriority::LOW, SpellCategory::HEALING);

            queue->AddCondition(REST_HEALING_SURGE, [this](Player*, Unit*) {

                auto group = this->GetGroupMembers();

                for (auto* m : group) if (m && m->GetHealthPct() < 50.0f) return true;

                return false;

            }, "Ally < 50% HP (fast emergency heal)");


            queue->RegisterSpell(REST_HEALING_WAVE, SpellPriority::LOW, SpellCategory::HEALING);

            queue->AddCondition(REST_HEALING_WAVE, [this](Player*, Unit*) {

                auto group = this->GetGroupMembers();

                for (auto* m : group) if (m && m->GetHealthPct() < 80.0f) return true;

                return false;

            }, "Ally < 80% HP (efficient heal)");

            // UTILITY: Defensive and buffs

            queue->RegisterSpell(REST_ASTRAL_SHIFT, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);

            queue->AddCondition(REST_ASTRAL_SHIFT, [](Player* bot, Unit*) {

                return bot && bot->GetHealthPct() < 40.0f;

            }, "HP < 40% (40% dmg reduction)");


            queue->RegisterSpell(REST_SPIRITWALKERS_GRACE, SpellPriority::HIGH, SpellCategory::UTILITY);

            queue->AddCondition(REST_SPIRITWALKERS_GRACE, [](Player* bot, Unit*) {

                return bot && bot->GetHealthPct() < 60.0f && bot->HasUnitMovementFlag(MOVEMENTFLAG_FORWARD);

            }, "HP < 60% while moving (heal while moving)");


            queue->RegisterSpell(REST_WATER_SHIELD, SpellPriority::LOW, SpellCategory::UTILITY);

            queue->AddCondition(REST_WATER_SHIELD, [](Player* bot, Unit*) {

                return bot && !bot->HasAura(REST_WATER_SHIELD);

            }, "No Water Shield (mana regen)");


            queue->RegisterSpell(REST_WIND_SHEAR, SpellPriority::HIGH, SpellCategory::CROWD_CONTROL);

            queue->AddCondition(REST_WIND_SHEAR, [](Player*, Unit* target) {

                return target && target->IsNonMeleeSpellCast(false);

            }, "Target casting (interrupt)");


            queue->RegisterSpell(REST_PURIFY_SPIRIT, SpellPriority::MEDIUM, SpellCategory::UTILITY);

            queue->AddCondition(REST_PURIFY_SPIRIT, [this](Player*, Unit*) {

                auto group = this->GetGroupMembers();

                for (auto* m : group)
                {

                    if (m && (m->HasAuraType(SPELL_AURA_PERIODIC_DAMAGE) ||

                               m->HasAuraType(SPELL_AURA_MOD_DECREASE_SPEED)))

                        return true;

                }

                return false;

            }, "Ally has curse/magic/poison (dispel)");
        }

        auto* behaviorTree = ai->GetBehaviorTree();
        if (behaviorTree)
        {

            auto root = Selector("Restoration Shaman Healing", {
                // Tier 1: Emergency Raid Healing

                Sequence("Emergency Totems", {

                    Condition("4+ low HP", [this](Player*, Unit*) {

                        auto group = this->GetGroupMembers();

                        uint32 low = 0;

                        for (auto* m : group) if (m && m->GetHealthPct() < 60.0f) low++;

                        return low >= 4;

                    }),

                    Selector("Use emergency", {

                        Sequence("Healing Tide Totem", {

                            bot::ai::Action("Cast HTT", [this](Player* bot, Unit*) {

                                if (this->CanCastSpell(REST_HEALING_TIDE_TOTEM, bot))
                                {

                                    this->CastSpell(REST_HEALING_TIDE_TOTEM, bot);

                                    this->_lastHealingTideTotemTime = GameTime::GetGameTimeMS();

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),

                        Sequence("Ancestral Protection", {

                            Condition("2+ critical", [this](Player*, Unit*) {

                                auto group = this->GetGroupMembers();

                                uint32 critical = 0;

                                for (auto* m : group) if (m && m->GetHealthPct() < 20.0f) critical++;

                                return critical >= 2;

                            }),

                            Condition("Has spell", [this](Player* bot, Unit*) {

                                return bot->HasSpell(REST_ANCESTRAL_PROTECTION_TOTEM);

                            }),

                            bot::ai::Action("Cast APT", [this](Player* bot, Unit*) {

                                if (this->CanCastSpell(REST_ANCESTRAL_PROTECTION_TOTEM, bot))
                                {

                                    this->CastSpell(REST_ANCESTRAL_PROTECTION_TOTEM, bot);

                                    this->_lastAncestralProtectionTotemTime = GameTime::GetGameTimeMS();

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        })

                    })

                }),

                // Tier 2: Major Healing Cooldowns

                Sequence("Major Cooldowns", {

                    Condition("3+ injured", [this](Player*, Unit*) {

                        auto group = this->GetGroupMembers();

                        uint32 injured = 0;

                        for (auto* m : group) if (m && m->GetHealthPct() < 60.0f) injured++;

                        return injured >= 3;

                    }),

                    Selector("Use cooldowns", {

                        Sequence("Ascendance", {

                            Condition("Not active", [this](Player*, Unit*) {

                                return !this->_ascendanceActive;

                            }),

                            bot::ai::Action("Cast Ascendance", [this](Player* bot, Unit*) {

                                if (this->CanCastSpell(REST_ASCENDANCE, bot))
                                {

                                    this->CastSpell(REST_ASCENDANCE, bot);

                                    this->_ascendanceActive = true;

                                    this->_ascendanceEndTime = GameTime::GetGameTimeMS() + 15000;

                                    this->_lastAscendanceTime = GameTime::GetGameTimeMS();

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),

                        Sequence("Spirit Link Totem", {

                            bot::ai::Action("Cast SLT", [this](Player* bot, Unit*) {

                                if (this->CanCastSpell(REST_SPIRIT_LINK_TOTEM, bot))
                                {

                                    this->CastSpell(REST_SPIRIT_LINK_TOTEM, bot);

                                    this->_lastSpiritLinkTotemTime = GameTime::GetGameTimeMS();

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),

                        Sequence("Earthen Wall Totem", {

                            Condition("Has spell", [this](Player* bot, Unit*) {

                                return bot->HasSpell(REST_EARTHEN_WALL_TOTEM);

                            }),

                            bot::ai::Action("Cast EWT", [this](Player* bot, Unit*) {

                                if (this->CanCastSpell(REST_EARTHEN_WALL_TOTEM, bot))
                                {

                                    this->CastSpell(REST_EARTHEN_WALL_TOTEM, bot);

                                    this->_lastEarthenWallTotemTime = GameTime::GetGameTimeMS();

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        })

                    })

                }),

                // Tier 3: HoT and Shield Maintenance

                Sequence("Maintain HoTs", {

                    Selector("Apply HoTs", {

                        Sequence("Earth Shield Tank", {

                            bot::ai::Action("Cast Earth Shield", [this](Player*, Unit*) {

                                auto group = this->GetGroupMembers();

                                for (auto* m : group)
                                {

                                    if (m && this->IsTankRole(m) && !this->_earthShieldTracker.HasEarthShield(m->GetGUID()))
                                    {

                                        if (this->CanCastSpell(REST_EARTH_SHIELD, m))
                                        {

                                            this->CastSpell(REST_EARTH_SHIELD, m);

                                            this->_earthShieldTracker.ApplyEarthShield(m->GetGUID(), 600000);

                                            return NodeStatus::SUCCESS;

                                        }

                                    }

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),

                        Sequence("Riptide Spread", {

                            bot::ai::Action("Cast Riptide", [this](Player*, Unit*) {

                                auto group = this->GetGroupMembers();

                                for (auto* m : group)
                                {

                                    if (m && m->GetHealthPct() < 90.0f && this->_riptideTracker.NeedsRiptideRefresh(m->GetGUID()))
                                    {

                                        if (this->CanCastSpell(REST_RIPTIDE, m))
                                        {

                                            this->CastSpell(REST_RIPTIDE, m);

                                            this->_riptideTracker.ApplyRiptide(m->GetGUID(), 18000);

                                            return NodeStatus::SUCCESS;

                                        }

                                    }

                                }

                                return NodeStatus::FAILURE;

                            })

                        })

                    })

                }),

                // Tier 4: AoE Healing

                Sequence("AoE Healing", {

                    Condition("2+ injured", [this](Player*, Unit*) {

                        auto group = this->GetGroupMembers();

                        uint32 injured = 0;

                        for (auto* m : group) if (m && m->GetHealthPct() < 80.0f) injured++;

                        return injured >= 2;

                    }),

                    Selector("Cast AoE", {

                        Sequence("Healing Rain", {

                            Condition("3+ stacked", [this](Player*, Unit*) {

                                auto group = this->GetGroupMembers();

                                for (auto* anchor : group)
                                {

                                    if (!anchor || anchor->GetHealthPct() >= 80.0f) continue;

                                    uint32 nearby = 0;

                                    for (auto* m : group)

                                        if (m && anchor->GetDistance(m) <= 10.0f && m->GetHealthPct() < 80.0f)

                                            nearby++;

                                    if (nearby >= 3) return true;

                                }

                                return false;

                            }),

                            bot::ai::Action("Cast Healing Rain", [this](Player*, Unit*) {

                                auto group = this->GetGroupMembers();

                                for (auto* anchor : group)
                                {

                                    if (!anchor || anchor->GetHealthPct() >= 80.0f) continue;

                                    uint32 nearby = 0;

                                    for (auto* m : group)

                                        if (m && anchor->GetDistance(m) <= 10.0f && m->GetHealthPct() < 80.0f)

                                            nearby++;

                                    if (nearby >= 3 && this->CanCastSpell(REST_HEALING_RAIN, anchor)) {

                                        this->CastSpell(REST_HEALING_RAIN, anchor);

                                        return NodeStatus::SUCCESS;

                                    }

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),

                        Sequence("Chain Heal", {

                            bot::ai::Action("Cast Chain Heal", [this](Player*, Unit*) {

                                auto group = this->GetGroupMembers();

                                for (auto* m : group)
                                {

                                    if (m && m->GetHealthPct() < 75.0f)
                                    {

                                        if (this->CanCastSpell(REST_CHAIN_HEAL, m))
                                        {

                                            this->CastSpell(REST_CHAIN_HEAL, m);

                                            return NodeStatus::SUCCESS;

                                        }

                                    }

                                }

                                return NodeStatus::FAILURE;

                            })

                        })

                    })

                }),

                // Tier 5: Direct Healing

                Sequence("Direct Healing", {

                    Selector("Cast heals", {

                        Sequence("Healing Surge", {

                            Condition("Ally < 50%", [this](Player*, Unit*) {

                                auto group = this->GetGroupMembers();

                                for (auto* m : group) if (m && m->GetHealthPct() < 50.0f) return true;

                                return false;

                            }),

                            bot::ai::Action("Cast Healing Surge", [this](Player*, Unit*) {

                                auto group = this->GetGroupMembers();

                                for (auto* m : group)
                                {

                                    if (m && m->GetHealthPct() < 50.0f)
                                    {

                                        if (this->CanCastSpell(REST_HEALING_SURGE, m))
                                        {

                                            this->CastSpell(REST_HEALING_SURGE, m);

                                            return NodeStatus::SUCCESS;

                                        }

                                    }

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),

                        Sequence("Healing Wave", {

                            bot::ai::Action("Cast Healing Wave", [this](Player*, Unit*) {

                                auto group = this->GetGroupMembers();

                                for (auto* m : group)
                                {

                                    if (m && m->GetHealthPct() < 80.0f)
                                    {

                                        if (this->CanCastSpell(REST_HEALING_WAVE, m))
                                        {

                                            this->CastSpell(REST_HEALING_WAVE, m);

                                            return NodeStatus::SUCCESS;

                                        }

                                    }

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

    [[nodiscard]] ::std::vector<Unit*> GetGroupMembers() const
    {
        ::std::vector<Unit*> members;
        Player* bot = this->GetBot();
        if (!bot) return members;

        Group* group = bot->GetGroup();
        if (!group) return members;

        for (GroupReference const& ref : group->GetMembers())
        {

            if (Player* member = ref.GetSource())

            {

                if (member->IsAlive() && bot->IsInMap(member))

                    members.push_back(member);

            }
        }
        return members;
    }

    // Hero talent detection cache (refreshed on combat start)
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot

#endif // PLAYERBOT_RESTORATIONSHAMANREFACTORED_H
