/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Balance Druid Refactored - Template-Based Implementation
 *
 * This file provides a complete, template-based implementation of Balance Druid
 * using the RangedDpsSpecialization with dual resource system (Mana + Astral Power).
 */

#pragma once


#include "../Common/StatusEffectTracker.h"
#include "../Common/CooldownManager.h"
#include "../Common/RotationHelpers.h"
#include "../CombatSpecializationTemplates.h"
#include "../ResourceTypes.h"
#include "Player.h"
#include "SpellMgr.h"
#include "SpellAuraEffects.h"
#include "Log.h"
// Phase 5 Integration: Decision Systems
#include "../../Decision/ActionPriorityQueue.h"
#include "../../Decision/BehaviorTree.h"
#include "../BotAI.h"
#include "GameTime.h"

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
// ============================================================================
// BALANCE DRUID SPELL IDs (WoW 12.0 - The War Within)
// See central registry: WoW120Spells::Druid and WoW120Spells::Druid::Balance
// ============================================================================

enum BalanceDruidSpells
{
    // Astral Power Generators
    WRATH                    = WoW120Spells::Druid::Balance::WRATH,
    STARFIRE                 = WoW120Spells::Druid::Balance::STARFIRE,
    STELLAR_FLARE            = WoW120Spells::Druid::Balance::STELLAR_FLARE,
    FORCE_OF_NATURE          = WoW120Spells::Druid::Balance::FORCE_OF_NATURE,

    // Astral Power Spenders
    STARSURGE                = WoW120Spells::Druid::Balance::STARSURGE,
    STARFALL                 = WoW120Spells::Druid::Balance::STARFALL,
    NEW_MOON                 = WoW120Spells::Druid::Balance::NEW_MOON,
    HALF_MOON                = WoW120Spells::Druid::Balance::HALF_MOON,
    FULL_MOON                = WoW120Spells::Druid::Balance::FULL_MOON,

    // DoTs
    MOONFIRE                 = WoW120Spells::Druid::Balance::MOONFIRE_BALANCE,
    SUNFIRE                  = WoW120Spells::Druid::Balance::SUNFIRE_BALANCE,

    // Major Cooldowns
    INCARNATION_CHOSEN       = WoW120Spells::Druid::Balance::INCARNATION_CHOSEN,
    CELESTIAL_ALIGNMENT      = WoW120Spells::Druid::Balance::CELESTIAL_ALIGNMENT,
    WARRIOR_OF_ELUNE         = WoW120Spells::Druid::Balance::WARRIOR_OF_ELUNE,
    FURY_OF_ELUNE            = WoW120Spells::Druid::Balance::FURY_OF_ELUNE,
    CONVOKE_THE_SPIRITS      = WoW120Spells::Druid::Balance::CONVOKE_THE_SPIRITS,

    // Utility
    MOONKIN_FORM             = WoW120Spells::Druid::MOONKIN_FORM,
    SOLAR_BEAM               = WoW120Spells::Druid::SOLAR_BEAM,
    TYPHOON                  = WoW120Spells::Druid::TYPHOON,
    MIGHTY_BASH              = WoW120Spells::Druid::MIGHTY_BASH,
    MASS_ENTANGLEMENT        = WoW120Spells::Druid::MASS_ENTANGLEMENT,
    REMOVE_CORRUPTION        = WoW120Spells::Druid::REMOVE_CORRUPTION,
    SOOTHE                   = WoW120Spells::Druid::SOOTHE,
    INNERVATE                = WoW120Spells::Druid::INNERVATE,

    // Defensives
    BARKSKIN                 = WoW120Spells::Druid::BARKSKIN,
    RENEWAL                  = WoW120Spells::Druid::RENEWAL,
    REGROWTH                 = WoW120Spells::Druid::REGROWTH,
    BEAR_FORM                = WoW120Spells::Druid::BEAR_FORM,
    FRENZIED_REGENERATION    = WoW120Spells::Druid::Guardian::FRENZIED_REGENERATION,

    // Eclipse System
    ECLIPSE_SOLAR            = WoW120Spells::Druid::Balance::SOLAR_ECLIPSE,
    ECLIPSE_LUNAR            = WoW120Spells::Druid::Balance::LUNAR_ECLIPSE,
    BALANCE_OF_ALL_THINGS    = WoW120Spells::Druid::Balance::BALANCE_OF_ALL_THINGS,

    // Procs and Buffs
    SHOOTING_STARS           = WoW120Spells::Druid::Balance::SHOOTING_STARS,
    STARWEAVERS_WARP         = WoW120Spells::Druid::Balance::STARWEAVERS_WARP,
    STARWEAVERS_WEFT         = WoW120Spells::Druid::Balance::STARWEAVERS_WEFT,

    // Talents
    WILD_MUSHROOM            = WoW120Spells::Druid::Balance::WILD_MUSHROOM_GROUND,
    TWIN_MOONS               = WoW120Spells::Druid::Balance::TWIN_MOONS,
    SOUL_OF_THE_FOREST       = WoW120Spells::Druid::Balance::SOUL_OF_THE_FOREST
};

// Dual resource type for Balance Druid (Mana + Astral Power)
struct ManaAstralPowerResource
{
    uint32 mana{0};
    uint32 astralPower{0};
    uint32 maxMana{100000};
    uint32 maxAstralPower{100};

    bool available{true};
    bool Consume(uint32 manaCost)
    {
        if (mana >= manaCost) {
            mana -= manaCost;
            return true;
        }
        return false;
    }
    void Regenerate(uint32 diff)
    {
        // Resource regeneration logic (simplified)
        available = true;
    }

    [[nodiscard]] uint32 GetAvailable() const {
        return 100; // Simplified - override in specific implementations
    }

    [[nodiscard]] uint32 GetMax() const {
        return 100; // Simplified - override in specific implementations
    }


    void Initialize(Player* bot)
    {
        // CRITICAL: Do NOT call bot->GetMaxPower() or bot->GetPower() here!
        // This is called from constructor before bot is fully in world.
        // Mana values will be fetched lazily in GetCurrent()/GetMax() or
        // on first Update() when bot->IsInWorld() returns true.
        astralPower = 0;
        _initialized = false;
    }

    void DeferredInitialize(Player* bot)
    {
        if (bot && bot->IsInWorld() && !_initialized)
        {
            maxMana = bot->GetMaxPower(POWER_MANA);
            mana = bot->GetPower(POWER_MANA);
            _initialized = true;
        }
    }

private:
    bool _initialized = false;
};

// ============================================================================
// BALANCE ECLIPSE TRACKER
// ============================================================================

class BalanceEclipseTracker
{
public:
    enum EclipseState
    {
        NONE,
        SOLAR,
        LUNAR
    };

    BalanceEclipseTracker()
        : _currentEclipse(NONE)
        , _eclipseEndTime(0)
    {
    }

    void EnterSolarEclipse()
    {
        _currentEclipse = SOLAR;
        _eclipseEndTime = GameTime::GetGameTimeMS() + 15000; // 15 sec duration
    }

    void EnterLunarEclipse()
    {
        _currentEclipse = LUNAR;
        _eclipseEndTime = GameTime::GetGameTimeMS() + 15000;
    }

    EclipseState GetCurrentEclipse() const { return _currentEclipse; }
    bool IsInEclipse() const { return _currentEclipse != NONE; }
    bool IsInSolarEclipse() const { return _currentEclipse == SOLAR; }
    bool IsInLunarEclipse() const { return _currentEclipse == LUNAR; }

    void Update(Player* bot)
    {
        if (!bot)
            return;

        uint32 now = GameTime::GetGameTimeMS();

        // Check eclipse buffs
        if (bot->HasAura(ECLIPSE_SOLAR))
        {
            _currentEclipse = SOLAR;
        }
        else if (bot->HasAura(ECLIPSE_LUNAR))
        {
            _currentEclipse = LUNAR;
        }
        else if (_currentEclipse != NONE && now >= _eclipseEndTime)
        {
            _currentEclipse = NONE;
            _eclipseEndTime = 0;
        }
    }

private:
    CooldownManager _cooldowns;
    EclipseState _currentEclipse;
    uint32 _eclipseEndTime;
};

// ============================================================================
// BALANCE DOT TRACKER
// ============================================================================

class BalanceDoTTracker
{
public:
    BalanceDoTTracker()
        : _trackedDoTs()
    {
    }

    void ApplyDoT(ObjectGuid guid, uint32 spellId, uint32 duration)
    {
        _trackedDoTs[guid][spellId] = GameTime::GetGameTimeMS() + duration;
    }

    bool HasDoT(ObjectGuid guid, uint32 spellId) const
    {
        auto it = _trackedDoTs.find(guid);
        if (it == _trackedDoTs.end())
            return false;

        auto dotIt = it->second.find(spellId);
        if (dotIt == it->second.end())
            return false;

        return GameTime::GetGameTimeMS() < dotIt->second;
    }

    uint32 GetTimeRemaining(ObjectGuid guid, uint32 spellId) const
    {
        auto it = _trackedDoTs.find(guid);
        if (it == _trackedDoTs.end())
            return 0;

        auto dotIt = it->second.find(spellId);
        if (dotIt == it->second.end())
            return 0;

        uint32 now = GameTime::GetGameTimeMS();
        return (dotIt->second > now) ? (dotIt->second - now) : 0;
    }

    bool NeedsRefresh(ObjectGuid guid, uint32 spellId, uint32 pandemicWindow = 5400) const
    {
        return GetTimeRemaining(guid, spellId) < pandemicWindow;
    }

    void Update()
    {
        uint32 now = GameTime::GetGameTimeMS();
        for (auto targetIt = _trackedDoTs.begin(); targetIt != _trackedDoTs.end();)
        {
            for (auto dotIt = targetIt->second.begin(); dotIt != targetIt->second.end();)
            {
                if (now >= dotIt->second)
                    dotIt = targetIt->second.erase(dotIt);
                else
                    ++dotIt;
            }

            if (targetIt->second.empty())
                targetIt = _trackedDoTs.erase(targetIt);
            else
                ++targetIt;
        }
    }

private:
    ::std::unordered_map<ObjectGuid, ::std::unordered_map<uint32, uint32>> _trackedDoTs;
};

// ============================================================================
// BALANCE DRUID REFACTORED
// ============================================================================

class BalanceDruidRefactored : public RangedDpsSpecialization<ManaAstralPowerResource>
{
public:
    using Base = RangedDpsSpecialization<ManaAstralPowerResource>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::_resource;
    explicit BalanceDruidRefactored(Player* bot)        : RangedDpsSpecialization<ManaAstralPowerResource>(bot)

        , _eclipseTracker()
        , _dotTracker()
        , _starfallActive(false)
        , _starfallEndTime(0)
        , _shootingStarsProc(false)
    {
        // CRITICAL: Do NOT call bot->GetMaxPower(), bot->GetPower(), or bot->GetName() here!
        // Bot is not fully in world during constructor. Resource initialization deferred
        // to first UpdateRotation() call via DeferredInitialize().
        this->_resource.Initialize(bot);

        // Phase 5: Initialize decision systems
        InitializeBalanceMechanics();
    }

    void UpdateRotation(::Unit* target) override
    {
        if (!target || !target->IsAlive() || !target->IsHostileTo(this->GetBot()))
            return;

        // Detect hero talents if not yet cached
        if (!_heroTalents.detected)
            _heroTalents.Refresh(this->GetBot());

        // Hero talent rotation branching
        // Balance Druid has access to: Keeper of the Grove / Elune's Chosen
        if (_heroTalents.IsTree(HeroTalentTree::KEEPER_OF_THE_GROVE))
        {
            // Keeper of the Grove: Power of the Dream enhances treants, Control of the Dream
            if (this->CanCastSpell(WoW120Spells::Druid::Balance::TREANTS_OF_THE_MOON, target))
            {
                this->CastSpell(WoW120Spells::Druid::Balance::TREANTS_OF_THE_MOON, target);
                return;
            }
            if (this->CanCastSpell(WoW120Spells::Druid::Balance::POWER_OF_THE_DREAM, this->GetBot()))
            {
                this->CastSpell(WoW120Spells::Druid::Balance::POWER_OF_THE_DREAM, this->GetBot());
                return;
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::ELUNES_CHOSEN))
        {
            // Elune's Chosen: Lunar Calling empowers Starfire, Astral Insight procs
            if (this->GetBot()->HasAura(WoW120Spells::Druid::Balance::ASTRAL_INSIGHT))
            {
                // Astral Insight proc active - prioritize Starsurge for empowered damage
                if (this->CanCastSpell(WoW120Spells::Druid::Balance::STARSURGE, target))
                {
                    this->CastSpell(WoW120Spells::Druid::Balance::STARSURGE, target);
                    return;
                }
            }
        }

        // Deferred initialization of mana resources (safe now that bot is in world)
        this->_resource.DeferredInitialize(this->GetBot());

        // Update Balance state
        UpdateBalanceState(target);

        // Ensure Moonkin Form
        EnsureMoonkinForm();

        // Use major cooldowns
        HandleCooldowns();

        // Determine if AoE or single target
        uint32 enemyCount = this->GetEnemiesInRange(40.0f);
        if (enemyCount >= 3)
        {
            ExecuteAoERotation(target, enemyCount);
        }
        else
        {
            ExecuteSingleTargetRotation(target);
        }
    }    void UpdateBuffs() override
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        // Defensive cooldowns
        HandleDefensiveCooldowns();
    }

    // Note: GetOptimalRange is final in RangedDpsSpecialization base class (defaults to 30-40 yards)

protected:
    void ExecuteSingleTargetRotation(::Unit* target)
    {
        ObjectGuid targetGuid = target->GetGUID();
        uint32 ap = this->_resource.astralPower;

        // Priority 1: Use Shooting Stars proc (free Starsurge)
        if (_shootingStarsProc && this->CanCastSpell(STARSURGE, target))
        {
            this->CastSpell(STARSURGE, target);
            _shootingStarsProc = false;
            return;
        }

        // Priority 2: Maintain Moonfire
        if (_dotTracker.NeedsRefresh(targetGuid, MOONFIRE))
        {
            if (this->CanCastSpell(MOONFIRE, target))
            {
                this->CastSpell(MOONFIRE, target);
                _dotTracker.ApplyDoT(targetGuid, MOONFIRE, 22000); // 22 sec duration
                return;
            }
        }

        // Priority 3: Maintain Sunfire
        if (_dotTracker.NeedsRefresh(targetGuid, SUNFIRE))
        {
            if (this->CanCastSpell(SUNFIRE, target))
            {
                this->CastSpell(SUNFIRE, target);
                _dotTracker.ApplyDoT(targetGuid, SUNFIRE, 18000); // 18 sec duration
                return;
            }
        }

        // Priority 4: Maintain Stellar Flare (talent)
        if (_dotTracker.NeedsRefresh(targetGuid, STELLAR_FLARE))
        {
            if (this->CanCastSpell(STELLAR_FLARE, target))
            {
                this->CastSpell(STELLAR_FLARE, target);
                _dotTracker.ApplyDoT(targetGuid, STELLAR_FLARE, 24000);
                return;
            }
        }

        // Priority 5: Starsurge (spend AP)
        if (ap >= 30 && this->CanCastSpell(STARSURGE, target))
        {
            this->CastSpell(STARSURGE, target);
            ConsumeAstralPower(30);
            return;
        }

        // Priority 6: Starfire (Lunar Eclipse or default)
        if (_eclipseTracker.IsInLunarEclipse() || !_eclipseTracker.IsInEclipse())
        {
            if (this->CanCastSpell(STARFIRE, target))
            {
                this->CastSpell(STARFIRE, target);
                GenerateAstralPower(8);

                if (!_eclipseTracker.IsInEclipse())
                    _eclipseTracker.EnterLunarEclipse();

                return;
            }
        }

        // Priority 7: Wrath (Solar Eclipse)
        if (_eclipseTracker.IsInSolarEclipse())
        {
            if (this->CanCastSpell(WRATH, target))
            {
                this->CastSpell(WRATH, target);
                GenerateAstralPower(6);
                return;
            }
        }

        // Priority 8: Wrath filler
        if (this->CanCastSpell(WRATH, target))
        {
            this->CastSpell(WRATH, target);
            GenerateAstralPower(6);

            if (!_eclipseTracker.IsInEclipse())
                _eclipseTracker.EnterSolarEclipse();
        }
    }

    void ExecuteAoERotation(::Unit* target, uint32 enemyCount)
    {
        uint32 ap = this->_resource.astralPower;

        // Priority 1: Starfall (AoE AP spender)
        if (ap >= 50 && !_starfallActive && this->CanCastSpell(STARFALL, this->GetBot()))
        {
            this->CastSpell(STARFALL, this->GetBot());
            _starfallActive = true;
            _starfallEndTime = GameTime::GetGameTimeMS() + 8000;
            ConsumeAstralPower(50);
            return;
        }

        // Priority 2: Sunfire (AoE DoT)
        if (this->CanCastSpell(SUNFIRE, target))
        {
            this->CastSpell(SUNFIRE, target);
            return;
        }

        // Priority 3: Moonfire (AoE DoT with Twin Moons)
        if (this->CanCastSpell(MOONFIRE, target))
        {
            this->CastSpell(MOONFIRE, target);
            return;
        }

        // Priority 4: Fury of Elune (talent)
        if (this->CanCastSpell(FURY_OF_ELUNE, this->GetBot()))
        {
            this->CastSpell(FURY_OF_ELUNE, this->GetBot());
            return;
        }

        // Priority 5: Starsurge
        if (ap >= 30 && this->CanCastSpell(STARSURGE, target))
        {
            this->CastSpell(STARSURGE, target);
            ConsumeAstralPower(30);
            return;
        }

        // Priority 6: Starfire filler
        if (this->CanCastSpell(STARFIRE, target))
        {
            this->CastSpell(STARFIRE, target);
            GenerateAstralPower(8);
            return;
        }
    }

    void HandleCooldowns()
    {
        Player* bot = this->GetBot();
        uint32 ap = this->_resource.astralPower;

        // Incarnation/Celestial Alignment (major burst)
        if (ap >= 40 && this->CanCastSpell(INCARNATION_CHOSEN, bot))
        {
            this->CastSpell(INCARNATION_CHOSEN, bot);
            

        // Register cooldowns using CooldownManager
        // COMMENTED OUT:         _cooldowns.RegisterBatch({
        // COMMENTED OUT:             {WRATH, 0, 1},
        // COMMENTED OUT:             {STARFIRE, 0, 1},
        // COMMENTED OUT:             {STARSURGE, 0, 1},
        // COMMENTED OUT:             {STARFALL, 0, 1},
        // COMMENTED OUT:             {MOONFIRE, 0, 1},
        // COMMENTED OUT:             {SUNFIRE, 0, 1},
        // COMMENTED OUT:             {INCARNATION_CHOSEN, CooldownPresets::MAJOR_OFFENSIVE, 1},
        // COMMENTED OUT:             {CELESTIAL_ALIGNMENT, CooldownPresets::MAJOR_OFFENSIVE, 1},
        // COMMENTED OUT:             {CONVOKE_THE_SPIRITS, CooldownPresets::MINOR_OFFENSIVE, 1},
        // COMMENTED OUT:             {WARRIOR_OF_ELUNE, CooldownPresets::OFFENSIVE_45, 1},
        // COMMENTED OUT:             {FURY_OF_ELUNE, CooldownPresets::OFFENSIVE_60, 1},
        // COMMENTED OUT:             {BARKSKIN, CooldownPresets::OFFENSIVE_60, 1},
        // COMMENTED OUT:             {RENEWAL, 90000, 1},
        // COMMENTED OUT:             {SOLAR_BEAM, CooldownPresets::OFFENSIVE_60, 1},
        // COMMENTED OUT:         });

        TC_LOG_DEBUG("playerbot", "Balance: Incarnation activated");
        }
        else if (ap >= 40 && this->CanCastSpell(CELESTIAL_ALIGNMENT, bot))
        {
            this->CastSpell(CELESTIAL_ALIGNMENT, bot);
            TC_LOG_DEBUG("playerbot", "Balance: Celestial Alignment");
        }

        // Convoke the Spirits
        if (this->CanCastSpell(CONVOKE_THE_SPIRITS, bot))
        {
            this->CastSpell(CONVOKE_THE_SPIRITS, bot);
            TC_LOG_DEBUG("playerbot", "Balance: Convoke the Spirits");
        }

        // Warrior of Elune
        if (this->CanCastSpell(WARRIOR_OF_ELUNE, bot))
        {
            this->CastSpell(WARRIOR_OF_ELUNE, bot);
        }
    }

    void HandleDefensiveCooldowns()
    {
        Player* bot = this->GetBot();
        float healthPct = bot->GetHealthPct();

        // Barkskin
        if (healthPct < 50.0f && this->CanCastSpell(BARKSKIN, bot))
        {
            this->CastSpell(BARKSKIN, bot);
            TC_LOG_DEBUG("playerbot", "Balance: Barkskin");
            return;
        }

        // Renewal
        if (healthPct < 40.0f && this->CanCastSpell(RENEWAL, bot))
        {
            this->CastSpell(RENEWAL, bot);
            TC_LOG_DEBUG("playerbot", "Balance: Renewal");
            return;
        }

        // Regrowth
        if (healthPct < 60.0f && this->CanCastSpell(REGROWTH, bot))
        {
            this->CastSpell(REGROWTH, bot);
            return;
        }
    }

    void EnsureMoonkinForm()
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        if (!bot->HasAura(MOONKIN_FORM) && this->CanCastSpell(MOONKIN_FORM, bot))
        {
            this->CastSpell(MOONKIN_FORM, bot);
            TC_LOG_DEBUG("playerbot", "Balance: Moonkin Form activated");
        }
    }

private:
    void UpdateBalanceState(::Unit* target)
    {
        // Update Eclipse tracker
        _eclipseTracker.Update(this->GetBot());

        // Update DoT tracker
        _dotTracker.Update();

        // Update Starfall
        if (_starfallActive && GameTime::GetGameTimeMS() >= _starfallEndTime)
        {
            _starfallActive = false;
            _starfallEndTime = 0;
        }

        // Update Shooting Stars proc
        if (this->GetBot() && this->GetBot()->HasAura(SHOOTING_STARS))
            _shootingStarsProc = true;
        else
            _shootingStarsProc = false;

        // Update Astral Power from bot
        if (this->GetBot())
        {
            this->_resource.astralPower = this->GetBot()->GetPower(POWER_LUNAR_POWER); // Astral Power uses LUNAR_POWER
            this->_resource.mana = this->GetBot()->GetPower(POWER_MANA);
        }
    }

    void GenerateAstralPower(uint32 amount)
    {
        this->_resource.astralPower = ::std::min(this->_resource.astralPower + amount, this->_resource.maxAstralPower);
    }

    void ConsumeAstralPower(uint32 amount)
    {
        this->_resource.astralPower = (this->_resource.astralPower > amount) ? this->_resource.astralPower - amount : 0;
    }

    void InitializeBalanceMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)
        BotAI* ai = this;
        if (!ai) return;

        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            // EMERGENCY: Defensive cooldowns
            queue->RegisterSpell(BARKSKIN, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
            queue->AddCondition(BARKSKIN, [this](Player* bot, Unit*) {
                return bot && bot->GetHealthPct() < 50.0f;
            }, "HP < 50% (damage reduction)");

            // CRITICAL: Major burst cooldowns
            queue->RegisterSpell(INCARNATION_CHOSEN, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(INCARNATION_CHOSEN, [this](Player* bot, Unit*) {
                return bot && bot->HasSpell(INCARNATION_CHOSEN) && this->_resource.astralPower >= 40;
            }, "40+ AP (major burst, 3min CD)");

            queue->RegisterSpell(CELESTIAL_ALIGNMENT, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(CELESTIAL_ALIGNMENT, [this](Player* bot, Unit*) {
                return bot && this->_resource.astralPower >= 40;
            }, "40+ AP (burst damage, 3min CD)");

            queue->RegisterSpell(CONVOKE_THE_SPIRITS, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(CONVOKE_THE_SPIRITS, [this](Player* bot, Unit*) {
                return bot && bot->HasSpell(CONVOKE_THE_SPIRITS);
            }, "Random spell burst (2min CD, talent)");

            // HIGH: DoT Maintenance
            queue->RegisterSpell(MOONFIRE, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(MOONFIRE, [this](Player*, Unit* target) {
                return target && this->_dotTracker.NeedsRefresh(target->GetGUID(), MOONFIRE);
            }, "Refresh Moonfire (pandemic window)");

            queue->RegisterSpell(SUNFIRE, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SUNFIRE, [this](Player*, Unit* target) {
                return target && this->_dotTracker.NeedsRefresh(target->GetGUID(), SUNFIRE);
            }, "Refresh Sunfire");

            queue->RegisterSpell(STELLAR_FLARE, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(STELLAR_FLARE, [this](Player* bot, Unit* target) {
                return target && bot->HasSpell(STELLAR_FLARE) &&
                       this->_dotTracker.NeedsRefresh(target->GetGUID(), STELLAR_FLARE);
            }, "Refresh Stellar Flare (talent)");

            // MEDIUM: Astral Power Spenders
            queue->RegisterSpell(STARSURGE, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(STARSURGE, [this](Player*, Unit* target) {
                return target && (this->_shootingStarsProc || this->_resource.astralPower >= 30);
            }, "30 AP or Shooting Stars proc");

            queue->RegisterSpell(STARFALL, SpellPriority::MEDIUM, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(STARFALL, [this](Player*, Unit*) {
                return this->_resource.astralPower >= 50 && !this->_starfallActive &&
                       this->GetEnemiesInRange(40.0f) >= 3;
            }, "50 AP, 3+ enemies (AoE ground effect)");

            // LOW: Astral Power Generators
            queue->RegisterSpell(STARFIRE, SpellPriority::LOW, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(STARFIRE, [this](Player*, Unit* target) {
                return target && (this->_eclipseTracker.IsInLunarEclipse() || !this->_eclipseTracker.IsInEclipse());
            }, "Lunar Eclipse or no Eclipse (8 AP)");

            queue->RegisterSpell(WRATH, SpellPriority::LOW, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(WRATH, [this](Player*, Unit* target) {
                return target && this->_eclipseTracker.IsInSolarEclipse();
            }, "Solar Eclipse (6 AP)");
        }

        auto* behaviorTree = ai->GetBehaviorTree();
        if (behaviorTree)
        {
            auto root = Selector("Balance Druid DPS", {
                // Tier 1: Burst Cooldowns (Incarnation/Celestial Alignment, Convoke)
                Sequence("Burst Cooldowns", {
                    Condition("40+ AP and in combat", [this](Player* bot, Unit*) {
                        return bot && bot->IsInCombat() && this->_resource.astralPower >= 40;
                    }),
                    Selector("Use burst cooldowns", {
                        Sequence("Incarnation (talent)", {
                            Condition("Has Incarnation", [this](Player* bot, Unit*) {
                                return bot->HasSpell(INCARNATION_CHOSEN);
                            }),
                            bot::ai::Action("Cast Incarnation", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(INCARNATION_CHOSEN, bot))
                                {
                                    this->CastSpell(INCARNATION_CHOSEN, bot);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Celestial Alignment", {
                            bot::ai::Action("Cast Celestial Alignment", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(CELESTIAL_ALIGNMENT, bot))
                                {
                                    this->CastSpell(CELESTIAL_ALIGNMENT, bot);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Convoke the Spirits (talent)", {
                            Condition("Has Convoke", [this](Player* bot, Unit*) {
                                return bot->HasSpell(CONVOKE_THE_SPIRITS);
                            }),
                            bot::ai::Action("Cast Convoke", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(CONVOKE_THE_SPIRITS, bot))
                                {
                                    this->CastSpell(CONVOKE_THE_SPIRITS, bot);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 2: DoT Maintenance (Moonfire, Sunfire, Stellar Flare)
                Sequence("DoT Maintenance", {
                    Condition("Has target", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim();
                    }),
                    Selector("Apply/Refresh DoTs", {
                        Sequence("Moonfire", {
                            Condition("Needs Moonfire", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                return target && this->_dotTracker.NeedsRefresh(target->GetGUID(), MOONFIRE);
                            }),
                            bot::ai::Action("Cast Moonfire", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(MOONFIRE, target))
                                {
                                    this->CastSpell(MOONFIRE, target);
                                    this->_dotTracker.ApplyDoT(target->GetGUID(), MOONFIRE, 22000);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Sunfire", {
                            Condition("Needs Sunfire", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                return target && this->_dotTracker.NeedsRefresh(target->GetGUID(), SUNFIRE);
                            }),
                            bot::ai::Action("Cast Sunfire", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(SUNFIRE, target))
                                {
                                    this->CastSpell(SUNFIRE, target);
                                    this->_dotTracker.ApplyDoT(target->GetGUID(), SUNFIRE, 18000);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Stellar Flare (talent)", {
                            Condition("Needs Stellar Flare", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                return bot->HasSpell(STELLAR_FLARE) && target &&
                                       this->_dotTracker.NeedsRefresh(target->GetGUID(), STELLAR_FLARE);
                            }),
                            bot::ai::Action("Cast Stellar Flare", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(STELLAR_FLARE, target))
                                {
                                    this->CastSpell(STELLAR_FLARE, target);
                                    this->_dotTracker.ApplyDoT(target->GetGUID(), STELLAR_FLARE, 24000);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 3: Astral Power Spender (Starsurge, Starfall)
                Sequence("AP Spender", {
                    Condition("Has 30+ AP and target", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim() &&
                               (this->_resource.astralPower >= 30 || this->_shootingStarsProc);
                    }),
                    Selector("Spend AP", {
                        Sequence("Starfall (AoE)", {
                            Condition("50+ AP, 3+ enemies, not active", [this](Player*, Unit*) {
                                return this->_resource.astralPower >= 50 && !this->_starfallActive &&
                                       this->GetEnemiesInRange(40.0f) >= 3;
                            }),
                            bot::ai::Action("Cast Starfall", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(STARFALL, bot))
                                {
                                    this->CastSpell(STARFALL, bot);
                                    this->_starfallActive = true;
                                    this->_starfallEndTime = GameTime::GetGameTimeMS() + 8000;
                                    this->ConsumeAstralPower(50);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Starsurge (single target)", {
                            Condition("30+ AP or Shooting Stars proc", [this](Player*, Unit*) {
                                return this->_resource.astralPower >= 30 || this->_shootingStarsProc;
                            }),
                            bot::ai::Action("Cast Starsurge", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(STARSURGE, target))
                                {
                                    this->CastSpell(STARSURGE, target);
                                    if (this->_shootingStarsProc)
                                        this->_shootingStarsProc = false;
                                    else
                                        this->ConsumeAstralPower(30);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 4: AP Generator (Starfire in Lunar, Wrath in Solar)
                Sequence("AP Generator", {
                    Condition("Has target", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim();
                    }),
                    Selector("Generate AP", {
                        Sequence("Starfire (Lunar Eclipse)", {
                            Condition("Lunar Eclipse or no Eclipse", [this](Player*, Unit*) {
                                return this->_eclipseTracker.IsInLunarEclipse() || !this->_eclipseTracker.IsInEclipse();
                            }),
                            bot::ai::Action("Cast Starfire", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(STARFIRE, target))
                                {
                                    this->CastSpell(STARFIRE, target);
                                    this->GenerateAstralPower(8);
                                    if (!this->_eclipseTracker.IsInEclipse())
                                        this->_eclipseTracker.EnterLunarEclipse();
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Wrath (Solar Eclipse)", {
                            Condition("Solar Eclipse", [this](Player*, Unit*) {
                                return this->_eclipseTracker.IsInSolarEclipse();
                            }),
                            bot::ai::Action("Cast Wrath", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(WRATH, target))
                                {
                                    this->CastSpell(WRATH, target);
                                    this->GenerateAstralPower(6);
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



private:
    BalanceEclipseTracker _eclipseTracker;
    BalanceDoTTracker _dotTracker;
    bool _starfallActive;
    uint32 _starfallEndTime;
    bool _shootingStarsProc;

    // Hero talent detection cache (refreshed on combat start)
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot
