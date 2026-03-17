/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Frost Death Knight Refactored - Template-Based Implementation
 *
 * This file provides a complete, template-based implementation of Frost Death Knight
 * using the MeleeDpsSpecialization with dual resource system (Runes + Runic Power).
 */

#pragma once


#include "../Common/StatusEffectTracker.h"
#include "../Common/CooldownManager.h"
#include "../Common/RotationHelpers.h"
#include "../CombatSpecializationTemplates.h"
#include "../ResourceTypes.h"
#include "../SpellValidation_WoW120.h"  // Central spell registry
#include "../HeroTalentDetector.h"      // Hero talent tree detection
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
// FROST DEATH KNIGHT SPELL ALIASES (WoW 12.0 - The War Within)
// Consolidated spell IDs from central registry - NO duplicates
// ============================================================================

namespace FrostDeathKnightSpells
{
    // Rune Spenders
    constexpr uint32 OBLITERATE               = WoW120Spells::DeathKnight::Frost::OBLITERATE;
    constexpr uint32 HOWLING_BLAST            = WoW120Spells::DeathKnight::Frost::HOWLING_BLAST;
    constexpr uint32 REMORSELESS_WINTER       = WoW120Spells::DeathKnight::Frost::REMORSELESS_WINTER;
    constexpr uint32 GLACIAL_ADVANCE          = WoW120Spells::DeathKnight::Frost::GLACIAL_ADVANCE;
    constexpr uint32 FROSTSCYTHE              = WoW120Spells::DeathKnight::Frost::FROSTSCYTHE;

    // Runic Power Spenders
    constexpr uint32 FROST_STRIKE             = WoW120Spells::DeathKnight::Frost::FROST_STRIKE;
    constexpr uint32 HORN_OF_WINTER           = WoW120Spells::DeathKnight::Frost::HORN_OF_WINTER;

    // Cooldowns
    constexpr uint32 PILLAR_OF_FROST          = WoW120Spells::DeathKnight::Frost::PILLAR_OF_FROST;
    constexpr uint32 EMPOWER_RUNE_WEAPON      = WoW120Spells::DeathKnight::Frost::EMPOWER_RUNE_WEAPON;
    constexpr uint32 BREATH_OF_SINDRAGOSA     = WoW120Spells::DeathKnight::Frost::BREATH_OF_SINDRAGOSA;
    constexpr uint32 FROSTWYRMS_FURY          = WoW120Spells::DeathKnight::Frost::FROSTWYRMS_FURY;

    // Utility
    constexpr uint32 DEATH_GRIP_FROST         = WoW120Spells::DeathKnight::DEATH_GRIP;
    constexpr uint32 MIND_FREEZE_FROST        = WoW120Spells::DeathKnight::MIND_FREEZE;
    constexpr uint32 CHAINS_OF_ICE            = WoW120Spells::DeathKnight::CHAINS_OF_ICE;
    constexpr uint32 DARK_COMMAND_FROST       = WoW120Spells::DeathKnight::DARK_COMMAND;
    constexpr uint32 ANTI_MAGIC_SHELL_FROST   = WoW120Spells::DeathKnight::ANTI_MAGIC_SHELL;
    constexpr uint32 ICEBOUND_FORTITUDE_FROST = WoW120Spells::DeathKnight::ICEBOUND_FORTITUDE;
    constexpr uint32 DEATHS_ADVANCE_FROST     = WoW120Spells::DeathKnight::DEATHS_ADVANCE;

    // Procs and Buffs
    constexpr uint32 KILLING_MACHINE          = WoW120Spells::DeathKnight::Frost::KILLING_MACHINE;
    constexpr uint32 RIME                     = WoW120Spells::DeathKnight::Frost::RIME;
    constexpr uint32 RAZORICE                 = WoW120Spells::DeathKnight::Frost::RAZORICE_PROC;
    constexpr uint32 FROZEN_PULSE             = WoW120Spells::DeathKnight::Frost::FROZEN_PULSE;

    // Diseases
    constexpr uint32 FROST_FEVER_DK           = WoW120Spells::DeathKnight::Frost::FROST_FEVER;

    // Talents
    constexpr uint32 OBLITERATION             = WoW120Spells::DeathKnight::Frost::OBLITERATION;
    constexpr uint32 BREATH_OF_SINDRAGOSA_TALENT = BREATH_OF_SINDRAGOSA;
    constexpr uint32 GATHERING_STORM          = WoW120Spells::DeathKnight::Frost::GATHERING_STORM;
    constexpr uint32 ICECAP                   = WoW120Spells::DeathKnight::Frost::ICECAP;
    constexpr uint32 INEXORABLE_ASSAULT       = WoW120Spells::DeathKnight::Frost::INEXORABLE_ASSAULT;
    constexpr uint32 COLD_HEART               = WoW120Spells::DeathKnight::Frost::COLD_HEART;

    // Aliases with FROST_ prefix for RegisterSpell compatibility
    constexpr uint32 FROST_ICEBOUND_FORTITUDE = ICEBOUND_FORTITUDE_FROST;
    constexpr uint32 FROST_PILLAR_OF_FROST = PILLAR_OF_FROST;
    constexpr uint32 FROST_EMPOWER_RUNE_WEAPON = EMPOWER_RUNE_WEAPON;
    constexpr uint32 FROST_OBLITERATE = OBLITERATE;
    constexpr uint32 FROST_HOWLING_BLAST = HOWLING_BLAST;
    constexpr uint32 FROST_FROST_STRIKE = FROST_STRIKE;
    constexpr uint32 FROST_BREATH_OF_SINDRAGOSA = BREATH_OF_SINDRAGOSA;
    constexpr uint32 FROST_REMORSELESS_WINTER = REMORSELESS_WINTER;
    constexpr uint32 FROST_HORN_OF_WINTER = HORN_OF_WINTER;
}
using namespace FrostDeathKnightSpells;

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

// Dual resource type for Frost Death Knight (simplified runes)
struct FrostRuneRunicPowerResource
{
    uint32 bloodRunes{0};
    uint32 frostRunes{0};
    uint32 unholyRunes{0};
    uint32 runicPower{0};
    uint32 maxRunicPower{100};
    bool available{true};

    bool Consume(uint32 runesCost)
    {
        uint32 totalRunes = bloodRunes + frostRunes + unholyRunes;
        if (totalRunes >= runesCost) {
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
        // Runes regenerate over time
        static uint32 regenTimer = 0;
        regenTimer += diff;
        if (regenTimer >= 10000) { // 10 seconds per rune
            uint32 totalRunes = bloodRunes + frostRunes + unholyRunes;
            if (totalRunes < 6)
            {
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
        return 6; // Max 6 runes
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
// FROST KILLING MACHINE TRACKER
// ============================================================================

class FrostKillingMachineTracker
{
public:
    FrostKillingMachineTracker()
        : _kmActive(false)
        , _kmStacks(0)
    {
    }

    void ActivateProc(uint32 stacks = 1)
    {
        _kmActive = true;
        _kmStacks = stacks;
    }

    void ConsumeProc()
    {
        if (_kmStacks > 0)
            _kmStacks--;

        if (_kmStacks == 0)
            _kmActive = false;
    }

    bool IsActive() const { return _kmActive; }
    uint32 GetStacks() const { return _kmStacks; }

    void Update(Player* bot)
    {
        if (!bot)
            return;

        if (Aura* aura = bot->GetAura(KILLING_MACHINE))
        {
            _kmActive = true;
            _kmStacks = aura->GetStackAmount();        }
        else
        {
            _kmActive = false;
            _kmStacks = 0;
        }
    }

private:
    CooldownManager _cooldowns;
    bool _kmActive;
    uint32 _kmStacks;
};

// ============================================================================
// FROST RIME TRACKER
// ============================================================================

class FrostRimeTracker
{
public:
    FrostRimeTracker()
        : _rimeActive(false)
    {
    }

    void ActivateProc()
    {
        _rimeActive = true;
    }

    void ConsumeProc()
    {
        _rimeActive = false;
    }

    bool IsActive() const { return _rimeActive; }

    void Update(Player* bot)
    {
        if (!bot)
            return;

        _rimeActive = bot->HasAura(RIME);
    }

private:
    bool _rimeActive;
};

// ============================================================================
// FROST DEATH KNIGHT REFACTORED
// ============================================================================

class FrostDeathKnightRefactored : public MeleeDpsSpecialization<FrostRuneRunicPowerResource>
{
public:
    using Base = MeleeDpsSpecialization<FrostRuneRunicPowerResource>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::_resource;
    explicit FrostDeathKnightRefactored(Player* bot)
        : MeleeDpsSpecialization<FrostRuneRunicPowerResource>(bot)
        , _kmTracker()
        , _rimeTracker()
        , _pillarOfFrostActive(false)
        , _pillarEndTime(0)
        , _breathOfSindragosaActive(false)
        , _lastRemorselessWinterTime(0)
    {
        // CRITICAL: Do NOT call bot->GetPower(), bot->GetMaxPower(), or bot->GetName() here!
        // Bot is not fully in world during constructor.
        // FrostRuneRunicPowerResource::Initialize() is safe - it only sets default rune values.
        this->_resource.Initialize(bot);

        // Note: Do NOT call bot->GetName() here - Player data may not be loaded yet
        TC_LOG_DEBUG("playerbot", "FrostDeathKnightRefactored created for bot GUID: {}",
            bot ? bot->GetGUID().GetCounter() : 0);

        InitializeFrostMechanics();
    }

    void UpdateRotation(::Unit* target) override
    {
        if (!target || !target->IsAlive() || !target->IsHostileTo(this->GetBot()))
            return;

        // Detect hero talents if not yet cached
        if (!_heroTalents.detected)
            _heroTalents.Refresh(this->GetBot());

        // Hero talent rotation branching
        // Frost DK has access to: Rider of the Apocalypse / Deathbringer
        if (_heroTalents.IsTree(HeroTalentTree::RIDER_OF_THE_APOCALYPSE))
        {
            // Rider: Death Charge gap-closer and empowered Rider's Champion strikes
            if (this->CanCastSpell(WoW120Spells::DeathKnight::Frost::DEATH_CHARGE, target))
            {
                this->CastSpell(WoW120Spells::DeathKnight::Frost::DEATH_CHARGE, target);
                return;
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::DEATHBRINGER))
        {
            // Deathbringer: Reaper's Mark on target triggers Wave of Souls
            if (this->CanCastSpell(WoW120Spells::DeathKnight::Frost::FROST_REAPER_MARK, target))
            {
                this->CastSpell(WoW120Spells::DeathKnight::Frost::FROST_REAPER_MARK, target);
                return;
            }
            // Exterminate during Reaper's Mark for bonus damage
            if (target->HasAura(WoW120Spells::DeathKnight::Blood::REAPERS_MARK_DEBUFF))
            {
                if (this->CanCastSpell(WoW120Spells::DeathKnight::Frost::FROST_EXTERMINATE, target))
                {
                    this->CastSpell(WoW120Spells::DeathKnight::Frost::FROST_EXTERMINATE, target);
                    return;
                }
            }
        }

        // Update Frost state
        UpdateFrostState();

        // Use major cooldowns
        HandleCooldowns();

        // Determine if AoE or single target
        uint32 enemyCount = this->GetEnemiesInRange(10.0f);
        if (enemyCount >= 3)
        {
            ExecuteAoERotation(target, enemyCount);
        }
        else
        {
            ExecuteSingleTargetRotation(target);
        }
    }

    void UpdateBuffs() override
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        // Defensive cooldowns
        HandleDefensiveCooldowns();
    }


protected:
    void ExecuteSingleTargetRotation(::Unit* target)
    {
        uint32 rp = this->_resource.runicPower;
        uint32 totalRunes = this->_resource.bloodRunes + this->_resource.frostRunes + this->_resource.unholyRunes;

        // Priority 1: Breath of Sindragosa (if talented and high RP)
        if (_breathOfSindragosaActive)
        {
            // During Breath, spam Obliterate and Frost Strike to maintain
            if (rp < 20 && totalRunes >= 2 && this->CanCastSpell(OBLITERATE, target))
            {
                this->CastSpell(OBLITERATE, target);
                ConsumeRunes(2);
                GenerateRunicPower(15);
                return;
            }

            if (rp >= 25 && this->CanCastSpell(FROST_STRIKE, target))
            {
                this->CastSpell(FROST_STRIKE, target);
                ConsumeRunicPower(25);
                return;
            }
        }

        // Priority 2: Use Rime proc (free Howling Blast)
        if (_rimeTracker.IsActive() && this->CanCastSpell(HOWLING_BLAST, target))
        {
            this->CastSpell(HOWLING_BLAST, target);
            _rimeTracker.ConsumeProc();
            return;
        }

        // Priority 3: Obliterate with Killing Machine proc
        if (_kmTracker.IsActive() && totalRunes >= 2 && this->CanCastSpell(OBLITERATE, target))
        {
            this->CastSpell(OBLITERATE, target);
            _kmTracker.ConsumeProc();
            ConsumeRunes(2);
            GenerateRunicPower(15);
            return;
        }

        // Priority 4: Remorseless Winter (AoE slow)
        if (totalRunes >= 1 && this->CanCastSpell(REMORSELESS_WINTER, this->GetBot()))
        {
            this->CastSpell(REMORSELESS_WINTER, this->GetBot());
            _lastRemorselessWinterTime = GameTime::GetGameTimeMS();
            ConsumeRunes(1);
            return;
        }

        // Priority 5: Frost Strike (dump RP)
        if (rp >= 50 && this->CanCastSpell(FROST_STRIKE, target))
        {
            this->CastSpell(FROST_STRIKE, target);
            ConsumeRunicPower(25);
            return;
        }

        // Priority 6: Obliterate (main rune spender)
        if (totalRunes >= 2 && this->CanCastSpell(OBLITERATE, target))
        {
            this->CastSpell(OBLITERATE, target);
            ConsumeRunes(2);
            GenerateRunicPower(15);
            return;
        }

        // Priority 7: Frost Strike (prevent RP capping)
        if (rp >= 25 && this->CanCastSpell(FROST_STRIKE, target))
        {
            this->CastSpell(FROST_STRIKE, target);
            ConsumeRunicPower(25);
            return;
        }

        // Priority 8: Horn of Winter (talent, generate resources)
        if (totalRunes < 3 && rp < 70 && this->CanCastSpell(HORN_OF_WINTER, this->GetBot()))
        {
            this->CastSpell(HORN_OF_WINTER, this->GetBot());
            GenerateRunicPower(25);
            return;
        }
    }

    void ExecuteAoERotation(::Unit* target, uint32 enemyCount)
    {
        uint32 rp = this->_resource.runicPower;
        uint32 totalRunes = this->_resource.bloodRunes + this->_resource.frostRunes + this->_resource.unholyRunes;

        // Priority 1: Remorseless Winter
        if (totalRunes >= 1 && this->CanCastSpell(REMORSELESS_WINTER, this->GetBot()))
        {
            this->CastSpell(REMORSELESS_WINTER, this->GetBot());
            _lastRemorselessWinterTime = GameTime::GetGameTimeMS();
            ConsumeRunes(1);
            return;
        }

        // Priority 2: Howling Blast (AoE)
        if (totalRunes >= 1 && this->CanCastSpell(HOWLING_BLAST, target))
        {
            this->CastSpell(HOWLING_BLAST, target);
            ConsumeRunes(1);
            GenerateRunicPower(10);
            return;
        }

        // Priority 3: Frostscythe (talent, AoE cleave)
        if (totalRunes >= 2 && this->CanCastSpell(FROSTSCYTHE, target))
        {
            this->CastSpell(FROSTSCYTHE, target);
            ConsumeRunes(2);
            GenerateRunicPower(15);
            return;
        }

        // Priority 4: Glacial Advance (talent, ranged AoE)
        if (totalRunes >= 2 && this->CanCastSpell(GLACIAL_ADVANCE, target))
        {
            this->CastSpell(GLACIAL_ADVANCE, target);
            ConsumeRunes(2);
            return;
        }

        // Priority 5: Frost Strike (dump RP)
        if (rp >= 25 && this->CanCastSpell(FROST_STRIKE, target))
        {
            this->CastSpell(FROST_STRIKE, target);
            ConsumeRunicPower(25);
            return;
        }

        // Priority 6: Obliterate (if no AoE available)
        if (totalRunes >= 2 && this->CanCastSpell(OBLITERATE, target))
        {
            this->CastSpell(OBLITERATE, target);
            ConsumeRunes(2);
            GenerateRunicPower(15);
            return;
        }
    }

    void HandleCooldowns()
    {
        Player* bot = this->GetBot();
        uint32 rp = this->_resource.runicPower;
        uint32 totalRunes = this->_resource.bloodRunes + this->_resource.frostRunes + this->_resource.unholyRunes;

        // Pillar of Frost (major damage CD)
        if (totalRunes >= 3 && this->CanCastSpell(PILLAR_OF_FROST, bot))
        {
            this->CastSpell(PILLAR_OF_FROST, bot);
            _pillarOfFrostActive = true;
            _pillarEndTime = GameTime::GetGameTimeMS() + 12000; // 12 sec duration
            

        // Register cooldowns using CooldownManager
        // COMMENTED OUT:         _cooldowns.RegisterBatch({
        // COMMENTED OUT:             {OBLITERATE, 0, 1},
        // COMMENTED OUT:             {FROST_STRIKE, 0, 1},
        // COMMENTED OUT:             {HOWLING_BLAST, 0, 1},
        // COMMENTED OUT:             {REMORSELESS_WINTER, 20000, 1},
        // COMMENTED OUT:             {PILLAR_OF_FROST, CooldownPresets::OFFENSIVE_60, 1},
        // COMMENTED OUT:             {EMPOWER_RUNE_WEAPON, CooldownPresets::MINOR_OFFENSIVE, 1},
        // COMMENTED OUT:             {BREATH_OF_SINDRAGOSA, CooldownPresets::MINOR_OFFENSIVE, 1},
        // COMMENTED OUT:             {FROSTWYRMS_FURY, CooldownPresets::MAJOR_OFFENSIVE, 1},
        // COMMENTED OUT:             {DEATH_GRIP_FROST, 25000, 1},
        // COMMENTED OUT:             {ANTI_MAGIC_SHELL_FROST, CooldownPresets::OFFENSIVE_60, 1},
        // COMMENTED OUT:             {ICEBOUND_FORTITUDE_FROST, CooldownPresets::MAJOR_OFFENSIVE, 1},
        // COMMENTED OUT:             {DEATHS_ADVANCE_FROST, 90000, 1},
        // COMMENTED OUT:         });

        TC_LOG_DEBUG("playerbot", "Frost: Pillar of Frost activated");
        }

        // Empower Rune Weapon (rune refresh)
        if (totalRunes == 0 && this->CanCastSpell(EMPOWER_RUNE_WEAPON, bot))
        {
            this->CastSpell(EMPOWER_RUNE_WEAPON, bot);
            this->_resource.bloodRunes = 2;
            this->_resource.frostRunes = 2;
            this->_resource.unholyRunes = 2;
            GenerateRunicPower(25);
            TC_LOG_DEBUG("playerbot", "Frost: Empower Rune Weapon");
        }

        // Breath of Sindragosa (talent, channel burst)
        if (rp >= 60 && this->CanCastSpell(BREATH_OF_SINDRAGOSA, bot))
        {
            this->CastSpell(BREATH_OF_SINDRAGOSA, bot);
            _breathOfSindragosaActive = true;
            TC_LOG_DEBUG("playerbot", "Frost: Breath of Sindragosa");
        }

        // Frostwyrm's Fury (AoE burst)
        if (this->CanCastSpell(FROSTWYRMS_FURY, bot))
        {
            this->CastSpell(FROSTWYRMS_FURY, bot);
            TC_LOG_DEBUG("playerbot", "Frost: Frostwyrm's Fury");
        }
    }

    void HandleDefensiveCooldowns()
    {
        Player* bot = this->GetBot();
        float healthPct = bot->GetHealthPct();

        // Icebound Fortitude
        if (healthPct < 40.0f && this->CanCastSpell(ICEBOUND_FORTITUDE_FROST, bot))
        {
            this->CastSpell(ICEBOUND_FORTITUDE_FROST, bot);
            TC_LOG_DEBUG("playerbot", "Frost: Icebound Fortitude");
            return;
        }

        // Anti-Magic Shell
        if (healthPct < 60.0f && this->CanCastSpell(ANTI_MAGIC_SHELL_FROST, bot))
        {
            this->CastSpell(ANTI_MAGIC_SHELL_FROST, bot);
            TC_LOG_DEBUG("playerbot", "Frost: Anti-Magic Shell");
            return;
        }

        // Death's Advance
        if (healthPct < 70.0f && this->CanCastSpell(DEATHS_ADVANCE_FROST, bot))
        {
            this->CastSpell(DEATHS_ADVANCE_FROST, bot);
            TC_LOG_DEBUG("playerbot", "Frost: Death's Advance");
            return;
        }
    }

private:
    void UpdateFrostState()
    {
        // Update Killing Machine tracker
        _kmTracker.Update(this->GetBot());

        // Update Rime tracker
        _rimeTracker.Update(this->GetBot());

        // Update Pillar of Frost
        if (_pillarOfFrostActive && GameTime::GetGameTimeMS() >= _pillarEndTime)
        {
            _pillarOfFrostActive = false;
            _pillarEndTime = 0;
        }

        // Update Breath of Sindragosa
        if (_breathOfSindragosaActive)
        {
            if (!this->GetBot() || !this->GetBot()->HasAura(BREATH_OF_SINDRAGOSA))
                _breathOfSindragosaActive = false;
        }

        // Update Runic Power from bot
        if (this->GetBot())
            this->_resource.runicPower = this->GetBot()->GetPower(POWER_RUNIC_POWER);

        // Update runes (simplified)
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

    void InitializeFrostMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)
        // DESIGN NOTE: BotAI integration for ActionPriorityQueue and BehaviorTree registration
        // Implementation should include:
        // - Proper BotAI retrieval from Player instance (via custom Player extension or bot registry)
        // - ActionPriorityQueue registration for Frost DK spells (Icebound Fortitude, Pillar of Frost, Empower Rune Weapon, Obliterate, Howling Blast, Frost Strike, Breath of Sindragosa, Remorseless Winter, Horn of Winter)
        // - BehaviorTree construction with burst cooldowns, priority procs (Killing Machine/Rime), runic power/rune spenders
        // Reference: WoW 12.0 Death Knight mechanics, Frost specialization burst windows and proc management
        // Commenting out for now until BotAI integration is implemented
        /*
        BotAI* ai = this->GetBot()->GetBotAI();
        if (!ai) return;

        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            // EMERGENCY: Defensive cooldowns
            queue->RegisterSpell(FROST_ICEBOUND_FORTITUDE, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
            queue->AddCondition(FROST_ICEBOUND_FORTITUDE, [](Player* bot, Unit*) {
                return bot && bot->GetHealthPct() < 35.0f;
            }, "HP < 35% (damage reduction)");

            // CRITICAL: Major burst cooldowns
            queue->RegisterSpell(FROST_PILLAR_OF_FROST, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(FROST_PILLAR_OF_FROST, [this](Player*, Unit* target) {
                return target && !this->_pillarOfFrostActive;
            }, "Major burst CD (12s, Str buff)");

            queue->RegisterSpell(FROST_EMPOWER_RUNE_WEAPON, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(FROST_EMPOWER_RUNE_WEAPON, [this](Player*, Unit* target) {
                return target && this->_resource.GetAvailable() < 3;
            }, "< 3 runes (instant refresh)");

            // HIGH: Priority damage abilities
            queue->RegisterSpell(FROST_OBLITERATE, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(FROST_OBLITERATE, [this](Player*, Unit* target) {
                return target && (this->_kmTracker.IsActive() || this->_resource.GetAvailable() >= 2);
            }, "KM proc or 2 runes (heavy damage)");

            queue->RegisterSpell(FROST_HOWLING_BLAST, SpellPriority::HIGH, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(FROST_HOWLING_BLAST, [this](Player*, Unit* target) {
                return target && (this->_rimeTracker.IsActive() || this->GetEnemiesInRange(10.0f) >= 3);
            }, "Rime proc or 3+ enemies");

            queue->RegisterSpell(FROST_FROST_STRIKE, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(FROST_FROST_STRIKE, [this](Player*, Unit* target) {
                return target && this->_resource.runicPower >= 25;
            }, "25 RP (spender)");

            // MEDIUM: Cooldowns & utility
            queue->RegisterSpell(FROST_BREATH_OF_SINDRAGOSA, SpellPriority::MEDIUM, SpellCategory::OFFENSIVE);
            queue->AddCondition(FROST_BREATH_OF_SINDRAGOSA, [this](Player* bot, Unit* target) {
                return bot && target && bot->HasSpell(FROST_BREATH_OF_SINDRAGOSA) &&
                       this->_resource.runicPower >= 50 && !this->_breathOfSindragosaActive;
            }, "50 RP, talent (channeled burst)");

            queue->RegisterSpell(FROST_REMORSELESS_WINTER, SpellPriority::MEDIUM, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(FROST_REMORSELESS_WINTER, [this](Player*, Unit*) {
                return this->GetEnemiesInRange(8.0f) >= 2;
            }, "2+ enemies (AoE damage)");

            queue->RegisterSpell(FROST_HORN_OF_WINTER, SpellPriority::MEDIUM, SpellCategory::UTILITY);
            queue->AddCondition(FROST_HORN_OF_WINTER, [this](Player*, Unit*) {
                return this->_resource.GetAvailable() < 3 && this->_resource.runicPower < 60;
            }, "< 3 runes, < 60 RP (resource gen)");

            // LOW: Filler
            queue->RegisterSpell(FROST_FROST_STRIKE, SpellPriority::LOW, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(FROST_FROST_STRIKE, [this](Player*, Unit* target) {
                return target && this->_resource.runicPower >= 25;
            }, "Dump RP");
        }

        auto* behaviorTree = ai->GetBehaviorTree();
        if (behaviorTree)
        {
            auto root = Selector("Frost Death Knight DPS", {
                // Tier 1: Burst Cooldowns (Pillar of Frost)
                Sequence("Burst Cooldowns", {
                    Condition("Has target", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim();
                    }),
                    Selector("Use burst", {
                        Sequence("Pillar of Frost", {
                            Condition("Not active", [this](Player*, Unit*) {
                                return !this->_pillarOfFrostActive;
                            }),
                            bot::ai::Action("Cast Pillar", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(FROST_PILLAR_OF_FROST, bot))
                                {
                                    this->CastSpell(FROST_PILLAR_OF_FROST, bot);
                                    this->_pillarOfFrostActive = true;
                                    this->_pillarEndTime = GameTime::GetGameTimeMS() + 12000;
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Empower Rune Weapon", {
                            Condition("< 3 runes", [this](Player*, Unit*) {
                                return this->_resource.GetAvailable() < 3;
                            }),
                            bot::ai::Action("Cast ERW", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(FROST_EMPOWER_RUNE_WEAPON, bot))
                                {
                                    this->CastSpell(FROST_EMPOWER_RUNE_WEAPON, bot);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 2: Priority Abilities (KM Obliterate, Rime Howling Blast)
                Sequence("Priority Procs", {
                    Condition("Has target", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim();
                    }),
                    Selector("Use procs", {
                        Sequence("KM Obliterate", {
                            Condition("KM active and 2 runes", [this](Player*, Unit*) {
                                return this->_kmTracker.IsActive() && this->_resource.GetAvailable() >= 2;
                            }),
                            bot::ai::Action("Cast Obliterate", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(FROST_OBLITERATE, target))
                                {
                                    this->CastSpell(FROST_OBLITERATE, target);
                                    this->GenerateRunicPower(20);
                                    if (this->_kmTracker.IsActive())
                                        this->_kmTracker.ConsumeProc();
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Rime Howling Blast", {
                            Condition("Rime active", [this](Player*, Unit*) {
                                return this->_rimeTracker.IsActive();
                            }),
                            bot::ai::Action("Cast Howling Blast", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(FROST_HOWLING_BLAST, target))
                                {
                                    this->CastSpell(FROST_HOWLING_BLAST, target);
                                    if (this->_rimeTracker.IsActive())
                                        this->_rimeTracker.ConsumeProc();
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 3: Runic Power Spender (Frost Strike)
                Sequence("RP Spender", {
                    Condition("25+ RP and target", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim() && this->_resource.runicPower >= 25;
                    }),
                    bot::ai::Action("Cast Frost Strike", [this](Player* bot, Unit*) {
                        Unit* target = bot->GetVictim();
                        if (target && this->CanCastSpell(FROST_FROST_STRIKE, target))
                        {
                            this->CastSpell(FROST_FROST_STRIKE, target);
                            this->ConsumeRunicPower(25);
                            return NodeStatus::SUCCESS;
                        }
                        return NodeStatus::FAILURE;
                    })
                }),

                // Tier 4: Rune Spender (Obliterate builder)
                Sequence("Rune Spender", {
                    Condition("2+ runes and target", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim() && this->_resource.GetAvailable() >= 2;
                    }),
                    Selector("Spend runes", {
                        Sequence("Howling Blast (AoE)", {
                            Condition("3+ enemies", [this](Player*, Unit*) {
                                return this->GetEnemiesInRange(10.0f) >= 3;
                            }),
                            bot::ai::Action("Cast Howling Blast", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(FROST_HOWLING_BLAST, target))
                                {
                                    this->CastSpell(FROST_HOWLING_BLAST, target);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Obliterate (ST)", {
                            bot::ai::Action("Cast Obliterate", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(FROST_OBLITERATE, target))
                                {
                                    this->CastSpell(FROST_OBLITERATE, target);
                                    this->GenerateRunicPower(20);
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
        */
    }

private:
    FrostKillingMachineTracker _kmTracker;
    FrostRimeTracker _rimeTracker;
    bool _pillarOfFrostActive;
    uint32 _pillarEndTime;
    bool _breathOfSindragosaActive;
    uint32 _lastRemorselessWinterTime;

    // Hero talent detection cache (refreshed on combat start)
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot
