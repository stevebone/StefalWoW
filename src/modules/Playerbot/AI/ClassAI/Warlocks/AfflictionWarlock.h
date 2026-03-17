/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Affliction Warlock Refactored - Template-Based Implementation
 *
 * This file provides a complete, template-based implementation of Affliction Warlock
 * using the RangedDpsSpecialization with dual resource system (Mana + Soul Shards).
 */

#pragma once

#include "../Common/StatusEffectTracker.h"
#include "../Common/CooldownManager.h"
#include "../Common/RotationHelpers.h"
#include "Pet.h"

#include "../CombatSpecializationTemplates.h"
#include "../ResourceTypes.h"
#include "Player.h"
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
// NOTE: BaselineRotationManager.h no longer needed - baseline rotation check
// is now handled centrally at the dispatch level in ClassAI::OnCombatUpdate()

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
// AFFLICTION WARLOCK SPELL IDs (WoW 12.0 - The War Within)
// Central Registry: WoW120Spells::Warlock::Affliction in SpellValidation_WoW120_Part2.h
// ============================================================================

enum AfflictionWarlockSpells
{
    // DoT Spells - Using central registry: WoW120Spells::Warlock::Affliction
    AGONY                    = WoW120Spells::Warlock::Affliction::AGONY,
    CORRUPTION               = WoW120Spells::Warlock::CORRUPTION,
    UNSTABLE_AFFLICTION      = WoW120Spells::Warlock::Affliction::UNSTABLE_AFFLICTION,
    SIPHON_LIFE              = WoW120Spells::Warlock::Affliction::SIPHON_LIFE,
    HAUNT                    = WoW120Spells::Warlock::Affliction::HAUNT,

    // Direct Damage - Using central registry: WoW120Spells::Warlock
    SHADOW_BOLT_AFF          = WoW120Spells::Warlock::SHADOW_BOLT,
    DRAIN_SOUL               = WoW120Spells::Warlock::Affliction::DRAIN_SOUL,
    MALEFIC_RAPTURE          = WoW120Spells::Warlock::Affliction::MALEFIC_RAPTURE,

    // Major Cooldowns - Using central registry: WoW120Spells::Warlock::Affliction
    PHANTOM_SINGULARITY      = WoW120Spells::Warlock::Affliction::PHANTOM_SINGULARITY,
    VILE_TAINT               = WoW120Spells::Warlock::Affliction::VILE_TAINT,
    SOUL_ROT                 = WoW120Spells::Warlock::Affliction::SOUL_ROT,
    SUMMON_DARKGLARE         = WoW120Spells::Warlock::Affliction::SUMMON_DARKGLARE,
    DARK_SOUL_MISERY         = WoW120Spells::Warlock::Affliction::DARK_SOUL_MISERY,

    // AoE - Using central registry: WoW120Spells::Warlock::Affliction
    SEED_OF_CORRUPTION       = WoW120Spells::Warlock::Affliction::SEED_OF_CORRUPTION,
    SOULBURN                 = WoW120Spells::Warlock::Affliction::SOULBURN,

    // Pet Management - Using central registry: WoW120Spells::Warlock
    SUMMON_IMP_AFF           = WoW120Spells::Warlock::SUMMON_IMP,
    SUMMON_VOIDWALKER_AFF    = WoW120Spells::Warlock::SUMMON_VOIDWALKER,
    SUMMON_FELHUNTER_AFF     = WoW120Spells::Warlock::SUMMON_FELHUNTER,
    SUMMON_SUCCUBUS_AFF      = WoW120Spells::Warlock::SUMMON_SUCCUBUS,
    COMMAND_DEMON_AFF        = WoW120Spells::Warlock::COMMAND_DEMON,

    // Utility - Using central registry: WoW120Spells::Warlock
    CURSE_OF_WEAKNESS        = WoW120Spells::Warlock::CURSE_OF_WEAKNESS,
    CURSE_OF_TONGUES         = WoW120Spells::Warlock::CURSE_OF_TONGUES,
    CURSE_OF_EXHAUSTION      = WoW120Spells::Warlock::CURSE_OF_EXHAUSTION,
    UNENDING_RESOLVE         = WoW120Spells::Warlock::UNENDING_RESOLVE,
    DARK_PACT                = WoW120Spells::Warlock::Affliction::DARK_PACT,
    MORTAL_COIL              = WoW120Spells::Warlock::MORTAL_COIL,
    HOWL_OF_TERROR           = WoW120Spells::Warlock::HOWL_OF_TERROR,
    FEAR                     = WoW120Spells::Warlock::FEAR,
    BANISH                   = WoW120Spells::Warlock::BANISH,
    SOULSTONE                = WoW120Spells::Warlock::SOULSTONE,

    // Defensives - Using central registry: WoW120Spells::Warlock
    HEALTH_FUNNEL            = WoW120Spells::Warlock::HEALTH_FUNNEL,
    DEMONIC_CIRCLE_TELEPORT  = WoW120Spells::Warlock::DEMONIC_CIRCLE_TELEPORT,
    DEMONIC_GATEWAY          = WoW120Spells::Warlock::DEMONIC_GATEWAY,
    BURNING_RUSH             = WoW120Spells::Warlock::BURNING_RUSH,

    // Procs and Buffs - Using central registry: WoW120Spells::Warlock::Affliction
    NIGHTFALL                = WoW120Spells::Warlock::Affliction::NIGHTFALL,
    INEVITABLE_DEMISE        = WoW120Spells::Warlock::Affliction::INEVITABLE_DEMISE,
    TORMENTED_CRESCENDO      = WoW120Spells::Warlock::Affliction::TORMENTED_CRESCENDO,

    // Talents - Using central registry: WoW120Spells::Warlock::Affliction
    GRIMOIRE_OF_SACRIFICE    = WoW120Spells::Warlock::Affliction::GRIMOIRE_OF_SACRIFICE,
    SOUL_CONDUIT             = WoW120Spells::Warlock::Affliction::SOUL_CONDUIT,
    CREEPING_DEATH           = WoW120Spells::Warlock::Affliction::CREEPING_DEATH,
    WRITHE_IN_AGONY          = WoW120Spells::Warlock::Affliction::WRITHE_IN_AGONY
};

// Dual resource type for Warlock (Mana + Soul Shards)
struct ManaSoulShardResource
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
        // Mana regenerates naturally over time
        // This is a simplified implementation
        if (mana < maxMana)
        {
            uint32 regenAmount = (maxMana / 100) * (diff / 1000);
            mana = ::std::min(mana + regenAmount, maxMana);
        }
        available = mana > 0;
    }

    [[nodiscard]] uint32 GetAvailable() const { return mana; }
    [[nodiscard]] uint32 GetMax() const { return maxMana; }

    void Initialize(Player* /*bot*/)
    {
        // CRITICAL: NEVER call GetMaxPower()/GetPower() during construction!
        // Even with IsInWorld() check, the power data may not be initialized yet
        // during bot login. Use static defaults and refresh later in UpdateRotation.
        maxMana = 100000;  // Standard max mana
        mana = 100000;
        soulShards = 0;
        available = true;
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
// AFFLICTION DOT TRACKER
// ============================================================================

class AfflictionDoTTracker
{
public:
    AfflictionDoTTracker()
        : _trackedDoTs()
    {
    }

    struct DoTInfo
    {
        uint32 spellId;
        uint32 endTime;
        uint32 stacks;
    };

    void ApplyDoT(ObjectGuid targetGuid, uint32 spellId, uint32 duration, uint32 stacks = 1)
    {
        auto& dotsOnTarget = _trackedDoTs[targetGuid];
        dotsOnTarget[spellId] = { spellId, GameTime::GetGameTimeMS() + duration, stacks };
    }

    void RemoveDoT(ObjectGuid targetGuid, uint32 spellId)
    {
        auto it = _trackedDoTs.find(targetGuid);
        if (it != _trackedDoTs.end())
        {
            it->second.erase(spellId);
            if (it->second.empty())
                _trackedDoTs.erase(it);
        }
    }

    bool HasDoT(ObjectGuid targetGuid, uint32 spellId) const
    {
        auto it = _trackedDoTs.find(targetGuid);
        if (it == _trackedDoTs.end())
            return false;

        auto dotIt = it->second.find(spellId);
        if (dotIt == it->second.end())
            return false;

        return GameTime::GetGameTimeMS() < dotIt->second.endTime;
    }

    uint32 GetDoTTimeRemaining(ObjectGuid targetGuid, uint32 spellId) const
    {
        auto it = _trackedDoTs.find(targetGuid);
        if (it == _trackedDoTs.end())
            return 0;

        auto dotIt = it->second.find(spellId);
        if (dotIt == it->second.end())
            return 0;

        uint32 now = GameTime::GetGameTimeMS();
        return (dotIt->second.endTime > now) ? (dotIt->second.endTime - now) : 0;
    }

    bool NeedsRefresh(ObjectGuid targetGuid, uint32 spellId, uint32 pandemicWindow = 5400) const
    {
        uint32 remaining = GetDoTTimeRemaining(targetGuid, spellId);
        return remaining < pandemicWindow; // Refresh if < 5.4 sec (pandemic window)
    }

    uint32 GetDoTCount(ObjectGuid targetGuid) const
    {
        auto it = _trackedDoTs.find(targetGuid);
        if (it == _trackedDoTs.end())
            return 0;

        uint32 count = 0;
        uint32 now = GameTime::GetGameTimeMS();
        for (const auto& pair : it->second)
        {
            if (now < pair.second.endTime)
                ++count;
        }
        return count;
    }

    void Update()
    {
        uint32 now = GameTime::GetGameTimeMS();
        for (auto targetIt = _trackedDoTs.begin(); targetIt != _trackedDoTs.end();)
        {
            for (auto dotIt = targetIt->second.begin(); dotIt != targetIt->second.end();)
            {
                if (now >= dotIt->second.endTime)
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
    CooldownManager _cooldowns;
    ::std::unordered_map<ObjectGuid, ::std::unordered_map<uint32, DoTInfo>> _trackedDoTs;
};

// ============================================================================
// AFFLICTION WARLOCK REFACTORED
// ============================================================================

class AfflictionWarlockRefactored : public RangedDpsSpecialization<ManaSoulShardResource>
{
public:
    using Base = RangedDpsSpecialization<ManaSoulShardResource>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::_resource;
    explicit AfflictionWarlockRefactored(Player* bot)
        : RangedDpsSpecialization<ManaSoulShardResource>(bot)
        , _dotTracker()
        , _nightfallProc(false)
        , _lastDarkglareTime(0)
    {
        // Initialize mana/soul shard resources (safe with IsInWorld check)
        this->_resource.Initialize(bot);

        // Note: Do NOT call bot->GetName() here - Player data may not be loaded yet
        // Logging will happen once bot is fully active
        TC_LOG_DEBUG("playerbot", "AfflictionWarlockRefactored created for bot GUID: {}",
            bot ? bot->GetGUID().GetCounter() : 0);

        // Phase 5: Initialize decision systems
        InitializeAfflictionMechanics();
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
            // Hellcaller: Wither replaces Corruption with stacking damage
            if (!target->HasAura(WoW120Spells::Warlock::Affliction::WITHER) &&
                this->CanCastSpell(WoW120Spells::Warlock::Affliction::WITHER, target))
            {
                this->CastSpell(WoW120Spells::Warlock::Affliction::WITHER, target);
                return;
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::SOUL_HARVESTER))
        {
            // Soul Harvester: Demonic Soul for soul shard generation burst
            if (this->CanCastSpell(WoW120Spells::Warlock::Affliction::DEMONIC_SOUL, target))
            {
                this->CastSpell(WoW120Spells::Warlock::Affliction::DEMONIC_SOUL, target);
                return;
            }
        }

        // NOTE: Baseline rotation check is now handled at the dispatch level in
        // ClassAI::OnCombatUpdate(). This method is ONLY called when the bot has
        // already chosen a specialization (level 10+ with talents). This ensures
        // clean separation of concerns and prevents code duplication across all
        // 39 specialization classes.

        // Update Affliction state
        UpdateAfflictionState();

        // NOTE: Pet summoning moved to UpdateBuffs() since summons have 6s cast time
        // and must be done OUT OF COMBAT. See UpdateBuffs() below.

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

protected:
    void ExecuteSingleTargetRotation(::Unit* target)
    {
        ObjectGuid targetGuid = target->GetGUID();
        uint32 shards = this->_resource.soulShards;
        float targetHpPct = target->GetHealthPct();

        // Priority 1: Use Darkglare when all DoTs are up
        if (_dotTracker.GetDoTCount(targetGuid) >= 3 && shards >= 1)
        {
            if (this->CanCastSpell(SUMMON_DARKGLARE, this->GetBot()))
            {
                this->CastSpell(SUMMON_DARKGLARE, this->GetBot());
                _lastDarkglareTime = GameTime::GetGameTimeMS();
                

        // Register cooldowns using CooldownManager
        // COMMENTED OUT:         _cooldowns.RegisterBatch({
        // COMMENTED OUT:             {SUMMON_DARKGLARE, CooldownPresets::MINOR_OFFENSIVE, 1},
        // COMMENTED OUT:             {PHANTOM_SINGULARITY, CooldownPresets::OFFENSIVE_45, 1},
        // COMMENTED OUT:             {VILE_TAINT, 20000, 1},
        // COMMENTED OUT:             {SOUL_ROT, CooldownPresets::OFFENSIVE_60, 1},
        // COMMENTED OUT:             {UNENDING_RESOLVE, CooldownPresets::MAJOR_OFFENSIVE, 1},
        // COMMENTED OUT:             {DARK_PACT, CooldownPresets::OFFENSIVE_60, 1},
        // COMMENTED OUT:             {MORTAL_COIL, CooldownPresets::OFFENSIVE_45, 1},
        // COMMENTED OUT:             {HOWL_OF_TERROR, 40000, 1},
        // COMMENTED OUT:         });

        TC_LOG_DEBUG("playerbot", "Affliction: Summon Darkglare");
                return;
            }
        }

        // Priority 2: Maintain Agony (most important DoT)
        if (_dotTracker.NeedsRefresh(targetGuid, AGONY))
        {
            if (this->CanCastSpell(AGONY, target))
            {
                this->CastSpell(AGONY, target);
                _dotTracker.ApplyDoT(targetGuid, AGONY, 18000); // 18 sec duration
                return;
            }
        }

        // Priority 3: Maintain Corruption
        if (_dotTracker.NeedsRefresh(targetGuid, CORRUPTION))
        {
            if (this->CanCastSpell(CORRUPTION, target))
            {
                this->CastSpell(CORRUPTION, target);
                _dotTracker.ApplyDoT(targetGuid, CORRUPTION, 14000); // 14 sec duration
                return;
            }
        }

        // Priority 4: Maintain Unstable Affliction
        if (_dotTracker.NeedsRefresh(targetGuid, UNSTABLE_AFFLICTION))
        {
            if (this->CanCastSpell(UNSTABLE_AFFLICTION, target))
            {
                this->CastSpell(UNSTABLE_AFFLICTION, target);
                _dotTracker.ApplyDoT(targetGuid, UNSTABLE_AFFLICTION, 8000); // 8 sec duration
                GenerateSoulShard(1);
                return;
            }
        }

        // Priority 5: Maintain Siphon Life (talent)
        if (_dotTracker.NeedsRefresh(targetGuid, SIPHON_LIFE))
        {
            if (this->CanCastSpell(SIPHON_LIFE, target))
            {
                this->CastSpell(SIPHON_LIFE, target);
                _dotTracker.ApplyDoT(targetGuid, SIPHON_LIFE, 15000); // 15 sec duration
                return;
            }
        }

        // Priority 6: Phantom Singularity (talent)
        if (this->CanCastSpell(PHANTOM_SINGULARITY, target))
        {
            this->CastSpell(PHANTOM_SINGULARITY, target);
            return;
        }

        // Priority 7: Vile Taint (talent)
        if (this->CanCastSpell(VILE_TAINT, target))
        {
            this->CastSpell(VILE_TAINT, target);
            return;
        }

        // Priority 8: Malefic Rapture (spend shards)
        if (shards >= 1 && _dotTracker.GetDoTCount(targetGuid) >= 2)
        {
            if (this->CanCastSpell(MALEFIC_RAPTURE, target))
            {
                this->CastSpell(MALEFIC_RAPTURE, target);
                ConsumeSoulShard(1);
                return;
            }
        }

        // Priority 9: Drain Soul (execute < 20%)
        if (targetHpPct < 20.0f && this->CanCastSpell(DRAIN_SOUL, target))
        {
            this->CastSpell(DRAIN_SOUL, target);
            GenerateSoulShard(1);
            return;
        }

        // Priority 10: Shadow Bolt (filler + shard gen)
        // NOTE: For level 10 warlocks, this is often the only damage spell available!
        TC_LOG_INFO("playerbot", "Affliction {}: Priority 10 - shards={}, nightfall={}, target={}",
                    this->GetBot()->GetName(), shards, _nightfallProc ? "true" : "false", target->GetName());

        // Try Shadow Bolt regardless of shard count for low-level warlocks
        bool canCastShadowBolt = this->CanCastSpell(SHADOW_BOLT_AFF, target);
        TC_LOG_INFO("playerbot", "Affliction {}: CanCastSpell(SHADOW_BOLT_AFF={})={}",
                    this->GetBot()->GetName(), static_cast<uint32>(SHADOW_BOLT_AFF), canCastShadowBolt ? "YES" : "NO");

        if (_nightfallProc || canCastShadowBolt)
        {
            TC_LOG_INFO("playerbot", "Affliction {}: CASTING Shadow Bolt on {}",
                        this->GetBot()->GetName(), target->GetName());
            this->CastSpell(SHADOW_BOLT_AFF, target);
            _nightfallProc = false;
            if (shards < 5)
                GenerateSoulShard(1);
            return;
        }
        else
        {
            TC_LOG_WARN("playerbot", "Affliction {}: Cannot cast Shadow Bolt! HasSpell={}",
                        this->GetBot()->GetName(), this->GetBot()->HasSpell(SHADOW_BOLT_AFF) ? "YES" : "NO");
        }
    }

    void ExecuteAoERotation(::Unit* target, uint32 enemyCount)
    {
        uint32 shards = this->_resource.soulShards;

        // Priority 1: Soul Rot (AoE DoT)
        if (this->CanCastSpell(SOUL_ROT, target))
        {
            this->CastSpell(SOUL_ROT, target);
            return;
        }

        // Priority 2: Vile Taint (AoE DoT)
        if (this->CanCastSpell(VILE_TAINT, target))
        {
            this->CastSpell(VILE_TAINT, target);
            return;
        }

        // Priority 3: Seed of Corruption (AoE spread)
        if (enemyCount >= 4 && this->CanCastSpell(SEED_OF_CORRUPTION, target))
        {
            this->CastSpell(SEED_OF_CORRUPTION, target);
            return;
        }

        // Priority 4: Apply Agony on all targets (multi-target DoT tracking)
        {
            Player* bot = this->GetBot();
            if (!bot) return;

            // Get all nearby enemies within 40 yards
            ::std::list<::Unit*> nearbyEnemies;
            Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(bot, bot, 40.0f);
            Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(bot, nearbyEnemies, u_check);
            Cell::VisitAllObjects(bot, searcher, 40.0f);

            // Limit DoT spreading to prevent excessive target count
            uint32 dotTargetsProcessed = 0;
            const uint32 MAX_DOT_TARGETS = 8; // Limit to 8 targets for performance

            for (::Unit* enemy : nearbyEnemies)
            {
                if (!enemy || !enemy->IsAlive() || !bot->IsValidAttackTarget(enemy))
                    continue;

                if (dotTargetsProcessed >= MAX_DOT_TARGETS)
                    break;

                ObjectGuid enemyGuid = enemy->GetGUID();

                // Priority: Apply Agony if missing or needs refresh
                if (_dotTracker.NeedsRefresh(enemyGuid, AGONY, 5000)) // Refresh with 5s remaining
                {
                    if (this->CanCastSpell(AGONY, enemy))
                    {
                        if (this->CastSpell(AGONY, enemy))
                        {
                            _dotTracker.ApplyDoT(enemyGuid, AGONY, 18000, 1); // 18s duration
                            TC_LOG_DEBUG("playerbot", "Affliction: Applied Agony to {} in AoE rotation",
                                         enemy->GetName());
                            dotTargetsProcessed++;
                            return; // Return after applying one DoT per update to avoid ability spam
                        }
                    }
                }

                // Apply Corruption if missing (secondary priority)
                if (!_dotTracker.HasDoT(enemyGuid, CORRUPTION))
                {
                    if (this->CanCastSpell(CORRUPTION, enemy))
                    {
                        if (this->CastSpell(CORRUPTION, enemy))
                        {
                            _dotTracker.ApplyDoT(enemyGuid, CORRUPTION, 14000); // 14s duration
                            TC_LOG_DEBUG("playerbot", "Affliction: Applied Corruption to {} in AoE rotation",
                                         enemy->GetName());
                            dotTargetsProcessed++;
                            return; // Return after applying one DoT per update
                        }
                    }
                }

                // Apply Siphon Life if talented and missing (tertiary priority)
                if (bot->HasSpell(SIPHON_LIFE) && !_dotTracker.HasDoT(enemyGuid, SIPHON_LIFE))
                {
                    if (this->CanCastSpell(SIPHON_LIFE, enemy))
                    {
                        if (this->CastSpell(SIPHON_LIFE, enemy))
                        {
                            _dotTracker.ApplyDoT(enemyGuid, SIPHON_LIFE, 15000); // 15s duration
                            TC_LOG_DEBUG("playerbot", "Affliction: Applied Siphon Life to {} in AoE rotation",
                                         enemy->GetName());
                            dotTargetsProcessed++;
                            return; // Return after applying one DoT per update
                        }
                    }
                }

                dotTargetsProcessed++;
            }
        }

        // Priority 5: Malefic Rapture (AoE shard spender)
        if (shards >= 2 && this->CanCastSpell(MALEFIC_RAPTURE, target))
        {
            this->CastSpell(MALEFIC_RAPTURE, target);
            ConsumeSoulShard(2);
            return;
        }

        // Priority 6: Shadow Bolt filler
        if (shards < 5 && this->CanCastSpell(SHADOW_BOLT_AFF, target))
        {
            this->CastSpell(SHADOW_BOLT_AFF, target);
            GenerateSoulShard(1);
            return;
        }
    }

    void HandleDefensiveCooldowns()
    {
        Player* bot = this->GetBot();
        float healthPct = bot->GetHealthPct();

        // Unending Resolve
        if (healthPct < 40.0f && this->CanCastSpell(UNENDING_RESOLVE, bot))
        {
            this->CastSpell(UNENDING_RESOLVE, bot);
            TC_LOG_DEBUG("playerbot", "Affliction: Unending Resolve");
            return;
        }

        // Dark Pact (talent shield)
        if (healthPct < 50.0f && this->CanCastSpell(DARK_PACT, bot))
        {
            this->CastSpell(DARK_PACT, bot);
            TC_LOG_DEBUG("playerbot", "Affliction: Dark Pact");
            return;
        }

        // Mortal Coil (heal + fear)
        if (healthPct < 60.0f && this->CanCastSpell(MORTAL_COIL, bot))
        {
            this->CastSpell(MORTAL_COIL, bot);
            TC_LOG_DEBUG("playerbot", "Affliction: Mortal Coil");
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

        // Check if pet exists and is alive
        if (Pet* pet = bot->GetPet())
        {
            if (pet->IsAlive())
                return; // Pet is active
        }

        // CRITICAL: For self-cast spells like pet summons, pass nullptr as target!
        // Passing 'bot' causes CanCastSpell to fail because bot->IsFriendlyTo(bot) = true

        // Try summons in order of preference with fallbacks for low-level warlocks
        // Priority 1: Felhunter (best for Affliction - interrupt + dispel) - Level 30+
        if (bot->HasSpell(SUMMON_FELHUNTER_AFF) && this->CanCastSpell(SUMMON_FELHUNTER_AFF, nullptr))
        {
            this->CastSpell(SUMMON_FELHUNTER_AFF, bot);
            TC_LOG_INFO("playerbot", "Affliction {}: Summoning Felhunter", bot->GetName());
            return;
        }

        // Priority 2: Voidwalker (tank for leveling) - Level 10+
        if (bot->HasSpell(SUMMON_VOIDWALKER_AFF) && this->CanCastSpell(SUMMON_VOIDWALKER_AFF, nullptr))
        {
            this->CastSpell(SUMMON_VOIDWALKER_AFF, bot);
            TC_LOG_INFO("playerbot", "Affliction {}: Summoning Voidwalker (fallback)", bot->GetName());
            return;
        }

        // Priority 3: Imp (ranged DPS) - Level 3+
        if (bot->HasSpell(SUMMON_IMP_AFF) && this->CanCastSpell(SUMMON_IMP_AFF, nullptr))
        {
            this->CastSpell(SUMMON_IMP_AFF, bot);
            TC_LOG_INFO("playerbot", "Affliction {}: Summoning Imp (fallback)", bot->GetName());
            return;
        }

        // Diagnostic: Show which spells the bot actually has
        TC_LOG_DEBUG("playerbot", "Affliction {}: No pet summon spell available (level {}) - HasSpell: Imp={}, Voidwalker={}, Felhunter={}",
                     bot->GetName(), bot->GetLevel(),
                     bot->HasSpell(SUMMON_IMP_AFF) ? "Y" : "N",
                     bot->HasSpell(SUMMON_VOIDWALKER_AFF) ? "Y" : "N",
                     bot->HasSpell(SUMMON_FELHUNTER_AFF) ? "Y" : "N");
    }

private:
    void UpdateAfflictionState()
    {
        // Update DoT tracker
        _dotTracker.Update();

        // Update Nightfall proc
        if (this->GetBot() && this->GetBot()->HasAura(NIGHTFALL))
            _nightfallProc = true;
        else
            _nightfallProc = false;

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

    void InitializeAfflictionMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)
        BotAI* ai = this;
        if (!ai) return;

        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            // EMERGENCY: Defensive cooldowns
            queue->RegisterSpell(UNENDING_RESOLVE, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
            queue->AddCondition(UNENDING_RESOLVE, [this](Player* bot, Unit*) {
                return bot && bot->GetHealthPct() < 40.0f;
            }, "HP < 40% (damage reduction)");

            // CRITICAL: Major burst cooldown - Darkglare extends all DoTs
            queue->RegisterSpell(SUMMON_DARKGLARE, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(SUMMON_DARKGLARE, [this](Player* bot, Unit* target) {
                return target && this->_dotTracker.GetDoTCount(target->GetGUID()) >= 3;
            }, "3+ DoTs active (extend duration)");

            // HIGH: Core DoTs (highest to lowest priority)
            queue->RegisterSpell(AGONY, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(AGONY, [this](Player*, Unit* target) {
                return target && this->_dotTracker.NeedsRefresh(target->GetGUID(), AGONY);
            }, "Refresh Agony (pandemic window)");

            queue->RegisterSpell(CORRUPTION, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(CORRUPTION, [this](Player*, Unit* target) {
                return target && this->_dotTracker.NeedsRefresh(target->GetGUID(), CORRUPTION);
            }, "Refresh Corruption");

            queue->RegisterSpell(UNSTABLE_AFFLICTION, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(UNSTABLE_AFFLICTION, [this](Player*, Unit* target) {
                return target && this->_dotTracker.NeedsRefresh(target->GetGUID(), UNSTABLE_AFFLICTION);
            }, "Refresh UA (generates shard)");

            queue->RegisterSpell(SIPHON_LIFE, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SIPHON_LIFE, [this](Player* bot, Unit* target) {
                return target && bot->HasSpell(SIPHON_LIFE) &&
                       this->_dotTracker.NeedsRefresh(target->GetGUID(), SIPHON_LIFE);
            }, "Refresh Siphon Life (talent)");

            // MEDIUM: Cooldown DoTs
            queue->RegisterSpell(PHANTOM_SINGULARITY, SpellPriority::MEDIUM, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(PHANTOM_SINGULARITY, [this](Player* bot, Unit* target) {
                return target && bot->HasSpell(PHANTOM_SINGULARITY);
            }, "AoE DoT (45s CD)");

            queue->RegisterSpell(VILE_TAINT, SpellPriority::MEDIUM, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(VILE_TAINT, [this](Player* bot, Unit* target) {
                return target && bot->HasSpell(VILE_TAINT);
            }, "AoE DoT (20s CD)");

            queue->RegisterSpell(SOUL_ROT, SpellPriority::MEDIUM, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(SOUL_ROT, [this](Player* bot, Unit* target) {
                return target && bot->HasSpell(SOUL_ROT);
            }, "AoE DoT (60s CD)");

            // MEDIUM: Shard spender
            queue->RegisterSpell(MALEFIC_RAPTURE, SpellPriority::MEDIUM, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(MALEFIC_RAPTURE, [this](Player*, Unit* target) {
                return target && this->_resource.soulShards >= 1 &&
                       this->_dotTracker.GetDoTCount(target->GetGUID()) >= 2;
            }, "Spend shard (2+ DoTs active)");

            // MEDIUM: Execute phase
            queue->RegisterSpell(DRAIN_SOUL, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(DRAIN_SOUL, [](Player*, Unit* target) {
                return target && target->GetHealthPct() < 20.0f;
            }, "Execute < 20% (generates shards)");

            // LOW: Filler + shard generator
            queue->RegisterSpell(SHADOW_BOLT_AFF, SpellPriority::LOW, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SHADOW_BOLT_AFF, [this](Player*, Unit* target) {
                return target && this->_resource.soulShards < 5;
            }, "Filler (generates shards)");
        }

        auto* behaviorTree = ai->GetBehaviorTree();
        if (behaviorTree)
        {
            auto root = Selector("Affliction Warlock DPS", {
                // Tier 1: Burst Window (Darkglare extends all DoTs)
                Sequence("Burst Cooldown", {
                    Condition("3+ DoTs active", [this](Player* bot, Unit*) {
                        Unit* target = bot->GetVictim();
                        return bot && target && this->_dotTracker.GetDoTCount(target->GetGUID()) >= 3;
                    }),
                    bot::ai::Action("Cast Darkglare", [this](Player* bot, Unit*) {
                        if (this->CanCastSpell(SUMMON_DARKGLARE, bot))
                        {
                            this->CastSpell(SUMMON_DARKGLARE, bot);
                            return NodeStatus::SUCCESS;
                        }
                        return NodeStatus::FAILURE;
                    })
                }),

                // Tier 2: DoT Maintenance (Agony → Corruption → UA → Siphon Life)
                Sequence("DoT Maintenance", {
                    Condition("Has target", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim();
                    }),
                    Selector("Apply/Refresh DoTs", {
                        Sequence("Agony", {
                            Condition("Needs Agony", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                return target && this->_dotTracker.NeedsRefresh(target->GetGUID(), AGONY);
                            }),
                            bot::ai::Action("Cast Agony", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(AGONY, target))
                                {
                                    this->CastSpell(AGONY, target);
                                    this->_dotTracker.ApplyDoT(target->GetGUID(), AGONY, 18000);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Corruption", {
                            Condition("Needs Corruption", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                return target && this->_dotTracker.NeedsRefresh(target->GetGUID(), CORRUPTION);
                            }),
                            bot::ai::Action("Cast Corruption", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(CORRUPTION, target))
                                {
                                    this->CastSpell(CORRUPTION, target);
                                    this->_dotTracker.ApplyDoT(target->GetGUID(), CORRUPTION, 14000);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Unstable Affliction", {
                            Condition("Needs UA", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                return target && this->_dotTracker.NeedsRefresh(target->GetGUID(), UNSTABLE_AFFLICTION);
                            }),
                            bot::ai::Action("Cast UA", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(UNSTABLE_AFFLICTION, target))
                                {
                                    this->CastSpell(UNSTABLE_AFFLICTION, target);
                                    this->_dotTracker.ApplyDoT(target->GetGUID(), UNSTABLE_AFFLICTION, 8000);
                                    this->GenerateSoulShard(1);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Siphon Life", {
                            Condition("Needs Siphon Life", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                return bot && target && bot->HasSpell(SIPHON_LIFE) &&
                                       this->_dotTracker.NeedsRefresh(target->GetGUID(), SIPHON_LIFE);
                            }),
                            bot::ai::Action("Cast Siphon Life", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(SIPHON_LIFE, target))
                                {
                                    this->CastSpell(SIPHON_LIFE, target);
                                    this->_dotTracker.ApplyDoT(target->GetGUID(), SIPHON_LIFE, 15000);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 3: Shard Spender (Malefic Rapture when 2+ DoTs active)
                Sequence("Shard Spender", {
                    Condition("1+ shards and 2+ DoTs", [this](Player* bot, Unit*) {
                        Unit* target = bot->GetVictim();
                        return bot && target && this->_resource.soulShards >= 1 &&
                               this->_dotTracker.GetDoTCount(target->GetGUID()) >= 2;
                    }),
                    bot::ai::Action("Cast Malefic Rapture", [this](Player* bot, Unit*) {
                        Unit* target = bot->GetVictim();
                        if (target && this->CanCastSpell(MALEFIC_RAPTURE, target))
                        {
                            this->CastSpell(MALEFIC_RAPTURE, target);
                            this->ConsumeSoulShard(1);
                            return NodeStatus::SUCCESS;
                        }
                        return NodeStatus::FAILURE;
                    })
                }),

                // Tier 4: Shard Generator (Drain Soul execute, Shadow Bolt filler)
                Sequence("Shard Generator", {
                    Condition("Has target and < 5 shards", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim() && this->_resource.soulShards < 5;
                    }),
                    Selector("Generate shards", {
                        Sequence("Drain Soul (execute)", {
                            Condition("Target < 20% HP", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                return target && target->GetHealthPct() < 20.0f;
                            }),
                            bot::ai::Action("Cast Drain Soul", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(DRAIN_SOUL, target))
                                {
                                    this->CastSpell(DRAIN_SOUL, target);
                                    this->GenerateSoulShard(1);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Shadow Bolt (filler)", {
                            bot::ai::Action("Cast Shadow Bolt", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(SHADOW_BOLT_AFF, target))
                                {
                                    this->CastSpell(SHADOW_BOLT_AFF, target);
                                    this->GenerateSoulShard(1);
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
    AfflictionDoTTracker _dotTracker;
    bool _nightfallProc;
    uint32 _lastDarkglareTime;

    // Hero talent detection cache (refreshed on combat start)
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot
