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

#ifndef PLAYERBOT_SHADOWPRIESTREFACTORED_H
#define PLAYERBOT_SHADOWPRIESTREFACTORED_H

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
// WoW 12.0 (The War Within) - Shadow Priest Spell IDs
// Using central registry: WoW120Spells::Priest and WoW120Spells::Priest::Shadow
constexpr uint32 SHADOW_MIND_BLAST = WoW120Spells::Priest::Shadow::MIND_BLAST_SHADOW;
constexpr uint32 SHADOW_MIND_FLAY = WoW120Spells::Priest::Shadow::MIND_FLAY;
constexpr uint32 SHADOW_VAMPIRIC_TOUCH = WoW120Spells::Priest::Shadow::VAMPIRIC_TOUCH;
constexpr uint32 SHADOW_SHADOW_WORD_PAIN = WoW120Spells::Priest::SHADOW_WORD_PAIN;
constexpr uint32 SHADOW_DEVOURING_PLAGUE = WoW120Spells::Priest::Shadow::DEVOURING_PLAGUE;
constexpr uint32 SHADOW_VOID_ERUPTION = WoW120Spells::Priest::Shadow::VOID_ERUPTION;
constexpr uint32 SHADOW_VOID_BOLT = WoW120Spells::Priest::Shadow::VOID_BOLT;
constexpr uint32 SHADOW_MIND_SEAR = WoW120Spells::Priest::Shadow::MIND_SEAR;
constexpr uint32 SHADOW_SHADOW_CRASH = WoW120Spells::Priest::Shadow::SHADOW_CRASH;
constexpr uint32 SHADOW_VOID_TORRENT = WoW120Spells::Priest::Shadow::VOID_TORRENT;
constexpr uint32 SHADOW_DARK_ASCENSION = WoW120Spells::Priest::Shadow::DARK_ASCENSION;
constexpr uint32 SHADOW_MINDGAMES = WoW120Spells::Priest::Shadow::MINDGAMES_SHADOW;
constexpr uint32 SHADOW_SHADOW_WORD_DEATH = WoW120Spells::Priest::SHADOW_WORD_DEATH;
constexpr uint32 SHADOW_VAMPIRIC_EMBRACE = WoW120Spells::Priest::Shadow::VAMPIRIC_EMBRACE;
constexpr uint32 SHADOW_DISPERSION = WoW120Spells::Priest::Shadow::DISPERSION;
constexpr uint32 SHADOW_FADE = WoW120Spells::Priest::FADE;
constexpr uint32 SHADOW_DESPERATE_PRAYER = WoW120Spells::Priest::DESPERATE_PRAYER;
constexpr uint32 SHADOW_POWER_WORD_FORTITUDE = WoW120Spells::Priest::POWER_WORD_FORTITUDE;
constexpr uint32 SHADOW_SHADOWFORM = WoW120Spells::Priest::Shadow::SHADOWFORM;

// TrinityCore 12.0 Talent Spell IDs
// Using central registry: WoW120Spells::Priest::Shadow
constexpr uint32 SHADOW_MINDBENDER = WoW120Spells::Priest::Shadow::MINDBENDER;
constexpr uint32 SHADOW_SHADOWFIEND = WoW120Spells::Priest::Shadow::SHADOWFIEND_SHADOW;
constexpr uint32 SHADOW_HALO = WoW120Spells::Priest::Shadow::HALO_SHADOW;

// Insanity tracker (Shadow Priest secondary resource - primary is still Mana)
class InsanityTracker
{
public:
    InsanityTracker() : _insanity(0), _maxInsanity(100) {}

    void GenerateInsanity(uint32 amount)
    {
        _insanity = ::std::min(_insanity + amount, _maxInsanity);
    }

    void SpendInsanity(uint32 amount)
    {
        if (_insanity >= amount)
            _insanity -= amount;
        else
            _insanity = 0;
    }

    [[nodiscard]] bool HasInsanity(uint32 amount) const { return _insanity >= amount; }
    [[nodiscard]] uint32 GetInsanity() const { return _insanity; }
    [[nodiscard]] uint32 GetInsanityPercent() const { return (_insanity * 100) / _maxInsanity; }

    void Reset() { _insanity = 0; }

private:
    CooldownManager _cooldowns;
    uint32 _insanity;
    uint32 _maxInsanity;
};

// Voidform tracker (Shadow's burst mode)
class VoidformTracker
{
public:
    VoidformTracker() : _voidformActive(false), _voidformStacks(0), _voidformEndTime(0) {}

    void ActivateVoidform()
    {
        _voidformActive = true;
        _voidformStacks = 1;
        _voidformEndTime = GameTime::GetGameTimeMS() + 15000; // Base duration
    }

    void DeactivateVoidform()
    {
        _voidformActive = false;
        _voidformStacks = 0;
    }

    void IncrementStack()
    {
        if (_voidformActive)
        {
            _voidformStacks++;
            // Each stack increases drain rate, but also extends slightly
            _voidformEndTime = GameTime::GetGameTimeMS() + 1000; // 1 sec extension per stack
        }
    }

    [[nodiscard]] bool IsActive() const
    {
        return _voidformActive && GameTime::GetGameTimeMS() < _voidformEndTime;
    }

    [[nodiscard]] uint32 GetStacks() const { return _voidformStacks; }

    void Update(Player* bot)
    {
        if (!bot)
            return;

        // Check if Voidform buff is active
        if (bot->HasAura(SHADOW_VOID_ERUPTION) || bot->HasAura(194249)) // Voidform buff ID
        {
            if (!_voidformActive)
                ActivateVoidform();
        }
        else
        {
            if (_voidformActive)
                DeactivateVoidform();
        }

        // Voidform expires over time
        if (_voidformActive && GameTime::GetGameTimeMS() >= _voidformEndTime)
            DeactivateVoidform();
    }

private:
    bool _voidformActive;
    uint32 _voidformStacks;
    uint32 _voidformEndTime;
};

// DoT tracker for Vampiric Touch and Shadow Word: Pain
class ShadowDoTTracker
{
public:
    ShadowDoTTracker() = default;

    void ApplyVampiricTouch(ObjectGuid guid, uint32 duration = 21000)
    {
        _vampiricTouchTargets[guid] = GameTime::GetGameTimeMS() + duration;
    }

    void ApplyShadowWordPain(ObjectGuid guid, uint32 duration = 16000)
    {
        _shadowWordPainTargets[guid] = GameTime::GetGameTimeMS() + duration;
    }

    [[nodiscard]] bool HasVampiricTouch(ObjectGuid guid) const
    {
        auto it = _vampiricTouchTargets.find(guid);
        if (it == _vampiricTouchTargets.end())
            return false;
        return GameTime::GetGameTimeMS() < it->second;
    }

    [[nodiscard]] bool HasShadowWordPain(ObjectGuid guid) const
    {
        auto it = _shadowWordPainTargets.find(guid);
        if (it == _shadowWordPainTargets.end())
            return false;
        return GameTime::GetGameTimeMS() < it->second;
    }

    [[nodiscard]] uint32 GetVampiricTouchTimeRemaining(ObjectGuid guid) const
    {
        auto it = _vampiricTouchTargets.find(guid);
        if (it == _vampiricTouchTargets.end())
            return 0;

        uint32 now = GameTime::GetGameTimeMS();
        if (now >= it->second)
            return 0;

        return it->second - now;
    }

    [[nodiscard]] uint32 GetShadowWordPainTimeRemaining(ObjectGuid guid) const
    {
        auto it = _shadowWordPainTargets.find(guid);
        if (it == _shadowWordPainTargets.end())
            return 0;

        uint32 now = GameTime::GetGameTimeMS();
        if (now >= it->second)
            return 0;

        return it->second - now;
    }

    [[nodiscard]] bool NeedsVampiricTouchRefresh(ObjectGuid guid, uint32 pandemicWindow = 6300) const
    {
        uint32 remaining = GetVampiricTouchTimeRemaining(guid);
        return remaining < pandemicWindow;
    }

    [[nodiscard]] bool NeedsShadowWordPainRefresh(ObjectGuid guid, uint32 pandemicWindow = 4800) const
    {
        uint32 remaining = GetShadowWordPainTimeRemaining(guid);
        return remaining < pandemicWindow;
    }

    void Update(Player* bot)
    {
        if (!bot)
            return;

        // Clean up expired DoTs
        uint32 now = GameTime::GetGameTimeMS();
        for (auto it = _vampiricTouchTargets.begin(); it != _vampiricTouchTargets.end();)
        {
            if (now >= it->second)
                it = _vampiricTouchTargets.erase(it);
            else
                ++it;
        }

        for (auto it = _shadowWordPainTargets.begin(); it != _shadowWordPainTargets.end();)
        {
            if (now >= it->second)
                it = _shadowWordPainTargets.erase(it);
            else
                ++it;
        }
    }

private:
    ::std::unordered_map<ObjectGuid, uint32> _vampiricTouchTargets; // GUID -> expiration time
    ::std::unordered_map<ObjectGuid, uint32> _shadowWordPainTargets; // GUID -> expiration time
};

// ============================================================================
// SURGE OF INSANITY PROC TRACKER
// ============================================================================
// Surge of Insanity: Devouring Plague grants Surge of Insanity, making the next
// Mind Flay: Insanity or Mind Spike: Insanity instant and more powerful.
// 2 stacks max, consumed on cast.

constexpr uint32 SHADOW_SURGE_OF_INSANITY = WoW120Spells::Priest::Shadow::SURGE_OF_INSANITY;
constexpr uint32 SHADOW_MIND_FLAY_INSANITY = WoW120Spells::Priest::Shadow::MIND_FLAY_INSANITY;
constexpr uint32 SHADOW_MIND_SPIKE_INSANITY = WoW120Spells::Priest::Shadow::MIND_SPIKE_INSANITY;
constexpr uint32 SHADOW_DEATHSPEAKER = WoW120Spells::Priest::Shadow::DEATHSPEAKER;

class SurgeOfInsanityTracker
{
public:
    SurgeOfInsanityTracker() : _stacks(0), _hasMindSpike(false) {}

    /// Called when Devouring Plague is cast (generates proc stacks)
    void OnDevouringPlagueCast() { _stacks = ::std::min(_stacks + 1u, 2u); }

    /// Consume one stack after casting Mind Flay/Spike: Insanity
    void ConsumeStack()
    {
        if (_stacks > 0)
            _stacks--;
    }

    [[nodiscard]] bool IsActive() const { return _stacks > 0; }
    [[nodiscard]] uint32 GetStacks() const { return _stacks; }
    [[nodiscard]] bool HasMindSpikeTalent() const { return _hasMindSpike; }

    /// Sync with actual aura state
    void Update(Player* bot)
    {
        if (!bot)
            return;

        // Check Mind Spike talent (replaces Mind Flay)
        _hasMindSpike = bot->HasSpell(WoW120Spells::Priest::Shadow::MIND_SPIKE);

        if (Aura* aura = bot->GetAura(SHADOW_SURGE_OF_INSANITY))
            _stacks = aura->GetStackAmount();
        else
            _stacks = 0;
    }

private:
    uint32 _stacks;
    bool _hasMindSpike;
};

class ShadowPriestRefactored : public RangedDpsSpecialization<ManaResource>
{
public:
    using Base = RangedDpsSpecialization<ManaResource>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::_resource;
    explicit ShadowPriestRefactored(Player* bot)
        : RangedDpsSpecialization<ManaResource>(bot)
        , _insanityTracker()
        , _voidformTracker()
        , _dotTracker()
        , _surgeOfInsanityTracker()
        , _talentState(bot)  // NEW: TrinityCore 12.0 talent state
        , _darkAscensionActive(false)
        , _darkAscensionEndTime(0)
        , _lastDarkAscensionTime(0)
        , _lastVoidTorrentTime(0)
        , _lastMindgamesTime(0)
        , _lastVampiricEmbraceTime(0)
        , _lastMindbenderTime(0)  // NEW: Track Mindbender/Shadowfiend CD
        , _lastHaloTime(0)        // NEW: Track Halo CD
        // _cooldowns removed - inherited from base class, initialized there
    {
        // Register cooldowns for major abilities
        // COMMENTED OUT:         _cooldowns.RegisterBatch({
        // COMMENTED OUT:             {SHADOW_VOID_ERUPTION, 90000, 1},
        // COMMENTED OUT:             {SHADOW_POWER_INFUSION, 120000, 1},
        // COMMENTED OUT:             {SHADOW_SHADOW_CRASH, 30000, 1},
        // COMMENTED OUT:             {SHADOW_VOID_TORRENT, 45000, 1},
        // COMMENTED OUT:             {SHADOW_SHADOWFIEND, 180000, 1}
        // COMMENTED OUT:         });

        // Phase 5 Integration: Initialize decision systems
        InitializeShadowMechanics();

        TC_LOG_DEBUG("playerbot", "ShadowPriestRefactored initialized for bot {}", bot->GetGUID().GetCounter());
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
        if (_heroTalents.IsTree(HeroTalentTree::VOIDWEAVER))
        {
            // Voidweaver: Void Blast for enhanced shadow burst
            if (this->CanCastSpell(WoW120Spells::Priest::Shadow::SHADOW_VOID_BLAST, target))
            {
                this->CastSpell(WoW120Spells::Priest::Shadow::SHADOW_VOID_BLAST, target);
                return;
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::ARCHON))
        {
            // Archon: Divine Halo (Shadow) for dark radiant burst
            if (this->CanCastSpell(WoW120Spells::Priest::Shadow::SHADOW_DIVINE_HALO, this->GetBot()))
            {
                this->CastSpell(WoW120Spells::Priest::Shadow::SHADOW_DIVINE_HALO, this->GetBot());
                return;
            }
        }

        UpdateShadowState();

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

        // Shadowform (mandatory buff - 10% damage increase)
        if (!bot->HasAura(SHADOW_SHADOWFORM))
        {
            if (this->CanCastSpell(SHADOW_SHADOWFORM, bot))
            {
                this->CastSpell(SHADOW_SHADOWFORM, bot);
            }
        }

        // Power Word: Fortitude (group buff)
        if (!bot->HasAura(SHADOW_POWER_WORD_FORTITUDE))
        {
            if (this->CanCastSpell(SHADOW_POWER_WORD_FORTITUDE, bot))
            {
                this->CastSpell(SHADOW_POWER_WORD_FORTITUDE, bot);
            }        }

        // Vampiric Embrace (healing for group)
        if ((GameTime::GetGameTimeMS() - _lastVampiricEmbraceTime) >= 120000) // 2 min CD
        {            if (bot->GetHealthPct() < 70.0f || (bot->GetGroup() && bot->GetGroup()->GetMembersCount() > 1))
            {
                if (this->CanCastSpell(SHADOW_VAMPIRIC_EMBRACE, bot))
                {
                    this->CastSpell(SHADOW_VAMPIRIC_EMBRACE, bot);
                    _lastVampiricEmbraceTime = GameTime::GetGameTimeMS();
                }
            }
        }
    }

    void UpdateDefensives()
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        float healthPct = bot->GetHealthPct();

        // Dispersion (immune + heal)
        if (healthPct < 25.0f && this->CanCastSpell(SHADOW_DISPERSION, bot))
        {
            this->CastSpell(SHADOW_DISPERSION, bot);
            return;
        }

        // Desperate Prayer (self-heal)
        if (healthPct < 40.0f && this->CanCastSpell(SHADOW_DESPERATE_PRAYER, bot))
        {
            this->CastSpell(SHADOW_DESPERATE_PRAYER, bot);
            return;
        }

        // Fade (threat reduction)
        if (healthPct < 60.0f && bot->GetThreatManager().GetThreatListSize() > 0)
        {
            if (this->CanCastSpell(SHADOW_FADE, bot))
            {
                this->CastSpell(SHADOW_FADE, bot);
                return;
            }
        }
    }

private:
    

    void UpdateShadowState()
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        _voidformTracker.Update(bot);
        _dotTracker.Update(bot);
        _surgeOfInsanityTracker.Update(bot);
        _talentState.Update();  // NEW: Update talent state
        UpdateCooldownStates();
    }

    // ========================================================================
    // NEW: TrinityCore 12.0 TALENT-BASED ROTATION ENHANCEMENTS
    // ========================================================================

    /**
     * Handle Inescapable Torment talent optimization
     *
     * When Mindbender/Shadowfiend is active, prioritize Mind Blast and SW:Death
     * to extend the pet duration and trigger additional damage.
     */
    bool HandleInescapableTormentPriority(::Unit* target)
    {
        Player* bot = this->GetBot();
        if (!bot || !target)
            return false;

        // Only active if we have the talent and pet is out
        if (!_talentState.talents.HasInescapableTorment())
            return false;

        if (!_talentState.inescapableTorment.IsPetActive())
            return false;

        // Prioritize Mind Blast to extend pet
        if (this->CanCastSpell(SHADOW_MIND_BLAST, target))
        {
            this->CastSpell(SHADOW_MIND_BLAST, target);
            _insanityTracker.GenerateInsanity(12);
            _talentState.inescapableTorment.OnExtensionApplied(1000); // 1 sec extension
            TC_LOG_DEBUG("module.playerbot.shadow",
                "ShadowPriest: {} used Mind Blast for Inescapable Torment (pet remaining: {}ms)",
                bot->GetName(), _talentState.inescapableTorment.GetRemainingPetDuration());
            return true;
        }

        // SW:Death also triggers extension (execute phase)
        if (target->GetHealthPct() < 20.0f && this->CanCastSpell(SHADOW_SHADOW_WORD_DEATH, target))
        {
            this->CastSpell(SHADOW_SHADOW_WORD_DEATH, target);
            _insanityTracker.GenerateInsanity(15);
            _talentState.inescapableTorment.OnExtensionApplied(1000);
            return true;
        }

        return false;
    }

    /**
     * Handle Death's Torment talent for execute phase
     *
     * When target has DoTs and is low health, SW:Death will hit multiple times
     * if the target dies.
     */
    bool HandleDeathsTormentPriority(::Unit* target)
    {
        Player* bot = this->GetBot();
        if (!bot || !target)
            return false;

        if (!_talentState.talents.HasDeathsTorment())
            return false;

        // Only prioritize if target is in execute range and has our DoTs
        if (target->GetHealthPct() > 20.0f)
            return false;

        bool hasVT = _dotTracker.HasVampiricTouch(target->GetGUID());
        bool hasSWP = _dotTracker.HasShadowWordPain(target->GetGUID());

        if (!hasVT && !hasSWP)
            return false;

        // High priority SW:Death - will proc multiple hits on kill
        if (this->CanCastSpell(SHADOW_SHADOW_WORD_DEATH, target))
        {
            this->CastSpell(SHADOW_SHADOW_WORD_DEATH, target);
            _insanityTracker.GenerateInsanity(15);
            TC_LOG_DEBUG("module.playerbot.shadow",
                "ShadowPriest: {} used SW:Death for Death's Torment proc (target HP: {:.1f}%)",
                bot->GetName(), target->GetHealthPct());
            return true;
        }

        return false;
    }

    /**
     * Handle Insidious Ire - maximize DoT damage by not refreshing too early
     *
     * With this talent, each DoT tick increases damage. We want to let DoTs
     * tick longer before refreshing to maximize the stacking bonus.
     */
    bool ShouldDelayDoTRefresh(ObjectGuid targetGuid, uint32 spellId) const
    {
        if (!_talentState.talents.HasInsidiousIre())
            return false;

        // Get current damage bonus from Insidious Ire
        float bonus = _talentState.insidiousIre.GetCurrentBonus(targetGuid, spellId);

        // If we have a significant bonus (>10%), delay refresh slightly
        // to maximize damage from remaining ticks
        if (bonus > 10.0f)
        {
            // Only delay if DoT still has substantial time remaining
            uint32 remaining = 0;
            if (spellId == SHADOW_VAMPIRIC_TOUCH)
                remaining = _dotTracker.GetVampiricTouchTimeRemaining(targetGuid);
            else if (spellId == SHADOW_SHADOW_WORD_PAIN)
                remaining = _dotTracker.GetShadowWordPainTimeRemaining(targetGuid);

            // Don't delay if less than 2 seconds remain
            return remaining > 2000;
        }

        return false;
    }

    /**
     * Handle Mental Decay - Mind abilities extend DoTs
     *
     * With this talent, Mind Flay/Sear/Spike extend VT and SW:P.
     * We can use fewer DoT refreshes and more Mind abilities.
     */
    bool ShouldUseMindAbilityForExtension(ObjectGuid targetGuid) const
    {
        if (!_talentState.talents.HasMentalDecay())
            return false;

        // Check if DoTs need extension but not full refresh
        uint32 vtRemaining = _dotTracker.GetVampiricTouchTimeRemaining(targetGuid);
        uint32 swpRemaining = _dotTracker.GetShadowWordPainTimeRemaining(targetGuid);

        // If DoTs have 3-6 seconds remaining, Mind abilities can extend them
        // instead of using a full recast
        bool vtNeedsExtension = vtRemaining > 3000 && vtRemaining < 6000;
        bool swpNeedsExtension = swpRemaining > 3000 && swpRemaining < 6000;

        return vtNeedsExtension || swpNeedsExtension;
    }

    /**
     * Handle Shadowy Apparitions - track and optimize for crit procs
     */
    void OnDoTCriticalHit(ObjectGuid targetGuid)
    {
        if (_talentState.talents.HasShadowyApparitions())
        {
            _talentState.apparitions.OnDoTCritical(targetGuid);
            TC_LOG_DEBUG("module.playerbot.shadow",
                "ShadowPriest: Shadowy Apparition spawned (total: {})",
                _talentState.apparitions.GetTotalSpawned());
        }
    }

    /**
     * Summon Mindbender or Shadowfiend with talent tracking
     */
    bool SummonMindbenderOrShadowfiend(::Unit* target)
    {
        Player* bot = this->GetBot();
        if (!bot || !target)
            return false;

        // Check cooldown (3 min for Shadowfiend, 1 min for Mindbender)
        uint32 cooldownTime = _talentState.talents.HasMindbender() ? 60000 : 180000;
        if ((GameTime::GetGameTimeMS() - _lastMindbenderTime) < cooldownTime)
            return false;

        uint32 petSpell = _talentState.talents.HasMindbender() ?
            SHADOW_MINDBENDER : SHADOW_SHADOWFIEND;

        if (this->CanCastSpell(petSpell, target))
        {
            this->CastSpell(petSpell, target);
            _lastMindbenderTime = GameTime::GetGameTimeMS();

            // Track for Inescapable Torment
            uint32 baseDuration = _talentState.talents.HasMindbender() ? 15000 : 15000;
            _talentState.inescapableTorment.OnPetSummoned(ObjectGuid::Empty, baseDuration);

            TC_LOG_DEBUG("module.playerbot.shadow",
                "ShadowPriest: {} summoned {} for combat",
                bot->GetName(),
                _talentState.talents.HasMindbender() ? "Mindbender" : "Shadowfiend");
            return true;
        }

        return false;
    }

    /**
     * Cast Halo with Power Surge tracking
     */
    bool CastHaloWithPowerSurge(::Unit* target)
    {
        Player* bot = this->GetBot();
        if (!bot || !target)
            return false;

        if (!bot->HasSpell(SHADOW_HALO))
            return false;

        // Check cooldown (40 sec)
        if ((GameTime::GetGameTimeMS() - _lastHaloTime) < 40000)
            return false;

        if (this->CanCastSpell(SHADOW_HALO, target))
        {
            this->CastSpell(SHADOW_HALO, target);
            _lastHaloTime = GameTime::GetGameTimeMS();

            // Track for Power Surge talent
            if (_talentState.talents.HasPowerSurge())
            {
                _talentState.powerSurge.OnHaloCast(false); // Shadow version
                TC_LOG_DEBUG("module.playerbot.shadow",
                    "ShadowPriest: {} cast Halo with Power Surge proc",
                    bot->GetName());
            }
            return true;
        }

        return false;
    }

    void UpdateCooldownStates()
    {
        Player* bot = this->GetBot();
        if (!bot)            return;

        // Dark Ascension state (alternative to Void Eruption)
        if (_darkAscensionActive && GameTime::GetGameTimeMS() >= _darkAscensionEndTime)
            _darkAscensionActive = false;

        if (bot->HasAura(SHADOW_DARK_ASCENSION))
        {
            _darkAscensionActive = true;
            if (Aura* aura = bot->GetAura(SHADOW_DARK_ASCENSION))                _darkAscensionEndTime = GameTime::GetGameTimeMS() + aura->GetDuration();        }
    }

    void ExecuteSingleTargetRotation(::Unit* target)
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        uint32 insanity = _insanityTracker.GetInsanity();

        // ====================================================================
        // NEW: TrinityCore 12.0 TALENT PRIORITY HANDLING
        // ====================================================================

        // Priority 1: Inescapable Torment - extend pet when active
        if (HandleInescapableTormentPriority(target))
            return;

        // Priority 2: Death's Torment - execute phase optimization
        if (HandleDeathsTormentPriority(target))
            return;

        // Summon pet if available (for Inescapable Torment synergy)
        if (insanity >= 40 && !_talentState.inescapableTorment.IsPetActive())
        {
            if (SummonMindbenderOrShadowfiend(target))
                return;
        }

        // ====================================================================
        // END TALENT PRIORITY HANDLING
        // ====================================================================

        // Priority 3: Consume Surge of Insanity proc (instant Mind Flay/Spike: Insanity)
        if (_surgeOfInsanityTracker.IsActive())
        {
            uint32 insanitySpell = _surgeOfInsanityTracker.HasMindSpikeTalent() ?
                SHADOW_MIND_SPIKE_INSANITY : SHADOW_MIND_FLAY_INSANITY;
            if (this->CanCastSpell(insanitySpell, target))
            {
                this->CastSpell(insanitySpell, target);
                _surgeOfInsanityTracker.ConsumeStack();
                _insanityTracker.GenerateInsanity(12); // Surge of Insanity generates extra insanity
                return;
            }
        }

        // Enter Voidform (or Dark Ascension)
        if (insanity >= 60 && !_voidformTracker.IsActive())
        {
            // Dark Ascension (alternative to Void Eruption)
            if (bot->HasSpell(SHADOW_DARK_ASCENSION) && (GameTime::GetGameTimeMS() - _lastDarkAscensionTime) >= 60000)            {
                if (this->CanCastSpell(SHADOW_DARK_ASCENSION, bot))
                {                    this->CastSpell(SHADOW_DARK_ASCENSION, bot);
                _darkAscensionActive = true;
                    _darkAscensionEndTime = GameTime::GetGameTimeMS() + 15000;
                    _lastDarkAscensionTime = GameTime::GetGameTimeMS();
                    _insanityTracker.SpendInsanity(50);
                    return;                }
            }

            // Void Eruption (enter Voidform)
            if (this->CanCastSpell(SHADOW_VOID_ERUPTION, target))
            {                this->CastSpell(SHADOW_VOID_ERUPTION, target);
            _voidformTracker.ActivateVoidform();
                _insanityTracker.Reset(); // Void Eruption consumes all Insanity
                return;
            }
        }        // Maintain DoTs (with Insidious Ire consideration)
        bool vtNeedsRefresh = !_dotTracker.HasVampiricTouch(target->GetGUID()) ||
            _dotTracker.NeedsVampiricTouchRefresh(target->GetGUID());
        bool swpNeedsRefresh = !_dotTracker.HasShadowWordPain(target->GetGUID()) ||
            _dotTracker.NeedsShadowWordPainRefresh(target->GetGUID());

        // NEW: Mental Decay - use Mind abilities to extend DoTs instead of recasting
        if (_talentState.talents.HasMentalDecay() &&
            ShouldUseMindAbilityForExtension(target->GetGUID()))
        {
            // Use Mind Flay to extend DoTs instead of refreshing
            if (this->CanCastSpell(SHADOW_MIND_FLAY, target))
            {
                this->CastSpell(SHADOW_MIND_FLAY, target);
                _insanityTracker.GenerateInsanity(3);
                _talentState.mentalDecay.OnMindAbilityUsed(target->GetGUID());
                return;
            }
        }

        // NEW: Insidious Ire - delay refresh to maximize stacking damage
        if (vtNeedsRefresh && !ShouldDelayDoTRefresh(target->GetGUID(), SHADOW_VAMPIRIC_TOUCH))
        {
            if (this->CanCastSpell(SHADOW_VAMPIRIC_TOUCH, target))
            {
                this->CastSpell(SHADOW_VAMPIRIC_TOUCH, target);
                _dotTracker.ApplyVampiricTouch(target->GetGUID(), 21000);
                _insanityTracker.GenerateInsanity(5);
                // Track for Insidious Ire
                _talentState.insidiousIre.OnDoTApplied(target->GetGUID(), SHADOW_VAMPIRIC_TOUCH);
                return;
            }
        }

        if (swpNeedsRefresh && !ShouldDelayDoTRefresh(target->GetGUID(), SHADOW_SHADOW_WORD_PAIN))
        {
            if (this->CanCastSpell(SHADOW_SHADOW_WORD_PAIN, target))
            {
                this->CastSpell(SHADOW_SHADOW_WORD_PAIN, target);
                _dotTracker.ApplyShadowWordPain(target->GetGUID(), 16000);
                _insanityTracker.GenerateInsanity(4);
                // Track for Insidious Ire
                _talentState.insidiousIre.OnDoTApplied(target->GetGUID(), SHADOW_SHADOW_WORD_PAIN);
                return;
            }
        }

        // Voidform rotation (in Voidform, use Void Bolt instead of Mind Blast)
        if (_voidformTracker.IsActive())
        {
            // Void Bolt (Voidform exclusive)
            if (this->CanCastSpell(SHADOW_VOID_BOLT, target))
            {
                this->CastSpell(SHADOW_VOID_BOLT, target);
                _voidformTracker.IncrementStack();                // Void Bolt refreshes DoTs                _dotTracker.ApplyVampiricTouch(target->GetGUID(), 21000);                _dotTracker.ApplyShadowWordPain(target->GetGUID(), 16000);
                return;
            }
        }

        // Devouring Plague (Insanity spender)
        if (insanity >= 50)
        {
            if (this->CanCastSpell(SHADOW_DEVOURING_PLAGUE, target))
            {
                this->CastSpell(SHADOW_DEVOURING_PLAGUE, target);
                _insanityTracker.SpendInsanity(50);
                return;
            }
        }

        // Mindgames (cooldown ability)
        if (bot->HasSpell(SHADOW_MINDGAMES) && (GameTime::GetGameTimeMS() - _lastMindgamesTime) >= 45000)        {
            if (this->CanCastSpell(SHADOW_MINDGAMES, target))
            {
                this->CastSpell(SHADOW_MINDGAMES, target);
                _lastMindgamesTime = GameTime::GetGameTimeMS();
                _insanityTracker.GenerateInsanity(10);
                return;
            }
        }

        // Shadow Word: Death (execute + Insanity on kill)
        if (target->GetHealthPct() < 20.0f)
        {
            if (this->CanCastSpell(SHADOW_SHADOW_WORD_DEATH, target))
            {
                this->CastSpell(SHADOW_SHADOW_WORD_DEATH, target);
                _insanityTracker.GenerateInsanity(15);
                return;
            }
        }

        // Mind Blast (Insanity generator)
        if (this->CanCastSpell(SHADOW_MIND_BLAST, target))
        {
            this->CastSpell(SHADOW_MIND_BLAST, target);
            _insanityTracker.GenerateInsanity(12);
            return;
        }

        // Void Torrent (channeled damage)
        if (bot->HasSpell(SHADOW_VOID_TORRENT) && (GameTime::GetGameTimeMS() - _lastVoidTorrentTime) >= 30000)
        
        {
            if (this->CanCastSpell(SHADOW_VOID_TORRENT, target))
            {
                this->CastSpell(SHADOW_VOID_TORRENT, target);
                _lastVoidTorrentTime = GameTime::GetGameTimeMS();
                _insanityTracker.GenerateInsanity(15);
                return;
            }
        }

        // Mind Flay (filler - channels, generates Insanity)
        if (this->CanCastSpell(SHADOW_MIND_FLAY, target))
        {
            this->CastSpell(SHADOW_MIND_FLAY, target);
            _insanityTracker.GenerateInsanity(3);
            return;
        }
    }

    void ExecuteAoERotation(::Unit* target, uint32 enemyCount)
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        uint32 insanity = _insanityTracker.GetInsanity();

        // Enter Voidform for AoE burst
        if (insanity >= 60 && !_voidformTracker.IsActive() && enemyCount >= 4)
        {
            if (bot->HasSpell(SHADOW_DARK_ASCENSION) && (GameTime::GetGameTimeMS() - _lastDarkAscensionTime) >= 60000)
            {
                if (this->CanCastSpell(SHADOW_DARK_ASCENSION, bot))
                {
                    this->CastSpell(SHADOW_DARK_ASCENSION, bot);
                    _darkAscensionActive = true;
                    _darkAscensionEndTime = GameTime::GetGameTimeMS() + 15000;
                    _lastDarkAscensionTime = GameTime::GetGameTimeMS();
                    _insanityTracker.SpendInsanity(50);
                    return;
                }
            }

            if (this->CanCastSpell(SHADOW_VOID_ERUPTION, target))
            {
                this->CastSpell(SHADOW_VOID_ERUPTION, target);
                _voidformTracker.ActivateVoidform();
                _insanityTracker.Reset();
                return;
            }
        }

        // Shadow Crash (AoE DoT application)
        if (bot->HasSpell(SHADOW_SHADOW_CRASH) && enemyCount >= 3)
        {
            if (this->CanCastSpell(SHADOW_SHADOW_CRASH, target))
            {
                this->CastSpell(SHADOW_SHADOW_CRASH, target);
                _insanityTracker.GenerateInsanity(15);
                return;
            }
        }

        // Vampiric Touch on multiple targets
        if (enemyCount <= 5)
        {
            // Multi-dot if 5 or fewer enemies            if (!_dotTracker.HasVampiricTouch(target->GetGUID()))
            {
                if (this->CanCastSpell(SHADOW_VAMPIRIC_TOUCH, target))
                {
                    this->CastSpell(SHADOW_VAMPIRIC_TOUCH, target);
                    _dotTracker.ApplyVampiricTouch(target->GetGUID(), 21000);
                    _insanityTracker.GenerateInsanity(5);
                    return;
                }
            }
        }

        // Devouring Plague (AoE spender)
        if (insanity >= 50 && enemyCount >= 3)
        {
            if (this->CanCastSpell(SHADOW_DEVOURING_PLAGUE, target))
            {
                this->CastSpell(SHADOW_DEVOURING_PLAGUE, target);
                _insanityTracker.SpendInsanity(50);
                return;
            }
        }

        // Mind Sear (AoE filler)
        if (enemyCount >= 3)
        {
            if (this->CanCastSpell(SHADOW_MIND_SEAR, target))
            {
                this->CastSpell(SHADOW_MIND_SEAR, target);
                _insanityTracker.GenerateInsanity(5);
                return;
            }
        }

        // Mind Blast
        if (this->CanCastSpell(SHADOW_MIND_BLAST, target))
        {
            this->CastSpell(SHADOW_MIND_BLAST, target);
            _insanityTracker.GenerateInsanity(12);
            return;
        }

        // Mind Flay (filler)
        if (this->CanCastSpell(SHADOW_MIND_FLAY, target))
        {
            this->CastSpell(SHADOW_MIND_FLAY, target);
            _insanityTracker.GenerateInsanity(3);
            return;
        }
    }

    // Phase 5 Integration: Decision Systems Initialization
    void InitializeShadowMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)
        BotAI* ai = this;

        // ========================================================================
        // ActionPriorityQueue: Register Shadow Priest spells with priorities
        // ========================================================================
        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            // EMERGENCY: Survival cooldowns (HP < 30%)
            queue->RegisterSpell(SHADOW_DISPERSION, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
            queue->AddCondition(SHADOW_DISPERSION, [this](Player* bot, Unit*) {
                return bot && bot->GetHealthPct() < 25.0f;
            }, "Bot HP < 25% (immune + heal)");

            queue->RegisterSpell(SHADOW_DESPERATE_PRAYER, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
            queue->AddCondition(SHADOW_DESPERATE_PRAYER, [this](Player* bot, Unit*) {
                return bot && bot->GetHealthPct() < 40.0f;
            }, "Bot HP < 40% (instant heal)");

            // CRITICAL: Voidform entry and Insanity spenders
            queue->RegisterSpell(SHADOW_VOID_ERUPTION, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(SHADOW_VOID_ERUPTION, [this](Player* bot, Unit*) {
                return this->_insanityTracker.GetInsanity() >= 60 && !this->_voidformTracker.IsActive();
            }, "60+ Insanity and not in Voidform (enter Voidform)");

            queue->RegisterSpell(SHADOW_DARK_ASCENSION, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(SHADOW_DARK_ASCENSION, [this](Player* bot, Unit*) {
                return bot && bot->HasSpell(SHADOW_DARK_ASCENSION) &&
                       this->_insanityTracker.GetInsanity() >= 60 &&
                       (GameTime::GetGameTimeMS() - this->_lastDarkAscensionTime) >= 60000;
            }, "60+ Insanity and Dark Ascension off CD (alternative burst)");

            queue->RegisterSpell(SHADOW_DEVOURING_PLAGUE, SpellPriority::CRITICAL, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SHADOW_DEVOURING_PLAGUE, [this](Player* bot, Unit* target) {
                return target && this->_insanityTracker.GetInsanity() >= 50;
            }, "50+ Insanity (primary Insanity spender)");

            // HIGH: Insanity generators and execute
            queue->RegisterSpell(SHADOW_MIND_BLAST, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SHADOW_MIND_BLAST, [this](Player* bot, Unit* target) {
                return target && !this->_voidformTracker.IsActive();
            }, "Not in Voidform (primary Insanity generator)");

            queue->RegisterSpell(SHADOW_VOID_BOLT, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SHADOW_VOID_BOLT, [this](Player* bot, Unit* target) {
                return target && this->_voidformTracker.IsActive();
            }, "In Voidform (replaces Mind Blast, refreshes DoTs)");

            queue->RegisterSpell(SHADOW_SHADOW_WORD_DEATH, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SHADOW_SHADOW_WORD_DEATH, [this](Player* bot, Unit* target) {
                return target && target->GetHealthPct() < 20.0f;
            }, "Target HP < 20% (execute + Insanity on kill)");

            queue->RegisterSpell(SHADOW_SHADOW_CRASH, SpellPriority::HIGH, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(SHADOW_SHADOW_CRASH, [this](Player* bot, Unit* target) {
                return bot && bot->HasSpell(SHADOW_SHADOW_CRASH) &&
                       target && this->GetEnemiesInRange(40.0f) >= 3;
            }, "3+ enemies (AoE DoT application)");

            // MEDIUM: DoT maintenance and cooldown abilities
            queue->RegisterSpell(SHADOW_VAMPIRIC_TOUCH, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SHADOW_VAMPIRIC_TOUCH, [this](Player* bot, Unit* target) {
                return target && (!this->_dotTracker.HasVampiricTouch(target->GetGUID()) ||
                                  this->_dotTracker.NeedsVampiricTouchRefresh(target->GetGUID()));
            }, "Vampiric Touch missing or needs pandemic refresh");

            queue->RegisterSpell(SHADOW_SHADOW_WORD_PAIN, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SHADOW_SHADOW_WORD_PAIN, [this](Player* bot, Unit* target) {
                return target && (!this->_dotTracker.HasShadowWordPain(target->GetGUID()) ||
                                  this->_dotTracker.NeedsShadowWordPainRefresh(target->GetGUID()));
            }, "Shadow Word: Pain missing or needs pandemic refresh");

            queue->RegisterSpell(SHADOW_MINDGAMES, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SHADOW_MINDGAMES, [this](Player* bot, Unit* target) {
                return bot && bot->HasSpell(SHADOW_MINDGAMES) &&
                       target && (GameTime::GetGameTimeMS() - this->_lastMindgamesTime) >= 45000;
            }, "Mindgames off CD (damage + Insanity gen)");

            queue->RegisterSpell(SHADOW_VOID_TORRENT, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SHADOW_VOID_TORRENT, [this](Player* bot, Unit* target) {
                return bot && bot->HasSpell(SHADOW_VOID_TORRENT) &&
                       target && (GameTime::GetGameTimeMS() - this->_lastVoidTorrentTime) >= 30000;
            }, "Void Torrent off CD (channeled damage + Insanity)");

            // LOW: Filler spells
            queue->RegisterSpell(SHADOW_MIND_FLAY, SpellPriority::LOW, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SHADOW_MIND_FLAY, [this](Player* bot, Unit* target) {
                return target && this->GetEnemiesInRange(40.0f) < 3;
            }, "< 3 enemies (single target filler)");

            queue->RegisterSpell(SHADOW_MIND_SEAR, SpellPriority::LOW, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(SHADOW_MIND_SEAR, [this](Player* bot, Unit* target) {
                return target && this->GetEnemiesInRange(40.0f) >= 3;
            }, "3+ enemies (AoE filler)");

            TC_LOG_INFO("module.playerbot", " SHADOW PRIEST: Registered {} spells in ActionPriorityQueue", queue->GetSpellCount());
        }

        // ========================================================================
        // BehaviorTree: Shadow Priest DPS rotation logic
        // ========================================================================
        auto* behaviorTree = ai->GetBehaviorTree();
        if (behaviorTree)
        {
            auto root = Selector("Shadow Priest DPS", {
                // Tier 1: Voidform Entry (60+ Insanity)
                Sequence("Voidform Entry", {
                    Condition("Has 60+ Insanity", [this](Player* bot, Unit*) {
                        return this->_insanityTracker.GetInsanity() >= 60 && !this->_voidformTracker.IsActive();
                    }),
                    Selector("Choose Voidform Ability", {
                        // Option 1: Dark Ascension (if talented and off CD)
                        Sequence("Cast Dark Ascension", {
                            Condition("Has Dark Ascension talent", [this](Player* bot, Unit*) {
                                return bot && bot->HasSpell(SHADOW_DARK_ASCENSION) &&
                                       (GameTime::GetGameTimeMS() - this->_lastDarkAscensionTime) >= 60000;
                            }),
                            bot::ai::Action("Cast Dark Ascension", [this](Player* bot, Unit* target) -> NodeStatus {
                                if (this->CanCastSpell(SHADOW_DARK_ASCENSION, bot))
                                {
                                    this->CastSpell(SHADOW_DARK_ASCENSION, bot);
                                    this->_darkAscensionActive = true;
                                    this->_darkAscensionEndTime = GameTime::GetGameTimeMS() + 15000;
                                    this->_lastDarkAscensionTime = GameTime::GetGameTimeMS();
                                    this->_insanityTracker.SpendInsanity(50);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        // Option 2: Void Eruption (default Voidform entry)
                        Sequence("Cast Void Eruption", {
                            Condition("Void Eruption available", [this](Player* bot, Unit* target) {
                                return target && this->CanCastSpell(SHADOW_VOID_ERUPTION, target);
                            }),
                            bot::ai::Action("Cast Void Eruption", [this](Player* bot, Unit* target) -> NodeStatus {
                                if (this->CanCastSpell(SHADOW_VOID_ERUPTION, target))
                                {
                                    this->CastSpell(SHADOW_VOID_ERUPTION, target);
                                    this->_voidformTracker.ActivateVoidform();
                                    this->_insanityTracker.Reset();
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 2: DoT Maintenance (Vampiric Touch, Shadow Word: Pain)
                Sequence("DoT Maintenance", {
                    Condition("Target exists", [this](Player* bot, Unit* target) {
                        return target != nullptr;
                    }),
                    Selector("Apply or Refresh DoTs", {
                        // Vampiric Touch maintenance
                        Sequence("Maintain Vampiric Touch", {
                            Condition("VT missing or needs refresh", [this](Player* bot, Unit* target) {
                                return !this->_dotTracker.HasVampiricTouch(target->GetGUID()) ||
                                       this->_dotTracker.NeedsVampiricTouchRefresh(target->GetGUID());
                            }),
                            bot::ai::Action("Cast Vampiric Touch", [this](Player* bot, Unit* target) -> NodeStatus {
                                if (this->CanCastSpell(SHADOW_VAMPIRIC_TOUCH, target))
                                {
                                    this->CastSpell(SHADOW_VAMPIRIC_TOUCH, target);
                                    this->_dotTracker.ApplyVampiricTouch(target->GetGUID(), 21000);
                                    this->_insanityTracker.GenerateInsanity(5);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        // Shadow Word: Pain maintenance
                        Sequence("Maintain Shadow Word: Pain", {
                            Condition("SWP missing or needs refresh", [this](Player* bot, Unit* target) {
                                return !this->_dotTracker.HasShadowWordPain(target->GetGUID()) ||
                                       this->_dotTracker.NeedsShadowWordPainRefresh(target->GetGUID());
                            }),
                            bot::ai::Action("Cast Shadow Word: Pain", [this](Player* bot, Unit* target) -> NodeStatus {
                                if (this->CanCastSpell(SHADOW_SHADOW_WORD_PAIN, target))
                                {
                                    this->CastSpell(SHADOW_SHADOW_WORD_PAIN, target);
                                    this->_dotTracker.ApplyShadowWordPain(target->GetGUID(), 16000);
                                    this->_insanityTracker.GenerateInsanity(4);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 3: Insanity Management (Spend at 50+ with Devouring Plague, Generate with Mind Blast/Void Bolt)
                Sequence("Insanity Management", {
                    Condition("Target exists", [this](Player* bot, Unit* target) {
                        return target != nullptr;
                    }),
                    Selector("Spend or Generate Insanity", {
                        // Spend Insanity (50+)
                        Sequence("Spend Insanity", {
                            Condition("Has 50+ Insanity", [this](Player* bot, Unit*) {
                                return this->_insanityTracker.GetInsanity() >= 50;
                            }),
                            bot::ai::Action("Cast Devouring Plague", [this](Player* bot, Unit* target) -> NodeStatus {
                                if (this->CanCastSpell(SHADOW_DEVOURING_PLAGUE, target))
                                {
                                    this->CastSpell(SHADOW_DEVOURING_PLAGUE, target);
                                    this->_insanityTracker.SpendInsanity(50);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        // Generate Insanity (Voidform: Void Bolt, Normal: Mind Blast)
                        Selector("Generate Insanity", {
                            // In Voidform: Void Bolt
                            Sequence("Cast Void Bolt", {
                                Condition("In Voidform", [this](Player* bot, Unit*) {
                                    return this->_voidformTracker.IsActive();
                                }),
                                bot::ai::Action("Cast Void Bolt", [this](Player* bot, Unit* target) -> NodeStatus {
                                    if (this->CanCastSpell(SHADOW_VOID_BOLT, target))
                                    {
                                        this->CastSpell(SHADOW_VOID_BOLT, target);
                                        this->_voidformTracker.IncrementStack();
                                        // Void Bolt refreshes DoTs
                                        this->_dotTracker.ApplyVampiricTouch(target->GetGUID(), 21000);
                                        this->_dotTracker.ApplyShadowWordPain(target->GetGUID(), 16000);
                                        return NodeStatus::SUCCESS;
                                    }
                                    return NodeStatus::FAILURE;
                                })
                            }),
                            // Outside Voidform: Mind Blast
                            Sequence("Cast Mind Blast", {
                                Condition("Not in Voidform", [this](Player* bot, Unit*) {
                                    return !this->_voidformTracker.IsActive();
                                }),
                                bot::ai::Action("Cast Mind Blast", [this](Player* bot, Unit* target) -> NodeStatus {
                                    if (this->CanCastSpell(SHADOW_MIND_BLAST, target))
                                    {
                                        this->CastSpell(SHADOW_MIND_BLAST, target);
                                        this->_insanityTracker.GenerateInsanity(12);
                                        return NodeStatus::SUCCESS;
                                    }
                                    return NodeStatus::FAILURE;
                                })
                            }),
                            // Execute: Shadow Word: Death (target < 20%)
                            Sequence("Execute Phase", {
                                Condition("Target HP < 20%", [this](Player* bot, Unit* target) {
                                    return target && target->GetHealthPct() < 20.0f;
                                }),
                                bot::ai::Action("Cast Shadow Word: Death", [this](Player* bot, Unit* target) -> NodeStatus {
                                    if (this->CanCastSpell(SHADOW_SHADOW_WORD_DEATH, target))
                                    {
                                        this->CastSpell(SHADOW_SHADOW_WORD_DEATH, target);
                                        this->_insanityTracker.GenerateInsanity(15);
                                        return NodeStatus::SUCCESS;
                                    }
                                    return NodeStatus::FAILURE;
                                })
                            })
                        })
                    })
                }),

                // Tier 4: Filler Rotation (Mind Flay single target, Mind Sear AoE)
                Sequence("Filler Rotation", {
                    Condition("Target exists", [this](Player* bot, Unit* target) {
                        return target != nullptr;
                    }),
                    Selector("Choose Filler", {
                        // AoE filler (3+ enemies)
                        Sequence("AoE Filler", {
                            Condition("3+ enemies", [this](Player* bot, Unit*) {
                                return this->GetEnemiesInRange(40.0f) >= 3;
                            }),
                            bot::ai::Action("Cast Mind Sear", [this](Player* bot, Unit* target) -> NodeStatus {
                                if (this->CanCastSpell(SHADOW_MIND_SEAR, target))
                                {
                                    this->CastSpell(SHADOW_MIND_SEAR, target);
                                    this->_insanityTracker.GenerateInsanity(5);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        // Single target filler
                        Sequence("Single Target Filler", {
                            bot::ai::Action("Cast Mind Flay", [this](Player* bot, Unit* target) -> NodeStatus {
                                if (this->CanCastSpell(SHADOW_MIND_FLAY, target))
                                {
                                    this->CastSpell(SHADOW_MIND_FLAY, target);
                                    this->_insanityTracker.GenerateInsanity(3);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                })
            });

            behaviorTree->SetRoot(root);
            TC_LOG_INFO("module.playerbot", " SHADOW PRIEST: BehaviorTree initialized with 4-tier DPS rotation");
        }
    }

    // Member variables
    InsanityTracker _insanityTracker;
    VoidformTracker _voidformTracker;
    ShadowDoTTracker _dotTracker;
    SurgeOfInsanityTracker _surgeOfInsanityTracker;

    // NEW: TrinityCore 12.0 Talent State
    PriestTalentState _talentState;

    bool _darkAscensionActive;
    uint32 _darkAscensionEndTime;

    uint32 _lastDarkAscensionTime;
    uint32 _lastVoidTorrentTime;
    uint32 _lastMindgamesTime;
    uint32 _lastVampiricEmbraceTime;

    // NEW: TrinityCore 12.0 Cooldown Tracking
    uint32 _lastMindbenderTime;
    uint32 _lastHaloTime;

    // Hero talent detection cache (refreshed on combat start)
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot

#endif // PLAYERBOT_SHADOWPRIESTREFACTORED_H
