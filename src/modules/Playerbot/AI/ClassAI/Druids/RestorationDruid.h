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

#ifndef PLAYERBOT_RESTORATIONDRUIDREFACTORED_H
#define PLAYERBOT_RESTORATIONDRUIDREFACTORED_H

#include "../CombatSpecializationTemplates.h"
#include "Player.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "Group.h"
#include <unordered_map>
#include <vector>
#include "Log.h"
#include "../../Decision/ActionPriorityQueue.h"
#include "../../Decision/BehaviorTree.h"
#include "../BotAI.h"

// Central Spell Registry - See WoW120Spells::Druid namespace
#include "../SpellValidation_WoW120.h"
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
// WoW 12.0 (The War Within) - Restoration Druid Spell IDs
// Using central registry: WoW120Spells::Druid and WoW120Spells::Druid::Restoration
constexpr uint32 RESTO_REJUVENATION = WoW120Spells::Druid::REJUVENATION;
constexpr uint32 RESTO_REGROWTH = WoW120Spells::Druid::REGROWTH;
constexpr uint32 RESTO_WILD_GROWTH = WoW120Spells::Druid::WILD_GROWTH;
constexpr uint32 RESTO_SWIFTMEND = WoW120Spells::Druid::SWIFTMEND;
constexpr uint32 RESTO_LIFEBLOOM = WoW120Spells::Druid::Restoration::LIFEBLOOM;
constexpr uint32 RESTO_EFFLORESCENCE = WoW120Spells::Druid::Restoration::EFFLORESCENCE;
constexpr uint32 RESTO_TRANQUILITY = WoW120Spells::Druid::Restoration::TRANQUILITY;
constexpr uint32 RESTO_IRONBARK = WoW120Spells::Druid::Restoration::IRONBARK;
constexpr uint32 RESTO_NATURES_SWIFTNESS = WoW120Spells::Druid::Restoration::NATURES_SWIFTNESS;
constexpr uint32 RESTO_CENARION_WARD = WoW120Spells::Druid::Restoration::CENARION_WARD;
constexpr uint32 RESTO_FLOURISH = WoW120Spells::Druid::Restoration::FLOURISH;
constexpr uint32 RESTO_INCARNATION_TREE = WoW120Spells::Druid::Restoration::INCARNATION_TREE;
constexpr uint32 RESTO_NOURISH = WoW120Spells::Druid::Restoration::NOURISH;
constexpr uint32 RESTO_HEALING_TOUCH = WoW120Spells::Druid::Restoration::HEALING_TOUCH;
constexpr uint32 RESTO_INNERVATE = WoW120Spells::Druid::INNERVATE;
constexpr uint32 RESTO_BARKSKIN = WoW120Spells::Druid::BARKSKIN;
constexpr uint32 RESTO_RENEWAL = WoW120Spells::Druid::RENEWAL;
constexpr uint32 RESTO_MOONFIRE = WoW120Spells::Druid::MOONFIRE;
constexpr uint32 RESTO_CLEARCASTING = WoW120Spells::Druid::Restoration::CLEARCASTING_RESTO;

// Mana resource is defined in CombatSpecializationTemplates.h as uint32
// No custom ManaResource struct needed - use ManaResource (uint32 typedef)

// ============================================================================
// CLEARCASTING / OMEN OF CLARITY PROC TRACKER
// ============================================================================
// Omen of Clarity: Lifebloom HoT ticks have a chance to grant Clearcasting,
// making the next Regrowth instant and free. Essential for mana-efficient healing.
class RestoClearcastingTracker
{
public:
    RestoClearcastingTracker() : _active(false) {}

    [[nodiscard]] bool IsActive() const { return _active; }

    void ConsumeProc() { _active = false; }

    void Update(Player* bot)
    {
        if (!bot)
            return;

        _active = bot->HasAura(RESTO_CLEARCASTING);
    }

private:
    bool _active;
};

// HoT (Heal over Time) tracking system
class RestorationHoTTracker
{
public:
    RestorationHoTTracker() = default;

    void ApplyRejuvenation(ObjectGuid guid, uint32 duration = 15000)
    {
        _rejuvenationTargets[guid] = GameTime::GetGameTimeMS() + duration;
    }

    void ApplyLifebloom(ObjectGuid guid, uint32 duration = 15000)
    {
        _lifebloomTargets[guid] = GameTime::GetGameTimeMS() + duration;
    }

    void ApplyWildGrowth(ObjectGuid guid, uint32 duration = 7000)
    {
        _wildGrowthTargets[guid] = GameTime::GetGameTimeMS() + duration;
    }

    void ApplyCenarionWard(ObjectGuid guid, uint32 duration = 30000)
    {
        _cenarionWardTargets[guid] = GameTime::GetGameTimeMS() + duration;
    }

    [[nodiscard]] bool HasRejuvenation(ObjectGuid guid) const
    {
        auto it = _rejuvenationTargets.find(guid);
        return it != _rejuvenationTargets.end() && GameTime::GetGameTimeMS() < it->second;
    }

    [[nodiscard]] bool HasLifebloom(ObjectGuid guid) const
    {
        auto it = _lifebloomTargets.find(guid);
        return it != _lifebloomTargets.end() && GameTime::GetGameTimeMS() < it->second;
    }

    [[nodiscard]] bool HasWildGrowth(ObjectGuid guid) const
    {
        auto it = _wildGrowthTargets.find(guid);
        return it != _wildGrowthTargets.end() && GameTime::GetGameTimeMS() < it->second;
    }

    [[nodiscard]] bool HasCenarionWard(ObjectGuid guid) const
    {
        auto it = _cenarionWardTargets.find(guid);
        return it != _cenarionWardTargets.end() && GameTime::GetGameTimeMS() < it->second;
    }

    [[nodiscard]] uint32 GetLifebloomTimeRemaining(ObjectGuid guid) const
    {
        auto it = _lifebloomTargets.find(guid);
        if (it != _lifebloomTargets.end())
        {

            uint32 now = GameTime::GetGameTimeMS();

            return now < it->second ? (it->second - now) : 0;
        }
        return 0;
    }

    [[nodiscard]] bool NeedsLifebloomRefresh(ObjectGuid guid, uint32 pandemicWindow = 4500) const
    {
        uint32 remaining = GetLifebloomTimeRemaining(guid);
        return remaining < pandemicWindow; // Refresh in pandemic window
    }

    [[nodiscard]] uint32 GetActiveRejuvenationCount() const
    {
        uint32 count = 0;
        uint32 now = GameTime::GetGameTimeMS();
        for (const auto& pair : _rejuvenationTargets)
        {

            if (now < pair.second)

                ++count;
        }
        return count;
    }

    void Update(const ::std::vector<Unit*>& group)
    {
        for (Unit* member : group)
        {
            if (!member)
                continue;

            ObjectGuid guid = member->GetGUID();

            // Sync with actual auras
            if (Aura* rejuv = member->GetAura(RESTO_REJUVENATION))
            {
                _rejuvenationTargets[guid] = GameTime::GetGameTimeMS() + rejuv->GetDuration();
            }
            else
            {
                _rejuvenationTargets.erase(guid);
            }

            if (Aura* lifebloom = member->GetAura(RESTO_LIFEBLOOM))
            {
                _lifebloomTargets[guid] = GameTime::GetGameTimeMS() + lifebloom->GetDuration();
            }
            else
            {
                _lifebloomTargets.erase(guid);
            }

            if (Aura* wildGrowth = member->GetAura(RESTO_WILD_GROWTH))
            {
                _wildGrowthTargets[guid] = GameTime::GetGameTimeMS() + wildGrowth->GetDuration();
            }
            else
            {
                _wildGrowthTargets.erase(guid);
            }

            if (Aura* cenarionWard = member->GetAura(RESTO_CENARION_WARD))
            {
                _cenarionWardTargets[guid] = GameTime::GetGameTimeMS() + cenarionWard->GetDuration();
            }
            else
            {
                _cenarionWardTargets.erase(guid);
            }
        }
    }

private:
    CooldownManager _cooldowns;
    ::std::unordered_map<ObjectGuid, uint32> _rejuvenationTargets;
    ::std::unordered_map<ObjectGuid, uint32> _lifebloomTargets;
    ::std::unordered_map<ObjectGuid, uint32> _wildGrowthTargets;
    ::std::unordered_map<ObjectGuid, uint32> _cenarionWardTargets;
};

// Swiftmend target tracker (requires a HoT to cast)
class RestorationSwiftmendTracker
{
public:
    RestorationSwiftmendTracker() : _lastSwiftmendTime(0) {}

    [[nodiscard]] bool CanUseSwiftmend() const
    {
        // 15 sec cooldown
        return (GameTime::GetGameTimeMS() - _lastSwiftmendTime) >= 15000;
    }

    void UseSwiftmend()
    {
        _lastSwiftmendTime = GameTime::GetGameTimeMS();
    }
    private:
    uint32 _lastSwiftmendTime;
};

class RestorationDruidRefactored : public HealerSpecialization<ManaResource>
{
public:
    using Base = HealerSpecialization<ManaResource>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::_resource;
    explicit RestorationDruidRefactored(Player* bot)        : HealerSpecialization<ManaResource>(bot)

        , _hotTracker()
        , _swiftmendTracker()
        , _clearcastingTracker()
        , _treeFormActive(false)
        , _treeFormEndTime(0)
        
        , _lastInnervateTime(0)
        , _cooldowns()
    {
        // Register cooldowns for major abilities
        // COMMENTED OUT:         _cooldowns.RegisterBatch({
        // COMMENTED OUT: 
        // COMMENTED OUT:             {RESTO_DRUID_TRANQUILITY, 180000, 1},
        // COMMENTED OUT: 
        // COMMENTED OUT:             {RESTO_DRUID_INCARNATION, 180000, 1},
        // COMMENTED OUT: 
        // COMMENTED OUT:             {RESTO_DRUID_CONVOKE, 120000, 1},
        // COMMENTED OUT: 
        // COMMENTED OUT:             {RESTO_DRUID_FLOURISH, 90000, 1},
        // COMMENTED OUT: 
        // COMMENTED OUT:             {RESTO_DRUID_IRONBARK, 90000, 1}
        // COMMENTED OUT:         });

        // _resource is uint32, no Initialize method - managed by base class

        // Phase 5: Initialize decision systems
        InitializeRestorationMechanics();

        // Register healing spell efficiency tiers
        GetEfficiencyManager().RegisterSpell(RESTO_REJUVENATION, HealingSpellTier::VERY_HIGH, "Rejuvenation");
        GetEfficiencyManager().RegisterSpell(RESTO_LIFEBLOOM, HealingSpellTier::VERY_HIGH, "Lifebloom");
        GetEfficiencyManager().RegisterSpell(RESTO_WILD_GROWTH, HealingSpellTier::MEDIUM, "Wild Growth");
        GetEfficiencyManager().RegisterSpell(RESTO_REGROWTH, HealingSpellTier::HIGH, "Regrowth");
        GetEfficiencyManager().RegisterSpell(RESTO_SWIFTMEND, HealingSpellTier::LOW, "Swiftmend");
        GetEfficiencyManager().RegisterSpell(RESTO_TRANQUILITY, HealingSpellTier::EMERGENCY, "Tranquility");
        GetEfficiencyManager().RegisterSpell(RESTO_IRONBARK, HealingSpellTier::EMERGENCY, "Ironbark");
        GetEfficiencyManager().RegisterSpell(RESTO_CENARION_WARD, HealingSpellTier::MEDIUM, "Cenarion Ward");
        GetEfficiencyManager().RegisterSpell(RESTO_FLOURISH, HealingSpellTier::LOW, "Flourish");

        TC_LOG_DEBUG("playerbot", "RestorationDruidRefactored initialized for bot {}", bot->GetGUID().GetCounter());
    }

    void UpdateRotation(::Unit* target) override
    {
        Player* bot = this->GetBot();
        if (!bot)

            return;

        // Detect hero talents if not yet cached
        if (!_heroTalents.detected)
            _heroTalents.Refresh(this->GetBot());

        // Hero talent rotation branching
        // Restoration Druid has access to: Keeper of the Grove / Wildstalker
        if (_heroTalents.IsTree(HeroTalentTree::KEEPER_OF_THE_GROVE))
        {
            // Keeper of the Grove: Grove Guardians summon healing treants
            if (this->CanCastSpell(WoW120Spells::Druid::Restoration::GROVE_GUARDIANS, bot))
            {
                // Summon grove guardians when healing is needed
                if (bot->GetPowerPct(POWER_MANA) > 20.0f)
                {
                    this->CastSpell(WoW120Spells::Druid::Restoration::GROVE_GUARDIANS, bot);
                    return;
                }
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::WILDSTALKER))
        {
            // Wildstalker: Strategic Infusion enhances HoTs
            if (this->CanCastSpell(WoW120Spells::Druid::Restoration::STRATEGIC_INFUSION, bot))
            {
                this->CastSpell(WoW120Spells::Druid::Restoration::STRATEGIC_INFUSION, bot);
                return;
            }
        }

        UpdateRestorationState();

        ::std::vector<Unit*> group = GetGroupMembers();
        if (group.empty())

            group.push_back(bot); // Solo healing (self)

        ExecuteHealingRotation(group);
    }

    void UpdateBuffs() override
    {
        Player* bot = this->GetBot();
        if (!bot)

            return;

        // Innervate for mana regeneration
        if (bot->GetPowerPct(POWER_MANA) < 30)
        {

            if (this->CanCastSpell(RESTO_INNERVATE, bot))

            {

                this->CastSpell(RESTO_INNERVATE, bot);

                _lastInnervateTime = GameTime::GetGameTimeMS();

            }
        }
    }

    void UpdateDefensives()
    {
        Player* bot = this->GetBot();
        if (!bot)

            return;

        float healthPct = bot->GetHealthPct();

        // Barkskin (personal damage reduction)
        if (healthPct < 50.0f && this->CanCastSpell(RESTO_BARKSKIN, bot))
        {

            this->CastSpell(RESTO_BARKSKIN, bot);

            return;
        }

        // Renewal (self-heal)
        if (healthPct < 60.0f && this->CanCastSpell(RESTO_RENEWAL, bot))
        {

            this->CastSpell(RESTO_RENEWAL, bot);

            return;
        }
    }

private:    

    void UpdateRestorationState()
    {
        Player* bot = this->GetBot();
        if (!bot)

            return;

        // Update Clearcasting (Omen of Clarity) proc status
        _clearcastingTracker.Update(bot);

        // _resource is uint32, no Update method - managed by base class
        UpdateCooldownStates();
    }

    void UpdateCooldownStates()
    {
        Player* bot = this->GetBot();
        if (!bot)

            return;

        // Tree Form state
        if (_treeFormActive && GameTime::GetGameTimeMS() >= _treeFormEndTime)

            _treeFormActive = false;

        if (bot->HasAura(RESTO_INCARNATION_TREE))
        {

            _treeFormActive = true;

            if (Aura* aura = bot->GetAura(RESTO_INCARNATION_TREE))

                _treeFormEndTime = GameTime::GetGameTimeMS() + aura->GetDuration();
                }
    }

    void ExecuteHealingRotation(const ::std::vector<Unit*>& group)
    {
        _hotTracker.Update(group);

        // Emergency group-wide healing
        if (HandleEmergencyHealing(group))

            return;

        // Priority: Consume Clearcasting proc on Regrowth (free instant Regrowth)
        if (_clearcastingTracker.IsActive())
        {
            // Find most injured group member for free Regrowth
            Unit* ccTarget = nullptr;
            float lowestPct = 90.0f;
            for (Unit* member : group)
            {
                if (member && member->GetHealthPct() < lowestPct)
                {
                    lowestPct = member->GetHealthPct();
                    ccTarget = member;
                }
            }
            if (ccTarget && this->CanCastSpell(RESTO_REGROWTH, ccTarget))
            {
                this->CastSpell(RESTO_REGROWTH, ccTarget);
                _clearcastingTracker.ConsumeProc();
                return; // Free Regrowth - don't waste the proc
            }
        }

        // Maintain Lifebloom on tank
        if (HandleLifebloom(group))

            return;

        // Spread Rejuvenation
        if (HandleRejuvenation(group))

            return;

        // Wild Growth for AoE healing
        if (HandleWildGrowth(group))

            return;

        // Swiftmend for quick single-target healing
        if (HandleSwiftmend(group))

            return;

        // Cenarion Ward (talent)
        if (HandleCenarionWard(group))

            return;

        // Regrowth for direct healing
        if (HandleRegrowth(group))

            return;

        // DPS rotation when no healing needed
        HandleDPSRotation();
    }

    bool HandleEmergencyHealing(const ::std::vector<Unit*>& group)
    {
        Player* bot = this->GetBot();
        if (!bot)

            return false;

        // Count critically injured allies
        uint32 criticalCount = 0;
        for (Unit* member : group)
        {

            if (member && member->GetHealthPct() < 40.0f)

                ++criticalCount;
        }

        // Tranquility (raid-wide emergency healing)
        if (criticalCount >= 3 && (GameTime::GetGameTimeMS() - _lastTranquilityTime) >= 180000) // 3 min CD
        {

            if (this->CanCastSpell(RESTO_TRANQUILITY, bot))

            {

                this->CastSpell(RESTO_TRANQUILITY, bot);

                _lastTranquilityTime = GameTime::GetGameTimeMS();

                return true;

            }
        }

        // Incarnation: Tree of Life (major healing CD)
        if (criticalCount >= 2 && !_treeFormActive)
        {

            if (this->CanCastSpell(RESTO_INCARNATION_TREE, bot))

            {

                this->CastSpell(RESTO_INCARNATION_TREE, bot);

                _treeFormActive = true;

                _treeFormEndTime = GameTime::GetGameTimeMS() + 30000; // 30 sec

                return true;

            }
        }

        // Nature's Swiftness + Regrowth instant cast
        for (Unit* member : group)
        {
            if (member && member->GetHealthPct() < 30.0f)
            {
                if (this->CanCastSpell(RESTO_NATURES_SWIFTNESS, bot))
                {
                    this->CastSpell(RESTO_NATURES_SWIFTNESS, bot);
                    if (this->CanCastSpell(RESTO_REGROWTH, member))
                    {
                        this->CastSpell(RESTO_REGROWTH, member);
                        return true;
                    }
                }
            }
        }

        // Ironbark on tank taking heavy damage
        for (Unit* member : group)
        {

            if (member && member->GetHealthPct() < 50.0f && IsTank(member))

            {

                if (this->CanCastSpell(RESTO_IRONBARK, member))

                {

                    this->CastSpell(RESTO_IRONBARK, member);

                    return true;

                }

            }
        }

        return false;
    }

    bool HandleLifebloom(const ::std::vector<Unit*>& group)
    {
        // Maintain Lifebloom on primary tank
        Unit* tank = GetMainTank(group);

        if (!tank)
        {
            return false;
        }

        if (!tank)
        {
            TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: tank in method GetGUID");
            return false;
        }
        if (tank && _hotTracker.NeedsLifebloomRefresh(tank->GetGUID()))
        {

            if (this->CanCastSpell(RESTO_LIFEBLOOM, tank))

            {

                this->CastSpell(RESTO_LIFEBLOOM, tank);

                _hotTracker.ApplyLifebloom(tank->GetGUID(), 15000);

                return true;

            }
        }

        return false;
    }    bool HandleRejuvenation(const ::std::vector<Unit*>& group)
    {
        uint32 activeRejuvs = _hotTracker.GetActiveRejuvenationCount();

        // Spread Rejuvenation to injured allies (maintain on ~3-4 targets)
        if (activeRejuvs < 4)
        {

            for (Unit* member : group)
            {

                if (member && member->GetHealthPct() < 95.0f && !_hotTracker.HasRejuvenation(member->GetGUID()))

                {

                    if (this->CanCastSpell(RESTO_REJUVENATION, member))

                    {

                        this->CastSpell(RESTO_REJUVENATION, member);

                        _hotTracker.ApplyRejuvenation(member->GetGUID(), 15000);

                        return true;

                    }

                }

            }
        }

        return false;
    }

    bool HandleWildGrowth(const ::std::vector<Unit*>& group)
    {
        // Count injured allies without Wild Growth
        uint32 needsHealing = 0;
        for (Unit* member : group)
        {
            if (member && member->GetHealthPct() < 85.0f && !_hotTracker.HasWildGrowth(member->GetGUID()))
                ++needsHealing;
        }

        // Use Wild Growth if 3+ allies need healing
        if (needsHealing >= 3)
        {

            Unit* target = GetGroupMemberNeedingHealing(group, 85.0f);

            if (target && IsHealAllowedByMana(RESTO_WILD_GROWTH) && this->CanCastSpell(RESTO_WILD_GROWTH, target))

            {

                this->CastSpell(RESTO_WILD_GROWTH, target);
                // Apply to all nearby allies (simplified)

                for (Unit* member : group)
                {

                    if (member)

                        _hotTracker.ApplyWildGrowth(member->GetGUID(), 7000);

                }

                return true;

            }
        }

        return false;
    }

    bool HandleSwiftmend(const ::std::vector<Unit*>& group)
    {
        if (!_swiftmendTracker.CanUseSwiftmend())

            return false;

        // Use Swiftmend on injured ally with a HoT
        for (Unit* member : group)
        {

            if (member && member->GetHealthPct() < 70.0f)

            {

                if (_hotTracker.HasRejuvenation(member->GetGUID()) || _hotTracker.HasWildGrowth(member->GetGUID()))

                {

                    if (IsHealAllowedByMana(RESTO_SWIFTMEND) && this->CanCastSpell(RESTO_SWIFTMEND, member))

                    {

                        this->CastSpell(RESTO_SWIFTMEND, member);

                        _swiftmendTracker.UseSwiftmend();

                        return true;

                    }

                }

            }
        }

        return false;    }

    bool HandleCenarionWard(const ::std::vector<Unit*>& group)
    {
        Player* bot = this->GetBot();
        
        if (!bot || !bot->HasSpell(RESTO_CENARION_WARD))

            return false;

        // Apply Cenarion Ward to tank
        Unit* tank = GetMainTank(group);

        if (!tank)
        {
            return false;
        }

        if (!tank)
        {
            return false;
        }
        if (tank && !_hotTracker.HasCenarionWard(tank->GetGUID()))
        {

            if (IsHealAllowedByMana(RESTO_CENARION_WARD) && this->CanCastSpell(RESTO_CENARION_WARD, tank))

            {

                this->CastSpell(RESTO_CENARION_WARD, tank);

                _hotTracker.ApplyCenarionWard(tank->GetGUID(), 30000);

                return true;

            }
        }

        return false;
    }

    bool HandleRegrowth(const ::std::vector<Unit*>& group)
    {
        // Use Regrowth for direct healing when needed
        for (Unit* member : group)
        {

            if (member && member->GetHealthPct() < 80.0f)

            {

                if (IsHealAllowedByMana(RESTO_REGROWTH) && this->CanCastSpell(RESTO_REGROWTH, member))

                {

                    this->CastSpell(RESTO_REGROWTH, member);

                    return true;

                }

            }
        }

        return false;
    }

    void HandleDPSRotation()
    {
        Player* bot = this->GetBot();
        if (!bot)

            return;

        // When no healing is needed, contribute damage
        Unit* target = bot->GetVictim();
        if (!target)

            target = FindNearbyEnemy();

        if (target)
        {
            // Moonfire for ranged damage

            if (!target->HasAura(RESTO_MOONFIRE) && this->CanCastSpell(RESTO_MOONFIRE, target))

            {

                this->CastSpell(RESTO_MOONFIRE, target);

            }
        }
    }

    [[nodiscard]] ::std::vector<Unit*> GetGroupMembers() const
    {
        ::std::vector<Unit*> members;

        Player* bot = this->GetBot();
        if (!bot)

            return members;

        Group* group = bot->GetGroup();
        if (!group)

            return members;

        for (GroupReference& itr : group->GetMembers())
        {

            if (Player* member = itr.GetSource())

            {

                if (member->IsInWorld() && bot->GetDistance(member) <= 40.0f)
                members.push_back(member);

            }
        }

        return members;
    }

    [[nodiscard]] Unit* GetGroupMemberNeedingHealing(const ::std::vector<Unit*>& group, float healthThreshold) const
    {
        Unit* mostInjured = nullptr;
        float lowestHealth = 100.0f;

        for (Unit* member : group)
        {

            if (member && member->GetHealthPct() < healthThreshold && member->GetHealthPct() < lowestHealth)

            {

                mostInjured = member;

                lowestHealth = member->GetHealthPct();

            }
        }

        return mostInjured;
    }

    [[nodiscard]] Unit* GetMainTank(const ::std::vector<Unit*>& group) const
    {
        // Find the group member with tank role or highest threat
        for (Unit* member : group)
        {

            if (member && IsTank(member))

                return member;
        }

        return !group.empty() ? group[0] : nullptr;
    }

    [[nodiscard]] bool IsTank(Unit* unit) const
    {
        if (!unit)

            return false;

        // Check if player has tank spec/role
        if (Player* player = unit->ToPlayer())
        {
            // Simplified tank detection - check if unit is actively tanking
            // GetPrimaryTalentTree API is deprecated in TrinityCore
            // Use threat/role-based detection instead

            return player->GetVictim() != nullptr;
        }

        return false;
    }

    [[nodiscard]] Unit* FindNearbyEnemy() const
    {
        // Simplified enemy finding - in real implementation, query nearby hostile units
        Player* bot = this->GetBot();
        return bot ? bot->GetVictim() : nullptr;
    }

    // Member variables
    RestorationHoTTracker _hotTracker;
    RestorationSwiftmendTracker _swiftmendTracker;
    RestoClearcastingTracker _clearcastingTracker;

    bool _treeFormActive;
    uint32 _treeFormEndTime;
    uint32 _lastInnervateTime;
    uint32 _lastTranquilityTime = 0;
    CooldownManager _cooldowns;

    // Hero talent detection cache (refreshed on combat start)
    HeroTalentCache _heroTalents;

    // ========================================================================
    // PHASE 5: DECISION SYSTEM INTEGRATION
    // ========================================================================

    void InitializeRestorationMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)
        BotAI* ai = this;
        if (!ai) return;

        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            // EMERGENCY: Raid-wide emergency healing

            queue->RegisterSpell(RESTO_TRANQUILITY, SpellPriority::EMERGENCY, SpellCategory::HEALING);

            queue->AddCondition(RESTO_TRANQUILITY, [this](Player*, Unit*) {

                auto group = this->GetGroupMembers();

                uint32 critical = 0;

                for (auto* m : group) if (m && m->GetHealthPct() < 40.0f) critical++;

                return critical >= 3;

            }, "3+ allies < 40% HP (channel heal)");


            queue->RegisterSpell(RESTO_NATURES_SWIFTNESS, SpellPriority::EMERGENCY, SpellCategory::HEALING);

            queue->AddCondition(RESTO_NATURES_SWIFTNESS, [this](Player*, Unit*) {

                auto group = this->GetGroupMembers();

                for (auto* m : group) if (m && m->GetHealthPct() < 30.0f) return true;

                return false;

            }, "Ally < 30% HP (instant cast)");

            // CRITICAL: Major healing cooldowns

            queue->RegisterSpell(RESTO_INCARNATION_TREE, SpellPriority::CRITICAL, SpellCategory::HEALING);

            queue->AddCondition(RESTO_INCARNATION_TREE, [this](Player*, Unit*) {

                if (this->_treeFormActive) return false;

                auto group = this->GetGroupMembers();

                uint32 injured = 0;

                for (auto* m : group) if (m && m->GetHealthPct() < 60.0f) injured++;

                return injured >= 2;

            }, "2+ allies < 60% HP (30s form)");


            queue->RegisterSpell(RESTO_IRONBARK, SpellPriority::CRITICAL, SpellCategory::DEFENSIVE);

            queue->AddCondition(RESTO_IRONBARK, [this](Player*, Unit*) {

                auto group = this->GetGroupMembers();

                for (auto* m : group)

                    if (m && m->GetHealthPct() < 50.0f && this->IsTank(m)) return true;

                return false;

            }, "Tank < 50% HP (20% dmg reduction)");

            // HIGH: Core HoT maintenance

            queue->RegisterSpell(RESTO_LIFEBLOOM, SpellPriority::HIGH, SpellCategory::HEALING);

            queue->AddCondition(RESTO_LIFEBLOOM, [this](Player*, Unit*) {

                auto group = this->GetGroupMembers();

                Unit* tank = this->GetMainTank(group);

                return tank && this->_hotTracker.NeedsLifebloomRefresh(tank->GetGUID());

            }, "Tank needs Lifebloom (pandemic refresh)");


            queue->RegisterSpell(RESTO_SWIFTMEND, SpellPriority::HIGH, SpellCategory::HEALING);

            queue->AddCondition(RESTO_SWIFTMEND, [this](Player*, Unit*) {

                if (!this->_swiftmendTracker.CanUseSwiftmend()) return false;

                auto group = this->GetGroupMembers();

                for (auto* m : group)
                {

                    if (m && m->GetHealthPct() < 70.0f)
                    {

                        auto guid = m->GetGUID();

                        if (this->_hotTracker.HasRejuvenation(guid) ||

                            this->_hotTracker.HasWildGrowth(guid)) return true;

                    }

                }

                return false;

            }, "Ally < 70% HP with HoT (instant heal)");


            queue->RegisterSpell(RESTO_WILD_GROWTH, SpellPriority::HIGH, SpellCategory::HEALING);

            queue->AddCondition(RESTO_WILD_GROWTH, [this](Player*, Unit*) {

                auto group = this->GetGroupMembers();

                uint32 needs = 0;

                for (auto* m : group)

                    if (m && m->GetHealthPct() < 85.0f && !this->_hotTracker.HasWildGrowth(m->GetGUID()))

                        needs++;

                return needs >= 3;

            }, "3+ allies < 85% HP (AoE HoT)");

            // MEDIUM: Rejuvenation spreading

            queue->RegisterSpell(RESTO_REJUVENATION, SpellPriority::MEDIUM, SpellCategory::HEALING);

            queue->AddCondition(RESTO_REJUVENATION, [this](Player*, Unit*) {

                uint32 active = this->_hotTracker.GetActiveRejuvenationCount();

                if (active >= 4) return false;

                auto group = this->GetGroupMembers();

                for (auto* m : group)

                    if (m && m->GetHealthPct() < 95.0f && !this->_hotTracker.HasRejuvenation(m->GetGUID()))

                        return true;

                return false;

            }, "Ally < 95% HP, maintain 4 HoTs");


            queue->RegisterSpell(RESTO_CENARION_WARD, SpellPriority::MEDIUM, SpellCategory::HEALING);

            queue->AddCondition(RESTO_CENARION_WARD, [this](Player* bot, Unit*) {

                if (!bot->HasSpell(RESTO_CENARION_WARD)) return false;

                auto group = this->GetGroupMembers();

                Unit* tank = this->GetMainTank(group);

                return tank && !this->_hotTracker.HasCenarionWard(tank->GetGUID());

            }, "Tank needs Cenarion Ward (30s reactive)");


            queue->RegisterSpell(RESTO_REGROWTH, SpellPriority::MEDIUM, SpellCategory::HEALING);

            queue->AddCondition(RESTO_REGROWTH, [this](Player*, Unit*) {

                auto group = this->GetGroupMembers();

                for (auto* m : group)

                    if (m && m->GetHealthPct() < 80.0f) return true;

                return false;

            }, "Ally < 80% HP (direct + HoT)");

            // LOW: Mana management

            queue->RegisterSpell(RESTO_INNERVATE, SpellPriority::LOW, SpellCategory::UTILITY);

            queue->AddCondition(RESTO_INNERVATE, [this](Player* bot, Unit*) {

                return bot && bot->GetPowerPct(POWER_MANA) < 30.0f;

            }, "Mana < 30% (100% regen)");

            // DEFENSIVE: Personal defensives

            queue->RegisterSpell(RESTO_BARKSKIN, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);

            queue->AddCondition(RESTO_BARKSKIN, [](Player* bot, Unit*) {

                return bot && bot->GetHealthPct() < 50.0f;

            }, "HP < 50% (20% dmg reduction)");


            queue->RegisterSpell(RESTO_RENEWAL, SpellPriority::HIGH, SpellCategory::HEALING);

            queue->AddCondition(RESTO_RENEWAL, [](Player* bot, Unit*) {

                return bot && bot->GetHealthPct() < 60.0f;

            }, "HP < 60% (self-heal)");

            // UTILITY: DPS contribution

            queue->RegisterSpell(RESTO_MOONFIRE, SpellPriority::LOW, SpellCategory::DAMAGE_SINGLE);

            queue->AddCondition(RESTO_MOONFIRE, [](Player* bot, Unit* target) {

                return target && !target->HasAura(RESTO_MOONFIRE);

            }, "No Moonfire (contribute DPS)");
        }

        auto* behaviorTree = ai->GetBehaviorTree();
        if (behaviorTree)
        {

            auto root = Selector("Restoration Druid Healing", {
                // Tier 1: Emergency Group Healing

                Sequence("Emergency Healing", {

                    Condition("3+ critical", [this](Player*, Unit*) {

                        auto group = this->GetGroupMembers();

                        uint32 critical = 0;

                        for (auto* m : group) if (m && m->GetHealthPct() < 40.0f) critical++;

                        return critical >= 3;

                    }),

                    Selector("Use emergency", {

                        Sequence("Tranquility", {

                            bot::ai::Action("Cast Tranquility", [this](Player* bot, Unit*) {

                                if (this->CanCastSpell(RESTO_TRANQUILITY, bot))
                                {

                                    this->CastSpell(RESTO_TRANQUILITY, bot);

                                    this->_lastTranquilityTime = GameTime::GetGameTimeMS();

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),

                        Sequence("Nature's Swiftness", {

                            bot::ai::Action("Instant Regrowth", [this](Player* bot, Unit*) {

                                auto group = this->GetGroupMembers();

                                for (auto* m : group)
                                {

                                    if (m && m->GetHealthPct() < 30.0f)
                                    {

                                        if (this->CanCastSpell(RESTO_NATURES_SWIFTNESS, bot))
                                        {

                                            this->CastSpell(RESTO_NATURES_SWIFTNESS, bot);

                                            if (this->CanCastSpell(RESTO_REGROWTH, m))
                                            {

                                                this->CastSpell(RESTO_REGROWTH, m);

                                                return NodeStatus::SUCCESS;

                                            }

                                        }

                                    }

                                }

                                return NodeStatus::FAILURE;

                            })

                        })

                    })

                }),

                // Tier 2: Major Healing Cooldowns

                Sequence("Major Cooldowns", {

                    Condition("2+ injured", [this](Player*, Unit*) {

                        auto group = this->GetGroupMembers();

                        uint32 injured = 0;

                        for (auto* m : group) if (m && m->GetHealthPct() < 60.0f) injured++;

                        return injured >= 2;

                    }),

                    Selector("Use cooldowns", {

                        Sequence("Tree Form", {

                            Condition("Not active", [this](Player*, Unit*) {

                                return !this->_treeFormActive;

                            }),

                            bot::ai::Action("Cast Incarnation", [this](Player* bot, Unit*) {

                                if (this->CanCastSpell(RESTO_INCARNATION_TREE, bot))
                                {

                                    this->CastSpell(RESTO_INCARNATION_TREE, bot);

                                    this->_treeFormActive = true;

                                    this->_treeFormEndTime = GameTime::GetGameTimeMS() + 30000;

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),

                        Sequence("Ironbark Tank", {

                            bot::ai::Action("Cast Ironbark", [this](Player*, Unit*) {

                                auto group = this->GetGroupMembers();

                                for (auto* m : group)
                                {

                                    if (m && m->GetHealthPct() < 50.0f && this->IsTank(m))
                                    {

                                        if (this->CanCastSpell(RESTO_IRONBARK, m))
                                        {

                                            this->CastSpell(RESTO_IRONBARK, m);

                                            return NodeStatus::SUCCESS;

                                        }

                                    }

                                }

                                return NodeStatus::FAILURE;

                            })

                        })

                    })

                }),

                // Tier 3: HoT Maintenance

                Sequence("Maintain HoTs", {

                    Selector("Apply HoTs", {

                        Sequence("Lifebloom Tank", {

                            bot::ai::Action("Cast Lifebloom", [this](Player*, Unit*) {

                                auto group = this->GetGroupMembers();

                                Unit* tank = this->GetMainTank(group);

                                if (tank && this->_hotTracker.NeedsLifebloomRefresh(tank->GetGUID()))
                                {

                                    if (this->CanCastSpell(RESTO_LIFEBLOOM, tank))
                                    {

                                        this->CastSpell(RESTO_LIFEBLOOM, tank);

                                        this->_hotTracker.ApplyLifebloom(tank->GetGUID(), 15000);

                                        return NodeStatus::SUCCESS;

                                    }

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),

                        Sequence("Wild Growth AoE", {

                            Condition("3+ need healing", [this](Player*, Unit*) {

                                auto group = this->GetGroupMembers();

                                uint32 needs = 0;

                                for (auto* m : group)

                                    if (m && m->GetHealthPct() < 85.0f && !this->_hotTracker.HasWildGrowth(m->GetGUID()))

                                        needs++;

                                return needs >= 3;

                            }),

                            bot::ai::Action("Cast Wild Growth", [this](Player*, Unit*) {

                                auto group = this->GetGroupMembers();

                                Unit* target = this->GetGroupMemberNeedingHealing(group, 85.0f);

                                if (target && this->CanCastSpell(RESTO_WILD_GROWTH, target))
                                {

                                    this->CastSpell(RESTO_WILD_GROWTH, target);

                                    for (auto* m : group)
                                    {

                                        if (m) this->_hotTracker.ApplyWildGrowth(m->GetGUID(), 7000);

                                    }

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),

                        Sequence("Rejuvenation Spread", {

                            Condition("< 4 active", [this](Player*, Unit*) {

                                return this->_hotTracker.GetActiveRejuvenationCount() < 4;

                            }),

                            bot::ai::Action("Cast Rejuvenation", [this](Player*, Unit*) {

                                auto group = this->GetGroupMembers();

                                for (auto* m : group)
                                {

                                    if (m && m->GetHealthPct() < 95.0f && !this->_hotTracker.HasRejuvenation(m->GetGUID()))
                                    {

                                        if (this->CanCastSpell(RESTO_REJUVENATION, m))
                                        {

                                            this->CastSpell(RESTO_REJUVENATION, m);

                                            this->_hotTracker.ApplyRejuvenation(m->GetGUID(), 15000);

                                            return NodeStatus::SUCCESS;

                                        }

                                    }

                                }

                                return NodeStatus::FAILURE;

                            })

                        })

                    })

                }),

                // Tier 4: Direct Healing

                Sequence("Direct Healing", {

                    Selector("Cast heals", {

                        Sequence("Swiftmend", {

                            Condition("Can use", [this](Player*, Unit*) {

                                return this->_swiftmendTracker.CanUseSwiftmend();

                            }),

                            bot::ai::Action("Cast Swiftmend", [this](Player*, Unit*) {

                                auto group = this->GetGroupMembers();

                                for (auto* m : group)
                                {

                                    if (m && m->GetHealthPct() < 70.0f)
                                    {

                                        auto guid = m->GetGUID();

                                        if (this->_hotTracker.HasRejuvenation(guid) || this->_hotTracker.HasWildGrowth(guid))
                                        {

                                            if (this->CanCastSpell(RESTO_SWIFTMEND, m))
                                            {

                                                this->CastSpell(RESTO_SWIFTMEND, m);

                                                this->_swiftmendTracker.UseSwiftmend();

                                                return NodeStatus::SUCCESS;

                                            }

                                        }

                                    }

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),

                        Sequence("Regrowth", {

                            bot::ai::Action("Cast Regrowth", [this](Player*, Unit*) {

                                auto group = this->GetGroupMembers();

                                for (auto* m : group)
                                {

                                    if (m && m->GetHealthPct() < 80.0f)
                                    {

                                        if (this->CanCastSpell(RESTO_REGROWTH, m))
                                        {

                                            this->CastSpell(RESTO_REGROWTH, m);

                                            return NodeStatus::SUCCESS;

                                        }

                                    }

                                }

                                return NodeStatus::FAILURE;

                            })

                        })

                    })

                }),

                // Tier 5: DPS Contribution

                Sequence("DPS Filler", {

                    Condition("Group healthy", [this](Player*, Unit*) {

                        auto group = this->GetGroupMembers();

                        for (auto* m : group) if (m && m->GetHealthPct() < 90.0f) return false;

                        return true;

                    }),

                    bot::ai::Action("Cast Moonfire", [this](Player* bot, Unit* target) {
                        if (!target) target = this->FindNearbyEnemy();

                        if (target && !target->HasAura(RESTO_MOONFIRE))
                        {

                            if (this->CanCastSpell(RESTO_MOONFIRE, target))
                            {

                                this->CastSpell(RESTO_MOONFIRE, target);

                                return NodeStatus::SUCCESS;

                            }

                        }

                        return NodeStatus::FAILURE;

                    })

                })

            });


            behaviorTree->SetRoot(root);
        }
    }
};

} // namespace Playerbot

#endif // PLAYERBOT_RESTORATIONDRUIDREFACTORED_H
