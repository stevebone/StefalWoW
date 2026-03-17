/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Demonology Warlock Refactored - Template-Based Implementation
 *
 * This file provides a complete, template-based implementation of Demonology Warlock
 * using the RangedDpsSpecialization with dual resource system (Mana + Soul Shards).
 */

#pragma once


#include "../Common/StatusEffectTracker.h"
#include "../Common/CooldownManager.h"
#include "../Common/RotationHelpers.h"
#include "../CombatSpecializationTemplates.h"
#include "../ResourceTypes.h"
#include "Player.h"
#include "Pet.h"
#include "SpellMgr.h"
#include "SpellAuraEffects.h"
#include "Log.h"
// Phase 5 Integration: Decision Systems
#include "../../Decision/ActionPriorityQueue.h"
#include "../../Decision/BehaviorTree.h"
#include "../BotAI.h"
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
// ============================================================================
// DEMONOLOGY WARLOCK SPELL IDs (WoW 12.0 - The War Within)
// Central Registry: WoW120Spells::Warlock::Demonology in SpellValidation_WoW120_Part2.h
// ============================================================================

enum DemonologyWarlockSpells
{
    // Core Builders - Using central registry: WoW120Spells::Warlock::Demonology
    HAND_OF_GULDAN           = WoW120Spells::Warlock::Demonology::HAND_OF_GULDAN,
    DEMONBOLT                = WoW120Spells::Warlock::Demonology::DEMONBOLT,
    SHADOW_BOLT_DEMO         = WoW120Spells::Warlock::SHADOW_BOLT,

    // Demon Summoning - Using central registry: WoW120Spells::Warlock::Demonology
    CALL_DREADSTALKERS       = WoW120Spells::Warlock::Demonology::CALL_DREADSTALKERS,
    SUMMON_VILEFIEND         = WoW120Spells::Warlock::Demonology::SUMMON_VILEFIEND,
    GRIMOIRE_FELGUARD        = WoW120Spells::Warlock::Demonology::GRIMOIRE_FELGUARD,
    NETHER_PORTAL            = WoW120Spells::Warlock::Demonology::NETHER_PORTAL,
    SUMMON_DEMONIC_TYRANT    = WoW120Spells::Warlock::Demonology::SUMMON_DEMONIC_TYRANT,

    // Permanent Pets - Using central registry: WoW120Spells::Warlock
    SUMMON_FELGUARD          = WoW120Spells::Warlock::SUMMON_FELGUARD,
    SUMMON_VOIDWALKER_DEMO   = WoW120Spells::Warlock::SUMMON_VOIDWALKER,
    SUMMON_IMP_DEMO          = WoW120Spells::Warlock::SUMMON_IMP,
    COMMAND_DEMON_DEMO       = WoW120Spells::Warlock::COMMAND_DEMON,

    // Direct Damage - Using central registry: WoW120Spells::Warlock::Demonology
    IMPLOSION                = WoW120Spells::Warlock::Demonology::IMPLOSION,
    DEMONFIRE                = WoW120Spells::Warlock::Demonology::DEMONFIRE,
    DOOM                     = WoW120Spells::Warlock::Demonology::DOOM,

    // Buffs and Procs - Using central registry: WoW120Spells::Warlock::Demonology
    DEMONIC_CORE             = WoW120Spells::Warlock::Demonology::DEMONIC_CORE,
    DEMONIC_CALLING          = WoW120Spells::Warlock::Demonology::DEMONIC_CALLING,
    DEMONIC_STRENGTH         = WoW120Spells::Warlock::Demonology::DEMONIC_STRENGTH,
    POWER_SIPHON             = WoW120Spells::Warlock::Demonology::POWER_SIPHON,

    // Major Cooldowns - Using central registry: WoW120Spells::Warlock::Demonology
    SUMMON_DEMONIC_TYRANT_CD = WoW120Spells::Warlock::Demonology::SUMMON_DEMONIC_TYRANT,
    NETHER_PORTAL_CD         = WoW120Spells::Warlock::Demonology::NETHER_PORTAL,
    GUILLOTINE               = WoW120Spells::Warlock::Demonology::GUILLOTINE,

    // Utility - Using central registry: WoW120Spells::Warlock::Demonology
    SOUL_STRIKE              = WoW120Spells::Warlock::Demonology::SOUL_STRIKE,
    FEL_DOMINATION           = WoW120Spells::Warlock::Demonology::FEL_DOMINATION,
    HEALTH_FUNNEL_DEMO       = WoW120Spells::Warlock::HEALTH_FUNNEL,
    BANISH_DEMO              = WoW120Spells::Warlock::BANISH,
    FEAR_DEMO                = WoW120Spells::Warlock::FEAR,
    MORTAL_COIL_DEMO         = WoW120Spells::Warlock::MORTAL_COIL,
    SHADOWFURY               = WoW120Spells::Warlock::SHADOWFURY,

    // Defensives - Using central registry: WoW120Spells::Warlock
    UNENDING_RESOLVE_DEMO    = WoW120Spells::Warlock::UNENDING_RESOLVE,
    DARK_PACT_DEMO           = WoW120Spells::Warlock::Affliction::DARK_PACT,
    DEMONIC_CIRCLE_TELEPORT_DEMO = WoW120Spells::Warlock::DEMONIC_CIRCLE_TELEPORT,
    DEMONIC_GATEWAY_DEMO     = WoW120Spells::Warlock::DEMONIC_GATEWAY,
    BURNING_RUSH_DEMO        = WoW120Spells::Warlock::BURNING_RUSH,

    // Talents - Using central registry: WoW120Spells::Warlock::Demonology
    FROM_THE_SHADOWS         = WoW120Spells::Warlock::Demonology::FROM_THE_SHADOWS,
    SOUL_CONDUIT_DEMO        = WoW120Spells::Warlock::Demonology::SOUL_CONDUIT,
    INNER_DEMONS             = WoW120Spells::Warlock::Demonology::INNER_DEMONS,
    CARNIVOROUS_STALKERS     = WoW120Spells::Warlock::Demonology::CARNIVOROUS_STALKERS
};

// Dual resource type for Demonology Warlock
struct ManaSoulShardResourceDemo
{
    uint32 mana{0};
    uint32 soulShards{0};
    uint32 maxMana{100000};
    uint32 maxSoulShards{5};
    bool available{true};

    bool Consume(uint32 manaCost)
    {
        if (mana >= manaCost) {
            mana -= manaCost;
            available = mana > 0;
            return true;
        }
        return false;
    }

    void Regenerate(uint32 diff)
    {
        // Mana regenerates naturally
        if (mana < maxMana)
        {
            uint32 regenAmount = (maxMana / 100) * (diff / 1000);
            mana = ::std::min(mana + regenAmount, maxMana);
        }
        available = mana > 0;
    }

    [[nodiscard]] uint32 GetAvailable() const { return mana; }
    [[nodiscard]] uint32 GetMax() const { return maxMana; }

    void Initialize(Player* bot)
    {
        // Defer player data access until bot is fully in world
        // During construction, Player data may not be loaded yet
        if (bot && bot->IsInWorld())
        {
            maxMana = bot->GetMaxPower(POWER_MANA);
            mana = bot->GetPower(POWER_MANA);
        }
        // Use safe defaults until data is available
        soulShards = 0;
        available = maxMana > 0;
    }

    // Refresh resource values from player when data becomes available
    void RefreshFromPlayer(Player* bot)
    {
        if (bot && bot->IsInWorld())
        {
            maxMana = bot->GetMaxPower(POWER_MANA);
            mana = bot->GetPower(POWER_MANA);
            available = mana > 0;
        }
    }
};

// ============================================================================
// DEMONOLOGY DEMON TRACKER
// ============================================================================

class DemonologyDemonTracker
{
public:
    DemonologyDemonTracker()
        : _wildImpCount(0)
        , _dreadstalkerActive(false)
        , _dreadstalkerEndTime(0)
        , _vileFiendActive(false)
        , _vileFiendEndTime(0)
        , _tyrantActive(false)
        , _tyrantEndTime(0)
    {
    }

    void SummonWildImps(uint32 count)
    {
        _wildImpCount += count;
    }

    void ExplodeWildImps()
    {
        _wildImpCount = 0;
    }

    uint32 GetWildImpCount() const { return _wildImpCount; }

    void SummonDreadstalkers()
    {
        _dreadstalkerActive = true;
        _dreadstalkerEndTime = GameTime::GetGameTimeMS() + 12000; // 12 sec duration
    }

    bool AreDreadstalkersActive() const { return _dreadstalkerActive; }

    void SummonVilefiend()
    {
        _vileFiendActive = true;
        _vileFiendEndTime = GameTime::GetGameTimeMS() + 15000; // 15 sec duration
    }

    bool IsVilefiendActive() const { return _vileFiendActive; }

    void SummonTyrant()
    {
        _tyrantActive = true;
        _tyrantEndTime = GameTime::GetGameTimeMS() + 15000; // 15 sec base duration
    }

    bool IsTyrantActive() const { return _tyrantActive; }

    uint32 GetActiveDemonCount() const
    {
        uint32 count = _wildImpCount;
        if (_dreadstalkerActive) count += 2; // Dreadstalkers spawn as pair
        if (_vileFiendActive) count += 1;
        if (_tyrantActive) count += 1;
        return count;
    }

    void Update()
    {
        uint32 now = GameTime::GetGameTimeMS();

        // Update Dreadstalkers
        if (_dreadstalkerActive && now >= _dreadstalkerEndTime)
        {
            _dreadstalkerActive = false;
            _dreadstalkerEndTime = 0;
        }

        // Update Vilefiend
        if (_vileFiendActive && now >= _vileFiendEndTime)
        {
            _vileFiendActive = false;
            _vileFiendEndTime = 0;
        }

        // Update Tyrant
        if (_tyrantActive && now >= _tyrantEndTime)
        {
            _tyrantActive = false;
            _tyrantEndTime = 0;
        }

        // Wild Imps naturally despawn after 20 sec, but we track via spells
        // Decay imps over time
        if (_wildImpCount > 0 && rand() % 100 < 5) // 5% chance per update
            _wildImpCount = ::std::max(0u, _wildImpCount - 1);
    }

private:
    CooldownManager _cooldowns;
    uint32 _wildImpCount;
    bool _dreadstalkerActive;
    uint32 _dreadstalkerEndTime;
    bool _vileFiendActive;
    uint32 _vileFiendEndTime;
    bool _tyrantActive;
    uint32 _tyrantEndTime;
};

// ============================================================================
// DEMONOLOGY WARLOCK REFACTORED
// ============================================================================

class DemonologyWarlockRefactored : public RangedDpsSpecialization<ManaSoulShardResourceDemo>
{
public:
    using Base = RangedDpsSpecialization<ManaSoulShardResourceDemo>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::_resource;
    explicit DemonologyWarlockRefactored(Player* bot)
        : RangedDpsSpecialization<ManaSoulShardResourceDemo>(bot)
        , _demonTracker()
        , _demonicCoreStacks(0)
        , _lastTyrantTime(0)
    {
        // Initialize mana/soul shard resources (safe with IsInWorld check)
        this->_resource.Initialize(bot);

        // Note: Do NOT call bot->GetName() here - Player data may not be loaded yet
        // Logging will happen once bot is fully active
        TC_LOG_DEBUG("playerbot", "DemonologyWarlockRefactored created for bot GUID: {}",
            bot ? bot->GetGUID().GetCounter() : 0);

        // Phase 5: Initialize decision systems
        InitializeDemonologyMechanics();
    }

    void UpdateRotation(::Unit* target) override
    {
        if (!target || !target->IsAlive() || !target->IsHostileTo(this->GetBot()))
            return;

        // Detect hero talents if not yet cached
        if (!_heroTalents.detected)
            _heroTalents.Refresh(this->GetBot());

        // Hero talent rotation branches
        if (_heroTalents.IsTree(HeroTalentTree::DIABOLIST))
        {
            // Diabolist: Diabolic Ritual for empowered demon summoning
            if (this->CanCastSpell(WoW120Spells::Warlock::Demonology::DIABOLIC_RITUAL, target))
            {
                this->CastSpell(WoW120Spells::Warlock::Demonology::DIABOLIC_RITUAL, target);
                return;
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::SOUL_HARVESTER))
        {
            // Soul Harvester: Demonic Soul for enhanced soul shard generation
            if (this->CanCastSpell(WoW120Spells::Warlock::Demonology::DEMO_DEMONIC_SOUL, target))
            {
                this->CastSpell(WoW120Spells::Warlock::Demonology::DEMO_DEMONIC_SOUL, target);
                return;
            }
        }

        // Update Demonology state
        UpdateDemonologyState();

        // Ensure Felguard is active
        EnsureFelguardActive();

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
    }

    void UpdateBuffs() override
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        // Defensive cooldowns
        HandleDefensiveCooldowns();
    }

    /**
     * @brief Called by BotAI when NOT in combat - handles pet summoning
     * CRITICAL FIX: Refactored warlocks were NOT summoning pets because
     * OnNonCombatUpdate was not overridden, and UpdateBuffs was only called IN combat!
     * Pet summons have 6 second cast time - MUST happen out of combat!
     */
    void OnNonCombatUpdate(uint32 /*diff*/) override
    {
        Player* bot = this->GetBot();
        if (!bot || !bot->IsAlive())
            return;

        // Don't summon while casting (6s cast time!)
        if (bot->HasUnitState(UNIT_STATE_CASTING))
            return;

        // Primary purpose: Ensure Felguard is summoned out of combat
        EnsureFelguardActive();
    }

protected:
    void ExecuteSingleTargetRotation(::Unit* target)
    {
        uint32 shards = this->_resource.soulShards;
        uint32 demonCount = _demonTracker.GetActiveDemonCount();

        // Priority 1: Summon Demonic Tyrant (when we have multiple demons out)
        if (demonCount >= 3 && this->CanCastSpell(SUMMON_DEMONIC_TYRANT, this->GetBot()))
        {
            this->CastSpell(SUMMON_DEMONIC_TYRANT, this->GetBot());
            _demonTracker.SummonTyrant();
            _lastTyrantTime = GameTime::GetGameTimeMS();
            TC_LOG_DEBUG("playerbot", "Demonology: Summon Demonic Tyrant ({} demons)", demonCount);
            return;
        }

        // Priority 2: Call Dreadstalkers (core demon summon)
        if (shards >= 2 && this->CanCastSpell(CALL_DREADSTALKERS, this->GetBot()))
        {
            this->CastSpell(CALL_DREADSTALKERS, this->GetBot());
            _demonTracker.SummonDreadstalkers();
            ConsumeSoulShard(2);
            return;
        }

        // Priority 3: Grimoire: Felguard (talent, major CD)
        if (this->CanCastSpell(GRIMOIRE_FELGUARD, this->GetBot()))
        {
            this->CastSpell(GRIMOIRE_FELGUARD, this->GetBot());
            

        // Register cooldowns using CooldownManager
        // COMMENTED OUT:         _cooldowns.RegisterBatch({
        // COMMENTED OUT:             {CALL_DREADSTALKERS, 0, 1},
        // COMMENTED OUT:             {SUMMON_DEMONIC_TYRANT, 90000, 1},
        // COMMENTED OUT:             {GRIMOIRE_FELGUARD, CooldownPresets::MINOR_OFFENSIVE, 1},
        // COMMENTED OUT:             {NETHER_PORTAL, CooldownPresets::MAJOR_OFFENSIVE, 1},
        // COMMENTED OUT:             {GUILLOTINE, CooldownPresets::OFFENSIVE_45, 1},
        // COMMENTED OUT:             {UNENDING_RESOLVE_DEMO, CooldownPresets::MAJOR_OFFENSIVE, 1},
        // COMMENTED OUT:             {DARK_PACT_DEMO, CooldownPresets::OFFENSIVE_60, 1},
        // COMMENTED OUT:             {MORTAL_COIL_DEMO, CooldownPresets::OFFENSIVE_45, 1},
        // COMMENTED OUT:             {SHADOWFURY, CooldownPresets::OFFENSIVE_60, 1},
        // COMMENTED OUT:         });

        TC_LOG_DEBUG("playerbot", "Demonology: Grimoire Felguard");
            return;
        }

        // Priority 4: Summon Vilefiend (talent)
        if (shards >= 1 && this->CanCastSpell(SUMMON_VILEFIEND, this->GetBot()))
        {
            this->CastSpell(SUMMON_VILEFIEND, this->GetBot());
            _demonTracker.SummonVilefiend();
            ConsumeSoulShard(1);
            return;
        }

        // Priority 5: Nether Portal (talent, major CD)
        if (shards >= 1 && this->CanCastSpell(NETHER_PORTAL, this->GetBot()))
        {
            this->CastSpell(NETHER_PORTAL, this->GetBot());
            TC_LOG_DEBUG("playerbot", "Demonology: Nether Portal");
            return;
        }

        // Priority 6: Hand of Gul'dan (summon Wild Imps)
        if (shards >= 3 && this->CanCastSpell(HAND_OF_GULDAN, target))
        {
            this->CastSpell(HAND_OF_GULDAN, target);
            _demonTracker.SummonWildImps(3); // Summons 3 Wild Imps per cast
            ConsumeSoulShard(3);
            return;
        }

        // Priority 7: Demonbolt (use Demonic Core procs)
        if (_demonicCoreStacks > 0 || (shards >= 2 && this->CanCastSpell(DEMONBOLT, target)))
        {
            this->CastSpell(DEMONBOLT, target);
            if (_demonicCoreStacks > 0)
                _demonicCoreStacks--;
            else
                ConsumeSoulShard(2);
            GenerateSoulShard(2);
            return;
        }

        // Priority 8: Guillotine (Felguard burst)
        if (this->CanCastSpell(GUILLOTINE, target))
        {
            this->CastSpell(GUILLOTINE, target);
            return;
        }

        // Priority 9: Shadow Bolt (filler + shard gen)
        if (shards < 5 && this->CanCastSpell(SHADOW_BOLT_DEMO, target))
        {
            this->CastSpell(SHADOW_BOLT_DEMO, target);
            GenerateSoulShard(1);
            return;
        }
    }

    void ExecuteAoERotation(::Unit* target, uint32 enemyCount)
    {
        uint32 shards = this->_resource.soulShards;
        uint32 wildImps = _demonTracker.GetWildImpCount();

        // Priority 1: Summon Demonic Tyrant
        if (_demonTracker.GetActiveDemonCount() >= 3 && this->CanCastSpell(SUMMON_DEMONIC_TYRANT, this->GetBot()))
        {
            this->CastSpell(SUMMON_DEMONIC_TYRANT, this->GetBot());
            _demonTracker.SummonTyrant();
            return;
        }

        // Priority 2: Implosion (explode Wild Imps for AoE)
        if (wildImps >= 4 && this->CanCastSpell(IMPLOSION, target))
        {
            this->CastSpell(IMPLOSION, target);
            _demonTracker.ExplodeWildImps();
            TC_LOG_DEBUG("playerbot", "Demonology: Implosion ({} imps)", wildImps);
            return;
        }

        // Priority 3: Call Dreadstalkers
        if (shards >= 2 && this->CanCastSpell(CALL_DREADSTALKERS, this->GetBot()))
        {
            this->CastSpell(CALL_DREADSTALKERS, this->GetBot());
            _demonTracker.SummonDreadstalkers();
            ConsumeSoulShard(2);
            return;
        }

        // Priority 4: Hand of Gul'dan (AoE + summon imps)
        if (shards >= 3 && this->CanCastSpell(HAND_OF_GULDAN, target))
        {
            this->CastSpell(HAND_OF_GULDAN, target);
            _demonTracker.SummonWildImps(3);
            ConsumeSoulShard(3);
            return;
        }

        // Priority 5: Summon Vilefiend
        if (shards >= 1 && this->CanCastSpell(SUMMON_VILEFIEND, this->GetBot()))
        {
            this->CastSpell(SUMMON_VILEFIEND, this->GetBot());
            _demonTracker.SummonVilefiend();
            ConsumeSoulShard(1);
            return;
        }

        // Priority 6: Demonbolt (proc or shard)
        if (_demonicCoreStacks > 0 || (shards >= 2 && this->CanCastSpell(DEMONBOLT, target)))
        {
            this->CastSpell(DEMONBOLT, target);
            if (_demonicCoreStacks > 0)
                _demonicCoreStacks--;
            else
                ConsumeSoulShard(2);
            GenerateSoulShard(2);
            return;
        }

        // Priority 7: Shadow Bolt filler
        if (shards < 5 && this->CanCastSpell(SHADOW_BOLT_DEMO, target))
        {
            this->CastSpell(SHADOW_BOLT_DEMO, target);
            GenerateSoulShard(1);
            return;
        }
    }

    void HandleDefensiveCooldowns()
    {
        Player* bot = this->GetBot();
        float healthPct = bot->GetHealthPct();

        // Unending Resolve
        if (healthPct < 40.0f && this->CanCastSpell(UNENDING_RESOLVE_DEMO, bot))
        {
            this->CastSpell(UNENDING_RESOLVE_DEMO, bot);
            TC_LOG_DEBUG("playerbot", "Demonology: Unending Resolve");
            return;
        }

        // Dark Pact
        if (healthPct < 50.0f && this->CanCastSpell(DARK_PACT_DEMO, bot))
        {
            this->CastSpell(DARK_PACT_DEMO, bot);
            TC_LOG_DEBUG("playerbot", "Demonology: Dark Pact");
            return;
        }

        // Mortal Coil
        if (healthPct < 60.0f && this->CanCastSpell(MORTAL_COIL_DEMO, bot))
        {
            this->CastSpell(MORTAL_COIL_DEMO, bot);
            TC_LOG_DEBUG("playerbot", "Demonology: Mortal Coil");
            return;
        }

        // Health Funnel (heal Felguard if low)
        if (Pet* pet = bot->GetPet())
        {
            if (pet->GetHealthPct() < 40.0f && this->CanCastSpell(HEALTH_FUNNEL_DEMO, pet))
            {
                this->CastSpell(HEALTH_FUNNEL_DEMO, pet);
                TC_LOG_DEBUG("playerbot", "Demonology: Health Funnel");
                return;
            }
        }
    }

    void EnsureFelguardActive()
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        // Check if pet exists
        if (Pet* pet = bot->GetPet())
        {
            if (pet->IsAlive())
                return; // Pet is active
        }

        // Summon Felguard (Demonology's main pet)
        if (this->CanCastSpell(SUMMON_FELGUARD, bot))
        {            this->CastSpell(SUMMON_FELGUARD, bot);
            TC_LOG_DEBUG("playerbot", "Demonology: Summon Felguard");
        }
    }

private:
    void UpdateDemonologyState()
    {
        // Update demon tracker
        _demonTracker.Update();

        // Update Demonic Core stacks
        if (this->GetBot())
        {
            if (Aura* aura = this->GetBot()->GetAura(DEMONIC_CORE))
                _demonicCoreStacks = aura->GetStackAmount();
                else
                _demonicCoreStacks = 0;
        }

        // Update soul shards from bot
        if (this->GetBot())
        {
            this->_resource.soulShards = this->GetBot()->GetPower(POWER_SOUL_SHARDS);
            this->_resource.mana = this->GetBot()->GetPower(POWER_MANA);
        }
    }

    void GenerateSoulShard(uint32 amount)
    {
        this->_resource.soulShards = ::std::min(this->_resource.soulShards + amount, this->_resource.maxSoulShards);
    }

    void ConsumeSoulShard(uint32 amount)
    {
        this->_resource.soulShards = (this->_resource.soulShards > amount) ? this->_resource.soulShards - amount : 0;
    }

    void InitializeDemonologyMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)
        BotAI* ai = this;
        if (!ai) return;

        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            // EMERGENCY: Defensive cooldowns
            queue->RegisterSpell(UNENDING_RESOLVE_DEMO, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
            queue->AddCondition(UNENDING_RESOLVE_DEMO, [this](Player* bot, Unit*) {
                return bot && bot->GetHealthPct() < 40.0f;
            }, "HP < 40% (damage reduction)");

            // CRITICAL: Demonic Tyrant (extends all demon durations)
            queue->RegisterSpell(SUMMON_DEMONIC_TYRANT, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(SUMMON_DEMONIC_TYRANT, [this](Player*, Unit*) {
                return this->_demonTracker.GetActiveDemonCount() >= 3;
            }, "3+ demons active (extend + buff)");

            // HIGH: Core demon summoners
            queue->RegisterSpell(CALL_DREADSTALKERS, SpellPriority::HIGH, SpellCategory::OFFENSIVE);
            queue->AddCondition(CALL_DREADSTALKERS, [this](Player*, Unit*) {
                return this->_resource.soulShards >= 2;
            }, "2 shards (summon Dreadstalkers)");

            queue->RegisterSpell(HAND_OF_GULDAN, SpellPriority::HIGH, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(HAND_OF_GULDAN, [this](Player*, Unit* target) {
                return target && this->_resource.soulShards >= 3;
            }, "3 shards (summon Wild Imps)");

            queue->RegisterSpell(SUMMON_VILEFIEND, SpellPriority::HIGH, SpellCategory::OFFENSIVE);
            queue->AddCondition(SUMMON_VILEFIEND, [this](Player* bot, Unit*) {
                return bot->HasSpell(SUMMON_VILEFIEND) && this->_resource.soulShards >= 1;
            }, "1 shard (summon Vilefiend, talent)");

            // MEDIUM: Cooldowns
            queue->RegisterSpell(GRIMOIRE_FELGUARD, SpellPriority::MEDIUM, SpellCategory::OFFENSIVE);
            queue->AddCondition(GRIMOIRE_FELGUARD, [this](Player* bot, Unit*) {
                return bot->HasSpell(GRIMOIRE_FELGUARD);
            }, "Summon Felguard (2min CD, talent)");

            queue->RegisterSpell(NETHER_PORTAL, SpellPriority::MEDIUM, SpellCategory::OFFENSIVE);
            queue->AddCondition(NETHER_PORTAL, [this](Player* bot, Unit*) {
                return bot->HasSpell(NETHER_PORTAL) && this->_resource.soulShards >= 1;
            }, "Demon portal (3min CD, talent)");

            queue->RegisterSpell(GUILLOTINE, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(GUILLOTINE, [this](Player* bot, Unit* target) {
                return target && bot->HasSpell(GUILLOTINE);
            }, "Felguard burst (45s CD, talent)");

            // MEDIUM: Demonbolt (proc or spender)
            queue->RegisterSpell(DEMONBOLT, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(DEMONBOLT, [this](Player*, Unit* target) {
                return target && (this->_demonicCoreStacks > 0 || this->_resource.soulShards >= 2);
            }, "Demonic Core proc or 2 shards");

            // MEDIUM: Implosion (explode Wild Imps for AoE)
            queue->RegisterSpell(IMPLOSION, SpellPriority::MEDIUM, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(IMPLOSION, [this](Player*, Unit* target) {
                return target && this->_demonTracker.GetWildImpCount() >= 4;
            }, "4+ Wild Imps (explode for AoE)");

            // LOW: Filler + shard generator
            queue->RegisterSpell(SHADOW_BOLT_DEMO, SpellPriority::LOW, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SHADOW_BOLT_DEMO, [this](Player*, Unit* target) {
                return target && this->_resource.soulShards < 5;
            }, "Filler (generates shards)");
        }

        auto* behaviorTree = ai->GetBehaviorTree();
        if (behaviorTree)
        {
            auto root = Selector("Demonology Warlock DPS", {
                // Tier 1: Burst Window (Demonic Tyrant extends all demons)
                Sequence("Burst Cooldown", {
                    Condition("3+ demons active", [this](Player*, Unit*) {
                        return this->_demonTracker.GetActiveDemonCount() >= 3;
                    }),
                    bot::ai::Action("Cast Demonic Tyrant", [this](Player* bot, Unit*) {
                        if (this->CanCastSpell(SUMMON_DEMONIC_TYRANT, bot))
                        {
                            this->CastSpell(SUMMON_DEMONIC_TYRANT, bot);
                            this->_demonTracker.SummonTyrant();
                            return NodeStatus::SUCCESS;
                        }
                        return NodeStatus::FAILURE;
                    })
                }),

                // Tier 2: Demon Summoning (Dreadstalkers → Vilefiend → Hand of Gul'dan)
                Sequence("Demon Summoning", {
                    Condition("Has target and shards", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim() && this->_resource.soulShards >= 1;
                    }),
                    Selector("Summon demons", {
                        Sequence("Dreadstalkers", {
                            Condition("2+ shards", [this](Player*, Unit*) {
                                return this->_resource.soulShards >= 2;
                            }),
                            bot::ai::Action("Cast Call Dreadstalkers", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(CALL_DREADSTALKERS, bot))
                                {
                                    this->CastSpell(CALL_DREADSTALKERS, bot);
                                    this->_demonTracker.SummonDreadstalkers();
                                    this->ConsumeSoulShard(2);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Vilefiend (talent)", {
                            Condition("1+ shard and has spell", [this](Player* bot, Unit*) {
                                return this->_resource.soulShards >= 1 && bot->HasSpell(SUMMON_VILEFIEND);
                            }),
                            bot::ai::Action("Cast Summon Vilefiend", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(SUMMON_VILEFIEND, bot))
                                {
                                    this->CastSpell(SUMMON_VILEFIEND, bot);
                                    this->_demonTracker.SummonVilefiend();
                                    this->ConsumeSoulShard(1);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Hand of Gul'dan", {
                            Condition("3+ shards", [this](Player*, Unit*) {
                                return this->_resource.soulShards >= 3;
                            }),
                            bot::ai::Action("Cast Hand of Gul'dan", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(HAND_OF_GULDAN, target))
                                {
                                    this->CastSpell(HAND_OF_GULDAN, target);
                                    this->_demonTracker.SummonWildImps(3);
                                    this->ConsumeSoulShard(3);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 3: Demon Abilities (Demonbolt, Implosion, Cooldowns)
                Sequence("Demon Abilities", {
                    Condition("Has target", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim();
                    }),
                    Selector("Use demon abilities", {
                        Sequence("Demonbolt (proc or shard)", {
                            Condition("Demonic Core proc or 2+ shards", [this](Player*, Unit*) {
                                return this->_demonicCoreStacks > 0 || this->_resource.soulShards >= 2;
                            }),
                            bot::ai::Action("Cast Demonbolt", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(DEMONBOLT, target))
                                {
                                    this->CastSpell(DEMONBOLT, target);
                                    if (this->_demonicCoreStacks > 0)
                                        this->_demonicCoreStacks--;
                                    else
                                        this->ConsumeSoulShard(2);
                                    this->GenerateSoulShard(2);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Implosion (AoE)", {
                            Condition("4+ Wild Imps", [this](Player*, Unit*) {
                                return this->_demonTracker.GetWildImpCount() >= 4;
                            }),
                            bot::ai::Action("Cast Implosion", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(IMPLOSION, target))
                                {
                                    this->CastSpell(IMPLOSION, target);
                                    this->_demonTracker.ExplodeWildImps();
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Guillotine (Felguard burst)", {
                            Condition("Has Guillotine talent", [this](Player* bot, Unit*) {
                                return bot->HasSpell(GUILLOTINE);
                            }),
                            bot::ai::Action("Cast Guillotine", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(GUILLOTINE, target))
                                {
                                    this->CastSpell(GUILLOTINE, target);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 4: Shard Generator (Shadow Bolt filler)
                Sequence("Shard Generator", {
                    Condition("Has target and < 5 shards", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim() && this->_resource.soulShards < 5;
                    }),
                    bot::ai::Action("Cast Shadow Bolt", [this](Player* bot, Unit*) {
                        Unit* target = bot->GetVictim();
                        if (target && this->CanCastSpell(SHADOW_BOLT_DEMO, target))
                        {
                            this->CastSpell(SHADOW_BOLT_DEMO, target);
                            this->GenerateSoulShard(1);
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
    DemonologyDemonTracker _demonTracker;
    uint32 _demonicCoreStacks;
    uint32 _lastTyrantTime;

    // Hero talent detection cache (refreshed on combat start)
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot
