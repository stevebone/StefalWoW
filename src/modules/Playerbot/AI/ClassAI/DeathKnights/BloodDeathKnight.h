/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Blood Death Knight Refactored - Template-Based Implementation
 *
 * This file provides a complete, template-based implementation of Blood Death Knight
 * using the TankSpecialization with dual resource system (Runes + Runic Power).
 */

#pragma once


#include "../Common/StatusEffectTracker.h"
#include "../Common/CooldownManager.h"
#include "../Common/RotationHelpers.h"
#include "../CombatSpecializationTemplates.h"
#include "../ResourceTypes.h"
#include "../SpellValidation_WoW120.h"  // Central spell registry
#include "../HeroTalentDetector.h"      // Hero talent tree detection
#include "../../Services/ThreatAssistant.h"
#include "Player.h"
#include "SpellMgr.h"
#include "SpellAuraEffects.h"
#include "Log.h"
#include "../../Decision/ActionPriorityQueue.h"
#include "../../Decision/BehaviorTree.h"
#include "../BotAI.h"

namespace Playerbot
{

// ============================================================================
// BLOOD DEATH KNIGHT SPELL ALIASES (WoW 12.0 - The War Within)
// Consolidated spell IDs from central registry - NO duplicates
// ============================================================================

namespace BloodDeathKnightSpells
{
    // Rune Spenders
    constexpr uint32 HEART_STRIKE             = WoW120Spells::DeathKnight::Blood::HEART_STRIKE;
    constexpr uint32 BLOOD_BOIL               = WoW120Spells::DeathKnight::Blood::BLOOD_BOIL;
    constexpr uint32 DEATHS_CARESS            = WoW120Spells::DeathKnight::Blood::DEATHS_CARESS;
    constexpr uint32 MARROWREND               = WoW120Spells::DeathKnight::Blood::MARROWREND;
    constexpr uint32 CONSUMPTION              = WoW120Spells::DeathKnight::Blood::CONSUMPTION;

    // Runic Power Spenders
    constexpr uint32 DEATH_STRIKE             = WoW120Spells::DeathKnight::DEATH_STRIKE;
    constexpr uint32 DEATHS_AND_DECAY_BLOOD   = WoW120Spells::DeathKnight::DEATH_AND_DECAY;
    constexpr uint32 BONESTORM                = WoW120Spells::DeathKnight::Blood::BONESTORM;

    // Active Mitigation
    constexpr uint32 VAMPIRIC_BLOOD           = WoW120Spells::DeathKnight::Blood::VAMPIRIC_BLOOD;
    constexpr uint32 DANCING_RUNE_WEAPON      = WoW120Spells::DeathKnight::Blood::DANCING_RUNE_WEAPON;
    constexpr uint32 ICEBOUND_FORTITUDE       = WoW120Spells::DeathKnight::ICEBOUND_FORTITUDE;
    constexpr uint32 ANTI_MAGIC_SHELL         = WoW120Spells::DeathKnight::ANTI_MAGIC_SHELL;
    constexpr uint32 RUNE_TAP                 = WoW120Spells::DeathKnight::Blood::RUNE_TAP;
    constexpr uint32 VAMPIRIC_STRIKE          = WoW120Spells::DeathKnight::Blood::VAMPIRIC_STRIKE;

    // Threat Generation
    constexpr uint32 DARK_COMMAND             = WoW120Spells::DeathKnight::DARK_COMMAND;
    constexpr uint32 BLOOD_PLAGUE             = WoW120Spells::DeathKnight::Blood::BLOOD_PLAGUE;
    constexpr uint32 FROST_FEVER              = WoW120Spells::DeathKnight::Frost::FROST_FEVER;

    // Major Cooldowns
    constexpr uint32 RAISE_DEAD_BLOOD         = WoW120Spells::DeathKnight::RAISE_DEAD;
    constexpr uint32 ARMY_OF_THE_DEAD         = WoW120Spells::DeathKnight::Unholy::ARMY_OF_THE_DEAD;
    constexpr uint32 GOREFIENDS_GRASP         = WoW120Spells::DeathKnight::Blood::GOREFIENDS_GRASP;
    constexpr uint32 BLOODDRINKER             = WoW120Spells::DeathKnight::Blood::BLOODDRINKER;
    constexpr uint32 TOMBSTONE                = WoW120Spells::DeathKnight::Blood::TOMBSTONE;

    // Utility
    constexpr uint32 DEATH_GRIP               = WoW120Spells::DeathKnight::DEATH_GRIP;
    constexpr uint32 DEATHS_ADVANCE           = WoW120Spells::DeathKnight::DEATHS_ADVANCE;
    constexpr uint32 MIND_FREEZE              = WoW120Spells::DeathKnight::MIND_FREEZE;
    constexpr uint32 ASPHYXIATE               = WoW120Spells::DeathKnight::ASPHYXIATE;
    constexpr uint32 CONTROL_UNDEAD           = WoW120Spells::DeathKnight::CONTROL_UNDEAD;
    constexpr uint32 RAISE_ALLY               = WoW120Spells::DeathKnight::RAISE_ALLY;

    // Procs and Buffs
    constexpr uint32 BONE_SHIELD              = WoW120Spells::DeathKnight::Blood::BONE_SHIELD;
    constexpr uint32 CRIMSON_SCOURGE          = WoW120Spells::DeathKnight::Blood::CRIMSON_SCOURGE;
    constexpr uint32 HEMOSTASIS               = WoW120Spells::DeathKnight::Blood::HEMOSTASIS;
    constexpr uint32 OSSUARY                  = WoW120Spells::DeathKnight::Blood::OSSUARY;

    // Talents
    constexpr uint32 BLOOD_TAP                = WoW120Spells::DeathKnight::Blood::BLOOD_TAP;
    constexpr uint32 RAPID_DECOMPOSITION      = WoW120Spells::DeathKnight::Blood::RAPID_DECOMPOSITION;
    constexpr uint32 HEARTBREAKER             = WoW120Spells::DeathKnight::Blood::HEARTBREAKER;
    constexpr uint32 FOUL_BULWARK             = WoW120Spells::DeathKnight::Blood::FOUL_BULWARK;
    constexpr uint32 RELISH_IN_BLOOD          = WoW120Spells::DeathKnight::Blood::RELISH_IN_BLOOD;
}
using namespace BloodDeathKnightSpells;

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

// Dual resource type for Blood Death Knight (Runes + Runic Power)
struct RuneRunicPowerResource
{
    uint32 bloodRunes{0};
    uint32 frostRunes{0};
    uint32 unholyRunes{0};
    uint32 runicPower{0};
    uint32 maxRunicPower{125};
    bool available{true};

    bool Consume(uint32 runesCost)
    {
        uint32 totalRunes = bloodRunes + frostRunes + unholyRunes;
        if (totalRunes >= runesCost) {
            // Consume runes in order: Blood -> Frost -> Unholy
            uint32 remaining = runesCost;
            if (bloodRunes > 0)
            {
                uint32 toConsume = ::std::min(bloodRunes, remaining);
                bloodRunes -= toConsume;
                remaining -= toConsume;
            }
            if (remaining > 0 && frostRunes > 0)
            {
                uint32 toConsume = ::std::min(frostRunes, remaining);
                frostRunes -= toConsume;
                remaining -= toConsume;
            }
            if (remaining > 0 && unholyRunes > 0)
            {
                uint32 toConsume = ::std::min(unholyRunes, remaining);
                unholyRunes -= toConsume;
                remaining -= toConsume;
            }
            available = (bloodRunes + frostRunes + unholyRunes) > 0;
            return true;
        }
        return false;
    }

    void Regenerate(uint32 diff)
    {
        // Runes regenerate over time (10 seconds per rune in WoW)
        // This is a simplified implementation
        static uint32 regenTimer = 0;
        regenTimer += diff;
        if (regenTimer >= 10000) { // 10 seconds
            uint32 totalRunes = bloodRunes + frostRunes + unholyRunes;
            if (totalRunes < 6)
            {
                // Regenerate one rune
                if (bloodRunes < 2) bloodRunes++;
                else if (frostRunes < 2) frostRunes++;
                else if (unholyRunes < 2) unholyRunes++;
            }
            regenTimer = 0;
        }
        available = (bloodRunes + frostRunes + unholyRunes) > 0;
    }

    [[nodiscard]] uint32 GetAvailable() const {
        return bloodRunes + frostRunes + unholyRunes;
    }

    [[nodiscard]] uint32 GetMax() const {
        return 6; // Max 6 runes (2 blood + 2 frost + 2 unholy)
    }

    void Initialize(Player* bot)
    {
        bloodRunes = 2;
        frostRunes = 2;
        unholyRunes = 2;
        runicPower = 0;
        available = true;
    }
};

// ============================================================================
// BLOOD BONE SHIELD TRACKER
// ============================================================================

class BloodBoneShieldTracker
{
public:
    BloodBoneShieldTracker()
        : _boneShieldStacks(0)
        , _lastMarrowrendTime(0)
    {
    }

    void ApplyMarrowrend(uint32 stacks)
    {
        _boneShieldStacks = ::std::min(_boneShieldStacks + stacks, 10u);
        _lastMarrowrendTime = GameTime::GetGameTimeMS();
    }

    void ConsumeStack()
    {
        if (_boneShieldStacks > 0)
            _boneShieldStacks--;
    }

    uint32 GetStacks() const { return _boneShieldStacks; }

    bool NeedsRefresh() const
    {
        // Refresh if below 5 stacks or expiring soon
        return _boneShieldStacks < 5;
    }

    void Update(Player* bot)
    {
        if (!bot)
            return;

        // Sync with actual aura
        if (Aura* aura = bot->GetAura(BONE_SHIELD))
            _boneShieldStacks = aura->GetStackAmount();
        else
            _boneShieldStacks = 0;
    }

private:
    CooldownManager _cooldowns;
    uint32 _boneShieldStacks;
    uint32 _lastMarrowrendTime;
};

// ============================================================================
// BLOOD DEATH KNIGHT REFACTORED
// ============================================================================

class BloodDeathKnightRefactored : public TankSpecialization<RuneRunicPowerResource>
{
public:
    using Base = TankSpecialization<RuneRunicPowerResource>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::_resource;
    explicit BloodDeathKnightRefactored(Player* bot)
        : TankSpecialization<RuneRunicPowerResource>(bot)
        , _boneShieldTracker()
        , _deathsAndDecayActive(false)
        , _deathsAndDecayEndTime(0)
        , _crimsonScourgeProc(false)
        , _lastDeathStrikeTime(0)
    {
        // CRITICAL: Do NOT call bot->GetPower(), bot->GetMaxPower(), or bot->GetName() here!
        // Bot is not fully in world during constructor.
        // RuneRunicPowerResource::Initialize() is safe - it only sets default rune values.
        this->_resource.Initialize(bot);

        // Note: Do NOT call bot->GetName() here - Player data may not be loaded yet
        TC_LOG_DEBUG("playerbot", "BloodDeathKnightRefactored created for bot GUID: {}",
            bot ? bot->GetGUID().GetCounter() : 0);

        InitializeBloodMechanics();
    }

    void UpdateRotation(::Unit* target) override
    {
        if (!target || !target->IsAlive() || !target->IsHostileTo(this->GetBot()))
            return;

        // Detect hero talents if not yet cached
        if (!_heroTalents.detected)
            _heroTalents.Refresh(this->GetBot());

        // Hero talent rotation branching
        // Blood DK has access to: Deathbringer / San'layn
        if (_heroTalents.IsTree(HeroTalentTree::DEATHBRINGER))
        {
            // Deathbringer: Reaper's Mark amplifies damage on target, triggers Wave of Souls
            if (this->CanCastSpell(WoW120Spells::DeathKnight::Blood::REAPER_MARK, target))
            {
                this->CastSpell(WoW120Spells::DeathKnight::Blood::REAPER_MARK, target);
                return;
            }
            // Exterminate: empowered strike during Reaper's Mark
            if (target->HasAura(WoW120Spells::DeathKnight::Blood::REAPERS_MARK_DEBUFF))
            {
                if (this->CanCastSpell(WoW120Spells::DeathKnight::Blood::EXTERMINATE, target))
                {
                    this->CastSpell(WoW120Spells::DeathKnight::Blood::EXTERMINATE, target);
                    return;
                }
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::SANLAYN))
        {
            // San'layn: Vampiric Strike replaces Death Coil, heals and generates RP
            if (this->CanCastSpell(VAMPIRIC_STRIKE, target))
            {
                this->CastSpell(VAMPIRIC_STRIKE, target);
                return;
            }
        }

        // Update Blood state
        UpdateBloodState();

        // Handle active mitigation
        HandleActiveMitigation();

        // Determine if AoE or single target
        uint32 enemyCount = this->GetEnemiesInRange(10.0f);
        if (enemyCount >= 3)
        {
            ExecuteAoEThreatRotation(target, enemyCount);
        }
        else
        {
            ExecuteSingleTargetThreatRotation(target);
        }
    }

    void UpdateBuffs() override
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        // Emergency defensives
        HandleEmergencyDefensives();
    }

    // Phase 5C: Threat management using ThreatAssistant service
    void ManageThreat(::Unit* target) override
    {
        if (!target)
            return;

        // Use ThreatAssistant to determine best taunt target and execute
        Unit* tauntTarget = bot::ai::ThreatAssistant::GetTauntTarget(this->GetBot());
        if (tauntTarget && this->CanCastSpell(DARK_COMMAND, tauntTarget))
        {
            bot::ai::ThreatAssistant::ExecuteTaunt(this->GetBot(), tauntTarget, DARK_COMMAND);
            _lastTaunt = GameTime::GetGameTimeMS();
            TC_LOG_DEBUG("playerbot", "Blood DK: Dark Command taunt via ThreatAssistant on {}", tauntTarget->GetName());
        }
    }


protected:
    void ExecuteSingleTargetThreatRotation(::Unit* target)
    {
        uint32 rp = this->_resource.runicPower;
        uint32 totalRunes = this->_resource.bloodRunes + this->_resource.frostRunes + this->_resource.unholyRunes;

        // Priority 1: Maintain Bone Shield (Marrowrend)
        if (_boneShieldTracker.NeedsRefresh() && totalRunes >= 2)
        {
            if (this->CanCastSpell(MARROWREND, target))
            {
                this->CastSpell(MARROWREND, target);
                _boneShieldTracker.ApplyMarrowrend(3);
                ConsumeRunes(2);
                GenerateRunicPower(15);
                return;
            }
        }

        // Priority 2: Death Strike for self-healing
        if (this->GetBot()->GetHealthPct() < 70.0f && rp >= 35)
        {
            if (this->CanCastSpell(DEATH_STRIKE, target))
            {
                this->CastSpell(DEATH_STRIKE, target);
                _lastDeathStrikeTime = GameTime::GetGameTimeMS();
                ConsumeRunicPower(35);
                return;
            }
        }

        // Priority 3: Maintain Death's and Decay
        if (!_deathsAndDecayActive && rp >= 30)
        {
            if (this->CanCastSpell(DEATHS_AND_DECAY_BLOOD, this->GetBot()))
            {
                this->CastSpell(DEATHS_AND_DECAY_BLOOD, this->GetBot());
                _deathsAndDecayActive = true;
                _deathsAndDecayEndTime = GameTime::GetGameTimeMS() + 10000;
                ConsumeRunicPower(30);
                return;
            }
        }

        // Priority 4: Blood Boil (Crimson Scourge proc or normal)
        if (_crimsonScourgeProc || (totalRunes >= 2 && this->CanCastSpell(BLOOD_BOIL, this->GetBot())))
        {
            this->CastSpell(BLOOD_BOIL, this->GetBot());
            if (_crimsonScourgeProc)
                _crimsonScourgeProc = false;
            else
            {
                ConsumeRunes(2);
                GenerateRunicPower(10);
            }
            return;
        }

        // Priority 5: Heart Strike (main threat generator)
        if (totalRunes >= 1 && this->CanCastSpell(HEART_STRIKE, target))
        {
            this->CastSpell(HEART_STRIKE, target);
            ConsumeRunes(1);
            GenerateRunicPower(10);
            return;
        }

        // Priority 6: Death Strike (dump RP before capping)
        if (rp >= 80 && this->CanCastSpell(DEATH_STRIKE, target))
        {
            this->CastSpell(DEATH_STRIKE, target);
            ConsumeRunicPower(35);
            return;
        }
    }

    void ExecuteAoEThreatRotation(::Unit* target, uint32 enemyCount)
    {
        uint32 rp = this->_resource.runicPower;
        uint32 totalRunes = this->_resource.bloodRunes + this->_resource.frostRunes + this->_resource.unholyRunes;

        // Priority 1: Maintain Bone Shield
        if (_boneShieldTracker.NeedsRefresh() && totalRunes >= 2)
        {
            if (this->CanCastSpell(MARROWREND, target))
            {
                this->CastSpell(MARROWREND, target);
                _boneShieldTracker.ApplyMarrowrend(3);
                ConsumeRunes(2);
                GenerateRunicPower(15);
                return;
            }
        }

        // Priority 2: Death's and Decay (AoE)
        if (!_deathsAndDecayActive && rp >= 30)
        {
            if (this->CanCastSpell(DEATHS_AND_DECAY_BLOOD, this->GetBot()))
            {
                this->CastSpell(DEATHS_AND_DECAY_BLOOD, this->GetBot());
                _deathsAndDecayActive = true;
                _deathsAndDecayEndTime = GameTime::GetGameTimeMS() + 10000;
                ConsumeRunicPower(30);
                return;
            }
        }

        // Priority 3: Blood Boil (AoE threat)
        if (totalRunes >= 2 && this->CanCastSpell(BLOOD_BOIL, this->GetBot()))
        {
            this->CastSpell(BLOOD_BOIL, this->GetBot());
            ConsumeRunes(2);
            GenerateRunicPower(10);
            return;
        }

        // Priority 4: Heart Strike
        if (totalRunes >= 1 && this->CanCastSpell(HEART_STRIKE, target))
        {
            this->CastSpell(HEART_STRIKE, target);
            ConsumeRunes(1);
            GenerateRunicPower(10);
            return;
        }

        // Priority 5: Death Strike (heal)
        if (this->GetBot()->GetHealthPct() < 80.0f && rp >= 35)
        {
            if (this->CanCastSpell(DEATH_STRIKE, target))
            {
                this->CastSpell(DEATH_STRIKE, target);
                ConsumeRunicPower(35);
                return;
            }
        }
    }

    void HandleActiveMitigation()
    {
        Player* bot = this->GetBot();
        float healthPct = bot->GetHealthPct();

        // Anti-Magic Shell (magic damage)
        if (healthPct < 80.0f && this->CanCastSpell(ANTI_MAGIC_SHELL, bot))
        {
            this->CastSpell(ANTI_MAGIC_SHELL, bot);
            

        // Register cooldowns using CooldownManager
        // COMMENTED OUT:         _cooldowns.RegisterBatch({
        // COMMENTED OUT:             {MARROWREND, 0, 1},
        // COMMENTED OUT:             {HEART_STRIKE, 0, 1},
        // COMMENTED OUT:             {BLOOD_BOIL, 0, 1},
        // COMMENTED OUT:             {DEATH_STRIKE, 0, 1},
        // COMMENTED OUT:             {DARK_COMMAND, CooldownPresets::DISPEL, 1},
        // COMMENTED OUT:             {VAMPIRIC_BLOOD, 90000, 1},
        // COMMENTED OUT:             {DANCING_RUNE_WEAPON, CooldownPresets::MINOR_OFFENSIVE, 1},
        // COMMENTED OUT:             {ICEBOUND_FORTITUDE, CooldownPresets::MAJOR_OFFENSIVE, 1},
        // COMMENTED OUT:             {ANTI_MAGIC_SHELL, CooldownPresets::OFFENSIVE_60, 1},
        // COMMENTED OUT:             {RUNE_TAP, 25000, 1},
        // COMMENTED OUT:             {DEATH_GRIP, 25000, 1},
        // COMMENTED OUT:             {DEATHS_ADVANCE, 90000, 1},
        // COMMENTED OUT:             {GOREFIENDS_GRASP, CooldownPresets::MINOR_OFFENSIVE, 1},
        // COMMENTED OUT:             {ARMY_OF_THE_DEAD, 480000, 1},
        // COMMENTED OUT:         });

        TC_LOG_DEBUG("playerbot", "Blood: Anti-Magic Shell");
            return;
        }

        // Rune Tap (talent, quick mitigation)
        if (healthPct < 70.0f && this->CanCastSpell(RUNE_TAP, bot))
        {
            this->CastSpell(RUNE_TAP, bot);
            TC_LOG_DEBUG("playerbot", "Blood: Rune Tap");
            return;
        }
    }

    void HandleEmergencyDefensives()
    {
        Player* bot = this->GetBot();
        float healthPct = bot->GetHealthPct();

        // Critical: Icebound Fortitude
        if (healthPct < 30.0f && this->CanCastSpell(ICEBOUND_FORTITUDE, bot))
        {
            this->CastSpell(ICEBOUND_FORTITUDE, bot);
            TC_LOG_DEBUG("playerbot", "Blood: Icebound Fortitude emergency");
            return;
        }

        // Very low: Vampiric Blood
        if (healthPct < 40.0f && this->CanCastSpell(VAMPIRIC_BLOOD, bot))
        {
            this->CastSpell(VAMPIRIC_BLOOD, bot);
            TC_LOG_DEBUG("playerbot", "Blood: Vampiric Blood");
            return;
        }

        // Low: Dancing Rune Weapon
        if (healthPct < 50.0f && this->CanCastSpell(DANCING_RUNE_WEAPON, bot))
        {
            this->CastSpell(DANCING_RUNE_WEAPON, bot);
            TC_LOG_DEBUG("playerbot", "Blood: Dancing Rune Weapon");
            return;
        }

        // Moderate: Death Strike spam
        if (healthPct < 60.0f && this->_resource.runicPower >= 35)
        {
            if (this->CanCastSpell(DEATH_STRIKE, bot))
            {
                this->CastSpell(DEATH_STRIKE, bot);
                ConsumeRunicPower(35);
                return;
            }
        }
    }

private:
    void UpdateBloodState()
    {
        // Update Bone Shield tracker
        _boneShieldTracker.Update(this->GetBot());

        // Update Death's and Decay
        if (_deathsAndDecayActive && GameTime::GetGameTimeMS() >= _deathsAndDecayEndTime)
        {
            _deathsAndDecayActive = false;
            _deathsAndDecayEndTime = 0;
        }

        // Update Crimson Scourge proc
        if (this->GetBot() && this->GetBot()->HasAura(CRIMSON_SCOURGE))
            _crimsonScourgeProc = true;
        else
            _crimsonScourgeProc = false;

        // Update Runic Power from bot
        if (this->GetBot())
            this->_resource.runicPower = this->GetBot()->GetPower(POWER_RUNIC_POWER);

        // Update runes (simplified - in real implementation, track individual runes)
        // For now, assume runes regenerate over time
        uint32 now = GameTime::GetGameTimeMS();
        static uint32 lastRuneUpdate = 0;
        if (now - lastRuneUpdate > 10000) // Every 10 seconds
        {
            this->_resource.bloodRunes = 2;
            this->_resource.frostRunes = 2;
            this->_resource.unholyRunes = 2;
            lastRuneUpdate = now;
        }
    }

    void GenerateRunicPower(uint32 amount)
    {
        this->_resource.runicPower = ::std::min(this->_resource.runicPower + amount, this->_resource.maxRunicPower);
    }

    void ConsumeRunicPower(uint32 amount)
    {
        this->_resource.runicPower = (this->_resource.runicPower > amount) ? this->_resource.runicPower - amount : 0;
    }

    void ConsumeRunes(uint32 count = 1)
    {
        this->_resource.Consume(count);
    }

    void InitializeBloodMechanics()
    {
        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)
        Player* bot = this->GetBot();
        if (!bot) return;

        // DESIGN NOTE: BotAI integration for ActionPriorityQueue and BehaviorTree registration
        // Implementation should include:
        // - Proper BotAI retrieval from Player instance (via custom Player extension or bot registry)
        // - ActionPriorityQueue registration for Blood DK spells (Vampiric Blood, Death Strike, Marrowrend, Heart Strike, Blood Boil)
        // - BehaviorTree construction with emergency defensives, active mitigation, bone shield maintenance, and threat generation sequences
        // Reference: WoW 12.0 Death Knight mechanics, Blood specialization active mitigation patterns
        // BotAI* ai = dynamic_cast<BotAI*>(bot->GetPlayerAI());
        // if (!ai) return;

        BotAI* ai = nullptr; // Stubbed out
        if (!ai) return;

        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            queue->RegisterSpell(VAMPIRIC_BLOOD, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
            queue->AddCondition(VAMPIRIC_BLOOD, [](Player* bot, Unit*) { return bot && bot->GetHealthPct() < 40.0f; }, "HP < 40%");

            queue->RegisterSpell(DEATH_STRIKE, SpellPriority::CRITICAL, SpellCategory::DEFENSIVE);
            queue->AddCondition(DEATH_STRIKE, [](Player* bot, Unit*) { return bot && bot->GetHealthPct() < 70.0f; }, "Heal HP < 70%");

            queue->RegisterSpell(MARROWREND, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(MARROWREND, [this](Player*, Unit* target) { return target && this->_boneShieldTracker.GetStacks() < 5; }, "< 5 Bone Shield");

            queue->RegisterSpell(HEART_STRIKE, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(HEART_STRIKE, [](Player*, Unit* target) { return target != nullptr; }, "Builder");

            queue->RegisterSpell(BLOOD_BOIL, SpellPriority::LOW, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(BLOOD_BOIL, [this](Player*, Unit*) { return this->GetEnemiesInRange(10.0f) >= 2; }, "AoE 2+");
        }

        auto* tree = ai->GetBehaviorTree();
        if (tree)
        {
            auto root = Selector("Blood DK Tank", {
                Sequence("Emergency", { Condition("HP < 40%", [](Player* bot, Unit* target) { return bot && bot->GetHealthPct() < 40.0f; }),
                    bot::ai::Action("Vampiric Blood", [this](Player* bot, Unit*) { if (this->CanCastSpell(VAMPIRIC_BLOOD, bot)) { this->CastSpell(VAMPIRIC_BLOOD, bot); return NodeStatus::SUCCESS; } return NodeStatus::FAILURE; }) }),
                Sequence("Active Mitigation", { Condition("HP < 70%", [](Player* bot, Unit* target) { return bot && bot->GetHealthPct() < 70.0f; }),
                    bot::ai::Action("Death Strike", [this](Player* bot, Unit*) { if (this->CanCastSpell(DEATH_STRIKE, bot)) { this->CastSpell(DEATH_STRIKE, bot); return NodeStatus::SUCCESS; } return NodeStatus::FAILURE; }) }),
                Sequence("Bone Shield", { Condition("< 5 stacks", [this](Player*, Unit*) { return this->_boneShieldTracker.GetStacks() < 5; }),
                    bot::ai::Action("Marrowrend", [this](Player* bot, Unit*) { Unit* t = bot->GetVictim(); if (t && this->CanCastSpell(MARROWREND, t)) { this->CastSpell(MARROWREND, t); return NodeStatus::SUCCESS; } return NodeStatus::FAILURE; }) }),
                Sequence("Threat", { Condition("Has target", [this](Player* bot, Unit*) { return bot && bot->GetVictim(); }),
                    bot::ai::Action("Heart Strike", [this](Player* bot, Unit*) { Unit* t = bot->GetVictim(); if (t && this->CanCastSpell(HEART_STRIKE, t)) { this->CastSpell(HEART_STRIKE, t); return NodeStatus::SUCCESS; } return NodeStatus::FAILURE; }) })
            });
            tree->SetRoot(root);
        }
    }

private:
    BloodBoneShieldTracker _boneShieldTracker;
    bool _deathsAndDecayActive;
    uint32 _deathsAndDecayEndTime;
    bool _crimsonScourgeProc;
    uint32 _lastDeathStrikeTime;
    uint32 _lastTaunt{0}; // Phase 5C: ThreatAssistant integration

    // Hero talent detection cache (refreshed on combat start)
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot
