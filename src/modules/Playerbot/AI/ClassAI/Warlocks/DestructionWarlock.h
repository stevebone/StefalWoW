/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Destruction Warlock Refactored - Template-Based Implementation
 *
 * This file provides a complete, template-based implementation of Destruction Warlock
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
#include "ObjectAccessor.h"
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
// DESTRUCTION WARLOCK SPELL IDs (WoW 12.0 - The War Within)
// Central Registry: WoW120Spells::Warlock::Destruction in SpellValidation_WoW120_Part2.h
// ============================================================================

enum DestructionWarlockSpells
{
    // Core Spells - Using central registry: WoW120Spells::Warlock::Destruction
    CHAOS_BOLT               = WoW120Spells::Warlock::Destruction::CHAOS_BOLT,
    INCINERATE               = WoW120Spells::Warlock::Destruction::INCINERATE,
    CONFLAGRATE              = WoW120Spells::Warlock::Destruction::CONFLAGRATE,
    IMMOLATE                 = WoW120Spells::Warlock::Destruction::IMMOLATE,

    // AoE Spells - Using central registry: WoW120Spells::Warlock::Destruction
    RAIN_OF_FIRE             = WoW120Spells::Warlock::Destruction::RAIN_OF_FIRE,
    CHANNEL_DEMONFIRE        = WoW120Spells::Warlock::Destruction::CHANNEL_DEMONFIRE,
    CATACLYSM                = WoW120Spells::Warlock::Destruction::CATACLYSM,
    HAVOC                    = WoW120Spells::Warlock::Destruction::HAVOC,

    // Major Cooldowns - Using central registry: WoW120Spells::Warlock::Destruction
    SUMMON_INFERNAL          = WoW120Spells::Warlock::Destruction::SUMMON_INFERNAL,
    DARK_SOUL_INSTABILITY    = WoW120Spells::Warlock::Destruction::DARK_SOUL_INSTABILITY,
    SOUL_FIRE                = WoW120Spells::Warlock::Destruction::SOUL_FIRE,

    // Pet Management - Using central registry: WoW120Spells::Warlock
    SUMMON_IMP_DESTRO        = WoW120Spells::Warlock::SUMMON_IMP,
    SUMMON_VOIDWALKER_DESTRO = WoW120Spells::Warlock::SUMMON_VOIDWALKER,
    SUMMON_SUCCUBUS_DESTRO   = WoW120Spells::Warlock::SUMMON_SUCCUBUS,
    SUMMON_FELHUNTER_DESTRO  = WoW120Spells::Warlock::SUMMON_FELHUNTER,
    COMMAND_DEMON_DESTRO     = WoW120Spells::Warlock::COMMAND_DEMON,

    // Utility - Using central registry: WoW120Spells::Warlock
    CURSE_OF_TONGUES_DESTRO  = WoW120Spells::Warlock::CURSE_OF_TONGUES,
    CURSE_OF_WEAKNESS_DESTRO = WoW120Spells::Warlock::CURSE_OF_WEAKNESS,
    CURSE_OF_EXHAUSTION_DESTRO = WoW120Spells::Warlock::CURSE_OF_EXHAUSTION,
    SHADOWBURN               = WoW120Spells::Warlock::Destruction::SHADOWBURN,
    BACKDRAFT                = WoW120Spells::Warlock::Destruction::BACKDRAFT,

    // Defensives - Using central registry: WoW120Spells::Warlock
    UNENDING_RESOLVE_DESTRO  = WoW120Spells::Warlock::UNENDING_RESOLVE,
    DARK_PACT_DESTRO         = WoW120Spells::Warlock::Affliction::DARK_PACT,
    MORTAL_COIL_DESTRO       = WoW120Spells::Warlock::MORTAL_COIL,
    HOWL_OF_TERROR_DESTRO    = WoW120Spells::Warlock::HOWL_OF_TERROR,
    FEAR_DESTRO              = WoW120Spells::Warlock::FEAR,
    BANISH_DESTRO            = WoW120Spells::Warlock::BANISH,
    DEMONIC_CIRCLE_TELEPORT_DESTRO = WoW120Spells::Warlock::DEMONIC_CIRCLE_TELEPORT,
    DEMONIC_GATEWAY_DESTRO   = WoW120Spells::Warlock::DEMONIC_GATEWAY,
    BURNING_RUSH_DESTRO      = WoW120Spells::Warlock::BURNING_RUSH,

    // Procs and Buffs - Using central registry: WoW120Spells::Warlock::Destruction
    BACKDRAFT_BUFF           = WoW120Spells::Warlock::Destruction::BACKDRAFT_BUFF,
    REVERSE_ENTROPY          = WoW120Spells::Warlock::Destruction::REVERSE_ENTROPY,
    ERADICATION              = WoW120Spells::Warlock::Destruction::ERADICATION,
    FLASHOVER                = WoW120Spells::Warlock::Destruction::FLASHOVER,

    // Talents - Using central registry: WoW120Spells::Warlock::Destruction
    ROARING_BLAZE            = WoW120Spells::Warlock::Destruction::ROARING_BLAZE,
    INTERNAL_COMBUSTION      = WoW120Spells::Warlock::Destruction::INTERNAL_COMBUSTION,
    FIRE_AND_BRIMSTONE       = WoW120Spells::Warlock::Destruction::FIRE_AND_BRIMSTONE,
    INFERNO                  = WoW120Spells::Warlock::Destruction::INFERNO,
    GRIMOIRE_OF_SUPREMACY    = WoW120Spells::Warlock::Destruction::GRIMOIRE_OF_SUPREMACY
};

// Dual resource type for Destruction Warlock
struct ManaSoulShardResourceDestro
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
            return true;
        }
        return false;
    }

    void Regenerate(uint32 diff)
    {
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
// DESTRUCTION IMMOLATE TRACKER
// ============================================================================

class DestructionImmolateTracker
{
public:
    DestructionImmolateTracker()
        : _trackedTargets()
    {
    }

    void ApplyImmolate(ObjectGuid guid, uint32 duration)
    {
        _trackedTargets[guid] = GameTime::GetGameTimeMS() + duration;
    }

    void RemoveImmolate(ObjectGuid guid)
    {
        _trackedTargets.erase(guid);
    }

    bool HasImmolate(ObjectGuid guid) const
    {
        auto it = _trackedTargets.find(guid);
        if (it == _trackedTargets.end())
            return false;

        return GameTime::GetGameTimeMS() < it->second;
    }

    uint32 GetTimeRemaining(ObjectGuid guid) const
    {
        auto it = _trackedTargets.find(guid);
        if (it == _trackedTargets.end())
            return 0;

        uint32 now = GameTime::GetGameTimeMS();
        return (it->second > now) ? (it->second - now) : 0;
    }

    bool NeedsRefresh(ObjectGuid guid, uint32 pandemicWindow = 5400) const
    {
        uint32 remaining = GetTimeRemaining(guid);
        return remaining < pandemicWindow; // Pandemic window
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
// DESTRUCTION HAVOC TRACKER
// ============================================================================

class DestructionHavocTracker
{
public:
    DestructionHavocTracker()
        : _havocTargetGuid()
        , _havocEndTime(0)
        , _havocActive(false)
    {
    }

    void ApplyHavoc(ObjectGuid guid)
    {
        _havocTargetGuid = guid;
        _havocEndTime = GameTime::GetGameTimeMS() + 12000; // 12 sec duration
        _havocActive = true;
    }

    bool IsActive() const { return _havocActive; }
    ObjectGuid GetTarget() const { return _havocTargetGuid; }

    void Update()
    {
        if (_havocActive && GameTime::GetGameTimeMS() >= _havocEndTime)
        {
            _havocActive = false;
            _havocTargetGuid = ObjectGuid::Empty;
            _havocEndTime = 0;
        }
    }

private:
    ObjectGuid _havocTargetGuid;
    uint32 _havocEndTime;
    bool _havocActive;
};

// ============================================================================
// DESTRUCTION WARLOCK REFACTORED
// ============================================================================

class DestructionWarlockRefactored : public RangedDpsSpecialization<ManaSoulShardResourceDestro>
{
public:
    // Use base class members with type alias for cleaner syntax
    using Base = RangedDpsSpecialization<ManaSoulShardResourceDestro>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::_resource;
    explicit DestructionWarlockRefactored(Player* bot)
        : RangedDpsSpecialization<ManaSoulShardResourceDestro>(bot)
        , _immolateTracker()
        , _havocTracker()
        , _backdraftStacks(0)
        , _lastInfernalTime(0)
    {
        // Initialize mana/soul shard resources (safe with IsInWorld check)
        this->_resource.Initialize(bot);

        // Note: Do NOT call bot->GetName() here - Player data may not be loaded yet
        // Logging will happen once bot is fully active
        TC_LOG_DEBUG("playerbot", "DestructionWarlockRefactored created for bot GUID: {}",
            bot ? bot->GetGUID().GetCounter() : 0);

        // Phase 5: Initialize decision systems
        InitializeDestructionMechanics();
    }

    void UpdateRotation(::Unit* target) override
    {
        if (!target || !target->IsAlive() || !target->IsHostileTo(this->GetBot()))
            return;

        // Detect hero talents if not yet cached
        if (!_heroTalents.detected)
            _heroTalents.Refresh(this->GetBot());

        // Hero talent rotation branches
        if (_heroTalents.IsTree(HeroTalentTree::HELLCALLER))
        {
            // Hellcaller: Wither replaces Immolate with stacking fire damage
            if (!target->HasAura(WoW120Spells::Warlock::Destruction::DESTRO_WITHER) &&
                this->CanCastSpell(WoW120Spells::Warlock::Destruction::DESTRO_WITHER, target))
            {
                this->CastSpell(WoW120Spells::Warlock::Destruction::DESTRO_WITHER, target);
                return;
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::DIABOLIST))
        {
            // Diabolist: Diabolic Ritual for empowered infernal summoning
            if (this->CanCastSpell(WoW120Spells::Warlock::Destruction::DESTRO_DIABOLIC_RITUAL, target))
            {
                this->CastSpell(WoW120Spells::Warlock::Destruction::DESTRO_DIABOLIC_RITUAL, target);
                return;
            }
        }

        // Update Destruction state
        UpdateDestructionState();

        // NOTE: Pet summoning moved to UpdateBuffs() since summons have 6s cast time
        // and must be done OUT OF COMBAT. See UpdateBuffs() below.

        // Determine if AoE or single target
        uint32 enemyCount = this->GetEnemiesInRange(40.0f);
        if (enemyCount >= 3)
        {
            ExecuteAoERotation(target, enemyCount);
        }
        else if (enemyCount == 2)
        {
            ExecuteCleaveRotation(target);
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

        // CRITICAL: Summon pet OUT OF COMBAT (6 second cast time!)
        // This must be called in UpdateBuffs, not UpdateRotation
        EnsurePetActive();

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

        // Primary purpose: Ensure pet is summoned out of combat
        EnsurePetActive();
    }

    void ExecuteSingleTargetRotation(::Unit* target)
    {
        ObjectGuid targetGuid = target->GetGUID();
        uint32 shards = this->_resource.soulShards;
        float targetHpPct = target->GetHealthPct();

        // Priority 1: Use Summon Infernal (major CD)
        if (shards >= 2 && this->CanCastSpell(SUMMON_INFERNAL, this->GetBot()))
        {
            this->CastSpell(SUMMON_INFERNAL, this->GetBot());
            _lastInfernalTime = GameTime::GetGameTimeMS();
            

        // Register cooldowns using CooldownManager
        // COMMENTED OUT:         _cooldowns.RegisterBatch({
        // COMMENTED OUT:             {CONFLAGRATE, 13000, 1},
        // COMMENTED OUT:             {SUMMON_INFERNAL, CooldownPresets::MAJOR_OFFENSIVE, 1},
        // COMMENTED OUT:             {DARK_SOUL_INSTABILITY, CooldownPresets::MINOR_OFFENSIVE, 1},
        // COMMENTED OUT:             {SOUL_FIRE, 20000, 1},
        // COMMENTED OUT:             {CATACLYSM, CooldownPresets::OFFENSIVE_30, 1},
        // COMMENTED OUT:             {HAVOC, CooldownPresets::OFFENSIVE_30, 1},
        // COMMENTED OUT:             {SHADOWBURN, 12000, 1},
        // COMMENTED OUT:             {UNENDING_RESOLVE_DESTRO, CooldownPresets::MAJOR_OFFENSIVE, 1},
        // COMMENTED OUT:             {DARK_PACT_DESTRO, CooldownPresets::OFFENSIVE_60, 1},
        // COMMENTED OUT:             {MORTAL_COIL_DESTRO, CooldownPresets::OFFENSIVE_45, 1},
        // COMMENTED OUT:             {HOWL_OF_TERROR_DESTRO, 40000, 1},
        // COMMENTED OUT:         });

        TC_LOG_DEBUG("playerbot", "Destruction: Summon Infernal");
            // Continue rotation during Infernal
        }

        // Priority 2: Dark Soul: Instability (burst CD)
        if (this->CanCastSpell(DARK_SOUL_INSTABILITY, this->GetBot()))
        {
            this->CastSpell(DARK_SOUL_INSTABILITY, this->GetBot());
            TC_LOG_DEBUG("playerbot", "Destruction: Dark Soul Instability");
        }

        // Priority 3: Maintain Immolate
        if (_immolateTracker.NeedsRefresh(targetGuid))
        {
            if (this->CanCastSpell(IMMOLATE, target))
            {
                this->CastSpell(IMMOLATE, target);
                _immolateTracker.ApplyImmolate(targetGuid, 18000); // 18 sec duration
                return;
            }
        }

        // Priority 4: Conflagrate (generate shards + Backdraft)
        if (this->CanCastSpell(CONFLAGRATE, target))
        {
            this->CastSpell(CONFLAGRATE, target);
            GenerateSoulShard(1);
            _backdraftStacks = ::std::min(_backdraftStacks + 2, 4u); // Grants 2 stacks
            return;
        }

        // Priority 5: Soul Fire (talent, strong direct damage)
        if (this->CanCastSpell(SOUL_FIRE, target))
        {
            this->CastSpell(SOUL_FIRE, target);
            return;
        }

        // Priority 6: Chaos Bolt (shard spender)
        if (shards >= 2 && this->CanCastSpell(CHAOS_BOLT, target))        {
            this->CastSpell(CHAOS_BOLT, target);
            ConsumeSoulShard(2);
            return;
        }

        // Priority 7: Channel Demonfire (talent, requires Immolate)
        if (_immolateTracker.HasImmolate(targetGuid) && this->CanCastSpell(CHANNEL_DEMONFIRE, target))
        {
            this->CastSpell(CHANNEL_DEMONFIRE, target);
            return;
        }

        // Priority 8: Shadowburn (execute < 20%)
        if (targetHpPct < 20.0f && this->CanCastSpell(SHADOWBURN, target))
        {
            this->CastSpell(SHADOWBURN, target);
            GenerateSoulShard(1);
            return;
        }        // Priority 9: Incinerate (filler + shard gen)
        if (shards < 5 && this->CanCastSpell(INCINERATE, target))
        {
            this->CastSpell(INCINERATE, target);
            GenerateSoulShard(1);
            if (_backdraftStacks > 0)
                _backdraftStacks--;
            return;
        }
    }

    void ExecuteCleaveRotation(::Unit* target)
    {
        ObjectGuid targetGuid = target->GetGUID();
        uint32 shards = this->_resource.soulShards;

        // Priority 1: Havoc on secondary target
        if (!_havocTracker.IsActive() && this->CanCastSpell(HAVOC, target))
        {
            // Find best secondary target for Havoc (not the primary target)
            ::Unit* secondaryTarget = FindBestHavocTarget(target);
            if (secondaryTarget)
            {
                if (this->CastSpell(HAVOC, secondaryTarget))
                {
                    _havocTracker.ApplyHavoc(secondaryTarget->GetGUID());
                    TC_LOG_DEBUG("playerbot", "Destruction: Havoc applied to {} (secondary target, primary: {})",
                                 secondaryTarget->GetName(), target->GetName());
                }
            }
        }

        // Priority 2: Maintain Immolate on primary
        if (_immolateTracker.NeedsRefresh(targetGuid))
        {
            if (this->CanCastSpell(IMMOLATE, target))
            {
                this->CastSpell(IMMOLATE, target);
                _immolateTracker.ApplyImmolate(targetGuid, 18000);
                return;
            }
        }

        // Priority 3: Conflagrate
        if (this->CanCastSpell(CONFLAGRATE, target))
        {
            this->CastSpell(CONFLAGRATE, target);
            GenerateSoulShard(1);
            _backdraftStacks = ::std::min(_backdraftStacks + 2, 4u);
            return;
        }

        // Priority 4: Chaos Bolt (cleaves with Havoc)
        if (shards >= 2 && this->CanCastSpell(CHAOS_BOLT, target))
        {
            this->CastSpell(CHAOS_BOLT, target);
            ConsumeSoulShard(2);
            return;
        }

        // Priority 5: Incinerate filler
        if (shards < 5 && this->CanCastSpell(INCINERATE, target))
        {
            this->CastSpell(INCINERATE, target);
            GenerateSoulShard(1);
            if (_backdraftStacks > 0)
                _backdraftStacks--;
            return;
        }
    }

    void ExecuteAoERotation(::Unit* target, uint32 enemyCount)
    {
        uint32 shards = this->_resource.soulShards;

        // Priority 1: Summon Infernal
        if (shards >= 2 && this->CanCastSpell(SUMMON_INFERNAL, this->GetBot()))
        {
            this->CastSpell(SUMMON_INFERNAL, this->GetBot());
            _lastInfernalTime = GameTime::GetGameTimeMS();
            return;
        }

        // Priority 2: Cataclysm (AoE + applies Immolate)
        if (this->CanCastSpell(CATACLYSM, target))
        {
            this->CastSpell(CATACLYSM, target);
            TC_LOG_DEBUG("playerbot", "Destruction: Cataclysm");
            return;
        }

        // Priority 3: Rain of Fire (AoE shard spender)
        if (shards >= 3 && this->CanCastSpell(RAIN_OF_FIRE, this->GetBot()))
        {
            this->CastSpell(RAIN_OF_FIRE, this->GetBot());
            ConsumeSoulShard(3);
            return;
        }

        // Priority 4: Channel Demonfire (if targets have Immolate)
        if (this->CanCastSpell(CHANNEL_DEMONFIRE, target))
        {
            this->CastSpell(CHANNEL_DEMONFIRE, target);
            return;
        }

        // Priority 5: Havoc on secondary target
        if (!_havocTracker.IsActive() && this->CanCastSpell(HAVOC, target))
        {
            this->CastSpell(HAVOC, target);
            _havocTracker.ApplyHavoc(target->GetGUID());
            return;
        }

        // Priority 6: Conflagrate
        if (this->CanCastSpell(CONFLAGRATE, target))
        {
            this->CastSpell(CONFLAGRATE, target);
            GenerateSoulShard(1);
            return;
        }

        // Priority 7: Incinerate filler
        if (shards < 5 && this->CanCastSpell(INCINERATE, target))
        {
            this->CastSpell(INCINERATE, target);
            GenerateSoulShard(1);
            return;
        }
    }

    void HandleDefensiveCooldowns()
    {
        Player* bot = this->GetBot();
        float healthPct = bot->GetHealthPct();

        // Unending Resolve
        if (healthPct < 40.0f && this->CanCastSpell(UNENDING_RESOLVE_DESTRO, bot))
        {
            this->CastSpell(UNENDING_RESOLVE_DESTRO, bot);
            TC_LOG_DEBUG("playerbot", "Destruction: Unending Resolve");
            return;
        }

        // Dark Pact
        if (healthPct < 50.0f && this->CanCastSpell(DARK_PACT_DESTRO, bot))
        {
            this->CastSpell(DARK_PACT_DESTRO, bot);
            TC_LOG_DEBUG("playerbot", "Destruction: Dark Pact");
            return;
        }

        // Mortal Coil
        if (healthPct < 60.0f && this->CanCastSpell(MORTAL_COIL_DESTRO, bot))
        {
            this->CastSpell(MORTAL_COIL_DESTRO, bot);
            TC_LOG_DEBUG("playerbot", "Destruction: Mortal Coil");
            return;
        }
    }

    void EnsurePetActive()
    {
        // THREAD-SAFETY FIX: Store GUID first, then use ObjectAccessor::FindPlayer
        // to get a validated pointer. This prevents use-after-free when bot is deleted
        // by main thread while worker thread is executing this function.
        Player* initialBot = this->GetBot();
        if (!initialBot)
            return;

        // Store GUID for thread-safe lookup
        ObjectGuid botGuid = initialBot->GetGUID();

        // Get validated pointer via ObjectAccessor (thread-safe)
        Player* bot = ObjectAccessor::FindPlayer(botGuid);
        if (!bot)
        {
            // Bot was deleted between GetBot() and FindPlayer() - race condition avoided
            return;
        }

        // Don't summon while casting (6s cast time!)
        if (bot->HasUnitState(UNIT_STATE_CASTING))
            return;

        // Check if pet exists
        if (Pet* pet = bot->GetPet())
        {
            if (pet->IsAlive())
                return; // Pet is active
        }

        // CRITICAL: For self-cast spells like pet summons, pass nullptr as target!
        // Passing 'bot' causes CanCastSpell to fail because bot->IsFriendlyTo(bot) = true

        // Priority 1: Imp (best for Destruction - ranged DPS)
        if (bot->HasSpell(SUMMON_IMP_DESTRO) && this->CanCastSpell(SUMMON_IMP_DESTRO, nullptr))
        {
            this->CastSpell(SUMMON_IMP_DESTRO, bot);
            TC_LOG_INFO("playerbot", "Destruction {}: Summoning Imp", bot->GetName());
            return;
        }

        // Diagnostic: Show which spells the bot actually has
        TC_LOG_DEBUG("playerbot", "Destruction {}: No pet summon spell available (level {}) - HasSpell: Imp={}",
                     bot->GetName(), bot->GetLevel(),
                     bot->HasSpell(SUMMON_IMP_DESTRO) ? "Y" : "N");
    }

private:
    void UpdateDestructionState()
    {
        // Update Immolate tracker
        _immolateTracker.Update();

        // Update Havoc tracker
        _havocTracker.Update();

        // Update Backdraft stacks
        if (this->GetBot())
        {
            if (Aura* aura = this->GetBot()->GetAura(BACKDRAFT_BUFF))
                _backdraftStacks = aura->GetStackAmount();
                else
                _backdraftStacks = 0;
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

    ::Unit* FindBestHavocTarget(::Unit* primaryTarget)
    {
        if (!primaryTarget || !this->GetBot())
            return nullptr;

        Player* bot = this->GetBot();
        ObjectGuid primaryGuid = primaryTarget->GetGUID();

        // Get all nearby enemies within 40 yards
        ::std::list<::Unit*> nearbyEnemies;
        Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(bot, bot, 40.0f);
        Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(bot, nearbyEnemies, u_check);
        Cell::VisitAllObjects(bot, searcher, 40.0f);

        ::Unit* bestTarget = nullptr;
        float bestScore = 0.0f;

        for (::Unit* enemy : nearbyEnemies)
        {
            // Skip invalid targets, dead targets, or the primary target
            if (!enemy || !enemy->IsAlive() || !bot->IsValidAttackTarget(enemy))
                continue;

            if (enemy->GetGUID() == primaryGuid)
                continue; // Don't Havoc the primary target

            // Calculate target priority score
            float score = 100.0f;

            // Prefer targets with high health (long-lived targets)
            float healthPct = enemy->GetHealthPct();
            if (healthPct > 80.0f)
                score += 50.0f;
            else if (healthPct > 50.0f)
                score += 30.0f;
            else if (healthPct < 20.0f)
                score -= 20.0f; // Deprioritize targets about to die

            // Prefer targets close to the primary target (for cleave efficiency)
            float distanceToPrimary = enemy->GetDistance(primaryTarget);
            if (distanceToPrimary < 10.0f)
                score += 40.0f;
            else if (distanceToPrimary < 20.0f)
                score += 20.0f;
            else if (distanceToPrimary > 30.0f)
                score -= 30.0f; // Deprioritize far targets

            // Prefer targets without Havoc already applied
            if (_havocTracker.GetTarget() == enemy->GetGUID())
                score -= 100.0f;

            // Prefer elite/boss targets over normal mobs
            if (enemy->GetTypeId() == TYPEID_UNIT)
            {
                Creature* creature = enemy->ToCreature();
                if (creature)
                {
                    if (creature->isWorldBoss() || creature->IsDungeonBoss())
                        score += 100.0f; // Highest priority for bosses
                    // DESIGN NOTE: Elite creature classification for targeting priority
                    // Implementation should use TrinityCore 12.0 CreatureTemplate API.
                    // CreatureTemplate no longer has 'rank' field in current API.
                    // Should use creature->GetCreatureTemplate()->Classification or similar.
                    // Reference: TrinityCore 12.0 CreatureTemplate structure
                    // else if (creature->GetCreatureTemplate()->rank >= CREATURE_ELITE_ELITE)
                    //     score += 50.0f; // High priority for elites
                }
            }

            // Update best target if this one has a higher score
            if (score > bestScore)
            {
                bestScore = score;
                bestTarget = enemy;
            }
        }

        if (bestTarget)
        {
            TC_LOG_DEBUG("playerbot", "Destruction: Found best Havoc target: {} (score: {:.1f})",
                         bestTarget->GetName(), bestScore);
        }
        else
        {
            TC_LOG_DEBUG("playerbot", "Destruction: No suitable Havoc target found");
        }

        return bestTarget;
    }

    void InitializeDestructionMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)
        BotAI* ai = this;
        if (!ai) return;

        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            // EMERGENCY: Defensive cooldowns
            queue->RegisterSpell(UNENDING_RESOLVE_DESTRO, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
            queue->AddCondition(UNENDING_RESOLVE_DESTRO, [this](Player* bot, Unit*) {
                return bot && bot->GetHealthPct() < 40.0f;
            }, "HP < 40% (damage reduction)");

            // CRITICAL: Major burst cooldown - Summon Infernal
            queue->RegisterSpell(SUMMON_INFERNAL, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(SUMMON_INFERNAL, [this](Player*, Unit*) {
                return this->_resource.soulShards >= 2;
            }, "Major CD (3min, Infernal)");

            // CRITICAL: Dark Soul: Instability
            queue->RegisterSpell(DARK_SOUL_INSTABILITY, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(DARK_SOUL_INSTABILITY, [this](Player* bot, Unit*) {
                return bot->HasSpell(DARK_SOUL_INSTABILITY);
            }, "Burst CD (2min, crit buff)");

            // HIGH: Maintain Immolate
            queue->RegisterSpell(IMMOLATE, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(IMMOLATE, [this](Player*, Unit* target) {
                return target && this->_immolateTracker.NeedsRefresh(target->GetGUID());
            }, "Refresh Immolate (enables Conflagrate)");

            // HIGH: Conflagrate (shard generation + Backdraft)
            queue->RegisterSpell(CONFLAGRATE, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(CONFLAGRATE, [](Player*, Unit* target) {
                return target != nullptr;
            }, "Shard gen + Backdraft (2 stacks)");

            // HIGH: Soul Fire (talent, strong damage)
            queue->RegisterSpell(SOUL_FIRE, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SOUL_FIRE, [this](Player* bot, Unit* target) {
                return target && bot->HasSpell(SOUL_FIRE);
            }, "Strong direct damage (20s CD, talent)");

            // MEDIUM: Chaos Bolt (shard spender)
            queue->RegisterSpell(CHAOS_BOLT, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(CHAOS_BOLT, [this](Player*, Unit* target) {
                return target && this->_resource.soulShards >= 2;
            }, "2 shards (heavy damage)");

            // MEDIUM: Rain of Fire (AoE shard spender)
            queue->RegisterSpell(RAIN_OF_FIRE, SpellPriority::MEDIUM, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(RAIN_OF_FIRE, [this](Player*, Unit*) {
                return this->_resource.soulShards >= 3 && this->GetEnemiesInRange(40.0f) >= 3;
            }, "3 shards, 3+ enemies (AoE)");

            // MEDIUM: Havoc (cleave on 2nd target)
            queue->RegisterSpell(HAVOC, SpellPriority::MEDIUM, SpellCategory::UTILITY);
            queue->AddCondition(HAVOC, [this](Player*, Unit* target) {
                return target && !this->_havocTracker.IsActive() && this->GetEnemiesInRange(40.0f) >= 2;
            }, "2+ enemies (cleave to 2nd target)");

            // MEDIUM: Cataclysm (AoE + applies Immolate)
            queue->RegisterSpell(CATACLYSM, SpellPriority::MEDIUM, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(CATACLYSM, [this](Player* bot, Unit* target) {
                return target && bot->HasSpell(CATACLYSM) && this->GetEnemiesInRange(40.0f) >= 3;
            }, "3+ enemies (AoE + Immolate, 30s CD)");

            // MEDIUM: Channel Demonfire (requires Immolate)
            queue->RegisterSpell(CHANNEL_DEMONFIRE, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(CHANNEL_DEMONFIRE, [this](Player* bot, Unit* target) {
                return target && bot->HasSpell(CHANNEL_DEMONFIRE) &&
                       this->_immolateTracker.HasImmolate(target->GetGUID());
            }, "Requires Immolate (channeled, talent)");

            // MEDIUM: Shadowburn (execute)
            queue->RegisterSpell(SHADOWBURN, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SHADOWBURN, [this](Player* bot, Unit* target) {
                return target && bot->HasSpell(SHADOWBURN) && target->GetHealthPct() < 20.0f;
            }, "Execute < 20% (generates shard)");

            // LOW: Incinerate (filler + shard generator)
            queue->RegisterSpell(INCINERATE, SpellPriority::LOW, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(INCINERATE, [this](Player*, Unit* target) {
                return target && this->_resource.soulShards < 5;
            }, "Filler (generates shards, Backdraft)");
        }

        auto* behaviorTree = ai->GetBehaviorTree();
        if (behaviorTree)
        {
            auto root = Selector("Destruction Warlock DPS", {
                // Tier 1: Burst Cooldowns (Summon Infernal, Dark Soul)
                Sequence("Burst Cooldowns", {
                    Condition("Has shards and target", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim() && this->_resource.soulShards >= 2;
                    }),
                    Selector("Use burst cooldowns", {
                        Sequence("Summon Infernal", {
                            Condition("Can summon Infernal", [this](Player* bot, Unit*) {
                                return this->CanCastSpell(SUMMON_INFERNAL, bot);
                            }),
                            bot::ai::Action("Cast Summon Infernal", [this](Player* bot, Unit*) {
                                this->CastSpell(SUMMON_INFERNAL, bot);
                                return NodeStatus::SUCCESS;
                            })
                        }),
                        Sequence("Dark Soul: Instability", {
                            Condition("Has Dark Soul talent", [this](Player* bot, Unit*) {
                                return bot->HasSpell(DARK_SOUL_INSTABILITY);
                            }),
                            bot::ai::Action("Cast Dark Soul", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(DARK_SOUL_INSTABILITY, bot))
                                {
                                    this->CastSpell(DARK_SOUL_INSTABILITY, bot);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 2: DoT Maintenance & Shard Generation (Immolate, Conflagrate)
                Sequence("DoT & Shard Gen", {
                    Condition("Has target", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim();
                    }),
                    Selector("Maintain DoT and generate shards", {
                        Sequence("Immolate", {
                            Condition("Needs Immolate", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                return target && this->_immolateTracker.NeedsRefresh(target->GetGUID());
                            }),
                            bot::ai::Action("Cast Immolate", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(IMMOLATE, target))
                                {
                                    this->CastSpell(IMMOLATE, target);
                                    this->_immolateTracker.ApplyImmolate(target->GetGUID(), 18000);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Conflagrate", {
                            Condition("Can cast Conflagrate", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                return target && this->CanCastSpell(CONFLAGRATE, target);
                            }),
                            bot::ai::Action("Cast Conflagrate", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                this->CastSpell(CONFLAGRATE, target);
                                this->GenerateSoulShard(1);
                                this->_backdraftStacks = ::std::min(this->_backdraftStacks + 2, 4u);
                                return NodeStatus::SUCCESS;
                            })
                        })
                    })
                }),

                // Tier 3: Shard Spender (Chaos Bolt, Rain of Fire)
                Sequence("Shard Spender", {
                    Condition("Has 2+ shards and target", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim() && this->_resource.soulShards >= 2;
                    }),
                    Selector("Spend shards", {
                        Sequence("Rain of Fire (AoE)", {
                            Condition("3+ enemies and 3+ shards", [this](Player*, Unit*) {
                                return this->_resource.soulShards >= 3 && this->GetEnemiesInRange(40.0f) >= 3;
                            }),
                            bot::ai::Action("Cast Rain of Fire", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(RAIN_OF_FIRE, bot))
                                {
                                    this->CastSpell(RAIN_OF_FIRE, bot);
                                    this->ConsumeSoulShard(3);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Chaos Bolt (single target)", {
                            Condition("2+ shards", [this](Player*, Unit*) {
                                return this->_resource.soulShards >= 2;
                            }),
                            bot::ai::Action("Cast Chaos Bolt", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(CHAOS_BOLT, target))
                                {
                                    this->CastSpell(CHAOS_BOLT, target);
                                    this->ConsumeSoulShard(2);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 4: Shard Generator (Incinerate filler)
                Sequence("Shard Generator", {
                    Condition("Has target and < 5 shards", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim() && this->_resource.soulShards < 5;
                    }),
                    Selector("Generate shards", {
                        Sequence("Shadowburn (execute)", {
                            Condition("Target < 20% HP and has spell", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                return target && bot->HasSpell(SHADOWBURN) && target->GetHealthPct() < 20.0f;
                            }),
                            bot::ai::Action("Cast Shadowburn", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(SHADOWBURN, target))
                                {
                                    this->CastSpell(SHADOWBURN, target);
                                    this->GenerateSoulShard(1);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Incinerate (filler)", {
                            bot::ai::Action("Cast Incinerate", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(INCINERATE, target))
                                {
                                    this->CastSpell(INCINERATE, target);
                                    this->GenerateSoulShard(1);
                                    if (this->_backdraftStacks > 0)
                                        this->_backdraftStacks--;
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
    DestructionImmolateTracker _immolateTracker;
    DestructionHavocTracker _havocTracker;
    uint32 _backdraftStacks;
    uint32 _lastInfernalTime;

    // Hero talent detection cache (refreshed on combat start)
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot
