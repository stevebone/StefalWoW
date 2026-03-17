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

#ifndef PLAYERBOT_HOLYPRIESTREFACTORED_H
#define PLAYERBOT_HOLYPRIESTREFACTORED_H

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
#include "../Common/CooldownManager.h"
#include "../../BotAI.h"

// TrinityCore 12.0 New Priest Talents
#include "PriestTalentEnhancements.h"

// Central Spell Registry - See WoW120Spells::Priest namespace
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
// WoW 12.0 (The War Within) - Holy Priest Spell IDs
// Using central registry: WoW120Spells::Priest and WoW120Spells::Priest::HolyPriest
constexpr uint32 HOLY_HEAL = WoW120Spells::Priest::HolyPriest::HEAL;
constexpr uint32 HOLY_FLASH_HEAL = WoW120Spells::Priest::FLASH_HEAL;
constexpr uint32 HOLY_PRAYER_OF_HEALING = WoW120Spells::Priest::HolyPriest::PRAYER_OF_HEALING;
constexpr uint32 HOLY_RENEW = WoW120Spells::Priest::HolyPriest::RENEW;
constexpr uint32 HOLY_PRAYER_OF_MENDING = WoW120Spells::Priest::HolyPriest::PRAYER_OF_MENDING;
constexpr uint32 HOLY_CIRCLE_OF_HEALING = WoW120Spells::Priest::HolyPriest::CIRCLE_OF_HEALING;
constexpr uint32 HOLY_HOLY_WORD_SERENITY = WoW120Spells::Priest::HolyPriest::HOLY_WORD_SERENITY;
constexpr uint32 HOLY_HOLY_WORD_SANCTIFY = WoW120Spells::Priest::HolyPriest::HOLY_WORD_SANCTIFY;
constexpr uint32 HOLY_HOLY_WORD_SALVATION = WoW120Spells::Priest::HolyPriest::HOLY_WORD_SALVATION;
constexpr uint32 HOLY_DIVINE_HYMN = WoW120Spells::Priest::HolyPriest::DIVINE_HYMN;
constexpr uint32 HOLY_GUARDIAN_SPIRIT = WoW120Spells::Priest::HolyPriest::GUARDIAN_SPIRIT;
constexpr uint32 HOLY_APOTHEOSIS = WoW120Spells::Priest::HolyPriest::APOTHEOSIS;
constexpr uint32 HOLY_DIVINE_STAR = WoW120Spells::Priest::Discipline::DIVINE_STAR;
constexpr uint32 HOLY_HALO = WoW120Spells::Priest::Discipline::HALO;
constexpr uint32 HOLY_HOLY_FIRE = WoW120Spells::Priest::HolyPriest::HOLY_FIRE;
constexpr uint32 HOLY_SMITE = WoW120Spells::Priest::SMITE;
constexpr uint32 HOLY_SYMBOL_OF_HOPE = WoW120Spells::Priest::HolyPriest::SYMBOL_OF_HOPE;
constexpr uint32 HOLY_FADE = WoW120Spells::Priest::FADE;
constexpr uint32 HOLY_DESPERATE_PRAYER = WoW120Spells::Priest::DESPERATE_PRAYER;
constexpr uint32 HOLY_POWER_WORD_FORTITUDE = WoW120Spells::Priest::POWER_WORD_FORTITUDE;
constexpr uint32 HOLY_PURIFY = WoW120Spells::Priest::PURIFY;

// NEW: TrinityCore 12.0 Talent Spell IDs
// See central registry: WoW120Spells::Priest::HolyPriest
constexpr uint32 HOLY_EMPYREAL_BLAZE = 372616;         // -> WoW120Spells::Priest::HolyPriest::EMPYREAL_BLAZE
constexpr uint32 HOLY_EMPYREAL_BLAZE_AURA = 372617;    // -> WoW120Spells::Priest::HolyPriest::EMPYREAL_BLAZE_AURA
constexpr uint32 HOLY_POWER_SURGE = 453109;            // -> WoW120Spells::Priest::HolyPriest::POWER_SURGE
constexpr uint32 HOLY_POWER_SURGE_PERIODIC = 453112;   // -> WoW120Spells::Priest::HolyPriest::POWER_SURGE_PERIODIC

// Renew HoT tracker
class RenewTracker
{
public:
    RenewTracker() = default;

    void ApplyRenew(ObjectGuid guid, uint32 duration = 15000)
    {
        _renewTargets[guid] = GameTime::GetGameTimeMS() + duration;
    }

    void RemoveRenew(ObjectGuid guid)
    {
        _renewTargets.erase(guid);
    }

    [[nodiscard]] bool HasRenew(ObjectGuid guid) const
    {
        auto it = _renewTargets.find(guid);
        if (it == _renewTargets.end())

            return false;
        return GameTime::GetGameTimeMS() < it->second;
    }

    [[nodiscard]] uint32 GetRenewTimeRemaining(ObjectGuid guid) const
    {
        auto it = _renewTargets.find(guid);
        if (it == _renewTargets.end())

            return 0;

        uint32 now = GameTime::GetGameTimeMS();
        if (now >= it->second)

            return 0;

        return it->second - now;
    }

    [[nodiscard]] bool NeedsRenewRefresh(ObjectGuid guid, uint32 pandemicWindow = 4500) const
    {
        uint32 remaining = GetRenewTimeRemaining(guid);
        return remaining < pandemicWindow;
    }

    [[nodiscard]] uint32 GetActiveRenewCount() const
    {
        uint32 count = 0;
        uint32 now = GameTime::GetGameTimeMS();
        for (const auto& pair : _renewTargets)
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

        // Clean up expired Renews
        uint32 now = GameTime::GetGameTimeMS();
        for (auto it = _renewTargets.begin(); it != _renewTargets.end();)
        {

            if (now >= it->second)

                it = _renewTargets.erase(it);

            else

                ++it;
        }
    }

private:
    CooldownManager _cooldowns;
    ::std::unordered_map<ObjectGuid, uint32> _renewTargets; // GUID -> expiration time
};

// Prayer of Mending tracker (bouncing heal)
class PrayerOfMendingTracker
{
public:
    PrayerOfMendingTracker() = default;

    void ApplyPoM(ObjectGuid guid, uint32 duration = 30000)
    {
        _pomTargets[guid] = GameTime::GetGameTimeMS() + duration;
    }

    void RemovePoM(ObjectGuid guid)
    {
        _pomTargets.erase(guid);
    }

    [[nodiscard]] bool HasPoM(ObjectGuid guid) const
    {
        auto it = _pomTargets.find(guid);
        if (it == _pomTargets.end())

            return false;
        return GameTime::GetGameTimeMS() < it->second;
    }

    [[nodiscard]] bool HasActivePomOnAnyTarget() const
    {
        uint32 now = GameTime::GetGameTimeMS();
        for (const auto& pair : _pomTargets)
        {

            if (now < pair.second)

                return true;
        }
        return false;
    }

    void Update(Player* bot)
    {
        if (!bot)

            return;

        // Clean up expired PoMs
        uint32 now = GameTime::GetGameTimeMS();
        for (auto it = _pomTargets.begin(); it != _pomTargets.end();)
        {

            if (now >= it->second)

                it = _pomTargets.erase(it);

            else

                ++it;
        }
    }

private:
    ::std::unordered_map<ObjectGuid, uint32> _pomTargets; // GUID -> expiration time
};

class HolyPriestRefactored : public HealerSpecialization<ManaResource>
{
public:
    using Base = HealerSpecialization<ManaResource>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::_resource;
    explicit HolyPriestRefactored(Player* bot)
        : HealerSpecialization<ManaResource>(bot)
        , _renewTracker()
        , _pomTracker()
        , _talentState(bot)  // NEW: TrinityCore 12.0 talent state
        , _apotheosisActive(false)
        , _apotheosisEndTime(0)
        , _lastApotheosisTime(0)
        , _lastGuardianSpiritTime(0)
        , _lastDivineHymnTime(0)
        , _lastSalvationTime(0)
        , _lastSymbolOfHopeTime(0)
        , _lastHaloTime(0)      // NEW: Track Halo CD for Power Surge
        , _lastHolyFireTime(0)  // NEW: Track Holy Fire for Empyreal Blaze
        , _cooldowns()
    {
        // Register cooldowns for major abilities
        // COMMENTED OUT:         _cooldowns.RegisterBatch({
        // COMMENTED OUT: 
        // COMMENTED OUT:             {HOLY_DIVINE_HYMN, 180000, 1},
        // COMMENTED OUT: 
        // COMMENTED OUT:             {HOLY_HOLY_WORD_SALVATION, 720000, 1},
        // COMMENTED OUT: 
        // COMMENTED OUT:             {HOLY_APOTHEOSIS, 120000, 1},
        // COMMENTED OUT: 
        // COMMENTED OUT:             {HOLY_GUARDIAN_SPIRIT, 180000, 1},
        // COMMENTED OUT: 
        // COMMENTED OUT:             {HOLY_SYMBOL_OF_HOPE, 300000, 1}
        // COMMENTED OUT:         });

        // Initialize Phase 5 systems
        InitializeHolyMechanics();

        // Register healing spell efficiency tiers
        GetEfficiencyManager().RegisterSpell(HOLY_HEAL, HealingSpellTier::VERY_HIGH, "Heal");
        GetEfficiencyManager().RegisterSpell(HOLY_RENEW, HealingSpellTier::VERY_HIGH, "Renew");
        GetEfficiencyManager().RegisterSpell(HOLY_PRAYER_OF_MENDING, HealingSpellTier::VERY_HIGH, "Prayer of Mending");
        GetEfficiencyManager().RegisterSpell(HOLY_FLASH_HEAL, HealingSpellTier::HIGH, "Flash Heal");
        GetEfficiencyManager().RegisterSpell(HOLY_CIRCLE_OF_HEALING, HealingSpellTier::HIGH, "Circle of Healing");
        GetEfficiencyManager().RegisterSpell(HOLY_HOLY_WORD_SERENITY, HealingSpellTier::MEDIUM, "Holy Word: Serenity");
        GetEfficiencyManager().RegisterSpell(HOLY_PRAYER_OF_HEALING, HealingSpellTier::MEDIUM, "Prayer of Healing");
        GetEfficiencyManager().RegisterSpell(HOLY_HOLY_WORD_SANCTIFY, HealingSpellTier::LOW, "Holy Word: Sanctify");
        GetEfficiencyManager().RegisterSpell(HOLY_DIVINE_STAR, HealingSpellTier::MEDIUM, "Divine Star");
        GetEfficiencyManager().RegisterSpell(HOLY_HALO, HealingSpellTier::LOW, "Halo");
        GetEfficiencyManager().RegisterSpell(HOLY_GUARDIAN_SPIRIT, HealingSpellTier::EMERGENCY, "Guardian Spirit");
        GetEfficiencyManager().RegisterSpell(HOLY_DIVINE_HYMN, HealingSpellTier::EMERGENCY, "Divine Hymn");
        GetEfficiencyManager().RegisterSpell(HOLY_HOLY_WORD_SALVATION, HealingSpellTier::EMERGENCY, "Holy Word: Salvation");
        GetEfficiencyManager().RegisterSpell(HOLY_DESPERATE_PRAYER, HealingSpellTier::EMERGENCY, "Desperate Prayer");

        TC_LOG_DEBUG("playerbot", "HolyPriestRefactored initialized for bot {}", bot->GetGUID().GetCounter());
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
        if (_heroTalents.IsTree(HeroTalentTree::ORACLE))
        {
            // Oracle: Premonition for predictive group healing
            if (this->CanCastSpell(WoW120Spells::Priest::HolyPriest::POWER_SURGE, this->GetBot()))
            {
                this->CastSpell(WoW120Spells::Priest::HolyPriest::POWER_SURGE, this->GetBot());
                return;
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::ARCHON))
        {
            // Archon: Divine Halo for radiant burst healing
            if (this->CanCastSpell(WoW120Spells::Priest::HolyPriest::DIVINE_HALO, this->GetBot()))
            {
                this->CastSpell(WoW120Spells::Priest::HolyPriest::DIVINE_HALO, this->GetBot());
                return;
            }
        }

        UpdateHolyState();

        // Holy is a healer - check group health first
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

        // Deal damage when no healing is needed
        ExecuteDamageRotation(target);
    }

    void UpdateBuffs() override
    {
        Player* bot = this->GetBot();
        if (!bot)

            return;

        // Power Word: Fortitude (group buff)
        if (!bot->HasAura(HOLY_POWER_WORD_FORTITUDE))
        {

            if (this->CanCastSpell(HOLY_POWER_WORD_FORTITUDE, bot))

            {

                this->CastSpell(HOLY_POWER_WORD_FORTITUDE, bot);

            }
        }
    }

    void UpdateDefensives()
    {
        Player* bot = this->GetBot();
        if (!bot)

            return;

        float healthPct = bot->GetHealthPct();

        // Desperate Prayer (self-heal + damage reduction)
        if (healthPct < 30.0f && this->CanCastSpell(HOLY_DESPERATE_PRAYER, bot))
        {

            this->CastSpell(HOLY_DESPERATE_PRAYER, bot);

            return;
        }

        // Guardian Spirit (self - cheat death)
        if (healthPct < 20.0f && (GameTime::GetGameTimeMS() - _lastGuardianSpiritTime) >= 120000)
        {

            if (this->CanCastSpell(HOLY_GUARDIAN_SPIRIT, bot))

            {

                this->CastSpell(HOLY_GUARDIAN_SPIRIT, bot);

                _lastGuardianSpiritTime = GameTime::GetGameTimeMS();

                return;

            }
        }

        // Fade (threat reduction)
        if (healthPct < 50.0f && bot->GetThreatManager().GetThreatListSize() > 0)
        {

            if (this->CanCastSpell(HOLY_FADE, bot))

            {

                this->CastSpell(HOLY_FADE, bot);

                return;

            }
        }
    }

private:
    

    void UpdateHolyState()
    {
        Player* bot = this->GetBot();
        if (!bot)

            return;

        _renewTracker.Update(bot);
        _pomTracker.Update(bot);
        _talentState.Update();  // NEW: Update talent state
        UpdateCooldownStates();
    }

    // ========================================================================
    // NEW: TrinityCore 12.0 TALENT-BASED ENHANCEMENTS
    // ========================================================================

    /**
     * Handle Empyreal Blaze talent for damage rotation
     *
     * When we have the Empyreal Blaze buff (from casting Holy Fire),
     * the next Holy Fire becomes instant cast and triggers AoE healing.
     */
    bool HandleEmpyrealBlazePriority(::Unit* target)
    {
        Player* bot = this->GetBot();
        if (!bot || !target)
            return false;

        if (!_talentState.talents.HasEmpyrealBlaze())
            return false;

        // Check if we have the Empyreal Blaze buff (instant Holy Fire ready)
        if (bot->HasAura(HOLY_EMPYREAL_BLAZE_AURA))
        {
            // High priority instant Holy Fire
            if (this->CanCastSpell(HOLY_HOLY_FIRE, target))
            {
                this->CastSpell(HOLY_HOLY_FIRE, target);
                _lastHolyFireTime = GameTime::GetGameTimeMS();
                _talentState.empyrealBlaze.OnBlazeConsumed();
                TC_LOG_DEBUG("module.playerbot.holy",
                    "HolyPriest: {} cast instant Holy Fire (Empyreal Blaze)",
                    bot->GetName());
                return true;
            }
        }

        return false;
    }

    /**
     * Handle Power Surge talent optimization
     *
     * After casting Halo, we get periodic healing/damage ticks.
     * We should time Halo usage for maximum group benefit.
     */
    bool HandlePowerSurgePriority(const ::std::vector<Unit*>& group)
    {
        Player* bot = this->GetBot();
        if (!bot)
            return false;

        if (!_talentState.talents.HasPowerSurge())
            return false;

        if (!bot->HasSpell(HOLY_HALO))
            return false;

        // Check cooldown (40 sec)
        if ((GameTime::GetGameTimeMS() - _lastHaloTime) < 40000)
            return false;

        // Count injured group members
        uint32 injuredCount = 0;
        for (Unit* member : group)
        {
            if (member && member->GetHealthPct() < 85.0f)
                ++injuredCount;
        }

        // Use Halo with Power Surge when 3+ injured for maximum value
        if (injuredCount >= 3)
        {
            if (this->CanCastSpell(HOLY_HALO, bot))
            {
                this->CastSpell(HOLY_HALO, bot);
                _lastHaloTime = GameTime::GetGameTimeMS();
                _talentState.powerSurge.OnHaloCast(true); // Holy version
                TC_LOG_DEBUG("module.playerbot.holy",
                    "HolyPriest: {} cast Halo with Power Surge ({} injured)",
                    bot->GetName(), injuredCount);
                return true;
            }
        }

        return false;
    }

    /**
     * Enhanced Holy Fire usage for Empyreal Blaze procs
     */
    bool CastHolyFireWithBlaze(::Unit* target)
    {
        Player* bot = this->GetBot();
        if (!bot || !target)
            return false;

        if (this->CanCastSpell(HOLY_HOLY_FIRE, target))
        {
            this->CastSpell(HOLY_HOLY_FIRE, target);
            _lastHolyFireTime = GameTime::GetGameTimeMS();

            // Track for Empyreal Blaze
            if (_talentState.talents.HasEmpyrealBlaze())
            {
                _talentState.empyrealBlaze.OnHolyFireCast();
                // The buff proc is handled by TrinityCore's spell scripts
            }
            return true;
        }

        return false;
    }    void UpdateCooldownStates()
    {
        Player* bot = this->GetBot();
        if (!bot)

            return;

        // Apotheosis state (massive healing cooldown)
        if (_apotheosisActive && GameTime::GetGameTimeMS() >= _apotheosisEndTime)

            _apotheosisActive = false;

        if (bot->HasAura(HOLY_APOTHEOSIS))
        {

            _apotheosisActive = true;

            if (Aura* aura = bot->GetAura(HOLY_APOTHEOSIS))

                _apotheosisEndTime = GameTime::GetGameTimeMS() + aura->GetDuration();
                }
    }

    bool HandleGroupHealing(const ::std::vector<Unit*>& group)
    {
        // Emergency cooldowns
        if (HandleEmergencyCooldowns(group))

            return true;

        // Maintain HoTs
        if (HandleHoTs(group))

            return true;

        // Holy Words (core spells)
        if (HandleHolyWords(group))

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
        // Holy Word: Salvation (massive AoE heal)
        uint32 criticalHealthCount = 0;
        for (Unit* member : group)
        {

            if (member && member->GetHealthPct() < 40.0f)

                ++criticalHealthCount;
        }

        if (criticalHealthCount >= 3 && (GameTime::GetGameTimeMS() - _lastSalvationTime) >= 720000) // 12 min CD

        {
        if (bot->HasSpell(HOLY_HOLY_WORD_SALVATION))

            {

                if (this->CanCastSpell(HOLY_HOLY_WORD_SALVATION, bot))

                {

                    this->CastSpell(HOLY_HOLY_WORD_SALVATION, bot);

                    _lastSalvationTime = GameTime::GetGameTimeMS();

                    return true;
                    }

            }
        }

        // Divine Hymn (channeled raid healing)
        uint32 lowHealthCount = 0;
        for (Unit* member : group)
        {

            if (member && member->GetHealthPct() < 60.0f)

                ++lowHealthCount;
        }

        if (lowHealthCount >= 4 && (GameTime::GetGameTimeMS() - _lastDivineHymnTime) >= 180000) // 3 min CD
        {

            if (this->CanCastSpell(HOLY_DIVINE_HYMN, bot))

            {

                this->CastSpell(HOLY_DIVINE_HYMN, bot);

                _lastDivineHymnTime = GameTime::GetGameTimeMS();

                return true;

            }
        }

        // Apotheosis (healing burst mode)
        if (lowHealthCount >= 3 && (GameTime::GetGameTimeMS() - _lastApotheosisTime) >= 120000) // 2 min CD

        {
        if (bot->HasSpell(HOLY_APOTHEOSIS))

            {

                if (this->CanCastSpell(HOLY_APOTHEOSIS, bot))

                {

                    this->CastSpell(HOLY_APOTHEOSIS, bot);
                    _apotheosisActive = true;

                    _apotheosisEndTime = GameTime::GetGameTimeMS() + 20000; // 20 sec

                    _lastApotheosisTime = GameTime::GetGameTimeMS();
                    return true;

                }

            }
        }

        // Guardian Spirit (tank save)
        for (Unit* member : group)
        {

            if (member && member->GetHealthPct() < 25.0f && IsTankRole(member))

            {

                if ((GameTime::GetGameTimeMS() - _lastGuardianSpiritTime) >= 120000) // 2 min CD

                {

                    if (this->CanCastSpell(HOLY_GUARDIAN_SPIRIT, member))

                    {

                        this->CastSpell(HOLY_GUARDIAN_SPIRIT, member);

                        _lastGuardianSpiritTime = GameTime::GetGameTimeMS();

                        return true;

                    }

                }

            }
        }

        // Symbol of Hope (mana emergency for group)
        Player* bot = this->GetBot();
        if (!bot)

            return false;

        uint32 manaPercent = bot->GetPower(POWER_MANA) * 100 / bot->GetMaxPower(POWER_MANA);
        if (manaPercent < 20 && (GameTime::GetGameTimeMS() - _lastSymbolOfHopeTime) >= 180000) // 3 min CD
        {

            if (bot->HasSpell(HOLY_SYMBOL_OF_HOPE))

            {
            if (this->CanCastSpell(HOLY_SYMBOL_OF_HOPE, bot))

                {

                    this->CastSpell(HOLY_SYMBOL_OF_HOPE, bot);

                    _lastSymbolOfHopeTime = GameTime::GetGameTimeMS();

                    return true;
                    }

            }
        }

        return false;
    }

    bool HandleHoTs(const ::std::vector<Unit*>& group)
    {
        uint32 activeRenews = _renewTracker.GetActiveRenewCount();

        // Prayer of Mending (bouncing heal)
        if (!_pomTracker.HasActivePomOnAnyTarget())
        {

            for (Unit* member : group)
            {

                if (member && member->GetHealthPct() < 95.0f)

                {

                    if (this->CanCastSpell(HOLY_PRAYER_OF_MENDING, member))

                    {

                        this->CastSpell(HOLY_PRAYER_OF_MENDING, member);

                        _pomTracker.ApplyPoM(member->GetGUID(), 30000);

                        return true;

                    }

                }

            }
        }

        // Renew on injured allies
        if (activeRenews < group.size())
        {

            for (Unit* member : group)
            {

                if (member && member->GetHealthPct() < 90.0f)

                {

                    if (_renewTracker.NeedsRenewRefresh(member->GetGUID()))

                    {

                        if (this->CanCastSpell(HOLY_RENEW, member))

                        {

                            this->CastSpell(HOLY_RENEW, member);

                            _renewTracker.ApplyRenew(member->GetGUID(), 15000);

                            return true;

                        }

                    }

                }

            }
        }

        return false;
    }

    bool HandleHolyWords(const ::std::vector<Unit*>& group)
    {
        // Holy Word: Serenity (big single-target heal)
        for (Unit* member : group)
        {

            if (member && member->GetHealthPct() < 50.0f)

            {

                if (this->CanCastSpell(HOLY_HOLY_WORD_SERENITY, member))
                {

                    this->CastSpell(HOLY_HOLY_WORD_SERENITY, member);

                    return true;

                }

            }
        }

        // Holy Word: Sanctify (AoE ground heal)
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

            if (this->CanCastSpell(HOLY_HOLY_WORD_SANCTIFY, stackedTarget))

            {
            this->CastSpell(HOLY_HOLY_WORD_SANCTIFY, stackedTarget);

                return true;

            }
        }

        return false;
    }

    bool HandleAoEHealing(const ::std::vector<Unit*>& group)
    {
        // NEW: Power Surge Halo priority
        if (HandlePowerSurgePriority(group))
            return true;

        // Circle of Healing (instant AoE)
        uint32 injuredCount = 0;
        for (Unit* member : group)
        {

            if (member && member->GetHealthPct() < 85.0f)

                ++injuredCount;
        }

        if (injuredCount >= 3)

        {
        if (bot->HasSpell(HOLY_CIRCLE_OF_HEALING))

            {

                for (Unit* member : group)

                {

                    if (member && member->GetHealthPct() < 85.0f)

                    {

                        if (this->CanCastSpell(HOLY_CIRCLE_OF_HEALING, member))

                        {

                            this->CastSpell(HOLY_CIRCLE_OF_HEALING, member);

                            return true;

                        }

                    }

                }

            }
        }

        // Prayer of Healing (group heal)
        if (injuredCount >= 3)
        {

            for (Unit* member : group)

            {

                if (member && member->GetHealthPct() < 80.0f)

                {

                    if (this->CanCastSpell(HOLY_PRAYER_OF_HEALING, member))

                    {

                        this->CastSpell(HOLY_PRAYER_OF_HEALING, member);

                        return true;

                    }

                }

            }
        }

        // Divine Star (damage + healing)
        if (injuredCount >= 2)

        {
        if (bot->HasSpell(HOLY_DIVINE_STAR))

            {

                if (this->CanCastSpell(HOLY_DIVINE_STAR, bot))

                {

                    this->CastSpell(HOLY_DIVINE_STAR, bot);

                    return true;

                }

            }
        }

        // Halo (large AoE damage + healing)
        if (injuredCount >= 4)

        {
        if (bot->HasSpell(HOLY_HALO))

            {

                if (this->CanCastSpell(HOLY_HALO, bot))

                {

                    this->CastSpell(HOLY_HALO, bot);

                    return true;

                }

            }
        }

        return false;
    }

    bool HandleDirectHealing(const ::std::vector<Unit*>& group)
    {
        // Flash Heal for emergency (gated by mana efficiency tier)
        for (Unit* member : group)
        {

            if (member && member->GetHealthPct() < 60.0f)

            {

                if (IsHealAllowedByMana(HOLY_FLASH_HEAL) && this->CanCastSpell(HOLY_FLASH_HEAL, member))

                {

                    this->CastSpell(HOLY_FLASH_HEAL, member);

                    return true;

                }

            }
        }

        // Heal (efficient single-target - always allowed, VERY_HIGH tier)
        for (Unit* member : group)
        {

            if (member && member->GetHealthPct() < 80.0f)

            {

                if (this->CanCastSpell(HOLY_HEAL, member))

                {

                    this->CastSpell(HOLY_HEAL, member);

                    return true;

                }

            }
        }        return false;
    }

    bool HandleSelfHealing()
    {
        Player* bot = this->GetBot();
        if (!bot)

            return false;

        // Renew
        if (_renewTracker.NeedsRenewRefresh(bot->GetGUID()))
        {

            if (this->CanCastSpell(HOLY_RENEW, bot))

            {

                this->CastSpell(HOLY_RENEW, bot);

                _renewTracker.ApplyRenew(bot->GetGUID(), 15000);

                return true;

            }
        }

        // Flash Heal
        if (bot->GetHealthPct() < 60.0f)
        {

            if (this->CanCastSpell(HOLY_FLASH_HEAL, bot))

            {

                this->CastSpell(HOLY_FLASH_HEAL, bot);

                return true;

            }
        }

        // Heal
        if (bot->GetHealthPct() < 80.0f)
        {

            if (this->CanCastSpell(HOLY_HEAL, bot))

            {

                this->CastSpell(HOLY_HEAL, bot);

                return true;

            }
        }

        return false;
    }

    void ExecuteDamageRotation(::Unit* target)
    {
        // NEW: Priority 1 - Empyreal Blaze instant Holy Fire
        if (HandleEmpyrealBlazePriority(target))
            return;

        // Holy Fire (DoT + damage + Empyreal Blaze proc)
        if (CastHolyFireWithBlaze(target))
            return;

        // Smite (filler)
        if (this->CanCastSpell(HOLY_SMITE, target))
        {

            this->CastSpell(HOLY_SMITE, target);

            return;
        }
    }

    [[nodiscard]] bool IsTankRole(Unit* unit) const
    {
        if (!unit)

            return false;

        if (Player* player = unit->ToPlayer())
        {
            // Simplified tank detection - check if player is currently tanking
            // A more robust implementation would check spec, but talent API is deprecated

            if (Unit* victim = player->GetVictim())

                return victim->GetVictim() == player;
                return false;
        }

        return false;
    }

    void InitializeHolyMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)
        BotAI* ai = this;
        if (!ai)

            return;

        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            // EMERGENCY

            queue->RegisterSpell(HOLY_GUARDIAN_SPIRIT, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);

            queue->AddCondition(HOLY_GUARDIAN_SPIRIT, [this](Player* bot, Unit* target) {

                return target && target->GetHealthPct() < 25.0f && this->IsTankRole(target);

            }, "Tank < 25%");


            queue->RegisterSpell(HOLY_DESPERATE_PRAYER, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);

            queue->AddCondition(HOLY_DESPERATE_PRAYER, [](Player* bot, Unit*) {

                return bot->GetHealthPct() < 30.0f;

            }, "Self < 30%");


            queue->RegisterSpell(HOLY_HOLY_WORD_SALVATION, SpellPriority::EMERGENCY, SpellCategory::HEALING);

            // CRITICAL

            queue->RegisterSpell(HOLY_HOLY_WORD_SERENITY, SpellPriority::CRITICAL, SpellCategory::HEALING);

            queue->AddCondition(HOLY_HOLY_WORD_SERENITY, [](Player* bot, Unit* target) {

                return target && target->GetHealthPct() < 50.0f;

            }, "Target < 50%");


            queue->RegisterSpell(HOLY_FLASH_HEAL, SpellPriority::CRITICAL, SpellCategory::HEALING);

            queue->AddCondition(HOLY_FLASH_HEAL, [](Player* bot, Unit* target) {

                return target && target->GetHealthPct() < 60.0f;

            }, "Target < 60%");


            queue->RegisterSpell(HOLY_DIVINE_HYMN, SpellPriority::CRITICAL, SpellCategory::HEALING);

            // HIGH

            queue->RegisterSpell(HOLY_HOLY_WORD_SANCTIFY, SpellPriority::HIGH, SpellCategory::HEALING);

            queue->RegisterSpell(HOLY_PRAYER_OF_MENDING, SpellPriority::HIGH, SpellCategory::HEALING);

            queue->RegisterSpell(HOLY_RENEW, SpellPriority::HIGH, SpellCategory::HEALING);

            queue->RegisterSpell(HOLY_APOTHEOSIS, SpellPriority::HIGH, SpellCategory::OFFENSIVE);

            // MEDIUM

            queue->RegisterSpell(HOLY_CIRCLE_OF_HEALING, SpellPriority::MEDIUM, SpellCategory::HEALING);

            queue->RegisterSpell(HOLY_PRAYER_OF_HEALING, SpellPriority::MEDIUM, SpellCategory::HEALING);

            queue->RegisterSpell(HOLY_HEAL, SpellPriority::MEDIUM, SpellCategory::HEALING);

            queue->RegisterSpell(HOLY_DIVINE_STAR, SpellPriority::MEDIUM, SpellCategory::HEALING);

            queue->RegisterSpell(HOLY_HALO, SpellPriority::MEDIUM, SpellCategory::HEALING);

            // LOW

            queue->RegisterSpell(HOLY_HOLY_FIRE, SpellPriority::LOW, SpellCategory::DAMAGE_SINGLE);

            queue->RegisterSpell(HOLY_SMITE, SpellPriority::LOW, SpellCategory::DAMAGE_SINGLE);

            queue->RegisterSpell(HOLY_PURIFY, SpellPriority::LOW, SpellCategory::UTILITY);


            TC_LOG_INFO("module.playerbot", " HOLY PRIEST: Registered {} spells", queue->GetSpellCount());
        }

        auto* behaviorTree = ai->GetBehaviorTree();
        if (behaviorTree)
        {

            auto root = Selector("Holy Priest Healer", {

                Sequence("Emergency", {

                    Condition("Critical", [](Player* bot, Unit*) {

                        if (bot->GetHealthPct() < 30.0f) return true;

                        Group* g = bot->GetGroup();

                        if (!g) return false;

                        for (GroupReference const& r : g->GetMembers())

                            if (Player* m = r.GetSource())

                                if (m->IsAlive() && m->GetHealthPct() < 30.0f) return true;

                        return false;

                    }),

                    Selector("Response", {

                        bot::ai::Action("Guardian Spirit", [this](Player* bot, Unit*) {

                            Group* g = bot->GetGroup();

                            if (!g) return NodeStatus::FAILURE;

                            for (GroupReference const& r : g->GetMembers())

                                if (Player* m = r.GetSource())

                                    if (m->IsAlive() && m->GetHealthPct() < 25.0f && this->IsTankRole(m) &&

                                        this->CanCastSpell(HOLY_GUARDIAN_SPIRIT, m))
                                        {

                                        this->CastSpell(HOLY_GUARDIAN_SPIRIT, m);

                                        return NodeStatus::SUCCESS;

                                    }

                            return NodeStatus::FAILURE;

                        }),

                        bot::ai::Action("Flash Heal", [this](Player* bot, Unit*) {

                            Group* g = bot->GetGroup();

                            if (!g) return NodeStatus::FAILURE;

                            for (GroupReference const& r : g->GetMembers())

                                if (Player* m = r.GetSource())

                                    if (m->IsAlive() && m->GetHealthPct() < 50.0f &&

                                        this->CanCastSpell(HOLY_FLASH_HEAL, m))
                                        {

                                        this->CastSpell(HOLY_FLASH_HEAL, m);

                                        return NodeStatus::SUCCESS;

                                    }

                            return NodeStatus::FAILURE;

                        })

                    })

                }),


                Sequence("HoT Maintenance", {

                    Condition("Has group", [](Player* bot, Unit*) { return bot->GetGroup() != nullptr; }),

                    Selector("HoT Priority", {

                        bot::ai::Action("Prayer of Mending", [this](Player* bot, Unit*) {

                            if (!this->_pomTracker.HasActivePomOnAnyTarget())
                            {

                                Group* g = bot->GetGroup();

                                if (!g) return NodeStatus::FAILURE;

                                for (GroupReference const& r : g->GetMembers())

                                    if (Player* m = r.GetSource())

                                        if (m->IsAlive() && m->GetHealthPct() < 95.0f &&

                                            this->CanCastSpell(HOLY_PRAYER_OF_MENDING, m))
                                            {

                                            this->CastSpell(HOLY_PRAYER_OF_MENDING, m);

                                            this->_pomTracker.ApplyPoM(m->GetGUID(), 30000);

                                            return NodeStatus::SUCCESS;

                                        }

                            }

                            return NodeStatus::FAILURE;

                        }),

                        bot::ai::Action("Renew", [this](Player* bot, Unit*) {

                            Group* g = bot->GetGroup();

                            if (!g) return NodeStatus::FAILURE;

                            for (GroupReference const& r : g->GetMembers())

                                if (Player* m = r.GetSource())

                                    if (m->IsAlive() && m->GetHealthPct() < 90.0f &&

                                        this->_renewTracker.NeedsRenewRefresh(m->GetGUID()) &&

                                        this->CanCastSpell(HOLY_RENEW, m))
                                        {

                                        this->CastSpell(HOLY_RENEW, m);

                                        this->_renewTracker.ApplyRenew(m->GetGUID(), 15000);

                                        return NodeStatus::SUCCESS;

                                    }

                            return NodeStatus::FAILURE;

                        })

                    })

                }),


                Sequence("Direct Healing", {

                    bot::ai::Action("Heal", [this](Player* bot, Unit*) {

                        Group* g = bot->GetGroup();

                        if (!g) return NodeStatus::FAILURE;

                        for (GroupReference const& r : g->GetMembers())

                            if (Player* m = r.GetSource())

                                if (m->IsAlive() && m->GetHealthPct() < 80.0f &&

                                    this->CanCastSpell(HOLY_HEAL, m))
                                    {

                                    this->CastSpell(HOLY_HEAL, m);

                                    return NodeStatus::SUCCESS;

                                }

                        return NodeStatus::FAILURE;

                    })

                })

            });


            behaviorTree->SetRoot(root);

            TC_LOG_INFO("module.playerbot", " HOLY PRIEST: BehaviorTree initialized");
        }
    }

    // Member variables
    RenewTracker _renewTracker;
    PrayerOfMendingTracker _pomTracker;

    // NEW: TrinityCore 12.0 Talent State
    PriestTalentState _talentState;

    bool _apotheosisActive;
    uint32 _apotheosisEndTime;

    uint32 _lastApotheosisTime;
    uint32 _lastGuardianSpiritTime;
    uint32 _lastDivineHymnTime;
    uint32 _lastSalvationTime;
    uint32 _lastSymbolOfHopeTime;

    // NEW: TrinityCore 12.0 Cooldown Tracking
    uint32 _lastHaloTime;
    uint32 _lastHolyFireTime;

    CooldownManager _cooldowns;

    // Hero talent detection cache (refreshed on combat start)
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot

#endif // PLAYERBOT_HOLYPRIESTREFACTORED_H
