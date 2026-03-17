/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Preservation Evoker Refactored - Enterprise-Grade Header-Only Implementation
 *
 * This file provides a complete, template-based implementation of Preservation Evoker
 * using the HealerSpecialization with Essence resource system and Echo mechanics.
 */

#pragma once

#include "../Common/StatusEffectTracker.h"
#include "../Common/CooldownManager.h"
#include "../Common/RotationHelpers.h"
#include "../CombatSpecializationTemplates.h"
#include "../ResourceTypes.h"
#include "../SpellValidation_WoW120.h"
#include "../HeroTalentDetector.h"      // Hero talent tree detection
#include "../../Services/HealingTargetSelector.h"
#include "Player.h"
#include "SpellMgr.h"
#include "SpellAuraEffects.h"
#include "Group.h"
#include "Log.h"
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
// PRESERVATION EVOKER SPELL IDs (WoW 12.0 - The War Within)
// See central registry: WoW120Spells::Evoker and WoW120Spells::Evoker::Preservation
// ============================================================================

enum PreservationEvokerSpells
{
    // Direct Heals
    EMERALD_BLOSSOM      = WoW120Spells::Evoker::EMERALD_BLOSSOM,
    VERDANT_EMBRACE      = WoW120Spells::Evoker::VERDANT_EMBRACE,
    LIVING_FLAME_HEAL    = WoW120Spells::Evoker::Preservation::LIVING_FLAME_HEAL,

    // Empowered Heals
    DREAM_BREATH         = WoW120Spells::Evoker::Preservation::DREAM_BREATH,
    SPIRIT_BLOOM         = WoW120Spells::Evoker::Preservation::SPIRITBLOOM,

    // Echo System
    ECHO                 = WoW120Spells::Evoker::Preservation::ECHO,
    REVERSION            = WoW120Spells::Evoker::Preservation::REVERSION,

    // Major Cooldowns
    EMERALD_COMMUNION    = WoW120Spells::Evoker::Preservation::EMERALD_COMMUNION,
    TEMPORAL_ANOMALY     = WoW120Spells::Evoker::Preservation::TEMPORAL_ANOMALY,
    REWIND               = WoW120Spells::Evoker::Preservation::REWIND,

    // Utility
    LIFEBIND             = WoW120Spells::Evoker::Preservation::LIFEBIND,
    BLESSING_BRONZE      = WoW120Spells::Evoker::BLESSING_OF_THE_BRONZE,
    TIME_DILATION        = WoW120Spells::Evoker::Preservation::TIME_DILATION,
    STASIS               = WoW120Spells::Evoker::Preservation::STASIS,
    RESCUE               = WoW120Spells::Evoker::RESCUE,

    // Defensive (shared with other Evoker specs)
    PRES_OBSIDIAN_SCALES = WoW120Spells::Evoker::OBSIDIAN_SCALES,
    PRES_RENEWING_BLAZE  = WoW120Spells::Evoker::RENEWING_BLAZE,
    TWIN_GUARDIAN        = WoW120Spells::Evoker::Preservation::TWIN_GUARDIAN,

    // Essence Generation
    AZURE_STRIKE_PRES    = WoW120Spells::Evoker::AZURE_STRIKE,
    DISINTEGRATE_PRES    = WoW120Spells::Evoker::DISINTEGRATE,

    // Procs
    ESSENCE_BURST_PRES   = WoW120Spells::Evoker::Preservation::ESSENCE_BURST_PRES,
    CALL_OF_YSERA        = WoW120Spells::Evoker::Preservation::CALL_OF_YSERA,

    // Talents
    FIELD_OF_DREAMS      = WoW120Spells::Evoker::Preservation::FIELD_OF_DREAMS,
    FLOW_STATE           = WoW120Spells::Evoker::Preservation::FLOW_STATE,
    LIFEFORCE_MENDER     = WoW120Spells::Evoker::Preservation::LIFEFORCE_MENDER,
    TEMPORAL_COMPRESSION = WoW120Spells::Evoker::Preservation::TEMPORAL_COMPRESSION
};

// Essence resource (same as Devastation)
struct EssenceResourcePres
{
    uint32 essence{0};
    uint32 maxEssence{5};
    bool available{true};

    bool Consume(uint32 cost)
    {
        if (essence >= cost) {
            essence -= cost;
            return true;
        }
        return false;
    }

    void Regenerate(uint32 diff) { available = true; }
    [[nodiscard]] uint32 GetAvailable() const { return essence; }
    [[nodiscard]] uint32 GetMax() const { return maxEssence; }

    void Initialize(Player* bot)
    {
        if (bot)
        {
            essence = 0;
            maxEssence = 5;
        }
    }
};

// ============================================================================
// EMPOWERMENT TRACKING
// ============================================================================

enum class EmpowerLevelPres : uint8
{
    NONE = 0,
    RANK_1 = 1,
    RANK_2 = 2,
    RANK_3 = 3,
    RANK_4 = 4
};

class PreservationEmpowermentTracker
{
public:
    PreservationEmpowermentTracker()
        : _isChanneling(false), _currentSpellId(0), _targetLevel(EmpowerLevelPres::NONE), _channelStartTime(0)
    {}

    void StartEmpower(uint32 spellId, EmpowerLevelPres targetLevel)
    {
        _isChanneling = true;
        _currentSpellId = spellId;
        _targetLevel = targetLevel;
        _channelStartTime = GameTime::GetGameTimeMS();
    }

    void StopEmpower() { _isChanneling = false; _currentSpellId = 0; }
    bool IsChanneling() const { return _isChanneling; }
    uint32 GetSpellId() const { return _currentSpellId; }

    bool ShouldRelease() const
    {
        if (!_isChanneling) return false;
        uint32 requiredTime = static_cast<uint32>(_targetLevel) * 750;
        uint32 channelTime = GameTime::GetGameTimeMS() - _channelStartTime;
        return channelTime >= requiredTime;
    }

private:
    bool _isChanneling;
    uint32 _currentSpellId;
    EmpowerLevelPres _targetLevel;
    uint32 _channelStartTime;
};

// ============================================================================
// ECHO SYSTEM (Preservation-specific, named PresEcho to avoid conflict with EvokerAI::Echo)
// ============================================================================

struct PresEcho
{
    ObjectGuid targetGuid;
    uint32 remainingHeals;
    uint32 healAmount;
    uint32 lastHealTime;
    uint32 healInterval;

    PresEcho() : remainingHeals(0), healAmount(0), lastHealTime(0), healInterval(2000) {}

    PresEcho(ObjectGuid guid, uint32 heals, uint32 amount)
        : targetGuid(guid), remainingHeals(heals), healAmount(amount), lastHealTime(GameTime::GetGameTimeMS()), healInterval(2000) {}

    bool ShouldHeal() const
    {
        return GameTime::GetGameTimeMS() - lastHealTime >= healInterval && remainingHeals > 0;
    }

    void ProcessHeal()
    {
        if (remainingHeals > 0)
        {
            remainingHeals--;
            lastHealTime = GameTime::GetGameTimeMS();
        }
    }

    bool IsExpired() const
    {
        return remainingHeals == 0;
    }
};

class EchoTracker
{
public:
    EchoTracker() : _maxEchoes(8) {}

    void CreateEcho(ObjectGuid targetGuid, uint32 healAmount, uint32 numHeals = 4)
    {
        // Remove existing echo on same target
        RemoveEcho(targetGuid);

        // Add new echo
        if (_echoes.size() < _maxEchoes)
        {
            _echoes.emplace_back(targetGuid, numHeals, healAmount);
        }
    }

    void RemoveEcho(ObjectGuid targetGuid)
    {
        _echoes.erase(
            ::std::remove_if(_echoes.begin(), _echoes.end(),
                [targetGuid](const PresEcho& echo) { return echo.targetGuid == targetGuid; }),
            _echoes.end()
        );
    }

    void Update(Player* bot)
    {
        if (!bot) return;

        // Process echo healing
        for (auto& echo : _echoes)
        {
            if (echo.ShouldHeal())
            {
                Unit* target = ObjectAccessor::GetUnit(*bot, echo.targetGuid);
                if (target && target->IsAlive())
                {
                    // Heal target via echo
                    echo.ProcessHeal();
                }
            }
        }

        // Remove expired echoes
        _echoes.erase(
            ::std::remove_if(_echoes.begin(), _echoes.end(),
                [](const PresEcho& echo) { return echo.IsExpired(); }),
            _echoes.end()
        );
    }

    [[nodiscard]] uint32 GetActiveEchoCount() const { return _echoes.size(); }

    [[nodiscard]] bool HasEcho(ObjectGuid targetGuid) const
    {
        return ::std::any_of(_echoes.begin(), _echoes.end(),
            [targetGuid](const PresEcho& echo) { return echo.targetGuid == targetGuid; });
    }

private:
    ::std::vector<PresEcho> _echoes;
    uint32 _maxEchoes;
};

// ============================================================================
// PRESERVATION EVOKER REFACTORED
// ============================================================================

class PreservationEvokerRefactored : public HealerSpecialization<EssenceResourcePres>
{
public:
    using Base = HealerSpecialization<EssenceResourcePres>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::_resource;

    explicit PreservationEvokerRefactored(Player* bot)
        : HealerSpecialization<EssenceResourcePres>(bot)
        , _empowermentTracker()
        , _echoTracker()
        , _essenceBurstStacks(0)
    {
        // EssenceResourcePres::Initialize() is safe - only sets default values
        this->_resource.Initialize(bot);

        // Phase 5: Initialize decision systems
        InitializePreservationMechanics();

        // Register healing spell efficiency tiers
        GetEfficiencyManager().RegisterSpell(LIVING_FLAME_HEAL, HealingSpellTier::VERY_HIGH, "Living Flame");
        GetEfficiencyManager().RegisterSpell(REVERSION, HealingSpellTier::VERY_HIGH, "Reversion");
        GetEfficiencyManager().RegisterSpell(EMERALD_BLOSSOM, HealingSpellTier::MEDIUM, "Emerald Blossom");
        GetEfficiencyManager().RegisterSpell(VERDANT_EMBRACE, HealingSpellTier::HIGH, "Verdant Embrace");
        GetEfficiencyManager().RegisterSpell(DREAM_BREATH, HealingSpellTier::MEDIUM, "Dream Breath");
        GetEfficiencyManager().RegisterSpell(SPIRIT_BLOOM, HealingSpellTier::LOW, "Spirit Bloom");
        GetEfficiencyManager().RegisterSpell(EMERALD_COMMUNION, HealingSpellTier::EMERGENCY, "Emerald Communion");

        // Note: Do NOT call bot->GetName() here - Player data may not be loaded yet
        TC_LOG_DEBUG("playerbot", "PreservationEvokerRefactored created for bot GUID: {}",
            bot ? bot->GetGUID().GetCounter() : 0);
    }

    void UpdateRotation(::Unit* target) override
    {
        if (!target)
            return;

        // Detect hero talents if not yet cached
        if (!_heroTalents.detected)
            _heroTalents.Refresh(this->GetBot());

        // Hero talent rotation branches
        if (_heroTalents.IsTree(HeroTalentTree::CHRONOWARDEN))
        {
            // Chronowarden: Temporal Burst for group haste during healing
            if (this->CanCastSpell(WoW120Spells::Evoker::Preservation::TEMPORAL_BURST, this->GetBot()))
            {
                this->CastSpell(WoW120Spells::Evoker::Preservation::TEMPORAL_BURST, this->GetBot());
                return;
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::FLAMESHAPER))
        {
            // Flameshaper: Engulf for damage-healing hybrid
            if (target && target->IsHostileTo(this->GetBot()) &&
                this->CanCastSpell(WoW120Spells::Evoker::Preservation::PRES_ENGULF, target))
            {
                this->CastSpell(WoW120Spells::Evoker::Preservation::PRES_ENGULF, target);
                return;
            }
        }

        // Preservation focuses on healing, not DPS rotation
    }

    void UpdateBuffs() override
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        // Update Preservation state
        UpdatePreservationState();

        // Handle empowerment channeling
        if (_empowermentTracker.IsChanneling())
        {
            if (_empowermentTracker.ShouldRelease())
            {
                ReleaseEmpoweredSpell();
            }
            return;
        }

        // Get group members for healing
        ::std::vector<Unit*> group = GetGroupMembers();
        if (group.empty())
            return;

        // Execute healing rotation
        ExecuteHealingRotation(group);
    }

    /// Check if Essence Burst proc is active (makes next essence spender free)
    [[nodiscard]] bool HasEssenceBurstProc() const { return _essenceBurstStacks > 0; }

    /// Consume one stack of Essence Burst
    void ConsumeEssenceBurst()
    {
        if (_essenceBurstStacks > 0)
            _essenceBurstStacks--;
    }

protected:
    void ExecuteHealingRotation(const ::std::vector<Unit*>& group)
    {
        uint32 essence = this->_resource.essence;

        // Priority 1: Emergency healing
        if (HandleEmergencyHealing(group))
            return;

        // Priority 2: Consume Essence Burst proc on healing spenders (free cast)
        if (HasEssenceBurstProc())
        {
            if (HandleEssenceBurstHealing(group))
                return;
        }

        // Priority 3: Maintain Echoes
        if (HandleEchoMaintenance(group))
            return;

        // Priority 4: HoT maintenance
        if (essence >= 3 && HandleHoTMaintenance(group))
            return;

        // Priority 5: Direct healing
        if (essence >= 3 && HandleDirectHealing(group))
            return;

        // Priority 6: Generate essence if low
        if (essence < 3)
            GenerateEssence();
    }

    /// Spend Essence Burst proc on an expensive healing spell for free
    bool HandleEssenceBurstHealing(const ::std::vector<Unit*>& group)
    {
        // Find most injured target
        Unit* target = GetMostInjuredTarget(group);
        if (!target || target->GetHealthPct() > 90.0f)
            return false;

        // Emerald Blossom (normally costs 3 essence - best value for free cast)
        if (target->GetHealthPct() < 80.0f && IsHealAllowedByMana(EMERALD_BLOSSOM) &&
            this->CanCastSpell(EMERALD_BLOSSOM, this->GetBot()))
        {
            this->CastSpell(EMERALD_BLOSSOM, this->GetBot());
            ConsumeEssenceBurst(); // Free cast - don't consume essence
            return true;
        }

        // Verdant Embrace (normally costs 1 essence - good single target heal)
        if (target->GetHealthPct() < 70.0f && IsHealAllowedByMana(VERDANT_EMBRACE) &&
            this->CanCastSpell(VERDANT_EMBRACE, target))
        {
            this->CastSpell(VERDANT_EMBRACE, target);
            ConsumeEssenceBurst(); // Free cast
            return true;
        }

        return false;
    }

    bool HandleEmergencyHealing(const ::std::vector<Unit*>& group)
    {
        uint32 criticalCount = 0;
        for (Unit* member : group)
            if (member && member->GetHealthPct() < 40.0f)
                criticalCount++;

        // Emerald Communion for 3+ critical
        if (criticalCount >= 3 && this->CanCastSpell(EMERALD_COMMUNION, this->GetBot()))
        {
            this->CastSpell(EMERALD_COMMUNION, this->GetBot());
            return true;
        }

        // Spirit Bloom for single critical target
        if (criticalCount >= 1 && this->_resource.essence >= 3)
        {
            Unit* target = GetLowestHealthTarget(group);
            if (target && IsHealAllowedByMana(SPIRIT_BLOOM) && this->CanCastSpell(SPIRIT_BLOOM, target))
            {
                StartEmpoweredSpell(SPIRIT_BLOOM, EmpowerLevelPres::RANK_3, target);
                return true;
            }
        }

        return false;
    }

    bool HandleEchoMaintenance(const ::std::vector<Unit*>& group)
    {
        // Maintain 3-4 echoes on injured allies
        uint32 activeEchoes = _echoTracker.GetActiveEchoCount();
        if (activeEchoes >= 4)
            return false;

        // Find target without echo and below 95% HP
        for (Unit* member : group)
        {
            if (member && member->GetHealthPct() < 95.0f && !_echoTracker.HasEcho(member->GetGUID()))
            {
                // Cast Reversion to create echo
                if (this->_resource.essence >= 1 && this->CanCastSpell(REVERSION, member))
                {
                    this->CastSpell(REVERSION, member);
                    this->_resource.Consume(1);
                    _echoTracker.CreateEcho(member->GetGUID(), 5000, 4); // 4 heals
                    return true;
                }
            }
        }

        return false;
    }

    bool HandleHoTMaintenance(const ::std::vector<Unit*>& group)
    {
        // Dream Breath for group-wide HoT
        uint32 injuredCount = 0;
        for (Unit* member : group)
            if (member && member->GetHealthPct() < 85.0f)
                injuredCount++;

        if (injuredCount >= 3)
        {
            Unit* target = GetMostInjuredTarget(group);
            if (target && IsHealAllowedByMana(DREAM_BREATH) && this->CanCastSpell(DREAM_BREATH, target))
            {
                StartEmpoweredSpell(DREAM_BREATH, EmpowerLevelPres::RANK_2, target);
                return true;
            }
        }

        return false;
    }

    bool HandleDirectHealing(const ::std::vector<Unit*>& group)
    {
        // Emerald Blossom for AoE healing
        uint32 injuredCount = 0;
        for (Unit* member : group)
            if (member && member->GetHealthPct() < 80.0f)
                injuredCount++;

        if (injuredCount >= 3 && IsHealAllowedByMana(EMERALD_BLOSSOM) && this->CanCastSpell(EMERALD_BLOSSOM, this->GetBot()))
        {
            this->CastSpell(EMERALD_BLOSSOM, this->GetBot());
            this->_resource.Consume(3);
            return true;
        }

        // Verdant Embrace for single target
        Unit* target = GetLowestHealthTarget(group);
        if (target && target->GetHealthPct() < 70.0f && IsHealAllowedByMana(VERDANT_EMBRACE) && this->CanCastSpell(VERDANT_EMBRACE, target))
        {
            this->CastSpell(VERDANT_EMBRACE, target);
            this->_resource.Consume(1);
            return true;
        }

        return false;
    }

    void GenerateEssence()
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        Unit* target = bot->GetVictim();
        if (!target)
            target = FindNearbyEnemy();

        if (target && this->CanCastSpell(AZURE_STRIKE_PRES, target))
        {
            this->CastSpell(AZURE_STRIKE_PRES, target);
            this->_resource.essence = ::std::min(this->_resource.essence + 2, this->_resource.maxEssence);
        }
    }

    Unit* GetLowestHealthTarget(const ::std::vector<Unit*>& group) const
    {
        return bot::ai::HealingTargetSelector::SelectTarget(this->GetBot(), 30.0f, 100.0f);
    }

    Unit* GetMostInjuredTarget(const ::std::vector<Unit*>& group) const
    {
        Unit* mostInjured = nullptr;
        float lowestPct = 100.0f;

        for (Unit* member : group)
        {
            if (member && member->GetHealthPct() < lowestPct)
            {
                lowestPct = member->GetHealthPct();
                mostInjured = member;
            }
        }

        return mostInjured;
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

    Unit* FindNearbyEnemy() const
    {
        Player* bot = this->GetBot();
        if (!bot) return nullptr;

        // For healer, just return current victim if any
        // This is mainly used for essence generation with damage spells
        return bot->GetVictim();
    }

    void UpdatePreservationState()
    {
        Player* bot = this->GetBot();
        if (!bot) return;

        // Update echo system
        _echoTracker.Update(bot);

        // Sync essence with actual resource
        this->_resource.essence = bot->GetPower(POWER_ALTERNATE_POWER);

        // Update Essence Burst stacks
        if (Aura* aura = bot->GetAura(ESSENCE_BURST_PRES))
            _essenceBurstStacks = aura->GetStackAmount();
        else
            _essenceBurstStacks = 0;
    }

    void StartEmpoweredSpell(uint32 spellId, EmpowerLevelPres targetLevel, Unit* target)
    {
        _empowermentTracker.StartEmpower(spellId, targetLevel);
        this->CastSpell(spellId, target);
    }

    void ReleaseEmpoweredSpell()
    {
        Player* bot = this->GetBot();
        if (bot && bot->IsNonMeleeSpellCast(false))
        {
            bot->InterruptNonMeleeSpells(false);
        }

        this->_resource.Consume(3);
        _empowermentTracker.StopEmpower();
    }

    // ========================================================================
    // PHASE 5: DECISION SYSTEM INTEGRATION
    // ========================================================================

    void InitializePreservationMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)
        BotAI* ai = this;
        if (!ai) return;

        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            // EMERGENCY: Raid-wide emergency healing
            queue->RegisterSpell(EMERALD_COMMUNION, SpellPriority::EMERGENCY, SpellCategory::HEALING);
            queue->AddCondition(EMERALD_COMMUNION, [this](Player*, Unit*) {
                auto group = this->GetGroupMembers();
                uint32 critical = 0;
                for (auto* m : group) if (m && m->GetHealthPct() < 40.0f) critical++;
                return critical >= 3;
            }, "3+ allies < 40% HP (massive heal, 3min CD)");

            queue->RegisterSpell(REWIND, SpellPriority::EMERGENCY, SpellCategory::HEALING);
            queue->AddCondition(REWIND, [this](Player*, Unit*) {
                auto group = this->GetGroupMembers();
                for (auto* m : group) if (m && m->GetHealthPct() < 30.0f) return true;
                return false;
            }, "Ally < 30% HP (undo damage, 2.5min CD)");

            // CRITICAL: Empowered heals
            queue->RegisterSpell(SPIRIT_BLOOM, SpellPriority::CRITICAL, SpellCategory::HEALING);
            queue->AddCondition(SPIRIT_BLOOM, [this](Player*, Unit*) {
                if (this->_resource.essence < 3 || this->_empowermentTracker.IsChanneling()) return false;
                auto group = this->GetGroupMembers();
                for (auto* m : group) if (m && m->GetHealthPct() < 50.0f) return true;
                return false;
            }, "Ally < 50% HP, 3 essence (empowered smart heal)");

            queue->RegisterSpell(DREAM_BREATH, SpellPriority::CRITICAL, SpellCategory::HEALING);
            queue->AddCondition(DREAM_BREATH, [this](Player*, Unit*) {
                if (this->_resource.essence < 3 || this->_empowermentTracker.IsChanneling()) return false;
                auto group = this->GetGroupMembers();
                uint32 injured = 0;
                for (auto* m : group) if (m && m->GetHealthPct() < 85.0f) injured++;
                return injured >= 3;
            }, "3+ allies < 85% HP, 3 essence (empowered HoT)");

            // HIGH: Direct heals and Echo maintenance
            queue->RegisterSpell(EMERALD_BLOSSOM, SpellPriority::HIGH, SpellCategory::HEALING);
            queue->AddCondition(EMERALD_BLOSSOM, [this](Player*, Unit*) {
                if (this->_resource.essence < 3) return false;
                auto group = this->GetGroupMembers();
                uint32 injured = 0;
                for (auto* m : group) if (m && m->GetHealthPct() < 80.0f) injured++;
                return injured >= 3;
            }, "3+ allies < 80% HP, 3 essence (AoE heal)");

            queue->RegisterSpell(REVERSION, SpellPriority::HIGH, SpellCategory::HEALING);
            queue->AddCondition(REVERSION, [this](Player*, Unit*) {
                if (this->_resource.essence < 1) return false;
                auto group = this->GetGroupMembers();
                for (auto* m : group)
                {
                    if (m && m->GetHealthPct() < 95.0f && !this->_echoTracker.HasEcho(m->GetGUID()))
                        return true;
                }
                return false;
            }, "Ally < 95% without Echo, 1 essence (HoT + Echo)");

            queue->RegisterSpell(VERDANT_EMBRACE, SpellPriority::HIGH, SpellCategory::HEALING);
            queue->AddCondition(VERDANT_EMBRACE, [this](Player*, Unit*) {
                if (this->_resource.essence < 1) return false;
                auto group = this->GetGroupMembers();
                for (auto* m : group) if (m && m->GetHealthPct() < 70.0f) return true;
                return false;
            }, "Ally < 70% HP, 1 essence (heal + teleport)");

            // MEDIUM: Utility and support
            queue->RegisterSpell(TEMPORAL_ANOMALY, SpellPriority::MEDIUM, SpellCategory::DEFENSIVE);
            queue->AddCondition(TEMPORAL_ANOMALY, [this](Player*, Unit*) {
                auto group = this->GetGroupMembers();
                uint32 injured = 0;
                for (auto* m : group) if (m && m->GetHealthPct() < 70.0f) injured++;
                return injured >= 2;
            }, "2+ allies < 70% HP (heal after dmg, 3min CD)");

            queue->RegisterSpell(TIME_DILATION, SpellPriority::MEDIUM, SpellCategory::UTILITY);
            queue->AddCondition(TIME_DILATION, [this](Player*, Unit*) {
                return this->_echoTracker.GetActiveEchoCount() >= 3;
            }, "3+ active Echoes (extend HoTs)");

            // LOW: Essence generation
            queue->RegisterSpell(AZURE_STRIKE_PRES, SpellPriority::LOW, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(AZURE_STRIKE_PRES, [this](Player*, Unit* target) {
                return target && this->_resource.essence < 4;
            }, "Essence < 4 (generates 2 essence)");

            // DEFENSIVE: Personal survival
            queue->RegisterSpell(OBSIDIAN_SCALES, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
            queue->AddCondition(OBSIDIAN_SCALES, [](Player* bot, Unit*) {
                return bot && bot->GetHealthPct() < 40.0f;
            }, "HP < 40% (30% dmg reduction)");

            queue->RegisterSpell(RENEWING_BLAZE, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
            queue->AddCondition(RENEWING_BLAZE, [](Player* bot, Unit*) {
                return bot && bot->GetHealthPct() < 50.0f;
            }, "HP < 50% (self-heal)");
        }

        auto* behaviorTree = ai->GetBehaviorTree();
        if (behaviorTree)
        {
            auto root = Selector("Preservation Evoker Healing", {
                // Tier 1: Emergency Healing
                Sequence("Emergency Healing", {
                    Condition("3+ critical", [this](Player*, Unit*) {
                        auto group = this->GetGroupMembers();
                        uint32 critical = 0;
                        for (auto* m : group) if (m && m->GetHealthPct() < 40.0f) critical++;
                        return critical >= 3;
                    }),
                    bot::ai::Action("Cast Emerald Communion", [this](Player* bot, Unit*) {
                        if (this->CanCastSpell(EMERALD_COMMUNION, bot))
                        {
                            this->CastSpell(EMERALD_COMMUNION, bot);
                            return NodeStatus::SUCCESS;
                        }
                        return NodeStatus::FAILURE;
                    })
                }),

                // Tier 2: Empowered Heals
                Sequence("Empowered Heals", {
                    Condition("3+ essence", [this](Player*, Unit*) {
                        return this->_resource.essence >= 3;
                    }),
                    Condition("Not channeling", [this](Player*, Unit*) {
                        return !this->_empowermentTracker.IsChanneling();
                    }),
                    Selector("Cast empowered", {
                        Sequence("Spirit Bloom Critical", {
                            Condition("Ally < 50%", [this](Player*, Unit*) {
                                auto group = this->GetGroupMembers();
                                for (auto* m : group) if (m && m->GetHealthPct() < 50.0f) return true;
                                return false;
                            }),
                            bot::ai::Action("Cast Spirit Bloom", [this](Player*, Unit*) {
                                auto group = this->GetGroupMembers();
                                Unit* target = this->GetLowestHealthTarget(group);
                                if (target && this->CanCastSpell(SPIRIT_BLOOM, target))
                                {
                                    this->StartEmpoweredSpell(SPIRIT_BLOOM, EmpowerLevelPres::RANK_3, target);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Dream Breath AoE", {
                            Condition("3+ injured", [this](Player*, Unit*) {
                                auto group = this->GetGroupMembers();
                                uint32 injured = 0;
                                for (auto* m : group) if (m && m->GetHealthPct() < 85.0f) injured++;
                                return injured >= 3;
                            }),
                            bot::ai::Action("Cast Dream Breath", [this](Player*, Unit*) {
                                auto group = this->GetGroupMembers();
                                Unit* target = this->GetMostInjuredTarget(group);
                                if (target && this->CanCastSpell(DREAM_BREATH, target))
                                {
                                    this->StartEmpoweredSpell(DREAM_BREATH, EmpowerLevelPres::RANK_2, target);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 3: Echo Maintenance
                Sequence("Echo Maintenance", {
                    Condition("< 4 echoes", [this](Player*, Unit*) {
                        return this->_echoTracker.GetActiveEchoCount() < 4;
                    }),
                    Condition("Has essence", [this](Player*, Unit*) {
                        return this->_resource.essence >= 1;
                    }),
                    bot::ai::Action("Cast Reversion", [this](Player*, Unit*) {
                        auto group = this->GetGroupMembers();
                        for (Unit* member : group)
                        {
                            if (member && member->GetHealthPct() < 95.0f && !this->_echoTracker.HasEcho(member->GetGUID()))
                            {
                                if (this->CanCastSpell(REVERSION, member))
                                {
                                    this->CastSpell(REVERSION, member);
                                    this->_resource.Consume(1);
                                    this->_echoTracker.CreateEcho(member->GetGUID(), 5000, 4);
                                    return NodeStatus::SUCCESS;
                                }
                            }
                        }
                        return NodeStatus::FAILURE;
                    })
                }),

                // Tier 4: Direct Healing
                Sequence("Direct Healing", {
                    Condition("Has essence", [this](Player*, Unit*) {
                        return this->_resource.essence >= 1;
                    }),
                    Selector("Cast heals", {
                        Sequence("Emerald Blossom AoE", {
                            Condition("3+ injured", [this](Player*, Unit*) {
                                if (this->_resource.essence < 3) return false;
                                auto group = this->GetGroupMembers();
                                uint32 injured = 0;
                                for (auto* m : group) if (m && m->GetHealthPct() < 80.0f) injured++;
                                return injured >= 3;
                            }),
                            bot::ai::Action("Cast Emerald Blossom", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(EMERALD_BLOSSOM, bot))
                                {
                                    this->CastSpell(EMERALD_BLOSSOM, bot);
                                    this->_resource.Consume(3);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Verdant Embrace", {
                            bot::ai::Action("Cast Verdant Embrace", [this](Player*, Unit*) {
                                Unit* target = this->GetLowestHealthTarget(this->GetGroupMembers());
                                if (target && target->GetHealthPct() < 70.0f && this->CanCastSpell(VERDANT_EMBRACE, target))
                                {
                                    this->CastSpell(VERDANT_EMBRACE, target);
                                    this->_resource.Consume(1);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 5: Generate Essence
                Sequence("Generate Essence", {
                    Condition("< 3 essence", [this](Player*, Unit*) {
                        return this->_resource.essence < 3;
                    }),
                    bot::ai::Action("Cast Azure Strike", [this](Player*, Unit*) {
                        Unit* target = this->FindNearbyEnemy();
                        if (target && this->CanCastSpell(AZURE_STRIKE_PRES, target))
                        {
                            this->CastSpell(AZURE_STRIKE_PRES, target);
                            this->_resource.essence = ::std::min(this->_resource.essence + 2, this->_resource.maxEssence);
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
    PreservationEmpowermentTracker _empowermentTracker;
    EchoTracker _echoTracker;
    uint32 _essenceBurstStacks;

    // Hero talent detection cache (refreshed on combat start)
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot
