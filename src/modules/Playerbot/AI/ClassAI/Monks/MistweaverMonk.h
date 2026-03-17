/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Mistweaver Monk Refactored - Template-Based Implementation
 *
 * This file provides a complete, template-based implementation of Mistweaver Monk
 * using the HealerSpecialization with Mana resource system.
 */

#pragma once


#include "../Common/StatusEffectTracker.h"
#include "../Common/CooldownManager.h"
#include "../Common/RotationHelpers.h"
#include "../CombatSpecializationTemplates.h"
#include "../ResourceTypes.h"
#include "../SpellValidation_WoW120.h"
#include "../HeroTalentDetector.h"
#include "Player.h"
#include "SpellMgr.h"
#include "SpellAuraEffects.h"
#include "Group.h"
#include "Log.h"
#include "../../Services/HealingTargetSelector.h"  // Phase 5B: Unified healing service
#include "../../Decision/ActionPriorityQueue.h"
#include "../../Decision/BehaviorTree.h"
#include "../BotAI.h"

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
// ============================================================================
// MISTWEAVER MONK SPELL IDs (WoW 12.0 - The War Within)
// Using centralized spell registry from SpellValidation_WoW120.h
// ============================================================================

namespace MistweaverMonkSpells
{
    // Import from central registry - Monk common spells
    using namespace WoW120Spells::Monk;

    // Direct Heals
    constexpr uint32 VIVIFY                  = Mistweaver::VIVIFY;
    constexpr uint32 SOOTHING_MIST           = Mistweaver::SOOTHING_MIST;
    constexpr uint32 ENVELOPING_MIST         = Mistweaver::ENVELOPING_MIST;
    constexpr uint32 EXPEL_HARM_MIST         = EXPEL_HARM;
    constexpr uint32 LIFE_COCOON             = Mistweaver::LIFE_COCOON;

    // HoT Management
    constexpr uint32 RENEWING_MIST           = Mistweaver::RENEWING_MIST;
    constexpr uint32 ESSENCE_FONT            = Mistweaver::ESSENCE_FONT;
    constexpr uint32 REVIVAL                 = Mistweaver::REVIVAL;

    // AoE Healing
    constexpr uint32 REFRESHING_JADE_WIND    = Mistweaver::REFRESHING_JADE_WIND;
    constexpr uint32 CHI_BURST_MIST          = Mistweaver::CHI_BURST;
    constexpr uint32 CHI_WAVE_MIST           = Mistweaver::CHI_WAVE;

    // Cooldowns
    constexpr uint32 INVOKE_YULON            = Mistweaver::INVOKE_YULON;
    constexpr uint32 INVOKE_CHI_JI           = Mistweaver::INVOKE_CHI_JI;
    constexpr uint32 INVOKE_SHEILUN          = Mistweaver::INVOKE_SHEILUN;
    constexpr uint32 SHEILUNS_GIFT           = Mistweaver::SHEILUNS_GIFT;

    // Utility
    constexpr uint32 THUNDER_FOCUS_TEA       = Mistweaver::THUNDER_FOCUS_TEA;
    constexpr uint32 MANA_TEA                = Mistweaver::MANA_TEA;
    constexpr uint32 FORTIFYING_BREW_MIST    = FORTIFYING_BREW;
    constexpr uint32 DIFFUSE_MAGIC_MIST      = DIFFUSE_MAGIC;
    constexpr uint32 DETOX_MIST              = DETOX;
    constexpr uint32 PARALYSIS_MIST          = PARALYSIS;

    // DPS Abilities (Fistweaving)
    constexpr uint32 RISING_SUN_KICK_MIST    = Mistweaver::RISING_SUN_KICK;
    constexpr uint32 BLACKOUT_KICK_MIST      = Mistweaver::BLACKOUT_KICK_MW;
    constexpr uint32 TIGER_PALM_MIST         = Mistweaver::TIGER_PALM_MW;
    constexpr uint32 SPINNING_CRANE_KICK_MIST = Mistweaver::SPINNING_CRANE_KICK_MW;

    // Procs and Buffs
    constexpr uint32 TEACHINGS_OF_THE_MONASTERY_MW = WoW120Spells::Monk::TEACHINGS_OF_THE_MONASTERY;
    constexpr uint32 ANCIENT_TEACHINGS       = Mistweaver::ANCIENT_TEACHINGS;
    constexpr uint32 FAELINE_STOMP           = Mistweaver::FAELINE_STOMP;

    // Talents
    constexpr uint32 UPWELLING               = Mistweaver::UPWELLING;
    constexpr uint32 LIFECYCLES              = Mistweaver::LIFECYCLES;
    constexpr uint32 SPIRIT_OF_THE_CRANE     = Mistweaver::SPIRIT_OF_THE_CRANE;
    constexpr uint32 CLOUDED_FOCUS           = Mistweaver::CLOUDED_FOCUS;
    constexpr uint32 RISING_MIST             = Mistweaver::RISING_MIST;
    constexpr uint32 SECRET_INFUSION         = Mistweaver::SECRET_INFUSION;

    // Hero Talents
    constexpr uint32 CELESTIAL_CONDUIT       = Mistweaver::CELESTIAL_CONDUIT;
    constexpr uint32 MW_ASPECT_OF_HARMONY    = Mistweaver::MW_ASPECT_OF_HARMONY;
}
using namespace MistweaverMonkSpells;

// ============================================================================
// MISTWEAVER RENEWING MIST TRACKER
// ============================================================================

class MistweaverReNewingMistTracker
{
public:
    MistweaverReNewingMistTracker()
        : _trackedTargets()
    {
    }

    void AddTarget(ObjectGuid guid)
    {
        _trackedTargets[guid] = GameTime::GetGameTimeMS() + 20000; // 20 sec duration
    }

    void RemoveTarget(ObjectGuid guid)
    {
        _trackedTargets.erase(guid);
    }

    bool HasReNewingMist(ObjectGuid guid) const
    {
        auto it = _trackedTargets.find(guid);
        if (it == _trackedTargets.end())

            return false;

        return GameTime::GetGameTimeMS() < it->second;
    }

    uint32 GetActiveCount() const
    {
        uint32 count = 0;
        uint32 now = GameTime::GetGameTimeMS();
        for (const auto& pair : _trackedTargets)
        {

            if (now < pair.second)

                ++count;
        }
        return count;
    }

    void Update()
    {
        uint32 now = GameTime::GetGameTimeMS();
        for (auto it = _trackedTargets.begin(); it != _trackedTargets.end();)
        {

            if (now >= it->second)

                it = _trackedTargets.erase(it);

            else

                ++it;
        }
    }

private:
    CooldownManager _cooldowns;
    ::std::unordered_map<ObjectGuid, uint32> _trackedTargets;
};

// ============================================================================
// MISTWEAVER SOOTHING MIST TRACKER
// ============================================================================

class MistweaverSoothingMistTracker
{
public:
    MistweaverSoothingMistTracker()
        : _currentTargetGuid()
        , _channelStartTime(0)
        , _isChanneling(false)
    {
    }

    void StartChannel(ObjectGuid guid)
    {
        _currentTargetGuid = guid;
        _channelStartTime = GameTime::GetGameTimeMS();
        _isChanneling = true;
    }

    void StopChannel()
    {
        _currentTargetGuid = ObjectGuid::Empty;
        _channelStartTime = 0;
        _isChanneling = false;
    }

    bool IsChanneling() const { return _isChanneling; }
    ObjectGuid GetTarget() const { return _currentTargetGuid; }

    bool CanInstantCast() const
    {
        // Soothing Mist enables instant Vivify/Enveloping Mist
        return _isChanneling && (GameTime::GetGameTimeMS() - _channelStartTime) > 500;
    }

    void Update(Player* bot)
    {
        if (!bot)

            return;

        // Check if still channeling
        if (_isChanneling)
        {

            if (!bot->HasUnitState(UNIT_STATE_CASTING) || !bot->HasAura(SOOTHING_MIST))

            {

                StopChannel();

            }
        }
    }

private:
    ObjectGuid _currentTargetGuid;
    uint32 _channelStartTime;
    bool _isChanneling;
};

// ============================================================================
// MISTWEAVER MONK REFACTORED
// ============================================================================

class MistweaverMonkRefactored : public HealerSpecialization<ManaResource>
{
public:
    using Base = HealerSpecialization<ManaResource>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::_resource;
    explicit MistweaverMonkRefactored(Player* bot)
        : HealerSpecialization<ManaResource>(bot)
        , _renewingMistTracker()
        , _soothingMistTracker()
        , _thunderFocusTeaActive(false)
        , _lastEssenceFontTime(0)
    {
        // Initialize mana resources
        this->_resource = bot->GetPower(POWER_MANA);

        // Phase 5: Initialize decision systems
        InitializeMistweaverMechanics();

        // Register healing spell efficiency tiers
        GetEfficiencyManager().RegisterSpell(VIVIFY, HealingSpellTier::VERY_HIGH, "Vivify");
        GetEfficiencyManager().RegisterSpell(RENEWING_MIST, HealingSpellTier::VERY_HIGH, "Renewing Mist");
        GetEfficiencyManager().RegisterSpell(ENVELOPING_MIST, HealingSpellTier::HIGH, "Enveloping Mist");
        GetEfficiencyManager().RegisterSpell(ESSENCE_FONT, HealingSpellTier::MEDIUM, "Essence Font");
        GetEfficiencyManager().RegisterSpell(LIFE_COCOON, HealingSpellTier::EMERGENCY, "Life Cocoon");
        GetEfficiencyManager().RegisterSpell(REVIVAL, HealingSpellTier::EMERGENCY, "Revival");
        GetEfficiencyManager().RegisterSpell(SOOTHING_MIST, HealingSpellTier::VERY_HIGH, "Soothing Mist");
        GetEfficiencyManager().RegisterSpell(THUNDER_FOCUS_TEA, HealingSpellTier::MEDIUM, "Thunder Focus Tea");

        TC_LOG_DEBUG("playerbot", "MistweaverMonkRefactored initialized for bot {}", bot->GetGUID().GetCounter());
    }

    void UpdateRotation(::Unit* target) override
    {
        // Mistweaver focuses on healing, not DPS rotation
        // Use UpdateBuffs() for healing logic
    }

    void UpdateBuffs() override
    {
        Player* bot = this->GetBot();
        if (!bot)

            return;

        // Lazy hero talent detection (once per bot lifetime until respec)
        if (!_heroTalents.detected)
            _heroTalents.Refresh(bot);

        // Update Mistweaver state
        UpdateMistweaverState();

        // Get group members
        ::std::vector<Unit*> group = GetGroupMembers();
        if (group.empty())

            return;

        // Handle healing rotation
        ExecuteHealingRotation(group);
    }

protected:
    void ExecuteHealingRotation(const ::std::vector<Unit*>& group)
    {
        // Hero talent tree rotation branching
        // Mistweaver has access to: Master of Harmony / Conduit of the Celestials
        if (_heroTalents.IsTree(HeroTalentTree::MASTER_OF_HARMONY))
        {
            // Master of Harmony: Aspect of Harmony accumulates vitality from healing done
            // Release accumulated vitality as burst healing when group needs it
            if (this->CanCastSpell(MW_ASPECT_OF_HARMONY, this->GetBot()))
            {
                // Use during heavy group damage or when Revival/Yulon are on cooldown
                uint32 lowHealthCount = 0;
                for (Unit* member : group)
                {
                    if (member->GetHealthPct() < 60.0f)
                        ++lowHealthCount;
                }
                if (lowHealthCount >= 2)
                {
                    this->CastSpell(MW_ASPECT_OF_HARMONY, this->GetBot());
                    return;
                }
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::CONDUIT_OF_THE_CELESTIALS))
        {
            // Conduit of the Celestials: Celestial Conduit channels celestial healing
            // Coordinate with major healing cooldowns for maximum throughput
            if (this->CanCastSpell(CELESTIAL_CONDUIT, this->GetBot()))
            {
                uint32 injuredCount = 0;
                for (Unit* member : group)
                {
                    if (member->GetHealthPct() < 70.0f)
                        ++injuredCount;
                }
                // Use when multiple group members are injured
                if (injuredCount >= 3)
                {
                    this->CastSpell(CELESTIAL_CONDUIT, this->GetBot());
                    return;
                }
            }
        }

        // Priority 1: Emergency healing
        if (HandleEmergencyHealing(group))

            return;

        // Priority 2: Thunder Focus Tea empowerment
        if (HandleThunderFocusTea(group))

            return;

        // Priority 3: Spread Renewing Mist
        if (HandleReNewingMist(group))

            return;

        // Priority 4: Essence Font for AoE healing
        if (HandleEssenceFont(group))

            return;

        // Priority 5: Single target healing
        if (HandleSingleTargetHealing(group))

            return;

        // Priority 6: Maintain Soothing Mist channel
        HandleSoothingMist(group);
    }

    bool HandleEmergencyHealing(const ::std::vector<Unit*>& group)
    {
        Player* bot = this->GetBot();

        // Critical: Life Cocoon
        for (Unit* member : group)
        {

            if (member->GetHealthPct() < 20.0f && this->CanCastSpell(LIFE_COCOON, member))

            {

                this->CastSpell(LIFE_COCOON, member);

                TC_LOG_DEBUG("playerbot", "Mistweaver: Life Cocoon on {}", member->GetName());

                return true;

            }
        }

        // Very low: Revival (raid-wide instant heal)
        uint32 lowHealthCount = 0;
        for (Unit* member : group)
        {

            if (member->GetHealthPct() < 40.0f)

                ++lowHealthCount;
        }

        if (lowHealthCount >= 3 && this->CanCastSpell(REVIVAL, bot))
        {

            this->CastSpell(REVIVAL, bot);
            

        // Register cooldowns using CooldownManager
        // COMMENTED OUT:         _cooldowns.RegisterBatch({
        // COMMENTED OUT: 
        // COMMENTED OUT:             {RENEWING_MIST, 8500, 1},
        // COMMENTED OUT: 
        // COMMENTED OUT:             {ESSENCE_FONT, 12000, 1},
        // COMMENTED OUT: 
        // COMMENTED OUT:             {LIFE_COCOON, CooldownPresets::MINOR_OFFENSIVE, 1},
        // COMMENTED OUT: 
        // COMMENTED OUT:             {REVIVAL, CooldownPresets::MAJOR_OFFENSIVE, 1},
        // COMMENTED OUT: 
        // COMMENTED OUT:             {INVOKE_YULON, CooldownPresets::MAJOR_OFFENSIVE, 1},
        // COMMENTED OUT: 
        // COMMENTED OUT:             {INVOKE_CHI_JI, CooldownPresets::MAJOR_OFFENSIVE, 1},
        // COMMENTED OUT: 
        // COMMENTED OUT:             {THUNDER_FOCUS_TEA, CooldownPresets::OFFENSIVE_30, 1},
        // COMMENTED OUT: 
        // COMMENTED OUT:             {FORTIFYING_BREW_MIST, 360000, 1},
        // COMMENTED OUT: 
        // COMMENTED OUT:             {DIFFUSE_MAGIC_MIST, 90000, 1},
        // COMMENTED OUT:         });

        TC_LOG_DEBUG("playerbot", "Mistweaver: Revival raid heal");

            return true;
        }

        // Low: Invoke Yu'lon
        if (lowHealthCount >= 2 && this->CanCastSpell(INVOKE_YULON, bot))
        {

            this->CastSpell(INVOKE_YULON, bot);

            TC_LOG_DEBUG("playerbot", "Mistweaver: Invoke Yu'lon");

            return true;
        }

        // Urgent single target heal
        for (Unit* member : group)
        {

            if (member->GetHealthPct() < 35.0f && this->CanCastSpell(VIVIFY, member))

            {

                this->CastSpell(VIVIFY, member);

                return true;

            }
        }

        return false;
    }

    bool HandleThunderFocusTea(const ::std::vector<Unit*>& group)
    {
        if (!_thunderFocusTeaActive)
        {
            // Use Thunder Focus Tea if we need emergency healing

            uint32 lowHealthCount = 0;

            for (Unit* member : group)

            {

                if (member->GetHealthPct() < 60.0f)

                    ++lowHealthCount;

            }


            if (lowHealthCount >= 2 && IsHealAllowedByMana(THUNDER_FOCUS_TEA) && this->CanCastSpell(THUNDER_FOCUS_TEA, this->GetBot()))

            {

                this->CastSpell(THUNDER_FOCUS_TEA, this->GetBot());

                _thunderFocusTeaActive = true;

                TC_LOG_DEBUG("playerbot", "Mistweaver: Thunder Focus Tea activated");

            }
        }

        // If active, use empowered spell
        if (_thunderFocusTeaActive)
        {
            // Empowered Vivify (free + cleave)

            for (Unit* member : group)

            {

                if (member->GetHealthPct() < 70.0f && this->CanCastSpell(VIVIFY, member))

                {

                    this->CastSpell(VIVIFY, member);

                    _thunderFocusTeaActive = false;

                    return true;

                }

            }
            // Empowered Renewing Mist (instant 2 charges)

            Unit* target = SelectHealingTarget(group);
            if (target && this->CanCastSpell(RENEWING_MIST, target))

            {

                this->CastSpell(RENEWING_MIST, target);

                _renewingMistTracker.AddTarget(target->GetGUID());

                _thunderFocusTeaActive = false;

                return true;

            }
        }

        return false;
    }

    bool HandleReNewingMist(const ::std::vector<Unit*>& group)
    {
        // Maintain Renewing Mist on targets
        uint32 activeCount = _renewingMistTracker.GetActiveCount();

        if (activeCount < 3) // Try to maintain on 3 targets
        {

            for (Unit* member : group)

            {

                if (member->GetHealthPct() < 95.0f && !_renewingMistTracker.HasReNewingMist(member->GetGUID()))

                {

                    if (this->CanCastSpell(RENEWING_MIST, member))

                    {

                        this->CastSpell(RENEWING_MIST, member);

                        _renewingMistTracker.AddTarget(member->GetGUID());

                        return true;

                    }

                }

            }
        }

        return false;
    }

    bool HandleEssenceFont(const ::std::vector<Unit*>& group)
    {
        // Use Essence Font for AoE healing
        uint32 injuredCount = 0;
        for (Unit* member : group)
        {

            if (member->GetHealthPct() < 80.0f)

                ++injuredCount;
        }

        if (injuredCount >= 3 && IsHealAllowedByMana(ESSENCE_FONT) && this->CanCastSpell(ESSENCE_FONT, this->GetBot()))
        {

            this->CastSpell(ESSENCE_FONT, this->GetBot());

            _lastEssenceFontTime = GameTime::GetGameTimeMS();

            return true;
        }

        return false;
    }

    bool HandleSingleTargetHealing(const ::std::vector<Unit*>& group)
    {
        Unit* target = SelectHealingTarget(group);
        if (!target)

            return false;

        float healthPct = target->GetHealthPct();

        // Priority 1: Enveloping Mist (strong single target HoT)
        if (healthPct < 70.0f && IsHealAllowedByMana(ENVELOPING_MIST) && this->CanCastSpell(ENVELOPING_MIST, target))
        {

            this->CastSpell(ENVELOPING_MIST, target);

            return true;
        }

        // Priority 2: Vivify (smart heal with cleave)
        if (healthPct < 80.0f && this->CanCastSpell(VIVIFY, target))
        {

            this->CastSpell(VIVIFY, target);

            return true;
        }

        return false;
    }

    bool HandleSoothingMist(const ::std::vector<Unit*>& group)
    {
        // If not channeling, start on lowest target
        if (!_soothingMistTracker.IsChanneling())
        {

            Unit* target = SelectHealingTarget(group);
            if (target && target->GetHealthPct() < 90.0f)

            {

                if (this->CanCastSpell(SOOTHING_MIST, target))

                {

                    this->CastSpell(SOOTHING_MIST, target);

                    _soothingMistTracker.StartChannel(target->GetGUID());

                    return true;

                }

            }
        }
        else
        {
            // Check if channel target still needs healing

            ObjectGuid targetGuid = _soothingMistTracker.GetTarget();

            Unit* target = ObjectAccessor::GetUnit(*this->GetBot(), targetGuid);


            if (!target || target->GetHealthPct() > 95.0f)

            {
                // Stop channel and find new target

                _soothingMistTracker.StopChannel();

                return false;

            }

            // Use instant Vivify/Enveloping Mist while channeling

            if (_soothingMistTracker.CanInstantCast())

            {

                if (target->GetHealthPct() < 70.0f && this->CanCastSpell(VIVIFY, target))

                {

                    this->CastSpell(VIVIFY, target);

                    return true;

                }

            }
        }

        return false;
    }

private:
    void UpdateMistweaverState()
    {
        // Update Renewing Mist tracker
        _renewingMistTracker.Update();

        // Update Soothing Mist tracker
        _soothingMistTracker.Update(this->GetBot());

        // Update Thunder Focus Tea status
        if (_thunderFocusTeaActive)
        {

            if (!this->GetBot()->HasAura(THUNDER_FOCUS_TEA))

                _thunderFocusTeaActive = false;
        }

        // Update mana from bot
        if (this->GetBot())
            this->_resource = this->GetBot()->GetPower(POWER_MANA);
    }

    Unit* SelectHealingTarget(const ::std::vector<Unit*>& group)
    {
        // Use unified HealingTargetSelector service (Phase 5B integration)
        // Eliminates 15+ lines of duplicated healing target logic
        return bot::ai::HealingTargetSelector::SelectTarget(this->GetBot(), 40.0f, 95.0f);
    }

    // ========================================================================
    // PHASE 5: DECISION SYSTEM INTEGRATION
    // ========================================================================

    void InitializeMistweaverMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)
        BotAI* ai = this;
        if (!ai) return;

        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            // EMERGENCY: Major healing cooldowns

            queue->RegisterSpell(REVIVAL, SpellPriority::EMERGENCY, SpellCategory::HEALING);

            queue->AddCondition(REVIVAL, [this](Player*, Unit*) {

                auto group = this->GetGroupMembers();

                uint32 low = 0;

                for (auto* m : group) if (m && m->GetHealthPct() < 50.0f) low++;

                return low >= 3;

            }, "3+ allies < 50% HP (instant raid heal, 3min CD)");


            queue->RegisterSpell(LIFE_COCOON, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);

            queue->AddCondition(LIFE_COCOON, [this](Player*, Unit*) {

                auto group = this->GetGroupMembers();

                for (auto* m : group) if (m && m->GetHealthPct() < 30.0f) return true;

                return false;

            }, "Ally < 30% HP (absorb shield, 2min CD)");

            // CRITICAL: Major healing spells

            queue->RegisterSpell(INVOKE_YULON, SpellPriority::CRITICAL, SpellCategory::HEALING);

            queue->AddCondition(INVOKE_YULON, [this](Player* bot, Unit*) {

                if (!bot->HasSpell(INVOKE_YULON)) return false;

                auto group = this->GetGroupMembers();

                uint32 injured = 0;

                for (auto* m : group) if (m && m->GetHealthPct() < 70.0f) injured++;

                return injured >= 3;

            }, "3+ allies < 70% HP (celestial, 3min CD)");


            queue->RegisterSpell(ESSENCE_FONT, SpellPriority::CRITICAL, SpellCategory::HEALING);

            queue->AddCondition(ESSENCE_FONT, [this](Player* bot, Unit*) {

                if (!bot || bot->GetPowerPct(POWER_MANA) < 10.0f) return false;

                auto group = this->GetGroupMembers();

                uint32 injured = 0;

                for (auto* m : group) if (m && m->GetHealthPct() < 85.0f) injured++;

                return injured >= 4;

            }, "4+ allies < 85% HP, 5% mana (AoE HoT, 12s CD)");

            // HIGH: Core HoT maintenance

            queue->RegisterSpell(RENEWING_MIST, SpellPriority::HIGH, SpellCategory::HEALING);

            queue->AddCondition(RENEWING_MIST, [this](Player*, Unit*) {

                uint32 active = this->_renewingMistTracker.GetActiveCount();

                auto group = this->GetGroupMembers();

                return active < group.size() && active < 3;

            }, "< 3 active (bouncing HoT, 8.5s CD, 2 charges)");


            queue->RegisterSpell(ENVELOPING_MIST, SpellPriority::HIGH, SpellCategory::HEALING);

            queue->AddCondition(ENVELOPING_MIST, [this](Player* bot, Unit*) {

                if (!bot || bot->GetPowerPct(POWER_MANA) < 10.0f) return false;

                auto group = this->GetGroupMembers();

                for (auto* m : group)
                {

                    if (m && m->GetHealthPct() < 65.0f && !m->HasAura(ENVELOPING_MIST))

                        return true;

                }

                return false;

            }, "Ally < 65% HP without HoT (6% mana)");


            queue->RegisterSpell(VIVIFY, SpellPriority::HIGH, SpellCategory::HEALING);

            queue->AddCondition(VIVIFY, [this](Player* bot, Unit*) {

                if (!bot || bot->GetPowerPct(POWER_MANA) < 10.0f) return false;

                auto group = this->GetGroupMembers();

                for (auto* m : group) if (m && m->GetHealthPct() < 75.0f) return true;

                return false;

            }, "Ally < 75% HP (smart cleave heal, 5% mana)");

            // MEDIUM: Soothing Mist channel

            queue->RegisterSpell(SOOTHING_MIST, SpellPriority::MEDIUM, SpellCategory::HEALING);

            queue->AddCondition(SOOTHING_MIST, [this](Player*, Unit*) {

                if (this->_soothingMistTracker.IsChanneling()) return false;

                auto group = this->GetGroupMembers();

                for (auto* m : group) if (m && m->GetHealthPct() < 80.0f) return true;

                return false;

            }, "Ally < 80% HP, not channeling (enables instant Vivify)");


            queue->RegisterSpell(THUNDER_FOCUS_TEA, SpellPriority::MEDIUM, SpellCategory::UTILITY);

            queue->AddCondition(THUNDER_FOCUS_TEA, [this](Player*, Unit*) {

                return !this->_thunderFocusTeaActive;

            }, "Not active (empower next spell, 30s CD)");

            // LOW: AoE healing

            queue->RegisterSpell(REFRESHING_JADE_WIND, SpellPriority::LOW, SpellCategory::HEALING);

            queue->AddCondition(REFRESHING_JADE_WIND, [this](Player* bot, Unit*) {

                if (!bot || !bot->HasSpell(REFRESHING_JADE_WIND) || bot->GetPowerPct(POWER_MANA) < 25.0f) return false;

                auto group = this->GetGroupMembers();

                uint32 stacked = 0;

                for (auto* m : group)

                    if (m && m->GetHealthPct() < 90.0f && m->GetDistance(bot) <= 10.0f)

                        stacked++;

                return stacked >= 3;

            }, "3+ stacked allies < 90% HP (AoE HoT, 25% mana)");


            queue->RegisterSpell(CHI_BURST_MIST, SpellPriority::LOW, SpellCategory::HEALING);

            queue->AddCondition(CHI_BURST_MIST, [this](Player* bot, Unit*) {

                if (!bot || !bot->HasSpell(CHI_BURST_MIST)) return false;

                auto group = this->GetGroupMembers();

                uint32 injured = 0;

                for (auto* m : group) if (m && m->GetHealthPct() < 85.0f) injured++;

                return injured >= 3;

            }, "3+ allies < 85% HP (AoE line heal, 30s CD)");

            // UTILITY: Defensive and mana

            queue->RegisterSpell(FORTIFYING_BREW_MIST, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);

            queue->AddCondition(FORTIFYING_BREW_MIST, [](Player* bot, Unit*) {

                return bot && bot->GetHealthPct() < 40.0f;

            }, "HP < 40% (20% DR, 6min CD)");


            queue->RegisterSpell(DIFFUSE_MAGIC_MIST, SpellPriority::HIGH, SpellCategory::DEFENSIVE);

            queue->AddCondition(DIFFUSE_MAGIC_MIST, [](Player* bot, Unit*) {

                return bot && bot->GetHealthPct() < 50.0f;

            }, "HP < 50% (magic immunity, 1.5min CD)");


            queue->RegisterSpell(MANA_TEA, SpellPriority::LOW, SpellCategory::UTILITY);

            queue->AddCondition(MANA_TEA, [](Player* bot, Unit*) {

                return bot && bot->HasSpell(MANA_TEA) && bot->GetPowerPct(POWER_MANA) < 50.0f;

            }, "Mana < 50% (channel regen)");


            queue->RegisterSpell(DETOX_MIST, SpellPriority::MEDIUM, SpellCategory::UTILITY);

            queue->AddCondition(DETOX_MIST, [this](Player*, Unit*) {

                auto group = this->GetGroupMembers();

                for (auto* m : group)
                {

                    if (m && (m->HasAuraType(SPELL_AURA_PERIODIC_DAMAGE) ||

                               m->HasAuraType(SPELL_AURA_MOD_DECREASE_SPEED)))

                        return true;

                }

                return false;

            }, "Ally has poison/disease (dispel)");
        }

        auto* behaviorTree = ai->GetBehaviorTree();
        if (behaviorTree)
        {

            auto root = Selector("Mistweaver Monk Healing", {
                // Tier 1: Emergency Healing

                Sequence("Emergency Healing", {

                    Condition("3+ critical", [this](Player*, Unit*) {

                        auto group = this->GetGroupMembers();

                        uint32 low = 0;

                        for (auto* m : group) if (m && m->GetHealthPct() < 50.0f) low++;

                        return low >= 3;

                    }),

                    Selector("Use emergency", {

                        Sequence("Revival", {

                            bot::ai::Action("Cast Revival", [this](Player* bot, Unit*) {

                                if (this->CanCastSpell(REVIVAL, bot))
                                {

                                    this->CastSpell(REVIVAL, bot);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),

                        Sequence("Life Cocoon", {

                            Condition("Ally < 30%", [this](Player*, Unit*) {

                                auto group = this->GetGroupMembers();

                                for (auto* m : group) if (m && m->GetHealthPct() < 30.0f) return true;

                                return false;

                            }),

                            bot::ai::Action("Cast Life Cocoon", [this](Player*, Unit*) {

                                auto group = this->GetGroupMembers();

                                for (auto* m : group)
                                {

                                    if (m && m->GetHealthPct() < 30.0f && this->CanCastSpell(LIFE_COCOON, m))
                                    {

                                        this->CastSpell(LIFE_COCOON, m);

                                        return NodeStatus::SUCCESS;

                                    }

                                }

                                return NodeStatus::FAILURE;

                            })

                        })

                    })

                }),

                // Tier 2: Major Cooldowns

                Sequence("Major Cooldowns", {

                    Condition("3+ injured", [this](Player*, Unit*) {

                        auto group = this->GetGroupMembers();

                        uint32 injured = 0;

                        for (auto* m : group) if (m && m->GetHealthPct() < 70.0f) injured++;

                        return injured >= 3;

                    }),

                    Selector("Use cooldowns", {

                        Sequence("Invoke Yu'lon", {

                            Condition("Has spell", [this](Player* bot, Unit*) {

                                return bot->HasSpell(INVOKE_YULON);

                            }),

                            bot::ai::Action("Cast Yu'lon", [this](Player* bot, Unit*) {

                                if (this->CanCastSpell(INVOKE_YULON, bot))
                                {

                                    this->CastSpell(INVOKE_YULON, bot);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),

                        Sequence("Essence Font", {

                            Condition("4+ injured", [this](Player*, Unit*) {

                                auto group = this->GetGroupMembers();

                                uint32 injured = 0;

                                for (auto* m : group) if (m && m->GetHealthPct() < 85.0f) injured++;

                                return injured >= 4;

                            }),

                            Condition("Has mana", [this](Player* bot, Unit*) {

                                return bot && bot->GetPowerPct(POWER_MANA) >= 10.0f;

                            }),

                            bot::ai::Action("Cast Essence Font", [this](Player* bot, Unit* target) {

                                Unit* healTarget = this->SelectHealingTarget(this->GetGroupMembers());

                                if (healTarget && this->CanCastSpell(ESSENCE_FONT, healTarget))
                                {

                                    this->CastSpell(ESSENCE_FONT, healTarget);

                                    this->_lastEssenceFontTime = GameTime::GetGameTimeMS();

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        })

                    })

                }),

                // Tier 3: HoT Maintenance

                Sequence("Maintain HoTs", {

                    Selector("Apply HoTs", {

                        Sequence("Renewing Mist", {

                            Condition("< 3 active", [this](Player*, Unit*) {

                                uint32 active = this->_renewingMistTracker.GetActiveCount();

                                auto group = this->GetGroupMembers();

                                return active < group.size() && active < 3;

                            }),

                            bot::ai::Action("Cast Renewing Mist", [this](Player*, Unit*) {

                                Unit* target = this->SelectHealingTarget(this->GetGroupMembers());

                                if (target && this->CanCastSpell(RENEWING_MIST, target))
                                {

                                    this->CastSpell(RENEWING_MIST, target);

                                    this->_renewingMistTracker.AddTarget(target->GetGUID());

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),

                        Sequence("Enveloping Mist", {

                            Condition("Has mana", [this](Player* bot, Unit*) {

                                return bot && bot->GetPowerPct(POWER_MANA) >= 10.0f;

                            }),

                            bot::ai::Action("Cast Enveloping Mist", [this](Player*, Unit*) {

                                auto group = this->GetGroupMembers();

                                for (auto* m : group)
                                {

                                    if (m && m->GetHealthPct() < 65.0f && !m->HasAura(ENVELOPING_MIST))
                                    {

                                        if (this->CanCastSpell(ENVELOPING_MIST, m))
                                        {

                                            this->CastSpell(ENVELOPING_MIST, m);

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

                    Condition("Has mana", [this](Player* bot, Unit*) {

                        return bot && bot->GetPowerPct(POWER_MANA) >= 10.0f;

                    }),

                    Selector("Cast heals", {

                        Sequence("Vivify", {

                            Condition("Ally < 75%", [this](Player*, Unit*) {

                                auto group = this->GetGroupMembers();

                                for (auto* m : group) if (m && m->GetHealthPct() < 75.0f) return true;

                                return false;

                            }),

                            bot::ai::Action("Cast Vivify", [this](Player*, Unit*) {

                                Unit* target = this->SelectHealingTarget(this->GetGroupMembers());

                                if (target && this->CanCastSpell(VIVIFY, target))
                                {

                                    this->CastSpell(VIVIFY, target);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),

                        Sequence("Soothing Mist", {

                            Condition("Not channeling", [this](Player*, Unit*) {

                                return !this->_soothingMistTracker.IsChanneling();

                            }),

                            bot::ai::Action("Cast Soothing Mist", [this](Player*, Unit*) {

                                Unit* target = this->SelectHealingTarget(this->GetGroupMembers());

                                if (target && this->CanCastSpell(SOOTHING_MIST, target))
                                {

                                    this->CastSpell(SOOTHING_MIST, target);

                                    this->_soothingMistTracker.StartChannel(target->GetGUID());

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

private:
    MistweaverReNewingMistTracker _renewingMistTracker;
    MistweaverSoothingMistTracker _soothingMistTracker;
    bool _thunderFocusTeaActive;
    uint32 _lastEssenceFontTime;
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot
