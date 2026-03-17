/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Havoc Demon Hunter Specialization - REFACTORED
 *
 * This implements the complete migration of Havoc Demon Hunter to the template-based
 * architecture, eliminating code duplication while maintaining full functionality.
 *
 * Havoc focuses on high mobility, metamorphosis burst windows, and fury management
 * for sustained melee damage with exceptional mobility.
 */

#pragma once


#include "../Common/StatusEffectTracker.h"
#include "../Common/CooldownManager.h"
#include "../Common/RotationHelpers.h"
#include "../CombatSpecializationTemplates.h"
#include "../SpellValidation_WoW120.h"
#include "../HeroTalentDetector.h"      // Hero talent tree detection
#include "ObjectGuid.h"
#include "../../../Spatial/SpatialGridManager.h"
#include "ObjectAccessor.h"
#include "Creature.h"
#include "Map.h"
#include "../ResourceTypes.h"
#include "Unit.h"
#include "SpellHistory.h"
#include <unordered_map>
#include <deque>
#include <chrono>
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
// HAVOC DEMON HUNTER SPELL IDs (WoW 12.0 - The War Within)
// Using centralized spell registry from SpellValidation_WoW120.h
// Note: Uses enum to maintain compatibility with DemonHunterAI.h and existing code
// ============================================================================

enum HavocSpells
{
    // Core Abilities - Using central registry: WoW120Spells::DemonHunter::Havoc
    SPELL_DEMONS_BITE           = WoW120Spells::DemonHunter::Havoc::DEMONS_BITE,
    SPELL_CHAOS_STRIKE          = WoW120Spells::DemonHunter::Havoc::CHAOS_STRIKE,
    SPELL_ANNIHILATION          = WoW120Spells::DemonHunter::Havoc::ANNIHILATION,
    SPELL_BLADE_DANCE           = WoW120Spells::DemonHunter::Havoc::BLADE_DANCE,
    SPELL_DEATH_SWEEP           = WoW120Spells::DemonHunter::Havoc::DEATH_SWEEP,
    SPELL_EYE_BEAM              = WoW120Spells::DemonHunter::Havoc::EYE_BEAM,
    SPELL_IMMOLATION_AURA       = WoW120Spells::DemonHunter::Havoc::IMMOLATION_AURA,
    SPELL_FEL_RUSH              = WoW120Spells::DemonHunter::FEL_RUSH,
    SPELL_VENGEFUL_RETREAT      = WoW120Spells::DemonHunter::VENGEFUL_RETREAT,

    // Major Cooldowns - Using central registry: WoW120Spells::DemonHunter
    SPELL_METAMORPHOSIS         = WoW120Spells::DemonHunter::METAMORPHOSIS_HAVOC,
    SPELL_FEL_BARRAGE           = WoW120Spells::DemonHunter::Havoc::FEL_BARRAGE,
    SPELL_CHAOS_NOVA            = WoW120Spells::DemonHunter::CHAOS_NOVA,
    SPELL_DARKNESS              = WoW120Spells::DemonHunter::DARKNESS,
    SPELL_BLUR                  = WoW120Spells::DemonHunter::BLUR,

    // Talents/Passives - Using central registry: WoW120Spells::DemonHunter::Havoc
    SPELL_DEMONIC               = WoW120Spells::DemonHunter::Havoc::DEMONIC,
    SPELL_MOMENTUM              = WoW120Spells::DemonHunter::Havoc::MOMENTUM,
    SPELL_BLIND_FURY            = WoW120Spells::DemonHunter::Havoc::BLIND_FURY,
    SPELL_FIRST_BLOOD           = WoW120Spells::DemonHunter::Havoc::FIRST_BLOOD,
    SPELL_TRAIL_OF_RUIN         = WoW120Spells::DemonHunter::Havoc::TRAIL_OF_RUIN,
    SPELL_CHAOS_CLEAVE          = WoW120Spells::DemonHunter::Havoc::CHAOS_CLEAVE,
    SPELL_CYCLE_OF_HATRED       = WoW120Spells::DemonHunter::Havoc::CYCLE_OF_HATRED,

    // Utility - Using central registry: WoW120Spells::DemonHunter
    SPELL_DISRUPT               = WoW120Spells::DemonHunter::DISRUPT,
    SPELL_CONSUME_MAGIC         = WoW120Spells::DemonHunter::CONSUME_MAGIC,
    SPELL_IMPRISON              = WoW120Spells::DemonHunter::IMPRISON,
    SPELL_SPECTRAL_SIGHT        = WoW120Spells::DemonHunter::SPECTRAL_SIGHT,
    SPELL_TORMENT               = WoW120Spells::DemonHunter::Havoc::TORMENT,

    // Buffs/Debuffs - Using central registry: WoW120Spells::DemonHunter::Havoc
    BUFF_MOMENTUM               = WoW120Spells::DemonHunter::Havoc::BUFF_MOMENTUM,
    BUFF_FURIOUS_GAZE           = WoW120Spells::DemonHunter::Havoc::BUFF_FURIOUS_GAZE,
    BUFF_METAMORPHOSIS          = WoW120Spells::DemonHunter::Havoc::BUFF_METAMORPHOSIS,
    BUFF_PREPARED               = WoW120Spells::DemonHunter::Havoc::BUFF_PREPARED,
    BUFF_IMMOLATION_AURA        = WoW120Spells::DemonHunter::Havoc::IMMOLATION_AURA,
    BUFF_BLADE_DANCE            = WoW120Spells::DemonHunter::Havoc::BLADE_DANCE,
    BUFF_BLUR                   = WoW120Spells::DemonHunter::BLUR
};

// Fury resource type (simple uint32)
using FuryResource = uint32;

/**
 * Soul Fragment tracking system for Havoc
 *
 * Tracks soul fragments for healing and damage bonuses
 */
class HavocSoulFragmentTracker
{
public:
    explicit HavocSoulFragmentTracker(Player* bot)
        : _bot(bot)
        , _fragmentCount(0)
        , _lastFragmentTime(0)
        , _maxFragments(5)
    {
    }

    void GenerateFragments(uint32 count)
    {
        _fragmentCount = ::std::min<uint32>(_fragmentCount + count, _maxFragments);
        _lastFragmentTime = GameTime::GetGameTimeMS();
    }

    bool ConsumeFragments(uint32 count = 1)
    {
        if (_fragmentCount >= count)
        {

            _fragmentCount -= count;

            return true;
        }
        return false;
    }

    uint32 GetFragmentCount() const { return _fragmentCount; }
    bool HasFragments() const { return _fragmentCount > 0; }

    void UpdateFragments()
    {
        // Soul fragments expire after 20 seconds
        if (_fragmentCount > 0 && GameTime::GetGameTimeMS() - _lastFragmentTime > 20000)
        {

            _fragmentCount = 0;
        }
    }

private:
    CooldownManager _cooldowns;
    Player* _bot;
    uint32 _fragmentCount;
    uint32 _lastFragmentTime;
    const uint32 _maxFragments;
};

/**
 * Momentum tracking system for Havoc
 *
 * Tracks momentum buff from movement abilities for optimal damage
 */
class MomentumTracker
{
public:
    explicit MomentumTracker(Player* bot)
        : _bot(bot)
        , _momentumActive(false)
        , _momentumEndTime(0)
        , _felRushCharges(2)
        , _lastFelRushRecharge(0)
        , _vengefulRetreatReady(true)
        , _lastVengefulRetreat(0)
    {
    }

    void TriggerMomentum()
    {
        _momentumActive = true;
        _momentumEndTime = GameTime::GetGameTimeMS() + 6000; // 6 second buff
    }

    bool HasMomentum() const
    {
        return _momentumActive && GameTime::GetGameTimeMS() < _momentumEndTime;
    }

    void UpdateMomentum()
    {
        uint32 currentTime = GameTime::GetGameTimeMS();

        // Check momentum expiry
        if (_momentumActive && currentTime >= _momentumEndTime)
        {

            _momentumActive = false;

            _momentumEndTime = 0;
        }

        // Recharge Fel Rush (10 second recharge, 2 charges max)
        if (_felRushCharges < 2)
        {

            if (currentTime - _lastFelRushRecharge > 10000)

            {

                _felRushCharges++;

                _lastFelRushRecharge = currentTime;

            }
        }

        // Check Vengeful Retreat cooldown (25 seconds)
        if (!_vengefulRetreatReady && currentTime - _lastVengefulRetreat > 25000)
        {

            _vengefulRetreatReady = true;
        }
    }

    bool CanUseFelRush() const { return _felRushCharges > 0; }
    bool CanUseVengefulRetreat() const { return _vengefulRetreatReady; }

    void UseFelRush()
    {
        if (_felRushCharges > 0)
        {

            _felRushCharges--;

            if (_felRushCharges == 1) // Just consumed first charge

                _lastFelRushRecharge = GameTime::GetGameTimeMS();

            TriggerMomentum();
        }
    }

    void UseVengefulRetreat()
    {
        _vengefulRetreatReady = false;
        _lastVengefulRetreat = GameTime::GetGameTimeMS();
        TriggerMomentum();
    }

    uint32 GetFelRushCharges() const { return _felRushCharges; }

private:
    Player* _bot;
    bool _momentumActive;
    uint32 _momentumEndTime;
    uint32 _felRushCharges;
    uint32 _lastFelRushRecharge;
    bool _vengefulRetreatReady;
    uint32 _lastVengefulRetreat;
};

/**
 * Refactored Havoc Demon Hunter using template architecture
 *
 * Key features:
 * - Inherits from MeleeDpsSpecialization<FuryResource> for role defaults
 * - Comprehensive metamorphosis management
 * - Momentum optimization for maximum DPS
 * - Eye Beam positioning and timing
 * - Soul fragment tracking for sustain
 * - Advanced mobility with Fel Rush and Vengeful Retreat
 */
class HavocDemonHunterRefactored : public MeleeDpsSpecialization<FuryResource>
{
public:
    using Base = MeleeDpsSpecialization<FuryResource>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::_resource;
    using Base::CanUseAbility;
    using Base::ConsumeResource;
    explicit HavocDemonHunterRefactored(Player* bot)
        : MeleeDpsSpecialization<FuryResource>(bot)
        
        , _soulFragments(bot)
        , _momentumTracker(bot)
        , _metamorphosisActive(false)
        , _metamorphosisEndTime(0)
        , _eyeBeamChanneling(false)
        , _eyeBeamEndTime(0)
        , _lastDemonsBite(0)
        , _lastChaosStrike(0)
        , _lastBladeDance(0)
        , _immolationAuraActive(false)
        , _immolationAuraEndTime(0)
        , _furiousGazeActive(false)
        , _furiousGazeEndTime(0)
    {
        // Initialize fury
        _maxResource = 120; // Havoc has 120 max fury
        _resource = 0;      // Start with no fury

        // Setup Havoc-specific cooldown tracking

        // Phase 5: Initialize decision systems
        InitializeHavocMechanics();
    }

    // ========================================================================
    // CORE ROTATION - Havoc specific logic
    // ========================================================================

    void UpdateRotation(::Unit* target) override
    {
        if (!target || !target->IsAlive() || !target->IsHostileTo(this->GetBot()))

            return;

        // Detect hero talents if not yet cached
        if (!_heroTalents.detected)
            _heroTalents.Refresh(this->GetBot());

        // Hero talent rotation branching
        // Havoc DH has access to: Aldrachi Reaver / Fel-Scarred
        if (_heroTalents.IsTree(HeroTalentTree::ALDRACHI_REAVER))
        {
            // Aldrachi Reaver: Art of the Glaive empowers glaive abilities
            if (this->CanCastSpell(WoW120Spells::DemonHunter::Havoc::ART_OF_THE_GLAIVE, target))
            {
                this->CastSpell(WoW120Spells::DemonHunter::Havoc::ART_OF_THE_GLAIVE, target);
                return;
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::FEL_SCARRED))
        {
            // Fel-Scarred: Enhanced Metamorphosis with Demonic Intensity
            // Violent Transformation empowers Meta transformations
            if (this->CanCastSpell(WoW120Spells::DemonHunter::Havoc::DEMONIC_INTENSITY, this->GetBot()))
            {
                this->CastSpell(WoW120Spells::DemonHunter::Havoc::DEMONIC_INTENSITY, this->GetBot());
                return;
            }
        }

        // Update Havoc-specific mechanics
        UpdateHavocState();

        // Handle Eye Beam channeling
        if (_eyeBeamChanneling)
        {

            if (GameTime::GetGameTimeMS() < _eyeBeamEndTime)

                return; // Still channeling

            else

            {

                _eyeBeamChanneling = false;

                _eyeBeamEndTime = 0;
                // Demonic talent triggers short Meta after Eye Beam

                if (this->GetBot()->HasSpell(SPELL_DEMONIC))

                    TriggerDemonicMetamorphosis();

            }
        }

        // Check for AoE situation
        uint32 enemyCount = this->GetEnemiesInRange(8.0f);
        if (enemyCount >= 3)
        {

            ExecuteAoERotation(target, enemyCount);

            return;
        }

        // Single target rotation
        ExecuteSingleTargetRotation(target);
    }

    void UpdateBuffs() override
    {
        Player* bot = this->GetBot();

        // Use Blur for defense if health is low
        if (bot->GetHealthPct() < 50.0f && this->CanUseAbility(SPELL_BLUR))
        {

            this->CastSpell(SPELL_BLUR, bot);
        }

        // Maintain Immolation Aura
        if (!_immolationAuraActive && this->CanUseAbility(SPELL_IMMOLATION_AURA))
        {

            this->CastSpell(SPELL_IMMOLATION_AURA, bot);

            _immolationAuraActive = true;

            _immolationAuraEndTime = GameTime::GetGameTimeMS() + 6000;
        }

        // Use Darkness for group defense
        if (IsGroupTakingHeavyDamage() && this->CanUseAbility(SPELL_DARKNESS))
        {

            this->CastSpell(SPELL_DARKNESS, bot);
        }
    }

    void OnInterruptRequired(::Unit* target, uint32 /*spellId*/)
    {
        if (this->CanUseAbility(SPELL_DISRUPT))
        {

            this->CastSpell(SPELL_DISRUPT, target);
        }
    }

    void OnDispelRequired(::Unit* target)
    {
        if (this->CanUseAbility(SPELL_CONSUME_MAGIC))
        {

            this->CastSpell(SPELL_CONSUME_MAGIC, target);
        }
    }

    Position GetOptimalPosition(::Unit* target)
    {
        // Havoc prefers to be behind target for Chaos Strike crit bonus
        if (target)
        {
            // If preparing for Eye Beam, position for maximum targets

            if (ShouldPrepareEyeBeam())

            {

                return GetEyeBeamPosition(target);

            }

            // Normal positioning - behind target

            float angle = target->GetOrientation() + static_cast<float>(M_PI);
            float distance = 3.0f;


            Position pos;

            pos.m_positionX = target->GetPositionX() + cos(angle) * distance;
            pos.m_positionY = target->GetPositionY() + sin(angle) * distance;
            pos.m_positionZ = target->GetPositionZ();
            pos.SetOrientation(target->GetRelativeAngle(&pos));


            return pos;
        }
        return GetBot()->GetPosition();
    }

protected:
    // ========================================================================
    // RESOURCE MANAGEMENT OVERRIDE
    // ========================================================================

    uint32 GetSpellResourceCost(uint32 spellId) const override
    {
        switch (spellId)
        {

            case SPELL_CHAOS_STRIKE:
            return _metamorphosisActive ? 25 : 40;

            case SPELL_ANNIHILATION:
            return 25; // During Meta only

            case SPELL_BLADE_DANCE:
            return this->GetBot()->HasSpell(SPELL_FIRST_BLOOD) ? 15 : 35;

            case SPELL_DEATH_SWEEP:
            return 15; // During Meta only

            case SPELL_EYE_BEAM:
            return 30;

            case SPELL_FEL_BARRAGE:
            return 60;

            case SPELL_CHAOS_NOVA:
            return 30;

            case SPELL_DEMONS_BITE:
            return 0; // Generates 20-30 fury

            default:
            return 0;
        }
    }

    // ========================================================================
    // HAVOC SPECIFIC ROTATION LOGIC
    // ========================================================================

    void ExecuteSingleTargetRotation(::Unit* target)
    {
        uint32 currentFury = _resource;
        uint32 currentTime = GameTime::GetGameTimeMS();

        // Priority 1: Metamorphosis for burst phase
        if (ShouldUseMetamorphosis() && this->CanUseAbility(SPELL_METAMORPHOSIS))
        {

            this->CastSpell(SPELL_METAMORPHOSIS, this->GetBot());

            _metamorphosisActive = true;

            _metamorphosisEndTime = currentTime + 30000;

            return;
        }

        // Priority 2: Eye Beam with proper positioning
        if (currentFury >= 30 && this->CanUseAbility(SPELL_EYE_BEAM))
        {

            if (this->GetBot()->GetDistance(target) <= 20.0f) // Within Eye Beam range

            {

                this->CastSpell(SPELL_EYE_BEAM, target);

                _eyeBeamChanneling = true;

                _eyeBeamEndTime = currentTime + 2000; // 2 second channel

                this->ConsumeResource(SPELL_EYE_BEAM);

                // Blind Fury generates extra fury

                if (this->GetBot()->HasSpell(SPELL_BLIND_FURY))

                    GenerateFury(50);

                else

                    GenerateFury(30);

                // Trigger Furious Gaze haste buff

                _furiousGazeActive = true;

                _furiousGazeEndTime = currentTime + 10000;

                return;

            }
        }

        // Priority 3: Blade Dance for dodge or AoE
        if (ShouldUseBladeDance(target) && currentFury >= GetSpellResourceCost(SPELL_BLADE_DANCE))
        {

            uint32 spellId = _metamorphosisActive ? SPELL_DEATH_SWEEP : SPELL_BLADE_DANCE;

            this->CastSpell(spellId, this->GetBot());

            _lastBladeDance = currentTime;

            this->ConsumeResource(SPELL_BLADE_DANCE);

            return;
        }

        // Priority 4: Build momentum if talented
        if (this->GetBot()->HasSpell(SPELL_MOMENTUM) && !_momentumTracker.HasMomentum())
        {

            if (BuildMomentum(target))

                return;
        }

        // Priority 5: Chaos Strike spam when high fury
        if (currentFury >= GetSpellResourceCost(SPELL_CHAOS_STRIKE))
        {

            uint32 spellId = _metamorphosisActive ? SPELL_ANNIHILATION : SPELL_CHAOS_STRIKE;

            this->CastSpell(spellId, target);

            _lastChaosStrike = currentTime;

            this->ConsumeResource(SPELL_CHAOS_STRIKE);

            // Chaos Strike has 40% chance to refund 20 fury

            if (rand() % 100 < 40)

                GenerateFury(20);

            // Generate soul fragments

            if (rand() % 100 < 25)

                _soulFragments.GenerateFragments(1);


            return;
        }

        // Priority 6: Demon's Bite to generate fury
        if (currentFury < 80)
        {

            this->CastSpell(SPELL_DEMONS_BITE, target);

            _lastDemonsBite = currentTime;

            GenerateFury(rand() % 11 + 20); // Generate 20-30 fury

            return;
        }
    }

    void ExecuteAoERotation(::Unit* target, uint32 enemyCount)
    {
        uint32 currentFury = _resource;

        // Priority 1: Eye Beam for massive AoE
        if (currentFury >= 30 && this->CanUseAbility(SPELL_EYE_BEAM))
        {

            this->CastSpell(SPELL_EYE_BEAM, target);

            _eyeBeamChanneling = true;

            _eyeBeamEndTime = GameTime::GetGameTimeMS() + 2000;

            this->ConsumeResource(SPELL_EYE_BEAM);

            GenerateFury(this->GetBot()->HasSpell(SPELL_BLIND_FURY) ? 50 : 30);

            return;
        }

        // Priority 2: Fel Barrage for heavy AoE burst
        if (currentFury >= 60 && enemyCount >= 5 && this->CanUseAbility(SPELL_FEL_BARRAGE))
        {

            this->CastSpell(SPELL_FEL_BARRAGE, target);

            this->ConsumeResource(SPELL_FEL_BARRAGE);

            return;
        }

        // Priority 3: Blade Dance/Death Sweep for AoE
        if (currentFury >= GetSpellResourceCost(SPELL_BLADE_DANCE))
        {

            uint32 spellId = _metamorphosisActive ? SPELL_DEATH_SWEEP : SPELL_BLADE_DANCE;

            this->CastSpell(spellId, this->GetBot());

            this->ConsumeResource(SPELL_BLADE_DANCE);

            return;
        }

        // Priority 4: Chaos Nova for AoE stun
        if (currentFury >= 30 && enemyCount >= 4 && this->CanUseAbility(SPELL_CHAOS_NOVA))
        {

            this->CastSpell(SPELL_CHAOS_NOVA, this->GetBot());

            this->ConsumeResource(SPELL_CHAOS_NOVA);

            _soulFragments.GenerateFragments(enemyCount / 2);

            return;
        }

        // Priority 5: Build fury with Demon's Bite
        if (currentFury < 60)
        {

            this->CastSpell(SPELL_DEMONS_BITE, target);

            GenerateFury(rand() % 11 + 20);

            return;
        }
    }

private:
    // ========================================================================
    // HAVOC STATE MANAGEMENT
    // ========================================================================

    void UpdateHavocState()
    {
        uint32 currentTime = GameTime::GetGameTimeMS();

        // Update soul fragments
        _soulFragments.UpdateFragments();

        // Update momentum
        _momentumTracker.UpdateMomentum();

        // Check Metamorphosis expiry
        if (_metamorphosisActive && currentTime > _metamorphosisEndTime)
        {

            _metamorphosisActive = false;

            _metamorphosisEndTime = 0;
        }

        // Check Immolation Aura expiry
        if (_immolationAuraActive && currentTime > _immolationAuraEndTime)
        {

            _immolationAuraActive = false;
            _immolationAuraEndTime = 0;
        }

        // Check Furious Gaze expiry
        if (_furiousGazeActive && currentTime > _furiousGazeEndTime)
        {

            _furiousGazeActive = false;

            _furiousGazeEndTime = 0;
        }

        // Passive fury decay out of combat
        if (!this->GetBot()->IsInCombat() && _resource > 0)
        {

            _resource = (_resource > 1) ? _resource - 1 : 0;
        }
    }

    bool ShouldUseMetamorphosis()
    {
        ::Unit* target = this->GetBot()->GetVictim();
        if (!target)

            return false;

        // Use on high priority targets or burst opportunities
        return (target->GetHealthPct() > 60.0f && _resource > 80) ||

               target->GetLevel() > this->GetBot()->GetLevel() + 2 ||

               this->GetEnemiesInRange(8.0f) >= 4;
    }

    bool ShouldUseBladeDance(Unit* /*target*/) const
    {
        // Use for dodge when taking damage or in AoE
        return GetBot()->GetHealthPct() < 70.0f ||

               this->GetEnemiesInRange(8.0f) >= 2 ||

               (GameTime::GetGameTimeMS() - _lastBladeDance > 9000); // Use on CD for First Blood
    }

    bool ShouldPrepareEyeBeam() const
    {
        if (_resource < 30 || _eyeBeamChanneling)

            return false;

        // Check spell cooldown using TrinityCore API
        if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(SPELL_EYE_BEAM, DIFFICULTY_NONE))
        {

            if (this->GetBot()->GetSpellHistory()->HasCooldown(SPELL_EYE_BEAM))

            {

                uint32 cooldownRemaining = static_cast<uint32>(this->GetBot()->GetSpellHistory()->GetRemainingCooldown(spellInfo).count());

                return cooldownRemaining < 2000;
                }
                }
                return true; // No cooldown, ready to cast
    }

    Position GetEyeBeamPosition(Unit* target)
    {
        // Position to hit maximum enemies in a line
        ::std::vector<Unit*> enemies = GetEnemiesInRangeVector(20.0f);

        if (enemies.size() <= 1)

            return GetOptimalPosition(target);

        // Find best angle to hit most enemies
        float bestAngle = this->GetBot()->GetRelativeAngle(target);
        uint32 bestHits = 1;

        for (float angle = 0; angle < 2.0f * static_cast<float>(M_PI); angle += static_cast<float>(M_PI) / 12.0f) // Check 24 angles
        {

            uint32 hits = CountEnemiesInCone(angle, 20.0f, static_cast<float>(M_PI) / 6.0f);

            if (hits > bestHits)

            {

                bestHits = hits;

                bestAngle = angle;

            }
        }

        // Position at optimal angle
        Position pos;
        pos.m_positionX = target->GetPositionX() - cos(bestAngle) * 5.0f;
        pos.m_positionY = target->GetPositionY() - sin(bestAngle) * 5.0f;        pos.m_positionZ = target->GetPositionZ();        pos.SetOrientation(bestAngle);        return pos;
    }

    bool BuildMomentum(Unit* target)
    {
        // Use Fel Rush to engage and build momentum
        if (_momentumTracker.CanUseFelRush() &&

            this->GetBot()->GetDistance(target) > 5.0f &&

            this->GetBot()->GetDistance(target) < 20.0f)
        {

            this->CastSpell(SPELL_FEL_RUSH, target);

            _momentumTracker.UseFelRush();

            GenerateFury(40); // Fel Rush generates 40 fury

            return true;
        }

        // Use Vengeful Retreat for momentum (requires melee range)
        if (_momentumTracker.CanUseVengefulRetreat() &&

            this->GetBot()->GetDistance(target) < 5.0f)
        {

            this->CastSpell(SPELL_VENGEFUL_RETREAT, this->GetBot());

            _momentumTracker.UseVengefulRetreat();

            return true;
        }

        return false;
    }

    void TriggerDemonicMetamorphosis()
    {
        // Demonic talent grants 6 second Meta after Eye Beam
        _metamorphosisActive = true;
        _metamorphosisEndTime = GameTime::GetGameTimeMS() + 6000;
    }

    bool IsGroupTakingHeavyDamage() const
    {
        uint32 injuredCount = 0;

        if (Group* group = this->GetBot()->GetGroup())
        {

            for (GroupReference& itr : group->GetMembers())

            {

                if (Player* member = itr.GetSource())
                {

                    if (member->IsAlive() && member->GetHealthPct() < 50.0f)

                        injuredCount++;

                }

            }
        }

        return injuredCount >= 2;
    }

    uint32 CountEnemiesInCone(float angle, float range, float arc) const
    {
        uint32 count = 0;

        ::std::list<Unit*> enemies;
        Trinity::AnyUnfriendlyUnitInObjectRangeCheck checker(this->GetBot(), this->GetBot(), range);
        Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(this->GetBot(), enemies, checker);
        // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitAllObjects
        Map* map = this->GetBot()->GetMap();
        if (map)
        {

            auto* spatialGrid = Playerbot::SpatialGridManager::Instance().GetGrid(map);

            if (spatialGrid)

            {

                auto guids = spatialGrid->QueryNearbyCreatureGuids(*this->GetBot(), range);

                for (ObjectGuid guid : guids)

                {

                    if (Creature* creature = ObjectAccessor::GetCreature(*this->GetBot(), guid))

                    {

                        if (checker(creature))

                            enemies.push_back(creature);

                    }

                }

            }
        }

        for (Unit* enemy : enemies)
        {

            float targetAngle = this->GetBot()->GetRelativeAngle(enemy);

            float angleDiff = ::std::abs(targetAngle - angle);

            // Normalize angle difference

            if (angleDiff > static_cast<float>(M_PI))

                angleDiff = 2.0f * static_cast<float>(M_PI) - angleDiff;


            if (angleDiff <= arc / 2)

                count++;
        }

        return count;
    }

    ::std::vector<Unit*> GetEnemiesInRangeVector(float range) const
    {
        ::std::vector<Unit*> enemies;

        ::std::list<Unit*> unitList;
        Trinity::AnyUnfriendlyUnitInObjectRangeCheck checker(this->GetBot(), this->GetBot(), range);
        Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(this->GetBot(), unitList, checker);
        // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitAllObjects
        Map* map = this->GetBot()->GetMap();
        if (map)
        {

            auto* spatialGrid = Playerbot::SpatialGridManager::Instance().GetGrid(map);

            if (spatialGrid)

            {

                auto guids = spatialGrid->QueryNearbyCreatureGuids(*this->GetBot(), range);

                for (ObjectGuid guid : guids)

                {

                    if (Creature* creature = ObjectAccessor::GetCreature(*this->GetBot(), guid))

                    {

                        if (checker(creature))

                            enemies.push_back(creature);

                    }

                }

            }
        }

        enemies.assign(unitList.begin(), unitList.end());
        return enemies;
    }

    void GenerateFury(uint32 amount)
    {
        _resource = ::std::min<uint32>(_resource + amount, _maxResource);
    }

    // ========================================================================
    // PHASE 5: DECISION SYSTEM INTEGRATION
    // ========================================================================

    void InitializeHavocMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)
        BotAI* ai = this;
        if (!ai) return;

        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            // EMERGENCY: Defensive cooldowns

            queue->RegisterSpell(SPELL_BLUR, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);

            queue->AddCondition(SPELL_BLUR, [this](Player* bot, Unit*) {

                return bot && bot->GetHealthPct() < 40.0f && !this->_metamorphosisActive;

            }, "HP < 40% (dodge + dmg reduction)");


            queue->RegisterSpell(SPELL_DARKNESS, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);

            queue->AddCondition(SPELL_DARKNESS, [this](Player*, Unit*) {

                return this->IsGroupTakingHeavyDamage();

            }, "Group heavy damage (smoke bomb)");

            // CRITICAL: Major burst cooldowns

            queue->RegisterSpell(SPELL_METAMORPHOSIS, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);

            queue->AddCondition(SPELL_METAMORPHOSIS, [this](Player*, Unit* target) {

                return target && this->ShouldUseMetamorphosis() && !this->_metamorphosisActive;

            }, "Burst window (30s transform)");


            queue->RegisterSpell(SPELL_FEL_BARRAGE, SpellPriority::CRITICAL, SpellCategory::DAMAGE_AOE);

            queue->AddCondition(SPELL_FEL_BARRAGE, [this](Player*, Unit*) {

                return this->_resource >= 60 && this->GetEnemiesInRange(8.0f) >= 5;

            }, "5+ enemies, 60 fury (heavy AoE)");

            // HIGH: Priority damage abilities

            queue->RegisterSpell(SPELL_EYE_BEAM, SpellPriority::HIGH, SpellCategory::DAMAGE_AOE);

            queue->AddCondition(SPELL_EYE_BEAM, [this](Player* bot, Unit* target) {

                return target && this->_resource >= 30 && !this->_eyeBeamChanneling &&

                       bot->GetDistance(target) <= 20.0f && this->GetEnemiesInRange(20.0f) >= 1;

            }, "30 fury, not channeling (2s channel + Demonic)");


            queue->RegisterSpell(SPELL_BLADE_DANCE, SpellPriority::HIGH, SpellCategory::DAMAGE_AOE);

            queue->AddCondition(SPELL_BLADE_DANCE, [this](Player*, Unit* target) {

                uint32 cost = this->GetSpellResourceCost(SPELL_BLADE_DANCE);

                return target && this->_resource >= cost &&

                       (this->GetEnemiesInRange(8.0f) >= 2 || this->GetBot()->GetHealthPct() < 70.0f);

            }, "15-35 fury, 2+ enemies or defensive");


            queue->RegisterSpell(SPELL_DEATH_SWEEP, SpellPriority::HIGH, SpellCategory::DAMAGE_AOE);

            queue->AddCondition(SPELL_DEATH_SWEEP, [this](Player*, Unit*) {

                return this->_metamorphosisActive && this->_resource >= 15;

            }, "Meta only, 15 fury (AoE)");

            // MEDIUM: Core rotation abilities

            queue->RegisterSpell(SPELL_CHAOS_STRIKE, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);

            queue->AddCondition(SPELL_CHAOS_STRIKE, [this](Player*, Unit* target) {

                uint32 cost = this->GetSpellResourceCost(SPELL_CHAOS_STRIKE);

                return target && this->_resource >= cost && !this->_metamorphosisActive;

            }, "40 fury (25 in Meta), fury spender");


            queue->RegisterSpell(SPELL_ANNIHILATION, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);

            queue->AddCondition(SPELL_ANNIHILATION, [this](Player*, Unit* target) {

                return target && this->_metamorphosisActive && this->_resource >= 25;

            }, "Meta only, 25 fury (enhanced Chaos Strike)");


            queue->RegisterSpell(SPELL_IMMOLATION_AURA, SpellPriority::MEDIUM, SpellCategory::DAMAGE_AOE);

            queue->AddCondition(SPELL_IMMOLATION_AURA, [this](Player*, Unit*) {

                return !this->_immolationAuraActive && this->GetEnemiesInRange(8.0f) >= 1;

            }, "Not active (6s AoE aura)");


            queue->RegisterSpell(SPELL_FEL_RUSH, SpellPriority::MEDIUM, SpellCategory::UTILITY);

            queue->AddCondition(SPELL_FEL_RUSH, [this](Player* bot, Unit* target) {

                return target && this->_momentumTracker.CanUseFelRush() &&

                       bot->GetDistance(target) > 5.0f && bot->GetDistance(target) < 20.0f &&

                       this->GetBot()->HasSpell(SPELL_MOMENTUM) && !this->_momentumTracker.HasMomentum();

            }, "Momentum: 5-20yd gap (40 fury + buff)");


            queue->RegisterSpell(SPELL_VENGEFUL_RETREAT, SpellPriority::MEDIUM, SpellCategory::UTILITY);

            queue->AddCondition(SPELL_VENGEFUL_RETREAT, [this](Player* bot, Unit* target) {

                return target && this->_momentumTracker.CanUseVengefulRetreat() &&

                       bot->GetDistance(target) < 5.0f &&

                       this->GetBot()->HasSpell(SPELL_MOMENTUM) && !this->_momentumTracker.HasMomentum();

            }, "Momentum: melee range (backward leap)");

            // LOW: Fury generators

            queue->RegisterSpell(SPELL_DEMONS_BITE, SpellPriority::LOW, SpellCategory::DAMAGE_SINGLE);

            queue->AddCondition(SPELL_DEMONS_BITE, [this](Player*, Unit* target) {

                return target && this->_resource < 80;

            }, "Fury < 80 (generates 20-30 fury)");

            // UTILITY: Interrupts and CC

            queue->RegisterSpell(SPELL_DISRUPT, SpellPriority::HIGH, SpellCategory::CROWD_CONTROL);

            queue->AddCondition(SPELL_DISRUPT, [](Player*, Unit* target) {

                return target && target->IsNonMeleeSpellCast(false);

            }, "Target casting (interrupt)");


            queue->RegisterSpell(SPELL_CONSUME_MAGIC, SpellPriority::MEDIUM, SpellCategory::UTILITY);

            queue->AddCondition(SPELL_CONSUME_MAGIC, [](Player*, Unit* target) {

                return target && target->HasAura(118);

            }, "Has dispellable magic buff");


            queue->RegisterSpell(SPELL_CHAOS_NOVA, SpellPriority::MEDIUM, SpellCategory::CROWD_CONTROL);

            queue->AddCondition(SPELL_CHAOS_NOVA, [this](Player*, Unit*) {

                return this->_resource >= 30 && this->GetEnemiesInRange(8.0f) >= 3;

            }, "3+ enemies, 30 fury (AoE stun)");
        }

        auto* behaviorTree = ai->GetBehaviorTree();
        if (behaviorTree)
        {

            auto root = Selector("Havoc Demon Hunter DPS", {
                // Tier 1: Emergency Defense

                Sequence("Emergency Defense", {

                    Condition("Low HP", [this](Player* bot, Unit*) {

                        return bot && bot->GetHealthPct() < 40.0f;

                    }),

                    Selector("Use defensive", {

                        Sequence("Blur", {

                            Condition("Not in Meta", [this](Player*, Unit*) {

                                return !this->_metamorphosisActive;

                            }),

                            Condition("Can cast", [this](Player* bot, Unit*) {

                                return this->CanCastSpell(SPELL_BLUR, bot);

                            }),

                            bot::ai::Action("Cast Blur", [this](Player* bot, Unit*) {

                                if (this->CanCastSpell(SPELL_BLUR, bot))
                                {

                                    this->CastSpell(SPELL_BLUR, bot);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),

                        Sequence("Darkness", {

                            Condition("Group heavy damage", [this](Player*, Unit*) {

                                return this->IsGroupTakingHeavyDamage();

                            }),

                            bot::ai::Action("Cast Darkness", [this](Player* bot, Unit*) {

                                if (this->CanCastSpell(SPELL_DARKNESS, bot))
                                {

                                    this->CastSpell(SPELL_DARKNESS, bot);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        })

                    })

                }),

                // Tier 2: Burst Cooldowns

                Sequence("Burst Phase", {

                    Condition("Has target", [this](Player* bot, Unit*) {

                        return bot && bot->GetVictim();

                    }),

                    Selector("Use cooldowns", {

                        Sequence("Metamorphosis", {

                            Condition("Should use Meta", [this](Player*, Unit*) {

                                return this->ShouldUseMetamorphosis() && !this->_metamorphosisActive;

                            }),

                            Condition("High fury", [this](Player*, Unit*) {

                                return this->_resource > 80;

                            }),

                            bot::ai::Action("Cast Metamorphosis", [this](Player* bot, Unit*) {

                                if (this->CanCastSpell(SPELL_METAMORPHOSIS, bot))
                                {

                                    this->CastSpell(SPELL_METAMORPHOSIS, bot);

                                    this->_metamorphosisActive = true;

                                    this->_metamorphosisEndTime = GameTime::GetGameTimeMS() + 30000;

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),

                        Sequence("Fel Barrage AoE", {

                            Condition("5+ enemies", [this](Player*, Unit*) {

                                return this->GetEnemiesInRange(8.0f) >= 5;

                            }),

                            Condition("60 fury", [this](Player*, Unit*) {

                                return this->_resource >= 60;

                            }),

                            bot::ai::Action("Cast Fel Barrage", [this](Player* bot, Unit* target) {

                                Unit* victim = bot->GetVictim();

                                if (victim && this->CanCastSpell(SPELL_FEL_BARRAGE, victim))
                                {

                                    this->CastSpell(SPELL_FEL_BARRAGE, victim);

                                    this->ConsumeResource(SPELL_FEL_BARRAGE);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        })

                    })

                }),

                // Tier 3: Priority Abilities

                Sequence("Priority Rotation", {

                    Condition("Has target", [this](Player* bot, Unit*) {

                        return bot && bot->GetVictim();

                    }),

                    Selector("Use abilities", {

                        Sequence("Eye Beam", {

                            Condition("30 fury", [this](Player*, Unit*) {

                                return this->_resource >= 30;

                            }),

                            Condition("Not channeling", [this](Player*, Unit*) {

                                return !this->_eyeBeamChanneling;

                            }),

                            Condition("In range", [this](Player* bot, Unit* target) {

                                Unit* victim = bot->GetVictim();

                                return target && bot->GetDistance(target) <= 20.0f;

                            }),

                            bot::ai::Action("Cast Eye Beam", [this](Player* bot, Unit* target) {

                                Unit* victim = bot->GetVictim();

                                if (victim && this->CanCastSpell(SPELL_EYE_BEAM, victim))
                                {

                                    this->CastSpell(SPELL_EYE_BEAM, victim);

                                    this->_eyeBeamChanneling = true;

                                    this->_eyeBeamEndTime = GameTime::GetGameTimeMS() + 2000;

                                    this->ConsumeResource(SPELL_EYE_BEAM);

                                    uint32 furyGen = this->GetBot()->HasSpell(SPELL_BLIND_FURY) ? 50 : 30;

                                    this->GenerateFury(furyGen);

                                    this->_furiousGazeActive = true;

                                    this->_furiousGazeEndTime = GameTime::GetGameTimeMS() + 10000;

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),

                        Sequence("Death Sweep (Meta)", {

                            Condition("In Metamorphosis", [this](Player*, Unit*) {

                                return this->_metamorphosisActive;

                            }),

                            Condition("15 fury", [this](Player*, Unit*) {

                                return this->_resource >= 15;

                            }),

                            bot::ai::Action("Cast Death Sweep", [this](Player* bot, Unit*) {

                                if (this->CanCastSpell(SPELL_DEATH_SWEEP, bot))
                                {

                                    this->CastSpell(SPELL_DEATH_SWEEP, bot);

                                    this->ConsumeResource(SPELL_BLADE_DANCE);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),

                        Sequence("Blade Dance", {

                            Condition("Not in Meta", [this](Player*, Unit*) {

                                return !this->_metamorphosisActive;

                            }),

                            Condition("Enough fury", [this](Player*, Unit*) {

                                uint32 cost = this->GetSpellResourceCost(SPELL_BLADE_DANCE);

                                return this->_resource >= cost;

                            }),

                            Condition("AoE or defensive", [this](Player* bot, Unit*) {

                                return this->GetEnemiesInRange(8.0f) >= 2 || bot->GetHealthPct() < 70.0f;

                            }),

                            bot::ai::Action("Cast Blade Dance", [this](Player* bot, Unit*) {

                                if (this->CanCastSpell(SPELL_BLADE_DANCE, bot))
                                {

                                    this->CastSpell(SPELL_BLADE_DANCE, bot);

                                    this->_lastBladeDance = GameTime::GetGameTimeMS();

                                    this->ConsumeResource(SPELL_BLADE_DANCE);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),

                        Sequence("Immolation Aura", {

                            Condition("Not active", [this](Player*, Unit*) {

                                return !this->_immolationAuraActive;

                            }),

                            bot::ai::Action("Cast Immolation Aura", [this](Player* bot, Unit*) {

                                if (this->CanCastSpell(SPELL_IMMOLATION_AURA, bot))
                                {

                                    this->CastSpell(SPELL_IMMOLATION_AURA, bot);

                                    this->_immolationAuraActive = true;

                                    this->_immolationAuraEndTime = GameTime::GetGameTimeMS() + 6000;

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),

                        Sequence("Build Momentum", {

                            Condition("Has Momentum talent", [this](Player* bot, Unit*) {

                                return bot->HasSpell(SPELL_MOMENTUM);

                            }),

                            Condition("No Momentum buff", [this](Player*, Unit*) {

                                return !this->_momentumTracker.HasMomentum();

                            }),

                            Selector("Use movement", {

                                Sequence("Fel Rush", {

                                    Condition("Can use", [this](Player*, Unit*) {

                                        return this->_momentumTracker.CanUseFelRush();

                                    }),

                                    Condition("5-20yd range", [this](Player* bot, Unit* target) {

                                        Unit* victim = bot->GetVictim();

                                        return target && bot->GetDistance(target) > 5.0f && bot->GetDistance(target) < 20.0f;

                                    }),

                                    bot::ai::Action("Cast Fel Rush", [this](Player* bot, Unit* target) {

                                        Unit* victim = bot->GetVictim();

                                        if (victim && this->CanCastSpell(SPELL_FEL_RUSH, victim))
                                        {

                                            this->CastSpell(SPELL_FEL_RUSH, victim);

                                            this->_momentumTracker.UseFelRush();

                                            this->GenerateFury(40);

                                            return NodeStatus::SUCCESS;

                                        }

                                        return NodeStatus::FAILURE;

                                    })

                                }),

                                Sequence("Vengeful Retreat", {

                                    Condition("Can use", [this](Player*, Unit*) {

                                        return this->_momentumTracker.CanUseVengefulRetreat();

                                    }),

                                    Condition("Melee range", [this](Player* bot, Unit* target) {

                                        Unit* victim = bot->GetVictim();

                                        return target && bot->GetDistance(target) < 5.0f;

                                    }),

                                    bot::ai::Action("Cast Vengeful Retreat", [this](Player* bot, Unit*) {

                                        if (this->CanCastSpell(SPELL_VENGEFUL_RETREAT, bot))
                                        {

                                            this->CastSpell(SPELL_VENGEFUL_RETREAT, bot);

                                            this->_momentumTracker.UseVengefulRetreat();

                                            return NodeStatus::SUCCESS;

                                        }

                                        return NodeStatus::FAILURE;

                                    })

                                })

                            })

                        })

                    })

                }),

                // Tier 4: Fury Spenders

                Sequence("Fury Spenders", {

                    Condition("Has target", [this](Player* bot, Unit*) {

                        return bot && bot->GetVictim();

                    }),

                    Selector("Use spenders", {

                        Sequence("Annihilation (Meta)", {

                            Condition("In Metamorphosis", [this](Player*, Unit*) {

                                return this->_metamorphosisActive;

                            }),

                            Condition("25 fury", [this](Player*, Unit*) {

                                return this->_resource >= 25;

                            }),

                            bot::ai::Action("Cast Annihilation", [this](Player* bot, Unit* target) {

                                Unit* victim = bot->GetVictim();

                                if (victim && this->CanCastSpell(SPELL_ANNIHILATION, victim))
                                {

                                    this->CastSpell(SPELL_ANNIHILATION, victim);

                                    this->_lastChaosStrike = GameTime::GetGameTimeMS();

                                    this->ConsumeResource(SPELL_CHAOS_STRIKE);

                                    if (rand() % 100 < 40) this->GenerateFury(20);

                                    if (rand() % 100 < 25) this->_soulFragments.GenerateFragments(1);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),

                        Sequence("Chaos Strike", {

                            Condition("Not in Meta", [this](Player*, Unit*) {

                                return !this->_metamorphosisActive;

                            }),

                            Condition("Enough fury", [this](Player*, Unit*) {

                                uint32 cost = this->GetSpellResourceCost(SPELL_CHAOS_STRIKE);

                                return this->_resource >= cost;

                            }),

                            bot::ai::Action("Cast Chaos Strike", [this](Player* bot, Unit* target) {

                                Unit* victim = bot->GetVictim();

                                if (victim && this->CanCastSpell(SPELL_CHAOS_STRIKE, victim))
                                {

                                    this->CastSpell(SPELL_CHAOS_STRIKE, victim);

                                    this->_lastChaosStrike = GameTime::GetGameTimeMS();

                                    this->ConsumeResource(SPELL_CHAOS_STRIKE);

                                    if (rand() % 100 < 40) this->GenerateFury(20);

                                    if (rand() % 100 < 25) this->_soulFragments.GenerateFragments(1);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        })

                    })

                }),

                // Tier 5: Fury Generators

                Sequence("Fury Generators", {

                    Condition("Has target", [this](Player* bot, Unit*) {

                        return bot && bot->GetVictim();

                    }),

                    Condition("Low fury", [this](Player*, Unit*) {

                        return this->_resource < 80;

                    }),

                    bot::ai::Action("Cast Demon's Bite", [this](Player* bot, Unit* target) {

                        Unit* victim = bot->GetVictim();

                        if (victim && this->CanCastSpell(SPELL_DEMONS_BITE, victim))
                        {

                            this->CastSpell(SPELL_DEMONS_BITE, victim);

                            this->_lastDemonsBite = GameTime::GetGameTimeMS();

                            this->GenerateFury(rand() % 11 + 20);

                            return NodeStatus::SUCCESS;

                        }

                        return NodeStatus::FAILURE;

                    })

                })

            });


            behaviorTree->SetRoot(root);
        }
    }

private:
    HavocSoulFragmentTracker _soulFragments;
    MomentumTracker _momentumTracker;

    // Metamorphosis tracking
    bool _metamorphosisActive;
    uint32 _metamorphosisEndTime;

    // Eye Beam channeling
    bool _eyeBeamChanneling;
    uint32 _eyeBeamEndTime;

    // Ability timing
    uint32 _lastDemonsBite;
    uint32 _lastChaosStrike;
    uint32 _lastBladeDance;

    // Buff tracking
    bool _immolationAuraActive;
    uint32 _immolationAuraEndTime;
    bool _furiousGazeActive;
    uint32 _furiousGazeEndTime;

    // Hero talent detection cache (refreshed on combat start)
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot
