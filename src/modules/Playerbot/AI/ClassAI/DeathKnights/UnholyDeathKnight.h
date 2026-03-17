/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Unholy Death Knight Refactored - Template-Based Implementation
 *
 * This file provides a complete, template-based implementation of Unholy Death Knight
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
#include "Pet.h"
#include "Log.h"
#include "../../Decision/ActionPriorityQueue.h"
#include "../../Decision/BehaviorTree.h"
#include "../BotAI.h"

namespace Playerbot
{

// ============================================================================
// UNHOLY DEATH KNIGHT SPELL ALIASES (WoW 12.0 - The War Within)
// Consolidated spell IDs from central registry - NO duplicates
// ============================================================================

namespace UnholyDeathKnightSpells
{
    // Rune Spenders
    constexpr uint32 FESTERING_STRIKE         = WoW120Spells::DeathKnight::Unholy::FESTERING_STRIKE;
    constexpr uint32 SCOURGE_STRIKE           = WoW120Spells::DeathKnight::Unholy::SCOURGE_STRIKE;
    constexpr uint32 CLAWING_SHADOWS          = WoW120Spells::DeathKnight::Unholy::CLAWING_SHADOWS;
    constexpr uint32 EPIDEMIC                 = WoW120Spells::DeathKnight::Unholy::EPIDEMIC;
    constexpr uint32 DEFILE                   = WoW120Spells::DeathKnight::Unholy::DEFILE;

    // Runic Power Spenders
    constexpr uint32 DEATH_COIL               = WoW120Spells::DeathKnight::DEATH_COIL;
    constexpr uint32 DARK_TRANSFORMATION      = WoW120Spells::DeathKnight::Unholy::DARK_TRANSFORMATION;

    // Diseases
    constexpr uint32 VIRULENT_PLAGUE          = WoW120Spells::DeathKnight::Unholy::VIRULENT_PLAGUE;
    constexpr uint32 OUTBREAK                 = WoW120Spells::DeathKnight::Unholy::OUTBREAK;

    // Pet Management
    constexpr uint32 RAISE_DEAD_UNHOLY        = WoW120Spells::DeathKnight::RAISE_DEAD;
    constexpr uint32 SUMMON_GARGOYLE          = WoW120Spells::DeathKnight::Unholy::SUMMON_GARGOYLE;
    constexpr uint32 ARMY_OF_THE_DEAD_UNHOLY  = WoW120Spells::DeathKnight::Unholy::ARMY_OF_THE_DEAD;
    constexpr uint32 APOCALYPSE               = WoW120Spells::DeathKnight::Unholy::APOCALYPSE;
    constexpr uint32 RAISE_ABOMINATION        = WoW120Spells::DeathKnight::Unholy::RAISE_ABOMINATION;

    // Major Cooldowns
    constexpr uint32 UNHOLY_ASSAULT           = WoW120Spells::DeathKnight::Unholy::UNHOLY_ASSAULT;
    constexpr uint32 UNHOLY_BLIGHT            = WoW120Spells::DeathKnight::Unholy::UNHOLY_BLIGHT;
    constexpr uint32 SOUL_REAPER              = WoW120Spells::DeathKnight::Unholy::SOUL_REAPER;

    // Utility
    constexpr uint32 DEATH_GRIP_UNHOLY        = WoW120Spells::DeathKnight::DEATH_GRIP;
    constexpr uint32 MIND_FREEZE_UNHOLY       = WoW120Spells::DeathKnight::MIND_FREEZE;
    constexpr uint32 CHAINS_OF_ICE_UNHOLY     = WoW120Spells::DeathKnight::CHAINS_OF_ICE;
    constexpr uint32 DARK_COMMAND_UNHOLY      = WoW120Spells::DeathKnight::DARK_COMMAND;
    constexpr uint32 ANTI_MAGIC_SHELL_UNHOLY  = WoW120Spells::DeathKnight::ANTI_MAGIC_SHELL;
    constexpr uint32 ICEBOUND_FORTITUDE_UNHOLY = WoW120Spells::DeathKnight::ICEBOUND_FORTITUDE;
    constexpr uint32 DEATHS_ADVANCE_UNHOLY    = WoW120Spells::DeathKnight::DEATHS_ADVANCE;
    constexpr uint32 CONTROL_UNDEAD_UNHOLY    = WoW120Spells::DeathKnight::CONTROL_UNDEAD;
    constexpr uint32 RAISE_ALLY_UNHOLY        = WoW120Spells::DeathKnight::RAISE_ALLY;

    // Procs and Buffs
    constexpr uint32 SUDDEN_DOOM              = WoW120Spells::DeathKnight::Unholy::SUDDEN_DOOM;
    constexpr uint32 RUNIC_CORRUPTION         = WoW120Spells::DeathKnight::Unholy::RUNIC_CORRUPTION;
    constexpr uint32 FESTERING_WOUND          = WoW120Spells::DeathKnight::Unholy::FESTERING_WOUND;
    constexpr uint32 UNHOLY_STRENGTH          = WoW120Spells::DeathKnight::Unholy::UNHOLY_STRENGTH;

    // Talents
    constexpr uint32 BURSTING_SORES           = WoW120Spells::DeathKnight::Unholy::BURSTING_SORES;
    constexpr uint32 INFECTED_CLAWS           = WoW120Spells::DeathKnight::Unholy::INFECTED_CLAWS;
    constexpr uint32 ALL_WILL_SERVE           = WoW120Spells::DeathKnight::Unholy::ALL_WILL_SERVE;
    constexpr uint32 UNHOLY_PACT              = WoW120Spells::DeathKnight::Unholy::UNHOLY_PACT;
    constexpr uint32 SUPERSTRAIN              = WoW120Spells::DeathKnight::Unholy::SUPERSTRAIN;

    // Aliases with UNHOLY_ prefix for RegisterSpell compatibility
    constexpr uint32 UNHOLY_ANTIMAGIC_SHELL = ANTI_MAGIC_SHELL_UNHOLY;
    constexpr uint32 UNHOLY_ARMY_OF_DEAD = ARMY_OF_THE_DEAD_UNHOLY;
    constexpr uint32 UNHOLY_APOCALYPSE = APOCALYPSE;
    constexpr uint32 UNHOLY_FESTERING_STRIKE = FESTERING_STRIKE;
    constexpr uint32 UNHOLY_SCOURGE_STRIKE = SCOURGE_STRIKE;
    constexpr uint32 UNHOLY_DEATH_COIL = DEATH_COIL;
}
using namespace UnholyDeathKnightSpells;

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

// Dual resource type for Unholy Death Knight
struct UnholyRuneRunicPowerResource
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
        bloodRunes = 2;
        frostRunes = 2;
        unholyRunes = 2;
        runicPower = 0;
    }
};

// ============================================================================
// UNHOLY FESTERING WOUND TRACKER
// ============================================================================

class UnholyFesteringWoundTracker
{
public:
    UnholyFesteringWoundTracker()
        : _trackedTargets()
    {
    }

    void ApplyWounds(ObjectGuid guid, uint32 count)
    {
        _trackedTargets[guid] = ::std::min(_trackedTargets[guid] + count, 6u); // Max 6 stacks
    }

    void BurstWounds(ObjectGuid guid, uint32 count)
    {
        if (_trackedTargets[guid] > count)
            _trackedTargets[guid] -= count;
        else
            _trackedTargets.erase(guid);
    }

    uint32 GetWoundCount(ObjectGuid guid) const
    {
        auto it = _trackedTargets.find(guid);
        return (it != _trackedTargets.end()) ? it->second : 0;
    }

    void Update(Unit* target)
    {
        if (!target)
            return;

        ObjectGuid guid = target->GetGUID();        

        // Sync with actual aura
        if (Aura* aura = target->GetAura(FESTERING_WOUND))
            _trackedTargets[guid] = aura->GetStackAmount();
            else
            _trackedTargets.erase(guid);
    }

private:
    CooldownManager _cooldowns;
    ::std::unordered_map<ObjectGuid, uint32> _trackedTargets;
};

// ============================================================================
// UNHOLY PET TRACKER
// ============================================================================

class UnholyPetTracker
{
public:
    UnholyPetTracker()
        : _ghoulActive(false)
        , _gargoyleActive(false)
        , _gargoyleEndTime(0)
        , _darkTransformationActive(false)
        , _darkTransformationEndTime(0)
    {
    }

    void SummonGhoul() { _ghoulActive = true; }
    bool IsGhoulActive() const { return _ghoulActive; }

    void SummonGargoyle()
    {
        _gargoyleActive = true;
        _gargoyleEndTime = GameTime::GetGameTimeMS() + 30000; // 30 sec duration
    }

    bool IsGargoyleActive() const { return _gargoyleActive; }

    void ActivateDarkTransformation()
    {
        _darkTransformationActive = true;
        _darkTransformationEndTime = GameTime::GetGameTimeMS() + 15000; // 15 sec duration
    }

    bool IsDarkTransformationActive() const { return _darkTransformationActive; }

    void Update(Player* bot)
    {
        if (!bot)
            return;

        // Update ghoul status
        Pet* pet = bot->GetPet();
        _ghoulActive = (pet && pet->IsAlive());

        // Update gargoyle
        uint32 now = GameTime::GetGameTimeMS();
        if (_gargoyleActive && now >= _gargoyleEndTime)
        {
            _gargoyleActive = false;
            _gargoyleEndTime = 0;
        }

        // Update Dark Transformation
        if (_darkTransformationActive && now >= _darkTransformationEndTime)
        {
            _darkTransformationActive = false;
            _darkTransformationEndTime = 0;
        }
    }

private:
    bool _ghoulActive;
    bool _gargoyleActive;
    uint32 _gargoyleEndTime;
    bool _darkTransformationActive;
    uint32 _darkTransformationEndTime;
};

// ============================================================================
// UNHOLY DEATH KNIGHT REFACTORED
// ============================================================================

class UnholyDeathKnightRefactored : public MeleeDpsSpecialization<UnholyRuneRunicPowerResource>
{
public:
    using Base = MeleeDpsSpecialization<UnholyRuneRunicPowerResource>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::_resource;
    explicit UnholyDeathKnightRefactored(Player* bot)
        : MeleeDpsSpecialization<UnholyRuneRunicPowerResource>(bot)
        , _woundTracker()
        , _petTracker()
        , _suddenDoomProc(false)
        , _lastOutbreakTime(0)
    {
        // CRITICAL: Do NOT call bot->GetPower(), bot->GetMaxPower(), or bot->GetName() here!
        // Bot is not fully in world during constructor.
        // UnholyRuneRunicPowerResource::Initialize() is safe - it only sets default rune values.
        this->_resource.Initialize(bot);

        // Note: Do NOT call bot->GetName() here - Player data may not be loaded yet
        TC_LOG_DEBUG("playerbot", "UnholyDeathKnightRefactored created for bot GUID: {}",
            bot ? bot->GetGUID().GetCounter() : 0);

        InitializeUnholyMechanics();
    }

    void UpdateRotation(::Unit* target) override
    {
        if (!target || !target->IsAlive() || !target->IsHostileTo(this->GetBot()))
            return;

        // Detect hero talents if not yet cached
        if (!_heroTalents.detected)
            _heroTalents.Refresh(this->GetBot());

        // Hero talent rotation branching
        // Unholy DK has access to: San'layn / Rider of the Apocalypse
        if (_heroTalents.IsTree(HeroTalentTree::SANLAYN))
        {
            // San'layn: Vampiric Strike replaces Scourge Strike with life-stealing variant
            if (this->CanCastSpell(WoW120Spells::DeathKnight::Unholy::UNHOLY_VAMPIRIC_STRIKE, target))
            {
                this->CastSpell(WoW120Spells::DeathKnight::Unholy::UNHOLY_VAMPIRIC_STRIKE, target);
                return;
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::RIDER_OF_THE_APOCALYPSE))
        {
            // Rider: Apocalypse summons enhanced horsemen, Morbidity enhances diseases
            if (this->CanCastSpell(WoW120Spells::DeathKnight::Unholy::UNHOLY_APOCALYPSE, target))
            {
                // Use enhanced Apocalypse when target has 4+ Festering Wounds
                if (target->HasAura(FESTERING_WOUND))
                {
                    this->CastSpell(WoW120Spells::DeathKnight::Unholy::UNHOLY_APOCALYPSE, target);
                    return;
                }
            }
        }

        // Update Unholy state
        UpdateUnholyState(target);

        // Ensure ghoul is summoned
        EnsureGhoulActive();

        // Use major cooldowns
        HandleCooldowns(target);

        // Determine if AoE or single target
        uint32 enemyCount = this->GetEnemiesInRange(10.0f);
        if (enemyCount >= 3)
        {
            ExecuteAoERotation(target, enemyCount);
        }        else
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


protected:
    void ExecuteSingleTargetRotation(::Unit* target)
    {
        ObjectGuid targetGuid = target->GetGUID();
        uint32 rp = this->_resource.runicPower;
        uint32 totalRunes = this->_resource.bloodRunes + this->_resource.frostRunes + this->_resource.unholyRunes;
        uint32 wounds = _woundTracker.GetWoundCount(targetGuid);        // Priority 1: Apply/maintain Virulent Plague
        if (!target->HasAura(VIRULENT_PLAGUE) && this->CanCastSpell(OUTBREAK, target))
        {
            this->CastSpell(OUTBREAK, target);
            _lastOutbreakTime = GameTime::GetGameTimeMS();
            return;
        }

        // Priority 2: Soul Reaper (execute < 35%)
        if (target->GetHealthPct() < 35.0f && this->CanCastSpell(SOUL_REAPER, target))
        {
            this->CastSpell(SOUL_REAPER, target);
            return;
        }

        // Priority 3: Death Coil with Sudden Doom proc
        if (_suddenDoomProc && this->CanCastSpell(DEATH_COIL, target))
        {
            this->CastSpell(DEATH_COIL, target);
            _suddenDoomProc = false;
            return;
        }

        // Priority 4: Scourge Strike (burst wounds if 4+ stacks)
        if (wounds >= 4 && totalRunes >= 1 && this->CanCastSpell(SCOURGE_STRIKE, target))
        {
            this->CastSpell(SCOURGE_STRIKE, target);
            _woundTracker.BurstWounds(targetGuid, wounds);
            ConsumeRunes(1);
            GenerateRunicPower(10);
            return;
        }

        // Priority 5: Festering Strike (apply wounds)
        if (wounds < 4 && totalRunes >= 2 && this->CanCastSpell(FESTERING_STRIKE, target))
        {
            this->CastSpell(FESTERING_STRIKE, target);
            _woundTracker.ApplyWounds(targetGuid, 4);
            ConsumeRunes(2);
            GenerateRunicPower(15);
            return;
        }

        // Priority 6: Dark Transformation (empower ghoul)
        if (rp >= 40 && !_petTracker.IsDarkTransformationActive() && this->CanCastSpell(DARK_TRANSFORMATION, this->GetBot()))
        {
            this->CastSpell(DARK_TRANSFORMATION, this->GetBot());
            _petTracker.ActivateDarkTransformation();
            ConsumeRunicPower(40);
            return;
        }

        // Priority 7: Death Coil (dump RP)
        if (rp >= 50 && this->CanCastSpell(DEATH_COIL, target))        {
            this->CastSpell(DEATH_COIL, target);
            ConsumeRunicPower(30);
            return;
        }

        // Priority 8: Scourge Strike (burst any wounds)
        if (wounds > 0 && totalRunes >= 1 && this->CanCastSpell(SCOURGE_STRIKE, target))
        {
            this->CastSpell(SCOURGE_STRIKE, target);
            _woundTracker.BurstWounds(targetGuid, wounds);
            ConsumeRunes(1);
            GenerateRunicPower(10);
            return;
        }

        // Priority 9: Death Coil (prevent RP capping)
        if (rp >= 30 && this->CanCastSpell(DEATH_COIL, target))
        {
            this->CastSpell(DEATH_COIL, target);
            ConsumeRunicPower(30);
            return;
        }
    }

    void ExecuteAoERotation(::Unit* target, uint32 enemyCount)
    {
        uint32 rp = this->_resource.runicPower;
        uint32 totalRunes = this->_resource.bloodRunes + this->_resource.frostRunes + this->_resource.unholyRunes;

        // Priority 1: Apply Virulent Plague
        if (!target->HasAura(VIRULENT_PLAGUE) && this->CanCastSpell(OUTBREAK, target))
        {
            this->CastSpell(OUTBREAK, target);
            return;
        }

        // Priority 2: Epidemic (spread disease)
        if (totalRunes >= 1 && this->CanCastSpell(EPIDEMIC, target))        {
            this->CastSpell(EPIDEMIC, target);
            ConsumeRunes(1);
            GenerateRunicPower(10);
            return;
        }

        // Priority 3: Defile (talent, ground AoE)
        if (totalRunes >= 1 && this->CanCastSpell(DEFILE, this->GetBot()))
        {
            this->CastSpell(DEFILE, this->GetBot());
            ConsumeRunes(1);
            return;
        }

        // Priority 4: Death Coil
        if (rp >= 30 && this->CanCastSpell(DEATH_COIL, target))
        {
            this->CastSpell(DEATH_COIL, target);
            ConsumeRunicPower(30);
            return;
        }

        // Priority 5: Scourge Strike
        if (totalRunes >= 1 && this->CanCastSpell(SCOURGE_STRIKE, target))
        {
            this->CastSpell(SCOURGE_STRIKE, target);
            ConsumeRunes(1);
            GenerateRunicPower(10);
            return;
        }
    }

    void HandleCooldowns(::Unit* target)
    {
        ObjectGuid targetGuid = target->GetGUID();
        uint32 wounds = _woundTracker.GetWoundCount(targetGuid);
        uint32 totalRunes = this->_resource.bloodRunes + this->_resource.frostRunes + this->_resource.unholyRunes;

        // Apocalypse (burst wounds + summon ghouls)
        if (wounds >= 4 && this->CanCastSpell(APOCALYPSE, target))
        {
            this->CastSpell(APOCALYPSE, target);
            _woundTracker.BurstWounds(targetGuid, wounds);
            

        // Register cooldowns using CooldownManager
        // COMMENTED OUT:         _cooldowns.RegisterBatch({
        // COMMENTED OUT:             {FESTERING_STRIKE, 0, 1},
        // COMMENTED OUT:             {SCOURGE_STRIKE, 0, 1},
        // COMMENTED OUT:             {DEATH_COIL, 0, 1},
        // COMMENTED OUT:             {OUTBREAK, 0, 1},
        // COMMENTED OUT:             {DARK_TRANSFORMATION, 0, 1},
        // COMMENTED OUT:             {APOCALYPSE, 90000, 1},
        // COMMENTED OUT:             {ARMY_OF_THE_DEAD_UNHOLY, 480000, 1},
        // COMMENTED OUT:             {SUMMON_GARGOYLE, CooldownPresets::MAJOR_OFFENSIVE, 1},
        // COMMENTED OUT:             {UNHOLY_ASSAULT, 90000, 1},
        // COMMENTED OUT:             {UNHOLY_BLIGHT, CooldownPresets::OFFENSIVE_45, 1},
        // COMMENTED OUT:             {SOUL_REAPER, 6000, 1},
        // COMMENTED OUT:             {DEATH_GRIP_UNHOLY, 25000, 1},
        // COMMENTED OUT:             {ANTI_MAGIC_SHELL_UNHOLY, CooldownPresets::OFFENSIVE_60, 1},
        // COMMENTED OUT:             {ICEBOUND_FORTITUDE_UNHOLY, CooldownPresets::MAJOR_OFFENSIVE, 1},
        // COMMENTED OUT:             {DEATHS_ADVANCE_UNHOLY, 90000, 1},
        // COMMENTED OUT:         });

        TC_LOG_DEBUG("playerbot", "Unholy: Apocalypse");
        }

        // Army of the Dead
        if (this->CanCastSpell(ARMY_OF_THE_DEAD_UNHOLY, this->GetBot()))
        {
            this->CastSpell(ARMY_OF_THE_DEAD_UNHOLY, this->GetBot());
            TC_LOG_DEBUG("playerbot", "Unholy: Army of the Dead");
        }

        // Summon Gargoyle
        if (this->CanCastSpell(SUMMON_GARGOYLE, this->GetBot()))
        {
            this->CastSpell(SUMMON_GARGOYLE, this->GetBot());
            _petTracker.SummonGargoyle();
            TC_LOG_DEBUG("playerbot", "Unholy: Summon Gargoyle");
        }

        // Unholy Assault (talent)
        if (totalRunes >= 2 && this->CanCastSpell(UNHOLY_ASSAULT, target))
        {
            this->CastSpell(UNHOLY_ASSAULT, target);
            TC_LOG_DEBUG("playerbot", "Unholy: Unholy Assault");
        }

        // Unholy Blight (talent)
        if (this->CanCastSpell(UNHOLY_BLIGHT, this->GetBot()))
        {
            this->CastSpell(UNHOLY_BLIGHT, this->GetBot());
            TC_LOG_DEBUG("playerbot", "Unholy: Unholy Blight");
        }
    }

    void HandleDefensiveCooldowns()
    {
        Player* bot = this->GetBot();
        float healthPct = bot->GetHealthPct();

        // Icebound Fortitude
        if (healthPct < 40.0f && this->CanCastSpell(ICEBOUND_FORTITUDE_UNHOLY, bot))
        {
            this->CastSpell(ICEBOUND_FORTITUDE_UNHOLY, bot);
            TC_LOG_DEBUG("playerbot", "Unholy: Icebound Fortitude");
            return;
        }

        // Anti-Magic Shell
        if (healthPct < 60.0f && this->CanCastSpell(ANTI_MAGIC_SHELL_UNHOLY, bot))
        {
            this->CastSpell(ANTI_MAGIC_SHELL_UNHOLY, bot);
            TC_LOG_DEBUG("playerbot", "Unholy: Anti-Magic Shell");
            return;
        }

        // Death's Advance
        if (healthPct < 70.0f && this->CanCastSpell(DEATHS_ADVANCE_UNHOLY, bot))
        {
            this->CastSpell(DEATHS_ADVANCE_UNHOLY, bot);
            TC_LOG_DEBUG("playerbot", "Unholy: Death's Advance");
            return;
        }
    }

    void EnsureGhoulActive()
    {
        if (_petTracker.IsGhoulActive())
            return;

        Player* bot = this->GetBot();
        if (!bot)
            return;

        if (this->CanCastSpell(RAISE_DEAD_UNHOLY, bot))
        {
            this->CastSpell(RAISE_DEAD_UNHOLY, bot);
            _petTracker.SummonGhoul();
            TC_LOG_DEBUG("playerbot", "Unholy: Raise Dead (ghoul)");
        }
    }

private:
    void UpdateUnholyState(::Unit* target)
    {
        // Update Festering Wound tracker
        _woundTracker.Update(target);

        // Update pet tracker
        _petTracker.Update(this->GetBot());

        // Update Sudden Doom proc
        if (this->GetBot() && this->GetBot()->HasAura(SUDDEN_DOOM))
            _suddenDoomProc = true;
        else
            _suddenDoomProc = false;

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

    void InitializeUnholyMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)
        // DESIGN NOTE: BotAI integration for ActionPriorityQueue and BehaviorTree registration
        // Implementation should include:
        // - Proper BotAI retrieval from Player instance (via custom Player extension or bot registry)
        // - ActionPriorityQueue registration for Unholy DK spells (Anti-Magic Shell, Army of the Dead, Apocalypse, Festering Strike, Scourge Strike, Death Coil)
        // - BehaviorTree construction with burst cooldowns, Festering Wound management, wound application/bursting, and runic power dump
        // Reference: WoW 12.0 Death Knight mechanics, Unholy specialization pet management and Festering Wound stacking patterns
        // Commenting out for now until BotAI integration is implemented
        /*
        BotAI* ai = this->GetBot()->GetBotAI();
        if (!ai) return;

        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            queue->RegisterSpell(UNHOLY_ANTIMAGIC_SHELL, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
            queue->AddCondition(UNHOLY_ANTIMAGIC_SHELL, [](Player* bot, Unit*) { return bot && bot->GetHealthPct() < 40.0f; }, "HP < 40%");

            queue->RegisterSpell(UNHOLY_ARMY_OF_DEAD, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(UNHOLY_ARMY_OF_DEAD, [](Player*, Unit* target) { return target != nullptr; }, "Major CD");

            queue->RegisterSpell(UNHOLY_APOCALYPSE, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(UNHOLY_APOCALYPSE, [this](Player*, Unit* target) { return target && this->_woundTracker.GetWounds(target->GetGUID()) >= 4; }, "4+ wounds");

            queue->RegisterSpell(UNHOLY_FESTERING_STRIKE, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(UNHOLY_FESTERING_STRIKE, [this](Player*, Unit* target) { return target && this->_woundTracker.GetWounds(target->GetGUID()) < 4; }, "< 4 wounds");

            queue->RegisterSpell(UNHOLY_SCOURGE_STRIKE, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(UNHOLY_SCOURGE_STRIKE, [this](Player*, Unit* target) { return target && this->_woundTracker.GetWounds(target->GetGUID()) > 0; }, "Has wounds");

            queue->RegisterSpell(UNHOLY_DEATH_COIL, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(UNHOLY_DEATH_COIL, [this](Player*, Unit* target) { return target && (this->_suddenDoomProc || this->_resource.runicPower >= 40); }, "Sudden Doom or 40 RP");
        }

        auto* tree = ai->GetBehaviorTree();
        if (tree)
        {
            auto root = Selector("Unholy DK", {
                Sequence("Burst", { Condition("Has target", [this](Player* bot, Unit*) { return bot && bot->GetVictim(); }),
                    bot::ai::Action("Army/Apocalypse", [this](Player* bot, Unit*) { Unit* t = bot->GetVictim(); if (t && this->CanCastSpell(UNHOLY_ARMY_OF_DEAD, bot)) { this->CastSpell(UNHOLY_ARMY_OF_DEAD, bot); return NodeStatus::SUCCESS; } return NodeStatus::FAILURE; }) }),
                Sequence("Wounds", { Condition("< 4 wounds", [this](Player* bot, Unit*) { Unit* t = bot->GetVictim(); return t && this->_woundTracker.GetWounds(t->GetGUID()) < 4; }),
                    bot::ai::Action("Festering Strike", [this](Player* bot, Unit*) { Unit* t = bot->GetVictim(); if (t && this->CanCastSpell(UNHOLY_FESTERING_STRIKE, t)) { this->CastSpell(UNHOLY_FESTERING_STRIKE, t); this->_woundTracker.AddWounds(t->GetGUID(), 2); return NodeStatus::SUCCESS; } return NodeStatus::FAILURE; }) }),
                Sequence("Pop Wounds", { Condition("Has wounds", [this](Player* bot, Unit*) { Unit* t = bot->GetVictim(); return t && this->_woundTracker.GetWounds(t->GetGUID()) > 0; }),
                    bot::ai::Action("Scourge Strike", [this](Player* bot, Unit*) { Unit* t = bot->GetVictim(); if (t && this->CanCastSpell(UNHOLY_SCOURGE_STRIKE, t)) { this->CastSpell(UNHOLY_SCOURGE_STRIKE, t); this->_woundTracker.ConsumeWounds(t->GetGUID(), 1); return NodeStatus::SUCCESS; } return NodeStatus::FAILURE; }) }),
                Sequence("RP Dump", { Condition("40+ RP", [this](Player*, Unit*) { return this->_resource.runicPower >= 40; }),
                    bot::ai::Action("Death Coil", [this](Player* bot, Unit*) { Unit* t = bot->GetVictim(); if (t && this->CanCastSpell(UNHOLY_DEATH_COIL, t)) { this->CastSpell(UNHOLY_DEATH_COIL, t); this->ConsumeRunicPower(40); return NodeStatus::SUCCESS; } return NodeStatus::FAILURE; }) })
            });
            tree->SetRoot(root);
        }
        */
    }

private:
    UnholyFesteringWoundTracker _woundTracker;
    UnholyPetTracker _petTracker;
    bool _suddenDoomProc;
    uint32 _lastOutbreakTime;

    // Hero talent detection cache (refreshed on combat start)
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot
