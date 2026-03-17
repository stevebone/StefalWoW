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

#ifndef PLAYERBOT_DISCIPLINEPRIESTREFACTORED_H
#define PLAYERBOT_DISCIPLINEPRIESTREFACTORED_H

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
// WoW 12.0 (The War Within) - Discipline Priest Spell IDs
// Using central registry: WoW120Spells::Priest and WoW120Spells::Priest::Discipline
constexpr uint32 DISC_SMITE = WoW120Spells::Priest::SMITE;
constexpr uint32 DISC_PENANCE = WoW120Spells::Priest::Discipline::PENANCE;
constexpr uint32 DISC_POWER_WORD_SHIELD = WoW120Spells::Priest::POWER_WORD_SHIELD;
constexpr uint32 DISC_SHADOW_MEND = WoW120Spells::Priest::Discipline::SHADOW_MEND;
constexpr uint32 DISC_PURGE_WICKED = WoW120Spells::Priest::Discipline::PURGE_THE_WICKED;
constexpr uint32 DISC_POWER_WORD_RADIANCE = WoW120Spells::Priest::Discipline::POWER_WORD_RADIANCE;
constexpr uint32 DISC_RAPTURE = WoW120Spells::Priest::Discipline::RAPTURE;
constexpr uint32 DISC_PAIN_SUPPRESSION = WoW120Spells::Priest::Discipline::PAIN_SUPPRESSION;
constexpr uint32 DISC_BARRIER = WoW120Spells::Priest::Discipline::POWER_WORD_BARRIER;
constexpr uint32 DISC_EVANGELISM = WoW120Spells::Priest::Discipline::EVANGELISM;
constexpr uint32 DISC_SCHISM = WoW120Spells::Priest::Discipline::SCHISM;
constexpr uint32 DISC_MINDGAMES = WoW120Spells::Priest::Discipline::MINDGAMES;
constexpr uint32 DISC_SHADOW_COVENANT = WoW120Spells::Priest::Discipline::SHADOW_COVENANT;
constexpr uint32 DISC_POWER_WORD_LIFE = WoW120Spells::Priest::Discipline::POWER_WORD_LIFE;
constexpr uint32 DISC_PURIFY = WoW120Spells::Priest::PURIFY;
constexpr uint32 DISC_SHADOW_WORD_PAIN = WoW120Spells::Priest::SHADOW_WORD_PAIN;
constexpr uint32 DISC_FADE = WoW120Spells::Priest::FADE;
constexpr uint32 DISC_DESPERATE_PRAYER = WoW120Spells::Priest::DESPERATE_PRAYER;
constexpr uint32 DISC_POWER_WORD_FORTITUDE = WoW120Spells::Priest::POWER_WORD_FORTITUDE;

// Atonement tracker - tracks which allies have Atonement buff for damage-to-healing conversion
class AtonementTracker
{
public:
    AtonementTracker() = default;

    void ApplyAtonement(ObjectGuid guid, uint32 duration = 15000)
    {
        _atonementTargets[guid] = GameTime::GetGameTimeMS() + duration;
    }

    void RemoveAtonement(ObjectGuid guid)
    {
        _atonementTargets.erase(guid);
    }

    [[nodiscard]] bool HasAtonement(ObjectGuid guid) const
    {
        auto it = _atonementTargets.find(guid);
        if (it == _atonementTargets.end())

            return false;
        return GameTime::GetGameTimeMS() < it->second;
    }

    [[nodiscard]] uint32 GetAtonementTimeRemaining(ObjectGuid guid) const
    {
        auto it = _atonementTargets.find(guid);
        if (it == _atonementTargets.end())

            return 0;

        uint32 now = GameTime::GetGameTimeMS();
        if (now >= it->second)

            return 0;

        return it->second - now;
    }

    [[nodiscard]] bool NeedsAtonementRefresh(ObjectGuid guid, uint32 refreshWindow = 3000) const
    {
        uint32 remaining = GetAtonementTimeRemaining(guid);
        return remaining < refreshWindow;
    }

    [[nodiscard]] uint32 GetActiveAtonementCount() const
    {
        uint32 count = 0;
        uint32 now = GameTime::GetGameTimeMS();
        for (const auto& pair : _atonementTargets)
        {

            if (now < pair.second)

                ++count;
        }
        return count;
    }

    void Update(Player* bot)
    {
        if (!bot)

            return;

        // Clean up expired Atonements
        uint32 now = GameTime::GetGameTimeMS();
        for (auto it = _atonementTargets.begin(); it != _atonementTargets.end();)
        {

            if (now >= it->second)

                it = _atonementTargets.erase(it);

            else

                ++it;
        }
    }

private:
    CooldownManager _cooldowns;
    ::std::unordered_map<ObjectGuid, uint32> _atonementTargets; // GUID -> expiration time
};

// Power Word: Shield tracker
class PowerWordShieldTracker
{
public:
    PowerWordShieldTracker() = default;

    void ApplyShield(ObjectGuid guid, uint32 duration = 15000)
    {
        _shieldTargets[guid] = GameTime::GetGameTimeMS() + duration;
    }

    void RemoveShield(ObjectGuid guid)
    {
        _shieldTargets.erase(guid);
    }

    [[nodiscard]] bool HasShield(ObjectGuid guid) const
    {
        auto it = _shieldTargets.find(guid);
        if (it == _shieldTargets.end())

            return false;
        return GameTime::GetGameTimeMS() < it->second;
    }

    void Update(Player* bot)
    {
        if (!bot)

            return;

        // Clean up expired shields
        uint32 now = GameTime::GetGameTimeMS();
        for (auto it = _shieldTargets.begin(); it != _shieldTargets.end();)
        {

            if (now >= it->second)

                it = _shieldTargets.erase(it);

            else

                ++it;
        }
    }

private:
    ::std::unordered_map<ObjectGuid, uint32> _shieldTargets; // GUID -> expiration time
};

class DisciplinePriestRefactored : public HealerSpecialization<ManaResource>
{
public:
    using Base = HealerSpecialization<ManaResource>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::_resource;
    explicit DisciplinePriestRefactored(Player* bot)
        : HealerSpecialization<ManaResource>(bot)
        , _atonementTracker()
        , _shieldTracker()
        , _raptureActive(false)
        , _raptureEndTime(0)
        , _lastEvangelismTime(0)
        , _lastPainSuppressionTime(0)
    {
        // Register cooldowns for major abilities
        // COMMENTED OUT:         _cooldowns.RegisterBatch({
        // COMMENTED OUT: 
        // COMMENTED OUT:             {DISC_POWER_WORD_BARRIER, 180000, 1},
        // COMMENTED OUT: 
        // COMMENTED OUT:             {DISC_RAPTURE, 90000, 1},
        // COMMENTED OUT: 
        // COMMENTED OUT:             {DISC_PAIN_SUPPRESSION, 180000, 1},
        // COMMENTED OUT: 
        // COMMENTED OUT:             {DISC_EVANGELISM, 90000, 1},
        // COMMENTED OUT: 
        // COMMENTED OUT:             {DISC_SHADOW_FIEND, 180000, 1}
        // COMMENTED OUT:         });

        // Initialize Phase 5 systems
        InitializeDisciplineMechanics();

        // Register healing spell efficiency tiers
        GetEfficiencyManager().RegisterSpell(DISC_POWER_WORD_SHIELD, HealingSpellTier::VERY_HIGH, "Power Word: Shield");
        GetEfficiencyManager().RegisterSpell(DISC_SHADOW_MEND, HealingSpellTier::HIGH, "Shadow Mend");
        GetEfficiencyManager().RegisterSpell(DISC_PENANCE, HealingSpellTier::MEDIUM, "Penance");
        GetEfficiencyManager().RegisterSpell(DISC_POWER_WORD_RADIANCE, HealingSpellTier::LOW, "Power Word: Radiance");
        GetEfficiencyManager().RegisterSpell(DISC_POWER_WORD_LIFE, HealingSpellTier::MEDIUM, "Power Word: Life");
        GetEfficiencyManager().RegisterSpell(DISC_PAIN_SUPPRESSION, HealingSpellTier::EMERGENCY, "Pain Suppression");
        GetEfficiencyManager().RegisterSpell(DISC_BARRIER, HealingSpellTier::EMERGENCY, "Power Word: Barrier");

        TC_LOG_DEBUG("playerbot", "DisciplinePriestRefactored initialized for bot {}", this->GetBot()->GetGUID().GetCounter());
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
        if (_heroTalents.IsTree(HeroTalentTree::ORACLE))
        {
            // Oracle: Premonition for predictive healing
            if (this->CanCastSpell(WoW120Spells::Priest::Discipline::PREMONITION, this->GetBot()))
            {
                this->CastSpell(WoW120Spells::Priest::Discipline::PREMONITION, this->GetBot());
                return;
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::VOIDWEAVER))
        {
            // Voidweaver: Void Blast for shadow-enhanced atonement damage
            if (target && target->IsHostileTo(this->GetBot()) &&
                this->CanCastSpell(WoW120Spells::Priest::Discipline::VOID_BLAST, target))
            {
                this->CastSpell(WoW120Spells::Priest::Discipline::VOID_BLAST, target);
                return;
            }
        }

        UpdateDisciplineState();

        // Discipline is a healer - check group health first

        if (Group* group = bot->GetGroup())
        {
        ::std::vector<Unit*> groupMembers;

            for (GroupReference const& ref : group->GetMembers())

            {

                if (Player* member = ref.GetSource())
                {

                    if (member->IsAlive() && bot->IsInMap(member))

                        groupMembers.push_back(member);

                }

            }


            if (!groupMembers.empty())

            {

                if (HandleGroupHealing(groupMembers))

                    return;

            }
        }

        // Solo healing (self)
        if (bot->GetHealthPct() < 80.0f)
        {

            if (HandleSelfHealing())

                return;
        }

        // Deal damage to trigger Atonement healing
        ExecuteAtonementDamageRotation(target);
    }

    void UpdateBuffs() override
    {
        Player* bot = this->GetBot();
        if (!bot)

            return;

        // Power Word: Fortitude (group buff)
        if (!bot->HasAura(DISC_POWER_WORD_FORTITUDE))
        {

            if (this->CanCastSpell(DISC_POWER_WORD_FORTITUDE, bot))

            {

                this->CastSpell(DISC_POWER_WORD_FORTITUDE, bot);

            }
        }
    }

    void UpdateDefensives()
    {
        Player* bot = this->GetBot();
        if (!bot)

            return;

        float healthPct = bot->GetHealthPct();        // Desperate Prayer (self-heal + damage reduction)
        if (healthPct < 30.0f && this->CanCastSpell(DISC_DESPERATE_PRAYER, bot))
        {

            this->CastSpell(DISC_DESPERATE_PRAYER, bot);

            return;
        }

        // Fade (threat reduction)
        if (healthPct < 50.0f && bot->GetThreatManager().GetThreatListSize() > 0)
        {

            if (this->CanCastSpell(DISC_FADE, bot))

            {

                this->CastSpell(DISC_FADE, bot);

                return;

            }
        }

        // Power Word: Shield (self)        if (healthPct < 60.0f && !_shieldTracker.HasShield(bot->GetGUID()))
        {

            if (this->CanCastSpell(DISC_POWER_WORD_SHIELD, bot))

            {

                this->CastSpell(DISC_POWER_WORD_SHIELD, bot);
                _shieldTracker.ApplyShield(bot->GetGUID(), 15000);
                _atonementTracker.ApplyAtonement(bot->GetGUID(), 15000);

                return;

            }
        }
    }

private:
        void UpdateDisciplineState()
    {
        Player* bot = this->GetBot();
        if (!bot)

            return;

        _atonementTracker.Update(bot);
        _shieldTracker.Update(bot);
        UpdateCooldownStates();
    }

    void UpdateCooldownStates()
    {
        Player* bot = this->GetBot();
        if (!bot)

            return;

        // Rapture state (free shields)
        if (_raptureActive && GameTime::GetGameTimeMS() >= _raptureEndTime)

            _raptureActive = false;

        if (bot->HasAura(DISC_RAPTURE))
        {

            _raptureActive = true;

            if (Aura* aura = bot->GetAura(DISC_RAPTURE))

                _raptureEndTime = GameTime::GetGameTimeMS() + aura->GetDuration();
                }
    }

    bool HandleGroupHealing(const ::std::vector<Unit*>& group)
    {
        // Emergency cooldowns
        if (HandleEmergencyCooldowns(group))

            return true;

        // Maintain Atonement on injured allies
        if (HandleAtonementMaintenance(group))

            return true;

        // Direct healing when Atonement isn't enough
        if (HandleDirectHealing(group))

            return true;

        // Apply shields
        if (HandleShielding(group))

            return true;

        return false;
    }

    bool HandleEmergencyCooldowns(const ::std::vector<Unit*>& group)
    {
        Player* bot = this->GetBot();
        if (!bot)

            return false;

        // Pain Suppression (critical tank save)
        for (Unit* member : group)
        {

            if (member && member->GetHealthPct() < 20.0f && IsTankRole(member))

            {

                if ((GameTime::GetGameTimeMS() - _lastPainSuppressionTime) >= 180000) // 3 min CD

                {

                    if (this->CanCastSpell(DISC_PAIN_SUPPRESSION, member))

                    {

                        this->CastSpell(DISC_PAIN_SUPPRESSION, member);

                        _lastPainSuppressionTime = GameTime::GetGameTimeMS();

                        return true;

                    }

                }

            }
        }

        // Power Barrier (raid-wide damage reduction)
        uint32 lowHealthCount = 0;
        for (Unit* member : group)
        {

            if (member && member->GetHealthPct() < 50.0f)

                ++lowHealthCount;
        }

        if (lowHealthCount >= 3 && (GameTime::GetGameTimeMS() - _lastBarrierTime) >= 180000) // 3 min CD
        {

            if (this->CanCastSpell(DISC_BARRIER, bot))

            {

                this->CastSpell(DISC_BARRIER, bot); // Ground-targeted AoE

                _lastBarrierTime = GameTime::GetGameTimeMS();

                return true;
                }
        }

        // Rapture (spam shields for heavy damage)
        if (lowHealthCount >= 4 && (GameTime::GetGameTimeMS() - _lastRaptureTime) >= 90000) // 90 sec CD
        {

            if (this->CanCastSpell(DISC_RAPTURE, bot))

            {

                this->CastSpell(DISC_RAPTURE, bot);

                _raptureActive = true;
                _raptureEndTime = GameTime::GetGameTimeMS() + 8000; // 8 sec duration

                _lastRaptureTime = GameTime::GetGameTimeMS();
                return true;
                }
        }

        return false;
    }

    bool HandleAtonementMaintenance(const ::std::vector<Unit*>& group)
    {
        Player* bot = this->GetBot();
        if (!bot)

            return false;

        uint32 activeAtonements = _atonementTracker.GetActiveAtonementCount();

        // Evangelism (extend all Atonements by 6 sec)        if (activeAtonements >= 4 && (GameTime::GetGameTimeMS() - _lastEvangelismTime) >= 90000) // 90 sec CD
        {

            if (bot->HasSpell(DISC_EVANGELISM))

            {

                if (this->CanCastSpell(DISC_EVANGELISM, bot))

                {

                    this->CastSpell(DISC_EVANGELISM, bot);

                    _lastEvangelismTime = GameTime::GetGameTimeMS();

                    // Extend all current Atonements

                    for (Unit* member : group)
                    {

                        if (member && _atonementTracker.HasAtonement(member->GetGUID()))

                        {

                            uint32 remaining = _atonementTracker.GetAtonementTimeRemaining(member->GetGUID());
                            _atonementTracker.ApplyAtonement(member->GetGUID(), remaining + 6000);

                        }

                    }

                    return true;

                }

            }
        }

        // Power Word: Radiance (AoE Atonement application)
        if (activeAtonements < 3)
        {

            for (Unit* member : group)
            {

                if (member && member->GetHealthPct() < 90.0f &&
                !_atonementTracker.HasAtonement(member->GetGUID()))

                {

                    if (IsHealAllowedByMana(DISC_POWER_WORD_RADIANCE) && this->CanCastSpell(DISC_POWER_WORD_RADIANCE, member))

                    {

                        this->CastSpell(DISC_POWER_WORD_RADIANCE, member);
                        // Radiance applies Atonement to 5 nearby allies

                        _atonementTracker.ApplyAtonement(member->GetGUID(), 15000);

                        return true;

                    }

                }

            }
        }

        // Apply Atonement via Power Word: Shield on injured allies

        for (Unit* member : group)
        {

            if (member && member->GetHealthPct() < 85.0f)

            {

                if (_atonementTracker.NeedsAtonementRefresh(member->GetGUID()) &&
                !_shieldTracker.HasShield(member->GetGUID()))
                {

                    if (this->CanCastSpell(DISC_POWER_WORD_SHIELD, member))

                    {

                        this->CastSpell(DISC_POWER_WORD_SHIELD, member);

                        _shieldTracker.ApplyShield(member->GetGUID(), 15000);

                        _atonementTracker.ApplyAtonement(member->GetGUID(), 15000);

                        return true;

                    }

                }

            }
        }        return false;
    }

    bool HandleDirectHealing(const ::std::vector<Unit*>& group)
    {        Player* bot = this->GetBot();
    if (!bot)

            return false;

        // Shadow Mend for emergency direct healing
        for (Unit* member : group)
        {

            if (member && member->GetHealthPct() < 50.0f)

            {

                if (IsHealAllowedByMana(DISC_SHADOW_MEND) && this->CanCastSpell(DISC_SHADOW_MEND, member))

                {

                    this->CastSpell(DISC_SHADOW_MEND, member);

                    _atonementTracker.ApplyAtonement(member->GetGUID(), 15000);

                    return true;

                }

            }
        }

        // Power Word: Life (instant emergency heal)
        for (Unit* member : group)
        {

            if (member && member->GetHealthPct() < 35.0f)

            {
            if (bot->HasSpell(DISC_POWER_WORD_LIFE))

                {

                    if (IsHealAllowedByMana(DISC_POWER_WORD_LIFE) && this->CanCastSpell(DISC_POWER_WORD_LIFE, member))

                    {

                        this->CastSpell(DISC_POWER_WORD_LIFE, member);

                        return true;

                    }

                }

            }
        }

        // Penance (channeled heal/damage - use for healing if needed)
        for (Unit* member : group)
        {

            if (member && member->GetHealthPct() < 60.0f)

            {

                if (IsHealAllowedByMana(DISC_PENANCE) && this->CanCastSpell(DISC_PENANCE, member))

                {

                    this->CastSpell(DISC_PENANCE, member);

                    return true;

                }

            }
        }

        return false;
    }

    bool HandleShielding(const ::std::vector<Unit*>& group)
    {
        // During Rapture, spam shields on everyone
        if (_raptureActive)
        {

            for (Unit* member : group)
            {

                if (member && !_shieldTracker.HasShield(member->GetGUID()))

                {

                    if (this->CanCastSpell(DISC_POWER_WORD_SHIELD, member))

                    {

                        this->CastSpell(DISC_POWER_WORD_SHIELD, member);

                        _shieldTracker.ApplyShield(member->GetGUID(), 15000);
                        _atonementTracker.ApplyAtonement(member->GetGUID(), 15000);

                        return true;
                        }

                }

            }
        }

        // Normal shielding for tanks and injured allies
        for (Unit* member : group)
        {

            if (member && (IsTankRole(member) || member->GetHealthPct() < 75.0f))

            {

                if (!_shieldTracker.HasShield(member->GetGUID()))

                {

                    if (this->CanCastSpell(DISC_POWER_WORD_SHIELD, member))

                    {

                        this->CastSpell(DISC_POWER_WORD_SHIELD, member);

                        _shieldTracker.ApplyShield(member->GetGUID(), 15000);

                        _atonementTracker.ApplyAtonement(member->GetGUID(), 15000);

                        return true;

                    }

                }

            }
        }

        return false;
    }    bool HandleSelfHealing()
    {
        Player* bot = this->GetBot();
        if (!bot)

            return false;

        // Power Word: Shield
        if (!_shieldTracker.HasShield(bot->GetGUID()))
        {

            if (this->CanCastSpell(DISC_POWER_WORD_SHIELD, bot))

            {

                this->CastSpell(DISC_POWER_WORD_SHIELD, bot);

                _shieldTracker.ApplyShield(bot->GetGUID(), 15000);

                _atonementTracker.ApplyAtonement(bot->GetGUID(), 15000);

                return true;

            }
        }

        // Shadow Mend
        if (bot->GetHealthPct() < 60.0f)
        {

            if (IsHealAllowedByMana(DISC_SHADOW_MEND) && this->CanCastSpell(DISC_SHADOW_MEND, bot))

            {

                this->CastSpell(DISC_SHADOW_MEND, bot);

                return true;

            }
        }

        // Penance (self-heal)
        if (bot->GetHealthPct() < 70.0f)
        {

            if (IsHealAllowedByMana(DISC_PENANCE) && this->CanCastSpell(DISC_PENANCE, bot))

            {

                this->CastSpell(DISC_PENANCE, bot);

                return true;

            }
        }

        return false;
    }

    void ExecuteAtonementDamageRotation(::Unit* target)
    {
        Player* bot = this->GetBot();
        if (!bot || !target)

            return;

        // Deal damage to trigger Atonement healing (55% of damage done heals allies with Atonement)

        // Schism (damage amplification debuff)
        if (bot->HasSpell(DISC_SCHISM))
        {

            if (this->CanCastSpell(DISC_SCHISM, target))

            {

                this->CastSpell(DISC_SCHISM, target);

                return;

            }
        }

        // Mindgames (damage + healing reversal)
        if (bot->HasSpell(DISC_MINDGAMES))
        {

            if (this->CanCastSpell(DISC_MINDGAMES, target))

            {

                this->CastSpell(DISC_MINDGAMES, target);

                return;

            }
        }

        // Penance (offensive - high damage for Atonement)
        if (this->CanCastSpell(DISC_PENANCE, target))
        {

            this->CastSpell(DISC_PENANCE, target);

            return;
        }

        // Purge the Wicked (DoT - continuous Atonement healing)
        if (bot->HasSpell(DISC_PURGE_WICKED))
        {
            if (!target->HasAura(DISC_PURGE_WICKED))

            {

                if (this->CanCastSpell(DISC_PURGE_WICKED, target))

                {

                    this->CastSpell(DISC_PURGE_WICKED, target);

                    return;

                }

            }
        }
        else
        {
            // Shadow Word: Pain (alternative DoT)
            if (!target->HasAura(DISC_SHADOW_WORD_PAIN))

            {

                if (this->CanCastSpell(DISC_SHADOW_WORD_PAIN, target))

                {

                    this->CastSpell(DISC_SHADOW_WORD_PAIN, target);

                    return;

                }

            }
        }

        // Smite (filler)
        if (this->CanCastSpell(DISC_SMITE, target))
        {

            this->CastSpell(DISC_SMITE, target);

            return;
        }
    }

    [[nodiscard]] bool IsTankRole(Unit* unit) const
    {
        if (!unit)

            return false;

        if (Player* player = unit->ToPlayer())
        {
            // Simplified tank detection - check if player is currently tanking
            // A more robust implementation would check spec, but talent API is deprecated

            if (Unit* victim = player->GetVictim())

                return victim->GetVictim() == player;
                return false;
        }

        return false;
    }

    void InitializeDisciplineMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)
        // ========================================================================
        // PHASE 5 INTEGRATION: ActionPriorityQueue (Disc Healer + Atonement)
        // ========================================================================
        BotAI* ai = this;
        if (!ai)

            return;

        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            // ====================================================================
            // EMERGENCY TIER - Life-saving abilities
            // ====================================================================

            queue->RegisterSpell(DISC_PAIN_SUPPRESSION,

                SpellPriority::EMERGENCY,

                SpellCategory::DEFENSIVE);

            queue->AddCondition(DISC_PAIN_SUPPRESSION,

                [this](Player* bot, Unit* target) {
                    // Tank save < 20%

                    return target && target->GetHealthPct() < 20.0f &&

                           this->IsTankRole(target);

                },

                "Tank < 20% (Pain Suppression)");


            queue->RegisterSpell(DISC_DESPERATE_PRAYER,

                SpellPriority::EMERGENCY,

                SpellCategory::DEFENSIVE);

            queue->AddCondition(DISC_DESPERATE_PRAYER,

                [](Player* bot, Unit*) {

                    return bot->GetHealthPct() < 30.0f;

                },

                "Self HP < 30%");


            queue->RegisterSpell(DISC_POWER_WORD_SHIELD,

                SpellPriority::EMERGENCY,

                SpellCategory::DEFENSIVE);

            queue->AddCondition(DISC_POWER_WORD_SHIELD,

                [this](Player* bot, Unit* target) {
                    // Emergency shield for critical HP

                    return target && target->GetHealthPct() < 20.0f &&

                           !this->_shieldTracker.HasShield(target->GetGUID());

                },

                "Target < 20% and no shield");

            // ====================================================================
            // CRITICAL TIER - Fast heals and major cooldowns
            // ====================================================================

            queue->RegisterSpell(DISC_SHADOW_MEND,

                SpellPriority::CRITICAL,

                SpellCategory::HEALING);

            queue->AddCondition(DISC_SHADOW_MEND,

                [](Player* bot, Unit* target) {

                    return target && target->GetHealthPct() < 50.0f;

                },

                "Target < 50% (fast heal)");


            queue->RegisterSpell(DISC_PENANCE,

                SpellPriority::CRITICAL,

                SpellCategory::HEALING);

            queue->AddCondition(DISC_PENANCE,

                [](Player* bot, Unit* target) {
                    // Penance for healing or damage

                    return target && (target->IsHostileTo(bot) ||

                           target->GetHealthPct() < 60.0f);

                },

                "Target < 60% or hostile");


            queue->RegisterSpell(DISC_POWER_WORD_RADIANCE,

                SpellPriority::CRITICAL,

                SpellCategory::HEALING);

            queue->AddCondition(DISC_POWER_WORD_RADIANCE,

                [this](Player* bot, Unit*) {
                    // AoE Atonement application

                    return this->_atonementTracker.GetActiveAtonementCount() < 3;

                },

                "< 3 active Atonements (AoE)");


            queue->RegisterSpell(DISC_RAPTURE,

                SpellPriority::CRITICAL,

                SpellCategory::DEFENSIVE);

            queue->AddCondition(DISC_RAPTURE,

                [](Player* bot, Unit*) {
                    // Heavy damage - spam shields

                    Group* group = bot->GetGroup();

                    if (!group)

                        return false;


                    uint32 injured = 0;

                    for (GroupReference const& ref : group->GetMembers())

                    {

                        if (Player* member = ref.GetSource())

                        {

                            if (member->IsAlive() && member->GetHealthPct() < 50.0f)

                                injured++;

                        }

                    }

                    return injured >= 4;

                },

                "4+ injured (mass shields)");

            // ====================================================================
            // HIGH TIER - Atonement application and maintenance
            // ====================================================================

            queue->RegisterSpell(DISC_SCHISM,

                SpellPriority::HIGH,

                SpellCategory::DAMAGE_SINGLE);

            queue->AddCondition(DISC_SCHISM,

                [this](Player* bot, Unit* target) {
                    // Damage amplification for Atonement

                    return target && this->_atonementTracker.GetActiveAtonementCount() >= 2;

                },

                "2+ Atonements active (amp damage)");


            queue->RegisterSpell(DISC_EVANGELISM,

                SpellPriority::HIGH,

                SpellCategory::UTILITY);

            queue->AddCondition(DISC_EVANGELISM,

                [this](Player* bot, Unit*) {
                    // Extend all Atonements

                    return this->_atonementTracker.GetActiveAtonementCount() >= 4;

                },

                "4+ Atonements (extend)");


            queue->RegisterSpell(DISC_MINDGAMES,

                SpellPriority::HIGH,

                SpellCategory::DAMAGE_SINGLE);

            queue->AddCondition(DISC_MINDGAMES,

                [this](Player* bot, Unit* target) {
                    // Burst damage for Atonement

                    return target && this->_atonementTracker.GetActiveAtonementCount() >= 2;

                },

                "2+ Atonements (burst)");

            // ====================================================================
            // MEDIUM TIER - Standard rotation
            // ====================================================================

            queue->RegisterSpell(DISC_PURGE_WICKED,

                SpellPriority::MEDIUM,

                SpellCategory::DAMAGE_SINGLE);

            queue->AddCondition(DISC_PURGE_WICKED,

                [](Player* bot, Unit* target) {

                    return target && !target->HasAura(DISC_PURGE_WICKED);

                },

                "DoT not active");


            queue->RegisterSpell(DISC_SHADOW_WORD_PAIN,

                SpellPriority::MEDIUM,

                SpellCategory::DAMAGE_SINGLE);

            queue->AddCondition(DISC_SHADOW_WORD_PAIN,

                [](Player* bot, Unit* target) {

                    return target && !target->HasAura(DISC_SHADOW_WORD_PAIN);

                },

                "DoT not active");


            queue->RegisterSpell(DISC_SMITE,

                SpellPriority::MEDIUM,

                SpellCategory::DAMAGE_SINGLE);

            queue->AddCondition(DISC_SMITE,

                [this](Player* bot, Unit* target) {
                    // Filler damage for Atonement

                    return target && this->_atonementTracker.GetActiveAtonementCount() > 0;

                },

                "Atonement active (filler)");

            // ====================================================================
            // LOW TIER - Utility
            // ====================================================================

            queue->RegisterSpell(DISC_PURIFY,

                SpellPriority::LOW,

                SpellCategory::UTILITY);


            queue->RegisterSpell(DISC_FADE,

                SpellPriority::LOW,

                SpellCategory::DEFENSIVE);

            queue->AddCondition(DISC_FADE,

                [](Player* bot, Unit*) {

                    return bot->GetThreatManager().GetThreatListSize() > 0 &&

                           bot->GetHealthPct() < 50.0f;

                },

                "Has threat and HP < 50%");


            TC_LOG_INFO("module.playerbot", " DISCIPLINE PRIEST: Registered {} spells in ActionPriorityQueue",

                queue->GetSpellCount());
        }

        // ========================================================================
        // PHASE 5 INTEGRATION: BehaviorTree (Disc Healer + Atonement Flow)
        // ========================================================================
        auto* behaviorTree = ai->GetBehaviorTree();
        if (behaviorTree)
        {

            auto root = Selector("Discipline Priest Healer", {
                // ================================================================
                // TIER 1: EMERGENCY HEALING (HP < 30%)
                // ================================================================

                Sequence("Emergency Healing", {

                    Condition("Critical HP < 30%", [](Player* bot, Unit*) {
                        // Check self or group for critical HP

                        if (bot->GetHealthPct() < 30.0f)

                            return true;


                        Group* group = bot->GetGroup();

                        if (!group)

                            return false;


                        for (GroupReference const& ref : group->GetMembers())

                        {

                            if (Player* member = ref.GetSource())

                            {

                                if (member->IsAlive() && member->GetHealthPct() < 30.0f)

                                    return true;

                            }

                        }

                        return false;

                    }),

                    Selector("Emergency Response", {
                        // Pain Suppression for tank

                        bot::ai::Action("Cast Pain Suppression", [this](Player* bot, Unit*) {

                            Group* group = bot->GetGroup();

                            if (!group)

                                return NodeStatus::FAILURE;


                            for (GroupReference const& ref : group->GetMembers())

                            {

                                if (Player* member = ref.GetSource())

                                {

                                    if (member->IsAlive() && member->GetHealthPct() < 20.0f &&

                                        this->IsTankRole(member) &&

                                        this->CanCastSpell(DISC_PAIN_SUPPRESSION, member))

                                    {

                                        this->CastSpell(DISC_PAIN_SUPPRESSION, member);

                                        return NodeStatus::SUCCESS;

                                    }

                                }

                            }

                            return NodeStatus::FAILURE;

                        }),
                        // Desperate Prayer for self

                        bot::ai::Action("Cast Desperate Prayer", [this](Player* bot, Unit*) {

                            if (bot->GetHealthPct() < 30.0f &&

                                this->CanCastSpell(DISC_DESPERATE_PRAYER, bot))

                            {

                                this->CastSpell(DISC_DESPERATE_PRAYER, bot);

                                return NodeStatus::SUCCESS;

                            }

                            return NodeStatus::FAILURE;

                        }),
                        // Shadow Mend emergency spam

                        bot::ai::Action("Cast Shadow Mend", [this](Player* bot, Unit*) {

                            Group* group = bot->GetGroup();

                            if (!group)

                                return NodeStatus::FAILURE;


                            for (GroupReference const& ref : group->GetMembers())

                            {

                                if (Player* member = ref.GetSource())

                                {

                                    if (member->IsAlive() && member->GetHealthPct() < 40.0f &&

                                        this->CanCastSpell(DISC_SHADOW_MEND, member))

                                    {

                                        this->CastSpell(DISC_SHADOW_MEND, member);

                                        this->_atonementTracker.ApplyAtonement(member->GetGUID(), 15000);

                                        return NodeStatus::SUCCESS;

                                    }

                                }

                            }

                            return NodeStatus::FAILURE;

                        })

                    })

                }),

                // ================================================================
                // TIER 2: ATONEMENT MAINTENANCE
                // ================================================================

                Sequence("Atonement Maintenance", {

                    Condition("Has group", [](Player* bot, Unit*) {

                        return bot->GetGroup() != nullptr;

                    }),

                    Selector("Atonement Priority", {
                        // Evangelism - extend all Atonements

                        Sequence("Evangelism", {

                            Condition("4+ Atonements", [this](Player* bot, Unit*) {

                                return this->_atonementTracker.GetActiveAtonementCount() >= 4;

                            }),

                            bot::ai::Action("Cast Evangelism", [this](Player* bot, Unit*) {

                                if (this->CanCastSpell(DISC_EVANGELISM, bot))

                                {

                                    this->CastSpell(DISC_EVANGELISM, bot);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),
                        // Power Word: Radiance - AoE Atonement

                        Sequence("Power Word: Radiance", {

                            Condition("< 3 Atonements", [this](Player* bot, Unit*) {

                                return this->_atonementTracker.GetActiveAtonementCount() < 3;

                            }),

                            bot::ai::Action("Cast Radiance", [this](Player* bot, Unit*) {

                                Group* group = bot->GetGroup();

                                if (!group)

                                    return NodeStatus::FAILURE;


                                for (GroupReference const& ref : group->GetMembers())

                                {

                                    if (Player* member = ref.GetSource())

                                    {

                                        if (member->IsAlive() && member->GetHealthPct() < 90.0f &&

                                            this->CanCastSpell(DISC_POWER_WORD_RADIANCE, member))

                                        {

                                            this->CastSpell(DISC_POWER_WORD_RADIANCE, member);

                                            this->_atonementTracker.ApplyAtonement(member->GetGUID(), 15000);

                                            return NodeStatus::SUCCESS;

                                        }

                                    }

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),
                        // Power Word: Shield - Single target Atonement

                        bot::ai::Action("Cast Shield", [this](Player* bot, Unit*) {

                            Group* group = bot->GetGroup();

                            if (!group)

                                return NodeStatus::FAILURE;


                            for (GroupReference const& ref : group->GetMembers())

                            {

                                if (Player* member = ref.GetSource())

                                {

                                    if (member->IsAlive() && member->GetHealthPct() < 85.0f &&

                                        this->_atonementTracker.NeedsAtonementRefresh(member->GetGUID()) &&

                                        !this->_shieldTracker.HasShield(member->GetGUID()) &&

                                        this->CanCastSpell(DISC_POWER_WORD_SHIELD, member))

                                    {

                                        this->CastSpell(DISC_POWER_WORD_SHIELD, member);

                                        this->_shieldTracker.ApplyShield(member->GetGUID(), 15000);

                                        this->_atonementTracker.ApplyAtonement(member->GetGUID(), 15000);

                                        return NodeStatus::SUCCESS;

                                    }

                                }

                            }

                            return NodeStatus::FAILURE;

                        })

                    })

                }),

                // ================================================================
                // TIER 3: DIRECT HEALING (When Atonement isn't enough)
                // ================================================================

                Sequence("Direct Healing", {

                    Condition("Injured allies", [](Player* bot, Unit*) {

                        Group* group = bot->GetGroup();

                        if (!group)

                            return false;


                        for (GroupReference const& ref : group->GetMembers())

                        {

                            if (Player* member = ref.GetSource())

                            {

                                if (member->IsAlive() && member->GetHealthPct() < 70.0f)

                                    return true;

                            }

                        }

                        return false;

                    }),

                    Selector("Healing Priority", {
                        // Penance for moderate damage

                        bot::ai::Action("Cast Penance Heal", [this](Player* bot, Unit*) {

                            Group* group = bot->GetGroup();

                            if (!group)

                                return NodeStatus::FAILURE;


                            for (GroupReference const& ref : group->GetMembers())

                            {

                                if (Player* member = ref.GetSource())

                                {

                                    if (member->IsAlive() && member->GetHealthPct() < 60.0f &&

                                        this->CanCastSpell(DISC_PENANCE, member))

                                    {

                                        this->CastSpell(DISC_PENANCE, member);

                                        return NodeStatus::SUCCESS;

                                    }

                                }

                            }

                            return NodeStatus::FAILURE;

                        })

                    })

                }),

                // ================================================================
                // TIER 4: ATONEMENT DAMAGE ROTATION
                // ================================================================

                Sequence("Atonement Damage", {

                    Condition("Has Atonements", [this](Player* bot, Unit*) {

                        return this->_atonementTracker.GetActiveAtonementCount() > 0;

                    }),

                    Condition("Has hostile target", [](Player* bot, Unit* target) {

                        return target && target->IsHostileTo(bot);

                    }),

                    Selector("Damage Priority", {
                        // Schism - damage amplification

                        Sequence("Schism", {

                            Condition("2+ Atonements", [this](Player* bot, Unit*) {

                                return this->_atonementTracker.GetActiveAtonementCount() >= 2;

                            }),

                            bot::ai::Action("Cast Schism", [this](Player* bot, Unit* target) {

                                if (this->CanCastSpell(DISC_SCHISM, target))

                                {

                                    this->CastSpell(DISC_SCHISM, target);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),
                        // Mindgames - burst damage

                        bot::ai::Action("Cast Mindgames", [this](Player* bot, Unit* target) {

                            if (this->CanCastSpell(DISC_MINDGAMES, target))

                            {

                                this->CastSpell(DISC_MINDGAMES, target);

                                return NodeStatus::SUCCESS;

                            }

                            return NodeStatus::FAILURE;

                        }),
                        // Penance (offensive)

                        bot::ai::Action("Cast Penance Damage", [this](Player* bot, Unit* target) {

                            if (this->CanCastSpell(DISC_PENANCE, target))

                            {

                                this->CastSpell(DISC_PENANCE, target);

                                return NodeStatus::SUCCESS;

                            }

                            return NodeStatus::FAILURE;

                        }),
                        // Purge the Wicked (DoT)

                        Sequence("Purge the Wicked", {

                            Condition("DoT not active", [](Player* bot, Unit* target) {

                                return target && !target->HasAura(DISC_PURGE_WICKED);

                            }),

                            bot::ai::Action("Cast Purge the Wicked", [this](Player* bot, Unit* target) {

                                if (this->CanCastSpell(DISC_PURGE_WICKED, target))

                                {

                                    this->CastSpell(DISC_PURGE_WICKED, target);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),
                        // Smite (filler)

                        bot::ai::Action("Cast Smite", [this](Player* bot, Unit* target) {

                            if (this->CanCastSpell(DISC_SMITE, target))

                            {

                                this->CastSpell(DISC_SMITE, target);

                                return NodeStatus::SUCCESS;

                            }

                            return NodeStatus::FAILURE;

                        })

                    })

                })

            });


            behaviorTree->SetRoot(root);

            TC_LOG_INFO("module.playerbot", " DISCIPLINE PRIEST: BehaviorTree initialized with Atonement flow");
        }
    }

    // Member variables
    AtonementTracker _atonementTracker;
    PowerWordShieldTracker _shieldTracker;

    bool _raptureActive;
    uint32 _raptureEndTime;
    uint32 _lastEvangelismTime;
    uint32 _lastPainSuppressionTime;
    uint32 _lastBarrierTime;
    uint32 _lastRaptureTime;

    // Hero talent detection cache (refreshed on combat start)
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot

#endif // PLAYERBOT_DISCIPLINEPRIESTREFACTORED_H
